// SPDX-License-Identifier: GPL-2.0
// ALSA	SoC SP7350 aud driver
//
// Author: ChingChou Huang <chingchouhuang@sunplus.com>
//
//

#include <linux/module.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/of_platform.h>
#include <linux/i2c.h>
#include "aud_hw.h"
#include "spsoc_util.h"
#if IS_ENABLED(CONFIG_SND_SOC_ES8316)
#define ES8316_RESET		0x00

/* Clock Management */
#define ES8316_CLKMGR_CLKSW	0x01
#define ES8316_CLKMGR_CLKSEL	0x02
#define ES8316_CLKMGR_ADCOSR	0x03
#define ES8316_CLKMGR_ADCDIV1	0x04
#define ES8316_CLKMGR_ADCDIV2	0x05
#define ES8316_CLKMGR_DACDIV1	0x06
#define ES8316_CLKMGR_DACDIV2	0x07
#define ES8316_CLKMGR_CPDIV	0x08

/* Serial Data Port Control */
#define ES8316_SERDATA1		0x09
#define ES8316_SERDATA_ADC	0x0a
#define ES8316_SERDATA_DAC	0x0b

/* System Control */
#define ES8316_SYS_VMIDSEL	0x0c
#define ES8316_SYS_PDN		0x0d
#define ES8316_SYS_LP1		0x0e
#define ES8316_SYS_LP2		0x0f
#define ES8316_SYS_VMIDLOW	0x10
#define ES8316_SYS_VSEL		0x11
#define ES8316_SYS_REF		0x12

/* Headphone Mixer */
#define ES8316_HPMIX_SEL	0x13
#define ES8316_HPMIX_SWITCH	0x14
#define ES8316_HPMIX_PDN	0x15
#define ES8316_HPMIX_VOL	0x16

/* Charge Pump Headphone driver */
#define ES8316_CPHP_OUTEN	0x17
#define ES8316_CPHP_ICAL_VOL	0x18
#define ES8316_CPHP_PDN1	0x19
#define ES8316_CPHP_PDN2	0x1a
#define ES8316_CPHP_LDOCTL	0x1b

/* Calibration */
#define ES8316_CAL_TYPE		0x1c
#define ES8316_CAL_SET		0x1d
#define ES8316_CAL_HPLIV	0x1e
#define ES8316_CAL_HPRIV	0x1f
#define ES8316_CAL_HPLMV	0x20
#define ES8316_CAL_HPRMV	0x21

/* ADC Control */
#define ES8316_ADC_PDN_LINSEL	0x22
#define ES8316_ADC_PGAGAIN	0x23
#define ES8316_ADC_D2SEPGA	0x24
#define ES8316_ADC_DMIC		0x25
#define ES8316_ADC_MUTE		0x26
#define ES8316_ADC_VOLUME	0x27
#define ES8316_ADC_ALC1		0x29
#define ES8316_ADC_ALC2		0x2a
#define ES8316_ADC_ALC3		0x2b
#define ES8316_ADC_ALC4		0x2c
#define ES8316_ADC_ALC5		0x2d
#define ES8316_ADC_ALC_NG	0x2e

/* DAC Control */
#define ES8316_DAC_PDN		0x2f
#define ES8316_DAC_SET1		0x30
#define ES8316_DAC_SET2		0x31
#define ES8316_DAC_SET3		0x32
#define ES8316_DAC_VOLL		0x33
#define ES8316_DAC_VOLR		0x34

/* GPIO */
#define ES8316_GPIO_SEL		0x4d
#define ES8316_GPIO_DEBOUNCE	0x4e
#define ES8316_GPIO_FLAG	0x4f

/* Test mode */
#define ES8316_TESTMODE		0x50
#define ES8316_TEST1		0x51
#define ES8316_TEST2		0x52
#define ES8316_TEST3		0x53

/*
 * Field definitions
 */

/* ES8316_RESET */
#define ES8316_RESET_CSM_ON		0x80

