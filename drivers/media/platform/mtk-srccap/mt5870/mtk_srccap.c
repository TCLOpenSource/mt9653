// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <asm/siginfo.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/videodev2.h>
#include <linux/errno.h>
#include <linux/compat.h>
#include <linux/delay.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-sg.h>
#include <media/videobuf2-memops.h>

#include "mtk_srccap.h"
#include "mtk_srccap_hdmirx.h"
#include "mtk_srccap_adc.h"
#include "mtk_srccap_avd.h"
#include "mtk_srccap_avd_avd.h"
#include "mtk_srccap_avd_mux.h"
#include "mtk_srccap_avd_colorsystem.h"
#include "mtk_srccap_common.h"
#include "mtk_srccap_mux.h"
#include "mtk_srccap_offlinedetect.h"
#include "mtk_srccap_timingdetect.h"
#include "mtk_srccap_dscl.h"
#include "mtk_srccap_memout.h"
//#include "mtk_srccap_vbi.h"
#include "drv_HDMI.h"
//#include "mtk_drv_vbi.h"
#include "apiXC_ModeParse.h"
#include "apiXC.h"
#include "mtk_srccap_adc_drv.h"
#include "mtk_srccap_common_drv.h"
#include "mtk_srccap_timingdetect_drv.h"
#include "mtk_srccap_mux_drv.h"
#include "hwreg_common_riu.h"
#include "mtk-reserved-memory.h"

bool gSrccapInitfunc = FALSE;
struct srccap_capability gstSrccapCapability[2] = {{0}, {0} };
int dbglevel;

module_param(dbglevel, int, 0444); //S_IRUGO = 0444

static inline struct mtk_srccap_ctx *mtk_srccap_fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct mtk_srccap_ctx, fh);
}

/* ============================================================================================== */
/* --------------------------------------- v4l2_ioctl_ops --------------------------------------- */
/* ============================================================================================== */
static int mtk_srccap_s_input(
	struct file *file,
	void *fh,
	unsigned int input)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if ((input == V4L2_SRCCAP_INPUT_SOURCE_NONE)
		|| (input >= V4L2_SRCCAP_INPUT_SOURCE_NUM)) {
		return -EINVAL;
	}

	srccap_dev->srccap_info.src = input;

	mtk_avd_GetInputSource(srccap_dev->srccap_info.src);
//	mtk_srccap_drv_adc_set_source(srccap_dev->srccap_info.src, srccap_dev);
//	mtk_srccap_drv_mux_set_source(srccap_dev->srccap_info.src, srccap_dev);

	mtk_adc_set_port(srccap_dev);
	mtk_hdmirx_set_current_port(input);

	return 0;
}

static int mtk_srccap_g_input(
	struct file *file,
	void *fh,
	unsigned int *input)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	*input = srccap_dev->srccap_info.src;
	return 0;
}

static int mtk_srccap_g_edid(
	struct file *file,
	void *fh,
	struct v4l2_edid *edid)
{
	return mtk_hdmirx_g_edid(edid);
}

static int mtk_srccap_s_edid(
	struct file *file,
	void *fh,
	struct v4l2_edid *edid)
{
	return mtk_hdmirx_s_edid(edid);
}

static int mtk_scrcap_s_std(
	struct file *file,
	void *fh,
	v4l2_std_id std)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (std < V4L2_STD_UNKNOWN || std > V4L2_STD_ALL) {
		pr_err("Invalid STD \r\n");
		return -EINVAL;
	}

	srccap_dev->avd_info.region_type = std;
	mtk_avd_SetColorSystem(srccap_dev->avd_info.region_type);

	return 0;
}

static int mtk_scrcap_g_std(
	struct file *file,
	void *fh,
	v4l2_std_id *std)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	*std = srccap_dev->avd_info.region_type;
	return 0;
}

static int mtk_srccap_g_audio(
	struct file *file,
	void *fh,
	struct v4l2_audio *audio)
{
	return mtk_hdmirx_g_audio_channel_status(audio);
}

static int mtk_srccap_g_dv_timings(
	struct file *file,
	void *fh,
	struct v4l2_dv_timings *timings)
{
	return mtk_hdmirx_g_dv_timings(timings);
}

static int mtk_srccap_s_fmt_vid_cap(
	struct file *file,
	void *fh,
	struct v4l2_format *format)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	return mtk_memout_s_fmt_vid_cap(srccap_dev, format);
}

static int mtk_srccap_s_fmt_vid_cap_mplane(
	struct file *file,
	void *fh,
	struct v4l2_format *format)
{
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	ret = mtk_hdmirx_store_color_fmt(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return mtk_memout_s_fmt_vid_cap_mplane(srccap_dev, format);
}

static int mtk_srccap_g_fmt_vid_cap(
	struct file *file,
	void *fh,
	struct v4l2_format *format)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	return mtk_memout_g_fmt_vid_cap(srccap_dev, format);
}

static int mtk_srccap_g_fmt_vid_cap_mplane(
	struct file *file,
	void *fh,
	struct v4l2_format *format)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	return mtk_memout_g_fmt_vid_cap_mplane(srccap_dev, format);
}

static int mtk_srccap_reqbufs(
	struct file *file,
	void *fh,
	struct v4l2_requestbuffers *req_buf)
{
	int ret = 0;
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_ctx = mtk_srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = v4l2_m2m_reqbufs(file, srccap_ctx->m2m_ctx, req_buf);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return mtk_memout_reqbufs(srccap_dev, req_buf);
}

static int mtk_srccap_querybuf(
	struct file *file,
	void *fh,
	struct v4l2_buffer *buf)
{
	int ret = 0;
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_ctx = mtk_srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret =  v4l2_m2m_querybuf(file, srccap_ctx->m2m_ctx, buf);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

static int mtk_srccap_qbuf(
	struct file *file,
	void *fh,
	struct v4l2_buffer *buf)
{
	int ret = 0;
	struct v4l2_plane *plane_ptr = NULL;
	struct v4l2_memout_buf *buf_ptr = NULL;
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;
	int plane_cnt = 0;
	s32 frame_fd = 0, meta_fd = 0;

	srccap_ctx = mtk_srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

#ifdef CONFIG_OPTEE
	/* config buffer security */
	switch (buf->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		for (plane_cnt = 0; plane_cnt < (buf->length - 1); plane_cnt++) {
			if (buf->memory == V4L2_MEMORY_USERPTR) {
				plane_ptr = &(buf->m.planes[plane_cnt]);
				buf_ptr = (struct v4l2_memout_buf *)plane_ptr->m.userptr;

				ret = mtk_memout_cfg_buf_security(srccap_dev,
					buf_ptr->frame_fd, buf_ptr->meta_fd);
			}

			if (buf->memory == V4L2_MEMORY_DMABUF) {
				frame_fd = buf->m.planes[plane_cnt].m.fd;
				meta_fd = buf->m.planes[buf->length - 1].m.fd;

				ret = mtk_memout_cfg_buf_security(srccap_dev, frame_fd, meta_fd);
			}
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				SRCCAP_MSG_ERROR("[SVP] config mplanes[%u] security fail\n",
					plane_cnt);
				return ret;
			}
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		if (buf->memory == V4L2_MEMORY_USERPTR) {
			buf_ptr = (struct v4l2_memout_buf *)buf->m.userptr;
			ret = mtk_memout_cfg_buf_security(srccap_dev,
				buf_ptr->frame_fd, buf_ptr->meta_fd);
		}

		if (buf->memory == V4L2_MEMORY_DMABUF) {
			frame_fd = buf->m.planes[0].m.fd;
			meta_fd = buf->m.planes[1].m.fd;

			ret = mtk_memout_cfg_buf_security(srccap_dev, frame_fd, meta_fd);
		}
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			SRCCAP_MSG_ERROR("[SVP] config buf security fail\n");
			return ret;
		}
		break;
	default:
		SRCCAP_MSG_ERROR("[SVP] Unsupport buf type: %d\n", buf->type);
		return -EINVAL;
	}
#endif

