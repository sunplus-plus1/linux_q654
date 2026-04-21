// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/mpi.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/count_zeros.h>
#include <crypto/internal/rsa.h>
#include <crypto/internal/akcipher.h>
#include <crypto/akcipher.h>
#include <crypto/algapi.h>
#include "sp-crypto.h"
#include "sp-rsa.h"

#define MAX_RSA_BITS	(2048)
#define MAX_RSA_BYTES	(MAX_RSA_BITS / BITS_PER_BYTE)

#define RSA_BYTES_MASK	(7)

//#define TEST_RSA_SPEED

#define OFFSET_SRC	(MAX_RSA_BYTES * 0)
#define OFFSET_EXP	(MAX_RSA_BYTES * 1)
#define OFFSET_MOD	(MAX_RSA_BYTES * 2)
#define OFFSET_DST	(MAX_RSA_BYTES * 3)
#define OFFSET_P2	(MAX_RSA_BYTES * 4)

struct rsa_priv_data {
	/* WORK BUFFER: MAX_RSA_BYTES * 5 */
	/* SRC + EXP + MOD + DST + P2 */
	u8 *va;
	dma_addr_t pa;

	unsigned int nbytes;
	struct mutex lock; // hw lock
	wait_queue_head_t wait;
	u32 wait_flag;

	struct sp_crypto_dev *dev;
};

static struct rsa_priv_data rsa = {};

struct sp_rsa_key {
	MPI n;
	MPI e;
	MPI d;
	MPI p;
	MPI q;
	MPI dp;
	MPI dq;
	MPI qinv;
};

static inline struct sp_rsa_key *sp_rsa_get_key(struct crypto_akcipher *tfm)
{
	return akcipher_tfm_ctx(tfm);
}

static void sp_rsa_free_key(struct sp_rsa_key *key)
{
	mpi_free(key->d);
	mpi_free(key->e);
	mpi_free(key->n);
	mpi_free(key->p);
	mpi_free(key->q);
	mpi_free(key->dp);
	mpi_free(key->dq);
	mpi_free(key->qinv);
	key->d = NULL;
	key->e = NULL;
	key->n = NULL;
	key->p = NULL;
	key->q = NULL;
	key->dp = NULL;
	key->dq = NULL;
	key->qinv = NULL;
}

static rsabase_t mont_w(void *ptr, unsigned int len)
{
	rsabase_t t = 1;
	rsabase_t m;
	int i;
	int lb = RSA_BASE;

#ifdef RSA_DATA_BIGENDBIAN
	m = sp_rsabase_be_to_cpu(*(rsabase_t *)(ptr + len - sizeof(m)));
#else
	m = sp_rsabase_le_to_cpu(*(rsabase_t *)ptr);
#endif
	for (i = 1 ; i < lb - 1; i++)
		t = (t * t * m);

	return -t;
}

static int sp_rsa_enc(struct akcipher_request *req);
static int sp_rsa_dec(struct akcipher_request *req);
static int sp_rsa_set_pub_key(struct crypto_akcipher *tfm, const void *key, unsigned int keylen);
static int sp_rsa_set_priv_key(struct crypto_akcipher *tfm, const void *key, unsigned int keylen);
static unsigned int sp_rsa_max_size(struct crypto_akcipher *tfm);
static void sp_rsa_exit_tfm(struct crypto_akcipher *tfm);

static struct akcipher_alg sp_rsa_alg = {
	.encrypt = sp_rsa_enc,
	.decrypt = sp_rsa_dec,
	.set_priv_key = sp_rsa_set_priv_key,
	.set_pub_key = sp_rsa_set_pub_key,
	.max_size = sp_rsa_max_size,
	.exit = sp_rsa_exit_tfm,
	.base = {
		.cra_name = "rsa",
		.cra_driver_name = "sp-rsa",
		.cra_priority = 200,
		.cra_module = THIS_MODULE,
		.cra_ctxsize = sizeof(struct sp_rsa_key),
	},
};