/* ES8316_CLKMGR_CLKSW */
#define ES8316_CLKMGR_CLKSW_MCLK_ON	0x40
#define ES8316_CLKMGR_CLKSW_BCLK_ON	0x20

/* ES8316_SERDATA1 */
#define ES8316_SERDATA1_MASTER		0x80
#define ES8316_SERDATA1_BCLK_INV	0x20

/* ES8316_SERDATA_ADC and _DAC */
#define ES8316_SERDATA2_FMT_MASK	0x3
#define ES8316_SERDATA2_FMT_I2S		0x00
#define ES8316_SERDATA2_FMT_LEFTJ	0x01
#define ES8316_SERDATA2_FMT_RIGHTJ	0x02
#define ES8316_SERDATA2_FMT_PCM		0x03
#define ES8316_SERDATA2_ADCLRP		0x20
#define ES8316_SERDATA2_LEN_MASK	0x1c
#define ES8316_SERDATA2_LEN_24		0x00
#define ES8316_SERDATA2_LEN_20		0x04
#define ES8316_SERDATA2_LEN_18		0x08
#define ES8316_SERDATA2_LEN_16		0x0c
#define ES8316_SERDATA2_LEN_32		0x10

/* ES8316_GPIO_DEBOUNCE	*/
#define ES8316_GPIO_ENABLE_INTERRUPT		0x02

/* ES8316_GPIO_FLAG */
#define ES8316_GPIO_FLAG_GM_NOT_SHORTED		0x02
#define ES8316_GPIO_FLAG_HP_NOT_INSERTED	0x04