	ret = v4l2_m2m_qbuf(file, srccap_ctx->m2m_ctx, buf);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

static int mtk_srccap_dqbuf(
	struct file *file,
	void *fh,
	struct v4l2_buffer *buf)
{
	int ret = 0;
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_ctx = mtk_srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = v4l2_m2m_dqbuf(file, srccap_ctx->m2m_ctx, buf);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return mtk_memout_dqbuf(srccap_dev, buf);
}

static int timing_monitor_task(void *data)
{
	if (data == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	struct v4l2_event event;
	static enum v4l2_srccap_pcm_status pre_avd_status = V4L2_SRCCAP_PCM_UNSTABLE;
	struct v4l2_srccap_pcm avd_pcm;
	struct v4l2_timing stTimingInfo;

	memset(&pre_avd_status, 0, sizeof(struct v4l2_srccap_pcm));
	memset(&stTimingInfo, 0, sizeof(struct v4l2_timing));

#ifdef T32_AVD_SIMULATION
	if ((srccap_dev->srccap_info.src <= V4L2_SRCCAP_INPUT_SOURCE_CVBS8) &&
		(srccap_dev->srccap_info.src >= V4L2_SRCCAP_INPUT_SOURCE_CVBS)) {
		/* set mux */
		MS_S16 s16AVPathId;
		XC_MUX_PATH_INFO AVPathInfo;
		XC_MUX_PATH_INFO AVAllPathInfo[MAX_SYNC_DATA_PATH_SUPPORTED];
		MS_U8 u8AVCount = 0;

		memset(&AVPathInfo, 0, sizeof(XC_MUX_PATH_INFO));
		memset(AVAllPathInfo, 0, sizeof(XC_MUX_PATH_INFO)*MAX_SYNC_DATA_PATH_SUPPORTED);

		//step 1. MApi_XC_Mux_GetPathInfo
		u8AVCount = MApi_XC_Mux_GetPathInfo(AVAllPathInfo);

		if (u8AVCount > 0) {
			do {
				u8AVCount = u8AVCount - 1;

				if (AVAllPathInfo[u8AVCount].dest != OUTPUT_SCALER_MAIN_WINDOW)
					break;
				if (AVAllPathInfo[u8AVCount].src == INPUT_SOURCE_CVBS) {
					SRCCAP_MSG_ERROR(
						"eInputSrc %u was connected to OUTPUT_SCALER_MAIN_WINDOW.\n",
						INPUT_SOURCE_CVBS);
				} else {
					if (MApi_XC_Mux_DeletePath
						(AVAllPathInfo[u8AVCount].src,
						OUTPUT_SCALER_MAIN_WINDOW) == -1)
						SRCCAP_MSG_ERROR(
							" Delete path fail: src = %d, dest = %d\n",
							AVAllPathInfo[u8AVCount].src,
							OUTPUT_SCALER_MAIN_WINDOW);
					break;
				}
			} while (u8AVCount > 0);
		}
		AVPathInfo.Path_Type = PATH_TYPE_SYNCHRONOUS;
		AVPathInfo.src = INPUT_SOURCE_CVBS;
		AVPathInfo.dest = OUTPUT_SCALER_MAIN_WINDOW;
		AVPathInfo.SyncEventHandler = NULL;
		AVPathInfo.DestOnOff_Event_Handler = NULL;
		AVPathInfo.path_thread = NULL;
		AVPathInfo.dest_periodic_handler = NULL;
		//step 2. MApi_XC_Mux_CreatePath
		s16AVPathId = MApi_XC_Mux_CreatePath(&AVPathInfo, sizeof(XC_MUX_PATH_INFO));

		//step 3. MApi_XC_Mux_EnablePath
		MApi_XC_Mux_EnablePath(s16AVPathId);

		//step 4. MApi_XC_SetInputSource
		MApi_XC_SetInputSource(INPUT_SOURCE_CVBS, 0);

		//step 5. Add ADC register
		MApi_XC_Write2ByteMask(0x102502, 0x0400, 0xFFFF);
		MApi_XC_Write2ByteMask(0x102504, 0x0900, 0xFF00);
		MApi_XC_Write2ByteMask(0x102508, 0x00E0, 0x00FF);
		MApi_XC_Write2ByteMask(0x10250A, 0x0E01, 0xFFFF);
		MApi_XC_Write2ByteMask(0x10250C, 0x0000, 0xFFFF);
		MApi_XC_Write2ByteMask(0x102570, 0x0001, 0x000F);
		MApi_XC_Write2ByteMask(0x10257A, 0x0000, 0x000F);
		MApi_XC_Write2ByteMask(0x10257E, 0x0002, 0x000F);
		MApi_XC_Write2ByteMask(0x102586, 0x4C52, 0xFFFF);
		MApi_XC_Write2ByteMask(0x102590, 0x00C8, 0x00FF);
		MApi_XC_Write2ByteMask(0x1025B4, 0x0001, 0x000F);
		MApi_XC_Write2ByteMask(0x1025B6, 0x2000, 0xF000);
		MApi_XC_Write2ByteMask(0x1025CE, 0x0100, 0x0F00);
		MApi_XC_Write2ByteMask(0x1025F2, 0x0028, 0x00FF);
		MApi_XC_Write2ByteMask(0x1025F6, 0x0060, 0x00F0);
		MApi_XC_Write2ByteMask(0x1025FC, 0x0000, 0x0FFF);
		MApi_XC_Write2ByteMask(0x1025FE, 0x0000, 0xFFF0);
	}
#endif

#ifdef T32_HDMI_SIMULATION
	bool en = FALSE;
	static enum v4l2_srccap_pcm_status pre_status = V4L2_SRCCAP_PCM_UNSTABLE;
	struct v4l2_srccap_pcm_timing_count timing_count;
	struct v4l2_srccap_pcm pcm;
	struct v4l2_output_ext_info timing_info;

	memset(&timing_count, 0, sizeof(struct v4l2_srccap_pcm_timing_count));
	memset(&pcm, 0, sizeof(struct v4l2_srccap_pcm));
	memset(&timing_info, 0, sizeof(struct v4l2_output_ext_info));

	/* set mux */
	MS_S16 s16PathId;
	XC_MUX_PATH_INFO PathInfo;
	XC_MUX_PATH_INFO AllPathInfo[MAX_SYNC_DATA_PATH_SUPPORTED];
	MS_U8 u8Count = 0;

	memset(&PathInfo, 0, sizeof(XC_MUX_PATH_INFO));
	memset(AllPathInfo, 0, sizeof(XC_MUX_PATH_INFO)*MAX_SYNC_DATA_PATH_SUPPORTED);

	//step 1. MApi_XC_Mux_GetPathInfo
	u8Count = MApi_XC_Mux_GetPathInfo(AllPathInfo);

	if (u8Count > 0) {
		do {
			u8Count = u8Count - 1;

			if (AllPathInfo[u8Count].dest == OUTPUT_SCALER_MAIN_WINDOW) {
				if (AllPathInfo[u8Count].src == INPUT_SOURCE_HDMI) {
					SRCCAP_MSG_ERROR(
					    "eInputSrc %u was connected to OUTPUT_SCALER_MAIN_WINDOW.\n",
					    INPUT_SOURCE_HDMI);
				} else {
					if (MApi_XC_Mux_DeletePath(AllPathInfo[u8Count].src,
					    OUTPUT_SCALER_MAIN_WINDOW) == -1)
						SRCCAP_MSG_ERROR(
						    " Delete path fail: src = %d, dest = %d\n",
						    AllPathInfo[u8Count].src,
						    OUTPUT_SCALER_MAIN_WINDOW);
					break;
				}
			}
		} while (u8Count > 0);
	}
	PathInfo.Path_Type = PATH_TYPE_SYNCHRONOUS;
	PathInfo.src = INPUT_SOURCE_HDMI;
	PathInfo.dest = OUTPUT_SCALER_MAIN_WINDOW;
	PathInfo.SyncEventHandler = NULL;
	PathInfo.DestOnOff_Event_Handler = NULL;
	PathInfo.path_thread = NULL;
	PathInfo.dest_periodic_handler = NULL;
	//step 2. MApi_XC_Mux_CreatePath
	s16PathId = MApi_XC_Mux_CreatePath(&PathInfo, sizeof(XC_MUX_PATH_INFO));

	//step 3. MApi_XC_Mux_EnablePath
	MApi_XC_Mux_EnablePath(s16PathId);

	//step 4. MApi_XC_SetInputSource
	MApi_XC_SetInputSource(INPUT_SOURCE_HDMI, 0);

	//step 5. MApi_XC_DisableInputSource
	MApi_XC_DisableInputSource(DISABLE, MAIN_WINDOW);

	/* prepare pcm */
	ret = mtk_srccap_drv_detect_timing_pcm_init();
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	timing_count.enSrcType = V4L2_SRCCAP_INPUT_SOURCE_HDMI;
	timing_count.u16TimingStableCounter = 10;
	timing_count.u16TimingNosyncCounter = 10;
	ret = mtk_srccap_drv_set_detect_timing_counter(&timing_count);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	pcm.enSrcType = INPUT_SOURCE_HDMI;
	ret = mtk_srccap_drv_set_pcm(&pcm, srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	en = TRUE;
	ret = mtk_srccap_drv_auto_no_signal(&en, SRCCAP_SET, srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
#endif

	while (video_is_registered(srccap_dev->vdev)) {
		if (((srccap_dev->srccap_info.src <= V4L2_SRCCAP_INPUT_SOURCE_SCART2) &&
			(srccap_dev->srccap_info.src >= V4L2_SRCCAP_INPUT_SOURCE_ATV)) ||
			((srccap_dev->srccap_info.src >= V4L2_SRCCAP_INPUT_SOURCE_CVBS) &&
			(srccap_dev->srccap_info.src <= V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4))) {
			memset(&avd_pcm, 0, sizeof(struct v4l2_srccap_pcm));
			ret = mtk_avd_TimingMonitor(&avd_pcm, srccap_dev);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}
			if (avd_pcm.enPCMStatus != pre_avd_status) {
				memset(&event, 0, sizeof(struct v4l2_event));
				event.type = V4L2_EVENT_SIGNAL_STATUS_CHANGE;
				event.id = 0;
				v4l2_event_queue(srccap_dev->vdev, &event);
				if (avd_pcm.enPCMStatus == V4L2_SRCCAP_PCM_STABLE_SYNC)
					SRCCAP_MSG_INFO("(%s)AVD_STABLE_SYNC\n", __func__);
				else if (avd_pcm.enPCMStatus == V4L2_SRCCAP_PCM_STABLE_NOSYNC)
					SRCCAP_MSG_INFO("(%s)AVD_NOSYNC\n", __func__);
				else if (avd_pcm.enPCMStatus == V4L2_SRCCAP_PCM_UNSTABLE)
					SRCCAP_MSG_INFO("(%s)AVD_UNSTABLE\n", __func__);
				srccap_dev->avd_info.enVdSignalStatus = avd_pcm.enPCMStatus;
			}
			pre_avd_status = avd_pcm.enPCMStatus;

			ret = mtk_avd_TimingVideoInfo(&stTimingInfo, srccap_dev);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}

			srccap_dev->timingdetect_info.h_total = 0;
			srccap_dev->timingdetect_info.v_total = 0;
			srccap_dev->timingdetect_info.h_de = stTimingInfo.de_height;
			srccap_dev->timingdetect_info.v_de = stTimingInfo.de_width;
			srccap_dev->timingdetect_info.v_freqx1000 = stTimingInfo.v_freq;
			srccap_dev->timingdetect_info.interlaced = stTimingInfo.interlance;
		} else {
#ifdef T32_HDMI_SIMULATION
			ret = mtk_srccap_drv_get_pcm(&pcm, srccap_dev);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}

			if (pcm.enPCMStatus != pre_status) {
				v4l2_event_queue(srccap_dev->vdev, &event);
				if (pcm.enPCMStatus == V4L2_SRCCAP_PCM_STABLE_SYNC) {
					timing_info.type = V4L2_EXT_INFO_TYPE_TIMING;
					mtk_srccap_drv_set_output_info(&timing_info, srccap_dev);
					mtk_srccap_drv_get_output_info(&timing_info, srccap_dev);
					srccap_dev->timingdetect_info.h_de =
						timing_info.v4l2Timing.de_width;
					srccap_dev->timingdetect_info.v_de =
						timing_info.v4l2Timing.de_height;
					srccap_dev->timingdetect_info.v_freqx1000 =
						timing_info.v4l2Timing.v_freq;
					srccap_dev->timingdetect_info.interlaced =
						timing_info.v4l2Timing.interlance;
					srccap_dev->timingdetect_info.h_total =
						timing_info.v4l2Timing.h_total;
					srccap_dev->timingdetect_info.v_total =
						timing_info.v4l2Timing.v_total;
				}
			}
			pre_status = pcm.enPCMStatus;
#endif
		}
		usleep_range(30000, 31000); // sleep 30ms

		if (kthread_should_stop()) {
			pr_err("%s has been stopped.\n", __func__);
			return 0;
		}
	}

	return 0;
}

static int mtk_srccap_subscribe_event_signal_status(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev->srccap_info.timing_monitor_task == NULL) {
		srccap_dev->srccap_info.timing_monitor_task = kthread_create(
			timing_monitor_task,
			srccap_dev,
			"timing_monitor_task");
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM))
			return -ENOMEM;
		wake_up_process(srccap_dev->srccap_info.timing_monitor_task);
	}

	return 0;
}

