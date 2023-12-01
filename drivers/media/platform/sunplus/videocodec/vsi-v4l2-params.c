/*
 *    VSI v4l2 codec parameter manager.
 *
 *    Copyright (c) 2019: VeriSilicon Inc.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License: version 2: as
 *    published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful:
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License version 2 for more details.
 *
 *    You may obtain a copy of the GNU General Public License
 *    Version 2 at the following locations:
 *    https://opensource.org/licenses/gpl-2.0.php
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-vmalloc.h>
#include "vsi-v4l2-priv.h"

void init_encparams (struct vsi_v4l2_encparams *param)
{
#define INIT_PARAM(member, id)	\
	member.head = /*(0x1ul << PARAM_UPDATE_BIT ) |*/ (sizeof(member) << 12) | id

	memset(param, 0, sizeof(*param));
	INIT_PARAM(param->m_framerate, V4L2_ENCCFG_FRAMERATE);
	INIT_PARAM(param->m_cropinfo, V4L2_ENCCFG_CROPINFO);
	INIT_PARAM(param->m_intrapicrate, V4L2_ENCCFG_INTRAPICRATE);
	INIT_PARAM(param->m_gopsize, V4L2_ENCCFG_GOPSIZE);
	INIT_PARAM(param->m_profile, V4L2_ENCCFG_PROFILE);
	INIT_PARAM(param->m_level, V4L2_ENCCFG_LEVEL);
	INIT_PARAM(param->m_bitrate, V4L2_ENCCFG_BITRATE);
	INIT_PARAM(param->m_qprange, V4L2_ENCCFG_QPRANGE);
	INIT_PARAM(param->m_bitratemode, V4L2_ENCCFG_BITRATEMODE);
	INIT_PARAM(param->m_sliceinfo, V4L2_ENCCFG_SLICEINFO);
	INIT_PARAM(param->m_rcmode, V4L2_ENCCFG_RCMODE);
	INIT_PARAM(param->m_qphdr, V4L2_ENCCFG_QPHDR);
	INIT_PARAM(param->m_qphdrip, V4L2_ENCCFG_QPHDRIP);
	INIT_PARAM(param->m_rotation, V4L2_ENCCFG_ROTATION);
	INIT_PARAM(param->m_gdrduration, V4L2_ENCCFG_GDRDURATION);
	INIT_PARAM(param->m_idrhdr, V4L2_ENCCFG_IDRHDR);
	INIT_PARAM(param->m_entropymode, V4L2_ENCCFG_ENTROYPMODE);
	INIT_PARAM(param->m_cpbsize, V4L2_ENCCFG_CPBSIZE);
	INIT_PARAM(param->m_trans8x8, V4L2_ENCCFG_TRANS8X8);
	INIT_PARAM(param->m_ctrintrapred, V4L2_ENCCFG_CTRINTRAPRED);
	INIT_PARAM(param->m_roiinfo, V4L2_ENCCFG_ROIINFO);
	INIT_PARAM(param->m_ipcminfo, V4L2_ENCCFG_IPCMINFO);
	INIT_PARAM(param->m_aspect, V4L2_ENCCFG_ASPECT);
	INIT_PARAM(param->m_srcinfo, V4L2_ENCCFG_SRCINFO);
	INIT_PARAM(param->m_codecfmt, V4L2_ENCCFG_CODECFMT);
	INIT_PARAM(param->m_vuiinfo, V4L2_ENCCFG_VUIINFO);
	INIT_PARAM(param->m_jpgfixqp, V4L2_ENCCFG_JPGFIXQP);
	INIT_PARAM(param->m_restartinterval, V4L2_ENCCFG_RESTARTINTERVAL);
	INIT_PARAM(param->m_restartinterval, V4L2_ENCCFG_SKIPMODE);
	INIT_PARAM(param->m_aud, V4L2_ENCCFG_AUD);
	INIT_PARAM(param->m_loopfilter, V4L2_ENCCFG_LOOPFILTER);
	INIT_PARAM(param->m_chromaqpoffset, V4L2_ENCCFG_CHROMAQPOFFSET);
	INIT_PARAM(param->m_iqprange, V4L2_ENCCFG_IQPRANGE);
	INIT_PARAM(param->m_pqprange, V4L2_ENCCFG_PQPRANGE);
	INIT_PARAM(param->m_mvrange, V4L2_ENCCFG_MVRANGE);
	INIT_PARAM(param->m_headermode, V4L2_ENCCFG_HEADERMODE);
	INIT_PARAM(param->m_vpxpartitions, V4L2_ENCCFG_VPXPARTITIONS);
	INIT_PARAM(param->m_refno, V4L2_ENCCFG_REFNO);
	INIT_PARAM(param->m_vpxfilterlvl, V4L2_ENCCFG_VPXFILTERLVL);
	INIT_PARAM(param->m_vpxfiltersharp, V4L2_ENCCFG_VPXFILTERSHARP);
	INIT_PARAM(param->m_goldenperiod, V4L2_ENCCFG_GOLDENPERIOD);
	INIT_PARAM(param->m_tier, V4L2_ENCCFG_TIER);
	INIT_PARAM(param->m_refresh, V4L2_ENCCFG_REFRESH);
	INIT_PARAM(param->m_temporalid, V4L2_ENCCFG_TEMPORALID);
	INIT_PARAM(param->m_strongsmooth, V4L2_ENCCFG_STRONGSMOOTH);
	INIT_PARAM(param->m_tmvp, V4L2_ENCCFG_TMVP);
	INIT_PARAM(param->m_startcode, V4L2_ENCCFG_STARTCODE);
	INIT_PARAM(param->m_resendSPSPPS, V4L2_ENCCFG_RESENDSPSPPS);
	INIT_PARAM(param->m_jpgcodingmode, V4L2_ENCCFG_JPGCODINGMODE);
	INIT_PARAM(param->m_gopcfg, V4L2_ENCCFG_GOPCFG);
	INIT_PARAM(param->m_forceidr, V4L2_ENCCFG_FORCEIDR);
	INIT_PARAM(param->m_securemode, V4L2_ENCCFG_SECUREMODE);
	INIT_PARAM(param->m_scaleoutput, V4L2_ENCCFG_SCALEOUTPUT);
}

