/*------------------------------------------------------------------------------
* MediaTek Inc. (C) 2019. All rights reserved.
*
* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein is
* confidential and proprietary to MediaTek Inc. and/or its licensors. Without
* the prior written permission of MediaTek inc. and/or its licensors, any
* reproduction, modification, use or disclosure of MediaTek Software, and
* information contained herein, in whole or in part, shall be strictly
* prohibited.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
* ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
* WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
* NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
* RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
* INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
* TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
* RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
* OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
* SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
* RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
* ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
* RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
* MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
* CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
* The following software/firmware and/or related documentation ("MediaTek
* Software") have been modified by MediaTek Inc. All revisions are subject to
* any receiver's applicable license agreements with MediaTek Inc.
*----------------------------------------------------------------------------*/

//////////////////////////////////////////////////////////////////////////////
/////////////////////
///
/// @file   drvMVOP.h
/// @brief  MVOP Driver Interface
/// @author MediaTek Inc.
///////////////////////////////////////////////////////////////////////////////
////////////////////

/*!
 \defgroup MVOP MVOP interface (drvMVOP.h)
 \ingroup VDEC

 <b> MVOP api flow </b> \n
   \image html drvMVOP_flow.png

 \defgroup MVOP MVOP module
 \ingroup  MVOP
 \defgroup MVOP_Basic MVOP_Basic
 \ingroup MVOP
 \defgroup MVOP_SetCommand MVOP SetCommand
 \ingroup MVOP
 \defgroup MVOP_GetCommand MVOP GetCommand
 \ingroup MVOP
 \defgroup MVOP_Debug MVOP Debug
 \ingroup MVOP
*/

//#ifdef MSOS_TYPE_LINUX_KERNEL
//#define SYMBOL_WEAK
//#else
#define SYMBOL_WEAK __attribute__((weak))
//#endif

#ifndef _DRV_MVOP_H_
#define _DRV_MVOP_H_

#ifdef __cplusplus
extern "C"
{
#endif
//#include "MsDevice.h"
#include "UFO.h"
#include "MsTypes.h"
#include "apiXC.h"  // for EN_POWER_MODE
//----------------------------------------------------------------------------
//---------------------
//  Driver Capability
//----------------------------------------------------------------------------
//---------------------


//----------------------------------------------------------------------------
//---------------------
//  Macro and Define
//----------------------------------------------------------------------------
//---------------------
/// Version string.
#define MSIF_MVOP_LIB_CODE              {'M', 'V', 'O', 'P'}    //Lib code
#define MSIF_MVOP_LIBVER                {'1', '0'}            //LIB version
#define MSIF_MVOP_BUILDNUM              {'4', '2'}            //Build Number
#define MSIF_MVOP_CHANGELIST            {'0', '0', '7', '0', '2', '3', '8', '3'}
//P4 ChangeList Number

#define MVOP_DRV_VERSION( \
	/* Character String for DRV/API version  */ \
	MSIF_TAG,/* 'MSIF'                                           */ \
	MSIF_CLASS,/* '00'                                           */ \
	MSIF_CUS,/* 0x0000                                           */ \
	MSIF_MOD,/* 0x0000                                           */ \
	MSIF_CHIP, \
	MSIF_CPU, \
	MSIF_MVOP_LIB_CODE,/* IP__                                   */ \
	MSIF_MVOP_LIBVER,/* 0.0 ~ Z.Z                                */ \
	MSIF_MVOP_BUILDNUM,/* 00 ~ 99                                */ \
	MSIF_MVOP_CHANGELIST,/* CL#                                  */ \
	MSIF_OS)

#define _SUPPORT_IMG_OFFSET_
#define ENABLE_UTOPIA2_INTERFACE    1
#define ENABLE_MUTEX                1

#define API_MVOP_DMS_DIPS_ADD_VERSION                   8
#define API_MVOP_DMS_DISP_SIZE_VERSION                  2
#define API_MVOP_DMS_STREAM_VERSION                     2
#define API_MVOP_DMS_CROP_VERSION                       1
#define CMD_MVOP_DYNAMIC_FPS_VERSION                    2
#define API_MVOP_DMS_INTSTATUS_VERSION                  1

typedef void (*HAL_XCWrite)(MS_U32 u32Reg, MS_U8 u16Value);

//----------------------------------------------------------------------------
//---------------------
//  Type and Structure
//----------------------------------------------------------------------------
//---------------------
typedef enum {
	E_MVOP_OK                   = 0,
	E_MVOP_FAIL                 = 1,
	E_MVOP_INVALID_PARAM        = 2,
	E_MVOP_NOT_INIT             = 3,
	E_MVOP_UNSUPPORTED          = 4
} MVOP_Result;

/// MVOP tile format
typedef enum {
	E_MVOP_TILE_8x32            = 0,
	E_MVOP_TILE_16x32           = 1,
	E_MVOP_TILE_NONE            = 2,
	E_MVOP_TILE_32x16           = 3,
	E_MVOP_TILE_32x32           = 4,
	E_MVOP_TILE_COMP_4x2        = 5,            ///< vsd
	E_MVOP_TILE_COMP_8x1        = 6,            ///< vsd
	E_MVOP_TILE_COMP_32x16_82PACK = 7,          ///< 82 pack 10bits
} MVOP_TileFormat;

/// MVOP RGB format
typedef enum {
	E_MVOP_RGB_565              = 0,
	E_MVOP_RGB_888              = 1,
	E_MVOP_RGB_NONE             = 0xff
} MVOP_RgbFormat;

/// MVOP input mode
typedef enum {
	VOPINPUT_HARDWIRE           = 0,        ///< hardwire mode (MVD)
	VOPINPUT_HARDWIRECLIP       = 1,        ///< hardware clip mode (MVD)
	VOPINPUT_MCUCTRL            = 2,       ///< MCU control mode (M4VD, JPG)
	VOPINPUT_IFRAME             = 3,        ///< MCU control mode (MVD)
} VOPINPUTMODE;

/// MVOP input mode
typedef enum {
    E_VOPMIRROR_VERTICAL        = 1,
    E_VOPMIRROR_HORIZONTALL     = 2,
    E_VOPMIRROR_HVBOTH          = 3,
    E_VOPMIRROR_NONE            = 4,
} MVOP_DrvMirror;

/// MVOP repeat field mode
typedef enum {
    E_MVOP_RPTFLD_NONE          = 0,   ///< normal display top and bottom fields
    E_MVOP_RPTFLD_TOP           = 1,            ///< top field
    E_MVOP_RPTFLD_BOT           = 2,            ///< bottom field
} MVOP_RptFldMode;

/// MVOP Output 3D Type
typedef enum {
	E_MVOP_OUTPUT_3D_NONE,                      ///< Output 2D
	E_MVOP_OUTPUT_3D_TB,                        ///< Output Top Bottom
	E_MVOP_OUTPUT_3D_SBS,                       ///< Output Side By Side
	E_MVOP_OUTPUT_3D_LA,                        ///< Output Line Alternative
	E_MVOP_OUTPUT_3D_MAXNUM,
} EN_MVOP_Output_3D_TYPE;

typedef enum {
	MVOP_MPEG4                  = 0,            ///< 0x0
	MVOP_MJPEG,                                 ///< 0x1
	MVOP_H264,                                  ///< 0x2
	// <ATV MM>
	MVOP_RM, //jyliu.rm                         ///< 0x3
	MVOP_TS,                                    ///< 0x4
	MVOP_MPG,                                   ///< 0x5
	MVOP_AV1,                                   ///< 0x6
	MVOP_AV1_FG,                                ///< 0x7 av1 + film grain
	MVOP_UNKNOWN                 = 0xff,
	// </ATV MM>
} MVOP_VIDEO_TYPE;


/// MVOP input parameter
typedef struct DLL_PACKED {
	MS_PHY u32YOffset;
	MS_PHY u32UVOffset;
	MS_U16 u16HSize;
	MS_U16 u16VSize;
	MS_U16 u16StripSize;

	MS_BOOL bYUV422;                            ///< YUV422 or YUV420
	MS_BOOL bSD;                                ///< SD or HD
	MS_BOOL bProgressive;     ///< Progressive or Interlace

	// in func MDrv_MVOP_Input_Mode(), bSD is used to set dc_strip[7:0].
	// in func MDrv_MVOP_Input_Mode_Ext(), bSD is don't care and
	//    dc_strip[7:0] is set according to Hsize
	MS_BOOL bUV7bit;         ///< +S3, UV 7 bit or not
	MS_BOOL bDramRdContd;    ///< +S3, continue read out or jump 32
	MS_BOOL bField;
	///< +S3, Field 0 or 1 //(stbdc)for u4 bottom
	///field enabled when bfield is true
	MS_BOOL b422pack;
	///< +S3, YUV422 pack mode
	MS_U16 u16CropX;
	MS_U16 u16CropY;
	MS_U16 u16CropWidth;
	MS_U16 u16CropHeight;
	MVOP_VIDEO_TYPE enVideoType;
} MVOP_InputCfg;


