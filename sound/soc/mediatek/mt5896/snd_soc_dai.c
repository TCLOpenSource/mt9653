// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

#include <linux/module.h>
#include <linux/device.h>
#include <linux/io.h>
#include <sound/soc.h>
#include "snd_soc_dma.h"
#include "vbdma.h"

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
static struct voc_pcm_dma_data voc_pcm_dma_wr = {
	.name = "DMA writer",
	.channel = 0,
};

//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
//#if 0
//static int voc_soc_dai_ops_startup(struct snd_pcm_substream *substream,
//					struct snd_soc_dai *dai)
//{
//	pr_debug("%s: dai = %s\n", __func__, dai->name);
//	return 0;
//}

//static void voc_soc_dai_ops_shutdown(struct snd_pcm_substream *substream,
//						struct snd_soc_dai *dai)
//{
//	pr_debug("%s: dai = %s\n", __func__, dai->name);
//}

//static int voc_soc_dai_ops_trigger(struct snd_pcm_substream *substream,
//					int cmd,
//					struct snd_soc_dai *dai)
//{
//	pr_debug("%s: dai = %s\n", __func__, dai->name);
//	return 0;
//}

//static int voc_soc_dai_ops_prepare(struct snd_pcm_substream *substream,
//					struct snd_soc_dai *dai)
//{
//	pr_debug("%s: dai = %s\n", __func__, dai->name);
//	return 0;
//}

//static int voc_soc_dai_ops_hw_params(struct snd_pcm_substream *substream,
//				struct snd_pcm_hw_params *params,
//				struct snd_soc_dai *dai)
//{
//	pr_debug("%s: dai = %s\n", __func__, dai->name);
//	return 0;
//}

//static int voc_soc_dai_ops_hw_free(struct snd_pcm_substream *substream,
//					struct snd_soc_dai *dai)
//{
//	pr_debug("%s: dai = %s\n", __func__, dai->name);
//	return 0;
//}

//static int voc_soc_dai_ops_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
//{
//	pr_debug("%s: dai = %s\n", __func__, dai->name);
//	return 0;
//}

//static int voc_soc_dai_ops_set_clkdiv(struct snd_soc_dai *dai, int div_id,
//						int div)
//{
//	pr_debug("%s: dai = %s\n", __func__, dai->name);
//	return 0;
//}

//static int voc_soc_dai_ops_set_sysclk(struct snd_soc_dai *dai, int clk_id,
//						unsigned int freq, int dir)
//{
//	pr_debug("%s: dai = %s\n", __func__, dai->name);
//	return 0;
//}

//static struct snd_soc_dai_ops voc_soc_cpu_dai_ops = {
//	.set_sysclk = voc_soc_dai_ops_set_sysclk,
//	.set_pll = NULL,
//	.set_clkdiv = voc_soc_dai_ops_set_clkdiv,

//	.set_fmt = voc_soc_dai_ops_set_fmt,

//	.startup = voc_soc_dai_ops_startup,
//	.shutdown = voc_soc_dai_ops_shutdown,
//	.trigger = voc_soc_dai_ops_trigger,
//	.prepare = voc_soc_dai_ops_prepare,
//	.hw_params = voc_soc_dai_ops_hw_params,
//	.hw_free = voc_soc_dai_ops_hw_free,
//};
//#endif

static int voc_soc_dai_probe(struct snd_soc_dai *dai)
{
	pr_debug("%s: dai = %s\n", __func__, dai->name);
	dai->capture_dma_data = (void *)&voc_pcm_dma_wr;
	return 0;
}

static int voc_soc_dai_remove(struct snd_soc_dai *dai)
{
	pr_debug("%s: dai = %s\n", __func__, dai->name);
	return 0;
}

static int voc_soc_dai_suspend(struct snd_soc_dai *dai)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	struct snd_soc_pcm_runtime *rtd;
	struct snd_soc_card *card = dai->component->card;

	pr_debug("%s: dai = %s\n", __func__, dai->name);

	list_for_each_entry(rtd, &card->rtd_list, list) {

		if (rtd->dai_link->ignore_suspend)
			continue;

		substream =
		    rtd->pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
		if (substream) {
			// FIXME: the open/close code
			// should lock this as well
			//
			if (substream->runtime == NULL)
				continue;

			runtime = substream->runtime;

			pr_debug("%s: VocDmaInitChannel\n", __func__);
			pr_debug("params_channels     = %d\n"
						, runtime->channels);
			pr_debug("params_rate         = %d\n", runtime->rate);
			pr_debug("params_sample_width = %d\n",
				snd_pcm_format_physical_width(runtime->format));
//#if 0
//			// reconfigure CM4 after updating image
//			VocDmaInitChannel((U32) (runtime->dma_addr - MIU_BASE),
//					runtime->dma_bytes,
//					runtime->channels,
//					snd_pcm_format_physical_width
//					(runtime->format), runtime->rate);
//#endif
		}
	}
	return 0;
}

extern bool voc_soc_is_seamless_enabled(void);

