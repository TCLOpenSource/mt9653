# SPDX-License-Identifier: GPL-2.0
# MTK Platform Support
ccflags-y += -I$(srctree)/drivers/soc/mediatek/mtk-memory
ccflags-y += -I$(srctree)/drivers/clk/mediatek

obj-$(CONFIG_SND_MT5896) += mtk-mic-communication.o
mtk-mic-communication-objs += ./vbdma.o
mtk-mic-communication-objs += voc_drv_rpmsg.o
mtk-mic-communication-objs += voc_common.o

obj-$(CONFIG_SND_MT5896) += mtk-mic-capture.o
mtk-mic-capture-objs += voc_card.o
mtk-mic-capture-objs += voc_vd_task.o
mtk-mic-capture-objs += voc_communication.o
mtk-mic-capture-objs += voc_drv_paganini.o
mtk-mic-capture-objs += voc_hal_paganini.o
mtk-mic-capture-objs += voc_drv_mailbox.o
mtk-mic-capture-objs += voc_hal_mailbox.o
mtk-mic-capture-objs += voc_drv_stub.o
mtk-mic-capture-objs += voc_common.o

obj-$(CONFIG_SND_MT5896) += mtk-audio-mcu-communication.o
mtk-audio-mcu-communication-objs += audio_mcu_communication.o
mtk-audio-mcu-communication-objs += audio_mcu_rpmsg.o

obj-$(CONFIG_SND_MT5896) += mtk-audio-mcu.o
mtk-audio-mcu-objs += audio_mcu.o

obj-$(CONFIG_SND_MT5896) += mtk-aloop.o
mtk-aloop-objs += audio_loopback.o
