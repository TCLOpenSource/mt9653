// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/platform_device.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

#include "mtk_srccap.h"
#include "mtk_srccap_dv.h"

#include "hwreg_srccap_dv_descrb.h"

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define DV_DESCRB_PKT_LENGTH (128) // 128 byte
#define DV_DESCRB_PKT_HEADER (3)
#define DV_DESCRB_PKT_TAIL (4)
#define DV_DESCRB_PKT_META_LEN_SIZE (2)
#define DV_DESCRB_PKT_REMAIN_LENGTH \
	(DV_DESCRB_PKT_LENGTH \
	 - DV_DESCRB_PKT_HEADER \
	 - DV_DESCRB_PKT_TAIL \
	 - DV_DESCRB_PKT_META_LEN_SIZE)
#define DV_DESCRB_MAX_HW_SUPPORT_PKT (5)
#define DV_DESCRB_PKT_REPEAT_NUM (3)
#define DV_DESCRB_PKT_NUM \
	(DV_DESCRB_PKT_REPEAT_NUM \
	 * DV_DESCRB_MAX_HW_SUPPORT_PKT)
#define DV_DESCRB_META_LENGTH \
	(DV_DESCRB_PKT_LENGTH \
	 * DV_DESCRB_PKT_NUM)
#define DV_DESCRB_META_LEN_ADDR_HI (1)
#define DV_DESCRB_META_LEN_ADDR_LO (2)
#define DV_DESCRB_IEEE_OUI_VALUE (0x00D046)
#define DV_DESCRB_H14B_IEEE_OUI_VALUE (0x000C03)
#define DV_DESCRB_IEEE_OUI_LENGTH (3)
#define DV_DESCRB_EMP_IEEE_OUI_OFFSET (7)
#define DV_DESCRB_EMP_DATA_VERSION_OFFSET (10)
#define DV_DESCRB_EMP_DATA_VERSION_FORM1 (0)
#define DV_DESCRB_EMP_DATA_VERSION_FORM2 (1)
#define DV_DESCRB_EMP_HEADER_SIZE (7)
#define DV_DESCRB_EMP_PAYLOAD_SIZE (28)
#define DV_DESCRB_EMP_EDR_DATA_OFFSET (13)
#define DV_DESCRB_DRM_PKT_TYPE (0x87)
#define DV_DESCRB_DRM_VERSION (0x1)
#define DV_DESCRB_DRM_LENGTH (0x1A)
#define DV_DESCRB_DRM_EOTF_VALUE (0x2)
#define DV_DESCRB_DRM_STATIC_METADATA_ID_VALUE (0x0)
#define DV_DESCRB_DRM_PRIMARIES_X_R_VALUE (0x8A26)
#define DV_DESCRB_DRM_PRIMARIES_X_G_VALUE (0x2108)
#define DV_DESCRB_DRM_PRIMARIES_X_B_VALUE (0x19FA)
#define DV_DESCRB_DRM_PRIMARIES_Y_R_VALUE (0x3976)
#define DV_DESCRB_DRM_PRIMARIES_Y_G_VALUE (0x9B7E)
#define DV_DESCRB_DRM_PRIMARIES_Y_B_VALUE (0x0846)
#define DV_DESCRB_DRM_MAX_LUMA_VALUE (0x03E8)
#define DV_DESCRB_DRM_MIN_LUMA_VALUE (0x0032)
#define DV_DESCRB_DRM_WHITE_POINT_X_VALUE (0x3DEE)
#define DV_DESCRB_DRM_WHITE_POINT_Y_VALUE (0x4030)
#define DV_DESCRB_DRM_MAX_CLL_VALUE (0x03FE) //Content Light Level
#define DV_DESCRB_DRM_MAX_FALL_VALUE (0x03AA) //Frame Average Light Level
#define INDEX_0 (0)
#define INDEX_1 (1)
#define INDEX_2 (2)
#define DV_DESCRB_VSIF_PKT_TYPE (0x81)
#define DV_DESCRB_VSIF_VERSION (0x1)
#define DV_DESCRB_VSIF_LENGTH (0x1B)
#define DV_DESCRB_H14B_VSIF_LENGTH (0x18)
#define DV_DESCRB_VSIF_SIGNAL_TYPE_MASK (0x2)
#define DV_DESCRB_VSIF_LL_MASK (0x1)
#define DV_DESCRB_VSIF_PAYLOAD_OFFSET (7)
#define DV_DESCRB_VSIF_PAYLOAD_SIZE (24)
#define DV_DESCRB_CRC_LENGTH (4)
#define DV_DESCRB_CRC_HW_BIT_WIDTH (3)
#define DV_DESCRB_CRC_CTRL_COUNT (5)
#define DV_DESCRB_CONTENT_TYPE_GAME (0x2)
#define DV_DESCRB_DATA_RB_RETRY_THRESHOLD	(5)
#define DV_DESCRB_PACK_MODE_REG_VAL_1P (0)
#define DV_DESCRB_PACK_MODE_REG_VAL_2P (1)
#define DV_DESCRB_PACK_MODE_REG_VAL_4P (2)
#define DV_DESCRB_PACK_MODE_REG_VAL_8P (3)


#define DV_DESCRB_GET_PKT_TYPE(data)		((((data) & 0xFF) >> 6) & 0x3)
#define DV_DESCRB_GET_METADATA_TYPE(data)	((((data) & 0xFF) >> 4) & 0x3)


#define DIVIDE_BY_2(x)		((x) / 2)
#define SHIFT_OFFSET_8		(8)
#define SHIFT_OFFSET_16		(16)
#define SHIFT_OFFSET_24		(24)
#define MASK_BYTE0		(0xFF)
#define MASK_BYTE1		(0xFF00)
#define MASK_2_BYTES		(0xFFFF)
#define GET_BYTE0(data)		((data) & MASK_BYTE0)
#define GET_BYTE1(data)		(((data) >> SHIFT_OFFSET_8) & MASK_BYTE0)
#define IS_SOURCE_TUNNEL_MODE(interface, color_format)                               \
		(((interface == SRCCAP_DV_DESCRB_INTERFACE_SINK_LED) ||     \
		(interface == SRCCAP_DV_DESCRB_INTERFACE_FORM_1)) &&         \
		(color_format == M_HDMI_COLOR_RGB))
#define BIT_WIDTH_1		(1)
#define BIT_WIDTH_3		(3)
#define BIT_WIDTH_4		(4)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------
enum srccap_dv_descrb_pkt_type {
	SRCCAP_DV_DESCRB_PKT_TYPE_SINGLE = 0,
	SRCCAP_DV_DESCRB_PKT_TYPE_FIRST,
	SRCCAP_DV_DESCRB_PKT_TYPE_MIDDLE,
	SRCCAP_DV_DESCRB_PKT_TYPE_LAST,
	SRCCAP_DV_DESCRB_PKT_TYPE_MAX
};

// 0b00: Dolby Vision metadata
// 0b01~0b11: Reserved for future use
enum srccap_dv_descrb_metadata_type {
	SRCCAP_DV_DESCRB_METADATA_TYPE_DV = 0,
	SRCCAP_DV_DESCRB_METADATA_TYPE_MAX,
};

