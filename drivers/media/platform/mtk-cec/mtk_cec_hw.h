/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/platform_device.h>
#include <linux/io.h>

#include "mtk_cec.h"


#define BMASK(_bits_)		(BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))
#define RBYTE(reg)		(readb_relaxed(reg))
#define R2BYTE(reg)	(readw_relaxed(reg))

#define WBYTE(reg, val)	writeb_relaxed(val, reg)
#define W2BYTE(reg, val) writew_relaxed(val, reg)
#define W2BYTEMSK(reg, val, mask)\
			({ writew_relaxed((readw_relaxed(reg) & ~(mask)) \
				| ((val) & (mask)), reg); })


#define CEC_VERSION_11      0UL   //CEC1.1
#define CEC_VERSION_12      1UL   //CEC1.2
#define CEC_VERSION_12a     2UL   //CEC1.2a
#define CEC_VERSION_13      3UL   //CEC1.3
#define CEC_VERSION_13a     4UL   //CEC1.3a
#define CEC_VERSION_14      5UL   //CEC1.4

#define HDMI_CEC_VERSION    CEC_VERSION_14

typedef enum _MsCEC_MSG_ABORT_REASON {
	E_MSG_AR_UNRECOGNIZE_OPCDE  = 0x00,  // abort reason
	E_MSG_AR_CANNOTRESPOND      = 0x01,  // abort reason
	E_MSG_AR_CANNOTPROVIDESCR   = 0x02,  // abort reason
	E_MSG_AR_INVALID_OPCODE     = 0x03,  // abort reason
	E_MSG_AR_REFUSED            = 0x04,  // abort reason
	E_MSG_AR_IGNORED            = 0x05,
	E_MSG_AR_SUCCEEDED          = 0x06,
} MsCEC_MSG_ABORT_REASON;

