// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author jefferry.yen <jefferry.yen@mediatek.com>
 */
#include <linux/export.h>
#include <linux/ktime.h>
#include <linux/input.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/kmod.h>
#include <linux/sched.h>
#include <linux/freezer.h>
#include <linux/version.h>
#ifdef CONFIG_AMAZON_SILENT_OTA_BACKLIGHT
#include <linux/silent_ota_backlight.h>
#endif
#include "ir_core.h"
#include "ir_common.h"
#include "ir_ts.h"

#include "ir_config.h"

struct list_head stage_List = LIST_HEAD_INIT(
								  stage_List);
EXPORT_SYMBOL(stage_List);

struct time_stage *pdiff_time;
struct timex txc_start;
struct timex txc_end;
struct timex txc;
unsigned long ts_IsEnable;
u8 u8FifoDataLen;

static DEFINE_SPINLOCK(gettime_lock);

static inline void do_gettimeofday(struct timeval *tv)
{
	struct timespec64 now;

	ktime_get_real_ts64(&now);
	tv->tv_sec = now.tv_sec;
	tv->tv_usec = now.tv_nsec/1000;
}
static unsigned long cal_time(struct timex *tmx)
{
	unsigned long caltime;

	caltime = tmx->time.tv_sec * 1000000 +
			  tmx->time.tv_usec;
	return caltime;
}

void gettime(int flag, u8 stage)
{
	if (ts_IsEnable == 0x1) {
		spin_lock_irq(&gettime_lock);
		do_gettimeofday(&(txc.time));
		switch (stage) {
		case MDRV_IR_ISR:
			if (flag == TIME_START) {
				pdiff_time = kmalloc(sizeof(*pdiff_time),
									 GFP_ATOMIC);
				if (!pdiff_time) {
					spin_unlock_irq(&gettime_lock);
					return;
				}
				memset(pdiff_time, 0, sizeof(*pdiff_time));
				pdiff_time->time_MDrv_IR_ISR.start_time =
					cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MDrv_IR_ISR.end_time = cal_time(&txc);
				pdiff_time->time_MDrv_IR_ISR.diff_time =
					pdiff_time->time_MDrv_IR_ISR.end_time -
					pdiff_time->time_MDrv_IR_ISR.start_time;
			}
			break;
		case MIRC_DATA_STORE:
			if (flag == TIME_START) {
				pdiff_time->time_MIRC_Data_Store.start_time =
					cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MIRC_Data_Store.end_time =
					cal_time(&txc);
				pdiff_time->time_MIRC_Data_Store.diff_time =
					pdiff_time->time_MIRC_Data_Store.end_time -
					pdiff_time->time_MIRC_Data_Store.start_time;
			}
			break;
		case MIRC_Data_WAKEUP:
			if (flag == TIME_START) {
				pdiff_time->time_MIRC_Data_Wakeup.start_time =
					cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MIRC_Data_Wakeup.end_time =
					cal_time(&txc);
				pdiff_time->time_MIRC_Data_Wakeup.diff_time =
					pdiff_time->time_MIRC_Data_Wakeup.end_time -
					pdiff_time->time_MIRC_Data_Wakeup.start_time;
			}
			break;
		case MIRC_DATA_CTRL_THREAD:
			if (flag == TIME_START) {
				pdiff_time->time_MIRC_Data_Ctrl_Thread.start_time
					= cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MIRC_Data_Ctrl_Thread.end_time =
					cal_time(&txc);
				pdiff_time->time_MIRC_Data_Ctrl_Thread.diff_time =
					pdiff_time->time_MIRC_Data_Ctrl_Thread.end_time -
					pdiff_time->time_MIRC_Data_Ctrl_Thread.start_time;
				list_add_tail(&pdiff_time->list, &stage_List);
				pdiff_time = NULL;
			}
			break;
		case MIRC_DATA_GET:
			if (flag == TIME_START) {
				pdiff_time->time_MIRC_Data_Get.start_time =
					cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MIRC_Data_Get.end_time =
					cal_time(&txc);
				pdiff_time->time_MIRC_Data_Get.diff_time =
					pdiff_time->time_MIRC_Data_Get.end_time -
					pdiff_time->time_MIRC_Data_Get.start_time;
			}
			break;
		case MIRC_DECODE_LIST_ENTRY:
			if (flag == TIME_START) {
				pdiff_time->time_MIRC_Decode_list_entry.start_time
					= cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MIRC_Decode_list_entry.end_time =
					cal_time(&txc);
				pdiff_time->time_MIRC_Decode_list_entry.diff_time
					=
						pdiff_time->time_MIRC_Decode_list_entry.end_time -
						pdiff_time->time_MIRC_Decode_list_entry.start_time;
			}
			break;
		case MIRC_DIFF_PROTOCOL:
			if (flag == TIME_START) {
				pdiff_time->time_MIRC_Diff_protocol.start_time =
					cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MIRC_Diff_protocol.end_time =
					cal_time(&txc);
				pdiff_time->time_MIRC_Diff_protocol.diff_time =
					pdiff_time->time_MIRC_Diff_protocol.end_time -
					pdiff_time->time_MIRC_Diff_protocol.start_time;
			}
			break;
		case MIRC_DATA_MATCH_SHOTCOUNT:
			if (flag == TIME_START) {
				pdiff_time->time_MIRC_Data_Match_shotcount.start_time
					= cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MIRC_Data_Match_shotcount.end_time
					= cal_time(&txc);
				pdiff_time->time_MIRC_Data_Match_shotcount.diff_time
					=
					pdiff_time->time_MIRC_Data_Match_shotcount.end_time
					-
					pdiff_time->time_MIRC_Data_Match_shotcount.start_time;
			}
			break;
		case MIRC_SEND_KEY_EVENT:
			if (flag == TIME_START) {
				pdiff_time->time_MIRC_Send_key_event.start_time =
					cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MIRC_Send_key_event.end_time =
					cal_time(&txc);
				pdiff_time->time_MIRC_Send_key_event.diff_time =
					pdiff_time->time_MIRC_Send_key_event.end_time -
					pdiff_time->time_MIRC_Send_key_event.start_time;
			}
			break;
		case MIRC_KEYCODE_FROM_MAP:
			if (flag == TIME_START) {
				pdiff_time->time_MIRC_Keycode_From_Map.start_time
					= cal_time(&txc);
			} else if (flag == TIME_END) {
				pdiff_time->time_MIRC_Keycode_From_Map.end_time =
					cal_time(&txc);
				pdiff_time->time_MIRC_Keycode_From_Map.diff_time =
					pdiff_time->time_MIRC_Keycode_From_Map.end_time -
					pdiff_time->time_MIRC_Keycode_From_Map.start_time;
			}
			break;
		default:
			//IR_PRINT("no match IR time stage");
			break;
		}
		spin_unlock_irq(&gettime_lock);
	}
}


