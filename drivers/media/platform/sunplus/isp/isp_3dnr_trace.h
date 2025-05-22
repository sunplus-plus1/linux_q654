// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP3DNR
 *
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM isp_3dnr

#if !defined(_ISP_3DNR_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _ISP_3DNR_TRACE_H

#include <linux/tracepoint.h>

TRACE_EVENT(isp_3dnr_stream_on_start, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	    TP_STRUCT__entry(__field(int, device_id)),
	    TP_fast_assign(__entry->device_id = dev_id;),
	    TP_printk("(isp) 3dnr stream on start, device_id = %d\n",
		      __entry->device_id));

TRACE_EVENT(isp_3dnr_stream_on_end, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	    TP_STRUCT__entry(__field(int, device_id)),
	    TP_fast_assign(__entry->device_id = dev_id;),
	    TP_printk("(isp) 3dnr stream on end, device_id = %d\n",
		      __entry->device_id));

TRACE_EVENT(isp_3dnr_stream_off_start, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	    TP_STRUCT__entry(__field(int, device_id)),
	    TP_fast_assign(__entry->device_id = dev_id;),
	    TP_printk("(isp) 3dnr stream off start, device_id = %d\n",
		      __entry->device_id));

TRACE_EVENT(isp_3dnr_stream_off_end, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	    TP_STRUCT__entry(__field(int, device_id)),
	    TP_fast_assign(__entry->device_id = dev_id;),
	    TP_printk("(isp) 3dnr stream off end, device_id = %d\n",
		      __entry->device_id));

TRACE_EVENT(isp_3dnr_q_start, TP_PROTO(int dev_id, int buff_idx),
	    TP_ARGS(dev_id, buff_idx),

	    TP_STRUCT__entry(__field(int, dev_id) __field(int, buff_idx)),
	    TP_fast_assign(__entry->dev_id = dev_id;
			   __entry->buff_idx = buff_idx;),
	    TP_printk("(isp) 3dnr q start (device %d)(idx=%d)\n",
		      __entry->dev_id, __entry->buff_idx));

TRACE_EVENT(isp_3dnr_q_end, TP_PROTO(int dev_id, int buff_idx),
	    TP_ARGS(dev_id, buff_idx),

	    TP_STRUCT__entry(__field(int, dev_id) __field(int, buff_idx)),
	    TP_fast_assign(__entry->dev_id = dev_id;
			   __entry->buff_idx = buff_idx;),
	    TP_printk("(isp) 3dnr q end (device %d)(idx=%d)\n",
		      __entry->dev_id, __entry->buff_idx));

TRACE_EVENT(isp_3dnr_dq_start, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	    TP_STRUCT__entry(__field(int, dev_id)),
	    TP_fast_assign(__entry->dev_id = dev_id;),
	    TP_printk("(isp) 3dnr dq start (device %d)\n",
		      __entry->dev_id));

TRACE_EVENT(isp_3dnr_dq_end, TP_PROTO(int dev_id, int buff_idx),
	    TP_ARGS(dev_id, buff_idx),

	    TP_STRUCT__entry(__field(int, dev_id) __field(int, buff_idx)),
	    TP_fast_assign(__entry->dev_id = dev_id;
			   __entry->buff_idx = buff_idx;),
	    TP_printk("(isp) 3dnr dq end (device %d)(idx=%d)\n",
		      __entry->dev_id, __entry->buff_idx));

TRACE_EVENT(isp_3dnr_buf_count, TP_PROTO(int dev_id, int buff_count),
	    TP_ARGS(dev_id, buff_count),

	    TP_STRUCT__entry(__field(int, dev_id) __field(int, buff_count)),
	    TP_fast_assign(__entry->dev_id = dev_id;
			   __entry->buff_count = buff_count;),
	    TP_printk("(isp) 3dnr get buf. (device %d)(buf_count=%d)\n",
		      __entry->dev_id, __entry->buff_count));

