// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP
 *
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM isp

#if !defined(_ISP_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _ISP_TRACE_H

#include <linux/tracepoint.h>

TRACE_EVENT(isp_stream_on_start, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) stream on start, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(isp_stream_on_end, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) stream on end, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(isp_stream_off_start, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) stream off start, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(isp_stream_off_end, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) stream off end, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(reg_user_start, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) add new reg buf node start, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(reg_user_done, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) add new reg buf node end, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(reg_new_setting, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) reg new setting, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(reg_last_setting, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) reg last setting, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(reg_no_setting, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) reg no setting, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(isp_q_start, TP_PROTO(void *vfh, int buff_idx),
	    TP_ARGS(vfh, buff_idx),
	    TP_STRUCT__entry(__field(void *, handle) __field(int, buff_idx)),
	    TP_fast_assign(__entry->handle = vfh; __entry->buff_idx = buff_idx;),
	    TP_printk("(isp) q start (hdl 0x%px)(idx=%d)\n", __entry->handle, __entry->buff_idx));

TRACE_EVENT(isp_q_end, TP_PROTO(void *vfh, int buff_idx),
	    TP_ARGS(vfh, buff_idx),
	    TP_STRUCT__entry(__field(void *, handle) __field(int, buff_idx)),
	    TP_fast_assign(__entry->handle = vfh; __entry->buff_idx = buff_idx;),
	    TP_printk("(isp) q end (hdl 0x%px)(idx=%d)\n", __entry->handle, __entry->buff_idx));

TRACE_EVENT(isp_out_q_start, TP_PROTO(int buff_idx), TP_ARGS(buff_idx),
	    TP_STRUCT__entry(__field(int, buff_idx)),
	    TP_fast_assign(__entry->buff_idx = buff_idx;),
	    TP_printk("(isp) out q start (idx=%d)\n", __entry->buff_idx));

TRACE_EVENT(isp_out_q_end, TP_PROTO(int buff_idx), TP_ARGS(buff_idx),
	    TP_STRUCT__entry(__field(int, buff_idx)),
	    TP_fast_assign(__entry->buff_idx = buff_idx;),
	    TP_printk("(isp) out q end (idx=%d)\n", __entry->buff_idx));

TRACE_EVENT(isp_cap_q_start, TP_PROTO(int buff_idx), TP_ARGS(buff_idx),
	    TP_STRUCT__entry(__field(int, buff_idx)),
	    TP_fast_assign(__entry->buff_idx = buff_idx;),
	    TP_printk("(isp) cap q start (idx=%d)\n", __entry->buff_idx));

TRACE_EVENT(isp_cap_q_end, TP_PROTO(int buff_idx), TP_ARGS(buff_idx),
	    TP_STRUCT__entry(__field(int, buff_idx)),
	    TP_fast_assign(__entry->buff_idx = buff_idx;),
	    TP_printk("(isp) cap q end (idx=%d)\n", __entry->buff_idx));

TRACE_EVENT(isp_dq_start, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) dq start, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(isp_dq_end, TP_PROTO(void *vfh, int buff_idx),
	    TP_ARGS(vfh, buff_idx),
	    TP_STRUCT__entry(__field(void *, handle) __field(int, buff_idx)),
	    TP_fast_assign(__entry->handle = vfh; __entry->buff_idx = buff_idx;),
	    TP_printk("(isp) dq end (hdl 0x%px)(idx=%d)\n", __entry->handle, __entry->buff_idx));

TRACE_EVENT(isp_run_start, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) in/out pair start, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(isp_run_end, TP_PROTO(void *vfh), TP_ARGS(vfh),
	    TP_STRUCT__entry(__field(void *, handle)),
	    TP_fast_assign(__entry->handle = vfh;),
	    TP_printk("(isp) in/out pair end, hdl = 0x%px\n", __entry->handle));

TRACE_EVENT(isp_hw_start, TP_PROTO(void *vfh, int buff_idx),
	    TP_ARGS(vfh, buff_idx),
	    TP_STRUCT__entry(__field(void *, handle) __field(int, buff_idx)),
	    TP_fast_assign(__entry->handle = vfh; __entry->buff_idx = buff_idx;),
	    TP_printk("(isp) hw start (hdl 0x%px)(idx=%d)\n", __entry->handle, __entry->buff_idx));

TRACE_EVENT(isp_hw_end, TP_PROTO(void *vfh, int buff_idx),
	    TP_ARGS(vfh, buff_idx),
	    TP_STRUCT__entry(__field(void *, handle) __field(int, buff_idx)),
	    TP_fast_assign(__entry->handle = vfh; __entry->buff_idx = buff_idx;),
	    TP_printk("(isp) hw end (hdl 0x%px)(idx=%d)\n", __entry->handle, __entry->buff_idx));

#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../../drivers/media/platform/sunplus/isp
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE isp_trace
#include <trace/define_trace.h>
