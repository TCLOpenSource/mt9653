/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_EARC_H
#define MTK_EARC_H
#include <linux/platform_device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <linux/mtk-v4l2-earc.h>
#include "../mtk_earc_sysfs.h"

#define MTK_EARC_NAME "mtk_earc"
#define EARC_LABEL "[EARC] "

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif


/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

#define EARC_EVENTQ_SIZE (8)
#define EARC_NSEC_PER_USEC (1000)

#define LOG_LEVEL_MASK    (0xffff)
#define LOG_LEVEL_NONE	  (0)
#define LOG_LEVEL_NORMAL  (1)
#define LOG_LEVEL_FATAL   (2)
#define LOG_LEVEL_ERROR   (3)
#define LOG_LEVEL_WARNING (4)
#define LOG_LEVEL_INFO    (5)
#define LOG_LEVEL_DEBUG   (6)

#define IOREMAP_OFFSET_SIZE			2
#define IOREMAP_REG_INFO_OFFSET		2
#define IOREMAP_REG_PA_SIZE_IDX		3

#define EARC_UNSPECIFIED_NODE_NUM	0xFF

extern int log_level;
#define EARC_LOG(_dbgLevel_, _fmt, _args...) \
do { \
	switch (_dbgLevel_) { \
	case LOG_LEVEL_FATAL: \
	if ((log_level & LOG_LEVEL_MASK) == LOG_LEVEL_NORMAL)  \
		pr_crit(EARC_LABEL "[fatal] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	else if ((log_level & LOG_LEVEL_MASK) >= LOG_LEVEL_FATAL) \
		pr_emerg(EARC_LABEL "[fatal] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	case LOG_LEVEL_ERROR: \
	if ((log_level & LOG_LEVEL_MASK) == LOG_LEVEL_NORMAL)  \
		pr_err(EARC_LABEL "[error] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	else if ((log_level & LOG_LEVEL_MASK) >= LOG_LEVEL_ERROR) \
		pr_emerg(EARC_LABEL "[error] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	case LOG_LEVEL_WARNING: \
	if ((log_level & LOG_LEVEL_MASK) == LOG_LEVEL_NORMAL)  \
		pr_warn(EARC_LABEL "[warning] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	else if ((log_level & LOG_LEVEL_MASK) >= LOG_LEVEL_WARNING) \
		pr_emerg(EARC_LABEL "[warning] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	case LOG_LEVEL_DEBUG: \
	if ((log_level & LOG_LEVEL_MASK) == LOG_LEVEL_NORMAL)  \
		pr_debug(EARC_LABEL "[debug] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	else if ((log_level & LOG_LEVEL_MASK) >= LOG_LEVEL_DEBUG) \
		pr_emerg(EARC_LABEL "[debug] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	case LOG_LEVEL_INFO: \
	if ((log_level & LOG_LEVEL_MASK) == LOG_LEVEL_NORMAL)  \
		pr_info(EARC_LABEL "[info] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	else if ((log_level & LOG_LEVEL_MASK) >= LOG_LEVEL_INFO) \
		pr_emerg(EARC_LABEL "[info] [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	default: \
	break; \
	} \
} while (0)

#define EARC_MSG_DEBUG(format, args...) \
			EARC_LOG(LOG_LEVEL_DEBUG, format, ##args)
#define EARC_MSG_INFO(format, args...) \
			EARC_LOG(LOG_LEVEL_INFO, format, ##args)
#define EARC_MSG_WARNING(format, args...) \
			EARC_LOG(LOG_LEVEL_WARNING, format, ##args)
#define EARC_MSG_ERROR(format, args...) \
			EARC_LOG(LOG_LEVEL_ERROR, format, ##args)
#define EARC_MSG_FATAL(format, args...) \
			EARC_LOG(LOG_LEVEL_FATAL, format, ##args)

#define EARC_MSG_POINTER_CHECK() \
		do { \
			pr_err(EARC_LABEL"[%s,%d] pointer is NULL\n", __func__, __LINE__); \
			dump_stack(); \
		} while (0)

#define EARC_MSG_RETURN_CHECK(result) \
			pr_err(EARC_LABEL"[%s,%d] return is %d\n", __func__, __LINE__, (result))

#define EARC_GET_TIMESTAMP(tv) \
		do { \
			struct timespec ts; \
			ktime_get_ts(&ts); \
			((struct timeval *)tv)->tv_sec = ts.tv_sec; \
			((struct timeval *)tv)->tv_usec = ts.tv_nsec / EARC_NSEC_PER_USEC; \
		} while (0)

struct earc_clock {
	struct clk *earc_audio2earc_dmacro;
	struct clk *earc_audio2earc;        //EN
	struct clk *earc_cm2earc;           //EN
	struct clk *earc_debounce2earc;     //EN
	struct clk *earc_dm_prbs2earc;      //EN
	struct clk *earc_atop_txpll_ck;     //FIX
	struct clk *earc_atop_audio_ck;     //FIX
	struct clk *hdmirx_earc_debounce_int_ck;   //MUX
	struct clk *hdmirx_earc_dm_prbs_int_ck;    //MUX
	struct clk *hdmirx_earc_cm_int_ck;         //MUX
	struct clk *hdmirx_earc_audio_int_ck;      //MUX
	struct clk *earc_earc_audio_int_ck;      //MUX
};



struct mtk_earc_dev {
	struct device *dev;
	struct v4l2_device v4l2_dev;
	struct v4l2_m2m_dev *mdev;
	struct video_device *vdev;
	u16 dev_id;
	struct kobject *mtkdbg_kobj;
	struct kobject *rddbg_kobj;
	struct earc_clock st_earc_clk;
	struct v4l2_ctrl_handler earc_ctrl_handler;
	//struct earc_info earc_info;

	struct mutex mutex;
	struct clk *clk;
	spinlock_t lock;
	void __iomem	**reg_base;
	u64 *pIoremap;
	int earc_port_sel;
	int earc_bank_num;
	int earc_test;
	int earc_fixed_dd_index;
	int earc_support_mode;
	int earc_hw_ver_1;
	int earc_hw_ver_2;
	u8 reg_info_offset;
	u8 reg_pa_idx;
	u8 reg_pa_size_idx;
	struct task_struct *earc_event_task;
	struct mtk_earc_info earc_info;
	//int irq_earc; //CY TODO
};
struct mtk_earc_ctx {
	struct v4l2_fh fh;
	struct mtk_earc_dev *earc_dev;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */

int mtk_earc_sysfs_init(struct mtk_earc_dev *earc_dev);
int mtk_earc_sysfs_deinit(struct mtk_earc_dev *earc_dev);

#endif  //#ifndef MTK_EARC_H

