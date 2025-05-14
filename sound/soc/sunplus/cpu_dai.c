// SPDX-License-Identifier: GPL-2.0
// ALSA	SoC SP7350 i2s driver
//
// Author: ChingChou Huang <chingchouhuang@sunplus.com>
//
//

#include <sound/pcm_params.h>
#include <linux/clk.h>
#include <sound/tlv.h>
#include "aud_hw.h"
#include "spsoc_pcm.h"
#include "spsoc_util.h"

void __iomem *i2saudio_base;
struct clk *cpudai_plla;
#define	AUD_FORMATS	SNDRV_PCM_FMTBIT_S16_LE
			//(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_3LE)

static const DECLARE_TLV_DB_SCALE(volume_tlv, -6000, 0,	1);
static const char * const cpm0_5_out[] = {"pcm0", "pcm5"};
static const struct soc_enum cpm0_5_out_enum = SOC_ENUM_DOUBLE(reg_aud_grm_gain_control_8,
							       16, 17, 2, cpm0_5_out);

static const struct snd_kcontrol_new aud_snd_controls[]	= {
	/* master gains	*/
	SOC_SINGLE_TLV("Playback Volume", reg_aud_grm_master_gain, 8, 2 << 22, 0, volume_tlv),
	/* Playback gains */
	SOC_DOUBLE_TLV("A0 Volume", reg_aud_grm_gain_control_5, 0, 8, 0x80, 0,	volume_tlv),
	SOC_DOUBLE_TLV("A1 Volume", reg_aud_grm_gain_control_5, 16, 24, 0x80, 0, volume_tlv),

	SOC_DOUBLE_TLV("A2 Volume", reg_aud_grm_gain_control_6, 0, 8, 0x80, 0,	volume_tlv),
	SOC_DOUBLE_TLV("A3 Volume", reg_aud_grm_gain_control_6, 16, 24, 0x80, 0, volume_tlv),

	SOC_SINGLE_TLV("A4 Volume 1", reg_aud_grm_gain_control_7, 14, 2 << 16, 0, volume_tlv),
	SOC_SINGLE_TLV("A4 Volume 2", reg_aud_grm_gain_control_8, 14, 2 << 16, 0, volume_tlv),
	SOC_DOUBLE_TLV("A20 Volume", reg_aud_grm_gain_control_10, 0, 8, 0x80, 0, volume_tlv),

	/* Mux */
	SOC_ENUM("pcm0 pcm5 Output", cpm0_5_out_enum),

	/* Mix */
	SOC_SINGLE("Mix0",	reg_aud_grm_mix_control_0,	0,	1,	0),
	SOC_SINGLE("Mix1",	reg_aud_grm_mix_control_0,	1,	1,	0),
	SOC_SINGLE("Mix2",	reg_aud_grm_mix_control_0,	2,	1,	0),
	SOC_SINGLE("Mix3",	reg_aud_grm_mix_control_0,	3,	1,	0),
	SOC_SINGLE("Mix4",	reg_aud_grm_mix_control_0,	4,	1,	0),
	SOC_SINGLE("Mix5",	reg_aud_grm_mix_control_0,	5,	1,	0),
	SOC_SINGLE("Mix6",	reg_aud_grm_mix_control_0,	6,	1,	0),
	SOC_SINGLE("Mix7",	reg_aud_grm_mix_control_0,	7,	1,	0),
	SOC_SINGLE("Mix8",	reg_aud_grm_mix_control_0,	8,	1,	0),
	SOC_SINGLE("Mix9",	reg_aud_grm_mix_control_0,	9,	1,	0),
	SOC_SINGLE("Mix30",	reg_aud_grm_mix_control_0,	10,	1,	0),
	SOC_SINGLE("Mix36",	reg_aud_grm_mix_control_0,	11,	1,	0),
	SOC_SINGLE("Mix46",	reg_aud_grm_mix_control_0,	12,	1,	0),
	SOC_SINGLE("Mix52",	reg_aud_grm_mix_control_0,	13,	1,	0),
	SOC_SINGLE("Mix72",	reg_aud_grm_mix_control_0,	14,	1,	0),

	SOC_SINGLE("Mix10",	reg_aud_grm_mix_control_0,	16,	1,	0),
	SOC_SINGLE("Mix11",	reg_aud_grm_mix_control_0,	17,	1,	0),
	SOC_SINGLE("Mix12",	reg_aud_grm_mix_control_0,	18,	1,	0),
	SOC_SINGLE("Mix13",	reg_aud_grm_mix_control_0,	19,	1,	0),
	SOC_SINGLE("Mix14",	reg_aud_grm_mix_control_0,	20,	1,	0),
	SOC_SINGLE("Mix15",	reg_aud_grm_mix_control_0,	21,	1,	0),
	SOC_SINGLE("Mix16",	reg_aud_grm_mix_control_0,	22,	1,	0),
	SOC_SINGLE("Mix17",	reg_aud_grm_mix_control_0,	23,	1,	0),
	SOC_SINGLE("Mix18",	reg_aud_grm_mix_control_0,	24,	1,	0),
	SOC_SINGLE("Mix19",	reg_aud_grm_mix_control_0,	25,	1,	0),

	SOC_SINGLE("Mix31",	reg_aud_grm_mix_control_0,	26,	1,	0),
	SOC_SINGLE("Mix37",	reg_aud_grm_mix_control_0,	27,	1,	0),
	SOC_SINGLE("Mix47",	reg_aud_grm_mix_control_0,	28,	1,	0),
	SOC_SINGLE("Mix53",	reg_aud_grm_mix_control_0,	29,	1,	0),

	SOC_SINGLE("Mix26",	reg_aud_grm_mix_control_1,	0,	1,	0),
	SOC_SINGLE("Mix27",	reg_aud_grm_mix_control_1,	1,	1,	0),
	SOC_SINGLE("Mix28",	reg_aud_grm_mix_control_1,	2,	1,	0),
	SOC_SINGLE("Mix29",	reg_aud_grm_mix_control_1,	3,	1,	0),
	SOC_SINGLE("Mix73",	reg_aud_grm_mix_control_1,	4,	1,	0),
	SOC_SINGLE("Mix74",	reg_aud_grm_mix_control_1,	5,	1,	0),
	SOC_SINGLE("Mix39",	reg_aud_grm_mix_control_1,	6,	1,	0),
	SOC_SINGLE("Mix32",	reg_aud_grm_mix_control_1,	7,	1,	0),
	SOC_SINGLE("Mix33",	reg_aud_grm_mix_control_1,	8,	1,	0),
	SOC_SINGLE("Mix34",	reg_aud_grm_mix_control_1,	9,	1,	0),
	SOC_SINGLE("Mix35",	reg_aud_grm_mix_control_1,	10,	1,	0),
	SOC_SINGLE("Mix75",	reg_aud_grm_mix_control_1,	11,	1,	0),
	SOC_SINGLE("Mix76",	reg_aud_grm_mix_control_1,	12,	1,	0),
	SOC_SINGLE("Mix40",	reg_aud_grm_mix_control_1,	13,	1,	0),
	SOC_SINGLE("Mix41",	reg_aud_grm_mix_control_1,	14,	1,	0),

	SOC_SINGLE("Mix42",	reg_aud_grm_mix_control_1,	16,	1,	0),
	SOC_SINGLE("Mix43",	reg_aud_grm_mix_control_1,	17,	1,	0),
	SOC_SINGLE("Mix44",	reg_aud_grm_mix_control_1,	18,	1,	0),
	SOC_SINGLE("Mix45",	reg_aud_grm_mix_control_1,	19,	1,	0),
	SOC_SINGLE("Mix77",	reg_aud_grm_mix_control_1,	20,	1,	0),
	SOC_SINGLE("Mix78",	reg_aud_grm_mix_control_1,	21,	1,	0),
	SOC_SINGLE("Mix56",	reg_aud_grm_mix_control_1,	22,	1,	0),
	SOC_SINGLE("Mix48",	reg_aud_grm_mix_control_1,	23,	1,	0),
	SOC_SINGLE("Mix49",	reg_aud_grm_mix_control_1,	24,	1,	0),
	SOC_SINGLE("Mix50",	reg_aud_grm_mix_control_1,	25,	1,	0),
	SOC_SINGLE("Mix51",	reg_aud_grm_mix_control_1,	26,	1,	0),
	SOC_SINGLE("Mix79",	reg_aud_grm_mix_control_1,	27,	1,	0),
	SOC_SINGLE("Mix80",	reg_aud_grm_mix_control_1,	28,	1,	0),
	SOC_SINGLE("Mix57",	reg_aud_grm_mix_control_1,	29,	1,	0),
	SOC_SINGLE("Mix55",	reg_aud_grm_mix_control_1,	30,	1,	0),

	SOC_SINGLE("Mix58",	reg_aud_grm_mix_control_2,	0,	1,	0),
	SOC_SINGLE("Mix59",	reg_aud_grm_mix_control_2,	1,	1,	0),
	SOC_SINGLE("Mix60",	reg_aud_grm_mix_control_2,	2,	1,	0),
	SOC_SINGLE("Mix61",	reg_aud_grm_mix_control_2,	3,	1,	0),
	SOC_SINGLE("Mix62",	reg_aud_grm_mix_control_2,	4,	1,	0),
	SOC_SINGLE("Mix63",	reg_aud_grm_mix_control_2,	5,	1,	0),
	SOC_SINGLE("Mix64",	reg_aud_grm_mix_control_2,	6,	1,	0),
	SOC_SINGLE("Mix65",	reg_aud_grm_mix_control_2,	7,	1,	0),
	SOC_SINGLE("Mix66",	reg_aud_grm_mix_control_2,	8,	1,	0),
	SOC_SINGLE("Mix67",	reg_aud_grm_mix_control_2,	9,	1,	0),
	SOC_SINGLE("Mix68",	reg_aud_grm_mix_control_2,	10,	1,	0),
	SOC_SINGLE("Mix69",	reg_aud_grm_mix_control_2,	11,	1,	0),
	SOC_SINGLE("Mix70",	reg_aud_grm_mix_control_2,	12,	1,	0),
	SOC_SINGLE("Mix71",	reg_aud_grm_mix_control_2,	13,	1,	0),

	SOC_SINGLE("Mix20",	reg_aud_grm_mix_control_2,	16,	1,	0),
	SOC_SINGLE("Mix21",	reg_aud_grm_mix_control_2,	17,	1,	0),
	SOC_SINGLE("Mix22",	reg_aud_grm_mix_control_2,	18,	1,	0),
	SOC_SINGLE("Mix23",	reg_aud_grm_mix_control_2,	19,	1,	0),
	SOC_SINGLE("Mix24",	reg_aud_grm_mix_control_2,	20,	1,	0),
	SOC_SINGLE("Mix25",	reg_aud_grm_mix_control_2,	21,	1,	0),
	SOC_SINGLE("Mix38",	reg_aud_grm_mix_control_2,	22,	1,	0),
	SOC_SINGLE("Mix54",	reg_aud_grm_mix_control_2,	23,	1,	0),
	SOC_SINGLE("Mix81",	reg_aud_grm_mix_control_2,	24,	1,	0),
	SOC_SINGLE("Mix82",	reg_aud_grm_mix_control_2,	25,	1,	0),
	SOC_SINGLE("Mix83",	reg_aud_grm_mix_control_2,	26,	1,	0),
	SOC_SINGLE("Mix84",	reg_aud_grm_mix_control_2,	27,	1,	0),
	//SOC_SINGLE_TLV()
};

