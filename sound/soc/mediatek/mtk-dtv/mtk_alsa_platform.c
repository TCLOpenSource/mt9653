// SPDX-License-Identifier: GPL-2.0
/*
 * mtk_alsa_platform.c  --  Mediatek DTV platform
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <sound/soc.h>
#include <sound/control.h>

#include "mtk_alsa_coprocessor_platform.h"
#include "mtk_alsa_playback_platform.h"
#include "mtk_alsa_dplayback_platform.h"
#include "mtk_alsa_pcm_platform.h"
#include "mtk_alsa_chip.h"

static const struct of_device_id capture_dt_match[] = {
	{ .compatible = "mediatek,mtk-capture-platform", },
	{ }
};

MODULE_DEVICE_TABLE(of, capture_dt_match);

static struct platform_driver capture_driver = {
	.driver = {
		.name = "dsp-cap-platform",
		.of_match_table = capture_dt_match,
	},
	.probe = capture_dev_probe,
	.remove = capture_dev_remove,
};

static const struct of_device_id duplex_dt_match[] = {
	{ .compatible = "mediatek,mtk-duplex-platform", },
	{ }
};
MODULE_DEVICE_TABLE(of, duplex_dt_match);

static struct platform_driver duplex_driver = {
	.driver = {
		.name = "snd-duplex-platform",
		.of_match_table = duplex_dt_match,
	},
	.probe = duplex_dev_probe,
	.remove = duplex_dev_remove,
};

static const struct of_device_id playback_dt_match[] = {
	{ .compatible = "mediatek,mtk-playback-platform", },
	{ }
};
MODULE_DEVICE_TABLE(of, playback_dt_match);

static struct platform_driver playback_driver = {
	.driver = {
		.name = "playback-dma-platform",
		.of_match_table = playback_dt_match,
	},
	.probe = playback_dev_probe,
	.remove = playback_dev_remove,
};

static const struct of_device_id dplayback_dt_match[] = {
	{ .compatible = "mediatek,mtk-dplayback-platform", },
	{ }
};
MODULE_DEVICE_TABLE(of, dplayback_dt_match);

static struct platform_driver dplayback_driver = {
	.driver = {
		.name = "dplayback-dma-platform",
		.of_match_table = dplayback_dt_match,
	},
	.probe = dplayback_dev_probe,
	.remove = dplayback_dev_remove,
};

static struct platform_driver * const drivers[] = {
	&playback_driver,
	&dplayback_driver,
	&duplex_driver,
	&capture_driver
};

static int __init alsa_platform_init(void)
{
	int ret;

	mtk_alsa_delaytask(1);
	ret = platform_register_drivers(drivers, ARRAY_SIZE(drivers));

	return ret;
}

static void __exit alsa_platform_exit(void)
{
	platform_unregister_drivers(drivers, ARRAY_SIZE(drivers));
}

module_init(alsa_platform_init);
module_exit(alsa_platform_exit);

MODULE_DESCRIPTION("ALSA SoC platform driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("AUDIO SoC plaform driver");
