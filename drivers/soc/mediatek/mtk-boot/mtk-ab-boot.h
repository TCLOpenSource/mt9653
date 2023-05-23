/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
 *
 * Copyright (c) 2022 MediaTek Inc.
 */
#ifndef __MTK_AB_BOOT_H__
#define __MTK_AB_BOOT_H__

#include <linux/tee_drv.h>

#define TEE_PAYLOAD_NUM	4

#define TEE_BOOT_IOC			_IOWR('t', 202, uint32_t)
#define RETRY_COUNT_IOC			_IOWR('t', 203, uint32_t)

#define AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS 32
#define DTB_VERSION_STRUCT_MAGIC_LEN 16
struct uboot_avb_data {
	uint64_t rollback_index[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS];
	char magic[DTB_VERSION_STRUCT_MAGIC_LEN]; //magic = "avb_version "
}; //272 bytes

#endif				/* __MTK_TEE_H__ */
