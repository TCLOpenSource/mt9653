obj-$(CONFIG_MTK_REALTIME_FRAMEWORK) += mtk_realtime_producer.o
obj-$(CONFIG_MTK_REALTIME_FRAMEWORK) += mtk_realtime_example.o

ifeq ($(ENABLE_SDDI),true)
ccflags-y += -DSDDI_ENABLE
else ifeq ($(LINUX_ONLY),true)
ccflags-y += -DLINUX_ONLY_ENABLE
ccflags-y += -DSET_USER_ID=3041
else ifeq ($(ANDROID_TWO_WORLDS),true)
ccflags-y += -DAOSP_ENABLE
ccflags-y += -DSET_USER_ID=2941
else ifneq ($(ANDROID),true)
ccflags-y += -DMIDEMO_ENABLE
else
ccflags-y += -DAOW_ENABLE
ccflags-y += -DSET_USER_ID=2941
endif

