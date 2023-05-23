/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */
#ifndef _IR_CORE_
#define _IR_CORE_
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/kfifo.h>
#include <linux/semaphore.h>
#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/version.h>
#include <linux/time.h>	 //added
#include <linux/timer.h> //added
#include <linux/types.h> //added
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include "ir_common.h"
#include "share_struct.h"

/* Define the max number of pulse/space transitions to buffer */
#define MAX_IR_DATA_SIZE	  512

enum IR_DBG_LEVEL_e {
	IR_DBG_NONE = 0,
	IR_DBG_ERR,
	IR_DBG_WRN,
	IR_DBG_MSG,
	IR_DBG_INFO,
	IR_DBG_ALL
};

#define IRDBG_LEVEL MIRC_Get_IRDBG_Level()
///ASCII color code
#define ASCII_COLOR_RED							 "\033[1;31m"
#define ASCII_COLOR_WHITE						 "\033[1;37m"
#define ASCII_COLOR_YELLOW						 "\033[1;33m"
#define ASCII_COLOR_BLUE						 "\033[1;36m"
#define ASCII_COLOR_GREEN						 "\033[1;32m"
#define ASCII_COLOR_END							 "\033[0m"
///debug level
#define IR_PRINT(fmt, args...) \
({ \
	do { \
		pr_notice(fmt, ##args); \
	} while (0); \
})
#define IRDBG_INFO(fmt, args...) \
({ \
	do { \
		if (IRDBG_LEVEL >= IR_DBG_INFO) { \
			pr_notice(ASCII_COLOR_GREEN"[IR INFO]:%s[%d]: ", __func__, __LINE__); \
			pr_notice(fmt, ##args); \
			pr_notice(ASCII_COLOR_END); \
		} \
	} while (0); \
})
#define IRDBG_MSG(fmt, args...) \
({ \
	do { \
		if (IRDBG_LEVEL == IR_DBG_MSG) { \
			pr_notice(ASCII_COLOR_WHITE"[IR MSG]:%s[%d]: ", __func__, __LINE__); \
			pr_notice(fmt, ##args); \
			pr_notice(ASCII_COLOR_END); \
		} \
	} while (0); \
})
#define IRDBG_WRN(fmt, args...) \
({ \
	do { \
		if (IRDBG_LEVEL >= IR_DBG_WRN) { \
			pr_notice(ASCII_COLOR_YELLOW"[IR WRN ]: %s[%d]: ", __func__, __LINE__); \
			pr_notice(fmt, ##args); \
			pr_notice(ASCII_COLOR_END); \
		} \
	} while (0); \
})
#define IRDBG_ERR(fmt, args...) \
({ \
	do { \
		if (IRDBG_LEVEL >= IR_DBG_ERR) { \
			pr_notice(ASCII_COLOR_RED"[IR ERR ]: %s[%d]: ", __func__, __LINE__); \
			pr_notice(fmt, ##args); \
			pr_notice(ASCII_COLOR_END); \
		} \
	} while (0); \
})

enum IR_SPEED_LEVEL_e {
	IR_SPEED_FAST_H = 0,
	IR_SPEED_FAST_N,
	IR_SPEED_FAST_L,
	IR_SPEED_NORMAL,
	IR_SPEED_SLOW_H,
	IR_SPEED_SLOW_N,
	IR_SPEED_SLOW_L
};

struct IRModHandle {
	int							s32IRMajor;
	int							s32IRMinor;
	struct cdev					cDevice;
	const struct file_operations		IRFop;
	struct fasync_struct		*async_queue; /* asynchronous readers */
	unsigned long				u32IRFlag;
	struct mtk_ir_dev			*pIRDev;
};

#define IR_SUPPORT_MAX		   16

struct mtk_ir_dev {
	struct mutex			 lock;
	//lock for handle mtk_ir_dev menber variables
	struct ir_raw_data_ctrl	 *raw;
	//shot count handle struct
	u64						 enabled_protocols;
	//surport decoders
	spinlock_t				 keylock;
	//spinlock
	void					 *priv;
	//
	enum IR_SPEED_LEVEL_e		 speed;
	//ir speed level
	u8						 filter_flag;
	struct semaphore		 sem;
	wait_queue_head_t		 read_wait;
	struct kfifo_rec_ptr_1	 read_fifo;
	//for user space read
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	struct input_dev		 *input_dev;
	bool					 keypressed;
	u32						 last_keycode;
	u32						 last_scancode;
	const char				 *input_name;
	const char				 *input_phys;
	char					 *driver_name;
	u32						  map_num;
#endif
	enum IR_Mode_e			 ir_mode;
	struct IR_Profile_s		 support_ir[IR_SUPPORT_MAX];
	u8						 support_num;
};


struct ir_raw_data {
	u8		  pulse;
	//SW shotcount status  or HW repeatflag
	u32		  duration;
	//SW shotcount value or HW keyvalue
};

struct ir_decoder_handler {
	struct list_head list;

	u64 protocols; /* which are handled by this handler */
	int (*decode)(struct mtk_ir_dev *dev, struct ir_raw_data event);
};

#define DEFINE_IR_RAW_DATA(event) \
struct ir_raw_data event = { \
	.duration = 0, \
	.pulse = 0}


struct ir_scancode {
	u32 scancode;
	u64 scancode_protocol;
};


struct ir_raw_data_ctrl {
	struct task_struct		*thread;
	spinlock_t				lock;
	struct kfifo_rec_ptr_1	kfifo;			 /* fifo for the pulse/space durations */
	struct mtk_ir_dev		*dev;
	struct ir_scancode		prev_sc;
	struct ir_scancode		this_sc;
	u8						u8RepeatFlag;
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	unsigned long			keyup_jiffies;	   //time record
	struct timer_list		timer_keyup;	   //event up timer
#endif
};


//struct key_map_table {
//	u32 scancode;
//	u32 keycode;
//};
//
//struct key_map {
//	struct key_map_table   *scan;
//	unsigned int		   size; /* Max number of entries */
//	const char			   *name;
//	u32					   headcode;
//};
//
//struct key_map_list {
//	struct list_head list;
//	struct key_map map;
//};

enum IR_SPEED_LEVEL_e MIRC_Get_IRSpeed_Level(void);
void MIRC_Set_IRSpeed_Level(enum IR_SPEED_LEVEL_e level);
enum IR_DBG_LEVEL_e MIRC_Get_IRDBG_Level(void);
void MIRC_Set_IRDBG_Level(enum IR_DBG_LEVEL_e level);
u64 MIRC_Get_Protocols(void);
void MIRC_Set_Protocols(u64 data);
void MIRC_Send_KeyEvent(u8 flag);
unsigned long MIRC_Get_LastKey_Time(void);
unsigned long MIRC_Get_System_Time(void);
bool MIRC_IsEnable_WhiteBalance(void);
void MIRC_Set_WhiteBalance_Enable(bool bEnable);
void MIRC_IR_Config_Dump(void);
void MIRC_Set_IR_Enable(enum IR_Type_e eIRType, u32 u32HeadCode, u8 u8Enable);
int MIRC_Data_Store(struct mtk_ir_dev *dev, struct ir_raw_data *ev);
void MIRC_Data_Wakeup(struct mtk_ir_dev *dev);
int MIRC_Data_Ctrl_Init(struct mtk_ir_dev *dev);
void MIRC_Data_Ctrl_DeInit(struct mtk_ir_dev *dev);
int MIRC_Decoder_Register(struct ir_decoder_handler *handler);
void MIRC_Decoder_UnRegister(struct ir_decoder_handler *handler);
int MIRC_IRCustomer_Config(struct IR_Profile_s *stIr, u8 num);
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
unsigned long MIRC_Get_Event_Timeout(void);
void MIRC_Set_Event_Timeout(unsigned long time);
void MIRC_Timer_Proc(struct timer_list *t);
int MIRC_Map_Register(struct key_map_list *map);
u32 MIRC_Get_Keycode(u32 keymapnum, u32 scancode);
void MIRC_Map_UnRegister(struct key_map_list *map);
void MIRC_Keyup(struct mtk_ir_dev *dev);
void MIRC_Keydown(struct mtk_ir_dev *dev, int scancode);
#endif
#endif
