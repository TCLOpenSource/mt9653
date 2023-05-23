// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/string.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include "mtk_pcmcia_hal_diff.h"

#include "mtk_pcmcia_drv.h"
#include "mtk_pcmcia_hal.h"
#include "mtk_pcmcia_private.h"

#include "mtk_pcmcia_stub.h"


#define MLP_CI_CORE_ENTRY(fmt, arg...)	//pr_info("[mdbgin_CADD_CI_DRV]" fmt, ##arg)

int mtk_pcmcia_drv_stub;

module_param_named(cam_drv_stub, mtk_pcmcia_drv_stub, int, 0644);
MODULE_PARM_DESC(cam_drv_stub, "enable stub messages");

#define PCMCIA_MAX_DETECT_COUNT			1UL
#define PCMCIA_MAX_POLLING_COUNT		20000UL
#define PCMCIA_DEFAULT_RESET_DURATION	20UL
#define PCMCIA_HW_MAX_RETRY_COUNT		100UL
#define PCMCIA_RESET_RETRY_COUNT		3
#define PCMCIA_DELAY_LOOP_COUNT			50

#define PCMCIA_UTOPIA2					TRUE

#define MDrv_MSPI_SlaveEnable_Channel(x, y)
#define MDrv_MSPI_Write_Channel(x, y, z) mtk_pcmcia_to_spi_write(y, z)
#define MDrv_MSPI_Read_Channel(v, w, x, y, z) mtk_pcmcia_to_spi_read(w, x, y, z)
#define MDrv_MSPI_RWBytes(x, y)

#define SCALE_FACTOR 2
#define SHIFT_FACTOR 8

#if PCMCIA_IRQ_ENABLE
static unsigned char _gbPCMCIA_Irq[E_PCMCIA_MODULE_MAX] = { 0 };
static unsigned char _gbPCMCIA_IrqStatus[E_PCMCIA_MODULE_MAX] = { FALSE };
#endif
static unsigned char _gu8PCMCIA_Command[E_PCMCIA_MODULE_MAX] = { 0 };

static unsigned char _gu8HW_ResetDuration = PCMCIA_DEFAULT_RESET_DURATION;

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
static signed int SYS_fd = -1;

#if PCMCIA_UTOPIA2
extern void *pModulePcm;

#else
static signed int Pcmcia_Mutex = -1;
#endif
/* PCMCIA_MAP_IOC_INFO */
typedef struct {
	unsigned short addr;
	unsigned char u8Value;
	unsigned char u8Type;		// 1: AttribMem, 2: IOMem
	unsigned short data_len;
	unsigned char *read_buffer;
	unsigned char *u8pWriteBuffer;
} PCMCIA_Map_Info_t;
#endif

#if (PCMCIA_MSPI_BURST)
static unsigned char u8MspiBuf[256];
#endif
extern int ci_interrupt_irq;



#define TAG_PCMCIA "PCMCIA"

#define PCMCIA_DBG_FUNC()
#define PCMCIA_DBG_INFO(x, args...) pr_info(x, ##args)
#define PCMCIA_DBG_REG_DUMP(x, args...)
#define PCMCIA_DBG_ERR(x, args...)
#define ULOGE(TAG_PCMCIA, x, args...) pr_info(x, ##args)
#define MsOS_EnableInterrupt(x)	 enable_irq((int)x)
#define MsOS_CompleteInterrupt(x)	//wait
#define MsOS_DisableInterrupt(x) disable_irq_nosync((int)x)
#define MsOS_AttachInterrupt(x, x1)
#define UTOPIA_STATUS_SUCCESS				0x00000000
#define UTOPIA_STATUS_FAIL					0x00000001
#define MsOS_DelayTask(x)  msleep_interruptible((unsigned int)x)

#define PCMCIA_PERF 0



#define PCM_ENTER()
#define PCM_EXIT()


#define BIT0  0x0001UL
#define BIT1  0x0002UL
#define BIT2  0x0004UL
#define BIT3  0x0008UL
#define BIT4  0x0010UL
#define BIT5  0x0020UL
#define BIT6  0x0040UL
#define BIT7  0x0080UL
#define BIT8  0x0100UL
#define BIT9  0x0200UL
#define BIT10 0x0400UL
#define BIT11 0x0800UL
#define BIT12 0x1000UL
#define BIT13 0x2000UL
#define BIT14 0x4000UL
#define BIT15 0x8000UL

#define PCMCIA_FIRE_COMMAND			BIT0
#define PCMCIA_CLEAN_STATE_RD_DONE	BIT1
#define PCMCIA_STATE_RD_DONE		BIT0
#define PCMCIA_STATE_BUS_IDLE		BIT1
#define PCMCIA_DETECT_PIN_MODULEA	BIT2
#define PCMCIA_DETECT_PIN_MODULEB	BIT3

/* Table 2-6 Tuple Summary Tabl (Spec P.24)*/
/* Layer 1 Tuples */
#define CISTPL_NULL					0x00UL
#define CISTPL_DEVICE				0x01UL
#define CISTPL_LONGLINK_CB			0x02UL
#define CISTPL_INDIRECT				0x03UL
#define CISTPL_CONFIG_CB			0x04UL
#define CISTPL_CFTABLE_ENTRY_CB		0x05UL
#define CISTPL_LONGLINK_MFC			0x06UL
#define CISTPL_BAR					0x07UL
#define CISTPL_PWR_MGMNT			0x08UL
#define CISTPL_EXTDEVICE			0x09UL
#define CISTPL_CHECKSUM				0x10UL
#define CISTPL_LONGLINK_A			0x11UL
#define CISTPL_LONGLINK_C			0x12UL
#define CISTPL_LINKTARGET			0x13UL
#define CISTPL_NO_LINK				0x14UL
#define CISTPL_VERS_1				0x15UL
#define CISTPL_ALTSTR				0x16UL
#define CISTPL_DEVICE_A				0x17UL
#define CISTPL_JEDEC_C				0x18UL
#define CISTPL_JEDEC_A				0x19UL
#define CISTPL_CONFIG				0x1AUL
#define CISTPL_CFTABLE_ENTRY		0x1BUL
#define CISTPL_DEVICE_OC			0x1CUL
#define CISTPL_DEVICE_OA			0x1DUL
#define CISTPL_DEVICE_GEO			0x1EUL
#define CISTPL_DEVICE_GEO_A			0x1FUL
#define CISTPL_MANFID				0x20UL
#define CISTPL_FUNCID				0x21UL
#define CISTPL_FUNCE				0x22UL
#define CISTPL_END					0xFFUL
/* Layer 2 Tuples */
#define CISTPL_SWIL					0x23UL
#define CISTPL_VERS_2				0x40UL
#define CISTPL_FORMAT				0x41UL
#define CISTPL_GEOMETRY				0x42UL
#define CISTPL_BYTEORDER			0x43UL
#define CISTPL_DATE					0x44UL
#define CISTPL_BATTERY				0x45UL
#define CISTPL_FORMAT_A				0x47UL
/* Layer 3 Tuples */
#define CISTPL_ORG					0x46UL
/* Layer 4 Tuples */
#define CISTPL_SPCL					0x90UL
#ifdef MSOS_TYPE_LINUX_KERNEL
#define PCM_TASK_EVENTS				   0xFFFFFFFF
#define PCM_EVENT(type)				   (0x1 << type)
#define PCM_EVENT_GET(event, type)		(event & (0x1 << type))
#endif

