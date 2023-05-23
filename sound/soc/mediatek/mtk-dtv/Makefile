# SPDX-License-Identifier: GPL-2.0
# MTK Platform Support

ccflags-y += -I$(srctree)/drivers/soc/mediatek/mtk-memory

obj-$(CONFIG_SND_SOC_MEDIATEK_DTV) += mtk-soc.o
mtk-soc-objs += mtk_alsa_soc.o

obj-$(CONFIG_SND_SOC_MEDIATEK_DTV) += snd-coprocessor.o
snd-coprocessor-objs += mtk_alsa_coprocessor_platform.o

obj-$(CONFIG_SND_SOC_MEDIATEK_DTV) += snd-pcm-platform.o
snd-pcm-platform-objs := 					\
			mtk_alsa_platform.o			\
			mtk_alsa_playback_platform.o		\
			mtk_alsa_dplayback_platform.o	\
			mtk_alsa_duplex_platform.o		\
			mtk_alsa_capture_platform.o

# Machine support
obj-$(CONFIG_SND_SOC_MEDIATEK_DTV) += snd-machine.o
snd-machine-objs += mtk_alsa_evb.o
