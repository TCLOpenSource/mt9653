// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------------------------------------
#include "mtk_sti_msos.h"
#include "mtk_njpeg_def.h"
#include "mtk_jpeg_memory.h"

//-------------------------------------------------------------------------------------------------
// Local Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Local Defines
//-------------------------------------------------------------------------------------------------
#define HLEN	(sizeof(__memt__))
#define MIN_BLOCK (HLEN * 4)

#define AVAIL (__mem_avail__[0])

#define MIN_POOL_SIZE (HLEN * 10)

#define MEM_PARTITION 2

//-------------------------------------------------------------------------------------------------
// Local Structures
//-------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// @brief \b Struct \b Name: __memt__
/// @brief \b Struct \b Description: mpool structure for jpd
//-----------------------------------------------------------------------------
typedef struct __mem__ {
	struct __mem__ *next;	///< single-linked list
	__u32 len;		///< length of following block
} __memt__, *__memp__;


//-------------------------------------------------------------------------------------------------
// Global Variables
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------
__memt__ __mem_avail__[MEM_PARTITION] = {
	{NULL, 0},
	{NULL, 0},
};


//-------------------------------------------------------------------------------------------------
// Debug Functions
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Local Functions
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Global Functions
//-------------------------------------------------------------------------------------------------
__u8 JPEG_MEMORY_init_mempool(void *pool, __u32 size)
{
	if (size < MIN_POOL_SIZE)
		return FALSE;

	__mem_avail__[0].next = NULL;
	__mem_avail__[0].len = 0;
	__mem_avail__[1].next = NULL;
	__mem_avail__[1].len = 0;

	if (pool == NULL) {
		pool = (void *)1;
		size--;
	}

	AVAIL.next = (struct __mem__ *)pool;
	AVAIL.len = size;

	(AVAIL.next)->next = NULL;
	(AVAIL.next)->len = size - HLEN;

	return TRUE;
}

void *JPEG_MEMORY_malloc(__u32 size)
{
	__memp__ q;
	__memp__ p;
	__u32 k;

	q = &AVAIL;

	while (1) {
		p = q->next;
		if (p == NULL)
			return NULL;

		if (p->len >= size)
			break;

		q = p;
	}

	k = p->len - size;

	if (k < MIN_BLOCK) {
		q->next = p->next;
		return &p[1];
	}

	k -= HLEN;
	p->len = k;

	q = (__memp__) ((uintptr_t) ((__u8 *) (&p[1])) + k);
	q->len = size;
	q->next = NULL;

	return &q[1];
}

void JPEG_MEMORY_free(void *memp)
{
	__memp__ q;
	__memp__ p;
	__memp__ p0;

	if ((memp == NULL) || (AVAIL.len == 0))
		return;

	p0 = (__memp__) memp;
	p0 = &p0[-1];

	q = &AVAIL;

	while (1) {
		p = q->next;

		if ((p == NULL) || (p > (__memp__) memp))
			break;

		q = p;
	}

	if ((p != NULL) && ((((__u8 *) memp) + p0->len) == (__u8 *) p)) {
		p0->len += p->len + HLEN;
		p0->next = p->next;
	} else
		p0->next = p;

	if ((((__u8 *) q) + q->len + HLEN) == (__u8 *) p0) {
		q->len += p0->len + HLEN;
		q->next = p0->next;
	} else
		q->next = p0;
}
