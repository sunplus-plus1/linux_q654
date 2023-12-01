/*
 *    public header file for vsi v4l2 driver and daemon.
 *
 *    Copyright (c) 2019, VeriSilicon Inc.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License, version 2, as
 *    published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License version 2 for more details.
 *
 *    You may obtain a copy of the GNU General Public License
 *    Version 2 at the following locations:
 *    https://opensource.org/licenses/gpl-2.0.php
 */

#ifndef VSI_V4L2_H
#define VSI_V4L2_H

#define MAX_STREAMS 200
#define MAX_GOP_SIZE 8
#define MAX_INTRA_PIC_RATE 0x7fffffff
#define NO_RESPONSE_SEQID 0xFFFFFFFE
#define SEQID_UPLIMT 0x7FFFFFFE

#define OUTF_BASE	0x3ffff000L
#define VSI_DAEMON_FNAME	"vsi_daemon_ctrl"
#define VSI_DAEMON_PATH	"/usr/bin/vsidaemon"
#define VSI_DAEMON_DEVMAJOR	100

/* some common defines between driver and daemon */
#define DEFAULTLEVEL	0

#define FRAMETYPE_I			(1<<1)
#define FRAMETYPE_P			(1<<2)
#define FRAMETYPE_B			(1<<3)
#define LAST_BUFFER_FLAG	(1<<4)
#define FORCE_IDR			(1<<5)

#define VSI_V4L2_MAX_ROI_REGIONS			8
#define VSI_V4L2_MAX_ROI_REGIONS_H1		2
#define VSI_V4L2_MAX_IPCM_REGIONS			8
#define MAX_LAYER_V4L2						8
#define VSI_V4L2_MAX_SCENE_CHANGE		20
#define VSI_V4L2_MAX_MOSAIC_NUM			12

/**********************  enc hw info structures **********************/
enum vsi_v4l2enc_pixel_fmt {
	VSI_V4L2ENC_PIX_FMT_NV12 = 0,
	VSI_V4L2ENC_PIX_FMT_NV12M,
	VSI_V4L2ENC_PIX_FMT_GREY,
	VSI_V4L2ENC_PIX_FMT_411SP,
	VSI_V4L2ENC_PIX_FMT_NV16,
	VSI_V4L2ENC_PIX_FMT_NV16M,
	VSI_V4L2ENC_PIX_FMT_NV61,
	VSI_V4L2ENC_PIX_FMT_NV61M,
	VSI_V4L2ENC_PIX_FMT_NV24,
	VSI_V4L2ENC_PIX_FMT_YUV420,
	VSI_V4L2ENC_PIX_FMT_YUV420M,
	VSI_V4L2ENC_PIX_FMT_YVU420,
	VSI_V4L2ENC_PIX_FMT_YVU420M,
	VSI_V4L2ENC_PIX_FMT_YUV422M,
	VSI_V4L2ENC_PIX_FMT_YVU422M,
	VSI_V4L2ENC_PIX_FMT_YUV444M,
	VSI_V4L2ENC_PIX_FMT_YVU444M,
	VSI_V4L2ENC_PIX_FMT_NV21,
	VSI_V4L2ENC_PIX_FMT_NV21M,
	VSI_V4L2ENC_PIX_FMT_YUYV,
	VSI_V4L2ENC_PIX_FMT_UYVY,
	VSI_V4L2ENC_PIX_FMT_RGB565,
	VSI_V4L2ENC_PIX_FMT_BGR565,
	VSI_V4L2ENC_PIX_FMT_RGB555,
	VSI_V4L2ENC_PIX_FMT_ABGR555,
	VSI_V4L2ENC_PIX_FMT_RGB444,
	VSI_V4L2ENC_PIX_FMT_BGR444,
	VSI_V4L2ENC_PIX_FMT_RGB32,
	VSI_V4L2ENC_PIX_FMT_RGBA32,
	VSI_V4L2ENC_PIX_FMT_ARGB32,
	VSI_V4L2ENC_PIX_FMT_RGBX32,
	VSI_V4L2ENC_PIX_FMT_XRGB32,
	VSI_V4L2ENC_PIX_FMT_BGR32,
	VSI_V4L2ENC_PIX_FMT_BGRA32,
	VSI_V4L2ENC_PIX_FMT_ABGR32,
	VSI_V4L2ENC_PIX_FMT_XBGR32,
	VSI_V4L2ENC_PIX_FMT_BGRX32,
	VSI_V4L2ENC_PIX_FMT_RGB101010,
	VSI_V4L2ENC_PIX_FMT_BGR101010,
	VSI_V4L2ENC_PIX_FMT_P010,
	VSI_V4L2ENC_PIX_FMT_I010,
	VSI_V4L2ENC_PIX_FMT_NV12_4L4,
	VSI_V4L2ENC_PIX_FMT_P010_4L4,
	VSI_V4L2ENC_PIX_FMT_DTRC,
	VSI_V4L2ENC_PIX_FMT_NV12X,
	VSI_V4L2ENC_PIX_FMT_TILEX,
	VSI_V4L2ENC_PIX_FMT_RFC,
	VSI_V4L2ENC_PIX_FMT_RFCX,

	VSI_V4L2ENC_PIX_FMT_MAX
};

enum vsi_v4l2enc_h264_profile {
	VSI_V4L2ENC_H264_PROFILE_BASELINE				= 0,
	VSI_V4L2ENC_H264_PROFILE_CONSTRAINED_BASELINE	= 1,
	VSI_V4L2ENC_H264_PROFILE_MAIN					= 2,
	VSI_V4L2ENC_H264_PROFILE_EXTENDED				= 3,
	VSI_V4L2ENC_H264_PROFILE_HIGH					= 4,
	VSI_V4L2ENC_H264_PROFILE_HIGH_10				= 5,
	VSI_V4L2ENC_H264_PROFILE_HIGH_422				= 6,
	VSI_V4L2ENC_H264_PROFILE_HIGH_444_PREDICTIVE	= 7,
	VSI_V4L2ENC_H264_PROFILE_HIGH_10_INTRA			= 8,
	VSI_V4L2ENC_H264_PROFILE_HIGH_422_INTRA			= 9,
	VSI_V4L2ENC_H264_PROFILE_HIGH_444_INTRA			= 10,
	VSI_V4L2ENC_H264_PROFILE_CAVLC_444_INTRA		= 11,
	VSI_V4L2ENC_H264_PROFILE_SCALABLE_BASELINE		= 12,
	VSI_V4L2ENC_H264_PROFILE_SCALABLE_HIGH			= 13,
	VSI_V4L2ENC_H264_PROFILE_SCALABLE_HIGH_INTRA	= 14,
	VSI_V4L2ENC_H264_PROFILE_STEREO_HIGH			= 15,
	VSI_V4L2ENC_H264_PROFILE_MULTIVIEW_HIGH			= 16,
	VSI_V4L2ENC_H264_PROFILE_CONSTRAINED_HIGH		= 17,
};

enum vsi_v4l2enc_hevc_profile {
	VSI_V4L2ENC_HEVC_PROFILE_MAIN 					= 0,
	VSI_V4L2ENC_HEVC_PROFILE_MAIN_STILL_PICTURE 	= 1,
	VSI_V4L2ENC_HEVC_PROFILE_MAIN_10 				= 2,
};

enum vsi_v4l2enc_vp9_profile {
	VSI_V4L2ENC_VP9_PROFILE_0		= 0,
	VSI_V4L2ENC_VP9_PROFILE_1		= 1,
	VSI_V4L2ENC_VP9_PROFILE_2		= 2,
	VSI_V4L2ENC_VP9_PROFILE_3		= 3,
};

