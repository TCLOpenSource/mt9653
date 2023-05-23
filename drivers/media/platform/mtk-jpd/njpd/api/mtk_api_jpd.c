// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021-2022 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------------------------------------


#include "mtk_sti_msos.h"
#include "mtk_api_jpd.h"
#include "mtk_drv_jpd.h"
#include "mtk_hal_jpd.h"
#include "mtk_reg_jpd.h"

//-------------------------------------------------------------------------------------------------
// Local Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Local Defines
//-------------------------------------------------------------------------------------------------
#define LEVEL_API_ALWAYS 0
#define LEVEL_API_TRACE 0x1
#define LEVEL_API_DEBUG 0x2
#define LEVEL_API_PRINTMEM 0x4
#define LEVEL_API_IRQ 0x8

#define ISR_EVENT_NUM 16
#define DELAY_100us 100
#define DELAY_10us 10
#define CHECK_ERR_CNT 100
#define MAX_PARSER_COUNT 16
#define BUF_PARTITION 2
#define BYTES_LEFT_MAX_SIZE 0x1000
#define MWC_LINE_NUM 32
#define SCAN_DEPTH 1536
#define HUFF_NUM_SIZE 17
#define HUFF_VAL_SIZE 256
#define MAX_TABLE_SIZE 16
#define GTABLE_DC_SIZE 12
#define GTABLE_AC_SIZE 162
#define QTABLE_SIZE 128

static int jpd_api_debug;
module_param(jpd_api_debug, int, 0644);

#define njpd_parser_debug(handle, level, fmt, args...) \
	do { \
		if ((jpd_api_debug & level) == level) \
			pr_info("jpd_api: parser(%d) %s: " fmt "\n", handle, __func__, ##args); \
	} while (0)

#define njpd_debug(level, fmt, args...) \
	do { \
		if ((jpd_api_debug & level) == level) \
			pr_info("jpd_api: %s: " fmt "\n", __func__, ##args); \
	} while (0)

#define njpd_parser_err(handle, fmt, args...) \
	pr_err("jpd_api: parser(%d)[ERR] %s: " fmt "\n", handle, __func__, ##args)

#define njpd_err(fmt, args...) \
	pr_err("jpd_api: [ERR] %s: " fmt "\n", __func__, ##args)

#if SUPPORT_PROGRESSIVE
#define HUFF_EXTEND_TBL(x, s) ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))
#define HUFF_EXTEND_P(x, s) HUFF_EXTEND_TBL(x, s)
#endif
//-------------------------------------------------------------------------------------------------
// Local Enumerations
//-------------------------------------------------------------------------------------------------

typedef enum {
	E_SCALE_ORG = 1,
	E_SCALE_HALF = 2,
	E_SCALE_FOURTH = 4,
	E_SCALE_EIGHTH = 8,
} SCALE_NUM;

typedef enum {
	E_COM0 = 0,
	E_COM1 = 1,
	E_COM2 = 2,
	E_COM3 = 3,
} COM_NUM;

//-------------------------------------------------------------------------------------------------
// Local Structures
//-------------------------------------------------------------------------------------------------
typedef struct {
	bool active;
	NJPD_API_AllocBufFunc allocBufFunc;
	NJPD_API_FreeBufFunc freeBufFunc;
	void *priv;
	NJPD_API_Format format;
	bool is_iommu;
	NJPD_API_DownScaleFactor scale_factor;
	NJPD_API_Rect rect;
	NJPD_API_Rect roi_rect;
	NJPD_API_Pic output_res;
	NJPD_API_Pic max_res;
	NJPD_API_Pic down_scale;
	bool has_max_res;
	bool has_down_scale;
	NJPD_API_ImageInfo image_info;
	NJPD_OutputFormat out_format;
	NJPD_VerificationMode vry_mode;

	u8 *read_point;
	NJPD_API_Buf *read_buffer;
	size_t data_fill;
	size_t data_left;
	size_t last_marker_offset;
	size_t out_buf_size;
	bool is_progressive;
	bool check_SOI;
	bool check_DHT;
	bool check_DQT;
	bool check_SOS;
	NJPD_TableInfo table_info;
	NJPD_HuffInfo huff_info[NJPD_MAXHUFFTABLES];
	NJPD_QuantTbl quant_table[NJPD_MAXQUANTTABLES];
	NJPD_SOFInfo sof_info;
	NJPD_SOSInfo sos_info;

	bool need_more_data;
	bool has_last_buffer;
	bool last_buffer_used;
	size_t last_buffer_length;
	void *last_buffer_data;
#if SUPPORT_PROGRESSIVE
	NJPD_ProgInfo prog_info;
	bool first_sos;
#endif
} NJPD_Parser_Info;

typedef struct NJPD_Read_Buffer_Node_t {
	struct NJPD_Read_Buffer_Node_t *next;
	NJPD_API_Buf buf;
} NJPD_Read_Buffer_Node;

typedef struct {
	bool buffer_empty[BUF_PARTITION];
	bool is_progressive;
	u8 next_index;
	u16 consume_num;
	NJPD_Read_Buffer_Node *read_list;
	NJPD_Read_Buffer_Node *read_list_tail;
	bool no_more_data;
#if SUPPORT_PROGRESSIVE
	bool bWriteRLE;
	NJPD_ProgInfo prog_info;
	u8 u8ComponentsInScan;
	u8 u8ComponentsInFrame;
	u8 u8ComponentList[NJPD_MAXCOMPSINSCAN];
	u8 u8ComponentHSamp[NJPD_MAXCOMPONENTS];
	u8 u8ComponentVSamp[NJPD_MAXCOMPONENTS];
	NJPD_API_Buf progressive_read_buffer;
#endif
} NJPD_Decoder_Info;

typedef struct {
	int jpd_irq;
	bool isrEvents[ISR_EVENT_NUM];
	mtk_jpd_riu_banks stBankOffsets;
} NJPD_API_CTX;

//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------

static NJPD_Parser_Info gNJPDParserInfo[MAX_PARSER_COUNT];
static NJPD_API_CTX gNJPDContext;
static NJPD_Decoder_Info gNJPDDecoderInfo;

static const u8 cgDefault_DHT[] = {
	0x01, 0xa2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
	0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x10,
	0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
	0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d,
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
	0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x11,
	0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
	0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};

static const u8 _u8Jpeg_zigzag_order[64] = {
	0, 2, 3, 9, 10, 20, 21, 35,
	1, 4, 8, 11, 19, 22, 34, 36,
	5, 7, 12, 18, 23, 33, 37, 48,
	6, 13, 17, 24, 32, 38, 47, 49,
	14, 16, 25, 31, 39, 46, 50, 57,
	15, 26, 30, 40, 45, 51, 56, 58,
	27, 29, 41, 44, 52, 55, 59, 62,
	28, 42, 43, 53, 54, 60, 61, 63
};

static const u16 g16IQ_TBL_NJPD[QTABLE_SIZE] = {
	0x0010, 0x000c, 0x000e, 0x000e, 0x0012, 0x0018, 0x0031, 0x0048,
	0x000b, 0x000c, 0x000d, 0x0011, 0x0016, 0x0023, 0x0040, 0x005c,
	0x000a, 0x000e, 0x0010, 0x0016, 0x0025, 0x0037, 0x004e, 0x005f,
	0x0010, 0x0013, 0x0018, 0x001d, 0x0038, 0x0040, 0x0057, 0x0062,
	0x0018, 0x001a, 0x0028, 0x0033, 0x0044, 0x0051, 0x0067, 0x0070,
	0x0028, 0x003a, 0x0039, 0x0057, 0x006d, 0x0068, 0x0079, 0x0064,
	0x0033, 0x003c, 0x0045, 0x0050, 0x0067, 0x0071, 0x0078, 0x0067,
	0x003d, 0x0037, 0x0038, 0x003e, 0x004d, 0x005c, 0x0065, 0x0063,

	0x0011, 0x0012, 0x0018, 0x002f, 0x0063, 0x0063, 0x0063, 0x0063,
	0x0012, 0x0015, 0x001a, 0x0042, 0x0063, 0x0063, 0x0063, 0x0063,
	0x0018, 0x001a, 0x0038, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063,
	0x002f, 0x0042, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063,
	0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063,
	0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063,
	0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063,
	0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063, 0x0063
};

static const u8 gau8Scale_factor[4] = {1, 2, 4, 8};
static const u8 gau8Scale_align_factor[4] = {1, 16, 32, 64};

#if SUPPORT_PROGRESSIVE
/* entry n is (-1 << n) + 1 */
static const __s32 extend_offset[16] = {
	0, ((-1) << 1) + 1, ((-1) << 2) + 1, ((-1) << 3) + 1, ((-1) << 4) + 1, ((-1) << 5) + 1,
	((-1) << 6) + 1, ((-1) << 7) + 1, ((-1) << 8) + 1, ((-1) << 9) + 1, ((-1) << 10) + 1,
	((-1) << 11) + 1, ((-1) << 12) + 1, ((-1) << 13) + 1, ((-1) << 14) + 1, ((-1) << 15) + 1
};

/* entry n is 2**(n-1) */
static const __s32 extend_test[16] = {
	0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000,
	    0x4000
};
#endif

//-------------------------------------------------------------------------------------------------
// Local Functions
//-------------------------------------------------------------------------------------------------
void *NJPD_MALLOC(u32 size)
{
	njpd_debug(LEVEL_API_DEBUG, "allocate mem size:%d", size);
	return STI_MALLOC(size);
}

void NJPD_FREE(void *x, u32 size)
{
	njpd_debug(LEVEL_API_DEBUG, "free mem size:%d", size);
	STI_FREE(x, size);
}

void NJPD_PrintMem(uintptr_t u32Addr, u32 u32Size)
{
	u32 u32i;

	njpd_debug(LEVEL_API_ALWAYS, "===========================================================");
	njpd_debug(LEVEL_API_ALWAYS, "print memory addr=0x%tx, size=0x%tx",
		   (ptrdiff_t) u32Addr, (ptrdiff_t) u32Size);
	njpd_debug(LEVEL_API_ALWAYS, "===========================================================");
	for (u32i = 0; u32i < u32Size / E_OFFSET8 + ((u32Size % E_OFFSET8) ? 1 : 0); u32i++) {
		njpd_debug(LEVEL_API_ALWAYS, "%02x %02x %02x %02x %02x %02x %02x %02x",
			*((u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM0)),
			*((u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM1)),
			*((u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM2)),
			*((u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM3)),
			*((u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM4)),
			*((u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM5)),
			*((u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM6)),
			*((u8 *)(u32Addr + u32i * E_OFFSET8 + E_NUM7))
		    );
	}
	njpd_debug(LEVEL_API_ALWAYS,
		   "===========================================================\n");

}

u32 NJPD_Align(u32 u32Value, u32 u32Factor)
{
	njpd_debug(LEVEL_API_DEBUG, "before align u32Value = %u, u32Factor = %u"
		, u32Value, u32Factor);
	if (u32Factor == 0)
		njpd_err("Factor = %u", u32Factor);
	else if ((u32Value % u32Factor) != 0)
		u32Value += (u32Factor - (u32Value % u32Factor));
	njpd_debug(LEVEL_API_DEBUG, "after align u32Value = %u, u32Factor = %u"
		, u32Value, u32Factor);
	return u32Value;
}

static u16 NJPD_ISR_GetEvents(void)
{
	int i;
	u16 u16EventFlag = 0;

	for (i = 0; i < ISR_EVENT_NUM; i++) {
		if (gNJPDContext.isrEvents[i])
			u16EventFlag |= (1 << i);
	}
	return u16EventFlag;
}

static void NJPD_ISR_StoreEvents(u16 u16EventFlag)
{
	int i;

	for (i = 0; i < ISR_EVENT_NUM; i++) {
		if (u16EventFlag & (1 << i))
			gNJPDContext.isrEvents[i] = true;
	}
}

static void NJPD_ISR_ClearEvents(u16 u16EventFlag)
{
	int i;

	for (i = 0; i < ISR_EVENT_NUM; i++) {
		if (u16EventFlag & (1 << i))
			gNJPDContext.isrEvents[i] = false;
	}
}

static bool NJPD_ISR_UnhandledEvent(void)
{
	int i;

	for (i = 0; i < ISR_EVENT_NUM; i++) {
		if (gNJPDContext.isrEvents[i] == true)
			return true;
	}
	return false;

}
static u16 MApi_NJPD_GetEventFlag(void)
{
	bool bEnableIsr = (gNJPDContext.jpd_irq >= 0);
	u16 u16EventFlag;

	if (bEnableIsr == true)
		u16EventFlag = NJPD_ISR_GetEvents();
	else
		u16EventFlag = MDrv_NJPD_GetEventFlag();
	return u16EventFlag;
}

static void MApi_NJPD_ClearEventFlag(u16 u16EventFlag)
{
	bool bEnableIsr = (gNJPDContext.jpd_irq >= 0);

	if (bEnableIsr == true)
		NJPD_ISR_ClearEvents(u16EventFlag);
	else
		MDrv_NJPD_SetEventFlag(u16EventFlag);
}

static int _MApi_NJPD_Parser_Get_Free_Handle(void)
{
	int i;

	for (i = 0; i < MAX_PARSER_COUNT; i++) {
		if (gNJPDParserInfo[i].active == false)
			return i;
	}
	return -1;
}

static bool _Mapi_NJPD_Parser_Skip_Bytes(int handle, u32 count)
{
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	if (gNJPDParserInfo[handle].data_left < count) {
		if (gNJPDParserInfo[handle].has_last_buffer == true &&
			gNJPDParserInfo[handle].last_buffer_used == false) {

			count -= gNJPDParserInfo[handle].data_left;
			gNJPDParserInfo[handle].last_buffer_used = true;
			gNJPDParserInfo[handle].read_point =
			    (u8 *)gNJPDParserInfo[handle].read_buffer->va;
			gNJPDParserInfo[handle].data_left =
			    gNJPDParserInfo[handle].read_buffer->filled_length;
			gNJPDParserInfo[handle].data_fill =
			    gNJPDParserInfo[handle].read_buffer->filled_length;
			_Mapi_NJPD_Parser_Skip_Bytes(handle,
				gNJPDParserInfo[handle].read_buffer->offset);
			if (gNJPDParserInfo[handle].data_left < count) {
				njpd_parser_err(handle, "skip byte fail, marker length error");
				return false;
			}
		} else {
			return false;
		}
	}

	gNJPDParserInfo[handle].data_left -= count;
	gNJPDParserInfo[handle].read_point += count;
	return true;
}

static u8 _Mapi_NJPD_Parser_Get_Char(int handle)
{
	u8 u8Char;
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return 0;
	}

	if (gNJPDParserInfo[handle].data_left == 0) {
		if (gNJPDParserInfo[handle].has_last_buffer == true &&
			gNJPDParserInfo[handle].last_buffer_used == false) {

			gNJPDParserInfo[handle].last_buffer_used = true;
			gNJPDParserInfo[handle].read_point =
			    (u8 *)gNJPDParserInfo[handle].read_buffer->va;
			gNJPDParserInfo[handle].data_left =
			    gNJPDParserInfo[handle].read_buffer->filled_length;
			gNJPDParserInfo[handle].data_fill =
			    gNJPDParserInfo[handle].read_buffer->filled_length;
			_Mapi_NJPD_Parser_Skip_Bytes(handle,
				gNJPDParserInfo[handle].read_buffer->offset);
		} else {
			gNJPDParserInfo[handle].need_more_data = true;
			return 0;
		}
	}
	u8Char = *gNJPDParserInfo[handle].read_point;
	gNJPDParserInfo[handle].read_point++;
	gNJPDParserInfo[handle].data_left--;

	return u8Char;
}

static u16 _Mapi_NJPD_Parser_Get_Marker_Length(int handle)
{
	u8 u8CharH;
	u8 u8CharL;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return 0;
	}
	u8CharH = _Mapi_NJPD_Parser_Get_Char(handle);
	u8CharL = _Mapi_NJPD_Parser_Get_Char(handle);

	if (gNJPDParserInfo[handle].need_more_data == true)
		return 0;

	return (u16) ((((u16) u8CharH) << E_NUM8) + u8CharL);
}

static void _Mapi_NJPD_Parser_Get_Aligned_Resolution(int handle, u16 *width, u16 *height)
{
	u8 u8Y_VSF;
	u8 u8Y_HSF;
	u32 u32Width = *width;
	u32 u32Height = *height;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return;
	}
	u8Y_VSF = gNJPDParserInfo[handle].sof_info.u8ComponentVSamp[0];
	u8Y_HSF = gNJPDParserInfo[handle].sof_info.u8ComponentHSamp[0];

	u32Width = NJPD_Align(u32Width, u8Y_HSF * NJPD_ALIGN_BASE);
	u32Height = NJPD_Align(u32Height, u8Y_VSF * NJPD_ALIGN_BASE);

	*width = (u16)u32Width;
	*height = (u16)u32Height;

	njpd_parser_debug(handle, LEVEL_API_DEBUG, "_u8Comp_v_samp = %d, _u8Comp_h_samp = %d",
			  u8Y_VSF, u8Y_HSF);
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "AlignWidth = %d, AlignHeight = %d", *width,
			  *height);
}

static bool _Mapi_NJPD_Parser_Get_Output_Resolution(int handle, u16 width, u16 height)
{
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_API_Pic *pstOutPic;
	NJPD_API_Pic *pstTmpPic;
	NJPD_API_Rect *pstRect;
	NJPD_API_Rect *pstROIRect;
	NJPD_API_DownScaleFactor eFactor = E_NJPD_API_DOWNSCALE_ORG;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstOutPic = &gNJPDParserInfo[handle].output_res;
	pstRect = &gNJPDParserInfo[handle].rect;
	pstROIRect = &gNJPDParserInfo[handle].roi_rect;

	_Mapi_NJPD_Parser_Get_Aligned_Resolution(handle, &width, &height);
	pstSOFInfo->u16ImageXSize = width;
	pstSOFInfo->u16ImageYSize = height;

	if (gNJPDParserInfo[handle].has_down_scale == true) {
		// bigger than pic
		pstTmpPic = &gNJPDParserInfo[handle].down_scale;

		for (eFactor = E_NJPD_API_DOWNSCALE_EIGHTH;
				eFactor >= E_NJPD_API_DOWNSCALE_ORG; eFactor--) {
			if (eFactor == E_NJPD_API_DOWNSCALE_ORG) {
				width = pstSOFInfo->u16OriginalImageXSize;
				height = pstSOFInfo->u16OriginalImageYSize;
				_Mapi_NJPD_Parser_Get_Aligned_Resolution(handle, &width, &height);
			} else {
				width = (pstSOFInfo->u16ImageXSize / gau8Scale_factor[eFactor]);
				height = (pstSOFInfo->u16ImageYSize / gau8Scale_factor[eFactor]);
			}
			if ((width >= pstTmpPic->width) && (height >= pstTmpPic->height))
				break;
		}
		if (eFactor < E_NJPD_API_DOWNSCALE_ORG) {
			//error, but decode original size
			njpd_parser_err(handle, "down scale (%u*%u) too big",
				pstTmpPic->width, pstTmpPic->height);
			eFactor = E_NJPD_API_DOWNSCALE_ORG;
		}
	} else if (gNJPDParserInfo[handle].has_max_res == true) {
		// smaller than pic
		pstTmpPic = &gNJPDParserInfo[handle].max_res;

		for (eFactor = E_NJPD_API_DOWNSCALE_ORG;
				eFactor < E_NJPD_API_DOWNSCALE_MAX; eFactor++) {
			if (eFactor == E_NJPD_API_DOWNSCALE_ORG) {
				width = pstSOFInfo->u16OriginalImageXSize;
				height = pstSOFInfo->u16OriginalImageYSize;
				_Mapi_NJPD_Parser_Get_Aligned_Resolution(handle, &width, &height);
			} else {
				width = (pstSOFInfo->u16ImageXSize / gau8Scale_factor[eFactor]);
				height = (pstSOFInfo->u16ImageYSize / gau8Scale_factor[eFactor]);
			}
			if ((width <= pstTmpPic->width) && (height <= pstTmpPic->height))
				break;
		}
		if (eFactor == E_NJPD_API_DOWNSCALE_MAX) {
			njpd_parser_err(handle, "max resolution (%u*%u) too small",
				pstTmpPic->width, pstTmpPic->height);
			return false;
		}
	}

	gNJPDParserInfo[handle].scale_factor = eFactor;
	if (eFactor == E_NJPD_API_DOWNSCALE_ORG) {
		pstOutPic->width = width;
		pstOutPic->height = height;
		pstRect->width = pstSOFInfo->u16OriginalImageXSize;
		pstRect->height = pstSOFInfo->u16OriginalImageYSize;
		pstRect->x = 0;
		pstRect->y = 0;
	} else {
		//scale 1/2, 1/4, 1/8, align 16, 32, 64
		//roi = origin->align by format ->align 16, 32, 64 and cut the rest
		//output = roi/scale
		pstROIRect->width = (pstSOFInfo->u16ImageXSize /
							gau8Scale_align_factor[eFactor]) *
							gau8Scale_align_factor[eFactor];
		pstROIRect->height = pstSOFInfo->u16ImageYSize;

		pstOutPic->width = pstROIRect->width / gau8Scale_factor[eFactor];
		pstOutPic->height = pstROIRect->height / gau8Scale_factor[eFactor];
		pstRect->width = pstROIRect->width / gau8Scale_factor[eFactor];
		pstRect->height = pstSOFInfo->u16OriginalImageYSize / gau8Scale_factor[eFactor];
		pstRect->x = 0;
		pstRect->y = 0;
	}

	njpd_parser_debug(handle, LEVEL_API_DEBUG, "output size width = %d, height = %d",
				pstOutPic->width, pstOutPic->height);
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "output rect width = %d, height = %d",
				pstRect->width, pstRect->height);
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "output roi rect width = %d, height = %d",
				pstROIRect->width, pstROIRect->height);
	return true;
}


static u8 _Mapi_NJPD_Find_Next_Markers(int handle)
{
	u8 u8Char;
	u32 u32GarbageBytes = 0;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return 0;
	}

	do {
		do {
			u32GarbageBytes++;
			u8Char = _Mapi_NJPD_Parser_Get_Char(handle);
			if (gNJPDParserInfo[handle].need_more_data == true)
				return 0;
		} while (u8Char != E_MARKER_FF);

		do {
			u8Char = _Mapi_NJPD_Parser_Get_Char(handle);
			if (gNJPDParserInfo[handle].need_more_data == true)
				return 0;
		} while (u8Char == E_MARKER_FF);
	} while (u8Char == E_MARKER_00);

	if (u32GarbageBytes > 1) {
		njpd_parser_debug(handle, LEVEL_API_ALWAYS,
				  "find %d garbage bytes before marker 0x%x", u32GarbageBytes,
				  u8Char);
	}

	return u8Char;
}

