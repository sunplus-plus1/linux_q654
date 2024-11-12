/* SPDX-License-Identifier: GPL-2.0
 * ALSA SoC SP7350 pcm driver
 * Author: ChingChou Huang <chingchouhuang@sunplus.com>
 */
#ifndef _SPSOC_PCM_H
#define _SPSOC_PCM_H

#include <linux/interrupt.h>

#define DRAM_PCM_BUF_LENGTH	(128 * 1024)

#define PERIOD_BYTES_MIN_CONS	128
#define PERIOD_BYTES_MAX_CONS	(64 * 1024)

#define NUM_FIFO_TX		8 // A0~A4, A20, A26~A27
#define NUM_FIFO_RX		8 // A22~A25, A14, A17~A18, A21
#define NUM_FIFO		(NUM_FIFO_TX + NUM_FIFO_RX)

#define SP_I2S_0                0
#define SP_TDM                  1
#define SP_I2S_1                2
#define SP_I2S_2                3
#define SP_SPDIF                4
#define SP_OTHER                4
#define I2S_P_INC0		(0x1 << 0)//0x1f
#define I2S_P_INC1		(0x1 << 6)
#define I2S_P_INC2		(0x1 << 19)
#define I2S_C_INC0		(0x1 << 11)
#define I2S_C_INC1		(0x1 << 16)
#define I2S_C_INC2		(0x1 << 10)
#define TDMPDM_C_INC0		((0x1 << 14) | (0x3 << 17) | (0x1 << 21) | (0xf << 22))
#define TDM_P_INC0		((0x1 << 20) | (0x3 << 26) | 0x1f)
#define SPDIF_P_INC0		(0x1 << 5)
#define SPDIF_C_INC0		(0x1 << 13)

#define aud_enable_i2stdm_p	(0x01 | (0x1df << 16))
#define aud_enable_i2s1_p	(0x01 << 13)
#define aud_enable_i2s2_p	(0x01 << 15)
//#define aud_enable_i2s_c	(0x01 << 11)
#define aud_enable_i2s0_c	(0x01 << 3)
#define aud_enable_i2s1_c	(0x01 << 11)
#define aud_enable_i2s2_c	(0x01 << 5)
#define aud_enable_spdiftx0_p	(0x01 << 1)
#define aud_enable_spdif_c	(0x01 << 6)
//#define aud_enable_tdm_p	(0x01 | (0x5f<<16))
#define aud_enable_tdmpdm_c	(0x01 << 12)

#define aud_test_mode		(0)
#define chktimeout		1000
#define DRAM_HDMI_BUF_LENGTH	(DRAM_PCM_BUF_LENGTH * 4)

struct spsoc_runtime_data {
	//spinlock_t	lock; //lock
	dma_addr_t	dma_buffer; /* physical address of dma buffer */
	dma_addr_t	dma_buffer_end;	/* first address beyond DMA buffer */
	size_t		period_size;

	struct		hrtimer hrt;
	struct		tasklet_struct tasklet;
	int		poll_time_ns;
	int		timeoutcount;
	struct		snd_pcm_substream *substream;
	int		period;
	int		periods;
	unsigned int	offset;
	unsigned int	last_offset;
	unsigned int	speed;
	unsigned int	size;
	unsigned char	trigger_flag;
	unsigned int	start_threshold;
	unsigned char	usemmap_flag;
	//unsigned char	start_oncetime;
	unsigned int	last_remainder;
	unsigned int	fifosize_from_user;
	atomic_t	running;
};

/*--------------------------------------------------------------------------
 *			IOCTL Command
 *--------------------------------------------------------------------------
 */
typedef struct  t_AUD_FIFO_PARAMS {
	//unsigned int en_flag;	// enable or disable
	//unsigned int fifo_status;
	// A0~A4
	unsigned long pcmtx_virt_base; // audhw_ya (virtual address)
	dma_addr_t pcmtx_phys_base; // audhw_ya (physical address)
	//unsigned int pcmtx_length;
	// IEC0
	//unsigned long iec0tx_virtAddrBase;
	//dma_addr_t iec0tx_physAddrBase;
	//unsigned int iec0tx_length;
	// IEC1
	//unsigned long iec1tx_virtAddrBase;
	//dma_addr_t iec1tx_physAddrBase;
	//unsigned int iec1tx_length;
	//
	unsigned long mic_virt_base;
	dma_addr_t mic_phys_base;
	//unsigned int mic_length;
	// total length
	unsigned int txbuf_len;
	unsigned int rxbuf_len;
	unsigned int buf_total_len;
} AUD_FIFO_PARAMS;

typedef struct t_auddrv_param {
	AUD_FIFO_PARAMS	fifo_info;
} auddrv_param;

extern auddrv_param aud_param;
#endif /* _SPSOC_PCM_H */
