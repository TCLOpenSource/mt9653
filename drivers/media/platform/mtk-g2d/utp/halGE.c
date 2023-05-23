// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
//#include "MsCommon.h"
#ifndef MSOS_TYPE_LINUX_KERNEL
#include <string.h>
#include <linux/sched.h>
#endif
#include <../mtk-g2d-utp-wrapper.h>
#include "regGE.h"
#include "apiGFX.h"
#include "drvGE.h"
#include "halGE.h"
#include "sti_msos.h"
#include "riu.h"
#include "GE0_2410.h"
#include "GE2_2412.h"
//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define GE_CMDQ_ENABLE              1UL	// Always Enable
#define GE_CMD_SIZE_MAX             GE_STAT_CMDQ_MAX
#define GE_VCMD_SIZE_MAX            GE_STAT_VCMDQ_MAX
#define GE_CMD_SIZE                 1UL	// 1 queue entry available for 2 commands
#define CODA_SHIFT                  4UL

#define GE_TAG_INTERRUPT_WAITING_TIME 10	// ms
#define GE_TAG_INTERRUPT_DEBUG_PRINT_THRESHOLD (500/GE_TAG_INTERRUPT_WAITING_TIME)

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------
#if (__GE_WAIT_TAG_MODE == __USE_GE_INT_MODE)
typedef enum {
	E_GE_CLEAR_INT = 0x0001,
	E_GE_MASK_INT = 0x0002,
	E_GE_UNMASK_INT = 0x0004,
	E_GE_INT_TAG_MODE = 0x0008,
	E_GE_INT_NORMAL_MODE = 0x0010
} E_GE_INT_OP;
#endif

//------------------------------------------------------------------------------------------------
//  Global Variables
//------------------------------------------------------------------------------------------------


#define REG_GE_INVALID 0xFF

const MS_U8 _GE_Reg_Backup[] = {
	REG_GE_VCMDQ_BASE_L, REG_GE_VCMDQ_BASE_H, REG_GE_BASE_MSB, REG_GE_YUV_MODE,
	    REG_GE_VCMDQ_SIZE,
	REG_GE_EN, REG_GE_CFG, REG_GE_TH, REG_GE_ROP2_, REG_GE_BLEND, REG_GE_ALPHA,
	    REG_GE_ALPHA_CONST,
	REG_GE_SCK_HTH_L, REG_GE_SCK_HTH_H, REG_GE_SCK_LTH_L, REG_GE_SCK_LTH_H, REG_GE_DCK_HTH_L,
	REG_GE_DCK_HTH_H, REG_GE_DCK_LTH_L, REG_GE_DCK_LTH_H, REG_GE_OP_MODE, REG_GE_ATEST_TH,
	REG_GE_SRC_BASE_L, REG_GE_SRC_BASE_H, REG_GE_DST_BASE_L, REG_GE_DST_BASE_H,
	REG_GE_SRC_PITCH, REG_GE_DST_PITCH, REG_GE_FMT,
	0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e,	// I0~I4
	0x003f, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048,	// I5-I9
	0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051, 0x0052,	// I10-I14
	0x0053, 0x0054,		// I15
	REG_GE_CLIP_L, REG_GE_CLIP_R, REG_GE_CLIP_T, REG_GE_CLIP_B, REG_GE_ROT_MODE,
	    REG_GE_BLT_SCK_MODE,
	REG_GE_BLT_SCK_CONST_L, REG_GE_BLT_SCK_CONST_H, REG_GE_BLT_DST_X_OFST,
	    REG_GE_BLT_DST_Y_OFST,
	REG_GE_LINE_DELTA_, REG_GE_LINE_STYLE, REG_GE_LINE_LEN, REG_GE_BLT_SRC_DX,
	    REG_GE_BLT_SRC_DY,
	REG_GE_ITALIC_OFFSET, REG_GE_ITALIC_DELTA, REG_GE_PRIM_V0_X, REG_GE_PRIM_V0_Y,
	    REG_GE_PRIM_V1_X,
	REG_GE_PRIM_V1_Y, REG_GE_PRIM_V2_X, REG_GE_PRIM_V2_Y, REG_GE_BLT_SRC_W, REG_GE_BLT_SRC_H,
	REG_GE_PRIM_C_L, REG_GE_PRIM_C_H, REG_GE_PRIM_RDX_L, REG_GE_PRIM_RDX_H, REG_GE_PRIM_RDY_L,
	REG_GE_PRIM_RDY_H, REG_GE_PRIM_GDX_L, REG_GE_PRIM_GDX_H, REG_GE_PRIM_GDY_L,
	    REG_GE_PRIM_GDY_H,
	REG_GE_PRIM_BDX_L, REG_GE_PRIM_BDX_H, REG_GE_PRIM_BDY_L, REG_GE_PRIM_BDY_H, REG_GE_PRIM_ADX,
	REG_GE_PRIM_ADY, REG_GE_INVALID
};

#define IP_VERSION pGEHalLocal->u32IpVersion
//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  Local Var
//------------------------------------------------------------------------------
GE_CHIP_PROPERTY g_GeChipPro = {
	.WordUnit = GE_WordUnit,

	.bSupportFourePixelMode = TRUE,
	.bFourPixelModeStable = TRUE,

	.bSupportMultiPixel = FALSE,
	.bSupportSpiltMode = TRUE,
	.bSupportTwoSourceBitbltMode = FALSE,
	.MIUSupportMaxNUM = GE_MAX_MIU,
	.BltDownScaleCaps = {
			     .u8RangeMax = 1,
			     .u8RangeMin = 32,
			     .u8ContinuousRangeMin = 1,
			     .bFullRangeSupport = TRUE,

			     .u8ShiftRangeMax = 0,	/// 1   = 2^0   = 1<<0
			     .u8ShiftRangeMin = 5,	/// 32  = 2^5   = 1<<5
			     .u8ShiftContinuousRangeMin = 0,	/// 1   = 2^0   = 1<<0
	}
};


#if (__GE_WAIT_TAG_MODE == __USE_GE_INT_MODE)
static MS_VIRT virtHalIomapBaseAddr;
static MS_BOOL bGeIrqInited = FALSE;
static MS_S32 s32GeWaitTagEventHandle = -1;
static MS_S32 s32WaitingTagPid = -1;

//void _GE_WaitTag_InterruptCbk(MS_U32 eIntNum);
static GE_Result _GE_Ctrl_IntMode(GE_CTX_HAL_LOCAL *pGEHalLocal, E_GE_INT_OP int_op);
static void _GE_Print_GeWaitTagTimeout_Msg(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U16 tagID);
#endif


//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------
void GE_Chip_Proprity_Init(GE_CTX_HAL_LOCAL *pGEHalLocal)
{
	pGEHalLocal->pGeChipPro = &g_GeChipPro;
}