/* Used to handle IR raw handler extensions */
static DEFINE_MUTEX(ir_decoder_lock);
static LIST_HEAD(ir_decoder_list);

static bool WhiteBalanceFlag = FALSE;
u64	 protocols; //support protocols for decoder
static unsigned long
_ulLastKeyPresentTime; //record last key press time
enum IR_DBG_LEVEL_e _edbglevel =
	IR_DBG_NONE;//IR_DBG_INFO; //debug level for /sys/device/ir/IRDebug

#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
#define IR_FIRST_REPEAT_TIMEOUT 150
#define IR_DEFAULT_TIMEOUT 150	//timeout maybe > two IR Waveform cycle times
static unsigned long  eventtimeout =
	IR_DEFAULT_TIMEOUT; //for  event timeout
enum IR_Type_e g_IrType;
int ir_map_period(enum IR_Type_e IrType)
{
	switch (IrType) {
	case IR_TYPE_NEC:
		return NEC_PERIOD;
	case IR_TYPE_RCA:
		return RCA_PERIOD;
	case IR_TYPE_P7051:
		return P7051_PERIOD;
	case IR_TYPE_RC5:
	case IR_TYPE_RC5X:
		return RC5_PERIOD;
	case IR_TYPE_RC6_MODE0:
	case IR_TYPE_RC6:
		return RC6_PERIOD;
	case IR_TYPE_TOSHIBA:
		return TOSHIBA_PERIOD;
	case IR_TYPE_RCMM:
		return RCMM_PERIOD;
	case IR_TYPE_KONKA:
		return KONKA_PERIOD;
	case IR_TYPE_SHARP:
		return SHARP_PERIOD;
	case IR_TYPE_SONY:
		return SONY_PERIOD;
	default:
		IRDBG_ERR("IR TYPE not define,please check!!!\n");
		return IR_DEFAULT_TIMEOUT;
	}
}
u32 eventtimeoutMax;
//EXPORT_SYMBOL(eventtimeoutMax);
void MIRC_support_ir_max_timeout(void)
{
	struct mtk_ir_dev *dev = IRDev.pIRDev;
	int i = 0;
	u32 temp = 0;

	for (i = 0; i < IR_SUPPORT_MAX; i++)
		if (dev->support_ir[i].u8Enable == TRUE) {
			if (!dev->support_ir[i].u32IRSpeed)
				temp = (ir_map_period(dev->support_ir[i].eIRType))
					   + 20;
			else
				temp = dev->support_ir[i].u32IRSpeed *
					   (ir_map_period(
							dev->support_ir[i].eIRType)) + 20;
			if (temp > eventtimeoutMax)
				eventtimeoutMax = temp;
		}
}
EXPORT_SYMBOL(MIRC_support_ir_max_timeout);