static unsigned	int audreg_read(struct snd_soc_component *component, unsigned int reg)
{
	int val, addr_g, addr_i;
	volatile u32 *addr;

	addr_i = reg % 100;
	addr_g = (reg -	addr_i)	/ 100 -	60;
	addr = (i2saudio_base	+ addr_g * 32 *	4 + addr_i * 4);
	//val =	(int)(*(volatile unsigned int *) (addr));
	val = *addr;
	//SYNCHRONIZE_IO;
	//pr_info("*val=%08x\n", val);
	return val;
}

static int audreg_write(struct snd_soc_component *component, unsigned int reg, unsigned int value)
{
	int addr_g, addr_i;
	volatile u32 *addr;

	addr_i = reg % 100;
	addr_g = (reg -	addr_i)	/ 100 -	60;
	addr = (i2saudio_base	+ addr_g * 32 *	4 + addr_i * 4);
	*addr =	value;
	//SYNCHRONIZE_IO;
	//pr_info("val=%08x\n",	(*(volatile unsigned int *)(addr)));
	return 0;
}

void aud_clk_cfg(int pll_id, int source, unsigned int SAMPLE_RATE)
{
	static int pre_plla;
	volatile register_audio *regs0 = i2saudio_base;
	int err	= 0;

	if (SAMPLE_RATE	== 44100) {
		if (pre_plla !=	135475200) {
			err = clk_set_rate(cpudai_plla,	135475200);
			pre_plla = 135475200;
		}
	} else {
		if (pre_plla !=	147456000) {
			err = clk_set_rate(cpudai_plla,	147456000);
			pre_plla = 147456000;
		}
	}
	if (err)
		pr_err("%s IN, plla set	rate false %d\n", __func__, pre_plla);

	if (source == SNDRV_PCM_FORMAT_S24_3LE)	{
		if (pll_id == SP_I2S_0)	{
			regs0->pcm_cfg			= 0x5d;	//tx0
			regs0->ext_adc_cfg		= 0x5d;	//rx0
		} else if (pll_id == SP_I2S_1) {
			regs0->int_adc_dac_cfg		= 0x005d005d;
		} else if (pll_id == SP_I2S_2) {
			regs0->hdmi_tx_i2s_cfg		= 0x5d;	//tx2
			regs0->hdmi_rx_i2s_cfg		= 0x5d;	//rx2
		} // else {}
	} else { //SNDRV_PCM_FORMAT_S16_LE
		if (pll_id == SP_I2S_0)	{
			regs0->pcm_cfg			= 0x71; //tx0
			regs0->ext_adc_cfg		= 0x71; //rx0
		} else if (pll_id == SP_I2S_1) {
			regs0->int_adc_dac_cfg		= 0x00710071;
		} else if (pll_id	== SP_I2S_2) {
			regs0->hdmi_tx_i2s_cfg		= 0x71;	//tx2
			regs0->hdmi_rx_i2s_cfg		= 0x71;	//rx2
		} // else {}
	}
	// 147M	Setting
	if (SAMPLE_RATE == 44100 || SAMPLE_RATE == 48000) {
		regs0->aud_ext_dac_xck_cfg			= 0x6883;
		//If tx1, tx2	use xck	need to	set G62.0, xck1	need to	set G92.31
		if (pll_id == SP_I2S_0)	{
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_ext_dac_bck_cfg	= 0x6003;
				//64FS. 48kHz	= 147Mhz/3/4/4/(64)
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_ext_dac_bck_cfg	= 0x6007;
				//32FS. 48kHz	= 147Mhz/3/4/8/(32)
		} else if (pll_id == SP_I2S_1) {
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_int_dac_xck_cfg	= 0x6887;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_int_dac_xck_cfg	= 0x688f;

			regs0->aud_int_dac_bck_cfg		= 0x6001;
		} else if (pll_id == SP_I2S_2) {
			regs0->aud_hdmi_tx_mclk_cfg		= 0x6883;
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_hdmi_tx_bck_cfg	= 0x6003;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_hdmi_tx_bck_cfg	= 0x6007;
		} else { //pll_id == SP_SPDIF)
			regs0->aud_ext_dac_xck_cfg		= 0x6883;
			regs0->aud_iec0_bclk_cfg		= 0x6001;
		}
	} else if ((SAMPLE_RATE	== 88200) || (SAMPLE_RATE == 96000)) {
		regs0->aud_ext_dac_xck_cfg			= 0x6881;
		if (pll_id == SP_I2S_0)	{
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_ext_dac_bck_cfg	= 0x6003;
				//64FS. 96kHz	= 147Mhz/3/2/4/(64)
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_ext_dac_bck_cfg	= 0x6007;
				//32FS. 96kHz	= 147Mhz/3/2/8/(32)
		} else if (pll_id == SP_I2S_1) {
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_int_dac_xck_cfg	= 0x6883;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_int_dac_xck_cfg	= 0x6887;

			regs0->aud_int_dac_bck_cfg		= 0x6001;
		} else if (pll_id == SP_I2S_2) {
			regs0->aud_hdmi_tx_mclk_cfg		= 0x6881;
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_hdmi_tx_bck_cfg	= 0x6003;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_hdmi_tx_bck_cfg	= 0x6007;
		} else { //pll_id == SP_SPDIF
			regs0->aud_ext_dac_xck_cfg		= 0x6881;
			regs0->aud_iec0_bclk_cfg		= 0x6001;
		}
	} else if ((SAMPLE_RATE	== 176400) || (SAMPLE_RATE == 192000)) {
		regs0->aud_ext_dac_xck_cfg			= 0x6880; //PLLA.
		if (pll_id == SP_I2S_0)	{
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_ext_dac_bck_cfg	= 0x6003;
				//64FS. 192kHz = 147Mhz/3/1/4/(64)
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_ext_dac_bck_cfg	= 0x6007;
				//32FS. 192kHz = 147Mhz/3/1/8/(32)
		} else if (pll_id == SP_I2S_1) {
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_int_dac_xck_cfg	= 0x6881;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_int_dac_xck_cfg	= 0x6883;

			regs0->aud_int_dac_bck_cfg		= 0x6001;
		} else if (pll_id == SP_I2S_2) {
			regs0->aud_hdmi_tx_mclk_cfg		= 0x6880;
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_hdmi_tx_bck_cfg	= 0x6003;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_hdmi_tx_bck_cfg	= 0x6007;
		} else {//pll_id == SP_SPDIF
			regs0->aud_ext_dac_xck_cfg		= 0x6883;
			regs0->aud_iec0_bclk_cfg		= 0x6001;
		}
	} else if (SAMPLE_RATE == 8000) {
		regs0->aud_ext_dac_xck_cfg			= 0x6981;
		if (pll_id == SP_I2S_0)	{
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_ext_dac_bck_cfg	= 0x600f;
				//64FS. 32kHz	= 147Mhz/9/2/4/(64)
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_ext_dac_bck_cfg	= 0x601f;
				//32FS. 32kHz	= 147Mhz/9/2/8/(32)
		} else if (pll_id == SP_I2S_1) {
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_int_dac_xck_cfg	= 0x688f;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_int_dac_xck_cfg	= 0x689f;

			regs0->aud_int_dac_bck_cfg		= 0x6001;
		} else if (pll_id == SP_I2S_2) {
			regs0->aud_hdmi_tx_mclk_cfg		= 0x6981;
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_hdmi_tx_bck_cfg	= 0x600f;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_hdmi_tx_bck_cfg	= 0x601f;
		} else { //pll_id == SP_SPDIF
			regs0->aud_ext_dac_xck_cfg		= 0x6981;
			regs0->aud_iec0_bclk_cfg		= 0x6003;
		}
	} else if (SAMPLE_RATE == 16000) {
		regs0->aud_ext_dac_xck_cfg			= 0x6981;
		if (pll_id == SP_I2S_0)	{
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_ext_dac_bck_cfg	= 0x6007;
				//64FS. 32kHz	= 147Mhz/9/2/4/(64)
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_ext_dac_bck_cfg	= 0x600f;
				//32FS. 32kHz	= 147Mhz/9/2/8/(32)
		} else if (pll_id == SP_I2S_1) {
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_int_dac_xck_cfg	= 0x6887;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_int_dac_xck_cfg	= 0x688f;

			regs0->aud_int_dac_bck_cfg		= 0x6001;
		} else if (pll_id == SP_I2S_2) {
			regs0->aud_hdmi_tx_mclk_cfg		= 0x6981;
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_hdmi_tx_bck_cfg	= 0x6007;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_hdmi_tx_bck_cfg	= 0x600f;
		} else { //pll_id == SP_SPDIF
			regs0->aud_ext_dac_xck_cfg		= 0x6981;
			regs0->aud_iec0_bclk_cfg		= 0x6003;
		}
	} else if (SAMPLE_RATE == 32000) {
		regs0->aud_ext_dac_xck_cfg			= 0x6981;
		if (pll_id == SP_I2S_0)	{
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_ext_dac_bck_cfg	= 0x6003;
				//64FS. 32kHz	= 147Mhz/9/2/4/(64)
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_ext_dac_bck_cfg	= 0x6007;
				//32FS. 32kHz	= 147Mhz/9/2/8/(32)
		} else if (pll_id == SP_I2S_1) {
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_int_dac_xck_cfg	= 0x6883;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_int_dac_xck_cfg	= 0x6887;

			regs0->aud_int_dac_bck_cfg		= 0x6001;
		} else if (pll_id == SP_I2S_2) {
			regs0->aud_hdmi_tx_mclk_cfg		= 0x6981;
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_hdmi_tx_bck_cfg	= 0x6003;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_hdmi_tx_bck_cfg	= 0x6007;
		} else { //pll_id == SP_SPDIF
			regs0->aud_ext_dac_xck_cfg		= 0x6981;
			regs0->aud_iec0_bclk_cfg		= 0x6003;
		}
	} else if (SAMPLE_RATE == 64000) {
		regs0->aud_ext_dac_xck_cfg			= 0x6980;
		if (pll_id == SP_I2S_0)	{
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_ext_dac_bck_cfg	= 0x6003;
				//64FS. 64kHz	= 147Mhz/9/1/4/(64)
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_ext_dac_bck_cfg	= 0x6007;
				//32FS. 64kHz	= 147Mhz/9/1/8/(32)
		} else if (pll_id == SP_I2S_1) {
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_int_dac_xck_cfg	= 0x6981;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_int_dac_xck_cfg	= 0x6983;

			regs0->aud_int_dac_bck_cfg		= 0x6001;
		} else if (pll_id == SP_I2S_2) {
			regs0->aud_hdmi_tx_mclk_cfg		= 0x6980;
			if (source == SNDRV_PCM_FORMAT_S24_3LE)
				regs0->aud_hdmi_tx_bck_cfg	= 0x6003;
			else //SNDRV_PCM_FORMAT_S16_LE
				regs0->aud_hdmi_tx_bck_cfg	= 0x6007;
		} else { //pll_id == SP_SPDIF
			regs0->aud_ext_dac_xck_cfg		= 0x6980;
			regs0->aud_iec0_bclk_cfg		= 0x6003;
		}
	} else {
		if (pll_id == SP_I2S_0)	{
			regs0->aud_ext_dac_bck_cfg		= 0;
		} else if (pll_id == SP_I2S_1) {
			regs0->aud_int_dac_xck_cfg		= 0;
			regs0->aud_int_dac_bck_cfg		= 0;
		} else if (pll_id == SP_I2S_2) {
			regs0->aud_hdmi_tx_mclk_cfg		= 0;
			regs0->aud_hdmi_tx_bck_cfg		= 0;
		} else {
			regs0->aud_ext_dac_xck_cfg		= 0;
			regs0->aud_iec0_bclk_cfg		= 0;
		}
		//regs0->aud_ext_adc_xck_cfg			= 0;
		regs0->aud_int_adc_xck_cfg			= 0;
		regs0->aud_ext_adc_bck_cfg			= 0;
		regs0->aud_iec1_bclk_cfg			= 0;
	}
}

