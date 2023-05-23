// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2015 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/of.h>
#include <linux/kdebug.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/sched/clock.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <asm/kexec.h>
#include <asm/memory.h>
#include <asm/stacktrace.h>
#include <asm/system_misc.h>

#include <mrdump.h>
#include <mt-plat/mboot_params.h>
#include "mrdump_mini.h"
#include "mrdump_private.h"

/* for arm_smccc_smc */
#include <linux/arm-smccc.h>
#include <uapi/linux/psci.h>

static inline unsigned long get_linear_memory_size(void)
{
	return (unsigned long)high_memory - PAGE_OFFSET;
}

/* no export symbol to aee_exception_reboot, only used in exception flow */
/* PSCI v1.1 extended power state encoding for SYSTEM_RESET2 function */
#define PSCI_1_1_RESET2_TYPE_VENDOR_SHIFT   31
#define PSCI_1_1_RESET2_TYPE_VENDOR     \
	(1 << PSCI_1_1_RESET2_TYPE_VENDOR_SHIFT)

static void aee_exception_reboot(void)
{
	struct arm_smccc_res res;
	int opt1 = 1, opt2 = 0;

	arm_smccc_smc(PSCI_1_1_FN_SYSTEM_RESET2,
		PSCI_1_1_RESET2_TYPE_VENDOR | opt1,
		opt2, 0, 0, 0, 0, 0, &res);
}

/*save stack as binary into buf,
 *return value

    -1: bottom unaligned
    -2: bottom out of kernel addr space
    -3 top out of kernel addr addr
    -4: buff len not enough
    >0: used length of the buf
 */
int aee_dump_stack_top_binary(char *buf, int buf_len, unsigned long bottom,
		unsigned long top)
{
	/*should check stack address in kernel range */
	if (bottom & 3)
		return -1;
	if (!((bottom >= (PAGE_OFFSET + THREAD_SIZE)) &&
	      (bottom <= (PAGE_OFFSET + get_linear_memory_size())))) {
		if (!((bottom >= VMALLOC_START) && (bottom <= VMALLOC_END)))
			return -2;
	}

	if (!((top >= (PAGE_OFFSET + THREAD_SIZE)) &&
	      (top <= (PAGE_OFFSET + get_linear_memory_size())))) {
		if (!((top >= VMALLOC_START) && (top <= VMALLOC_END)))
			return -3;
	}

	if (top > ALIGN(bottom, THREAD_SIZE))
		top = ALIGN(bottom, THREAD_SIZE);

	if (buf_len < top - bottom)
		return -4;

	memcpy((void *)buf, (void *)bottom, top - bottom);

	return top - bottom;
}

#if defined(CONFIG_RANDOMIZE_BASE) && defined(CONFIG_ARM64)
static inline void show_kaslr(void)
{
	u64 const kaslr_off = kaslr_offset();

	pr_notice("Kernel Offset: 0x%llx from 0x%lx\n",
			kaslr_off, KIMAGE_VADDR);
	pr_notice("PHYS_OFFSET: 0x%llx\n", PHYS_OFFSET);
	aee_rr_rec_kaslr_offset(kaslr_off);
}
#else
static inline void show_kaslr(void)
{
	pr_notice("Kernel Offset: disabled\n");
	aee_rr_rec_kaslr_offset(0xd15ab1e);
}
#endif

static char nested_panic_buf[1024];
int aee_nested_printf(const char *fmt, ...)
{
	va_list args;
	static int total_len;

	va_start(args, fmt);
	total_len += vsnprintf(nested_panic_buf, sizeof(nested_panic_buf),
			fmt, args);
	va_end(args);

	aee_sram_fiq_log(nested_panic_buf);

	return total_len;
}

static volatile int num_die;
static void nested_die_check(int fiq_step)
{
	static volatile int last_step;

	if (num_die <= 1) {
		last_step = fiq_step;
		aee_rr_rec_fiq_step(fiq_step);
	}

	if ((num_die > 1 && fiq_step == last_step) ||
			fiq_step == AEE_FIQ_STEP_COMMON_DIE_DONE){
		aee_nested_printf("nd-%d, fs-%d, ls-%d\n",
				  num_die, fiq_step, last_step);
#if IS_ENABLED(CONFIG_MEDIATEK_CACHE_API)
		dis_D_inner_flush_all();
#else
		pr_info("dis_D_inner_flush_all invalid");
#endif
		aee_exception_reboot();
	}
}

int mrdump_common_die(int fiq_step, int reboot_reason, const char *msg,
		      struct pt_regs *regs)
{
	++num_die;
	nested_die_check(fiq_step);
	show_kaslr();

	nested_die_check(AEE_FIQ_STEP_COMMON_DIE_SCP);
	aee_rr_rec_scp();

	nested_die_check(AEE_FIQ_STEP_COMMON_DIE_MRDUMP);
	__mrdump_create_oops_dump(reboot_reason, regs, msg);

