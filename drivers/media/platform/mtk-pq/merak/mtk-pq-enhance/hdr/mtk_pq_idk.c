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
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <linux/videodev2.h>
#include <linux/uaccess.h>

#include "mtk_pq.h"
#include "mtk_pq_buffer.h"
#include "hwreg_pq_display_scmi.h"
#include "drv_hwreg_pq_dovi_hw5_top.h"

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define IDK_MAX_PATH_NAME_LENGTH    (256)
#define IDK_MAX_OPTION_LENGTH         (7)
#define IDK_MAX_BYPASS_SCRIPT_SIZE (4096)
#define IDK_MAX_SCRIPT_LINE_LENGTH   (64)
#define IDK_POS_NEWLINE              (2)
#define IDK_POS_NEWLINE_CHAR         (0xD)
#define IDK_POS_WRITE_TYPE           (6)
#define IDK_POS_WRITE_ADDR           (8)
#define IDK_COUNT_W_MODE             (2)
#define IDK_COUNT_B_MODE             (3)
#define IDK_COUNT_COLOR              (3)
#define IDK_COUNT_BIT_WIDTH          (12)
#define IDK_COUNT_BIT_PER_BYTE       (8)
#define IDK_COUNT_STRLEN_WRIU        (4)
/* Bytes per Pixel for YUV422 12bit is 4 */
#define IDK_COUNT_YUV422_12b_BPP     (4)
#define IDK_COUNT_BITS_ALIGNMENT     (256)

#define IDK_OFFSET_1                 (1)
#define IDK_OFFSET_2                 (2)
#define IDK_OFFSET_3                 (3)
#define IDK_OFFSET_4                 (4)
#define IDK_OFFSET_5                 (5)
#define IDK_OFFSET_6                 (6)
#define IDK_OFFSET_7                 (7)

#define IDK_MASK_BYTE_HIGH           (0xF0)
#define IDK_MASK_BYTE_LOW            (0x0F)
#define IDK_MASK_BYTE_SHIFT          (4)

#define IDK_COUNT_INBUF_JUMP         (9)
#define IDK_COUNT_OUTBUF_JUMP        (8)

#define SYSFS_MAX_BUF_COUNT       (0x100)

#define Fld(wid, shft, ac)    (((uint32_t)wid<<16)|(shft<<8)|ac)
#define AC_FULLB0           1
#define AC_FULLB1           2
#define AC_FULLB2           3
#define AC_FULLB3           4
#define AC_FULLW10          5
#define AC_FULLW21          6
#define AC_FULLW32          7
#define AC_FULLDW           8
#define AC_MSKB0            9
#define AC_MSKB1            10
#define AC_MSKB2            11
#define AC_MSKB3            12
#define AC_MSKW10           13
#define AC_MSKW21           14
#define AC_MSKW32           15
#define AC_MSKDW            16

#define MASK_FULLDW		(0xFFFFFFFF)
#define MASK_FULLW32		(0xFFFF0000)
#define MASK_FULLW21		(0x00FFFF00)
#define MASK_FULLW10		(0x0000FFFF)
#define MASK_FULLB3		(0xFF000000)
#define MASK_FULLB2		(0x00FF0000)
#define MASK_FULLB1		(0x0000FF00)
#define MASK_FULLB0		(0x000000FF)

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
uint16_t gu16IDKEnable;

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------
#ifdef DOLBY_IDK_DUMP_ENABLE
static unsigned int _mtk_pq_idkdump_get_bits_set(unsigned int v)
{
	/* c accumulates the total bits set in v */
	unsigned int c;

	for (c = 0; v; c++)
		v &= v - 1; /* clear the least significant bit set */

	return c;
}

