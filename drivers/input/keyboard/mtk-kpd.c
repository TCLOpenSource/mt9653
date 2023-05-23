// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 * Author Terry Chang <terry.chang@mediatek.com>
 */
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/regmap.h>

#define KPD_NAME	"mtk-kpd"

#define KP_STA			(0x0000)
#define KP_MEM1			(0x0004)
#define KP_MEM2			(0x0008)
#define KP_MEM3			(0x000c)
#define KP_MEM4			(0x0010)
#define KP_MEM5			(0x0014)
#define KP_DEBOUNCE		(0x0018)

#define KPD_DEBOUNCE_MASK	((1U << 14) - 1)
#define KPD_DOUBLE_KEY_MASK	(1U << 0)

#define KPD_NUM_MEMS	5
#define KPD_MEM5_BITS	8
#define KPD_NUM_KEYS	72	/* 4 * 16 + KPD_MEM5_BITS */

struct mtk_keypad {
	struct input_dev *input_dev;
	struct wakeup_source *suspend_lock;
	struct tasklet_struct tasklet;
	struct clk *clk;
	void __iomem *base;
	unsigned int irqnr;
	u32 key_debounce;
	u32 hw_map_num;
	u32 hw_init_map[KPD_NUM_KEYS];
	u16 keymap_state[KPD_NUM_MEMS];
};

static void kpd_get_keymap_state(void __iomem *kp_base, u16 state[])
{
	state[0] = readw(kp_base + KP_MEM1);
	state[1] = readw(kp_base + KP_MEM2);
	state[2] = readw(kp_base + KP_MEM3);
	state[3] = readw(kp_base + KP_MEM4);
	state[4] = readw(kp_base + KP_MEM5);
}

static void kpd_keymap_handler(unsigned long data)
{
	int i, j;
	int pressed;
	u16 new_state[KPD_NUM_MEMS], change, mask;
	u16 hw_keycode, keycode;
	void *dest;
	struct mtk_keypad *keypad = (struct mtk_keypad *)data;

	kpd_get_keymap_state(keypad->base, new_state);

	__pm_wakeup_event(keypad->suspend_lock, 500);

	for (i = 0; i < KPD_NUM_MEMS; i++) {
		change = new_state[i] ^ keypad->keymap_state[i];
		if (!change)
			continue;

		for (j = 0; j < 16U; j++) {
			mask = (u16) 1 << j;
			if (!(change & mask))
				continue;

			hw_keycode = (i << 4) + j;

			if (hw_keycode >= KPD_NUM_KEYS)
				continue;

			/* bit is 1: not pressed, 0: pressed */
			pressed = (new_state[i] & mask) == 0U;
			pr_debug("(%s) HW keycode = %d\n",
				(pressed) ? "pressed" : "released",
					hw_keycode);

			keycode = keypad->hw_init_map[hw_keycode];
			if (!keycode)
				continue;
			input_report_key(keypad->input_dev, keycode, pressed);
			input_sync(keypad->input_dev);
			pr_debug("report Linux keycode = %d\n", keycode);
		}
	}

	dest = memcpy(keypad->keymap_state, new_state, sizeof(new_state));
	enable_irq(keypad->irqnr);
}

static irqreturn_t kpd_irq_handler(int irq, void *dev_id)
{
	/* use _nosync to avoid deadlock */
	struct mtk_keypad *keypad = dev_id;

	disable_irq_nosync(keypad->irqnr);
	tasklet_schedule(&keypad->tasklet);
	return IRQ_HANDLED;
}

static int kpd_get_dts_info(struct mtk_keypad *keypad,
				struct device_node *node)
{
	int ret;

	ret = of_property_read_u32(node, "mediatek,key-debounce-ms",
		&keypad->key_debounce);
	if (ret) {
		pr_debug("read mediatek,key-debounce-ms error.\n");
		return ret;
	}

	ret = of_property_read_u32(node, "mediatek,hw-map-num",
		&keypad->hw_map_num);
	if (ret) {
		pr_debug("read mediatek,hw-map-num error.\n");
		return ret;
	}