	nested_die_check(AEE_FIQ_STEP_COMMON_DIE_TRACE);
	switch (reboot_reason) {
	case AEE_REBOOT_MODE_KERNEL_OOPS:
		aee_show_regs(regs);
		dump_stack();
		break;
	case AEE_REBOOT_MODE_KERNEL_PANIC:
#ifndef CONFIG_DEBUG_BUGVERBOSE
		dump_stack();
#endif
		break;
	case AEE_REBOOT_MODE_HANG_DETECT:
		aee_rr_rec_exp_type(AEE_EXP_TYPE_HANG_DETECT);
		break;
	default:
		/* Don't print anything */
		break;
	}

	nested_die_check(AEE_FIQ_STEP_COMMON_DIE_REGS);
	mrdump_mini_ke_cpu_regs(regs);

	nested_die_check(AEE_FIQ_STEP_COMMON_DIE_CS);
	aee_zap_locks();
	console_unlock();

	nested_die_check(AEE_FIQ_STEP_COMMON_DIE_DONE);
	return NOTIFY_DONE;
}
EXPORT_SYMBOL(mrdump_common_die);

int ipanic(struct notifier_block *this, unsigned long event, void *ptr)
{
	struct pt_regs saved_regs;
	int fiq_step;

	aee_rr_rec_exp_type(AEE_EXP_TYPE_KE);
	fiq_step = AEE_FIQ_STEP_KE_IPANIC_START;
	crash_setup_regs(&saved_regs, NULL);
	return mrdump_common_die(fiq_step,
				 AEE_REBOOT_MODE_KERNEL_PANIC,
				 "Kernel Panic", &saved_regs);
}

static int ipanic_die(struct notifier_block *self, unsigned long cmd, void *ptr)
{
	struct die_args *dargs = (struct die_args *)ptr;
	int fiq_step;

	aee_rr_rec_exp_type(AEE_EXP_TYPE_KE);
	fiq_step = AEE_FIQ_STEP_KE_IPANIC_DIE;
	return mrdump_common_die(fiq_step,
				 AEE_REBOOT_MODE_KERNEL_OOPS,
				 "Kernel Oops", dargs->regs);
}

static struct notifier_block panic_blk = {
	.notifier_call = ipanic,
};

static struct notifier_block die_blk = {
	.notifier_call = ipanic_die,
};

static __init int mrdump_parse_chosen(struct mrdump_params *mparams)
{
	struct device_node *node;
	u32 reg[2];
	const char *lkver, *ddr_rsv;

	memset(mparams, 0, sizeof(struct mrdump_params));

	node = of_find_node_by_path("/chosen");
	if (node) {
		if (of_property_read_u32_array(node, "mrdump,cblock",
					       reg, ARRAY_SIZE(reg)) == 0) {
			mparams->cb_addr = reg[0];
			mparams->cb_size = reg[1];
			pr_notice("%s: mrdump_cbaddr=%x, mrdump_cbsize=%x\n",
				  __func__, mparams->cb_addr, mparams->cb_size);
		}

		if (of_property_read_string(node, "mrdump,lk", &lkver) == 0) {
			strlcpy(mparams->lk_version, lkver,
				sizeof(mparams->lk_version));
			pr_notice("%s: lk version %s\n", __func__, lkver);
		}

		if (of_property_read_string(node, "mrdump,ddr_rsv",
					    &ddr_rsv) == 0) {
			if (strcmp(ddr_rsv, "yes") == 0)
				mparams->drm_ready = true;
			pr_notice("%s: ddr reserve mode %s\n", __func__,
				  ddr_rsv);
		}

		return 0;
	}
	of_node_put(node);
	pr_notice("%s: Can't find chosen node\n", __func__);
	return -1;
}

static int __init mrdump_panic_init(void)
{
	struct mrdump_params mparams = {};

#ifdef MODULE
	if (mrdump_ka_init()) {
		pr_notice("%s: mrdump helper not available\n", __func__);
		return -EINVAL;
	}
#endif
	mrdump_parse_chosen(&mparams);
#ifdef MODULE
	mrdump_module_init_mboot_params();
#endif
	mrdump_hw_init(mparams.drm_ready);
	mrdump_cblock_init(mparams.cb_addr, mparams.cb_size);
	if (mrdump_cblock == NULL) {
		pr_notice("%s: MT-RAMDUMP no control block\n", __func__);
		return -EINVAL;
	}
	mrdump_mini_init(&mparams);

	if (strcmp(mparams.lk_version, MRDUMP_GO_DUMP) == 0) {
		mrdump_full_init();
	} else {
		pr_notice("%s: Full ramdump disabled, version %s not matched.\n",
			  __func__, mparams.lk_version);
	}

	mrdump_wdt_init();

	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	register_die_notifier(&die_blk);
	pr_debug("ipanic: startup\n");
	return 0;
}

arch_initcall(mrdump_panic_init);

#ifdef MODULE
static void __exit mrdump_panic_exit(void)
{
	atomic_notifier_chain_unregister(&panic_notifier_list, &panic_blk);
	unregister_die_notifier(&die_blk);
	pr_debug("ipanic: exit\n");
}
module_exit(mrdump_panic_exit);
#endif