static bool _Mapi_NJPD_Read_Default_DHT(int handle)
{
	u16 i, index, count;
	u32 u32Left;
	u8 u8HuffNum[HUFF_NUM_SIZE];
	u8 u8HuffVal[HUFF_VAL_SIZE];
	u8 u8Valid[HUFF_NUM_SIZE];
	u32 u32DefaultDHTIndex = 0;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");

	u32Left =
	    (u16) ((cgDefault_DHT[u32DefaultDHTIndex] << E_NUM8) +
		   cgDefault_DHT[u32DefaultDHTIndex + E_NUM1]);
	u32DefaultDHTIndex += E_NUM2;

	if (u32Left < E_NUM2) {
		njpd_parser_err(handle, "bad default DHT marker ");
		return false;
	}

	u32Left -= E_NUM2;

	while (u32Left) {
		// set it to zero, initialize
		NJPD_memset((void *)u8HuffNum, 0, HUFF_NUM_SIZE);
		NJPD_memset((void *)u8Valid, 0, HUFF_NUM_SIZE);
		NJPD_memset((void *)u8HuffVal, 0, HUFF_VAL_SIZE);

		index = cgDefault_DHT[u32DefaultDHTIndex];
		u32DefaultDHTIndex++;

		u8HuffNum[0] = 0;

		count = 0;

		for (i = 1; i <= MAX_TABLE_SIZE; i++) {
			u8HuffNum[i] = cgDefault_DHT[u32DefaultDHTIndex];
			u32DefaultDHTIndex++;
			count += u8HuffNum[i];
		}

		if (count > HUFF_VAL_SIZE - 1) {
			njpd_parser_err(handle, "bad default DHT count:%d", count);
			return false;
		}

		for (i = 0; i < count; i++) {
			u8HuffVal[i] = cgDefault_DHT[u32DefaultDHTIndex];
			u32DefaultDHTIndex++;
		}

		i = 1 + MAX_TABLE_SIZE + count;

		if (u32Left < (u32) i) {
			njpd_parser_err(handle, "bad default DHT Left:%d, i:%d", u32Left, i);
			return false;
		}

		u32Left -= i;

		index = (index & E_MARKER_0F)
			+ ((index & E_MARKER_10) >> E_NUM4) * (NJPD_MAXHUFFTABLES >> E_NUM1);

		if (index >= NJPD_MAXHUFFTABLES) {
			njpd_parser_err(handle, "bad DHT index:%d", index);
			return false;
		}

		gNJPDParserInfo[handle].huff_info[index].u8SymbolCnt = count;

		if (gNJPDParserInfo[handle].huff_info[index].bValid == false)
			gNJPDParserInfo[handle].huff_info[index].bValid = true;

		NJPD_memcpy((void *)gNJPDParserInfo[handle].huff_info[index].u8Huff_num,
			    (void *)u8HuffNum,	HUFF_NUM_SIZE);
		NJPD_memcpy((void *)gNJPDParserInfo[handle].huff_info[index].u8Huff_val,
			    (void *)u8HuffVal,	HUFF_VAL_SIZE);

		for (i = 1; i <= MAX_TABLE_SIZE; i++) {
			if (u8HuffNum[HUFF_NUM_SIZE - i] != 0) {
				count = count - u8HuffNum[HUFF_NUM_SIZE - i];
				u8HuffNum[HUFF_NUM_SIZE - i] = count;
				u8Valid[HUFF_NUM_SIZE - i] = 1;
			} else {
				count = count - u8HuffNum[HUFF_NUM_SIZE - i];
				u8HuffNum[HUFF_NUM_SIZE - i] = count;
				u8Valid[HUFF_NUM_SIZE - i] = 0;
			}
		}

		NJPD_memcpy((void *)gNJPDParserInfo[handle].huff_info[index].u8Symbol,
			    (void *)u8HuffNum,	HUFF_NUM_SIZE);
		NJPD_memcpy((void *)gNJPDParserInfo[handle].huff_info[index].u8Valid,
			    (void *)u8Valid, HUFF_NUM_SIZE);
	}
	return true;
}


/* END OF EXIF PARSING SECTION */
//------------------------------------------------------------------------------
// Read a Huffman code table.
static bool _Mapi_NJPD_Read_DHT_Marker(int handle, u32 u32Left)
{
	u16 i, indextmp, count;
	u8 u8HuffNum[HUFF_NUM_SIZE];
	u8 u8HuffVal[HUFF_VAL_SIZE];
	u8 u8Valid[HUFF_NUM_SIZE];

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "u32Left:%d", u32Left);
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}

	if (u32Left < E_NUM2) {
		njpd_parser_err(handle, "bad DHT marker");
		return false;
	}
	u32Left -= E_NUM2;

	while (u32Left) {
		// set it to zero, initialize
		NJPD_memset((void *)u8HuffNum, 0, HUFF_NUM_SIZE);
		NJPD_memset((void *)u8Valid, 0, HUFF_NUM_SIZE);
		NJPD_memset((void *)u8HuffVal, 0, HUFF_VAL_SIZE);

		indextmp = _Mapi_NJPD_Parser_Get_Char(handle);
		u8HuffNum[0] = 0;
		count = 0;

		for (i = 1; i <= MAX_TABLE_SIZE; i++) {
			u8HuffNum[i] = _Mapi_NJPD_Parser_Get_Char(handle);
			count += u8HuffNum[i];
		}

		if (count > HUFF_VAL_SIZE - 1) {
			njpd_parser_err(handle, "bad DHT count:%d", count);
			return false;
		}

		for (i = 0; i < count; i++)
			u8HuffVal[i] = _Mapi_NJPD_Parser_Get_Char(handle);

		i = 1 + MAX_TABLE_SIZE + count;

		if (u32Left < (u32) i) {
			njpd_parser_err(handle, "bad DHT, left:%d, should bigger than %d", u32Left,
					i);
			return false;
		}

		u32Left -= i;

		indextmp = (indextmp & E_MARKER_0F)
			+ ((indextmp & E_MARKER_10) >> E_NUM4) * (NJPD_MAXHUFFTABLES >> E_NUM1);

		if (indextmp >= NJPD_MAXHUFFTABLES) {
			njpd_parser_err(handle, "bad DHT index:%d", indextmp);
			return false;
		}

		gNJPDParserInfo[handle].huff_info[indextmp].u8SymbolCnt = count;

		if (gNJPDParserInfo[handle].huff_info[indextmp].bValid == false)
			gNJPDParserInfo[handle].huff_info[indextmp].bValid = true;
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "DHT index : %d", indextmp);

		NJPD_memcpy((void *)gNJPDParserInfo[handle].huff_info[indextmp].u8Huff_num,
			    (void *)u8HuffNum, HUFF_NUM_SIZE);
		NJPD_memcpy((void *)gNJPDParserInfo[handle].huff_info[indextmp].u8Huff_val,
			    (void *)u8HuffVal, HUFF_VAL_SIZE);

		for (i = 1; i <= MAX_TABLE_SIZE; i++) {
			if (u8HuffNum[HUFF_NUM_SIZE - i] != 0) {
				count = count - u8HuffNum[HUFF_NUM_SIZE - i];
				u8HuffNum[HUFF_NUM_SIZE - i] = count;
				u8Valid[HUFF_NUM_SIZE - i] = 1;
			} else {
				count = count - u8HuffNum[HUFF_NUM_SIZE - i];
				u8HuffNum[HUFF_NUM_SIZE - i] = count;
				u8Valid[HUFF_NUM_SIZE - i] = 0;
			}
		}

		NJPD_memcpy((void *)gNJPDParserInfo[handle].huff_info[indextmp].u8Symbol,
			    (void *)u8HuffNum, HUFF_NUM_SIZE);
		NJPD_memcpy((void *)gNJPDParserInfo[handle].huff_info[indextmp].u8Valid,
			    (void *)u8Valid, HUFF_NUM_SIZE);
	}
	return true;
}

//------------------------------------------------------------------------------
// Read a quantization table.
static u8 _Mapi_NJPD_Read_DQT_Marker(int handle, u32 u32Left)
{
	u16 n, i, prec;
	u32 u32Temp;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "u32Left:%d", u32Left);
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}

	if (u32Left < E_NUM2) {
		njpd_parser_err(handle, "bad DQT marker");
		return false;
	}
	u32Left -= E_NUM2;

	while (u32Left) {
		n = _Mapi_NJPD_Parser_Get_Char(handle);
		prec = n >> E_NUM4;
		n &= E_MARKER_0F;

		if (n >= NJPD_MAXQUANTTABLES) {
			njpd_parser_err(handle, "bad DQT table, n:%d ", n);
			return false;
		}

		if (gNJPDParserInfo[handle].quant_table[n].bValid == false)
			gNJPDParserInfo[handle].quant_table[n].bValid = true;

		// read quantization entries, in zag order
		for (i = 0; i < E_OFFSET64; i++) {
			u32Temp = _Mapi_NJPD_Parser_Get_Char(handle);

			if (prec)
				u32Temp = (u32Temp << E_NUM8) + _Mapi_NJPD_Parser_Get_Char(handle);

			gNJPDParserInfo[handle].quant_table[n].s16Value[i] = u32Temp;
		}

		i = E_OFFSET64 + 1;

		if (prec)
			i += E_OFFSET64;

		if (u32Left < (u32) i) {
			njpd_parser_err(handle, "bad DQT table, left:%d should bigger than %d",
					u32Left, i);
			return false;
		}

		u32Left -= i;
	}
	return true;
}

//------------------------------------------------------------------------------
// Read the start of frame (SOF) marker.
static u8 _Mapi_NJPD_Read_SOF_Marker(int handle, u32 u32Left)
{
	u8 i;
	u8 c1;
	NJPD_SOFInfo *pstSOFInfo;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "u32Left:%d", u32Left);
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;

	if (_Mapi_NJPD_Parser_Get_Char(handle) != E_NUM8) {
		njpd_parser_err(handle, "bad precision");
		return false;
	}

	pstSOFInfo->u16OriginalImageYSize =
		(u16) ((_Mapi_NJPD_Parser_Get_Char(handle) << E_NUM8) +
		_Mapi_NJPD_Parser_Get_Char(handle));
	pstSOFInfo->u16OriginalImageXSize =
		(u16) ((_Mapi_NJPD_Parser_Get_Char(handle) << E_NUM8) +
		_Mapi_NJPD_Parser_Get_Char(handle));
	// save the original image size

	pstSOFInfo->u8ComponentsInFrame = _Mapi_NJPD_Parser_Get_Char(handle);

	if (pstSOFInfo->u8ComponentsInFrame > NJPD_MAXCOMPONENTS) {
		njpd_parser_err(handle, "too many components:%d", pstSOFInfo->u8ComponentsInFrame);
		return false;
	}

	if (u32Left != (u32) (pstSOFInfo->u8ComponentsInFrame * E_OFFSET3 + E_NUM8)) {
		njpd_parser_err(handle, "bad length:%d, should be:%d", u32Left,
				(pstSOFInfo->u8ComponentsInFrame * E_OFFSET3 + E_NUM8));
		return false;
	}

	pstSOFInfo->u8LumaComponentID = E_COM1;
	pstSOFInfo->u8ChromaComponentID = E_COM2;
	pstSOFInfo->u8Chroma2ComponentID = E_COM3;

	for (i = 0; i < pstSOFInfo->u8ComponentsInFrame; i++) {
		pstSOFInfo->u8ComponentIdent[i] = _Mapi_NJPD_Parser_Get_Char(handle);
		if (pstSOFInfo->u8ComponentIdent[i] == 0) {
			pstSOFInfo->u8LumaComponentID = E_COM0;
			pstSOFInfo->u8ChromaComponentID = E_COM1;
			pstSOFInfo->u8Chroma2ComponentID = E_COM2;
		}

		c1 = _Mapi_NJPD_Parser_Get_Char(handle);

		pstSOFInfo->u8ComponentHSamp[i] = (c1 >> E_NUM4) & E_MARKER_0F;
		pstSOFInfo->u8ComponentVSamp[i] = (c1 & E_MARKER_0F);
		pstSOFInfo->u8ComponentQuant[i] = _Mapi_NJPD_Parser_Get_Char(handle);

		// only has one component, but its sampling factor is 1x2
		// Per the JPEG spec A.2.2
		// (see the attached file, "regardless of the values of H1 and V1"),
		// please always set H=1 & V=1 to hw, when mono image.
		if (pstSOFInfo->u8ComponentsInFrame == 1) {
			pstSOFInfo->u8ComponentHSamp[0] = 1;
			pstSOFInfo->u8ComponentVSamp[0] = 1;
		}

	}

	if (_Mapi_NJPD_Parser_Get_Output_Resolution(handle, pstSOFInfo->u16OriginalImageXSize,
						pstSOFInfo->u16OriginalImageYSize) == false) {
		njpd_parser_err(handle, "get output resolotuin failed");
		return false;
	}

	if (gNJPDParserInfo[handle].output_res.height < 1) {
		njpd_parser_err(handle, "output height = %d",
				gNJPDParserInfo[handle].output_res.height);
		return false;
	}

	if (gNJPDParserInfo[handle].output_res.width < 1) {
		njpd_parser_err(handle, "output width = %d",
				gNJPDParserInfo[handle].output_res.width);
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
// Used to skip unrecognized markers.
static u8 _Mapi_NJPD_Skip_Variable_Marker(int handle, u32 u32Left)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "u32Left:%d", u32Left);
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}

	if (u32Left < E_NUM2) {
		njpd_parser_err(handle, "bad variable marker Left:%d", u32Left);
		return false;
	}

	u32Left -= E_NUM2;

	if (_Mapi_NJPD_Parser_Skip_Bytes(handle, u32Left) == false) {
		njpd_parser_err(handle, "skip byte failed");
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
// Read a define restart interval (DRI) marker.
static u8 _Mapi_NJPD_Read_DRI_Marker(int handle, u32 u32Left)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "u32Left:%d", u32Left);
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	if (u32Left != E_NUM4) {
		njpd_parser_err(handle, "bad DRI marker");
		return false;
	}

	gNJPDParserInfo[handle].table_info.u16RestartInterval =
	    (u16) ((_Mapi_NJPD_Parser_Get_Char(handle) << E_NUM8)
	    + _Mapi_NJPD_Parser_Get_Char(handle));
	return true;
}

//------------------------------------------------------------------------------
// Read a start of scan (SOS) marker.
static u8 _Mapi_NJPD_Read_SOS_Marker(int handle, u32 u32Left)
{
	u16 i, ci, n, c, cc;
	u8 c1;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_SOSInfo *pstSOSInfo;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "u32Left:%d", u32Left);
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;

	n = _Mapi_NJPD_Parser_Get_Char(handle);

	pstSOSInfo->u8ComponentsInScan = n;

	u32Left -= E_NUM3;

	if ((u32Left != (u32) (n * E_NUM2 + E_NUM3)) || (n < 1) || (n > NJPD_MAXCOMPSINSCAN)) {
		njpd_parser_err(handle, "bad SOS marker Left:%d n:%d", u32Left, n);
		return false;
	}

	for (i = 0; i < n; i++) {
		cc = _Mapi_NJPD_Parser_Get_Char(handle);
		c = _Mapi_NJPD_Parser_Get_Char(handle);
		u32Left -= E_NUM2;

		for (ci = 0; ci < pstSOFInfo->u8ComponentsInFrame; ci++) {
			if (cc == pstSOFInfo->u8ComponentIdent[ci])
				break;
		}

		if (ci >= pstSOFInfo->u8ComponentsInFrame) {
			njpd_parser_err(handle, "bad SOS component:%d ci:%d",
					pstSOFInfo->u8ComponentsInFrame, ci);
			return false;
		}

		pstSOSInfo->u8ComponentList[i] = ci;
		pstSOSInfo->u8ComponentDCTable[ci] = (c >> E_NUM4) & E_MARKER_0F;
		pstSOSInfo->u8ComponentACTable[ci] =
			(c & E_MARKER_0F) + (NJPD_MAXHUFFTABLES >> E_NUM1);
	}

	//HW limitation, for baseline JPEG, U.V need to refer to the same DC and AC huffman table.
	if (!(gNJPDParserInfo[handle].is_progressive)
		&& (pstSOFInfo->u8ComponentsInFrame == E_COM3)) {
		if ((pstSOSInfo->u8ComponentDCTable[E_COM1]
			!= pstSOSInfo->u8ComponentDCTable[E_COM2])
			|| (pstSOSInfo->u8ComponentACTable[E_COM1]
				!= pstSOSInfo->u8ComponentACTable[E_COM2])) {
			gNJPDParserInfo[handle].table_info.bIs3HuffTbl = true;
		}
	}

	pstSOSInfo->u8SpectralStart = _Mapi_NJPD_Parser_Get_Char(handle);
	pstSOSInfo->u8SpectralEnd = _Mapi_NJPD_Parser_Get_Char(handle);
	c1 = _Mapi_NJPD_Parser_Get_Char(handle);
	pstSOSInfo->u8SuccessiveHigh = (c1 & E_MARKER_F0) >> E_NUM4;
	pstSOSInfo->u8SuccessiveLow = (c1 & E_MARKER_0F);

	if (!(gNJPDParserInfo[handle].is_progressive)) {
		pstSOSInfo->u8SpectralStart = 0;
		pstSOSInfo->u8SpectralEnd = E_OFFSET64 - 1;
	}

	u32Left -= E_NUM3;

	if (_Mapi_NJPD_Parser_Skip_Bytes(handle, u32Left) == false) {
		njpd_parser_err(handle, "skip byte failed");
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
// Creates the tables needed for efficient Huffman decoding.
static u8 _Mapi_NJPD_Make_Hufftable(int handle, u8 indextmp)
{
	u16 p, i, l, si;
	u8 *huffsize;
	u16 *huffcode;
	u16 _code;
#if SUPPORT_PROGRESSIVE
	u16 lastp;
	u16 subtree;
	u16 code_size;
	s16 nextfreeentry;
	s16 currententry;
	NJPD_HuffTbl *hs;
	NJPD_ProgInfo *pstProgInfo;
#endif
	njpd_parser_debug(handle, LEVEL_API_TRACE, "make huff table : %d", indextmp);

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}

	huffsize = NJPD_MALLOC(NJPD_HUFFSIZE * sizeof(u8));
	huffcode = NJPD_MALLOC(NJPD_HUFFSIZE * sizeof(u16));

	if (huffsize == NULL || huffcode == NULL) {
		if (huffsize)
			NJPD_FREE(huffsize, NJPD_HUFFSIZE * sizeof(u8));

		if (huffcode)
			NJPD_FREE(huffcode, NJPD_HUFFSIZE * sizeof(u16));

		njpd_parser_err(handle, "malloc fail!!!");
		return false;
	}
	memset(huffsize, 0, NJPD_HUFFSIZE * sizeof(u8));
	memset(huffcode, 0, NJPD_HUFFSIZE * sizeof(u16));

	p = 0;

	for (l = 1; l <= E_NUM16; l++) {
		for (i = 1; i <= gNJPDParserInfo[handle].huff_info[indextmp].u8Huff_num[l]; i++) {
			huffsize[p++] = l;

			if (p >= NJPD_HUFFSIZE) {
				njpd_parser_err(handle, "invalid Huff table");
				NJPD_FREE(huffsize, NJPD_HUFFSIZE * sizeof(u8));
				NJPD_FREE(huffcode, NJPD_HUFFSIZE * sizeof(u16));
				return false;
			}
		}
	}

	huffsize[p] = 0;
#if SUPPORT_PROGRESSIVE
	lastp = p;
#endif
	_code = 0;
	si = huffsize[0];
	p = 0;

	while (huffsize[p]) {
		while (huffsize[p] == si) {
			huffcode[p++] = _code;
			_code++;

			//add protection
			if (p >= NJPD_HUFFSIZE) {
				njpd_parser_err(handle, "invalid Huff table case 2");
				NJPD_FREE(huffsize, NJPD_HUFFSIZE * sizeof(u8));
				NJPD_FREE(huffcode, NJPD_HUFFSIZE * sizeof(u16));
				return false;
			}
		}

		_code <<= 1;
		si++;
	}

	// Calculate the min code
	for (i = 1; i <= E_NUM16; i++) {
		gNJPDParserInfo[handle].huff_info[indextmp].u16Code[i] =
		    huffcode[gNJPDParserInfo[handle].huff_info[indextmp].u8Symbol[i]];
	}
#if SUPPORT_PROGRESSIVE
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;
	// SW doesn't need huff table when baseline decoding
	if (gNJPDParserInfo[handle].is_progressive) {
		hs = &(pstProgInfo->_Huff_tbls[indextmp]);
		NJPD_memset((void *)(hs->s16Look_up), 0, sizeof(hs->s16Look_up));
		NJPD_memset((void *)(hs->s16Tree), 0, sizeof(hs->s16Tree));
		NJPD_memset((void *)(hs->u8Code_size), 0, sizeof(hs->u8Code_size));

		nextfreeentry = -E_NUM1;

		p = 0;

		while (p < lastp) {
			i = gNJPDParserInfo[handle].huff_info[indextmp].u8Huff_val[p];
			_code = huffcode[p];
			code_size = huffsize[p];

			hs->u8Code_size[i] = code_size;

			if (code_size <= E_NUM8) {
				_code <<= (E_NUM8 - code_size);

				for (l = 1 << (E_NUM8 - code_size); l > 0; l--) {
					hs->s16Look_up[_code] = i;
					_code++;
				}
			} else {
				subtree = (_code >> (code_size - E_NUM8)) & E_MARKER_FF;

				currententry = hs->s16Look_up[subtree];

				if (currententry == 0) {
					hs->s16Look_up[subtree] = currententry = nextfreeentry;

					nextfreeentry -= E_NUM2;
				}

				_code <<= (E_NUM16 - (code_size - E_NUM8));

				for (l = code_size; l > E_NUM9; l--) {
					if ((_code & E_MARKER_8000) == 0)
						currententry--;

					if (hs->s16Tree[-currententry - E_NUM1] == 0) {
						hs->s16Tree[-currententry - E_NUM1] = nextfreeentry;

						currententry = nextfreeentry;

						nextfreeentry -= E_NUM2;
					} else {
						currententry = hs->s16Tree[-currententry - E_NUM1];
					}

					_code <<= E_NUM1;
				}

				if ((_code & E_MARKER_8000) == 0)
					currententry--;

				hs->s16Tree[-currententry - 1] = i;
			}
			p++;
		}
	}
#endif

	NJPD_FREE(huffsize, NJPD_HUFFSIZE * sizeof(u8));
	NJPD_FREE(huffcode, NJPD_HUFFSIZE * sizeof(u16));
	return true;
}

