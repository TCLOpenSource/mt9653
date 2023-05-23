// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <uapi/linux/sched/types.h>

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_address.h>
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
#include <linux/wait.h>
#include <linux/vmalloc.h>
#include <linux/miscdevice.h>
#include <linux/ion.h>
#include <linux/syscalls.h>
#include <linux/of.h>

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
#include "mtk_srccap_timingdetect.h"
#include "mtk_srccap_dscl.h"
#include "mtk_srccap_memout.h"

//#include "mtk_srccap_vbi.h"
#include "drv_HDMI.h"
//#include "mtk_drv_vbi.h"
#include "hwreg_common_riu.h"
#include "mtk-reserved-memory.h"
#include "mtk_iommu_common.h"
#include "mtk-cma.h"
#include "avd-ex.h"
#include "vbi-ex.h"
#include "mtk_drv_vbi.h"
#include "drv_scriptmgt.h"
#include "linux/metadata_utility/m_srccap.h"

static struct miscdevice vbi_misc_dev;
static struct device *vbi_mmap_device;
static struct mtk_srccap_dev *srccap_vbi_dev;
static struct device_node *vbi_mmap_node;
static struct srccap_vbi_info srccap_vbi_info;
static u64 u64vbiemi0_base;
static struct of_mmap_info_data of_vbi_mmap_info;
static int vbideviceinitialized;
static bool srccap_is_dvdma_available = TRUE;

int hdmirxloglevel = LOG_LEVEL_ERROR;
module_param(hdmirxloglevel, int, 0644);

int log_level = LOG_LEVEL_OFF;
#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
int atrace_enable_srccap;
#endif
int dbglevel;


module_param(dbglevel, int, 0444); //S_IRUGO = 0444

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static inline struct mtk_srccap_ctx *srccap_fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct mtk_srccap_ctx, fh);
}