void sp_i2s_spdif_tx_dma_en(int	dev_no,	bool on)
{
	volatile register_audio *regs0 = i2saudio_base;
	int tout = 0;

	if (dev_no == SP_I2S_0)	{
		if (on)	{
			if ((regs0->aud_fifo_enable & I2S_P_INC0))
				return;

			regs0->aud_fifo_enable	|= I2S_P_INC0;
			regs0->aud_fifo_reset	|= I2S_P_INC0;
			while ((regs0->aud_fifo_reset &	I2S_P_INC0) && tout < chktimeout)
				tout++;
			regs0->aud_enable |= aud_enable_i2stdm_p;
		} else {
			regs0->aud_fifo_enable &= (~I2S_P_INC0);
			regs0->aud_enable      &= (~aud_enable_i2stdm_p);
		}
	} else if (dev_no == SP_I2S_1) {
		if (on)	{
			if ((regs0->aud_fifo_enable & I2S_P_INC1))
				return;

			regs0->aud_fifo_enable	|= I2S_P_INC1;
			regs0->aud_fifo_reset	|= I2S_P_INC1;
			while ((regs0->aud_fifo_reset &	I2S_P_INC1) && tout < chktimeout)
				tout++;
			regs0->aud_enable |= aud_enable_i2s1_p;
		} else {
			regs0->aud_fifo_enable &= (~I2S_P_INC1);
			regs0->aud_enable      &= (~aud_enable_i2s1_p);
		}
	} else if (dev_no == SP_I2S_2) {
		if (on)	{
			if ((regs0->aud_fifo_enable & I2S_P_INC2))
				return;

			regs0->aud_fifo_enable	|= I2S_P_INC2;
			regs0->aud_fifo_reset	|= I2S_P_INC2;
			while ((regs0->aud_fifo_reset &	I2S_P_INC2) && tout < chktimeout)
				tout++;
			regs0->aud_enable |= aud_enable_i2s2_p;
		} else {
			regs0->aud_fifo_enable &= (~I2S_P_INC2);
			regs0->aud_enable      &= (~aud_enable_i2s2_p);
		}
	} else if (dev_no == SP_SPDIF) {
		if (on)	{
			if ((regs0->aud_fifo_enable & SPDIF_P_INC0))
				return;

			regs0->aud_fifo_enable	|= SPDIF_P_INC0;
			regs0->aud_fifo_reset	|= SPDIF_P_INC0;
			while ((regs0->aud_fifo_reset &	SPDIF_P_INC0) && tout < chktimeout)
				tout++;
			regs0->aud_enable |= aud_enable_spdiftx0_p;
		} else {
			regs0->aud_fifo_enable &= (~SPDIF_P_INC0);
			regs0->aud_enable      &= (~aud_enable_spdiftx0_p);
		}
	} else {
		pr_err("no support channel\n");
	}
	if (tout >= chktimeout)
		pr_err("XXX sp_i2s_spdif_tx_dma_en TIMEOUT\n");
	pr_debug("tx:fifo_enable 0x%x enable 0x%x\n", regs0->aud_fifo_enable, regs0->aud_enable);
}