typedef struct DLL_PACKED {
	MS_U16 u16V_TotalCount;               ///< Vertical total count
	MS_U16 u16H_TotalCount;               ///< Horizontal total count
	MS_U16 u16VBlank0_Start;              ///< Vertical blank 0 start
	MS_U16 u16VBlank0_End;                ///< Vertical blank 0 End
	MS_U16 u16VBlank1_Start;              ///< Vertical blank 1 start
	MS_U16 u16VBlank1_End;                ///< Vertical blank 1 End
	MS_U16 u16TopField_Start;             ///< Top field start
	MS_U16 u16BottomField_Start;          ///< bottom field start
	MS_U16 u16TopField_VS;                ///< top field vsync
	MS_U16 u16BottomField_VS;             ///< bottom field vsync
	MS_U16 u16HActive_Start;              ///< Horizontal disaply start

	MS_BOOL bInterlace;                   ///< interlace or not
	MS_U8 u8Framerate;                    ///< frame rate
	MS_U16 u16H_Freq ;                    ///< horizontal frequency
	MS_U16 u16Num;                        ///< MVOP SYNTHESIZER numerator
	MS_U16 u16Den;                        ///< MVOP SYNTHESIZER denumerator
	MS_U8 u8MvdFRCType;                   ///< flag for frame rate convert

	MS_U16 u16ExpFrameRate;               ///< Frame Rate
	MS_U16 u16Width;                      ///< Width
	MS_U16 u16Height;                     ///< Height
	MS_U16 u16HImg_Start;                 ///< Horizontal disaply start
	MS_U16 u16VImg_Start0;                ///< Vertical disaply start
	MS_U16 u16VImg_Start1;                ///< Vertical disaply start
	MS_BOOL bHDuplicate;            ///< flag for vop horizontal duplicate
} MVOP_Timing;

//Display information For GOP
typedef struct DLL_PACKED {
	MS_U32 VDTOT;                            ///< Output vertical total
	MS_U32 DEVST;                            ///< Output DE vertical start
	MS_U32 DEVEND;                           ///< Output DE Vertical end
	MS_U32 HDTOT;                            ///< Output horizontal total
	MS_U32 DEHST;                            ///< Output DE horizontal start
	MS_U32 DEHEND;                           ///< Output DE horizontal end
	MS_BOOL bInterlaceMode;
} MVOP_DST_DispInfo;

//MVOP timing information from mvop registers
typedef struct {
	MS_U16 u16V_TotalCount;                     ///< Vertical total count
	MS_U16 u16H_TotalCount;                     ///< Horizontal total count
	MS_U16 u16VBlank0_Start;                    ///< Vertical blank 0 start
	MS_U16 u16VBlank0_End;                      ///< Vertical blank 0 End
	MS_U16 u16VBlank1_Start;                    ///< Vertical blank 1 start
	MS_U16 u16VBlank1_End;                      ///< Vertical blank 1 End
	MS_U16 u16TopField_Start;                   ///< Top field start
	MS_U16 u16BottomField_Start;                ///< bottom field start
	MS_U16 u16TopField_VS;                      ///< top field vsync
	MS_U16 u16BottomField_VS;                   ///< bottom field vsync
	MS_U16 u16HActive_Start;                    ///< Horizontal disaply start
	MS_BOOL bInterlace;                         ///< interlace or not
	MS_BOOL bEnabled;                           ///< MVOP is enabled or not
} MVOP_TimingInfo_FromRegisters;

typedef enum {
	MVOP_PATTERN_NORMAL         = 0,
	MVOP_PATTERN_COLORBAR       = 1,
	MVOP_PATTERN_FRAMECOLOR     = 2,
	MVOP_PATTERN_YCBCR          = 3,
	MVOP_PATTERN_Y              = 4,
	MVOP_PATTERN_CB             = 5,
	MVOP_PATTERN_CR             = 6,
	MVOP_PATTERN_DEFAULT        = 7
	///< new pattern = (last parrtern + 1) % MVOP_PATTERN_DEFAULT
} MVOP_Pattern;

typedef enum {
	MVOP_INPUT_DRAM             = 0,
	MVOP_INPUT_H264             = 1,            ///< SVD for T2
	MVOP_INPUT_MVD              = 2,
	MVOP_INPUT_RVD              = 3,
	MVOP_INPUT_CLIP             = 4,
	MVOP_INPUT_JPD              = 5,
	MVOP_INPUT_HVD_3DLR         = 6,            ///< HVD 3D L/R mode
	MVOP_INPUT_MVD_3DLR         = 7,            ///< MVD 3D L/R mode
	MVOP_INPUT_DRAM_3DLR        = 8,
	MVOP_INPUT_EVD              = 9,
	MVOP_INPUT_EVD_3DLR         = 10,
	MVOP_INPUT_VP9              = 11,
	MVOP_INPUT_EVD_DV           = 12,
	MVOP_INPUT_H264_DV          = 13,
	MVOP_INPUT_EVD_TCH          = 14,
	MVOP_INPUT_AVS2             = 15,
	MVOP_INPUT_DMS_DRAM         = 16,
	MVOP_INPUT_DMS_DRAM_HDR_DV  = 17,
	MVOP_INPUT_UNKNOWN          = -1
} MVOP_InputSel;

/// MVOP VideoStat data structure
typedef struct DLL_PACKED {
	MS_U16 u16HorSize;
	MS_U16 u16VerSize;
	MS_U16 u16FrameRate;
	MS_U8 u8AspectRate;
	MS_U8 u8Interlace;
	MS_U16 u16HorOffset;
	MS_U16 u16VerOffset;
} MVOP_VidStat;

/// MVOP driver info.
typedef struct DLL_PACKED _MVOP_DrvInfo {
	MS_U32 u32MaxSynClk;
	MS_U32 u32MinSynClk;
	MS_U32 u32MaxFreerunClk;
} MVOP_DrvInfo;

///MVOP driver status
typedef struct DLL_PACKED _MVOP_DrvStatus {
	MS_BOOL bIsInit;
	MS_BOOL bIsEnable;
	MS_BOOL bIsUVShift;
	MS_BOOL bIsBlackBG;
	MS_BOOL bIsMonoMode;
	MS_BOOL bIsSetTiming;
} MVOP_DrvStatus;

///MVOP module enum
typedef enum {
	E_MVOP_MODULE_MAIN          = 0,
	E_MVOP_MODULE_SUB           = 1,
	E_MVOP_MODULE_MAX
} MVOP_Module;

///MVOP device enum
typedef enum {
	E_MVOP_DEV_0                = 0,            ///< Main mvop
	E_MVOP_DEV_1                = 1,            ///< Sub mvop
	E_MVOP_DEV_2                = 2,            ///< 3rd mvop
	E_MVOP_DEV_NONE             = 0xff
} MVOP_DevID;

///MVOP handle to carry out MVOP info.
typedef struct DLL_PACKED _MVOP_Handle {
	MVOP_Module eModuleNum;
} MVOP_Handle;


typedef enum {
	E_MVOP_MAIN_VIEW            = 0,
	E_MVOP_2ND_VIEW             = 0x01,         ///< buffer underflow
	E_MVOP_DV_EL_VIEW           = 0x02,         ///< Dual DV address
} MVOP_3DView;

typedef struct DLL_PACKED {
	MVOP_3DView eView;
	MS_PHY u32YOffset;
	MS_PHY u32UVOffset;
} MVOP_BaseAddInput;

typedef struct DLL_PACKED {
	MS_BOOL bIsEnableLuma;
	MS_BOOL bIsEnableChroma;
	MS_U8 u8LumaValue;
	MS_U8 u8ChromaValue;
} MVOP_VC1RangeMapInfo;

typedef struct DLL_PACKED {
	MS_PHY u32MSBYOffset;
	MS_PHY u32MSBUVOffset;
	MS_PHY u32LSBYOffset;
	MS_PHY u32LSBUVOffset;
	MS_BOOL bProgressive;
	MS_BOOL b422Pack;
	MS_BOOL bEnLSB;
} MVOP_EVDBaseAddInput;

typedef enum {
	E_MVOP_EVD_8BIT             = 0,
	E_MVOP_EVD_10BIT            = 0x01,         ///< buffer underflow
} MVOP_EVDBit;

typedef struct DLL_PACKED {
	MS_BOOL bEnableEVD;
	MVOP_EVDBit eEVDBit[2];
} MVOP_EVDFeature;

typedef enum {
	E_MVOP_MAIN_STREAM          = 0,
	//E_MVOP_2nd_STREAM         = 1,
} MVOP_StreamID;

typedef struct DLL_PACKED {
	MS_U32  u32Size;
	MS_U32  u32Version;

	MS_U8   u8MIU[2];
	MS_PHY  u32ProtectStart[2];
	MS_PHY  u32ProtectEnd[2];
	MS_BOOL bEnable;
} MVOP_MIUReadPrct;

typedef enum {
	E_MVOP_MF_ROTATE_NONE       = 0,
	E_MVOP_MF_ROTATE_90         = 0x01,         ///< buffer underflow
	E_MVOP_MF_ROTATE_180        = 0x02,         ///< buffer underflow
	E_MVOP_MF_ROTATE_270        = 0x03,         ///< buffer underflow
} MVOP_MFDECRotate;

typedef enum {
	E_MVOP_OUTPUT_444           = 0,
	E_MVOP_OUTPUT_422           = 0x01,
} MVOP_OutputColorFmt;

