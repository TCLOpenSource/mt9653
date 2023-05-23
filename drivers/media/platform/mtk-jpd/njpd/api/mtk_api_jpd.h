/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _API_NJPD_H_
#define _API_NJPD_H_

#include "mtk_njpeg_def.h"
#include "mtk_sti_msos.h"
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif
//-------------------------------------------------------------------------------------------------
// Macro and Define
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Type and Structure
//-------------------------------------------------------------------------------------------------
#if SUPPORT_PROGRESSIVE
	//-----------------------------------------------------------------------------
	/// @brief \b Struct \b Name: JPEG_CoeffBuf
	/// @brief \b Struct \b Description: The info of coefficient for JPEG decode
	//-----------------------------------------------------------------------------
	typedef struct {
		u8 *pu8Data;		///<data of coefficient of DC, AC
		u16 u16Block_num_x;	///<the number of block for width
		u16 u16Block_num_y;	///<the number of block for height
		u16 u16Block_size;	///<block size
		u8 u8Block_len_x;	///<The width of block
		u8 u8Block_len_y;	///<The height of block
	} JPEG_CoeffBuf, *PJPEG_CoeffBuf;

	//-----------------------------------------------------------------------------
	/// @brief \b Struct \b Name: JPEG_SVLD
	/// @brief \b Struct \b Description: The info of SVLD for JPEG decode
	//-----------------------------------------------------------------------------
	typedef struct {
		union {
			struct {
				__u32 data:12;	///<The value of VLI(2's compl & sign-ext)
				__u32 run:4;	///<run value
				__u32 EOB:1;	///<end of block
				__u32 altzz:1;	///<alternative zig-zag
				__u32 zzorder:1;	///<zig-zag scan order
				__u32 trash:13; ///<reserved
			};

			struct {
				__u8 byte0;	///<byte0 of SVLD
				__u8 byte1;	///<byte1 of SVLD
				__u8 byte2;	///<byte2 of SVLD
				__u8 byte3;	///<byte3 of SVLD
			};
		};
	} JPEG_SVLD_Info;

	typedef struct {
		u16 _u16Comp_h_blocks[NJPD_MAXCOMPONENTS];
		u16 _u16Comp_v_blocks[NJPD_MAXCOMPONENTS];
		u8 _u8Mcu_org[NJPD_MAXBLOCKSPERMCU];
		u8 _u8Blocks_per_mcu;
		u16 gu16Max_mcus_per_row;
		u16 _u16Max_mcus_per_col;
		u32 _u32Max_blocks_per_row;
		u16 _u16Mcus_per_row;
		u16 _u16Mcus_per_col;

		u16 _u16Max_blocks_per_mcu;
		u8 gu8Max_mcu_x_size;
		u8 gu8Max_mcu_y_size;

		u16 _u16Total_lines_left;
		u32 _u32Block_y_mcu[NJPD_MAXCOMPONENTS];
		NJPD_HuffTbl _Huff_tbls[NJPD_MAXHUFFTABLES];
		PJPEG_CoeffBuf _DC_Coeffs[NJPD_MAXCOMPONENTS];
		PJPEG_CoeffBuf _AC_Coeffs[NJPD_MAXCOMPONENTS];
		u32 _u32Last_dc_val[NJPD_MAXCOMPONENTS];
		u32 _u32EOB_run;

		u16 _u16Restarts_left;
		u16 _u16Next_restart_num;

		s16 _s16Bits_left;
		u32 _u32Bit_buf;

		NJPD_BLOCK_TYPE _s16dc_pred[3];
		u32 _u32RLEOffset;
	} NJPD_ProgInfo;

	typedef u8(*NJPD_API_PdecodeBlockFunc) (int, u8, u16, u16);

