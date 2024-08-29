// SPDX-License-Identifier: GPL-2.0

//#define DEBUG
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

#define M_KEYLEN(x)     (((x) >> 2) << 16)

#define WORK_BUF_SIZE	(CHACHA_BLOCK_SIZE + AES_KEYSIZE_256 + CHACHA_IV_SIZE)
#define IV_OFFSET	(CHACHA_BLOCK_SIZE + AES_KEYSIZE_256)
#define KEY_OFFSET	(CHACHA_BLOCK_SIZE)

struct sp_aes_ctx {
	u8 key[AES_KEYSIZE_256];
	u32 mode;
	u32 gctr;
	u32 keysize;
};

struct sp_aes_priv {
	u8 *va;
	dma_addr_t pa;
	bool done;
	struct mutex lock; // hw lock
} sp_aes;

static struct sp_crypto_dev *crypto;
static struct device *dev;
static struct sp_crypto_reg *reg;

static int sp_aes_set_key(struct crypto_tfm *tfm, const u8 *key, unsigned int key_len)
{
	struct sp_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	int err;

	//SP_CRYPTO_DBG("[%s:%d] %px %px %u", __func__, __LINE__, ctx, key, key_len);
	err = aes_check_keylen(key_len);
	if (!err) {
		memcpy(ctx->key, key, key_len);
		ctx->keysize = key_len;
	}

	return err;
}

static void sp_aes_hw_exec(u32 src, u32 dst, u32 bytes)
{
	W(AESSPTR, src);
	W(AESDPTR, dst);

	sp_aes.done = false;
	smp_wmb(); /* memory barrier */
	W(AESDMACS, SEC_DMA_SIZE(bytes) | SEC_DMA_ENABLE);
	while (!sp_aes.done)
		smp_wmb(); // busy wait, can't schedule
}

static void sp_aes_exec(struct crypto_tfm *tfm, u8 *out, const u8 *in, u32 mode)
{
	struct sp_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	const u32 bytes = AES_BLOCK_SIZE;

	//SP_CRYPTO_DBG("[%s] %px %px %px %u %u\n", __func__, ctx, in, out, mode, blksize);
	mutex_lock(&sp_aes.lock);

	// mode
	W(AESPAR0, mode | M_KEYLEN(ctx->keysize));

	// key
	memcpy(sp_aes.va + KEY_OFFSET, ctx->key, ctx->keysize);
	W(AESPAR2, sp_aes.pa + KEY_OFFSET);

	memcpy(sp_aes.va, in, bytes);

	sp_aes_hw_exec(sp_aes.pa, sp_aes.pa, bytes);

	memcpy(out, sp_aes.va, bytes);

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

static int sp_skcipher_set_key(struct crypto_skcipher *tfm, const u8 *key, unsigned int key_len)
{
	return sp_aes_set_key(&tfm->base, key, key_len);
}

#define IV ((u32 *)(sp_aes.va + IV_OFFSET))

static int sp_skcipher_crypt(struct skcipher_request *req, u32 mode)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sp_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	const unsigned int ivsize = crypto_skcipher_ivsize(tfm);
	const unsigned int blksize = crypto_skcipher_chunksize(tfm);
	struct skcipher_walk walk;
	unsigned int nbytes;
	int err;

	mutex_lock(&sp_aes.lock);

	// mode
	mode |= ctx->mode | ctx->gctr;
	W(AESPAR0, mode | M_KEYLEN(ctx->keysize));

	// key
	memcpy(sp_aes.va + KEY_OFFSET, ctx->key, ctx->keysize);
	W(AESPAR2, sp_aes.pa + KEY_OFFSET);

	// iv in
	if (ivsize) {
		memcpy(IV, req->iv, ivsize);
		W(AESPAR1, sp_aes.pa + IV_OFFSET);
	}

	err = skcipher_walk_virt(&walk, req, false);
	while (!err && (nbytes = walk.nbytes) > 0) {
		void *src = walk.src.virt.addr;
		void *dst = walk.dst.virt.addr;
		u32 bytes = nbytes & ~(blksize - 1) ?: nbytes;

do_blocks:
		if (bytes > blksize) {
			dma_addr_t src_pa, dst_pa;

			if (ctx->mode == M_CHACHA20) {
				u32 iv = IV[0];
				u32 blks = bytes / CHACHA_BLOCK_SIZE;

				if ((iv + blks) < iv)
					bytes = (~iv + 1) * CHACHA_BLOCK_SIZE;
			}

			if (src != dst) {
				src_pa = dma_map_single(dev, src, bytes, DMA_TO_DEVICE);
				dst_pa = dma_map_single(dev, dst, bytes, DMA_FROM_DEVICE);
			} else {
				src_pa = dma_map_single(dev, src, bytes, DMA_BIDIRECTIONAL);
				dst_pa = src_pa;
			}
			if (unlikely(dma_mapping_error(dev, dst_pa))) {
				if (unlikely(!dma_mapping_error(dev, src_pa)))
					dma_unmap_single(dev, src_pa, bytes, DMA_TO_DEVICE);
				bytes -= blksize;
				dev_warn_once(dev, "dma_mapping_error, decrease bytes & retry!\n");
				goto do_blocks;
			}

			sp_aes_hw_exec(src_pa, dst_pa, bytes);

			if (src != dst) {
				dma_unmap_single(dev, src_pa, bytes, DMA_TO_DEVICE);
				dma_unmap_single(dev, dst_pa, bytes, DMA_FROM_DEVICE);
			} else {
				dma_unmap_single(dev, src_pa, bytes, DMA_BIDIRECTIONAL);
			}
		} else {
			memcpy(sp_aes.va, src, bytes);
			//dump_buf(sp_aes.va, bytes);

			sp_aes_hw_exec(sp_aes.pa, sp_aes.pa, blksize);

			//dump_buf(sp_aes.va, bytes);
			memcpy(dst, sp_aes.va, bytes);
		}

		/* CHACHA20 fix hw iv issue */
		if (ctx->mode == M_CHACHA20 && !IV[0])
			IV[1]--;

		nbytes -= bytes;
		err = skcipher_walk_done(&walk, nbytes);
	}

	// iv out
	switch (ctx->mode) {
	case M_AES_CBC:
	case M_AES_CTR:
		memcpy(req->iv, IV, ivsize);
	}

	mutex_unlock(&sp_aes.lock);

	return err;
}