u32 writeout_encparam(struct vsi_v4l2_ctx *ctx, void *outbuf, u32 *paramsize)
{
#define COPY_PARAM(member, buf)	\
	{	\
		if (test_and_clear_bit(PARAM_UPDATE_BIT, &member.head)) {	\
				memcpy(buf, &member, sizeof(member));	\
				buf += sizeof(member);	\
				noparm++;	\
				v4l2_klog(LOGLVL_VERBOSE, "write msg param %d @ offset %d", id, size);	\
				size += sizeof(member);	\
			}	\
			break;	\
	}

	struct vsi_v4l2_encparams *param = &ctx->mediacfg.m_encparams;
	u32 noparm = 0, size = 0;
	int id;
	u8 *buf = outbuf;
	s32 val, val1;

	for (id = V4L2_ENCCFG_UNKNOWN + 1; id < V4L2_ENCCFG_NO; id++) {
		switch (id) {
		case V4L2_ENCCFG_FRAMERATE:
			COPY_PARAM(param->m_framerate, buf)
		case V4L2_ENCCFG_CROPINFO:
			COPY_PARAM(param->m_cropinfo, buf)
		case V4L2_ENCCFG_JPGFIXQP:
			COPY_PARAM(param->m_jpgfixqp, buf)
		case V4L2_ENCCFG_RESTARTINTERVAL:
			COPY_PARAM(param->m_restartinterval, buf)
		case V4L2_ENCCFG_INTRAPICRATE:
			COPY_PARAM(param->m_intrapicrate, buf)
		case V4L2_ENCCFG_GOPSIZE:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_gopsize.head)) {
				vsiv4l2_verify_gopsize(ctx);
				set_bit(PARAM_UPDATE_BIT, &param->m_gopsize.head);
			}
			COPY_PARAM(param->m_gopsize, buf)
		case V4L2_ENCCFG_PROFILE:
			COPY_PARAM(param->m_profile, buf)
		case V4L2_ENCCFG_LEVEL:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_level.head)) {
				val = param->m_level.level;
				param->m_level.level = vsi_get_level(ctx);
				if (param->m_level.level != val)
					set_bit(PARAM_UPDATE_BIT, &param->m_level.head);
			}
			COPY_PARAM(param->m_level, buf)
		case V4L2_ENCCFG_BITRATE:
			COPY_PARAM(param->m_bitrate, buf)
		case V4L2_ENCCFG_QPRANGE:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_qprange.head)) {
				val = ctx->mediacfg.m_encparams.m_qprange.qpMax;
				val1 = ctx->mediacfg.m_encparams.m_qprange.qpMin;
				if (param->m_codecfmt.codecFormat == V4L2_DAEMON_CODEC_ENC_HEVC ||
					param->m_codecfmt.codecFormat == V4L2_DAEMON_CODEC_ENC_H264) {
					param->m_qprange.qpMin = ctx->mediacfg.qpMin_h26x;
					param->m_qprange.qpMax = ctx->mediacfg.qpMax_h26x;
				} else if (param->m_codecfmt.codecFormat == V4L2_DAEMON_CODEC_ENC_VP9 ||
					param->m_codecfmt.codecFormat == V4L2_DAEMON_CODEC_ENC_VP8) {
					param->m_qprange.qpMin = ctx->mediacfg.qpMin_vpx;
					param->m_qprange.qpMax = ctx->mediacfg.qpMax_vpx;
				}
				if (val != ctx->mediacfg.m_encparams.m_qprange.qpMax ||
					val1 != ctx->mediacfg.m_encparams.m_qprange.qpMin)
					set_bit(PARAM_UPDATE_BIT, &param->m_qprange.head);
			}
			COPY_PARAM(param->m_qprange, buf)
		case V4L2_ENCCFG_BITRATEMODE:
			COPY_PARAM(param->m_bitratemode, buf)
		case V4L2_ENCCFG_SLICEINFO:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_sliceinfo.head)) {
				if (vsiv4l2_verify_slicemode(ctx))
					set_bit(PARAM_UPDATE_BIT, &param->m_sliceinfo.head);
			}
			COPY_PARAM(param->m_sliceinfo, buf)
		case V4L2_ENCCFG_RCMODE:
			COPY_PARAM(param->m_rcmode, buf)
		case V4L2_ENCCFG_QPHDR:
			COPY_PARAM(param->m_qphdr, buf)
		case V4L2_ENCCFG_QPHDRIP:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_qphdrip.head)) {
				val = ctx->mediacfg.m_encparams.m_qphdrip.qpHdrI;
				val1 = ctx->mediacfg.m_encparams.m_qphdrip.qpHdrP;
				if (param->m_codecfmt.codecFormat == V4L2_DAEMON_CODEC_ENC_HEVC ||
					param->m_codecfmt.codecFormat == V4L2_DAEMON_CODEC_ENC_H264) {
					param->m_qphdrip.qpHdrI = ctx->mediacfg.qpHdrI_h26x;
					param->m_qphdrip.qpHdrP = ctx->mediacfg.qpHdrP_h26x;
				} else if (param->m_codecfmt.codecFormat == V4L2_DAEMON_CODEC_ENC_VP9 ||
					param->m_codecfmt.codecFormat == V4L2_DAEMON_CODEC_ENC_VP8) {
					vsiv4l2_verify_vpxqphdr(ctx);
				}
				if (val != ctx->mediacfg.m_encparams.m_qphdrip.qpHdrI ||
					val1 != ctx->mediacfg.m_encparams.m_qphdrip.qpHdrP)
					set_bit(PARAM_UPDATE_BIT, &param->m_qphdrip.head);
			}
			COPY_PARAM(param->m_qphdrip, buf)
		case V4L2_ENCCFG_ROTATION:
			COPY_PARAM(param->m_rotation, buf)
		case V4L2_ENCCFG_GDRDURATION:
			COPY_PARAM(param->m_gdrduration, buf)
		case V4L2_ENCCFG_IDRHDR:
			COPY_PARAM(param->m_idrhdr, buf)
		case V4L2_ENCCFG_ENTROYPMODE:
			COPY_PARAM(param->m_entropymode, buf)
		case V4L2_ENCCFG_CPBSIZE:
			COPY_PARAM(param->m_cpbsize, buf)
		case V4L2_ENCCFG_TRANS8X8:
			COPY_PARAM(param->m_trans8x8, buf)
		case V4L2_ENCCFG_CTRINTRAPRED:
			COPY_PARAM(param->m_ctrintrapred, buf)
		case V4L2_ENCCFG_FORCEIDR:
			COPY_PARAM(param->m_forceidr, buf)
		case V4L2_ENCCFG_ROIINFO:
			if (!vsi_convertROI(ctx))
				clear_bit(PARAM_UPDATE_BIT, &param->m_roiinfo.head);
			COPY_PARAM(param->m_roiinfo, buf)
		case V4L2_ENCCFG_IPCMINFO:
			if (!vsi_convertIPCM(ctx))
				clear_bit(PARAM_UPDATE_BIT, &param->m_ipcminfo.head);
			COPY_PARAM(param->m_ipcminfo, buf)
		case V4L2_ENCCFG_ASPECT:
			COPY_PARAM(param->m_aspect, buf)
		case V4L2_ENCCFG_SRCINFO:
			COPY_PARAM(param->m_srcinfo, buf)
		case V4L2_ENCCFG_CODECFMT:
			COPY_PARAM(param->m_codecfmt, buf)
		case V4L2_ENCCFG_VUIINFO:
			COPY_PARAM(param->m_vuiinfo, buf)
		case V4L2_ENCCFG_SKIPMODE:
			COPY_PARAM(param->m_vuiinfo, buf)
		case V4L2_ENCCFG_AUD:
			COPY_PARAM(param->m_aud, buf)
		case V4L2_ENCCFG_LOOPFILTER:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_loopfilter.head)) {
				val = vsiv4l2_has_loopfilter(ctx);
				if (val != -1 &&
					param->m_loopfilter.disableDeblockingFilter != val) {
					param->m_loopfilter.disableDeblockingFilter = val;
					set_bit(PARAM_UPDATE_BIT, &param->m_loopfilter.head);
				}	
			}
			COPY_PARAM(param->m_loopfilter, buf)
		case V4L2_ENCCFG_CHROMAQPOFFSET:
			COPY_PARAM(param->m_chromaqpoffset, buf)
		case V4L2_ENCCFG_IQPRANGE:
			COPY_PARAM(param->m_iqprange, buf)
		case V4L2_ENCCFG_PQPRANGE:
			COPY_PARAM(param->m_pqprange, buf)
		case V4L2_ENCCFG_MVRANGE:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_mvrange.head)) {
				if (!vsiv4l2_has_meMode(ctx))
					param->m_mvrange.meVertSearchRange = 0;
				set_bit(PARAM_UPDATE_BIT, &param->m_mvrange.head);
			}
			COPY_PARAM(param->m_mvrange, buf)
		case V4L2_ENCCFG_HEADERMODE:
			COPY_PARAM(param->m_headermode, buf)
		case V4L2_ENCCFG_VPXPARTITIONS:	
			COPY_PARAM(param->m_vpxpartitions, buf)
		case V4L2_ENCCFG_REFNO:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_refno.head)) {
				if (vsiv4l2_verify_refno(ctx))
					set_bit(PARAM_UPDATE_BIT, &param->m_refno.head);
			}
			COPY_PARAM(param->m_refno, buf)
		case V4L2_ENCCFG_VPXFILTERLVL:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_vpxfilterlvl.head)) {
				if (vsiv4l2_verify_vpxfilterlvl(ctx))
					set_bit(PARAM_UPDATE_BIT, &param->m_vpxfilterlvl.head);
			}
			COPY_PARAM(param->m_vpxfilterlvl, buf)
		case V4L2_ENCCFG_VPXFILTERSHARP:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_vpxfiltersharp.head)) {
				if (vsiv4l2_verify_vpxfiltersharp(ctx))
					set_bit(PARAM_UPDATE_BIT, &param->m_vpxfiltersharp.head);
			}
			COPY_PARAM(param->m_vpxfiltersharp, buf)
		case V4L2_ENCCFG_GOLDENPERIOD:
			if (test_and_clear_bit(PARAM_CHECK_BIT, &param->m_goldenperiod.head)) {
				if (vsiv4l2_verify_vpxgoldenrate(ctx))
					set_bit(PARAM_UPDATE_BIT, &param->m_goldenperiod.head);
			}
			COPY_PARAM(param->m_goldenperiod, buf)
		case V4L2_ENCCFG_TIER:
			COPY_PARAM(param->m_tier, buf)
		case V4L2_ENCCFG_REFRESH:
			COPY_PARAM(param->m_refresh, buf)
		case V4L2_ENCCFG_TEMPORALID:
			COPY_PARAM(param->m_temporalid, buf)
		case V4L2_ENCCFG_STRONGSMOOTH:
			COPY_PARAM(param->m_strongsmooth, buf)
		case V4L2_ENCCFG_TMVP:
			COPY_PARAM(param->m_tmvp, buf)
		case V4L2_ENCCFG_STARTCODE:
			COPY_PARAM(param->m_startcode, buf)
		case V4L2_ENCCFG_RESENDSPSPPS:
			COPY_PARAM(param->m_resendSPSPPS, buf)
		case V4L2_ENCCFG_JPGCODINGMODE:
			COPY_PARAM(param->m_jpgcodingmode, buf)
		case V4L2_ENCCFG_GOPCFG:
			COPY_PARAM(param->m_gopcfg, buf)
		case V4L2_ENCCFG_SECUREMODE:
			COPY_PARAM(param->m_securemode, buf)
		case V4L2_ENCCFG_SCALEOUTPUT:
			COPY_PARAM(param->m_scaleoutput, buf)
		default:
			break;
		}
	}
	*paramsize = size;
	return noparm;
}