/// Version string

static unsigned char _gbHighActive;
static unsigned char _gbCardInside[E_PCMCIA_MODULE_MAX];
static unsigned char _gbPCMCIA_Detect_Enable;
static unsigned int _gu32PCMCIA_CD_To_HWRST_Timer[E_PCMCIA_MODULE_MAX];
static unsigned char _u8PCMCIACurModule = PCMCIA_DEFAULT_MODULE;
#if PCMCIA_IRQ_ENABLE
static IsrCallback _fnIsrCallback[E_PCMCIA_MODULE_MAX] = { NULL };

static PCMCIA_ISR _gPCMCIA_ISR;
#endif
static unsigned char _gbCardReady = FALSE;

static unsigned char _mdrv_pcmcia_ReadReg(unsigned int u32Addr, unsigned char *pu8Value);
static unsigned char _mdrv_pcmcia_WriteReg(unsigned int u32Addr, unsigned char u8Value);
static void _mdrv_pcmcia_COMPANION_SetGPIO(PCMCIA_MODULE module,
					   unsigned char bHigh);
#if (PCMCIA_MSPI_BURST)
static unsigned char _mdrv_pcmcia_WriteRegMask(unsigned int u32Addr, unsigned char u8Value,
					 unsigned char u8Mask);
static unsigned char _MSPI_RWLong(unsigned char u8Cmd, unsigned short addr, unsigned char *u8data,
				unsigned short u16Len);
static void _MSPI_BurstRst(void);
static unsigned char _MSPI_MIU_CheckDone(void);
#endif

static unsigned char _mdrv_pcmcia_ReadReg(unsigned int u32Addr, unsigned char *pu8Value)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//Check
	if (pu8Value == NULL)
		return FALSE;

	*pu8Value = HAL_PCMCIA_Read_Byte(u32Addr);

	return TRUE;
}

static unsigned char _mdrv_pcmcia_WriteReg(unsigned int u32Addr, unsigned char u8Value)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	HAL_PCMCIA_Write_Byte(u32Addr, u8Value);

	return TRUE;
}

#if (PCMCIA_MSPI_BURST)
static unsigned char _mdrv_pcmcia_WriteRegMask(unsigned int u32Addr, unsigned char u8Value,
					 unsigned char u8Mask)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	HAL_PCMCIA_Write_ByteMask(u32Addr, u8Value, u8Mask);
	return TRUE;
}
#endif
static void _mdrv_pcmcia_SwitchModule(PCMCIA_MODULE module)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	unsigned char u8Reg = 0x0;

	if (_u8PCMCIACurModule == module)
		return;

	_mdrv_pcmcia_ReadReg(REG_PCMCIA_MODULE_VCC_OOB, (unsigned char *) &u8Reg);
	u8Reg &= ~(BIT0 | BIT1);

	//	MODULE_SEL[1:0] 1:0		Module select.
	//	00: No destination selected.
	//	01: Select module A.
	//	10: Select module B.
	//	11: Reserved.

	if (module == E_PCMCIA_MODULE_A)
		u8Reg |= BIT0;
	else
		u8Reg |= BIT1;


	_mdrv_pcmcia_WriteReg(REG_PCMCIA_MODULE_VCC_OOB, u8Reg);
	_u8PCMCIACurModule = module;
}

