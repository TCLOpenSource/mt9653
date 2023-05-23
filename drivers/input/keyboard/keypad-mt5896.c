// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Owen.Tseng <Owen.Tseng@mediatek.com>
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/timekeeping.h>
#include <linux/freezer.h>
#include <linux/of_address.h>
#include "keypad-mt5896-coda.h"
#include "keypad-mt5896-riu.h"
#include "keypad5896.h"

#define SAR_CTRL_SINGLE_MODE			(1 << 4)
#define SAR_CTRL_DIG_FREERUN_MODE		(1 << 5)
#define SAR_CTRL_ATOP_FREERUN_MODE		(1 << 9)
#define SAR_CTRL_8_CHANNEL				(1 << 0xb)
#define SAR_CTRL_LOAD_EN				(1 << 0xe)
#define SAR_CH_MAX						4
#define SAR_KEY_MAX						8
#define SAR_CHANNEL_USED_MAX			2
#define TASK_POLLING_TIMES				100//msec
#define PM_SLEEP_INTERRUPT_NUM			0x21

#define MTK_KEYPAD_DEVICE_NAME			"MTK TV KEYPAD"
#define INPUTID_VENDOR					0x3697UL
#define INPUTID_PRODUCT					0x0002
#define INPUTID_VERSION					0x0002
#define RELEASE_KEY						0xFF
#define DEFAULT_DEBOUNCE				200  //200msec
#define DEFAULT_DEBOUNCE_MAX			1000  //1sec
#define RELEASE_ADC						0x3F0
#define REG_0010_PM_SLEEP				(0x010)
#define SAR_WAKE_MASK					(1 << 1)
#define KEY_STR_LENSIZE					32
#define KEY_NUMS						8
#define BYTE_SHIFT						8
#define OFFSET_IDX						2

enum mtk_keypad_type {
	KEYPAD_TYPE_MTK,
	KEYPAD_TYPE_CUSTOMIZED0,
};

typedef struct {
	unsigned int keycode;
	unsigned int keythresh_vol;
} list_key;


struct mtk_keypad {
	struct input_dev *input_dev;
	struct platform_device *pdev;
	void __iomem *base;
	void __iomem *base_p;
	wait_queue_head_t wait;
	list_key scanmap[SAR_KEY_MAX];
	bool stopped;
	bool wake_enabled;
	int irq;
	enum mtk_keypad_type type;
	unsigned int row_state[SAR_CHANNEL_USED_MAX];
	unsigned int *keycodes;
	unsigned int *keythreshold;
	unsigned int  chanel;
	unsigned int  low_bound;
	unsigned int  high_bound;
	unsigned int  key_num;
	unsigned int  key_adc;
	unsigned int  prekey[SAR_CHANNEL_USED_MAX];
	unsigned int  pressed[SAR_CHANNEL_USED_MAX];
	unsigned int  ch_detected;
	unsigned int  repeat_count;
	unsigned int  debounce_time;
	bool no_autorepeat;
	unsigned int  reset_timeout;
	unsigned int  reset_key;
	struct kobject *keypad_kobj;
};

static unsigned int		keystored_t;
static unsigned char	_uprelease;
static unsigned char	_downtrig;
static ktime_t			_start_time;


static void mtk_keypad_map(struct mtk_keypad *keypad)
{
	unsigned int i;
	unsigned int j;
	unsigned int keynum;

	list_key tmp;
	list_key buffer[SAR_KEY_MAX+1];

	keynum = keypad->key_num;
	for (i = 0; i < keynum; i++) {
		buffer[i].keycode = (keypad->keycodes)[i];
		buffer[i].keythresh_vol = (keypad->keythreshold)[i];
	}

	for (i = 0; i < keynum; i++) {
		for (j = i; j < keynum; j++) {
			if (buffer[i].keythresh_vol > buffer[j].keythresh_vol) {
				memcpy(&tmp, &buffer[i], sizeof(list_key));
				memcpy(&buffer[i], &buffer[j], sizeof(list_key));
				memcpy(&buffer[j], &tmp, sizeof(list_key));
			}
		}
	}

	for (i = 0; i < keypad->key_num; i++) {
		if (i == (keypad->key_num - 1))
			buffer[i+1].keythresh_vol = 0x3FF;
		buffer[i].keythresh_vol = (buffer[i].keythresh_vol +
							buffer[i+1].keythresh_vol)/2;
		keypad->scanmap[i].keycode = buffer[i].keycode;
		keypad->scanmap[i].keythresh_vol = buffer[i].keythresh_vol;
	}
}


