/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#ifndef _LINUX_MTK_TVA_TRACE_H
#define _LINUX_MTK_TVA_TRACE_H
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>

#define atrace_init(format, ...)
/**
 * Trace the beginning of a context.  name is used to identify the context.
 * This is often used to time function execution.
 */
void atrace_begin(const char *name);

/**
 * Trace the end of a context.
 * This should match up (and occur after) a corresponding ATRACE_BEGIN.
 */
void atrace_end(const char *name);

/**
 * Trace the beginning of an asynchronous event. Unlike ATRACE_BEGIN/ATRACE_END
 * contexts, asynchronous events do not need to be nested. The name describes
 * the event, and the cookie provides a unique identifier for distinguishing
 * simultaneous events. The name and cookie used to begin an event must be
 * used to end it.
 */
void atrace_async_begin(const char *name, int32_t cookie);

/**
 * Trace the end of an asynchronous event.
 * This should have a corresponding ATRACE_ASYNC_BEGIN.
 */
void atrace_async_end(const char *name, int32_t cookie);

/**
 * Traces an integer counter value.  name is used to identify the counter.
 * This can be used to track how a value changes over time.
 */
void atrace_int(const char *name, int32_t value);
/**
 * Traces a 64-bit integer counter value.  name is used to identify the
 * counter. This can be used to track how a value changes over time.
 */
void atrace_int64(const char *name, int64_t value);

#endif /* _LINUX_MTK_TVA_TRACE_H */
