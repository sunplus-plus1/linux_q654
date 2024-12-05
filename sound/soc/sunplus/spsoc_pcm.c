// SPDX-License-Identifier: GPL-2.0
// ALSA SoC SP7350 pcm driver
//
// Author: ChingChou Huang <chingchouhuang@sunplus.com>
//

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <sound/pcm_params.h>
#include "spsoc_pcm.h"
#include "spsoc_util.h"
#include "aud_hw.h"

void __iomem *pcmaudio_base;
//--------------------------------------------------------------------------
//		Feature definition
//--------------------------------------------------------------------------
#define USE_KELNEL_MALLOC

//--------------------------------------------------------------------------
//		Hardware definition	/Data structure
//--------------------------------------------------------------------------
static const struct snd_pcm_hardware spsoc_pcm_hardware = {
	.info = SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID | SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_BATCH,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.period_bytes_min	= PERIOD_BYTES_MIN_CONS,
	.period_bytes_max	= PERIOD_BYTES_MAX_CONS,
	.periods_min		= 2,
	.periods_max		= 4,
	.buffer_bytes_max	= DRAM_PCM_BUF_LENGTH,
	.channels_min		= 2,
	.channels_max		= 16,
};

//--------------------------------------------------------------------------
//		Global Parameters
//--------------------------------------------------------------------------
struct sunplus_audio_base *spauddata = NULL;
auddrv_param aud_param;
static DEFINE_SPINLOCK(set_lock);

#if (MMAP_IRQ)
static irqreturn_t aud_fifo_int(int irq, void *dev)
{
	volatile register_audio *regs0 = (volatile register_audio *)pcmaudio_base;
	struct spsoc_runtime_data *prtd = dev;
	u32 value;

	/* clear interrupt flag */
	value = readl(&regs0->G019_RESERVED[8]);
	pr_debug("int 0x%x msk 0x%x\n",value, regs0->G019_RESERVED[10]);
	if (value & prtd->irq_num) {
	//if (regs0->G019_RESERVED[8] & prtd->irq_num) {
		prtd->irq_done = 1;
		value = readl(&regs0->G019_RESERVED[10]) | prtd->irq_num;
		writel(value, &regs0->G019_RESERVED[10]);
		//regs0->G019_RESERVED[10] |= prtd->irq_num;

		tasklet_hi_schedule(&prtd->tasklet);
	}

	return IRQ_HANDLED;
}
#endif

void run_start(int ch, int run_length, struct spsoc_runtime_data *pprtd)
{
	volatile register_audio *regs0 = (volatile register_audio *)pcmaudio_base;
	u32 value;

	//printk("ch 0x%x, val 0x%x 0x%x\n", ch, run_length, iprtd->period);
	//spin_lock(&set_lock);
	writel(run_length, &regs0->aud_delta_0);
	value = readl(&regs0->aud_inc_0);
	writel(value | ch, &regs0->aud_inc_0);
#if (MMAP_IRQ)
	u32 temp_val;

	if (pprtd->irq_done) {
		switch (ch) {
		case I2S_P_INC0:
			value = run_length > 512 ? 40 : run_length / 40;
			temp_val = value | (readl(&regs0->G019_RESERVED[0]) & 0xffff0000);
			writel(temp_val, &regs0->G019_RESERVED[0]);
			pr_debug("0 [0] 0x%x\n", temp_val);
			break;
		case I2S_C_INC0:
			if (regs0->aud_a11_cnt <= pprtd->period) {
				temp_val = (pprtd->period / 4) |
					   (readl(&regs0->G019_RESERVED[5]) & 0xffff0000);
				//regs0->G019_RESERVED[5] |= pprtd->period / 4;
			} else {
				temp_val = (run_length / 4) |
					   (readl(&regs0->G019_RESERVED[5]) & 0xffff0000);
				//regs0->G019_RESERVED[5] |= run_length / 4;
			}
			writel(temp_val, &regs0->G019_RESERVED[5]);
			pr_debug("0 a11_cnt 0x%x [5] 0x%x\n", regs0->aud_a11_cnt, temp_val);
			break;
		default:
			break;
		}
		//regs0->G019_RESERVED[8] = pprtd->irq_num;
		writel(pprtd->irq_num, &regs0->G019_RESERVED[8]);
		value = 0;
		do {
			value++;
			temp_val = readl(&regs0->G019_RESERVED[8]) & pprtd->irq_num;
		} while (temp_val && (value < 5));

		if (value < 5) {
			pprtd->irq_done = 0;
			temp_val = readl(&regs0->G019_RESERVED[10]) & (~pprtd->irq_num);
			writel(temp_val, &regs0->G019_RESERVED[10]);
			//regs0->G019_RESERVED[10] &= ~pprtd->irq_num;
		} else {
			pr_debug("@@@@@ no op int [8] 0x%x, 0x%x @@@@@\n", regs0->G019_RESERVED[8],
				 temp_val);
			tasklet_hi_schedule(&pprtd->tasklet);
		}

		pr_debug("1 [8] 0x%x [10] 0x%x\n", regs0->G019_RESERVED[8], regs0->G019_RESERVED[10]);
	}
#endif
	//spin_unlock(&set_lock);
}
//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
static void hrtimer_pcm_tasklet(unsigned long priv)
{
	struct spsoc_runtime_data *iprtd = (struct spsoc_runtime_data *)priv;
	struct snd_pcm_substream *substream = iprtd->substream;
	//struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int delta, appl_ofs, tout = 0;
	volatile register_audio *regs0 = pcmaudio_base;
	unsigned long flags;

	if (!atomic_read(&iprtd->running))
		return;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (substream->pcm->device == SP_SPDIF)
			iprtd->offset = regs0->aud_a5_ptr & 0xfffffc;
		else if (substream->pcm->device == SP_I2S_1)
			iprtd->offset = regs0->aud_a6_ptr & 0xfffffc;
		else if (substream->pcm->device == SP_I2S_2)
			iprtd->offset = regs0->aud_a19_ptr & 0xfffffc;
		else
			iprtd->offset = regs0->aud_a0_ptr & 0xfffffc;

		pr_debug("P:?_ptr=0x%x\n", iprtd->offset);
		if (iprtd->offset < iprtd->fifosize_from_user) {
			if (iprtd->usemmap_flag == 1) {
				spin_lock_irqsave(&set_lock, flags);
				if (substream->pcm->device == SP_I2S_0) {
					pr_debug("regs0->aud_a0_cnt 0x%x\n", regs0->aud_a0_cnt);
					while (regs0->aud_a0_cnt != 0 && tout < iprtd->timeoutcount) {
						udelay(1);
						tout++;
						//schedule_timeout_interruptible(5);
					}
					run_start(I2S_P_INC0, iprtd->period, iprtd);
					pr_debug("regs0->aud_a0_cnt 0x%x tout %d\n", regs0->aud_a0_cnt, tout);
					iprtd->offset = regs0->aud_a0_ptr & 0xfffffc;
				} else if (substream->pcm->device == SP_I2S_1) {
					while (regs0->aud_a6_cnt != 0 && tout < iprtd->timeoutcount) {
						udelay(1);
						tout++;
					}
					run_start(I2S_P_INC1, iprtd->period, iprtd);
					iprtd->offset = regs0->aud_a6_ptr & 0xfffffc;
				} else if (substream->pcm->device == SP_I2S_2) {
					while (regs0->aud_a19_cnt != 0 && tout < iprtd->timeoutcount) {
						udelay(1);
						tout++;
					}
					run_start(I2S_P_INC2, iprtd->period, iprtd);
					iprtd->offset = regs0->aud_a19_ptr & 0xfffffc;
				} else if (substream->pcm->device == SP_TDM) {
					while (regs0->aud_a0_cnt != 0 && tout < iprtd->timeoutcount) {
						udelay(1);
						tout++;
					}
					run_start(TDM_P_INC0, iprtd->period, iprtd);
					iprtd->offset = regs0->aud_a0_ptr & 0xfffffc;
				} else if (substream->pcm->device == SP_SPDIF) {
//pr_debug("***%s IN, aud_a5_ptr=0x%x, dma_area=0x%x, pos=0x%lx count_bytes 0x%x\n", __func__,
//	   regs0->aud_a5_ptr, hwbuf, pos, count_bytes);
					while (regs0->aud_a5_cnt != 0 && tout < iprtd->timeoutcount) {
						udelay(1);
						tout++;
					}
					run_start(SPDIF_P_INC0, iprtd->period, iprtd);
					iprtd->offset = regs0->aud_a5_ptr & 0xfffffc;
				}
				spin_unlock_irqrestore(&set_lock, flags);
				if (tout >= iprtd->timeoutcount)
					pr_err("XXX hrtimer_pcm_tasklet TIMEOUT\n");
			} else {
				if ((iprtd->offset % iprtd->period) != 0) {
					appl_ofs = (iprtd->offset + (iprtd->period >> 2)) /
						   iprtd->period;
					if (appl_ofs < iprtd->periods)
						iprtd->offset = iprtd->period * appl_ofs;
					else
						iprtd->offset = 0; //iprtd->period * appl_ofs;
				}
			}
		}
		// If we've transferred at least a period then report it and reset our poll time

		//if (delta >= iprtd->period )  //ending normal
		//{
			//pr_debug("a0_ptr=0x%08x\n",iprtd->offset);
		iprtd->last_offset = iprtd->offset;
		snd_pcm_period_elapsed(substream);
		//}
	} else {
		if (substream->pcm->device == SP_I2S_0) // i2s //need to check
			iprtd->offset = regs0->aud_a11_ptr & 0xfffffc;
		else if (substream->pcm->device == SP_I2S_1) // i2s
			iprtd->offset = regs0->aud_a16_ptr & 0xfffffc;
		else if (substream->pcm->device == SP_I2S_2) // i2s
			iprtd->offset = regs0->aud_a10_ptr & 0xfffffc;
		else if (substream->pcm->device == SP_SPDIF) // spdif0
			iprtd->offset = regs0->aud_a13_ptr & 0xfffffc;
		else // tdm, pdm
			iprtd->offset = regs0->aud_a22_ptr & 0xfffffc;

		pr_debug("C:?_ptr=0x%x - 0x%x cnt_a11 0x%x ", iprtd->offset, iprtd->last_offset,
			 regs0->aud_a11_cnt);
		if (iprtd->usemmap_flag == 1) {
			spin_lock_irqsave(&set_lock, flags);
			if (iprtd->offset >= iprtd->last_offset)
				delta = iprtd->offset - iprtd->last_offset;
			else
				delta = iprtd->size + iprtd->offset - iprtd->last_offset;

			if (delta > 0) { //ending normal
				if (substream->pcm->device == SP_I2S_0)
					run_start(I2S_C_INC0, delta, iprtd);
				else if (substream->pcm->device == SP_I2S_1)
					run_start(I2S_C_INC1, delta, iprtd);
				else if (substream->pcm->device == SP_I2S_2)
					run_start(I2S_C_INC2, delta, iprtd);
				else if (substream->pcm->device == SP_SPDIF)
					run_start(SPDIF_C_INC0, delta, iprtd);
				else
					run_start(TDMPDM_C_INC0, delta, iprtd);
				pr_debug("C? inc 0x%x\n", delta);
			}
			spin_unlock_irqrestore(&set_lock, flags);
		}
#if (MMAP_IRQ)
		else {
			if ((iprtd->offset % iprtd->period) != 0) {
				appl_ofs = (iprtd->offset + (iprtd->period >> 2)) /
					   iprtd->period;
				if (appl_ofs < iprtd->periods)
					iprtd->offset = iprtd->period * appl_ofs;
				else
					iprtd->offset = 0; //iprtd->period * appl_ofs;
			}
		}
#endif
		iprtd->last_offset = iprtd->offset;
		snd_pcm_period_elapsed(substream);
	}
}

