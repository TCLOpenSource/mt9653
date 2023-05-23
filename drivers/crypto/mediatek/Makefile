# SPDX-License-Identifier: GPL-2.0-only
obj-$(CONFIG_CRYPTO_DEV_MEDIATEK) += mtk-crypto.o
mtk-crypto-objs:= mtk-platform.o mtk-aes.o mtk-sha.o

obj-$(CONFIG_CRYPTO_DEV_MEDIATEK_DTV) += mtk-dtv-crypto.o
mtk-dtv-crypto-objs:= mtk-dtv-platform.o main.o mtk-cipher.o

ccflags-y += -I$(srctree)/drivers/iommu/mtk_iommu_dtv
