/* SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause */
#ifndef _MDRV_IR_ST_H_
#define _MDRV_IR_ST_H_

//-------------------------------------------------------------------------------------------------
//	Type and Structure
//-------------------------------------------------------------------------------------------------
#define IR_MAX_BUF_DPH	2
#define IR_MAX_BUF_LEN	256

//-------------------------------------------------------------------------------------------------
//	Type and Structure
//-------------------------------------------------------------------------------------------------

enum EN_SHOT_SEL {
	EN_SHOT_P = 0x01,	/// 2'b01: only pshot edge detect for counter
	EN_SHOT_N = 0x02,	/// 2'b10: only nshot edge detect for counter
	EN_SHOT_PN = 0x03,	/// 2'b11/2'b00: both pshot/nshot edge detect for counter
	EN_SHOT_INVLD,		/// Invalid for value greater than 2'b11

};

enum IR_PROCOCOL_TYPE {
	E_IR_PROTOCOL_NONE = 0,
	E_IR_PROTOCOL_NEC,
	E_IR_PROTOCOL_RC5,
	E_IR_PROTOCOL_PZ_OCN,
	E_IR_PROTOCOL_MAX,
};

struct MS_IR_KeyInfo_s {
	unsigned char u8Key;
	unsigned char u8System;
	unsigned char u8Flag;
	unsigned char u8Valid;
};

/// define IR key code time & bounds
struct MS_IR_TimeBnd_s {
	signed short s16Time;	///key code time
	signed char s8UpBnd;	///upper bound
	signed char s8LoBnd;	///low bound
};

/// define IR key code time tail
struct MS_IR_TimeTail_s {
	unsigned int gu32KeyMin;	 /// Min Tail Time for key
	unsigned int gu32KeyMax;	 /// Max Tail Time for key
	unsigned int gu32RptMin;	 /// Min Tail Time for Rpt
	unsigned int gu32RptMax;	 /// Max Tail Time for Rpt
};

/// define IR time parameters
struct MS_IR_TimeCfg_s {
	struct MS_IR_TimeBnd_s tHdr;	/// header code time
	struct MS_IR_TimeBnd_s tOff;	/// off code time
	struct MS_IR_TimeBnd_s tOffRpt;	/// off code repeat time
	struct MS_IR_TimeBnd_s tLg01Hg;	/// logical 0/1 high time
	struct MS_IR_TimeBnd_s tLg0;	/// logical 0 time
	struct MS_IR_TimeBnd_s tLg1;	/// logical 1 time
	struct MS_IR_TimeBnd_s tSepr;	/// Separate time
	unsigned int u32TimeoutCyc;	/// Timeout cycle count
	unsigned short u16RCBitTime;	/// RC Bit Time
	struct MS_IR_TimeTail_s tTail;	/// Tail Time for sw shot mode
};

/// define IR configuration parameters
struct MS_IR_InitCfg_s {
	unsigned char u8DecMode;	/// IR mode selection
	unsigned char u8ExtFormat;	/// IR extension format
	unsigned char u8Ctrl0;		/// IR enable control 0
	unsigned char u8Ctrl1;		/// IR enable control 1
	unsigned char u8Clk_mhz;	/// IR required clock
	unsigned char u8HdrCode0;	/// IR Header code 0
	unsigned char u8HdrCode1;	/// IR Header code 1
	unsigned char u8CCodeBytes;	/// Customer codes: 1 or 2 bytes
	unsigned char u8CodeBits;	/// Code bits: 1~128 bits
	unsigned char u8KeySelect;	/// IR select Nth key N(1~16)
	unsigned short u16GlhrmNum;	/// Glitch Remove Number
	enum EN_SHOT_SEL enShotSel;	/// Shot selection for SW decoder
	unsigned int bInvertPolar;	/// Invert the polarity for input IR signal

};

/// define Ping-Pong Buffer structure for IR SW shot count
struct MS_IR_ShotInfo_s {
	unsigned char u8RdIdx;		//Read Index
	unsigned char u8WtIdx;		//Write Index
	unsigned int u32Length;		//Data Length for Read Index buffer
	unsigned int u32Buffer[IR_MAX_BUF_DPH][IR_MAX_BUF_LEN];	 //Ping-Pong Buffer
};

/// define HeaderInfo for sw mode change headercode in apps
struct MS_MultiIR_HeaderInfo_s {
	unsigned char _u8IRHeaderCode0;		//IRHeaderCode0
	unsigned char _u8IRHeaderCode1;		//IRHeaderCode1
	unsigned char _u8IR2HeaderCode0;	//IR2HeaderCode0
	unsigned char _u8IR2HeaderCode1;	//IR2HeaderCode1
};

#endif // _MDRV_IR_ST_H_
