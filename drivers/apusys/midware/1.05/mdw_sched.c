// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kref.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>

#include "mdw_cmn.h"
#include "mdw_queue.h"
#include "mdw_rsc.h"
#include "mdw_cmd.h"
#include "mdw_sched.h"
#include "mdw_dispr.h"
#include "mdw_trace.h"
#ifdef APUSYS_OPTIONS_INF_MNOC
#include "mnoc_api.h"
#endif
#ifndef AI_SIM
#include "reviser_export.h"
#endif
#include "mdw_tag.h"
#define CREATE_TRACE_POINTS
#include "mdw_events.h"

MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

#ifdef COMMAND_UTILIZATION

#define NSEC_PER_JIFFY   ((NSEC_PER_SEC+HZ/2)/HZ)

#define EXEC_TIME_LOG_PERIOD_MIN (HZ)
#define EXEC_TIME_LOG_PERIOD_MAX (10*HZ)
#define EXEC_TIME_LOG_PERIOD_DEFAULT (HZ)

int call_times;
int call_times2;
int proc_call_times;
struct exec_time_log {
	struct list_head node;
	u64 s_ktime_ns;
	u64 e_ktime_ns;
};

u32 exec_time_log_period = EXEC_TIME_LOG_PERIOD_DEFAULT;
//void exec_time_log_timer_handler(struct timer_list *data);
DEFINE_TIMER(exec_time_log_timer, exec_time_log_timer_handler);
struct list_head exec_time_log_list;
struct mutex exec_time_log_list_lock;
void exec_time_log_init(void)
{
	call_times = 0;
	call_times2 = 0;
	INIT_LIST_HEAD(&exec_time_log_list);
	mutex_init(&exec_time_log_list_lock);
	mod_timer(&exec_time_log_timer, jiffies + exec_time_log_period);
}

void exec_time_log_proc(struct exec_time_result *result)
{
	struct exec_time_log *entry, *tmp;
	u64 current_ktime_ns;
	u64 exec_time = 0;
	int proc_call_times_location1 = 0;
	int proc_call_times_location2 = 0;
	int proc_call_times_location3 = 0;
	int proc_call_times_location4 = 0;

	//mdw_flw_debug("enter exec time log proc\n");
	if (list_empty(&exec_time_log_list)) {
		//return 0;
		goto proc_error;
	}

	//mdw_flw_debug("exec time log proc location 1\n");
	mutex_lock(&exec_time_log_list_lock);
	if (result) {
		result->min = 0xffffffff;
		result->max = 0;
		result->avg = 0;
		result->tot = 0;
		result->cnt = 0;
	}
	proc_call_times = 0;
	current_ktime_ns = ktime_get_ns();

	list_for_each_entry_safe(entry, tmp, &exec_time_log_list, node) {
		proc_call_times++;
		if (entry->e_ktime_ns &&
entry->e_ktime_ns <= (current_ktime_ns - (exec_time_log_period * NSEC_PER_JIFFY))) {
			list_del(&entry->node);
			kfree(entry);
			proc_call_times_location1++;
			//mdw_flw_debug("exec time log proc location 2\n");
			continue;
		}
		if (entry->s_ktime_ns <=
(current_ktime_ns - (exec_time_log_period * NSEC_PER_JIFFY))) {
			entry->s_ktime_ns =
		current_ktime_ns - (exec_time_log_period * NSEC_PER_JIFFY);
			proc_call_times_location2++;
			//mdw_flw_debug("exec time log proc location 3\n");
		}
		if (entry->e_ktime_ns) {
			exec_time = entry->e_ktime_ns - entry->s_ktime_ns;
			proc_call_times_location3++;
			//mdw_flw_debug("exec time log proc location 4\n");
		} else {
			exec_time = current_ktime_ns - entry->s_ktime_ns;
			proc_call_times_location4++;
			//mdw_flw_debug("exec time log proc location 5\n");
		}
		if (result) {
			result->cnt++;
			result->tot += (u32)exec_time;
			if (result->min > (u32)exec_time)
				result->min = (u32)exec_time;
			if (result->max < (u32)exec_time)
				result->max = (u32)exec_time;
		}
	};
	mutex_unlock(&exec_time_log_list_lock);

	if (result && (result->cnt > 0))
		result->avg = result->tot / result->cnt;

	mdw_flw_debug("proc_call_times =%d\n", proc_call_times);
	//mdw_mem_debug("proc_call_times =%d\n",proc_call_times);

	mdw_flw_debug("proc_call_times_location1 =%d\n", proc_call_times_location1);
	//mdw_flw_debug("proc_call_times_location2 =%d\n",proc_call_times_location2);
	//mdw_flw_debug("proc_call_times_location3 =%d\n",proc_call_times_location3);
	//mdw_flw_debug("proc_call_times_location4 =%d\n",proc_call_times_location4);
	//mdw_flw_debug("exec time log proc location 6\n");
proc_error:
	return;
}
EXPORT_SYMBOL(exec_time_log_proc);

