/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright 2018-2020 NXP
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef _UAPI__LINUX_IMX_VPU_H
#define _UAPI__LINUX_IMX_VPU_H

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

/*imx v4l2 controls & extension controls*/

//ctrls & extension ctrls definitions
#define VSI_V4L2_CID_NORMAL_BASE			(V4L2_CID_USER_BASE + 0x1080)
#define V4L2_CID_NON_FRAME			(VSI_V4L2_CID_NORMAL_BASE + 0)
#define V4L2_CID_DIS_REORDER		(VSI_V4L2_CID_NORMAL_BASE + 1)
#define V4L2_CID_ROI_COUNT			(VSI_V4L2_CID_NORMAL_BASE + 2)
#define V4L2_CID_ROI				(VSI_V4L2_CID_NORMAL_BASE + 3)
#define V4L2_CID_IPCM_COUNT			(VSI_V4L2_CID_NORMAL_BASE + 4)
#define V4L2_CID_IPCM				(VSI_V4L2_CID_NORMAL_BASE + 5)
#define V4L2_CID_HDR10META			(VSI_V4L2_CID_NORMAL_BASE + 6)
#define V4L2_CID_SECUREMODE			(VSI_V4L2_CID_NORMAL_BASE + 7)
#define V4L2_CID_COMPRESSOR_CAPS	(VSI_V4L2_CID_NORMAL_BASE + 8)
#define V4L2_CID_COMPRESSOR_MODE	(VSI_V4L2_CID_NORMAL_BASE + 9)
#define V4L2_CID_DIVX_VERSION		(VSI_V4L2_CID_NORMAL_BASE + 10)
#define V4L2_CID_ADDR_OFFSET		(VSI_V4L2_CID_NORMAL_BASE + 11)
#define V4L2_CID_ENC_SCALE_INFO	(VSI_V4L2_CID_NORMAL_BASE + 12)

//extension to V4L2_CID_MPEG_VIDEO_BITRATE_MODE based on ctrl SW
#define V4L2_MPEG_VIDEO_BITRATE_MODE_CVBR		3
#define V4L2_MPEG_VIDEO_BITRATE_MODE_ABR		4
#define V4L2_MPEG_VIDEO_BITRATE_MODE_CRF		5
//endof extension to V4L2_CID_MPEG_VIDEO_BITRATE_MODE

/**  missing symbols for old linux version ***/
//symbols newin 5.10
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_1_0
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_1_0			0
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_1_1
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_1_1			1
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_2_0
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_2_0			2
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_2_1
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_2_1			3
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_3_0
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_3_0			4
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_3_1
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_3_1			5
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_4_0
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_4_0			6
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_4_1
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_4_1			7
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_5_0
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_5_0			8
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_5_1
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_5_1			9
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_5_2
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_5_2			10
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_6_0
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_6_0			11
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_6_1
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_6_1			12
#endif
#ifndef V4L2_MPEG_VIDEO_VP9_LEVEL_6_2
#define  V4L2_MPEG_VIDEO_VP9_LEVEL_6_2			13
#endif

#define VSI_V4L2_CID_MISSING_BASE			(V4L2_CID_USER_BASE + 0x1000)
#ifndef V4L2_CID_MPEG_VIDEO_VP9_LEVEL
#define V4L2_CID_MPEG_VIDEO_VP9_LEVEL			(VSI_V4L2_CID_MISSING_BASE + 11)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_CONSTANT_QUALITY
#define V4L2_CID_MPEG_VIDEO_CONSTANT_QUALITY		(VSI_V4L2_CID_MISSING_BASE + 12)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE
#define V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE		(VSI_V4L2_CID_MISSING_BASE + 13)
#endif
//symbols new in v5.11
#ifndef V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MAX_QP
#define V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MAX_QP	(VSI_V4L2_CID_MISSING_BASE + 14)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MIN_QP
#define V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MIN_QP        (VSI_V4L2_CID_MISSING_BASE + 15)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MAX_QP
#define V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MAX_QP        (VSI_V4L2_CID_MISSING_BASE + 16)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MIN_QP
#define V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MIN_QP        (VSI_V4L2_CID_MISSING_BASE + 17)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MAX_QP
#define V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MAX_QP        (VSI_V4L2_CID_MISSING_BASE + 18)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MIN_QP
#define V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MIN_QP        (VSI_V4L2_CID_MISSING_BASE + 19)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MAX_QP
#define V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MAX_QP        (VSI_V4L2_CID_MISSING_BASE + 20)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_AU_DELIMITER
#define V4L2_CID_MPEG_VIDEO_AU_DELIMITER				(VSI_V4L2_CID_MISSING_BASE + 21)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MIN_QP
#define V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MIN_QP	(VSI_V4L2_CID_MISSING_BASE + 22)
#endif
//symbols new in v5.15
#ifndef V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD
#define V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD	(VSI_V4L2_CID_MISSING_BASE + 23)
#endif
/**  endof missing symbols for old linux version ***/


