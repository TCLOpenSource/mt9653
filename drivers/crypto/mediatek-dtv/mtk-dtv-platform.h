/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef __MTK_PLATFORM_H_
#define __MTK_PLATFORM_H_

#include <crypto/algapi.h>
#include <crypto/internal/aead.h>
#include <crypto/internal/hash.h>
#include <crypto/scatterwalk.h>
#include <crypto/skcipher.h>
#include <linux/crypto.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/scatterlist.h>
#include <linux/miscdevice.h>
#include <crypto/mtk-cryptodev.h>

#define PFX "mtk-dtv-crypto: "
#define derr(format, a...) pr_err(PFX "%s[%u] (%s:%u): " format "\n", \
			       current->comm, current->pid, __func__, __LINE__, ##a)
#define dwarning(format, a...) pr_warn(PFX "%s[%u] (%s:%u): " format "\n", \
			       current->comm, current->pid, __func__, __LINE__, ##a)
#define dinfo(format, a...) pr_info(PFX "%s[%u] (%s:%u): " format "\n", \
			       current->comm, current->pid, __func__, __LINE__, ##a)
#define ddebug(format, a...) pr_debug(PFX "%s[%u] (%s:%u): " format "\n", \
			       current->comm, current->pid, __func__, __LINE__, ##a)


struct fcrypt {
	struct list_head list;
	struct mutex sem;
};

/* compatibility stuff */
#ifdef CONFIG_COMPAT
#include <linux/compat.h>

/* input of CIOCGSESSION */
struct compat_session_op {
	/* Specify either cipher or mac
	 */
	uint32_t cavenderid;	/* mtk_cipher_cavender_id_t */
	uint32_t cipher;	/* cryptodev_crypto_op_t */
	uint32_t mac;		/* cryptodev_crypto_op_t */
	uint32_t dataformat;

	struct compat_data_info	{
		uint32_t cryptolen;
		uint32_t skippedlen;
		uint32_t skippedoffset;
	} sample_info;

	uint32_t keysource;
	uint32_t oddkeyindex;
	uint32_t evenkeyindex;

	uint32_t keylen;
	compat_uptr_t key;	/* pointer to key data */
	uint32_t mackeylen;
	compat_uptr_t mackey;	/* pointer to mac key data */

	uint32_t pidnum;
	compat_uptr_t pid;
	uint32_t keytype;

	uint32_t ses;		/* session identifier */
};

/* input of CIOCCRYPT */
struct compat_crypt_op {
	uint32_t srcfd;		/* fd identifier */
	uint32_t dstfd;		/* fd identifier */
	uint32_t ses;		/* session identifier */
	uint16_t op;		/* COP_ENCRYPT or COP_DECRYPT */
	uint16_t flags;		/* see COP_FLAG_* */
	uint32_t len;		/* length of source data */
	compat_uptr_t mac;	/* pointer to output data for hash/MAC operations */
	compat_uptr_t iv;	/* initialization vector for encryption operations */
};

/* compat ioctls, defined for the above structs */
#define COMPAT_CIOCGSESSION    _IOWR('c', 102, struct compat_session_op)
#define COMPAT_CIOCCRYPT       _IOWR('c', 104, struct compat_crypt_op)
#define COMPAT_CIOCASYNCCRYPT  _IOW('c', 107, struct compat_crypt_op)
#define COMPAT_CIOCASYNCFETCH  _IOR('c', 108, struct compat_crypt_op)

#endif				/* CONFIG_COMPAT */

/* kernel-internal extension to struct crypt_op */
struct kernel_crypt_op {
	struct crypt_op cop;

	int ivlen;
	__u8 iv[EALG_MAX_BLOCK_LEN];

	int digestsize;
	uint8_t hash_output[AALG_MAX_RESULT_LEN];

	struct task_struct *task;
	struct mm_struct *mm;
};

#include "mtk-cipher.h"

/* other internal structs */
struct csession {
	struct list_head entry;
	struct mutex sem;
	struct cipher_data cdata;
	uint32_t sid;
	uint32_t alignmask;
};

struct csession *crypto_get_session_by_sid(struct fcrypt *fcr, uint32_t sid);

static inline void crypto_put_session(struct csession *ses_ptr)
{
	mutex_unlock(&ses_ptr->sem);
}

/**
 * struct mtk_cryp - Cryptographic device
 * @base:	pointer to mapped register I/O base
 * @dev:	pointer to device
 *
 * Structure storing cryptographic device information.
 */
struct mtk_cryp {
	void __iomem *d4_base;
	void __iomem *d5_base;
	void __iomem *d7_base;
	struct device *dev;
	struct miscdevice *misc_dev;
	struct mutex crypto_lock;
};

int mtk_cipher_sample_parameters(struct cipher_data *cdata,
				  uint32_t cryptolen, uint32_t skippedlen, uint32_t skippedoffset);
int mtk_cipher_aes_setkeyindex(struct cipher_data *cdata,
				  uint32_t oddkeyindex, uint32_t evenkeyindex);
int mtk_cipher_set_ts_pidinfo(struct cipher_data *cdata,
				  unsigned int dataformat, unsigned int pidnum,
				  unsigned short int *pid, unsigned int keytype);
int mtk_cipher_set_crypt(struct cipher_data *cdata, struct crypt_op *cop);
int mtk_cipher_aes_setkey(struct mtk_cipher_ctx *ctx, const u8 *key, unsigned int keylen);
int mtk_cipher_init(struct cipher_data *out, const char *alg_name,
			uint8_t *keyp, size_t keylen, int keycource, int stream);
void mtk_cipher_deinit(struct cipher_data *cdata);
int mtk_cipher_encrypt(struct mtk_cipher_ctx *ctx);
int mtk_cipher_register(struct mtk_cryp *cryp);
#endif				/* __MTK_PLATFORM_H_ */