static void mtk_keypad_scan(struct mtk_keypad *keypad,
			unsigned int *row_state)
{
	unsigned int scancnt;

	for (scancnt = 0; scancnt < (keypad->key_num); scancnt++) {
		if ((keypad->key_adc) < (keypad->scanmap[scancnt].keythresh_vol)) {
			row_state[keypad->chanel] = keypad->scanmap[scancnt].keycode;
			break;
		}
	}

	if (scancnt == keypad->key_num)
		row_state[keypad->chanel] = RELEASE_KEY;
}

static struct task_struct *t_keypad_tsk;

static int t_keypad_thread(void *arg)
{
	struct mtk_keypad *keypad_t = arg;
	struct input_dev *input_dev_t = keypad_t->input_dev;
	unsigned int ctrl_t, adc_val_t, ch_offset_t;
	unsigned int row_state_t[SAR_CHANNEL_USED_MAX];

	memset(row_state_t, 0, sizeof(row_state_t));

	while (1) {
		wait_event_freezable(keypad_t->wait, _downtrig == 1);
		msleep(TASK_POLLING_TIMES);
		ctrl_t = mtk_readw_relaxed(keypad_t->base + REG_0000_SAR);
		ctrl_t |= SAR_CTRL_LOAD_EN;
		mtk_writew_relaxed(keypad_t->base + REG_0000_SAR, ctrl_t);
		ch_offset_t = (keypad_t->ch_detected) << 2;
		adc_val_t = mtk_readw_relaxed(keypad_t->base +
						REG_0100_SAR + ch_offset_t);
		keypad_t->key_adc = adc_val_t;
		mtk_keypad_scan(keypad_t, row_state_t);
		keystored_t = row_state_t[keypad_t->ch_detected];

		if (_downtrig) {
			if ((keystored_t != RELEASE_KEY) && (_uprelease)) {
				keypad_t->prekey[keypad_t->ch_detected] = keystored_t;
				keypad_t->pressed[keypad_t->ch_detected] = 1;
				_uprelease = 0;
				input_event(input_dev_t, EV_KEY, keystored_t, 1);
				input_sync(keypad_t->input_dev);
				_start_time = ktime_get();
			}
		}

		if (_uprelease == 0) {
			if ((keystored_t == keypad_t->prekey[keypad_t->ch_detected])
				&& (keypad_t->pressed[keypad_t->ch_detected])
				&& (keystored_t != RELEASE_KEY)) {

				if (keypad_t->no_autorepeat == 0) {
					if (ktime_ms_delta(ktime_get(), _start_time) >
						(keypad_t->debounce_time)) {
						input_event(input_dev_t, EV_KEY, keystored_t, 2);
						input_sync(keypad_t->input_dev);
						_start_time = ktime_get();
					}
				}
				keypad_t->repeat_count++;
			} else if ((adc_val_t > keypad_t->scanmap[(keypad_t->key_num)-1]
			.keythresh_vol) && (keypad_t->pressed[keypad_t->ch_detected])) {

				keypad_t->pressed[keypad_t->ch_detected] = 0;
				keypad_t->repeat_count = 0;
				input_event(input_dev_t, EV_KEY,
						keypad_t->prekey[keypad_t->ch_detected], 0);
				input_sync(keypad_t->input_dev);
				keypad_t->prekey[keypad_t->ch_detected] = 0;
				_uprelease = 1;
				_downtrig = 0;
			} else
				;
			/*Key Long Pressed reset*/
			if ((keypad_t->reset_timeout) && (_uprelease == 0)) {
				if (ktime_ms_delta(ktime_get(), _start_time) >
					(keypad_t->reset_timeout)) {
					if ((keypad_t->prekey[keypad_t->ch_detected]) ==
						(keypad_t->reset_key)) {
						_uprelease = 1;
						_downtrig = 0;
						keypad_t->repeat_count = 0;
						keypad_t->pressed[keypad_t->ch_detected] = 0;
						keypad_t->prekey[keypad_t->ch_detected] = 0;
					}
				}
			}
			if (keypad_t->no_autorepeat == 0) {
				if ((ktime_ms_delta(ktime_get(), _start_time) >
				(keypad_t->debounce_time)) && _uprelease == 0) {

					keypad_t->repeat_count = 0;
					keypad_t->pressed[keypad_t->ch_detected] = 0;
					input_event(input_dev_t, EV_KEY,
						keypad_t->prekey[keypad_t->ch_detected], 0);
					input_sync(keypad_t->input_dev);
					keypad_t->prekey[keypad_t->ch_detected] = 0;
					_uprelease = 1;
					_downtrig = 0;
				}
			}
		}
	}
	return 0;
}

