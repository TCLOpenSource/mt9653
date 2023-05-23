// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <crypto/hash.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include "mtk-dtv-platform.h"
#include <crypto/mtk-cryptodev.h>
#include <linux/of_address.h>
#include <linux/ioctl.h>
#include <linux/random.h>
#include <linux/syscalls.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <crypto/authenc.h>
#include <linux/sysctl.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/iommu.h>
#include "mtk-cipher.h"

/* ====== CryptoAPI ====== */

struct crypt_priv {
	struct fcrypt fcrypt;
};

static inline void cryptodev_cipher_get_iv(struct cipher_data *cdata, void *iv, size_t iv_size)
{
	memcpy(iv, cdata->async.iv, min(iv_size, sizeof(cdata->async.iv)));
}

static inline void cryptodev_cipher_set_iv(struct cipher_data *cdata, void *iv, size_t iv_size)
{
	memcpy(cdata->async.iv, iv, min(iv_size, sizeof(cdata->async.iv)));
}

static int crypto_run(struct fcrypt *fcr, struct kernel_crypt_op *kcop)
{
	struct csession *ses_ptr;
	struct crypt_op *cop = &kcop->cop;
	int ret = 0;

	if (unlikely(cop->op != COP_ENCRYPT && cop->op != COP_DECRYPT)) {
		ddebug("invalid operation op=%u", cop->op);
		return -EINVAL;
	}

	/* this also enters ses_ptr->sem */
	ses_ptr = crypto_get_session_by_sid(fcr, cop->ses);
	if (unlikely(!ses_ptr)) {
		derr("invalid session ID=0x%08X", cop->ses);
		return -EINVAL;
	}

	if (ses_ptr->cdata.init != 0) {
		int blocksize = ses_ptr->cdata.blocksize;

		if ((unlikely(cop->len % blocksize)) && (!ses_ptr->cdata.stream)) {
			derr("data size (%u) isn't a multiple of block size (%u)",
				 cop->len, blocksize);
			ret = -EINVAL;
			goto out_unlock;
		}

		cryptodev_cipher_set_iv(&ses_ptr->cdata, kcop->iv,
					min(ses_ptr->cdata.ivsize, kcop->ivlen));
	}

	if (likely(cop->len)) {
		ret = mtk_cipher_set_crypt(&ses_ptr->cdata, &kcop->cop);
		if (unlikely(ret))
			goto out_unlock;

		ret = mtk_cipher_encrypt(ses_ptr->cdata.async.base);
		if (unlikely(ret))
			goto out_unlock;
	}

	if (ses_ptr->cdata.init != 0) {
		cryptodev_cipher_get_iv(&ses_ptr->cdata, kcop->iv,
					min(ses_ptr->cdata.ivsize, kcop->ivlen));
	}

out_unlock:
	crypto_put_session(ses_ptr);
	return ret;
}

static int cryptodev_get_cipher_keylen(unsigned int *keylen, struct session_op *sop, int aead)
{
	unsigned int klen = sop->keylen;

	if (unlikely(sop->keylen > CRYPTO_CIPHER_MAX_KEY_LEN))
		return -EINVAL;

	*keylen = klen;
	return 0;
}

static int cryptodev_get_cipher_key(uint8_t *key, struct session_op *sop, int aead)
{
	/* now copy the blockcipher key */
	if (unlikely(copy_from_user(key, sop->key, sop->keylen)))
		return -EFAULT;

	return 0;
}

static int mtk_cipher_get_ts_pidnum(struct session_op *sop)
{
	if (unlikely(sop->pidnum > TS_PID_MAXNUM))
		return -EINVAL;
	return 0;
}

static int mtk_cipher_get_ts_pid(struct session_op *sop, unsigned short int *tspid)
{
	if (unlikely(copy_from_user(tspid, sop->pid, (sop->pidnum) * sizeof(uint16_t))))
		return -EFAULT;
	return 0;
}

