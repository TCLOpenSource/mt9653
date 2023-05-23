/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __UAPI_MTK_IOMMU_DTV_API_H_
#define __UAPI_MTK_IOMMU_DTV_API_H_


#ifndef __KERNEL__
#include <stddef.h>
#endif
#include <linux/ioctl.h>
#include <linux/types.h>

#define MAX_NAME_SIZE	16
#define MAX_TAG_NUM		8

#define MDIOMMU_DEV_NAME "/dev/iommu_mtkd"
#define IOVA_START_ADDR (0x200000000ULL)
#define IOVA_END_ADDR   (0x400000000ULL)

/*allocate buffer without iova immediately
 *but iova will be allocated by delayed work
 */
#define IOMMUMD_FLAG_NOMAPIOVA	(1 << 16)

/* allocate buffer with double iova size */
#define IOMMUMD_FLAG_2XIOVA	(1 << 17)

/* zero buffer */
#define IOMMUMD_FLAG_ZEROPAGE	(1 << 18)

/* 1 DMA ZONE,0 HIGH/NORMAL/DMA ZONE;*/
#define IOMMUMD_FLAG_DMAZONE    (1 << 19)

struct mdiommu_ioc_data {
	unsigned int flag;
	unsigned long long addr;
	unsigned int len;
	int fd;
	unsigned int pipelineid;
	unsigned long long va;
	int status;
	unsigned int dmabuf_id;
};

struct mdiommu_reserve_iova {
	char space_tag[MAX_NAME_SIZE];
	unsigned int buf_tag_array[MAX_TAG_NUM];
	unsigned long long base_addr;	/* return start address */
	unsigned int size;	/* reserve size */
	unsigned int buf_tag_num;
};

struct mdiommu_buf_tag {
	char buf_tag[MAX_NAME_SIZE];
	unsigned int id;
	unsigned int max_size;	/* should not greater than 4G */
};

typedef enum {
	E_AID_TYPE_NS = 0,	/* 00 */
	E_AID_TYPE_SDC = 1,	/* 01 */
	E_AID_TYPE_S = 2,	/* 10 */
	E_AID_TYPE_CSP = 3,	/* 11 */
	E_AID_TYPE_NUM = 4,
} EN_AID_TYPE;

struct mdseal_ioc_data {
	unsigned int pipelineID;
	EN_AID_TYPE enType;
};
#define SEAL_AID_INDEX_MASK       (0x3F)

#define MDIOMMU_IOC_MAGIC  'M'
#define MDIOMMU_IOC_SUPPORT	\
		_IOWR(MDIOMMU_IOC_MAGIC, 0, int)
#define MDIOMMU_IOC_QUERY_BUFTAG	\
		_IOWR(MDIOMMU_IOC_MAGIC, 1, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_RESERVE_IOVA	\
		_IOWR(MDIOMMU_IOC_MAGIC, 2, struct mdiommu_reserve_iova)
#define MDIOMMU_IOC_FREE_IOVA	\
		_IOWR(MDIOMMU_IOC_MAGIC, 3, struct mdiommu_reserve_iova)
#define MDIOMMU_IOC_ALLOC	\
		_IOWR(MDIOMMU_IOC_MAGIC, 4, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_ALLOCPIPELINEID	\
		_IOWR(MDIOMMU_IOC_MAGIC, 5, unsigned int)
#define MDIOMMU_IOC_FREEPIPELINEID	\
		_IOWR(MDIOMMU_IOC_MAGIC, 6, unsigned int)
#define MDIOMMU_IOC_AUTHORIZE	\
		_IOWR(MDIOMMU_IOC_MAGIC, 7, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_UNAUTHORIZE	\
		_IOWR(MDIOMMU_IOC_MAGIC, 8, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_GETIOVA	\
		_IOWR(MDIOMMU_IOC_MAGIC, 9, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_RESIZE	\
		_IOWR(MDIOMMU_IOC_MAGIC, 10, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_FLUSH	\
		_IOWR(MDIOMMU_IOC_MAGIC, 11, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_VA2IOVA	\
		_IOWR(MDIOMMU_IOC_MAGIC, 12, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_TEST	\
		_IOWR(MDIOMMU_IOC_MAGIC, 13, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_SHARE	\
		_IOWR(MDIOMMU_IOC_MAGIC, 14, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_GETFD	\
		_IOWR(MDIOMMU_IOC_MAGIC, 15, struct mdiommu_ioc_data)
#define MDIOMMU_IOC_DELETE	\
		_IOW(MDIOMMU_IOC_MAGIC, 16, struct mdiommu_ioc_data)
#define MDSEAL_IOC_GETAIDTYPE	\
		_IOWR(MDIOMMU_IOC_MAGIC, 17, struct mdseal_ioc_data)
#define MDIOMMU_IOC_QUERY_BUFTAG_ID	\
		_IOWR(MDIOMMU_IOC_MAGIC, 18, struct mdiommu_buf_tag)
#define MDIOMMU_IOC_QUERY_ID		\
		_IOWR(MDIOMMU_IOC_MAGIC, 19, struct mdiommu_ioc_data)
#endif