struct srccap_dv_vsif_payload {
	/* PB4 */
	__u8 u1ll		: BIT_WIDTH_1;
	__u8 u4signalType	: BIT_WIDTH_4;
	__u8 u3version		: BIT_WIDTH_3;
	/* PB5 */
	__u8 u4effTmaxPqHi	: BIT_WIDTH_4;
	__u8 u1bt2020		: BIT_WIDTH_1;
	__u8 u1l11Md		: BIT_WIDTH_1;
	__u8 u1AuxMd		: BIT_WIDTH_1;
	__u8 u1backltCtrlMd	: BIT_WIDTH_1;
	/* PB6 */
	__u8 u8effTmaxPqLow;
	/* PB7 */
	__u8 u8auxRunmode;
	/* PB8 */
	__u8 u8auxRunversion;
	/* PB9 */
	__u8 u8auxDebug0;
	/* PB10 */
	__u8 u4contentType	: BIT_WIDTH_4;
	__u8 u4rsv		: BIT_WIDTH_4;
	/* PB11 */
	__u8 u4intendWhitePoint	: BIT_WIDTH_4;
	__u8 u1crF		: BIT_WIDTH_1;
	__u8 u3rsv		: BIT_WIDTH_3;
	/* PB12 */
	__u8 u8l11Byte2;
	/* PB13 */
	__u8 u8l11Byte3;
};


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static const u32 crc32_lut[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------
static int dv_descrb_get_crc32(u8 *data, u32 size, u32 *expected_crc)
{
	u32 c = 0;
	u8 *p = NULL;

	if ((data == NULL) || (expected_crc == NULL))
		return -EINVAL;

	c = *expected_crc;
	p = data;

	c = ~c;

	while (size) {
		c = (c << SHIFT_OFFSET_8)
			^ crc32_lut[((c >> SHIFT_OFFSET_24) ^ *p) & MASK_BYTE0];
		p++;
		size--;
	}

	*expected_crc = c;

	return 0;
}

static int dv_descrb_check_sw_crc(u8 *data, u32 size, bool *crc)
{
	int ret = 0;
	u32 actual_crc = 0;
	u32 expected_crc = 0;
	u8 *ptr = NULL;
	int i = 0;

	if ((data == NULL) || (crc == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if (size < (DV_DESCRB_EMP_EDR_DATA_OFFSET + DV_DESCRB_CRC_LENGTH)) {
		SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
			"emp size %d not valid\n", size);
		ret = -EINVAL;
		goto exit;
	}

	actual_crc = 0;
	ptr = &data[size - DV_DESCRB_CRC_LENGTH];
	for (i = 0; i < DV_DESCRB_CRC_LENGTH; i++) {
		actual_crc = ((actual_crc << SHIFT_OFFSET_8) | (*ptr));
		ptr++;
	}

	ret = dv_descrb_get_crc32(
		(data + DV_DESCRB_EMP_EDR_DATA_OFFSET),
		(size - DV_DESCRB_EMP_EDR_DATA_OFFSET - DV_DESCRB_CRC_LENGTH),
		&expected_crc);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_PKT,
		"actual_crc = 0x%08x, expected_crc = 0x%08x\n"
		, actual_crc, expected_crc);
	*crc = (actual_crc == expected_crc) ? (TRUE) : (FALSE);

exit:
	return ret;
}

static int dv_descrb_check_hw_crc(u8 dev_id, u8 pkt_num, bool *crc)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	u64 crc_reg = 0;

	if (crc == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	ret = drv_hwreg_srccap_dv_descrb_get_crc_reg(dev_id, &crc_reg);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	*crc = TRUE;
	for (i = 0; i < pkt_num; i++) {
		for (j = 0; j < DV_DESCRB_CRC_HW_BIT_WIDTH; j++) {
			if ((crc_reg & SRCCAP_DV_BIT(i*DV_DESCRB_CRC_HW_BIT_WIDTH+j)) != 0)
				break;
		}
		if (j >= DV_DESCRB_CRC_HW_BIT_WIDTH) {
			*crc = FALSE;
			break;
		}
	}

exit:
	return ret;
}

static int dv_descrb_read_data(u8 dev_id, u16 addr, u16 *data)
{
	int ret = 0;
	bool riu = FALSE;
	u8 flag = 0;
	u8 flag_rb = 0;
	u8 retry = DV_DESCRB_DATA_RB_RETRY_THRESHOLD;

	// Set read back addr
	riu = TRUE;
	ret = drv_hwreg_srccap_dv_descrb_set_addr(dev_id, addr, riu, NULL, NULL);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	// Set check flag
	riu = TRUE;
	flag = (u8)(addr & 0xF);
	ret = drv_hwreg_srccap_dv_descrb_set_r_check(dev_id, flag, riu, NULL, NULL);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	// Read back check flag
	while (retry--) {
		riu = TRUE;
		ret = drv_hwreg_srccap_dv_descrb_get_r_check_rb(dev_id, &flag_rb);
		if (ret < 0) {
			SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
			goto exit;
		}

		if (flag_rb == flag)
			break;
	}

	if (flag_rb != flag) {
		ret = -EAGAIN;
		goto exit;
	}

	// Read back metadata
	riu = TRUE;
	ret = drv_hwreg_srccap_dv_descrb_get_r_data(dev_id, data);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

exit:
	return ret;
}

static int dv_descrb_get_ieee_oui(u8 *data, u8 offset, u32 *ieee_oui)
{
	int ret = 0;
	int i;

	if ((data == NULL) || (ieee_oui == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	*ieee_oui = 0;

	i = DV_DESCRB_IEEE_OUI_LENGTH;
	while (i--)
		*ieee_oui = ((*ieee_oui) << SHIFT_OFFSET_8) | (data[offset + i]);

exit:
	return ret;
}

static int dv_descrb_get_dv_game(u8 *data, bool *is_game)
{
	int ret = 0;
	struct srccap_dv_vsif_payload *payload = NULL;

	if ((data == NULL) || (is_game == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	payload = (struct srccap_dv_vsif_payload *)data;

	if (payload->u1l11Md
		&& (payload->u4contentType == DV_DESCRB_CONTENT_TYPE_GAME))
		*is_game = TRUE;
	else
		*is_game = FALSE;

exit:
	return ret;
}

static void dv_descrb_neg_ctrl_reset(
	struct srccap_dv_descrb_info *descrb_info)
{
	descrb_info->buf.DoViCtr = DV_DESCRB_CRC_CTRL_COUNT;
	descrb_info->buf.neg_ctrl = M_DV_CRC_STATE_OK;
	descrb_info->buf.in_transition = false;
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_DEBUG, "crc reset, DoViCtr=%d, NEG=%d\n",
		descrb_info->buf.DoViCtr,
		descrb_info->buf.neg_ctrl);
}

static void dv_descrb_neg_ctrl_stop(
	struct srccap_dv_descrb_info *descrb_info)
{
	descrb_info->buf.DoViCtr = 0;
	descrb_info->buf.neg_ctrl = M_DV_CRC_STATE_OK;
	descrb_info->buf.in_transition = false;
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_DEBUG, "crc stop, DoViCtr=%d, NEG=%d\n",
		descrb_info->buf.DoViCtr,
		descrb_info->buf.neg_ctrl);
}

static void dv_descrb_neg_ctrl_count_fail(
	struct srccap_dv_descrb_info *descrb_info)
{
	if (descrb_info->buf.DoViCtr > 0) {
		descrb_info->buf.DoViCtr--;
		descrb_info->buf.neg_ctrl = M_DV_CRC_STATE_FAIL_TRANSITION;
	} else
		descrb_info->buf.neg_ctrl = M_DV_CRC_STATE_FAIL_STABLE;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_DEBUG, "crc fail, DoViCtr=%d, NEG=%d\n",
		descrb_info->buf.DoViCtr,
		descrb_info->buf.neg_ctrl);
}

static void dv_descrb_transition_ctrl(
	struct srccap_dv_descrb_info *descrb_info)
{
	descrb_info->buf.in_transition = true;

	descrb_info->buf.DoViCtr = 0;
	descrb_info->buf.neg_ctrl = M_DV_CRC_STATE_OK;
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_DEBUG, "transition, DoViCtr=%d, NEG=%d\n",
		descrb_info->buf.DoViCtr,
		descrb_info->buf.neg_ctrl);
}


static int dv_descrb_parse_md(
	u8 dev_id,
	u8 pkt_num,
	u8 *buf)
{
	int ret = 0;
	int i, j;
	u16 data = 0;
	u8 pkt_type = 0;
	u8 metadata_type = 0;
	u16 addr_start = 0;
	u16 addr_end = 0;
	u16 addr_offset = 0;

	if (buf == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	for (i = 0; i < pkt_num; i++) {
		ret = dv_descrb_read_data(dev_id,
			DIVIDE_BY_2(i * DV_DESCRB_PKT_REPEAT_NUM * DV_DESCRB_PKT_LENGTH),
			&data);
		if (ret < 0) {
			SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
			goto exit;
		}

		pkt_type = DV_DESCRB_GET_PKT_TYPE(data);
		metadata_type = DV_DESCRB_GET_METADATA_TYPE(data);

		if (metadata_type != SRCCAP_DV_DESCRB_METADATA_TYPE_DV) {
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC,
				"metadata type (%u) != SRCCAP_DV_DESCRB_METADATA_TYPE_DV (%u)\n",
				metadata_type, SRCCAP_DV_DESCRB_METADATA_TYPE_DV);
			goto exit;
		}

		addr_offset =
			DIVIDE_BY_2(i * DV_DESCRB_PKT_REPEAT_NUM * DV_DESCRB_PKT_LENGTH);

		switch (pkt_type) {
		case SRCCAP_DV_DESCRB_PKT_TYPE_SINGLE:
		case SRCCAP_DV_DESCRB_PKT_TYPE_FIRST:
			addr_start =
				DIVIDE_BY_2((DV_DESCRB_PKT_HEADER + DV_DESCRB_PKT_META_LEN_SIZE))
				+ addr_offset;
			break;
		case SRCCAP_DV_DESCRB_PKT_TYPE_MIDDLE:
		case SRCCAP_DV_DESCRB_PKT_TYPE_LAST:
			addr_start =
				DIVIDE_BY_2(DV_DESCRB_PKT_HEADER)
				+ addr_offset;
			break;
		default:
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC,
				"invalid pkt type=%u\n", pkt_type);
			goto exit;
		}

		addr_end =
			DIVIDE_BY_2((DV_DESCRB_PKT_LENGTH - DV_DESCRB_PKT_TAIL))
			+ addr_offset;

		for (j = addr_start; j < addr_end; j++) {
			ret = dv_descrb_read_data(dev_id, j, &data);
			if (ret < 0) {
				SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
				goto exit;
			}

			if (j == addr_start) {
				*buf = GET_BYTE1(data);
				buf++;
			} else {
				*buf =  GET_BYTE0(data);
				buf++;
				*buf =  GET_BYTE1(data);
				buf++;
			}
		}
	}

exit:
	return ret;
}

