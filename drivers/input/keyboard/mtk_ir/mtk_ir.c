// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author jefferry.yen <jefferry.yen@mediatek.com>
 */

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/ktime.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include "mtk_devid.h"
#include "mtk_ir.h"
#include "ir_core.h"
#include "ir_common.h"
#include "mtk_ir_reg.h"
#include "mtk_ir_io.h"
#include "ir_config.h"
#include "ir_ts.h"
#include "ir-mt58xx-riu.h"
#define IR_GET_KEYMAP_FROM_DTS     1// 1
#define IR_RESUME_SEND_WAKEUPKEY     0// 1
#if IR_RESUME_SEND_WAKEUPKEY
#include <soc/mediatek/mtk-pm.h>
#define RC_MAGIC                    0x807f46
#endif
#define BIT15                       0x00008000
#define BUF_64                       64
#define BUF_33                       33
#define BUF_40                       40

#define REG_IR_CLEAN_FIFO_BIT	BIT15

#define MTK_IR_DEVICE_NAME    "MTK Smart TV IR Receiver"
#define MTK_IR_DRIVER_NAME    "MTK_IR"
#define MOD_IR_DEVICE_COUNT     1

#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
#define INPUTID_VENDOR 0x3697UL
#define INPUTID_PRODUCT 0x0001
#define INPUTID_VERSION 0x0001
#endif

#define IR_DTS_NAME_SIZE 50
#define MAX_PROFILE_NUM 10
#define MAX_KEYMAP_SIZE 512

static unsigned int profile_num = 0;
struct key_map_list *key_map_listn = NULL;
char dts_str[IR_DTS_NAME_SIZE];
struct key_map_table *table[MAX_PROFILE_NUM];
static struct IR_Profile_s ir_dts_config[MAX_PROFILE_NUM];

#if IR_GET_KEYMAP_FROM_DTS
//get IR keymap frome DTS
static int mtk_ir_get_keymap_from_dts(struct device_node *np)
{
	struct device_node *np_ir_key;
	unsigned int *ir_keymap = NULL;
	int ret = 0;
	int i = 0;
	uint32_t u32headcode_Tmp = 0x0;
	uint32_t u32protocol_Tmp = 0x0;
	uint32_t keymap_size = 0x0;
	unsigned int profile_index = 0;

	IR_PRINT("#####mtk_ir_get_keymap_from_dts#####\n");

	ir_keymap = kzalloc(sizeof(unsigned int) * MAX_KEYMAP_SIZE, GFP_KERNEL);
	if ((ir_keymap == NULL)) {
		IRDBG_ERR("%s: kzalloc ir_keymap fail!\n", __func__);
		goto error_kzalloc_fail;
	}

	key_map_listn = kzalloc(sizeof(struct key_map_list) * MAX_PROFILE_NUM, GFP_KERNEL);
	if ((key_map_listn == NULL)) {
		IRDBG_ERR("%s: kzalloc key_map_listn fail!\n", __func__);
		goto error_kzalloc_list_fail;
	}

	for (profile_index = 0; profile_index < MAX_PROFILE_NUM; profile_index++) {
		memset(dts_str, 0, sizeof(dts_str));
		ret = snprintf(dts_str, IR_DTS_NAME_SIZE, "ir_keymap%d", profile_index);
		if (ret < 0) {
			IRDBG_ERR("snprintf failed at %d: %d\n", profile_index, ret);
			goto fail_get_dt_property;
		}
		np_ir_key =  of_find_node_by_name(np, dts_str);
		if (np_ir_key == NULL) {
			IRDBG_ERR("[%s, %d]: find keymap node failed, name = %s\n",
				__func__, __LINE__, dts_str);
			goto fail_get_dt_property;
		}

		IR_PRINT("====================================================================\n");
		ret = of_property_read_u32(np_ir_key, "protocol", &u32protocol_Tmp);
		if (ret != 0x0) {
			IRDBG_ERR("[%s, %d]: read protocol failed, name = %s\n",
			__func__, __LINE__, dts_str);
			goto fail_get_dt_property;
		} else 
			IR_PRINT("[%s]:protocol =0x%x\n",dts_str,u32protocol_Tmp);


		ret = of_property_read_u32(np_ir_key, "customercode", &u32headcode_Tmp);
		if (ret != 0x0) {
			IRDBG_ERR("[%s, %d]: read customercode failed, name = %s\n",
			__func__, __LINE__, dts_str);
			goto fail_get_dt_property;
		} else 
			IR_PRINT("[%s]:customercode =0x%x\n",dts_str,u32headcode_Tmp);


		//get IR keymap and keymap size frome DTS
		keymap_size = of_property_read_variable_u32_array(np_ir_key,"ir_key_table",
								  ir_keymap, 2, MAX_KEYMAP_SIZE);
		keymap_size = keymap_size/2;
		if (keymap_size == 0) {
			IRDBG_ERR("%s: keymap_size fail, name = %s!!!\n", __func__, dts_str);
			goto fail_get_dt_property;
		}
		IR_PRINT("[%s]:keymap_size =%d\n",dts_str,keymap_size);



		table[profile_index] = kmalloc(sizeof(struct key_map_table) * keymap_size, GFP_KERNEL);
		if ((table[profile_index] == NULL)) {
			IRDBG_ERR("%s: kmalloc table fail, name = %s!!!\n", __func__, dts_str);
			goto fail_get_dt_property;
		}
		memset(table[profile_index], 0, sizeof(struct key_map_table) * keymap_size);
		for (i = 0; i < keymap_size ; i++) {
			table[profile_index][i].keycode =ir_keymap[i*2];
			table[profile_index][i].scancode =ir_keymap[i*2+1];
			IR_PRINT("==>[table] [%d](0x%x,0x%x)\n",i,
				table[profile_index][i].keycode,table[profile_index][i].scancode);
		}

		key_map_listn[profile_index].map.scan = table[profile_index] ;
		key_map_listn[profile_index].map.headcode = u32headcode_Tmp;
		key_map_listn[profile_index].map.size = keymap_size;
		key_map_listn[profile_index].map.name = dts_str;
		
		ir_dts_config[profile_index].eIRType = u32protocol_Tmp;
		ir_dts_config[profile_index].u32HeadCode = u32headcode_Tmp;
		ir_dts_config[profile_index].u32IRSpeed = 0;
		ir_dts_config[profile_index].u8Enable =1;
		MIRC_Map_Register(&key_map_listn[profile_index]);

		profile_num++;
	}
error_kzalloc_list_fail:
fail_get_dt_property:
	if (ir_keymap)
		kfree(ir_keymap);

error_kzalloc_fail:
	IR_PRINT("end %s\n", __func__);
	return 0;
}
#endif

static inline u16 ir_read(void *reg)
{
	return mtk_readw_relaxed(reg);
}

static inline void ir_write(void *reg, uint16_t value)
{
	mtk_writew_relaxed(reg, value);
}

//IR Debug level for customer setting
static enum IR_DBG_LEVEL_e ir_dbglevel = IR_DBG_ERR;

//IR Speed level for customer setting
static enum IR_SPEED_LEVEL_e ir_speed = IR_SPEED_FAST_H;

static DEFINE_SPINLOCK(ts_lock);

static void free_list_time_stage(void)
{

	struct	list_head *pos, *q;

	spin_lock(&ts_lock);
	list_for_each_safe(pos, q, &stage_List) {
		struct time_stage *ts = NULL;

		ts = list_entry(pos, struct time_stage, list);
		list_del_init(pos);
		kfree(ts);
	}
	spin_unlock(&ts_lock);
}


/*******************  Global Variable  Start ******************/
static u8 ir_irq_enable =
	0xFF; //for record irq enable status
static u8 ir_irq_sel =
	0xFF;	//for record irq type (IR_RC or IR_ALL)
static pid_t MasterPid;
static enum IR_Mode_e ir_config_mode =
	IR_TYPE_MAX_MODE;
//for fulldecode ir headcode config
u32 u32IRHeaderCode[2] = {IR_DEFAULT_CUSTOMER_CODE0, IR_DEFAULT_CUSTOMER_CODE1};
//for record repeat times
static u8 u8RepeatCount;
//for record get ir key time
static u32 u32KeyTime;
static spinlock_t  irq_read_lock;

/*******************  Global Variable  End ******************/

static int _mod_ir_open(struct inode *inode,
						  struct file *filp);
static int _mod_ir_release(struct inode *inode,
							struct file *filp);
static ssize_t _mod_ir_read(struct file *filp,
						char __user *buf, size_t count, loff_t *f_pos);
static ssize_t _mod_ir_write(struct file *filp,
						const char __user *buf, size_t count,
						loff_t *f_pos);
static unsigned int _mod_ir_poll(struct file *
								 filp, poll_table *wait);
#ifdef HAVE_UNLOCKED_IOCTL
static long _mod_ir_ioctl(struct file *filp,
						  unsigned int cmd, unsigned long arg);
#else
static int _mod_ir_ioctl(struct inode *inode,
						 struct file *filp, unsigned int cmd,
						 unsigned long arg);
#endif
#ifdef CONFIG_COMPAT
static long _mod_ir_compat_ioctl(struct file *
						filp, unsigned int cmd, unsigned long arg);
#endif

static int						_mod_ir_fasync(
	int fd, struct file *filp, int mode);

struct IRModHandle IRDev = {
	.s32IRMajor =			   MDRV_MAJOR_IR,
	.s32IRMinor =			   MDRV_MINOR_IR,
	.cDevice = {
		.kobj =					{.name = MDRV_NAME_IR, },
		.owner =				THIS_MODULE,
	},
	.IRFop = {
		.open =					_mod_ir_open,
		.release =				_mod_ir_release,
		.read =					_mod_ir_read,
		.write =					_mod_ir_write,
		.poll =					_mod_ir_poll,
#ifdef HAVE_UNLOCKED_IOCTL
		.unlocked_ioctl =		_mod_ir_ioctl,
#else
		.ioctl =				_mod_ir_ioctl,
#endif
#ifdef CONFIG_COMPAT
		.compat_ioctl =			_mod_ir_compat_ioctl,
#endif
		.fasync =				_mod_ir_fasync,
	},
};

static void _Mdrv_IR_SetHeaderCode(
	u32 u32Headcode0, u32 u32Headcode1)
{
	struct IRModHandle *mtk_dev = &IRDev;
	u8 i = 0;
	u8 index = 0;

	u32IRHeaderCode[0] = u32Headcode0;
	u32IRHeaderCode[1] = u32Headcode1;

	if (mtk_dev->pIRDev->ir_mode ==
		IR_TYPE_FULLDECODE_MODE) {
		if ((u32Headcode0 != 0xFFFF)
		   && (u32Headcode0 != 0x0000)) {
			//set register for full mode decoder0
			ir_write(REG_IR_CCODE, ((u32IRHeaderCode[0] & 0xff) << 8)
							|((u32IRHeaderCode[0] >> 8) & 0xff));
		}
		if ((u32Headcode1 != 0xFFFF)
		   && (u32Headcode1 != 0x0000)) {
			//set register for full mode decoder1
			ir_write(REG_IR_CCODE1, ((u32IRHeaderCode[1] & 0xff)
							<< 8)|((u32IRHeaderCode[1] >> 8) & 0xff));
		}
	}

	for (i = 0; i < mtk_dev->pIRDev->support_num;
		i++) {
		if (mtk_dev->pIRDev->support_ir[i].eIRType ==
		   IR_TYPE_NEC) {
			if ((u32IRHeaderCode[index] != 0xFFFF)
			   && (u32IRHeaderCode[index] != 0x0000)) {
				mtk_dev->pIRDev->support_ir[i].u32HeadCode =
					u32IRHeaderCode[index];
			}
			index++;
			if (index == 2)
				return;
		}

	}
	IRDBG_WRN("Warnning: ioctl: MDRV_IR_SET_HEADER only support NEC type,please check\n");
}