//------------------------------------------------------------------------------
// Verifies the quantization tables needed for this scan are available.
static u8 _Mapi_NJPD_Check_Quant_Tables(int handle)
{
	u8 i;
	u8 comp;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_SOSInfo *pstSOSInfo;
	NJPD_QuantTbl *pstQuantTable;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;
	pstQuantTable = gNJPDParserInfo[handle].quant_table;

	for (i = 0; i < pstSOSInfo->u8ComponentsInScan; i++) {
		comp = pstSOFInfo->u8ComponentQuant[pstSOSInfo->u8ComponentList[i]];
		if (pstQuantTable[comp].bValid == false) {
			njpd_parser_err(handle, "undefined quant table");
			return false;
		}
	}
	return true;
}

//------------------------------------------------------------------------------
// Verifies that all the Huffman tables needed for this scan are available.
static u8 _Mapi_NJPD_Check_Huff_Tables(int handle)
{
	u8 i;
	u8 comp1, comp2;
	NJPD_HuffInfo *pstHuffInfo;
	NJPD_SOSInfo *pstSOSInfo;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstHuffInfo = gNJPDParserInfo[handle].huff_info;
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;

	for (i = 0; i < pstSOSInfo->u8ComponentsInScan; i++) {
		comp1 = pstSOSInfo->u8ComponentDCTable[pstSOSInfo->u8ComponentList[i]];
		if ((pstSOSInfo->u8SpectralStart == 0)
		    && (pstHuffInfo[comp1].bValid == false)) {
			njpd_parser_err(handle, "comp1 undefined huff table");
			return false;
		}

		comp2 = pstSOSInfo->u8ComponentACTable[pstSOSInfo->u8ComponentList[i]];
		if ((pstSOSInfo->u8SpectralEnd > 0)
		    && (pstHuffInfo[comp2].bValid == false)) {
			njpd_parser_err(handle, "comp2 undefined huff table");
			return false;
		}
	}

	for (i = 0; i < NJPD_MAXHUFFTABLES; i++) {
		if (pstHuffInfo[i].bValid) {
			if (!_Mapi_NJPD_Make_Hufftable(handle, i))
				return false;
		}
	}
	return true;
}

static bool _Mapi_NJPD_Parser_Locate_SOI_Marker(int handle)
{
	u8 u8LastChar, u8ThisChar;
	u32 u32BytesLeft;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}

	u8LastChar = _Mapi_NJPD_Parser_Get_Char(handle);
	u8ThisChar = _Mapi_NJPD_Parser_Get_Char(handle);
	u32BytesLeft = BYTES_LEFT_MAX_SIZE;

	for (;;) {
		if ((u8LastChar == E_MARKER_FF) && (u8ThisChar == E_NJPD_MARKER_SOI))
			break;

		if (--u32BytesLeft == 0) {
			njpd_parser_err(handle, "couldn't find SOI, not jpeg format");
			return false;
		}

		u8LastChar = u8ThisChar;
		u8ThisChar = _Mapi_NJPD_Parser_Get_Char(handle);
	}

	njpd_parser_debug(handle, LEVEL_API_TRACE, "find SOI");

	return true;
}

static NJPD_API_ParserResult _Mapi_NJPD_Parser_Process_Markers(int handle)
{
	u8 u8Char;
	u32 u32MarkerLength;
	u32 u32BufferRemain;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_HEADER_INVALID;
	}

	for (;;) {
		gNJPDParserInfo[handle].last_marker_offset =
		    gNJPDParserInfo[handle].data_fill - gNJPDParserInfo[handle].data_left;
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "last_marker_offset = 0x%zx",
			gNJPDParserInfo[handle].last_marker_offset);
		u8Char = _Mapi_NJPD_Find_Next_Markers(handle);
		if (gNJPDParserInfo[handle].need_more_data == true) {
			njpd_parser_debug(handle, LEVEL_API_ALWAYS,
					  "need more data(find next length)");
			gNJPDParserInfo[handle].need_more_data = false;
			return E_NJPD_API_HEADER_PARTIAL;
		}
		if (u8Char == E_NJPD_MARKER_EOI) {
			njpd_parser_debug(handle, LEVEL_API_DEBUG, "find eoi marker 0x%x", u8Char);
			return E_NJPD_API_HEADER_END;
		}
		u32MarkerLength = _Mapi_NJPD_Parser_Get_Marker_Length(handle);
		if (gNJPDParserInfo[handle].has_last_buffer == false ||
			gNJPDParserInfo[handle].last_buffer_used == true) {
			u32BufferRemain = gNJPDParserInfo[handle].data_left;
		} else {
			u32BufferRemain =
			    gNJPDParserInfo[handle].data_left +
			    (gNJPDParserInfo[handle].read_buffer->filled_length -
			    gNJPDParserInfo[handle].read_buffer->offset);
		}
		if ((gNJPDParserInfo[handle].need_more_data == true)
		    || (u32MarkerLength > u32BufferRemain)) {
			njpd_parser_debug(handle, LEVEL_API_ALWAYS,
			"need more data(get marker length),length = 0x%x, data left = 0x%x",
				u32MarkerLength, u32BufferRemain);
			gNJPDParserInfo[handle].need_more_data = false;
			return E_NJPD_API_HEADER_PARTIAL;
		}

		switch (u8Char) {
		case E_NJPD_MARKER_APP1:	// for mpo exif
		case E_NJPD_MARKER_APP2:	//mpo
			if (!_Mapi_NJPD_Skip_Variable_Marker(handle, u32MarkerLength)) {
				njpd_parser_err(handle, "skip marker 0x%x failed", u8Char);
				return E_NJPD_API_HEADER_INVALID;
			}
			break;
		case E_NJPD_MARKER_SOF0:	// baseline
		case E_NJPD_MARKER_SOF1:	// extended sequential DCT
			gNJPDParserInfo[handle].is_progressive = false;
			if (_Mapi_NJPD_Read_SOF_Marker(handle, u32MarkerLength) == false) {
				njpd_parser_err(handle, "read SOF failed");
				return E_NJPD_API_HEADER_INVALID;
			}
			break;
		case E_NJPD_MARKER_SOF2:	// progressive
			gNJPDParserInfo[handle].is_progressive = true;
#if SUPPORT_PROGRESSIVE
			if (_Mapi_NJPD_Read_SOF_Marker(handle, u32MarkerLength) == false) {
				njpd_parser_err(handle, "read progressive SOF failed");
				return E_NJPD_API_HEADER_INVALID;
			}
			break;
#else
			njpd_parser_err(handle, "find SOF2, not support progressive");
			return E_NJPD_API_HEADER_INVALID;
#endif
		case E_NJPD_MARKER_SOF3:
		case E_NJPD_MARKER_SOF5:
		case E_NJPD_MARKER_SOF6:
		case E_NJPD_MARKER_SOF7:
		case E_NJPD_MARKER_SOF9:
		case E_NJPD_MARKER_SOF10:
		case E_NJPD_MARKER_SOF11:
		case E_NJPD_MARKER_SOF13:
		case E_NJPD_MARKER_SOF14:
		case E_NJPD_MARKER_SOF15:
		case E_NJPD_MARKER_SOI:
			njpd_parser_err(handle, "find unsupport marker 0x%x", u8Char);
			return E_NJPD_API_HEADER_INVALID;
		case E_NJPD_MARKER_SOS:
			if (_Mapi_NJPD_Read_SOS_Marker(handle, u32MarkerLength) == false) {
				njpd_parser_err(handle, "read SOS failed");
				return E_NJPD_API_HEADER_INVALID;
			}

			if (!gNJPDParserInfo[handle].is_progressive) {
				//baseline need to set offset to end of SOS,
				//progressive need to keep offset before SOS
				//in order to locate followind data
				gNJPDParserInfo[handle].last_marker_offset =
				    gNJPDParserInfo[handle].data_fill -
				    gNJPDParserInfo[handle].data_left;
			}
			gNJPDParserInfo[handle].check_SOS = true;
			return E_NJPD_API_HEADER_VALID;
		case E_NJPD_MARKER_DHT:
			if (_Mapi_NJPD_Read_DHT_Marker(handle, u32MarkerLength) == false) {
				njpd_parser_err(handle, "read DHT failed");
				return E_NJPD_API_HEADER_INVALID;
			}
			gNJPDParserInfo[handle].check_DHT = true;
			break;
			// Sorry, no arithmitic support at this time. Dumb patents!
		case E_NJPD_MARKER_DAC:
			njpd_parser_err(handle, "find unsupport marker 0x%x", u8Char);
			return E_NJPD_API_HEADER_INVALID;
		case E_NJPD_MARKER_DQT:
			if (_Mapi_NJPD_Read_DQT_Marker(handle, u32MarkerLength) == false) {
				njpd_parser_err(handle, "read DQT failed");
				return E_NJPD_API_HEADER_INVALID;
			}
			gNJPDParserInfo[handle].check_DQT = true;
			break;
		case E_NJPD_MARKER_DRI:
			if (_Mapi_NJPD_Read_DRI_Marker(handle, u32MarkerLength) == false) {
				njpd_parser_err(handle, "read DRI failed");
				return E_NJPD_API_HEADER_INVALID;
			}
			break;
		case E_NJPD_MARKER_JPG:
		case E_NJPD_MARKER_RST0:
			/* no parameters */
		case E_NJPD_MARKER_RST1:
		case E_NJPD_MARKER_RST2:
		case E_NJPD_MARKER_RST3:
		case E_NJPD_MARKER_RST4:
		case E_NJPD_MARKER_RST5:
		case E_NJPD_MARKER_RST6:
		case E_NJPD_MARKER_RST7:
		case E_NJPD_MARKER_TEM:
			njpd_parser_err(handle, "find unsupport marker 0x%x", u8Char);
			return E_NJPD_API_HEADER_INVALID;
		case E_NJPD_MARKER_APP0:
		case E_NJPD_MARKER_DNL:
		case E_NJPD_MARKER_DHP:
		case E_NJPD_MARKER_EXP:
		case E_NJPD_MARKER_APP3:
		case E_NJPD_MARKER_APP4:
		case E_NJPD_MARKER_APP5:
		case E_NJPD_MARKER_APP6:
		case E_NJPD_MARKER_APP7:
		case E_NJPD_MARKER_APP8:
		case E_NJPD_MARKER_APP9:
		case E_NJPD_MARKER_APP10:
		case E_NJPD_MARKER_APP11:
		case E_NJPD_MARKER_APP12:
		case E_NJPD_MARKER_APP13:
		case E_NJPD_MARKER_APP14:
		case E_NJPD_MARKER_APP15:
		case E_NJPD_MARKER_JPG0:
		case E_NJPD_MARKER_JPG1:
		case E_NJPD_MARKER_JPG2:
		case E_NJPD_MARKER_JPG3:
		case E_NJPD_MARKER_JPG4:
		case E_NJPD_MARKER_JPG5:
		case E_NJPD_MARKER_JPG6:
		case E_NJPD_MARKER_JPG7:
		case E_NJPD_MARKER_JPG8:
		case E_NJPD_MARKER_JPG9:
		case E_NJPD_MARKER_JPG10:
		case E_NJPD_MARKER_JPG11:
		case E_NJPD_MARKER_JPG12:
		case E_NJPD_MARKER_JPG13:
		case E_NJPD_MARKER_COM:
			/* must be DNL, DHP, EXP, APPn, JPGn, COM, or RESn or APP0 */
			if (!_Mapi_NJPD_Skip_Variable_Marker(handle, u32MarkerLength)) {
				njpd_parser_err(handle, "skip marker 0x%x failed", u8Char);
				return E_NJPD_API_HEADER_INVALID;
			}
			break;
		default:
			break;
		}
	}
	return E_NJPD_API_HEADER_VALID;
}

#if SUPPORT_PROGRESSIVE
//------------------------------------------------------------------------------
// Logical rotate left operation.
static u32 _Mapi_NJPD_Rol(u32 i, u8 j)
{
	return ((i << j) | (i >> (E_NUM32 - j)));
}
//------------------------------------------------------------------------------
// The coeff_buf series of methods originally stored the coefficients
// into a "virtual" file which was located in EMS, XMS, or a disk file. A cache
// was used to make this process more efficient. Now, we can store the entire
// thing in RAM.
static PJPEG_CoeffBuf _Mapi_NJPD_Coeff_Buf_Open(u16 block_num_x, u16 block_num_y,
					       u8 block_len_x, u8 block_len_y)
{
	PJPEG_CoeffBuf cb = (PJPEG_CoeffBuf) NJPD_MALLOC(sizeof(JPEG_CoeffBuf));
	size_t size;

	if (cb == NULL)
		return NULL;

	memset(cb, 0, sizeof(JPEG_CoeffBuf));

	cb->u16Block_num_x = block_num_x;
	cb->u16Block_num_y = block_num_y;

	cb->u8Block_len_x = block_len_x;
	cb->u8Block_len_y = block_len_y;

	cb->u16Block_size = (block_len_x * block_len_y) * sizeof(NJPD_BLOCK_TYPE);

	size = cb->u16Block_size;
	size = size * block_num_x * block_num_y;
	cb->pu8Data = (u8 *) NJPD_MALLOC(size);

	if (cb->pu8Data == NULL) {
		njpd_err("pu8Data malloc fail");
		NJPD_FREE(cb, sizeof(JPEG_CoeffBuf));
		return NULL;
	}

	memset(cb->pu8Data, 0, size);

	return cb;
}

static void _Mapi_NJPD_Coeff_Buf_Free(PJPEG_CoeffBuf cb, u16 block_num_x, u16 block_num_y,
					       u8 block_len_x, u8 block_len_y)
{
	if (cb == NULL)
		return;

	cb->u16Block_num_x = block_num_x;
	cb->u16Block_num_y = block_num_y;

	cb->u8Block_len_x = block_len_x;
	cb->u8Block_len_y = block_len_y;

	cb->u16Block_size = (block_len_x * block_len_y) * sizeof(NJPD_BLOCK_TYPE);

	if (cb->pu8Data != NULL)
		NJPD_FREE(cb->pu8Data, cb->u16Block_size * block_num_x * block_num_y);

	NJPD_FREE(cb, sizeof(JPEG_CoeffBuf));
}

//------------------------------------------------------------------------------
static NJPD_BLOCK_TYPE *_Mapi_NJPD_Coeff_Buf_Getp(PJPEG_CoeffBuf cb, u16 block_x, u16 block_y)
{
	if (block_x >= cb->u16Block_num_x) {
		njpd_err("line: %d", __LINE__);
		return NULL;
	}

	if (block_y >= cb->u16Block_num_y) {
		njpd_err("line: %d", __LINE__);
		return NULL;
	}

	return (NJPD_BLOCK_TYPE *) ((uintptr_t) (cb->pu8Data + block_x * cb->u16Block_size
					     + block_y * (cb->u16Block_size * cb->u16Block_num_x)));
}

static u8 _Mapi_NJPD_Parser_Get_CharP(int handle, u8 *Ppadding_flag)
{
	u8 u8Char;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	if (gNJPDParserInfo[handle].data_left == 0) {
		if (gNJPDParserInfo[handle].has_last_buffer == true &&
			gNJPDParserInfo[handle].last_buffer_used == false) {

			gNJPDParserInfo[handle].last_buffer_used = true;
			gNJPDParserInfo[handle].read_point =
			    (u8 *)gNJPDParserInfo[handle].read_buffer->va;
			gNJPDParserInfo[handle].data_left =
			    gNJPDParserInfo[handle].read_buffer->filled_length;
			gNJPDParserInfo[handle].data_fill =
			    gNJPDParserInfo[handle].read_buffer->filled_length;
			_Mapi_NJPD_Parser_Skip_Bytes(handle,
				gNJPDParserInfo[handle].read_buffer->offset);
		} else {
			*Ppadding_flag = true;
			gNJPDParserInfo[handle].need_more_data = true;
			return 0;
		}
	}

	*Ppadding_flag = false;

	u8Char = *gNJPDParserInfo[handle].read_point;
	gNJPDParserInfo[handle].read_point++;
	gNJPDParserInfo[handle].data_left--;
	return u8Char;
}

//------------------------------------------------------------------------------
// Inserts a previously retrieved character back into the input buffer.
static void _Mapi_NJPD_Parser_Stuff_Char(int handle, u8 q)
{
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return;
	}
	if ((gNJPDParserInfo[handle].data_left + 1) > gNJPDParserInfo[handle].data_fill) {
		njpd_parser_err(handle, "stuff out of start point");
		return;
	}
	gNJPDParserInfo[handle].read_point--;
	*gNJPDParserInfo[handle].read_point = q;
	gNJPDParserInfo[handle].data_left++;
}

//------------------------------------------------------------------------------
// Retrieves one character from the input stream, but does
// not read past markers. Will continue to return 0xFF when a
// marker is encountered.
// FIXME: Bad name?
static u8 _Mapi_NJPD_Parser_Get_Octet(int handle)
{
	u8 padding_flag;
	u8 c = _Mapi_NJPD_Parser_Get_CharP(handle, &padding_flag);

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	if (c == E_MARKER_FF) {
		if (padding_flag)
			return E_MARKER_FF;

		c = _Mapi_NJPD_Parser_Get_CharP(handle, &padding_flag);
		if (padding_flag) {
			_Mapi_NJPD_Parser_Stuff_Char(handle, E_MARKER_FF);
			return E_MARKER_FF;
		}

		if (c == E_MARKER_00)
			return E_MARKER_FF;
		_Mapi_NJPD_Parser_Stuff_Char(handle, c);
		_Mapi_NJPD_Parser_Stuff_Char(handle, E_MARKER_FF);
		return E_MARKER_FF;
	}

	return c;
}

//------------------------------------------------------------------------------
// Retrieves a variable number of bits from the input stream.
// Markers will not be read into the input bit buffer. Instead,
// an infinite number of all 1's will be returned when a marker
// is encountered.
// FIXME: Is it better to return all 0's instead, like the older implementation?
static u32 _Mapi_NJPD_Parser_Get_Bits_2(int handle, u8 numbits)
{
	u32 i;
	u8 c1, c2;
	NJPD_ProgInfo *pstProgInfo;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;
	i = (pstProgInfo->_u32Bit_buf >> (E_NUM16 - numbits)) & ((1 << numbits) - 1);

	pstProgInfo->_s16Bits_left -= numbits;
	if (pstProgInfo->_s16Bits_left <= 0) {
		pstProgInfo->_u32Bit_buf = _Mapi_NJPD_Rol(pstProgInfo->_u32Bit_buf, numbits +=
						      pstProgInfo->_s16Bits_left);

		c1 = _Mapi_NJPD_Parser_Get_Octet(handle);
		c2 = _Mapi_NJPD_Parser_Get_Octet(handle);

		pstProgInfo->_u32Bit_buf = (pstProgInfo->_u32Bit_buf & E_MARKER_FFFF)
			| (((u32) c1) << E_NUM24) | (((u32) c2) << E_NUM16);

		pstProgInfo->_u32Bit_buf =
		    _Mapi_NJPD_Rol(pstProgInfo->_u32Bit_buf, -pstProgInfo->_s16Bits_left);

		pstProgInfo->_s16Bits_left += E_NUM16;
	} else {
		pstProgInfo->_u32Bit_buf = _Mapi_NJPD_Rol(pstProgInfo->_u32Bit_buf, numbits);
	}

	return i;
}

//------------------------------------------------------------------------------
// Decodes a Huffman encoded symbol.
static s32 _Mapi_NJPD_Parser_Huff_Decode(int handle, NJPD_HuffTbl *Ph)
{
	s32 symbol;
	NJPD_ProgInfo *pstProgInfo;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;
	// Check first 8-bits: do we have a complete symbol?
	symbol = Ph->s16Look_up[(pstProgInfo->_u32Bit_buf >> E_NUM8) & E_MARKER_FF];
	if (symbol < 0) {
		// Decode more bits, use a tree traversal to find symbol.
		_Mapi_NJPD_Parser_Get_Bits_2(handle, E_NUM8);

		do {
			symbol = Ph->s16Tree[~symbol +
				(1 - _Mapi_NJPD_Parser_Get_Bits_2(handle, 1))];
		} while (symbol < 0);
	} else {
		_Mapi_NJPD_Parser_Get_Bits_2(handle, Ph->u8Code_size[symbol]);
	}

	return symbol;
}


//------------------------------------------------------------------------------
// Determines the component order inside each MCU.
// Also calcs how many MCU's are on each row, etc.
static void _Mapi_NJPD_Parser_Calc_Mcu_Block_Order(int handle)
{
	u8 component_num, component_id;
	u8 max_h_samp = 0, max_v_samp = 0;
	u8 num_blocks;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_SOSInfo *pstSOSInfo;
	NJPD_ProgInfo *pstProgInfo;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;

	for (component_id = 0; component_id < pstSOFInfo->u8ComponentsInFrame; component_id++) {
		if (pstSOFInfo->u8ComponentHSamp[component_id] > max_h_samp)
			max_h_samp = pstSOFInfo->u8ComponentHSamp[component_id];

		if (pstSOFInfo->u8ComponentVSamp[component_id] > max_v_samp)
			max_v_samp = pstSOFInfo->u8ComponentVSamp[component_id];
	}

	if ((max_h_samp == 0) || (max_v_samp == 0)) {
		njpd_parser_err(handle, "not enough header info");
		return;
	}

	for (component_id = 0; component_id < pstSOFInfo->u8ComponentsInFrame; component_id++) {
		pstProgInfo->_u16Comp_h_blocks[component_id] =
		    ((((pstSOFInfo->u16OriginalImageXSize *
			pstSOFInfo->u8ComponentHSamp[component_id]) + (max_h_samp -
							E_NUM1)) / max_h_samp) + E_NUM7) / E_NUM8;
		pstProgInfo->_u16Comp_v_blocks[component_id] =
		    ((((pstSOFInfo->u16OriginalImageYSize *
			pstSOFInfo->u8ComponentVSamp[component_id]) + (max_v_samp -
							E_NUM1)) / max_v_samp) + E_NUM7) / E_NUM8;
	}

	if (pstSOSInfo->u8ComponentsInScan == E_NUM1) {
		pstProgInfo->_u16Mcus_per_row =
		    pstProgInfo->_u16Comp_h_blocks[pstSOSInfo->u8ComponentList[0]];
		pstProgInfo->_u16Mcus_per_col =
		    pstProgInfo->_u16Comp_v_blocks[pstSOSInfo->u8ComponentList[0]];
	} else {
		pstProgInfo->_u16Mcus_per_row =
		    (((pstSOFInfo->u16OriginalImageXSize + E_NUM7) / E_NUM8)
		    + (max_h_samp - E_NUM1)) / max_h_samp;
		pstProgInfo->_u16Mcus_per_col =
		    (((pstSOFInfo->u16OriginalImageYSize + E_NUM7) / E_NUM8)
		    + (max_v_samp - E_NUM1)) / max_v_samp;
	}

	if (pstSOSInfo->u8ComponentsInScan == 1) {
		pstProgInfo->_u8Mcu_org[0] = pstSOSInfo->u8ComponentList[0];

		pstProgInfo->_u8Blocks_per_mcu = 1;
	} else {
		pstProgInfo->_u8Blocks_per_mcu = 0;

		for (component_num = 0; component_num < pstSOSInfo->u8ComponentsInScan;
		     component_num++) {

			component_id = pstSOSInfo->u8ComponentList[component_num];

			num_blocks =
			    pstSOFInfo->u8ComponentHSamp[component_id] *
			    pstSOFInfo->u8ComponentVSamp[component_id];

			while (num_blocks--) {
				pstProgInfo->_u8Mcu_org[pstProgInfo->_u8Blocks_per_mcu++] =
				    component_id;
			}
		}
	}
}

