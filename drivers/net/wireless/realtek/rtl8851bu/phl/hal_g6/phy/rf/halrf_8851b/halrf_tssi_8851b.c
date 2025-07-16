/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#include "../halrf_precomp.h"

#ifdef RF_8851B_SUPPORT

void _tssi_backup_bb_registers_8851b(
	struct rf_info *rf,
	enum phl_phy_idx phy,
	u32 *reg,
	u32 *reg_backup,
	u32 reg_num)
{
	u32 i;

	for (i = 0; i < reg_num; i++) {
		reg_backup[i] = halrf_rreg(rf, reg[i], MASKDWORD);

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] Backup BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
	}
}

void _tssi_reload_bb_registers_8851b(
	struct rf_info *rf,
	enum phl_phy_idx phy,
	u32 *reg,
	u32 *reg_backup,
	u32 reg_num)

{
	u32 i;

	for (i = 0; i < reg_num; i++) {
		halrf_wreg(rf, reg[i], MASKDWORD, reg_backup[i]);

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] Reload BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
	}
}

u8 _halrf_ch_to_idx_8851b(struct rf_info *rf, u8 channel)
{
	u8	channelIndex;

	if (channel >= 1 && channel <= 14)
		channelIndex = channel - 1;
	else if (channel >= 36 && channel <= 64)
		channelIndex = (channel - 36) / 2 + 14;
	else if (channel >= 100 && channel <= 144)
		channelIndex = ((channel - 100) / 2) + 15 + 14;
	else if (channel >= 149 && channel <= 177)
		channelIndex = ((channel - 149) / 2) + 38 + 14;
	else
		channelIndex = 0;

	return channelIndex;
}

u8 _halrf_idx_to_ch_8851b(struct rf_info *rf, u8 idx)
{
	u8 channelIndex;

	if (idx >= 0 && idx <= 13)
		channelIndex = idx + 1;
	else if (idx >= (0 + 14) && idx <= (14 + 14))
		channelIndex = (idx - 14) * 2 + 36;
	else if (idx >= (15 + 14) && idx <= (37 + 14))
		channelIndex = (idx - 15 - 14) * 2 + 100;
	else if (idx >= (38 + 14) && idx <= (52 + 14))
		channelIndex = (idx - 38 - 14) * 2 + 149;
	else
		channelIndex = 0;

	return channelIndex;
}

void _halrf_tssi_hw_tx_8851b(struct rf_info *rf,
			enum phl_phy_idx phy, u8 path, u16 cnt, u16 period, s16 dbm, u32 rate, u8 bw,
			bool enable)
{
	struct rf_pmac_tx_info tx_info = {0};

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s\n", __func__);

	tx_info.ppdu = rate;
	tx_info.mcs = 7;
	tx_info.bw = bw;
	tx_info.nss = 1;
	tx_info.gi = 1;
	tx_info.txagc_cw = 0;
	tx_info.dbm = dbm;
	tx_info.cnt = cnt;
	tx_info.time = 20;
	tx_info.period = period;
	tx_info.length = 0;

	halrf_set_pmac_tx(rf, phy, path, &tx_info, enable, false);
}

void _halrf_tssi_set_sys_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s\n", __func__);

	halrf_wreg(rf, 0x12bc, 0x000ffff0, 0xb5b5);
	halrf_wreg(rf, 0x32bc, 0x000ffff0, 0xb5b5);
	halrf_wreg(rf, 0x0300, 0xff000000, 0x16);
	halrf_wreg(rf, 0x0304, 0x0000ffff, 0x1f19);
	halrf_wreg(rf, 0x0308, 0xff000000, 0x1c);
	halrf_wreg(rf, 0x0314, 0xffff0000, 0x2041);
	halrf_wreg(rf, 0x0318, 0xffffffff, 0x20012041);
	halrf_wreg(rf, 0x0324, 0xffff0000, 0x2001);
	halrf_wreg(rf, 0x0020, 0x00006000, 0x3);
	halrf_wreg(rf, 0x0024, 0x00006000, 0x3);
	halrf_wreg(rf, 0x0704, 0xffff0000, 0x601e);
	halrf_wreg(rf, 0x2704, 0xffff0000, 0x601e);
	halrf_wreg(rf, 0x0700, 0xf0000000, 0x4);
	halrf_wreg(rf, 0x2700, 0xf0000000, 0x4);
	halrf_wreg(rf, 0x0650, 0x3c000000, 0x0);
	halrf_wreg(rf, 0x2650, 0x3c000000, 0x0);
	
	if (channel >= 1 && channel <= 14) {
		halrf_wreg(rf, 0x120c, 0x000000ff, 0x33);
		halrf_wreg(rf, 0x12c0, 0x0ff00000, 0x33);
		halrf_wreg(rf, 0x58f8, 0x40000000, 0x1);
		halrf_wreg(rf, 0x5814, 0x20000000, 0x0);
	} else {	
		halrf_wreg(rf, 0x120c, 0x000000ff, 0x44);
		halrf_wreg(rf, 0x12c0, 0x0ff00000, 0x44);
		halrf_wreg(rf, 0x58f8, 0x40000000, 0x0);
		halrf_wreg(rf, 0x5814, 0x20000000, 0x0);
	}
}

void _halrf_tssi_ini_txpwr_ctrl_bb_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	halrf_wreg(rf, 0x566c, 0x00001000, 0x0);
	halrf_wreg(rf, 0x5800, 0xffffffff, 0x003f807f);
#if 0
	halrf_wreg(rf, 0x5804, 0xffffffff, 0x02b27000);
	halrf_wreg(rf, 0x5808, 0xffffffff, 0x02b27000);
#endif
	halrf_wreg(rf, 0x580c, 0x0000007f, 0x40);
	halrf_wreg(rf, 0x580c, 0x0fffff00, 0x00040);
	halrf_wreg(rf, 0x5810, 0xffffffff, 0x59010000);
	halrf_wreg(rf, 0x5814, 0x01ffffff, 0x026d000);
	halrf_wreg(rf, 0x5814, 0xf8000000, 0x00);

	if (phl_is_mp_mode(rf->phl_com))
		halrf_wreg(rf, 0x5818, 0x00ffffff, 0x2c1800);
	else
		halrf_wreg(rf, 0x5818, 0x00ffffff, 0x2c18e8);

	halrf_wreg(rf, 0x5818, 0x07000000, 0x0);
	halrf_wreg(rf, 0x5818, 0xf0000000, 0x0);
	halrf_wreg(rf, 0x581c, 0x3fffffff, 0x3dc80280);
	halrf_wreg(rf, 0x5820, 0xffffffff, 0x00000080);
	halrf_wreg(rf, 0x58e8, 0x0000003f, 0x04);
	halrf_wreg(rf, 0x580c, 0x10000000, 0x1);
	halrf_wreg(rf, 0x580c, 0x40000000, 0x1);
	halrf_wreg(rf, 0x5834, 0x3fffffff, 0x000115f2);
	halrf_wreg(rf, 0x5838, 0x7fffffff, 0x0000121);
	halrf_wreg(rf, 0x5854, 0x3fffffff, 0x000115f2);
	halrf_wreg(rf, 0x5858, 0x7fffffff, 0x0000121);
	halrf_wreg(rf, 0x5860, 0x80000000, 0x0);
	halrf_wreg(rf, 0x5864, 0x07ffffff, 0x00801ff);
	halrf_wreg(rf, 0x5898, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x589c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x58a4, 0x000000ff, 0x16);
	halrf_wreg(rf, 0x58b0, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x58b4, 0x7fffffff, 0x0a002000);
	halrf_wreg(rf, 0x58b8, 0x7fffffff, 0x00007628);

	/*GNT_WL==0 & 0x58bc[18]==1, Bypass TSSI*/
	/*GNT_BT==1 & 0x58bc[20]==1, Bypass TSSI*/
	/*GNT_BT_TX==1 & 0x58bc[22]==1, Bypass TSSI*/

	if (phl_is_mp_mode(rf->phl_com))
		halrf_wreg(rf, 0x58bc, 0x07ffffff, 0x7f7807f);
	else
		halrf_wreg(rf, 0x58bc, 0x07ffffff, 0x7a7807f);

	halrf_wreg(rf, 0x58c0, 0xfffe0000, 0x003f);
	halrf_wreg(rf, 0x58c4, 0xffffffff, 0x0003ffff);
	halrf_wreg(rf, 0x58c8, 0x00ffffff, 0x000000);
	halrf_wreg(rf, 0x58c8, 0xf0000000, 0x0);
	halrf_wreg(rf, 0x58cc, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x58d0, 0x07ffffff, 0x2008101);
	halrf_wreg(rf, 0x58d4, 0x000000ff, 0x00);
	halrf_wreg(rf, 0x58d4, 0x0003fe00, 0x0ff);
	halrf_wreg(rf, 0x58d4, 0x07fc0000, 0x100);
	halrf_wreg(rf, 0x58d8, 0xffffffff, 0x8008016c);
	halrf_wreg(rf, 0x58dc, 0x0001ffff, 0x0807f);
	halrf_wreg(rf, 0x58dc, 0xfff00000, 0x800);
	halrf_wreg(rf, 0x58f0, 0x0003ffff, 0x001ff);
	halrf_wreg(rf, 0x58f4, 0x000fffff, 0x00000);
	halrf_wreg(rf, 0x58f8, 0x000fffff, 0x00000);
}

void _halrf_tssi_ini_txpwr_ctrl_bb_he_tb_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	halrf_wreg(rf, 0x58a0, MASKDWORD, 0x000000fe);
	halrf_wreg(rf, 0x58e4, 0x0000007f, 0x1f);
}

void _halrf_tssi_set_dck_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	halrf_wreg(rf, 0x580c, 0x0fff0000, 0x000);
	halrf_wreg(rf, 0x5814, 0x00001000, 0x1);
	halrf_wreg(rf, 0x5814, 0x00002000, 0x1);
	halrf_wreg(rf, 0x5814, 0x00004000, 0x1);
	halrf_wreg(rf, 0x5814, 0x00038000, 0x3);
	halrf_wreg(rf, 0x5814, 0x003c0000, 0x5);
	halrf_wreg(rf, 0x5814, 0x18000000, 0x0);
}

void _halrf_tssi_set_bbgain_split_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	halrf_wreg(rf, 0x5818, 0x08000000, 0x1);
	halrf_wreg(rf, 0x58d4, 0xf0000000, 0x7);
	halrf_wreg(rf, 0x58f0, 0x000c0000, 0x1);
	halrf_wreg(rf, 0x58f0, 0xfff00000, 0x400);
}

