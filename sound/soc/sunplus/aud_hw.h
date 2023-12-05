#ifndef __AUD_HW_H__
#define __AUD_HW_H__

#if defined(CONFIG_SND_SOC_AUD_Q654)
#include "spsoc_util-645.h"
#endif

#define PLLA_FRE 147456000
#define DPLL_FRE 45158400

void AUDHW_pin_mx(void);
void AUDHW_Mixer_Setting(void *auddrvdata);
void AUDHW_SystemInit(void *auddrvdata);
void snd_aud_config(void *auddrvdata);
#endif

