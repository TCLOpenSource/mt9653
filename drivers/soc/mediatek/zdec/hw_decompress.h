/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "mtk_zdec.h"

#define unzip_buf mtk_unzip_buf
#define req_hw mtk_req_hw

struct hw_capability {
		unsigned int comp_type;
		unsigned int hash_type;
		int min_size;           // (1 << min_size) compressed size support
		int max_size;           // (1 << max_size) compressed size support
};

static inline const struct hw_capability get_hw_capability(void)
{
	return (struct hw_capability) {

		.comp_type = HW_IOVEC_COMP_GZIP | HW_IOVEC_COMP_ZLIB,
		.hash_type = HW_IOVEC_HASH_SHA256,

		/* Don't care if input size is under 1<<17 (128KB) */
		.min_size = 1,
		.max_size = 17,
	};
}

static inline struct unzip_buf *unzip_alloc(size_t len)
{
	return mtk_unzip_alloc(len);
}

static inline void unzip_free(struct unzip_buf *buf)
{
	mtk_unzip_free(buf);
}

static inline int unzip_decompress_async(
	struct scatterlist *sg, unsigned int comp_bytes,
	struct page **opages, int npages, void **uzpriv,
	void (*cb)(int err, int decompressed, void *arg),
	void *arg, bool may_wait,
	enum mtk_comp_type comp_type, struct req_hw *rq_hw)
{
	struct hw_capability hw_cap = get_hw_capability();

	if (!(hw_cap.comp_type & comp_type))
		return -1;
	int shift_byte;

	if (comp_type == HW_IOVEC_COMP_ZLIB)
		shift_byte = 2;
	else
		shift_byte = 10;
	return mtk_init_decompress_async(sg,
									comp_bytes,
									opages,
									npages,
									uzpriv,
									cb,
									arg,
									may_wait,
									shift_byte,
									rq_hw);
}

static inline void unzip_update_endpointer(void *uzpriv)
{
	mtk_fire_decompress_async(uzpriv);
}

static inline int unzip_decompress_wait(void *uzpriv)
{
	int ret = mtk_zdec_wait(uzpriv);
	return ret;
}

static inline void calculate_hw_hash(unsigned long vstart, unsigned long pstart,
		 unsigned long length)
{
	/*wait for winnie to provide related API*/
}





