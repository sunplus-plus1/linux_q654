
#ifndef _dhd_config_
#define _dhd_config_

#include <bcmdevs.h>
#include <bcmdevs_legacy.h>
#include <siutils.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <wlioctl.h>
#include <802.11.h>

/* message levels */
#define CONFIG_ERROR_LEVEL	(1 << 0)
#define CONFIG_TRACE_LEVEL	(1 << 1)
#define CONFIG_INFO_LEVEL	(1 << 2)
#define CONFIG_MSG_LEVEL	(1 << 0)

#define FW_TYPE_STA     0
#define FW_TYPE_APSTA   1
#define FW_TYPE_P2P     2
#define FW_TYPE_MESH    3
#define FW_TYPE_EZMESH  4
#define FW_TYPE_ES      5
#define FW_TYPE_MFG     6

#define FW_PATH_AUTO_SELECT 1
#ifdef BCMDHD_MDRIVER
#define CONFIG_PATH_AUTO_SELECT
#else
//#define CONFIG_PATH_AUTO_SELECT
#endif
extern char firmware_path[MOD_PARAM_PATHLEN];
#ifdef RMMOD_POWER_DOWN_LATER
extern atomic_t exit_in_progress;
extern bool is_power_on;
#endif
#if defined(BCMSDIO)
extern uint dhd_rxbound;
extern uint dhd_txbound;
#endif
#ifdef BCMSDIO
#define TXGLOM_RECV_OFFSET 8
extern uint dhd_doflow;
extern uint dhd_slpauto;
#endif

#ifdef SET_FWNV_BY_MAC
typedef struct wl_mac_range {
	uint32 oui;
	uint32 nic_start;
	uint32 nic_end;
} wl_mac_range_t;

typedef struct wl_mac_list {
	int count;
	wl_mac_range_t *mac;
	char name[MOD_PARAM_PATHLEN];
} wl_mac_list_t;

typedef struct wl_mac_list_ctrl {
	int count;
	struct wl_mac_list *m_mac_list_head;
} wl_mac_list_ctrl_t;
#endif

typedef struct wl_chip_nv_path {
	uint chip;
	uint chiprev;
	char name[MOD_PARAM_PATHLEN];
} wl_chip_nv_path_t;

typedef struct wl_chip_nv_path_list_ctrl {
	int count;
	struct wl_chip_nv_path *m_chip_nv_path_head;
} wl_chip_nv_path_list_ctrl_t;

typedef struct wmes_param {
	int aifsn[AC_COUNT];
	int ecwmin[AC_COUNT];
	int ecwmax[AC_COUNT];
	int txop[AC_COUNT];
} wme_param_t;

#ifdef PKT_FILTER_SUPPORT
#define DHD_CONF_FILTER_MAX	8
#define PKT_FILTER_LEN 300
#define MAGIC_PKT_FILTER_LEN 450
typedef struct conf_pkt_filter_add {
	uint32 count;
	char filter[DHD_CONF_FILTER_MAX][PKT_FILTER_LEN];
} conf_pkt_filter_add_t;

typedef struct conf_pkt_filter_del {
	uint32 count;
	uint32 id[DHD_CONF_FILTER_MAX];
} conf_pkt_filter_del_t;
#endif

#define CONFIG_COUNTRY_LIST_SIZE 500
typedef struct country_list {
	struct country_list *next;
	wl_country_t cspec;
} country_list_t;

typedef struct wl_ccode_all {
	wl_country_t cspec;
	int32 ww_2g_chan_only;
	int32 disable_5g_band;
	int32 disable_6g_band;
} wl_ccode_all_t;

/* mchan_params */
#define MCHAN_MAX_NUM 4
#define MIRACAST_SOURCE	1
#define MIRACAST_SINK	2
typedef struct mchan_params {
	struct mchan_params *next;
	int bw;
	int p2p_mode;
	int miracast_mode;
} mchan_params_t;