GE_Result _GE_SetBltScaleRatio2HW(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_ScaleInfo *pScaleinfo)
{
	MS_U16 u16RegVal;

	//check parameters
	if (pScaleinfo == NULL)
		return E_GE_FAIL_PARAM;

	GE_CODA_WriteReg(pGEHalLocal, REG_0190_GE0, (MS_U16) (pScaleinfo->x & 0xFFFF));
	GE_CODA_WriteReg(pGEHalLocal, REG_0194_GE0, (MS_U16) (pScaleinfo->y & 0xFFFF));
	//Set Initial DeltaX, DeltaY:
	GE_CODA_WriteReg(pGEHalLocal, REG_0178_GE0, (MS_U16) (pScaleinfo->init_x & 0xFFFF));
	GE_CODA_WriteReg(pGEHalLocal, REG_017C_GE0, (MS_U16) (pScaleinfo->init_y & 0xFFFF));

	//set MSBs of REG_GE_BLT_SRC_DY, REG_GE_BLT_SRC_DY:
	u16RegVal = GE_CODA_ReadReg(pGEHalLocal, REG_0178_GE0) & ~(GE_STBB_DX_MSB);
	u16RegVal |= (((pScaleinfo->x >> 16) << GE_STBB_DX_MSB_SHFT) & GE_STBB_DX_MSB);
	GE_CODA_WriteReg(pGEHalLocal, REG_0178_GE0, u16RegVal);

	u16RegVal = GE_CODA_ReadReg(pGEHalLocal, REG_017C_GE0) & ~(GE_STBB_DY_MSB);
	u16RegVal |= (((pScaleinfo->y >> 16) << GE_STBB_DY_MSB_SHFT) & GE_STBB_DY_MSB);
	GE_CODA_WriteReg(pGEHalLocal, REG_017C_GE0, u16RegVal);

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
static void GE_DumpReg(GE_CTX_HAL_LOCAL *pGEHalLocal)
{
	MS_U32 i;

	G2D_ERROR("Dump GE register:\n");
	for (i = 0; i < 0x80; i++) {
		if (i % 0x08 == 0) {
			G2D_ERROR("\n");
			G2D_ERROR("h%02x    ", (MS_U8) i);
		}
		G2D_ERROR("%04x ",  mtk_read2byte((i*CODA_SHIFT)+GE0_2410_BASE));
	}

	G2D_ERROR("\n");
}

MS_U8 _GFXAPI_MIU_ID(MS_PHY ge_fbaddr)
{
	MS_U8 u8MIUSelTMP = 0;
	MS_PHY PhyOffset = 0;

	_phy_to_miu_offset(u8MIUSelTMP, PhyOffset, ge_fbaddr);
	UNUSED(PhyOffset);

	return u8MIUSelTMP;
}

MS_PHY _GFXAPI_PHYS_ADDR_IN_MIU(MS_PHY ge_fbaddr)
{
	MS_U8 u8MIUSelTMP = 0;
	MS_PHY PhyOffset = 0;

	_phy_to_miu_offset(u8MIUSelTMP, PhyOffset, ge_fbaddr);
	UNUSED(u8MIUSelTMP);

	return PhyOffset;
}

#if (__GE_WAIT_TAG_MODE == __USE_GE_INT_MODE)
void _GE_WaitTag_InterruptCbk(MS_U32 eIntNum)
{
#if defined(MSOS_TYPE_LINUX)
	MS_S32 s32CurPid = (MS_S32) getpid();
#endif

#if defined(MSOS_TYPE_LINUX)
	if (s32WaitingTagPid == s32CurPid)
#endif
	{
		(*
		 ((volatile MS_U16 *)(virtHalIomapBaseAddr + GE_BANK_NUM * 2 +
				      ((REG_GE_SRCMASK_GB) << 2)))) = 0xE0;

		if (s32GeWaitTagEventHandle > 0) {
			if (MsOS_SetEvent(s32GeWaitTagEventHandle, 0x1) == FALSE) {
				GE_DBG("[%s, %d]:  MsOS_ReleaseSemaphore failed\r\n", __func__,
				       __LINE__);
			}
		}
	}

	MsOS_EnableInterrupt(E_INT_IRQ_GE);
#ifdef MSOS_TYPE_LINUX
	MsOS_CompleteInterrupt(E_INT_IRQ_GE);
#endif
}

static GE_Result _GE_Ctrl_IntMode(GE_CTX_HAL_LOCAL *pGEHalLocal, E_GE_INT_OP int_op)
{
	MS_U16 u16IntReg = 0;

	u16IntReg = GE_CODA_ReadReg(pGEHalLocal, REG_0078_GE0);

	if (E_GE_CLEAR_INT & int_op) {
		u16IntReg |= GE_INT_MODE_CLEAR;
		GE_CODA_WriteReg(pGEHalLocal, REG_0078_GE0, u16IntReg);
		u16IntReg &= (~GE_INT_MODE_CLEAR);
	}

	if (E_GE_MASK_INT & int_op)
		u16IntReg |= GE_INT_TAG_MASK;

	if (E_GE_UNMASK_INT & int_op)
		u16IntReg &= (~GE_INT_TAG_MASK);

	if (E_GE_INT_TAG_MODE & int_op)
		u16IntReg |= GE_INT_TAG_MODE;

	if (E_GE_INT_NORMAL_MODE & int_op)
		u16IntReg &= (~GE_INT_TAG_MODE);

	GE_CODA_WriteReg(pGEHalLocal, REG_0078_GE0, u16IntReg);

	return E_GE_OK;
}

static MS_BOOL _GE_IsTagInterruptEnabled(GE_CTX_HAL_LOCAL *pGEHalLocal)
{
	MS_BOOL bret = FALSE;
	MS_U16 u16IntReg = 0;

	u16IntReg = GE_CODA_ReadReg(pGEHalLocal, REG_0078_GE0);

	bret = FALSE;
	if ((GE_INT_TAG_MODE & u16IntReg) > 0)
		bret = TRUE;

	return bret;
}

static void _GE_Print_GeWaitTagTimeout_Msg(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U16 tagID)
{
	MS_U16 tmp_reg = 0;

	GE_DBG("[%s, %d]: >>>>>>>>>>>>>>>>>> ge wait event timeout <<<<<<<<<<<<<<<<<<<\r\n",
	       __func__, __LINE__);
	GE_DBG("[%s, %d]: current proc id = %td \r\n", __func__, __LINE__,
	       (ptrdiff_t) s32WaitingTagPid);

	tmp_reg = GE_CODA_ReadReg(pGEHalLocal, REG_0078_GE0);
	GE_DBG("[%s, %d]: ge int status = 0x%x \r\n", __func__, __LINE__, tmp_reg);

	tmp_reg = INTR_CTNL_BK(0x56);
	GE_DBG("[%s, %d]: cpu int mask = 0x%x \r\n", __func__, __LINE__, tmp_reg);
	tmp_reg = INTR_CTNL_BK(0x5E);
	GE_DBG("[%s, %d]: cpu int status = 0x%x \r\n", __func__, __LINE__, tmp_reg);

	tmp_reg = GE_CODA_ReadReg(pGEHalLocal, REG_0094_GE0);
	GE_DBG("[%s, %d]: int_tag = 0x%x \r\n", __func__, __LINE__, tmp_reg);

	tmp_reg = GE_CODA_ReadReg(pGEHalLocal, REG_00C8_GE0);
	GE_DBG("[%s, %d]: tag = 0x%x \r\n", __func__, __LINE__, tmp_reg);

	GE_DBG("[%s, %d]: tagID = 0x%x \r\n", __func__, __LINE__, tagID);
}

GE_Result HAL_GE_exit(GE_CTX_HAL_LOCAL *pGEHalLocal)
{

	if (MsOS_DetachInterrupt(E_INT_IRQ_GE) == TRUE)
		bGeIrqInited = FALSE;

	if (s32GeWaitTagEventHandle > 0) {
		if (MsOS_DeleteEventGroup(s32GeWaitTagEventHandle == TRUE))
			s32GeWaitTagEventHandle = -1;
	}


	return E_GE_OK;
}
#endif

static void GE_Reset(GE_CTX_HAL_LOCAL *pGEHalLocal)
{
	MS_U16 reg0, reg1;

	reg0 = mtk_read2byte(REG_0000_GE0);
	reg1 = mtk_read2byte(REG_0004_GE0);

	mtk_write2byte(REG_0000_GE0, 0);
	mtk_write2byte(REG_0004_GE0, 0);

	mtk_write2byte(REG_0000_GE0, reg0);
	mtk_write2byte(REG_0004_GE0, reg1);
}

static MS_U8 GE_MapVQ2Reg(GFX_VcmqBufSize enBufSize)
{
	switch (enBufSize) {
	case GFX_VCMD_16K:
		return GE_VQ_16K;
	case GFX_VCMD_32K:
		return GE_VQ_32K;
	case GFX_VCMD_64K:
		return GE_VQ_64K;
	case GFX_VCMD_128K:
		return GE_VQ_128K;
	case GFX_VCMD_256K:
		return GE_VQ_256K;
	case GFX_VCMD_512K:
		return GE_VQ_512K;
	case GFX_VCMD_1024K:
		return GE_VQ_1024K;
	default:
		return 0;
	}
}

void GE_CODA_WaitCmdQAvail(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U32 u32Count)
{
	MS_U32 waitcount = 0;
	MS_U16 tmp1 = 0;
	MS_U32 u32CmdMax;
	MS_U32 u32VQFreeCnt;

	/// VCMQ enabled
	if (mtk_read2bytemask(REG_0004_GE0, REG_EN_GE_VCMQ) != 0) {
		// 16 Bytes one command in VCMDQ.
		u32CmdMax = (512 << mtk_read2bytemask(REG_00A8_GE0, REG_GE_VCMQ_SIZE));
		u32Count = MIN(u32CmdMax, u32Count);

		while (mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ2_STATUS) < u32Count) {

			if (waitcount >= 0x80000) {
				G2D_ERROR("V0 Wait command queue: %d : %tx, %tx\n", tmp1,
				  (ptrdiff_t)mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ2_STATUS),
				  (ptrdiff_t) u32Count);
				waitcount = 0;
				tmp1++;
				if (tmp1 > 10) {
					GE_DumpReg(pGEHalLocal);
					GE_Reset(pGEHalLocal);
				}
			}
			waitcount++;

		}
		tmp1 = 0;
		waitcount = 0;

		//If u32Count >= u32CmdMax, It will be dead loop. if match
		//Full VCMDQ, hw will hang, so keep the logic.
		u32VQFreeCnt = mtk_read2byte(REG_0010_GE0) +
			(mtk_read2bytemask(REG_0014_GE0, REG_GE_VCMQ_STATUS_1) << 16);
		while (u32VQFreeCnt >= (MS_U32) (u32CmdMax - u32Count)) {

			if (waitcount >= 0x80000) {
				G2D_ERROR("Wait VCMQ : %d : %tx, %tx\n", tmp1,
					  (ptrdiff_t) u32VQFreeCnt, (ptrdiff_t) u32Count);
				waitcount = 0;
				tmp1++;
				if (tmp1 > 10) {
					GE_DumpReg(pGEHalLocal);
					GE_Reset(pGEHalLocal);
				}
			}
			waitcount++;
			u32VQFreeCnt = mtk_read2byte(REG_0010_GE0) +
				(mtk_read2bytemask(REG_0014_GE0, REG_GE_VCMQ_STATUS_1) << 16);
		}
	} else {
		u32Count = MIN(GE_CMD_SIZE_MAX, u32Count);

		while (mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ2_STATUS) < u32Count) {
			if (waitcount >= 0x80000) {
				G2D_ERROR("Wait command queue: %d : %tx, %tx\n", tmp1,
				  (ptrdiff_t) mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ2_STATUS),
				  (ptrdiff_t) u32Count);
				waitcount = 0;
				tmp1++;
				if (tmp1 > 10) {
					GE_DumpReg(pGEHalLocal);
					GE_Reset(pGEHalLocal);
				}
			}
			waitcount++;
		}

	}
}