#if (!MMAP_IRQ)
static enum hrtimer_restart snd_hrtimer_callback(struct hrtimer *hrt)
{
	struct spsoc_runtime_data *iprtd = container_of(hrt, struct spsoc_runtime_data, hrt);

	pr_debug("%s %s-run %d\n", __func__, iprtd->substream->stream ? "c" : "p",
		 atomic_read(&iprtd->running));
	//if (!atomic_read(&iprtd->running))
	if (atomic_read(&iprtd->running) == 2) {
		pr_debug("cancel htrimer !!!\n");
		atomic_set(&iprtd->running, 0);
		return HRTIMER_NORESTART;
	}

	tasklet_hi_schedule(&iprtd->tasklet);
	//hrtimer_pcm_tasklet((unsigned long)iprtd);
	hrtimer_forward_now(hrt, ns_to_ktime(iprtd->poll_time_ns));

	return HRTIMER_RESTART;
}
#endif

//--------------------------------------------------------------------------
//		ASoC platform driver
//--------------------------------------------------------------------------
static int spsoc_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = DRAM_PCM_BUF_LENGTH;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		buf->area = (unsigned char *)aud_param.fifo_info.pcmtx_virt_base;
		buf->addr = aud_param.fifo_info.pcmtx_phys_base;
	}

	if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		buf->area = (unsigned char *)aud_param.fifo_info.mic_virt_base;
		buf->addr = aud_param.fifo_info.mic_phys_base;
	}

	buf->bytes = DRAM_PCM_BUF_LENGTH;
	if (!buf->area) {
		pr_err("Failed to allocate dma memory, please increase uncached DMA memory region\n");
		return -ENOMEM;
	}
	pr_debug("preallocate_dma_buffer %s %d: area=%p, addr=%llx, size=%ld\n", substream->name,
		 stream, buf->area, buf->addr, size);

	return 0;
}

