// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
 * Author polar.chou <polar.chou@mediatek.com>
 */
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/freezer.h>
#include <linux/wait.h>

#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/uaccess.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <media/rc-core.h>
#include "mtk-rc-riu.h"

#define DRIVER_NAME    "mtk-ir-tx"
#define DEVICE_NAME    "MTK IRTX Transmitter"

#define TAG "[IRTX] "
#define LOGI(format, ...) do { \
	if (_verbose) \
		pr_info(TAG "%s:%d " format, __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#define LOGE(format, ...) pr_err(TAG "%s:%d " format, __func__, __LINE__, ##__VA_ARGS__)
//Define IR	 IRQ Vector
void __iomem *irtx_map_membase;
#define MAX_IRTX_UNITS               14//Set unitXX(XX = 00~13)
#define IR_TX_UNIT_ENDMARK           0x0e

#define IR_TX_REG_BASE               (0x000000UL)//bank0x311
#define IR_TX_MEM_BASE               (0x000100UL)//bank0x312

#define IR_TX_SET_REG(x)             (((x)+IR_TX_REG_BASE)*2)//bank0x311
#define IR_TX_MEM_SET_REG(x)         (((x)+IR_TX_MEM_BASE)*2)//bank0x312

//---------------------------------------------
// definition for IR TX status
//---------------------------------------------
#define IR_TX_REG_UNIT_VALUE_L    IR_TX_SET_REG(0x38UL)
#define IR_TX_REG_UNIT_VALUE_H    IR_TX_SET_REG(0x3AUL)
#define IR_TX_REG_CYCLETIME_H     IR_TX_SET_REG(0x3CUL)
#define IR_TX_REG_CYCLETIME_L     IR_TX_SET_REG(0x3EUL)
#define IR_TX_REG_CLKDIV          IR_TX_SET_REG(0x40UL)
#define IR_TX_CARRIER_CNT         IR_TX_SET_REG(0x42UL)
#define IR_TX_STATUS              IR_TX_SET_REG(0x44UL)
#define IR_TX_FLAG                IR_TX_SET_REG(0x46UL)
#define IR_TX_DONE                IR_TX_SET_REG(0x48UL)

#define IR_TX_REG_UNIT(unit, hl)  IR_TX_SET_REG((unit)*4+(hl)*2)

#define IR_TX_MEM_REG_STATUS      IR_TX_MEM_SET_REG(0x02UL)
#define IR_TX_MEM_REG_ADDR        IR_TX_MEM_SET_REG(0x04UL)
#define IR_TX_MEM_REG_DATA        IR_TX_MEM_SET_REG(0x0CUL)

#define IR_TX_STATUS_EN           (1L << 0)
#define IR_TX_STATUS_RESET        (1L << 1)
#define IR_TX_STATUS_INT_EN       (1L << 2)
#define IR_TX_STATUS_POLARITY     (1L << 3)
#define IR_TX_STATUS_CARRIER_EN   (1L << 4)
#define IR_TX_STATUS_TEST_SEL     (3L << 8)

#define MCU_CLK                   216000UL//depend on reg_ckg_mcu in PM_TOP bank
#define IR_TX_TIMEOUT_COUNT       1000UL // 10ms~10ms
#define IR_TX_UNIT_COUNT_MAX      0x10000UL
#define IR_TX_MAGIC               0x5874
//-------------------------------------------------------------------------------------------------
// Macro and Define
//-------------------------------------------------------------------------------------------------
//IRTX_IOC
#define	IRTX_IOCTL_BASE	'W'
#define	IRTXIOC_WRITEDATA	_IOR(IRTX_IOCTL_BASE, 0, int)
static int irtx_debug;
static int _max_irtx_units = 14;
static int pad;
static int _verbose = 1;
static int _irtx_timeout = IR_TX_TIMEOUT_COUNT;
static int _timeout_early = 300;
static uint _clk_source;
static int _wait_ready;
static int _sync;
static int _async_id;
module_param(pad, int, 0644);
module_param_named(verbose, _verbose, int, 0644);
module_param_named(timeout, _irtx_timeout, int, 0644);
module_param_named(early, _timeout_early, int, 0644);
module_param_named(clk, _clk_source, uint, 0644);
module_param_named(units, _max_irtx_units, int, 0644);
module_param_named(wait_ready, _wait_ready, int, 0644);
module_param_named(sync, _sync, int, 0644);

struct miscdata {
int val;
char *str;
unsigned int size;
};
struct mtk_ir_dev {
	struct miscdevice misc;
	struct miscdata data;
	void __iomem *reg_base;
	struct device *dev;
	u32 sample_rate;
	u32 ckdiv;
	u32 duty_cycle;
	u32 carrier;
	u16 carrier_count[2];
	bool inverse;
	struct mutex lock;
	u32 ir_tx_status;
	// async mode variables
	struct task_struct *kthread;
	spinlock_t async_lock;
	struct wait_queue_head wait;
	struct list_head tx_head;
	struct rc_dev *rcdev;
	bool carrierreport;
};

struct irtx_data {
	char *data;
	u32 count;
	int id;
	struct list_head list;
};

static struct mtk_ir_dev _irtxdev = {
	.sample_rate = 6000000,
	.duty_cycle = 40,
	.carrier = 38000,
	.ir_tx_status = IR_TX_STATUS_EN | IR_TX_STATUS_RESET |
		IR_TX_STATUS_INT_EN | IR_TX_STATUS_CARRIER_EN | IR_TX_STATUS_POLARITY,
};

static u32 _div_round(u32 x, u32 y)
{
	return (x + (y/2)) / y;
}

static inline u16 irtx_read(u32 reg)
{
	return mtk_readw_relaxed((void *)(irtx_map_membase+reg));
}

static inline void irtx_write(u32 reg, uint16_t value)
{
	mtk_writew_relaxed((void *)(irtx_map_membase+reg), value);
}

static int _mtk_ir_set_carrier_duty(struct mtk_ir_dev *dev)
{
	u32 ckdiv;
	u32 carrier_h, carrier_l;
	u32 carrier;
	u16 reg;
	const u32 clk_hz = _clk_source * 1000;

	dev->ckdiv = _div_round(clk_hz, dev->sample_rate);
	ckdiv = dev->ckdiv - 1;

	if (ckdiv & ~0xff) {
		pr_info("ckdiv overflow(>0xff)! chdiv = 0x%04x for sample_rate = %d\n",
				ckdiv, dev->sample_rate);
		return 1;
	}

	carrier = _div_round(clk_hz, dev->ckdiv * dev->carrier);
	carrier_h = _div_round(carrier * dev->duty_cycle, 100);
	carrier_l = carrier - carrier_h;

	if (carrier_h & ~0xff || carrier_l & ~0xff) {
		pr_info("carrier reg overflow(>0xff)! carrier(h,l) = (0x%04x,0x%04x) for sample_rate = %d and carrier = %d\n",
				carrier_h, carrier_l, dev->sample_rate, dev->carrier);
		return 1;
	}

	pr_info("ckdiv=0x%02x, carrier_h=0x%02x, carrier_l=0x%02x\n",
			ckdiv, carrier_h, carrier_l);

	reg = irtx_read(IR_TX_REG_CLKDIV);
	reg = (reg & ~0xff00) | (ckdiv << 8);
	irtx_write(IR_TX_REG_CLKDIV, reg);

	reg = (carrier_l << 8) | carrier_h;
	irtx_write(IR_TX_CARRIER_CNT, reg);

	dev->carrier_count[0] = carrier_h;
	dev->carrier_count[1] = carrier_l;
	return 0;
}

static inline void _irtx_set_mem_data(u32 *code, int *shift,
		u32 unit, bool force)
{
	*code = *code | (unit << *shift);
	*shift += 4;
	if (*shift == 16 || force) {
		irtx_write(IR_TX_MEM_REG_DATA, (u16)*code);
		*shift = 0;
		*code = 0;
	}
}

static int _irtx_set_index_data(struct mtk_ir_dev *dev,
		u32 *unitbuf, u32 units, u32 *indexbuf, u32 indexes,
		u32 *head, u8 *memdata)
{
	int i;
	u16 unit_tag[MAX_IRTX_UNITS];
	u16 unit_repeat[MAX_IRTX_UNITS];
	int u;
	int max_tag = 0, idx = 0;
	int ret = 0;
	u32 total_count = 0;
	const u32 carrier_count = dev->carrier_count[0] + dev->carrier_count[1];
	const u32 carrier_offset = dev->carrier_count[0] + dev->carrier_count[1]/2;
	const u32 clk_mhz = _clk_source / 1000;
	u32 tail = *head;
	u32 total_us = 0;
	u32 code = 0;
	int shift = 0;

	// scan indexbuf
	for (i = *head; i < indexes; i += 2) {
		u16 tag = (indexbuf[i]<<8) | (indexbuf[i+1]);

		for (u = 0; u < max_tag; u++) {
			if (unit_tag[u] == tag)
				break;
		}
		if (u == max_tag) {
			if (u >= _max_irtx_units)
				break;
			unit_tag[max_tag++] = tag;
		}
		memdata[idx++] = u;
		tail += 2;
	}

	// setup long idle signel
	irtx_write(IR_TX_REG_UNIT(IR_TX_UNIT_ENDMARK-1, 0), IR_TX_UNIT_COUNT_MAX/2);
	irtx_write(IR_TX_REG_UNIT(IR_TX_UNIT_ENDMARK-1, 1), IR_TX_UNIT_COUNT_MAX/2);
	irtx_write(IR_TX_REG_UNIT_VALUE_L, 0xaaaa);
	irtx_write(IR_TX_REG_UNIT_VALUE_H, 0x02aa);

	irtx_write(IR_TX_MEM_REG_STATUS, 0x5100);
	// set unit data
	for (i = 0; i < max_tag; i++) {
		u8 level1, level2;
		u32 count1, count2;
		u32 tmp;

		level1 = unit_tag[i] >> 8;
		level2 = unit_tag[i] & 0xff;
		count1 = _div_round(unitbuf[level1]*(clk_mhz), dev->ckdiv);
		count2 = _div_round(unitbuf[level2]*(clk_mhz), dev->ckdiv);

		// Modify count1 to fit carrier boundary
		// The high signal counter should be stop in unit level2
		// Or the signal will be keeped in high on low signal.
		tmp = count1 <= carrier_offset ? 0 : count1 - carrier_offset;
		tmp = _div_round(tmp, carrier_count) * carrier_count + carrier_offset;
		pr_info("fix level1 count from 0x%04x to 0x%04x\n", count1, tmp);
		count1 = tmp;

		// fix long low timing
		unit_repeat[i] = 0;
		if (count2 >= IR_TX_UNIT_COUNT_MAX) {
			unit_repeat[i] = (u16)(count2 / IR_TX_UNIT_COUNT_MAX);
			tmp = count2 % IR_TX_UNIT_COUNT_MAX;
			pr_info("fix long level2 from 0x%04x to 0x%04x + %d repeats\n",
					count2, tmp, (int)unit_repeat[i]);
			count2 = tmp;
			// HW count cannot be 0
			if (count2 == 0)
				count2 = 1;
		}
		irtx_write(IR_TX_REG_UNIT(i, 0), count1);
		irtx_write(IR_TX_REG_UNIT(i, 1), count2);
		pr_info("add unit pair %d th (%d us, %d us)\n", i,
				unitbuf[level1], unitbuf[level2]);
	}

	irtx_write(IR_TX_MEM_REG_ADDR, 0);

	for (i = 0; i < idx; i++) {
		int j;
		u32 unit = memdata[i];
		u32 count;
		u8 level1, level2;

		if (unit >= max_tag) {
			pr_info("index data overflow: index[%d] = %d >= %d\n",
					i, unit, units);
			ret = -EFBIG;
			goto cleanup;
		}
		level1 = unit_tag[unit] >> 8;
		level2 = unit_tag[unit] & 0xff;
		total_us += unitbuf[level1] + unitbuf[level2];
		pr_info("get index data %d(th) = %d, (%d us, %d us)\n", i, unit,
				unitbuf[level1], unitbuf[level2]);

		count = irtx_read(IR_TX_REG_UNIT(i, 0));
		count += irtx_read(IR_TX_REG_UNIT(i, 1));
		total_count += count;
		_irtx_set_mem_data(&code, &shift, unit, 0);
		for (j = 0; j < unit_repeat[unit]; j++) {
			total_count += IR_TX_UNIT_COUNT_MAX;
			_irtx_set_mem_data(&code, &shift, IR_TX_UNIT_ENDMARK-1, 0);
		}
	}
	_irtx_set_mem_data(&code, &shift, IR_TX_UNIT_ENDMARK, 1);

	// set total cycle count
	pr_info("set total cycle count = %d (0x%08x), total time = %d us\n",
			total_count, total_count, total_us);
	irtx_write(IR_TX_REG_CYCLETIME_L, total_count & 0xffff);
	irtx_write(IR_TX_REG_CYCLETIME_H, total_count >> 16);

cleanup:
	if (ret == 0) {
		pr_info("send %d ~ %d , total %d\n", *head+1, tail, indexes);
		*head = tail;
		return total_us;
	}
	return ret;
}

static int mtk_ir_tx(struct mtk_ir_dev *dev, u32 *unitbuf, u32 units,
		u32 *indexbuf, u32 indexes)
{
	u32 head = 0;
	int ret = 0;
	u32 timeout = 0;
	u8 *memdata;
	ktime_t ts, te;

	// mutex_lock(&dev->lock);

	memdata = kmalloc(indexes/2, GFP_KERNEL);
	if (memdata == NULL) {
		ret = -ENOMEM;
		goto exit_irtx;
	}

	ret = _mtk_ir_set_carrier_duty(dev);
	if (ret)
		goto exit_irtx;

	while (head < indexes) {
		s32 total_us;

		irtx_write(IR_TX_STATUS, IR_TX_STATUS_RESET);
		total_us = _irtx_set_index_data(dev, unitbuf, units,
				indexbuf, indexes, &head, memdata);
		if (total_us < 0) {
			ret = total_us;
			pr_info("irtx set index data fail! (%d)\n", ret);
			goto exit_irtx;
		}

		irtx_write(IR_TX_STATUS, dev->ir_tx_status);
		irtx_write(IR_TX_FLAG, 1);

		if (_verbose) {
			ts = ktime_get();
			pr_info("pre-check done flag: %d\n", irtx_read(IR_TX_DONE));
		}
		if (_wait_ready) {
			if (total_us < _timeout_early)
				total_us = _timeout_early;
			usleep_range(total_us-_timeout_early, total_us-_timeout_early+40);

			// check the ir tx done bit and clear done bit
			for (timeout = 0; timeout < _irtx_timeout; timeout++) {
				if (irtx_read(IR_TX_DONE)) {
					irtx_write(IR_TX_DONE, 1);
					break;
				}
				usleep_range(10, 20);
			}
			if (_verbose) {
				te = ktime_get();
				pr_info("check done flag in %lld us, loop %d time(s)\n",
						ktime_us_delta(te, ts), timeout);
			}
			if (timeout >= _irtx_timeout) {
				pr_info("IR_TX_timeout... give up\n");
				ret = -EBUSY;
				goto exit_irtx;
			}
		} else {
			usleep_range(total_us, total_us+50);
			if (_verbose)
				pr_info("post-check done flag: %d\n", irtx_read(IR_TX_DONE));
		}
	}

exit_irtx:
	kfree(memdata);
	// mutex_unlock(&dev->lock);
	return 0;
}

static ssize_t do_mtk_ir_tx(const char *buf, size_t count)
{
	const int u32sz = sizeof(u32);
	u32 *data = (u32 *)buf;
	int datalen;
	int unit_len;
	int index_len;
	u32 *unitbuf;
	u32 *indexbuf;
	int ret = 0;

	if (count > 4) {
		u16 tag = data[0] >> 16;
		u16 version = data[0] & 0xffff;

		if (tag != IR_TX_MAGIC || version != 0x0001) {
			pr_info("irtx_send: wrong header magic(0x%04x) or version(%d)\n",
					tag, version);
			return -EINVAL;
		}
		data++;
	}
	// check minimal data length
	datalen = 2;
	if (count < datalen*u32sz) {
		pr_info("irtx_send: data header count = %d < %d ! skip\n",
				count, datalen*u32sz);
		return -EINVAL;
	}
	// check data length
	unit_len = data[datalen];
	unitbuf = &data[datalen+1];
	datalen += unit_len+1;
	if (count < datalen*u32sz) {
		pr_info("irtx_send: data unit count = %d < %d ! skip\n",
				count, datalen*u32sz);
		return -EINVAL;
	}

	index_len = data[datalen];
	indexbuf = &data[datalen+1];
	datalen += index_len+1;
	if (count < datalen*u32sz) {
		pr_info("irtx_send: data index count = %d < %d ! skip\n",
				count, datalen*u32sz);
		return -EINVAL;
	}

//#if 0
//	if (unit_len > MAX_IRTX_UNITS) {
//		pr_info("irtx_send: support maximum units %d (<%d)! skip\n",
//				MAX_IRTX_UNITS, unit_len);
//		return -EINVAL;
//	}
//#endif

	// print data for debug
	if (_verbose) {
		pr_info("irtx_send: duty_cycle = %d, carrier = %d\n", data[0], data[1]);
		print_hex_dump(KERN_INFO, "txbuf ", DUMP_PREFIX_OFFSET,
				16, 4, data, count, true);
		//pr_info("unit data: len = %d, databuf= 0x%08x\n", unit_len, unitbuf);
		print_hex_dump(KERN_INFO, "units ", DUMP_PREFIX_OFFSET,
				16, 4, unitbuf, unit_len*u32sz, true);
		//pr_info("index data: len = %d, databuf= 0x%08x\n",
		//		index_len, data+3+unit_len);
		print_hex_dump(KERN_INFO, "units ", DUMP_PREFIX_OFFSET,
				16, 4, indexbuf, index_len*u32sz, true);
	}

	_irtxdev.duty_cycle = data[0];
	_irtxdev.carrier = data[1];
	_irtxdev.ir_tx_status = IR_TX_STATUS_EN | IR_TX_STATUS_RESET |
			IR_TX_STATUS_INT_EN | IR_TX_STATUS_CARRIER_EN | IR_TX_STATUS_POLARITY;
	ret = mtk_ir_tx(&_irtxdev, unitbuf, unit_len, indexbuf, index_len);

	if (ret < 0)
		return ret;

	return count;
}

static ssize_t auto_do_mtk_ir_tx(void)
{
	const int u32sz = sizeof(u32);

	return 0;
}

static int _irtx_kthread(void *p)
{
	struct mtk_ir_dev *dev = p;

	while (1) {
		struct irtx_data *txdata;

		wait_event_freezable(dev->wait,
				     !list_empty(&dev->tx_head) || kthread_should_stop());

		if (kthread_should_stop())
			break;

		txdata = list_first_entry_or_null(&dev->tx_head, struct irtx_data, list);
		if (txdata) {
			unsigned long flags;

			pr_info("send irtx id:%d\n", txdata->id);
			do_mtk_ir_tx(txdata->data, txdata->count);
			spin_lock_irqsave(&dev->async_lock, flags);
			list_del(&txdata->list);
			spin_unlock_irqrestore(&dev->async_lock, flags);
			kfree(txdata->data);
			kfree(txdata);
		}
	}
	return 0;
}

static ssize_t irtx_send_sysfs_store(struct kobject *kobj, struct kobj_attribute *attr
				     , const char *buf, size_t count)
{
	struct mtk_ir_dev *dev = &_irtxdev;
	struct irtx_data *tx = NULL;
	unsigned long flags;

	if (_sync)
		return do_mtk_ir_tx(buf, count);

	tx = kzalloc(sizeof(*tx), GFP_KERNEL);
	if (!tx)
		return -ENOMEM;

	tx->data = kmemdup(buf, count, GFP_KERNEL);
	if (!tx->data) {
		kfree(tx);
		return -ENOMEM;
	}
	tx->count = count;
	INIT_LIST_HEAD(&tx->list);

	spin_lock_irqsave(&dev->async_lock, flags);
	tx->id = ++_async_id;
	list_add_tail(&tx->list, &dev->tx_head);
	spin_unlock_irqrestore(&dev->async_lock, flags);
	pr_info("add async request %d to irtx queue\n", tx->id);

	wake_up_interruptible(&dev->wait);

	return count;
}

static ssize_t irtx_send_sysfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "sample_rate: %d\n", _irtxdev.sample_rate);
	len += snprintf(buf+len, PAGE_SIZE-len, "duty_cycle: %d\n", _irtxdev.duty_cycle);
	len += snprintf(buf+len, PAGE_SIZE-len, "carrier: %d\n", _irtxdev.carrier);
	return len;
}