unsigned long MIRC_Get_Event_Timeout(void)
{
	return eventtimeout;
}

void MIRC_Set_Event_Timeout(unsigned long time)
{
	eventtimeout = time;
}
#endif

enum IR_SPEED_LEVEL_e MIRC_Get_IRSpeed_Level(void)
{
	return IRDev.pIRDev->speed;
}

void MIRC_Set_IRSpeed_Level(enum IR_SPEED_LEVEL_e
							level)
{
	IRDev.pIRDev->speed = level;
}

enum IR_DBG_LEVEL_e MIRC_Get_IRDBG_Level(void)
{
	return _edbglevel;
}

void MIRC_Set_IRDBG_Level(enum IR_DBG_LEVEL_e level)
{
	_edbglevel = level;
}

u64 MIRC_Get_Protocols(void)
{
	return protocols;
}
void MIRC_Set_Protocols(u64 data)
{
	protocols = data;
	IRDev.pIRDev->enabled_protocols = protocols;
}

unsigned long MIRC_Get_System_Time(void)
{
	return ((unsigned long)((jiffies) * (1000 / HZ)));
}

unsigned long MIRC_Get_LastKey_Time(void)
{
	return _ulLastKeyPresentTime;
}

bool MIRC_IsEnable_WhiteBalance(void)
{
	return WhiteBalanceFlag;
}
void MIRC_Set_WhiteBalance_Enable(bool bEnable)
{
	u8 i = 0;

	WhiteBalanceFlag = bEnable;
	if (!bEnable)
		for (i = 0; i < IRDev.pIRDev->support_num; i++)
			IRDev.pIRDev->support_ir[i].u8Enable = TRUE;
	else
		for (i = 0; i < IRDev.pIRDev->support_num; i++)
			IRDev.pIRDev->support_ir[i].u8Enable = FALSE;
}

void MIRC_Send_KeyEvent(u8
						flag) // 0 ---> up ,1 -->down
{
	struct mtk_ir_dev *dev = IRDev.pIRDev;

	gettime(TIME_START, MIRC_SEND_KEY_EVENT);
	if (flag == 0) {
		MIRC_Keyup(dev);
		dev->raw->prev_sc.scancode = 0;
		IRDBG_INFO("key_up =====\n");
	} else if (flag == 1) {
		IRDBG_INFO("key_down =====\n");
	}
	gettime(TIME_END, MIRC_SEND_KEY_EVENT);
}

void MIRC_IR_Config_Dump(void)
{
	u8 i = 0;

	for (i = 0; i < IRDev.pIRDev->support_num; i++) {
		IR_PRINT("eIRType = %x, u32HeadCode = %x u8Enable = %x\n",
				 IRDev.pIRDev->support_ir[i].eIRType,
				 IRDev.pIRDev->support_ir[i].u32HeadCode,
				 IRDev.pIRDev->support_ir[i].u8Enable);
	}
}

void MIRC_Set_IR_Enable(enum IR_Type_e eIRType,
						u32 u32HeadCode,
						u8 u8Enable)
{
	u8 i = 0;

	for (i = 0; i < IRDev.pIRDev->support_num; i++) {
		if ((IRDev.pIRDev->support_ir[i].eIRType ==
			 eIRType)
			&& (IRDev.pIRDev->support_ir[i].u32HeadCode ==
				u32HeadCode)) {
			IRDev.pIRDev->support_ir[i].u8Enable = u8Enable;
			break;
		}
	}
}