void _halrf_tssi_set_tmeter_tbl_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	struct halrf_pwr_track_info *pwr_trk = &rf->pwr_track;
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	struct rtw_hal_com_t *hal = rf->hal_com;

	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;

	u8 i, thermal = 0xff, j;
	u32 thermal_offset_tmp = 0;
	s8 thermal_offset[64] = {0};
	s8 thermal_up_a[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_a[DELTA_SWINGIDX_SIZE] = {0};

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	if (tssi_info->tssi_type[HW_PHY_0] == TSSI_CAL) {
		halrf_wreg(rf, 0x5810, 0x00010000, 0x0);
		halrf_wreg(rf, 0x5810, 0x01000000, 0x1);

		halrf_wreg(rf, 0x5810, 0x0000fc00, 32);
		halrf_wreg(rf, 0x5864, 0x03f00000, 32);

		for (i = 0; i < 64; i = i + 4) {
			thermal_offset_tmp = (thermal_offset[i] & 0xff) |
					(thermal_offset[i + 1] & 0xff) << 8 |
					(thermal_offset[i + 2] & 0xff) << 16 |
					(thermal_offset[i + 3] & 0xff) << 24;

			halrf_wreg(rf, (0x5c00 + i), MASKDWORD, thermal_offset_tmp);
			
			RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			       "[TSSI] write addr:0x%x value=0x%08x\n",
			       (0x5c00 + i), thermal_offset_tmp);
		}

		halrf_wreg(rf, 0x5864, BIT(26), 0x1);
		halrf_wreg(rf, 0x5864, BIT(26), 0x0);

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s TSSI Calibration Mode return !!!\n", __func__);

		return;
	}

	if (channel >= 1 && channel <= 14) {
		hal_mem_cpy(hal, thermal_up_a, pwr_trk->delta_swing_table_idx_2ga_p, sizeof(thermal_up_a));
		hal_mem_cpy(hal, thermal_down_a, pwr_trk->delta_swing_table_idx_2ga_n, sizeof(thermal_down_a));
	} else if (channel >= 36 && channel <= 64) {
		hal_mem_cpy(hal, thermal_up_a, pwr_trk->delta_swing_table_idx_5ga_p[0], sizeof(thermal_up_a));
		hal_mem_cpy(hal, thermal_down_a, pwr_trk->delta_swing_table_idx_5ga_n[0], sizeof(thermal_down_a));
	} else if (channel >= 100 && channel <= 144) {
		hal_mem_cpy(hal, thermal_up_a, pwr_trk->delta_swing_table_idx_5ga_p[1], sizeof(thermal_up_a));
		hal_mem_cpy(hal, thermal_down_a, pwr_trk->delta_swing_table_idx_5ga_n[1], sizeof(thermal_down_a));
	} else if (channel >= 149 && channel <= 177) {
		hal_mem_cpy(hal, thermal_up_a, pwr_trk->delta_swing_table_idx_5ga_p[2], sizeof(thermal_up_a));
		hal_mem_cpy(hal, thermal_down_a, pwr_trk->delta_swing_table_idx_5ga_n[2], sizeof(thermal_down_a));
	}

	/*path s0*/
	hal_mem_set(hal, thermal_offset, 0, sizeof(thermal_offset));
	halrf_efuse_get_info(rf, EFUSE_INFO_RF_THERMAL_A, &thermal, 1);

	/*thermal = 32;*/

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
	       "[TSSI] channel=%d thermal_pahtA=0x%x tssi_info->tssi_type[HW_PHY_0]=%d\n",
	       channel, thermal, tssi_info->tssi_type[HW_PHY_0]);

	halrf_wreg(rf, 0x5810, 0x00010000, 0x0);
	halrf_wreg(rf, 0x5810, 0x01000000, 0x1);

	if (thermal == 0xff) {
		halrf_wreg(rf, 0x5810, 0x0000fc00, 32);
		halrf_wreg(rf, 0x5864, 0x03f00000, 32);

		for (i = 0; i < 64; i = i + 4) {
			thermal_offset_tmp = (thermal_offset[i] & 0xff) |
					(thermal_offset[i + 1] & 0xff) << 8 |
					(thermal_offset[i + 2] & 0xff) << 16 |
					(thermal_offset[i + 3] & 0xff) << 24;

			halrf_wreg(rf, (0x5c00 + i), MASKDWORD, thermal_offset_tmp);

			RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			       "[TSSI] write addr:0x%x value=0x%08x\n",
			       (0x5c00 + i), thermal_offset_tmp);
		}
	}

	tssi_info->thermal[RF_PATH_A] = thermal;

	if (thermal != 0xff) {
		halrf_wreg(rf, 0x5810, 0x0000fc00, (thermal & 0x3f));
		halrf_wreg(rf, 0x5864, 0x03f00000, (thermal & 0x3f));

		i = 0;
		for (j = 0; j < 32; j++) {
			if (i < DELTA_SWINGIDX_SIZE)
				thermal_offset[j] = -1 * thermal_down_a[i++];
			else
				thermal_offset[j] = -1 * thermal_down_a[DELTA_SWINGIDX_SIZE - 1];
		}

		i = 1;
		for (j = 63; j >= 32; j--) {
			if (i < DELTA_SWINGIDX_SIZE)
				thermal_offset[j] = thermal_up_a[i++];
			else
				thermal_offset[j] = thermal_up_a[DELTA_SWINGIDX_SIZE - 1];
		}

		for (i = 0; i < 64; i = i + 4) {
			RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			       "[TSSI] thermal_offset[%.2d]=%.2x %.2x %.2x %.2x\n",
			       i, thermal_offset[i + 3] & 0xff, thermal_offset[i + 2] & 0xff,
			       thermal_offset[i + 1] & 0xff, thermal_offset[i] & 0xff);
		}

		for (i = 0; i < 64; i = i + 4) {
			thermal_offset_tmp = (thermal_offset[i] & 0xff) |
					(thermal_offset[i + 1] & 0xff) << 8 |
					(thermal_offset[i + 2] & 0xff) << 16 |
					(thermal_offset[i + 3] & 0xff) << 24;

			halrf_wreg(rf, (0x5c00 + i), MASKDWORD, thermal_offset_tmp);

			RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			       "[TSSI] write addr:0x%x value=0x%08x\n",
			       (0x5c00 + i), thermal_offset_tmp);
		}
	}
	halrf_wreg(rf, 0x5864, BIT(26), 0x1);
	halrf_wreg(rf, 0x5864, BIT(26), 0x0);
}

void _halrf_tssi_set_tmeter_tbl_zere_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	u8 i;
	u32 thermal_offset_tmp = 0;
	u32 ftable_reg[TSSI_PATH_MAX_8851B] = {0x5c00};
	u32 ftable_base_reg[TSSI_PATH_MAX_8851B] = {0x5810};
	u32 ftable_trigger_reg[TSSI_PATH_MAX_8851B] = {0x5864};
	s8 thermal_offset[64] = {0};

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	halrf_wreg(rf, ftable_base_reg[path], 0x00010000, 0x0);
	halrf_wreg(rf, ftable_base_reg[path], 0x01000000, 0x1);

	halrf_wreg(rf, ftable_base_reg[path], 0x0000fc00, 32);
	halrf_wreg(rf, ftable_trigger_reg[path], 0x03f00000, 32);

	for (i = 0; i < 64; i = i + 4) {
		thermal_offset_tmp = (thermal_offset[i] & 0xff) |
				(thermal_offset[i + 1] & 0xff) << 8 |
				(thermal_offset[i + 2] & 0xff) << 16 |
				(thermal_offset[i + 3] & 0xff) << 24;

		halrf_wreg(rf, (ftable_reg[path] + i), MASKDWORD, thermal_offset_tmp);
		
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		       "[TSSI] write addr:0x%x value=0x%08x\n",
		       (ftable_reg[path] + i), thermal_offset_tmp);
	}

	halrf_wreg(rf, ftable_trigger_reg[path], BIT(26), 0x1);
	halrf_wreg(rf, ftable_trigger_reg[path], BIT(26), 0x0);

}

void _halrf_tssi_set_dac_gain_tbl_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	halrf_wreg(rf, 0x58b0, 0x00000fff, 0x000);
	halrf_wreg(rf, 0x5a00, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a04, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a08, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a0c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a10, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a14, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a18, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a1c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a20, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a24, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a28, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a2c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a30, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a34, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a38, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a3c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a40, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a44, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a48, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a4c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a50, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a54, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a58, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a5c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a60, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a64, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a68, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a6c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a70, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a74, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a78, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a7c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a80, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a84, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a88, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a8c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a90, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a94, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a98, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5a9c, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5aa0, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5aa4, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5aa8, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5aac, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5ab0, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5ab4, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5ab8, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5abc, MASKDWORD, 0x00000000);
	halrf_wreg(rf, 0x5ac0, MASKDWORD, 0x00000000);
}

void _halrf_tssi_slope_cal_org_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   channel=%d   path=%d\n",
		__func__, channel, path);

	if (channel >= 1 && channel <= 14) {
		halrf_wreg(rf, 0x5608, 0x07ffffff, 0x0201008);
		halrf_wreg(rf, 0x560c, 0x07ffffff, 0x0201008);
		halrf_wreg(rf, 0x5610, 0x07ffffff, 0x0200e08);
		halrf_wreg(rf, 0x5614, 0x07ffffff, 0x0201008);
		halrf_wreg(rf, 0x5618, 0x07ffffff, 0x0201008);
		halrf_wreg(rf, 0x561c, 0x000001ff, 0x007);
		halrf_wreg(rf, 0x561c, 0xffff0000, 0x0808);
		halrf_wreg(rf, 0x5620, 0xffffffff, 0x08080808);
		halrf_wreg(rf, 0x5624, 0xffffffff, 0x08080808);
		halrf_wreg(rf, 0x5628, 0xffffffff, 0x08080808);
		halrf_wreg(rf, 0x562c, 0x0000ffff, 0x0808);
		halrf_wreg(rf, 0x581c, 0x00100000, 0x1);
	} else {
		halrf_wreg(rf, 0x5608, 0x07ffffff, 0x0201008);
		halrf_wreg(rf, 0x560c, 0x07ffffff, 0x0341a08);
		halrf_wreg(rf, 0x5610, 0x07ffffff, 0x0201417);
		halrf_wreg(rf, 0x5614, 0x07ffffff, 0x0201008);
		halrf_wreg(rf, 0x5618, 0x07ffffff, 0x0201008);
		halrf_wreg(rf, 0x561c, 0x000001ff, 0x008);
		halrf_wreg(rf, 0x561c, 0xffff0000, 0x0808);
		halrf_wreg(rf, 0x5620, 0xffffffff, 0x0e0e0808);
		halrf_wreg(rf, 0x5624, 0xffffffff, 0x08080d18);
		halrf_wreg(rf, 0x5628, 0xffffffff, 0x08080808);
		halrf_wreg(rf, 0x562c, 0x0000ffff, 0x0808);
		halrf_wreg(rf, 0x581c, 0x00100000, 0x1);
	}
}


void _halrf_tssi_slope_cal_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;

	s16 power_2g_high[4] = {80, 24, 0, -24};
	s16 power_2g_low[4] = {56, 8, -16, -36};
	s16 power_5g_high[4] = {80, 24, 0, -36};
	s16 power_5g_low[4] = {56, 8, -16, -44};
	s16 power_high[4] = {0}, power_low[4] = {0};
	u32 tssi_cw_rpt_high = 0, tssi_cw_rpt_low = 0,
		tssi_cw_rpt_offset[4] = {0};

	u32 tssi_trigger[TSSI_PATH_MAX_8851B] = {0x5820};
	u32 tssi_cw_rpt_addr[TSSI_PATH_MAX_8851B] = {0x1c18};

	u8 i, j, k;
	u32 rate = HT_MF_FMT;
	u8 bw = 0;
	u8 phy_map;

	phy_map = (BIT(phy) << 4) | BIT(path);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   channel=%d   path=%d\n",
		__func__, channel, path);

	if (channel >= 1 && channel <= 14) {
		for (i = 0; i < 4; i++) {
			power_high[i] = power_2g_high[i];
			power_low[i] = power_2g_low[i];
		}
	} else {
		for (i = 0; i < 4; i++) {
			power_high[i] = power_5g_high[i];
			power_low[i] = power_5g_low[i];
		}
	}
	if(!rf->is_chl_rfk) {
		halrf_btc_rfk_ntfy(rf, phy_map, RF_BTC_TSSI, RFK_ONESHOT_START);
	}
	for (j = 0; j < 4; j++) {
		/*high power*/
		halrf_wreg(rf, tssi_trigger[path], 0x80000000, 0x0);
		halrf_wreg(rf, tssi_trigger[path], 0x80000000, 0x1);

		_halrf_tssi_hw_tx_8851b(rf, phy, path, 100, 5000, power_high[j], rate, bw, true);
		
		for (k = 0; halrf_rreg(rf, tssi_cw_rpt_addr[path], BIT(16)) == 0; k++) {
			halrf_delay_us(rf, 1);

			if (k > 100) {
				//halrf_set_pseudo_cw(rf, i, power_high[j], false);
				RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI PA K] TSSI finish bit k > 100us path=%d\n",
					i);
				_halrf_tssi_hw_tx_8851b(rf, phy, path, 100, 5000, power_high[j], rate, bw, false);
				return;
			}
		}
		_halrf_tssi_hw_tx_8851b(rf, phy, path, 100, 5000, power_high[j], rate, bw, false);
		tssi_cw_rpt_high = halrf_rreg(rf, tssi_cw_rpt_addr[path], 0x000001ff);

		/*low power*/
		halrf_wreg(rf, tssi_trigger[path], 0x80000000, 0x0);
		halrf_wreg(rf, tssi_trigger[path], 0x80000000, 0x1);

		_halrf_tssi_hw_tx_8851b(rf, phy, path, 100, 5000, power_low[j], rate, bw, true);
		
		for (k = 0; halrf_rreg(rf, tssi_cw_rpt_addr[path], BIT(16)) == 0; k++) {
			halrf_delay_us(rf, 1);

			if (k > 100) {
				//halrf_set_pseudo_cw(rf, i, power_low[j], false);
				RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI PA K] TSSI finish bit k > 100us path=%d\n",
					i);
				_halrf_tssi_hw_tx_8851b(rf, phy, path, 100, 5000, power_high[j], rate, bw, false);
				return;
			}
		}
		_halrf_tssi_hw_tx_8851b(rf, phy, path, 100, 5000, power_high[j], rate, bw, false);
		tssi_cw_rpt_low = halrf_rreg(rf, tssi_cw_rpt_addr[path], 0x000001ff);

		tssi_cw_rpt_offset[j] = tssi_cw_rpt_high - tssi_cw_rpt_low;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"[TSSI PA K] power_high[%d]=%d  power_low[%d]=%d\n", 
			j, power_high[j], j, power_low[j]);

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"[TSSI PA K] tssi_cw_rpt_offset[%d](0x%x) = tssi_cw_rpt_high(0x%x) - tssi_cw_rpt_low(0x%x)\n", 
			j, tssi_cw_rpt_offset[j], tssi_cw_rpt_high, tssi_cw_rpt_low);

		//halrf_set_pseudo_cw(rf, i, power_low[j], false);

	}

	halrf_wreg(rf, 0x581c, 0x00100000, 0x1);
	halrf_wreg(rf, 0x58cc, 0x00001000, 0x1);
	halrf_wreg(rf, 0x58cc, 0x00000007, 0x7);
	halrf_wreg(rf, 0x58cc, 0x00000038, 0x6);
	halrf_wreg(rf, 0x58cc, 0x000001c0, 0x3);
	halrf_wreg(rf, 0x58cc, 0x00000e00, 0x1);

	halrf_wreg(rf, 0x5828, 0x7fc00000, tssi_cw_rpt_offset[0]);
	halrf_wreg(rf, 0x5898, 0x000000ff, power_high[0] - power_low[0]);

	halrf_wreg(rf, 0x5830, 0x7fc00000, tssi_cw_rpt_offset[0]);
	halrf_wreg(rf, 0x5898, 0x0000ff00, power_high[0] - power_low[0]);

	halrf_wreg(rf, 0x5838, 0x7fc00000, tssi_cw_rpt_offset[1]);
	halrf_wreg(rf, 0x5898, 0x00ff0000, power_high[1] - power_low[1]);

	halrf_wreg(rf, 0x5840, 0x7fc00000, tssi_cw_rpt_offset[1]);
	halrf_wreg(rf, 0x5898, 0xff000000, power_high[1] - power_low[1]);

	halrf_wreg(rf, 0x5848, 0x7fc00000, tssi_cw_rpt_offset[2]);
	halrf_wreg(rf, 0x589c, 0x000000ff, power_high[2] - power_low[2]);

	halrf_wreg(rf, 0x5850, 0x7fc00000, tssi_cw_rpt_offset[2]);
	halrf_wreg(rf, 0x589c, 0x0000ff00, power_high[2] - power_low[2]);

	halrf_wreg(rf, 0x5858, 0x7fc00000, tssi_cw_rpt_offset[3]);
	halrf_wreg(rf, 0x589c, 0x00ff0000, power_high[3] - power_low[3]);

	halrf_wreg(rf, 0x5860, 0x7fc00000, tssi_cw_rpt_offset[3]);
	halrf_wreg(rf, 0x589c, 0xff000000, power_high[3] - power_low[3]);
	if(!rf->is_chl_rfk) {
		halrf_btc_rfk_ntfy(rf, phy_map, RF_BTC_TSSI, RFK_ONESHOT_STOP);
	}
}

