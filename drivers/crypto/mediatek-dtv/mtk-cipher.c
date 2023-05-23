// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/ioctl.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <linux/uaccess.h>
#include <crypto/algapi.h>
#include <crypto/hash.h>
#include <crypto/mtk-cryptodev.h>
#include <crypto/aead.h>
#include <linux/rtnetlink.h>
#include <crypto/authenc.h>
#include "mtk-dtv-platform.h"
#include <crypto/aes.h>
#include <crypto/des.h>
#include <linux/iommu.h>
#include "mtk_iommu_dtv_api.h"
#include <linux/delay.h>
#include <linux/bits.h>

/* AES flags */
#define AES_FLAGS_ECB		BIT(8)
#define AES_FLAGS_CBC		BIT(13)
#define AES_FLAGS_CTR		BIT(12)
#define AES_FLAGS_ENCRYPT	BIT(8)
#define AES_FLAGS_DECRYPT	BIT(9)
#define DES_FLAGS_ECB		BIT(2)
#define TDES_FLAGS_ECB		BIT(3)
#define AES_SELKEY_256		BIT(7)
#define AES_CBCS_CENC_MODE  BIT(0)

#define CIPHER_REGIV     (0x68)
#define CIPHER_REGCTRL   (0x50)
#define CIPHER_REGCTRL4  (0x5D)
#define CIPHER_REGDMACTRL (0x5E)
#define CIPHER_REGCTRL2  (0x5F)
#define CIPHER_REGSTATUS (0x7F)
#define CIPHER_REG_CBCS_CENC_CTRL  (0x18)
#define CIPHER_REG_CBCS_CENC_CTRL2 (0x19)

#define CIPHER_FLAGS_SECRETKEY BIT(12)
#define CIPHER_FLAGS_SHAFROMIN BIT(8)
#define CIPHER_FLAGS_FILEOUT BIT(8)
#define CIPHER_FLAGS_FILEIN  BIT(0)
#define CIPHER_FLAGS_BUSY	BIT(0)

#define CIPHER_TIMEOUT (0x100000)
#define CIPHER_FLUSHTIME (300)

#define CIPHER_RIUBYTENUM (2)
#define CIPHER_XIUBYTENUM (4)
#define CIPHER_RIUHEX2CPU_SHIFT (2)
#define CIPHER_H2SHIFTL2BYTE    (16)
#define CIPHER_REGKEY    (0x60)
#define CIPHER_REGALGO   (0x51)
#define CIPHER_REGSRC_L  (0x52)
#define CIPHER_REGSRC_H  (0x53)
#define CIPHER_REGSIZE_L (0x54)
#define CIPHER_REGSIZE_H (0x55)
#define CIPHER_REGDST_L  (0x56)
#define CIPHER_REGDST_H  (0x57)
#define CIPHER_REGSA_L   (0x58)
#define CIPHER_REGSA_H   (0x59)
#define CIPHER_REGADDR_EXT (0x16)
#define CIPHER_FIXSA_EXT    (0xF000)
#define CIPHER_SRC_EXTSHIFT (32)
#define CIPHER_DST_EXTSHIFT (24)
#define CIPHER_ADDR_EXT_MASK (0xF00000000ULL)

#define CIPHER_KEYINDEX_MASK (0xFF)
#define CIPHER_HIGHBYTE_SHIFTBIT (8)
#define CIPHER_HIGH2BYTE_SHIFTBIT (16)
#define CIPHER_HIGH3BYTE_SHIFTBIT (24)

/* TS mode */
#define TS_MODE             BIT(15)
#define TS_SCRMB_MASK       BIT(11)
#define TS_SCRMB_EVEN_PATTERN     BIT(10)
#define TS_SCRMB_ODD_PATTERN    (BIT(10)|BIT(9))
#define TS_HW_PARSER_MODE   BIT(8)
#define TS_INSERT_SCRMB     BIT(7)
#define TS_REMOVE_SCRMB     BIT(6)
#define TS_CLEAR_MODE       BIT(5)
#define TS_INIT_TRUST       BIT(4)
#define TS_AUTO_MODE        BIT(2)
#define TS_PKT192_MODE      BIT(1)

