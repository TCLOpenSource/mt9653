// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */





#include <linux/init.h>
#include <linux/module.h>
#include <linux/crypto.h>
#include "mtk-mzc.h"
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/lzo.h>
#include <linux/highmem.h>
#include <crypto/internal/scompress.h>


static int MZC_init(struct crypto_tfm *tfm)
{
	// lzo part.
	return 0;
}

static void MZC_exit(struct crypto_tfm *tfm)
{
	// lzo part
	// MZC part is a kernel module
}

int MTK_MZC_compress(const u8 *src,
			    unsigned int slen, u8 *dst, unsigned int *dlen)
{
	int err;
	err = mtk_dtv_lenc_cmdq_run(src, dst, dlen);
	return err;
}
EXPORT_SYMBOL(MTK_MZC_compress);

static int __MZC_decompress(struct crypto_tfm *tfm,
				const u8 *src, unsigned int slen,
				u8 *dst, unsigned int *dlen)
{
	return 0;
}

static int __MZC_compress(struct crypto_tfm *tfm,
				const u8 *src, unsigned int slen,
				u8 *dst, unsigned int *dlen)
{
	return 0;
}

static int MZC_compress(struct crypto_tfm *tfm, const u8 *src,
			    unsigned int slen, u8 *dst, unsigned int *dlen)
{
	return __MZC_compress(tfm, src, slen, dst, dlen);
}

static int MZC_decompress(struct crypto_tfm *tfm, const u8 *src,
			      unsigned int slen, u8 *dst, unsigned int *dlen)
{
	return __MZC_decompress(tfm, src, slen, dst, dlen);
}

static int MZC_sdecompress(struct crypto_scomp *tfm, const u8 *src,
			      unsigned int slen, u8 *dst,
			      unsigned int *dlen, void *ctx)
{
	return __MZC_decompress(&tfm->base, src, slen, dst, dlen);
}

static int MZC_scompress(struct crypto_scomp *tfm, const u8 *src,
			    unsigned int slen,
			    u8 *dst, unsigned int *dlen, void *ctx)
{
	return __MZC_compress(&tfm->base, src, slen, dst, dlen);
}

static void *MZC_alloc_ctx(struct crypto_scomp *tfm)
{
	return NULL;
}

static void MZC_free_ctx(struct crypto_scomp *tfm, void *ctx)
{
	;
}


static struct crypto_alg alg = {
	.cra_name		= "MZC",
	.cra_driver_name	= "MZC",
	.cra_flags		= CRYPTO_ALG_TYPE_COMPRESS,
	.cra_ctxsize		= 0,
	.cra_module		= THIS_MODULE,
	.cra_init		= MZC_init,
	.cra_exit		= MZC_exit,
	.cra_u			= { .compress = {
	.coa_compress	= MZC_compress,
	.coa_decompress	= MZC_decompress } }
};

static struct scomp_alg scomp = {
	.alloc_ctx		= MZC_alloc_ctx,
	.free_ctx		= MZC_free_ctx,
	.compress		= MZC_scompress,
	.decompress		= MZC_sdecompress,
	.base			= {
		.cra_name	= "MZC",
		.cra_driver_name = "mzc-scomp",
		.cra_module	 = THIS_MODULE,
	}
};

static int __init MZC_mod_init(void)
{
	return crypto_register_alg(&alg);
}

static void __exit MZC_mod_fini(void)
{
	crypto_unregister_alg(&alg);
}

module_init(MZC_mod_init);
module_exit(MZC_mod_fini);

MODULE_DESCRIPTION("MZC Compression Algorithm");
MODULE_ALIAS_CRYPTO("MZC");
MODULE_LICENSE("GPL");