void _halrf_tssi_alignment_default_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path, bool all)
{
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   channel=%d   path=%d   all=%d\n",
		__func__, channel, path, all);

	if (channel >= 1 && channel <= 14) {
		halrf_wreg(rf, 0x5604, 0x80000000, 0x1);
		halrf_wreg(rf, 0x5600, 0x3fffffff, 0x000000);
		halrf_wreg(rf, 0x5604, 0x003fffff, 0x2d2400);
		halrf_wreg(rf, 0x5630, 0x3fffffff, 0x00000000);
		halrf_wreg(rf, 0x5634, 0x000003ff, 0x000);
		halrf_wreg(rf, 0x5634, 0x000ffc00, 0x000);
		halrf_wreg(rf, 0x5634, 0x3ff00000, 0x3fa);
		halrf_wreg(rf, 0x5638, 0x000003ff, 0x02e);
		halrf_wreg(rf, 0x5638, 0x000ffc00, 0x09c);
		halrf_wreg(rf, 0x563c, 0x3fffffff, 0x00000000);
		halrf_wreg(rf, 0x5640, 0x3fffffff, 0x3fb00000);
		halrf_wreg(rf, 0x5644, 0x000003ff, 0x02f);
		halrf_wreg(rf, 0x5644, 0x000ffc00, 0x09c);
	} else {
		halrf_wreg(rf, 0x5604, 0x80000000, 0x1);
		halrf_wreg(rf, 0x5600, 0x3fffffff, 0x000000);
		halrf_wreg(rf, 0x5604, 0x003fffff, 0x3b2d24);
		halrf_wreg(rf, 0x5630, 0x3fffffff, 0x00000000);
		halrf_wreg(rf, 0x5634, 0x000003ff, 0x000);
		halrf_wreg(rf, 0x5634, 0x000ffc00, 0x3cb);
		halrf_wreg(rf, 0x5634, 0x3ff00000, 0x030);
		halrf_wreg(rf, 0x5638, 0x000003ff, 0x73);
		halrf_wreg(rf, 0x5638, 0x000ffc00, 0xd4);
		halrf_wreg(rf, 0x563c, 0x3fffffff, 0x00000000);
		halrf_wreg(rf, 0x5640, 0x3fffffff, 0x00000000);
		halrf_wreg(rf, 0x5644, 0x000fffff, 0x00000);
	}

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"======>%s\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n",
		__func__,
		0x5600, halrf_rreg(rf, 0x5600, 0xffffffff),
		0x5604, halrf_rreg(rf, 0x5604, 0xffffffff),
		0x5630, halrf_rreg(rf, 0x5630, 0xffffffff),
		0x5634, halrf_rreg(rf, 0x5634, 0xffffffff),
		0x5638, halrf_rreg(rf, 0x5638, 0xffffffff),
		0x563c, halrf_rreg(rf, 0x563c, 0xffffffff),
		0x5640, halrf_rreg(rf, 0x5640, 0xffffffff),
		0x5644, halrf_rreg(rf, 0x5644, 0xffffffff));
}

void _halrf_tssi_run_slope_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	halrf_wreg(rf, 0x5820, 0x80000000, 0x0);
	halrf_wreg(rf, 0x5820, 0x80000000, 0x1);
}

void _halrf_tssi_set_slope_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	halrf_wreg(rf, 0x5820, 0x80000000, 0x0);
	halrf_wreg(rf, 0x5818, 0x10000000, 0x0);
	halrf_wreg(rf, 0x5814, 0x00000800, 0x1);
	halrf_wreg(rf, 0x581c, 0x20000000, 0x1);
	halrf_wreg(rf, 0x5820, 0x0000f000, 0xf);
	halrf_wreg(rf, 0x581c, 0x000003ff, 0x280);
	halrf_wreg(rf, 0x581c, 0x000ffc00, 0x200);
	halrf_wreg(rf, 0x58b8, 0x007f0000, 0x00);
	halrf_wreg(rf, 0x58b8, 0x7f000000, 0x00);
	halrf_wreg(rf, 0x58b4, 0x7f000000, 0x0a);
	halrf_wreg(rf, 0x58b8, 0x0000007f, 0x28);
	halrf_wreg(rf, 0x58b8, 0x00007f00, 0x76);
	halrf_wreg(rf, 0x5810, 0x20000000, 0x0);
	halrf_wreg(rf, 0x580c, 0x10000000, 0x1);
	halrf_wreg(rf, 0x580c, 0x40000000, 0x1);
	halrf_wreg(rf, 0x5834, 0x0003ffff, 0x115f2);
	halrf_wreg(rf, 0x5834, 0x3ffc0000, 0x000);
	halrf_wreg(rf, 0x5838, 0x00000fff, 0x121);
	halrf_wreg(rf, 0x5838, 0x003ff000, 0x000);
	halrf_wreg(rf, 0x5854, 0x0003ffff, 0x115f2);
	halrf_wreg(rf, 0x5854, 0x3ffc0000, 0x000);
	halrf_wreg(rf, 0x5858, 0x00000fff, 0x121);
	halrf_wreg(rf, 0x5858, 0x003ff000, 0x000);
	halrf_wreg(rf, 0x5824, 0x0003ffff, 0x115f2);
	halrf_wreg(rf, 0x5824, 0x3ffc0000, 0x000);
	halrf_wreg(rf, 0x5828, 0x00000fff, 0x121);
	halrf_wreg(rf, 0x5828, 0x003ff000, 0x000);
	halrf_wreg(rf, 0x582c, 0x0003ffff, 0x115f2);
	halrf_wreg(rf, 0x582c, 0x3ffc0000, 0x000);
	halrf_wreg(rf, 0x5830, 0x00000fff, 0x121);
	halrf_wreg(rf, 0x5830, 0x003ff000, 0x000);
	halrf_wreg(rf, 0x583c, 0x0003ffff, 0x115f2);
	halrf_wreg(rf, 0x583c, 0x3ffc0000, 0x000);
	halrf_wreg(rf, 0x5840, 0x00000fff, 0x121);
	halrf_wreg(rf, 0x5840, 0x003ff000, 0x000);
	halrf_wreg(rf, 0x5844, 0x0003ffff, 0x115f2);
	halrf_wreg(rf, 0x5844, 0x3ffc0000, 0x000);
	halrf_wreg(rf, 0x5848, 0x00000fff, 0x121);
	halrf_wreg(rf, 0x5848, 0x003ff000, 0x000);
	halrf_wreg(rf, 0x584c, 0x0003ffff, 0x115f2);
	halrf_wreg(rf, 0x584c, 0x3ffc0000, 0x000);
	halrf_wreg(rf, 0x5850, 0x00000fff, 0x121);
	halrf_wreg(rf, 0x5850, 0x003ff000, 0x000);
	halrf_wreg(rf, 0x585c, 0x0003ffff, 0x115f2);
	halrf_wreg(rf, 0x585c, 0x3ffc0000, 0x000);
	halrf_wreg(rf, 0x5860, 0x00000fff, 0x121);
	halrf_wreg(rf, 0x5860, 0x003ff000, 0x000);
}

void _halrf_tssi_set_tssi_track_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);
	
	halrf_wreg(rf, 0x5820, 0x80000000, 0x0);
	halrf_wreg(rf, 0x5818, 0x10000000, 0x0);
	halrf_wreg(rf, 0x5814, 0x00000800, 0x0);
	halrf_wreg(rf, 0x581c, 0x20000000, 0x1);
	halrf_wreg(rf, 0x5864, 0x000003ff, 0x1ff);
	halrf_wreg(rf, 0x5864, 0x000ffc00, 0x200);
	halrf_wreg(rf, 0x5820, 0x00000fff, 0x080);
	halrf_wreg(rf, 0x5814, 0x01000000, 0x0);
}

void _halrf_tssi_set_txagc_offset_mv_avg_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n", __func__, path);

	halrf_wreg(rf, 0x58e4, 0x00003800, 0x1);
	halrf_wreg(rf, 0x58e4, 0x00004000, 0x0);
	halrf_wreg(rf, 0x58e4, 0x00008000, 0x1);
	halrf_wreg(rf, 0x58e4, 0x000f0000, 0x0);
}

u32 _halrf_tssi_get_cck_efuse_group_8851b(struct rf_info *rf,
						enum phl_phy_idx phy)
{
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;
	u32 offset_index = 0;

	if (channel >= 1 && channel <= 2)
		offset_index = 0;
	else if (channel >= 3 && channel <= 5)
		offset_index = 1;
	else if (channel >= 6 && channel <= 8)
		offset_index = 2;
	else if (channel >= 9 && channel <= 11)
		offset_index = 3;
	else if (channel >= 12 && channel <= 13)
		offset_index = 4;
	else if (channel == 14)
		offset_index = 5;

	return offset_index;
}

u32 _halrf_tssi_get_ofdm_efuse_group_8851b(struct rf_info *rf,
						enum phl_phy_idx phy)
{
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;
	u32 offset_index = 0;

