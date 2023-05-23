/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_JPD_H__
#define __MTK_PLATFORM_JPD_H__

#include <linux/types.h>
#include <linux/interrupt.h>

typedef enum {
	E_PLATFORM_JPD_CMD_TYPE_DECODE = 0,
	E_PLATFORM_JPD_CMD_TYPE_FEED_READ = 1,
	E_PLATFORM_JPD_CMD_TYPE_FEED_WRITE = 2,
	E_PLATFORM_JPD_CMD_TYPE_DRAIN = 3,
	E_PLATFORM_JPD_CMD_TYPE_EOS = 4,
} platform_jpd_cmd_type;

typedef enum {
	E_PLATFORM_JPD_CMD_STATE_DECODE_DONE = 0,	// for write buffer
	E_PLATFORM_JPD_CMD_STATE_ERROR = 1,	// for read buffer
	E_PLATFORM_JPD_CMD_STATE_SKIP = 2,	// for read buffer
	E_PLATFORM_JPD_CMD_STATE_DROP = 3,	// for write buffer
	E_PLATFORM_JPD_CMD_STATE_EOS = 4,	// for write buffer
	E_PLATFORM_JPD_CMD_STATE_DRAIN_DONE = 5,	// for write buffer
} platform_jpd_cmd_state;

typedef enum {
	E_PLATFORM_JPD_FORMAT_JPEG = 0,
	E_PLATFORM_JPD_FORMAT_MJPEG = 1,
} platform_jpd_format;

typedef enum {
	E_PLATFORM_JPD_PORT_READ = 0,
	E_PLATFORM_JPD_PORT_WRITE = 1,
	E_PLATFORM_JPD_PORT_BOTH = 2,
} platform_jpd_port;

typedef enum {
	E_PLATFORM_JPD_PARSER_OK = 0,
	E_PLATFORM_JPD_PARSER_FAIL = 1,
	E_PLATFORM_JPD_PARSER_PARTIAL = 2,
} platform_jpd_parser_result;

typedef struct {
	struct device *dev;
	void __iomem *jpd_reg_base;
	void __iomem *jpd_ext_reg_base;
	struct clk *clk_njpd;
	struct clk *clk_smi2jpd;
	struct clk *clk_njpd2jpd;
	int jpd_irq;
} platform_jpd_initParam;

typedef struct {
	platform_jpd_cmd_type cmd_type;
	u64 timestamp;
	void *buf_priv;
	struct list_head list;
} platform_jpd_base_cmd;

typedef struct {
	void *va;
	dma_addr_t pa;
	size_t offset;
	size_t filled_length;
	size_t size;
} platform_jpd_buf;

typedef struct {
	u16 width;
	u16 height;
} platform_jpd_pic;

typedef struct {
	u16 x;
	u16 y;
	u16 width;
	u16 height;
} platform_jpd_rect;

typedef struct {
	u32 width;
	u32 height;
} platform_jpd_image_info;

typedef struct {
	u32 width;
	u32 height;
	platform_jpd_rect rect;
	u32 write_buf_size;
} platform_jpd_buf_info;


typedef int (*return_buf_func) (platform_jpd_base_cmd *base_cmd,
				platform_jpd_cmd_state state, void *user_priv);

int platform_jpd_init(platform_jpd_initParam *init_param);
int platform_jpd_deinit(void);
int platform_jpd_open(int *handle, void *user_priv, return_buf_func cb);
int platform_jpd_set_format(int handle, platform_jpd_format format);
int platform_jpd_set_rank(int handle, u32 rank);
int platform_jpd_set_roi(int handle, platform_jpd_rect rect);
int platform_jpd_set_down_scale(int handle, platform_jpd_pic pic);
int platform_jpd_set_max_resolution(int handle, platform_jpd_pic pic);
int platform_jpd_set_output_format(int handle, u8 format);
int platform_jpd_set_verification_mode(int handle, u8 mode);
platform_jpd_parser_result platform_jpd_prepare_decode(int handle,
						       platform_jpd_buf buf, void *buf_priv,
						       platform_jpd_base_cmd **cmd);
int platform_jpd_get_image_info(int handle, platform_jpd_image_info *image_info);
int platform_jpd_get_buf_info(int handle, platform_jpd_buf_info *buf_info);
int platform_jpd_start_port(int handle, platform_jpd_port port);
int platform_jpd_insert_cmd(int handle, platform_jpd_base_cmd *cmd);
int platform_jpd_insert_write_buf(int handle, platform_jpd_buf buf, void *buf_priv);
int platform_jpd_flush_port(int handle, platform_jpd_port port);
int platform_jpd_drain(int handle);
int platform_jpd_eos(int handle);
int platform_jpd_close(int handle);
irqreturn_t platform_jpd_irq(int irq, void *priv);
int platform_jpd_free_cmd(int handle, platform_jpd_base_cmd *cmd);

#endif				// __MTK_PLATFORM_JPD_H__