enum vsi_v4l2enc_vp8_profile {
	VSI_V4L2ENC_VP8_PROFILE_0		= 0,
	VSI_V4L2ENC_VP8_PROFILE_1		= 1,
	VSI_V4L2ENC_VP8_PROFILE_2		= 2,
	VSI_V4L2ENC_VP8_PROFILE_3		= 3,
};

enum vsi_v4l2enc_av1_profile {
	VSI_V4L2ENC_AV1_MAIN_PROFILE			= 0,
	VSI_V4L2ENC_AV1_HIGH_PROFILE			= 1,
	VSI_V4L2ENC_AV1_PROFESSIONAL_PROFILE	= 2,
};

enum vsi_v4l2enc_h264_level {
	VSI_V4L2ENC_H264_LEVEL_1_0	= 0,
	VSI_V4L2ENC_H264_LEVEL_1B	= 1,
	VSI_V4L2ENC_H264_LEVEL_1_1	= 2,
	VSI_V4L2ENC_H264_LEVEL_1_2	= 3,
	VSI_V4L2ENC_H264_LEVEL_1_3	= 4,
	VSI_V4L2ENC_H264_LEVEL_2_0	= 5,
	VSI_V4L2ENC_H264_LEVEL_2_1	= 6,
	VSI_V4L2ENC_H264_LEVEL_2_2	= 7,
	VSI_V4L2ENC_H264_LEVEL_3_0	= 8,
	VSI_V4L2ENC_H264_LEVEL_3_1	= 9,
	VSI_V4L2ENC_H264_LEVEL_3_2	= 10,
	VSI_V4L2ENC_H264_LEVEL_4_0	= 11,
	VSI_V4L2ENC_H264_LEVEL_4_1	= 12,
	VSI_V4L2ENC_H264_LEVEL_4_2	= 13,
	VSI_V4L2ENC_H264_LEVEL_5_0	= 14,
	VSI_V4L2ENC_H264_LEVEL_5_1	= 15,
	VSI_V4L2ENC_H264_LEVEL_5_2	= 16,
	VSI_V4L2ENC_H264_LEVEL_6_0	= 17,
	VSI_V4L2ENC_H264_LEVEL_6_1	= 18,
	VSI_V4L2ENC_H264_LEVEL_6_2	= 19,
};

enum vsi_v4l2enc_hevc_level {
	VSI_V4L2ENC_HEVC_LEVEL_1	= 0,
	VSI_V4L2ENC_HEVC_LEVEL_2	= 1,
	VSI_V4L2ENC_HEVC_LEVEL_2_1	= 2,
	VSI_V4L2ENC_HEVC_LEVEL_3	= 3,
	VSI_V4L2ENC_HEVC_LEVEL_3_1	= 4,
	VSI_V4L2ENC_HEVC_LEVEL_4	= 5,
	VSI_V4L2ENC_HEVC_LEVEL_4_1	= 6,
	VSI_V4L2ENC_HEVC_LEVEL_5	= 7,
	VSI_V4L2ENC_HEVC_LEVEL_5_1	= 8,
	VSI_V4L2ENC_HEVC_LEVEL_5_2	= 9,
	VSI_V4L2ENC_HEVC_LEVEL_6	= 10,
	VSI_V4L2ENC_HEVC_LEVEL_6_1	= 11,
	VSI_V4L2ENC_HEVC_LEVEL_6_2	= 12,
};

enum vsi_v4l2enc_vp9_level {
	VSI_V4L2ENC_VP9_LEVEL_1_0	= 0,
	VSI_V4L2ENC_VP9_LEVEL_1_1	= 1,
	VSI_V4L2ENC_VP9_LEVEL_2_0	= 2,
	VSI_V4L2ENC_VP9_LEVEL_2_1	= 3,
	VSI_V4L2ENC_VP9_LEVEL_3_0	= 4,
	VSI_V4L2ENC_VP9_LEVEL_3_1	= 5,
	VSI_V4L2ENC_VP9_LEVEL_4_0	= 6,
	VSI_V4L2ENC_VP9_LEVEL_4_1	= 7,
	VSI_V4L2ENC_VP9_LEVEL_5_0	= 8,
	VSI_V4L2ENC_VP9_LEVEL_5_1	= 9,
	VSI_V4L2ENC_VP9_LEVEL_5_2	= 10,
	VSI_V4L2ENC_VP9_LEVEL_6_0	= 11,
	VSI_V4L2ENC_VP9_LEVEL_6_1	= 12,
	VSI_V4L2ENC_VP9_LEVEL_6_2	= 13,
};

enum vsi_v4l2enc_av1_level {
	VSI_V4L2ENC_AV1_LEVEL_2_0	= 0,
	VSI_V4L2ENC_AV1_LEVEL_2_1	= 1,
	VSI_V4L2ENC_AV1_LEVEL_2_2	= 2,
	VSI_V4L2ENC_AV1_LEVEL_2_3	= 3,
	VSI_V4L2ENC_AV1_LEVEL_3_0	= 4,
	VSI_V4L2ENC_AV1_LEVEL_3_1	= 5,
	VSI_V4L2ENC_AV1_LEVEL_3_2	= 6,
	VSI_V4L2ENC_AV1_LEVEL_3_3	= 7,
	VSI_V4L2ENC_AV1_LEVEL_4_0	= 8,
	VSI_V4L2ENC_AV1_LEVEL_4_1	= 9,
	VSI_V4L2ENC_AV1_LEVEL_4_2	= 10,
	VSI_V4L2ENC_AV1_LEVEL_4_3	= 11,
	VSI_V4L2ENC_AV1_LEVEL_5_0	= 12,
	VSI_V4L2ENC_AV1_LEVEL_5_1	= 13,
	VSI_V4L2ENC_AV1_LEVEL_5_2	= 14,
	VSI_V4L2ENC_AV1_LEVEL_5_3	= 15,
	VSI_V4L2ENC_AV1_LEVEL_6_0	= 16,
	VSI_V4L2ENC_AV1_LEVEL_6_1	= 17,
	VSI_V4L2ENC_AV1_LEVEL_6_2	= 18,
	VSI_V4L2ENC_AV1_LEVEL_6_3	= 19,
	VSI_V4L2ENC_AV1_LEVEL_7_0	= 20,
	VSI_V4L2ENC_AV1_LEVEL_7_1	= 21,
	VSI_V4L2ENC_AV1_LEVEL_7_2	= 22,
	VSI_V4L2ENC_AV1_LEVEL_7_3	= 23,
};

/* color space*/
enum vsi_v4l2enc_color_space {
	VSI_V4L2_COLORSPACE_REC709	= 0,
	VSI_V4L2_COLORSPACE_REC601	= 1,
	VSI_V4L2_COLORSPACE_BT2020	= 2,
};

/* color space conversion quantization range */
enum vsi_v4l2enc_csc_range {
	VSI_V4L2ENC_CSC_DEFAULT		= 0,
	VSI_V4L2ENC_CSC_FULL_RANGE	= 1,
	VSI_V4L2ENC_CSC_LIM_RANGE	= 2,
};

enum vsi_v4l2_vui_aspect {
	VSI_V4L2_VUI_ASPECT_1x1     = 0,
	VSI_V4L2_VUI_ASPECT_4x3,
	VSI_V4L2_VUI_ASPECT_16x9,
	VSI_V4L2_VUI_ASPECT_221x100,
};

