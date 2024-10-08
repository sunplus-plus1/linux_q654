// SPDX-License-Identifier: GPL-2.0

#include <crypto/algapi.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/poly1305.h>
#include <crypto/hash.h>
#include <crypto/ghash.h>
#include <crypto/md5.h>
#include <crypto/sha2.h>
#include <crypto/sha3.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/crypto.h>

#include "sp-crypto.h"
#include "sp-hash.h"

#define FIX_VPN_L2TP

//#define DEBUG
#ifdef DEBUG
#define DBG_PRNT	pr_info
#define DBG_DUMP	dump_buf
#else
#define DBG_PRNT(...)
#define DBG_DUMP(...)
#endif

#define SHA3_BUF_SIZE	(200)
#define WORK_BUF_SIZE	(65536)

struct sp_hash_ctx {
	u32 mode;
	u32 blocks_size;
	u32 hash_offset;
	u32 key_offset;
	u32 bytes;	// bytes in blocks
	u64 byte_count; // total bytes
	u32 skip;	// first N bytes is KEY
	u8 key[GHASH_BLOCK_SIZE]; // GHASH only
};

struct sp_hash_priv {
	/* WORKBUF: blocks + key + hash */
	u8 *va;
	dma_addr_t pa;

	bool done;
	wait_queue_head_t wait;
	struct mutex lock; // hw lock
} sp_hash;

static struct sp_crypto_dev *crypto;
static struct device *dev;
static struct sp_crypto_reg *reg;

static void do_blocks(struct sp_hash_ctx *ctx, u32 len, u32 flag)
{
	DBG_PRNT("%s (%08x): %u\n", __func__, ctx->mode | flag, len);
	if (len) {
		if (ctx->skip) {
			memcpy(sp_hash.va + ctx->key_offset, sp_hash.va, ctx->skip);
			len -= ctx->skip;
		}

		W(HASHPAR0, ctx->mode | flag);
		W(HASHSPTR, sp_hash.pa + ctx->skip);
		DBG_PRNT("HASHPAR0 = %08x\n", R(HASHPAR0));
		DBG_PRNT("HASHPAR1 = %08x\n", R(HASHPAR1));
		DBG_PRNT("HASHPAR2 = %08x\n", R(HASHPAR2));
		DBG_PRNT("HASHSPTR = %08x\n", R(HASHSPTR));
		DBG_PRNT("HASHDPTR = %08x\n", R(HASHDPTR));
		DBG_PRNT("HASHDMACS = %08x\n", SEC_DMA_SIZE(len) | SEC_DMA_ENABLE);

		sp_hash.done = false;
		smp_wmb(); /* memory barrier */
		W(HASHDMACS, SEC_DMA_SIZE(len) | SEC_DMA_ENABLE);
		wait_event_interruptible_timeout(sp_hash.wait, sp_hash.done, 60 * HZ);

		ctx->bytes = 0;
		ctx->skip = 0;
	}
}