//------------------------------------------------------------------------------
// Find a start of scan (SOS) marker.
static NJPD_API_ParserResult _Mapi_NJPD_Parser_Locate_SOS_Marker(int handle)
{
	NJPD_API_ParserResult eRet;
	u8 *pu8TempPoint;
	size_t szTempDataLeft;
	bool bLastBuffer;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_HEADER_INVALID;
	}

	eRet = _Mapi_NJPD_Parser_Process_Markers(handle);

	if (eRet == E_NJPD_API_HEADER_END) {
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "find EOI, No more SOS");
	} else if (eRet == E_NJPD_API_HEADER_INVALID) {
		njpd_parser_err(handle, "invalid header");
	} else if (eRet == E_NJPD_API_HEADER_PARTIAL) {
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "parse SOS partial done");
	} else {
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "SOS Located");

		//ensure next marker
		pu8TempPoint = gNJPDParserInfo[handle].read_point;
		szTempDataLeft = gNJPDParserInfo[handle].data_left;
		bLastBuffer = gNJPDParserInfo[handle].last_buffer_used;

		_Mapi_NJPD_Find_Next_Markers(handle);
		if (gNJPDParserInfo[handle].need_more_data == true) {
			njpd_parser_debug(handle, LEVEL_API_DEBUG,
					  "SOS data is incomplete");
			gNJPDParserInfo[handle].need_more_data = false;
			return E_NJPD_API_HEADER_PARTIAL;
		}

		//find next marker, back to end of SOS
		gNJPDParserInfo[handle].read_point = pu8TempPoint;
		gNJPDParserInfo[handle].data_left = szTempDataLeft;
		gNJPDParserInfo[handle].last_buffer_used = bLastBuffer;
	}
	return eRet;
}

static NJPD_API_ParserResult _Mapi_NJPD_Parser_Init_Scan(int handle)
{
	NJPD_API_ParserResult eRet;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_TableInfo *pstTableInfo;
	NJPD_ProgInfo *pstProgInfo;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_HEADER_INVALID;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstTableInfo = &gNJPDParserInfo[handle].table_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;

	if (gNJPDParserInfo[handle].first_sos == true) {
		eRet = _Mapi_NJPD_Parser_Locate_SOS_Marker(handle);
		if (eRet != E_NJPD_API_HEADER_VALID)
			return eRet;
	} else {
		gNJPDParserInfo[handle].first_sos = true;
	}
	_Mapi_NJPD_Parser_Calc_Mcu_Block_Order(handle);

	if (gNJPDParserInfo[handle].check_DHT) {
		if (!_Mapi_NJPD_Check_Huff_Tables(handle))
			return E_NJPD_API_HEADER_INVALID;
	}

	NJPD_memset((void *)pstProgInfo->_u32Last_dc_val, 0,
		    pstSOFInfo->u8ComponentsInFrame * sizeof(u32));

	pstProgInfo->_u32EOB_run = 0;

	if (pstTableInfo->u16RestartInterval) {
		pstProgInfo->_u16Restarts_left = pstTableInfo->u16RestartInterval;
		pstProgInfo->_u16Next_restart_num = 0;
	}

	// pre-fill bit buffer for later decoding
	pstProgInfo->_s16Bits_left = E_NUM16;
	_Mapi_NJPD_Parser_Get_Bits_2(handle, E_NUM16);
	_Mapi_NJPD_Parser_Get_Bits_2(handle, E_NUM16);

	return E_NJPD_API_HEADER_VALID;
}

//------------------------------------------------------------------------------
// Starts a frame. Determines if the number of components or sampling factors
// are supported.
static u8 _Mapi_NJPD_Parser_Init_Frame(int handle)
{
	u8 gu8Scan_type;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_ProgInfo *pstProgInfo;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;

	if (pstSOFInfo->u8ComponentsInFrame == 1) {
		gu8Scan_type = E_JPEG_GRAYSCALE;
		pstProgInfo->_u16Max_blocks_per_mcu = 1;
		pstProgInfo->gu8Max_mcu_x_size = E_NUM8;
		pstProgInfo->gu8Max_mcu_y_size = E_NUM8;
	} else if (pstSOFInfo->u8ComponentsInFrame == E_NUM3) {
		if (((pstSOFInfo->u8ComponentHSamp[1] != 1) //support only U_H1V1 & V_H1V1
			|| (pstSOFInfo->u8ComponentVSamp[1] != 1))
			|| ((pstSOFInfo->u8ComponentHSamp[E_NUM2] != 1)
		     || (pstSOFInfo->u8ComponentVSamp[E_NUM2] != 1))) {
			njpd_parser_err(handle, "line: %d, unsupport samp factors", __LINE__);
			return false;
		}

		if ((pstSOFInfo->u8ComponentHSamp[0] == 1)
		    && (pstSOFInfo->u8ComponentVSamp[0] == 1)) {
			//set RGB based jpeg flag
			if (pstSOFInfo->u8ComponentIdent[0] == E_NUM82
			    || pstSOFInfo->u8ComponentIdent[0] == E_NUM71
			    || pstSOFInfo->u8ComponentIdent[0] == E_NUM66) {
				njpd_parser_err(handle,
					"line: %d, unsupport samp factors", __LINE__);
				return false;
			}

			gu8Scan_type = E_JPEG_YH1V1;	//4:4:4
			pstProgInfo->_u16Max_blocks_per_mcu = E_NUM3;
			pstProgInfo->gu8Max_mcu_x_size = E_NUM8;
			pstProgInfo->gu8Max_mcu_y_size = E_NUM8;
		} else if ((pstSOFInfo->u8ComponentHSamp[0] == E_NUM2)
			   && (pstSOFInfo->u8ComponentVSamp[0] == 1)) {
			gu8Scan_type = E_JPEG_YH2V1;	//4:2:2
			pstProgInfo->_u16Max_blocks_per_mcu = E_NUM4;
			pstProgInfo->gu8Max_mcu_x_size = E_NUM16;
			pstProgInfo->gu8Max_mcu_y_size = E_NUM8;
		} else if ((pstSOFInfo->u8ComponentHSamp[0] == 1)
			   && (pstSOFInfo->u8ComponentVSamp[0] == E_NUM2)) {
			gu8Scan_type = E_JPEG_YH1V2;	//4:4:0
			pstProgInfo->_u16Max_blocks_per_mcu = E_NUM4;
			pstProgInfo->gu8Max_mcu_x_size = E_NUM8;
			pstProgInfo->gu8Max_mcu_y_size = E_NUM16;
		} else if ((pstSOFInfo->u8ComponentHSamp[0] == E_NUM2)
			   && (pstSOFInfo->u8ComponentVSamp[0] == E_NUM2)) {
			gu8Scan_type = E_JPEG_YH2V2;	//4:2:0
			pstProgInfo->_u16Max_blocks_per_mcu = E_NUM6;
			pstProgInfo->gu8Max_mcu_x_size = E_NUM16;
			pstProgInfo->gu8Max_mcu_y_size = E_NUM16;
		} else if ((pstSOFInfo->u8ComponentHSamp[0] == E_NUM4)
			   && (pstSOFInfo->u8ComponentVSamp[0] == 1)) {
			gu8Scan_type = E_JPEG_YH4V1;    //4:1:1
			pstProgInfo->_u16Max_blocks_per_mcu = E_NUM6;
			pstProgInfo->gu8Max_mcu_x_size = E_NUM32;
			pstProgInfo->gu8Max_mcu_y_size = E_NUM8;
		} else {
			njpd_parser_err(handle, "line: %d, unsupport samp factors", __LINE__);
			return false;
		}
	} else {
		njpd_parser_err(handle, "unsupport colorspace");
		return false;
	}

	njpd_parser_debug(handle, LEVEL_API_DEBUG, "gu8Scan_type = %d", gu8Scan_type);

	pstProgInfo->gu16Max_mcus_per_row =
	    (pstSOFInfo->u16OriginalImageXSize +
	     (pstProgInfo->gu8Max_mcu_x_size - 1)) / pstProgInfo->gu8Max_mcu_x_size;
	pstProgInfo->_u16Max_mcus_per_col =
	    (pstSOFInfo->u16OriginalImageYSize +
	     (pstProgInfo->gu8Max_mcu_y_size - 1)) / pstProgInfo->gu8Max_mcu_y_size;

	pstProgInfo->_u32Max_blocks_per_row =
	    pstProgInfo->gu16Max_mcus_per_row * pstProgInfo->_u16Max_blocks_per_mcu;

	// Should never happen
	if (pstProgInfo->_u32Max_blocks_per_row > JPEG_MAXBLOCKSPERROW) {
		njpd_parser_err(handle, "assertion error");
		return false;
	}

	pstProgInfo->_u16Total_lines_left =
	    pstProgInfo->_u16Max_mcus_per_col * pstProgInfo->gu8Max_mcu_y_size;
	return true;
}

// Restart interval processing.
static u8 _Mapi_NJPD_Parser_Process_Restart(int handle)
{
	u16 i, c = 0;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_TableInfo *pstTableInfo;
	NJPD_ProgInfo *pstProgInfo;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstTableInfo = &gNJPDParserInfo[handle].table_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;

	// Let's scan a little bit to find the marker, but not _too_ far.
	// 1536 is a "fudge factor" that determines how much to scan.
	for (i = SCAN_DEPTH; i > 0; i--) {
		if (_Mapi_NJPD_Parser_Get_Char(handle) == E_MARKER_FF)
			break;
	}

	if (i == 0) {
		njpd_parser_err(handle, "line: %d", __LINE__);
		return FALSE;
	}

	for (; i > 0; i--) {
		c = _Mapi_NJPD_Parser_Get_Char(handle);
		if (c != E_MARKER_FF)
			break;
	}

	if (i == 0) {
		njpd_parser_err(handle, "line: %d", __LINE__);
		return false;
	}
	// Is it the expected marker? If not, something bad happened.
	if (c != (pstProgInfo->_u16Next_restart_num + E_NJPD_MARKER_RST0)) {
		njpd_parser_err(handle, "line: %d", __LINE__);
		return false;
	}
	// Reset each component's DC prediction values.
	NJPD_memset((void *)&pstProgInfo->_u32Last_dc_val, 0,
		    pstSOFInfo->u8ComponentsInFrame * sizeof(__u32));

	pstProgInfo->_u32EOB_run = 0;

	pstProgInfo->_u16Restarts_left = pstTableInfo->u16RestartInterval;

	pstProgInfo->_u16Next_restart_num = (pstProgInfo->_u16Next_restart_num + 1) & E_NUM7;

	// Get the bit buffer going again...
	{
		pstProgInfo->_s16Bits_left = E_NUM16;
		_Mapi_NJPD_Parser_Get_Bits_2(handle, E_NUM16);
		_Mapi_NJPD_Parser_Get_Bits_2(handle, E_NUM16);
	}
	return true;
}

//------------------------------------------------------------------------------
// The following methods decode the various types of blocks encountered
// in progressively encoded images.
static u8 _Mapi_NJPD_Parser_Decode_Block_DC_First(int handle, u8 component_id,
		u16 block_x, u16 block_y)
{
	s32 s, r;
	NJPD_BLOCK_TYPE *p;
	NJPD_SOSInfo *pstSOSInfo;
	NJPD_ProgInfo *pstProgInfo;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;

	p = _Mapi_NJPD_Coeff_Buf_Getp(pstProgInfo->_DC_Coeffs[component_id], block_x, block_y);

	if (p == NULL) {
		njpd_parser_err(handle, "line: %d", __LINE__);
		return false;
	}

	s = _Mapi_NJPD_Parser_Huff_Decode
		(handle, &pstProgInfo->_Huff_tbls[pstSOSInfo->u8ComponentDCTable[component_id]]);
	if (s != 0) {
		r = _Mapi_NJPD_Parser_Get_Bits_2(handle, s);
		s = HUFF_EXTEND_P(r, s);
	}
	// In NJPD mode, the DC coefficient is the difference of nearest DC
	pstProgInfo->_u32Last_dc_val[component_id] =
		(s += pstProgInfo->_u32Last_dc_val[component_id]);

	p[0] = s << pstSOSInfo->u8SuccessiveLow;
	return true;
}

//------------------------------------------------------------------------------
static u8 _Mapi_NJPD_Parser_Decode_Block_DC_Refine(int handle, u8 component_id,
		u16 block_x, u16 block_y)
{
	NJPD_BLOCK_TYPE *p;
	NJPD_SOSInfo *pstSOSInfo;
	NJPD_ProgInfo *pstProgInfo;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;

	if (_Mapi_NJPD_Parser_Get_Bits_2(handle, 1)) {
		p = _Mapi_NJPD_Coeff_Buf_Getp(pstProgInfo->_DC_Coeffs[component_id],
			block_x, block_y);

		if (p == NULL) {
			njpd_parser_err(handle, "line: %d", __LINE__);
			return false;
		}

		p[0] |= (1 << pstSOSInfo->u8SuccessiveLow);
	}
	return true;
}

//------------------------------------------------------------------------------
static u8 _Mapi_NJPD_Parser_Decode_Block_AC_First(int handle, u8 component_id,
			u16 block_x, u16 block_y)
{
	NJPD_BLOCK_TYPE *p;
	s32 k, s, r;
	NJPD_SOSInfo *pstSOSInfo;
	NJPD_ProgInfo *pstProgInfo;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;
	if (pstProgInfo->_u32EOB_run) {
		pstProgInfo->_u32EOB_run--;
		return true;
	}

	p = _Mapi_NJPD_Coeff_Buf_Getp(pstProgInfo->_AC_Coeffs[component_id], block_x, block_y);

	if (p == NULL) {
		njpd_parser_err(handle, "line: %d", __LINE__);
		return false;
	}

	for (k = pstSOSInfo->u8SpectralStart; k <= pstSOSInfo->u8SpectralEnd; k++) {
		s = _Mapi_NJPD_Parser_Huff_Decode(handle, &pstProgInfo->_Huff_tbls[
			pstSOSInfo->u8ComponentACTable[component_id]]);

		r = s >> E_NUM4;
		s &= E_NUM15;

		if (s) {
			k += r;
			if (k > E_NUM63) {
				njpd_parser_err(handle, "line: %d", __LINE__);
				return false;
			}

			r = _Mapi_NJPD_Parser_Get_Bits_2(handle, s);
			s = HUFF_EXTEND_P(r, s);

			// No need to do ZAG order in NJPD mode
			p[k] = s << pstSOSInfo->u8SuccessiveLow;
		} else {
			if (r == E_NUM15) {
				k += E_NUM15;
				if (k > E_NUM63) {
					njpd_parser_err(handle, "line: %d", __LINE__);
					return false;
				}
			} else {
				pstProgInfo->_u32EOB_run = 1 << r;

				if (r)
					pstProgInfo->_u32EOB_run +=
					_Mapi_NJPD_Parser_Get_Bits_2(handle, r);

				pstProgInfo->_u32EOB_run--;

				break;
			}
		}
	}
	return true;
}

//------------------------------------------------------------------------------
static u8 _Mapi_NJPD_Parser_Decode_Block_AC_Refine(int handle, u8 component_id,
			u16 block_x, u16 block_y)
{
	s32 s, k, r;
	s32 p1;
	s32 m1;
	NJPD_BLOCK_TYPE *p;
	NJPD_BLOCK_TYPE *this_coef;
	NJPD_SOSInfo *pstSOSInfo;
	NJPD_ProgInfo *pstProgInfo;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;
	p1 = 1 << pstSOSInfo->u8SuccessiveLow;
	m1 = (-1) << pstSOSInfo->u8SuccessiveLow;

	p = _Mapi_NJPD_Coeff_Buf_Getp(pstProgInfo->_AC_Coeffs[component_id], block_x, block_y);

	if (p == NULL) {
		njpd_parser_err(handle, "line: %d", __LINE__);
		return false;
	}

	k = pstSOSInfo->u8SpectralStart;

	if (pstProgInfo->_u32EOB_run == 0) {
		for (; (k <= pstSOSInfo->u8SpectralEnd) && (k < E_NUM64); k++) {
			s = _Mapi_NJPD_Parser_Huff_Decode(handle, &pstProgInfo->_Huff_tbls[
				pstSOSInfo->u8ComponentACTable[component_id]]);

			r = s >> E_NUM4;
			s &= E_NUM15;

			if (s) {
				if (s != 1) {
					njpd_parser_err(handle, "decode error");
					return false;
				}

				if (_Mapi_NJPD_Parser_Get_Bits_2(handle, 1))
					s = p1;
				else
					s = m1;
			} else {
				if (r != E_NUM15) {
					pstProgInfo->_u32EOB_run = 1 << r;

					if (r)
						pstProgInfo->_u32EOB_run +=
						_Mapi_NJPD_Parser_Get_Bits_2(handle, r);

					break;
				}
			}

			do {
				// No need to do ZAG order in NJPD mode
				this_coef = p + k;

				if (*this_coef != 0) {
					if (_Mapi_NJPD_Parser_Get_Bits_2(handle, 1) &&
						((*this_coef & p1) == 0))
						*this_coef += (*this_coef >= 0) ? p1 : m1;
				} else {
					if (--r < 0)
						break;
				}

				k++;
			} while ((k <= pstSOSInfo->u8SpectralEnd) && (k < E_NUM64));

			if ((s) && (k < E_NUM64)) {
				// No need to do ZAG order in NJPD mode
				p[k] = s;
			}
		}
	}

	if (pstProgInfo->_u32EOB_run > 0) {
		for (; (k <= pstSOSInfo->u8SpectralEnd) && (k < E_NUM64); k++) {
			// No need to do ZAG order in NJPD mode
			this_coef = p + k;

			if (*this_coef != 0) {
				if (_Mapi_NJPD_Parser_Get_Bits_2(handle, 1) &&
					((*this_coef & p1) == 0))
					*this_coef += (*this_coef >= 0) ? p1 : m1;
			}
		}

		pstProgInfo->_u32EOB_run--;
	}
	return true;
}

//------------------------------------------------------------------------------
// Decode a scan in a progressively encoded image.
static u8 _Mapi_NJPD_Parser_Decode_Scan(int handle, NJPD_API_PdecodeBlockFunc decode_block_func)
{
	u16 mcu_row, mcu_col, mcu_block;
	u32 block_x_mcu[NJPD_MAXCOMPONENTS], block_y_mcu[NJPD_MAXCOMPONENTS];
	u32 component_num, component_id;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_SOSInfo *pstSOSInfo;
	NJPD_TableInfo *pstTableInfo;
	NJPD_ProgInfo *pstProgInfo;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;
	pstTableInfo = &gNJPDParserInfo[handle].table_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;

	NJPD_memset((void *)block_y_mcu, 0, sizeof(block_y_mcu));

	for (mcu_col = 0; mcu_col < pstProgInfo->_u16Mcus_per_col; mcu_col++) {

		NJPD_memset((void *)block_x_mcu, 0, sizeof(block_x_mcu));

		for (mcu_row = 0; mcu_row < pstProgInfo->_u16Mcus_per_row; mcu_row++) {
			__u8 block_x_mcu_ofs = 0, block_y_mcu_ofs = 0;

			if ((pstTableInfo->u16RestartInterval)
			    && (pstProgInfo->_u16Restarts_left == 0)) {
				if (!_Mapi_NJPD_Parser_Process_Restart(handle))
					return false;
			}

			for (mcu_block = 0; mcu_block < pstProgInfo->_u8Blocks_per_mcu;
			     mcu_block++) {
				component_id = pstProgInfo->_u8Mcu_org[mcu_block];

				if (!decode_block_func
				    (handle, component_id, block_x_mcu[component_id] +
				    block_x_mcu_ofs, block_y_mcu[component_id] + block_y_mcu_ofs)) {
					njpd_parser_err(handle, "line: %d", __LINE__);
					return false;
				}

				if (pstSOSInfo->u8ComponentsInScan == 1)
					block_x_mcu[component_id]++;
				else if (++block_x_mcu_ofs ==
					pstSOFInfo->u8ComponentHSamp[component_id]) {
					block_x_mcu_ofs = 0;

					if (++block_y_mcu_ofs ==
						pstSOFInfo->u8ComponentVSamp[component_id]) {
						block_y_mcu_ofs = 0;
						block_x_mcu[component_id] +=
							pstSOFInfo->u8ComponentHSamp[component_id];
					}
				}
			}

			pstProgInfo->_u16Restarts_left--;
		}

		if (pstSOSInfo->u8ComponentsInScan == 1) {
			block_y_mcu[pstSOSInfo->u8ComponentList[0]]++;
		} else {
			for (component_num = 0; component_num < pstSOSInfo->u8ComponentsInScan;
			     component_num++) {
				component_id = pstSOSInfo->u8ComponentList[component_num];

				block_y_mcu[component_id] +=
				    pstSOFInfo->u8ComponentVSamp[component_id];
			}
		}
	}
	return true;
}