#define V4L2_MAX_ROI_REGIONS		(8)
struct v4l2_enc_roi_param {
	struct v4l2_rect rect;
	__u32 enable;
	__s32 qp_delta;
	__u32 reserved[2];
};

struct v4l2_enc_roi_params {
	__u32 num_roi_regions;
	struct v4l2_enc_roi_param roi_params[V4L2_MAX_ROI_REGIONS];
	__u32 config_store;
	__u32 reserved[2];
};

#define V4L2_MAX_IPCM_REGIONS		2
struct v4l2_enc_ipcm_param {
	struct v4l2_rect rect;
	__u32 enable;
	__u32 reserved[2];
};
struct v4l2_enc_ipcm_params {
	__u32 num_ipcm_regions;
	struct v4l2_enc_ipcm_param ipcm_params[V4L2_MAX_IPCM_REGIONS];
	__u32 config_store;
	__u32 reserved[2];
};

struct v4l2_hdr10_meta {
	__u32 hasHdr10Meta;
	__u32 redPrimary[2];
	__u32 greenPrimary[2];
	__u32 bluePrimary[2];
	__u32 whitePoint[2];
	__u32 maxMasteringLuminance;
	__u32 minMasteringLuminance;
	__u32 maxContentLightLevel;
	__u32 maxFrameAverageLightLevel;
};

//work with V4L2_CID_ENC_SCALE_INFO
struct v4l2_enc_scaleinfo {
	__u32 scaleWidth;
	__u32 scaleHeight;
	__u32 scaleOutput;			//if enable scale output
	__u32 scaleOutputFormat;		//0:yuv422, 1 YUV420SP
};

/*imx v4l2 command*/
#define V4L2_DEC_CMD_IMX_BASE		(0x08000000)
#define V4L2_DEC_CMD_RESET			(V4L2_DEC_CMD_IMX_BASE + 1)

/*imx v4l2 event*/
//error happened in dec/enc
#define V4L2_EVENT_CODEC_ERROR		(V4L2_EVENT_PRIVATE_START + 1)
//frame loss in dec/enc
#define V4L2_EVENT_SKIP				(V4L2_EVENT_PRIVATE_START + 2)
//crop area change in dec, not reso change
#define V4L2_EVENT_CROPCHANGE		(V4L2_EVENT_PRIVATE_START + 3)
//some options can't be handled by codec, so might be ignored or updated. But codec could go on.
#define V4L2_EVENT_INVALID_OPTION	(V4L2_EVENT_PRIVATE_START + 4)

/*imx v4l2 warning msg, attached with event V4L2_EVENT_INVALID_OPTION*/
enum {
	UNKONW_WARNING = -1,		//not known warning type
	ROIREGION_NOTALLOW,		//(part of)roi region can not work with media setting and be ignored by enc
	IPCMREGION_NOTALLOW,		//(part of)ipcm region can not work with media setting and be ignored by enc
	LEVEL_UPDATED,				//current level cant't work with media setting and be updated by enc

	INTRAREGION_NOTALLOW,		//intra region can not work with media setting and be ignored by enc
	GOPSIZE_NOTALLOW,			//wrong gop size (B frame numbers) setting and be updated by enc
};