enum vsi_v4l2_bitrate_mode {
	VSI_V4L2_BITRATE_MODE_VBR = 0,
	VSI_V4L2_BITRATE_MODE_CBR,
	VSI_V4L2_BITRATE_MODE_CQ,
	VSI_V4L2_BITRATE_MODE_CVBR,
	VSI_V4L2_BITRATE_MODE_ABR,
	VSI_V4L2_BITRATE_MODE_CRF,
};

enum vsi_v4l2_hevctier {
	VSI_V4L2_HEVC_TIER_MAIN = 0,
	VSI_V4L2_HEVC_TIER_HIGH,
};

enum vsi_v4l2_hevc_refreshtype {
	VSI_V4L2_HEVC_REFRESH_NONE		= 0,
	VSI_V4L2_HEVC_REFRESH_CRA,
	VSI_V4L2_HEVC_REFRESH_IDR,
};

/******************	communication with v4l2 driver. ***********/
//multislice_mode bit
#define SLICEMODE_SINGLE		0
#define SLICEMODE_MAXMB		1
#define SLICEMODE_MAXSIZE		2
//jpeg subsampling mode bit
#define JPG_SUBSAMPLING_444	0
#define JPG_SUBSAMPLING_422	1
#define JPG_SUBSAMPLING_420	2
#define JPG_SUBSAMPLING_411	3
#define JPG_SUBSAMPLING_410	4
#define JPG_SUBSAMPLING_GRAY	5
//bitdepth bit
#define BITDEPTH_8		0
#define BITDEPTH_10		1
#define BITDEPTH_12		2
#define BITDEPTH_14		3
#define BITDEPTH_16		4
//meSearchRange pixels bit
#define MESEARCH_24	0
#define MESEARCH_40	1
#define MESEARCH_48	2
#define MESEARCH_64	3
//loopfilter mode bit
#define LOOPFILTER_DISABLE		0
#define LOOPFILTER_ENABLE		1
#define LOOPFILTER_DIS_ATSLICE	2
//map_meVertSearchRange bit
#define MESARCH_RANGE24	0
#define MESARCH_RANGE40	1
#define MESARCH_RANGE48	2
#define MESARCH_RANGE64	3
//map_headerMode bit
#define HEADERMODE_SEPARATE	0
#define HEADERMODE_JOINED	1

#define PIX_FMT_BIT_VIDEO	0	//if pixel format supported by video encoder
#define PIX_FMT_BIT_IMAGE	1	//if pixel format supported by image encoder

#define ENC_SCALEDOUT_YUV422 0	//YUYV
#define ENC_SCALEDOUT_NV12   1
struct vsi_v4l2_codedfmt_common {
	s32 valid;
	s32 min_IQP;
	s32 max_IQP;
	u32 map_bitdepth;

	s32 min_width;		//vsi_enum_encfsize
	s32 min_height;
	s32 max_width;
	s32 max_height;

	s32 width_align;	//src video/image requirement, refer to vsi_enum_encfsize
	s32 height_align;
	s32 crop_align;		//refer vsiv4l2_verifycrop

	u32 map_profile;
	u32 map_level;
};

struct vsi_v4l2_codedfmt_HEVC {
	struct vsi_v4l2_codedfmt_common common;
	s32 min_PBQP;
	s32 max_PBQP;
	s32 map_meVertSearchRange;
	s32 max_refno;
	s32 max_codinglayer;
	s32 max_codinglayerQP;
	s32 max_level;
	s32 max_profile;
	s32 max_tier;
	s32 has_strong_intra_smoothing;
	s32 has_Bframe;
	u32 map_multislicemode;
	u32 map_loopfilter;
};

struct vsi_v4l2_codedfmt_H264 {
	struct vsi_v4l2_codedfmt_common common;
	s32 min_PBQP;
	s32 max_PBQP;
	s32 map_meVertSearchRange;
	s32 max_codinglayer;
	s32 max_codinglayerQP;
	s32 max_level;
	s32 max_profile;
	s32 has_transform8x8;
	s32 has_constrained_intra_pred;
	u32 map_loopfilter;
	s32 has_Bframe;
	u32 map_multislicemode;
	s32 min_chromaqpoffset;
	s32 max_chromaqpoffset;
};

struct vsi_v4l2_codedfmt_JPEG {
	struct vsi_v4l2_codedfmt_common common;
	u32 map_jpgcodingmode;
	u32 max_restartinterval;
};

struct vsi_v4l2_codedfmt_VP8 {
	struct vsi_v4l2_codedfmt_common common;
	s32 min_PBQP;
	s32 max_PBQP;
	s32 max_refno;
	s32 max_profile;
	s32 max_vpxdctPartitions;
	s32 max_vpxfilterLevel;
	s32 max_vpxfilterSharpness;
	s32 max_vpxgoldenPictureRate;
	s32 has_Bframe;
};

struct vsi_v4l2_codedfmt_VP9 {
	struct vsi_v4l2_codedfmt_common common;
	s32 min_PBQP;
	s32 max_PBQP;
	s32 max_level;
	s32 max_profile;
	s32 max_vpxfilterLevel;
	s32 max_vpxfilterSharpness;
	s32 max_vpxgoldenPictureRate;
	s32 has_Bframe;
};

struct vsi_v4l2_codedfmt_AV1 {
	struct vsi_v4l2_codedfmt_common common;
	s32 min_PBQP;
	s32 max_PBQP;
	s32 max_level;
	s32 max_profile;
	s32 has_Bframe;
};

struct vsi_v4l2_codedfmt_WEBP {
	struct vsi_v4l2_codedfmt_common common;
	//s32 max_profile;
	s32 max_vpxdctPartitions;
	s32 max_vpxfilterLevel;
	s32 max_vpxfilterSharpness;
};


//ctrl table's .def value. Make sync between driver and daemon init value since kernel won't set it if app uses def value
struct vsi_v4l2_default_ctrlval {
	s32 intraPicRate;		//GOP_SIZE and REFRESH_PERIOD
	u32 bitPerSecond;
	s32 gopSize;
	s32 bitratemode;			//refer to above bitrate_mode
	s32 tolMovingBitRate;		//BITRATE_PEAK
	s32 enable_picRc;
	s32 enable_ctbRc;
	s32 enable_idrHdr;		//REPEAT_SEQ_HEADER
	s32 multislice_mode;		//use macro above
	s32 multislice_maxsliceMB;
	s32 gdrDuration;
	s32 enable_pictureSkip;
	s32 enable_sendAud;
	s32 vui_aspect;		//see enum vui_aspect above
	s32 tc_Offset;	//H264_LOOP_FILTER_ALPHA and HEVC_LF_TC_OFFSET_DIV2
	s32 beta_Offset;	//H264_LOOP_FILTER_BETA and HEVC_LF_BETA_OFFSET_DIV2
	s32 meVertSearchRange;		//use macro above
	s32 resendSPSPPS;
	s32 enable_hierachy;		//h264/265
	s32 codinglayerNumber;		//h264/265
	s32 codinglayerqp;		//H264_HIERARCHICAL_CODING_LAYER_QP lower 16 bits and HEVC
	s32 headermode;		//V4L2_CID_MPEG_VIDEO_HEADER_MODE, JOINT or SEPARATE

	s32 profile_h264;		//use macro above
	s32 profile_hevc;
	s32 profile_vp8;
	s32 profile_vp9;