typedef enum MCHAN_MODE {
	MCHAN_AUTO = -1,	/* Auto selection by Chip */
	MCHAN_SCC = 0,		/* Same Channel Concurrent */
	MCHAN_SBSC = 1,		/* Same Band Same Channel concurrent */
	MCHAN_MCC = 2,		/* Multiple Channel Concurrent */
	MCHAN_RSDB = 3		/* RSDB concurrent */
} mchan_mode_t;

#ifdef SCAN_SUPPRESS
enum scan_intput_flags {
	NO_SCAN_INTPUT	= (1 << (0)),
	SCAN_CURCHAN_INTPUT	= (1 << (1)),
	SCAN_LIGHT_INTPUT	= (1 << (2)),
};
#endif

enum war_flags {
	SET_CHAN_INCONN	= (1 << (0)),
	FW_REINIT_INCSA	= (1 << (1)),
	FW_REINIT_EMPTY_SCAN	= (1 << (2)),
	P2P_AP_MAC_CONFLICT	= (1 << (3)),
	RESEND_EAPOL_PKT	= (1 << (4)),
	FW_REINIT_RXF0OVFL	= (1 << (5))
};

enum in4way_flags {
	STA_NO_SCAN_IN4WAY	= (1 << (0)),
	STA_NO_BTC_IN4WAY	= (1 << (1)),
	STA_WAIT_DISCONNECTED	= (1 << (2)),
	AP_WAIT_STA_RECONNECT	= (1 << (3)),
	STA_FAKE_SCAN_IN_CONNECT	= (1 << (4)),
	STA_REASSOC_RETRY	= (1 << (5)),
};

enum in_suspend_flags {
	NO_EVENT_IN_SUSPEND		= (1 << (0)),
	NO_TXDATA_IN_SUSPEND	= (1 << (1)),
	NO_TXCTL_IN_SUSPEND		= (1 << (2)),
	AP_DOWN_IN_SUSPEND		= (1 << (3)),
	ROAM_OFFLOAD_IN_SUSPEND	= (1 << (4)),
	AP_FILTER_IN_SUSPEND	= (1 << (5)),
	WOWL_IN_SUSPEND			= (1 << (6)),
	ALL_IN_SUSPEND 			= 0xFFFFFFFF,
};

enum in_suspend_mode {
	EARLY_SUSPEND = 0,
	PM_NOTIFIER = 1,
	SUSPEND_MODE_2 = 2
};

enum hostsleep_mode {
	HOSTSLEEP_CLEAR = 0,
	HOSTSLEEP_FW_SET = 1,
	HOSTSLEEP_DHD_SET = 2,
};

enum conn_state {
	CONN_STATE_IDLE = 0,
	CONN_STATE_CONNECTING = 1,
	CONN_STATE_AUTH_SAE_M1 = 2,
	CONN_STATE_AUTH_SAE_M2 = 3,
	CONN_STATE_AUTH_SAE_M3 = 4,
	CONN_STATE_AUTH_SAE_M4 = 5,
	CONN_STATE_ASSOCIATED = 6,
	CONN_STATE_REQID = 7,
	CONN_STATE_RSPID = 8,
	CONN_STATE_WSC_START = 9,
	CONN_STATE_WPS_M1 = 10,
	CONN_STATE_WPS_M2 = 11,
	CONN_STATE_WPS_M3 = 12,
	CONN_STATE_WPS_M4 = 13,
	CONN_STATE_WPS_M5 = 14,
	CONN_STATE_WPS_M6 = 15,
	CONN_STATE_WPS_M7 = 16,
	CONN_STATE_WPS_M8 = 17,
	CONN_STATE_WSC_DONE = 18,
	CONN_STATE_4WAY_M1 = 19,
	CONN_STATE_4WAY_M2 = 20,
	CONN_STATE_4WAY_M3 = 21,
	CONN_STATE_4WAY_M4 = 22,
	CONN_STATE_ADD_KEY = 23,
	CONN_STATE_CONNECTED = 24,
	CONN_STATE_GROUPKEY_M1 = 25,
	CONN_STATE_GROUPKEY_M2 = 26,
};

