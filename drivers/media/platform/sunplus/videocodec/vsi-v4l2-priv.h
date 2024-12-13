/*
 *    VSI V4L2 kernel driver private header file.
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

#ifndef VSI_V4L2_PRIV_H
#define VSI_V4L2_PRIV_H

#include <linux/version.h>
#include <linux/v4l2-controls.h>
#include "hantro_vpu.h"
#include "vsi-v4l2.h"
#include <linux/math64.h>

#define CTX_SEQID_UPLIMT 0x7FFFFFFF
#define CTX_ARRAY_ID(ctxid)	((ctxid) & 0xFFFFFFFF)
#define CTX_SEQ_ID(ctxid)	((ctxid) >> 32)

#define VIDEO_NODE_OFFSET 64

#define MIN_FRAME_4ENC	1

#define MAX_MIN_BUFFERS_FOR_CAPTURE	16
#define MAX_MIN_BUFFERS_FOR_OUTPUT	16

#define ENC_EXTRA_HEADER_SIZE 1024

#define DEFAULT_GOP_SIZE	1
#define DEFAULT_INTRA_PIC_RATE	30
#define DEFAULT_QP			30

#define DEFAULT_PIXELDEPTH		10		//set outputPixelDepth to this will make daemon return default pixeldepth

#define CTRL_IDX_MIN	0
#define CTRL_IDX_MAX	1
#define CTRL_IDX_DEF	2

#if KERNEL_VERSION(5, 5, 0) > LINUX_VERSION_CODE
#define VSI_DEVTYPE	VFL_TYPE_GRABBER
#else
#define VSI_DEVTYPE	VFL_TYPE_VIDEO
#endif

/* declarations */
extern int vsi_kloglvl;

//compound type for extension ctrls
#define VSI_V4L2_CMPTYPE_ROI				(V4L2_CTRL_COMPOUND_TYPES + 100)
#define VSI_V4L2_CMPTYPE_IPCM				(V4L2_CTRL_COMPOUND_TYPES + 101)
#define VSI_V4L2_CMPTYPE_HDR10META		(V4L2_CTRL_COMPOUND_TYPES + 102)
#define VSI_V4L2_CMPTYPE_ENCSCALEOUT		(V4L2_CTRL_COMPOUND_TYPES + 103)

enum {
	LOGLVL_VERBOSE = 0,	//log all
	LOGLVL_FLOW,			//log all except cmd exchange with daemon
	LOGLVL_CONFIG,			// log ctrl/config info, mostly at beginning
	LOGLVL_BRIEF,			// log critical point (streamon/off, etc)
	LOGLVL_WARNING,		// log warning msg
	LOGLVL_ERROR,			// log error
};

#define v4l2_klog(lvl, fmt, ...) {\
	if (lvl == LOGLVL_ERROR)	\
		pr_err(fmt, ##__VA_ARGS__);	\
	else if (lvl >= vsi_kloglvl)		\
		pr_info(fmt, ##__VA_ARGS__);	\
}

#define is_vsi_ctrl(x) ((V4L2_CTRL_ID2WHICH(x) == V4L2_CTRL_CLASS_USER) && \
			  V4L2_CTRL_DRIVER_PRIV(x))

#define flag_updateparam(member)	\
	set_bit(PARAM_UPDATE_BIT, &ctx->mediacfg.m_encparams.member.head);
#define flag_checkparam(member)	\
	set_bit(PARAM_CHECK_BIT, &ctx->mediacfg.m_encparams.member.head);

/**  missing symbols for old linux version ***/
#ifndef V4L2_MPEG_VIDEO_H264_LEVEL_5_2
#define V4L2_MPEG_VIDEO_H264_LEVEL_5_2	16
#endif

/****************************************************************/
/* Extension of color macros copied from NXP ticket 464. In new kernel version they may be removed */