static int es8316_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = asoc_rtd_to_codec(rtd, 0)->component;
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(component);
	int regVal;
	u8 mask;

	snd_soc_dapm_force_enable_pin(dapm, "Bias");
	snd_soc_dapm_force_enable_pin(dapm, "Analog power");
	snd_soc_dapm_force_enable_pin(dapm, "Mic Bias");
	snd_soc_dapm_force_enable_pin(dapm, "ADC Clock");
	snd_soc_dapm_force_enable_pin(dapm, "ADC Vref");
	snd_soc_dapm_force_enable_pin(dapm, "ADC bias");
	snd_soc_dapm_force_enable_pin(dapm, "Mono ADC");
	snd_soc_dapm_force_enable_pin(dapm, "DAC Clock");
	snd_soc_dapm_force_enable_pin(dapm, "DAC Vref");
	snd_soc_dapm_force_enable_pin(dapm, "Left DAC");
	snd_soc_dapm_force_enable_pin(dapm, "Right DAC");
	snd_soc_dapm_force_enable_pin(dapm, "Line input PGA");
	snd_soc_dapm_force_enable_pin(dapm, "Digital Mic Mux");
	snd_soc_dapm_force_enable_pin(dapm, "Line input PGA");
	snd_soc_dapm_force_enable_pin(dapm, "Left Headphone Mux");
	snd_soc_dapm_force_enable_pin(dapm, "Right Headphone Mux");
	snd_soc_dapm_force_enable_pin(dapm, "Left Headphone Mixer");
	snd_soc_dapm_force_enable_pin(dapm, "Right Headphone Mixer");
	snd_soc_dapm_force_enable_pin(dapm, "Left Headphone Mixer Out");
	snd_soc_dapm_force_enable_pin(dapm, "Right Headphone Mixer Out");
	snd_soc_dapm_force_enable_pin(dapm, "Headphone Charge Pump");
	snd_soc_dapm_force_enable_pin(dapm, "Headphone Charge Pump Clock");
	snd_soc_dapm_force_enable_pin(dapm, "Left Headphone Charge Pump");
	snd_soc_dapm_force_enable_pin(dapm, "Right Headphone Charge Pump");
	snd_soc_dapm_force_enable_pin(dapm, "Left Headphone Driver");
	snd_soc_dapm_force_enable_pin(dapm, "Right Headphone Driver");
	snd_soc_dapm_force_enable_pin(dapm, "Left Headphone ical");
	snd_soc_dapm_force_enable_pin(dapm, "Right Headphone ical");
	snd_soc_dapm_force_enable_pin(dapm, "Headphone Out");
	snd_soc_dapm_force_enable_pin(dapm, "I2S OUT");

	snd_soc_dapm_sync(dapm);
	msleep(20);

	//snd_soc_component_update_bits(component, ES8316_SYS_PDN, 0x08, 0);//Bias
	//snd_soc_component_update_bits(component, ES8316_SYS_PDN, 0x10, 0);//Analog power
	//snd_soc_component_update_bits(component, ES8316_SYS_PDN, 0x20, 0);//Mic Bias
	//snd_soc_component_update_bits(component, ES8316_SYS_PDN, 0x02, 0);//ADC Vref
	//snd_soc_component_update_bits(component, ES8316_SYS_PDN, 0x04, 0);//ADC bias
	//snd_soc_component_update_bits(component, ES8316_SYS_PDN, 0x01, 0);//DAC Vref

	//snd_soc_component_update_bits(component, ES8316_CLKMGR_CLKSW, 0x10, 0x10);//Headphone Charge Pump Clock
	snd_soc_component_update_bits(component, ES8316_CLKMGR_CLKSW, 0x60, 0x60);

	snd_soc_component_update_bits(component, ES8316_SERDATA_DAC,
				      ES8316_SERDATA2_LEN_MASK, ES8316_SERDATA2_LEN_16);

	mask = ES8316_SERDATA1_MASTER | ES8316_SERDATA1_BCLK_INV | 0x10;
	snd_soc_component_update_bits(component, ES8316_SERDATA1, mask, ES8316_SERDATA1_MASTER);

	mask = ES8316_SERDATA2_FMT_MASK | ES8316_SERDATA2_ADCLRP;
	snd_soc_component_update_bits(component, ES8316_SERDATA_ADC, mask, 0);
	snd_soc_component_update_bits(component, ES8316_SERDATA_DAC, mask, 0);

	mask = 0xc0;
	snd_soc_component_update_bits(component, ES8316_DAC_SET1, mask, 0);//DAC Source Mux

	//mask = 0x70;
	//snd_soc_component_update_bits(component, ES8316_HPMIX_SEL, mask, 0x10);//Left Headphone Mux
	//mask = 0x07;
	//snd_soc_component_update_bits(component, ES8316_HPMIX_SEL, mask, 0x01);//Right Headphone Mux

	snd_soc_component_update_bits(component, ES8316_ADC_PDN_LINSEL, 0x30, 0x10);//Differential Mux
	//snd_soc_component_update_bits(component, ES8316_ADC_PDN_LINSEL, 0x80, 0x00);//Line input PGA
	//snd_soc_component_update_bits(component, ES8316_ADC_PDN_LINSEL, 0x40, 0x00);//Mono ADC

	snd_soc_component_write(component, ES8316_SYS_LP1, 0);//0x3f
	snd_soc_component_write(component, ES8316_SYS_LP2, 0);//0x1f

	snd_soc_component_update_bits(component, ES8316_CPHP_ICAL_VOL, 0x30, 0x00);//Headphone Playback Volume
	//snd_soc_component_update_bits(component, ES8316_CPHP_ICAL_VOL, 0x80, 0x00);//Left Headphone ical
	//snd_soc_component_update_bits(component, ES8316_CPHP_ICAL_VOL, 0x08, 0x00);//Right Headphone ical

	snd_soc_component_update_bits(component, ES8316_ADC_DMIC, 0x03, 0);//Digital Mic Mux

	snd_soc_component_update_bits(component, ES8316_HPMIX_SWITCH, 0x80, 0x80);//Left DAC Switch
	snd_soc_component_update_bits(component, ES8316_HPMIX_SWITCH, 0x40, 0x00);//LLIN Switch
	snd_soc_component_update_bits(component, ES8316_HPMIX_SWITCH, 0x08, 0x08);//Right DAC Switch
	snd_soc_component_update_bits(component, ES8316_HPMIX_SWITCH, 0x04, 0x00);//"RLIN Switch

	//snd_soc_component_update_bits(component, ES8316_HPMIX_PDN, 0x01, 0x0);//Right Headphone Mixer Out
	//snd_soc_component_update_bits(component, ES8316_HPMIX_PDN, 0x02, 0x0);//Right Headphone Mixer
	//snd_soc_component_update_bits(component, ES8316_HPMIX_PDN, 0x10, 0x0);//Left Headphone Mixer Out
	//snd_soc_component_update_bits(component, ES8316_HPMIX_PDN, 0x20, 0x0);//Left Headphone Mixer

	snd_soc_component_update_bits(component, ES8316_HPMIX_VOL, 0xf0, 0xb0);//Headphone Mixer Volume
	snd_soc_component_update_bits(component, ES8316_HPMIX_VOL, 0x0f, 0x0b);

	//snd_soc_component_update_bits(component, ES8316_CPHP_PDN2, 0x20, 0x00);//Headphone Charge Pump
	snd_soc_component_update_bits(component, ES8316_CPHP_PDN2, 0x1f, 0x10);

	mask = 0xff;
	snd_soc_component_update_bits(component, ES8316_CPHP_LDOCTL, mask, 0x30);

	//snd_soc_component_update_bits(component, ES8316_CPHP_PDN1, 0x04, 0x00);//Headphone Out
	snd_soc_component_update_bits(component, ES8316_CPHP_PDN1, 0x03, 0x02);

	//snd_soc_component_update_bits(component, ES8316_DAC_PDN, 0x01, 0);//Right DAC
	//snd_soc_component_update_bits(component, ES8316_DAC_PDN, 0x10, 0);//Left DAC

	//snd_soc_component_update_bits(component, ES8316_CPHP_OUTEN, 0x40, 0x40);//Left Headphone Charge Pump
	//snd_soc_component_update_bits(component, ES8316_CPHP_OUTEN, 0x20, 0x20);//Left Headphone Driver
	//snd_soc_component_update_bits(component, ES8316_CPHP_OUTEN, 0x04, 0x04);//Right Headphone Charge Pump
	//snd_soc_component_update_bits(component, ES8316_CPHP_OUTEN, 0x02, 0x02);//Right Headphone Driver

	snd_soc_component_write(component, ES8316_DAC_VOLL, 0);//DAC Playback Volume
	snd_soc_component_write(component, ES8316_DAC_VOLR, 0);//DAC Playback Volume

	snd_soc_component_write(component, ES8316_ADC_VOLUME, 0);//ADC Capture Volume

	mask = 0xf0;
	snd_soc_component_update_bits(component, ES8316_ADC_PGAGAIN, mask, 0xa0);//ADC PGA Gain Volume

	//snd_soc_component_update_bits(component, ES8316_CLKMGR_CLKSW, 0x08, 0x08);//ADC Clock
	//snd_soc_component_update_bits(component, ES8316_CLKMGR_CLKSW, 0x04, 0x04);//DAC Clock
	snd_soc_component_update_bits(component, ES8316_CLKMGR_CLKSW, 0x03, 0x03);

	return 0;
}
#endif
static int spsoc_hw_params(struct snd_pcm_substream *substream,	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd	= asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	unsigned int pll_out, fmt;
	int ret	= 0;

	pll_out	= params_rate(params);
	fmt = params_format(params);
	pr_debug("%s IN, pull_out %d fmt %d channels %d\n", __func__, pll_out, fmt,
		 params_channels(params));
	pr_debug("buffer_size 0x%x buffer_bytes 0x%x\n", params_buffer_size(params),
		 params_buffer_bytes(params));

	ret = snd_soc_dai_set_fmt(cpu_dai, fmt);
	switch (pll_out) {
	case 8000:
	case 16000:
	case 32000:
	case 44100:
	case 48000:
	case 64000:
	case 96000:
	case 192000:
		ret = snd_soc_dai_set_pll(cpu_dai, substream->pcm->device, substream->stream,
					  fmt, pll_out);
		break;
//#if 0
//	case 11025:
//	case 22050:
//	case 44100:
//	case 88200:
//	case 176400:
//ret = snd_soc_dai_set_pll(cpu_dai, substream->pcm->device, substream->stream, DPLL_FRE, pll_out);
//		break;
//#endif
	default:
		pr_err("NO support the rate");
		break;
	}
	//if( substream->stream	== SNDRV_PCM_STREAM_CAPTURE)
	//	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBM_CFM);

	pr_debug("%s OUT\n", __func__);
	if (ret	< 0)
		return ret;
	return 0;
}