void hw_test(void)
{
	unsigned int pcmdata[96], regtemp, regtemp1, regtemp2, regtemp3, regtemp4;
	int i, j, val, run_num = 50, run_length = 384, tout;
	unsigned char *buf;
	volatile register_audio *regs0 = pcmaudio_base;

	pcmdata[0] = 0x00000000;
	pcmdata[1] = 0x000018f9;
	pcmdata[2] = 0x000c7c80;
	pcmdata[3] = 0x30fb0018;
	pcmdata[4] = 0x7d80471c;
	pcmdata[5] = 0x00238e00;
	pcmdata[6] = 0x5a82002d;
	pcmdata[7] = 0x41006a6d;
	pcmdata[8] = 0x00353680;
	pcmdata[9] = 0x7641003b;
	pcmdata[10] = 0x20807d8a;
	pcmdata[11] = 0x003ec500;
	pcmdata[12] = 0x7fff003f;
	pcmdata[13] = 0xff807d8b;
	pcmdata[14] = 0x003ec580;
	pcmdata[15] = 0x7642003b;
	pcmdata[16] = 0x21006a6f;
	pcmdata[17] = 0x00353780;
	pcmdata[18] = 0x5a84002d;
	pcmdata[19] = 0x4200471f;
	pcmdata[20] = 0x00238f80;
	pcmdata[21] = 0x30fe0018;
	pcmdata[22] = 0x7f0018fc;
	pcmdata[23] = 0x000c7e00;
	pcmdata[24] = 0x00030000;
	pcmdata[25] = 0x0180e70a;
	pcmdata[26] = 0x00f38500;
	pcmdata[27] = 0xcf0700e7;
	pcmdata[28] = 0x8380b8e6;
	pcmdata[29] = 0x00dc7300;
	pcmdata[30] = 0xa58000d2;
	pcmdata[31] = 0xc0009595;
	pcmdata[32] = 0x00caca80;
	pcmdata[33] = 0x89c000c4;
	pcmdata[34] = 0xe0008276;
	pcmdata[35] = 0x00c13b00;
	pcmdata[36] = 0x800000c0;
	pcmdata[37] = 0x00008275;
	pcmdata[38] = 0x00c13a80;
	pcmdata[39] = 0x89bc00c4;
	pcmdata[40] = 0xde009590;
	pcmdata[41] = 0x00cac800;
	pcmdata[42] = 0xa57a00d2;
	pcmdata[43] = 0xbd00b8de;
	pcmdata[44] = 0x00dc6f00;
	pcmdata[45] = 0xceff00e7;
	pcmdata[46] = 0x7f80e702;
	pcmdata[47] = 0x00f38100;
	pcmdata[48] = 0x00000000;
	pcmdata[49] = 0x000018f9;
	pcmdata[50] = 0x000c7c80;
	pcmdata[51] = 0x30fb0018;
	pcmdata[52] = 0x7d80471c;
	pcmdata[53] = 0x00238e00;
	pcmdata[54] = 0x5a82002d;
	pcmdata[55] = 0x41006a6d;
	pcmdata[56] = 0x00353680;
	pcmdata[57] = 0x7641003b;
	pcmdata[58] = 0x20807d8a;
	pcmdata[59] = 0x003ec500;
	pcmdata[60] = 0x7fff003f;
	pcmdata[61] = 0xff807d8b;
	pcmdata[62] = 0x003ec580;
	pcmdata[63] = 0x7642003b;
	pcmdata[64] = 0x21006a6f;
	pcmdata[65] = 0x00353780;
	pcmdata[66] = 0x5a84002d;
	pcmdata[67] = 0x4200471f;
	pcmdata[68] = 0x00238f80;
	pcmdata[69] = 0x30fe0018;
	pcmdata[70] = 0x7f0018fc;
	pcmdata[71] = 0x000c7e00;
	pcmdata[72] = 0x00030000;
	pcmdata[73] = 0x0180e70a;
	pcmdata[74] = 0x00f38500;
	pcmdata[75] = 0xcf0700e7;
	pcmdata[76] = 0x8380b8e6;
	pcmdata[77] = 0x00dc7300;
	pcmdata[78] = 0xa58000d2;
	pcmdata[79] = 0xc0009595;
	pcmdata[80] = 0x00caca80;
	pcmdata[81] = 0x89c000c4;
	pcmdata[82] = 0xe0008276;
	pcmdata[83] = 0x00c13b00;
	pcmdata[84] = 0x800000c0;
	pcmdata[85] = 0x00008275;
	pcmdata[86] = 0x00c13a80;
	pcmdata[87] = 0x89bc00c4;
	pcmdata[88] = 0xde009590;
	pcmdata[89] = 0x00cac800;
	pcmdata[90] = 0xa57a00d2;
	pcmdata[91] = 0xbd00b8de;
	pcmdata[92] = 0x00dc6f00;
	pcmdata[93] = 0xceff00e7;
	pcmdata[94] = 0x7f80e702;
	pcmdata[95] = 0x00f38100;

	pr_info("=== AUDIO I2S0 -> I2S2 test start ===\n");

	regtemp  = regs0->pcm_cfg;
	regtemp1 = regs0->ext_adc_cfg;
	regtemp2 = regs0->hdmi_tx_i2s_cfg;
	regtemp3 = regs0->hdmi_rx_i2s_cfg;
	regtemp4 = regs0->int_adc_dac_cfg;

	regs0->pcm_cfg		= 0x71; //tx0
	regs0->ext_adc_cfg	= 0x71; //rx0
	regs0->hdmi_tx_i2s_cfg	= 0x271; //tx2
	regs0->hdmi_rx_i2s_cfg	= 0x271; //rx2
	regs0->int_adc_dac_cfg	= 0x02710271;
	//rx1 tx1, if RI2S_0 tx1(slave) -> rx0 -> tx2/tx0 0x004d024d

	regs0->aud_ext_dac_xck_cfg	= 0x6883;
	//is20 PLLA 147M, 2 chs 64 bits  48k = 147M/12(3/4 xck)/4(bck)/64
	regs0->aud_ext_dac_bck_cfg	= 0x6007;
	regs0->aud_int_dac_xck_cfg	= 0x688f; //i2s1
	regs0->aud_int_dac_bck_cfg	= 0x6001;
	regs0->aud_hdmi_tx_mclk_cfg	= 0x6883; //i2s2
	regs0->aud_hdmi_tx_bck_cfg	= 0x6007;

	regs0->aud_audhwya = aud_param.fifo_info.pcmtx_phys_base;
	regs0->aud_a0_base = 0;
	regs0->aud_a0_length = DRAM_PCM_BUF_LENGTH;

	regs0->aud_a10_base = DRAM_PCM_BUF_LENGTH * NUM_FIFO_TX;
	regs0->aud_a16_base = regs0->aud_a10_base + 2 * DRAM_PCM_BUF_LENGTH;
	regs0->aud_a10_length = 2 * DRAM_PCM_BUF_LENGTH;
	regs0->aud_a16_length = 2 * DRAM_PCM_BUF_LENGTH;

	val = regs0->aud_fifo_enable;
	val |= I2S_P_INC0 | I2S_C_INC1 | I2S_C_INC2; //| I2S_P_INC1 | I2S_P_INC2 | I2S_C_INC0;
	regs0->aud_fifo_enable = val;
	regs0->aud_fifo_reset = val;
	tout = 0;
	while (regs0->aud_fifo_reset && tout > chktimeout)
		tout++;
	if (tout >= chktimeout)
		pr_err("XXX hw_test TIMEOUT 1\n");
	memset((void *)aud_param.fifo_info.pcmtx_virt_base, 0, DRAM_PCM_BUF_LENGTH);
	memset((void *)aud_param.fifo_info.mic_virt_base, 0, 4 * DRAM_PCM_BUF_LENGTH);
	for (j = 0; j < run_num; j++)
		memcpy((void *)aud_param.fifo_info.pcmtx_virt_base + j * run_length, pcmdata,
		       run_length);
//buf = (unsigned char *)aud_param.fifo_info.mic_virt_base;
//for(j = 0; j < 96; j++)
//	printk("0x%02x%02x%02x%02x\n", *(buf + j * 4 + 3), *(buf + j * 4 + 2), *(buf + j * 4 + 1),
// *(buf + j * 4));

	val = regs0->aud_enable;
	val |= aud_enable_i2stdm_p | aud_enable_i2s1_c | aud_enable_i2s2_c;
	regs0->aud_enable = val;
	val = 0;
	while (1) {
		if (val++ < run_num) {
			tout = 0;
			while (regs0->aud_inc_0 != 0 && tout > chktimeout)
				tout++;
			regs0->aud_delta_0 = run_length;
			regs0->aud_inc_0 |= I2S_P_INC0;
			tout = 0;
			while (regs0->aud_inc_0 != 0 && tout > chktimeout)
				tout++;
			if (tout >= chktimeout)
				pr_err("XXX hw_test TIMEOUT 2\n");
		} else {
			break;
		}
	}

	val = run_length * run_num;
	i = 0;
	while ((regs0->aud_a10_cnt < val) && (regs0->aud_a16_cnt < val)) {
//printk("p aud_a0_cnt 0x%x 0x%x, a10_cnt 0x%x 0x%x\n", regs0->aud_a0_cnt, regs0->aud_a0_ptr,
//regs0->aud_a10_cnt, regs0->aud_a10_ptr);
		if (regs0->aud_a10_cnt == 0 && regs0->aud_a16_cnt == 0)
			i++;

		if (i > val)
			break;
	};
	// test end
	regs0->aud_fifo_enable &= ~(I2S_P_INC0 | I2S_C_INC1 | I2S_C_INC2);
	regs0->aud_enable = 0;

	val -= run_length;
	for (i = 0; i < 2; i++) {
		if (i == 0) {
			if (regs0->aud_a16_cnt == 0) {
				pr_err("AUDIO I2S1 no connect\n");
				continue;
			}
			buf = (unsigned char *)aud_param.fifo_info.mic_virt_base +
			      2 * DRAM_PCM_BUF_LENGTH;
		} else {
			if (regs0->aud_a10_cnt == 0) {
				pr_err("AUDIO I2S2 no connect\n");
				continue;
			}
			buf = (unsigned char *)aud_param.fifo_info.mic_virt_base;
		}

		for (j = 0; j < val; j++) {
//printk("0x%02x%02x%02x%02x\n", *(buf + j * 4 + 3), *(buf + j * 4 + 2), *(buf + j * 4 + 1),
// *(buf + j * 4));
			if (memcmp(buf + j, (unsigned char *)pcmdata, run_length) == 0)
				break;
		}
		//printk("j = %d\n", j);
		if (j < val) {
//#if 0
//val = j + run_length;
//while (j < val) {
//	pr_info("0x%02x%02x%02x%02x\n", *(buf + j + 3), *(buf + j + 2), *(buf + j + 1), *(buf + j));
//	j += 4;
//}
//#endif
			if (i == 0)
				pr_info("AUDIO I2S0->I2S1 PASS\n");
			else
				pr_info("AUDIO I2S0->I2S2 PASS\n");
		} else {
//#if 0
//val += run_length;
//while (j < val) {
//	pr_info("0x%02x%02x%02x%02x\n", *(buf + j + 3), *(buf + j + 2), *(buf + j + 1), *(buf + j));
//	j += 4;
//}
//#endif
			if (i == 0)
				pr_err("AUDIO I2S0->I2S1 FAIL\n");
			else
				pr_err("AUDIO I2S0->I2S2 FAIL\n");
		}
	}

	regs0->pcm_cfg		= regtemp;
	regs0->ext_adc_cfg	= regtemp1;
	regs0->hdmi_tx_i2s_cfg	= regtemp2;
	regs0->hdmi_rx_i2s_cfg	= regtemp3;
	regs0->int_adc_dac_cfg	= regtemp4;
	regs0->aud_a0_base = DRAM_PCM_BUF_LENGTH * (NUM_FIFO_TX - 1);
	regs0->aud_a10_base = DRAM_PCM_BUF_LENGTH * (NUM_FIFO - 1);
	regs0->aud_a16_base = DRAM_PCM_BUF_LENGTH * (NUM_FIFO - 1);
	regs0->aud_a0_length = 0;
	regs0->aud_a10_length = 0;
	regs0->aud_a16_length = 0;
}

