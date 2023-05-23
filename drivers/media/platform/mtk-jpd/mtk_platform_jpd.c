// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include "mtk_platform_jpd.h"
#include "mtk_api_jpd.h"

//-------------------------------------------------------------------------------------------------
// Local Defines
//-------------------------------------------------------------------------------------------------
static int platform_jpd_debug;
module_param(platform_jpd_debug, int, 0644);
static unsigned long platform_jpd_task_count;
module_param(platform_jpd_task_count, ulong, 0644);

static DEFINE_SPINLOCK(platform_jpd_leak_lock);
static unsigned short platform_jpd_decode_cmd_leak;
static unsigned short platform_jpd_read_cmd_leak;
static unsigned short platform_jpd_write_cmd_leak;
static unsigned short platform_jpd_drain_cmd_leak;
static unsigned short platform_jpd_eos_cmd_leak;
static unsigned short platform_jpd_inter_buf_leak;
module_param(platform_jpd_decode_cmd_leak, ushort, 0644);
module_param(platform_jpd_read_cmd_leak, ushort, 0644);
module_param(platform_jpd_write_cmd_leak, ushort, 0644);
module_param(platform_jpd_drain_cmd_leak, ushort, 0644);
module_param(platform_jpd_eos_cmd_leak, ushort, 0644);
module_param(platform_jpd_inter_buf_leak, ushort, 0644);

#define LEVEL_ALWAYS    0
#define LEVEL_FLOW      0x1
#define LEVEL_DEBUG     0x2
#define LEVEL_PARSER    0x4
#define LEVEL_CMD       0x8
#define LEVEL_TASK      0x10
#define LEVEL_TASK_WQ   0x20
#define LEVEL_IRQ       0x40
#define LEVEL_BUFFER    0x80

