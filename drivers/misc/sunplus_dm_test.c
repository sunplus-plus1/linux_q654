// SPDX-License-Identifier: GPL-2.0
/*
 * Dummy Master device driver for SUNPLUS SP7350 SoC
 *
 *  <dump help info>
 *    echo > /sys/module/sunplus_dm_test/parameters/test
 *  <dump dm reg info>
 *    echo 10 0 0 > /sys/module/sunplus_dm_test/parameters/test
 *  <dump set parameter info>
 *    echo 11 0 0 > /sys/module/sunplus_dm_test/parameters/test
 *
 *  <set address>
 *    echo 20 aa bb > /sys/module/sunplus_dm_test/parameters/test
 *  <set length and op_mode>
 *    echo 21 cc dd > /sys/module/sunplus_dm_test/parameters/test
 *
 *  <run dummymaster0>
 *    echo 1 0 0 > /sys/module/sunplus_dm_test/parameters/test
 *  <run dummymaster1>
 *    echo 0 1 0 > /sys/module/sunplus_dm_test/parameters/test
 *  <run dummymaster2>
 *    echo 0 0 1 > /sys/module/sunplus_dm_test/parameters/test
 *
 *  <test process>
 *    1. set DummyMaster Enable
 *    2. calculate request period depends on sysclk and dramclk
 *    3. set WBE=1 and Write Length = 16 and op_mode = 1 / 2 / 3 / 15
 *           op_mode = 1 (only write)
 *           op_mode = 2 (only read)
 *           op_mode = 3 (1 write 1 read)
 *           op_mode = 15 (1 write 3 read)
 *    4. set start/end address = 0x0010-0000/0x0020-0000
 *    5. set urgent en/dis and limit value
 *    6. set request period and read total_cmd_cnt and total_cycle_cnt
 *      6-1. set request period by calculate (10% ~ 100%)
 *      6-2. set DummyMaster SOFT_RESET and Enable
 *      6-3. wait 0.001 second
 *      6-4. read read total_cmd_cnt and total_cycle_cnt
 *    7. set DummyMaster Disable
 *    8. show setting parameter
 *    9. show measure result
 *
 * Author: Hammer Hsieh <hammerh0314@gmail.com>
 */
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
//#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
//#include <linux/reset.h>
//#include <linux/smp.h>
#include <linux/types.h>

#define DM_OP_MODE		0x000
#define DM_ADDR_BASE		0x004
#define DM_ADDR_OFS		0x008
#define DM_CONTROL		0x00c
#define DM_URGENT		0x010
#define DM_REQ_PERIOD		0x014
#define DM_UNFINISH_CNT		0x018
#define DM_ERROR		0x01c
#define DM_GOLDEN_VALUE		0x020
#define DM_TOTAL_CMD_CNT	0x024
#define DM_TOTAL_CYCLE_CNT	0x028

#define Q654_SYSTEM_CLOCK 600000 /* 600 MHz*/
#define Q654_DRAM_CLOCK 1600000 /* 1600 MHz*/
#define Q654_MAX_BANDWIDTH_DM0 (Q654_DRAM_CLOCK * 8)
#define Q654_MAX_BANDWIDTH_DM1 (Q654_DRAM_CLOCK * 8 / 2)

static const char * const DummyMasterDefine[] = {
	"op_mode", 		/* G14.00 */
	"addr_base", 		/* G14.01 */
	"addr_offset", 		/* G14.02 */
	"control", 		/* G14.03 */
	"urgent", 		/* G14.04 */
	"req_period", 		/* G14.05 */
	"unfin_cnt", 		/* G14.06 */
	"error", 		/* G14.07 */
	"golden_value", 	/* G14.08 */
	"total_cmd_cnt", 	/* G14.09 */
	"total_cycle_cnt"  	/* G14.10 */
};

static const char * const show_op_mode[] = {
/* For OP_MODE setting */
	"4'b0000 = IDEL",
	"4'b0001 = ONLY_WRITE",
	"4'b0010 = ONLY_READ",
	"4'b0011 = READ_after_WRITE(default)",
	"4'b0100 = reserved",
	"4'b0101 = WRITE and wdata equal golden value",
	"4'b0110 = READ and check rdata equal golden value",
	"4'b0111 = READ_after_WRITE and selftest with golden value",
	"4'b1000 = reserved",
	"4'b1001 = WRITE and write inc value",
	"4'b1010 = READ and check rdata equal inc value",
	"4'b1011 = READ_after_WRITE selftest inc value",
	"4'b1100 = reserved",
	"4'b1101 = reserved",
	"4'b1110 = reserved",
	"4'b1111 = 3_READ_1_WRITE"
};

u32 request_period_bw[11] = {
	0x00000078, //Target BW 10%
	0x0000003c, //Target BW 20%
	0x00000028, //Target BW 30%
	0x0000001e, //Target BW 40%
	0x00000018, //Target BW 50%
	0x00000014, //Target BW 60%
	0x000e0011, //Target BW 70%
	0x0000000f, //Target BW 80%
	0x0021000d, //Target BW 90%
	0x0000000c, //Target BW 100%
	0x00010001, //Target BW max  (1.1 cycle)
};

u32 unfinished_cmd_cnt[11] = {
	0x03ff03ff, //Target BW 10%
	0x03ff03ff, //Target BW 20%
	0x03ff03ff, //Target BW 30%
	0x03ff03ff, //Target BW 40%
	0x03ff03ff, //Target BW 50%
	0x03ff03ff, //Target BW 60%
	0x03ff03ff, //Target BW 70%
	0x03ff03ff, //Target BW 80%
	0x03ff03ff, //Target BW 90%
	0x03ff03ff, //Target BW 100%
	0x03ff03ff, //Target BW max  (1.1 cycle)
};

u32 target_bandwidth[11] = {
	(Q654_DRAM_CLOCK * 8/1000) * 1 / 10, //Target BW 10%
	(Q654_DRAM_CLOCK * 8/1000) * 2 / 10, //Target BW 20%
	(Q654_DRAM_CLOCK * 8/1000) * 3 / 10, //Target BW 30%
	(Q654_DRAM_CLOCK * 8/1000) * 4 / 10, //Target BW 40%
	(Q654_DRAM_CLOCK * 8/1000) * 5 / 10, //Target BW 50%
	(Q654_DRAM_CLOCK * 8/1000) * 6 / 10, //Target BW 60%
	(Q654_DRAM_CLOCK * 8/1000) * 7 / 10, //Target BW 70%
	(Q654_DRAM_CLOCK * 8/1000) * 8 / 10, //Target BW 80%
	(Q654_DRAM_CLOCK * 8/1000) * 9 / 10, //Target BW 90%
	(Q654_DRAM_CLOCK * 8/1000), //Target BW 100%
	(Q654_DRAM_CLOCK * 8/1000), //Target BW max  (1.1 cycle)
};

struct sunplus_moon0 {
	int num1;//group;
	int num2;//byte_num;
	int num3;//bit_from;
	int num4;//bit_to;
	int num5;//bit_value, with readl function
	int type;
	char *name;
};