	if (channel >= 1 && channel <= 2)
		offset_index = 0;
	else if (channel >= 3 && channel <= 5)
		offset_index = 1;
	else if (channel >= 6 && channel <= 8)
		offset_index = 2;
	else if (channel >= 9 && channel <= 11)
		offset_index = 3;
	else if (channel >= 12 && channel <= 14)
		offset_index = 4;
	else if (channel >= 36 && channel <= 40)
		offset_index = 5;
	else if (channel >= 44 && channel <= 48)
		offset_index = 6;
	else if (channel >= 52 && channel <= 56)
		offset_index = 7;
	else if (channel >= 60 && channel <= 64)
		offset_index = 8;
	else if (channel >= 100 && channel <= 104)
		offset_index = 9;
	else if (channel >= 108 && channel <= 112)
		offset_index = 10;
	else if (channel >= 116 && channel <= 120)
		offset_index = 11;
	else if (channel >= 124 && channel <= 128)
		offset_index = 12;
	else if (channel >= 132 && channel <= 136)
		offset_index = 13;
	else if (channel >= 140 && channel <= 144)
		offset_index = 14;
	else if (channel >= 149 && channel <= 153)
		offset_index = 15;
	else if (channel >= 157 && channel <= 161)
		offset_index = 16;
	else if (channel >= 165 && channel <= 169)
		offset_index = 17;
	else if (channel >= 173 && channel <= 177)
		offset_index = 18;
	else if (channel > 40 && channel < 44)
		offset_index = 0x0506;
	else if (channel > 48  && channel < 52)
		offset_index = 0x0607;
	else if (channel > 56 && channel < 60)
		offset_index = 0x0708;
	else if (channel > 104 && channel < 108)
		offset_index = 0x090a;
	else if (channel > 112 && channel < 116)
		offset_index = 0x0a0b;
	else if (channel > 120 && channel < 124)
		offset_index = 0x0b0c;
	else if (channel > 128 && channel < 132)
		offset_index = 0x0c0d;
	else if (channel > 136 && channel < 140)
		offset_index = 0x0d0e;
	else if (channel > 153 && channel < 157)
		offset_index = 0x0f10;
	else if (channel > 161 && channel < 165)
		offset_index = 0x1011;
	else if (channel > 169 && channel < 173)
		offset_index = 0x1112;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI efuse] ======> %s offset_index=0x%x(%d) channel=%d\n",
			__func__, offset_index, offset_index, channel);

	return offset_index;
}

s8 _halrf_tssi_get_ofdm_efuse_tssi_de_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u32 group_idx;
	s8 first_de = 0, second_de = 0, final_de;

	group_idx = _halrf_tssi_get_ofdm_efuse_group_8851b(rf, phy);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   group_idx=0x%x(%d)\n",
		__func__, group_idx, group_idx);

	if (group_idx >= 0x0506) {
		first_de = tssi_info->tssi_efuse[path][EFUSE_TSSI_MCS][group_idx >> 8];

		second_de = tssi_info->tssi_efuse[path][EFUSE_TSSI_MCS][group_idx & 0xff];

		final_de = (first_de + second_de) / 2;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI efuse] first_de=%d second_de=%d final_de=%d\n",
			first_de, second_de, final_de);
	} else {
		final_de = tssi_info->tssi_efuse[path][EFUSE_TSSI_MCS][group_idx];

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI efuse] final_de=%d\n",
			final_de);
	}

	return final_de;
}


u32 _halrf_tssi_get_tssi_trim_efuse_group_8851b(struct rf_info *rf,
						enum phl_phy_idx phy)
{
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;
	u32 group_index = 0;

	if (channel >= 1 && channel <= 8)
		group_index = 0;
	else if (channel >= 9 && channel <= 14)
		group_index = 1;
	else if (channel >= 36 && channel <= 48)
		group_index = 2;
	else if (channel >= 52 && channel <= 64)
		group_index = 3;
	else if (channel >= 100 && channel <= 112)
		group_index = 4;
	else if (channel >= 116 && channel <= 128)
		group_index = 5;
	else if (channel >= 132 && channel <= 144)
		group_index = 6;
	else if (channel >= 149 && channel <= 177)
		group_index = 7;
#if 0
	else if (channel > 48 && channel < 52)
		group_index = 0x0203;
	else if (channel > 112 && channel < 116)
		group_index = 0x0405;
#endif

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI Trim] ======> %s offset_index=0x%x(%d) channel=%d\n",
			__func__, group_index, group_index, channel);

	return group_index;
}

s8 _halrf_tssi_get_ofdm_tssi_trim_de_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u32 group_idx;
	s8 first_de = 0, second_de = 0, final_de;

	group_idx = _halrf_tssi_get_tssi_trim_efuse_group_8851b(rf, phy);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI Trim] ======> %s   group_idx=0x%x(%d)\n",
		__func__, group_idx, group_idx);

	if (group_idx >= 0x0203) {
		first_de = tssi_info->tssi_trim[path][group_idx >> 8];

		second_de = tssi_info->tssi_trim[path][group_idx & 0xff];

		final_de = (first_de + second_de) / 2;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI Trim] Trim_first_de=%d Trim_second_de=%d Trim_final_de=%d\n",
			first_de, second_de, final_de);
	} else {
		final_de = tssi_info->tssi_trim[path][group_idx];

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI Trim] Trim_final_de=%d\n",
			final_de);
	}

	return final_de;
}

void _halrf_tssi_alimentk_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;
	u8 ch_idx = _halrf_ch_to_idx_8851b(rf, channel);
	s16 power_2g[4] = {48, 20, 4, 4};
	s16 power_5g[4] = {48, 20, 4, 4};
	s16 power[4] = {0};
	s32 tssi_alim_offset[4] = {0};
	s32 aliment_diff, tssi_cw_default;
	u32 tssi_cw_rpt[4] = {0}, tx_couter, tx_couter_tmp, tmp, retry;

	u32 tssi_trigger[TSSI_PATH_MAX_8851B] = {0x5820};
	u32 tssi_cw_rpt_addr[TSSI_PATH_MAX_8851B] = {0x1c18};

	u32 tssi_cw_default_addr[TSSI_PATH_MAX_8851B][4] =
					{{0x5634, 0x5630, 0x5630, 0x5630}};
	
	u32 tssi_cw_default_mask[4] =
			{0x000003ff, 0x3ff00000, 0x000ffc00, 0x000003ff};

	u32 bb_reg[6] = {0x5820, 0x4978, 0x58e4, 0x49c0, 0x0d18,
			0x0d80};
	u32 bb_reg_backup[6] = {0};
	u32 backup_num = 6;

	u8 i, j, k;
	u32 rate = HT_MF_FMT;
	u8 bw = 0, band;
	u8 phy_map;

	u32 start_time, finish_time;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   channel=%d   path=%d\n",
		__func__, channel, path);

	if (tssi_info->check_backup_aligmk[path][ch_idx] == true) {
		halrf_wreg(rf, 0x5630 + (path << 13), 0xffffffff, tssi_info->alignment_backup_by_ch[path][ch_idx][0]);
		halrf_wreg(rf, 0x5634 + (path << 13), 0xffffffff, tssi_info->alignment_backup_by_ch[path][ch_idx][1]);
		halrf_wreg(rf, 0x563c + (path << 13), 0xffffffff, tssi_info->alignment_backup_by_ch[path][ch_idx][2]);
		halrf_wreg(rf, 0x5640 + (path << 13), 0xffffffff, tssi_info->alignment_backup_by_ch[path][ch_idx][3]);

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   Reload TSSI Alignment !!!\n", __func__);

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"[TSSI PA K]\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n",
			0x5600 + (path << 13), halrf_rreg(rf, 0x5600 + (path << 13), 0xffffffff),
			0x5604 + (path << 13), halrf_rreg(rf, 0x5604 + (path << 13), 0xffffffff),
			0x5630 + (path << 13), halrf_rreg(rf, 0x5630 + (path << 13), 0xffffffff),
			0x5634 + (path << 13), halrf_rreg(rf, 0x5634 + (path << 13), 0xffffffff),
			0x5638 + (path << 13), halrf_rreg(rf, 0x5638 + (path << 13), 0xffffffff),
			0x563c + (path << 13), halrf_rreg(rf, 0x563c + (path << 13), 0xffffffff),
			0x5640 + (path << 13), halrf_rreg(rf, 0x5640 + (path << 13), 0xffffffff),
			0x5644 + (path << 13), halrf_rreg(rf, 0x5644 + (path << 13), 0xffffffff));
		return;
	}

	phy_map = (BIT(phy) << 4) | BIT(path);

	start_time = _os_get_cur_time_us();

	halrf_hal_bb_backup_info(rf, phy);

	if (channel >= 1 && channel <= 14) {
		for (i = 0; i < 4; i++) {
			power[i] = power_2g[i];
		}
	} else {
		for (i = 0; i < 4; i++) {
			power[i] = power_5g[i];
		}
	}

	if (channel >= 1 && channel <= 14)
		band = TSSI_ALIMK_2G;
	else if (channel >= 36 && channel <= 64)
		band = TSSI_ALIMK_5GL;
	else if (channel >= 100 && channel <= 144)
		band = TSSI_ALIMK_5GM;
	else if (channel >= 149 && channel <= 177)
		band = TSSI_ALIMK_5GH;
	else
		band = TSSI_ALIMK_2G;

	_tssi_backup_bb_registers_8851b(rf, phy, bb_reg, bb_reg_backup,
					backup_num);

	halrf_wreg(rf, 0x5820, 0x0000f000, 0x8);

	halrf_wreg(rf, 0x58e4, 0x00003800, 0x2);

	//halrf_btc_rfk_ntfy(rf, phy_map, RF_BTC_TSSI, RFK_START);
	for (j = 0; j < 2; j++) {
		halrf_wreg(rf, tssi_trigger[path], 0x80000000, 0x0);
		halrf_wreg(rf, tssi_trigger[path], 0x80000000, 0x1);

		tx_couter = halrf_rreg(rf, 0x1a40, 0xffff);

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"[TSSI PA K] 0x%x = 0x%08x   path=%d\n",
			tssi_trigger[path],
			halrf_rreg(rf, tssi_trigger[path], 0xffffffff), path);

		_halrf_tssi_hw_tx_8851b(rf, phy, path, 100, 5000, power[j], rate, bw, true);

		//halrf_delay_ms(rf, 10);

		tx_couter_tmp = halrf_rreg(rf, 0x1a40, 0xffff) - tx_couter;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI PA K] First HWTXcounter=%d path=%d\n",
			tx_couter_tmp, path);

		for (k = 0; halrf_rreg(rf, tssi_cw_rpt_addr[path], BIT(16)) == 0; k++) {
			if (phl_is_mp_mode(rf->phl_com)) {
				halrf_delay_ms(rf, 1);
				retry = 30;
			} else {
				halrf_delay_us(rf, 30);
				retry = 50;
			}

			tx_couter_tmp = halrf_rreg(rf, 0x1a40, 0xffff) - tx_couter;

			RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI PA K] Flow k = %d HWTXcounter=%d path=%d\n",
					k, tx_couter_tmp, path);

			if (k > retry) {
				RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI PA K] TSSI finish bit k > %d mp:100ms normal:30us path=%d\n",
					k, path);
				
				_halrf_tssi_hw_tx_8851b(rf, phy, path, 100, 5000, power[j], rate, bw, false);

				_tssi_reload_bb_registers_8851b(rf, phy, bb_reg, bb_reg_backup,
					backup_num);

				halrf_hal_bb_restore_info(rf, phy);

				if (!phl_is_mp_mode(rf->phl_com))
					halrf_tx_mode_switch(rf, phy, 0);
				
				finish_time = _os_get_cur_time_us();
				tssi_info->tssi_alimk_time = tssi_info->tssi_alimk_time + (finish_time - start_time);
				return;
			}
		}

		tssi_cw_rpt[j] = halrf_rreg(rf, tssi_cw_rpt_addr[path], 0x000001ff);

		_halrf_tssi_hw_tx_8851b(rf, phy, path, 100, 5000, power[j], rate, bw, false);

		tx_couter = halrf_rreg(rf, 0x1a40, 0xffff) - tx_couter;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"[TSSI PA K] Fianl HWTXcounter = %d path=%d\n",
			tx_couter, path);

	}

	for (j = 0; j < 2; j++) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"[TSSI PA K] power[%d]=%d  tssi_cw_rpt[%d]=%d\n",
			j, power[j], j, tssi_cw_rpt[j]);
	}

	tmp = halrf_rreg(rf, tssi_cw_default_addr[path][1],
					tssi_cw_default_mask[1]);

	if (tmp & BIT(8))
		tssi_cw_default = tmp | 0xfffffe00;
	else
		tssi_cw_default = tmp;

	tssi_alim_offset[1] = tssi_cw_rpt[0] - ((power[0] - power[1]) * 2) -
		tssi_cw_rpt[1] + tssi_cw_default;

	aliment_diff = tssi_alim_offset[1] - tssi_cw_default;
		
	/*tssi_alim_offset[2]*/
	tmp = halrf_rreg(rf, tssi_cw_default_addr[path][2],
					tssi_cw_default_mask[2]);
	
	if (tmp & BIT(8))
		tssi_cw_default = tmp | 0xfffffe00;
	else
		tssi_cw_default = tmp;

	tssi_alim_offset[2] = tssi_cw_default + aliment_diff;

	/*tssi_alim_offset[3]*/
	tmp = halrf_rreg(rf, tssi_cw_default_addr[path][3],
					tssi_cw_default_mask[3]);
	
	if (tmp & BIT(8))
		tssi_cw_default = tmp | 0xfffffe00;
	else
		tssi_cw_default = tmp;

	tssi_alim_offset[3] = tssi_cw_default + aliment_diff;

	tmp = ((tssi_alim_offset[1] & 0x3ff) << 20) |
		((tssi_alim_offset[2] & 0x3ff) << 10) |
		(tssi_alim_offset[3] & 0x3ff);
	halrf_wreg(rf, 0x5630, 0x3fffffff, tmp);
	halrf_wreg(rf, 0x563c, 0x3fffffff, tmp);
	
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"[TSSI PA K] tssi_alim_offset = 0x%x   0x%x   0x%x   0x%x\n",
		halrf_rreg(rf, 0x5634, 0x000003ff),
		halrf_rreg(rf, 0x5630, 0x3ff00000),
		halrf_rreg(rf, 0x5630, 0x000ffc00),
		halrf_rreg(rf, 0x5630, 0x000003ff)
		);

	tssi_info->alignment_done[path][band] = true;
	tssi_info->alignment_value[path][band][0] = halrf_rreg(rf, 0x5630 + (path << 13), 0xffffffff);
	tssi_info->alignment_value[path][band][1] = halrf_rreg(rf, 0x5634 + (path << 13), 0xffffffff);
	tssi_info->alignment_value[path][band][2] = halrf_rreg(rf, 0x563c + (path << 13), 0xffffffff);
	tssi_info->alignment_value[path][band][3] = halrf_rreg(rf, 0x5640 + (path << 13), 0xffffffff);

	tssi_info->check_backup_aligmk[path][ch_idx] = true;
	tssi_info->alignment_backup_by_ch[path][ch_idx][0] = halrf_rreg(rf, 0x5630 + (path << 13), 0xffffffff);
	tssi_info->alignment_backup_by_ch[path][ch_idx][1] = halrf_rreg(rf, 0x5634 + (path << 13), 0xffffffff);
	tssi_info->alignment_backup_by_ch[path][ch_idx][2] = halrf_rreg(rf, 0x563c + (path << 13), 0xffffffff);
	tssi_info->alignment_backup_by_ch[path][ch_idx][3] = halrf_rreg(rf, 0x5640 + (path << 13), 0xffffffff);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"[TSSI PA K] tssi_info->alignment_value[path=%d][band=%d][0], 0x%x = 0x%08x\n",
		path, band, 0x5630 + (path << 13), tssi_info->alignment_value[path][band][0]);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"[TSSI PA K] tssi_info->alignment_value[path=%d][band=%d][1], 0x%x = 0x%08x\n",
		path, band, 0x5634 + (path << 13), tssi_info->alignment_value[path][band][1]);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"[TSSI PA K] tssi_info->alignment_value[path=%d][band=%d][2], 0x%x = 0x%08x\n",
		path, band, 0x563c + (path << 13), tssi_info->alignment_value[path][band][2]);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"[TSSI PA K] tssi_info->alignment_value[path=%d][band=%d][2], 0x%x = 0x%08x\n",
		path, band, 0x5640 + (path << 13), tssi_info->alignment_value[path][band][3]);


	_tssi_reload_bb_registers_8851b(rf, phy, bb_reg, bb_reg_backup,
						backup_num);

	halrf_hal_bb_restore_info(rf, phy);

	if (!phl_is_mp_mode(rf->phl_com))
		halrf_tx_mode_switch(rf, phy, 0);

	finish_time = _os_get_cur_time_us();
	tssi_info->tssi_alimk_time = tssi_info->tssi_alimk_time + (finish_time - start_time);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI PA K] %s processing time = %d.%dms\n",
		__func__,
		HALRF_ABS(finish_time, start_time) / 1000,
		HALRF_ABS(finish_time, start_time) % 1000);
}