void sp_i2s_spdif_rx_dma_en(int	dev_no,	bool on)
{
	volatile register_audio *regs0 = i2saudio_base;
	int tout = 0;

	if (dev_no == SP_I2S_0)	{
		if (on)	{
			if ((regs0->aud_fifo_enable & I2S_C_INC0))
				return;

			regs0->aud_fifo_enable	|= I2S_C_INC0;
			regs0->aud_fifo_reset	|= I2S_C_INC0;
			while ((regs0->aud_fifo_reset &	I2S_C_INC0) && tout < chktimeout)
				tout++;
			regs0->aud_enable |= aud_enable_i2s0_c;
		} else {
			regs0->aud_fifo_enable	&= (~I2S_C_INC0);
			regs0->aud_enable	&= (~aud_enable_i2s0_c);
		}
	} else if (dev_no == SP_I2S_1) {
		if (on)	{
			if ((regs0->aud_fifo_enable & I2S_C_INC1))
				return;

			regs0->aud_fifo_enable	|= I2S_C_INC1;
			regs0->aud_fifo_reset	|= I2S_C_INC1;
			while ((regs0->aud_fifo_reset &	I2S_C_INC1) && tout < chktimeout)
				tout++;
			regs0->aud_enable |= aud_enable_i2s1_c;
		} else {
			regs0->aud_fifo_enable	&= (~I2S_C_INC1);
			regs0->aud_enable	&= (~aud_enable_i2s1_c);
		}
	} else if (dev_no == SP_I2S_2) {
		if (on)	{
			if ((regs0->aud_fifo_enable & I2S_C_INC2))
				return;

			regs0->aud_fifo_enable	|= I2S_C_INC2;
			regs0->aud_fifo_reset	|= I2S_C_INC2;
			while ((regs0->aud_fifo_reset &	I2S_C_INC2) && tout < chktimeout)
				tout++;
			regs0->aud_enable |= aud_enable_i2s2_c;
		} else {
			regs0->aud_fifo_enable	&= (~I2S_C_INC2);
			regs0->aud_enable	&= (~aud_enable_i2s2_c);
		}
	} else { //SPDIF
		if (on)	{
			if ((regs0->aud_fifo_enable & SPDIF_C_INC0))
				return;

			regs0->aud_fifo_enable	|= SPDIF_C_INC0;
			regs0->aud_fifo_reset	|= SPDIF_C_INC0;
			while ((regs0->aud_fifo_reset &	SPDIF_C_INC0) && tout < chktimeout)
				tout++;
			regs0->aud_enable |= aud_enable_spdif_c;
		} else {
			regs0->aud_fifo_enable	&= (~SPDIF_C_INC0);
			regs0->aud_enable	&= (~aud_enable_spdif_c);
		}
	}
	if (tout >= chktimeout)
		pr_err("XXX sp_i2s_spdif_rx_dma_en TIMEOUT\n");
	pr_debug("rx:fifo_enable 0x%x enable 0x%x\n", regs0->aud_fifo_enable, regs0->aud_enable);
}