#define jpd_debug(level, fmt, args...) \
	do { \
		if ((platform_jpd_debug & level) == level) \
			pr_info("platform_jpd: %s: " fmt "\n", __func__, ##args); \
	} while (0)

#define jpd_n_debug(level, fmt, args...) \
	do { \
		if ((platform_jpd_debug & level) == level) \
			pr_info("platform_jpd: [%d] %s: " fmt "\n", handle, __func__, ##args); \
	} while (0)

#define jpd_err(fmt, args...) \
	pr_err("platform_jpd: [ERR] %s: " fmt "\n", __func__, ##args)

#define jpd_n_err(fmt, args...) \
	pr_err("platform_jpd: [ERR][%d] %s: " fmt "\n", handle, __func__, ##args)

#define PRINT_DECODE_CMD(level, action, decode_cmd) \
	jpd_n_debug(level, "%s decode cmd: index %d, size 0x%zx, iova %pad, " \
		"offset 0x%zx, length 0x%zx", \
		action, decode_cmd->index, decode_cmd->api_cmd.read_buffer.size, \
		&decode_cmd->api_cmd.read_buffer.pa, decode_cmd->api_cmd.read_buffer.offset, \
		decode_cmd->api_cmd.read_buffer.filled_length)

#define PRINT_READ_CMD(level, action, read_cmd) \
	jpd_n_debug(level, "%s read cmd: index %d(%d), size 0x%zx, iova %pad, "	\
		"offset 0x%zx, length 0x%zx", \
		action, read_cmd->index, read_cmd->decode_order, read_cmd->api_read_buf.size, \
		&read_cmd->api_read_buf.pa, read_cmd->api_read_buf.offset, \
		read_cmd->api_read_buf.filled_length)

#define PRINT_WRITE_CMD(level, action, write_cmd) \
	jpd_n_debug(level, "%s write cmd: index %d, size 0x%zx, iova %pad", \
		action, write_cmd->index, write_cmd->api_write_buf.size, \
		&write_cmd->api_write_buf.pa)

#define WAKE_UP_TASK(wake_up) \
	do { \
		if (wake_up) { \
			spin_lock(&jpd_dev.task_wq_lock); \
			if (jpd_dev.task_wake_up_flag == false) { \
				jpd_dev.task_wake_up_flag = true; \
				spin_unlock(&jpd_dev.task_wq_lock); \
				jpd_n_debug(LEVEL_TASK_WQ, "wake up"); \
				wake_up_interruptible(&jpd_dev.task_wq); \
			} else { \
				spin_unlock(&jpd_dev.task_wq_lock); \
			} \
		} \
	} while (0)

#define TRY_RESTORE_WRITE_CMD(ginst, write_cmd) \
	do { \
		if (likely(ginst->write_port_enable)) { \
			PRINT_WRITE_CMD(LEVEL_TASK, "restore", write_cmd); \
			list_add(&write_cmd->base.list, &ginst->write_list); \
		} else { \
			PRINT_WRITE_CMD(LEVEL_CMD, "skip", write_cmd); \
			ginst->cb((platform_jpd_base_cmd *)write_cmd, \
				E_PLATFORM_JPD_CMD_STATE_SKIP, ginst->user_priv); \
			_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)write_cmd); \
		} \
	} while (0)

#define MAX_PLATFORM_JPD_INSTANCE_NUM 16

#define IOMMU_TAG_SHIFT 24
#define JPD_INTER               (12UL << IOMMU_TAG_SHIFT)
#define DMA_ATTR_WRITE_COMBINE  (1UL << 2)

//-------------------------------------------------------------------------------------------------
// Local Structures
//-------------------------------------------------------------------------------------------------
typedef enum {
	E_PLATFORM_JPD_STOPPED = 0,
	E_PLATFORM_JPD_DECODING = 1,
	E_PLATFORM_JPD_DECODE_DONE = 2,
	E_PLATFORM_JPD_DECODE_FAIL = 3,
} platform_jpd_decode_state;

typedef struct {
	platform_jpd_base_cmd base;
	NJPD_API_DecCmd api_cmd;
	u32 index;
} platform_jpd_decode_cmd;

typedef struct {
	platform_jpd_base_cmd base;
	NJPD_API_Buf api_read_buf;
	u32 index;
	u16 decode_order;
} platform_jpd_feed_read_cmd;

typedef struct {
	platform_jpd_base_cmd base;
	NJPD_API_Buf api_write_buf;
	u32 index;
} platform_jpd_feed_write_cmd;

typedef struct {
	platform_jpd_base_cmd base;
} platform_jpd_drain_cmd;

typedef struct {
	platform_jpd_base_cmd base;
} platform_jpd_eos_cmd;

typedef struct {
	bool active;
	bool find_SOS;
	u8 last_byte;
	int handle;
	int parser_handle;
	platform_jpd_format format;
	bool read_port_enable;
	bool write_port_enable;
	u32 read_index;
	u32 write_index;

	struct mutex list_lock;
	struct list_head read_list;
	struct list_head write_list;
	u32 rank;
	int priority;
	return_buf_func cb;
	void *user_priv;
} platform_jpd_instance;

typedef struct {
	struct mutex task_lock;
	int instance;
	platform_jpd_decode_state state;
	u16 feed_count;
	bool no_more_data;
	u64 timestamp;
	platform_jpd_decode_cmd *cmd;
	struct list_head data_list;
	platform_jpd_feed_write_cmd *write_buf;
} platform_jpd_dec;

typedef struct {
	struct device *dev;
	struct mutex inst_lock;
	struct task_struct *jpd_task;
	wait_queue_head_t task_wq;
	spinlock_t task_wq_lock;
	bool task_wake_up_flag;
	platform_jpd_instance inst[MAX_PLATFORM_JPD_INSTANCE_NUM];
	platform_jpd_dec jpd_dec;
} platform_jpd_dev;

//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------
static platform_jpd_dev jpd_dev;

//-------------------------------------------------------------------------------------------------
// Local Functions
//-------------------------------------------------------------------------------------------------
static inline int _check_handle(int handle)
{
	if (unlikely(handle < 0 || handle >= MAX_PLATFORM_JPD_INSTANCE_NUM)) {
		jpd_n_err("bad handle(%d)", handle);
		return -EINVAL;
	}
	if (unlikely(jpd_dev.inst[handle].active == false)) {
		jpd_n_err("handle(%d) not opened", handle);
		return -ENODEV;
	}
	return 0;
}

static NJPD_API_Result _platform_jpd_alloc_buf(size_t size, NJPD_API_Buf *api_buf, void *priv)
{
	platform_jpd_instance *ginst;
	int handle;
	// DMA_ATTR_WRITE_COMBINE for uncached buffer
	unsigned long buf_tag = (JPD_INTER | DMA_ATTR_WRITE_COMBINE);

	if (unlikely(jpd_dev.dev == NULL || api_buf == NULL || size == 0 || priv == NULL)) {
		jpd_err("bad params (%lx, %lx, 0x%zx, %lx)",
			(uintptr_t) jpd_dev.dev, (uintptr_t) api_buf, size, (uintptr_t) priv);
		return E_NJPD_API_FAILED;
	}
	ginst = (platform_jpd_instance *)priv;
	handle = ginst->handle;
	if (unlikely(_check_handle(handle) != 0)) {
		jpd_n_err(" check handle failed");
		return E_NJPD_API_FAILED;
	}

	api_buf->va = dma_alloc_attrs(jpd_dev.dev, size, &api_buf->pa, GFP_KERNEL, buf_tag);

	jpd_n_debug(LEVEL_BUFFER, "size 0x%zx, va %lx, iova %pad, tag 0x%lx",
		    size, (uintptr_t) api_buf->va, &api_buf->pa, buf_tag);

	if (unlikely(!api_buf->va)) {
		api_buf->size = 0;
		jpd_n_err("dma_alloc_attrs failed");
		return E_NJPD_API_FAILED;
	}

	api_buf->size = size;
	api_buf->offset = api_buf->filled_length = 0;

	spin_lock(&platform_jpd_leak_lock);
	++platform_jpd_inter_buf_leak;
	spin_unlock(&platform_jpd_leak_lock);

	return E_NJPD_API_OKAY;
}

static NJPD_API_Result _platform_jpd_free_buf(NJPD_API_Buf api_buf, void *priv)
{
	platform_jpd_instance *ginst = (platform_jpd_instance *)priv;
	int handle = -1;
	unsigned long buf_tag = JPD_INTER;

	if (unlikely(jpd_dev.dev == NULL || api_buf.va == NULL || api_buf.size == 0)) {
		jpd_err("bad params (%lx, %lx, 0x%zx)",
			(uintptr_t) jpd_dev.dev, (uintptr_t) api_buf.va, api_buf.size);
		return E_NJPD_API_FAILED;
	}
	if (likely(ginst))
		handle = ginst->handle;

	jpd_n_debug(LEVEL_BUFFER, "size 0x%zx, iova %pad, tag 0x%lx",
		    api_buf.size, &api_buf.pa, buf_tag);

	dma_free_attrs(jpd_dev.dev, api_buf.size, api_buf.va, api_buf.pa, buf_tag);

	spin_lock(&platform_jpd_leak_lock);
	--platform_jpd_inter_buf_leak;
	spin_unlock(&platform_jpd_leak_lock);

	return E_NJPD_API_OKAY;
}

static inline void *_platform_jpd_alloc_cmd(platform_jpd_cmd_type cmd_type, void *buf_priv)
{
	platform_jpd_base_cmd *cmd = NULL;
	unsigned short *cmd_leak = NULL;

	switch (cmd_type) {
	case E_PLATFORM_JPD_CMD_TYPE_DECODE:
		cmd = kmalloc(sizeof(platform_jpd_decode_cmd), GFP_KERNEL);
		cmd_leak = &platform_jpd_decode_cmd_leak;
		break;
	case E_PLATFORM_JPD_CMD_TYPE_FEED_READ:
		cmd = kmalloc(sizeof(platform_jpd_feed_read_cmd), GFP_KERNEL);
		cmd_leak = &platform_jpd_read_cmd_leak;
		break;
	case E_PLATFORM_JPD_CMD_TYPE_FEED_WRITE:
		cmd = kmalloc(sizeof(platform_jpd_feed_write_cmd), GFP_KERNEL);
		cmd_leak = &platform_jpd_write_cmd_leak;
		break;
	case E_PLATFORM_JPD_CMD_TYPE_DRAIN:
		cmd = kmalloc(sizeof(platform_jpd_drain_cmd), GFP_KERNEL);
		cmd_leak = &platform_jpd_drain_cmd_leak;
		break;
	case E_PLATFORM_JPD_CMD_TYPE_EOS:
		cmd = kmalloc(sizeof(platform_jpd_eos_cmd), GFP_KERNEL);
		cmd_leak = &platform_jpd_eos_cmd_leak;
		break;
	default:
		break;
	}

	if (unlikely(!cmd)) {
		jpd_err("alloc failed, cmd type(%d)", cmd_type);
		return NULL;
	}

	cmd->cmd_type = cmd_type;
	cmd->buf_priv = buf_priv;

	if (likely(cmd_leak)) {
		spin_lock(&platform_jpd_leak_lock);
		++(*cmd_leak);
		spin_unlock(&platform_jpd_leak_lock);
	}

	return cmd;
}

static inline void _platform_jpd_free_cmd(void *ginst, platform_jpd_base_cmd *cmd)
{
	if (likely(cmd)) {
		platform_jpd_decode_cmd *decode_cmd;
		unsigned short *cmd_leak = NULL;

		switch (cmd->cmd_type) {
		case E_PLATFORM_JPD_CMD_TYPE_DECODE:
			decode_cmd = (platform_jpd_decode_cmd *)cmd;
			_platform_jpd_free_buf(decode_cmd->api_cmd.internal_buffer, ginst);
			cmd_leak = &platform_jpd_decode_cmd_leak;
			break;
		case E_PLATFORM_JPD_CMD_TYPE_FEED_READ:
			cmd_leak = &platform_jpd_read_cmd_leak;
			break;
		case E_PLATFORM_JPD_CMD_TYPE_FEED_WRITE:
			cmd_leak = &platform_jpd_write_cmd_leak;
			break;
		case E_PLATFORM_JPD_CMD_TYPE_DRAIN:
			cmd_leak = &platform_jpd_drain_cmd_leak;
			break;
		case E_PLATFORM_JPD_CMD_TYPE_EOS:
			cmd_leak = &platform_jpd_eos_cmd_leak;
			break;
		default:
			break;
		}
		kfree(cmd);

		if (likely(cmd_leak)) {
			spin_lock(&platform_jpd_leak_lock);
			--(*cmd_leak);
			spin_unlock(&platform_jpd_leak_lock);
		}
	}
}

static int _platform_jpd_find_next_inst_by_priority(void)
{
	int i;
	int next_inst = -1;
	u32 highest_rank = U32_MAX;
	int highest_priority = -1;

	for (i = 0; i < MAX_PLATFORM_JPD_INSTANCE_NUM; i++) {
		if (jpd_dev.inst[i].active &&
		    !list_empty(&jpd_dev.inst[i].read_list) &&
		    !list_empty(&jpd_dev.inst[i].write_list) &&
		    jpd_dev.inst[i].rank != U32_MAX &&
		    jpd_dev.inst[i].rank <= highest_rank &&
		    (jpd_dev.inst[i].rank < highest_rank ||
		     jpd_dev.inst[i].priority > highest_priority)) {
			next_inst = i;
			highest_rank = jpd_dev.inst[i].rank;
			highest_priority = jpd_dev.inst[i].priority;
		}
	}

	if (next_inst >= 0) {
		for (i = 0; i < MAX_PLATFORM_JPD_INSTANCE_NUM; i++) {
			if (jpd_dev.inst[i].active)
				++jpd_dev.inst[i].priority;
		}
		jpd_dev.inst[next_inst].priority = 0;
	}
	return next_inst;
}

static bool _platform_jpd_stopped_process(void)
{
	int handle;
	bool find_next_inst = false;
	platform_jpd_instance *ginst;
	struct list_head *r_pos, *w_pos, *n;
	platform_jpd_base_cmd *base_cmd;
	platform_jpd_decode_cmd *decode_cmd;
	platform_jpd_feed_read_cmd *feed_read_cmd;
	platform_jpd_feed_write_cmd *feed_write_cmd;

	handle = _platform_jpd_find_next_inst_by_priority();
	if (handle == -1) {
		jpd_debug(LEVEL_TASK, "next inst not found");
		return find_next_inst;
	}
	find_next_inst = true;
	jpd_n_debug(LEVEL_TASK, "next inst(%d)", handle);

	ginst = &jpd_dev.inst[handle];

	mutex_lock(&ginst->list_lock);
	if (unlikely(list_empty(&ginst->read_list) || list_empty(&ginst->write_list))) {
		jpd_n_debug(LEVEL_TASK, "list empty (%d, %d)",
			    list_empty(&ginst->read_list), list_empty(&ginst->write_list));
		mutex_unlock(&ginst->list_lock);
		return find_next_inst;
	}
	list_for_each_safe(r_pos, n, &ginst->read_list) {
		base_cmd = list_entry(r_pos, platform_jpd_base_cmd, list);

		if (unlikely(base_cmd->cmd_type != E_PLATFORM_JPD_CMD_TYPE_DECODE &&
			     base_cmd->cmd_type != E_PLATFORM_JPD_CMD_TYPE_DRAIN &&
			     base_cmd->cmd_type != E_PLATFORM_JPD_CMD_TYPE_EOS)) {
			if (likely(base_cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_FEED_READ)) {
				feed_read_cmd = (platform_jpd_feed_read_cmd *)base_cmd;
				PRINT_READ_CMD(LEVEL_CMD, "skip", feed_read_cmd);
				ginst->cb(base_cmd, E_PLATFORM_JPD_CMD_STATE_SKIP,
					  ginst->user_priv);
			} else {
				jpd_n_debug(LEVEL_CMD, "skip cmd: type(%d)", base_cmd->cmd_type);
				ginst->cb(base_cmd, E_PLATFORM_JPD_CMD_STATE_ERROR,
					  ginst->user_priv);
			}
			list_del(r_pos);
			_platform_jpd_free_cmd(ginst, base_cmd);
			continue;
		}

		w_pos = ginst->write_list.next;
		feed_write_cmd = list_entry(w_pos, platform_jpd_feed_write_cmd, base.list);

		list_del(r_pos);
		list_del(w_pos);
		mutex_unlock(&ginst->list_lock);

		if (likely(base_cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_DECODE)) {
			decode_cmd = (platform_jpd_decode_cmd *)base_cmd;

			feed_read_cmd =
			    _platform_jpd_alloc_cmd(E_PLATFORM_JPD_CMD_TYPE_FEED_READ,
						    decode_cmd->base.buf_priv);
			if (unlikely(feed_read_cmd == NULL)) {
				jpd_n_err("read cmd alloc failed");

				PRINT_DECODE_CMD(LEVEL_CMD, "skip", decode_cmd);
				ginst->cb((platform_jpd_base_cmd *)decode_cmd,
					  E_PLATFORM_JPD_CMD_STATE_SKIP, ginst->user_priv);
				_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)decode_cmd);

				mutex_lock(&ginst->list_lock);
				TRY_RESTORE_WRITE_CMD(ginst, feed_write_cmd);
				mutex_unlock(&ginst->list_lock);
				return find_next_inst;
			}

			if (unlikely
			    (MApi_NJPD_Decoder_Start
			     (&decode_cmd->api_cmd,
			      feed_write_cmd->api_write_buf) != E_NJPD_API_OKAY)) {
				jpd_n_err("MApi_NJPD_Decoder_Start failed, index %d",
					  decode_cmd->index);

				PRINT_DECODE_CMD(LEVEL_CMD, "skip", decode_cmd);
				ginst->cb((platform_jpd_base_cmd *)decode_cmd,
					  E_PLATFORM_JPD_CMD_STATE_SKIP, ginst->user_priv);
				_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)decode_cmd);
				_platform_jpd_free_cmd(ginst,
						       (platform_jpd_base_cmd *)feed_read_cmd);

				mutex_lock(&ginst->list_lock);
				TRY_RESTORE_WRITE_CMD(ginst, feed_write_cmd);
				mutex_unlock(&ginst->list_lock);
				return find_next_inst;
			}

			jpd_dev.jpd_dec.cmd = decode_cmd;
			jpd_dev.jpd_dec.feed_count = 0;
			feed_read_cmd->api_read_buf = decode_cmd->api_cmd.read_buffer;
			feed_read_cmd->index = decode_cmd->index;
			feed_read_cmd->decode_order = jpd_dev.jpd_dec.feed_count++;
			list_add_tail(&feed_read_cmd->base.list, &jpd_dev.jpd_dec.data_list);
			jpd_dev.jpd_dec.write_buf = feed_write_cmd;
			jpd_dev.jpd_dec.timestamp = decode_cmd->base.timestamp;
			jpd_dev.jpd_dec.no_more_data = false;
			jpd_dev.jpd_dec.instance = handle;
			jpd_dev.jpd_dec.state = E_PLATFORM_JPD_DECODING;

			jpd_n_debug(LEVEL_TASK, "decode frame pts %lld start",
						jpd_dev.jpd_dec.timestamp);
			PRINT_DECODE_CMD(LEVEL_TASK, "start", decode_cmd);
			PRINT_WRITE_CMD(LEVEL_TASK, "feed", feed_write_cmd);

			return find_next_inst;
		}

		if (base_cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_EOS) {
			jpd_n_debug(LEVEL_CMD, "return eos cmd");
			ginst->cb((platform_jpd_base_cmd *)feed_write_cmd,
				  E_PLATFORM_JPD_CMD_STATE_EOS, ginst->user_priv);
		} else {
			jpd_n_debug(LEVEL_CMD, "return drain cmd");
			ginst->cb((platform_jpd_base_cmd *)feed_write_cmd,
				  E_PLATFORM_JPD_CMD_STATE_DRAIN_DONE, ginst->user_priv);
		}
		_platform_jpd_free_cmd(ginst, base_cmd);
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)feed_write_cmd);
		return find_next_inst;
	}

	jpd_n_debug(LEVEL_TASK, "no decode cmd");
	mutex_unlock(&ginst->list_lock);

	return find_next_inst;
}