static unsigned int _mtk_pq_idkdump_get_AC(uint32_t mask)
{
	bool MaskB0 = false, MaskB1 = false, MaskB2 = false, MaskB3 = false;

	switch (mask) {
	case MASK_FULLDW:
		return AC_FULLDW;
	case MASK_FULLW32:
		return AC_FULLW32;
	case MASK_FULLW21:
		return AC_FULLW21;
	case MASK_FULLW10:
		return AC_FULLW10;
	case MASK_FULLB3:
		return AC_FULLB3;
	case MASK_FULLB2:
		return AC_FULLB2;
	case MASK_FULLB1:
		return AC_FULLB1;
	case MASK_FULLB0:
		return AC_FULLB0;
	}

	MaskB0 = !!(mask & MASK_FULLB0);
	MaskB1 = !!(mask & MASK_FULLB1);
	MaskB2 = !!(mask & MASK_FULLB2);
	MaskB3 = !!(mask & MASK_FULLB3);

	if (MaskB3 && MaskB2 && MaskB1 && MaskB0)
		return AC_MSKDW;
	else if (MaskB3 && MaskB2)
		return AC_MSKW32;
	else if (MaskB2 && MaskB1)
		return AC_MSKW21;
	else if (MaskB1 && MaskB0)
		return AC_MSKW10;
	else if (MaskB3)
		return AC_MSKB3;
	else if (MaskB2)
		return AC_MSKB2;
	else if (MaskB1)
		return AC_MSKB1;
	else if (MaskB0)
		return AC_MSKB0;

	return 0;
}

static int _mtk_pq_idkdump_bypass_checkfile(char *buf)
{
	struct kstat kFileStat;

	if (buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	memset(&kFileStat, 0, sizeof(struct kstat));

	/* find file size */
	if (vfs_stat(buf, &kFileStat) != 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Cannot find size of file:%s!\n", buf);
		return -EINVAL;
	}

	/* verify file size is not too large */
	if (kFileStat.size >= IDK_MAX_BYPASS_SCRIPT_SIZE) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Script size:%lld is too large!\n", kFileStat.size);
		return -EINVAL;
	}

	return kFileStat.size;
}
#endif