#if (PCMCIA_MSPI_BURST)
static unsigned char _MSPI_RWLong(unsigned char u8Cmd, unsigned short addr, unsigned char *u8data,
				unsigned short u16Len)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	pr_info("[%s][%d]\n", __func__, __LINE__);
	unsigned short i = 0;
	unsigned short Size = 0;
	unsigned short SizePcmBurst = 0;
	unsigned char bRet = TRUE;
	unsigned char u8reg = 0;

	if (u8Cmd == PCMCIA_ATTRIBMEMORY_WRITE || u8Cmd == PCMCIA_IO_WRITE) {
		while (u16Len) {
			SizePcmBurst =
				(u16Len >
				 MAX_PCMCIA_BURST_WRITE_SIZE) ?
				MAX_PCMCIA_BURST_WRITE_SIZE : u16Len;
			//pr_info("PCM Burst %d, left %d\n", (int)SizePcmBurst, (int)u16Len);

			u16Len -= SizePcmBurst;
			i = 0;
			u8MspiBuf[i++] = MSPI_CMD_MIU_W;
			u8MspiBuf[i++] =
				(REG_COMPANION_PCMBURST_ADDR >> 0) & 0xFF;
			u8MspiBuf[i++] =
				(REG_COMPANION_PCMBURST_ADDR >> 8) & 0xFF;
			u8MspiBuf[i++] =
				(REG_COMPANION_PCMBURST_ADDR >> 16) & 0xFF;
			u8MspiBuf[i++] =
				(REG_COMPANION_PCMBURST_ADDR >> 24) & 0xFF;
			u8MspiBuf[i++] = 0x0;	// reg_status_expect
			u8MspiBuf[i++] = 0x0;	// reg_status_mask
			u8MspiBuf[i++] = 0x0;	// reg_pcm_burst_addr_offset
			u8MspiBuf[i++] = 0x0;
			u8MspiBuf[i++] = u8Cmd;	// reg_pcm_cmd
			u8MspiBuf[i++] = 0x0;
			u8MspiBuf[i++] = addr & 0xFF;	// reg_adr
			u8MspiBuf[i++] = (addr >> 8) & 0xFF;
			u8MspiBuf[i++] = SizePcmBurst;	// reg_total_burst_num
			u8MspiBuf[i++] = 0;
			u8MspiBuf[i++] = PCMBURST_WRITE & 0xFF;
			u8MspiBuf[i++] = (PCMBURST_WRITE >> 8) & 0xFF;

			MDrv_MSPI_RWBytes(MSPI_READ_OPERATION, 0);	// clean rbf_size
			while (SizePcmBurst) {
				Size =
					(SizePcmBurst >
					 MAX_MSPI_BURST_WRITE_SIZE -
					 i) ? (MAX_MSPI_BURST_WRITE_SIZE -
					   i) : SizePcmBurst;


				MDrv_MSPI_SlaveEnable_Channel
					(MOLESKINE_MSPI_CHANNEL, TRUE);
				MDrv_MSPI_Write_Channel(MOLESKINE_MSPI_CHANNEL,
							u8MspiBuf, i);

				//MsOS_DelayTask(100);
				MDrv_MSPI_Write_Channel(MOLESKINE_MSPI_CHANNEL,
							u8data, Size);
				MDrv_MSPI_SlaveEnable_Channel
					(MOLESKINE_MSPI_CHANNEL, FALSE);
				SizePcmBurst -= Size;
				u8data += Size;
				//MsOS_DelayTask(100);
				i = 0;
				u8MspiBuf[i++] = MSPI_CMD_MIU_W;
				u8MspiBuf[i++] =
					(REG_COMPANION_PCMBURST_WFIFO >> 0) & 0xFF;
				u8MspiBuf[i++] =
					(REG_COMPANION_PCMBURST_WFIFO >> 8) & 0xFF;
				u8MspiBuf[i++] =
					(REG_COMPANION_PCMBURST_WFIFO >> 16) & 0xFF;
				u8MspiBuf[i++] =
					(REG_COMPANION_PCMBURST_WFIFO >> 24) & 0xFF;
			}

			for (i = 0; i < MAX_MSPI_STATUS_COUNT; i++) {
				_mdrv_pcmcia_ReadReg(REG_PCM_BURST_STATUS_0,
							 &u8reg);
				if (u8reg & REG_PCM_BURST_WRITE_DONE) {
					_mdrv_pcmcia_WriteRegMask
						(REG_PCM_BURST_STATUS_CLR,
						 REG_PCM_WRITE_FINISH_CLR,
						 REG_PCM_WRITE_FINISH_CLR);
					break;
				}
			}
			if (i == MAX_MSPI_STATUS_COUNT)
				pr_info
					("[PCM] warning, BURST Write not finish\n");

			_mdrv_pcmcia_ReadReg(REG_PCM_BURST_WFIFO_RMN, &u8reg);
			if (u8reg != 0)
				pr_info("[PCM] warning, WFIFO not empty %d\n",
					   (int)u8reg);

		}
	} else if (u8Cmd == PCMCIA_ATTRIBMEMORY_READ || u8Cmd == PCMCIA_IO_READ) {
		while (u16Len) {
			//unsigned short index = 0;
			SizePcmBurst =
				(u16Len >
				 MAX_PCMCIA_BURST_READ_SIZE) ?
				MAX_PCMCIA_BURST_READ_SIZE : u16Len;
			//pr_info("PCM Burst %d, left %d\n", (int)SizePcmBurst, (int)u16Len);

			u16Len -= SizePcmBurst;

			i = 0;
			u8MspiBuf[i++] = MSPI_CMD_MIU_W;
			u8MspiBuf[i++] =
				(REG_COMPANION_PCMBURST_ADDR >> 0) & 0xFF;
			u8MspiBuf[i++] =
				(REG_COMPANION_PCMBURST_ADDR >> 8) & 0xFF;
			u8MspiBuf[i++] =
				(REG_COMPANION_PCMBURST_ADDR >> 16) & 0xFF;
			u8MspiBuf[i++] =
				(REG_COMPANION_PCMBURST_ADDR >> 24) & 0xFF;
			u8MspiBuf[i++] = 0x0;	// reg_status_expect
			u8MspiBuf[i++] = 0x0;	// reg_status_mask
			u8MspiBuf[i++] = 0x0;	// reg_pcm_burst_addr_offset
			u8MspiBuf[i++] = 0x0;
			u8MspiBuf[i++] = u8Cmd;	// reg_pcm_cmd
			u8MspiBuf[i++] = 0x0;
			u8MspiBuf[i++] = addr & 0xFF;	// reg_adr
			u8MspiBuf[i++] = (addr >> 8) & 0xFF;
			u8MspiBuf[i++] = SizePcmBurst;	// reg_total_burst_num
			u8MspiBuf[i++] = 0;
			u8MspiBuf[i++] = PCMBURST_READ & 0xFF;
			u8MspiBuf[i++] = (PCMBURST_READ >> 8) & 0xFF;

			MDrv_MSPI_RWBytes(MSPI_READ_OPERATION, 0);	// clean rbf_size
			MDrv_MSPI_SlaveEnable_Channel(MOLESKINE_MSPI_CHANNEL,
							  TRUE);
			MDrv_MSPI_Write_Channel(MOLESKINE_MSPI_CHANNEL,
						u8MspiBuf, i);
			MDrv_MSPI_SlaveEnable_Channel(MOLESKINE_MSPI_CHANNEL,
							  FALSE);

			for (i = 0; i < MAX_MSPI_STATUS_COUNT; i++) {
				_mdrv_pcmcia_ReadReg(REG_PCM_BURST_STATUS_0,
							 &u8reg);
				if (u8reg & REG_PCM_BURST_READ_DONE) {
					_mdrv_pcmcia_WriteRegMask
						(REG_PCM_BURST_STATUS_CLR,
						 REG_PCM_READ_FINISH_CLR,
						 REG_PCM_READ_FINISH_CLR);
					break;

				}
				MsOS_DelayTask(1);
			}
			if (i == MAX_MSPI_STATUS_COUNT) {
				pr_info
					("[PCM] warning, BURST Write not finish\n");
			}

			while (SizePcmBurst) {
				unsigned char u8TmpBuf[MAX_MSPI_BURST_READ_SIZE + 1];

				Size =
					(SizePcmBurst >
					 MAX_MSPI_BURST_READ_SIZE) ?
					MAX_MSPI_BURST_READ_SIZE : SizePcmBurst;
				_MSPI_MIU_CheckDone();


				i = 0;
				u8MspiBuf[i++] = MSPI_CMD_MIU_R;
				u8MspiBuf[i++] =
					(REG_COMPANION_PCMBURST_RFIFO >> 0) & 0xFF;
				u8MspiBuf[i++] =
					(REG_COMPANION_PCMBURST_RFIFO >> 8) & 0xFF;
				u8MspiBuf[i++] =
					(REG_COMPANION_PCMBURST_RFIFO >> 16) & 0xFF;
				u8MspiBuf[i++] =
					(REG_COMPANION_PCMBURST_RFIFO >> 24) & 0xFF;
				u8MspiBuf[i++] = Size + 1;	// 1 for MSPI status
				MDrv_MSPI_RWBytes(MSPI_READ_OPERATION, 0);	// clean rbf_size
				MDrv_MSPI_SlaveEnable_Channel
					(MOLESKINE_MSPI_CHANNEL, TRUE);
				MDrv_MSPI_Write_Channel
					(MOLESKINE_MSPI_CHANNEL, u8MspiBuf, i);
				MDrv_MSPI_SlaveEnable_Channel
					(MOLESKINE_MSPI_CHANNEL, FALSE);

				_MSPI_MIU_CheckDone();
				u8MspiBuf[0] = MSPI_CMD_MIU_ST;
				MDrv_MSPI_SlaveEnable_Channel
					(MOLESKINE_MSPI_CHANNEL, TRUE);
				//MDrv_MSPI_Write_Channel(MOLESKINE_MSPI_CHANNEL, u8MspiBuf, 1);
				MDrv_MSPI_RWBytes(MSPI_WRITE_OPERATION, 0);	// clean rbf_size
				MDrv_MSPI_Read_Channel(MOLESKINE_MSPI_CHANNEL,
					u8MspiBuf, 1, u8TmpBuf, Size + 1);
				MDrv_MSPI_SlaveEnable_Channel
					(MOLESKINE_MSPI_CHANNEL, FALSE);
				if (u8TmpBuf[0] != 0x0A) {
					pr_info
						("[PCM] warning, MIU_ST status 0x%x\n",
						 (int)u8TmpBuf[0]);
				}

				memcpy(u8data, &u8TmpBuf[1], Size);
				SizePcmBurst -= Size;
				u8data += Size;
				_mdrv_pcmcia_ReadReg(REG_PCM_BURST_RFIFO_RMN,
							 &u8reg);
				if (u8reg != SizePcmBurst)
					pr_info
						("[PCM] warning, RFIFO rmn %d, SizePcmBurst %d\n",
						 (int)u8reg, (int)SizePcmBurst);

				MsOS_DelayTask(10);
			}

			_mdrv_pcmcia_ReadReg(REG_PCM_BURST_RFIFO_RMN, &u8reg);
			if (u8reg != 0)
				pr_info("[PCM] warning, RFIFO not empty, %d\n",
					   (int)u8reg);
		}
	}
	return bRet;
}