///MVOP set command used by MDrv_MVOP_SetCommand()
typedef enum {
	E_MVOP_CMD_SET_TYPE         = 0x100,
	E_MVOP_CMD_SET_VSIZE_MIN,
	///< 0x101 Enable vsize minimum checking
	E_MVOP_CMD_SET_STB_FD_MASK_CLR,
	///< 0x102 Force set fd_mask to low
	E_MVOP_CMD_SET_3DLR_INST_VBLANK,
	///< 0x103 Vertical blanking lines between L & R for 3D L/R mode.
	E_MVOP_CMD_SET_3DLR_ALT_OUT,
	///< 0x104 Alternative lines output for 3D L/R mode.
	//If interlace output, both view output the same field.
	E_MVOP_CMD_SET_RGB_FMT,
	///< 0x105 RGB format input: MVOP_RgbFormat.
	E_MVOP_CMD_SET_SW_CTRL_FIELD_ENABLE,
	///< 0x106 Force read top or bottom field.
	E_MVOP_CMD_SET_SW_CTRL_FIELD_DSIABLE,
	///< 0x107 Disable force read control
	E_MVOP_CMD_SET_3DLR_2ND_CFG,
	///< 0x108 Enable supporting 2nd pitch(h/vsize) for 3D L/R mode.
	E_MVOP_CMD_SET_VSIZE_DUPLICATE,
	///< 0x109 Enable/Disable VSize duplicate.
	E_MVOP_CMD_SET_3DLR_ALT_OUT_SBS,
	///< 0x10A Line alternative read of 3D L/R mode, side-by-side output.
	E_MVOP_CMD_SET_FIELD_DUPLICATE,
	///< 0x10B Repeat field for interlace source. (only input one field)
	E_MVOP_CMD_SET_VSYNC_MODE,
	///< 0x10C Set VSync mode (0: original; 1: new)
	E_MVOP_CMD_SET_VSIZE_X4_DUPLICATE,
	///< 0x10D Enable/Disable VSize x4 duplicate.
	E_MVOP_CMD_SET_HSIZE_X4_DUPLICATE,
	///< 0x10E Enable/Disable VSize x4 duplicate.
	E_MVOP_CMD_SET_BASE_ADD_MULTI_VIEW,
	///< 0x10F Set Muiltiple view Base Address for 3D L/R mode.
	E_MVOP_CMD_SET_FIRE_MVOP,
	///< 0x110 Force load MVOP register in.
	E_MVOP_CMD_SET_VC1_RANGE_MAP,
	///< 0x111 Set VC1 Range Map Luma/Chroma Address
	E_MVOP_CMD_SET_POWER_STATE,
	///< 0x112 Set MVOP STR(suspend/resume)
	E_MVOP_CMD_SET_420_BW_SAVING_MODE,
	///< 0X113 Set MVOP BW SAVING MODE(0:original 1:save 1/4 bw)
	E_MVOP_CMD_SET_EVD_BASE_ADD,
	///< 0x114 Set 10bits or 8bits base addresss for EVD.
	E_MVOP_CMD_SET_EVD_FEATURE,
	///< 0x115 Set EVD Mode and 10bits or 8 bits Enable.
	E_MVOP_CMD_SET_MVD_LATE_REPEAT,
	///< 0x116 Set repeat previous frame if HVD/MVD can not finish vsync.
	E_MVOP_CMD_SET_HANDSHAKE_MODE,
	///< 0x117 TV chip support handshake mode after muji and monet.
	E_MVOP_CMD_SET_FRC_OUTPUT,
	///< 0x118 Reset MVOP output timing.
	E_MVOP_CMD_SET_XC_GEN_TIMING,
	///< 0x119 Set timing from xc(true) or from mvop(false).
	E_MVOP_CMD_SET_RESET_SETTING,
	///< 0x11A Reset mvop setting from for exit.
	E_MVOP_CMD_SET_SEL_OP_FIELD,
	///< 0x11B Set mvop output filed (new for sub).
	E_MVOP_CMD_SET_SIZE_FROM_MVD,
	///< 0x11C Set mvop output h/w size from vdec (new for sub).
	E_MVOP_CMD_SET_CROP_START_POS,
	///< 0x11D Set mvop crop start x/y (new for sub).
	E_MVOP_CMD_SET_IMAGE_WIDTH_HEIGHT,
	///< 0x11E Set mvop crop  x/y size (new for sub).
	E_MVOP_CMD_SET_INV_OP_VS,
	///< 0x11F Set mvop vsync inverse (new for sub).
	E_MVOP_CMD_SET_FORCE_TOP,
	///< 0x120 Set mvop OP only top field (new for sub).
	E_MVOP_CMD_SET_STREAM_INFO,
	///< 0x121 Set stream infomation before inputcfg and outputcfg,
	//clear in the end of outputcfg.
	E_MVOP_CMD_SET_FORCE_P_MODE,
	///< 0x122 Set MVOP is force p mode for bbc ip change.
	E_MVOP_CMD_SET_EXT_FPS,
	///< 0x123 Set MVOP is fps before setoutputcfg.
	E_MVOP_CMD_SET_INTERLACE_FIELD_MODE,
	///< 0x124 Field mode / Frame mode change for C2
	E_MVOP_CMD_SET_UV_SWAP,
	///< 0x125 UV Swap: enable for yvyu
	E_MVOP_CMD_SET_XC_H_DOWN_SCALE,
	///< 0x126 Set XC Is Hsize dowb scaling.
	E_MVOP_CMD_SET_10B_8B_DITHER,
	///< 0x127 Set mvop 10 bits to 8 bits dither mode.
	E_MVOP_CMD_SET_MVOP_2P,
	///< 0x128 Set mvop 2p mode, for macan ve sw patch:2580x1440.
	E_MVOP_CMD_SET_SAVE_MIU_AREA,
	///< 0x129 Set mvop miu save area.
	E_MVOP_CMD_SET_MFDEC_ROTATE,
	///< 0x12A Set mfdec rotate(90 deg. 180 deg. 270 deg).
	E_MVOP_CMD_SET_ENABLE_FOD,
	///< 0x12B Set Enable Field Order Detect: send signal to wb.
	E_MVOP_CMD_SET_FDMASK,
	///< 0x12C Set mvop fdmask.

	// This category of command is only called by dms in interrupt duration.
	// DMS and MVOP is always in the same mode(ex. kernel/user mode),
	// Utopia kernel mode interrupt could not get mutex (ex.
	//UtopiaResourceObtain, for kernel team lock design in kernel space)
	// Otherwise might casue system stucked.
	// All commands that dms calling in interrupt please declare
	//in this category please.
	// Ref. 6a4397c52f968f09069cc3f747ca4a11b7304e67 for kernel
	//mode interrupt protection.
	E_MVOP_CMD_SET_DMS_TYPE     = 0x200,
	E_MVOP_CMD_SET_DMS_INT,                    ///< 0x201 Set DMS Interrupt.
	E_MVOP_CMD_SET_DMS_END      = 0x2ff,

	E_MVOP_CMD_GET_TYPE         = 0x400,
	E_MVOP_CMD_GET_3DLR_ALT_OUT,
	///< 0x401 Query if it is 3D L/R alternative lines output.
	E_MVOP_CMD_GET_MIRROR_MODE,
	///< 0x402 Get Mirror Mode
	E_MVOP_CMD_GET_3DLR_2ND_CFG,
	///< 0x403 Get if 3D LR 2nd pitch is enabled.
	E_MVOP_CMD_GET_VSIZE_DUPLICATE,
	///< 0x404 Get if VSize duplicate is enabled.
	E_MVOP_CMD_GET_BASE_ADD_BITS,
	///< 0x405 Get bits of y/uv address.
	E_MVOP_CMD_GET_TOTAL_MVOP_NUM,
	///< 0X406 Get how many mvop in this chip.
	E_MVOP_CMD_GET_MAXIMUM_CLK,
	///< 0x407 Get maximum mvop(dc0) clock.
	E_MVOP_CMD_GET_CURRENT_CLK,
	///< 0x408 Get current mvop(dc0) clock.
	E_MVOP_CMD_GET_BASE_ADD_MULTI_VIEW,
	///< 0x409 Get base address of main/sub view.
	E_MVOP_CMD_GET_TOP_FIELD_IMAGE_VSTART,
	///< 0x40A Get top field vstart.
	E_MVOP_CMD_GET_BOTTOM_FIELD_IMAGE_VSTART,
	///< 0x40B Get bot field vstart.
	E_MVOP_CMD_GET_VCOUNT,
	///< 0x40C Get tgen vcount.
	E_MVOP_CMD_GET_HANDSHAKE_MODE,
	///< 0x40D Get Handshaking mode status.
	E_MVOP_CMD_GET_MAX_FPS,
	///< 0x40E Get mvop supporting max framerate.
	E_MVOP_CMD_GET_CROP_FOR_XC,
	///< 0x40F Set crop and Get crop infomation. Without set register.
	E_MVOP_CMD_GET_MVOP_SW_CROP_ADD,
	///< 0x410 If hw not support crop, sw patch for base address.
	E_MVOP_CMD_GET_IS_SW_CROP,
	///< 0x411 If hw not support crop or not?
	E_MVOP_CMD_GET_OUTPUT_HV_RATIO,
	///< 0x412 Get mvop output HV ratio.
	E_MVOP_CMD_GET_INPUT,
	///< 0x413 LG
	E_MVOP_CMD_GET_SUB_INPUT,
	///< 0x414 LG
	E_MVOP_CMD_GET_IS_XC_GEN_TIMING,
	///< 0x415 Get if timing is from xc (or from mvop).
	E_MVOP_CMD_GET_IS_MVOP_AUTO_GEN_BLACK,
	///< 0x416 Get if mvop support auto bk background.
	E_MVOP_CMD_GET_4K2K2PMODE,
	///< 0x417 Get mvop 4k2k 2p mode or not.
	E_MVOP_CMD_GET_ISMVOPSENDDATA,
	///< 0x418 Get if mvop hw is sending data or not. (check 1st frame to xc)
	E_MVOP_CMD_GET_IS_BOT_FIELD,
	///< 0x419 Get mvop field signal for mcu mode.
	E_MVOP_CMD_GET_SOURCE_WIDTH_HEIGHT,
	///< 0x41A Get MVOP display source width/height.
	E_MVOP_CMD_GET_MFDEC_ROTATE,
	///< 0x41B Get MVOP MFDEC Rotate status.
	E_MVOP_CMD_GET_DATA_HV_SIZE,
	///< 0x41C Get data Width and Height.
	E_MVOP_CMD_GET_HVTT_FOR_DYNAMIC_FPS,
	///< 0x41D Get MVOP MFDEC Rotate status.
	E_MVOP_CMD_GET_VIDEO_DELAY,
	///< 0x41E Get MVOP video delay time for avsync.
	E_MVOP_CMD_GET_RGB_FMT,
	///< 0x41F Get RGB format: MVOP_RgbFormat.
	E_MVOP_CMD_GET_IS_DMS_FIRE_IN_TIME,
	///< 0x420 Get is DMS have enough time to fire current
	///setting and effect in next vsync.
	E_MVOP_CMD_GET_SLT_GOLDEN,
	///< 0x421 Get MVOP SLT golden value by chip.

	// Same as E_MVOP_CMD_SET_DMS_TYPE
	E_MVOP_CMD_GET_DMS_TYPE     = 0x500,
	E_MVOP_CMD_GET_DMS_INT,
	///< 0x501 Get DMS Interrupt.
	E_MVOP_CMD_GET_DMS_IS_SUPPORT_HSD,
	///< 0x502 For DMS Get HSD Capability(doing in mvop task).
	E_MVOP_CMD_GET_DMS_END      = 0x5ff,

	E_MVOP_CMD_PRESET_TYPE      = 0x800,
	E_MVOP_CMD_PRESET_MVOP01_SWITCH,
	///< 0x801 Switch main sub mvop bank register address.
	///main api = 0x1013, sub api = 0x1014
} MVOP_Command;