static struct kobj_attribute kobj_attr_irtx_send =
	__ATTR(send, 0660, irtx_send_sysfs_show, irtx_send_sysfs_store);

static struct kobject *kobj_ref_irtx;


static ssize_t mtk_irtxmisc_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{

	int len = 1;

	return len;
}

int mtk_irtxmisc_open(struct inode *inode, struct file *filep)
{

	return 0;
}
int mtk_irtxmisc_release(struct inode *inode, struct file *filep)
{
	return 0;
}
/**
 *
 * Debug command :
 *
 * echo "irtx_test" > /dev/irtx
 *      : generate NEC NUM_1
 *
 * Debug command :
 *
 * echo "debug_enable" > /dev/irtx
 *      : show irtx flow.
 *
 * echo "debug_disable" > /dev/irtx
 *      : disable show irtx flow.
 *
 */

static ssize_t mtk_irtxmisc_write(struct file *file, const char __user *buf,
				  size_t count, loff_t *ppos)
{
	char tmp[64];
	int unit_len = 8;
	u32 unitbuf[8] = {4500, 560, 560, 1680, 96152, 9000, 20000, 16000};
	int index_len = 70;
	size_t ret = 0;
	//Use NEC NUM_1 to test
	u32 indexbuf[70] = {7, 7, 5, 0, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 3, 1, 3, 1, 3,
			    1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 2, 1, 3, 1, 2, 1, 2, 1, 3, 1, 2, 1, 2,
			    1, 3, 1, 2, 1, 2, 1, 3, 1, 3, 1, 2, 1, 3, 1, 3, 1, 2, 1, 3, 1, 3};

	_irtxdev.duty_cycle = 40;
	_irtxdev.carrier = 38000;
	_irtxdev.ir_tx_status = IR_TX_STATUS_EN | IR_TX_STATUS_RESET |
				(IR_TX_STATUS_INT_EN & IR_TX_STATUS_CARRIER_EN) | IR_TX_STATUS_POLARITY;

	memset(tmp, 0, 64);
	if (count > 64 || copy_from_user(tmp, buf, count)) {
		pr_info("copy_from_user fail\n");
		return count;
	}

	if (strncmp(tmp, "irtx_test", strlen("irtx_test")) == 0) {
		mtk_ir_tx(&_irtxdev, unitbuf, unit_len, indexbuf, index_len);
		pr_info("irtx_test!!!!\n");
	} else if (strncmp(tmp, "debug_enable", strlen("debug_enable")) == 0) {
		irtx_debug = 1;
		pr_info("Use irtx debug enable\n");
	} else if (strncmp(tmp, "debug_disable", strlen("debug_disable")) == 0) {
		irtx_debug = 0;
		pr_info("Use irtx debug disable\n");
	}

	//To-Do :Fix it
	//do_mtk_ir_tx(buf, count);
	return count;
}
//For LTP
long mtk_irtx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	int unit_len = 8;
	u32 unitbuf[8] = {4500, 560, 560, 1680, 96152, 9000, 20000, 16000};
	int index_len = 70;
	//Use NEC NUM_1 to LTP test
	u32 indexbuf[70] = {7, 7, 5, 0, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 3, 1, 3, 1, 3,
			    1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 2, 1, 3, 1, 2, 1, 2, 1, 3, 1, 2, 1, 2,
			    1, 3, 1, 2, 1, 2, 1, 3, 1, 3, 1, 2, 1, 3, 1, 3, 1, 2, 1, 3, 1, 3};

	_irtxdev.duty_cycle = 40;
	_irtxdev.carrier = 38000;
	_irtxdev.ir_tx_status = IR_TX_STATUS_EN | IR_TX_STATUS_RESET |
			(IR_TX_STATUS_INT_EN & IR_TX_STATUS_CARRIER_EN) | IR_TX_STATUS_POLARITY;

	switch (cmd) {
	case IRTXIOC_WRITEDATA:
		pr_info("IRTXIOC_WRITEDATA\n");
		ret = mtk_ir_tx(&_irtxdev, unitbuf, unit_len, indexbuf, index_len);
		break;

	default:
		pr_info("No cmd\n");
		ret = -EINVAL;
		break;

	}

	return ret;
}
static int mtk_set_carrier_report(struct rc_dev *dev, int enable)
{
	struct mtk_ir_dev *mtk_irtx = dev->priv;

	if (mtk_irtx->carrierreport != enable) {
		pr_info("%sabling carrier reports\n", enable ? "en" : "dis");
		mtk_irtx->carrierreport = !!enable;
	}

	return 0;
}
static int mtk_cir_set_duty_cycle(struct rc_dev *dev, u32 duty_cycle)
{
	struct mtk_ir_dev *mtk_irtx = dev->priv;

	_irtxdev.duty_cycle = duty_cycle;
	pr_info("new duty_cycle %d\n", _irtxdev.duty_cycle);
	return 0;
}

