// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/debugfs.h>
#include <linux/version.h>
#include <linux/dma-direct.h>

#include <common/mdla_device.h>
#include <common/mdla_cmd_proc.h>
#include <common/mdla_power_ctrl.h>
#include <common/mdla_ioctl.h>

#include <utilities/mdla_debug.h>
#include <utilities/mdla_profile.h>
#include <utilities/mdla_util.h>
#include <utilities/mdla_trace.h>

#include <interface/mdla_intf.h>
#include <interface/mdla_cmd_data_v2_0.h>

#include <platform/mdla_plat_api.h>
#include <platform/mdla_plat_internal.h>
#include <platform/v2_0/mdla_hw_reg_v2_0.h>
#include <platform/v2_0/mdla_irq_v2_0.h>
#include <platform/v2_0/mdla_pmu_v2_0.h>
#include <platform/v2_0/mdla_sched_v2_0.h>
#include "mdla_mt589x_reg_v2_0.h"
#include "mdla_teec_api.h"
#include <comm_driver.h>

static u32 mdla_suspend;

static void __iomem *reg_sctrl_ctxt;

static int mdla_plat_pre_cmd_handle(u32 core_id, struct command_entry *ce)
{
	const struct mdla_util_io_ops *io = mdla_util_io_ops_get();

	if (mdla_dbg_read_u32(FS_DUMP_CMDBUF))
		mdla_dbg_plat_cb()->create_dump_cmdbuf(mdla_get_device(core_id), ce);

	if (!mdla_dbg_read_u32(FS_SECURE_DRV_EN))
		io->cmde.write(core_id, MREG_TOP_G_INTP2,
			MDLA_IRQ_MASK & ~(INTR_SWCMD_DONE));

	return 0;
}

static int mdla_plat_post_cmd_handle(u32 core_id, struct command_entry *ce)
{
	/* clear current event id */
	mdla_util_io_ops_get()->cmde.write(core_id, MREG_TOP_G_CDMA4, 1);

	return 0;
}

static void mdla_plat_raw_process_command(u32 core_id, u32 evt_id,
						dma_addr_t addr, u32 count)
{
	const struct mdla_util_io_ops *io = mdla_util_io_ops_get();

	/* set command address */
	io->cmde.write(core_id, MREG_TOP_G_CDMA1, addr);
	/* set command number */
	io->cmde.write(core_id, MREG_TOP_G_CDMA2, count);
	/* trigger hw */
	io->cmde.write(core_id, MREG_TOP_G_CDMA3, evt_id);
}

static int mdla_plat_process_command(u32 core_id, struct command_entry *ce)
{
	dma_addr_t addr;
	u32 evt_id, count;
	int ret = 0;

	ce->state = CE_RUN;
	//spin_lock_irqsave(&mdla_get_device(core_id)->hw_lock, flags);
	if (!mdla_dbg_read_u32(FS_SECURE_DRV_EN) || !ce->cmd_svp) {
		addr = ce->mva;
		count = ce->count;
		evt_id = ce->count;

		mdla_drv_debug("%s: count: %d, addr: %lx\n",
			__func__, ce->count,
			(unsigned long)addr);

		mdla_plat_raw_process_command(core_id, evt_id, addr, count);
	} else {
		ret = mdla_plat_teec_send_command(ce->cmd_svp);
	}
	//spin_unlock_irqrestore(&mdla_get_device(core_id)->hw_lock, flags);

	return ret;
}

static int mdla_plat_suspend(u32 core_id)
{
	struct mdla_dev *mdla_info = mdla_get_device(core_id);

	mdla_drv_debug("%s(): Core (%d) suspend\n", __func__, core_id);

	// wait unfinished inference job complete
	mutex_lock(&mdla_info->cmd_lock);
	mdla_suspend = 1;
	mutex_unlock(&mdla_info->cmd_lock);

	return 0;
}

static int mdla_plat_resume(u32 core_id)
{
	mdla_drv_debug("%s(): Core (%d) resume\n", __func__, core_id);
	mdla_suspend = 0;

	return 0;
}

static void mdla_v2_0_reset(u32 core_id, const char *str)
{
	unsigned long flags;
	struct mdla_dev *dev = mdla_get_device(core_id);

	if (unlikely(!dev)) {
		mdla_err("%s(): No mdla device (%d)\n", __func__, core_id);
		return;
	}

	/* use power down==>power on apis insted bus protect init */
	mdla_drv_debug("%s: MDLA RESET: %s\n", __func__,
		str);

	if (!mdla_dbg_read_u32(FS_SECURE_DRV_EN)) {
		spin_lock_irqsave(&dev->hw_lock, flags);
		iowrite32((ioread32(reg_sctrl_ctxt) & ~0x3) | 0x2, reg_sctrl_ctxt);
		spin_unlock_irqrestore(&dev->hw_lock, flags);
	}

	mdla_trace_reset(core_id, str);
}

static int mdla_plat_check_resource(u32 core)
{
#ifdef ENABLE_VIRTUAL_HASHKEY
	u32 hw_support;

	hw_support = comm_util_get_cb()->hw_supported();

	mdla_drv_debug("%s(): enable vir_hashkey option (%x)\n",
			__func__, hw_support);
	return (!hw_support | mdla_suspend);
#endif
	return mdla_suspend;
}

/* platform public functions */

int mdla_mt589x_v2_0_init(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mdla_plat_drv *drv_data;
	struct mdla_cmd_cb_func *cmd_cb = mdla_cmd_plat_cb();

	dev_info(&pdev->dev, "%s()\n", __func__);

	reg_sctrl_ctxt = ioremap_nocache(APU_SCTRL_PGRMP_SCTRL_MDLA_CORE0_CTXT, PAGE_SIZE);
	if (!reg_sctrl_ctxt) {
		dev_info(&pdev->dev, "%s() ioremap reg_sctrl_ctxt fail\n", __func__);
		return -ENOMEM;
	}

	if (mdla_v2_0_init(pdev)) {
		dev_info(&pdev->dev, "%s() mdla_v2_0 init fail.\n", __func__);
		return -1;
	}

	drv_data = (struct mdla_plat_drv *)of_device_get_match_data(dev);
	if (!drv_data)
		return -1;

	mdla_pwr_reset_setup(mdla_v2_0_reset);

	/* replace command callback */
	cmd_cb->pre_cmd_handle      = mdla_plat_pre_cmd_handle;
	cmd_cb->post_cmd_handle     = mdla_plat_post_cmd_handle;
	cmd_cb->process_command     = mdla_plat_process_command;

	cmd_cb->suspend_cb = mdla_plat_suspend;
	cmd_cb->resume_cb = mdla_plat_resume;
	cmd_cb->check_resource = mdla_plat_check_resource;

	mdla_ioctl_set_efuse(0);

	return 0;
}

void mdla_mt589x_v2_0_deinit(struct platform_device *pdev)
{
	mdla_drv_debug("%s unregister power -\n", __func__);
	iounmap(reg_sctrl_ctxt);
	mdla_v2_0_deinit(pdev);
}