	s32 avclevel;		//max lvl
	s32 hevclevel;
	s32 vp9level;

	s32 qpMinI;		//for 264/265
	s32 qpMaxI;		//for 264/265
	s32 qpMinPB;		//for 264/265
	s32 qpMaxPB;		//for 264/265
	s32 qpMax_h26x;
	s32 qpMin_h26x;
	s32 qpMin_vpx;		//-1
	s32 qpMax_vpx;

	s32 qpHdr;			//CONSTANT_QUALITY		-1
	s32 qpHdrB;		//H264/HEVC_B_FRAME_QP
	s32 qpHdrI_h26x;
	s32 qpHdrP_h26x;
	s32 qpHdrI_vpx;
	s32 qpHdrP_vpx;

	s32 jpg_fixedQP;		//-1
	s32 jpg_restartInterval;
	s32 jpg_codingMode;		//macro above

	s32 hevc_PRefN;
	s32 hevc_tier;			//macro above
	s32 hevc_refreshtype;	//macro above
	s32 hevc_temporalId;
	s32 hevc_enableTMVP;
	s32 hevc_withoutStartcode;
	s32 hevc_enableStrongsmoothing;
	s32 hevc_disableDeblockingFilter;

	s32 h264_enableCabac;
	s32 h264_cpbSize;		//-1
	s32 h264_8x8transform;
	s32 h264_chroma_qp_offset;
	s32 h264_disableDeblockingFilter;
	s32 h264_enableConstrainedIntraPred;
	s32 h264_vui_sar_enable;

	s32 vpx_dctPartitions;
	s32 vpx_PRefN;
	s32 vpx_filterLevel;
	s32 vpx_filterSharpness;
	s32 vpx_goldenPictureRate;
};
/**********************  enc hw info structures end **********************/

/**********************  dec hw info structures **********************/
enum vsi_v4l2dec_h264_level {
	VSI_V4L2DEC_H264_LEVEL_1_0	= 0,
	VSI_V4L2DEC_H264_LEVEL_5_1	= 15,
	VSI_V4L2DEC_H264_LEVEL_5_2	= 16,
	VSI_V4L2DEC_H264_LEVEL_6_0	= 17,
	VSI_V4L2DEC_H264_LEVEL_6_1	= 18,
	VSI_V4L2DEC_H264_LEVEL_6_2	= 19,
};

enum vsi_v4l2dec_hevc_level {
	VSI_V4L2DEC_HEVC_LEVEL_1	= 0,
	VSI_V4L2DEC_HEVC_LEVEL_5_1	= 8,
	VSI_V4L2DEC_HEVC_LEVEL_5_2	= 9,
	VSI_V4L2DEC_HEVC_LEVEL_6	= 10,
	VSI_V4L2DEC_HEVC_LEVEL_6_1	= 11,
	VSI_V4L2DEC_HEVC_LEVEL_6_2	= 12,
};

struct vsi_v4l2_decfmt_common {
	s32 valid;

	s32 min_width;		//vsi_enum_decfsize
	s32 min_height;
	s32 max_width;
	s32 max_height;
	s32 width_align;
	s32 height_align;
};

struct vsi_v4l2_decfmt_HEVC {
	struct vsi_v4l2_decfmt_common common;
	s32 max_level;
};

struct vsi_v4l2_decfmt_H264 {
	struct vsi_v4l2_decfmt_common common;
	s32 max_level;
};

struct vsi_v4l2_decfmt_MVC {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_JPEG {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_VP6 {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_VP7 {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_WEBP {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_VP8 {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_VP9 {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_AV1 {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_MPEG2 {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_SORENSON {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_DIVX {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_XVID {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_MPEG4 {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_H263 {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_VC1G {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_VC1L {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_RV {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_AVS {
	struct vsi_v4l2_decfmt_common common;
};

struct vsi_v4l2_decfmt_AVS2 {
	struct vsi_v4l2_decfmt_common common;
};
/**********************  dec hw info structures end **********************/

struct vsi_v4l2_dev_info {
	s32 dec_corenum;
	s32 enc_corenum;
	//s32 enc_isH1;
	//u32 max_dec_resolution;
	//u64 hw_decformat;

	/**********************  enc hw info **********************/
	//u64 hw_encformat;

	//bits: 0..15, Basic align (e.g. for Y). 16..19, 20..23, 24..27,28..31, plane[0]-[3]'s division factor
	u32 enc_video_stride_align; //encoder ctrl-sw video requested stride align
	u32 enc_image_stride_align; //encoder ctrl-sw image requested stride align
	s32 b_pixelAlign;		//Pleaser refer to vsiv4l2_enc_getalign(). 1 means align to pixel, 0 means align to byte
	u64 max_bitrate;
	s32 has_framerc;
	s32 has_ctbrc;
	s32 has_rotation;
	s32 has_mirror;
	s32 has_enc_securemode;
	s32 max_ROIregion;
	s32 max_IPCMregion;
	s32 max_gdrDuration;
	u32 map_headerMode;
	uint32_t map_scaledOutput;	//encoder prp scaled output support formats
	struct vsi_v4l2_codedfmt_HEVC hevcinfo;
	struct vsi_v4l2_codedfmt_H264 h264info;
	struct vsi_v4l2_codedfmt_JPEG jpeginfo;
	struct vsi_v4l2_codedfmt_VP8 vp8info;
	struct vsi_v4l2_codedfmt_VP9 vp9info;
	struct vsi_v4l2_codedfmt_AV1 av1info;
	struct vsi_v4l2_codedfmt_WEBP webpinfo;
	u8 has_pix_fmt[VSI_V4L2ENC_PIX_FMT_MAX];
	struct vsi_v4l2_default_ctrlval def_ctrlval;

	/**********************  enc hw info end **********************/
	/**********************  dec hw info **********************/
	u64 compressor_fmt;   // compressor supported formats. bit 0: has dec400 (1) or not (0)
	s32 has_DTRC;
	s32 has_RFC;
	struct vsi_v4l2_decfmt_HEVC decinfo_HEVC;
	struct vsi_v4l2_decfmt_H264 decinfo_H264;
	struct vsi_v4l2_decfmt_MVC decinfo_H264_MVC;
	struct vsi_v4l2_decfmt_JPEG decinfo_JPEG;
	struct vsi_v4l2_decfmt_VP6 decinfo_VP6;
	struct vsi_v4l2_decfmt_VP7 decinfo_VP7;
	struct vsi_v4l2_decfmt_WEBP decinfo_WEBP;
	struct vsi_v4l2_decfmt_VP8 decinfo_VP8;
	struct vsi_v4l2_decfmt_VP9 decinfo_VP9;
	struct vsi_v4l2_decfmt_AV1 decinfo_AV1;
	struct vsi_v4l2_decfmt_MPEG2 decinfo_MPEG2;
	struct vsi_v4l2_decfmt_SORENSON decinfo_SORENSON;
	struct vsi_v4l2_decfmt_DIVX decinfo_DIVX;
	struct vsi_v4l2_decfmt_XVID decinfo_XVID;
	struct vsi_v4l2_decfmt_MPEG4 decinfo_MPEG4;
	struct vsi_v4l2_decfmt_H263 decinfo_H263;
	struct vsi_v4l2_decfmt_VC1G decinfo_VC1_G;
	struct vsi_v4l2_decfmt_VC1L decinfo_VC1_L;
	struct vsi_v4l2_decfmt_RV decinfo_RV;
	struct vsi_v4l2_decfmt_AVS decinfo_AVS;
	struct vsi_v4l2_decfmt_AVS2 decinfo_AVS2;
	/**********************  dec hw info end **********************/
};

/* daemon ioctl id definitions */
#define VSIV4L2_IOCTL_BASE			'd'
#define VSI_IOCTL_CMD_BASE		_IO(VSIV4L2_IOCTL_BASE, 0x44)

/* user space daemno should use this ioctl to initial HW info to v4l2 driver */
#define VSI_IOCTL_CMD_INITDEV		_IOW(VSIV4L2_IOCTL_BASE, 45, struct vsi_v4l2_dev_info)
/* end of daemon ioctl id definitions */

enum v4l2_daemon_cmd_id {
	/*  every command should mark which kind of parameters is valid.
	 *      For example, V4L2_DAEMON_VIDIOC_BUF_RDY can contains input or output buffers.
	 *          also it can contains other parameters.  */
	V4L2_DAEMON_VIDIOC_STREAMON = 0,//for streamon and start
	V4L2_DAEMON_VIDIOC_BUF_RDY,
	V4L2_DAEMON_VIDIOC_CMD_STOP, //this is for flush.
	V4L2_DAEMON_VIDIOC_DESTROY_ENC,	//enc destroy
	V4L2_DAEMON_VIDIOC_ENC_RESET,	//enc reset, as in spec
	//above are enc cmds

