// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP
 *
 */
#ifndef ISP_DEBUGFS_H
#define ISP_DEBUGFS_H

#define ISP_VERSION (6)

char *isp_dbgfs_get_reg_filename(void);
char *isp_dbgfs_get_input_filename(void);
char *isp_dbgfs_get_input_2_filename(void);

char *isp_dbgfs_get_mlsc_filename(void);
char *isp_dbgfs_get_fcurve_filename(void);
char *isp_dbgfs_get_wdr_filename(void);
char *isp_dbgfs_get_lce_filename(void);
char *isp_dbgfs_get_cnr_filename(void);
char *isp_dbgfs_get_3dnr_y_filename(void);
char *isp_dbgfs_get_3dnr_uv_filename(void);
char *isp_dbgfs_get_3dnr_v_filename(void);
char *isp_dbgfs_get_3dnr_mot_filename(void);

char *isp_dbgfs_get_dump_folder(void);
u8 isp_dbgfs_get_dump_statis(void);
u8 isp_dbgfs_get_dump_3dnr(void);
u8 isp_dbgfs_get_dump_output_frame(void);
u8 isp_dbgfs_get_dump_input_frame(void);
u8 isp_dbgfs_get_dump_3a(void);
u8 isp_dbgfs_get_dump_reg(void);

int isp_debugfs_init(struct device *d);
void isp_debugfs_remove(struct device *d);

int isp_dbgfs_target_idx_exist(u8 regQ_idx);

#endif /* ISP_DEBUGFS_H */