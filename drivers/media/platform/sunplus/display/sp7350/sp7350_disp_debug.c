// SPDX-License-Identifier: GPL-2.0
/*
 * Sunplus SP7350 SoC Display function debug
 *
 * Author: Hammer Hsieh <hammer.hsieh@sunplus.com>
 */
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>

#include "sp7350_display.h"
#include "sp7350_disp_regs.h"

static void sp7350_dmix_luma_adjust_selftest(void);
static void sp7350_dmix_chroma_adjust_selftest(void);
static void sp7350_tcon_rgb_adjust_selftest(u32 channel_sel);


static char *spmon_skipspace(char *p)
{
	int i = 256;

	if (p == NULL)
		return NULL;
	while (i) {
	//while (1) {
		int c = p[0];

		i--;
		if (c == ' ' || c == '\t' || c == '\v')
			p++;
		else
			break;
	}
	return p;
}

#if 1
static char *spmon_readint(char *p, int *x)
{
	int base = 10;
	int cnt, retval;

	if (x == NULL)
		return p;

	//*x = 0;

	if (p == NULL)
		return NULL;

	p = spmon_skipspace(p);

	if (p[0] == '0' && p[1] == 'x') {
		base = 16;
		p += 2;
	}
	if (p[0] == '0' && p[1] == 'o') {
		base = 8;
		p += 2;
	}
	if (p[0] == '0' && p[1] == 'b') {
		base = 2;
		p += 2;
	}

	retval = 0;
	for (cnt = 0; 1; cnt++) {
		int c = *p++;
		int val;

		// translate 0~9, a~z, A~Z
		if (c >= '0' && c <= '9')
			val = c - '0';
		else if (c >= 'a' && c <= 'z')
			val = c - 'a' + 10;
		else if (c >= 'A' && c <= 'Z')
			val = c - 'A' + 10;
		else
			break;
		if (val >= base)
			break;

		retval = retval * base + val;
	}

	if (cnt == 0)
		return p;			// no translation is done??

	*(unsigned int *) x = retval;		// store it
	return p;
}
#endif

/* sunplus_debug_cmd
 *
 *  <Resolution check>
 *    echo "dispmon reso" > /proc/disp_mon
 *
 *  <bist output check>
 *    echo "dispmon bist info" > /proc/disp_mon
 *    echo "dispmon bist img off" > /proc/disp_mon
 *    echo "dispmon bist img bar" > /proc/disp_mon
 *    echo "dispmon bist img bor" > /proc/disp_mon
 *    echo "dispmon bist vscl off" > /proc/disp_mon
 *    echo "dispmon bist vscl bar" > /proc/disp_mon
 *    echo "dispmon bist vscl bor" > /proc/disp_mon
 *    echo "dispmon bist osd0 off" > /proc/disp_mon
 *    echo "dispmon bist osd0 bar" > /proc/disp_mon
 *    echo "dispmon bist osd0 bor" > /proc/disp_mon
 *    echo "dispmon bist dmix region" > /proc/disp_mon
 *    echo "dispmon bist dmix bar" > /proc/disp_mon
 *    echo "dispmon bist dmix barv" > /proc/disp_mon
 *    echo "dispmon bist dmix bor" > /proc/disp_mon
 *    echo "dispmon bist dmix bor black" > /proc/disp_mon
 *    echo "dispmon bist dmix bor red" > /proc/disp_mon
 *    echo "dispmon bist dmix bor green" > /proc/disp_mon
 *    echo "dispmon bist dmix bor blue" > /proc/disp_mon
 *    echo "dispmon bist dmix bor0" > /proc/disp_mon
 *    echo "dispmon bist dmix bor1" > /proc/disp_mon
 *    echo "dispmon bist dmix snow" > /proc/disp_mon
 *    echo "dispmon bist dmix snowhalf" > /proc/disp_mon
 *    echo "dispmon bist dmix snowmax" > /proc/disp_mon
 *    echo "dispmon bist dmix adj on" > /proc/disp_mon
 *    echo "dispmon bist dmix adj off" > /proc/disp_mon
 *    echo "dispmon bist dmix adj luma" > /proc/disp_mon
 *    echo "dispmon bist dmix adj chroma" > /proc/disp_mon
 *    echo "dispmon bist tgen dtgadj ptg" > /proc/disp_mon
 *    echo "dispmon bist tgen dtgadj osd0" > /proc/disp_mon
 *    echo "dispmon bist tgen dtgadj osd1" > /proc/disp_mon
 *    echo "dispmon bist tgen dtgadj osd2" > /proc/disp_mon
 *    echo "dispmon bist tgen dtgadj osd3" > /proc/disp_mon
 *    echo "dispmon bist tcon off" > /proc/disp_mon
 *    echo "dispmon bist tcon gen on" > /proc/disp_mon
 *    echo "dispmon bist tcon gen off" > /proc/disp_mon
 *    echo "dispmon bist tcon hor1 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon hor1 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon hor2 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon hor2 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon hor3 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon hor3 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon ver1 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon ver1 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon ver2 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon ver2 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon ver3 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon ver3 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv1 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv1 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv2 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv2 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv3 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv3 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv4 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv4 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv5 inter" > /proc/disp_mon
 *    echo "dispmon bist tcon hv5 exter" > /proc/disp_mon
 *    echo "dispmon bist tcon gamma on"  > /proc/disp_mon
 *    echo "dispmon bist tcon gamma off" > /proc/disp_mon
 *    echo "dispmon bist tcon gamma upd1" > /proc/disp_mon
 *    echo "dispmon bist tcon gamma [wr|wg|wb|rr|rg|rb|rrgb]" > /proc/disp_mon
 *    echo "dispmon bist tcon rgb selftest [R|G|B|RG|RB|GB|RGB]"  > /proc/disp_mon
 *    echo "dispmon bist tcon rgb on" > /proc/disp_mon
 *    echo "dispmon bist tcon rgb off" > /proc/disp_mon
 *    echo "dispmon bist tcon dither set6bit 0 0 0"  > /proc/disp_mon
 *    echo "dispmon bist tcon dither set8bit"  > /proc/disp_mon
 *    echo "dispmon bist tcon dither set 0 0 0 0"  > /proc/disp_mon
 *    echo "dispmon bist tcon dither on" > /proc/disp_mon
 *    echo "dispmon bist tcon dither off" > /proc/disp_mon
 *    echo "dispmon bist tcon bitswap set [RGB|RBG|GBR|GRB|BRG|BGR] [MSB|LSB]" > /proc/disp_mon
 *    echo "dispmon bist tcon bitswap on" > /proc/disp_mon
 *    echo "dispmon bist tcon bitswap off" > /proc/disp_mon
 *
 *  <dmix layer check/set>
 *    echo "dispmon layer info" > /proc/disp_mon
 *    echo "dispmon layer vpp0 blend" > /proc/disp_mon
 *    echo "dispmon layer vpp0 trans" > /proc/disp_mon
 *    echo "dispmon layer vpp0 opaci" > /proc/disp_mon
 *    echo "dispmon layer osd0 blend" > /proc/disp_mon
 *    echo "dispmon layer osd0 trans" > /proc/disp_mon
 *    echo "dispmon layer osd0 opaci" > /proc/disp_mon
 *
 *  <dump each block info>
 *    echo "dispmon dump vpp" > /proc/disp_mon
 *    echo "dispmon dump osd" > /proc/disp_mon
 *    echo "dispmon dump tgen" > /proc/disp_mon
 *    echo "dispmon dump dmix" > /proc/disp_mon
 *    echo "dispmon dump tcon" > /proc/disp_mon
 *    echo "dispmon dump mipitx" > /proc/disp_mon
 *    echo "dispmon dump all" > /proc/disp_mon
 *
 */