int buf_tmp[1000];
struct sunplus_moon0 sp_moon0[] = {
	/*                group   byte_num   bit_from   bit_to  bit_value  type   name           */
	/* G00.01[15] */ {.num1=  0,.num2=  1,.num3=15,.num4=15,.num5= 0,.type=242,.name="CARD_CTL2"},
	/* G00.01[14] */ {.num1=  0,.num2=  1,.num3=14,.num4=14,.num5= 0,.type=241,.name="CARD_CTL1"},
	/* G00.01[13] */ {.num1=  0,.num2=  1,.num3=13,.num4=13,.num5= 0,.type=240,.name="CARD_CTL0"},
	/* G00.01[12] */ {.num1=  0,.num2=  1,.num3=12,.num4=12,.num5= 0,.type= 11,.name="BR0"},
	/* G00.01[11] */ {.num1=  0,.num2=  1,.num3=11,.num4=11,.num5= 0,.type= 21,.name="THERMAL"}, //TBD
	/* G00.01[10] */ {.num1=  0,.num2=  1,.num3=10,.num4=10,.num5= 0,.type= 28,.name="SYSTOP"},
	/* G00.01[09] */ {.num1=  0,.num2=  1,.num3= 9,.num4= 9,.num5= 0,.type= 26,.name="RBUS_BLKB"},
	/* G00.01[08] */ {.num1=  0,.num2=  1,.num3= 8,.num4= 8,.num5= 0,.type= 18,.name="RBUS"},
	/* G00.01[07] */ {.num1=  0,.num2=  1,.num3= 7,.num4= 7,.num5= 0,.type=257,.name="GMAC"},
	/* G00.01[06] */ {.num1=  0,.num2=  1,.num3= 6,.num4= 6,.num5= 0,.type= 12,.name="CA55"},
	/* G00.01[05] */ {.num1=  0,.num2=  1,.num3= 5,.num4= 5,.num5= 0,.type= 13,.name="CA55_CUL3"},
	/* G00.01[04] */ {.num1=  0,.num2=  1,.num3= 4,.num4= 4,.num5= 0,.type= 17,.name="CA55_CORE3"},
	/* G00.01[03] */ {.num1=  0,.num2=  1,.num3= 3,.num4= 3,.num5= 0,.type= 16,.name="CA55_CORE2"},
	/* G00.01[02] */ {.num1=  0,.num2=  1,.num3= 2,.num4= 2,.num5= 0,.type= 15,.name="CA55_CORE1"},
	/* G00.01[01] */ {.num1=  0,.num2=  1,.num3= 1,.num4= 1,.num5= 0,.type= 14,.name="CA55_CORE0"},
	/* G00.01[00] */ {.num1=  0,.num2=  1,.num3= 0,.num4= 0,.num5= 0,.type= 10,.name="SYSTEM"},

	/* G00.02[15] */ {.num1=  0,.num2=  2,.num3=15,.num4=15,.num5= 0,.type=  0,.name="Reserved"},
	/* G00.02[14] */ {.num1=  0,.num2=  2,.num3=14,.num4=14,.num5= 0,.type=261,.name="DUMMYMASTER1"},
	/* G00.02[13] */ {.num1=  0,.num2=  2,.num3=13,.num4=13,.num5= 0,.type=260,.name="DUMMYMASTER0"},
	/* G00.02[12] */ {.num1=  0,.num2=  2,.num3=12,.num4=12,.num5= 0,.type=756,.name="VCL5"},
	/* G00.02[11] */ {.num1=  0,.num2=  2,.num3=11,.num4=11,.num5= 0,.type=755,.name="VCL4"},
	/* G00.02[10] */ {.num1=  0,.num2=  2,.num3=10,.num4=10,.num5= 0,.type=754,.name="VCL3"},
	/* G00.02[09] */ {.num1=  0,.num2=  2,.num3= 9,.num4= 9,.num5= 0,.type=753,.name="VCL2"},
	/* G00.02[08] */ {.num1=  0,.num2=  2,.num3= 8,.num4= 8,.num5= 0,.type=752,.name="VCL1"},
	/* G00.02[07] */ {.num1=  0,.num2=  2,.num3= 7,.num4= 7,.num5= 0,.type=751,.name="VCL0"},
	/* G00.02[06] */ {.num1=  0,.num2=  2,.num3= 6,.num4= 6,.num5= 0,.type=750,.name="VCL"},
	/* G00.02[05] */ {.num1=  0,.num2=  2,.num3= 5,.num4= 5,.num5= 0,.type=230,.name="DRAM"},
	/* G00.02[04] */ {.num1=  0,.num2=  2,.num3= 4,.num4= 4,.num5= 0,.type=232,.name="DDRCTL"},
	/* G00.02[03] */ {.num1=  0,.num2=  2,.num3= 3,.num4= 3,.num5= 0,.type=233,.name="TZC"},
	/* G00.02[02] */ {.num1=  0,.num2=  2,.num3= 2,.num4= 2,.num5= 0,.type=231,.name="DDR_PHY0"},
	/* G00.02[01] */ {.num1=  0,.num2=  2,.num3= 1,.num4= 1,.num5= 0,.type= 23,.name="CPIOR"},
	/* G00.02[00] */ {.num1=  0,.num2=  2,.num3= 0,.num4= 0,.num5= 0,.type=221,.name="CBDMA0"},

	/* G00.03[15] */ {.num1=  0,.num2=  3,.num3=15,.num4=15,.num5= 0,.type=732,.name="VSCL0"},
	/* G00.03[14] */ {.num1=  0,.num2=  3,.num3=14,.num4=14,.num5= 0,.type=733,.name="VPOST0"},
	/* G00.03[13] */ {.num1=  0,.num2=  3,.num3=13,.num4=13,.num5= 0,.type=743,.name="TGEN"},
	/* G00.03[12] */ {.num1=  0,.num2=  3,.num3=12,.num4=12,.num5= 0,.type=744,.name="TCON"},
	/* G00.03[11] */ {.num1=  0,.num2=  3,.num3=11,.num4=11,.num5= 0,.type=737,.name="OSD3"},
	/* G00.03[10] */ {.num1=  0,.num2=  3,.num3=10,.num4=10,.num5= 0,.type=736,.name="OSD2"},
	/* G00.03[09] */ {.num1=  0,.num2=  3,.num3= 9,.num4= 9,.num5= 0,.type=735,.name="OSD1"},
	/* G00.03[08] */ {.num1=  0,.num2=  3,.num3= 8,.num4= 8,.num5= 0,.type=734,.name="OSD0"},
	/* G00.03[07] */ {.num1=  0,.num2=  3,.num3= 7,.num4= 7,.num5= 0,.type=745,.name="MIPITX"},
	/* G00.03[06] */ {.num1=  0,.num2=  3,.num3= 6,.num4= 6,.num5= 0,.type=731,.name="IMGREAD"},
	/* G00.03[05] */ {.num1=  0,.num2=  3,.num3= 5,.num4= 5,.num5= 0,.type=741,.name="GPOST3"},
	/* G00.03[04] */ {.num1=  0,.num2=  3,.num3= 4,.num4= 4,.num5= 0,.type=740,.name="GPOST2"},
	/* G00.03[03] */ {.num1=  0,.num2=  3,.num3= 3,.num4= 3,.num5= 0,.type=739,.name="GPOST1"},
	/* G00.03[02] */ {.num1=  0,.num2=  3,.num3= 2,.num4= 2,.num5= 0,.type=738,.name="GPOST0"},
	/* G00.03[01] */ {.num1=  0,.num2=  3,.num3= 1,.num4= 1,.num5= 0,.type=742,.name="DMIX"},
	/* G00.03[00] */ {.num1=  0,.num2=  3,.num3= 0,.num4= 0,.num5= 0,.type=730,.name="DISPSYS"},