void exec_time_log_clear(void)
{
	struct exec_time_log *entry, *tmp;
	u64 current_ktime_ns;
	//mdw_flw_debug("exec time log clear location 1\n");
	if (list_empty(&exec_time_log_list)) {
		mdw_flw_debug("exec time log clear location 2\n");
		return;
	}
	//mdw_flw_debug("exec time log clear location 3\n");
	mutex_lock(&exec_time_log_list_lock);
	current_ktime_ns = ktime_get_ns();
	list_for_each_entry_safe(entry, tmp, &exec_time_log_list, node) {
		if (entry->e_ktime_ns &&
entry->e_ktime_ns <= (current_ktime_ns - (exec_time_log_period * NSEC_PER_JIFFY))) {
			list_del(&entry->node);
			//mdw_flw_debug("exec time log clear location 4\n");
			//mdw_flw_debug("statistic call times = %d\n", call_times);
			//call_times2++;
			kfree(entry);
		}
	};
	mdw_mem_debug("total statistic call times = %d\n", call_times);
	//mdw_mem_debug("total statistic call times2 = %d\n", call_times2);
	call_times = 0;
	//call_times2 = 0;
	mutex_unlock(&exec_time_log_list_lock);
	//mdw_flw_debug("exec time log clear location 5\n");
}

//static void exec_time_log_timer_handler(struct timer_list *data);
//static DEFINE_TIMER(exec_time_log_timer, exec_time_log_timer_handler);

void exec_time_log_timer_handler(struct timer_list *data)
{
	//mdw_flw_debug("statistic call times = %d\n", call_times);
	//call_times = 0;
	exec_time_log_clear();
	mod_timer(&exec_time_log_timer, jiffies + exec_time_log_period);
}

//struct exec_time_log *entry;
void *exec_time_log_start(void)
{
	struct exec_time_log *entry;

	if (utilization_init_timer_flag == UTILIZATION_FLAG_ON) {
		//mdw_flw_debug("record utilization\n");
		entry = kzalloc(sizeof(struct exec_time_log), GFP_KERNEL);
		if (entry) {
			//mdw_flw_debug("record utilization location1\n");
			INIT_LIST_HEAD(&entry->node);
			//mdw_flw_debug("record utilization location2\n");
			mutex_lock(&exec_time_log_list_lock);
			//mdw_flw_debug("record utilization location3\n");
			entry->s_ktime_ns = ktime_get_ns();
			//mdw_flw_debug("record utilization location4\n");
			list_add_tail(&entry->node, &exec_time_log_list);
			//mdw_flw_debug("record utilization location5\n");
			call_times++;
			mutex_unlock(&exec_time_log_list_lock);
		}
		//mdw_flw_debug("record utilization location6\n");
		return (void *)entry;
	} else
		return NULL;
}
EXPORT_SYMBOL(exec_time_log_start);

