// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/delay.h>
//#include <soc/mediatek/smi.h>

#include "mtk_vcodec_enc_pm.h"
#include "mtk_vcodec_util.h"
#if IS_ENABLED(CONFIG_VIDEO_MEDIATEK_VCU)
#include "mtk_vcu.h"
#endif

#ifdef CONFIG_MTK_PSEUDO_M4U
#include <mach/mt_iommu.h>
#include "mach/pseudo_m4u.h"
#include "smi_port.h"
#endif

#define USE_GCE 1
#ifdef ENC_DVFS
#include <linux/pm_qos.h>
#include <mmdvfs_pmqos.h>
#include "vcodec_dvfs.h"
#define STD_VENC_FREQ 250
#define STD_LUMA_BW 100
#define STD_CHROMA_BW 50
static struct pm_qos_request venc_qos_req_f;
static u64 venc_freq;

static u32 venc_freq_step_size;
static u64 venc_freq_steps[MAX_FREQ_STEP];
/*
 * static struct codec_history *venc_hists;
 * static struct codec_job *venc_jobs;
 */
/* 1080p60, 4k30, 4k60, 1 core 4k60*/
static u64 venc_freq_map[] = {249, 364, 458, 624};

#endif

#ifdef ENC_EMI_BW
#include <mtk_smi.h>
#include <mtk_qos_bound.h>
#include "vcodec_bw.h"
#define CORE_NUM 1
/*
 * static unsigned int gVENCFrmTRAVC[3] = {6, 12, 6};
 * static unsigned int gVENCFrmTRHEVC[3] = {6, 12, 6};
 *
 * static long long venc_start_time;
 * static struct vcodec_bw *venc_bw;
 */
static struct plist_head venc_rlist;
static struct mm_qos_request venc_rcpu;
static struct mm_qos_request venc_rec;
static struct mm_qos_request venc_bsdma;
static struct mm_qos_request venc_sv_comv;
static struct mm_qos_request venc_rd_comv;
static struct mm_qos_request venc_cur_luma;
static struct mm_qos_request venc_cur_chroma;
static struct mm_qos_request venc_ref_luma;
static struct mm_qos_request venc_ref_chroma;
static struct mm_qos_request venc_sub_r_luma;
static struct mm_qos_request venc_sub_w_luma;
#endif

#define VENC_FRMAE_RATE_30 (30)
#define VENC_FHD_WIDTH (1920)
#define VENC_FHD_HEIGHT (1088)
#define VENC_CLK_432M (432 * 1024 *1024)
#define VENC_CLK_240M (240 * 1024 *1024)

/* first scenario based version, will change to loading based */
struct temp_job {
	int ctx_id;
	int format;
	int type;
	int module;
	int visible_width; /* temp usage only, will use kcy */
	int visible_height; /* temp usage only, will use kcy */
	int operation_rate;
	long long submit;
	int kcy;
	struct temp_job *next;
};

void mtk_venc_init_ctx_pm(struct mtk_vcodec_ctx *ctx)
{
	ctx->async_mode = 0;
}

int mtk_vcodec_init_enc_pm(struct mtk_vcodec_dev *mtkdev)
{
	int ret = 0;
	struct platform_device *pdev;
	struct mtk_vcodec_pm *pm;

	pdev = mtkdev->plat_dev;
	pm = &mtkdev->pm;
	memset(pm, 0, sizeof(struct mtk_vcodec_pm));

	pm->venc_clk = devm_clk_get(&pdev->dev, "venc_clk");
	if (IS_ERR(pm->venc_clk)) {
		mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get venc_clk\n");
		pm->venc_clk = NULL;
	}

	pm->venc_ip = devm_clk_get(&pdev->dev, "venc_ip_swen");
	if (IS_ERR(pm->venc_ip)) {
		mtk_v4l2_err("[VCODEC][ERROR] Unable to devm_clk_get ip_swen\n");
		pm->venc_ip = NULL;
	}

	return ret;
}