static int mtk_crypto_cap(struct session_op *sop, char *alg_name, size_t namesize)
{
	char *algo;
	size_t algo_len;

	switch (sop->cipher) {
	case CRYPTO_DES_CBC:
		algo = "cbc(des)";
		break;
	case CRYPTO_DES_ECB:
		algo = "ecb(des)";
		break;
	case CRYPTO_AES_CBC:
		algo = "cbc(aes)";
		break;
	case CRYPTO_AES_CBCS:
		algo = "cbcs(aes)";
		break;
	case CRYPTO_AES_ECB:
		algo = "ecb(aes)";
		break;
	case CRYPTO_AES_CTR:
		algo = "ctr(aes)";
		break;
	case CRYPTO_AES_CENS:
		algo = "cens(aes)";
		break;
	case CRYPTO_TDES_ECB:
		algo = "ecb(tdes)";
		break;
	case CRYPTO_TDES_CBC:
		algo = "cbc(tdes)";
		break;
	case CRYPTO_TDES_CTR:
		algo = "ctr(tdes)";
		break;
	case CRYPTO_NULL:
		algo = "ecb(cipher_null)";
		break;
	default:
		ddebug("bad cipher: %d", sop->cipher);
		return -EINVAL;
	}
	algo_len = strlen(algo) + 1;
	if (algo_len > namesize)
		return -EINVAL;

	memcpy(alg_name, algo, algo_len);
	return 0;
}

static int mtk_cipher_stream_init(int stream, struct csession *ses_new, struct session_op *sop)
{
	int ret = 0;

	if (stream) {
		if (sop->dataformat == DATA_FORMAT_DASH) {
			((ses_new->cdata).async.base)->tsdata.dataformat = sop->dataformat;
		} else {
			ret = mtk_cipher_get_ts_pidnum(sop);
			if (unlikely(ret < 0)) {
				ddebug("PID number %d too much.", sop->pidnum);
				return -EINVAL;
			}

			uint16_t tspid[sop->pidnum];

			ret = mtk_cipher_get_ts_pid(sop, tspid);
			if (unlikely(ret < 0))
				return -EINVAL;

			ret =
				mtk_cipher_set_ts_pidinfo(&ses_new->cdata,
						sop->dataformat, sop->pidnum, tspid, sop->keytype);
			if (unlikely(ret < 0)) {
				ddebug("Failed to load ts pid.");
				return -EINVAL;
			}
		}
	}
	return 0;
}

static int cryptodev_specialhandle(struct csession *ses_new, struct session_op *sop)
{
	int ret = 0;

	if ((sop->cipher == CRYPTO_AES_CBCS) || (sop->cipher == CRYPTO_AES_CENS)) {
		ret = mtk_cipher_sample_parameters(
				&ses_new->cdata, sop->sample_info.cryptolen,
				sop->sample_info.skippedlen,
				sop->sample_info.skippedoffset);
		if (ret < 0) {
			ddebug("cryptolen = 0x%x, skippedlen = 0x%x, skippedoffset = 0x%x",
					sop->sample_info.cryptolen, sop->sample_info.skippedlen,
					sop->sample_info.skippedoffset);
			return -EINVAL;
		}
	}

	if (sop->keysource == KEYSOURCE_KSLT) {
		ddebug("oddkeyindex %x, evenkeyindex %x",
				sop->oddkeyindex, sop->evenkeyindex);
		ret = mtk_cipher_aes_setkeyindex(
				&ses_new->cdata, sop->oddkeyindex, sop->evenkeyindex);
		if (unlikely(ret < 0)) {
			ddebug("Fail to set key slot index");
			return -EINVAL;
		}
	}
	return 0;
}