TRACE_EVENT(
	isp_3dnr_buf_swap,
	TP_PROTO(int dev_id, int swap_count, int buff_in_idx, int buff_out_idx),
	TP_ARGS(dev_id, swap_count, buff_in_idx, buff_out_idx),

	TP_STRUCT__entry(__field(int, dev_id) __field(int, swap_count)
				 __field(int, buff_in_idx)
					 __field(int, buff_out_idx)),
	TP_fast_assign(__entry->dev_id = dev_id;
		       __entry->swap_count = swap_count;
		       __entry->buff_in_idx = buff_in_idx;
		       __entry->buff_out_idx = buff_out_idx;),
	TP_printk(
		"(isp) 3dnr get buf. swap case (device %d)(swap_count=%d)(buf_in_idx=%d)(buf_out_idx=%d)\n",
		__entry->dev_id, __entry->swap_count, __entry->buff_in_idx,
		__entry->buff_out_idx));

TRACE_EVENT(
	isp_3dnr_buf_first, TP_PROTO(int dev_id, int buff_out_idx),
	TP_ARGS(dev_id, buff_out_idx),

	TP_STRUCT__entry(__field(int, dev_id) __field(int, buff_out_idx)),
	TP_fast_assign(__entry->dev_id = dev_id;
		       __entry->buff_out_idx = buff_out_idx;),
	TP_printk(
		"(isp) 3dnr get buf. first frame case (device %d)(buf_out_idx=%d)\n",
		__entry->dev_id, __entry->buff_out_idx));

TRACE_EVENT(isp_3dnr_buf_no_frame, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	    TP_STRUCT__entry(__field(int, dev_id)),
	    TP_fast_assign(__entry->dev_id = dev_id;),
	    TP_printk("(isp) 3dnr get buf. no frame (device %d)\n",
		      __entry->dev_id));

TRACE_EVENT(
	isp_3dnr_buf_next_frame,
	TP_PROTO(int dev_id, int buff_in_idx, int buff_out_idx),
	TP_ARGS(dev_id, buff_in_idx, buff_out_idx),

	TP_STRUCT__entry(__field(int, dev_id) __field(int, buff_in_idx)
				 __field(int, buff_out_idx)),
	TP_fast_assign(__entry->dev_id = dev_id;
		       __entry->buff_in_idx = buff_in_idx;
		       __entry->buff_out_idx = buff_out_idx;),
	TP_printk(
		"(isp) 3dnr get buf. next frame case (device %d)(buf_in_idx=%d)(buf_out_idx=%d)\n",
		__entry->dev_id, __entry->buff_in_idx, __entry->buff_out_idx));

TRACE_EVENT(
	isp_3dnr_buf_not_enough, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	TP_STRUCT__entry(__field(int, dev_id)),
	TP_fast_assign(__entry->dev_id = dev_id;),
	TP_printk("(isp) 3dnr get buf. frame is not enough (device %d)\n",
		  __entry->dev_id));

TRACE_EVENT(isp_3dnr_done_count, TP_PROTO(int dev_id, int buff_count),
	    TP_ARGS(dev_id, buff_count),

	    TP_STRUCT__entry(__field(int, dev_id) __field(int, buff_count)),
	    TP_fast_assign(__entry->dev_id = dev_id;
			   __entry->buff_count = buff_count;),
	    TP_printk("(isp) 3dnr hw done. (device %d)(buf_count=%d)\n",
		      __entry->dev_id, __entry->buff_count));

TRACE_EVENT(
	isp_3dnr_done_first_enough, TP_PROTO(int dev_id, int buff_count),
	TP_ARGS(dev_id, buff_count),

	TP_STRUCT__entry(__field(int, dev_id) __field(int, buff_count)),
	TP_fast_assign(__entry->dev_id = dev_id;
		       __entry->buff_count = buff_count;),
	TP_printk(
		"(isp) 3dnr hw done. first frame case. enough frames(device %d)(buf_count=%d)\n",
		__entry->dev_id, __entry->buff_count));

TRACE_EVENT(
	isp_3dnr_done_first_not_enough, TP_PROTO(int dev_id, int buff_count),
	TP_ARGS(dev_id, buff_count),

	TP_STRUCT__entry(__field(int, dev_id) __field(int, buff_count)),
	TP_fast_assign(__entry->dev_id = dev_id;
		       __entry->buff_count = buff_count;),
	TP_printk(
		"(isp) 3dnr hw done. first frame case. not enough frame(device %d)(buf_count=%d)\n",
		__entry->dev_id, __entry->buff_count));

