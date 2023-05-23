/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */



#ifndef _NJPEG_DEF_H_
#define _NJPEG_DEF_H_

//-------------------------------------------------------------------------------------------------
// Local Compiler Options
//-------------------------------------------------------------------------------------------------
#define JPEG_STATIC    static

//NJPD IP supporting feature

#define ENABLE_TEST_09_NJPEGWriteProtectTest        FALSE
#define ENABLE_TEST_11_NJPEGScaleDownFunctionTest_2 FALSE
#define ENABLE_TEST_11_NJPEGScaleDownFunctionTest_4 FALSE
#define ENABLE_TEST_11_NJPEGScaleDownFunctionTest_8 FALSE
#define ENABLE_TEST_16_NJPEGEnablePsaveModeTest     FALSE
#define ENABLE_TEST_18_miu_sel_128M                 FALSE
#define ENABLE_TEST_18_miu_sel_64M                  FALSE
#define ENABLE_TEST_18_miu_sel_32M                  FALSE
#define ENABLE_TEST_22_AutoProtectFailTest          FALSE
#define ENABLE_TEST_NJPD_01_table_read_write_test   FALSE
#define ENABLE_TEST_NJPD_13_ROI_Test                FALSE
#define ENABLE_TEST_NJPD_17_Obuf_Output_Format      FALSE
#define ENABLE_TEST_NJPD_18_Ibuf_Burst_Length_Test  FALSE
#define ENABLE_TEST_NJPD_21_No_Reset_Table_Test     FALSE

//-------------------------------------------------------------------------------------------------
// Macro and Define
//-------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define NJPD_BLOCK_TYPE        s16
//------------------------------------------------------------------------------
#define NJPD_QUANT_TYPE        s16
//------------------------------------------------------------------------------
// May need to be adjusted if support for other colorspaces/sampling factors is added
#define NJPD_MAXBLOCKSPERMCU   10
//------------------------------------------------------------------------------
#define NJPD_MAXHUFFTABLES     8
//------------------------------------------------------------------------------
#define NJPD_MAXQUANTTABLES    4
//------------------------------------------------------------------------------
#define NJPD_MAXCOMPONENTS     4
//------------------------------------------------------------------------------
#define NJPD_MAXCOMPSINSCAN    4
//------------------------------------------------------------------------------
#define NJPD_HUFFSIZE          257
//------------------------------------------------------------------------------
#define NJPD_ALIGN_BASE        8
//------------------------------------------------------------------------------
#define NJPD_ADDR_ALIGN        16
//------------------------------------------------------------------------------
#define NJPD_YUV422_OUT_SIZE   2
//------------------------------------------------------------------------------
#define NJPD_VRY_IBUF_L        0x15
#define NJPD_VRY_IBUF_H        0x1a
//------------------------------------------------------------------------------
#define NJPD_PING_PONG_BUFFER_COUNT   2
//------------------------------------------------------------------------------
//Increase this if you increase the max width!
#define JPEG_MAXBLOCKSPERROW   6144

#define NJPD_memcpy(pDstAddr, pSrcAddr, u32Size) memcpy((pDstAddr), (pSrcAddr), (u32Size))
#define NJPD_memset(pDstAddr, u8value, u32Size) memset((pDstAddr), (u8value), (u32Size))

#define JPEG_AtoU32(pData, u32value)	\
	do {	\
		u32value = 0;	\
		while (('0' <= *pData) && ('9' >= *pData)) {	\
			u32value = (10 * u32value) + (__u32)(*pData - '0');	\
			pData++;	\
		}	\
	} while (0)

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

//-------------------------------------------------------------------------------------------------
// Type and Structure
//-------------------------------------------------------------------------------------------------
typedef enum {
	E_NJPD_NONE,
	E_NJPD_IBUF_BURST_LENGTH,
	E_NJPD_NO_RESET_TABLE
} NJPD_VerificationMode;