#define V4L2_COLORSPACE_GENERIC_FILM  (V4L2_COLORSPACE_DCI_P3+1)
#define V4L2_COLORSPACE_ST428			(V4L2_COLORSPACE_DCI_P3+2)

#define V4L2_XFER_FUNC_LINEAR			(V4L2_XFER_FUNC_SMPTE2084+1)
#define V4L2_XFER_FUNC_GAMMA22		(V4L2_XFER_FUNC_SMPTE2084+2)
#define V4L2_XFER_FUNC_GAMMA28		(V4L2_XFER_FUNC_SMPTE2084+3)
#define V4L2_XFER_FUNC_HLG				(V4L2_XFER_FUNC_SMPTE2084+4)
#define V4L2_XFER_FUNC_XVYCC			(V4L2_XFER_FUNC_SMPTE2084+5)
#define V4L2_XFER_FUNC_BT1361			(V4L2_XFER_FUNC_SMPTE2084+6)
#define V4L2_XFER_FUNC_ST428			(V4L2_XFER_FUNC_SMPTE2084+7)

#define V4L2_YCBCR_ENC_BT470_6M		(V4L2_YCBCR_ENC_SMPTE240M+1)

/*V4L2 status*/
enum CTX_STATUS {
	VSI_STATUS_INIT = 0,	/*init is a public state for dec and enc*/
	ENC_STATUS_ENCODING,
	ENC_STATUS_DRAINING,
	ENC_STATUS_STOPPED,
	ENC_STATUS_EOS,

	DEC_STATUS_DECODING,
	DEC_STATUS_DRAINING,
	DEC_STATUS_STOPPED,
	DEC_STATUS_SEEK,
	DEC_STATUS_CAPSETUP,
	DEC_STATUS_ENDSTREAM,
};

struct vsi_v4l2_mem_info_internal {
	ulong size;
	dma_addr_t  busaddr;
	void *vaddr;
	struct page *pageaddr;
};

struct vsi_video_fmt {
	char *name;
	u32 fourcc;	//V4L2 video format defines
	s32 enc_fmt;	//our own enc video format defines
	s32 dec_fmt;	//our own dec video format defines
	u32 flag;
	u32 num_planes;
};