int sp_rsa_init(void)
{
	if (!rsa.dev) {
		rsa.dev = sp_crypto_alloc_dev(SP_CRYPTO_RSA);
		if (!rsa.dev || !rsa.dev->reg)
			return -ENODEV;

		init_waitqueue_head(&rsa.wait);
		mutex_init(&rsa.lock);
		rsa.va = dma_alloc_coherent(rsa.dev->device, MAX_RSA_BYTES * 5, &rsa.pa, GFP_KERNEL);
		if (!rsa.va)
			return -ENOMEM;

		struct sp_crypto_reg *reg = rsa.dev->reg;
		W(RSASPTR, rsa.pa + OFFSET_SRC);
		W(RSAYPTR, rsa.pa + OFFSET_EXP);
		W(RSANPTR, rsa.pa + OFFSET_MOD);
		W(RSADPTR, rsa.pa + OFFSET_DST);
		W(RSAP2PTR, rsa.pa + OFFSET_P2);
	}

	return crypto_register_akcipher(&sp_rsa_alg);
}
EXPORT_SYMBOL(sp_rsa_init);

void sp_rsa_finit(void)
{
	crypto_unregister_akcipher(&sp_rsa_alg);
}
EXPORT_SYMBOL(sp_rsa_finit);

void sp_rsa_irq(void *devid, u32 flag)
{
	//SP_CRYPTO_INF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s:%08x\n", __FUNCTION__, flag);
	rsa.wait_flag = SP_CRYPTO_TRUE;
	wake_up(&rsa.wait);
}
EXPORT_SYMBOL(sp_rsa_irq);

static __maybe_unused void hexdump(unsigned char *buf, unsigned int len)
{
	print_hex_dump(KERN_CONT, "", DUMP_PREFIX_OFFSET,
			16, 1,
			buf, len, false);
}

#define MAX_EXTERN_MPI_BITS 16384

/* Modified from lib/crypto/mpi/mpicoder.c:mpi_write_to_sgl() */
static int write_to_sgl(struct scatterlist *sgl, unsigned nbytes, void *a, unsigned int n)
{
	u8 *p, *p2;
#if BYTES_PER_MPI_LIMB == 4
	__be32 alimb;
#elif BYTES_PER_MPI_LIMB == 8
	__be64 alimb;
#else
#error please implement for this limb size.
#endif
	struct sg_mapping_iter miter;
	int i, x, buf_len;
	int nents;
	mpi_limb_t *d = (mpi_limb_t *)a;
	unsigned int nlimbs;

	if (nbytes < n)
		return -EOVERFLOW;

	nents = sg_nents_for_len(sgl, nbytes);
	if (nents < 0)
		return -EINVAL;

	sg_miter_start(&miter, sgl, nents, SG_MITER_ATOMIC | SG_MITER_TO_SG);
	sg_miter_next(&miter);
	buf_len = miter.length;
	p2 = miter.addr;

	while (nbytes > n) {
		i = min_t(unsigned, nbytes - n, buf_len);
		memset(p2, 0, i);
		p2 += i;
		nbytes -= i;

		buf_len -= i;
		if (!buf_len) {
			sg_miter_next(&miter);
			buf_len = miter.length;
			p2 = miter.addr;
		}
	}

	nlimbs = DIV_ROUND_UP(n, BYTES_PER_MPI_LIMB);
	for (i = nlimbs - 1; i >= 0; i--) {
#if BYTES_PER_MPI_LIMB == 4
		alimb = d[i] ? cpu_to_be32(d[i]) : 0;
#elif BYTES_PER_MPI_LIMB == 8
		alimb = d[i] ? cpu_to_be64(d[i]) : 0;
#else
#error please implement for this limb size.
#endif
		p = (u8 *)&alimb;

		for (x = 0; x < sizeof(alimb); x++) {
			*p2++ = *p++;
			if (!--buf_len) {
				sg_miter_next(&miter);
				buf_len = miter.length;
				p2 = miter.addr;
			}
		}
	}

	sg_miter_stop(&miter);
	return 0;
}