static int sp_shash_init(struct shash_desc *desc)
{
	struct crypto_tfm *tfm = &desc->tfm->base;
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(tfm);
	u32 bsize = crypto_tfm_alg_blocksize(tfm);
	u32 digest_len = crypto_shash_alg(desc->tfm)->digestsize;
	u32 hash_size, key_size;
	u32 *hash;

	ctx->mode = crypto_tfm_alg_priority(tfm) & ~SP_CRYPTO_PRI;
	ctx->skip = (ctx->mode == M_POLY1305) ? POLY1305_KEY_SIZE : 0;

	hash_size = ((ctx->mode & M_MMASK) == M_SHA3) ? SHA3_BUF_SIZE : (ctx->skip ?: digest_len);
	key_size = (ctx->mode == M_GHASH) ? GHASH_BLOCK_SIZE : (ctx->skip ?: bsize);

	ctx->hash_offset = WORK_BUF_SIZE - hash_size;
	ctx->key_offset = ctx->hash_offset - key_size;
	ctx->blocks_size = ctx->key_offset / bsize * bsize;
	hash = (u32 *)(sp_hash.va + ctx->hash_offset);

	switch (ctx->mode) {
	case M_MD5:
		hash[0] = MD5_H0;
		hash[1] = MD5_H1;
		hash[2] = MD5_H2;
		hash[3] = MD5_H3;
		break;
	case M_SHA256:
		hash[0] = cpu_to_be32(SHA256_H0);
		hash[1] = cpu_to_be32(SHA256_H1);
		hash[2] = cpu_to_be32(SHA256_H2);
		hash[3] = cpu_to_be32(SHA256_H3);
		hash[4] = cpu_to_be32(SHA256_H4);
		hash[5] = cpu_to_be32(SHA256_H5);
		hash[6] = cpu_to_be32(SHA256_H6);
		hash[7] = cpu_to_be32(SHA256_H7);
		break;
	case M_SHA512:
		((u64 *)hash)[0] = cpu_to_be64(SHA512_H0);
		((u64 *)hash)[1] = cpu_to_be64(SHA512_H1);
		((u64 *)hash)[2] = cpu_to_be64(SHA512_H2);
		((u64 *)hash)[3] = cpu_to_be64(SHA512_H3);
		((u64 *)hash)[4] = cpu_to_be64(SHA512_H4);
		((u64 *)hash)[5] = cpu_to_be64(SHA512_H5);
		((u64 *)hash)[6] = cpu_to_be64(SHA512_H6);
		((u64 *)hash)[7] = cpu_to_be64(SHA512_H7);
		break;
	default:
		memset(hash, 0, hash_size);
		break;
	}

	ctx->byte_count = 0;
	ctx->bytes = 0;
	DBG_PRNT("[%08x] %u %u %u\n", ctx->mode, bsize, digest_len, ctx->blocks_size);

	return 0;
}

static void sp_hash_lock(struct sp_hash_ctx *ctx)
{
	if (!ctx->byte_count) {
		mutex_lock(&sp_hash.lock);
		if (ctx->mode == M_GHASH)
			memcpy(sp_hash.va + ctx->key_offset, ctx->key, sizeof(ctx->key));
		W(HASHDPTR, sp_hash.pa + ctx->hash_offset);
		W(HASHPAR1, sp_hash.pa + ctx->hash_offset);
		W(HASHPAR2, sp_hash.pa + ctx->key_offset);
	}
}

static int sp_shash_update(struct shash_desc *desc, const u8 *data, u32 len)
{
	if (len) {
		struct sp_hash_ctx *ctx = crypto_tfm_ctx(&desc->tfm->base);
		const u32 blocks_size = ctx->blocks_size;
		u32 avail = blocks_size - ctx->bytes; // free bytes in blocks

		//DBG_PRNT("%px[%u] %u\n", data, ctx->bytes, len);
		sp_hash_lock(ctx);
		ctx->byte_count += len;

		if (avail >= len) {
			memcpy(sp_hash.va + ctx->bytes, data, len); // append to blocks
		} else {
			do {
				memcpy(sp_hash.va + ctx->bytes, data, avail); // fill blocks
				do_blocks(ctx, blocks_size, 0);

				data += avail;
				len -= avail;

				avail = blocks_size;
			} while (len > blocks_size);

			memcpy(sp_hash.va, data, len); // saved to blocks
		}
		ctx->bytes += len;
	}

	return 0;
}