/*=======================IR	 Setting Functions========================*/

static void mtk_ir_nec_clearfifo(void)
{
	unsigned long i;

	for (i = 0; i < 16;
		 i++) { //FIFO maxinum depth is 16 bytes(typically, SW decode mode will use 16bytes)
		unsigned char u8Garbage;

		if (ir_read(REG_IR_SHOT_CNT_H_FIFO_STATUS) &
			IR_FIFO_EMPTY)
			break;

		u8Garbage = ir_read(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
		ir_write(REG_IR_FIFO_RD_PULSE, ir_read(REG_IR_FIFO_RD_PULSE) | 0x0001UL);
	}
}
static void mtk_ir_rc_clearfifo(void)
{
	unsigned long i;

	for (i = 0; i < 8; i++) {
		unsigned char u8Garbage;

		if (ir_read(REG_IR_RC_KEY_FIFO_STATUS) &
			IR_RC_FIFO_EMPTY)
			break;

		u8Garbage = ir_read(REG_IR_RC_KEY_COMMAND_ADD) >> 8;
		ir_write(REG_IR_RC_FIFO_RD_PULSE,
					ir_read(REG_IR_RC_FIFO_RD_PULSE) | 0x0001UL); //read
	}
}
static void _MDrv_IR_Timing(void)
{
	// header code upper bound
	ir_write(REG_IR_HDC_UPB, IR_HDC_UPB);

	// header code lower bound
	ir_write(REG_IR_HDC_LOB, IR_HDC_LOB);

	// off code upper bound
	ir_write(REG_IR_OFC_UPB, IR_OFC_UPB);

	// off code lower bound
	ir_write(REG_IR_OFC_LOB, IR_OFC_LOB);

	// off code repeat upper bound
	ir_write(REG_IR_OFC_RP_UPB, IR_OFC_RP_UPB);

	// off code repeat lower bound
	ir_write(REG_IR_OFC_RP_LOB, IR_OFC_RP_LOB);

	// logical 0/1 high upper bound
	ir_write(REG_IR_LG01H_UPB, IR_LG01H_UPB);

	// logical 0/1 high lower bound
	ir_write(REG_IR_LG01H_LOB, IR_LG01H_LOB);

	// logical 0 upper bound
	ir_write(REG_IR_LG0_UPB, IR_LG0_UPB);

	// logical 0 lower bound
	ir_write(REG_IR_LG0_LOB, IR_LG0_LOB);

	// logical 1 upper bound
	ir_write(REG_IR_LG1_UPB, IR_LG1_UPB);

	// logical 1 lower bound
	ir_write(REG_IR_LG1_LOB, IR_LG1_LOB);

	// timeout cycles
	ir_write(REG_IR_TIMEOUT_CYC_L, IR_RP_TIMEOUT & 0xFFFFUL);
}

static void mtk_ir_hw_fulldecode_config(void)
{
	//1. set customer code0
	ir_write(REG_IR_CCODE, ((u32IRHeaderCode[0] & 0xff) << 8)
				| ((u32IRHeaderCode[0] >> 8) & 0xff));

	//2. set customer code1
	ir_write(REG_IR_CCODE1, ((u32IRHeaderCode[1] & 0xff) << 8)
						| ((u32IRHeaderCode[1] >> 8) & 0xff));

	if (ir_config_mode == IR_TYPE_FULLDECODE_MODE)
		return;
	ir_config_mode = IR_TYPE_FULLDECODE_MODE;
	IR_PRINT("##### MTK IR HW Full Decode Config #####\n");
//Prevent residual value exist in sw fifo when system wakeup from standby status
	ir_write(REG_IR_CTRL, ir_read(REG_IR_CTRL) & (0xFF00));//disable IR ctrl regs

	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL)
		 | REG_IR_CLEAN_FIFO_BIT); //clear IR FIFO(Trigger reg_ir_fifo_clr)

	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL)
		 & (0xFF));//disable IR FIFO settings

//3. set NEC timing & FIFO depth 16 bytes
	_MDrv_IR_Timing();

	//[10:8]: FIFO depth, [11]:Enable FIFO full
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, 0xF00UL);

	ir_write(REG_IR_CCODE1_CHK_EN, ir_read(REG_IR_CCODE1_CHK_EN) | IR_CCODE1_CHK_EN);

//4. Glitch Remove (for prevent Abnormal waves)
//	 reg_ir_glhrm_num = 4£¬reg_ir_glhrm_en = 1
	ir_write(REG_IR_GLHRM_NUM, 0x804UL);

//5.set full decode mode
	ir_write(REG_IR_GLHRM_NUM, ir_read(REG_IR_GLHRM_NUM) | (0x3UL << 12));

//6.set Data bits ===>two byte customer code +IR DATA bits(32bits)+timeout clear
//+timeout value 4 high bits

	ir_write(REG_IR_TIMEOUT_CYC_H_CODE_BYTE, IR_CCB_CB |
		 0x30UL | ((IR_RP_TIMEOUT >> 16) & 0x0FUL));

//7.set IR clock DIV
	//set clock div --->IR clock =1MHZ
	ir_write(REG_IR_CKDIV_NUM_KEY_DATA, IR_CKDIV_NUM);

//8.reg_ir_wkup_key_sel ---> out key value
	//wakeup key sel
	ir_write(REG_IR_FIFO_RD_PULSE, ir_read(REG_IR_FIFO_RD_PULSE) | 0x0020UL);
//9. set ctrl enable IR
	ir_write(REG_IR_CTRL, IR_TIMEOUT_CHK_EN |
						IR_INV			 |
						IR_RPCODE_EN		 |
						IR_LG01H_CHK_EN	 |
						IR_DCODE_PCHK_EN	 |
						IR_CCODE_CHK_EN	 |
						IR_LDCCHK_EN		 |
						IR_EN);
	mtk_ir_nec_clearfifo();
}

int mtk_ir_isr_getdata_fulldecode(
	struct IRModHandle *mtk_dev)
{
	int ret = 0;
	u8 u8Ir_Index = 0;
	u8 u8Keycode = 0;
	u8 u8Repeat = 0;
	u32 u32IRSpeed = 0;
	DEFINE_IR_RAW_DATA(ev);

	if (ir_read(REG_IR_SHOT_CNT_H_FIFO_STATUS) &
	   IR_FIFO_EMPTY) {
		return -1;
	}


	u8Keycode = ir_read(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
	u8Repeat = (ir_read(REG_IR_SHOT_CNT_H_FIFO_STATUS) &
		    IR_RPT_FLAG) ? 1 : 0;
	u8Ir_Index = ir_read(REG_IR_SHOT_CNT_H_FIFO_STATUS) &
			     0x40 ? 1 : 0;//IR remote controller TX1 send flag
	ir_write(REG_IR_FIFO_RD_PULSE, ir_read(REG_IR_FIFO_RD_PULSE) | 0x0001UL);

	ev.duration = (u32IRHeaderCode[u8Ir_Index] << 8) |
		       u8Keycode;
	ev.pulse = u8Repeat;
	IRDBG_INFO(" u8Ir_Index = %d\n", u8Ir_Index);
	IRDBG_INFO("[Full Decode] Headcode =%x Key =%x\n",
			   u32IRHeaderCode[u8Ir_Index], u8Keycode);
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	mtk_dev->pIRDev->map_num =
		mtk_dev->pIRDev->support_ir[u8Ir_Index].u32HeadCode;
#endif

	u32IRSpeed =
		mtk_dev->pIRDev->support_ir[u8Ir_Index].u32IRSpeed;
	if (u8Repeat) {
		if (((u32IRSpeed != 0)
			 && (u8RepeatCount < (u32IRSpeed - 1)))
			|| ((u32IRSpeed == 0)
				&& (u8RepeatCount < mtk_dev->pIRDev->speed))) {
			u8RepeatCount++;
			mtk_ir_nec_clearfifo();
			return -1;
		}
	} else
		u8RepeatCount = 0;

	ret = MIRC_Data_Store(mtk_dev->pIRDev, &ev);
	if (ret < 0)
		IRDBG_ERR("Store IR data Error!\n");
	mtk_ir_nec_clearfifo();
	return ret;
}

static void mtk_ir_hw_rawdecode_config(void)
{
	if (ir_config_mode == IR_TYPE_RAWDATA_MODE)
		return;
	ir_config_mode = IR_TYPE_RAWDATA_MODE;
	IR_PRINT("##### MTK IR HW RAW Decode Config #####\n");
	//Prevent residual value exist in sw fifo when system wakeup from standby status

	//disable IR ctrl regs
	ir_write(REG_IR_CTRL, ir_read(REG_IR_CTRL) & (0xFF00));

	//clear IR FIFO(Trigger reg_ir_fifo_clr)
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL,
	ir_read(REG_IR_SEPR_BIT_FIFO_CTRL) | REG_IR_CLEAN_FIFO_BIT);

	//disable IR FIFO settings
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL,
	ir_read(REG_IR_SEPR_BIT_FIFO_CTRL) & (0xFF));

//1. set NEC timing	 & FIFO depth 16 bytes
	_MDrv_IR_Timing();
	//[10:8]: FIFO depth, [11]:Enable FIFO full
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, 0xF00UL);

//2. Glitch Remove (for prevent Abnormal waves)	 reg_ir_glhrm_num = 4£¬reg_ir_glhrm_en = 1
	ir_write(REG_IR_GLHRM_NUM, 0x804UL);
//3.set RAW decode mode
	ir_write(REG_IR_GLHRM_NUM, ir_read(REG_IR_GLHRM_NUM) | (0x2UL << 12));

//4.reg_ir_wkup_key_sel ---> out key value
	//wakeup key sel
	ir_write(REG_IR_FIFO_RD_PULSE, ir_read(REG_IR_FIFO_RD_PULSE) | 0x0020UL);

//5.set IR clock DIV
	//set clock div --->IR clock =1MHZ
	ir_write(REG_IR_CKDIV_NUM_KEY_DATA, IR_CKDIV_NUM);

//6.set Data bits ===>two byte customer code
// +IR DATA bits(32bits)+timeout clear+timeout value 4 high bits
	ir_write(REG_IR_TIMEOUT_CYC_H_CODE_BYTE, IR_CCB_CB |
		 0x30UL | ((IR_RP_TIMEOUT >> 16) & 0x0FUL));

//7. set ctrl enable IR
	ir_write(REG_IR_CTRL, IR_TIMEOUT_CHK_EN |
						IR_INV			 |
						IR_RPCODE_EN		 |
						IR_LG01H_CHK_EN	 |
						IR_LDCCHK_EN		 |
						IR_EN);
	mtk_ir_nec_clearfifo();
}

int mtk_ir_isr_getdata_rawdecode(
	struct IRModHandle *mtk_dev)
{
	int j = 0, i = 0;
	unsigned char  _u8IRRawModeBuf[4] = {0};
	int ret = -1;
	DEFINE_IR_RAW_DATA(ev);
	static u8 speed;