static int spsoc_pcm_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct spsoc_runtime_data *prtd;
	int ret = 0;

	pr_debug("%s IN, %s-devnum: %d\n", __func__, substream->stream ? "cap" : "play",
		 substream->pcm->device);

	if (!IS_ENABLED(CONFIG_SND_SOC_ES8316_SUNPLUS) &&
	    substream->pcm->device == 4 && substream->stream == 1)
		hw_test();

	if (substream->pcm->device > SP_OTHER) {
		pr_info("[sound] device num: %d\n", substream->pcm->device);
		ret = -EINVAL;
		goto out;
	}

	snd_soc_set_runtime_hwparams(substream, &spsoc_pcm_hardware);

	/* Ensure that buffer size is a multiple of period size */
	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		goto out;

	snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 128);

	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (!prtd) {
		//pr_err(" memory get error(spsoc_runtime_data)\n");
		ret = -ENOMEM;
		goto out;
	}

	//spin_lock_init(&prtd->lock);
	runtime->private_data = prtd;
	prtd->substream = substream;
#if (MMAP_IRQ)
	ret = request_irq(spauddata->irq, aud_fifo_int, IRQF_SHARED, "fifo_irq", prtd);
	if (ret)
		pr_err("audio: unable to register IRQ(%d)\n", spauddata->irq);
#else
	hrtimer_init(&prtd->hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	prtd->hrt.function = snd_hrtimer_callback;
#endif
	tasklet_init(&prtd->tasklet, hrtimer_pcm_tasklet, (unsigned long)prtd);
	pr_debug("%s OUT, prtd addr %p\n", __func__, prtd);
out:
	return ret;
}

static int spsoc_pcm_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct spsoc_runtime_data *prtd = substream->runtime->private_data;

	pr_debug("%s IN, %s\n", __func__, substream->stream ? "cap" : "play");
#if (MMAP_IRQ)
	free_irq(spauddata->irq, prtd);
#else
	hrtimer_cancel(&prtd->hrt);
#endif
	kfree(prtd);
	return 0;
}

static int spsoc_pcm_hw_params(struct snd_soc_component *component,
			       struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct spsoc_runtime_data *prtd = runtime->private_data;
	int reserve_buf;
	volatile register_audio *regs0 = pcmaudio_base;

	pr_debug("%s IN, params_rate=%d\n", __func__, params_rate(params));
	pr_debug("area=0x%p, addr=0x%llx, bytes=0x%zx\n", substream->dma_buffer.area,
		 substream->dma_buffer.addr, substream->dma_buffer.bytes);
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	prtd->usemmap_flag = 0;
	//prtd->last_remainder = 0;
	prtd->fifosize_from_user = 0;
	//prtd->last_appl_ofs = 0;

	prtd->dma_buffer = runtime->dma_addr;
	prtd->dma_buffer_end = runtime->dma_addr + runtime->dma_bytes;
	//prtd->period_size = params_period_bytes(params);
	prtd->period_size = params_period_size(params);
	prtd->size = params_buffer_bytes(params);
	prtd->periods = params_periods(params);
	prtd->period = params_period_bytes(params);
	prtd->speed = params_rate(params);
	prtd->offset = 0;
	prtd->last_offset = 0;
	prtd->trigger_flag = 0;
	prtd->start_threshold = 0;
	prtd->irq_num = 0;
	prtd->irq_done = 1;
	atomic_set(&prtd->running, 0);
	pr_debug("prtd->size=0x%x, prtd->periods=0x%x, prtd->period=0x%x, period_size=0x%x\n",
		 prtd->size, prtd->periods, prtd->period, params_period_size(params));

	regs0->aud_audhwya = aud_param.fifo_info.pcmtx_phys_base;
	prtd->fifosize_from_user = prtd->size;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reserve_buf = 480000 / params_rate(params); //10*48000/rate
		prtd->poll_time_ns = div_u64((u64)(prtd->period_size - reserve_buf) *
					     1000000000UL +  prtd->speed - 1, prtd->speed);
//prtd->poll_time_ns =div_u64((u64)params_period_size(params) * 1000000000UL +  96000 - 1, 480000);
		switch (substream->pcm->device) {
		case SP_TDM:
			regs0->aud_a0_base = 0;
			regs0->aud_a1_base = regs0->aud_a0_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a2_base = regs0->aud_a1_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a3_base = regs0->aud_a2_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a4_base = regs0->aud_a3_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a20_base = regs0->aud_a4_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a26_base = regs0->aud_a20_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a27_base = regs0->aud_a26_base + DRAM_PCM_BUF_LENGTH;

			regs0->aud_a0_length = prtd->fifosize_from_user;
			regs0->aud_a1_length = prtd->fifosize_from_user;
			regs0->aud_a2_length = prtd->fifosize_from_user;
			regs0->aud_a3_length = prtd->fifosize_from_user;
			regs0->aud_a4_length = prtd->fifosize_from_user;
			regs0->aud_a20_length = prtd->fifosize_from_user;
			regs0->aud_a26_length = prtd->fifosize_from_user;
			regs0->aud_a27_length = prtd->fifosize_from_user;
			pr_debug("TDM P aud_base 0x%x\n", regs0->aud_a0_base);
			break;
		case SP_I2S_0:
			regs0->aud_a0_base = 0;
			regs0->aud_a0_length = prtd->fifosize_from_user;
			pr_debug("I2S_0 P aud_base 0x%x\n", regs0->aud_a0_base);
			break;
		case SP_I2S_1:
			regs0->aud_a6_base = 0;
			regs0->aud_a6_length = prtd->fifosize_from_user;
			pr_debug("I2S_1 P aud_base 0x%x\n", regs0->aud_a6_base);
			break;
		case SP_I2S_2:
			regs0->aud_a19_base = 0;
			regs0->aud_a19_length = prtd->fifosize_from_user;
			pr_debug("I2S_2 P aud_base 0x%x\n", regs0->aud_a19_base);
			break;
		case SP_SPDIF:
			regs0->aud_a5_base = 0;
			regs0->aud_a5_length = prtd->fifosize_from_user;
			pr_debug("SPDIF P aud_base 0x%x\n", regs0->aud_a5_base);
			break;
		default:
			pr_err("###Wrong device no.\n");
			break;
		}
	} else {
		prtd->poll_time_ns = div_u64((u64)(params_period_size(params)) * 1000000000UL +
					     params_rate(params) - 1, params_rate(params));
		switch (substream->pcm->device) {
		case SP_I2S_0:
			regs0->aud_a11_base = DRAM_PCM_BUF_LENGTH * NUM_FIFO_TX;
			regs0->aud_a11_length = prtd->fifosize_from_user;
			pr_debug("I2S_0 C aud_base 0x%x\n", regs0->aud_a11_base);
			break;
		case SP_I2S_1:
			regs0->aud_a16_base = DRAM_PCM_BUF_LENGTH * NUM_FIFO_TX;
			regs0->aud_a16_length = prtd->fifosize_from_user;
			pr_debug("I2S_1 C aud_base 0x%x\n", regs0->aud_a16_base);
			break;
		case SP_I2S_2:
			regs0->aud_a10_base = DRAM_PCM_BUF_LENGTH * NUM_FIFO_TX;
			regs0->aud_a10_length = prtd->fifosize_from_user;
			pr_debug("I2S_2 C aud_base 0x%x\n", regs0->aud_a10_base);
			break;
		case SP_TDM:
			regs0->aud_a22_base = DRAM_PCM_BUF_LENGTH * NUM_FIFO_TX;
			regs0->aud_a23_base = regs0->aud_a22_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a24_base = regs0->aud_a23_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a25_base = regs0->aud_a24_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a14_base = regs0->aud_a25_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a17_base = regs0->aud_a14_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a18_base = regs0->aud_a17_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a21_base = regs0->aud_a18_base + DRAM_PCM_BUF_LENGTH;
			regs0->aud_a22_length = prtd->fifosize_from_user;
			regs0->aud_a23_length = prtd->fifosize_from_user;
			regs0->aud_a24_length = prtd->fifosize_from_user;
			regs0->aud_a25_length = prtd->fifosize_from_user;
			regs0->aud_a14_length = prtd->fifosize_from_user;
			regs0->aud_a17_length = prtd->fifosize_from_user;
			regs0->aud_a18_length = prtd->fifosize_from_user;
			regs0->aud_a21_length = prtd->fifosize_from_user;
			pr_debug("TDM/PDM C aud_base 0x%x\n", regs0->aud_a22_base);
			break;
		case SP_SPDIF:
			regs0->aud_a13_base = DRAM_PCM_BUF_LENGTH * NUM_FIFO_TX;
			regs0->aud_a13_length = prtd->fifosize_from_user;
			pr_debug("SPDIF C aud_base 0x%x\n", regs0->aud_a13_base);
			break;
		default:
			pr_err("###Wrong device no.\n");
			break;
		}
	}
	prtd->timeoutcount = prtd->poll_time_ns / 1000; //unit:1us
	pr_debug("rate %d, poll_time_ns %d timeoutcount %d\n", params_rate(params),
		 prtd->poll_time_ns, prtd->timeoutcount);
	return 0;
}