void _halrf_tssi_alimentk_done_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, enum rf_path path)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;
	u8 band;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   phy=%d   path=%d\n", __func__, phy, path);

	if (channel >= 1 && channel <= 14)
		band = TSSI_ALIMK_2G;
	else if (channel >= 36 && channel <= 64)
		band = TSSI_ALIMK_5GL;
	else if (channel >= 100 && channel <= 144)
		band = TSSI_ALIMK_5GM;
	else if (channel >= 149 && channel <= 177)
		band = TSSI_ALIMK_5GH;
	else
		band = TSSI_ALIMK_2G;

	if (tssi_info->alignment_done[path][band] == true) {
		halrf_wreg(rf, 0x5630 + (path << 13), 0xffffffff, tssi_info->alignment_value[path][band][0]);
		halrf_wreg(rf, 0x5634 + (path << 13), 0xffffffff, tssi_info->alignment_value[path][band][1]);
		halrf_wreg(rf, 0x563c + (path << 13), 0xffffffff, tssi_info->alignment_value[path][band][2]);
		halrf_wreg(rf, 0x5640 + (path << 13), 0xffffffff, tssi_info->alignment_value[path][band][3]);
	}

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"[TSSI PA K]\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n0x%x = 0x%08x\n",
		0x5600 + (path << 13), halrf_rreg(rf, 0x5600 + (path << 13), 0xffffffff),
		0x5604 + (path << 13), halrf_rreg(rf, 0x5604 + (path << 13), 0xffffffff),
		0x5630 + (path << 13), halrf_rreg(rf, 0x5630 + (path << 13), 0xffffffff),
		0x5634 + (path << 13), halrf_rreg(rf, 0x5634 + (path << 13), 0xffffffff),
		0x5638 + (path << 13), halrf_rreg(rf, 0x5638 + (path << 13), 0xffffffff),
		0x563c + (path << 13), halrf_rreg(rf, 0x563c + (path << 13), 0xffffffff),
		0x5640 + (path << 13), halrf_rreg(rf, 0x5640 + (path << 13), 0xffffffff),
		0x5644 + (path << 13), halrf_rreg(rf, 0x5644 + (path << 13), 0xffffffff));
}

void halrf_tssi_backup_txagc_8851b(struct rf_info *rf, enum phl_phy_idx phy, bool enable)
{
#if 0
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;
	u8 bw = rf->hal_com->band[phy].cur_chandef.bw;
	s8 power, power_tmp;
	s16 xdbm;
	u32 i, j, tx_couter = 0;
	u8 ch_idx = _halrf_ch_to_idx_8851b(rf, channel);
	u8 channel_tmp;

	u32 bb_reg[5] = {0x5820, 0x58e4, 0x49c0, 0x0d18, 0x0d80};
	u32 bb_reg_backup[5] = {0};
	u32 backup_num = 5;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   phy=%d   enable=%d\n", __func__, phy, enable);

	if (phl_is_mp_mode(rf->phl_com)) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s   phy=%d   MP Mode return!!!\n",
			__func__, phy);
		return;
	}

	if (!(rf->support_ability & HAL_RF_TX_PWR_TRACK)) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s   phy=%d   rf->support_ability=%d return!!!\n",
			__func__, phy, rf->support_ability);
		return;
	}

	if (enable == false) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s   phy=%d   enable=%d  return!!!\n",
			__func__, phy, enable);
		return;
	}

	if (rf->is_tssi_mode[RF_PATH_A] != true) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s   phy=%d   rf->is_tssi_mode[A] != true  return!!!\n",
			__func__, phy);
		return;
	}

	if (tssi_info->check_backup_txagc[ch_idx] == true) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s check_backup_txagc[%d]==true Backup 0x1c60=0x%x 0x3c60=0x%x channel=%d   return!!!\n",
			__func__, ch_idx,
			tssi_info->backup_txagc_offset[RF_PATH_A][ch_idx],
			tssi_info->backup_txagc_offset[RF_PATH_B][ch_idx],
			channel);

		halrf_wreg(rf, 0x5818, 0x000000ff,
			tssi_info->backup_txagc_offset[RF_PATH_A][ch_idx]);

		halrf_wreg(rf, 0x5818 , 0x10000000, 0x0);
		halrf_wreg(rf, 0x5818 , 0x10000000, 0x1);

		return;	
	}

	halrf_hal_bb_backup_info(rf, phy);

	_tssi_backup_bb_registers_8851b(rf, phy, bb_reg, bb_reg_backup,
					backup_num);

	halrf_wreg(rf, 0x5820, 0x0000f000, 0x8);

	halrf_wreg(rf, 0x58e4, 0x00003800, 0x2);

	if (bw == CHANNEL_WIDTH_40)
		channel_tmp = channel - 2;
	else if (bw == CHANNEL_WIDTH_80)
		channel_tmp = channel - 6;
	else
		channel_tmp = channel;

	power = halrf_get_power_limit(rf, phy, RF_PATH_A,
		RTW_DATA_RATE_MCS0, CHANNEL_WIDTH_20,
		PW_LMT_NONBF, PW_LMT_PH_1T, channel_tmp);

	if (power > 17 * 4)
		power_tmp = 17 * 4;
	else
		power_tmp = power;

	xdbm = power_tmp * 100 / 4;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   phy=%d limit_table=%d  power_tmp=%d xdbm=%d\n",
		__func__, phy, power, power_tmp, xdbm);
	if(!rf->is_chl_rfk) {
		halrf_btc_rfk_ntfy(rf, (BIT(phy) << 4), RF_BTC_TSSI, RFK_START);
		halrf_tmac_tx_pause(rf, phy, true);
	}
	/*Path A*/
	tx_couter= halrf_rreg(rf, 0x1a40, 0xffff);

	_halrf_tssi_hw_tx_8851b(rf, phy, RF_PATH_AB, 1000, 100, power_tmp, HT_MF_FMT, 0, true);

#if 0
	/*Path B*/
	tx_couter[RF_PATH_B] = halrf_rreg(rf, 0x1a40, 0xffff);
	
	_halrf_tssi_hw_tx_8851b(rf, phy, RF_PATH_B, 1000, 100, xdbm, HT_MF_FMT, 0, true);
	halrf_delay_ms(rf, 15);
	_halrf_tssi_hw_tx_8851b(rf, phy, RF_PATH_B, 1000, 100, xdbm, HT_MF_FMT, 0, false);
#endif

	for (i = 0; i < 6; i++) {
		for (j = 0; j < 300; j++)
			halrf_delay_us(rf, 10);

		if (halrf_rreg(rf, 0x1c60, 0xff000000) != 0x0 &&
			halrf_rreg(rf, 0x3c60, 0xff000000) != 0x0) {
			tssi_info->backup_txagc_offset[RF_PATH_A][ch_idx] =
				halrf_rreg(rf, 0x1c60, 0xff000000);

			tssi_info->backup_txagc_offset[RF_PATH_B][ch_idx] =
				halrf_rreg(rf, 0x3c60, 0xff000000);

			tssi_info->check_backup_txagc[ch_idx] = true;
			break;
		}

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"Get   %d times   0x1c60=0x%x   0x3c60=0x%x\n",
			i,
			tssi_info->backup_txagc_offset[RF_PATH_A][ch_idx],
			tssi_info->backup_txagc_offset[RF_PATH_B][ch_idx]);
	}

	tssi_info->backup_txagc_oft_ther[RF_PATH_A][ch_idx] =
		halrf_get_thermal_8851b(rf, RF_PATH_A);

	tssi_info->backup_txagc_oft_ther[RF_PATH_B][ch_idx] =
		halrf_get_thermal_8851b(rf, RF_PATH_B);

	_halrf_tssi_hw_tx_8851b(rf, phy, RF_PATH_AB, 1000, 100, power_tmp, HT_MF_FMT, 0, false);

	tx_couter = halrf_rreg(rf, 0x1a40, 0xffff) - tx_couter;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,	"======>%s tx couter=%d\n",
		__func__, tx_couter);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"Backup TSSI TXAGC Offset   ch_idx=%d   0x1c60=0x%x   therA=0x%x   channel=%d\n",
		ch_idx,
		tssi_info->backup_txagc_offset[RF_PATH_A][ch_idx],
		tssi_info->backup_txagc_oft_ther[RF_PATH_A][ch_idx],
		channel);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"Backup TSSI TXAGC Offset   ch_idx=%d   0x3c60=0x%x   therB=0x%x   channel=%d\n",
		ch_idx,
		tssi_info->backup_txagc_offset[RF_PATH_B][ch_idx],
		tssi_info->backup_txagc_oft_ther[RF_PATH_B][ch_idx],
		channel);

	halrf_wreg(rf, 0x5818, 0x000000ff,
		tssi_info->backup_txagc_offset[RF_PATH_A][ch_idx]);

	halrf_wreg(rf, 0x5818 , 0x10000000, 0x0);
	halrf_wreg(rf, 0x5818 , 0x10000000, 0x1);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
		"======>%s Set 0x5818[7:0]=0x%x\n",
		__func__,
		halrf_rreg(rf, 0x5818, 0x000000ff));

	_tssi_reload_bb_registers_8851b(rf, phy, bb_reg, bb_reg_backup,
						backup_num);

	halrf_hal_bb_restore_info(rf, phy);
	halrf_tx_mode_switch(rf, phy, 0);
	if(!rf->is_chl_rfk) {
		halrf_tmac_tx_pause(rf, phy, false);
		halrf_btc_rfk_ntfy(rf, (BIT(phy) << 4), RF_BTC_TSSI, RFK_STOP);
	}
	//_tssi_reload_bb_registers_8851b(rf, phy, bb_reg, bb_reg_backup, backup_num);