static int MIRC_Key_process(struct ir_raw_data_ctrl *raw)
{
	if (raw->this_sc.scancode_protocol !=
		raw->prev_sc.scancode_protocol) {
		//if before timeout send keyup + keydown
		if (time_after_eq((raw->keyup_jiffies +
						   msecs_to_jiffies(
							   eventtimeout)),
						  jiffies)) {
			MIRC_Keyup(raw->dev);
		}
		IRDBG_INFO("[2]========= Down First!\n");
		MIRC_Keydown(raw->dev,
					 raw->this_sc.scancode);//first keydown
	} else { // same protocol
		if (raw->this_sc.scancode !=
			raw->prev_sc.scancode) {
			// diff key press
			//if before timeout send keyup + keydown
			if (time_after_eq((raw->keyup_jiffies +
							   msecs_to_jiffies(
							   eventtimeout)),
						  jiffies)) {
				MIRC_Keyup(raw->dev);
			}
		IRDBG_INFO("[1]Down First!\n");
		MIRC_Keydown(raw->dev,
				raw->this_sc.scancode);
					//first keydown
		} else {
			if ((raw->u8RepeatFlag == 0)
			|| (!raw->dev->keypressed)) {
			// press the same fast
			//if before timeout send keyup + keydown
				if (time_after_eq(
				(raw->keyup_jiffies +
				msecs_to_jiffies(eventtimeout)), jiffies))
					MIRC_Keyup(raw->dev);

				MIRC_Keydown(raw->dev,
				raw->this_sc.scancode);
			} else
				IRDBG_INFO("Repeat!\n");
		}
	}

	return 0;
}
static int MIRC_Data_Ctrl_Thread(void *data)
{
	struct ir_raw_data ev;
	struct ir_raw_data_ctrl *raw = (struct
									 ir_raw_data_ctrl *)data;
	int retval;
	int get_scancode;

	ev.duration = 0;
	ev.pulse = 0;

	while (!kthread_should_stop()) {
		get_scancode = 0;
		spin_lock_irq(&raw->lock);
		retval = kfifo_len(&raw->kfifo);

		if (retval < sizeof(ev)) {
			set_current_state(TASK_INTERRUPTIBLE);

			if (kthread_should_stop())
				set_current_state(TASK_RUNNING);
			spin_unlock_irq(&raw->lock);
			schedule();
			continue;
		}
		u8FifoDataLen = (retval / sizeof(ev));
		gettime(TIME_START, MIRC_DATA_CTRL_THREAD);

		gettime(TIME_START, MIRC_DATA_GET);
		retval = kfifo_out(&raw->kfifo, &ev, sizeof(ev));
		gettime(TIME_END, MIRC_DATA_GET);

		spin_unlock_irq(&raw->lock);

		mutex_lock(&ir_decoder_lock);
		gettime(TIME_START, MIRC_DECODE_LIST_ENTRY);
		if (raw->dev->ir_mode ==
			IR_TYPE_SWDECODE_MODE) { //sw decoder
			struct ir_decoder_handler *handler;

			IRDBG_MSG("shotcount[ %5d  %d ]\n", ev.duration,
					  ev.pulse);
			list_for_each_entry(handler, &ir_decoder_list,
								list) {
				if (handler->decode(raw->dev, ev) > 0) {
					get_scancode = 1;
					break;
				}
			}
		} else { //hw decoder
			raw->this_sc.scancode = ev.duration;
			raw->u8RepeatFlag = ev.pulse;
			get_scancode = 1;
			IRDBG_MSG("keycode =%x	repeatflag =%d\n",
					  raw->this_sc.scancode,
					  raw->u8RepeatFlag);
		}
		if (pdiff_time != NULL)
			pdiff_time->flag = raw->u8RepeatFlag;
		gettime(TIME_END, MIRC_DECODE_LIST_ENTRY);
		if (get_scancode) {
			IRDBG_INFO("[IR]scancode=0x%x, repeat=%d\n", raw->this_sc.scancode, raw->u8RepeatFlag);
#ifdef CONFIG_IR_DISABLE_REPEAT
			clear_bit(EV_REP, raw->dev->input_dev->evbit);
#endif
			//diff protocol
			if (raw->u8RepeatFlag == 1)
				eventtimeout = eventtimeoutMax;
			else
				eventtimeout = IR_FIRST_REPEAT_TIMEOUT;
			_ulLastKeyPresentTime = MIRC_Get_System_Time();
			if (raw->dev->filter_flag == 0) {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
				//diff protocol
				gettime(TIME_START, MIRC_DIFF_PROTOCOL);
				MIRC_Key_process(raw);
				if (pdiff_time != NULL)
					pdiff_time->keycode =
					raw->this_sc.scancode;
				gettime(TIME_END, MIRC_DIFF_PROTOCOL);
			}

			del_timer(&(raw->timer_keyup));
			raw->timer_keyup.expires = jiffies +
						msecs_to_jiffies(eventtimeout);
			raw->keyup_jiffies = jiffies;
			add_timer(&(raw->timer_keyup));
#else
				// for DFB polling read	  /dev/ir
				if (down_trylock(&raw->dev->sem) == 0) {

					if (kfifo_in(&raw->dev->read_fifo,
							&raw->this_sc.scancode,
							sizeof(u32)) !=
							sizeof(u32)) {
						up(&raw->dev->sem);
						mutex_unlock(&ir_decoder_lock);
						return -ENOMEM;
					}
					up(&raw->dev->sem);
					wake_up_interruptible(
					&raw->dev->read_wait);
				}
#endif
			//record IR last scancode
			raw->prev_sc.scancode_protocol =
				raw->this_sc.scancode_protocol;
			raw->prev_sc.scancode = raw->this_sc.scancode;
			raw->this_sc.scancode_protocol = 0;
			raw->this_sc.scancode = 0x0;
			raw->u8RepeatFlag = 0;
		}
		mutex_unlock(&ir_decoder_lock);
		gettime(TIME_END, MIRC_DATA_CTRL_THREAD);

	}

	return 0;
}