static int spsoc_pcm_hw_free(struct snd_soc_component *component,
			     struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct spsoc_runtime_data *iprtd = runtime->private_data;
	int dma_initial;
	volatile register_audio *regs0 = pcmaudio_base;

	snd_pcm_set_runtime_buffer(substream, NULL);
	tasklet_kill(&iprtd->tasklet);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_initial = DRAM_PCM_BUF_LENGTH * (NUM_FIFO_TX - 1);
		switch (substream->pcm->device) {
		case SP_TDM: // tdm
			regs0->aud_a0_base	= dma_initial;
			regs0->aud_a1_base	= dma_initial;
			regs0->aud_a2_base	= dma_initial;
			regs0->aud_a3_base	= dma_initial;
			regs0->aud_a4_base	= dma_initial;
			regs0->aud_a20_base	= dma_initial;
			regs0->aud_a26_base	= dma_initial;
			regs0->aud_a27_base	= dma_initial;

			regs0->aud_a0_length	= 0;
			regs0->aud_a1_length	= 0;
			regs0->aud_a2_length	= 0;
			regs0->aud_a3_length	= 0;
			regs0->aud_a4_length	= 0;
			regs0->aud_a20_length	= 0;
			regs0->aud_a26_length	= 0;
			regs0->aud_a27_length	= 0;
			break;
		case SP_I2S_0: // i2s
			regs0->aud_a0_base	= dma_initial;
			regs0->aud_a0_length	= 0;
			break;
		case SP_I2S_1:
			regs0->aud_a6_base	= dma_initial;
			regs0->aud_a6_length	= 0;
			break;
		case SP_I2S_2:
			regs0->aud_a19_base	= dma_initial;
			regs0->aud_a19_length	= 0;
			break;
		case SP_SPDIF: // spdif0
			regs0->aud_a5_base	= dma_initial;
			regs0->aud_a5_length	= 0;
			break;
		default:
			pr_err("###Wrong device no.\n");
			break;
		}
		memset((void *)aud_param.fifo_info.pcmtx_virt_base, 0,
		       aud_param.fifo_info.txbuf_len);
	} else {
		dma_initial = DRAM_PCM_BUF_LENGTH * (NUM_FIFO - 1);
		switch (substream->pcm->device) {
		case SP_I2S_0: // i2s
			regs0->aud_a11_base	= dma_initial;
			regs0->aud_a11_length	= 0;
			break;
		case SP_I2S_1:
			regs0->aud_a16_base	= dma_initial;
			regs0->aud_a16_length	= 0;
			break;
		case SP_I2S_2:
			regs0->aud_a10_base	= dma_initial;
			regs0->aud_a10_length	= 0;
			break;
		case SP_TDM: // tdm
			regs0->aud_a22_base	= dma_initial;
			regs0->aud_a23_base	= dma_initial;
			regs0->aud_a24_base	= dma_initial;
			regs0->aud_a25_base	= dma_initial;
			regs0->aud_a14_base	= dma_initial;
			regs0->aud_a17_base	= dma_initial;
			regs0->aud_a18_base	= dma_initial;
			regs0->aud_a21_base	= dma_initial;
			regs0->aud_a22_length	= 0;
			regs0->aud_a23_length	= 0;
			regs0->aud_a24_length	= 0;
			regs0->aud_a25_length	= 0;
			regs0->aud_a14_length	= 0;
			regs0->aud_a17_length	= 0;
			regs0->aud_a18_length	= 0;
			regs0->aud_a21_length	= 0;
			break;
		case SP_SPDIF: // spdif0
			regs0->aud_a13_base	= dma_initial;
			regs0->aud_a13_length	= 0;
			break;
		default:
			pr_err("###Wrong device no.\n");
			break;
		}
		memset((void *)aud_param.fifo_info.pcmtx_virt_base, 0,
		       aud_param.fifo_info.rxbuf_len);
	}

	pr_debug("%s IN, %s-device=%d\n", __func__, substream->stream ? "cap" : "play",
		 substream->pcm->device);
	return 0;
}

static int spsoc_pcm_prepare(struct snd_soc_component *component,
			     struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct spsoc_runtime_data *iprtd = runtime->private_data;
	volatile register_audio *regs0 = pcmaudio_base;
	int tout;

	pr_debug("%s IN, buffer_size=0x%lx %s-dev %d\n", __func__, runtime->buffer_size,
		 substream->stream ? "cap" : "play", substream->pcm->device);
	pr_debug("runtime->period_size=0x%lx, runtime->periods=0x%x\n", runtime->period_size,
		 runtime->periods);
	//tasklet_kill(&iprtd->tasklet);
	iprtd->offset = 0;
	iprtd->last_offset = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		tout = 0;
		switch (substream->pcm->device) {
		case SP_I2S_0:
			iprtd->irq_num = 0x1 << 0;
			regs0->aud_fifo_reset |= I2S_P_INC0;
			while ((regs0->aud_fifo_reset & I2S_P_INC0) != 0 && tout < chktimeout)
				tout++;
			break;
		case SP_I2S_1:
			regs0->aud_fifo_reset |= I2S_P_INC1;
			while ((regs0->aud_fifo_reset & I2S_P_INC1) != 0 && tout < chktimeout)
				tout++;
			break;
		case SP_I2S_2:
			regs0->aud_fifo_reset |= I2S_P_INC2;
			while ((regs0->aud_fifo_reset & I2S_P_INC2) != 0 && tout < chktimeout)
				tout++;
			break;
		case SP_SPDIF:
			regs0->aud_fifo_reset |= SPDIF_P_INC0;
			while ((regs0->aud_fifo_reset & SPDIF_P_INC0) != 0 && tout < chktimeout)
				tout++;
			break;
		case SP_TDM:
			break;
		default:
			pr_err("###Wrong device no.\n");
			break;
		}
	} else { //if( substream->stream == SNDRV_PCM_STREAM_CAPTURE )
		tout = 0;
		switch (substream->pcm->device) {
		case SP_I2S_0:
			iprtd->irq_num = 0x1 << 10;
			regs0->aud_fifo_reset |= I2S_C_INC0;
			while ((regs0->aud_fifo_reset & I2S_C_INC0) != 0 && tout < chktimeout)
				tout++;
			break;
		case SP_I2S_1:
			regs0->aud_fifo_reset |= I2S_C_INC1;
			while ((regs0->aud_fifo_reset & I2S_C_INC1) != 0 && tout < chktimeout)
				tout++;
			break;
		case SP_I2S_2:
			regs0->aud_fifo_reset |= I2S_C_INC2;
			while ((regs0->aud_fifo_reset & I2S_C_INC2) != 0 && tout < chktimeout)
				tout++;
			break;
		case SP_TDM:
			break;
		case SP_SPDIF:
			regs0->aud_fifo_reset |= SPDIF_C_INC0;
			while ((regs0->aud_fifo_reset & SPDIF_C_INC0) != 0 && tout < chktimeout)
				tout++;
			break;
		default:
			pr_err("###Wrong device no.\n");
			break;
		}
	}
	if (tout >= chktimeout)
		pr_err("XXX spsoc_pcm_prepare TIMEOUT\n");
	return 0;
}