static unsigned char _MSPI_MIU_CheckDone(void)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	pr_info("[%s][%d]\n", __func__, __LINE__);
	unsigned char u8tmp = 0;
	int i = 0;

	u8MspiBuf[0] = MSPI_CMD_MIU_ST;

	for (; i < MAX_MSPI_STATUS_COUNT; i++) {
		MDrv_MSPI_SlaveEnable_Channel(MOLESKINE_MSPI_CHANNEL, TRUE);
		MDrv_MSPI_RWBytes(MSPI_READ_OPERATION, 0);	// clean rbf_size
		//MDrv_MSPI_Write_Channel(MOLESKINE_MSPI_CHANNEL, u8MspiBuf, 1);
		MDrv_MSPI_RWBytes(MSPI_WRITE_OPERATION, 0);	// clean rbf_size
		MDrv_MSPI_Read_Channel(MOLESKINE_MSPI_CHANNEL, u8MspiBuf, 1, &u8tmp, 1);
		MDrv_MSPI_SlaveEnable_Channel(MOLESKINE_MSPI_CHANNEL, FALSE);
		if (u8tmp == MSPI_MIU_STATUS_DONE
			|| u8tmp == MSPI_MIU_STATUS_NONE)
			break;
	}
	MDrv_MSPI_RWBytes(MSPI_READ_OPERATION, 0);	// clean rbf_size
	MDrv_MSPI_RWBytes(MSPI_WRITE_OPERATION, 0);	// clean rbf_size

	if (i == MAX_MSPI_STATUS_COUNT) {
		pr_info("[PCM] MSPI MIU timeout, status 0x%x\n", (int)u8tmp);
		return FALSE;
	}
	return TRUE;
}

static void _MSPI_BurstRst(void)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	_mdrv_pcmcia_WriteRegMask(REG_PCM_BURST_CTRL, REG_PCM_BURST_SW_RST_ON,
				  REG_PCM_BURST_SW_RST_MASK);
	_mdrv_pcmcia_WriteRegMask(REG_PCM_BURST_CTRL, REG_PCM_BURST_SW_RST_OFF,
				  REG_PCM_BURST_SW_RST_MASK);
}
#endif

#if PCMCIA_IRQ_ENABLE
void mdrv_pcmcia_isr(void)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	ISR_STS IsrSts;
	PCMCIA_MODULE module = E_PCMCIA_MODULE_A;

	memset(&_gPCMCIA_ISR, 0x00, sizeof(_gPCMCIA_ISR));
	memset(&IsrSts, 0x00, sizeof(ISR_STS));
	pr_info("[%s][%d]\n", __func__, __LINE__);
	/* MASK PCMCIA IRQ */
	HAL_PCMCIA_GetIntStatus(&IsrSts);
	HAL_PCMCIA_MaskInt(0x0, TRUE);
	HAL_PCMCIA_ClrInt(0x0);

	if (IsrSts.bCardAInsert || IsrSts.bCardARemove || IsrSts.bCardAData) {
		//ULOGE("PCMCIA", "[%s][%d]\n", __FUNCTION__, __LINE__);
		if (_gbPCMCIA_Irq[E_PCMCIA_MODULE_A])
			_gbPCMCIA_IrqStatus[E_PCMCIA_MODULE_A] = TRUE;

		if (IsrSts.bCardAInsert) {
			_gPCMCIA_ISR.bISRCardInsert = TRUE;
			ULOGE("PCMCIA", "[%s][%d] bISRCardInsert\n",
				  __func__, __LINE__);
		}
		if (IsrSts.bCardARemove) {
			_gPCMCIA_ISR.bISRCardRemove = TRUE;
			ULOGE("PCMCIA", "[%s][%d] bISRCardRemove\n",
				  __func__, __LINE__);
		}
		if (IsrSts.bCardAData) {
			_gPCMCIA_ISR.bISRCardData = TRUE;
			//ULOGE("PCMCIA", "[%s][%d] bISRCardData\n", __FUNCTION__, __LINE__);
		}
		if (_fnIsrCallback[E_PCMCIA_MODULE_A] != NULL) {
			module = E_PCMCIA_MODULE_A;
			_fnIsrCallback[E_PCMCIA_MODULE_A] ((void
								*)(&_gPCMCIA_ISR),
							   (void *)&module);
		}
	} else if (IsrSts.bCardBInsert || IsrSts.bCardBRemove
		   || IsrSts.bCardBData) {
		if (_gbPCMCIA_Irq[E_PCMCIA_MODULE_B])
			_gbPCMCIA_IrqStatus[E_PCMCIA_MODULE_B] = TRUE;

		if (IsrSts.bCardBInsert)
			_gPCMCIA_ISR.bISRCardInsert = TRUE;

		if (IsrSts.bCardBRemove)
			_gPCMCIA_ISR.bISRCardRemove = TRUE;

		if (IsrSts.bCardBData)
			_gPCMCIA_ISR.bISRCardData = TRUE;

		if (_fnIsrCallback[E_PCMCIA_MODULE_B] != NULL) {
			module = E_PCMCIA_MODULE_B;
			_fnIsrCallback[E_PCMCIA_MODULE_B] ((void
								*)(&_gPCMCIA_ISR),
							   (void *)&module);
		}
	} else {
#ifndef CONFIG_PCMCIA_MSPI
		ULOGE("PCMCIA", "[PCMCIA] IRQ but nothing happen\n");
		//MS_ASSERT( 0 );
#endif
	}



	/* Enable HK PCMCIA IRQ */
	//MsOS_EnableInterrupt(ci_interrupt_irq);
	//MsOS_CompleteInterrupt(E_INT_IRQ_PCM); kernel mode not need

	/* UNMASK PCMCIA IRQ */
	HAL_PCMCIA_MaskInt(0x0, FALSE);
	pr_info("[%s][%d]\n", __func__, __LINE__);
}
#endif