static bool _platform_jpd_decoding_process(void)
{
	int handle;
	bool find_read_cmd = false;
	platform_jpd_instance *ginst;
	struct list_head *r_pos, *n;
	platform_jpd_base_cmd *base_cmd;
	platform_jpd_feed_read_cmd *feed_read_cmd;
	NJPD_API_DecodeStatus decode_status;
	u16 consumed_data_num;

	handle = jpd_dev.jpd_dec.instance;
	ginst = &jpd_dev.inst[handle];

	decode_status = MApi_NJPD_Decoder_WaitDone();

	if (decode_status == E_NJPD_API_DEC_DONE) {
		jpd_dev.jpd_dec.state = E_PLATFORM_JPD_DECODE_DONE;
		return true;
	} else if (decode_status == E_NJPD_API_DEC_FAILED) {
		jpd_dev.jpd_dec.state = E_PLATFORM_JPD_DECODE_FAIL;
		return true;
	}

	consumed_data_num = MApi_NJPD_Decoder_ConsumedDataNum();

	list_for_each_safe(r_pos, n, &jpd_dev.jpd_dec.data_list) {
		if (r_pos->next == &jpd_dev.jpd_dec.data_list) {
			// keep last feed read cmd
			break;
		}
		feed_read_cmd = list_entry(r_pos, platform_jpd_feed_read_cmd, base.list);
		if (consumed_data_num <= feed_read_cmd->decode_order) {
			// no feed cmd consumed
			break;
		}
		list_del(r_pos);
		PRINT_READ_CMD(LEVEL_CMD, "skip", feed_read_cmd);
		ginst->cb((platform_jpd_base_cmd *)feed_read_cmd, E_PLATFORM_JPD_CMD_STATE_SKIP,
			  ginst->user_priv);
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)feed_read_cmd);
	}

	if (!jpd_dev.jpd_dec.no_more_data) {
		mutex_lock(&ginst->list_lock);
		if (list_empty(&ginst->read_list)) {
			jpd_n_debug(LEVEL_TASK, "read list empty");
			mutex_unlock(&ginst->list_lock);
			return find_read_cmd;
		}
		find_read_cmd = true;

		list_for_each_safe(r_pos, n, &ginst->read_list) {
			base_cmd = list_entry(r_pos, platform_jpd_base_cmd, list);

			if (likely(base_cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_FEED_READ)) {
				feed_read_cmd = (platform_jpd_feed_read_cmd *)base_cmd;

				if (unlikely
				    (MApi_NJPD_Decoder_FeedData(feed_read_cmd->api_read_buf) !=
				     E_NJPD_API_OKAY)) {
					jpd_n_err("MApi_NJPD_Decoder_FeedData failed, index %d",
						  feed_read_cmd->index);
					list_del(r_pos);
					PRINT_READ_CMD(LEVEL_CMD, "skip", feed_read_cmd);
					ginst->cb(base_cmd, E_PLATFORM_JPD_CMD_STATE_SKIP,
						  ginst->user_priv);
					_platform_jpd_free_cmd(ginst,
						(platform_jpd_base_cmd *)feed_read_cmd);

					if (unlikely
					    (MApi_NJPD_Decoder_NoMoreData() != E_NJPD_API_OKAY)) {
						jpd_n_err("MApi_NJPD_Decoder_NoMoreData failed");
					}
					jpd_dev.jpd_dec.no_more_data = true;
					break;
				}
				feed_read_cmd->decode_order = jpd_dev.jpd_dec.feed_count++;
				list_del(r_pos);
				list_add_tail(&feed_read_cmd->base.list,
						&jpd_dev.jpd_dec.data_list);

				PRINT_READ_CMD(LEVEL_TASK, "feed", feed_read_cmd);
			} else {
				if (unlikely(MApi_NJPD_Decoder_NoMoreData() != E_NJPD_API_OKAY))
					jpd_n_err("MApi_NJPD_Decoder_NoMoreData failed");

				jpd_dev.jpd_dec.no_more_data = true;
				break;
			}
		}
		mutex_unlock(&ginst->list_lock);
	}
	return find_read_cmd;
}