static int sp_shash_final(struct shash_desc *desc, u8 *out)
{
	struct crypto_tfm *tfm = &desc->tfm->base;
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(tfm);
	u32 bsize = crypto_tfm_alg_blocksize(tfm);
	u32 digest_len = crypto_shash_alg(desc->tfm)->digestsize;
	u32 t = ctx->bytes % bsize;
	u8 *p = sp_hash.va + ctx->bytes;
	int padding; // padding zero bytes
	u32 poly1305_padding = 0;
	int ret = 0;

	sp_hash_lock(ctx);

	// padding
	switch (ctx->mode) {
	case M_MD5:
	case M_SHA256:
		padding = (bsize * 2 - t - 1 - sizeof(u64)) % bsize;
		*p++ = 0x80;
		break;
	case M_SHA512:
		padding = (bsize * 2 - t - 1 - sizeof(u128)) % bsize;
		*p++ = 0x80;
		break;
	case M_GHASH:
		padding = (t || !ctx->byte_count) ? (bsize - t) : 0;
		break;
	case M_POLY1305:
		if (ctx->byte_count < POLY1305_KEY_SIZE) {
			ret = -ENOKEY;
			goto out;
		}
		padding = (ctx->byte_count == POLY1305_KEY_SIZE) || t;
		if (padding) {
			poly1305_padding = (1 << 8);
			if (ctx->byte_count == POLY1305_KEY_SIZE) {
				padding = bsize;
			} else {
				padding = bsize - t - 1;
				*p++ = 1;
			}
		}
		break;
	default: // SHA3
		padding = bsize - t - 1;
		*p++ = 0x06;
		break;
	}

	memset(p, 0, padding); // padding zero
	p += padding;

	switch (ctx->mode) {
	case M_MD5:
		((u32 *)p)[0] = ctx->byte_count << 3;
		((u32 *)p)[1] = ctx->byte_count >> 29;
		p += sizeof(u64);
		break;
	case M_SHA256:
		((u64 *)p)[0] = cpu_to_be64(ctx->byte_count << 3);
		p += sizeof(u64);
		break;
	case M_SHA512:
		((u64 *)p)[0] = cpu_to_be64(ctx->byte_count >> 61);
		((u64 *)p)[1] = cpu_to_be64(ctx->byte_count << 3);
		p += sizeof(u64) << 1;
		break;
	case M_GHASH:
	case M_POLY1305:
		break;
	default: // SHA3
		*(p - 1) |= 0x80;
		break;
	}

	//DBG_DUMP(sp_hash.va, p - sp_hash.va);
	do_blocks(ctx, p - sp_hash.va, M_FINAL | poly1305_padding);

	memcpy(out, sp_hash.va + ctx->hash_offset, digest_len);
	//DBG_DUMP(out, digest_len);
out:
	mutex_unlock(&sp_hash.lock);
	return ret;
}

static int sp_shash_ghash_setkey(struct crypto_shash *tfm,
				 const u8 *key, unsigned int keylen)
{
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(&tfm->base);

	if (keylen != GHASH_BLOCK_SIZE)
		return -EINVAL;

	memcpy(ctx->key, key, keylen);

	return 0;
}