/* Modified from lib/crypto/mpi/mpicoder.c:mpi_read_raw_from_sgl() */
static int read_from_sgl(struct scatterlist *sgl, unsigned int nbytes, void *buf)
{
	struct sg_mapping_iter miter;
	unsigned int nbits, nlimbs;
	int x, j, z, lzeros, ents;
	unsigned int len, l;
	const u8 *buff;
	mpi_limb_t a, *d = (mpi_limb_t *)buf;

	ents = sg_nents_for_len(sgl, nbytes);
	if (ents < 0)
		return 0;

	sg_miter_start(&miter, sgl, ents, SG_MITER_ATOMIC | SG_MITER_FROM_SG);

	lzeros = 0;
	len = 0;
	while (nbytes > 0) {
		while (len && !*buff) {
			lzeros++;
			len--;
			buff++;
		}

		if (len && *buff)
			break;

		sg_miter_next(&miter);
		buff = miter.addr;
		len = miter.length;

		nbytes -= lzeros;
		lzeros = 0;
	}

	miter.consumed = lzeros;

	nbytes -= lzeros;
	nbits = nbytes * 8;
	if (nbits > MAX_EXTERN_MPI_BITS) {
		sg_miter_stop(&miter);
		pr_info("MPI: mpi too large (%u bits)\n", nbits);
		return 0;
	}

	if (nbytes > 0)
		nbits -= count_leading_zeros(*buff) - (BITS_PER_LONG - 8);

	sg_miter_stop(&miter);

	if (nbytes) {
		nlimbs = DIV_ROUND_UP(nbytes, BYTES_PER_MPI_LIMB);
		j = nlimbs - 1;
		a = 0;
		z = BYTES_PER_MPI_LIMB - nbytes % BYTES_PER_MPI_LIMB;
		z %= BYTES_PER_MPI_LIMB;
		l = nbytes;

		while (sg_miter_next(&miter)) {
			buff = miter.addr;
			len = min_t(unsigned, miter.length, l);
			l -= len;

			for (x = 0; x < len; x++) {
				a <<= 8;
				a |= *buff++;
				if (((z + x + 1) % BYTES_PER_MPI_LIMB) == 0) {
					d[j--] = a;
					a = 0;
				}
			}
			z += x;
		}
	}

	return nbytes;
}

static int sp_powm(struct akcipher_request *req, MPI n, MPI e)
{
	struct sp_crypto_reg *reg = rsa.dev->reg;
	struct device *dev = rsa.dev->device;
	int ret, nbytes;

	nbytes = mpi_get_size(n);
	if (nbytes > MAX_RSA_BYTES) {
		dev_err(dev, "mod size %d > MAX_RSA_BYTES %d\n", nbytes, MAX_RSA_BYTES);
		return -EINVAL;
	}

	mutex_lock(&rsa.lock);
	memset(rsa.va, 0, OFFSET_MOD);
	ret = read_from_sgl(req->src, req->src_len, rsa.va + OFFSET_SRC);
	if (!ret) {
		mutex_unlock(&rsa.lock);
		return -EINVAL;
	}
	//hexdump(rsa.va + OFFSET_SRC, ret);
	memcpy(rsa.va + OFFSET_EXP, e->d, mpi_get_size(e));

	/* check if the mod is the same */
	if (rsa.nbytes != nbytes ||
		memcmp(rsa.va + OFFSET_MOD, n->d, nbytes)) {
		memcpy(rsa.va + OFFSET_MOD, n->d, nbytes);
		rsa.nbytes = nbytes;

		rsabase_t w = mont_w(n->d, nbytes);
		W(RSAWPTRL, (u32)w);
		W(RSAWPTRH, (u32)(w >> BITS_PER_REG));
		W(RSAPAR0, RSA_SET_PARA_D(nbytes * BITS_PER_BYTE) | RSA_PARA_PRECAL_P2);
	} else {
		W(RSAPAR0, RSA_SET_PARA_D(nbytes * BITS_PER_BYTE) | RSA_PARA_FETCH_P2);
	}

	rsa.wait_flag = SP_CRYPTO_FALSE;
	smp_wmb(); /* memory barrier */

#ifdef RSA_DATA_BIGENDBIAN
	W(RSADMACS, SEC_DMA_SIZE(nbytes) | SEC_DATA_BE | SEC_DMA_ENABLE);
#else
	W(RSADMACS, SEC_DMA_SIZE(nbytes) | SEC_DATA_LE | SEC_DMA_ENABLE);
#endif
	ret = wait_event_interruptible_timeout(rsa.wait, rsa.wait_flag, 30 * HZ);
	if (!ret) {
		dev_err(dev, "wait RSA timeout\n");
		ret = -ETIMEDOUT;
	} else if (ret < 0) {
		dev_err(dev, "wait RSA error: %d\n", ret);
		//rsa.nbytes = 0; // reset
	} else {
		ret = write_to_sgl(req->dst, req->dst_len, rsa.va + OFFSET_DST, nbytes);
	}
	mutex_unlock(&rsa.lock);

	return ret;
}