static int spsoc_pcm_trigger(struct snd_soc_component *component,
			     struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct spsoc_runtime_data *prtd = runtime->private_data;
	unsigned int startthreshold = 0, tout;
	volatile register_audio *regs0 = pcmaudio_base;

	pr_debug("%s IN, %s-cmd %d pcm->device %d\n", __func__, substream->stream ? "cap" : "play",
		 cmd, substream->pcm->device);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if ((frames_to_bytes(runtime, runtime->start_threshold) % prtd->period) == 0) {
			startthreshold = frames_to_bytes(runtime, runtime->start_threshold);
			pr_debug("0:frame_bits %d start_threshold 0x%lx startthreshold=0x%x\n",
				 runtime->frame_bits, runtime->start_threshold, startthreshold);
		} else {
			startthreshold = (frames_to_bytes(runtime, runtime->start_threshold) /
					  prtd->period + 1) * prtd->period;
			pr_debug("1:frame_bits %d start_threshold 0x%lx startthreshold=0x%x\n",
				 runtime->frame_bits, runtime->start_threshold, startthreshold);
		}

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (prtd->trigger_flag == 0) {
				tout = 0;
				if (substream->pcm->device == SP_I2S_0) {
					while ((regs0->aud_inc_0 & I2S_P_INC0) != 0 &&
					       tout < chktimeout)
						tout++;
					run_start(I2S_P_INC0, prtd->period, prtd);
					pr_debug("a0_ptr=0x%x cnt 0x%x\n", regs0->aud_a0_ptr,
						 regs0->aud_a0_cnt);
				} else if (substream->pcm->device == SP_I2S_1) {
					pr_debug("a6_ptr=0x%x cnt 0x%x\n", regs0->aud_a6_ptr,
						 regs0->aud_a6_cnt);
					while ((regs0->aud_inc_0 & I2S_P_INC1) != 0 &&
					       tout < chktimeout)
						tout++;
					run_start(I2S_P_INC1, prtd->period, prtd);
				} else if (substream->pcm->device == SP_I2S_2) {
					pr_debug("a19_ptr=0x%x cnt 0x%x\n", regs0->aud_a19_ptr,
						 regs0->aud_a19_cnt);
					while ((regs0->aud_inc_0 & I2S_P_INC2) != 0 &&
					       tout < chktimeout)
						tout++;
					run_start(I2S_P_INC2, prtd->period, prtd);
				} else if (substream->pcm->device == SP_TDM) {
					pr_debug("a0_ptr=0x%x cnt 0x%x\n", regs0->aud_a0_ptr,
						 regs0->aud_a0_cnt);
					while ((regs0->aud_inc_0 & TDM_P_INC0) != 0 &&
					       tout < chktimeout)
						tout++;
					run_start(TDM_P_INC0, prtd->period, prtd);
				} else if (substream->pcm->device == SP_SPDIF) {
					pr_debug("a5_ptr=0x%x cnt 0x%x\n", regs0->aud_a5_ptr,
						 regs0->aud_a5_cnt);
					while ((regs0->aud_inc_0 & SPDIF_P_INC0) != 0 &&
					       tout < chktimeout)
						tout++;
					run_start(SPDIF_P_INC0, prtd->period, prtd);
				}
			}

			prtd->trigger_flag = 1;
			prtd->start_threshold = 0;
		} else { // if( substream->stream == SNDRV_PCM_STREAM_CAPTURE){
			pr_debug("C:prtd->start_threshold=0x%x\n", prtd->start_threshold);
			regs0->aud_delta_0 = startthreshold;
			prtd->start_threshold = 0;
#if (MMAP_IRQ)
			if (substream->pcm->device == SP_I2S_0) {
				regs0->G019_RESERVED[5] &= 0xffff0000;
				regs0->G019_RESERVED[5] |= prtd->period / 4;
				regs0->G019_RESERVED[8] = prtd->irq_num;
				regs0->G019_RESERVED[10] &= ~prtd->irq_num;
			}
			prtd->irq_done = 0;
			pr_debug("2 [8] 0x%x [10] 0x%x\n", regs0->G019_RESERVED[8],
				 regs0->G019_RESERVED[10]);
#endif
		}

#if (aud_test_mode) //for test
		regs0->aud_embedded_input_ctrl = aud_test_mode;
		pr_info("!!!aud_embedded_input_ctrl = 0x%x!!!\n", regs0->aud_embedded_input_ctrl);
#endif
#if (!MMAP_IRQ)
		if (atomic_read(&prtd->running) == 2) {
			//hrtimer_start(&prtd->hrt, ns_to_ktime(prtd->poll_time_ns),
			//	      HRTIMER_MODE_REL);
			pr_debug("!!!hrtimer non stop!!!\n");
			//snd_hrtimer_callback(&prtd->hrt);
			//while (atomic_read(&prtd->running) != 0)
		}
		hrtimer_start(&prtd->hrt, ns_to_ktime(prtd->poll_time_ns), HRTIMER_MODE_REL);
#endif
		atomic_set(&prtd->running, 1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		atomic_set(&prtd->running, 2);
#if (MMAP_IRQ)
		regs0->G019_RESERVED[10] |= prtd->irq_num;
		prtd->irq_done = 1;
		atomic_set(&prtd->running, 0);
#endif
		tout = 0;
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			prtd->trigger_flag = 0;
			if (substream->pcm->device == SP_I2S_0) {
				while ((regs0->aud_inc_0 & I2S_P_INC0) != 0 && tout < chktimeout)
					tout++;
				//regs0->aud_inc_0 = regs0->aud_inc_0&(~I2S_P_INC0);
			} else if (substream->pcm->device == SP_I2S_1) {
				while ((regs0->aud_inc_0 & I2S_P_INC1) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_I2S_2) {
				while ((regs0->aud_inc_0 & I2S_P_INC2) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_TDM) {
				while ((regs0->aud_inc_0 & TDM_P_INC0) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_SPDIF) {
				while ((regs0->aud_inc_0 & SPDIF_P_INC0) != 0 && tout < chktimeout)
					tout++;
			}
			//regs0->aud_inc_0 = regs0->aud_inc_0&(~SPDIF_P_INC0);
		} else { //if( substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			if (substream->pcm->device == SP_I2S_0) {
				while ((regs0->aud_inc_0 & I2S_C_INC0) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_I2S_1) {
				while ((regs0->aud_inc_0 & I2S_C_INC1) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_I2S_2) {
				while ((regs0->aud_inc_0 & I2S_C_INC2) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_SPDIF) {
				while ((regs0->aud_inc_0 & SPDIF_C_INC0) != 0 && tout < chktimeout)
					tout++;
			} else {
				while ((regs0->aud_inc_0 & TDMPDM_C_INC0) != 0 && tout < chktimeout)
					tout++;
			}
		}
		//hrtimer_start(&prtd->hrt, ns_to_ktime(1), HRTIMER_MODE_REL);
		break;
	default:
		pr_err("%s out\n", __func__);
		return -EINVAL;
	}
	if (tout >= chktimeout)
		pr_err("XXX spsoc_pcm_trigger TIMEOUT\n");
	return 0;
}

static snd_pcm_uframes_t spsoc_pcm_pointer(struct snd_soc_component *component,
					   struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct spsoc_runtime_data *prtd = runtime->private_data;
	snd_pcm_uframes_t offset;

#if (MMAP_IRQ)
	if (prtd->usemmap_flag) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (substream->pcm->device == SP_SPDIF)
				offset = regs0->aud_a5_ptr & 0xfffffc;
			else if (substream->pcm->device == SP_I2S_1)
				offset = regs0->aud_a6_ptr & 0xfffffc;
			else if (substream->pcm->device == SP_I2S_2)
				offset = regs0->aud_a19_ptr & 0xfffffc;
			else
				offset = regs0->aud_a0_ptr & 0xfffffc;
		} else {
			if (substream->pcm->device == SP_I2S_0) // i2s //need to check
				offset = regs0->aud_a11_ptr & 0xfffffc;
			else if (substream->pcm->device == SP_I2S_1) // i2s
				offset = regs0->aud_a16_ptr & 0xfffffc;
			else if (substream->pcm->device == SP_I2S_2) // i2s
				offset = regs0->aud_a10_ptr & 0xfffffc;
			else if (substream->pcm->device == SP_SPDIF) // spdif0
				offset = regs0->aud_a13_ptr & 0xfffffc;
			else // tdm, pdm
				offset = regs0->aud_a22_ptr & 0xfffffc;
		}
	} else
		offset = prtd->offset;
	offset = bytes_to_frames(runtime, offset);
#else
	offset = bytes_to_frames(runtime, prtd->offset);
#endif

	pr_debug("%s-offset=0x%lx\n", substream->stream ? "c" : "p", offset);
	return offset;
}

static int spsoc_pcm_mmap(struct snd_soc_component *component, struct snd_pcm_substream *substream,
			  struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct spsoc_runtime_data *prtd = runtime->private_data;
	int ret = 0;
#if (MMAP_IRQ)
	volatile register_audio *regs0 = pcmaudio_base;

	pr_debug("spsoc_pcm_mmap 0x%x 0x%x 0x%x 0x%x 0x%x\n", regs0->G019_RESERVED[8],
		 regs0->G019_RESERVED[9], regs0->G019_RESERVED[10], regs0->G019_RESERVED[11],
		 regs0->G019_RESERVED[0]);
#else
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		prtd->poll_time_ns = div_u64((u64)prtd->period_size * 1000000000UL +  prtd->speed -
					     1, prtd->speed);
		prtd->timeoutcount = prtd->poll_time_ns / 1000; //unit:1us
	}
	pr_info("%s IN, dir %d poll_time_ns %d timeoutcount %d\n", __func__, substream->stream,
		prtd->poll_time_ns, prtd->timeoutcount);
#endif
	prtd->usemmap_flag = 1;
#ifdef USE_KELNEL_MALLOC
	pr_debug("dev: 0x%p, dma_area 0x%p dma_addr 0x%llx dma_bytes 0x%zx\n",
		 substream->pcm->card->dev, runtime->dma_area, runtime->dma_addr,
		 runtime->dma_bytes);
	ret = dma_mmap_wc(substream->pcm->card->dev, vma,
			  runtime->dma_area,
			  runtime->dma_addr,
			  runtime->dma_bytes);
#else
	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	ret = remap_pfn_range(vma, vma->vm_start,
			      substream->dma_buffer.addr >> PAGE_SHIFT,
			      vma->vm_end - vma->vm_start, vma->vm_page_prot);
#endif

	return ret;
}