int MIRC_Data_Store(struct mtk_ir_dev *dev,
					struct ir_raw_data *ev)
{
	if (!dev->raw)
		return -EINVAL;
	if (kfifo_in(&dev->raw->kfifo, ev,
				 sizeof(*ev)) != sizeof(*ev))
		return -ENOMEM;

	return 0;
}

void MIRC_Data_Wakeup(struct mtk_ir_dev *dev)
{
	unsigned long flags;

	if (!dev->raw)
		return;

	spin_lock_irqsave(&dev->raw->lock, flags);
	wake_up_process(dev->raw->thread);
	spin_unlock_irqrestore(&dev->raw->lock, flags);
}

/*
 * Used to (un)register raw event clients
 */
int MIRC_Data_Ctrl_Init(struct mtk_ir_dev *dev)
{
	int ret;

	if (!dev)
		return -EINVAL;

	dev->raw = kzalloc(sizeof(*dev->raw), GFP_KERNEL);
	if (!dev->raw)
		return -ENOMEM;

	dev->raw->dev = dev;
	dev->enabled_protocols = 0;
	ret = kfifo_alloc(&dev->raw->kfifo,
				sizeof(struct ir_raw_data) * MAX_IR_DATA_SIZE,
				GFP_KERNEL);
	if (ret < 0)
		goto out;

	spin_lock_init(&dev->raw->lock);
	dev->raw->thread = kthread_run(
					MIRC_Data_Ctrl_Thread, dev->raw,
					"MTK_ir");

	if (IS_ERR(dev->raw->thread)) {
		ret = PTR_ERR(dev->raw->thread);
		goto out;
	}
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	// init timer event up
	dev->raw->keyup_jiffies =
		jiffies;//record timeout for event up
	timer_setup(&(dev->raw->timer_keyup),
				MIRC_Timer_Proc, 0);
	dev->raw->timer_keyup.expires = jiffies +
							msecs_to_jiffies(
							eventtimeout);
#endif
	return 0;
out:
	kfree(dev->raw);
	dev->raw = NULL;
	return ret;
}

void MIRC_Data_Ctrl_DeInit(struct mtk_ir_dev *
						   dev)
{
	if (!dev || !dev->raw)
		return;
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	del_timer(&(dev->raw->timer_keyup));
#endif
	kthread_stop(dev->raw->thread);
	kfifo_free(&dev->raw->kfifo);
	kfree(dev->raw);
	dev->raw = NULL;
}

int MIRC_IRCustomer_Remove(struct IR_Profile_s *stIr)
{
	int i, j;
	struct mtk_ir_dev *dev = IRDev.pIRDev;

	if ((dev == NULL) || (stIr == NULL))
		return -EINVAL;
	for (i = 0; i < dev->support_num; i++) {
		if (memcmp(&dev->support_ir[i], stIr,
				   sizeof(struct IR_Profile_s)) == 0) {
			break;
		}
	}
	if (i < dev->support_num) {
		dev->support_num--;
		for (j = i; j < dev->support_num; j++) {
			memcpy(&dev->support_ir[j],
				   &dev->support_ir[j + 1],
				   sizeof(struct IR_Profile_s));
		}
		return 0;
	}
	return -1;
}
EXPORT_SYMBOL_GPL(MIRC_IRCustomer_Remove);