static int dv_descrb_parse_vsem(
	struct hdmi_emp_packet_info *emp,
	u16 emp_num,
	u8 *vsem,
	u16 *vsem_size,
	bool *is_ll,
	bool *is_game)
{
	int ret = 0;
	int i;
	u8 offset;
	u32 ieee_oui = 0;
	u8 data_version;
	u16 remain_size;
	u8 *edr_data = NULL;

	if ((emp == NULL)
		|| (vsem == NULL)
		|| (vsem_size == NULL)
		|| (is_ll == NULL)
		|| (is_game == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	*vsem_size = 0;
	*is_ll = FALSE;
	*is_game = FALSE;

	for (i = 0; i < emp_num; i++) {
		// Parse first EMP
		if (i == 0) {
			if (emp[i].u8seq_index != 0)
				break;

			/* IEEE OUI */
			if (emp[i].u8organ_id == 0) {
				offset = DV_DESCRB_EMP_IEEE_OUI_OFFSET;
				dv_descrb_get_ieee_oui(emp[i].au8md, offset, &ieee_oui);
			}

			if (ieee_oui != DV_DESCRB_IEEE_OUI_VALUE)
				break;

			/* data_version */
			offset = DV_DESCRB_EMP_DATA_VERSION_OFFSET;
			data_version = emp[i].au8md[offset];
			if (data_version == DV_DESCRB_EMP_DATA_VERSION_FORM1)
				*is_ll = FALSE;
			else
				*is_ll = TRUE;

			/* edr_data */
			offset = DV_DESCRB_EMP_EDR_DATA_OFFSET;
			edr_data = (u8 *)(&emp[i].au8md[offset]);

			/* DV game */
			if (*is_ll) {
				/* form2_metadata(): Same as payload of Dolby VSIF */
				dv_descrb_get_dv_game((u8 *)edr_data, is_game);
			}

			/* packet size */
			remain_size = *vsem_size =
				(((emp[i].u8data_set_len_msb << SHIFT_OFFSET_8)
				| (emp[i].u8data_set_len_lsb))
				+ DV_DESCRB_EMP_HEADER_SIZE);
		}

		if (remain_size == 0)
			break;

		if (remain_size >= DV_DESCRB_EMP_PAYLOAD_SIZE) {
			memcpy(vsem, emp[i].au8md, DV_DESCRB_EMP_PAYLOAD_SIZE);
			vsem += DV_DESCRB_EMP_PAYLOAD_SIZE;
			remain_size -= DV_DESCRB_EMP_PAYLOAD_SIZE;
		} else {
			memcpy(vsem, emp[i].au8md, remain_size);
			break;
		}
	}

exit:
	return ret;
}

static void dv_descrb_drm_index_correction(
	struct hdmi_hdr_packet_info *drm,
	u8 *u8RIndex,
	u8 *u8GIndex,
	u8 *u8BIndex)
{
	if ((drm->u16Display_Primaries_X[INDEX_0] > drm->u16Display_Primaries_X[INDEX_1])
	  && (drm->u16Display_Primaries_X[INDEX_0] > drm->u16Display_Primaries_X[INDEX_2]))
		*u8RIndex = INDEX_0;
	else if (drm->u16Display_Primaries_X[INDEX_1]
			> drm->u16Display_Primaries_X[INDEX_2])
		*u8RIndex = INDEX_1;
	else
		*u8RIndex = INDEX_2;
	if ((drm->u16Display_Primaries_Y[INDEX_0] > drm->u16Display_Primaries_Y[INDEX_1])
		&& (drm->u16Display_Primaries_Y[INDEX_0] > drm->u16Display_Primaries_Y[INDEX_2]))
		*u8GIndex = INDEX_0;
	else if (drm->u16Display_Primaries_Y[INDEX_1] > drm->u16Display_Primaries_Y[INDEX_2])
		*u8GIndex = INDEX_1;
	else
		*u8GIndex = INDEX_2;
	if ((*u8RIndex == INDEX_0) && (*u8GIndex == INDEX_1))
		*u8BIndex = INDEX_2;
	else if ((*u8RIndex == INDEX_0) && (*u8GIndex == INDEX_2))
		*u8BIndex = INDEX_1;
	else if ((*u8RIndex == INDEX_1) && (*u8GIndex == INDEX_0))
		*u8BIndex = INDEX_2;
	else if ((*u8RIndex == INDEX_1) && (*u8GIndex == INDEX_2))
		*u8BIndex = INDEX_0;
	else if ((*u8RIndex == INDEX_2) && (*u8GIndex == INDEX_0))
		*u8BIndex = INDEX_1;
	else
		*u8BIndex = INDEX_0;
}

static int dv_descrb_create_drm_packet(u8 *drm_raw, struct hdmi_hdr_packet_info *drm)
{
	int ret = 0;

	if (drm_raw == NULL || drm == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		ret = -1;
	} else {
		u8 drm_pkt[SRCCAP_DV_DESCRB_DRM_SIZE] = {
			// header
			DV_DESCRB_DRM_PKT_TYPE, DV_DESCRB_DRM_VERSION, DV_DESCRB_DRM_LENGTH,
			// payload
			0x0, drm->u8EOTF, drm->u8Static_Metadata_ID,
			GET_BYTE0(drm->u16Display_Primaries_X[INDEX_0]),
			GET_BYTE1(drm->u16Display_Primaries_X[INDEX_0]),
			GET_BYTE0(drm->u16Display_Primaries_Y[INDEX_0]),
			GET_BYTE1(drm->u16Display_Primaries_Y[INDEX_0]),
			GET_BYTE0(drm->u16Display_Primaries_X[INDEX_1]),
			GET_BYTE1(drm->u16Display_Primaries_X[INDEX_1]),
			GET_BYTE0(drm->u16Display_Primaries_Y[INDEX_1]),
			GET_BYTE1(drm->u16Display_Primaries_Y[INDEX_1]),
			GET_BYTE0(drm->u16Display_Primaries_X[INDEX_2]),
			GET_BYTE1(drm->u16Display_Primaries_X[INDEX_2]),
			GET_BYTE0(drm->u16Display_Primaries_Y[INDEX_2]),
			GET_BYTE1(drm->u16Display_Primaries_Y[INDEX_2]),
			GET_BYTE0(drm->u16White_Point_X), GET_BYTE1(drm->u16White_Point_X),
			GET_BYTE0(drm->u16White_Point_Y), GET_BYTE1(drm->u16White_Point_Y),
			GET_BYTE0(drm->u16Max_Display_Mastering_Luminance),
			GET_BYTE1(drm->u16Max_Display_Mastering_Luminance),
			GET_BYTE0(drm->u16Min_Display_Mastering_Luminance),
			GET_BYTE1(drm->u16Min_Display_Mastering_Luminance),
			GET_BYTE0(drm->u16Maximum_Content_Light_Level),
			GET_BYTE1(drm->u16Maximum_Content_Light_Level),
			GET_BYTE0(drm->u16Maximum_Frame_Average_Light_Level),
			GET_BYTE1(drm->u16Maximum_Frame_Average_Light_Level)
		};

		memcpy(drm_raw, drm_pkt, SRCCAP_DV_DESCRB_DRM_SIZE);
	}

	return ret;
}

static int dv_descrb_parse_drm(
	struct hdmi_hdr_packet_info *drm,
	u8 *drm_raw,
	bool *is_dv)
{
	int ret = 0;


	if ((drm == NULL)
		|| (drm_raw == NULL)
		|| (is_dv == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if ((drm->u8Version == DV_DESCRB_DRM_VERSION)
		&& (drm->u8Length == DV_DESCRB_DRM_LENGTH)
		&& (drm->u8EOTF == DV_DESCRB_DRM_EOTF_VALUE)
		&& (drm->u8Static_Metadata_ID == DV_DESCRB_DRM_STATIC_METADATA_ID_VALUE)) {

		u8 u8RIndex = 0;
		u8 u8GIndex = 0;
		u8 u8BIndex = 0;

		// Establishing correct indices for RGB based on CTA-861-G, Section 6.9.1
		dv_descrb_drm_index_correction(drm, &u8RIndex, &u8GIndex, &u8BIndex);

		// Decision logic to establish the interface is a Dolby Vision unique DRM from a PC
		if (((drm->u16Display_Primaries_X[u8RIndex] & MASK_2_BYTES)
				== DV_DESCRB_DRM_PRIMARIES_X_R_VALUE)
			&& ((drm->u16Display_Primaries_Y[u8RIndex] & MASK_2_BYTES)
				== DV_DESCRB_DRM_PRIMARIES_Y_R_VALUE)
			&& ((drm->u16Display_Primaries_X[u8GIndex] & MASK_2_BYTES)
				== DV_DESCRB_DRM_PRIMARIES_X_G_VALUE)
			&& ((drm->u16Display_Primaries_Y[u8GIndex] & MASK_2_BYTES)
				== DV_DESCRB_DRM_PRIMARIES_Y_G_VALUE)
			&& ((drm->u16Display_Primaries_X[u8BIndex] & MASK_2_BYTES)
				== DV_DESCRB_DRM_PRIMARIES_X_B_VALUE)
			&& ((drm->u16Display_Primaries_Y[u8BIndex] & MASK_2_BYTES)
				== DV_DESCRB_DRM_PRIMARIES_Y_B_VALUE)
			&& ((drm->u16Max_Display_Mastering_Luminance & MASK_2_BYTES)
				== DV_DESCRB_DRM_MAX_LUMA_VALUE)
			&& ((drm->u16Min_Display_Mastering_Luminance & MASK_2_BYTES)
				== DV_DESCRB_DRM_MIN_LUMA_VALUE)
			&& ((drm->u16White_Point_X & MASK_2_BYTES)
				== DV_DESCRB_DRM_WHITE_POINT_X_VALUE)
			&& ((drm->u16White_Point_Y & MASK_2_BYTES)
				== DV_DESCRB_DRM_WHITE_POINT_Y_VALUE)
			&& ((drm->u16Maximum_Content_Light_Level & MASK_2_BYTES)
				== DV_DESCRB_DRM_MAX_CLL_VALUE)
			&& ((drm->u16Maximum_Frame_Average_Light_Level & MASK_2_BYTES)
				== DV_DESCRB_DRM_MAX_FALL_VALUE)) {
			/// Dolby Unique DRM
			*is_dv = true;

			ret = dv_descrb_create_drm_packet(drm_raw, drm);
		} else
			*is_dv = false;
	}

exit:
	return ret;
}

static int dv_descrb_parse_vsif(
	struct hdmi_vsif_packet_info *vsif,
	u16 vsif_num,
	u8 *vsif_raw,
	bool *is_dv,
	bool *is_ll,
	bool *is_game)
{
	int ret = 0;
	int i;
	u32 ieee_oui = 0;
	struct hdmi_vsif_packet_info *raw_data;
	bool isH14B = FALSE;
	bool isDVVisif = FALSE;
	struct srccap_dv_vsif_payload *vsif_payload = NULL;

	if ((vsif == NULL)
		|| (vsif_raw == NULL)
		|| (is_dv == NULL)
		|| (is_ll == NULL)
		|| (is_game == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	*is_dv = FALSE;
	*is_ll = FALSE;
	*is_game = FALSE;

	for (i = 0; i < vsif_num; i++) {
		if ((vsif[i].u8hb0 == DV_DESCRB_VSIF_PKT_TYPE)
			&& (vsif[i].u8version == DV_DESCRB_VSIF_VERSION)
			&& (vsif[i].u5length == DV_DESCRB_VSIF_LENGTH)) {
			isDVVisif = TRUE;
		} else if ((vsif[i].u8hb0 == DV_DESCRB_VSIF_PKT_TYPE)
			&& (vsif[i].u8version == DV_DESCRB_VSIF_VERSION)
			&& (vsif[i].u5length == DV_DESCRB_H14B_VSIF_LENGTH)) {
			isH14B = TRUE;
		} else {
			continue;
		}

		//Get IEEE OUI
		dv_descrb_get_ieee_oui(vsif[i].au8ieee, 0, &ieee_oui);

		if ((ieee_oui != DV_DESCRB_IEEE_OUI_VALUE)
			&& (isDVVisif == TRUE))
			continue;

		if ((ieee_oui != DV_DESCRB_H14B_IEEE_OUI_VALUE)
			&& (isH14B == TRUE))
			continue;

		// Get VSIF raw data
		raw_data = (struct hdmi_vsif_packet_info *)vsif_raw;
		raw_data->u8hb0 = DV_DESCRB_VSIF_PKT_TYPE;
		raw_data->u8version = DV_DESCRB_VSIF_VERSION;
		raw_data->u5length = DV_DESCRB_VSIF_LENGTH;
		raw_data->u8crc = vsif[i].u8crc;
		memcpy(raw_data->au8ieee, vsif[i].au8ieee, DV_DESCRB_IEEE_OUI_LENGTH);
		memcpy((vsif_raw + DV_DESCRB_VSIF_PAYLOAD_OFFSET),
			vsif[i].au8payload, DV_DESCRB_VSIF_PAYLOAD_SIZE);

		if (isDVVisif == TRUE) {
			/* Dolby VSIF */
			vsif_payload = (struct srccap_dv_vsif_payload *)vsif[i].au8payload;
			*is_dv = (vsif_payload->u4signalType & 0x1);
			*is_ll = vsif_payload->u1ll;

			/* DV game */
			dv_descrb_get_dv_game((u8 *)vsif_payload, is_game);
		} else {
			/* H14b VSIF */
			*is_dv = TRUE;
		}

		break;
	}

exit:
	return ret;
}

static int dv_descrb_get_vsif(
	struct srccap_dv_dqbuf_in *in,
	u8 index,
	enum srccap_dv_descrb_interface *interface,
	enum m_hdmi_color_format color_format,
	bool *is_vsif_received)
{
	int ret = 0;
	bool is_dv = FALSE;
	bool is_ll = FALSE;
	bool is_game = FALSE;
	bool crc = FALSE;

	if ((in == NULL)
		|| (interface == NULL)
		|| (is_vsif_received == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	dv_descrb_parse_vsif(
		in->vsif,
		in->vsif_num,
		in->dev->dv_info.descrb_info.pkt_info.vsif,
		&is_dv,
		&is_ll,
		&is_game);

	if (is_dv) {
		if (!is_ll) {
			crc = in->dev->dv_info.descrb_info.buf.crc[index];
			*interface = SRCCAP_DV_DESCRB_INTERFACE_SINK_LED;
			if (crc) {
				/* CRC pass: normal DV */
				dv_descrb_neg_ctrl_reset(&(in->dev->dv_info.descrb_info));
			} else if ((in->dev->dv_info.descrb_info.common.interface != (*interface))
				|| (in->dev->dv_info.descrb_info.buf.in_transition == true)) {
				/* CRC fail: transition */
				dv_descrb_transition_ctrl(&(in->dev->dv_info.descrb_info));
			} else {
				/* CRC fail: DV NEG */
				dv_descrb_neg_ctrl_count_fail(&(in->dev->dv_info.descrb_info));
			}
		} else {
			dv_descrb_neg_ctrl_reset(&(in->dev->dv_info.descrb_info));
			if (color_format == M_HDMI_COLOR_RGB)
				*interface = SRCCAP_DV_DESCRB_INTERFACE_SOURCE_LED_RGB;
			else
				*interface = SRCCAP_DV_DESCRB_INTERFACE_SOURCE_LED_YUV;
		}

		/* DV game */
		in->dev->dv_info.descrb_info.common.dv_game_mode = is_game;

		*is_vsif_received = TRUE;

		if (g_dv_debug_level & SRCCAP_DV_DBG_LEVEL_HDMI_PKT) {
			int sum = 0;

			mtk_dv_debug_checksum(
				(u8 *)&in->vsif[0],
				sizeof(struct hdmi_vsif_packet_info),
				&sum);
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_PKT,
				"vsif sum=%u, crc=%d\n", sum, crc);
		}
	}

exit:
	return ret;
}

static int dv_descrb_get_emp(
	struct srccap_dv_dqbuf_in *in,
	enum srccap_dv_descrb_interface *interface,
	enum m_hdmi_color_format color_format,
	bool *is_vsem_received,
	u16 *vsem_size)
{
	int ret = 0;
	bool is_ll = FALSE;
	bool is_game = FALSE;
	bool crc = FALSE;

	if ((in == NULL)
		|| (interface == NULL)
		|| (vsem_size == NULL)
		|| (is_vsem_received == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	*vsem_size = 0;
	ret = dv_descrb_parse_vsem(
		in->emp,
		in->emp_num,
		in->dev->dv_info.descrb_info.pkt_info.vsem,
		vsem_size,
		&is_ll,
		&is_game);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	if (*vsem_size != 0) {
		ret = dv_descrb_check_sw_crc(in->dev->dv_info.descrb_info.pkt_info.vsem,
			*vsem_size,
			&crc);
		if (ret < 0) {
			SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
			goto exit;
		}
	}

	if ((*vsem_size != 0)) {
		/* determin interface */
		if (!is_ll) {
			*interface = SRCCAP_DV_DESCRB_INTERFACE_FORM_1;
		} else {
			if (color_format == M_HDMI_COLOR_RGB)
				*interface = SRCCAP_DV_DESCRB_INTERFACE_FORM_2_RGB;
			else
				*interface = SRCCAP_DV_DESCRB_INTERFACE_FORM_2_YUV;
		}

		/* CRC decision */
		if (crc == TRUE) {
			/* CRC pass: normal DV */
			dv_descrb_neg_ctrl_reset(&(in->dev->dv_info.descrb_info));

			/* DV game */
			in->dev->dv_info.descrb_info.common.dv_game_mode = is_game;
		} else if ((in->dev->dv_info.descrb_info.common.interface != (*interface))
			|| (in->dev->dv_info.descrb_info.buf.in_transition == true)) {
			/* CRC fail: transition */
			dv_descrb_transition_ctrl(&(in->dev->dv_info.descrb_info));
		} else {
			/* CRC fail: DV NEG */
			dv_descrb_neg_ctrl_count_fail(&(in->dev->dv_info.descrb_info));
		}
		*is_vsem_received = TRUE;

		if (g_dv_debug_level & SRCCAP_DV_DBG_LEVEL_HDMI_PKT) {
			int sum = 0;

			mtk_dv_debug_checksum(
				in->dev->dv_info.descrb_info.pkt_info.vsem,
				*vsem_size,
				&sum);
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_PKT,
				"vsem len=%u, sum=%u, crc=%d\n", *vsem_size, sum, crc);
		}
	}
exit:
	return ret;
}

static int dv_descrb_get_drm(
	struct srccap_dv_dqbuf_in *in,
	enum srccap_dv_descrb_interface *interface,
	enum m_hdmi_color_format color_format,
	bool *is_drm_received)
{
	int ret = 0;
	bool is_dv = FALSE;

	if ((in == NULL)
		|| (interface == NULL)
		|| (is_drm_received == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	dv_descrb_parse_drm(
		in->drm,
		in->dev->dv_info.descrb_info.pkt_info.drm,
		&is_dv);

	if (is_dv) {
		if (color_format == M_HDMI_COLOR_RGB)
			*interface = SRCCAP_DV_DESCRB_INTERFACE_DRM_SOURCE_LED_RGB;
		else
			*interface = SRCCAP_DV_DESCRB_INTERFACE_DRM_SOURCE_LED_YUV;

		dv_descrb_neg_ctrl_reset(&(in->dev->dv_info.descrb_info));

		*is_drm_received = TRUE;

		if (g_dv_debug_level & SRCCAP_DV_DBG_LEVEL_HDMI_PKT) {
			int sum = 0;

			mtk_dv_debug_checksum(
				(u8 *)&in->drm[0],
				sizeof(struct hdmi_hdr_packet_info),
				&sum);
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_PKT,
				"drm sum=%u\n", sum);
		}
	}
exit:
	return ret;
}

//-----------------------------------------------------------------------------
// Global Functions
//-----------------------------------------------------------------------------
int mtk_dv_descrb_init(
	struct srccap_dv_init_in *in,
	struct srccap_dv_init_out *out)
{
	int ret = 0;
	int i;
	u8 dev_id = 0;
	u16 meta_length = 0;
	u8 pkt_num = 0;
	bool riu = FALSE;

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	memset(&in->dev->dv_info.descrb_info.buf, 0, sizeof(struct srccap_dv_descrb_buf));

	for (i = 0; i < SRCCAP_DV_DESCRB_BUF_NUM; i++) {
		if (in->dev->dv_info.descrb_info.buf.data[i] == NULL)
			in->dev->dv_info.descrb_info.buf.data[i] =
				devm_kzalloc(in->dev->dev,
					(DV_DESCRB_PKT_LENGTH * DV_DESCRB_MAX_HW_SUPPORT_PKT),
					GFP_KERNEL);
		if (!in->dev->dv_info.descrb_info.buf.data[i])
			return -ENOMEM;
	}

	dev_id = in->dev->dev_id;

	// Set metadata length
	meta_length = DV_DESCRB_META_LENGTH;
	riu = TRUE;
	ret = drv_hwreg_srccap_dv_descrb_set_meta_length(dev_id, meta_length, riu, NULL, NULL);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	// Set packet number
	pkt_num = DV_DESCRB_PKT_NUM;
	riu = TRUE;
	ret = drv_hwreg_srccap_dv_descrb_set_pkt_num(dev_id, pkt_num, riu, NULL, NULL);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

exit:
	return ret;
}

int mtk_dv_descrb_streamon(
	struct srccap_dv_streamon_in *in,
	struct srccap_dv_streamon_out *out)
{
	int ret = 0;

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if (in->dev == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	dv_descrb_neg_ctrl_stop(&(in->dev->dv_info.descrb_info));
	in->dev->dv_info.descrb_info.common.interface = SRCCAP_DV_DESCRB_INTERFACE_NONE;
	in->dev->dv_info.descrb_info.common.dv_game_mode = FALSE;
	in->dev->dv_info.descrb_info.buf.in_transition = FALSE;

exit:
	return ret;
}

int mtk_dv_descrb_streamoff(
	struct srccap_dv_streamoff_in *in,
	struct srccap_dv_streamoff_out *out)
{
	int ret = 0;

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if (in->dev == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	dv_descrb_neg_ctrl_stop(&(in->dev->dv_info.descrb_info));
	in->dev->dv_info.descrb_info.common.interface = SRCCAP_DV_DESCRB_INTERFACE_NONE;
	in->dev->dv_info.descrb_info.common.dv_game_mode = FALSE;
	in->dev->dv_info.descrb_info.buf.in_transition = FALSE;

exit:
	return ret;
}


int mtk_dv_descrb_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out)
{
	int ret = 0;
	enum m_hdmi_color_format color_format = M_HDMI_COLOR_UNKNOWN;
	enum srccap_dv_descrb_interface interface = SRCCAP_DV_DESCRB_INTERFACE_NONE;
	bool is_vsif_received = FALSE;
	bool is_vsem_received = FALSE;
	bool is_drm_received = FALSE;
	u16 vsem_size = 0;
	u8 index = 0;
	bool crc = FALSE;
	struct mtk_srccap_dev *srccap_dev = NULL;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "descramble dequeue buffer\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	srccap_dev = in->dev;
	index = (in->dev->dv_info.descrb_info.buf.index) ? (0) : (1);
	crc = in->dev->dv_info.descrb_info.buf.crc[index];

	/* get color format and color range */
	if (!in->avi_valid) {
		SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_PKT, "AVI is not valid\n");
		color_format = M_HDMI_COLOR_YUV_444;
	} else {
		color_format = in->avi->enColorForamt;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_PKT,
		"color_format=%d\n", color_format);

	/* get EMP */
	if (in->emp_num > 0) {
		ret = dv_descrb_get_emp(
			in,
			&interface,
			color_format,
			&is_vsem_received,
			&vsem_size);
	}

	/* get DRM */
	if (in->drm_num > 0)
		ret = dv_descrb_get_drm(in, &interface, color_format, &is_drm_received);

	/* get dolby VSIF */
	if (!is_vsem_received && !is_drm_received)
		ret = dv_descrb_get_vsif(in, index, &interface, color_format, &is_vsif_received);

	/* non DV */
	if (interface == SRCCAP_DV_DESCRB_INTERFACE_NONE) {
		if (in->dev->dv_info.descrb_info.common.interface != interface) {
			/* transition DV -> non DV */
			dv_descrb_transition_ctrl(&(in->dev->dv_info.descrb_info));
		} else {
			/* normal */
			dv_descrb_neg_ctrl_stop(&(in->dev->dv_info.descrb_info));
		}
	}

	in->dev->dv_info.descrb_info.common.interface = interface;
	in->dev->dv_info.descrb_info.pkt_info.is_vsif_received = is_vsif_received;
	in->dev->dv_info.descrb_info.pkt_info.is_vsem_received = is_vsem_received;
	in->dev->dv_info.descrb_info.pkt_info.is_drm_received = is_drm_received;
	in->dev->dv_info.descrb_info.pkt_info.vsem_size = vsem_size;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_PKT,
		"index=%d, interface=%u, is_vsif=%u, is_vsem=%u(size=%u), is_drm=%u, crc=%d\n",
		index,
		interface,
		is_vsif_received,
		is_vsem_received, vsem_size,
		is_drm_received,
		crc);

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_DEBUG,
		"index=%d, crc=%d, DoViCtr=%d, NEG=%d, game=%d\n",
		index,
		crc,
		in->dev->dv_info.descrb_info.buf.DoViCtr,
		in->dev->dv_info.descrb_info.buf.neg_ctrl,
		in->dev->dv_info.descrb_info.common.dv_game_mode);

	srccap_dev->dv_info.descrb_info.common.hdmi_422_pack_en =
		IS_SOURCE_TUNNEL_MODE(interface, color_format);

exit:
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "descramble dequeue buffer done\n");

	return ret;
}

void mtk_dv_descrb_handle_irq(
	void *param)
{
	int ret = 0;
	struct mtk_srccap_dev *srccap_dev = NULL;
	u8 dev_id = 0;
	u8 pkt_num = 0;
	bool en = FALSE;
	bool riu = FALSE;
	bool crc = FALSE;
	u16 hi, lo;
	u16 metadata_len = 0;
	u8 index;
	u32 hw_ver = 0;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "descramble handle IRQ\n");

	if (param == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	srccap_dev = (struct mtk_srccap_dev *)param;
	dev_id = (u8)srccap_dev->dev_id;
	index = srccap_dev->dv_info.descrb_info.buf.index;
	hw_ver = srccap_dev->srccap_info.cap.u32DV_Srccap_HWVersion;

	// Set CRC 12bit 444 according to color format
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC,
		"descramble IRQ color_format=%d\n",
		srccap_dev->dv_info.descrb_info.pkt_info.color_format);

	if (srccap_dev->dv_info.descrb_info.pkt_info.color_format
		== SRCCAP_DV_DESCRB_COLOR_FORMAT_RGB)
		en = FALSE;
	else
		en = TRUE;
	riu = TRUE;
	ret = drv_hwreg_srccap_dv_descrb_set_crc_12b_444_enable(dev_id, en, riu, NULL, NULL);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC,
		"descramble IRQ DV Srccap HW Version=%d\n", hw_ver);

	if (hw_ver >= DV_SRCCAP_HW_TAG_V2) {
		u32 dts_np_reg_val = 0;
		u32 hsize = 0;
		bool is_frl_mode = srccap_dev->timingdetect_info.data.frl;

		// Set 1248pack mode (np mode)
		dts_np_reg_val = (is_frl_mode == TRUE) ?
			DV_DESCRB_PACK_MODE_REG_VAL_8P : DV_DESCRB_PACK_MODE_REG_VAL_1P;

		ret = drv_hwreg_srccap_dv_descrb_set_1248pack_mode
			(dev_id, dts_np_reg_val, riu, NULL, NULL);
		if (ret < 0) {
			SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
			goto exit;
		}

		// Set input hsize
		hsize = srccap_dev->dv_info.dma_info.mem_fmt.src_width;
		ret = drv_hwreg_srccap_dv_descrb_set_hsize(dev_id, hsize, riu, NULL, NULL);
		if (ret < 0) {
			SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
			goto exit;
		}
	}

	// Enable descramble read back
	en = TRUE;
	riu = TRUE;
	ret = drv_hwreg_srccap_dv_descrb_set_r_enable(dev_id, en, riu, NULL, NULL);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	// Get first CRC
	pkt_num = 1;
	ret = dv_descrb_check_hw_crc(dev_id, pkt_num, &crc);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	// Check first CRC
	srccap_dev->dv_info.descrb_info.buf.crc[index] = crc;
	if (!crc) {
		SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC, "first crc fail\n");
		goto done;
	}

	// Get metadata length
	ret |= dv_descrb_read_data(dev_id, DV_DESCRB_META_LEN_ADDR_HI, &hi);
	ret |= dv_descrb_read_data(dev_id, DV_DESCRB_META_LEN_ADDR_LO, &lo);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	metadata_len = ((hi & MASK_BYTE1) | (lo & MASK_BYTE0));
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC, "metadata len=%u\n", metadata_len);

	// Get packet number
	if ((metadata_len % DV_DESCRB_PKT_REMAIN_LENGTH) == 0)
		pkt_num = (metadata_len / DV_DESCRB_PKT_REMAIN_LENGTH);
	else
		pkt_num = (metadata_len / DV_DESCRB_PKT_REMAIN_LENGTH) + 1;
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC, "pkt num=%u\n", pkt_num);

	if (pkt_num > DV_DESCRB_MAX_HW_SUPPORT_PKT) {
		SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC,
			"pkg num (%u) > DV_DESCRB_MAX_HW_SUPPORT_PKT (%u)\n",
			pkt_num, DV_DESCRB_MAX_HW_SUPPORT_PKT);
		goto exit;
	}

	// Check other CRC
	ret = dv_descrb_check_hw_crc(dev_id, pkt_num, &crc);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC, "crc=%u\n", crc);
	srccap_dev->dv_info.descrb_info.buf.crc[index] = crc;
	if (!crc)
		goto done;


	// Get metadata
	ret = dv_descrb_parse_md(
		dev_id,
		pkt_num,
		srccap_dev->dv_info.descrb_info.buf.data[index]);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	srccap_dev->dv_info.descrb_info.buf.data_length[index] =
		metadata_len;

	if (g_dv_debug_level & SRCCAP_DV_DBG_LEVEL_HDMI_DESC) {
		int sum = 0;

		mtk_dv_debug_checksum(
			srccap_dev->dv_info.descrb_info.buf.data[index],
			metadata_len,
			&sum);

		SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_HDMI_DESC,
			"metadata len=%u, sum=%u\n", metadata_len, sum);
	}

