/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __DRV_SC_H__
#define __DRV_SC_H__


#ifdef __cplusplus
extern "C" {
#endif

#define MS_BOOL u8
#define MS_U32  u32
#define MS_U8   u8
#define MS_U16  u16
#define MS_S16  s16
#define MS_S32  s32
#define MS_PHY  u64
#define MS_U64  u64
#define MS_VIRT size_t
#define MS_S8   s8
#define FALSE   0
#define TRUE    1
#define BOOL    u8
#define U8    u8
#define U32     u32
#define U16     u16
#define DLL_PACKED
#define MSOS_TYPE_LINUX_KERNEL

//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define MSIF_SC_LIB_CODE            {'S', 'C'}	//Lib code
#define MSIF_SC_LIBVER              {'0', '1'}	//LIB version
#define MSIF_SC_BUILDNUM            {'0', '1'}	//Build Number
#define MSIF_SC_CHANGELIST          {'0', '0', '0', '0', '0', '0', '0', '0'}

#define SC_ATR_LEN_MAX              33	///< Maximum length of ATR
#define SC_ATR_LEN_MIN              2	///< Minimum length of ATR
#define SC_HIST_LEN_MAX             15	///< Maximum length of ATR history
#define SC_PPS_LEN_MAX              6	///< Maximum length of PPS
#define SC_FIFO_SIZE                512	// Rx fifo size

///SC_Param.u8UartMode
#define SC_UART_CHAR_7              (0x02)
#define SC_UART_CHAR_8              (0x03)
#define SC_UART_STOP_1              (0x00)
#define SC_UART_STOP_2              (0x04)
#define SC_UART_PARITY_NO           (0x00)
#define SC_UART_PARITY_ODD          (0x08)
#define SC_UART_PARITY_EVEN         (0x08|0x10)

#define SC_PROC_LOCK                (0xFE66)
#define SC_PROC_UNLOCK              (0x0)

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------

	typedef struct _SC_Status {
		MS_BOOL bCardIn;	///< Card status

	} SC_Status;

	typedef enum _SC_DbgLv {
		E_SC_DBGLV_NONE,	//no debug message
		E_SC_DBGLV_ERR_ONLY,	//show error only
		E_SC_DBGLV_REG_DUMP,	//show error & reg dump
		E_SC_DBGLV_INFO,	//show error & informaiton
		E_SC_DBGLV_ALL,	//show error, information & funciton name
	} SC_DbgLv;

/// SmartCard DDI error code
	typedef enum {
		E_SC_FAIL,	///< Function fail
		E_SC_OK,	///< No Error
		E_SC_PPSFAIL,	///< Do PPS fail
		E_SC_INVCONV,	///< Inverse convention
		E_SC_CARDIN,	///< Card in
		E_SC_CARDOUT,	///< Card out
		E_SC_NODATA,	///< No data
		E_SC_TIMEOUT,	///< Timeout
		E_SC_OVERFLOW,	///< Rx data fifo overflow
		E_SC_CRCERROR,	///< ATR checksum error
		E_SC_DATAERROR,	///< Data error
		E_SC_PARMERROR,	///< Parameter error
	} SC_Result;

/// SmartCard event
	typedef enum {
		E_SC_EVENT_DATA,	///< Rx data valid
		E_SC_EVENT_IN,	///< Card in
		E_SC_EVENT_OUT,	///< Card out

	} SC_Event;

/// SmartCard CLK setting
	typedef enum {
		E_SC_CLK_3M,	///< 3 MHz
		E_SC_CLK_4P5M,	///< 4.5 MHz
		E_SC_CLK_6M,	///< 6 MHz
		E_SC_CLK_13M,	///< 6 MHz
		E_SC_CLK_4M,	///< 4 MHz
		E_SC_CLK_TSIO_MODE	///< TSIO mode
	} SC_ClkCtrl;

/// SmartCard VCC control mode
	typedef enum {
		E_SC_VCC_CTRL_8024_ON,	///< by external 8024 on
		E_SC_VCC_CTRL_LOW,	///< by direct Vcc (low active)
		E_SC_VCC_CTRL_HIGH,	///< by direct Vcc (high active)
		E_SC_OCP_VCC_HIGH,
		E_SC_VCC_VCC_ONCHIP_8024,
	} SC_VccCtrl;

// smart card 3V/5V control
	typedef enum {
		E_SC_VOLTAGE_3_POINT_3V,	///<3.3V
		E_SC_VOLTAGE_5V,	///< 5V
		E_SC_VOLTAGE_3V,
		E_SC_VOLTAGE_1P8V,
		E_SC_VOLTAGE_MAX
	} SC_VoltageCtrl;

	typedef void (*P_SC_En5V_Callback) (MS_BOOL bEnable);


	typedef enum {
		E_SC_HIGH_ACTIVE,
		E_SC_LOW_ACTIVE
	} SC_CardDetType;

	typedef enum {
		E_SC_TSIO_DISABLE,
		E_SC_TSIO_BGA,
		E_SC_TSIO_CARD
	} SC_TsioCtrl;
/// SmartCard Timeout config
	typedef struct DLL_PACKED {
		MS_U32 u32T0SendTimeoutMs;
		MS_U32 u32T0RecvTimeoutMs;
		MS_U32 u32T0WaitProByteMs;	// Wait procedure byte timeout ms
		MS_U32 u32T1SendTimeoutMs;
		MS_U32 u32T1RecvTimeoutMs;
		MS_U32 u32T14SendTimeoutMs;
		MS_U32 u32T14RecvTimeoutMs;
	} SC_TimeoutCfg;

/// SmartCard configuration
	typedef struct DLL_PACKED {
		MS_U8 u8Protocal;	///< T=
		SC_ClkCtrl eCardClk;	///< Clock
		MS_U8 u8UartMode;	///< Uart Mode
		MS_U16 u16ClkDiv;	///< Div
		SC_VccCtrl eVccCtrl;
		MS_U16 u16Bonding;	///Chip Bonding type
		P_SC_En5V_Callback pfOCPControl;
		MS_U8 u8Convention;	///< Convention
		SC_VoltageCtrl eVoltage;
		SC_CardDetType u8CardDetInvert;
		SC_TimeoutCfg TimeoutCfg;
		SC_TsioCtrl eTsioCtrl;
	} SC_Param;

/// SmartCard event callback
	typedef void (*P_SC_Callback) (MS_U8 u8SCID, SC_Event eEvent);

	typedef struct _Smart_Dev {

	} SC_Dev;


/// SmartCard Info
	typedef struct DLL_PACKED {
		// SmsartCard Protocol
		MS_U8 u8Protocol;	///T= Protocol
		MS_BOOL bSpecMode;	///Special mode
		MS_U8 pu8Atr[SC_ATR_LEN_MAX];	///Atr buffer
		MS_U16 u16AtrLen;	///Atr length
		MS_U8 pu8Hist[SC_HIST_LEN_MAX];	///History buffer
		MS_U16 u16HistLen;	///History length
		MS_U8 u8Fi;	///Fi
		MS_U8 u8Di;	///Di
#ifdef UFO_PUBLIC_HEADER_212
		MS_U8 u8N;	////N
#endif
		// Device Setting
		MS_BOOL bInited;
		MS_BOOL bOpened;	///Open
		MS_BOOL bCardIn;	///Status care in
		MS_BOOL blast_cardin;
		SC_ClkCtrl eCardClk;	///< Clock
		MS_U8 u8UartMode;	///< Uart Mode
		SC_VccCtrl eVccCtrl;
		MS_U16 u16ClkDiv;	///< Div
		MS_U16 u16Bonding;	//@TODO: how to take care of bonding?????
		P_SC_En5V_Callback pfEn5V;
		MS_U8 u8Convention;

		MS_U8 u8FifoRx[SC_FIFO_SIZE];
		MS_U16 u16FifoRxRead;
		MS_U16 u16FifoRxWrite;

		MS_U8 u8FifoTx[SC_FIFO_SIZE];
		MS_U16 u16FifoTxRead;
		MS_U16 u16FifoTxWrite;
		P_SC_Callback pfNotify;	///Call back funtcion

		MS_BOOL bLastCardIn;
		MS_S32 s32DevFd;
#ifdef UFO_PUBLIC_HEADER_212
		SC_VoltageCtrl eVoltage;
#endif
		SC_TimeoutCfg TimeoutCfg;
		SC_TsioCtrl eTsioCtrl;
	} SC_Info;


/// SmartCard Caps
	typedef struct DLL_PACKED {
		MS_U8 u8DevNum;	///SmartCard Device Number

	} SC_Caps;

///Define SC Command Index
	typedef enum {
		//MIPS-->51 Command Index
		SC_CMDIDX_RAW_EXCHANGE = 0x01,	/// SC Command Index is Raw Data Exchange
		SC_CMDIDX_GET_ATR = 0x02,	/// SC Command Index is Get ATR
		SC_CMDIDX_SEND = 0x03,	/// SC Command Index is Send Data
		SC_CMDIDX_RECV = 0x04,	/// SC Command Index is Receive Data

		SC_CMDIDX_ACK_51ToMIPS = 0x30,	/// SC Command Index is ACK 51 To MIPS

		//51->MIPS Command Index
		SC_CMDIDX_ACK_MIPSTo51 = 0xA0,	/// SC Command Index is ACK MIPS To 51

	} SC_CmdIndex;


///Define SC Acknowledge Flags
	typedef enum {
		E_SC_ACKFLG_NULL = 0,	/// Ack flag for NULL
		E_SC_ACKFLG_WAIT_RAW_EXCHANGE = (1 << 0),	/// Ack flag for Raw Exchange
		E_SC_ACKFLG_WAIT_GET_ATR = (1 << 1),	/// Ack flag for ATR
		E_SC_ACKFLG_WAIT_SEND = (1 << 2),	/// Ack flag for Sending
		E_SC_ACKFLG_WAIT_RECV = (1 << 3),	/// Ack flag for Receiving

	} SC_AckFlags;

/// SmartCard Caps
	typedef struct {
		MS_PHY u32DataBuffAddr;	///SmartCard Data Buffer Address, 4K alignment
		MS_PHY u32FwBuffAddr;	///SmartCard Firmware Buffer Address, 64K alignment
	} SC_BuffAddr;

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_INIT
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Init(MS_U8 u8SCID);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Open(MS_U8 u8SCID, MS_U8 u8Protocol, SC_Param *pParam,
			       P_SC_Callback pfSmartNotify);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Config(MS_U8 u8SCID, SC_Param *pParam);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Close(MS_U8 u8SCID);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Reset(MS_U8 u8SCID, SC_Param *pParam);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Activate(MS_U8 u8SCID);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Deactivate(MS_U8 u8SCID);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Reset_ATR(MS_U8 u8SCID, SC_Param *pParam, MS_U8 *pu8Atr,
				    MS_U16 *pu16AtrLen, MS_U8 *pu8His, MS_U16 *pu16HisLen);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_PPS(MS_U8 u8SCID);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Send(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 u16SendDataLen,
			       MS_U32 u32TimeoutMs);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Recv(MS_U8 u8SCID, MS_U8 *pu8ReadData, MS_U16 *u16ReadDataLen,
			       MS_U32 u32TimeoutMs);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_T0_SendRecv(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 u16SendLen,
				      MS_U8 *pu8RecvData, MS_U16 *pu16RecvLen);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_T1_SendRecv(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 *u16SendDataLen,
				      MS_U8 *pu8ReadData, MS_U16 *u16ReadDataLen);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_T14_SendRecv(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 u16SendLen,
				       MS_U8 *pu8RecvData, MS_U16 *pu16RecvLen);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Exit(MS_U8 u8SCID);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_GetATR(MS_U8 u8SCID, MS_U32 u32TimeOut, MS_U8 *pu8Atr,
				 MS_U16 *pu16AtrLen, MS_U8 *pu8His, MS_U16 *pu16HisLen);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_COMMON
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	const SC_Info *MDrv_SC_GetInfo(MS_U8 u8SCID);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_COMMON
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
//SC_Result       MDrv_SC_GetLibVer(const MSIF_Version **ppVersion); //for STI
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_COMMON
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_GetStatus(MS_U8 u8SCID, SC_Status *pStatus);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_COMMON
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	void MDrv_SC_SetDbgLevel(SC_DbgLv eLevel);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_SetPPS(MS_U8 u8SCID, MS_U8 u8SCProtocol, MS_U8 u8Di, MS_U8 u8Fi);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_COMMON
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_ClearState(MS_U8 u8SCID);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_PowerOff(void);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_CONTROL
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_SetGuardTime(MS_U8 u8SCID, MS_U8 u8GuardTime);

//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_Task_Proc(void);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	MS_BOOL MDrv_SC_ISR_Proc(MS_U8 u8SCID);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_RawExchange(MS_U8 u8SCID, MS_U8 *pu8SendData, MS_U16 *u16SendDataLen,
				      MS_U8 *pu8ReadData, MS_U16 *u16ReadDataLen);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_RawExchangeTimeout(MS_U8 u8SCID, MS_U8 *pu8SendData,
					     MS_U16 *u16SendDataLen, MS_U8 *pu8ReadData,
					     MS_U16 *u16ReadDataLen, MS_U32 u32TimeoutMs);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	SC_Result MDrv_SC_SetBuffAddr(SC_BuffAddr *pScBuffAddr);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	MS_BOOL MDrv_SC_CardVoltage_Config(MS_U8 u8SCID, SC_VoltageCtrl eVoltage);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_ToBeRemove
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
	void MDrv_SC_EnableTimeout(MS_BOOL bTimeout);
//-------------------------------------------------------------------------------------------------
/// MOBF Encrypt
/// @ingroup G_SC_COMMON
/// @param u32Key \b IN: Key
/// @param bEnable \b IN: TRUE/FLASE
/// @return DRVAESDMA_OK : Success
/// @return Others : Fail
//-------------------------------------------------------------------------------------------------
// for sti
	typedef enum {
		E_POWER_SUSPEND = 1,
		E_POWER_RESUME = 2,
		E_POWER_MECHANICAL = 3,
		E_POWER_SOFT_OFF = 4,
	} EN_POWER_MODE;

	MS_U32 MDrv_SC_SetPowerState(EN_POWER_MODE u16PowerState);

	//void mdrv_sc_init_sw(void __iomem *riu_vaddr, void __iomem *ext_riu_vaddr);	//for STI
	void mdrv_sc_init_sw(void __iomem *riu_vaddr);
	void mdrv_sc_set_rf_pop_delay(int rf_pop_delay);

#ifdef __cplusplus
}
#endif

#endif				// __DRV_SC_H__