static int crypto_create_session(struct fcrypt *fcr, struct session_op *sop)
{
#define CIPHER_ALGONAME_LEN	(0x20)
	struct csession *ses_new = NULL, *ses_ptr;
	int ret = 0;
	char *alg_name = NULL;
	const char *hash_name = NULL;
	int hmac_mode = 1, stream = 0, aead = 0;
	struct {
		uint8_t ckey[CRYPTO_CIPHER_MAX_KEY_LEN];
	} keys;

	/* Does the request make sense? */
	if (unlikely(!sop->cipher && !sop->mac)) {
		ddebug("Both 'cipher' and 'mac' unset.");
		return -EINVAL;
	}

	if ((sop->dataformat > 0) && (sop->dataformat < DATA_FORMAT_MAX)) {
		ddebug("Set data format.");
		stream = 1;
	}

	ddebug("sop.cipher %d, sop.keylen %d, sop.key 0x%lx\n", sop->cipher, sop->keylen,
		   (unsigned int)sop->key);
	alg_name = kzalloc(CIPHER_ALGONAME_LEN, GFP_KERNEL);
	if (!alg_name)
		return -ENOMEM;
	ret = mtk_crypto_cap(sop, alg_name, CIPHER_ALGONAME_LEN);
	if (unlikely(ret < 0))
		goto algo_error;

	/* Create a session and put it to the list. Zeroing the structure helps */
	/* also with a single exit point in case of errors */
	ses_new = kzalloc(sizeof(*ses_new), GFP_KERNEL);
	if (!ses_new)
		goto algo_error;

	/* Set-up crypto transform. */
	if (alg_name) {
		unsigned int keylen;
		int i;

		ret = cryptodev_get_cipher_keylen(&keylen, sop, aead);
		if (unlikely(ret < 0)) {
			ddebug("Setting key failed for %s-%zu.",
			alg_name, (size_t) sop->keylen);
			goto session_error;
		}

		if (sop->keysource == KEYSOURCE_SW) {
			ret = cryptodev_get_cipher_key(keys.ckey, sop, aead);
			if (unlikely(ret < 0))
				goto session_error;

			for (i = 0; i < keylen; i++)
				ddebug("keys.ckey[%d] = 0x%x\n", i, keys.ckey[i]);
		} else {
			memset(keys.ckey, 0x0, CRYPTO_CIPHER_MAX_KEY_LEN);
		}

		ret = mtk_cipher_init(&ses_new->cdata, alg_name,
				keys.ckey, keylen, sop->keysource, stream);
		if (ret < 0) {
			ddebug("Failed to load cipher for %s", alg_name);
			ret = -EINVAL;
			goto session_init_error;
		}

		ddebug("Set sample parameter for %s", alg_name);
		ret = cryptodev_specialhandle(ses_new, sop);
		if (unlikely(ret < 0))
			goto session_init_error;

		ret = mtk_cipher_stream_init(stream, ses_new, sop);
		if (unlikely(ret < 0))
			goto session_init_error;
	}

	ses_new->alignmask = ses_new->cdata.alignmask;
	ddebug("got alignmask %d", ses_new->alignmask);

	// vender
	ses_new->cdata.cavenderid = sop->cavenderid;
	ddebug("ca vender id %d", ses_new->cdata.cavenderid);

	/* put the new session to the list */
	get_random_bytes(&ses_new->sid, sizeof(ses_new->sid));
	mutex_init(&ses_new->sem);

	mutex_lock(&fcr->sem);
restart:
	list_for_each_entry(ses_ptr, &fcr->list, entry) {
		/* Check for duplicate SID */
		if (unlikely(ses_new->sid == ses_ptr->sid)) {
			get_random_bytes(&ses_new->sid, sizeof(ses_new->sid));
			/* Unless we have a broken RNG this */
			/* shouldn't loop forever... ;-) */
			goto restart;
		}
	}
	ddebug("creat sid: 0x%x", ses_new->sid);

	list_add(&ses_new->entry, &fcr->list);
	mutex_unlock(&fcr->sem);

	/* Fill in some values for the user. */
	sop->ses = ses_new->sid;
	ret = 0;
	goto algo_error;

session_init_error:
	mtk_cipher_deinit(&ses_new->cdata);
session_error:
	kfree(ses_new);
algo_error:
	kfree(alg_name);
	return ret;
}

/* Everything that needs to be done when removing a session. */
static inline void crypto_destroy_session(struct csession *ses_ptr)
{
	if (!mutex_trylock(&ses_ptr->sem)) {
		ddebug("Waiting for semaphore of sid=0x%08X", ses_ptr->sid);
		mutex_lock(&ses_ptr->sem);
	}
	ddebug("Removed session 0x%08X", ses_ptr->sid);
	mtk_cipher_deinit(&ses_ptr->cdata);
	mutex_unlock(&ses_ptr->sem);
	mutex_destroy(&ses_ptr->sem);
	kfree(ses_ptr);
}