	ev.pulse = 1;
	for (j = 0; j < 4; j++) {
		if (ir_read(REG_IR_SHOT_CNT_H_FIFO_STATUS) &
			 IR_FIFO_EMPTY)	 // check FIFO empty
			break;
		_u8IRRawModeBuf[j] = ir_read(REG_IR_CKDIV_NUM_KEY_DATA) >> 8;
		ir_write(REG_IR_FIFO_RD_PULSE, ir_read(REG_IR_FIFO_RD_PULSE) | 0x0001UL);
		udelay(10);
		//For Delay
	}
	IRDBG_INFO("[Raw Decode] Headcode =%x Key =%x\n",
			   (_u8IRRawModeBuf[0] << 8 | _u8IRRawModeBuf[1]),
			   _u8IRRawModeBuf[2]);
	for (i = 0; i < mtk_dev->pIRDev->support_num; i++) {
		if ((_u8IRRawModeBuf[0] == (((u8)((
						mtk_dev->pIRDev->support_ir[i].u32HeadCode) >>
						8)) & 0xff))
		   && (_u8IRRawModeBuf[1] == (((u8)(
					mtk_dev->pIRDev->support_ir[i].u32HeadCode))
					& 0xff))) {
			if (_u8IRRawModeBuf[2] == (u8)(
					~_u8IRRawModeBuf[3])) {
				static u8 repeatcount;

				if ((u32)(MIRC_Get_System_Time()-u32KeyTime) <=
				   NEC_REPEAT_TIMEOUT) {
					ev.pulse = 1;
					u32KeyTime = MIRC_Get_System_Time();
					if (((speed != 0) && (repeatcount >= speed))
						|| ((speed == 0)
						&& (repeatcount >= mtk_dev->pIRDev->speed))) {
						mtk_dev->pIRDev->filter_flag = 0;
					} else {
						repeatcount++;
						IRDBG_INFO("repeat filter ====\n");
						mtk_dev->pIRDev->filter_flag = 1;
					}
				} else {
					u32KeyTime = MIRC_Get_System_Time();
					speed = mtk_dev->pIRDev->support_ir[i].u32IRSpeed;
					ev.pulse = 0;
					repeatcount = 0;
					mtk_dev->pIRDev->filter_flag = 0;
				}
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
				mtk_dev->pIRDev->map_num =
					mtk_dev->pIRDev->support_ir[i].u32HeadCode;
#endif
				ev.duration =
					(mtk_dev->pIRDev->support_ir[i].u32HeadCode<<8)
					| _u8IRRawModeBuf[2];

				ret = MIRC_Data_Store(mtk_dev->pIRDev, &ev);
				if (ret < 0)
					IRDBG_ERR("Store IR data Error!\n");
				mtk_ir_nec_clearfifo();
				return ret;
			}
		}
	}
	IRDBG_ERR("NO IR Matched!!!\n");
	mtk_ir_nec_clearfifo();
	return ret;
}

void mtk_ir_hw_rc5decode_config(void)
{
	if (ir_config_mode == IR_TYPE_HWRC5_MODE)
		return;
	ir_config_mode = IR_TYPE_HWRC5_MODE;

	IR_PRINT("##### MTK IR HW RC5 Decode Config #####\n");
	//Prevent residual value exist in sw
	//fifo when system wakeup from standby status
	//disable IR ctrl regs
	ir_write(REG_IR_CTRL, ir_read(REG_IR_CTRL) & (0xFF00));

	//clear IR FIFO(Trigger reg_ir_fifo_clr)
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL,
		 ir_read(REG_IR_SEPR_BIT_FIFO_CTRL) | REG_IR_CLEAN_FIFO_BIT);

	//disable IR FIFO settings
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL) & (0xFF));


	//00 register set rc5 enable
	ir_write(REG_IR_RC_CTRL, IR_RC_EN);

	//01 register set rc5 long pulse threshold , IR head detect 889*2=1778
	ir_write(REG_IR_RC_LONGPULSE_THR, IR_RC5_LONGPULSE_THR);

	//02 register set rc6 long pulse margin
	// for rc6 only ,old version retain
	ir_write(REG_IR_RC_LONGPULSE_MAR, IR_RC5_LONGPULSE_MAR);

	//03 register set RC5 Integrator threshold * 8.
	//To judge whether it is 0 or 1. ++  RC CLK DIV
	ir_write(REG_IR_RC_CLK_INT_THR, (IR_RC_INTG_THR(
					IR_RC5_INTG_THR_TIME) << 1) + (IR_RC_CLK_DIV(
					IR_CLK) << 8));

	//04 register set RC5 watch dog counter and RC5 timeout counter
	ir_write(REG_IR_RC_WD_TIMEOUT_CNT, IR_RC_WDOG_CNT(
							IR_RC5_WDOG_TIME) + (IR_RC_TMOUT_CNT(
							IR_RC5_TIMEOUT_TIME) << 8));

	//05 register set rc5 power wakeup key1&key2
	ir_write(REG_IR_RC_COMP_KEY1_KEY2, 0xffffUL);

	//0A register set RC5 power wakeup address & enable power key wakeup
	ir_write(REG_IR_RC_CMP_RCKEY, IR_RC_POWER_WAKEUP_EN + IR_RC5_POWER_WAKEUP_KEY);

	//50 register set code bytes
	ir_write(REG_IR_TIMEOUT_CYC_H_CODE_BYTE, (IR_RC5_BITS << 8) |
		0x30UL);//[6:4]=011 : timeout be clear at Key Data Code check pass
	ir_write(REG_IR_CTRL, IR_INV);
	//ir_write(REG_IR_RC_FIFO_RD_PULSE, ir_read(REG_IR_RC_FIFO_RD_PULSE)| 0x1UL);
}

int mtk_ir_isr_getdata_rc5decode(
	struct IRModHandle *mtk_dev)
{
	u8 u8KeyAddr = 0;
	u8 u8KeyCmd = 0;
	u8 u8Flag = 0;
	u8 i = 0;
	int ret = -1;
	u32 prev_duration = 0;
	static u8 speed;
	static u8 repeatcount;
	DEFINE_IR_RAW_DATA(ev);


	u8KeyAddr = ir_read(REG_IR_RC_KEY_COMMAND_ADD)
				& 0x1f;//RC5: {2'b0,toggle,address[4:0]} reg[7:0]
	u8KeyCmd = (ir_read(REG_IR_RC_KEY_COMMAND_ADD) & 0x3f00)
			   >> 8;//RC5: {repeat,1'b0,command[13:8]} reg[15:8]
	u8Flag = (ir_read(REG_IR_RC_KEY_COMMAND_ADD) & 0x8000)
			 >> 15;//repeat
	ir_write(REG_IR_RC_FIFO_RD_PULSE, ir_read(REG_IR_RC_FIFO_RD_PULSE) | 0x1UL);
	IRDBG_INFO("[RC5] u8KeyAddr = %x  u8KeyCmd = %x	 repeat = %d .\n",
			   u8KeyAddr, u8KeyCmd, u8Flag);

	for (i = 0; i < mtk_dev->pIRDev->support_num;
		i++) {
		if (u8KeyAddr == ((u8)(
				 mtk_dev->pIRDev->support_ir[i].u32HeadCode))) {
			ev.duration = (u8KeyAddr << 8) | u8KeyCmd;
			ev.pulse = u8Flag;
			if ((u8Flag == 1)
			   && (ev.duration == prev_duration)) {
				if (((speed != 0) && (repeatcount >= speed))
					|| ((speed == 0)
						&& (repeatcount >= mtk_dev->pIRDev->speed)))
					mtk_dev->pIRDev->filter_flag = 0;
				else {
					repeatcount++;
					IRDBG_INFO("repeat filter ====\n");
					mtk_dev->pIRDev->filter_flag = 1;
				}
			} else {
				speed = mtk_dev->pIRDev->support_ir[i].u32IRSpeed;
				repeatcount = 0;
				mtk_dev->pIRDev->filter_flag = 0;
				prev_duration = ev.duration;
			}
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
			mtk_dev->pIRDev->map_num =
				mtk_dev->pIRDev->support_ir[i].u32HeadCode;
#endif
			ret = MIRC_Data_Store(mtk_dev->pIRDev, &ev);
			if (ret < 0)
				IRDBG_ERR("Store IR data Error!\n");
			mtk_ir_rc_clearfifo();
			return ret;
		}
	}
	mtk_ir_rc_clearfifo();

	return 0;
}

int mtk_ir_isr_getdata_rc5xdecode(
	struct IRModHandle *mtk_dev)
{
	u8 u8KeyAddr = 0;
	u8 u8KeyCmd = 0;
	u8 u8Flag = 0;

	u8KeyAddr = ir_read(REG_IR_RC_KEY_COMMAND_ADD)
				& 0x3f;//RC5: {2'b0,toggle,address[4:0]} reg[7:0]
	u8KeyCmd = (ir_read(REG_IR_RC_KEY_COMMAND_ADD) & 0x7f00)
			   >> 8;//RC5EXT: {repeat,command[6:0]}
	u8Flag = (ir_read(REG_IR_RC_KEY_COMMAND_ADD) & 0x8000)
			 >> 15;//repeat
	mtk_ir_rc_clearfifo();
	IRDBG_INFO("[RC5] u8KeyAddr = %x  u8KeyCmd = %x	 u8Flag = %d .\n",
			   u8KeyAddr, u8KeyCmd, u8Flag);
	return 0;
}

void mtk_ir_hw_rc6decode_config(void)
{
	if (ir_config_mode == IR_TYPE_HWRC6_MODE)
		return;
	ir_config_mode = IR_TYPE_HWRC6_MODE;

	IR_PRINT("##### MTK IR HW RC6 Decode Config #####\n");
	//Prevent residual value exist in sw fifo when system wakeup from standby status
	ir_write(REG_IR_CTRL, ir_read(REG_IR_CTRL) & (0xFF00));//disable IR ctrl regs

	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL)
		 | REG_IR_CLEAN_FIFO_BIT); //clear IR FIFO(Trigger reg_ir_fifo_clr)
	//disable IR FIFO settings
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL)
		 & (0xFF));//disable IR FIFO settings

//00 register set rc6 enable
	ir_write(REG_IR_RC_CTRL, IR_RC_AUTOCONFIG|IR_RC_EN|IR_RC6_EN|(((
					IR_RC6_LDPS_THR(IR_RC6_LEADPULSE) >> 5) & 0x1f)));

//01 register set rc6 long pulse threshold , IR head detect 444*2=889
	ir_write(REG_IR_RC_LONGPULSE_THR, IR_RC6_LONGPULSE_THR);

//02 register set rc6 long pulse margin
	ir_write(REG_IR_RC_LONGPULSE_MAR, (IR_RC6_LGPS_MAR(
					IR_RC6_LONGPULSE_MAR) & 0x1ff)|
					(((IR_RC6_LDPS_THR(IR_RC6_LEADPULSE) >> 8) & 0x1c)
					<<8));

//03 register set RC6 Integrator threshold * 8.
// To judge whether it is 0 or 1. ++  RC CLK DIV  ++RC6 ECO function enable
	ir_write(REG_IR_RC_CLK_INT_THR, (IR_RC_INTG_THR(
					IR_RC6_INTG_THR_TIME) << 1) + (IR_RC_CLK_DIV(
					IR_CLK) << 8) + IR_RC6_ECO_EN);


//04 register set RC6 watch dog counter and RC6 timeout counter
	ir_write(REG_IR_RC_WD_TIMEOUT_CNT, IR_RC_WDOG_CNT(
						IR_RC6_WDOG_TIME) + (IR_RC_TMOUT_CNT(
						IR_RC6_TIMEOUT_TIME) << 8));
	//05 register set rc6 power wakeup key1&key2
	ir_write(REG_IR_RC_COMP_KEY1_KEY2, 0xffffUL);//disable IR ctrl regs

	//0A register set RC5 power wakeup address & enable power key wakeup
	ir_write(REG_IR_RC_CMP_RCKEY, IR_RC_POWER_WAKEUP_EN +
							IR_RC6_POWER_WAKEUP_KEY);

	//50 register set code bytes
	ir_write(REG_IR_TIMEOUT_CYC_H_CODE_BYTE, (IR_RC6_BITS << 8) |
		0x30UL);//[6:4]=011 : timeout be clear at Key Data Code check pass

}