#endif

	typedef enum {
		E_NJPD_API_FAILED = 0,
		E_NJPD_API_OKAY = 1,
	} NJPD_API_Result;

	typedef enum {
		E_NJPD_API_FORMAT_JPEG = 0,
		E_NJPD_API_FORMAT_MJPEG = 1,
	} NJPD_API_Format;

	typedef enum {
		E_NJPD_API_HEADER_INVALID = 0,
		E_NJPD_API_HEADER_VALID = 1,
		E_NJPD_API_HEADER_PARTIAL = 2,
		E_NJPD_API_HEADER_END = 3,
	} NJPD_API_ParserResult;

	typedef enum {
		E_NJPD_API_DOWNSCALE_MIN = -1,
		E_NJPD_API_DOWNSCALE_ORG = 0,
		E_NJPD_API_DOWNSCALE_HALF = 1,
		E_NJPD_API_DOWNSCALE_FOURTH = 2,
		E_NJPD_API_DOWNSCALE_EIGHTH = 3,
		E_NJPD_API_DOWNSCALE_MAX = 4,
	} NJPD_API_DownScaleFactor;

	typedef enum {
		E_NJPD_API_DEC_FAILED = 0,
		E_NJPD_API_DEC_DONE = 1,
		E_NJPD_API_DEC_DECODING = 2,
		E_NJPD_API_DEC_BUF_EMPTY_0 = 3,
		E_NJPD_API_DEC_BUF_EMPTY_1 = 4,
		E_NJPD_API_DEC_BUF_EMPTY_01 = 5,
		E_NJPD_API_DEC_BUF_EMPTY_10 = 6,
	} NJPD_API_DecodeStatus;

	typedef struct {
		void __iomem *jpd_reg_base;
		void __iomem *jpd_ext_reg_base;
		struct clk *clk_njpd;
		struct clk *clk_smi2jpd;
		struct clk *clk_njpd2jpd;
		int jpd_irq;
	} NJPD_API_InitParam;

	typedef struct {
		void *va;
		dma_addr_t pa;
		size_t offset;
		size_t filled_length;
		size_t size;
	} NJPD_API_Buf;

	typedef NJPD_API_Result(*NJPD_API_AllocBufFunc) (size_t size, NJPD_API_Buf *buf,
							 void *priv);
	typedef NJPD_API_Result(*NJPD_API_FreeBufFunc) (NJPD_API_Buf buf, void *priv);

	typedef struct {
		NJPD_API_AllocBufFunc pAllocBufFunc;
		NJPD_API_FreeBufFunc pFreeBufFunc;
		void *priv;
	} NJPD_API_ParserInitParam;

	typedef struct {
		u16 width;
		u16 height;
	} NJPD_API_Pic;

	typedef struct {
		u16 x;
		u16 y;
		u16 width;
		u16 height;
	} NJPD_API_Rect;

	typedef struct {
		u32 width;
		u32 height;
	} NJPD_API_ParserImageInfo;

	typedef struct {
		dma_addr_t g_table_pa;
		dma_addr_t h_table_pa;
		size_t h_table_size;
		dma_addr_t q_table_pa;
		bool diff_q_table;
		bool diff_h_table;
		u8 comps_in_frame;
		u8 comp_v_samp;
		u8 comp_h_samp;
		u16 restart_interval;
	} NJPD_API_TableInfo;

	typedef struct {
		u32 width;
		u32 height;
		NJPD_API_Rect rect;
		u32 write_buf_size;
	} NJPD_API_ParserBufInfo;

	typedef struct {
		bool mjpeg;
		bool iommu;
		bool progressive;
		u8 format;
		u8 mode;
		u32 width;
		u32 height;
		NJPD_API_DownScaleFactor eFactor;
		NJPD_API_Rect ROI_rect;
		NJPD_API_Buf read_buffer;
		NJPD_API_Buf internal_buffer;
		NJPD_API_TableInfo table_info;
#if SUPPORT_PROGRESSIVE
		size_t prog_info_buffer_offset;
		u8 u8ComponentsInScan;
		u8 u8ComponentsInFrame;
		u8 u8ComponentList[NJPD_MAXCOMPSINSCAN];
		u8 u8ComponentHSamp[NJPD_MAXCOMPONENTS];
		u8 u8ComponentVSamp[NJPD_MAXCOMPONENTS];
		size_t progressive_read_buffer_offset;
#endif
	} NJPD_API_DecCmd;

//-------------------------------------------------------------------------------------------------
// Common Functions
//-------------------------------------------------------------------------------------------------
	NJPD_API_Result MApi_NJPD_Init(NJPD_API_InitParam *pInitParam);

	NJPD_API_Result MApi_NJPD_Deinit(void);

//-------------------------------------------------------------------------------------------------
// Parser Functions
//-------------------------------------------------------------------------------------------------
	NJPD_API_Result MApi_NJPD_Parser_Open(int *pHandle,
					      NJPD_API_ParserInitParam *pParserInitParam);

	NJPD_API_Result MApi_NJPD_Parser_SetFormat(int handle, NJPD_API_Format eFormat);

	NJPD_API_Result MApi_NJPD_Parser_SetIommu(int handle, bool bIommu);

	NJPD_API_Result MApi_NJPD_Parser_SetROI(int handle, NJPD_API_Rect ROI_rect);

	NJPD_API_Result MApi_NJPD_Parser_SetDownScale(int handle, NJPD_API_Pic pic);

	NJPD_API_Result MApi_NJPD_Parser_SetMaxResolution(int handle, NJPD_API_Pic pic);

	NJPD_API_Result MApi_NJPD_Parser_SetOutputFormat(int handle, u8 format);

	NJPD_API_Result MApi_NJPD_Parser_SetVerificationMode(int handle, u8 mode);

	NJPD_API_Result MApi_NJPD_Parser_Reset(int handle);

	NJPD_API_ParserResult MApi_NJPD_Parser_ParseHeader(int handle, NJPD_API_Buf buf,
							   NJPD_API_DecCmd *pDecCmd);

	NJPD_API_Result MApi_NJPD_Parser_GetImageInfo(int handle,
						      NJPD_API_ParserImageInfo *pImageInfo);

	NJPD_API_Result MApi_NJPD_Parser_GetBufInfo(int handle, NJPD_API_ParserBufInfo *pBufInfo);

	NJPD_API_Result MApi_NJPD_Parser_Close(int handle);

//-------------------------------------------------------------------------------------------------
// Decoder Functions
//-------------------------------------------------------------------------------------------------
	NJPD_API_Result MApi_NJPD_Decoder_Start(NJPD_API_DecCmd *pDecCmd, NJPD_API_Buf write_buf);

	NJPD_API_Result MApi_NJPD_Decoder_FeedData(NJPD_API_Buf read_buf);

	NJPD_API_Result MApi_NJPD_Decoder_NoMoreData(void);

	NJPD_API_DecodeStatus MApi_NJPD_Decoder_WaitDone(void);

	u16 MApi_NJPD_Decoder_ConsumedDataNum(void);

	NJPD_API_Result MApi_NJPD_Decoder_Stop(void);

	void MApi_NJPD_Decoder_IRQ(void);

#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif				// _API_NJPD_H_
//------------------------------------------------------------------------------