	/* G00.04[15] */ {.num1=  0,.num2=  4,.num3=15,.num4=15,.num5= 0,.type=715,.name="VI3_CSIIW3"},
	/* G00.04[14] */ {.num1=  0,.num2=  4,.num3=14,.num4=14,.num5= 0,.type=714,.name="VI3_CSIIW2"},
	/* G00.04[13] */ {.num1=  0,.num2=  4,.num3=13,.num4=13,.num5= 0,.type=713,.name="VI3_CSIIW1"},
	/* G00.04[12] */ {.num1=  0,.num2=  4,.num3=12,.num4=12,.num5= 0,.type=712,.name="VI3_CSIIW0"},
	/* G00.04[11] */ {.num1=  0,.num2=  4,.num3=11,.num4=11,.num5= 0,.type=711,.name="VI2_CSIIW1"},
	/* G00.04[10] */ {.num1=  0,.num2=  4,.num3=10,.num4=10,.num5= 0,.type=710,.name="VI2_CSIIW0"},
	/* G00.04[09] */ {.num1=  0,.num2=  4,.num3= 9,.num4= 9,.num5= 0,.type=709,.name="VI1_CSIIW1"},
	/* G00.04[08] */ {.num1=  0,.num2=  4,.num3= 8,.num4= 8,.num5= 0,.type=708,.name="VI1_CSIIW0"},
	/* G00.04[07] */ {.num1=  0,.num2=  4,.num3= 7,.num4= 7,.num5= 0,.type=707,.name="VI0_CSIIW1"},
	/* G00.04[06] */ {.num1=  0,.num2=  4,.num3= 6,.num4= 6,.num5= 0,.type=706,.name="VI0_CSIIW0"},
	/* G00.04[05] */ {.num1=  0,.num2=  4,.num3= 5,.num4= 5,.num5= 0,.type=705,.name="MIPICSI5"},
	/* G00.04[04] */ {.num1=  0,.num2=  4,.num3= 4,.num4= 4,.num5= 0,.type=704,.name="MIPICSI4"},
	/* G00.04[03] */ {.num1=  0,.num2=  4,.num3= 3,.num4= 3,.num5= 0,.type=703,.name="MIPICSI3"},
	/* G00.04[02] */ {.num1=  0,.num2=  4,.num3= 2,.num4= 2,.num5= 0,.type=702,.name="MIPICSI2"},
	/* G00.04[01] */ {.num1=  0,.num2=  4,.num3= 1,.num4= 1,.num5= 0,.type=701,.name="MIPICSI1"},
	/* G00.04[00] */ {.num1=  0,.num2=  4,.num3= 0,.num4= 0,.num5= 0,.type=700,.name="MIPICSI0"},

	/* G00.05[15] */ {.num1=  0,.num2=  5,.num3=15,.num4=15,.num5= 0,.type=124,.name="SEMAPHORE"},
	/* G00.05[14] */ {.num1=  0,.num2=  5,.num3=14,.num4=14,.num5= 0,.type=123,.name="PRNG"},
	/* G00.05[13] */ {.num1=  0,.num2=  5,.num3=13,.num4=13,.num5= 0,.type=122,.name="OTPRX"},
	/* G00.05[12] */ {.num1=  0,.num2=  5,.num3=12,.num4=12,.num5= 0,.type=728,.name="VI23_CSIIW0"},
	/* G00.05[11] */ {.num1=  0,.num2=  5,.num3=11,.num4=11,.num5= 0,.type=727,.name="VI23_CSIIW3"},
	/* G00.05[10] */ {.num1=  0,.num2=  5,.num3=10,.num4=10,.num5= 0,.type=726,.name="VI23_CSIIW2"},
	/* G00.05[09] */ {.num1=  0,.num2=  5,.num3= 9,.num4= 9,.num5= 0,.type=725,.name="VI23_CSIIW1"},
	/* G00.05[08] */ {.num1=  0,.num2=  5,.num3= 8,.num4= 8,.num5= 0,.type=724,.name="MIPICSI23_SEL"},
	/* G00.05[07] */ {.num1=  0,.num2=  5,.num3= 7,.num4= 7,.num5= 0,.type=723,.name="VI5_CSIIW3"},
	/* G00.05[06] */ {.num1=  0,.num2=  5,.num3= 6,.num4= 6,.num5= 0,.type=722,.name="VI5_CSIIW2"},
	/* G00.05[05] */ {.num1=  0,.num2=  5,.num3= 5,.num4= 5,.num5= 0,.type=721,.name="VI5_CSIIW1"},
	/* G00.05[04] */ {.num1=  0,.num2=  5,.num3= 4,.num4= 4,.num5= 0,.type=720,.name="VI5_CSIIW0"},
	/* G00.05[03] */ {.num1=  0,.num2=  5,.num3= 3,.num4= 3,.num5= 0,.type=719,.name="VI4_CSIIW3"},
	/* G00.05[02] */ {.num1=  0,.num2=  5,.num3= 2,.num4= 2,.num5= 0,.type=718,.name="VI4_CSIIW2"},
	/* G00.05[01] */ {.num1=  0,.num2=  5,.num3= 1,.num4= 1,.num5= 0,.type=717,.name="VI4_CSIIW1"},
	/* G00.05[00] */ {.num1=  0,.num2=  5,.num3= 0,.num4= 0,.num5= 0,.type=716,.name="VI4_CSIIW0"},

	/* G00.06[15] */ {.num1=  0,.num2=  6,.num3=15,.num4=15,.num5= 0,.type=201,.name="USBC0"},
	/* G00.06[14] */ {.num1=  0,.num2=  6,.num3=14,.num4=14,.num5= 0,.type=202,.name="U3PHY0"},
	/* G00.06[13] */ {.num1=  0,.num2=  6,.num3=13,.num4=13,.num5= 0,.type=203,.name="USB30C0"},
	/* G00.06[12] */ {.num1=  0,.num2=  6,.num3=12,.num4=12,.num5= 0,.type=200,.name="UPHY0"},
	/* G00.06[11] */ {.num1=  0,.num2=  6,.num3=11,.num4=11,.num5= 0,.type=263,.name="U2AXI"},
	/* G00.06[10] */ {.num1=  0,.num2=  6,.num3=10,.num4=10,.num5= 0,.type=105,.name="UADBG"},
	/* G00.06[09] */ {.num1=  0,.num2=  6,.num3= 9,.num4= 9,.num5= 0,.type=103,.name="UA3"},
	/* G00.06[08] */ {.num1=  0,.num2=  6,.num3= 8,.num4= 8,.num5= 0,.type=102,.name="UA2"},
	/* G00.06[07] */ {.num1=  0,.num2=  6,.num3= 7,.num4= 7,.num5= 0,.type=101,.name="UA1"},
	/* G00.06[06] */ {.num1=  0,.num2=  6,.num3= 6,.num4= 6,.num5= 0,.type=100,.name="UA0"},
	/* G00.06[05] */ {.num1=  0,.num2=  6,.num3= 5,.num4= 5,.num5= 0,.type=134,.name="UADMA23"},
	/* G00.06[04] */ {.num1=  0,.num2=  6,.num3= 4,.num4= 4,.num5= 0,.type=133,.name="UADMA01"},
	/* G00.06[03] */ {.num1=  0,.num2=  6,.num3= 3,.num4= 3,.num5= 0,.type=703,.name="SPIND"},
	/* G00.06[02] */ {.num1=  0,.num2=  6,.num3= 2,.num4= 2,.num5= 0,.type= 24,.name="BCH"},
	/* G00.06[01] */ {.num1=  0,.num2=  6,.num3= 1,.num4= 1,.num5= 0,.type=246,.name="SPIFL"},
	/* G00.06[00] */ {.num1=  0,.num2=  6,.num3= 0,.num4= 0,.num5= 0,.type=233,.name="INTERRUPT"},