MS_PHY GE_ConvertAPIAddr2HAL(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U8 u8MIUId,
			     MS_PHY PhyGE_APIAddrInMIU)
{
	return PhyGE_APIAddrInMIU;
}

void GE_WaitIdle(GE_CTX_HAL_LOCAL *pGEHalLocal)
{
	MS_U32 waitcount = 0;
	MS_U16 tmp1 = 0;
	MS_U8 i = 0;

	for (i = 0; i < (GE_WordUnit >> 2); i++) {
		GE_CODA_WriteReg(pGEHalLocal, REG_00C4_GE0, 0);
		GE_CODA_WriteReg(pGEHalLocal, REG_00C8_GE0,
			GE_GetNextTAGID(pGEHalLocal, FALSE)); //dummy
	}

	// GE will pack 2 register commands before CMDQ
	// We need to push fifo if there is one command in the fifo before
	// CMDQ. Then the GE status register will be consistent after idle.

	GE_CODA_WaitCmdQAvail(pGEHalLocal, GE_STAT_CMDQ_MAX);// Wait CMDQ empty

	// Wait level-2 command queue flush
	while (mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ1_STATUS) !=
		GE_STAT_CMDQ2_MAX) {

		if (waitcount >= 0x80000) {
			G2D_ERROR("Wait Idle: %u : %tx\n", tmp1,
			  (ptrdiff_t) mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ2_STATUS));
			waitcount = 0;
			tmp1++;
			if (tmp1 > 10) {
				GE_DumpReg(pGEHalLocal);
				GE_Reset(pGEHalLocal);
			}
		}
		waitcount++;
	}

	waitcount = 0;
	tmp1 = 0;

	// Wait GE idle
	while (mtk_read2bytemask(REG_001C_GE0, REG_GE_BUSY)) {

		if (waitcount >= 0x80000) {
			G2D_ERROR("Wait Busy: %u : %tx\n", tmp1,
			  (ptrdiff_t) mtk_read2bytemask(REG_001C_GE0, REG_GE_CMQ2_STATUS));
			waitcount = 0;
			tmp1++;
			if (tmp1 > 10) {
				GE_DumpReg(pGEHalLocal);
				GE_Reset(pGEHalLocal);
			}
		}
		waitcount++;
	}

}

MS_U16 GE_CODA_ReadReg(GE_CTX_HAL_LOCAL *pGEHalLocal, uint32_t addr)
{
	MS_U16 u16NoFIFOMask;

	switch (addr) {	//for registers which do not go through command queue
	case REG_0000_GE0://REG_GE_EN
		u16NoFIFOMask = GE_EN_GE;
		break;
	case REG_0008_GE0: //REG_GE_DEBUG
	case REG_000C_GE0: //REG_GE_TH
	case REG_0014_GE0: //REG_GE_BIST_STAT
	case REG_001C_GE0: //REG_GE_STAT
	case REG_0010_GE0: //REG_GE_VCMDQ_STAT
	case REG_00C8_GE0: //REG_GE_TAG
	case REG_00C4_GE0: //REG_GE_TAG_H
	case REG_00A0_GE0: //REG_GE_VCMDQ_BASE_L
	case REG_00A4_GE0: //REG_GE_VCMDQ_BASE_H
	case REG_0020_GE0: //REG_GE_MIU_PROT_LTH_L(0):
	case REG_0024_GE0: //REG_GE_MIU_PROT_LTH_H(0):
	case REG_0028_GE0: //REG_GE_MIU_PROT_HTH_L(0):
	case REG_002C_GE0: //REG_GE_MIU_PROT_HTH_H(0):
	case REG_0030_GE0: //REG_GE_MIU_PROT_LTH_L(1):
	case REG_0034_GE0: //REG_GE_MIU_PROT_LTH_H(1):
	case REG_0038_GE0: //REG_GE_MIU_PROT_HTH_L(1):
	case REG_003C_GE0: //REG_GE_MIU_PROT_HTH_H(1):
		u16NoFIFOMask = 0xffff;
		break;
	case REG_00A8_GE0: //REG_GE_VCMDQ_SIZE:
		u16NoFIFOMask = GE_VCMDQ_SIZE_MASK;
		break;
	case REG_008C_GE0: //REG_GE_BASE_MSB:
		u16NoFIFOMask = GE_VCMDQ_MSB_MASK;
		break;
	default:
		u16NoFIFOMask = 0;
		break;
	}

	if (u16NoFIFOMask == 0)
		return pGEHalLocal->u16RegGETable[(addr-GE0_2410_BASE)/4];

	return (mtk_read2byte(addr) & u16NoFIFOMask) |
		(pGEHalLocal->u16RegGETable[(addr-GE0_2410_BASE)/4] & ~u16NoFIFOMask);
}

