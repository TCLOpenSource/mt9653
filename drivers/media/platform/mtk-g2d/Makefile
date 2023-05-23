MTK_G2D_PATH=$(srctree)/drivers/media/platform/mtk-g2d
ccflags-y := -DMSOS_TYPE_LINUX_KERNEL
ccflags-y += -DSTI_PLATFORM_BRING_UP
ccflags-y += -I$(MTK_G2D_PATH)/utp/

mtk-v4l2-g2d-objs := mtk-g2d.o mtk-g2d-utp-wrapper.o
mtk-v4l2-g2d-objs += utp/sti_msos.o
mtk-v4l2-g2d-objs += utp/apiGFX.o
mtk-v4l2-g2d-objs += utp/mapiGFX.o
mtk-v4l2-g2d-objs += utp/drvGE.o
mtk-v4l2-g2d-objs += utp/halGE.o
mtk-v4l2-g2d-objs += utp/riu.o

obj-$(CONFIG_MEDIATEK_G2D)+= mtk-v4l2-g2d.o