static int sp_rsa_enc(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	const struct sp_rsa_key *pkey = sp_rsa_get_key(tfm);

	return sp_powm(req, pkey->n, pkey->e);
}

static int sp_rsa_dec(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	const struct sp_rsa_key *pkey = sp_rsa_get_key(tfm);

	return sp_powm(req, pkey->n, pkey->d);
}

static int sp_rsa_set_pub_key(struct crypto_akcipher *tfm, const void *key, unsigned int keylen)
{
	struct sp_rsa_key *mpi_key = sp_rsa_get_key(tfm);
	struct rsa_key raw_key = {0};
	int ret;

	/* Free the old MPI key if any */
	sp_rsa_free_key(mpi_key);

	ret = rsa_parse_pub_key(&raw_key, key, keylen);
	if (ret)
		return ret;

	mpi_key->e = mpi_read_raw_data(raw_key.e, raw_key.e_sz);
	if (!mpi_key->e)
		goto err;

	mpi_key->n = mpi_read_raw_data(raw_key.n, raw_key.n_sz);
	if (!mpi_key->n)
		goto err;

	return 0;

err:
	sp_rsa_free_key(mpi_key);
	return -ENOMEM;
}

static int sp_rsa_set_priv_key(struct crypto_akcipher *tfm, const void *key, unsigned int keylen)
{
	struct sp_rsa_key *mpi_key = sp_rsa_get_key(tfm);
	struct rsa_key raw_key = {0};
	int ret;

	/* Free the old MPI key if any */
	sp_rsa_free_key(mpi_key);

	ret = rsa_parse_priv_key(&raw_key, key, keylen);
	if (ret)
		return ret;

	mpi_key->d = mpi_read_raw_data(raw_key.d, raw_key.d_sz);
	if (!mpi_key->d)
		goto err;

	mpi_key->e = mpi_read_raw_data(raw_key.e, raw_key.e_sz);
	if (!mpi_key->e)
		goto err;

	mpi_key->n = mpi_read_raw_data(raw_key.n, raw_key.n_sz);
	if (!mpi_key->n)
		goto err;

	mpi_key->p = mpi_read_raw_data(raw_key.p, raw_key.p_sz);
	if (!mpi_key->p)
		goto err;

	mpi_key->q = mpi_read_raw_data(raw_key.q, raw_key.q_sz);
	if (!mpi_key->q)
		goto err;

	mpi_key->dp = mpi_read_raw_data(raw_key.dp, raw_key.dp_sz);
	if (!mpi_key->dp)
		goto err;

	mpi_key->dq = mpi_read_raw_data(raw_key.dq, raw_key.dq_sz);
	if (!mpi_key->dq)
		goto err;

	mpi_key->qinv = mpi_read_raw_data(raw_key.qinv, raw_key.qinv_sz);
	if (!mpi_key->qinv)
		goto err;

	return 0;

err:
	sp_rsa_free_key(mpi_key);
	return -ENOMEM;
}

static unsigned int sp_rsa_max_size(struct crypto_akcipher *tfm)
{
	struct sp_rsa_key *pkey = sp_rsa_get_key(tfm);

	return mpi_get_size(pkey->n);
}

static void sp_rsa_exit_tfm(struct crypto_akcipher *tfm)
{
	struct sp_rsa_key *pkey = sp_rsa_get_key(tfm);

	sp_rsa_free_key(pkey);
}

MODULE_ALIAS_CRYPTO("rsa");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sunplus RSA hardware acceleration");
