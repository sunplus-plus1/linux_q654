// SPDX-License-Identifier: GPL-2.0
// 2022.12 / chingchou.huang
// This	file is	all about audio	hardware initialization.
// include ADC/DAC/SPDIF/PLL...etc
//
#include <linux/device.h>
#include "aud_hw.h"
#include "spsoc_pcm.h"
#include "spsoc_util.h"

#ifdef __LOG_NAME__
#undef __LOG_NAME__
#endif
#define	__LOG_NAME__	"audhw"

#define	ONEHOT_B10 0x00000400
#define	ONEHOT_B11 0x00000800
#define	ONEHOT_B12 0x00001000
#if IS_MODULE(CONFIG_SND_SOC_AUD_SP7350)
auddrv_param aud_param;
#endif

void AUDHW_pin_mx(void)
{
}
EXPORT_SYMBOL_GPL(AUDHW_pin_mx);

void AUDHW_Mixer_Setting(void *auddrvdata)
{
	struct sunplus_audio_base *pauddrvdata = auddrvdata;
	u32 val;
	volatile register_audio *regs0 = pauddrvdata->audio_base;
	//67. 0~4
	regs0->aud_grm_master_gain	= 0x80000000; //aud_grm_master_gain
	regs0->aud_grm_gain_control_0	= 0x80808080; //aud_grm_gain_control_0
	regs0->aud_grm_gain_control_1	= 0x80808080; //aud_grm_gain_control_1
	regs0->aud_grm_gain_control_2	= 0x808000; //aud_grm_gain_control_2
	regs0->aud_grm_gain_control_3	= 0x80808080; //aud_grm_gain_control_3
	regs0->aud_grm_gain_control_4	= 0x0000007f; //aud_grm_gain_control_4
	//val =	0x204; //1=pcm, mix75,	mix73
	//val =	val|0x08100000; //1=pcm, mix79,	mix77
	val = 0x20402040;
	regs0->aud_grm_mix_control_1	= val; //aud_grm_mix_control_1
	val = 0;
	regs0->aud_grm_mix_control_2	= val; //aud_grm_mix_control_2
	//EXT DAC I2S
	regs0->aud_grm_switch_0		= 0x76543210; //aud_grm_switch_0
	regs0->aud_grm_switch_1		= 0x98;	//aud_grm_switch_1
	//INT DAC I2S
	regs0->aud_grm_switch_int	= 0x76543210; //aud_grm_switch_int
	regs0->aud_grm_delta_volume	= 0x8000; //aud_grm_delta_volume
	regs0->aud_grm_delta_ramp_pcm	= 0x8000; //aud_grm_delta_ramp_pcm
	regs0->aud_grm_delta_ramp_risc	= 0x8000; //aud_grm_delta_ramp_risc
	regs0->aud_grm_delta_ramp_linein = 0x8000; //aud_grm_delta_ramp_linein
	regs0->aud_grm_other		= 0x0; //aud_grm_other	for A20

	regs0->aud_grm_switch_hdmi_tx	= 0x76543210; //aud_grm_switch_hdmi_tx
}
EXPORT_SYMBOL_GPL(AUDHW_Mixer_Setting);

/************sub api**************/
///// utilities	of test	program	/////
void AUDHW_Cfg_AdcIn(void *auddrvdata)
{
	struct sunplus_audio_base *pauddrvdata = auddrvdata;
	int val, tout = 0;
	volatile register_audio *regs0 = pauddrvdata->audio_base;

	regs0->adcp_ch_enable	= 0x0; //adcp_ch_enable
	regs0->adcp_fubypass	= 0x7777; //adcp_fubypass

	regs0->adcp_mode_ctrl	|= 0x300; //enable ch2/3

	regs0->adcp_risc_gain	= 0x1111; //adcp_risc_gain, all gains are	1x
	regs0->G018_reserved_00	= 0x3; //adcprc A16~18
	val			= 0x650100; //steplen0=0, Eth_off=0x65, Eth_on=0x100, steplen0=0
	regs0->adcp_agc_cfg	= val; //adcp_agc_cfg0
   //ch0
	val = (1 << 6) | ONEHOT_B11;
	regs0->adcp_init_ctrl =	val;
	do {
		val = regs0->adcp_init_ctrl;
		tout++;
	} while	((val & ONEHOT_B12) != 0 && tout < chktimeout);
	if (tout >= chktimeout)
		pr_err("XXX AUDHW_Cfg_AdcIn TIMEOUT 1\n");

	val = (1 << 6) | 2 | ONEHOT_B10;
	regs0->adcp_init_ctrl =	val;

	val = 0x800000;
	regs0->adcp_gain_0 = val;

	val = regs0->adcp_risc_gain;
	val = val & 0xfff0;
	val = val | 1;
	regs0->adcp_risc_gain =	val;
   //ch1
	tout = 0;
	val = (1 << 6) | (1 << 4) | ONEHOT_B11;
	regs0->adcp_init_ctrl =	val;
	do {
		val = regs0->adcp_init_ctrl;
		tout++;
	} while	((val & ONEHOT_B12) != 0 && tout < chktimeout);
	if (tout >= chktimeout)
		pr_err("XXX AUDHW_Cfg_AdcIn TIMEOUT 2\n");

	val = (1 << 6) | (1 << 4) | 2 | ONEHOT_B10;
	regs0->adcp_init_ctrl =	val;

	val = 0x800000;
	regs0->adcp_gain_1 = val;

	val = regs0->adcp_risc_gain;
	val = val & 0xff0f;
	val = val | 0x10;
	regs0->adcp_risc_gain =	val;
}