#endif
}

void halrf_do_tssi_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, bool hwtx_en)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;
	u8 ch_idx = _halrf_ch_to_idx_8851b(rf, channel);
	u32 i;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   phy=%d   ch=%d   ch_idx=%d\n",
		__func__, phy, channel, ch_idx);

	tssi_info->tssi_alimk_time = 0;
	/*tssi_info->check_backup_txagc[ch_idx] = false;*/
	ch_idx++;
	ch_idx--;

	halrf_tssi_disable_8851b(rf, phy);

	for (i = RF_PATH_A; i < TSSI_PATH_MAX_8851B; i++) {
		halrf_write_fwofld_start(rf);		/*FW Offload Start*/

		_halrf_tssi_set_sys_8851b(rf, phy, i);
		_halrf_tssi_ini_txpwr_ctrl_bb_8851b(rf, phy, i);
		_halrf_tssi_ini_txpwr_ctrl_bb_he_tb_8851b(rf, phy, i);
		_halrf_tssi_set_dck_8851b(rf, phy, i);
		//_halrf_tssi_set_bbgain_split_8851b(rf, phy, i);
		_halrf_tssi_set_tmeter_tbl_8851b(rf, phy, i);
		_halrf_tssi_set_dac_gain_tbl_8851b(rf, phy, i);
		_halrf_tssi_slope_cal_org_8851b(rf, phy, i);
		_halrf_tssi_alignment_default_8851b(rf, phy, i, true);

		_halrf_tssi_set_slope_8851b(rf, phy, i);

		halrf_write_fwofld_end(rf);		/*FW Offload End*/

		//halrf_btc_rfk_ntfy(rf, (BIT(phy) << 4), RF_BTC_TSSI, RFK_START);
		//halrf_tmac_tx_pause(rf, phy, true);

		//if (rfe_type <= 50)
			//_halrf_tssi_slope_cal_8851b(rf, phy, i);

		//halrf_tmac_tx_pause(rf, phy, false);
		//halrf_btc_rfk_ntfy(rf, (BIT(phy) << 4), RF_BTC_TSSI, RFK_STOP);
#if 0
		if(!rf->is_chl_rfk) {
			halrf_btc_rfk_ntfy(rf, (BIT(phy) << 4), RF_BTC_TSSI, RFK_START);
			halrf_tmac_tx_pause(rf, phy, true);
		}
		_halrf_tssi_alimentk_8851b(rf, phy, i);
		if(!rf->is_chl_rfk) {
			halrf_tmac_tx_pause(rf, phy, false);
			halrf_btc_rfk_ntfy(rf, (BIT(phy) << 4), RF_BTC_TSSI, RFK_STOP);
		}
#endif
	}

	if (phl_is_mp_mode(rf->phl_com)) {
		if (tssi_info->tssi_type[HW_PHY_0] == TSSI_ON || tssi_info->tssi_type[HW_PHY_0] == TSSI_CAL) {
			halrf_write_fwofld_start(rf);		/*FW Offload Start*/

			halrf_tssi_enable_8851b(rf, phy);
			halrf_tssi_set_efuse_to_de_8851b(rf, phy);

			halrf_write_fwofld_end(rf);		/*FW Offload End*/
		} else
			halrf_tssi_disable_8851b(rf, phy);

	} else {
		if (rf->support_ability & HAL_RF_TX_PWR_TRACK) {
			halrf_write_fwofld_start(rf);		/*FW Offload Start*/

			halrf_tssi_enable_8851b(rf, phy);
			halrf_tssi_set_efuse_to_de_8851b(rf, phy);

			halrf_write_fwofld_end(rf);		/*FW Offload End*/

			//halrf_tssi_backup_txagc_8851b(rf, phy, true);
		}
	}

	tssi_info->do_tssi = true;
}

void halrf_do_tssi_scan_8851b(struct rf_info *rf,
					enum phl_phy_idx phy)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u32 i;
	u8 channel = rf->hal_com->band[phy].cur_chandef.center_ch;
	u8 band;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   phy=%d  channel=%d\n",
					__func__, phy, channel);

	if (rf->is_tssi_mode[RF_PATH_A] != true) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s   phy=%d   rf->is_tssi_mode[A] != true  return!!!\n",
			__func__, phy);
		return;
	}

	if (channel >= 1 && channel <= 14)
		band = TSSI_ALIMK_2G;
	else if (channel >= 36 && channel <= 64)
		band = TSSI_ALIMK_5GL;
	else if (channel >= 100 && channel <= 144)
		band = TSSI_ALIMK_5GM;
	else if (channel >= 149 && channel <= 177)
		band = TSSI_ALIMK_5GH;
	else
		band = TSSI_ALIMK_2G;

	halrf_write_fwofld_start(rf);		/*FW Offload Start*/

	halrf_tssi_disable_8851b(rf, phy);

	for (i = RF_PATH_A; i < TSSI_PATH_MAX_8851B; i++) {
		_halrf_tssi_set_sys_8851b(rf, phy, i);
		//_halrf_tssi_ini_txpwr_ctrl_bb_8851b(rf, phy, i);
		//_halrf_tssi_ini_txpwr_ctrl_bb_he_tb_8851b(rf, phy, i);
		//_halrf_tssi_set_dck_8851b(rf, phy, i);
		//_halrf_tssi_set_bbgain_split_8851b(rf, phy, i);
		_halrf_tssi_set_tmeter_tbl_8851b(rf, phy, i);
		//_halrf_tssi_set_dac_gain_tbl_8851b(rf, phy, i);
		//_halrf_tssi_slope_cal_org_8851b(rf, phy, i);
		//_halrf_tssi_set_tssi_slope_8851b(rf, phy, i);

		if (tssi_info->alignment_done[i][band] == true)
			_halrf_tssi_alimentk_done_8851b(rf, phy, i);
		else
			_halrf_tssi_alignment_default_8851b(rf, phy, i, false);
	}

	halrf_tssi_enable_8851b(rf, phy);
	halrf_tssi_set_efuse_to_de_8851b(rf, phy);
	halrf_write_fwofld_end(rf);		/*FW Offload End*/
}

void halrf_tssi_get_efuse_8851b(struct rf_info *rf,
						enum phl_phy_idx phy)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	s8 de_offset = 0;
	u32 i, j;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s\n", __func__);

	/*path s0*/
	for (i = EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_1, j = 0; i <= EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_6; i++, j++) {
		halrf_efuse_get_info(rf, i, &de_offset, 1);
		tssi_info->tssi_efuse[RF_PATH_A][EFUSE_TSSI_CCK][j] = de_offset;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI][Efuse] Efuse_addr=0x%x   value=0x%x\n",
			i, tssi_info->tssi_efuse[RF_PATH_A][EFUSE_TSSI_CCK][j]);
	}

	for (i = EFUSE_INFO_RF_2G_BW40M_A_TSSI_DE_1, j = 0; i <= EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_14; i++, j++) {
		halrf_efuse_get_info(rf, i, &de_offset, 1);
		tssi_info->tssi_efuse[RF_PATH_A][EFUSE_TSSI_MCS][j] = de_offset;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI][Efuse] Efuse_addr=0x%x   value=0x%x\n",
			i, tssi_info->tssi_efuse[RF_PATH_A][EFUSE_TSSI_MCS][j]);
	}

	/*path s1*/
	for (i = EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_1, j = 0; i <= EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_6; i++, j++) {
		halrf_efuse_get_info(rf, i, &de_offset, 1);
		tssi_info->tssi_efuse[RF_PATH_B][EFUSE_TSSI_CCK][j] = de_offset;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI][Efuse] Efuse_addr=0x%x   value=0x%x\n",
			i, tssi_info->tssi_efuse[RF_PATH_B][EFUSE_TSSI_CCK][j]);
	}

	for (i = EFUSE_INFO_RF_2G_BW40M_B_TSSI_DE_1, j = 0; i <= EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_14; i++, j++) {
		halrf_efuse_get_info(rf, i, &de_offset, 1);
		tssi_info->tssi_efuse[RF_PATH_B][EFUSE_TSSI_MCS][j] = de_offset;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI][Efuse] Efuse_addr=0x%x   value=0x%x\n",
			i, tssi_info->tssi_efuse[RF_PATH_B][EFUSE_TSSI_MCS][j]);
	}
}

bool halrf_tssi_check_efuse_data_8851b(struct rf_info *rf,
						enum phl_phy_idx phy)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u32 i, j, countr = 0;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s\n", __func__);

	halrf_tssi_get_efuse_8851b(rf, phy);

	/*path s0*/
	for (i = EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_1, j = 0; i <= EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_6; i++, j++) {
		if ((tssi_info->tssi_efuse[RF_PATH_A][EFUSE_TSSI_CCK][j] & 0xff) == 0xff)
			countr++;
	}

	for (i = EFUSE_INFO_RF_2G_BW40M_A_TSSI_DE_1, j = 0; i <= EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_14; i++, j++) {
		if ((tssi_info->tssi_efuse[RF_PATH_A][EFUSE_TSSI_MCS][j] & 0xff) == 0xff)
			countr++;
	}

	/*path s1*/
	for (i = EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_1, j = 0; i <= EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_6; i++, j++) {
		if ((tssi_info->tssi_efuse[RF_PATH_B][EFUSE_TSSI_CCK][j] & 0xff) == 0xff)
			countr++;
	}

	for (i = EFUSE_INFO_RF_2G_BW40M_B_TSSI_DE_1, j = 0; i <= EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_14; i++, j++) {
		if ((tssi_info->tssi_efuse[RF_PATH_B][EFUSE_TSSI_MCS][j] & 0xff) == 0xff)
			countr++;
	}

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI][Efuse] countr == %d\n", countr);

	if (countr == 50) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI][Efuse] Efuse all are 0xff, return false!!!\n");
		return false;
	}

	return true;	
}

