/* SPDX-License-Identifier: GPL-2.0
 * ALSA SoC SP7350 pcm driver
 * Author: ChingChou Huang <chingchouhuang@sunplus.com>
 */
#ifndef	_SPSOC_UTIL_H
#define	_SPSOC_UTIL_H

#include <sound/dmaengine_pcm.h>
#include "types.h"

struct sunplus_audio_base {
	void __iomem *audio_base;
	struct device *dev;
	struct regmap *regmap;
	struct reset_control *clk_rst;
	struct clk *aud_clocken;
	//struct clk *plla_clocken;
	//tdm
	struct snd_dmaengine_dai_dma_data	dma_playback;
	struct snd_dmaengine_dai_dma_data	dma_capture;
	int					master;
};

extern void __iomem *audio_base;

/**********************************************************
 *			BASE
 **********************************************************/
#define	REG_BASEADDR	0xF8000000 //0x40000000

//#define REG(g, i)	((void __iomem *) VA_IOB_ADDR(((g) * 32	+ (i)) * 4))

/**********************************************************
 *
 **********************************************************/
#define	AUD_RATES	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_32000 |\
			 SNDRV_PCM_RATE_44100 |	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_64000 |\
			 SNDRV_PCM_RATE_96000 |	SNDRV_PCM_RATE_192000)

#define	AUD_RATES_C	(SNDRV_PCM_RATE_32000 |	SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000)

/**********************************************************
 *			Register Definition
 **********************************************************/
