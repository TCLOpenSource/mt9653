// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kobject.h>

#include "clkdbg.h"
#include "clk-dtv.h"

#define CLKDBG_PM_DOMAIN	0

static const struct clkdbg_ops *clkdbg_ops;

static struct provider_clk *provider_clks;
#define SUPPORT_CLOCK_NUMBER	4096

static const char *get_provider_name(struct device_node *node, u32 *cells);
struct provider_clk *get_all_provider_clks(void)
{
	struct device_node *node = NULL;
	int n = 0;

	if (!provider_clks) {
		provider_clks = kzalloc(sizeof(struct provider_clk)*SUPPORT_CLOCK_NUMBER,
							GFP_KERNEL);
		if (IS_ERR_OR_NULL(provider_clks))
			return NULL;
	}

	if (provider_clks[0].ck)
		return provider_clks;

	do {
		const char *node_name;
		u32 cells;
		int i;

		node = of_find_node_with_property(node, "#clock-cells");

		if (!node)
			break;

		node_name = get_provider_name(node, &cells);


		if (cells == 0) {
			struct of_phandle_args pa;

			pa.np = node;
			pa.args_count = 0;
			provider_clks[n].ck = of_clk_get_from_provider(&pa);
			++n;
			continue;
		}

		for (i = 0; i < SUPPORT_CLOCK_NUMBER; i++) {
			struct of_phandle_args pa;
			struct clk *ck = NULL;

			pa.np = node;
			pa.args[0] = i;
			pa.args_count = 1;
			ck = of_clk_get_from_provider(&pa);

			if (PTR_ERR(ck) == -EINVAL)
				break;
			else if (IS_ERR_OR_NULL(ck))
				continue;

			provider_clks[n].ck = ck;
			provider_clks[n].idx = i;
			provider_clks[n].provider_name = node_name;
			++n;
		}
	} while (node);

	return provider_clks;
}

void set_clkdbg_ops(const struct clkdbg_ops *ops)
{
	clkdbg_ops = ops;
}

static const struct fmeter_clk *get_all_fmeter_clks(void)
{
	if (!clkdbg_ops || !clkdbg_ops->get_all_fmeter_clks)
		return NULL;

	return clkdbg_ops->get_all_fmeter_clks();
}

static void *prepare_fmeter(void)
{
	if (!clkdbg_ops || !clkdbg_ops->prepare_fmeter)
		return NULL;

	return clkdbg_ops->prepare_fmeter();
}

static void unprepare_fmeter(void *data)
{
	if (!clkdbg_ops || !clkdbg_ops->unprepare_fmeter)
		return;

	clkdbg_ops->unprepare_fmeter(data);
}

static u32 fmeter_freq(const struct fmeter_clk *fclk)
{
	if (!clkdbg_ops || !clkdbg_ops->fmeter_freq)
		return 0;

	return clkdbg_ops->fmeter_freq(fclk);
}

static const struct regname *get_all_regnames(void)
{
	if (!clkdbg_ops || !clkdbg_ops->get_all_regnames)
		return NULL;

	return clkdbg_ops->get_all_regnames();
}

static const char * const *get_all_clk_names(void)
{
	if (!clkdbg_ops || !clkdbg_ops->get_all_clk_names)
		return NULL;

	return clkdbg_ops->get_all_clk_names();
}

static const char * const *get_pwr_names(void)
{
	static const char * const default_pwr_names[] = {
		[0]  = "(MD)",
		[1]  = "(CONN)",
		[2]  = "(DDRPHY)",
		[3]  = "(DISP)",
		[4]  = "(MFG)",
		[5]  = "(ISP)",
		[6]  = "(INFRA)",
		[7]  = "(VDEC)",
		[8]  = "(CPU, CA7_CPUTOP)",
		[9]  = "(FC3, CA7_CPU0, CPUTOP)",
		[10] = "(FC2, CA7_CPU1, CPU3)",
		[11] = "(FC1, CA7_CPU2, CPU2)",
		[12] = "(FC0, CA7_CPU3, CPU1)",
		[13] = "(MCUSYS, CA7_DBG, CPU0)",
		[14] = "(MCUSYS, VEN, BDP)",
		[15] = "(CA15_CPUTOP, ETH, MCUSYS)",
		[16] = "(CA15_CPU0, HIF)",
		[17] = "(CA15_CPU1, CA15-CX0, INFRA_MISC)",
		[18] = "(CA15_CPU2, CA15-CX1)",
		[19] = "(CA15_CPU3, CA15-CPU0)",
		[20] = "(VEN2, MJC, CA15-CPU1)",
		[21] = "(VEN, CA15-CPUTOP)",
		[22] = "(MFG_2D)",
		[23] = "(MFG_ASYNC, DBG)",
		[24] = "(AUDIO, MFG_2D)",
		[25] = "(USB, VCORE_PDN, MFG_ASYNC)",
		[26] = "(ARMPLL_DIV, CPUTOP_SRM_SLPB)",
		[27] = "(MD2, CPUTOP_SRM_PDN)",
		[28] = "(CPU3_SRM_PDN)",
		[29] = "(CPU2_SRM_PDN)",
		[30] = "(CPU1_SRM_PDN)",
		[31] = "(CPU0_SRM_PDN)",
	};

	if (!clkdbg_ops || !clkdbg_ops->get_pwr_names)
		return default_pwr_names;

	return clkdbg_ops->get_pwr_names();
}

#ifdef MODULE
unsigned int __get_enable_count(struct clk *clk)
{
	return -1;
}
#else
unsigned int __get_enable_count(struct clk *clk)
{
	return __clk_get_enable_count(clk);
}
#endif

struct clk *clk_get_by_name(const char *name)
{

	struct clk *c;
	struct clk_hw *c_hw;
	int size = strlen(name);
	struct provider_clk *pvdck = get_all_provider_clks();

	if (!pvdck)
		return NULL;

	for (; pvdck->ck; pvdck++) {
		c = pvdck->ck;

		if (!c)
			continue;

		c_hw = __clk_get_hw(c);
		if (!c_hw)
			continue;

		if (strlen(clk_hw_get_name(c_hw)) == size
			&& !strncmp(clk_hw_get_name(c_hw), name, size))
			return c;
	}

	return NULL;
}

static bool is_valid_reg(void __iomem *addr)
{
#ifdef CONFIG_64BIT
	return ((u64)addr & 0xf0000000) || (((u64)addr >> 32) & 0xf0000000);
#else
	return ((u32)addr & 0xf0000000);
#endif
}

enum clkdbg_opt {
	CLKDBG_EN_SUSPEND_SAVE_1,
	CLKDBG_EN_SUSPEND_SAVE_2,
	CLKDBG_EN_SUSPEND_SAVE_3,
};

static u32 clkdbg_flags;

static void set_clkdbg_flag(enum clkdbg_opt opt)
{
	clkdbg_flags |= BIT(opt);
}

static void clr_clkdbg_flag(enum clkdbg_opt opt)
{
	clkdbg_flags &= ~BIT(opt);
}

static bool has_clkdbg_flag(enum clkdbg_opt opt)
{
	return !!(clkdbg_flags & BIT(opt));
}

typedef void (*fn_fclk_freq_proc)(const struct fmeter_clk *fclk,
					u32 freq, void *data);

static void proc_all_fclk_freq(fn_fclk_freq_proc proc, void *data)
{
	void *fmeter_data;
	const struct fmeter_clk *fclk;

	fclk = get_all_fmeter_clks();

	if (!fclk || !proc)
		return;

	fmeter_data = prepare_fmeter();

	for (; fclk->type; fclk++) {
		u32 freq;

		freq = fmeter_freq(fclk);
		proc(fclk, freq, data);
	}

	unprepare_fmeter(fmeter_data);
}

static void print_fclk_freq(const struct fmeter_clk *fclk, u32 freq, void *data)
{
	clk_warn("%2d: %-29s: %u\n", fclk->id, fclk->name, freq);
}

void print_fmeter_all(void)
{
	proc_all_fclk_freq(print_fclk_freq, NULL);
}

static void seq_print_fclk_freq(const struct fmeter_clk *fclk,
				u32 freq, void *data)
{
	struct seq_file *s = data;

	seq_printf(s, "%2d: %-29s: %u\n", fclk->id, fclk->name, freq);
}

static int seq_print_fmeter_all(struct seq_file *s, void *v)
{
	proc_all_fclk_freq(seq_print_fclk_freq, s);

	return 0;
}

typedef void (*fn_regname_proc)(const struct regname *rn, void *data);