void GE_CODA_WriteReg(GE_CTX_HAL_LOCAL *pGEHalLocal, uint32_t addr, uint32_t value)
{
	// CMDQ special command
	if ((addr-GE0_2410_BASE)/4 < GE_TABLE_REGNUM) {
		pGEHalLocal->u16RegGETable[(addr-GE0_2410_BASE)/4] = value;
	} else {
		G2D_INFO("Reg Index [%d]is out of GE_TABLE_REGNUM [0x%lx]range!!!!\n",
			  addr, GE_TABLE_REGNUM);
	}

	GE_CODA_WaitCmdQAvail(pGEHalLocal, GE_CMD_SIZE);

	mtk_write2byte(addr, value);
}

void GE_ResetState(GE_CTX_HAL_LOCAL *pGEHalLocal)
{
	GE_WaitIdle(pGEHalLocal);

	GE_CODA_WriteReg(pGEHalLocal, REG_0000_GE0, GE_EN_GE);

	GE_CODA_WriteReg(pGEHalLocal, REG_000C_GE0, 0x0000);

	GE_CODA_WriteReg(pGEHalLocal, REG_0188_GE0, GE_LINEPAT_RST);
	GE_CODA_WriteReg(pGEHalLocal, REG_0164_GE0, GE_BLT_SCK_NEAREST);
	GE_CODA_WriteReg(pGEHalLocal, REG_0044_GE0, GE_ALPHA_ARGB1555);
}


void GE_Init_RegImage(GE_CTX_HAL_LOCAL *pGEHalLocal)
{
	MS_U8 addr;

	for (addr = 0; addr < GE_TABLE_REGNUM; addr++) {
		pGEHalLocal->u16RegGETable[addr] = mtk_read2byte((addr*4)+GE0_2410_BASE);
	}

}

void GE_Init(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_Config *cfg)
{
	MS_U16 u16temp = 0;

	GE_WaitIdle(pGEHalLocal);

	u16temp = GE_CODA_ReadReg(pGEHalLocal, REG_0004_GE0);

	if ((u16temp & BIT(1)) != BIT(1)) { //if VQ is Not Enabled
		// Set default FMT for avoiding 1st set buffinfo error.
		GE_CODA_WriteReg(pGEHalLocal, REG_00D0_GE0,
			    (GE_FMT_ARGB1555 << GE_SRC_FMT_SHFT) +
			    (GE_FMT_ARGB1555 << GE_DST_FMT_SHFT));

		GE_ResetState(pGEHalLocal);
	}

#if defined STI_PLATFORM_BRING_UP
	GE_Init_RegImage(pGEHalLocal);
#endif
	GE_CODA_WriteReg(pGEHalLocal, REG_000C_GE0, GE_THRESHOLD_SETTING);
	//Mask Interrupt
	GE_CODA_WriteReg(pGEHalLocal, REG_0078_GE0, 0x00C0);

	//CRC
	mtk_write2bytemask(REG_0000_GE2, 1, REG_EN_GE_CHK_CRC);
	mtk_write2bytemask(REG_0004_GE2, 1, REG_EN_GE_CHK_CLR_CRC);

#if (__GE_WAIT_TAG_MODE == __USE_GE_INT_MODE)

	if (bGeIrqInited == FALSE) {
		if (_GE_IsTagInterruptEnabled(pGEHalLocal) == FALSE)
			_GE_Ctrl_IntMode(pGEHalLocal, E_GE_INT_TAG_MODE | E_GE_MASK_INT);

		if (MsOS_AttachInterrupt(E_INT_IRQ_GE, _GE_WaitTag_InterruptCbk) == FALSE) {
			GE_DBG("[%s, %d]: MsOS_AttachInterrupt failed \r\n", __func__,
			       __LINE__);
			return;
		}
		MsOS_EnableInterrupt(E_INT_IRQ_GE);

		bGeIrqInited = TRUE;
	}

	if (s32GeWaitTagEventHandle < 0) {
		s32GeWaitTagEventHandle = MsOS_CreateEventGroup("GE_Wait_Event_Handle");
		if (s32GeWaitTagEventHandle < 0) {
			GE_DBG("[%s, %d]: ge_semid_waitTag = %td", __func__, __LINE__,
			       (ptrdiff_t) s32GeWaitTagEventHandle);
			return;
		}
	}
#endif

}

GE_Result GE_SetRotate(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_RotateAngle geRotAngle)
{
	MS_U16 u16RegVal;

	u16RegVal =
	(GE_CODA_ReadReg(pGEHalLocal, REG_0164_GE0) & ~REG_GE_ROT_MODE_MASK) |
		(geRotAngle << REG_GE_ROT_MODE_SHFT);
	GE_CODA_WriteReg(pGEHalLocal, REG_0164_GE0, u16RegVal);

	return E_GE_OK;
}

GE_Result GE_SetOnePixelMode(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL enable)
{
	MS_U16 u16en;

	u16en = GE_CODA_ReadReg(pGEHalLocal, REG_0000_GE0);

	if (enable)
		u16en |= GE_EN_ONE_PIXEL_MODE;
	else
		u16en &= (~GE_EN_ONE_PIXEL_MODE);

	u16en |= GE_EN_BURST;

	GE_CODA_WriteReg(pGEHalLocal, REG_0000_GE0, u16en);

	return E_GE_OK;
}

GE_Result GE_SetBlend(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_BlendOp eBlendOp)
{
	MS_U16 u16op;

	switch (eBlendOp) {
	case E_GE_BLEND_ONE:
	case E_GE_BLEND_CONST:
	case E_GE_BLEND_ASRC:
	case E_GE_BLEND_ADST:
	case E_GE_BLEND_ROP8_ALPHA:
	case E_GE_BLEND_ROP8_SRCOVER:
	case E_GE_BLEND_ROP8_DSTOVER:
	case E_GE_BLEND_ZERO:
	case E_GE_BLEND_CONST_INV:
	case E_GE_BLEND_ASRC_INV:
	case E_GE_BLEND_ADST_INV:
	case E_GE_BLEND_ALPHA_ADST:
	case E_GE_BLEND_SRC_ATOP_DST:
	case E_GE_BLEND_DST_ATOP_SRC:
	case E_GE_BLEND_SRC_XOR_DST:
	case E_GE_BLEND_INV_CONST:
	case E_GE_BLEND_FADEIN:
	case E_GE_BLEND_FADEOUT:

		u16op = eBlendOp;
		break;
	default:
		return E_GE_FAIL_PARAM;
	}

	u16op = (GE_CODA_ReadReg(pGEHalLocal, REG_0044_GE0) & ~GE_BLEND_MASK) | u16op;
	GE_CODA_WriteReg(pGEHalLocal, REG_0044_GE0, u16op);

	return E_GE_OK;
}


GE_Result GE_SetAlpha(GE_CTX_HAL_LOCAL *pGEHalLocal, GFX_AlphaSrcFrom eAlphaSrc)
{
	MS_U16 u16src;

	switch (eAlphaSrc) {
	case ALPHA_FROM_CONST:
	case ALPHA_FROM_ASRC:
	case ALPHA_FROM_ADST:
	case ALPHA_FROM_ROP8_SRC:
	case ALPHA_FROM_ROP8_IN:
	case ALPHA_FROM_ROP8_DSTOUT:
	case ALPHA_FROM_ROP8_SRCOUT:
	case ALPHA_FROM_ROP8_OVER:
	case ALPHA_FROM_ROP8_INV_CONST:
	case ALPHA_FROM_ROP8_INV_ASRC:
	case ALPHA_FROM_ROP8_INV_ADST:
	case ALPHA_FROM_ROP8_SRC_ATOP_DST:
	case ALPHA_FROM_ROP8_DST_ATOP_SRC:
	case ALPHA_FROM_ROP8_SRC_XOR_DST:
	case ALPHA_FROM_ROP8_INV_SRC_ATOP_DST:
	case ALPHA_FROM_ROP8_INV_DST_ATOP_SRC:

		u16src = eAlphaSrc;
		break;
	default:
		return E_GE_FAIL_PARAM;
	}

	u16src =
		(GE_CODA_ReadReg(pGEHalLocal, REG_0048_GE0) & ~GE_ALPHA_MASK)
		| (u16src << GE_ALPHA_SHFT);
	GE_CODA_WriteReg(pGEHalLocal, REG_0048_GE0, u16src);

	return E_GE_OK;
}