/* Look up a session by ID and remove. */
static int crypto_finish_session(struct fcrypt *fcr, uint32_t sid)
{
	struct csession *tmp, *ses_ptr;
	struct list_head *head;
	int ret = 0;

	mutex_lock(&fcr->sem);
	head = &fcr->list;
	list_for_each_entry_safe(ses_ptr, tmp, head, entry) {
		if (ses_ptr->sid == sid) {
			list_del(&ses_ptr->entry);
			crypto_destroy_session(ses_ptr);
			break;
		}
	}

	if (unlikely(!ses_ptr)) {
		derr("Session with sid=0x%08X not found!", sid);
		ret = -ENOENT;
	}
	mutex_unlock(&fcr->sem);

	return ret;
}

/* Remove all sessions when closing the file */
static int crypto_finish_all_sessions(struct fcrypt *fcr)
{
	struct csession *tmp, *ses_ptr;
	struct list_head *head;

	mutex_lock(&fcr->sem);

	head = &fcr->list;
	list_for_each_entry_safe(ses_ptr, tmp, head, entry) {
		list_del(&ses_ptr->entry);
		crypto_destroy_session(ses_ptr);
	}
	mutex_unlock(&fcr->sem);

	return 0;
}

/* Look up session by session ID. The returned session is locked. */
struct csession *crypto_get_session_by_sid(struct fcrypt *fcr, uint32_t sid)
{
	struct csession *ses_ptr, *retval = NULL;

	ddebug("%s: sid = 0x%x\n", __func__, sid);

	if (unlikely(fcr == NULL))
		return NULL;

	mutex_lock(&fcr->sem);
	list_for_each_entry(ses_ptr, &fcr->list, entry) {
		if (ses_ptr->sid == sid) {
			mutex_lock(&ses_ptr->sem);
			retval = ses_ptr;
			break;
		}
	}
	mutex_unlock(&fcr->sem);

	return retval;
}

static int mtk_dtv_cryptodev_open(struct inode *inode, struct file *filp)
{
	struct crypt_priv *pcr;

	pcr = kzalloc(sizeof(*pcr), GFP_KERNEL);
	if (!pcr)
		return -ENOMEM;
	filp->private_data = pcr;

	mutex_init(&pcr->fcrypt.sem);
	INIT_LIST_HEAD(&pcr->fcrypt.list);
	ddebug("Cryptodev initialised");
	return 0;

/* In case of errors, free any memory allocated so far */
err_ringalloc:
	mutex_destroy(&pcr->fcrypt.sem);
	kfree(pcr);
	filp->private_data = NULL;
	return -ENOMEM;
}

static int mtk_dtv_cryptodev_release(struct inode *inode, struct file *filp)
{
	struct crypt_priv *pcr = filp->private_data;

	if (!pcr)
		return 0;

	crypto_finish_all_sessions(&pcr->fcrypt);

	mutex_destroy(&pcr->fcrypt.sem);

	kfree(pcr);
	filp->private_data = NULL;

	ddebug("Cryptodev handle deinitialised");
	return 0;
}

static int clonefd(struct file *filp)
{
	int ret;

	ret = get_unused_fd_flags(0);
	if (ret >= 0) {
		get_file(filp);
		fd_install(ret, filp);
	}
	return ret;
}

/* this function has to be called from process context */
static int fill_kcop_from_cop(struct kernel_crypt_op *kcop, struct fcrypt *fcr)
{
	struct crypt_op *cop = &kcop->cop;
	struct csession *ses_ptr;
	int rc;

	/* this also enters ses_ptr->sem */
	ses_ptr = crypto_get_session_by_sid(fcr, cop->ses);
	if (unlikely(!ses_ptr)) {
		derr("invalid session ID=0x%08X", cop->ses);
		return -EINVAL;
	}
	kcop->ivlen = cop->iv ? ses_ptr->cdata.ivsize : 0;
	kcop->digestsize = 0;	/* will be updated during operation */

	crypto_put_session(ses_ptr);

	if (cop->iv) {
		rc = copy_from_user(kcop->iv, cop->iv, kcop->ivlen);
		if (unlikely(rc)) {
			derr(
				 "error copying IV (%d bytes), copy_from_user returned %d for address %p",
				 kcop->ivlen, rc, cop->iv);
			return -EFAULT;
		}
	}

	return 0;
}

/* this function has to be called from process context */
static int fill_cop_from_kcop(struct kernel_crypt_op *kcop, struct fcrypt *fcr)
{
	int ret;

	if (kcop->ivlen && kcop->cop.flags & COP_FLAG_WRITE_IV) {
		ret = copy_to_user(kcop->cop.iv, kcop->iv, kcop->ivlen);
		if (unlikely(ret))
			return -EFAULT;
	}
	return 0;
}

