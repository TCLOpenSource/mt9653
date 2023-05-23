// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>

#ifdef TRACE_PUT
#undef TRACE_PUT
#endif

#define TRACE_BUF_SIZE 256
#ifdef MET_SUPPORT
#define MET_OFFSET	65535
#else
#define MET_OFFSET 0
#endif

#define TRACE_PUTS(p) \
	do { \
		trace_puts(p);; \
	} while (0)

/**
 * Trace the beginning of a context.  name is used to identify the context.
 * This is often used to time function execution.
 */
static noinline int tracing_mark_write(const char *buf)
{
	TRACE_PUTS(buf);
	return 0;
}

void atrace_begin(const char *name)
{
	char buf[TRACE_BUF_SIZE];

	int len = snprintf(buf, sizeof(buf),
		"B|%d|%s", (current->tgid + MET_OFFSET), name);

	if (len >= TRACE_BUF_SIZE)
		len = TRACE_BUF_SIZE - 1;

	tracing_mark_write(buf);
}
EXPORT_SYMBOL(atrace_begin);

/**
 * Trace the end of a context.
 * This should match up (and occur after) a corresponding ATRACE_BEGIN.
 */
void atrace_end(const char *name)
{
	char buf[TRACE_BUF_SIZE];

	int len = snprintf(buf, sizeof(buf),
		"E|%s", name);

	if (len >= TRACE_BUF_SIZE)
		len = TRACE_BUF_SIZE - 1;

	tracing_mark_write(buf);
}
EXPORT_SYMBOL(atrace_end);

/**
 * Trace the beginning of an asynchronous event. Unlike ATRACE_BEGIN/ATRACE_END
 * contexts, asynchronous events do not need to be nested. The name describes
 * the event, and the cookie provides a unique identifier for distinguishing
 * simultaneous events. The name and cookie used to begin an event must be
 * used to end it.
 */
void atrace_async_begin(const char *name, int32_t cookie)
{
	char buf[TRACE_BUF_SIZE];

	int len = snprintf(buf, sizeof(buf),
		"S|%d|%s|%d", (current->tgid + MET_OFFSET), name, cookie);

	if (len >= TRACE_BUF_SIZE)
		len = TRACE_BUF_SIZE - 1;

	tracing_mark_write(buf);
}
EXPORT_SYMBOL(atrace_async_begin);

/**
 * Trace the end of an asynchronous event.
 * This should have a corresponding ATRACE_ASYNC_BEGIN.
 */
void atrace_async_end(const char *name, int32_t cookie)
{
	char buf[TRACE_BUF_SIZE];

	int len = snprintf(buf, sizeof(buf),
		"F|%d|%s|%d", (current->tgid + MET_OFFSET), name, cookie);

	if (len >= TRACE_BUF_SIZE)
		len = TRACE_BUF_SIZE - 1;

	tracing_mark_write(buf);
}
EXPORT_SYMBOL(atrace_async_end);

/**
 * Traces an integer counter value.  name is used to identify the counter.
 * This can be used to track how a value changes over time.
 */
void atrace_int(const char *name, int32_t value)
{
	char buf[TRACE_BUF_SIZE];

	int len = snprintf(buf, sizeof(buf),
		"C|%d|%s|%d", (current->tgid + MET_OFFSET), name, value);

	if (len >= TRACE_BUF_SIZE)
		len = TRACE_BUF_SIZE - 1;

	tracing_mark_write(buf);
}
EXPORT_SYMBOL(atrace_int);

/**
 * Traces a 64-bit integer counter value.  name is used to identify the
 * counter. This can be used to track how a value changes over time.
 */
void atrace_int64(const char *name, int64_t value)
{
	char buf[TRACE_BUF_SIZE];

	int len = snprintf(buf, sizeof(buf),
		"C|%d|%s|%lld", (current->tgid + MET_OFFSET), name, value);

	if (len >= TRACE_BUF_SIZE)
		len = TRACE_BUF_SIZE - 1;

	tracing_mark_write(buf);
}
EXPORT_SYMBOL(atrace_int64);


static int __init mtk_tv_atrace_probe(void)
{
	return 0;
}

static void __exit mtk_tv_atrace_remove(void)
{

}

module_init(mtk_tv_atrace_probe);
module_exit(mtk_tv_atrace_remove);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("MTK TV atrace driver");