int mtk_ir_isr_getdata_rc6decode(
	struct IRModHandle *mtk_dev)
{
	u8 u8KeyAddr = 0;
	u8 u8KeyCmd = 0;
	u8 u8Flag = 0;
	u8 u8Mode = 0;

	u8KeyAddr = ir_read(REG_IR_RC_KEY_COMMAND_ADD)
				&0xff;//RC6_address
	u8KeyCmd = (ir_read(REG_IR_RC_KEY_COMMAND_ADD)&0xff00)
			   >>8;
	u8Flag = (ir_read(REG_IR_RC_KEY_MISC)&0x10)
			 >>4;//repeat
	u8Mode = ir_read(REG_IR_RC_KEY_MISC)&0x05;
	mtk_ir_rc_clearfifo();
	IRDBG_INFO("[RC6_MODE%d] u8KeyAddr = %x	 u8KeyCmd = %x	u8Flag = %d .\n",
			   u8Mode, u8KeyAddr, u8KeyCmd, u8Flag);
	return 0;
}

static void mtk_ir_sw_decode_config(void)
{
	if (ir_config_mode == IR_TYPE_SWDECODE_MODE)
		return;
	ir_config_mode = IR_TYPE_SWDECODE_MODE;

	IR_PRINT("##### MTK IR SW Decode Config V3.0  #####\n");
//Prevent residual value exist in sw fifo when system wakeup from standby status
	ir_write(REG_IR_CTRL, ir_read(REG_IR_CTRL) & (0xFF00));//disable IR ctrl regs

	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL)
		 | REG_IR_CLEAN_FIFO_BIT); //clear IR FIFO(Trigger reg_ir_fifo_clr)

	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL)
		 & (0xFF));//disable IR FIFO settings

//1. set ctrl enable IR
	ir_write(REG_IR_CTRL, IR_TIMEOUT_CHK_EN |
					   IR_INV			 |
					   IR_RPCODE_EN		 |
					   IR_LG01H_CHK_EN	 |
					   IR_DCODE_PCHK_EN	 |
					   IR_CCODE_CHK_EN	 |
					   IR_LDCCHK_EN		 |
					   IR_EN);
//2.set IR clock DIV
	ir_write(REG_IR_CKDIV_NUM_KEY_DATA, IR_CKDIV_NUM);
//3.set SW decode mode & Glitch Remove (for prevent Abnormal waves)
//	 reg_ir_glhrm_num = 4£¬reg_ir_glhrm_en = 1
	ir_write(REG_IR_GLHRM_NUM, 0x1804UL);
//4.set PN shot interrupt
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL) | 0x3UL << 12);

//5.set FIFO Depth to 64 bytes depth
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL) | 0x7UL << 8);

//6.set 1 shot_cnt 1 interrupt
	ir_write(REG_IR_SHOT_CNT_NUML, ir_read(REG_IR_SHOT_CNT_NUML) | 0x0UL << 2);
	ir_write(REG_IR_TIMEOUT_CYC_H_CODE_BYTE, ir_read(REG_IR_TIMEOUT_CYC_H_CODE_BYTE)
		 & (0xFF0F));

	//7.IR Timeout Clear Set
	//timeout be cleas at pshot or nshot edge
	ir_write(REG_IR_TIMEOUT_CYC_H_CODE_BYTE, ir_read(REG_IR_TIMEOUT_CYC_H_CODE_BYTE)
		 | 0x6UL << 4);

	//8.Disable IR Timeout Clear by SoftWare
	ir_write(REG_IR_TIMEOUT_CYC_H_CODE_BYTE, ir_read(REG_IR_TIMEOUT_CYC_H_CODE_BYTE)
		 | 0x0UL << 7);

	//8.IR SW FIFO enable
	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL)
		 | (0x1UL << 14));

	ir_write(REG_IR_CKDIV_NUM_KEY_DATA,  0x00CFUL);

	ir_write(REG_IR_SEPR_BIT_FIFO_CTRL, ir_read(REG_IR_SEPR_BIT_FIFO_CTRL) | (0x1UL << 6));

	mtk_ir_nec_clearfifo();
}
int mtk_ir_isr_getdata_swdecode(struct IRModHandle *mtk_dev)
{
	int ret = 0;
	DEFINE_IR_RAW_DATA(ev);

	ev.duration = ((ir_read(REG_IR_SHOT_CNT_H_FIFO_STATUS)
					&0xF) << 16) | ((ir_read(REG_IR_SHOT_CNT_L)) & 0xFFFF);
	ev.pulse = (ir_read(REG_IR_SHOT_CNT_H_FIFO_STATUS))
			   &0x10?false:true;
	ret = MIRC_Data_Store(mtk_dev->pIRDev, &ev);
	if (ret < 0)
		IRDBG_ERR("Store IR data Error!\n");
	return ret;
}

/*========================= end Register Setting Functions ==================*/

irqreturn_t _MDrv_IR_RC_ISR(int irq, void *dev_id)
{
	struct IRModHandle *mtk_dev = dev_id;
	int ret = 0;

	if (mtk_dev->pIRDev->ir_mode == IR_TYPE_HWRC5_MODE)
		ret = mtk_ir_isr_getdata_rc5decode(mtk_dev);
	else if (mtk_dev->pIRDev->ir_mode ==
			IR_TYPE_HWRC5X_MODE)
		ret = mtk_ir_isr_getdata_rc5xdecode(mtk_dev);
	else// for IR_TYPE_HWRC6_MODE
		ret = mtk_ir_isr_getdata_rc6decode(mtk_dev);

	if (ret >= 0)
		MIRC_Data_Wakeup(mtk_dev->pIRDev);
	return IRQ_HANDLED;
}

irqreturn_t _MDrv_IR_ISR(int irq, void *dev_id)
{
	struct IRModHandle *mtk_dev = dev_id;
	int ret = 0;

	if (mtk_dev == NULL)
		return -EINVAL;

	gettime(TIME_START, MDRV_IR_ISR);

	if (mtk_dev->pIRDev->ir_mode ==
	   IR_TYPE_FULLDECODE_MODE)
		ret = mtk_ir_isr_getdata_fulldecode(mtk_dev);
	else if (mtk_dev->pIRDev->ir_mode ==
			IR_TYPE_RAWDATA_MODE)
		ret = mtk_ir_isr_getdata_rawdecode(mtk_dev);
	else {
		spin_lock(&irq_read_lock);
		while (((ir_read(REG_IR_SHOT_CNT_H_FIFO_STATUS) &
				  IR_FIFO_EMPTY) != IR_FIFO_EMPTY)) {
			ret = mtk_ir_isr_getdata_swdecode(mtk_dev);
			if (ret < 0) {
				spin_unlock(&irq_read_lock);
				return IRQ_HANDLED;
			}
			ir_write(REG_IR_FIFO_RD_PULSE, ir_read(REG_IR_FIFO_RD_PULSE) | 0x0001);
		}
		spin_unlock(&irq_read_lock);
	}
	if (ret >= 0)
		MIRC_Data_Wakeup(mtk_dev->pIRDev);

	gettime(TIME_END, MDRV_IR_ISR);

	return IRQ_HANDLED;
}

static void mtk_ir_enable(u8 bEnableIR)
{
	struct mtk_ir_dev *dev = IRDev.pIRDev;

	if (dev == NULL)
		return;
	if (bEnableIR) {
		if (ir_irq_enable == 0) {
			mtk_ir_nec_clearfifo();
			mtk_ir_rc_clearfifo();

			enable_irq(ir_irq_base);

			if ((dev->ir_mode == IR_TYPE_HWRC5_MODE)
			   || (dev->ir_mode == IR_TYPE_HWRC5X_MODE)
			   || (dev->ir_mode == IR_TYPE_HWRC6_MODE))
				ir_write(REG_IR_RC_CTRL, ir_read(REG_IR_RC_CTRL) | IR_RC_EN);
			else
				ir_write(REG_IR_CTRL, ir_read(REG_IR_CTRL) | IR_EN);
			ir_irq_enable = 1;
		}
	} else {
		if (ir_irq_enable == 1) {
			if ((dev->ir_mode == IR_TYPE_HWRC5_MODE)
			   || (dev->ir_mode == IR_TYPE_HWRC5X_MODE)
			   || (dev->ir_mode == IR_TYPE_HWRC6_MODE))
				ir_write(REG_IR_RC_CTRL, ir_read(REG_IR_RC_CTRL) & (u16)~IR_RC_EN);
			else
				ir_write(REG_IR_CTRL, ir_read(REG_IR_CTRL) & (u16)~IR_EN);
			disable_irq(ir_irq_base);
			ir_irq_enable = 0;
		}
	}
}
extern int bypass_ir_config;
static void mtk_ir_customer_config(void)
{
	MIRC_Set_IRDBG_Level(ir_dbglevel);
	MIRC_Set_IRSpeed_Level(ir_speed);
	if (profile_num >0)
		MIRC_IRCustomer_Config(ir_dts_config, profile_num);
	else if (!bypass_ir_config)
		MIRC_IRCustomer_Config(ir_config, IR_SUPPORT_NUM);
}
void mtk_ir_reinit(void)
{
	int ret;
	struct mtk_ir_dev *dev = IRDev.pIRDev;

	if (dev == NULL)
		return;
	switch (dev->ir_mode) {
	case IR_TYPE_FULLDECODE_MODE: {
		mtk_ir_hw_fulldecode_config();
	}
	break;
	case IR_TYPE_RAWDATA_MODE: {
		mtk_ir_hw_rawdecode_config();
	}
	break;
	case IR_TYPE_HWRC5_MODE: {
		mtk_ir_hw_rc5decode_config();
	}
	break;
	case IR_TYPE_HWRC5X_MODE: {
		mtk_ir_hw_rc5decode_config();
	}
	break;
	case IR_TYPE_HWRC6_MODE: {
		mtk_ir_hw_rc6decode_config();
	}
	break;
	case IR_TYPE_SWDECODE_MODE: {
		mtk_ir_sw_decode_config();
	}
	break;
	default:
		IRDBG_INFO("No IR Mode Support!\n");
		break;
	}
	if ((dev->ir_mode == IR_TYPE_HWRC5_MODE)
	   || (dev->ir_mode == IR_TYPE_HWRC5X_MODE)
	   || (dev->ir_mode == IR_TYPE_HWRC6_MODE)) {
		if (ir_irq_sel == 0) {
			free_irq(ir_irq_base, &IRDev);
			ir_irq_enable = 0;
			ret = request_irq(ir_rc_irq_base, _MDrv_IR_RC_ISR, 0,
						"IR_RC", &IRDev);
			if (ret < 0) {
				IRDBG_ERR("IR IRQ registartion ERROR!\n");
			} else {
				ir_irq_enable = 1;
				IRDBG_INFO("IR IRQ registartion OK!\n");
			}
			ir_irq_sel = 1;
		}
	} else {
		if (ir_irq_sel == 1) {
			free_irq(ir_rc_irq_base, &IRDev);
			ir_irq_enable = 0;
			ret = request_irq(ir_irq_base,
					_MDrv_IR_ISR, 0, "IR", &IRDev);
			if (ret < 0) {
				IRDBG_ERR("IR IRQ registartion ERROR!\n");
			} else {
				ir_irq_enable = 1;
				IRDBG_INFO("IR IRQ registartion OK!\n");
			}
			ir_irq_sel = 0;
		}
	}
}
EXPORT_SYMBOL_GPL(mtk_ir_reinit);