static int kcop_from_user(struct kernel_crypt_op *kcop, struct fcrypt *fcr, void __user *arg)
{
	if (unlikely(copy_from_user(&kcop->cop, arg, sizeof(kcop->cop))))
		return -EFAULT;

	return fill_kcop_from_cop(kcop, fcr);
}

static int kcop_to_user(struct kernel_crypt_op *kcop, struct fcrypt *fcr, void __user *arg)
{
	int ret;

	ret = fill_cop_from_kcop(kcop, fcr);
	if (unlikely(ret)) {
		derr("Error in fill_cop_from_kcop");
		return ret;
	}

	if (unlikely(copy_to_user(arg, &kcop->cop, sizeof(kcop->cop)))) {
		derr("Cannot copy to userspace");
		return -EFAULT;
	}
	return 0;
}

static int get_session_info(struct fcrypt *fcr, struct session_info_op *siop)
{
	struct csession *ses_ptr;
	struct crypto_tfm *tfm;

	/* this also enters ses_ptr->sem */
	ses_ptr = crypto_get_session_by_sid(fcr, siop->ses);
	if (unlikely(!ses_ptr)) {
		derr("invalid session ID=0x%08X", siop->ses);
		return -EINVAL;
	}

	siop->flags = 0;
	siop->alignmask = ses_ptr->alignmask;
	crypto_put_session(ses_ptr);
	return 0;
}

static long mtk_dtv_cryptodev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg_)
{
	void __user *arg = (void __user *)arg_;
	struct session_op sop;
	struct kernel_crypt_op kcop;
	struct session_info_op siop;
	struct crypt_priv *pcr = filp->private_data;
	struct fcrypt *fcr;
	unsigned int ses;
	int ret = -EFAULT;

	if (arg == 0)
		return -EINVAL;

	fcr = &pcr->fcrypt;
	ddebug("arg = 0x%lx\n", (unsigned long)arg);
	ddebug("cmd = %d\n", (unsigned int)cmd);

	switch (cmd) {
	case CRIOGET:
		return ret;
	case CIOCGSESSION:
		if (copy_from_user(&sop, arg, sizeof(sop)))
			return -EFAULT;
		ddebug("sop.cipher %d, sop.keylen %d, sop.key 0x%lx\n",
			   sop.cipher, sop.keylen, (unsigned long)sop.key);
		ret = crypto_create_session(fcr, &sop);
		if (ret)
			return ret;
		ret = copy_to_user(arg, &sop, sizeof(sop));
		if (ret)
			return -EFAULT;
		return ret;
	case CIOCFSESSION:
		ret = get_user(ses, (uint32_t __user *) arg);
		if (unlikely(ret))
			return ret;
		ret = crypto_finish_session(fcr, ses);
		return ret;
	case CIOCGSESSINFO:
		if (unlikely(copy_from_user(&siop, arg, sizeof(siop))))
			return -EFAULT;

		ret = get_session_info(fcr, &siop);
		if (unlikely(ret))
			return ret;
		return copy_to_user(arg, &siop, sizeof(siop));
	case CIOCCRYPT:
		ret = kcop_from_user(&kcop, fcr, arg);
		if (ret) {
			dwarning("Error copying from user");
			return ret;
		}

		ret = crypto_run(fcr, &kcop);
		if ((ret)) {
			dwarning("Error in crypto_run");
			return ret;
		}

		return kcop_to_user(&kcop, fcr, arg);
	default:
		derr("Error unknown cmd");
		return -EINVAL;
	}
}

static unsigned int mtk_dtv_cryptodev_poll(struct file *file, poll_table *wait)
{
	return 0;
}

/* compatibility code for 32bit userlands */
#ifdef CONFIG_COMPAT

