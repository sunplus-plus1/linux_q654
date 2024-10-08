/* SPDX-License-Identifier: GPL-2.0
 * ALSA SoC SP7350 pcm driver
 * Author: ChingChou Huang <chingchouhuang@sunplus.com>
 */
#ifndef CYGONCE_HAL_TYPE_H
#define CYGONCE_HAL_TYPE_H

typedef unsigned char		__u8;
typedef unsigned short		__u16;
typedef unsigned int		__u32;
typedef unsigned long long	__u64;
typedef signed char		__s8;
typedef signed short		__s16;
typedef signed int		__s32;
typedef signed long long	__s64;

#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif
#define	BIG_ENDIAN		4321

#ifndef NULL
#define NULL	((void *)0)
#endif

#endif // CYGONCE_HAL_TYPE_H