static int mtk_cir_set_carrier(struct rc_dev *dev, u32 carrier)
{
	struct mtk_ir_dev *mtk_irtx = dev->priv;

	if (!carrier)
		return -EINVAL;

	_irtxdev.carrier = carrier;
	pr_info("new carrier %d\n", _irtxdev.carrier);
	return 0;
}
static int mtk_cir_buf2index(unsigned int *indexbuf, u32 *unitbuf, unsigned int unit_len,
			     unsigned int *txbuf, unsigned int index_len)
{
	unsigned int i,j;
	u32 *unitbuf_tmp = NULL;
	unsigned int malloc_len = unit_len * sizeof(unsigned int);

	unitbuf_tmp = kmalloc(malloc_len, GFP_KERNEL);
	if (unitbuf_tmp == NULL)
		goto exit_buf2index;

	memset(unitbuf_tmp, 0, unit_len);

	//Calculate data range
	for (i = 0; i < unit_len - 1; i++)
		unitbuf_tmp[i+1] = (unitbuf[i]+unitbuf[i+1])/2;


	for (i = 0; i < unit_len; i++)
		pr_info("unitbuf[%d] :%d,unitbuf_tmp[%d] :%d\n", i, unitbuf[i], i, unitbuf_tmp[i]);
	//Change shot count to unitbuf index
	for (i = 0; i < index_len; i++)
		for (j = 0; j < unit_len; j++)
			if (txbuf[i]  > unitbuf_tmp[j])
				indexbuf[i] = j;
	for (i = 0; i < index_len; i++)
		pr_info("indexbuf :%d\n", indexbuf[i]);
exit_buf2index:
	kfree(unitbuf_tmp);
	return 0;
}
static int mtk_cir_txfunc(struct rc_dev *dev, unsigned int *txbuf,
			  unsigned int count)
{
	unsigned int i;
	unsigned int unit_len = 13;
	u32 unitbuf[13] = {160, 300, 420, 560, 700, 880, 980, 1690, 4500, 9000, 16000, 46308,
			   96152};
	unsigned int index_len = count + 1;

	unsigned int *indexbuf;
	unsigned int malloc_len = index_len * sizeof(unsigned int);

	indexbuf = kmalloc(malloc_len, GFP_KERNEL);
	if (indexbuf == NULL)
		goto exit_txfunc;
	memset(indexbuf, 0, index_len);

	//put txbuf[0] to indexbuf[1]
	mtk_cir_buf2index(indexbuf + 1, unitbuf, unit_len, txbuf, index_len);

	_irtxdev.ir_tx_status = IR_TX_STATUS_EN | IR_TX_STATUS_RESET |
			IR_TX_STATUS_INT_EN | IR_TX_STATUS_CARRIER_EN;
	if (irtx_debug == 1) {
		pr_info("=========================\n");
		pr_info("indexbuf [%d]:\n", count);
		for (i = 0; i < index_len; i++)
			pr_info("%d,", indexbuf[i]);
		pr_info("=========================\n");
		pr_info("unitbuf [%d]:\n", unit_len);
		for (i = 0; i < unit_len; i++)
			pr_info("%d,", unitbuf[i]);
		pr_info("=========================\n");
		pr_info("\n(duty_cycle,carrier,ir_tx_status) = (%d ,%d ,%d)\n",
			_irtxdev.duty_cycle, _irtxdev.carrier, _irtxdev.ir_tx_status);
		pr_info("=========================\n");
	}

	mtk_ir_tx(&_irtxdev, unitbuf, unit_len, indexbuf, index_len);

exit_txfunc:
	kfree(indexbuf);
	return count;
}
static const struct file_operations mtk_irtx_fops = {
	.owner = THIS_MODULE,
	.open = mtk_irtxmisc_open,
	.read = mtk_irtxmisc_read,
	.write = mtk_irtxmisc_write,
	.release = mtk_irtxmisc_release,
	.unlocked_ioctl = mtk_irtx_ioctl,
	.compat_ioctl = mtk_irtx_ioctl,
};