struct vsi_v4l2_encparams {
	struct v4l2_enccfg_framerate m_framerate;
	struct v4l2_enccfg_cropinfo m_cropinfo;
	struct v4l2_enccfg_intrapicrate m_intrapicrate;
	struct v4l2_enccfg_gopsize m_gopsize;
	struct v4l2_enccfg_profile m_profile;
	struct v4l2_enccfg_level m_level;
	struct v4l2_enccfg_bitrate m_bitrate;
	struct v4l2_enccfg_qprange m_qprange;
	struct v4l2_enccfg_bitratemode m_bitratemode;
	struct v4l2_enccfg_sliceinfo m_sliceinfo;
	struct v4l2_enccfg_rcmode m_rcmode;
	struct v4l2_enccfg_qphdr m_qphdr;
	struct v4l2_enccfg_qphdrip m_qphdrip;
	struct v4l2_enccfg_rotation m_rotation;
	struct v4l2_enccfg_gdrduration m_gdrduration;
	struct v4l2_enccfg_idrhdr m_idrhdr;
	struct v4l2_enccfg_entroypmode m_entropymode;
	struct v4l2_enccfg_cpbsize m_cpbsize;
	struct v4l2_enccfg_trans8x8 m_trans8x8;
	struct v4l2_enccfg_ctrintrapred m_ctrintrapred;
	struct v4l2_enccfg_forceidr m_forceidr;
	struct v4l2_enccfg_roiinfo m_roiinfo;
	struct v4l2_enccfg_ipcminfo m_ipcminfo;
	struct v4l2_enccfg_aspect m_aspect;
	struct v4l2_enccfg_srcinfo m_srcinfo;
	struct v4l2_enccfg_codecfmt m_codecfmt;
	struct v4l2_enccfg_vuiinfo m_vuiinfo;
	struct v4l2_enccfg_jpgfixqp m_jpgfixqp;
	struct v4l2_enccfg_restartinterval m_restartinterval;
	struct  v4l2_enccfg_skipmode m_skipmode;
	struct  v4l2_enccfg_aud m_aud;
	struct v4l2_enccfg_loopfilter m_loopfilter;
	struct v4l2_enccfg_chromaqpoffset m_chromaqpoffset;
	struct v4l2_enccfg_Iqprange m_iqprange;
	struct v4l2_enccfg_Pqprange m_pqprange;
	struct v4l2_enccfg_MVrange m_mvrange;
	struct v4l2_enccfg_headermode m_headermode;
	struct v4l2_enccfg_gopcfg m_gopcfg;
	struct v4l2_enccfg_vpxpartitions m_vpxpartitions;
	struct v4l2_enccfg_refno m_refno;
	struct v4l2_enccfg_vpxfilterlvl m_vpxfilterlvl;
	struct v4l2_enccfg_vpxfiltersharp m_vpxfiltersharp;
	struct v4l2_enccfg_goldenperiod m_goldenperiod;
	struct v4l2_enccfg_tier m_tier;
	struct v4l2_enccfg_refresh m_refresh;
	struct v4l2_enccfg_temporalid m_temporalid;
	struct v4l2_enccfg_strongsmooth m_strongsmooth;
	struct v4l2_enccfg_tmvp m_tmvp;
	struct v4l2_enccfg_startcode m_startcode;
	struct v4l2_enccfg_resendSPSPPS m_resendSPSPPS;
	struct v4l2_enccfg_jpgcodingmode m_jpgcodingmode;
	struct v4l2_enccfg_securemode m_securemode;
	struct v4l2_enccfg_scaleoutput m_scaleoutput;
};

struct vsi_v4l2_mediacfg {

	/*from ctrls setting*/
	//unsigned int gopsize;
	//move to encparams.specific.enc_h26x_cmd.gopSize
	//unsigned int avcprofile;
	//move to encparams.specific.enc_h26x_cmd.profile

	/* from set format */
	//unsigned int width;
	//move to encparams.general.width
	//unsigned int height;
	//move to encparams.general.height
	//unsigned int pixelformat;
	//move to encparams.general.inputFormat
	u32 field;		/* enum v4l2_field */
	u32 srcplanes;
	u32 dstplanes;
	u32 bytesperline;	/* for padding, zero if unused */
	u32 sizeimagesrc[VB2_MAX_PLANES];	/*this is for input plane size*/
	u32 sizeimagedst[VB2_MAX_PLANES];	/*this is for output plane size*/
	u32 colorspace;	/* enum v4l2_colorspace */
	u32 flags;		/* format flags (V4L2_PIX_FMT_FLAG_*) */
	u32 quantization;	/* enum v4l2_quantization */
	u32 xfer_func;	/* enum v4l2_xfer_func */
	u32 minbuf_4capture;
	u32 minbuf_4output;
	u32 infmt_fourcc;
	u32 outfmt_fourcc;
	u32 src_pixeldepth;	//dec only
	u32 orig_dpbsize;	//dec only

	/*save enc info locally before sending to daemon to save some buffer*/
	s32 profile_h264;
	s32 profile_hevc;
	s32 profile_vp8;
	s32 profile_vp9;
	s32 profile_av1;
	s32 avclevel;
	s32 hevclevel;
	s32 vp9level;
	s32 av1level;
	struct v4l2_enc_roi_params roiinfo;
	struct v4l2_enc_ipcm_params ipcminfo;
	s32 qpMin_h26x;
	s32 qpMax_h26x;
	s32 qpMin_vpx;
	s32 qpMax_vpx;
	s32 qpHdrI_h26x;  // for 264/5 I frame QP
	s32 qpHdrP_h26x;  // for 264/5 P frame PQ
	s32 qpHdrI_vpx;  // for vpx I frame QP
	s32 qpHdrP_vpx;  // for vpx P frame PQ
	s32 vpx_PRefN;
	s32 hevc_PRefN;
	s32 disableDeblockingFilter_h264;
	s32 disableDeblockingFilter_hevc;
	/* end of save enc info locally before sending to daemon to save some buffer*/