typedef	struct {
	// Group 009 : AUD_REG_G0
	unsigned int  audif_ctrl		; // 00
	unsigned int  aud_enable		; // 01
	unsigned int  pcm_cfg			; // 02
	unsigned int  i2s_mute_flag_ctrl	; // 03
	unsigned int  ext_adc_cfg		; // 04
	unsigned int  int_dac_ctrl0		; // 05
	unsigned int  int_adc_ctrl		; // 06
	unsigned int  adc_in_path_switch	; // 07
	unsigned int  int_adc_dac_cfg		; // 08
	unsigned int  G009_reserved_9		; // 09	ahb_ifx_cfg
	unsigned int  iec_cfg			; // 10
	unsigned int  iec0_valid_out		; // 11
	unsigned int  iec0_par0_out		; // 12
	unsigned int  iec0_par1_out		; // 13
	unsigned int  iec1_valid_out		; // 14
	unsigned int  iec1_par0_out		; // 15
	unsigned int  iec1_par1_out		; // 16
	unsigned int  iec0_rx_debug_info	; // 17
	unsigned int  iec0_valid_in		; // 18
	unsigned int  iec0_par0_in		; // 19
	unsigned int  iec0_par1_in		; // 20
	unsigned int  iec1_rx_debug_info	; // 21
	unsigned int  iec1_valid_in		; // 22
	unsigned int  iec1_par0_in		; // 23
	unsigned int  iec1_par1_in		; // 24
	unsigned int  iec2_rx_debug_info	; // 25
	unsigned int  iec2_valid_in		; // 26
	unsigned int  iec2_par0_in		; // 27
	unsigned int  iec2_par1_in		; // 28
	unsigned int  G009_reserved_29		; // 29
	unsigned int  iec_tx_user_wdata		; // 30
	unsigned int  iec_tx_user_ctrl		; // 31

	// Group 010 : AUD_REG_G1
	unsigned int  adcp_ch_enable		; // 00, ADCPRC Configuration Group	1
	unsigned int  adcp_fubypass		; // 01, ADCPRC Configuration Group	2
	unsigned int  adcp_mode_ctrl		; // 02, ADCPRC Mode Control
	unsigned int  adcp_init_ctrl		; // 03, ADCP Initialization Control
	unsigned int  adcp_coeff_din		; // 04, Coefficient Data Input
	unsigned int  adcp_agc_cfg		; // 05, ADCPRC AGC	Configuration of Ch0/1
	unsigned int  adcp_agc_cfg2		; // 06, ADCPRC AGC	Configuration of Ch2/3
	unsigned int  adcp_gain_0		; // 07, ADCP System Gain1
	unsigned int  adcp_gain_1		; // 08, ADCP System Gain2
	unsigned int  adcp_gain_2		; // 09, ADCP System Gain3
	unsigned int  adcp_gain_3		; // 10, ADCP System Gain4
	unsigned int  adcp_risc_gain		; // 11, ADCP RISC Gain
	unsigned int  adcp_mic_l		; // 12, ADCPRC Microphone - in Left Channel Data
	unsigned int  adcp_mic_r		; // 13, ADCPRC Microphone - in Right Channel Data
	unsigned int  adcp_agc_gain		; // 14, ADCPRC AGC	Gain
	unsigned int  G010_reserved_15		; // 15, Reserved
	unsigned int  aud_apt_mode		; // 16, Audio Playback Timer Mode
	unsigned int  aud_apt_data		; // 17, Audio Playback Timer
	unsigned int  aud_apt_parameter		; // 18, Audio Playback Timer Parameter
	unsigned int  G010_reserved_19		; // 19, Reserved aud_audhwya2
	unsigned int  aud_audhwya		; // 20, DRAM Base Address Offset
	unsigned int  aud_inc_0			; // 21, DMA Counter Increment/Decrement
	unsigned int  aud_delta_0		; // 22, Delta Value
	unsigned int  aud_fifo_enable		; // 23, Audio FIFO	Enable
	unsigned int  aud_fifo_mode		; // 24, FIFO Mode Control
	unsigned int  aud_fifo_support		; // 25, Supported FIFOs (Debug Function)
	unsigned int  aud_fifo_reset		; // 26, Host FIFO Reset
	unsigned int  aud_chk_ctrl		; // 27, Checksum Control (Debug Function)
	unsigned int  aud_checksum_data		; // 28, Checksum Data (Debug Function) aud_new_pts
	unsigned int  aud_chk_tcnt		; // 29, Target Count of Checksum aud_new_pts_ptr
	unsigned int  aud_embedded_input_ctrl	; // 30, Embedded Input Control ( Debug Function )
	unsigned int  aud_misc_ctrl		; // 31, Miscellaneous Control

	// Group 011 : AUD_REG_G2
	unsigned int  aud_ext_dac_xck_cfg	; // 00
	unsigned int  aud_ext_dac_bck_cfg	; // 01
	unsigned int  aud_iec0_bclk_cfg		; // 02
	unsigned int  aud_ext_adc_xck_cfg	; // 03
	unsigned int  aud_ext_adc_bck_cfg	; // 04
	unsigned int  aud_int_adc_xck_cfg	; // 05
	unsigned int  G011_reserved_6		; // 06
	unsigned int  aud_int_dac_xck_cfg	; // 07
	unsigned int  aud_int_dac_bck_cfg	; // 08
	unsigned int  aud_iec1_bclk_cfg		; // 09
	unsigned int  G011_reserved_10		; // 10
	unsigned int  aud_pcm_iec_bclk_cfg	; // 11
	unsigned int  aud_xck_osr104_cfg	; // 12
	unsigned int  aud_hdmi_tx_mclk_cfg	; // 13
	unsigned int  aud_hdmi_tx_bck_cfg	; // 14
	unsigned int  hdmi_tx_i2s_cfg		; // 15
	unsigned int  hdmi_rx_i2s_cfg		; // 16
	unsigned int  aud_aadc_agc01_cfg0	; // 17
	unsigned int  aud_aadc_agc01_cfg1	; // 18
	unsigned int  aud_aadc_agc01_cfg2	; // 19
	unsigned int  aud_aadc_agc01_cfg3	; // 20
	unsigned int  int_adc_ctrl3		; // 21
	unsigned int  int_adc_ctrl2		; // 22
	unsigned int  int_dac_ctrl2		; // 23
	unsigned int  G011_reserved_24		; // 24
	unsigned int  int_dac_ctrl1		; // 25
	unsigned int  G011_reserved_26		; // 26 aud_force_cken
	unsigned int  G011_reserved_27		; // 27 aud_recovery_trl
	unsigned int  G011_reserved_28		; // 28 pcm_iec_par0_out
	unsigned int  G011_reserved_29		; // 29 pcm_iec_par1_out
	unsigned int  G011_reserved_30		; // 30 dmactrl_cnt_inc_1
	unsigned int  G011_reserved_31		; // 31 dmactrl_cnt_delta_1

	// Group 012 : AUD_REG_G3
	unsigned int  aud_bt_ifx_cfg		; // 00
	unsigned int  aud_bt_i2s_cfg		; // 01
	unsigned int  aud_bt_xck_cfg		; // 02
	unsigned int  aud_bt_bck_cfg		; // 03
	unsigned int  aud_bt_sync_cfg		; // 04
	unsigned int  G012_reserved_5		; // 05 ifx0_sampling_rate_cnt
	unsigned int  G012_reserved_6		; // 06 ifx1_sampling_rate_cnt
	unsigned int  aud_asrc_ctrl		; // 07 asrc_ctrl
	unsigned int  aud_pwm_xck_cfg		; // 08
	unsigned int  aud_pwm_bck_cfg		; // 09
	unsigned int  G012_reserved_10		; // 10 pga_gain_maximum_ctrl0
	unsigned int  G012_reserved_11		; // 11 pga_gain_maximum_ctrl1
	unsigned int  G012_reserved_12		; // 12
	unsigned int  G012_reserved_13		; // 13 pgag_sample_cnt_01
	unsigned int  G012_reserved_14		; // 14 adac_pga_gain_0l_ctrl
	unsigned int  G012_reserved_15		; // 15 adac_pga_gain_0r_ctrl
	unsigned int  G012_reserved_16		; // 16 adac_pga_gain_1l_ctrl
	unsigned int  G012_reserved_17		; // 17 adac_pga_gain_1r_ctrl
	unsigned int  G012_reserved_18		; // 18 adac_pga_gain_2r_ctrl
	unsigned int  G012_reserved_19		; // 19 aud_adac_agc_status
	unsigned int  aud_aadc_agc2_cfg0	; // 20
	unsigned int  aud_aadc_agc2_cfg1	; // 21
	unsigned int  aud_aadc_agc2_cfg2	; // 22
	unsigned int  aud_aadc_agc2_cfg3	; // 23
	unsigned int  aud_opt_test_pat		; // 24
	unsigned int  aud_sys_status0		; // 25
	unsigned int  aud_sys_status1		; // 26
	unsigned int  int_adc_ctrl1		; // 27
	unsigned int  bt_mute_flag		; // 28 other_status
	unsigned int  cdrpll_losd_ctrl		; // 29
	unsigned int  G012_reserved_30		; // 30 losd_release_cnt
	unsigned int  other_config		; // 31 other_ctrl

	// Group 013 : AUD_DMA_G0
	unsigned int  aud_a0_base		; // 00
	unsigned int  aud_a0_length		; // 01
	unsigned int  aud_a0_ptr		; // 02
	unsigned int  aud_a0_cnt		; // 03
	unsigned int  aud_a1_base		; // 04
	unsigned int  aud_a1_length		; // 05
	unsigned int  aud_a1_ptr		; // 06
	unsigned int  aud_a1_cnt		; // 07
	unsigned int  aud_a2_base		; // 08
	unsigned int  aud_a2_length		; // 09
	unsigned int  aud_a2_ptr		; // 10
	unsigned int  aud_a2_cnt		; // 11
	unsigned int  aud_a3_base		; // 12
	unsigned int  aud_a3_length		; // 13
	unsigned int  aud_a3_ptr		; // 14
	unsigned int  aud_a3_cnt		; // 15
	unsigned int  aud_a4_base		; // 16
	unsigned int  aud_a4_length		; // 17
	unsigned int  aud_a4_ptr		; // 18
	unsigned int  aud_a4_cnt		; // 19
	unsigned int  aud_a5_base		; // 20
	unsigned int  aud_a5_length		; // 21
	unsigned int  aud_a5_ptr		; // 22
	unsigned int  aud_a5_cnt		; // 23
	unsigned int  aud_a6_base		; // 24
	unsigned int  aud_a6_length		; // 25
	unsigned int  aud_a6_ptr		; // 26
	unsigned int  aud_a6_cnt		; // 27
	unsigned int  aud_a7_base		; // 28
	unsigned int  aud_a7_length		; // 29
	unsigned int  aud_a7_ptr		; // 30
	unsigned int  aud_a7_cnt		; // 31

	// Group 014 : AUD_DMA_G1
	unsigned int  aud_a8_base		; // 00
	unsigned int  aud_a8_length		; // 01
	unsigned int  aud_a8_ptr		; // 02
	unsigned int  aud_a8_cnt		; // 03
	unsigned int  aud_a9_base		; // 04
	unsigned int  aud_a9_length		; // 05
	unsigned int  aud_a9_ptr		; // 06
	unsigned int  aud_a9_cnt		; // 07
	unsigned int  aud_a10_base		; // 08
	unsigned int  aud_a10_length		; // 09
	unsigned int  aud_a10_ptr		; // 10
	unsigned int  aud_a10_cnt		; // 11
	unsigned int  aud_a11_base		; // 12
	unsigned int  aud_a11_length		; // 13
	unsigned int  aud_a11_ptr		; // 14
	unsigned int  aud_a11_cnt		; // 15
	unsigned int  aud_a12_base		; // 16
	unsigned int  aud_a12_length		; // 17
	unsigned int  aud_a12_ptr		; // 18
	unsigned int  aud_a12_cnt		; // 19
	unsigned int  aud_a13_base		; // 20
	unsigned int  aud_a13_length		; // 21
	unsigned int  aud_a13_ptr		; // 22
	unsigned int  aud_a13_cnt		; // 23
	unsigned int  aud_a14_base		; // 24
	unsigned int  aud_a14_length		; // 25
	unsigned int  aud_a14_ptr		; // 26
	unsigned int  aud_a14_cnt		; // 27
	unsigned int  aud_a15_base		; // 28
	unsigned int  aud_a15_length		; // 29
	unsigned int  aud_a15_ptr		; // 30
	unsigned int  aud_a15_cnt		; // 31

	// Group 015 : AUD_DMA_G2
	unsigned int  aud_a16_base		; // 00
	unsigned int  aud_a16_length		; // 01
	unsigned int  aud_a16_ptr		; // 02
	unsigned int  aud_a16_cnt		; // 03
	unsigned int  aud_a17_base		; // 04
	unsigned int  aud_a17_length		; // 05
	unsigned int  aud_a17_ptr		; // 06
	unsigned int  aud_a17_cnt		; // 07
	unsigned int  aud_a18_base		; // 08
	unsigned int  aud_a18_length		; // 09
	unsigned int  aud_a18_ptr		; // 10
	unsigned int  aud_a18_cnt		; // 11
	unsigned int  aud_a19_base		; // 12
	unsigned int  aud_a19_length		; // 13
	unsigned int  aud_a19_ptr		; // 14
	unsigned int  aud_a19_cnt		; // 15
	unsigned int  aud_a20_base		; // 16
	unsigned int  aud_a20_length		; // 17
	unsigned int  aud_a20_ptr		; // 18
	unsigned int  aud_a20_cnt		; // 19
	unsigned int  aud_a21_base		; // 20
	unsigned int  aud_a21_length		; // 21
	unsigned int  aud_a21_ptr		; // 22
	unsigned int  aud_a21_cnt		; // 23
	unsigned int  aud_a22_base		; // 24
	unsigned int  aud_a22_length		; // 25
	unsigned int  aud_a22_ptr		; // 26
	unsigned int  aud_a22_cnt		; // 27
	unsigned int  aud_a23_base		; // 28
	unsigned int  aud_a23_length		; // 29
	unsigned int  aud_a23_ptr		; // 30
	unsigned int  aud_a23_cnt		; // 31

	// Group 016 : AUD_GRM
	unsigned int  aud_grm_master_gain	; // 00 Gain Control
	unsigned int  aud_grm_gain_control_0	; // 01 Gain Control
	unsigned int  aud_grm_gain_control_1	; // 02 Gain Control
	unsigned int  aud_grm_gain_control_2	; // 03 Gain Control
	unsigned int  aud_grm_gain_control_3	; // 04 Gain Control
	unsigned int  aud_grm_gain_control_4	; // 05 Gain Control
	unsigned int  aud_grm_mix_control_0	; // 06 Mixer Setting
	unsigned int  aud_grm_mix_control_1	; // 07 Mixer Setting
	unsigned int  aud_grm_mix_control_2	; // 08 Mixer Setting
	unsigned int  aud_grm_switch_0		; // 09 Channel Switch
	unsigned int  aud_grm_switch_1		; // 10 Channel Switch
	unsigned int  aud_grm_switch_int	; // 11 Channel Switch
	unsigned int  aud_grm_delta_volume	; // 12 Gain Update
	unsigned int  aud_grm_delta_ramp_pcm	; // 13 Gain Update
	unsigned int  aud_grm_delta_ramp_risc	; // 14 Gain Update
	unsigned int  aud_grm_delta_ramp_linein	; // 15 Gain Update
	unsigned int  aud_grm_other		; // 16 Other Setting
	unsigned int  aud_grm_gain_control_5	; // 17 Gain Control
	unsigned int  aud_grm_gain_control_6	; // 18 Gain Control
	unsigned int  aud_grm_gain_control_7	; // 19 Gain Control
	unsigned int  aud_grm_gain_control_8	; // 20 Gain Control
	unsigned int  aud_grm_fifo_eflag	; // 21 FIFO Error Flag
	unsigned int  G016_reserved_22		; // 22 aud_grm_gain_control_9
	unsigned int  G016_reserved_23		; // 23 aud_grm_gain_control_10
	unsigned int  aud_grm_switch_hdmi_tx	; // 24 AUD_GRM_SWITCH_HDMI_TX
	unsigned int  G016_reserved_25		; // 25 aud_grm_gain_control_11
	unsigned int  G016_reserved_26		; // 26 aud_grm_tdm_switch_0
	unsigned int  G016_reserved_27		; // 27 aud_grm_tdm_switch_1
	unsigned int  G016_reserved_28		; // 28 aud_grm_gain_control_12
	unsigned int  G016_reserved_29		; // 29 aud_grm_mix_control_3
	unsigned int  aud_grm_path_select	; // 30 aud_grm_path_select
	unsigned int  G016_reserved_31		; // 31

	// Group 017 : AUD_MONITOR
	unsigned int  G017_AUD[32];

	// Group 018 : AUD_REG_G4
	unsigned int  G018_reserved_00		; // 00
	unsigned int  G018_reserved_01		; // 01
	unsigned int  G018_reserved_02		; // 02
	unsigned int  G018_reserved_03		; // 03
	unsigned int  G018_reserved_04		; // 04
	unsigned int  G018_reserved_05		; // 05
	unsigned int  G018_reserved_06		; // 06
	unsigned int  G018_reserved_07		; // 07
	unsigned int  G018_reserved_08		; // 08
	unsigned int  G018_reserved_09		; // 09
	unsigned int  G018_reserved_10		; // 10
	unsigned int  G018_reserved_11		; // 11
	unsigned int  G018_reserved_12		; // 12
	unsigned int  G018_reserved_13		; // 13
	unsigned int  G018_reserved_14		; // 14
	unsigned int  i2s_pwm_control_1		; // 15
	unsigned int  i2s_pwm_control_2		; // 16
	unsigned int  i2s_pwm_control_3		; // 17
	unsigned int  i2s_pwm_control_4		; // 18
	unsigned int  classd_mos_control	; // 19
	unsigned int  G018_reserved_20		; // 20 iec0_par2_in
	unsigned int  G018_reserved_21		; // 21 iec0_par3_in
	unsigned int  G018_reserved_22		; // 22 iec0_par4_in
	unsigned int  G018_reserved_23		; // 23 iec0_par5_in
	unsigned int  G018_reserved_24		; // 24 iec1_par2_in
	unsigned int  G018_reserved_25		; // 25 iec1_par3_in
	unsigned int  G018_reserved_26		; // 26 iec1_par4_in
	unsigned int  G018_reserved_27		; // 27 iec1_par5_in
	unsigned int  G018_reserved_28		; // 28 iec2_par2_in
	unsigned int  G018_reserved_29		; // 29 iec2_par3_in
	unsigned int  G018_reserved_30		; // 30 iec2_par4_in
	unsigned int  G018_reserved_31		; // 31 iec2_par5_in

	// Group 019 : AUD_REG_G5
	unsigned int  G019_RESERVED[32];

	// Group 020 : AUD_DMA_G3
	unsigned int  aud_a24_base		; // 00
	unsigned int  aud_a24_length		; // 01
	unsigned int  aud_a24_ptr		; // 02
	unsigned int  aud_a24_cnt		; // 03
	unsigned int  aud_a25_base		; // 04
	unsigned int  aud_a25_length		; // 05
	unsigned int  aud_a25_ptr		; // 06
	unsigned int  aud_a25_cnt		; // 07
	unsigned int  aud_a26_base		; // 08
	unsigned int  aud_a26_length		; // 09
	unsigned int  aud_a26_ptr		; // 10
	unsigned int  aud_a26_cnt		; // 11
	unsigned int  aud_a27_base		; // 12
	unsigned int  aud_a27_length		; // 13
	unsigned int  aud_a27_ptr		; // 14
	unsigned int  aud_a27_cnt		; // 15
	unsigned int  aud_a28_base		; // 16
	unsigned int  aud_a28_length		; // 17
	unsigned int  aud_a28_ptr		; // 18
	unsigned int  aud_a28_cnt		; // 19
	unsigned int  aud_a29_base		; // 20
	unsigned int  aud_a29_length		; // 21
	unsigned int  aud_a29_ptr		; // 22
	unsigned int  aud_a29_cnt		; // 23
	unsigned int  G020_reserved_24		; // 24
	unsigned int  G020_reserved_25		; // 25
	unsigned int  G020_reserved_26		; // 26
	unsigned int  G020_reserved_27		; // 27
	unsigned int  G020_reserved_28		; // 28
	unsigned int  G020_reserved_29		; // 29
	unsigned int  G020_reserved_30		; // 30
	unsigned int  G020_reserved_31		; // 31

	// Group 021 : AUD_REG_G6
	unsigned int  tdm_rx_cfg0		; //0
	unsigned int  tdm_rx_cfg1		; //1
	unsigned int  tdm_rx_cfg2		; //2
	unsigned int  tdm_rx_cfg3		; //3
	unsigned int  tdm_rx_info_0		; //4 tdm_rx_info_0
	unsigned int  tdm_rx_info_1		; //5 tdm_rx_info_1
	unsigned int  tdm_tx_cfg0		; //6
	unsigned int  tdm_tx_cfg1		; //7
	unsigned int  tdm_tx_cfg2		; //8
	unsigned int  tdm_tx_cfg3		; //9
	unsigned int  tdm_tx_info_0		; //10 tdm_tx_info_0
	unsigned int  tdm_tx_info_1		; //11 tdm_tx_info_1
	unsigned int  tdm_tx_info_2		; //12 tdm_tx_info_2
	unsigned int  G021_reserved_13		; //13
	unsigned int  pdm_rx_cfg0		; //14
	unsigned int  pdm_rx_cfg1		; //15
	unsigned int  pdm_rx_cfg2		; //16
	unsigned int  pdm_rx_cfg3		; //17
	unsigned int  pdm_rx_cfg4		; //18
	unsigned int  pdm_rx_cfg5		; //19
	unsigned int  G021_reserved_20		; //20
	unsigned int  G021_reserved_21		; //21
	unsigned int  tdm_tx_xck_cfg		; //22
	unsigned int  tdm_tx_bck_cfg		; //23
	unsigned int  tdm_rx_xck_cfg		; //24
	unsigned int  tdm_rx_bck_cfg		; //25 TDM_RX_BCK_CFG
	unsigned int  pdm_rx_xck_cfg		; //26
	unsigned int  pdm_rx_bck_cfg		; //27
	unsigned int  hdmi_rx_xck_cfg		; //28 hdmi_rx_xck_cfg
	unsigned int  hdmi_rx_bck_cfg		; //29 hdmi_rx_bck_cfg
	unsigned int  tdmpdm_tx_sel		; //30
	unsigned int  aud_ext_dac_xck1_cfg	; //31 aud_ext_dac_xck1_cfg

	// Group 022 : AUD_REG_G7
	unsigned int G022_RESERVED[32];
} register_audio;