static int _mtk_pq_idkdump_bypass(char *buf)
{
#ifdef DOLBY_IDK_DUMP_ENABLE
	int ret = 0;
	int file_size = 0;
	int line_number = 0;
	unsigned int mask_width = 0, AC, LSB;
	uint32_t  u32Addr = 0, u32Value = 0, u32Mask = 0;
	char  *pu8ScriptBuf = NULL;
	char  *pu8ScriptBuf_ori = NULL;
	char  *pu8EachLine = NULL;
	char   u8LineCmd[IDK_MAX_SCRIPT_LINE_LENGTH];
	struct file *pFile = NULL;
	mm_segment_t old_fs;

	if (buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "bypass script path:%s\n", buf);

	/* start read from file */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pFile = filp_open(buf, O_RDONLY, 0);

	if (IS_ERR(pFile)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Cannot Open file %s!\n", buf);
		ret = -EINVAL;
		goto close_file;
	}

	file_size = _mtk_pq_idkdump_bypass_checkfile(buf);
	if (file_size <= 0)
		goto close_file;

	/* malloc space to store script file +1 is to add a \0 to end this file */
	pu8ScriptBuf = vmalloc(file_size+1);
	if (pu8ScriptBuf == NULL)
		goto close_file;

	pu8ScriptBuf_ori = pu8ScriptBuf;
	memset(pu8ScriptBuf, 0, file_size+1);

	/* read file into buffer */
	ret = kernel_read(pFile, pu8ScriptBuf, file_size, &pFile->f_pos);
	if (ret != file_size) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "cannot read from file:%s, read %d bytes only\n",
			buf, file_size);
		ret = -EINVAL;
		goto free_buf;
	}
	ret = 0;

	pu8EachLine = strsep(&pu8ScriptBuf, "\x0A");
	while (pu8EachLine != NULL) {
		line_number++;

		if (pu8ScriptBuf == NULL) {
			/* last line */
			if (strlen(pu8EachLine) >= IDK_MAX_SCRIPT_LINE_LENGTH)
				continue;
		} else if (pu8ScriptBuf-pu8EachLine >= IDK_MAX_SCRIPT_LINE_LENGTH) {
			pu8EachLine = strsep(&pu8ScriptBuf, "\x0A");
			continue;
		}

		if (pu8ScriptBuf == NULL) {
			scnprintf(u8LineCmd, IDK_MAX_SCRIPT_LINE_LENGTH, "%s", pu8EachLine);
		} else {
			scnprintf(u8LineCmd, pu8ScriptBuf-pu8EachLine, "%s", pu8EachLine);
			/* remove \0x0D for window file new line */
			if (u8LineCmd[pu8ScriptBuf-pu8EachLine-IDK_POS_NEWLINE]
				== IDK_POS_NEWLINE_CHAR)
				u8LineCmd[pu8ScriptBuf-pu8EachLine-IDK_POS_NEWLINE] = 0;
		}

		if (strlen(u8LineCmd) <= IDK_COUNT_STRLEN_WRIU) {
			pu8EachLine = strsep(&pu8ScriptBuf, "\x0A");
			continue;
		}

		if (strncmp(u8LineCmd, "wriu", IDK_COUNT_STRLEN_WRIU) == 0) {
			if ((*(u8LineCmd+IDK_POS_WRITE_TYPE)) == 'w') {
				if (sscanf(u8LineCmd+IDK_POS_WRITE_ADDR, "0x%x 0x%x", &u32Addr,
					&u32Value)
					!= IDK_COUNT_W_MODE)
					continue;

				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE,
					"line%d: w u32Addr:%x u32Value:%x\n",
					line_number, u32Addr, u32Value);
				drv_hwreg_pq_enhance_idk_write2byte(
					(u32Addr+(u32Addr&0xFF))<<1, u32Value);
			} else if ((*(u8LineCmd+IDK_POS_WRITE_TYPE)) == 'b') {
				if (sscanf(u8LineCmd+IDK_POS_WRITE_ADDR, "0x%x 0x%x 0x%x",
					&u32Addr, &u32Value, &u32Mask) != IDK_COUNT_B_MODE)
					continue;

				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE,
					"line%d: b u32Addr:%x u32Value:%x u32Mask:%x\n",
					line_number, u32Addr, u32Value, u32Mask);

				/* get mask width [15:8] gives 8. mask must be continuous */
				mask_width = _mtk_pq_idkdump_get_bits_set(u32Mask);

				/* get LSB */
				LSB = ffs(u32Mask)-1;

				/* get AC */
				AC = _mtk_pq_idkdump_get_AC(u32Mask);

				if (AC == 0) {
					STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
						"line%d: cannot identify mask:%x\n",
						line_number, u32Mask);
					ret = -EINVAL;
					goto free_buf;
				}

				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE,
					"write cmd:Addr:%x u32Value:%x mask_width:%d LSB:%d AC:%d\n",
					(u32Addr+(u32Addr&0xFF))<<1, u32Value >> LSB,
					mask_width, LSB, AC);

				drv_hwreg_pq_enhance_idk_write2bytemask(
					(u32Addr+(u32Addr&0xFF))<<1,
					u32Value >> LSB,
					Fld(mask_width, LSB, AC));
			}
		}

		pu8EachLine = strsep(&pu8ScriptBuf, "\x0A");
	}

free_buf:
	vfree(pu8ScriptBuf_ori);

close_file:
	set_fs(old_fs);
	filp_close(pFile, NULL);
	pFile = NULL;

	return ret;
#else
	return -ESRCH;
#endif
}

static void _mtk_pq_idkdump_printHelp(void)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "Command format is incorrect!\n");
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "Usage:\n                      ");
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "\"enable=1\" or 0 to enable/disable IDK");
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "\"bypass=<bypass_script_path>\"");
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "to load the bypass script file");
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "\"dump=<dump_folder>\"");
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "to dump the current frame under DV IDK mode\n");
}