	/* G00.07[15] */ {.num1=  0,.num2=  7,.num3=15,.num4=15,.num5= 0,.type=  0,.name="SECGRP_PAI"},
	/* G00.07[14] */ {.num1=  0,.num2=  7,.num3=14,.num4=14,.num5= 0,.type=757,.name="NPU"},
	/* G00.07[13] */ {.num1=  0,.num2=  7,.num3=13,.num4=13,.num5= 0,.type=  0,.name="NICPAI"},
	/* G00.07[12] */ {.num1=  0,.num2=  7,.num3=12,.num4=12,.num5= 0,.type=  0,.name="NICPAII"},
	/* G00.07[11] */ {.num1=  0,.num2=  7,.num3=11,.num4=11,.num5= 0,.type=  0,.name="NICTOP"},
	/* G00.07[10] */ {.num1=  0,.num2=  7,.num3=10,.num4=10,.num5= 0,.type=215,.name="STC_AV4"},
	/* G00.07[09] */ {.num1=  0,.num2=  7,.num3= 9,.num4= 9,.num5= 0,.type=216,.name="STC_TIMESTAMP"},
	/* G00.07[08] */ {.num1=  0,.num2=  7,.num3= 8,.num4= 8,.num5= 0,.type=214,.name="STC_AV3"},
	/* G00.07[07] */ {.num1=  0,.num2=  7,.num3= 7,.num4= 7,.num5= 0,.type=  0,.name="Reserved"},
	/* G00.07[06] */ {.num1=  0,.num2=  7,.num3= 6,.num4= 6,.num5= 0,.type= 34,.name="SEC"},
	/* G00.07[05] */ {.num1=  0,.num2=  7,.num3= 5,.num4= 5,.num5= 0,.type= 33,.name="PNAND"},
	/* G00.07[04] */ {.num1=  0,.num2=  7,.num3= 4,.num4= 4,.num5= 0,.type=132,.name="AXI_DMA"},
	/* G00.07[03] */ {.num1=  0,.num2=  7,.num3= 3,.num4= 3,.num5= 0,.type=254,.name="MAILBOX"},
	/* G00.07[02] */ {.num1=  0,.num2=  7,.num3= 2,.num4= 2,.num5= 0,.type=108,.name="VIDEO_CODEC"},
	/* G00.07[01] */ {.num1=  0,.num2=  7,.num3= 1,.num4= 1,.num5= 0,.type=107,.name="VCE"},
	/* G00.07[00] */ {.num1=  0,.num2=  7,.num3= 0,.num4= 0,.num5= 0,.type=106,.name="VCD"},

	/* G00.08[15] */ {.num1=  0,.num2=  8,.num3=15,.num4=15,.num5= 0,.type= 31,.name="CM4"},
	/* G00.08[14] */ {.num1=  0,.num2=  8,.num3=14,.num4=14,.num5= 0,.type=131,.name="GDMAUA"},
	/* G00.08[13] */ {.num1=  0,.num2=  8,.num3=13,.num4=13,.num5= 0,.type=107,.name="UA7"},
	/* G00.08[12] */ {.num1=  0,.num2=  8,.num3=12,.num4=12,.num5= 0,.type=106,.name="UA6"},
	/* G00.08[11] */ {.num1=  0,.num2=  8,.num3=11,.num4=11,.num5= 0,.type=  0,.name="INT_AO"},
	/* G00.08[10] */ {.num1=  0,.num2=  8,.num3=10,.num4=10,.num5= 0,.type=253,.name="RTC"},
	/* G00.08[09] */ {.num1=  0,.num2=  8,.num3= 9,.num4= 9,.num5= 0,.type=  0,.name="PMC"},
	/* G00.08[08] */ {.num1=  0,.num2=  8,.num3= 8,.num4= 8,.num5= 0,.type=  0,.name="SYS_AO"},
	/* G00.08[07] */ {.num1=  0,.num2=  8,.num3= 7,.num4= 7,.num5= 0,.type= 34,.name="AXIM_PAII"},
	/* G00.08[06] */ {.num1=  0,.num2=  8,.num3= 6,.num4= 6,.num5= 0,.type= 33,.name="AXIM_PAI"},
	/* G00.08[05] */ {.num1=  0,.num2=  8,.num3= 5,.num4= 5,.num5= 0,.type= 32,.name="AXIM_TOP"},
	/* G00.08[04] */ {.num1=  0,.num2=  8,.num3= 4,.num4= 4,.num5= 0,.type=747,.name="AUD"},
	/* G00.08[03] */ {.num1=  0,.num2=  8,.num3= 3,.num4= 3,.num5= 0,.type=  0,.name="HBUS"},
	/* G00.08[02] */ {.num1=  0,.num2=  8,.num3= 2,.num4= 2,.num5= 0,.type=  0,.name="DPLL"},
	/* G00.08[01] */ {.num1=  0,.num2=  8,.num3= 1,.num4= 1,.num5= 0,.type=  0,.name="SECGRP_MAIN"},
	/* G00.08[00] */ {.num1=  0,.num2=  8,.num3= 0,.num4= 0,.num5= 0,.type=  0,.name="SECGRP_PAII"},

	/* G00.09[15] */ {.num1=  0,.num2=  9,.num3=15,.num4=15,.num5= 0,.type=116,.name="I2C6"},
	/* G00.09[14] */ {.num1=  0,.num2=  9,.num3=14,.num4=14,.num5= 0,.type=115,.name="I2C5"},
	/* G00.09[13] */ {.num1=  0,.num2=  9,.num3=13,.num4=13,.num5= 0,.type=114,.name="I2C4"},
	/* G00.09[12] */ {.num1=  0,.num2=  9,.num3=12,.num4=12,.num5= 0,.type=113,.name="I2C3"},
	/* G00.09[11] */ {.num1=  0,.num2=  9,.num3=11,.num4=11,.num5= 0,.type=112,.name="I2C2"},
	/* G00.09[10] */ {.num1=  0,.num2=  9,.num3=10,.num4=10,.num5= 0,.type=111,.name="I2C1"},
	/* G00.09[09] */ {.num1=  0,.num2=  9,.num3= 9,.num4= 9,.num5= 0,.type=110,.name="I2C0"},
	/* G00.09[08] */ {.num1=  0,.num2=  9,.num3= 8,.num4= 8,.num5= 0,.type=  0,.name="GPIO_AO_INT"},
	/* G00.09[07] */ {.num1=  0,.num2=  9,.num3= 7,.num4= 7,.num5= 0,.type=  0,.name="NICPAII"},
	/* G00.09[06] */ {.num1=  0,.num2=  9,.num3= 6,.num4= 6,.num5= 0,.type=746,.name="DISP_PWM"},
	/* G00.09[05] */ {.num1=  0,.num2=  9,.num3= 5,.num4= 5,.num5= 0,.type=  0,.name="SAR12B"},
	/* G00.09[04] */ {.num1=  0,.num2=  9,.num3= 4,.num4= 4,.num5= 0,.type=130,.name="AHB_DMA"},
	/* G00.09[03] */ {.num1=  0,.num2=  9,.num3= 3,.num4= 3,.num5= 0,.type=213,.name="STC_AV2"},
	/* G00.09[02] */ {.num1=  0,.num2=  9,.num3= 2,.num4= 2,.num5= 0,.type=212,.name="STC_AV1"},
	/* G00.09[01] */ {.num1=  0,.num2=  9,.num3= 1,.num4= 1,.num5= 0,.type=211,.name="STC_AV0"},
	/* G00.09[00] */ {.num1=  0,.num2=  9,.num3= 0,.num4= 0,.num5= 0,.type=210,.name="STC0"},