#define ENABLE_SPECIFIC_HANDLE BIT(8)
#define TS_ENABLE_TWO_KEY   BIT(6)
#define TS_INSERT_SCRMB_EVEN_PATTERN      BIT(5)
#define TS_INSERT_SCRMB_ODD_PATTERN    (BIT(5)|BIT(4))
#define TS_BYPASS_PID_SCRMB BIT(3)

#define TS_RIUCTRL2 (0x3)
#define TS_RIUPID0  (0x1)
#define TS_RIUPID2  (0x20)
#define TS_PID01_NUM (2)
#define TS_PID_MASK (0x1FFF)

#define TS_CTRL2_HIGH2BYTE_SHIFBIT (16)

static struct mtk_cryp *cryp_dev;

int mtk_cipher_set_ts_pidinfo(struct cipher_data *cdata,
				  unsigned int dataformat, unsigned int pidnum,
				  unsigned short int *pid, unsigned int keytype)
{
	unsigned short int *tspid;

	tspid = kcalloc(pidnum, sizeof(unsigned short int), GFP_KERNEL);
	if (!tspid)
		return -ENOMEM;

	memcpy(tspid, pid, pidnum * sizeof(unsigned short int));

	(cdata->async.base)->tsdata.dataformat = dataformat;
	(cdata->async.base)->tsdata.pidnum = pidnum;
	(cdata->async.base)->tsdata.pid = tspid;
	(cdata->async.base)->ckey.keytype |= keytype;
	dev_info(cryp_dev->dev, "%s: done", __func__);
	return 0;
}

static void mtk_cipher_aes_inverse(u8 *invkey, u8 *key, unsigned int keylen)
{
	int i;

	for (i = 0; i < keylen; i++)
		*(invkey + i) = *(key + keylen - 1 - i);
}

static int mtk_cipher_set_crypt_addr(struct cipher_data *cdata, struct crypt_op *cop)
{
	struct mtk_cipher_addr_ctx *addr = &((cdata->async.base)->addr);
	int ret;
	unsigned int bufsize;

	dev_info(cryp_dev->dev, "(cdata->async.base)->addr  = 0x%lx",
		 *(unsigned long *)&((cdata->async.base)->addr));

	if (cop->srcfd == 0) {
		dev_err(cryp_dev->dev, "no input source!\n");
		return -EINVAL;
	}
	// TODO mpu

	ret = mtkd_iommu_getiova(cop->srcfd, (unsigned long long *)(&(addr->src)), &bufsize);
	if (ret != 0) {
		dev_err(cryp_dev->dev, "src iova = 0x%llx, bufsize = 0x%x",
			(unsigned long long)(addr->src), bufsize);
		return -EFAULT;
	}
	if (bufsize < cop->len) {
		dev_err(cryp_dev->dev, "bufsize < data length");
		return -EINVAL;
	}
	dev_info(cryp_dev->dev, "src iova = 0x%lx, bufsize = 0x%x", (addr->src), bufsize);

	if (cop->dstfd == 0)
		addr->dst = addr->src;
	else {
		ret = mtkd_iommu_getiova(cop->dstfd,
			(unsigned long long *)(&(addr->dst)), &bufsize);
		if (ret != 0) {
			dev_err(cryp_dev->dev, "dst iova = 0x%llx, bufsize = 0x%x",
				(unsigned long long)(addr->dst), bufsize);
			return -EFAULT;
		}
		if (bufsize < cop->len) {
			dev_err(cryp_dev->dev, "bufsize < data length");
			return -EINVAL;
		}
	}
	dev_info(cryp_dev->dev, "dst iova = 0x%llx, bufsize = 0x%x",
		(unsigned long long)(addr->dst), bufsize);
	addr->len = cop->len;

	dev_info(cryp_dev->dev, "addr->src = 0x%llx, addr->dst = 0x%llx, addr->len = 0x%lx\n",
		 (unsigned long long)addr->src, (unsigned long long)addr->dst, addr->len);
	return 0;
}