static void sunplus_debug_cmd(char *tmpbuf)
{

	tmpbuf = spmon_skipspace(tmpbuf);

	pr_info("run disp debug cmd\n");

	if (!strncasecmp(tmpbuf, "bist", 4)) {
		tmpbuf = spmon_skipspace(tmpbuf + 4);
		pr_info("bist all info\n");
		if (!strncasecmp(tmpbuf, "info", 4)) {
			sp7350_vpp_bist_info();
			sp7350_osd_bist_info();
			sp7350_dmix_bist_info();
			sp7350_tcon_bist_info();
		} else if (!strncasecmp(tmpbuf, "img", 3)) {
			tmpbuf = spmon_skipspace(tmpbuf + 3);
			pr_info("bist vpp0 imgread cmd\n");
			sp7350_vpp_layer_onoff(1);
			sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_BLENDING);
			if (!strncasecmp(tmpbuf, "off", 3)) {
				pr_info("bist vpp0 imgread off\n");
				sp7350_vpp_bist_set(0, 0, 0);
			} else if (!strncasecmp(tmpbuf, "bar", 3)) {
				pr_info("bist vpp0 imgread bar\n");
				sp7350_vpp_bist_set(0, 1, 0);
			} else if (!strncasecmp(tmpbuf, "bor", 3)) {
				pr_info("bist vpp0 imgread bor\n");
				sp7350_vpp_bist_set(0, 1, 1);
			} else
				pr_info("bist vpp0 imgread cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "vscl", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("bist vpp0 vscl cmd\n");
			sp7350_vpp_layer_onoff(1);
			sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_BLENDING);
			if (!strncasecmp(tmpbuf, "off", 3)) {
				pr_info("bist vpp0 vscl off\n");
				sp7350_vpp_bist_set(1, 0, 0);
			} else if (!strncasecmp(tmpbuf, "bar", 3)) {
				pr_info("bist vpp0 vscl bar\n");
				sp7350_vpp_bist_set(1, 1, 0);
			} else if (!strncasecmp(tmpbuf, "bor", 3)) {
				pr_info("bist vpp0 vscl bor\n");
				sp7350_vpp_bist_set(1, 1, 1);
			} else
				pr_info("bist vpp0 vscl cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "osd0", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("bist osd0 cmd\n");
			sp7350_osd_layer_onoff(SP7350_LAYER_OSD0, 1); //OSD0 on
			sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_BLENDING);
			sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
			if (!strncasecmp(tmpbuf, "off", 3)) {
				pr_info("bist osd0 off\n");
				sp7350_osd_bist_set(0, 0, 0);
			} else if (!strncasecmp(tmpbuf, "bar", 3)) {
				pr_info("bist osd0 bar\n");
				sp7350_osd_bist_set(0, 1, 0);
			} else if (!strncasecmp(tmpbuf, "bor", 3)) {
				pr_info("bist osd0 bor\n");
				sp7350_osd_bist_set(0, 1, 1);
			} else
				pr_info("bist osd0 cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "osd1", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("bist osd1 cmd\n");
			sp7350_osd_layer_onoff(SP7350_LAYER_OSD1, 1); //OSD1 on
			sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_BLENDING);
			sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
			if (!strncasecmp(tmpbuf, "off", 3)) {
				pr_info("bist osd1 off\n");
				sp7350_osd_bist_set(1, 0, 0);
			} else if (!strncasecmp(tmpbuf, "bar", 3)) {
				pr_info("bist osd1 bar\n");
				sp7350_osd_bist_set(1, 1, 0);
			} else if (!strncasecmp(tmpbuf, "bor", 3)) {
				pr_info("bist osd1 bor\n");
				sp7350_osd_bist_set(1, 1, 1);
			} else
				pr_info("bist osd1 cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "osd2", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("bist osd2 cmd\n");
			sp7350_osd_layer_onoff(SP7350_LAYER_OSD2, 1); //OSD2 on
			sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_BLENDING);
			sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
			if (!strncasecmp(tmpbuf, "off", 3)) {
				pr_info("bist osd2 off\n");
				sp7350_osd_bist_set(2, 0, 0);
			} else if (!strncasecmp(tmpbuf, "bar", 3)) {
				pr_info("bist osd2 bar\n");
				sp7350_osd_bist_set(2, 1, 0);
			} else if (!strncasecmp(tmpbuf, "bor", 3)) {
				pr_info("bist osd2 bor\n");
				sp7350_osd_bist_set(2, 1, 1);
			} else
				pr_info("bist osd2 cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "osd3", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("bist osd3 cmd\n");
			sp7350_osd_layer_onoff(SP7350_LAYER_OSD3, 1); //OSD3 on
			sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_BLENDING);
			sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
			if (!strncasecmp(tmpbuf, "off", 3)) {
				pr_info("bist osd3 off\n");
				sp7350_osd_bist_set(3, 0, 0);
			} else if (!strncasecmp(tmpbuf, "bar", 3)) {
				pr_info("bist osd3 bar\n");
				sp7350_osd_bist_set(3, 1, 0);
			} else if (!strncasecmp(tmpbuf, "bor", 3)) {
				pr_info("bist osd3 bor\n");
				sp7350_osd_bist_set(3, 1, 1);
			} else
				pr_info("bist osd3 cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "tgen", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("bist tgen cmd\n");
			if (!strncasecmp(tmpbuf, "dtgadj", 6)) {
				int input, i;
				int adj_value = -1;
				tmpbuf = spmon_skipspace(tmpbuf + 6);
				pr_info("bist tgen dtg adj\n");
				if (!strncasecmp(tmpbuf, "ptg", 3)) {
					pr_info("bist tgen dtgadj ptg\n");
					input = SP7350_TGEN_DTG_ADJ_PTG;
					tmpbuf = spmon_skipspace(tmpbuf + 3);
				} else if (!strncasecmp(tmpbuf, "osd0", 4)) {
					pr_info("bist tgen dtgadj osd0\n");
					input = SP7350_TGEN_DTG_ADJ_OSD0;
					tmpbuf = spmon_skipspace(tmpbuf + 4);
				} else if (!strncasecmp(tmpbuf, "osd1", 4)) {
					pr_info("bist tgen dtgadj osd1\n");
					input = SP7350_TGEN_DTG_ADJ_OSD1;
					tmpbuf = spmon_skipspace(tmpbuf + 4);
				} else if (!strncasecmp(tmpbuf, "osd2", 4)) {
					pr_info("bist tgen dtgadj osd2\n");
					input = SP7350_TGEN_DTG_ADJ_OSD2;
					tmpbuf = spmon_skipspace(tmpbuf + 4);
				} else if (!strncasecmp(tmpbuf, "osd3", 4)) {
					pr_info("bist tgen dtgadj osd3\n");
					input = SP7350_TGEN_DTG_ADJ_OSD3;
					tmpbuf = spmon_skipspace(tmpbuf + 4);
				} else {
					pr_info("bist tgen dtgadj cmd undef\n");
					return;
				}
				tmpbuf = spmon_readint(tmpbuf, &adj_value);
				if (adj_value != -1) {
					pr_info("Set[%d] adj_value %d\n", input, adj_value);
					sp7350_tgen_input_adjust(input, adj_value);
				}
				else {	/* autotest */
					for (i = 0; i <= 0x3f; i++) {
						pr_info("Set[%d] adj_value %d\n",input, i);
						sp7350_tgen_input_adjust(input, i);
						msleep(300);
					}
				}
			}  else
				pr_info("bist tgen cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "dmix", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("bist dmix cmd\n");
			sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_TRANSPARENT);
			sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
			if (!strncasecmp(tmpbuf, "region", 6)) {
				pr_info("bist dmix region\n");
				sp7350_dmix_ptg_set(SP7350_DMIX_BIST_REGION, 0x29f06e);
			} else if (!strncasecmp(tmpbuf, "snowhalf", 8)) {
				pr_info("bist dmix snowhalf\n");
				sp7350_dmix_ptg_set(SP7350_DMIX_BIST_SNOW_HALF, 0x29f06e);
			} else if (!strncasecmp(tmpbuf, "snowmax", 7)) {
				pr_info("bist dmix snowmax\n");
				sp7350_dmix_ptg_set(SP7350_DMIX_BIST_SNOW_MAX, 0x29f06e);
			} else if (!strncasecmp(tmpbuf, "snow", 4)) {
				pr_info("bist dmix snow\n");
				sp7350_dmix_ptg_set(SP7350_DMIX_BIST_SNOW, 0x29f06e);
			} else if (!strncasecmp(tmpbuf, "barv", 4)) {
				pr_info("bist dmix bar rot90\n");
				sp7350_dmix_ptg_set(SP7350_DMIX_BIST_COLORBAR_ROT90, 0x29f06e);
			} else if (!strncasecmp(tmpbuf, "bar", 3)) {
				pr_info("bist dmix bar rot0\n");
				sp7350_dmix_ptg_set(SP7350_DMIX_BIST_COLORBAR_ROT0, 0x29f06e);
			} else if (!strncasecmp(tmpbuf, "bor0", 4)) {
				pr_info("bist dmix bor pix0\n");
				sp7350_dmix_ptg_set(SP7350_DMIX_BIST_BORDER_NONE, 0x29f06e);
			} else if (!strncasecmp(tmpbuf, "bor1", 4)) {
				pr_info("bist dmix bor pix1\n");
				sp7350_dmix_ptg_set(SP7350_DMIX_BIST_BORDER_ONE, 0x29f06e);
			} else if (!strncasecmp(tmpbuf, "bor", 3)) {
				tmpbuf = spmon_skipspace(tmpbuf + 3);
				pr_info("bist dmix bor pix7\n");
				if (!strncasecmp(tmpbuf, "black", 5)) {
					pr_info("bist dmix bor black\n");
					sp7350_dmix_ptg_set(SP7350_DMIX_BIST_BORDER, 0x108080); //BG black
				} else if (!strncasecmp(tmpbuf, "red", 3)) {
					pr_info("bist dmix bor red\n");
					sp7350_dmix_ptg_set(SP7350_DMIX_BIST_BORDER, 0x4040f0); //BG red
				} else if (!strncasecmp(tmpbuf, "green", 5)) {
					pr_info("bist dmix bor green\n");
					sp7350_dmix_ptg_set(SP7350_DMIX_BIST_BORDER, 0x101010); //BG green
				} else if (!strncasecmp(tmpbuf, "blue", 4)) {
					pr_info("bist dmix bor blue\n");
					sp7350_dmix_ptg_set(SP7350_DMIX_BIST_BORDER, 0x29f06e); //BG blue
				} else {
					pr_info("bist dmix bor blue(def)\n");
					sp7350_dmix_ptg_set(SP7350_DMIX_BIST_BORDER, 0x29f06e); //BG blue
				}
			}else if (!strncasecmp(tmpbuf, "adj", 3)) {
				tmpbuf = spmon_skipspace(tmpbuf + 3);
				pr_info("bist dmix color adj\n");
				if (!strncasecmp(tmpbuf, "on", 2)) {
					pr_info("bist dmix adj on\n");
					sp7350_dmix_color_adj_onoff(1);
				} else if (!strncasecmp(tmpbuf, "off", 3)) {
					pr_info("bist dmix adj off\n");
					sp7350_dmix_color_adj_onoff(0);
				} else if (!strncasecmp(tmpbuf, "luma", 4)) {
					pr_info("bist dmix adj luma selftest\n");
					sp7350_dmix_luma_adjust_selftest();
				} else if (!strncasecmp(tmpbuf, "chroma", 6)) {
					pr_info("bist dmix adj chroma selftest\n");
					sp7350_dmix_chroma_adjust_selftest();
				} else
					pr_info("bist dmix adj cmd undef\n");

			}  else
				pr_info("bist dmix cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "tcon", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("bist tcon cmd\n");
			if (!strncasecmp(tmpbuf, "off", 3)) {
				pr_info("bist tcon off\n");
				sp7350_tcon_bist_set(0, 0);
			} else if (!strncasecmp(tmpbuf, "gen", 3)) {
				tmpbuf = spmon_skipspace(tmpbuf + 4);
				if (!strncasecmp(tmpbuf, "on", 2)) {
					pr_info("bist tcon gen pix enable\n");
					sp7350_tcon_gen_pix_set(1);
				} else if (!strncasecmp(tmpbuf, "off", 3)) {
					pr_info("bist tcon gen pix disble\n");
					sp7350_tcon_gen_pix_set(0);
				} else
					pr_info("bist tcon gen cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "hor1", 4)) {
				tmpbuf = spmon_skipspace(tmpbuf + 4);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon H_1_COLORBAR internal\n");
					sp7350_tcon_bist_set(1, 0);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon H_1_COLORBAR external\n");
					sp7350_tcon_bist_set(2, 0);
				} else
					pr_info("bist tcon hor1 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "hor2", 4)) {
				tmpbuf = spmon_skipspace(tmpbuf + 4);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon H_2_RAMP internal\n");
					sp7350_tcon_bist_set(1, 1);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon H_2_RAMP external\n");
					sp7350_tcon_bist_set(2, 1);
				} else
					pr_info("bist tcon hor2 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "hor3", 4)) {
				tmpbuf = spmon_skipspace(tmpbuf + 4);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon H_3_ODDEVEN internal\n");
					sp7350_tcon_bist_set(1, 2);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon H_3_ODDEVEN external\n");
					sp7350_tcon_bist_set(2, 2);
				} else
					pr_info("bist tcon hor3 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "ver1", 4)) {
				tmpbuf = spmon_skipspace(tmpbuf + 4);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon V_1_COLORBAR internal\n");
					sp7350_tcon_bist_set(1, 3);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon V_1_COLORBAR external\n");
					sp7350_tcon_bist_set(2, 3);
				} else
					pr_info("bist tcon ver1 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "ver2", 4)) {
				tmpbuf = spmon_skipspace(tmpbuf + 4);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon V_2_RAMP internal\n");
					sp7350_tcon_bist_set(1, 4);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon V_2_RAMP external\n");
					sp7350_tcon_bist_set(2, 4);
				} else
					pr_info("bist tcon ver2 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "ver3", 4)) {
				tmpbuf = spmon_skipspace(tmpbuf + 4);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon V_3_ODDEVEN internal\n");
					sp7350_tcon_bist_set(1, 5);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon V_3_ODDEVEN external\n");
					sp7350_tcon_bist_set(2, 5);
				} else
					pr_info("bist tcon ver3 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "hv1", 3)) {
				tmpbuf = spmon_skipspace(tmpbuf + 3);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon HV_1_CHECK internal\n");
					sp7350_tcon_bist_set(1, 6);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon HV_1_CHECK external\n");
					sp7350_tcon_bist_set(2, 6);
				} else
					pr_info("bist tcon hv1 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "hv2", 3)) {
				tmpbuf = spmon_skipspace(tmpbuf + 3);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon HV_2_FRAME internal\n");
					sp7350_tcon_bist_set(1, 7);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon HV_2_FRAME external\n");
					sp7350_tcon_bist_set(2, 7);
				} else
					pr_info("bist tcon hv2 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "hv3", 3)) {
				tmpbuf = spmon_skipspace(tmpbuf + 3);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon HV_3_MOIRE_A internal\n");
					sp7350_tcon_bist_set(1, 8);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon HV_3_MOIRE_A external\n");
					sp7350_tcon_bist_set(2, 8);
				} else
					pr_info("bist tcon hv3 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "hv4", 3)) {
				tmpbuf = spmon_skipspace(tmpbuf + 3);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon HV_4_MOIRE_B internal\n");
					sp7350_tcon_bist_set(1, 9);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon HV_4_MOIRE_B external\n");
					sp7350_tcon_bist_set(2, 9);
				} else
					pr_info("bist tcon hv4 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "hv5", 3)) {
				tmpbuf = spmon_skipspace(tmpbuf + 3);
				if (!strncasecmp(tmpbuf, "inter", 5)) {
					pr_info("bist tcon HV_5_CONTRAST internal\n");
					sp7350_tcon_bist_set(1, 10);
				} else if (!strncasecmp(tmpbuf, "exter", 5)) {
					pr_info("bist tcon HV_5_CONTRAST external\n");
					sp7350_tcon_bist_set(2, 10);
				} else
					pr_info("bist tcon hv5 cmd undef\n");

			} else if (!strncasecmp(tmpbuf, "gamma", 5)) {
				u16 *tmptablebuf = NULL;
				//u16 tmptable[512], tmptable2[512];
				u16 *tmptable = NULL, *tmptable2 = NULL;
				int i = 0;
				int tablesize = 512;
				int channel_val = -1;
				int step = 4096 / tablesize;

				tmptablebuf = kmalloc(tablesize*sizeof(u16)*2, GFP_KERNEL);
				if (!tmptablebuf) {
					pr_info("kmalloc fail!!!\n");
					return;
				}
				tmptable = tmptablebuf;
				tmptable2 = tmptablebuf + tablesize;

				tmpbuf = spmon_skipspace(tmpbuf + 5);
				if (!strncasecmp(tmpbuf, "upd1", 4)) {
					pr_info("bist tcon gamma table update1\n");
					for (i = 0; i < tablesize; i++) {
						//tmptable[i] = i%256;
						tmptable[i] = i*step;
					}
					for (i = 0; i < 3; i++) {
						sp7350_tcon_gamma_table_set(i+1, tmptable, tablesize);
						sp7350_tcon_gamma_table_get(i+1, tmptable2, tablesize);
						if (memcmp(tmptable, tmptable2, tablesize*sizeof(u16))) {
							pr_info("Gamma table %s update fail.\n", i==2 ? "B": i ? "G" : "R");
							pr_info("Input gamma table:\n");
							print_hex_dump(KERN_INFO, "DISP DBG", DUMP_PREFIX_OFFSET, 16, 1,
								tmptable, tablesize*sizeof(u16), true);
							pr_info("Output gamma table:\n");
							print_hex_dump(KERN_INFO, "DISP DBG", DUMP_PREFIX_OFFSET, 16, 1,
								tmptable2, tablesize*sizeof(u16), true);
						}
						else {
							pr_info("Gamma table %s update success.\n", i==2 ? "B": i ? "G" : "R");
						}
					}
				} else if (!strncasecmp(tmpbuf, "upd2", 4)){
					pr_info("bist tcon gamma table update2\n");
					for (i = 0; i < tablesize; i++) {
						//tmptable[i] = tablesize%256 - i;
						tmptable[i] = 0xFFF- i*step;
					}
					for (i = 0; i < 3; i++) {
						sp7350_tcon_gamma_table_set(i+1, tmptable, tablesize);
						sp7350_tcon_gamma_table_get(i+1, tmptable2, tablesize);
						if (memcmp(tmptable, tmptable2, tablesize*sizeof(u16))) {
							pr_info("Gamma table %s update fail.\n", i==2 ? "B": i ? "G" : "R");
							pr_info("Input gamma table:\n");
							print_hex_dump(KERN_INFO, "DISP DBG", DUMP_PREFIX_OFFSET, 16, 1,
								tmptable, tablesize*sizeof(u16), true);
							pr_info("Output gamma table:\n");
							print_hex_dump(KERN_INFO, "DISP DBG", DUMP_PREFIX_OFFSET, 16, 1,
								tmptable2, tablesize*sizeof(u16), true);
						}
						else {
							pr_info("Gamma table %s update success.\n", i==2 ? "B": i ? "G" : "R");
						}
					}
				} else if (!strncasecmp(tmpbuf, "wr", 2)){
					pr_info("bist tcon gamma table write r channel\n");
					tmpbuf = spmon_skipspace(tmpbuf + 2);
					tmpbuf = spmon_readint(tmpbuf, &channel_val);
					if (channel_val != -1) {
						pr_info("Write r channel %d\n", channel_val);
						for (i = 0; i < tablesize; i++) {
							tmptable[i] = channel_val;
						}
					}
					else {  /* use default value */
						for (i = 0; i < tablesize; i++) {
							tmptable[i] = 0xFFF- i*step;
						}
					}
					sp7350_tcon_gamma_table_set(SP7350_TCON_GM_UPDDEL_RGB_R, tmptable, tablesize);
				} else if (!strncasecmp(tmpbuf, "wg", 2)){
					pr_info("bist tcon gamma table write g channel\n");
					tmpbuf = spmon_skipspace(tmpbuf + 2);
					tmpbuf = spmon_readint(tmpbuf, &channel_val);
					if (channel_val != -1) {
						pr_info("Write g channel %d\n", channel_val);
						for (i = 0; i < tablesize; i++) {
							tmptable[i] = channel_val;
						}
					}
					else {  /* use default value */
						for (i = 0; i < tablesize; i++) {
							tmptable[i] = 0xFFF- i*step;
						}
					}
					sp7350_tcon_gamma_table_set(SP7350_TCON_GM_UPDDEL_RGB_G, tmptable, tablesize);
				} else if (!strncasecmp(tmpbuf, "wb", 2)){
					pr_info("bist tcon gamma table write b channel\n");
					tmpbuf = spmon_skipspace(tmpbuf + 2);
					tmpbuf = spmon_readint(tmpbuf, &channel_val);
					if (channel_val != -1) {
						pr_info("Write b channel %d\n", channel_val);
						for (i = 0; i < tablesize; i++) {
							tmptable[i] = channel_val;
						}
					}
					else {  /* use default value */
						for (i = 0; i < tablesize; i++) {
							tmptable[i] = 0xFFF- i*step;
						}
					}
					sp7350_tcon_gamma_table_set(SP7350_TCON_GM_UPDDEL_RGB_B, tmptable, tablesize);
				} else if (!strncasecmp(tmpbuf, "rr", 2)){
					pr_info("bist tcon gamma table read r channel\n");
					sp7350_tcon_gamma_table_get(SP7350_TCON_GM_UPDDEL_RGB_R, tmptable2, tablesize);
					pr_info("Output gamma R table:\n");
					print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, 16, 1,
						tmptable2, tablesize*sizeof(u16), true);
				} else if (!strncasecmp(tmpbuf, "rg", 2)){
					pr_info("bist tcon gamma table read g channel\n");
					sp7350_tcon_gamma_table_get(SP7350_TCON_GM_UPDDEL_RGB_G, tmptable2, tablesize);
					pr_info("Output gamma G table:\n");
					print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, 16, 1,
						tmptable2, tablesize*sizeof(u16), true);
				} else if (!strncasecmp(tmpbuf, "rb", 2)){
					pr_info("bist tcon gamma table read b channel\n");
					sp7350_tcon_gamma_table_get(SP7350_TCON_GM_UPDDEL_RGB_B, tmptable2, tablesize);
					pr_info("Output gamma B table:\n");
					print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, 16, 1,
						tmptable2, tablesize*sizeof(u16), true);
				} else if (!strncasecmp(tmpbuf, "rrgb", 4)){
					pr_info("bist tcon gamma table read\n");
					for (i = 0; i < 3; i++) {
						sp7350_tcon_gamma_table_get(i+1, tmptable2, tablesize);
						if (memcmp(tmptable, tmptable2, tablesize*sizeof(u16))) {
							pr_info("Output gamma table:\n");
							print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
								tmptable2, tablesize*sizeof(u16), true);
						}
					}
				} else if (!strncasecmp(tmpbuf, "on", 2)) {
					pr_info("bist tcon gamma table enable.\n");
					sp7350_tcon_gamma_table_enable(1);
				} else if (!strncasecmp(tmpbuf, "off", 3)) {
					pr_info("bist tcon gamma table disable.\n");
					sp7350_tcon_gamma_table_enable(0);
				} else
					pr_info("bist tcon gamma cmd undef\n");

				kfree(tmptablebuf);
				tmptablebuf = NULL;

			} else if (!strncasecmp(tmpbuf, "rgb", 3)) {
				u32 channel_sel = 0;
				tmpbuf = spmon_skipspace(tmpbuf + 3);
				if (!strncasecmp(tmpbuf, "selftest", 8)) {
					tmpbuf = spmon_skipspace(tmpbuf + 8);
					if (strstr(tmpbuf, "R"))
						channel_sel |= SP7350_TCON_RGB_ADJ_CHANNEL_R_EN;
					if (strstr(tmpbuf, "G"))
						channel_sel |= SP7350_TCON_RGB_ADJ_CHANNEL_G_EN;
					if (strstr(tmpbuf, "B"))
						channel_sel |= SP7350_TCON_RGB_ADJ_CHANNEL_B_EN;

					if (!channel_sel) {
						pr_info("Invalid channel_sel!!!\n");
					}
					else {
						pr_info("RGB Adjust selftest[0x%X].\n", channel_sel);
						sp7350_tcon_rgb_adjust_selftest(channel_sel);
					}
				} else if (!strncasecmp(tmpbuf, "on", 2)) {
					tmpbuf = spmon_skipspace(tmpbuf + 2);
					if (strstr(tmpbuf, "R"))
						channel_sel |= SP7350_TCON_RGB_ADJ_CHANNEL_R_EN;
					if (strstr(tmpbuf, "G"))
						channel_sel |= SP7350_TCON_RGB_ADJ_CHANNEL_G_EN;
					if (strstr(tmpbuf, "B"))
						channel_sel |= SP7350_TCON_RGB_ADJ_CHANNEL_B_EN;

					if (!channel_sel) {
						pr_info("Invalid channel_sel!!!\n");
					}
					else {
						pr_info("RGB Adjust Enable[0x%X].\n", channel_sel);
						sp7350_tcon_rgb_adjust_enable(channel_sel);
					}

				} else if (!strncasecmp(tmpbuf, "off", 3)) {
					sp7350_tcon_rgb_adjust_enable(0);
				} else
					pr_info("bist tcon rgb cmd undef\n");
			} else if (!strncasecmp(tmpbuf, "dither", 6)) {
				tmpbuf = spmon_skipspace(tmpbuf + 6);
				if (!strncasecmp(tmpbuf, "set6bit", 7)) {
					u32 mode = 0, table_v_shift_en = 0, table_h_shift_en = 0;
					tmpbuf = spmon_skipspace(tmpbuf + 7);
					if (!strncasecmp(tmpbuf, "0", 1))
						mode = SP7350_TCON_DITHER_6BIT_MODE_MATCHED;
					else
						mode = SP7350_TCON_DITHER_6BIT_MODE_ROBUST;

					tmpbuf = spmon_skipspace(tmpbuf + 1);
					if (!strncasecmp(tmpbuf, "0", 1))
						table_v_shift_en = 0;
					else
						table_v_shift_en = 1;

					tmpbuf = spmon_skipspace(tmpbuf + 1);
					if (!strncasecmp(tmpbuf, "0", 1))
						table_h_shift_en = 0;
					else
						table_h_shift_en = 1;

					pr_info("DITHER set With MODE[%s] VSHIFT[%d] HSHIFT[%d].\n", mode ? "ROBUST" :"MATCHED", table_v_shift_en, table_h_shift_en);
					sp7350_tcon_enhanced_dither_6bit_set(mode, table_v_shift_en, table_h_shift_en);
				} else if (!strncasecmp(tmpbuf, "set8bit", 7)) {
					sp7350_tcon_enhanced_dither_8bit_set();
				}else if (!strncasecmp(tmpbuf, "set", 3)) {
					u32 rgbc_sel = 0, method = 0, temporal_mode_en = 0, dot_mode = 0;
					tmpbuf = spmon_skipspace(tmpbuf + 3);
					if (!strncasecmp(tmpbuf, "0", 1))
						rgbc_sel = SP7350_TCON_DITHER_RGBC_SEL_R;
					else
						rgbc_sel = SP7350_TCON_DITHER_RGBC_SEL_RGB;

					tmpbuf = spmon_skipspace(tmpbuf + 1);
					if (!strncasecmp(tmpbuf, "0", 1))
						method = SP7350_TCON_DITHER_INIT_MODE_METHOD2;
					else
						method = SP7350_TCON_DITHER_INIT_MODE_METHOD1;

					tmpbuf = spmon_skipspace(tmpbuf + 1);
					if (!strncasecmp(tmpbuf, "0", 1))
						temporal_mode_en = 0;
					else
						temporal_mode_en = 1;

					tmpbuf = spmon_skipspace(tmpbuf + 1);
					if (!strncasecmp(tmpbuf, "0", 1))
						dot_mode = SP7350_TCON_DITHER_PANEL_DOT_MODE_1DOT;
					else if (!strncasecmp(tmpbuf, "1", 1))
						dot_mode = SP7350_TCON_DITHER_PANEL_DOT_MODE_H2DOT;
					else if (!strncasecmp(tmpbuf, "2", 1))
						dot_mode = SP7350_TCON_DITHER_PANEL_DOT_MODE_V2DOT;
					else if (!strncasecmp(tmpbuf, "3", 1))
						dot_mode = SP7350_TCON_DITHER_PANEL_DOT_MODE_2DOT;
					else {
						pr_info("Invalid DITHER panel dot mode!!!\n");
						return;
					}
					pr_info("DITHER set With RGBC[%s] METHOD[%d] TEMP[%d] DOT[%s].\n",
						rgbc_sel? "RGB":"R", method, temporal_mode_en,
						dot_mode == SP7350_TCON_DITHER_PANEL_DOT_MODE_2DOT ? "2DOT":
						(dot_mode == SP7350_TCON_DITHER_PANEL_DOT_MODE_V2DOT ?"V2DOT":
						(dot_mode == SP7350_TCON_DITHER_PANEL_DOT_MODE_H2DOT?"H2DOT":"1DOT")));
					sp7350_tcon_enhanced_dither_set(rgbc_sel, method, temporal_mode_en, dot_mode);
				} else if (!strncasecmp(tmpbuf, "on", 2)) {
					sp7350_tcon_enhanced_dither_enable(1);
				} else if (!strncasecmp(tmpbuf, "off", 3)) {
					sp7350_tcon_enhanced_dither_enable(0);
				} else
					pr_info("bist tcon dither cmd undef\n");
			} else if (!strncasecmp(tmpbuf, "bitswap", 7)) {
				u32 channel_mode = 0, bit_mode = 0;
				char *tmp = tmpbuf;
				tmpbuf = spmon_skipspace(tmpbuf + 7);
				if (!strncasecmp(tmpbuf, "set", 3)) {
					tmpbuf = spmon_skipspace(tmpbuf + 3);
					if (!strncasecmp(tmpbuf, "RGB", 3))
						channel_mode = SP7350_TCON_BIT_SW_CHNL_RGB;
					else if (!strncasecmp(tmpbuf, "RBG", 3))
						channel_mode = SP7350_TCON_BIT_SW_CHNL_RBG;
					else if (!strncasecmp(tmpbuf, "GBR", 3))
						channel_mode = SP7350_TCON_BIT_SW_CHNL_GBR;
					else if (!strncasecmp(tmpbuf, "GRB", 3))
						channel_mode = SP7350_TCON_BIT_SW_CHNL_GRB;
					else if (!strncasecmp(tmpbuf, "BGR", 3))
						channel_mode = SP7350_TCON_BIT_SW_CHNL_BGR;
					else if (!strncasecmp(tmpbuf, "BRG", 3))
						channel_mode = SP7350_TCON_BIT_SW_CHNL_BRG;
					else {
						pr_info("Invalid BITSWAP Channel mode!!!\n");
						return;
					}

					tmp = tmpbuf;
					tmpbuf = spmon_skipspace(tmpbuf + 3);
					if (!strncasecmp(tmpbuf, "MSB", 3))
						bit_mode = SP7350_TCON_BIT_SW_BIT_MSB;
					else if (!strncasecmp(tmpbuf, "LSB", 3))
						bit_mode = SP7350_TCON_BIT_SW_BIT_LSB;
					else {
						pr_info("Invalid BITSWAP bit mode!!!\n");
						return;
					}

					pr_info("bitswap set With %.*s %.*s.\n", 3, tmp, 3, tmpbuf);
					sp7350_tcon_bitswap_set(bit_mode, channel_mode);
				} else if (!strncasecmp(tmpbuf, "on", 2)) {
					sp7350_tcon_bitswap_enable(1);
				} else if (!strncasecmp(tmpbuf, "off", 3)) {
					sp7350_tcon_bitswap_enable(0);
				} else
					pr_info("bist tcon bitswap cmd undef\n");
			} else
				pr_info("bist tcon cmd undef\n");

		} else
			pr_info("bist cmd undef\n");
	} else if (!strncasecmp(tmpbuf, "layer", 5)) {
		tmpbuf = spmon_skipspace(tmpbuf + 5);

		if (!strncasecmp(tmpbuf, "info", 4)) {
			pr_info("layer info cmd\n");
			sp7350_dmix_all_layer_info();
		} else if (!strncasecmp(tmpbuf, "vpp0", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("layer vpp0 cmd\n");
			if (!strncasecmp(tmpbuf, "blend", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_BLENDING);
			} else if (!strncasecmp(tmpbuf, "trans", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
			} else if (!strncasecmp(tmpbuf, "opaci", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_OPACITY);
			} else
				pr_info("layer vpp0 cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "osd0", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("layer osd0 cmd\n");
			if (!strncasecmp(tmpbuf, "blend", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_BLENDING);
			} else if (!strncasecmp(tmpbuf, "trans", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
			} else if (!strncasecmp(tmpbuf, "opaci", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_OPACITY);
			} else
				pr_info("layer osd0 cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "osd1", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("layer osd1 cmd\n");
			if (!strncasecmp(tmpbuf, "blend", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_BLENDING);
			} else if (!strncasecmp(tmpbuf, "trans", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
			} else if (!strncasecmp(tmpbuf, "opaci", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_OPACITY);
			} else
				pr_info("layer osd1 cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "osd2", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("layer osd2 cmd\n");
			if (!strncasecmp(tmpbuf, "blend", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_BLENDING);
			} else if (!strncasecmp(tmpbuf, "trans", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
			} else if (!strncasecmp(tmpbuf, "opaci", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_OPACITY);
			} else
				pr_info("layer osd2 cmd undef\n");

		} else if (!strncasecmp(tmpbuf, "osd3", 4)) {
			tmpbuf = spmon_skipspace(tmpbuf + 4);
			pr_info("layer osd3 cmd\n");
			if (!strncasecmp(tmpbuf, "blend", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_BLENDING);
			} else if (!strncasecmp(tmpbuf, "trans", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_TRANSPARENT);
			} else if (!strncasecmp(tmpbuf, "opaci", 5)) {
				sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_OSD3, SP7350_DMIX_OPACITY);
			} else
				pr_info("layer osd3 cmd undef\n");

		} else
			pr_info("layer cmd undef\n");

	} else if (!strncasecmp(tmpbuf, "dump", 4)) {
		tmpbuf = spmon_skipspace(tmpbuf + 4);

		if (!strncasecmp(tmpbuf, "vpp", 3)) {
			pr_info("dump vpp path info\n");
			sp7350_vpp_decrypt_info();
		} else if (!strncasecmp(tmpbuf, "osdh", 4)) {
			pr_info("dump osd header info\n");
			sp7350_osd_header_show();
		} else if (!strncasecmp(tmpbuf, "osd", 3)) {
			pr_info("dump osd path info\n");
			sp7350_osd_decrypt_info();
		} else if (!strncasecmp(tmpbuf, "tgen", 4)) {
			pr_info("dump tgen info\n");
			sp7350_tgen_decrypt_info();
		} else if (!strncasecmp(tmpbuf, "dmix", 4)) {
			pr_info("dump dmix info\n");
			sp7350_dmix_decrypt_info();
		} else if (!strncasecmp(tmpbuf, "tcon", 4)) {
			pr_info("dump tcon info\n");
			sp7350_tcon_decrypt_info();
		} else if (!strncasecmp(tmpbuf, "mipitx", 6)) {
			pr_info("dump mipitx info\n");
			sp7350_mipitx_decrypt_info();
			sp7350_mipitx_pllclk_get();
			sp7350_mipitx_txpll_get();
		} else if (!strncasecmp(tmpbuf, "all", 3)) {
			pr_info("dump all info\n");
			sp7350_vpp_decrypt_info();
			sp7350_osd_decrypt_info();
			sp7350_tgen_decrypt_info();
			sp7350_dmix_decrypt_info();
			sp7350_tcon_decrypt_info();
			sp7350_mipitx_decrypt_info();
			sp7350_mipitx_pllclk_get();
			sp7350_mipitx_txpll_get();
		} else if (!strncasecmp(tmpbuf, "reso", 4)) {
			pr_info("dump resolution\n");
			sp7350_vpp_imgread_resolution_chk();
			sp7350_vpp_vscl_resolution_chk();
			sp7350_osd_resolution_chk();
			sp7350_tgen_resolution_chk();
			sp7350_tcon_resolution_chk();
			sp7350_mipitx_resolution_chk();
			sp7350_mipitx_pllclk_get();
			sp7350_mipitx_txpll_get();
		} else if (!strncasecmp(tmpbuf, "timing", 6)) {
			pr_info("dump timing\n");
			sp7350_tgen_timing_get();
			sp7350_tcon_timing_get();
			sp7350_mipitx_timing_get();
			sp7350_mipitx_pllclk_get();
			sp7350_mipitx_txpll_get();
		}

	} else
		pr_err("unknown command:%s\n", tmpbuf);

	(void)tmpbuf;
}

int sunplus_proc_open_show(struct seq_file *m, void *v)
{
	//pr_info("%s\n", __func__);
	//pr_info("SP7350 DISPLAY DEBUG TEST\n");

	return 0;
}

int sunplus_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunplus_proc_open_show, NULL);
}

static ssize_t sunplus_proc_write(struct file *filp, const char __user *buffer,
				size_t len, loff_t *f_pos)
{
	char pbuf[256];
	char *tmpbuf = pbuf;

	if (len > sizeof(pbuf)) {
		pr_err("intput error len:%d!\n", (int)len);
		return -ENOSPC;
	}

	if (copy_from_user(tmpbuf, buffer, len)) {
		pr_err("intput error!\n");
		return -EFAULT;
	}

	if (len == 0)
		tmpbuf[len] = '\0';
	else
		tmpbuf[len - 1] = '\0';

	if (!strncasecmp(tmpbuf, "dispmon", 7))
		sunplus_debug_cmd(tmpbuf + 7);

	return len;
}

//static const struct proc_ops sp_disp_proc_ops = {
const struct proc_ops sp_disp_proc_ops = {
	.proc_open = sunplus_proc_open,
	.proc_write = sunplus_proc_write,
	.proc_read = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release,
};

static int set_debug_cmd(const char *val, const struct kernel_param *kp)
{
	sunplus_debug_cmd((char *)val);

	return 0;
}

static struct kernel_param_ops debug_param_ops = {
	.set = set_debug_cmd,
};
module_param_cb(debug, &debug_param_ops, NULL, 0644);
MODULE_PARM_DESC(debug, "SP7350 display debug test");

/*
 * sp7350 debug fs reference
 *
 * /sys/kernel/debug/f8005c80.display/
 *   -- sp7350_dump_regs
 *   -- /regs/vpp
 *
 */
static int sp7350_debug_dump_regs(struct sp_disp_device *disp_dev,
				  struct seq_file *m)
{
	pr_info("%s test\n", __func__);

	return 0;
}

static int sp7350_dump_regs_show(struct seq_file *m, void *p)
{
	struct sp_disp_device *disp_dev = m->private;

	return sp7350_debug_dump_regs(disp_dev, m);
}
DEFINE_SHOW_ATTRIBUTE(sp7350_dump_regs);

static int sp7350_debug_dump_vpp_regs(struct sp_disp_device *disp_dev,
				  struct seq_file *m)
{
	pr_info("%s test\n", __func__);

	return 0;
}

static int sp7350_dump_vpp_regs_show(struct seq_file *m, void *p)
{
	struct sp_disp_device *disp_dev = m->private;

	return sp7350_debug_dump_vpp_regs(disp_dev, m);
}
DEFINE_SHOW_ATTRIBUTE(sp7350_dump_vpp_regs);

void sp7350_debug_init(struct sp_disp_device *disp_dev)
{
	struct sp7350_debug *debug = &disp_dev->debug;
	struct dentry *regs_dir;

	debug->debugfs_dir = debugfs_create_dir(dev_name(disp_dev->pdev), NULL);

	pr_info("%s dev_name: [%s]\n", __func__, dev_name(disp_dev->pdev));

	debugfs_create_file("sp7350_dump_regs", 0444, debug->debugfs_dir, disp_dev,
			    &sp7350_dump_regs_fops);

	regs_dir = debugfs_create_dir("regs", debug->debugfs_dir);

	debugfs_create_file("vpp", 0444, regs_dir, disp_dev,
			    &sp7350_dump_vpp_regs_fops);

}

void sp7350_debug_cleanup(struct sp_disp_device *disp_dev)
{
	debugfs_remove_recursive(disp_dev->debug.debugfs_dir);
}

#define DEBUG_DMIX_ADJUST_LUMA_CP_EN  1
static void sp7350_dmix_luma_adjust_selftest(void)
{
	u8 tmpcptable[SP7350_TCON_RGB_ADJ_CP_SIZE];
	u8 tmpcpsrctable[SP7350_TCON_RGB_ADJ_CP_SIZE];
	u8 tmpcpsdttable[SP7350_TCON_RGB_ADJ_CP_SIZE];
	u16 tmpslopetable[SP7350_TCON_RGB_ADJ_SLOPE_SIZE];
	u16 tmpslopetable2[SP7350_TCON_RGB_ADJ_SLOPE_SIZE];
	int i,j;

	pr_info(" DMIX Luma Adjustment SELFTEST.\n");

	for (i = 0; i < 0xff; i+=3) {
		#if DEBUG_DMIX_ADJUST_LUMA_CP_EN
		for (j = 0; j < SP7350_TCON_RGB_ADJ_CP_SIZE; j++) {
			tmpcptable[j] = (i + j)%0xff;
		}
		#endif
		for (j = 0; j < SP7350_TCON_RGB_ADJ_SLOPE_SIZE; j++) {
			tmpslopetable[j] = (i * 8 + j)%0x7ff;
		}
		#if DEBUG_DMIX_ADJUST_LUMA_CP_EN
		pr_info(" DMIX Luma Adjustment SELFTEST cpsrc[%02x %02x %02x], cpsdt[%02x %02x %02x], slope[%04x %04x %04x %04x].\n",
			tmpcptable[0], tmpcptable[1], tmpcptable[2],
			tmpcptable[0], tmpcptable[1], tmpcptable[2],
			tmpslopetable[0], tmpslopetable[1], tmpslopetable[2], tmpslopetable[3]);
		sp7350_dmix_color_adj_luma_cp_set( tmpcptable, tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE);
		#else
		pr_info(" DMIX Luma Adjustment SELFTEST slope[%04x %04x %04x %04x].\n",
			tmpslopetable[0], tmpslopetable[1], tmpslopetable[2], tmpslopetable[3]);
		#endif
		sp7350_dmix_color_adj_luma_slope_set( tmpslopetable, SP7350_TCON_RGB_ADJ_SLOPE_SIZE);
		sp7350_dmix_color_adj_onoff(1);
		msleep(300);
		#if DEBUG_DMIX_ADJUST_LUMA_CP_EN
		sp7350_dmix_color_adj_luma_cp_get( tmpcpsrctable, tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE);
		if (memcmp(tmpcptable, tmpcpsrctable, SP7350_TCON_RGB_ADJ_CP_SIZE)) {
			pr_info(" DMIX Luma Adjust control point source update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcpsrctable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			break;
		}
		if (memcmp(tmpcptable, tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE)) {
			pr_info(" DMIX Luma Adjust control point destination update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			break;
		}
		#endif
		sp7350_dmix_color_adj_luma_slope_get(tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE);
		if (memcmp(tmpslopetable, tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE)) {
			pr_info(" DMIX Luma Adjust slope update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpslopetable, SP7350_TCON_RGB_ADJ_SLOPE_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE, false);
			break;
		}
	}

	/* disbable */
	//sp7350_dmix_color_adj_onoff(0);
}

static void sp7350_dmix_chroma_adjust_selftest(void)
{
	int i,j;
	u16 satcos, satsin;
	u16 tmpsatcos, tmpsatsin;


	for (i = 0; i < 3; i++) {
		satcos = 0;
		satsin = 0;
		sp7350_dmix_color_adj_onoff(1);
		for (j = 0; j < 0x3ff; j+=8) {
			if (i == 0) {
				satcos = j;
				satsin = j;
			}
			else if (i == 1) {
				satcos = j;
			}
			else if (i == 2) {
				satsin = j;
			}

			pr_info(" DMIX Chroma Adjustment SELFTEST satcos[%02x], satsin[%02x ].\n", satcos, satsin);
			sp7350_dmix_color_adj_crma_set( satcos, satcos);
			msleep(300);
			sp7350_dmix_color_adj_crma_get( &tmpsatcos, &tmpsatsin);
			if (satcos != tmpsatcos || satcos != tmpsatsin) {
				pr_info(" DMIX Chroma Adjustment get satcos[%02x], satsin[%02x ].\n", tmpsatcos, tmpsatsin);
				break;
			}
		}
		msleep(3000);
		/* disbable */
		sp7350_dmix_color_adj_onoff(0);
		msleep(3000);
	}


}

static void sp7350_tcon_rgb_adjust_selftest(u32 channel_sel)
{
	u8 tmpcptable[SP7350_TCON_RGB_ADJ_CP_SIZE];
	u8 tmpcpsrctable[SP7350_TCON_RGB_ADJ_CP_SIZE];
	u8 tmpcpsdttable[SP7350_TCON_RGB_ADJ_CP_SIZE];
	u16 tmpslopetable[SP7350_TCON_RGB_ADJ_SLOPE_SIZE];
	u16 tmpslopetable2[SP7350_TCON_RGB_ADJ_SLOPE_SIZE];
	int i,j;

	pr_info(" RGB Adjustment SELFTEST with %s%s%s Channel.\n",
		(channel_sel & SP7350_TCON_RGB_ADJ_CHANNEL_R_EN) ? "R" : "",
		(channel_sel & SP7350_TCON_RGB_ADJ_CHANNEL_G_EN) ? "G" : "",
		(channel_sel & SP7350_TCON_RGB_ADJ_CHANNEL_B_EN) ? "B" : "");

	for (i = 0; i < 0xff; i+=3) {
		for (j = 0; j < SP7350_TCON_RGB_ADJ_CP_SIZE; j++) {
			tmpcptable[j] = (i + j)%0xff;
		}
		for (j = 0; j < SP7350_TCON_RGB_ADJ_SLOPE_SIZE; j++) {
			tmpslopetable[j] = (i * 8 + j)%0x7ff;
		}
		pr_info(" RGB Adjustment SELFTEST cpsrc[%02x %02x %02x], cpsdt[%02x %02x %02x], slope[%04x %04x %04x %04x].\n",
			tmpcptable[0], tmpcptable[1], tmpcptable[2],
			tmpcptable[0], tmpcptable[1], tmpcptable[2],
			tmpslopetable[0], tmpslopetable[1], tmpslopetable[2], tmpslopetable[3]);
		sp7350_tcon_rgb_adjust_cp_set(SP7350_TCON_RGB_ADJ_CHANNEL_R_EN, tmpcptable, tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE);
		sp7350_tcon_rgb_adjust_cp_set(SP7350_TCON_RGB_ADJ_CHANNEL_G_EN, tmpcptable, tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE);
		sp7350_tcon_rgb_adjust_cp_set(SP7350_TCON_RGB_ADJ_CHANNEL_B_EN, tmpcptable, tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE);
		sp7350_tcon_rgb_adjust_slope_set(SP7350_TCON_RGB_ADJ_CHANNEL_R_EN, tmpslopetable, SP7350_TCON_RGB_ADJ_SLOPE_SIZE);
		sp7350_tcon_rgb_adjust_slope_set(SP7350_TCON_RGB_ADJ_CHANNEL_G_EN, tmpslopetable, SP7350_TCON_RGB_ADJ_SLOPE_SIZE);
		sp7350_tcon_rgb_adjust_slope_set(SP7350_TCON_RGB_ADJ_CHANNEL_B_EN, tmpslopetable, SP7350_TCON_RGB_ADJ_SLOPE_SIZE);
		//sp7350_tcon_rgb_adjust_enable(SP7350_TCON_RGB_ADJ_CHANNEL_R_EN|SP7350_TCON_RGB_ADJ_CHANNEL_G_EN|SP7350_TCON_RGB_ADJ_CHANNEL_B_EN);
		sp7350_tcon_rgb_adjust_enable(channel_sel);
		//udelay(1000);
		msleep(300);
		sp7350_tcon_rgb_adjust_cp_get(SP7350_TCON_RGB_ADJ_CHANNEL_R_EN, tmpcpsrctable, tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE);
		if (memcmp(tmpcptable, tmpcpsrctable, SP7350_TCON_RGB_ADJ_CP_SIZE)) {
			pr_info(" RGB Adjust control point source R update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcpsrctable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			break;
		}
		if (memcmp(tmpcptable, tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE)) {
			pr_info(" RGB Adjust control point destination R update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			break;
		}
		sp7350_tcon_rgb_adjust_cp_get(SP7350_TCON_RGB_ADJ_CHANNEL_G_EN, tmpcpsrctable, tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE);
		if (memcmp(tmpcptable, tmpcpsrctable, SP7350_TCON_RGB_ADJ_CP_SIZE)) {
			pr_info(" RGB Adjust control point source G update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcpsrctable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			break;
		}
		if (memcmp(tmpcptable, tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE)) {
			pr_info(" RGB Adjust control point destination G update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			break;
		}
		sp7350_tcon_rgb_adjust_cp_get(SP7350_TCON_RGB_ADJ_CHANNEL_B_EN, tmpcpsrctable, tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE);
		if (memcmp(tmpcptable, tmpcpsrctable, SP7350_TCON_RGB_ADJ_CP_SIZE)) {
			pr_info(" RGB Adjust control point source B update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcpsrctable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			break;
		}
		if (memcmp(tmpcptable, tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE)) {
			pr_info(" RGB Adjust control point destination B update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcptable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpcpsdttable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			break;
		}
		sp7350_tcon_rgb_adjust_slope_get(SP7350_TCON_RGB_ADJ_CHANNEL_R_EN, tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE);
		if (memcmp(tmpslopetable, tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE)) {
			pr_info(" RGB Adjust slope R update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpslopetable, SP7350_TCON_RGB_ADJ_SLOPE_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE, false);
			break;
		}
		sp7350_tcon_rgb_adjust_slope_get(SP7350_TCON_RGB_ADJ_CHANNEL_G_EN, tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE);
		if (memcmp(tmpslopetable, tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE)) {
			pr_info(" RGB Adjust slope G update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpslopetable, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpslopetable2, SP7350_TCON_RGB_ADJ_CP_SIZE, false);
			break;
		}
		sp7350_tcon_rgb_adjust_slope_get(SP7350_TCON_RGB_ADJ_CHANNEL_B_EN, tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE);
		if (memcmp(tmpslopetable, tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE)) {
			pr_info(" RGB Adjust slope B update fail.\n");
			pr_info("Input:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpslopetable, SP7350_TCON_RGB_ADJ_SLOPE_SIZE, false);
			pr_info("Output:\n");
			print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
				tmpslopetable2, SP7350_TCON_RGB_ADJ_SLOPE_SIZE, false);
			break;
		}
	}

	/* disbable */
	sp7350_tcon_rgb_adjust_enable(0);
}

