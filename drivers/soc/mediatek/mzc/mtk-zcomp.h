/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2018 MediaTek Inc.
 */

#ifndef _MTK_ZCOMP_H_
#define _MTK_ZCOMP_H_

struct mtk_zcomp_strm {
	/* compression/decompression buffer */
	void *buffer;
	struct crypto_comp *tfm;
};

/* dynamic per-device compression frontend */
struct mtk_zcomp {
	struct mtk_zcomp_strm * __percpu *stream;
	const char *name;
	struct hlist_node node;
};

int mtk_zcomp_cpu_up_prepare(unsigned int cpu, struct hlist_node *node);
int mtk_zcomp_cpu_dead(unsigned int cpu, struct hlist_node *node);
ssize_t mtk_zcomp_available_show(const char *comp, char *buf);
bool mtk_zcomp_available_algorithm(const char *comp);

struct mtk_zcomp *mtk_zcomp_create(const char *comp);
void mtk_zcomp_destroy(struct mtk_zcomp *comp);

struct mtk_zcomp_strm *mtk_zcomp_stream_get(struct mtk_zcomp *comp);
void mtk_zcomp_stream_put(struct mtk_zcomp *comp);

int mtk_zcomp_compress(struct mtk_zcomp_strm *zstrm,
		const void *src, unsigned int *dst_len);

int mtk_zcomp_decompress(struct mtk_zcomp_strm *zstrm,
		const void *src, unsigned int src_len, void *dst);

bool mtk_zcomp_set_max_streams(struct mtk_zcomp *comp, int num_strm);
#endif /* _ZCOMP_H_ */
