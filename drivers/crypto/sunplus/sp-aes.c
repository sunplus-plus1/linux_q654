// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Sunplus Technology Co., Ltd.
 *       All rights reserved.
 */
//#define DEBUG
#include <crypto/internal/aead.h>
#include <crypto/internal/skcipher.h>
#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <crypto/scatterwalk.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/crypto.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <crypto/aes.h>
#include <crypto/chacha.h>
#include "sp-crypto.h"

#define SEC_DMA_ENABLE	(1 << 0)
#define SEC_DMA_SIZE(x) ((x) << 16)
#define M_KEYLEN(x)     (((x) >> 2) << 16)
#define AES_BUF_SIZE	(CHACHA_BLOCK_SIZE + AES_KEYSIZE_256 + CHACHA_IV_SIZE)

struct sp_crypto_aes_ctx {
	u32 key[AES_KEYSIZE_256];
	u32 key_length;
	u32 blksize;
	uintptr_t iv;
};

struct sp_aes_priv {
	u8* va;
	dma_addr_t pa;
	bool done;
	wait_queue_head_t wait;
	struct mutex lock;
} sp_aes;

extern struct sp_crypto_dev *sp_crypto_dev(void);

static int sp_aes_set_key(struct crypto_tfm *tfm, const u8 *in_key,
		unsigned int key_len)
{
	struct sp_crypto_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	int err;

	err = aes_check_keylen(key_len);
	if (err)
		return err;

	SP_CRYPTO_DBG("[%s:%d] %px %px %u", __FUNCTION__, __LINE__, ctx, in_key, key_len);
	ctx->key_length = key_len;
	if (!sp_aes.va)
		sp_aes.va = dma_alloc_coherent(sp_crypto_dev()->device, AES_BUF_SIZE, &sp_aes.pa, GFP_KERNEL);
	memcpy(ctx->key, in_key, key_len);
	ctx->blksize = AES_BLOCK_SIZE;

	return 0;
}

static void sp_aes_exec(struct crypto_tfm *tfm, uintptr_t dst, uintptr_t src, u32 size, u32 mode)
{
	struct sp_crypto_reg *reg = sp_crypto_dev()->reg;
	struct sp_crypto_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	int b = size < CHACHA_BLOCK_SIZE;

	SP_CRYPTO_DBG("[%04x:%02x] %08lx %08lx %u\n", mode, ctx->blksize, src, dst, size);
	mutex_lock(&sp_aes.lock);

	sp_aes.done = false;
	if (b) {
		memcpy(sp_aes.va, (void *)src, size);
		//dump_buf(sp_aes.va, size);
	}
	memcpy(sp_aes.va + CHACHA_BLOCK_SIZE, ctx->key, ctx->key_length);
	if (ctx->iv) {
		memcpy(sp_aes.va + CHACHA_BLOCK_SIZE + AES_KEYSIZE_256, (void *)ctx->iv, CHACHA_IV_SIZE);
		ctx->iv = sp_aes.pa + CHACHA_BLOCK_SIZE + AES_KEYSIZE_256;
	}
	smp_wmb(); /* memory barrier */

	W(AESPAR0, mode | M_KEYLEN(ctx->key_length));
	W(AESPAR1, ctx->iv); // iv
	ctx->iv = 0; // initial iv @ ctx 1st call hw
	W(AESPAR2, sp_aes.pa + CHACHA_BLOCK_SIZE); // kptr
	W(AESSPTR, b ? sp_aes.pa : src);
	W(AESDPTR, b ? sp_aes.pa : dst);
	W(AESDMACS, SEC_DMA_SIZE(b ? CHACHA_BLOCK_SIZE : size) | SEC_DMA_ENABLE);
	wait_event_interruptible_timeout(sp_aes.wait, sp_aes.done, 60*HZ);

	SP_CRYPTO_TRACE();
	if (b) {
		//dump_buf(sp_aes.va, size);
		memcpy((void *)dst, sp_aes.va, size);
	}

	mutex_unlock(&sp_aes.lock);
}

static void sp_aes_encrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	return sp_aes_exec(tfm, (uintptr_t)out, (uintptr_t)in, AES_BLOCK_SIZE, M_AES_ECB | M_ENC);
}

static void sp_aes_decrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	return sp_aes_exec(tfm, (uintptr_t)out, (uintptr_t)in, AES_BLOCK_SIZE, M_AES_ECB | M_DEC);
}

static int sp_chacha_set_key(struct crypto_skcipher *tfm,
	const u8 *in_key, unsigned int key_len)
{
	return sp_aes_set_key(&tfm->base, in_key, key_len);
}