static void mtk_cipher_set_crypt_stream(struct cipher_data *cdata, struct crypt_op *cop)
{
	struct mtk_cipher_ctx *base = cdata->async.base;
	struct mtk_cipher_key_ctx *ckey = &((cdata->async.base)->ckey);
	struct mtk_cipher_data_ctx *tsdata = &((cdata->async.base)->tsdata);
	struct mtk_cipher_sample_ctx *sampleinfo = &((cdata->async.base)->sampleinfo);
	unsigned int parserctl1 = 0, parserctl2 = 0, smaplectl = 0;

	if ((cdata->stream) && (tsdata->dataformat != DATA_FORMAT_DASH)) {
		dev_info(cryp_dev->dev, "config parser ctl");
		if ((tsdata->dataformat == DATA_FORMAT_TS_PKT192)
			|| (tsdata->dataformat == DATA_FORMAT_TS_PKT192_CLEAR))
			parserctl1 |= TS_PKT192_MODE;

		if (cop->op == COP_ENCRYPT) {
			if ((tsdata->dataformat == DATA_FORMAT_TS_PKT188_CLEAR)
				|| (tsdata->dataformat == DATA_FORMAT_TS_PKT192_CLEAR)) {
				dev_info(cryp_dev->dev, "ts encrypt and clear mode");
				parserctl1 |= TS_CLEAR_MODE;
			}
		}

		if (cop->op == COP_DECRYPT) {
			parserctl1 |= (TS_REMOVE_SCRMB | TS_SCRMB_MASK);
		} else {
			parserctl1 |= (TS_INSERT_SCRMB | TS_SCRMB_MASK);
			if (((ckey->keytype)&CIPHER_KEYTYPE_ODD) == CIPHER_KEYTYPE_ODD) {
				dev_info(cryp_dev->dev, "CIPHER_KEYTYPE_ODD");
				parserctl1 |= TS_SCRMB_ODD_PATTERN;
				parserctl2 |= TS_INSERT_SCRMB_ODD_PATTERN;
			} else {
				dev_info(cryp_dev->dev, "CIPHER_KEYTYPE_EVEN");
				parserctl1 |= TS_SCRMB_EVEN_PATTERN;
				parserctl2 |= TS_INSERT_SCRMB_EVEN_PATTERN;
			}
		}
		parserctl2 |= (TS_ENABLE_TWO_KEY | ENABLE_SPECIFIC_HANDLE);
		parserctl1 |= (TS_AUTO_MODE | TS_INIT_TRUST | TS_HW_PARSER_MODE | TS_MODE);
		base->parser = (parserctl2 << TS_CTRL2_HIGH2BYTE_SHIFBIT) | parserctl1;
		dev_info(cryp_dev->dev, "parserctl1 = 0x%x, parserctl2 = 0x%x", parserctl1,
			 parserctl2);
		dev_info(cryp_dev->dev, "base->parser 0x%x\n", base->parser);
	} else if (tsdata->dataformat == DATA_FORMAT_DASH) {
		dev_info(cryp_dev->dev, "set sample info");
		smaplectl = (((uint8_t)(sampleinfo->skippedoffset)) << CIPHER_HIGHBYTE_SHIFTBIT);
		smaplectl |= (((uint8_t)(sampleinfo->cryptolen)) << CIPHER_HIGH2BYTE_SHIFTBIT);
		smaplectl |= (((uint8_t)(sampleinfo->skippedlen)) << CIPHER_HIGH3BYTE_SHIFTBIT);
		if (smaplectl != 0x0)
			smaplectl |= AES_CBCS_CENC_MODE;
	}
}

int mtk_cipher_set_crypt(struct cipher_data *cdata, struct crypt_op *cop)
{
	struct mtk_cipher_ctx *base = cdata->async.base;

	dev_info(cryp_dev->dev, "&base  = 0x%llx", (unsigned long long)base);
	if (mtk_cipher_set_crypt_addr(cdata, cop) != 0x0)
		return -EINVAL;

	mtk_cipher_aes_inverse(base->iv, cdata->async.iv, cdata->ivsize);

	if ((cdata->cavenderid != CA_VENDOR_ID_NAGRA)
		&& ((base->cipher & AES_FLAGS_CTR) == AES_FLAGS_CTR))
		return 0;

	if (cop->op == COP_DECRYPT)
		base->cipher |= AES_FLAGS_DECRYPT;

	mtk_cipher_set_crypt_stream(cdata, cop);

	return 0;
}