static bool _platform_jpd_decode_done_process(void)
{
	int handle;
	platform_jpd_instance *ginst;
	struct list_head *r_pos, *n;
	platform_jpd_feed_read_cmd *feed_read_cmd;

	handle = jpd_dev.jpd_dec.instance;
	ginst = &jpd_dev.inst[handle];

	jpd_n_debug(LEVEL_TASK, "decode frame pts %lld done", jpd_dev.jpd_dec.timestamp);

	if (likely(jpd_dev.jpd_dec.cmd)) {
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)jpd_dev.jpd_dec.cmd);
		jpd_dev.jpd_dec.cmd = NULL;
	} else {
		jpd_n_err("jpd_dec.cmd is NULL");
	}

	if (unlikely(list_empty(&jpd_dev.jpd_dec.data_list)))
		jpd_n_err("jpd_dec.data_list is empty");

	list_for_each_safe(r_pos, n, &jpd_dev.jpd_dec.data_list) {
		feed_read_cmd = list_entry(r_pos, platform_jpd_feed_read_cmd, base.list);

		if (r_pos->next == &jpd_dev.jpd_dec.data_list) {
			PRINT_READ_CMD(LEVEL_CMD, "decode done", feed_read_cmd);
			ginst->cb((platform_jpd_base_cmd *)feed_read_cmd,
				  E_PLATFORM_JPD_CMD_STATE_DECODE_DONE, ginst->user_priv);
		} else {
			PRINT_READ_CMD(LEVEL_CMD, "skip", feed_read_cmd);
			ginst->cb((platform_jpd_base_cmd *)feed_read_cmd,
				  E_PLATFORM_JPD_CMD_STATE_SKIP, ginst->user_priv);
		}
		list_del(r_pos);
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)feed_read_cmd);
	}

	if (likely(jpd_dev.jpd_dec.write_buf)) {
		jpd_dev.jpd_dec.write_buf->base.timestamp = jpd_dev.jpd_dec.timestamp;
		PRINT_WRITE_CMD((LEVEL_CMD | LEVEL_TASK), "decode done", jpd_dev.jpd_dec.write_buf);
		ginst->cb((platform_jpd_base_cmd *)jpd_dev.jpd_dec.write_buf,
			  E_PLATFORM_JPD_CMD_STATE_DECODE_DONE, ginst->user_priv);
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)jpd_dev.jpd_dec.write_buf);
		jpd_dev.jpd_dec.write_buf = NULL;
	} else {
		jpd_n_err("jpd_dec.write_buf is NULL");
	}

	if (unlikely(MApi_NJPD_Decoder_Stop() != E_NJPD_API_OKAY))
		jpd_n_err("MApi_NJPD_Decoder_Stop failed");

	jpd_dev.jpd_dec.instance = -1;
	jpd_dev.jpd_dec.state = E_PLATFORM_JPD_STOPPED;

	return true;
}

static bool _platform_jpd_decode_fail_process(void)
{
	int handle;
	platform_jpd_instance *ginst;
	struct list_head *r_pos, *n;
	platform_jpd_feed_read_cmd *feed_read_cmd;

	handle = jpd_dev.jpd_dec.instance;
	ginst = &jpd_dev.inst[handle];

	jpd_n_debug(LEVEL_TASK, "decode frame pts %lld fail", jpd_dev.jpd_dec.timestamp);

	if (likely(jpd_dev.jpd_dec.cmd)) {
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)jpd_dev.jpd_dec.cmd);
		jpd_dev.jpd_dec.cmd = NULL;
	} else {
		jpd_n_err("jpd_dec.cmd is NULL");
	}

	if (unlikely(list_empty(&jpd_dev.jpd_dec.data_list)))
		jpd_n_err("jpd_dec.data_list is empty");

	list_for_each_safe(r_pos, n, &jpd_dev.jpd_dec.data_list) {
		feed_read_cmd = list_entry(r_pos, platform_jpd_feed_read_cmd, base.list);

		if (r_pos->next == &jpd_dev.jpd_dec.data_list) {
			PRINT_READ_CMD((LEVEL_CMD | LEVEL_TASK), "decode error", feed_read_cmd);
			ginst->cb((platform_jpd_base_cmd *)feed_read_cmd,
				  E_PLATFORM_JPD_CMD_STATE_ERROR, ginst->user_priv);
		} else {
			PRINT_READ_CMD(LEVEL_CMD, "skip", feed_read_cmd);
			ginst->cb((platform_jpd_base_cmd *)feed_read_cmd,
				  E_PLATFORM_JPD_CMD_STATE_SKIP, ginst->user_priv);
		}
		list_del(r_pos);
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)feed_read_cmd);
	}

	if (likely(jpd_dev.jpd_dec.write_buf)) {
		mutex_lock(&ginst->list_lock);
		TRY_RESTORE_WRITE_CMD(ginst, jpd_dev.jpd_dec.write_buf);
		mutex_unlock(&ginst->list_lock);
		jpd_dev.jpd_dec.write_buf = NULL;
	} else {
		jpd_n_err("jpd_dec.write_buf is NULL");
	}

	if (unlikely(MApi_NJPD_Decoder_Stop() != E_NJPD_API_OKAY))
		jpd_n_err("MApi_NJPD_Decoder_Stop failed");

	jpd_dev.jpd_dec.instance = -1;
	jpd_dev.jpd_dec.state = E_PLATFORM_JPD_STOPPED;

	return true;
}