	V4L2_DAEMON_VIDIOC_FAKE,//fake command.

	/*Below is for decoder*/
	V4L2_DAEMON_VIDIOC_S_EXT_CTRLS,
	V4L2_DAEMON_VIDIOC_RESET_BITRATE,
	V4L2_DAEMON_VIDIOC_CHANGE_RES,
	V4L2_DAEMON_VIDIOC_G_FMT,
	V4L2_DAEMON_VIDIOC_S_SELECTION,
	V4L2_DAEMON_VIDIOC_S_FMT,
	V4L2_DAEMON_VIDIOC_PACKET, // tell daemon a frame is ready.
	V4L2_DAEMON_VIDIOC_STREAMON_CAPTURE,//for streamon and start
	V4L2_DAEMON_VIDIOC_STREAMON_OUTPUT,
	V4L2_DAEMON_VIDIOC_STREAMOFF_CAPTURE,
	V4L2_DAEMON_VIDIOC_STREAMOFF_OUTPUT,
	V4L2_DAEMON_VIDIOC_CMD_START,
	V4L2_DAEMON_VIDIOC_FRAME,
	V4L2_DAEMON_VIDIOC_DESTROY_DEC,

	V4L2_DAEMON_VIDIOC_EXIT,		//daemon should exit itself
	V4L2_DAEMON_VIDIOC_PICCONSUMED,
	V4L2_DAEMON_VIDIOC_CROPCHANGE,
	V4L2_DAEMON_VIDIOC_WARNONOPTION,
	V4L2_DAEMON_VIDIOC_STREAMOFF_CAPTURE_DONE,
	V4L2_DAEMON_VIDIOC_STREAMOFF_OUTPUT_DONE,
	V4L2_DAEMON_VIDIOC_TOTAL_AMOUNT,
};

enum v4l2_daemon_codec_fmt {
	/*enc format, identical to VCEncVideoCodecFormat except name*/
	V4L2_DAEMON_CODEC_ENC_BASE_ID = 0,	/*must start from 0*/
	V4L2_DAEMON_CODEC_ENC_HEVC = V4L2_DAEMON_CODEC_ENC_BASE_ID,
	V4L2_DAEMON_CODEC_ENC_H264,
	V4L2_DAEMON_CODEC_ENC_AV1,
	V4L2_DAEMON_CODEC_ENC_VP8,
	V4L2_DAEMON_CODEC_ENC_VP9,
	V4L2_DAEMON_CODEC_ENC_JPEG,
	V4L2_DAEMON_CODEC_ENC_WEBP,

	/*dec format*/
	V4L2_DAEMON_CODEC_DEC_BASE_ID,
	V4L2_DAEMON_CODEC_DEC_HEVC = V4L2_DAEMON_CODEC_DEC_BASE_ID,
	V4L2_DAEMON_CODEC_DEC_H264,
	V4L2_DAEMON_CODEC_DEC_H264_MVC,
	V4L2_DAEMON_CODEC_DEC_JPEG,
	V4L2_DAEMON_CODEC_DEC_VP9,
	V4L2_DAEMON_CODEC_DEC_MPEG2,
	V4L2_DAEMON_CODEC_DEC_MPEG4,
	V4L2_DAEMON_CODEC_DEC_SORENSON,
	V4L2_DAEMON_CODEC_DEC_VP6,
	V4L2_DAEMON_CODEC_DEC_VP7,
	V4L2_DAEMON_CODEC_DEC_VP8,
	V4L2_DAEMON_CODEC_DEC_WEBP,
	V4L2_DAEMON_CODEC_DEC_H263,
	V4L2_DAEMON_CODEC_DEC_VC1_G,
	V4L2_DAEMON_CODEC_DEC_VC1_L,
	V4L2_DAEMON_CODEC_DEC_RV,
	V4L2_DAEMON_CODEC_DEC_AVS,
	V4L2_DAEMON_CODEC_DEC_AVS2,
	V4L2_DAEMON_CODEC_DEC_DIVX,
	V4L2_DAEMON_CODEC_DEC_XVID,
	V4L2_DAEMON_CODEC_DEC_AV1,
	V4L2_DAEMON_CODEC_UNKNOW_TYPE,
};

#define DEC_FMT_BIT(codec) (1uLL << ((codec) - V4L2_DAEMON_CODEC_DEC_BASE_ID))
#define ENC_FMT_BIT(codec) (1uLL << ((codec) - V4L2_DAEMON_CODEC_ENC_BASE_ID))

enum vsi_v4l2dec_pixfmt {
	VSI_V4L2_DECOUT_DEFAULT,
	VSI_V4L2_DEC_PIX_FMT_NV12,
	VSI_V4L2_DEC_PIX_FMT_400,
	VSI_V4L2_DEC_PIX_FMT_411SP,
	VSI_V4L2_DEC_PIX_FMT_422SP,
	VSI_V4L2_DEC_PIX_FMT_444SP,

	VSI_V4L2_DECOUT_DTRC,
	VSI_V4L2_DECOUT_P010,
	VSI_V4L2_DECOUT_NV12_10BIT,
	VSI_V4L2_DECOUT_DTRC_10BIT,
	VSI_V4L2_DECOUT_RFC,
	VSI_V4L2_DECOUT_RFC_10BIT,
};

enum {
	DAEMON_OK = 0,						// no error.
	DAEMON_ENC_FRAME_READY = 1,			// frame encoded
	DAEMON_ENC_FRAME_ENQUEUE = 2,		// frame enqueued

	DAEMON_ERR_INST_CREATE = -1,		// inst_init() failed.
	DAEMON_ERR_SIGNAL_CONFIG = -2,		// sigsetjmp() failed.
	DAEMON_ERR_DAEMON_MISSING = -3,     // daemon is not alive.
	DAEMON_ERR_NO_MEM = -4,				// no mem, used also by driver.

	DAEMON_ERR_ENC_PARA = -100,			// Parameters Error.
	DAEMON_ERR_ENC_NOT_SUPPORT = -101,	// Not Support Error.
	DAEMON_ERR_ENC_INTERNAL = -102,		// Ctrlsw reported Error.
	DAEMON_ERR_ENC_BUF_MISSED = -103,	// No desired input buffer.
	DAEMON_ERR_ENC_FATAL_ERROR = -104,	// Fatal error.