GE_Result GE_EnableDFBBld(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL enable)
{
	MS_U16 u16RegVal;

	u16RegVal = GE_CODA_ReadReg(pGEHalLocal, REG_0000_GE0);

	if (enable)
		u16RegVal |= GE_EN_DFB_BLD;
	else
		u16RegVal &= ~GE_EN_DFB_BLD;

	GE_CODA_WriteReg(pGEHalLocal, REG_0000_GE0, u16RegVal);

	return E_GE_OK;
}

GE_Result GE_SetDFBBldFlags(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U16 u16DFBBldFlags)
{
	MS_U16 u16RegVal;

	u16RegVal = (GE_CODA_ReadReg(pGEHalLocal, REG_00AC_GE0) & ~GE_DFB_BLD_FLAGS_MASK);

	if (u16DFBBldFlags & E_GE_DFB_BLD_FLAG_COLORALPHA)
		u16RegVal |= GE_DFB_BLD_FLAG_COLORALPHA;

	if (u16DFBBldFlags & E_GE_DFB_BLD_FLAG_ALPHACHANNEL)
		u16RegVal |= GE_DFB_BLD_FLAG_ALPHACHANNEL;

	if (u16DFBBldFlags & E_GE_DFB_BLD_FLAG_COLORIZE)
		u16RegVal |= GE_DFB_BLD_FLAG_COLORIZE;

	if (u16DFBBldFlags & E_GE_DFB_BLD_FLAG_SRCPREMUL)
		u16RegVal |= GE_DFB_BLD_FLAG_SRCPREMUL;

	if (u16DFBBldFlags & E_GE_DFB_BLD_FLAG_SRCPREMULCOL)
		u16RegVal |= GE_DFB_BLD_FLAG_SRCPREMULCOL;

	if (u16DFBBldFlags & E_GE_DFB_BLD_FLAG_DSTPREMUL)
		u16RegVal |= GE_DFB_BLD_FLAG_DSTPREMUL;

	if (u16DFBBldFlags & E_GE_DFB_BLD_FLAG_XOR)
		u16RegVal |= GE_DFB_BLD_FLAG_XOR;

	if (u16DFBBldFlags & E_GE_DFB_BLD_FLAG_DEMULTIPLY)
		u16RegVal |= GE_DFB_BLD_FLAG_DEMULTIPLY;

	GE_CODA_WriteReg(pGEHalLocal, REG_00AC_GE0, u16RegVal);
	u16RegVal = (GE_CODA_ReadReg(pGEHalLocal, REG_00A8_GE0) & ~GE_DFB_SRC_COLORMASK);

	if (u16DFBBldFlags & (E_GE_DFB_BLD_FLAG_SRCCOLORMASK | E_GE_DFB_BLD_FLAG_SRCALPHAMASK))
		u16RegVal |= (1 << GE_DFB_SRC_COLORMASK_SHIFT);

	return E_GE_OK;
}

GE_Result GE_SetDFBBldOP(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_DFBBldOP geSrcBldOP,
			 GE_DFBBldOP geDstBldOP)
{
	MS_U16 u16RegVal;

	u16RegVal =
	(GE_CODA_ReadReg(pGEHalLocal, REG_00A8_GE0) &
	~(GE_DFB_SRCBLD_OP_MASK | GE_DFB_DSTBLD_OP_MASK));

	u16RegVal |=
	((geSrcBldOP << GE_DFB_SRCBLD_OP_SHFT) | (geDstBldOP << GE_DFB_DSTBLD_OP_SHFT));

	GE_CODA_WriteReg(pGEHalLocal, REG_00A8_GE0, u16RegVal);

	return E_GE_OK;
}

GE_Result GE_SetDFBBldConstColor(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_RgbColor geRgbColor)
{
	MS_U16 u16RegVal;

	u16RegVal =
	    ((GE_CODA_ReadReg(pGEHalLocal, REG_004C_GE0) & ~GE_ALPHA_CONST_MASK) |
	     (geRgbColor.a & 0xFF));
	GE_CODA_WriteReg(pGEHalLocal, REG_004C_GE0, u16RegVal);

	u16RegVal =
	    ((GE_CODA_ReadReg(pGEHalLocal, REG_00B0_GE0) & ~GE_R_CONST_MASK) |
	     ((geRgbColor.r << GE_R_CONST_SHIFT) & GE_R_CONST_MASK));
	GE_CODA_WriteReg(pGEHalLocal, REG_00B0_GE0, u16RegVal);

	u16RegVal =
	    ((GE_CODA_ReadReg(pGEHalLocal, REG_00B0_GE0) & ~GE_G_CONST_MASK) |
	     ((geRgbColor.g << GE_G_CONST_SHIFT) & GE_G_CONST_MASK));
	GE_CODA_WriteReg(pGEHalLocal, REG_00B0_GE0, u16RegVal);

	u16RegVal =
	    ((GE_CODA_ReadReg(pGEHalLocal, REG_00AC_GE0) & ~GE_B_CONST_MASK) |
	     ((geRgbColor.b << GE_B_CONST_SHIFT) & GE_B_CONST_MASK));
	GE_CODA_WriteReg(pGEHalLocal, REG_00AC_GE0, u16RegVal);

	return E_GE_OK;
}