int mtk_cipher_aes_setkeyindex(struct cipher_data *cdata,
		uint32_t oddkeyindex, uint32_t evenkeyindex)
{
	struct mtk_cipher_key_ctx *ckey = &((cdata->async.base)->ckey);

	dev_info(cryp_dev->dev, "oddkeyindex 0x%x, evenkeyindex 0x%x", oddkeyindex, evenkeyindex);
	ckey->keytype = CIPHER_KEYTYPE_KEYSLOT;
	ckey->oddkeyindex = oddkeyindex;
	ckey->evenkeyindex = evenkeyindex;

	return 0;
}

int mtk_cipher_aes_setkey(struct mtk_cipher_ctx *ctx, const u8 *key, unsigned int keylen)
{
	struct mtk_cipher_key_ctx *ckey = &(ctx->ckey);
	int i;

	dev_info(cryp_dev->dev, "&ckey = 0x%llx", (unsigned long long)&(ctx->ckey));
	dev_info(cryp_dev->dev, "(ctx->ckey) = 0x%lx", *(unsigned long *)&(ctx->ckey));

	ckey->keylen = keylen;
	mtk_cipher_aes_inverse((u8 *)(ckey->key), (u8 *)key, keylen);

	dev_info(cryp_dev->dev, "keylen = 0x%x", ckey->keylen);
	dev_info(cryp_dev->dev, "copy form key:");
	for (i = 0; i < keylen; i++)
		dev_info(cryp_dev->dev, "0x%x ", (*(key + i)));
	dev_info(cryp_dev->dev, "key:");
	for (i = 0; i < keylen / CIPHER_XIUBYTENUM; i++)
		dev_info(cryp_dev->dev, "0x%x ", (*((ckey->key) + i)));

	return 0;
}

/* Was correct key length supplied? */
static int check_key_size(size_t keylen, const char *alg_name,
			  unsigned int min_keysize, unsigned int max_keysize)
{
	if (max_keysize > 0 && unlikely((keylen < min_keysize) || (keylen > max_keysize))) {
		dev_err(cryp_dev->dev, "Wrong keylen '%zu' for algorithm '%s'. Use %u to %u.",
			keylen, alg_name, min_keysize, max_keysize);
		return -EINVAL;
	}

	return 0;
}

struct mtk_cipher_ctx *mtk_cipher_alloc(const char *alg_name)
{
	struct mtk_cipher_ctx *ctx;

	// TODO check algo

	ctx = kzalloc(sizeof(struct mtk_cipher_ctx), GFP_KERNEL);
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(struct mtk_cipher_ctx));

	dev_info(cryp_dev->dev, "&ctx = 0x%llx", (unsigned long long)ctx);
	dev_info(cryp_dev->dev, "&ctx->ckey = 0x%llx", (unsigned long long)&(ctx->ckey));
	dev_info(cryp_dev->dev, "&ctx->addr = 0x%llx", (unsigned long long)&(ctx->addr));
	return ctx;
}

static void mtk_cipher_free_cipher(struct mtk_cipher_ctx *s)
{
	kzfree(s->tsdata.pid);
	kzfree(s);
}