static int _platform_jpd_task(void *arg)
{
	int ret;
	bool keep_going = false;

	jpd_debug(LEVEL_ALWAYS, "run");

	for (;;) {
		jpd_debug(LEVEL_TASK_WQ, "waiting...");
		++platform_jpd_task_count;
		ret = wait_event_interruptible(jpd_dev.task_wq,
					       keep_going || jpd_dev.task_wake_up_flag
					       || kthread_should_stop());
		++platform_jpd_task_count;
		jpd_debug(LEVEL_TASK_WQ, "wait done");

		if (unlikely(ret != 0 || kthread_should_stop())) {
			jpd_err("break, (%d, %d)", ret, kthread_should_stop());
			break;
		}

		spin_lock(&jpd_dev.task_wq_lock);
		jpd_dev.task_wake_up_flag = false;
		spin_unlock(&jpd_dev.task_wq_lock);

		mutex_lock(&jpd_dev.jpd_dec.task_lock);
		switch (jpd_dev.jpd_dec.state) {
		case E_PLATFORM_JPD_STOPPED:
			keep_going = _platform_jpd_stopped_process();
			break;
		case E_PLATFORM_JPD_DECODING:
			keep_going = _platform_jpd_decoding_process();
			break;
		case E_PLATFORM_JPD_DECODE_DONE:
			keep_going = _platform_jpd_decode_done_process();
			break;
		case E_PLATFORM_JPD_DECODE_FAIL:
			keep_going = _platform_jpd_decode_fail_process();
			break;
		default:
			keep_going = false;
			jpd_err("bad state(%d)", jpd_dev.jpd_dec.state);
			break;
		}
		mutex_unlock(&jpd_dev.jpd_dec.task_lock);
	}

	jpd_debug(LEVEL_ALWAYS, "stop");
	return 0;
}

//-------------------------------------------------------------------------------------------------
// Interface Functions
//-------------------------------------------------------------------------------------------------
int platform_jpd_init(platform_jpd_initParam *init_param)
{
	int ret;
	int i;
	NJPD_API_InitParam api_param;

	if (unlikely(init_param->dev == NULL)) {
		jpd_err("dev is NULL");
		return -EINVAL;
	}

	api_param.jpd_reg_base = init_param->jpd_reg_base;
	api_param.jpd_ext_reg_base = init_param->jpd_ext_reg_base;
	api_param.clk_njpd = init_param->clk_njpd;
	api_param.clk_smi2jpd = init_param->clk_smi2jpd;
	api_param.clk_njpd2jpd = init_param->clk_njpd2jpd;
	api_param.jpd_irq = init_param->jpd_irq;

	if (unlikely(MApi_NJPD_Init(&api_param) != E_NJPD_API_OKAY)) {
		jpd_err("MApi_NJPD_Init failed");
		return -EFAULT;
	}

	init_waitqueue_head(&jpd_dev.task_wq);
	spin_lock_init(&jpd_dev.task_wq_lock);
	mutex_init(&jpd_dev.inst_lock);
	mutex_init(&jpd_dev.jpd_dec.task_lock);
	for (i = 0; i < MAX_PLATFORM_JPD_INSTANCE_NUM; i++)
		mutex_init(&jpd_dev.inst[i].list_lock);

	INIT_LIST_HEAD(&jpd_dev.jpd_dec.data_list);

	jpd_dev.jpd_task = kthread_create(_platform_jpd_task, NULL, "mtk_jpd_task");
	if (IS_ERR(jpd_dev.jpd_task)) {
		jpd_err("create jpd task failed(%ld)", PTR_ERR(jpd_dev.jpd_task));
		jpd_dev.jpd_task = NULL;
		ret = -ENOMEM;
		goto err_jpd_task;
	}

	jpd_dev.dev = init_param->dev;
	jpd_dev.jpd_dec.instance = -1;

	wake_up_process(jpd_dev.jpd_task);

	jpd_debug(LEVEL_ALWAYS, "success");

	return 0;

err_jpd_task:
	MApi_NJPD_Deinit();

	return ret;
}

int platform_jpd_deinit(void)
{
	if (likely(jpd_dev.jpd_task)) {
		kthread_stop(jpd_dev.jpd_task);
		jpd_dev.jpd_task = NULL;
	}

	MApi_NJPD_Deinit();

	jpd_debug(LEVEL_ALWAYS, "success");

	return 0;
}

int platform_jpd_open(int *handle, void *user_priv, return_buf_func cb)
{
	platform_jpd_instance *ginst;
	int i;
	int parser_handle = -1;
	NJPD_API_ParserInitParam api_param;

	if (unlikely(handle == NULL || cb == NULL)) {
		jpd_err("bad params (%lx, %lx)", (uintptr_t) handle, (uintptr_t) cb);
		return -EINVAL;
	}

	mutex_lock(&jpd_dev.inst_lock);

	for (i = 0; i < MAX_PLATFORM_JPD_INSTANCE_NUM; ++i) {
		if (jpd_dev.inst[i].active == false)
			break;
	}
	if (unlikely(i == MAX_PLATFORM_JPD_INSTANCE_NUM)) {
		jpd_err("no free platform jpd instance");
		mutex_unlock(&jpd_dev.inst_lock);
		return -ENOMEM;
	}

	ginst = &jpd_dev.inst[i];

	api_param.pAllocBufFunc = _platform_jpd_alloc_buf;
	api_param.pFreeBufFunc = _platform_jpd_free_buf;
	api_param.priv = ginst;
	if (unlikely(MApi_NJPD_Parser_Open(&parser_handle, &api_param) != E_NJPD_API_OKAY)) {
		jpd_err("[%d] MApi_NJPD_Parser_Open failed", i);
		mutex_unlock(&jpd_dev.inst_lock);
		return -ENOMEM;
	}

	ginst->handle = i;
	ginst->parser_handle = parser_handle;
	ginst->rank = 0;
	ginst->priority = 0;
	ginst->cb = cb;
	ginst->user_priv = user_priv;
	INIT_LIST_HEAD(&ginst->read_list);
	INIT_LIST_HEAD(&ginst->write_list);
	ginst->format = E_PLATFORM_JPD_FORMAT_JPEG;
	ginst->find_SOS = false;
	ginst->last_byte = 0;
	ginst->read_port_enable = false;
	ginst->write_port_enable = false;
	ginst->read_index = 0;
	ginst->write_index = 0;
	*handle = i;

	ginst->active = true;

	mutex_unlock(&jpd_dev.inst_lock);

	jpd_debug(LEVEL_FLOW, "[%d] parser handle(%d) success", *handle, parser_handle);

	return 0;
}