static void proc_all_regname(fn_regname_proc proc, void *data)
{
	const struct regname *rn = get_all_regnames();

	if (!rn)
		return;

	for (; rn->base; rn++)
		proc(rn, data);
}

static void print_reg(const struct regname *rn, void *data)
{
	if (!is_valid_reg(ADDR(rn)))
		return;

	clk_warn("%-21s: [0x%08x][0x%p] = 0x%08x\n",
		rn->name, PHYSADDR(rn), ADDR(rn), clk_readl(ADDR(rn)));
}

void print_regs(void)
{
	proc_all_regname(print_reg, NULL);
}

static void seq_print_reg(const struct regname *rn, void *data)
{
	struct seq_file *s = data;

	if (!is_valid_reg(ADDR(rn)))
		return;

	seq_printf(s, "%-21s: [0x%08x][0x%p] = 0x%08x\n",
		rn->name, PHYSADDR(rn), ADDR(rn), clk_readl(ADDR(rn)));
}

static int seq_print_regs(struct seq_file *s, void *v)
{
	proc_all_regname(seq_print_reg, s);

	return 0;
}

static void print_reg2(const struct regname *rn, void *data)
{
	if (!is_valid_reg(ADDR(rn)))
		return;

	clk_warn("%-21s: [0x%08x][0x%p] = 0x%08x\n",
		rn->name, PHYSADDR(rn), ADDR(rn), clk_readl(ADDR(rn)));

	msleep(20);
}

static int clkdbg_dump_regs2(struct seq_file *s, void *v)
{
	proc_all_regname(print_reg2, s);

	return 0;
}

static void dump_clk_state(const char *clkname, struct seq_file *s)
{
	struct clk *c = clk_get_by_name(clkname);
	struct clk *p = IS_ERR_OR_NULL(c) ? NULL : clk_get_parent(c);
	struct clk_hw *c_hw = __clk_get_hw(c);
	struct clk_hw *p_hw = __clk_get_hw(p);

	if (IS_ERR_OR_NULL(c)) {
		seq_printf(s, "[%17s: NULL]\n", clkname);
		return;
	}

	seq_printf(s, "[%-17s: %3s, %3d, %3d, %10ld, %17s]\n",
		clk_hw_get_name(c_hw),
//		(clk_hw_is_enabled(c_hw) || clk_hw_is_prepared(c_hw)) ?
		(clk_hw_is_enabled(c_hw)) ?
			"ON" : "off",
		clk_hw_is_prepared(c_hw),
		__get_enable_count(c),
		clk_hw_get_rate(c_hw),
		p ? clk_hw_get_name(p_hw) : "- ");
}

static int clkdbg_dump_state_all(struct seq_file *s, void *v)
{
	const char * const *ckn = get_all_clk_names();

	if (!ckn)
		return 0;

	for (; *ckn; ckn++)
		dump_clk_state(*ckn, s);

	return 0;
}

static const char *get_provider_name(struct device_node *node, u32 *cells)
{
	const char *name;
	const char *p;
	u32 cc;

	if (of_property_read_u32(node, "#clock-cells", &cc))
		cc = 0;

	if (cells)
		*cells = cc;

	if (cc == 0) {
		if (of_property_read_string(node, "clock-output-names", &name))
			name = node->name;

		return name;
	}

	/*
	 *  for dtv clk, data separation requirement,
	 *  all have same compatible name, unable to
	 *  differentiate prv with it
	 */
	if (IS_ENABLED(CONFIG_COMMON_CLK_MEDIATEK_TV))
		return node->name;

	if (of_property_read_string(node, "compatible", &name))
		name = node->name;

	p = strchr(name, '-');

	if (p)
		return p + 1;
	else
		return name;

}

static void dump_provider_clk(struct provider_clk *pvdck, struct seq_file *s)
{
	struct clk *c = pvdck->ck;
	struct clk *p = IS_ERR_OR_NULL(c) ? NULL : clk_get_parent(c);
	struct clk_hw *c_hw = __clk_get_hw(c);
	struct clk_hw *p_hw = __clk_get_hw(p);

	seq_printf(s, "[%10s: %-17s: %3s, %3d, %3d, %10ld, %17s]\n",
		pvdck->provider_name ? pvdck->provider_name : "/ ",
		clk_hw_get_name(c_hw),
		(clk_hw_is_enabled(c_hw) || clk_hw_is_prepared(c_hw)) ?
			"ON" : "off",
		clk_hw_is_prepared(c_hw),
		__get_enable_count(c),
		clk_hw_get_rate(c_hw),
		p ? clk_hw_get_name(p_hw) : "- ");

}

static int clkdbg_dump_provider_clks(struct seq_file *s, void *v)
{
	struct provider_clk *pvdck = get_all_provider_clks();

	if (!pvdck)
		return 0;

	for (; pvdck->ck; pvdck++)
		dump_provider_clk(pvdck, s);

	return 0;
}

static void dump_provider_mux(struct provider_clk *pvdck, struct seq_file *s)
{
	int i;
	struct clk *c = pvdck->ck;
	struct clk_hw *c_hw = __clk_get_hw(c);
	unsigned int np = clk_hw_get_num_parents(c_hw);

	if (np <= 1)
		return;

	dump_provider_clk(pvdck, s);

	for (i = 0; i < np; i++) {
		struct clk_hw *p_hw = clk_hw_get_parent_by_index(c_hw, i);

		if (IS_ERR_OR_NULL(p_hw))
			continue;

		seq_printf(s, "\t\t\t(%2d: %-17s: %3s, %10ld)\n",
			i,
			clk_hw_get_name(p_hw),
			(clk_hw_is_enabled(p_hw) || clk_hw_is_prepared(p_hw)) ?
				"ON" : "off",
			clk_hw_get_rate(p_hw));
	}
}

static int clkdbg_dump_muxes(struct seq_file *s, void *v)
{
	struct provider_clk *pvdck = get_all_provider_clks();

	if (!pvdck)
		return 0;

	for (; pvdck->ck; pvdck++)
		dump_provider_mux(pvdck, s);

	return 0;
}

char debug_provider[256];
static void tree_health_check(struct seq_file *s, struct clk *ck)
{
	int i;
	struct clk_hw *c_hw = __clk_get_hw(ck);
	unsigned int np = clk_hw_get_num_parents(c_hw);
	bool checked = false;

	for (i = 0; i < np; i++) {
		struct clk_hw *p_hw = clk_hw_get_parent_by_index(c_hw, i);

		if (IS_ERR_OR_NULL(p_hw)) {
			if (checked == false)
				seq_printf(s, "\n[%s] :", clk_hw_get_name(c_hw));
			checked = true;
			seq_printf(s, " %d", i);
		}
	}

}

static int clkdbg_mux_tree_health_check_p(struct seq_file *s, void *v)
{
	struct provider_clk *pvdck = get_all_provider_clks();
	const char *prov = debug_provider;

	seq_puts(s, "[clock_name] : parent not found (index)");

	if (!pvdck)
		return 0;

	for (; pvdck->ck; pvdck++) {
		if (!pvdck->provider_name && *prov != 0)
			continue;

		if (pvdck->provider_name &&
			strlen(prov) != strlen(pvdck->provider_name))
			continue;

		if (pvdck->provider_name &&
			strncmp(prov, pvdck->provider_name, strlen(pvdck->provider_name)))
			continue;

		tree_health_check(s, pvdck->ck);
	}

	seq_puts(s, "\n");
	return 0;
}

struct sw_en_state {
	struct provider_clk *pvdck;
	bool enabled;
};

static struct sw_en_state *swen_save_point;
static int log_provider_sw_en(struct provider_clk *pvdck, int idx)
{
	struct clk *c = pvdck->ck;
	struct clk_hw *c_hw = __clk_get_hw(c);
	const char *sw_en_str = "_swen";

	if (!pvdck->provider_name)
		return idx;

	if (!strstr(pvdck->provider_name, sw_en_str))
		return idx;

	pr_info("[%d]%s %s:%d\n", idx, pvdck->provider_name,
			clk_hw_get_name(c_hw), clk_hw_is_enabled(c_hw));
	swen_save_point[idx].pvdck = pvdck;
	swen_save_point[idx].enabled = clk_hw_is_enabled(c_hw);

	return ++idx;

}