TRACE_EVENT(
	isp_3dnr_done_swap_off, TP_PROTO(int dev_id, int swap_count),
	TP_ARGS(dev_id, swap_count),

	TP_STRUCT__entry(__field(int, dev_id) __field(int, swap_count)),
	TP_fast_assign(__entry->dev_id = dev_id;
		       __entry->swap_count = swap_count;),
	TP_printk(
		"(isp) 3dnr hw done. swap off case (device %d)(pre swap_count=%d)\n",
		__entry->dev_id, __entry->swap_count));

TRACE_EVENT(
	isp_3dnr_done_buf_release, TP_PROTO(int dev_id, int in_buff_idx),
	TP_ARGS(dev_id, in_buff_idx),

	TP_STRUCT__entry(__field(int, dev_id) __field(int, in_buff_idx)),
	TP_fast_assign(__entry->dev_id = dev_id;
		       __entry->in_buff_idx = in_buff_idx;),
	TP_printk(
		"(isp) 3dnr hw done. release input buffer case (device %d)(in_buff_idx=%d)\n",
		__entry->dev_id, __entry->in_buff_idx));

TRACE_EVENT(
	isp_3dnr_done_swap_on,
	TP_PROTO(int dev_id, int swap_count, int buf_count),
	TP_ARGS(dev_id, swap_count, buf_count),

	TP_STRUCT__entry(__field(int, dev_id) __field(int, swap_count)
				 __field(int, buf_count)),
	TP_fast_assign(__entry->dev_id = dev_id;
		       __entry->swap_count = swap_count;
		       __entry->buf_count = buf_count;),
	TP_printk(
		"(isp) 3dnr hw done. swap on case (device %d)(swap_count=%d)(buf_count=%d)\n",
		__entry->dev_id, __entry->swap_count, __entry->buf_count));

TRACE_EVENT(
	isp_3dnr_done_buf_error, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	TP_STRUCT__entry(__field(int, dev_id)),
	TP_fast_assign(__entry->dev_id = dev_id;),
	TP_printk(
		"(isp) 3dnr hw done. error. Have output, no input, not first frame (device %d)\n",
		__entry->dev_id));

TRACE_EVENT(isp_3dnr_done_no_frame, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	    TP_STRUCT__entry(__field(int, dev_id)),
	    TP_fast_assign(__entry->dev_id = dev_id;),
	    TP_printk("(isp) 3dnr hw done. No frame case (device %d)\n",
		      __entry->dev_id));

TRACE_EVENT(isp_3dnr_dev_alive, TP_PROTO(int dev_id, int alive),
	    TP_ARGS(dev_id, alive),

	    TP_STRUCT__entry(__field(int, dev_id) __field(int, alive)),
	    TP_fast_assign(__entry->dev_id = dev_id; __entry->alive = alive;),
	    TP_printk("(isp) 3dnr  (device %d)(alive=%d)\n",
		      __entry->dev_id, __entry->alive));

TRACE_EVENT(isp_3dnr_first_or_not, TP_PROTO(int dev_id, int first),
	    TP_ARGS(dev_id, first),

	    TP_STRUCT__entry(__field(int, dev_id) __field(int, first)),
	    TP_fast_assign(__entry->dev_id = dev_id; __entry->first = first;),
	    TP_printk("(isp) 3dnr  (device %d)(first=%d)\n",
		      __entry->dev_id, __entry->first));

TRACE_EVENT(isp_3dnr_frame_not_enough, TP_PROTO(int dev_id), TP_ARGS(dev_id),

	    TP_STRUCT__entry(__field(int, dev_id)),
	    TP_fast_assign(__entry->dev_id = dev_id;),
	    TP_printk("(isp) 3dnr  (device %d)\n", __entry->dev_id));

#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../../drivers/media/platform/sunplus/isp
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE isp_3dnr_trace
#include <trace/define_trace.h>