void AUDHW_SystemInit(void *auddrvdata)
{
	struct sunplus_audio_base *pauddrvdata = auddrvdata;
	volatile register_audio *regs0 = pauddrvdata->audio_base;
	int tout = 0;

	pr_debug("!!!audio_base 0x%p\n", regs0);
	pr_debug("!!!aud_fifo_reset 0x%p\n", &regs0->aud_fifo_reset);
	//reset	aud fifo
	regs0->audif_ctrl = 0x1; //aud_ctrl=1
	pr_debug("aud_fifo_reset 0x%x\n", regs0->aud_fifo_reset);
	regs0->audif_ctrl = 0x0; //aud_ctrl=0
	while (regs0->aud_fifo_reset && tout < chktimeout)
		tout++;
	if (tout >= chktimeout)
		pr_err("XXX AUDHW_SystemInit TIMEOUT\n");

	regs0->pcm_cfg		= 0x71;	//sp7350 tx0
	regs0->ext_adc_cfg	= 0x71;	//rx0
	regs0->hdmi_tx_i2s_cfg	= 0x71;	//sp7350 tx2 if tx2(slave) -> rx0 -> tx1/tx0  0x24d
	regs0->hdmi_rx_i2s_cfg	= 0x71;	//rx2
	regs0->int_adc_dac_cfg	= 0x00710071; //0x001c004d // rx1 tx1

	regs0->iec0_par0_out	= 0x40009800; //config PCM_IEC_TX, pcm_iec_par0_out
	regs0->iec0_par1_out	= 0x00000000; //pcm_iec_par1_out

	regs0->iec1_par0_out	= 0x40009800; //config PCM_IEC_TX, pcm_iec_par0_out
	regs0->iec1_par1_out	= 0x00000000; //pcm_iec_par1_out

	AUDHW_Cfg_AdcIn(auddrvdata);

	regs0->iec_cfg		= 0x4066; //iec_cfg
	// config playback timer //
	regs0->aud_apt_mode	= 1; // aud_apt_mode, reset mode
	regs0->aud_apt_data	= 0x00f0001e; // aud_apt_parameter, parameter	for 48khz

	regs0->adcp_ch_enable	= 0xf; //adcp_ch_enable, Only enable ADCP ch2&3
	regs0->aud_grm_path_select |= 0x03;

	regs0->aud_apt_mode	= 0; //clear reset of PTimer before enable FIFO

	regs0->aud_fifo_enable	= 0x0; // aud_fifo_enable
	regs0->aud_enable	= 0x0; //aud_enable[21]PWM 5f

	regs0->int_dac_ctrl1	&= 0x7fffffff;
	regs0->int_dac_ctrl1	|= (0x1 << 31);

	regs0->int_adc_ctrl	= 0x80000726;
	regs0->int_adc_ctrl2	= 0x26;
	regs0->int_adc_ctrl1	= 0x20;
	regs0->int_adc_ctrl	&= 0x7fffffff;
	regs0->int_adc_ctrl	|= (1 << 31);

	regs0->aud_fifo_mode	= 0x20001;
	//regs0->aud_asrc_ctrl = 0x4B0; //[7:4] if0  [11:8] if1
	//regs0->aud_asrc_ctrl = regs0->G063_reserved_7|0x1; // enable
	pr_debug("!!!aud_misc_ctrl 0x%x\n", regs0->aud_misc_ctrl);
	//regs0->aud_misc_ctrl |= 0x2;
#if IS_ENABLED(CONFIG_SND_SOC_ES8316_SUNPLUS)
	regs0->aud_ext_dac_xck_cfg	= 0x6883;
	regs0->aud_ext_dac_bck_cfg	= 0x6007;
#endif
}
EXPORT_SYMBOL_GPL(AUDHW_SystemInit);

void snd_aud_config(void *auddrvdata)
{
	struct sunplus_audio_base *pauddrvdata = auddrvdata;
	volatile register_audio *regs0 = pauddrvdata->audio_base;
	int dma_initial;

	dma_initial = DRAM_PCM_BUF_LENGTH * (NUM_FIFO_TX - 1);
	regs0->aud_audhwya	= aud_param.fifo_info.pcmtx_phys_base;
	regs0->aud_a0_base	= dma_initial;
	regs0->aud_a1_base	= dma_initial;
	regs0->aud_a2_base	= dma_initial;
	regs0->aud_a3_base	= dma_initial;
	regs0->aud_a4_base	= dma_initial;
	regs0->aud_a5_base	= dma_initial;
	regs0->aud_a6_base	= dma_initial;
	regs0->aud_a20_base	= dma_initial;
	regs0->aud_a19_base	= dma_initial;
	regs0->aud_a26_base	= dma_initial;
	regs0->aud_a27_base	= dma_initial;

	dma_initial = DRAM_PCM_BUF_LENGTH * (NUM_FIFO -	1);
	regs0->aud_a13_base	= dma_initial;
	regs0->aud_a16_base	= dma_initial;
	regs0->aud_a17_base	= dma_initial;
	regs0->aud_a18_base	= dma_initial;
	regs0->aud_a21_base	= dma_initial;
	regs0->aud_a22_base	= dma_initial;
	regs0->aud_a23_base	= dma_initial;
	regs0->aud_a24_base	= dma_initial;
	regs0->aud_a25_base	= dma_initial;
	regs0->aud_a10_base	= dma_initial;
	regs0->aud_a11_base	= dma_initial;
	regs0->aud_a14_base	= dma_initial;
}
EXPORT_SYMBOL_GPL(snd_aud_config);