void exec_time_log_end(void *p)
{
	struct exec_time_log *entry;

	if (utilization_init_timer_flag == UTILIZATION_FLAG_ON) {
		entry = (struct exec_time_log *)p;
		mutex_lock(&exec_time_log_list_lock);
		//mdw_flw_debug("after inference record utilization 1\n");
		entry->e_ktime_ns = ktime_get_ns();
		//call_times++;
		//mdw_flw_debug("after inference record utilization 2\n");
		mutex_unlock(&exec_time_log_list_lock);
		//mdw_flw_debug("after inference record utilization 3\n");
	}
}
EXPORT_SYMBOL(exec_time_log_end);

#endif


struct mdw_sched_mgr {
	struct task_struct *task;
	struct completion cmplt;

	struct list_head ds_list; //done sc list
	struct mutex mtx;

	bool pause;
	bool stop;
};

static struct mdw_cmd_parser *cmd_parser;
static struct mdw_sched_mgr ms_mgr;

#define MDW_EXEC_PRINT " pid(%d/%d) cmd(0x%llx/0x%llx-#%d/%u)"\
	" dev(%d/%s-#%d) mp(%u/%u/%u/0x%llx) sched(%d/%u/%u/%u/%u/%d)"\
	" mem(%lu/%d/0x%x/0x%x) boost(%u) time(%u/%u)"

static void mdw_sched_met_start(struct mdw_apu_sc *sc, struct mdw_dev_info *d)
{
	mdw_trace_begin("apusys_scheduler|dev: %d_%d, cmd_id: 0x%08llx",
		d->type,
		d->idx,
		sc->parent->kid);
}

static void mdw_sched_met_end(struct mdw_apu_sc *sc, struct mdw_dev_info *d,
	int ret)
{
	mdw_trace_end("apusys_scheduler|dev: %d_%d, cmd_id: 0x%08llx, ret:%d",
		d->type,
		d->idx,
		sc->parent->kid, ret);
}

static void mdw_sched_trace(struct mdw_apu_sc *sc,
	struct mdw_dev_info *d, struct apusys_cmd_hnd *h, int ret, int done)
{
	char state[16];

	/* prefix */
	memset(state, 0, sizeof(state));
	if (!done) {
		mdw_sched_met_start(sc, d);
		if (snprintf(state, sizeof(state)-1, "start :") < 0)
			return;
	} else {
		mdw_sched_met_end(sc, d, ret);
		if (ret) {
			if (snprintf(state, sizeof(state)-1, "fail :") < 0)
				return;
		} else {
			if (snprintf(state, sizeof(state)-1, "done :") < 0)
				return;
		}
	}

	/* if err, use mdw_drv_err */
	if (ret) {
		mdw_drv_err("%s"MDW_EXEC_PRINT" ret(%d)\n",
			state,
			sc->parent->pid,
			sc->parent->tgid,
			sc->parent->hdr->uid,
			sc->parent->kid,
			sc->idx,
			sc->parent->hdr->num_sc,
			d->type,
			d->name,
			d->idx,
			sc->pack_id,
			h->multicore_idx,
			sc->multi_total,
			sc->multi_bmp,
			sc->parent->hdr->priority,
			sc->parent->hdr->soft_limit,
			sc->parent->hdr->hard_limit,
			sc->hdr->ip_time,
			sc->hdr->suggest_time,
			0,//sc->par_cmd->power_save,
			sc->ctx,
			sc->hdr->tcm_force,
			sc->hdr->tcm_usage,
			sc->real_tcm_usage,
			h->boost_val,
			h->ip_time,
			sc->driver_time,
			ret);
	} else {
		mdw_drv_debug("%s"MDW_EXEC_PRINT" ret(%d)\n",
			state,
			sc->parent->pid,
			sc->parent->tgid,
			sc->parent->hdr->uid,
			sc->parent->kid,
			sc->idx,
			sc->parent->hdr->num_sc,
			d->type,
			d->name,
			d->idx,
			sc->pack_id,
			h->multicore_idx,
			sc->multi_total,
			sc->multi_bmp,
			sc->parent->hdr->priority,
			sc->parent->hdr->soft_limit,
			sc->parent->hdr->hard_limit,
			sc->hdr->ip_time,
			sc->hdr->suggest_time,
			0,//sc->par_cmd->power_save,
			sc->ctx,
			sc->hdr->tcm_force,
			sc->hdr->tcm_usage,
			sc->real_tcm_usage,
			h->boost_val,
			h->ip_time,
			sc->driver_time,
			ret);
	}