static int spsoc_pcm_copy(struct snd_soc_component *component, struct snd_pcm_substream *substream,
			  int channel, unsigned long pos, struct iov_iter *buf, unsigned long count)
{
	int ret = 0, tout = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct spsoc_runtime_data *prtd = runtime->private_data;
	char *hwbuf = runtime->dma_area + pos;
	unsigned long count_bytes = count;
	volatile register_audio *regs0 = pcmaudio_base;

	spin_lock(&set_lock);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
	//pr_debug("###%s IN, aud_a0_ptr=0x%x, dma_area=0x%x, pos=0x%lx count_bytes 0x%x\n",
	//__func__, regs0->aud_a0_ptr, hwbuf, pos, count_bytes);
		pr_debug("!%s IN, aud_a0_ptr=0x%x ", __func__, regs0->aud_a0_ptr);
		if (prtd->trigger_flag) {
			if (substream->pcm->device == SP_I2S_0) {
				pr_debug("dma_area=0x%p pos=0x%lx count 0x%lx a0_cnt 0x%x\n",
					 hwbuf, pos, count, regs0->aud_a0_cnt);
				while ((regs0->aud_inc_0 & I2S_P_INC0) != 0 && tout < chktimeout)
					tout++;
				tout = 0;
				while (regs0->aud_a0_cnt != 0 && tout < prtd->timeoutcount) {
					udelay(1);
					tout++;
				}
				run_start(I2S_P_INC0, count_bytes, prtd);
			} else if (substream->pcm->device == SP_TDM) {
				pr_debug("dma_area=0x%p pos=0x%lx count_bytes 0x%lx count 0x%lx\n",
					 hwbuf, pos, count_bytes, count);
				while ((regs0->aud_inc_0 & TDM_P_INC0) != 0 && tout < chktimeout)
					tout++;
				tout = 0;
				while (regs0->aud_a0_cnt != 0 && tout < prtd->timeoutcount) {
					udelay(1);
					tout++;
				}
				run_start(TDM_P_INC0, count_bytes, prtd);
			} else if (substream->pcm->device == SP_I2S_1) {
				pr_debug("dma_area=0x%p pos=0x%lx count_bytes 0x%lx count 0x%lx\n",
					 hwbuf, pos, count_bytes, count);
				while ((regs0->aud_inc_0 & I2S_P_INC1) != 0 && tout < chktimeout)
					tout++;
				tout = 0;
				while (regs0->aud_a6_cnt != 0 && tout < prtd->timeoutcount) {
					udelay(1);
					tout++;
				}
				run_start(I2S_P_INC1, count_bytes, prtd);
			} else if (substream->pcm->device == SP_I2S_2) {
				pr_debug("dma_area=0x%p pos=0x%lx count_bytes 0x%lx count 0x%lx\n",
					 hwbuf, pos, count_bytes, count);
				while ((regs0->aud_inc_0 & I2S_P_INC2) != 0 && tout < chktimeout)
					tout++;
				tout = 0;
				while (regs0->aud_a19_cnt != 0 && tout < prtd->timeoutcount) {
					udelay(1);
					tout++;
				}
				run_start(I2S_P_INC2, count_bytes, prtd);
			} else if (substream->pcm->device == SP_SPDIF) {
				pr_debug("dma_area=0x%p pos=0x%lx count_bytes 0x%lx count 0x%lx\n",
					 hwbuf, pos, count_bytes, count);
				while ((regs0->aud_inc_0 & SPDIF_P_INC0) != 0 && tout < chktimeout)
					tout++;
				tout = 0;
				while (regs0->aud_a5_cnt != 0 && tout < prtd->timeoutcount) {
					udelay(1);
					tout++;
				}
				run_start(SPDIF_P_INC0, count_bytes, prtd);
			}
#if (!MMAP_IRQ)
			//hrtimer_forward_now(&prtd->hrt, ns_to_ktime(prtd->poll_time_ns));
			hrtimer_start(&prtd->hrt, ns_to_ktime(prtd->poll_time_ns),
				      HRTIMER_MODE_REL);
			//hrtimer_restart(&prtd->hrt);
#endif
			//prtd->last_remainder = (count_bytes + prtd->last_remainder) % 4;
			//copy_from_user(hwbuf, buf, count_bytes);
		} else {
			if (substream->pcm->device == SP_I2S_0) {
				pr_debug("#dma_area=0x%p pos=0x%lx count_bytes 0x%lx\n",
					 hwbuf, pos, count_bytes);
				while ((regs0->aud_inc_0 & I2S_P_INC0) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_I2S_1) {
				pr_debug("#dma_area=0x%p pos=0x%lx count_bytes 0x%lx\n",
					 hwbuf, pos, count_bytes);
				while ((regs0->aud_inc_0 & I2S_P_INC1) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_I2S_2) {
				pr_debug("#dma_area=0x%p pos=0x%lx count_bytes 0x%lx\n",
					 hwbuf, pos, count_bytes);
				while ((regs0->aud_inc_0 & I2S_P_INC2) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_SPDIF) {
				pr_debug("#dma_area=0x%p pos=0x%lx count_bytes 0x%lx\n",
					 hwbuf, pos, count_bytes);
				while ((regs0->aud_inc_0 & SPDIF_P_INC0) != 0 && tout < chktimeout)
					tout++;
			} else if (substream->pcm->device == SP_TDM) {
				pr_debug("#dma_area=0x%p pos=0x%lx count_bytes 0x%lx\n",
					 hwbuf, pos, count_bytes);
				while ((regs0->aud_inc_0 & TDM_P_INC0) != 0 && tout < chktimeout)
					tout++;
			}
			prtd->start_threshold += frames_to_bytes(runtime, count);
		}
		copy_from_iter_toio(hwbuf, buf, count_bytes);
	} else { //capture
		pr_debug("###%s IN, buf=0x%p, dma_area=0x%p, pos=0x%lx count_bytes 0x%lx\n",
			 __func__, buf, hwbuf, pos, count_bytes);
		if (substream->pcm->device == SP_I2S_0) {
			while ((regs0->aud_inc_0 & I2S_C_INC0) != 0 && tout < chktimeout)
				tout++;
		} else if (substream->pcm->device == SP_I2S_1) {
			while ((regs0->aud_inc_0 & I2S_C_INC1) != 0 && tout < chktimeout)
				tout++;
		} else if (substream->pcm->device == SP_I2S_2) {
			while ((regs0->aud_inc_0 & I2S_C_INC2) != 0 && tout < chktimeout)
				tout++;
		} else if (substream->pcm->device == SP_SPDIF) {
			while ((regs0->aud_inc_0 & SPDIF_C_INC0) != 0 && tout < chktimeout)
				tout++;
		} else {
			while ((regs0->aud_inc_0 & TDMPDM_C_INC0) != 0 && tout < chktimeout)
				tout++;
		}

		copy_to_iter_fromio(buf, hwbuf, count_bytes);

		if (substream->pcm->device == SP_I2S_0)
			run_start(I2S_C_INC0, count_bytes, prtd);
		else if (substream->pcm->device == SP_I2S_1)
			run_start(I2S_C_INC1, count_bytes, prtd);
		else if (substream->pcm->device == SP_I2S_2)
			run_start(I2S_C_INC2, count_bytes, prtd);
		else if (substream->pcm->device == SP_SPDIF)
			run_start(SPDIF_C_INC0, count_bytes, prtd);
		else
			run_start(TDMPDM_C_INC0, count_bytes, prtd);
	}
	spin_unlock(&set_lock);
	if (tout >= prtd->timeoutcount)
		pr_err("XXX spsoc_pcm_copy TIMEOUT %s\n", substream->stream ? "cap" : "play");
	return ret;
}

static u64 spsoc_pcm_dmamask = DMA_BIT_MASK(32);

static int spsoc_pcm_new(struct snd_soc_component *component, struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;

	dev_dbg(component->dev, "%s IN %s %s\n", __func__, rtd->dai_link->name,
		rtd->dai_link->stream_name);
	if (!card->dev->dma_mask)
		card->dev->dma_mask = &spsoc_pcm_dmamask;

	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = spsoc_pcm_preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = spsoc_pcm_preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
out:
	return 0;
}

static void spsoc_pcm_free_dma_buffers(struct snd_soc_component *component, struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		buf->area = NULL;
	}
	pr_debug("%s IN\n", __func__);
}

static const struct snd_soc_component_driver sunplus_soc_platform = {
	//.ops		= &spsoc_pcm_ops,
	.pcm_construct	= spsoc_pcm_new,
	.pcm_destruct	= spsoc_pcm_free_dma_buffers,
	.open		= spsoc_pcm_open,
	.close		= spsoc_pcm_close,
	//.ioctl	= snd_pcm_lib_ioctl,
	.hw_params	= spsoc_pcm_hw_params,
	.hw_free	= spsoc_pcm_hw_free,
	.prepare	= spsoc_pcm_prepare,
	.trigger	= spsoc_pcm_trigger,
	.pointer	= spsoc_pcm_pointer,
	.mmap		= spsoc_pcm_mmap,
	.copy		= spsoc_pcm_copy,
	//.fill_silence	= pcm_silence ,
};

static int preallocate_dma_buffer(struct platform_device *pdev)
{
	unsigned int size;
	struct device *dev = &pdev->dev;

	aud_param.fifo_info.pcmtx_virt_base = 0;
	aud_param.fifo_info.mic_virt_base = 0;
	aud_param.fifo_info.txbuf_len = DRAM_PCM_BUF_LENGTH * NUM_FIFO_TX;
	aud_param.fifo_info.rxbuf_len = DRAM_PCM_BUF_LENGTH * NUM_FIFO_RX;
	size = aud_param.fifo_info.txbuf_len + aud_param.fifo_info.rxbuf_len;
	aud_param.fifo_info.buf_total_len = size;
#ifdef USE_KELNEL_MALLOC
	aud_param.fifo_info.pcmtx_virt_base = (unsigned long)
	dma_alloc_coherent(dev, PAGE_ALIGN(size), &aud_param.fifo_info.pcmtx_phys_base,
			   GFP_DMA | GFP_KERNEL);
#else
	aud_param.fifo_info.pcmtx_virt_base =
	(unsigned int)gp_chunk_malloc_nocache(1, 0, PAGE_ALIGN(size));
	aud_param.fifo_info.pcmtx_phys_base =
	gp_chunk_pa((void *)aud_param.fifo_info.pcmtx_virt_base);
#endif
	pr_debug("pcmtx_virt_base 0x%lx pcmtx_phys_base 0x%llx\n",
		 aud_param.fifo_info.pcmtx_virt_base, aud_param.fifo_info.pcmtx_phys_base);
	if (!aud_param.fifo_info.pcmtx_virt_base) {
		pr_err("failed to allocate playback DMA memory\n");
		return -ENOMEM;
	}

	aud_param.fifo_info.mic_virt_base = aud_param.fifo_info.pcmtx_virt_base +
	aud_param.fifo_info.txbuf_len;
	aud_param.fifo_info.mic_phys_base = aud_param.fifo_info.pcmtx_phys_base +
	aud_param.fifo_info.txbuf_len;

	return 0;
}

static void dma_free_dma_buffers(struct platform_device *pdev)
{
	unsigned int size;

	size = DRAM_PCM_BUF_LENGTH * (NUM_FIFO_TX + NUM_FIFO_RX);
#ifdef USE_KELNEL_MALLOC
	dma_free_coherent(&pdev->dev, size,
			  (unsigned int *)aud_param.fifo_info.pcmtx_virt_base,
			  aud_param.fifo_info.pcmtx_phys_base);
#else
	//gp_chunk_free( (void *)aud_param.fifo_info.pcmtx_virt_base);
#endif
}

void __iomem *pcm_get_spaud_data(void)
{
	struct device_node *np  = of_find_compatible_node(NULL, NULL, "sunplus,audio");
	struct platform_device *spaudpdev = of_find_device_by_node(np);

	if (!np) {
		dev_err(&spaudpdev->dev, "devicetree status is not available\n");
		of_node_put(np);
		return  NULL;
	}

	spaudpdev = of_find_device_by_node(np);
	if (!spaudpdev) {
		pr_info("No audio driver\n");
		of_node_put(np);
		return  NULL;
	}

	spauddata = dev_get_drvdata(&spaudpdev->dev);
	if (!spauddata)
		spauddata = ERR_PTR(-EPROBE_DEFER);

	return spauddata->audio_base;
}

static int snd_spsoc_pcm_probe(struct platform_device *pdev)
{
	int ret = 0;

	dev_info(&pdev->dev, "%s\n", __func__);
	pcmaudio_base = pcm_get_spaud_data();
	pr_debug("audio_base2=%p\n", pcmaudio_base);

	ret = devm_snd_soc_register_component(&pdev->dev, &sunplus_soc_platform, NULL, 0);
	// create & register device for file operation, used for 'ioctl'
	memset(&aud_param, 0, sizeof(struct t_auddrv_param));
	memset(&aud_param.fifo_info, 0, sizeof(struct t_AUD_FIFO_PARAMS));

	//aud_param.fsclkInfo.freq_mask = 0x0667; //192K
	ret = preallocate_dma_buffer(pdev);
	memset((void *)aud_param.fifo_info.pcmtx_virt_base, 0, aud_param.fifo_info.buf_total_len);
	return ret;
}

static int snd_spsoc_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	dma_free_dma_buffers(pdev);
	return 0;
}