GE_Result GE_GetFmtCaps(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_BufFmt fmt, GE_BufType type,
			GE_FmtCaps *caps)
{
	static const MS_U8 _u8GETileWidth[E_GE_FMT_RGB332 + 1] = { 8, 4, 2, 0, 1, 1 };

	caps->fmt = fmt;
	if (type == E_GE_BUF_SRC) {
		switch (fmt) {
		case E_GE_FMT_I1:
		case E_GE_FMT_I2:
		case E_GE_FMT_I4:
		case E_GE_FMT_I8:
		case E_GE_FMT_RGB332:
			caps->u8BaseAlign = 1;
			caps->u8PitchAlign = 1;
			caps->u8Non1pAlign = 0;
			caps->u8HeightAlign = 1;
			caps->u8StretchAlign = 1;
			caps->u8TileBaseAlign = 0x80;	//[HWBUG] 8;
			caps->u8TileWidthAlign = _u8GETileWidth[fmt];
			caps->u8TileHeightAlign = 16;
			break;
		case E_GE_FMT_RGB565:
		case E_GE_FMT_RGBA5551:
		case E_GE_FMT_RGBA4444:
		case E_GE_FMT_ARGB1555:
		case E_GE_FMT_1ABFgBg12355:
		case E_GE_FMT_ARGB4444:
		case E_GE_FMT_YUV422:
		case E_GE_FMT_FaBaFgBg2266:
		case E_GE_FMT_ABGR1555:
		case E_GE_FMT_BGRA5551:
		case E_GE_FMT_ABGR4444:
		case E_GE_FMT_BGRA4444:
		case E_GE_FMT_BGR565:
			caps->u8BaseAlign = 2;
			caps->u8PitchAlign = 2;
			caps->u8Non1pAlign = 0;
			caps->u8HeightAlign = 1;
			caps->u8StretchAlign = 2;
			caps->u8TileBaseAlign = 0x80;	//[HWBUG] 8;
			caps->u8TileWidthAlign = 16;
			caps->u8TileHeightAlign = 16;
			break;
		case E_GE_FMT_ABGR8888:
		case E_GE_FMT_ARGB8888:
		case E_GE_FMT_RGBA8888:
		case E_GE_FMT_BGRA8888:
		case E_GE_FMT_ACRYCB444:
		case E_GE_FMT_CRYCBA444:
		case E_GE_FMT_ACBYCR444:
		case E_GE_FMT_CBYCRA444:
			caps->u8BaseAlign = 4;
			caps->u8PitchAlign = 4;
			caps->u8Non1pAlign = 0;
			caps->u8HeightAlign = 1;
			caps->u8StretchAlign = 4;
			caps->u8TileBaseAlign = 0x80;	//[HWBUG] 8;
			caps->u8TileWidthAlign = 8;
			caps->u8TileHeightAlign = 16;
			break;
			// Not Support
		default:
			caps->fmt = E_GE_FMT_GENERIC;
			caps->u8BaseAlign = 4;
			caps->u8PitchAlign = 4;
			caps->u8Non1pAlign = 0;
			caps->u8HeightAlign = 1;
			caps->u8StretchAlign = 4;
			caps->u8TileBaseAlign = 0;
			caps->u8TileWidthAlign = 0;
			caps->u8TileHeightAlign = 0;
			return E_GE_FAIL_FORMAT;
		}
	} else {
		switch (fmt) {
		case E_GE_FMT_I8:
		case E_GE_FMT_RGB332:
			caps->u8BaseAlign = 1;
			caps->u8PitchAlign = 1;
			caps->u8Non1pAlign = 0;
			caps->u8HeightAlign = 1;
			caps->u8StretchAlign = 1;
			caps->u8TileBaseAlign = 8;
			caps->u8TileWidthAlign = _u8GETileWidth[fmt];
			caps->u8TileHeightAlign = 16;
			break;
		case E_GE_FMT_RGB565:
		case E_GE_FMT_ARGB1555:
		case E_GE_FMT_RGBA5551:
		case E_GE_FMT_RGBA4444:
		case E_GE_FMT_1ABFgBg12355:
		case E_GE_FMT_ARGB4444:
		case E_GE_FMT_YUV422:
		case E_GE_FMT_FaBaFgBg2266:
		case E_GE_FMT_ARGB1555_DST:
		case E_GE_FMT_ABGR1555:
		case E_GE_FMT_BGRA5551:
		case E_GE_FMT_ABGR4444:
		case E_GE_FMT_BGRA4444:
		case E_GE_FMT_BGR565:
			caps->u8BaseAlign = 2;
			caps->u8PitchAlign = 2;
			caps->u8Non1pAlign = 0;
			caps->u8HeightAlign = 1;
			caps->u8StretchAlign = 2;
			caps->u8TileBaseAlign = 8;
			caps->u8TileWidthAlign = 16;
			caps->u8TileHeightAlign = 16;
			break;
		case E_GE_FMT_ABGR8888:
		case E_GE_FMT_ARGB8888:
		case E_GE_FMT_RGBA8888:
		case E_GE_FMT_BGRA8888:
		case E_GE_FMT_ACRYCB444:
		case E_GE_FMT_CRYCBA444:
		case E_GE_FMT_ACBYCR444:
		case E_GE_FMT_CBYCRA444:
			caps->u8BaseAlign = 4;
			caps->u8PitchAlign = 4;
			caps->u8Non1pAlign = 0;
			caps->u8HeightAlign = 1;
			caps->u8StretchAlign = 4;
			caps->u8TileBaseAlign = 8;
			caps->u8TileWidthAlign = 8;
			caps->u8TileHeightAlign = 16;
			break;
			// Not Support
		case E_GE_FMT_I1:
		case E_GE_FMT_I2:
		case E_GE_FMT_I4:
		default:
			caps->fmt = E_GE_FMT_GENERIC;
			caps->u8BaseAlign = 4;
			caps->u8PitchAlign = 4;
			caps->u8Non1pAlign = 0;
			caps->u8HeightAlign = 1;
			caps->u8StretchAlign = 4;
			caps->u8TileBaseAlign = 0;
			caps->u8TileWidthAlign = 0;
			caps->u8TileHeightAlign = 0;
			return E_GE_FAIL_FORMAT;
		}
	}

	return E_GE_OK;
}

static MS_S32 direct_serial_diff(MS_U16 tagID1, MS_U16 tagID2)
{
	if (tagID1 < tagID2) {
		if ((tagID2 - tagID1) > 0x7FFF)
			return (MS_S32) (0xFFFFUL - tagID2 + tagID1 + 1);
		else
			return -(MS_S32) (tagID2 - tagID1);
	} else {
		if ((tagID1 - tagID2) > 0x7FFF)
			return -(MS_S32) (0xFFFF - tagID1 + tagID2 + 1);
		else
			return (MS_S32) (tagID1 - tagID2);
	}
}

//-------------------------------------------------------------------------------------------------
/// Wait GE TagID back
/// @param  tagID                     \b IN: tag id number for waiting
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result GE_WaitTAGID(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U16 tagID)
{
	MS_U16 tagID_HW;
	MS_U32 u32Temp;
#if (__GE_WAIT_TAG_MODE == __USE_GE_INT_MODE)
	MS_U8 timeout_count = 0;
	MS_U32 event = 0x0;
	MS_BOOL bHadPrintOutDbgMsg = FALSE;
	MS_U8 i = 0;
#endif

#if (__GE_WAIT_TAG_MODE == __USE_GE_INT_MODE)
	tagID_HW = GE_CODA_ReadReg(pGEHalLocal, REG_00C8_GE0);
	if (tagID_HW >= tagID)
		return E_GE_OK;

#if defined(MSOS_TYPE_LINUX)
	s32WaitingTagPid = (MS_S32) getpid();
#endif

	MsOS_ClearEvent(s32GeWaitTagEventHandle, 0x1);

	// unmask ge interrupt
	_GE_Ctrl_IntMode(pGEHalLocal, E_GE_CLEAR_INT | E_GE_UNMASK_INT);

	for (i = 0; i < (GE_WordUnit >> 2); i++) {
		GE_CODA_WriteReg(pGEHalLocal, REG_00D0_GE0, 0);
		GE_CODA_WriteReg(pGEHalLocal, REG_00D4_GE0, tagID);
	}

	while (MsOS_WaitEvent
	       (s32GeWaitTagEventHandle, 0x1, &event, E_OR_CLEAR,
		GE_TAG_INTERRUPT_WAITING_TIME) == FALSE) {
		tagID_HW = GE_CODA_ReadReg(pGEHalLocal, REG_00C8_GE0);
		if (direct_serial_diff(tagID_HW, tagID) >= 0)
			break;

		timeout_count++;

		if ((bHadPrintOutDbgMsg == FALSE)
		    && (timeout_count > GE_TAG_INTERRUPT_DEBUG_PRINT_THRESHOLD)) {
			_GE_Print_GeWaitTagTimeout_Msg(pGEHalLocal, tagID);
			bHadPrintOutDbgMsg = TRUE;
		}

		if (GE_CODA_ReadReg(pGEHalLocal, REG_001C_GE0) & GE_STAT_BUSY)
			continue;

		break;
	}

	// mask ge interrupt
	s32WaitingTagPid = 0;
	_GE_Ctrl_IntMode(pGEHalLocal, E_GE_MASK_INT);

#endif

	while (1) {

		tagID_HW = GE_CODA_ReadReg(pGEHalLocal, REG_00C8_GE0);
		if (direct_serial_diff(tagID_HW, tagID) >= 0) {
			//printf("tagIDHW = %04x %04x\n", tagID_HW, tagID);
			break;
		}

		u32Temp = GE_CODA_ReadReg(pGEHalLocal, REG_001C_GE0);
		if ((u32Temp & GE_STAT_CMDQ_MASK) < (16UL << 11))
			continue;
		if ((u32Temp & GE_STAT_CMDQ2_MASK) < (16UL << 3))
			continue;
		if (GE_CODA_ReadReg(pGEHalLocal, REG_0004_GE0) & GE_CFG_VCMDQ) {
			u32Temp = GE_CODA_ReadReg(pGEHalLocal, REG_0010_GE0);
			u32Temp |= (GE_CODA_ReadReg(pGEHalLocal, REG_0014_GE0) & 1) << 16;
			if (u32Temp)
				continue;

		}

		if (GE_CODA_ReadReg(pGEHalLocal, REG_001C_GE0) & GE_STAT_BUSY)
			continue;

		break;
	}

	return E_GE_OK;

}