static int srccap_get_clk(
	struct device *dev,
	char *s,
	struct clk **clk)
{
	if (dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (s == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (clk == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	*clk = devm_clk_get(dev, s);
	if (IS_ERR(*clk)) {
		SRCCAP_MSG_FATAL("unable to retrieve %s(%d)\n", s, *clk);
		return PTR_ERR(clk);
	}

	return 0;
}

static void srccap_put_clk(
	struct device *dev,
	struct clk *clk)
{
	if (dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	devm_clk_put(dev, clk);
}

static int srccap_enable_parent(
	struct mtk_srccap_dev *srccap_dev,
	struct clk *clk,
	char *s)
{
	int ret = 0;
	struct clk *parent = NULL;

	ret = srccap_get_clk(srccap_dev->dev, s, &parent);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	ret = clk_set_parent(clk, parent);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	ret = clk_prepare_enable(clk);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	srccap_put_clk(srccap_dev->dev, parent);

	return 0;
}

static const char *srccap_read_dts_string(
	struct device_node *node,
	char *prop_name)
{
	int ret = 0;
	const char *out_str = NULL;

	if (node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (prop_name == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	ret = of_property_read_string(node, prop_name, &out_str);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	SRCCAP_MSG_INFO("%s = (%s)\n", prop_name, out_str);
	return out_str;
}


static int srccap_read_dts_u32(
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

static int srccap_read_dts_u32_array(
	struct device_node *node,
	char *s,
	u32 *array,
	u32 size)
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

	if (array == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	ret = of_property_read_u32_array(node, s, array, size);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	return ret;
}

static int srccap_read_stub_value(const char *path_name, int *val)
{
	int ret = 0;
	long file_size;
	uint8_t *buffer;
	struct file *file = NULL;
	mm_segment_t cur_mm_seg;
	loff_t pos;

	file = filp_open(path_name, (O_CREAT | O_RDONLY), 0);
	if (IS_ERR(file)) {
		pr_err("Cannot open %s.\n", path_name);
		ret = -ENOENT;
		goto EXIT;
	}

	/* seek the position of the end of the file and store the position to file_size */
	vfs_llseek(file, 0, SEEK_END);
	file_size = file->f_pos;
	if (file_size <= 0) {
		pr_err("file_size is not positive.\n");
		ret = -ENOENT;
		goto CLOSE_FILE;
	}

	/* seek the position of the beginning of the file and allocate buffer */
	pos = vfs_llseek(file, 0, SEEK_SET);
	buffer = vmalloc(file_size);
	if (buffer == NULL) {
		pr_err("Buffer allocation failed\n");
		ret = -ENOMEM;
		goto CLOSE_FILE;
	}

	/* read from the beginning of the file to the end of the file */
	cur_mm_seg = get_fs();
	set_fs(KERNEL_DS);
	ret = kernel_read(file, (char *)buffer, file_size, &pos);
	set_fs(cur_mm_seg);
	if (ret != (__u32)file_size) {
		pr_err("File reading failed\n");
		ret = -EIO;
		goto FREE_BUFFER;
	}

	/* store data */
	if (sscanf(buffer, "%d\n", val) < 0) {
		pr_err("sscanf failed\n");
		ret = -EIO;
		goto FREE_BUFFER;
	}

	pr_debug("stub report status >> path:%s, val:%d\n", path_name, *val);
FREE_BUFFER:
	vfree(buffer);
CLOSE_FILE:
	filp_close(file, NULL);
EXIT:
	return ret;
}

static int srccap_set_input(struct mtk_srccap_dev *srccap_dev, unsigned int input)
{
	int ret = 0;
	struct clk *clk = NULL;
	char s[SRCCAP_STRING_SIZE_50] = {0};
	int size = SRCCAP_STRING_SIZE_50;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]s_input(%u): source=%u\n", srccap_dev->dev_id, input);

	srccap_dev->srccap_info.src = input;

	/* set ipdma clock */
	if (srccap_dev->dev_id == 0)
		clk = srccap_dev->srccap_info.clk.ipdma0_ck;
	else if (srccap_dev->dev_id == 1)
		clk = srccap_dev->srccap_info.clk.ipdma1_ck;
	else
		SRCCAP_MSG_ERROR("device id not found\n");

	switch (input) {
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
		snprintf(s, size, "srccap_ipdma%d_ck_hdmi_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
		snprintf(s, size, "srccap_ipdma%d_ck_hdmi2_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
		snprintf(s, size, "srccap_ipdma%d_ck_hdmi3_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		snprintf(s, size, "srccap_ipdma%d_ck_hdmi4_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		snprintf(s, size, "srccap_ipdma%d_ck_vd_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		snprintf(s, size, "srccap_ipdma%d_ck_vd_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		snprintf(s, size, "srccap_ipdma%d_ck_adc_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		snprintf(s, size, "srccap_ipdma%d_ck_adc_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		snprintf(s, size, "srccap_ipdma%d_ck_vd_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		snprintf(s, size, "srccap_ipdma%d_ck_vd_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
		snprintf(s, size, "srccap_ipdma%d_ck_hdmi_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
		snprintf(s, size, "srccap_ipdma%d_ck_hdmi2_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
		snprintf(s, size, "srccap_ipdma%d_ck_hdmi3_ck", srccap_dev->dev_id);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		snprintf(s, size, "srccap_ipdma%d_ck_hdmi4_ck", srccap_dev->dev_id);
		break;
	default:
		SRCCAP_MSG_ERROR("input source not found\n");
		return -EINVAL;
	}
	ret = srccap_enable_parent(srccap_dev, clk, s);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	/* set ipmux */
	ret = mtk_srccap_mux_set_source(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	mtk_hdmirx_set_current_port(srccap_dev);

	return 0;
}

static void srccap_compare_attr(struct srccap_timingdetect_data *data,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	struct v4l2_event event;
	enum v4l2_srccap_input_source src = srccap_dev->srccap_info.src;

	memset(&event, 0, sizeof(struct v4l2_event));

	if ((data->h_de != srccap_dev->timingdetect_info.data.h_de) ||
	    (data->v_de != srccap_dev->timingdetect_info.data.v_de)) {
		/* set event */
		if (srccap_dev->srccap_info.timing_monitor_task != NULL) {
			memset(&event, 0, sizeof(struct v4l2_event));
			event.type = V4L2_EVENT_RESOLUTION_CHANGE;
			event.id = 0;
			v4l2_event_queue(srccap_dev->vdev, &event);
		}
	}

	if (data->v_freqx1000 != srccap_dev->timingdetect_info.data.v_freqx1000) {
		/* set event */
		if (srccap_dev->srccap_info.timing_monitor_task != NULL) {
			memset(&event, 0, sizeof(struct v4l2_event));
			event.type = V4L2_EVENT_FRAMERATE_CHANGE;
			event.id = 0;
			v4l2_event_queue(srccap_dev->vdev, &event);
		}
	}

	if (data->interlaced != srccap_dev->timingdetect_info.data.interlaced) {
		/* set event */
		if (srccap_dev->srccap_info.timing_monitor_task != NULL) {
			memset(&event, 0, sizeof(struct v4l2_event));
			event.type = V4L2_EVENT_SCANTYPE_CHANGE;
			event.id = 0;
			v4l2_event_queue(srccap_dev->vdev, &event);
		}
	}

	if ((src >= V4L2_SRCCAP_INPUT_SOURCE_HDMI) ||
		(src <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4)) {
		if (data->colorformat !=
			srccap_dev->timingdetect_info.data.colorformat) {
			if (srccap_dev->srccap_info.timing_monitor_task != NULL) {
				/* set event */
				memset(&event, 0, sizeof(struct v4l2_event));
				event.type = V4L2_EVENT_COLORFORMAT_CHANGE;
				event.id = 0;
				v4l2_event_queue(srccap_dev->vdev, &event);
			}
		}
	}
}

static int srccap_set_capture_win(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	/* set capture window if not set already */
	if ((srccap_dev->timingdetect_info.cap_win.left == 0)
		&& (srccap_dev->timingdetect_info.cap_win.top == 0)
		&& (srccap_dev->timingdetect_info.cap_win.width == 0)
		&& (srccap_dev->timingdetect_info.cap_win.height == 0)) {
		srccap_dev->timingdetect_info.cap_win.left =
			srccap_dev->timingdetect_info.data.h_start;
		srccap_dev->timingdetect_info.cap_win.top =
			srccap_dev->timingdetect_info.data.v_start;
		srccap_dev->timingdetect_info.cap_win.width =
			(srccap_dev->timingdetect_info.data.yuv420) ?
			srccap_dev->timingdetect_info.data.h_de / SRCCAP_YUV420_WIDTH_DIVISOR :
			srccap_dev->timingdetect_info.data.h_de;
		srccap_dev->timingdetect_info.cap_win.height =
			srccap_dev->timingdetect_info.data.v_de;
	}

	SRCCAP_MSG_DEBUG("yuv420:%d, cap win(%d,%d,%d,%d)\n",
		srccap_dev->timingdetect_info.data.yuv420,
		srccap_dev->timingdetect_info.cap_win.left,
		srccap_dev->timingdetect_info.cap_win.top,
		srccap_dev->timingdetect_info.cap_win.width,
		srccap_dev->timingdetect_info.cap_win.height);

	ret = mtk_timingdetect_set_capture_win(srccap_dev,
		srccap_dev->timingdetect_info.cap_win);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	return 0;
}

static int srccap_check_colorformat(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_srccap_input_source src,
	struct srccap_timingdetect_data *data)
{
	int ret = 0;
	u8 hdmi_status = 0;
	bool is_hdmi_mode = FALSE;
	struct hdmi_avi_packet_info avi_info;

	memset(&avi_info, 0, sizeof(struct hdmi_avi_packet_info));

	if (srccap_dev->srccap_info.stub_mode == 0) {
		is_hdmi_mode = mtk_srccap_hdmirx_isHDMIMode(src);
		if (is_hdmi_mode) {
			hdmi_status = mtk_srccap_hdmirx_pkt_get_AVI(src, &avi_info,
				sizeof(struct hdmi_avi_packet_info));

			data->colorformat = avi_info.enColorForamt;
			SRCCAP_MSG_INFO("[CTRL_FLOW]color_format:%d\n", data->colorformat);
		} else {
			data->colorformat = (u8)M_HDMI_COLOR_RGB;
			SRCCAP_MSG_INFO("[CTRL_FLOW] DVI mode default color_format is RGB\n");
		}
	} else {
		int colorformat = 0, hdmi_mode = 0;

		ret = srccap_read_stub_value("/data/local/tmp/mtk_srccap_hdmi_mode", &hdmi_mode);
		if (ret < 0) {
			ret = srccap_read_stub_value("/tmp/mtk_srccap_hdmi_mode", &hdmi_mode);
			if (ret < 0)
				return ret;
		}

		if (hdmi_mode) {
			ret = srccap_read_stub_value("/data/local/tmp/mtk_srccap_colorformat",
				&colorformat);
			if (ret < 0) {
				ret = srccap_read_stub_value("/tmp/mtk_srccap_colorformat",
					&colorformat);
				if (ret < 0)
					return ret;
			}
			data->colorformat = colorformat;
		} else {
			data->colorformat = M_HDMI_COLOR_RGB;
		}
	}

	return ret;
}

static int srccap_set_progressive(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_timingdetect_vrr_type vrr_type)
{
	int ret = 0;
	//reserve first to add hdmi_free_sync check
	//bool hdmi_free_sync = FALSE;
	//enum v4l2_srccap_freesync_type enfstype = 0;
	//hdmi_free_sync = mtk_srccap_hdmirx_get_freesync_enable(srccap_dev->srccap_info.src,
	//							&enfstype);
	if ((vrr_type == SRCCAP_TIMINGDETECT_VRR_TYPE_VRR)
		|| (vrr_type == SRCCAP_TIMINGDETECT_VRR_TYPE_CVRR)
		|| (srccap_dev->timingdetect_info.data.vrr_enforcement_en == TRUE))
		//|| (hdmi_free_sync == TRUE))
		ret = mtk_timingdetect_force_progressive(srccap_dev, TRUE);
	else
		ret = mtk_timingdetect_force_progressive(srccap_dev, FALSE);

	return ret;
}

static int srccap_set_vtt_det_en(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_timingdetect_vrr_type vrr_type)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	bool en = FALSE;
	//reserve first to add hdmi_free_sync check
	//bool hdmi_free_sync = FALSE;
	//enum v4l2_srccap_freesync_type enfstype = 0;
	//hdmi_free_sync = mtk_srccap_hdmirx_get_freesync_enable(srccap_dev->srccap_info.src,
	//							&enfstype);
	if (srccap_dev->timingdetect_info.data.vrr_enforcement_en == FALSE) {
		if (vrr_type != SRCCAP_TIMINGDETECT_VRR_TYPE_NVRR)
			//||(hdmi_free_sync == TRUE));
			en = TRUE;

		ret = mtk_timingdetect_set_ans_vtt_det_en(srccap_dev, en);
		if (ret < 0)
			goto exit;

		if (en == TRUE) {
			ret = mtk_timingdetect_set_auto_no_signal_mute(srccap_dev, FALSE);
			if (ret < 0)
				goto exit;
		}

	}

	return ret;
exit:
	SRCCAP_MSG_RETURN_CHECK(ret);
	return ret;
}

static int srccap_set_vrr_related_settings(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_timingdetect_vrr_type vrr_type)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;

	//update vrr_type in metadata
	srccap_dev->timingdetect_info.data.vrr_type = vrr_type;

	ret = srccap_set_progressive(srccap_dev, vrr_type);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = srccap_set_vtt_det_en(srccap_dev, vrr_type);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

static void srccap_set_dscl(struct mtk_srccap_dev *srccap_dev)
{
	struct srccap_ml_script_info ml_script_info;
	u8 ml_mem_index = 0;
	int ret = 0;
	bool stub = 0;
	struct sm_ml_fire_info ml_fire_info;
	struct sm_ml_info ml_info;
	enum sm_return_type ml_ret = 0;

	memset(&ml_script_info, 0, sizeof(struct srccap_ml_script_info));
	memset(&ml_fire_info, 0, sizeof(struct sm_ml_fire_info));
	memset(&ml_info, 0, sizeof(struct sm_ml_info));

	ret = drv_hwreg_common_get_stub(&stub);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

	ml_ret = sm_ml_get_mem_index(srccap_dev->srccap_info.ml_info.fd_dscl, &ml_mem_index, FALSE);

	if ((ml_ret == E_SM_RET_OK) || (stub == true)) {
		ml_script_info.instance = srccap_dev->srccap_info.ml_info.fd_dscl;
		ml_script_info.mem_index = ml_mem_index;

		ret = sm_ml_get_info(ml_script_info.instance, ml_script_info.mem_index, &ml_info);
		if (ret == E_SM_RET_OK) {
			ml_script_info.start_addr = (void *)ml_info.start_va;
			ml_script_info.max_addr =
				(void *)((u8 *)ml_info.start_va + ml_info.mem_size);
			ml_script_info.mem_size = ml_info.mem_size;
		} else {
			SRCCAP_MSG_ERROR("sm_ml_get_info fail\n");
		}

		SRCCAP_MSG_DEBUG("sm_ml_get_info, start_va:%x, mem_size:%x, max addr:%x\n",
			ml_info.start_va, ml_info.mem_size, ml_script_info.max_addr);

		srccap_dev->dscl_info.dscl_size.input.width =
			(srccap_dev->timingdetect_info.data.yuv420 == true) ?
			srccap_dev->timingdetect_info.cap_win.width * SRCCAP_YUV420_WIDTH_DIVISOR :
			srccap_dev->timingdetect_info.cap_win.width;

		srccap_dev->dscl_info.dscl_size.input.height =
			srccap_dev->timingdetect_info.cap_win.height;

		srccap_dev->dscl_info.dscl_size.output.width =
			(srccap_dev->dscl_info.user_dscl_size.output.width == 0) ?
			srccap_dev->dscl_info.dscl_size.input.width :
			srccap_dev->dscl_info.user_dscl_size.output.width;

		srccap_dev->dscl_info.dscl_size.output.height =
			(srccap_dev->dscl_info.user_dscl_size.output.height == 0) ?
			srccap_dev->dscl_info.dscl_size.input.height :
			srccap_dev->dscl_info.user_dscl_size.output.height;

		SRCCAP_MSG_DEBUG("dscl(%u): input(%u,%u), output(%u,%u), yuv420:%d\n",
			srccap_dev->dev_id,
			srccap_dev->dscl_info.dscl_size.input.width,
			srccap_dev->dscl_info.dscl_size.input.height,
			srccap_dev->dscl_info.dscl_size.output.width,
			srccap_dev->dscl_info.dscl_size.output.height,
			srccap_dev->timingdetect_info.data.yuv420);

		ret = mtk_srccap_common_load_pqmap(srccap_dev,
				(struct srccap_ml_script_info *)&ml_script_info);
		if (ret < 0)
			SRCCAP_MSG_RETURN_CHECK(ret);

		mtk_dscl_set_scaling(srccap_dev, (struct srccap_ml_script_info *)&ml_script_info,
				&srccap_dev->dscl_info.dscl_size);

		mtk_memout_set_capture_size(srccap_dev,
			(struct srccap_ml_script_info *)&ml_script_info,
			(struct v4l2_area_size *)&srccap_dev->dscl_info.dscl_size.output);

		if (stub == 0) {
			ml_fire_info.cmd_cnt = ml_script_info.total_cmd_count;
			ml_fire_info.external = FALSE;
			ml_fire_info.mem_index = ml_mem_index;
			sm_ml_fire(srccap_dev->srccap_info.ml_info.fd_dscl, &ml_fire_info);
		}
	} else
		SRCCAP_MSG_DEBUG("sm_ml_get_mem_index fail, ml_ret:%d\n", ml_ret);
}

static int srccap_set_frl_tmds_mode(
	struct mtk_srccap_dev *srccap_dev,
	bool frl)
{
	int ret = 0;

	ret = mtk_srccap_dscl_set_source(srccap_dev, frl);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_timingdetect_set_8p(srccap_dev, frl);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

static int srccap_handle_status_change_event(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_timingdetect_vrr_type vrr_type,
	enum v4l2_timingdetect_status status)
{

	int ret = 0;
	enum srccap_adc_isog_detect_mode isog_mode = SRCCAP_ADC_ISOG_NORMAL_MODE;
	bool powerdown = FALSE;
	struct srccap_timingdetect_data data;
	enum v4l2_srccap_input_source src = srccap_dev->srccap_info.src;

	//reserve first to add hdmi_free_sync check
	//enum v4l2_srccap_freesync_type enfstype = 0;
	//bool hdmi_freesync = FALSE;
	memset(&data, 0, sizeof(struct srccap_timingdetect_data));

	switch (status) {
	case V4L2_TIMINGDETECT_NO_SIGNAL:
		isog_mode = SRCCAP_ADC_ISOG_STANDBY_MODE;
		powerdown = TRUE;

		break;
	case V4L2_TIMINGDETECT_UNSTABLE_SYNC:
		isog_mode = SRCCAP_ADC_ISOG_NORMAL_MODE;
		powerdown = TRUE;

		break;
	case V4L2_TIMINGDETECT_STABLE_SYNC:
		boottime_print("MTK Srccap timing stable [end]\n");
		ret = mtk_timingdetect_retrieve_timing(srccap_dev, vrr_type, FALSE, &data);
		//reserve first to add hdmi_free_sync check
		//enum v4l2_srccap_freesync_type enfstype;
		//hdmi_freesync = mtk_srccap_hdmirx_get_freesync_enable(srccap_dev->srccap_info.src,
		//&enfstype);
		//ret = mtk_timingdetect_retrieve_timing(srccap_dev, vrr_type,
		//, &data);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		if ((src >= V4L2_SRCCAP_INPUT_SOURCE_HDMI) &&
		(src <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4)) {
			ret = srccap_check_colorformat(srccap_dev, src, &data);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}
		}

		srccap_compare_attr(&data, srccap_dev);

		ret = mtk_timingdetect_store_timing_info(srccap_dev, data);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		/* set FRL/TMDS mode */
		ret = srccap_set_frl_tmds_mode(srccap_dev, data.frl);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		if (srccap_dev->adc_info.adc_src != SRCCAP_ADC_SOURCE_NONADC) {
			/* set adc mode when signal stable */
			ret = mtk_adc_set_mode(srccap_dev);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}

			/* load calibration table */
			ret = mtk_adc_load_cal_tbl(srccap_dev);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}
		}

		mtk_srccap_common_set_hdmi_420to444(srccap_dev);
		isog_mode = SRCCAP_ADC_ISOG_NORMAL_MODE;
		powerdown = FALSE;

		ret = mtk_timingdetect_set_auto_no_signal_mute(srccap_dev, FALSE);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		break;
	default:
		break;
	}

	ret = mtk_adc_set_isog_detect_mode(srccap_dev, isog_mode);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_adc_set_iclamp_rgb_powerdown(srccap_dev, powerdown);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = srccap_set_vrr_related_settings(srccap_dev, vrr_type);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

static int srccap_get_event_string(
	u32 event,
	u8 size,
	char *s)
{
	if (s == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (event) {
	case V4L2_EVENT_VSYNC:
		strncpy(s, "VSYNC", size);
		break;
	case V4L2_EVENT_CTRL:
		strncpy(s, "CTRL", size);
		break;
	case V4L2_EVENT_SIGNAL_STATUS_CHANGE:
		strncpy(s, "SIGNAL_STATUS_CHANGE", size);
		break;
	case V4L2_EVENT_RESOLUTION_CHANGE:
		strncpy(s, "RESOLUTION_CHANGE", size);
		break;
	case V4L2_EVENT_FRAMERATE_CHANGE:
		strncpy(s, "FRAMERATE_CHANGE", size);
		break;
	case V4L2_EVENT_SCANTYPE_CHANGE:
		strncpy(s, "SCANTYPE_CHANGE", size);
		break;
	case V4L2_EVENT_COLORFORMAT_CHANGE:
		strncpy(s, "COLORFORMAT_CHANGE", size);
		break;
	case V4L2_EVENT_HDMI_SIGNAL_CHANGE:
		strncpy(s, "HDMI_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_HDMI2_SIGNAL_CHANGE:
		strncpy(s, "HDMI2_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_HDMI3_SIGNAL_CHANGE:
		strncpy(s, "HDMI3_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_HDMI4_SIGNAL_CHANGE:
		strncpy(s, "HDMI4_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_CVBS_SIGNAL_CHANGE:
		strncpy(s, "CVBS_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_CVBS2_SIGNAL_CHANGE:
		strncpy(s, "CVBS2_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_CVBS3_SIGNAL_CHANGE:
		strncpy(s, "CVBS3_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_CVBS4_SIGNAL_CHANGE:
		strncpy(s, "CVBS4_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_CVBS5_SIGNAL_CHANGE:
		strncpy(s, "CVBS5_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_CVBS6_SIGNAL_CHANGE:
		strncpy(s, "CVBS6_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_CVBS7_SIGNAL_CHANGE:
		strncpy(s, "CVBS7_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_CVBS8_SIGNAL_CHANGE:
		strncpy(s, "CVBS8_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_SVIDEO_SIGNAL_CHANGE:
		strncpy(s, "SVIDEO_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_SVIDEO2_SIGNAL_CHANGE:
		strncpy(s, "SVIDEO2_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_SVIDEO3_SIGNAL_CHANGE:
		strncpy(s, "SVIDEO3_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_SVIDEO4_SIGNAL_CHANGE:
		strncpy(s, "SVIDEO4_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_YPBPR_SIGNAL_CHANGE:
		strncpy(s, "YPBPR_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_YPBPR2_SIGNAL_CHANGE:
		strncpy(s, "YPBPR2_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_YPBPR3_SIGNAL_CHANGE:
		strncpy(s, "YPBPR3_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_VGA_SIGNAL_CHANGE:
		strncpy(s, "VGA_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_VGA2_SIGNAL_CHANGE:
		strncpy(s, "VGA2_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_VGA3_SIGNAL_CHANGE:
		strncpy(s, "VGA3_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_SCART_SIGNAL_CHANGE:
		strncpy(s, "SCART_SIGNAL_CHANGE", size);
		break;
	case V4L2_EVENT_SCART2_SIGNAL_CHANGE:
		strncpy(s, "SCART2_SIGNAL_CHANGE", size);
		break;
	case V4L2_SRCCAP_SUSPEND_EVENT:
		strncpy(s, "SUSPEND", size);
		break;
	case V4L2_SRCCAP_RESUME_EVENT:
		strncpy(s, "RESUME", size);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int srccap_get_event_type(
	enum v4l2_srccap_input_source sync_src,
	int *event_type)
{
	switch (sync_src) {
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
		*event_type = V4L2_EVENT_HDMI_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
		*event_type = V4L2_EVENT_HDMI2_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
		*event_type = V4L2_EVENT_HDMI3_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		*event_type = V4L2_EVENT_HDMI4_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
		*event_type = V4L2_EVENT_CVBS_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
		*event_type = V4L2_EVENT_CVBS2_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
		*event_type = V4L2_EVENT_CVBS3_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
		*event_type = V4L2_EVENT_CVBS4_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
		*event_type = V4L2_EVENT_CVBS5_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
		*event_type = V4L2_EVENT_CVBS6_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
		*event_type = V4L2_EVENT_CVBS7_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		*event_type = V4L2_EVENT_CVBS8_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
		*event_type = V4L2_EVENT_SVIDEO_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
		*event_type = V4L2_EVENT_SVIDEO2_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
		*event_type = V4L2_EVENT_SVIDEO3_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		*event_type = V4L2_EVENT_SVIDEO4_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
		*event_type = V4L2_EVENT_YPBPR_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
		*event_type = V4L2_EVENT_YPBPR2_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		*event_type = V4L2_EVENT_YPBPR3_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
		*event_type = V4L2_EVENT_VGA_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
		*event_type = V4L2_EVENT_VGA2_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		*event_type = V4L2_EVENT_VGA3_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
		*event_type = V4L2_EVENT_SCART_SIGNAL_CHANGE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		*event_type = V4L2_EVENT_SCART2_SIGNAL_CHANGE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int srccap_get_inputsrc_type(
	u32 cid,
	enum v4l2_srccap_input_source *inputsrc)
{
	switch (cid) {
	case V4L2_CID_SRCCAP_HDMI_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_HDMI;
		break;
	case V4L2_CID_SRCCAP_HDMI2_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_HDMI2;
		break;
	case V4L2_CID_SRCCAP_HDMI3_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_HDMI3;
		break;
	case V4L2_CID_SRCCAP_HDMI4_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_HDMI4;
		break;
	case V4L2_CID_SRCCAP_CVBS_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
		break;
	case V4L2_CID_SRCCAP_CVBS2_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS2;
		break;
	case V4L2_CID_SRCCAP_CVBS3_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS3;
		break;
	case V4L2_CID_SRCCAP_CVBS4_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS4;
		break;
	case V4L2_CID_SRCCAP_CVBS5_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS5;
		break;
	case V4L2_CID_SRCCAP_CVBS6_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS6;
		break;
	case V4L2_CID_SRCCAP_CVBS7_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS7;
		break;
	case V4L2_CID_SRCCAP_CVBS8_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS8;
		break;
	case V4L2_CID_SRCCAP_SVIDEO_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO;
		break;
	case V4L2_CID_SRCCAP_SVIDEO2_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2;
		break;
	case V4L2_CID_SRCCAP_SVIDEO3_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3;
		break;
	case V4L2_CID_SRCCAP_SVIDEO4_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4;
		break;
	case V4L2_CID_SRCCAP_YPBPR_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
		break;
	case V4L2_CID_SRCCAP_YPBPR2_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_YPBPR2;
		break;
	case V4L2_CID_SRCCAP_YPBPR3_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_YPBPR3;
		break;
	case V4L2_CID_SRCCAP_VGA_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_VGA;
		break;
	case V4L2_CID_SRCCAP_VGA2_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_VGA2;
		break;
	case V4L2_CID_SRCCAP_VGA3_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_VGA3;
		break;
	case V4L2_CID_SRCCAP_SCART_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_SCART;
		break;
	case V4L2_CID_SRCCAP_SCART2_SYNC_STATUS:
		*inputsrc = V4L2_SRCCAP_INPUT_SOURCE_SCART2;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int srccap_handle_src_sync_status(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	int enable_offline_sog_ret = 0;
	int event_type = 0;
	bool sync_status = FALSE;
	bool old_sync_status = FALSE;
	bool sync_changed = FALSE;
	bool adc_offline_status = FALSE;
	enum v4l2_srccap_input_source sync_src_index = 0;
	struct v4l2_event event;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	memset(&event, 0, sizeof(struct v4l2_event));

	for (sync_src_index = V4L2_SRCCAP_INPUT_SOURCE_HDMI;
		sync_src_index < V4L2_SRCCAP_INPUT_SOURCE_NUM;
		sync_src_index++) {
		if (srccap_dev->srccap_info.sts.sync_status_event[sync_src_index] == FALSE)
			continue;

		switch (sync_src_index) {
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
		case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
		case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
		case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
		case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
		case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		case V4L2_SRCCAP_INPUT_SOURCE_VGA:
		case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
		case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		case V4L2_SRCCAP_INPUT_SOURCE_SCART:
		case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
			ret = mtk_adc_check_offline_source(
				srccap_dev, sync_src_index, &adc_offline_status);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				continue;
			}

			if (adc_offline_status == TRUE) {
				ret = mtk_adc_set_offline_mux(srccap_dev, sync_src_index);
				if (ret < 0) {
					SRCCAP_MSG_RETURN_CHECK(ret);
					continue;
				}

				enable_offline_sog_ret = mtk_adc_enable_offline_sog(srccap_dev,
											TRUE);
				if (enable_offline_sog_ret < 0) {
					SRCCAP_MSG_RETURN_CHECK(enable_offline_sog_ret);
					return enable_offline_sog_ret;
				}
				usleep_range(SRCCAP_UDELAY_1000, SRCCAP_UDELAY_1100); // sleep 1ms

				ret = mtk_timingdetect_init_offline(srccap_dev);
				if (ret) {
					SRCCAP_MSG_RETURN_CHECK(ret);
					ret = mtk_adc_enable_offline_sog(srccap_dev, FALSE);
					if (ret < 0) {
						SRCCAP_MSG_RETURN_CHECK(ret);
						return ret;
					}
					usleep_range(SRCCAP_UDELAY_1000, SRCCAP_UDELAY_1100);
					continue;
				}

				msleep(SRCCAP_BANK_REFRESH_MDELAY_60);
			}
			break;
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
			adc_offline_status = FALSE;
			break;
		default:
			continue;
		}

		ret = mtk_timingdetect_check_sync(sync_src_index, adc_offline_status, &sync_status);

		enable_offline_sog_ret = mtk_adc_enable_offline_sog(srccap_dev, FALSE);
		if (enable_offline_sog_ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(enable_offline_sog_ret);
			return enable_offline_sog_ret;
		}
		usleep_range(SRCCAP_UDELAY_1000, SRCCAP_UDELAY_1100); // sleep 1ms

		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			continue;
		}

		old_sync_status = srccap_dev->srccap_info.sync_status[sync_src_index];
		if (sync_status != old_sync_status) {
			memset(&event, 0, sizeof(struct v4l2_event));
			ret = srccap_get_event_type(sync_src_index, &event_type);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				continue;
			}
			srccap_dev->srccap_info.sync_status[sync_src_index] = sync_status;
			event.type = event_type;
			event.id = 0;
			v4l2_event_queue(srccap_dev->vdev, &event);
		}
		usleep_range(SRCCAP_UDELAY_1000, SRCCAP_UDELAY_1100); // sleep 1ms
	}

	return 0;
}

static int buffer_handling_task(void *data)
{
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;
	s64 jiffies = 0;
	bool timeout = FALSE;
	bool buffer_available = FALSE;
	u8 src_field = 0;

	if (data == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	srccap_ctx = (struct mtk_srccap_ctx *)data;
	srccap_dev = srccap_ctx->srccap_dev;
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	SRCCAP_GET_JIFFIES_FOR_N_MS(SRCCAP_MEMOUT_24FPS_FRAME_TIME, &jiffies);

	while (!kthread_should_stop()) {
		timeout = wait_event_interruptible_timeout(
			srccap_dev->srccap_info.waitq_list.waitq_buffer,
			srccap_dev->srccap_info.waitq_list.buffer_done,
			jiffies) <= 0;
		if (timeout)
			continue;
		srccap_dev->srccap_info.waitq_list.buffer_done = 0;

		/* ensure vb is available before use */
		mutex_lock(&srccap_ctx->fh.m2m_ctx->cap_q_ctx.q.mmap_lock);
		buffer_available = (srccap_ctx->fh.m2m_ctx->cap_q_ctx.q.num_buffers != 0);
		mutex_unlock(&srccap_ctx->fh.m2m_ctx->cap_q_ctx.q.mmap_lock);
		if (!buffer_available)
			continue;

		mtk_timingdetect_get_field(srccap_dev, &src_field);
		mtk_memout_process_buffer(srccap_dev, src_field);
	}

	return 0;
}

static int srccap_create_buffer_handling_task(struct mtk_srccap_ctx *srccap_ctx)
{
	struct mtk_srccap_dev *srccap_dev = NULL;
	spinlock_t *spinlock_buffer_handling_task = NULL;
	unsigned long flags = 0;
	struct task_struct *task_buffer_handling = NULL;

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	srccap_dev = srccap_ctx->srccap_dev;
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev->memout_info.buf_ctrl_mode != V4L2_MEMOUT_SWMODE)
		return 0;

	spinlock_buffer_handling_task =
		&srccap_dev->srccap_info.spinlock_list.spinlock_buffer_handling_task;

	task_buffer_handling = kthread_create(
		buffer_handling_task,
		srccap_ctx,
		"buffer handling task");
	if (task_buffer_handling == ERR_PTR(-ENOMEM)) {
		SRCCAP_MSG_ERROR("task not created\n");
		return -ENOMEM;
	}

	get_task_struct(task_buffer_handling);
	wake_up_process(task_buffer_handling);

	spin_lock_irqsave(spinlock_buffer_handling_task, flags);
	srccap_dev->srccap_info.buffer_handling_task = task_buffer_handling;
	spin_unlock_irqrestore(spinlock_buffer_handling_task, flags);

	return 0;
}

static int srccap_destroy_buffer_handling_task(struct mtk_srccap_ctx *srccap_ctx)
{
	struct mtk_srccap_dev *srccap_dev = NULL;
	spinlock_t *spinlock_buffer_handling_task = NULL;
	unsigned long flags = 0;
	struct task_struct *task_buffer_handling = NULL;

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	srccap_dev = srccap_ctx->srccap_dev;
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev->memout_info.buf_ctrl_mode != V4L2_MEMOUT_SWMODE)
		return 0;

	spinlock_buffer_handling_task =
		&srccap_dev->srccap_info.spinlock_list.spinlock_buffer_handling_task;

	spin_lock_irqsave(spinlock_buffer_handling_task, flags);
	task_buffer_handling = srccap_dev->srccap_info.buffer_handling_task;
	srccap_dev->srccap_info.buffer_handling_task = NULL;
	spin_unlock_irqrestore(spinlock_buffer_handling_task, flags);

	if (task_buffer_handling == ERR_PTR(-ENOMEM)) {
		SRCCAP_MSG_ERROR("task not destroyed\n");
		return -ENOMEM;
	}

	if (task_buffer_handling != NULL) {
		kthread_stop(task_buffer_handling);
		put_task_struct(task_buffer_handling);
	}

	return 0;
}

static int _mtk_dv_init(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	struct srccap_dv_init_in dv_in;
	struct srccap_dv_init_out dv_out;
	struct device_node *dv_node = NULL;
	struct of_mmap_info_data of_mmap_info = {0};

	memset(&dv_in, 0, sizeof(struct srccap_dv_init_in));
	memset(&dv_out, 0, sizeof(struct srccap_dv_init_out));
	memset(&of_mmap_info, 0, sizeof(struct of_mmap_info_data));

	dv_in.dev = srccap_dev;
	dv_in.mmap_size = 0;
	dv_in.mmap_addr = 0;

	if (srccap_dev->dev->of_node != NULL)
		of_node_get(srccap_dev->dev->of_node);

	/* get dolby dma address from MMAP */
	dv_node = of_find_node_by_name(srccap_dev->dev->of_node, "dv");
	if (dv_node == NULL)
		SRCCAP_MSG_POINTER_CHECK();
	else {
		ret = of_mtk_get_reserved_memory_info_by_idx(
			dv_node, 0, &of_mmap_info);
		if (ret < 0)
			SRCCAP_MSG_RETURN_CHECK(ret);
		else {
			dv_in.mmap_size = of_mmap_info.buffer_size;
			dv_in.mmap_addr = of_mmap_info.start_bus_address;
		}
		of_node_put(dv_node);
	}

	ret = mtk_dv_init(&dv_in, &dv_out);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto exit;
	}

exit:
	return ret;
}

static void _mtk_dv_dqbuf_free_param(
	struct hdmi_avi_packet_info  *avi,
	struct hdmi_vsif_packet_info *vsif_array,
	struct hdmi_emp_packet_info  *emp_array,
	struct hdmi_hdr_packet_info  *drm_array,
	struct srccap_dv_dqbuf_in    *dv_in,
	struct srccap_dv_dqbuf_out   *dv_out)
{
	if (avi != NULL)
		kfree(avi);

	if (vsif_array != NULL)
		kfree(vsif_array);

	if (emp_array != NULL)
		kfree(emp_array);

	if (drm_array != NULL)
		kfree(drm_array);

	if (dv_in != NULL)
		kfree(dv_in);

	if (dv_out != NULL)
		kfree(dv_out);
}

static int _mtk_dv_dqbuf(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_buffer *buf)
{
	int ret = 0;
	struct hdmi_avi_packet_info  *avi = NULL;
	struct hdmi_vsif_packet_info *vsif_array = NULL;
	struct hdmi_emp_packet_info  *emp_array = NULL;
	struct hdmi_hdr_packet_info  *drm_array = NULL;
	struct srccap_dv_dqbuf_in    *dv_in = NULL;
	struct srccap_dv_dqbuf_out   *dv_out = NULL;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (buf == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	avi = kzalloc(sizeof(struct hdmi_avi_packet_info), GFP_KERNEL);
	if (avi == NULL) {
		SRCCAP_MSG_ERROR("allocate memory fail\n");
		goto exit;
	}

	vsif_array = kzalloc(
		sizeof(struct hdmi_vsif_packet_info)
		* SRCCAP_MAX_VSIF_PACKET_NUMBER, GFP_KERNEL);
	if (vsif_array == NULL) {
		SRCCAP_MSG_ERROR("allocate memory fail\n");
		goto exit;
	}

	emp_array = kzalloc(
		sizeof(struct hdmi_emp_packet_info)
		* SRCCAP_MAX_EMP_PACKET_NUMBER, GFP_KERNEL);
	if (emp_array == NULL) {
		SRCCAP_MSG_ERROR("allocate memory fail\n");
		goto exit;
	}

	drm_array = kzalloc(
		sizeof(struct hdmi_hdr_packet_info)
		* MAX_DRM_PACKET_NUMBER, GFP_KERNEL);
	if (drm_array == NULL) {
		SRCCAP_MSG_ERROR("allocate memory fail\n");
		goto exit;
	}

	dv_in = kzalloc(sizeof(struct srccap_dv_dqbuf_in), GFP_KERNEL);
	if (dv_in == NULL) {
		SRCCAP_MSG_ERROR("allocate memory fail\n");
		goto exit;
	}

	dv_out = kzalloc(sizeof(struct srccap_dv_dqbuf_out), GFP_KERNEL);
	if (dv_out == NULL) {
		SRCCAP_MSG_ERROR("allocate memory fail\n");
		goto exit;
	}

	memset(dv_in, 0, sizeof(struct srccap_dv_dqbuf_in));
	memset(dv_out, 0, sizeof(struct srccap_dv_dqbuf_out));

	dv_in->dev = srccap_dev;
	dv_in->buf = buf;

	if (IS_SRCCAP_DV_STUB_MODE()) {
		/* stub mode test */
		ret = mtk_dv_descrb_stub_get_test_avi(
			avi, sizeof(struct hdmi_avi_packet_info));
	} else if (mtk_srccap_common_is_Cfd_stub_mode()) {
		ret = mtk_srccap_common_get_cfd_test_HDMI_pkt_AVI(avi);
	} else {
		ret = mtk_srccap_hdmirx_pkt_get_AVI(srccap_dev->srccap_info.src,
			avi, sizeof(struct hdmi_avi_packet_info));
	}

	if (ret == 0) {
		SRCCAP_MSG_DEBUG("[SRCCAPDV] get AVI packet info fail\n");
		dv_in->avi_valid = FALSE;
	} else {
		dv_in->avi_valid = TRUE;
		dv_in->avi = avi;
	}

	if (IS_SRCCAP_DV_STUB_MODE_VSIF()) {
		/* stub mode test */
		ret = mtk_dv_descrb_stub_get_test_vsif(
			(void *)vsif_array, sizeof(struct hdmi_vsif_packet_info)
			*SRCCAP_MAX_VSIF_PACKET_NUMBER);
	} else {
		ret = mtk_srccap_hdmirx_pkt_get_VSIF(srccap_dev->srccap_info.src,
			vsif_array, sizeof(struct hdmi_vsif_packet_info)
			* SRCCAP_MAX_VSIF_PACKET_NUMBER);
	}
	dv_in->vsif_num = ret;
	dv_in->vsif = vsif_array;

	if (IS_SRCCAP_DV_STUB_MODE_EMP()) {
		/* stub mode test */
		ret = mtk_dv_descrb_sutb_get_test_emp(
			(void *)emp_array, sizeof(struct hdmi_emp_packet_info)
			*SRCCAP_MAX_EMP_PACKET_NUMBER);
	} else {
		ret = mtk_srccap_hdmirx_pkt_get_VS_EMP(srccap_dev->srccap_info.src,
			emp_array, sizeof(struct hdmi_emp_packet_info)
			* SRCCAP_MAX_EMP_PACKET_NUMBER);
	}
	dv_in->emp_num = ret;
	dv_in->emp = emp_array;

	if (IS_SRCCAP_DV_STUB_MODE_DRM()) {
		/* stub mode test */
		ret = mtk_dv_descrb_stub_get_test_drm(
			(void *)drm_array, sizeof(struct hdmi_hdr_packet_info)
			*MAX_DRM_PACKET_NUMBER);
	} else {
		ret = mtk_srccap_hdmirx_pkt_get_HDRIF(srccap_dev->srccap_info.src,
			drm_array, sizeof(struct hdmi_hdr_packet_info)
			* MAX_DRM_PACKET_NUMBER);
	}
	dv_in->drm_num = ret;
	dv_in->drm = drm_array;

	ret = mtk_dv_dqbuf(dv_in, dv_out);
	if (ret)
		SRCCAP_MSG_RETURN_CHECK(ret);

exit:

	_mtk_dv_dqbuf_free_param(avi, vsif_array, emp_array, drm_array, dv_in, dv_out);

	return ret;
}

/* ============================================================================================== */
/* --------------------------------------- v4l2_ioctl_ops --------------------------------------- */
/* ============================================================================================== */
static int mtk_srccap_s_input(
	struct file *file,
	void *fh,
	unsigned int input)
{
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = srccap_set_input(srccap_dev, input);

	return ret;
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

	if (input == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
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

	if (srccap_dev->avd_info.bIsPassColorsys == TRUE) {
		SRCCAP_AVD_MSG_INFO("PyPASS Color system. Set V4L2_STD_ALL.\n");
		std = V4L2_STD_ALL;
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
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	return mtk_hdmirx_g_audio_channel_status(srccap_dev, audio);
}

static int mtk_srccap_g_dv_timings(
	struct file *file,
	void *fh,
	struct v4l2_dv_timings *timings)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	return mtk_hdmirx_g_dv_timings(srccap_dev, timings);
}

static int mtk_srccap_s_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (s == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]s_selection(%u): type=%u target=%u r=(%d,%d,%d)\n",
		srccap_dev->dev_id,
		s->type, s->target, s->r.top, s->r.width, s->r.height);

	switch (s->target) {
	case V4L2_SEL_TGT_CROP:
		srccap_dev->timingdetect_info.cap_win.left = s->r.left;
		srccap_dev->timingdetect_info.cap_win.top = s->r.top;
		srccap_dev->timingdetect_info.cap_win.width =
			(srccap_dev->timingdetect_info.data.yuv420 == true) ?
			s->r.width / SRCCAP_YUV420_WIDTH_DIVISOR : s->r.width;
		srccap_dev->timingdetect_info.cap_win.height = s->r.height;

		ret = mtk_timingdetect_set_capture_win(srccap_dev,
			srccap_dev->timingdetect_info.cap_win);
		if (ret) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	default:
		SRCCAP_MSG_ERROR("target type not found\n");
		return -EINVAL;
	}

	return 0;
}

static int mtk_srccap_g_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (s == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (s->target) {
	case V4L2_SEL_TGT_CROP:
		s->r.left = srccap_dev->timingdetect_info.cap_win.left;
		s->r.top = srccap_dev->timingdetect_info.cap_win.top;
		s->r.width = srccap_dev->timingdetect_info.cap_win.width;
		s->r.height = srccap_dev->timingdetect_info.cap_win.height;
		break;
	default:
		SRCCAP_MSG_ERROR("target type not found\n");
		return -EINVAL;
	}

	return 0;
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
	struct hdmi_avi_packet_info avi;
	struct srccap_dv_set_fmt_mplane_in dv_in;
	struct srccap_dv_set_fmt_mplane_out dv_out;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (format == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]s_fmt_vid_cap_mplane(%u): %s=%u %s=%u %s='%c''%c''%c''%c'\n",
		srccap_dev->dev_id,
		"width", format->fmt.pix_mp.width,
		"height", format->fmt.pix_mp.height,
		"format",
		format->fmt.pix_mp.pixelformat >> SRCCAP_SHIFT_NUM_24,
		format->fmt.pix_mp.pixelformat >> SRCCAP_SHIFT_NUM_16,
		format->fmt.pix_mp.pixelformat >> SRCCAP_SHIFT_NUM_8,
		format->fmt.pix_mp.pixelformat);

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	/* obtain hdmi color format */
	ret = mtk_hdmirx_store_color_fmt(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* get avi packet info */
	if (IS_SRCCAP_DV_STUB_MODE()) {
		/* stub mode test */
		ret = mtk_dv_descrb_stub_get_test_avi(
			&avi, sizeof(struct hdmi_avi_packet_info));
	} else if (mtk_srccap_common_is_Cfd_stub_mode()) {
		ret = mtk_srccap_common_get_cfd_test_HDMI_pkt_AVI(&avi);
	} else {
		ret = mtk_srccap_hdmirx_pkt_get_AVI(srccap_dev->srccap_info.src,
			&avi, sizeof(struct hdmi_avi_packet_info));
	}

	if (ret == 0) {
		SRCCAP_MSG_DEBUG("[SRCCAPDV] get AVI packet info fail\n");
		srccap_dev->dv_info.descrb_info.pkt_info.color_format =
			SRCCAP_DV_DESCRB_COLOR_FORMAT_UNKNOWN;
	} else {
		SRCCAP_MSG_DEBUG("[SRCCAPDV] get AVI packet info: %x\n", avi.enColorForamt);
		srccap_dev->dv_info.descrb_info.pkt_info.color_format =
			avi.enColorForamt;
	}

	/* dolby set format process start */
	memset(&dv_in, 0, sizeof(struct srccap_dv_set_fmt_mplane_in));
	memset(&dv_out, 0, sizeof(struct srccap_dv_set_fmt_mplane_out));
	dv_in.dev = srccap_dev;
	dv_in.fmt = format;
	dv_in.frame_num = SRCCAP_MEMOUT_FRAME_NUM;
	dv_in.rw_diff = SRCCAP_MEMOUT_RW_DIFF;
	ret = mtk_dv_set_fmt_mplane(&dv_in, &dv_out);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);
	/* dolby set format process end */
#endif

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

	srccap_ctx = srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (req_buf == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]reqbufs(%u): count=%u type=%u memory=%u\n",
		srccap_dev->dev_id,
		req_buf->count, req_buf->type, req_buf->memory);

	ret = v4l2_m2m_reqbufs(file, srccap_ctx->fh.m2m_ctx, req_buf);
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

	srccap_ctx = srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (buf == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]querybuf(%u): index=%u type=%u\n",
		srccap_dev->dev_id,
		buf->index, buf->type);

	ret =  v4l2_m2m_querybuf(file, srccap_ctx->fh.m2m_ctx, buf);
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

	srccap_ctx = srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (buf == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	SRCCAP_MSG_DEBUG("[CTRL_FLOW]qbuf(%u): index=%u type=%u memory=%u length=%u",
		srccap_dev->dev_id,
		buf->index, buf->type, buf->memory, buf->length);
	for (plane_cnt = 0; plane_cnt < buf->length; plane_cnt++)
		SRCCAP_MSG_DEBUG(" fd[%d]=%d", plane_cnt, buf->m.planes[plane_cnt].m.fd);
	SRCCAP_MSG_DEBUG("\n");

#if IS_ENABLED(CONFIG_OPTEE)
	/* config buffer security */
	if (V4L2_TYPE_IS_MULTIPLANAR(buf->type)) {
		for (plane_cnt = 0; plane_cnt < (buf->length - 1); plane_cnt++) {
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
	} else {
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
	}
#endif

	ret = v4l2_m2m_qbuf(file, srccap_ctx->fh.m2m_ctx, buf);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
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
	int plane_cnt = 0;

	srccap_ctx = srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (buf == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	SRCCAP_MSG_DEBUG("[CTRL_FLOW]dqbuf(%u): type=%u memory=%u length=%u\n",
		srccap_dev->dev_id,
		buf->type, buf->memory, buf->length);

	ret = v4l2_m2m_dqbuf(file, srccap_ctx->fh.m2m_ctx, buf);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	SRCCAP_MSG_DEBUG("[CTRL_FLOW]m2m_dqbuf(%u): index=%u", srccap_dev->dev_id, buf->index);
	for (plane_cnt = 0; plane_cnt < buf->length; plane_cnt++)
		SRCCAP_MSG_DEBUG(" fd[%d]=%d", plane_cnt, buf->m.planes[plane_cnt].m.fd);
	SRCCAP_MSG_DEBUG("\n");

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	ret = _mtk_dv_dqbuf(srccap_dev, buf);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);
#endif

	srccap_set_dscl(srccap_dev);

	return mtk_memout_dqbuf(srccap_dev, buf);
}

static int mtk_srccap_streamon(
	struct file *file,
	void *fh,
	enum v4l2_buf_type buf_type)
{
	int ret = 0;
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;
	struct srccap_dv_streamon_in dv_in;
	struct srccap_dv_streamon_out dv_out;
	enum srccap_common_irq_event event = SRCCAP_COMMON_IRQEVENT_MAX;

	srccap_ctx = srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]streamon(%u): type=%u\n", srccap_dev->dev_id, buf_type);

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	/* dolby stream on process start */
	memset(&dv_in, 0, sizeof(struct srccap_dv_streamon_in));
	memset(&dv_out, 0, sizeof(struct srccap_dv_streamon_out));
	dv_in.dev = srccap_dev;
	dv_in.buf_type = buf_type;

	/* resource management for dv dma */
	if (srccap_is_dvdma_available) {
		dv_in.dma_available = TRUE;
		srccap_is_dvdma_available = FALSE;
	}

	ret = mtk_dv_streamon(&dv_in, &dv_out);
	if (ret)
		SRCCAP_MSG_RETURN_CHECK(ret);

	if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_0)
		event = SRCCAP_COMMON_IRQEVENT_DV_HW5_IP_INT0;
	else if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_1)
		event = SRCCAP_COMMON_IRQEVENT_DV_HW5_IP_INT1;
	else {
		SRCCAP_MSG_ERROR("failed to subscribe dv irq event (dev_id=%x)\n",
			srccap_dev->dev_id);
		return -EINVAL;
	}
	ret = mtk_common_attach_irq(srccap_dev, SRCCAP_COMMON_IRQTYPE_HK, event,
			mtk_dv_handle_irq,
			(void *)srccap_dev);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	/* dolby stream on process end */
#endif

	ret = srccap_create_buffer_handling_task(srccap_ctx);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_memout_streamon(srccap_dev, buf_type);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_SWMODE) {
		if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_0)
			event = SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC0;
		else if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_1)
			event = SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC1;
		ret = mtk_common_attach_irq(srccap_dev, SRCCAP_COMMON_IRQTYPE_HK, event,
			mtk_memout_vsync_isr,
			(void *)srccap_dev);
		if (ret) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}

	ret = mtk_timingdetect_steamon(srccap_dev);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = srccap_set_capture_win(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	mtk_srccap_common_streamon(srccap_dev);

	srccap_dev->streaming = true;

	return v4l2_m2m_streamon(file, srccap_ctx->fh.m2m_ctx, buf_type);
}

static int mtk_srccap_streamoff(
	struct file *file,
	void *fh,
	enum v4l2_buf_type buf_type)
{
	int ret = 0;
	struct mtk_srccap_ctx *srccap_ctx = NULL;
	struct mtk_srccap_dev *srccap_dev = NULL;
	struct srccap_dv_streamoff_in dv_in;
	struct srccap_dv_streamoff_out dv_out;
	enum srccap_common_irq_event event = SRCCAP_COMMON_IRQEVENT_MAX;

	srccap_ctx = srccap_fh_to_ctx(fh);
	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

	if (srccap_ctx == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]streamoff(%u): type=%u\n", srccap_dev->dev_id, buf_type);

	ret = v4l2_m2m_streamoff(file, srccap_ctx->fh.m2m_ctx, buf_type);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	srccap_dev->streaming = false;

	ret = mtk_dscl_streamoff(srccap_dev);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_avd_streamoff(srccap_dev);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	mtk_srccap_common_stream_off(srccap_dev);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_timingdetect_streamoff(srccap_dev);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	/* dolby stream off process start */
	if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_0)
		event = SRCCAP_COMMON_IRQEVENT_DV_HW5_IP_INT0;
	else if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_1)
		event = SRCCAP_COMMON_IRQEVENT_DV_HW5_IP_INT1;
	else {
		SRCCAP_MSG_ERROR("failed to subscribe dv irq event (dev_id=%x)\n",
			srccap_dev->dev_id);
		return -EINVAL;
	}
	ret = mtk_common_detach_irq(srccap_dev, SRCCAP_COMMON_IRQTYPE_HK, event,
			mtk_dv_handle_irq,
			(void *)srccap_dev);
	if (ret)
		SRCCAP_MSG_RETURN_CHECK(ret);

	memset(&dv_in, 0, sizeof(struct srccap_dv_streamoff_in));
	memset(&dv_out, 0, sizeof(struct srccap_dv_streamoff_out));
	dv_in.dev = srccap_dev;
	dv_in.buf_type = buf_type;
	ret = mtk_dv_streamoff(&dv_in, &dv_out);
	if (ret)
		SRCCAP_MSG_RETURN_CHECK(ret);

	/* resource management for dv dma */
	if (srccap_dev->dv_info.dma_info.buf.available) {
		srccap_dev->dv_info.dma_info.buf.available = FALSE;
		srccap_is_dvdma_available = TRUE;
	}
	/* dolby stream off process end */
#endif

	ret = srccap_destroy_buffer_handling_task(srccap_ctx);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_SWMODE) {
		if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_0)
			event = SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC0;
		else if (srccap_dev->dev_id == SRCCAP_COMMON_DEVICE_1)
			event = SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC1;
		ret = mtk_common_detach_irq(srccap_dev, SRCCAP_COMMON_IRQTYPE_HK, event,
			mtk_memout_vsync_isr,
			(void *)srccap_dev);
	}

	return mtk_memout_streamoff(srccap_dev, buf_type);
}

static int sync_monitor_task(void *data)
{
	int ret = 0;

	if (data == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;

	while (!kthread_should_stop()) {
		ret = srccap_handle_src_sync_status(srccap_dev);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			continue;
		}

		usleep_range(SRCCAP_UDELAY_1000, SRCCAP_UDELAY_1100); // sleep 1ms
	} //while (!kthread_should_stop())

	ret = mtk_adc_enable_offline_sog(srccap_dev, FALSE);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	usleep_range(SRCCAP_UDELAY_1000, SRCCAP_UDELAY_1100); // sleep 1ms

	SRCCAP_MSG_INFO("%s has been stopped.\n", __func__);

	return 0;
}

static int timing_monitor_task(void *data)
{
	if (data == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = (struct mtk_srccap_dev *)data;
	u16 dev_id = srccap_dev->dev_id;
	enum v4l2_srccap_input_source src = srccap_dev->srccap_info.src;
	struct v4l2_event event;
	enum v4l2_timingdetect_status status = V4L2_TIMINGDETECT_NO_SIGNAL;
	static enum v4l2_timingdetect_status pre_avd_status[SRCCAP_DEV_NUM];
	enum v4l2_timingdetect_status avd_status;
	enum srccap_timingdetect_vrr_type vrr_type = 0;
	struct v4l2_ext_avd_info avd_info;
	struct v4l2_timing stTimingInfo;
	char event_name[SRCCAP_STRING_SIZE_50] = {0};
	bool hdmi_status = 0;

	memset(&event, 0, sizeof(struct v4l2_event));
	memset(&avd_status, 0, sizeof(enum v4l2_timingdetect_status));
	memset(&stTimingInfo, 0, sizeof(struct v4l2_timing));

	while (!kthread_should_stop()) {
		switch (src) {
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
		case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		case V4L2_SRCCAP_INPUT_SOURCE_DVI:
		case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
		case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
		case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
			ret = mtk_timingdetect_get_signal_status(srccap_dev, &vrr_type,
									FALSE, &status);
			//reserve first to add hdmi_free_sync check
			//enum v4l2_srccap_freesync_type enfstype;
			//ret = mtk_timingdetect_get_signal_status(srccap_dev, &vrr_type,
			//mtk_srccap_hdmirx_get_freesync_info(srccap_dev->srccap_info.src,
			//&enfstype), &status);

			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				continue;
			}

			if (srccap_dev->srccap_info.stub_mode == 0) {
				if (status == V4L2_TIMINGDETECT_STABLE_SYNC) {
					hdmi_status = mtk_srccap_hdmirx_get_HDMIStatus(src);
					if (!hdmi_status)
						status = V4L2_TIMINGDETECT_UNSTABLE_SYNC;
				}
			}
			break;
		case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
		case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
		case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		case V4L2_SRCCAP_INPUT_SOURCE_VGA:
		case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
		case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
			ret = mtk_timingdetect_get_signal_status(srccap_dev, &vrr_type,
									FALSE, &status);
			//reserve first to add hdmi_free_sync check
			//enum v4l2_srccap_freesync_type enfstype;
			//ret = mtk_timingdetect_get_signal_status(srccap_dev, &vrr_type,
			//mtk_srccap_hdmirx_get_freesync_info(srccap_dev->srccap_info.src,
			//&enfstype), &status);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				continue;
			}
			break;
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
		case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
		case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
		case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
		case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		case V4L2_SRCCAP_INPUT_SOURCE_SCART:
		case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
			srccap_dev->avd_info.bIsMonitorTaskWorking = TRUE;
			memset(&avd_info, 0, sizeof(struct v4l2_ext_avd_info));
			ret = mtk_avd_info(&avd_info);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}

			if (srccap_dev->avd_info.bIsATVScanMode !=
				(bool)avd_info.u8AutoTuningIsProgress)
				SRCCAP_AVD_MSG_INFO("bIsATVScanMode = %d\n",
				avd_info.u8AutoTuningIsProgress);

			srccap_dev->avd_info.bIsATVScanMode = (bool)avd_info.u8AutoTuningIsProgress;

			memset(&avd_status, 0, sizeof(enum v4l2_timingdetect_status));
			ret = mtk_avd_SourceSignalMonitor(&avd_status, srccap_dev);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				continue;
			}
			if (avd_status != pre_avd_status[dev_id]) {
				SRCCAP_AVD_MSG_INFO("[Notify]%s\n",
					SignalStatusToString(avd_status));
				srccap_dev->avd_info.enVdSignalStatus = avd_status;
				status = avd_status;
			}
			pre_avd_status[dev_id] = avd_status;
			break;
		default:
			return -EPERM;
		}

		/* handle status change */
		if ((status != srccap_dev->timingdetect_info.status)
			|| (vrr_type != srccap_dev->timingdetect_info.data.vrr_type)) {
			ret = srccap_handle_status_change_event(srccap_dev, vrr_type, status);
			if (ret < 0) {
				/* timing not found in timing table */
				if (ret == -ERANGE) {
					status = V4L2_TIMINGDETECT_UNSTABLE_SYNC;
					continue;
				}
				SRCCAP_MSG_RETURN_CHECK(ret);
				continue;
			}

			/* save timing status */
			srccap_dev->timingdetect_info.status = status;

			/* set event */
			if (!video_is_registered(srccap_dev->vdev)) {
				SRCCAP_MSG_ERROR("video device not registered\n");
				continue;
			}
			memset(&event, 0, sizeof(struct v4l2_event));
			event.type = V4L2_EVENT_SIGNAL_STATUS_CHANGE;
			event.id = 0;
			v4l2_event_queue(srccap_dev->vdev, &event);

			ret = srccap_get_event_string(event.type, SRCCAP_STRING_SIZE_50,
				event_name);
			if (ret < 0) {
				SRCCAP_MSG_ERROR("cannot get event string\n");
				return ret;
			}

			SRCCAP_MSG_INFO("[CTRL_FLOW]queue_event(%u): type=%s, status=%d\n",
				srccap_dev->dev_id,
				event_name,
				status);
		}
		/* save timing status */
		srccap_dev->timingdetect_info.status = status;

		usleep_range(8000, 8100); // sleep 8ms
	}

	srccap_dev->timingdetect_info.status = V4L2_TIMINGDETECT_NO_SIGNAL;
	memset(&srccap_dev->timingdetect_info.data,
		0, sizeof(struct srccap_timingdetect_data));
	srccap_dev->avd_info.bIsMonitorTaskWorking = FALSE;
	SRCCAP_MSG_INFO("%s has been stopped.\n", __func__);

	return 0;
}

static int mtk_srccap_subscribe_event_signal_status(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if (srccap_dev->srccap_info.timing_monitor_task == NULL) {
		srccap_dev->srccap_info.timing_monitor_task = kthread_create(
			timing_monitor_task,
			srccap_dev,
			"signal status monitor");
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}
		get_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		wake_up_process(srccap_dev->srccap_info.timing_monitor_task);
	}

	srccap_dev->srccap_info.sts.signalstatus_chg = TRUE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_subscribe_event_resolution_change(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if (srccap_dev->srccap_info.timing_monitor_task == NULL) {
		srccap_dev->srccap_info.timing_monitor_task = kthread_create(
			timing_monitor_task,
			srccap_dev,
			"resolution change monitor");
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}
		get_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		wake_up_process(srccap_dev->srccap_info.timing_monitor_task);
	}

	srccap_dev->srccap_info.sts.resolution_chg = TRUE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_subscribe_event_framerate_change(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if (srccap_dev->srccap_info.timing_monitor_task == NULL) {
		srccap_dev->srccap_info.timing_monitor_task = kthread_create(
			timing_monitor_task,
			srccap_dev,
			"framerate change monitor");
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}
		get_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		wake_up_process(srccap_dev->srccap_info.timing_monitor_task);
	}

	srccap_dev->srccap_info.sts.framerate_chg = TRUE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_subscribe_event_scantype_change(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if (srccap_dev->srccap_info.timing_monitor_task == NULL) {
		srccap_dev->srccap_info.timing_monitor_task = kthread_create(
			timing_monitor_task,
			srccap_dev,
			"scantype change monitor");
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}
		get_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		wake_up_process(srccap_dev->srccap_info.timing_monitor_task);
	}

	srccap_dev->srccap_info.sts.scantype_chg = TRUE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_subscribe_event_colorformat_change(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if (srccap_dev->srccap_info.timing_monitor_task == NULL) {
		srccap_dev->srccap_info.timing_monitor_task = kthread_create(
			timing_monitor_task,
			srccap_dev,
			"colorformat change monitor");
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}
		get_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		wake_up_process(srccap_dev->srccap_info.timing_monitor_task);
	}

	srccap_dev->srccap_info.sts.colorformat_chg = TRUE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_subscribe_event_sync_status_change(
	struct mtk_srccap_dev *srccap_dev,
	u32 event_type)
{
	enum v4l2_srccap_input_source query_src = V4L2_SRCCAP_INPUT_SOURCE_NONE;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (event_type) {
	case V4L2_EVENT_HDMI_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_HDMI;
		break;
	case V4L2_EVENT_HDMI2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_HDMI2;
		break;
	case V4L2_EVENT_HDMI3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_HDMI3;
		break;
	case V4L2_EVENT_HDMI4_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_HDMI4;
		break;
	case V4L2_EVENT_CVBS_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
		break;
	case V4L2_EVENT_CVBS2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS2;
		break;
	case V4L2_EVENT_CVBS3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS3;
		break;
	case V4L2_EVENT_CVBS4_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS4;
		break;
	case V4L2_EVENT_CVBS5_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS5;
		break;
	case V4L2_EVENT_CVBS6_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS6;
		break;
	case V4L2_EVENT_CVBS7_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS7;
		break;
	case V4L2_EVENT_CVBS8_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS8;
		break;
	case V4L2_EVENT_SVIDEO_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO;
		break;
	case V4L2_EVENT_SVIDEO2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2;
		break;
	case V4L2_EVENT_SVIDEO3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3;
		break;
	case V4L2_EVENT_SVIDEO4_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4;
		break;
	case V4L2_EVENT_YPBPR_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
		break;
	case V4L2_EVENT_YPBPR2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_YPBPR2;
		break;
	case V4L2_EVENT_YPBPR3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_YPBPR3;
		break;
	case V4L2_EVENT_VGA_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_VGA;
		break;
	case V4L2_EVENT_VGA2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_VGA2;
		break;
	case V4L2_EVENT_VGA3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_VGA3;
		break;
	case V4L2_EVENT_SCART_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SCART;
		break;
	case V4L2_EVENT_SCART2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SCART2;
		break;
	default:
		return -EINVAL;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_sync_monitor);

	if (srccap_dev->srccap_info.sync_monitor_task == NULL) {
		srccap_dev->srccap_info.sync_monitor_task = kthread_create(
			sync_monitor_task,
			srccap_dev,
			"sync status monitor");
		if (srccap_dev->srccap_info.sync_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_sync_monitor);
			return -ENOMEM;
		}
		get_task_struct(srccap_dev->srccap_info.sync_monitor_task);
		wake_up_process(srccap_dev->srccap_info.sync_monitor_task);
	}

	srccap_dev->srccap_info.sts.sync_status_event[query_src] = TRUE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_sync_monitor);

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

	char event[SRCCAP_STRING_SIZE_50] = {0};

	srccap_dev = video_get_drvdata(fh->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (event_sub == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	ret = srccap_get_event_string(event_sub->type, SRCCAP_STRING_SIZE_50, event);
	if (ret < 0) {
		SRCCAP_MSG_ERROR("cannot get event string\n");
		return ret;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]subscribe_event(%u): type=%s id=%u\n",
		srccap_dev->dev_id,
		event,
		event_sub->id);

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
	case V4L2_EVENT_RESOLUTION_CHANGE:
		ret = mtk_srccap_subscribe_event_resolution_change(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to subscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_FRAMERATE_CHANGE:
		ret = mtk_srccap_subscribe_event_framerate_change(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to subscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_SCANTYPE_CHANGE:
		ret = mtk_srccap_subscribe_event_scantype_change(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to subscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_COLORFORMAT_CHANGE:
		ret = mtk_srccap_subscribe_event_colorformat_change(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to subscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_HDMI_SIGNAL_CHANGE:
	case V4L2_EVENT_HDMI2_SIGNAL_CHANGE:
	case V4L2_EVENT_HDMI3_SIGNAL_CHANGE:
	case V4L2_EVENT_HDMI4_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS2_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS3_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS4_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS5_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS6_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS7_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS8_SIGNAL_CHANGE:
	case V4L2_EVENT_SVIDEO_SIGNAL_CHANGE:
	case V4L2_EVENT_SVIDEO2_SIGNAL_CHANGE:
	case V4L2_EVENT_SVIDEO3_SIGNAL_CHANGE:
	case V4L2_EVENT_SVIDEO4_SIGNAL_CHANGE:
	case V4L2_EVENT_YPBPR_SIGNAL_CHANGE:
	case V4L2_EVENT_YPBPR2_SIGNAL_CHANGE:
	case V4L2_EVENT_YPBPR3_SIGNAL_CHANGE:
	case V4L2_EVENT_VGA_SIGNAL_CHANGE:
	case V4L2_EVENT_VGA2_SIGNAL_CHANGE:
	case V4L2_EVENT_VGA3_SIGNAL_CHANGE:
	case V4L2_EVENT_SCART_SIGNAL_CHANGE:
	case V4L2_EVENT_SCART2_SIGNAL_CHANGE:
		ret = mtk_srccap_subscribe_event_sync_status_change(srccap_dev, event_sub->type);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to subscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_SRCCAP_SUSPEND_EVENT:
		SRCCAP_MSG_INFO("[SRCCAP]V4L2_SRCCAP_SUSPEND_EVENT");
		break;
	case V4L2_SRCCAP_RESUME_EVENT:
		SRCCAP_MSG_INFO("[SRCCAP]V4L2_SRCCAP_RESUME_EVENT");
		break;
	default:
		return -EINVAL;
	}

	return v4l2_event_subscribe(fh, event_sub, SRCCAP_EVENTQ_SIZE, NULL);
}

static int mtk_srccap_unsubscribe_event_signal_status(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if ((srccap_dev->srccap_info.timing_monitor_task != NULL) &&
		(srccap_dev->srccap_info.sts.resolution_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.framerate_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.scantype_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.colorformat_chg == FALSE)) {
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}

		kthread_stop(srccap_dev->srccap_info.timing_monitor_task);
		put_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		srccap_dev->srccap_info.timing_monitor_task = NULL;
	}

	srccap_dev->srccap_info.sts.signalstatus_chg = FALSE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_unsubscribe_event_resolution_change(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if ((srccap_dev->srccap_info.timing_monitor_task != NULL) &&
		(srccap_dev->srccap_info.sts.signalstatus_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.framerate_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.scantype_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.colorformat_chg == FALSE)) {
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}

		kthread_stop(srccap_dev->srccap_info.timing_monitor_task);
		put_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		srccap_dev->srccap_info.timing_monitor_task = NULL;
	}

	srccap_dev->srccap_info.sts.resolution_chg = FALSE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_unsubscribe_event_framerate_change(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if ((srccap_dev->srccap_info.timing_monitor_task != NULL) &&
		(srccap_dev->srccap_info.sts.signalstatus_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.resolution_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.scantype_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.colorformat_chg == FALSE)) {
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}

		kthread_stop(srccap_dev->srccap_info.timing_monitor_task);
		put_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		srccap_dev->srccap_info.timing_monitor_task = NULL;
	}

	srccap_dev->srccap_info.sts.framerate_chg = FALSE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_unsubscribe_event_scantype_change(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if ((srccap_dev->srccap_info.timing_monitor_task != NULL) &&
		(srccap_dev->srccap_info.sts.signalstatus_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.resolution_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.framerate_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.colorformat_chg == FALSE)) {
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}

		kthread_stop(srccap_dev->srccap_info.timing_monitor_task);
		put_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		srccap_dev->srccap_info.timing_monitor_task = NULL;
	}

	srccap_dev->srccap_info.sts.scantype_chg = FALSE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_unsubscribe_event_colorformat_change(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	if ((srccap_dev->srccap_info.timing_monitor_task != NULL) &&
		(srccap_dev->srccap_info.sts.signalstatus_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.resolution_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.framerate_chg == FALSE) &&
		(srccap_dev->srccap_info.sts.scantype_chg == FALSE)) {
		if (srccap_dev->srccap_info.timing_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
			return -ENOMEM;
		}

		kthread_stop(srccap_dev->srccap_info.timing_monitor_task);
		put_task_struct(srccap_dev->srccap_info.timing_monitor_task);
		srccap_dev->srccap_info.timing_monitor_task = NULL;
	}

	srccap_dev->srccap_info.sts.colorformat_chg = FALSE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);

	return 0;
}

static int mtk_srccap_unsubscribe_event_sync_status_change(
	struct mtk_srccap_dev *srccap_dev,
	u32 event_type)
{
	enum v4l2_srccap_input_source sync_src_index = 0;
	enum v4l2_srccap_input_source query_src = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (event_type) {
	case V4L2_EVENT_HDMI_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_HDMI;
		break;
	case V4L2_EVENT_HDMI2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_HDMI2;
		break;
	case V4L2_EVENT_HDMI3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_HDMI3;
		break;
	case V4L2_EVENT_HDMI4_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_HDMI4;
		break;
	case V4L2_EVENT_CVBS_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
		break;
	case V4L2_EVENT_CVBS2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS2;
		break;
	case V4L2_EVENT_CVBS3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS3;
		break;
	case V4L2_EVENT_CVBS4_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS4;
		break;
	case V4L2_EVENT_CVBS5_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS5;
		break;
	case V4L2_EVENT_CVBS6_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS6;
		break;
	case V4L2_EVENT_CVBS7_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS7;
		break;
	case V4L2_EVENT_CVBS8_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_CVBS8;
		break;
	case V4L2_EVENT_SVIDEO_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO;
		break;
	case V4L2_EVENT_SVIDEO2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2;
		break;
	case V4L2_EVENT_SVIDEO3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3;
		break;
	case V4L2_EVENT_SVIDEO4_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4;
		break;
	case V4L2_EVENT_YPBPR_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
		break;
	case V4L2_EVENT_YPBPR2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_YPBPR2;
		break;
	case V4L2_EVENT_YPBPR3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_YPBPR3;
		break;
	case V4L2_EVENT_VGA_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_VGA;
		break;
	case V4L2_EVENT_VGA2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_VGA2;
		break;
	case V4L2_EVENT_VGA3_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_VGA3;
		break;
	case V4L2_EVENT_SCART_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SCART;
		break;
	case V4L2_EVENT_SCART2_SIGNAL_CHANGE:
		query_src = V4L2_SRCCAP_INPUT_SOURCE_SCART2;
		break;
	default:
		return -EINVAL;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_sync_monitor);

	if (srccap_dev->srccap_info.sync_monitor_task != NULL) {
		for (sync_src_index = 0;
		sync_src_index < V4L2_SRCCAP_INPUT_SOURCE_NUM;
		sync_src_index++) {
			if (sync_src_index == query_src)
				continue;
			if (srccap_dev->srccap_info.sts.sync_status_event[sync_src_index] == TRUE)
				goto exit;
		}
		if (srccap_dev->srccap_info.sync_monitor_task == ERR_PTR(-ENOMEM)) {
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_sync_monitor);
			return -ENOMEM;
		}

		kthread_stop(srccap_dev->srccap_info.sync_monitor_task);
		put_task_struct(srccap_dev->srccap_info.sync_monitor_task);
		srccap_dev->srccap_info.sync_monitor_task = NULL;
	}
exit:
	srccap_dev->srccap_info.sts.sync_status_event[query_src] = FALSE;

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_sync_monitor);

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
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;
	enum v4l2_srccap_input_source query_src = V4L2_SRCCAP_INPUT_SOURCE_NONE;
	char event[SRCCAP_STRING_SIZE_50] = {0};

	srccap_dev = video_get_drvdata(fh->vdev);

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (event_sub == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	ret = srccap_get_event_string(event_sub->type, SRCCAP_STRING_SIZE_50, event);
	if (ret < 0) {
		SRCCAP_MSG_ERROR("cannot get event string\n");
		return ret;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]unsubscribe_event(%u): type=%s id=%u\n",
		srccap_dev->dev_id,
		event,
		event_sub->id);

	switch (event_sub->type) {
	case V4L2_EVENT_VSYNC:
		ret = mtk_common_unsubscribe_event_vsync(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_CTRL:
		ret = mtk_srccap_unsubscribe_event_ctrl(srccap_dev, event_sub);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_SIGNAL_STATUS_CHANGE:
		ret = mtk_srccap_unsubscribe_event_signal_status(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_RESOLUTION_CHANGE:
		ret = mtk_srccap_unsubscribe_event_resolution_change(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_FRAMERATE_CHANGE:
		ret = mtk_srccap_unsubscribe_event_framerate_change(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_SCANTYPE_CHANGE:
		ret = mtk_srccap_unsubscribe_event_scantype_change(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_COLORFORMAT_CHANGE:
		ret = mtk_srccap_unsubscribe_event_colorformat_change(srccap_dev);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_HDMI_SIGNAL_CHANGE:
	case V4L2_EVENT_HDMI2_SIGNAL_CHANGE:
	case V4L2_EVENT_HDMI3_SIGNAL_CHANGE:
	case V4L2_EVENT_HDMI4_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS2_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS3_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS4_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS5_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS6_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS7_SIGNAL_CHANGE:
	case V4L2_EVENT_CVBS8_SIGNAL_CHANGE:
	case V4L2_EVENT_SVIDEO_SIGNAL_CHANGE:
	case V4L2_EVENT_SVIDEO2_SIGNAL_CHANGE:
	case V4L2_EVENT_SVIDEO3_SIGNAL_CHANGE:
	case V4L2_EVENT_SVIDEO4_SIGNAL_CHANGE:
	case V4L2_EVENT_YPBPR_SIGNAL_CHANGE:
	case V4L2_EVENT_YPBPR2_SIGNAL_CHANGE:
	case V4L2_EVENT_YPBPR3_SIGNAL_CHANGE:
	case V4L2_EVENT_VGA_SIGNAL_CHANGE:
	case V4L2_EVENT_VGA2_SIGNAL_CHANGE:
	case V4L2_EVENT_VGA3_SIGNAL_CHANGE:
	case V4L2_EVENT_SCART_SIGNAL_CHANGE:
	case V4L2_EVENT_SCART2_SIGNAL_CHANGE:
		ret = mtk_srccap_unsubscribe_event_sync_status_change(srccap_dev, event_sub->type);
		if (ret < 0) {
			v4l2_err(&srccap_dev->v4l2_dev,
				"failed to unsubscribe event %d\n",
				event_sub->type);
			return -EPERM;
		}
		break;
	case V4L2_SRCCAP_SUSPEND_EVENT:
		SRCCAP_MSG_INFO("[SRCCAP] Release V4L2_SRCCAP_SUSPEND_EVENT");
		break;
	case V4L2_SRCCAP_RESUME_EVENT:
		SRCCAP_MSG_INFO("[SRCCAP] Release V4L2_SRCCAP_RESUME_EVENT");
		break;
	default:
		return -EINVAL;
	}

	return v4l2_event_unsubscribe(fh, event_sub);
}

static int mtk_srccap_query_cap(struct file *file, void *fh,
	struct v4l2_capability *cap)
{
	struct mtk_srccap_dev *srccap_dev = NULL;

	srccap_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW]query_cap(%u)\n", srccap_dev->dev_id);

	if (srccap_dev->dev_id == 0) {
		strncpy(cap->driver, SRCCAP_DRIVER0_NAME, sizeof(cap->driver)-1);
		strncpy(cap->card, SRCCAP_DRIVER0_NAME, sizeof(cap->card)-1);
	} else {
		strncpy(cap->driver, SRCCAP_DRIVER1_NAME, sizeof(cap->driver)-1);
		strncpy(cap->card, SRCCAP_DRIVER1_NAME, sizeof(cap->card)-1);
	}
	SRCCAP_MSG_INFO("[CTRL_FLOW]driver:%s\n", cap->driver);
	cap->bus_info[0] = 0;
	cap->device_caps = SRCCAP_DEVICE_CAPS;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}


static const struct v4l2_ioctl_ops mtk_srccap_ioctl_ops = {
	.vidioc_s_input				= mtk_srccap_s_input,
	.vidioc_g_input				= mtk_srccap_g_input,
	.vidioc_g_edid				= mtk_srccap_g_edid,
	.vidioc_s_edid				= mtk_srccap_s_edid,
	.vidioc_g_std				= mtk_scrcap_g_std,
	.vidioc_s_std				= mtk_scrcap_s_std,
	.vidioc_g_audio				= mtk_srccap_g_audio,
	.vidioc_g_dv_timings			= mtk_srccap_g_dv_timings,
	.vidioc_s_selection			= mtk_srccap_s_selection,
	.vidioc_g_selection			= mtk_srccap_g_selection,
	.vidioc_s_fmt_vid_cap			= mtk_srccap_s_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap_mplane		= mtk_srccap_s_fmt_vid_cap_mplane,
	.vidioc_g_fmt_vid_cap			= mtk_srccap_g_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap_mplane		= mtk_srccap_g_fmt_vid_cap_mplane,
	.vidioc_reqbufs				= mtk_srccap_reqbufs,
	.vidioc_querybuf			= mtk_srccap_querybuf,
	.vidioc_qbuf				= mtk_srccap_qbuf,
	.vidioc_dqbuf				= mtk_srccap_dqbuf,
	.vidioc_streamon			= mtk_srccap_streamon,
	.vidioc_streamoff			= mtk_srccap_streamoff,
	.vidioc_subscribe_event			= mtk_srccap_subscribe_event,
	.vidioc_unsubscribe_event		= mtk_srccap_unsubscribe_event,
	.vidioc_querycap                        = mtk_srccap_query_cap,
};

/* ============================================================================================== */
/* --------------------------------------- v4l2_ctrl_ops ---------------------------------------- */
/* ============================================================================================== */
static int mtk_srccap_set_stub(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_stub_mode stub)
{
	int ret = 0;
	bool stub_hwreg = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (stub == SRCCAP_STUB_MODE_OFF)
		stub_hwreg = FALSE;
	else
		stub_hwreg = TRUE;

	ret = drv_hwreg_common_set_stub(stub_hwreg);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	mtk_srccap_common_set_stub_mode(stub);

	ret = mtk_dv_set_stub(srccap_dev, stub);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	// store information for stub mode
	ret = mtk_timingdetect_set_hw_version(srccap_dev, TRUE);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

static int mtk_srccap_get_stub(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_stub_mode *stub)
{
	int ret = 0;
	bool stub_hwreg = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (stub == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	ret = drv_hwreg_common_get_stub(&stub_hwreg);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	srccap_dev->srccap_info.stub_mode = stub_hwreg;

	if (stub_hwreg == FALSE)
		*stub = SRCCAP_STUB_MODE_OFF;
	else
		*stub = SRCCAP_STUB_MODE_ON_COMMON;

	return 0;
}

static int mtk_srccap_get_sync_status(
	struct mtk_srccap_dev *srccap_dev,
	u32 ctrl_id,
	bool *sync_status)
{
	int ret = 0;
	enum v4l2_srccap_input_source query_src = V4L2_SRCCAP_INPUT_SOURCE_NONE;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (sync_status == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (srccap_dev->srccap_info.sync_monitor_task == NULL)
		return -EINVAL;

	ret = srccap_get_inputsrc_type(ctrl_id, &query_src);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	*sync_status = srccap_dev->srccap_info.sync_status[query_src];

	return 0;
}

static int mtk_srccap_set_buffer_handling_trigger(
	struct mtk_srccap_dev *srccap_dev)
{
	spinlock_t *spinlock_buffer_handling_task = NULL;
	unsigned long flags = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	spinlock_buffer_handling_task =
		&srccap_dev->srccap_info.spinlock_list.spinlock_buffer_handling_task;

	spin_lock_irqsave(spinlock_buffer_handling_task, flags);
	if (srccap_dev->srccap_info.buffer_handling_task != NULL) {
		srccap_dev->srccap_info.waitq_list.buffer_done = 1;
		wake_up_interruptible(&srccap_dev->srccap_info.waitq_list.waitq_buffer);
	}
	spin_unlock_irqrestore(spinlock_buffer_handling_task, flags);

	return 0;
}

static int mtk_srccap_g_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, srccap_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_SRCCAP_STUB:
	{
		enum srccap_stub_mode stub;

		SRCCAP_MSG_DEBUG("V4L2_CID_SRCCAP_STUB\n");
		memset(&stub, 0, sizeof(enum srccap_stub_mode));
		ret = mtk_srccap_get_stub(srccap_dev, &stub);
		memcpy((void *)ctrl->p_new.p_u8, &stub, sizeof(u8));
		break;
	}
	case V4L2_CID_SRCCAP_HDMI_SYNC_STATUS:
	case V4L2_CID_SRCCAP_HDMI2_SYNC_STATUS:
	case V4L2_CID_SRCCAP_HDMI3_SYNC_STATUS:
	case V4L2_CID_SRCCAP_HDMI4_SYNC_STATUS:
	case V4L2_CID_SRCCAP_CVBS_SYNC_STATUS:
	case V4L2_CID_SRCCAP_CVBS2_SYNC_STATUS:
	case V4L2_CID_SRCCAP_CVBS3_SYNC_STATUS:
	case V4L2_CID_SRCCAP_CVBS4_SYNC_STATUS:
	case V4L2_CID_SRCCAP_CVBS5_SYNC_STATUS:
	case V4L2_CID_SRCCAP_CVBS6_SYNC_STATUS:
	case V4L2_CID_SRCCAP_CVBS7_SYNC_STATUS:
	case V4L2_CID_SRCCAP_CVBS8_SYNC_STATUS:
	case V4L2_CID_SRCCAP_SVIDEO_SYNC_STATUS:
	case V4L2_CID_SRCCAP_SVIDEO2_SYNC_STATUS:
	case V4L2_CID_SRCCAP_SVIDEO3_SYNC_STATUS:
	case V4L2_CID_SRCCAP_SVIDEO4_SYNC_STATUS:
	case V4L2_CID_SRCCAP_YPBPR_SYNC_STATUS:
	case V4L2_CID_SRCCAP_YPBPR2_SYNC_STATUS:
	case V4L2_CID_SRCCAP_YPBPR3_SYNC_STATUS:
	case V4L2_CID_SRCCAP_VGA_SYNC_STATUS:
	case V4L2_CID_SRCCAP_VGA2_SYNC_STATUS:
	case V4L2_CID_SRCCAP_VGA3_SYNC_STATUS:
	case V4L2_CID_SRCCAP_SCART_SYNC_STATUS:
	case V4L2_CID_SRCCAP_SCART2_SYNC_STATUS:
	{
		bool sync_status = FALSE;

		SRCCAP_MSG_DEBUG("V4L2_CID_SRCCAP_SYNC_STATUS\n");
		ret = mtk_srccap_get_sync_status(srccap_dev, ctrl->id, &sync_status);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memcpy((void *)ctrl->p_new.p_u8, &sync_status, sizeof(u8));
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int mtk_srccap_s_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, srccap_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_SRCCAP_STUB:
	{
		enum srccap_stub_mode stub = (enum srccap_stub_mode)(ctrl->val);

		SRCCAP_MSG_DEBUG("V4L2_CID_SRCCAP_STUB\n");

		ret = mtk_srccap_set_stub(srccap_dev, stub);
		break;
	}
	case V4L2_CID_SRCCAP_BUFFER_HANDLING_TRIGGER:
	{
		ret = mtk_srccap_set_buffer_handling_trigger(srccap_dev);
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops mtk_srccap_ctrl_ops = {
	.g_volatile_ctrl = mtk_srccap_g_ctrl,
	.s_ctrl = mtk_srccap_s_ctrl,
};

static const struct v4l2_ctrl_config srccap_ctrl[] = {
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_STUB,
		.name = "Srccap Stub",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_HDMI_SYNC_STATUS,
		.name = "Srccap HDMI Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_HDMI2_SYNC_STATUS,
		.name = "Srccap HDMI2 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_HDMI3_SYNC_STATUS,
		.name = "Srccap HDMI3 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_HDMI4_SYNC_STATUS,
		.name = "Srccap HDMI4 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_CVBS_SYNC_STATUS,
		.name = "Srccap CVBS Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_CVBS2_SYNC_STATUS,
		.name = "Srccap CVBS2 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_CVBS3_SYNC_STATUS,
		.name = "Srccap CVBS3 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_CVBS4_SYNC_STATUS,
		.name = "Srccap CVBS4 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_CVBS5_SYNC_STATUS,
		.name = "Srccap CVBS5 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
		{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_CVBS6_SYNC_STATUS,
		.name = "Srccap CVBS6 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_CVBS7_SYNC_STATUS,
		.name = "Srccap CVBS7 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_CVBS8_SYNC_STATUS,
		.name = "Srccap CVBS8 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_SVIDEO_SYNC_STATUS,
		.name = "Srccap SVIDEO Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_SVIDEO2_SYNC_STATUS,
		.name = "Srccap SVIDEO2 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_SVIDEO3_SYNC_STATUS,
		.name = "Srccap SVIDEO3 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_SVIDEO4_SYNC_STATUS,
		.name = "Srccap SVIDEO4 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_YPBPR_SYNC_STATUS,
		.name = "Srccap YPBPR Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_YPBPR2_SYNC_STATUS,
		.name = "Srccap YPBPR2 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_YPBPR3_SYNC_STATUS,
		.name = "Srccap YPBPR3 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_VGA_SYNC_STATUS,
		.name = "Srccap VGA Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_VGA2_SYNC_STATUS,
		.name = "Srccap VGA2 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_VGA3_SYNC_STATUS,
		.name = "Srccap VGA3 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_SCART_SYNC_STATUS,
		.name = "Srccap SCART Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_SCART2_SYNC_STATUS,
		.name = "Srccap SCART2 Sync Status",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xf,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &mtk_srccap_ctrl_ops,
		.id = V4L2_CID_SRCCAP_BUFFER_HANDLING_TRIGGER,
		.name = "Srccap Buffer Handling Trigger",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.def = 0,
		.min = 0,
		.max = 1,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* ============================================================================================== */
/* ------------------------------------------ vb2_ops ------------------------------------------- */
/* ============================================================================================== */
static int srccap_queue_setup(
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
			if (srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_SWMODE) {
#ifdef M6L_SW_MODE_EN
				if (srccap_dev->srccap_info.cap.hw_version == 1) /* index control */
#endif
					sizes[0] += srccap_dev->memout_info.frame_pitch[i]
						* SRCCAP_MEMOUT_FRAME_NUM;
#ifdef M6L_SW_MODE_EN
				else /* address control */
					sizes[0] += srccap_dev->memout_info.frame_pitch[i];
#endif
			} else if (srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_HWMODE)
				sizes[0] += srccap_dev->memout_info.frame_pitch[i]
					* SRCCAP_MEMOUT_FRAME_NUM;
			else /* srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_BYPASSMODE */
				sizes[0] = 1; // minimum frame buf size
		}
	} else {
		for (i = 0; i < (*num_planes - 1); ++i) {
			if (srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_SWMODE) {
#ifdef M6L_SW_MODE_EN
				if (srccap_dev->srccap_info.cap.hw_version == 1) /* index control */
#endif
					sizes[i] = srccap_dev->memout_info.frame_pitch[i]
						* SRCCAP_MEMOUT_FRAME_NUM;
#ifdef M6L_SW_MODE_EN
				else /* address control */
					sizes[i] = srccap_dev->memout_info.frame_pitch[i];
#endif
			} else if (srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_HWMODE)
				sizes[i] = srccap_dev->memout_info.frame_pitch[i]
					* SRCCAP_MEMOUT_FRAME_NUM;
			else /* srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_BYPASSMODE */
				sizes[i] = 1; // minimum frame buf size
		}
	}
	sizes[*num_planes - 1] = 1; // minimum metadata buf size

	/* fill alloc_devs[] */
	for (i = 0; i < *num_planes; ++i)
		alloc_devs[i] = srccap_dev->dev;

	return 0;
}

static int srccap_buf_init(struct vb2_buffer *vb)
{
	return 0;
}

static int srccap_buf_pepare(struct vb2_buffer *vb)
{
	return 0;
}

static void srccap_buf_finish(struct vb2_buffer *vb)
{
}

static void srccap_buf_cleanup(struct vb2_buffer *vb)
{
}

static int srccap_start_streaming(
	struct vb2_queue *q,
	unsigned int count)
{
	return 0;
}

static void srccap_stop_streaming(struct vb2_queue *q)
{
}

static void srccap_buf_queue(struct vb2_buffer *vb)
{
	int ret = 0;
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct mtk_srccap_ctx *srccap_ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_srccap_dev *srccap_dev = srccap_ctx->srccap_dev;
	int plane_cnt = 0;
	struct srccap_dv_qbuf_in dv_in;
	struct srccap_dv_qbuf_out dv_out;

	SRCCAP_MSG_DEBUG("[CTRL_FLOW]m2m_qbuf(%u): index=%u type=%u memory=%u num_planes=%u",
		srccap_dev->dev_id,
		vb->index, vb->type, vb->memory, vb->num_planes);
	for (plane_cnt = 0; plane_cnt < vb->num_planes; plane_cnt++)
		SRCCAP_MSG_DEBUG(" fd[%d]=%d", plane_cnt, vb->planes[plane_cnt].m.fd);
	SRCCAP_MSG_DEBUG("\n");

	v4l2_m2m_buf_queue(srccap_ctx->fh.m2m_ctx, vbuf);

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	/* dolby queue buffer process start */
	memset(&dv_in, 0, sizeof(struct srccap_dv_qbuf_in));
	memset(&dv_out, 0, sizeof(struct srccap_dv_qbuf_out));
	dv_in.dev = srccap_dev;
	dv_in.vb = vb;
	ret = mtk_dv_qbuf(&dv_in, &dv_out);
	if (ret)
		SRCCAP_MSG_RETURN_CHECK(ret);
	/* dolby queue buffer process end */
#endif

	ret = mtk_memout_qbuf(srccap_dev, vb);
	if (ret)
		SRCCAP_MSG_RETURN_CHECK(ret);

	v4l2_m2m_dst_buf_remove(srccap_ctx->fh.m2m_ctx);
}

static const struct vb2_ops mtk_srccap_vb2_ops = {
	.queue_setup		= srccap_queue_setup,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
	.buf_init		= srccap_buf_init,
	.buf_prepare		= srccap_buf_pepare,
	.buf_finish		= srccap_buf_finish,
	.buf_cleanup		= srccap_buf_cleanup,
	.start_streaming	= srccap_start_streaming,
	.stop_streaming		= srccap_stop_streaming,
	.buf_queue		= srccap_buf_queue,
};

/* ============================================================================================== */
/* ---------------------------------------- vb2_mem_ops ----------------------------------------- */
/* ============================================================================================== */
/*
static void *srccap_vb2_get_userptr(
	struct device *dev,
	unsigned long vaddr,
	unsigned long size,
	enum dma_data_direction dma_dir)
{
	return NULL;
}

static void srccap_vb2_put_userptr(void *buf_priv)
{
}

const struct vb2_mem_ops mtk_srccap_vb2_mem_ops = {
	.get_userptr = mtk_srccap_vb2_get_userptr,
	.put_userptr = mtk_srccap_vb2_put_userptr,
};
 */

static int srccap_init_queue(
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
static int srccap_open(struct file *file)
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
	srccap_ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(srccap_dev->mdev, srccap_ctx, srccap_init_queue);

	// init function
	mtk_srccap_hdmirx_init(srccap_dev);

	mtk_srccap_avd_init(srccap_dev);

	mutex_unlock(&srccap_dev->mutex);
	return 0;
}

static int srccap_release(struct file *file)
{
	struct mtk_srccap_ctx *srccap_ctx = srccap_fh_to_ctx(file->private_data);
	struct mtk_srccap_dev *srccap = video_drvdata(file);

	SRCCAP_MSG_INFO("%s\n", __func__);

	mutex_lock(&srccap->mutex);

	mtk_srccap_hdmirx_release(srccap);
	v4l2_m2m_ctx_release(srccap_ctx->fh.m2m_ctx);
	v4l2_fh_del(&srccap_ctx->fh);
	v4l2_fh_exit(&srccap_ctx->fh);
	kfree(srccap_ctx);
	mutex_unlock(&srccap->mutex);

	return 0;
}

static const struct v4l2_file_operations mtk_srccap_fops = {
	.owner			= THIS_MODULE,
	.open			= srccap_open,
	.release		= srccap_release,
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
static void print_sysfs(
	char *buf,
	int *string_size,
	u16 max_size,
	u32 num)
{
	if ((buf == NULL) || (string_size == NULL)) {
		SRCCAP_MSG_ERROR("Failed to print due to NULL inputs!\r\n");
		return;
	}

	if (*string_size > max_size) {
		SRCCAP_MSG_ERROR("Failed to store into string buffer.");
		SRCCAP_MSG_ERROR("string size:%u, buffer size:%u\r\n",
			*string_size, max_size);
	} else {
		*string_size += snprintf(buf + *string_size, max_size
			- *string_size, "%u\n", num);
	}
}

static ssize_t help_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "Debug Help:\n"
	"- log_level <RW>: To control debug log level.\n"
	"	levels:\n"
	"		LOG_LEVEL_DEBUG   -- 6.\n"
	"		LOG_LEVEL_INFO    -- 5.\n"
	"		LOG_LEVEL_WARNING -- 4.\n"
	"		LOG_LEVEL_ERROR   -- 3.\n"
	"		LOG_LEVEL_FATAL   -- 2.\n"
	"		LOG_LEVEL_NORMAL  -- 1.\n"
	"		LOG_LEVEL_OFF     -- 0.\n");
}

static ssize_t log_level_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "log_level = %d\n", log_level);
}

static ssize_t log_level_store(struct kobject *kobj,
			       struct kobj_attribute *attr, const char *buf,
			       size_t count)
{
	unsigned long val = 0;
	int ret = 0;

	ret = kstrtoul(buf, SRCCAP_STRING_SIZE_10, &val);
	if (ret < 0)
		return ret;

	if (ret > 255)
		return -EINVAL;

	log_level = val;
	return count;
}

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
static ssize_t atrace_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "atrace_enable_srccap = 0x%x\n", atrace_enable_srccap);
}

static ssize_t atrace_enable_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf,
				   size_t count)

{
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);
	if (err)
		return err;

	atrace_enable_srccap = val;
	return count;
}
#endif

static ssize_t active_port_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);

	if (!srccap_dev) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "srccap_dev is NULL!\n");
		return -EINVAL;
	}

	return snprintf(buf, SRCCAP_STRING_SIZE_20, "actice = %d\n", srccap_dev->srccap_info.src);
}

static ssize_t active_port_store(struct kobject *kobj,
				 struct kobj_attribute *attr, const char *buf,
				 size_t count)
{
	unsigned long input = 0;
	int ret = 0;
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);

	if (!srccap_dev) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "srccap_dev is NULL!\n");
		return -EINVAL;
	}

	ret = kstrtoul(buf, SRCCAP_STRING_SIZE_10, &input);
	if (ret < 0)
		return ret;

	if (ret > 255)
		return -EINVAL;

	ret = srccap_set_input(srccap_dev, input);

	return input;
}

static ssize_t avd_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);

	if (!srccap_dev) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "srccap_dev is NULL!\n");
		return -EINVAL;
	}

	return snprintf(buf, SRCCAP_STRING_SIZE_65535, "AVD Info:\n"
	"		Source: %s\n"
	"		Notify Status :	%s\n"
	"		Signal Stage:	%s\n"
	"		Tv system :		%s\n"
	"		Scan Mode :		%s\n"
	"		Force mode:		%d\n"
	"		Force system:	%s\n"
	"		Color system:	0x%08llX\n"
	"		Eanble Count:	%d\n"
	"		Monitor Task:	%d\n"
	"		Bypass PQ:		%d\n"
	"		Bypass Color system:	%d\n",
	InputSourceToString(srccap_dev->srccap_info.src),
	SignalStatusToString(srccap_dev->avd_info.enVdSignalStatus),
	SignalStageToString((int)srccap_dev->avd_info.enSignalStage),
	VideoStandardToString(srccap_dev->avd_info.u64DetectTvSystem),
	ScanTypeToString(srccap_dev->avd_info.bIsATVScanMode),
	srccap_dev->avd_info.bForce,
	VideoStandardToString(srccap_dev->avd_info.u64ForceTvSystem),
	srccap_dev->avd_info.region_type,
	srccap_dev->avd_info.en_count,
	srccap_dev->avd_info.bIsMonitorTaskWorking,
	srccap_dev->avd_info.bIsPassPQ,
	srccap_dev->avd_info.bIsPassColorsys);
}

static ssize_t avd_store(struct kobject *kobj,
				 struct kobj_attribute *attr, const char *buf,
				 size_t count)
{
	unsigned long input = 0;
	int ret = 0;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_avd_store(dev, buf);
	if (ret) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_avd_store return %d!\n", ret);
		return ret;
	}

	return count;
}

static ssize_t ipdma_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query IP-DMA number
	print_sysfs(buf, &string_size, max_size, srccap_dev->srccap_info.cap.ipdma_cnt);

	return string_size;
}

static ssize_t ipdma_count_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t hdmi_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query HDMI number
	print_sysfs(buf, &string_size, max_size, srccap_dev->srccap_info.cap.hdmi_cnt);

	return string_size;
}

static ssize_t hdmi_count_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t cvbs_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query CVBS number
	print_sysfs(buf, &string_size, max_size, srccap_dev->srccap_info.cap.cvbs_cnt);

	return string_size;
}

static ssize_t cvbs_count_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t svideo_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query S-Video number
	print_sysfs(buf, &string_size, max_size, srccap_dev->srccap_info.cap.svideo_cnt);

	return string_size;
}

static ssize_t svideo_count_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t ypbpr_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query YPbPr number
	print_sysfs(buf, &string_size, max_size, srccap_dev->srccap_info.cap.ypbpr_cnt);

	return string_size;
}

static ssize_t ypbpr_count_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t vga_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query VGA number
	print_sysfs(buf, &string_size, max_size, srccap_dev->srccap_info.cap.vga_cnt);

	return string_size;
}

static ssize_t vga_count_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t atv_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query ATV number
	print_sysfs(buf, &string_size, max_size, srccap_dev->srccap_info.cap.atv_cnt);

	return string_size;
}

static ssize_t atv_count_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t dscl_scaling_cap_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query scaling cap
	print_sysfs(buf, &string_size, max_size, srccap_dev->srccap_info.cap.dscl_scaling_cap);

	return string_size;
}

static ssize_t dscl_scaling_cap_store(struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t hw_timestamp_max_value_show(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query video_device_num
	print_sysfs(buf, &string_size, max_size,
		srccap_dev->srccap_info.cap.hw_timestamp_max_value);

	return string_size;
}

static ssize_t hw_timestamp_max_value_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t video_device_num_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	//query video_device_num
	print_sysfs(buf, &string_size, max_size, srccap_dev->vdev->num);

	return string_size;
}

static ssize_t video_device_num_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t pattern_ip1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_IP1;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_srccap_pattern_show(buf, dev, position);
	if (count < 0) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_srccap_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_ip1_store(struct kobject *kobj,
				       struct kobj_attribute *attr,
				       const char *buf, size_t count)
{
	int ret = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_IP1;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_srccap_pattern_store(buf, dev, position);
	if (ret) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_pre_ip2_0_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_PRE_IP2_0;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_srccap_pattern_show(buf, dev, position);
	if (count < 0) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_srccap_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_pre_ip2_0_store(struct kobject *kobj,
				       struct kobj_attribute *attr,
				       const char *buf, size_t count)
{
	int ret = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_PRE_IP2_0;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_srccap_pattern_store(buf, dev, position);
	if (ret) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_pre_ip2_0_post_show(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   char *buf)
{
	ssize_t count = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_PRE_IP2_0_POST;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_srccap_pattern_show(buf, dev, position);
	if (count < 0) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_srccap_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_pre_ip2_0_post_store(struct kobject *kobj,
					    struct kobj_attribute *attr,
					    const char *buf, size_t count)
{
	int ret = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_PRE_IP2_0_POST;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_srccap_pattern_store(buf, dev, position);
	if (ret) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_pre_ip2_1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_PRE_IP2_1;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_srccap_pattern_show(buf, dev, position);
	if (count < 0) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_srccap_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_pre_ip2_1_store(struct kobject *kobj,
				       struct kobj_attribute *attr,
				       const char *buf, size_t count)
{
	int ret = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_PRE_IP2_1;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_srccap_pattern_store(buf, dev, position);
	if (ret) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_pre_ip2_1_post_show(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   char *buf)
{
	ssize_t count = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_PRE_IP2_1_POST;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_srccap_pattern_show(buf, dev, position);
	if (count < 0) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_srccap_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_pre_ip2_1_post_store(struct kobject *kobj,
					    struct kobj_attribute *attr,
					    const char *buf, size_t count)
{
	int ret = 0;
	enum srccap_pattern_position position = SRCCAP_PATTERN_POSITION_PRE_IP2_1_POST;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_srccap_pattern_store(buf, dev, position);
	if (ret) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t dv_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int ret = 0;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_dv_show(dev, buf);
	if (ret) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_dv_show return %d!\n", ret);
		return ret;
	}

	return ret;
}

static ssize_t dv_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	int ret = 0;
	struct device *dev = kobj_to_dev(kobj->parent);

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_dv_store(dev, buf);
	if (ret) {
		SRCCAP_LOG(LOG_LEVEL_ERROR, "mtk_dv_store return %d!\n", ret);
		return ret;
	}

	return count;
}

static ssize_t crop_align_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	print_sysfs(buf, &string_size, max_size,
		 srccap_dev->srccap_info.cap.crop_align);

	return string_size;
}

static ssize_t crop_align_store(struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t color_fmt_recommend_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	struct hdmi_avi_packet_info avi_info;
	enum v4l2_srccap_input_source src;
	enum m_hdmi_color_format hdmi_color_format;
	bool is_hdmi_mode = FALSE;
	bool is_rgb = FALSE;
	u8 hdmi_status = 0;
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	u32 yuv_single_win_format = 0;
	u32 yuv_multi_win_format = 0;
	u32 rgb_single_win_format = 0;
	u32 rgb_multi_win_format = 0;
	int string_size = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	src = srccap_dev->srccap_info.src;

	is_hdmi_mode = mtk_srccap_hdmirx_isHDMIMode(src);
	if (is_hdmi_mode) {
		hdmi_status = mtk_srccap_hdmirx_pkt_get_AVI(src, &avi_info,
			sizeof(struct hdmi_avi_packet_info));

		hdmi_color_format = avi_info.enColorForamt;
	}

	switch (src) {
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		is_rgb = (hdmi_color_format == M_HDMI_COLOR_RGB);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		is_rgb = TRUE;
		break;
	default:
		is_rgb = FALSE;
		break;
	}

	SRCCAP_MSG_INFO("[CTRL_FLOW] src: %d, is_rgb: %d\n", src, is_rgb);

	yuv_single_win_format =
		srccap_dev->srccap_info.cap.color_fmt.yuv_single_win_recommended_format;
	yuv_multi_win_format =
		srccap_dev->srccap_info.cap.color_fmt.yuv_multi_win_recommended_format;
	rgb_single_win_format =
		srccap_dev->srccap_info.cap.color_fmt.rgb_single_win_recommended_format;
	rgb_multi_win_format =
		srccap_dev->srccap_info.cap.color_fmt.rgb_multi_win_recommended_format;

	if (is_rgb) {
		print_sysfs(buf, &string_size, max_size, rgb_single_win_format);
		print_sysfs(buf, &string_size, max_size, rgb_multi_win_format);
	} else {
		print_sysfs(buf, &string_size, max_size, yuv_single_win_format);
		print_sysfs(buf, &string_size, max_size, yuv_multi_win_format);
	}

	return string_size;
}

static ssize_t color_fmt_recommend_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t color_fmt_support_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;
	int i = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	for (i = 0; i < srccap_dev->srccap_info.cap.color_fmt.support_cnt; i++)
		print_sysfs(buf, &string_size,
			max_size, srccap_dev->srccap_info.cap.color_fmt.color_fmt_support[i]);

	return string_size;
}

static ssize_t color_fmt_support_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t discrete_buffer_mode_support_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;

	if (srccap_dev->srccap_info.cap.hw_version == 1)
		print_sysfs(buf, &string_size, max_size, 0);
	else
		print_sysfs(buf, &string_size, max_size, 1);

	return string_size;
}

static ssize_t discrete_buffer_mode_support_store(struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t src_data_sync_port_num_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct device *dev = kobj_to_dev(kobj->parent);
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	u16 max_size = SRCCAP_STRING_SIZE_65535;
	int string_size = 0;
	int i = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

//HDMI1~4
	for (i = V4L2_SRCCAP_INPUT_SOURCE_HDMI; i <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4; i++) {
		string_size += snprintf(buf + string_size, max_size - string_size,
			"%u %u %u\n",
			i,
			srccap_dev->srccap_info.map[i].data_port,
			srccap_dev->srccap_info.map[i].sync_port);
	}
//CVBS1~8
	for (i = V4L2_SRCCAP_INPUT_SOURCE_CVBS; i <= V4L2_SRCCAP_INPUT_SOURCE_CVBS8; i++) {
		string_size += snprintf(buf + string_size, max_size - string_size,
			"%u %u %u\n",
			i,
			srccap_dev->srccap_info.map[i].data_port,
			srccap_dev->srccap_info.map[i].sync_port);
	}
//SVIDEO1~4
	for (i = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO; i <= V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4; i++) {
		string_size += snprintf(buf + string_size, max_size - string_size,
			"%u %u %u\n",
			i,
			srccap_dev->srccap_info.map[i].data_port,
			srccap_dev->srccap_info.map[i].sync_port);
	}
//SVIDEO1~4
	for (i = V4L2_SRCCAP_INPUT_SOURCE_YPBPR; i <= V4L2_SRCCAP_INPUT_SOURCE_YPBPR3; i++) {
		string_size += snprintf(buf + string_size, max_size - string_size,
			"%u %u %u\n",
			i,
			srccap_dev->srccap_info.map[i].data_port,
			srccap_dev->srccap_info.map[i].sync_port);
	}
//VGA1~3
	for (i = V4L2_SRCCAP_INPUT_SOURCE_VGA; i <= V4L2_SRCCAP_INPUT_SOURCE_VGA3; i++) {
		string_size += snprintf(buf + string_size, max_size - string_size,
			"%u %u %u\n",
			i,
			srccap_dev->srccap_info.map[i].data_port,
			srccap_dev->srccap_info.map[i].sync_port);
	}
//ATV
	for (i = V4L2_SRCCAP_INPUT_SOURCE_ATV; i <= V4L2_SRCCAP_INPUT_SOURCE_ATV; i++) {
		string_size += snprintf(buf + string_size, max_size - string_size,
			"%u %u %u\n",
			i,
			srccap_dev->srccap_info.map[i].data_port,
			srccap_dev->srccap_info.map[i].sync_port);
	}
//SCART1~2
	for (i = V4L2_SRCCAP_INPUT_SOURCE_SCART; i <= V4L2_SRCCAP_INPUT_SOURCE_SCART2; i++) {
		string_size += snprintf(buf + string_size, max_size - string_size,
			"%u %u %u\n",
			i,
			srccap_dev->srccap_info.map[i].data_port,
			srccap_dev->srccap_info.map[i].sync_port);
	}
//DVI1~4
	for (i = V4L2_SRCCAP_INPUT_SOURCE_DVI; i <= V4L2_SRCCAP_INPUT_SOURCE_DVI4; i++) {
		string_size += snprintf(buf + string_size, max_size - string_size,
			"%u %u %u\n",
			i,
			srccap_dev->srccap_info.map[i].data_port,
			srccap_dev->srccap_info.map[i].sync_port);
	}

	return string_size;
}

static ssize_t src_data_sync_port_num_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

#define SRCCAP_ATTR_RW(_name) \
	struct kobj_attribute dev_attr_##_name = __ATTR_RW(_name)
#define SRCCAP_ATTR_RO(_name) \
	struct kobj_attribute dev_attr_##_name = __ATTR_RO(_name)

static SRCCAP_ATTR_RO(help);
static SRCCAP_ATTR_RW(log_level);
#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
static SRCCAP_ATTR_RW(atrace_enable);
#endif
static SRCCAP_ATTR_RW(active_port);
static SRCCAP_ATTR_RW(avd);
static SRCCAP_ATTR_RW(ipdma_count);
static SRCCAP_ATTR_RW(hdmi_count);
static SRCCAP_ATTR_RW(cvbs_count);
static SRCCAP_ATTR_RW(svideo_count);
static SRCCAP_ATTR_RW(ypbpr_count);
static SRCCAP_ATTR_RW(vga_count);
static SRCCAP_ATTR_RW(atv_count);
static SRCCAP_ATTR_RW(dscl_scaling_cap);
static SRCCAP_ATTR_RW(hw_timestamp_max_value);
static SRCCAP_ATTR_RW(video_device_num);
static SRCCAP_ATTR_RW(pattern_ip1);
static SRCCAP_ATTR_RW(pattern_pre_ip2_0);
static SRCCAP_ATTR_RW(pattern_pre_ip2_0_post);
static SRCCAP_ATTR_RW(pattern_pre_ip2_1);
static SRCCAP_ATTR_RW(pattern_pre_ip2_1_post);
static SRCCAP_ATTR_RW(dv);
static SRCCAP_ATTR_RW(crop_align);

static SRCCAP_ATTR_RW(color_fmt_recommend);
static SRCCAP_ATTR_RW(color_fmt_support);
static SRCCAP_ATTR_RW(discrete_buffer_mode_support);
static SRCCAP_ATTR_RW(src_data_sync_port_num);

static struct attribute *mtk_srccap_attrs_0[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	&dev_attr_atrace_enable.attr,
#endif
	&dev_attr_active_port.attr,
	&dev_attr_avd.attr,
	&dev_attr_ipdma_count.attr,
	&dev_attr_hdmi_count.attr,
	&dev_attr_cvbs_count.attr,
	&dev_attr_svideo_count.attr,
	&dev_attr_ypbpr_count.attr,
	&dev_attr_vga_count.attr,
	&dev_attr_atv_count.attr,
	&dev_attr_dscl_scaling_cap.attr,
	&dev_attr_hw_timestamp_max_value.attr,
	&dev_attr_video_device_num.attr,
	&dev_attr_pattern_ip1.attr,
	&dev_attr_pattern_pre_ip2_0.attr,
	&dev_attr_pattern_pre_ip2_0_post.attr,
	&dev_attr_dv.attr,
	&dev_attr_crop_align.attr,
	&dev_attr_color_fmt_recommend.attr,
	&dev_attr_color_fmt_support.attr,
	&dev_attr_src_data_sync_port_num.attr,
	&dev_attr_discrete_buffer_mode_support.attr,
	NULL,
};

static struct attribute *mtk_srccap_attrs_1[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	&dev_attr_atrace_enable.attr,
#endif
	&dev_attr_active_port.attr,
	&dev_attr_avd.attr,
	&dev_attr_ipdma_count.attr,
	&dev_attr_hdmi_count.attr,
	&dev_attr_cvbs_count.attr,
	&dev_attr_svideo_count.attr,
	&dev_attr_ypbpr_count.attr,
	&dev_attr_vga_count.attr,
	&dev_attr_atv_count.attr,
	&dev_attr_dscl_scaling_cap.attr,
	&dev_attr_hw_timestamp_max_value.attr,
	&dev_attr_video_device_num.attr,
	&dev_attr_pattern_ip1.attr,
	&dev_attr_pattern_pre_ip2_1.attr,
	&dev_attr_pattern_pre_ip2_1_post.attr,
	&dev_attr_dv.attr,
	&dev_attr_crop_align.attr,
	&dev_attr_color_fmt_recommend.attr,
	&dev_attr_color_fmt_support.attr,
	&dev_attr_src_data_sync_port_num.attr,
	&dev_attr_discrete_buffer_mode_support.attr,
	NULL,
};

static const struct attribute_group mtk_srccap_attr_0_group = {
	.attrs = mtk_srccap_attrs_0
};

static const struct attribute_group mtk_srccap_attr_1_group = {
	.attrs = mtk_srccap_attrs_1
};

/* ============================================================================================== */
/* ---------------------------------------- v4l2_m2m_ops ---------------------------------------- */
/* ============================================================================================== */
static void srccap_m2m_device_run(void *priv)
{
}

static void srccap_m2m_job_abort(void *priv)
{
	struct mtk_srccap_ctx *srccap_ctx = priv;

	v4l2_m2m_job_finish(srccap_ctx->srccap_dev->mdev, srccap_ctx->fh.m2m_ctx);
}

static struct v4l2_m2m_ops mtk_srccap_m2m_ops = {
	.device_run = srccap_m2m_device_run,
	.job_abort = srccap_m2m_job_abort,
};

static void srccap_iounmap(struct mtk_srccap_dev *srccapdev)
{
	int i = 0;

	if (srccapdev->avd_info.pIoremap != NULL) {

		for (i = 0; i < srccapdev->avd_info.regCount; ++i)
			iounmap((void *)srccapdev->avd_info.pIoremap[i]);

		kfree(srccapdev->avd_info.pIoremap);
		srccapdev->avd_info.pIoremap = NULL;
	}

	if (srccapdev->vbi_info.VBICCvirAddress != NULL) {
		iounmap((void *)srccapdev->vbi_info.VBICCvirAddress);
		srccapdev->vbi_info.VBICCvirAddress = NULL;
	}
}

static int srccap_ioremap(struct mtk_srccap_dev *srccap_dev,
		struct device *pdev)
{
	int ret = 0, regSize = 0, i = 0, offset[2] = {0, 0};
	int j = 0;
	struct device *property_dev = pdev;
	u32 *reg = NULL;

	srccap_dev->avd_info.reg_info_offset = 2;
	srccap_dev->avd_info.reg_pa_idx = 1;
	srccap_dev->avd_info.reg_pa_size_idx = 3;

	ret = of_property_read_u32_array(property_dev->of_node,
				"reg_count", &srccap_dev->avd_info.regCount, 1);
	if (ret) {
		SRCCAP_MSG_ERROR("%s Failed to get reg_count\r\n", __func__);
		return -EINVAL;
	}

	regSize = (srccap_dev->avd_info.regCount << srccap_dev->avd_info.reg_info_offset);
	reg = kcalloc(regSize, sizeof(u32), GFP_KERNEL);
	if (reg == NULL) {
		SRCCAP_MSG_ERROR("%s Failed to kcalloc register\r\n", __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(property_dev->of_node,
				"ioremap", reg, regSize);
	if (ret < 0) {
		SRCCAP_MSG_ERROR("%s Failed to get register\r\n", __func__);
		kfree(reg);
		return -EINVAL;
	}

	srccap_dev->avd_info.pIoremap = kcalloc(srccap_dev->avd_info.regCount,
				sizeof(u64), GFP_KERNEL);
	if (srccap_dev->avd_info.pIoremap == NULL) {
		SRCCAP_MSG_ERROR("%s Failed to kcalloc g_ioremap\r\n", __func__);
		kfree(reg);
		return -EINVAL;
	}

	for (i = 0; i < srccap_dev->avd_info.regCount; ++i) {
		offset[0] = (i << srccap_dev->avd_info.reg_info_offset) +
			srccap_dev->avd_info.reg_pa_idx;
		offset[1] = (i << srccap_dev->avd_info.reg_info_offset) +
			srccap_dev->avd_info.reg_pa_size_idx;

		srccap_dev->avd_info.pIoremap[i] = (u64)ioremap(reg[offset[0]], reg[offset[1]]);

		SRCCAP_MSG_INFO("0x%lx = 0x%x(%x)\n",
			srccap_dev->avd_info.pIoremap[i], reg[offset[0]], reg[offset[1]]);

		drv_hwreg_common_setRIUaddr(reg[offset[0]] - SRCCAP_RIU_OFFSET, reg[offset[1]],
			srccap_dev->avd_info.pIoremap[i]);
	}
	kfree(reg);
	return ret;
}

static int srccap_get_input_source_num(
	const char *input_src_name)
{
	int strlen = 0;
	enum v4l2_srccap_input_source src = V4L2_SRCCAP_INPUT_SOURCE_NONE;

	if (input_src_name == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	strlen = strnlen(input_src_name, SRCCAP_MAX_INPUT_SOURCE_PORT_NUM);
	if (strlen >= SRCCAP_MAX_INPUT_SOURCE_PORT_NUM)
		return -ENOENT;

	if (strncmp("INPUT_SOURCE_HDMI", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_HDMI;
	else if (strncmp("INPUT_SOURCE_HDMI2", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_HDMI2;
	else if (strncmp("INPUT_SOURCE_HDMI3", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_HDMI3;
	else if (strncmp("INPUT_SOURCE_HDMI4", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_HDMI4;
	else if (strncmp("INPUT_SOURCE_CVBS", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
	else if (strncmp("INPUT_SOURCE_SVIDEO", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO;
	else if (strncmp("INPUT_SOURCE_YPBPR", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
	else if (strncmp("INPUT_SOURCE_VGA", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_VGA;
	else if (strncmp("INPUT_SOURCE_ATV", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_ATV;
	else if (strncmp("INPUT_SOURCE_SCART", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_SCART;
	else if (strncmp("INPUT_SOURCE_DVI", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_DVI;
	else if (strncmp("INPUT_SOURCE_DVI2", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_DVI2;
	else if (strncmp("INPUT_SOURCE_DVI3", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_DVI3;
	else if (strncmp("INPUT_SOURCE_DVI4", input_src_name, strlen) == 0)
		src = V4L2_SRCCAP_INPUT_SOURCE_DVI4;
	else
		return -ENOENT;

	SRCCAP_MSG_INFO("%s = (%d)\n", input_src_name, src);
	return (int)src;
}


static enum srccap_input_port srccap_get_rprefix_portnum(
	const char *input_port_name)
{
	int strlen = 0;
	enum srccap_input_port src = SRCCAP_INPUT_PORT_NONE;

	if (input_port_name == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	strlen = strnlen(input_port_name, SRCCAP_MAX_INPUT_SOURCE_PORT_NUM);
	if (strlen >= SRCCAP_MAX_INPUT_SOURCE_PORT_NUM)
		return -ENOENT;

	if (strncmp("INPUT_PORT_RGB0_DATA", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB0_DATA;
	else if (strncmp("INPUT_PORT_RGB1_DATA", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB1_DATA;
	else if (strncmp("INPUT_PORT_RGB2_DATA", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB2_DATA;
	else if (strncmp("INPUT_PORT_RGB3_DATA", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB3_DATA;
	else if (strncmp("INPUT_PORT_RGB4_DATA", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB4_DATA;
	else if (strncmp("INPUT_PORT_RGB0_SYNC", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB0_SYNC;
	else if (strncmp("INPUT_PORT_RGB1_SYNC", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB1_SYNC;
	else if (strncmp("INPUT_PORT_RGB2_SYNC", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB2_SYNC;
	else if (strncmp("INPUT_PORT_RGB3_SYNC", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB3_SYNC;
	else if (strncmp("INPUT_PORT_RGB4_SYNC", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_RGB4_SYNC;
	else
		src = SRCCAP_INPUT_PORT_NONE;

	return src;
}

static enum srccap_input_port srccap_get_yprefix_portnum(
	const char *input_port_name)
{
	int strlen = 0;
	enum srccap_input_port src = SRCCAP_INPUT_PORT_NONE;

	if (input_port_name == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	strlen = strnlen(input_port_name, SRCCAP_MAX_INPUT_SOURCE_PORT_NUM);
	if (strlen >= SRCCAP_MAX_INPUT_SOURCE_PORT_NUM)
		return -ENOENT;

	if (strncmp("INPUT_PORT_YCVBS0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YCVBS0;
	else if (strncmp("INPUT_PORT_YCVBS1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YCVBS1;
	else if (strncmp("INPUT_PORT_YCVBS2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YCVBS2;
	else if (strncmp("INPUT_PORT_YCVBS2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YCVBS3;
	else if (strncmp("INPUT_PORT_YCVBS4", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YCVBS4;
	else if (strncmp("INPUT_PORT_YCVBS5", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YCVBS5;
	else if (strncmp("INPUT_PORT_YCVBS6", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YCVBS6;
	else if (strncmp("INPUT_PORT_YCVBS7", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YCVBS7;
	else if (strncmp("INPUT_PORT_YG0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YG0;
	else if (strncmp("INPUT_PORT_YG1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YG1;
	else if (strncmp("INPUT_PORT_YG2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YG2;
	else if (strncmp("INPUT_PORT_YR0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YR0;
	else if (strncmp("INPUT_PORT_YR1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YR1;
	else if (strncmp("INPUT_PORT_YR2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YR2;
	else if (strncmp("INPUT_PORT_YB0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YB0;
	else if (strncmp("INPUT_PORT_YB1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YB1;
	else if (strncmp("INPUT_PORT_YB2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_YB2;
	else
		src = SRCCAP_INPUT_PORT_NONE;

	return src;
}

static enum srccap_input_port srccap_get_cprefix_portnum(
	const char *input_port_name)
{
	int strlen = 0;
	enum srccap_input_port src = SRCCAP_INPUT_PORT_NONE;

	if (input_port_name == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	strlen = strnlen(input_port_name, SRCCAP_MAX_INPUT_SOURCE_PORT_NUM);
	if (strlen >= SRCCAP_MAX_INPUT_SOURCE_PORT_NUM)
		return -ENOENT;

	if (strncmp("INPUT_PORT_CCVBS0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CCVBS0;
	else if (strncmp("INPUT_PORT_CCVBS1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CCVBS1;
	else if (strncmp("INPUT_PORT_CCVBS2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CCVBS2;
	else if (strncmp("INPUT_PORT_CCVBS3", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CCVBS3;
	else if (strncmp("INPUT_PORT_CCVBS4", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CCVBS4;
	else if (strncmp("INPUT_PORT_CCVBS5", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CCVBS5;
	else if (strncmp("INPUT_PORT_CCVBS6", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CCVBS6;
	else if (strncmp("INPUT_PORT_CCVBS7", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CCVBS7;
	else if (strncmp("INPUT_PORT_CR0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CR0;
	else if (strncmp("INPUT_PORT_CR1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CR1;
	else if (strncmp("INPUT_PORT_CR2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CR2;
	else if (strncmp("INPUT_PORT_CG0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CG0;
	else if (strncmp("INPUT_PORT_CG1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CG1;
	else if (strncmp("INPUT_PORT_CG2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CG2;
	else if (strncmp("INPUT_PORT_CB0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CB0;
	else if (strncmp("INPUT_PORT_CB1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CB1;
	else if (strncmp("INPUT_PORT_CB2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_CB2;
	else
		src = SRCCAP_INPUT_PORT_NONE;

	return src;
}

static int srccap_get_input_port_num(
	const char *input_port_name)
{
	int strlen = 0;
	enum srccap_input_port src = SRCCAP_INPUT_PORT_NONE;

	if (input_port_name == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	strlen = strnlen(input_port_name, SRCCAP_MAX_INPUT_SOURCE_PORT_NUM);
	if (strlen >= SRCCAP_MAX_INPUT_SOURCE_PORT_NUM)
		return -ENOENT;

	if (strncmp("INPUT_PORT_NONE", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_NONE;
	else if (strnstr(input_port_name, "INPUT_PORT_R", strlen) != NULL)
		src = srccap_get_rprefix_portnum(input_port_name);
	else if (strnstr(input_port_name, "INPUT_PORT_Y", strlen) != NULL)
		src = srccap_get_yprefix_portnum(input_port_name);
	else if (strnstr(input_port_name, "INPUT_PORT_C", strlen) != NULL)
		src = srccap_get_cprefix_portnum(input_port_name);
	else if (strncmp("INPUT_PORT_DVI0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_DVI0;
	else if (strncmp("INPUT_PORT_DVI1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_DVI1;
	else if (strncmp("INPUT_PORT_DVI2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_DVI2;
	else if (strncmp("INPUT_PORT_DVI3", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_DVI3;
	else if (strncmp("INPUT_PORT_HDMI0", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_HDMI0;
	else if (strncmp("INPUT_PORT_HDMI1", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_HDMI1;
	else if (strncmp("INPUT_PORT_HDMI2", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_HDMI2;
	else if (strncmp("INPUT_PORT_HDMI3", input_port_name, strlen) == 0)
		src = SRCCAP_INPUT_PORT_HDMI3;
	else
		return -ENOENT;

	SRCCAP_MSG_INFO("%s = (%d)\n", input_port_name, src);
	return (int)src;
}

static int srccap_parse_dts_clk(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	struct clk *clk = NULL;

	if (srccap_dev->dev_id == 0) {
		/* get VD sw_en clock hanlder */
		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_CMBAI2VD", &clk);
		srccap_dev->avd_info.stclock.cmbai2vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_CMBAO2VD", &clk);
		srccap_dev->avd_info.stclock.cmbao2vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_CMBBI2VD", &clk);
		srccap_dev->avd_info.stclock.cmbbi2vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_CMBBO2VD", &clk);
		srccap_dev->avd_info.stclock.cmbbo2vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_MCU_MAIL02VD", &clk);
		srccap_dev->avd_info.stclock.mcu_mail02vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_MCU_MAIL12VD", &clk);
		srccap_dev->avd_info.stclock.mcu_mail12vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_SMI2MCU_M2MCU", &clk);
		srccap_dev->avd_info.stclock.smi2mcu_m2mcu = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_SMI2VD", &clk);
		srccap_dev->avd_info.stclock.smi2vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_VD2X2VD", &clk);
		srccap_dev->avd_info.stclock.vd2x2vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_VD_32FSC2VD", &clk);
		srccap_dev->avd_info.stclock.vd_32fsc2vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_VD2VD", &clk);
		srccap_dev->avd_info.stclock.vd2vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_XTAL_12M2VD", &clk);
		srccap_dev->avd_info.stclock.xtal_12m2vd = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_XTAL_12M2MCU_M2RIU", &clk);
		srccap_dev->avd_info.stclock.xtal_12m2mcu_m2riu = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_EN_XTAL_12M2MCU_M2MCU", &clk);
		srccap_dev->avd_info.stclock.xtal_12m2mcu_m2mcu = clk;

		/* get VD buf sel clock hanlder */

		ret |= srccap_get_clk(srccap_dev->dev, "VD_CLK_ATV_INPUT", &clk);
		srccap_dev->avd_info.stclock.clk_vd_atv_input = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_CLK_CVBS_INPUT", &clk);
		srccap_dev->avd_info.stclock.clk_vd_cvbs_input = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "VD_CLK_BUF_SEL", &clk);
		srccap_dev->avd_info.stclock.clk_buf_sel = clk;

		ret |= srccap_get_clk(srccap_dev->dev, "srccap_ip1_xtal_ck", &clk);
		srccap_dev->srccap_info.clk.ip1_xtal_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_hdmi_idclk_ck", &clk);
		srccap_dev->srccap_info.clk.hdmi_idclk_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_hdmi2_idclk_ck", &clk);
		srccap_dev->srccap_info.clk.hdmi2_idclk_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_hdmi3_idclk_ck", &clk);
		srccap_dev->srccap_info.clk.hdmi3_idclk_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_hdmi4_idclk_ck", &clk);
		srccap_dev->srccap_info.clk.hdmi4_idclk_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_adc_ck", &clk);
		srccap_dev->srccap_info.clk.adc_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_hdmi_ck", &clk);
		srccap_dev->srccap_info.clk.hdmi_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_hdmi2_ck", &clk);
		srccap_dev->srccap_info.clk.hdmi2_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_hdmi3_ck", &clk);
		srccap_dev->srccap_info.clk.hdmi3_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_hdmi4_ck", &clk);
		srccap_dev->srccap_info.clk.hdmi4_ck = clk;
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_ipdma0_ck", &clk);
		srccap_dev->srccap_info.clk.ipdma0_ck = clk;
	} else if (srccap_dev->dev_id == 1) {
		ret |= srccap_get_clk(srccap_dev->dev, "srccap_ipdma1_ck", &clk);
		srccap_dev->srccap_info.clk.ipdma1_ck = clk;
	} else
		SRCCAP_MSG_ERROR("device id not found\n");

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	return ret;
}

#if defined(CONFIG_CMA) || defined(CONFIG_DD_USE_CMA)
static int comb_get_memory_cmaBase(struct device_node *np, unsigned long *base)
{
#define BUF_INFO_ADDR_CELLS 2
#define BUF_INFO_SIZE_CELLS 2

	uint32_t len = 0;
	__be32 *p32 = NULL;
	int id;
	struct device_node *node;
	int naddr = BUF_INFO_ADDR_CELLS;
	int nsize = BUF_INFO_SIZE_CELLS;

	p32 = (__be32 *) of_get_property(np, "cmas", &len);

	if (p32 == NULL) {
		SRCCAP_AVD_MSG_ERROR("Fail of_get_property in node\n");
		return -EFAULT;
	}

	id = be32_to_cpup(p32);
	node = of_parse_phandle(np, "memory-region", id);
	p32 = (__be32 *) of_get_property(node, "reg", &len);

	if (p32 == NULL || ((len / sizeof(__be32)) < (naddr + nsize))) {
		SRCCAP_AVD_MSG_ERROR("cannot get correct reg in node\n");
		return -EINVAL;
	}

	*base = of_read_number(p32, naddr);
	return 0;

#undef BUF_INFO_ADDR_CELLS
#undef BUF_INFO_SIZE_CELLS

}
#endif

#if defined(CONFIG_CMA) || defined(CONFIG_DD_USE_CMA)
static int comb_query_heap_id(char *heap_tag_name)
{
	int i;
	size_t nr_heaps;
	struct ion_heap_data *data;
	int heap_id = -ENODEV;

	nr_heaps = ion_query_heaps_kernel(NULL, 0);
	if (nr_heaps == 0)
		return -ENODEV;

	data = kcalloc(nr_heaps, sizeof(struct ion_heap_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (ion_query_heaps_kernel(data, nr_heaps) == 0) {
		kfree(data);
		SRCCAP_AVD_MSG_ERROR("query heaps failed\n");
		return -ENODEV;
	}

	for (i = 0; i < nr_heaps; i++) {
		if (strstr(data[i].name, heap_tag_name)) {
			heap_id = data[i].heap_id;
			break;
		}
	}
	kfree(data);
	return heap_id;
}
#endif

#if defined(CONFIG_CMA) || defined(CONFIG_DD_USE_CMA)
bool comb_get_memory(struct platform_device *pdev, u64 *pAddr,
	unsigned int *pSize, unsigned int *cma_id)
{
	struct device_node *np;
	struct of_mmap_info_data mmap_info;
	unsigned long attrs = 0;
	unsigned long cma_mem_base;
	int heap_id;
	dma_addr_t  bus_addr;
	dma_addr_t  bus_addr_offset;

	np = of_find_compatible_node(NULL, NULL, "mediatek,dtv-mmap-vd-comb-buf");
	if (np == NULL) {
		SRCCAP_AVD_MSG_ERROR("[COMB]of_find_compatible_node failed \r\n");
		return false;
	}

	of_mtk_get_reserved_memory_info_by_idx(np, 0, &mmap_info);
	SRCCAP_AVD_MSG_INFO("[COMB]mmap_info addr 0x%llx, mmap_info len 0x%llx",
		mmap_info.start_bus_address, mmap_info.buffer_size);

	if (comb_get_memory_cmaBase(np, &cma_mem_base) != 0) {
		SRCCAP_AVD_MSG_ERROR("[COMB]comb_get_memory_cmaBase failed \r\n");
		return false;
	}

	bus_addr = cma_mem_base;
	attrs |= DMA_ATTR_NO_KERNEL_MAPPING;
	bus_addr_offset = mmap_info.start_bus_address - bus_addr;
	dma_alloc_attrs(&pdev->dev, mmap_info.buffer_size, &bus_addr_offset, GFP_KERNEL, attrs);

	*pAddr = bus_addr - COMB_BUS_OFFSET;
	*pSize = mmap_info.buffer_size;
	*cma_id = mmap_info.cma_id;

	SRCCAP_AVD_MSG_INFO("[COMB][CMA]get OK addr 0x%llx, len 0x%lx cma_id = 0x%lx",
		*pAddr, *pSize, *cma_id);

	return true;
}
#endif

#if defined(CONFIG_CMA) || defined(CONFIG_DD_USE_CMA)
static int srccap_parse_dts_vd_cma_buf(
	struct mtk_srccap_dev *srccap_dev,
	struct platform_device *pdev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	bool result = false;
	struct device_node *buf_node;
	struct device_node *memory_info_node;
	struct of_mmap_info_data of_mmap_info = {0};
	uint32_t len = 0;
	uint32_t cma_id = 0;
	u64 u64emi0_base = 0;
	__be32 *p = NULL;
	u64 addr;

	if (srccap_dev->dev->of_node != NULL)
		of_node_get(srccap_dev->dev->of_node);

	buf_node = of_find_node_by_name(
		srccap_dev->dev->of_node,
		"MI_EXTIN_VD_COMB_BUF");
	if (buf_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	ret |= srccap_read_dts_u32(buf_node, "MI_EXTIN_VD_COMB_ALIMENT",
		&srccap_dev->avd_info.u32Comb3dAliment);
	ret |= srccap_read_dts_u32(buf_node, "MI_EXTIN_VD_COMB_SIZE",
		&srccap_dev->avd_info.u32Comb3dSize);

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		of_node_put(buf_node);
		return -ENOENT;
	}

	SRCCAP_AVD_MSG_INFO("[START][CMA][VD]Get VD comb buffer.\n");
	result = comb_get_memory(pdev, &addr, &len, &cma_id);
	if (result != true) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		of_node_put(buf_node);
		return -ENOENT;
	}

	srccap_dev->avd_info.Comb3dBufAddr = addr;
	srccap_dev->avd_info.Comb3dBufSize = len;
	srccap_dev->avd_info.Comb3dBufHeapId = cma_id;
	SRCCAP_AVD_MSG_INFO("[CMA][VD]addr = 0x%llx size = 0x%lx cma_id = 0x%lx\n",
		addr, len, cma_id);
	SRCCAP_AVD_MSG_INFO("[END][CMA][VD]Get VD comb buffer.\n");

	if (mtk_avd_InitHWinfo(srccap_dev) < 0) {
		SRCCAP_AVD_MSG_ERROR("Init AVD RegBase & Bank failed!!\r\n");
		ret = -ENOENT;
	}

	of_node_put(buf_node);
	return ret;
}
#endif

static int srccap_parse_dts_vd_mmap_buf(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	struct device_node *buf_node;
	struct device_node *mmap_node;
	struct device_node *memory_info_node;
	struct of_mmap_info_data of_mmap_info = {0};
	uint32_t len = 0;
	u64 u64emi0_base = 0;
	__be32 *p = NULL;

	if (srccap_dev->dev->of_node != NULL)
		of_node_get(srccap_dev->dev->of_node);

	buf_node = of_find_node_by_name(
		srccap_dev->dev->of_node,
		"MI_EXTIN_VD_COMB_BUF");
	if (buf_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	ret |= srccap_read_dts_u32(buf_node, "MI_EXTIN_VD_COMB_ALIMENT",
		&srccap_dev->avd_info.u32Comb3dAliment);
	ret |= srccap_read_dts_u32(buf_node, "MI_EXTIN_VD_COMB_SIZE",
		&srccap_dev->avd_info.u32Comb3dSize);
	of_node_put(buf_node);

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	// find memory_info node in dts
	memory_info_node = of_find_node_by_name(NULL, "memory_info");
	if (memory_info_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	// query cpu_emi0_base value from memory_info node
	p = (__be32 *)of_get_property(memory_info_node, "cpu_emi0_base", &len);

	if (p != NULL) {
		u64emi0_base = be32_to_cpup(p);
		of_node_put(memory_info_node);
		p = NULL;
	} else {
		SRCCAP_AVD_MSG_ERROR("can not find cpu_emi0_base info\n");
		of_node_put(memory_info_node);
		return -EINVAL;
	}
	SRCCAP_AVD_MSG_INFO("[MMAP][VD]cpu_emi0_base address is 0x%llX\n", u64emi0_base);

	mmap_node = of_find_compatible_node(NULL, NULL, "mediatek,dtv-mmap-vd-comb-buf");
	if (mmap_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	SRCCAP_AVD_MSG_INFO("[START][MMAP][VD]Get VD comb buffer. node : %s.\n", mmap_node->name);
	if (of_mtk_get_reserved_memory_info_by_idx(
		mmap_node, 0, &of_mmap_info) == 0) {
		SRCCAP_AVD_MSG_INFO("[MMAP][VD]Show vd mmap info\n");
		SRCCAP_AVD_MSG_INFO("[MMAP][VD]layer is %u\n",
			of_mmap_info.layer);
		SRCCAP_AVD_MSG_INFO("[MMAP][VD]miu is %u\n",
			of_mmap_info.miu);
		SRCCAP_AVD_MSG_INFO("[MMAP][VD]is_cache is %u\n",
			of_mmap_info.is_cache);
		SRCCAP_AVD_MSG_INFO("[MMAP][VD]cma_id is %u\n",
			of_mmap_info.cma_id);
		SRCCAP_AVD_MSG_INFO("[MMAP][VD]start_bus_address is 0x%llX\n",
			of_mmap_info.start_bus_address);
		SRCCAP_AVD_MSG_INFO("[MMAP][VD]buffer_size is 0x%llX\n",
			of_mmap_info.buffer_size);
		srccap_dev->avd_info.Comb3dBufAddr = of_mmap_info.start_bus_address - u64emi0_base;
		srccap_dev->avd_info.Comb3dBufSize = of_mmap_info.buffer_size;
		srccap_dev->avd_info.Comb3dBufHeapId = of_mmap_info.cma_id;
	} else
		SRCCAP_AVD_MSG_WARNING("[MMAP][VD]Get vd mmap info fail\n");

	SRCCAP_AVD_MSG_INFO("[END][MMAP][VD]Get VD comb buffer.\n");

	if (mtk_avd_InitHWinfo(srccap_dev) < 0) {
		SRCCAP_AVD_MSG_ERROR("Init AVD RegBase & Bank failed!!\r\n");
		ret = -ENOENT;
	}

	of_node_put(mmap_node);
	return ret;
}

#if defined(CONFIG_CMA) || defined(CONFIG_DD_USE_CMA)
static int vbi_get_memory_cmaBase(struct device_node *np, unsigned long *base)
{
#define BUF_INFO_ADDR_CELLS 2
#define BUF_INFO_SIZE_CELLS 2

	uint32_t len = 0;
	__be32 *p32 = NULL;
	int id;
	struct device_node *node;
	int naddr = BUF_INFO_ADDR_CELLS;
	int nsize = BUF_INFO_SIZE_CELLS;

	p32 = (__be32 *) of_get_property(np, "cmas", &len);
	if (p32 == NULL) {
		SRCCAP_VBI_MSG_ERROR("Fail of_get_property in node\n");
		return -EFAULT;
	}

	id = be32_to_cpup(p32);
	node = of_parse_phandle(np, "memory-region", id);
	p32 = (__be32 *) of_get_property(node, "reg", &len);

	if (p32 == NULL || ((len / sizeof(__be32)) < (naddr + nsize))) {
		SRCCAP_VBI_MSG_ERROR("cannot get correct reg in node\n");
		return -EINVAL;
	}

	*base = of_read_number(p32, naddr);
	return 0;

#undef BUF_INFO_ADDR_CELLS
#undef BUF_INFO_SIZE_CELLS

}
#endif

#if defined(CONFIG_CMA) || defined(CONFIG_DD_USE_CMA)
static int vbi_query_heap_id(char *heap_tag_name)
{
	int i;
	size_t nr_heaps;
	struct ion_heap_data *data;
	int heap_id = -ENODEV;

	nr_heaps = ion_query_heaps_kernel(NULL, 0);
	if (nr_heaps == 0)
		return -ENODEV;

	data = kcalloc(nr_heaps, sizeof(struct ion_heap_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (ion_query_heaps_kernel(data, nr_heaps) == 0) {
		kfree(data);
		SRCCAP_VBI_MSG_ERROR("query heaps failed\n");
		return -ENODEV;
	}

	for (i = 0; i < nr_heaps; i++) {
		if (strstr(data[i].name, heap_tag_name)) {
			heap_id = data[i].heap_id;
			break;
		}
	}
	kfree(data);
	return heap_id;
}
#endif

#if defined(CONFIG_CMA) || defined(CONFIG_DD_USE_CMA)
bool vbi_get_memory(struct platform_device *pdev, phys_addr_t *pAddr, unsigned int *pSize,
	unsigned int *cma_id)
{
	struct device_node *np;
	struct of_mmap_info_data mmap_info;
	unsigned long attrs = 0;
	unsigned long cma_mem_base;
	int heap_id;
	u64 bus_addr;
	u64 bus_addr_offset;

	np = of_find_compatible_node(NULL, NULL, "mediatek,dtv-mmap-vbi-buf");
	if (np == NULL) {
		SRCCAP_VBI_MSG_ERROR("[VBI]of_find_compatible_node failed \r\n");
		return false;
	}

	of_mtk_get_reserved_memory_info_by_idx(np, 0, &mmap_info);
	SRCCAP_VBI_MSG_INFO("[VBI]mmap_info addr 0x%llx, mmap_info len 0x%llx",
		mmap_info.start_bus_address, mmap_info.buffer_size);

	if (vbi_get_memory_cmaBase(np, &cma_mem_base) != 0) {
		SRCCAP_VBI_MSG_ERROR("[VBI]vbi_get_memory_cmaBase failed \r\n");
		return false;
	}

	bus_addr = cma_mem_base;
	attrs |= DMA_ATTR_NO_KERNEL_MAPPING;
	bus_addr_offset = mmap_info.start_bus_address - bus_addr;
	dma_alloc_attrs(&pdev->dev, mmap_info.buffer_size, &bus_addr_offset, GFP_KERNEL, attrs);

	*pAddr = bus_addr - VBI_BUS_OFFSET;
	*pSize = mmap_info.buffer_size;
	of_vbi_mmap_info.is_cache = mmap_info.is_cache;
	*cma_id = mmap_info.cma_id;
	SRCCAP_VBI_MSG_INFO("[VBI][CMA]get OK addr 0x%llx, len 0x%lx cma_id = 0x%lx",
		*pAddr, *pSize, *cma_id);

	return true;
}
#endif

#if defined(CONFIG_CMA) || defined(CONFIG_DD_USE_CMA)
static int srccap_parse_dts_vbi_cma_buf(
	struct platform_device *pdev)
{
	SRCCAP_VBI_MSG_INFO("[VBI][CMA] srccap vbi cma buf\n");
	if (vbideviceinitialized != 0)
		return 0;

	int ret = 0;
	int npages = 0;
	void *vaddr;
	u64 start_bus_pfn;
	int i = 0;
	pgprot_t pgprot;
	uint32_t len = 0;
	__be32 *p = NULL;
	u64 addr;
	uint32_t cma_id = 0;

	struct device_node *buf_node;
	struct device_node *memory_info_node;

	//srccap_vbi_info = srccap_dev->vbi_info;

	vbi_get_memory(pdev, (phys_addr_t *) &addr, &len, &cma_id);

	// step_2: mmap to kernel va
	npages = PAGE_ALIGN(len) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;

	start_bus_pfn = addr >> PAGE_SHIFT;

	if (of_vbi_mmap_info.is_cache)
		pgprot = PAGE_KERNEL;
	else
		pgprot = pgprot_writecombine(PAGE_KERNEL);

	SRCCAP_VBI_MSG_INFO("[VBI][CMA] start_bus_pfn is 0x%lX\n", start_bus_pfn);
	SRCCAP_VBI_MSG_INFO("[VBI][CMA] npages is %d\n", npages);
	for (i = 0; i < npages; ++i) {
		*(tmp++) = __pfn_to_page(start_bus_pfn);
		start_bus_pfn++;
	}

	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	SRCCAP_VBI_MSG_INFO("[VBI][CMA] [%d] kernel vaddr is 0x%lX\n", i, vaddr);

	mtk_vbi_SetBufferAddr(vaddr, srccap_vbi_info);

	vbideviceinitialized = 1;
	return 0;
}
#endif

static int srccap_vbi_mmap_open(struct inode *inode, struct file *file)
{
	return 0;
}


static int srccap_vbi_mmap_release(struct inode *inode,
		struct file *file)
{
	return 0;
}

static int srccap_vbi_userva_mmap(struct file *filp,
		struct vm_area_struct *vma)
{
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI] mmap\n");
	// test_1: mtk_ltp_mmap_map_user_va_rw
	int ret;
	u64 start_bus_pfn;
	pgprot_t pgprot;
	void *vaddr;
	size_t len;
	unsigned long start;

	// step_1: get mmap info
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI] mmap test index: 1\n");
	ret = of_mtk_get_reserved_memory_info_by_idx(vbi_mmap_node, 0, &of_vbi_mmap_info);
	if (ret < 0) {
		pr_emerg("mmap return %d\n", ret);
		return ret;
	}

	SRCCAP_VBI_MSG_INFO("[MMAP][VBI]Show vbi mmap info\n");
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI]layer is %u\n", of_vbi_mmap_info.layer);
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI]miu is %u\n", of_vbi_mmap_info.miu);
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI]is_cache is %u\n", of_vbi_mmap_info.is_cache);
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI]cma_id is %u\n", of_vbi_mmap_info.cma_id);
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI]start_bus_address is 0x%llX\n",
		of_vbi_mmap_info.start_bus_address);
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI]buffer_size is 0x%llX\n",
		of_vbi_mmap_info.buffer_size);

	// step_2: mmap to user_va
	len = vma->vm_end - vma->vm_start;
	//len = PAGE_SIZE;
	start = of_vbi_mmap_info.start_bus_address;
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI] Test mmap to user va\n");

	//start_bus_pfn = of_mmap_info.start_bus_address >> PAGE_SHIFT;
	SRCCAP_VBI_MSG_ERROR("[MMAP] Test for mmap region 0x%lx+%lx offset 0x%lx\n",
		start, len, vma->vm_pgoff << PAGE_SHIFT);
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	ret = vm_iomap_memory(vma, start, len);
	if (ret) {
		SRCCAP_VBI_MSG_ERROR("vm_iomap_memory failed with (%d)\n", ret);
		return ret;
	}

	return ret;
}

static const struct file_operations srccap_mmap_vbi_fops = {
	.owner = THIS_MODULE,
	.open = srccap_vbi_mmap_open,
	.release = srccap_vbi_mmap_release,
	.mmap = srccap_vbi_userva_mmap,
};

static int srccap_parse_dts_vbi_mmap_buf(
	struct mtk_srccap_dev *srccap_dev)
{
	SRCCAP_VBI_MSG_INFO("[SRCCAP][VBI] srccap vbi mmap buf\n");
	if (vbideviceinitialized != 0)
		return 0;

	int ret = 0;
	int npages = 0;
	void *vaddr;
	u64 start_bus_pfn;
	int i = 0;
	pgprot_t pgprot;
	uint32_t len = 0;
	__be32 *p = NULL;

	struct device_node *memory_info_node;

	srccap_vbi_info = srccap_dev->vbi_info;

	// find memory_info node in dts
	memory_info_node = of_find_node_by_name(NULL, "memory_info");
	if (memory_info_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	// query cpu_emi0_base value from memory_info node
	p = (__be32 *)of_get_property(memory_info_node, "cpu_emi0_base", &len);

	if (p != NULL) {
		u64vbiemi0_base = be32_to_cpup(p);
		of_node_put(memory_info_node);
		p = NULL;
	} else {
		SRCCAP_VBI_MSG_ERROR("can not find cpu_emi0_base info\n");
		of_node_put(memory_info_node);
		return -EINVAL;
	}

	vbi_mmap_node = of_find_compatible_node(NULL, NULL, "mediatek,dtv-mmap-vbi-buf");
	if (vbi_mmap_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	// step_1: get mmap info
	SRCCAP_VBI_MSG_INFO("[START][MMAP][VBI]Get VBI buffer. node : %s.\n", vbi_mmap_node->name);
	if (of_mtk_get_reserved_memory_info_by_idx(
		vbi_mmap_node, 0, &of_vbi_mmap_info) == 0) {
		SRCCAP_VBI_MSG_INFO("[MMAP][VBI]Show vd mmap info\n");
		SRCCAP_VBI_MSG_INFO("[MMAP][VBI]layer is %u\n",
			of_vbi_mmap_info.layer);
		SRCCAP_VBI_MSG_INFO("[MMAP][VBI]miu is %u\n",
			of_vbi_mmap_info.miu);
		SRCCAP_VBI_MSG_INFO("[MMAP][VBI]is_cache is %u\n",
			of_vbi_mmap_info.is_cache);
		SRCCAP_VBI_MSG_INFO("[MMAP][VBI]cma_id is %u\n",
			of_vbi_mmap_info.cma_id);
		SRCCAP_VBI_MSG_INFO("[MMAP][VBI]start_bus_address is 0x%llX\n",
			of_vbi_mmap_info.start_bus_address);
		SRCCAP_VBI_MSG_INFO("[MMAP][VBI]buffer_size is 0x%llX\n",
			of_vbi_mmap_info.buffer_size);
		srccap_vbi_info.vbiBufAddr = of_vbi_mmap_info.start_bus_address - u64vbiemi0_base;
		srccap_vbi_info.vbiBufSize = of_vbi_mmap_info.buffer_size;
		srccap_vbi_info.vbiBufHeapId = of_vbi_mmap_info.cma_id;
		SRCCAP_VBI_MSG_INFO(
			"\x1b[31m[MMAP][VBI]bufAddr = 0x%llx, bufsize = 0x%llx \x1b[0m\n",
		srccap_vbi_info.vbiBufAddr, srccap_vbi_info.vbiBufSize);

	} else {
		SRCCAP_VBI_MSG_ERROR("[MMAP][VBI]Get vbi mmap info fail\n");
	}

	//Only support non-cache.
	vaddr = ioremap_wc(of_vbi_mmap_info.start_bus_address, of_vbi_mmap_info.buffer_size);

	if (!vaddr) {
		SRCCAP_VBI_MSG_ERROR("mtk mmap failed to vbi vaddr\n");
		return -ENXIO;
	}

	srccap_vbi_info.VBICCvirAddress = vaddr;
	SRCCAP_VBI_MSG_INFO("[MMAP][VBI][%d] kernel vaddr is 0x%lX\n", i, vaddr);

	mtk_vbi_SetBufferAddr(vaddr, srccap_vbi_info);

	vbi_misc_dev.minor = MISC_DYNAMIC_MINOR;
	vbi_misc_dev.name = "vbi_mmap";
	vbi_misc_dev.fops = &srccap_mmap_vbi_fops;


	ret = misc_register(&vbi_misc_dev);
	if (ret < 0) {
		SRCCAP_VBI_MSG_ERROR("mtk mmap failed to register vbi misc dev\n");
		return ret;
	}

	vbideviceinitialized = 1;
	return 0;
}

static int srccap_parse_dts_buf(
	struct mtk_srccap_dev *srccap_dev,
	struct platform_device *pdev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (pdev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	enum V4L2_AVD_MEM_TYPE type;

	type = srccap_dev->avd_info.cus_setting.memtype;

	switch (type) {
	case V4L2_MEMORY_TYPE_MMAP:
		ret = srccap_parse_dts_vd_mmap_buf(srccap_dev);
		ret |= srccap_parse_dts_vbi_mmap_buf(srccap_dev);
		break;
#if defined(CONFIG_CMA) || defined(CONFIG_DD_USE_CMA)
	case V4L2_MEMORY_TYPE_CMA:
		ret = srccap_parse_dts_vd_cma_buf(srccap_dev, pdev);
		ret |= srccap_parse_dts_vbi_cma_buf(pdev);
		break;
#endif
	default:
		ret = ENOENT;
		break;
	}
	return ret;
}

static int srccap_parse_dts_cap(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u32 val = 0;
	struct device_node *cap_node;
	struct device_node *color_fmt_node;
	u32 support_cnt = 0;
	int i = 0;

	if (srccap_dev->dev->of_node != NULL)
		of_node_get(srccap_dev->dev->of_node);

	cap_node = of_find_node_by_name(srccap_dev->dev->of_node, "capability");
	if (cap_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	/* external use */
	ret |= srccap_read_dts_u32(cap_node, "ipdma_count", &val);
	srccap_dev->srccap_info.cap.ipdma_cnt = (u8)val;
	ret |= srccap_read_dts_u32(cap_node, "hdmi_count", &val);
	srccap_dev->srccap_info.cap.hdmi_cnt = (u8)val;
	ret |= srccap_read_dts_u32(cap_node, "cvbs_count", &val);
	srccap_dev->srccap_info.cap.cvbs_cnt = (u8)val;
	ret |= srccap_read_dts_u32(cap_node, "svideo_count", &val);
	srccap_dev->srccap_info.cap.svideo_cnt = (u8)val;
	ret |= srccap_read_dts_u32(cap_node, "ypbpr_count", &val);
	srccap_dev->srccap_info.cap.ypbpr_cnt = (u8)val;
	ret |= srccap_read_dts_u32(cap_node, "vga_count", &val);
	srccap_dev->srccap_info.cap.vga_cnt = (u8)val;
	ret |= srccap_read_dts_u32(cap_node, "atv_count", &val);
	srccap_dev->srccap_info.cap.atv_cnt = (u8)val;
	ret |= srccap_read_dts_u32(cap_node, "dscl_scaling_cap", &val);
	srccap_dev->srccap_info.cap.dscl_scaling_cap = (u8)val;
	ret |= srccap_read_dts_u32(cap_node, "hw_timestamp_max_value", &val);
	srccap_dev->srccap_info.cap.hw_timestamp_max_value = val;
	/* internal use */
	ret |= srccap_read_dts_u32(cap_node, "hw_version",
		&srccap_dev->srccap_info.cap.hw_version);
	ret |= srccap_read_dts_u32(cap_node, "xtal_clk",
		&srccap_dev->srccap_info.cap.xtal_clk);
	ret |= srccap_read_dts_u32(cap_node, "bit_per_word",
		&srccap_dev->srccap_info.cap.bpw);
	ret |= srccap_read_dts_u32(cap_node, "IRQ_Version",
		&srccap_dev->srccap_info.cap.u32IRQ_Version);
	ret |= srccap_read_dts_u32(cap_node, "crop_align",
		&srccap_dev->srccap_info.cap.crop_align);
	ret |= srccap_read_dts_u32(cap_node, "DV_Srccap_HWVersion",
		&srccap_dev->srccap_info.cap.u32DV_Srccap_HWVersion);
	ret |= srccap_read_dts_u32(cap_node, "HDMI_Srccap_HWVersion",
		&srccap_dev->srccap_info.cap.u32HDMI_Srccap_HWVersion);
	ret |= srccap_read_dts_u32(cap_node, "adc_multi_src_max_num",
		&srccap_dev->srccap_info.cap.adc_multi_src_max_num);

	of_node_get(cap_node);

	color_fmt_node = of_find_node_by_name(cap_node, "color_fmt");
	if (color_fmt_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		of_node_put(cap_node);
		return -ENOENT;
	}

	ret |= srccap_read_dts_u32(color_fmt_node, "yuv_single_win_recommended_format", &val);
	srccap_dev->srccap_info.cap.color_fmt.yuv_single_win_recommended_format = val;
	ret |= srccap_read_dts_u32(color_fmt_node, "yuv_multi_win_recommended_format", &val);
	srccap_dev->srccap_info.cap.color_fmt.yuv_multi_win_recommended_format = val;
	ret |= srccap_read_dts_u32(color_fmt_node, "rgb_single_win_recommended_format", &val);
	srccap_dev->srccap_info.cap.color_fmt.rgb_single_win_recommended_format = val;
	ret |= srccap_read_dts_u32(color_fmt_node, "rgb_multi_win_recommended_format", &val);
	srccap_dev->srccap_info.cap.color_fmt.rgb_multi_win_recommended_format = val;

	ret |= srccap_read_dts_u32(color_fmt_node, "support_cnt", &support_cnt);
	srccap_dev->srccap_info.cap.color_fmt.support_cnt = support_cnt;

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		of_node_put(color_fmt_node);
		of_node_put(cap_node);
		return -ENOENT;
	}

	srccap_dev->srccap_info.cap.color_fmt.color_fmt_support
			= kcalloc(support_cnt, sizeof(u32), GFP_KERNEL);
	if (srccap_dev->srccap_info.cap.color_fmt.color_fmt_support == NULL) {
		SRCCAP_MSG_ERROR("%s Failed to kcalloc color_fmt_support\r\n", __func__);
		of_node_put(color_fmt_node);
		of_node_put(cap_node);
		return -EINVAL;
	}

	ret = srccap_read_dts_u32_array(color_fmt_node, "support",
		srccap_dev->srccap_info.cap.color_fmt.color_fmt_support, support_cnt);
	if (ret < 0) {
		SRCCAP_MSG_ERROR("%s Failed to get color fmt support\r\n", __func__);
		kfree(srccap_dev->srccap_info.cap.color_fmt.color_fmt_support);
		of_node_put(color_fmt_node);
		of_node_put(cap_node);
		return -EINVAL;
	}

	for (i = 0; i < support_cnt; i++)
		SRCCAP_MSG_INFO("i:%d, color_fmt_support:%x\n",
			i, srccap_dev->srccap_info.cap.color_fmt.color_fmt_support[i]);

	of_node_put(color_fmt_node);
	of_node_put(cap_node);
	return ret;
}

static int srccap_parse_dts_portmap(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	int i = 0;
	u32 input_num = 0;
	char input[SRCCAP_STRING_SIZE_10] = {0};
	const char *out_str = NULL;
	u32 val = 0;
	enum v4l2_srccap_input_source src = V4L2_SRCCAP_INPUT_SOURCE_NONE;
	u32 max_input_num = V4L2_SRCCAP_INPUT_SOURCE_NUM;
	struct device_node *map_node;
	struct device_node *input_node;

	map_node = of_find_node_by_name(NULL, "port_map");
	if (map_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	ret |= srccap_read_dts_u32(map_node, "input_num", &input_num);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		of_node_put(map_node);
		return -ENOENT;
	}

	if (input_num > max_input_num) {
		of_node_put(map_node);
		return -E2BIG;
	}

	for (i = 0; i < input_num; i++) {
		memset(input, 0, sizeof(input));
		snprintf(input, sizeof(input), "input%d", i);

		of_node_get(map_node);
		/* find the i-th input and store its data and sync ports in */
		/* the mapping array and indicate the specific input's      */
		/* position in the array with source type                   */
		input_node = of_find_node_by_name(map_node, input);
		if (input_node == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			of_node_put(map_node);
			return -ENOENT;
		}

		out_str = srccap_read_dts_string(input_node, "source");
		if (out_str == NULL) {
			of_node_put(input_node);
			continue;
		}

		val = srccap_get_input_source_num(out_str);
		if (val >= max_input_num) {
			of_node_put(input_node);
			of_node_put(map_node);
			return -E2BIG;
		}

		src = val;
		out_str = srccap_read_dts_string(input_node, "data_port");
		if (out_str == NULL) {
			srccap_dev->srccap_info.map[src].data_port = SRCCAP_INPUT_PORT_NONE;
			srccap_dev->srccap_info.map[src].sync_port = SRCCAP_INPUT_PORT_NONE;
			of_node_put(input_node);
			continue;
		}

		val = 0;
		val = srccap_get_input_port_num(out_str);
		if (val < 0) {
			srccap_dev->srccap_info.map[src].data_port = SRCCAP_INPUT_PORT_NONE;
			srccap_dev->srccap_info.map[src].sync_port = SRCCAP_INPUT_PORT_NONE;
			of_node_put(input_node);
			continue;
		} else {
			srccap_dev->srccap_info.map[src].data_port = (enum srccap_input_port)val;
		}

		out_str = srccap_read_dts_string(input_node, "sync_port");
		if (out_str == NULL) {
			srccap_dev->srccap_info.map[src].sync_port = SRCCAP_INPUT_PORT_NONE;
			of_node_put(input_node);
			continue;
		}

		val = 0;
		val = srccap_get_input_port_num(out_str);
		if (val < 0) {
			srccap_dev->srccap_info.map[src].sync_port = SRCCAP_INPUT_PORT_NONE;
			of_node_put(input_node);
			continue;
		} else {
			srccap_dev->srccap_info.map[src].sync_port = (enum srccap_input_port)val;
		}

		of_node_put(input_node);
	}

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		of_node_put(map_node);
		return -ENOENT;
	}

	of_node_put(map_node);
	return ret;
}

static int srccap_parse_dts_timingtable(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0, index = 0;
	int offset_hfreqx10 = 0, offset_vfreqx1000 = 0, offset_hstart = 0, offset_vstart = 0,
		offset_hde = 0, offset_vde = 0, offset_htt = 0, offset_vtt = 0,
		offset_adcphase = 0, offset_status = 0;
	struct device_node *timingtable_node = NULL;
	u32 timing_num = 0, array_size = 0;
	u32 *timings = NULL;
	struct srccap_timingdetect_timing *timing_table = NULL;

	/* parse timing table info */
	timingtable_node = of_find_node_by_name(NULL, "timing_table");
	if (timingtable_node == NULL) {
		SRCCAP_MSG_ERROR("failed to find timing table device node\n");
		ret = -ENOENT;
		goto EXIT;
	}

	ret = srccap_read_dts_u32(timingtable_node, "timing_num", &timing_num);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		ret = -ENOENT;
		goto EXIT;
	}

	array_size = timing_num * SRCCAP_TIMINGDETECT_TIMING_INFO_NUM;
	timings = kcalloc(array_size, sizeof(u32), GFP_KERNEL);
	if (timings == NULL) {
		SRCCAP_MSG_ERROR("failed to kcalloc timings\n");
		ret = -ENOSPC;
		goto EXIT;
	}

	ret = srccap_read_dts_u32_array(timingtable_node, "timings", timings, array_size);
	if (ret < 0) {
		SRCCAP_MSG_ERROR("failed to parse timingtable\n");
		ret = -ENOENT;
		goto FREE_TIMINGS;
	}

	/* store timings into timing table */
	timing_table = kcalloc(timing_num, sizeof(struct srccap_timingdetect_timing), GFP_KERNEL);
	if (timing_table == NULL) {
		SRCCAP_MSG_ERROR("failed to kcalloc timing_table\n");
		ret = -ENOSPC;
		goto FREE_TIMINGS;
	}

	for (index = 0; index < timing_num; index++) {
		offset_hfreqx10 = (index * SRCCAP_TIMINGDETECT_TIMING_INFO_NUM);
		offset_vfreqx1000 = offset_hfreqx10 + 1;
		offset_hstart = offset_vfreqx1000 + 1;
		offset_vstart = offset_hstart + 1;
		offset_hde = offset_vstart + 1;
		offset_vde = offset_hde + 1;
		offset_htt = offset_vde + 1;
		offset_vtt = offset_htt + 1;
		offset_adcphase = offset_vtt + 1;
		offset_status = offset_adcphase + 1;

		timing_table[index].hfreqx10 = timings[offset_hfreqx10];
		timing_table[index].vfreqx1000 = timings[offset_vfreqx1000];
		timing_table[index].hstart = (u16)timings[offset_hstart];
		timing_table[index].vstart = (u16)timings[offset_vstart];
		timing_table[index].hde = (u16)timings[offset_hde];
		timing_table[index].vde = (u16)timings[offset_vde];
		timing_table[index].htt = (u16)timings[offset_htt];
		timing_table[index].vtt = (u16)timings[offset_vtt];
		timing_table[index].adcphase = (u16)timings[offset_adcphase];
		timing_table[index].status = (u16)timings[offset_status];

		SRCCAP_MSG_INFO("%s[%d] = <%u %u %u %u %u %u %u %u %u 0x%x>\n",
			"timing_table", index,
			timing_table[index].hfreqx10,
			timing_table[index].vfreqx1000,
			timing_table[index].hstart,
			timing_table[index].vstart,
			timing_table[index].hde,
			timing_table[index].vde,
			timing_table[index].htt,
			timing_table[index].vtt,
			timing_table[index].adcphase,
			timing_table[index].status);
	}

	srccap_dev->timingdetect_info.table_entry_count = (u16)timing_num;
	srccap_dev->timingdetect_info.timing_table = timing_table;

FREE_TIMINGS:
	kfree(timings);
EXIT:
	if (timingtable_node != NULL)
		of_node_put(timingtable_node);

	return ret;
}

static int srccap_parse_dts_vdsampling(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct device_node *vdsampling_node = NULL;
	struct srccap_timingdetect_vdsampling *sampling_table = NULL;
	u32 *samplings = NULL;
	u32 vdsampling_num = 0, array_size = 0, sampling_num = 0;
	int ret = 0, index = 0;
	int offset_videotype = 0, offset_vst = 0, offset_hst = 0, offset_vde = 0, offset_hde = 0,
		offset_htt = 0, offset_vfreq = 0;

	// find node
	vdsampling_node = of_find_node_by_name(NULL, "sampling_table");
	if (vdsampling_node == NULL) {
		SRCCAP_MSG_ERROR("failed to find vd sampling table device node\n");
		ret = -ENOENT;
		goto EXIT;
	}

	// get entry number from table
	ret = srccap_read_dts_u32(vdsampling_node, "sampling_num", &sampling_num);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		ret = -ENOENT;
		goto EXIT;
	}

	// calculate array size
	array_size = sampling_num * SRCCAP_VD_SAMPLING_INFO_NUM;

	samplings = kcalloc(array_size, sizeof(u32), GFP_KERNEL);
	if (samplings == NULL) {
		SRCCAP_MSG_ERROR("failed to kcalloc samplings\n");
		ret = -ENOSPC;
		goto EXIT;
	}

	// get table and cpy into samplings
	ret = srccap_read_dts_u32_array(vdsampling_node, "samplings", samplings, array_size);
	if (ret < 0) {
		SRCCAP_MSG_ERROR("failed to parse timingtable\n");
		ret = -ENOENT;
		goto FREE_MEM;
	}

	// create table structure pointer
	sampling_table = kcalloc(sampling_num, sizeof(struct srccap_timingdetect_vdsampling),
		GFP_KERNEL);
	if (sampling_table == NULL) {
		SRCCAP_MSG_ERROR("failed to kcalloc timing_table\n");
		ret = -ENOSPC;
		goto FREE_MEM;
	}

	// save table into structure pointer
	for (index = 0; index < sampling_num; index++) {
		offset_videotype = (index * SRCCAP_VD_SAMPLING_INFO_NUM);
		offset_vst = offset_videotype + 1;
		offset_hst = offset_vst + 1;
		offset_hde = offset_hst + 1;
		offset_vde = offset_hde + 1;
		offset_htt = offset_vde + 1;
		offset_vfreq = offset_htt + 1;

		sampling_table[index].videotype = (u16)samplings[offset_videotype];
		sampling_table[index].v_start = (u16)samplings[offset_vst];
		sampling_table[index].h_start = (u16)samplings[offset_hst];
		sampling_table[index].h_de = (u16)samplings[offset_hde];
		sampling_table[index].v_de = (u16)samplings[offset_vde];
		sampling_table[index].h_total = (u16)samplings[offset_htt];
		sampling_table[index].v_freqx1000 = (u16)samplings[offset_vfreq];

		SRCCAP_MSG_INFO("%s[%d] = <%u, %u, %u, %u, %u, %u, %u>\n",
			"vdsampling_table", index,
			sampling_table[index].videotype,
			sampling_table[index].v_start,
			sampling_table[index].h_start,
			sampling_table[index].h_de,
			sampling_table[index].v_de,
			sampling_table[index].h_total,
			sampling_table[index].v_freqx1000);
	}

	srccap_dev->timingdetect_info.vdsampling_table_entry_count = (u16)sampling_num;
	srccap_dev->timingdetect_info.vdsampling_table = sampling_table;
	srccap_dev->avd_info.vdsampling_table_entry_count = (u16)sampling_num;
	srccap_dev->avd_info.vdsampling_table = (struct srccap_avd_vd_sampling *)sampling_table;


FREE_MEM:
	kfree(samplings);
EXIT:
	if (vdsampling_node != NULL)
		of_node_put(vdsampling_node);

	return ret;
}

static int srccap_parse_dts_vdcussetting(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u32 val = 0;
	struct device_node *cus_node;
	struct device_node *vd_node;

	if (srccap_dev->dev->of_node != NULL)
		of_node_get(srccap_dev->dev->of_node);

	cus_node = of_find_node_by_name(srccap_dev->dev->of_node, "vd_customer_setting");
	if (cus_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOENT;
	}

	/* vd_sensibility */
	of_node_get(cus_node);
	vd_node = of_find_node_by_name(cus_node, "vd_sensibility");
	if (vd_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		of_node_put(cus_node);
		return -ENOENT;
	}
	ret |= srccap_read_dts_u32(vd_node, "vd_init", &val);
	srccap_dev->avd_info.cus_setting.sensibility.init = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_normal_detect_win_before_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.normal_detect_win_before_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_noamrl_detect_win_after_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.noamrl_detect_win_after_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_normal_cntr_fail_before_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.normal_cntr_fail_before_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_normal_cntr_sync_before_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.normal_cntr_sync_before_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_normal_cntr_sync_after_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.normal_cntr_sync_after_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_chan_scan_detect_win_before_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.scan_detect_win_before_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_chan_scan_detect_win_after_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.scan_detect_win_after_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_chan_scan_cntr_fail_before_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.scan_cntr_fail_before_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_chan_scan_cntr_sync_before_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.scan_cntr_sync_before_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_chan_scan_cntr_sync_after_lock", &val);
	srccap_dev->avd_info.cus_setting.sensibility.scan_cntr_sync_after_lock = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_hsen_chan_scan_hsync_check_count", &val);
	srccap_dev->avd_info.cus_setting.sensibility.scan_hsync_check_count = val;
	of_node_put(vd_node);

	/* vd_patchflag */
	of_node_get(cus_node);
	vd_node = of_find_node_by_name(cus_node, "vd_patchflag");
	if (vd_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		of_node_put(cus_node);
		return -ENOENT;
	}

	ret |= srccap_read_dts_u32(vd_node, "vd_init", &val);
	srccap_dev->avd_info.cus_setting.patchflag.init = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_init_patch_flag", &val);
	srccap_dev->avd_info.cus_setting.patchflag.init_patch_flag = val;
	of_node_put(vd_node);

	/* vd_factory */
	of_node_get(cus_node);
	vd_node = of_find_node_by_name(cus_node, "vd_factory");
	if (vd_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		of_node_put(cus_node);
		return -ENOENT;
	}
	ret |= srccap_read_dts_u32(vd_node, "vd_init", &val);
	srccap_dev->avd_info.cus_setting.factory.init = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_fix_gain", &val);
	srccap_dev->avd_info.cus_setting.factory.fix_gain = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_fine_gain", &val);
	srccap_dev->avd_info.cus_setting.factory.fine_gain = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_color_kill_high_bound", &val);
	srccap_dev->avd_info.cus_setting.factory.color_kill_high_bound = val;
	ret |= srccap_read_dts_u32(vd_node, "vd_color_kill_low_bound", &val);
	srccap_dev->avd_info.cus_setting.factory.color_kill_low_bound = val;
	of_node_put(vd_node);

	/* vd_memory_type */
	of_node_get(cus_node);
	vd_node = of_find_node_by_name(cus_node, "vd_memory_type");
	if (vd_node == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		of_node_put(cus_node);
		return -ENOENT;
	}
	ret |= srccap_read_dts_u32(vd_node, "memory_type", &val);
	srccap_dev->avd_info.cus_setting.memtype = (enum V4L2_AVD_MEM_TYPE)val;
	of_node_put(vd_node);

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		of_node_put(cus_node);
		return -ENOENT;
	}

	of_node_put(cus_node);
	return ret;
}

static int srccap_parse_dts(
	struct mtk_srccap_dev *srccap_dev,
	struct platform_device *pdev)
{
	int ret = 0;
	struct device *property_dev = &pdev->dev;

	/* parse ioremap */
	ret |= srccap_ioremap(srccap_dev, property_dev);

	/* parse clock */
	ret |= srccap_parse_dts_clk(srccap_dev);

	/* parse vd customer setting */
	ret |= srccap_parse_dts_vdcussetting(srccap_dev);

	/* parse buffer */
	ret |= srccap_parse_dts_buf(srccap_dev, pdev);

	/* parse capability */
	ret |= srccap_parse_dts_cap(srccap_dev);

	/* parse port map */
	ret |= srccap_parse_dts_portmap(srccap_dev);

	/* parse timing table */
	ret |= srccap_parse_dts_timingtable(srccap_dev);

	/* parse vd sampling*/
	ret |= srccap_parse_dts_vdsampling(srccap_dev);

	return ret;
}

static int srccap_create_sysfs(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;

	dev_info(srccap_dev->dev, "Create device attribute files");
	if (srccap_dev->dev_id == 0) {
		srccap_dev->mtkdbg_kobj = kobject_create_and_add("mtk_dbg", &srccap_dev->dev->kobj);
		ret = sysfs_create_group(srccap_dev->mtkdbg_kobj, &mtk_srccap_attr_0_group);
		if (ret) {
			dev_err(srccap_dev->dev,
				"[%s][%d]Fail to create sysfs files: %d\n",
				__func__, __LINE__, ret);
			return ret;
		}
	} else if (srccap_dev->dev_id == 1) {
		srccap_dev->mtkdbg_kobj = kobject_create_and_add("mtk_dbg", &srccap_dev->dev->kobj);
		ret = sysfs_create_group(srccap_dev->mtkdbg_kobj, &mtk_srccap_attr_1_group);
		if (ret) {
			dev_err(srccap_dev->dev,
				"[%s][%d]Fail to create sysfs files: %d\n",
				__func__, __LINE__, ret);
			return ret;
		}
	} else
		SRCCAP_MSG_ERROR("device id not found\n");

	return 0;
}

static int srccap_remove_sysfs(struct mtk_srccap_dev *srccap_dev)
{
	dev_info(srccap_dev->dev, "Remove device attribute files");
	if (srccap_dev->dev_id == 0) {
		sysfs_remove_group(srccap_dev->mtkdbg_kobj,
		&mtk_srccap_attr_0_group);
		kobject_del(srccap_dev->mtkdbg_kobj);
		srccap_dev->mtkdbg_kobj = NULL;
	} else {
		sysfs_remove_group(srccap_dev->mtkdbg_kobj,
		&mtk_srccap_attr_1_group);
		kobject_del(srccap_dev->mtkdbg_kobj);
		srccap_dev->mtkdbg_kobj = NULL;
	}
	return 0;
}


static void srccap_clear_info(struct mtk_srccap_dev *srccap_dev)
{
	memset(&srccap_dev->adc_info, 0, sizeof(struct srccap_adc_info));
	memset(&srccap_dev->avd_info, 0, sizeof(struct srccap_avd_info));
	memset(&srccap_dev->srccap_info, 0, sizeof(struct srccap_info));
	memset(&srccap_dev->common_info, 0, sizeof(struct srccap_common_info));
	memset(&srccap_dev->dscl_info, 0, sizeof(struct srccap_dscl_info));
	memset(&srccap_dev->hdmirx_info, 0, sizeof(struct srccap_hdmirx_info));
	memset(&srccap_dev->memout_info, 0, sizeof(struct srccap_memout_info));
#ifdef NO_DEFINE
	memset(&srccap_dev->mux_info, 0, sizeof(struct srccap_mux_info));
#endif
	memset(&srccap_dev->timingdetect_info, 0, sizeof(struct srccap_timingdetect_info));
	memset(&srccap_dev->vbi_info, 0, sizeof(struct srccap_vbi_info));
}

static int srccap_init_clock(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = mtk_timingdetect_init_clock(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	return 0;
}

static int srccap_init_waitqueue(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	init_waitqueue_head(&srccap_dev->srccap_info.waitq_list.waitq_buffer);

	return 0;
}

static void srccap_init_mutex(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	mutex_init(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
	mutex_init(&srccap_dev->srccap_info.mutex_list.mutex_irq_info);
	mutex_init(&srccap_dev->srccap_info.mutex_list.mutex_pqmap_dscl);
	mutex_init(&srccap_dev->srccap_info.mutex_list.mutex_timing_monitor);
	mutex_init(&srccap_dev->srccap_info.mutex_list.mutex_sync_monitor);
	mutex_init(&srccap_dev->srccap_info.mutex_list.mutex_online_source);
}

static void srccap_init_spinlock(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	spin_lock_init(&srccap_dev->srccap_info.spinlock_list.spinlock_buffer_handling_task);
}

static int srccap_init_menuload(struct mtk_srccap_dev *srccap_dev)
{
	enum sm_return_type ml_ret = E_SM_RET_FAIL;
	int ml_fd;
	struct sm_ml_res ml_res;
	uint8_t ml_index = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* allocate menuload buffer for dscl */
	memset(&ml_res, 0, sizeof(struct sm_ml_res));
	ml_res.sync_id = (srccap_dev->dev_id == 0) ? E_SM_ML_SRC0_SYNC : E_SM_ML_SRC1_SYNC;
	ml_res.buffer_cnt = SRCCAP_MEMOUT_FRAME_NUM;
	ml_res.cmd_cnt = SRCCAP_MENULOAD_CMD_COUNT;
	ml_ret = sm_ml_create_resource(&ml_fd, &ml_res);
	if (ml_ret != E_SM_RET_OK) {
		pr_err("[SRCCAP][%s][%d]can not create instance!", __func__, __LINE__);
		return -EINVAL;
	}

	srccap_dev->srccap_info.ml_info.fd_dscl = ml_fd;

	/* allocate menuload buffer for memout */
	memset(&ml_res, 0, sizeof(struct sm_ml_res));
	ml_res.sync_id = (srccap_dev->dev_id == 0) ? E_SM_ML_SRC0_SYNC : E_SM_ML_SRC1_SYNC;
	ml_res.buffer_cnt = SRCCAP_MEMOUT_FRAME_NUM;
	ml_res.cmd_cnt = SRCCAP_MENULOAD_CMD_COUNT;
	ml_ret = sm_ml_create_resource(&ml_fd, &ml_res);
	if (ml_ret != E_SM_RET_OK) {
		pr_err("[SRCCAP][%s][%d]can not create instance!", __func__, __LINE__);
		return -EINVAL;
	}

	srccap_dev->srccap_info.ml_info.fd_memout = ml_fd;

	return 0;
}

static int srccap_deinit_menuload(struct mtk_srccap_dev *srccap_dev)
{
	enum sm_return_type ml_ret = E_SM_RET_FAIL;

	ml_ret = sm_ml_destroy_resource(srccap_dev->srccap_info.ml_info.fd_dscl);
	if (ml_ret != E_SM_RET_OK) {
		pr_err("[SRCCAP][%s][%d]can not destroy instance!", __func__, __LINE__);
		return -EINVAL;
	}

	srccap_dev->srccap_info.ml_info.fd_dscl = 0;

	ml_ret = sm_ml_destroy_resource(srccap_dev->srccap_info.ml_info.fd_memout);
	if (ml_ret != E_SM_RET_OK) {
		pr_err("[SRCCAP][%s][%d]can not destroy instance!", __func__, __LINE__);
		return -EINVAL;
	}

	srccap_dev->srccap_info.ml_info.fd_memout = 0;

	return 0;
}

static int srccap_deinit_clock(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = mtk_timingdetect_deinit_clock(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -ENOENT;
	}

	return ret;
}

static int srccap_init(struct mtk_srccap_dev *srccap_dev, struct platform_device *pdev)
{
	int ret = 0;

	srccap_init_mutex(srccap_dev);

	ret = srccap_init_waitqueue(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

#if IS_ENABLED(CONFIG_OPTEE)
	ret = mtk_memout_svp_init(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
#endif

	ret = _mtk_dv_init(srccap_dev);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

	ret = mtk_common_init_triggergen(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_common_init_irq(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = srccap_init_menuload(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* initialize hardware once only */
	if (srccap_dev->dev_id != 0)
		return 0;

	ret = srccap_init_clock(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_timingdetect_init(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	ret = mtk_adc_init(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
#endif
	ret = mtk_hdmirx_init(srccap_dev, pdev, hdmirxloglevel);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_srccap_common_pq_hwreg_init(srccap_dev);

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_srccap_common_vd_pq_hwreg_init(srccap_dev);

	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

static int srccap_deinit(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	if (srccap_dev->dev_id == 0) {
		mtk_hdmirx_deinit(srccap_dev);
	}

	mtk_timingdetect_deinit(srccap_dev);
	//mtk_srccap_common_deinit_pqmap(srccap_dev);

	ret = srccap_deinit_menuload(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_srccap_common_pq_hwreg_deinit(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_srccap_common_vd_pq_hwreg_deinit(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	kfree(srccap_dev->srccap_info.cap.color_fmt.color_fmt_support);

	return ret;
}

static void srccap_free_mem(void *ptr)
{
	kfree(ptr);
	ptr = NULL;
}

static void srccap_release_memory(struct mtk_srccap_dev *srccap_dev)
{
	srccap_free_mem((void *)srccap_dev->timingdetect_info.vdsampling_table);
	srccap_free_mem((void *)srccap_dev->timingdetect_info.timing_table);
	srccap_free_mem((void *)srccap_dev->avd_info.vdsampling_table);
}

static int srccap_register_subdev(
	struct v4l2_device *v4l2_dev,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = mtk_srccap_register_adc_subdev(
		v4l2_dev,
		&srccap_dev->subdev_adc,
		&srccap_dev->adc_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register adc subdev\033[0m\n");
		goto EXIT;
	}
	ret = mtk_srccap_register_avd_subdev(
		v4l2_dev,
		&srccap_dev->subdev_avd,
		&srccap_dev->avd_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register avd subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_ADC;
	}
	ret = mtk_srccap_register_common_subdev(
		v4l2_dev,
		&srccap_dev->subdev_common,
		&srccap_dev->common_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register common subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_AVD;
	}
	ret = mtk_srccap_register_dscl_subdev(
		v4l2_dev,
		&srccap_dev->subdev_dscl,
		&srccap_dev->dscl_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register dscl subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_COMMON;
	}
	ret = mtk_srccap_register_hdmirx_subdev(
		v4l2_dev,
		&srccap_dev->subdev_hdmirx,
		&srccap_dev->hdmirx_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register hdmirx subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_DSCL;
	}
	ret = mtk_srccap_register_memout_subdev(
		v4l2_dev,
		&srccap_dev->subdev_memout,
		&srccap_dev->memout_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register memout subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_HDMIRX;
	}
	ret = mtk_srccap_register_mux_subdev(
		v4l2_dev,
		&srccap_dev->subdev_mux,
		&srccap_dev->mux_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register mux subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_MEMOUT;
	}
	ret = mtk_srccap_register_timingdetect_subdev(
		v4l2_dev,
		&srccap_dev->subdev_timingdetect,
		&srccap_dev->timingdetect_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register timingdetect subdev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_MUX;
	}
	ret = mtk_srccap_register_vbi_subdev(
		v4l2_dev,
		&srccap_dev->subdev_vbi,
		&srccap_dev->vbi_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register vbi sub dev\033[0m\n");
		goto UNREGISTER_SUBDEVICE_TIMINGDETECT;
	}

	return 0;

UNREGISTER_SUBDEVICE_TIMINGDETECT:
	mtk_srccap_unregister_timingdetect_subdev(&srccap_dev->subdev_timingdetect);
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
EXIT:

	return ret;
}

static int srccap_probe(struct platform_device *pdev)
{
	boottime_print("MTK Srccap insmod [begin]\n");
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;
	struct v4l2_device *v4l2_dev = NULL;
	const struct of_device_id *of_id = NULL;
	u32 ctrl_count = 0;
	u32 ctrl_num = sizeof(srccap_ctrl)/sizeof(struct v4l2_ctrl_config);
	u32 vdev_num = 0;

	pr_emerg("Probe starts\n");

	srccap_dev = devm_kzalloc(&pdev->dev, sizeof(struct mtk_srccap_dev), GFP_KERNEL);
	if (!srccap_dev)
		return -ENOMEM;

	srccap_clear_info(srccap_dev);

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
	ret = srccap_parse_dts(srccap_dev, pdev);
	if (ret) {
		dev_err(&pdev->dev, "\033[1;31mfailed to parse srccap dts\033[0m\n");
		goto EXIT;
	}

	/* initialize srccap ctrl handler */
	v4l2_ctrl_handler_init(&srccap_dev->srccap_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(&srccap_dev->srccap_ctrl_handler,
			&srccap_ctrl[ctrl_count], NULL);
	}

	ret = srccap_dev->srccap_ctrl_handler.error;
	if (ret) {
		dev_err(&pdev->dev, "failed to create srccap ctrl handler\n");
		goto FREE_V4L2_CTRL_HANDLER;
	}

	/* register v4l2 device */
	v4l2_dev = &srccap_dev->v4l2_dev;
	v4l2_dev->ctrl_handler = &srccap_dev->srccap_ctrl_handler;
	ret = v4l2_device_register(&pdev->dev, v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "\033[1;31mfailed to register v4l2 device\033[0m\n");
		ret = -EINVAL;
		goto FREE_V4L2_CTRL_HANDLER;
	}

	/* initialize m2m device */
	srccap_dev->mdev = v4l2_m2m_init(&mtk_srccap_m2m_ops);
	if (IS_ERR(srccap_dev->mdev)) {
		ret = PTR_ERR(srccap_dev->mdev);
		v4l2_err(v4l2_dev, "failed to init m2m device\n");
		goto UNREGISTER_V4L2_DEVICE;
	}

	/* register srccap sub-devices */
	ret = srccap_register_subdev(v4l2_dev, srccap_dev);
	if (ret) {
		v4l2_err(v4l2_dev, "\033[1;31mfailed to register sub devices\033[0m\n");
		goto RELEASE_M2M_DEVICE;
	}

	/* allocate video device */
	srccap_dev->vdev = video_device_alloc();
	if (!srccap_dev->vdev) {
		ret = -ENOMEM;
		goto RELEASE_M2M_DEVICE;
	}

	srccap_dev->vdev->fops = &mtk_srccap_fops;
	srccap_dev->vdev->device_caps = SRCCAP_DEVICE_CAPS;
	srccap_dev->vdev->v4l2_dev = v4l2_dev;
	srccap_dev->vdev->vfl_dir = VFL_DIR_M2M;
	srccap_dev->vdev->minor = -1;
	srccap_dev->vdev->tvnorms = V4L2_STD_ALL;
	srccap_dev->vdev->release = video_device_release;
	srccap_dev->vdev->ioctl_ops = &mtk_srccap_ioctl_ops;
	srccap_dev->vdev->lock = &srccap_dev->mutex;

	/* register video device with assigned device node number if possible */
	ret = srccap_read_dts_u32(srccap_dev->dev->of_node, "device_node_num", &vdev_num);
	if ((ret < 0) || (vdev_num == SRCCAP_UNSPECIFIED_NODE_NUM))
		vdev_num = -1;

	ret = video_register_device(srccap_dev->vdev, VFL_TYPE_GRABBER, vdev_num);
	if (ret) {
		v4l2_err(v4l2_dev,
			"\033[1;31mfailed to register video device\033[0m\n");
		goto RELEASE_VIDEO_DEVICE;
	}

	/* write sysfs */
	srccap_create_sysfs(srccap_dev);

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
		"%s-vdev", SRCCAP_NAME);

	ret = srccap_init(srccap_dev, pdev);
	if (ret) {
		pr_err("\033[1;31mfailed to initialize srccap\033[0m\n");
		goto UNREGISTER_VIDEO_DEVICE;
	}

	dev_dbg(&pdev->dev, "successfully probed\n");
	pr_emerg("successfully probed\n");

	boottime_print("MTK Srccap insmod [end]\n");

	return 0;

UNREGISTER_VIDEO_DEVICE:
	video_unregister_device(srccap_dev->vdev);
RELEASE_VIDEO_DEVICE:
	video_device_release(srccap_dev->vdev);
RELEASE_M2M_DEVICE:
	v4l2_m2m_release(srccap_dev->mdev);
UNREGISTER_V4L2_DEVICE:
	v4l2_device_unregister(v4l2_dev);
FREE_V4L2_CTRL_HANDLER:
	v4l2_ctrl_handler_free(&srccap_dev->srccap_ctrl_handler);
EXIT:

	return ret;
}

static int srccap_remove(struct platform_device *pdev)
{
	struct mtk_srccap_dev *srccap_dev = platform_get_drvdata(pdev);

#if IS_ENABLED(CONFIG_OPTEE)
	mtk_memout_svp_exit(srccap_dev);
#endif

	srccap_deinit(srccap_dev);
	srccap_iounmap(srccap_dev);
	video_unregister_device(srccap_dev->vdev);
	v4l2_device_unregister(srccap_dev->vdev->v4l2_dev);
	video_device_release(srccap_dev->vdev);
	mtk_srccap_unregister_vbi_subdev(&srccap_dev->subdev_vbi);
	mtk_srccap_unregister_timingdetect_subdev(&srccap_dev->subdev_timingdetect);
	mtk_srccap_unregister_mux_subdev(&srccap_dev->subdev_mux);
	mtk_srccap_unregister_memout_subdev(&srccap_dev->subdev_memout);
	mtk_srccap_unregister_hdmirx_subdev(&srccap_dev->subdev_hdmirx);
	mtk_srccap_unregister_dscl_subdev(&srccap_dev->subdev_dscl);
	mtk_srccap_unregister_common_subdev(&srccap_dev->subdev_common);
	mtk_srccap_unregister_avd_subdev(&srccap_dev->subdev_avd);
	mtk_srccap_unregister_adc_subdev(&srccap_dev->subdev_adc);
	v4l2_m2m_release(srccap_dev->mdev);
	v4l2_ctrl_handler_free(&srccap_dev->srccap_ctrl_handler);
	srccap_remove_sysfs(srccap_dev);
	srccap_release_memory(srccap_dev);

	return 0;
}

static void srccap_process_suspend(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	ret = srccap_deinit_clock(srccap_dev);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

	ret = mtk_adc_deinit(srccap_dev);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

}

static void srccap_process_resume(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	if (srccap_dev->dev_id == 0) {
		ret = srccap_init_clock(srccap_dev);
		if (ret < 0)
			SRCCAP_MSG_RETURN_CHECK(ret);

		ret = mtk_timingdetect_init(srccap_dev);
		if (ret < 0)
			SRCCAP_MSG_RETURN_CHECK(ret);

		ret = mtk_adc_init(srccap_dev);
		if (ret < 0)
			SRCCAP_MSG_RETURN_CHECK(ret);
	}
}

static int __maybe_unused srccap_runtime_suspend(struct device *dev)
{
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	struct v4l2_event event;

	if (srccap_dev->dev_id == 0) {
		event.type = V4L2_SRCCAP_SUSPEND_EVENT;
		event.id = 0;
		v4l2_event_queue(srccap_dev->vdev, &event);
	}

	srccap_process_suspend(srccap_dev);
	mtk_avd_SetPowerState(srccap_dev, E_POWER_SUSPEND);
	mtk_hdmirx_SetPowerState(srccap_dev, E_POWER_SUSPEND);
	mtk_srccap_common_irq_suspend();

	return 0;
}

static int __maybe_unused srccap_runtime_resume(struct device *dev)
{
	struct mtk_srccap_dev *srccap_dev = dev_get_drvdata(dev);
	struct v4l2_event event;

	srccap_process_resume(srccap_dev);
	mtk_dv_resume(srccap_dev);
	mtk_hdmirx_SetPowerState(srccap_dev, E_POWER_RESUME);
	mtk_avd_SetPowerState(srccap_dev, E_POWER_RESUME);
	mtk_srccap_common_irq_resume();

	if (srccap_dev->dev_id == 0) {
		event.type = V4L2_SRCCAP_RESUME_EVENT;
		event.id = 0;
		v4l2_event_queue(srccap_dev->vdev, &event);
	}

	return 0;
}

static int __maybe_unused srccap_suspend(struct device *dev)
{
	return srccap_runtime_suspend(dev);
}

static int __maybe_unused srccap_resume(struct device *dev)
{
	return srccap_runtime_resume(dev);
}

static const struct dev_pm_ops srccap_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(srccap_suspend, srccap_resume)
	SET_RUNTIME_PM_OPS(srccap_runtime_suspend, srccap_runtime_resume, NULL)
};

MODULE_DEVICE_TABLE(of, mtk_srccap_match);

static struct platform_driver srccap_pdrv = {
	.probe = srccap_probe,
	.remove = srccap_remove,
	.driver = {
		.name = SRCCAP_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mtk_srccap_match,
		.pm = &srccap_pm_ops,
	},
};

//module_platform_driver(srccap_pdrv);

static int __init _mtk_srccap_init(void)
{
	boottime_print("MTK Srccap init [begin]\n");
	platform_driver_register(&srccap_pdrv);
	boottime_print("MTK Srccap init [end]\n");
	return 0;
}

static void __exit _mtk_srccap_exit(void)
{
	platform_driver_unregister(&srccap_pdrv);
}


module_init(_mtk_srccap_init);
module_exit(_mtk_srccap_exit);


MODULE_AUTHOR("MediaTek TV");
MODULE_DESCRIPTION("MTK Source Capture Driver");
MODULE_LICENSE("GPL");