	/*internal storage*/
	union {
		struct vsi_v4l2_encparams m_encparams;
		struct v4l2_daemon_dec_params decparams;
	};
	struct v4l2_daemon_dec_params decparams_bkup;
	s32 minbuf_4output_bkup;
	s32 sizeimagedst_bkup;
};

struct vsi_v4l2_device {
	struct v4l2_device v4l2_dev;
	struct platform_device *pdev;
	struct device *dev;
	struct video_device *venc;
	struct video_device *vdec;
	struct mutex lock;
	struct mutex irqlock;
};

struct vsi_vpu_buf {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

struct vsi_queued_buf {
	struct v4l2_buffer qb;
	struct list_head list;
	struct v4l2_plane planes[VB2_MAX_PLANES];
};

struct cropinfo {
	u32 frame_width;
	u32 frame_height;
	u32 left;
	u32 top;
	u32 width;
	u32 height;
	u32 pic_wstride;
	struct v4l2_daemon_extra_info extra_info;
	struct cropinfo *next;
};

/*ctx flags*/
#define CTX_FLAG_ENC				(0 << 0)
#define CTX_FLAG_DEC				(1 << 0)

enum {
	CTX_FLAG_PRE_DRAINING_BIT = 1,	//decoder_cmd stop has comes
	CTX_FLAG_ENDOFSTRM_BIT,			// got last flag received from daemon
	CTX_FLAG_DAEMONLIVE_BIT,			// has send msg to daemon (so daemon has live instance for this ctx
 	CTX_FLAG_FORCEIDR_BIT,			// force idr invoked
	CTX_FLAG_SRCCHANGED_BIT,			// src change has come from daemon
	CTX_FLAG_DELAY_SRCCHANGED_BIT,	// src change has come from daemon	 but not sent to app
	CTX_FLAG_SRCBUF_BIT,				// if any src buf comes from last OUTPUT off or INIT
	CTX_FLAG_ENC_FLUSHBUF,				// if any src buf comes from last OUTPUT off or INIT
	CTX_FLAG_CAPTUREOFFDONE,			// daemon finish handling capoff
	CTX_FLAG_OUTPUTOFFDONE,				// daemon finish handling outputoff
};

/* flag for decoder buffer*/
enum {
	BUF_FLAG_QUEUED = 0,		/*buf queued from app*/
	BUF_FLAG_DONE,			/*buf returned from daemon*/
	BUF_FLAG_CROPCHANGE,		/*crop area update not sent to app but buffed */
};

struct vsi_v4l2_ctx {
	struct v4l2_fh fh;
	struct vsi_v4l2_device *dev;
	ulong ctxid;
	struct mutex ctxlock;

	s32 status;		/*hold current status*/
	s32 error;
	ulong flag;

	struct vb2_queue input_que;
	struct list_head input_list;
	struct vb2_queue output_que;
	struct list_head output_list;
	ulong vbufflag[VIDEO_MAX_FRAME];
	ulong srcvbufflag[VIDEO_MAX_FRAME];

	u32 rfc_luma_offset[VIDEO_MAX_FRAME];
	u32 rfc_chroma_offset[VIDEO_MAX_FRAME];
	s32 queued_srcnum;
	s32 buffed_capnum;
	s32 buffed_cropcapnum;
	u32 lastcapbuffer_idx;	//latest received capture buffer index

	struct vsi_v4l2_mediacfg mediacfg;