static irqreturn_t mtk_keypad_irq_thread(int irq, void *dev_id)
{

	struct mtk_keypad *keypad = dev_id;
	unsigned int int_sts;
	unsigned int int_ststmp;
	unsigned int ctrl, adc_val, ch_offset;
	unsigned int chanelcnt;
	unsigned int row_state[SAR_CHANNEL_USED_MAX];
	unsigned int keystored;
	unsigned int keycount_timeout;

	chanelcnt = 0;
	ctrl = mtk_readw_relaxed(keypad->base + REG_0000_SAR);
	ctrl |= SAR_CTRL_LOAD_EN;

	int_sts = mtk_readw_relaxed(keypad->base + REG_0064_SAR);
	int_ststmp = int_sts;

	while (int_ststmp) {
		if ((int_sts & (1 << chanelcnt)) == 0) {
			chanelcnt++;
			int_ststmp >>= 1;
			continue;
		}

		int_ststmp >>= 1;
		ch_offset = chanelcnt << 2;
		mtk_writew_relaxed(keypad->base + REG_0000_SAR, ctrl);
		adc_val = mtk_readw_relaxed(keypad->base + REG_0100_SAR + ch_offset);
		mtk_writew_relaxed(keypad->base + REG_005C_SAR, int_sts);
		//Prevent AC Keypad debounce interrupt and sar0,sar1,sar2,sar3 for keypad.
		if (adc_val < RELEASE_ADC && chanelcnt < 4) {
			_downtrig = 1;
			wake_up(&keypad->wait);
		}
		keypad->ch_detected = chanelcnt;
		chanelcnt = 0;
	}
	return IRQ_HANDLED;
}

static void mtk_keypad_start(struct mtk_keypad *keypad)
{
	unsigned int val;

	/* Tell IRQ thread that it may poll the device. */
	keypad->stopped = false;
	/* Enable interrupt bits. */
	val = mtk_readw_relaxed(keypad->base + REG_0000_SAR);
	val &= ~SAR_CTRL_SINGLE_MODE;
	val |= SAR_CTRL_DIG_FREERUN_MODE;
	val |= SAR_CTRL_ATOP_FREERUN_MODE;
	val &= ~SAR_CTRL_8_CHANNEL;

	mtk_writew_relaxed(keypad->base + REG_0000_SAR, val);
	val = mtk_readw_relaxed(keypad->base + REG_0058_SAR);
	val &= ~(1<<(keypad->chanel));
	mtk_writew_relaxed(keypad->base + REG_0058_SAR, val);
	val = mtk_readw_relaxed(keypad->base + REG_0040_SAR);
	val |= (1<<(keypad->chanel));
	mtk_writew_relaxed(keypad->base + REG_0040_SAR, val);
	val = keypad->high_bound;
	mtk_writew_relaxed(keypad->base + REG_0080_SAR+(keypad->chanel << 2), val);
	val = keypad->low_bound;
	mtk_writew_relaxed(keypad->base + REG_00C0_SAR+(keypad->chanel << 2), val);

	val = mtk_readw_relaxed(keypad->base_p + REG_0010_PM_SLEEP);
	val &= ~SAR_WAKE_MASK;
	mtk_writew_relaxed(keypad->base_p + REG_0010_PM_SLEEP, val);
}