done:
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO,
		"descramble handle IRQ done (index=%d)\n", index);

	srccap_dev->dv_info.descrb_info.buf.index = (index) ? (0) : (1);

exit:

	/* disable descramble read back */
	en = FALSE;
	riu = TRUE;
	ret = drv_hwreg_srccap_dv_descrb_set_r_enable(dev_id, en, riu, NULL, NULL);
	if (ret < 0)
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);

	return;
}

//-----------------------------------------------------------------------------
// Debug Functions
//-----------------------------------------------------------------------------
int mtk_dv_descrb_set_stub(
	int stub)
{
	int ret = 0;
	bool dv_stub_enable = FALSE;
	enum hwreg_srccap_dv_descrb_stub_ctrl mode_ctrl = HWREG_SRCCAP_DV_DESCRB_STUB_CTRL_NONE;

	if (stub >= SRCCAP_STUB_MODE_DV && stub < SRCCAP_STUB_MODE_DV_MAX)
		dv_stub_enable = TRUE;
	else
		dv_stub_enable = FALSE;

	if (stub == SRCCAP_STUB_MODE_DV_VSIF_NEG)
		mode_ctrl |= HWREG_SRCCAP_DV_DESCRB_STUB_CTRL_NEG;

	ret = drv_hwreg_srccap_dv_descrb_set_stub_mode(dv_stub_enable, mode_ctrl);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO,
		"set srccap_dv_descrb stub mode = %d, ctrl = 0x%x\n", stub, mode_ctrl);

