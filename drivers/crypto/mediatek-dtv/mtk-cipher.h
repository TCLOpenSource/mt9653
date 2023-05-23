/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef MTK_CIPHER_H
#define MTK_CIPHER_H

#include <linux/version.h>

struct cryptodev_result {
	struct completion completion;
	int err;
};

#include <linux/string.h>
#include <crypto/mtk-cryptodev.h>

#define TS_PID_MAXNUM (16)
struct mtk_cipher_data_ctx {
	u32 dataformat;
	u32 pidnum;
	uint16_t *pid;
};

#define CIPHER_KEYTYPE_EVEN     (0)
#define CIPHER_KEYTYPE_ODD      (0x1)
#define CIPHER_KEYTYPE_TWOKEY   (0x2)
#define CIPHER_KEYTYPE_KEYSLOT  (0x4)
struct mtk_cipher_key_ctx {
	u32 keytype;		/* for mtk_cipher_data_ctx */
	u32 oddkeyindex;
	u32 evenkeyindex;
	u32 keylen;
	u32 key[12];
};

struct mtk_cipher_addr_ctx {
	unsigned long long src;
	unsigned long long dst;
	unsigned long len;
};

struct mtk_cipher_sample_ctx {
	uint32_t cryptolen;
	uint32_t skippedlen;
	uint32_t skippedoffset;
};

struct mtk_cipher_ctx {
	u32 reqsize;
	u32 ivsize;
	u32 keysize;
	u32 cipher;
	u32 parser;
	u32 sample;
	uint8_t iv[EALG_MAX_BLOCK_LEN];
	struct mtk_cipher_key_ctx ckey;
	struct mtk_cipher_addr_ctx addr;
	struct mtk_cipher_data_ctx tsdata;
	struct mtk_cipher_sample_ctx sampleinfo;
};

struct cipher_data {
	int init;		/* 0 uninitialized */
	int blocksize;
	int aead;
	int stream;
	int ivsize;
	int alignmask;
	int cavenderid;
	struct {
		struct mtk_cipher_ctx *base;

		struct cryptodev_result result;
		uint8_t iv[EALG_MAX_BLOCK_LEN];
	} async;
};

#endif