	/* trace cmd end */
//	trace_mdw_cmd(done,
//		sc->parent->pid,
//		sc->parent->tgid,
//		sc->parent->hdr->uid,
//		sc->parent->kid,
//		sc->idx,
//		sc->parent->hdr->num_sc,
//		d->type,
//		d->name,
//		d->idx,
//		sc->pack_id,
//		h->multicore_idx,
//		sc->multi_total,
//		sc->multi_bmp,
//		sc->parent->hdr->priority,
//		sc->parent->hdr->soft_limit,
//		sc->parent->hdr->hard_limit,
//		sc->hdr->ip_time,
//		sc->hdr->suggest_time,
//		0,//sc->par_cmd->power_save,
//		sc->ctx,
//		sc->hdr->tcm_force,
//		sc->hdr->tcm_usage,
//		sc->real_tcm_usage,
//		h->boost_val,
//		h->ip_time,
//		ret);
}
#undef MDW_EXEC_PRINT

static int mdw_sched_sc_done(void)
{
	struct mdw_apu_cmd *c = NULL;
	struct mdw_apu_sc *sc = NULL, *s = NULL;
	int ret = 0;

	mdw_flw_debug("\n");
	mdw_trace_begin("check done list|%s", __func__);

	/* get done sc from done sc list */
	mutex_lock(&ms_mgr.mtx);
	s = list_first_entry_or_null(&ms_mgr.ds_list,
		struct mdw_apu_sc, ds_item);
	if (s)
		list_del(&s->ds_item);
	mutex_unlock(&ms_mgr.mtx);
	if (!s) {
		ret = -ENODATA;
		goto out;
	}
	/* recv finished subcmd */
	while (1) {
		c = s->parent;
		ret = cmd_parser->end_sc(s, &sc);
		mdw_flw_debug("\n");
		/* check return value */
		if (ret) {
			mdw_drv_err("parse done sc fail(%d)\n", ret);
			complete(&c->cmplt);
			break;
		}
		/* check parsed sc */
		if (sc) {
		/*
		 * finished sc parse ok, should be call
		 * again because residual sc
		 */
			mdw_flw_debug("sc(0x%llx-#%d)\n",
				sc->parent->kid, sc->idx);
			mdw_sched(sc);
		} else {
		/* finished sc parse done, break loop */
			mdw_sched(NULL);
			break;
		}
	};

out:
	mdw_trace_end("check done list|%s", __func__);
	return ret;
}

static void mdw_sched_enque_done_sc(struct kref *ref)
{
	struct mdw_apu_sc *sc =
		container_of(ref, struct mdw_apu_sc, multi_ref);

	mdw_flw_debug("sc(0x%llx-#%d)\n", sc->parent->kid, sc->idx);
	cmd_parser->put_ctx(sc);

	mutex_lock(&ms_mgr.mtx);
	list_add_tail(&sc->ds_item, &ms_mgr.ds_list);
	mutex_unlock(&ms_mgr.mtx);

	mdw_sched(NULL);
}

