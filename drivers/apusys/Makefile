PWD := $(shell pwd)
# using ?= so that user in other server can overwrite it by using environment variable
#KERNEL_DIR ?= /home/gem5/ai-simulator/8195/linux-gem5-headers

#export CROSS_COMPILE=/mfs/mcpslt0034/mtk18415/CAS_REAL/Machli_AN/merged/prebuilts/mtk_toolchain/gcc-aarch64-linux-gnu-5.5.0-ubuntu/x86_64/bin/aarch64-linux-gnu-
#KERNEL_OBJ_DIR = /mfs/mcpslt0034/mtk18145/m6_mainline_apollo-prod-2103_good_tag/out/target/product/moffit/KERNEL_OBJ
#KERNEL_SRC_DIR = /mfs/mcpslt0034/mtk18145/m6_mainline_apollo-prod-2103_good_tag/vendor/mediatek/tv/kernel/5.4

#obj-m += mdla_module.o
#obj-m += vpu0_module.o
#obj-m += gsm_module.o
#obj-m += dmabuf-tester.o
#obj-m += dmabuf-exporter.o
#obj-m += dmabuf-importer.o
obj-m += midware/
#obj-m += sample/
#obj-m += util/
#obj-m += reviser/
obj-m += mdla/
#obj-m += vpu/
obj-m += edma/
#obj-m += mnoc/
#obj-m += midware2/
obj-m += common/
#obj-m += unit_test/
obj-m += power/


#CROSS_COMPILE = aarch64-linux-gnu-
ARCH=arm64
#ccflags-y :=-fno-PIC
#INST_PATH = ../../../../../../../mtk_nn_esl_sdk_20200520/pattern/gem5_ia_boot/2nd_disk_put/
INST_PATH = ../../out/


MTK_PLATFORM = mt5896
AI_SIM = yes