	struct v4l2_ctrl_handler ctrlhdl;
	wait_queue_head_t retbuf_queue;
	wait_queue_head_t capoffdone_queue;

	u64	frameidx;
	s64	addr_offset;

	atomic_t srcframen;
	atomic_t dstframen;
	struct cropinfo *crophead;
	struct cropinfo *croptail;

	struct completion memcpy_done;
	int dma_result;
	int comp_init;

	int exp_fd[VIDEO_MAX_FRAME];	//export fd from other driver

	bool dma_remap;
};

int vsi_v4l2_release(struct file *filp);
void vsi_remove_ctx(struct vsi_v4l2_ctx *ctx);
struct vsi_v4l2_ctx *vsi_create_ctx(void);
void vsi_set_ctx_error(struct vsi_v4l2_ctx *ctx, s32 error);
void wakeup_ctxqueues(void);
int vsi_v4l2_reset_ctx(struct vsi_v4l2_ctx *ctx);
int vsi_v4l2_send_reschange(struct vsi_v4l2_ctx *ctx);
int vsi_v4l2_notify_reschange(struct vsi_v4l2_msg *pmsg);
int vsi_v4l2_handle_warningmsg(struct vsi_v4l2_msg *pmsg);
int vsi_v4l2_handle_streamoffdone(struct vsi_v4l2_msg *pmsg);
int vsi_v4l2_handle_cropchange(struct vsi_v4l2_msg *pmsg);
int vsi_v4l2_bufferdone(struct vsi_v4l2_msg *pmsg);
void vsi_v4l2_sendeos(struct vsi_v4l2_ctx *ctx);
int vsi_v4l2_handleerror(unsigned long ctxtid, int error);
int vsi_v4l2_handle_picconsumed(struct vsi_v4l2_msg *pmsg);
struct video_device *vsi_v4l2_probe_enc(
	struct platform_device *pdev,
	struct vsi_v4l2_device *vpu);
void vsi_v4l2_release_enc(struct video_device *venc);
struct video_device *vsi_v4l2_probe_dec(struct platform_device *pdev, struct vsi_v4l2_device *vpu);
void vsi_v4l2_release_dec(struct video_device *vdec);

u64 vsi_v4l2_getbandwidth(void);
int vsiv4l2_initdaemon(void);
void vsiv4l2_cleanupdaemon(void);
int vsi_clear_daemonmsg(int instid);
int vsiv4l2_execcmd(
	struct vsi_v4l2_ctx *ctx,
	enum v4l2_daemon_cmd_id id,
	void *args);
int vsi_v4l2_addinstance(pid_t *ppid);
int vsi_v4l2_quitinstance(void);
int vsi_v4l2_daemonalive(void);

void vsi_dec_updatevui(struct v4l2_daemon_dec_info *src, struct v4l2_daemon_dec_info *dst);
void vsi_dec_getvui(struct v4l2_format *v4l2fmt, struct v4l2_daemon_dec_info *decinfo);
void vsi_enum_encfsize(struct v4l2_frmsizeenum *f, u32 pixel_format);
void vsi_enum_decfsize(struct v4l2_frmsizeenum *f, u32 pixel_format);
void vsiv4l2_initcfg(struct vsi_v4l2_ctx *ctx);
int vsi_set_Level(struct vsi_v4l2_ctx *ctx, int type, int level);
int vsi_get_level(struct vsi_v4l2_ctx *ctx);
int vsiv4l2_setfmt(struct vsi_v4l2_ctx *ctx, struct v4l2_format *fmt);
int vsiv4l2_getfmt(struct vsi_v4l2_ctx *ctx, struct v4l2_format *fmt);
void vsi_v4l2_update_decfmt(struct vsi_v4l2_ctx *ctx);
int vsiv4l2_buffer_config(
	struct vsi_v4l2_ctx *ctx,
	int type,
	unsigned int *nbuffers,
	unsigned int *nplanes,
	unsigned int sizes[]
);

const struct vb2_mem_ops *get_vsi_mmop(void);
struct vsi_video_fmt *vsi_find_format(struct vsi_v4l2_ctx *ctx, struct v4l2_format *f);
struct vsi_video_fmt *vsi_enum_dec_format(int idx, int braw, struct vsi_v4l2_ctx *ctx);
struct vsi_video_fmt *vsi_enum_encformat(int idx, int braw);
int vsi_set_profile(struct vsi_v4l2_ctx *ctx, int type, int profile);
void vsiv4l2_set_hwinfo(struct vsi_v4l2_dev_info *hwinfo);
struct vsi_v4l2_dev_info *vsiv4l2_get_hwinfo(void);
int vsiv4l2_setROI(struct vsi_v4l2_ctx *ctx, void *params);
int vsiv4l2_configScaleOutput(struct vsi_v4l2_ctx *ctx, void *params);
int vsiv4l2_setIPCM(struct vsi_v4l2_ctx *ctx, void *params);
int vsiv4l2_getROIcount(void);
int vsiv4l2_getIPCMcount(void);
s32 vsi_convertROI(struct vsi_v4l2_ctx *ctx);
s32 vsi_convertIPCM(struct vsi_v4l2_ctx *ctx);
int vsiv4l2_verifycrop(struct vsi_v4l2_ctx *ctx, struct v4l2_selection *s);
void vsi_v4l2_update_ctrlcfg(struct v4l2_ctrl_config *cfg);
int vsiv4l2_has_jpgcodingmode(s32 codingmode);
int vsiv4l2_has_loopfilter(struct vsi_v4l2_ctx *ctx);
int vsiv4l2_has_meMode(struct vsi_v4l2_ctx * ctx);
int vsiv4l2_verify_slicemode(struct vsi_v4l2_ctx *ctx);
void vsiv4l2_verify_gopsize(struct vsi_v4l2_ctx *ctx);
s32 vsiv4l2_verify_h264_codinglayerqp(s32 ctrlval);
int vsiv4l2_verify_vpxfilterlvl(struct vsi_v4l2_ctx *ctx);
int vsiv4l2_verify_vpxfiltersharp(struct vsi_v4l2_ctx *ctx);
int vsiv4l2_verify_vpxgoldenrate(struct vsi_v4l2_ctx *ctx);
int vsiv4l2_verify_refno(struct vsi_v4l2_ctx *ctx);
void vsiv4l2_verify_vpxqphdr(struct vsi_v4l2_ctx *ctx);
s32 vsiv4l2_get_maxhevclayern(void);
void vsi_enc_update_ctrltbl(int ctrlid, int index, s32 val);
void init_encparams(struct vsi_v4l2_encparams *param);
u32 writeout_encparam(struct vsi_v4l2_ctx *ctx, void *outbuf, u32 *paramsize);

static inline int isencoder(struct vsi_v4l2_ctx *ctx)
{
	return ((ctx->flag & CTX_FLAG_DEC) == 0);
}

static inline int isdecoder(struct vsi_v4l2_ctx *ctx)
{
	return ((ctx->flag & CTX_FLAG_DEC) != 0);
}

static inline int isvalidtype(int buftype, int ctxtype)
{
	if (((ctxtype & CTX_FLAG_DEC) == 0) &&
		(buftype == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
		buftype == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE))
		return 1;
	if (((ctxtype & CTX_FLAG_DEC) != 0) &&
		(buftype == V4L2_BUF_TYPE_VIDEO_OUTPUT ||
		buftype == V4L2_BUF_TYPE_VIDEO_CAPTURE))
		return 1;
	return 0;
}

static inline int isimage(u32 pixelformat)
{
	return (pixelformat == V4L2_PIX_FMT_JPEG);
}

static inline int binputqueue(int qtype)
{
	return V4L2_TYPE_IS_OUTPUT(qtype);
}

static inline int brawfmt(int ctxtype, int qtype)
{
	if ((((ctxtype & CTX_FLAG_DEC) == 0) && qtype == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ||
		(((ctxtype & CTX_FLAG_DEC) != 0) && qtype == V4L2_BUF_TYPE_VIDEO_CAPTURE))
		return 1;
	return 0;
}

static inline struct vsi_vpu_buf *vb_to_vsibuf(struct vb2_buffer *vb)
{
	return container_of(to_vb2_v4l2_buffer(vb), struct vsi_vpu_buf, vb);
}

static inline struct vsi_v4l2_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct vsi_v4l2_ctx, fh);
}