static NJPD_API_ParserResult  _Mapi_NJPD_Parser_Init_Progressive(int handle)
{
	u8 i;
	u8 dc_only_scan, refinement_scan;
	NJPD_API_ParserResult eRet;
	NJPD_API_PdecodeBlockFunc decode_block_func;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_SOSInfo *pstSOSInfo;
	NJPD_ProgInfo *pstProgInfo;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_HEADER_INVALID;
	}
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;
	pstProgInfo = &gNJPDParserInfo[handle].prog_info;

	if (gNJPDParserInfo[handle].first_sos == true) //not first time
		goto NJPD_PARSER_START_INIT_SCAN;

	gNJPDParserInfo[handle].first_sos = true;

	if (pstSOFInfo->u8ComponentsInFrame == E_NUM4) {
		njpd_parser_err(handle, "unsupported colorspace");
		return E_NJPD_API_HEADER_INVALID;
	}

	if (!_Mapi_NJPD_Parser_Init_Frame(handle)) {
		njpd_parser_err(handle, "init frame fail");
		return E_NJPD_API_HEADER_INVALID;
	}
	// Allocate the coefficient buffers.
	for (i = 0; i < pstSOFInfo->u8ComponentsInFrame; i++) {
		pstProgInfo->_DC_Coeffs[i] =
		    _Mapi_NJPD_Coeff_Buf_Open(((pstProgInfo->gu16Max_mcus_per_row +
					  0x1) & ~0x1) * pstSOFInfo->u8ComponentHSamp[i],
					((pstProgInfo->_u16Max_mcus_per_col +
					  0x1) & ~0x1) * pstSOFInfo->u8ComponentVSamp[i], 1, 1);
		if (pstProgInfo->_DC_Coeffs[i] == NULL)
			return E_NJPD_API_HEADER_INVALID;

		pstProgInfo->_AC_Coeffs[i] =
			_Mapi_NJPD_Coeff_Buf_Open(((pstProgInfo->gu16Max_mcus_per_row +
				0x1) & ~0x1) * pstSOFInfo->u8ComponentHSamp[i],
				((pstProgInfo->_u16Max_mcus_per_col +
				0x1) & ~0x1) * pstSOFInfo->u8ComponentVSamp[i], E_NUM8, E_NUM8);
		if (pstProgInfo->_AC_Coeffs[i] == NULL)
			return E_NJPD_API_HEADER_INVALID;
	}
	gNJPDParserInfo[handle].first_sos = false;

NJPD_PARSER_START_INIT_SCAN:
	while (1) {
		eRet = _Mapi_NJPD_Parser_Init_Scan(handle);
		if (eRet == E_NJPD_API_HEADER_END)
			break;
		else if ((eRet == E_NJPD_API_HEADER_INVALID) || (eRet == E_NJPD_API_HEADER_PARTIAL))
			return eRet;

		dc_only_scan = (pstSOSInfo->u8SpectralStart == 0);
		refinement_scan = (pstSOSInfo->u8SuccessiveHigh != 0);

		if ((pstSOSInfo->u8SpectralStart > pstSOSInfo->u8SpectralEnd)
		    || (pstSOSInfo->u8SpectralEnd > E_NUM63)) {
			njpd_parser_err(handle, "line: %d, bad SOS spectral", __LINE__);
			return E_NJPD_API_HEADER_INVALID;
		}

		if (dc_only_scan) {
			if (pstSOSInfo->u8SpectralEnd) {
				njpd_parser_err(handle, "line: %d, bad SOS spectral", __LINE__);
				return false;
			}
		} else if (pstSOSInfo->u8ComponentsInScan != 1) {
			njpd_parser_err(handle, "line: %d, bad SOS spectral", __LINE__);
			return E_NJPD_API_HEADER_INVALID;
		}

		if ((refinement_scan)
		    && (pstSOSInfo->u8SuccessiveLow !=
			pstSOSInfo->u8SuccessiveHigh - 1)) {
			njpd_parser_err(handle, "bad SOS successive");
			return E_NJPD_API_HEADER_INVALID;
		}

		if (dc_only_scan) {
			if (refinement_scan)
				decode_block_func = _Mapi_NJPD_Parser_Decode_Block_DC_Refine;
			else
				decode_block_func = _Mapi_NJPD_Parser_Decode_Block_DC_First;
		} else {
			if (refinement_scan)
				decode_block_func = _Mapi_NJPD_Parser_Decode_Block_AC_Refine;
			else
				decode_block_func = _Mapi_NJPD_Parser_Decode_Block_AC_First;
		}

		if (!_Mapi_NJPD_Parser_Decode_Scan(handle, decode_block_func))
			return E_NJPD_API_HEADER_INVALID;

		pstProgInfo->_s16Bits_left = 0;

	}

	pstSOSInfo->u8ComponentsInScan = pstSOFInfo->u8ComponentsInFrame;

	for (i = 0; i < pstSOFInfo->u8ComponentsInFrame; i++)
		pstSOSInfo->u8ComponentList[i] = i;

	_Mapi_NJPD_Parser_Calc_Mcu_Block_Order(handle);
	return E_NJPD_API_HEADER_VALID;
}
#endif

static NJPD_API_ParserResult _MApi_NJPD_Parser_ParseHeader(int handle, NJPD_API_Buf buf)
{
	NJPD_API_ParserResult eRet = E_NJPD_API_HEADER_INVALID;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_HEADER_INVALID;
	}

	if (gNJPDParserInfo[handle].has_last_buffer == false) {
		gNJPDParserInfo[handle].read_point = (u8 *)buf.va;
		gNJPDParserInfo[handle].data_left = buf.filled_length;
		gNJPDParserInfo[handle].data_fill = buf.filled_length;
		_Mapi_NJPD_Parser_Skip_Bytes(handle, buf.offset);
	} else {
		gNJPDParserInfo[handle].read_point =
		    (u8 *)gNJPDParserInfo[handle].last_buffer_data;
		gNJPDParserInfo[handle].data_left = gNJPDParserInfo[handle].last_buffer_length;
		gNJPDParserInfo[handle].data_fill = gNJPDParserInfo[handle].last_buffer_length;
		gNJPDParserInfo[handle].read_buffer = &buf;
	}

	if (gNJPDParserInfo[handle].check_SOI == false) {
		//read SOI
		if (_Mapi_NJPD_Parser_Locate_SOI_Marker(handle) == false) {
			njpd_parser_err(handle, "locate soi failed");
			return E_NJPD_API_HEADER_INVALID;
		}
		gNJPDParserInfo[handle].check_SOI = true;
	}
	//read SOF, TABLE, SOS

	if (gNJPDParserInfo[handle].check_SOS == false) {
		eRet = _Mapi_NJPD_Parser_Process_Markers(handle);
		if (eRet == E_NJPD_API_HEADER_INVALID || eRet == E_NJPD_API_HEADER_END) {
			njpd_parser_err(handle, "process marker failed");
			return eRet;
		}

		if (gNJPDParserInfo[handle].check_DHT == false) {
			_Mapi_NJPD_Read_Default_DHT(handle);
			gNJPDParserInfo[handle].check_DHT = true;
		}

		if (gNJPDParserInfo[handle].check_DHT && !gNJPDParserInfo[handle].is_progressive) {
			if (!_Mapi_NJPD_Check_Huff_Tables(handle))
				return E_NJPD_API_HEADER_INVALID;
		}

		if (gNJPDParserInfo[handle].check_DQT) {
			if (!_Mapi_NJPD_Check_Quant_Tables(handle))
				return E_NJPD_API_HEADER_INVALID;
		}
	} else {
		if (!gNJPDParserInfo[handle].is_progressive) {
			njpd_parser_err(handle, "some flow goes wrong");
				return E_NJPD_API_HEADER_INVALID;
		}
	}
#if SUPPORT_PROGRESSIVE
	if (gNJPDParserInfo[handle].is_progressive) {
		eRet = _Mapi_NJPD_Parser_Init_Progressive(handle);
		if (eRet == E_NJPD_API_HEADER_INVALID) {
			njpd_parser_err(handle, "init progressive fail");
			return E_NJPD_API_HEADER_INVALID;
		}
	}
#endif
	if (gNJPDParserInfo[handle].has_last_buffer == true &&
		gNJPDParserInfo[handle].last_buffer_used == true) {
		NJPD_FREE(gNJPDParserInfo[handle].last_buffer_data,
				  gNJPDParserInfo[handle].last_buffer_length);
			gNJPDParserInfo[handle].last_buffer_data = NULL;
			gNJPDParserInfo[handle].has_last_buffer = false;
			gNJPDParserInfo[handle].last_buffer_used = false;
	}
	return eRet;
}

static void _MApi_NJPD_Parser_Print_HuffInfo(int handle)
{
	u16 dc_tbl_num_luma = 0, dc_tbl_num_chroma = 0;
	u16 ac_tbl_num_luma = 0, ac_tbl_num_chroma = 0;
	u16 dc_tbl_num_chroma2 = 0;
	u16 ac_tbl_num_chroma2 = 0;
	u16 i;
	NJPD_TableInfo *pstTableInfo;
	NJPD_HuffInfo *pstHuffInfo;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return;
	}
	pstTableInfo = &gNJPDParserInfo[handle].table_info;
	pstHuffInfo = gNJPDParserInfo[handle].huff_info;

	dc_tbl_num_luma = pstTableInfo->u16DcTblNumLuma;
	dc_tbl_num_chroma = pstTableInfo->u16DcTblNumChroma;
	ac_tbl_num_luma = pstTableInfo->u16AcTblNumLuma;
	ac_tbl_num_chroma = pstTableInfo->u16AcTblNumChroma;
	dc_tbl_num_chroma2 = pstTableInfo->u16DcTblNumChroma2;
	ac_tbl_num_chroma2 = pstTableInfo->u16AcTblNumChroma2;

	for (i = 1; i <= MAX_TABLE_SIZE; i++)
		njpd_parser_debug(handle, LEVEL_API_PRINTMEM,
			"dc luma i=%d, [%02x %04x %02x]\n", i,
			pstHuffInfo[dc_tbl_num_luma].u8Valid[i],
			pstHuffInfo[dc_tbl_num_luma].u16Code[i],
			pstHuffInfo[dc_tbl_num_luma].u8Symbol[i]);
	for (i = 1; i <= MAX_TABLE_SIZE; i++)
		njpd_parser_debug(handle, LEVEL_API_PRINTMEM,
			"ac luma i=%d, [%02x %04x %02x]\n", i,
			pstHuffInfo[ac_tbl_num_luma].u8Valid[i],
			pstHuffInfo[ac_tbl_num_luma].u16Code[i],
			pstHuffInfo[ac_tbl_num_luma].u8Symbol[i]);
	for (i = 1; i <= MAX_TABLE_SIZE; i++)
		njpd_parser_debug(handle, LEVEL_API_PRINTMEM,
			"dc chroma i=%d, [%02x %04x %02x]\n", i,
			pstHuffInfo[dc_tbl_num_chroma].u8Valid[i],
			pstHuffInfo[dc_tbl_num_chroma].u16Code[i],
			pstHuffInfo[dc_tbl_num_chroma].u8Symbol[i]);
	for (i = 1; i <= MAX_TABLE_SIZE; i++)
		njpd_parser_debug(handle, LEVEL_API_PRINTMEM,
			"ac chroma i=%d, [%02x %04x %02x]\n", i,
			pstHuffInfo[ac_tbl_num_chroma].u8Valid[i],
			pstHuffInfo[ac_tbl_num_chroma].u16Code[i],
			pstHuffInfo[ac_tbl_num_chroma].u8Symbol[i]);
	if (pstTableInfo->bIs3HuffTbl == true) {
		for (i = 1; i <= MAX_TABLE_SIZE; i++)
			njpd_parser_debug(handle, LEVEL_API_PRINTMEM,
				"dc chroma2 i=%d, [%02x %04x %02x]\n", i,
				pstHuffInfo[dc_tbl_num_chroma2].u8Valid[i],
				pstHuffInfo[dc_tbl_num_chroma2].u16Code[i],
				pstHuffInfo[dc_tbl_num_chroma2].u8Symbol[i]);
		for (i = 1; i <= MAX_TABLE_SIZE; i++)
			njpd_parser_debug(handle, LEVEL_API_PRINTMEM,
				"ac chroma2 i=%d, [%02x %04x %02x]\n", i,
				pstHuffInfo[ac_tbl_num_chroma2].u8Valid[i],
				pstHuffInfo[ac_tbl_num_chroma2].u16Code[i],
				pstHuffInfo[ac_tbl_num_chroma2].u8Symbol[i]);
	}
}

static void _MApi_NJPD_Parser_Write_GroupTable(int handle, void *va)
{
	u16 dc_tbl_num_luma = 0, dc_tbl_num_chroma = 0;
	u16 ac_tbl_num_luma = 0, ac_tbl_num_chroma = 0;
	u16 dc_tbl_num_chroma2 = 0;
	u16 ac_tbl_num_chroma2 = 0;
	u16 i;
	NJPD_TableInfo *pstTableInfo;
	NJPD_HuffInfo *pstHuffInfo;
	uintptr_t utTableAddr = (uintptr_t)va;

	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return;
	}
	pstTableInfo = &gNJPDParserInfo[handle].table_info;
	pstHuffInfo = gNJPDParserInfo[handle].huff_info;

	dc_tbl_num_luma = pstTableInfo->u16DcTblNumLuma;
	dc_tbl_num_chroma = pstTableInfo->u16DcTblNumChroma;
	ac_tbl_num_luma = pstTableInfo->u16AcTblNumLuma;
	ac_tbl_num_chroma = pstTableInfo->u16AcTblNumChroma;
	dc_tbl_num_chroma2 = pstTableInfo->u16DcTblNumChroma2;
	ac_tbl_num_chroma2 = pstTableInfo->u16AcTblNumChroma2;

	// DC
	for (i = 0; i < MAX_TABLE_SIZE; i++) {
		if (pstHuffInfo[dc_tbl_num_luma].u8Valid[i + 1]) {
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + i * E_OFFSET4)
				= pstHuffInfo[dc_tbl_num_luma].u8Symbol[i + 1];
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + i * E_OFFSET4 + E_NUM1)
				= pstHuffInfo[dc_tbl_num_luma].u16Code[i + 1] & E_MARKER_FF;
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + i * E_OFFSET4 + E_NUM2)
				= (pstHuffInfo[dc_tbl_num_luma].u16Code[i + 1] >> E_NUM8)
					& E_MARKER_FF;
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + i * E_OFFSET4 + E_NUM3)
				= pstHuffInfo[dc_tbl_num_luma].u8Valid[i + 1];
		} else {
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + i * E_OFFSET4)
				= pstHuffInfo[dc_tbl_num_luma].u8Symbol[i + 1];
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + i * E_OFFSET4 + E_NUM3)
				= pstHuffInfo[dc_tbl_num_luma].u8Valid[i + 1];
		}
	}

	// AC
	for (i = 0; i < MAX_TABLE_SIZE; i++) {
		if (pstHuffInfo[ac_tbl_num_luma].u8Valid[i + 1]) {
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET16 * E_NUM4 + i * E_OFFSET4)
				= (pstHuffInfo[ac_tbl_num_luma].u8Symbol[i + 1]
				+ pstHuffInfo[dc_tbl_num_luma].u8SymbolCnt);
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET16 * E_NUM4 + i * E_OFFSET4 + E_NUM1)
				= (pstHuffInfo[ac_tbl_num_luma].u16Code[i + 1] & E_MARKER_FF);
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET16 * E_NUM4 + i * E_OFFSET4 + E_NUM2)
				= ((pstHuffInfo[ac_tbl_num_luma].u16Code[i + 1] >> E_NUM8) &
				E_MARKER_FF);
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET16 * E_NUM4 + i * E_OFFSET4 + E_NUM3)
				= pstHuffInfo[ac_tbl_num_luma].u8Valid[i + 1];
		} else {
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET16 * E_NUM4 + i * E_OFFSET4)
				= (pstHuffInfo[ac_tbl_num_luma].u8Symbol[i + 1]
				+ pstHuffInfo[dc_tbl_num_luma].u8SymbolCnt);
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET16 * E_NUM4 + i * E_OFFSET4 + E_NUM3)
				= pstHuffInfo[ac_tbl_num_luma].u8Valid[i + 1];
		}
	}

	for (i = 0; i < MAX_TABLE_SIZE; i++) {
		if (pstHuffInfo[dc_tbl_num_chroma].u8Valid[i + 1]) {
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET32 * E_NUM4 + i * E_OFFSET4)
				= pstHuffInfo[dc_tbl_num_chroma].u8Symbol[i + 1];
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET32 * E_NUM4 + i * E_OFFSET4 + E_NUM1)
				= pstHuffInfo[dc_tbl_num_chroma].u16Code[i + 1] & E_MARKER_FF;
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET32 * E_NUM4 + i * E_OFFSET4 + E_NUM2)
				= (pstHuffInfo[dc_tbl_num_chroma].u16Code[i + 1] >> E_NUM8) &
				E_MARKER_FF;
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET32 * E_NUM4 + i * E_OFFSET4 + E_NUM3)
				= pstHuffInfo[dc_tbl_num_chroma].u8Valid[i + 1];
		} else {
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET32 * E_NUM4 + i * E_OFFSET4)
				= pstHuffInfo[dc_tbl_num_chroma].u8Symbol[i + 1];
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET32 * E_NUM4 + i * E_OFFSET4 + E_NUM3)
				= pstHuffInfo[dc_tbl_num_chroma].u8Valid[i + 1];
		}
	}

	for (i = 0; i < MAX_TABLE_SIZE; i++) {
		if (pstHuffInfo[ac_tbl_num_chroma].u8Valid[i + 1]) {
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET48 * E_NUM4 + i * E_OFFSET4)
				= (pstHuffInfo[ac_tbl_num_chroma].u8Symbol[i + 1]
				+ pstHuffInfo[dc_tbl_num_chroma].u8SymbolCnt);
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET48 * E_NUM4 + i * E_OFFSET4 + E_NUM1)
				= (pstHuffInfo[ac_tbl_num_chroma].u16Code[i + 1] & E_MARKER_FF);
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET48 * E_NUM4 + i * E_OFFSET4 + E_NUM2)
				= ((pstHuffInfo[ac_tbl_num_chroma].u16Code[i + 1] >> E_NUM8) &
				E_MARKER_FF);
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET48 * E_NUM4 + i * E_OFFSET4 + E_NUM3)
			    = pstHuffInfo[ac_tbl_num_chroma].u8Valid[i + 1];
		} else {
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET48 * E_NUM4 + i * E_OFFSET4)
				= (pstHuffInfo[ac_tbl_num_chroma].u8Symbol[i + 1]
				+ pstHuffInfo[dc_tbl_num_chroma].u8SymbolCnt);
			*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE
				+ E_OFFSET48 * E_NUM4 + i * E_OFFSET4 + E_NUM3)
				= pstHuffInfo[ac_tbl_num_chroma].u8Valid[i + 1];
		}
	}

	if (pstTableInfo->bIs3HuffTbl == true) {
		for (i = 0; i < MAX_TABLE_SIZE; i++) {
			if (pstHuffInfo[dc_tbl_num_chroma2].u8Valid[i + 1]) {
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET64 * E_NUM4 +
					i * E_OFFSET4)
					= pstHuffInfo[dc_tbl_num_chroma2].u8Symbol[i + 1];
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET64 * E_NUM4 +
					i * E_OFFSET4 + E_NUM1)
					= pstHuffInfo[dc_tbl_num_chroma2].u16Code[i + 1]
					& E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET64 * E_NUM4 +
					i * E_OFFSET4 + E_NUM2)
					= (pstHuffInfo[dc_tbl_num_chroma2].u16Code[i + 1] >>
					E_NUM8) & E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET64 * E_NUM4 +
					i * E_OFFSET4 + E_NUM3)
					= pstHuffInfo[dc_tbl_num_chroma2].u8Valid[i + 1];
			} else {
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET64 * E_NUM4 +
					i * E_OFFSET4)
					= pstHuffInfo[dc_tbl_num_chroma2].u8Symbol[i + 1];
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET64 * E_NUM4 +
					i * E_OFFSET4 + E_NUM3)
					= pstHuffInfo[dc_tbl_num_chroma2].u8Valid[i + 1];
			}
		}

		for (i = 0; i < MAX_TABLE_SIZE; i++) {
			if (pstHuffInfo[ac_tbl_num_chroma2].u8Valid[i + 1]) {
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET80 * E_NUM4 +
					i * E_OFFSET4)
					= (pstHuffInfo[ac_tbl_num_chroma2].u8Symbol[i + 1]
					+ pstHuffInfo[dc_tbl_num_chroma2].u8SymbolCnt);
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET80 * E_NUM4 +
					i * E_OFFSET4 + E_NUM1)
					= pstHuffInfo[ac_tbl_num_chroma2].u16Code[i + 1]
					& E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET80 * E_NUM4 +
					i * E_OFFSET4 + E_NUM2)
					= (pstHuffInfo[ac_tbl_num_chroma2].u16Code[i + 1] >>
					E_NUM8) & E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET80 * E_NUM4 +
					i * E_OFFSET4 + E_NUM3)
					= pstHuffInfo[ac_tbl_num_chroma2].u8Valid[i + 1];
			} else {
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET80 * E_NUM4 +
					i * E_OFFSET4)
					= (pstHuffInfo[ac_tbl_num_chroma2].u8Symbol[i + 1]
					+ pstHuffInfo[dc_tbl_num_chroma2].u8SymbolCnt);
				*(u8 *)(utTableAddr + NJPD_MEM_SCWGIF_BASE + E_OFFSET80 * E_NUM4 +
					i * E_OFFSET4 + E_NUM3)
					= pstHuffInfo[ac_tbl_num_chroma2].u8Valid[i + 1];
			}
		}
	}
}