	DAEMON_ERR_DEC_FATAL_ERROR = -200,	// Fatal error.
	DAEMON_ERR_DEC_METADATA_ONLY = -201,	// CMD_STOP after metadata-only.
};

//warn type attached in V4L2_DAEMON_VIDIOC_WARNONOPTION message. Stored in msg.error member
enum {
	UNKONW_WARNTYPE = -1,		//not known warning type
	WARN_ROIREGION,			//(part of)roi region can not work with media setting and be ignored by enc
	WARN_IPCMREGION,			//(part of)ipcm region can not work with media setting and be ignored by enc
	WARN_LEVEL,				//current level cant't work with media setting and be updated by enc

	WARN_INTRA_AREA,			//intra region can not work with media setting and be ignored by enc
	WARN_GOP_SIZE,				//wrong gop size (B frame numbers) setting and be updated by enc
};

struct v4l2_daemon_enc_buffers {
	/*IO*/
	s32 inbufidx;	 //from v4l2 driver, don't modify it
	s32 outbufidx;	 //-1:invalid, other:valid.

	dma_addr_t busLuma;
	s32 busLumaSize;
	dma_addr_t busChromaU;
	s32 busChromaUSize;
	dma_addr_t busChromaV;
	s32 busChromaVSize;

	dma_addr_t busLumaOrig;
	dma_addr_t busChromaUOrig;
	dma_addr_t busChromaVOrig;

	dma_addr_t busOutBuf;
	u32 outBufSize;

	dma_addr_t busScaleOutBuf;
	u32 scaleoutBufSize;

	u32 bytesused;	//bytes in buffer from user app, and encoded bytes from daemon
	u32 scale_bytesout;	//bytesused for output in scale out buffer
	s64 timestamp;