#ifdef DOLBY_IDK_DUMP_ENABLE
static void _mtk_pq_idkdump_YUV444toUYVY422(__u8 *in_buf, unsigned int hSize, unsigned int vSize,
						unsigned int hOffset, unsigned int alignment,
						__u8 *out_buf)
{
	unsigned int vCounter, i;
	unsigned int residual_bytes = 0;
	unsigned int group422 = 0;

	if ((in_buf == NULL) || (out_buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return;
	}

	/* start to convert from YUV 12 bit packed to YUV422 UYVY 12bit packed*/
	residual_bytes = ((hSize * IDK_COUNT_COLOR * IDK_COUNT_BIT_WIDTH) % alignment)
			/IDK_COUNT_BIT_PER_BYTE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE,
		"residual bytes(for %d bits alignment): %d\n", alignment, residual_bytes);
	group422 = hSize>>1; /* A group is 2 pixel = 2x12x3=72 bits = 9 bytes */
	for (vCounter = 0; vCounter < vSize; vCounter++) {
		for (i = 0; i < group422; i++) {
			/* convert Y0U0V0,Y1U1V1 to U0Y0V0Y1 */
			/* U0, last 8 bit */
			*(out_buf)     =    *(in_buf);
			/* U0, rest 4 bit */
			*(out_buf + IDK_OFFSET_1) = (*(in_buf + IDK_OFFSET_1)) & IDK_MASK_BYTE_LOW;
			/* Y0, last 8 bit */
			*(out_buf + IDK_OFFSET_2) =
				(((*(in_buf + IDK_OFFSET_1)) & IDK_MASK_BYTE_HIGH)
					>> IDK_MASK_BYTE_SHIFT) |
				(((*(in_buf+IDK_OFFSET_2)) & IDK_MASK_BYTE_LOW)
					<< IDK_MASK_BYTE_SHIFT);
			/* Y0, rest 4 bit */
			*(out_buf + IDK_OFFSET_3) =
				(((*(in_buf + IDK_OFFSET_2)) & IDK_MASK_BYTE_HIGH)
					>> IDK_MASK_BYTE_SHIFT);
			/* V0, last 8 bit */
			*(out_buf + IDK_OFFSET_4) =    *(in_buf + IDK_OFFSET_3);
			/* V0, rest 4 bit */
			*(out_buf + IDK_OFFSET_5) = (*(in_buf + IDK_OFFSET_4)) & IDK_MASK_BYTE_LOW;
			/* Y1, last 8 bit */
			*(out_buf + IDK_OFFSET_6) =    *(in_buf + IDK_OFFSET_6);
			/* Y1, rest 4 bit */
			*(out_buf + IDK_OFFSET_7) = (*(in_buf + IDK_OFFSET_7)) & IDK_MASK_BYTE_LOW;

			/* consumes in-buffer 9bytes = 72bits = 12x3x2bits */
			in_buf += IDK_COUNT_INBUF_JUMP;
		/* generates out-buffer 8bytes = UYVY 4channel x 2bytes to store 12bit data. */
			out_buf += IDK_COUNT_OUTBUF_JUMP;
		}

		if (residual_bytes != 0)
			in_buf += residual_bytes;   /* these are paddings for 256bits.*/

		/* if hOffset is different as hSize, need to neglect the rest bytes until offset */
		if (hOffset != hSize)
			in_buf += (hOffset-hSize)*IDK_COUNT_COLOR*IDK_COUNT_BIT_WIDTH
					/IDK_COUNT_BIT_PER_BYTE - residual_bytes;
	}
}