enum enq_pkt_type {
	ENQ_PKT_TYPE_EAPOL	= (1 << (0)),
	ENQ_PKT_TYPE_ARP	= (1 << (1)),
	ENQ_PKT_TYPE_DHCP	= (1 << (2)),
	ENQ_PKT_TYPE_ICMP	= (1 << (3)),
};

enum path_type {
	PATH_BY_CHIP = 0,
	PATH_BY_CHIP_BUS = 1,
	PATH_BY_MODULE = 2,
};

typedef struct dhd_conf {
	uint devid;
	uint chip;
	uint chiprev;
#if defined(BCMPCIE)
	uint svid;
	uint ssid;
#endif
#ifdef GET_OTP_MODULE_NAME
	char module_name[16];
#endif
	struct ether_addr otp_mac;
	int fw_type;
#ifdef SET_FWNV_BY_MAC
	wl_mac_list_ctrl_t fw_by_mac;
	wl_mac_list_ctrl_t nv_by_mac;
#endif
	wl_chip_nv_path_list_ctrl_t nv_by_chip;
	country_list_t *country_head;
	char *ccode_all_list;
	wl_ccode_all_t ccode_all;
	int ioctl_ver;
	int band;
	int bw_cap[2];
	int ap_mchan_mode;
	int go_mchan_mode;
	int csa;
	wl_country_t cspec;
	bool wbtext;
	bool fw_wbtext;
	uint roam_off;
	uint roam_off_suspend;
	int roam_trigger[2];
	int roam_scan_period[2];
	int roam_delta[2];
	int fullroamperiod;
#ifdef WL_SCHED_SCAN
	int max_sched_scan_reqs;
#endif /* WL_SCHED_SCAN */
	uint keep_alive_period;
	bool rekey_offload;
#ifdef ARP_OFFLOAD_SUPPORT
	bool garp;
#endif
	int force_wme_ac;
	wme_param_t wme_sta;
	wme_param_t wme_ap;
#ifdef PKT_FILTER_SUPPORT
	conf_pkt_filter_add_t pkt_filter_add;
	conf_pkt_filter_del_t pkt_filter_del;
	char *magic_pkt_filter_add;
	int magic_pkt_hdr_len;
	int pkt_filter_cnt_default;
#endif
	int srl;
	int lrl;
	uint bcn_timeout;
	int disable_proptx;
	int dhd_poll;
#ifdef BCMSDIO
	int use_rxchain;
	bool bus_rxglom;
	bool txglom_ext; /* Only for 43362/4330/43340/43341/43241 */
	/* terence 20161011:
	    1) conf->tx_max_offset = 1 to fix credict issue in adaptivity testing
	    2) conf->tx_max_offset = 1 will cause to UDP Tx not work in rxglom supported,
	        but not happened in sw txglom
	*/
	int tx_max_offset;
	uint txglomsize;
	int txctl_tmo_fix;
	bool txglom_mode;
	uint deferred_tx_len;
	/*txglom_bucket_size:
	 * 43362/4330: 1680
	 * 43340/43341/43241: 1684
	 */
	int txglom_bucket_size;
	int txinrx_thres;
	int dhd_txminmax; // -1=DATABUFCNT(bus)
#ifdef DYNAMIC_MAX_HDR_READ
	int max_hdr_read;
#endif
	bool oob_enabled_later;
#if defined(SDIO_ISR_THREAD)
	bool intr_extn;
#endif
#ifdef BCMSDIO_RXLIM_POST
	bool rxlim_en;
#endif
#ifdef BCMSDIO_TXSEQ_SYNC
	bool txseq_sync;
#endif
#ifdef BCMSDIO_INTSTATUS_WAR
	uint read_intr_mode;
#endif
	int kso_try_max;
#ifdef KSO_DEBUG
	uint kso_try_array[10];
#endif
#endif
#ifdef BCMPCIE
	int bus_deepsleep_disable;
	int flow_ring_queue_threshold;
	int d2h_intr_method;
	int d2h_intr_control;
	int enq_hdr_pkt;
	int aspm;
	int l1ss;
#endif
	int dpc_cpucore;
	int rxf_cpucore;
	int dhd_dpc_prio;
	int frameburst;
	bool deepsleep;
	int pm;
	int pm_in_suspend;
	int suspend_mode;
	int suspend_bcn_li_dtim;
#ifdef DHDTCPACK_SUPPRESS
	uint8 tcpack_sup_mode;
	uint32 tcpack_sup_ratio;
	uint32 tcpack_sup_delay;
#endif
	int pktprio8021x;
	uint insuspend;
	bool suspended;
	struct ether_addr bssid_insuspend;
#ifdef SUSPEND_EVENT
	char resume_eventmask[WL_EVENTING_MASK_LEN];
	bool wlfc;
#endif
#ifdef IDHCP
	int dhcpc_enable;
	int dhcpd_enable;
	struct ipv4_addr dhcpd_ip_addr;
	struct ipv4_addr dhcpd_ip_mask;
	struct ipv4_addr dhcpd_ip_start;
	struct ipv4_addr dhcpd_ip_end;
#endif
#ifdef ISAM_PREINIT
	char isam_init[50];
	char isam_config[300];
	char isam_enable[50];
#endif
	int ctrl_resched;
	uint rxcnt_timeout;
	mchan_params_t *mchan;
	char *wl_preinit;
	char *wl_suspend;
	char *wl_resume;
	uint in4way;
	char *wl_pre_in4way;
	char *wl_post_in4way;
	uint war;
#ifdef WL_EXT_WOWL
	uint wowl;
#ifdef BCMDBUS
	uint wowl_dngldown;
#endif
#endif
#ifdef GET_CUSTOM_MAC_FROM_CONFIG
	char hw_ether[62];
#endif
	wait_queue_head_t event_complete;
#ifdef PROPTX_MAXCOUNT
	int proptx_maxcnt_2g;
	int proptx_maxcnt_5g;
#endif /* DYNAMIC_PROPTX_MAXCOUNT */
#ifdef TPUT_MONITOR
	uint tput_monitor_ms;
	struct osl_timespec tput_ts;
	unsigned long last_tx;
	unsigned long last_rx;
#ifdef BCMSDIO
	int32 doflow_tput_thresh;
#endif
#endif
#ifdef SCAN_SUPPRESS
	uint scan_intput;
	int scan_busy_thresh;
	int scan_busy_tmo;
	int32 scan_tput_thresh;
#endif
#ifdef DHD_TPUT_PATCH
	bool tput_patch;
	int mtu;
	bool pktsetsum;
#endif
#ifdef SET_XPS_CPUS
	char *xps_cpus;
#endif
#ifdef SET_RPS_CPUS
	char *rps_cpus;
#endif
#ifdef CHECK_DOWNLOAD_FW
	bool fwchk;
#endif
	char *vndr_ie_assocreq;
} dhd_conf_t;