static struct shash_alg hash_algs[] = {
	{
		.digestsize	= GHASH_DIGEST_SIZE,
		.init		= sp_shash_init,
		.setkey		= sp_shash_ghash_setkey,
		.update		= sp_shash_update,
		.final		= sp_shash_final,
		.base		= {
			.cra_name	= "ghash",
			.cra_driver_name = "sp-ghash",
			.cra_blocksize	= GHASH_BLOCK_SIZE,
			.cra_module	= THIS_MODULE,
			.cra_priority	= SP_CRYPTO_PRI | M_GHASH,
			.cra_ctxsize	= sizeof(struct sp_hash_ctx),
		},
	},
	{
		.digestsize	= MD5_DIGEST_SIZE,
		.init		= sp_shash_init,
		.update		= sp_shash_update,
		.final		= sp_shash_final,
		.base		= {
			.cra_name	= "md5",
			.cra_driver_name = "sp-md5",
			.cra_blocksize	= MD5_HMAC_BLOCK_SIZE,
			.cra_module	= THIS_MODULE,
			.cra_priority	= SP_CRYPTO_PRI | M_MD5,
			.cra_ctxsize	= sizeof(struct sp_hash_ctx),
		},
	},
	{
		.digestsize	= SHA3_224_DIGEST_SIZE,
		.init		= sp_shash_init,
		.update		= sp_shash_update,
		.final		= sp_shash_final,
		.base		= {
			.cra_name	= "sha3-224",
			.cra_driver_name = "sp-sha3-224",
			.cra_blocksize	= SHA3_224_BLOCK_SIZE,
			.cra_module	= THIS_MODULE,
			.cra_priority	= SP_CRYPTO_PRI | M_SHA3_224,
			.cra_ctxsize	= sizeof(struct sp_hash_ctx),
		},
	},
	{
		.digestsize	= SHA3_256_DIGEST_SIZE,
		.init		= sp_shash_init,
		.update		= sp_shash_update,
		.final		= sp_shash_final,
		.base		= {
			.cra_name	= "sha3-256",
			.cra_driver_name = "sp-sha3-256",
			.cra_blocksize	= SHA3_256_BLOCK_SIZE,
			.cra_module	= THIS_MODULE,
			.cra_priority	= SP_CRYPTO_PRI | M_SHA3_256,
			.cra_ctxsize	= sizeof(struct sp_hash_ctx),
		},
	},
	{
		.digestsize	= SHA3_384_DIGEST_SIZE,
		.init		= sp_shash_init,
		.update		= sp_shash_update,
		.final		= sp_shash_final,
		.base		= {
			.cra_name	= "sha3-384",
			.cra_driver_name = "sp-sha3-384",
			.cra_blocksize	= SHA3_384_BLOCK_SIZE,
			.cra_module	= THIS_MODULE,
			.cra_priority	= SP_CRYPTO_PRI | M_SHA3_384,
			.cra_ctxsize	= sizeof(struct sp_hash_ctx),
		},
	},
	{
		.digestsize	= SHA3_512_DIGEST_SIZE,
		.init		= sp_shash_init,
		.update		= sp_shash_update,
		.final		= sp_shash_final,
		.base		= {
			.cra_name	= "sha3-512",
			.cra_driver_name = "sp-sha3-512",
			.cra_blocksize	= SHA3_512_BLOCK_SIZE,
			.cra_module	= THIS_MODULE,
			.cra_priority	= SP_CRYPTO_PRI | M_SHA3_512,
			.cra_ctxsize	= sizeof(struct sp_hash_ctx),
		},
	},
	#ifndef FIX_VPN_L2TP // FIXME: failed in VPN:L2TP auth
	{
		.digestsize	= SHA256_DIGEST_SIZE,
		.init		= sp_shash_init,
		.update		= sp_shash_update,
		.final		= sp_shash_final,
		.base		= {
			.cra_name	= "sha256",
			.cra_driver_name = "sp-sha256",
			.cra_blocksize	= SHA256_BLOCK_SIZE,
			.cra_module	= THIS_MODULE,
			.cra_priority	= SP_CRYPTO_PRI | M_SHA256,
			.cra_ctxsize	= sizeof(struct sp_hash_ctx),
		}
	},
	#endif
	{
		.digestsize	= SHA512_DIGEST_SIZE,
		.init		= sp_shash_init,
		.update		= sp_shash_update,
		.final		= sp_shash_final,
		.base		= {
			.cra_name	= "sha512",
			.cra_driver_name = "sp-sha512",
			.cra_blocksize	= SHA512_BLOCK_SIZE,
			.cra_module	= THIS_MODULE,
			.cra_priority	= SP_CRYPTO_PRI | M_SHA512,
			.cra_ctxsize	= sizeof(struct sp_hash_ctx),
		}
	},
	{
		.digestsize	= POLY1305_DIGEST_SIZE,
		.init		= sp_shash_init,
		.update		= sp_shash_update,
		.final		= sp_shash_final,
		.base		= {
			.cra_name	= "poly1305",
			.cra_driver_name = "sp-poly1305",
			.cra_blocksize	= POLY1305_BLOCK_SIZE,
			.cra_module	= THIS_MODULE,
			.cra_priority	= SP_CRYPTO_PRI | M_POLY1305,
			.cra_ctxsize	= sizeof(struct sp_hash_ctx),
		},
	},
};

int sp_hash_init(void)
{
	if (!dev) {
		crypto = sp_crypto_alloc_dev(SP_CRYPTO_HASH);
		dev = crypto->device;
		reg = crypto->reg;
		init_waitqueue_head(&sp_hash.wait);
		mutex_init(&sp_hash.lock);
		sp_hash.va = dma_alloc_coherent(dev, WORK_BUF_SIZE, &sp_hash.pa, GFP_KERNEL);
	}
	return crypto_register_shashes(hash_algs, ARRAY_SIZE(hash_algs));
}
EXPORT_SYMBOL(sp_hash_init);

int sp_hash_finit(void)
{
	crypto_unregister_shashes(hash_algs, ARRAY_SIZE(hash_algs));
	return 0;
}
EXPORT_SYMBOL(sp_hash_finit);

void sp_hash_irq(void *devid, u32 flag)
{
	if (flag & HASH_DMA_IF) {
		sp_hash.done = true;
		wake_up(&sp_hash.wait);
	}
}
EXPORT_SYMBOL(sp_hash_irq);
