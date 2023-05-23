# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the Mediatek SoCs built-in ethernet macs
#

obj-$(CONFIG_NET_MEDIATEK_SOC)                 += mtk_eth.o
mtk_eth-y := mtk_eth_soc.o mtk_sgmii.o mtk_eth_path.o
obj-$(CONFIG_NET_MEDIATEK_MT5896)              += mtk_gmac_mt5896.o
mtk_gmac_mt5896-y := mtk_gmac_mt5896_main.o mtk_gmac_mt5896_tool.o