typedef enum {
	E_MVOP_SYNCMODE,
	E_MVOP_FREERUNMODE,
	E_MVOP_HARDWAREMODE,
	E_MVOP_27MHZ                = 27000000ul,
	E_MVOP_54MHZ                = 54000000ul,
	E_MVOP_72MHZ                = 72000000ul,
	E_MVOP_108MHZ               = 108000000ul,
	E_MVOP_123MHZ               = 123000000ul,
	E_MVOP_144MHZ               = 144000000ul,
	E_MVOP_160MHZ               = 160000000ul,
	E_MVOP_172MHZ               = 172000000ul,
	E_MVOP_192MHZ               = 192000000ul,
	E_MVOP_320MHZ               = 320000000ul,
	E_MVOP_690MHZ               = 690000000ul,
	E_MVOP_FREQ_NOT_SUPPORT,
} MVOP_FREQUENCY;

// Interrupt
typedef enum {
	E_MVOP_INT_NONE             = 0,
	E_MVOP_INT_BUFF_UF          = 0x01,         ///< buffer underflow
	E_MVOP_INT_BUFF_OF          = 0x02,         ///< buffer overflow
	E_MVOP_INT_VSYNC            = 0x04,         ///< Vsync interrupt
	E_MVOP_INT_HSYNC            = 0x08,         ///< Hsync interrupt
	E_MVOP_INT_RDY              = 0x10,         ///< DC ready interrupt
	E_MVOP_INT_FDCHNG           = 0x20,         ///< field change
	E_MVOP_INT_MVDVS            = 0x80,         ///< MVD vsync interrupt

	E_MVOP_INT_3A_NONE_EX       = 0x100,
	E_MVOP_INT_VSIZE_EX         = 0x104,
	E_MVOP_INT_DC_START         = 0x108,        ///< New interrupt from M7632
} MVOP_IntType;

typedef struct DLL_PACKED {
	MS_U8  u8StrVer;
	MS_U8  u8Rsrvd;
	MS_U16 u16HSize;
	MS_U16 u16VSize;
	MS_U16 u16Fps;
} MVOP_CapInput;

typedef struct {
	MS_BOOL  bEnable3DLRALT;
	MS_BOOL  bEnable3DLRSBS;
	MS_BOOL  bEnableVdup;
	MS_BOOL  bEnableVx4;
	MS_BOOL  bEnableHx4;
	MS_BOOL  bEnableVsyncMode;
	MS_BOOL  bEnableHMirror;
	MS_BOOL  bEnableVMirror;
	MS_BOOL  bEnableBWSave;
	MS_BOOL  bEnableRptPreVsync;
} MVOP_ComdFlag;

typedef enum {
	E_MVOP_HS_NOT_SUPPORT       = 0,
	E_MVOP_HS_ENABLE            = 0x01,
	E_MVOP_HS_DISABLE           = 0x02,
	E_MVOP_HS_INVALID_PARAM     = 0x03,
} MVOP_HSMode;

typedef struct DLL_PACKED {
	MS_U16 u16HSize;
	MS_U16 u16VSize;
	MS_BOOL b3DTB;
	MS_BOOL b3DSBS;
	MS_BOOL bReserve[6];
} MVOP_GetMaxFps;

typedef enum {
	E_MVOP_PRO                  = 0,
	E_MVOP_INT_TB_ONE_FRAME     = 0x01,
	E_MVOP_INT_TB_SEP_FRAME     = 0x02,

} MVOP_OutputImodeType;

typedef struct DLL_PACKED {
	MS_U16 u16XStart;
	MS_U16 u16YStart;
	MS_U16 u16XSize;
	MS_U16 u16YSize;
	MS_U32 reserve[2];
} MVOP_XCGetCrop;

typedef struct {
	MS_U32 u32MSBStartY0;
	MS_U32 u32MSBStartUV0;
	MS_U32 u32MSBStartY1;
	MS_U32 u32MSBStartUV1;

	MS_U32 u32LSBStartY0;
	MS_U32 u32LSBStartUV0;
	MS_U32 u32LSBStartY1;
	MS_U32 u32LSBStartUV1;
	MVOP_XCGetCrop stXCCrop;
	MS_U16 u16XCap;
	MS_U16 u16YCap;
	MS_U32 reserve[4];
	MS_U32 u32MFCodecInfo;
	MS_U32 u32LumaMFCbitlen;
	MS_U32 u32ChromaMFCbitlen;
	MS_U16 u16Pitch;
	MS_U16 u16Pitch_2bit;
} MVOP_XCGetCropMirAdd;

typedef struct {
	float fHratio;
	float fVratio;
	float reserve0[4];
	MS_U32 reserve1[2];
} MVOP_OutputHVRatio;

typedef struct {
	MS_U16 u16Xpos;
	MS_U16 u16Ypos;
} MVOP_SetCropPos;

typedef struct {
	MS_U16 u16Width;
	MS_U16 u16Height;
} MVOP_SetImageWH;

typedef enum {
    E_MVOP_10to8_NONE           = 0,
    E_MVOP_10to8_ROUND_TOSS     = 0x01,
    E_MVOP_10to8_ROUND_RANDOM   = 0x02,
    E_MVOP_10to8_ROUND_HALF     = 0x03,
    E_MVOP_10to8_PSEU_STOP      = 0x04,
} MVOP_10to8Mode;

typedef enum {
	E_MVOP_SET_SI_NONE          = 0,
	E_MVOP_SET_SI_DV_SINGLE     = 0x01,
	E_MVOP_SET_SI_DV_DUAL       = 0x02,
	E_MVOP_SET_SI_NON_DV_HDR    = 0x03
} MVOP_SetStreamInfo;

typedef enum {
	E_MVOP_MAIN_WIN             = 0,
	///< main window if with PIP or without PIP
	E_MVOP_SUB_WIN              = 1,            ///< sub window if PIP

	E_MVOP_MULTI_WIN0,                          ///< multiWindow0
	E_MVOP_MULTI_WIN1,                          ///< multiWindow1
	E_MVOP_MULTI_WIN2,                          ///< multiWindow2
	E_MVOP_MULTI_WIN3,                          ///< multiWindow3
	E_MVOP_MULTI_WIN4,                          ///< multiWindow4
	E_MVOP_MULTI_WIN5,                          ///< multiWindow5
	E_MVOP_MULTI_WIN6,                          ///< multiWindow6
	E_MVOP_MULTI_WIN7,                          ///< multiWindow7

	E_MVOP_MULTI_WIN8,                          ///< multiWindow8
	E_MVOP_MULTI_WIN9,                          ///< multiWindow9
	E_MVOP_MULTI_WIN10,                         ///< multiWindow10
	E_MVOP_MULTI_WIN11,                         ///< multiWindow11
	E_MVOP_MULTI_WIN12,                         ///< multiWindow12
	E_MVOP_MULTI_WIN13,                         ///< multiWindow13
	E_MVOP_MULTI_WIN14,                         ///< multiWindow14
	E_MVOP_MULTI_WIN15,                         ///< multiWindow15

	E_MVOP_MAX_WIN
	///< The max support window
} MVOP_DMS_WIN;