//the opcode is referenced from CEC1.3a table 7 ~ 27
typedef enum _CEC_MSGLIST {
//----- One Touch Play ----------------------------
	E_MSG_ACTIVE_SOURCE                         = 0x82,
	E_MSG_OTP_IMAGE_VIEW_ON                     = 0x04,
	E_MSG_OTP_TEXT_VIEW_ON                      = 0x0D,


//----- Routing Control ---------------------------
	//E_MSG_RC_ACTIVE_SOURCE                      = 0x82,
	E_MSG_RC_INACTIVE_SOURCE                    = 0x9D,

	E_MSG_RC_REQ_ACTIVE_SOURCE                  = 0x85, // should be removed
	E_MSG_RC_REQUEST_ACTIVE_SOURCE              = 0x85,

	E_MSG_RC_ROUTING_CHANGE                     = 0x80,

	E_MSG_RC_ROUTING_INFO                       = 0x81, // should be removed
	E_MSG_RC_ROUTING_INFORMATION                = 0x81,

	E_MSG_RC_SET_STREM_PATH                     = 0x86,


//----- Standby Command ---------------------------
	E_MSG_STANDBY                               = 0x36,

//----- One Touch Record---------------------------
	E_MSG_OTR_RECORD_OFF                        = 0x0B,
	E_MSG_OTR_RECORD_ON                         = 0x09,
	E_MSG_OTR_RECORD_STATUS                     = 0x0A,
	E_MSG_OTR_RECORD_TV_SCREEN                  = 0x0F,

//----- Timer programmer -------------------------- CEC1.3a
	E_MSG_TP_CLEAR_ANALOG_TIMER                 = 0x33, // should be removed
	E_MSG_TP_CLEAR_ANALOGUE_TIMER               = 0x33,

	E_MSG_TP_CLEAR_DIGITAL_TIMER                = 0x99,

	E_MSG_TP_CLEAR_EXT_TIMER                    = 0xA1, // should be removed
	E_MSG_TP_CLEAR_EXTERNAL_TIMER               = 0xA1,

	E_MSG_TP_SET_ANALOG_TIMER                   = 0x34, // should be removed
	E_MSG_TP_SET_ANALOGUE_TIMER                 = 0x34,

	E_MSG_TP_SET_DIGITAL_TIMER                  = 0x97,

	E_MSG_TP_SET_EXT_TIMER                      = 0xA2, // should be removed
	E_MSG_TP_SET_EXTERNAL_TIMER                 = 0xA2,

	E_MSG_TP_SET_TIMER_PROGRAM_TITLE            = 0x67,
	E_MSG_TP_TIMER_CLEARD_STATUS                = 0x43,
	E_MSG_TP_TIMER_STATUS                       = 0x35,

//----- System Information ------------------------
	E_MSG_SI_CEC_VERSION                        = 0x9E,       //1.3a
	E_MSG_SI_GET_CEC_VERSION                    = 0x9F,       //1.3a

	E_MSG_SI_REQUEST_PHY_ADDR                   = 0x83, // should be removed
	E_MSG_SI_GIVE_PHYSICAL_ADDRESS              = 0x83,

	E_MSG_SI_GET_MENU_LANGUAGE                  = 0x91,
	//E_MSG_SI_POLLING_MESSAGE                    = ?,

	E_MSG_SI_REPORT_PHY_ADDR                    = 0x84, // should be removed
	E_MSG_SI_REPORT_PHYSICAL_ADDRESS            = 0x84,

	E_MSG_SI_SET_MENU_LANGUAGE                  = 0x32,

	//E_MSG_SI_REC_TYPE_PRESET                    = 0x00,  //parameter   ?
	//E_MSG_SI_REC_TYPE_OWNSRC                    = 0x01,  //parameter   ?

//----- Deck Control Feature-----------------------
	E_MSG_DC_DECK_CTRL                          = 0x42, // should be removed
	E_MSG_DC_DECK_CONTROL                       = 0x42,

	E_MSG_DC_DECK_STATUS                        = 0x1B,
	E_MSG_DC_GIVE_DECK_STATUS                   = 0x1A,
	E_MSG_DC_PLAY                               = 0x41,

//----- Tuner Control ------------------------------
	E_MSG_TC_GIVE_TUNER_STATUS                  = 0x08, // should be removed
	E_MSG_TC_GIVE_TUNER_DEVICE_STATUS           = 0x08,

	E_MSG_TC_SEL_ANALOG_SERVICE                 = 0x92, // should be removed
	E_MSG_TC_SEL_ANALOGUE_SERVICE               = 0x92,

	E_MSG_TC_SEL_DIGITAL_SERVICE                = 0x93, // should be removed
	E_MSG_TC_SELECT_DIGITAL_SERVICE             = 0x93,

	E_MSG_TC_TUNER_DEVICE_STATUS                = 0x07,

	E_MSG_TC_TUNER_STEP_DEC                     = 0x06, // should be removed
	E_MSG_TC_TUNER_STEP_DECREMENT               = 0x06,

	E_MSG_TC_TUNER_STEP_INC                     = 0x05, // should be removed
	E_MSG_TC_TUNER_STEP_INCREMENT               = 0x05,

//---------Vendor Specific -------------------------
	//E_MSG_VS_CEC_VERSION                        = 0x9E,       //1.3a
	E_MSG_VS_DEVICE_VENDOR_ID                   = 0x87,
	//E_MSG_VS_GET_CEC_VERSION                    = 0x9F,       //1.3a

	E_MSG_VS_GIVE_VENDOR_ID                     = 0x8C, // should be removed
	E_MSG_VS_GIVE_DEVICE_VENDOR_ID              = 0x8C,

	E_MSG_VS_VENDOR_COMMAND                     = 0x89,
	E_MSG_VS_VENDOR_COMMAND_WITH_ID             = 0xA0,      //1.3a

	E_MSG_VS_VENDOR_RC_BUT_DOWN                 = 0x8A, // should be removed
	E_MSG_VS_VENDOR_REMOTE_BUTTON_DOWN          = 0x8A,

	E_MSG_VS_VENDOR_RC_BUT_UP                   = 0x8B, // should be removed
	E_MSG_VS_VENDOR_REMOTE_BUTTON_UP            = 0x8B,

//----- OSD Display --------------------------------
	E_MSG_SET_OSD_STRING                        = 0x64,

//----- Device OSD Name Transfer  -------------------------
	E_MSG_OSDNT_GIVE_OSD_NAME                   = 0x46,
	E_MSG_OSDNT_SET_OSD_NAME                    = 0x47,

//----- Device Menu Control ------------------------
	E_MSG_DMC_MENU_REQUEST                      = 0x8D,
	E_MSG_DMC_MENU_STATUS                       = 0x8E,
	//E_MSG_DMC_MENU_ACTIVATED                    = 0x00,   //parameter
	//E_MSG_DMC_MENU_DEACTIVATED                  = 0x01,   //parameter

	E_MSG_UI_PRESS                              = 0x44, // should be removed
	E_MSG_DMC_USER_CONTROL_PRESSED              = 0x44,

	E_MSG_UI_RELEASE                            = 0x45, // should be removed
	E_MSG_DMC_USER_CONTROL_RELEASED             = 0x45,

//----- Remote Control Passthrough ----------------
//----- UI Message --------------------------------
//    E_MSG_RCP_USER_CONTROL_PRESSED              = 0x44,
//    E_MSG_RCP_USER_CONTROL_RELEASED             = 0x45,


//----- Power Status  ------------------------------
	E_MSG_PS_GIVE_POWER_STATUS                  = 0x8F, // should be removed
	E_MSG_PS_GIVE_DEVICE_POWER_STATUS           = 0x8F,

	E_MSG_PS_REPORT_POWER_STATUS                = 0x90,

//----- General Protocal Message ------------------

//----- Feature Abort -----------------------------
	E_MSG_FEATURE_ABORT                         = 0x00,

//----- Abort Message -----------------------------
	E_MSG_ABORT_MESSAGE                         = 0xFF,


//----- System Audio Control ------------------
	E_MSG_SAC_GIVE_AUDIO_STATUS                 = 0x71,
	E_MSG_SAC_GIVE_SYSTEM_AUDIO_MODE_STATUS     = 0x7D,
	E_MSG_SAC_REPORT_AUDIO_STATUS               = 0x7A,

	E_MSG_SAC_REPORT_SHORT_AUDIO_DESCRIPTOR     = 0xA3,
	E_MSG_SAC_REQUEST_SHORT_AUDIO_DESCRIPTOR    = 0xA4,

	E_MSG_SAC_SET_SYSTEM_AUDIO_MODE             = 0x72,
	E_MSG_SAC_SYSTEM_AUDIO_MODE_REQUEST         = 0x70,
	E_MSG_SAC_SYSTEM_AUDIO_MODE_STATUS          = 0x7E,

//----- System Audio Control ------------------
	E_MSG_SAC_SET_AUDIO_RATE                    = 0x9A,


//----- Audio Return Channel  Control ------------------
	E_MSG_ARC_INITIATE_ARC                      = 0xC0,
	E_MSG_ARC_REPORT_ARC_INITIATED              = 0xC1,
	E_MSG_ARC_REPORT_ARC_TERMINATED             = 0xC2,
	E_MSG_ARC_REQUEST_ARC_INITIATION            = 0xC3,
	E_MSG_ARC_REQUEST_ARC_TERMINATION           = 0xC4,
	E_MSG_ARC_TERMINATE_ARC                     = 0xC5,

//----- Capability Discovery and Control ------------------
	E_MSG_CDC_CDC_MESSAGE                       = 0xF8,

// HDMI 2.0
//----- Dynamic Auto Lipsync ---------------------
	E_MSG_REQUEST_CURRENT_LATENCY               = 0xA7,
	E_MSG_REPORT_CURRENT_LATENCY                = 0xA8,

} CEC_MSGLIST;