void halrf_set_tssi_de_for_tx_verify_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, u32 tssi_de, u8 path)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u32 tssi_de_tmp;
	s32 s_tssi_de, tmp;
	s8 tssi_trim;
	u32 addr_cck_long[1] = {0x5858};
	u32 addr_cck_short[1] = {0x5860};
	u32 addr_cck_bitmask[1] = {0x003ff000};
	u32 addr_mcs_20m[1] = {0x5838};
	u32 addr_mcs_40m[1] = {0x5840};
	u32 addr_mcs_80m[1] = {0x5848};
	u32 addr_mcs_80m_80m[1] = {0x5850};
	u32 addr_mcs_5m[1] = {0x5828};
	u32 addr_mcs_10m[1] = {0x5830};
	u32 addr_mcs_bitmask[1] = {0x003ff000};

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   phy=%d   tssi_de=0x%x   path=%d\n",
		__func__, phy, tssi_de, path);

	if (path != RF_PATH_A) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path = %d   return!!!\n",
			__func__, path);
		return;
	}

	if (tssi_de & BIT(7))
		s_tssi_de = tssi_de | 0xffffff00;
	else
		s_tssi_de = tssi_de;

	tssi_trim = _halrf_tssi_get_ofdm_tssi_trim_de_8851b(rf, phy, path);
	tmp = s_tssi_de + tssi_trim;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] tmp(%d) = s_tssi_de(%d) + tssi_trim(%d)\n",
		tmp, s_tssi_de, tssi_trim);

	tssi_de_tmp = tmp & 0x3ff;

	halrf_wreg(rf, addr_cck_long[path], addr_cck_bitmask[path], tssi_de_tmp);
	halrf_wreg(rf, addr_cck_short[path], addr_cck_bitmask[path], tssi_de_tmp);

	halrf_wreg(rf, addr_mcs_20m[path], addr_mcs_bitmask[path], tssi_de_tmp);
	halrf_wreg(rf, addr_mcs_40m[path], addr_mcs_bitmask[path], tssi_de_tmp);
	halrf_wreg(rf, addr_mcs_80m[path], addr_mcs_bitmask[path], tssi_de_tmp);
	halrf_wreg(rf, addr_mcs_80m_80m[path], addr_mcs_bitmask[path], tssi_de_tmp);
	halrf_wreg(rf, addr_mcs_5m[path], addr_mcs_bitmask[path], tssi_de_tmp);
	halrf_wreg(rf, addr_mcs_10m[path], addr_mcs_bitmask[path], tssi_de_tmp);
	tssi_info->tssi_de[path] = tmp;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] Set TSSI DE 0x%x[21:12]=0x%x 0x%x[21:12]=0x%x path=%d\n",
		addr_mcs_20m[path],
		halrf_rreg(rf, addr_mcs_20m[path], 0x003ff000),
		addr_cck_long[path],
		halrf_rreg(rf, addr_cck_long[path], 0x003ff000),
		path);

	/*Save TSSI data for WINCLI*/
	tssi_info->curr_tssi_cck_de[path] =
		(s8)(halrf_rreg(rf, addr_cck_long[path], addr_cck_bitmask[path]) & 0xff);
	tssi_info->curr_tssi_ofdm_de[path] =
		(s8)(halrf_rreg(rf, addr_mcs_20m[path], addr_mcs_bitmask[path]) & 0xff);
	tssi_info->curr_tssi_efuse_cck_de[path] = (s8)s_tssi_de;
	tssi_info->curr_tssi_efuse_ofdm_de[path] = (s8)s_tssi_de;
	tssi_info->curr_tssi_trim_de[path] = tssi_trim;
}

void halrf_set_tssi_de_offset_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, u32 tssi_de_offset, u8 path)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	s32 tssi_tmp, s_tssi_de_offset;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   phy=%d   tssi_de_offset=0x%x   path=%d\n",
		__func__, phy, tssi_de_offset, path);

	if (tssi_de_offset & BIT(7))
		s_tssi_de_offset = tssi_de_offset | 0xffffff00;
	else
		s_tssi_de_offset = tssi_de_offset;

	s_tssi_de_offset = s_tssi_de_offset * -1;


	tssi_tmp = tssi_info->tssi_de[RF_PATH_A] + s_tssi_de_offset;
	tssi_tmp = tssi_tmp & 0x3ff;
	halrf_wreg(rf, 0x5838, 0x003ff000, tssi_tmp);	/*20M*/
	halrf_wreg(rf, 0x5858, 0x003ff000, tssi_tmp);	/*CCK long*/
	halrf_wreg(rf, 0x5828, 0x003ff000, tssi_tmp);	/*5M*/
	halrf_wreg(rf, 0x5830, 0x003ff000, tssi_tmp);	/*10M*/
	halrf_wreg(rf, 0x5840, 0x003ff000, tssi_tmp);	/*40M*/
	halrf_wreg(rf, 0x5848, 0x003ff000, tssi_tmp);	/*80M*/
	halrf_wreg(rf, 0x5850, 0x003ff000, tssi_tmp);	/*80M+80M*/
	halrf_wreg(rf, 0x5860, 0x003ff000, tssi_tmp);	/*CCK short*/

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] tssi_tmp(%d) = tssi_info->tssi_de[RF_PATH_A](%d) + s_tssi_de_offset(%d) path=%d\n",
		tssi_tmp, tssi_info->tssi_de[RF_PATH_A],
		s_tssi_de_offset, path);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] Set TSSI DE 0x5838[21:12]=0x%x 0x5858[21:12]=0x%x path=%d\n",
		halrf_rreg(rf, 0x5838, 0x003ff000),
		halrf_rreg(rf, 0x5858, 0x003ff000),
		path);
}

void halrf_set_tssi_de_offset_zero_8851b(struct rf_info *rf,
							enum phl_phy_idx phy)
{
	halrf_wreg(rf, 0x5838, 0x003ff000, 0x0);	/*20M*/
	halrf_wreg(rf, 0x5858, 0x003ff000, 0x0);	/*CCK long*/
	halrf_wreg(rf, 0x5828, 0x003ff000, 0x0);	/*5M*/
	halrf_wreg(rf, 0x5830, 0x003ff000, 0x0);	/*10M*/
	halrf_wreg(rf, 0x5840, 0x003ff000, 0x0);	/*40M*/
	halrf_wreg(rf, 0x5848, 0x003ff000, 0x0);	/*80M*/
	halrf_wreg(rf, 0x5850, 0x003ff000, 0x0);	/*80M+80M*/
	halrf_wreg(rf, 0x5860, 0x003ff000, 0x0);	/*CCK short*/

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] Set TSSI DE Zero 0x5838[21:12]=0x%x 0x5858[21:12]=0x%x\n",
		halrf_rreg(rf, 0x5838, 0x003ff000),
		halrf_rreg(rf, 0x5858, 0x003ff000));
}

void halrf_tssi_enable_8851b(struct rf_info *rf,
					enum phl_phy_idx phy)
{
	u8 i;
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s\n", __func__);

	for (i = 0; i < TSSI_PATH_MAX_8851B; i++) {
		/*_halrf_tssi_set_tssi_slope_8851b(rf, phy, i);*/
		_halrf_tssi_set_tssi_track_8851b(rf, phy, i);
		_halrf_tssi_set_txagc_offset_mv_avg_8851b(rf, phy, i);

		if (i == RF_PATH_A) { 
			halrf_wreg(rf, 0x58e4, 0x00004000, 0x0);
			halrf_wreg(rf, 0x5820, 0x80000000, 0x0);
			halrf_wreg(rf, 0x5820, 0x80000000, 0x1);
			halrf_wrf(rf, i, 0x10055, 0x00080, 0x1);
			halrf_wreg(rf, 0x5818, 0x10000000, 0x0);
			halrf_wreg(rf, 0x5818, 0x18000000, 0x3);
			
			rf->is_tssi_mode[RF_PATH_A] = true;
		}
	}
}

void halrf_tssi_disable_8851b(struct rf_info *rf,
					enum phl_phy_idx phy)
{
	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s\n", __func__);

	if (phl_is_mp_mode(rf->phl_com))
		halrf_wreg(rf, 0x5818, 0x000000ff, 0x0);

	halrf_wreg(rf, 0x5820, 0x80000000, 0x0);
	halrf_wreg(rf, 0x5818, 0x10000000, 0x0);
	halrf_wreg(rf, 0x5818, 0x10000000, 0x1);
	halrf_wreg(rf, 0x5818, 0x10000000, 0x0);
	halrf_wreg(rf, 0x58e4, 0x00004000, 0x1);
	
	rf->is_tssi_mode[RF_PATH_A] = false;
}

s32 halrf_get_online_tssi_de_8851b(struct rf_info *rf, enum phl_phy_idx phy_idx,
				u8 path, s32 dbm, s32 puot)
{
	s32 de = ((puot - dbm) * 8) / 100;
	s32 s_tssi_offset;
	u32 tssi_offset;
	s8 tssi_trim;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   phy=%d\n", __func__, phy_idx);

	if (path == RF_PATH_A) {
		tssi_offset = halrf_rreg(rf, 0x5838, 0x003ff000);
		/*tssi_offset = halrf_rreg(rf, 0x5858, 0x003ff000);*/

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] Set TSSI DE 0x5838[21:12]=0x%x 0x5858[21:12]=0x%x path=%d\n",
			halrf_rreg(rf, 0x5838, 0x003ff000),
			halrf_rreg(rf, 0x5858, 0x003ff000),
			path);
	} else {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] ======> %s Set TSSI DE path != RF_PATH_A Return!!!\n",
			__func__);
		return 0;
	}

	if (tssi_offset & BIT(9))
		s_tssi_offset = tssi_offset | 0xfffffc00;
	else
		s_tssi_offset = tssi_offset;

	tssi_trim = _halrf_tssi_get_ofdm_tssi_trim_de_8851b(rf, phy_idx, path);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] de(%d) = de(%d) + s_tssi_offset(%d) - tssi_trim(%d) path=%d\n",
		(de + s_tssi_offset - tssi_trim), de, s_tssi_offset, tssi_trim, path);

	de = de + s_tssi_offset - tssi_trim;

	de = de & 0xff;

	return de;
}

void halrf_tssi_set_efuse_to_de_8851b(struct rf_info *rf,
						enum phl_phy_idx phy)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;

	u32 addr_cck_long[1] = {0x5858};
	u32 addr_cck_short[1] = {0x5860};
	u32 addr_cck_bitmask[1] = {0x003ff000};
	u32 addr_mcs_20m[1] = {0x5838};
	u32 addr_mcs_40m[1] = {0x5840};
	u32 addr_mcs_80m[1] = {0x5848};
	u32 addr_mcs_80m_80m[1] = {0x5850};
	u32 addr_mcs_5m[1] = {0x5828};
	u32 addr_mcs_10m[1] = {0x5830};
	u32 addr_mcs_bitmask[1] = {0x003ff000};
	u32 i, group_idx;
	s32 tmp;
	s8 ofmd_de, tssi_trim;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s\n", __func__);

	if (tssi_info->tssi_type[HW_PHY_0] == TSSI_CAL) {
		/*halrf_set_tssi_de_offset_zero_8851b(rf, phy);*/
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s TSSI_CAL mode set TSSI Trim !!!\n", __func__);

		for (i = 0; i < TSSI_PATH_MAX_8851B; i++) {
			tssi_trim = _halrf_tssi_get_ofdm_tssi_trim_de_8851b(rf, phy, i);

			tmp = (s32)tssi_trim;

			halrf_wreg(rf, addr_cck_long[i], addr_cck_bitmask[i],
					(u32)(tmp & 0x3ff));
			halrf_wreg(rf, addr_cck_short[i], addr_cck_bitmask[i],
					(u32)(tmp & 0x3ff));

			RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] TSSI_CAL Set TSSI Trim CCK DE 0x%x[21:12]=0x%x\n",
				addr_cck_long[i], halrf_rreg(rf, addr_cck_long[i], addr_cck_bitmask[i]));

			halrf_wreg(rf, addr_mcs_20m[i], addr_mcs_bitmask[i],
					(u32)(tmp & 0x3ff));
			halrf_wreg(rf, addr_mcs_40m[i], addr_mcs_bitmask[i],
					(u32)(tmp & 0x3ff));
			halrf_wreg(rf, addr_mcs_80m[i], addr_mcs_bitmask[i],
					(u32)(tmp & 0x3ff));
			halrf_wreg(rf, addr_mcs_80m_80m[i], addr_mcs_bitmask[i],
					(u32)(tmp & 0x3ff));
			halrf_wreg(rf, addr_mcs_5m[i], addr_mcs_bitmask[i],
					(u32)(tmp & 0x3ff));
			halrf_wreg(rf, addr_mcs_10m[i], addr_mcs_bitmask[i],
					(u32)(tmp & 0x3ff));

			RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] TSSI_CAL Set TSSI Trim MCS DE 0x%x[21:12]=0x%x\n",
				addr_mcs_20m[i], halrf_rreg(rf, addr_mcs_20m[i], addr_mcs_bitmask[i]));

			/*Save TSSI data for WINCLI*/
			tssi_info->curr_tssi_cck_de[i] = (s8)(tmp & 0xff);
			tssi_info->curr_tssi_ofdm_de[i] = (s8)(tmp & 0xff);
			tssi_info->curr_tssi_efuse_cck_de[i] = 0;
			tssi_info->curr_tssi_efuse_ofdm_de[i] = 0;
			tssi_info->curr_tssi_trim_de[i] = tssi_trim;
		}
		return;
	}

	for (i = 0; i < TSSI_PATH_MAX_8851B; i++) {
		/*CCK*/
		group_idx = _halrf_tssi_get_cck_efuse_group_8851b(rf, phy);
		tssi_trim = _halrf_tssi_get_ofdm_tssi_trim_de_8851b(rf, phy, i);

		tmp = tssi_info->tssi_efuse[i][EFUSE_TSSI_CCK][group_idx] +
			tssi_trim;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"[TSSI] tmp(0x%x) = tssi_efuse_cck[%d](0x%x) + tssi_trim(0x%x) path=%d\n",
			tmp, group_idx, tssi_info->tssi_efuse[i][EFUSE_TSSI_CCK][group_idx],
			tssi_trim, i);

		halrf_wreg(rf, addr_cck_long[i], addr_cck_bitmask[i],
				(u32)(tmp & 0x3ff));
		halrf_wreg(rf, addr_cck_short[i], addr_cck_bitmask[i],
				(u32)(tmp & 0x3ff));

		/*Save TSSI data for WINCLI*/
		tssi_info->curr_tssi_cck_de[i] = (s8)(tmp & 0xff);

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] Set TSSI CCK DE 0x%x[21:12]=0x%x\n",
			addr_cck_long[i], halrf_rreg(rf, addr_cck_long[i], addr_cck_bitmask[i]));

		/*OFDM*/
		ofmd_de = _halrf_tssi_get_ofdm_efuse_tssi_de_8851b(rf, phy, i);
		tssi_trim = _halrf_tssi_get_ofdm_tssi_trim_de_8851b(rf, phy, i);
		tmp = ofmd_de + tssi_trim;

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"[TSSI] tmp(0x%x) = tssi_efuse_ofdm(0x%x) + tssi_trim(0x%x) path=%d\n",
			tmp, ofmd_de, tssi_trim, i);

		halrf_wreg(rf, addr_mcs_20m[i], addr_mcs_bitmask[i],
			(u32)(tmp & 0x3ff));
		halrf_wreg(rf, addr_mcs_40m[i], addr_mcs_bitmask[i],
			(u32)(tmp & 0x3ff));
		halrf_wreg(rf, addr_mcs_80m[i], addr_mcs_bitmask[i],
			(u32)(tmp & 0x3ff));
		halrf_wreg(rf, addr_mcs_80m_80m[i], addr_mcs_bitmask[i],
			(u32)(tmp & 0x3ff));
		halrf_wreg(rf, addr_mcs_5m[i], addr_mcs_bitmask[i],
			(u32)(tmp & 0x3ff));
		halrf_wreg(rf, addr_mcs_10m[i], addr_mcs_bitmask[i],
			(u32)(tmp & 0x3ff));

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] Set TSSI MCS DE 0x%x[21:12]=0x%x\n",
			addr_mcs_20m[i], halrf_rreg(rf, addr_mcs_20m[i], addr_mcs_bitmask[i]));
		
		/*Save TSSI data for WINCLI*/
		tssi_info->curr_tssi_ofdm_de[i] = (s8)(tmp & 0xff);
		tssi_info->curr_tssi_efuse_cck_de[i] = tssi_info->tssi_efuse[i][EFUSE_TSSI_CCK][group_idx];
		tssi_info->curr_tssi_efuse_ofdm_de[i] = ofmd_de;
		tssi_info->curr_tssi_trim_de[i] = tssi_trim;
	}

}

