// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/uaccess.h>
#include <linux/compat.h>
#include <common/mdla_device.h>
#include <common/mdla_ioctl.h>
#include <common/mdla_cmd_proc.h>
#include <common/mdla_power_ctrl.h>
#include <interface/mdla_cmd_data_v2_0.h>
#include <utilities/mdla_debug.h>
#include <utilities/mdla_util.h>
#include <comm_driver.h>

static int mdla_efuse = 1;
static int do_suspend;
static int mdla_ioctl_block_cmd;

static int (*mdla_ioctl_perf)(struct file *filp, u32 command,
		unsigned long arg, bool need_pwr_on);

static long mdla_ioctl_run_cmd(void *ioctl_cmd, int ioctl_svp)
{
	struct mdla_run_cmd_sync_svp cmd_ut;
	struct ioctl_run_cmd *run_cmd = ioctl_cmd;

	if (!ioctl_svp) {
		memset(&cmd_ut, 0, sizeof(cmd_ut));
		cmd_ut.req.secure = 0;
		cmd_ut.req.pipe_id = UINT_MAX;
		cmd_ut.req.buf[0].size = 8;
		cmd_ut.req.buf[0].iova = run_cmd->mva;
		cmd_ut.req.buf[0].buf_type = EN_MDLA_BUF_TYPE_CODE;
		cmd_ut.req.buf[0].buf_source = MEM_IOMMU;
		cmd_ut.req.buf_count = 1;
		cmd_ut.req.cmd_count = run_cmd->count;
	} else {
		memcpy(&cmd_ut, ioctl_cmd, sizeof(cmd_ut));
	}

	return mdla_cmd_ops_get()->ut_run_sync(&cmd_ut,
			mdla_get_device(0));
}

// ioctl_malloc
static int prepare_malloc_data(struct comm_kmem *mem, void *data, int svp)
{
	struct ioctl_malloc *malloc;
	struct ioctl_malloc_svp *malloc_svp;
	u32 type;

	if (svp) {
		malloc_svp = (struct ioctl_malloc_svp *)data;
		mem->secure = malloc_svp->secure;
		mem->pipe_id = malloc_svp->pipe_id;
		mem->size = malloc_svp->size;
		mem->align = malloc_svp->align;
		type = malloc_svp->mem_type;
		mem->iova = malloc_svp->mva;
		mem->kva = malloc_svp->kva;

	} else {
		malloc = (struct ioctl_malloc *)data;
		mem->pipe_id = 0xFFFFFFFF;
		mem->size = malloc->size;
		mem->align = 0;
		type = malloc->type;
		mem->iova = malloc->mva;
		mem->kva = malloc->kva;
	}

	switch (type) {
	case MEM_DRAM_CACHED:
	case MEM_IOMMU_CACHED:
		mem->cache = 1;
		mem->mem_type = APU_COMM_MEM_DRAM_DMA;
		mem->iova |=  0x200000000;
		break;
	case MEM_DRAM:
	case MEM_IOMMU:
		mem->mem_type = APU_COMM_MEM_DRAM_DMA;
		mem->iova |=  0x200000000;
		break;
	case MEM_GSM:
		mem->mem_type = APU_COMM_MEM_VLM;
		break;
	default:
		mdla_err("unsupport type\n");
		return -EINVAL;
	}

	return 0;
}

static int finish_malloc_data(struct comm_kmem *mem, void *data, int svp)
{
	struct ioctl_malloc *malloc;
	struct ioctl_malloc_svp *malloc_svp;
	u32 type;
	u64 phyaddr;

	phyaddr = mem->iova;

	if (svp) {
		malloc_svp = (struct ioctl_malloc_svp *)data;
		type = malloc_svp->mem_type;
		malloc_svp->kva = (u64)mem->kva;
		malloc_svp->mva = mem->iova & 0xFFFFFFFF;
		malloc_svp->pa = mem->iova;

	} else {
		malloc = (struct ioctl_malloc *)data;
		type = malloc->type;
		malloc->kva = (u64)mem->kva;
		malloc->mva = mem->iova & 0xFFFFFFFF;
		malloc->mva_h = (mem->iova >> 32) & 0xFFFFFFFF;
		malloc->pa = mem->iova;
	}

	return 0;
}

static int mdla_ioctl_mem_alloc(void *ioctl_data, int from_user, int svp)
{
	struct comm_kmem mem;

	memset(&mem, 0, sizeof(mem));

	if (prepare_malloc_data(&mem, ioctl_data, svp))
		return -EINVAL;

	if (comm_util_get_cb()->alloc(&mem)) {
		mdla_err("%s: failed!!\n", __func__);
		return -EFAULT;
	}

	if (from_user)
		mem.kva = 0;

	if (finish_malloc_data(&mem, ioctl_data, svp))
		return -EINVAL;

	return 0;
}

static int mdla_ioctl_mem_free(void *ioctl_data, int svp)
{
	struct comm_kmem mem;

	memset(&mem, 0, sizeof(mem));

	if (prepare_malloc_data(&mem, ioctl_data, svp))
		return -EINVAL;

	if (comm_util_get_cb()->free(&mem, 1)) {
		mdla_err("%s: failed!!\n", __func__);
		return -EFAULT;
	}

	return 0;
}