static void mtk_ir_init(int bResumeInit)
{
	int ret = 0;
	struct mtk_ir_dev *dev = IRDev.pIRDev;

	if (dev == NULL)
		return;
	switch (dev->ir_mode) {
	case IR_TYPE_FULLDECODE_MODE: {
		mtk_ir_hw_fulldecode_config();
	}
	break;
	case IR_TYPE_RAWDATA_MODE: {
		mtk_ir_hw_rawdecode_config();
	}
	break;
	case IR_TYPE_HWRC5_MODE: {
		mtk_ir_hw_rc5decode_config();
	}
	break;
	case IR_TYPE_HWRC5X_MODE: {
		mtk_ir_hw_rc5decode_config();
	}
	break;
	case IR_TYPE_HWRC6_MODE: {
		mtk_ir_hw_rc6decode_config();
	}
	break;
	case IR_TYPE_SWDECODE_MODE: {
		mtk_ir_sw_decode_config();
	}
	break;
	default:
		IRDBG_INFO("No IR Mode Support!\n");
		break;
	}
	if ((!bResumeInit) && (ir_irq_sel == 0xFF)) {
		ir_irq_enable = 0;
		if ((dev->ir_mode == IR_TYPE_HWRC5_MODE)
		   || (dev->ir_mode == IR_TYPE_HWRC5X_MODE)
		   || (dev->ir_mode == IR_TYPE_HWRC6_MODE)) {
			ret = request_irq(ir_rc_irq_base, _MDrv_IR_RC_ISR, 0,
					"IR_RC", &IRDev);
			ir_irq_sel = 1;
		} else {
			ret = request_irq(ir_irq_base,
					_MDrv_IR_ISR, 0, "IR", &IRDev);
			ir_irq_sel = 0;
		}

		if (ret < 0) {
			IRDBG_ERR("IR IRQ registartion ERROR!\n");
		}

		else {
			ir_irq_enable = 1;
			IRDBG_INFO("IR IRQ registartion OK!\n");
		}
	}
}

struct input_dev *ms_ir_dev;
EXPORT_SYMBOL(ms_ir_dev);
int mtk_ir_register_device(
		struct platform_device *pdev)
{
	struct mtk_ir_dev *dev = NULL;
	int ret;
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	int i = 0;
#endif
	//1. alloc struct ir_dev
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	//2. init locks
	spin_lock_init(&dev->keylock);
	spin_lock_init(&irq_read_lock);

	mutex_init(&dev->lock);
	mutex_lock(&dev->lock);
	//3. init other args
	dev->priv = &IRDev;
	//4. init readfifo & sem & waitqueue
	ret = kfifo_alloc(&dev->read_fifo, MAX_IR_DATA_SIZE,
					 GFP_KERNEL);
	if (ret < 0) {
		IRDBG_ERR("ERROR kfifo_alloc!\n");
		goto out_dev;
	}
	sema_init(&dev->sem, 1);
	init_waitqueue_head(&dev->read_wait);

#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	//5. alloc struct input_dev
	dev->input_dev = input_allocate_device();
	if (!dev->input_dev) {
		ret = -ENOMEM;
		goto out_kfifo;
	}
	input_set_drvdata(dev->input_dev, dev);
	dev->input_name = MTK_IR_DEVICE_NAME;
	dev->input_phys = "/dev/ir";
	dev->driver_name = MTK_IR_DRIVER_NAME;
	dev->map_num =
		NUM_KEYMAP_MTK_TV; //default :mtk keymap
	//6. init&register	input device
	dev->input_dev->id.bustype = BUS_I2C;
	dev->input_dev->id.vendor = INPUTID_VENDOR;
	dev->input_dev->id.product = INPUTID_PRODUCT;
	dev->input_dev->id.version = INPUTID_VERSION;
	set_bit(EV_KEY, dev->input_dev->evbit);
	set_bit(EV_REP, dev->input_dev->evbit);
	set_bit(EV_MSC, dev->input_dev->evbit);
	set_bit(MSC_SCAN, dev->input_dev->mscbit);
	dev->input_dev->dev.parent = &(pdev->dev);
	dev->input_dev->phys = dev->input_phys;
	dev->input_dev->name = dev->input_name;
	for (i = 0; i < KEY_CNT; i++)
		__set_bit(i, dev->input_dev->keybit);
	__clear_bit(BTN_TOUCH,
				dev->input_dev->keybit);// IR device without this case

	ret = input_register_device(dev->input_dev);
#endif
	//7. register IR Data ctrl
	ret = MIRC_Data_Ctrl_Init(dev);
	if (ret < 0) {
		IRDBG_ERR("Init IR Raw Data Ctrl Failed!\n");

		goto out_unlock;

		return -EINVAL;
	}
	IRDev.pIRDev = dev;
	ms_ir_dev = dev->input_dev;
	mutex_unlock(&dev->lock);
	return 0;
out_unlock:
	mutex_unlock(&dev->lock);
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	input_unregister_device(dev->input_dev);
	dev->input_dev = NULL;
#endif
out_kfifo:
	kfifo_free(&dev->read_fifo);
out_dev:
	kfree(dev);
	return ret;
}

int mtk_ir_unregister_device(void)
{
	struct mtk_ir_dev *dev = IRDev.pIRDev;

	if (dev == NULL)
		return -EINVAL;
//pr_info("mtk_ir_unregister_device start\n");
	kfifo_free(&dev->read_fifo);
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	input_unregister_device(dev->input_dev);
	dev->input_dev = NULL;
#endif
	MIRC_Data_Ctrl_DeInit(dev);
	kfree(dev);
	return 0;
}


/*=========================IR Cdev fileoperations Functions=============*/
/* Used to keep track of known keymaps */
static int _mod_ir_open(struct inode *inode,
						struct file *filp)
{
	struct IRModHandle *dev;

	dev = container_of(inode->i_cdev, struct IRModHandle,
					   cDevice);
	filp->private_data = dev;

	return 0;
}

static int _mod_ir_release(struct inode *inode,
						   struct file *filp)
{
	return 0;
}

static ssize_t _mod_ir_write(struct file *filp,
							 const char __user *buf, size_t count,
							 loff_t *f_pos)
{
	return 0;
}

static ssize_t _mod_ir_read(struct file *filp,
				char __user *buf, size_t count, loff_t *f_pos)
{
	struct IRModHandle *ir = &IRDev;
	u32 read_data = 0;
	int ret = 0;

	if (down_interruptible(&ir->pIRDev->sem))
		return -EAGAIN;

	while (!kfifo_len(&ir->pIRDev->read_fifo)) {
		up(&ir->pIRDev->sem);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(
				ir->pIRDev->read_wait,
				(kfifo_len(&ir->pIRDev->read_fifo) > 0)))
			return -ERESTARTSYS;

		if (down_interruptible(&ir->pIRDev->sem))
			return -ERESTARTSYS;
	}
	ret = 0;
	while (kfifo_len(&ir->pIRDev->read_fifo)
		   && ret < count) {
		kfifo_out(&ir->pIRDev->read_fifo, &read_data,
				  sizeof(read_data));

		if (copy_to_user(buf + ret, &read_data,
						 sizeof(read_data))) {
			ret = -EFAULT;
			goto out;
		}
		ret += sizeof(read_data);
	}
out:
	up(&ir->pIRDev->sem);
	return ret;

}


static unsigned int _mod_ir_poll(struct file *
								 filp, poll_table *wait)
{
	struct IRModHandle *ir = &IRDev;
	unsigned int mask = 0;
	int ret = 0;

	if (down_interruptible(&ir->pIRDev->sem))
		return -EAGAIN;
	poll_wait(filp, &ir->pIRDev->read_wait, wait);
	ret = kfifo_len(&ir->pIRDev->read_fifo);
	if (ret > 0)
		mask |= POLLIN|POLLRDNORM;
	up(&ir->pIRDev->sem);
	return mask;
}

static int _mod_ir_fasync(int fd,
						  struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode,
						 &IRDev.async_queue);
}

#ifdef HAVE_UNLOCKED_IOCTL
static long _mod_ir_ioctl(struct file *filp,
						  unsigned int cmd, unsigned long arg)
#else
static int _mod_ir_ioctl(struct inode *inode,
						 struct file *filp, unsigned int cmd,
						 unsigned long arg)