static inline struct vsi_v4l2_ctx *ctrl_to_ctx(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct vsi_v4l2_ctx, ctrlhdl);
}

static inline int addcropmsg(struct vsi_v4l2_ctx *ctx, struct vsi_v4l2_msg *pmsg)
{
	struct cropinfo *crop = vmalloc(sizeof(struct cropinfo));

	if (!crop)
		return -ENOMEM;

	crop->frame_width = pmsg->params.dec_params.pic_info.pic_info.width;
	crop->frame_height = pmsg->params.dec_params.pic_info.pic_info.height;
	crop->left = pmsg->params.dec_params.pic_info.pic_info.crop_left;
	crop->top = pmsg->params.dec_params.pic_info.pic_info.crop_top;
	crop->width = pmsg->params.dec_params.pic_info.pic_info.crop_width;
	crop->height = pmsg->params.dec_params.pic_info.pic_info.crop_height;
	crop->pic_wstride = pmsg->params.dec_params.pic_info.pic_info.pic_wstride;
	memcpy(&crop->extra_info, &pmsg->params.dec_params.pic_info.pic_info.extra_info,
			sizeof(struct v4l2_daemon_extra_info));
	crop->next = NULL;
	if (ctx->croptail == NULL)
		ctx->crophead = ctx->croptail = crop;
	else {
		ctx->croptail->next = crop;
		ctx->croptail = crop;
	}
	return 0;
}