int mdw_sched_dev_routine(void *arg)
{
	int ret = 0;
	struct mdw_dev_info *d = (struct mdw_dev_info *)arg;
	struct apusys_cmd_hnd h;
	struct mdw_apu_sc *sc = NULL;

	/* execute */
	while (!kthread_should_stop() && d->stop == false) {
		mdw_flw_debug("\n");
		ret = wait_for_completion_interruptible(&d->cmplt);
		if (ret)
			goto next;

		sc = (struct mdw_apu_sc *)d->sc;
		if (!sc) {
			mdw_drv_warn("no sc to exec\n");
			goto next;
		}

		/* get mem ctx */
		if (cmd_parser->get_ctx(sc)) {
			mdw_drv_err("cmd(0x%llx-#%d) get ctx fail\n",
				sc->parent->kid, sc->idx);
			goto next;
		}

		/* construct cmd hnd */
		mdw_queue_boost(sc);
		if (cmd_parser->set_hnd(sc, d->idx, &h)) {
			mdw_drv_err("cmd(0x%llx-#%d) set hnd fail\n",
				sc->parent->kid, sc->idx);
			goto next;
		}

		mdw_trace_begin("dev(%s-%d) exec|sc(0x%llx-%d) boost(%d/%u)",
			d->name, d->idx, sc->parent->kid, sc->idx,
			h.boost_val, sc->boost);
#ifndef AI_SIM
		/*
		 * Execute reviser to switch VLM:
		 * Skip set context on preemptive command,
		 * context should be set by engine driver itself.
		 * Give engine a callback to set context id.
		 */
		if (d->type != APUSYS_DEVICE_MDLA &&
			d->type != APUSYS_DEVICE_MDLA_RT) {
			reviser_set_context(d->type,
					d->idx, sc->ctx);
		}
#endif

#ifdef APUSYS_OPTIONS_INF_MNOC
		/* count qos start */
		apu_cmd_qos_start(sc->parent->kid, sc->idx,
			sc->type, d->idx, h.boost_val);
#endif
		/* execute */
		mdw_sched_trace(sc, d, &h, ret, 0);
		getnstimeofday(&sc->ts_start);
		ret = d->dev->send_cmd(APUSYS_CMD_EXECUTE, &h, d->dev);
		getnstimeofday(&sc->ts_end);
		sc->driver_time = mdw_cmn_get_time_diff(&sc->ts_start,
			&sc->ts_end);
		mdw_sched_trace(sc, d, &h, ret, 1);

		/* clr hnd */
		cmd_parser->clr_hnd(sc, &h);

#ifdef APUSYS_OPTIONS_INF_MNOC
		/* count qos end */
		mutex_lock(&sc->mtx);
		sc->bw += apu_cmd_qos_end(sc->parent->kid, sc->idx,
			sc->type, d->idx);
		sc->ip_time = sc->ip_time > h.ip_time ? sc->ip_time : h.ip_time;
		sc->boost = h.boost_val;
		sc->status = ret;
		mdw_flw_debug("multi bmp(0x%llx)\n", sc->multi_bmp);
		mutex_unlock(&sc->mtx);
#else
		mutex_lock(&sc->mtx);
		sc->ip_time = sc->ip_time > h.ip_time ? sc->ip_time : h.ip_time;
		mutex_unlock(&sc->mtx);
#endif

		/* put device */
		if (mdw_rsc_put_dev(d))
			mdw_drv_err("put dev(%d-#%d) fail\n",
				d->type, d->dev->idx);

		mdw_flw_debug("sc(0x%llx-#%d) ref(%d)\n", sc->parent->kid,
			sc->idx, kref_read(&sc->multi_ref));
		kref_put(&sc->multi_ref, mdw_sched_enque_done_sc);

		mdw_trace_end("dev(%s-%d) exec|boost(%d)",
			d->name, d->idx, h.boost_val);
next:
		mdw_flw_debug("done\n");
		continue;
	}

	complete(&d->thd_done);

	return 0;
}

static int mdw_sched_get_type(uint64_t bmp)
{
	uint64_t *local_bmp_p;
	unsigned long tmp[BITS_TO_LONGS(APUSYS_DEVICE_MAX)];

	local_bmp_p = (uint64_t *) &bmp;
	memset(&tmp, 0, sizeof(tmp));
	//bitmap_from_u32array(tmp, APUSYS_DEVICE_MAX, (const uint32_t *)&bmp, 2);
	bitmap_from_arr32(tmp, (const uint32_t *)local_bmp_p, APUSYS_DEVICE_MAX);

	return find_last_bit((unsigned long *)&tmp, APUSYS_DEVICE_MAX);
}