typedef struct DLL_PACKED {
	MS_U32 u32ApiDMSADD_Version;
	///< Version of current structure. Please always set to
	///"API_MVOP_DMS_DIPS_ADD_VERSION" as input
	MS_U16 u16ApiDMSADD_Length;
	///< Length of this structure,
	///u16ApiDMSADD_Length=sizeof(MVOP_DMSDisplayADD)

	MS_U8  u8FRAME_ID;
	MS_PHY u32MSB_FB_ADDR[2];
	///< 0:8 bit y address, 1:8 bit uv address
	MS_U8  u8DMSB_FB_MIU[2];
	///< 0:8 bit y miu select, 1:8 bit uv miu select

	MS_U16 u32BIT_DEPTH[2];
	///< 0:y display depth(8 bits or 10 bits), 1:uv display depth(8 bits or 10 bits)
	MS_PHY u32LSB_FB_ADDR[2];
	///< 0:2 bit y address, 1:2 bit uv address
	MS_U8  u32LSB_FB_MIU[2];
	///< 0:2 bit y miu select, 1:2 bit uv miu select

	MS_BOOL bMFDEC_EN;
	MS_U8   u8MFDEC_ID;
	MS_U32  u32UNCOMPRESS_MODE;
	MS_PHY  u32BITLEN_FB_ADDR;
	///< 0:8 bit EL y address, 1:8 bit EL uv address
	MS_U8   u8BITLEN_FB_MIU;
	///< 0:8 bit EL y address, 1:8 bit EL uv address
	MS_U32  u32BITLEN_FB_PITCH;

	MS_BOOL bDualDVEN;
	MS_PHY  u32DualDV_EL_FB[2];
	///< 0:8 bit EL y address, 1:8 bit EL uv address
	MS_U8   u8DualDV_EL_MIU[2];
	///< 0:8 bit EL y miu select, 1:8 bit EL uv miu select

	MS_BOOL bEN3D;
	MS_PHY  u32RViewFB[2];
	///< 0:R view y address, 1:R view uv address

	MS_BOOL bOutputIMode;
	MS_U8   u8FDMask;
	MS_U8   u8BotFlag;
	MS_BOOL bXCBOB_EN;
	MS_BOOL bBLEN_SHT_MD;
	///< 0:H265 mfdec shift 4 lines;1:vp9 mfdec shift 8 lines
	MS_BOOL bInputIMode;
	///< 0x20[2] = 1: progressive for force p mdoe
	MS_U64  u64TimeStamp;
	///< New timestamp feature from M7632
	MS_BOOL bForceTimeStamp;
	///< 1: timestamp from u64TimeStamp, 0: none
	MVOP_OutputImodeType eIModeType;
	///< force p mode dynamic switch interlace type. ver.6
	MS_BOOL bAV1Mode;
	///< AV1 + mfdec compress mode. ver.7
	MS_BOOL bAllowIntrabc;
	///< AV1 + intrabc stream. ver.7
	MS_BOOL bOneField;
	///< OneField control. ver.8
} MVOP_DMSDisplayADD;

typedef struct DLL_PACKED {
	MS_U32 u32ApiDMSSize_Version;
	///< Version of current structure. Please always set to
	///"API_MVOP_DMS_DISP_SIZE_VERSION" as input
	MS_U16 u16ApiDMSSize_Length;
	///< Length of this structure,
	///u16ApiDMSSize_Length=sizeof(MVOP_DMSDisplaySize)

	MS_U16 u16Width;                            ///< MSB/Dual DV BL width
	MS_U16 u16Height;                           ///< MSB/Dual DV BL height
	MS_U16 u16Pitch[2];                         ///< MSB/Dual DV BL pitch
	MS_U8  U8DSIndex;                           ///< DS index

	MS_BOOL bDualDV_EN;
	MS_U16 u16Width_EL;                         ///< Dual DV EL width
	MS_U16 u16Height_EL;                        ///< Dual DV EL height
	MS_U16 u16Pitch_EL[2];                      ///< Dual DV EL pitch

	MS_BOOL bHDup;
	MS_BOOL bVDup;

	MS_BOOL bHSD;                               ///< HSD enable, ver. 2
	MS_U16  u16OutputWidth;
	///< mvop output width(for calculate hsd index)
} MVOP_DMSDisplaySize;

typedef struct DLL_PACKED {
	MS_U32 u32ApiDMSStream_Version;
	///< Version of current structure.
	///Please always set to "API_MVOP_DMS_STREAM_VERSION" as input
	MS_U16 u16ApiDMSStream_Length;
	///< Length of this structure,
	///u16ApiDMSStream_Length=sizeof(MVOP_DMSStreamINFO)

	MVOP_TileFormat eTileFormat;
	MS_BOOL bIs422Mode;
	MS_BOOL bIsDRAMCont;
	MS_BOOL bDDR4_REMAP;
	MS_BOOL bDS_EN;
} MVOP_DMSStreamINFO;

typedef struct DLL_PACKED {
	MS_U32 u32ApiDMSCrop_Version;
	///< Version of current structure. Please always set to
	///"API_MVOP_DMS_CROP_VERSION" as input
	MS_U16 u16ApiDMSCrop_Length;
	///< Length of this structure,
	///u16ApiDMSCrop_Length=sizeof(MVOP_DMSCropINFO)

	MVOP_SetCropPos stCropStart;
	MVOP_SetImageWH stCropSize;
} MVOP_DMSCropINFO;

typedef struct DLL_PACKED {
	MS_U32 u32DyncFPS_Version;
	///< Version of current structure. Please always set to
	///"CMD_MVOP_DYNAMIC_FPS_VERSION" as input
	MS_U16 u16DyncFPS_Length;
	///< Length of this structure,
	///u16DyncFPS_Length=sizeof(MVOP_DYNAMIC_FPS)

	MS_U16 u16HTotal;
	MS_U16 u16VTotal;
	MS_U16 u16HorSize;
	MS_U16 u16VerSize;
	MS_U32 u16FrameRate;
	MS_BOOL bHdup;
} MVOP_DYNAMIC_FPS;

typedef struct DLL_PACKED {
	MS_U32 u32DMSIntStatus_Version;
	///< Version of current structure. Please always set to
	///"API_MVOP_DMS_INTSTATUS_VERSION" as input
	MS_U16 u16DMSIntStatus_Length;
	///< Length of this structure,
	///u16DMSIntStatus_Length=sizeof(MVOP_DMSIntStatus)

	MS_U8 u8IntReg0x1F;
	MS_U8 u8IntReg0x3A;
} MVOP_DMSIntStatus;

typedef struct DLL_PACKED {
	MS_U32 u32IsFireOnTime_Version;
	///<Version of current structure. Please always set to
	//"CMD_MVOP_IS_DMS_FIRE_VERSION" as input
	MS_U16 u16IsFireOnTime_Length;
	///<Length of this structure,
	//u16PanelInfoEX_Length=sizeof(MVOP_DMSFireOnTime)

	MS_U32 u32NeedTime;
	MS_BOOL bIsOnTime;
	MS_U32 u32Vcount;
} MVOP_DMSFireOnTime;
//----------------------------------------------------------------------------
//---------------------
//  Function and Variable
//----------------------------------------------------------------------------
//---------------------

//----------------------------------------------------------------------------
//---------------------
/// Map Context for Multi Process Use.
/// @ingroup MVOP_Basic
/// @param stHd \b IN : Devide ID(main mvop or sub).
/// @return void
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_GetConfig(MVOP_Handle *stHd);

// Basic Function
//----------------------------------------------------------------------------
//---------------------
/// Initial Main MVOP.
/// @ingroup MVOP_Basic
/// @param void \b IN : void
/// @return void
//----------------------------------------------------------------------------
//---------------------

void SYMBOL_WEAK MDrv_MVOP_Init(void);

//----------------------------------------------------------------------------
//---------------------
/// Disable Main MVOP clock.
/// @ingroup MVOP_Basic
/// @param void \b IN : void
/// @return void
//----------------------------------------------------------------------------
//---------------------
void SYMBOL_WEAK MDrv_MVOP_Exit(void);
//----------------------------------------------------------------------------
//---------------------
/// Main MVOP power switch; start to generate timing.
/// @ingroup MVOP_Basic
/// @param MS_BOOL \b IN : enable or disable MVOP.
/// @return void
//----------------------------------------------------------------------------
//---------------------
void SYMBOL_WEAK MDrv_MVOP_Enable(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP Input stream information and reset ex-stream status.
/// @ingroup MVOP_Basic
/// @param in \b IN : Set mvop input: Hardwire mode or MCU mode / Codec type.
/// @param pCfg \b IN : Set mvop input: MCU mode data format setting.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result SYMBOL_WEAK MDrv_MVOP_SetInputCfg(MVOP_InputSel in,
	MVOP_InputCfg *pCfg);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP Output timing information.