static struct platform_driver snd_spsoc_driver = {
	.driver = {
		.name		= "spsoc-pcm-driver",
		.owner	= THIS_MODULE,
		//.of_match_table	= of_match_ptr(sunplus_audio_pcm_dt_ids),
	},

	.probe	= snd_spsoc_pcm_probe,
	.remove = snd_spsoc_remove,
};

static struct platform_device *spsoc_pcm_device;
static int __init snd_spsoc_pcm_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&snd_spsoc_driver);
	if (ret)
		pr_err("pcm driver register error\n");

	spsoc_pcm_device = platform_device_alloc("spsoc-pcm-driver", -1);
	if (!spsoc_pcm_device) {
		pr_err("device error\n");
		return -ENOMEM;
	}

	ret = platform_device_add(spsoc_pcm_device);
	if (ret)
		platform_device_put(spsoc_pcm_device);

	return ret;
}
module_init(snd_spsoc_pcm_init);

static void __exit snd_spsoc_pcm_exit(void)
{
	platform_device_unregister(spsoc_pcm_device);
	platform_driver_unregister(&snd_spsoc_driver);
}
module_exit(snd_spsoc_pcm_exit);

MODULE_AUTHOR("Sunplus Technology Inc.");
MODULE_DESCRIPTION("Sunplus SoC ALSA PCM module");
MODULE_LICENSE("GPL");