int platform_jpd_set_format(int handle, platform_jpd_format format)
{
	platform_jpd_instance *ginst;
	NJPD_API_Format api_format;
	int ret;
	bool iommu = true;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	switch (format) {
	case E_PLATFORM_JPD_FORMAT_MJPEG:
		api_format = E_NJPD_API_FORMAT_MJPEG;
		break;
	case E_PLATFORM_JPD_FORMAT_JPEG:
	default:
		api_format = E_NJPD_API_FORMAT_JPEG;
		break;
	}

	if (unlikely
	    (MApi_NJPD_Parser_SetFormat(ginst->parser_handle, api_format) != E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_SetFormat(%d) failed", api_format);
		return -EFAULT;
	}

	if (unlikely(MApi_NJPD_Parser_SetIommu(ginst->parser_handle, iommu) != E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_SetIommu(%d) failed", iommu);
		return -EFAULT;
	}

	ginst->format = format;

	jpd_n_debug(LEVEL_ALWAYS, "format(%d) iommu(%d) success", api_format, iommu);

	return 0;
}

int platform_jpd_set_rank(int handle, u32 rank)
{
	platform_jpd_instance *ginst;
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	ginst->rank = rank;

	jpd_n_debug(LEVEL_ALWAYS, "rank(%d) success", rank);

	return 0;
}

int platform_jpd_set_roi(int handle, platform_jpd_rect rect)
{
	platform_jpd_instance *ginst;
	NJPD_API_Rect api_rect = { rect.x, rect.y, rect.width, rect.height };
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely(MApi_NJPD_Parser_SetROI(ginst->parser_handle, api_rect) != E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_SetROI(%d, %d, %d, %d) failed",
			  api_rect.x, api_rect.y, api_rect.width, api_rect.height);
		return -EFAULT;
	}

	jpd_n_debug(LEVEL_ALWAYS, "roi rect[%d, %d, %d, %d] success",
		    api_rect.x, api_rect.y, api_rect.width, api_rect.height);

	return 0;
}

int platform_jpd_set_down_scale(int handle, platform_jpd_pic pic)
{
	platform_jpd_instance *ginst;
	NJPD_API_Pic api_pic = { pic.width, pic.height };
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely
	    (MApi_NJPD_Parser_SetDownScale(ginst->parser_handle, api_pic) != E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_SetDownScale(%d, %d) failed", api_pic.width,
			  api_pic.height);
		return -EFAULT;
	}

	jpd_n_debug(LEVEL_ALWAYS, "down scale pic[%d, %d] success", api_pic.width, api_pic.height);

	return 0;
}

int platform_jpd_set_max_resolution(int handle, platform_jpd_pic pic)
{
	platform_jpd_instance *ginst;
	NJPD_API_Pic api_pic = { pic.width, pic.height };
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely
	    (MApi_NJPD_Parser_SetMaxResolution(ginst->parser_handle, api_pic) != E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_SetMaxResolution(%d, %d) failed", api_pic.width,
			  api_pic.height);
		return -EFAULT;
	}

	jpd_n_debug(LEVEL_ALWAYS, "maximum pic[%d, %d] success", api_pic.width, api_pic.height);

	return 0;
}

int platform_jpd_set_output_format(int handle, u8 format)
{
	platform_jpd_instance *ginst;
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely
	    (MApi_NJPD_Parser_SetOutputFormat(ginst->parser_handle, format) != E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_SetOutputFormat(%d) failed", format);
		return -EFAULT;
	}

	jpd_n_debug(LEVEL_ALWAYS, "format(%d) success", format);

	return 0;
}

int platform_jpd_set_verification_mode(int handle, u8 mode)
{
	platform_jpd_instance *ginst;
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely
	    (MApi_NJPD_Parser_SetVerificationMode(ginst->parser_handle, mode) != E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_SetVerificationMode(%d) failed", mode);
		return -EFAULT;
	}

	jpd_n_debug(LEVEL_ALWAYS, "mode(%d) success", mode);

	return 0;
}

platform_jpd_parser_result platform_jpd_prepare_decode(int handle,
						       platform_jpd_buf buf, void *buf_priv,
						       platform_jpd_base_cmd **cmd)
{
	platform_jpd_instance *ginst;
	NJPD_API_DecCmd api_cmd;
	NJPD_API_Buf api_buf = { buf.va, buf.pa, buf.offset, buf.filled_length, buf.size };
	NJPD_API_ParserResult ret;

	if (unlikely(_check_handle(handle) != 0)) {
		jpd_n_err(" check handle failed");
		return E_PLATFORM_JPD_PARSER_FAIL;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely(buf.va == NULL || buf.filled_length == 0 || cmd == NULL)) {
		jpd_n_err("bad params (%lx, 0x%zx, %lx)",
			  (uintptr_t) buf.va, buf.filled_length, (uintptr_t) cmd);
		return E_PLATFORM_JPD_PARSER_FAIL;
	}

	if (ginst->find_SOS) {
		platform_jpd_feed_read_cmd *feed_read_cmd =
		    _platform_jpd_alloc_cmd(E_PLATFORM_JPD_CMD_TYPE_FEED_READ, buf_priv);
		if (unlikely(feed_read_cmd == NULL)) {
			jpd_n_err("read cmd alloc failed");
			return E_PLATFORM_JPD_PARSER_FAIL;
		}

		feed_read_cmd->api_read_buf = api_buf;
		feed_read_cmd->index = ginst->read_index++;
		feed_read_cmd->decode_order = 0;

		*cmd = (platform_jpd_base_cmd *)feed_read_cmd;
		PRINT_READ_CMD(LEVEL_PARSER, "create", feed_read_cmd);
		return E_PLATFORM_JPD_PARSER_OK;
	}

	ret = MApi_NJPD_Parser_ParseHeader(ginst->parser_handle, api_buf, &api_cmd);

	if (likely(ret == E_NJPD_API_HEADER_VALID)) {
		platform_jpd_decode_cmd *decode_cmd =
		    _platform_jpd_alloc_cmd(E_PLATFORM_JPD_CMD_TYPE_DECODE, buf_priv);
		if (unlikely(decode_cmd == NULL)) {
			jpd_n_err("decode cmd alloc failed");
			return E_PLATFORM_JPD_PARSER_FAIL;
		}

		decode_cmd->api_cmd = api_cmd;
		decode_cmd->index = ginst->read_index++;

		*cmd = (platform_jpd_base_cmd *)decode_cmd;

		MApi_NJPD_Parser_Reset(ginst->parser_handle);
		if (ginst->format == E_PLATFORM_JPD_FORMAT_JPEG)
			ginst->find_SOS = true;

		PRINT_DECODE_CMD(LEVEL_PARSER, "create", decode_cmd);
		return E_PLATFORM_JPD_PARSER_OK;
	} else if (ret == E_NJPD_API_HEADER_PARTIAL) {
		jpd_n_debug(LEVEL_PARSER, "partial image");
		return E_PLATFORM_JPD_PARSER_PARTIAL;
	}

	MApi_NJPD_Parser_Reset(ginst->parser_handle);
	jpd_n_err("invalid image");
	return E_PLATFORM_JPD_PARSER_FAIL;
}