int MIRC_IRCustomer_Add(struct IR_Profile_s *stIr)
{
	struct mtk_ir_dev *dev = IRDev.pIRDev;

	if ((dev == NULL) || (stIr == NULL))
		return -EINVAL;
	if (dev->support_num >= IR_SUPPORT_MAX)
		return -EINVAL;
	IRDBG_INFO("eIRType = %d,u32HeadCode = %x,u32IRSpeed = %d\n",
			   stIr->eIRType,
			   stIr->u32HeadCode, stIr->u32IRSpeed);
	memcpy(&dev->support_ir[dev->support_num], stIr,
		   sizeof(struct IR_Profile_s));
	dev->support_num++;
	return 0;
}
EXPORT_SYMBOL_GPL(MIRC_IRCustomer_Add);

int MIRC_IRCustomer_Init(void)
{
	struct mtk_ir_dev *dev = IRDev.pIRDev;
	enum IR_Type_e IrType = IR_TYPE_MAX;
	u8 i = 0;
	bool bIsDiffType = false;

	if (dev == NULL)
		return -EINVAL;

	if (dev->support_num ==	1) {
		//for the case of IR_SUPPORT_NUM is 1
		IrType = dev->support_ir[0].eIRType;
	}
	for (i = 0; i < (dev->support_num - 1); i++) {
		if (dev->support_ir[i].eIRType ==
			dev->support_ir[i + 1].eIRType) {
			IrType = dev->support_ir[i].eIRType;
			bIsDiffType = false;
		} else {
			dev->ir_mode = IR_TYPE_SWDECODE_MODE;
			bIsDiffType = true;
			break;
		}
	}


	if (bIsDiffType == false) {
		if (IrType == IR_TYPE_NEC) {
			if (dev->support_num <= 2) {
				//set customer code0
				u32IRHeaderCode[0] =
					dev->support_ir[0].u32HeadCode;
				u32IRHeaderCode[1] =
					dev->support_ir[1].u32HeadCode;
				dev->ir_mode = IR_TYPE_FULLDECODE_MODE;
			} else {
				dev->ir_mode = IR_TYPE_RAWDATA_MODE;
			}
		} else if (IrType == IR_TYPE_RC5) {
			dev->ir_mode = IR_TYPE_HWRC5_MODE;
		} else if (IrType == IR_TYPE_RC5X) {
			dev->ir_mode = IR_TYPE_HWRC5X_MODE;
		} else if (IrType == IR_TYPE_RC6_MODE0) {
			dev->ir_mode = IR_TYPE_HWRC6_MODE;
		} else {
			dev->ir_mode = IR_TYPE_SWDECODE_MODE;
		}
	}


	dev->ir_mode = IR_TYPE_SWDECODE_MODE;


	if (dev->ir_mode == IR_TYPE_SWDECODE_MODE) {
		for (i = 0; i < (dev->support_num); i++) {
			switch (dev->support_ir[i].eIRType) {
			case IR_TYPE_NEC:
				nec_decode_init();
				break;
			case IR_TYPE_SONY:
				sony_decode_init();
				break;
			case IR_TYPE_RCA:
				rca_decode_init();
				break;
			case IR_TYPE_P7051:
				p7051_decode_init();
				break;
			case IR_TYPE_RC5:
			case IR_TYPE_RC5X:
				rc5_decode_init();
				break;
			case IR_TYPE_RC6_MODE0:
			case IR_TYPE_RC6:
				rc6_decode_init();
				break;
			case IR_TYPE_TOSHIBA:
				toshiba_decode_init();
				break;
			case IR_TYPE_RCMM:
				rcmm_decode_init();
				break;
			case IR_TYPE_KONKA:
				konka_decode_init();
				break;
			case IR_TYPE_PANASONIC:
				panasonic_decode_init();
				break;
			case IR_TYPE_RC311:
				rc311_decode_init();
				break;
			case IR_TYPE_BEKO_RC5:
				beko_rc5_decode_init();
				break;
			case IR_TYPE_SHARP:
				sharp_decode_init();
				break;
			default:
				IRDBG_ERR("IR TYPE not define!!\n");
				break;
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(MIRC_IRCustomer_Init);

int MIRC_IRCustomer_Config(struct IR_Profile_s *stIr,
						   u8 num)
{
	u8 i = 0;

	if (stIr == NULL)
		return -EINVAL;
	if (num > IR_SUPPORT_MAX)
		num = IR_SUPPORT_MAX;

	for (i = 0; i < num; i++) {
		if (MIRC_IRCustomer_Add(&stIr[i]) != 0)
			return -EINVAL;
	}
	MIRC_IRCustomer_Init();
	MIRC_support_ir_max_timeout();
	return 0;
}

int MIRC_IRCustomer_DeConfig(struct IR_Profile_s *stIr,
							 u8 num)
{
	struct mtk_ir_dev *dev = IRDev.pIRDev;
	u8 i = 0;

	if ((stIr == NULL) || (dev == NULL))
		return -EINVAL;
	for (i = 0; i < num; i++)
		MIRC_IRCustomer_Remove(&stIr[i]);

	return 0;
}

/*
 * Extension interface - used to register the IR decoders
 */
int MIRC_Decoder_Register(struct
			ir_decoder_handler *handler)
{
	mutex_lock(&ir_decoder_lock);
	list_add_tail(&handler->list, &ir_decoder_list);
	protocols |= handler->protocols;
	MIRC_Set_Protocols(protocols);
	mutex_unlock(&ir_decoder_lock);

	return 0;
}
EXPORT_SYMBOL(MIRC_Decoder_Register);

void MIRC_Decoder_UnRegister(struct
				ir_decoder_handler *handler)
{
	mutex_lock(&ir_decoder_lock);
	list_del(&handler->list);
	protocols &= ~(handler->protocols);
	MIRC_Set_Protocols(protocols);
	mutex_unlock(&ir_decoder_lock);
}
EXPORT_SYMBOL(MIRC_Decoder_UnRegister);

#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE

static LIST_HEAD(keymap_list);

static DEFINE_SPINLOCK(key_map_lock);

//key map functions
int MIRC_Map_Register(struct key_map_list *map)
{
	spin_lock(&key_map_lock);
	list_add_tail(&map->list, &keymap_list);
	spin_unlock(&key_map_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(MIRC_Map_Register);

void MIRC_Map_UnRegister(struct key_map_list *
						 map)
{
	spin_lock(&key_map_lock);
	list_del(&map->list);
	spin_unlock(&key_map_lock);
}
EXPORT_SYMBOL_GPL(MIRC_Map_UnRegister);

void MIRC_Timer_Proc(struct timer_list *t)
{
	struct ir_raw_data_ctrl *raw = from_timer(raw, t,
							timer_keyup);
	struct mtk_ir_dev *dev = raw->dev;

	MIRC_Keyup(dev);
	dev->raw->prev_sc.scancode = 0;
	//IR_PRINT("upupup\n");
}

int bypass_ir_config;
EXPORT_SYMBOL(bypass_ir_config);


void MIRC_Dis_support_ir(void)
{
	int i = 0;

	for (i = 0; i < IRDev.pIRDev->support_num; i++) {
		if (IRDev.pIRDev->support_ir[i].u8Enable == TRUE)
			pr_notice("no.%d : %d\n", i,
				   IRDev.pIRDev->support_ir[i].u8Enable);
		IRDev.pIRDev->support_ir[i].u8Enable = FALSE;

	}
	bypass_ir_config = 1;
}
EXPORT_SYMBOL(MIRC_Dis_support_ir);

void MIRC_Decoder_free(void)
{
	struct	list_head *pos, *q;

	list_for_each_safe(pos, q, &ir_decoder_list) {
		struct ir_decoder_handler *dec = NULL;

		dec = list_entry(pos, struct ir_decoder_handler,
						 list);
		list_del(pos);
	}
	MIRC_IRCustomer_DeConfig(ir_config,
							 1/*IR_SUPPORT_NUM*/);
	IRDev.pIRDev->support_num = 0;
}
EXPORT_SYMBOL(MIRC_Decoder_free);


void MIRC_Map_free(void)
{
	struct	list_head *pos, *q;

	list_for_each_safe(pos, q, &keymap_list) {
		struct key_map_list *map = NULL;

		map = list_entry(pos, struct key_map_list, list);
		list_del(pos);
	}
}
EXPORT_SYMBOL(MIRC_Map_free);


static u32 MIRC_Keycode_From_Map(u32 keymapnum,
								 u32 scancode)
{
	struct key_map_list *map = NULL;
	u32 keycode = KEY_RESERVED;
	u8 i = 0;
	bool match_flag = false;

	gettime(TIME_START, MIRC_KEYCODE_FROM_MAP);
	spin_lock(&key_map_lock);
	list_for_each_entry(map, &keymap_list, list) {
#ifdef CONFIG_MP_AMZ_MISC
/* Need to consided the case when header is 0, such as Amazon Greta's */
		if ((((scancode >> 8) == map->map.headcode)
			 && ((scancode >> 8) != 0))
			|| (keymapnum == map->map.headcode))
#else
		if (((scancode >> 8) == map->map.headcode)
			|| (keymapnum == map->map.headcode))
#endif
		{
			IRDBG_INFO("[Match] Keymap Name: %s\n",
					   map->map.name);
			match_flag = true;
			break;//find IR map
		}
	}
	if (match_flag) {
		for (i = 0; i < map->map.size; i++) {
			if (((map->map.scan[i].scancode & 0xff) ==
				 (scancode & 0xff))
				&& ((scancode >> 8) == map->map.headcode)) {
				IRDBG_INFO("[8bit Scancode =%x],Keycode= %x\n",
						   (scancode & 0xff),
						   map->map.scan[i].keycode);
				keycode = map->map.scan[i].keycode;
				break;//match scancode, and return keycode
			}
			if (((map->map.scan[i].scancode & 0xffff) ==
				 (scancode & 0xffff))
				&& (keymapnum == map->map.headcode)) {
				IRDBG_INFO("[16bit Scancode =%x],Keycode= %x\n",
						   (scancode & 0xffff),
						   map->map.scan[i].keycode);
				keycode = map->map.scan[i].keycode;
				break;//match scancode, and return keycode
			}
		}
	}
	spin_unlock(&key_map_lock);
	gettime(TIME_END, MIRC_KEYCODE_FROM_MAP);
	return keycode;
}

u32 MIRC_Get_Keycode(u32 keymapnum, u32 scancode)
{
	return MIRC_Keycode_From_Map(keymapnum, scancode);
}
EXPORT_SYMBOL(MIRC_Get_Keycode);

static void MIRC_Do_Keyup(struct mtk_ir_dev *
						  dev, bool sync)
{
	if (!dev->keypressed)
		return;

#ifdef CONFIG_AMAZON_SILENT_OTA_BACKLIGHT
	toggle_backlight(dev->last_keycode);
#endif
	input_report_key(dev->input_dev,
					 dev->last_keycode, 0);
	if (sync)
		input_sync(dev->input_dev);

	dev->keypressed = false;
}

void MIRC_Keyup(struct mtk_ir_dev *dev)
{
	unsigned long flags	 = 0;

	spin_lock_irqsave(&dev->keylock, flags);
	MIRC_Do_Keyup(dev, true);
	spin_unlock_irqrestore(&dev->keylock, flags);
}

static void MIRC_Do_Keydown(struct mtk_ir_dev *
							dev, int scancode,
							u32 keycode)
{
	bool new_event = !dev->keypressed ||
					 dev->last_scancode != scancode;

	if (new_event && dev->keypressed)
		MIRC_Do_Keyup(dev, false);

	input_event(dev->input_dev, EV_MSC, MSC_SCAN,
				scancode);

	if (new_event && keycode != KEY_RESERVED) {
		/* Register a keypress */
		dev->keypressed = true;
		dev->last_scancode = scancode;
		dev->last_keycode = keycode;
		input_report_key(dev->input_dev, keycode, 1);
	}

	input_sync(dev->input_dev);
}

void MIRC_Keydown(struct mtk_ir_dev *dev,
				  int scancode)
{
	unsigned long flags = 0;
	u32 keycode = 0;

	if (MIRC_IsEnable_WhiteBalance() == TRUE) {
		input_event(dev->input_dev, EV_MSC, MSC_SCAN,
					scancode | 0x1000000);
		input_sync(dev->input_dev);
		dev->last_scancode = scancode;
		return;
	}
	keycode = MIRC_Keycode_From_Map(dev->map_num,
				scancode);
	if (keycode == KEY_RESERVED) {
		IRDBG_ERR("Key Map Match scancode Failed!\n");
		return;
	}
	spin_lock_irqsave(&dev->keylock, flags);
	MIRC_Do_Keydown(dev, scancode, keycode);
	spin_unlock_irqrestore(&dev->keylock, flags);
}
#endif