static int mdw_sched_dispatch(struct mdw_apu_sc *sc)
{
	int ret = 0, dev_num = 0, exec_num = 0;

	mdw_trace_begin("mdw dispatch|sc(0x%llx-%d) pack(%u) multi(%d)",
		sc->parent->kid, sc->idx, sc->pack_id, sc->parent->multi);

	/* get dev num */
	dev_num =  mdw_rsc_get_dev_num(sc->type);

	/* check exec #dev */
	exec_num = cmd_parser->exec_core_num(sc);
	exec_num = exec_num < dev_num ? exec_num : dev_num;
	sc->multi_total = exec_num;

	/* select dispatch policy */
	if (sc->pack_id)
		ret = mdw_dispr_pack(sc);
	else if (sc->parent->multi == HDR_FLAG_MULTI_MULTI && exec_num >= 2)
		ret = mdw_dispr_multi(sc);
	else
		ret = mdw_dispr_norm(sc);

	mdw_trace_end("mdw dispatch|ret(%d)", ret);

	return ret;
}

static int mdw_sched_routine(void *arg)
{
	int ret = 0;
	struct mdw_apu_sc *sc = NULL;
	uint64_t bmp = 0;
	int t = 0;

	mdw_flw_debug("\n");

	while (!kthread_should_stop() && !ms_mgr.stop) {
		ret = wait_for_completion_interruptible(&ms_mgr.cmplt);
		if (ret)
			mdw_drv_warn("sched ret(%d)\n", ret);

		if (ms_mgr.pause == true)
			continue;

		if (!mdw_sched_sc_done()) {
			mdw_sched(NULL);
			goto next;
		}

		mdw_dispr_check();

		bmp = mdw_rsc_get_avl_bmp();
		t = mdw_sched_get_type(bmp);
		if (t < 0 || t >= APUSYS_DEVICE_MAX) {
			mdw_flw_debug("nothing to sched(%d)\n", t);
			goto next;
		}

		/* get queue */
		sc = mdw_queue_pop(t);
		if (!sc) {
			mdw_drv_err("pop sc(%d) fail\n", t);
			goto fail_pop_sc;
		}
		mdw_flw_debug("pop sc(0x%llx-#%d/%d/%llu)\n",
			sc->parent->kid, sc->idx, sc->type, sc->period);

		/* dispatch cmd */
		ret = mdw_sched_dispatch(sc);
		if (ret) {
			mdw_flw_debug("sc(0x%llx-#%d) dispatch fail",
				sc->parent->kid, sc->idx);
			goto fail_exec_sc;
		}

		goto next;

fail_exec_sc:
	if (mdw_queue_insert(sc, true)) {
		mdw_drv_err("sc(0x%llx-#%d) insert fail\n",
			sc->parent->kid, sc->idx);
	}
fail_pop_sc:
next:
		mdw_flw_debug("\n");
		if (mdw_rsc_get_avl_bmp())
			mdw_sched(NULL);
	}

	mdw_drv_warn("schedule thread end\n");

	return 0;
}

int mdw_sched(struct mdw_apu_sc *sc)
{
	int ret = 0;

	/* no input sc, trigger sched thread only */
	if (!sc) {
		complete(&ms_mgr.cmplt);
		return 0;
	}

	/* insert sc to queue */
	ret = mdw_queue_insert(sc, false);
	if (ret) {
		mdw_drv_err("sc(0x%llx-#%d) enque fail\n",
			sc->parent->kid, sc->idx);
		return ret;
	}

	complete(&ms_mgr.cmplt);

	mdw_flw_debug("\n");

	return 0;
}