static void mtk_keypad_stop(struct mtk_keypad *keypad)
{

}

static int mtk_keypad_open(struct input_dev *input_dev)
{
	struct mtk_keypad *keypad = input_get_drvdata(input_dev);

	mtk_keypad_start(keypad);
	mtk_keypad_map(keypad);
	return 0;
}

static void mtk_keypad_close(struct input_dev *input_dev)
{
	struct mtk_keypad *keypad = input_get_drvdata(input_dev);

	mtk_keypad_stop(keypad);
}

#ifdef CONFIG_OF
static struct mtk_keypad_platdata *
mtk_keypad_parse_dt(struct device *dev)
{
	struct mtk_keypad_platdata *pdata;
	struct device_node *np = dev->of_node, *key_np;
	uint32_t *keythreshold;
	uint32_t *keyscancode;
	uint32_t keychanel;
	uint32_t keynum;
	uint32_t key_l;
	uint32_t key_h;
	uint32_t debouncetiming;
	uint32_t rst_timing;
	uint32_t  rst_key;

	rst_timing = 0;

	if (!np) {
		dev_err(dev, "missing device tree data\n");
		return ERR_PTR(-EINVAL);
	}

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	of_property_read_u32(np, "mtk,keypad-chanel", &keychanel);
	of_property_read_u32(np, "mtk,keypad-num", &keynum);
	of_property_read_u32(np, "mtk,keypad-lowbd", &key_l);
	of_property_read_u32(np, "mtk,keypad-upwbd", &key_h);
	of_property_read_u32(np, "mtk,keypad-debounce", &debouncetiming);
	of_property_read_u32(np, "mtk,rst-timeout", &rst_timing);
	of_property_read_u32(np, "mtk,rst-key", &rst_key);

	if (keychanel > SAR_CH_MAX)
		return ERR_PTR(-EINVAL);

	if ((!keynum) || (keynum > SAR_KEY_MAX))
		return ERR_PTR(-EINVAL);

	keythreshold = devm_kzalloc(dev, sizeof(uint32_t) * keynum, GFP_KERNEL);
	if (!keythreshold)
		return ERR_PTR(-ENOMEM);

	pdata->threshold = keythreshold;

	keyscancode = devm_kzalloc(dev, sizeof(uint32_t) * keynum, GFP_KERNEL);
	if (!keyscancode)
		return ERR_PTR(-ENOMEM);

	pdata->sarkeycode = keyscancode;
	pdata->chanel = keychanel;
	pdata->key_num = keynum;
	pdata->low_bound = key_l;
	pdata->high_bound = key_h;
	pdata->debounce_time = debouncetiming;
	pdata->reset_timeout = rst_timing;
	pdata->reset_key = rst_key;

	for_each_child_of_node(np, key_np) {
		u32 key_code, th_vol;

		of_property_read_u32(key_np, "linux,code", &key_code);
		of_property_read_u32(key_np, "keypad,threshold", &th_vol);
		*keythreshold++ = th_vol;
		*keyscancode++ = key_code;
	}

	pdata->wakeup = of_property_read_bool(np, "wakeup-source") ||
			/* legacy name */
			of_property_read_bool(np, "linux,input-wakeup");

	if (of_get_property(np, "linux,input-no-autorepeat", NULL))
		pdata->no_autorepeat = true;
	else
		pdata->no_autorepeat = 0;

	return pdata;
}
#else
static struct mtk_keypad_platdata *
mtk_keypad_parse_dt(struct device *dev)
{
	dev_err(dev, "no platform data defined\n");

	return ERR_PTR(-EINVAL);
}
#endif

static ssize_t keycode_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mtk_keypad *keypad = platform_get_drvdata(pdev);

	return scnprintf(buf, KEY_STR_LENSIZE, "%x\n", (keypad->keycodes)[0]);
}