static bool _MApi_NJPD_Parser_WriteGrpinf(int handle, void *va)
{
	u16 ci, dc_tbl_num_luma = 0, dc_tbl_num_chroma = 0;
	u16 ac_tbl_num_luma = 0, ac_tbl_num_chroma = 0;
	u8 luma_ci = 0, chroma_ci = 0;
	u16 dc_tbl_num_chroma2 = 0;
	u16 ac_tbl_num_chroma2 = 0;
	u8 chroma2_ci = 0;
	NJPD_TableInfo *pstTableInfo;
	NJPD_HuffInfo *pstHuffInfo;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_SOSInfo *pstSOSInfo;
	uintptr_t utTableAddr = (uintptr_t)va;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstTableInfo = &gNJPDParserInfo[handle].table_info;
	pstHuffInfo = gNJPDParserInfo[handle].huff_info;
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstSOSInfo = &gNJPDParserInfo[handle].sos_info;

	if (gNJPDParserInfo[handle].check_DHT) {
		for (ci = 0; ci < pstSOFInfo->u8ComponentsInFrame; ci++) {
			if (pstSOFInfo->u8LumaComponentID == pstSOFInfo->u8ComponentIdent[ci]) {
				luma_ci = ci;
				break;
			}
		}

		for (ci = 0; ci < pstSOFInfo->u8ComponentsInFrame; ci++) {
			if (pstSOFInfo->u8ChromaComponentID == pstSOFInfo->u8ComponentIdent[ci]) {
				chroma_ci = ci;
				break;
			}
		}

		if (pstTableInfo->bIs3HuffTbl == true) {
			for (ci = 0; ci < pstSOFInfo->u8ComponentsInFrame; ci++) {
				if (pstSOFInfo->u8Chroma2ComponentID
				    == pstSOFInfo->u8ComponentIdent[ci]) {
					chroma2_ci = ci;
					break;
				}
			}
		}

		dc_tbl_num_luma = pstSOSInfo->u8ComponentDCTable[luma_ci];
		dc_tbl_num_chroma = pstSOSInfo->u8ComponentDCTable[chroma_ci];

		ac_tbl_num_luma = pstSOSInfo->u8ComponentACTable[luma_ci];
		ac_tbl_num_chroma = pstSOSInfo->u8ComponentACTable[chroma_ci];

		if (pstTableInfo->bIs3HuffTbl == true) {
			dc_tbl_num_chroma2 = pstSOSInfo->u8ComponentDCTable[chroma2_ci];
			ac_tbl_num_chroma2 = pstSOSInfo->u8ComponentACTable[chroma2_ci];
		}

		pstTableInfo->u16DcTblNumLuma = dc_tbl_num_luma;
		pstTableInfo->u16DcTblNumChroma = dc_tbl_num_chroma;
		pstTableInfo->u16AcTblNumLuma = ac_tbl_num_luma;
		pstTableInfo->u16AcTblNumChroma = ac_tbl_num_chroma;
		pstTableInfo->u16DcTblNumChroma2 = dc_tbl_num_chroma2;
		pstTableInfo->u16AcTblNumChroma2 = ac_tbl_num_chroma2;

		pstTableInfo->u8DcLumaCnt = pstHuffInfo[dc_tbl_num_luma].u8SymbolCnt;
		pstTableInfo->u8DcChromaCnt = pstHuffInfo[dc_tbl_num_chroma].u8SymbolCnt;
		pstTableInfo->u8DcChroma2Cnt = pstHuffInfo[dc_tbl_num_chroma2].u8SymbolCnt;
		pstTableInfo->u8AcLumaCnt = pstHuffInfo[ac_tbl_num_luma].u8SymbolCnt;
		pstTableInfo->u8AcChromaCnt = pstHuffInfo[ac_tbl_num_chroma].u8SymbolCnt;
		pstTableInfo->u8AcChroma2Cnt = pstHuffInfo[ac_tbl_num_chroma2].u8SymbolCnt;

		if (jpd_api_debug & LEVEL_API_PRINTMEM)
			_MApi_NJPD_Parser_Print_HuffInfo(handle);

		_MApi_NJPD_Parser_Write_GroupTable(handle, va);
	}
	if (jpd_api_debug & LEVEL_API_PRINTMEM) {
		njpd_parser_debug(handle, LEVEL_API_PRINTMEM,
				  "print the Group table!!!!!!!!!!!!!!!!\n");
		if (pstTableInfo->bIs3HuffTbl == true)
			NJPD_PrintMem(utTableAddr + NJPD_MEM_SCWGIF_BASE, E_OFFSET96 * E_NUM4);
		else
			NJPD_PrintMem(utTableAddr + NJPD_MEM_SCWGIF_BASE, E_OFFSET64 * E_NUM4);
	}
	return true;
}

static bool _MApi_NJPD_Parser_WriteSymidx(int handle, void *va)
{
	u16 ac_tbl_num_luma = 0, ac_tbl_num_chroma = 0;
	u16 dc_tbl_num_luma = 0, dc_tbl_num_chroma = 0;
	u16 ac_tbl_num_chroma2 = 0;
	u16 dc_tbl_num_chroma2 = 0;
	u16 i = 0;
	u16 u16LumaCnt = 0;
	u16 u16ChromaCnt = 0;
	u16 u16Chroma2Cnt = 0;
	u16 u16MaxCnt;
	NJPD_TableInfo *pstTableInfo;
	NJPD_HuffInfo *pstHuffInfo;
	uintptr_t utTableAddr = (uintptr_t)va;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstTableInfo = &gNJPDParserInfo[handle].table_info;
	pstHuffInfo = gNJPDParserInfo[handle].huff_info;

	if (gNJPDParserInfo[handle].check_DHT == true) {
		dc_tbl_num_luma = pstTableInfo->u16DcTblNumLuma;
		dc_tbl_num_chroma = pstTableInfo->u16DcTblNumChroma;
		ac_tbl_num_luma = pstTableInfo->u16AcTblNumLuma;
		ac_tbl_num_chroma = pstTableInfo->u16AcTblNumChroma;
		dc_tbl_num_chroma2 = pstTableInfo->u16DcTblNumChroma2;
		ac_tbl_num_chroma2 = pstTableInfo->u16AcTblNumChroma2;

		for (i = 0; i < GTABLE_DC_SIZE; i++) {
			if (i < pstTableInfo->u8DcLumaCnt) {
				*(u8 *)(utTableAddr + NJPD_MEM_SYMIDX_BASE + u16LumaCnt * E_OFFSET4)
				    = pstHuffInfo[dc_tbl_num_luma].u8Huff_val[i];
				u16LumaCnt++;
			}
			if (i < pstTableInfo->u8DcChromaCnt) {
				*(u8 *)(utTableAddr + NJPD_MEM_SYMIDX_BASE
					+ u16ChromaCnt * E_OFFSET4 + E_NUM1)
					= pstHuffInfo[dc_tbl_num_chroma].u8Huff_val[i];
				u16ChromaCnt++;
			}
			if (pstTableInfo->bIs3HuffTbl && i < pstTableInfo->u8DcChroma2Cnt) {
				*(u8 *)(utTableAddr + NJPD_MEM_SYMIDX_BASE
					+ u16Chroma2Cnt * E_OFFSET4 + E_NUM2)
					= pstHuffInfo[dc_tbl_num_chroma2].u8Huff_val[i];
				u16Chroma2Cnt++;
			} else if (i < pstTableInfo->u8DcChromaCnt) {
				*(u8 *)(utTableAddr + NJPD_MEM_SYMIDX_BASE
					+ u16Chroma2Cnt * E_OFFSET4 + E_NUM2)
					= pstHuffInfo[dc_tbl_num_chroma].u8Huff_val[i];
				u16Chroma2Cnt++;
			}
		}

		for (i = 0; i < GTABLE_AC_SIZE; i++) {
			if (i < pstTableInfo->u8AcLumaCnt) {
				*(u8 *)(utTableAddr + NJPD_MEM_SYMIDX_BASE + u16LumaCnt * E_OFFSET4)
				    = pstHuffInfo[ac_tbl_num_luma].u8Huff_val[i];
				u16LumaCnt++;
			}
			if (i < pstTableInfo->u8AcChromaCnt) {
				*(u8 *)(utTableAddr + NJPD_MEM_SYMIDX_BASE
					+ u16ChromaCnt * E_OFFSET4 + E_NUM1)
					= pstHuffInfo[ac_tbl_num_chroma].u8Huff_val[i];
				u16ChromaCnt++;
			}
			if (pstTableInfo->bIs3HuffTbl && i < pstTableInfo->u8AcChroma2Cnt) {
				*(u8 *)(utTableAddr + NJPD_MEM_SYMIDX_BASE
					+ u16Chroma2Cnt * E_OFFSET4 + E_NUM2)
					= pstHuffInfo[ac_tbl_num_chroma2].u8Huff_val[i];
				u16Chroma2Cnt++;
			} else if (i < pstTableInfo->u8AcChromaCnt) {
				*(u8 *)(utTableAddr + NJPD_MEM_SYMIDX_BASE
					+ u16Chroma2Cnt * E_OFFSET4 + E_NUM2)
					= pstHuffInfo[ac_tbl_num_chroma].u8Huff_val[i];
				u16Chroma2Cnt++;
			}
		}
	}
	//MsOS_FlushMemory();
	u16MaxCnt = (u16LumaCnt > u16ChromaCnt) ? u16LumaCnt : u16ChromaCnt;
	u16MaxCnt = (u16MaxCnt > u16Chroma2Cnt) ? u16MaxCnt : u16Chroma2Cnt;
	u16MaxCnt = ((u16MaxCnt % E_OFFSET4) == E_NUM2
		     || (u16MaxCnt % E_OFFSET4) == E_NUM3) ?
		     u16MaxCnt : ((u16MaxCnt / E_OFFSET4) * E_OFFSET4 + E_NUM2);
	pstTableInfo->u32HTableSize = u16MaxCnt;


	if (jpd_api_debug & LEVEL_API_PRINTMEM) {
		njpd_parser_debug(handle, LEVEL_API_PRINTMEM,
				  "print the Huffman table!!!!!!!!!!!!!!!!\n");
		NJPD_PrintMem(utTableAddr + NJPD_MEM_SYMIDX_BASE, u16MaxCnt * E_OFFSET4);
	}
	return true;
}

static bool _MApi_NJPD_Parser_WriteIQTbl(int handle, void *va)
{
	u8 i;
	u8 com_num = E_COM0;
	u8 u8Value;
	u8 comp[NJPD_MAXCOMPONENTS];
	NJPD_TableInfo *pstTableInfo;
	NJPD_SOFInfo *pstSOFInfo;
	NJPD_QuantTbl *pstQuantTable;
	uintptr_t utTableAddr = (uintptr_t)va;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return false;
	}
	pstTableInfo = &gNJPDParserInfo[handle].table_info;
	pstSOFInfo = &gNJPDParserInfo[handle].sof_info;
	pstQuantTable = gNJPDParserInfo[handle].quant_table;

	if (gNJPDParserInfo[handle].check_DQT) {
		// Calculate how many valid quantization tables
		memset((void *)comp, 0, NJPD_MAXCOMPONENTS);
		for (i = 0; i < pstSOFInfo->u8ComponentsInFrame; i++)
			comp[pstSOFInfo->u8ComponentQuant[i]] = 1;

		for (i = 0; i < NJPD_MAXCOMPONENTS; i++) {
			if (comp[i] == 1)
				com_num++;
		}

		for (i = 0; i < E_OFFSET64; i++) {
			if (com_num == E_COM1) {
				*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4) =
				    pstQuantTable[pstSOFInfo->u8ComponentQuant[E_COM0]].s16Value
				    [_u8Jpeg_zigzag_order[i]] & E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4 + E_NUM1) =
				    pstQuantTable[pstSOFInfo->u8ComponentQuant[E_COM0]].s16Value
				    [_u8Jpeg_zigzag_order[i]] & E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4 + E_NUM2) =
				    pstQuantTable[pstSOFInfo->u8ComponentQuant[E_COM0]].s16Value
				    [_u8Jpeg_zigzag_order[i]] & E_MARKER_FF;
			} else if (com_num == E_COM2) {
				*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4) =
				    pstQuantTable[pstSOFInfo->u8ComponentQuant[E_COM0]].s16Value
				    [_u8Jpeg_zigzag_order[i]] & E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4 + E_NUM1) =
				    pstQuantTable[pstSOFInfo->u8ComponentQuant[E_COM1]].s16Value
				    [_u8Jpeg_zigzag_order[i]] & E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4 + E_NUM2) =
				    pstQuantTable[pstSOFInfo->u8ComponentQuant[E_COM1]].s16Value
				    [_u8Jpeg_zigzag_order[i]] & E_MARKER_FF;
			} else if (com_num == E_COM3) {
				*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4) =
				    pstQuantTable[pstSOFInfo->u8ComponentQuant[E_COM0]].s16Value
				    [_u8Jpeg_zigzag_order[i]] & E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4 + E_NUM1) =
				    pstQuantTable[pstSOFInfo->u8ComponentQuant[E_COM1]].s16Value
				    [_u8Jpeg_zigzag_order[i]] & E_MARKER_FF;
				*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4 + E_NUM2) =
				    pstQuantTable[pstSOFInfo->u8ComponentQuant[E_COM2]].s16Value
				    [_u8Jpeg_zigzag_order[i]] & E_MARKER_FF;
			}
		}
	} else {

		for (i = 0; i < E_OFFSET64; i++) {
			u8Value = g16IQ_TBL_NJPD[i] & E_MARKER_FF;
			*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE + i * E_OFFSET4) = u8Value;
		}
		for (i = E_OFFSET64; i < QTABLE_SIZE; i++) {
			u8Value = g16IQ_TBL_NJPD[i] & E_MARKER_FF;
			*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE
				+ (i - E_OFFSET64) * E_OFFSET4 + E_NUM1) = u8Value;
			*(u8 *)(utTableAddr + NJPD_MEM_QTBL_BASE
				+ (i - E_OFFSET64) * E_OFFSET4 + E_NUM2) = u8Value;
		}
	}

	if (com_num == E_COM3)
		pstTableInfo->bDiffQTable = true;

	//MsOS_FlushMemory();
	if (jpd_api_debug & LEVEL_API_PRINTMEM) {
		njpd_parser_debug(handle, LEVEL_API_PRINTMEM,
				  "print the Quantization table!!!!!!!!!!!!!!!!\n");
		NJPD_PrintMem(utTableAddr + NJPD_MEM_QTBL_BASE, E_OFFSET64 * E_OFFSET4);
	}
	return true;
}

static void _Mapi_NJPD_Parser_Reset(int handle)
{
	NJPD_Parser_Info *pstParserInfo;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return;
	}
	pstParserInfo = &gNJPDParserInfo[handle];

	pstParserInfo->read_point = NULL;
	pstParserInfo->data_fill = 0;
	pstParserInfo->data_left = 0;
	pstParserInfo->last_marker_offset = 0;
	pstParserInfo->out_buf_size = 0;
	pstParserInfo->is_progressive = false;
	pstParserInfo->check_SOI = false;
	pstParserInfo->check_DHT = false;
	pstParserInfo->check_DQT = false;
	pstParserInfo->check_SOS = false;
	pstParserInfo->need_more_data = false;
	NJPD_memset(&pstParserInfo->table_info, 0, sizeof(NJPD_TableInfo));
	NJPD_memset(&pstParserInfo->huff_info[0], 0, sizeof(NJPD_HuffInfo) * NJPD_MAXHUFFTABLES);
	NJPD_memset(&pstParserInfo->quant_table[0], 0, sizeof(NJPD_QuantTbl) * NJPD_MAXQUANTTABLES);
	NJPD_memset(&pstParserInfo->sof_info, 0, sizeof(NJPD_SOFInfo));
	NJPD_memset(&pstParserInfo->sos_info, 0, sizeof(NJPD_SOSInfo));
	if (gNJPDParserInfo[handle].last_buffer_data != NULL) {
		NJPD_FREE(gNJPDParserInfo[handle].last_buffer_data,
			  gNJPDParserInfo[handle].last_buffer_length);
		gNJPDParserInfo[handle].last_buffer_data = NULL;
		gNJPDParserInfo[handle].has_last_buffer = false;
	}
}

static bool _Mapi_NJPD_Decoder_Check_ReadList(void)
{
	NJPD_API_Buf *pstBuf;
	NJPD_Read_Buffer_Node *pstNode;
	u32 u32BufSize;

	njpd_debug(LEVEL_API_TRACE, "enter");
	if ((gNJPDDecoderInfo.buffer_empty[gNJPDDecoderInfo.next_index] == true)
	    && (gNJPDDecoderInfo.read_list != NULL)) {
		pstNode = gNJPDDecoderInfo.read_list;
		pstBuf = &pstNode->buf;
		if (pstBuf == NULL) {
			njpd_err("node with null buffer");
			return false;
		}
		u32BufSize = pstBuf->filled_length - pstBuf->offset;
		u32BufSize = NJPD_Align(u32BufSize, NJPD_ADDR_ALIGN);
		if (MDrv_NJPD_SetReadBuffer(pstBuf->pa + pstBuf->offset, u32BufSize,
		    gNJPDDecoderInfo.next_index) == true) {
			gNJPDDecoderInfo.buffer_empty[gNJPDDecoderInfo.next_index] = false;
			MDrv_NJPD_SetMRC_Valid(gNJPDDecoderInfo.next_index);
			gNJPDDecoderInfo.next_index ^= 1;
			gNJPDDecoderInfo.read_list = gNJPDDecoderInfo.read_list->next;
			NJPD_FREE(pstNode, sizeof(NJPD_Read_Buffer_Node));
		} else {
			njpd_err("set buffer fail");
			return false;
		}

	} else {
		// do nothing
	}
	return true;
}

static void _Mapi_NJPD_Decoder_Free_ReadList(void)
{
	NJPD_Read_Buffer_Node *pstNode;

	njpd_debug(LEVEL_API_TRACE, "enter");
	while (gNJPDDecoderInfo.read_list != NULL) {
		pstNode = gNJPDDecoderInfo.read_list;
		gNJPDDecoderInfo.read_list = gNJPDDecoderInfo.read_list->next;
		NJPD_FREE(pstNode, sizeof(NJPD_Read_Buffer_Node));
	}
}

static bool _MApi_NJPD_Decoder_Check_NoMoreData(void)
{
	njpd_debug(LEVEL_API_TRACE, "enter");
	if (gNJPDDecoderInfo.is_progressive) {
		//progressive only have 1 input buffer in decode state
		//last buffer will trigger in writeRLE
		return false;
	}
	njpd_debug(LEVEL_API_DEBUG, "no_more_data(%d)", gNJPDDecoderInfo.no_more_data);
	njpd_debug(LEVEL_API_DEBUG, "buffer_empty[0](%d), buffer_empty[1](%d)",
		gNJPDDecoderInfo.buffer_empty[0], gNJPDDecoderInfo.buffer_empty[1]);
	if ((gNJPDDecoderInfo.no_more_data == true) &&
	    (gNJPDDecoderInfo.read_list == NULL) &&
	    (gNJPDDecoderInfo.buffer_empty[0] == true) &&
	    (gNJPDDecoderInfo.buffer_empty[1] == true)) {
		njpd_debug(LEVEL_API_DEBUG, "send no more data");
		MDrv_NJPD_ReadLastBuffer();
		return true;
	}
	return false;
}

static void _Mapi_NJPD_Decoder_Reset(void)
{
	njpd_debug(LEVEL_API_TRACE, "enter");
	gNJPDDecoderInfo.buffer_empty[0] = true;
	gNJPDDecoderInfo.buffer_empty[1] = true;
	gNJPDDecoderInfo.next_index = 0;
	gNJPDDecoderInfo.consume_num = 0;
	gNJPDDecoderInfo.read_list = NULL;
	gNJPDDecoderInfo.read_list_tail = NULL;
	memset(gNJPDContext.isrEvents, 0, ISR_EVENT_NUM);
}

static void _MApi_NJPD_Decoder_PrintError(u16 u16EventFlag)
{
	if (u16EventFlag & NJPD_EVENT_WRITE_PROTECT)
		njpd_err("Write protect!!");

	if (u16EventFlag & NJPD_EVENT_RES_MARKER_ERR)
		njpd_err("restart marker error!!");

	if (u16EventFlag & NJPD_EVENT_RMID_ERR)
		njpd_err("restart marker index disorder error!!");

	if (u16EventFlag & NJPD_EVENT_MINICODE_ERR)
		njpd_err("mini-code error!!");

	if (u16EventFlag & NJPD_EVENT_INV_SCAN_ERR)
		njpd_err("inverse scan error!!");

	if (u16EventFlag & NJPD_EVENT_DATA_LOSS_ERR)
		njpd_err("data loss error!!");

	if (u16EventFlag & NJPD_EVENT_HUFF_TABLE_ERR)
		njpd_err("Huffman table error!!");

	if (jpd_api_debug & LEVEL_API_PRINTMEM)
		MDrv_NJPD_Debug();

	if (u16EventFlag & (NJPD_EVENT_RES_MARKER_ERR | NJPD_EVENT_RMID_ERR))
		njpd_err("rst error!!");

	if (u16EventFlag & (NJPD_EVENT_MINICODE_ERR | NJPD_EVENT_INV_SCAN_ERR
			| NJPD_EVENT_DATA_LOSS_ERR | NJPD_EVENT_HUFF_TABLE_ERR))
		njpd_err("bitstream error!!");
}

static NJPD_API_Result _MApi_NJPD_Decoder_CheckDataLoss(bool bNoMoreData)
{
	u8 u8CheckCnt;
	u32 u32CurrMWC;

	njpd_debug(LEVEL_API_TRACE, "enter");
	if (bNoMoreData && NJPD_ISR_UnhandledEvent() == false) {
		njpd_debug(LEVEL_API_DEBUG, "bNoMoreData is true and no unhandle event");
		for (u8CheckCnt = 1; u8CheckCnt <= CHECK_ERR_CNT; u8CheckCnt++) {
			u32CurrMWC = MDrv_NJPD_GetCurMWCAddr();
			if (NJPD_ISR_UnhandledEvent() == true) {
				njpd_debug(LEVEL_API_DEBUG, "unhandle event coming, end loop");
				break;
			}
			else if (u32CurrMWC != MDrv_NJPD_GetCurMWCAddr())
				u8CheckCnt -= 1;
			else if (NJPD_ISR_UnhandledEvent() == false
				&& u32CurrMWC == MDrv_NJPD_GetCurMWCAddr()
				&& u8CheckCnt == CHECK_ERR_CNT) {
				_MApi_NJPD_Decoder_PrintError(NJPD_EVENT_DATA_LOSS_ERR);
				return E_NJPD_API_FAILED;
			}
			MsOS_DelayTaskUs(DELAY_10us);
		}
		njpd_debug(LEVEL_API_DEBUG, "u8CheckCnt = %u", u8CheckCnt);
	}
	return E_NJPD_API_OKAY;
}