	long addr_offset;	//gap between VPU and CPU bus address, def 0
};

enum {
	V4L2_ENCCFG_UNKNOWN = 0,
	//V4L2_ENCCFG_VIDEO_COMMON,
	//V4L2_ENCCFG_JPEG_COMMON,
	V4L2_ENCCFG_FRAMERATE,
	V4L2_ENCCFG_CROPINFO,
	V4L2_ENCCFG_INTRAPICRATE,
	V4L2_ENCCFG_GOPSIZE,
	V4L2_ENCCFG_PROFILE,
	V4L2_ENCCFG_LEVEL,
	V4L2_ENCCFG_BITRATE,
	V4L2_ENCCFG_QPRANGE,
	V4L2_ENCCFG_BITRATEMODE,
	V4L2_ENCCFG_SLICEINFO,
	V4L2_ENCCFG_RCMODE,
	V4L2_ENCCFG_QPHDR,
	V4L2_ENCCFG_QPHDRIP,
	V4L2_ENCCFG_ROTATION,
	V4L2_ENCCFG_GDRDURATION,
	V4L2_ENCCFG_IDRHDR,
	V4L2_ENCCFG_ENTROYPMODE,
	V4L2_ENCCFG_CPBSIZE,
	V4L2_ENCCFG_TRANS8X8,
	V4L2_ENCCFG_CTRINTRAPRED,
	V4L2_ENCCFG_FORCEIDR,
	V4L2_ENCCFG_ROIINFO,
	V4L2_ENCCFG_IPCMINFO,
	V4L2_ENCCFG_ASPECT,
	V4L2_ENCCFG_SRCINFO,
	V4L2_ENCCFG_CODECFMT,
	V4L2_ENCCFG_VUIINFO,
	V4L2_ENCCFG_JPGFIXQP,
	V4L2_ENCCFG_RESTARTINTERVAL,
	V4L2_ENCCFG_SKIPMODE,
	V4L2_ENCCFG_AUD,
	V4L2_ENCCFG_LOOPFILTER,
	V4L2_ENCCFG_CHROMAQPOFFSET,
	V4L2_ENCCFG_IQPRANGE,
	V4L2_ENCCFG_PQPRANGE,
	V4L2_ENCCFG_MVRANGE,
	V4L2_ENCCFG_HEADERMODE,
	V4L2_ENCCFG_GOPCFG,
	V4L2_ENCCFG_VPXPARTITIONS,
	V4L2_ENCCFG_REFNO,
	V4L2_ENCCFG_VPXFILTERLVL,
	V4L2_ENCCFG_VPXFILTERSHARP,
	V4L2_ENCCFG_GOLDENPERIOD,
	V4L2_ENCCFG_TIER,
	V4L2_ENCCFG_REFRESH,
	V4L2_ENCCFG_TEMPORALID,
	V4L2_ENCCFG_STRONGSMOOTH,
	V4L2_ENCCFG_TMVP,
	V4L2_ENCCFG_STARTCODE,
	V4L2_ENCCFG_RESENDSPSPPS,
	V4L2_ENCCFG_JPGCODINGMODE,
	V4L2_ENCCFG_SECUREMODE,
	V4L2_ENCCFG_SCALEOUTPUT,
	V4L2_ENCCFG_NO,
};
/*unreferenced:
	V4L2_CID_MPEG_VIDEO_MAX_REF_PIC
	V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD_TYPE
	V4L2_CID_MPEG_VIDEO_VPX_GOLDEN_FRAME_SEL
	V4L2_CID_MPEG_VIDEO_HEVC_FRAME_RATE_RESOLUTION	? to framerate?
	V4L2_CID_MPEG_VIDEO_HEVC_MAX_NUM_MERGE_MV_MINUS1
	V4L2_CID_MPEG_VIDEO_HEVC_SIZE_OF_LENGTH_FIELD
*/
#define PARAM_UPDATE_BIT	31		//need flush param to daem
#define PARAM_CHECK_BIT	30		//need check value change before flush param to daemon

//config struct hdr: 0..11, id, 12..23 content size, 24..31 flag used by kernel
struct v4l2_enccfg_framerate {
	ulong head;
	s32 outputRateNumer;
	s32 outputRateDenom;
	s32 inputRateNumer;
	s32 inputRateDenom;
};

struct v4l2_enccfg_cropinfo {
	ulong head;
	s32 horOffsetSrc;
	s32 verOffsetSrc;
	s32 width;
	s32 height;
};

struct v4l2_enccfg_intrapicrate {
	ulong head;
	s32 intraPicRate;
};

struct v4l2_enccfg_gopsize {
	ulong head;
	u32 gopSize;
};

struct v4l2_enccfg_profile {
	ulong head;
	s32 profile;              /*main profile or main still picture profile*/
};

struct v4l2_enccfg_level {
	ulong head;
	s32 level;
};

struct v4l2_enccfg_bitrate {
	ulong head;
	u32 bitPerSecond;
};

struct v4l2_enccfg_qprange {
	ulong head;
	s32 qpMin;
	s32 qpMax;
};

struct v4l2_enccfg_bitratemode {
	ulong head;
	s32 bitrateMode;
	s32 hrdConformance;
	s32 tolMovingBitRate;
};

struct v4l2_enccfg_sliceinfo {
	ulong head;
	s32 multislice_mode;
	s32 sliceSize;      //multi slice
};

struct v4l2_enccfg_rcmode {
	ulong head;
	s32 picRc;
	s32 ctbRc;
};

struct v4l2_enccfg_qphdr {
	ulong head;
	s32 qpHdr;
};

struct v4l2_enccfg_qphdrip {
	ulong head;
	s32 qpHdrI;
	s32 qpHdrP;
	s32 qpHdrB;
};

struct v4l2_enccfg_rotation {
	ulong head;
	s32 rotation;
};

struct v4l2_enccfg_gdrduration {
	ulong head;
	s32 gdrDuration;
};

struct v4l2_enccfg_idrhdr {
	ulong head;
	u32 idrHdr;
};

struct v4l2_enccfg_entroypmode {
	ulong head;
	s32 enableCabac;      /* [0,1] H.264 entropy coding mode, 0 for CAVLC, 1 for CABAC */
};

struct v4l2_enccfg_cpbsize {
	ulong head;
	s32 cpbSize;
};

struct v4l2_enccfg_trans8x8 {
	ulong head;
	u32 transform8x8Enable;
};

struct v4l2_enccfg_ctrintrapred {
	ulong head;
	u32 constrained_intra_pred_flag;
};

struct v4l2_enccfg_forceidr {
	ulong head;
	//u32 force_idr;
};

struct v4l2_enccfg_roiinfo {
	ulong head;
	s32 roiAreaEnable[VSI_V4L2_MAX_ROI_REGIONS]; //8 roi for H2, 2 roi for H1
	s32 roiAreaTop[VSI_V4L2_MAX_ROI_REGIONS];
	s32 roiAreaLeft[VSI_V4L2_MAX_ROI_REGIONS];
	s32 roiAreaBottom[VSI_V4L2_MAX_ROI_REGIONS];
	s32 roiAreaRight[VSI_V4L2_MAX_ROI_REGIONS];
	s32 roiDeltaQp[VSI_V4L2_MAX_ROI_REGIONS];    //roiQp has higher priority than roiDeltaQp
	s32 roiQp[VSI_V4L2_MAX_ROI_REGIONS];    //only H2 use it
};

struct v4l2_enccfg_ipcminfo {
	ulong head;
	s32 pcm_loop_filter_disabled_flag;
	s32 ipcmAreaEnable[VSI_V4L2_MAX_IPCM_REGIONS];
	s32 ipcmAreaTop[VSI_V4L2_MAX_IPCM_REGIONS];    //ipcm area 1, 2
	s32 ipcmAreaLeft[VSI_V4L2_MAX_IPCM_REGIONS];
	s32 ipcmAreaBottom[VSI_V4L2_MAX_IPCM_REGIONS];
	s32 ipcmAreaRight[VSI_V4L2_MAX_IPCM_REGIONS];
};

struct v4l2_enccfg_aspect {
	ulong head;
	s32 enable;
	u32 sarWidth;
	u32 sarHeight;
};

struct v4l2_enccfg_srcinfo {
	ulong head;
	s32 lumWidthSrc;      //input width
	s32 lumHeightSrc;      //input height
	s32 inputFormat;  //input format
};

struct v4l2_enccfg_codecfmt {
	ulong head;
	s32 codecFormat;
};

struct v4l2_enccfg_vuiinfo {
	ulong head;
	u32 vuiVideoSignalTypePresentFlag;//1
	u32 vuiVideoFormat;               //default 5
	s32 videoRange;
	u32 vuiColorDescripPresentFlag;    //1 if elems below exist
	u32 vuiColorPrimaries;
	u32 vuiTransferCharacteristics;
	u32 vuiMatrixCoefficients;
	s32 colorSpace;
};

struct v4l2_enccfg_jpgfixqp {
	ulong head;
	s32 fixedQP;
};

struct v4l2_enccfg_restartinterval {
	ulong head;
	s32 restartInterval;
};

struct  v4l2_enccfg_skipmode {
	ulong head;
	s32 pictureSkip;
};

struct  v4l2_enccfg_aud {
	ulong head;
	s32 sendAud;
};

struct  v4l2_enccfg_loopfilter {
	ulong head;
	s32 disableDeblockingFilter;
	s32 tc_Offset;			//alpha
	s32 beta_Offset;		//beta
};

struct v4l2_enccfg_chromaqpoffset {
	ulong head;
	s32 chroma_qp_offset;
};

struct v4l2_enccfg_Iqprange {
	ulong head;
	s32 qpMinI;
	s32 qpMaxI;
};

struct v4l2_enccfg_Pqprange {
	ulong head;
	s32 qpMinPB;
	s32 qpMaxPB;
};

struct v4l2_enccfg_MVrange {
	ulong head;
	s32 meVertSearchRange;
};

struct v4l2_enccfg_headermode {
	ulong head;
	s32 headermode;
};

struct v4l2_enccfg_gopcfg {
	ulong head;
	s32 hierachy_enable;
	s32 hierachy_codingtype;
	s32 codinglayer;
	s32 codinglayerqp[MAX_LAYER_V4L2];
};

struct v4l2_enccfg_vpxpartitions {
	ulong head;
	s32 dctPartitions;
};

struct v4l2_enccfg_refno {
	ulong head;
	s32 refFrameAmount;
};

struct v4l2_enccfg_vpxfilterlvl {
	ulong head;
	s32 filterLevel;
};

struct v4l2_enccfg_vpxfiltersharp {
	ulong head;
	s32 filterSharpness;
};

struct v4l2_enccfg_goldenperiod {
	ulong head;
	s32 goldenPictureRate;
};

struct v4l2_enccfg_tier {
	ulong head;
	s32 tier;               /*main tier or high tier*/
};

struct v4l2_enccfg_refresh {
	ulong head;
	s32 refreshtype;
	s32 idr_interval;
};

struct v4l2_enccfg_temporalid {
	ulong head;
	s32 temporalId;
};

struct v4l2_enccfg_strongsmooth {
	ulong head;
	u32 strong_intra_smoothing_enabled_flag;
};

struct v4l2_enccfg_tmvp {
	ulong head;
	s32 enableTMVP;
};

struct v4l2_enccfg_startcode {
	ulong head;
	s32 streamType;
};

struct v4l2_enccfg_resendSPSPPS {
	ulong head;
	s32 resendSPSPPS;
};

struct v4l2_enccfg_jpgcodingmode {
	ulong head;
	s32 codingMode;
};

struct v4l2_enccfg_securemode {
	ulong head;
	s32 secureModeOn;
};

struct v4l2_enccfg_scaleoutput {
	ulong head;
	u32 scaleWidth;
	u32 scaleHeight;
	u32 scaleOutput;
	u32 scaleOutputFormat;	//0:YUV422, 1 yuv420SP
};

//only one of video and jpg content will be sent in a ctx, so no jpg structure here
#define MAX_ENC_PARAM_SIZE	\
	sizeof(struct v4l2_enccfg_framerate) \
	+ sizeof(struct v4l2_enccfg_cropinfo) \
	+ sizeof(struct v4l2_enccfg_intrapicrate)	\
	+ sizeof(struct v4l2_enccfg_gopsize) \
	+ sizeof(struct v4l2_enccfg_profile) \
	+ sizeof(struct v4l2_enccfg_level) \
	+ sizeof(struct v4l2_enccfg_bitrate) \
	+ sizeof(struct v4l2_enccfg_qprange) \
	+ sizeof(struct v4l2_enccfg_bitratemode) \
	+ sizeof(struct v4l2_enccfg_sliceinfo) \
	+ sizeof(struct v4l2_enccfg_rcmode) \
	+ sizeof(struct v4l2_enccfg_qphdr) \
	+ sizeof(struct v4l2_enccfg_qphdrip) \
	+ sizeof(struct v4l2_enccfg_rotation) \
	+ sizeof(struct v4l2_enccfg_gdrduration) \
	+ sizeof(struct v4l2_enccfg_idrhdr) \
	+ sizeof(struct v4l2_enccfg_entroypmode) \
	+ sizeof(struct v4l2_enccfg_cpbsize) \
	+ sizeof(struct v4l2_enccfg_trans8x8) \
	+ sizeof(struct v4l2_enccfg_ctrintrapred) \
	+ sizeof(struct v4l2_enccfg_forceidr) \
	+ sizeof(struct v4l2_enccfg_roiinfo) \
	+ sizeof(struct v4l2_enccfg_ipcminfo) \
	+ sizeof(struct v4l2_enccfg_aspect) \
	+ sizeof(struct v4l2_enccfg_srcinfo) \
	+ sizeof(struct v4l2_enccfg_codecfmt) \
	+ sizeof(struct v4l2_enccfg_vuiinfo) \
	+ sizeof(struct v4l2_enccfg_skipmode)	\
	+ sizeof(struct  v4l2_enccfg_aud)	\
	+ sizeof(struct v4l2_enccfg_loopfilter)		\
	+ sizeof(struct  v4l2_enccfg_chromaqpoffset)	\
	+ sizeof(struct v4l2_enccfg_Iqprange)	\
	+ sizeof(struct v4l2_enccfg_Pqprange)	\
	+ sizeof(struct v4l2_enccfg_MVrange)	\
	+ sizeof(struct v4l2_enccfg_headermode)	\
	+ sizeof(struct v4l2_enccfg_gopcfg)	\
	+ sizeof(struct v4l2_enccfg_vpxpartitions)	\
	+ sizeof(struct v4l2_enccfg_refno)	\
	+ sizeof(struct v4l2_enccfg_vpxfilterlvl)	\
	+ sizeof(struct v4l2_enccfg_vpxfiltersharp)	\
	+ sizeof(struct v4l2_enccfg_goldenperiod)	\
	+ sizeof(struct v4l2_enccfg_tier)	\
	+ sizeof(struct v4l2_enccfg_refresh)	\
	+ sizeof(struct v4l2_enccfg_temporalid)	\
	+ sizeof(struct v4l2_enccfg_strongsmooth)	\
	+ sizeof(struct v4l2_enccfg_tmvp)	\
	+ sizeof(struct v4l2_enccfg_startcode)	\
	+ sizeof(struct v4l2_enccfg_resendSPSPPS)	\
	+ sizeof(struct v4l2_enccfg_securemode)	\
	+ sizeof(struct v4l2_enccfg_scaleoutput)


struct v4l2_daemon_enc_params {
	struct v4l2_daemon_enc_buffers io_buffer;
	u32 param_num;		//number of valid parameters following
	u8  paramlist[MAX_ENC_PARAM_SIZE];
};

struct v4l2_daemon_dec_buffers {
	/*IO*/
	s32 inbufidx;	 //from v4l2 driver, don't modify it
	s32 outbufidx;	 //-1:invalid, other:valid.