static void clkdbg_log_sw_ens(void)
{
	struct provider_clk *pvdck = get_all_provider_clks();
	int i = 0;

	if (!pvdck)
		return;

	if (!swen_save_point) {
		swen_save_point = kmalloc(sizeof(*swen_save_point)*1024, GFP_KERNEL);

		if (IS_ERR_OR_NULL(swen_save_point)) {
			pr_err("out of mem\n");
			return;
		}
	}

	memset(swen_save_point, 0, sizeof(*swen_save_point)*1024);//do reset

	for (; pvdck->ck; pvdck++)
		i = log_provider_sw_en(pvdck, i);

}

static void clkdbg_dump_swen_logs(struct seq_file *s)
{
	struct sw_en_state *root = swen_save_point;

	if (!root || !root->pvdck) {
		seq_puts(s, " : No log to dump");
		return;
	}

	for (; root->pvdck; root++) {
		struct provider_clk *pvdck = root->pvdck;
		struct clk_hw *c_hw = __clk_get_hw(root->pvdck->ck);

		seq_printf(s, "\n[%s]", pvdck->provider_name);
		seq_printf(s, " %s : %d", clk_hw_get_name(c_hw),
			root->enabled);
	}

}

static void clkdbg_swen_cmp_w_log(struct seq_file *s)
{
	struct sw_en_state *root = swen_save_point;

	if (!root || !root->pvdck) {
		seq_puts(s, " : No log to compare\n");
		return;
	}

	for (; root->pvdck; root++) {
		struct clk *c = root->pvdck->ck;
		struct clk_hw *c_hw = __clk_get_hw(c);

		if (clk_hw_is_enabled(c_hw) != root->enabled)
			seq_printf(s, "[%s] %d -> %d\n", clk_hw_get_name(c_hw),
				root->enabled, clk_hw_is_enabled(c_hw));
	}

}

static int sw_en_dumplog_show(struct seq_file *s, void *v)
{
	seq_puts(s, "SW_EN log info:");

	clkdbg_dump_swen_logs(s);

	seq_puts(s, "\n");

	return 0;
}

static int sw_en_cmp_w_log_show(struct seq_file *s, void *v)
{
	seq_puts(s, "diff:   log -> cur\n");

	clkdbg_swen_cmp_w_log(s);

	seq_puts(s, "=====================\n");

	return 0;
}

static int dump_pwr_status(u32 spm_pwr_status, struct seq_file *s)
{
	int i;
	const char * const *pwr_name = get_pwr_names();

	seq_printf(s, "SPM_PWR_STATUS: 0x%08x\n\n", spm_pwr_status);

	for (i = 0; i < 32; i++) {
		const char *st = (spm_pwr_status & BIT(i)) ? "ON" : "off";

		seq_printf(s, "[%2d]: %3s: %s\n", i, st, pwr_name[i]);
	}

	return 0;
}

static u32 read_spm_pwr_status(void)
{
	static void __iomem *scpsys_base;

	if (!scpsys_base)
		scpsys_base = ioremap(0x10006000, PAGE_SIZE);

	return clk_readl(scpsys_base + 0x60c);
}

static int clkdbg_pwr_status(struct seq_file *s, void *v)
{
	return dump_pwr_status(read_spm_pwr_status(), s);
}

#define COMMAND_LEN	128
static char last_cmd[COMMAND_LEN] = {0};

const char *get_last_cmd(void)
{
	return last_cmd;
}

static int clkop_int_ckname(int (*clkop)(struct clk *clk),
			const char *clkop_name, const char *clk_name,
			struct clk *ck, struct seq_file *s)
{
	struct clk *clk;

	if (!IS_ERR_OR_NULL(ck)) {
		clk = ck;
	} else {
		clk = clk_get_by_name(clk_name);
		if (IS_ERR_OR_NULL(clk)) {
			seq_printf(s, "clk_lookup(%s): 0x%p\n", clk_name, clk);
			return PTR_ERR(clk);
		}
	}

	return clkop(clk);
}

static int clkdbg_clkop_int_ckname(int (*clkop)(struct clk *clk),
			const char *clkop_name, struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	if (!clk_name)
		return 0;

	if (strcmp(clk_name, "all") == 0) {
		int r = 0;
		struct provider_clk *pvdck = get_all_provider_clks();

		if (!pvdck)
			return 0;

		for (; pvdck->ck; pvdck++) {
			r |= clkop_int_ckname(clkop, clkop_name, NULL,
						pvdck->ck, s);
		}

		seq_printf(s, "%s(%s): %d\n", clkop_name, clk_name, r);

		return r;
	}

	r = clkop_int_ckname(clkop, clkop_name, clk_name, NULL, s);
	seq_printf(s, "%s(%s): %d\n", clkop_name, clk_name, r);

	return r;
}

static void clkop_void_ckname(void (*clkop)(struct clk *clk),
			const char *clkop_name, const char *clk_name,
			struct clk *ck, struct seq_file *s)
{
	struct clk *clk;

	if (!IS_ERR_OR_NULL(ck)) {
		clk = ck;
	} else {
		clk = clk_get_by_name(clk_name);
		if (IS_ERR_OR_NULL(clk)) {
			seq_printf(s, "clk_lookup(%s): 0x%p\n", clk_name, clk);
			return;
		}
	}

	clkop(clk);
}

static int clkdbg_clkop_void_ckname(void (*clkop)(struct clk *clk),
			const char *clkop_name, struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *clk_name;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	if (!clk_name)
		return 0;

	if (strcmp(clk_name, "all") == 0) {
		struct provider_clk *pvdck = get_all_provider_clks();

		if (!pvdck)
			return 0;

		for (; pvdck->ck; pvdck++) {
			clkop_void_ckname(clkop, clkop_name, NULL,
						pvdck->ck, s);
		}

		seq_printf(s, "%s(%s)\n", clkop_name, clk_name);

		return 0;
	}

	clkop_void_ckname(clkop, clkop_name, clk_name, NULL, s);
	seq_printf(s, "%s(%s)\n", clkop_name, clk_name);

	return 0;
}

static int clkdbg_prepare(struct seq_file *s, void *v)
{
	return clkdbg_clkop_int_ckname(clk_prepare,
					"clk_prepare", s, v);
}

static int clkdbg_unprepare(struct seq_file *s, void *v)
{
	return clkdbg_clkop_void_ckname(clk_unprepare,
					"clk_unprepare", s, v);
}

static int clkdbg_enable(struct seq_file *s, void *v)
{
	return clkdbg_clkop_int_ckname(clk_enable,
					"clk_enable", s, v);
}

static int clkdbg_disable(struct seq_file *s, void *v)
{
	return clkdbg_clkop_void_ckname(clk_disable,
					"clk_disable", s, v);
}

static int clkdbg_prepare_enable(struct seq_file *s, void *v)
{
	return clkdbg_clkop_int_ckname(clk_prepare_enable,
					"clk_prepare_enable", s, v);
}

static int clkdbg_disable_unprepare(struct seq_file *s, void *v)
{
	return clkdbg_clkop_void_ckname(clk_disable_unprepare,
					"clk_disable_unprepare", s, v);
}