/*used to explain struct v4l2_event.u.data in app side*/
struct v4l2_extra_info {
  __u32 offset[3]; //offset of each plane
  __u8 bit_depth; //pixel bit depth
  __u8 is_interlaced;/**< is sequence interlaced */
  __u8 fields_in_picture;/**< how many fields in picture:0-no field;1-one field;2-two fields */
  __u8 top_field_first;/**< 1-top field firs; 0-bottom field first */
  __u8 height_align_factor; //height align factor
  __u8 compressed; //whether the data is compressed (1) or not (0)
  __u8 dec400_tile_mode[3];//dec400 compressing tile mode of each plane
};

enum hw_compressor_format {
  COMPRESSOR_NONE = 0,
  COMPRESSOR_DEC400 = 1,
};

enum dec400_mode {
  DEC400_MODE_STREAM_BYPASS = (0 << 8),
  DEC400_MODE_WORKING = (1 << 8),
};

/* imx v4l2 formats */
/*raw formats*/
#define V4L2_PIX_FMT_BGR565		v4l2_fourcc('B', 'G', 'R', 'P') /* 16  BGR-5-6-5     */
#define V4L2_PIX_FMT_NV12X			v4l2_fourcc('N', 'V', 'X', '2') /* Y/CbCr 4:2:0 for 10bit  */
#define V4L2_PIX_FMT_DTRC			v4l2_fourcc('D', 'T', 'R', 'C') /* 8bit tile output, uncompressed */
#define V4L2_PIX_FMT_P010			v4l2_fourcc('P', '0', '1', '0')	/*ms p010, data stored in upper 10 bits of 16 */
#define V4L2_PIX_FMT_TILEX			v4l2_fourcc('D', 'T', 'R', 'X') /* 10 bit tile output, uncompressed */
#define V4L2_PIX_FMT_RFC			v4l2_fourcc('R', 'F', 'C', '0') /* 8bit tile output, with rfc*/
#define V4L2_PIX_FMT_RFCX			v4l2_fourcc('R', 'F', 'C', 'X') /* 10 bit tile output, with rfc */
#define V4L2_PIX_FMT_411SP			v4l2_fourcc('4', '1', 'S', 'P') /* YUV 411 Semi planar */
#ifndef	V4L2_PIX_FMT_NV12_4L4
#define V4L2_PIX_FMT_NV12_4L4		v4l2_fourcc('V', 'T', '1', '2') /* 12 Y/CbCr 4:2:0 4x4 tiles */
#endif
#ifndef	V4L2_PIX_FMT_P010_4L4
#define V4L2_PIX_FMT_P010_4L4		v4l2_fourcc('T', '0', '1', '0') /* 12 Y/CbCr 4:2:0 10-bit 4x4 macroblocks */
#endif
/*codec format*/
#define V4L2_PIX_FMT_AV1			v4l2_fourcc('A', 'V', '1', '0')	/* av1 */
#define V4L2_PIX_FMT_RV				v4l2_fourcc('R', 'V', '0', '0')	/* rv */
#define V4L2_PIX_FMT_AVS			v4l2_fourcc('A', 'V', 'S', '0')	/* avs */
#define V4L2_PIX_FMT_AVS2			v4l2_fourcc('A', 'V', 'S', '2')	/* avs2 */
#define V4L2_PIX_FMT_VP6			v4l2_fourcc('V', 'P', '6', '0') /* vp6 */
#define V4L2_PIX_FMT_VP7			v4l2_fourcc('V', 'P', '7', '0') /* vp7 */
#define V4L2_PIX_FMT_WEBP			v4l2_fourcc('W', 'E', 'B', 'P') /* webp */
#define V4L2_PIX_FMT_DIVX			v4l2_fourcc('D', 'I', 'V', 'X') /* DIVX */
#define V4L2_PIX_FMT_SORENSON		v4l2_fourcc('S', 'R', 'S', 'N') /* sorenson */
/*codec formats*/
#endif	//#ifndef _UAPI__LINUX_IMX_VPU_H