static inline int update_and_removecropinfo(struct vsi_v4l2_ctx *ctx)
{
	struct vsi_v4l2_mediacfg *pcfg = &ctx->mediacfg;
	struct cropinfo *crop = ctx->crophead;

	if (crop) {
		pcfg->decparams.dec_info.io_buffer.output_width = crop->frame_width;
		pcfg->decparams.dec_info.io_buffer.output_height = crop->frame_height;
		pcfg->decparams.dec_info.io_buffer.output_wstride = crop->pic_wstride;
		pcfg->bytesperline = crop->pic_wstride;
		pcfg->decparams.dec_info.dec_info.frame_width = crop->frame_width;
		pcfg->decparams.dec_info.dec_info.frame_height = crop->frame_height;
		pcfg->decparams.dec_info.dec_info.visible_rect.left = crop->left;
		pcfg->decparams.dec_info.dec_info.visible_rect.top = crop->top;
		pcfg->decparams.dec_info.dec_info.visible_rect.width = crop->width;
		pcfg->decparams.dec_info.dec_info.visible_rect.height = crop->height;

		memcpy(&pcfg->decparams.pic_info.pic_info.extra_info, &crop->extra_info,
				sizeof(struct v4l2_daemon_extra_info));
		if (!crop->next)
			ctx->crophead = ctx->croptail = NULL;
		else
			ctx->crophead = crop->next;
		vfree(crop);
		return 1;
	}
	return 0;
}

static inline void removeallcropinfo(struct vsi_v4l2_ctx *ctx)
{
	struct cropinfo *nc, *crop = ctx->crophead;

	while (crop) {
		nc = crop->next;
		vfree(crop);
		crop = nc;
	}
	ctx->crophead = ctx->croptail = NULL;
}