int mdw_sched_pause(void)
{
	struct mdw_dev_info *d = NULL;
	int type = 0, idx = 0, ret = 0, i = 0;

	if (ms_mgr.pause == true) {
		mdw_drv_warn("pause ready\n");
		return 0;
	}

	ms_mgr.pause = true;

	for (type = 0; type < APUSYS_DEVICE_MAX; type++) {
		for (idx = 0; idx < mdw_rsc_get_dev_num(type); idx++) {
			d = mdw_rsc_get_dinfo(type, idx);
			if (!d)
				continue;
			ret = d->suspend(d);
			if (ret) {
				mdw_drv_err("dev(%s%d) suspend fail(%d)\n",
					d->name, d->idx, ret);
				goto fail_sched_pause;
			}
		}
	}

	mdw_drv_info("pause\n");
	goto out;

fail_sched_pause:
	for (idx -= 1; idx >= 0; idx--) {
		d = mdw_rsc_get_dinfo(type, idx);
		if (!d)
			continue;
		if (d->resume(d)) {
			mdw_drv_err("dev(%s%d) resume fail(%d)\n",
				d->name, d->idx, ret);
		}
	}

	for (i = 0; i < type; i++) {
		for (idx = 0; idx < mdw_rsc_get_dev_num(type); idx++) {
			d = mdw_rsc_get_dinfo(type, idx);
			if (!d)
				continue;
			if (d->resume(d)) {
				mdw_drv_err("dev(%s%d) resume fail(%d)\n",
					d->name, d->idx, ret);
			}
		}
	}

	ms_mgr.pause = false;
	mdw_drv_warn("resume\n");
out:
	return ret;
}

void mdw_sched_restart(void)
{
	struct mdw_dev_info *d = NULL;
	int type = 0, idx = 0, ret = 0;

	if (ms_mgr.pause == false)
		mdw_drv_warn("resume ready\n");
	else
		mdw_drv_info("resume\n");

	for (type = 0; type < APUSYS_DEVICE_MAX; type++) {
		for (idx = 0; idx < mdw_rsc_get_dev_num(type); idx++) {
			d = mdw_rsc_get_dinfo(type, idx);
			if (!d)
				continue;
			ret = d->resume(d);
			if (ret)
				mdw_drv_err("dev(%s%d) resume fail(%d)\n",
				d->name, d->idx, ret);
		}
	}

	ms_mgr.pause = false;
	mdw_sched(NULL);
}

void mdw_sched_set_thd_group(void)
{
	//struct file *fd;
	char buf[8];
	mm_segment_t oldfs;

	oldfs = get_fs();
	//set_fs(get_ds());
	set_fs(KERNEL_DS);

	//fd = filp_open(APUSYS_THD_TASK_FILE_PATH, O_WRONLY, 0);
	//if (IS_ERR(fd)) {
	if (1) {
		mdw_drv_debug("don't support low latency group\n");
		goto out;
	}

	memset(buf, 0, sizeof(buf));
	if (snprintf(buf, sizeof(buf)-1, "%d", ms_mgr.task->pid) < 0)
		goto fail_set_name;
	//vfs_write(fd, (__force const char __user *)buf,
		//sizeof(buf), &fd->f_pos);
	//kernel_write(fd, (__force const char __user *)buf,
		//sizeof(buf), &fd->f_pos);
	mdw_drv_debug("setup worker(%d/%s) to group\n",
		ms_mgr.task->pid, buf);

fail_set_name:
	//filp_close(fd, NULL);
out:
	set_fs(oldfs);
}

int mdw_sched_init(void)
{
	memset(&ms_mgr, 0, sizeof(ms_mgr));
	ms_mgr.pause = false;
	ms_mgr.stop = false;
	init_completion(&ms_mgr.cmplt);
	INIT_LIST_HEAD(&ms_mgr.ds_list);
	mutex_init(&ms_mgr.mtx);

	cmd_parser = mdw_cmd_get_parser();
	if (!cmd_parser)
		return -ENODEV;

	ms_mgr.task = kthread_run(mdw_sched_routine,
		NULL, "apusys_sched");
	if (!ms_mgr.task) {
		mdw_drv_err("create kthread(sched) fail\n");
		return -ENOMEM;
	}

	mdw_dispr_init();

	return 0;
}

void mdw_sched_exit(void)
{
	ms_mgr.stop = true;
	mdw_sched(NULL);
	mdw_dispr_exit();
}