static int mtk_srccap_subscribe_event_ctrl(struct mtk_srccap_dev *srccap_dev,
	const struct v4l2_event_subscription *event_sub)
{
	int iRet = 0;

	iRet |= mtk_hdmirx_subscribe_ctrl_event(srccap_dev, event_sub);

	return iRet;
}

static int mtk_srccap_subscribe_event(
	struct v4l2_fh *fh,
	const struct v4l2_event_subscription *event_sub)
{
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(fh->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	pr_err("[SRCCAP][%s]type:%lu id:%lu\n"
		, __func__, event_sub->type, event_sub->id);

	switch (event_sub->type) {
	case V4L2_EVENT_VSYNC:
		ret = mtk_common_subscribe_event_vsync(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to subscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_CTRL:
		ret = mtk_srccap_subscribe_event_ctrl(srccap_dev, event_sub);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to subscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_SIGNAL_STATUS_CHANGE:
		ret = mtk_srccap_subscribe_event_signal_status(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to subscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_SRCCAP_SUSPEND_EVENT:
		//pr_err("[SRCCAP]V4L2_SRCCAP_SUSPEND_EVENT");
		break;
	case V4L2_SRCCAP_RESUME_EVENT:
		//pr_err("[SRCCAP]V4L2_SRCCAP_RESUME_EVENT");
		break;
	default:
		return -EINVAL;
	}

	return v4l2_event_subscribe(fh, event_sub, SRCCAP_EVENTQ_SIZE, NULL);
}

int mtk_srccap_unsubscribe_event_signal_status(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev->srccap_info.timing_monitor_task != NULL) {
		kthread_stop(srccap_dev->srccap_info.timing_monitor_task);
		srccap_dev->srccap_info.timing_monitor_task = NULL;
	}

	return 0;
}

static int mtk_srccap_unsubscribe_event_ctrl(struct mtk_srccap_dev *srccap_dev,
	const struct v4l2_event_subscription *event_sub)
{
	int iRet = 0;

	iRet |= mtk_hdmirx_unsubscribe_ctrl_event(srccap_dev, event_sub);

	return iRet;
}

static int mtk_srccap_unsubscribe_event(
	struct v4l2_fh *fh,
	const struct v4l2_event_subscription *event_sub)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(fh->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	pr_err("[SRCCAP][%s]type:%lu id:%lu\n"
		, __func__, event_sub->type, event_sub->id);

	switch (event_sub->type) {
	case V4L2_EVENT_VSYNC:
		if (mtk_common_unsubscribe_event_vsync(srccap_dev)) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_CTRL:
		if (mtk_srccap_unsubscribe_event_ctrl(srccap_dev, event_sub)) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_SIGNAL_STATUS_CHANGE:
		if (mtk_srccap_unsubscribe_event_signal_status(srccap_dev)) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_SRCCAP_SUSPEND_EVENT:
		//pr_err("[SRCCAP] Release V4L2_SRCCAP_SUSPEND_EVENT");
		break;
	case V4L2_SRCCAP_RESUME_EVENT:
		//pr_err("[SRCCAP] Release V4L2_SRCCAP_RESUME_EVENT");
		break;
	default:
		return -EINVAL;
	}

	return v4l2_event_unsubscribe(fh, event_sub);
}

static int mtk_srccap_streamon(
	struct file *file,
	void *fh,
	enum v4l2_buf_type buf_type)
{
	int ret = 0;
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_ctx = mtk_srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	pr_err("[SRCCAP][%s]buf_type:%ld\n"
		, __func__, buf_type);

	if (V4L2_TYPE_IS_OUTPUT(buf_type))
		srccap_ctx->m2m_ctx->out_q_ctx.q.streaming = 1;
	else
		srccap_ctx->m2m_ctx->cap_q_ctx.q.streaming = 1;

	ret = v4l2_m2m_streamon(file, srccap_ctx->m2m_ctx, buf_type);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (V4L2_TYPE_IS_OUTPUT(buf_type))
		v4l2_m2m_set_src_buffered(srccap_ctx->m2m_ctx, true);
	else
		v4l2_m2m_set_dst_buffered(srccap_ctx->m2m_ctx, true);

	return mtk_memout_streamon(srccap_dev, buf_type);
}

static int mtk_srccap_streamoff(
	struct file *file,
	void *fh,
	enum v4l2_buf_type buf_type)
{
	int ret = 0;
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_ctx = mtk_srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	pr_err("[SRCCAP][%s]buf_type:%ld\n"
		, __func__, buf_type);

	ret = v4l2_m2m_streamoff(file, srccap_ctx->m2m_ctx, buf_type);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (V4L2_TYPE_IS_OUTPUT(buf_type))
		srccap_ctx->m2m_ctx->out_q_ctx.q.streaming = 0;
	else
		srccap_ctx->m2m_ctx->cap_q_ctx.q.streaming = 0;

	return mtk_memout_streamoff(srccap_dev, buf_type);
}

static const struct v4l2_ioctl_ops mtk_srccap_ioctl_ops = {
	.vidioc_s_input = mtk_srccap_s_input,
	.vidioc_g_input = mtk_srccap_g_input,
	.vidioc_g_edid  = mtk_srccap_g_edid,
	.vidioc_s_edid  = mtk_srccap_s_edid,
	.vidioc_g_std = mtk_scrcap_g_std,
	.vidioc_s_std = mtk_scrcap_s_std,
	.vidioc_g_audio = mtk_srccap_g_audio,
	.vidioc_g_dv_timings = mtk_srccap_g_dv_timings,
	.vidioc_s_fmt_vid_cap = mtk_srccap_s_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap_mplane = mtk_srccap_s_fmt_vid_cap_mplane,
	.vidioc_g_fmt_vid_cap = mtk_srccap_g_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap_mplane = mtk_srccap_g_fmt_vid_cap_mplane,
	.vidioc_reqbufs = mtk_srccap_reqbufs,
	.vidioc_querybuf = mtk_srccap_querybuf,
	.vidioc_qbuf = mtk_srccap_qbuf,
	.vidioc_dqbuf = mtk_srccap_dqbuf,
	.vidioc_subscribe_event = mtk_srccap_subscribe_event,
	.vidioc_unsubscribe_event = mtk_srccap_unsubscribe_event,
	.vidioc_streamon = mtk_srccap_streamon,
	.vidioc_streamoff = mtk_srccap_streamoff,
};

/* ============================================================================================== */
/* ------------------------------------------ vb2_ops ------------------------------------------- */
/* ============================================================================================== */
static int mtk_srccap_queue_setup(
	struct vb2_queue *vq,
	unsigned int *num_buffers,
	unsigned int *num_planes,
	unsigned int sizes[],
	struct device *alloc_devs[])
{
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;
	int i = 0;
	bool contiguous = FALSE;

	srccap_ctx = vb2_get_drv_priv(vq);
	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	srccap_dev = srccap_ctx->srccap_dev;
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* fill num_planes */
	*num_planes = srccap_dev->memout_info.num_bufs + 1; // number of buffer planes + 1(metadata)

	/* fill sizes[] */
	contiguous = srccap_dev->memout_info.num_bufs < srccap_dev->memout_info.num_planes;
	if (contiguous) {
		/* add all plane sizes to sizes[0] for contiguous planes */
		for (i = 0; i < SRCCAP_MAX_PLANE_NUM; ++i) {
			if (srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_SWMODE)
				sizes[0] += srccap_dev->memout_info.frame_pitch[i];
			else /* srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_HWMODE */
				sizes[0] += srccap_dev->memout_info.frame_pitch[i] * (*num_buffers);
		}
	} else {
		for (i = 0; i < (*num_planes - 1); ++i) {
			if (srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_SWMODE)
				sizes[i] = srccap_dev->memout_info.frame_pitch[i];
			else /* srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_HWMODE */
				sizes[i] = srccap_dev->memout_info.frame_pitch[i] * (*num_buffers);
		}
	}
	sizes[*num_planes - 1] = 1; // minimum metadata buf size

	/* fill alloc_devs[] */
	for (i = 0; i < *num_planes; ++i)
		alloc_devs[i] = srccap_dev->dev;

	return 0;
}

static int mtk_srccap_buf_init(struct vb2_buffer *vb)
{
	return 0;
}

static int mtk_srccap_buf_pepare(struct vb2_buffer *vb)
{
	return 0;
}

static void mtk_srccap_buf_finish(struct vb2_buffer *vb)
{
}

static void mtk_srccap_buf_cleanup(struct vb2_buffer *vb)
{
}

static int mtk_srccap_start_streaming(
	struct vb2_queue *q,
	unsigned int count)
{
	return 0;
}

static void mtk_srccap_stop_streaming(struct vb2_queue *q)
{
}

static void mtk_srccap_buf_queue(struct vb2_buffer *vb)
{
	int ret = 0;
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct mtk_srccap_ctx *srccap_ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_srccap_dev *srccap_dev = srccap_ctx->srccap_dev;

	v4l2_m2m_buf_queue(srccap_ctx->m2m_ctx, vbuf);

	ret = mtk_memout_qbuf(srccap_dev, vb);
	if (ret)
		SRCCAP_MSG_RETURN_CHECK(ret);

	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
}

static const struct vb2_ops mtk_srccap_vb2_ops = {
	.queue_setup		= mtk_srccap_queue_setup,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
	.buf_init		= mtk_srccap_buf_init,
	.buf_prepare		= mtk_srccap_buf_pepare,
	.buf_finish		= mtk_srccap_buf_finish,
	.buf_cleanup		= mtk_srccap_buf_cleanup,
	.start_streaming	= mtk_srccap_start_streaming,
	.stop_streaming		= mtk_srccap_stop_streaming,
	.buf_queue		= mtk_srccap_buf_queue,
};

/* ============================================================================================== */
/* ---------------------------------------- vb2_mem_ops ----------------------------------------- */
/* ============================================================================================== */
/*
static void *mtk_srccap_vb2_get_userptr(
	struct device *dev,
	unsigned long vaddr,
	unsigned long size,
	enum dma_data_direction dma_dir)
{
	return NULL;
}

static void mtk_srccap_vb2_put_userptr(void *buf_priv)
{
}

const struct vb2_mem_ops mtk_srccap_vb2_mem_ops = {
	.get_userptr = mtk_srccap_vb2_get_userptr,
	.put_userptr = mtk_srccap_vb2_put_userptr,
};
 */

static int init_queue(
	void *priv,
	struct vb2_queue *src_vq,
	struct vb2_queue *dst_vq)
{
	int ret = 0;
	struct mtk_srccap_ctx *srccap_ctx = priv;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	//src_vq->io_modes = VB2_USERPTR;
	src_vq->io_modes = VB2_DMABUF;
	src_vq->drv_priv = srccap_ctx;
	src_vq->ops = &mtk_srccap_vb2_ops;
	//src_vq->mem_ops = &mtk_srccap_vb2_mem_ops;
	src_vq->mem_ops = &vb2_dma_sg_memops;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &srccap_ctx->srccap_dev->mutex;
	src_vq->dev = srccap_ctx->srccap_dev->v4l2_dev.dev;
	src_vq->allow_zero_bytesused = 1;
	src_vq->min_buffers_needed = 1;

	ret = vb2_queue_init(src_vq);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	//dst_vq->io_modes = VB2_USERPTR;
	dst_vq->io_modes = VB2_DMABUF;
	dst_vq->drv_priv = srccap_ctx;
	dst_vq->ops = &mtk_srccap_vb2_ops;
	//dst_vq->mem_ops = &mtk_srccap_vb2_mem_ops;
	dst_vq->mem_ops = &vb2_dma_sg_memops;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &srccap_ctx->srccap_dev->mutex;
	dst_vq->dev = srccap_ctx->srccap_dev->v4l2_dev.dev;
	dst_vq->allow_zero_bytesused = 1;
	dst_vq->min_buffers_needed = 1;

	ret = vb2_queue_init(dst_vq);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

/* ============================================================================================== */
/* ------------------------------------ v4l2_file_operations ------------------------------------ */
/* ============================================================================================== */
static int mtk_srccap_open(struct file *file)
{
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = video_drvdata(file);
	struct video_device *vdev = video_devdata(file);
	struct mtk_srccap_ctx *srccap_ctx = NULL;

	SRCCAP_MSG_INFO("%s\n", __func__);

	srccap_ctx = kzalloc(sizeof(struct mtk_srccap_ctx), GFP_KERNEL);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOMEM;
	}

	if (mutex_lock_interruptible(&srccap_dev->mutex)) {
		kfree(srccap_ctx);
		return -ERESTARTSYS;
	}

	/* initialize srccap context struct */
	v4l2_fh_init(&srccap_ctx->fh, vdev);
	file->private_data = &srccap_ctx->fh;
	v4l2_fh_add(&srccap_ctx->fh);
	srccap_ctx->srccap_dev = srccap_dev;
	srccap_ctx->m2m_ctx = v4l2_m2m_ctx_init(srccap_dev->mdev, srccap_ctx, init_queue);

	// init function
	mtk_hdmirx_init(srccap_dev);

	if (!gSrccapInitfunc) {
		MApi_XC_Mux_Init(NULL);
		MApi_XC_ModeParse_Init();
	}
	mtk_srccap_avd_init(srccap_dev);
	mtk_srccap_timingdetect_drv_init(srccap_dev);

	mutex_unlock(&srccap_dev->mutex);
	return 0;
}

static int mtk_srccap_release(struct file *file)
{
	struct mtk_srccap_ctx *srccap_ctx = mtk_srccap_fh_to_ctx(file->private_data);
	struct mtk_srccap_dev *srccap = video_drvdata(file);

	SRCCAP_MSG_INFO("%s\n", __func__);

	mutex_lock(&srccap->mutex);

	mtk_srccap_timingdetect_drv_exit(srccap_ctx->srccap_dev);
	mtk_hdmirx_release(srccap);
	v4l2_m2m_ctx_release(srccap_ctx->m2m_ctx);
	v4l2_fh_del(&srccap_ctx->fh);
	v4l2_fh_exit(&srccap_ctx->fh);

	kfree(srccap_ctx);
	mutex_unlock(&srccap->mutex);

	return 0;
}

static const struct v4l2_file_operations mtk_srccap_fops = {
	.owner			= THIS_MODULE,
	.open			= mtk_srccap_open,
	.release		= mtk_srccap_release,
	.unlocked_ioctl		= video_ioctl2,
};

static u16 dev_index[SRCCAP_DEV_NUM] = {0, 1};

static const struct of_device_id mtk_srccap_match[] = {
	{
		.compatible = "mediatek,srccap0",
		.data = &dev_index[0],
	},
	{
		.compatible = "mediatek,srccap1",
		.data = &dev_index[1],
	},
	{},
};

/* ============================================================================================== */
/* -------------------------------------- device_attribute -------------------------------------- */
/* ============================================================================================== */
static void sysfs_print(
	char *u8Buf,
	int *intStrSize,
	u16 u16Size,
	u32 u32Count)
{
	if ((u8Buf == NULL) || (intStrSize == NULL)) {
		SRCCAP_MSG_ERROR("Failed to print due to NULL inputs!\r\n");
		return;
	}

	if (*intStrSize > u16Size) {
		SRCCAP_MSG_ERROR("Failed to store into string buffer.");
		SRCCAP_MSG_ERROR("string size:%u, buffer size:%u\r\n",
			*intStrSize, u16Size);
	} else {
		*intStrSize += snprintf(u8Buf + *intStrSize, u16Size
			- *intStrSize, "%u\n", u32Count);
	}
}

static ssize_t sysfs_show_ipdma_count_0(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query IP-DMA number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[0].ipdma_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_ipdma_count_0(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_hdmi_count_0(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query HDMI number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[0].hdmi_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_hdmi_count_0(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_cvbs_count_0(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query CVBS number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[0].cvbs_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_cvbs_count_0(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_svideo_count_0(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query S-Video number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[0].svideo_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_svideo_count_0(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_ypbpr_count_0(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query YPbPr number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[0].ypbpr_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_ypbpr_count_0(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_vga_count_0(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query VGA number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[0].vga_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_vga_count_0(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_atv_count_0(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query ATV number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[0].atv_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_atv_count_0(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_ipdma_count_1(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query IP-DMA number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[1].ipdma_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_ipdma_count_1(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_hdmi_count_1(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query HDMI number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[1].hdmi_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_hdmi_count_1(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_cvbs_count_1(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query CVBS number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[1].cvbs_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_cvbs_count_1(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_svideo_count_1(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query S-Video number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[1].svideo_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_svideo_count_1(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_ypbpr_count_1(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query YPbPr number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[1].ypbpr_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_ypbpr_count_1(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_vga_count_1(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query VGA number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[1].vga_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_vga_count_1(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t sysfs_show_atv_count_1(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 u16Size = 65535;
	int intStrSize = 0;

	//query ATV number
	sysfs_print(buf, &intStrSize, u16Size,
		gstSrccapCapability[1].atv_cnt);

	return intStrSize;
}

static ssize_t sysfs_store_atv_count_1(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

// S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH = 0644
static struct device_attribute mtk_srccap_attr0[] = {
	__ATTR(mtk_srccap_capability_ipdma_count, 0644,
		sysfs_show_ipdma_count_0, sysfs_store_ipdma_count_0),
	__ATTR(mtk_srccap_capability_hdmi_count, 0644,
		sysfs_show_hdmi_count_0, sysfs_store_hdmi_count_0),
	__ATTR(mtk_srccap_capability_cvbs_count, 0644,
		sysfs_show_cvbs_count_0, sysfs_store_cvbs_count_0),
	__ATTR(mtk_srccap_capability_svideo_count, 0644,
		sysfs_show_svideo_count_0, sysfs_store_svideo_count_0),
	__ATTR(mtk_srccap_capability_ypbpr_count, 0644,
		sysfs_show_ypbpr_count_0, sysfs_store_ypbpr_count_0),
	__ATTR(mtk_srccap_capability_vga_count, 0644,
		sysfs_show_vga_count_0, sysfs_store_vga_count_0),
	__ATTR(mtk_srccap_capability_atv_count, 0644,
		sysfs_show_atv_count_0, sysfs_store_atv_count_0),
};

static struct device_attribute mtk_srccap_attr1[] = {
	__ATTR(mtk_srccap_capability_ipdma_count, 0644,
			sysfs_show_ipdma_count_1, sysfs_store_ipdma_count_1),
	__ATTR(mtk_srccap_capability_hdmi_count, 0644,
			sysfs_show_hdmi_count_1, sysfs_store_hdmi_count_1),
	__ATTR(mtk_srccap_capability_cvbs_count, 0644,
			sysfs_show_cvbs_count_1, sysfs_store_cvbs_count_1),
	__ATTR(mtk_srccap_capability_svideo_count, 0644,
			sysfs_show_svideo_count_1, sysfs_store_svideo_count_1),
	__ATTR(mtk_srccap_capability_ypbpr_count, 0644,
			sysfs_show_ypbpr_count_1, sysfs_store_ypbpr_count_1),
	__ATTR(mtk_srccap_capability_vga_count, 0644,
			sysfs_show_vga_count_1, sysfs_store_vga_count_1),
	__ATTR(mtk_srccap_capability_atv_count, 0644,
			sysfs_show_atv_count_1, sysfs_store_atv_count_1),
};

/* ============================================================================================== */
/* ---------------------------------------- v4l2_m2m_ops ---------------------------------------- */
/* ============================================================================================== */
static void mtk_srccap_m2m_device_run(void *priv)
{
}

static void mtk_srccap_m2m_job_abort(void *priv)
{
	struct mtk_srccap_ctx *srccap_ctx = priv;

	v4l2_m2m_job_finish(srccap_ctx->srccap_dev->mdev, srccap_ctx->m2m_ctx);
}

static struct v4l2_m2m_ops mtk_srccap_m2m_ops = {
	.device_run = mtk_srccap_m2m_device_run,
	.job_abort = mtk_srccap_m2m_job_abort,
};

static irqreturn_t mtk_scrcap_hdmi_hdcp_irq(int irq, void *priv)
{
	mtk_hdmirx_hdcp_isr(irq, priv);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_srccap_hdmi_scdc_irq(int irq, void *priv)
{
	return IRQ_HANDLED;
}

static void srccap_get_capability(struct mtk_srccap_dev *srccapdev)
{
	if (srccapdev != NULL) {
		if (srccapdev->dev_id == 0) {
			memset(&gstSrccapCapability[0], 0,
				sizeof(struct srccap_capability));
			gstSrccapCapability[0].ipdma_cnt
				= srccapdev->srccap_info.srccap_cap.ipdma_cnt;
			gstSrccapCapability[0].hdmi_cnt
				= srccapdev->srccap_info.srccap_cap.hdmi_cnt;
			gstSrccapCapability[0].cvbs_cnt
				= srccapdev->srccap_info.srccap_cap.cvbs_cnt;
			gstSrccapCapability[0].svideo_cnt
				= srccapdev->srccap_info.srccap_cap.svideo_cnt;
			gstSrccapCapability[0].ypbpr_cnt
				= srccapdev->srccap_info.srccap_cap.ypbpr_cnt;
			gstSrccapCapability[0].vga_cnt
				= srccapdev->srccap_info.srccap_cap.vga_cnt;
			SRCCAP_MSG_INFO("device 0 capability received\n");
		} else if (srccapdev->dev_id == 1) {
			memset(&gstSrccapCapability[1], 0,
				sizeof(struct srccap_capability));
			gstSrccapCapability[1].ipdma_cnt
				= srccapdev->srccap_info.srccap_cap.ipdma_cnt;
			gstSrccapCapability[1].hdmi_cnt
				= srccapdev->srccap_info.srccap_cap.hdmi_cnt;
			gstSrccapCapability[1].cvbs_cnt
				= srccapdev->srccap_info.srccap_cap.cvbs_cnt;
			gstSrccapCapability[1].svideo_cnt
				= srccapdev->srccap_info.srccap_cap.svideo_cnt;
			gstSrccapCapability[1].ypbpr_cnt
				= srccapdev->srccap_info.srccap_cap.ypbpr_cnt;
			gstSrccapCapability[1].vga_cnt
				= srccapdev->srccap_info.srccap_cap.vga_cnt;
			SRCCAP_MSG_INFO("device 1 capability received\n");
		} else
			SRCCAP_MSG_ERROR("device id not found\n");
	}
}

static void mtk_srccap_iounmap(struct mtk_srccap_dev *srccapdev)
{
	int i = 0;

	if (srccapdev->avd_info.pIoremap != NULL) {

		for (i = 0; i < srccapdev->avd_info.regCount; ++i)
			iounmap(srccapdev->avd_info.pIoremap[i]);

		kfree(srccapdev->avd_info.pIoremap);
		srccapdev->avd_info.pIoremap = NULL;
	}
}

static int mtk_srccap_ioremap(struct mtk_srccap_dev *pqdev,
		struct device *pdev)
{
	int ret = 0, regSize = 0, i = 0, offset[2] = {0, 0};
	int j = 0;
	struct device *property_dev = pdev;
	u32 *reg = NULL;

	pqdev->avd_info.reg_info_offset = 2;
	pqdev->avd_info.reg_pa_idx = 1;
	pqdev->avd_info.reg_pa_size_idx = 3;

	ret = of_property_read_u32_array(property_dev->of_node,
				"vd_reg_count", &pqdev->avd_info.regCount, 1);
	if (ret) {
		SRCCAP_MSG_ERROR("%s Failed to get reg_count\r\n", __func__);
		return -EINVAL;
	}

	regSize = (pqdev->avd_info.regCount << pqdev->avd_info.reg_info_offset);
	reg = kcalloc(regSize, sizeof(u32), GFP_KERNEL);
	if (reg == NULL) {
		SRCCAP_MSG_ERROR("%s Failed to kcalloc register\r\n", __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(property_dev->of_node,
				"vd_reg_ioremap", reg, regSize);
	if (ret) {
		SRCCAP_MSG_ERROR("%s Failed to get register\r\n", __func__);
		return -EINVAL;
	}

	pqdev->avd_info.pIoremap = kcalloc(pqdev->avd_info.regCount,
				sizeof(u64), GFP_KERNEL);
	if (reg == NULL) {
		SRCCAP_MSG_ERROR("%s Failed to kcalloc g_ioremap\r\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < pqdev->avd_info.regCount; ++i) {
		offset[0] = (i << pqdev->avd_info.reg_info_offset) +
			pqdev->avd_info.reg_pa_idx;
		offset[1] = (i << pqdev->avd_info.reg_info_offset) +
			pqdev->avd_info.reg_pa_size_idx;

		pqdev->avd_info.pIoremap[i] = ioremap(reg[offset[0]], reg[offset[1]]);

		SRCCAP_MSG_INFO("[AVD]0x%lx = 0x%x(%x)\n",
			pqdev->avd_info.pIoremap[i], reg[offset[0]], reg[offset[1]]);

		drv_hwreg_common_setRIUaddr(reg[offset[0]], reg[offset[1]],
			pqdev->avd_info.pIoremap[i]);
	}
	kfree(reg);
	return ret;
}

static int mtk_srccap_read_dts_u32(
	struct device_node *node,
	char *s,
	u32 *val)
{
	int ret = 0;

	if (node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (s == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (val == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	ret = of_property_read_u32(node, s, val);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	SRCCAP_MSG_INFO("%s = 0x%x(%u)\n", s, *val, *val);
	return ret;
}

static int mtk_srccap_parse_dts_reg(struct device *property_dev)
{
	int ret = 0;
	stHDMIRx_Bank stHDMIRxBank;
	struct reg_srccap_riu_addr srccap_riu_addr;
	struct device_node *reg_node;
	struct device_node *bank_node;

	memset(&stHDMIRxBank, 0, sizeof(stHDMIRx_Bank));
	memset(&srccap_riu_addr, 0, sizeof(struct reg_srccap_riu_addr));

	reg_node = of_find_node_by_name(property_dev->of_node, "regbases");
	if (reg_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	ret |= mtk_srccap_read_dts_u32(reg_node, "regbase1",
		&stHDMIRxBank.regbase1);
	ret |= mtk_srccap_read_dts_u32(reg_node, "regbase2",
		&stHDMIRxBank.regbase2);

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	//hdmirx_init_regbase(srccap_dev);

	bank_node = of_find_node_by_name(property_dev->of_node, "banks");
	if (bank_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdmirx_dtop",
		&stHDMIRxBank.bank_hdmirx_dtop);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdmirx_powersave",
		&stHDMIRxBank.bank_hdmirx_powersave);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdmirx_hdcp",
		&stHDMIRxBank.bank_hdmirx_hdcp);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdmirx_dtop_pkt",
		&stHDMIRxBank.bank_hdmirx_dtop_pkt);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdmirx_ve",
		&stHDMIRxBank.bank_hdmirx_ve);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdmirx_hdmi",
		&stHDMIRxBank.bank_hdmirx_hdmi);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdmirx_hdmi2",
		&stHDMIRxBank.bank_hdmirx_hdmi2);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdmirx_hdmi3",
		&stHDMIRxBank.bank_hdmirx_hdmi3);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdmirx_em",
		&stHDMIRxBank.bank_hdmirx_em);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_combo_gp_top",
		&stHDMIRxBank.bank_combo_gp_top);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_chip_gpio_mux",
		&stHDMIRxBank.bank_chip_gpio_mux);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_phy2p1_0",
		&stHDMIRxBank.bank_phy2p1_0);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_phy2p1_1",
		&stHDMIRxBank.bank_phy2p1_1);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_phy2p1_2",
		&stHDMIRxBank.bank_phy2p1_2);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_phy2p1_3",
		&stHDMIRxBank.bank_phy2p1_3);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_phy2p1_4",
		&stHDMIRxBank.bank_phy2p1_4);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_phy_train",
		&stHDMIRxBank.bank_phy_train);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_clkgen0",
		&stHDMIRxBank.bank_clkgen0);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_clkgen1",
		&stHDMIRxBank.bank_clkgen1);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_ddc",
		&stHDMIRxBank.bank_ddc);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_scdc",
		&stHDMIRxBank.bank_scdc);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_phy21_pm",
		&stHDMIRxBank.bank_phy21_pm);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_hdcpkey",
		&stHDMIRxBank.bank_hdcpkey);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_pm_slp",
		&stHDMIRxBank.bank_pm_slp);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_sc",
		&stHDMIRxBank.bank_sc);

#ifdef T32_HDMI_SIMULATION
	ret |= mtk_srccap_read_dts_u32(reg_node, "regbase1",
		&srccap_riu_addr.regbase);
	ret |= mtk_srccap_read_dts_u32(reg_node, "regbase2",
		&srccap_riu_addr.regbase_ext);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_sc",
		&srccap_riu_addr.bank_sc);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_sc_ext",
		&srccap_riu_addr.bank_sc_ext);
	ret |= mtk_srccap_read_dts_u32(bank_node, "bank_mux",
		&srccap_riu_addr.bank_mux);
	drv_hwreg_srccap_memout_riu_init(&srccap_riu_addr);
#endif

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	//hdmirx_init_bank(srccap_dev);

	if (!Drv_HDMIRx_HW_Init(&stHDMIRxBank)) {
		pr_err("Init HDMIRx RegBase & Bank failed!!\r\n");
		ret = -ENOENT;
	}

	return ret;
}

static int mtk_srccap_parse_dts_buf(
	struct mtk_srccap_dev *srccap_dev,
	struct device *property_dev)
{
	int ret = 0;
	struct device_node *buf_node;
	struct device_node *mmap_node;
	struct of_mmap_info_data of_mmap_info = {0};

	buf_node = of_find_node_by_name(
		property_dev->of_node,
		"MI_EXTIN_VD_COMB_BUF");
	if (buf_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	ret |= mtk_srccap_read_dts_u32(buf_node, "MI_EXTIN_VD_COMB_BUF_ADR",
		&srccap_dev->avd_info.Comb3dBufAddr);
	ret |= mtk_srccap_read_dts_u32(buf_node, "MI_EXTIN_VD_COMB_BUF_LEN",
		&srccap_dev->avd_info.Comb3dBufSize);
	ret |= mtk_srccap_read_dts_u32(buf_node, "MI_EXTIN_VD_COMB_BUF_CMA_HID",
		&srccap_dev->avd_info.Comb3dBufHeapId);
	ret |= mtk_srccap_read_dts_u32(buf_node, "MI_EXTIN_VD_COMB_ALIMENT",
		&srccap_dev->avd_info.u32Comb3dAliment);
	ret |= mtk_srccap_read_dts_u32(buf_node, "MI_EXTIN_VD_COMB_SIZE",
		&srccap_dev->avd_info.u32Comb3dSize);

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}
	mmap_node = of_find_compatible_node(NULL, NULL, "mediatek,dtv-mmap-vd-comb-buf");
	if (mmap_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	SRCCAP_MSG_INFO("[START][MMAP]Get VD comb buffer. node : %s.\n", mmap_node->name);
	if (of_mtk_get_reserved_memory_info_by_idx(
		mmap_node, 0, &of_mmap_info) == 0) {
		SRCCAP_MSG_INFO("Show vd mmap info\n");
		SRCCAP_MSG_INFO("layer is %u\n",
			of_mmap_info.layer);
		SRCCAP_MSG_INFO("miu is %u\n",
			of_mmap_info.miu);
		SRCCAP_MSG_INFO("is_cache is %u\n",
			of_mmap_info.is_cache);
		SRCCAP_MSG_INFO("cma_id is %u\n",
			of_mmap_info.cma_id);
		SRCCAP_MSG_INFO("start_bus_address is 0x%llX\n",
			of_mmap_info.start_bus_address);
		SRCCAP_MSG_INFO("buffer_size is 0x%llX\n",
			of_mmap_info.buffer_size);
		//srccap_dev->avd_info.Comb3dBufAddr = of_mmap_info.start_bus_address;
		//srccap_dev->avd_info.Comb3dBufSize = of_mmap_info.buffer_size;
		//srccap_dev->avd_info.Comb3dBufHeapId = of_mmap_info.cma_id;
	} else
		SRCCAP_MSG_INFO("Get vd mmap info fail\n");

	SRCCAP_MSG_INFO("[END][MMAP]Get VD comb buffer.\n");

	if (mtk_avd_InitHWinfo(srccap_dev) < 0) {
		pr_err("Init AVD RegBase & Bank failed!!\r\n");
		ret = -ENOENT;
	}

	return ret;
}

static int mtk_srccap_parse_dts_cap(
	struct mtk_srccap_dev *srccap_dev,
	struct device *property_dev)
{
	int ret = 0;
	u32 val = 0;
	struct device_node *cap_node;

	cap_node = of_find_node_by_name(property_dev->of_node, "capability");
	if (cap_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	/* external use */
	ret |= mtk_srccap_read_dts_u32(cap_node, "ipdma_count", &val);
	srccap_dev->srccap_info.srccap_cap.ipdma_cnt = (u8)val;
	ret |= mtk_srccap_read_dts_u32(cap_node, "hdmi_count", &val);
	srccap_dev->srccap_info.srccap_cap.hdmi_cnt = (u8)val;
	ret |= mtk_srccap_read_dts_u32(cap_node, "cvbs_count", &val);
	srccap_dev->srccap_info.srccap_cap.cvbs_cnt = (u8)val;
	ret |= mtk_srccap_read_dts_u32(cap_node, "svideo_count", &val);
	srccap_dev->srccap_info.srccap_cap.svideo_cnt = (u8)val;
	ret |= mtk_srccap_read_dts_u32(cap_node, "ypbpr_count", &val);
	srccap_dev->srccap_info.srccap_cap.ypbpr_cnt = (u8)val;
	ret |= mtk_srccap_read_dts_u32(cap_node, "vga_count", &val);
	srccap_dev->srccap_info.srccap_cap.vga_cnt = (u8)val;
	ret |= mtk_srccap_read_dts_u32(cap_node, "atv_count", &val);
	srccap_dev->srccap_info.srccap_cap.atv_cnt = (u8)val;

	/* internal use */
	ret |= mtk_srccap_read_dts_u32(cap_node, "bit_per_word",
		&srccap_dev->srccap_info.srccap_cap.bpw);

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	srccap_get_capability(srccap_dev);

	return ret;
}

static int mtk_srccap_parse_dts_portmap(
	struct mtk_srccap_dev *srccap_dev,
	struct device *property_dev)
{
	int ret = 0;
	int i = 0;
	u32 input_num = 0;
	char input[10] = {0};
	u32 val = 0;
	enum v4l2_srccap_input_source src = V4L2_SRCCAP_INPUT_SOURCE_NONE;
	u32 max_input_num = V4L2_SRCCAP_INPUT_SOURCE_NUM;
	struct device_node *map_node;
	struct device_node *input_node;

	map_node = of_find_node_by_name(property_dev->of_node, "port_map");
	if (map_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	ret |= mtk_srccap_read_dts_u32(map_node, "input_num", &input_num);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	if (input_num > max_input_num)
		return -E2BIG;

	for (i = 0; i < input_num; i++) {
		memset(input, 0, sizeof(input));
		sprintf(input, "input%d", i);
		/* find the i-th input and store its data and sync ports in */
		/* the mapping array and indicate the specific input's      */
		/* position in the array with source type                   */
		input_node = of_find_node_by_name(map_node, input);
		if (input_node == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			return -ENOENT;
		}

		ret |= mtk_srccap_read_dts_u32(input_node, "source", &val);
		src = val;
		ret |= mtk_srccap_read_dts_u32(input_node, "data_port", &val);
		srccap_dev->srccap_info.map[src].data_port = val;
		ret |= mtk_srccap_read_dts_u32(input_node, "sync_port", &val);
		srccap_dev->srccap_info.map[src].sync_port = val;
	}

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	return ret;
}

static int mtk_srccap_parse_dts(
	struct mtk_srccap_dev *srccap_dev,
	struct platform_device *pdev)
{
	int ret = 0;
	struct device *property_dev = &pdev->dev;

	srccap_dev->irq_hdcp = platform_get_irq(pdev, 0);
	if (srccap_dev->irq_hdcp < 0)
		SRCCAP_MSG_ERROR("Failed to get hdcp irq\r\n");
	else {
		pr_info("hdcp_irq[0] : %x\r\n", srccap_dev->irq_hdcp);

		ret = devm_request_irq(&pdev->dev, srccap_dev->irq_hdcp,
			mtk_scrcap_hdmi_hdcp_irq, IRQF_ONESHOT, pdev->name, srccap_dev);
		if (ret) {
			SRCCAP_MSG_ERROR("Failed to request hdcp irq\r\n");
			ret = -EINVAL;
			return ret;
		}
	}

	srccap_dev->irq_scdc = platform_get_irq(pdev, 1);
	if (srccap_dev->irq_scdc < 0)
		SRCCAP_MSG_ERROR("Failed to get scdc irq\r\n");
	else {
		pr_info("scdc_irq[1] : %x\r\n", srccap_dev->irq_scdc);

		ret = devm_request_irq(&pdev->dev, srccap_dev->irq_scdc,
			mtk_srccap_hdmi_scdc_irq, IRQF_ONESHOT, pdev->name, srccap_dev);
		if (ret) {
			SRCCAP_MSG_ERROR("Failed to request scdc irq\r\n");
			ret = -EINVAL;
			return ret;
		}
	}

	/* parse register base */
	ret |= mtk_srccap_parse_dts_reg(property_dev);

	/* parse buffer */
	ret |= mtk_srccap_parse_dts_buf(srccap_dev, property_dev);

	/* parse capability */
	ret |= mtk_srccap_parse_dts_cap(srccap_dev, property_dev);

	/* parse port map */
	ret |= mtk_srccap_parse_dts_portmap(srccap_dev, property_dev);

	/* prase ioremao */
	ret |= mtk_srccap_ioremap(srccap_dev, property_dev);
	return ret;
}

static int mtk_srccap_create_sysfs(struct mtk_srccap_dev *srccap_dev)
{
	int i = 0;

	if (srccap_dev->dev_id == 0) {
		for (i = 0; i <
		(sizeof(mtk_srccap_attr0) / sizeof(struct device_attribute));
		i++) {
			if (device_create_file(srccap_dev->dev,
			&mtk_srccap_attr0[i]) != 0) {
				SRCCAP_MSG_INFO("sys file %d creation\n"
							"failed.\n", i);
			}
		}
	} else if (srccap_dev->dev_id == 1) {
		for (i = 0; i <
		(sizeof(mtk_srccap_attr1) / sizeof(struct device_attribute));
		i++) {
			if (device_create_file(srccap_dev->dev,
				&mtk_srccap_attr1[i]) != 0) {
				SRCCAP_MSG_INFO("sys file %d creation\n"
							"failed.\n", i);
			}
		}
	} else
		SRCCAP_MSG_ERROR("device id not found\n");

	return 0;
}

static void mtk_srccap_clear_info(struct mtk_srccap_dev *srccap_dev)
{
	memset(&srccap_dev->srccap_info, 0,
		sizeof(struct srccap_info));
#if NO_DEFINE
	memset(&srccap_dev->common_info, 0,
		sizeof(struct srccap_common_info));
	memset(&srccap_dev->hdmirx_info, 0,
		sizeof(struct srccap_hdmirx_info));
	memset(&srccap_dev->adc_info, 0,
		sizeof(struct srccap_adc_info));
#endif
	memset(&srccap_dev->avd_info, 0,
		sizeof(struct srccap_avd_info));
#if NO_DEFINE
	memset(&srccap_dev->mux_info, 0,
		sizeof(struct srccap_mux_info));
	memset(&srccap_dev->offlinedetect_info, 0,
		sizeof(struct srccap_offlinedetect_info));
	memset(&srccap_dev->timingdetect_info, 0,
		sizeof(struct srccap_timingdetect_info));
	memset(&srccap_dev->dscl_info, 0,
		sizeof(struct srccap_dscl_info));
#endif
	memset(&srccap_dev->memout_info, 0,
		sizeof(struct srccap_memout_info));
}

static int mtk_srccap_init(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

#ifdef CONFIG_OPTEE
	ret = mtk_memout_svp_init(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
#endif

	return ret;
}

static int srccap_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;
	struct v4l2_device *v4l2_dev = NULL;
	const struct of_device_id *of_id = NULL;

	pr_emerg("Probe starts\n");
	srccap_dev = devm_kzalloc(&pdev->dev,
		sizeof(struct mtk_srccap_dev), GFP_KERNEL);
	if (!srccap_dev)
		return -ENOMEM;

	mtk_srccap_clear_info(srccap_dev);

	spin_lock_init(&srccap_dev->lock);
	mutex_init(&srccap_dev->mutex);

	/* save device */
	srccap_dev->dev = &pdev->dev;

	/* find device ID */
	of_id = of_match_node(mtk_srccap_match, pdev->dev.of_node);
	if (!of_id) {
		ret = -ENODEV;
		goto UNREGISTER_VIDEO_DEVICE;
	}
	srccap_dev->dev_id = *(u16 *)of_id->data;

	/* parse srccap device tree */
	ret = mtk_srccap_parse_dts(srccap_dev, pdev);
	if (ret) {
		dev_err(&pdev->dev,
			"\033[1;31mfailed to parse srccap dts\033[0m\n");
		goto EXIT;
	}

	/* write sysfs */
	mtk_srccap_create_sysfs(srccap_dev);

	/* register v4l2 device */
	v4l2_dev = &srccap_dev->v4l2_dev;
	ret = v4l2_device_register(&pdev->dev, v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev,
			"\033[1;31mfailed to register v4l2 device\033[0m\n");
		ret = -EINVAL;
		goto  EXIT;
	}

	/* initialize m2m device */
	srccap_dev->mdev = v4l2_m2m_init(&mtk_srccap_m2m_ops);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to init m2m device\n");
		goto UNREGISTER_V4L2_DEVICE;
	}

	v4l2_ctrl_handler_init(&srccap_dev->srccap_ctrl_handler, 0);
	v4l2_dev->ctrl_handler = &srccap_dev->srccap_ctrl_handler;

	/* register srccap sub-devices */
	ret = mtk_srccap_register_adc_subdev(
		v4l2_dev,
		&srccap_dev->subdev_adc,
		&srccap_dev->adc_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register adc subdev\033[0m\n");
		goto RELEASE_M2M_DEVICE;
	}
	ret = mtk_srccap_register_avd_subdev(
		v4l2_dev,
		&srccap_dev->subdev_avd,
		&srccap_dev->avd_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register avd subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_ADC;
	}
	ret = mtk_srccap_register_common_subdev(
		v4l2_dev,
		&srccap_dev->subdev_common,
		&srccap_dev->common_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register common subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_AVD;
	}
	ret = mtk_srccap_register_dscl_subdev(
		v4l2_dev,
		&srccap_dev->subdev_dscl,
		&srccap_dev->dscl_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register dscl subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_COMMON;
	}
	ret = mtk_srccap_register_hdmirx_subdev(
		v4l2_dev,
		&srccap_dev->subdev_hdmirx,
		&srccap_dev->hdmirx_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register hdmirx subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_DSCL;
	}
	ret = mtk_srccap_register_memout_subdev(
		v4l2_dev,
		&srccap_dev->subdev_memout,
		&srccap_dev->memout_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register memout subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_HDMIRX;
	}
	ret = mtk_srccap_register_mux_subdev(
		v4l2_dev,
		&srccap_dev->subdev_mux,
		&srccap_dev->mux_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register mux subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_MEMOUT;
	}
	ret = mtk_srccap_register_offlinedetect_subdev(
		v4l2_dev,
		&srccap_dev->subdev_offlinedetect,
		&srccap_dev->offlinedetect_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register offlinedetect subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_MUX;
	}
	ret = mtk_srccap_register_timingdetect_subdev(
		v4l2_dev,
		&srccap_dev->subdev_timingdetect,
		&srccap_dev->timingdetect_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register timingdetect subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_OFFLINEDETECT;
	}
	ret = mtk_srccap_register_vbi_subdev(
		v4l2_dev,
		&srccap_dev->subdev_vbi,
		&srccap_dev->vbi_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register vbi sub dev\n");
		goto UNREGISTER_SUBDEVICE_TIMINGDETECT;
	}

	/* allocate video device */
	srccap_dev->vdev = video_device_alloc();
	if (!srccap_dev->vdev) {
		ret = -ENOMEM;
		goto UNREGISTER_SUBDEVICE_VBI;
	}

	srccap_dev->vdev->fops = &mtk_srccap_fops;
	srccap_dev->vdev->device_caps =
		V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	srccap_dev->vdev->v4l2_dev = v4l2_dev;
	srccap_dev->vdev->vfl_dir = VFL_DIR_M2M;
	srccap_dev->vdev->minor = -1;
	srccap_dev->vdev->tvnorms = V4L2_STD_ALL;
	srccap_dev->vdev->release = video_device_release;
	srccap_dev->vdev->ioctl_ops = &mtk_srccap_ioctl_ops;
	srccap_dev->vdev->lock = &srccap_dev->mutex;

	/* register video device */
	ret = video_register_device(srccap_dev->vdev,
		VFL_TYPE_GRABBER, SRCCAP_VIDEO_DEV_NUM_BASE + srccap_dev->dev_id);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register video device\033[0m\n");
		goto RELEASE_VIDEO_DEVICE;
	}
	video_set_drvdata(srccap_dev->vdev, srccap_dev);
	platform_set_drvdata(pdev, srccap_dev);

	v4l2_info(v4l2_dev, "device registered as /dev/video%d\n",
		srccap_dev->vdev->num);

	pm_runtime_enable(&pdev->dev);
	of_id = of_match_node(mtk_srccap_match, pdev->dev.of_node);
	if (!of_id) {
		ret = -ENODEV;
		goto UNREGISTER_VIDEO_DEVICE;
	}

	srccap_dev->dev_id = *(u16 *)of_id->data;
	snprintf(srccap_dev->vdev->name, sizeof(srccap_dev->vdev->name),
		"%s-vdev", MTK_SRCCAP_NAME);

	ret = mtk_srccap_init(srccap_dev);
	if (ret) {
		pr_err("\033[1;31mfailed to initialize srccap\033[0m\n");
		goto UNREGISTER_VIDEO_DEVICE;
	}

	dev_dbg(&pdev->dev, "successfully probed\n");
	pr_emerg("successfully probed\n");

	return 0;

UNREGISTER_VIDEO_DEVICE:
	video_unregister_device(srccap_dev->vdev);
RELEASE_VIDEO_DEVICE:
	video_device_release(srccap_dev->vdev);
UNREGISTER_SUBDEVICE_VBI:
	mtk_srccap_unregister_vbi_subdev(&srccap_dev->subdev_vbi);
UNREGISTER_SUBDEVICE_TIMINGDETECT:
	mtk_srccap_unregister_timingdetect_subdev(&srccap_dev->subdev_timingdetect);
UNREGISTER_SUBDEVICE_OFFLINEDETECT:
	mtk_srccap_unregister_offlinedetect_subdev(&srccap_dev->subdev_offlinedetect);
UNREGISTER_SUBDEVICE_MUX:
	mtk_srccap_unregister_mux_subdev(&srccap_dev->subdev_mux);
UNREGISTER_SUBDEVICE_MEMOUT:
	mtk_srccap_unregister_memout_subdev(&srccap_dev->subdev_memout);
UNREGISTER_SUBDEVICE_HDMIRX:
	mtk_srccap_unregister_hdmirx_subdev(&srccap_dev->subdev_hdmirx);
UNREGISTER_SUBDEVICE_DSCL:
	mtk_srccap_unregister_dscl_subdev(&srccap_dev->subdev_dscl);
UNREGISTER_SUBDEVICE_COMMON:
	mtk_srccap_unregister_common_subdev(&srccap_dev->subdev_common);
UNREGISTER_SUBDEVICE_AVD:
	mtk_srccap_unregister_avd_subdev(&srccap_dev->subdev_avd);
UNREGISTER_SUBDEVICE_ADC:
	mtk_srccap_unregister_adc_subdev(&srccap_dev->subdev_adc);
RELEASE_M2M_DEVICE:
	v4l2_m2m_release(srccap_dev->mdev);
UNREGISTER_V4L2_DEVICE:
	v4l2_device_unregister(v4l2_dev);
EXIT:

	return ret;
}

static int srccap_remove(struct platform_device *pdev)
{
	struct mtk_srccap_dev *srccap_dev = platform_get_drvdata(pdev);

#ifdef CONFIG_OPTEE
	mtk_memout_svp_exit(srccap_dev);
#endif

	video_unregister_device(srccap_dev->vdev);
	video_device_release(srccap_dev->vdev);
	mtk_srccap_unregister_vbi_subdev(&srccap_dev->subdev_vbi);
	mtk_srccap_unregister_timingdetect_subdev(&srccap_dev->subdev_timingdetect);
	mtk_srccap_unregister_offlinedetect_subdev(&srccap_dev->subdev_offlinedetect);
	mtk_srccap_unregister_mux_subdev(&srccap_dev->subdev_mux);
	mtk_srccap_unregister_memout_subdev(&srccap_dev->subdev_memout);
	mtk_srccap_unregister_hdmirx_subdev(&srccap_dev->subdev_hdmirx);
	mtk_srccap_unregister_dscl_subdev(&srccap_dev->subdev_dscl);
	mtk_srccap_unregister_common_subdev(&srccap_dev->subdev_common);
	mtk_srccap_unregister_avd_subdev(&srccap_dev->subdev_avd);
	mtk_srccap_unregister_adc_subdev(&srccap_dev->subdev_adc);
	v4l2_m2m_release(srccap_dev->mdev);
	v4l2_device_unregister(srccap_dev->vdev->v4l2_dev);
	mtk_srccap_iounmap(srccap_dev);

	return 0;
}

static int __maybe_unused srccap_runtime_suspend(struct device *dev)
{
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	struct v4l2_event event;

	mtk_hdmirx_STREventProc(E_POWER_SUSPEND); // HDMI suspend
	event.type = V4L2_SRCCAP_SUSPEND_EVENT;
	event.id = 0;
	v4l2_event_queue(srccap_dev->vdev, &event);
	return 0;
}

static int __maybe_unused srccap_runtime_resume(struct device *dev)
{
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	struct v4l2_event event;

	mtk_hdmirx_STREventProc(E_POWER_RESUME); // HDMI resume
	event.type = V4L2_SRCCAP_RESUME_EVENT;
	event.id = 0;
	v4l2_event_queue(srccap_dev->vdev, &event);
	return 0;
}

static const struct dev_pm_ops srccap_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
						pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(srccap_runtime_suspend, srccap_runtime_resume, NULL)
};

MODULE_DEVICE_TABLE(of, mtk_srccap_match);

static struct platform_driver srccap_pdrv = {
	.probe = srccap_probe,
	.remove = srccap_remove,
	.driver = {
		.name = MTK_SRCCAP_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mtk_srccap_match,
		.pm = &srccap_pm_ops,
	},
};

module_platform_driver(srccap_pdrv);

MODULE_AUTHOR("MediaTek TV");
MODULE_DESCRIPTION("MTK Source Capture Driver");
MODULE_LICENSE("GPL");