static int mtk_cipher_aes_cap(struct cipher_data *out, const char *alg_name)
{
	struct mtk_cipher_ctx *ctx = out->async.base;
	char *pch;

	dev_info(cryp_dev->dev, "alg_name = %s", alg_name);
	pch = strstr(alg_name, "aes");
	if (pch == NULL) {
		goto cap_tdes;
	} else {
		ctx->keysize = AES_BLOCK_SIZE;
		ctx->cipher = AES_FLAGS_ECB;
		out->blocksize = AES_BLOCK_SIZE;
		goto cap_iv;
	}
cap_tdes:
	pch = strstr(alg_name, "tdes");
	if (pch == NULL) {
		goto cap_des;
	} else {
		ctx->keysize = DES3_EDE_KEY_SIZE;
		ctx->cipher = TDES_FLAGS_ECB;
		out->blocksize = DES3_EDE_BLOCK_SIZE;
		goto cap_iv;
	}
cap_des:
	pch = strstr(alg_name, "des");
	if (pch == NULL)
		return -EINVAL;

	ctx->keysize = DES_BLOCK_SIZE;
	ctx->cipher = DES_FLAGS_ECB;
	out->blocksize = DES_BLOCK_SIZE;

cap_iv:
	ctx->ivsize = AES_BLOCK_SIZE;

	pch = strstr(alg_name, "cbc"); // or "cbcs"
	if (pch != NULL) {
		ctx->cipher |= AES_FLAGS_CBC;
		goto cap_finddone;
	}

	pch = strstr(alg_name, "ctr");
	if (pch != NULL) {
		ctx->cipher |= AES_FLAGS_CTR;
		goto cap_finddone;
	}

	pch = strstr(alg_name, "cens");
	if (pch != NULL)
		ctx->cipher |= AES_FLAGS_CTR;

cap_finddone:
	out->ivsize = AES_BLOCK_SIZE;
	out->alignmask = AES_BLOCK_SIZE;
	ctx->reqsize = sizeof(u64);
	return 0;
}

int mtk_cipher_sample_parameters(struct cipher_data *out, uint32_t cryptolen,
			uint32_t skippedlen, uint32_t skippedoffset)
{
	struct mtk_cipher_ctx *ctx = out->async.base;

	ctx->sampleinfo.cryptolen = cryptolen;
	ctx->sampleinfo.skippedlen = skippedlen;
	ctx->sampleinfo.skippedoffset = skippedoffset;
	return 0;
}

int mtk_cipher_init(struct cipher_data *out, const char *alg_name,
			uint8_t *keyp, size_t keylen, int keysource, int stream)
{
	int ret;
	unsigned int min_keysize, max_keysize;
	unsigned int algo;
	struct mtk_cipher_ctx *ctx = out->async.base;

	out->async.base = mtk_cipher_alloc(alg_name);
	// TODO check key index

	ret = mtk_cipher_aes_cap(out, alg_name);
	if (unlikely(ret)) {
		dev_err(cryp_dev->dev, "Cap algo %s not support.", alg_name);
		return -EINVAL;
	}

	// key source is from key slot
	if ((keysource > 0) && (keysource < KEYSOURCE_MAX))
		goto cipher_init_finish;

	algo = (out->async.base)->cipher;
	if ((algo & AES_FLAGS_ECB) == AES_FLAGS_ECB) {
		min_keysize = AES_MIN_KEY_SIZE;
		max_keysize = AES_MAX_KEY_SIZE;
	} else if ((algo & DES_FLAGS_ECB) == DES_FLAGS_ECB) {
		min_keysize = DES_KEY_SIZE;
		max_keysize = DES_KEY_SIZE;
	} else {
		min_keysize = DES_KEY_SIZE;
		max_keysize = DES3_EDE_KEY_SIZE;
	}

	ret = check_key_size(keylen, alg_name, min_keysize, max_keysize);
	if (unlikely(ret))
		return -EINVAL;

	ret = mtk_cipher_aes_setkey(out->async.base, keyp, keylen);
	if (unlikely(ret)) {
		dev_err(cryp_dev->dev, "Setting key failed for %s-%zu.", alg_name, keylen);
		return -EINVAL;
	}

cipher_init_finish:
	init_completion(&out->async.result.completion);

	out->init = 1;
	out->stream = stream;
	return ret;
}

void mtk_cipher_deinit(struct cipher_data *cdata)
{
	mtk_cipher_free_cipher(cdata->async.base);
}

static inline u16 mtk_cipher_read(void __iomem *base, u32 offset)
{
	return readw(base + offset);
}

static inline void mtk_cipher_write(u16 val, void __iomem *base, u32 offset)
{
	writew(val, base + offset);
}

static void mtk_cipher_aes_clearsetting(void)
{
	dev_info(cryp_dev->dev, "Reset Key selection");
	mtk_cipher_write(0, cryp_dev->d5_base, CIPHER_REGCTRL4 << CIPHER_RIUHEX2CPU_SHIFT);
	mtk_cipher_write(0, cryp_dev->d7_base, 0x0);
}

