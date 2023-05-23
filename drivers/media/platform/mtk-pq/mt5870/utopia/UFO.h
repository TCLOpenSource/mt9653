/* SPDX-License-Identifier: GPL-2.0 */
/*
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
*/

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   UFO.h
/// @brief  MStar Common Interface Header File
/// @author MStar Semiconductor Inc.
/// @note   utopia feature definition file
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _UFO_H_
#define _UFO_H_

/////////////////////////////////////////////////////////
/// UTOPIA CODELINE, DO NOT MODIFY
/////////////////////////////////////////////////////////
#define UFO_PUBLIC_HEADER_700

/////////////////////////////////////////////////////////
/// DMX lib feature define
/////////////////////////////////////////////////////////
#define UFO_DMX_FQ
#define UFO_DMX_TSO
//#define UFO_DMX_TSO2

/////////////////////////////////////////////////////////
/// XC lib feature define
/////////////////////////////////////////////////////////
//#define UFO_XC_FORCEWIRTE
//#define UFO_XC_FORCEWRITE
//#define UFO_XC_HDMI_5V_DETECT
#define UFO_XC_PIP_PATCH_USING_SC1_MAIN_AS_SC0_SUB
#define UFO_XC_PQ_PATH

/*
 * XC Pixelshift new mechanism define
 */
#define UFO_XC_SETPIXELSHIFT

/*
 * XC HDR function
 */
#define UFO_XC_HDR

#ifdef UFO_XC_HDR
/*
 * XC HDR version
 */
#define UFO_XC_HDR_VERSION 2   // for HW supported Dolby/Open HDR
#endif

/*
 * XC test pattern function
 */
#define UFO_XC_TEST_PATTERN

/*
 * XC auto download function
 */
#define UFO_XC_AUTO_DOWNLOAD

/*
 * xc zorder
 */
#define UFO_XC_ZORDER

/*
 * XC SUPPORT FRC
 */
#define UFO_XC_FRC_INSIDE

#define UFO_XC_SET_DSINFO_V0

#define UFO_XC_SETWINDOW_LITE

/*
 * XC SUPPORT DS PQ relat to SUPPORT_PQ_DS
 */
#define UFO_XC_SWDR


/////////////////////////////////////////////////////////
/// GOP lib feature define
/////////////////////////////////////////////////////////
#define UFO_GOP_DIP_PINPON

/////////////////////////////////////////////////////////
/// DIP lib feature define
/////////////////////////////////////////////////////////
#define UFO_DIP_SELECT_TILE_BLOCK

/////////////////////////////////////////////////////////
/// Presetcontrol for TEE
/////////////////////////////////////////////////////////
#define UFO_VDEC_TEE_VPU

/////////////////////////////////////////////////////////
/// GFX function
/////////////////////////////////////////////////////////
#define UFO_GFX_TRIANGLE
#define UFO_GFX_SPAN


/////////////////////////////////////////////////////////
/// MVOP GET SUPPORTED MAX FPS
/////////////////////////////////////////////////////////
#define UFO_MVOP_GET_MAX_FPS

/////////////////////////////////////////////////////////
/// MVOP RESET SETTING
/////////////////////////////////////////////////////////
#define UFO_MVOP_RESET_SETTING



//MI support HDCP2.2
#define UFO_HDCP2_2


/////////////////////////////////////////////////////////
/// Get MVOP HSK Mode Blk screen
/////////////////////////////////////////////////////////
#define UFO_MVOP_GET_IS_MVOP_AUTO_GEN_BLACK

/////////////////////////////////////////////////////////
/// MVOP Dolby HDR
/////////////////////////////////////////////////////////
#define UFO_MVOP_DOLBY_HDR

#define IMPORT_PORTMAPPING

/// XC 3D format, FRC in scaler
/////////////////////////////////////////////////////////
#define UFO_XC_GET_3D_FORMAT

/////////////////////////////////////////////////////////
/// XC SUPPORT Dual miu
/////////////////////////////////////////////////////////
//#define UFO_XC_SUPPORT_DUAL_MIU

/////////////////////////////////////////////////////////
/// MVOP Support Stream Info Setting
/////////////////////////////////////////////////////////
#define UFO_MVOP_STREAM_INFO

/////////////////////////////////////////////////////////
/// MVOP Support FBDec Clk
/////////////////////////////////////////////////////////
#define UFO_MVOP_FB_DEC_CLK

/////////////////////////////////////////////////////////
/// DEMOD feature define
/// support VCM
/////////////////////////////////////////////////////////
#define UFO_SUPPORT_VCM
#define UFO_DEMOD_DVBS_TSCONTROL
#define UFO_DEMOD_DVBS_CUSTOMIZED_DISEQC_SEND_CMD
/////////////////////////////////////////////////////////
/// MDebug feature define
/////////////////////////////////////////////////////////
#define UFO_MDEBUG_SUPPORT

/////////////////////////////////////////////////////////
/// Audio feature define
/////////////////////////////////////////////////////////
#define UFO_AUDIO_AD_SUPPORT
#define UFO_AUDIO_AC4_SUPPORT
#define UFO_AUDIO_MPEGH_SUPPORT
#define UFO_AUDIO_OPUS_SUPPORT
//#define UFO_AUDIO_CODEC_CAPABILITY_BY_LICENCE
#define UFO_AUDIO_MULTI_CHANNEL
//#define UFO_AUDIO_PCM_MIXER_SUPPORT
//#define UFO_AUDIO_SPDIF_NONPCM_PAUSE
//#define UFO_AUDIO_AVMONITOR_SYNC
//#define UFO_AUDIO_HDMI_RX_HBR_SW_CTRL
#define UFO_AUDIO_DOLBY_DAP_UI_SUPPORT
//#define UFO_AUDIO_FW_TEMPO_SUPPORT
#define UFO_AUDIO_SPDIF_NONPCM_ADDRAWPTR
#define UFO_AUDIO_ATMOS_FLUSH_EVO_METATDATA_BUFFER
#define UFO_AUDIO_EVO_SYNC_CTRL_FOR_ATMOS_ENC
#define UFO_AUDIO_ALWAYS_ENCODE

/////////////////////////////////////////////////////////
/// SYS lib feature define
/////////////////////////////////////////////////////////
#define UFO_SYS_TEE_INFO_SET_MBOOT

/////////////////////////////////////////////////////////
/// XC Support Dynamic Contrl DNR in DS Case
///
///No need this UFO,because DNR and force bob mode now is controlled in QMap DS XRule
/////////////////////////////////////////////////////////
//#define UFO_XC_SWDS_DYMAMIC_CONTROL_DNR

/////////////////////////////////////////////////////////
/// Support MFD lib
/////////////////////////////////////////////////////////
#define UFO_MFD_LIB_INCLUDE

/////////////////////////////////////////////////////////
/// DIP support MFDec
/////////////////////////////////////////////////////////
#define UFO_DIP_SUPPORT_MFDEC

/////////////////////////////////////////////////////////
/// xc support bwd
/////////////////////////////////////////////////////////
#define SUPPORT_BWD

/////////////////////////////////////////////////////////
/// set xc cma information to utopia
/////////////////////////////////////////////////////////
#define UFO_SET_XC_CMA_INFORMATION
#endif // _UFO_H_
