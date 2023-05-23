#ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/include/
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/2.5/inc/
#ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/debug/
ccflags-y += -I$(srctree)/drivers/devfreq/
ccflags-y += -I$(srctree)/drivers/soc/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y += -I$(srctree)/drivers/apusys/power/2.5/inc/
ccflags-y += -I$(srctree)/drivers/apusys/common/1.0/

SRC := common/apu-clk.o common/apu-common.o common/apu-regulator.o common/apu-rpc.o common/apu-of.o common/apu-plat.o  common/apu-driver.o common/apu-dbg.o common/apu-trace.o
SRC += governor/gov-apusys.o governor/gov-user.o governor/common.o governor/gov-constrain.o governor/gov-apusys-pe.o
SRC += devices/dev-freq-core.o devices/dev-freq-conn.o devices/dev-freq-vpu.o devices/dev-freq-mdla.o devices/dev-cb.o devices/dev-freq-iommu.o

SRC += platform/plat-regulator.o platform/plat-util.o platform/plat-device.o
SRC += platform/plat-mt589x-mtcmos.o
SRC += platform/plat-mt5896.o platform/plat-mt5896-clk.o
SRC += platform/plat-mt5897.o platform/plat-mt5897-clk.o
SRC += platform/plat-mt5879-mtcmos.o
SRC += platform/plat-mt5879.o platform/plat-mt5879-clk.o
SRC += platform/plat-mt5896_e3.o platform/plat-mt5896_e3-clk.o
SRC += platform/plat-teec-api.o

ccflags-y += -I$(srctree)/drivers/apusys/power/2.5/platform/

#obj-$(CONFIG_MTK_APUSYS_SUPPORT) += apu_top.o
obj-m += apu_power.o
apu_power-objs = $(SRC)