	dma_addr_t busInBuf;
	u32 inBufSize;
	s32 inputFormat;  //input format
	s32 srcwidth;      //encode width
	s32 srcheight;      //encode height
//infer output
	dma_addr_t busOutBuf;	//for Y or YUV
	s32    OutBufSize;
	dma_addr_t busOutBufUV;
	s32    OutUVBufSize;
	s32 outBufFormat;
	s32 output_width;
	s32 output_height;
	s32 output_wstride;
	s32 output_hstride;
	s32 outputPixelDepth;

	dma_addr_t rfc_luma_offset;
	dma_addr_t rfc_chroma_offset;

	u32 bytesused;	//valid bytes in buffer from user app.
	s64 timestamp;

	s32 no_reordering_decoding;
	s32 securemode_on;
	u32 compressor_mode;
	s32 divx_version;

	long addr_offset;	//gap between VPU and CPU bus address, def 0
};

//stub struct
struct v4l2_daemon_dec_pp_cfg {
	u32 x;
	u32 y;
	u32 width;
	u32 height;
};

struct v4l2_vpu_hdr10_meta {
	u32 hasHdr10Meta;
	u32 redPrimary[2];
	u32 greenPrimary[2];
	u32 bluePrimary[2];
	u32 whitePoint[2];
	u32 maxMasteringLuminance;
	u32 minMasteringLuminance;
	u32 maxContentLightLevel;
	u32 maxFrameAverageLightLevel;
};

struct v4l2_daemon_dec_info {
	u32 frame_width;
	u32 frame_height;
	u32 bit_depth;
	struct {
		u32 left;
		u32 top;
		u32 width;
		u32 height;
	} visible_rect;
	u32 needed_dpb_nums;
	u32 dpb_buffer_size;
	uint32_t pic_wstride;
	struct v4l2_daemon_dec_pp_cfg pp_params;
	u32 colour_description_present_flag;
	u32 matrix_coefficients;
	u32 colour_primaries;
	u32 transfer_characteristics;
	u32 video_range;
	enum vsi_v4l2dec_pixfmt src_pix_fmt;
	struct v4l2_vpu_hdr10_meta vpu_hdr10_meta;
};

struct v4l2_daemon_extra_info {
  uint32_t offset[3]; //offset of each plane
  uint8_t bit_depth ; //pixel bit depth
  uint8_t is_interlaced;/**< is sequence interlaced */
  uint8_t fields_in_picture;/**< how many fields in picture:0-no field;1-one field;2-two fields */
  uint8_t top_field_first;/**< 1-top field firs; 0-bottom field first */
  uint8_t height_align_factor; //height align factor
  uint8_t compressed; //whether the data is compressed (1) or not (0)
  uint8_t dec400_tile_mode[3];//dec400 compressing tile mode of each plane
};

struct v4l2_daemon_pic_info {
	u32 width;
	u32 height;
	u32 crop_left;
	u32 crop_top;
	u32 crop_width;
	u32 crop_height;
	uint32_t pic_wstride;
	struct v4l2_daemon_extra_info extra_info;
};

struct v4l2_daemon_dec_resochange_params {
	struct v4l2_daemon_dec_buffers io_buffer;
	struct v4l2_daemon_dec_info dec_info;
};

struct v4l2_daemon_dec_pictureinfo_params {
	//v4l2_daemon_dec_buffers io_buffer;
	struct v4l2_daemon_pic_info pic_info;
};

struct v4l2_daemon_dec_params {
	union {
		struct v4l2_daemon_dec_buffers io_buffer;
		struct v4l2_daemon_dec_resochange_params dec_info;
		struct v4l2_daemon_dec_pictureinfo_params pic_info;
	};
//	struct TBCfg general;
};

struct vsi_v4l2_msg_hdr {
	s32 size;
	s32 error;
	ulong seq_id;
	ulong inst_id;
	enum v4l2_daemon_cmd_id cmd_id;
	enum v4l2_daemon_codec_fmt codec_fmt;
	s32 param_type;
};

struct vsi_v4l2_msg {
	s32 size;
	s32 error;
	ulong seq_id;
	ulong inst_id;
	enum v4l2_daemon_cmd_id cmd_id;
	enum v4l2_daemon_codec_fmt codec_fmt;
	u32 param_type;
	// above part must be identical to vsi_v4l2_msg_hdr

	union {
		struct v4l2_daemon_enc_params enc_params;
		struct v4l2_daemon_dec_params dec_params;
	} params;
};

#endif	//#ifndef VSI_V4L2_H