	/* G00.10[15] */ {.num1=  0,.num2= 10,.num3=15,.num4=15,.num5= 0,.type=800,.name="PD_CPIOR0"},
	/* G00.10[14] */ {.num1=  0,.num2= 10,.num3=14,.num4=14,.num5= 0,.type=801,.name="PD_CBDMA0"},
	/* G00.10[13] */ {.num1=  0,.num2= 10,.num3=13,.num4=13,.num5= 0,.type=802,.name="PD_CARD2"},
	/* G00.10[12] */ {.num1=  0,.num2= 10,.num3=12,.num4=12,.num5= 0,.type=803,.name="PD_CARD1"},
	/* G00.10[11] */ {.num1=  0,.num2= 10,.num3=11,.num4=11,.num5= 0,.type=804,.name="PD_CARD0"},
	/* G00.10[10] */ {.num1=  0,.num2= 10,.num3=10,.num4=10,.num5= 0,.type=805,.name="PD_CA55"},
	/* G00.10[09] */ {.num1=  0,.num2= 10,.num3= 9,.num4= 9,.num5= 0,.type=806,.name="PD_AXI_DMA"},
	/* G00.10[08] */ {.num1=  0,.num2= 10,.num3= 8,.num4= 8,.num5= 0,.type=125,.name="SPICB5"},
	/* G00.10[07] */ {.num1=  0,.num2= 10,.num3= 7,.num4= 7,.num5= 0,.type=124,.name="SPICB4"},
	/* G00.10[06] */ {.num1=  0,.num2= 10,.num3= 6,.num4= 6,.num5= 0,.type=123,.name="SPICB3"},
	/* G00.10[05] */ {.num1=  0,.num2= 10,.num3= 5,.num4= 5,.num5= 0,.type=122,.name="SPICB2"},
	/* G00.10[04] */ {.num1=  0,.num2= 10,.num3= 4,.num4= 4,.num5= 0,.type=121,.name="SPICB1"},
	/* G00.10[03] */ {.num1=  0,.num2= 10,.num3= 3,.num4= 3,.num5= 0,.type=120,.name="SPICB0"},
	/* G00.10[02] */ {.num1=  0,.num2= 10,.num3= 2,.num4= 2,.num5= 0,.type=119,.name="I2C9"},
	/* G00.10[01] */ {.num1=  0,.num2= 10,.num3= 1,.num4= 1,.num5= 0,.type=118,.name="I2C8"},
	/* G00.10[00] */ {.num1=  0,.num2= 10,.num3= 0,.num4= 0,.num5= 0,.type=117,.name="I2C7"},

	/* G00.11[15] */ {.num1=  0,.num2= 11,.num3=15,.num4=15,.num5= 0,.type=807,.name="PD_SPI_NOR"},
	/* G00.11[14] */ {.num1=  0,.num2= 11,.num3=14,.num4=14,.num5= 0,.type=808,.name="PD_SPI_ND"},
	/* G00.11[13] */ {.num1=  0,.num2= 11,.num3=13,.num4=13,.num5= 0,.type=809,.name="PD_SEC"},
	/* G00.11[12] */ {.num1=  0,.num2= 11,.num3=12,.num4=12,.num5= 0,.type=810,.name="PD_OSD3"},
	/* G00.11[11] */ {.num1=  0,.num2= 11,.num3=11,.num4=11,.num5= 0,.type=811,.name="PD_OSD2"},
	/* G00.11[10] */ {.num1=  0,.num2= 11,.num3=10,.num4=10,.num5= 0,.type=812,.name="PD_OSD1"},
	/* G00.11[09] */ {.num1=  0,.num2= 11,.num3= 9,.num4= 9,.num5= 0,.type=813,.name="PD_OSD0"},
	/* G00.11[08] */ {.num1=  0,.num2= 11,.num3= 8,.num4= 8,.num5= 0,.type=814,.name="PD_NPU"},
	/* G00.11[07] */ {.num1=  0,.num2= 11,.num3= 7,.num4= 7,.num5= 0,.type=815,.name="PD_NBS"},
	/* G00.11[06] */ {.num1=  0,.num2= 11,.num3= 6,.num4= 6,.num5= 0,.type=816,.name="PD_IMGREAD"},
	/* G00.11[05] */ {.num1=  0,.num2= 11,.num3= 5,.num4= 5,.num5= 0,.type=817,.name="PD_GMAC"},
	/* G00.11[04] */ {.num1=  0,.num2= 11,.num3= 4,.num4= 4,.num5= 0,.type=818,.name="PD_DUMMY1"},
	/* G00.11[03] */ {.num1=  0,.num2= 11,.num3= 3,.num4= 3,.num5= 0,.type=819,.name="PD_DUMMY0"},
	/* G00.11[02] */ {.num1=  0,.num2= 11,.num3= 2,.num4= 2,.num5= 0,.type=820,.name="PD_CSETR"},
	/* G00.11[01] */ {.num1=  0,.num2= 11,.num3= 1,.num4= 1,.num5= 0,.type=821,.name="PD_CSDBG"},
	/* G00.11[00] */ {.num1=  0,.num2= 11,.num3= 0,.num4= 0,.num5= 0,.type=822,.name="PD_CPIOR1"},

	/* G00.12[15] */ {.num1=  0,.num2= 12,.num3=15,.num4=15,.num5= 0,.type=823,.name="QCTRL"},
	/* G00.12[14] */ {.num1=  0,.num2= 12,.num3=14,.num4=14,.num5= 0,.type=824,.name="PD_HWUA_TX"},
	/* G00.12[13] */ {.num1=  0,.num2= 12,.num3=13,.num4=13,.num5= 0,.type=825,.name="PD_CM4"},
	/* G00.12[12] */ {.num1=  0,.num2= 12,.num3=12,.num4=12,.num5= 0,.type=826,.name="PD_AUD"},
	/* G00.12[11] */ {.num1=  0,.num2= 12,.num3=11,.num4=11,.num5= 0,.type=827,.name="PD_AHB_DMA"},
	/* G00.12[10] */ {.num1=  0,.num2= 12,.num3=10,.num4=10,.num5= 0,.type=828,.name="PD_VI5"},
	/* G00.12[09] */ {.num1=  0,.num2= 12,.num3= 9,.num4= 9,.num5= 0,.type=829,.name="PD_VI4"},
	/* G00.12[08] */ {.num1=  0,.num2= 12,.num3= 8,.num4= 8,.num5= 0,.type=830,.name="PD_VI23"},
	/* G00.12[07] */ {.num1=  0,.num2= 12,.num3= 7,.num4= 7,.num5= 0,.type=831,.name="PD_VI1_CSII"},
	/* G00.12[06] */ {.num1=  0,.num2= 12,.num3= 6,.num4= 6,.num5= 0,.type=832,.name="PD_VI0_CSII"},
	/* G00.12[05] */ {.num1=  0,.num2= 12,.num3= 5,.num4= 5,.num5= 0,.type=833,.name="PD_VCL"},
	/* G00.12[04] */ {.num1=  0,.num2= 12,.num3= 4,.num4= 4,.num5= 0,.type=834,.name="PD_VCE"},
	/* G00.12[03] */ {.num1=  0,.num2= 12,.num3= 3,.num4= 3,.num5= 0,.type=835,.name="PD_VCD"},
	/* G00.12[02] */ {.num1=  0,.num2= 12,.num3= 2,.num4= 2,.num5= 0,.type=836,.name="PD_USBC0"},
	/* G00.12[01] */ {.num1=  0,.num2= 12,.num3= 1,.num4= 1,.num5= 0,.type=837,.name="PD_USB30C0"},
	/* G00.12[00] */ {.num1=  0,.num2= 12,.num3= 0,.num4= 0,.num5= 0,.type=838,.name="PD_UART2AXI"},
	/* Gxx.xx[xx] */ {.num1=999,.num2=999,.num3=99,.num4=99,.num5= 0,.type=999,.name="Undefined"},
};

struct sunplus_dummy_master {
	void __iomem *clken; /* moon0 clken REGISTER */
	void __iomem *base0;  /* G14  DummyMaster0 REGISTER */
	void __iomem *base1;  /* G337 DummyMaster1 REGISTER */
	struct clk *clk_dm0, *clk_dm1;
};

struct sunplus_dummy_master *sp_dm_test;