void _mdrv_pcmcia_Exit(unsigned char bSuspend)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	if (bSuspend == FALSE) {

	} else {
		// suspend
#if PCMCIA_IRQ_ENABLE
		MsOS_DisableInterrupt(ci_interrupt_irq);
#endif
	}
	HAL_PCMCIA_ClkCtrl(FALSE);

}


void mdrv_pcmcia_init_sw(void __iomem *riu_vaddr, void __iomem *ext_riu_vaddr,
			 unsigned char bHighActiveTrigger)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	pr_info("[%s][%d]\n", __func__, __LINE__);

	unsigned int i = 0;

	if (riu_vaddr && ext_riu_vaddr) {
		pr_info("[%s][%d]\n", __func__, __LINE__);
#if (!PCMCIA_MSPI_SUPPORTED)
		HAL_PCMCIA_Set_RIU_base(riu_vaddr);
		HAL_Set_EXTRIU_base(ext_riu_vaddr);
#endif
	}
	pr_info("[%s][%d]\n", __func__, __LINE__);

	_gbHighActive = bHighActiveTrigger;
	pr_info("[%s][%d]\n", __func__, __LINE__);

	for (i = 0; i < E_PCMCIA_MODULE_MAX; i++) {
		_gbCardInside[i] = FALSE;
		_gu32PCMCIA_CD_To_HWRST_Timer[i] = 0;
		_gu8PCMCIA_Command[i] = 0;
#if PCMCIA_IRQ_ENABLE
		_fnIsrCallback[i] = NULL;
		_gbPCMCIA_Irq[i] = false;
		_gbPCMCIA_IrqStatus[i] = false;
#endif
	}
	pr_info("[%s][%d]\n", __func__, __LINE__);

	_u8PCMCIACurModule = PCMCIA_DEFAULT_MODULE;
	_gbPCMCIA_Detect_Enable = TRUE;
	pr_info("[%s][%d]\n", __func__, __LINE__);


}

void _mdrv_pcmcia_InitHW(unsigned char bResume)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	pr_info("[%s][%d]\n", __func__, __LINE__);
	unsigned char u8Reg;


	HAL_PCMCIA_ClkCtrl(TRUE);

#if (PCMCIA_MSPI_BURST)
	_MSPI_BurstRst();
#endif
	/* Initailze PCMCIA Registers. */
	_mdrv_pcmcia_ReadReg(REG_PCMCIA_MODULE_VCC_OOB, (unsigned char *) &u8Reg);

	u8Reg = (BIT6 | BIT0);	// reg_module_sel(BIT 1~0): module select
	//							01: select module A
	// reg_single_card(BIT4):	Only support single card
	//							1: support 1 card only
	// reg_vcc_en(BIT5):		0: VCC Disable
	// reg_oob_en(BIT6):		1: OOB enable

	_mdrv_pcmcia_WriteReg(REG_PCMCIA_MODULE_VCC_OOB, u8Reg);


#if PCMCIA_IRQ_ENABLE

	if (bResume == FALSE)
		MsOS_AttachInterrupt(E_INT_IRQ_PCM, (InterruptCb) _mdrv_pcmcia_Isr);


	_mdrv_pcmcia_WriteReg(REG_PCMCIA_INT_MASK_CLEAR, 0x7C);

#endif
	pr_info("[%s][%d]\n", __func__, __LINE__);

}

unsigned int mdrv_pcmcia_set_power_state(EN_POWER_MODE power_state)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	switch (power_state) {
	case E_POWER_RESUME:
		_mdrv_pcmcia_InitHW(TRUE);
		break;
	case E_POWER_SUSPEND:
		_mdrv_pcmcia_Exit(TRUE);
		break;
	case E_POWER_MECHANICAL:
	case E_POWER_SOFT_OFF:
	default:
		break;
	}
	return UTOPIA_STATUS_SUCCESS;
}
unsigned char _mdrv_pcmcia_DetectV2(PCMCIA_MODULE module)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	unsigned char u8value = 0;

	unsigned char u8DetectPin =
		(module ==
		 E_PCMCIA_MODULE_A) ? PCMCIA_DETECT_PIN_MODULEA :
		PCMCIA_DETECT_PIN_MODULEB;

//	  if(module >= E_PCMCIA_MODULE_MAX)
//	  {
//		  ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n", (int)module);
//		  return FALSE;
//	  }

	if (!_gbPCMCIA_Detect_Enable)
		return FALSE;

	if (mtk_pcmcia_drv_stub) {
		set_stub_statue(STUB_STATE_CARD_DETECT,
						STUB_READ,
						REG_PCMCIA_READ_DATA_DONE_BUS_IDLE,
						&u8value,
						~u8DetectPin);
	} else
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA_DONE_BUS_IDLE, &u8value);
	//pr_info("[%s][%d] u8value=0x02%x\n", __func__, __LINE__, u8value);


	if ((u8value & u8DetectPin) != 0) {
		//pr_info("[%s][%d]return false\n", __func__, __LINE__);
		return FALSE;
	} else {
		//pr_info("[%s][%d]return true\n", __func__, __LINE__);
		return TRUE;
	}
}

unsigned char mdrv_pcmcia_polling(PCMCIA_MODULE module)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	unsigned char bCardDetect;
	unsigned char bModuleStatusChange = FALSE;

	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return FALSE;
	}

	bCardDetect = _mdrv_pcmcia_DetectV2(module);

	if (_gbCardInside[module] != bCardDetect)
		bModuleStatusChange = TRUE;

	_gbCardInside[module] = bCardDetect;

	if (bModuleStatusChange) {
		if (_gbCardInside[module]) {
			//_gu32PCMCIA_CD_To_HWRST_Timer[module] = MsOS_GetSystemTime();//mark nuused
			_mdrv_pcmcia_COMPANION_SetGPIO(module, TRUE);
			//ULOGE("PCMCIA", "[%s][%d]Card detected\n", __FUNCTION__,__LINE__);
		} else {
			_mdrv_pcmcia_COMPANION_SetGPIO(module, FALSE);
			//ULOGE("PCMCIA", "[%s][%d]Card removed\n", __FUNCTION__,__LINE__);
		}
	}

	return bModuleStatusChange;
}

void mdrv_pcmcia_reset_hw(PCMCIA_MODULE module)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	unsigned char u8Reg = 0, u8RawReg = 0;
	unsigned char u8ResetRetry = 0, u8DelayLoop = 0;

	pr_info("[%s][%d]\n", __func__, __LINE__);
	unsigned char bit = (module == E_PCMCIA_MODULE_A) ? BIT2 : BIT3;

	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return;
	}

	PCM_ENTER();