int platform_jpd_get_image_info(int handle, platform_jpd_image_info *image_info)
{
	platform_jpd_instance *ginst;
	NJPD_API_ParserImageInfo api_image_info;
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely(image_info == NULL)) {
		jpd_n_err("bad params (%lx)", (uintptr_t) image_info);
		return -EINVAL;
	}

	if (unlikely
	    (MApi_NJPD_Parser_GetImageInfo(ginst->parser_handle, &api_image_info) !=
	     E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_GetImageInfo failed");
		return -EFAULT;
	}

	image_info->width = api_image_info.width;
	image_info->height = api_image_info.height;

	jpd_n_debug(LEVEL_DEBUG, "%d x %d", image_info->width, image_info->height);

	return 0;
}

int platform_jpd_get_buf_info(int handle, platform_jpd_buf_info *buf_info)
{
	platform_jpd_instance *ginst;
	NJPD_API_ParserBufInfo api_buf_info;
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely(buf_info == NULL)) {
		jpd_n_err("bad params (%lx)", (uintptr_t) buf_info);
		return -EINVAL;
	}

	if (unlikely
	    (MApi_NJPD_Parser_GetBufInfo(ginst->parser_handle, &api_buf_info) != E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_GetBufInfo failed");
		return -EFAULT;
	}

	buf_info->width = api_buf_info.width;
	buf_info->height = api_buf_info.height;
	buf_info->rect.x = api_buf_info.rect.x;
	buf_info->rect.y = api_buf_info.rect.y;
	buf_info->rect.width = api_buf_info.rect.width;
	buf_info->rect.height = api_buf_info.rect.height;
	buf_info->write_buf_size = api_buf_info.write_buf_size;

	jpd_n_debug(LEVEL_DEBUG, "%d x %d[%d, %d, %d, %d](0x%x)",
		    buf_info->width, buf_info->height, buf_info->rect.x, buf_info->rect.y,
		    buf_info->rect.width, buf_info->rect.height, buf_info->write_buf_size);

	return 0;
}

int platform_jpd_start_port(int handle, platform_jpd_port port)
{
	platform_jpd_instance *ginst;
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	mutex_lock(&ginst->list_lock);
	if (port == E_PLATFORM_JPD_PORT_READ || port == E_PLATFORM_JPD_PORT_BOTH)
		ginst->read_port_enable = true;
	if (port == E_PLATFORM_JPD_PORT_WRITE || port == E_PLATFORM_JPD_PORT_BOTH)
		ginst->write_port_enable = true;
	mutex_unlock(&ginst->list_lock);

	jpd_n_debug(LEVEL_ALWAYS, "port(%d) success", port);

	return 0;
}

int platform_jpd_insert_cmd(int handle, platform_jpd_base_cmd *cmd)
{
	platform_jpd_instance *ginst;
	platform_jpd_decode_cmd *decode_cmd = (platform_jpd_decode_cmd *)cmd;
	platform_jpd_feed_read_cmd *feed_read_cmd = (platform_jpd_feed_read_cmd *)cmd;
	int ret;
	bool wake_up;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely(cmd == NULL)) {
		jpd_n_err("bad params (%lx)", (uintptr_t) cmd);
		return -EINVAL;
	}

	mutex_lock(&ginst->list_lock);
	if (unlikely(ginst->read_port_enable == false)) {
		mutex_unlock(&ginst->list_lock);
		jpd_n_err("read port disabled");
		if (cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_DECODE)
			PRINT_DECODE_CMD(LEVEL_CMD, "discard", decode_cmd);
		else
			PRINT_READ_CMD(LEVEL_CMD, "discard", feed_read_cmd);
		_platform_jpd_free_cmd(ginst, cmd);
		return -ENODEV;
	}
	wake_up = list_empty(&ginst->read_list);
	list_add_tail(&cmd->list, &ginst->read_list);
	WAKE_UP_TASK(wake_up);
	mutex_unlock(&ginst->list_lock);

	if (cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_DECODE)
		PRINT_DECODE_CMD(LEVEL_CMD, "insert", decode_cmd);
	else
		PRINT_READ_CMD(LEVEL_CMD, "insert", feed_read_cmd);

	return 0;
}

int platform_jpd_insert_write_buf(int handle, platform_jpd_buf buf, void *buf_priv)
{
	platform_jpd_instance *ginst;
	platform_jpd_feed_write_cmd *feed_write_cmd;
	NJPD_API_Buf api_buf = { buf.va, buf.pa, buf.offset, buf.filled_length, buf.size };
	int ret;
	bool wake_up;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	if (unlikely(buf.size == 0)) {
		jpd_n_err("bad params (0x%zx)", buf.size);
		return -EINVAL;
	}

	feed_write_cmd = _platform_jpd_alloc_cmd(E_PLATFORM_JPD_CMD_TYPE_FEED_WRITE, buf_priv);
	if (unlikely(feed_write_cmd == NULL)) {
		jpd_n_err("write cmd alloc failed");
		return -ENOMEM;
	}

	feed_write_cmd->base.timestamp = 0;
	feed_write_cmd->api_write_buf = api_buf;
	feed_write_cmd->index = ginst->write_index++;

	mutex_lock(&ginst->list_lock);
	if (unlikely(ginst->write_port_enable == false)) {
		mutex_unlock(&ginst->list_lock);
		jpd_n_err("write port disabled");
		PRINT_WRITE_CMD(LEVEL_CMD, "discard", feed_write_cmd);
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)feed_write_cmd);
		return -ENODEV;
	}
	wake_up = list_empty(&ginst->write_list);
	list_add_tail(&feed_write_cmd->base.list, &ginst->write_list);
	WAKE_UP_TASK(wake_up);
	mutex_unlock(&ginst->list_lock);

	PRINT_WRITE_CMD(LEVEL_CMD, "insert", feed_write_cmd);

	return 0;
}