static struct miscdevice mtk_irtx_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "irtx",
	.fops = &mtk_irtx_fops,
};


static int mtk_irtx_probe(struct platform_device *pdev)
{
	int error = 0;
	struct mtk_ir_dev *pdata;
	struct resource *res;
	int ret = 0;
	struct device_node *np = NULL;
	struct rc_dev *rcdev;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	platform_set_drvdata(pdev, pdata);
	pdata->dev = &pdev->dev;

	// pdev->dev.platform_data = &mtk_irtx_misc;


	np = pdev->dev.of_node;
	if (!np)
		pr_info("%s:fail to find node\n", __func__);

	if (of_property_read_u32(np, "clock-frequency", &_clk_source)) {
		_clk_source = MCU_CLK;
		pr_info("irb set _clk_source to default value (%d)\n", _clk_source);
	} else {
		_clk_source = _clk_source/1000;
		pr_info("irb _clk_source (%d)\n", _clk_source);
	}
	/* irtx reg */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pdata->reg_base = devm_ioremap_resource(&pdev->dev, res);
	irtx_map_membase = pdata->reg_base;
	if (IS_ERR(pdata->reg_base)) {
		dev_err(&pdev->dev, "err reg_base=0x%lx\n",
			(unsigned long)pdata->reg_base);
		return PTR_ERR(pdata->reg_base);
	}

	// pr_info("===>reg=0x%08x\n", pdata->reg_base);


	memset(&(pdata->data), 0, sizeof(pdata->data));
	pdata->misc = mtk_irtx_misc;
	error = misc_register(&mtk_irtx_misc);

	rcdev = devm_rc_allocate_device(&pdev->dev, RC_DRIVER_IR_RAW_TX);
	if (!rcdev)
		return -ENOMEM;

	rcdev->priv = pdev;
	rcdev->driver_name = DRIVER_NAME;
	rcdev->device_name = DEVICE_NAME;
	rcdev->tx_ir = mtk_cir_txfunc;
	rcdev->s_tx_duty_cycle = mtk_cir_set_duty_cycle;
	rcdev->s_tx_carrier = mtk_cir_set_carrier;
	rcdev->s_carrier_report	= mtk_set_carrier_report;
	error = devm_rc_register_device(&pdev->dev, rcdev);

	if (error < 0)
		pr_info("failed to register rc device %d\n",error);

	pdata->rcdev = rcdev;

	pr_info("%s:misc_register %d\n", __func__, error);

	return 0;
}

static int mtk_irtx_remove(struct platform_device *pdev)
{
	int error = 0;

	misc_deregister(&mtk_irtx_misc);
	return error;
}
#ifdef CONFIG_PM
static int mtk_irtx_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int mtk_irtx_drv_resume(struct platform_device *pdev)
{
	return 0;
}
#endif
static const struct of_device_id of_mtk_irtx_match[] = {
	{ .compatible = "mtk-irtx", },
	{},
};
MODULE_DEVICE_TABLE(of, of_mtk_irtx_match);

static struct platform_driver mtk_irtx_driver = {
	.probe		= mtk_irtx_probe,
	.remove		= mtk_irtx_remove,
	#ifdef CONFIG_PM
	.suspend	= mtk_irtx_drv_suspend,
	.resume		= mtk_irtx_drv_resume,
	#endif
	.driver		= {
		.name	= "mtk_irtx",
		.of_match_table = of_mtk_irtx_match,
	},
};

module_platform_driver(mtk_irtx_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("polar.chou@mediatek.com");
MODULE_DESCRIPTION("MTK IR TX driver");