static void test_help(void)
{
	pr_info(
		"sp_dm test:\n"
		"  -- This tool works for Sunplus SoC DummyMaster module --\n"
		"  show this help message\n"
		"    echo > /sys/module/sunplus_dm_test/parameters/test\n"
		"  dump clken status\n"
		"    echo 0 0 0 > /sys/module/sunplus_dm_test/parameters/test\n"
		"  dump dummy master register status\n"
		"    echo 10 0 0 > /sys/module/sunplus_dm_test/parameters/test\n"
		"  dump setting parameter\n"
		"    echo 11 0 0 > /sys/module/sunplus_dm_test/parameters/test\n"
		"  set start/end address (aa/bb)\n"
		"    echo 20 aa bb > /sys/module/sunplus_dm_test/parameters/test\n"
		"  set length and op_mode (cc/dd)\n"
		"    echo 21 cc dd > /sys/module/sunplus_dm_test/parameters/test\n"
		"  run dummy master test\n"
		"    echo 1 0 0 > /sys/module/sunplus_dm_test/parameters/test\n"
		"    echo 0 1 0 > /sys/module/sunplus_dm_test/parameters/test\n"
		"    echo 0 0 1 > /sys/module/sunplus_dm_test/parameters/test\n"
	);
}
u32 set_start_address = 0x00100000, set_end_address = 0x00200000;
u32 set_write_length = 0x10, set_op_mode = 0x01;