static inline void compat_to_session_op(struct compat_session_op *compat, struct session_op *sop)
{
	sop->cavenderid = compat->cavenderid;
	sop->cipher = compat->cipher;
	sop->mac = compat->mac;
	sop->dataformat = compat->dataformat;

	sop->sample_info.cryptolen = compat->sample_info.cryptolen;
	sop->sample_info.skippedlen = compat->sample_info.skippedlen;
	sop->sample_info.skippedoffset = compat->sample_info.skippedoffset;

	sop->keysource = compat->keysource;
	sop->oddkeyindex = compat->oddkeyindex;
	sop->evenkeyindex = compat->evenkeyindex;
	sop->keylen = compat->keylen;

	sop->key = compat_ptr(compat->key);
	sop->mackeylen = compat->mackeylen;
	sop->mackey = compat_ptr(compat->mackey);

	sop->pidnum = compat->pidnum;
	sop->pid = compat_ptr(compat->pid);
	sop->keytype = compat->keytype;

	sop->ses = compat->ses;
}

static inline void session_op_to_compat(struct session_op *sop, struct compat_session_op *compat)
{
	compat->cavenderid = sop->cavenderid;
	compat->cipher = sop->cipher;
	compat->mac = sop->mac;
	compat->dataformat = sop->dataformat;

	compat->sample_info.cryptolen = sop->sample_info.cryptolen;
	compat->sample_info.skippedlen = sop->sample_info.skippedlen;
	compat->sample_info.skippedoffset = sop->sample_info.skippedoffset;

	compat->keysource = sop->keysource;
	compat->oddkeyindex = sop->oddkeyindex;
	compat->evenkeyindex = sop->evenkeyindex;
	compat->keylen = sop->keylen;

	compat->key = ptr_to_compat(sop->key);
	compat->mackeylen = sop->mackeylen;
	compat->mackey = ptr_to_compat(sop->mackey);

	compat->pidnum = sop->pidnum;
	compat->pid = ptr_to_compat(sop->pid);
	compat->keytype = sop->keytype;

	compat->ses = sop->ses;
}

static inline void compat_to_crypt_op(struct compat_crypt_op *compat, struct crypt_op *cop)
{
	cop->srcfd = compat->srcfd;
	cop->dstfd = compat->dstfd;
	cop->ses = compat->ses;
	cop->op = compat->op;
	cop->flags = compat->flags;
	cop->len = compat->len;

	cop->mac = compat_ptr(compat->mac);
	cop->iv = compat_ptr(compat->iv);
}

static inline void crypt_op_to_compat(struct crypt_op *cop, struct compat_crypt_op *compat)
{
	compat->srcfd = cop->srcfd;
	compat->dstfd = cop->dstfd;
	compat->ses = cop->ses;
	compat->op = cop->op;
	compat->flags = cop->flags;
	compat->len = cop->len;

	compat->mac = ptr_to_compat(cop->mac);
	compat->iv = ptr_to_compat(cop->iv);
}

static int compat_kcop_from_user(struct kernel_crypt_op *kcop,
				 struct fcrypt *fcr, void __user *arg)
{
	struct compat_crypt_op compat_cop;

	if (unlikely(copy_from_user(&compat_cop, arg, sizeof(compat_cop))))
		return -EFAULT;
	compat_to_crypt_op(&compat_cop, &kcop->cop);

	return fill_kcop_from_cop(kcop, fcr);
}

static int compat_kcop_to_user(struct kernel_crypt_op *kcop, struct fcrypt *fcr, void __user *arg)
{
	int ret;
	struct compat_crypt_op compat_cop;

	ret = fill_cop_from_kcop(kcop, fcr);
	if (unlikely(ret)) {
		dwarning("Error in fill_cop_from_kcop");
		return ret;
	}
	crypt_op_to_compat(&kcop->cop, &compat_cop);

	if (unlikely(copy_to_user(arg, &compat_cop, sizeof(compat_cop)))) {
		dwarning("Error copying to user");
		return -EFAULT;
	}
	return 0;
}