static int aud_cpudai_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "%s IN\n", __func__);
	return 0;
}

static int aud_cpudai_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *socdai)
{
	volatile register_audio *regs0 = i2saudio_base;

	pr_debug("%s, %s-params = %x\n", __func__, substream->stream ? "cap" : "play",
		 params_format(params));

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (substream->pcm->device == SP_I2S_0)	{
			regs0->aud_asrc_ctrl = 0x4B0; //[7:4]	if0  [11:8] if1
			regs0->aud_asrc_ctrl = regs0->aud_asrc_ctrl | 0x1; // enable
		}
	} else
		sp_i2s_spdif_tx_dma_en(substream->pcm->device, true);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_3LE: //0x20
	//case SNDRV_PCM_FORMAT_S24_LE:	//6
//#if 0
//		regs0->pcm_cfg		= 0x4d;	//tx0
//		regs0->ext_adc_cfg	= 0x4d;	//rx0
//		regs0->hdmi_tx_i2s_cfg	= 0x24d; //tx2 if tx2(slave) ->	rx0 -> tx1/tx0	0x24d
//		regs0->hdmi_rx_i2s_cfg	= 0x4d;	//rx2
//		regs0->int_adc_dac_cfg	= 0x024d024d; //0x001c004d // rx1 tx1
//#endif
		regs0->aud_fifo_mode	= 0x20000;
		break;
	case SNDRV_PCM_FORMAT_S16_LE: //2