void mtk_vcodec_release_enc_pm(struct mtk_vcodec_dev *mtkdev)
{
	struct platform_device *pdev;
	struct mtk_vcodec_pm *pm;

	pdev = mtkdev->plat_dev;
	pm = &mtkdev->pm;

	if (pm->venc_clk)
		devm_clk_put(&pdev->dev, pm->venc_clk);

	if (pm->venc_ip)
		devm_clk_put(&pdev->dev, pm->venc_ip);

	mtk_v4l2_debug(VCODEC_DBG_L4, "done");
}

void mtk_venc_deinit_ctx_pm(struct mtk_vcodec_ctx *ctx)
{
}

void mtk_vcodec_enc_clock_on(struct mtk_vcodec_ctx *ctx, int core_id)
{
	struct mtk_vcodec_pm *pm = &ctx->dev->pm;
	struct mtk_enc_params *enc_params = &ctx->enc_params;
	struct mtk_q_data *q_data_src = &ctx->q_data[MTK_Q_DATA_SRC];
	int ret;
	unsigned long long capability;
	long round_rate;

	if (pm->venc_clk)
	{
		capability = (unsigned long long)q_data_src->coded_width *
					q_data_src->coded_height *
					enc_params->framerate_num /
					enc_params->framerate_denom;

		if (capability > VENC_FHD_WIDTH * VENC_FHD_HEIGHT * VENC_FRMAE_RATE_30) {

			round_rate = clk_round_rate(pm->venc_clk, VENC_CLK_432M);
			clk_set_rate(pm->venc_clk, round_rate);
		} else {
			round_rate = clk_round_rate(pm->venc_clk, VENC_CLK_240M);
			clk_set_rate(pm->venc_clk, round_rate);
		}
	}

	if (pm->venc_ip) {
		ret = clk_prepare_enable(pm->venc_ip);
		if (ret)
			mtk_v4l2_err("clk_prepare_enable venc_ip fail %d", ret);
	}
}

void mtk_vcodec_enc_clock_off(struct mtk_vcodec_ctx *ctx, int core_id)
{
	struct mtk_vcodec_pm *pm = &ctx->dev->pm;

	if (pm->venc_ip)
		clk_disable_unprepare(pm->venc_ip);
}

#ifndef TV_ENCODE_INTEGRATION
void mtk_prepare_venc_dvfs(void)
{

}

void mtk_unprepare_venc_dvfs(void)
{

}

void mtk_prepare_venc_emi_bw(void)
{

}

void mtk_unprepare_venc_emi_bw(void)
{

}


void mtk_venc_dvfs_begin(struct temp_job **job_list)
{

}

void mtk_venc_dvfs_end(struct temp_job *job)
{

}

void mtk_venc_emi_bw_begin(struct temp_job **jobs)
{

}

void mtk_venc_emi_bw_end(struct temp_job *job)
{

}

void mtk_venc_pmqos_prelock(struct mtk_vcodec_ctx *ctx, int core_id)
{
}

void mtk_venc_pmqos_begin_frame(struct mtk_vcodec_ctx *ctx, int core_id)
{
}

void mtk_venc_pmqos_end_frame(struct mtk_vcodec_ctx *ctx, int core_id)
{
}

/* Total job count after this one is inserted */
void mtk_venc_pmqos_gce_flush(struct mtk_vcodec_ctx *ctx, int core_id,
				int job_cnt)
{

}

/* Remaining job count after this one is done */
void mtk_venc_pmqos_gce_done(struct mtk_vcodec_ctx *ctx, int core_id,
				int job_cnt)
{

}
#endif

int mtk_venc_ion_config_buff(struct dma_buf *dmabuf)
{
/* for dma-buf using ion buffer, ion will check portid in dts
 * So, don't need to config buffer at user side, but remember
 * set iommus attribute in dts file.
 */

	return 0;
}