exit:
	return ret;
}

int mtk_dv_descrb_stub_get_test_avi(
	void *ptr,
	MS_U32 u32_buf_len)
{
	int ret = 0;
	struct hdmi_avi_packet_info *avi;

	if (!ptr)
		return ret;

	if (u32_buf_len < sizeof(struct hdmi_avi_packet_info))
		return ret;

	avi = (struct hdmi_avi_packet_info *)ptr;
	memset(avi, 0, sizeof(struct hdmi_avi_packet_info));

	/* Color Format */
	if (IS_SRCCAP_DV_STUB_MODE_RGB())
		avi->enColorForamt = M_HDMI_COLOR_RGB;
	else
		avi->enColorForamt = M_HDMI_COLOR_YUV_422;

	ret = 1;	// receive 1 pkt

	return ret;
}

int mtk_dv_descrb_stub_get_test_vsif(
	void *ptr,
	MS_U32 u32_buf_len)
{
	int ret = 0;
	struct hdmi_vsif_packet_info *vsif_array;
	struct srccap_dv_vsif_payload *vsif_payload = NULL;
	u32 ieee_oui = 0;
	int i;

	if (!ptr)
		return ret;

	if (u32_buf_len < sizeof(struct hdmi_vsif_packet_info))
		return ret;

	vsif_array = (struct hdmi_vsif_packet_info *)ptr;
	memset(vsif_array, 0, sizeof(struct hdmi_vsif_packet_info));

	/* stub mode test */
	vsif_array->u8hb0 = DV_DESCRB_VSIF_PKT_TYPE;
	vsif_array->u8version = DV_DESCRB_VSIF_VERSION;

	if (IS_SRCCAP_DV_STUB_MODE_H14B()) {
		vsif_array->u5length = DV_DESCRB_H14B_VSIF_LENGTH;
		ieee_oui = DV_DESCRB_H14B_IEEE_OUI_VALUE;
	} else {
		vsif_array->u5length = DV_DESCRB_VSIF_LENGTH;
		ieee_oui = DV_DESCRB_IEEE_OUI_VALUE;
	}

	vsif_array->u8crc = 1;
	for (i = 0; i < DV_DESCRB_IEEE_OUI_LENGTH; i++) {
		vsif_array->au8ieee[i] = GET_BYTE0(ieee_oui);
		ieee_oui >>= SHIFT_OFFSET_8;
	}

	/* PB4 */
	vsif_payload = (struct srccap_dv_vsif_payload *)vsif_array->au8payload;

	if (!IS_SRCCAP_DV_STUB_MODE_H14B())
		vsif_array->au8payload[0] = (DV_DESCRB_VSIF_SIGNAL_TYPE_MASK);

	/* LL */
	if (IS_SRCCAP_DV_STUB_MODE_LL_FORM2())
		vsif_array->au8payload[0] |= DV_DESCRB_VSIF_LL_MASK;

	/* DV game (LL RGB) */
	if (IS_SRCCAP_DV_STUB_MODE_LL_FORM2() && IS_SRCCAP_DV_STUB_MODE_RGB()) {
		vsif_payload->u1l11Md = 1;
		vsif_payload->u4contentType = DV_DESCRB_CONTENT_TYPE_GAME;
	}

	ret = 1;	// receive 1 pkt

	return ret;
}