//#if 0
//		regs0->pcm_cfg		= 0x61;	//tx0
//		regs0->ext_adc_cfg	= 0x61;	//rx0
//		regs0->hdmi_tx_i2s_cfg	= 0x261; //tx2 if tx2(slave) ->	rx0 -> tx1/tx0	0x24d
//		regs0->hdmi_rx_i2s_cfg	= 0x261; //rx2
//		regs0->int_adc_dac_cfg	= 0x00610061; //0x001c004d // rx1 tx1
//#endif
		regs0->aud_fifo_mode	= 0x20001;
		break;
	default:
		pr_err("Unknown	data format\n");
		return -EINVAL;
	}
	//pr_debug("%s IN! aud_asrc_ctrl 0x%x\n", __func__, regs0->aud_asrc_ctrl);
	return 0;
}

static int aud_cpudai_trigger(struct snd_pcm_substream *substream, int cmd,
			      struct snd_soc_dai *dai)
{
	int capture = (substream->stream == SNDRV_PCM_STREAM_CAPTURE);
	int ret	= 0;

	pr_debug("%s IN, cmd=%d, capture=%d\n", __func__, cmd, capture);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (capture)
			sp_i2s_spdif_rx_dma_en(substream->pcm->device, true);
		//else
		//	sp_i2s_spdif_tx_dma_en(substream->pcm->device, true);
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (capture)
			sp_i2s_spdif_rx_dma_en(substream->pcm->device, true);
		//else
		//	sp_i2s_spdif_tx_dma_en(substream->pcm->device, true);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		if (capture)
			sp_i2s_spdif_rx_dma_en(substream->pcm->device, false);
		//else
		//	sp_i2s_spdif_tx_dma_en(substream->pcm->device, false);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (capture)
			sp_i2s_spdif_rx_dma_en(substream->pcm->device, false);
		//else
		//	sp_i2s_spdif_tx_dma_en(substream->pcm->device, false);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int spsoc_cpu_set_fmt(struct snd_soc_dai	*codec_dai, unsigned int fmt)
{
	pr_debug("%s IN\n", __func__);
	return 0;
}

static void aud_cpudai_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai	*dai)
{
	int capture = (substream->stream == SNDRV_PCM_STREAM_CAPTURE);

	dev_dbg(dai->dev, "%s IN, capture %d\n", __func__, capture);
	if (capture)
		sp_i2s_spdif_rx_dma_en(substream->pcm->device, false);
	else
		sp_i2s_spdif_tx_dma_en(substream->pcm->device, false);

	if (substream->pcm->device != 0)
		aud_clk_cfg(substream->pcm->device, 0, 0);
}

