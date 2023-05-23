// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */





#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/crypto.h>
#include "mtk-mzc.h"
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/lzo.h>
#include <linux/highmem.h>
#include <crypto/internal/scompress.h>

struct hybrid_lzo_ctx {
	void *lzo_comp_mem;
};


static int MZC_hybrid_init(struct crypto_tfm *tfm)
{
	// lzo part.
	struct hybrid_lzo_ctx *ctx = crypto_tfm_ctx(tfm);

	ctx->lzo_comp_mem = kmalloc(LZO1X_MEM_COMPRESS,
				    GFP_KERNEL | __GFP_NOWARN);
	if (!ctx->lzo_comp_mem)
		ctx->lzo_comp_mem = vmalloc(LZO1X_MEM_COMPRESS);
	if (!ctx->lzo_comp_mem)
		WARN_ON(1);

	// MZC part is a kernel module

	return 0;
}

static void MZC_hybrid_exit(struct crypto_tfm *tfm)
{
	// lzo part
	struct hybrid_lzo_ctx *ctx = crypto_tfm_ctx(tfm);

	kvfree(ctx->lzo_comp_mem);

	// MZC part is a kernel module
}

static int __MZC_hybrid_compress(struct crypto_tfm *tfm, const u8 *src,
			    unsigned int slen, u8 *dst, unsigned int *dlen)
{
	return 0;
}

int MTK_MZC_hybrid_compress(struct crypto_tfm *tfm, int *is_mzc, const u8 *src,
			    unsigned int slen, u8 *v_dst,
			    phys_addr_t in, phys_addr_t dst,
			    unsigned int *dlen)
{
//src for lzo in for mzc
	int err = -1;
	struct hybrid_lzo_ctx *ctx = crypto_tfm_ctx(tfm);
	if (*is_mzc == INITIAL_VALUE) {
		err = mtk_dtv_lenc_hybrid_cmdq_run(in,
					dst, dlen);
		if (err == MZC_ERR_NO_ENC_RESOURCE) {
			size_t tmp_len = *dlen;
			unsigned long slen = PAGE_SIZE;
			err = lzo1x_1_compress(src,
				slen,
				v_dst,
				&tmp_len,
				ctx->lzo_comp_mem);
			if (err == LZO_E_OK)
				*dlen = tmp_len;
			*is_mzc = LZO;
		} else {
			*is_mzc = MZC;
		}
	} else {
		if (*is_mzc == LZO) {
			size_t tmp_len = *dlen;
			unsigned long slen = PAGE_SIZE;
			err = lzo1x_1_compress(src,
				slen,
				v_dst,
				&tmp_len,
				ctx->lzo_comp_mem);
			if (err == LZO_E_OK)
				*dlen = tmp_len;
		} else if (*is_mzc == MZC) {
			err = mtk_dtv_lenc_cmdq_run(in,
					dst, dlen);
		} else {
			WARN_ON(1);
		}
	}
	return err;
}
EXPORT_SYMBOL(MTK_MZC_hybrid_compress);

static int __MZC_hybrid_decompress(struct crypto_tfm *tfm,
				const u8 *src, unsigned int slen,
				u8 *dst, unsigned int *dlen)
{
	return 0;
}

int MTK_MZC_hybrid_decompress(struct crypto_tfm *tfm, int *is_mzc,
				const phys_addr_t *src,
				unsigned int slen, phys_addr_t *dst,
				unsigned int *dlen,
				phys_addr_t out_addr,
				phys_addr_t addr0,
				phys_addr_t addr1,
				phys_addr_t addr2)
{
	int err;
	if ((*is_mzc) == 0) {
		size_t tmp_len = *dlen;

		err = lzo1x_decompress_safe((u8 *)(src), slen, (u8 *)(dst), &tmp_len);
		if (err == LZO_E_OK)
			*dlen = tmp_len;
	} else {
		if (!addr0)
			err = mtk_dtv_ldec_cmdq_run_split(addr1, addr2, out_addr);
		else
			err = mtk_dtv_ldec_cmdq_run(addr0, out_addr);
	}
	return err;
}
EXPORT_SYMBOL(MTK_MZC_hybrid_decompress);

static int MZC_hybrid_compress(struct crypto_tfm *tfm, const u8 *src,
			    unsigned int slen, u8 *dst, unsigned int *dlen)
{
	return __MZC_hybrid_compress(tfm, src, slen, dst, dlen);
}

static int MZC_hybrid_decompress(struct crypto_tfm *tfm, const u8 *src,
			      unsigned int slen, u8 *dst, unsigned int *dlen)
{
	return __MZC_hybrid_decompress(tfm, src, slen, dst, dlen);
}

static int MZC_hybrid_sdecompress(struct crypto_scomp *tfm, const u8 *src,
			      unsigned int slen, u8 *dst,
			      unsigned int *dlen, void *ctx)
{
	return __MZC_hybrid_decompress(&tfm->base, src, slen, dst, dlen);
}

static int MZC_hybrid_scompress(struct crypto_scomp *tfm, const u8 *src,
			    unsigned int slen,
			    u8 *dst, unsigned int *dlen, void *ctx)
{
	return __MZC_hybrid_compress(&tfm->base, src, slen, dst, dlen);
}

static void *MZC_hybrid_alloc_ctx(struct crypto_scomp *tfm)
{
	return NULL;
}

static void MZC_hybrid_free_ctx(struct crypto_scomp *tfm, void *ctx)
{
	;
}


static struct crypto_alg alg = {
	.cra_name		= "mzc_hybrid",
	.cra_driver_name	= "mzc_hybrid",
	.cra_flags		= CRYPTO_ALG_TYPE_COMPRESS,
	.cra_ctxsize		= sizeof(struct hybrid_lzo_ctx),
	.cra_module		= THIS_MODULE,
	.cra_init		= MZC_hybrid_init,
	.cra_exit		= MZC_hybrid_exit,
	.cra_u			= { .compress = {
	.coa_compress	= MZC_hybrid_compress,
	.coa_decompress	= MZC_hybrid_decompress } }
};

static struct scomp_alg scomp = {
	.alloc_ctx		= MZC_hybrid_alloc_ctx,
	.free_ctx		= MZC_hybrid_free_ctx,
	.compress		= MZC_hybrid_scompress,
	.decompress		= MZC_hybrid_sdecompress,
	.base			= {
		.cra_name	= "mzc_hybrid",
		.cra_driver_name = "mzc-scomp",
		.cra_module	 = THIS_MODULE,
	}
};

static int __init MZC_hybrid_mod_init(void)
{
	return crypto_register_alg(&alg);
}

static void __exit MZC_hybrid_mod_fini(void)
{
	crypto_unregister_alg(&alg);
}

module_init(MZC_hybrid_mod_init);
module_exit(MZC_hybrid_mod_fini);

MODULE_DESCRIPTION("mzc_hybrid Compression Algorithm");
MODULE_ALIAS_CRYPTO("mzc_hybrid");
MODULE_LICENSE("GPL");