#if PCMCIA_IRQ_ENABLE
	//mdrv_pcmcia_Enable_Interrupt( DISABLE );
#endif
	_gbCardReady = FALSE;
	pr_info("[%s][%d] _gbCardReady=FALSE\n", __func__, __LINE__);
	for (u8ResetRetry = 0; u8ResetRetry < PCMCIA_RESET_RETRY_COUNT;
		 u8ResetRetry++) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_MODULE_VCC_OOB,
					 (unsigned char *) &u8Reg);
		u8Reg |= bit;	//1: RESET = HIGH


		_mdrv_pcmcia_WriteReg(REG_PCMCIA_MODULE_VCC_OOB, u8Reg);
		MsOS_DelayTask(_gu8HW_ResetDuration);	// MUST...for HW reset
		u8Reg &= ~bit;	//0: RESET = LOW

		_mdrv_pcmcia_WriteReg(REG_PCMCIA_MODULE_VCC_OOB, u8Reg);
		pr_info("[%s][%d] u8ResetRetry=%d\n", __func__, __LINE__,
			   u8ResetRetry);
		for (u8DelayLoop = 0; u8DelayLoop < PCMCIA_DELAY_LOOP_COUNT;
			 u8DelayLoop++) {
			MsOS_DelayTask(100);
			if (mtk_pcmcia_drv_stub) {
				set_stub_statue(STUB_STATE_CARD_RESET,
					STUB_READ,
					REG_PCMCIA_STAT_INT_RAW_INT,
					&u8RawReg,
					0x00);
			} else
				_mdrv_pcmcia_ReadReg(REG_PCMCIA_STAT_INT_RAW_INT,
						 (unsigned char *) &u8RawReg);
			pr_info("[%s][%d]u8RawReg=%x\n", __func__, __LINE__,
				   u8RawReg);
			if (!u8RawReg) {
				_gbCardReady = TRUE;
				pr_info
					("[%s][%d]_gbCardReady=TRUE u8DelayLoop=%d\n",
					 __func__, __LINE__, u8DelayLoop);
				goto HW_RESET_SUCCESS;
			}

		}
	}

HW_RESET_SUCCESS:

	PCM_EXIT();
}

void mdrv_pcmcia_write_attrib_mem(PCMCIA_MODULE module, unsigned short addr,
				   unsigned char value)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	unsigned char u8Reg = 0;
	unsigned short i;

	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return;
	}


	_mdrv_pcmcia_SwitchModule(module);

	// select attribute memory write, low byte
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_PCM_MEM_IO_CMD,
				  PCMCIA_ATTRIBMEMORY_WRITE);

	// write address
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR1, (addr >> 8));
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR0, addr);

	// write data
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_WRITE_DATA, value);

	// fire command
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
				  PCMCIA_FIRE_COMMAND);


	//polling if fire is done
	for (i = 0; i < PCMCIA_MAX_POLLING_COUNT; i++) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
					 (unsigned char *) &u8Reg);

		if (!(u8Reg & PCMCIA_FIRE_COMMAND))
			break;

	}

	// polling if bus is idle
	for (i = 0; i < PCMCIA_MAX_POLLING_COUNT; i++) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA_DONE_BUS_IDLE,
					 (unsigned char *) &u8Reg);

		if (u8Reg & PCMCIA_STATE_BUS_IDLE)
			break;

	}


}

void mdrv_pcmcia_read_attrib_mem(PCMCIA_MODULE module, unsigned short addr,
				  unsigned char *dest)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	unsigned short u16TryLoop = 0;
	unsigned char u8Reg = 0;

	//pr_info("[%s][%d]\n", __func__, __LINE__);
	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return;
	}
	// CIS readout with 8Bit I/O accesses
	// requires that we read only every second
	// byte. (The result of reading the even addresses does not seem to work on most modules)

	_mdrv_pcmcia_SwitchModule(module);

	// select attribute memory read, low byte
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_PCM_MEM_IO_CMD,
				  PCMCIA_ATTRIBMEMORY_READ);

	// read address
	if (mtk_pcmcia_drv_stub) {
		unsigned char write_value;

		write_value = (addr >> SHIFT_FACTOR);
		set_stub_statue(STUB_STATE_READ_CIS,
						STUB_WRITE,
						REG_PCMCIA_ADDR1,
						&write_value,
						NULL);

		write_value = addr;
		set_stub_statue(STUB_STATE_READ_CIS,
						STUB_WRITE,
						REG_PCMCIA_ADDR0,
						&write_value,
						NULL);
	} else {
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR1,
			(unsigned char) ((addr * SCALE_FACTOR) >> SHIFT_FACTOR));
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR0, (unsigned char) (addr * SCALE_FACTOR));
	}

	// fire command
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
				  PCMCIA_FIRE_COMMAND);

	//polling if fire is done
	while (1) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
					 (unsigned char *) &u8Reg);

		if (!(u8Reg & PCMCIA_FIRE_COMMAND))
			break;


		u16TryLoop++;
		if (u16TryLoop > PCMCIA_HW_MAX_RETRY_COUNT) {
			u16TryLoop = 0;
			ULOGE("PCMCIA",
				  "[%s:%d][Warning!][PCMCIA] Timeout!\n",
				  __FILE__, __LINE__);

			return;
		}

	}

	//polling if data ready
	while (1) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA_DONE_BUS_IDLE,
					 (unsigned char *) &u8Reg);

		if (u8Reg & PCMCIA_STATE_RD_DONE) {
			if (mtk_pcmcia_drv_stub) {
				set_stub_statue(STUB_STATE_READ_CIS,
								STUB_READ,
								REG_PCMCIA_READ_DATA,
								dest,
								NULL);
			} else
				_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA, dest);
			break;
		}

		u16TryLoop++;
		if (u16TryLoop > PCMCIA_HW_MAX_RETRY_COUNT) {
			u16TryLoop = 0;
			ULOGE("PCMCIA",
				  "[%s:%d][Warning!][PCMCIA] Timeout!\n",
				  __FILE__, __LINE__);

			return;
		}

	}

	// clean stat_rd done
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
				  PCMCIA_CLEAN_STATE_RD_DONE);

	// polling if bus is idle
	while (1) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA_DONE_BUS_IDLE,
					 (unsigned char *) &u8Reg);

		if (PCMCIA_STATE_BUS_IDLE ==
			(u8Reg & (PCMCIA_STATE_BUS_IDLE | PCMCIA_STATE_RD_DONE))) {
			break;
		}

		u16TryLoop++;
		if (u16TryLoop > PCMCIA_HW_MAX_RETRY_COUNT) {
			u16TryLoop = 0;
			ULOGE("PCMCIA",
				  "[%s:%d][Warning!][PCMCIA] Timeout!\n",
				  __FILE__, __LINE__);

			return;
		}

	}

	//pr_info("Read Type %bx, Addr %x, value %bx\n", u8AccessType, Addr, u8mem);
}