#define	reg_aud_grm_master_gain		6700
#define	reg_aud_grm_gain_control_0	6701
#define	reg_aud_grm_gain_control_1	6702
#define	reg_aud_grm_gain_control_2	6703
#define	reg_aud_grm_gain_control_3	6704
#define	reg_aud_grm_gain_control_4	6705
#define	reg_aud_grm_mix_control_0	6706
#define	reg_aud_grm_mix_control_1	6707
#define	reg_aud_grm_mix_control_2	6708
#define	reg_aud_grm_switch_0		6709
#define	reg_aud_grm_switch_1		6710
#define	reg_aud_grm_switch_int		6711
#define	reg_aud_grm_delta_volume	6712
#define	reg_aud_grm_delta_ramp_pcm	6713
#define	reg_aud_grm_delta_ramp_risc	6714
#define	reg_aud_grm_delta_ramp_linein	6715
#define	reg_aud_grm_other		6716
#define	reg_aud_grm_gain_control_5	6717
#define	reg_aud_grm_gain_control_6	6718
#define	reg_aud_grm_gain_control_7	6719
#define	reg_aud_grm_gain_control_8	6720
#define	reg_aud_grm_fifo_eflag		6721
#define	reg_iec_tx_interfacegain	6722
#define	reg_i2s_tx_interfacegain	6723
#define	reg_aud_grm_switch_hdmi		6724
#define	reg_aud_grm_switch_hdmi_tx	6724
#define	reg_aud_grm_gain_control_10	6725

/**********************************************************
 *			Function Prototype
 **********************************************************/
void delay_ms(unsigned int ms_count);
int sunplus_i2s_register(struct	device *dev);
int sunplus_tdm_register(struct	device *dev);
#endif /* _SPSOC_UTIL_H	*/

