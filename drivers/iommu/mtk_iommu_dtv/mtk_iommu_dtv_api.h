/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */

#ifndef _MTK_IOMMU_DTV_KERNEL_API_H_
#define _MTK_IOMMU_DTV_KERNEL_API_H_

#include <uapi/linux/mtk_iommu_dtv_api.h>
#include <linux/dma-buf.h>

#define IOMMUMD_FLAG_CMA0	(1 << 14)
#define IOMMUMD_FLAG_CMA1	(1 << 15)


/*
 *-----------------------------------------------------------------
 * mtkd_iommu_query_buftag
 * check buf tag support or not
 * @param  buf_tag                \b IN: buf tag id
 * @return 0 : succeed
 *        -1 : not support
 *-----------------------------------------------------------------
 */
int mtkd_iommu_query_buftag(unsigned int buf_tag, unsigned int *max_size);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_query_id
 * check buf tag id and max_size
 * @param  buf_tag                \b IN: buf tag string
 * @param  id                     \b OUT: buf tag id
 * @param  max_size               \b OUT: buf tag max_size
 * @return 0 : succeed
 *        -1 : not support
 *-----------------------------------------------------------------
 */
int mtkd_iommu_query_id(char *name, unsigned int *id, unsigned int *max_size);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_allocpipelineid
 * allocate pipeline id
 * @param  pipelineid                \b OUT: pipeline id
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_allocpipelineid(unsigned int *pipelineid);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_freepipelineid
 * free pipeline id
 * @param  pipelineid                \b IN: pipeline id
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_freepipelineid(unsigned int pipelineid);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_reserve_iova
 * reserve iova
 * @param  space_tag                 \b IN: space name
 * @param  buf_tag_array             \b IN: buf tag id
 * @param  addr                      \b OUT: reserve base address
 * @param  size                      \b IN: reserve size
 * @param  buf_tag_num               \b IN: number of buf tag id
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_reserve_iova(char *space_tag, unsigned int *buf_tag_array,
	unsigned long long *addr, unsigned int size, unsigned int buf_tag_num);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_free_iova
 * free reserve iova
 * @param  space_tag                 \b IN: space name
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_free_iova(char *space_tag);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_va2iova
 * allocate iova from va
 * @param  va                     \b IN: virtual address
 * @param  size                   \b IN: size
 * @param  addr                   \b OUT: iova address
 * @param  fd                     \b OUT: buffer fd
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_va2iova(unsigned long va, unsigned int size,
			unsigned long long *addr, int *fd);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_buffer_authorize
 * set buffer secure
 * @param  buf_tag                \b IN: virtual address
 * @param  size                   \b IN: size
 * @param  addr                   \b IN: buffer address
 * @param  pipeline_id            \b IN: pipeline id
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_buffer_authorize(unsigned int buf_tag,
	unsigned long long addr, size_t size, unsigned int pipeline_id);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_buffer_unauthorize
 * set buffer from seucre to non secure
 * @param  addr                   \b IN: buffer address
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_buffer_unauthorize(unsigned long long addr);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_getiova
 * get iova and size from fd
 * @param  fd                     \b IN: buffer fd
 * @param  addr                   \b OUT: iova address
 * @param  size                   \b OUT: size
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_getiova(int fd, unsigned long long *addr, unsigned int *size);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_getiova_kernel
 * get iova and size from dmabuf
 * @param  dmabuf                     \b IN: buffer dmabuf
 * @param  addr                   \b OUT: iova address
 * @param  size                   \b OUT: size
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_getiova_kernel(struct dma_buf *dmabuf, unsigned long long *addr, unsigned int *size);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_buffer_alloc
 * allocate iommu buffer
 * @param  flag                   \b IN: buf tag id and flags
 * @param  size                   \b IN: size
 * @param  fd                     \b OUT: buffer fd
 * @param  addr                   \b OUT: iova address
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_buffer_alloc(unsigned int flag, unsigned int size,
			unsigned long long *addr, int *fd);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_buffer_resize
 * resize iommu buffer
 * @param  fd                     \b IN: buffer fd
 * @param  size                   \b IN: new size
 * @param  addr                   \b OUT: iova address
 * @param  flag                   \b IN: buf tag id and flags
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_buffer_resize(int fd, unsigned int size,
			unsigned long long *addr, unsigned int flag);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_buffer_free
 * free iommu buffer
 * @param  fd                     \b IN: buffer fd
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_buffer_free(int fd);

/*
 *-----------------------------------------------------------------
 * mtkd_seal_getaidtype
 * get group AID type
 * @param  psealdata->pipelineID   \b IN: pipelineID(GAID)
 * @param  psealdata->enType       \b OUT: group AID type
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_seal_getaidtype(struct mdseal_ioc_data *psealdata);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_share
 * increase file count and get buffer unique id for sharing
 * @param  fd                     \b IN: buffer fd in process A
 * @param  db_id                  \b OUT: buffer unique id
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_share(int fd, u32 *db_id);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_getfd
 * take unique buf id and help install fd to process B
 * @param  fd                     \b OUT: buffer fd for process B
 * @param  len                    \b OUT: buffer size
 * @param  db_id                  \b IN: buffer unique id
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_getfd(int *fd, unsigned int *len, u32 db_id);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_delete
 * decrease file count to destroy share buffer
 * @param  db_id                  \b IN: buffer unique id
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_delete(u32 db_id);

/*
 *-----------------------------------------------------------------
 * mtkd_iommu_flush
 * flush va area
 * @param  va                  \b IN: buffer kernel or user virtual address
 * @param  size                \b IN: buffer size
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_iommu_flush(unsigned long va, unsigned int size);

/*
 *-----------------------------------------------------------------
 * mtkd_query_dmabuf_id_by_iova
 * flush va area
 * @param  iova              \b IN: buffer iova
 * @param  db_id             \b OUT: buffer's dmabuf_id
 * @param  buf_start         \b OUT: buffer's start addr
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mtkd_query_dmabuf_id_by_iova(unsigned long long iova,
		u32 *db_id, unsigned long long *buf_start);
#endif
