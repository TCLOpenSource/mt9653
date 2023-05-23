/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _IOPORT_H_
#define _IOPORT_H_

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------

///This Enum is interface for Hal and Application level.
typedef enum {
	INPUT_PORT_NONE_PORT = 0,

	INPUT_PORT_ANALOG0 = 1,	// Data port
	INPUT_PORT_ANALOG1,
	INPUT_PORT_ANALOG2,
	INPUT_PORT_ANALOG3,
	INPUT_PORT_ANALOG4,

	// Reserved area

	INPUT_PORT_ANALOG0_SYNC = 10,	// Sync port
	INPUT_PORT_ANALOG1_SYNC,
	INPUT_PORT_ANALOG2_SYNC,
	INPUT_PORT_ANALOG3_SYNC,
	INPUT_PORT_ANALOG4_SYNC,

	// Reserved area

	INPUT_PORT_YMUX_CVBS0 = 30,
	INPUT_PORT_YMUX_CVBS1,
	INPUT_PORT_YMUX_CVBS2,
	INPUT_PORT_YMUX_CVBS3,
	INPUT_PORT_YMUX_CVBS4,
	INPUT_PORT_YMUX_CVBS5,
	INPUT_PORT_YMUX_CVBS6,
	INPUT_PORT_YMUX_CVBS7,
	INPUT_PORT_YMUX_G0,
	INPUT_PORT_YMUX_G1,
	INPUT_PORT_YMUX_G2,
	INPUT_PORT_YMUX_R0,
	INPUT_PORT_YMUX_R1,
	INPUT_PORT_YMUX_R2,
	INPUT_PORT_YMUX_B0,
	INPUT_PORT_YMUX_B1,
	INPUT_PORT_YMUX_B2,

	// Reserved area

	INPUT_PORT_CMUX_CVBS0 = 50,
	INPUT_PORT_CMUX_CVBS1,
	INPUT_PORT_CMUX_CVBS2,
	INPUT_PORT_CMUX_CVBS3,
	INPUT_PORT_CMUX_CVBS4,
	INPUT_PORT_CMUX_CVBS5,
	INPUT_PORT_CMUX_CVBS6,
	INPUT_PORT_CMUX_CVBS7,
	INPUT_PORT_CMUX_R0,
	INPUT_PORT_CMUX_R1,
	INPUT_PORT_CMUX_R2,
	INPUT_PORT_CMUX_G0,
	INPUT_PORT_CMUX_G1,
	INPUT_PORT_CMUX_G2,
	INPUT_PORT_CMUX_B0,
	INPUT_PORT_CMUX_B1,
	INPUT_PORT_CMUX_B2,
	// Reserved area

	INPUT_PORT_DVI0 = 80,
	INPUT_PORT_DVI1,
	INPUT_PORT_DVI2,
	INPUT_PORT_DVI3,

	// Reserved area

	INPUT_PORT_MVOP = 100,
	INPUT_PORT_MVOP2,
	INPUT_PORT_MVOP3,

	INPUT_PORT_SCALER_OP = 110,
	INPUT_PORT_SCALER_DI,

	INPUT_PORT_YMUX_RY0 = 120,
	INPUT_PORT_YMUX_RY1,
	INPUT_PORT_YMUX_RY2,
} E_MUX_INPUTPORT;

///This Enum is interface for Hal and Application level.
typedef enum {
	E_INPUT_NOT_SUPPORT_MHL = 0x0,
	E_INPUT_SUPPORT_MHL_PORT_DVI0 = 0x1,
	E_INPUT_SUPPORT_MHL_PORT_DVI1 = 0x2,
	E_INPUT_SUPPORT_MHL_PORT_DVI2 = 0x4,
	E_INPUT_SUPPORT_MHL_PORT_DVI3 = 0x8,
	E_INPUT_ALL_SUPPORT_MHL = 0xF,

} EN_MUX_INPUTPORT_MHL_INFO;

///This Enum is interface for Hal and Application level.
typedef enum {
	OUTPUT_PORT_NONE_PORT = 0,

	OUTPUT_PORT_SCALER_MAIN_WINDOW = 1,
	OUTPUT_PORT_SCALER2_MAIN_WINDOW = 2,

	// Reserved area

	OUTPUT_PORT_SCALER_SUB_WINDOW1 = 10,
	OUTPUT_PORT_SCALER2_SUB_WINDOW = 11,

	// Reserved area

	OUTPUT_PORT_CVBS1 = 20,
	OUTPUT_PORT_CVBS2,

	// Reserved area

	OUTPUT_PORT_YPBPR1 = 40,
	OUTPUT_PORT_YPBPR2,

	// Reserved area

	OUTPUT_PORT_HDMI1 = 60,
	OUTPUT_PORT_HDMI2,

	// Reserved area

	OUTPUT_PORT_OFFLINE_DETECT = 80,

	// Reserved area
	OUTPUT_PORT_DWIN = 100,

} E_MUX_OUTPUTPORT;

#define IsAnalogPort(x)         ((x) >= INPUT_PORT_ANALOG0 && (x) <= INPUT_PORT_ANALOG4)
#define IsDVIPort(x)            ((x) >= INPUT_PORT_DVI0 && (x) <= INPUT_PORT_DVI3)
#define IsYCVBSPort(x)          ((x) >= INPUT_PORT_YMUX_CVBS0 && (x) <= INPUT_PORT_YMUX_CVBS7)
#define IsCCVBSPort(x)          ((x) >= INPUT_PORT_CMUX_CVBS0 && (x) <= INPUT_PORT_CMUX_CVBS7)
#define IsCVBSPort(x)           (IsYCVBSPort(x) || IsCCVBSPort(x))

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

#endif				// _IOPORT_H_