static int clkdbg_set_parent(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *parent_name;
	struct clk *clk;
	struct clk *parent;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	parent_name = strsep(&c, " ");

	if (!clk_name || !parent_name)
		return 0;

	seq_printf(s, "clk_set_parent(%s, %s): ", clk_name, parent_name);

	clk = clk_get_by_name(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get_by_name(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	parent = clk_get_by_name(parent_name);
	if (IS_ERR_OR_NULL(parent)) {
		seq_printf(s, "clk_get_by_name(): 0x%p\n", parent);
		return PTR_ERR(parent);
	}

	r = clk_prepare_enable(clk);
	if (r) {
		seq_printf(s, "clk_prepare_enable(): %d\n", r);
		return r;
	}

	r = clk_set_parent(clk, parent);
	seq_printf(s, "%d\n", r);

	clk_disable_unprepare(clk);

	return r;
}

static int clkdbg_set_rate(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *rate_str;
	struct clk *clk;
	unsigned long rate = 0;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	rate_str = strsep(&c, " ");

	if (!clk_name || !rate_str)
		return 0;

	r = kstrtoul(rate_str, 0, &rate);

	seq_printf(s, "clk_set_rate(%s, %lu): %d: ", clk_name, rate, r);

	clk = clk_get_by_name(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get_by_name(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_set_rate(clk, rate);
	seq_printf(s, "%d\n", r);

	return r;
}

void *reg_from_str(const char *str)
{
	static u32 phys;
	static void __iomem *virt;

	if (sizeof(void *) == sizeof(unsigned long)) {
		unsigned long v;

		if (kstrtoul(str, 0, &v) == 0) {
			if ((0xf0000000 & v) < 0x20000000) {
				if (virt && v > phys && v < phys + PAGE_SIZE)
					return virt + v - phys;

				if (virt)
					iounmap(virt);

				phys = v & ~(PAGE_SIZE - 1);
				virt = ioremap(phys, PAGE_SIZE);

				return virt + v - phys;
			}

			return (void *)((uintptr_t)v);
		}
	} else if (sizeof(void *) == sizeof(unsigned long long)) {
		unsigned long long v;

		if (kstrtoull(str, 0, &v) == 0) {
			if ((0xfffffffff0000000ULL & v) < 0x20000000) {
				if (virt && v > phys && v < phys + PAGE_SIZE)
					return virt + v - phys;

				if (virt)
					iounmap(virt);

				phys = v & ~(PAGE_SIZE - 1);
				virt = ioremap(phys, PAGE_SIZE);

				return virt + v - phys;
			}

			return (void *)((uintptr_t)v);
		}
	} else {
		clk_warn("unexpected pointer size: sizeof(void *): %zu\n",
			sizeof(void *));
	}

	clk_warn("%s(): parsing error: %s\n", __func__, str);

	return NULL;
}

static int parse_reg_val_from_cmd(void __iomem **preg, unsigned long *pval)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *reg_str;
	char *val_str;
	int r = 0;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	reg_str = strsep(&c, " ");
	val_str = strsep(&c, " ");

	if (preg && reg_str) {
		*preg = reg_from_str(reg_str);
		if (*preg)
			r++;
	}

	if (pval && val_str && !kstrtoul(val_str, 0, pval))
		r++;

	return r;
}

static int set_calc_enable(struct seq_file *s, int excel_group)
{
	u16 reg_tmp = 0, access_reg, reg_clk_calc_en = 0, freq_reg;
	u16 mask = 0;
	int count = 0, ret;
	u32 target_reg[2] = {0};
	struct device_node *test_ctrl_data;
	void __iomem *ckgen_base = NULL, *test_ctrl_base = NULL;

	/* Acquire test_ctrl device node */
	test_ctrl_data = of_find_node_by_name(NULL, "test_ctrl_data");
	if (!test_ctrl_data)
		return -ENODEV;

	/* Acquire ctrl base addr */
	ckgen_base = of_iomap(test_ctrl_data, 0);
	test_ctrl_base = of_iomap(test_ctrl_data, 1);
	if (!ckgen_base || !test_ctrl_base) {
		ret = -ENODEV;
		goto out;
	}

	ret = of_property_read_variable_u32_array(test_ctrl_data, "reg_clk_calc_en_set",
				target_reg, 2, 2);
	if (ret < 0)
		goto out;
	reg_clk_calc_en = (u16)target_reg[0];
	reg_tmp = readw(test_ctrl_base + reg_clk_calc_en);
	reg_tmp &= 0xfffe;
	writew(reg_tmp, test_ctrl_base + reg_clk_calc_en);
	seq_printf(s, "reset:0x%x 0x%x\n", reg_clk_calc_en, reg_tmp);

	/* for enable */
	ret = of_property_read_variable_u32_array(test_ctrl_data, "enable_reg",
				target_reg, 2, 2);
	if (ret < 0)
		goto out;
	access_reg = (u16)target_reg[0];
	reg_tmp = readw(ckgen_base + access_reg);
	reg_tmp |= (1<<target_reg[1]);
	writew(reg_tmp, ckgen_base + access_reg);
	seq_printf(s, "enable_reg: 0x%x, 0x%x\n", access_reg, reg_tmp);

	ret = of_property_read_variable_u32_array(test_ctrl_data, "reg_src_sel",
				target_reg, 2, 2);
	if (ret < 0)
		goto out;
	access_reg = (u16)target_reg[0];
	reg_tmp = excel_group;
	writew(reg_tmp, test_ctrl_base + access_reg); // reg_clk_calc_src_sel

	reg_tmp = readw(test_ctrl_base + reg_clk_calc_en);
	seq_printf(s, "enable b: 0x%x\n", reg_tmp);
	reg_tmp |= 0x1;
	writew(reg_tmp, test_ctrl_base+reg_clk_calc_en);
	seq_printf(s, "enable a : 0x%x\n", readw(test_ctrl_base+reg_clk_calc_en));

	ret = of_property_read_variable_u32_array(test_ctrl_data, "calc_done",
				target_reg, 2, 2);
	if (ret < 0)
		goto out;
	access_reg = (u16)target_reg[0];
	mask = 1<<target_reg[1];
	while ((readw(test_ctrl_base+access_reg)&mask) == 0) {
		count++;
		if (count > 1000000)
			break;
	}
	/* Check calc done */
	seq_printf(s, "done : 0x%x\n", readw(test_ctrl_base+access_reg));

	ret = of_property_read_variable_u32_array(test_ctrl_data, "report_reg",
				target_reg, 2, 2);
	if (ret < 0)
		goto out;
	access_reg = (u16)target_reg[0];
	freq_reg = readw(test_ctrl_base+access_reg);
	ret = freq_reg;
	seq_printf(s, "report : 0x%x\n", readw(test_ctrl_base+access_reg));

out:
	if (ckgen_base)
		iounmap(ckgen_base);

	if (test_ctrl_base)
		iounmap(test_ctrl_base);

	of_node_put(test_ctrl_data);
	return ret;

}

static int test_one_clk(struct seq_file *s, void *v, int excel_group,
		int clk_num, const u16 div, const int src_bit_shift)
{
	u16 reg_tmp = 0, access_reg;
	int ret;
	u32 subsys_reg;
	struct device_node *test_ctrl_data;
	int subsys_offset = 0, subsys_size = 0;
	void __iomem *ckgen_base = NULL;

	/* Acquire test_ctrl device node */
	test_ctrl_data = of_find_node_by_name(NULL, "test_ctrl_data");
	if (!test_ctrl_data)
		return -ENODEV;

	/* Acquire ctrl base addr */
	ckgen_base = of_iomap(test_ctrl_data, 0);

	ret = of_property_read_u32(test_ctrl_data, "test_ctrl_offset", &subsys_offset);
	if (ret < 0) {
		ret = -ENODEV;
		goto out;
	}

	subsys_size = of_property_count_elems_of_size(test_ctrl_data, "subsys_reg", sizeof(u32));
	if (subsys_size < 0) {
		ret = -ENODEV;
		goto out;
	}
	seq_printf(s, "subsys: base %d size %d\n", subsys_offset, subsys_size);

	if (excel_group < subsys_offset || excel_group >= subsys_size + subsys_offset) {
		ret = -EINVAL;
		goto out;
	}

	of_property_read_u32_index(test_ctrl_data, "subsys_reg",
				excel_group-subsys_offset, &subsys_reg);

	if (subsys_reg == 0) {
		ret = -ENODEV;
		goto out;
	}

	reg_tmp = 0;
	reg_tmp |= 0x1;//enable bit
	reg_tmp |= (div<<4);//div_sel
	reg_tmp |= (clk_num<<(8-src_bit_shift));//test_src_rtl
	access_reg = (u16)subsys_reg;
	writew(0, ckgen_base+access_reg);
	writew(reg_tmp, ckgen_base+access_reg);
	seq_printf(s, "\nsubsys_ctrl_reg: 0x%x\n", subsys_reg);
	seq_printf(s, "ctrl_value: 0x%x\n", readw(ckgen_base+access_reg));

	ret = set_calc_enable(s, excel_group);

out:
	if (ckgen_base)
		iounmap(ckgen_base);

	of_node_put(test_ctrl_data);
	return ret;
}

#define TEST_CTRL_REF	12
#define TEST_CTRL_DIV	1000
static int clkdbg_test_one_clk(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *excel_group_str;
	char *clk_num_str;
	char *div_str;
	char *bit_num_ext;
	unsigned int excel_group, clk_num, div, bit_shift;
	int freq_report = 0;
	int ret = 0;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	excel_group_str = strsep(&c, " ");
	clk_num_str = strsep(&c, " ");
	div_str = strsep(&c, " ");
	bit_num_ext = strsep(&c, " ");

	if (!excel_group_str || !clk_num_str || !div_str)
		return 0;

	ret = kstrtouint(excel_group_str, 0, &excel_group);
	if (ret)
		return ret;

	ret = kstrtouint(clk_num_str, 0, &clk_num);
	if (ret)
		return ret;

	ret = kstrtouint(div_str, 0, &div);
	if (ret)
		return ret;
	if (div == 0)
		return 0;

	if (!bit_num_ext)
		bit_shift = 0;
	else {
		ret = kstrtouint(bit_num_ext, 0, &bit_shift);
		if (ret)
			return ret;
		bit_shift = bit_shift > 0 ? 1 : 0;
	}

	freq_report = test_one_clk(s, v, excel_group, clk_num, div, bit_shift);

	seq_printf(s, "excel_group:%d\n", excel_group);
	seq_printf(s, "clk_num:%d\n", clk_num);
	seq_printf(s, "div:%d\n", 1<<(div-1));

	if (freq_report < 0)
		seq_printf(s, "Error:%d\n", freq_report);
	else
		seq_printf(s, "freq:%d\n", freq_report*TEST_CTRL_REF*(1<<(div-1))/TEST_CTRL_DIV);

	return 0;

}

static int clkdbg_reg_read(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	if (parse_reg_val_from_cmd(&reg, NULL) != 1)
		return 0;

	seq_printf(s, "readl(0x%p): ", reg);

	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_write(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	if (parse_reg_val_from_cmd(&reg, &val) != 2)
		return 0;

	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_writel(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_set(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	if (parse_reg_val_from_cmd(&reg, &val) != 2)
		return 0;

	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_setl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_clr(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	if (parse_reg_val_from_cmd(&reg, &val) != 2)
		return 0;

	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_clrl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int parse_val_from_cmd(unsigned long *pval)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *val_str;
	int r = 0;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	val_str = strsep(&c, " ");

	if (pval && val_str && !kstrtoul(val_str, 0, pval))
		r++;

	return r;
}

static int clkdbg_show_flags(struct seq_file *s, void *v)
{
	static const char * const clkdbg_opt_name[] = {
		"CLKDBG_EN_SUSPEND_SAVE_1",
		"CLKDBG_EN_SUSPEND_SAVE_2",
		"CLKDBG_EN_SUSPEND_SAVE_3",
	};

	int i;

	seq_printf(s, "clkdbg_flags: 0x%08x\n", clkdbg_flags);

	for (i = 0; i < ARRAY_SIZE(clkdbg_opt_name); i++) {
		const char *onff = has_clkdbg_flag(i) ? "ON" : "off";

		seq_printf(s, "[%2d]: %3s: %s\n", i, onff, clkdbg_opt_name[i]);
	}

	return 0;
}

static int clkdbg_set_flag(struct seq_file *s, void *v)
{
	unsigned long val;

	if (parse_val_from_cmd(&val) != 1)
		return 0;

	set_clkdbg_flag(val);

	seq_printf(s, "clkdbg_flags: 0x%08x\n", clkdbg_flags);

	return 0;
}

static int clkdbg_clr_flag(struct seq_file *s, void *v)
{
	unsigned long val;

	if (parse_val_from_cmd(&val) != 1)
		return 0;

	clr_clkdbg_flag(val);

	seq_printf(s, "clkdbg_flags: 0x%08x\n", clkdbg_flags);

	return 0;
}

#if CLKDBG_PM_DOMAIN

/*
 * pm_domain support
 */

static struct generic_pm_domain **get_all_genpd(void)
{
	static struct generic_pm_domain *pds[20];
	static int num_pds;
	const int maxpd = ARRAY_SIZE(pds);
	struct device_node *node;

	if (num_pds)
		goto out;

	node = of_find_node_with_property(NULL, "#power-domain-cells");

	if (!node)
		return NULL;

	for (num_pds = 0; num_pds < maxpd; num_pds++) {
		struct of_phandle_args pa;

		pa.np = node;
		pa.args[0] = num_pds;
		pa.args_count = 1;
		pds[num_pds] = of_genpd_get_from_provider(&pa);

		if (IS_ERR(pds[num_pds])) {
			pds[num_pds] = NULL;
			break;
		}
	}

out:
	return pds;
}

static struct platform_device *pdev_from_name(const char *name)
{
	struct generic_pm_domain **pds = get_all_genpd();

	for (; *pds; pds++) {
		struct pm_domain_data *pdd;
		struct generic_pm_domain *pd = *pds;

		if (IS_ERR_OR_NULL(pd))
			continue;

		list_for_each_entry(pdd, &pd->dev_list, list_node) {
			struct device *dev = pdd->dev;
			struct platform_device *pdev = to_platform_device(dev);

			if (strcmp(name, pdev->name) == 0)
				return pdev;
		}
	}

	return NULL;
}

static struct generic_pm_domain *genpd_from_name(const char *name)
{
	struct generic_pm_domain **pds = get_all_genpd();

	for (; *pds; pds++) {
		struct generic_pm_domain *pd = *pds;

		if (IS_ERR_OR_NULL(pd))
			continue;

		if (strcmp(name, pd->name) == 0)
			return pd;
	}

	return NULL;
}

struct genpd_dev_state {
	struct device *dev;
	bool active;
	int usage_count;
	unsigned int disable_depth;
	enum rpm_status runtime_status;
};

struct genpd_state {
	struct generic_pm_domain *pd;
	enum gpd_status status;
	struct genpd_dev_state *dev_state;
	int num_dev_state;
};

static void save_all_genpd_state(struct genpd_state *genpd_states,
				struct genpd_dev_state *genpd_dev_states)
{
	struct genpd_state *pdst = genpd_states;
	struct genpd_dev_state *devst = genpd_dev_states;
	struct generic_pm_domain **pds = get_all_genpd();

	for (; *pds; pds++) {
		struct pm_domain_data *pdd;
		struct generic_pm_domain *pd = *pds;

		if (IS_ERR_OR_NULL(pd))
			continue;

		pdst->pd = pd;
		pdst->status = pd->status;
		pdst->dev_state = devst;
		pdst->num_dev_state = 0;

		list_for_each_entry(pdd, &pd->dev_list, list_node) {
			struct device *d = pdd->dev;

			devst->dev = d;
			devst->active = pm_runtime_active(d);
			devst->usage_count = atomic_read(&d->power.usage_count);
			devst->disable_depth = d->power.disable_depth;
			devst->runtime_status = d->power.runtime_status;

			devst++;
			pdst->num_dev_state++;
		}

		pdst++;
	}

	pdst->pd = NULL;
	devst->dev = NULL;
}

static void dump_genpd_state(struct genpd_state *pdst, struct seq_file *s)
{
	static const char * const gpd_status_name[] = {
		"ACTIVE",
		"POWER_OFF",
	};

	static const char * const prm_status_name[] = {
		"active",
		"resuming",
		"suspended",
		"suspending",
	};

	seq_puts(s, "domain_on [pmd_name  status]\n");
	seq_puts(s, "\tdev_on (dev_name usage_count, disable, status)\n");
	seq_puts(s, "------------------------------------------------------\n");

	for (; pdst->pd; pdst++) {
		int i;
		struct generic_pm_domain *pd = pdst->pd;

		if (IS_ERR_OR_NULL(pd)) {
			seq_printf(s, "pd: 0x%p\n", pd);
			continue;
		}

		seq_printf(s, "%c [%-9s %11s]\n",
			(pdst->status == GPD_STATE_ACTIVE) ? '+' : '-',
			pd->name, gpd_status_name[pdst->status]);

		for (i = 0; i < pdst->num_dev_state; i++) {
			struct genpd_dev_state *devst = &pdst->dev_state[i];
			struct device *dev = devst->dev;
			struct platform_device *pdev = to_platform_device(dev);

			seq_printf(s, "\t%c (%-19s %3d, %d, %10s)\n",
				devst->active ? '+' : '-',
				pdev->name,
				devst->usage_count,
				devst->disable_depth,
				prm_status_name[devst->runtime_status]);
		}
	}
}

static void seq_print_all_genpd(struct seq_file *s)
{
	static struct genpd_dev_state devst[100];
	static struct genpd_state pdst[20];

	save_all_genpd_state(pdst, devst);
	dump_genpd_state(pdst, s);
}

static int clkdbg_dump_genpd(struct seq_file *s, void *v)
{
	seq_print_all_genpd(s);

	return 0;
}

static int clkdbg_pm_runtime_enable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	if (!dev_name)
		return 0;

	seq_printf(s, "pm_runtime_enable(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		pm_runtime_enable(&pdev->dev);
		seq_puts(s, "\n");
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_disable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	if (!dev_name)
		return 0;

	seq_printf(s, "pm_runtime_disable(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		pm_runtime_disable(&pdev->dev);
		seq_puts(s, "\n");
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_get_sync(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	if (!dev_name)
		return 0;

	seq_printf(s, "pm_runtime_get_sync(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		int r = pm_runtime_get_sync(&pdev->dev);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_put_sync(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	if (!dev_name)
		return 0;

	seq_printf(s, "pm_runtime_put_sync(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		int r = pm_runtime_put_sync(&pdev->dev);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int genpd_op(const char *gpd_op_name, struct seq_file *s)
{
	char cmd[sizeof(last_cmd)+1];
	char *c = cmd;
	char *ign;
	char *pd_name;
	struct generic_pm_domain *genpd;
	int gpd_op_id;
	int (*gpd_op)(struct generic_pm_domain *) = NULL;
	int r = 0;

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	ign = strsep(&c, " ");
	pd_name = strsep(&c, " ");

	if (!pd_name)
		return 0;

	if (strcmp(gpd_op_name, "power_on") == 0)
		gpd_op_id = 1;
	else
		gpd_op_id = 0;

	if (strcmp(pd_name, "all") == 0) {
		struct generic_pm_domain **pds = get_all_genpd();

		for (; *pds; pds++) {
			genpd = *pds;

			if (IS_ERR_OR_NULL(genpd))
				continue;

			gpd_op = (gpd_op_id == 1) ?
					genpd->power_on : genpd->power_off;
			r |= gpd_op(genpd);
		}

		seq_printf(s, "%s(%s): %d\n", gpd_op_name, pd_name, r);

		return 0;
	}

	genpd = genpd_from_name(pd_name);
	if (genpd) {
		gpd_op = (gpd_op_id == 1) ? genpd->power_on : genpd->power_off;
		r = gpd_op(genpd);

		seq_printf(s, "%s(%s): %d\n", gpd_op_name, pd_name, r);
	} else {
		seq_printf(s, "genpd_from_name(%s): NULL\n", pd_name);
	}

	return 0;
}

static int clkdbg_pwr_on(struct seq_file *s, void *v)
{
	return genpd_op("power_on", s);
}

static int clkdbg_pwr_off(struct seq_file *s, void *v)
{
	return genpd_op("power_off", s);
}

#endif /* CLKDBG_PM_DOMAIN */

/*
 * Suspend / resume handler
 */

#include <linux/suspend.h>
#include <linux/syscore_ops.h>

struct provider_clk_state {
	struct provider_clk *pvdck;
	bool prepared;
	bool enabled;
	int enable_count;
	unsigned long rate;
};

struct save_point {
	u32 spm_pwr_status;
	struct provider_clk_state clks_states[512];
#if CLKDBG_PM_DOMAIN
	struct genpd_state genpd_states[20];
	struct genpd_dev_state genpd_dev_states[100];
#endif
};

static struct save_point save_point_1;
static struct save_point save_point_2;
static struct save_point save_point_3;

static void save_pwr_status(u32 *spm_pwr_status)
{
	*spm_pwr_status = read_spm_pwr_status();
}

static void save_all_clks_state(struct provider_clk_state *clks_states)
{
	struct provider_clk *pvdck = get_all_provider_clks();
	struct provider_clk_state *st = clks_states;

	if (!pvdck)
		return;

	for (; pvdck->ck; pvdck++, st++) {
		struct clk *c = pvdck->ck;
		struct clk_hw *c_hw = __clk_get_hw(c);

		st->pvdck = pvdck;
		st->prepared = clk_hw_is_prepared(c_hw);
		st->enabled = clk_hw_is_enabled(c_hw);
		st->enable_count = __get_enable_count(c);
		st->rate = clk_hw_get_rate(c_hw);
	}
}

static void dump_provider_clk_state(struct provider_clk_state *st,
					struct seq_file *s)
{
	struct provider_clk *pvdck = st->pvdck;
	struct clk_hw *c_hw = __clk_get_hw(pvdck->ck);

	seq_printf(s, "[%10s: %-17s: %3s, %3d, %3d, %10ld]\n",
		pvdck->provider_name ? pvdck->provider_name : "/ ",
		clk_hw_get_name(c_hw),
		(st->prepared || st->enabled) ?
			"ON" : "off",
		st->prepared,
		st->enable_count,
		st->rate);
}

static void store_save_point(struct save_point *sp)
{
	save_all_clks_state(sp->clks_states);
	save_pwr_status(&sp->spm_pwr_status);

#if CLKDBG_PM_DOMAIN
	save_all_genpd_state(sp->genpd_states, sp->genpd_dev_states);
#endif
}

static void dump_save_point(struct save_point *sp, struct seq_file *s)
{
	struct provider_clk_state *st = sp->clks_states;

	for (; st->pvdck; st++)
		dump_provider_clk_state(st, s);

	seq_puts(s, "\n");
	dump_pwr_status(sp->spm_pwr_status, s);

#if CLKDBG_PM_DOMAIN
	seq_puts(s, "\n");
	dump_genpd_state(sp->genpd_states, s);
#endif
}

static int clkdbg_dump_suspend_clks_1(struct seq_file *s, void *v)
{
	dump_save_point(&save_point_1, s);
	return 0;
}

static int clkdbg_dump_suspend_clks_2(struct seq_file *s, void *v)
{
	dump_save_point(&save_point_2, s);
	return 0;
}

static int clkdbg_dump_suspend_clks_3(struct seq_file *s, void *v)
{
	dump_save_point(&save_point_3, s);
	return 0;
}

static int clkdbg_dump_suspend_clks(struct seq_file *s, void *v)
{
	if (has_clkdbg_flag(CLKDBG_EN_SUSPEND_SAVE_3) &&
			save_point_3.spm_pwr_status)
		return clkdbg_dump_suspend_clks_3(s, v);
	else if (has_clkdbg_flag(CLKDBG_EN_SUSPEND_SAVE_2) &&
			save_point_2.spm_pwr_status)
		return clkdbg_dump_suspend_clks_2(s, v);
	else if (has_clkdbg_flag(CLKDBG_EN_SUSPEND_SAVE_1) &&
			save_point_1.spm_pwr_status)
		return clkdbg_dump_suspend_clks_1(s, v);

	return 0;
}

static int clkdbg_pm_event_handler(struct notifier_block *nb,
					unsigned long event, void *ptr)
{
	switch (event) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		/* suspend */
		if (has_clkdbg_flag(CLKDBG_EN_SUSPEND_SAVE_1)) {
			store_save_point(&save_point_1);
			return NOTIFY_OK;
		}

		break;
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		/* resume */
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block clkdbg_pm_notifier = {
	.notifier_call = clkdbg_pm_event_handler,
};

static int clkdbg_syscore_suspend(void)
{
	if (has_clkdbg_flag(CLKDBG_EN_SUSPEND_SAVE_2))
		store_save_point(&save_point_2);

	return 0;
}

static void clkdbg_syscore_resume(void)
{
}

static struct syscore_ops clkdbg_syscore_ops = {
	.suspend = clkdbg_syscore_suspend,
	.resume = clkdbg_syscore_resume,
};

static int __init clkdbg_pm_init(void)
{
	register_syscore_ops(&clkdbg_syscore_ops);
	register_pm_notifier(&clkdbg_pm_notifier);

	return 0;
}

static int clkdbg_suspend_ops_valid(suspend_state_t state)
{
	return state == PM_SUSPEND_MEM;
}

static int clkdbg_suspend_ops_begin(suspend_state_t state)
{
	return 0;
}

static int clkdbg_suspend_ops_prepare(void)
{
	return 0;
}

static int clkdbg_suspend_ops_enter(suspend_state_t state)
{
	if (has_clkdbg_flag(CLKDBG_EN_SUSPEND_SAVE_3))
		store_save_point(&save_point_3);

	return 0;
}

static void clkdbg_suspend_ops_finish(void)
{
}

static void clkdbg_suspend_ops_end(void)
{
}

static const struct platform_suspend_ops clkdbg_suspend_ops = {
	.valid = clkdbg_suspend_ops_valid,
	.begin = clkdbg_suspend_ops_begin,
	.prepare = clkdbg_suspend_ops_prepare,
	.enter = clkdbg_suspend_ops_enter,
	.finish = clkdbg_suspend_ops_finish,
	.end = clkdbg_suspend_ops_end,
};

static int clkdbg_suspend_set_ops(struct seq_file *s, void *v)
{
	suspend_set_ops(&clkdbg_suspend_ops);

	return 0;
}

static const struct cmd_fn *custom_cmds;

void set_custom_cmds(const struct cmd_fn *cmds)
{
	custom_cmds = cmds;
}

static int clkdbg_cmds(struct seq_file *s, void *v);

static const struct cmd_fn common_cmds[] = {
	CMDFN("dump_regs", seq_print_regs),
	CMDFN("dump_regs2", clkdbg_dump_regs2),
	CMDFN("dump_state", clkdbg_dump_state_all),
	CMDFN("dump_clks", clkdbg_dump_provider_clks),
	CMDFN("dump_muxes", clkdbg_dump_muxes),
	CMDFN("fmeter", seq_print_fmeter_all),
	CMDFN("pwr_status", clkdbg_pwr_status),
	CMDFN("prepare", clkdbg_prepare),
	CMDFN("unprepare", clkdbg_unprepare),
	CMDFN("enable", clkdbg_enable),
	CMDFN("disable", clkdbg_disable),
	CMDFN("prepare_enable", clkdbg_prepare_enable),
	CMDFN("disable_unprepare", clkdbg_disable_unprepare),
	CMDFN("set_parent", clkdbg_set_parent),
	CMDFN("set_rate", clkdbg_set_rate),
	CMDFN("reg_read", clkdbg_reg_read),
	CMDFN("reg_write", clkdbg_reg_write),
	CMDFN("reg_set", clkdbg_reg_set),
	CMDFN("reg_clr", clkdbg_reg_clr),
	CMDFN("show_flags", clkdbg_show_flags),
	CMDFN("set_flag", clkdbg_set_flag),
	CMDFN("clr_flag", clkdbg_clr_flag),
#if CLKDBG_PM_DOMAIN
	CMDFN("dump_genpd", clkdbg_dump_genpd),
	CMDFN("pm_runtime_enable", clkdbg_pm_runtime_enable),
	CMDFN("pm_runtime_disable", clkdbg_pm_runtime_disable),
	CMDFN("pm_runtime_get_sync", clkdbg_pm_runtime_get_sync),
	CMDFN("pm_runtime_put_sync", clkdbg_pm_runtime_put_sync),
	CMDFN("pwr_on", clkdbg_pwr_on),
	CMDFN("pwr_off", clkdbg_pwr_off),
#endif /* CLKDBG_PM_DOMAIN */
	CMDFN("suspend_set_ops", clkdbg_suspend_set_ops),
	CMDFN("dump_suspend_clks", clkdbg_dump_suspend_clks),
	CMDFN("dump_suspend_clks_1", clkdbg_dump_suspend_clks_1),
	CMDFN("dump_suspend_clks_2", clkdbg_dump_suspend_clks_2),
	CMDFN("dump_suspend_clks_3", clkdbg_dump_suspend_clks_3),
	CMDFN("test_clk", clkdbg_test_one_clk),
	CMDFN("swen_cmp_w_log", sw_en_cmp_w_log_show),
	CMDFN("swen_log_dump", sw_en_dumplog_show),
	CMDFN("parent_health", clkdbg_mux_tree_health_check_p),
	CMDFN("cmds", clkdbg_cmds),
	{}
};

static int clkdbg_cmds(struct seq_file *s, void *v)
{
	const struct cmd_fn *cf;

	for (cf = common_cmds; cf->cmd; cf++)
		seq_printf(s, "%s\n", cf->cmd);

	for (cf = custom_cmds; cf && cf->cmd; cf++)
		seq_printf(s, "%s\n", cf->cmd);

	seq_puts(s, "\n");

	return 0;
}

static int clkdbg_show(struct seq_file *s, void *v)
{
	const struct cmd_fn *cf;
	char cmd[sizeof(last_cmd)+1];

	strncpy(cmd, last_cmd, sizeof(last_cmd));
	cmd[sizeof(last_cmd)] = '\0';

	for (cf = custom_cmds; cf && cf->cmd; cf++) {
		char *c = cmd;
		char *token = strsep(&c, " ");

		if (strcmp(cf->cmd, token) == 0)
			return cf->fn(s, v);
	}

	for (cf = common_cmds; cf->cmd; cf++) {
		char *c = cmd;
		char *token = strsep(&c, " ");

		if (strcmp(cf->cmd, token) == 0)
			return cf->fn(s, v);
	}

	return 0;
}

static int clkdbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, clkdbg_show, NULL);
}

static ssize_t clkdbg_write(
		struct file *file,
		const char __user *buffer,
		size_t count,
		loff_t *data)
{
	int len = 0;

	if (count == 0)
		return 0;

	len = (count < (sizeof(last_cmd) - 1)) ? count : (sizeof(last_cmd) - 1);
	if (copy_from_user(last_cmd, buffer, len))
		return 0;

	last_cmd[len] = '\0';

	if (last_cmd[len - 1] == '\n')
		last_cmd[len - 1] = 0;

	return count;
}

static const struct file_operations clkdbg_fops = {
	.owner		= THIS_MODULE,
	.open		= clkdbg_open,
	.read		= seq_read,
	.write		= clkdbg_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/*
 * init functions
 */

#include <linux/kernel.h>
static ssize_t help_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{

	static const char help_info[] = "CLK Info Help:\n"
			"- Get whole clock status:\n"
			"$ echo dump_clks > /proc/clkdbg ; cat /proc/clkdbg\n\n"
			"- Log sw en status:\n"
			"$ echo fire > /sys/kernel/clk/clk_debug/log_swen\n\n"
			"- Compare sw en status with log:\n"
			"$ echo swen_cmp_w_log > /proc/clkdbg ; cat /proc/clkdbg\n\n"
			"- Health check of mux:(suggest ooxx is the target provider)\n"
			"$ echo ooxx > /sys/kernel/clk/clk_debug/mux/tree_health_check_provider\n"
			"$ echo parent_health > /proc/clkdbg ; cat /proc/clkdbg\n";

	ssize_t count;

	count = snprintf(buf, strlen(help_info)+1, "%s", help_info);

	return count;

}

static ssize_t log_swen_store(struct kobject *obj,
			struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	const char *trigger_str = "fire";

	if (strncmp(buf, trigger_str, strlen(trigger_str))) {
		pr_err("cmd '%s' not support\n", buf);
		return -EINVAL;
	}

	clkdbg_log_sw_ens();

	return count;
}

static ssize_t tree_health_check_provider_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)

{
	ssize_t count;

	count = snprintf(buf, strlen(debug_provider)+2, "%s\n", debug_provider);

	return count;
}

static ssize_t tree_health_check_provider_store(struct kobject *obj,
			struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	int len = strlen(buf) < 255 ? strlen(buf):254;

	memset(debug_provider, 0, 256);

	if (len > 1) {
		strncpy(debug_provider, buf, len);
		debug_provider[len-1] = '\0';
	}

	return len;
}



/* HELP */
static struct kobj_attribute _help = __ATTR_RO(help);
static struct attribute *clkinfo_attrs[] = {
	&_help.attr,
	NULL,
};

/* SW EN debug */
static struct kobj_attribute log_swen = __ATTR_WO(log_swen);

static struct attribute *clkdbg_attrs[] = {
	&log_swen.attr,
	NULL,
};
/* mux debug */
static struct kobj_attribute tree_health_check_provider
			= __ATTR_RW(tree_health_check_provider);

static struct attribute *clkdbg_mux_attrs[] = {
	&tree_health_check_provider.attr,
	NULL,
};


static const struct attribute_group clk_info_attr_group = {
	.attrs = clkinfo_attrs,
};

static const struct attribute_group clk_dbg_attr_group = {
	.attrs = clkdbg_attrs,
};

static const struct attribute_group clk_dbgmux_attr_group = {
	.attrs = clkdbg_mux_attrs,
};

static struct kobject *clk_kobj;
static struct kobject *clk_info_kobj;
static struct kobject *clk_debug_kobj;
static struct kobject *clk_debug_mux_kobj;

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
static struct dentry *mux_debug_dentry;
static struct dentry *swen_debug_dentry;
extern int clock_read(enum merak_regbank_type bank_type, unsigned int reg_addr,
	unsigned int start, unsigned int end);

static int ckgen_debug_show(struct seq_file *s, void *data)
{
	struct device_node *target = (struct device_node *)s->private;
	struct device_node *child = NULL;
	int ret;
	enum merak_regbank_type bank_type;
	u32 _reg[3] = {0};

	if (!target)
		return 0;

	for_each_child_of_node(target, child) {
		if (of_property_read_bool(child, "NONPM"))
			bank_type = CLK_NONPM;
		else if (of_property_read_bool(child, "PM"))
			bank_type = CLK_PM;
		else
			seq_printf(s ,"%s pm/nonpm NA\n", child->name);

		ret = of_property_read_u32_array(child, "reg", _reg, sizeof(_reg)/sizeof(u32));
		if (ret >= 0) {
			seq_printf(s ,"%s 0x%x\n", child->name,
				clock_read(CLK_NONPM, _reg[0], _reg[1], _reg[2]));
		} else
			seq_printf(s ,"%s reg NA\n", child->name);

	}

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(ckgen_debug);

static struct dentry *pll_debug_root;
static struct dentry *monitor_dentry;
static int crystal_ref_show(struct seq_file *s, void *data)
{
	struct device_node *target = (struct device_node *)s->private;
	u32 spec = 0, ret = 0;

	ret = of_property_read_u32_index(target, "spec", 0, &spec);
	if (ret < 0) {
		seq_puts(s, "get spec error\n");
		return ret;
	}
	seq_printf(s, "%d\n", spec);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(crystal_ref);

static int crystal_maxdiff_show(struct seq_file *s, void *data)
{
	struct device_node *target = (struct device_node *)s->private;
	u32 diff = 0, ret = 0;

	ret = of_property_read_u32_index(target, "spec", 1, &diff);
	if (ret < 0) {
		seq_puts(s, "get spec error\n");
		return ret;
	}

	seq_printf(s, "%d\n", diff);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(crystal_maxdiff);

#define MAX_DATA	6
#define LEAST_DATA	4
#define DATA_STAR_INDEX	2
#define DATA_IN_ONE	2
static int crystal_measure_2level_show(struct seq_file *s, void *data)
{
	struct device_node *target = (struct device_node *)s->private;
	u32 test_ctrl_para[MAX_DATA] = {0};
	int ret = 0, freq = 0, div = 0, i = DATA_STAR_INDEX;
	void __iomem *setting;

	ret = of_property_read_variable_u32_array(target, "cal", test_ctrl_para,
					LEAST_DATA, MAX_DATA);
	if (ret < 0) {
		seq_puts(s, "get cal error\n");
		return ret;
	}

	while (i < (MAX_DATA - 1) && test_ctrl_para[i] != 0) {
		unsigned long tmp = test_ctrl_para[i] & PAGE_MASK;
		unsigned long diff = test_ctrl_para[i] - tmp;

		setting = ioremap_wc(tmp, PAGE_SIZE);
		writew(test_ctrl_para[i+1], setting+diff);
		iounmap(setting);
		i = i + DATA_IN_ONE;
	}
	freq = set_calc_enable(s, test_ctrl_para[0]);

	/* Reset seq_print */
	s->from = 0;
	s->count = 0;
	s->version = 0;
	s->index = 0;

	div = test_ctrl_para[1];
	seq_printf(s, "%d\n", freq*TEST_CTRL_REF*div/TEST_CTRL_DIV);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(crystal_measure_2level);

#define	CLK_PHY_INDEX	0
#define	CLK_NUM_INDEX	1
#define CLK_DIV_INDEX	2
#define CLK_DATA_NUM	3
static int crystal_measure_show(struct seq_file *s, void *data)
{
	struct device_node *target = (struct device_node *)s->private;
	u32 test_ctrl_para[CLK_DATA_NUM] = {0}; /* _phy _num div */
	int ret = 0, freq = 0, div = 0;

	ret = of_property_read_u32_array(target, "cal",
				test_ctrl_para, CLK_DATA_NUM);
	if (ret) {
		seq_puts(s, "get cal error\n");
		return ret;
	}

	freq = test_one_clk(s, NULL, test_ctrl_para[CLK_PHY_INDEX],
			test_ctrl_para[CLK_NUM_INDEX], test_ctrl_para[CLK_DIV_INDEX], 0);

	/* Reset seq_print */
	s->from = 0;
	s->count = 0;
	s->version = 0;
	s->index = 0;

	div = 1<<(test_ctrl_para[CLK_DIV_INDEX]-1);
	seq_printf(s, "%d\n", freq*TEST_CTRL_REF*div/TEST_CTRL_DIV);

	return ret;
}
DEFINE_SHOW_ATTRIBUTE(crystal_measure);
#endif

static int __init clkdbg_debug_init(void)
{
	struct proc_dir_entry *entry = NULL;
#ifdef CONFIG_DEBUG_FS
	struct device_node *pll_debug_node = NULL;
	struct device_node *mux_debug_node = NULL;
	struct device_node *swen_debug_node = NULL;
	struct device_node *tmp = NULL;
	struct dentry *tmp_dentry = NULL;
#endif
	entry = proc_create("clkdbg", 0, 0, &clkdbg_fops);
	if (!entry)
		return -ENOMEM;

	set_clkdbg_flag(CLKDBG_EN_SUSPEND_SAVE_3);
	clkdbg_pm_init();

	clk_kobj = kobject_create_and_add("clk", kernel_kobj);
	if (!clk_kobj)
		return -ENODEV;

	clk_info_kobj = kobject_create_and_add("clk_info", clk_kobj);
	if (!clk_info_kobj) {
		kobject_put(clk_kobj);
		return -ENOMEM;
	}
	sysfs_create_group(clk_info_kobj, &clk_info_attr_group);

	clk_debug_kobj = kobject_create_and_add("clk_debug", clk_kobj);
	if (!clk_debug_kobj)
		return -ENOMEM;
	sysfs_create_group(clk_debug_kobj, &clk_dbg_attr_group);

	clk_debug_mux_kobj = kobject_create_and_add("mux", clk_debug_kobj);
	if (!clk_debug_mux_kobj)
		return -ENOMEM;
	sysfs_create_group(clk_debug_mux_kobj, &clk_dbgmux_attr_group);

#ifdef CONFIG_DEBUG_FS
	pll_debug_node = of_find_node_by_name(NULL, "debug_plls");
	if (!pll_debug_node)
		goto debug_setup_mux;

	pll_debug_root = debugfs_create_dir("pll", NULL);
	monitor_dentry = debugfs_create_dir("monitor", pll_debug_root);

	tmp = of_find_compatible_node(pll_debug_node, NULL, "mediatek,pll_debug_1");
	while (tmp) {
		pr_warn("support pll_1:(%s) %u\n", tmp->name, tmp->phandle);
		tmp_dentry = debugfs_create_dir(tmp->name, monitor_dentry);
		debugfs_create_file("measure", 0440, tmp_dentry, tmp,
					&crystal_measure_fops);
		debugfs_create_file("ref", 0440, tmp_dentry, tmp,
					&crystal_ref_fops);
		debugfs_create_file("maxdiff", 0440, tmp_dentry, tmp,
					&crystal_maxdiff_fops);
		tmp = of_find_compatible_node(tmp, NULL, "mediatek,pll_debug_1");
	}

	tmp = of_find_compatible_node(pll_debug_node, NULL, "mediatek,pll_debug_2");
	while (tmp) {
		pr_warn("support pll_2:(%s) %u\n", tmp->name, tmp->phandle);
		tmp_dentry = debugfs_create_dir(tmp->name, monitor_dentry);
		debugfs_create_file("measure", 0440, tmp_dentry, tmp,
					&crystal_measure_2level_fops);
		debugfs_create_file("ref", 0440, tmp_dentry, tmp,
					&crystal_ref_fops);
		debugfs_create_file("maxdiff", 0440, tmp_dentry, tmp,
					&crystal_maxdiff_fops);
		tmp = of_find_compatible_node(tmp, NULL, "mediatek,pll_debug_2");
	}
	of_node_put(tmp);

debug_setup_mux:
	mux_debug_node = of_find_node_by_name(NULL, "debug_mux");
	if (!mux_debug_node)
		goto debug_setup_swen;

	mux_debug_dentry = debugfs_create_dir("debug_mux", NULL);
	debugfs_create_file("dump", 0440, mux_debug_dentry, mux_debug_node,
				&ckgen_debug_fops);

debug_setup_swen:
	swen_debug_node = of_find_node_by_name(NULL, "debug_swen");
	if (!swen_debug_node)
		goto exit;

	swen_debug_dentry = debugfs_create_dir("debug_swen", NULL);
	debugfs_create_file("dump", 0440, swen_debug_dentry, swen_debug_node,
				&ckgen_debug_fops);

#endif
exit:
	return 0;
}

#ifdef MODULE
module_init(clkdbg_debug_init);
MODULE_DESCRIPTION("dbg for clk");
MODULE_LICENSE("GPL v2");
#else
module_init(clkdbg_debug_init);
#endif