#if SUPPORT_PROGRESSIVE
static bool _MApi_NJPD_Decoder_WriteRLE(JPEG_SVLD_Info *pVld, u8 bDecodeNow)
{
	u8 *mrc_buffer = (u8 *) (uintptr_t) gNJPDDecoderInfo.progressive_read_buffer.va;
	size_t mrc_size = (gNJPDDecoderInfo.progressive_read_buffer.size /
						NJPD_PING_PONG_BUFFER_COUNT);
	NJPD_ProgInfo *pstProgInfo = &gNJPDDecoderInfo.prog_info;

	if (gNJPDDecoderInfo.next_index == 1)
		mrc_buffer += mrc_size;

	NJPD_memcpy((void *)(mrc_buffer + pstProgInfo->_u32RLEOffset), (void *)pVld, E_NUM4);
	pstProgInfo->_u32RLEOffset += E_NUM4;

	// Check if buffer full
	if ((pstProgInfo->_u32RLEOffset == mrc_size)
	    || (bDecodeNow == true)) {
		njpd_debug(LEVEL_API_DEBUG, "Do RLE, LENG 0x%x, bDecodeNow = %d",
			pstProgInfo->_u32RLEOffset, bDecodeNow);

		if (bDecodeNow == true) {
			njpd_debug(LEVEL_API_DEBUG, "last one bit enable");
			MDrv_NJPD_ReadLastBuffer();
		}
		if (gNJPDDecoderInfo.next_index == 0) {
			MDrv_NJPD_SetReadBuffer(gNJPDDecoderInfo.progressive_read_buffer.pa,
				mrc_size,
				gNJPDDecoderInfo.next_index);
			MDrv_NJPD_SetMRC_Valid(0);
			MsOS_DelayTaskUs(DELAY_100us);
			njpd_debug(LEVEL_API_DEBUG, "decode MRC0");
		} else {
			MDrv_NJPD_SetReadBuffer(gNJPDDecoderInfo.progressive_read_buffer.pa +
				mrc_size,
				mrc_size,
				gNJPDDecoderInfo.next_index);
			MDrv_NJPD_SetMRC_Valid(1);
			MsOS_DelayTaskUs(DELAY_100us);
			njpd_debug(LEVEL_API_DEBUG, "decode MRC1");
		}
		gNJPDDecoderInfo.buffer_empty[gNJPDDecoderInfo.next_index] = false;
		gNJPDDecoderInfo.next_index ^= 1;
		pstProgInfo->_u32RLEOffset = 0;
		gNJPDDecoderInfo.bWriteRLE = true;
	}
	return true;
}
static bool _MApi_NJPD_Decoder_DoRLE(NJPD_BLOCK_TYPE *p, u8 eop, u8 comp_id, u8 BlockInRange)
{
	JPEG_SVLD_Info my_vld;
	u8 counter;
	s16 value;
	u16 run;
	u8 cur_blk;
	NJPD_BLOCK_TYPE predictor;
	NJPD_ProgInfo *pstProgInfo = &gNJPDDecoderInfo.prog_info;

	if (comp_id == 0)
		cur_blk = 1;	// Y
	else if (comp_id == 1)
		cur_blk = E_NUM3;	// U
	else
		cur_blk = E_NUM2;	// V

	predictor = pstProgInfo->_s16dc_pred[cur_blk - 1];

	run = 0;
	my_vld.byte0 = my_vld.byte1 = my_vld.byte2 = my_vld.byte3 = 0;

	//sent DC info
	//my_vld.run = 8;
	if (BlockInRange)	//Current block is within display range.
		my_vld.run = E_NUM8;
	else
		my_vld.run = 0;

	value = (p[0] - predictor);
	my_vld.data = value;

	if (!_MApi_NJPD_Decoder_WriteRLE(&my_vld, false))
		return false;

	if (BlockInRange == false)	//Current block is not within display range.
		return true;

	my_vld.byte0 = my_vld.byte1 = my_vld.byte2 = my_vld.byte3 = 0;

	for (counter = 1; counter < E_NUM64; counter++) {
		if (p[counter] == 0) {
			run++;
		} else {
			while (run > E_NUM15) {
				my_vld.data = 0;
				my_vld.run = E_NUM15;

				if (!_MApi_NJPD_Decoder_WriteRLE(&my_vld, false))
					return false;
				my_vld.byte0 = my_vld.byte1 = my_vld.byte2 = my_vld.byte3 = 0;
				run -= E_NUM16;
			}

			my_vld.data = p[counter];
			my_vld.run = run;

			if (counter == E_NUM63 && eop && p[E_NUM63] != 0) {
				if (!_MApi_NJPD_Decoder_WriteRLE(&my_vld, true))
					return false;

				pstProgInfo->_s16dc_pred[cur_blk - 1] = p[0]; //update predictor
				return true;
			}

			if (!_MApi_NJPD_Decoder_WriteRLE(&my_vld, false))
				return false;

			my_vld.byte0 = my_vld.byte1 = my_vld.byte2 = my_vld.byte3 = 0;
			run = 0;
		}
	}

	counter = E_NUM63;

	if (p[counter] == 0) {
		my_vld.data = p[counter];
		my_vld.EOB = 1;
		my_vld.run = 0;
		if (eop) {
			if (!_MApi_NJPD_Decoder_WriteRLE(&my_vld, true))
				return false;
		} else {
			if (!_MApi_NJPD_Decoder_WriteRLE(&my_vld, false))
				return false;
		}
	}

	pstProgInfo->_s16dc_pred[cur_blk - 1] = p[0]; //update predictor
	return true;
}
static bool _MApi_NJPD_Decoder_LoadNextRow(void)
{
	NJPD_BLOCK_TYPE p[E_NUM64];
	//u8 EOF_Flag = false;
	u16 mcu_row, mcu_block;
	u8 component_num, component_id;
	u16 block_x_mcu[NJPD_MAXCOMPONENTS];
	u16 block_x_mcu_ofs = 0, block_y_mcu_ofs = 0;
	NJPD_BLOCK_TYPE *pAC;
	NJPD_BLOCK_TYPE *pDC;
	u8 u8i;
	NJPD_ProgInfo *pstProgInfo = &gNJPDDecoderInfo.prog_info;

	NJPD_memset((void *)block_x_mcu, 0, NJPD_MAXCOMPONENTS * sizeof(u16));

	for (mcu_row = 0; mcu_row < pstProgInfo->_u16Mcus_per_row; mcu_row++) {

		for (mcu_block = 0; mcu_block < pstProgInfo->_u8Blocks_per_mcu; mcu_block++) {

			component_id = pstProgInfo->_u8Mcu_org[mcu_block];

			pAC =
			    _Mapi_NJPD_Coeff_Buf_Getp(pstProgInfo->_AC_Coeffs[component_id],
						block_x_mcu[component_id] + block_x_mcu_ofs,
						pstProgInfo->_u32Block_y_mcu[component_id] +
						block_y_mcu_ofs);
			if (pAC == NULL) {
				njpd_err("line: %d", __LINE__);
				return false;
			}

			pDC =
			    _Mapi_NJPD_Coeff_Buf_Getp(pstProgInfo->_DC_Coeffs[component_id],
						block_x_mcu[component_id] + block_x_mcu_ofs,
						pstProgInfo->_u32Block_y_mcu[component_id] +
						block_y_mcu_ofs);
			if (pDC == NULL) {
				njpd_err("line: %d", __LINE__);
				return false;
			}
			p[0] = pDC[0];

			for (u8i = 1; u8i < E_NUM64; u8i++)
				p[u8i] = pAC[u8i];

			if (1/*pNJPEGContext->_Progressive_ROI_flag == FALSE*/) {
				if ((mcu_block == (pstProgInfo->_u8Blocks_per_mcu - 1))
				    && (mcu_row == (pstProgInfo->_u16Mcus_per_row - 1))
				    && (pstProgInfo->_u16Total_lines_left ==
					pstProgInfo->gu8Max_mcu_y_size)) {
					if (!_MApi_NJPD_Decoder_DoRLE(p, true, component_id, true))
						return false;
				} else {
					if (!_MApi_NJPD_Decoder_DoRLE(p, false, component_id, true))
						return false;
				}
			}
			///TODO:ROI
			//else if (pNJPEGContext->_u16Total_lines_left
			//	== pNJPEGContext->gu8Max_mcu_y_size) {
			//	if ((mcu_block == (pNJPEGContext->_u8Blocks_per_mcu - 1))
			//		&& ((mcu_row + 2) * pNJPEGContext->gu8Max_mcu_x_size
			//		> pNJPEGContext->ROI_width)
			//		&& EOF_Flag == FALSE) {
			//		EOF_Flag = TRUE;
			//		JPEG_DEBUG_API_MSG("EOF!!!!!No ROI!!!\n");
			//		if (!_MApi_NJPD_Decoder_DoRLE(p, TRUE, component_id, TRUE))
			//			return FALSE;
			//	} else {
			//		if (!_MApi_NJPD_Decoder_DoRLE(p, FALSE, component_id, TRUE))
			//			return FALSE;
			//	}
			//} else if ((mcu_row + 1) * pNJPEGContext->gu8Max_mcu_x_size
			//	<= pNJPEGContext->ROI_width) {
			//	if (!JPEG_do_RLE(p, FALSE, component_id, TRUE))
			//		return FALSE;
			//}

			if (gNJPDDecoderInfo.u8ComponentsInScan == 1) {
				block_x_mcu[component_id]++;
			} else {
				if (++block_x_mcu_ofs ==
				    gNJPDDecoderInfo.u8ComponentHSamp[component_id]) {
					block_x_mcu_ofs = 0;

					if (++block_y_mcu_ofs ==
					    gNJPDDecoderInfo.u8ComponentVSamp[component_id]) {
						block_y_mcu_ofs = 0;

						block_x_mcu[component_id] +=
						    gNJPDDecoderInfo.u8ComponentHSamp[component_id];
					}
				}
			}
		}
	}

	if (gNJPDDecoderInfo.u8ComponentsInScan == 1) {
		pstProgInfo->_u32Block_y_mcu[gNJPDDecoderInfo.u8ComponentList[0]]++;
	} else {
		for (component_num = 0; component_num < gNJPDDecoderInfo.u8ComponentsInScan;
		     component_num++) {
			component_id = gNJPDDecoderInfo.u8ComponentList[component_num];

			pstProgInfo->_u32Block_y_mcu[component_id] +=
			    gNJPDDecoderInfo.u8ComponentVSamp[component_id];
		}
	}
	return true;
}

static NJPD_API_DecodeStatus _MApi_NJPD_Decoder_DecodeProgressive(void)
{
	NJPD_ProgInfo *pstProgInfo = &gNJPDDecoderInfo.prog_info;

	njpd_debug(LEVEL_API_TRACE, "enter");
	gNJPDDecoderInfo.bWriteRLE = false;

	njpd_debug(LEVEL_API_DEBUG, "_u16Mcus_per_row = %d", pstProgInfo->_u16Mcus_per_row);
	njpd_debug(LEVEL_API_DEBUG, "_u16Mcus_per_col = %d", pstProgInfo->_u16Mcus_per_col);
	njpd_debug(LEVEL_API_DEBUG, "_u8Blocks_per_mcu = %d", pstProgInfo->_u8Blocks_per_mcu);
	njpd_debug(LEVEL_API_DEBUG, "gu8Max_mcu_x_size = %d", pstProgInfo->gu8Max_mcu_x_size);
	njpd_debug(LEVEL_API_DEBUG, "gu8Max_mcu_y_size = %d", pstProgInfo->gu8Max_mcu_y_size);

	while ((!gNJPDDecoderInfo.bWriteRLE ||
		gNJPDDecoderInfo.buffer_empty[gNJPDDecoderInfo.next_index]) &&
		(pstProgInfo->_u16Total_lines_left > 0)) {
		njpd_debug(LEVEL_API_TRACE, "total_lines_left = %u",
			pstProgInfo->_u16Total_lines_left);
		if (!_MApi_NJPD_Decoder_LoadNextRow()) {
			njpd_err("_MApi_NJPD_Decoder_LoadNextRow failed");
			return E_NJPD_API_DEC_FAILED;
		}

		pstProgInfo->_u16Total_lines_left -= pstProgInfo->gu8Max_mcu_y_size;
	}

	return E_NJPD_API_DEC_DECODING;
}

static void _Mapi_NJPD_Decoder_Free_ProgressiveBuf(void)
{
	u8 i;
	NJPD_ProgInfo *pstProgInfo = &gNJPDDecoderInfo.prog_info;

	for (i = 0; i <  gNJPDDecoderInfo.u8ComponentsInFrame; i++) {
		_Mapi_NJPD_Coeff_Buf_Free(pstProgInfo->_DC_Coeffs[i],
			((pstProgInfo->gu16Max_mcus_per_row +
			0x1) & ~0x1) * gNJPDDecoderInfo.u8ComponentHSamp[i],
			((pstProgInfo->_u16Max_mcus_per_col +
			0x1) & ~0x1) * gNJPDDecoderInfo.u8ComponentVSamp[i], 1, 1);

		_Mapi_NJPD_Coeff_Buf_Free(pstProgInfo->_AC_Coeffs[i],
			((pstProgInfo->gu16Max_mcus_per_row +
			0x1) & ~0x1) * gNJPDDecoderInfo.u8ComponentHSamp[i],
			((pstProgInfo->_u16Max_mcus_per_col +
			0x1) & ~0x1) * gNJPDDecoderInfo.u8ComponentVSamp[i], E_NUM8, E_NUM8);
	}
}

#endif
//-------------------------------------------------------------------------------------------------
// Common Functions
//-------------------------------------------------------------------------------------------------
NJPD_API_Result MApi_NJPD_Init(NJPD_API_InitParam *pInitParam)
{
	njpd_debug(LEVEL_API_TRACE, "enter");

	if (pInitParam == NULL) {
		njpd_err("parameter is null");
		return E_NJPD_API_FAILED;
	}
	gNJPDContext.jpd_irq = pInitParam->jpd_irq;
	gNJPDContext.stBankOffsets.bank_jpd = pInitParam->jpd_reg_base;
	gNJPDContext.stBankOffsets.bank_jpd_ext = pInitParam->jpd_ext_reg_base;
	gNJPDContext.stBankOffsets.clk_jpd = pInitParam->clk_njpd;
	gNJPDContext.stBankOffsets.clk_njpd2jpd = pInitParam->clk_njpd2jpd;
	gNJPDContext.stBankOffsets.sw_en_smi2jpd = pInitParam->clk_smi2jpd;
	MDrv_NJPD_SetRIUBank(&gNJPDContext.stBankOffsets);
	memset(&gNJPDDecoderInfo, 0, sizeof(NJPD_Decoder_Info));

	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Deinit(void)
{
	njpd_debug(LEVEL_API_TRACE, "enter");
	return E_NJPD_API_OKAY;
}

//-------------------------------------------------------------------------------------------------
// Parser Functions
//-------------------------------------------------------------------------------------------------
NJPD_API_Result MApi_NJPD_Parser_Open(int *pHandle, NJPD_API_ParserInitParam *pParserInitParam)
{
	njpd_debug(LEVEL_API_TRACE, "enter");
	if ((pHandle == NULL) || (pParserInitParam == NULL)) {
		njpd_err("parameter is null");
		return E_NJPD_API_FAILED;
	}

	*pHandle = _MApi_NJPD_Parser_Get_Free_Handle();
	if (*pHandle < 0) {
		njpd_err("failed to get free handle");
		return E_NJPD_API_FAILED;
	}

	NJPD_memset(&gNJPDParserInfo[*pHandle], 0, sizeof(NJPD_Parser_Info));
	gNJPDParserInfo[*pHandle].active = true;
	gNJPDParserInfo[*pHandle].allocBufFunc = pParserInitParam->pAllocBufFunc;
	gNJPDParserInfo[*pHandle].freeBufFunc = pParserInitParam->pFreeBufFunc;
	gNJPDParserInfo[*pHandle].priv = pParserInitParam->priv;

	njpd_debug(LEVEL_API_DEBUG, "open success with handle:%d", *pHandle);
	MDrv_NJPD_SetDbgLevel(jpd_api_debug);
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Parser_SetFormat(int handle, NJPD_API_Format eFormat)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	gNJPDParserInfo[handle].format = eFormat;
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Parser_SetIommu(int handle, bool bIommu)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	gNJPDParserInfo[handle].is_iommu = bIommu;
	return E_NJPD_API_OKAY;
}

// ROI -> DownScale
NJPD_API_Result MApi_NJPD_Parser_SetROI(int handle, NJPD_API_Rect ROI_rect)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	gNJPDParserInfo[handle].roi_rect = ROI_rect;
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Parser_SetDownScale(int handle, NJPD_API_Pic pic)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	gNJPDParserInfo[handle].down_scale = pic;
	gNJPDParserInfo[handle].has_down_scale = true;
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Parser_SetMaxResolution(int handle, NJPD_API_Pic pic)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	gNJPDParserInfo[handle].max_res = pic;
	gNJPDParserInfo[handle].has_max_res = true;
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Parser_SetOutputFormat(int handle, u8 format)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	gNJPDParserInfo[handle].out_format = format;
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Parser_SetVerificationMode(int handle, u8 mode)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	gNJPDParserInfo[handle].vry_mode = mode;
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Parser_Reset(int handle)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	_Mapi_NJPD_Parser_Reset(handle);

	return E_NJPD_API_OKAY;
}

NJPD_API_ParserResult MApi_NJPD_Parser_ParseHeader(int handle, NJPD_API_Buf buf,
						   NJPD_API_DecCmd *pDecCmd)
{
	NJPD_API_ParserResult ret;
	size_t sztTempLength, sztLastLength;
	u32 u32InterBufLength;
	void *pTempData;

	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (buf.va == NULL || pDecCmd == NULL) {
		njpd_parser_err(handle, "parameter is null");
		return E_NJPD_API_HEADER_INVALID;
	}
	if (handle >= MAX_PARSER_COUNT || handle < 0) {
		njpd_parser_err(handle, "handle %d is invalid", handle);
		return E_NJPD_API_HEADER_INVALID;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_HEADER_INVALID;
	}

	ret = _MApi_NJPD_Parser_ParseHeader(handle, buf);

	if (ret == E_NJPD_API_HEADER_INVALID) {
		// error reset
	} else if (ret == E_NJPD_API_HEADER_PARTIAL) {
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "partial done, has_last_buffer = %d",
			  gNJPDParserInfo[handle].has_last_buffer);
		if (gNJPDParserInfo[handle].has_last_buffer == true) {
			sztLastLength =
			    gNJPDParserInfo[handle].data_fill -
			    gNJPDParserInfo[handle].last_marker_offset;
			sztTempLength = sztLastLength + (buf.filled_length - buf.offset);
			pTempData = (void *)NJPD_MALLOC(sztTempLength);
			if (pTempData == NULL) {
				njpd_parser_err(handle, "alloc temp buffer failed");
				return E_NJPD_API_HEADER_INVALID;
			}

			memcpy(pTempData, gNJPDParserInfo[handle].last_buffer_data +
			       gNJPDParserInfo[handle].last_marker_offset, sztLastLength);
			memcpy(pTempData + sztLastLength, (buf.va + buf.offset),
					(buf.filled_length - buf.offset));

			NJPD_FREE(gNJPDParserInfo[handle].last_buffer_data,
				  gNJPDParserInfo[handle].last_buffer_length);
			gNJPDParserInfo[handle].last_buffer_data = pTempData;
			gNJPDParserInfo[handle].last_buffer_length = sztTempLength;
			njpd_parser_debug(handle, LEVEL_API_DEBUG, "sztTempLength = 0x%zx",
				sztTempLength);
			njpd_parser_debug(handle, LEVEL_API_DEBUG, "data_fill = 0x%zx",
				gNJPDParserInfo[handle].data_fill);
			njpd_parser_debug(handle, LEVEL_API_DEBUG, "last_marker_offset = 0x%zx",
				gNJPDParserInfo[handle].last_marker_offset);
		} else {
			gNJPDParserInfo[handle].last_buffer_length =
			    gNJPDParserInfo[handle].data_fill -
			    gNJPDParserInfo[handle].last_marker_offset;
			gNJPDParserInfo[handle].last_buffer_data =
			    (void *)NJPD_MALLOC(gNJPDParserInfo[handle].last_buffer_length);
			if (gNJPDParserInfo[handle].last_buffer_data == NULL) {
				njpd_parser_err(handle, "alloc last buffer failed");
				return E_NJPD_API_HEADER_INVALID;
			}
			memcpy(gNJPDParserInfo[handle].last_buffer_data,
			       buf.va + gNJPDParserInfo[handle].last_marker_offset,
			       gNJPDParserInfo[handle].last_buffer_length);
			njpd_parser_debug(handle, LEVEL_API_DEBUG, "last_buffer_length = 0x%zx",
				gNJPDParserInfo[handle].last_buffer_length);
			njpd_parser_debug(handle, LEVEL_API_DEBUG, "data_fill = 0x%zx",
				gNJPDParserInfo[handle].data_fill);
			njpd_parser_debug(handle, LEVEL_API_DEBUG, "last_marker_offset = 0x%zx",
				gNJPDParserInfo[handle].last_marker_offset);
		}
		gNJPDParserInfo[handle].has_last_buffer = true;
		gNJPDParserInfo[handle].last_buffer_used = false;
		// continue
	} else {
		pDecCmd->mjpeg = (gNJPDParserInfo[handle].format == 1) ? 1 : 0;
		pDecCmd->iommu = gNJPDParserInfo[handle].is_iommu;
		pDecCmd->progressive = gNJPDParserInfo[handle].is_progressive;
		pDecCmd->format = gNJPDParserInfo[handle].out_format;
		pDecCmd->mode = gNJPDParserInfo[handle].vry_mode;
		pDecCmd->width = gNJPDParserInfo[handle].sof_info.u16ImageXSize;
		pDecCmd->height = gNJPDParserInfo[handle].sof_info.u16ImageYSize;
		pDecCmd->eFactor = gNJPDParserInfo[handle].scale_factor;
		pDecCmd->ROI_rect = gNJPDParserInfo[handle].roi_rect;
		pDecCmd->read_buffer = buf;

		u32InterBufLength = NJPD_MEM_TBL_TOTAL_SIZE;
		if (!gNJPDParserInfo[handle].is_progressive) {
			//baseline set offset to end of SOS, HW need to read data
			//progressive HW don't need input data, keep origin offset
			pDecCmd->read_buffer.offset = gNJPDParserInfo[handle].last_marker_offset;
		}
#if SUPPORT_PROGRESSIVE
		if (gNJPDParserInfo[handle].is_progressive) {
			//add progressive info size and buffer size for HW
			u32InterBufLength += NJPD_MEM_PROGRESSIVE_INFO_SIZE;
			u32InterBufLength += NJPD_MEM_PROGRESSIVE_BUF_SIZE;
		}
#endif

		if (gNJPDParserInfo[handle].allocBufFunc(u32InterBufLength,
			&pDecCmd->internal_buffer,
			gNJPDParserInfo[handle].priv) == E_NJPD_API_FAILED) {
			njpd_parser_err(handle, "handle %d allocate inter buf failed", handle);
			return E_NJPD_API_HEADER_INVALID;
		}

		NJPD_memset(pDecCmd->internal_buffer.va, 0, u32InterBufLength);
		if (_MApi_NJPD_Parser_WriteGrpinf(handle, pDecCmd->internal_buffer.va) == false) {
			njpd_parser_err(handle, "handle %d write g table failed", handle);
			return E_NJPD_API_HEADER_INVALID;
		}
		if (_MApi_NJPD_Parser_WriteSymidx(handle, pDecCmd->internal_buffer.va) == false) {
			njpd_parser_err(handle, "handle %d write h table failed", handle);
			return E_NJPD_API_HEADER_INVALID;
		}
		if (_MApi_NJPD_Parser_WriteIQTbl(handle, pDecCmd->internal_buffer.va) == false) {
			njpd_parser_err(handle, "handle %d write q table failed", handle);
			return E_NJPD_API_HEADER_INVALID;
		}

		pDecCmd->table_info.g_table_pa = pDecCmd->internal_buffer.pa + NJPD_MEM_SCWGIF_BASE;
		pDecCmd->table_info.h_table_pa = pDecCmd->internal_buffer.pa + NJPD_MEM_SYMIDX_BASE;
		pDecCmd->table_info.h_table_size = gNJPDParserInfo[handle].table_info.u32HTableSize;
		pDecCmd->table_info.diff_h_table = gNJPDParserInfo[handle].table_info.bIs3HuffTbl;
		pDecCmd->table_info.q_table_pa = pDecCmd->internal_buffer.pa + NJPD_MEM_QTBL_BASE;
		pDecCmd->table_info.diff_q_table = gNJPDParserInfo[handle].table_info.bDiffQTable;
		pDecCmd->table_info.restart_interval =
		    gNJPDParserInfo[handle].table_info.u16RestartInterval;
		pDecCmd->table_info.comp_v_samp =
		    gNJPDParserInfo[handle].sof_info.u8ComponentVSamp[0];
		pDecCmd->table_info.comp_h_samp =
		    gNJPDParserInfo[handle].sof_info.u8ComponentHSamp[0];
		pDecCmd->table_info.comps_in_frame =
		    gNJPDParserInfo[handle].sof_info.u8ComponentsInFrame;

		// for getimageinfo, getbufferinfo
		gNJPDParserInfo[handle].image_info.u16OriginX =
		    gNJPDParserInfo[handle].sof_info.u16OriginalImageXSize;
		gNJPDParserInfo[handle].image_info.u16OriginY =
		    gNJPDParserInfo[handle].sof_info.u16OriginalImageYSize;
		gNJPDParserInfo[handle].image_info.u16DecodeAlignX =
		    gNJPDParserInfo[handle].output_res.width;
		gNJPDParserInfo[handle].image_info.u16DecodeAlignY =
		    gNJPDParserInfo[handle].output_res.height;

		njpd_parser_debug(handle, LEVEL_API_DEBUG, "mjpeg:%d, iommu:%d, format:%d, mode:%d",
				  pDecCmd->mjpeg, pDecCmd->iommu, pDecCmd->format, pDecCmd->mode);
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "width:%d, height:%d",
				  pDecCmd->width, pDecCmd->height);
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "is_progressive:%d",
				  pDecCmd->progressive);
		njpd_parser_debug(handle, LEVEL_API_DEBUG,
				  "factor:%d, ROIrect x:%d, y:%d, width:%d, height:%d",
				  pDecCmd->eFactor, pDecCmd->ROI_rect.x, pDecCmd->ROI_rect.y,
				  pDecCmd->ROI_rect.width, pDecCmd->ROI_rect.height);
		njpd_parser_debug(handle, LEVEL_API_DEBUG,
				  "read buffer size:0x%zx(0x%zx), offset:0x%zx, iova:%pad, va:%p",
				  pDecCmd->read_buffer.filled_length, pDecCmd->read_buffer.size,
				  pDecCmd->read_buffer.offset, &pDecCmd->read_buffer.pa,
				  pDecCmd->read_buffer.va);
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "h table size:0x%zx, iova:%pad, diff:%d",
				  pDecCmd->table_info.h_table_size, &pDecCmd->table_info.h_table_pa,
				  pDecCmd->table_info.diff_h_table);
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "g table iova:%pad",
				  &pDecCmd->table_info.g_table_pa);
		njpd_parser_debug(handle, LEVEL_API_DEBUG, "q table iova:%pad, diff:%d",
				  &pDecCmd->table_info.q_table_pa,
				  pDecCmd->table_info.diff_q_table);