static int sp_cra_init(struct crypto_tfm *tfm)
{
	struct sp_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	ctx->gctr = (ctx->mode ? 32 : 128) << 24; // 2nd call from gcm.c hack
	ctx->mode = crypto_tfm_alg_priority(tfm) & ~SP_CRYPTO_PRI;

	return 0;
}

static int sp_skcipher_encrypt(struct skcipher_request *req)
{
	return sp_skcipher_crypt(req, M_ENC);
}

static int sp_skcipher_decrypt(struct skcipher_request *req)
{
	return sp_skcipher_crypt(req, M_DEC);
}

static struct crypto_alg sp_aes_generic_alg = {
	.cra_name		=	"aes",
	.cra_driver_name	=	"sp-aes-generic",
	.cra_priority		=	SP_CRYPTO_PRI,
	.cra_flags		=	CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		=	AES_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct sp_aes_ctx),
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
		.base.cra_name		= "ecb(aes)",
		.base.cra_driver_name	= "sp-aes-ecb",
		.base.cra_priority	= SP_CRYPTO_PRI | M_AES_ECB,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_alignmask	= AES_BLOCK_SIZE - 1,
		.base.cra_ctxsize	= sizeof(struct sp_aes_ctx),
		.base.cra_init		= sp_cra_init,
		.base.cra_module	= THIS_MODULE,

		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.ivsize			= 0,
		.chunksize		= AES_BLOCK_SIZE,
		.setkey			= sp_skcipher_set_key,
		.encrypt		= sp_skcipher_encrypt,
		.decrypt		= sp_skcipher_decrypt,
	},
	{
		.base.cra_name		= "cbc(aes)",
		.base.cra_driver_name	= "sp-aes-cbc",
		.base.cra_priority	= SP_CRYPTO_PRI | M_AES_CBC,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_alignmask	= AES_BLOCK_SIZE - 1,
		.base.cra_ctxsize	= sizeof(struct sp_aes_ctx),
		.base.cra_init		= sp_cra_init,
		.base.cra_module	= THIS_MODULE,

		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.ivsize			= AES_BLOCK_SIZE,
		.chunksize		= AES_BLOCK_SIZE,
		.setkey			= sp_skcipher_set_key,
		.encrypt		= sp_skcipher_encrypt,
		.decrypt		= sp_skcipher_decrypt,
	},
	{
		.base.cra_name		= "ctr(aes)",
		.base.cra_driver_name	= "sp-aes-ctr",
		.base.cra_priority	= SP_CRYPTO_PRI | M_AES_CTR,
		.base.cra_blocksize	= 1,
		.base.cra_alignmask	= AES_BLOCK_SIZE - 1,
		.base.cra_ctxsize	= sizeof(struct sp_aes_ctx),
		.base.cra_init		= sp_cra_init,
		.base.cra_module	= THIS_MODULE,

		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.ivsize			= AES_BLOCK_SIZE,
		.chunksize		= AES_BLOCK_SIZE,
		.setkey			= sp_skcipher_set_key,
		.encrypt		= sp_skcipher_encrypt,
		.decrypt		= sp_skcipher_decrypt,
	},
	{
		.base.cra_name		= "chacha20",
		.base.cra_driver_name	= "sp-chacha20",
		.base.cra_priority	= SP_CRYPTO_PRI | M_CHACHA20,
		.base.cra_blocksize	= 1,
		.base.cra_alignmask	= AES_BLOCK_SIZE - 1,
		.base.cra_ctxsize	= sizeof(struct sp_aes_ctx),
		.base.cra_init		= sp_cra_init,
		.base.cra_module	= THIS_MODULE,

		.min_keysize		= CHACHA_KEY_SIZE,
		.max_keysize		= CHACHA_KEY_SIZE,
		.ivsize			= CHACHA_IV_SIZE,
		.chunksize		= CHACHA_BLOCK_SIZE,
		.setkey			= sp_skcipher_set_key,
		.encrypt		= sp_skcipher_encrypt,
		.decrypt		= sp_skcipher_decrypt,
	},
};

int sp_aes_init(void)
{
	if (!dev) {
		crypto = sp_crypto_alloc_dev(SP_CRYPTO_AES);
		dev = crypto->device;
		reg = crypto->reg;
		mutex_init(&sp_aes.lock);
		sp_aes.va = dma_alloc_coherent(dev, WORK_BUF_SIZE, &sp_aes.pa, GFP_KERNEL);
	}
	return crypto_register_alg(&sp_aes_generic_alg) ||
	       crypto_register_skciphers(algs, ARRAY_SIZE(algs));
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
	if (flag & AES_DMA_IF)
		sp_aes.done = true;
}
EXPORT_SYMBOL(sp_aes_irq);