	if (keypad->hw_map_num > KPD_NUM_KEYS) {
		pr_debug("hw-map-num error, it cannot bigger than %d.\n",
			KPD_NUM_KEYS);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(node, "mediatek,hw-init-map",
		keypad->hw_init_map, keypad->hw_map_num);

	if (ret) {
		pr_debug("hw-init-map was not defined in dts.\n");
		return ret;
	}

	pr_debug("deb= %d\n", keypad->key_debounce);

	return 0;
}

static int kpd_pdrv_probe(struct platform_device *pdev)
{
	struct mtk_keypad *keypad;
	struct resource *res;
	int i;
	int ret;

	keypad = devm_kzalloc(&pdev->dev, sizeof(*keypad), GFP_KERNEL);
	if (!keypad)
		return -ENOMEM;

	keypad->clk = devm_clk_get(&pdev->dev, "kpd");
	if (IS_ERR(keypad->clk))
		return PTR_ERR(keypad->clk);

	ret = clk_prepare_enable(keypad->clk);
	if (ret) {
		pr_debug("cannot prepare/enable keypad clock\n");
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENODEV;
		goto err_unprepare_clk;
	}

	keypad->base = devm_ioremap(&pdev->dev, res->start,
			resource_size(res));
	if (!keypad->base) {
		pr_debug("KP iomap failed\n");
		ret = -EBUSY;
		goto err_unprepare_clk;
	}

	keypad->irqnr = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!keypad->irqnr) {
		pr_debug("KP get irqnr failed\n");
		ret = -ENODEV;
		goto err_unprepare_clk;
	}

	ret = kpd_get_dts_info(keypad, pdev->dev.of_node);
	if (ret) {
		pr_debug("get dts info failed.\n");
		goto err_unprepare_clk;
	}

	memset(keypad->keymap_state, 0xff, sizeof(keypad->keymap_state));

	keypad->input_dev = devm_input_allocate_device(&pdev->dev);
	if (!keypad->input_dev) {
		pr_notice("input allocate device fail.\n");
		ret = -ENOMEM;
		goto err_unprepare_clk;
	}

	keypad->input_dev->name = KPD_NAME;
	keypad->input_dev->id.bustype = BUS_HOST;
	keypad->input_dev->dev.parent = &pdev->dev;

	__set_bit(EV_KEY, keypad->input_dev->evbit);

	for (i = 0; i < KPD_NUM_KEYS; i++) {
		if (keypad->hw_init_map[i])
			__set_bit(keypad->hw_init_map[i],
				keypad->input_dev->keybit);
	}

	ret = input_register_device(keypad->input_dev);
	if (ret) {
		pr_notice("register input device failed (%d)\n", ret);
		goto err_unprepare_clk;
	}

	input_set_drvdata(keypad->input_dev, keypad);

	keypad->suspend_lock = wakeup_source_register(NULL, "kpd wakelock");
	if (!keypad->suspend_lock) {
		pr_notice("wakeup source init failed.\n");
		goto err_unregister_device;
	}

	tasklet_init(&keypad->tasklet, kpd_keymap_handler,
					(unsigned long)keypad);

	writew((u16)(keypad->key_debounce & KPD_DEBOUNCE_MASK),
			keypad->base + KP_DEBOUNCE);

	/* register IRQ */
	ret = request_irq(keypad->irqnr, kpd_irq_handler, IRQF_TRIGGER_NONE,
			KPD_NAME, keypad);
	if (ret) {
		pr_notice("register IRQ failed (%d)\n", ret);
		goto err_irq;
	}

	ret = enable_irq_wake(keypad->irqnr);
	if (ret < 0)
		pr_notice("irq %d enable irq wake fail\n", keypad->irqnr);

	return 0;

err_irq:
	tasklet_kill(&keypad->tasklet);

err_unregister_device:
	input_unregister_device(keypad->input_dev);

err_unprepare_clk:
	clk_disable_unprepare(keypad->clk);

	return ret;
}


static int kpd_pdrv_remove(struct platform_device *pdev)
{
	struct mtk_keypad *keypad = platform_get_drvdata(pdev);

	tasklet_kill(&keypad->tasklet);
	wakeup_source_unregister(keypad->suspend_lock);
	input_unregister_device(keypad->input_dev);
	clk_disable_unprepare(keypad->clk);

	return 0;
}

static const struct of_device_id kpd_of_match[] = {
	{.compatible = "mediatek,mt6779-keypad"},
	{.compatible = "mediatek,kp"},
	{}
};

static struct platform_driver kpd_pdrv = {
	.probe = kpd_pdrv_probe,
	.remove = kpd_pdrv_remove,
	.driver = {
		   .name = KPD_NAME,
		   .of_match_table = kpd_of_match,
	},
};
module_platform_driver(kpd_pdrv);

MODULE_AUTHOR("Mediatek Corporation");
MODULE_DESCRIPTION("MTK Keypad (KPD) Driver");
MODULE_LICENSE("GPL");