//-------------------------------------------------------------------------------------------------
/// MDrv_GE_SAVE_CHIP_IMAGE
//-------------------------------------------------------------------------------------------------
GE_Result GE_Restore_HAL_Context(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL bNotFirstInit)
{
	//MS_U16 i = 0;
	//MS_U16 u16RegVal;
	//uint32_t addr;

	//GE_WaitIdle(pGEHalLocal);


	//while ((_GE_Reg_Backup[i] != REG_GE_INVALID)) {
		//if (_GE_Reg_Backup[i] >= GE_TABLE_REGNUM)
		//	break;
		//addr =(_GE_Reg_Backup[i]*4) + GE0_2410_BASE;
	//	if (bNotFirstInit)
	//		u16RegVal = GE_ReadReg(pGEHalLocal, _GE_Reg_Backup[i]);
	//	else
	//		u16RegVal = GE_ReadReg(pGEHalLocal, _GE_Reg_Backup[i] + GE_TABLE_REGNUM);

	//	GE_CODA_WriteReg(pGEHalLocal, _GE_Reg_Backup[i], u16RegVal);
	//	i++;
//	}


	//GE_DBG(printf("GE_Restore_HAL_Context finished\n\n"));

	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Calculate Blit Scale Ratio:
//-------------------------------------------------------------------------------------------------
GE_Result GE_CalcBltScaleRatio(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U16 u16SrcWidth,
			       MS_U16 u16SrcHeight, MS_U16 u16DstWidth, MS_U16 u16DstHeight,
			       GE_ScaleInfo *pScaleinfo)
{
	if (pScaleinfo == NULL)
		return E_GE_FAIL_PARAM;

	if (u16SrcWidth >= (u16DstWidth << g_GeChipPro.BltDownScaleCaps.u8ShiftRangeMin)) {
		pScaleinfo->x = 0xFFFFFFFF;
	} else {
		pScaleinfo->x =
		    GE_Divide2Fixed(u16SrcWidth, u16DstWidth,
				    g_GeChipPro.BltDownScaleCaps.u8ShiftRangeMin, 12);
	}

	if (u16SrcHeight >= (u16DstHeight << g_GeChipPro.BltDownScaleCaps.u8ShiftRangeMin)) {
		pScaleinfo->y = 0xFFFFFFFF;
	} else {
		pScaleinfo->y =
		    GE_Divide2Fixed(u16SrcHeight, u16DstHeight,
				    g_GeChipPro.BltDownScaleCaps.u8ShiftRangeMin, 12);
	}

	//HW use format S0.12 which means Bit(12) should be Sign bit
	// If overflow, S bit maybe wrong, handle it as actually value we hoped
	pScaleinfo->init_x = GE_Divide2Fixed(u16SrcWidth - u16DstWidth, 2 * u16DstWidth, 0, 12);
	if (u16SrcWidth >= u16DstWidth)
		pScaleinfo->init_x &= (~(1 << 12));
	else
		pScaleinfo->init_x |= (1 << 12);

	pScaleinfo->init_y = GE_Divide2Fixed(u16SrcHeight - u16DstHeight, 2 * u16DstHeight, 0, 12);
	if (u16SrcHeight >= u16DstHeight)
		pScaleinfo->init_y &= (~(1 << 12));
	else
		pScaleinfo->init_y |= (1 << 12);

	if (pGEHalLocal->bYScalingPatch) {
		if (u16SrcHeight <= 5)
			pScaleinfo->init_y = (1 << 12);
	}
	return E_GE_OK;
}

//-------------------------------------------------------------------------------------------------
/// Set GE scale register
/// @param  GE_Rect *src                    \b IN: src coordinate setting
/// @param  GE_DstBitBltType *dst           \b IN: dst coordinate setting
/// @return @ref GE_Result
//-------------------------------------------------------------------------------------------------
GE_Result GE_SetBltScaleRatio(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_Rect *src, GE_DstBitBltType *dst,
			      GE_Flag flags, GE_ScaleInfo *scaleinfo)
{
	GE_ScaleInfo geScaleinfo, *pGeScaleInfo = scaleinfo;

	if (flags & E_GE_FLAG_BYPASS_STBCOEF) {
		_GE_SetBltScaleRatio2HW(pGEHalLocal, pGeScaleInfo);
	} else if (flags & E_GE_FLAG_BLT_STRETCH) {
		/* Safe Guard. Prevent set scaling ratio < 1/32. Also prevent 0 h/w */
		if ((src->width - 1) >=
		    (dst->dstblk.width << g_GeChipPro.BltDownScaleCaps.u8ShiftRangeMin)) {
			if (pGEHalLocal->bIsComp == FALSE)
				return E_GE_FAIL_PARAM;

			dst->dstblk.width =
			    ((src->width - 1) >> g_GeChipPro.BltDownScaleCaps.u8ShiftRangeMin) + 1;
		}
		if ((src->height - 1) >=
		    (dst->dstblk.height << g_GeChipPro.BltDownScaleCaps.u8ShiftRangeMin)) {
			if (pGEHalLocal->bIsComp == FALSE)
				return E_GE_FAIL_PARAM;

			dst->dstblk.height =
			    ((src->height - 1) >> g_GeChipPro.BltDownScaleCaps.u8ShiftRangeMin) + 1;
		}

		pGeScaleInfo = &geScaleinfo;
		GE_CalcBltScaleRatio(pGEHalLocal, src->width, src->height, dst->dstblk.width,
				     dst->dstblk.height, pGeScaleInfo);
		_GE_SetBltScaleRatio2HW(pGEHalLocal, pGeScaleInfo);
	} else {
		pGeScaleInfo = &geScaleinfo;

		pGeScaleInfo->x = (1 << 12);
		pGeScaleInfo->y = (1 << 12);
		pGeScaleInfo->init_x = 0;
		pGeScaleInfo->init_y = 0;

		_GE_SetBltScaleRatio2HW(pGEHalLocal, pGeScaleInfo);
	}

	return E_GE_OK;
}

MS_U16 GE_GetNextTAGID(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL bStepTagBefore)
{
	MS_U16 tagID;

	if (bStepTagBefore) {
		if (0 == ++pGEHalLocal->pHALShared->global_tagID)
			++pGEHalLocal->pHALShared->global_tagID;
	}
	tagID = pGEHalLocal->pHALShared->global_tagID;

	return tagID;
}

GE_Result GE_SetVCmdBuffer(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_PHY PhyAddr, GFX_VcmqBufSize enBufSize)
{
	MS_U16 u16RegVal;

	if (enBufSize >= GFX_VCMD_1024K)
		return E_GE_NOT_SUPPORT;

	HAL_GE_SetBufferAddr(pGEHalLocal, PhyAddr, E_GE_BUF_VCMDQ);

	u16RegVal =
	    (GE_CODA_ReadReg(pGEHalLocal, REG_00A8_GE0) & ~GE_VCMDQ_SIZE_MASK) |
	    ((GE_MapVQ2Reg(enBufSize) & GE_VCMDQ_SIZE_MASK));
	GE_CODA_WriteReg(pGEHalLocal, REG_00A8_GE0, u16RegVal);

	return E_GE_OK;
}

GE_Result GE_InitCtxHalPalette(GE_CTX_HAL_LOCAL *pGEHalLocal)
{
	MS_U32 u32Idx;

	for (u32Idx = 0; u32Idx < GE_PALETTE_NUM; u32Idx++) {
		GE_CODA_WriteReg(pGEHalLocal, REG_00BC_GE0,
			    ((u32Idx) & GE_CLUT_CTRL_IDX_MASK) | GE_CLUT_CTRL_RD);
		GE_WaitIdle(pGEHalLocal);
		pGEHalLocal->u32Palette[u32Idx] =
		    ByteSwap32((((MS_U32) GE_CODA_ReadReg(pGEHalLocal, REG_00B8_GE0) << 16) |
				GE_CODA_ReadReg(pGEHalLocal, REG_00B4_GE0)));
	}

	pGEHalLocal->bPaletteDirty = FALSE;

	return E_GE_OK;
}

void GE_Init_HAL_Context(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_CTX_HAL_SHARED *pHALShared,
			 MS_BOOL bNeedInitShared)
{
	memset(pGEHalLocal, 0, sizeof(*pGEHalLocal));

	if (bNeedInitShared) {
		memset(pHALShared, 0, sizeof(*pHALShared));
		pHALShared->global_tagID = 1;
	}
	pGEHalLocal->pHALShared = pHALShared;
	pGEHalLocal->bYScalingPatch = FALSE;
}

MS_BOOL GE_NonOnePixelModeCaps(GE_CTX_HAL_LOCAL *pGEHalLocal, PatchBitBltInfo *patchInfo)
{
	GE_ScaleInfo geScaleinfo;
	GE_Result ret;

	patchInfo->scaleinfo = &geScaleinfo;
	ret =
	    GE_CalcBltScaleRatio(pGEHalLocal, patchInfo->src.width, patchInfo->src.height,
				 patchInfo->dst.dstblk.width, patchInfo->dst.dstblk.height,
				 patchInfo->scaleinfo);

	if (ret == E_GE_FAIL_PARAM) {
		return pGEHalLocal->pGeChipPro->bFourPixelModeStable;
	} else if ((patchInfo->scaleinfo->x != GE_SCALING_MULITPLIER)
		   || (patchInfo->scaleinfo->y != GE_SCALING_MULITPLIER)) {
		return FALSE;
	} else {
		return pGEHalLocal->pGeChipPro->bFourPixelModeStable;
	}
}

GE_Result HAL_GE_EnableCalcSrc_WidthHeight(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL bEnable)
{
	MS_U16 u16en;

	u16en = GE_CODA_ReadReg(pGEHalLocal, REG_0000_GE0);

	if (bEnable) {
		if (u16en & GE_EN_BURST)
			GE_CODA_WriteReg(pGEHalLocal, REG_0000_GE0, u16en | GE_EN_CALC_SRC_WH);

	} else {
		GE_CODA_WriteReg(pGEHalLocal, REG_0000_GE0, u16en & (~GE_EN_CALC_SRC_WH));
	}

	return E_GE_OK;
}

GE_Result HAL_GE_AdjustDstWin(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_BOOL bDstXInv)
{
	MS_U16 u16ClipL = 0, u16ClipR = 0;
	MS_U16 u16DstX = 0;

	u16DstX = GE_CODA_ReadReg(pGEHalLocal, REG_01A8_GE0);
	if (bDstXInv == FALSE) {
		u16ClipR = GE_CODA_ReadReg(pGEHalLocal, REG_0158_GE0);
		if (u16ClipR < u16DstX)
			GE_CODA_WriteReg(pGEHalLocal, REG_01A8_GE0, u16ClipR);

	} else {
		u16ClipL = GE_CODA_ReadReg(pGEHalLocal, REG_0154_GE0);
		if (u16ClipL > u16DstX)
			GE_CODA_WriteReg(pGEHalLocal, REG_01A8_GE0, u16ClipL);
	}

	return E_GE_OK;
}

GE_Result HAL_GE_SetBufferAddr(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_PHY PhyAddr,
			       GE_BufType enBuffType)
{
	MS_U16 u16tmp_REG_GE_BASE_MSB = 0;

	switch (enBuffType) {
	case E_GE_BUF_SRC:
		GE_CODA_WriteReg(pGEHalLocal, REG_0080_GE0, (PhyAddr & 0xFFFF));
		GE_CODA_WriteReg(pGEHalLocal, REG_0084_GE0, ((PhyAddr >> 16) & 0x7FFF));
		u16tmp_REG_GE_BASE_MSB =
		    (GE_CODA_ReadReg(pGEHalLocal, REG_008C_GE0) & ~GE_SRC_MSB_MASK);
		u16tmp_REG_GE_BASE_MSB =
			(u16tmp_REG_GE_BASE_MSB | ((PhyAddr >> 31) & 0x7));
		GE_CODA_WriteReg(pGEHalLocal, REG_008C_GE0, u16tmp_REG_GE_BASE_MSB);
		break;
	case E_GE_BUF_DST:
		GE_CODA_WriteReg(pGEHalLocal, REG_0098_GE0, (PhyAddr & 0xFFFF));
		GE_CODA_WriteReg(pGEHalLocal, REG_009C_GE0, ((PhyAddr >> 16) & 0x7FFF));
		u16tmp_REG_GE_BASE_MSB =
			(GE_CODA_ReadReg(pGEHalLocal, REG_008C_GE0) & ~GE_DST_MSB_MASK);
		u16tmp_REG_GE_BASE_MSB =
			(u16tmp_REG_GE_BASE_MSB | (((PhyAddr >> 31) & 0x7) << 4));
		GE_CODA_WriteReg(pGEHalLocal, REG_008C_GE0, u16tmp_REG_GE_BASE_MSB);
		break;
	case E_GE_BUF_VCMDQ:
		GE_CODA_WriteReg(pGEHalLocal, REG_00A0_GE0, (PhyAddr & 0xFFFF));
		GE_CODA_WriteReg(pGEHalLocal, REG_00A4_GE0, ((PhyAddr >> 16) & 0x7FFF));
		u16tmp_REG_GE_BASE_MSB =
			(GE_CODA_ReadReg(pGEHalLocal, REG_008C_GE0) & ~GE_VCMDQ_MSB_MASK);
		u16tmp_REG_GE_BASE_MSB =
			(u16tmp_REG_GE_BASE_MSB | (((PhyAddr >> 31) & 0x7) << 8));
		GE_CODA_WriteReg(pGEHalLocal, REG_008C_GE0, u16tmp_REG_GE_BASE_MSB);
		break;
	default:
		G2D_ERROR("Buffer Type Error!!!!\n");
		break;
	}

	return E_GE_OK;
}

GE_Result HAL_GE_GetCRC(GE_CTX_HAL_LOCAL *pGEHalLocal, MS_U32 *pu32CRCvalue)
{
	MS_U16 u16CRC_L = 0;
	MS_U16 u16CRC_H = 0;

	u16CRC_L = mtk_read2byte(REG_0050_GE2);
	u16CRC_H = mtk_read2byte(REG_0054_GE2);

	*pu32CRCvalue = (((MS_U32) u16CRC_H << 16) | u16CRC_L);

	return E_GE_OK;
}

#if defined(MSOS_TYPE_LINUX_KERNEL)
GE_Result HAL_GE_STR_RestoreReg(GE_CTX_HAL_LOCAL *pGEHalLocal, GE_STR_SAVE_AREA *pGFX_STRPrivate)
{
	MS_U8 i = 0UL;

	//enable power
	//STR_CLK_REG(CHIP_GE_CLK) = pGFX_STRPrivate->GECLK_Reg;

	for (i = 0; _GE_Reg_Backup[i] < REG_GE_INVALID; ++i) {
		mtk_write2byte((_GE_Reg_Backup[i]*4) + GE0_2410_BASE,
			pGFX_STRPrivate->GE0_Reg[_GE_Reg_Backup[i]]);
	}

	return E_GE_OK;
}
#endif