static int test_set(const char *val, const struct kernel_param *kp)
{
	int i, j, value1, value2, value3, echo_cnt;
	u32 tmp_value, tmp_value1, tmp_value2;
	u32 total_cmd_count[11], total_cycle_count[11], measure_bandwidth[11];

	echo_cnt = sscanf(val, "%d %d %d", &value1, &value2, &value3);
	//pr_info("value1= %d, value2= %d, value3= %d\n", value1, value2, value3);
	switch (echo_cnt) {
	case 0:
		test_help();
		break;
	case 1:
		pr_info("test 1\n");
		pr_info("value1=%d\n", value1);

		break;
	case 2:
		pr_info("test 2\n");
		pr_info("value1=%d, value2=%d\n", value1, value2);
		
		break;
	case 3:
		//pr_info("test 3\n");
		if ((value1 == 1) && (value2 == 0) && (value3 == 0)) {
			pr_info("DM0(EN) DM1(--) DM2(--)\n");
			/*
			 * Enable Dummy Master module
			 */
			writel((u32)value1 , sp_dm_test->base0 + DM_CONTROL); /* Dummy Master 0 Enable */
			writel(0x00000000 , sp_dm_test->base1 + DM_CONTROL); /* Dummy Master 1 Disable */
			/*
			 * Calculate req_period from target BW 10% to 100%
			 */
			tmp_value1 = Q654_SYSTEM_CLOCK;
			tmp_value2 = Q654_MAX_BANDWIDTH_DM0/(set_write_length*16*10);
			for(i = 0; i < 10; i++) {
				if(tmp_value1%(tmp_value2 *(i+1)) != 0) {
					tmp_value = tmp_value1/(tmp_value2*(i+1));
					tmp_value = tmp_value1 - (tmp_value * tmp_value2 * (i+1));
					tmp_value = tmp_value * 100 / tmp_value2/(i+1);
					request_period_bw[i] = (tmp_value << 16) | (tmp_value1/tmp_value2/(i+1));
					//pr_info("%d%% %d cmd %d cycle, req_period %d.%02d\n", (i+1)*10, tmp_value2*(i+1), tmp_value1, tmp_value1/tmp_value2/(i+1), tmp_value);
				} else {
					request_period_bw[i] = (0 << 16) | (tmp_value1/tmp_value2/(i+1));
					//pr_info("%d%% %d cmd %d cycle, req_period %d.00\n", (i+1)*10, tmp_value2*(i+1), tmp_value1, tmp_value1/tmp_value2/(i+1));
				}
			}
			/*
			 * LEN=16, WBE=1
			 * OP_MODE=1/2/3/15(SP7021), OP_MODE=1/2/3/10(Q645/Q654)
			 */
			tmp_value = 0;
			tmp_value = set_write_length << 8 | 1 << 4 | set_op_mode;
			writel(tmp_value , sp_dm_test->base0 + DM_OP_MODE); /* LEN=16, WBE=1, OP_MODE=1/2/3/15 */
			/*
			 * Test Start Address, default 0x00100000, selectable
			 * Test End Address, default 0x00200000, selectable
			 */
			writel(set_start_address , sp_dm_test->base0 + DM_ADDR_BASE);
			writel(set_end_address , sp_dm_test->base0 + DM_ADDR_OFS);
			/* Urgent Enable and set urgent value 0x20 */
			writel(0x80000020 , sp_dm_test->base0 + DM_URGENT);
			/* set golden value as 0x12345678 */
			writel(0x12345678 , sp_dm_test->base0 + DM_GOLDEN_VALUE);

			for(i = 0; i < 11; i++) {
				writel(request_period_bw[i] , sp_dm_test->base0 + DM_REQ_PERIOD); /* Request period 10% - 100% */
				writel(0x00000101 , sp_dm_test->base0 + DM_CONTROL); /* Dummy Master 0 Enable & SOFT_RST */
				//writel(0x00000101 , sp_dm_test->base1 + DM_CONTROL); /* Dummy Master 1 Enable & SOFT_RST */
				//writel(0x00000101 , sp_dm_test->base2 + DM_CONTROL); /* Dummy Master 2 Enable & SOFT_RST */
				udelay(1000);
				total_cmd_count[i] = FIELD_GET(GENMASK(31, 0), readl(sp_dm_test->base0 + DM_TOTAL_CMD_CNT));
				total_cycle_count[i] = FIELD_GET(GENMASK(31, 0), readl(sp_dm_test->base0 + DM_TOTAL_CYCLE_CNT));
				unfinished_cmd_cnt[i] = FIELD_GET(GENMASK(31, 0), readl(sp_dm_test->base0 + DM_UNFINISH_CNT));
				//pr_info("set request_period_bw[%d] = %d\n", i, request_period_bw[i]);
				//pr_info("get total_cmd_count[%d] = %d,total_cycle_count[%d] = %d\n", i, total_cmd_count[i], i, total_cycle_count[i]);
			}
			writel(0x00000000 , sp_dm_test->base0 + DM_CONTROL); /* Dummy Master 0 Disable */
			writel(0x00000000 , sp_dm_test->base1 + DM_CONTROL); /* Dummy Master 1 Disable */

			pr_info("sysclk = %dMHz,  dramclk = %dMHz, max_BW = %dMBps\n",
				Q654_SYSTEM_CLOCK/1000, Q654_DRAM_CLOCK/1000, Q654_MAX_BANDWIDTH_DM0/1000);
			pr_info("-- Dummy Master Parameter Setting --\n");
			tmp_value = FIELD_GET(GENMASK(3, 0), readl(sp_dm_test->base0 + DM_OP_MODE));
			pr_info("    OP_Mode = %d (%s)\n", tmp_value, show_op_mode[tmp_value]);	

			tmp_value1 = FIELD_GET(GENMASK(4, 4), readl(sp_dm_test->base0 + DM_OP_MODE));
			tmp_value2 = FIELD_GET(GENMASK(15, 8), readl(sp_dm_test->base0 + DM_OP_MODE));
			pr_info("    WBE = %d, Request Length = (%d * 16 Byte)\n", tmp_value1, tmp_value2);

			tmp_value1 = FIELD_GET(GENMASK(31, 31), readl(sp_dm_test->base0 + DM_URGENT));
			tmp_value2 = FIELD_GET(GENMASK(6, 0), readl(sp_dm_test->base0 + DM_URGENT));
			if (tmp_value1 == 1)
				pr_info("    Urgent Enable, and Limit = %d\n", tmp_value2);
			else
				pr_info("    Urgent Disable\n");
			
			tmp_value1 = FIELD_GET(GENMASK(30, 0), readl(sp_dm_test->base0 + DM_ADDR_BASE));		
			tmp_value2 = FIELD_GET(GENMASK(30, 0), readl(sp_dm_test->base0 + DM_ADDR_OFS));
			pr_info("    Test Address From 0x%08x to 0x%08x\n", tmp_value1, tmp_value2);

			pr_info("-- Dummy Master Test Result --\n");
			pr_info("-- Target / Measure / Theory / percentage(%%) --\n");
			for(i = 0; i < 11; i++) {
				//measure_bandwidth[i] = (total_cmd_count[i] * 256 * (Q654_SYSTEM_CLOCK / 100000) / (total_cycle_count[i] / 100));
				measure_bandwidth[i] = (u32)DIV_ROUND_CLOSEST_ULL((u64)total_cmd_count[i]*256*Q654_SYSTEM_CLOCK, (u64)total_cycle_count[i]*1000);
				measure_bandwidth[i] = (u32)DIV_ROUND_CLOSEST_ULL((u64)measure_bandwidth[i]*1000*1000, (u64)1024*1024);
				//pr_info("%d%% req_period 0x%08x\n", (i+1)*10, request_period_bw[i]);
				if (i == 10)
					pr_info("\n    MAX    (%04d MB/s/ %04d MB/s), %d%%\n", measure_bandwidth[i], target_bandwidth[i], measure_bandwidth[i]*100/target_bandwidth[i]);
				else
					pr_info("    %03d%%   (%04d MB/s/ %04d MB/s), %d%%\n", (i+1)*10, measure_bandwidth[i], target_bandwidth[i], measure_bandwidth[i]*100/target_bandwidth[i]);
			}
		
		}
		else if ((value1 == 0) && (value2 == 1) && (value3 == 0)) {
			pr_info("DM0(--) DM1(EN) DM2(--)\n");
			/*
			 * Enable Dummy Master module
			 */
			writel(0x00000000 , sp_dm_test->base0 + DM_CONTROL); /* Dummy Master 0 Enable */
			writel((u32)value1 , sp_dm_test->base1 + DM_CONTROL); /* Dummy Master 1 Disable */
			/*
			 * Calculate req_period from target BW 10% to 100%
			 */
			tmp_value1 = Q654_SYSTEM_CLOCK;
			tmp_value2 = Q654_MAX_BANDWIDTH_DM1/(set_write_length*16*10);
			for(i = 0; i < 10; i++) {
				if(tmp_value1%(tmp_value2 *(i+1)) != 0) {
					tmp_value = tmp_value1/(tmp_value2*(i+1));
					tmp_value = tmp_value1 - (tmp_value * tmp_value2 * (i+1));
					tmp_value = tmp_value * 100 / tmp_value2/(i+1);
					request_period_bw[i] = (tmp_value << 16) | (tmp_value1/tmp_value2/(i+1));
					//pr_info("%d%% %d cmd %d cycle, req_period %d.%02d\n", (i+1)*10, tmp_value2*(i+1), tmp_value1, tmp_value1/tmp_value2/(i+1), tmp_value);
				} else {
					request_period_bw[i] = (0 << 16) | (tmp_value1/tmp_value2/(i+1));
					//pr_info("%d%% %d cmd %d cycle, req_period %d.00\n", (i+1)*10, tmp_value2*(i+1), tmp_value1, tmp_value1/tmp_value2/(i+1));
				}
			}
			/*
			 * LEN=16, WBE=1
			 * OP_MODE=1/2/3/15(SP7021), OP_MODE=1/2/3/10(Q645/Q654)
			 */
			tmp_value = 0;
			tmp_value = set_write_length << 8 | 1 << 4 | set_op_mode;
			writel(tmp_value , sp_dm_test->base1 + DM_OP_MODE); /* LEN=16, WBE=1, OP_MODE=1/2/3/15 */
			/*
			 * Test Start Address, default 0x00100000, selectable
			 * Test End Address, default 0x00200000, selectable
			 */
			writel(set_start_address , sp_dm_test->base1 + DM_ADDR_BASE);
			writel(set_end_address , sp_dm_test->base1 + DM_ADDR_OFS);
			/* Urgent Enable and set urgent value 0x20 */
			writel(0x80000020 , sp_dm_test->base1 + DM_URGENT);
			/* set golden value as 0x12345678 */
			writel(0x12345678 , sp_dm_test->base1 + DM_GOLDEN_VALUE);

			for(i = 0; i < 11; i++) {
				writel(request_period_bw[i] , sp_dm_test->base1 + DM_REQ_PERIOD); /* Request period 10% - 100% */
				//writel(0x00000101 , sp_dm_test->base0 + DM_CONTROL); /* Dummy Master 0 Enable & SOFT_RST */
				writel(0x00000101 , sp_dm_test->base1 + DM_CONTROL); /* Dummy Master 1 Enable & SOFT_RST */
				//writel(0x00000101 , sp_dm_test->base2 + DM_CONTROL); /* Dummy Master 2 Enable & SOFT_RST */
				udelay(1000);
				total_cmd_count[i] = FIELD_GET(GENMASK(31, 0), readl(sp_dm_test->base1 + DM_TOTAL_CMD_CNT));
				total_cycle_count[i] = FIELD_GET(GENMASK(31, 0), readl(sp_dm_test->base1 + DM_TOTAL_CYCLE_CNT));
				unfinished_cmd_cnt[i] = FIELD_GET(GENMASK(31, 0), readl(sp_dm_test->base1 + DM_UNFINISH_CNT));
				//pr_info("set request_period_bw[%d] = %d\n", i, request_period_bw[i]);
				//pr_info("get total_cmd_count[%d] = %d,total_cycle_count[%d] = %d\n", i, total_cmd_count[i], i, total_cycle_count[i]);
			}
			writel(0x00000000 , sp_dm_test->base0 + DM_CONTROL); /* Dummy Master 0 Disable */
			writel(0x00000000 , sp_dm_test->base1 + DM_CONTROL); /* Dummy Master 1 Disable */

			/* For Q654 */
			pr_info("sysclk = %dMHz,  dramclk = %dMHz, max_BW = %dMBps\n",
				Q654_SYSTEM_CLOCK/1000, Q654_DRAM_CLOCK/1000, Q654_MAX_BANDWIDTH_DM1/1000);
			pr_info("-- Dummy Master Parameter Setting --\n");
			tmp_value = FIELD_GET(GENMASK(3, 0), readl(sp_dm_test->base1 + DM_OP_MODE));
			pr_info("    OP_Mode = %d (%s)\n", tmp_value, show_op_mode[tmp_value]);	

			tmp_value1 = FIELD_GET(GENMASK(4, 4), readl(sp_dm_test->base1 + DM_OP_MODE));
			tmp_value2 = FIELD_GET(GENMASK(15, 8), readl(sp_dm_test->base1 + DM_OP_MODE));
			pr_info("    WBE = %d, Request Length = (%d * 16 Byte)\n", tmp_value1, tmp_value2);

			tmp_value1 = FIELD_GET(GENMASK(31, 31), readl(sp_dm_test->base1 + DM_URGENT));
			tmp_value2 = FIELD_GET(GENMASK(6, 0), readl(sp_dm_test->base1 + DM_URGENT));
			if (tmp_value1 == 1)
				pr_info("    Urgent Enable, and Limit = %d\n", tmp_value2);
			else
				pr_info("    Urgent Disable\n");
			
			tmp_value1 = FIELD_GET(GENMASK(30, 0), readl(sp_dm_test->base1 + DM_ADDR_BASE));		
			tmp_value2 = FIELD_GET(GENMASK(30, 0), readl(sp_dm_test->base1 + DM_ADDR_OFS));
			pr_info("    Test Address From 0x%08x to 0x%08x\n", tmp_value1, tmp_value2);

			pr_info("-- Dummy Master Test Result --\n");
			pr_info("-- Target / Measure / Theory / percentage(%%) --\n");
			for(i = 0; i < 11; i++) {
				//measure_bandwidth[i] = (total_cmd_count[i] * 256 * (Q654_SYSTEM_CLOCK / 100) / (total_cycle_count[i] / 100))/1000;
				measure_bandwidth[i] = (u32)DIV_ROUND_CLOSEST_ULL((u64)total_cmd_count[i]*256*Q654_SYSTEM_CLOCK, (u64)total_cycle_count[i]*1000);
				measure_bandwidth[i] = (u32)DIV_ROUND_CLOSEST_ULL((u64)measure_bandwidth[i]*1000*1000, (u64)1024*1024);
				//pr_info("%d%% req_period 0x%08x\n", (i+1)*10, request_period_bw[i]);
				if (i == 10)
					pr_info("\n    MAX    (%04d MB/s/ %04d MB/s), %d%%\n", measure_bandwidth[i], target_bandwidth[i]/2, measure_bandwidth[i]*100/(target_bandwidth[i]/2));
				else
					pr_info("    %03d%%   (%04d MB/s/ %04d MB/s), %d%%\n", (i+1)*10, measure_bandwidth[i], target_bandwidth[i]/2, measure_bandwidth[i]*100/(target_bandwidth[i]/2));
			}
		}
		else if ((value1 == 0) && (value2 == 0) && (value3 == 1)) {
			pr_info("DM0(--) DM1(--) DM2(EN)\n");
			//TBD
		}
		else if ((value1 == 1) && (value2 == 1) && (value3 == 0)) {
			pr_info("DM0(EN) DM1(EN) DM2(--)\n");
			//TBD
		}
		else if ((value1 == 1) && (value2 == 0) && (value3 == 1)) {
			pr_info("DM0(EN) DM1(--) DM2(EN)\n");
			//TBD
		}
		else if ((value1 == 0) && (value2 == 1) && (value3 == 1)) {
			pr_info("DM0(--) DM1(EN) DM2(EN)\n");
			//TBD
		}
		else if ((value1 == 1) && (value2 == 1) && (value3 == 1)) {
			pr_info("DM0(EN) DM1(EN) DM2(EN)\n");
			//TBD
		}
		else if ((value1 == 10) && (value2 == 0) && (value3 == 0)) {
			pr_info("Dump dummy master reg setting\n");
			for(i = 0; i < 11; i++) {
				pr_info("G014.%02d : 0x%08x(%s)\n", i, readl(sp_dm_test->base0 + i * 4),
					DummyMasterDefine[i]);
			}
			pr_info("\n");
			for(i = 0; i < 11; i++) {
				pr_info("G337.%02d : 0x%08x(%s)\n", i, readl(sp_dm_test->base1 + i * 4),
					DummyMasterDefine[i]);
			}
		}
		else if ((value1 == 11) && (value2 == 0) && (value3 == 0)) {
			pr_info("Dump current setting\n");
			pr_info("sysclk = %dMHz,  dramclk = %dMHz, max_BW = %dMBps(DM0) %dMBps(DM1)\n",
				Q654_SYSTEM_CLOCK/1000, Q654_DRAM_CLOCK/1000, Q654_MAX_BANDWIDTH_DM0/1000, Q654_MAX_BANDWIDTH_DM1/1000);	
			pr_info("set_start_address = 0x%08x(%d)\nset_end_address = 0x%08x(%d)\n",
				set_start_address, set_start_address, set_end_address, set_end_address);
			pr_info("set_write_length = %d\nset_op_mode = %d (%s)\n",
				set_write_length, set_op_mode, show_op_mode[set_op_mode]);
		}
		else if (value1 == 20) {
			pr_info("set address parameter\n");
			set_start_address = (u32)value2;
			set_end_address = (u32)value3;
			pr_info("set_start_address = 0x%08x(%d)\nset_end_address = 0x%08x(%d)\n",
				set_start_address, set_start_address, set_end_address, set_end_address);
		}
		else if (value1 == 21) {
			pr_info("set length and op_mode parameter\n");
			set_write_length = (u32)value2;
			set_op_mode = (u32)value3;
			pr_info("length = %d\nset_op_mode = %d (%s)\n",
				set_write_length, set_op_mode, show_op_mode[set_op_mode]);

		} else {
			pr_info("Dump clk enable status\n");
			for(i = 0; i < 12; i++) {
				tmp_value = readl(sp_dm_test->clken + (i+1) * 4);
				for(j = 0; j < 16; j++) {
					sp_moon0[i*16+j].num5 = tmp_value & BIT(15-j);
				}
			}
			for(i = 0; i < 192; i++) {
				if (sp_moon0[i].type != 0) buf_tmp[sp_moon0[i].type] = i;
			}
			pr_info("\n");

			pr_info("CHIP Revision: 0x%08x\n", readl(sp_dm_test->clken + 0 * 4));

			for(i = 0; i < 1000; i++)
				buf_tmp[i] = 192;
			j = 0;
			for(i = 0; i < 192; i++) {
				sp_moon0[i].type = 0;
				if(!strcmp(sp_moon0[i].name,"Reserved"))
					sp_moon0[i].type = 0;
				else {
					sp_moon0[i].type = j+1;
					buf_tmp[j] = i;
					j++;
				}
			}
			for(i = 0; i < 192/4; i++) {
				if(buf_tmp[i*4] != 192)
					pr_info("%s:%s, %s:%s, %s:%s, %s:%s\n",
					sp_moon0[buf_tmp[i*4+0]].name, (sp_moon0[buf_tmp[i*4+0]].num5)?"(O)":"(--)",
					sp_moon0[buf_tmp[i*4+1]].name, (sp_moon0[buf_tmp[i*4+1]].num5)?"(O)":"(--)",
					sp_moon0[buf_tmp[i*4+2]].name, (sp_moon0[buf_tmp[i*4+2]].num5)?"(O)":"(--)",
					sp_moon0[buf_tmp[i*4+3]].name, (sp_moon0[buf_tmp[i*4+3]].num5)?"(O)":"(--)");
			}
		}
		break;
	}

	return 0;
}

