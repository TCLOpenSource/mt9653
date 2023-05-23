/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef MTK_CRYPTODEV_H
#define MTK_CRYPTODEV_H

#include <linux/types.h>
#include <linux/version.h>
#ifndef __KERNEL__
#define __user
#endif

/* API extensions for linux */
#define CRYPTO_HMAC_MAX_KEY_LEN		512
#define CRYPTO_CIPHER_MAX_KEY_LEN	64

/* All the supported algorithms
 */

enum mtk_cipher_cavender_id_t {
	CA_VENDOR_ID_INVALID = 0,
	CA_VENDOR_ID_NAGRA = 2,
	CA_VENDOR_ID_DEFAULT = 16,
	CA_VENDOR_ID_MAX,
};

enum mtk_cipher_keysource_t {
	KEYSOURCE_SW,
	KEYSOURCE_KSLT, /* KL */
	KEYSOURCE_MAX,
};

enum mtk_cipher_dataformat_t {
	DATA_FORMAT_NONE,
	DATA_FORMAT_TS_PKT188,
	DATA_FORMAT_TS_PKT188_CLEAR,
	DATA_FORMAT_TS_PKT192,
	DATA_FORMAT_TS_PKT192_CLEAR,
	DATA_FORMAT_DASH, // MPEG_DASH
	DATA_FORMAT_MAX,
};

enum cryptodev_crypto_op_t {
	CRYPTO_DES_CBC = 1,
	CRYPTO_3DES_CBC = 2,
	CRYPTO_AES_CBC = 11,
	CRYPTO_AES_CBCS = 12,
	CRYPTO_NULL = 16,
	CRYPTO_AES_CTR = 21,
	CRYPTO_AES_CENS = 22,
	CRYPTO_AES_XTS = 22,
	CRYPTO_AES_ECB = 23,
	CRYPTO_DES_ECB = 51,
	CRYPTO_TDES_ECB = 52,
	CRYPTO_TDES_CBC = 53,
	CRYPTO_TDES_CTR = 54,

	CRYPTO_ALGORITHM_ALL,	/* Keep updated - see below */
};

#define	CRYPTO_ALGORITHM_MAX	(CRYPTO_ALGORITHM_ALL - 1)

/* Values for ciphers */
#define DES_BLOCK_LEN		8
#define DES3_BLOCK_LEN		8
#define AES_BLOCK_LEN		RIJNDAEL128_BLOCK_LEN

/* the maximum of the above */
#define EALG_MAX_BLOCK_LEN	16

/* Values for hashes/MAC */
#define AALG_MAX_RESULT_LEN		64

/* maximum length of verbose alg names (depends on CRYPTO_MAX_ALG_NAME) */
#define CRYPTODEV_MAX_ALG_NAME		64

#define HASH_MAX_LEN 64

/* input of CIOCGSESSION */
struct session_op {
	/* Specify either cipher or mac
	 */
	__u32 cavenderid;	/* mtk_cipher_cavender_id_t */
	__u32 cipher;		/* cryptodev_crypto_op_t */
	__u32 mac;		/* cryptodev_crypto_op_t */
	__u32 dataformat;	/* mtk_cipher_dataformat_t */

	struct data_info {
	__u32 cryptolen;
	__u32 skippedlen;
	__u32 skippedoffset;
	} sample_info;

	__u32 keysource; /* mtk_cipher_keysource_t */
	__u32 oddkeyindex;
	__u32 evenkeyindex;

	__u32 keylen;
	__u8 __user *key;
	__u32 mackeylen;
	__u8 __user *mackey;

	__u32 pidnum;
	__u16 __user *pid;
	__u32 keytype;

	__u32 ses;		/* session identifier */
};

struct session_info_op {
	__u32 ses;		/* session identifier */

	/* verbose names for the requested ciphers */
	struct alg_info {
		char cra_name[CRYPTODEV_MAX_ALG_NAME];
		char cra_driver_name[CRYPTODEV_MAX_ALG_NAME];
	} cipher_info, hash_info;

	__u16 alignmask;	/* alignment constraints */
	__u32 flags;		/* SIOP_FLAGS_* */
};

/* If this flag is set then this algorithm uses
 * a driver only available in kernel (software drivers,
 * or drivers based on instruction sets do not set this flag).
 *
 * If multiple algorithms are involved (as in AEAD case), then
 * if one of them is kernel-driver-only this flag will be set.
 */
#define SIOP_FLAG_KERNEL_DRIVER_ONLY 1

#define	COP_ENCRYPT	0
#define COP_DECRYPT	1

/* input of CIOCCRYPT */
struct crypt_op {
	__u32 srcfd;		/* fd identifier */
	__u32 dstfd;		/* fd identifier */
	__u32 ses;		/* session identifier */
	__u16 op;		/* COP_ENCRYPT or COP_DECRYPT */
	__u16 flags;		/* see COP_FLAG_* */
	__u32 len;		/* length of source data */
	/* pointer to output data for hash/MAC operations */
	__u8 __user *mac;
	/* initialization vector for encryption operations */
	__u8 __user *iv;
};

/* struct crypt_op flags */

#define COP_FLAG_NONE		(0 << 0)	/* totally no flag */
#define COP_FLAG_UPDATE		(1 << 0)	/* multi-update hash mode */
#define COP_FLAG_FINAL		(1 << 1)	/* multi-update final hash mode */
#define COP_FLAG_WRITE_IV	(1 << 2)	/* update the IV during operation */
#define COP_FLAG_RESET		(1 << 6)	/* multi-update reset the state. */
						 /* should be used in combination */
						 /* with COP_FLAG_UPDATE */


/* ioctl's. Compatible with old linux cryptodev.h
 */
#define CRIOGET         _IOWR('c', 101, __u32)
#define CIOCGSESSION    _IOWR('c', 102, struct session_op)
#define CIOCFSESSION    _IOW('c', 103, __u32)
#define CIOCCRYPT       _IOWR('c', 104, struct crypt_op)
#define CIOCKEY         _IOWR('c', 105, struct crypt_kop)
#define CIOCASYMFEAT    _IOR('c', 106, __u32)
#define CIOCGSESSINFO	_IOWR('c', 107, struct session_info_op)

/* to indicate that CRIOGET is not required in linux
 */
#define CRIOGET_NOT_NEEDED 1

#endif				/* MTK_CRYPTODEV_H */