static int _mtk_pq_idkdump_get_pq_buffer(struct mtk_pq_platform_device *pqdev, __u8 **in_buf,
					unsigned int window_id)
{
	struct pq_buffer pq_buf;

	if ((pqdev == NULL) || (in_buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	memset(&pq_buf, 0, sizeof(struct pq_buffer));

	/* try use devs0, but is this always correct? */
	if (mtk_get_pq_buffer(pqdev->pq_devs[window_id], PQU_BUF_SCMI, &pq_buf) < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Get PQ buffer error!\n");
		return -EINVAL;
	}

	if (pq_buf.va == 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "PQ buffer is NULL!\n");
		return -EINVAL;
	}

	*in_buf = (__u8 *)pq_buf.va;

	return 0;
}
#endif


static int _mtk_pq_idkdump_dump(struct device *dev, char *buf)
{
#ifdef DOLBY_IDK_DUMP_ENABLE
	int ret = 0;
	bool bIsZero = 0;
	unsigned int window_id = 0;
	struct mtk_pq_platform_device *pqdev = NULL;
	char pu8DumpPath[IDK_MAX_PATH_NAME_LENGTH];
	char pu8WindowID[IDK_MAX_OPTION_LENGTH];
	char *pCommaPos = NULL;
	struct file *pFile = NULL;

	uint16_t u16hSize = 0;
	uint16_t u16vSize = 0;
	uint16_t u16hOffset = 0;

	unsigned int write_size = 0;

	__u8 *in_buf;
	__u8 *out_buf;
	__u8 *out_buf_head;

	mm_segment_t old_fs;

	drv_hwreg_pq_display_scmi_get_hSize(&u16hSize);
	drv_hwreg_pq_display_scmi_get_vSize(&u16vSize);
	drv_hwreg_pq_display_scmi_get_hOffset(&u16hOffset);

	bIsZero = !((!!u16hSize) && (!!u16vSize) && (!!u16hOffset) && (!!dev) && (!!buf));

	if (bIsZero || (u16hOffset < u16hSize)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid hSize:%d vSize:%d hOffset:%d or input pointer\n",
			u16hSize, u16vSize, u16hOffset);
		return -EINVAL;
	}

	pqdev = dev_get_drvdata(dev);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE,
		"u16hSize:%d u16vSize:%d u16hOffset:%d\n",
		u16hSize, u16vSize, u16hOffset);

	/* check buf is "PATH" or "WINDOWID,PATH" */
	pCommaPos = strchr(buf, ',');
	if (pCommaPos != NULL) {
		/* not null means WINDOWID,PATH */
		if (pCommaPos - buf >= IDK_MAX_OPTION_LENGTH) {
			/* if the string before "," is too long */
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"window id string is longer than %d!\n", IDK_MAX_OPTION_LENGTH);
			return -EINVAL;
		}

		/* convert windowID string to number and check value */
		memcpy(pu8WindowID, buf, pCommaPos - buf);
		window_id = strtoul(pu8WindowID, NULL, 0);
		if (window_id >= PQ_WIN_MAX_NUM) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"window id %d  is longer than MAX %d!\n",
				window_id, PQ_WIN_MAX_NUM);
			return -EINVAL;
		}

		ret = scnprintf(pu8DumpPath, IDK_MAX_PATH_NAME_LENGTH,
			"%s/%dx%d_UYVY_12b.yuv", pCommaPos+1, u16hSize, u16vSize);
		if ((ret < 0) || (ret >= IDK_MAX_PATH_NAME_LENGTH)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"dump path is larger than %d!\n", IDK_MAX_PATH_NAME_LENGTH);
			return -EINVAL;
		}

	} else {
		/* buf should be PATH only, take window id as 0" */
		window_id = 0;
		ret = scnprintf(pu8DumpPath, IDK_MAX_PATH_NAME_LENGTH,
			"%s/%dx%d_UYVY_12b.yuv", buf, u16hSize, u16vSize);
		if ((ret < 0) || (ret >= IDK_MAX_PATH_NAME_LENGTH)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"dump path is larger than %d!\n", IDK_MAX_PATH_NAME_LENGTH);
			return -EINVAL;
		}
	}

	if (_mtk_pq_idkdump_get_pq_buffer(pqdev, &(in_buf), window_id) < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Get PQ buffer error!\n");
		return -EPERM;
	}

	/* malloc a buffer to contain re-arranged frame data */
	/* Bytes per Pixel for YUV422 12bit is 4 */
	write_size = u16hSize*u16vSize*IDK_COUNT_YUV422_12b_BPP;
	out_buf = vmalloc(write_size);
	if (out_buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "failed to malloc size:%d\n", write_size);
		return -ENOMEM;
	}
	out_buf_head = out_buf;
	memset(out_buf, 0, write_size);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE,
		"dump size:%d dump path:%s\n", write_size, pu8DumpPath);

	/* data format conversion */
	_mtk_pq_idkdump_YUV444toUYVY422(in_buf, u16hSize, u16vSize, u16hOffset,
		IDK_COUNT_BITS_ALIGNMENT, out_buf);

	/* start write to file */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pFile = filp_open(pu8DumpPath, O_CREAT | O_RDWR | O_TRUNC, O_RDWR);

	if (!IS_ERR(pFile)) {
		if (kernel_write(pFile, (char *)(out_buf_head), write_size, &pFile->f_pos)
			< 0){
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE,
				"write filename = %s fail!\n", pu8DumpPath);
		}
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE,
			"Open filename = %s fail!\n", pu8DumpPath);
	}

	set_fs(old_fs);
	if (!IS_ERR(pFile)) {
		filp_close(pFile, NULL);
		pFile = NULL;
	}
	vfree(out_buf_head);

	return 0;