static const struct kernel_param_ops test_ops = {
	.set = test_set,
};
module_param_cb(test, &test_ops, NULL, 0600);

static const struct of_device_id sp_dm_test_of_match[] = {
	{ .compatible = "sunplus,sp7350-dm-test" },
	{}
};
MODULE_DEVICE_TABLE(of, sp_dm_test_of_match);

static int sp_dm_test_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunplus_dummy_master *priv;
	int ret = 0;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	sp_dm_test = priv;

	priv->clken = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->clken))
		return PTR_ERR(priv->clken);

	priv->base0 = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(priv->base0))
		return PTR_ERR(priv->base0);

	priv->base1 = devm_platform_ioremap_resource(pdev, 2);
	if (IS_ERR(priv->base1))
		return PTR_ERR(priv->base1);

	return ret;
}

static struct platform_driver sp_dm_test_driver = {
	.probe		= sp_dm_test_probe,
	.driver		= {
		.name	= "sp_dm_test",
		.owner	= THIS_MODULE,
		.of_match_table = sp_dm_test_of_match,
	},
};

module_platform_driver(sp_dm_test_driver);

MODULE_DESCRIPTION("Sunplus SoC DM Test Driver");
MODULE_AUTHOR("Hammer Hsieh <hammerh0314@gmail.com>");
MODULE_LICENSE("GPL");