static int spsoc_cpu_set_pll(struct snd_soc_dai	*dai, int pll_id, int source,
			     unsigned int freq_in, unsigned int freq_out)
{
	pr_debug("%s IN %d %d\n", __func__, freq_out, pll_id);
	aud_clk_cfg(pll_id, freq_in, freq_out);
	return 0;
}

static const struct snd_soc_dai_ops aud_dai_cpu_ops = {
	.trigger	= aud_cpudai_trigger,
	.hw_params	= aud_cpudai_hw_params,
	.set_fmt	= spsoc_cpu_set_fmt,
	.set_pll	= spsoc_cpu_set_pll,
	.startup	= aud_cpudai_startup,
	.shutdown	= aud_cpudai_shutdown,
};

static struct snd_soc_dai_driver aud_cpu_dai[] = {
	{
		.name =	"spsoc-i2s-dai-0",
		.playback = {
			.channels_min	= 2,
			.channels_max	= 2,
			.rates		= AUD_RATES,
			.formats	= AUD_FORMATS,
		},
		.capture = {
			.channels_min	= 2,
			.channels_max	= 2,
			.rates		= AUD_RATES,
			.formats	= AUD_FORMATS,
		},
		.ops = &aud_dai_cpu_ops
	},
	{
		.name =	"spsoc-i2s-dai-1",
		.playback = {
			.channels_min	= 2,
			.channels_max	= 2,
			.rates		= AUD_RATES,
			.formats	= AUD_FORMATS,
		},
		.capture = {
			.channels_min	= 2,
			.channels_max	= 2,
			.rates		= AUD_RATES,
			.formats	= AUD_FORMATS,
		},
		.ops = &aud_dai_cpu_ops
	},
	{
		.name =	"spsoc-i2s-dai-2",
		.playback = {
			.channels_min	= 2,
			.channels_max	= 2,
			.rates		= AUD_RATES,
			.formats	= AUD_FORMATS,
		},
		.capture = {
			.channels_min	= 2,
			.channels_max	= 2,
			.rates		= AUD_RATES,
			.formats	= AUD_FORMATS,
		},
		.ops = &aud_dai_cpu_ops
	},
	{
		.name =	"spsoc-spdif-dai",
		.playback = {
			.channels_min	= 1,
			.channels_max	= 2,
			.rates		= AUD_RATES,
			.formats	= AUD_FORMATS,
		},
		.capture = {
			.channels_min	= 1,
			.channels_max	= 2,
			.rates		= AUD_RATES,
			.formats	= AUD_FORMATS,
		},
		.ops = &aud_dai_cpu_ops
	},
};

static const struct snd_soc_component_driver sunplus_cpu_component = {
	.name 		= "spsoc-cpu-dai",
	.read		= audreg_read,
	.write		= audreg_write,
	.controls	= aud_snd_controls,
	.num_controls	= ARRAY_SIZE(aud_snd_controls),
};

int sunplus_i2s_register(struct	device *dev)
{
	int ret	= 0;
	struct sunplus_audio_base *spauddata = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);
	i2saudio_base =	spauddata->audio_base;
	cpudai_plla = spauddata->aud_clocken;

	AUDHW_pin_mx();
	AUDHW_Mixer_Setting(spauddata);
	AUDHW_SystemInit(spauddata);
	snd_aud_config(spauddata);
	pr_debug("SP7350 aud set done\n");

	ret = devm_snd_soc_register_component(dev, &sunplus_cpu_component, aud_cpu_dai,
					      ARRAY_SIZE(aud_cpu_dai));
	return ret;
}
EXPORT_SYMBOL_GPL(sunplus_i2s_register);