static const struct snd_soc_ops spsoc_aud_ops	= {
	.hw_params = spsoc_hw_params,
};

SND_SOC_DAILINK_DEFS(sp_i2s_0,
		     DAILINK_COMP_ARRAY(COMP_CPU("spsoc-i2s-dai-0")),
		     DAILINK_COMP_ARRAY(COMP_CODEC("aud-codec", "aud-codec-i2s-dai-0")),
		     DAILINK_COMP_ARRAY(COMP_PLATFORM("spsoc-pcm-driver")));

SND_SOC_DAILINK_DEFS(sp_i2s_1,
		     DAILINK_COMP_ARRAY(COMP_CPU("spsoc-i2s-dai-1")),
		     DAILINK_COMP_ARRAY(COMP_CODEC("aud-codec", "aud-codec-i2s-dai-1")),
		     DAILINK_COMP_ARRAY(COMP_PLATFORM("spsoc-pcm-driver")));

SND_SOC_DAILINK_DEFS(sp_i2s_2,
		     DAILINK_COMP_ARRAY(COMP_CPU("spsoc-i2s-dai-2")),
		     DAILINK_COMP_ARRAY(COMP_CODEC("aud-codec", "aud-codec-i2s-dai-2")),
		     DAILINK_COMP_ARRAY(COMP_PLATFORM("spsoc-pcm-driver")));

