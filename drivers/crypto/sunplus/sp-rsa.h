/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SP_RSA_H__
#define __SP_RSA_H__

#include <linux/mpi.h>

//#define RSA_DATA_BIGENDBIAN

#define RSA_BASE   64
#define RSA_BASE_MASK ((1 << (RSA_BASE - 2)) - 1)

#if (RSA_BASE == 32)
#define SP_RSA_LB   5
#define rsabase_t   s32

#define sp_rsabase_le_to_cpu __le32_to_cpu
#define sp_rsabase_be_to_cpu __be32_to_cpu

#elif (RSA_BASE == 64)
#define SP_RSA_LB   6
#define rsabase_t   s64

#define sp_rsabase_le_to_cpu __le64_to_cpu
#define sp_rsabase_be_to_cpu __be64_to_cpu
#endif

void sp_rsa_finit(void);
int sp_rsa_init(void);
void sp_rsa_irq(void *devid, u32 flag);
#endif //__SP_RSA_H__