static int voc_soc_dai_resume(struct snd_soc_dai *dai)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	struct voc_pcm_runtime_data *prtd;
	//struct voc_pcm_dma_data *dma_data;
	struct snd_soc_pcm_runtime *rtd;
	//struct snd_soc_codec *codec;
	struct snd_soc_card *card = dai->component->card;

	pr_debug("%s: dai = %s\n", __func__, dai->name);
//    VocDmaReset();
	pr_info("Dma reset\n");

	list_for_each_entry(rtd, &card->rtd_list, list) {

		if (rtd->dai_link->ignore_suspend)
			continue;

		for (substream =
		     rtd->pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
		     substream; substream = substream->next) {
			if (substream->runtime == NULL) {
				//when seamless mode is enabled,
				//we have to reset CM4 audio
				// if recording isn't required.
				//
				//            if(voc_soc_is_seamless_enabled())
				//                  VocDmaResetAudio();
				continue;
			}

			runtime = substream->runtime;
			prtd = runtime->private_data;

			if (prtd && prtd->dma_data) {
				//memset(runtime->dma_area,
				//	0, runtime->dma_bytes);
				prtd->dma_level_count = 0;
				prtd->state = DMA_EMPTY;	//EMPTY
			}
//#if 0
//			if (VocIsCm4ShutDown()) {
//				pr_debug("%s: VocDmaInitChannel\n", __func__);
//				// reconfigure CM4 after updating image
//				VocDmaInitChannel((U32)(runtime->dma_addr
//					- MIU_BASE),
//					runtime->dma_bytes,
//					runtime->channels,
//					snd_pcm_format_physical_width
//					(runtime->format),
//					runtime->rate);
//			}
//#endif
		}
	}

	return 0;
}

struct snd_soc_dai_driver voc_soc_cpu_dai_drv = {
	.probe = voc_soc_dai_probe,
	.remove = voc_soc_dai_remove,
	.suspend = voc_soc_dai_suspend,
	.resume = voc_soc_dai_resume,

	.capture = {
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
			SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000,
			.formats =
			SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
			},
//  .ops                                = &voc_soc_cpu_dai_ops,
};

static const struct snd_soc_component_driver voc_soc_component = {
	.name = "voc-pcm",
};

static int voc_cpu_dai_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	u32 vbdma_base[2];
	u32 addr;
	int size;
	unsigned char *riu_map = NULL;

	pr_err("%s enter\r\n", __func__);

	ret =
		snd_soc_register_component(&pdev->dev, &voc_soc_component,
						&voc_soc_cpu_dai_drv, 1);

	of_property_read_u32_array(np, "reg", vbdma_base, 2);
	of_property_read_u32_index(np, "reg", 5, &addr);
	of_property_read_u32_index(np, "reg", 7, &size);
	riu_map = (unsigned char *)ioremap(addr, size);
	vbdma_get_riu_base(riu_map, vbdma_base[1]);

	if (ret)
		return ret;

	return 0;
}

static int voc_cpu_dai_remove(struct platform_device *pdev)
{
	pr_debug("%s enter\r\n", __func__);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static struct platform_driver voc_cpu_dai_driver = {
	.probe = voc_cpu_dai_probe,
	.remove = (voc_cpu_dai_remove),
	.driver = {
			.name = "voc-cpu-dai",
			.owner = THIS_MODULE,
			},
};

static struct platform_device *voc_cpu_dai_device;
static int __init voc_cpu_dai_init(void)
{

	int ret = 0;
	struct device_node *np;


	voc_cpu_dai_device = platform_device_alloc("voc-cpu-dai", -1);
	if (!voc_cpu_dai_device) {
		pr_err("%s: platform_device_alloc voc-cpu-dai error\r\n",
				__func__);
		return -ENOMEM;
	}
#if CONFIG_OF
	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-mic-dai");
	if (np) {
		voc_cpu_dai_device->dev.of_node = of_node_get(np);
		of_node_put(np);
	}
#endif
	ret = platform_device_add(voc_cpu_dai_device);
	if (ret) {
		pr_err("%s: platform_device_add voc_cpu_dai_device error\r\n",
				__func__);
		platform_device_put(voc_cpu_dai_device);
	}

	ret = platform_driver_register(&voc_cpu_dai_driver);
	if (ret) {
		pr_err("%s: platform_driver_register", __func__);
		pr_err("%s: voc_cpu_dai_driver error\r\n", __func__);
		platform_device_unregister(voc_cpu_dai_device);
	}

	return ret;
}

static void __exit voc_cpu_dai_exit(void)
{
	pr_debug("%s\r\n", __func__);
	platform_device_unregister(voc_cpu_dai_device);
	platform_driver_unregister(&voc_cpu_dai_driver);
}

module_init(voc_cpu_dai_init);
module_exit(voc_cpu_dai_exit);


/* Module information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allen-HW Hsu, allen-hw.hsu@mediatek.com");
MODULE_DESCRIPTION("Voc Audio ALSA SoC Dai");