int mtk_dv_descrb_stub_get_test_drm(
	void *ptr,
	MS_U32 u32_buf_len)
{
	int ret = 0;
	struct hdmi_hdr_packet_info *drm_array;

	if (!ptr)
		return ret;

	if (u32_buf_len < sizeof(struct hdmi_hdr_packet_info))
		return ret;

	drm_array = (struct hdmi_hdr_packet_info *)ptr;
	memset(drm_array, 0, sizeof(struct hdmi_hdr_packet_info));

	/* stub mode test */
	drm_array->u8EOTF = DV_DESCRB_DRM_EOTF_VALUE;
	drm_array->u8Static_Metadata_ID = DV_DESCRB_DRM_STATIC_METADATA_ID_VALUE;
	drm_array->u16Display_Primaries_X[INDEX_0] = DV_DESCRB_DRM_PRIMARIES_X_R_VALUE;
	drm_array->u16Display_Primaries_X[INDEX_1] = DV_DESCRB_DRM_PRIMARIES_X_G_VALUE;
	drm_array->u16Display_Primaries_X[INDEX_2] = DV_DESCRB_DRM_PRIMARIES_X_B_VALUE;
	drm_array->u16Display_Primaries_Y[INDEX_0] = DV_DESCRB_DRM_PRIMARIES_Y_R_VALUE;
	drm_array->u16Display_Primaries_Y[INDEX_1] = DV_DESCRB_DRM_PRIMARIES_Y_G_VALUE;
	drm_array->u16Display_Primaries_Y[INDEX_2] = DV_DESCRB_DRM_PRIMARIES_Y_B_VALUE;
	drm_array->u16White_Point_X = DV_DESCRB_DRM_WHITE_POINT_X_VALUE;
	drm_array->u16White_Point_Y = DV_DESCRB_DRM_WHITE_POINT_Y_VALUE;
	drm_array->u16Max_Display_Mastering_Luminance = DV_DESCRB_DRM_MAX_LUMA_VALUE;
	drm_array->u16Min_Display_Mastering_Luminance = DV_DESCRB_DRM_MIN_LUMA_VALUE;
	drm_array->u16Maximum_Content_Light_Level = DV_DESCRB_DRM_MAX_CLL_VALUE;
	drm_array->u16Maximum_Frame_Average_Light_Level = DV_DESCRB_DRM_MAX_FALL_VALUE;
	drm_array->u8Version = DV_DESCRB_DRM_VERSION;
	drm_array->u8Length = DV_DESCRB_DRM_LENGTH;


	ret = 1;	// receive 1 pkt

	return ret;
}