//UI command parameter: Table 27 User Control Codes
typedef enum _CEC_MSG_USER_CTRL_PARM {
	E_MSG_UI_SELECT             = 0x00,
	E_MSG_UI_UP                 = 0x01,
	E_MSG_UI_DOWN               = 0x02,
	E_MSG_UI_LEFT               = 0x03,
	E_MSG_UI_RIGHT              = 0x04,
	E_MSG_UI_RIGHT_UP           = 0x05,
	E_MSG_UI_RIGHT_DOWN         = 0x06,
	E_MSG_UI_LEFT_UP            = 0x07,
	E_MSG_UI_LEFT_DOWN          = 0x08,
	E_MSG_UI_ROOTMENU           = 0x09,
	E_MSG_UI_SETUP_MENU         = 0x0A,
	E_MSG_UI_CONTENTS_MENU      = 0x0B,
	E_MSG_UI_FAVORITE_MENU      = 0x0C,
	E_MSG_UI_EXIT               = 0x0D,

// 0x0E ~ 0x1F  reserved

	E_MSG_UI_NUMBER_0           = 0x20,
	E_MSG_UI_NUMBER_1           = 0x21,
	E_MSG_UI_NUMBER_2           = 0x22,
	E_MSG_UI_NUMBER_3           = 0x23,
	E_MSG_UI_NUMBER_4           = 0x24,
	E_MSG_UI_NUMBER_5           = 0x25,
	E_MSG_UI_NUMBER_6           = 0x26,
	E_MSG_UI_NUMBER_7           = 0x27,
	E_MSG_UI_NUMBER_8           = 0x28,
	E_MSG_UI_NUMBER_9           = 0x29,

	E_MSG_UI_DOT                = 0x2A,
	E_MSG_UI_ENTER              = 0x2B,
	E_MSG_UI_CLEAR              = 0x2C,

// 0x2D ~ 0x2E reserved
	E_MSG_UI_NEXT_FAVORITE      = 0x2F,

	E_MSG_UI_CHANNEL_UP         = 0x30,
	E_MSG_UI_CHANNEL_DOWN       = 0x31,
	E_MSG_UI_PREVIOUS_CHANNEL   = 0x32,
	E_MSG_UI_SOUND_SELECT       = 0x33,
	E_MSG_UI_INPUT_SELECT       = 0x34,
	E_MSG_UI_DISPLAY_INFO       = 0x35,
	E_MSG_UI_HELP               = 0x36,
	E_MSG_UI_PAGE_UP            = 0x37,
	E_MSG_UI_PAGE_DOWN          = 0x38,

// 0x39 ~ 0x3F reserved

	E_MSG_UI_POWER              = 0x40,
	E_MSG_UI_VOLUME_UP          = 0x41,
	E_MSG_UI_VOLUME_DOWN        = 0x42,
	E_MSG_UI_MUTE               = 0x43,
	E_MSG_UI_PLAY               = 0x44,
	E_MSG_UI_STOP               = 0x45,
	E_MSG_UI_PAUSE              = 0x46,
	E_MSG_UI_RECORD             = 0x47,
	E_MSG_UI_REWIND             = 0x48,
	E_MSG_UI_FAST_FORWARD       = 0x49,
	E_MSG_UI_EJECT              = 0x4A,
	E_MSG_UI_FORWARD            = 0x4B,
	E_MSG_UI_BACKWARD           = 0x4C,
	E_MSG_UI_STOP_RECORD        = 0x4D,
	E_MSG_UI_PAUSE_RECORD       = 0x4E,

// 0x4F reserved

	E_MSG_UI_ANGLE                      = 0x50,
	E_MSG_UI_SUB_PICTURE                = 0x51,
	E_MSG_UI_VIDEO_ON_DEMAND            = 0x52,
	E_MSG_UI_ELECTRONIC_PROGRAM_GUIDE   = 0x53,
	E_MSG_UI_TIMER_PROGRAMMING          = 0x54,
	E_MSG_UI_INITIAL_CONFIGURATION      = 0x55,

// 0x56 ~ 0x5F reserved

//0x60 ~ 0x6D, identified as function
	E_MSG_UI_PLAY_FUN               = 0x60,
	E_MSG_UI_PSUSE_PLAY_FUN         = 0x61,
	E_MSG_UI_RECORD_FUN             = 0x62,
	E_MSG_UI_PAUSE_RECORD_FUN       = 0x63,
	E_MSG_UI_STOP_FUN               = 0x64,
	E_MSG_UI_MUTE_FUN               = 0x65,
	E_MSG_UI_RESTORE_VOLUME_FUN     = 0x66,
	E_MSG_UI_TUNE_FUN               = 0x67,
	E_MSG_UI_SELECT_MEDIA_FUN       = 0x68,
	E_MSG_UI_SELECT_AV_INPUT_FUN    = 0x69,
	E_MSG_UI_SELECT_AUDIO_INPUT_FUN = 0x6A,
	E_MSG_UI_POWER_TOGGLE_FUN       = 0x6B,
	E_MSG_UI_POWER_OFF_FUN          = 0x6C,
	E_MSG_UI_POWER_ON_FUN           = 0x6D,

// 0x6E ~ 0x70 reserved

	E_MSG_UI_F1_BLUE            = 0x71,
	E_MSG_UI_F2_RED             = 0x72,
	E_MSG_UI_F3_GREEN           = 0x73,
	E_MSG_UI_F4_YELLOW          = 0x74,
	E_MSG_UI_F5                 = 0x75,
	E_MSG_UI_DATA               = 0x76,

// 0x77 ~ 0xFF reserved
} CEC_MSG_USER_CTRL_PARM;


void mt5870_init_regbase(void __iomem *reg);
void mtk_cec_enable(struct mtk_cec_dev *pdev);
void mtk_cec_disable(struct mtk_cec_dev *pdev);
void mtk_cec_setlogicaladdress(struct mtk_cec_dev *pdev, u8 u8logicaladdr);
bool mtk_cec_sendframe(struct mtk_cec_dev *pdev, u8 *data, u8 len);
u8 mtk_cec_get_event_status(struct mtk_cec_dev *pdev);
void mtk_cec_clear_event_status(struct mtk_cec_dev *pdev, u16 u16clrvalue);
void mtk_cec_get_receive_message(struct mtk_cec_dev *pdev);
void mtk_cec_config_wakeup(struct mtk_cec_dev *pdev);