void mdrv_pcmcia_write_io_mem(PCMCIA_MODULE module, unsigned short addr,
				   unsigned char value)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	unsigned char u8Reg = 0;
	unsigned short i;

	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return;
	}

	_mdrv_pcmcia_SwitchModule(module);

	// select attribute memory write, low byte
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_PCM_MEM_IO_CMD, PCMCIA_IO_WRITE);

	if (mtk_pcmcia_drv_stub) {
		unsigned char write_value;

		write_value = (addr >> SHIFT_FACTOR);
		set_stub_statue(STUB_STATE_WRITE_IO_MEM,
						STUB_WRITE,
						REG_PCMCIA_ADDR1,
						&write_value,
						NULL);

		write_value = addr;
		set_stub_statue(STUB_STATE_WRITE_IO_MEM,
						STUB_WRITE,
						REG_PCMCIA_ADDR0,
						&write_value,
						NULL);

		write_value = value;
		set_stub_statue(STUB_STATE_WRITE_IO_MEM,
						STUB_WRITE,
						REG_PCMCIA_WRITE_DATA,
						&write_value,
						NULL);

	} else {
		// write address
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR1, (addr >> SHIFT_FACTOR));
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR0, addr);
		// write data
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_WRITE_DATA, value);
	}
	// fire command
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
				  PCMCIA_FIRE_COMMAND);


	//polling if fire is done
	for (i = 0; i < PCMCIA_MAX_POLLING_COUNT; i++) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
					 (unsigned char *) &u8Reg);

		if (!(u8Reg & PCMCIA_FIRE_COMMAND))
			break;


		if (!mdrv_pcmcia_is_module_still_plugged(module))
			return;


	}

	// polling if bus is idle
	for (i = 0; i < PCMCIA_MAX_POLLING_COUNT; i++) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA_DONE_BUS_IDLE,
					 (unsigned char *) &u8Reg);

		if (u8Reg & PCMCIA_STATE_BUS_IDLE)
			break;


		if (!mdrv_pcmcia_is_module_still_plugged(module))
			return;


	}
}
#if (!PCMCIA_MSPI_SUPPORTED)
void mdrv_pcmcia_write_io_mem_long(PCMCIA_MODULE module, unsigned short addr,
				   unsigned short data_len, unsigned char *write_buffer)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);

	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return;
	}
	_mdrv_pcmcia_SwitchModule(module);

#if PCMCIA_PERF
	unsigned int u32Time1, u32Time2;

	u32Time1 = MsOS_GetSystemTime();
#endif

#if (PCMCIA_MSPI_BURST)
	_MSPI_RWLong(PCMCIA_IO_WRITE, addr, write_buffer, data_len);
#else
	unsigned char u8Reg = 0;
	unsigned short i, j;

	// select attribute memory write, low byte
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_PCM_MEM_IO_CMD, PCMCIA_IO_WRITE);

	// write address
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR1, (addr >> 8));
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR0, addr);

	for (i = 0; i < data_len; i++) {
		// write data
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_WRITE_DATA, write_buffer[i]);

		// fire command
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
					  PCMCIA_FIRE_COMMAND);

		//polling if fire is done
		for (j = 0; j < PCMCIA_MAX_POLLING_COUNT; ++j) {
			_mdrv_pcmcia_ReadReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
						 (unsigned char *) &u8Reg);

			if (!(u8Reg & PCMCIA_FIRE_COMMAND))
				break;


			if (!mdrv_pcmcia_is_module_still_plugged(module))
				return;


		}

		// polling if bus is idle
		for (j = 0; j < PCMCIA_MAX_POLLING_COUNT; ++j) {
			_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA_DONE_BUS_IDLE,
						 (unsigned char *) &u8Reg);

			if (u8Reg & PCMCIA_STATE_BUS_IDLE)
				break;


			if (!mdrv_pcmcia_is_module_still_plugged(module))
				return;


		}
	}
#endif

#if PCMCIA_PERF
	u32Time2 = MsOS_GetSystemTime();
	if (data_len > 10) {
		pr_info("\n\n[%s]: Len(%d), Diff(%d ms)\n\n",
			   __func__, data_len, (int)(u32Time2 - u32Time1));
	}
#endif
}
#endif
//! This function is read one byte of from the card IO memory at address addr.
unsigned char mdrv_pcmcia_read_io_mem(PCMCIA_MODULE module, unsigned short addr)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	unsigned char u8Reg = 0;
	unsigned char value = 0;
	unsigned short i;

	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return 0;
	}
	_mdrv_pcmcia_SwitchModule(module);

	// select attribute memory read, low byte
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_PCM_MEM_IO_CMD, PCMCIA_IO_READ);

	// read address
	if (mtk_pcmcia_drv_stub) {
		unsigned char write_value;

		write_value = (addr >> SHIFT_FACTOR);
		set_stub_statue(STUB_STATE_READ_IO_MEM,
						STUB_WRITE,
						REG_PCMCIA_ADDR1,
						&write_value,
						NULL);

		write_value = addr;
		set_stub_statue(STUB_STATE_READ_IO_MEM,
						STUB_WRITE,
						REG_PCMCIA_ADDR0,
						&write_value,
						NULL);
	} else {
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR1, (addr >> SHIFT_FACTOR));
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_ADDR0, addr);
	}

	// fire command
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
				  PCMCIA_FIRE_COMMAND);

	//polling if fire is done
	for (i = 0; i < PCMCIA_MAX_POLLING_COUNT; i++) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
					 (unsigned char *) &u8Reg);

		if (!(u8Reg & PCMCIA_FIRE_COMMAND))
			break;


		if (!mdrv_pcmcia_is_module_still_plugged(module))
			return 0x00;


	}

	//polling if data ready
	for (i = 0; i < PCMCIA_MAX_POLLING_COUNT; i++) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA_DONE_BUS_IDLE,
					 (unsigned char *) &u8Reg);

		if (u8Reg & PCMCIA_STATE_RD_DONE) {
			if (mtk_pcmcia_drv_stub) {
				set_stub_statue(STUB_STATE_READ_IO_MEM,
								STUB_READ,
								REG_PCMCIA_READ_DATA,
								&value,
								NULL);
			} else {
				_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA,
							 (unsigned char *) &value);
			}

			break;
		}

		if (!mdrv_pcmcia_is_module_still_plugged(module))
			return 0x00;


	}

	// clean stat_rd done
	_mdrv_pcmcia_WriteReg(REG_PCMCIA_FIRE_READ_DATA_CLEAR,
				  PCMCIA_CLEAN_STATE_RD_DONE);

	// polling if bus is idle
	for (i = 0; i < PCMCIA_MAX_POLLING_COUNT; i++) {
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_READ_DATA_DONE_BUS_IDLE,
					 (unsigned char *) &u8Reg);

		if (PCMCIA_STATE_BUS_IDLE ==
			(u8Reg & (PCMCIA_STATE_BUS_IDLE | PCMCIA_STATE_RD_DONE))) {
			break;
		}

		if (!mdrv_pcmcia_is_module_still_plugged(module))
			return 0x00;


	}

	//pr_info("Read Addr %x, value %bx\n", addr, u8mem);

	return value;
}