SND_SOC_DAILINK_DEFS(sp_tdm,
		     DAILINK_COMP_ARRAY(COMP_CPU("spsoc-tdm-driver-dai")),
		     DAILINK_COMP_ARRAY(COMP_CODEC("aud-codec", "aud-codec-tdm-dai")),
		     DAILINK_COMP_ARRAY(COMP_PLATFORM("spsoc-pcm-driver")));

SND_SOC_DAILINK_DEFS(sp_spdif,
		     DAILINK_COMP_ARRAY(COMP_CPU("spsoc-spdif-dai")),
		     DAILINK_COMP_ARRAY(COMP_CODEC("aud-codec", "aud-spdif-dai")),
		     DAILINK_COMP_ARRAY(COMP_PLATFORM("spsoc-pcm-driver")));

#if IS_ENABLED(CONFIG_SND_SOC_ES8326_SUNPLUS)
SND_SOC_DAILINK_DEFS(es8326,
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_CODEC("es8326.0-0019", "ES8326 HiFi")),
		     DAILINK_COMP_ARRAY(COMP_PLATFORM("spsoc-pcm-driver")));
#endif

#if IS_ENABLED(CONFIG_SND_SOC_ES8316)
SND_SOC_DAILINK_DEFS(es8316,
		     DAILINK_COMP_ARRAY(COMP_DUMMY()),
		     DAILINK_COMP_ARRAY(COMP_CODEC("es8316.0-0011", "ES8316 HiFi")),
		     DAILINK_COMP_ARRAY(COMP_PLATFORM("spsoc-pcm-driver")));
#endif

