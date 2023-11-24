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
#include <crypto/aes.h>
#include <crypto/chacha.h>
#include "sp-crypto.h"

#define SEC_DMA_ENABLE	(1 << 0)
#define SEC_DMA_SIZE(x) ((x) << 16)
#define M_KEYLEN(x)     (((x) >> 2) << 16)
#define AES_BUF_SIZE	(AES_BLOCK_SIZE + AES_KEYSIZE_256)

struct sp_crypto_aes_ctx {
	u32 key[AES_KEYSIZE_256];
	u32 key_length;
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

	return 0;
}

static void sp_aes_exec(struct crypto_tfm *tfm, u8 *out, const u8 *in, u32 mode)
{
	struct sp_crypto_reg *reg = sp_crypto_dev()->reg;
	const struct sp_crypto_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	SP_CRYPTO_DBG("[%s:%d] %px %px %px %px %u\n", __FUNCTION__, __LINE__, reg, ctx, in, out, mode);
	mutex_lock(&sp_aes.lock);

	sp_aes.done = false;
	//dump_buf(in, AES_BLOCK_SIZE);
	memcpy(sp_aes.va, in, AES_BLOCK_SIZE);
	memcpy(sp_aes.va + AES_BLOCK_SIZE, ctx->key, ctx->key_length);
	smp_wmb(); /* memory barrier */

	W(AESPAR0, M_AES_ECB | M_KEYLEN(ctx->key_length) | mode);
	W(AESPAR1, 0); // iv
	W(AESPAR2, sp_aes.pa + AES_BLOCK_SIZE);
	W(AESSPTR, sp_aes.pa);
	W(AESDPTR, sp_aes.pa);
	W(AESDMACS, SEC_DMA_SIZE(AES_BLOCK_SIZE) | SEC_DMA_ENABLE);
	wait_event_interruptible_timeout(sp_aes.wait, sp_aes.done, 60*HZ);
	//while (!(R(SECIF) & AES_DMA_IF));
	//W(SECIF, AES_DMA_IF); // clear aes dma finish flag

	SP_CRYPTO_TRACE();
	memcpy(out, sp_aes.va, AES_BLOCK_SIZE);
	//dump_buf(out, AES_BLOCK_SIZE);

	mutex_unlock(&sp_aes.lock);
}

static void sp_aes_encrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	return sp_aes_exec(tfm, out, in, M_ENC);
}

static void sp_aes_decrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	return sp_aes_exec(tfm, out, in, M_DEC);
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

int sp_aes_init(void)
{
	init_waitqueue_head(&sp_aes.wait);
	mutex_init(&sp_aes.lock);
	return crypto_register_alg(&sp_aes_generic_alg);
}
EXPORT_SYMBOL(sp_aes_init);

int sp_aes_finit(void)
{
	crypto_unregister_alg(&sp_aes_generic_alg);
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