void halrf_tssi_default_txagc_8851b(struct rf_info *rf,
					enum phl_phy_idx phy, bool enable)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
//	u8 channel = rf->hal_com->band[0].cur_chandef.center_ch;
//	u8 ch_idx = _halrf_ch_to_idx_8851b(rf, channel);

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   ch=%d\n", __func__,
		rf->hal_com->band[0].cur_chandef.center_ch);

	if (enable == true) {
		/*SCAN_START*/
		tssi_info->start_time = _os_get_cur_time_us();
		if (rf->is_tssi_mode[RF_PATH_A] != true) {
			RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
				"======>%s   phy=%d   rf->is_tssi_mode[A] != true  return!!!\n",
				__func__, phy);
			RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   SCAN_START\n", __func__);
			//halrf_do_tssi_8851b(rf, phy, false);
			return;
		}
	} else {
		/*SCAN_END*/
		
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s 1 SCAN_END Set 0x5818[7:0]=0x%x\n",
			__func__,
			halrf_rreg(rf, 0x5818, 0x000000ff));

		//halrf_wreg(rf, 0x5818, 0x000000ff,
		//	tssi_info->backup_txagc_offset[RF_PATH_A][ch_idx]);

		halrf_wreg(rf, 0x5818, 0x000000ff, 0xe8);

		halrf_wreg(rf, 0x5818 , 0x10000000, 0x0);
		halrf_wreg(rf, 0x5818 , 0x10000000, 0x1);

		_halrf_tssi_alimentk_done_8851b(rf, phy, RF_PATH_A);
		_halrf_tssi_alimentk_done_8851b(rf, phy, RF_PATH_B);

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s 2 SCAN_END Set 0x5818[7:0]=0x%x\n",
			__func__,
			halrf_rreg(rf, 0x5818, 0x000000ff));

		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s   SCAN_END\n", __func__);

		tssi_info->finish_time = _os_get_cur_time_us();
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "[TSSI] %s processing time = %d.%dms\n",
			__func__,
			HALRF_ABS(tssi_info->finish_time, tssi_info->start_time) / 1000,
			HALRF_ABS(tssi_info->finish_time, tssi_info->start_time) % 1000);
	}
}

void halrf_tssi_scan_ch_setting_8851b(struct rf_info *rf,
	enum phl_phy_idx phy, enum rf_path path)
{
	halrf_tssi_scan_ch_8851b(rf, path);
	halrf_tssi_set_efuse_to_de_8851b(rf, phy);
}

void halrf_tssi_scan_ch_8851b(struct rf_info *rf, enum rf_path path)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u32 tssi_trk_man[1] = {0x5818};
	u32 tssi_en[1] = {0x5820};
	u8 channel = rf->hal_com->band[0].cur_chandef.center_ch;
	u8 ch_idx = _halrf_ch_to_idx_8851b(rf, channel);
	u8 band;

	if (rf->is_tssi_mode[RF_PATH_A] != true) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s   rf->is_tssi_mode[A] != true  return!!!\n",
			__func__);
		return;
	}

	if (path != RF_PATH_A) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s   path = %d  return!!!\n",
			__func__, path);
		return;
	}	

	if (channel >= 1 && channel <= 14)
		band = TSSI_ALIMK_2G;
	else if (channel >= 36 && channel <= 64)
		band = TSSI_ALIMK_5GL;
	else if (channel >= 100 && channel <= 144)
		band = TSSI_ALIMK_5GM;
	else if (channel >= 149 && channel <= 177)
		band = TSSI_ALIMK_5GH;
	else
		band = TSSI_ALIMK_2G;

	if (!phl_is_mp_mode(rf->phl_com)) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s   Restore_txagc_offset[%d][%d]=0x%x   ther=0x%x   channel=%d   path=%d\n",
			__func__, path, ch_idx, tssi_info->backup_txagc_offset[path][ch_idx],
			tssi_info->backup_txagc_oft_ther[path][ch_idx], channel, path);
		ch_idx++;
		ch_idx--;

		if (tssi_info->alignment_done[path][band] == true)
			_halrf_tssi_alimentk_done_8851b(rf, 0, path);
		else
			_halrf_tssi_alignment_default_8851b(rf, 0, path, false);


		halrf_wreg(rf, tssi_en[path], BIT(31), 0x0);
		halrf_wreg(rf, tssi_en[path], BIT(31), 0x1);
		//halrf_wreg(rf, tssi_trk_man[path], 0x000000ff,
		//	tssi_info->backup_txagc_offset[path][ch_idx]);
		halrf_wreg(rf, tssi_trk_man[path], 0x000000ff, 0xe8);
		halrf_wreg(rf, tssi_trk_man[path], BIT(28), 0x0);
		halrf_wreg(rf, tssi_trk_man[path], BIT(28), 0x1);
	}
}

u32 halrf_tssi_get_final_8851b(struct rf_info *rf, enum rf_path path)
{
	u32 i;

	RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======>%s   path=%d\n",
		__func__, path);

	if (path == RF_PATH_A) {
		halrf_wreg(rf, 0x5818, 0x10000000, 0x0);
		halrf_wreg(rf, 0x5820, 0x80000000, 0x0);
		halrf_wreg(rf, 0x5820, 0x80000000, 0x1);
		halrf_wreg(rf, 0x58b4, 0x0000001f, 0x7);
		for (i = 0; i < 300; i ++)
			halrf_delay_us(rf, 10);
		return halrf_rreg(rf, 0x1c00, 0x0003fe00);
	} else
		return 0x0;
}

void halrf_tssi_hw_tx_8851b(struct rf_info *rf,
			enum phl_phy_idx phy, u8 path, u16 cnt, s16 dbm, u32 rate, u8 bw,
			bool enable)
{
	_halrf_tssi_hw_tx_8851b(rf, phy, path, cnt, 5000, dbm, HT_MF_FMT, 0, enable);
}

void halrf_tssi_ant_open_8851b(struct rf_info *rf)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;

	if (rf->is_tssi_mode[RF_PATH_A] != true) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK,
			"======>%s   rf->is_tssi_mode[A] != true  return!!!\n",
			__func__);
		return;
	}

	if (phl_is_mp_mode(rf->phl_com))
		return;

	if (halrf_rreg(rf, 0x1c60, 0xff000000) == 0x7f) {
		RF_DBG(rf, DBG_RF_TX_PWR_TRACK, "======> %s 0x1c60=0x7f Do TSSI Disable,Enable\n", __func__);
		tssi_info->high_pwr_rst_cnt++;
		halrf_tssi_disable_8851b(rf, HW_PHY_0);
		halrf_tssi_enable_8851b(rf, HW_PHY_0);
	}
}

void halrf_get_tssi_info_8851b(struct rf_info *rf,
		char input[][16], u32 *_used, char *output, u32 *_out_len)
{
	struct halrf_tssi_info *tssi_info = &rf->tssi;
	u32 tmp, tmp1, pg_ther, cur_ther;
	s32 delta_tssi;
	s32 diff_ther;
	s32 int_tmp[TSSI_PATH_MAX_8851B], float_tmp[TSSI_PATH_MAX_8851B];
	s8 txagc_offset[TSSI_PATH_MAX_8851B] = {0};
	
	RF_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, " %-25s = %x\n",
		 "RF Para Ver", halrf_get_radio_ver_from_reg(rf));

	RF_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, " %-25s = 0x%x / 0x%x\n",
		 "TSSI DCK A / B", halrf_rreg(rf, 0x1c04, 0x00FFF000),
		 halrf_rreg(rf, 0x3c04, 0x00FFF000));

	tmp = halrf_rreg(rf, 0x1c78, 0x1ff);
	tmp1 = halrf_rreg(rf, 0x3c78, 0x1ff);
	RF_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, " %-25s = %d.%d / %d.%d\n",
		 "T-MAC xdbm A / B",
		 tmp / 4, tmp * 100 / 4 % 100,
		 tmp1 / 4, tmp1 * 100 / 4 % 100);

	halrf_wreg(rf, 0x58b4, 0x0000001f, 0x7);
	tmp = halrf_rreg(rf, 0x1c00, 0x0ffc0000);
	if (tmp & BIT(9))
		delta_tssi = tmp | 0xfffffc00;
	else
		delta_tssi = tmp;
	RF_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, " %-25s = %d / %d / %d\n",
		 "TSSI C / Final / Delta A", halrf_rreg(rf, 0x1c00, 0x000001ff),
		 halrf_rreg(rf, 0x1c00, 0x0003fe00), delta_tssi);

	pg_ther = halrf_rreg(rf, 0x5810, 0x0000fc00);
	cur_ther = halrf_rreg(rf, 0x1c10, 0xff000000);
	diff_ther = pg_ther - cur_ther;
	RF_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, " %-25s = %d / %d / %d / %d\n",
		 "TherA PG / Cur / Diff / F", ((tssi_info->thermal[RF_PATH_A] == 0xff) ? 0xff : pg_ther),
		 cur_ther, diff_ther,
		 (s8)halrf_rreg(rf, 0x1c08, 0xff000000));

	txagc_offset[RF_PATH_A] = (s8)halrf_rreg(rf, 0x1c60, 0xff000000);
	RF_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, " %-25s = 0x%x (%d)\n",
		 "TXAGC Offset A",
		 (txagc_offset[RF_PATH_A] & 0xff), txagc_offset[RF_PATH_A]);

	int_tmp[RF_PATH_A] = txagc_offset[RF_PATH_A] * 125 / 1000;
	float_tmp[RF_PATH_A] = txagc_offset[RF_PATH_A] * 125 % 1000;
	float_tmp[RF_PATH_A] < 0 ? float_tmp[RF_PATH_A] = float_tmp[RF_PATH_A] * -1 : 0;
	RF_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, " %-25s = %s%d.%ddB\n",
		 "TXAGC Offset dB A",
		 (int_tmp[RF_PATH_A] == 0 && txagc_offset[RF_PATH_A] < 0) ? "-" : "",
		 int_tmp[RF_PATH_A], float_tmp[RF_PATH_A]);
}

#endif	/*RF_8851B_SUPPORT*/