/// @ingroup MVOP_Basic
/// @param pstVideoStatus \b IN : Set mvop output Timing.
/// @param bEnHDup \b IN : Is Hsize duplicated?
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result SYMBOL_WEAK MDrv_MVOP_SetOutputCfg(MVOP_VidStat *pstVideoStatus,
	MS_BOOL bEnHDup);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP Background color as black.
/// @ingroup MVOP_Basic
/// @param void \b IN : void.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_EnableBlackBG(void);
// SetCommand Function
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP UV forwarding one line.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable or disable.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_EnableUVShift(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP only Luma data(Black/white feature).
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable or disable.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SetMonoMode(MS_BOOL bEnable);
// GetCommand Function
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP source Width.
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: Width.
//----------------------------------------------------------------------------
//---------------------
MS_U16 SYMBOL_WEAK MDrv_MVOP_GetHSize(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP source Height.
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: Height.
//----------------------------------------------------------------------------
//---------------------
MS_U16 SYMBOL_WEAK MDrv_MVOP_GetVSize(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP the timing of xc data starting(capture win. h start).
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: timing of data start in Htt.
//----------------------------------------------------------------------------
//---------------------
MS_U16 SYMBOL_WEAK MDrv_MVOP_GetHStart(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP the timing of xc data starting(capture win. v start).
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: timing of data start in Vtt.
//----------------------------------------------------------------------------
//---------------------
MS_U16 SYMBOL_WEAK MDrv_MVOP_GetVStart(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP timing is interlace or not.
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: true or false.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_GetIsInterlace(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP H data is duplicated or not.
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: true or false.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_GetIsHDuplicate(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP is enable or not.
/// @ingroup MVOP_GetCommand
/// @param pbEnable \b IN : Is enable or not.
/// @return: true or false.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_GetIsEnable(MS_BOOL *pbEnable);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP output timing.
/// @ingroup MVOP_GetCommand
/// @param pMVOPTiming \b IN : the pointer of timing struc.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result SYMBOL_WEAK MDrv_MVOP_GetOutputTiming(MVOP_Timing *pMVOPTiming);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP library version.
/// @ingroup MVOP_GetCommand
/// @param ppVersion \b IN : mvop library version no..
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_GetLibVer(const MSIF_Version **ppVersion);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP HW capability.
/// @ingroup MVOP_GetCommand
/// @param u16HSize \b IN : input height.
/// @param u16VSize \b IN : input height.
/// @param u16Fps \b IN : input framerate.
/// @return MS_BOOL: support this timing or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_CheckCapability(MS_U16 u16HSize, MS_U16 u16VSize,
	MS_U16 u16Fps);
//   Functions for the 3rd MVOP }
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP maximum of horizontal offset.
/// @ingroup MVOP_GetCommand
/// @param u16HSize \b IN : input height.
/// @param u16VSize \b IN : input height.
/// @param u16Fps \b IN : input framerate.
/// @return MS_BOOL: maximum of horizontal offset.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_GetMaxHOffset(MS_U16 u16HSize, MS_U16 u16VSize, MS_U16 u16Fps);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP maximum of vertical offset.
/// @ingroup MVOP_GetCommand
/// @param u16HSize \b IN : input height.
/// @param u16VSize \b IN : input height.
/// @param u16Fps \b IN : input framerate.
/// @return MS_BOOL: maximum of vertical offset.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_GetMaxVOffset(MS_U16 u16HSize, MS_U16 u16VSize, MS_U16 u16Fps);
// MVOP_Debug
//----------------------------------------------------------------------------
//---------------------
/// Set MVOP debug level.
/// @ingroup MVOP_SetCommand
/// @param level \b IN: level.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SetDbgLevel(MS_U8 level);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP clock infomation.
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return MVOP_DrvInfo: the pointer of clock information.
//----------------------------------------------------------------------------
//---------------------
const MVOP_DrvInfo *MDrv_MVOP_GetInfo(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP current setting status.
/// @ingroup MVOP_GetCommand
/// @param pMVOPStat \b IN : the pointer of mvop status.
/// @return MS_BOOL: get is status successful or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL SYMBOL_WEAK MDrv_MVOP_GetStatus(MVOP_DrvStatus *pMVOPStat);
//----------------------------------------------------------------------------
//---------------------
/// Set MVOP Pattern type.
/// @ingroup MVOP_SetCommand
/// @param enMVOPPattern \b IN: pattern type.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void SYMBOL_WEAK MDrv_MVOP_SetPattern(MVOP_Pattern enMVOPPattern);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP tile format.
/// @ingroup MVOP_SetCommand
/// @param eTileFmt \b IN: tile format.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SetTileFormat(MVOP_TileFormat eTileFmt);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP current display timing infomation from register.
/// @ingroup MVOP_GetCommand
/// @param pDstInfo \b IN : the pointer of mvop timing.
/// @param u32SizeofDstInfo \b IN : the size of pDstInfo.
/// @return MS_BOOL: get is status successful or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_GetDstInfo(MVOP_DST_DispInfo *pDstInfo,
	MS_U32 u32SizeofDstInfo);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP Vtt fixed.
/// @ingroup MVOP_SetCommand
/// @param u16FixVtt \b IN: Vtt.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SetFixVtt(MS_U16 u16FixVtt);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP MMIO Map base address.
/// @ingroup MVOP_SetCommand
/// @param void \b IN: void.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SetMMIOMapBase(void);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP access which miu.
/// @ingroup MVOP_Basic
/// @param u8Miu \b IN : miu 0/1/2...
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_MiuSwitch(MS_U8 u8Miu);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP MCU mode YUV address.
/// @ingroup MVOP_SetCommand
/// @param u32YOffset \b IN: Y address.
/// @param u32UVOffset \b IN: UV address.
/// @param bProgressive \b IN: Is progressive.
/// @param b422pack \b IN: Is 422 packed or not.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SetBaseAdd(MS_PHY u32YOffset, MS_PHY u32UVOffset,
	MS_BOOL bProgressive, MS_BOOL b422pack);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP elect output field for display one field. default: mvop t/b
//toggle, update one field to both t/b xc buffer.
/// @ingroup MVOP_SetCommand
/// @param void \b IN: void.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SEL_OP_FIELD(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP hsize/vsize from vdec DIU/WB.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: Enable/Disable.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SetRegSizeFromMVD(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP crop x/y start.
/// @ingroup MVOP_SetCommand
/// @param u16Xpos \b IN: x start.
/// @param u16Ypos \b IN: y start.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SetStartPos(MS_U16 u16Xpos, MS_U16 u16Ypos);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP crop x/y size.
/// @ingroup MVOP_SetCommand
/// @param u16Width \b IN: x size.
/// @param u16Height \b IN: y size.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SetImageWidthHight(MS_U16 u16Width, MS_U16 u16Height);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP mirror mode.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable/disable.
/// @param eMirrorMode \b IN: mirror mode type.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void SYMBOL_WEAK MDrv_MVOP_SetVOPMirrorMode(MS_BOOL bEnable,
	MVOP_DrvMirror eMirrorMode);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP output vsync inversed.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable/disable.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_INV_OP_VS(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP output only top field.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable/disable.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_FORCE_TOP(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP enable freerun mode.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable/disable.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_EnableFreerunMode(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP cuurent frame base address(MCU mode only).
/// @ingroup MVOP_GetCommand
/// @param u32YOffset \b IN : the pointer of Luma address.
/// @param u32UVOffset \b IN : the pointer of Chroma address.
/// @return void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_GetBaseAdd(MS_PHY *u32YOffset, MS_PHY *u32UVOffset);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP current setting status.
/// @ingroup MVOP_GetCommand
/// @param pMVOPStat \b IN : the pointer of mvop status.
/// @return MS_BOOL: get is status successful or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubGetStatus(MVOP_DrvStatus *pMVOPStat);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP the timing of xc data starting(capture win. h start).
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: timing of data start in Htt.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_SubGetHStart(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP the timing of xc data starting(capture win. v start).
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: timing of data start in Vtt.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_SubGetVStart(void);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP Pattern type.
/// @ingroup MVOP_SetCommand
/// @param enMVOPPattern \b IN: pattern type.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SubSetPattern(MVOP_Pattern enMVOPPattern);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP tile format.
/// @ingroup MVOP_SetCommand
/// @param eTileFmt \b IN: tile format.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubSetTileFormat(MVOP_TileFormat eTileFmt);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP MMIO Map base address.
/// @ingroup MVOP_SetCommand
/// @param void \b IN: void.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubSetMMIOMapBase(void);
//----------------------------------------------------------------------------
//---------------------
/// Initial Sub MVOP.
/// @ingroup MVOP_Basic
/// @param void \b IN : void
/// @return void
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SubInit(void);
//----------------------------------------------------------------------------
//---------------------
/// Disable Sub MVOP clock.
/// @ingroup MVOP_Basic
/// @param void \b IN : void
/// @return void
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SubExit(void);
//----------------------------------------------------------------------------
//---------------------
/// Sub MVOP power switch; start to generate timing.
/// @ingroup MVOP_Basic
/// @param MS_BOOL \b IN : enable or disable MVOP.
/// @return void
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SubEnable(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP is enable or not.
/// @ingroup MVOP_GetCommand
/// @param pbEnable \b IN : Is enable or not.
/// @return: true or false.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_SubGetIsEnable(MS_BOOL *pbEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP UV forwarding one line.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable or disable.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SubEnableUVShift(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP Background color as black.
/// @ingroup MVOP_Basic
/// @param void \b IN : void.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SubEnableBlackBG(void);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP only Luma data(Black/white feature).
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable or disable.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SubSetMonoMode(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP Input stream information and reset ex-stream status.
/// @ingroup MVOP_Basic
/// @param in \b IN : Set mvop input: Hardwire mode or MCU mode / Codec type.
/// @param pCfg \b IN : Set mvop input: MCU mode data format setting.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_SubSetInputCfg(MVOP_InputSel in, MVOP_InputCfg *pCfg);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP Output timing information.
/// @ingroup MVOP_Basic
/// @param pstVideoStatus \b IN : Set mvop output Timing.
/// @param bEnHDup \b IN : Is Hsize duplicated?
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_SubSetOutputCfg(MVOP_VidStat *pstVideoStatus,
	MS_BOOL bEnHDup);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP output timing.
/// @ingroup MVOP_GetCommand
/// @param pMVOPTiming \b IN : the pointer of timing struc.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_SubGetOutputTiming(MVOP_Timing *pMVOPTiming);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP source Width.
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: Width.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_SubGetHSize(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP source Height.
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: Height.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_SubGetVSize(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP timing is interlace or not.
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: true or false.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubGetIsInterlace(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP H data is duplicated or not.
/// @ingroup MVOP_GetCommand
/// @param void \b IN : void.
/// @return: true or false.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubGetIsHDuplicate(void);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP HW capability.
/// @ingroup MVOP_GetCommand
/// @param u16HSize \b IN : input height.
/// @param u16VSize \b IN : input height.
/// @param u16Fps \b IN : input framerate.
/// @return MS_BOOL: support this timing or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubCheckCapability(MS_U16 u16HSize, MS_U16 u16VSize,
	MS_U16 u16Fps);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP maximum of horizontal offset.
/// @ingroup MVOP_GetCommand
/// @param u16HSize \b IN : input height.
/// @param u16VSize \b IN : input height.
/// @param u16Fps \b IN : input framerate.
/// @return MS_BOOL: maximum of horizontal offset.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_SubGetMaxHOffset(MS_U16 u16HSize, MS_U16 u16VSize,
	MS_U16 u16Fps);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP maximum of vertical offset.
/// @ingroup MVOP_GetCommand
/// @param u16HSize \b IN : input height.
/// @param u16VSize \b IN : input height.
/// @param u16Fps \b IN : input framerate.
/// @return MS_BOOL: maximum of vertical offset.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_SubGetMaxVOffset(MS_U16 u16HSize, MS_U16 u16VSize,
	MS_U16 u16Fps);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP current display timing infomation from register.
/// @ingroup MVOP_GetCommand
/// @param pDstInfo \b IN : the pointer of mvop timing.
/// @param u32SizeofDstInfo \b IN : the size of pDstInfo.
/// @return MS_BOOL: get is status successful or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubGetDstInfo(MVOP_DST_DispInfo *pDstInfo,
	MS_U32 u32SizeofDstInfo);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP Vtt fixed.
/// @ingroup MVOP_SetCommand
/// @param u16FixVtt \b IN: Vtt.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubSetFixVtt(MS_U16 u16FixVtt);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP access which miu.
/// @ingroup MVOP_Basic
/// @param u8Miu \b IN : miu 0/1/2...
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_SubMiuSwitch(MS_U8 u8Miu);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP mirror mode.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable/disable.
/// @param eMirrorMode \b IN: mirror mode type.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SubSetVOPMirrorMode(MS_BOOL bEnable, MVOP_DrvMirror eMirrorMode);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP enable freerun mode.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable/disable.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubEnableFreerunMode(MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Sub MVOP MCU mode YUV address.
/// @ingroup MVOP_SetCommand
/// @param u32YOffset \b IN: Y address.
/// @param u32UVOffset \b IN: UV address.
/// @param bProgressive \b IN: Is progressive.
/// @param b422pack \b IN: Is 422 packed or not.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_SubSetBaseAdd(MS_PHY u32YOffset, MS_PHY u32UVOffset,
	MS_BOOL bProgressive, MS_BOOL b422pack);
//----------------------------------------------------------------------------
//---------------------
/// Get Sub MVOP cuurent frame base address(MCU mode only).
/// @ingroup MVOP_GetCommand
/// @param u32YOffset \b IN : the pointer of Luma address.
/// @param u32UVOffset \b IN : the pointer of Chroma address.
/// @return void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SubGetBaseAdd(MS_PHY *u32YOffset, MS_PHY *u32UVOffset);

//----------------------------------------------------------------------------
//---------------------
/// Initial Extended MVOP.
/// @ingroup MVOP_Basic
/// @param eID \b IN : Device ID.
/// @param void \b IN : void.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_Init(MVOP_DevID eID, MS_U32 u32InitParam);
//----------------------------------------------------------------------------
//---------------------
/// Disable Extended  MVOP clock.
/// @ingroup MVOP_Basic
/// @param eID \b IN : Device ID.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_Exit(MVOP_DevID eID, MS_U32 u32ExitParam);
//----------------------------------------------------------------------------
//---------------------
/// Extended MVOP power switch; start to generate timing.
/// @ingroup MVOP_Basic
/// @param eID \b IN : Device ID.
/// @param MS_BOOL \b IN : enable or disable MVOP.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_Enable(MVOP_DevID eID, MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended  MVOP Input stream information and reset ex-stream status.
/// @ingroup MVOP_Basic
/// @param eID \b IN : Device ID.
/// @param in \b IN : Set mvop input: Hardwire mode or MCU mode / Codec type.
/// @param pCfg \b IN : Set mvop input: MCU mode data format setting.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_SetInputCfg(MVOP_DevID eID, MVOP_InputSel in,
	MVOP_InputCfg *pCfg);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended  MVOP Output timing information.
/// @ingroup MVOP_Basic
/// @param eID \b IN : Device ID.
/// @param pstVideoStatus \b IN : Set mvop output Timing.
/// @param bEnHDup \b IN : Is Hsize duplicated?
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_SetOutputCfg(MVOP_DevID eID,
	MVOP_VidStat *pstVideoStatus, MS_BOOL bEnHDup);

//----------------------------------------------------------------------------
//---------------------
/// Set Extended MVOP Pattern type.
/// @ingroup MVOP_SetCommand
/// @param eID \b IN : Device ID.
/// @param enMVOPPattern \b IN: pattern type.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_SetPattern(MVOP_DevID eID, MVOP_Pattern enMVOPPattern);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended MVOP tile format.
/// @ingroup MVOP_SetCommand
/// @param eID \b IN : Device ID.
/// @param eTileFmt \b IN: tile format.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_SetTileFormat(MVOP_DevID eID,
	MVOP_TileFormat eTileFmt);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended MVOP UV forwarding one line.
/// @ingroup MVOP_SetCommand
/// @param eID \b IN : Device ID.
/// @param bEnable \b IN: enable or disable.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_EnableUVShift(MVOP_DevID eID, MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended MVOP Background color as black.
/// @ingroup MVOP_Basic
/// @param eID \b IN : Device ID.
/// @param void \b IN : void.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_EnableBlackBG(MVOP_DevID eID);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended MVOP only Luma data(Black/white feature).
/// @ingroup MVOP_SetCommand
/// @param eID \b IN : Device ID.
/// @param bEnable \b IN: enable or disable.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_SetMonoMode(MVOP_DevID eID, MS_BOOL bEnable);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended MVOP Vtt fixed.
/// @ingroup MVOP_SetCommand
/// @param eID \b IN : Device ID.
/// @param u16FixVtt \b IN: Vtt.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_SetFixVtt(MVOP_DevID eID, MS_U16 u16FixVtt);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended MVOP access which miu.
/// @ingroup MVOP_Basic
/// @param eID \b IN : Device ID.
/// @param u8Miu \b IN : miu 0/1/2...
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_MiuSwitch(MVOP_DevID eID, MS_U8 u8Miu);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended MVOP mirror mode.
/// @ingroup MVOP_SetCommand
/// @param eID \b IN : Device ID.
/// @param bEnable \b IN: enable/disable.
/// @param eMirrorMode \b IN: mirror mode type.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_SetVOPMirrorMode(MVOP_DevID eID, MS_BOOL bEnable,
	MVOP_DrvMirror eMirrorMode);
//----------------------------------------------------------------------------
//---------------------
/// Set Extended MVOP enable freerun mode.
/// @ingroup MVOP_SetCommand
/// @param eID \b IN : Device ID.
/// @param bEnable \b IN: enable/disable.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_EnableFreerunMode(MVOP_DevID eID, MS_BOOL bEnable);

//----------------------------------------------------------------------------
//---------------------
/// Get Extended MVOP output timing.
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param pMVOPTiming \b IN : the pointer of timing struc.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//-----------------------------------------------------------------------------
//--------------------
MVOP_Result MDrv_MVOP_EX_GetOutputTiming(MVOP_DevID eID,
	MVOP_Timing *pMVOPTiming);
//----------------------------------------------------------------------------
//---------------------
/// Get Extended MVOP is enable or not.
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param pbEnable \b IN : Is enable or not.
/// @return: true or false.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EX_GetIsEnable(MVOP_DevID eID, MS_BOOL *pbEnable);
//----------------------------------------------------------------------------
//---------------------
/// Get Extended MVOP the timing of xc data starting(capture win. h start).
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param void \b IN : void.
/// @return: timing of data start in Htt.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_EX_GetHStart(MVOP_DevID eID);
//-----------------------------------------------------------------------------
//--------------------
/// Get Extended MVOP the timing of xc data starting(capture win. v start).
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param void \b IN : void.
/// @return: timing of data start in Vtt.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_EX_GetVStart(MVOP_DevID eID);
//----------------------------------------------------------------------------
//---------------------
/// Get Extended MVOP source Width.
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param void \b IN : void.
/// @return: Width.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_EX_GetHSize(MVOP_DevID eID);
//----------------------------------------------------------------------------
//---------------------
/// Get Extended MVOP source Height.
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param void \b IN : void.
/// @return: Height.
//----------------------------------------------------------------------------
//---------------------
MS_U16 MDrv_MVOP_EX_GetVSize(MVOP_DevID eID);
//----------------------------------------------------------------------------
//---------------------
/// Get Extended MVOP timing is interlace or not.
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param void \b IN : void.
/// @return: true or false.
//-----------------------------------------------------------------------------
//--------------------
MS_BOOL MDrv_MVOP_EX_GetIsInterlace(MVOP_DevID eID);
//-----------------------------------------------------------------------------
//--------------------
/// Get Extended MVOP H data is duplicated or not.
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param void \b IN : void.
/// @return: true or false.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_EX_GetIsHDuplicate(MVOP_DevID eID);
//-----------------------------------------------------------------------------
//--------------------
/// Get Extended MVOP current setting status.
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param pMVOPStat \b IN : the pointer of mvop status.
/// @return MS_BOOL: get is status successful or not.
//-----------------------------------------------------------------------------
//--------------------
MS_BOOL MDrv_MVOP_EX_GetStatus(MVOP_DevID eID, MVOP_DrvStatus *pMVOPStat);
#if defined(__aarch64__)
MS_BOOL MDrv_MVOP_EX_CheckCapability(MVOP_DevID eID, MS_U64 u32InParam);
#else
//-----------------------------------------------------------------------------
//--------------------
/// Get Extended MVOP HW capability.
/// @ingroup MVOP_GetCommand
/// @param eID \b IN : Device ID.
/// @param u16HSize \b IN : input height.
/// @param u16VSize \b IN : input height.
/// @param u16Fps \b IN : input framerate.
/// @return MS_BOOL: support this timing or not.
//----------------------------------------------------------------------------
//---------------------
MS_BOOL MDrv_MVOP_EX_CheckCapability(MVOP_DevID eID, MS_U32 u32InParam);
#endif
MS_BOOL MDrv_MVOP_EX_GetDstInfo(MVOP_DevID eID, MVOP_DST_DispInfo *pDstInfo,
	MS_U32 u32SizeofDstInfo);
//   Functions for the 3rd MVOP }
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP enable freerun mode.
/// @ingroup MVOP_SetCommand
/// @param bEnable \b IN: enable/disable.
/// @return: MS_BOOL: setting successfully or not.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_SendBlueScreen(MS_U16 u16Width, MS_U16 u16Height);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP by command.
/// @ingroup MVOP_SetCommand
/// @param stHd \b IN: device id(main/sub..).
/// @param eCmd \b IN: command enum.
/// @param pPara \b IN: command parameters.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result SYMBOL_WEAK MDrv_MVOP_SetCommand(MVOP_Handle *stHd,
	MVOP_Command eCmd, void *pPara);
//----------------------------------------------------------------------------
//---------------------
/// Get Main MVOP information by command.
/// @ingroup MVOP_GetCommand
/// @param stHd \b IN : Devide ID(main mvop or sub).
/// @param eCmd \b IN : command enum.
/// @param pPara \b IN : command parameters.
/// @param u32ParaSize \b IN : size of Parameter structure.
/// @return MVOP_Result
///     - E_MVOP_OK: success
///     - E_MVOP_FAIL: failed
///     - E_MVOP_INVALID_PARAM: input param error.
///     - E_MVOP_NOT_INIT: not initialized yet.
///     - E_MVOP_UNSUPPORTED: input case not support.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result SYMBOL_WEAK MDrv_MVOP_GetCommand(MVOP_Handle *stHd,
	MVOP_Command eCmd, void *pPara, MS_U32 u32ParaSize);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP output clock.
/// @ingroup MVOP_SetCommand
/// @param eFreq \b IN: clock.
/// @return: void.
//----------------------------------------------------------------------------
//---------------------
void MDrv_MVOP_SetFrequency(MVOP_FREQUENCY eFreq);
//----------------------------------------------------------------------------
//---------------------
/// Set Main MVOP STR command.
/// @ingroup MVOP_Basic
/// @param u16PowerState \b IN : command of STR(resume/suspend)
/// @return MS_U32: utopia status.
//-----------------------------------------------------------------------------
//--------------------
MS_U32 MDrv_MVOP_SetPowerState(EN_POWER_MODE u16PowerState);
//----------------------------------------------------------------------------
//---------------------
/// Set MVOP DMS display buffer address command.
/// @ingroup MVOP_Basic
/// @param stHd \b IN : Devide ID(main mvop or sub).
/// @param WIN_ID \b IN : multi-window case, xc display window enum.
/// @param pstMVOPDISPADD \b IN : structure of address infromation.
/// @param fpXCWrite \b IN : if need xc cmdq or ds generate script to set mvop
///register, use xc api; if not(write register by mvop driver), use NULL.
/// @return MS_U32: utopia status.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result SYMBOL_WEAK MDrv_MVOP_DMS_SetDispADDInfo(MVOP_Handle *stHd,
	MVOP_DMS_WIN eWIN_ID, MVOP_DMSDisplayADD *pstMVOPDISPADD,
	HAL_XCWrite fpXCWrite);
//----------------------------------------------------------------------------
//---------------------
/// Set MVOP DMS display buffer size command.
/// @ingroup MVOP_Basic
/// @param stHd \b IN : Devide ID(main mvop or sub).
/// @param WIN_ID \b IN : multi-window case, xc display window enum.
/// @param pstMVOPDISPSIZE \b IN : structure of  size/ds index infromation.
/// @param fpXCWrite \b IN : if need xc cmdq or ds generate script to set mvop
/// register, use xc api; if not(write register by mvop driver), use NULL.
/// @return MS_U32: utopia status.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result SYMBOL_WEAK MDrv_MVOP_DMS_SetDispSizeInfo(MVOP_Handle *stHd,
	MVOP_DMS_WIN WIN_ID, MVOP_DMSDisplaySize *pstMVOPDISPSIZE,
	HAL_XCWrite fpXCWrite);
//----------------------------------------------------------------------------
//---------------------
/// Set MVOP DMS display crop size command.
/// @ingroup MVOP_Basic
/// @param stHd \b IN : Devide ID(main mvop or sub).
/// @param WIN_ID \b IN : multi-window case, xc display window enum.
/// @param pstMVOPSTREAMINFO \b IN : structure of crop infromation.
/// @param fpXCWrite \b IN : if need xc cmdq or ds generate script to set mvop
/// register, use xc api; if not(write register by mvop driver), use NULL.
/// @return MS_U32: utopia status.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result SYMBOL_WEAK MDrv_MVOP_DMS_SetCropInfo(MVOP_Handle *stHd,
	MVOP_DMS_WIN WIN_ID, MVOP_DMSCropINFO *pstMVOPCROPINFO,
	HAL_XCWrite fpXCWrite);