//-----------------------------------------------------------------------------
//Description: JPEG scan type
typedef enum {
	E_JPEG_GRAYSCALE = 0,
	E_JPEG_YH1V1 = 1,
	E_JPEG_YH2V1 = 2,
	E_JPEG_YH1V2 = 3,
	E_JPEG_YH2V2 = 4,
	E_JPEG_YH4V1 = 5,
	E_JPEG_CMYK = 6,
	E_JPEG_RGB = 7
} JPEG_ScanType;

typedef enum {
	E_NJPD_MARKER_SOF0 = 0xC0,
	E_NJPD_MARKER_SOF1 = 0xC1,
	E_NJPD_MARKER_SOF2 = 0xC2,
	E_NJPD_MARKER_SOF3 = 0xC3,
	E_NJPD_MARKER_SOF5 = 0xC5,
	E_NJPD_MARKER_SOF6 = 0xC6,
	E_NJPD_MARKER_SOF7 = 0xC7,
	E_NJPD_MARKER_JPG = 0xC8,
	E_NJPD_MARKER_SOF9 = 0xC9,
	E_NJPD_MARKER_SOF10 = 0xCA,
	E_NJPD_MARKER_SOF11 = 0xCB,
	E_NJPD_MARKER_SOF13 = 0xCD,
	E_NJPD_MARKER_SOF14 = 0xCE,
	E_NJPD_MARKER_SOF15 = 0xCF,
	E_NJPD_MARKER_DHT = 0xC4,
	E_NJPD_MARKER_DAC = 0xCC,
	E_NJPD_MARKER_RST0 = 0xD0,
	E_NJPD_MARKER_RST1 = 0xD1,
	E_NJPD_MARKER_RST2 = 0xD2,
	E_NJPD_MARKER_RST3 = 0xD3,
	E_NJPD_MARKER_RST4 = 0xD4,
	E_NJPD_MARKER_RST5 = 0xD5,
	E_NJPD_MARKER_RST6 = 0xD6,
	E_NJPD_MARKER_RST7 = 0xD7,
	E_NJPD_MARKER_SOI = 0xD8,
	E_NJPD_MARKER_EOI = 0xD9,
	E_NJPD_MARKER_SOS = 0xDA,
	E_NJPD_MARKER_DQT = 0xDB,
	E_NJPD_MARKER_DNL = 0xDC,
	E_NJPD_MARKER_DRI = 0xDD,
	E_NJPD_MARKER_DHP = 0xDE,
	E_NJPD_MARKER_EXP = 0xDF,
	E_NJPD_MARKER_APP0 = 0xE0,
	E_NJPD_MARKER_APP1 = 0xE1,
	E_NJPD_MARKER_APP2 = 0xE2,
	E_NJPD_MARKER_APP3 = 0xE3,
	E_NJPD_MARKER_APP4 = 0xE4,
	E_NJPD_MARKER_APP5 = 0xE5,
	E_NJPD_MARKER_APP6 = 0xE6,
	E_NJPD_MARKER_APP7 = 0xE7,
	E_NJPD_MARKER_APP8 = 0xE8,
	E_NJPD_MARKER_APP9 = 0xE9,
	E_NJPD_MARKER_APP10 = 0xEA,
	E_NJPD_MARKER_APP11 = 0xEB,
	E_NJPD_MARKER_APP12 = 0xEC,
	E_NJPD_MARKER_APP13 = 0xED,
	E_NJPD_MARKER_APP14 = 0xEE,
	E_NJPD_MARKER_APP15 = 0xEF,
	E_NJPD_MARKER_JPG0 = 0xF0,
	E_NJPD_MARKER_JPG1 = 0xF1,
	E_NJPD_MARKER_JPG2 = 0xF2,
	E_NJPD_MARKER_JPG3 = 0xF3,
	E_NJPD_MARKER_JPG4 = 0xF4,
	E_NJPD_MARKER_JPG5 = 0xF5,
	E_NJPD_MARKER_JPG6 = 0xF6,
	E_NJPD_MARKER_JPG7 = 0xF7,
	E_NJPD_MARKER_JPG8 = 0xF8,
	E_NJPD_MARKER_JPG9 = 0xF9,
	E_NJPD_MARKER_JPG10 = 0xFA,
	E_NJPD_MARKER_JPG11 = 0xFB,
	E_NJPD_MARKER_JPG12 = 0xFC,
	E_NJPD_MARKER_JPG13 = 0xFD,
	E_NJPD_MARKER_COM = 0xFE,
	E_NJPD_MARKER_TEM = 0x01,
	E_NJPD_MARKER_ERROR = 0x100
} E_NJPD_HeaderMarker;