static int mtk_cipher_aes_encrypt(struct mtk_cipher_ctx *ctx)
{
	struct mtk_cipher_addr_ctx *addr = &ctx->addr;
	struct mtk_cipher_key_ctx *key = &ctx->ckey;
	struct mtk_cipher_data_ctx *tsdata = &ctx->tsdata;
	unsigned long ext_addr = 0;
	unsigned short *pkey;
	unsigned short *piv;
	int i;
	unsigned short tmp;

	dev_info(cryp_dev->dev, "addr = 0x%lx, key = 0x%lx", (unsigned long)addr,
		 (unsigned long)key);
	dev_info(cryp_dev->dev, "ctx->addr = 0x%lx, ctx->ckey = 0x%lx",
		 *(unsigned long *)&(ctx->addr), *(unsigned long *)&(ctx->ckey));
	dev_info(cryp_dev->dev,
		"cryp_dev->d5_base 0x%llx\n", (unsigned long long)cryp_dev->d5_base);
	dev_info(cryp_dev->dev,
		"cryp_dev->d4_base 0x%llx\n", (unsigned long long)cryp_dev->d4_base);

	mtk_cipher_aes_clearsetting();

	// keytype for ts
	if (((key->keytype)&CIPHER_KEYTYPE_KEYSLOT) == CIPHER_KEYTYPE_KEYSLOT) {
		dev_info(cryp_dev->dev, "CIPHER_KEYTYPE_KEYSLOT");
		dev_info(cryp_dev->dev, "odd key index %d, even key index %d",
			(int)key->oddkeyindex, (int)key->evenkeyindex);
		mtk_cipher_write((((key->oddkeyindex)&CIPHER_KEYINDEX_MASK)
				<<CIPHER_HIGHBYTE_SHIFTBIT)
				|((key->evenkeyindex)&CIPHER_KEYINDEX_MASK), cryp_dev->d7_base, 0);
		goto cipher_setkey_done;
	}

	if (key->keytype == CIPHER_KEYTYPE_ODD) {
		dev_info(cryp_dev->dev, "CIPHER_KEYTYPE_ODD");
		pkey = (unsigned short *)key->key;
		for (i = 0; i < (key->keylen) / CIPHER_RIUBYTENUM; i++) {
			mtk_cipher_write(*(pkey + i), cryp_dev->d4_base,
					 (CIPHER_REGKEY + i) << CIPHER_RIUHEX2CPU_SHIFT);
			dev_info(cryp_dev->dev, "0x%x ", *(pkey + i));
		}
		mtk_cipher_write(((key->oddkeyindex) & CIPHER_KEYINDEX_MASK)
				<< CIPHER_HIGHBYTE_SHIFTBIT, cryp_dev->d7_base, 0);
		goto cipher_setkey_done;
	}
	// keyindex
	if (key->evenkeyindex) {
		mtk_cipher_write(key->evenkeyindex, cryp_dev->d7_base, 0);
	} else {
		// CIPHER_KEYTYPE_EVEN
		// set key
		dev_info(cryp_dev->dev, "KEYLEN: 0x%x\n", key->keylen);
		dev_info(cryp_dev->dev, "KEY:\n");
		pkey = (unsigned short *)key->key;
		for (i = 0; i < (key->keylen) / CIPHER_RIUBYTENUM; i++) {
			mtk_cipher_write(*(pkey + i), cryp_dev->d5_base,
					 (CIPHER_REGKEY + i) << CIPHER_RIUHEX2CPU_SHIFT);
			dev_info(cryp_dev->dev, "0x%x ", *(pkey + i));
		}
		tmp = mtk_cipher_read(cryp_dev->d5_base,
					CIPHER_REGCTRL4 << CIPHER_RIUHEX2CPU_SHIFT);
		if ((key->keylen) == AES_KEYSIZE_256) {
			dev_info(cryp_dev->dev, "AES_SELKEY_256");
			mtk_cipher_write(tmp | AES_SELKEY_256, cryp_dev->d5_base,
					 CIPHER_REGCTRL4 << CIPHER_RIUHEX2CPU_SHIFT);
		} else {
			dev_info(cryp_dev->dev, "AES_SELKEY_128");
			mtk_cipher_write(tmp & (~AES_SELKEY_256), cryp_dev->d5_base,
					 CIPHER_REGCTRL4 << CIPHER_RIUHEX2CPU_SHIFT);
		}
	}
cipher_setkey_done:
	// set algo
	mtk_cipher_write((u16) ctx->cipher, cryp_dev->d5_base,
			 CIPHER_REGALGO << CIPHER_RIUHEX2CPU_SHIFT);
	// cbcs or cens
	mtk_cipher_write((u16) ctx->sample, cryp_dev->d7_base,
			 CIPHER_REG_CBCS_CENC_CTRL << CIPHER_RIUHEX2CPU_SHIFT);
	mtk_cipher_write((u16) ((ctx->sample) >> CIPHER_H2SHIFTL2BYTE), cryp_dev->d7_base,
			 CIPHER_REG_CBCS_CENC_CTRL2 << CIPHER_RIUHEX2CPU_SHIFT);

	// addr
	mtk_cipher_write((u16) (addr->src), cryp_dev->d5_base,
			 CIPHER_REGSRC_L << CIPHER_RIUHEX2CPU_SHIFT);
	mtk_cipher_write((u16) ((addr->src) >> CIPHER_H2SHIFTL2BYTE), cryp_dev->d5_base,
			 CIPHER_REGSRC_H << CIPHER_RIUHEX2CPU_SHIFT);

	mtk_cipher_write((u16) (addr->len), cryp_dev->d5_base,
			 CIPHER_REGSIZE_L << CIPHER_RIUHEX2CPU_SHIFT);
	mtk_cipher_write((u16) ((addr->len) >> CIPHER_H2SHIFTL2BYTE), cryp_dev->d5_base,
			 CIPHER_REGSIZE_H << CIPHER_RIUHEX2CPU_SHIFT);

	mtk_cipher_write((u16) (addr->dst), cryp_dev->d5_base,
			 CIPHER_REGDST_L << CIPHER_RIUHEX2CPU_SHIFT);
	mtk_cipher_write((u16) ((addr->dst) >> CIPHER_H2SHIFTL2BYTE), cryp_dev->d5_base,
			 CIPHER_REGDST_H << CIPHER_RIUHEX2CPU_SHIFT);

	ext_addr =
		(((addr->src)&CIPHER_ADDR_EXT_MASK) >> CIPHER_SRC_EXTSHIFT) |
		(((addr->dst)&CIPHER_ADDR_EXT_MASK) >> CIPHER_DST_EXTSHIFT) | CIPHER_FIXSA_EXT;
	mtk_cipher_write(ext_addr, cryp_dev->d7_base,
			 CIPHER_REGADDR_EXT << CIPHER_RIUHEX2CPU_SHIFT);


	// set iv
	piv = (unsigned short *)ctx->iv;
	dev_info(cryp_dev->dev, "iv: ");
	for (i = 0; i < (ctx->ivsize) / CIPHER_RIUBYTENUM; i++) {
		dev_info(cryp_dev->dev, "0x%x ", (*(piv + i)));
		if (key->keytype == CIPHER_KEYTYPE_KEYSLOT) {
			if (key->oddkeyindex != 0x0) {
				mtk_cipher_write(*(piv + i), cryp_dev->d4_base,
						 (CIPHER_REGIV + i) << CIPHER_RIUHEX2CPU_SHIFT);
			}
			// default set to even iv
			mtk_cipher_write(*(piv + i), cryp_dev->d5_base,
					 (CIPHER_REGIV + i) << CIPHER_RIUHEX2CPU_SHIFT);
		} else if (key->keytype == CIPHER_KEYTYPE_ODD) {
			mtk_cipher_write(*(piv + i), cryp_dev->d4_base,
					 (CIPHER_REGIV + i) << CIPHER_RIUHEX2CPU_SHIFT);
		} else {
			mtk_cipher_write(*(piv + i), cryp_dev->d5_base,
					 (CIPHER_REGIV + i) << CIPHER_RIUHEX2CPU_SHIFT);
		}
	}

	if (ctx->parser) {
		int j;

		mtk_cipher_write((uint16_t) ctx->parser, cryp_dev->d4_base, 0x0);
		mtk_cipher_write((uint16_t) ((ctx->parser >> CIPHER_H2SHIFTL2BYTE)),
				 cryp_dev->d4_base, TS_RIUCTRL2 << CIPHER_RIUHEX2CPU_SHIFT);
		for (i = 0; i < TS_PID01_NUM | i < (tsdata->pidnum); i++) {
			dev_info(cryp_dev->dev, "pid%d 0x%x", i, *((tsdata->pid) + i));
			mtk_cipher_write(*((tsdata->pid) + i), cryp_dev->d4_base,
					 (TS_RIUPID0 + i) << CIPHER_RIUHEX2CPU_SHIFT);
		}
		for (j = 0; i < (tsdata->pidnum); i++, j++)
			mtk_cipher_write(*((tsdata->pid) + i), cryp_dev->d4_base,
					 (TS_RIUPID2 + j) << CIPHER_RIUHEX2CPU_SHIFT);
		for (; i < TS_PID_MAXNUM; i++, j++)
			mtk_cipher_write(TS_PID_MASK, cryp_dev->d4_base,
					 (TS_RIUPID2 + j) << CIPHER_RIUHEX2CPU_SHIFT);
	} else {
		mtk_cipher_write(0, cryp_dev->d4_base, 0x0);
		mtk_cipher_write(0, cryp_dev->d4_base, TS_RIUCTRL2 << CIPHER_RIUHEX2CPU_SHIFT);
	}

	// dma_sync_single_for_device(cryp_dev->dev, addr->src, PAGE_SIZE, DMA_BIDIRECTIONAL);
	// dma_sync_single_for_cpu(cryp_dev->dev, addr->src, PAGE_SIZE, DMA_BIDIRECTIONAL);

	// start
	mtk_cipher_write(CIPHER_FLAGS_FILEOUT, cryp_dev->d5_base,
			 CIPHER_REGCTRL << CIPHER_RIUHEX2CPU_SHIFT);
	mtk_cipher_write(CIPHER_FLAGS_FILEOUT | CIPHER_FLAGS_FILEIN, cryp_dev->d5_base,
			 CIPHER_REGCTRL << CIPHER_RIUHEX2CPU_SHIFT);

	// polling
	i = 0;
	while (!(mtk_cipher_read(cryp_dev->d5_base,
				CIPHER_REGSTATUS << CIPHER_RIUHEX2CPU_SHIFT) & CIPHER_FLAGS_BUSY)) {
		i++;
		if (i >= CIPHER_TIMEOUT) {
			dev_err(cryp_dev->dev, "timeout!\n");
			return -EFAULT;
		}
	}

	// TODO copy iv: COP_FLAG_WRITE_IV

	// dma_sync_single_for_device(cryp_dev->dev, addr->src, PAGE_SIZE, DMA_BIDIRECTIONAL);
	// dma_sync_single_for_cpu(cryp_dev->dev, addr->src, PAGE_SIZE, DMA_BIDIRECTIONAL);
	msleep(CIPHER_FLUSHTIME);

	return 0;
}

int mtk_cipher_encrypt(struct mtk_cipher_ctx *ctx)
{
	return mtk_cipher_aes_encrypt(ctx);
}

static void mtk_cipher_prepare_engine(void)
{
	u16 val;

	val = mtk_cipher_read(cryp_dev->d5_base, CIPHER_REGDMACTRL << CIPHER_RIUHEX2CPU_SHIFT);
	mtk_cipher_write(val & ~(CIPHER_FLAGS_SECRETKEY), cryp_dev->d5_base,
			 CIPHER_REGDMACTRL << CIPHER_RIUHEX2CPU_SHIFT);
	val = mtk_cipher_read(cryp_dev->d5_base, CIPHER_REGCTRL2 << CIPHER_RIUHEX2CPU_SHIFT);
	mtk_cipher_write(val & ~(CIPHER_FLAGS_SHAFROMIN), cryp_dev->d5_base,
			 CIPHER_REGCTRL2 << CIPHER_RIUHEX2CPU_SHIFT);
}

int mtk_cipher_register(struct mtk_cryp *cryp)
{
	cryp_dev = cryp;

	mtk_cipher_prepare_engine();
	return 0;
}