#if SUPPORT_PROGRESSIVE
		if (gNJPDParserInfo[handle].is_progressive) {
			pDecCmd->prog_info_buffer_offset = NJPD_MEM_TBL_TOTAL_SIZE;

			if (sizeof(NJPD_ProgInfo) > NJPD_MEM_PROGRESSIVE_INFO_SIZE) {
				njpd_parser_err(handle, "info size error");
				return E_NJPD_API_HEADER_INVALID;
			}
			memcpy(pDecCmd->internal_buffer.va + pDecCmd->prog_info_buffer_offset,
			       &gNJPDParserInfo[handle].prog_info, sizeof(NJPD_ProgInfo));
			//pDecCmd->prog_info = &gNJPDParserInfo[handle].prog_info;
			pDecCmd->u8ComponentsInScan =
				gNJPDParserInfo[handle].sos_info.u8ComponentsInScan;
			pDecCmd->u8ComponentsInFrame =
				gNJPDParserInfo[handle].sof_info.u8ComponentsInFrame;
			memcpy(pDecCmd->u8ComponentList,
			       gNJPDParserInfo[handle].sos_info.u8ComponentList,
			       sizeof(u8) * NJPD_MAXCOMPSINSCAN);
			memcpy(pDecCmd->u8ComponentHSamp,
			       gNJPDParserInfo[handle].sof_info.u8ComponentHSamp,
			       sizeof(u8) * NJPD_MAXCOMPONENTS);
			memcpy(pDecCmd->u8ComponentVSamp,
			       gNJPDParserInfo[handle].sof_info.u8ComponentVSamp,
			       sizeof(u8) * NJPD_MAXCOMPONENTS);

			pDecCmd->progressive_read_buffer_offset =
				NJPD_MEM_TBL_TOTAL_SIZE + NJPD_MEM_PROGRESSIVE_INFO_SIZE;

		}
#endif
	}
	return ret;
}

NJPD_API_Result MApi_NJPD_Parser_GetImageInfo(int handle, NJPD_API_ParserImageInfo *pImageInfo)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	pImageInfo->width = gNJPDParserInfo[handle].image_info.u16OriginX;
	pImageInfo->height = gNJPDParserInfo[handle].image_info.u16OriginY;

	njpd_parser_debug(handle, LEVEL_API_DEBUG, "width:%d, height:%d",
			  pImageInfo->width, pImageInfo->height);
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Parser_GetBufInfo(int handle, NJPD_API_ParserBufInfo *pBufInfo)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	pBufInfo->width = gNJPDParserInfo[handle].image_info.u16DecodeAlignX;
	pBufInfo->height = gNJPDParserInfo[handle].image_info.u16DecodeAlignY;
	pBufInfo->rect = gNJPDParserInfo[handle].rect;
	pBufInfo->write_buf_size = pBufInfo->width * pBufInfo->height * BUF_PARTITION;

	njpd_parser_debug(handle, LEVEL_API_DEBUG, "width:%d, height:%d, write_buf_size:0x%x",
			  pBufInfo->width, pBufInfo->height, pBufInfo->write_buf_size);
	njpd_parser_debug(handle, LEVEL_API_DEBUG, "rect x:%d, y:%d, width:%d, height:%d",
			  pBufInfo->rect.x, pBufInfo->rect.y, pBufInfo->rect.width,
			  pBufInfo->rect.height);
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Parser_Close(int handle)
{
	njpd_parser_debug(handle, LEVEL_API_TRACE, "enter");
	if (handle < 0) {
		njpd_parser_err(handle, "handle(%d) is invalid", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].active == false) {
		njpd_parser_err(handle, "handle %d is not active", handle);
		return E_NJPD_API_FAILED;
	}
	if (gNJPDParserInfo[handle].last_buffer_data != NULL) {
		NJPD_FREE(gNJPDParserInfo[handle].last_buffer_data,
			  gNJPDParserInfo[handle].last_buffer_length);
		gNJPDParserInfo[handle].last_buffer_data = NULL;
	}

	gNJPDParserInfo[handle].active = false;
	return E_NJPD_API_OKAY;
}

//-------------------------------------------------------------------------------------------------
// Decoder Functions
//-------------------------------------------------------------------------------------------------
NJPD_API_Result MApi_NJPD_Decoder_Start(NJPD_API_DecCmd *pDecCmd, NJPD_API_Buf write_buf)
{
	u8 Y_VSF = pDecCmd->table_info.comp_v_samp;
	u8 Y_HSF = pDecCmd->table_info.comp_h_samp;
	u8 bUV_en;
	u16 reg_value;
	///TODO:check output buffer enough
	NJPD_BufCfg structBuf = {0};

	njpd_debug(LEVEL_API_TRACE, "enter");

	MDrv_NJPD_PowerOn();
	MDrv_NJPD_Rst();
	if (pDecCmd->format != E_NJPD_OUTPUT_ORIGINAL)
		MDrv_NJPD_SetOutputFormat(0, pDecCmd->format);
	if (pDecCmd->mode == E_NJPD_IBUF_BURST_LENGTH)
		MDrv_NJPD_SetIBufReadLength(NJPD_VRY_IBUF_L, NJPD_VRY_IBUF_H);

	_Mapi_NJPD_Decoder_Reset();
	if (pDecCmd->table_info.comps_in_frame > 1)
		bUV_en = true;
	else
		bUV_en = false;

	if (pDecCmd->progressive == false) {
		//align 16 and check size
		structBuf.u32MRCBufSize = pDecCmd->read_buffer.filled_length;
		structBuf.u32MRCBufSize = NJPD_Align(structBuf.u32MRCBufSize, NJPD_ADDR_ALIGN);

		structBuf.u64MRCBufAddr = pDecCmd->read_buffer.pa;
		structBuf.u32MRCBufOffset = pDecCmd->read_buffer.offset;
	} else {
#if SUPPORT_PROGRESSIVE
		gNJPDDecoderInfo.progressive_read_buffer.va =
			pDecCmd->internal_buffer.va + pDecCmd->progressive_read_buffer_offset;
		gNJPDDecoderInfo.progressive_read_buffer.pa =
			pDecCmd->internal_buffer.pa + pDecCmd->progressive_read_buffer_offset;
		gNJPDDecoderInfo.progressive_read_buffer.size = NJPD_MEM_PROGRESSIVE_BUF_SIZE;

		structBuf.u32MRCBufSize = gNJPDDecoderInfo.progressive_read_buffer.size;

		structBuf.u64MRCBufAddr = gNJPDDecoderInfo.progressive_read_buffer.pa;
		structBuf.u32MRCBufOffset = 0;
#endif
	}

	structBuf.u64MWCBufAddr = write_buf.pa;
	structBuf.u16MWCBufLineNum = MWC_LINE_NUM;
	//mrc, mwc
	MDrv_NJPD_InitBuf(structBuf);
	gNJPDDecoderInfo.buffer_empty[0] = false;
	gNJPDDecoderInfo.buffer_empty[1] = true;
	gNJPDDecoderInfo.next_index = 1;
	gNJPDDecoderInfo.read_list = NULL;
	gNJPDDecoderInfo.read_list_tail = NULL;
	gNJPDDecoderInfo.no_more_data = false;

	MDrv_NJPD_SetPicDimension(pDecCmd->width, pDecCmd->height);

	if (pDecCmd->mode != E_NJPD_NO_RESET_TABLE) {
		if (pDecCmd->progressive == false) {
			MDrv_NJPD_SetDifferentHTable(pDecCmd->table_info.diff_h_table);
			MDrv_NJPD_GTable_Rst(1);
			//G table
			MDrv_NJPD_GTable_Reload_Enable(1);
			MDrv_NJPD_SetGTableStartAddr(pDecCmd->table_info.g_table_pa);
			//H
			MDrv_NJPD_HTable_Reload_Enable(1);
			MDrv_NJPD_SetHTableStartAddr(pDecCmd->table_info.h_table_pa);
			MDrv_NJPD_SetHTableSize(pDecCmd->table_info.h_table_size);
		}
		//Q
		if (pDecCmd->table_info.diff_q_table == true)
			MDrv_NJPD_SetDifferentQTable(1);
		MDrv_NJPD_QTable_Reload_Enable(1);
		MDrv_NJPD_SetQTableStartAddr(pDecCmd->table_info.q_table_pa);

		MDrv_NJPD_TableLoadingStart();
	}

	reg_value = MDrv_NJPD_Get_GlobalSetting00();
	Y_VSF -= 1;
	Y_HSF -= 1;
	//if (pNJPEGContext->_u16Restart_interval) {
	if (pDecCmd->table_info.restart_interval) {
		njpd_debug(LEVEL_API_ALWAYS,
			   "RST found! Enable NJPD_RST_EN! Restart_interval = %d\n",
			   pDecCmd->table_info.restart_interval);
		//MDrv_Write2Byte(BK_NJPD_RSTINTV, _u16Restart_interval - 1);
		MDrv_NJPD_SetRSTIntv(pDecCmd->table_info.restart_interval);
		MDrv_NJPD_SetScalingDownFactor((NJPD_SCALING_DOWN_FACTOR) pDecCmd->eFactor);
		reg_value |= (NJPD_RST_EN | ((u32) bUV_en) << E_NUM3 | (Y_VSF << E_NUM2) | Y_HSF);
	} else {
		MDrv_NJPD_SetScalingDownFactor((NJPD_SCALING_DOWN_FACTOR) pDecCmd->eFactor);
		reg_value |= (((u32) bUV_en) << E_NUM3 | (Y_VSF << E_NUM2) | Y_HSF);
	}

	if ((pDecCmd->ROI_rect.width != 0) && (pDecCmd->ROI_rect.height != 0)) {
		MDrv_NJPD_SetROI(pDecCmd->ROI_rect.x, pDecCmd->ROI_rect.y,
			pDecCmd->ROI_rect.width, pDecCmd->ROI_rect.height);
		reg_value |= NJPD_ROI_EN;
	}

	if (pDecCmd->progressive == true)
		MDrv_NJPD_SetSoftwareVLD(true);

	// progressive will set valid later
	if (pDecCmd->progressive == false) {
		MDrv_NJPD_SetMRC_Valid(0);
		MsOS_DelayTaskUs(DELAY_100us);
	}
	// enable NJPD decoding
	MDrv_NJPD_EnablePowerSaving();
	MDrv_NJPD_SetAutoProtect(true);

	MDrv_NJPD_SetWPENStartAddr(write_buf.pa);
	MDrv_NJPD_SetWPENEndAddr(write_buf.pa + write_buf.size); // HW align 8

	MDrv_NJPD_Set_GlobalSetting00(reg_value | NJPD_SWRST);

	MDrv_NJPD_DecodeEnable();
	njpd_debug(LEVEL_API_TRACE, "decode enable done");

	gNJPDDecoderInfo.is_progressive = pDecCmd->progressive;
#if SUPPORT_PROGRESSIVE
	if (pDecCmd->progressive) {
		gNJPDDecoderInfo.next_index = 0;
		memcpy(&gNJPDDecoderInfo.prog_info,
			(pDecCmd->internal_buffer.va + pDecCmd->prog_info_buffer_offset),
			sizeof(NJPD_ProgInfo));
		gNJPDDecoderInfo.u8ComponentsInScan = pDecCmd->u8ComponentsInScan;
		gNJPDDecoderInfo.u8ComponentsInFrame = pDecCmd->u8ComponentsInFrame;
		memcpy(gNJPDDecoderInfo.u8ComponentList, pDecCmd->u8ComponentList,
			sizeof(u8) * NJPD_MAXCOMPSINSCAN);
		memcpy(gNJPDDecoderInfo.u8ComponentHSamp, pDecCmd->u8ComponentHSamp,
			sizeof(u8) * NJPD_MAXCOMPONENTS);
		memcpy(gNJPDDecoderInfo.u8ComponentVSamp, pDecCmd->u8ComponentVSamp,
			sizeof(u8) * NJPD_MAXCOMPONENTS);

		if (_MApi_NJPD_Decoder_DecodeProgressive() == E_NJPD_API_DEC_FAILED)
			return E_NJPD_API_FAILED;
	}
#endif
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Decoder_FeedData(NJPD_API_Buf read_buf)
{
	NJPD_Read_Buffer_Node *pstNewNode;
	u32 u32BufSize;

	njpd_debug(LEVEL_API_TRACE, "enter");

	if ((gNJPDDecoderInfo.buffer_empty[gNJPDDecoderInfo.next_index] == 1)
	    && (gNJPDDecoderInfo.read_list == NULL)) {
		// set it to hw
		u32BufSize = read_buf.filled_length - read_buf.offset;
		u32BufSize = NJPD_Align(u32BufSize, NJPD_ADDR_ALIGN);
		if (MDrv_NJPD_SetReadBuffer
		    (read_buf.pa + read_buf.offset, u32BufSize, gNJPDDecoderInfo.next_index)) {
			gNJPDDecoderInfo.buffer_empty[gNJPDDecoderInfo.next_index] = false;
			MDrv_NJPD_SetMRC_Valid(gNJPDDecoderInfo.next_index);
			gNJPDDecoderInfo.next_index ^= 1;
		} else {
			njpd_err("set buffer fail");
			return E_NJPD_API_FAILED;
		}

	} else {
		// add into list
		pstNewNode = (NJPD_Read_Buffer_Node *)NJPD_MALLOC(sizeof(NJPD_Read_Buffer_Node));
		if (pstNewNode == NULL) {
			njpd_err("alloc buf node fail");
			return E_NJPD_API_FAILED;
		}
		memcpy(&pstNewNode->buf, &read_buf, sizeof(NJPD_API_Buf));
		pstNewNode->next = NULL;
		if (gNJPDDecoderInfo.read_list == NULL) {
			gNJPDDecoderInfo.read_list = pstNewNode;
			gNJPDDecoderInfo.read_list_tail = gNJPDDecoderInfo.read_list;
		} else {
			gNJPDDecoderInfo.read_list_tail->next = pstNewNode;
			gNJPDDecoderInfo.read_list_tail = gNJPDDecoderInfo.read_list_tail->next;
		}
	}
	return E_NJPD_API_OKAY;
}

NJPD_API_Result MApi_NJPD_Decoder_NoMoreData(void)
{
	njpd_debug(LEVEL_API_TRACE, "enter");
	gNJPDDecoderInfo.no_more_data = true;
	if (_MApi_NJPD_Decoder_Check_NoMoreData() == false)
		return E_NJPD_API_OKAY;

	return _MApi_NJPD_Decoder_CheckDataLoss(true);
}

NJPD_API_DecodeStatus MApi_NJPD_Decoder_WaitDone(void)
{
	u16 u16EventFlag;
	bool bNoMoreData;

	u16EventFlag = MApi_NJPD_GetEventFlag();

	if (u16EventFlag & E_NJPD_EVENT_MRC0_EMPTY) {
		gNJPDDecoderInfo.buffer_empty[0] = true;
		gNJPDDecoderInfo.consume_num++;
		_Mapi_NJPD_Decoder_Check_ReadList();
	}
	if (u16EventFlag & E_NJPD_EVENT_MRC1_EMPTY) {
		gNJPDDecoderInfo.buffer_empty[1] = true;
		gNJPDDecoderInfo.consume_num++;
		_Mapi_NJPD_Decoder_Check_ReadList();
	}

	if (u16EventFlag & NJPD_EVENT_ERROR) {
		njpd_err("get error event flag 0x%x", u16EventFlag);
		_MApi_NJPD_Decoder_PrintError(u16EventFlag);
		return E_NJPD_API_DEC_FAILED;
	}

	if (u16EventFlag & E_NJPD_EVENT_DEC_DONE) {
		njpd_debug(LEVEL_API_TRACE, "get decode done event flag!!");
		return E_NJPD_API_DEC_DONE;
	}

	bNoMoreData = _MApi_NJPD_Decoder_Check_NoMoreData();

	MApi_NJPD_ClearEventFlag(u16EventFlag);

	if (_MApi_NJPD_Decoder_CheckDataLoss(bNoMoreData) == E_NJPD_API_FAILED)
		return E_NJPD_API_DEC_FAILED;

#if SUPPORT_PROGRESSIVE
	if (gNJPDDecoderInfo.is_progressive)
		if ((u16EventFlag & E_NJPD_EVENT_MRC0_EMPTY) ||
			(u16EventFlag & E_NJPD_EVENT_MRC1_EMPTY))
			if (_MApi_NJPD_Decoder_DecodeProgressive() == E_NJPD_API_DEC_FAILED)
				return E_NJPD_API_DEC_FAILED;
#endif
	if ((u16EventFlag & E_NJPD_EVENT_MRC0_EMPTY) && (u16EventFlag & E_NJPD_EVENT_MRC1_EMPTY)) {
		if (gNJPDDecoderInfo.next_index == 0)
			return E_NJPD_API_DEC_BUF_EMPTY_01;
		else
			return E_NJPD_API_DEC_BUF_EMPTY_10;
	} else if (u16EventFlag & E_NJPD_EVENT_MRC0_EMPTY) {
		return E_NJPD_API_DEC_BUF_EMPTY_0;
	} else if (u16EventFlag & E_NJPD_EVENT_MRC1_EMPTY) {
		return E_NJPD_API_DEC_BUF_EMPTY_1;
	} else {
		return E_NJPD_API_DEC_DECODING;
	}
}

u16 MApi_NJPD_Decoder_ConsumedDataNum(void)
{
	njpd_debug(LEVEL_API_TRACE, "enter");
	njpd_debug(LEVEL_API_DEBUG, "consume_num : %d", gNJPDDecoderInfo.consume_num);
	return gNJPDDecoderInfo.consume_num;
}

NJPD_API_Result MApi_NJPD_Decoder_Stop(void)
{
	njpd_debug(LEVEL_API_TRACE, "enter");
	_Mapi_NJPD_Decoder_Free_ReadList();
	MDrv_NJPD_Rst();
	MDrv_NJPD_PowerOff();
#if SUPPORT_PROGRESSIVE
	if (gNJPDDecoderInfo.is_progressive)
		_Mapi_NJPD_Decoder_Free_ProgressiveBuf();
#endif
	return E_NJPD_API_OKAY;
}

void MApi_NJPD_Decoder_IRQ(void)
{
	u16 u16EventFlag = MDrv_NJPD_GetEventFlag();

	njpd_debug(LEVEL_API_IRQ, "IRQ receive event : 0x%04x", u16EventFlag);

	if (u16EventFlag & NJPD_EVENT_ALL) {
		NJPD_ISR_StoreEvents(u16EventFlag);
		MDrv_NJPD_SetEventFlag(u16EventFlag);
	}
}