static int sp_chacha_crypt(struct skcipher_request *req, u32 mode)
{
	struct device *dev = sp_crypto_dev()->device;
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sp_crypto_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	unsigned int nbytes;
	int err;

	err = skcipher_walk_virt(&walk, req, false);
	ctx->iv = (uintptr_t)req->iv;
	ctx->blksize = CHACHA_BLOCK_SIZE;

	while ((nbytes = walk.nbytes) > 0) {
		uintptr_t src = (uintptr_t)walk.src.virt.addr;
		uintptr_t dst = (uintptr_t)walk.dst.virt.addr;
		u32 bytes = nbytes - (nbytes % ctx->blksize) ?: nbytes;

		SP_CRYPTO_DBG("%u %u %u\n", nbytes, walk.total, bytes);
		if (bytes >= CHACHA_BLOCK_SIZE) {
			if (src != dst) {
				src = dma_map_single(dev, (void *)src, bytes, DMA_TO_DEVICE);
				dst = dma_map_single(dev, (void *)dst, bytes, DMA_FROM_DEVICE);
			} else {
				src = dma_map_single(dev, (void *)src, bytes, DMA_BIDIRECTIONAL);
				dst = src;
			}
		}
		sp_aes_exec(&tfm->base, dst, src, bytes, mode);
		if (bytes >= CHACHA_BLOCK_SIZE) {
			if (src != dst) {
				dma_unmap_single(dev, src, bytes, DMA_TO_DEVICE);
				dma_unmap_single(dev, dst, bytes, DMA_FROM_DEVICE);
			} else {
				dma_unmap_single(dev, src, bytes, DMA_BIDIRECTIONAL);
			}
		}

		nbytes -= bytes;
		err = skcipher_walk_done(&walk, nbytes);
		if (err)
			break;
	}

	return err;
}

static int sp_chacha_encrypt(struct skcipher_request *req)
{
	return sp_chacha_crypt(req, M_CHACHA20 | M_ENC);
}

static int sp_chacha_decrypt(struct skcipher_request *req)
{
	return sp_chacha_crypt(req, M_CHACHA20 | M_DEC);
}

static struct crypto_alg sp_aes_generic_alg = {
	.cra_name		=	"aes",
	.cra_driver_name	=	"sp-aes-generic",
	.cra_priority		=	400,
	.cra_flags		=	CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		=	AES_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct sp_crypto_aes_ctx),
	.cra_module		=	THIS_MODULE,
	.cra_u			=	{
		.cipher = {
			.cia_min_keysize	=	AES_MIN_KEY_SIZE,
			.cia_max_keysize	=	AES_MAX_KEY_SIZE,
			.cia_setkey		=	sp_aes_set_key,
			.cia_encrypt		=	sp_aes_encrypt,
			.cia_decrypt		=	sp_aes_decrypt
		}
	}
};

static struct skcipher_alg algs[] = {
	{
		.base.cra_name		= "chacha20",
		.base.cra_driver_name	= "sp-chacha20-generic",
		.base.cra_priority	= 400,
		.base.cra_blocksize	= 1,
		.base.cra_ctxsize	= sizeof(struct sp_crypto_aes_ctx),
		.base.cra_module	= THIS_MODULE,

		.min_keysize		= CHACHA_KEY_SIZE,
		.max_keysize		= CHACHA_KEY_SIZE,
		.ivsize			= CHACHA_IV_SIZE,
		.chunksize		= CHACHA_BLOCK_SIZE,
		.setkey			= sp_chacha_set_key,
		.encrypt		= sp_chacha_encrypt,
		.decrypt		= sp_chacha_decrypt,
	},
};

int sp_aes_init(void)
{
	init_waitqueue_head(&sp_aes.wait);
	mutex_init(&sp_aes.lock);
	return crypto_register_alg(&sp_aes_generic_alg) || crypto_register_skciphers(algs, ARRAY_SIZE(algs));
}
EXPORT_SYMBOL(sp_aes_init);

int sp_aes_finit(void)
{
	crypto_unregister_alg(&sp_aes_generic_alg);
	crypto_unregister_skciphers(algs, ARRAY_SIZE(algs));
	return 0;
}
EXPORT_SYMBOL(sp_aes_finit);

void sp_aes_irq(void *devid, u32 flag)
{
	if (flag & AES_DMA_IF) {
		sp_aes.done = true;
		wake_up(&sp_aes.wait);
	}
}
EXPORT_SYMBOL(sp_aes_irq);