int platform_jpd_flush_port(int handle, platform_jpd_port port)
{
	platform_jpd_instance *ginst;
	struct list_head *r_pos, *w_pos, *n;
	platform_jpd_base_cmd *base_cmd;
	platform_jpd_decode_cmd *decode_cmd;
	platform_jpd_feed_read_cmd *feed_read_cmd;
	platform_jpd_feed_write_cmd *feed_write_cmd;
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	mutex_lock(&ginst->list_lock);
	if (port == E_PLATFORM_JPD_PORT_READ || port == E_PLATFORM_JPD_PORT_BOTH) {
		ginst->read_port_enable = false;
		list_for_each_safe(r_pos, n, &ginst->read_list) {
			base_cmd = list_entry(r_pos, platform_jpd_base_cmd, list);

			if (base_cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_DECODE) {
				decode_cmd = (platform_jpd_decode_cmd *)base_cmd;
				PRINT_DECODE_CMD(LEVEL_CMD, "skip", decode_cmd);
				ginst->cb(base_cmd, E_PLATFORM_JPD_CMD_STATE_SKIP,
					ginst->user_priv);
			} else if (base_cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_FEED_READ) {
				feed_read_cmd = (platform_jpd_feed_read_cmd *)base_cmd;
				PRINT_READ_CMD(LEVEL_CMD, "skip", feed_read_cmd);
				ginst->cb(base_cmd, E_PLATFORM_JPD_CMD_STATE_SKIP,
					ginst->user_priv);
			} else if (base_cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_DRAIN) {
				jpd_n_debug(LEVEL_CMD, "skip drain cmd");
			} else if (base_cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_EOS) {
				jpd_n_debug(LEVEL_CMD, "skip eos cmd");
			} else {
				jpd_n_err("skip unknown cmd");
			}
			list_del(r_pos);
			_platform_jpd_free_cmd(ginst, base_cmd);
		}
	}
	if (port == E_PLATFORM_JPD_PORT_WRITE || port == E_PLATFORM_JPD_PORT_BOTH) {
		ginst->write_port_enable = false;
		list_for_each_safe(w_pos, n, &ginst->write_list) {
			base_cmd = list_entry(w_pos, platform_jpd_base_cmd, list);

			if (likely(base_cmd->cmd_type == E_PLATFORM_JPD_CMD_TYPE_FEED_WRITE)) {
				feed_write_cmd = (platform_jpd_feed_write_cmd *)base_cmd;
				PRINT_WRITE_CMD(LEVEL_CMD, "skip", feed_write_cmd);
			} else {
				jpd_n_err("skip unknown cmd");
			}
			ginst->cb(base_cmd, E_PLATFORM_JPD_CMD_STATE_SKIP, ginst->user_priv);
			list_del(w_pos);
			_platform_jpd_free_cmd(ginst, base_cmd);
		}
	}
	mutex_unlock(&ginst->list_lock);

	mutex_lock(&jpd_dev.jpd_dec.task_lock);
	if (handle == jpd_dev.jpd_dec.instance) {
		jpd_n_debug(LEVEL_ALWAYS, "flushing decoding instance, port(%d)", port);
		if (unlikely(MApi_NJPD_Decoder_Stop() != E_NJPD_API_OKAY))
			jpd_n_err("MApi_NJPD_Decoder_Stop failed");
		if (likely(jpd_dev.jpd_dec.cmd)) {
			_platform_jpd_free_cmd(ginst,
					       (platform_jpd_base_cmd *)jpd_dev.jpd_dec.cmd);
			jpd_dev.jpd_dec.cmd = NULL;
		} else {
			jpd_n_err("jpd_dec.cmd is NULL");
		}

		if (unlikely(list_empty(&jpd_dev.jpd_dec.data_list)))
			jpd_n_err("jpd_dec.data_list is empty");

		list_for_each_safe(r_pos, n, &jpd_dev.jpd_dec.data_list) {
			feed_read_cmd = list_entry(r_pos, platform_jpd_feed_read_cmd, base.list);

			PRINT_READ_CMD(LEVEL_CMD, "skip", feed_read_cmd);
			ginst->cb((platform_jpd_base_cmd *)feed_read_cmd,
				  E_PLATFORM_JPD_CMD_STATE_SKIP, ginst->user_priv);
			list_del(r_pos);
			_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)feed_read_cmd);
		}
		if (likely(jpd_dev.jpd_dec.write_buf)) {
			if (port == E_PLATFORM_JPD_PORT_READ) {
				mutex_lock(&ginst->list_lock);
				TRY_RESTORE_WRITE_CMD(ginst, jpd_dev.jpd_dec.write_buf);
				mutex_unlock(&ginst->list_lock);
			} else {
				PRINT_WRITE_CMD(LEVEL_CMD, "drop", jpd_dev.jpd_dec.write_buf);
				ginst->cb((platform_jpd_base_cmd *)jpd_dev.jpd_dec.write_buf,
					  E_PLATFORM_JPD_CMD_STATE_DROP, ginst->user_priv);
				_platform_jpd_free_cmd(ginst,
					(platform_jpd_base_cmd *)jpd_dev.jpd_dec.write_buf);
			}
			jpd_dev.jpd_dec.write_buf = NULL;
		} else {
			jpd_n_err("jpd_dec.write_buf is NULL");
		}

		jpd_dev.jpd_dec.instance = -1;
		jpd_dev.jpd_dec.state = E_PLATFORM_JPD_STOPPED;
	}
	mutex_unlock(&jpd_dev.jpd_dec.task_lock);

	jpd_n_debug(LEVEL_FLOW, "port(%d) success", port);

	return 0;
}

int platform_jpd_drain(int handle)
{
	platform_jpd_instance *ginst;
	platform_jpd_drain_cmd *drain_cmd;
	int ret;
	bool wake_up;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	drain_cmd = _platform_jpd_alloc_cmd(E_PLATFORM_JPD_CMD_TYPE_DRAIN, NULL);
	if (unlikely(drain_cmd == NULL)) {
		jpd_n_err("drain cmd alloc failed");
		return -ENOMEM;
	}

	mutex_lock(&ginst->list_lock);
	if (unlikely(ginst->read_port_enable == false)) {
		mutex_unlock(&ginst->list_lock);
		jpd_n_err("read port disabled");
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)drain_cmd);
		return -ENODEV;
	}
	wake_up = list_empty(&ginst->read_list);
	list_add_tail(&drain_cmd->base.list, &ginst->read_list);
	WAKE_UP_TASK(wake_up);
	mutex_unlock(&ginst->list_lock);

	jpd_n_debug(LEVEL_ALWAYS, "success");

	return 0;
}

int platform_jpd_eos(int handle)
{
	platform_jpd_instance *ginst;
	platform_jpd_eos_cmd *eos_cmd;
	int ret;
	bool wake_up;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	eos_cmd = _platform_jpd_alloc_cmd(E_PLATFORM_JPD_CMD_TYPE_EOS, NULL);
	if (unlikely(eos_cmd == NULL)) {
		jpd_n_err("eos cmd alloc failed");
		return -ENOMEM;
	}

	mutex_lock(&ginst->list_lock);
	if (unlikely(ginst->read_port_enable == false)) {
		mutex_unlock(&ginst->list_lock);
		jpd_n_err("read port disabled");
		_platform_jpd_free_cmd(ginst, (platform_jpd_base_cmd *)eos_cmd);
		return -ENODEV;
	}
	wake_up = list_empty(&ginst->read_list);
	list_add_tail(&eos_cmd->base.list, &ginst->read_list);
	WAKE_UP_TASK(wake_up);
	mutex_unlock(&ginst->list_lock);

	jpd_n_debug(LEVEL_ALWAYS, "success");

	return 0;
}

int platform_jpd_close(int handle)
{
	platform_jpd_instance *ginst;
	int ret;

	ret = _check_handle(handle);
	if (unlikely(ret != 0)) {
		jpd_n_err(" check handle failed(%d)", ret);
		return ret;
	}
	ginst = &jpd_dev.inst[handle];

	platform_jpd_flush_port(handle, E_PLATFORM_JPD_PORT_BOTH);

	mutex_lock(&jpd_dev.inst_lock);

	ginst->active = false;

	if (unlikely(MApi_NJPD_Parser_Close(ginst->parser_handle) != E_NJPD_API_OKAY)) {
		jpd_n_err("MApi_NJPD_Parser_Close failed");
		mutex_unlock(&jpd_dev.inst_lock);
		return -EFAULT;
	}
	mutex_unlock(&jpd_dev.inst_lock);

	jpd_n_debug(LEVEL_FLOW, "leak check(%d, %d, %d, %d, %d, %d)",
		    platform_jpd_decode_cmd_leak, platform_jpd_read_cmd_leak,
		    platform_jpd_write_cmd_leak, platform_jpd_drain_cmd_leak,
		    platform_jpd_eos_cmd_leak, platform_jpd_inter_buf_leak);
	jpd_n_debug(LEVEL_FLOW, "success");

	return 0;
}

irqreturn_t platform_jpd_irq(int irq, void *priv)
{
	MApi_NJPD_Decoder_IRQ();
	if (jpd_dev.task_wake_up_flag == false) {
		jpd_dev.task_wake_up_flag = true;
		jpd_debug(LEVEL_IRQ, "wake up");
		wake_up_interruptible(&jpd_dev.task_wq);
	}
	return IRQ_HANDLED;
}

int platform_jpd_free_cmd(int handle, platform_jpd_base_cmd *cmd)
{
	void *ginst = NULL;

	if (likely(handle >= 0 && handle < MAX_PLATFORM_JPD_INSTANCE_NUM))
		ginst = &jpd_dev.inst[handle];

	_platform_jpd_free_cmd(ginst, cmd);

	return 0;
}
