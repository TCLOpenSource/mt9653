#
# Makefile for the kernel IrDA char device drivers.
#
ccflags-y += -Idrivers/drv/input/keyboard/ir_mirc
ccflags-y +=  -Iinclude/linux
obj-$(CONFIG_MTK_TV_IR_IN) += mtk-ir.o
mtk-ir-objs	 += mtk_ir.o \
					ir_core.o \
					protocols/ir-nec-decoder.o \
					protocols/ir-sony-decoder.o \
					protocols/ir-rca-decoder.o \
					protocols/ir-p7051-decoder.o \
					protocols/ir-rc5-decoder.o \
					protocols/ir-rc6-decoder.o \
					protocols/ir-toshiba-decoder.o \
					protocols/ir-metz-decoder.o \
					protocols/ir-rcmm-decoder.o \
					protocols/ir-konka-decoder.o \
					protocols/ir-panasonic-decoder.o \
					protocols/ir-rc311-decoder.o \
					protocols/ir-beko-rc5-decoder.o \
					protocols/ir-sharp-decoder.o

ifeq ($(CONFIG_MTK_TV_IR_KEYMAP_MTK_NEC), y)
mtk-ir-objs +=	keymaps/keymap-mtk-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_TCL_RCA), y)
mtk-ir-objs +=	keymaps/keymap-tcl-rca-tv.o
endif
ifeq ($(CONFIG_MTK_TV_IR_KEYMAP_SONY), y)
mtk-ir-objs +=	keymaps/keymap-sony-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_TCL), y)
mtk-ir-objs +=	keymaps/keymap-tcl-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_CHANGHONG), y)
mtk-ir-objs +=	keymaps/keymap-changhong-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_HISENSE), y)
mtk-ir-objs +=	keymaps/keymap-hisense-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_HAIER), y)
mtk-ir-objs +=	keymaps/keymap-haier-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_KONKA), y)
mtk-ir-objs +=	keymaps/keymap-konka-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_SKYWORTH), y)
mtk-ir-objs +=	keymaps/keymap-skyworth-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_PANASONIC_7051), y)
mtk-ir-objs +=	keymaps/keymap-p7051-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_KATHREIN), y)
mtk-ir-objs +=	keymaps/keymap-rc6-kathrein.o
endif
ifeq ($(CONFIG_IR_KEYMAP_RC5), y)
mtk-ir-objs +=	keymaps/keymap-rc5-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_METZ), y)
mtk-ir-objs += keymaps/keymap-metz-rm18-tv.o
mtk-ir-objs += keymaps/keymap-metz-rm19-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_SAMPO), y)
mtk-ir-objs += keymaps/keymap-rc311-sampo.o
endif
ifeq ($(CONFIG_IR_KEYMAP_GOME_NEC), y)
mtk-ir-objs += keymaps/keymap-gome-tv.o
endif
ifeq ($(CONFIG_IR_KEYMAP_BEKO_RC5), y)
mtk-ir-objs += keymaps/keymap-beko-rc5-tv.o
endif
obj-$(CONFIG_MTK_TV_IR_DYNAMIC_CONFIG) += ir_dynamic_config/