#endif
{
	int retval = 0;
	int err = 0;
	struct MS_IR_KeyInfo_s keyinfo;
	u32 keyvalue = 0;
	u8 bEnableIR;
	pid_t masterPid;
	u32 u32HeadCode = 0;

	if ((_IOC_TYPE(cmd) != IR_IOC_MAGIC)
		|| (_IOC_NR(cmd) > IR_IOC_MAXNR))
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok((void __user *)arg,
				_IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok((void __user *)arg,
				_IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {
	case MDRV_IR_LTP_INIT:
		Set_LTP_INIT();
		pr_info("MDRV_IR_LTP_INIT\n");
		break;

	case MDRV_IR_LTP_RESULT:
		if (Get_LTP_result())
			retval = 0;
		else
			retval = -EINVAL;
		pr_info("MDRV_IR_LTP_RESULT\n");
		break;

	case MDRV_IR_INIT:
		IRDBG_INFO("ioctl:MDRV_IR_INIT\n");
		mtk_ir_init(0);
		IRDev.u32IRFlag |=
			(IRFLAG_IRENABLE|IRFLAG_HWINITED);
		break;
	case MDRV_IR_SEND_KEY:
		IRDBG_INFO("ioctl:MDRV_IR_SEND_KEY\n");
		if (copy_from_user(&keyinfo.u8Key,
						  &(((struct MS_IR_KeyInfo_s __user *)arg)->u8Key),
						  sizeof(keyinfo.u8Key))) {
			IR_PRINT("ioctl: MDRV_IR_ENABLE_IR keyinfo.u8Key fail\n");
			return -EFAULT;
		}
		if (copy_from_user(&keyinfo.u8Flag,
						  &(((struct MS_IR_KeyInfo_s __user *)arg)->u8Flag),
						  sizeof(keyinfo.u8Flag))) {
			IR_PRINT("ioctl: MDRV_IR_ENABLE_IR keyinfo.u8Flag fail\n");
			return -EFAULT;
		}
		keyvalue = (keyinfo.u8Key<<8 | keyinfo.u8Flag);
		keyvalue |= (0x01 << 28);
		//sned key
		if (down_trylock(&IRDev.pIRDev->sem) == 0) {
			if (kfifo_in(&IRDev.pIRDev->read_fifo, &keyvalue,
						 sizeof(u32)) != sizeof(u32))
				return -ENOMEM;
			up(&IRDev.pIRDev->sem);
			wake_up_interruptible(&IRDev.pIRDev->read_wait);
		}
		break;

	case MDRV_IR_SET_DELAYTIME:
		IRDBG_INFO("ioctl:MDRV_IR_GET_LASTKEYTIME\n");
		break;
	case MDRV_IR_ENABLE_IR:
		IRDBG_INFO("ioctl: MDRV_IR_ENABLE_IR\n");
		if (copy_from_user(&bEnableIR, (int __user *)arg,
						  sizeof(bEnableIR))) {
			IR_PRINT("ioctl: MDRV_IR_ENABLE_IR fail\n");
			return -EFAULT;
		}
		mtk_ir_enable(bEnableIR);
		if (bEnableIR)
			IRDev.u32IRFlag |= IRFLAG_IRENABLE;
		else
			IRDev.u32IRFlag &= ~IRFLAG_IRENABLE;

		break;
	case MDRV_IR_SET_MASTER_PID:
		IRDBG_INFO("ioctl:MDRV_IR_SET_MASTER_PID\n");
		if (copy_from_user(&masterPid, (pid_t __user *)arg,
						  sizeof(masterPid))) {
			IR_PRINT("ioctl: MDRV_IR_SET_MASTER_PID fail\n");
			return -EFAULT;
		}
		MasterPid = masterPid;
		break;

	case MDRV_IR_GET_MASTER_PID:
		IRDBG_INFO("ioctl:MDRV_IR_GET_MASTER_PID\n");
		masterPid = MasterPid;

		if (copy_to_user((pid_t __user *)arg, &masterPid,
						sizeof(masterPid))) {
			IR_PRINT("ioctl: MDRV_IR_GET_MASTER_PID fail\n");
			return -EFAULT;
		}
		break;

	case MDRV_IR_INITCFG:
		IRDBG_INFO("ioctl:MDRV_IR_INITCFG\n");
		break;

	case MDRV_IR_TIMECFG:
		IRDBG_INFO("ioctl:MDRV_IR_TIMECFG\n");
		break;
	case MDRV_IR_GET_SWSHOT_BUF:
		IRDBG_INFO("ioctl:MDRV_IR_GET_SWSHOT_BUF\n");
		break;
	case MDRV_IR_SET_HEADER: {
		struct MS_MultiIR_HeaderInfo_s stItMulti_HeaderInfo;
		u32 u32HeadCode0 = 0;
		u32 u32HeadCode1 = 0;

		IRDBG_INFO("ioctl:MDRV_IR_SET_HEADER\n");
		if (copy_from_user(&stItMulti_HeaderInfo,
						   (struct MS_MultiIR_HeaderInfo_s __user *)arg,
						   sizeof(struct MS_MultiIR_HeaderInfo_s))) {
			IR_PRINT("ioctl: MDRV_IR_SET_HEADER fail\n");
			return -EFAULT;
		}
		u32HeadCode0 =
			(stItMulti_HeaderInfo._u8IRHeaderCode1 |
			 (stItMulti_HeaderInfo._u8IRHeaderCode0 << 8))
			&0xFFFF;
		u32HeadCode1 =
			(stItMulti_HeaderInfo._u8IR2HeaderCode1 |
			 (stItMulti_HeaderInfo._u8IR2HeaderCode0 << 8))
			&0xFFFF;
		IRDBG_INFO(
			KERN_ERR"[SET_HEADER] u32HeadCode0 = 0x%x",
			u32HeadCode0);
		IRDBG_INFO(
			KERN_ERR"[SET_HEADER] u32HeadCode1 = 0x%x",
			u32HeadCode1);
		_Mdrv_IR_SetHeaderCode(u32HeadCode0, u32HeadCode1);
		break;
	}
	case MDRV_IR_SET_PROTOCOL:
		IRDBG_INFO("ioctl:MDRV_IR_SET_PROTOCOL\n");
		break;
	case MDRV_IR_ENABLE_WB_IR:
		IRDBG_INFO("ioctl:MDRV_IR_ENABLE_WB_IR\n");
		retval = __get_user(bEnableIR, (int __user *)arg);
		MIRC_Set_WhiteBalance_Enable(bEnableIR);
		break;
	case MDRV_IR_SET_KEY_EVENT: {
		u8 u8EventFlag = 0xFF;

		IRDBG_INFO("ioctl:MDRV_IR_SET_KEY_EVENT\n");
		if (copy_from_user(&u8EventFlag, (u8 __user *)arg,
						   sizeof(u8))) {
			IR_PRINT("ioctl: MDRV_IR_SET_KEY_EVENT fail\n");
			return -EFAULT;
		}
		MIRC_Send_KeyEvent(u8EventFlag);
		break;
	}
	default:
		IR_PRINT("ioctl: unknown command\n");
		return -ENOTTY;
	}
	return retval;
}

#ifdef CONFIG_COMPAT
static long _mod_ir_compat_ioctl(struct file *
				filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	int err = 0;
	struct MS_IR_KeyInfo_s keyinfo;
	u32 keyvalue = 0;
	u8 bEnableIR;
	pid_t masterPid;
	u32 u32HeadCode = 0;

	if ((_IOC_TYPE(cmd) != IR_IOC_MAGIC)
		|| (_IOC_NR(cmd) > IR_IOC_MAXNR)) {
		return -ENOTTY;
	}
	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok((void __user *)arg,
				 _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err =  !access_ok((void __user *)arg,
				  _IOC_SIZE(cmd));
	}
	if (err)
		return -EFAULT;

	switch (cmd) {
	case MDRV_IR_LTP_INIT:
		Set_LTP_INIT();
		pr_info("MDRV_IR_LTP_INIT\n");
		break;

	case MDRV_IR_LTP_RESULT:
		if (Get_LTP_result())
			retval = 0;
		else
			retval = -EINVAL;
		pr_info("MDRV_IR_LTP_RESULT\n");
		break;

	case MDRV_IR_INIT:
		IRDBG_INFO("ioctl:MDRV_IR_INIT\n");
		mtk_ir_init(0);
		IRDev.u32IRFlag |=
			(IRFLAG_IRENABLE|IRFLAG_HWINITED);
		break;
	case MDRV_IR_SEND_KEY:
		IRDBG_INFO("ioctl:MDRV_IR_SEND_KEY\n");
		if (copy_from_user(&keyinfo.u8Key,
						  &(((struct MS_IR_KeyInfo_s __user *)arg)->u8Key),
						  sizeof(keyinfo.u8Key))) {
			IR_PRINT("ioctl: MDRV_IR_SEND_KEY keyinfo.u8Key fail\n");
			return -EFAULT;
		}
		if (copy_from_user(&keyinfo.u8Flag,
						  &(((struct MS_IR_KeyInfo_s __user *)arg)->u8Flag),
						  sizeof(keyinfo.u8Flag))) {
			IR_PRINT("ioctl: MDRV_IR_SEND_KEY keyinfo.u8Flag fail\n");
			return -EFAULT;
		}
		keyvalue = (keyinfo.u8Key<<8 | keyinfo.u8Flag);
		keyvalue |= (0x01 << 28);
		//sned key
		if (down_trylock(&IRDev.pIRDev->sem) == 0) {
			if (kfifo_in(&IRDev.pIRDev->read_fifo, &keyvalue,
						 sizeof(u32)) != sizeof(u32))
				return -ENOMEM;
			up(&IRDev.pIRDev->sem);
			wake_up_interruptible(&IRDev.pIRDev->read_wait);
		}
		break;

	case MDRV_IR_SET_DELAYTIME:
		IRDBG_INFO("ioctl:MDRV_IR_GET_LASTKEYTIME\n");
		break;
	case MDRV_IR_ENABLE_IR:
		IRDBG_INFO("ioctl: MDRV_IR_ENABLE_IR\n");
		if (copy_from_user(&bEnableIR, (int __user *)arg,
						  sizeof(bEnableIR))) {
			IR_PRINT("ioctl: MDRV_IR_ENABLE_IR fail\n");
			return -EFAULT;
		}
		mtk_ir_enable(bEnableIR);
		if (bEnableIR)
			IRDev.u32IRFlag |= IRFLAG_IRENABLE;
		else
			IRDev.u32IRFlag &= ~IRFLAG_IRENABLE;

		break;
	case MDRV_IR_SET_MASTER_PID:
		IRDBG_INFO("ioctl:MDRV_IR_SET_MASTER_PID\n");
		if (copy_from_user(&masterPid, (pid_t __user *)arg,
						  sizeof(masterPid))) {
			IR_PRINT("ioctl: MDRV_IR_SET_MASTER_PID fail\n");
			return -EFAULT;
		}
		MasterPid = masterPid;
		break;

	case MDRV_IR_GET_MASTER_PID:
		IRDBG_INFO("ioctl:MDRV_IR_GET_MASTER_PID\n");
		masterPid = MasterPid;
		if (copy_to_user((pid_t __user *)arg, &masterPid,
						 sizeof(masterPid))) {
			IR_PRINT("ioctl: MDRV_IR_GET_MASTER_PID fail\n");
			return -EFAULT;
		}
		break;

	case MDRV_IR_INITCFG:
		IRDBG_INFO("ioctl:MDRV_IR_INITCFG\n");
		break;

	case MDRV_IR_TIMECFG:
		IRDBG_INFO("ioctl:MDRV_IR_TIMECFG\n");
		break;
	case MDRV_IR_GET_SWSHOT_BUF:
		IRDBG_INFO("ioctl:MDRV_IR_GET_SWSHOT_BUF\n");
		break;
	case MDRV_IR_SET_HEADER: {
		struct MS_MultiIR_HeaderInfo_s stItMulti_HeaderInfo;
		u32 u32HeadCode0 = 0;
		u32 u32HeadCode1 = 0;

		IRDBG_INFO("ioctl:MDRV_IR_SET_HEADER\n");
		if (copy_from_user(&stItMulti_HeaderInfo,
						   (struct MS_MultiIR_HeaderInfo_s __user *)arg,
						   sizeof(struct MS_MultiIR_HeaderInfo_s))) {
			IR_PRINT("ioctl: MDRV_IR_SET_HEADER fail\n");
			return -EFAULT;
		}
		u32HeadCode0 =
			(stItMulti_HeaderInfo._u8IRHeaderCode1 |
			 (stItMulti_HeaderInfo._u8IRHeaderCode0 << 8))
			&0xFFFF;
		u32HeadCode1 =
			(stItMulti_HeaderInfo._u8IR2HeaderCode1 |
			 (stItMulti_HeaderInfo._u8IR2HeaderCode0 << 8))
			&0xFFFF;
		IRDBG_INFO(
			KERN_ERR"[SET_HEADER] u32HeadCode0 = 0x%x",
			u32HeadCode0);
		IRDBG_INFO(
			KERN_ERR"[SET_HEADER] u32HeadCode1 = 0x%x",
			u32HeadCode1);
		_Mdrv_IR_SetHeaderCode(u32HeadCode0, u32HeadCode1);
		break;
		}
	case MDRV_IR_ENABLE_WB_IR:
		IRDBG_INFO("ioctl:MDRV_IR_ENABLE_WB_IR\n");
		retval = __get_user(bEnableIR, (int __user *)arg);
		MIRC_Set_WhiteBalance_Enable(bEnableIR);
		break;
	case MDRV_IR_SET_PROTOCOL:
		IRDBG_INFO("ioctl:MDRV_IR_SET_PROTOCOL\n");
		break;
	default:
		IR_PRINT("ioctl: unknown command\n");
		return -ENOTTY;
	}
	return retval;
}
#endif

static int mtk_ir_cdev_init(void)
{
	int ret;
	dev_t dev = MKDEV(0x9c, 0x00);

	IR_PRINT("##### MTK IR Cdev Init start #####\n");
	if (IRDev.s32IRMajor) {
		dev = MKDEV(IRDev.s32IRMajor, IRDev.s32IRMinor);
		ret = register_chrdev_region(dev,
							MOD_IR_DEVICE_COUNT, MDRV_NAME_IR);
	} else {
		ret = alloc_chrdev_region(&dev, IRDev.s32IRMinor,
							MOD_IR_DEVICE_COUNT, MDRV_NAME_IR);
		IRDev.s32IRMajor = MAJOR(dev);
	}

	if (ret < 0) {
		IRDBG_ERR("Unable to get major %d\n",
				  IRDev.s32IRMajor);
		return ret;
	}

	cdev_init(&IRDev.cDevice, &IRDev.IRFop);
	ret = cdev_add(&IRDev.cDevice, dev, MOD_IR_DEVICE_COUNT);
	if (ret != 0) {
		IRDBG_ERR("Unable add a character device\n");
		unregister_chrdev_region(dev,
								 MOD_IR_DEVICE_COUNT);
		return ret;
	}
#ifdef CONFIG_MTK_UDEV_NODE
	struct class *chrdev_modir_class = NULL;

	chrdev_modir_class = class_create(THIS_MODULE,
									  MDRV_NAME_IR);
	if (IS_ERR(chrdev_modir_class)) {
		ret = PTR_ERR(chrdev_modir_class);
		return ret;
	}

	device_create(chrdev_modir_class, NULL,
				  MKDEV(IRDev.s32IRMajor, IRDev.s32IRMinor), NULL,
				  MDRV_NAME_IR);
#endif
	IR_PRINT("##### MTK IR Cdev Init end #####\n");
	return 0;
}

/*=================end IR Cdev fileoperations Functions ========================*/

/***=============== IR ATTR Functions=================***/
static ssize_t IRProtocols_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
	unsigned long long enable_protocols = 0;
	u8 i = 0;
	ssize_t ret = 0;

	IR_PRINT("===========Protocols Attr Show========\n");
	enable_protocols = MIRC_Get_Protocols();
	IR_PRINT("Eenable_Protocols =0x%llx\n",
			 enable_protocols);
	strncat(buf, "Eenable Protocols Name:", 23);
	for (i = 0; i < IR_TYPE_MAX; i++)
		if ((enable_protocols>>i) & 0x01) {
			switch (i) {
			case IR_TYPE_NEC:
				strncat(buf, "NEC  ", 5);
				break;
			case IR_TYPE_RCA:
				strncat(buf, "RCA  ", 5);
				break;
			case IR_TYPE_P7051:
				strncat(buf, "PANASONIC 7051	 ", 16);
				break;
			case IR_TYPE_METZ:
				strncat(buf, "metz  ", 6);
				break;
			case IR_TYPE_SONY:
				strncat(buf, "SONY  ", 6);
				break;
			default:
				break;
			}
		}
	strncat(buf, "\n", 1);
	ret = strlen(buf)+1;
	return ret;
}

static ssize_t IRProtocols_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	unsigned long protocol = 0;
	int n;

	IR_PRINT("===========Protocols Attr Store========\n");
	if (kstrtol(data, 10, &protocol) != -1)
		IR_PRINT("Set IR Protocol :0x%lx\n", protocol);
	MIRC_Set_Protocols(protocol);
	return strlen(data);
}