static inline void printbufinfo(struct vb2_queue *vq)
{
	struct vsi_vpu_buf *buf, *node;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(vq->drv_priv);

	v4l2_klog(LOGLVL_VERBOSE, "#################################################");
	v4l2_klog(LOGLVL_VERBOSE, "que has %d vb2 buffers, que count = %d", vq->num_buffers, vq->queued_count);
	v4l2_klog(LOGLVL_VERBOSE, "input_list:");
	if (!list_empty(&ctx->input_list)) {
		list_for_each_entry_safe(buf, node, &ctx->input_list, list) {
			v4l2_klog(LOGLVL_VERBOSE, "list node %d", buf->vb.vb2_buf.index);
		}
	}
	v4l2_klog(LOGLVL_VERBOSE, "output_list:");
	if (!list_empty(&ctx->output_list)) {
		list_for_each_entry_safe(buf, node, &ctx->output_list, list) {
			v4l2_klog(LOGLVL_VERBOSE, "list node %d", buf->vb.vb2_buf.index);
		}
	}
	v4l2_klog(LOGLVL_VERBOSE, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
}

static inline int inst_isactive(struct vsi_v4l2_ctx *ctx)
{
	if (ctx->status == DEC_STATUS_DECODING ||
		ctx->status == DEC_STATUS_DRAINING ||
		ctx->status == ENC_STATUS_ENCODING ||
		ctx->status == ENC_STATUS_DRAINING)
		return 1;
	return 0;
}

static inline void return_all_buffers(struct vb2_queue *vq, int status, int bRelbuf)
{
	int i;
	struct vsi_vpu_buf *buf, *node;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(vq->drv_priv);
	struct list_head *plist;

	v4l2_klog(LOGLVL_FLOW, "%s", __func__);

	if (binputqueue(vq->type))
		plist = &ctx->input_list;
	else
		plist = &ctx->output_list;

	for (i = 0; i < vq->num_buffers; ++i) {
		if (vq->bufs[i]->state == VB2_BUF_STATE_ACTIVE) {
			v4l2_klog(LOGLVL_FLOW, "return buffer %d", i);
			vb2_buffer_done(vq->bufs[i], status);
		}
	}
	if (bRelbuf) {
		list_for_each_entry_safe(buf, node, plist, list) {
			for (i = 0; i < buf->vb.vb2_buf.num_planes; i++)
				vb2_set_plane_payload(&buf->vb.vb2_buf, i, 0);
			list_del(&buf->list);
			v4l2_klog(LOGLVL_FLOW, "clear buffer %d", buf->vb.vb2_buf.index);
		}
	}
}

static inline void print_queinfo(struct vb2_queue *q)
{
	int i, k;

	v4l2_klog(LOGLVL_VERBOSE, "got %d buffer", q->num_buffers);
	for (i = 0; i < q->num_buffers; i++) {
		struct vb2_buffer	*buf = q->bufs[i];

		v4l2_klog(LOGLVL_VERBOSE, "buf %d. %p has %d planes", i, buf, buf->num_planes);
		for (k = 0; k < buf->num_planes; k++) {
			int *data = vb2_plane_vaddr(buf, k);

			v4l2_klog(LOGLVL_VERBOSE, "plane %d = %lx, size = %x, offset = %x",
				k, (unsigned long)data, buf->planes[k].length, buf->planes[k].m.offset);
		}
	}
}

static inline int vsi_checkctx_outputoffdone(struct vsi_v4l2_ctx *ctx)
{
	if (test_and_clear_bit(CTX_FLAG_OUTPUTOFFDONE, &ctx->flag) || ctx->error < 0)
		return 1;
	return 0;
}

static inline int vsi_checkctx_capoffdone(struct vsi_v4l2_ctx *ctx)
{
	if (test_and_clear_bit(CTX_FLAG_CAPTUREOFFDONE, &ctx->flag) || ctx->error < 0)
		return 1;
	return 0;
}

#endif	//VSI_V4L2_PRIV_H