static int mdla_ioctl_gsm_info(struct ioctl_config *cfg, int from_user)
{
	struct comm_kmem mem;

	mem.mem_type = APU_COMM_MEM_VLM;
	if (comm_util_get_cb()->vlm_info(&mem)) {
		mdla_err("%s: failed!!\n", __func__);
		return -EFAULT;
	}

	cfg->arg[0] = mem.size;
	cfg->arg[1] = mem.iova;
	cfg->arg[2] = mem.phy;

	if (!from_user)
		cfg->arg[3] = mem.kva;

	mdla_drv_debug("%s: size:%llu, phy:%llx, iova:%llx\n", __func__,
			cfg->arg[0],
			cfg->arg[2],
			cfg->arg[1]);

	return 0;
}

static int mdla_map_uva(struct file *filp, struct vm_area_struct *vma)
{
	return comm_util_get_cb()->map_uva(vma);
}

static int mdla_ioctl_pwr_ctrl(enum MDLA_POWER_CTL_OP pctrl)
{
	return 0;
}

static int copy_from_ioctl(void *dst, void *src,
		unsigned int size, unsigned int from_user)
{
	if (from_user) {
		if (copy_from_user(dst, src, size))
			return -EFAULT;
	} else {
		memcpy(dst, src, size);
	}

	return 0;
}

static int copy_to_ioctl(void *dst, void *src,
		unsigned int size, unsigned int from_user)
{
	if (from_user) {
		if (copy_to_user(dst, src, size))
			return -EFAULT;
	} else {
		memcpy(dst, src, size);
	}

	return 0;
}

static long mdla_ioctl_config(void *arg, unsigned int from_user)
{
	long ret = 0;
	struct ioctl_config cfg;

	if (from_user) {
		if (copy_from_user(&cfg, arg, sizeof(cfg)))
			return -EFAULT;
	} else {
		memcpy(&cfg, (void *)arg, sizeof(cfg));
	}

	if (mdla_efuse) {
		if (cfg.op != MDLA_CFG_EFUSE_INFO)
			return -EFAULT;
	}

	switch (cfg.op) {
	case MDLA_CFG_NONE:
		break;
	case MDLA_CFG_TIMEOUT_GET:
		cfg.arg[0] = mdla_dbg_read_u32(FS_TIMEOUT);
		cfg.arg_count = 1;
		break;
	case MDLA_CFG_TIMEOUT_SET:
		if (cfg.arg_count == 1)
			mdla_dbg_write_u32(FS_TIMEOUT, cfg.arg[0]);
		break;
	case MDLA_CFG_FIFO_SZ_GET:
		cfg.arg[0] = 1;
		cfg.arg_count = 1;
		break;
	case MDLA_CFG_FIFO_SZ_SET:
		return -EINVAL;
	case MDLA_CFG_GSM_INFO:
		ret = mdla_ioctl_gsm_info(&cfg, from_user);
		cfg.arg_count = 4;
		break;
	case MDLA_CFG_EFUSE_INFO:
		cfg.arg[0] = !mdla_efuse;
		cfg.arg_count = 1;
		break;
	case MDLA_CFG_POWER_CTL:
		if (cfg.arg_count == 1) {
			if (!(cfg.arg[0] & MDLA_APU_PWR_MSK))
				mdla_ioctl_block_cmd = 1;

			if (!mdla_ioctl_pwr_ctrl(cfg.arg[0]) &&
					cfg.arg[0] & MDLA_APU_PWR_MSK)
				mdla_ioctl_block_cmd = 0;
		}
		break;
	case MDLA_CFG_PWR_STAT_SET:
		ret = mdla_pwr_ops_get()->set_pll(0, cfg.arg[0]);
		break;
	case MDLA_CFG_PWR_STAT_GET:
		ret = mdla_pwr_ops_get()->get_pll(0, &cfg.arg[1],
				&cfg.arg[2], &cfg.arg[3]);
		break;
	default:
		return -EINVAL;
	}

	if (from_user) {
		if (copy_to_user((void *)arg, &cfg, sizeof(cfg)))
			return -EFAULT;
	} else {
		memcpy((void *)arg, &cfg, sizeof(cfg));
	}

	return ret;
}