int mtk_dv_descrb_sutb_get_test_emp(
	void *ptr,
	MS_U32 u32_buf_len)
{
	int ret = 0;
	struct hdmi_emp_packet_info *emp_array;
	int emp_size;
	u32 ieee_oui = DV_DESCRB_IEEE_OUI_VALUE;
	u32 expected_crc = 0;
	u8 offset = 0;
	int i;

	if (!ptr)
		return ret;

	if (u32_buf_len < sizeof(struct hdmi_emp_packet_info))
		return ret;

	emp_array = (struct hdmi_emp_packet_info *)ptr;
	memset(emp_array, 0, sizeof(struct hdmi_emp_packet_info));

	/* stub mode test */
	emp_array->u8seq_index = 0;
	emp_array->u8organ_id = 0;
	for (i = 0; i < DV_DESCRB_IEEE_OUI_LENGTH; i++) {
		offset = (DV_DESCRB_EMP_IEEE_OUI_OFFSET + i);
		emp_array->au8md[offset] = GET_BYTE0(ieee_oui);
		ieee_oui >>= SHIFT_OFFSET_8;
	}

	/* version */
	offset = DV_DESCRB_EMP_DATA_VERSION_OFFSET;
	if (IS_SRCCAP_DV_STUB_MODE_LL_FORM2())
		emp_array->au8md[offset] = DV_DESCRB_EMP_DATA_VERSION_FORM2;
	else
		emp_array->au8md[offset] = DV_DESCRB_EMP_DATA_VERSION_FORM1;

	/* size */
	emp_size = (DV_DESCRB_EMP_PAYLOAD_SIZE - DV_DESCRB_EMP_HEADER_SIZE);
	emp_array->u8data_set_len_msb = GET_BYTE1(emp_size);
	emp_array->u8data_set_len_lsb = GET_BYTE0(emp_size);


	/* edr_data */
	/* DV game (Form2 RGB) */
	if (IS_SRCCAP_DV_STUB_MODE_LL_FORM2() && IS_SRCCAP_DV_STUB_MODE_RGB()) {
		struct srccap_dv_vsif_payload *form2_data = NULL;

		offset = DV_DESCRB_EMP_EDR_DATA_OFFSET;
		form2_data = (struct srccap_dv_vsif_payload *)(&emp_array->au8md[offset]);
		form2_data->u1l11Md = 1;
		form2_data->u4contentType = DV_DESCRB_CONTENT_TYPE_GAME;
	}


	/* crc */
	emp_size = DV_DESCRB_EMP_PAYLOAD_SIZE;
	offset = DV_DESCRB_EMP_EDR_DATA_OFFSET;
	dv_descrb_get_crc32(
		&(emp_array->au8md[offset]),
		(emp_size - DV_DESCRB_EMP_EDR_DATA_OFFSET - DV_DESCRB_CRC_LENGTH),
		&expected_crc);
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO,
		"expected_crc = 0x%08x\n", expected_crc);

	if (IS_SRCCAP_DV_STUB_MODE_NEG())
		expected_crc++;		// SW CRC fail

	offset = emp_size - DV_DESCRB_CRC_LENGTH;
	i = DV_DESCRB_CRC_LENGTH;
	while (i--) {
		emp_array->au8md[(offset + i)] = GET_BYTE0(expected_crc);
		expected_crc >>= SHIFT_OFFSET_8;
	}

	ret = 1;	// receive 1 pkt

	return ret;
}