#ifdef BCMSDIO
void dhd_conf_get_otp(dhd_pub_t *dhd, bcmsdh_info_t *sdh, si_t *sih);
void dhd_conf_set_txglom_params(dhd_pub_t *dhd, bool enable);
bool dhd_conf_legacy_otp_chip(dhd_pub_t *dhd);
bool dhd_conf_syna_secure_chip(dhd_pub_t *dhd);
#endif
#ifdef BCMPCIE
bool dhd_conf_legacy_msi_chip(dhd_pub_t *dhd);
#if defined(BCMPCIE_CTO_PREVENTION)
bool dhd_conf_legacy_cto_chip(uint16 chip);
#endif
#endif
bool dhd_conf_get_csa(dhd_pub_t *dhd);
#ifdef WL_CFG80211
bool dhd_conf_legacy_chip_check(dhd_pub_t *dhd);
bool dhd_conf_new_chip_check(dhd_pub_t *dhd);
bool dhd_conf_extsae_chip(dhd_pub_t *dhd);
#endif
void dhd_conf_set_path(dhd_pub_t *dhd, char *dst_path, char *src_path,
	char *prefix, char *file_ext, int path_type);
void dhd_conf_update_path(dhd_pub_t *dhd);
void dhd_conf_set_path_params(dhd_pub_t *dhd);
int dhd_conf_set_intiovar(dhd_pub_t *dhd, int ifidx, uint cmd, char *name,
	int val, int def, bool down);