static long mtk_dtv_cryptodev_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg_)
{
	void __user *arg = (void __user *)arg_;
	struct crypt_priv *pcr = file->private_data;
	struct fcrypt *fcr;
	struct session_op sop;
	struct compat_session_op compat_sop;
	struct kernel_crypt_op kcop;
	int ret;
	uint32_t ses;

	if (unlikely(!pcr))
		return -EFAULT;

	fcr = &pcr->fcrypt;

	switch (cmd) {
	case CRIOGET:
	case CIOCFSESSION:
	case CIOCGSESSINFO:
		return mtk_dtv_cryptodev_ioctl(file, cmd, arg_);

	case COMPAT_CIOCGSESSION:
		if (unlikely(copy_from_user(&compat_sop, arg, sizeof(compat_sop))))
			return -EFAULT;

		compat_to_session_op(&compat_sop, &sop);

		ret = crypto_create_session(fcr, &sop);
		if (unlikely(ret))
			return ret;

		session_op_to_compat(&sop, &compat_sop);
		ret = copy_to_user(arg, &compat_sop, sizeof(compat_sop));
		if (unlikely(ret)) {
			crypto_finish_session(fcr, sop.ses);
			return -EFAULT;
		}
		return ret;

	case COMPAT_CIOCCRYPT:
		ret = compat_kcop_from_user(&kcop, fcr, arg);
		if (unlikely(ret))
			return ret;

		ret = crypto_run(fcr, &kcop);
		if (unlikely(ret))
			return ret;

		return compat_kcop_to_user(&kcop, fcr, arg);
	default:
		derr("Error unknown cmd");
		return -EINVAL;
	}
}
#endif				/* CONFIG_COMPAT */

static const struct file_operations mtk_dtv_cryptodev_fops = {
	.owner = THIS_MODULE,
	.open = mtk_dtv_cryptodev_open,
	.release = mtk_dtv_cryptodev_release,
	.unlocked_ioctl = mtk_dtv_cryptodev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mtk_dtv_cryptodev_compat_ioctl,
#endif				/* CONFIG_COMPAT */
	.poll = mtk_dtv_cryptodev_poll,
};

static struct miscdevice mtk_dtv_cryptodev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "crypto",
	.fops = &mtk_dtv_cryptodev_fops,
	.mode = 0666,
};

static int mtk_crypto_probe(struct platform_device *pdev)
{
#define MTK_CRYPTO_D7INDEX (2)
	struct mtk_cryp *cryp;
	int ret;

	if (!pdev)
		return -EINVAL;

	cryp = devm_kzalloc(&pdev->dev, sizeof(*cryp), GFP_KERNEL);
	if (!cryp)
		return -ENOMEM;

	cryp->d4_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(cryp->d4_base))
		return PTR_ERR(cryp->d4_base);
	cryp->d5_base = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(cryp->d5_base))
		return PTR_ERR(cryp->d5_base);
	cryp->d7_base = devm_platform_ioremap_resource(pdev, MTK_CRYPTO_D7INDEX);
	if (IS_ERR(cryp->d7_base))
		return PTR_ERR(cryp->d7_base);

	cryp->dev = &pdev->dev;
	dev_info(cryp->dev, "[%s] d4_base = 0x%lx, d5_base = 0x%lx, d7_base = 0x%lx\n", __func__,
		 (unsigned long)cryp->d4_base, (unsigned long)cryp->d5_base,
		 (unsigned long)cryp->d7_base);

	mutex_init(&(cryp->crypto_lock));

	/* Initialize hardware modules */
	ret = mtk_cipher_register(cryp);
	if (ret) {
		dev_err(cryp->dev, "Unable to register cipher.\n");
		return -EFAULT;
	}

	/* register misc device */
	cryp->misc_dev = &mtk_dtv_cryptodev;
	ret = misc_register(cryp->misc_dev);
	if (ret) {
		dev_err(cryp->dev, "[%s] register of /dev/mtk_dtv_crypto failed\n", __func__);
		return ret;
	}

	platform_set_drvdata(pdev, cryp);

	return 0;
}

static int mtk_crypto_remove(struct platform_device *pdev)
{
	struct mtk_cryp *cryp = platform_get_drvdata(pdev);

	iommu_fwspec_free(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id of_crypto_id[] = {
	{.compatible = "mediatek,dtv-crypto"},
	{},
};

MODULE_DEVICE_TABLE(of, of_crypto_id);

static struct platform_driver mtk_crypto_driver = {
	.probe = mtk_crypto_probe,
	.remove = mtk_crypto_remove,
	.driver = {
		   .name = "crypto",
		   .of_match_table = of_crypto_id,
		   },
};

module_platform_driver(mtk_crypto_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WinniePooh Wu <winniepooh.wu@mediatek.com>");
MODULE_DESCRIPTION("Cryptographic accelerator driver for DTV");