static long mdla_ioctl_entry(unsigned int command, unsigned long arg, unsigned int from_user)
{
	int ret = 0;
	struct ioctl_malloc malloc_data;
	struct ioctl_run_cmd cmd;
	struct ioctl_run_cmd_sync_svp cmd_svp;
	struct ioctl_malloc_svp malloc_svp;

	if (mdla_efuse && (command != IOCTL_CONFIG)) {
		pr_err("exit(mdla disabled)\n");
		return -EINVAL;
	}

	switch (command) {
	case IOCTL_CONFIG:
		ret = mdla_ioctl_config((void *)arg, from_user);
		break;
	case IOCTL_MALLOC:
		if (copy_from_ioctl(&malloc_data, (void *)arg,
					sizeof(struct ioctl_malloc),
					from_user))
			return -EFAULT;

		if (mdla_ioctl_mem_alloc(&malloc_data, from_user, 0))
			return -EFAULT;

		mdla_drv_debug("ioctl_malloc size:[%d] mva:[0x%x] type:[%d] pid:[%d]\n",
				malloc_data.size,
				malloc_data.mva,
				malloc_data.type,
				current->pid);

		if (copy_to_ioctl((void *)arg, &malloc_data,
					sizeof(struct ioctl_malloc),
					from_user)) {
			mdla_ioctl_mem_free(&malloc_data, 0);

			return -EFAULT;
		}
		break;
	case IOCTL_FREE:
		if (copy_from_ioctl(&malloc_data, (void *)arg,
					sizeof(struct ioctl_malloc),
					from_user))
			return -EFAULT;

		if (mdla_ioctl_mem_free(&malloc_data, 0))
			return -EFAULT;

		mdla_drv_debug("ioctl_free size:[%d] mva:[0x%x] pid:[%d]\n",
				malloc_data.size,
				malloc_data.mva,
				current->pid);
		break;
	case IOCTL_RUN_CMD_SYNC:
		if (copy_from_ioctl(&cmd, (void *)arg,
					sizeof(struct ioctl_run_cmd),
					from_user))
			return -EFAULT;

		mdla_drv_debug("run_cmd_sync mva:[0x%x] count:[%d] pid:[%d]\n",
				cmd.mva,
				cmd.count,
				current->pid);

		ret = mdla_ioctl_run_cmd(&cmd, 0);
		break;
	case IOCTL_MALLOC_SVP:
		if (copy_from_ioctl(&malloc_svp, (void *)arg,
					sizeof(struct ioctl_malloc_svp),
					from_user))
			return -EFAULT;

		mdla_drv_debug("malloc_svp size:[%d] mva:[0x%x] pid:[%d]\n",
				malloc_svp.size,
				malloc_svp.mva,
				current->pid);

		if (mdla_ioctl_mem_alloc(&malloc_svp, from_user, 1))
			return -EFAULT;

		if (copy_to_ioctl((void *)arg, &malloc_svp,
					sizeof(struct ioctl_malloc_svp),
					from_user))
			return -EFAULT;
		break;
	case IOCTL_FREE_SVP:
		if (copy_from_ioctl(&malloc_svp, (void *)arg,
					sizeof(struct ioctl_malloc_svp),
					from_user))
			return -EFAULT;

		mdla_drv_debug("ioctl_free_svp size:[%d] mva:[0x%x] pid:[%d]\n",
				malloc_svp.size,
				malloc_svp.mva,
				current->pid);

		ret = mdla_ioctl_mem_free(&malloc_svp, 1);
		break;
	case IOCTL_RUN_CMD_SYNC_SVP:
		if (copy_from_ioctl(&cmd_svp, (void *)arg,
					sizeof(struct ioctl_run_cmd_sync_svp),
					from_user))
			return -EFAULT;

		mdla_drv_debug("run_cmd_svp secure:[%d] buf_count:[%d] pid:[%d]\n",
				cmd_svp.req.secure,
				cmd_svp.req.buf_count,
				current->pid);

		ret = mdla_ioctl_run_cmd(&cmd_svp, 1);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static long mdla_ioctl(struct file *filp, unsigned int command, unsigned long arg)
{
	return mdla_ioctl_entry(command, arg, 1);
}

long mdla_kernel_ioctl(unsigned int command, void *arg)
{
	if (do_suspend) {
		mdla_drv_debug("suspending\n");
		return -EFAULT;
	}
	return mdla_ioctl_entry(command, (unsigned long)arg, 0);
}
EXPORT_SYMBOL(mdla_kernel_ioctl);

#ifdef CONFIG_COMPAT
static long mdla_compat_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	return mdla_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int mdla_open(struct inode *inodep, struct file *filep)
{
	mdla_drv_debug("%s(): Device has been opened\n", __func__);
	return 0;
}

static int mdla_release(struct inode *inodep, struct file *filep)
{
	mdla_drv_debug("%s(): Device successfully closed\n", __func__);
	return 0;
}

const static struct file_operations fops = {
	.open		= mdla_open,
	.unlocked_ioctl	= mdla_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= mdla_compat_ioctl,
#endif
	.mmap		= mdla_map_uva,
	.release	= mdla_release,
};

const struct file_operations *mdla_fops_get(void)
{
	return &fops;
}

void mdla_ioctl_register_perf_handle(int (*pmu_ioctl)(struct file *filp,
			u32 command,
			unsigned long arg,
			bool need_pwr_on))
{
	// for each core
	if (pmu_ioctl)
		mdla_ioctl_perf	= pmu_ioctl;
}

void mdla_ioctl_unregister_perf_handle(void)
{
	mdla_ioctl_perf	= NULL;
}

void mdla_ioctl_set_efuse(u32 efuse)
{
	mdla_efuse = efuse;
};