unsigned char mdrv_pcmcia_is_module_still_plugged(PCMCIA_MODULE module)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return FALSE;
	}

	return (_gbCardInside[module]);
}
#if (!PCMCIA_MSPI_SUPPORTED)
unsigned short mdrv_pcmcia_read_data(PCMCIA_MODULE module, unsigned char *read_buffer,
				   unsigned short read_buffer_size)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return FALSE;
	}

	unsigned short data_len = 0;

	data_len =
		(unsigned short) mdrv_pcmcia_read_io_mem(module,
						  PCMCIA_PHYS_REG_SIZEHIGH) << 8 |
		(unsigned short) mdrv_pcmcia_read_io_mem(module, PCMCIA_PHYS_REG_SIZELOW);

	if ((read_buffer_size != 0) & (data_len > read_buffer_size))
		data_len = read_buffer_size;

#if PCMCIA_PERF
	unsigned int u32Time1, u32Time2;

	u32Time1 = MsOS_GetSystemTime();
#endif

#if (PCMCIA_MSPI_BURST)
	_MSPI_RWLong(PCMCIA_IO_READ, PCMCIA_PHYS_REG_DATA, read_buffer,
			 data_len);
#else
	unsigned short i = 0;

	for (i = 0; i < data_len; i++) {
		read_buffer[i] =
			mdrv_pcmcia_read_io_mem(module, PCMCIA_PHYS_REG_DATA);
	}
#endif

#if PCMCIA_PERF
	u32Time2 = MsOS_GetSystemTime();
	if (data_len > 10) {
		pr_info("\n\n[%s]: Len(%d), Diff(%d ms)\n\n",
			   __func__, data_len, (int)(u32Time2 - u32Time1));
	}
#endif

	return data_len;
}
#endif
unsigned char mdrv_pcmcia_ready_status(void)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	if (_gbCardReady) {
		//pr_info("[%s][%d] _gbCardReady=TRUE\n", __func__, __LINE__);
		return TRUE;
	}

	//pr_info("[%s][%d] _gbCardReady=FALSE\n", __func__, __LINE__);
	return FALSE;

}

#if PCMCIA_IRQ_ENABLE
void _mdrv_pcmcia_Enable_InterruptV2(PCMCIA_MODULE module, unsigned char bEnable)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	unsigned char u8Reg;

	pr_info("[%s][%d]\n", __func__, __LINE__);
	if (module >= E_PCMCIA_MODULE_MAX) {
		ULOGE("PCMCIA", "ERROR: Module 0x%x not support\n",
			  (int)module);
		return;
	}

	_mdrv_pcmcia_Set_InterruptStatusV2(module, FALSE);

	if (bEnable == true) {
		_gbPCMCIA_Irq[module] = true;
		pr_info("ci irq = %x\n", ci_interrupt_irq);
		/* Enable MPU PCMCIA IRQ. */
		//MsOS_EnableInterrupt( ci_interrupt_irq );


		/* Enable IP PCMCIA IRQ. */
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_INT_MASK_CLEAR,
					 (unsigned char *) &u8Reg);
		if (module == E_PCMCIA_MODULE_A) {
			u8Reg &= (~BIT2);
			u8Reg &= (~BIT1);
			u8Reg &= (~BIT0);
		} else {	// Module B
			u8Reg &= (~BIT5);
			u8Reg &= (~BIT4);
			u8Reg &= (~BIT3);
		}
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_INT_MASK_CLEAR, u8Reg);
	} else {
		_gbPCMCIA_Irq[module] = false;

		/* Here DON"T Disable MPU PCMCIA IRQ. */
		/* Disable IP PCMCIA IRQ. */
		_mdrv_pcmcia_ReadReg(REG_PCMCIA_INT_MASK_CLEAR,
					 (unsigned char *) &u8Reg);
		if (module == E_PCMCIA_MODULE_A) {
			u8Reg |= BIT2;	//Don't mask cardA insert/remove
		} else {	// Module B
			u8Reg |= BIT5;
		}
		_mdrv_pcmcia_WriteReg(REG_PCMCIA_INT_MASK_CLEAR, u8Reg);
	}
}

void _mdrv_pcmcia_Set_InterruptStatusV2(PCMCIA_MODULE module, unsigned char Status)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	_gbPCMCIA_IrqStatus[module] = Status;
}

unsigned char _mdrv_pcmcia_Get_InterruptStatusV2(PCMCIA_MODULE module)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	return _gbPCMCIA_IrqStatus[module];
}

void _mdrv_pcmcia_InstarllIsrCallbackV2(PCMCIA_MODULE module,
					IsrCallback fnIsrCallback)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	_fnIsrCallback[module] = fnIsrCallback;
}

#if defined(MSOS_TYPE_LINUX_KERNEL) && defined(PCMCIA_IRQ_ENABLE)
unsigned char _mdrv_pcmcia_IsrStatus(PCMCIA_MODULE *module, PCMCIA_ISR *pStIsr)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	return TRUE;
}
#endif
#endif

static void _mdrv_pcmcia_COMPANION_SetGPIO(PCMCIA_MODULE module, unsigned char bHigh)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
}

// backward compatible

#if PCMCIA_IRQ_ENABLE

void mdrv_pcmcia_Enable_InterruptV2(PCMCIA_MODULE module, unsigned char bEnable)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	_mdrv_pcmcia_Enable_InterruptV2(module, bEnable);
}

void mdrv_pcmcia_Set_InterruptStatus(unsigned char Status)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	_mdrv_pcmcia_Set_InterruptStatusV2(PCMCIA_DEFAULT_MODULE, Status);
}

unsigned char mdrv_pcmcia_Get_InterruptStatus(void)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	return _mdrv_pcmcia_Get_InterruptStatusV2(PCMCIA_DEFAULT_MODULE);
}

void mdrv_pcmcia_InstarllIsrCallback(IsrCallback fnIsrCallback)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	_mdrv_pcmcia_InstarllIsrCallbackV2(PCMCIA_DEFAULT_MODULE,
					   fnIsrCallback);
}

#endif				// PCMCIA_IRQ_ENABLE

unsigned char _bInited_Drv = FALSE;
void mdrv_pcmcia_init_hw(void)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	ULOGE("PCMCIA", "[%s][%d]\n", __func__, __LINE__);
	//_mdrv_pcmcia_InitSW(bCD_Reverse); change to .prob
	if (_bInited_Drv == FALSE) {
		_mdrv_pcmcia_InitHW(FALSE);
		_bInited_Drv = TRUE;
	}
}

void mdrv_pcmcia_exit(void)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	_mdrv_pcmcia_Exit(FALSE);
	_bInited_Drv = FALSE;
}