static struct snd_soc_dai_link spsoc_aud_dai[] = {
	{
		.name		= "aud_i2s_0",
		.stream_name	= "aud_dac0",
		.ops		= &spsoc_aud_ops,
		SND_SOC_DAILINK_REG(sp_i2s_0),
	},
	{
		.name		= "aud_tdm",
		.stream_name	= "aud_tdm0",
		.ops		= &spsoc_aud_ops,
		SND_SOC_DAILINK_REG(sp_tdm),
	},
	{
		.name		= "aud_i2s_1",
		.stream_name	= "aud_dac1",
		.ops		= &spsoc_aud_ops,
		SND_SOC_DAILINK_REG(sp_i2s_1),
	},
	{
		.name		= "aud_i2s_2",
		.stream_name	= "aud_dac2",
		.ops		= &spsoc_aud_ops,
		SND_SOC_DAILINK_REG(sp_i2s_2),
	},
	{
		.name		= "aud_spdif",
		.stream_name	= "aud_spdif0",
		.ops		= &spsoc_aud_ops,
		SND_SOC_DAILINK_REG(sp_spdif),
	},
#if IS_ENABLED(CONFIG_SND_SOC_ES8326_SUNPLUS)
	{
		.name		= "analog_es8326",
		.stream_name	= "afe",
		.ops		= &spsoc_aud_ops,
		.no_pcm		= 1,
		.dpcm_playback	= 1,
		.dpcm_capture	= 1,
		SND_SOC_DAILINK_REG(es8326),
	},
#endif
#if IS_ENABLED(CONFIG_SND_SOC_ES8316)
	{
		.name		= "analog_es8316",
		.stream_name	= "afe",
		.init 		= es8316_init,
		.ops		= &spsoc_aud_ops,
		.no_pcm		= 1,
		.dpcm_playback	= 1,
		.dpcm_capture	= 1,
		SND_SOC_DAILINK_REG(es8316),
	},
#endif
};

static struct snd_soc_card spsoc_smdk =	{
	.name		= "sp-aud", // card name
	.long_name	= "SP7350, Sunplus Technology Inc.",
	.owner		= THIS_MODULE,
	.dai_link	= spsoc_aud_dai,
	.num_links	= ARRAY_SIZE(spsoc_aud_dai),
};

static struct platform_device *spsoc_snd_device;

static int __init snd_spsoc_audio_init(void)
{
	int ret;
#if IS_ENABLED(CONFIG_SND_SOC_ES8316) || IS_ENABLED(CONFIG_SND_SOC_ES8326_SUNPLUS)
	int i;
	struct snd_soc_card *card = &spsoc_smdk;
	struct snd_soc_dai_link *dai_link;
	struct device_node *np;
	struct i2c_client *client;
	struct snd_soc_component *component;

	// Get i2c codec device name
#if IS_ENABLED(CONFIG_SND_SOC_ES8316)
	np = of_find_compatible_node(NULL, NULL, "everest,es8316");
#elif IS_ENABLED(CONFIG_SND_SOC_ES8326_SUNPLUS)
	np = of_find_compatible_node(NULL, NULL, "everest,es8326");
#endif
	if (np) {
		client = of_find_i2c_device_by_node(np);
		if (client)
			component = snd_soc_lookup_component_nolocked(&client->dev, NULL);
		else
			pr_err("### No i2c device found\n");
		//if (!of_property_read_string(np, "codec-name", &name))
			//printk("%s\n", name);
	} else {
		pr_err("### No i2c device node found\n");
	}

	for_each_card_prelinks(card, i, dai_link) {
		// Change default code name by i2c dev codec name.
		if (strstr(dai_link->codecs->name, client->name))
			dai_link->codecs->name = component->name;
	}
#endif

	spsoc_snd_device = platform_device_alloc("soc-audio", -1);
	if (!spsoc_snd_device)
		return -ENOMEM;

	pr_info("%s, create soc_card\n", __func__);
	platform_set_drvdata(spsoc_snd_device, &spsoc_smdk);

	ret = platform_device_add(spsoc_snd_device);
	if (ret)
		platform_device_put(spsoc_snd_device);

	return ret;
}
module_init(snd_spsoc_audio_init);

static void __exit snd_spsoc_audio_exit(void)
{
	platform_device_unregister(spsoc_snd_device);
}
module_exit(snd_spsoc_audio_exit);

MODULE_AUTHOR("Sunplus Technology Inc.");
MODULE_DESCRIPTION("Sunplus SP7350 audio card driver");
MODULE_LICENSE("GPL");
