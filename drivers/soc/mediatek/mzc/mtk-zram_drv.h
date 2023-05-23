/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2018 MediaTek Inc.
 */

#ifndef _MTK_ZRAM_DRV_H_
#define _MTK_ZRAM_DRV_H_

#include <linux/rwsem.h>
#include "mtk-zsmalloc.h"
#include <linux/crypto.h>
#include "mtk-zcomp.h"

#define SECTORS_PER_PAGE_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define SECTORS_PER_PAGE	(1 << SECTORS_PER_PAGE_SHIFT)
#define ZRAM_LOGICAL_BLOCK_SHIFT 12
#define ZRAM_LOGICAL_BLOCK_SIZE	(1 << ZRAM_LOGICAL_BLOCK_SHIFT)
#define ZRAM_SECTOR_PER_LOGICAL_BLOCK	\
	(1 << (ZRAM_LOGICAL_BLOCK_SHIFT - SECTOR_SHIFT))


extern void *mtk_zs_map_object(struct mtk_zs_pool *pool, unsigned long handle,
			enum mtk_zs_mapmode mm, phys_addr_t *in_addr0, phys_addr_t *in_addr1,
			phys_addr_t *in_addr2, bool is_mzc);

extern void mtk_zs_unmap_object(struct mtk_zs_pool *pool, unsigned long handle,
			enum mtk_zs_mapmode mm, bool is_mzc);

extern unsigned long mtk_zs_get_total_pages(struct mtk_zs_pool *pool);
extern size_t mtk_zs_huge_class_size(struct mtk_zs_pool *pool);
extern unsigned long mtk_zs_malloc(struct mtk_zs_pool *pool,
		size_t size, gfp_t gfp);
extern void mtk_zs_destroy_pool(struct mtk_zs_pool *pool);
extern struct mtk_zs_pool *mtk_zs_create_pool(const char *name);
extern void mtk_zs_pool_stats(struct mtk_zs_pool *pool,
		struct mtk_zs_pool_stats *stats);
extern unsigned long mtk_zs_compact(struct mtk_zs_pool *pool);
extern void mtk_zs_free(struct mtk_zs_pool *pool, unsigned long handle);

extern bool MZC_ready;
extern int MTK_MZC_hybrid_compress(struct crypto_tfm *tfm, int *is_mzc,
				const u8 *src,
			    unsigned int slen,
			    u8 *v_dst, phys_addr_t in,
			    phys_addr_t dst,
			    unsigned int *dlen);

extern int MTK_MZC_hybrid_decompress(struct crypto_tfm *tfm, int *is_mzc,
				const phys_addr_t *src,
				unsigned int slen, phys_addr_t *dst,
				unsigned int *dlen, phys_addr_t out_addr,
				phys_addr_t addr0, phys_addr_t addr1,
				phys_addr_t addr2);

extern int MTK_MZC_compress(const u8 *src,
			    unsigned int slen, u8 *dst, unsigned int *dlen);


/*
 * The lower ZRAM_FLAG_SHIFT bits of table.flags is for
 * object size (excluding header), the higher bits is for
 * zram_pageflags.
 *
 * zram is mainly used for memory efficiency so we want to keep memory
 * footprint small so we can squeeze size and flags into a field.
 * The lower ZRAM_FLAG_SHIFT bits is for object size (excluding header),
 * the higher bits is for zram_pageflags.
 */
#define ZRAM_FLAG_SHIFT 24

/* Flags for zram pages (table[page_no].flags) */
enum zram_pageflags {
	/* zram slot is locked */
	ZRAM_LOCK = ZRAM_FLAG_SHIFT,
	ZRAM_SAME,	/* Page consists the same element */
	ZRAM_WB,	/* page is stored on backing_device */
	ZRAM_UNDER_WB,	/* page is under writeback */
	ZRAM_HUGE,	/* Incompressible page */
	ZRAM_IDLE,	/* not accessed page since last idle marking */
	ZRAM_IS_MZC,

	__NR_ZRAM_PAGEFLAGS,
};

/*-- Data structures */

/* Allocated for each disk page */
struct zram_table_entry {
	union {
		unsigned long handle;
		unsigned long element;
	};
	unsigned long flags;
#ifdef CONFIG_ZRAM_MEMORY_TRACKING
	ktime_t ac_time;
#endif
};

struct zram_stats {
	atomic64_t compr_data_size;	/* compressed size of pages stored */
	atomic64_t num_reads;	/* failed + successful */
	atomic64_t num_writes;	/* --do-- */
	atomic64_t failed_reads;	/* can happen when memory is too low */
	atomic64_t failed_writes;	/* can happen when memory is too low */
	atomic64_t invalid_io;	/* non-page-aligned I/O requests */
	atomic64_t notify_free;	/* no. of swap slot free notifications */
	atomic64_t same_pages;		/* no. of same element filled pages */
	atomic64_t huge_pages;		/* no. of huge pages */
	atomic64_t pages_stored;	/* no. of pages currently stored */
	atomic_long_t max_used_pages;	/* no. of maximum pages stored */
	atomic64_t writestall;		/* no. of write slow paths */
	atomic64_t miss_free;		/* no. of missed free */
#ifdef	CONFIG_ZRAM_WRITEBACK
	atomic64_t bd_count;		/* no. of pages in backing device */
	atomic64_t bd_reads;		/* no. of reads from backing device */
	atomic64_t bd_writes;		/* no. of writes from backing device */
#endif
};

struct zram {
	struct zram_table_entry *table;
	struct mtk_zs_pool *mem_pool;
	struct mtk_zcomp *comp;
	struct gendisk *disk;
	/* Prevent concurrent execution of device init */
	struct rw_semaphore init_lock;
	/*
	 * the number of pages zram can consume for storing compressed data
	 */
	unsigned long limit_pages;

	struct zram_stats stats;
	/*
	 * This is the limit on amount of *uncompressed* worth of data
	 * we can store in a disk.
	 */
	u64 disksize;	/* bytes */
	char compressor[CRYPTO_MAX_ALG_NAME];
	/*
	 * zram is claimed so open request will be failed
	 */
	bool claim; /* Protected by bdev->bd_mutex */
	struct file *backing_dev;
#ifdef CONFIG_ZRAM_WRITEBACK
	spinlock_t wb_limit_lock;
	bool wb_limit_enable;
	u64 bd_wb_limit;
	struct block_device *bdev;
	unsigned int old_block_size;
	unsigned long *bitmap;
	unsigned long nr_pages;
#endif
#ifdef CONFIG_ZRAM_MEMORY_TRACKING
	struct dentry *debugfs_dir;
#endif
};
#endif