#else
	return -ESRCH;
#endif
}

//-----------------------------------------------------------------------------
// Global Functions
//-----------------------------------------------------------------------------
int _mtk_pq_idkdump_store(struct device *dev, const char *buf)
{
	int ret = 0;
	char pu8InputCommand[IDK_MAX_PATH_NAME_LENGTH];
	char pu8TargetPath[IDK_MAX_PATH_NAME_LENGTH];
	char pu8Option[IDK_MAX_OPTION_LENGTH];
	char *pu8EqualSign;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	/* remove the trailing '\0a' */
	ret = scnprintf(pu8InputCommand, IDK_MAX_PATH_NAME_LENGTH, "%s", buf);
	if ((ret < 0) || (ret >= IDK_MAX_PATH_NAME_LENGTH)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"input command is larger than %d!\n", IDK_MAX_PATH_NAME_LENGTH);
		return -EINVAL;
	}
	pu8InputCommand[strlen(pu8InputCommand)-1] = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE, "input command:%s\n", pu8InputCommand);
	pu8EqualSign = strchr(pu8InputCommand, '=');

	if (pu8EqualSign == NULL) {
		_mtk_pq_idkdump_printHelp();
		return -EINVAL;
	}

	pu8InputCommand[pu8EqualSign-pu8InputCommand] = 0;

	/* find option string */
	ret = scnprintf(pu8Option, IDK_MAX_OPTION_LENGTH, "%s", pu8InputCommand);
	if (ret == 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "no option found!\n");
		return -EINVAL;
	}

	if (strncmp(pu8Option, "enable", sizeof(pu8Option)) == 0) {
		ret = scnprintf(pu8Option, IDK_MAX_OPTION_LENGTH, "%s", pu8EqualSign+1);
		if ((ret < 0) || (ret >= IDK_MAX_OPTION_LENGTH)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"enable option is larger than %d!\n", IDK_MAX_OPTION_LENGTH);
			return -EINVAL;
		}

		if (strncmp(pu8Option, "1", sizeof(pu8Option)) == 0)
			gu16IDKEnable = 1;
		else if (strncmp(pu8Option, "0", sizeof(pu8Option)) == 0)
			gu16IDKEnable = 0;
		else
			_mtk_pq_idkdump_printHelp();

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ENHANCE,
			"Set IDK enable Flag as %d!\n", gu16IDKEnable);
		return 0;
	} else if (strncmp(pu8Option, "bypass", sizeof(pu8Option)) == 0) {
		scnprintf(pu8TargetPath, IDK_MAX_PATH_NAME_LENGTH, "%s", pu8EqualSign+1);
		/* do not check again since pu8EqualSign is part of size IDK_MAX_PATH_NAME_LENGTH */

		/* use this string as path to bypass script */
		ret = _mtk_pq_idkdump_bypass(pu8TargetPath);
	} else if (strncmp(pu8Option, "dump", sizeof(pu8Option)) == 0) {
		scnprintf(pu8TargetPath, IDK_MAX_PATH_NAME_LENGTH, "%s", pu8EqualSign+1);
		/* do not check again since pu8EqualSign is part of size IDK_MAX_PATH_NAME_LENGTH */

		/* use this string as folder to output dump file */
		ret = _mtk_pq_idkdump_dump(dev, pu8TargetPath);
	} else {
		_mtk_pq_idkdump_printHelp();
		return -EINVAL;
	}

	return ret;
}

int _mtk_pq_idkdump_show(struct device *dev, char *buf)
{
	ssize_t count = 0;
	ssize_t total_count = 0;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"enable_status:%d\n", gu16IDKEnable);
	total_count += count;

	return total_count;
}