static ssize_t keycode_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mtk_keypad *keypad = platform_get_drvdata(pdev);
	int keycnt = 0;
	int key_ch = 0;
	unsigned int key_code = 0;

	printk("size nums:%d\n",size);
	printk("channel:%d\n",buf[size-1]);
	key_ch = buf[size-1];
	for(keycnt = 0; keycnt < (size>>1); keycnt++) {
		key_code = buf[OFFSET_IDX*keycnt] | (buf[OFFSET_IDX*keycnt + 1] << BYTE_SHIFT);
		(keypad->keycodes)[keycnt] = key_code;
		printk("key_code:%x\n",key_code);
	}

	return size;
}
static DEVICE_ATTR_RW(keycode);

static ssize_t keybond_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mtk_keypad *keypad = platform_get_drvdata(pdev);

	return scnprintf(buf, KEY_STR_LENSIZE, "%x\n", (keypad->keythreshold)[0]);
}

static ssize_t keybond_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mtk_keypad *keypad = platform_get_drvdata(pdev);
	int keycnt = 0;
	int key_ch = 0;
	unsigned int key_bond = 0;

	printk("size nums:%d\n",size);
	printk("channel:%d\n",buf[size-1]);
	key_ch = buf[size-1];

	for(keycnt = 0; keycnt < (size>>1); keycnt++) {
		key_bond = buf[OFFSET_IDX*keycnt] | (buf[OFFSET_IDX*keycnt + 1] << BYTE_SHIFT);
		(keypad->keythreshold)[keycnt] = key_bond;
		printk("key_bond:%x\n",key_bond);
	}

	return size;
}
static DEVICE_ATTR_RW(keybond);

static struct attribute *keypad_extend_attrs[] = {
	&dev_attr_keycode.attr,
	&dev_attr_keybond.attr,
	NULL,
};

static const struct attribute_group keypad_extend_attr_group = {
	.name = "keypad_extend",
	.attrs = keypad_extend_attrs
};

static int mtk_keypad_probe(struct platform_device *pdev)
{
	const struct mtk_keypad_platdata *pdata;
	struct mtk_keypad *keypad;
	//struct resource *res;
	struct input_dev *input_dev;
	int error;
	int keycnt;
	struct device_node *np = NULL;

	np = pdev->dev.of_node;
	if (!np) {
		dev_err(&pdev->dev, "fail to find node\n");
		return -EINVAL;
	}

	_uprelease = 1;
	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		pdata = mtk_keypad_parse_dt(&pdev->dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	}
	keypad = devm_kzalloc(&pdev->dev, sizeof(*keypad), GFP_KERNEL);
	//Apply input_dev structure
	input_dev = devm_input_allocate_device(&pdev->dev);
	if (!keypad || !input_dev)
		return -ENOMEM;
	keypad->base = of_iomap(np, 0);
	keypad->base_p = of_iomap(np, 1);
	if (!keypad->base)
		return -EBUSY;

	keypad->input_dev = input_dev;
	keypad->pdev = pdev;
	keypad->stopped = true;
	keypad->keycodes = pdata->sarkeycode;
	keypad->keythreshold = pdata->threshold;
	keypad->chanel = pdata->chanel;
	keypad->low_bound = pdata->low_bound;
	keypad->high_bound = pdata->high_bound;
	keypad->key_num = pdata->key_num;
	keypad->no_autorepeat = pdata->no_autorepeat;
	keypad->debounce_time = pdata->debounce_time;
	keypad->reset_timeout = pdata->reset_timeout;
	keypad->reset_key = pdata->reset_key;

	if ((keypad->debounce_time < DEFAULT_DEBOUNCE) ||
		(keypad->debounce_time > DEFAULT_DEBOUNCE_MAX))
		keypad->debounce_time = DEFAULT_DEBOUNCE;

	init_waitqueue_head(&keypad->wait);
	if (pdev->dev.of_node)
		keypad->type = of_device_is_compatible(pdev->dev.of_node,
			"mediatek,mt5896-keypad");
	else
		keypad->type = platform_get_device_id(pdev)->driver_data;

	//input_dev->name = pdev->name;
	input_dev->name = MTK_KEYPAD_DEVICE_NAME;
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &pdev->dev;
	input_dev->id.vendor = INPUTID_VENDOR;
	input_dev->id.product = INPUTID_PRODUCT;
	input_dev->id.version = INPUTID_VERSION;
	input_dev->open = mtk_keypad_open;
	input_dev->close = mtk_keypad_close;

	for (keycnt = 0; keycnt < keypad->key_num; keycnt++)
		input_set_capability(input_dev, EV_KEY, (keypad->keycodes)[keycnt]);

	input_set_drvdata(input_dev, keypad);
	keypad->irq = platform_get_irq(pdev, 0);
	if (keypad->irq < 0) {
		error = keypad->irq;
		goto err_handle;
	}

	error = devm_request_irq(&pdev->dev, keypad->irq, mtk_keypad_irq_thread,
						IRQF_SHARED, dev_name(&pdev->dev), keypad);

	if (error) {
		dev_err(&pdev->dev, "failed to register keypad interrupt\n");
		goto err_handle;
	}

	platform_set_drvdata(pdev, keypad);
	/*Register Input node*/
	error = input_register_device(keypad->input_dev);
	if (error) {
		dev_err(&pdev->dev, "failed to input keypad device\n");
		goto err_handle;
	}

	/*patch for add_timer issue*/
	t_keypad_tsk = kthread_create(t_keypad_thread, keypad, "Keypad_Check");
	if (IS_ERR(t_keypad_tsk)) {
		error = PTR_ERR(t_keypad_tsk);
		t_keypad_tsk = NULL;
		goto err_handle;
	} else
		wake_up_process(t_keypad_tsk);
	/*patch for add_timer issue*/

	error = sysfs_create_group(&pdev->dev.kobj, &keypad_extend_attr_group);
	if (error < 0)
		dev_err(&pdev->dev, "wdt sysfs_create_group failed: %d\n", error);

	if (pdev->dev.of_node)
		devm_kfree(&pdev->dev, (void *)pdata);

	return 0;

err_handle:
	return error;
}