typedef enum {
	E_NJPD_MIU_LAST_Z_PATCH,
	E_NJPD_EAGLE_SW_PATCH,
	E_NJPD_EIFFEL_REDUCE_CLK
} NJPD_PATCH_INDEX;

typedef struct {
	s16 s16Value[64];	// value of Q table
	u8 bValid;		// has Q table or not.
} NJPD_QuantTbl;

typedef struct {
	u16 u16OriginX;
	u16 u16OriginY;
	u16 u16DecodeAlignX;
	u16 u16DecodeAlignY;
} NJPD_API_ImageInfo;

typedef struct {
	u8 u8Huff_num[17];	// number of Huffman codes per bit size
	u8 u8Huff_val[256];	// Huffman codes per bit size
	u8 u8Symbol[17];	// u8Huff_num in reverse order
	u16 u16Code[17];	// Minimun code word
	u8 u8Valid[17];		// Valid bit for NJPD
	u8 u8SymbolCnt;
	u8 bValid;		// has huffman table or not
} NJPD_HuffInfo;

typedef struct {
	s16 s16Look_up[256];	// the lookup of huffman code
	// FIXME: Is 512 tree entries really enough to handle _all_ possible
	// code sets? I think so but not 100% positive.
	s16 s16Tree[512];	// huffman tree of huffman code
	u8 u8Code_size[256];	// code size of huffman code
} NJPD_HuffTbl;

typedef struct {
	u16 u16ImageYSize;
	u16 u16ImageXSize;
	u16 u16OriginalImageYSize;
	u16 u16OriginalImageXSize;
	u8 u8ComponentsInFrame;
	u8 u8ComponentHSamp[NJPD_MAXCOMPONENTS];
	u8 u8ComponentVSamp[NJPD_MAXCOMPONENTS];
	u8 u8ComponentQuant[NJPD_MAXCOMPONENTS];
	u8 u8ComponentIdent[NJPD_MAXCOMPONENTS];
	u8 u8LumaComponentID;
	u8 u8ChromaComponentID;
	u8 u8Chroma2ComponentID;
	u16 u16MaxWidth;
	u16 u16MaxHeight;
	u32 u32WriteBufferSize;
} NJPD_SOFInfo;

typedef struct {
	u8 u8ComponentsInScan;
	u8 u8ComponentList[NJPD_MAXCOMPSINSCAN];
	u8 u8ComponentACTable[NJPD_MAXCOMPSINSCAN];
	u8 u8ComponentDCTable[NJPD_MAXCOMPSINSCAN];
	bool bIs3HuffTable;
	u8 u8SpectralStart;
	u8 u8SpectralEnd;
	u8 u8SuccessiveLow;
	u8 u8SuccessiveHigh;
} NJPD_SOSInfo;

typedef struct {
	u8 bIs3HuffTbl;
	u8 u8DcLumaCnt;
	u8 u8DcChromaCnt;
	u8 u8DcChroma2Cnt;
	u8 u8AcLumaCnt;
	u8 u8AcChromaCnt;
	u8 u8AcChroma2Cnt;
	u16 u16DcTblNumLuma;
	u16 u16DcTblNumChroma;
	u16 u16DcTblNumChroma2;
	u16 u16AcTblNumLuma;
	u16 u16AcTblNumChroma;
	u16 u16AcTblNumChroma2;
	dma_addr_t paGTablePA;
	dma_addr_t paHTablePA;
	u32 u32HTableSize;
	dma_addr_t paQTablePA;
	bool bDiffQTable;
	u16 u16RestartInterval;
} NJPD_TableInfo;

#endif				//_NJPEG_DEF_H_
