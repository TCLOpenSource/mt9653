/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#ifndef _IR_TS_H
#define _IR_TS_H
enum IR_TIME_STAGE {
	MDRV_IR_ISR = 0,
	MIRC_DATA_STORE,
	MIRC_Data_WAKEUP,
	MIRC_DATA_CTRL_THREAD,
	MIRC_DATA_GET,
	MIRC_DECODE_LIST_ENTRY,
	MIRC_DIFF_PROTOCOL,
	MIRC_DATA_MATCH_SHOTCOUNT,
	MIRC_SEND_KEY_EVENT,
	MIRC_KEYCODE_FROM_MAP,
};
enum START_END {
	TIME_START = 0,
	TIME_END,
};
struct time_range {
	unsigned long start_time;
	unsigned long end_time;
	unsigned long diff_time;
};
struct time_stage {
	unsigned int keycode;
	u8 flag;
	struct time_range time_MDrv_IR_ISR;
	struct time_range time_MIRC_Data_Get;
	struct time_range time_MIRC_Decode_list_entry;
	struct time_range time_MIRC_Diff_protocol;
	struct time_range time_MIRC_Keycode_From_Map;
	struct time_range time_MIRC_Data_Ctrl_Thread;
	struct time_range time_MIRC_Data_Store;
	struct time_range time_MIRC_Data_Match_shotcount;
	struct time_range time_MIRC_Send_key_event;
	struct time_range time_MIRC_Data_Wakeup;
	struct list_head list;
};
void gettime(int flag, u8 stage);
#endif