static ssize_t IRDebug_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
#define CUR_DBG_LV_SZ 30
	ssize_t ret = 0;
	enum IR_DBG_LEVEL_e level = IR_DBG_NONE;
	int ret_flag = 0;

	IR_PRINT("===========MTK IR Debug Show========\n");
	IR_PRINT("Debug Level Table:\n");
	IR_PRINT("			0:IR_DBG_NONE\n");
	IR_PRINT("			1:IR_DBG_ERR\n");
	IR_PRINT("			2:IR_DBG_WRN\n");
	IR_PRINT("			3:IR_DBG_MSG\n");
	IR_PRINT("			4:IR_DBG_INFO\n");
	IR_PRINT("			5:IR_DBG_ALL\n");
	level = MIRC_Get_IRDBG_Level();
	ret_flag = snprintf(buf, CUR_DBG_LV_SZ,
			 "Current Debug Level:	%d\n", level);
	if (ret_flag < 0) {
		IR_PRINT("snprintf fail.(%d)\n", ret_flag);
		return -1;
	}
	ret = strlen(buf)+1;
	return ret;
}

static ssize_t IRDebug_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	unsigned long level = 0;

	IR_PRINT("===========MTK IR Debug Store========\n");
	if (kstrtoul(data, 10, &level) != 0)
		return 0;

	if (level > IR_DBG_ALL)
		level = IR_DBG_ALL;
	IR_PRINT("Set IR Debug Level :%d\n", level);
	MIRC_Set_IRDBG_Level(level);
	return strlen(data);
}
static ssize_t IRSpeed_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
#define CUR_SPD_LV_SZ 30
	ssize_t ret = 0;
	enum IR_SPEED_LEVEL_e level = IR_DBG_NONE;
	int ret_flag = 0;

	IR_PRINT("===========MTK IR Speed Show========\n");
	IR_PRINT("IR Speed Level Table:\n");
	IR_PRINT("			0:IR_SPEED_FAST_H\n");
	IR_PRINT("			1:IR_SPEED_FAST_N\n");
	IR_PRINT("			2:IR_SPEED_FAST_L\n");
	IR_PRINT("			3:IR_SPEED_NORMAL\n");
	IR_PRINT("			4:IR_SPEED_SLOW_H\n");
	IR_PRINT("			5:IR_SPEED_SLOW_N\n");
	IR_PRINT("			6:IR_SPEED_SLOW_L\n");
	level = MIRC_Get_IRSpeed_Level();
	ret_flag = snprintf(buf, CUR_SPD_LV_SZ,
			 "Current Speed Level:	%d\n", (int)level);
	if (ret_flag < 0) {
		IR_PRINT("snprintf fail.(%d)\n", ret_flag);
		return -1;
	}
	ret = strlen(buf)+1;
	return ret;
}

static ssize_t IRSpeed_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	unsigned long speed = 0;

	IR_PRINT("===========MTK IR Speed Store========\n");
	if (kstrtoul(data, 10, &speed) != 0)
		return 0;

	//sscanf(data,"%d",&level);
	if (speed > IR_SPEED_SLOW_L)
		speed = IR_SPEED_SLOW_L;
	IR_PRINT("Set IR Speed Level :%ld\n", speed);
	MIRC_Set_IRSpeed_Level(speed);
	return strlen(data);
}
static ssize_t IREnable_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
#define CUR_IR_EN_SZ 30
	ssize_t ret = 0;
	unsigned int IsEnable = 0;
	int ret_flag = 0;

	IsEnable = ir_irq_enable;
	ret_flag = snprintf(buf, CUR_IR_EN_SZ,
			 "Current IR IsEnable:	%d\n", IsEnable);
	if (ret_flag < 0) {
		IR_PRINT("snprintf fail.(%d)\n", ret_flag);
		return -1;
	}
	ret = strlen(buf)+1;
	return ret;
}

static ssize_t IREnable_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	unsigned long Enable = 0;

	if (kstrtoul(data, 10, &Enable) != 0)
		return 0;

	if (Enable)
		pr_info("=======Enable IR!\n");
	else
		pr_info("=======Disable IR!\n");

	mtk_ir_enable(Enable);
	return strlen(data);
}

//show usage IREvent
static ssize_t IREvent_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
#define USAGE_SZ 30
	ssize_t ret = 0;
	int ret_flag = 0;

	ret_flag = snprintf(buf, USAGE_SZ,
			 "usage: echo up > IREvent\n");
	if (ret_flag < 0) {
		IR_PRINT("snprintf fail.(%d)\n", ret_flag);
		return -1;
	}
	ret = strlen(buf)+1;
	return ret;
}

static ssize_t IREvent_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	if (data != NULL) {
		IRDBG_INFO(" data = %s\n", data);
		if ((strncmp(data, "up", 2) == 0)
		   || (strncmp(data, "UP", 2) == 0)) {
			MIRC_Send_KeyEvent(0);
		}
		return strlen(data);
	} else
		return -1;
}


static ssize_t ts_IREnable_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
	ssize_t ret = 0;
	int ret_flag = 0;

	//IR_PRINT("===========ts IR enable Sshow========\n");
	//IsEnable = MIRC_Get_IRSpeed_Level();
	ret_flag = snprintf(buf, BUF_64, "Current IR ts_IsEnable:  %d\n", (int)ts_IsEnable);
	if (ret_flag < 0) {
		IR_PRINT("snprintf fail.(%d)\n", ret_flag);
		return -1;
	}
	ret = strlen(buf)+1;
	return ret;
}

static ssize_t
ts_IREnable_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	if (kstrtoul(data, 10, &ts_IsEnable) != 0)
		return 0;

	if (ts_IsEnable) {
		pr_info("=======Enable IR ts!\n");
	} else {
		free_list_time_stage();
		pr_info("=======Disable IR ts!\n");
	}
	return strlen(data);
}

char *keycode = "key_code";
char *repflag = "flag";
char *ir_isr = "IR_ISR";
char *data_get = "Data_Get";
char *decode_list_entry = "Decode_list_entry";
char *diff_protocol = "Diff_protocol";
char *keycode_from_map = " Keycode_From_Map";
char *data_ctrl_thread = "Data_Ctrl_Thread";

static ssize_t IRTimeAnalysis_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
	ssize_t	  ret = 0;
	struct list_head *pos, *q;
	struct time_stage *ts;
	int i = 0;

	if (ts_IsEnable == 0x1) {

		IR_PRINT(" no.%8s %4s %8s %10s %17s %15s %17s %17s (µs)\n",
				 keycode, repflag, ir_isr, data_get, decode_list_entry,
				 diff_protocol, keycode_from_map, data_ctrl_thread);


		list_for_each_entry(ts, &stage_List, list) {
			i++;
			IR_PRINT("%03d 0x%06x %4u %8lu %10lu %17lu %15lu %17lu %17lu\n",
					 i,
					 ts->keycode,
					 ts->flag,
					 ts->time_MDrv_IR_ISR.diff_time,
					 //ts->time_MIRC_Data_Store.diff_time,
					 ts->time_MIRC_Data_Get.diff_time,
					 ts->time_MIRC_Decode_list_entry.diff_time,
					 ts->time_MIRC_Diff_protocol.diff_time,
					 //ts->time_MIRC_Data_Match_shotcount.diff_time,
					 //ts->time_MIRC_Send_key_event.diff_time,
					 //ts->time_MIRC_Data_Wakeup.diff_time,
					 ts->time_MIRC_Keycode_From_Map.diff_time,
					 ts->time_MIRC_Data_Ctrl_Thread.diff_time
					);
		}
		ret = strlen(buf)+1;
	}
	return ret;
}

static ssize_t IRTimeAnalysis_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	if (data != NULL) {
		IRDBG_INFO(" data = %s\n", data);
		if ((strncmp(data, "reset", 5) == 0)
		   || (strncmp(data, "RESET", 5) == 0))
			free_list_time_stage();
		return strlen(data);
	} else
		return -1;
}


#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
static ssize_t IRTimeout_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
#define CUR_EV_TIM_SZ 30
	ssize_t ret = 0;
	int ret_flag = 0;
	unsigned long timeout = 0;

	IR_PRINT("===========MTK IR Timeout Show========\n");
	timeout = MIRC_Get_Event_Timeout();
	ret_flag = snprintf(buf, CUR_EV_TIM_SZ,
			 "Current EventTimeout:	 %lu\n", timeout);
	if (ret_flag < 0) {
		IR_PRINT("snprintf fail.(%d)\n", ret_flag);
		return -1;
	}
	ret = strlen(buf)+1;
	return ret;
}

static ssize_t IRTimeout_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	unsigned long timeout = 0;

	IR_PRINT("===========MTK IR Timeout Store========\n");
	if (kstrtoul(data, 10, &timeout) != 0)
		return 0;
	IR_PRINT("Set IR EventTimeout  :%lu\n", timeout);
	MIRC_Set_Event_Timeout(timeout);
	return strlen(data);
}
static DEVICE_ATTR_RW(IRTimeout);
#endif

static ssize_t IRWhiteBalance_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
	ssize_t ret = 0;
	bool flag = 0;
	int ret_flag = 0;

	IR_PRINT("===========MTK IR Timeout Show========\n");
	flag = MIRC_IsEnable_WhiteBalance();
	ret_flag = snprintf(buf, BUF_33, "WhiteBalance IR Status:  %s\n",
			(flag == TRUE)?"enable" : "disable");
	if (ret_flag < 0) {
		IR_PRINT("snprintf fail.(%d)\n", ret_flag);
		return -1;
	}
	ret = strlen(buf)+1;
	return ret;
}

static ssize_t IRWhiteBalance_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	unsigned long Enable = 0;

	if (kstrtoul(data, 10, &Enable) != 0)
		return 0;

	if (Enable)
		pr_info("=======Enable WhiteBalance IR!\n");
	else
		pr_info("=======Disable WhiteBalance IR!\n");

	MIRC_Set_WhiteBalance_Enable(Enable);
	return strlen(data);
}

