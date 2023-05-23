# SPDX-License-Identifier: GPL-2.0
# MTK Platform Support
obj-$(CONFIG_SND_SOC_MT5896) += mtk-mic-dai.o
mtk-mic-dai-objs += snd_soc_dai.o
mtk-mic-dai-objs += ./vbdma.o

obj-$(CONFIG_SND_SOC_MT5896) += mtk-mic-dma.o
mtk-mic-dma-objs += snd_soc_dma.o

# Machine support
obj-$(CONFIG_SND_SOC_MT5896) += mtk-mic-machine.o
mtk-mic-machine-objs += snd_soc_machine.o
#ifeq ($(CONFIG_SND_SOC_COMMUNICATION_RPMSG),y)

obj-$(CONFIG_SND_SOC_MT5896) += mtk-mic-communication.o
mtk-mic-communication-objs += voc_communication.o
#else
#mtk-mic-machine-objs += ./communication/drv-mbx.o
#endif