//----------------------------------------------------------------------------
//---------------------
/// Set MVOP DMS display buffer size command.
/// @ingroup MVOP_Basic
/// @param stHd \b IN : Devide ID(main mvop or sub).
/// @param WIN_ID \b IN : multi-window case, xc display window enum.
/// @param pstMVOPSTREAMINFO \b IN : structure of stream infromation.
/// @param fpXCWrite \b IN : if need xc cmdq or ds generate script to set mvop
//register, use xc api; if not(write register by mvop driver), use NULL.
/// @return MS_U32: utopia status.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result SYMBOL_WEAK MDrv_MVOP_DMS_SetStreamInfo(MVOP_Handle *stHd,
	MVOP_DMS_WIN WIN_ID, MVOP_DMSStreamINFO *pstMVOPSTREAMINFO,
	HAL_XCWrite fpXCWrite);

//----------------------------------------------------------------------------
//---------------------
/// Set MVOP DMS display buffer size command.
/// @ingroup MVOP_Basic
/// @param stHd \b IN : Devide ID(main mvop or sub).
/// @param eIntType \e IN : enum of interrupt type.
/// @return MVOP_Result: utopia status.
//----------------------------------------------------------------------------
//---------------------
MVOP_Result MDrv_MVOP_EnableInterrupt_EX(MVOP_Handle *stHd,
	MVOP_IntType eIntType);

#ifdef __cplusplus
}
#endif

#endif // _DRV_MVOP_H_