static ssize_t IRConfig_show(
	struct device *device,
	struct device_attribute *mattr, char *buf)
{
	ssize_t ret = 0;
	int ret_flag = 0;

	IR_PRINT("===========MTK IR config Show========\n");
	MIRC_IR_Config_Dump();
	ret_flag = snprintf(buf, BUF_40,
			"=======================================\n");
	if (ret_flag < 0) {
		IR_PRINT("snprintf fail.(%d)\n", ret_flag);
		return -1;
	}
	ret = strlen(buf)+1;
	return ret;
}

static int str2ul(const char *str, u32 *val1,
				  u32 *val2, u32 *val3)
{
	if (str == NULL)
		return -1;
	if (strchr(str, ':') == NULL)
		return -1;
	if (sscanf(str, "%i:%i:%i", val1, val2,
			   val3) == -1)
		return -1;
	return 0;
}

static ssize_t IRConfig_store(
	struct device *device,
	struct device_attribute *mattr, const char *data,
	size_t len)
{
	u32 protocol = IR_TYPE_MAX;
	u32 headcode = 0;
	u32 enable = 0;

	if (str2ul(data, &protocol, &headcode, &enable) != 0) {
		IRDBG_ERR("Data Format : echo protocol:headcode:enable > IRConfig\n");
		return strlen(data);
	}
	pr_info("Config IR: protocol = %x,headcode = 0x%x,enable = %d!\n",
		   protocol, headcode, enable);
	MIRC_Set_IR_Enable((enum IR_Type_e)protocol, headcode,
					   (u8)enable);
	return strlen(data);
}
static DEVICE_ATTR_RW(IRConfig);
static DEVICE_ATTR_RW(IRWhiteBalance);
static DEVICE_ATTR_RW(IRProtocols);
static DEVICE_ATTR_RW(IRDebug);
static DEVICE_ATTR_RW(IRSpeed);
static DEVICE_ATTR_RW(IREnable);
static DEVICE_ATTR_RW(IREvent);
static DEVICE_ATTR_RW(ts_IREnable);
static DEVICE_ATTR_RW(IRTimeAnalysis);

static int mtk_ir_creat_sysfs_attr(
	struct platform_device *pdev)
{
	struct device *dev = &(pdev->dev);
	//int ret = 0;

	IRDBG_INFO("Debug MTK IR Creat Sysfs\n");
	if ((device_create_file(dev,
								  &dev_attr_IRProtocols)))
		return -1;
	if ((device_create_file(dev,
								  &dev_attr_IRDebug)))
		return -1;
	if ((device_create_file(dev,
								  &dev_attr_IRSpeed)))
		return -1;
	if ((device_create_file(dev,
								  &dev_attr_IREnable)))
		return -1;
	if ((device_create_file(dev,
								  &dev_attr_IREvent)))
		return -1;
	if ((device_create_file(dev,
								  &dev_attr_IRWhiteBalance)))
		return -1;
	if ((device_create_file(dev,
								  &dev_attr_IRConfig)))
		return -1;
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	if ((device_create_file(dev,
								  &dev_attr_IRTimeout)))
		return -1;
#endif
	if ((device_create_file(dev,
								  &dev_attr_ts_IREnable)))
		return -1;
	if ((device_create_file(dev,
								  &dev_attr_IRTimeAnalysis)))
		return -1;
	return 0;
}
/**===============end IR ATTR Functions ==================**/
static int mtk_ir_drv_probe(
struct platform_device *pdev)
{
	int ret = 0;
	int i = 0;
	int reg_index = 0;
	int res;
	struct resource reg;
	struct device_node *np;
	unsigned int irq, rc_irq;


	if (pdev == NULL || !(pdev->name))
		return -EINVAL;

	np = pdev->dev.of_node;
	res = strcmp(pdev->name, "mtk-ir");
	if (res || pdev->id != 0)
		IRDBG_ERR("platform_device pdev Failed!\n");

	IRDev.u32IRFlag = 0;

	ret = of_address_to_resource(np, 0, &reg);

	if (ret < 0) {
		dev_err(&pdev->dev, "no reg/irq defined\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq defined\n");
		return -EINVAL;
	}

	rc_irq = platform_get_irq(pdev, 1);
	if (rc_irq < 0) {
		dev_err(&pdev->dev, "no rc_irq defined\n");
		return -EINVAL;
	}

	ir_irq_base = irq;
	ir_rc_irq_base = rc_irq;

	ir_map_membase = devm_ioremap(&pdev->dev, reg.start,
			resource_size(&reg));

	ret = mtk_ir_cdev_init();
	if (ret < 0)
		dev_err(&pdev->dev, "mtk_ir_cdev_init Failed!\n");

	ret = mtk_ir_creat_sysfs_attr(pdev);
	if (ret < 0)
		dev_err(&pdev->dev, "mtk_ir_creat_sysfs_attr Failed!\n");

	ret = mtk_ir_register_device(pdev);
	if (ret < 0)
		dev_err(&pdev->dev, "mtk_ir_register_device Failed!\n");

	pdev->dev.platform_data = &IRDev;

#if IR_GET_KEYMAP_FROM_DTS
	mtk_ir_get_keymap_from_dts(np);
#endif
	mtk_ir_customer_config();

#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
	mtk_ir_init(0);
	IRDev.u32IRFlag |=
		(IRFLAG_IRENABLE|IRFLAG_HWINITED);
#endif
	return ret;
}

static int mtk_ir_drv_remove(
	struct platform_device *pdev)
{
	int res;

	if (pdev == NULL || !(pdev->name))
		return -1;

	res = strcmp(pdev->name, "mtk-ir");
	if (res || pdev->id != 0)
		return -1;
	IR_PRINT("##### IR Driver Remove #####\n");
	cdev_del(&IRDev.cDevice);
	unregister_chrdev_region(MKDEV(IRDev.s32IRMajor,
							IRDev.s32IRMinor), MOD_IR_DEVICE_COUNT);
	mtk_ir_unregister_device();
	IRDev.u32IRFlag = 0;
	pdev->dev.platform_data = NULL;
	return 0;
}

static int mtk_ir_drv_suspend(
	struct platform_device *dev, pm_message_t state)
{
	struct IRModHandle *pIRHandle = (struct IRModHandle *)(
								 dev->dev.platform_data);
	IR_PRINT("##### IR Driver Suspend #####\n");
#if IR_RESUME_SEND_WAKEUPKEY
	pm_set_wakeup_reason(NULL);
#endif
	if (pIRHandle
		&& (pIRHandle->u32IRFlag&IRFLAG_HWINITED)) {
		if (pIRHandle->u32IRFlag & IRFLAG_IRENABLE) {
			mtk_ir_enable(0);
			ir_config_mode = IR_TYPE_MAX_MODE;
		}
	}
	return 0;
}

static int mtk_ir_drv_resume(
		struct platform_device *dev)
{
	struct IRModHandle *pIRHandle = (struct IRModHandle *)(
								 dev->dev.platform_data);
#if IR_RESUME_SEND_WAKEUPKEY
	struct mtk_ir_dev *ir_dev = IRDev.pIRDev;
	int32_t wakeup_src = 0;
	int32_t wakeup_key = 0;

	wakeup_src = pm_get_wakeup_reason();
	wakeup_key = pm_get_wakeup_key();
	IR_PRINT("MTK_IR:WakeUp (name=%s, id=0x%02X ,key=0x%02X).\n", pm_get_wakeup_reason_str(),
								     wakeup_src, wakeup_key);

#endif
	IR_PRINT("##### IR Driver Resume #####\n");
	if (pIRHandle
		&& (pIRHandle->u32IRFlag&IRFLAG_HWINITED)) {
		mtk_ir_init(1);
		if (pIRHandle->u32IRFlag & IRFLAG_IRENABLE) {
			mtk_ir_enable(1);
#if IR_RESUME_SEND_WAKEUPKEY
			if (wakeup_src == 1) {
				if (wakeup_key == 0) {
					wakeup_key = KEY_POWER;
					IR_PRINT("# IR Driver Resume # not find wakeup key!!!\n");
				}
				input_event(ir_dev->input_dev, EV_MSC, MSC_SCAN, RC_MAGIC);
				input_sync(ir_dev->input_dev);
				input_report_key(ir_dev->input_dev, wakeup_key, 1);
				input_sync(ir_dev->input_dev);
				input_report_key(ir_dev->input_dev, wakeup_key, 0);
				input_sync(ir_dev->input_dev);
			}

#endif
		}
	}
	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id mtkir_of_device_ids[]
		= {
	{.compatible = "mtk-ir"},
	{},
};
#endif

static struct platform_driver mtk_ir_driver = {
	.probe		= mtk_ir_drv_probe,
	.remove		= mtk_ir_drv_remove,
	.suspend	= mtk_ir_drv_suspend,
	.resume		= mtk_ir_drv_resume,

	.driver = {
#if defined(CONFIG_OF)
		.of_match_table = mtkir_of_device_ids,
#endif
		.name	= "mtk-ir",
		.owner	= THIS_MODULE,
		.bus   = &platform_bus_type,
	}
};

static int __init mtk_ir_drv_init_module(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_ir_driver);
	if (ret)
		IRDBG_ERR("Register MTK IR Platform Driver Failed!");
	if (profile_num == 0) {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
#ifdef CONFIG_MTK_TV_IR_KEYMAP_MTK_NEC
	init_key_map_mtk_tv();
#endif
#ifdef CONFIG_MTK_TV_IR_KEYMAP_SONY
	init_key_map_sony_tv();
#endif
#ifdef CONFIG_IR_KEYMAP_TCL_RCA
	init_key_map_tcl_rca_tv();
#endif
#ifdef CONFIG_IR_KEYMAP_TCL
	init_key_map_tcl_tv();
#endif
#ifdef CONFIG_IR_KEYMAP_KONKA
	init_key_map_konka_tv();
#endif
#ifdef CONFIG_IR_KEYMAP_KATHREIN
	init_key_map_rc6_kathrein();
#endif
#ifdef CONFIG_IR_KEYMAP_RC5
	init_key_map_rc5_tv();
#endif
#ifdef CONFIG_IR_KEYMAP_SAMPO
	init_key_map_rc311_samp();
#endif


#endif
	}
	return ret;
}

static void __exit mtk_ir_drv_exit_module(void)
{
	int i = 0;
	platform_driver_unregister(&mtk_ir_driver);

	if (profile_num == 0) {
#ifdef CONFIG_MTK_TV_MIRC_INPUT_DEVICE
#ifdef CONFIG_MTK_TV_IR_KEYMAP_MTK_NEC
	exit_key_map_mtk_tv();
#endif
#ifdef CONFIG_MTK_TV_IR_KEYMAP_SONY
	exit_key_map_sony_tv();
#endif
#ifdef CONFIG_IR_KEYMAP_TCL_RCA
	exit_key_map_tcl_rca_tv();
#endif
#ifdef CONFIG_IR_KEYMAP_TCL
	exit_key_map_tcl_tv();
#endif
#ifdef CONFIG_IR_KEYMAP_KONKA
	exit_key_map_konka_tv();
#endif
#ifdef CONFIG_IR_KEYMAP_KATHREIN
	exit_key_map_rc6_kathrein();
#endif
#ifdef CONFIG_IR_KEYMAP_RC5
	exit_key_map_rc5_tv();
#endif

#endif
	} else {
		kfree(key_map_listn);
		for (i = 0; i < profile_num ; i++)
			kfree(table[i]);
	}
}

module_init(mtk_ir_drv_init_module);
module_exit(mtk_ir_drv_exit_module);

MODULE_AUTHOR("jefferry.yen@mediatek.com");
MODULE_DESCRIPTION("IR driver");
MODULE_LICENSE("GPL");