static int mtk_keypad_remove(struct platform_device *pdev)
{
	struct mtk_keypad *keypad = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	device_init_wakeup(&pdev->dev, 0);
	input_unregister_device(keypad->input_dev);
	return 0;
}


#ifdef CONFIG_PM_SLEEP
static void mtk_keypad_toggle_wakeup(struct mtk_keypad *keypad, bool enable)
{
}

static int mtk_keypad_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mtk_keypad *keypad = platform_get_drvdata(pdev);
	struct input_dev *input_dev = keypad->input_dev;

	mutex_lock(&input_dev->mutex);
	if (input_dev->users)
		mtk_keypad_stop(keypad);

	disable_irq(PM_SLEEP_INTERRUPT_NUM);
	mtk_keypad_toggle_wakeup(keypad, true);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static int mtk_keypad_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mtk_keypad *keypad = platform_get_drvdata(pdev);
	struct input_dev *input_dev = keypad->input_dev;

	mutex_lock(&input_dev->mutex);
	mtk_keypad_toggle_wakeup(keypad, false);
	mtk_keypad_start(keypad);
	mtk_keypad_map(keypad);
	enable_irq(PM_SLEEP_INTERRUPT_NUM);
	mutex_unlock(&input_dev->mutex);
	return 0;
}
#endif

static const struct dev_pm_ops mtk_keypad_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_keypad_suspend, mtk_keypad_resume)
};

#ifdef CONFIG_OF
static const struct of_device_id mtk_keypad_dt_match[] = {
	{ .compatible = "mediatek,mt5896-keypad" },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_keypad_dt_match);
#endif

static const struct platform_device_id mtk_keypad_driver_ids[] = {
	{
		.name       = "mtk-keypad",
		.driver_data    = KEYPAD_TYPE_MTK,
	},
	{ },
};
MODULE_DEVICE_TABLE(platform, mtk_keypad_driver_ids);

static struct platform_driver mtk_keypad_driver = {
	.probe      = mtk_keypad_probe,
	.remove     = mtk_keypad_remove,
	.driver     = {
		.name   = "mtk-keypad",
		.of_match_table = of_match_ptr(mtk_keypad_dt_match),
		.pm = &mtk_keypad_pm_ops,
	},
	.id_table   = mtk_keypad_driver_ids,
};
module_platform_driver(mtk_keypad_driver);

MODULE_DESCRIPTION("Mtk keypad driver");
MODULE_AUTHOR("Owen.Tseng");
MODULE_LICENSE("GPL");