int dhd_conf_get_band(dhd_pub_t *dhd);
bool dhd_conf_same_country(dhd_pub_t *dhd, char *buf);
int dhd_conf_country(dhd_pub_t *dhd, char *cmd, char *buf);
int dhd_conf_get_country(dhd_pub_t *dhd, wl_country_t *cspec);
#ifdef CCODE_LIST
int dhd_ccode_map_country_all(dhd_pub_t *dhd, wl_country_t *cspec);
int dhd_ccode_map_country_list(dhd_pub_t *dhd, wl_country_t *cspec);
#endif
int dhd_conf_roam_prof(dhd_pub_t *dhd, int ifidx);
void dhd_conf_set_roam(dhd_pub_t *dhd, int ifidx);
void dhd_conf_set_wme(dhd_pub_t *dhd, int ifidx, int mode);
void dhd_conf_set_mchan_bw(dhd_pub_t *dhd, int go, int source);
void dhd_conf_add_pkt_filter(dhd_pub_t *dhd);
bool dhd_conf_del_pkt_filter(dhd_pub_t *dhd, uint32 id);
void dhd_conf_discard_pkt_filter(dhd_pub_t *dhd);
int dhd_conf_read_config(dhd_pub_t *dhd, char *conf_path);
int dhd_conf_set_chiprev(dhd_pub_t *dhd, uint chip, uint chiprev);
uint dhd_conf_get_chip(void *context);
uint dhd_conf_get_chiprev(void *context);
int dhd_conf_get_pm(dhd_pub_t *dhd);
int dhd_conf_custom_mac(dhd_pub_t *dhd);
int dhd_conf_reg2args(dhd_pub_t *dhd, char *cmd, bool set, uint32 index, uint32 *val);
bool dhd_conf_set_wl_cmd(dhd_pub_t *dhd, char *data, bool down);
int dhd_conf_check_hostsleep(dhd_pub_t *dhd, int cmd, void *buf, int len,
	int *hostsleep_set, int *hostsleep_val, int *ret);
void dhd_conf_get_hostsleep(dhd_pub_t *dhd,
	int hostsleep_set, int hostsleep_val, int ret);
int dhd_conf_mkeep_alive(dhd_pub_t *dhd, int ifidx, int id, int period,
	char *packet, bool bcast);
#ifdef ARP_OFFLOAD_SUPPORT
void dhd_conf_set_garp(dhd_pub_t *dhd, int ifidx, uint32 ipa, bool enable);
#endif
#ifdef PROP_TXSTATUS
int dhd_conf_get_disable_proptx(dhd_pub_t *dhd);
#endif
#ifdef TPUT_MONITOR
void dhd_conf_tput_monitor(dhd_pub_t *dhd);
#endif
uint dhd_conf_get_insuspend(dhd_pub_t *dhd, uint mask);
int dhd_conf_set_suspend_resume(dhd_pub_t *dhd, int suspend);
void dhd_conf_postinit_ioctls(dhd_pub_t *dhd);
void dhd_conf_preinit_ioctls_sta(dhd_pub_t *dhd, int ifidx);
int dhd_conf_preinit(dhd_pub_t *dhd);
int dhd_conf_reset(dhd_pub_t *dhd);
int dhd_conf_attach(dhd_pub_t *dhd);
void dhd_conf_detach(dhd_pub_t *dhd);
void *dhd_get_pub(struct net_device *dev);
int wl_pattern_atoh(char *src, char *dst);
int dhd_conf_suspend_resume_sta(dhd_pub_t *dhd, int ifidx, int suspend);
/* Add to adjust 802.1x priority */
extern void pktset8021xprio(void *pkt, int prio);
#if defined(BCMSDIO) || defined(BCMPCIE) || defined(BCMDBUS)
extern int dhd_bus_sleep(dhd_pub_t *dhdp, bool sleep, uint32 *intstatus);
#endif
#ifdef WL_EXT_WOWL
int dhd_conf_wowl_dngldown(dhd_pub_t *dhd);
#endif
#endif /* _dhd_config_ */
