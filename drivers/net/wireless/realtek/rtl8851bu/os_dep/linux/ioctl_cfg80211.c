/******************************************************************************
 *
 * Copyright(c) 2007 - 2019 Realtek Corporation.
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
#define  _IOCTL_CFG80211_C_

#include <drv_types.h>

#ifdef CONFIG_IOCTL_CFG80211

#ifndef DBG_RTW_CFG80211_STA_PARAM
#define DBG_RTW_CFG80211_STA_PARAM 0
#endif

#ifndef DBG_RTW_CFG80211_MESH_CONF
#define DBG_RTW_CFG80211_MESH_CONF 0
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
#define STATION_INFO_INACTIVE_TIME	BIT(NL80211_STA_INFO_INACTIVE_TIME)
#define STATION_INFO_RX_BYTES		BIT(NL80211_STA_INFO_RX_BYTES)
#define STATION_INFO_TX_BYTES		BIT(NL80211_STA_INFO_TX_BYTES)
#define STATION_INFO_LLID			BIT(NL80211_STA_INFO_LLID)
#define STATION_INFO_PLID			BIT(NL80211_STA_INFO_PLID)
#define STATION_INFO_PLINK_STATE	BIT(NL80211_STA_INFO_PLINK_STATE)
#define STATION_INFO_SIGNAL			BIT(NL80211_STA_INFO_SIGNAL)
#define STATION_INFO_TX_BITRATE		BIT(NL80211_STA_INFO_TX_BITRATE)
#define STATION_INFO_RX_PACKETS		BIT(NL80211_STA_INFO_RX_PACKETS)
#define STATION_INFO_TX_PACKETS		BIT(NL80211_STA_INFO_TX_PACKETS)
#define STATION_INFO_TX_RETRIES		BIT(NL80211_STA_INFO_TX_RETRIES)
#define STATION_INFO_TX_FAILED		BIT(NL80211_STA_INFO_TX_FAILED)
#define STATION_INFO_RX_BITRATE		BIT(NL80211_STA_INFO_RX_BITRATE)
#define STATION_INFO_LOCAL_PM		BIT(NL80211_STA_INFO_LOCAL_PM)
#define STATION_INFO_PEER_PM		BIT(NL80211_STA_INFO_PEER_PM)
#define STATION_INFO_NONPEER_PM		BIT(NL80211_STA_INFO_NONPEER_PM)
#define STATION_INFO_RX_BYTES64		BIT(NL80211_STA_INFO_RX_BYTES64)
#define STATION_INFO_TX_BYTES64		BIT(NL80211_STA_INFO_TX_BYTES64)
#define STATION_INFO_ASSOC_REQ_IES	0
#endif /* Linux kernel >= 4.0.0 */

#define RTW_MAX_MGMT_TX_CNT (8)
#ifndef RTW_MAX_MGMT_TX_MS_GAS
#define RTW_MAX_MGMT_TX_MS_GAS (500)
#endif /*RTW_MAX_MGMT_TX_MS_GAS*/
#define RTW_SCAN_IE_LEN_MAX      2304
#define RTW_MAX_REMAIN_ON_CHANNEL_DURATION 5000 /* ms */
#define RTW_MAX_NUM_PMKIDS 4

#define RTW_CH_MAX_2G_CHANNEL               14      /* Max channel in 2G band */

#ifdef CONFIG_WAPI_SUPPORT

#ifndef WLAN_CIPHER_SUITE_SMS4
#define WLAN_CIPHER_SUITE_SMS4          0x00147201
#endif

#ifndef WLAN_AKM_SUITE_WAPI_PSK
#define WLAN_AKM_SUITE_WAPI_PSK         0x000FAC04
#endif

#ifndef WLAN_AKM_SUITE_WAPI_CERT
#define WLAN_AKM_SUITE_WAPI_CERT        0x000FAC12
#endif

#ifndef NL80211_WAPI_VERSION_1
#define NL80211_WAPI_VERSION_1          (1 << 2)
#endif

#endif /* CONFIG_WAPI_SUPPORT */

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(4, 11, 12))
#ifdef CONFIG_RTW_80211R
#define WLAN_AKM_SUITE_FT_8021X		0x000FAC03
#define WLAN_AKM_SUITE_FT_PSK			0x000FAC04
#endif
#endif

#define WIFI_CIPHER_SUITE_GCMP		0x000FAC08
#define WIFI_CIPHER_SUITE_GCMP_256	0x000FAC09
#define WIFI_CIPHER_SUITE_CCMP_256	0x000FAC0A
#define WIFI_CIPHER_SUITE_BIP_GMAC_128	0x000FAC0B
#define WIFI_CIPHER_SUITE_BIP_GMAC_256	0x000FAC0C
#define WIFI_CIPHER_SUITE_BIP_CMAC_256	0x000FAC0D

/*
 * If customer need, defining this flag will make driver 
 * always return -EBUSY at the condition of scan deny.
 */
/* #define CONFIG_NOTIFY_SCAN_ABORT_WITH_BUSY */

static const u32 rtw_cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
#ifdef CONFIG_WAPI_SUPPORT
	WLAN_CIPHER_SUITE_SMS4,
#endif /* CONFIG_WAPI_SUPPORT */
#ifdef CONFIG_IEEE80211W
	WLAN_CIPHER_SUITE_AES_CMAC,
	WIFI_CIPHER_SUITE_GCMP,
	WIFI_CIPHER_SUITE_GCMP_256,
	WIFI_CIPHER_SUITE_CCMP_256,
	WIFI_CIPHER_SUITE_BIP_GMAC_128,
	WIFI_CIPHER_SUITE_BIP_GMAC_256,
	WIFI_CIPHER_SUITE_BIP_CMAC_256,
#endif /* CONFIG_IEEE80211W */
};

#define RATETAB_ENT(_rate, _rateid, _flags) \
	{								\
		.bitrate	= (_rate),				\
		.hw_value	= (_rateid),				\
		.flags		= (_flags),				\
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
/* if wowlan is not supported, kernel generate a disconnect at each suspend
 * cf: /net/wireless/sysfs.c, so register a stub wowlan.
 * Moreover wowlan has to be enabled via a the nl80211_set_wowlan callback.
 * (from user space, e.g. iw phy0 wowlan enable)
 */
static const struct wiphy_wowlan_support wowlan_stub = {
	.flags = WIPHY_WOWLAN_ANY,
	.n_patterns = 0,
	.pattern_max_len = 0,
	.pattern_min_len = 0,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	.max_pkt_offset = 0,
#endif
};
#endif

static const struct ieee80211_rate rtw_rates[] = {
	RATETAB_ENT(10,  0x1,   0),
	RATETAB_ENT(20,  0x2,   0),
	RATETAB_ENT(55,  0x4,   0),
	RATETAB_ENT(110, 0x8,   0),
	RATETAB_ENT(60,  0x10,  0),
	RATETAB_ENT(90,  0x20,  0),
	RATETAB_ENT(120, 0x40,  0),
	RATETAB_ENT(180, 0x80,  0),
	RATETAB_ENT(240, 0x100, 0),
	RATETAB_ENT(360, 0x200, 0),
	RATETAB_ENT(480, 0x400, 0),
	RATETAB_ENT(540, 0x800, 0),
};

#define rtw_a_rates		(rtw_rates + 4)
#define RTW_A_RATES_NUM	8
#define rtw_g_rates		(rtw_rates + 0)
#define RTW_G_RATES_NUM	12

static int rtw_cfg80211_set_assocresp_ies(struct net_device *net, const u8 *buf, int len);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
static u8 rtw_chdef_to_cfg80211_chan_def(struct wiphy *wiphy,
		struct cfg80211_chan_def *chdef,
		struct rtw_chan_def *rtw_chdef,
		u8 ht)
{
	return rtw_bchbw_to_cfg80211_chan_def(wiphy, chdef
		, rtw_chdef->band, rtw_chdef->chan, rtw_chdef->bw, rtw_chdef->offset, ht);
}

static void rtw_get_chdef_from_cfg80211_chan_def(struct cfg80211_chan_def *chdef,
		u8 *ht, struct rtw_chan_def *rtw_chdef)
{
	u8 bw, offset;

	rtw_get_bchbw_from_cfg80211_chan_def(chdef
		, ht, &rtw_chdef->band, &rtw_chdef->chan, &bw, &offset);

	rtw_chdef->bw = bw;
	rtw_chdef->offset = offset;
}
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29))
static enum nl80211_channel_type rtw_chdef_to_nl80211_channel_type(struct rtw_chan_def *rtw_chdef, u8 ht)
{
	return rtw_bchbw_to_nl80211_channel_type(rtw_chdef->band, rtw_chdef->chan, rtw_chdef->bw, rtw_chdef->offset, ht);
}

static void rtw_get_chdef_from_nl80211_channel_type(struct ieee80211_channel *chan,
			enum nl80211_channel_type ctype,
			u8 *ht, struct rtw_chan_def *rtw_chdef)
{
	u8 bw, offset;

	rtw_get_bchbw_from_nl80211_channel_type(chan, ctype
		, &rtw_chdef->band, ht, &rtw_chdef->chan, &bw, &offset);

	rtw_chdef->bw = bw;
	rtw_chdef->offset = offset;
}
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)) */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
bool rtw_cfg80211_allow_ch_switch_notify(_adapter *adapter)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0))
	if ((!MLME_IS_AP(adapter))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
		&& (!MLME_IS_ADHOC(adapter))
		&& (!MLME_IS_ADHOC_MASTER(adapter))
		&& (!MLME_IS_MESH(adapter))
#elif defined(CONFIG_RTW_MESH)
		&& (!MLME_IS_MESH(adapter))
#endif
		)
		return 0;
#endif
	return 1;
}

u8 rtw_cfg80211_ch_switch_notify(_adapter *adapter,
					struct _ADAPTER_LINK *alink,
					struct rtw_chan_def *rtw_chdef,
					u8 ht, bool started)
{
	struct wiphy *wiphy = adapter_to_wiphy(adapter);
	u8 ret = _SUCCESS;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
	unsigned int link_id = 0;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	struct cfg80211_chan_def chdef;
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	u16 punct_bitmap = 0; /*TBD*/
	#endif

	ret = rtw_chdef_to_cfg80211_chan_def(wiphy, &chdef, rtw_chdef, ht);
	if (ret != _SUCCESS)
		goto exit;

	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0))
	if (started) {
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
		cfg80211_ch_switch_started_notify(adapter->pnetdev, &chdef, link_id, 0, false, punct_bitmap);
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
		cfg80211_ch_switch_started_notify(adapter->pnetdev, &chdef, link_id, 0, false);
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))
		/* --- cfg80211_ch_switch_started_notfiy() ---
		 *  A new parameter, bool quiet, is added from Linux kernel v5.11,
		 *  to see if block-tx was requested by the AP. since currently,
		 *  the API is used for station before connected in rtw_chk_start_clnt_join()
		 *  the quiet is set to false here first. May need to refine it if
		 *  called by others with block-tx.
		 */
		cfg80211_ch_switch_started_notify(adapter->pnetdev, &chdef, 0, false);
		#else
		cfg80211_ch_switch_started_notify(adapter->pnetdev, &chdef, 0);
		#endif
		goto exit;
	}
	#endif

	if (!rtw_cfg80211_allow_ch_switch_notify(adapter))
		goto exit;

	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	cfg80211_ch_switch_notify(adapter->pnetdev, &chdef, link_id, punct_bitmap);
	#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 2))
	/* ToDo CONFIG_RTW_MLD */
	cfg80211_ch_switch_notify(adapter->pnetdev, &chdef, link_id);
	#else
	cfg80211_ch_switch_notify(adapter->pnetdev, &chdef);
	#endif

#else
	int freq = rtw_bch2freq(rtw_chdef->band, rtw_chdef->chan);
	enum nl80211_channel_type ctype;

	if (!rtw_cfg80211_allow_ch_switch_notify(adapter))
		goto exit;

	if (!freq) {
		ret = _FAIL;
		goto exit;
	}

	ctype = rtw_chdef_to_nl80211_channel_type(rtw_chdef, ht);
	cfg80211_ch_switch_notify(adapter->pnetdev, freq, ctype);
#endif

exit:
	return ret;
}
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)) */

static void rtw_2g_channels_init(struct ieee80211_channel *channels)
{
	_rtw_memcpy(channels, rtw_2ghz_channels, sizeof(rtw_2ghz_channels));
}

#if CONFIG_IEEE80211_BAND_5GHZ
static void rtw_5g_channels_init(struct ieee80211_channel *channels)
{
	_rtw_memcpy(channels, rtw_5ghz_a_channels, sizeof(rtw_5ghz_a_channels));
}
#endif

#if CONFIG_IEEE80211_BAND_6GHZ
static void rtw_6g_channels_init(struct ieee80211_channel *channels)
{
	_rtw_memcpy(channels, rtw_6ghz_channels, sizeof(rtw_6ghz_channels));
}
#endif

void rtw_2g_rates_init(struct ieee80211_rate *rates)
{
	_rtw_memcpy(rates, rtw_g_rates,
		sizeof(struct ieee80211_rate) * RTW_G_RATES_NUM
	);
}

void rtw_5g_rates_init(struct ieee80211_rate *rates)
{
	_rtw_memcpy(rates, rtw_a_rates,
		sizeof(struct ieee80211_rate) * RTW_A_RATES_NUM
	);
}

#if CONFIG_IEEE80211_BAND_6GHZ
void rtw_6g_rates_init(struct ieee80211_rate *rates)
{
	_rtw_memcpy(rates, rtw_a_rates,
		sizeof(struct ieee80211_rate) * RTW_A_RATES_NUM
	);
}
#endif

struct ieee80211_supported_band *rtw_spt_band_alloc(enum band_type band)
{
	struct ieee80211_supported_band *spt_band = NULL;
	int n_channels, n_bitrates;

	if (rtw_band_to_nl80211_band(band) == NUM_NL80211_BANDS)
		goto exit;

	if (band == BAND_ON_24G) {
		n_channels = MAX_CHANNEL_NUM_2G;
		n_bitrates = RTW_G_RATES_NUM;
	} else if (band == BAND_ON_5G) {
		n_channels = MAX_CHANNEL_NUM_5G;
		n_bitrates = RTW_A_RATES_NUM;
#if CONFIG_IEEE80211_BAND_6GHZ
	} else if (band == BAND_ON_6G) {
		n_channels = MAX_CHANNEL_NUM_6G;
		n_bitrates = RTW_A_RATES_NUM;
#endif
	} else
		goto exit;

	spt_band = (struct ieee80211_supported_band *)rtw_zmalloc(
		sizeof(struct ieee80211_supported_band)
		+ sizeof(struct ieee80211_channel) * n_channels
		+ sizeof(struct ieee80211_rate) * n_bitrates
#if defined(CONFIG_80211AX_HE) && (defined(CPTCFG_VERSION) || (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)))
		+ sizeof(struct ieee80211_sband_iftype_data) * 2
#endif /* defined(CONFIG_80211AX_HE) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0) */
	);
	if (!spt_band)
		goto exit;

	spt_band->channels = (struct ieee80211_channel *)(((u8 *)spt_band) + sizeof(struct ieee80211_supported_band));
	spt_band->bitrates = (struct ieee80211_rate *)(((u8 *)spt_band->channels) + sizeof(struct ieee80211_channel) * n_channels);
	spt_band->band = rtw_band_to_nl80211_band(band);
	spt_band->n_channels = n_channels;
	spt_band->n_bitrates = n_bitrates;
#if defined(CONFIG_80211AX_HE) && (defined(CPTCFG_VERSION) || (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)))
	spt_band->iftype_data = (struct ieee80211_sband_iftype_data *)(((u8 *)spt_band->bitrates)
	                        + sizeof(struct ieee80211_rate) * n_bitrates);
	spt_band->n_iftype_data = 0;
#endif /* defined(CONFIG_80211AX_HE) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0) */

	if (band == BAND_ON_24G) {
		rtw_2g_channels_init(spt_band->channels);
		rtw_2g_rates_init(spt_band->bitrates);
	}
#if CONFIG_IEEE80211_BAND_5GHZ
	else if (band == BAND_ON_5G) {
		rtw_5g_channels_init(spt_band->channels);
		rtw_5g_rates_init(spt_band->bitrates);
	}
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	else if (band == BAND_ON_6G) {
		rtw_6g_channels_init(spt_band->channels);
		rtw_6g_rates_init(spt_band->bitrates);
	}
#endif

	/* spt_band.ht_cap */

exit:

	return spt_band;
}

void rtw_spt_band_free(struct ieee80211_supported_band *spt_band)
{
	u32 size = 0;

	if (!spt_band)
		return;

	if (spt_band->band == NL80211_BAND_2GHZ) {
		size = sizeof(struct ieee80211_supported_band)
			+ sizeof(struct ieee80211_channel) * MAX_CHANNEL_NUM_2G
			+ sizeof(struct ieee80211_rate) * RTW_G_RATES_NUM;
	} else if (spt_band->band == NL80211_BAND_5GHZ) {
		size = sizeof(struct ieee80211_supported_band)
			+ sizeof(struct ieee80211_channel) * MAX_CHANNEL_NUM_5G
			+ sizeof(struct ieee80211_rate) * RTW_A_RATES_NUM;
	}
#if CONFIG_IEEE80211_BAND_6GHZ
	else if (spt_band->band == NL80211_BAND_6GHZ) {
		size = sizeof(struct ieee80211_supported_band)
			+ sizeof(struct ieee80211_channel) * MAX_CHANNEL_NUM_6G
			+ sizeof(struct ieee80211_rate) * RTW_A_RATES_NUM;
	}
#endif

#if defined(CONFIG_80211AX_HE) && (defined(CPTCFG_VERSION) || (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)))
	size +=	sizeof(struct ieee80211_sband_iftype_data) * 2;
#endif /* defined(CONFIG_80211AX_HE) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0) */

	rtw_mfree((u8 *)spt_band, size);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
static const struct ieee80211_txrx_stypes
	rtw_cfg80211_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_ADHOC] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_AP_VLAN] = {
		/* copy AP */
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
#if defined(RTW_DEDICATED_P2P_DEVICE)
	[NL80211_IFTYPE_P2P_DEVICE] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
#endif
#if defined(CONFIG_RTW_MESH)
	[NL80211_IFTYPE_MESH_POINT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4)
			| BIT(IEEE80211_STYPE_AUTH >> 4)
	},
#endif

};
#endif

NDIS_802_11_NETWORK_INFRASTRUCTURE nl80211_iftype_to_rtw_network_type(enum nl80211_iftype type)
{
	switch (type) {
	case NL80211_IFTYPE_ADHOC:
		return Ndis802_11IBSS;

	#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
	case NL80211_IFTYPE_P2P_CLIENT:
	#endif
	case NL80211_IFTYPE_STATION:
		return Ndis802_11Infrastructure;

#ifdef CONFIG_AP_MODE
	#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
	case NL80211_IFTYPE_P2P_GO:
	#endif
	case NL80211_IFTYPE_AP:
		return Ndis802_11APMode;
#endif

#ifdef CONFIG_RTW_MESH
	case NL80211_IFTYPE_MESH_POINT:
		return Ndis802_11_mesh;
#endif

#ifdef CONFIG_WIFI_MONITOR
	case NL80211_IFTYPE_MONITOR:
		return Ndis802_11Monitor;
#endif /* CONFIG_WIFI_MONITOR */

	default:
		return Ndis802_11InfrastructureMax;
	}
}

u32 nl80211_iftype_to_rtw_mlme_state(enum nl80211_iftype type)
{
	switch (type) {
	case NL80211_IFTYPE_ADHOC:
		return WIFI_ADHOC_STATE;

	#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
	case NL80211_IFTYPE_P2P_CLIENT:
	#endif
	case NL80211_IFTYPE_STATION:
		return WIFI_STATION_STATE;

#ifdef CONFIG_AP_MODE
	#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
	case NL80211_IFTYPE_P2P_GO:
	#endif
	case NL80211_IFTYPE_AP:
		return WIFI_AP_STATE;
#endif

#ifdef CONFIG_RTW_MESH
	case NL80211_IFTYPE_MESH_POINT:
		return WIFI_MESH_STATE;
#endif

	case NL80211_IFTYPE_MONITOR:
		return WIFI_MONITOR_STATE;

	default:
		return WIFI_NULL_STATE;
	}
}

static
enum role_type nl80211_iftype_to_rtw_phl_role_type(enum nl80211_iftype type)
{
	switch (type) {
	case NL80211_IFTYPE_ADHOC:
		return PHL_RTYPE_ADHOC;
	#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
	case NL80211_IFTYPE_P2P_CLIENT:
		return PHL_RTYPE_P2P_GC;
	#endif
	case NL80211_IFTYPE_STATION:
		return PHL_RTYPE_STATION;

#ifdef CONFIG_AP_MODE
	#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
	case NL80211_IFTYPE_P2P_GO:
		return PHL_RTYPE_P2P_GO;
	#endif
	case NL80211_IFTYPE_AP:
		return PHL_RTYPE_AP;
#endif

#ifdef CONFIG_RTW_MESH
	case NL80211_IFTYPE_MESH_POINT:
		return PHL_RTYPE_MESH;
#endif

	case NL80211_IFTYPE_MONITOR:
		return PHL_RTYPE_MONITOR;

	default:
		return PHL_RTYPE_NONE;
	}
}

static int rtw_cfg80211_sync_iftype(_adapter *adapter)
{
	struct wireless_dev *rtw_wdev = adapter->rtw_wdev;

	if (!(nl80211_iftype_to_rtw_mlme_state(rtw_wdev->iftype) & MLME_STATE(adapter))) {
		/* iftype and mlme state is not syc */
		NDIS_802_11_NETWORK_INFRASTRUCTURE network_type;

		network_type = nl80211_iftype_to_rtw_network_type(rtw_wdev->iftype);
		if (network_type != Ndis802_11InfrastructureMax) {

			rtw_set_802_11_infrastructure_mode(adapter, network_type, 0);
			rtw_setopmode_cmd(adapter, network_type, RTW_CMDF_WAIT_ACK);
		} else {
			rtw_warn_on(1);
			RTW_WARN(FUNC_ADPT_FMT" iftype:%u is not support\n", FUNC_ADPT_ARG(adapter), rtw_wdev->iftype);
			return _FAIL;
		}
	}

	return _SUCCESS;
}

static u64 rtw_get_systime_us(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
	return ktime_to_us(ktime_get_boottime());
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
	struct timespec ts;
	get_monotonic_boottime(&ts);
	return ((u64)ts.tv_sec * 1000000) + ts.tv_nsec / 1000;
#else
	struct timeval tv;
	do_gettimeofday(&tv);
	return ((u64)tv.tv_sec * 1000000) + tv.tv_usec;
#endif
}

/* Try to remove non target BSS's SR to reduce PBC overlap rate */
static int rtw_cfg80211_clear_wps_sr_of_non_target_bss(_adapter *padapter, struct wlan_network *pnetwork, struct cfg80211_ssid *req_ssid)
{
	int ret = 0;
	u8 *psr = NULL, sr = 0;
	NDIS_802_11_SSID *pssid = &pnetwork->network.Ssid;
	u32 wpsielen = 0;
	u8 *wpsie = NULL;

	if (pssid->SsidLength == req_ssid->ssid_len
		&& _rtw_memcmp(pssid->Ssid, req_ssid->ssid, req_ssid->ssid_len) == _TRUE)
		goto exit;

	wpsie = rtw_get_wps_ie(pnetwork->network.IEs + _FIXED_IE_LENGTH_
		, pnetwork->network.IELength - _FIXED_IE_LENGTH_, NULL, &wpsielen);
	if (wpsie && wpsielen > 0)
		psr = rtw_get_wps_attr_content(wpsie, wpsielen, WPS_ATTR_SELECTED_REGISTRAR, &sr, NULL);

	if (psr && sr) {
		if (0)
			RTW_INFO("clear sr of non target bss:%s("MAC_FMT")\n"
				, pssid->Ssid, MAC_ARG(pnetwork->network.MacAddress));
		*psr = 0; /* clear sr */
		ret = 1;
	}

exit:
	return ret;
}

#define MAX_BSSINFO_LEN 1000
struct cfg80211_bss *rtw_cfg80211_inform_bss(_adapter *padapter, struct wlan_network *pnetwork)
{
	struct ieee80211_channel *notify_channel;
	struct cfg80211_bss *bss = NULL;
	u16 band;
	u16 channel;
	u32 freq;
	u64 notify_timestamp;
	u16 notify_capability;
	u16 notify_interval;
	u8 *notify_ie;
	size_t notify_ielen;
	s32 notify_signal;
	/* u8 buf[MAX_BSSINFO_LEN]; */

	u8 *pbuf;
	size_t buf_size = MAX_BSSINFO_LEN;
	size_t len, bssinf_len = 0;
	struct rtw_ieee80211_hdr *pwlanhdr;
	unsigned short *fctrl;
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	struct wireless_dev *wdev = padapter->rtw_wdev;
	struct wiphy *wiphy = wdev->wiphy;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

	pbuf = rtw_zmalloc(buf_size);
	if (pbuf == NULL) {
		RTW_INFO("%s pbuf allocate failed  !!\n", __FUNCTION__);
		return bss;
	}

	/* RTW_INFO("%s\n", __func__); */

	bssinf_len = pnetwork->network.IELength + sizeof(struct rtw_ieee80211_hdr_3addr);
	if (bssinf_len > buf_size) {
		RTW_INFO("%s IE Length too long > %zu byte\n", __FUNCTION__, buf_size);
		goto exit;
	}

#ifndef CONFIG_WAPI_SUPPORT
	{
		u16 wapi_len = 0;

		if (rtw_get_wapi_ie(pnetwork->network.IEs, pnetwork->network.IELength, NULL, &wapi_len) > 0) {
			if (wapi_len > 0) {
				RTW_INFO("%s, no support wapi!\n", __FUNCTION__);
				goto exit;
			}
		}
	}
#endif /* !CONFIG_WAPI_SUPPORT */

	band = pnetwork->network.Configuration.Band;
	channel = pnetwork->network.Configuration.DSConfig;
	freq = rtw_bch2freq(band, channel);
	notify_channel = ieee80211_get_channel(wiphy, freq);

	if (0)
		notify_timestamp = le64_to_cpu(*(u64 *)rtw_get_timestampe_from_ie(pnetwork->network.IEs));
	else
		notify_timestamp = rtw_get_systime_us();

	notify_interval = le16_to_cpu(*(u16 *)rtw_get_beacon_interval_from_ie(pnetwork->network.IEs));
	notify_capability = le16_to_cpu(*(u16 *)rtw_get_capability_from_ie(pnetwork->network.IEs));

	notify_ie = pnetwork->network.IEs + _FIXED_IE_LENGTH_;
	notify_ielen = pnetwork->network.IELength - _FIXED_IE_LENGTH_;

	/*RTW_WKARD_CORE_RSSI_V1*/
	/* We've set wiphy's signal_type as CFG80211_SIGNAL_TYPE_MBM: signal strength in mBm (100*dBm) */
	if (check_fwstate(pmlmepriv, WIFI_ASOC_STATE) == _TRUE &&
	    is_same_network(&pmlmepriv->dev_cur_network.network, &pnetwork->network)) {
		notify_signal = 100 * rtw_phl_rssi_to_dbm(padapter->recvinfo.signal_strength); /* dbm */
	} else {
		notify_signal = 100 * rtw_phl_rssi_to_dbm(pnetwork->network.PhyInfo.SignalStrength); /* pnetwork->network.PhyInfo.rssi -dbm */
	}
#if 0
	RTW_INFO("bssid: "MAC_FMT", rssi:%d\t", MAC_ARG(pnetwork->network.MacAddress),notify_signal);
	RTW_INFO("ss:%d, sq:%d, orssi:%d\n",
		pnetwork->network.PhyInfo.SignalStrength,
		pnetwork->network.PhyInfo.SignalQuality,
		pnetwork->network.PhyInfo.rssi);
#endif
#if 0
	RTW_INFO("bssid: "MAC_FMT"\n", MAC_ARG(pnetwork->network.MacAddress));
	RTW_INFO("Channel: %d(%d)\n", channel, freq);
	RTW_INFO("Capability: %X\n", notify_capability);
	RTW_INFO("Beacon interval: %d\n", notify_interval);
	RTW_INFO("Signal: %d\n", notify_signal);
	RTW_INFO("notify_timestamp: %llu\n", notify_timestamp);
#endif

	/* pbuf = buf; */

	pwlanhdr = (struct rtw_ieee80211_hdr *)pbuf;
	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	SetSeqNum(pwlanhdr, 0/*pmlmeext->mgnt_seq*/);
	/* pmlmeext->mgnt_seq++; */

	if (pnetwork->network.Reserved[0] == BSS_TYPE_BCN) { /* WIFI_BEACON */
		_rtw_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
		set_frame_sub_type(pbuf, WIFI_BEACON);
	} else {
		_rtw_memcpy(pwlanhdr->addr1, adapter_mac_addr(padapter), ETH_ALEN);
		set_frame_sub_type(pbuf, WIFI_PROBERSP);
	}

	_rtw_memcpy(pwlanhdr->addr2, pnetwork->network.MacAddress, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, pnetwork->network.MacAddress, ETH_ALEN);


	/* pbuf += sizeof(struct rtw_ieee80211_hdr_3addr); */
	len = sizeof(struct rtw_ieee80211_hdr_3addr);
	_rtw_memcpy((pbuf + len), pnetwork->network.IEs, pnetwork->network.IELength);
	*((u64 *)(pbuf + len)) = cpu_to_le64(notify_timestamp);

	len += pnetwork->network.IELength;

	#if defined(CONFIG_P2P) && 0
	if(rtw_get_p2p_ie(pnetwork->network.IEs+12, pnetwork->network.IELength-12, NULL, NULL))
		RTW_INFO("%s, got p2p_ie\n", __func__);
	#endif

#if 1
	bss = cfg80211_inform_bss_frame(wiphy, notify_channel, (struct ieee80211_mgmt *)pbuf,
					len, notify_signal, GFP_ATOMIC);
#else

	bss = cfg80211_inform_bss(wiphy, notify_channel, (const u8 *)pnetwork->network.MacAddress,
		notify_timestamp, notify_capability, notify_interval, notify_ie,
		notify_ielen, notify_signal, GFP_ATOMIC/*GFP_KERNEL*/);
#endif

	if (unlikely(!bss)) {
		RTW_INFO(FUNC_ADPT_FMT" bss NULL\n", FUNC_ADPT_ARG(padapter));
		goto exit;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 38))
#ifndef COMPAT_KERNEL_RELEASE
	/* patch for cfg80211, update beacon ies to information_elements */
	if (pnetwork->network.Reserved[0] == BSS_TYPE_BCN) { /* WIFI_BEACON */

		if (bss->len_information_elements != bss->len_beacon_ies) {
			bss->information_elements = bss->beacon_ies;
			bss->len_information_elements =  bss->len_beacon_ies;
		}
	}
#endif /* COMPAT_KERNEL_RELEASE */
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 38) */

#if 0
	{
		if (bss->information_elements == bss->proberesp_ies) {
			if (bss->len_information_elements !=  bss->len_proberesp_ies)
				RTW_INFO("error!, len_information_elements != bss->len_proberesp_ies\n");
		} else if (bss->len_information_elements <  bss->len_beacon_ies) {
			bss->information_elements = bss->beacon_ies;
			bss->len_information_elements =  bss->len_beacon_ies;
		}
	}
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
	cfg80211_put_bss(wiphy, bss);
#else
	cfg80211_put_bss(bss);
#endif

exit:
	if (pbuf)
		rtw_mfree(pbuf, buf_size);
	return bss;

}

/*
	Check the given bss is valid by kernel API cfg80211_get_bss()
	@padapter : the given adapter

	return _TRUE if bss is valid,  _FALSE for not found.
*/
int rtw_cfg80211_check_bss(_adapter *padapter)
{
	WLAN_BSSID_EX  *pnetwork = &(padapter->mlmeextpriv.mlmext_info.dev_network);
	struct cfg80211_bss *bss = NULL;
	struct ieee80211_channel *notify_channel = NULL;
	u32 freq;

	if (!(pnetwork) || !(padapter->rtw_wdev))
		return _FALSE;

	freq = rtw_bch2freq(pnetwork->Configuration.Band, pnetwork->Configuration.DSConfig);
	notify_channel = ieee80211_get_channel(padapter->rtw_wdev->wiphy, freq);
	bss = cfg80211_get_bss(padapter->rtw_wdev->wiphy, notify_channel,
			pnetwork->MacAddress, pnetwork->Ssid.Ssid,
			pnetwork->Ssid.SsidLength,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
			pnetwork->InfrastructureMode == Ndis802_11Infrastructure?IEEE80211_BSS_TYPE_ESS:IEEE80211_BSS_TYPE_IBSS,
			IEEE80211_PRIVACY(pnetwork->Privacy));
#else
			pnetwork->InfrastructureMode == Ndis802_11Infrastructure?WLAN_CAPABILITY_ESS:WLAN_CAPABILITY_IBSS, pnetwork->InfrastructureMode == Ndis802_11Infrastructure?WLAN_CAPABILITY_ESS:WLAN_CAPABILITY_IBSS);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
	cfg80211_put_bss(padapter->rtw_wdev->wiphy, bss);
#else
	cfg80211_put_bss(bss);
#endif

	return bss != NULL;
}

void rtw_cfg80211_ibss_indicate_connect(_adapter *padapter)
{
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct wlan_network  *cur_network = &(pmlmepriv->dev_cur_network);
	struct wireless_dev *pwdev = padapter->rtw_wdev;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0))
	struct wiphy *wiphy = pwdev->wiphy;
	int freq = 2412;
	struct ieee80211_channel *notify_channel;
#endif

	RTW_INFO(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(padapter));

	if (pwdev->iftype != NL80211_IFTYPE_ADHOC)
		return;

	if (!rtw_cfg80211_check_bss(padapter)) {
		WLAN_BSSID_EX  *pnetwork = &(padapter->mlmeextpriv.mlmext_info.dev_network);
		struct wlan_network *scanned = pmlmepriv->cur_network_scanned;

		if (check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) {

			_rtw_memcpy(&cur_network->network, pnetwork, sizeof(WLAN_BSSID_EX));
			if (cur_network) {
				if (!rtw_cfg80211_inform_bss(padapter, cur_network))
					RTW_INFO(FUNC_ADPT_FMT" inform fail !!\n", FUNC_ADPT_ARG(padapter));
				else
					RTW_INFO(FUNC_ADPT_FMT" inform success !!\n", FUNC_ADPT_ARG(padapter));
			} else {
				RTW_INFO("cur_network is not exist!!!\n");
				return ;
			}
		} else {
			if (scanned == NULL)
				rtw_warn_on(1);

			if (_rtw_memcmp(&(scanned->network.Ssid), &(pnetwork->Ssid), sizeof(NDIS_802_11_SSID)) == _TRUE
				&& _rtw_memcmp(scanned->network.MacAddress, pnetwork->MacAddress, sizeof(NDIS_802_11_MAC_ADDRESS)) == _TRUE
			) {
				if (!rtw_cfg80211_inform_bss(padapter, scanned)){
					RTW_INFO(FUNC_ADPT_FMT" inform fail !!\n", FUNC_ADPT_ARG(padapter));
				} else {
					/* RTW_INFO(FUNC_ADPT_FMT" inform success !!\n", FUNC_ADPT_ARG(padapter)); */
				}
			} else {
				RTW_INFO("scanned & pnetwork compare fail\n");
				rtw_warn_on(1);
			}
		}

		if (!rtw_cfg80211_check_bss(padapter))
			RTW_PRINT(FUNC_ADPT_FMT" BSS not found !!\n", FUNC_ADPT_ARG(padapter));
	}
	/* notify cfg80211 that device joined an IBSS */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0))
	freq = rtw_bch2freq(cur_network->network.Configuration.Band, cur_network->network.Configuration.DSConfig);
	if (1)
		RTW_INFO("band: %d, chan: %d, freq: %d\n", cur_network->network.Configuration.Band,
				cur_network->network.Configuration.DSConfig, freq);
	notify_channel = ieee80211_get_channel(wiphy, freq);
	cfg80211_ibss_joined(padapter->pnetdev, cur_network->network.MacAddress, notify_channel, GFP_ATOMIC);
#else
	cfg80211_ibss_joined(padapter->pnetdev, cur_network->network.MacAddress, GFP_ATOMIC);
#endif
}

void rtw_cfg80211_indicate_connect(_adapter *padapter)
{
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct wlan_network  *cur_network = &(pmlmepriv->dev_cur_network);
	struct wireless_dev *pwdev = padapter->rtw_wdev;
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(padapter);
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo = &(padapter->wdinfo);
#endif
#if defined(CPTCFG_VERSION) || LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
	struct cfg80211_roam_info roam_info ={};
#endif

	RTW_INFO(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(padapter));
	if (pwdev->iftype != NL80211_IFTYPE_STATION
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
		&& pwdev->iftype != NL80211_IFTYPE_P2P_CLIENT
		#endif
	)
		return;

	if (!MLME_IS_STA(padapter))
		return;

	if (check_fwstate(pmlmepriv, WIFI_MONITOR_STATE) != _TRUE) {
		WLAN_BSSID_EX  *pnetwork = &(padapter->mlmeextpriv.mlmext_info.dev_network);
		struct wlan_network *scanned = pmlmepriv->cur_network_scanned;

		/* RTW_INFO(FUNC_ADPT_FMT" BSS not found\n", FUNC_ADPT_ARG(padapter)); */

		if (scanned == NULL) {
			rtw_warn_on(1);
			goto check_bss;
		}

		if (_rtw_memcmp(scanned->network.MacAddress, pnetwork->MacAddress, sizeof(NDIS_802_11_MAC_ADDRESS)) == _TRUE
			&& _rtw_memcmp(&(scanned->network.Ssid), &(pnetwork->Ssid), sizeof(NDIS_802_11_SSID)) == _TRUE
		) {
			if (!rtw_cfg80211_inform_bss(padapter, scanned))
				RTW_INFO(FUNC_ADPT_FMT" inform fail !!\n", FUNC_ADPT_ARG(padapter));
			else {
				/* RTW_INFO(FUNC_ADPT_FMT" inform success !!\n", FUNC_ADPT_ARG(padapter)); */
			}
		} else {
			RTW_INFO("scanned: %s("MAC_FMT"), cur: %s("MAC_FMT")\n",
				scanned->network.Ssid.Ssid, MAC_ARG(scanned->network.MacAddress),
				pnetwork->Ssid.Ssid, MAC_ARG(pnetwork->MacAddress)
			);
			rtw_warn_on(1);
		}
	}

check_bss:
	if (!rtw_cfg80211_check_bss(padapter))
		RTW_PRINT(FUNC_ADPT_FMT" BSS not found !!\n", FUNC_ADPT_ARG(padapter));

	_rtw_spinlock_bh(&pwdev_priv->connect_req_lock);

	if (rtw_to_roam(padapter) > 0) {
		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 39) || defined(COMPAT_KERNEL_RELEASE)
		struct wiphy *wiphy = pwdev->wiphy;
		struct ieee80211_channel *notify_channel;
		u32 freq;

		freq = rtw_bch2freq(cur_network->network.Configuration.Band,
				cur_network->network.Configuration.DSConfig);
		notify_channel = ieee80211_get_channel(wiphy, freq);
		#endif

		#if defined(CPTCFG_VERSION) || LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
		#if defined(CONFIG_MLD_KERNEL_PATCH) || LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
		/* ToDo CONFIG_RTW_MLD */
		roam_info.links[0].bssid = cur_network->network.MacAddress;
		#else
		roam_info.bssid = cur_network->network.MacAddress;
		#endif
		roam_info.req_ie = pmlmepriv->assoc_req + sizeof(struct rtw_ieee80211_hdr_3addr) + 2;
		roam_info.req_ie_len = pmlmepriv->assoc_req_len - sizeof(struct rtw_ieee80211_hdr_3addr) - 2;
		roam_info.resp_ie = pmlmepriv->assoc_rsp + sizeof(struct rtw_ieee80211_hdr_3addr) + 6;
		roam_info.resp_ie_len = pmlmepriv->assoc_rsp_len - sizeof(struct rtw_ieee80211_hdr_3addr) - 6;

		cfg80211_roamed(padapter->pnetdev, &roam_info, GFP_ATOMIC);
		#else
		cfg80211_roamed(padapter->pnetdev
			#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 39) || defined(COMPAT_KERNEL_RELEASE)
			, notify_channel
			#endif
			, cur_network->network.MacAddress
			, pmlmepriv->assoc_req + sizeof(struct rtw_ieee80211_hdr_3addr) + 2
			, pmlmepriv->assoc_req_len - sizeof(struct rtw_ieee80211_hdr_3addr) - 2
			, pmlmepriv->assoc_rsp + sizeof(struct rtw_ieee80211_hdr_3addr) + 6
			, pmlmepriv->assoc_rsp_len - sizeof(struct rtw_ieee80211_hdr_3addr) - 6
			, GFP_ATOMIC);
		#endif /*LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)*/

		RTW_INFO(FUNC_ADPT_FMT" call cfg80211_roamed\n", FUNC_ADPT_ARG(padapter));

#ifdef CONFIG_RTW_80211R
		if (rtw_ft_roam(padapter))
			rtw_ft_set_status(padapter, RTW_FT_ASSOCIATED_STA);
#endif
	} else {
		#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0) || defined(COMPAT_KERNEL_RELEASE)
		RTW_INFO("pwdev->sme_state(b)=%d\n", pwdev->sme_state);
		#endif

		if (check_fwstate(pmlmepriv, WIFI_MONITOR_STATE) != _TRUE)
			rtw_cfg80211_connect_result(pwdev, cur_network->network.MacAddress
				, pmlmepriv->assoc_req + sizeof(struct rtw_ieee80211_hdr_3addr) + 2
				, pmlmepriv->assoc_req_len - sizeof(struct rtw_ieee80211_hdr_3addr) - 2
				, pmlmepriv->assoc_rsp + sizeof(struct rtw_ieee80211_hdr_3addr) + 6
				, pmlmepriv->assoc_rsp_len - sizeof(struct rtw_ieee80211_hdr_3addr) - 6
				, WLAN_STATUS_SUCCESS, GFP_ATOMIC);
		#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0) || defined(COMPAT_KERNEL_RELEASE)
		RTW_INFO("pwdev->sme_state(a)=%d\n", pwdev->sme_state);
		#endif
	}

	rtw_wdev_free_connect_req(pwdev_priv);

	_rtw_spinunlock_bh(&pwdev_priv->connect_req_lock);
}

void rtw_cfg80211_indicate_disconnect(_adapter *padapter, u16 reason, u8 locally_generated)
{
	struct wireless_dev *pwdev = padapter->rtw_wdev;
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(padapter);
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo = &(padapter->wdinfo);
#endif

	RTW_INFO(FUNC_ADPT_FMT" ,reason = %d\n", FUNC_ADPT_ARG(padapter), reason);

	/*always replace privated definitions with wifi reserved value 0*/
	if (WLAN_REASON_IS_PRIVATE(reason))
		reason = 0;

	if (pwdev->iftype != NL80211_IFTYPE_STATION
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
		&& pwdev->iftype != NL80211_IFTYPE_P2P_CLIENT
		#endif
	)
		return;

	if (!MLME_IS_STA(padapter))
		return;

	_rtw_spinlock_bh(&pwdev_priv->connect_req_lock);

	if (padapter->ndev_unregistering || !rtw_wdev_not_indic_disco(pwdev_priv)) {
		#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0) || defined(COMPAT_KERNEL_RELEASE)
		RTW_INFO("pwdev->sme_state(b)=%d\n", pwdev->sme_state);

		if (pwdev->sme_state == CFG80211_SME_CONNECTING) {
			RTW_INFO(FUNC_ADPT_FMT" call cfg80211_connect_result, reason:%d\n", FUNC_ADPT_ARG(padapter), reason);
			rtw_cfg80211_connect_result(pwdev, NULL, NULL, 0, NULL, 0,
				reason?reason:WLAN_STATUS_UNSPECIFIED_FAILURE,
				GFP_ATOMIC);
		} else if (pwdev->sme_state == CFG80211_SME_CONNECTED) {
			RTW_INFO(FUNC_ADPT_FMT" call cfg80211_disconnected, reason:%d\n", FUNC_ADPT_ARG(padapter), reason);
			rtw_cfg80211_disconnected(pwdev, reason, NULL, 0, locally_generated, GFP_ATOMIC);
		}

		RTW_INFO("pwdev->sme_state(a)=%d\n", pwdev->sme_state);
		#else
		if (pwdev_priv->connect_req) {
			RTW_INFO(FUNC_ADPT_FMT" call cfg80211_connect_result, reason:%d\n", FUNC_ADPT_ARG(padapter), reason);
			rtw_cfg80211_connect_result(pwdev, NULL, NULL, 0, NULL, 0,
				reason?reason:WLAN_STATUS_UNSPECIFIED_FAILURE,
				GFP_ATOMIC);
		} else {
			RTW_INFO(FUNC_ADPT_FMT" call cfg80211_disconnected, reason:%d\n", FUNC_ADPT_ARG(padapter), reason);
			rtw_cfg80211_disconnected(pwdev, reason, NULL, 0, locally_generated, GFP_ATOMIC);
		}
		#endif
	}

	rtw_wdev_free_connect_req(pwdev_priv);

	_rtw_spinunlock_bh(&pwdev_priv->connect_req_lock);
}


#ifdef CONFIG_AP_MODE
static int rtw_cfg80211_ap_set_encryption(struct net_device *dev, struct ieee_param *param)
{
	int ret = 0;
	u32 wep_key_idx, wep_key_len;
	struct sta_info *psta = NULL, *pbcmc_sta = NULL;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct security_priv *psecuritypriv = &(padapter->securitypriv);
	struct sta_priv *pstapriv = &padapter->stapriv;
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	struct link_security_priv *lsecuritypriv = &padapter_link->securitypriv;

	RTW_INFO("%s\n", __FUNCTION__);

	param->u.crypt.err = 0;
	param->u.crypt.alg[IEEE_CRYPT_ALG_NAME_LEN - 1] = '\0';

	if (is_broadcast_mac_addr(param->sta_addr)) {
		if (param->u.crypt.idx >= WEP_KEYS
			#ifdef CONFIG_IEEE80211W
			&& param->u.crypt.idx > BIP_MAX_KEYID
			#endif
		) {
			ret = -EINVAL;
			goto exit;
		}
	} else {
		psta = rtw_get_stainfo(pstapriv, param->sta_addr);
		if (!psta) {
			ret = -EINVAL;
			RTW_INFO(FUNC_ADPT_FMT", sta "MAC_FMT" not found\n"
				, FUNC_ADPT_ARG(padapter), MAC_ARG(param->sta_addr));
			goto exit;
		}
	#ifdef CONFIG_RTW_80211R_AP
		if ((psta->authalg == WLAN_AUTH_FT) &&
			!(psta->state & WIFI_FW_ASSOC_SUCCESS)) {
			ret = -EINVAL;
			RTW_INFO(FUNC_ADPT_FMT", sta "MAC_FMT
				" not ready to setkey before assoc success!\n"
				, FUNC_ADPT_ARG(padapter), MAC_ARG(param->sta_addr));
			goto exit;
		}
	#endif
	}

	if (strcmp(param->u.crypt.alg, "none") == 0 && (psta == NULL)) {
		/* todo:clear default encryption keys */

		RTW_INFO("clear default encryption keys, keyid=%d\n", param->u.crypt.idx);

		goto exit;
	}


	if (strcmp(param->u.crypt.alg, "WEP") == 0 && (psta == NULL)) {
		RTW_INFO("r871x_set_encryption, crypt.alg = WEP\n");

		wep_key_idx = param->u.crypt.idx;
		wep_key_len = param->u.crypt.key_len;

		RTW_INFO("r871x_set_encryption, wep_key_idx=%d, len=%d\n", wep_key_idx, wep_key_len);

		if ((wep_key_idx >= WEP_KEYS) || (wep_key_len <= 0)) {
			ret = -EINVAL;
			goto exit;
		}

		if (wep_key_len > 0)
			wep_key_len = wep_key_len <= WLAN_KEY_LEN_WEP40 ? WLAN_KEY_LEN_WEP40 : WLAN_KEY_LEN_WEP104;

		if (psecuritypriv->bWepDefaultKeyIdxSet == 0) {
			/* wep default key has not been set, so use this key index as default key. */

			psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Auto;
			psecuritypriv->ndisencryptstatus = Ndis802_11Encryption1Enabled;
			psecuritypriv->dot11PrivacyAlgrthm = _WEP40_;
			psecuritypriv->dot118021XGrpPrivacy = _WEP40_;

			if (wep_key_len == WLAN_KEY_LEN_WEP104) {
				psecuritypriv->dot11PrivacyAlgrthm = _WEP104_;
				psecuritypriv->dot118021XGrpPrivacy = _WEP104_;
			}

			psecuritypriv->dot11PrivacyKeyIndex = wep_key_idx;
		}

		_rtw_memcpy(&(psecuritypriv->dot11DefKey[wep_key_idx].skey[0]), param->u.crypt.key, wep_key_len);

		psecuritypriv->dot11DefKeylen[wep_key_idx] = wep_key_len;

		rtw_ap_set_wep_key(padapter, padapter_link,
				param->u.crypt.key, wep_key_len, wep_key_idx, 1);

		goto exit;

	}

	if (!psta) { /* group key */
		if (param->u.crypt.set_tx == 0) { /* group key, TX only */
			if (strcmp(param->u.crypt.alg, "WEP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set WEP TX GTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				psecuritypriv->dot118021XGrpPrivacy = _WEP40_;
				_rtw_memcpy(lsecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key, (param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
				if (param->u.crypt.key_len == 13)
					psecuritypriv->dot118021XGrpPrivacy = _WEP104_;

			} else if (strcmp(param->u.crypt.alg, "TKIP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set TKIP TX GTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				psecuritypriv->dot118021XGrpPrivacy = _TKIP_;
				_rtw_memcpy(lsecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key, (param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
				/* set mic key */
				_rtw_memcpy(lsecuritypriv->dot118021XGrptxmickey[param->u.crypt.idx].skey, &(param->u.crypt.key[16]), 8);
				_rtw_memcpy(lsecuritypriv->dot118021XGrprxmickey[param->u.crypt.idx].skey, &(param->u.crypt.key[24]), 8);
				psecuritypriv->busetkipkey = _TRUE;

			} else if (strcmp(param->u.crypt.alg, "CCMP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set CCMP TX GTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				psecuritypriv->dot118021XGrpPrivacy = _AES_;
				_rtw_memcpy(lsecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key, (param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));

			} else if (strcmp(param->u.crypt.alg, "GCMP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set GCMP TX GTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				psecuritypriv->dot118021XGrpPrivacy = _GCMP_;
				_rtw_memcpy(lsecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,
					param->u.crypt.key,
					(param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));

			} else if (strcmp(param->u.crypt.alg, "GCMP_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set GCMP_256 TX GTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				psecuritypriv->dot118021XGrpPrivacy = _GCMP_256_;
				_rtw_memcpy(lsecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,
					param->u.crypt.key,
					(param->u.crypt.key_len > 32 ? 32 : param->u.crypt.key_len));

			} else if (strcmp(param->u.crypt.alg, "CCMP_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set CCMP_256 TX GTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				psecuritypriv->dot118021XGrpPrivacy = _CCMP_256_;
				_rtw_memcpy(lsecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,
					param->u.crypt.key,
					(param->u.crypt.key_len > 32 ? 32: param->u.crypt.key_len));

			#ifdef CONFIG_IEEE80211W
			} else if (strcmp(param->u.crypt.alg, "BIP") == 0) {
				psecuritypriv->dot11wCipher = _BIP_CMAC_128_;
				RTW_INFO(FUNC_ADPT_FMT" set TX CMAC-128 IGTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				_rtw_memcpy(lsecuritypriv->dot11wBIPKey[param->u.crypt.idx].skey, param->u.crypt.key, (param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
				lsecuritypriv->dot11wBIPKeyid = param->u.crypt.idx;
				lsecuritypriv->dot11wBIPtxpn.val = RTW_GET_LE64(param->u.crypt.seq);
				lsecuritypriv->binstallBIPkey = _TRUE;
				rtw_ap_set_group_key(padapter, padapter_link, param->u.crypt.key, psecuritypriv->dot11wCipher, param->u.crypt.idx);
				goto exit;
			} else if (strcmp(param->u.crypt.alg, "BIP_GMAC_128") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set TX GMAC-128 IGTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				psecuritypriv->dot11wCipher = _BIP_GMAC_128_;
				_rtw_memcpy(lsecuritypriv->dot11wBIPKey[param->u.crypt.idx].skey,
					param->u.crypt.key, param->u.crypt.key_len);
				lsecuritypriv->dot11wBIPKeyid = param->u.crypt.idx;
				lsecuritypriv->dot11wBIPtxpn.val = RTW_GET_LE64(param->u.crypt.seq);
				lsecuritypriv->binstallBIPkey = _TRUE;
				goto exit;
			} else if (strcmp(param->u.crypt.alg, "BIP_GMAC_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set TX GMAC-256 IGTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				psecuritypriv->dot11wCipher = _BIP_GMAC_256_;
				_rtw_memcpy(lsecuritypriv->dot11wBIPKey[param->u.crypt.idx].skey,
					param->u.crypt.key, param->u.crypt.key_len);
				lsecuritypriv->dot11wBIPKeyid = param->u.crypt.idx;
				lsecuritypriv->dot11wBIPtxpn.val = RTW_GET_LE64(param->u.crypt.seq);
				lsecuritypriv->binstallBIPkey = _TRUE;
				goto exit;
			} else if (strcmp(param->u.crypt.alg, "BIP_CMAC_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set TX CMAC-256 IGTK idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
				psecuritypriv->dot11wCipher = _BIP_CMAC_256_;
				_rtw_memcpy(lsecuritypriv->dot11wBIPKey[param->u.crypt.idx].skey,
					param->u.crypt.key, param->u.crypt.key_len);
				lsecuritypriv->dot11wBIPKeyid = param->u.crypt.idx;
				lsecuritypriv->dot11wBIPtxpn.val = RTW_GET_LE64(param->u.crypt.seq);
				lsecuritypriv->binstallBIPkey = _TRUE;
				goto exit;
			#endif /* CONFIG_IEEE80211W */

			} else if (strcmp(param->u.crypt.alg, "none") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" clear group key, idx:%u\n"
					, FUNC_ADPT_ARG(padapter), param->u.crypt.idx);
				psecuritypriv->dot118021XGrpPrivacy = _NO_PRIVACY_;
			} else {
				RTW_WARN(FUNC_ADPT_FMT" set group key, not support\n"
					, FUNC_ADPT_ARG(padapter));
				goto exit;
			}

			lsecuritypriv->dot118021XGrpKeyid = param->u.crypt.idx;
			pbcmc_sta = rtw_get_bcmc_stainfo(padapter, padapter_link);
			if (pbcmc_sta) {
				pbcmc_sta->dot11txpn.val = RTW_GET_LE64(param->u.crypt.seq);
				pbcmc_sta->ieee8021x_blocked = _FALSE;
				pbcmc_sta->dot118021XPrivacy = psecuritypriv->dot118021XGrpPrivacy; /* rx will use bmc_sta's dot118021XPrivacy			 */
			}
			lsecuritypriv->binstallGrpkey = _TRUE;
			psecuritypriv->dot11PrivacyAlgrthm = psecuritypriv->dot118021XGrpPrivacy;/* !!! */

			rtw_ap_set_group_key(padapter, padapter_link, param->u.crypt.key, psecuritypriv->dot118021XGrpPrivacy, param->u.crypt.idx);
		}

		goto exit;

	}

	if (psecuritypriv->dot11AuthAlgrthm == dot11AuthAlgrthm_8021X && psta) { /* psk/802_1x */
		if (param->u.crypt.set_tx == 1) {
			u8 iv[IV_LENGTH];

			/* pairwise key */
			if (param->u.crypt.key_len == 32)
				_rtw_memcpy(psta->dot118021x_UncstKey.skey,
						param->u.crypt.key,
						(param->u.crypt.key_len > 32 ? 32 : param->u.crypt.key_len));
			else
				_rtw_memcpy(psta->dot118021x_UncstKey.skey,
						param->u.crypt.key,
						(param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));

			if (strcmp(param->u.crypt.alg, "WEP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set WEP PTK of "MAC_FMT" idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot118021XPrivacy = _WEP40_;
				if (param->u.crypt.key_len == 13)
					psta->dot118021XPrivacy = _WEP104_;

			} else if (strcmp(param->u.crypt.alg, "TKIP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set TKIP PTK of "MAC_FMT" idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot118021XPrivacy = _TKIP_;
				/* set mic key */
				_rtw_memcpy(psta->dot11tkiptxmickey.skey, &(param->u.crypt.key[16]), 8);
				_rtw_memcpy(psta->dot11tkiprxmickey.skey, &(param->u.crypt.key[24]), 8);
				psecuritypriv->busetkipkey = _TRUE;

			} else if (strcmp(param->u.crypt.alg, "CCMP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set CCMP PTK of "MAC_FMT" idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot118021XPrivacy = _AES_;

			} else if (strcmp(param->u.crypt.alg, "GCMP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set GCMP PTK of "MAC_FMT" idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot118021XPrivacy = _GCMP_;

			} else if (strcmp(param->u.crypt.alg, "GCMP_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set GCMP_256 PTK of "MAC_FMT" idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot118021XPrivacy = _GCMP_256_;

			} else if (strcmp(param->u.crypt.alg, "CCMP_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set CCMP_256 PTK of "MAC_FMT" idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot118021XPrivacy = _CCMP_256_;

			} else if (strcmp(param->u.crypt.alg, "none") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" clear pairwise key of "MAC_FMT" idx:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx);
				psta->dot118021XPrivacy = _NO_PRIVACY_;
			} else {
				RTW_WARN(FUNC_ADPT_FMT" set pairwise key of "MAC_FMT", not support\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr));
				goto exit;
			}

			psta->dot11txpn.val = RTW_GET_LE64(param->u.crypt.seq);
			psta->dot11rxpn.val = RTW_GET_LE64(param->u.crypt.seq);
			if (rtw_pn_to_iv(param->u.crypt.seq,
			    iv, param->u.crypt.idx,
			    padapter->securitypriv.dot11PrivacyAlgrthm)) {
				struct stainfo_rxcache *prxcache = &psta->sta_recvpriv.rxcache;
				int i;

				for (i = 0; i < RTW_MAX_TID_NUM; i++)
					_rtw_memcpy(prxcache->iv[i], iv, IV_LENGTH);
			}
			psta->ieee8021x_blocked = _FALSE;

			if (psta->dot118021XPrivacy != _NO_PRIVACY_) {
				psta->bpairwise_key_installed = _TRUE;

				/* WPA2 key-handshake has completed */
				if (psecuritypriv->ndisauthtype == Ndis802_11AuthModeWPA2PSK)
					psta->state &= (~WIFI_UNDER_KEY_HANDSHAKE);
			}

			rtw_ap_set_pairwise_key(padapter, psta);
		} else {
			/* peer's group key, RX only */
			#ifdef CONFIG_RTW_MESH
			if (strcmp(param->u.crypt.alg, "CCMP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set CCMP GTK of "MAC_FMT", idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->group_privacy = _AES_;
				_rtw_memcpy(psta->gtk.skey, param->u.crypt.key, (param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
				psta->gtk_bmp |= BIT(param->u.crypt.idx);
				psta->gtk_pn.val = RTW_GET_LE64(param->u.crypt.seq);

			} else if (strcmp(param->u.crypt.alg, "GCMP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set GCMP GTK of "MAC_FMT", idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->group_privacy = _GCMP_;
				_rtw_memcpy(psta->gtk.skey, param->u.crypt.key, (param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
				psta->gtk_bmp |= BIT(param->u.crypt.idx);
				psta->gtk_pn.val = RTW_GET_LE64(param->u.crypt.seq);

			} else if (strcmp(param->u.crypt.alg, "CCMP_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set CCMP_256 GTK of "MAC_FMT", idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->group_privacy = _CCMP_256_;
				_rtw_memcpy(psta->gtk.skey, param->u.crypt.key, (param->u.crypt.key_len > 32 ? 32 : param->u.crypt.key_len));
				psta->gtk_bmp |= BIT(param->u.crypt.idx);
				psta->gtk_pn.val = RTW_GET_LE64(param->u.crypt.seq);

			} else if (strcmp(param->u.crypt.alg, "GCMP_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set GCMP_256 GTK of "MAC_FMT", idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->group_privacy = _GCMP_256_;
				_rtw_memcpy(psta->gtk.skey, param->u.crypt.key, (param->u.crypt.key_len > 32 ? 32 : param->u.crypt.key_len));
				psta->gtk_bmp |= BIT(param->u.crypt.idx);
				psta->gtk_pn.val = RTW_GET_LE64(param->u.crypt.seq);

			#ifdef CONFIG_IEEE80211W
			} else if (strcmp(param->u.crypt.alg, "BIP") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set CMAC-128 IGTK of "MAC_FMT", idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot11wCipher = _BIP_CMAC_128_;
				_rtw_memcpy(psta->igtk.skey, param->u.crypt.key, (param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
				psta->igtk_bmp |= BIT(param->u.crypt.idx);
				psta->igtk_id = param->u.crypt.idx;
				psta->igtk_pn.val = RTW_GET_LE64(param->u.crypt.seq);
				goto exit;

			} else if (strcmp(param->u.crypt.alg, "BIP_GMAC_128") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set GMAC-128 IGTK of "MAC_FMT", idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot11wCipher = _BIP_GMAC_128_;
				_rtw_memcpy(psta->igtk.skey, param->u.crypt.key, (param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
				psta->igtk_bmp |= BIT(param->u.crypt.idx);
				psta->igtk_id = param->u.crypt.idx;
				psta->igtk_pn.val = RTW_GET_LE64(param->u.crypt.seq);
				goto exit;

			} else if (strcmp(param->u.crypt.alg, "BIP_CMAC_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set CMAC-256 IGTK of "MAC_FMT", idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot11wCipher = _BIP_CMAC_256_;
				_rtw_memcpy(psta->igtk.skey, param->u.crypt.key, (param->u.crypt.key_len > 32 ? 32 : param->u.crypt.key_len));
				psta->igtk_bmp |= BIT(param->u.crypt.idx);
				psta->igtk_id = param->u.crypt.idx;
				psta->igtk_pn.val = RTW_GET_LE64(param->u.crypt.seq);
				goto exit;

			} else if (strcmp(param->u.crypt.alg, "BIP_GMAC_256") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" set GMAC-256 IGTK of "MAC_FMT", idx:%u, len:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx, param->u.crypt.key_len);
				psta->dot11wCipher = _BIP_GMAC_256_;
				_rtw_memcpy(psta->igtk.skey, param->u.crypt.key, (param->u.crypt.key_len > 32 ? 32 : param->u.crypt.key_len));
				psta->igtk_bmp |= BIT(param->u.crypt.idx);
				psta->igtk_id = param->u.crypt.idx;
				psta->igtk_pn.val = RTW_GET_LE64(param->u.crypt.seq);
				goto exit;
			#endif /* CONFIG_IEEE80211W */

			} else if (strcmp(param->u.crypt.alg, "none") == 0) {
				RTW_INFO(FUNC_ADPT_FMT" clear group key of "MAC_FMT", idx:%u\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr)
					, param->u.crypt.idx);
				psta->group_privacy = _NO_PRIVACY_;
				psta->gtk_bmp &= ~BIT(param->u.crypt.idx);
			} else
			#endif /* CONFIG_RTW_MESH */
			{
				RTW_WARN(FUNC_ADPT_FMT" set group key of "MAC_FMT", not support\n"
					, FUNC_ADPT_ARG(padapter), MAC_ARG(psta->phl_sta->mac_addr));
				goto exit;
			}

			#ifdef CONFIG_RTW_MESH
			rtw_ap_set_sta_key(padapter, psta->phl_sta->mac_addr, psta->group_privacy
				, param->u.crypt.key, param->u.crypt.idx, 1);
			#endif
		}

	}

exit:
	return ret;
}
#endif /* CONFIG_AP_MODE */

static int rtw_cfg80211_set_encryption(struct net_device *dev, struct ieee_param *param)
{
	int ret = 0;
	u32 wep_key_idx, wep_key_len;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	struct link_mlme_priv *lmlmepriv = &padapter_link->mlmepriv;
	struct link_security_priv *lsecuritypriv = &padapter_link->securitypriv;
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo = &padapter->wdinfo;
#endif /* CONFIG_P2P */

	RTW_INFO("%s\n", __func__);

	param->u.crypt.err = 0;
	param->u.crypt.alg[IEEE_CRYPT_ALG_NAME_LEN - 1] = '\0';

	if (is_broadcast_mac_addr(param->sta_addr)) {
		if (param->u.crypt.idx >= WEP_KEYS
			#ifdef CONFIG_IEEE80211W
			&& param->u.crypt.idx > BIP_MAX_KEYID
			#endif
		) {
			ret = -EINVAL;
			goto exit;
		}
	} else {
#ifdef CONFIG_WAPI_SUPPORT
		if (strcmp(param->u.crypt.alg, "SMS4"))
#endif
		{
			ret = -EINVAL;
			goto exit;
		}
	}

	if (strcmp(param->u.crypt.alg, "WEP") == 0) {
		RTW_INFO("wpa_set_encryption, crypt.alg = WEP\n");

		wep_key_idx = param->u.crypt.idx;
		wep_key_len = param->u.crypt.key_len;

		if ((wep_key_idx >= WEP_KEYS) || (wep_key_len <= 0)) {
			ret = -EINVAL;
			goto exit;
		}

		if (psecuritypriv->bWepDefaultKeyIdxSet == 0) {
			/* wep default key has not been set, so use this key index as default key. */

			wep_key_len = wep_key_len <= 5 ? 5 : 13;

			psecuritypriv->ndisencryptstatus = Ndis802_11Encryption1Enabled;
			psecuritypriv->dot11PrivacyAlgrthm = _WEP40_;
			psecuritypriv->dot118021XGrpPrivacy = _WEP40_;

			if (wep_key_len == 13) {
				psecuritypriv->dot11PrivacyAlgrthm = _WEP104_;
				psecuritypriv->dot118021XGrpPrivacy = _WEP104_;
			}

			psecuritypriv->dot11PrivacyKeyIndex = wep_key_idx;
		}

		_rtw_memcpy(&(psecuritypriv->dot11DefKey[wep_key_idx].skey[0]), param->u.crypt.key, wep_key_len);

		psecuritypriv->dot11DefKeylen[wep_key_idx] = wep_key_len;

		rtw_set_key(padapter, padapter_link, wep_key_idx, 0, _TRUE);

		goto exit;
	}

	if (padapter->securitypriv.dot11AuthAlgrthm == dot11AuthAlgrthm_8021X) { /* 802_1x */
		struct sta_info *psta, *pbcmc_sta;
		struct sta_priv *pstapriv = &padapter->stapriv;

		/* RTW_INFO("%s, : dot11AuthAlgrthm == dot11AuthAlgrthm_8021X\n", __func__); */

		if (MLME_IS_STA(padapter) || MLME_IS_MP(padapter)) {/* sta mode */
#ifdef CONFIG_RTW_80211R
			if (rtw_ft_roam(padapter))
				psta = rtw_get_stainfo(pstapriv, pmlmepriv->assoc_bssid);
			else
#endif
				psta = rtw_get_stainfo(pstapriv, get_link_bssid(lmlmepriv));
			if (psta == NULL) {
				/* DEBUG_ERR( ("Set wpa_set_encryption: Obtain Sta_info fail\n")); */
				RTW_INFO("%s, : Obtain Sta_info fail\n", __func__);
			} else {
				/* Jeff: don't disable ieee8021x_blocked while clearing key */
				if (strcmp(param->u.crypt.alg, "none") != 0)
					psta->ieee8021x_blocked = _FALSE;

				if ((padapter->securitypriv.ndisencryptstatus == Ndis802_11Encryption2Enabled) ||
				    (padapter->securitypriv.ndisencryptstatus ==  Ndis802_11Encryption3Enabled))
					psta->dot118021XPrivacy = padapter->securitypriv.dot11PrivacyAlgrthm;

				if (param->u.crypt.set_tx == 1) { /* pairwise key */
					u8 iv[IV_LENGTH];

					RTW_INFO(FUNC_ADPT_FMT" set %s PTK idx:%u, len:%u\n"
						, FUNC_ADPT_ARG(padapter), param->u.crypt.alg, param->u.crypt.idx, param->u.crypt.key_len);

					if (strcmp(param->u.crypt.alg, "GCMP_256") == 0
						|| strcmp(param->u.crypt.alg, "CCMP_256") == 0) {
						_rtw_memcpy(psta->dot118021x_UncstKey.skey,
							param->u.crypt.key,
							((param->u.crypt.key_len > 32) ?
								32 : param->u.crypt.key_len));
					} else
						_rtw_memcpy(psta->dot118021x_UncstKey.skey,
							param->u.crypt.key,
							(param->u.crypt.key_len > 16 ?
								16 : param->u.crypt.key_len));

					if (strcmp(param->u.crypt.alg, "TKIP") == 0) { /* set mic key */
						_rtw_memcpy(psta->dot11tkiptxmickey.skey, &(param->u.crypt.key[16]), 8);
						_rtw_memcpy(psta->dot11tkiprxmickey.skey, &(param->u.crypt.key[24]), 8);
						padapter->securitypriv.busetkipkey = _FALSE;
					}
					psta->dot11txpn.val = RTW_GET_LE64(param->u.crypt.seq);
					psta->dot11rxpn.val = RTW_GET_LE64(param->u.crypt.seq);
					if (rtw_pn_to_iv(param->u.crypt.seq,
					    iv, param->u.crypt.idx,
					    padapter->securitypriv.dot11PrivacyAlgrthm)) {
						struct stainfo_rxcache *prxcache = &psta->sta_recvpriv.rxcache;
						int i;

						for (i = 0; i < RTW_MAX_TID_NUM; i++)
							_rtw_memcpy(prxcache->iv[i], iv, IV_LENGTH);
					}
					psta->bpairwise_key_installed = _TRUE;
					#ifdef CONFIG_RTW_80211R
					psta->ft_pairwise_key_installed = _TRUE;
					#endif
					rtw_setstakey_cmd(padapter, psta, UNICAST_KEY, _TRUE);

				} else { /* group key */
					if (strcmp(param->u.crypt.alg, "TKIP") == 0
						|| strcmp(param->u.crypt.alg, "CCMP") == 0
						|| strcmp(param->u.crypt.alg, "GCMP") == 0) {
						RTW_INFO(FUNC_ADPT_FMT" set %s GTK idx:%u, len:%u\n"
							, FUNC_ADPT_ARG(padapter), param->u.crypt.alg, param->u.crypt.idx, param->u.crypt.key_len);
						_rtw_memcpy(lsecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,
							param->u.crypt.key,
							(param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
						_rtw_memcpy(lsecuritypriv->dot118021XGrptxmickey[param->u.crypt.idx].skey, &(param->u.crypt.key[16]), 8);
						_rtw_memcpy(lsecuritypriv->dot118021XGrprxmickey[param->u.crypt.idx].skey, &(param->u.crypt.key[24]), 8);
						lsecuritypriv->binstallGrpkey = _TRUE;
						if (param->u.crypt.idx < 4)
							_rtw_memcpy(lsecuritypriv->iv_seq[param->u.crypt.idx], param->u.crypt.seq, 8);
						lsecuritypriv->dot118021XGrpKeyid = param->u.crypt.idx;
						rtw_set_key(padapter, padapter_link, param->u.crypt.idx, 1, _TRUE);
					} else if (strcmp(param->u.crypt.alg, "GCMP_256") == 0
						|| strcmp(param->u.crypt.alg, "CCMP_256") == 0) {
						RTW_INFO(FUNC_ADPT_FMT" set %s GTK idx:%u, len:%u\n"
							, FUNC_ADPT_ARG(padapter), param->u.crypt.alg, param->u.crypt.idx, param->u.crypt.key_len);
						_rtw_memcpy(
							lsecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,
							param->u.crypt.key,
							(param->u.crypt.key_len > 32 ? 32 : param->u.crypt.key_len));
						lsecuritypriv->binstallGrpkey = _TRUE;
						lsecuritypriv->dot118021XGrpKeyid = param->u.crypt.idx;
						rtw_set_key(padapter, padapter_link, param->u.crypt.idx, 1, _TRUE);
					#ifdef CONFIG_IEEE80211W
					} else if (strcmp(param->u.crypt.alg, "BIP") == 0) {
						psecuritypriv->dot11wCipher = _BIP_CMAC_128_;
						RTW_INFO(FUNC_ADPT_FMT" set CMAC-128 IGTK idx:%u, len:%u\n"
							, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
						_rtw_memcpy(lsecuritypriv->dot11wBIPKey[param->u.crypt.idx].skey,
							param->u.crypt.key,
							(param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
						lsecuritypriv->dot11wBIPKeyid = param->u.crypt.idx;
						lsecuritypriv->dot11wBIPrxpn.val = RTW_GET_LE64(param->u.crypt.seq);
						lsecuritypriv->binstallBIPkey = _TRUE;
						rtw_set_key(padapter, padapter_link, param->u.crypt.idx, 1, _TRUE);
					} else if (strcmp(param->u.crypt.alg, "BIP_GMAC_128") == 0) {
						psecuritypriv->dot11wCipher = _BIP_GMAC_128_;
						RTW_INFO(FUNC_ADPT_FMT" set GMAC-128 IGTK idx:%u, len:%u\n"
							, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
						_rtw_memcpy(lsecuritypriv->dot11wBIPKey[param->u.crypt.idx].skey,
							param->u.crypt.key,
							(param->u.crypt.key_len > 16 ? 16 : param->u.crypt.key_len));
						lsecuritypriv->dot11wBIPKeyid = param->u.crypt.idx;
						lsecuritypriv->dot11wBIPrxpn.val = RTW_GET_LE64(param->u.crypt.seq);
						lsecuritypriv->binstallBIPkey = _TRUE;
					} else if (strcmp(param->u.crypt.alg, "BIP_GMAC_256") == 0) {
						psecuritypriv->dot11wCipher = _BIP_GMAC_256_;
						RTW_INFO(FUNC_ADPT_FMT" set GMAC-256 IGTK idx:%u, len:%u\n"
							, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
						_rtw_memcpy(lsecuritypriv->dot11wBIPKey[param->u.crypt.idx].skey,
							param->u.crypt.key,
							(param->u.crypt.key_len > 32 ? 32 : param->u.crypt.key_len));
						lsecuritypriv->dot11wBIPKeyid = param->u.crypt.idx;
						lsecuritypriv->dot11wBIPrxpn.val = RTW_GET_LE64(param->u.crypt.seq);
						lsecuritypriv->binstallBIPkey = _TRUE;
					} else if (strcmp(param->u.crypt.alg, "BIP_CMAC_256") == 0) {
						psecuritypriv->dot11wCipher = _BIP_CMAC_256_;
						RTW_INFO(FUNC_ADPT_FMT" set CMAC-256 IGTK idx:%u, len:%u\n"
							, FUNC_ADPT_ARG(padapter), param->u.crypt.idx, param->u.crypt.key_len);
						_rtw_memcpy(lsecuritypriv->dot11wBIPKey[param->u.crypt.idx].skey,
							param->u.crypt.key, param->u.crypt.key_len);
						lsecuritypriv->dot11wBIPKeyid = param->u.crypt.idx;
						lsecuritypriv->dot11wBIPrxpn.val = RTW_GET_LE64(param->u.crypt.seq);
						lsecuritypriv->binstallBIPkey = _TRUE;
					#endif /* CONFIG_IEEE80211W */

					}

					/* WPA/WPA2 key-handshake has completed */
					clr_fwstate(pmlmepriv, WIFI_UNDER_KEY_HANDSHAKE);

				}
			}

			pbcmc_sta = rtw_get_bcmc_stainfo(padapter, padapter_link);
			if (pbcmc_sta == NULL) {
				/* DEBUG_ERR( ("Set OID_802_11_ADD_KEY: bcmc stainfo is null\n")); */
			} else {
				/* Jeff: don't disable ieee8021x_blocked while clearing key */
				if (strcmp(param->u.crypt.alg, "none") != 0)
					pbcmc_sta->ieee8021x_blocked = _FALSE;

				if ((padapter->securitypriv.ndisencryptstatus == Ndis802_11Encryption2Enabled) ||
				    (padapter->securitypriv.ndisencryptstatus ==  Ndis802_11Encryption3Enabled))
					pbcmc_sta->dot118021XPrivacy = padapter->securitypriv.dot11PrivacyAlgrthm;
			}
		} else if (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)) { /* adhoc mode */
		}
	}

	#ifdef CONFIG_WAPI_SUPPORT
	if (strcmp(param->u.crypt.alg, "SMS4") == 0)
		rtw_wapi_set_set_encryption(padapter, param);
	#endif

exit:

	RTW_INFO("%s, ret=%d\n", __func__, ret);


	return ret;
}

static int cfg80211_rtw_add_key(struct wiphy *wiphy, struct net_device *ndev
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	, int link_id
#endif
	, u8 key_index
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	, bool pairwise
#endif
	, const u8 *mac_addr, struct key_params *params)
{
	char *alg_name;
	u32 param_len;
	struct ieee_param *param = NULL;
	int ret = 0;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct wireless_dev *rtw_wdev = padapter->rtw_wdev;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
#ifdef CONFIG_TDLS
	struct sta_info *ptdls_sta;
#endif /* CONFIG_TDLS */

	if (mac_addr)
		RTW_INFO(FUNC_NDEV_FMT" adding key for %pM\n", FUNC_NDEV_ARG(ndev), mac_addr);
	RTW_INFO(FUNC_NDEV_FMT" cipher=0x%x\n", FUNC_NDEV_ARG(ndev), params->cipher);
	RTW_INFO(FUNC_NDEV_FMT" key_len=%d, key_index=%d\n", FUNC_NDEV_ARG(ndev), params->key_len, key_index);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	RTW_INFO(FUNC_NDEV_FMT" pairwise=%d\n", FUNC_NDEV_ARG(ndev), pairwise);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	RTW_INFO(FUNC_NDEV_FMT" link_id=%d\n", FUNC_NDEV_ARG(ndev), link_id);
#endif

	if (rtw_cfg80211_sync_iftype(padapter) != _SUCCESS) {
		ret = -ENOTSUPP;
		goto addkey_end;
	}

	param_len = sizeof(struct ieee_param) + params->key_len;
	param = rtw_malloc(param_len);
	if (param == NULL)
		return -1;

	_rtw_memset(param, 0, param_len);

	param->cmd = IEEE_CMD_SET_ENCRYPTION;
	_rtw_memset(param->sta_addr, 0xff, ETH_ALEN);

	switch (params->cipher) {
	case IW_AUTH_CIPHER_NONE:
		/* todo: remove key */
		/* remove = 1;	 */
		alg_name = "none";
		break;
	case WLAN_CIPHER_SUITE_WEP40:
	case WLAN_CIPHER_SUITE_WEP104:
		alg_name = "WEP";
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		alg_name = "TKIP";
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		alg_name = "CCMP";
		break;
	case WIFI_CIPHER_SUITE_GCMP:
		alg_name = "GCMP";
		break;
	case WIFI_CIPHER_SUITE_GCMP_256:
		alg_name = "GCMP_256";
		break;
	case WIFI_CIPHER_SUITE_CCMP_256:
		alg_name = "CCMP_256";
		break;
#ifdef CONFIG_IEEE80211W
	case WLAN_CIPHER_SUITE_AES_CMAC:
		alg_name = "BIP";
		break;
	case WIFI_CIPHER_SUITE_BIP_GMAC_128:
		alg_name = "BIP_GMAC_128";
		break;
	case WIFI_CIPHER_SUITE_BIP_GMAC_256:
		alg_name = "BIP_GMAC_256";
		break;
	case WIFI_CIPHER_SUITE_BIP_CMAC_256:
		alg_name = "BIP_CMAC_256";
		break;
#endif /* CONFIG_IEEE80211W */
#ifdef CONFIG_WAPI_SUPPORT
	case WLAN_CIPHER_SUITE_SMS4:
		alg_name = "SMS4";
		if (pairwise == NL80211_KEYTYPE_PAIRWISE) {
			if (key_index != 0 && key_index != 1) {
				ret = -ENOTSUPP;
				goto addkey_end;
			}
			_rtw_memcpy((void *)param->sta_addr, (void *)mac_addr, ETH_ALEN);
		} else
			RTW_INFO("mac_addr is null\n");
		RTW_INFO("rtw_wx_set_enc_ext: SMS4 case\n");
		break;
#endif

	default:
		ret = -ENOTSUPP;
		goto addkey_end;
	}

	strncpy((char *)param->u.crypt.alg, alg_name, IEEE_CRYPT_ALG_NAME_LEN);


	if (!mac_addr || is_broadcast_ether_addr(mac_addr)
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
		|| !pairwise
		#endif
	) {
		param->u.crypt.set_tx = 0; /* for wpa/wpa2 group key */
	} else {
		param->u.crypt.set_tx = 1; /* for wpa/wpa2 pairwise key */
	}

	param->u.crypt.idx = key_index;

	if (params->seq_len && params->seq) {
		_rtw_memcpy(param->u.crypt.seq, (u8 *)params->seq, params->seq_len);
		RTW_INFO(FUNC_NDEV_FMT" seq_len:%u, seq:0x%llx\n", FUNC_NDEV_ARG(ndev)
			, params->seq_len, RTW_GET_LE64(param->u.crypt.seq));
	}

	if (params->key_len && params->key) {
		param->u.crypt.key_len = params->key_len;
		_rtw_memcpy(param->u.crypt.key, (u8 *)params->key, params->key_len);
	}

	if (MLME_IS_STA(padapter)) {
#ifdef CONFIG_TDLS
		if (rtw_tdls_is_driver_setup(padapter) == _FALSE && mac_addr) {
			ptdls_sta = rtw_get_stainfo(&padapter->stapriv, (void *)mac_addr);
			if (ptdls_sta != NULL && ptdls_sta->tdls_sta_state) {
				_rtw_memcpy(ptdls_sta->tpk.tk, params->key, params->key_len);
				goto addkey_end;
			}
		}
#endif /* CONFIG_TDLS */
		ret = rtw_cfg80211_set_encryption(ndev, param);
	} else if (MLME_IS_AP(padapter) || MLME_IS_MESH(padapter)) {
#ifdef CONFIG_AP_MODE
		if (mac_addr)
			_rtw_memcpy(param->sta_addr, (void *)mac_addr, ETH_ALEN);

		ret = rtw_cfg80211_ap_set_encryption(ndev, param);
#endif
	} else if (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE
		|| check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE
	) {
		/* RTW_INFO("@@@@@@@@@@ fw_state=0x%x, iftype=%d\n", pmlmepriv->fw_state, rtw_wdev->iftype); */
		ret = rtw_cfg80211_set_encryption(ndev, param);
	} else
		RTW_INFO("error! fw_state=0x%x, iftype=%d\n", pmlmepriv->fw_state, rtw_wdev->iftype);


addkey_end:
	if (param)
		rtw_mfree(param, param_len);

	return ret;

}

static int cfg80211_rtw_get_key(struct wiphy *wiphy, struct net_device *ndev
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	, int link_id
#endif
	, u8 keyid
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	, bool pairwise
#endif
	, const u8 *mac_addr, void *cookie
	, void (*callback)(void *cookie, struct key_params *))
{
#define GET_KEY_PARAM_FMT_S " keyid=%d"
#define GET_KEY_PARAM_ARG_S , keyid
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	#define GET_KEY_PARAM_FMT_2_6_37 ", pairwise=%d"
	#define GET_KEY_PARAM_ARG_2_6_37 , pairwise
#else
	#define GET_KEY_PARAM_FMT_2_6_37 ""
	#define GET_KEY_PARAM_ARG_2_6_37
#endif
#define GET_KEY_PARAM_FMT_E ", addr=%pM"
#define GET_KEY_PARAM_ARG_E , mac_addr

	_adapter *adapter = (_adapter *)rtw_netdev_priv(ndev);
	struct security_priv *sec = &adapter->securitypriv;
	struct sta_priv *stapriv = &adapter->stapriv;
	struct sta_info *sta = NULL;
	u32 cipher = _NO_PRIVACY_;
	union Keytype *key = NULL;
	u8 key_len = 0;
	u64 *pn = NULL;
	u8 pn_len = 0;
	u8 pn_val[8] = {0};

	struct key_params params;
	int ret = -ENOENT;

	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *adapter_link = GET_PRIMARY_LINK(adapter);
	struct link_security_priv *lsec = &adapter_link->securitypriv;


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	RTW_INFO(FUNC_NDEV_FMT" link_id=%d\n", FUNC_NDEV_ARG(ndev), link_id);
#endif

	if (keyid >= WEP_KEYS
		#ifdef CONFIG_IEEE80211W
		&& keyid > BIP_MAX_KEYID
		#endif
	)
		goto exit;

	if (!mac_addr || is_broadcast_ether_addr(mac_addr)
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
		|| (MLME_IS_STA(adapter) && !pairwise)
		#endif
	) {
		/* WEP key, TX GTK/IGTK, RX GTK/IGTK(for STA mode) */
		if (is_wep_enc(sec->dot118021XGrpPrivacy))
		{
			if (keyid >= WEP_KEYS)
				goto exit;
			if (!(sec->key_mask & BIT(keyid)))
				goto exit;
			cipher = sec->dot118021XGrpPrivacy;
			key = &sec->dot11DefKey[keyid];
		} else {
			if (keyid < WEP_KEYS) {
				if (lsec->binstallGrpkey != _TRUE)
					goto exit;
				cipher = sec->dot118021XGrpPrivacy;
				key = &lsec->dot118021XGrpKey[keyid];
				sta = rtw_get_bcmc_stainfo(adapter, adapter_link);
				if (sta)
					pn = &sta->dot11txpn.val;
			#ifdef CONFIG_IEEE80211W
			} else if (keyid <= BIP_MAX_KEYID) {
				if (SEC_IS_BIP_KEY_INSTALLED(lsec) != _TRUE)
					goto exit;
				cipher = sec->dot11wCipher;
				key = &lsec->dot11wBIPKey[keyid];
				pn = &lsec->dot11wBIPtxpn.val;
			#endif
			}
		}
	} else {
		/* Pairwise key, RX GTK/IGTK for specific peer */
		sta = rtw_get_stainfo(stapriv, mac_addr);
		if (!sta)
			goto exit;

		if (keyid < WEP_KEYS && pairwise) {
			if (sta->bpairwise_key_installed != _TRUE)
				goto exit;
			cipher = sta->dot118021XPrivacy;
			key = &sta->dot118021x_UncstKey;
		#ifdef CONFIG_RTW_MESH
		} else if (keyid < WEP_KEYS && !pairwise) {
			if (!(sta->gtk_bmp & BIT(keyid)))
				goto exit;
			cipher = sta->group_privacy;
			key = &sta->gtk;
		#ifdef CONFIG_IEEE80211W
		} else if (keyid <= BIP_MAX_KEYID && !pairwise) {
			if (!(sta->igtk_bmp & BIT(keyid)))
				goto exit;
			cipher = sta->dot11wCipher;
			key = &sta->igtk;
			pn = &sta->igtk_pn.val;
		#endif
		#endif /* CONFIG_RTW_MESH */
		}
	}

	if (!key)
		goto exit;

	if (cipher == _WEP40_) {
		cipher = WLAN_CIPHER_SUITE_WEP40;
		key_len = sec->dot11DefKeylen[keyid];
	} else if (cipher == _WEP104_) {
		cipher = WLAN_CIPHER_SUITE_WEP104;
		key_len = sec->dot11DefKeylen[keyid];
	} else if (cipher == _TKIP_ || cipher == _TKIP_WTMIC_) {
		cipher = WLAN_CIPHER_SUITE_TKIP;
		key_len = 16;
	} else if (cipher == _AES_) {
		cipher = WLAN_CIPHER_SUITE_CCMP;
		key_len = 16;
#ifdef CONFIG_WAPI_SUPPORT
	} else if (cipher == _SMS4_) {
		cipher = WLAN_CIPHER_SUITE_SMS4;
		key_len = 16;
#endif
	} else if (cipher == _GCMP_) {
		cipher = WIFI_CIPHER_SUITE_GCMP;
		key_len = 16;
	} else if (cipher == _CCMP_256_) {
		cipher = WIFI_CIPHER_SUITE_CCMP_256;
		key_len = 32;
	} else if (cipher == _GCMP_256_) {
		cipher = WIFI_CIPHER_SUITE_GCMP_256;
		key_len = 32;
	#ifdef CONFIG_IEEE80211W
	} else if (cipher == _BIP_CMAC_128_) {
		cipher = WLAN_CIPHER_SUITE_AES_CMAC;
		key_len = 16;
	} else if (cipher == _BIP_GMAC_128_) {
		cipher = WIFI_CIPHER_SUITE_BIP_GMAC_128;
		key_len = 16;
	} else if (cipher == _BIP_GMAC_256_) {
		cipher = WIFI_CIPHER_SUITE_BIP_GMAC_256;
		key_len = 32;
	} else if (cipher == _BIP_CMAC_256_) {
		cipher = WIFI_CIPHER_SUITE_BIP_CMAC_256;
		key_len = 32;
	#endif
	} else {
		RTW_WARN(FUNC_NDEV_FMT" unknown cipher:%u\n", FUNC_NDEV_ARG(ndev), cipher);
		rtw_warn_on(1);
		goto exit;
	}

	if (pn) {
		*((u64 *)pn_val) = cpu_to_le64(*pn);
		pn_len = 6;
	}

	ret = 0;
	
exit:
	RTW_INFO(FUNC_NDEV_FMT
		GET_KEY_PARAM_FMT_S
		GET_KEY_PARAM_FMT_2_6_37
		GET_KEY_PARAM_FMT_E
		" ret %d\n", FUNC_NDEV_ARG(ndev)
		GET_KEY_PARAM_ARG_S
		GET_KEY_PARAM_ARG_2_6_37
		GET_KEY_PARAM_ARG_E
		, ret);
	if (pn)
		RTW_INFO(FUNC_NDEV_FMT " seq:0x%llx\n", FUNC_NDEV_ARG(ndev), *pn);

	if (ret == 0) {
		_rtw_memset(&params, 0, sizeof(params));

		params.cipher = cipher;
		params.key = key->skey;
		params.key_len = key_len;
		if (pn) {
			params.seq = pn_val;
			params.seq_len = pn_len;
		}

		callback(cookie, &params);
	}

	return ret;
}

static int cfg80211_rtw_del_key(struct wiphy *wiphy, struct net_device *ndev
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	, int link_id
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	, u8 key_index, bool pairwise, const u8 *mac_addr)
#else /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) */
	, u8 key_index, const u8 *mac_addr)
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) */
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct security_priv *psecuritypriv = &padapter->securitypriv;

	RTW_INFO(FUNC_NDEV_FMT" key_index=%d, addr=%pM\n", FUNC_NDEV_ARG(ndev), key_index, mac_addr);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	RTW_INFO(FUNC_NDEV_FMT" link_id=%d\n", FUNC_NDEV_ARG(ndev), link_id);
#endif

	if (key_index == psecuritypriv->dot11PrivacyKeyIndex) {
		/* clear the flag of wep default key set. */
		psecuritypriv->bWepDefaultKeyIdxSet = 0;
	}

	return 0;
}

static int cfg80211_rtw_set_default_key(struct wiphy *wiphy, struct net_device *ndev
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	, int link_id
#endif
	, u8 key_index
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)) || defined(COMPAT_KERNEL_RELEASE)
	, bool unicast, bool multicast
	#endif
)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct security_priv *psecuritypriv = &padapter->securitypriv;

#define SET_DEF_KEY_PARAM_FMT " key_index=%d"
#define SET_DEF_KEY_PARAM_ARG , key_index
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)) || defined(COMPAT_KERNEL_RELEASE)
	#define SET_DEF_KEY_PARAM_FMT_2_6_38 ", unicast=%d, multicast=%d"
	#define SET_DEF_KEY_PARAM_ARG_2_6_38 , unicast, multicast
#else
	#define SET_DEF_KEY_PARAM_FMT_2_6_38 ""
	#define SET_DEF_KEY_PARAM_ARG_2_6_38
#endif

	RTW_INFO(FUNC_NDEV_FMT
		SET_DEF_KEY_PARAM_FMT
		SET_DEF_KEY_PARAM_FMT_2_6_38
		"\n", FUNC_NDEV_ARG(ndev)
		SET_DEF_KEY_PARAM_ARG
		SET_DEF_KEY_PARAM_ARG_2_6_38
	);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	RTW_INFO(FUNC_NDEV_FMT" link_id=%d\n", FUNC_NDEV_ARG(ndev), link_id);
#endif

	/* ToDo CONFIG_RTW_MLD */
	if ((key_index < WEP_KEYS) && ((psecuritypriv->dot11PrivacyAlgrthm == _WEP40_) || (psecuritypriv->dot11PrivacyAlgrthm == _WEP104_))) { /* set wep default key */
		psecuritypriv->ndisencryptstatus = Ndis802_11Encryption1Enabled;

		psecuritypriv->dot11PrivacyKeyIndex = key_index;

		psecuritypriv->dot11PrivacyAlgrthm = _WEP40_;
		psecuritypriv->dot118021XGrpPrivacy = _WEP40_;
		if (psecuritypriv->dot11DefKeylen[key_index] == WLAN_KEY_LEN_WEP104) {
			psecuritypriv->dot11PrivacyAlgrthm = _WEP104_;
			psecuritypriv->dot118021XGrpPrivacy = _WEP104_;
		}

		psecuritypriv->bWepDefaultKeyIdxSet = 1; /* set the flag to represent that wep default key has been set */
	}

	return 0;

}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30))
int cfg80211_rtw_set_default_mgmt_key(struct wiphy *wiphy, struct net_device *ndev
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	, int link_id
#endif
	, u8 key_index)
{
#define SET_DEF_KEY_PARAM_FMT " key_index=%d"
#define SET_DEF_KEY_PARAM_ARG , key_index

	RTW_INFO(FUNC_NDEV_FMT
		SET_DEF_KEY_PARAM_FMT
		"\n", FUNC_NDEV_ARG(ndev)
		SET_DEF_KEY_PARAM_ARG
	);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)) || defined(CONFIG_MLD_KERNEL_PATCH)
	RTW_INFO(FUNC_NDEV_FMT" link_id=%d\n", FUNC_NDEV_ARG(ndev), link_id);
#endif

	/* ToDo CONFIG_RTW_MLD */
	return 0;
}
#endif

#if defined(CONFIG_GTK_OL) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0))
static int cfg80211_rtw_set_rekey_data(struct wiphy *wiphy,
	struct net_device *ndev,
	struct cfg80211_gtk_rekey_data *data)
{
	/*int i;*/
	struct sta_info *psta;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct mlme_priv   *pmlmepriv = &padapter->mlmepriv;
	struct sta_priv *pstapriv = &padapter->stapriv;
	struct security_priv *psecuritypriv = &(padapter->securitypriv);

	psta = rtw_get_stainfo(pstapriv, get_bssid(pmlmepriv));
	if (psta == NULL) {
		RTW_INFO("%s, : Obtain Sta_info fail\n", __func__);
		return -1;
	}

	_rtw_memcpy(psta->kek, data->kek, NL80211_KEK_LEN);
	/*printk("\ncfg80211_rtw_set_rekey_data KEK:");
	for(i=0;i<NL80211_KEK_LEN; i++)
		printk(" %02x ", psta->kek[i]);*/
	_rtw_memcpy(psta->kck, data->kck, NL80211_KCK_LEN);
	/*printk("\ncfg80211_rtw_set_rekey_data KCK:");
	for(i=0;i<NL80211_KCK_LEN; i++)
		printk(" %02x ", psta->kck[i]);*/
	_rtw_memcpy(psta->replay_ctr, data->replay_ctr, NL80211_REPLAY_CTR_LEN);
	psecuritypriv->binstallKCK_KEK = _TRUE;
	/*printk("\nREPLAY_CTR: ");
	for(i=0;i<RTW_REPLAY_CTR_LEN; i++)
		printk(" %02x ", psta->replay_ctr[i]);*/

	return 0;
}
#endif /*CONFIG_GTK_OL*/

#ifdef CONFIG_RTW_MESH
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
static enum nl80211_mesh_power_mode rtw_mesh_ps_to_nl80211_mesh_power_mode(u8 ps)
{
	if (ps == RTW_MESH_PS_UNKNOWN)
		return NL80211_MESH_POWER_UNKNOWN;
	if (ps == RTW_MESH_PS_ACTIVE)
		return NL80211_MESH_POWER_ACTIVE;
	if (ps == RTW_MESH_PS_LSLEEP)
		return NL80211_MESH_POWER_LIGHT_SLEEP;
	if (ps == RTW_MESH_PS_DSLEEP)
		return NL80211_MESH_POWER_DEEP_SLEEP;

	rtw_warn_on(1);
	return NL80211_MESH_POWER_UNKNOWN;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
enum nl80211_plink_state rtw_plink_state_to_nl80211_plink_state(u8 plink_state)
{
	if (plink_state == RTW_MESH_PLINK_UNKNOWN)
		return NUM_NL80211_PLINK_STATES;
	if (plink_state == RTW_MESH_PLINK_LISTEN)
		return NL80211_PLINK_LISTEN;
	if (plink_state == RTW_MESH_PLINK_OPN_SNT)
		return NL80211_PLINK_OPN_SNT;
	if (plink_state == RTW_MESH_PLINK_OPN_RCVD)
		return NL80211_PLINK_OPN_RCVD;
	if (plink_state == RTW_MESH_PLINK_CNF_RCVD)
		return NL80211_PLINK_CNF_RCVD;
	if (plink_state == RTW_MESH_PLINK_ESTAB)
		return NL80211_PLINK_ESTAB;
	if (plink_state == RTW_MESH_PLINK_HOLDING)
		return NL80211_PLINK_HOLDING;
	if (plink_state == RTW_MESH_PLINK_BLOCKED)
		return NL80211_PLINK_BLOCKED;

	rtw_warn_on(1);
	return NUM_NL80211_PLINK_STATES;
}

u8 nl80211_plink_state_to_rtw_plink_state(enum nl80211_plink_state plink_state)
{
	if (plink_state == NL80211_PLINK_LISTEN)
		return RTW_MESH_PLINK_LISTEN;
	if (plink_state == NL80211_PLINK_OPN_SNT)
		return RTW_MESH_PLINK_OPN_SNT;
	if (plink_state == NL80211_PLINK_OPN_RCVD)
		return RTW_MESH_PLINK_OPN_RCVD;
	if (plink_state == NL80211_PLINK_CNF_RCVD)
		return RTW_MESH_PLINK_CNF_RCVD;
	if (plink_state == NL80211_PLINK_ESTAB)
		return RTW_MESH_PLINK_ESTAB;
	if (plink_state == NL80211_PLINK_HOLDING)
		return RTW_MESH_PLINK_HOLDING;
	if (plink_state == NL80211_PLINK_BLOCKED)
		return RTW_MESH_PLINK_BLOCKED;

	rtw_warn_on(1);
	return RTW_MESH_PLINK_UNKNOWN;
}
#endif

static void rtw_cfg80211_fill_mesh_only_sta_info(struct mesh_plink_ent *plink, struct sta_info *sta, struct station_info *sinfo)
{
	sinfo->filled |= STATION_INFO_LLID;
	sinfo->llid = plink->llid;
	sinfo->filled |= STATION_INFO_PLID;
	sinfo->plid = plink->plid;
	sinfo->filled |= STATION_INFO_PLINK_STATE;
	sinfo->plink_state = rtw_plink_state_to_nl80211_plink_state(plink->plink_state);
	if (!sta && plink->scanned) {
		sinfo->filled |= STATION_INFO_SIGNAL;
		sinfo->signal = rtw_phl_rssi_to_dbm(plink->scanned->network.PhyInfo.SignalStrength);
		sinfo->filled |= STATION_INFO_INACTIVE_TIME;
		if (plink->plink_state == RTW_MESH_PLINK_UNKNOWN)
			sinfo->inactive_time = 0 - 1;
		else
			sinfo->inactive_time = rtw_get_passing_time_ms(plink->scanned->last_scanned);
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	if (sta) {
		sinfo->filled |= STATION_INFO_LOCAL_PM;
		sinfo->local_pm = rtw_mesh_ps_to_nl80211_mesh_power_mode(sta->local_mps);
		sinfo->filled |= STATION_INFO_PEER_PM;
		sinfo->peer_pm = rtw_mesh_ps_to_nl80211_mesh_power_mode(sta->peer_mps);
		sinfo->filled |= STATION_INFO_NONPEER_PM;
		sinfo->nonpeer_pm = rtw_mesh_ps_to_nl80211_mesh_power_mode(sta->nonpeer_mps);
	}
#endif
}
#endif /* CONFIG_RTW_MESH */

static void rtw_desc_rate_to_nss_mcs(u16 rate_idx, u8 *rate_mode, u8 *nss, u8 *mcs)
{
	u8 mcs_in = 0, nss_in = 0;

	/* enum rtw_data_rate */
	if ((RTW_DATA_RATE_MCS0 <= rate_idx) &&
	   (rate_idx <= RTW_DATA_RATE_MCS31)) {
		*rate_mode = RTW_HT_MODE;
		mcs_in = rate_idx - RTW_DATA_RATE_MCS0;
	} else if ((RTW_DATA_RATE_VHT_NSS1_MCS0 <= rate_idx) &&
		   (rate_idx <= RTW_DATA_RATE_VHT_NSS4_MCS9)) {
		*rate_mode = RTW_VHT_MODE;
		mcs_in = rate_idx & 0xF;
		nss_in = ((rate_idx - RTW_DATA_RATE_VHT_NSS1_MCS0) >> 4) + 1;
	} else if ((RTW_DATA_RATE_HE_NSS1_MCS0 <= rate_idx) &&
		   (rate_idx <= RTW_DATA_RATE_HE_NSS4_MCS11)) {
		*rate_mode = RTW_HE_MODE;
		mcs_in = rate_idx & 0xF;
		nss_in = ((rate_idx - RTW_DATA_RATE_HE_NSS1_MCS0) >> 4) + 1;
	} else {
		*rate_mode = RTW_LEGACY_MODE;
	}

	if (nss)
		*nss = nss_in;
	if (mcs)
		*mcs = mcs_in;
}

static void sta_set_rate_info(_adapter *adapter, struct rate_info *rinfo,
			      u16 rtw_rate_idx, u8 sgi, u8 bw)
{
	u8 mcs = 0;
	u8 nss = 0;
	u8 mod = 0;

	rinfo->flags = 0;
	rtw_desc_rate_to_nss_mcs(rtw_rate_idx, &mod, &nss, &mcs);

	if (sgi)
		rinfo->flags |= RATE_INFO_FLAGS_SHORT_GI;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	if (mod == RTW_HE_MODE) {
		rinfo->flags |= RATE_INFO_FLAGS_HE_MCS;
		rinfo->bw = bw == CHANNEL_WIDTH_160 ? RATE_INFO_BW_160 :
			    bw == CHANNEL_WIDTH_80 ? RATE_INFO_BW_80 :
			    bw == CHANNEL_WIDTH_40 ? RATE_INFO_BW_40 : RATE_INFO_BW_20;
		rinfo->nss = nss;
		rinfo->mcs = mcs;
	} else if (mod == RTW_VHT_MODE) {
		rinfo->flags |= RATE_INFO_FLAGS_VHT_MCS;
		rinfo->bw = bw == CHANNEL_WIDTH_160 ? RATE_INFO_BW_160 :
			    bw == CHANNEL_WIDTH_80 ? RATE_INFO_BW_80 :
			    bw == CHANNEL_WIDTH_40 ? RATE_INFO_BW_40 : RATE_INFO_BW_20;
		rinfo->nss = nss;
		rinfo->mcs = mcs;
	} else if (mod == RTW_HT_MODE) {
		rinfo->flags |= RATE_INFO_FLAGS_MCS;
		rinfo->bw = bw ? RATE_INFO_BW_40 : RATE_INFO_BW_20;
		rinfo->mcs = mcs;
	}
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	if (mod == RTW_VHT_MODE) {
		rinfo->flags |= RATE_INFO_FLAGS_VHT_MCS;
		rinfo->bw = bw == CHANNEL_WIDTH_160 ? RATE_INFO_BW_160 :
			    bw == CHANNEL_WIDTH_80 ? RATE_INFO_BW_80 :
			    bw == CHANNEL_WIDTH_40 ? RATE_INFO_BW_40 : RATE_INFO_BW_20;
		rinfo->nss = nss;
		rinfo->mcs = mcs;
	} else if (mod == RTW_HT_MODE) {
		rinfo->flags |= RATE_INFO_FLAGS_MCS;
		rinfo->bw = bw ? RATE_INFO_BW_40 : RATE_INFO_BW_20;
		rinfo->mcs = mcs;
	}
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	if (mod == RTW_VHT_MODE) {
		rinfo->flags |= RATE_INFO_FLAGS_VHT_MCS;
		rinfo->flags |= bw == CHANNEL_WIDTH_160 ? RATE_INFO_FLAGS_160_MHZ_WIDTH :
				bw == CHANNEL_WIDTH_80 ? RATE_INFO_FLAGS_80_MHZ_WIDTH :
				bw == CHANNEL_WIDTH_40 ? RATE_INFO_FLAGS_40_MHZ_WIDTH : 0;
		rinfo->nss = nss;
		rinfo->mcs = mcs;
	} else if (mod == RTW_HT_MODE) {
		rinfo->flags |= RATE_INFO_FLAGS_MCS;
		rinfo->flags |= bw ? RATE_INFO_FLAGS_40_MHZ_WIDTH : 0;
		rinfo->mcs = mcs;
	}
#else
	if (mod == RTW_VHT_MODE) {
		rinfo->legacy = 0;
		RTW_INFO("Cannot report VHT rate in current kernel version\n");
	} else if (mod == RTW_HT_MODE) {
		rinfo->flags |= RATE_INFO_FLAGS_MCS;
		rinfo->flags |= bw ? RATE_INFO_FLAGS_40_MHZ_WIDTH : 0;
		rinfo->mcs = mcs;
	}
#endif
	else {
		rinfo->legacy = rtw_desc_rate_to_bitrate(0, rtw_rate_idx, 0);
	}
}

static int cfg80211_rtw_get_station(struct wiphy *wiphy,
	struct net_device *ndev,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0))
	u8 *mac,
#else
	const u8 *mac,
#endif
	struct station_info *sinfo)
{
	int ret = 0;
	u8 bw, sgi;
	u16 rtw_rate_idx;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct sta_info *psta = NULL;
	struct sta_priv *pstapriv = &padapter->stapriv;
#ifdef CONFIG_RTW_MESH
	struct mesh_plink_ent *plink = NULL;
#endif
	sinfo->filled = 0;

	if (!mac) {
		RTW_INFO(FUNC_NDEV_FMT" mac==%p\n", FUNC_NDEV_ARG(ndev), mac);
		ret = -ENOENT;
		goto exit;
	}

	psta = rtw_get_stainfo(pstapriv, mac);
#ifdef CONFIG_RTW_MESH
	if (MLME_IS_MESH(padapter)) {
		if (psta)
			plink = psta->plink;
		if (!plink)
			plink = rtw_mesh_plink_get(padapter, mac);
	}
#endif /* CONFIG_RTW_MESH */

	if ((!MLME_IS_MESH(padapter) && !psta)
		#ifdef CONFIG_RTW_MESH
		|| (MLME_IS_MESH(padapter) && !plink)
		#endif
	) {
		RTW_INFO(FUNC_NDEV_FMT" no sta info for mac="MAC_FMT"\n"
			, FUNC_NDEV_ARG(ndev), MAC_ARG(mac));
		ret = -ENOENT;
		goto exit;
	}

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO(FUNC_NDEV_FMT" mac="MAC_FMT"\n", FUNC_NDEV_ARG(ndev), MAC_ARG(mac));
#endif

	/* for infra./P2PClient mode */
	if (MLME_IS_STA(padapter)
		&& check_fwstate(pmlmepriv, WIFI_ASOC_STATE)) {
		struct wlan_network  *cur_network = &(pmlmepriv->dev_cur_network);

		if (_rtw_memcmp((u8 *)mac, cur_network->network.MacAddress, ETH_ALEN) == _FALSE) {
			RTW_INFO("%s, mismatch bssid="MAC_FMT"\n", __func__, MAC_ARG(cur_network->network.MacAddress));
			ret = -ENOENT;
			goto exit;
		}

		sinfo->filled |= STATION_INFO_SIGNAL;
		sinfo->signal = rtw_phl_rssi_to_dbm(padapter->recvinfo.signal_strength);

		sinfo->filled |= STATION_INFO_TX_BITRATE;
		sinfo->txrate.legacy = rtw_get_cur_max_rate(padapter);
	}

	if (psta) {
		if (!MLME_IS_STA(padapter)
			|| check_fwstate(pmlmepriv, WIFI_ASOC_STATE) == _FALSE
		) {
			sinfo->filled |= STATION_INFO_SIGNAL;
			/* ToDo: need API to query hal_sta->rssi_stat.rssi */
			/* sinfo->signal = rtw_phl_rssi_to_dbm(psta->phl_sta->rssi_stat.rssi); */
		}
		sinfo->filled |= STATION_INFO_INACTIVE_TIME;
		sinfo->inactive_time = rtw_get_passing_time_ms(psta->sta_stats.last_rx_time);
		sinfo->filled |= STATION_INFO_RX_PACKETS;
		sinfo->rx_packets = sta_rx_data_pkts(psta);
		sinfo->filled |= STATION_INFO_TX_PACKETS;
		sinfo->tx_packets = psta->sta_stats.tx_pkts;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
		sinfo->filled |= STATION_INFO_RX_BYTES64;
		sinfo->filled |= STATION_INFO_TX_BYTES64;
#else
		sinfo->filled |= STATION_INFO_RX_BYTES;
		sinfo->filled |= STATION_INFO_TX_BYTES;
#endif
		sinfo->rx_bytes = psta->sta_stats.rx_bytes;
		sinfo->tx_bytes = psta->sta_stats.tx_bytes;

		/* Although according to cfg80211.h struct station_info */
		/* @txrate: current unicast bitrate from this station */
		/* We still report sinfo->txrate as bitrate to this station */
		sinfo->filled |= STATION_INFO_TX_BITRATE;
		rtw_rate_idx = rtw_get_current_tx_rate(padapter, psta);
		sgi = rtw_get_current_tx_sgi(padapter, psta);
		bw = psta->phl_sta->rlink->chandef.bw;
		sta_set_rate_info(padapter, &sinfo->txrate, rtw_rate_idx, sgi, bw);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
		/* Although @rxrate: current unicast bitrate to this station */
		/* We report sinfo->rxrate as bitrate from this station */
		sinfo->filled |= STATION_INFO_RX_BITRATE;
		rtw_get_current_rx_info(padapter, psta, &rtw_rate_idx, &bw, &sgi);
		sta_set_rate_info(padapter, &sinfo->rxrate, rtw_rate_idx, sgi, bw);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
		if (rtw_get_sta_tx_stat(padapter, psta) != RTW_NOT_SUPPORT) {
			sinfo->filled |= STATION_INFO_TX_FAILED;
			sinfo->filled |= STATION_INFO_TX_RETRIES;
			sinfo->tx_failed = psta->sta_stats.tx_fail_cnt_sum;
			sinfo->tx_retries = psta->sta_stats.tx_retry_cnt_sum;
		}
#endif
	}

#ifdef CONFIG_RTW_MESH
	if (MLME_IS_MESH(padapter))
		rtw_cfg80211_fill_mesh_only_sta_info(plink, psta, sinfo);
#endif

exit:
	return ret;
}

extern int netdev_open(struct net_device *pnetdev);

#if 0
enum nl80211_iftype {
	NL80211_IFTYPE_UNSPECIFIED,
	NL80211_IFTYPE_ADHOC, /* 1 */
	NL80211_IFTYPE_STATION, /* 2 */
	NL80211_IFTYPE_AP, /* 3 */
	NL80211_IFTYPE_AP_VLAN,
	NL80211_IFTYPE_WDS,
	NL80211_IFTYPE_MONITOR, /* 6 */
	NL80211_IFTYPE_MESH_POINT,
	NL80211_IFTYPE_P2P_CLIENT, /* 8 */
	NL80211_IFTYPE_P2P_GO, /* 9 */
	/* keep last */
	NUM_NL80211_IFTYPES,
	NL80211_IFTYPE_MAX = NUM_NL80211_IFTYPES - 1
};
#endif
static int cfg80211_rtw_change_iface(struct wiphy *wiphy,
				     struct net_device *ndev,
				     enum nl80211_iftype type,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)) && !defined(CPTCFG_VERSION)
				     u32 *flags,
#endif
				     struct vif_params *params)
{
	enum nl80211_iftype old_type;
	NDIS_802_11_NETWORK_INFRASTRUCTURE networkType;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct wireless_dev *rtw_wdev = padapter->rtw_wdev;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo = &(padapter->wdinfo);
#endif
#ifdef CONFIG_MONITOR_MODE_XMIT
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
#endif
	int ret = 0;
	u8 change = _FALSE;

	RTW_INFO(FUNC_NDEV_FMT" type=%d\n", FUNC_NDEV_ARG(ndev), type);

	if (adapter_to_dvobj(padapter)->processing_dev_remove == _TRUE) {
		ret = -EPERM;
		goto exit;
	}


	RTW_INFO(FUNC_NDEV_FMT" call netdev_open\n", FUNC_NDEV_ARG(ndev));
	if (netdev_open(ndev) != 0) {
		RTW_INFO(FUNC_NDEV_FMT" call netdev_open fail\n", FUNC_NDEV_ARG(ndev));
		ret = -EPERM;
		goto exit;
	}

	old_type = rtw_wdev->iftype;
	RTW_INFO(FUNC_NDEV_FMT" old_iftype=%d, new_iftype=%d\n",
		FUNC_NDEV_ARG(ndev), old_type, type);

	if (old_type != type) {
		change = _TRUE;
		pmlmeext->action_public_rxseq = 0xffff;
		pmlmeext->action_public_dialog_token = 0xff;
	}

#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
	if (type != NL80211_IFTYPE_P2P_CLIENT && type != NL80211_IFTYPE_P2P_GO) {
		if (!rtw_p2p_chk_role(pwdinfo, P2P_ROLE_DISABLE)) {
			if (!rtw_p2p_enable(padapter, P2P_ROLE_DISABLE)) {
				ret = -EOPNOTSUPP;
				goto exit;
			}
		}
	}
#endif

	/* initial default type */
	ndev->type = ARPHRD_ETHER;

	switch (type) {
	case NL80211_IFTYPE_ADHOC:
		networkType = Ndis802_11IBSS;
		break;

	case NL80211_IFTYPE_STATION:
		networkType = Ndis802_11Infrastructure;
		break;

	case NL80211_IFTYPE_AP:
		networkType = Ndis802_11APMode;
		break;

#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
	case NL80211_IFTYPE_P2P_CLIENT:
		networkType = Ndis802_11Infrastructure;
		if (change) {
			if (!rtw_p2p_enable(padapter, P2P_ROLE_CLIENT)) {
				ret = -EOPNOTSUPP;
				goto exit;
			}
		}
		break;

	case NL80211_IFTYPE_P2P_GO:
		networkType = Ndis802_11APMode;
		if (change) {
			if (!rtw_p2p_enable(padapter, P2P_ROLE_GO)) {
				ret = -EOPNOTSUPP;
				goto exit;
			}
		}
		break;
#endif /* defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)) */

#ifdef CONFIG_RTW_MESH
	case NL80211_IFTYPE_MESH_POINT:
		networkType = Ndis802_11_mesh;
		break;
#endif

#ifdef CONFIG_WIFI_MONITOR
	case NL80211_IFTYPE_MONITOR:
		networkType = Ndis802_11Monitor;

#ifdef CONFIG_CUSTOMER_ALIBABA_GENERAL
		ndev->type = ARPHRD_IEEE80211; /* IEEE 802.11 : 801 */
#else
		ndev->type = ARPHRD_IEEE80211_RADIOTAP; /* IEEE 802.11 + radiotap header : 803 */
#endif
		break;
#endif /* CONFIG_WIFI_MONITOR */
	default:
		ret = -EOPNOTSUPP;
		goto exit;
	}

	rtw_wdev->iftype = type;

	if (rtw_set_802_11_infrastructure_mode(padapter, networkType, 0) == _FALSE) {
		rtw_wdev->iftype = old_type;
		ret = -EPERM;
		goto exit;
	}
#ifdef CONFIG_HWSIM
	rtw_setopmode_cmd(padapter, networkType, RTW_CMDF_DIRECTLY);
#else
	rtw_setopmode_cmd(padapter, networkType, RTW_CMDF_WAIT_ACK);
#endif

#ifdef CONFIG_MONITOR_MODE_XMIT
	if (check_fwstate(pmlmepriv, WIFI_MONITOR_STATE) == _TRUE)
		rtw_indicate_connect(padapter);
#endif

	#if defined(CONFIG_RTW_WDS) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))
	if (params->use_4addr != -1) {
		RTW_INFO(FUNC_NDEV_FMT" use_4addr=%d\n",
			 FUNC_NDEV_ARG(ndev), params->use_4addr);
		adapter_set_use_wds(padapter, params->use_4addr);
	}
	#endif

exit:

	RTW_INFO(FUNC_NDEV_FMT" ret:%d\n", FUNC_NDEV_ARG(ndev), ret);
	return ret;
}

void rtw_cfg80211_indicate_scan_done(_adapter *adapter, bool aborted)
{
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(adapter);

#if defined(CPTCFG_VERSION) || (KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE)
	struct cfg80211_scan_info info;

	memset(&info, 0, sizeof(info));
	info.aborted = aborted;
#endif

	_rtw_spinlock_bh(&pwdev_priv->scan_req_lock);
	if (pwdev_priv->scan_request != NULL) {
		#ifdef CONFIG_DEBUG_CFG80211
		RTW_INFO("%s with scan req\n", __FUNCTION__);
		#endif

		/* avoid WARN_ON(request != wiphy_to_dev(request->wiphy)->scan_req); */
		if (pwdev_priv->scan_request->wiphy != pwdev_priv->rtw_wdev->wiphy)
			RTW_INFO("error wiphy compare\n");
		else
#if defined(CPTCFG_VERSION) || (KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE)
			cfg80211_scan_done(pwdev_priv->scan_request, &info);
#else
			cfg80211_scan_done(pwdev_priv->scan_request, aborted);
#endif

		pwdev_priv->scan_request = NULL;
	} else {
		#ifdef CONFIG_DEBUG_CFG80211
		RTW_INFO("%s without scan req\n", __FUNCTION__);
		#endif
	}
	_rtw_spinunlock_bh(&pwdev_priv->scan_req_lock);
}

u32 rtw_cfg80211_wait_scan_req_empty(_adapter *adapter, u32 timeout_ms)
{
	struct rtw_wdev_priv *wdev_priv = adapter_wdev_data(adapter);
	u8 empty = _FALSE;
	systime start;
	u32 pass_ms;

	start = rtw_get_current_time();

	while (rtw_get_passing_time_ms(start) <= timeout_ms) {

		if (RTW_CANNOT_RUN(adapter_to_dvobj(adapter)))
			break;

		if (!wdev_priv->scan_request) {
			empty = _TRUE;
			break;
		}

		rtw_msleep_os(10);
	}

	pass_ms = rtw_get_passing_time_ms(start);

	if (empty == _FALSE && pass_ms > timeout_ms)
		RTW_PRINT(FUNC_ADPT_FMT" pass_ms:%u, timeout\n"
			, FUNC_ADPT_ARG(adapter), pass_ms);

	return pass_ms;
}

void rtw_cfg80211_unlink_bss(_adapter *padapter, struct wlan_network *pnetwork)
{
	struct wireless_dev *pwdev = padapter->rtw_wdev;
	struct wiphy *wiphy = pwdev->wiphy;
	struct cfg80211_bss *bss = NULL;
	WLAN_BSSID_EX select_network = pnetwork->network;

	bss = cfg80211_get_bss(wiphy, NULL/*notify_channel*/,
		select_network.MacAddress, select_network.Ssid.Ssid,
		select_network.Ssid.SsidLength,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
		select_network.InfrastructureMode == Ndis802_11Infrastructure?IEEE80211_BSS_TYPE_ESS:IEEE80211_BSS_TYPE_IBSS,
		IEEE80211_PRIVACY(select_network.Privacy));
#else
		select_network.InfrastructureMode == Ndis802_11Infrastructure?WLAN_CAPABILITY_ESS:WLAN_CAPABILITY_IBSS,
		select_network.InfrastructureMode == Ndis802_11Infrastructure?WLAN_CAPABILITY_ESS:WLAN_CAPABILITY_IBSS);
#endif

	if (bss) {
		cfg80211_unlink_bss(wiphy, bss);
		RTW_INFO("%s(): cfg80211_unlink %s!!\n", __func__, select_network.Ssid.Ssid);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
		cfg80211_put_bss(padapter->rtw_wdev->wiphy, bss);
#else
		cfg80211_put_bss(bss);
#endif
	}
	return;
}

/* if target wps scan ongoing, target_ssid is filled */
int rtw_cfg80211_is_target_wps_scan(struct cfg80211_scan_request *scan_req, struct cfg80211_ssid *target_ssid)
{
	int ret = 0;

	if (scan_req->n_ssids != 1
		|| scan_req->ssids[0].ssid_len == 0
		|| scan_req->n_channels != 1
	)
		goto exit;

	/* under target WPS scan */
	_rtw_memcpy(target_ssid, scan_req->ssids, sizeof(struct cfg80211_ssid));
	ret = 1;

exit:
	return ret;
}

static void _rtw_cfg80211_surveydone_event_callback(_adapter *padapter, struct cfg80211_scan_request *scan_req)
{
	struct rtw_chset *chset = adapter_to_chset(padapter);
	_list					*plist, *phead;
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	_queue				*queue	= &(pmlmepriv->scanned_queue);
	struct	wlan_network	*pnetwork = NULL;
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(padapter);
	struct cfg80211_ssid target_ssid;
	u8 target_wps_scan = 0;
	u8 band;
	u8 ch;

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO("%s\n", __func__);
#endif

	if (scan_req)
		target_wps_scan = rtw_cfg80211_is_target_wps_scan(scan_req, &target_ssid);
	else {
		_rtw_spinlock_bh(&pwdev_priv->scan_req_lock);
		if (pwdev_priv->scan_request != NULL)
			target_wps_scan = rtw_cfg80211_is_target_wps_scan(pwdev_priv->scan_request, &target_ssid);
		_rtw_spinunlock_bh(&pwdev_priv->scan_req_lock);
	}

	_rtw_spinlock_bh(&(pmlmepriv->scanned_queue.lock));

	phead = get_list_head(queue);
	plist = get_next(phead);

	while (1) {
		if (rtw_end_of_queue_search(phead, plist) == _TRUE)
			break;

		pnetwork = LIST_CONTAINOR(plist, struct wlan_network, list);
		band = pnetwork->network.Configuration.Band;
		ch = pnetwork->network.Configuration.DSConfig;

		/* report network only if the current channel set contains the channel to which this network belongs */
		if (rtw_chset_search_bch(chset, band, ch) >= 0
			&& rtw_mlme_band_check(padapter, ch) == _TRUE
			&& _TRUE == rtw_validate_ssid(&(pnetwork->network.Ssid))
			&& !rtw_chset_is_bch_non_ocp(chset, band, ch)
		) {
			if (target_wps_scan)
				rtw_cfg80211_clear_wps_sr_of_non_target_bss(padapter, pnetwork, &target_ssid);
			rtw_cfg80211_inform_bss(padapter, pnetwork);
		}

		plist = get_next(plist);
	}

	_rtw_spinunlock_bh(&(pmlmepriv->scanned_queue.lock));
}

inline void rtw_cfg80211_surveydone_event_callback(_adapter *padapter)
{
	_rtw_cfg80211_surveydone_event_callback(padapter, NULL);
}

static int rtw_cfg80211_set_probe_req_wpsp2pie(_adapter *padapter, char *buf, int len)
{
	int ret = 0;
	uint wps_ielen = 0;
	u8 *wps_ie;
	u32	p2p_ielen = 0;
	u8 *p2p_ie;
	u32	wfd_ielen = 0;
	u8 *wfd_ie;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO("%s, ielen=%d\n", __func__, len);
#endif

	if (len > 0) {
		wps_ie = rtw_get_wps_ie(buf, len, NULL, &wps_ielen);
		if (wps_ie) {
			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("probe_req_wps_ielen=%d\n", wps_ielen);
			#endif

			if (pmlmepriv->wps_probe_req_ie) {
				u32 free_len = pmlmepriv->wps_probe_req_ie_len;
				pmlmepriv->wps_probe_req_ie_len = 0;
				rtw_mfree(pmlmepriv->wps_probe_req_ie, free_len);
				pmlmepriv->wps_probe_req_ie = NULL;
			}

			pmlmepriv->wps_probe_req_ie = rtw_malloc(wps_ielen);
			if (pmlmepriv->wps_probe_req_ie == NULL) {
				RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
				return -EINVAL;

			}
			_rtw_memcpy(pmlmepriv->wps_probe_req_ie, wps_ie, wps_ielen);
			pmlmepriv->wps_probe_req_ie_len = wps_ielen;
		}

		/* buf += wps_ielen; */
		/* len -= wps_ielen; */

		#ifdef CONFIG_P2P
		p2p_ie = rtw_get_p2p_ie(buf, len, NULL, &p2p_ielen);
		if (p2p_ie) {
			struct wifidirect_info *wdinfo = &padapter->wdinfo;
			u32 attr_contentlen = 0;
			u8 listen_ch_attr[5];

			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("probe_req_p2p_ielen=%d\n", p2p_ielen);
			#endif

			if (pmlmepriv->p2p_probe_req_ie) {
				u32 free_len = pmlmepriv->p2p_probe_req_ie_len;
				pmlmepriv->p2p_probe_req_ie_len = 0;
				rtw_mfree(pmlmepriv->p2p_probe_req_ie, free_len);
				pmlmepriv->p2p_probe_req_ie = NULL;
			}

			pmlmepriv->p2p_probe_req_ie = rtw_malloc(p2p_ielen);
			if (pmlmepriv->p2p_probe_req_ie == NULL) {
				RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
				return -EINVAL;

			}
			_rtw_memcpy(pmlmepriv->p2p_probe_req_ie, p2p_ie, p2p_ielen);
			pmlmepriv->p2p_probe_req_ie_len = p2p_ielen;

			attr_contentlen = sizeof(listen_ch_attr);
			if (rtw_get_p2p_attr_content(p2p_ie, p2p_ielen, P2P_ATTR_LISTEN_CH, (u8 *)listen_ch_attr, (uint *) &attr_contentlen)) {
				if (wdinfo->listen_channel !=  listen_ch_attr[4]) {
					RTW_INFO(FUNC_ADPT_FMT" listen channel - country:%c%c%c, class:%u, ch:%u\n",
						FUNC_ADPT_ARG(padapter), listen_ch_attr[0], listen_ch_attr[1], listen_ch_attr[2],
						listen_ch_attr[3], listen_ch_attr[4]);
					wdinfo->listen_channel = listen_ch_attr[4];
				}
			}
		}
		#endif /* CONFIG_P2P */

		#ifdef CONFIG_WFD
		wfd_ie = rtw_get_wfd_ie(buf, len, NULL, &wfd_ielen);
		if (wfd_ie) {
			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("probe_req_wfd_ielen=%d\n", wfd_ielen);
			#endif

			if (rtw_mlme_update_wfd_ie_data(pmlmepriv, MLME_PROBE_REQ_IE, wfd_ie, wfd_ielen) != _SUCCESS)
				return -EINVAL;
		}
		#endif /* CONFIG_WFD */

		#ifdef CONFIG_RTW_MBO
		rtw_mbo_update_ie_data(padapter, buf, len);
		#endif
	}

	return ret;

}

#ifdef CONFIG_CONCURRENT_MODE
u8 rtw_cfg80211_scan_via_buddy(_adapter *padapter, struct cfg80211_scan_request *request)
{
	int i;
	u8 ret = _FALSE;
	_adapter *iface = NULL;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(padapter);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;

	for (i = 0; i < dvobj->iface_nums; i++) {
		struct mlme_priv *buddy_mlmepriv;
		struct rtw_wdev_priv *buddy_wdev_priv;

		iface = dvobj->padapters[i];
		if (iface == NULL)
			continue;

		if (iface == padapter)
			continue;

		if (rtw_is_adapter_up(iface) == _FALSE)
			continue;

		buddy_mlmepriv = &iface->mlmepriv;
		if (!check_fwstate(buddy_mlmepriv, WIFI_UNDER_SURVEY))
			continue;

		buddy_wdev_priv = adapter_wdev_data(iface);
		_rtw_spinlock_bh(&pwdev_priv->scan_req_lock);
		_rtw_spinlock_bh(&buddy_wdev_priv->scan_req_lock);
		if (buddy_wdev_priv->scan_request) {
			pmlmepriv->scanning_via_buddy_intf = _TRUE;
			_rtw_spinlock_bh(&pmlmepriv->lock);
			set_fwstate(pmlmepriv, WIFI_UNDER_SURVEY);
			_rtw_spinunlock_bh(&pmlmepriv->lock);
			pwdev_priv->scan_request = request;
			ret = _TRUE;
		}
		_rtw_spinunlock_bh(&buddy_wdev_priv->scan_req_lock);
		_rtw_spinunlock_bh(&pwdev_priv->scan_req_lock);

		if (ret == _TRUE)
			goto exit;
	}

exit:
	return ret;
}

void rtw_cfg80211_indicate_scan_done_for_buddy(_adapter *padapter, bool bscan_aborted)
{
	int i;
	_adapter *iface = NULL;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct mlme_priv *mlmepriv;
	struct rtw_wdev_priv *wdev_priv;
	bool indicate_buddy_scan;

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if ((iface) && rtw_is_adapter_up(iface)) {

			if (iface == padapter)
				continue;

			mlmepriv = &(iface->mlmepriv);
			wdev_priv = adapter_wdev_data(iface);

			indicate_buddy_scan = _FALSE;
			_rtw_spinlock_bh(&wdev_priv->scan_req_lock);
			if (mlmepriv->scanning_via_buddy_intf == _TRUE) {
				mlmepriv->scanning_via_buddy_intf = _FALSE;
				clr_fwstate(mlmepriv, WIFI_UNDER_SURVEY);
				if (wdev_priv->scan_request)
					indicate_buddy_scan = _TRUE;
			}
			_rtw_spinunlock_bh(&wdev_priv->scan_req_lock);

			if (indicate_buddy_scan == _TRUE) {
				rtw_cfg80211_surveydone_event_callback(iface);
				rtw_indicate_scan_done(iface, bscan_aborted);
			}

		}
	}
}
#endif /* CONFIG_CONCURRENT_MODE */

#if CONFIG_IEEE80211_BAND_6GHZ
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
int rtw_cfg80211_split_scan_6ghz(_adapter *padapter) /* second scan of two part scan */
{
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(padapter);
	struct cfg80211_scan_request *request = pwdev_priv->scan_request;
	struct cfg80211_ssid *ssids = request->ssids;
	struct rtw_chset *chset = adapter_to_chset(padapter);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct rtw_phl_rnb_rpt_element *rnb_ie;
	_queue *queue = &(pmlmepriv->scanned_queue);
	_list *plist, *phead;
	struct wlan_network *pnetwork;
	struct sitesurvey_parm *parm;
	u8 non_psc_chan[MAX_CHANNEL_NUM_6G] = {0};
	int non_psc_num = 0;
	int ret;
	int i, j;

	rnb_ie = rtw_zmalloc(sizeof(struct rtw_phl_rnb_rpt_element));
	if (rnb_ie == NULL)
		return -ENOMEM;

	parm = rtw_malloc(sizeof(struct sitesurvey_parm));
	if (parm == NULL) {
		rtw_mfree(rnb_ie, sizeof(struct rtw_phl_rnb_rpt_element));
		return -ENOMEM;
	}

	rtw_init_sitesurvey_parm(padapter, parm);
	parm->scan_6ghz_only = true;

	/* parsing request ssids, n_ssids */
	for (i = 0; i < request->n_ssids && ssids && i < RTW_SSID_SCAN_AMOUNT; i++) {
		#ifdef CONFIG_DEBUG_CFG80211
		RTW_INFO("%s: ssid=%s, len=%d\n", __func__, ssids[i].ssid, ssids[i].ssid_len);
		#endif
		_rtw_memcpy(&parm->ssid[i].Ssid, ssids[i].ssid, ssids[i].ssid_len);
		parm->ssid[i].SsidLength = ssids[i].ssid_len;
		parm->ssid_num++;
	}

	/* if no ssid entry, set the scan type as passive */
	if (request->n_ssids == 0)
		parm->scan_mode = RTW_PHL_SCAN_PASSIVE;

	/* Find non-PSC 6GHz channels found in RNR IEs from first scan */
	_rtw_spinlock_bh(&(pmlmepriv->scanned_queue.lock));
	phead = get_list_head(queue);
	plist = get_next(phead);

	while (1) {
		if (rtw_end_of_queue_search(phead, plist) == _TRUE)
			break;

		pnetwork = LIST_CONTAINOR(plist, struct wlan_network, list);
		if (rtw_parse_reduced_nb_rpt(padapter, pnetwork->network.IEs, pnetwork->network.IELength, rnb_ie)) {
			for (i = 0; i < rnb_ie->nb_ap_num; i++) {
				if ((rnb_ie->nb_aps[i].chan_def.band == BAND_ON_6G) &&
				    !Is6GHzPreferScanChannel(rnb_ie->nb_aps[i].chan_def.chan)) {
					/* skip non colocated APs */
					if (!rnb_ie->nb_aps[i].tbtt_infos[0].bss_param.colated_ap)
						continue;

					for (j = 0; j < MAX_CHANNEL_NUM_6G; j++) {
						if (non_psc_chan[j] == 0) {
							non_psc_chan[j] = rnb_ie->nb_aps[i].chan_def.chan;
							non_psc_num++;
							break;
						} else if (non_psc_chan[j] == rnb_ie->nb_aps[i].chan_def.chan) {
							break;
						}
					}
				}
			}
		}
		plist = get_next(plist);
	}
	_rtw_spinunlock_bh(&(pmlmepriv->scanned_queue.lock));

	/* Add 6GHz PSC channels and non-PSC channels found in RNR IEs above into scan list */
	for (i = 0; i < request->n_channels && i < RTW_CHANNEL_SCAN_AMOUNT; i++) {
		if (nl80211_band_to_rtw_band(request->channels[i]->band) != BAND_ON_6G)
			continue;

		if (!Is6GHzPreferScanChannel(request->channels[i]->hw_value)) {
			for (j = 0; j < non_psc_num; j++)
				if (non_psc_chan[j] == request->channels[i]->hw_value)
					break;
			if (j == non_psc_num)
				continue;
		}

		#ifdef CONFIG_DEBUG_CFG80211
		RTW_INFO(FUNC_ADPT_FMT CHAN_FMT"\n", FUNC_ADPT_ARG(padapter), CHAN_ARG(request->channels[i]));
		#endif
		parm->ch[parm->ch_num].hw_value = request->channels[i]->hw_value;
		parm->ch[parm->ch_num].flags = request->channels[i]->flags;
		parm->ch[parm->ch_num].band = BAND_ON_6G;
		parm->ch_num++;
	}

	ret = rtw_sitesurvey_cmd(padapter, parm);

	rtw_mfree(rnb_ie, sizeof(struct rtw_phl_rnb_rpt_element));
	rtw_mfree(parm, sizeof(struct sitesurvey_parm));

	return ret;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) */
#endif /* CONFIG_IEEE80211_BAND_6GHZ */

static int cfg80211_rtw_scan(struct wiphy *wiphy
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))
	, struct net_device *ndev
	#endif
	, struct cfg80211_scan_request *request)
{
	int i;
	u8 _status = _FALSE;
	int ret = 0;
	struct sitesurvey_parm *parm = NULL;
	u8 survey_times = 3;
	u8 survey_times_for_one_ch = 6;
	struct cfg80211_ssid *ssids = request->ssids;
	int social_channel = 0, j = 0;
	bool need_indicate_scan_done = _FALSE;
	bool ps_denied = _FALSE;
	u8 ssc_chk;
	_adapter *padapter;
	struct wireless_dev *wdev;
	struct rtw_wdev_priv *pwdev_priv;
	struct mlme_priv *pmlmepriv = NULL;
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo;
#endif /* CONFIG_P2P */
#if CONFIG_IEEE80211_BAND_6GHZ
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	bool scan_11ac_chan = false;
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	wdev = request->wdev;
	#if defined(RTW_DEDICATED_P2P_DEVICE)
	if (wdev == wiphy_to_pd_wdev(wiphy))
		padapter = wiphy_to_adapter(wiphy);
	else
	#endif
	if (wdev_to_ndev(wdev))
		padapter = (_adapter *)rtw_netdev_priv(wdev_to_ndev(wdev));
	else {
		ret = -EINVAL;
		goto exit;
	}
#else
	if (ndev == NULL) {
		ret = -EINVAL;
		goto exit;
	}
	padapter = (_adapter *)rtw_netdev_priv(ndev);
	wdev = ndev_to_wdev(ndev);
#endif

	pwdev_priv = adapter_wdev_data(padapter);
	pmlmepriv = &padapter->mlmepriv;
#ifdef CONFIG_P2P
	pwdinfo = &(padapter->wdinfo);
#endif /* CONFIG_P2P */

	RTW_INFO(FUNC_ADPT_FMT"%s\n", FUNC_ADPT_ARG(padapter)
		, wdev == wiphy_to_pd_wdev(wiphy) ? " PD" : "");

	if (RTW_CANNOT_RUN(adapter_to_dvobj(padapter))) {
		RTW_DBG(FUNC_ADPT_FMT "- bDriverStopped(%s) bSurpriseRemoved(%s)\n",
			FUNC_ADPT_ARG(padapter),
			dev_is_drv_stopped(adapter_to_dvobj(padapter)) ? "True" : "False",
			dev_is_surprise_removed(adapter_to_dvobj(padapter)) ? "True" : "False");
		ret = -EBUSY;
		goto exit;
	}

	if (adapter_to_pwrctl(padapter)->bInSuspend == _TRUE) {
		RTW_INFO("%s return -EBUSY since bInSuspend is TRUE\n", __func__);
		ret = -EBUSY;
		goto exit;
	}

	parm = rtw_malloc(sizeof(*parm));
	if (parm == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	rtw_init_sitesurvey_parm(padapter, parm);

#ifdef CONFIG_RTW_SCAN_RAND
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0))
	if (request->flags & NL80211_SCAN_FLAG_RANDOM_ADDR
	    && MLME_IS_STA(padapter)
	    && (check_fwstate(&padapter->mlmepriv, WIFI_ASOC_STATE) == _FALSE) ) {
		pwdev_priv->random_mac_enabled = true;
		get_random_mask_addr(pwdev_priv->pno_mac_addr, request->mac_addr,
				     request->mac_addr_mask);
		RTW_INFO("%s random mac: " MAC_FMT " \n",
			 __func__, MAC_ARG(pwdev_priv->pno_mac_addr));
	} else {
		pwdev_priv->random_mac_enabled = false;
	}
#endif /* LINUX_VERSION_CODE */
#endif /* CONFIG_RTW_SCAN_RAND */

	ssc_chk = rtw_sitesurvey_condition_check(padapter, _TRUE);

	if (ssc_chk == SS_DENY_MP_MODE)
		goto bypass_p2p_chk;
#ifdef DBG_LA_MODE
	if (ssc_chk == SS_DENY_LA_MODE)
		goto bypass_p2p_chk;
#endif

#ifdef CONFIG_P2P
	if (request->n_ssids && ssids
		&& _rtw_memcmp(ssids[0].ssid, "DIRECT-", 7)
		&& rtw_get_p2p_ie((u8 *)request->ie, request->ie_len, NULL, NULL)
	) {
		if (rtw_p2p_chk_role(pwdinfo, P2P_ROLE_DISABLE)) {
			if (!rtw_p2p_enable(padapter, P2P_ROLE_DEVICE)) {
				ret = -EOPNOTSUPP;
				goto exit;
			}
		} else {
			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("%s, role=%d\n", __func__, rtw_p2p_role(pwdinfo));
			#endif
		}

		if (request->n_channels == 3 &&
			request->channels[0]->hw_value == 1 &&
			request->channels[1]->hw_value == 6 &&
			request->channels[2]->hw_value == 11
		)
		social_channel = 1;
		parm->scan_type = RTW_SCAN_P2P;
	}
#endif /*CONFIG_P2P*/

	if (request->ie && request->ie_len > 0)
		rtw_cfg80211_set_probe_req_wpsp2pie(padapter, (u8 *)request->ie, request->ie_len);

bypass_p2p_chk:

	switch (ssc_chk) {
		case SS_ALLOW :
			break;

		case SS_DENY_MP_MODE:
			ret = -EPERM;
			goto exit;
		#ifdef DBG_LA_MODE
		case SS_DENY_LA_MODE:
			ret = -EPERM;
			goto exit;
		#endif
		case SS_DENY_BLOCK_SCAN :
		case SS_DENY_SELF_AP_UNDER_WPS :
		case SS_DENY_SELF_AP_UNDER_LINKING :
		case SS_DENY_SELF_AP_UNDER_SURVEY :
		case SS_DENY_SELF_STA_UNDER_SURVEY :
		#ifdef CONFIG_CONCURRENT_MODE
		case SS_DENY_BUDDY_UNDER_LINK_WPS :
		#endif
		case SS_DENY_BUSY_TRAFFIC :
		case SS_DENY_ADAPTIVITY:
			need_indicate_scan_done = _TRUE;
			goto check_need_indicate_scan_done;

		case SS_DENY_BY_DRV :
			#ifdef CONFIG_NOTIFY_SCAN_ABORT_WITH_BUSY
			ret = -EBUSY;
			goto exit;
			#else
			need_indicate_scan_done = _TRUE;
			goto check_need_indicate_scan_done;
			#endif
			break;

		case SS_DENY_SELF_STA_UNDER_LINKING :
			ret = -EBUSY;
			goto check_need_indicate_scan_done;

		#ifdef CONFIG_CONCURRENT_MODE
		case SS_DENY_BUDDY_UNDER_SURVEY :
			{
				bool scan_via_buddy = rtw_cfg80211_scan_via_buddy(padapter, request);

				if (scan_via_buddy == _FALSE)
					need_indicate_scan_done = _TRUE;

				goto check_need_indicate_scan_done;
			}
		#endif

		default :
			RTW_ERR("site survey check code (%d) unknown\n", ssc_chk);
			need_indicate_scan_done = _TRUE;
			goto check_need_indicate_scan_done;
	}

	/* parsing request ssids, n_ssids */
	for (i = 0; i < request->n_ssids && ssids && i < RTW_SSID_SCAN_AMOUNT; i++) {
		#ifdef CONFIG_DEBUG_CFG80211
		RTW_INFO("ssid=%s, len=%d\n", ssids[i].ssid, ssids[i].ssid_len);
		#endif
		_rtw_memcpy(&parm->ssid[i].Ssid, ssids[i].ssid, ssids[i].ssid_len);
		parm->ssid[i].SsidLength = ssids[i].ssid_len;
	}
	parm->ssid_num = i;

	/* no ssid entry, set the scan type as passive */
	if (request->n_ssids == 0)
		parm->scan_mode = RTW_PHL_SCAN_PASSIVE;

	/* parsing channels, n_channels */
	for (i = 0; i < request->n_channels && i < RTW_CHANNEL_SCAN_AMOUNT; i++) {
		#if CONFIG_IEEE80211_BAND_6GHZ
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		if (padapter->registrypriv.split_scan_6ghz) {
			/* Perform two part scan only if both 2.4G/5G and 6G channels exist*/
			if (request->channels[i]->band == NL80211_BAND_6GHZ) {
				/* if scan_11ac_chan = false, then perform scan with
				 * all 6GHz channels listed in scan request directly */
				if (scan_11ac_chan) {
					parm->pending_6ghz_scan = true;
					continue;
				}
			} else {
				scan_11ac_chan = true;
				if (parm->pending_6ghz_scan)
					RTW_WARN("%s: 6GHz chan in scan channels[] before 11ac chan !!\n", __func__);
			}
		}
		#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) */
		#endif /* CONFIG_IEEE80211_BAND_6GHZ */

		#ifdef CONFIG_DEBUG_CFG80211
		RTW_INFO(FUNC_ADPT_FMT CHAN_FMT"\n", FUNC_ADPT_ARG(padapter), CHAN_ARG(request->channels[i]));
		#endif
		parm->ch[parm->ch_num].hw_value = request->channels[i]->hw_value;
		parm->ch[parm->ch_num].flags = request->channels[i]->flags;
		parm->ch[parm->ch_num].band = nl80211_band_to_rtw_band(request->channels[i]->band);
		parm->ch_num++;
	}

	#if CONFIG_IEEE80211_BAND_6GHZ
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	parm->scan_6ghz_only = request->scan_6ghz;
	#else
	if (!scan_11ac_chan)
		parm->scan_6ghz_only = true;
	#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) */
	#endif /* CONFIG_IEEE80211_BAND_6GHZ */

	_rtw_spinlock_bh(&pwdev_priv->scan_req_lock);
	_rtw_spinlock_bh(&pmlmepriv->lock);

	_status = rtw_sitesurvey_cmd(padapter, parm);
	if (_status == _SUCCESS)
		pwdev_priv->scan_request = request;
	else
		ret = -1;
	_rtw_spinunlock_bh(&pmlmepriv->lock);
	_rtw_spinunlock_bh(&pwdev_priv->scan_req_lock);

check_need_indicate_scan_done:
	if (_TRUE == need_indicate_scan_done) {
#if (KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE)
		struct cfg80211_scan_info info;

		memset(&info, 0, sizeof(info));
		info.aborted = 0;
#endif
		/* the process time of scan results must be over at least 1ms in the newly Android */
		rtw_msleep_os(1);

		_rtw_cfg80211_surveydone_event_callback(padapter, request);
#if (KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE)
		cfg80211_scan_done(request, &info);
#else
		cfg80211_scan_done(request, 0);
#endif
	}

exit:
	if (parm)
		rtw_mfree(parm, sizeof(*parm));

#ifdef RTW_BUSY_DENY_SCAN
	if (pmlmepriv)
		pmlmepriv->lastscantime = rtw_get_current_time();
#endif

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0))
static void cfg80211_rtw_abort_scan(struct wiphy *wiphy,
				    struct wireless_dev *wdev)
{
	_adapter *padapter = wiphy_to_adapter(wiphy);

	RTW_INFO("=>"FUNC_NDEV_FMT" - Abort Scan\n", FUNC_ADPT_ARG(padapter));
	if (wdev->iftype != NL80211_IFTYPE_STATION) {
		RTW_ERR("abort scan ignored, iftype(%d)\n", wdev->iftype);
		return;
	}
	rtw_scan_abort(padapter, 0);
}
#endif /* LINUX_VERSION_CODE >= 4.5.0 */

static int cfg80211_rtw_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
#if 0
	struct iwm_priv *iwm = wiphy_to_iwm(wiphy);

	if (changed & WIPHY_PARAM_RTS_THRESHOLD &&
	    (iwm->conf.rts_threshold != wiphy->rts_threshold)) {
		int ret;

		iwm->conf.rts_threshold = wiphy->rts_threshold;

		ret = iwm_umac_set_config_fix(iwm, UMAC_PARAM_TBL_CFG_FIX,
				CFG_RTS_THRESHOLD,
				iwm->conf.rts_threshold);
		if (ret < 0)
			return ret;
	}

	if (changed & WIPHY_PARAM_FRAG_THRESHOLD &&
	    (iwm->conf.frag_threshold != wiphy->frag_threshold)) {
		int ret;

		iwm->conf.frag_threshold = wiphy->frag_threshold;

		ret = iwm_umac_set_config_fix(iwm, UMAC_PARAM_TBL_FA_CFG_FIX,
				CFG_FRAG_THRESHOLD,
				iwm->conf.frag_threshold);
		if (ret < 0)
			return ret;
	}
#endif
	RTW_INFO("%s\n", __func__);
	return 0;
}



static int rtw_cfg80211_set_wpa_version(struct security_priv *psecuritypriv, u32 wpa_version)
{
	RTW_INFO("%s, wpa_version=%d\n", __func__, wpa_version);

	if (!wpa_version) {
		psecuritypriv->ndisauthtype = Ndis802_11AuthModeOpen;
		return 0;
	}


	if (wpa_version & (NL80211_WPA_VERSION_1 | NL80211_WPA_VERSION_2))
		psecuritypriv->ndisauthtype = Ndis802_11AuthModeWPAPSK;

#if 0
	if (wpa_version & NL80211_WPA_VERSION_2)
		psecuritypriv->ndisauthtype = Ndis802_11AuthModeWPA2PSK;
#endif

	#ifdef CONFIG_WAPI_SUPPORT
	if (wpa_version & NL80211_WAPI_VERSION_1)
		psecuritypriv->ndisauthtype = Ndis802_11AuthModeWAPI;
	#endif

	return 0;

}

static int rtw_cfg80211_set_auth_type(struct security_priv *psecuritypriv,
		enum nl80211_auth_type sme_auth_type)
{
	RTW_INFO("%s, nl80211_auth_type=%d\n", __func__, sme_auth_type);

	if (NL80211_AUTHTYPE_MAX <= (int)MLME_AUTHTYPE_SAE) {
		if (MLME_AUTHTYPE_SAE == psecuritypriv->auth_type) {
			/* This case pre handle in
			 * rtw_check_connect_sae_compat()
			 */
			psecuritypriv->auth_alg = WLAN_AUTH_SAE;
			return 0;
		}
	} else if (sme_auth_type == (int)MLME_AUTHTYPE_SAE) {
		psecuritypriv->auth_type = MLME_AUTHTYPE_SAE;
		psecuritypriv->auth_alg = WLAN_AUTH_SAE;
		return 0;
	}

	psecuritypriv->auth_type = sme_auth_type;

	switch (sme_auth_type) {
	case NL80211_AUTHTYPE_AUTOMATIC:

		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Auto;

		break;
	case NL80211_AUTHTYPE_OPEN_SYSTEM:

		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Open;

		if (psecuritypriv->ndisauthtype > Ndis802_11AuthModeWPA)
			psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;

#ifdef CONFIG_WAPI_SUPPORT
		if (psecuritypriv->ndisauthtype == Ndis802_11AuthModeWAPI)
			psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_WAPI;
#endif

		break;
	case NL80211_AUTHTYPE_SHARED_KEY:

		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Shared;

		psecuritypriv->ndisencryptstatus = Ndis802_11Encryption1Enabled;


		break;
	default:
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Open;
		/* return -ENOTSUPP; */
	}

	return 0;

}

static int rtw_cfg80211_set_cipher(struct security_priv *psecuritypriv, u32 cipher, bool ucast)
{
	u32 ndisencryptstatus = Ndis802_11EncryptionDisabled;

	u32 *profile_cipher = ucast ? &psecuritypriv->dot11PrivacyAlgrthm :
		&psecuritypriv->dot118021XGrpPrivacy;

	RTW_INFO("%s, ucast=%d, cipher=0x%x\n", __func__, ucast, cipher);


	if (!cipher) {
		*profile_cipher = _NO_PRIVACY_;
		psecuritypriv->ndisencryptstatus = ndisencryptstatus;
		return 0;
	}

	switch (cipher) {
	case IW_AUTH_CIPHER_NONE:
		*profile_cipher = _NO_PRIVACY_;
		ndisencryptstatus = Ndis802_11EncryptionDisabled;
#ifdef CONFIG_WAPI_SUPPORT
		if (psecuritypriv->dot11PrivacyAlgrthm == _SMS4_)
			*profile_cipher = _SMS4_;
#endif
		break;
	case WLAN_CIPHER_SUITE_WEP40:
		*profile_cipher = _WEP40_;
		ndisencryptstatus = Ndis802_11Encryption1Enabled;
		break;
	case WLAN_CIPHER_SUITE_WEP104:
		*profile_cipher = _WEP104_;
		ndisencryptstatus = Ndis802_11Encryption1Enabled;
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		*profile_cipher = _TKIP_;
		ndisencryptstatus = Ndis802_11Encryption2Enabled;
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		*profile_cipher = _AES_;
		ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WIFI_CIPHER_SUITE_GCMP:
		*profile_cipher = _GCMP_;
		ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WIFI_CIPHER_SUITE_GCMP_256:
		*profile_cipher = _GCMP_256_;
		ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WIFI_CIPHER_SUITE_CCMP_256:
		*profile_cipher = _CCMP_256_;
		ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
#ifdef CONFIG_WAPI_SUPPORT
	case WLAN_CIPHER_SUITE_SMS4:
		*profile_cipher = _SMS4_;
		ndisencryptstatus = Ndis802_11_EncrypteionWAPI;
		break;
#endif
	default:
		RTW_INFO("Unsupported cipher: 0x%x\n", cipher);
		return -ENOTSUPP;
	}

	if (ucast) {
		psecuritypriv->ndisencryptstatus = ndisencryptstatus;

		/* if(psecuritypriv->dot11PrivacyAlgrthm >= _AES_) */
		/*	psecuritypriv->ndisauthtype = Ndis802_11AuthModeWPA2PSK; */
	}

	return 0;
}

static int rtw_cfg80211_set_key_mgt(struct security_priv *psecuritypriv, u32 key_mgt)
{
	RTW_INFO("%s, key_mgt=0x%x\n", __func__, key_mgt);

	if (key_mgt == WLAN_AKM_SUITE_8021X) {
		/* *auth_type = UMAC_AUTH_TYPE_8021X; */
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;
		psecuritypriv->rsn_akm_suite_type = 1;
	} else if (key_mgt == WLAN_AKM_SUITE_PSK) {
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;
		psecuritypriv->rsn_akm_suite_type = 2;
	} else if (key_mgt == WLAN_AKM_SUITE_SAE) {
		psecuritypriv->rsn_akm_suite_type = 8;
	}
#ifdef CONFIG_WAPI_SUPPORT
	else if (key_mgt == WLAN_AKM_SUITE_WAPI_PSK)
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_WAPI;
	else if (key_mgt == WLAN_AKM_SUITE_WAPI_CERT)
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_WAPI;
#endif
#ifdef CONFIG_RTW_80211R
	else if (key_mgt == WLAN_AKM_SUITE_FT_8021X) {
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;
		psecuritypriv->rsn_akm_suite_type = 3;
	} else if (key_mgt == WLAN_AKM_SUITE_FT_PSK) {
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;
		psecuritypriv->rsn_akm_suite_type = 4;
	}
#endif
	else {
		RTW_INFO("Invalid key mgt: 0x%x\n", key_mgt);
		/* return -EINVAL; */
	}

	return 0;
}

static int rtw_cfg80211_set_wpa_ie(_adapter *padapter, u8 *pie, size_t ielen)
{
	u8 *buf = NULL, *pos = NULL;
	int group_cipher = 0, pairwise_cipher = 0;
	u8 mfp_opt = MFP_NO;
	int ret = 0;
	int wpa_ielen = 0;
	int wpa2_ielen = 0;
	int rsnx_ielen = 0;
	u8 *pwpa, *pwpa2, *prsnx;
	u8 null_addr[] = {0, 0, 0, 0, 0, 0};

	if (pie == NULL || !ielen) {
		/* Treat this as normal case, but need to clear WIFI_UNDER_WPS */
		_clr_fwstate_(&padapter->mlmepriv, WIFI_UNDER_WPS);
		goto exit;
	}

	if (ielen > MAX_WPA_IE_LEN + MAX_WPS_IE_LEN + MAX_P2P_IE_LEN) {
		ret = -EINVAL;
		goto exit;
	}

	buf = rtw_zmalloc(ielen);
	if (buf == NULL) {
		ret =  -ENOMEM;
		goto exit;
	}

	_rtw_memcpy(buf, pie , ielen);

	RTW_INFO("set wpa_ie(length:%zu):\n", ielen);
	RTW_INFO_DUMP(NULL, buf, ielen);

	pos = buf;
	if (ielen < RSN_HEADER_LEN) {
		ret  = -1;
		goto exit;
	}

	pwpa = rtw_get_wpa_ie(buf, &wpa_ielen, ielen);
	if (pwpa && wpa_ielen > 0) {
		if (rtw_parse_wpa_ie(pwpa, wpa_ielen + 2, &group_cipher, &pairwise_cipher, NULL) == _SUCCESS) {
			padapter->securitypriv.dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;
			padapter->securitypriv.ndisauthtype = Ndis802_11AuthModeWPAPSK;
			_rtw_memcpy(padapter->securitypriv.supplicant_ie, &pwpa[0], wpa_ielen + 2);

			RTW_INFO("got wpa_ie, wpa_ielen:%u\n", wpa_ielen);
		}
	}

	pwpa2 = rtw_get_wpa2_ie(buf, &wpa2_ielen, ielen);
	if (pwpa2 && wpa2_ielen > 0) {
		if (rtw_parse_wpa2_ie(pwpa2, wpa2_ielen + 2, &group_cipher, &pairwise_cipher, NULL, NULL, &mfp_opt, NULL) == _SUCCESS) {
			padapter->securitypriv.dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;
			padapter->securitypriv.ndisauthtype = Ndis802_11AuthModeWPA2PSK;
			_rtw_memcpy(padapter->securitypriv.supplicant_ie, &pwpa2[0], wpa2_ielen + 2);

			RTW_INFO("got wpa2_ie, wpa2_ielen:%u\n", wpa2_ielen);
		}

		prsnx = rtw_get_ie(buf, WLAN_EID_RSNX, &rsnx_ielen, ielen);
		if (prsnx && (rsnx_ielen > 0)) {
			if ((rsnx_ielen + 2) <= MAX_RSNX_IE_LEN) {
				_rtw_memset(padapter->securitypriv.rsnx_ie, 0,
					MAX_RSNX_IE_LEN);
				padapter->securitypriv.rsnx_ie_len = \
					(rsnx_ielen + 2);
				_rtw_memcpy(padapter->securitypriv.rsnx_ie,
					prsnx,
					padapter->securitypriv.rsnx_ie_len);
			} else
				RTW_ERR("%s:no more buf to save RSNX Cap!\n",
					__func__);
		} else {
			_rtw_memset(padapter->securitypriv.rsnx_ie, 0,
				MAX_RSNX_IE_LEN);
			padapter->securitypriv.rsnx_ie_len = 0;
		}

	}



	if (group_cipher == 0)
		group_cipher = WPA_CIPHER_NONE;
	if (pairwise_cipher == 0)
		pairwise_cipher = WPA_CIPHER_NONE;

	switch (group_cipher) {
	case WPA_CIPHER_NONE:
		padapter->securitypriv.dot118021XGrpPrivacy = _NO_PRIVACY_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11EncryptionDisabled;
		break;
	case WPA_CIPHER_WEP40:
		padapter->securitypriv.dot118021XGrpPrivacy = _WEP40_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
		break;
	case WPA_CIPHER_TKIP:
		padapter->securitypriv.dot118021XGrpPrivacy = _TKIP_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption2Enabled;
		break;
	case WPA_CIPHER_CCMP:
		padapter->securitypriv.dot118021XGrpPrivacy = _AES_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WPA_CIPHER_GCMP:
		padapter->securitypriv.dot118021XGrpPrivacy = _GCMP_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WPA_CIPHER_GCMP_256:
		padapter->securitypriv.dot118021XGrpPrivacy = _GCMP_256_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WPA_CIPHER_CCMP_256:
		padapter->securitypriv.dot118021XGrpPrivacy = _CCMP_256_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WPA_CIPHER_WEP104:
		padapter->securitypriv.dot118021XGrpPrivacy = _WEP104_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
		break;
	}

	switch (pairwise_cipher) {
	case WPA_CIPHER_NONE:
		padapter->securitypriv.dot11PrivacyAlgrthm = _NO_PRIVACY_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11EncryptionDisabled;
		break;
	case WPA_CIPHER_WEP40:
		padapter->securitypriv.dot11PrivacyAlgrthm = _WEP40_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
		break;
	case WPA_CIPHER_TKIP:
		padapter->securitypriv.dot11PrivacyAlgrthm = _TKIP_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption2Enabled;
		break;
	case WPA_CIPHER_CCMP:
		padapter->securitypriv.dot11PrivacyAlgrthm = _AES_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WPA_CIPHER_GCMP:
		padapter->securitypriv.dot11PrivacyAlgrthm = _GCMP_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WPA_CIPHER_GCMP_256:
		padapter->securitypriv.dot11PrivacyAlgrthm = _GCMP_256_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WPA_CIPHER_CCMP_256:
		padapter->securitypriv.dot11PrivacyAlgrthm = _CCMP_256_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	case WPA_CIPHER_WEP104:
		padapter->securitypriv.dot11PrivacyAlgrthm = _WEP104_;
		padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
		break;
	}

	if (mfp_opt == MFP_INVALID) {
		RTW_INFO(FUNC_ADPT_FMT" invalid MFP setting\n", FUNC_ADPT_ARG(padapter));
		ret = -EINVAL;
		goto exit;
	}
	padapter->securitypriv.mfp_opt = mfp_opt;

	{/* handle wps_ie */
		uint wps_ielen;
		u8 *wps_ie;

		wps_ie = rtw_get_wps_ie(buf, ielen, NULL, &wps_ielen);
		if (wps_ie && wps_ielen > 0) {
			RTW_INFO("got wps_ie, wps_ielen:%u\n", wps_ielen);
			padapter->securitypriv.wps_ie_len = wps_ielen < MAX_WPS_IE_LEN ? wps_ielen : MAX_WPS_IE_LEN;
			_rtw_memcpy(padapter->securitypriv.wps_ie, wps_ie, padapter->securitypriv.wps_ie_len);
			set_fwstate(&padapter->mlmepriv, WIFI_UNDER_WPS);
		} else
			_clr_fwstate_(&padapter->mlmepriv, WIFI_UNDER_WPS);
	}

	{/* handle owe_ie */
		uint owe_ielen;
		u8 *owe_ie;

		owe_ie = rtw_get_owe_ie(buf, ielen, NULL, &owe_ielen);
		if (owe_ie && owe_ielen > 0) {
			RTW_INFO("got owe_ie, owe_ielen:%u\n", owe_ielen);
			padapter->securitypriv.owe_ie_len = owe_ielen < MAX_OWE_IE_LEN ? owe_ielen : MAX_OWE_IE_LEN;
			_rtw_memcpy(padapter->securitypriv.owe_ie, owe_ie, padapter->securitypriv.owe_ie_len);
		}
	}

	#ifdef CONFIG_P2P
	{/* check p2p_ie for assoc req; */
		uint p2p_ielen = 0;
		u8 *p2p_ie;
		struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

		p2p_ie = rtw_get_p2p_ie(buf, ielen, NULL, &p2p_ielen);
		if (p2p_ie) {
			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("%s p2p_assoc_req_ielen=%d\n", __FUNCTION__, p2p_ielen);
			#endif

			if (pmlmepriv->p2p_assoc_req_ie) {
				u32 free_len = pmlmepriv->p2p_assoc_req_ie_len;
				pmlmepriv->p2p_assoc_req_ie_len = 0;
				rtw_mfree(pmlmepriv->p2p_assoc_req_ie, free_len);
				pmlmepriv->p2p_assoc_req_ie = NULL;
			}

			pmlmepriv->p2p_assoc_req_ie = rtw_malloc(p2p_ielen);
			if (pmlmepriv->p2p_assoc_req_ie == NULL) {
				RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
				goto exit;
			}
			_rtw_memcpy(pmlmepriv->p2p_assoc_req_ie, p2p_ie, p2p_ielen);
			pmlmepriv->p2p_assoc_req_ie_len = p2p_ielen;
		}
	}
	#endif /* CONFIG_P2P */

	#ifdef CONFIG_WFD
	{
		uint wfd_ielen = 0;
		u8 *wfd_ie;
		struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

		wfd_ie = rtw_get_wfd_ie(buf, ielen, NULL, &wfd_ielen);
		if (wfd_ie) {
			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("%s wfd_assoc_req_ielen=%d\n", __FUNCTION__, wfd_ielen);
			#endif

			if (rtw_mlme_update_wfd_ie_data(pmlmepriv, MLME_ASSOC_REQ_IE, wfd_ie, wfd_ielen) != _SUCCESS)
				goto exit;
		}
	}
	#endif /* CONFIG_WFD */

	#ifdef CONFIG_RTW_MULTI_AP
	padapter->multi_ap = rtw_get_multi_ap_ie_ext(buf, ielen) & MULTI_AP_BACKHAUL_STA;
	if (padapter->multi_ap)
		adapter_set_use_wds(padapter, 1);
	#endif

exit:
	if (buf)
		rtw_mfree(buf, ielen);
	if (ret)
		_clr_fwstate_(&padapter->mlmepriv, WIFI_UNDER_WPS);

	return ret;
}

static int cfg80211_rtw_join_ibss(struct wiphy *wiphy, struct net_device *ndev,
				  struct cfg80211_ibss_params *params)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	NDIS_802_11_SSID ndis_ssid;
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	struct link_mlme_ext_priv *pmlmeext = &padapter_link->mlmeextpriv;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	struct cfg80211_chan_def *pch_def;
#endif
	struct ieee80211_channel *pch;
	int ret = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	pch_def = (struct cfg80211_chan_def *)(&params->chandef);
	pch = (struct ieee80211_channel *) pch_def->chan;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
	pch = (struct ieee80211_channel *)(params->channel);
#endif

	if (!params->ssid || !params->ssid_len) {
		ret = -EINVAL;
		goto exit;
	}

	if (params->ssid_len > IW_ESSID_MAX_SIZE) {
		ret = -E2BIG;
		goto exit;
	}

#ifdef CONFIG_CONCURRENT_MODE
	if (rtw_mi_buddy_check_fwstate(padapter, WIFI_UNDER_LINKING)) {
		RTW_INFO("%s, but buddy_intf is under linking\n", __FUNCTION__);
		ret = -EINVAL;
		goto exit;
	}
	rtw_mi_buddy_scan_abort(padapter, _TRUE); /* OR rtw_mi_scan_abort(padapter, _TRUE);*/
#endif /*CONFIG_CONCURRENT_MODE*/


	_rtw_memset(&ndis_ssid, 0, sizeof(NDIS_802_11_SSID));
	ndis_ssid.SsidLength = params->ssid_len;
	_rtw_memcpy(ndis_ssid.Ssid, (u8 *)params->ssid, params->ssid_len);

	/* RTW_INFO("ssid=%s, len=%zu\n", ndis_ssid.Ssid, params->ssid_len); */

	psecuritypriv->ndisencryptstatus = Ndis802_11EncryptionDisabled;
	psecuritypriv->dot11PrivacyAlgrthm = _NO_PRIVACY_;
	psecuritypriv->dot118021XGrpPrivacy = _NO_PRIVACY_;
	psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Open; /* open system */
	psecuritypriv->ndisauthtype = Ndis802_11AuthModeOpen;

	ret = rtw_cfg80211_set_auth_type(psecuritypriv, NL80211_AUTHTYPE_OPEN_SYSTEM);
	rtw_set_802_11_authentication_mode(padapter, psecuritypriv->ndisauthtype);

	RTW_INFO("%s: center_freq = %d\n", __func__, pch->center_freq);
	pmlmeext->chandef.chan = rtw_freq2ch(pch->center_freq);

	if (rtw_set_802_11_ssid(padapter, &ndis_ssid) == _FALSE) {
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}

static int cfg80211_rtw_leave_ibss(struct wiphy *wiphy, struct net_device *ndev)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct wireless_dev *rtw_wdev = padapter->rtw_wdev;
	enum nl80211_iftype old_type;
	int ret = 0;

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

#if (RTW_CFG80211_BLOCK_STA_DISCON_EVENT & RTW_CFG80211_BLOCK_DISCON_WHEN_DISCONNECT)
	rtw_wdev_set_not_indic_disco(adapter_wdev_data(padapter), 1);
#endif

	old_type = rtw_wdev->iftype;

	rtw_set_to_roam(padapter, 0);

	if (check_fwstate(&padapter->mlmepriv, WIFI_ASOC_STATE)) {
		rtw_scan_abort(padapter, 0);

		rtw_wdev->iftype = NL80211_IFTYPE_STATION;

		if (rtw_set_802_11_infrastructure_mode(padapter, Ndis802_11Infrastructure, 0) == _FALSE) {
			rtw_wdev->iftype = old_type;
			ret = -EPERM;
			goto leave_ibss;
		}
		rtw_setopmode_cmd(padapter, Ndis802_11Infrastructure, RTW_CMDF_WAIT_ACK);
	}

leave_ibss:
#if (RTW_CFG80211_BLOCK_STA_DISCON_EVENT & RTW_CFG80211_BLOCK_DISCON_WHEN_DISCONNECT)
	rtw_wdev_set_not_indic_disco(adapter_wdev_data(padapter), 0);
#endif

	return 0;
}

bool rtw_cfg80211_is_connect_requested(_adapter *adapter)
{
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(adapter);
	bool requested;

	_rtw_spinlock_bh(&pwdev_priv->connect_req_lock);
	requested = pwdev_priv->connect_req ? 1 : 0;
	_rtw_spinunlock_bh(&pwdev_priv->connect_req_lock);

	return requested;
}

static int _rtw_disconnect(struct wiphy *wiphy, struct net_device *ndev)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &pmlmeext->mlmext_info;
	u8 ret = _FAIL;

	/* if(check_fwstate(&padapter->mlmepriv, WIFI_ASOC_STATE)) */
	{
		rtw_scan_abort(padapter, 0);
		rtw_join_abort_timeout(padapter, 300);

		ret = rtw_disassoc_cmd(padapter, 500, RTW_CMDF_WAIT_ACK);
#ifdef CONFIG_STA_CMD_DISPR
		if (ret == _FAIL && padapter->disconnect_token)
			return 0;
#endif /* CONFIG_STA_CMD_DISPR */

		if ((MLME_IS_ASOC(padapter) == _TRUE)
#ifdef CONFIG_STA_CMD_DISPR
		    && (MLME_IS_STA(padapter) == _FALSE)
#endif /* CONFIG_STA_CMD_DISPR */
		   )
			rtw_free_assoc_resources_cmd(padapter, _TRUE, RTW_CMDF_WAIT_ACK);

		RTW_INFO("%s...call rtw_indicate_disconnect\n", __func__);
		/* indicate locally_generated = 0 when suspend */
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0))
		rtw_indicate_disconnect(padapter, 0, wiphy->dev.power.is_prepared ? _FALSE : _TRUE);
		#else
		/*
		* for kernel < 4.2, DISCONNECT event is hardcoded with
		* NL80211_ATTR_DISCONNECTED_BY_AP=1 in NL80211 layer
		* no need to judge if under suspend
		*/
		rtw_indicate_disconnect(padapter, 0, _TRUE);
		#endif

		pmlmeinfo->disconnect_occurred_time = rtw_systime_to_ms(rtw_get_current_time());
		pmlmeinfo->disconnect_code = DISCONNECTION_BY_SYSTEM_DUE_TO_HIGH_LAYER_COMMAND;
		pmlmeinfo->wifi_reason_code = WLAN_REASON_DEAUTH_LEAVING;
	}
	return 0;
}

#if (KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE) \
    && !defined(CONFIG_KERNEL_PATCH_EXTERNAL_AUTH)
static bool rtw_check_connect_sae_compat(struct cfg80211_connect_params *sme)
{
	struct rtw_ieee802_11_elems elems;
	struct rsne_info info;
	u8 AKM_SUITE_SAE[] = {0x00, 0x0f, 0xac, 8};
	int i;

	if (sme->auth_type != (int)MLME_AUTHTYPE_SHARED_KEY)
		return false;

	if (rtw_ieee802_11_parse_elems((u8 *)sme->ie, sme->ie_len, &elems, 0)
	    == ParseFailed)
		return false;

	if (!elems.rsn_ie)
		return false;

	if (rtw_rsne_info_parse(elems.rsn_ie - 2, elems.rsn_ie_len + 2, &info) == _FAIL)
		return false;

	for (i = 0; i < info.akm_cnt; i++)
		if (_rtw_memcmp(info.akm_list + i * RSN_SELECTOR_LEN,
				AKM_SUITE_SAE, RSN_SELECTOR_LEN) == _TRUE)
			return true;

	return false;
}
#else
#define rtw_check_connect_sae_compat(sme)	false
#endif

/* todo: move to rtw_security.c ? */
static int rtw_set_security(struct _ADAPTER *a,
			    struct cfg80211_connect_params *sme)
{
	struct security_priv *sec = &a->securitypriv;
	int ret = 0;


	sec->ndisencryptstatus = Ndis802_11EncryptionDisabled;
	sec->dot11PrivacyAlgrthm = _NO_PRIVACY_;
	sec->dot118021XGrpPrivacy = _NO_PRIVACY_;
	sec->dot11AuthAlgrthm = dot11AuthAlgrthm_Open; /* open system */
	sec->ndisauthtype = Ndis802_11AuthModeOpen;
	sec->auth_alg = WLAN_AUTH_OPEN;
	sec->extauth_status = WLAN_STATUS_UNSPECIFIED_FAILURE;

	ret = rtw_cfg80211_set_wpa_version(sec, sme->crypto.wpa_versions);
	if (ret < 0)
		return -1;

#ifdef CONFIG_WAPI_SUPPORT
	if (sme->crypto.wpa_versions & NL80211_WAPI_VERSION_1) {
		a->wapiInfo.bWapiEnable = true;
		a->wapiInfo.extra_prefix_len = WAPI_EXT_LEN;
		a->wapiInfo.extra_postfix_len = SMS4_MIC_LEN;
	} else {
		a->wapiInfo.bWapiEnable = false;
	}
#endif

	ret = rtw_cfg80211_set_auth_type(sec, sme->auth_type);

#ifdef CONFIG_WAPI_SUPPORT
	if (sec->dot11AuthAlgrthm == dot11AuthAlgrthm_WAPI)
		a->mlmeextpriv.mlmext_info.auth_algo = sec->dot11AuthAlgrthm;
#endif
	if (ret < 0)
		return -1;

	if (sme->crypto.n_ciphers_pairwise) {
		ret = rtw_cfg80211_set_cipher(sec, sme->crypto.ciphers_pairwise[0], _TRUE);
		if (ret < 0)
			return -1;
	}

	/* For WEP Shared auth */
	if (sme->key_len > 0 && sme->key) {
		u32 wep_key_idx, wep_key_len, wep_total_len;
		NDIS_802_11_WEP	*pwep = NULL;
		RTW_INFO("%s(): Shared/Auto WEP\n", __FUNCTION__);

		wep_key_idx = sme->key_idx;
		wep_key_len = sme->key_len;

		if (sme->key_idx > WEP_KEYS)
			return -EINVAL;

		if (!wep_key_len)
			return -EINVAL;
		wep_key_len = wep_key_len <= 5 ? 5 : 13;
		wep_total_len = wep_key_len + FIELD_OFFSET(NDIS_802_11_WEP, KeyMaterial);
		pwep = (NDIS_802_11_WEP *) rtw_malloc(wep_total_len);
		if (pwep == NULL) {
			RTW_INFO(" wpa_set_encryption: pwep allocate fail !!!\n");
			return -ENOMEM;
		}

		_rtw_memset(pwep, 0, wep_total_len);

		pwep->KeyLength = wep_key_len;
		pwep->Length = wep_total_len;

		if (wep_key_len == 13) {
			a->securitypriv.dot11PrivacyAlgrthm = _WEP104_;
			a->securitypriv.dot118021XGrpPrivacy = _WEP104_;
		}

		pwep->KeyIndex = wep_key_idx;
		pwep->KeyIndex |= 0x80000000;

		_rtw_memcpy(pwep->KeyMaterial, (void *)sme->key, pwep->KeyLength);

		if (rtw_set_802_11_add_wep(a, pwep) == (u8)_FAIL)
			ret = -EOPNOTSUPP ;

		if (pwep)
			rtw_mfree((u8 *)pwep, wep_total_len);

		if (ret < 0)
			return ret;
	}

	ret = rtw_cfg80211_set_cipher(sec, sme->crypto.cipher_group, _FALSE);
	if (ret < 0)
		return ret;

	if (sme->crypto.n_akm_suites) {
		ret = rtw_cfg80211_set_key_mgt(sec, sme->crypto.akm_suites[0]);
		if (ret < 0)
			return ret;
	}
#ifdef CONFIG_8011R
	else {
		/*It could be a connection without RSN IEs*/
		sec->rsn_akm_suite_type = 0;
	}
#endif

#ifdef CONFIG_WAPI_SUPPORT
	if (sme->crypto.akm_suites[0] == WLAN_AKM_SUITE_WAPI_PSK)
		a->wapiInfo.bWapiPSK = true;
	else if (sme->crypto.akm_suites[0] == WLAN_AKM_SUITE_WAPI_CERT)
		a->wapiInfo.bWapiPSK = false;
#endif

	rtw_set_802_11_authentication_mode(a, sec->ndisauthtype);

	/* rtw_set_802_11_encryption_mode(a, a->securitypriv.ndisencryptstatus); */

	return 0;
}

static int rtw_set_wpa_ie(struct _ADAPTER *a,
			  struct cfg80211_connect_params *sme)
{
	return rtw_cfg80211_set_wpa_ie(a, (u8 *)sme->ie, sme->ie_len);
}

static int cfg80211_rtw_connect(struct wiphy *wiphy, struct net_device *ndev,
				struct cfg80211_connect_params *sme)
{
	int ret = 0;
	const u8 *bssid = NULL;
	NDIS_802_11_SSID ndis_ssid;
	/* u8 matched_by_bssid=_FALSE; */
	/* u8 matched_by_ssid=_FALSE; */
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(padapter);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	u16 ch_hw_vlue = 0;
	enum band_type ch_band = BAND_MAX;

#if (RTW_CFG80211_BLOCK_STA_DISCON_EVENT & RTW_CFG80211_BLOCK_DISCON_WHEN_CONNECT)
	rtw_wdev_set_not_indic_disco(pwdev_priv, 1);
#endif

	RTW_INFO("=>"FUNC_NDEV_FMT" - Start to Connection\n", FUNC_NDEV_ARG(ndev));
	RTW_INFO("privacy=%d, key=%p, key_len=%d, key_idx=%d, auth_type=%d\n",
		sme->privacy, sme->key, sme->key_len, sme->key_idx, sme->auth_type);

	if (rtw_check_connect_sae_compat(sme)) {

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
		sme->auth_type = NL80211_AUTHTYPE_SAE;
#else /* LINUX_VERSION_CODE < 3.8 */
		/*
		 * When the linux kernel version is less than v3.8, there is no
		 * NL80211_AUTHTYPE_SAE, also the driver checks psecuritypriv->auth_type
		 * in rtw_cfg80211_set_auth_type() instead of sme's auth_type in this
		 * case already. So NL80211_AUTHTYPE_AUTOMATIC is set here when there
		 * is no NL80211_AUTHTYPE_SAE in this case.
		 *
		 * PS: NL80211_AUTHTYPE_AUTOMATIC is NL80211_AUTHTYPE_MAX + 1
		 *     So it won't conflicts with other defined type in this case.
		 *
		 * PS: The driver supports from Linux Kernel 2.6.35. and there is
		 *     NL80211_AUTHTYPE_MAX / NL80211_AUTHTYPE_AUTOMATIC defined
		 *     in linux kernel 2.6.35 already.
		 */
		sme->auth_type = NL80211_AUTHTYPE_AUTOMATIC;
#endif /* LINUX_VERSION_CODE */

		psecuritypriv->auth_type = MLME_AUTHTYPE_SAE;
		psecuritypriv->auth_alg = WLAN_AUTH_SAE;
		RTW_INFO("%s set sme->auth_type for SAE compat\n", __FUNCTION__);
	}

	if (pwdev_priv->block == _TRUE) {
		ret = -EBUSY;
		RTW_INFO("%s wdev_priv.block is set\n", __FUNCTION__);
		goto exit;
	}

       if (check_fwstate(pmlmepriv, WIFI_ASOC_STATE | WIFI_UNDER_LINKING) == _TRUE) {

		_rtw_disconnect(wiphy, ndev);
		RTW_INFO("%s disconnect before connecting! fw_state=0x%x\n",
			__FUNCTION__, pmlmepriv->fw_state);
	}

	if (!sme->ssid || !sme->ssid_len) {
		ret = -EINVAL;
		goto exit;
	}

	if (sme->ssid_len > IW_ESSID_MAX_SIZE) {
		ret = -E2BIG;
		goto exit;
	}

	rtw_mi_scan_abort(padapter, _TRUE);

	rtw_join_abort_timeout(padapter, 300);
#ifdef CONFIG_CONCURRENT_MODE
	if (rtw_mi_buddy_check_fwstate(padapter, WIFI_UNDER_LINKING)) {
		ret = -EINVAL;
		goto exit;
	}
#endif

	_rtw_memset(&ndis_ssid, 0, sizeof(NDIS_802_11_SSID));
	ndis_ssid.SsidLength = sme->ssid_len;
	_rtw_memcpy(ndis_ssid.Ssid, (u8 *)sme->ssid, sme->ssid_len);

	RTW_INFO("ssid=%s, len=%zu\n", ndis_ssid.Ssid, sme->ssid_len);


	if (sme->bssid) {
		RTW_INFO("bssid="MAC_FMT"\n", MAC_ARG(sme->bssid));
		bssid = sme->bssid;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0))
	else if (sme->bssid_hint) {
		RTW_INFO("bssid_hint="MAC_FMT"\n", MAC_ARG(sme->bssid_hint));
		bssid = sme->bssid_hint;
	}
#endif

	ret = rtw_set_security(padapter, sme);
	if (ret < 0)
		goto exit;

	if (!is_wep_enc(psecuritypriv->dot11PrivacyAlgrthm) && (psecuritypriv->dot11PrivacyAlgrthm != _SMS4_)) {
		ret = rtw_set_wpa_ie(padapter, sme);
		if (ret < 0)
			goto exit;
	}
#ifdef CONFIG_RTW_MBO
	rtw_mbo_update_ie_data(padapter, (u8 *)sme->ie, sme->ie_len);
#endif

	if (sme->channel) {
		ch_hw_vlue = sme->channel->hw_value;
		ch_band = nl80211_band_to_rtw_band(sme->channel->band);
	}

	if (rtw_set_802_11_connect(padapter, bssid, &ndis_ssid,
				   ch_hw_vlue, ch_band) == _FALSE) {
		ret = -1;
		goto exit;
	}


	_rtw_spinlock_bh(&pwdev_priv->connect_req_lock);

	if (pwdev_priv->connect_req) {
		rtw_wdev_free_connect_req(pwdev_priv);
		RTW_INFO(FUNC_NDEV_FMT" free existing connect_req\n", FUNC_NDEV_ARG(ndev));
	}

	pwdev_priv->connect_req = (struct cfg80211_connect_params *)rtw_malloc(sizeof(*pwdev_priv->connect_req));
	if (pwdev_priv->connect_req)
		_rtw_memcpy(pwdev_priv->connect_req, sme, sizeof(*pwdev_priv->connect_req));
	else
		RTW_WARN(FUNC_NDEV_FMT" alloc connect_req fail\n", FUNC_NDEV_ARG(ndev));

	_rtw_spinunlock_bh(&pwdev_priv->connect_req_lock);

	RTW_INFO("set ssid:dot11AuthAlgrthm=%d, dot11PrivacyAlgrthm=%d, dot118021XGrpPrivacy=%d\n", psecuritypriv->dot11AuthAlgrthm, psecuritypriv->dot11PrivacyAlgrthm,
		psecuritypriv->dot118021XGrpPrivacy);

exit:
	RTW_INFO("<=%s, ret %d\n", __FUNCTION__, ret);

#if (RTW_CFG80211_BLOCK_STA_DISCON_EVENT & RTW_CFG80211_BLOCK_DISCON_WHEN_CONNECT)
	rtw_wdev_set_not_indic_disco(pwdev_priv, 0);
#endif

	return ret;
}

static int cfg80211_rtw_disconnect(struct wiphy *wiphy, struct net_device *ndev,
				   u16 reason_code)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);

	RTW_INFO(FUNC_NDEV_FMT" - Start to Disconnect\n", FUNC_NDEV_ARG(ndev));

#if (RTW_CFG80211_BLOCK_STA_DISCON_EVENT & RTW_CFG80211_BLOCK_DISCON_WHEN_DISCONNECT)
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	if (!wiphy->dev.power.is_prepared)
	#endif
		rtw_wdev_set_not_indic_disco(adapter_wdev_data(padapter), 1);
#endif

	rtw_set_to_roam(padapter, 0);

	/* if(check_fwstate(&padapter->mlmepriv, WIFI_ASOC_STATE)) */
	{
		_rtw_disconnect(wiphy, ndev);
	}

#if (RTW_CFG80211_BLOCK_STA_DISCON_EVENT & RTW_CFG80211_BLOCK_DISCON_WHEN_DISCONNECT)
	rtw_wdev_set_not_indic_disco(adapter_wdev_data(padapter), 0);
#endif

	RTW_INFO(FUNC_NDEV_FMT" return 0\n", FUNC_NDEV_ARG(ndev));
	return 0;
}

static int cfg80211_rtw_set_txpower(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	struct wireless_dev *wdev,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)) || defined(COMPAT_KERNEL_RELEASE)
	enum nl80211_tx_power_setting type, int mbm)
#else
	enum tx_power_setting type, int dbm)
#endif
{
#if 0
	struct iwm_priv *iwm = wiphy_to_iwm(wiphy);
	int ret;

	switch (type) {
	case NL80211_TX_POWER_AUTOMATIC:
		return 0;
	case NL80211_TX_POWER_FIXED:
		if (mbm < 0 || (mbm % 100))
			return -EOPNOTSUPP;

		if (!test_bit(IWM_STATUS_READY, &iwm->status))
			return 0;

		ret = iwm_umac_set_config_fix(iwm, UMAC_PARAM_TBL_CFG_FIX,
					      CFG_TX_PWR_LIMIT_USR,
					      MBM_TO_DBM(mbm) * 2);
		if (ret < 0)
			return ret;

		return iwm_tx_power_trigger(iwm);
	default:
		IWM_ERR(iwm, "Unsupported power type: %d\n", type);
		return -EOPNOTSUPP;
	}
#endif
	RTW_INFO("%s\n", __func__);
	return 0;
}

static int cfg80211_rtw_get_txpower(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	struct wireless_dev *wdev,
#endif
	int *dbm)
{
	RTW_INFO("%s\n", __func__);

	*dbm = (12);

	return 0;
}

inline bool rtw_cfg80211_pwr_mgmt(_adapter *adapter)
{
	struct rtw_wdev_priv *rtw_wdev_priv = adapter_wdev_data(adapter);
	return rtw_wdev_priv->power_mgmt;
}

static int cfg80211_rtw_set_power_mgmt(struct wiphy *wiphy,
				       struct net_device *ndev,
				       bool enabled, int timeout)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct rtw_wdev_priv *rtw_wdev_priv = adapter_wdev_data(padapter);
#ifdef CONFIG_POWER_SAVE
#ifdef CONFIG_RTW_LPS
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_phl_com_t *phl_com = dvobj->phl_com;
	struct rtw_ps_cap_t *ps_cap_p = &phl_com->dev_cap.ps_cap;
#endif /* CONFIG_RTW_LPS */
#endif /* CONFIG_POWER_SAVE */

	RTW_INFO(FUNC_NDEV_FMT" enabled:%u, timeout:%d\n", FUNC_NDEV_ARG(ndev),
		enabled, timeout);

	rtw_wdev_priv->power_mgmt = enabled;

#ifdef CONFIG_POWER_SAVE
#ifdef CONFIG_RTW_LPS
	if (enabled) {
		if (ps_cap_p->lps_en == PS_OP_MODE_DISABLED)
			rtw_phl_dbg_ps_op_mode(GET_PHL_INFO(dvobj), HW_BAND_0, PS_MODE_LPS, PS_OP_MODE_AUTO);
	} else {
		if (ps_cap_p->lps_en != PS_OP_MODE_DISABLED)
			rtw_phl_dbg_ps_op_mode(GET_PHL_INFO(dvobj), HW_BAND_0, PS_MODE_LPS, PS_OP_MODE_DISABLED);
	}
#endif /* CONFIG_RTW_LPS */
#endif /* CONFIG_POWER_SAVE */

	return 0;
}

static void _rtw_set_pmksa(struct net_device *ndev,
	u8 *bssid, u8 *pmkid)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	u8 index, blInserted = _FALSE;

	/* overwrite PMKID */
	for (index = 0 ; index < NUM_PMKID_CACHE; index++) {
		if (_rtw_memcmp(psecuritypriv->PMKIDList[index].Bssid, bssid, ETH_ALEN) == _TRUE) {
			/* BSSID is matched, the same AP => rewrite with new PMKID. */
			RTW_INFO("BSSID("MAC_FMT") exists in the PMKList.\n", MAC_ARG(bssid));

			_rtw_memcpy(psecuritypriv->PMKIDList[index].PMKID, pmkid, WLAN_PMKID_LEN);
			psecuritypriv->PMKIDList[index].bUsed = _TRUE;
			blInserted = _TRUE;
			break;
		}
	}

	if (!blInserted) {
		/* Find a new entry */
		RTW_INFO("Use the new entry index = %d for this PMKID.\n",
			psecuritypriv->PMKIDIndex);

		_rtw_memcpy(psecuritypriv->PMKIDList[psecuritypriv->PMKIDIndex].Bssid, bssid, ETH_ALEN);
		_rtw_memcpy(psecuritypriv->PMKIDList[psecuritypriv->PMKIDIndex].PMKID, pmkid, WLAN_PMKID_LEN);

		psecuritypriv->PMKIDList[psecuritypriv->PMKIDIndex].bUsed = _TRUE;
		psecuritypriv->PMKIDIndex++ ;
		if (psecuritypriv->PMKIDIndex == 16)
			psecuritypriv->PMKIDIndex = 0;
	}
}

static int cfg80211_rtw_set_pmksa(struct wiphy *wiphy,
				  struct net_device *ndev,
				  struct cfg80211_pmksa *pmksa)
{
	u8	index, blInserted = _FALSE;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct mlme_priv *mlme = &padapter->mlmepriv;
	struct security_priv	*psecuritypriv = &padapter->securitypriv;
	u8	strZeroMacAddress[ETH_ALEN] = { 0x00 };
	bool sae_auth = rtw_sec_chk_auth_type(padapter, MLME_AUTHTYPE_SAE);

	RTW_INFO(FUNC_NDEV_FMT" "MAC_FMT" "KEY_FMT"\n", FUNC_NDEV_ARG(ndev)
		, MAC_ARG(pmksa->bssid), KEY_ARG(pmksa->pmkid));

	if (_rtw_memcmp((u8 *)pmksa->bssid, strZeroMacAddress, ETH_ALEN) == _TRUE)
		return -EINVAL;

	_rtw_set_pmksa(ndev, (u8 *)pmksa->bssid, (u8 *)pmksa->pmkid);

	if (sae_auth &&
		(psecuritypriv->extauth_status == WLAN_STATUS_SUCCESS)) {
		RTW_PRINT("SAE: auth success, start assoc\n");
		start_clnt_assoc(padapter);
	}

	return 0;
}

static int cfg80211_rtw_del_pmksa(struct wiphy *wiphy,
				  struct net_device *ndev,
				  struct cfg80211_pmksa *pmksa)
{
	u8	index, bMatched = _FALSE;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct security_priv	*psecuritypriv = &padapter->securitypriv;

	RTW_INFO(FUNC_NDEV_FMT" "MAC_FMT" "KEY_FMT"\n", FUNC_NDEV_ARG(ndev)
		, MAC_ARG(pmksa->bssid), KEY_ARG(pmksa->pmkid));

	for (index = 0 ; index < NUM_PMKID_CACHE; index++) {
		if (_rtw_memcmp(psecuritypriv->PMKIDList[index].Bssid, (u8 *)pmksa->bssid, ETH_ALEN) == _TRUE) {
			/* BSSID is matched, the same AP => Remove this PMKID information and reset it. */
			_rtw_memset(psecuritypriv->PMKIDList[index].Bssid, 0x00, ETH_ALEN);
			_rtw_memset(psecuritypriv->PMKIDList[index].PMKID, 0x00, WLAN_PMKID_LEN);
			psecuritypriv->PMKIDList[index].bUsed = _FALSE;
			bMatched = _TRUE;
			RTW_INFO(FUNC_NDEV_FMT" clear id:%hhu\n", FUNC_NDEV_ARG(ndev), index);
			break;
		}
	}

	if (_FALSE == bMatched) {
		RTW_INFO(FUNC_NDEV_FMT" do not have matched BSSID\n"
			, FUNC_NDEV_ARG(ndev));
		return -EINVAL;
	}

	return 0;
}

static int cfg80211_rtw_flush_pmksa(struct wiphy *wiphy,
				    struct net_device *ndev)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct security_priv	*psecuritypriv = &padapter->securitypriv;

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	_rtw_memset(&psecuritypriv->PMKIDList[0], 0x00, sizeof(RT_PMKID_LIST) * NUM_PMKID_CACHE);
	psecuritypriv->PMKIDIndex = 0;

	return 0;
}

#ifdef CONFIG_AP_MODE
void rtw_cfg80211_indicate_sta_assoc(_adapter *padapter, u8 *pmgmt_frame, uint frame_len)
{
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
#if !defined(RTW_USE_CFG80211_STA_EVENT) && !defined(COMPAT_KERNEL_RELEASE)
	s32 freq;
	int channel;
	struct wireless_dev *pwdev = padapter->rtw_wdev;
	struct link_mlme_ext_priv *pmlmeext = &(padapter_link->mlmeextpriv);
#endif
	struct net_device *ndev = padapter->pnetdev;

	RTW_INFO(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(padapter));

#if defined(RTW_USE_CFG80211_STA_EVENT) || defined(COMPAT_KERNEL_RELEASE)
	{
		struct station_info sinfo;
		u8 ie_offset;
		if (get_frame_sub_type(pmgmt_frame) == WIFI_ASSOCREQ)
			ie_offset = _ASOCREQ_IE_OFFSET_;
		else /* WIFI_REASSOCREQ */
			ie_offset = _REASOCREQ_IE_OFFSET_;

		memset(&sinfo, 0, sizeof(sinfo));
		sinfo.filled = STATION_INFO_ASSOC_REQ_IES;
		sinfo.assoc_req_ies = pmgmt_frame + WLAN_HDR_A3_LEN + ie_offset;
		sinfo.assoc_req_ies_len = frame_len - WLAN_HDR_A3_LEN - ie_offset;
		cfg80211_new_sta(ndev, get_addr2_ptr(pmgmt_frame), &sinfo, GFP_ATOMIC);
	}
#else /* defined(RTW_USE_CFG80211_STA_EVENT) */
	channel = pmlmeext->chandef.chan;
	freq = rtw_ch2freq(channel);

	#ifdef COMPAT_KERNEL_RELEASE
	rtw_cfg80211_rx_mgmt(pwdev, freq, 0, pmgmt_frame, frame_len, GFP_ATOMIC);
	#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && !defined(CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER)
	rtw_cfg80211_rx_mgmt(pwdev, freq, 0, pmgmt_frame, frame_len, GFP_ATOMIC);
	#else /* COMPAT_KERNEL_RELEASE */
	{
		/* to avoid WARN_ON(wdev->iftype != NL80211_IFTYPE_STATION)  when calling cfg80211_send_rx_assoc() */
		#ifndef CONFIG_PLATFORM_MSTAR
		pwdev->iftype = NL80211_IFTYPE_STATION;
		#endif /* CONFIG_PLATFORM_MSTAR */
		RTW_INFO("iftype=%d before call cfg80211_send_rx_assoc()\n", pwdev->iftype);
		rtw_cfg80211_send_rx_assoc(padapter, NULL, pmgmt_frame, frame_len);
		RTW_INFO("iftype=%d after call cfg80211_send_rx_assoc()\n", pwdev->iftype);
		pwdev->iftype = NL80211_IFTYPE_AP;
		/* cfg80211_rx_action(padapter->pnetdev, freq, pmgmt_frame, frame_len, GFP_ATOMIC); */
	}
	#endif /* COMPAT_KERNEL_RELEASE */
#endif /* defined(RTW_USE_CFG80211_STA_EVENT) */

}

void rtw_cfg80211_indicate_sta_disassoc(_adapter *padapter, const u8 *da, unsigned short reason)
{
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
#if !defined(RTW_USE_CFG80211_STA_EVENT) && !defined(COMPAT_KERNEL_RELEASE)
	s32 freq;
	int channel;
	u8 *pmgmt_frame;
	uint frame_len;
	struct rtw_ieee80211_hdr *pwlanhdr;
	unsigned short *fctrl;
	u8 mgmt_buf[128] = {0};
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct wireless_dev *wdev = padapter->rtw_wdev;
#endif
	struct net_device *ndev = padapter->pnetdev;

	RTW_INFO(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(padapter));

#if defined(RTW_USE_CFG80211_STA_EVENT) || defined(COMPAT_KERNEL_RELEASE)
	cfg80211_del_sta(ndev, da, GFP_ATOMIC);
#else /* defined(RTW_USE_CFG80211_STA_EVENT) */
	channel = padapter_link->mlmeext.chandef.chan;
	freq = rtw_ch2freq(channel);

	pmgmt_frame = mgmt_buf;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pmgmt_frame;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, adapter_mac_addr(padapter), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, da, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->dev_network)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	set_frame_sub_type(pmgmt_frame, WIFI_DEAUTH);

	pmgmt_frame += sizeof(struct rtw_ieee80211_hdr_3addr);
	frame_len = sizeof(struct rtw_ieee80211_hdr_3addr);

	reason = cpu_to_le16(reason);
	pmgmt_frame = rtw_set_fixed_ie(pmgmt_frame, _RSON_CODE_ , (unsigned char *)&reason, &frame_len);

	#ifdef COMPAT_KERNEL_RELEASE
	rtw_cfg80211_rx_mgmt(wdev, freq, 0, mgmt_buf, frame_len, GFP_ATOMIC);
	#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && !defined(CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER)
	rtw_cfg80211_rx_mgmt(wdev, freq, 0, mgmt_buf, frame_len, GFP_ATOMIC);
	#else /* COMPAT_KERNEL_RELEASE */
	cfg80211_send_disassoc(padapter->pnetdev, mgmt_buf, frame_len);
	/* cfg80211_rx_action(padapter->pnetdev, freq, mgmt_buf, frame_len, GFP_ATOMIC); */
	#endif /* COMPAT_KERNEL_RELEASE */
#endif /* defined(RTW_USE_CFG80211_STA_EVENT) */
}

static int rtw_cfg80211_monitor_if_open(struct net_device *ndev)
{
	int ret = 0;

	RTW_INFO("%s\n", __func__);

	return ret;
}

static int rtw_cfg80211_monitor_if_close(struct net_device *ndev)
{
	int ret = 0;

	RTW_INFO("%s\n", __func__);

	return ret;
}

static int rtw_cfg80211_monitor_if_xmit_entry(struct sk_buff *skb, struct net_device *ndev)
{
	int ret = 0;
	int rtap_len;
	int qos_len = 0;
	int dot11_hdr_len = 24;
	int snap_len = 6;
	unsigned char *pdata;
	u16 frame_ctl;
	unsigned char src_mac_addr[ETH_ALEN];
	unsigned char dst_mac_addr[ETH_ALEN];
	struct rtw_ieee80211_hdr *dot11_hdr;
	struct ieee80211_radiotap_header *rtap_hdr;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct rf_ctl_t *rfctl = adapter_to_rfctl(padapter);
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	rtw_mstat_update(MSTAT_TYPE_SKB, MSTAT_ALLOC_SUCCESS, skb->truesize);

	if (alink_is_tx_blocked_by_ch_waiting(padapter_link))
		goto fail;

	if (unlikely(skb->len < sizeof(struct ieee80211_radiotap_header)))
		goto fail;

	rtap_hdr = (struct ieee80211_radiotap_header *)skb->data;
	if (unlikely(rtap_hdr->it_version))
		goto fail;

	rtap_len = ieee80211_get_radiotap_len(skb->data);
	if (unlikely(skb->len < rtap_len))
		goto fail;

	if (rtap_len != 14) {
		RTW_INFO("radiotap len (should be 14): %d\n", rtap_len);
		goto fail;
	}

	/* Skip the ratio tap header */
	skb_pull(skb, rtap_len);

	dot11_hdr = (struct rtw_ieee80211_hdr *)skb->data;
	frame_ctl = le16_to_cpu(dot11_hdr->frame_ctl);
	/* Check if the QoS bit is set */
	if ((frame_ctl & RTW_IEEE80211_FCTL_FTYPE) == RTW_IEEE80211_FTYPE_DATA) {
		/* Check if this ia a Wireless Distribution System (WDS) frame
		 * which has 4 MAC addresses
		 */
		if (dot11_hdr->frame_ctl & 0x0080)
			qos_len = 2;
		if ((dot11_hdr->frame_ctl & 0x0300) == 0x0300)
			dot11_hdr_len += 6;

		_rtw_memcpy(dst_mac_addr, dot11_hdr->addr1, sizeof(dst_mac_addr));
		_rtw_memcpy(src_mac_addr, dot11_hdr->addr2, sizeof(src_mac_addr));

		/* Skip the 802.11 header, QoS (if any) and SNAP, but leave spaces for
		 * for two MAC addresses
		 */
		skb_pull(skb, dot11_hdr_len + qos_len + snap_len - sizeof(src_mac_addr) * 2);
		pdata = (unsigned char *)skb->data;
		_rtw_memcpy(pdata, dst_mac_addr, sizeof(dst_mac_addr));
		_rtw_memcpy(pdata + sizeof(dst_mac_addr), src_mac_addr, sizeof(src_mac_addr));

		RTW_INFO("should be eapol packet\n");

		/* Use the real net device to transmit the packet */
		ret = _rtw_xmit_entry(skb, padapter->pnetdev);

		return ret;

	} else if ((frame_ctl & (RTW_IEEE80211_FCTL_FTYPE | RTW_IEEE80211_FCTL_STYPE))
		== (RTW_IEEE80211_FTYPE_MGMT | RTW_IEEE80211_STYPE_ACTION)
	) {
		/* only for action frames */
		struct xmit_frame		*pmgntframe;
		struct pkt_attrib	*pattrib;
		unsigned char	*pframe;
		/* u8 category, action, OUI_Subtype, dialogToken=0; */
		/* unsigned char	*frame_body; */
		struct rtw_ieee80211_hdr *pwlanhdr;
		struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
		struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
		u8 *buf = skb->data;
		u32 len = skb->len;
		u8 category, action;
		int type = -1;

		if (rtw_action_frame_parse(buf, len, &category, &action) == _FALSE) {
			RTW_INFO(FUNC_NDEV_FMT" frame_control:0x%x\n", FUNC_NDEV_ARG(ndev),
				le16_to_cpu(((struct rtw_ieee80211_hdr_3addr *)buf)->frame_ctl));
			goto fail;
		}

		RTW_INFO("RTW_Tx:da="MAC_FMT" via "FUNC_NDEV_FMT"\n",
			MAC_ARG(GetAddr1Ptr(buf)), FUNC_NDEV_ARG(ndev));
		#ifdef CONFIG_P2P
		type = rtw_p2p_check_frames(padapter, buf, len, _TRUE);
		if (type >= 0)
			goto dump;
		#endif
		if (category == RTW_WLAN_CATEGORY_PUBLIC)
			RTW_INFO("RTW_Tx:%s\n", action_public_str(action));
		else
			RTW_INFO("RTW_Tx:category(%u), action(%u)\n", category, action);
#ifdef CONFIG_P2P
dump:
#endif
		/* starting alloc mgmt frame to dump it */
		pmgntframe = alloc_mgtxmitframe(pxmitpriv);
		if (pmgntframe == NULL)
			goto fail;

		/* update attribute */
		pattrib = &pmgntframe->attrib;
		update_mgntframe_attrib(padapter, padapter_link, pattrib);
		pattrib->retry_ctrl = _FALSE;

		_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

		pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;

		_rtw_memcpy(pframe, (void *)buf, len);
		pattrib->pktlen = len;

#ifdef CONFIG_P2P
		if (type >= 0)
			rtw_xframe_chk_wfd_ie(pmgntframe);
#endif /* CONFIG_P2P */

		pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;
		/* update seq number */
		pmlmeext->mgnt_seq = GetSequence(pwlanhdr);
		pattrib->seqnum = pmlmeext->mgnt_seq;
		pmlmeext->mgnt_seq++;


		pattrib->last_txcmdsz = pattrib->pktlen;

		dump_mgntframe(padapter, pmgntframe);

	} else
		RTW_INFO("frame_ctl=0x%x\n", frame_ctl & (RTW_IEEE80211_FCTL_FTYPE | RTW_IEEE80211_FCTL_STYPE));


fail:

	rtw_skb_free(skb);

	return 0;

}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0))
static void rtw_cfg80211_monitor_if_set_multicast_list(struct net_device *ndev)
{
	RTW_INFO("%s\n", __func__);
}
#endif
static int rtw_cfg80211_monitor_if_set_mac_address(struct net_device *ndev, void *addr)
{
	int ret = 0;

	RTW_INFO("%s\n", __func__);

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29))
static const struct net_device_ops rtw_cfg80211_monitor_if_ops = {
	.ndo_open = rtw_cfg80211_monitor_if_open,
	.ndo_stop = rtw_cfg80211_monitor_if_close,
	.ndo_start_xmit = rtw_cfg80211_monitor_if_xmit_entry,
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0))
	.ndo_set_multicast_list = rtw_cfg80211_monitor_if_set_multicast_list,
	#endif
	.ndo_set_mac_address = rtw_cfg80211_monitor_if_set_mac_address,
};
#endif

static int rtw_cfg80211_add_monitor_if(_adapter *padapter, char *name, struct net_device **ndev)
{
	int ret = 0;
	struct net_device *mon_ndev = NULL;
	struct wireless_dev *mon_wdev = NULL;
	struct rtw_netdev_priv_indicator *pnpi;
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(padapter);

	if (!name) {
		RTW_INFO(FUNC_ADPT_FMT" without specific name\n", FUNC_ADPT_ARG(padapter));
		ret = -EINVAL;
		goto out;
	}

	if (pwdev_priv->pmon_ndev) {
		RTW_INFO(FUNC_ADPT_FMT" monitor interface exist: "NDEV_FMT"\n",
			FUNC_ADPT_ARG(padapter), NDEV_ARG(pwdev_priv->pmon_ndev));
		ret = -EBUSY;
		goto out;
	}

	mon_ndev = alloc_etherdev(sizeof(struct rtw_netdev_priv_indicator));
	if (!mon_ndev) {
		RTW_INFO(FUNC_ADPT_FMT" allocate ndev fail\n", FUNC_ADPT_ARG(padapter));
		ret = -ENOMEM;
		goto out;
	}

	mon_ndev->type = ARPHRD_IEEE80211_RADIOTAP;
	strncpy(mon_ndev->name, name, IFNAMSIZ);
	mon_ndev->name[IFNAMSIZ - 1] = 0;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 11, 8))
	mon_ndev->priv_destructor = rtw_ndev_destructor;
#else
	mon_ndev->destructor = rtw_ndev_destructor;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29))
	mon_ndev->netdev_ops = &rtw_cfg80211_monitor_if_ops;
#else
	mon_ndev->open = rtw_cfg80211_monitor_if_open;
	mon_ndev->stop = rtw_cfg80211_monitor_if_close;
	mon_ndev->hard_start_xmit = rtw_cfg80211_monitor_if_xmit_entry;
	mon_ndev->set_mac_address = rtw_cfg80211_monitor_if_set_mac_address;
#endif

	pnpi = netdev_priv(mon_ndev);
	pnpi->priv = padapter;
	pnpi->sizeof_priv = sizeof(_adapter);

	/*  wdev */
	mon_wdev = (struct wireless_dev *)rtw_zmalloc(sizeof(struct wireless_dev));
	if (!mon_wdev) {
		RTW_INFO(FUNC_ADPT_FMT" allocate mon_wdev fail\n", FUNC_ADPT_ARG(padapter));
		ret = -ENOMEM;
		goto out;
	}

	mon_wdev->wiphy = padapter->rtw_wdev->wiphy;
	mon_wdev->netdev = mon_ndev;
	mon_wdev->iftype = NL80211_IFTYPE_MONITOR;
	mon_ndev->ieee80211_ptr = mon_wdev;

	ret = register_netdevice(mon_ndev);
	if (ret)
		goto out;

	*ndev = pwdev_priv->pmon_ndev = mon_ndev;
	_rtw_memcpy(pwdev_priv->ifname_mon, name, IFNAMSIZ + 1);

out:
	if (ret && mon_wdev) {
		rtw_mfree((u8 *)mon_wdev, sizeof(struct wireless_dev));
		mon_wdev = NULL;
	}

	if (ret && mon_ndev) {
		free_netdev(mon_ndev);
		*ndev = mon_ndev = NULL;
	}

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
static struct wireless_dev *
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)) || defined(COMPAT_KERNEL_RELEASE)
static struct net_device *
#else
static int
#endif
	cfg80211_rtw_add_virtual_intf(
		struct wiphy *wiphy,
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
		const char *name,
		#else
		char *name,
		#endif
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
		unsigned char name_assign_type,
		#endif
		enum nl80211_iftype type,
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)) && !defined(CPTCFG_VERSION)
		u32 *flags,
		#endif
		struct vif_params *params)
{
	int ret = 0;
	struct wireless_dev *wdev = NULL;
	struct net_device *ndev = NULL;
	_adapter *padapter;
	struct dvobj_priv *dvobj = wiphy_to_dvobj(wiphy);

	rtw_set_rtnl_lock_holder(dvobj, current);

	RTW_INFO(FUNC_WIPHY_FMT" name:%s, type:%d\n", FUNC_WIPHY_ARG(wiphy), name, type);

	switch (type) {
	case NL80211_IFTYPE_MONITOR:
		padapter = wiphy_to_adapter(wiphy); /* TODO: get ap iface ? */
		ret = rtw_cfg80211_add_monitor_if(padapter, (char *)name, &ndev);
		if (ret == 0)
			wdev = ndev->ieee80211_ptr;
		break;

#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_P2P_GO:
#if !RTW_P2P_GROUP_INTERFACE
		RTW_ERR("%s, can't add GO/GC interface when RTW_P2P_GROUP_INTERFACE is not defined\n", __func__);
		ret = -EOPNOTSUPP;
		break;
#endif
#endif
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_AP:
#ifdef CONFIG_RTW_MESH
	case NL80211_IFTYPE_MESH_POINT:
#endif
		padapter = dvobj_get_unregisterd_adapter(dvobj);
		if (!padapter) {
			RTW_WARN("adapter pool empty!\n");
			ret = -ENODEV;
			break;
		}

		if (rtw_os_ndev_init(padapter, name) != _SUCCESS) {
			RTW_WARN("ndev init fail!\n");
			ret = -ENODEV;
			break;
		}

		#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
		if (type == NL80211_IFTYPE_P2P_CLIENT)
			rtw_p2p_enable(padapter, P2P_ROLE_CLIENT);
		else if (type == NL80211_IFTYPE_P2P_GO)
			rtw_p2p_enable(padapter, P2P_ROLE_GO);
		#endif

		ndev = padapter->pnetdev;
		wdev = ndev->ieee80211_ptr;
		break;

#if defined(CONFIG_P2P) && defined(RTW_DEDICATED_P2P_DEVICE)
	case NL80211_IFTYPE_P2P_DEVICE:
		ret = rtw_pd_iface_alloc(wiphy, name, &wdev);
		break;
#endif

	case NL80211_IFTYPE_ADHOC:
	case NL80211_IFTYPE_AP_VLAN:
	case NL80211_IFTYPE_WDS:
	default:
		ret = -ENODEV;
		RTW_INFO("Unsupported interface type\n");
		break;
	}

	if (ndev)
		RTW_INFO(FUNC_WIPHY_FMT" ndev:%p, ret:%d\n", FUNC_WIPHY_ARG(wiphy), ndev, ret);
	else
		RTW_INFO(FUNC_WIPHY_FMT" wdev:%p, ret:%d\n", FUNC_WIPHY_ARG(wiphy), wdev, ret);

	rtw_set_rtnl_lock_holder(dvobj, NULL);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	return wdev ? wdev : ERR_PTR(ret);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)) || defined(COMPAT_KERNEL_RELEASE)
	return ndev ? ndev : ERR_PTR(ret);
#else
	return ret;
#endif
}

static int cfg80211_rtw_del_virtual_intf(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct wireless_dev *wdev
#else
	struct net_device *ndev
#endif
)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct net_device *ndev = wdev_to_ndev(wdev);
#endif
	int ret = 0;
	struct dvobj_priv *dvobj = wiphy_to_dvobj(wiphy);
	_adapter *adapter;
	struct rtw_wdev_priv *pwdev_priv;

	rtw_set_rtnl_lock_holder(dvobj, current);

	if (ndev) {
		adapter = (_adapter *)rtw_netdev_priv(ndev);
		pwdev_priv = adapter_wdev_data(adapter);

		if (ndev == pwdev_priv->pmon_ndev) {
			unregister_netdevice(ndev);
			pwdev_priv->pmon_ndev = NULL;
			pwdev_priv->ifname_mon[0] = '\0';
			RTW_INFO(FUNC_NDEV_FMT" remove monitor ndev\n", FUNC_NDEV_ARG(ndev));
		} else {
			RTW_INFO(FUNC_NDEV_FMT" unregister ndev\n", FUNC_NDEV_ARG(ndev));
			rtw_os_ndev_unregister(adapter);
		}
#ifdef CONFIG_P2P
		if (!rtw_p2p_chk_role(&adapter->wdinfo, P2P_ROLE_DISABLE))
			rtw_p2p_enable(adapter, P2P_ROLE_DISABLE);
#endif
	} else
#if defined(CONFIG_P2P) && defined(RTW_DEDICATED_P2P_DEVICE)
	if (wdev->iftype == NL80211_IFTYPE_P2P_DEVICE) {
		if (wdev == wiphy_to_pd_wdev(wiphy))
			rtw_pd_iface_free(wiphy);
		else {
			RTW_ERR(FUNC_WIPHY_FMT" unknown P2P Device wdev:%p\n", FUNC_WIPHY_ARG(wiphy), wdev);
			rtw_warn_on(1);
		}
	} else
#endif
	{
		ret = -EINVAL;
		goto exit;
	}

exit:
	rtw_set_rtnl_lock_holder(dvobj, NULL);
	return ret;
}

static int rtw_add_beacon(_adapter *adapter, const u8 *head, size_t head_len, const u8 *tail, size_t tail_len)
{
	int ret = 0;
	u8 *pbuf = NULL;
	uint len, wps_ielen = 0;
	uint p2p_ielen = 0;
	struct mlme_priv *pmlmepriv = &(adapter->mlmepriv);
	/* struct sta_priv *pstapriv = &padapter->stapriv; */


	RTW_INFO("%s beacon_head_len=%zu, beacon_tail_len=%zu\n", __FUNCTION__, head_len, tail_len);


	if (check_fwstate(pmlmepriv, WIFI_AP_STATE) != _TRUE)
		return -EINVAL;

	if (head_len < 24)
		return -EINVAL;

	pbuf = rtw_zmalloc(head_len + tail_len);
	if (!pbuf) {
		ret = -ENOMEM;
		goto exit;
	}


	/* _rtw_memcpy(&pstapriv->max_num_sta, param->u.bcn_ie.reserved, 2); */

	/* if((pstapriv->max_num_sta>NUM_STA) || (pstapriv->max_num_sta<=0)) */
	/*	pstapriv->max_num_sta = NUM_STA; */


	_rtw_memcpy(pbuf, (void *)head + 24, head_len - 24); /* 24=beacon header len. */
	_rtw_memcpy(pbuf + head_len - 24, (void *)tail, tail_len);

	len = head_len + tail_len - 24;

	/* check wps ie if inclued */
	if (rtw_get_wps_ie(pbuf + _FIXED_IE_LENGTH_, len - _FIXED_IE_LENGTH_, NULL, &wps_ielen))
		RTW_INFO("add bcn, wps_ielen=%d\n", wps_ielen);

#ifdef CONFIG_P2P
	/* check p2p if enable */
	if (rtw_get_p2p_ie(pbuf + _FIXED_IE_LENGTH_, len - _FIXED_IE_LENGTH_, NULL, &p2p_ielen)) {
		struct wifidirect_info *pwdinfo = &(adapter->wdinfo);

		RTW_INFO("got p2p_ie, len=%d\n", p2p_ielen);

		if (!rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO)) {
			RTW_INFO("add p2p beacon whitout GO mode, p2p_role=%d\n",
				 rtw_p2p_role(pwdinfo));
			ret = -EOPNOTSUPP;
			goto exit;
		}
	}
#endif /* CONFIG_P2P */

	if (adapter_to_dvobj(adapter)->wpas_type == RTW_WPAS_ANDROID) {
		/* pbss_network->IEs will not include p2p_ie, wfd ie */
		rtw_ies_remove_ie(pbuf, &len, _BEACON_IE_OFFSET_, _VENDOR_SPECIFIC_IE_, P2P_OUI, 4);
		rtw_ies_remove_ie(pbuf, &len, _BEACON_IE_OFFSET_, _VENDOR_SPECIFIC_IE_, WFD_OUI, 4);
	}

	if (rtw_check_beacon_data(adapter, pbuf,  len) == _SUCCESS) {
		ret = 0;
	} else
		ret = -EINVAL;

exit:
	if (pbuf)
		rtw_mfree(pbuf, head_len + tail_len);

	return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)) && !defined(COMPAT_KERNEL_RELEASE)
static int cfg80211_rtw_add_beacon(struct wiphy *wiphy, struct net_device *ndev,
		struct beacon_parameters *info)
{
	int ret = 0;
	_adapter *adapter = (_adapter *)rtw_netdev_priv(ndev);

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	if (rtw_cfg80211_sync_iftype(adapter) != _SUCCESS) {
		ret = -ENOTSUPP;
		goto exit;
	}
	rtw_mi_scan_abort(adapter, _TRUE);
	rtw_mi_buddy_set_scan_deny(adapter, 300);
	ret = rtw_add_beacon(adapter, info->head, info->head_len, info->tail, info->tail_len);

exit:
	return ret;
}

static int cfg80211_rtw_set_beacon(struct wiphy *wiphy, struct net_device *ndev,
		struct beacon_parameters *info)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(ndev);
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *adapter_link = GET_PRIMARY_LINK(adapter);
	struct link_mlme_ext_priv *pmlmeext = &(adapter_link->mlmeextpriv);

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	pmlmeext->bstart_bss = _TRUE;

	cfg80211_rtw_add_beacon(wiphy, ndev, info);

	return 0;
}

static int	cfg80211_rtw_del_beacon(struct wiphy *wiphy, struct net_device *ndev)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(ndev);

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	rtw_mi_buddy_set_scan_deny(adapter, 300);
	rtw_mi_scan_abort(adapter, _TRUE);
	rtw_stop_ap_cmd(adapter, RTW_CMDF_WAIT_ACK);
	return 0;
}
#else
static int rtw_cfg80211_set_beacon_ies(struct net_device *net, const u8 *head,
				       int head_len, const u8 *tail, int tail_len);
static int cfg80211_rtw_start_ap(struct wiphy *wiphy, struct net_device *ndev,
		struct cfg80211_ap_settings *settings)
{
	int ret = 0;
	_adapter *adapter = (_adapter *)rtw_netdev_priv(ndev);
	struct mlme_ext_priv *pmlmeext = &adapter->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &(pmlmeext->mlmext_info);
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *adapter_link = GET_PRIMARY_LINK(adapter);

	if (adapter_to_dvobj(adapter)->wpas_type == RTW_WPAS_W1FI) {
		/* turn on the beacon send */
		adapter_link->mlmeextpriv.bstart_bss = _TRUE;
	}

	RTW_INFO(FUNC_NDEV_FMT" hidden_ssid:%d, auth_type:%d\n", FUNC_NDEV_ARG(ndev),
		settings->hidden_ssid, settings->auth_type);

	if (rtw_cfg80211_sync_iftype(adapter) != _SUCCESS) {
		ret = -ENOTSUPP;
		goto exit;
	}

#ifdef RTW_PHL_BCN
	{
		struct rtw_wifi_role_t *wrole = adapter->phl_role;
		struct rtw_wifi_role_link_t *rlink = adapter_link->wrlink;
		if (rlink)
			rlink->bcn_cmn.bcn_added = 0;
	}
#endif

	/* Kernel < v5.x, the auth_type set as NL80211_AUTHTYPE_AUTOMATIC. if
	 * the AKM SAE in the RSN IE, we have to update the auth_type for SAE in
	 * rtw_check_beacon_data().
	 *
	 * we only update auth_type when rtw_check_beacon_data()
	 */
	/* rtw_cfg80211_set_auth_type(&adapter->securitypriv, settings->auth_type); */

	rtw_mi_scan_abort(adapter, _TRUE);
	rtw_mi_buddy_set_scan_deny(adapter, 300);
	ret = rtw_add_beacon(adapter, settings->beacon.head, settings->beacon.head_len,
		settings->beacon.tail, settings->beacon.tail_len);

	pmlmeinfo->hidden_ssid_mode = settings->hidden_ssid;

	rtw_cfg80211_set_beacon_ies(ndev, settings->beacon.head,
				    settings->beacon.head_len,
				    settings->beacon.tail,
				    settings->beacon.tail_len);

#ifdef CONFIG_RTW_80211R_AP
	rtw_ft_update_assocresp_ies(ndev, settings);
#endif

	if (settings->beacon.assocresp_ies &&
		settings->beacon.assocresp_ies_len > 0) {
			rtw_cfg80211_set_assocresp_ies(ndev,
			settings->beacon.assocresp_ies,
			settings->beacon.assocresp_ies_len);
	}

	if (settings->ssid && settings->ssid_len) {
		WLAN_BSSID_EX *pbss_network = &adapter->mlmepriv.dev_cur_network.network;
		WLAN_BSSID_EX *pbss_network_ext = &pmlmeinfo->dev_network;

		if (0)
			RTW_INFO(FUNC_ADPT_FMT" ssid:(%s,%zu), from ie:(%s,%d)\n", FUNC_ADPT_ARG(adapter),
				settings->ssid, settings->ssid_len,
				pbss_network->Ssid.Ssid, pbss_network->Ssid.SsidLength);

		_rtw_memcpy(pbss_network->Ssid.Ssid, (void *)settings->ssid, settings->ssid_len);
		pbss_network->Ssid.SsidLength = settings->ssid_len;
		_rtw_memcpy(pbss_network_ext->Ssid.Ssid, (void *)settings->ssid, settings->ssid_len);
		pbss_network_ext->Ssid.SsidLength = settings->ssid_len;

		if (0)
			RTW_INFO(FUNC_ADPT_FMT" after ssid:(%s,%d), (%s,%d)\n", FUNC_ADPT_ARG(adapter),
				pbss_network->Ssid.Ssid, pbss_network->Ssid.SsidLength,
				pbss_network_ext->Ssid.Ssid, pbss_network_ext->Ssid.SsidLength);
	}
	pmlmeinfo->assoc_AP_vendor = HT_IOT_PEER_REALTEK;

exit:
	return ret;
}

static int rtw_cfg80211_set_assocresp_ies(struct net_device *net, const u8 *buf, int len)
{
	int ret = 0;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(net);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

	RTW_INFO("%s, ielen=%d\n", __func__, len);

	if (len <= 0)
		goto exit;

	if (pmlmepriv->assoc_rsp) {
		u32 free_len = pmlmepriv->assoc_rsp_len;

		pmlmepriv->assoc_rsp_len = 0;
		rtw_mfree(pmlmepriv->assoc_rsp, free_len);
		pmlmepriv->assoc_rsp = NULL;
	}

	pmlmepriv->assoc_rsp = rtw_malloc(len);
	if (pmlmepriv->assoc_rsp == NULL) {
		RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	_rtw_memcpy(pmlmepriv->assoc_rsp, buf, len);
	pmlmepriv->assoc_rsp_len = len;

exit:
	return ret;
}


static int rtw_cfg80211_check_beacon_ies(struct net_device *net, const u8 *head,
					 int head_len, const u8 *tail,
					 int tail_len)
{
	int ret = 2;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(net);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

	if (pmlmepriv->beacon_head_ie &&
	    (pmlmepriv->beacon_head_ie_len == head_len)) {
		if (_rtw_memcmp(pmlmepriv->beacon_head_ie, head, head_len) ==
		    _TRUE)
			ret--;
	}
	if (pmlmepriv->beacon_tail_ie &&
	    (pmlmepriv->beacon_tail_ie_len == tail_len)) {
		if (_rtw_memcmp(pmlmepriv->beacon_tail_ie, tail, tail_len) ==
		    _TRUE)
			ret--;
	}
	return ret;
}

static int rtw_cfg80211_set_beacon_ies(struct net_device *net, const u8 *head,
				       int head_len, const u8 *tail,
				       int tail_len)
{
	int ret = 0;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(net);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

	RTW_INFO("%s, len(head, tail)=(%d, %d)\n", __func__, head_len,
		 tail_len);

	if (pmlmepriv->beacon_head_ie) {
		u32 free_len = pmlmepriv->beacon_head_ie_len;

		pmlmepriv->beacon_head_ie_len = 0;
		rtw_mfree(pmlmepriv->beacon_head_ie, free_len);
		pmlmepriv->beacon_head_ie = NULL;
	}

	if (head_len) {
		pmlmepriv->beacon_head_ie = rtw_malloc(head_len);
		if (pmlmepriv->beacon_head_ie == NULL) {
			RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__,
				 __LINE__);
			return -EINVAL;
		}
		_rtw_memcpy(pmlmepriv->beacon_head_ie, head, head_len);
		pmlmepriv->beacon_head_ie_len = head_len;
	}

	if (pmlmepriv->beacon_tail_ie) {
		u32 free_len = pmlmepriv->beacon_tail_ie_len;

		pmlmepriv->beacon_tail_ie_len = 0;
		rtw_mfree(pmlmepriv->beacon_tail_ie, free_len);
		pmlmepriv->beacon_tail_ie = NULL;
	}

	if (tail_len) {
		pmlmepriv->beacon_tail_ie = rtw_malloc(tail_len);
		if (pmlmepriv->beacon_tail_ie == NULL) {
			RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__,
				 __LINE__);
			return -EINVAL;
		}
		_rtw_memcpy(pmlmepriv->beacon_tail_ie, tail, tail_len);
		pmlmepriv->beacon_tail_ie_len = tail_len;
	}

	return ret;
}

static int cfg80211_rtw_change_beacon(struct wiphy *wiphy, struct net_device *ndev,
		struct cfg80211_beacon_data *info)
{
	int ret = 0;
	_adapter *adapter = (_adapter *)rtw_netdev_priv(ndev);

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

#ifdef not_yet
	/*
	 * @proberesp_ies: extra information element(s) to add into Probe Response
	 *	frames or %NULL
	 * @proberesp_ies_len: length of proberesp_ies in octets
	 */
	if (info->proberesp_ies_len > 0)
		rtw_cfg80211_set_proberesp_ies(ndev, info->proberesp_ies, info->proberesp_ies_len);
#endif /* not_yet */

	if (info->assocresp_ies_len > 0)
		rtw_cfg80211_set_assocresp_ies(ndev, info->assocresp_ies, info->assocresp_ies_len);

	if (rtw_cfg80211_check_beacon_ies(ndev, info->head, info->head_len,
					  info->tail, info->tail_len) != 0) {
		ret = rtw_add_beacon(adapter, info->head, info->head_len,
				     info->tail, info->tail_len);
		rtw_cfg80211_set_beacon_ies(ndev, info->head, info->head_len,
					    info->tail, info->tail_len);
	}
	return ret;
}

static int cfg80211_rtw_stop_ap(struct wiphy *wiphy, struct net_device *ndev
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 2)) || defined(CONFIG_MLD_KERNEL_PATCH)
	, unsigned int link_id
#endif
)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(ndev);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 2)) || defined(CONFIG_MLD_KERNEL_PATCH)
	RTW_INFO(FUNC_NDEV_FMT" link_id:%d\n", FUNC_NDEV_ARG(ndev), link_id);
#else
	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));
#endif
	/* ToDo CONFIG_RTW_MLD */

	rtw_ap_stop_set_state(adapter, AP_STOP_ST_START);
	rtw_mi_buddy_set_scan_deny(adapter, 300);
	rtw_mi_scan_abort(adapter, _TRUE);
	rtw_sta_flush(adapter, _TRUE);
	rtw_stop_ap_cmd(adapter, RTW_CMDF_WAIT_ACK);
	return 0;
}
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)) */

#if CONFIG_RTW_MACADDR_ACL && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
static int cfg80211_rtw_set_mac_acl(struct wiphy *wiphy, struct net_device *ndev,
		const struct cfg80211_acl_data *params)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(ndev);
	u8 acl_mode = RTW_ACL_MODE_DISABLED;
	int ret = -1;
	int i;

	if (!params) {
		RTW_WARN(FUNC_ADPT_FMT" params NULL\n", FUNC_ADPT_ARG(adapter));
		rtw_macaddr_acl_clear(adapter, RTW_ACL_PERIOD_BSS);
		goto exit;
	}

	RTW_INFO(FUNC_ADPT_FMT" acl_policy:%d, entry_num:%d\n"
		, FUNC_ADPT_ARG(adapter), params->acl_policy, params->n_acl_entries);

	if (params->acl_policy == NL80211_ACL_POLICY_ACCEPT_UNLESS_LISTED)
		acl_mode = RTW_ACL_MODE_ACCEPT_UNLESS_LISTED;
	else if (params->acl_policy == NL80211_ACL_POLICY_DENY_UNLESS_LISTED)
		acl_mode = RTW_ACL_MODE_DENY_UNLESS_LISTED;

	rtw_macaddr_acl_clear(adapter, RTW_ACL_PERIOD_BSS);

	rtw_set_macaddr_acl(adapter, RTW_ACL_PERIOD_BSS, acl_mode);

	for (i = 0; i < params->n_acl_entries; i++)
		rtw_acl_add_sta(adapter, RTW_ACL_PERIOD_BSS, params->mac_addrs[i].addr);

	ret = 0;

exit:
	return ret;
}
#endif /* CONFIG_RTW_MACADDR_ACL && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)) */

const char *_nl80211_sta_flags_str[] = {
	"INVALID",
	"AUTHORIZED",
	"SHORT_PREAMBLE",
	"WME",
	"MFP",
	"AUTHENTICATED",
	"TDLS_PEER",
	"ASSOCIATED",
};

#define nl80211_sta_flags_str(_f) ((_f <= NL80211_STA_FLAG_MAX) ? _nl80211_sta_flags_str[_f] : _nl80211_sta_flags_str[0])

const char *_nl80211_plink_state_str[] = {
	"LISTEN",
	"OPN_SNT",
	"OPN_RCVD",
	"CNF_RCVD",
	"ESTAB",
	"HOLDING",
	"BLOCKED",
	"UNKNOWN",
};

#define nl80211_plink_state_str(_s) ((_s < NUM_NL80211_PLINK_STATES) ? _nl80211_plink_state_str[_s] : _nl80211_plink_state_str[NUM_NL80211_PLINK_STATES])

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
#define NL80211_PLINK_ACTION_NO_ACTION PLINK_ACTION_INVALID
#define NL80211_PLINK_ACTION_OPEN PLINK_ACTION_OPEN
#define NL80211_PLINK_ACTION_BLOCK PLINK_ACTION_BLOCK
#define NUM_NL80211_PLINK_ACTIONS 3
#endif

const char *_nl80211_plink_actions_str[] = {
	"NO_ACTION",
	"OPEN",
	"BLOCK",
	"UNKNOWN",
};

#define nl80211_plink_actions_str(_a) ((_a < NUM_NL80211_PLINK_ACTIONS) ? _nl80211_plink_actions_str[_a] : _nl80211_plink_actions_str[NUM_NL80211_PLINK_ACTIONS])

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
const char *_nl80211_mesh_power_mode_str[] = {
	"UNKNOWN",
	"ACTIVE",
	"LIGHT_SLEEP",
	"DEEP_SLEEP",
};

#define nl80211_mesh_power_mode_str(_p) ((_p <= NL80211_MESH_POWER_MAX) ? _nl80211_mesh_power_mode_str[_p] : _nl80211_mesh_power_mode_str[0])
#endif

void dump_station_parameters(void *sel, struct wiphy *wiphy, const struct station_parameters *params)
{
#if DBG_RTW_CFG80211_STA_PARAM
	if (params->supported_rates_len) {
		#define SUPP_RATES_BUF_LEN (3 * RTW_G_RATES_NUM + 1)
		int i;
		char supp_rates_buf[SUPP_RATES_BUF_LEN] = {0};
		u8 cnt = 0;

		rtw_warn_on(params->supported_rates_len > RTW_G_RATES_NUM);

		for (i = 0; i < params->supported_rates_len; i++) {
			if (i >= RTW_G_RATES_NUM)
				break;
			cnt += snprintf(supp_rates_buf + cnt, SUPP_RATES_BUF_LEN - cnt -1
				, "%02X ", params->supported_rates[i]);
			if (cnt >= SUPP_RATES_BUF_LEN - 1)
				break;
		}

		RTW_PRINT_SEL(sel, "supported_rates:%s\n", supp_rates_buf);
	}

	if (params->vlan)
		RTW_PRINT_SEL(sel, "vlan:"NDEV_FMT"\n", NDEV_ARG(params->vlan));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
	if (params->sta_flags_mask) {
		#define STA_FLAGS_BUF_LEN 128
		int i = 0;
		char sta_flags_buf[STA_FLAGS_BUF_LEN] = {0};
		u8 cnt = 0;

		for (i = 1; i <= NL80211_STA_FLAG_MAX; i++) {
			if (params->sta_flags_mask & BIT(i)) {
				cnt += snprintf(sta_flags_buf + cnt, STA_FLAGS_BUF_LEN - cnt -1, "%s=%u "
					, nl80211_sta_flags_str(i), (params->sta_flags_set & BIT(i)) ? 1 : 0);
				if (cnt >= STA_FLAGS_BUF_LEN - 1)
					break;
			}
		}

		RTW_PRINT_SEL(sel, "sta_flags:%s\n", sta_flags_buf);
	}
#else
	u32 station_flags;
	#error "TBD\n"
#endif

	if (params->listen_interval != -1)
		RTW_PRINT_SEL(sel, "listen_interval:%d\n", params->listen_interval);

	if (params->aid)
		RTW_PRINT_SEL(sel, "aid:%u\n", params->aid);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0))
	if (params->peer_aid)
		RTW_PRINT_SEL(sel, "peer_aid:%u\n", params->peer_aid);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26))
	if (params->plink_action != NL80211_PLINK_ACTION_NO_ACTION)
		RTW_PRINT_SEL(sel, "plink_action:%s\n", nl80211_plink_actions_str(params->plink_action));
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	if (params->sta_modify_mask & STATION_PARAM_APPLY_PLINK_STATE)
	#endif
		RTW_PRINT_SEL(sel, "plink_state:%s\n"
			, nl80211_plink_state_str(params->plink_state));
#endif

#if 0 /* TODO */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28))
	const struct ieee80211_ht_cap *ht_capa;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	const struct ieee80211_vht_cap *vht_capa;
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	if (params->sta_modify_mask & STATION_PARAM_APPLY_UAPSD)
		RTW_PRINT_SEL(sel, "uapsd_queues:0x%02x\n", params->uapsd_queues);
	if (params->max_sp)
		RTW_PRINT_SEL(sel, "max_sp:%u\n", params->max_sp);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	if (params->local_pm != NL80211_MESH_POWER_UNKNOWN) {
		RTW_PRINT_SEL(sel, "local_pm:%s\n"
			, nl80211_mesh_power_mode_str(params->local_pm));
	}

	if (params->sta_modify_mask & STATION_PARAM_APPLY_CAPABILITY)
		RTW_PRINT_SEL(sel, "capability:0x%04x\n", params->capability);

#if 0 /* TODO */
	const u8 *ext_capab;
	u8 ext_capab_len;
#endif
#endif

#if 0 /* TODO */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	const u8 *supported_channels;
	u8 supported_channels_len;
	const u8 *supported_oper_classes;
	u8 supported_oper_classes_len;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
	u8 opmode_notif;
	bool opmode_notif_used;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
	int support_p2p_ps;
#endif
#endif
#endif /* DBG_RTW_CFG80211_STA_PARAM */
}

static int	cfg80211_rtw_add_station(struct wiphy *wiphy, struct net_device *ndev,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0))
	u8 *mac,
#else
	const u8 *mac,
#endif
	struct station_parameters *params)
{
	int ret = 0;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
#if defined(CONFIG_TDLS) || defined(CONFIG_RTW_MESH)
	struct sta_priv *pstapriv = &padapter->stapriv;
#endif
#ifdef CONFIG_TDLS
	struct rtw_wifi_role_t *wrole = padapter->phl_role;
	struct sta_info *psta;
	enum role_type rtype = PHL_RTYPE_TDLS;
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_SUCCESS;
#endif /* CONFIG_TDLS */
	void *phl = GET_PHL_INFO(adapter_to_dvobj(padapter));
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	u16 main_id = rtw_phl_get_macid_max_num(phl);
	struct rtw_phl_mld_t *pmld = NULL;

	RTW_INFO(FUNC_NDEV_FMT" mac:"MAC_FMT"\n", FUNC_NDEV_ARG(ndev), MAC_ARG(mac));

#if CONFIG_RTW_MACADDR_ACL
	if (rtw_access_ctrl(padapter, mac) == _FALSE) {
		RTW_INFO(FUNC_NDEV_FMT" deny by macaddr ACL\n", FUNC_NDEV_ARG(ndev));
		ret = -EINVAL;
		goto exit;
	}
#endif

	dump_station_parameters(RTW_DBGDUMP, wiphy, params);

#ifdef CONFIG_RTW_MESH
	if (MLME_IS_MESH(padapter)) {
		struct rtw_mesh_cfg *mcfg = &padapter->mesh_cfg;
		struct rtw_mesh_info *minfo = &padapter->mesh_info;
		struct mesh_plink_pool *plink_ctl = &minfo->plink_ctl;
		struct mesh_plink_ent *plink = NULL;
		struct wlan_network *scanned = NULL;
		bool acnode = 0;
		u8 add_new_sta = 0, probe_req = 0;

		if (params->plink_state != NL80211_PLINK_LISTEN) {
			RTW_WARN(FUNC_NDEV_FMT" %s\n", FUNC_NDEV_ARG(ndev), nl80211_plink_state_str(params->plink_state));
			rtw_warn_on(1);
		}
		if (!params->aid || params->aid > pstapriv->max_aid) {
			RTW_WARN(FUNC_NDEV_FMT" invalid aid:%u\n", FUNC_NDEV_ARG(ndev), params->aid);
			rtw_warn_on(1);
			ret = -EINVAL;
			goto exit;
		}

		_rtw_spinlock_bh(&(plink_ctl->lock));

		plink = _rtw_mesh_plink_get(padapter, mac);
		if (plink)
			goto release_plink_ctl;

		#if CONFIG_RTW_MESH_PEER_BLACKLIST
		if (rtw_mesh_peer_blacklist_search(padapter, mac)) {
			RTW_INFO(FUNC_NDEV_FMT" deny by peer blacklist\n"
				, FUNC_NDEV_ARG(ndev));
			ret = -EINVAL;
			goto release_plink_ctl;
		}
		#endif

		scanned = rtw_find_network(&padapter->mlmepriv.scanned_queue, mac);
		if (!scanned
			|| rtw_get_passing_time_ms(scanned->last_scanned) >= mcfg->peer_sel_policy.scanr_exp_ms
		) {
			if (!scanned)
				RTW_INFO(FUNC_NDEV_FMT" corresponding network not found\n", FUNC_NDEV_ARG(ndev));
			else
				RTW_INFO(FUNC_NDEV_FMT" corresponding network too old\n", FUNC_NDEV_ARG(ndev));

			if (adapter_to_rfctl(padapter)->offch_state == OFFCHS_NONE)
				probe_req = 1;

			ret = -EINVAL;
			goto release_plink_ctl;
		}

		#if CONFIG_RTW_MESH_ACNODE_PREVENT
		if (plink_ctl->acnode_rsvd)
			acnode = rtw_mesh_scanned_is_acnode_confirmed(padapter, scanned);
		#endif

		/* wpa_supplicant's auto peer will initiate peering when candidate peer is reported without max_peer_links consideration */
		if (plink_ctl->num >= mcfg->max_peer_links + acnode ? 1 : 0) {
			RTW_INFO(FUNC_NDEV_FMT" exceed max_peer_links:%u%s\n"
				, FUNC_NDEV_ARG(ndev), mcfg->max_peer_links, acnode ? " acn" : "");
			ret = -EINVAL;
			goto release_plink_ctl;
		}

		if (!rtw_bss_is_candidate_mesh_peer(padapter, &scanned->network, 1, 1)) {
			RTW_WARN(FUNC_NDEV_FMT" corresponding network is not candidate with same ch\n"
				, FUNC_NDEV_ARG(ndev));
			ret = -EINVAL;
			goto release_plink_ctl;
		}

		#if CONFIG_RTW_MESH_CTO_MGATE_BLACKLIST
		if (!rtw_mesh_cto_mgate_network_filter(padapter, scanned)) {
			RTW_INFO(FUNC_NDEV_FMT" peer filtered out by cto_mgate check\n"
				, FUNC_NDEV_ARG(ndev));
			ret = -EINVAL;
			goto release_plink_ctl;
		}
		#endif

		if (_rtw_mesh_plink_add(padapter, mac) == _SUCCESS) {
			/* hook corresponding network in scan queue */
			plink = _rtw_mesh_plink_get(padapter, mac);
			plink->aid = params->aid;
			plink->scanned = scanned;

			#if CONFIG_RTW_MESH_ACNODE_PREVENT
			if (acnode) {
				RTW_INFO(FUNC_ADPT_FMT" acnode "MAC_FMT"\n"
				, FUNC_ADPT_ARG(padapter), MAC_ARG(scanned->network.MacAddress));
			}
			#endif

			add_new_sta = 1;
		} else {
			RTW_WARN(FUNC_NDEV_FMT" rtw_mesh_plink_add not success\n"
				, FUNC_NDEV_ARG(ndev));
			ret = -EINVAL;
		}
release_plink_ctl:
		_rtw_spinunlock_bh(&(plink_ctl->lock));

		if (probe_req)
			issue_probereq(padapter, &padapter->mlmepriv.dev_cur_network.dev_network.mesh_id, mac);

		if (add_new_sta) {
			struct station_info sinfo;

			#ifdef CONFIG_DFS_MASTER
			if (IS_UNDER_CAC(adapter_to_rfctl(padapter)))
				rtw_force_stop_cac(adapter_to_rfctl(padapter), 300);
			#endif

			/* indicate new sta */
			_rtw_memset(&sinfo, 0, sizeof(sinfo));
			cfg80211_new_sta(ndev, mac, &sinfo, GFP_ATOMIC);
		}
		goto exit;
	}
#endif /* CONFIG_RTW_MESH */

#ifdef CONFIG_TDLS
	if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER)) {
		if (wrole->type != PHL_RTYPE_TDLS) {
			pstatus = rtw_phl_cmd_wrole_change(phl, wrole, NULL, WR_CHG_TYPE,
							   (u8*)&rtype, sizeof(enum role_type),
							   PHL_CMD_DIRECTLY, 0);
			if (pstatus != RTW_PHL_STATUS_SUCCESS) {
				RTW_ERR("%s - change to phl role type = %d fail with error = %d\n",
					__func__, rtype, pstatus);
				rtw_warn_on(1);
				ret = -EOPNOTSUPP;
				goto exit;
			}
		}
		psta = rtw_get_stainfo(pstapriv, (u8 *)mac);
		if (psta == NULL) {
			pmld = rtw_phl_alloc_mld(phl, padapter->phl_role, (u8 *)mac, DTYPE);
			if (pmld == NULL) {
				RTW_INFO("[%s] Alloc mld for "MAC_FMT" fail\n", __FUNCTION__, MAC_ARG(mac));
				ret = -EOPNOTSUPP;
				goto exit;
			}
			psta = rtw_alloc_stainfo(pstapriv, (u8 *)mac, DTYPE, main_id, padapter_link->wrlink->id, PHL_CMD_WAIT);
			if (psta == NULL) {
				RTW_INFO("[%s] Alloc station for "MAC_FMT" fail\n", __FUNCTION__, MAC_ARG(mac));
				ret = -EOPNOTSUPP;
				goto exit;
			}
			rtw_phl_link_mld_stainfo(pmld, psta->phl_sta);
		}
	}
#endif /* CONFIG_TDLS */

exit:
	return ret;
}

static int	cfg80211_rtw_del_station(struct wiphy *wiphy, struct net_device *ndev,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0))
	u8 *mac
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0))
	const u8 *mac
#else
	struct station_del_parameters *params
#endif
)
{
	int ret = 0;
	_list	*phead, *plist;
	u8 updated = _FALSE;
	const u8 *target_mac;
	struct sta_info *psta = NULL;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct sta_priv *pstapriv = &padapter->stapriv;
	bool is_ucast = 1;
	u8 use_disassoc = _FALSE;
	u16 reason_code = WLAN_REASON_DEAUTH_LEAVING;
	u8 ignore_del = _FALSE;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0))
	target_mac = mac;
#else
	target_mac = params->mac;
	reason_code = params->reason_code;
	if (params->subtype == (WIFI_DISASSOC >> 4))
		use_disassoc = _TRUE;
#endif

	RTW_INFO("+"FUNC_NDEV_FMT" mac=%pM\n", FUNC_NDEV_ARG(ndev), target_mac);

	if (check_fwstate(pmlmepriv, (WIFI_ASOC_STATE | WIFI_AP_STATE | WIFI_MESH_STATE)) != _TRUE) {
		RTW_INFO("%s, fw_state != FW_LINKED|WIFI_AP_STATE|WIFI_MESH_STATE\n", __func__);
		return -EINVAL;
	}


	if (!target_mac) {
		RTW_INFO("flush all sta, and cam_entry\n");

		flush_all_cam_entry(padapter, PHL_CMD_WAIT, 50);	/* clear CAM */

#ifdef CONFIG_AP_MODE
		ret = rtw_sta_flush(padapter, _TRUE);
#endif
		return ret;
	}


	RTW_INFO("free sta macaddr =" MAC_FMT "\n", MAC_ARG(target_mac));

	/* broadcast deauth */
	if (is_broadcast_mac_addr(target_mac)) {
		if (MLME_IS_AP(padapter) && MLME_IS_ASOC(padapter)) {
			is_ucast = 0;
			issue_deauth(padapter, (u8 *)target_mac, WLAN_REASON_PREV_AUTH_NOT_VALID);
		}
		else {
			return -EINVAL;
		}
	}

	rtw_stapriv_asoc_list_lock(pstapriv);

	phead = &pstapriv->asoc_list;
	plist = get_next(phead);

	/* check asoc_queue */
	while ((rtw_end_of_queue_search(phead, plist)) == _FALSE) {
		psta = LIST_CONTAINOR(plist, struct sta_info, asoc_list);

		plist = get_next(plist);

		if ((_rtw_memcmp((u8 *)target_mac, psta->phl_sta->mac_addr, ETH_ALEN) || is_ucast == 0)
			&& !_rtw_memcmp((u8 *)target_mac, padapter->mac_addr, ETH_ALEN)
			&& !is_broadcast_mac_addr(psta->phl_sta->mac_addr)) {

			if (ATOMIC_READ(&psta->deleting) == 0) {
				ATOMIC_INC(&psta->deleting);
				RTW_INFO("psta->deleting=%u, block auth\n", ATOMIC_READ(&psta->deleting));
			} else {
				ignore_del = _TRUE;
				continue;
			}

			if (psta->dot8021xalg == 1 && psta->bpairwise_key_installed == _FALSE) {
				RTW_INFO("%s, sta's dot8021xalg = 1 and key_installed = _FALSE\n", __func__);

				if (MLME_IS_AP(padapter)) {
					rtw_stapriv_asoc_list_del(pstapriv, psta);
					RTW_INFO("%s, asoc_list_cnt=%d\n", __func__, pstapriv->asoc_list_cnt);

					ap_free_sta(padapter, psta, is_ucast, WLAN_REASON_IEEE_802_1X_AUTH_FAILED, _TRUE, _FALSE);
					psta = NULL;

					if (is_ucast)
						break;
				}
			} else {
				RTW_INFO("free psta=%p, aid=%d\n", psta, psta->phl_sta->aid);
				rtw_stapriv_asoc_list_del(pstapriv, psta);
				RTW_INFO("%s, asoc_list_cnt=%d\n", __func__, pstapriv->asoc_list_cnt);

				/* rtw_stapriv_asoc_list_unlock(pstapriv); */
				if (MLME_IS_AP(padapter))
					updated |= ap_free_sta(padapter, psta, is_ucast, reason_code, _TRUE, use_disassoc);
				else
					updated |= ap_free_sta(padapter, psta, is_ucast, WLAN_REASON_DEAUTH_LEAVING, _TRUE, _FALSE);
				/* rtw_stapriv_asoc_list_lock(pstapriv); */

				psta = NULL;

				if (is_ucast)
					break;
			}
		}
	}

	rtw_stapriv_asoc_list_unlock(pstapriv);

	if (ignore_del == _FALSE)
		associated_clients_update(padapter, updated, STA_INFO_UPDATE_ALL);

#ifdef CONFIG_RTW_MESH
	if (MLME_IS_MESH(padapter))
		rtw_mesh_plink_del(padapter, target_mac);
#endif

	RTW_INFO("-"FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	return ret;

}

static int cfg80211_rtw_change_station(struct wiphy *wiphy,
	struct net_device *ndev,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0))
	u8 *mac,
#else
	const u8 *mac,
#endif
	struct station_parameters *params)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(ndev);
	int ret = 0;

	RTW_INFO(FUNC_ADPT_FMT" mac:"MAC_FMT"\n", FUNC_ADPT_ARG(adapter), MAC_ARG(mac));

	dump_station_parameters(RTW_DBGDUMP, wiphy, params);

#ifdef CONFIG_RTW_MESH
	if (MLME_IS_MESH(adapter)) {
		enum cfg80211_station_type sta_type = CFG80211_STA_MESH_PEER_USER;
		u8 plink_state = nl80211_plink_state_to_rtw_plink_state(params->plink_state);

		ret = cfg80211_check_station_change(wiphy, params, sta_type);
		if (ret) {
			RTW_INFO("cfg80211_check_station_change return %d\n", ret);
			goto exit;
		}

		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		if (!(params->sta_modify_mask & STATION_PARAM_APPLY_PLINK_STATE))
			goto exit;
		#endif

		if (rtw_mesh_set_plink_state_cmd(adapter, mac, plink_state) != _SUCCESS)
			ret = -ENOENT;
	}

exit:
#endif /* CONFIG_RTW_MESH */

	if (ret)
		RTW_INFO(FUNC_ADPT_FMT" mac:"MAC_FMT" ret:%d\n",
			FUNC_ADPT_ARG(adapter), MAC_ARG(mac), ret);
	return ret;
}

struct sta_info *rtw_sta_info_get_by_idx(struct sta_priv *pstapriv, const int idx, u8 *asoc_list_num)
{
	_list	*phead, *plist;
	struct sta_info *psta = NULL;
	int i = 0;

	phead = &pstapriv->asoc_list;
	plist = get_next(phead);

	/* check asoc_queue */
	while ((rtw_end_of_queue_search(phead, plist)) == _FALSE) {
		if (idx == i)
			psta = LIST_CONTAINOR(plist, struct sta_info, asoc_list);
		plist = get_next(plist);
		i++;
	}

	if (asoc_list_num)
		*asoc_list_num = i;

	return psta;
}

static int	cfg80211_rtw_dump_station(struct wiphy *wiphy, struct net_device *ndev,
		int idx, u8 *mac, struct station_info *sinfo)
{
#define DBG_DUMP_STATION 0

	int ret = 0;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct sta_priv *pstapriv = &padapter->stapriv;
	struct sta_info *psta = NULL;
#ifdef CONFIG_RTW_MESH
	struct mesh_plink_ent *plink = NULL;
#endif
	u8 asoc_list_num;

	if (DBG_DUMP_STATION)
		RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	rtw_stapriv_asoc_list_lock(pstapriv);
	psta = rtw_sta_info_get_by_idx(pstapriv, idx, &asoc_list_num);
	rtw_stapriv_asoc_list_unlock(pstapriv);

#ifdef CONFIG_RTW_MESH
	if (MLME_IS_MESH(padapter)) {
		if (psta)
			plink = psta->plink;
		if (!plink)
			plink = rtw_mesh_plink_get_no_estab_by_idx(padapter, idx - asoc_list_num);
	}
#endif /* CONFIG_RTW_MESH */

	if ((!MLME_IS_MESH(padapter) && !psta)
		#ifdef CONFIG_RTW_MESH
		|| (MLME_IS_MESH(padapter) && !plink)
		#endif
	) {
		if (DBG_DUMP_STATION)
			RTW_INFO(FUNC_NDEV_FMT" end with idx:%d\n", FUNC_NDEV_ARG(ndev), idx);
		ret = -ENOENT;
		goto exit;
	}

	if (psta)
		_rtw_memcpy(mac, psta->phl_sta->mac_addr, ETH_ALEN);
	#ifdef CONFIG_RTW_MESH
	else
		_rtw_memcpy(mac, plink->addr, ETH_ALEN);
	#endif
	
	sinfo->filled = 0;

	if (psta) {
		sinfo->filled |= STATION_INFO_SIGNAL;
		/* ToDo: need API to query hal_sta->rssi_stat.rssi */
		/* sinfo->signal = rtw_phl_rssi_to_dbm(psta->phl_sta->rssi_stat.rssi); */
		sinfo->filled |= STATION_INFO_INACTIVE_TIME;
		sinfo->inactive_time = rtw_get_passing_time_ms(psta->sta_stats.last_rx_time);
	}

#ifdef CONFIG_RTW_MESH
	if (MLME_IS_MESH(padapter))
		rtw_cfg80211_fill_mesh_only_sta_info(plink, psta, sinfo);
#endif

exit:
	return ret;
}

static int	cfg80211_rtw_change_bss(struct wiphy *wiphy, struct net_device *ndev,
		struct bss_parameters *params)
{
	_adapter *adapter = rtw_netdev_priv(ndev);

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	if (params->ap_isolate != -1) {
		RTW_INFO("ap_isolate=%d\n", params->ap_isolate);
		adapter->mlmepriv.ap_isolate = params->ap_isolate ? 1 : 0;
	}
#endif

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29))
static int	cfg80211_rtw_set_txq_params(struct wiphy *wiphy
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	, struct net_device *ndev
#endif
	, struct ieee80211_txq_params *params)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	_adapter *padapter = rtw_netdev_priv(ndev);
#else
	_adapter *padapter = wiphy_to_adapter(wiphy);
#endif
	u8	ac, AIFS, ECWMin, ECWMax, aSifsTime, vo_cw;
	u16	TXOP;
	u8	shift_count = 0;
	u32	acParm;
	struct registry_priv *pregpriv = &padapter->registrypriv;
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	struct link_mlme_ext_priv	*pmlmeext = &padapter_link->mlmeextpriv;
	struct link_mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
	ac = params->ac;
#else
	ac = params->queue;
#endif

	switch (ac) {
	case NL80211_TXQ_Q_VO:
		ac = XMIT_VO_QUEUE;
		vo_cw = (pregpriv->vo_edca >> 8) & 0xff;
		if(vo_cw) {
			/* change vo contention window */
			params->cwmin = vo_cw & 0xf;
			params->cwmax = vo_cw >> 4;
		}
		break;

	case NL80211_TXQ_Q_VI:
		ac = XMIT_VI_QUEUE;
		break;

	case NL80211_TXQ_Q_BE:
		ac = XMIT_BE_QUEUE;
		break;

	case NL80211_TXQ_Q_BK:
		ac = XMIT_BK_QUEUE;
		break;

	default:
		break;
	}

#if 0
	RTW_INFO("ac=%d\n", ac);
	RTW_INFO("txop=%u\n", params->txop);
	RTW_INFO("cwmin=%u\n", params->cwmin);
	RTW_INFO("cwmax=%u\n", params->cwmax);
	RTW_INFO("aifs=%u\n", params->aifs);
#endif

	if (WIFI_ROLE_LINK_IS_ON_5G(padapter_link) ||
	    (pmlmeext->cur_wireless_mode & WLAN_MD_11N))
		aSifsTime = 16;
	else
		aSifsTime = 10;

	AIFS = params->aifs * pmlmeinfo->slotTime + aSifsTime;

	while ((params->cwmin + 1) >> shift_count != 1) {
		shift_count++;
		if (shift_count == 15)
			break;
	}

	ECWMin = shift_count;

	shift_count = 0;
	while ((params->cwmax + 1) >> shift_count != 1) {
		shift_count++;
		if (shift_count == 15)
			break;
	}

	ECWMax = shift_count;

	TXOP = params->txop;

	acParm = AIFS | (ECWMin << 8) | (ECWMax << 12) | (TXOP << 16);

	if (ac == XMIT_BE_QUEUE) {
		padapter->last_edca = acParm;
		acParm = rtw_get_turbo_edca(padapter, AIFS, ECWMin, ECWMax, TXOP);
		if (acParm)
			padapter->last_edca = acParm;
		else
			acParm = padapter->last_edca;
	}

	set_txq_params_cmd(padapter, padapter_link, acParm, ac);

	return 0;
}
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))
static int	cfg80211_rtw_set_channel(struct wiphy *wiphy
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	, struct net_device *ndev
	#endif
	, struct ieee80211_channel *chan, enum nl80211_channel_type channel_type)
{
	_adapter *padapter;
	struct _ADAPTER_LINK *padapter_link;
	struct rtw_chan_def chdef = {0};
	enum band_type chan_band = nl80211_band_to_rtw_band(chan->band);
	int chan_target = (u8) ieee80211_frequency_to_channel(chan->center_freq);
	int chan_offset = CHAN_OFFSET_NO_EXT;
	int chan_width = CHANNEL_WIDTH_20;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	if (ndev) {
		RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));
		padapter = (_adapter *)rtw_netdev_priv(ndev);
	} else
#endif
		padapter = wiphy_to_adapter(wiphy);

	padapter_link = GET_PRIMARY_LINK(padapter);

	switch (channel_type) {
	case NL80211_CHAN_NO_HT:
	case NL80211_CHAN_HT20:
		chan_width = CHANNEL_WIDTH_20;
		chan_offset = CHAN_OFFSET_NO_EXT;
		break;
	case NL80211_CHAN_HT40MINUS:
		chan_width = CHANNEL_WIDTH_40;
		chan_offset = CHAN_OFFSET_LOWER;
		break;
	case NL80211_CHAN_HT40PLUS:
		chan_width = CHANNEL_WIDTH_40;
		chan_offset = CHAN_OFFSET_UPPER;
		break;
	default:
		chan_width = CHANNEL_WIDTH_20;
		chan_offset = CHAN_OFFSET_NO_EXT;
		break;
	}

	RTW_INFO(FUNC_ADPT_FMT" band:%d, ch:%d, bw:%d, offset:%d\n"
		, FUNC_ADPT_ARG(padapter), chan_band, chan_target, chan_width, chan_offset);

	chdef.band = chan_band;
	chdef.chan = chan_target;
	chdef.bw = chan_width;
	chdef.offset = chan_offset;

	rtw_set_chbw_cmd(padapter, padapter_link, &chdef, RTW_CMDF_WAIT_ACK, RFK_TYPE_FORCE_NOT_DO);

	return 0;
}
#endif /*#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))*/

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
static void rtw_get_chbwoff_from_cfg80211_chan_def(
	struct cfg80211_chan_def *chandef,
	u8 *ht, struct rtw_chan_def *rtw_chdef)
{
	struct ieee80211_channel *chan = chandef->chan;

	rtw_chdef->chan = chan->hw_value;
	rtw_chdef->band = nl80211_band_to_rtw_band(chan->band);
	*ht = 1;

	switch (chandef->width) {
	case NL80211_CHAN_WIDTH_20_NOHT:
		*ht = 0;
		fallthrough;
	case NL80211_CHAN_WIDTH_20:
		rtw_chdef->bw = CHANNEL_WIDTH_20;
		rtw_chdef->offset = CHAN_OFFSET_NO_EXT;
		break;
	case NL80211_CHAN_WIDTH_40:
		rtw_chdef->bw = CHANNEL_WIDTH_40;
		rtw_chdef->offset = (chandef->center_freq1 > chan->center_freq)
				? CHAN_OFFSET_UPPER
				: CHAN_OFFSET_LOWER;
		break;
	case NL80211_CHAN_WIDTH_80:
		rtw_chdef->bw = CHANNEL_WIDTH_80;
		rtw_chdef->offset = (chandef->center_freq1 > chan->center_freq)
				? CHAN_OFFSET_UPPER
				: CHAN_OFFSET_LOWER;
		break;
	case NL80211_CHAN_WIDTH_160:
		rtw_chdef->bw = CHANNEL_WIDTH_160;
		rtw_chdef->offset = (chandef->center_freq1 > chan->center_freq)
				? CHAN_OFFSET_UPPER
				: CHAN_OFFSET_LOWER;
		break;
	case NL80211_CHAN_WIDTH_80P80:
		rtw_chdef->bw = CHANNEL_WIDTH_80_80;
		rtw_chdef->offset = (chandef->center_freq1 > chan->center_freq)
				? CHAN_OFFSET_UPPER
				: CHAN_OFFSET_LOWER;
		break;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	case NL80211_CHAN_WIDTH_5:
		rtw_chdef->bw = CHANNEL_WIDTH_5;
		rtw_chdef->offset = CHAN_OFFSET_NO_EXT;
		break;
	case NL80211_CHAN_WIDTH_10:
		rtw_chdef->bw = CHANNEL_WIDTH_10;
		rtw_chdef->offset = CHAN_OFFSET_NO_EXT;
		break;
#endif
	default:
		*ht = 0;
		rtw_chdef->bw = CHANNEL_WIDTH_20;
		rtw_chdef->offset = CHAN_OFFSET_NO_EXT;
		RTW_INFO("unsupported cwidth:%u\n", chandef->width);
		rtw_warn_on(1);
	};
}
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)) */

static int cfg80211_rtw_set_monitor_channel(struct wiphy *wiphy
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	, struct cfg80211_chan_def *chandef
#else
	, struct ieee80211_channel *chan
	, enum nl80211_channel_type channel_type
#endif
	)
{
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct rtw_chan_def target_chdef = {0};
	u8 ht_option;
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO("center_freq %u Mhz band %u ch %u width %u freq1 %u freq2 %u\n"
		, chandef->chan->center_freq
		, chandef->chan->band
		, chandef->chan->hw_value
		, chandef->width
		, chandef->center_freq1
		, chandef->center_freq2);
#endif /* CONFIG_DEBUG_CFG80211 */

	rtw_get_chbwoff_from_cfg80211_chan_def(chandef,
		&ht_option, &target_chdef);
#else
#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO("center_freq %u Mhz ch %u channel_type %u\n"
		, chan->center_freq
		, chan->hw_value
		, channel_type);
#endif /* CONFIG_DEBUG_CFG80211 */

	rtw_get_chdef_from_nl80211_channel_type(chan, channel_type,
		&ht_option, &target_chdef);
#endif
	RTW_INFO(FUNC_ADPT_FMT" band:%d, ch:%d, bw:%d, offset:%d\n",
		FUNC_ADPT_ARG(padapter), target_chdef.band, target_chdef.chan,
		target_chdef.bw, target_chdef.offset);

	rtw_set_chbw_cmd(padapter, padapter_link,
		&target_chdef, RTW_CMDF_WAIT_ACK, RFK_TYPE_FORCE_NOT_DO);

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
static int cfg80211_rtw_get_channel(struct wiphy *wiphy,
	struct wireless_dev *wdev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 2)) || defined(CONFIG_MLD_KERNEL_PATCH)
	unsigned int link_id,
#endif
	struct cfg80211_chan_def *chandef)
{
	_adapter *a = (_adapter *)rtw_netdev_priv(wdev_to_ndev(wdev));
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *a_link = GET_PRIMARY_LINK(a);
	struct link_mlme_ext_priv *mlmeext = &(a_link->mlmeextpriv);
	u8 ht_option = 0;
	int ret = _FAIL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 2)) || defined(CONFIG_MLD_KERNEL_PATCH)
	RTW_INFO(FUNC_ADPT_FMT" link_id:%d\n", FUNC_ADPT_ARG(a), link_id);
#endif

#ifdef CONFIG_80211N_HT
	ht_option = a_link->mlmepriv.htpriv.ht_option;
#endif /* CONFIG_80211N_HT */

	ret = rtw_chdef_to_cfg80211_chan_def(wiphy, chandef,
			&mlmeext->chandef, ht_option);

	if (ret == _FAIL)
		return -ENODATA;
	else
		return 0;
}
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)) */

/*
static int	cfg80211_rtw_auth(struct wiphy *wiphy, struct net_device *ndev,
		struct cfg80211_auth_request *req)
{
	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	return 0;
}

static int	cfg80211_rtw_assoc(struct wiphy *wiphy, struct net_device *ndev,
		struct cfg80211_assoc_request *req)
{
	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	return 0;
}
*/
#endif /* CONFIG_AP_MODE */

void rtw_cfg80211_external_auth_request(_adapter *padapter, union recv_frame *rframe)
{
	struct rtw_external_auth_params params;
	struct wireless_dev *wdev = padapter->rtw_wdev;
	struct net_device *netdev = wdev_to_ndev(wdev);
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	struct link_mlme_ext_priv *pmlmeext = &(padapter_link->mlmeextpriv);
	struct link_mlme_ext_info *pmlmeinfo = &(pmlmeext->mlmext_info);

	u8 frame[256] = { 0 };
	uint frame_len = 24;
	s32 freq = 0;

	/* rframe, in this case is null point */
	freq = rtw_bch2freq(pmlmeext->chandef.band,
				pmlmeext->chandef.chan);

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO(FUNC_ADPT_FMT": freq=%d channel=%d\n", FUNC_ADPT_ARG(padapter), freq, padapter_link->mlmeextpriv.chandef.chan);
#endif

#if (KERNEL_VERSION(4, 17, 0) <= LINUX_VERSION_CODE) \
    || defined(CONFIG_KERNEL_PATCH_EXTERNAL_AUTH)
	params.action = EXTERNAL_AUTH_START;
	_rtw_memcpy(params.bssid, get_my_bssid(&pmlmeinfo->network), ETH_ALEN);
	params.ssid.ssid_len = pmlmeinfo->network.Ssid.SsidLength;
	_rtw_memcpy(params.ssid.ssid, pmlmeinfo->network.Ssid.Ssid,
		pmlmeinfo->network.Ssid.SsidLength);
	params.key_mgmt_suite = 0x8ac0f00;

	RTW_INFO("external auth: use kernel API: cfg80211_external_auth_request()\n");
	cfg80211_external_auth_request(netdev,
		(struct cfg80211_external_auth_params *)&params, GFP_ATOMIC);
#elif (KERNEL_VERSION(2, 6, 37) <= LINUX_VERSION_CODE)
	set_frame_sub_type(frame, WIFI_AUTH);

	_rtw_memcpy(frame + 4, get_my_bssid(&pmlmeinfo->network), ETH_ALEN);
	_rtw_memcpy(frame + 10, padapter_link->mac_addr, ETH_ALEN);
	_rtw_memcpy(frame + 16, get_my_bssid(&pmlmeinfo->network), ETH_ALEN);
	RTW_PUT_LE32((frame + 18), 0x8ac0f00);
	RTW_PUT_LE32((frame + 24), 0x0003);

	if (pmlmeinfo->network.Ssid.SsidLength) {
		*(frame + 26) = pmlmeinfo->network.Ssid.SsidLength;
		_rtw_memcpy(frame + 27, pmlmeinfo->network.Ssid.Ssid,
			pmlmeinfo->network.Ssid.SsidLength);
		frame_len = 27 + pmlmeinfo->network.Ssid.SsidLength;
	}

	RTW_INFO("external auth: with wpa_supplicant patch\n");
	rtw_cfg80211_rx_mgmt(wdev, freq, 0, frame, frame_len, GFP_ATOMIC);
#endif
}

void rtw_cfg80211_rx_probe_request(_adapter *adapter, union recv_frame *rframe)
{
	struct wireless_dev *wdev = NULL;
	struct wiphy *wiphy = NULL;
	u8 *frame = get_recvframe_data(rframe);
	uint frame_len = rframe->u.hdr.len;
	s32 freq;
	struct _ADAPTER_LINK *adapter_link = rframe->u.hdr.adapter_link;
	u8 ch, sch = rtw_get_oper_ch(adapter, adapter_link);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	wiphy = adapter->rtw_wdev->wiphy;
#if defined(RTW_DEDICATED_P2P_DEVICE)
	wdev = wiphy_to_pd_wdev(wiphy);
#endif
#endif

	if (wdev == NULL)
		wdev = adapter->rtw_wdev;

	ch = rframe->u.hdr.attrib.ch ? rframe->u.hdr.attrib.ch : sch;
	freq = rtw_ch2freq(ch);

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO("RTW_Rx: probe request, ch=%d(%d), ta="MAC_FMT"\n"
		, ch, sch, MAC_ARG(get_addr2_ptr(frame)));
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)) || defined(COMPAT_KERNEL_RELEASE)
	rtw_cfg80211_rx_mgmt(wdev, freq, 0, frame, frame_len, GFP_ATOMIC);
#else
	cfg80211_rx_action(adapter->pnetdev, freq, frame, frame_len, GFP_ATOMIC);
#endif
}

void rtw_cfg80211_rx_action_p2p(_adapter *adapter, union recv_frame *rframe)
{
	struct wireless_dev *wdev = adapter->rtw_wdev;
	u8 *frame = get_recvframe_data(rframe);
	uint frame_len = rframe->u.hdr.len;
	s32 freq;
	struct _ADAPTER_LINK *adapter_link = rframe->u.hdr.adapter_link;
	enum band_type band = rtw_get_oper_band(adapter, adapter_link);
	u8 ch, sch = rtw_get_oper_ch(adapter, adapter_link);
	u8 category, action;
	int type;

	ch = rframe->u.hdr.attrib.ch ? rframe->u.hdr.attrib.ch : sch;
	freq = rtw_bch2freq(band, ch);

	RTW_INFO("RTW_Rx:band=%d, ch=%d(%d), ta="MAC_FMT"\n"
		, band, ch, sch, MAC_ARG(get_addr2_ptr(frame)));
#ifdef CONFIG_P2P
	type = rtw_p2p_check_frames(adapter, frame, frame_len, _FALSE);
	if (type >= 0)
		goto indicate;
#endif
	rtw_action_frame_parse(frame, frame_len, &category, &action);
	RTW_INFO("RTW_Rx:category(%u), action(%u)\n", category, action);
#ifdef CONFIG_P2P
indicate:
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	rtw_cfg80211_rx_mgmt(wdev, freq, 0, frame, frame_len, GFP_ATOMIC);
#else
	cfg80211_rx_action(adapter->pnetdev, freq, frame, frame_len, GFP_ATOMIC);
#endif
}

void rtw_cfg80211_rx_p2p_action_public(_adapter *adapter, union recv_frame *rframe)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	struct wireless_dev *wdev = adapter->rtw_wdev;
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(adapter);
	u8 *frame = get_recvframe_data(rframe);
	uint frame_len = rframe->u.hdr.len;
	s32 freq;
	struct _ADAPTER_LINK *adapter_link = rframe->u.hdr.adapter_link;
	enum band_type band = rtw_get_oper_band(adapter, adapter_link);
	u8 ch, sch = rtw_get_oper_ch(adapter, adapter_link);
	u8 category, action;
	int type;

	ch = rframe->u.hdr.attrib.ch ? rframe->u.hdr.attrib.ch : sch;
	freq = rtw_bch2freq(band, ch);

	RTW_INFO("RTW_Rx:band=%d, ch=%d(%d), ta="MAC_FMT"\n"
		, band, ch, sch, MAC_ARG(get_addr2_ptr(frame)));
	#ifdef CONFIG_P2P
	type = rtw_p2p_check_frames(adapter, frame, frame_len, _FALSE);
	if (type >= 0) {
		switch (type) {
		case P2P_GO_NEGO_CONF:
			if (0) {
				RTW_INFO(FUNC_ADPT_FMT" Nego confirm. state=%u, status=%u, iaddr="MAC_FMT"\n"
					, FUNC_ADPT_ARG(adapter), pwdev_priv->nego_info.state, pwdev_priv->nego_info.status
					, MAC_ARG(pwdev_priv->nego_info.iface_addr));
			}
			if (pwdev_priv->nego_info.state == 2
				&& pwdev_priv->nego_info.status == 0
				&& rtw_check_invalid_mac_address(pwdev_priv->nego_info.iface_addr, _FALSE) == _FALSE
			) {
				_adapter *intended_iface = dvobj_get_adapter_by_addr(dvobj, pwdev_priv->nego_info.iface_addr);

				if (intended_iface) {
					RTW_INFO(FUNC_ADPT_FMT" Nego confirm. Allow only "ADPT_FMT" to scan for 2000 ms\n"
						, FUNC_ADPT_ARG(adapter), ADPT_ARG(intended_iface));
					/* allow only intended_iface to do scan for 2000 ms */
					rtw_mi_set_scan_deny(adapter, 2000);
					rtw_clear_scan_deny(intended_iface);
				}
			}
			break;
		case P2P_PROVISION_DISC_RESP:
		case P2P_INVIT_RESP:
			rtw_clear_scan_deny(adapter);
			#if !RTW_P2P_GROUP_INTERFACE
			rtw_mi_buddy_set_scan_deny(adapter, 2000);
			#endif
			break;
		}
		goto indicate;
	}
	#endif
	rtw_action_frame_parse(frame, frame_len, &category, &action);
	RTW_INFO("RTW_Rx:category(%u), action(%u)\n", category, action);
#ifdef CONFIG_P2P
indicate:
#endif
	#if defined(RTW_DEDICATED_P2P_DEVICE)
	if (rtw_cfg80211_redirect_pd_wdev(dvobj_to_wiphy(dvobj), get_ra(frame), &wdev))
		if (0)
			RTW_INFO("redirect to pd_wdev:%p\n", wdev);
	#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	rtw_cfg80211_rx_mgmt(wdev, freq, 0, frame, frame_len, GFP_ATOMIC);
#else
	cfg80211_rx_action(adapter->pnetdev, freq, frame, frame_len, GFP_ATOMIC);
#endif
}

void rtw_cfg80211_rx_action(_adapter *adapter, union recv_frame *rframe, const char *msg)
{
	struct wireless_dev *wdev = adapter->rtw_wdev;
	u8 *frame = get_recvframe_data(rframe);
	uint frame_len = rframe->u.hdr.len;
	s32 freq;
	struct _ADAPTER_LINK *adapter_link = rframe->u.hdr.adapter_link;
	enum band_type band = rtw_get_oper_band(adapter, adapter_link);
	u8 ch, sch = rtw_get_oper_ch(adapter, adapter_link);
	u8 category, action;
	int type = -1;

	ch = rframe->u.hdr.attrib.ch ? rframe->u.hdr.attrib.ch : sch;
	freq = rtw_bch2freq(band, ch);

	RTW_INFO("RTW_Rx:band=%d, ch=%d(%d), ta="MAC_FMT"\n"
		, band, ch, sch, MAC_ARG(get_addr2_ptr(frame)));

#ifdef CONFIG_RTW_MESH
	if (MLME_IS_MESH(adapter)) {
		type = rtw_mesh_check_frames_rx(adapter, frame, frame_len);
		if (type >= 0)
			goto indicate;
	}
#endif
	rtw_action_frame_parse(frame, frame_len, &category, &action);
	if (category == RTW_WLAN_CATEGORY_PUBLIC) {
		if (action == ACT_PUBLIC_GAS_INITIAL_REQ) {
			rtw_mi_set_scan_deny(adapter, 200);
			rtw_mi_scan_abort(adapter, _FALSE); /*rtw_scan_abort_no_wait*/
		}
	}
#ifdef CONFIG_RTW_MESH
indicate:
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	rtw_cfg80211_rx_mgmt(wdev, freq, 0, frame, frame_len, GFP_ATOMIC);
#else
	cfg80211_rx_action(adapter->pnetdev, freq, frame, frame_len, GFP_ATOMIC);
#endif

	if (type == -1) {
		if (msg)
			RTW_INFO("RTW_Rx:%s\n", msg);
		else
			RTW_INFO("RTW_Rx:category(%u), action(%u)\n", category, action);
	}
}

#ifdef CONFIG_RTW_80211K
void rtw_cfg80211_rx_rrm_action(_adapter *adapter, union recv_frame *rframe)
{
	struct wireless_dev *wdev = adapter->rtw_wdev;
	u8 *frame = get_recvframe_data(rframe);
	uint frame_len = rframe->u.hdr.len;
	s32 freq;
	struct _ADAPTER_LINK *adapter_link = rframe->u.hdr.adapter_link;
	u8 ch, sch = rtw_get_oper_ch(adapter, adapter_link);

	ch = rframe->u.hdr.attrib.ch ? rframe->u.hdr.attrib.ch : sch;
	freq = rtw_ch2freq(ch);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	rtw_cfg80211_rx_mgmt(wdev, freq, 0, frame, frame_len, GFP_ATOMIC);
#else
	cfg80211_rx_action(adapter->pnetdev, freq, frame, frame_len, GFP_ATOMIC);
#endif
	RTW_INFO("RTW_Rx:ch=%d(%d), ta="MAC_FMT"\n"
		, ch, sch, MAC_ARG(get_addr2_ptr(frame)));
}
#endif /* CONFIG_RTW_80211K */

void rtw_cfg80211_rx_mframe(_adapter *adapter, union recv_frame *rframe, const char *msg)
{
	struct wireless_dev *wdev = adapter->rtw_wdev;
	u8 *frame = get_recvframe_data(rframe);
	uint frame_len = rframe->u.hdr.len;
	s32 freq;
	struct _ADAPTER_LINK *adapter_link = rframe->u.hdr.adapter_link;
	u8 ch, sch = rtw_get_oper_ch(adapter, adapter_link);

	ch = rframe->u.hdr.attrib.ch ? rframe->u.hdr.attrib.ch : sch;
	freq = rtw_ch2freq(ch);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	rtw_cfg80211_rx_mgmt(wdev, freq, 0, frame, frame_len, GFP_ATOMIC);
#else
	cfg80211_rx_action(adapter->pnetdev, freq, frame, frame_len, GFP_ATOMIC);
#endif

	RTW_INFO("RTW_Rx:ch=%d(%d), ta="MAC_FMT"\n", ch, sch, MAC_ARG(get_addr2_ptr(frame)));
	if (!rtw_sae_preprocess(adapter, frame, frame_len, _FALSE)) {
		if (msg)
			RTW_INFO("RTW_Rx:%s\n", msg);
		else
			RTW_INFO("RTW_Rx:frame_control:0x%02x\n", le16_to_cpu(((struct rtw_ieee80211_hdr_3addr *)rframe)->frame_ctl));
	}
}

#ifdef CONFIG_P2P
#ifdef CONFIG_RTW_80211R
static s32 cfg80211_rtw_update_ft_ies(struct wiphy *wiphy,
	struct net_device *ndev,
	struct cfg80211_update_ft_ies_params *ftie)
{
	_adapter *padapter = NULL;
	struct mlme_priv *pmlmepriv = NULL;
	struct ft_roam_info *pft_roam = NULL;

	if (ndev == NULL)
		return  -EINVAL;

	padapter = (_adapter *)rtw_netdev_priv(ndev);
	pmlmepriv = &(padapter->mlmepriv);
	pft_roam = &(pmlmepriv->ft_roam);

#ifdef CONFIG_RTW_80211R_AP
	if (check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
		return rtw_ft_update_sta_ies(padapter, ftie);
#endif

	if (ftie->ie_len <= sizeof(pft_roam->updated_ft_ies)) {
		_rtw_spinlock_bh(&pmlmepriv->lock);
		_rtw_memcpy(pft_roam->updated_ft_ies, ftie->ie, ftie->ie_len);
		pft_roam->updated_ft_ies_len = ftie->ie_len;
		_rtw_spinunlock_bh(&pmlmepriv->lock);
	} else {
		RTW_ERR("FTIEs parsing fail!\n");
		return -EINVAL;
	}

	if (rtw_ft_roam_status(padapter, RTW_FT_AUTHENTICATED_STA)) {
		RTW_PRINT("auth success, start reassoc\n");
		rtw_ft_lock_set_status(padapter, RTW_FT_ASSOCIATING_STA);
		start_clnt_assoc(padapter);
	}

	return 0;
}
#endif

inline int rtw_cfg80211_iface_has_p2p_group_cap(_adapter *adapter)
{
#if RTW_P2P_GROUP_INTERFACE
	if (is_primary_adapter(adapter))
		return 0;
#endif
	return 1;
}

inline int rtw_cfg80211_is_p2p_scan(_adapter *adapter)
{
	#if defined(RTW_DEDICATED_P2P_DEVICE)
	if (wiphy_to_pd_wdev(adapter_to_wiphy(adapter))) /* pd_wdev exist */
		return rtw_cfg80211_is_scan_by_pd_wdev(adapter);
	#endif
	{
		/* Here are 2 cases:
		 * 1. RTW_DEDICATED_P2P_DEVICE defined but upper layer don't use pd_wdev or
		 * 2. RTW_DEDICATED_P2P_DEVICE not defined
		 * Both cases check whether the contents of scan_request is p2p scan.
		 */
		struct rtw_wdev_priv *wdev_data = adapter_wdev_data(adapter);
		int is_p2p_scan = 0;

		_rtw_spinlock_bh(&wdev_data->scan_req_lock);
		if (wdev_data->scan_request
			&& wdev_data->scan_request->n_ssids
			&& wdev_data->scan_request->ssids
			&& wdev_data->scan_request->ie
		) {
			if (_rtw_memcmp(wdev_data->scan_request->ssids[0].ssid, "DIRECT-", 7)
				&& rtw_get_p2p_ie((u8 *)wdev_data->scan_request->ie, wdev_data->scan_request->ie_len, NULL, NULL))
				is_p2p_scan = 1;
		}
		_rtw_spinunlock_bh(&wdev_data->scan_req_lock);

		return is_p2p_scan;
	}
}

#if defined(RTW_DEDICATED_P2P_DEVICE)
int rtw_pd_iface_alloc(struct wiphy *wiphy, const char *name, struct wireless_dev **pd_wdev)
{
	struct rtw_wiphy_data *wiphy_data = rtw_wiphy_priv(wiphy);
	struct wireless_dev *wdev = NULL;
	struct rtw_netdev_priv_indicator *npi;
	_adapter *primary_adpt = wiphy_to_adapter(wiphy);
	int ret = 0;

	if (wiphy_data->pd_wdev) {
		RTW_WARN(FUNC_WIPHY_FMT" pd_wdev already exists\n", FUNC_WIPHY_ARG(wiphy));
		ret = -EBUSY;
		goto exit;
	}

	wdev = (struct wireless_dev *)rtw_zmalloc(sizeof(struct wireless_dev));
	if (!wdev) {
		RTW_WARN(FUNC_WIPHY_FMT" allocate wdev fail\n", FUNC_WIPHY_ARG(wiphy));
		ret = -ENOMEM;
		goto exit;
	}

	wdev->wiphy = wiphy;
	wdev->iftype = NL80211_IFTYPE_P2P_DEVICE;
	_rtw_memcpy(wdev->address, adapter_mac_addr(primary_adpt), ETH_ALEN);

	wiphy_data->pd_wdev = wdev;
	*pd_wdev = wdev;

	RTW_INFO(FUNC_WIPHY_FMT" pd_wdev:%p, addr="MAC_FMT" added\n"
		, FUNC_WIPHY_ARG(wiphy), wdev, MAC_ARG(wdev_address(wdev)));

exit:
	if (ret && wdev) {
		rtw_mfree((u8 *)wdev, sizeof(struct wireless_dev));
		wdev = NULL;
	}

	return ret;
}

void rtw_pd_iface_free(struct wiphy *wiphy)
{
	struct dvobj_priv *dvobj = wiphy_to_dvobj(wiphy);
	struct rtw_wiphy_data *wiphy_data = rtw_wiphy_priv(wiphy);
	u8 rtnl_lock_needed;

	if (!wiphy_data->pd_wdev)
		goto exit;

	RTW_INFO(FUNC_WIPHY_FMT" pd_wdev:%p, addr="MAC_FMT"\n"
		, FUNC_WIPHY_ARG(wiphy), wiphy_data->pd_wdev
		, MAC_ARG(wdev_address(wiphy_data->pd_wdev)));

	rtnl_lock_needed = rtw_rtnl_lock_needed(dvobj);
	if (rtnl_lock_needed)
		rtnl_lock();
	cfg80211_unregister_wdev(wiphy_data->pd_wdev);
	if (rtnl_lock_needed)
		rtnl_unlock();

	rtw_mfree((u8 *)wiphy_data->pd_wdev, sizeof(struct wireless_dev));
	wiphy_data->pd_wdev = NULL;

exit:
	return;
}

static int cfg80211_rtw_start_p2p_device(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	_adapter *adapter = wiphy_to_adapter(wiphy);

	RTW_INFO(FUNC_WIPHY_FMT" wdev=%p\n", FUNC_WIPHY_ARG(wiphy), wdev);

	rtw_p2p_enable(adapter, P2P_ROLE_DEVICE);
	return 0;
}

static void cfg80211_rtw_stop_p2p_device(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	_adapter *adapter = wiphy_to_adapter(wiphy);

	RTW_INFO(FUNC_WIPHY_FMT" wdev=%p\n", FUNC_WIPHY_ARG(wiphy), wdev);

	if (rtw_cfg80211_is_p2p_scan(adapter))
		rtw_scan_abort(adapter, 0);

	rtw_p2p_enable(adapter, P2P_ROLE_DISABLE);
}

inline int rtw_cfg80211_redirect_pd_wdev(struct wiphy *wiphy, u8 *ra, struct wireless_dev **wdev)
{
	struct wireless_dev *pd_wdev = wiphy_to_pd_wdev(wiphy);

	if (pd_wdev && pd_wdev != *wdev
		&& _rtw_memcmp(wdev_address(pd_wdev), ra, ETH_ALEN) == _TRUE
	) {
		*wdev = pd_wdev;
		return 1;
	}
	return 0;
}

inline int rtw_cfg80211_is_scan_by_pd_wdev(_adapter *adapter)
{
	struct wiphy *wiphy = adapter_to_wiphy(adapter);
	struct rtw_wdev_priv *wdev_data = adapter_wdev_data(adapter);
	struct wireless_dev *wdev = NULL;

	_rtw_spinlock_bh(&wdev_data->scan_req_lock);
	if (wdev_data->scan_request)
		wdev = wdev_data->scan_request->wdev;
	_rtw_spinunlock_bh(&wdev_data->scan_req_lock);

	if (wdev && wdev == wiphy_to_pd_wdev(wiphy))
		return 1;

	return 0;
}
#endif /* RTW_DEDICATED_P2P_DEVICE */
#endif /* CONFIG_P2P */

inline void rtw_cfg80211_set_is_roch(_adapter *adapter, bool val)
{
	adapter->cfg80211_rochinfo.is_ro_ch = val;
	rtw_mi_update_iface_status(&(adapter->mlmepriv), 0);
}

inline bool rtw_cfg80211_get_is_roch(_adapter *adapter)
{
	return adapter->cfg80211_rochinfo.is_ro_ch;
}

inline bool rtw_cfg80211_is_ro_ch_once(_adapter *adapter)
{
	return adapter->cfg80211_rochinfo.last_ro_ch_time ? 1 : 0;
}

inline void rtw_cfg80211_set_last_ro_ch_time(_adapter *adapter)
{
	adapter->cfg80211_rochinfo.last_ro_ch_time = rtw_get_current_time();

	if (!adapter->cfg80211_rochinfo.last_ro_ch_time)
		adapter->cfg80211_rochinfo.last_ro_ch_time++;
}

inline s32 rtw_cfg80211_get_last_ro_ch_passing_ms(_adapter *adapter)
{
	return rtw_get_passing_time_ms(adapter->cfg80211_rochinfo.last_ro_ch_time);
}

#ifndef CONFIG_CFG80211_REPORT_PROBE_REQ
static inline bool chk_is_p2p_device(_adapter *adapter, struct wireless_dev *wdev)
{
	struct wiphy *wiphy = adapter_to_wiphy(adapter);

#if defined(RTW_DEDICATED_P2P_DEVICE)
	if (wdev == wiphy_to_pd_wdev(wiphy))
		return _TRUE;
#else
	#if defined(CONFIG_CONCURRENT_MODE) && !RTW_P2P_GROUP_INTERFACE
	if (adapter->iface_id == adapter->registrypriv.sel_p2p_iface)
	#endif
		return _TRUE;
#endif
	return _FALSE;
}
#endif

#if 1 /*CONFIG_PHL_ARCH*/
static s32 cfg80211_rtw_remain_on_channel(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct wireless_dev *wdev,
#else
	struct net_device *ndev,
#endif
	struct ieee80211_channel *channel,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
	enum nl80211_channel_type channel_type,
#endif
	unsigned int duration, u64 *cookie)
{
	s32 err = 0;
	u8 remain_ch = (u8) ieee80211_frequency_to_channel(channel->center_freq);
	_adapter *padapter = NULL;
	struct registry_priv  *pregistrypriv = NULL;
	//struct rtw_wdev_priv *pwdev_priv;
	struct cfg80211_roch_info *pcfg80211_rochinfo;
	struct back_op_param bkop_parm;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	u8 channel_type = 0;
#endif
	u8 is_p2p = _FALSE;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
#if defined(RTW_DEDICATED_P2P_DEVICE)
	if (wdev == wiphy_to_pd_wdev(wiphy))
		padapter = wiphy_to_adapter(wiphy);
	else
#endif
	if (wdev_to_ndev(wdev))
		padapter = (_adapter *)rtw_netdev_priv(wdev_to_ndev(wdev));
	else
		return -EINVAL;
#else
	struct wireless_dev *wdev;

	if (ndev == NULL)
		return -EINVAL;

	padapter = (_adapter *)rtw_netdev_priv(ndev);
	wdev = ndev_to_wdev(ndev);
#endif
	pregistrypriv = &padapter->registrypriv;
	pcfg80211_rochinfo = &padapter->cfg80211_rochinfo;

#ifndef CONFIG_CFG80211_REPORT_PROBE_REQ
#ifdef CONFIG_P2P
	is_p2p = chk_is_p2p_device(padapter, wdev);

	if (is_p2p) {
		struct wifidirect_info *pwdinfo = &padapter->wdinfo;

		if (rtw_p2p_chk_role(pwdinfo, P2P_ROLE_DISABLE))
			rtw_p2p_enable(padapter, P2P_ROLE_DEVICE);

		pwdinfo->listen_channel = remain_ch;
		RTW_INFO(FUNC_ADPT_FMT" init listen_channel %u\n",
			FUNC_ADPT_ARG(padapter),
			padapter->wdinfo.listen_channel);
	}
#endif
#endif

	*cookie = ATOMIC_INC_RETURN(&pcfg80211_rochinfo->ro_ch_cookie_gen);

	RTW_INFO(FUNC_ADPT_FMT"%s ch:%u duration:%d, cookie:0x%llx\n"
		, FUNC_ADPT_ARG(padapter), wdev == wiphy_to_pd_wdev(wiphy) ? " PD" : ""
		, remain_ch, duration, *cookie);

	if (rtw_chset_search_ch(adapter_to_chset(padapter), remain_ch) < 0) {
		RTW_WARN(FUNC_ADPT_FMT" invalid ch:%u\n", FUNC_ADPT_ARG(padapter), remain_ch);
		return -EFAULT;
	}

#ifdef CONFIG_MP_INCLUDED
	if (rtw_mp_mode_check(padapter)) {
		RTW_INFO("MP mode block remain_on_channel request\n");
		return -EFAULT;
	}
#endif
	if (adapter_to_pwrctl(padapter)->bInSuspend == _TRUE) {
		RTW_INFO("%s return -EBUSY since bInSuspend is TRUE\n", __func__);
		return -EBUSY;
	}

	rtw_scan_abort(padapter, 0);

	bkop_parm.off_ch_dur = pregistrypriv->roch_max_away_dur;
	bkop_parm.on_ch_dur = pregistrypriv->roch_min_home_dur;

	if (check_fwstate(&padapter->mlmepriv, WIFI_ASOC_STATE))
		bkop_parm.off_ch_ext_dur = pregistrypriv->roch_extend_dur;
	else
		bkop_parm.off_ch_ext_dur = pregistrypriv->roch_extend_dur * 6;

	rtw_phl_remain_on_ch_cmd(padapter, *cookie, wdev,
		channel, channel_type, duration, &bkop_parm, is_p2p);

	return 0;
}

#else /* !CONFIG_PHL_ARCH */

static s32 cfg80211_rtw_remain_on_channel(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct wireless_dev *wdev,
#else
	struct net_device *ndev,
#endif
	struct ieee80211_channel *channel,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
	enum nl80211_channel_type channel_type,
#endif
	unsigned int duration, u64 *cookie)
{
	s32 err = 0;
	u8 remain_ch = (u8) ieee80211_frequency_to_channel(channel->center_freq);
	_adapter *padapter = NULL;
	struct rtw_wdev_priv *pwdev_priv;
	struct wifidirect_info *pwdinfo;
	struct cfg80211_wifidirect_info *pcfg80211_wdinfo;
#ifdef CONFIG_CONCURRENT_MODE
	u8 is_p2p_find = _FALSE;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	#if defined(RTW_DEDICATED_P2P_DEVICE)
	if (wdev == wiphy_to_pd_wdev(wiphy))
		padapter = wiphy_to_adapter(wiphy);
	else
	#endif
	if (wdev_to_ndev(wdev))
		padapter = (_adapter *)rtw_netdev_priv(wdev_to_ndev(wdev));
	else {
		err = -EINVAL;
		goto exit;
	}
#else
	struct wireless_dev *wdev;

	if (ndev == NULL) {
		err = -EINVAL;
		goto exit;
	}
	padapter = (_adapter *)rtw_netdev_priv(ndev);
	wdev = ndev_to_wdev(ndev);
#endif

	pwdev_priv = adapter_wdev_data(padapter);
	pwdinfo = &padapter->wdinfo;
	pcfg80211_wdinfo = &padapter->cfg80211_wdinfo;
#ifdef CONFIG_CONCURRENT_MODE
	is_p2p_find = (duration < (pwdinfo->ext_listen_interval)) ? _TRUE : _FALSE;
#endif

	*cookie = ATOMIC_INC_RETURN(&pcfg80211_wdinfo->ro_ch_cookie_gen);

	RTW_INFO(FUNC_ADPT_FMT"%s ch:%u duration:%d, cookie:0x%llx\n"
		, FUNC_ADPT_ARG(padapter), wdev == wiphy_to_pd_wdev(wiphy) ? " PD" : ""
		, remain_ch, duration, *cookie);

	if (rtw_chset_search_ch(adapter_to_chset(padapter), remain_ch) < 0) {
		RTW_WARN(FUNC_ADPT_FMT" invalid ch:%u\n", FUNC_ADPT_ARG(padapter), remain_ch);
		err = -EFAULT;
		goto exit;
	}

#ifdef CONFIG_MP_INCLUDED
	if (rtw_mp_mode_check(padapter)) {
		RTW_INFO("MP mode block remain_on_channel request\n");
		err = -EFAULT;
		goto exit;
	}
#endif

	rtw_scan_abort(padapter);
#ifdef CONFIG_CONCURRENT_MODE
	/*don't scan_abort during p2p_listen.*/
	if (is_p2p_find)
		rtw_mi_buddy_scan_abort(padapter, _TRUE);
#endif /*CONFIG_CONCURRENT_MODE*/

	if (rtw_cfg80211_get_is_roch(padapter) == _TRUE) {
		_cancel_timer_ex(&padapter->cfg80211_wdinfo.remain_on_ch_timer);
		p2p_cancel_roch_cmd(padapter, 0, NULL, RTW_CMDF_WAIT_ACK);
	}

	/* if(!rtw_p2p_chk_role(pwdinfo, P2P_ROLE_CLIENT) && !rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO)) */
	if (rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE)
		#if defined(CONFIG_CONCURRENT_MODE) && defined(CONFIG_P2P)
		&& ((padapter->iface_id == padapter->registrypriv.sel_p2p_iface))
		#endif
	) {
		rtw_p2p_enable(padapter, P2P_ROLE_DEVICE);
		padapter->wdinfo.listen_channel = remain_ch;
		RTW_INFO(FUNC_ADPT_FMT" init listen_channel %u\n"
			, FUNC_ADPT_ARG(padapter), padapter->wdinfo.listen_channel);
	} else if (rtw_p2p_chk_state(pwdinfo , P2P_STATE_LISTEN)
		&& (time_after_eq(rtw_get_current_time(), pwdev_priv->probe_resp_ie_update_time)
			&& rtw_get_passing_time_ms(pwdev_priv->probe_resp_ie_update_time) < 50)
	) {
		if (padapter->wdinfo.listen_channel != remain_ch) {
			padapter->wdinfo.listen_channel = remain_ch;
			RTW_INFO(FUNC_ADPT_FMT" update listen_channel %u\n"
				, FUNC_ADPT_ARG(padapter), padapter->wdinfo.listen_channel);
		}
	} else {
		rtw_p2p_set_pre_state(pwdinfo, rtw_p2p_state(pwdinfo));
#ifdef CONFIG_DEBUG_CFG80211
		RTW_INFO("%s, role=%d, p2p_state=%d\n", __func__, rtw_p2p_role(pwdinfo), rtw_p2p_state(pwdinfo));
#endif
	}

	rtw_p2p_set_state(pwdinfo, P2P_STATE_LISTEN);

	#ifdef RTW_ROCH_DURATION_ENLARGE
	if (duration < 400)
		duration = duration * 3; /* extend from exper */
	#endif

#if defined(RTW_ROCH_BACK_OP) && defined(CONFIG_CONCURRENT_MODE)
	if (rtw_mi_check_status(padapter, MI_LINKED)) {
		if (is_p2p_find) /* p2p_find , duration<1000 */
			duration = duration + pwdinfo->ext_listen_interval;
		else /* p2p_listen, duration=5000 */
			duration = pwdinfo->ext_listen_interval + (pwdinfo->ext_listen_interval / 4);
	}
#endif /*defined (RTW_ROCH_BACK_OP) && defined(CONFIG_CONCURRENT_MODE) */

	rtw_cfg80211_set_is_roch(padapter, _TRUE);
	pcfg80211_wdinfo->ro_ch_wdev = wdev;
	pcfg80211_wdinfo->remain_on_ch_cookie = *cookie;
	pcfg80211_wdinfo->duration = duration;
	rtw_cfg80211_set_last_ro_ch_time(padapter);
	_rtw_memcpy(&pcfg80211_wdinfo->remain_on_ch_channel, channel, sizeof(struct ieee80211_channel));
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
	pcfg80211_wdinfo->remain_on_ch_type = channel_type;
	#endif
	pcfg80211_wdinfo->restore_channel = rtw_get_oper_ch(padapter);

	p2p_roch_cmd(padapter, *cookie, wdev, channel, pcfg80211_wdinfo->remain_on_ch_type,
		duration, RTW_CMDF_WAIT_ACK);

	rtw_cfg80211_ready_on_channel(wdev, *cookie, channel, channel_type, duration, GFP_KERNEL);
exit:
	return err;
}
#endif /* CONFIG_PHL_ARCH */

static s32 cfg80211_rtw_cancel_remain_on_channel(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct wireless_dev *wdev,
#else
	struct net_device *ndev,
#endif
	u64 cookie)
{
	s32 err = 0;
	_adapter *padapter;
	struct rtw_wdev_priv *pwdev_priv;
	struct cfg80211_roch_info *pcfg80211_rochinfo;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	#if defined(RTW_DEDICATED_P2P_DEVICE)
	if (wdev == wiphy_to_pd_wdev(wiphy))
		padapter = wiphy_to_adapter(wiphy);
	else
	#endif
	if (wdev_to_ndev(wdev))
		padapter = (_adapter *)rtw_netdev_priv(wdev_to_ndev(wdev));
	else {
		err = -EINVAL;
		goto exit;
	}
#else
	struct wireless_dev *wdev;

	if (ndev == NULL) {
		err = -EINVAL;
		goto exit;
	}
	padapter = (_adapter *)rtw_netdev_priv(ndev);
	wdev = ndev_to_wdev(ndev);
#endif

	pwdev_priv = adapter_wdev_data(padapter);
	pcfg80211_rochinfo = &padapter->cfg80211_rochinfo;

	RTW_INFO(FUNC_ADPT_FMT"%s cookie:0x%llx\n"
		, FUNC_ADPT_ARG(padapter), wdev == wiphy_to_pd_wdev(wiphy) ? " PD" : ""
		, cookie);

	if (rtw_cfg80211_get_is_roch(padapter) == _TRUE) {
		rtw_scan_abort(padapter, 0);
	}

exit:
	return err;
}

inline void rtw_cfg80211_set_is_mgmt_tx(_adapter *adapter, u8 val)
{
	struct rtw_wdev_priv *wdev_priv = adapter_wdev_data(adapter);

	wdev_priv->is_mgmt_tx = val;
	rtw_mi_update_iface_status(&(adapter->mlmepriv), 0);
}

inline u8 rtw_cfg80211_get_is_mgmt_tx(_adapter *adapter)
{
	struct rtw_wdev_priv *wdev_priv = adapter_wdev_data(adapter);

	return wdev_priv->is_mgmt_tx;
}

static int _cfg80211_rtw_mgmt_tx(_adapter *padapter, u8 tx_band, u8 tx_ch, u8 no_cck, const u8 *buf, size_t len, int wait_ack)
{
	struct xmit_frame	*pmgntframe;
	struct pkt_attrib	*pattrib;
	unsigned char	*pframe;
	int ret = _FAIL;
	bool ack = _TRUE;
	struct rtw_ieee80211_hdr *pwlanhdr;
#if defined(RTW_ROCH_BACK_OP) && defined(CONFIG_P2P) && defined(CONFIG_CONCURRENT_MODE)
	struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(padapter);
#endif
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_chan_def u_chdef = {0};
	u8 leave_op = 0;
#ifdef CONFIG_P2P
	struct cfg80211_roch_info *pcfg80211_rochinfo = &padapter->cfg80211_rochinfo;
	struct wifidirect_info *pwdinfo = &padapter->wdinfo;
#endif
#ifdef CONFIG_GO_APPEND_COUNTRY_IE
	u8 *p = NULL;
	uint ie_len = 0;
#endif
	u8 frame_styp;
	u8 hdr_len = sizeof(struct rtw_ieee80211_hdr_3addr);
	u8 *ies;
	sint ies_len;
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	struct link_mlme_ext_priv *lmlmeext = &(padapter_link->mlmeextpriv);
	struct link_mlme_ext_info *lmlmeinfo = &(lmlmeext->mlmext_info);
	WLAN_BSSID_EX *cur_network = &(lmlmeinfo->network);

	rtw_cfg80211_set_is_mgmt_tx(padapter, 1);

#if 0
/* phl_scan
 * Enable ext period when WIFI_ASOC_STATE
 * cancel roch when ext_listen_period is up
 * remove it, because phl_sacn will do extend
 */
#ifdef CONFIG_P2P
	if (rtw_cfg80211_get_is_roch(padapter) == _TRUE) {
		#ifdef CONFIG_CONCURRENT_MODE
		if (!check_fwstate(&padapter->mlmepriv, WIFI_ASOC_STATE)) {
			RTW_INFO("%s, extend ro ch time\n", __func__);
			_set_timer(&padapter->cfg80211_wdinfo.remain_on_ch_timer, pwdinfo->ext_listen_period);
		}
		#endif /* CONFIG_CONCURRENT_MODE */
	}
#endif /* CONFIG_P2P */
#endif
	if ((rtw_phl_mr_get_chandef(dvobj->phl, padapter->phl_role,
				padapter_link->wrlink, &u_chdef) != RTW_PHL_STATUS_SUCCESS)
				|| (u_chdef.chan == 0)) {
		RTW_ERR("%s get union chandef failed\n", __func__);
		rtw_warn_on(1);
		goto exit;
	}

	if (rtw_mi_check_status(padapter, MI_LINKED)
		&& tx_ch != u_chdef.chan
	) {

		/* phl_scan will self leave opch */
		if (!rtw_cfg80211_get_is_roch(padapter)) {
			rtw_leave_opch(padapter);
			leave_op = 1;
		}
	}

	/* phl_scan will self remain on ch */
	if (!rtw_cfg80211_get_is_roch(padapter)) {
		if (tx_ch != rtw_get_oper_ch(padapter, padapter_link))
			set_bch_bwmode(padapter, padapter_link, tx_band,
				tx_ch, CHAN_OFFSET_NO_EXT, CHANNEL_WIDTH_20, RFK_TYPE_FORCE_NOT_DO);
	}

	/* starting alloc mgmt frame to dump it */
	pmgntframe = alloc_mgtxmitframe(pxmitpriv);
	if (pmgntframe == NULL) {
		/* ret = -ENOMEM; */
		ret = _FAIL;
		goto exit;
	}

	/* update attribute */
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, padapter_link, pattrib);

	if (no_cck && IS_CCK_RATE(pattrib->rate)) {
		/* force OFDM 6M rate*/
		pattrib->rate = MGN_6M;
	}

	pattrib->retry_ctrl = _FALSE;

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;

	_rtw_memcpy(pframe, (void *)buf, len);
	pattrib->pktlen = len;

	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;
	/* update seq number */
	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pattrib->seqnum = pmlmeext->mgnt_seq;
	pmlmeext->mgnt_seq++;

	frame_styp = get_frame_sub_type(pframe);
	if (frame_styp == WIFI_PROBERSP) {
		/* Update VHT capability IE, HE capability IE and HE operation IE */
		ies = pmgntframe->buf_addr + TXDESC_OFFSET + hdr_len + _BEACON_IE_OFFSET_;
		ies_len = pattrib->pktlen - hdr_len - _BEACON_IE_OFFSET_;
		#ifdef CONFIG_80211AC_VHT
		rtw_update_probe_rsp_vht_cap(padapter, ies, ies_len);
		#endif
		#ifdef CONFIG_80211AX_HE
		rtw_update_probe_rsp_he_cap_and_op(padapter, ies, &ies_len, tx_band);
		pattrib->pktlen = ies_len + hdr_len + _BEACON_IE_OFFSET_;
		#endif

		#ifdef CONFIG_P2P_PS
		/* Add NoA IE in probe response */
		rtw_append_probe_resp_p2p_go_noa(pmgntframe);
		#endif

		#ifdef CONFIG_ECSA_PHL
		/* Update channel switch count of CSA/ECSA IE in probe response */
		rtw_ecsa_update_probe_resp(pmgntframe);
		#endif

		rtw_append_probe_resp_vendor_ie(pmgntframe);
		rtw_append_probe_resp_p2p_ie(pmgntframe);

		#ifdef CONFIG_GO_APPEND_COUNTRY_IE
		if (rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO)) {
			/* check if country IE is included */
			p = rtw_get_ie(cur_network->IEs + _BEACON_IE_OFFSET_, WLAN_EID_COUNTRY, &ie_len, (cur_network->IELength - _BEACON_IE_OFFSET_));

			if (!p)
				pframe += rtw_set_ie_country(padapter, pmgntframe->buf_addr + TXDESC_OFFSET + sizeof(struct rtw_ieee80211_hdr_3addr), &(pattrib->pktlen));
		}
		#endif

                /* hostapd would not add/remove CCK rate when switching bands in our case,
                   but there's no need to consider the CCK rate in P2P GO. */
		if (MLME_IS_AP(padapter) && !MLME_IS_GO(padapter))
			rtw_update_probe_rsp_basic_rate_and_ext(pmgntframe);
	}

#ifdef CONFIG_P2P
	rtw_xframe_chk_wfd_ie(pmgntframe);
#endif /* CONFIG_P2P */

	pattrib->last_txcmdsz = pattrib->pktlen;

	if (wait_ack) {
		if (dump_mgntframe_and_wait_ack(padapter, pmgntframe) != _SUCCESS) {
			ack = _FALSE;
			ret = _FAIL;

#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("%s, ack == _FAIL\n", __func__);
#endif
		} else {

#ifdef CONFIG_XMIT_ACK
			if (!MLME_IS_MESH(padapter)) /* TODO: remove this sleep for all mode */
				rtw_msleep_os(50);
#endif
#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("%s, ack=%d, ok!\n", __func__, ack);
#endif
			ret = _SUCCESS;
		}
	} else {
		dump_mgntframe(padapter, pmgntframe);
		ret = _SUCCESS;
	}

exit:

/* phl_scan will self return opch */
#if 0
	#ifdef CONFIG_P2P
	if (rtw_cfg80211_get_is_roch(padapter)
		&& !roch_stay_in_cur_chan(padapter)
		&& pcfg80211_wdinfo->remain_on_ch_channel.hw_value != u_ch
	) {
		/* roch is ongoing, switch back to rch */
		if (pcfg80211_wdinfo->remain_on_ch_channel.hw_value != tx_ch)
			set_channel_bwmode(padapter,
				pcfg80211_wdinfo->remain_on_ch_channel.hw_value,
				CHAN_OFFSET_NO_EXT,
				CHANNEL_WIDTH_20,
				RFK_TYPE_FORCE_NOT_DO);
	} else
	#endif
#endif
	if (leave_op) {
		if (rtw_mi_check_status(padapter, MI_LINKED))
			set_bch_bwmode(padapter, padapter_link, u_chdef.band, u_chdef.chan, u_chdef.offset, u_chdef.bw, RFK_TYPE_FORCE_NOT_DO);

		rtw_back_opch(padapter);
	}
	rtw_cfg80211_set_is_mgmt_tx(padapter, 0);

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO("%s, ret=%d\n", __func__, ret);
#endif

	return ret;

}

u8 rtw_mgnt_tx_handler(_adapter *adapter, u8 *buf)
{
	u8 rst = H2C_CMD_FAIL;
	struct mgnt_tx_parm *mgnt_parm = (struct mgnt_tx_parm *)buf;

	if (_cfg80211_rtw_mgmt_tx(adapter, mgnt_parm->tx_band, mgnt_parm->tx_ch, mgnt_parm->no_cck,
		mgnt_parm->buf, mgnt_parm->len, mgnt_parm->wait_ack) == _SUCCESS)
		rst = H2C_SUCCESS;

	return rst;
}

static int cfg80211_rtw_mgmt_tx(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct wireless_dev *wdev,
#else
	struct net_device *ndev,
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0)) || defined(COMPAT_KERNEL_RELEASE)
	struct ieee80211_channel *chan,
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)) || defined(COMPAT_KERNEL_RELEASE)
	bool offchan,
	#endif
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
	enum nl80211_channel_type channel_type,
	#endif
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
	bool channel_type_valid,
	#endif
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)) || defined(COMPAT_KERNEL_RELEASE)
	unsigned int wait,
	#endif
	const u8 *buf, size_t len,
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	bool no_cck,
	#endif
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0))
	bool dont_wait_for_ack,
	#endif
#else
	struct cfg80211_mgmt_tx_params *params,
#endif
	u64 *cookie)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)) || defined(COMPAT_KERNEL_RELEASE)
	struct ieee80211_channel *chan = params->chan;
	const u8 *buf = params->buf;
	size_t len = params->len;
	bool no_cck = params->no_cck;
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0))
	bool no_cck = 0;
#endif
	int ret = 0;
	u8 tx_ret;
	int wait_ack = 1;
	const u8 *dump_buf = buf;
	size_t dump_len = len;
	u32 dump_limit = RTW_MAX_MGMT_TX_CNT;
	u32 dump_cnt = 0;
	u32 sleep_ms = 0;
	u32 retry_guarantee_ms = 0;
	bool ack = _TRUE;
	u8 tx_band;
	u8 tx_ch;
	u8 category, action;
	u8 frame_styp;
#ifdef CONFIG_P2P
	u8 is_p2p = 0;
#endif
	int type = (-1);
	systime start = rtw_get_current_time();
	_adapter *padapter;
	struct dvobj_priv *dvobj;
	struct rtw_wdev_priv *pwdev_priv;
	struct rf_ctl_t *rfctl;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	#if defined(RTW_DEDICATED_P2P_DEVICE)
	if (wdev == wiphy_to_pd_wdev(wiphy))
		padapter = wiphy_to_adapter(wiphy);
	else
	#endif
	if (wdev_to_ndev(wdev))
		padapter = (_adapter *)rtw_netdev_priv(wdev_to_ndev(wdev));
	else {
		ret = -EINVAL;
		goto exit;
	}
#else
	struct wireless_dev *wdev;

	if (ndev == NULL) {
		ret = -EINVAL;
		goto exit;
	}
	padapter = (_adapter *)rtw_netdev_priv(ndev);
	wdev = ndev_to_wdev(ndev);
#endif

	if (chan == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	rfctl = adapter_to_rfctl(padapter);
	tx_band = (u8)nl80211_band_to_rtw_band(chan->band);
	tx_ch = (u8)ieee80211_frequency_to_channel(chan->center_freq);
	if (IS_CH_WAITING(rfctl)) {
		#ifdef CONFIG_DFS_MASTER
		if (rtw_rfctl_overlap_radar_detect_ch(rfctl, nl80211_band_to_rtw_band(chan->band), tx_ch, CHANNEL_WIDTH_20, CHAN_OFFSET_NO_EXT)) {
			ret = -EINVAL;
			goto exit;
		}
		#endif
	}

	dvobj = adapter_to_dvobj(padapter);
	pwdev_priv = adapter_wdev_data(padapter);

	/* cookie generation */
	*cookie = pwdev_priv->mgmt_tx_cookie++;

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO(FUNC_ADPT_FMT"%s len=%zu, ch=%d"
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
		", ch_type=%d"
		#endif
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
		", channel_type_valid=%d"
		#endif
		"\n", FUNC_ADPT_ARG(padapter), wdev == wiphy_to_pd_wdev(wiphy) ? " PD" : ""
		, len, tx_ch
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
		, channel_type
		#endif
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0))
		, channel_type_valid
		#endif
	);
#endif /* CONFIG_DEBUG_CFG80211 */

	/* indicate ack before issue frame to avoid racing with rsp frame */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	rtw_cfg80211_mgmt_tx_status(wdev, *cookie, buf, len, ack, GFP_KERNEL);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34) && LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 36))
	cfg80211_action_tx_status(ndev, *cookie, buf, len, ack, GFP_KERNEL);
#endif

	frame_styp = le16_to_cpu(((struct rtw_ieee80211_hdr_3addr *)buf)->frame_ctl) & IEEE80211_FCTL_STYPE;
	if (IEEE80211_STYPE_PROBE_RESP == frame_styp) {
#ifdef CONFIG_P2P
		/* 36 = IEEE80211_3ADDR_LEN + _TIMESTAMP_ + _BEACON_ITERVAL_ + _CAPABILITY_ */
		if(rtw_get_p2p_ie(dump_buf + 36, dump_len - 36, NULL, NULL)) {
			struct wifidirect_info *pwdinfo = &padapter->wdinfo;
			if (rtw_p2p_chk_role(pwdinfo, P2P_ROLE_DISABLE)) {
				if (!rtw_p2p_enable(padapter, P2P_ROLE_DEVICE)) {
					ret = -EOPNOTSUPP;
					goto exit;
				}
			}
			no_cck = 1;
		}
#endif
#ifdef CONFIG_DEBUG_CFG80211
		RTW_INFO("RTW_Tx: probe_resp tx_ch=%d, no_cck=%u, da="MAC_FMT"\n", tx_ch, no_cck, MAC_ARG(GetAddr1Ptr(buf)));
#endif /* CONFIG_DEBUG_CFG80211 */
		wait_ack = 0;
		goto dump;
	}
	else if (frame_styp == RTW_IEEE80211_STYPE_AUTH) {
		int retval = 0;

		RTW_INFO("RTW_Tx:tx_ch=%d, no_cck=%u, da="MAC_FMT"\n", tx_ch, no_cck, MAC_ARG(GetAddr1Ptr(buf)));

#ifdef CONFIG_RTW_80211R_AP
		rtw_ft_process_ft_auth_rsp(padapter, (u8 *)buf, len);
#endif

		retval = rtw_sae_preprocess(padapter, buf, len, _TRUE);
		if (retval == 2)
			goto exit;
		if (retval == 0)
			RTW_INFO("RTW_Tx:AUTH\n");
		dump_limit = 1;
		goto dump;
	}

	if (rtw_action_frame_parse(buf, len, &category, &action) == _FALSE) {
		RTW_INFO(FUNC_ADPT_FMT" frame_control:0x%02x\n", FUNC_ADPT_ARG(padapter),
			le16_to_cpu(((struct rtw_ieee80211_hdr_3addr *)buf)->frame_ctl));
		goto exit;
	}

	RTW_INFO("RTW_Tx:tx_ch=%d, no_cck=%u, da="MAC_FMT"\n", tx_ch, no_cck, MAC_ARG(GetAddr1Ptr(buf)));
#ifdef CONFIG_P2P
	type = rtw_p2p_check_frames(padapter, buf, len, _TRUE);
	if (type >= 0) {
		is_p2p = 1;
		no_cck = 1; /* force no CCK for P2P frames */
		goto dump;
	}
#endif
#ifdef CONFIG_RTW_MESH
	if (MLME_IS_MESH(padapter)) {
		type = rtw_mesh_check_frames_tx(padapter, &dump_buf, &dump_len);
		if (type >= 0) {
			dump_limit = 1;
			goto dump;
		}
	}
#endif
	if (category == RTW_WLAN_CATEGORY_PUBLIC) {
		RTW_INFO("RTW_Tx:%s\n", action_public_str(action));
		switch (action) {
		case ACT_PUBLIC_GAS_INITIAL_REQ:
		case ACT_PUBLIC_GAS_INITIAL_RSP:
			sleep_ms = 50;
			retry_guarantee_ms = RTW_MAX_MGMT_TX_MS_GAS;
			break;
		}
	}
#ifdef CONFIG_RTW_80211K
	else if (category == RTW_WLAN_CATEGORY_RADIO_MEAS)
		RTW_INFO("RTW_Tx: RRM Action\n");
#endif
	else
		RTW_INFO("RTW_Tx:category(%u), action(%u)\n", category, action);

dump:

	while (1) {
		dump_cnt++;

		/* ROCH uses phl scan to handle, cancel scan is not necessary */
		if (!rtw_cfg80211_get_is_roch(padapter) &&
		    frame_styp != IEEE80211_STYPE_PROBE_RESP) {
			rtw_mi_set_scan_deny(padapter, 1000);
			rtw_mi_scan_abort(padapter, _TRUE);
		}

		tx_ret = rtw_mgnt_tx_cmd(padapter, tx_band, tx_ch, no_cck, dump_buf, dump_len, wait_ack, RTW_CMDF_WAIT_ACK);
		if (tx_ret == _SUCCESS
			|| (dump_cnt >= dump_limit && rtw_get_passing_time_ms(start) >= retry_guarantee_ms))
			break;

		if (sleep_ms > 0)
			rtw_msleep_os(sleep_ms);
	}

	if (tx_ret != _SUCCESS || dump_cnt > 1) {
		RTW_INFO(FUNC_ADPT_FMT" %s (%d/%d) in %d ms\n", FUNC_ADPT_ARG(padapter),
			tx_ret == _SUCCESS ? "OK" : "FAIL", dump_cnt, dump_limit, rtw_get_passing_time_ms(start));
	}

#ifdef CONFIG_P2P
	if (is_p2p) {
		switch (type) {
		case P2P_GO_NEGO_CONF:
			if (0) {
				RTW_INFO(FUNC_ADPT_FMT" Nego confirm. state=%u, status=%u, iaddr="MAC_FMT"\n"
					, FUNC_ADPT_ARG(padapter), pwdev_priv->nego_info.state, pwdev_priv->nego_info.status
					, MAC_ARG(pwdev_priv->nego_info.iface_addr));
			}
			if (pwdev_priv->nego_info.state == 2
				&& pwdev_priv->nego_info.status == 0
				&& rtw_check_invalid_mac_address(pwdev_priv->nego_info.iface_addr, _FALSE) == _FALSE
			) {
				_adapter *intended_iface = dvobj_get_adapter_by_addr(dvobj, pwdev_priv->nego_info.iface_addr);

				if (intended_iface) {
					RTW_INFO(FUNC_ADPT_FMT" Nego confirm. Allow only "ADPT_FMT" to scan for 2000 ms\n"
						, FUNC_ADPT_ARG(padapter), ADPT_ARG(intended_iface));
					/* allow only intended_iface to do scan for 2000 ms */
					rtw_mi_set_scan_deny(padapter, 2000);
					rtw_clear_scan_deny(intended_iface);
				}
			}
			break;
		case P2P_INVIT_RESP:
			if (pwdev_priv->invit_info.flags & BIT(0)
				&& pwdev_priv->invit_info.status == 0
			) {
				rtw_clear_scan_deny(padapter);
				RTW_INFO(FUNC_ADPT_FMT" agree with invitation of persistent group\n",
					FUNC_ADPT_ARG(padapter));
				#if !RTW_P2P_GROUP_INTERFACE
				rtw_mi_buddy_set_scan_deny(padapter, 5000);
				#endif
			}
			break;
		}
	}
#endif /* CONFIG_P2P */

	/* Driver will stuck at CONNECT_FG if STA TX auth frame fail, so trigger join timeout handler */
	if (tx_ret != _SUCCESS && frame_styp == RTW_IEEE80211_STYPE_AUTH && MLME_IS_STA(padapter)) {
		RTW_INFO("TX auth fail, trigger join timeout handler\n");
		set_assoc_timer(&(padapter->mlmepriv), 1);
	}

	if (dump_buf != buf)
		rtw_mfree((u8 *)dump_buf, dump_len);
exit:
	return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0))
static void cfg80211_rtw_mgmt_frame_register(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct wireless_dev *wdev,
#else
	struct net_device *ndev,
#endif
	u16 frame_type, bool reg)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct net_device *ndev = wdev_to_ndev(wdev);
#endif
	_adapter *adapter;
	struct rtw_wdev_priv *pwdev_priv;

#if defined(RTW_DEDICATED_P2P_DEVICE)
	if (wdev == wiphy_to_pd_wdev(wiphy)) {
		adapter = wiphy_to_adapter(wiphy);
	} else
#endif
	{
		if (ndev == NULL)
			goto exit;
		adapter = (_adapter *)rtw_netdev_priv(ndev);
	}
	pwdev_priv = adapter_wdev_data(adapter);

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO(FUNC_ADPT_FMT" frame_type:%x, reg:%d\n", FUNC_ADPT_ARG(adapter),
		frame_type, reg);
#endif

	switch (frame_type) {
	case IEEE80211_STYPE_AUTH: /* 0x00B0 */
		if (reg > 0)
			SET_CFG80211_REPORT_MGMT(pwdev_priv, IEEE80211_STYPE_AUTH);
		else
			CLR_CFG80211_REPORT_MGMT(pwdev_priv, IEEE80211_STYPE_AUTH);
		break;
	case IEEE80211_STYPE_PROBE_REQ: /* 0x0040 */
		if (reg > 0)
			SET_CFG80211_REPORT_MGMT(pwdev_priv, IEEE80211_STYPE_PROBE_REQ);
		else
			CLR_CFG80211_REPORT_MGMT(pwdev_priv, IEEE80211_STYPE_PROBE_REQ);
		break;
#ifdef not_yet
	case IEEE80211_STYPE_ACTION: /* 0x00D0 */
		if (reg > 0)
			SET_CFG80211_REPORT_MGMT(pwdev_priv, IEEE80211_STYPE_ACTION);
		else
			CLR_CFG80211_REPORT_MGMT(pwdev_priv, IEEE80211_STYPE_ACTION);
		break;
#endif
	default:
		break;
	}

exit:
	return;
}
#else
static void cfg80211_rtw_update_mgmt_frame_register(
	struct wiphy *wiphy,
	struct wireless_dev *wdev,
	struct mgmt_frame_regs *upd)
{
	struct net_device *ndev;
	_adapter *padapter;
	struct rtw_wdev_priv *pwdev_priv;
	u32 rtw_stypes_mask = 0;
	u32 rtw_mstypes_mask = 0;

	ndev = wdev_to_ndev(wdev);

	if (ndev == NULL)
		goto exit;

	padapter = (_adapter *)rtw_netdev_priv(ndev);
	pwdev_priv = adapter_wdev_data(padapter);

	/* Driver only supports Auth and Probe request */
	rtw_stypes_mask = BIT(IEEE80211_STYPE_AUTH >> 4) |
			  BIT(IEEE80211_STYPE_PROBE_REQ >> 4);

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO(FUNC_ADPT_FMT " global_stypes:0x%08x interface_stypes:0x%08x\n",
		FUNC_ADPT_ARG(padapter), upd->global_stypes, upd->interface_stypes);
	RTW_INFO(FUNC_ADPT_FMT " global_mcast_stypes:0x%08x interface_mcast_stypes:0x%08x\n",
		FUNC_ADPT_ARG(padapter), upd->global_mcast_stypes, upd->interface_mcast_stypes);
	RTW_INFO(FUNC_ADPT_FMT " old_regs:0x%08x new_regs:0x%08x\n",
		FUNC_ADPT_ARG(padapter), pwdev_priv->mgmt_regs,
		(upd->interface_stypes & rtw_stypes_mask));
#endif
	if (pwdev_priv->mgmt_regs !=
			(upd->interface_stypes & rtw_stypes_mask)) {
		pwdev_priv->mgmt_regs = (upd->interface_stypes & rtw_stypes_mask);
	}

exit:
	return;
}
#endif

#if defined(CONFIG_TDLS) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
static int cfg80211_rtw_tdls_mgmt(struct wiphy *wiphy,
	struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0))
	const u8 *peer,
#else
	u8 *peer,
#endif
	u8 action_code,
	u8 dialog_token,
	u16 status_code,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0))
	u32 peer_capability,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0))
	bool initiator,
#endif
	const u8 *buf,
	size_t len)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &pmlmeext->mlmext_info;
	int ret = 0;
	struct tdls_txmgmt txmgmt;

	if (rtw_hw_chk_wl_func(dvobj, WL_FUNC_TDLS) == _FALSE) {
		RTW_INFO("Discard tdls action:%d, since hal doesn't support tdls\n", action_code);
		goto discard;
	}

	if (rtw_is_tdls_enabled(padapter) == _FALSE) {
		RTW_INFO("TDLS is not enabled\n");
		goto discard;
	}

	if (rtw_tdls_is_driver_setup(padapter)) {
		RTW_INFO("Discard tdls action:%d, let driver to set up direct link\n", action_code);
		goto discard;
	}

	_rtw_memset(&txmgmt, 0x00, sizeof(struct tdls_txmgmt));
	_rtw_memcpy(txmgmt.peer, peer, ETH_ALEN);
	txmgmt.action_code = action_code;
	txmgmt.dialog_token = dialog_token;
	txmgmt.status_code = status_code;
	txmgmt.len = len;
	txmgmt.buf = (u8 *)rtw_malloc(txmgmt.len);
	if (txmgmt.buf == NULL) {
		ret = -ENOMEM;
		goto bad;
	}
	_rtw_memcpy(txmgmt.buf, (void *)buf, txmgmt.len);

	/* Debug purpose */
#if 1
	RTW_INFO("%s %d\n", __FUNCTION__, __LINE__);
	RTW_INFO("peer:"MAC_FMT", action code:%d, dialog:%d, status code:%d\n",
		MAC_ARG(txmgmt.peer), txmgmt.action_code,
		txmgmt.dialog_token, txmgmt.status_code);
#if 0
	if (txmgmt.len > 0) {
		int i = 0;
		for (; i < len; i++)
			printk("%02x ", *(txmgmt.buf + i));
		RTW_INFO("len:%d\n", (u32)txmgmt.len);
	}
#endif
#endif

	switch (txmgmt.action_code) {
	case TDLS_SETUP_REQUEST:
		issue_tdls_setup_req(padapter, &txmgmt, _TRUE);
		break;
	case TDLS_SETUP_RESPONSE:
		issue_tdls_setup_rsp(padapter, &txmgmt);
		break;
	case TDLS_SETUP_CONFIRM:
		issue_tdls_setup_cfm(padapter, &txmgmt);
		break;
	case TDLS_TEARDOWN:
		issue_tdls_teardown(padapter, &txmgmt, _TRUE);
		break;
	case TDLS_DISCOVERY_REQUEST:
		issue_tdls_dis_req(padapter, &txmgmt);
		break;
	case TDLS_DISCOVERY_RESPONSE:
		issue_tdls_dis_rsp(padapter, &txmgmt, pmlmeinfo->enc_algo ? _TRUE : _FALSE);
		break;
	}

bad:
	if (txmgmt.buf)
		rtw_mfree(txmgmt.buf, txmgmt.len);

discard:
	return ret;
}

static int cfg80211_rtw_tdls_oper(struct wiphy *wiphy,
	struct net_device *ndev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0))
	const u8 *peer,
#else
	u8 *peer,
#endif
	enum nl80211_tdls_operation oper)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct tdls_info *ptdlsinfo = &padapter->tdlsinfo;
	struct tdls_txmgmt	txmgmt;
	struct sta_info *ptdls_sta = NULL;

	RTW_INFO(FUNC_NDEV_FMT", nl80211_tdls_operation:%d\n", FUNC_NDEV_ARG(ndev), oper);

	if (rtw_hw_chk_wl_func(dvobj, WL_FUNC_TDLS) == _FALSE) {
		RTW_INFO("Discard tdls oper:%d, since hal doesn't support tdls\n", oper);
		return 0;
	}

	if (rtw_is_tdls_enabled(padapter) == _FALSE) {
		RTW_INFO("TDLS is not enabled\n");
		return 0;
	}

	_rtw_memset(&txmgmt, 0x00, sizeof(struct tdls_txmgmt));
	if (peer)
		_rtw_memcpy(txmgmt.peer, peer, ETH_ALEN);

	if (rtw_tdls_is_driver_setup(padapter)) {
		/* these two cases are done by driver itself */
		if (oper == NL80211_TDLS_ENABLE_LINK || oper == NL80211_TDLS_DISABLE_LINK)
			return 0;
	}

	switch (oper) {
	case NL80211_TDLS_DISCOVERY_REQ:
		issue_tdls_dis_req(padapter, &txmgmt);
		break;
	case NL80211_TDLS_SETUP:
#ifdef CONFIG_WFD
		if (_AES_ != padapter->securitypriv.dot11PrivacyAlgrthm) {
			if (padapter->wdinfo.wfd_tdls_weaksec == _TRUE)
				issue_tdls_setup_req(padapter, &txmgmt, _TRUE);
			else
				RTW_INFO("[%s] Current link is not AES, SKIP sending the tdls setup request!!\n", __FUNCTION__);
		} else
#endif /* CONFIG_WFD */
		{
			issue_tdls_setup_req(padapter, &txmgmt, _TRUE);
		}
		break;
	case NL80211_TDLS_TEARDOWN:
		ptdls_sta = rtw_get_stainfo(&(padapter->stapriv), txmgmt.peer);
		if (ptdls_sta != NULL) {
			txmgmt.status_code = _RSON_TDLS_TEAR_UN_RSN_;
			issue_tdls_teardown(padapter, &txmgmt, _TRUE);
		} else
			RTW_INFO("TDLS peer not found\n");
		break;
	case NL80211_TDLS_ENABLE_LINK:
		RTW_INFO(FUNC_NDEV_FMT", NL80211_TDLS_ENABLE_LINK;mac:"MAC_FMT"\n", FUNC_NDEV_ARG(ndev), MAC_ARG(peer));
		ptdls_sta = rtw_get_stainfo(&(padapter->stapriv), (u8 *)peer);
		if (ptdls_sta != NULL) {
			rtw_tdls_set_link_established(padapter, _TRUE);
			ptdls_sta->tdls_sta_state |= TDLS_LINKED_STATE;
			ptdls_sta->state |= WIFI_ASOC_STATE;
			rtw_tdls_cmd(padapter, txmgmt.peer, TDLS_ESTABLISHED);
		}
		break;
	case NL80211_TDLS_DISABLE_LINK:
		RTW_INFO(FUNC_NDEV_FMT", NL80211_TDLS_DISABLE_LINK;mac:"MAC_FMT"\n", FUNC_NDEV_ARG(ndev), MAC_ARG(peer));
		ptdls_sta = rtw_get_stainfo(&(padapter->stapriv), (u8 *)peer);
		if (ptdls_sta != NULL) {
			if (!(ptdls_sta->tdls_sta_state & TDLS_RESETUP_STATE)) {
				rtw_tdls_teardown_pre_hdl(padapter, ptdls_sta);
				rtw_tdls_cmd(padapter, (u8 *)peer, TDLS_TEARDOWN_STA_LOCALLY_POST);
			} else {
				ptdls_sta->tdls_sta_state &= ~TDLS_RESETUP_STATE;
			}
		}
		break;
	}
	return 0;
}
#endif /* CONFIG_TDLS */

#if defined(CONFIG_RTW_MESH) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38))

#if DBG_RTW_CFG80211_MESH_CONF
#define LEGACY_RATES_STR_LEN (RTW_G_RATES_NUM * 5 + 1)
int get_legacy_rates_str(struct wiphy *wiphy, enum nl80211_band band, u32 mask, char *buf)
{
	int i;
	int cnt = 0;

	for (i = 0; i < wiphy->bands[band]->n_bitrates; i++) {
		if (mask & BIT(i)) {
			cnt += snprintf(buf + cnt, LEGACY_RATES_STR_LEN - cnt -1, "%d.%d "
				, wiphy->bands[band]->bitrates[i].bitrate / 10
				, wiphy->bands[band]->bitrates[i].bitrate % 10);
			if (cnt >= LEGACY_RATES_STR_LEN - 1)
				break;
		}
	}

	return cnt;
}

void dump_mesh_setup(void *sel, struct wiphy *wiphy, const struct mesh_setup *setup)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	struct cfg80211_chan_def *chdef = (struct cfg80211_chan_def *)(&setup->chandef);
#endif
	struct ieee80211_channel *chan;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	chan = (struct ieee80211_channel *)chdef->chan;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	chan = (struct ieee80211_channel *)setup->channel;
#endif

	RTW_PRINT_SEL(sel, "mesh_id:\"%s\", len:%u\n", setup->mesh_id, setup->mesh_id_len);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
	RTW_PRINT_SEL(sel, "sync_method:%u\n", setup->sync_method);
#endif
	RTW_PRINT_SEL(sel, "path_sel_proto:%u, path_metric:%u\n", setup->path_sel_proto, setup->path_metric);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	RTW_PRINT_SEL(sel, "auth_id:%u\n", setup->auth_id);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
	if (setup->ie && setup->ie_len) {
		RTW_PRINT_SEL(sel, "ie:%p, len:%u\n", setup->ie, setup->ie_len);
		dump_ies(RTW_DBGDUMP, setup->ie, setup->ie_len);
	}
#else
	if (setup->vendor_ie && setup->vendor_ie_len) {
		RTW_PRINT_SEL(sel, "ie:%p, len:%u\n", setup->vendor_ie, setup->vendor_ie_len);
		dump_ies(RTW_DBGDUMP, setup->vendor_ie, setup->vendor_ie_len);
	}
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
	RTW_PRINT_SEL(sel, "is_authenticated:%d, is_secure:%d\n", setup->is_authenticated, setup->is_secure);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	RTW_PRINT_SEL(sel, "user_mpm:%d\n", setup->user_mpm);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	RTW_PRINT_SEL(sel, "dtim_period:%u, beacon_interval:%u\n", setup->dtim_period, setup->beacon_interval);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	RTW_PRINT_SEL(sel, "center_freq:%u, ch:%u, width:%s, cfreq1:%u, cfreq2:%u\n"
		, chan->center_freq, chan->hw_value, nl80211_chan_width_str(chdef->width), chdef->center_freq1, chdef->center_freq2);
#else
	RTW_PRINT_SEL(sel, "center_freq:%u, ch:%u, channel_type:%s\n"
		, chan->center_freq, chan->hw_value, nl80211_channel_type_str(setup->channel_type));
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0))
	if (setup->mcast_rate[chan->band]) {
		RTW_PRINT_SEL(sel, "mcast_rate:%d.%d\n"
			, wiphy->bands[chan->band]->bitrates[setup->mcast_rate[chan->band] - 1].bitrate / 10
			, wiphy->bands[chan->band]->bitrates[setup->mcast_rate[chan->band] - 1].bitrate % 10
		);
	}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	if (setup->basic_rates) {
		char buf[LEGACY_RATES_STR_LEN] = {0};

		get_legacy_rates_str(wiphy, chan->band, setup->basic_rates, buf);
		RTW_PRINT_SEL(sel, "basic_rates:%s\n", buf);
	}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	if (setup->beacon_rate.control[chan->band].legacy) {
		char buf[LEGACY_RATES_STR_LEN] = {0};

		get_legacy_rates_str(wiphy, chan->band, setup->beacon_rate.control[chan->band].legacy, buf);
		RTW_PRINT_SEL(sel, "beacon_rate.legacy:%s\n", buf);
	}
	if (*((u32 *)&(setup->beacon_rate.control[chan->band].ht_mcs[0]))
		|| *((u32 *)&(setup->beacon_rate.control[chan->band].ht_mcs[4]))
		|| *((u16 *)&(setup->beacon_rate.control[chan->band].ht_mcs[8]))
	) {
		RTW_PRINT_SEL(sel, "beacon_rate.ht_mcs:"HT_RX_MCS_BMP_FMT"\n"
			, HT_RX_MCS_BMP_ARG(setup->beacon_rate.control[chan->band].ht_mcs));
	}

	if (setup->beacon_rate.control[chan->band].vht_mcs[0]
		|| setup->beacon_rate.control[chan->band].vht_mcs[1]
		|| setup->beacon_rate.control[chan->band].vht_mcs[2]
		|| setup->beacon_rate.control[chan->band].vht_mcs[3]
	) {
		int i;

		for (i = 0; i < 4; i++) {/* parsing up to 4SS */
			u16 mcs_mask = setup->beacon_rate.control[chan->band].vht_mcs[i];

			RTW_PRINT_SEL(sel, "beacon_rate.vht_mcs[%d]:%s\n", i
				, mcs_mask == 0x00FF ? "0~7" : mcs_mask == 0x01FF ? "0~8" : mcs_mask == 0x03FF ? "0~9" : "invalid");
		}
	}

	if (setup->beacon_rate.control[chan->band].gi) {
		RTW_PRINT_SEL(sel, "beacon_rate.gi:%s\n"
			, setup->beacon_rate.control[chan->band].gi == NL80211_TXRATE_FORCE_SGI ? "SGI" :
				setup->beacon_rate.control[chan->band].gi == NL80211_TXRATE_FORCE_LGI ? "LGI" : "invalid"
		);
	}
#endif
}

void dump_mesh_config(void *sel, const struct mesh_config *conf)
{
	RTW_PRINT_SEL(sel, "dot11MeshRetryTimeout:%u\n", conf->dot11MeshRetryTimeout);
	RTW_PRINT_SEL(sel, "dot11MeshConfirmTimeout:%u\n", conf->dot11MeshConfirmTimeout);
	RTW_PRINT_SEL(sel, "dot11MeshHoldingTimeout:%u\n", conf->dot11MeshHoldingTimeout);
	RTW_PRINT_SEL(sel, "dot11MeshMaxPeerLinks:%u\n", conf->dot11MeshMaxPeerLinks);
	RTW_PRINT_SEL(sel, "dot11MeshMaxRetries:%u\n", conf->dot11MeshMaxRetries);
	RTW_PRINT_SEL(sel, "dot11MeshTTL:%u\n", conf->dot11MeshTTL);
	RTW_PRINT_SEL(sel, "element_ttl:%u\n", conf->element_ttl);
	RTW_PRINT_SEL(sel, "auto_open_plinks:%d\n", conf->auto_open_plinks);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
	RTW_PRINT_SEL(sel, "dot11MeshNbrOffsetMaxNeighbor:%u\n", conf->dot11MeshNbrOffsetMaxNeighbor);
#endif

	RTW_PRINT_SEL(sel, "dot11MeshHWMPmaxPREQretries:%u\n", conf->dot11MeshHWMPmaxPREQretries);
	RTW_PRINT_SEL(sel, "path_refresh_time:%u\n", conf->path_refresh_time);
	RTW_PRINT_SEL(sel, "min_discovery_timeout:%u\n", conf->min_discovery_timeout);
	RTW_PRINT_SEL(sel, "dot11MeshHWMPactivePathTimeout:%u\n", conf->dot11MeshHWMPactivePathTimeout);
	RTW_PRINT_SEL(sel, "dot11MeshHWMPpreqMinInterval:%u\n", conf->dot11MeshHWMPpreqMinInterval);	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0))
	RTW_PRINT_SEL(sel, "dot11MeshHWMPperrMinInterval:%u\n", conf->dot11MeshHWMPperrMinInterval);
#endif
	RTW_PRINT_SEL(sel, "dot11MeshHWMPnetDiameterTraversalTime:%u\n", conf->dot11MeshHWMPnetDiameterTraversalTime);
	RTW_PRINT_SEL(sel, "dot11MeshHWMPRootMode:%u\n", conf->dot11MeshHWMPRootMode);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	RTW_PRINT_SEL(sel, "dot11MeshHWMPRannInterval:%u\n", conf->dot11MeshHWMPRannInterval);
	RTW_PRINT_SEL(sel, "dot11MeshGateAnnouncementProtocol:%d\n", conf->dot11MeshGateAnnouncementProtocol);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0))
	RTW_PRINT_SEL(sel, "dot11MeshForwarding:%d\n", conf->dot11MeshForwarding);
	RTW_PRINT_SEL(sel, "rssi_threshold:%d\n", conf->rssi_threshold);
#endif
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
	RTW_PRINT_SEL(sel, "ht_opmode:0x%04x\n", conf->ht_opmode);
#endif
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	RTW_PRINT_SEL(sel, "dot11MeshHWMPactivePathToRootTimeout:%u\n", conf->dot11MeshHWMPactivePathToRootTimeout);
	RTW_PRINT_SEL(sel, "dot11MeshHWMProotInterval:%u\n", conf->dot11MeshHWMProotInterval);
	RTW_PRINT_SEL(sel, "dot11MeshHWMPconfirmationInterval:%u\n", conf->dot11MeshHWMPconfirmationInterval);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	RTW_PRINT_SEL(sel, "power_mode:%s\n", nl80211_mesh_power_mode_str(conf->power_mode));
	RTW_PRINT_SEL(sel, "dot11MeshAwakeWindowDuration:%u\n", conf->dot11MeshAwakeWindowDuration);
#endif
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	RTW_PRINT_SEL(sel, "plink_timeout:%u\n", conf->plink_timeout);
#endif
}
#endif /* DBG_RTW_CFG80211_MESH_CONF */

static void rtw_cfg80211_mesh_info_set_profile(struct rtw_mesh_info *minfo, const struct mesh_setup *setup)
{
	_rtw_memcpy(minfo->mesh_id, setup->mesh_id, setup->mesh_id_len);
	minfo->mesh_id_len = setup->mesh_id_len;
	minfo->mesh_pp_id = setup->path_sel_proto;
	minfo->mesh_pm_id = setup->path_metric;
	minfo->mesh_cc_id = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
	minfo->mesh_sp_id = setup->sync_method;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	minfo->mesh_auth_id = setup->auth_id;
#else
	if (setup->is_authenticated) {
		u8 *rsn_ie;
		sint rsn_ie_len;
		struct rsne_info info;
		u8 *akm;
		u8 AKM_SUITE_SAE[4] = {0x00, 0x0F, 0xAC, 0x08};

		rsn_ie = rtw_get_ie(setup->ie, WLAN_EID_RSN, &rsn_ie_len, setup->ie_len);
		if (!rsn_ie || !rsn_ie_len) {
			rtw_warn_on(1);
			return;
		}

		if (rtw_rsne_info_parse(rsn_ie, rsn_ie_len + 2, &info) != _SUCCESS) {
			rtw_warn_on(1);
			return;
		}

		if (!info.akm_list || !info.akm_cnt) {
			rtw_warn_on(1);
			return;
		}

		akm = info.akm_list;
		while (akm < info.akm_list + info.akm_cnt * 4) {
			if (_rtw_memcmp(akm, AKM_SUITE_SAE, 4) == _TRUE) {
				minfo->mesh_auth_id = 0x01;
				break;
			}
		}

		if (!minfo->mesh_auth_id) {
			rtw_warn_on(1);
			return;
		}
	}
#endif
}

static inline bool chk_mesh_attr(enum nl80211_meshconf_params parm, u32 mask)
{
	return (mask >> (parm - 1)) & 0x1;
}

static void rtw_cfg80211_mesh_cfg_set(_adapter *adapter, const struct mesh_config *conf, u32 mask)
{
	struct rtw_mesh_cfg *mcfg = &adapter->mesh_cfg;

#if 0 /* driver MPM */
	if (chk_mesh_attr(NL80211_MESHCONF_RETRY_TIMEOUT, mask));
	if (chk_mesh_attr(NL80211_MESHCONF_CONFIRM_TIMEOUT, mask));
	if (chk_mesh_attr(NL80211_MESHCONF_HOLDING_TIMEOUT, mask));
	if (chk_mesh_attr(NL80211_MESHCONF_MAX_PEER_LINKS, mask));
	if (chk_mesh_attr(NL80211_MESHCONF_MAX_RETRIES, mask));
#endif

	if (chk_mesh_attr(NL80211_MESHCONF_TTL, mask))
		mcfg->dot11MeshTTL = conf->dot11MeshTTL;
	if (chk_mesh_attr(NL80211_MESHCONF_ELEMENT_TTL, mask))
		mcfg->element_ttl = conf->element_ttl;

#if 0 /* driver MPM */
	if (chk_mesh_attr(NL80211_MESHCONF_AUTO_OPEN_PLINKS, mask));
#endif

#if 0 /* TBD: synchronization */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
	if (chk_mesh_attr(NL80211_MESHCONF_SYNC_OFFSET_MAX_NEIGHBOR, mask));
#endif
#endif

	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_MAX_PREQ_RETRIES, mask))
		mcfg->dot11MeshHWMPmaxPREQretries = conf->dot11MeshHWMPmaxPREQretries;
	if (chk_mesh_attr(NL80211_MESHCONF_PATH_REFRESH_TIME, mask))
		mcfg->path_refresh_time = conf->path_refresh_time;
	if (chk_mesh_attr(NL80211_MESHCONF_MIN_DISCOVERY_TIMEOUT, mask))
		mcfg->min_discovery_timeout = conf->min_discovery_timeout;
	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_ACTIVE_PATH_TIMEOUT, mask))
		mcfg->dot11MeshHWMPactivePathTimeout = conf->dot11MeshHWMPactivePathTimeout;
	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_PREQ_MIN_INTERVAL, mask))
		mcfg->dot11MeshHWMPpreqMinInterval = conf->dot11MeshHWMPpreqMinInterval;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0))
	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_PERR_MIN_INTERVAL, mask))
		mcfg->dot11MeshHWMPperrMinInterval = conf->dot11MeshHWMPperrMinInterval;
#endif
	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_NET_DIAM_TRVS_TIME, mask))
		mcfg->dot11MeshHWMPnetDiameterTraversalTime = conf->dot11MeshHWMPnetDiameterTraversalTime;

	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_ROOTMODE, mask))
		mcfg->dot11MeshHWMPRootMode = conf->dot11MeshHWMPRootMode;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	if (chk_mesh_attr(NL80211_MESHCONF_GATE_ANNOUNCEMENTS, mask))
		mcfg->dot11MeshGateAnnouncementProtocol = conf->dot11MeshGateAnnouncementProtocol;
	/* our current gate annc implementation rides on root annc with gate annc bit in PREQ flags */
	if (mcfg->dot11MeshGateAnnouncementProtocol
		&& mcfg->dot11MeshHWMPRootMode <= RTW_IEEE80211_ROOTMODE_ROOT
	) {
		mcfg->dot11MeshHWMPRootMode = RTW_IEEE80211_PROACTIVE_RANN;
		RTW_INFO(ADPT_FMT" enable PROACTIVE_RANN becaue gate annc is needed\n", ADPT_ARG(adapter));
	}
	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_RANN_INTERVAL, mask))
		mcfg->dot11MeshHWMPRannInterval = conf->dot11MeshHWMPRannInterval;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0))
	if (chk_mesh_attr(NL80211_MESHCONF_FORWARDING, mask))
		mcfg->dot11MeshForwarding = conf->dot11MeshForwarding;

	if (chk_mesh_attr(NL80211_MESHCONF_RSSI_THRESHOLD, mask))
		mcfg->rssi_threshold = conf->rssi_threshold;
#endif

#if 0 /* controlled by driver */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
	if (chk_mesh_attr(NL80211_MESHCONF_HT_OPMODE, mask));
#endif
#endif
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_PATH_TO_ROOT_TIMEOUT, mask))
		mcfg->dot11MeshHWMPactivePathToRootTimeout = conf->dot11MeshHWMPactivePathToRootTimeout;
	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_ROOT_INTERVAL, mask))
		mcfg->dot11MeshHWMProotInterval = conf->dot11MeshHWMProotInterval;
	if (chk_mesh_attr(NL80211_MESHCONF_HWMP_CONFIRMATION_INTERVAL, mask))
		mcfg->dot11MeshHWMPconfirmationInterval = conf->dot11MeshHWMPconfirmationInterval;	
#endif

#if 0 /* TBD */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	if (chk_mesh_attr(NL80211_MESHCONF_POWER_MODE, mask));
	if (chk_mesh_attr(NL80211_MESHCONF_AWAKE_WINDOW, mask));
#endif
#endif

#if 0 /* driver MPM */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	if (chk_mesh_attr(NL80211_MESHCONF_PLINK_TIMEOUT, mask));
#endif
#endif
}

u8 *rtw_cfg80211_construct_mesh_beacon_ies(struct wiphy *wiphy, _adapter *adapter
	, const struct mesh_config *conf, const struct mesh_setup *setup
	, uint *ies_len)
{
	struct rtw_mesh_info *minfo = &adapter->mesh_info;
	struct rtw_mesh_cfg *mcfg = &adapter->mesh_cfg;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	struct cfg80211_chan_def *chdef = (struct cfg80211_chan_def *)(&setup->chandef);
#endif
	struct ieee80211_channel *chan;
	struct rtw_chan_def rtw_chdef = {0};
#endif
	uint len;
	u8 n_bitrates;
	u8 ht = 0;
	u8 vht = 0;
	u8 *rsn_ie = NULL;
	sint rsn_ie_len = 0;
	u8 *ies = NULL, *c;
	u8 supported_rates[RTW_G_RATES_NUM] = {0};
	int i;

	*ies_len = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	chan = (struct ieee80211_channel *)chdef->chan;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	chan = (struct ieee80211_channel *)setup->channel;
#endif

	n_bitrates = wiphy->bands[chan->band]->n_bitrates;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	rtw_get_chdef_from_cfg80211_chan_def(chdef, &ht, &rtw_chdef);
#else
	rtw_get_chdef_from_nl80211_channel_type(chan, setup->channel_type, &ht, &rtw_chdef);
#endif
	if (!rtw_chdef->chan)
		goto exit;

#if defined(CONFIG_80211AC_VHT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	vht = ht && rtw_chdef->chan > 14 && rtw_chdef->bw >= CHANNEL_WIDTH_80; /* VHT40/VHT20? */
#endif

	RTW_INFO(FUNC_ADPT_FMT" => ch:%u,%u,%u, ht:%u, vht:%u\n"
		, FUNC_ADPT_ARG(adapter), rtw_chdef->chan, rtw_chdef->bw, rtw_chdef->offset, ht, vht);
#endif

	rsn_ie = rtw_get_ie(setup->ie, WLAN_EID_RSN, &rsn_ie_len, setup->ie_len);
	if (rsn_ie && !rsn_ie_len) {
		rtw_warn_on(1);
		rsn_ie = NULL;
	}

	len = _BEACON_IE_OFFSET_
		+ 2 /* 0-length SSID */
		+ (n_bitrates >= 8 ? 8 : n_bitrates) + 2 /* Supported Rates */
		+ 3 /* DS parameter set */
		+ 6 /* TIM  */
		+ (n_bitrates > 8 ? n_bitrates - 8 + 2 : 0) /* Extended Supported Rates */
		+ (rsn_ie ? rsn_ie_len + 2 : 0) /* RSN */
		#if defined(CONFIG_80211N_HT)
		+ (ht ? HT_CAP_IE_LEN + 2 + HT_OP_IE_LEN + 2 : 0) /* HT */
		#endif
		#if defined(CONFIG_80211AC_VHT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
		+ (vht ? VHT_CAP_IE_LEN + 2 + VHT_OP_IE_LEN + 2 : 0) /* VHT */
		#endif
		+ minfo->mesh_id_len + 2 /* Mesh ID */
		+ 9 /* Mesh configuration */
		;

	ies = rtw_zmalloc(len);
	if (!ies)
		goto exit;

	/* timestamp */
	c = ies + 8;

	/* beacon interval */
	RTW_PUT_LE16(c , setup->beacon_interval);
	c += 2;

	/* capability */
	if (rsn_ie)
		*((u16 *)c) |= cpu_to_le16(cap_Privacy);
	c += 2;

	/* SSID */
	c = rtw_set_ie(c, WLAN_EID_SSID, 0, NULL, NULL);

	/* Supported Rates */
	for (i = 0; i < n_bitrates; i++) {
		supported_rates[i] = wiphy->bands[chan->band]->bitrates[i].bitrate / 5;
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
		if (setup->basic_rates & BIT(i))
		#else
		if (rtw_is_basic_rate_mix(supported_rates[i]))
		#endif
			supported_rates[i] |= IEEE80211_BASIC_RATE_MASK;
	}
	c = rtw_set_ie(c, WLAN_EID_SUPP_RATES, (n_bitrates >= 8 ? 8 : n_bitrates), supported_rates, NULL);

	/* DS parameter set */
	c = rtw_set_ie(c, WLAN_EID_DS_PARAMS, 1, &rtw_chdef->chan, NULL);

	/* TIM */
	*c = WLAN_EID_TIM;
	*(c + 1) = 4;
	c += 6;
	//c = rtw_set_ie(c, _TIM_IE_, 4, NULL, NULL);

	/* Extended Supported Rates */
	if (n_bitrates > 8)
		c = rtw_set_ie(c, WLAN_EID_EXT_SUPP_RATES, n_bitrates - 8, supported_rates + 8, NULL);

	/* RSN */
	if (rsn_ie)
		c = rtw_set_ie(c, WLAN_EID_RSN, rsn_ie_len, rsn_ie + 2, NULL);

#if defined(CONFIG_80211N_HT)
	if (ht) {
		struct ieee80211_sta_ht_cap *sta_ht_cap = &wiphy->bands[chan->band]->ht_cap;
		u8 ht_cap[HT_CAP_IE_LEN];
		u8 ht_op[HT_OP_IE_LEN];

		_rtw_memset(ht_cap, 0, HT_CAP_IE_LEN);
		_rtw_memset(ht_op, 0, HT_OP_IE_LEN);

		/* WLAN_EID_HT_CAP */
		RTW_PUT_LE16(HT_CAP_ELE_CAP_INFO(ht_cap), sta_ht_cap->cap);
		SET_HT_CAP_ELE_MAX_AMPDU_LEN_EXP(ht_cap, sta_ht_cap->ampdu_factor);
		SET_HT_CAP_ELE_MIN_MPDU_S_SPACE(ht_cap, sta_ht_cap->ampdu_density);
		_rtw_memcpy(HT_CAP_ELE_SUP_MCS_SET(ht_cap), &sta_ht_cap->mcs, 16);
		c = rtw_set_ie(c, WLAN_EID_HT_CAP, HT_CAP_IE_LEN, ht_cap, NULL);

		/* WLAN_EID_HT_OPERATION */
		SET_HT_OP_ELE_PRI_CHL(ht_op, rtw_chdef->chan);
		switch (rtw_chdef->offset) {
		case CHAN_OFFSET_UPPER:
			SET_HT_OP_ELE_2ND_CHL_OFFSET(ht_op, IEEE80211_SCA);
			break;
		case CHAN_OFFSET_LOWER:
			SET_HT_OP_ELE_2ND_CHL_OFFSET(ht_op, IEEE80211_SCB);
			break;
		case CHAN_OFFSET_NO_EXT:
		default:
			SET_HT_OP_ELE_2ND_CHL_OFFSET(ht_op, IEEE80211_SCN);
			break;
		}
		if (rtw_chdef->bw >= CHANNEL_WIDTH_40)
			SET_HT_OP_ELE_STA_CHL_WIDTH(ht_op, 1);
		else
			SET_HT_OP_ELE_STA_CHL_WIDTH(ht_op, 0);
		c = rtw_set_ie(c, WLAN_EID_HT_OPERATION, HT_OP_IE_LEN, ht_op, NULL);
	}
#endif /* defined(CONFIG_80211N_HT) */

#if defined(CONFIG_80211AC_VHT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	if (vht) {
		struct ieee80211_sta_vht_cap *sta_vht_cap = &wiphy->bands[chan->band]->vht_cap;
		u8 vht_cap[VHT_CAP_IE_LEN];
		u8 vht_op[VHT_OP_IE_LEN];
		u8 cch = rtw_phl_get_center_ch(&rtw_chdef);

		_rtw_memset(vht_op, 0, VHT_OP_IE_LEN);

		/* WLAN_EID_VHT_CAPABILITY */
		_rtw_memcpy(vht_cap, &sta_vht_cap->cap, 4);
		_rtw_memcpy(vht_cap + 4, &sta_vht_cap->vht_mcs, 8);
		c = rtw_set_ie(c, WLAN_EID_VHT_CAPABILITY, VHT_CAP_IE_LEN, vht_cap, NULL);

		/* WLAN_EID_VHT_OPERATION */
		if (rtw_chdef->bw < CHANNEL_WIDTH_80) {
			SET_VHT_OPERATION_ELE_CHL_WIDTH(vht_op, CH_WIDTH_20_40M);
			SET_VHT_OPERATION_ELE_CHL_CENTER_FREQ1(vht_op, 0);
			SET_VHT_OPERATION_ELE_CHL_CENTER_FREQ2(vht_op, 0);
		} else if (rtw_chdef->bw == CHANNEL_WIDTH_80) {
			SET_VHT_OPERATION_ELE_CHL_WIDTH(vht_op, CH_WIDTH_80_160M);
			SET_VHT_OPERATION_ELE_CHL_CENTER_FREQ1(vht_op, cch);
			SET_VHT_OPERATION_ELE_CHL_CENTER_FREQ2(vht_op, 0);
		} else {
			RTW_ERR(FUNC_ADPT_FMT" unsupported BW:%u\n", FUNC_ADPT_ARG(adapter), rtw_chdef->bw);
			rtw_warn_on(1);
			rtw_mfree(ies, len);
			goto exit;
		}

		/* Hard code 1 stream, MCS0-7 is a min Basic VHT MCS rates */
		vht_op[3] = 0xfc;
		vht_op[4] = 0xff;
		c = rtw_set_ie(c, WLAN_EID_VHT_OPERATION, VHT_OP_IE_LEN, vht_op, NULL);
	}
#endif /* defined(CONFIG_80211AC_VHT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)) */

	/* Mesh ID */
	c = rtw_set_ie_mesh_id(c, NULL, minfo->mesh_id, minfo->mesh_id_len);

	/* Mesh configuration */
	c = rtw_set_ie_mesh_config(c, NULL
		, minfo->mesh_pp_id
		, minfo->mesh_pm_id
		, minfo->mesh_cc_id
		, minfo->mesh_sp_id
		, minfo->mesh_auth_id
		, 0, 0, 0
		, 1
		, 0, 0
		, mcfg->dot11MeshForwarding
		, 0, 0, 0
	);

#if DBG_RTW_CFG80211_MESH_CONF
	RTW_INFO(FUNC_ADPT_FMT" ies_len:%u\n", FUNC_ADPT_ARG(adapter), len);
	dump_ies(RTW_DBGDUMP, ies + _BEACON_IE_OFFSET_, len - _BEACON_IE_OFFSET_);
#endif

exit:
	if (ies)
		*ies_len = len;
	return ies;
}

static int cfg80211_rtw_get_mesh_config(struct wiphy *wiphy, struct net_device *dev
	, struct mesh_config *conf)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	struct rtw_mesh_cfg *mesh_cfg = &adapter->mesh_cfg;
	int ret = 0;

	RTW_INFO(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(adapter));

	/* driver MPM */
	conf->dot11MeshRetryTimeout = 0;
	conf->dot11MeshConfirmTimeout = 0;
	conf->dot11MeshHoldingTimeout = 0;
	conf->dot11MeshMaxPeerLinks = mesh_cfg->max_peer_links;
	conf->dot11MeshMaxRetries = 0;

	conf->dot11MeshTTL = mesh_cfg->dot11MeshTTL;
	conf->element_ttl = mesh_cfg->element_ttl;

	/* driver MPM */
	conf->auto_open_plinks = 0;

	/* TBD: synchronization */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
	conf->dot11MeshNbrOffsetMaxNeighbor = 0;
#endif

	conf->dot11MeshHWMPmaxPREQretries = mesh_cfg->dot11MeshHWMPmaxPREQretries;
	conf->path_refresh_time = mesh_cfg->path_refresh_time;
	conf->min_discovery_timeout = mesh_cfg->min_discovery_timeout;
	conf->dot11MeshHWMPactivePathTimeout = mesh_cfg->dot11MeshHWMPactivePathTimeout;
	conf->dot11MeshHWMPpreqMinInterval = mesh_cfg->dot11MeshHWMPpreqMinInterval;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0))
	conf->dot11MeshHWMPperrMinInterval = mesh_cfg->dot11MeshHWMPperrMinInterval;
#endif
	conf->dot11MeshHWMPnetDiameterTraversalTime = mesh_cfg->dot11MeshHWMPnetDiameterTraversalTime;
	conf->dot11MeshHWMPRootMode = mesh_cfg->dot11MeshHWMPRootMode;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	conf->dot11MeshHWMPRannInterval = mesh_cfg->dot11MeshHWMPRannInterval;
#endif
	conf->dot11MeshGateAnnouncementProtocol = mesh_cfg->dot11MeshGateAnnouncementProtocol;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0))
	conf->dot11MeshForwarding = mesh_cfg->dot11MeshForwarding;
	conf->rssi_threshold = mesh_cfg->rssi_threshold;
#endif

	/* TBD */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
	conf->ht_opmode = 0xffff;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	conf->dot11MeshHWMPactivePathToRootTimeout = mesh_cfg->dot11MeshHWMPactivePathToRootTimeout;
	conf->dot11MeshHWMProotInterval = mesh_cfg->dot11MeshHWMProotInterval;
	conf->dot11MeshHWMPconfirmationInterval = mesh_cfg->dot11MeshHWMPconfirmationInterval;
#endif

	/* TBD: power save */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	conf->power_mode = NL80211_MESH_POWER_ACTIVE;
	conf->dot11MeshAwakeWindowDuration = 0;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	conf->plink_timeout = mesh_cfg->plink_timeout;
#endif

	return ret;
}

static void rtw_mbss_info_change_notify(_adapter *adapter, bool minfo_changed, bool need_work)
{
	if (need_work)
		rtw_mesh_work(&adapter->mesh_work);
}

static int cfg80211_rtw_update_mesh_config(struct wiphy *wiphy, struct net_device *dev
	, u32 mask, const struct mesh_config *nconf)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	int ret = 0;
	bool minfo_changed = _FALSE, need_work = _FALSE;
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *adapter_link = GET_PRIMARY_LINK(adapter);

	RTW_INFO(FUNC_ADPT_FMT" mask:0x%08x\n", FUNC_ADPT_ARG(adapter), mask);

	rtw_cfg80211_mesh_cfg_set(adapter, nconf, mask);
	rtw_update_beacon(adapter, adapter_link, WLAN_EID_MESH_CONFIG, NULL, _TRUE, 0);
#if CONFIG_RTW_MESH_CTO_MGATE_CARRIER
	if (rtw_mesh_cto_mgate_required(adapter))
		rtw_netif_carrier_off(adapter->pnetdev);
	else
		rtw_netif_carrier_on(adapter->pnetdev);
#endif
	need_work = rtw_ieee80211_mesh_root_setup(adapter);

	rtw_mbss_info_change_notify(adapter, minfo_changed, need_work);

	return ret;
}

static int cfg80211_rtw_join_mesh(struct wiphy *wiphy, struct net_device *dev,
	const struct mesh_config *conf, const struct mesh_setup *setup)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	u8 *ies = NULL;
	uint ies_len;
	int ret = 0;

	RTW_INFO(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(adapter));

#if DBG_RTW_CFG80211_MESH_CONF
	RTW_INFO(FUNC_ADPT_FMT" mesh_setup:\n", FUNC_ADPT_ARG(adapter));
	dump_mesh_setup(RTW_DBGDUMP, wiphy, setup);
	RTW_INFO(FUNC_ADPT_FMT" mesh_config:\n", FUNC_ADPT_ARG(adapter));
	dump_mesh_config(RTW_DBGDUMP, conf);
#endif

	if (rtw_cfg80211_sync_iftype(adapter) != _SUCCESS) {
		ret = -ENOTSUPP;
		goto exit;
	}

	/* initialization */
	rtw_mesh_init_mesh_info(adapter);

	/* apply cfg80211 settings*/
	rtw_cfg80211_mesh_info_set_profile(&adapter->mesh_info, setup);
	rtw_cfg80211_mesh_cfg_set(adapter, conf, 0xFFFFFFFF);

	/* apply cfg80211 settings (join only) */
	rtw_mesh_cfg_init_max_peer_links(adapter, conf->dot11MeshMaxPeerLinks);
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	rtw_mesh_cfg_init_plink_timeout(adapter, conf->plink_timeout);
	#endif

	rtw_ieee80211_mesh_root_setup(adapter);

	ies = rtw_cfg80211_construct_mesh_beacon_ies(wiphy, adapter, conf, setup, &ies_len);
	if (!ies) {
		ret = -EINVAL;
		goto exit;
	}

	/* start mbss */
	if (rtw_check_beacon_data(adapter, ies,  ies_len) != _SUCCESS) {
		ret = -EINVAL;
		goto exit;
	}
	
	rtw_mesh_work(&adapter->mesh_work);

exit:
	if (ies)
		rtw_mfree(ies, ies_len);
	if (ret)
		rtw_mesh_deinit_mesh_info(adapter);

	return ret;
}

static int cfg80211_rtw_leave_mesh(struct wiphy *wiphy, struct net_device *dev)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	int ret = 0;

	RTW_INFO(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(adapter));

	rtw_mesh_deinit_mesh_info(adapter);

	rtw_stop_ap_cmd(adapter, RTW_CMDF_WAIT_ACK);

	return ret;
}

static int cfg80211_rtw_add_mpath(struct wiphy *wiphy, struct net_device *dev
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0))
	, const u8 *dst, const u8 *next_hop
	#else
	, u8 *dst, u8 *next_hop
	#endif
)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	struct sta_priv *stapriv = &adapter->stapriv;
	struct sta_info *sta;
	struct rtw_mesh_path *mpath;
	int ret = 0;

	rtw_rcu_read_lock();

	sta = rtw_get_stainfo(stapriv, next_hop);
	if (!sta) {
		ret = -ENOENT;
		goto exit;
	}

	mpath = rtw_mesh_path_add(adapter, dst);
	if (!mpath) {
		ret = -ENOENT;
		goto exit;
	}

	rtw_mesh_path_fix_nexthop(mpath, sta);

exit:
	rtw_rcu_read_unlock();

	return ret;
}

static int cfg80211_rtw_del_mpath(struct wiphy *wiphy, struct net_device *dev
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0))
	, const u8 *dst
	#else
	, u8 *dst
	#endif
)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	int ret = 0;

	if (dst) {
		if (rtw_mesh_path_del(adapter, dst)) {
			ret = -ENOENT;
			goto exit;
		}
	} else {
		rtw_mesh_path_flush_by_iface(adapter);
	}	

exit:
	return ret;
}

static int cfg80211_rtw_change_mpath(struct wiphy *wiphy, struct net_device *dev
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0))
	, const u8 *dst, const u8 *next_hop
	#else
	, u8 *dst, u8 *next_hop
	#endif
)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	struct sta_priv *stapriv = &adapter->stapriv;
	struct sta_info *sta;
	struct rtw_mesh_path *mpath;
	int ret = 0;

	rtw_rcu_read_lock();

	sta = rtw_get_stainfo(stapriv, next_hop);
	if (!sta) {
		ret = -ENOENT;
		goto exit;
	}

	mpath = rtw_mesh_path_lookup(adapter, dst);
	if (!mpath) {
		ret = -ENOENT;
		goto exit;
	}

	rtw_mesh_path_fix_nexthop(mpath, sta);

exit:
	rtw_rcu_read_unlock();

	return ret;
}

static void rtw_cfg80211_mpath_set_pinfo(struct rtw_mesh_path *mpath, u8 *next_hop, struct mpath_info *pinfo)
{
	struct sta_info *next_hop_sta = rtw_rcu_dereference(mpath->next_hop);

	if (next_hop_sta)
		_rtw_memcpy(next_hop, next_hop_sta->phl_sta->mac_addr, ETH_ALEN);
	else
		_rtw_memset(next_hop, 0, ETH_ALEN);

	_rtw_memset(pinfo, 0, sizeof(*pinfo));

	pinfo->generation = mpath->adapter->mesh_info.mesh_paths_generation;

	pinfo->filled = 0
		| MPATH_INFO_FRAME_QLEN
		| MPATH_INFO_SN
		| MPATH_INFO_METRIC
		| MPATH_INFO_EXPTIME
		| MPATH_INFO_DISCOVERY_TIMEOUT
		| MPATH_INFO_DISCOVERY_RETRIES
		| MPATH_INFO_FLAGS
		;

	pinfo->frame_qlen = mpath->frame_queue_len;
	pinfo->sn = mpath->sn;
	pinfo->metric = mpath->metric;
	if (rtw_time_after(mpath->exp_time, rtw_get_current_time()))
		pinfo->exptime = rtw_get_remaining_time_ms(mpath->exp_time);
	pinfo->discovery_timeout = rtw_systime_to_ms(mpath->discovery_timeout);
	pinfo->discovery_retries = mpath->discovery_retries;
	if (mpath->flags & RTW_MESH_PATH_ACTIVE)
		pinfo->flags |= NL80211_MPATH_FLAG_ACTIVE;
	if (mpath->flags & RTW_MESH_PATH_RESOLVING)
		pinfo->flags |= NL80211_MPATH_FLAG_RESOLVING;
	if (mpath->flags & RTW_MESH_PATH_SN_VALID)
		pinfo->flags |= NL80211_MPATH_FLAG_SN_VALID;
	if (mpath->flags & RTW_MESH_PATH_FIXED)
		pinfo->flags |= NL80211_MPATH_FLAG_FIXED;
	if (mpath->flags & RTW_MESH_PATH_RESOLVED)
		pinfo->flags |= NL80211_MPATH_FLAG_RESOLVED;
}

static int cfg80211_rtw_get_mpath(struct wiphy *wiphy, struct net_device *dev, u8 *dst, u8 *next_hop, struct mpath_info *pinfo)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	struct rtw_mesh_path *mpath;
	int ret = 0;

	rtw_rcu_read_lock();

	mpath = rtw_mesh_path_lookup(adapter, dst);
	if (!mpath) {
		ret = -ENOENT;
		goto exit;
	}

	rtw_cfg80211_mpath_set_pinfo(mpath, next_hop, pinfo);

exit:
	rtw_rcu_read_unlock();

	return ret;
}

static int cfg80211_rtw_dump_mpath(struct wiphy *wiphy, struct net_device *dev, int idx, u8 *dst, u8 *next_hop, struct mpath_info *pinfo)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	struct rtw_mesh_path *mpath;
	int ret = 0;

	rtw_rcu_read_lock();

	mpath = rtw_mesh_path_lookup_by_idx(adapter, idx);
	if (!mpath) {
		ret = -ENOENT;
		goto exit;
	}

	_rtw_memcpy(dst, mpath->dst, ETH_ALEN);
	rtw_cfg80211_mpath_set_pinfo(mpath, next_hop, pinfo);

exit:
	rtw_rcu_read_unlock();

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0))
static void rtw_cfg80211_mpp_set_pinfo(struct rtw_mesh_path *mpath, u8 *mpp, struct mpath_info *pinfo)
{
	_rtw_memcpy(mpp, mpath->mpp, ETH_ALEN);

	_rtw_memset(pinfo, 0, sizeof(*pinfo));
	pinfo->generation = mpath->adapter->mesh_info.mpp_paths_generation;
}

static int cfg80211_rtw_get_mpp(struct wiphy *wiphy, struct net_device *dev, u8 *dst, u8 *mpp, struct mpath_info *pinfo)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	struct rtw_mesh_path *mpath;
	int ret = 0;

	rtw_rcu_read_lock();

	mpath = rtw_mpp_path_lookup(adapter, dst);
	if (!mpath) {
		ret = -ENOENT;
		goto exit;
	}

	rtw_cfg80211_mpp_set_pinfo(mpath, mpp, pinfo);

exit:
	rtw_rcu_read_unlock();

	return ret;
}

static int cfg80211_rtw_dump_mpp(struct wiphy *wiphy, struct net_device *dev, int idx, u8 *dst, u8 *mpp, struct mpath_info *pinfo)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(dev);
	struct rtw_mesh_path *mpath;
	int ret = 0;

	rtw_rcu_read_lock();

	mpath = rtw_mpp_path_lookup_by_idx(adapter, idx);
	if (!mpath) {
		ret = -ENOENT;
		goto exit;
	}

	_rtw_memcpy(dst, mpath->dst, ETH_ALEN);
	rtw_cfg80211_mpp_set_pinfo(mpath, mpp, pinfo);

exit:
	rtw_rcu_read_unlock();

	return ret;
}
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)) */

#endif /* defined(CONFIG_RTW_MESH) */

#if defined(CONFIG_PNO_SUPPORT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
static int cfg80211_rtw_sched_scan_start(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_sched_scan_request *request)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	struct cfg80211_ssid ssids[MAX_NLO_NUM];
	int n_ssids = 0;
	u32 interval = 0;
	u32 iterations = 0;
	u32 slow_interval = 0;
	u32 delay = 0;
	int i = 0;
	u8 ret = 0;

	RTW_INFO(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(padapter));

	if (padapter->netif_up == _FALSE) {
		RTW_INFO("%s: net device is down.\n", __func__);
		ret = -EIO;
		goto exit;
	}

	if (check_fwstate(pmlmepriv, WIFI_UNDER_SURVEY) == _TRUE ||
		check_fwstate(pmlmepriv, WIFI_ASOC_STATE) == _TRUE  ||
		check_fwstate(pmlmepriv, WIFI_UNDER_LINKING) == _TRUE) {
		RTW_INFO("%s: device is busy.\n", __func__);
		rtw_scan_abort(padapter, 0);
	}

	if (request == NULL) {
		RTW_INFO("%s: invalid cfg80211_requests parameters.\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
	/* Delay in seconds to use before starting the first scan */
	delay = request->delay ? request->delay : NLO_DEFAULT_SCAN_DELAY;
#endif

	/* Prepare scan interval (in second) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
	interval = request->scan_plans[0].interval;
	if (request->scan_plans[0].iterations) {
		iterations = request->scan_plans[0].iterations;
		if (request->n_scan_plans == MAX_NLO_SCAN_PLANS)
			slow_interval = request->scan_plans[1].interval;
		else
			slow_interval = interval;
	} else {
		iterations = 1;
		slow_interval = interval;
	}
#else
	interval = request->interval;
	iterations = 1;
	slow_interval = interval;
#endif

	/* Prepare SSIDs to be scanned */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
	n_ssids = rtw_min(request->n_match_sets, MAX_NLO_NUM);
	for (i = 0; i < n_ssids; i++) {
		_rtw_memcpy((void *)&ssids[i],
			    (void *)&request->match_sets[i].ssid,
			    sizeof(struct cfg80211_ssid));
	}
#else
	n_ssids = rtw_min(request->n_ssids, MAX_NLO_NUM);
	for (i = 0; i < n_ssids; i++) {
		_rtw_memcpy((void *)&ssids[i],
			    (void *)&request->ssids[i].ssid,
			    sizeof(struct cfg80211_ssid));
	}
#endif

	ret = rtw_nlo_enable(dev, ssids, n_ssids,
			     request->channels, request->n_channels, delay,
			     interval, iterations, slow_interval);

exit:
	return ret;
}

static int cfg80211_rtw_sched_scan_stop(struct wiphy *wiphy,
		struct net_device *dev
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)) || defined (COMPAT_KERNEL_RELEASE_4_19)
		, u64 reqid
#endif
)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);

	RTW_INFO(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(padapter));

	return rtw_nlo_disable(dev);
}

int	cfg80211_rtw_suspend(struct wiphy *wiphy, struct cfg80211_wowlan *wow) {
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	struct wow_priv *wowpriv = adapter_to_wowlan(padapter);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;

	RTW_DBG("==> %s\n",__func__);

	if (wowpriv->wow_nlo.nlo_en &&
	    !check_fwstate(pmlmepriv, WIFI_ASOC_STATE))
		pwrpriv->wowlan_pno_enable = _TRUE;

	RTW_DBG("<== %s\n",__func__);
	return 0;
}

int	cfg80211_rtw_resume(struct wiphy *wiphy) {

	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	struct wow_priv *wowpriv = adapter_to_wowlan(padapter);
	struct rtw_nlo_info *wow_nlo = &wowpriv->wow_nlo;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct sitesurvey_parm parm;
	int i, len;


	RTW_DBG("==> %s\n",__func__);
	if (pwrpriv->wowlan_pno_enable) {

		struct rtw_wdev_priv *pwdev_priv = adapter_wdev_data(padapter);
		int PNOWakeupScanWaitCnt = 0;

		rtw_cfg80211_disconnected(padapter->rtw_wdev, 0, NULL, 0, 1, GFP_ATOMIC);

		rtw_init_sitesurvey_parm(padapter, &parm);
		for (i=0;i<wow_nlo->num_of_networks && i < RTW_SSID_SCAN_AMOUNT; i++) {
			len = wow_nlo->ssidlen[i];
			_rtw_memcpy(&parm.ssid[i].Ssid, wow_nlo->ssid[i], len);
			parm.ssid[i].SsidLength = len;
		}
		parm.ssid_num = i;

		_rtw_spinlock_bh(&pmlmepriv->lock);
		//This modification fix PNO wakeup reconnect issue with hidden SSID AP.
		//rtw_sitesurvey_cmd(padapter, NULL);
		rtw_sitesurvey_cmd(padapter, &parm);
		_rtw_spinunlock_bh(&pmlmepriv->lock);

		for (PNOWakeupScanWaitCnt = 0; PNOWakeupScanWaitCnt < 10; PNOWakeupScanWaitCnt++) {
			if(check_fwstate(pmlmepriv, WIFI_UNDER_SURVEY) == _FALSE)
				break;
			rtw_msleep_os(1000);
		}

		_rtw_spinlock_bh(&pmlmepriv->lock);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)) || defined(COMPAT_KERNEL_RELEASE_4_19)
		cfg80211_sched_scan_results(padapter->rtw_wdev->wiphy, 0);
#else
		cfg80211_sched_scan_results(padapter->rtw_wdev->wiphy);
#endif
		_rtw_spinunlock_bh(&pmlmepriv->lock);

		pwrpriv->wowlan_pno_enable = _FALSE;
	}
	RTW_DBG("<== %s\n",__func__);
	return 0;
}
#endif /* CONFIG_PNO_SUPPORT */

static int rtw_cfg80211_set_beacon_wpsp2pie(struct net_device *ndev, char *buf, int len)
{
	int ret = 0;
	uint wps_ielen = 0;
	u8 *wps_ie;
	u32	p2p_ielen = 0;
	u8 wps_oui[8] = {0x0, 0x50, 0xf2, 0x04};
	u8 *p2p_ie;
	u32	wfd_ielen = 0;
	u8 *wfd_ie;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	struct link_mlme_ext_priv *pmlmeext = &(padapter_link->mlmeextpriv);

	RTW_INFO(FUNC_NDEV_FMT" ielen=%d\n", FUNC_NDEV_ARG(ndev), len);

	if (len > 0) {
		wps_ie = rtw_get_wps_ie(buf, len, NULL, &wps_ielen);
		if (wps_ie) {
			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("bcn_wps_ielen=%d\n", wps_ielen);
			#endif

			if (pmlmepriv->wps_beacon_ie) {
				u32 free_len = pmlmepriv->wps_beacon_ie_len;
				pmlmepriv->wps_beacon_ie_len = 0;
				rtw_mfree(pmlmepriv->wps_beacon_ie, free_len);
				pmlmepriv->wps_beacon_ie = NULL;
			}

			pmlmepriv->wps_beacon_ie = rtw_malloc(wps_ielen);
			if (pmlmepriv->wps_beacon_ie == NULL) {
				RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
				return -EINVAL;

			}

			_rtw_memcpy(pmlmepriv->wps_beacon_ie, wps_ie, wps_ielen);
			pmlmepriv->wps_beacon_ie_len = wps_ielen;

			rtw_update_beacon(padapter, padapter_link,
					_VENDOR_SPECIFIC_IE_, wps_oui, _TRUE, RTW_CMDF_WAIT_ACK);

		}

		/* buf += wps_ielen; */
		/* len -= wps_ielen; */

		#ifdef CONFIG_P2P
		p2p_ie = rtw_get_p2p_ie(buf, len, NULL, &p2p_ielen);
		if (p2p_ie) {
			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("bcn_p2p_ielen=%d\n", p2p_ielen);
			#endif

			if (pmlmepriv->p2p_beacon_ie) {
				u32 free_len = pmlmepriv->p2p_beacon_ie_len;
				pmlmepriv->p2p_beacon_ie_len = 0;
				rtw_mfree(pmlmepriv->p2p_beacon_ie, free_len);
				pmlmepriv->p2p_beacon_ie = NULL;
			}

			pmlmepriv->p2p_beacon_ie = rtw_malloc(p2p_ielen);
			if (pmlmepriv->p2p_beacon_ie == NULL) {
				RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
				return -EINVAL;

			}

			_rtw_memcpy(pmlmepriv->p2p_beacon_ie, p2p_ie, p2p_ielen);
			pmlmepriv->p2p_beacon_ie_len = p2p_ielen;

		}
		#endif /* CONFIG_P2P */


		#ifdef CONFIG_WFD
		wfd_ie = rtw_get_wfd_ie(buf, len, NULL, &wfd_ielen);
		if (wfd_ie) {
			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("bcn_wfd_ielen=%d\n", wfd_ielen);
			#endif

			if (rtw_mlme_update_wfd_ie_data(pmlmepriv, MLME_BEACON_IE, wfd_ie, wfd_ielen) != _SUCCESS)
				return -EINVAL;
		}
		#endif /* CONFIG_WFD */

		pmlmeext->bstart_bss = _TRUE;

	}

	return ret;

}

static int rtw_cfg80211_set_probe_resp_wpsp2pie(struct net_device *net, char *buf, int len)
{
	int ret = 0;
	uint wps_ielen = 0;
	u8 *wps_ie;
	u32	p2p_ielen = 0;
	u8 *p2p_ie;
	u32 vendor_ielen = 0;
	u8 *vendor_ie;
	u32	wfd_ielen = 0;
	u8 *wfd_ie;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(net);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO("%s, ielen=%d\n", __func__, len);
#endif

	if (len > 0) {
		wps_ie = rtw_get_wps_ie(buf, len, NULL, &wps_ielen);
		if (wps_ie) {
			uint	attr_contentlen = 0;
			u16	uconfig_method, *puconfig_method = NULL;

			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("probe_resp_wps_ielen=%d\n", wps_ielen);
			#endif

			if (check_fwstate(pmlmepriv, WIFI_UNDER_WPS)) {
				u8 sr = 0;
				rtw_get_wps_attr_content(wps_ie,  wps_ielen, WPS_ATTR_SELECTED_REGISTRAR, (u8 *)(&sr), NULL);

				if (sr != 0)
					RTW_INFO("%s, got sr\n", __func__);
				else {
					RTW_INFO("GO mode process WPS under site-survey,  sr no set\n");
					return ret;
				}
			}

			if (pmlmepriv->wps_probe_resp_ie) {
				u32 free_len = pmlmepriv->wps_probe_resp_ie_len;
				pmlmepriv->wps_probe_resp_ie_len = 0;
				rtw_mfree(pmlmepriv->wps_probe_resp_ie, free_len);
				pmlmepriv->wps_probe_resp_ie = NULL;
			}

			pmlmepriv->wps_probe_resp_ie = rtw_malloc(wps_ielen);
			if (pmlmepriv->wps_probe_resp_ie == NULL) {
				RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
				return -EINVAL;

			}

			/* add PUSH_BUTTON config_method by driver self in wpsie of probe_resp at GO Mode */
			puconfig_method = (u16 *)rtw_get_wps_attr_content(wps_ie, wps_ielen, WPS_ATTR_CONF_METHOD , NULL, &attr_contentlen);
			if (puconfig_method != NULL) {
				/* struct registry_priv *pregistrypriv = &padapter->registrypriv; */
				struct wireless_dev *wdev = padapter->rtw_wdev;

				#ifdef CONFIG_DEBUG_CFG80211
				/* printk("config_method in wpsie of probe_resp = 0x%x\n", be16_to_cpu(*puconfig_method)); */
				#endif

				#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
				/* for WIFI-DIRECT LOGO 4.2.2, AUTO GO can't set PUSH_BUTTON flags */
				if (wdev->iftype == NL80211_IFTYPE_P2P_GO) {
					uconfig_method = WPS_CM_PUSH_BUTTON;
					uconfig_method = cpu_to_be16(uconfig_method);

					*puconfig_method &= ~uconfig_method;
				}
				#endif
			}

			_rtw_memcpy(pmlmepriv->wps_probe_resp_ie, wps_ie, wps_ielen);
			pmlmepriv->wps_probe_resp_ie_len = wps_ielen;

		}

		/* buf += wps_ielen; */
		/* len -= wps_ielen; */

		vendor_ie = rtw_get_vendor_ie(buf, len, NULL, &vendor_ielen);
		if (vendor_ie) {
			pmlmepriv->probe_rsp = rtw_malloc(vendor_ielen);
			if (pmlmepriv->probe_rsp == NULL) {
				RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
				return -EINVAL;
			}
			_rtw_memcpy(pmlmepriv->probe_rsp, vendor_ie, vendor_ielen);
			pmlmepriv->probe_rsp_len = vendor_ielen;
		}

		#ifdef CONFIG_P2P
		p2p_ie = rtw_get_p2p_ie(buf, len, NULL, &p2p_ielen);
		if (p2p_ie) {
			u8 is_GO = _FALSE;
			u32 attr_contentlen = 0;
			u16 cap_attr = 0;

			#ifdef CONFIG_DEBUG_CFG80211
			RTW_INFO("probe_resp_p2p_ielen=%d\n", p2p_ielen);
			#endif

			/* Check P2P Capability ATTR */
			attr_contentlen = sizeof(cap_attr);
			if (rtw_get_p2p_attr_content(p2p_ie, p2p_ielen, P2P_ATTR_CAPABILITY, (u8 *)&cap_attr, (uint *) &attr_contentlen)) {
				u8 grp_cap = 0;
				/* RTW_INFO( "[%s] Got P2P Capability Attr!!\n", __FUNCTION__ ); */
				cap_attr = le16_to_cpu(cap_attr);
				grp_cap = (u8)((cap_attr >> 8) & 0xff);

				is_GO = (grp_cap & BIT(0)) ? _TRUE : _FALSE;

				if (is_GO)
					RTW_INFO("Got P2P Capability Attr, grp_cap=0x%x, is_GO\n", grp_cap);
			}


			if (is_GO == _FALSE) {
				if (pmlmepriv->p2p_probe_resp_ie) {
					u32 free_len = pmlmepriv->p2p_probe_resp_ie_len;
					pmlmepriv->p2p_probe_resp_ie_len = 0;
					rtw_mfree(pmlmepriv->p2p_probe_resp_ie, free_len);
					pmlmepriv->p2p_probe_resp_ie = NULL;
				}

				pmlmepriv->p2p_probe_resp_ie = rtw_malloc(p2p_ielen);
				if (pmlmepriv->p2p_probe_resp_ie == NULL) {
					RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
					return -EINVAL;

				}
				_rtw_memcpy(pmlmepriv->p2p_probe_resp_ie, p2p_ie, p2p_ielen);
				pmlmepriv->p2p_probe_resp_ie_len = p2p_ielen;
			} else {
				if (pmlmepriv->p2p_go_probe_resp_ie) {
					u32 free_len = pmlmepriv->p2p_go_probe_resp_ie_len;
					pmlmepriv->p2p_go_probe_resp_ie_len = 0;
					rtw_mfree(pmlmepriv->p2p_go_probe_resp_ie, free_len);
					pmlmepriv->p2p_go_probe_resp_ie = NULL;
				}

				pmlmepriv->p2p_go_probe_resp_ie = rtw_malloc(p2p_ielen);
				if (pmlmepriv->p2p_go_probe_resp_ie == NULL) {
					RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
					return -EINVAL;

				}
				_rtw_memcpy(pmlmepriv->p2p_go_probe_resp_ie, p2p_ie, p2p_ielen);
				pmlmepriv->p2p_go_probe_resp_ie_len = p2p_ielen;
			}

		}
		#endif /* CONFIG_P2P */


		#ifdef CONFIG_WFD
		wfd_ie = rtw_get_wfd_ie(buf, len, NULL, &wfd_ielen);
		#ifdef CONFIG_DEBUG_CFG80211
		RTW_INFO("probe_resp_wfd_ielen=%d\n", wfd_ielen);
		#endif

		if (rtw_mlme_update_wfd_ie_data(pmlmepriv, MLME_PROBE_RESP_IE, wfd_ie, wfd_ielen) != _SUCCESS)
			return -EINVAL;
		#endif /* CONFIG_WFD */

	}

	return ret;

}

static int rtw_cfg80211_set_assoc_resp_wpsp2pie(struct net_device *net, char *buf, int len)
{
	int ret = 0;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(net);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	u8 *ie;
	u32 ie_len;

	RTW_INFO("%s, ielen=%d\n", __func__, len);

	if (len <= 0)
		goto exit;

	ie = rtw_get_wps_ie(buf, len, NULL, &ie_len);
	if (ie && ie_len) {
		if (pmlmepriv->wps_assoc_resp_ie) {
			u32 free_len = pmlmepriv->wps_assoc_resp_ie_len;

			pmlmepriv->wps_assoc_resp_ie_len = 0;
			rtw_mfree(pmlmepriv->wps_assoc_resp_ie, free_len);
			pmlmepriv->wps_assoc_resp_ie = NULL;
		}

		pmlmepriv->wps_assoc_resp_ie = rtw_malloc(ie_len);
		if (pmlmepriv->wps_assoc_resp_ie == NULL) {
			RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
			return -EINVAL;
		}
		_rtw_memcpy(pmlmepriv->wps_assoc_resp_ie, ie, ie_len);
		pmlmepriv->wps_assoc_resp_ie_len = ie_len;
	}

	ie = rtw_get_p2p_ie(buf, len, NULL, &ie_len);
	if (ie && ie_len) {
		if (pmlmepriv->p2p_assoc_resp_ie) {
			u32 free_len = pmlmepriv->p2p_assoc_resp_ie_len;

			pmlmepriv->p2p_assoc_resp_ie_len = 0;
			rtw_mfree(pmlmepriv->p2p_assoc_resp_ie, free_len);
			pmlmepriv->p2p_assoc_resp_ie = NULL;
		}

		pmlmepriv->p2p_assoc_resp_ie = rtw_malloc(ie_len);
		if (pmlmepriv->p2p_assoc_resp_ie == NULL) {
			RTW_INFO("%s()-%d: rtw_malloc() ERROR!\n", __FUNCTION__, __LINE__);
			return -EINVAL;
		}
		_rtw_memcpy(pmlmepriv->p2p_assoc_resp_ie, ie, ie_len);
		pmlmepriv->p2p_assoc_resp_ie_len = ie_len;
	}

#ifdef CONFIG_WFD
	ie = rtw_get_wfd_ie(buf, len, NULL, &ie_len);
	if (rtw_mlme_update_wfd_ie_data(pmlmepriv, MLME_ASSOC_RESP_IE, ie, ie_len) != _SUCCESS)
		return -EINVAL;
#endif

exit:
	return ret;
}

int rtw_cfg80211_set_mgnt_wpsp2pie(struct net_device *net, char *buf, int len,
	int type)
{
	int ret = 0;
	uint wps_ielen = 0;
	u32	p2p_ielen = 0;

#ifdef CONFIG_DEBUG_CFG80211
	RTW_INFO("%s, ielen=%d\n", __func__, len);
#endif

	if ((rtw_get_wps_ie(buf, len, NULL, &wps_ielen) && (wps_ielen > 0))
		#ifdef CONFIG_P2P
		|| (rtw_get_p2p_ie(buf, len, NULL, &p2p_ielen) && (p2p_ielen > 0))
		#endif
	) {
		if (net != NULL) {
			switch (type) {
			case 0x1: /* BEACON */
				ret = rtw_cfg80211_set_beacon_wpsp2pie(net, buf, len);
				break;
			case 0x2: /* PROBE_RESP */
				ret = rtw_cfg80211_set_probe_resp_wpsp2pie(net, buf, len);
				#ifdef CONFIG_P2P
				if (ret == 0)
					adapter_wdev_data((_adapter *)rtw_netdev_priv(net))->probe_resp_ie_update_time = rtw_get_current_time();
				#endif
				break;
			case 0x4: /* ASSOC_RESP */
				ret = rtw_cfg80211_set_assoc_resp_wpsp2pie(net, buf, len);
				break;
			}
		}
	}

	return ret;

}

#ifdef CONFIG_80211N_HT
static void rtw_cfg80211_init_ht_capab_ex(_adapter *padapter,
	struct ieee80211_sta_ht_cap *ht_cap, enum band_type band, u8 rf_type,
	struct protocol_cap_t *dft_proto_cap,
	struct role_link_cap_t *dft_cap)
{
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	struct ht_priv		*phtpriv = &pmlmepriv->dev_htpriv;
	u8 stbc_rx_enable = _FALSE;

	rtw_ht_get_dft_setting(padapter, dft_proto_cap, dft_cap);

	/* RX LDPC */
	if (TEST_FLAG(phtpriv->ldpc_cap, LDPC_HT_ENABLE_RX))
		ht_cap->cap |= IEEE80211_HT_CAP_LDPC_CODING;

	/* TX STBC */
	if (TEST_FLAG(phtpriv->stbc_cap, STBC_HT_ENABLE_TX))
		ht_cap->cap |= IEEE80211_HT_CAP_TX_STBC;

	/* RX STBC */
	if (TEST_FLAG(phtpriv->stbc_cap, STBC_HT_ENABLE_RX)) {
		switch (rf_type) {
		case RF_1T1R:
			ht_cap->cap |= IEEE80211_HT_CAP_RX_STBC_1R;/*RX STBC One spatial stream*/
			break;
		case RF_2T2R:
		case RF_1T2R:
			ht_cap->cap |= IEEE80211_HT_CAP_RX_STBC_1R;/* Only one spatial-stream STBC RX is supported */
			break;
		case RF_3T3R:
		case RF_3T4R:
		case RF_4T4R:
			ht_cap->cap |= IEEE80211_HT_CAP_RX_STBC_1R;/* Only one spatial-stream STBC RX is supported */
			break;
		default:
			RTW_INFO("[warning] rf_type %d is not expected\n", rf_type);
			break;
		}
	}
}

static void rtw_cfg80211_init_ht_capab(_adapter *padapter,
	struct ieee80211_sta_ht_cap *ht_cap, enum band_type band, u8 rf_type,
	struct protocol_cap_t *dft_proto_cap,
	struct role_link_cap_t *dft_cap)
{
	struct registry_priv *regsty = &padapter->registrypriv;
	u8 rx_nss = 0;

	if (!regsty->ht_enable || !is_supported_ht(regsty->wireless_mode))
		return;

	ht_cap->ht_supported = 1;

	ht_cap->cap = IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
				IEEE80211_HT_CAP_SGI_40 | IEEE80211_HT_CAP_SGI_20 |
				IEEE80211_HT_CAP_DSSSCCK40 | IEEE80211_HT_CAP_MAX_AMSDU;
	rtw_cfg80211_init_ht_capab_ex(padapter, ht_cap, band, rf_type,
							dft_proto_cap, dft_cap);

	/*
	 *Maximum length of AMPDU that the STA can receive.
	 *Length = 2 ^ (13 + max_ampdu_length_exp) - 1 (octets)
	 */
	ht_cap->ampdu_factor = IEEE80211_HT_MAX_AMPDU_64K;

	/*Minimum MPDU start spacing , */
	ht_cap->ampdu_density = IEEE80211_HT_MPDU_DENSITY_16;

	ht_cap->mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;

	rx_nss = GET_PHY_RX_NSS_BY_BAND(adapter_to_dvobj(padapter), HW_BAND_0);
	switch (rx_nss) {
	case 1:
		ht_cap->mcs.rx_mask[0] = 0xFF;
		break;
	case 2:
		ht_cap->mcs.rx_mask[0] = 0xFF;
		ht_cap->mcs.rx_mask[1] = 0xFF;
		break;
	case 3:
		ht_cap->mcs.rx_mask[0] = 0xFF;
		ht_cap->mcs.rx_mask[1] = 0xFF;
		ht_cap->mcs.rx_mask[2] = 0xFF;
		break;
	case 4:
		ht_cap->mcs.rx_mask[0] = 0xFF;
		ht_cap->mcs.rx_mask[1] = 0xFF;
		ht_cap->mcs.rx_mask[2] = 0xFF;
		ht_cap->mcs.rx_mask[3] = 0xFF;
		break;
	default:
		RTW_ERR("%s, error rf_type=%d, rx_nss=%d\n", __func__, rf_type, rx_nss);
		rtw_warn_on(1);
	};

	ht_cap->mcs.rx_highest = cpu_to_le16(
		rtw_ht_mcs_rate(rtw_hw_is_bw_support(adapter_to_dvobj(padapter), CHANNEL_WIDTH_40)
			, rtw_hw_is_bw_support(adapter_to_dvobj(padapter), CHANNEL_WIDTH_40) ? ht_cap->cap & IEEE80211_HT_CAP_SGI_40 : ht_cap->cap & IEEE80211_HT_CAP_SGI_20
			, ht_cap->mcs.rx_mask) / 10);
}
#endif /* CONFIG_80211N_HT */

#if defined(CONFIG_80211AC_VHT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
static void rtw_cfg80211_init_vht_capab(_adapter *padapter,
	struct ieee80211_sta_vht_cap *sta_vht_cap,
	enum band_type band, u8 rf_type, struct protocol_cap_t *dft_proto_cap,
	struct role_link_cap_t *dft_cap)
{
	struct registry_priv *regsty = &padapter->registrypriv;
	u8 vht_cap_ie[2 + 12] = {0};

	if (!REGSTY_IS_11AC_ENABLE(regsty) || !is_supported_vht(regsty->wireless_mode))
		return;

	rtw_vht_get_dft_setting(padapter, dft_proto_cap, dft_cap);

	rtw_get_dft_vht_cap_ie(padapter, vht_cap_ie);

	sta_vht_cap->vht_supported = 1;

	_rtw_memcpy(&sta_vht_cap->cap, vht_cap_ie + 2, 4);
	_rtw_memcpy(&sta_vht_cap->vht_mcs, vht_cap_ie + 2 + 4, 8);
}
#endif /* defined(CONFIG_80211AC_VHT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)) */

#if defined(CONFIG_80211AX_HE) && (defined(CPTCFG_VERSION) || (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)))
static int rtw_cfg80211_init_he_capab(_adapter *padapter,
	struct ieee80211_sband_iftype_data *sta_iface_data,
	enum nl80211_iftype iftype, struct phy_cap_t *phy_cap,
	struct protocol_cap_t *dft_proto_cap, enum nl80211_band band)
{
	struct ieee80211_sta_he_cap *sta_he_cap = &(sta_iface_data->he_cap);
	void *phl = GET_PHL_INFO(adapter_to_dvobj(padapter));
	struct registry_priv *regsty = &padapter->registrypriv;
	u8 cap_len = 0;
	u8 he_mcs_set_ext_len = 0;
	u8 he_cap_ie[HE_CAP_ELE_MAX_LEN] = {0};
#if defined(CONFIG_IEEE80211_BAND_6GHZ) && (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
	u8 he_6g_band_cap_ie[HE_6G_BAND_CAP_MAX_LEN] = {0};
	enum role_type role_type;
#endif /* CONFIG_IEEE80211_BAND_6GHZ && (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)) */
	u8 ofst_80m = 3 + HE_CAP_ELE_MAC_CAP_LEN + HE_CAP_ELE_PHY_CAP_LEN;
	u8 ofst_160m = ofst_80m + 4;
	u8 ofst_80p80m = ofst_80m + 4;

	if (!REGSTY_IS_11AX_ENABLE(regsty) || !is_supported_he(regsty->wireless_mode))
		return _FAIL;


	sta_iface_data->types_mask = BIT(iftype);

	cap_len = rtw_get_dft_he_cap_ie(padapter, phy_cap, dft_proto_cap, he_cap_ie, nl80211_band_to_rtw_band(band));

	sta_he_cap->has_he = 1;

	/* mac & phy capability info */
	_rtw_memcpy(&sta_he_cap->he_cap_elem.mac_cap_info, he_cap_ie + 3, HE_CAP_ELE_MAC_CAP_LEN);
	_rtw_memcpy(&sta_he_cap->he_cap_elem.phy_cap_info,
	            he_cap_ie + 3 + HE_CAP_ELE_MAC_CAP_LEN,
				HE_CAP_ELE_PHY_CAP_LEN);

#if defined(CONFIG_IEEE80211_BAND_6GHZ) && (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
	/* 6G band cap */
	role_type = nl80211_iftype_to_rtw_phl_role_type(iftype);

	cap_len = rtw_build_he_6g_band_cap_ie_by_proto(padapter, role_type,
						       dft_proto_cap, he_6g_band_cap_ie);

	_rtw_memcpy(&sta_iface_data->he_6ghz_capa,
	            he_6g_band_cap_ie + 1, HE_6G_BAND_CAP_INFO_LEN);
#endif /* CONFIG_IEEE80211_BAND_6GHZ && (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)) */

	/* Supported HE-MCS And NSS Set */
	/* HE supports 80M BW */
	_rtw_memcpy(&sta_he_cap->he_mcs_nss_supp.rx_mcs_80, he_cap_ie + ofst_80m, 2);
	_rtw_memcpy(&sta_he_cap->he_mcs_nss_supp.tx_mcs_80, he_cap_ie + ofst_80m + 2, 2);

	/* HE supports 160M BW */
	if(GET_HE_PHY_CAP_SUPPORT_CHAN_WIDTH_SET(he_cap_ie + 3 + HE_CAP_ELE_MAC_CAP_LEN) & BIT2) {
		_rtw_memcpy(&sta_he_cap->he_mcs_nss_supp.rx_mcs_160, he_cap_ie + ofst_160m, 2);
		_rtw_memcpy(&sta_he_cap->he_mcs_nss_supp.tx_mcs_160, he_cap_ie + ofst_160m + 2, 2);
		ofst_80p80m += 4;
	} else {
		_rtw_memset(&sta_he_cap->he_mcs_nss_supp.rx_mcs_160, HE_MSC_NOT_SUPP_BYTE, 2);
		_rtw_memset(&sta_he_cap->he_mcs_nss_supp.tx_mcs_160, HE_MSC_NOT_SUPP_BYTE, 2);
	}

	/* HE supports 80M+80M BW */
	if(GET_HE_PHY_CAP_SUPPORT_CHAN_WIDTH_SET(he_cap_ie + 3 + HE_CAP_ELE_MAC_CAP_LEN) & BIT3) {
		_rtw_memcpy(&sta_he_cap->he_mcs_nss_supp.rx_mcs_80p80, he_cap_ie + ofst_80p80m, 2);
		_rtw_memcpy(&sta_he_cap->he_mcs_nss_supp.tx_mcs_80p80, he_cap_ie + ofst_80p80m + 2, 2);
	} else {
		_rtw_memset(&sta_he_cap->he_mcs_nss_supp.rx_mcs_80p80, HE_MSC_NOT_SUPP_BYTE, 2);
		_rtw_memset(&sta_he_cap->he_mcs_nss_supp.tx_mcs_80p80, HE_MSC_NOT_SUPP_BYTE, 2);
	}

	return _SUCCESS;
}


static void rtw_cfg80211_init_sband_iftype_data(_adapter *padapter,
	struct ieee80211_supported_band *band, struct phy_cap_t *phy_cap,
		struct role_link_cap_t *dft_cap,
		struct protocol_cap_t *dft_sta_proto_cap,
		struct protocol_cap_t *dft_ap_proto_cap)
{
	struct ieee80211_sband_iftype_data *he_iftype = NULL;
	void *phl = GET_PHL_INFO(adapter_to_dvobj(padapter));
	int ret = _FAIL;

	he_iftype = (struct ieee80211_sband_iftype_data *)(((u8 *)band->bitrates)
			            + sizeof(struct ieee80211_rate) * band->n_bitrates);

	ret = rtw_cfg80211_init_he_capab(padapter, he_iftype,
					NL80211_IFTYPE_AP, phy_cap, dft_ap_proto_cap, band->band);
	if (ret == _SUCCESS)
		band->n_iftype_data += 1;

	he_iftype += 1;
	ret = rtw_cfg80211_init_he_capab(padapter, he_iftype,
					NL80211_IFTYPE_STATION, phy_cap, dft_sta_proto_cap, band->band);

	if (ret == _SUCCESS)
		band->n_iftype_data += 1;
}
#endif /* defined(CONFIG_80211AX_HE) && (defined(CPTCFG_VERSION) || LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)) */

static int rtw_cfg80211_init_wiphy_band(_adapter *padapter, struct wiphy *wiphy)
{
	u8 rf_type;
	struct ieee80211_supported_band *band;
	int ret = _FAIL;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);

	struct protocol_cap_t dft_sta_proto_cap = {0};
	struct protocol_cap_t dft_ap_proto_cap = {0};
	struct role_link_cap_t dft_cap = {0};
	struct phy_cap_t *phy_cap;
	u8 hw_band = HW_BAND_0;

	/*init wiphy0 for band0*/
	rtw_phl_get_dft_cap(dvobj->phl, hw_band, &dft_cap);

	rtw_phl_get_dft_proto_cap(dvobj->phl, hw_band,
			PHL_RTYPE_STATION, &dft_sta_proto_cap);
	rtw_phl_get_dft_proto_cap(dvobj->phl, hw_band,
				PHL_RTYPE_AP, &dft_ap_proto_cap);
	phy_cap = &(dvobj->phl_com->phy_cap[hw_band]);

	/*TODO init wiphy1 for band1*/
	#if 0 /*#ifdef CONFIG_DBCC_SUPPORT*/
	/*if (dvobj->phl_com->dev_cap.hw_sup_flags & HW_SUP_DBCC)*/
	if (dvobj->phl_com->dev_cap.dbcc_sup) {
		hw_band = HW_BAND_1;
		rtw_phl_get_dft_cap(dvobj->phl, hw_band, &dft_cap);

		rtw_phl_get_dft_proto_cap(dvobj->phl, hw_band,
			PHL_RTYPE_STATION, &dft_sta_proto_cap);
		rtw_phl_get_dft_proto_cap(dvobj->phl, hw_band,
				PHL_RTYPE_AP, &dft_ap_proto_cap);
		phy_cap = &(dvobj->phl_com->phy_cap[hw_band]);
	}
	#endif

	rf_type = GET_HAL_RFPATH(dvobj);
	RTW_INFO("%s:rf_type=%d\n", __func__, rf_type);


	if (is_supported_24g(padapter->registrypriv.band_type) &&
	    (rtw_hw_is_band_support(dvobj, BAND_ON_24G))) {
		band = wiphy->bands[NL80211_BAND_2GHZ] = rtw_spt_band_alloc(BAND_ON_24G);
		if (!band)
			goto err;

		rtw_2g_channels_init(band->channels);
		rtw_2g_rates_init(band->bitrates);
		#if defined(CONFIG_80211N_HT)
		rtw_cfg80211_init_ht_capab(padapter, &band->ht_cap,
					BAND_ON_24G, rf_type, &dft_sta_proto_cap, &dft_cap);
		#endif
		#if defined(CONFIG_80211AX_HE) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
		rtw_cfg80211_init_sband_iftype_data(padapter, band, phy_cap,
			&dft_cap, &dft_sta_proto_cap, &dft_ap_proto_cap);
		#endif /* defined(CONFIG_80211AX_HE) && LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0) */
	}

	#if CONFIG_IEEE80211_BAND_5GHZ
	if (is_supported_5g(padapter->registrypriv.band_type) &&
	    (rtw_hw_is_band_support(dvobj, BAND_ON_5G))) {
		band = wiphy->bands[NL80211_BAND_5GHZ] = rtw_spt_band_alloc(BAND_ON_5G);
		if (!band)
			goto err;

		rtw_5g_channels_init(band->channels);
		rtw_5g_rates_init(band->bitrates);
		#if defined(CONFIG_80211N_HT)
		rtw_cfg80211_init_ht_capab(padapter, &band->ht_cap,
					BAND_ON_5G, rf_type, &dft_sta_proto_cap, &dft_cap);
		#endif
		#if defined(CONFIG_80211AC_VHT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
		rtw_cfg80211_init_vht_capab(padapter, &band->vht_cap,
					BAND_ON_5G, rf_type, &dft_sta_proto_cap, &dft_cap);
		#endif
		#if defined(CONFIG_80211AX_HE) && (defined(CPTCFG_VERSION) || (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)))
		rtw_cfg80211_init_sband_iftype_data(padapter, band, phy_cap,
			&dft_cap, &dft_sta_proto_cap, &dft_ap_proto_cap);
		#endif /* defined(CONFIG_80211AX_HE) && (defined(CPTCFG_VERSION) || LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)) */
	}
	#endif

	#if CONFIG_IEEE80211_BAND_6GHZ
	if (is_supported_6g(padapter->registrypriv.band_type) &&
	    (rtw_hw_is_band_support(dvobj, BAND_ON_6G))) {
		band = wiphy->bands[NL80211_BAND_6GHZ] = rtw_spt_band_alloc(BAND_ON_6G);
		if (!band)
			goto err;

		rtw_6g_channels_init(band->channels);
		rtw_6g_rates_init(band->bitrates);

		#if defined(CONFIG_80211AX_HE) && (defined(CPTCFG_VERSION) || (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)))
		rtw_cfg80211_init_sband_iftype_data(padapter, band, phy_cap,
			&dft_cap, &dft_sta_proto_cap, &dft_ap_proto_cap);
		#endif /* defined(CONFIG_80211AX_HE) && (defined(CPTCFG_VERSION) || LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)) */
	}
	#endif

	return _SUCCESS;

err:
	if (wiphy->bands[NL80211_BAND_2GHZ]) {
		rtw_spt_band_free(wiphy->bands[NL80211_BAND_2GHZ]);
		wiphy->bands[NL80211_BAND_2GHZ] = NULL;
	}
#if CONFIG_IEEE80211_BAND_5GHZ
	if (wiphy->bands[NL80211_BAND_5GHZ]) {
		rtw_spt_band_free(wiphy->bands[NL80211_BAND_5GHZ]);
		wiphy->bands[NL80211_BAND_5GHZ] = NULL;
	}
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	if (wiphy->bands[NL80211_BAND_6GHZ]) {
		rtw_spt_band_free(wiphy->bands[NL80211_BAND_6GHZ]);
		wiphy->bands[NL80211_BAND_6GHZ] = NULL;
	}
#endif

	return ret;
}

#if !defined(CONFIG_REGD_SRC_FROM_OS) || (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0))
void rtw_cfg80211_update_wiphy_max_txpower(_adapter *adapter, struct wiphy *wiphy)
{
	struct ieee80211_supported_band *band;
	struct ieee80211_channel *channel;
	s16 max_txpwr;
	int i;

	if (is_supported_24g(adapter->registrypriv.band_type)) {
		band = wiphy->bands[NL80211_BAND_2GHZ];
		if (band) {
			max_txpwr = 30 * MBM_PDBM; //phy_get_txpwr_by_rate_total_max_mbm(adapter, BAND_ON_24G, 1, 1);
			if (max_txpwr != UNSPECIFIED_MBM) {
				for (i = 0; i < band->n_channels; i++) {
					channel = &band->channels[i];
					channel->max_power = max_txpwr / MBM_PDBM;
				}
			}
		}
	}
#if CONFIG_IEEE80211_BAND_5GHZ
	if (is_supported_5g(adapter->registrypriv.band_type)) {
		band = wiphy->bands[NL80211_BAND_5GHZ];
		if (band) {
			max_txpwr = 30 * MBM_PDBM; //phy_get_txpwr_by_rate_total_max_mbm(adapter, BAND_ON_5G, 1, 1);
			if (max_txpwr != UNSPECIFIED_MBM) {
				for (i = 0; i < band->n_channels; i++) {
					channel = &band->channels[i];
					channel->max_power = max_txpwr / MBM_PDBM;
				}
			}
		}
	}
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	if (is_supported_6g(adapter->registrypriv.band_type)) {
		band = wiphy->bands[NL80211_BAND_6GHZ];
		if (band) {
			max_txpwr = 30 * MBM_PDBM; //phy_get_txpwr_by_rate_total_max_mbm(adapter, BAND_ON_6G, 1, 1);
			if (max_txpwr != UNSPECIFIED_MBM) {
				for (i = 0; i < band->n_channels; i++) {
					channel = &band->channels[i];
					channel->max_power = max_txpwr / MBM_PDBM;
				}
			}
		}
	}
#endif
}
#endif /* defined(CONFIG_REGD_SRC_FROM_OS) || (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)) */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)) && (CONFIG_IFACE_NUMBER >= 2)
struct ieee80211_iface_limit rtw_limits[] = {
	{
		.max = CONFIG_IFACE_NUMBER,
		.types = BIT(NL80211_IFTYPE_STATION)
			#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
			| BIT(NL80211_IFTYPE_P2P_CLIENT)
			#endif
	},
	#ifdef CONFIG_AP_MODE
	{
		.max = rtw_min(CONFIG_IFACE_NUMBER, CONFIG_LIMITED_AP_NUM),
		.types = BIT(NL80211_IFTYPE_AP)
			#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
			| BIT(NL80211_IFTYPE_P2P_GO)
			#endif
	},
	#endif
	#if defined(RTW_DEDICATED_P2P_DEVICE)
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_DEVICE)
	},
	#endif
	#if defined(CONFIG_RTW_MESH)
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_MESH_POINT)
	},
	#endif
};

struct ieee80211_iface_combination rtw_combinations[] = {
	{
		.limits = rtw_limits,
		.n_limits = ARRAY_SIZE(rtw_limits),
		#if defined(RTW_DEDICATED_P2P_DEVICE)
		.max_interfaces = CONFIG_IFACE_NUMBER + 1,
		#else
		.max_interfaces = CONFIG_IFACE_NUMBER,
		#endif
		#if defined(CONFIG_MCC_MODE) || defined(CONFIG_DBCC_SUPPORT)
		.num_different_channels = 2,
		#else
		.num_different_channels = 1,
		#endif

	},
};
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)) */

static int rtw_cfg80211_init_wiphy(_adapter *adapter, struct wiphy *wiphy)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	struct registry_priv *regsty = dvobj_to_regsty(dvobj);
	int ret = _FAIL;

	/* copy mac_addr to wiphy */
	_rtw_memcpy(wiphy->perm_addr, adapter_mac_addr(adapter), ETH_ALEN);

	wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

	wiphy->max_scan_ssids = RTW_SSID_SCAN_AMOUNT;
	wiphy->max_scan_ie_len = RTW_SCAN_IE_LEN_MAX;
	wiphy->max_num_pmkids = RTW_MAX_NUM_PMKIDS;

#if CONFIG_RTW_MACADDR_ACL && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	wiphy->max_acl_mac_addrs = NUM_ACL;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)) || defined(COMPAT_KERNEL_RELEASE)
	wiphy->max_remain_on_channel_duration = RTW_MAX_REMAIN_ON_CHANNEL_DURATION;
#endif

	wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION)
		| BIT(NL80211_IFTYPE_ADHOC)
		#ifdef CONFIG_AP_MODE
		| BIT(NL80211_IFTYPE_AP)
		#ifdef CONFIG_WIFI_MONITOR
		| BIT(NL80211_IFTYPE_MONITOR)
		#endif
		#endif
		#if defined(CONFIG_P2P) && ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE))
		| BIT(NL80211_IFTYPE_P2P_CLIENT)
		| BIT(NL80211_IFTYPE_P2P_GO)
		#if defined(RTW_DEDICATED_P2P_DEVICE)
		| BIT(NL80211_IFTYPE_P2P_DEVICE)
		#endif
		#endif

		#ifdef CONFIG_RTW_MESH
		| BIT(NL80211_IFTYPE_MESH_POINT)	/* 2.6.26 */
		#endif
		;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
#ifdef CONFIG_AP_MODE
	wiphy->mgmt_stypes = rtw_cfg80211_default_mgmt_stypes;
#endif /* CONFIG_AP_MODE */
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
	#ifdef CONFIG_WIFI_MONITOR
	wiphy->software_iftypes |= BIT(NL80211_IFTYPE_MONITOR);
	#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)) && (CONFIG_IFACE_NUMBER >= 2)
	wiphy->iface_combinations = rtw_combinations;
	wiphy->n_iface_combinations = ARRAY_SIZE(rtw_combinations);
#endif

	wiphy->cipher_suites = rtw_cipher_suites;
	wiphy->n_cipher_suites = ARRAY_SIZE(rtw_cipher_suites);

	if (rtw_cfg80211_init_wiphy_band(adapter, wiphy) != _SUCCESS) {
		RTW_ERR("rtw_cfg80211_init_wiphy_band fail\n");
		goto exit;
	}
	#if !defined(CONFIG_REGD_SRC_FROM_OS) || (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0))
	rtw_cfg80211_update_wiphy_max_txpower(adapter, wiphy);
	#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38) && LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
	wiphy->flags |= WIPHY_FLAG_SUPPORTS_SEPARATE_DEFAULT_KEYS;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0))
	wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	wiphy->flags |= WIPHY_FLAG_HAVE_AP_SME;
	/* OFFCHAN_TX not ready, Mgmt tx depend on REMAIN_ON_CHANNEL */
	/* wiphy->flags |= WIPHY_FLAG_OFFCHAN_TX; */
#endif

#if (KERNEL_VERSION(3, 2, 0) <= LINUX_VERSION_CODE)
	wiphy->flags |= WIPHY_FLAG_AP_UAPSD;
#endif

#if defined(CONFIG_PM) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0) || defined (COMPAT_KERNEL_RELEASE_4_19)
	wiphy->max_sched_scan_reqs = 1;
#else
	wiphy->flags |= WIPHY_FLAG_SUPPORTS_SCHED_SCAN;
#endif
#ifdef CONFIG_PNO_SUPPORT
	wiphy->max_sched_scan_ssids = MAX_NLO_NUM;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
	wiphy->max_match_sets = MAX_NLO_NUM;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
	wiphy->max_sched_scan_plans = MAX_NLO_SCAN_PLANS;
	wiphy->max_sched_scan_plan_interval = MAX_NLO_SCAN_PERIOD;
	wiphy->max_sched_scan_plan_iterations = MAX_NLO_NORMAL_SCAN_CYCLE;
#endif
#endif
#endif

#if defined(CONFIG_PM) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0))
	wiphy->wowlan = wowlan_stub;
#else
	wiphy->wowlan = &wowlan_stub;
#endif
#endif

#if defined(CONFIG_TDLS) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	wiphy->flags |= WIPHY_FLAG_SUPPORTS_TDLS;
#ifndef CONFIG_TDLS_DRIVER_SETUP
	wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;	/* Driver handles key exchange */
	wiphy->flags |= NL80211_ATTR_HT_CAPABILITY;
#endif /* CONFIG_TDLS_DRIVER_SETUP */
#endif /* CONFIG_TDLS */

#if 0 && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0))
	wiphy->flags |= WIPHY_FLAG_SUPPORTS_FW_ROAM;
#endif

#ifdef CONFIG_RTW_WDS
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))
	wiphy->flags |= WIPHY_FLAG_4ADDR_AP;
	wiphy->flags |= WIPHY_FLAG_4ADDR_STATION;
	#endif
#endif

#ifdef CONFIG_RTW_MESH
	wiphy->flags |= 0
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
		| WIPHY_FLAG_IBSS_RSN
		#endif
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
		| WIPHY_FLAG_MESH_AUTH
		#endif
		;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0))
	wiphy->features |= 0
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		| NL80211_FEATURE_USERSPACE_MPM
		#endif
		;
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)) */
#endif /* CONFIG_RTW_MESH */

#if defined(CONFIG_RTW_80211K) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0))
	wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_RRM);
#endif

#if (KERNEL_VERSION(3, 8, 0) <= LINUX_VERSION_CODE)
	wiphy->features |= NL80211_FEATURE_SAE;
#endif

#ifdef CONFIG_RTW_SCAN_RAND
	#if (KERNEL_VERSION(3, 19, 0) <= LINUX_VERSION_CODE)
	wiphy->features |= NL80211_FEATURE_SCAN_RANDOM_MAC_ADDR;
	#endif /* KERNEL_VERSION 3.19 */
#endif /* CONFIG_RTW_SCAN_RAND */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
#ifdef CONFIG_WIFI_MONITOR
	/* Currently only for Monitor debugging */
	wiphy->flags |= WIPHY_FLAG_SUPPORTS_5_10_MHZ;
#endif
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0)) */

#ifdef CONFIG_ECSA_PHL
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0))
	wiphy->flags |= WIPHY_FLAG_HAS_CHANNEL_SWITCH;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0))
	wiphy->max_num_csa_counters = MAX_CSA_CNT;
#endif
#endif /* CONFIG_ECSA_PHL */

#if CONFIG_IEEE80211_BAND_6GHZ
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	wiphy->flags |= WIPHY_FLAG_SPLIT_SCAN_6GHZ;
#endif
#endif

	ret = _SUCCESS;

exit:
	return ret;
}

#ifdef CONFIG_RFKILL_POLL
void rtw_cfg80211_init_rfkill(struct wiphy *wiphy)
{
	wiphy_rfkill_set_hw_state(wiphy, 0);
	wiphy_rfkill_start_polling(wiphy);
}

void rtw_cfg80211_deinit_rfkill(struct wiphy *wiphy)
{
	wiphy_rfkill_stop_polling(wiphy);
}

static void cfg80211_rtw_rfkill_poll(struct wiphy *wiphy)
{
	_adapter *padapter = NULL;
	bool blocked = _FALSE;
	u8 valid = 0;

	padapter = wiphy_to_adapter(wiphy);

	if (adapter_to_dvobj(padapter)->processing_dev_remove == _TRUE) {
		/*RTW_INFO("cfg80211_rtw_rfkill_poll: device is removed!\n");*/
		return;
	}

	blocked = rtw_hal_rfkill_poll(padapter, &valid);
	/*RTW_INFO("cfg80211_rtw_rfkill_poll: valid=%d, blocked=%d\n",
			valid, blocked);*/

	if (valid)
		wiphy_rfkill_set_hw_state(wiphy, blocked);
}
#endif

#if defined(CONFIG_RTW_HOSTAPD_ACS) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
#define SURVEY_INFO_TIME			SURVEY_INFO_CHANNEL_TIME
#define SURVEY_INFO_TIME_BUSY		SURVEY_INFO_CHANNEL_TIME_BUSY
#define SURVEY_INFO_TIME_EXT_BUSY	SURVEY_INFO_CHANNEL_TIME_EXT_BUSY
#define SURVEY_INFO_TIME_RX			SURVEY_INFO_CHANNEL_TIME_RX
#define SURVEY_INFO_TIME_TX			SURVEY_INFO_CHANNEL_TIME_TX
#endif

#ifdef CONFIG_FIND_BEST_CHANNEL
static void rtw_cfg80211_set_survey_info_with_find_best_channel(struct wiphy *wiphy
	, struct net_device *netdev, int idx, struct survey_info *info)
{
	_adapter *adapter = (_adapter *)rtw_netdev_priv(netdev);
	struct rtw_chset *chset = adapter_to_chset(adapter);
	u32 total_rx_cnt = 0;
	int i;

	s8 noise = -50;		/*channel noise in dBm. This and all following fields are optional */
	u64 time = 100;		/*amount of time in ms the radio was turn on (on the channel)*/
	u64 time_busy = 0;	/*amount of time the primary channel was sensed busy*/

	info->filled  = SURVEY_INFO_NOISE_DBM
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
		| SURVEY_INFO_TIME | SURVEY_INFO_TIME_BUSY
		#endif
		;

	for (i = 0; i < chset->chs_len; i++)
		total_rx_cnt += chset->chs[i].rx_count;

	time_busy = chset->chs[idx].rx_count * time / total_rx_cnt;
	noise += chset->chs[idx].rx_count * 50 / total_rx_cnt;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	info->channel_time = time;
	info->channel_time_busy = time_busy;
	#else
	info->time = time;
	info->time_busy = time_busy;
	#endif
#endif
	info->noise = noise;

	/* reset if final channel is got */
	if (idx == chset->chs_len - 1) {
		for (i = 0; i < chset->chs_len; i++)
			chset->chs[i].rx_count = 0;
	}
}
#endif /* CONFIG_FIND_BEST_CHANNEL */

#ifdef CONFIG_RTW_ACS
static void rtw_cfg80211_set_survey_info_with_clm(_adapter *padapter, int idx, struct survey_info *pinfo)
{
	s8 noise = -50;			/*channel noise in dBm. This and all following fields are optional */
	u8 time = SURVEY_TO;	/*amount of time in ms the radio was turn on (on the channel)*/
	u8 time_busy = 0;		/*amount of time the primary channel was sensed busy*/

	if ((idx < 0) || (pinfo == NULL))
		return;

	pinfo->filled  = SURVEY_INFO_NOISE_DBM
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
		| SURVEY_INFO_TIME | SURVEY_INFO_TIME_BUSY
		#endif
		;

	time_busy = rtw_acs_get_clm_ratio(padapter, pinfo->channel->band, pinfo->channel->hw_value);
	noise = rtw_acs_get_noise_dbm(padapter, pinfo->channel->band, pinfo->channel->hw_value);
	RTW_INFO("[%d] ch=%d, band=%d, time=%d(ms), time_busy=%d(ms), noise=%d(dbm)\n",
		idx, pinfo->channel->hw_value, pinfo->channel->band ,time, time_busy, noise);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0))
	pinfo->channel_time = time;
	pinfo->channel_time_busy = time_busy;
	#else
	pinfo->time = time;
	pinfo->time_busy = time_busy;
	#endif
#endif
	pinfo->noise = noise;
}
#endif

int rtw_hostapd_acs_dump_survey(struct wiphy *wiphy, struct net_device *netdev, int idx, struct survey_info *info)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(netdev);
	struct rtw_chset *chset = adapter_to_chset(padapter);
	RT_CHANNEL_INFO *chinfo;
	u32 freq = 0;
	u8 ret = 0;
	u16 channel = 0;
	u8 band = 0;

	if (!netdev || !info) {
		RTW_INFO("%s: invial parameters.\n", __func__);
		return -EINVAL;
	}

	_rtw_memset(info, 0, sizeof(struct survey_info));
	if (padapter->netif_up == _FALSE) {
		RTW_INFO("%s: net device is down.\n", __func__);
		return -EIO;
	}

	if (idx >= MAX_CHANNEL_NUM)
		return -ENOENT;

	chinfo = &chset->chs[idx];
	channel = chinfo->ChannelNum;
	band = chinfo->band;
	freq = rtw_bch2freq(band, channel);

	info->channel = ieee80211_get_channel(wiphy, freq);
	/* RTW_INFO("%s: channel %d, freq %d\n", __func__, channel, freq); */

	if (!info->channel)
		return -EINVAL;

	if (info->channel->flags == IEEE80211_CHAN_DISABLED)
		return ret;

	rtw_cfg80211_set_survey_info_with_clm(padapter, idx, info);

	return ret;
}
#endif /* defined(CONFIG_RTW_HOSTAPD_ACS) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)) */

#if defined(CPTCFG_VERSION) || (KERNEL_VERSION(4, 17, 0) <= LINUX_VERSION_CODE) \
    || defined(CONFIG_KERNEL_PATCH_EXTERNAL_AUTH)
int cfg80211_rtw_external_auth(struct wiphy *wiphy, struct net_device *dev,
	struct cfg80211_external_auth_params *params)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(dev));

	rtw_cfg80211_external_auth_status(wiphy, dev,
		(struct rtw_external_auth_params *)params);

	return 0;
}
#endif

void rtw_cfg80211_external_auth_status(struct wiphy *wiphy, struct net_device *dev,
	struct rtw_external_auth_params *params)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	struct sta_priv *pstapriv = &padapter->stapriv;
	struct sta_info	*psta = NULL;
	u8 *buf = NULL;
	u32 len = 0;

	RTW_INFO(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(dev));

	RTW_INFO("SAE: action: %u, status: %u\n", params->action, params->status);
	if (params->status == WLAN_STATUS_SUCCESS) {
		RTW_INFO("bssid: "MAC_FMT"\n", MAC_ARG(params->bssid));
		RTW_INFO("SSID: [%s]\n",
			((params->ssid.ssid_len == 0) ? "" : (char *)params->ssid.ssid));
		RTW_INFO("suite: 0x%08x\n", params->key_mgmt_suite);
	}

	psta = rtw_get_stainfo(pstapriv, params->bssid);
	if (psta && (params->status == WLAN_STATUS_SUCCESS)\
		&& MLME_IS_AP(padapter)) {
		/* AP mode */
		RTW_INFO("station match\n");

		psta->state &= ~WIFI_FW_AUTH_NULL;
		psta->state |= WIFI_FW_AUTH_SUCCESS;
		psta->expire_to = padapter->stapriv.assoc_to;

		/* ToDo: Kernel v5.1 pmkid is pointer */
		/* RTW_INFO_DUMP("PMKID:", params->pmkid, PMKID_LEN); */
		_rtw_set_pmksa(dev, params->bssid, params->pmkid);

		_rtw_spinlock_bh(&psta->lock);
		if ((psta->auth_len != 0) && (psta->pauth_frame != NULL)) {
			buf =  rtw_zmalloc(psta->auth_len);
			if (buf) {
				_rtw_memcpy(buf, psta->pauth_frame, psta->auth_len);
				len = psta->auth_len;
			}

			rtw_mfree(psta->pauth_frame, psta->auth_len);
			psta->pauth_frame = NULL;
			psta->auth_len = 0;
		}
		_rtw_spinunlock_bh(&psta->lock);

		if (buf) {
			struct _ADAPTER_LINK *padapter_link = psta->padapter_link;
			struct link_mlme_ext_priv *pmlmeext = &(padapter_link->mlmeextpriv);
			/* send the SAE auth Confirm */

			{
				rtw_mi_set_scan_deny(padapter, 1000);
				rtw_mi_scan_abort(padapter, _TRUE);

				RTW_INFO("SAE: Tx auth Confirm\n");
				rtw_mgnt_tx_cmd(padapter, pmlmeext->chandef.band, pmlmeext->chandef.chan, 1, buf, len, 0, RTW_CMDF_DIRECTLY);

			}

			rtw_mfree(buf, len);
			buf = NULL;
			len = 0;
		}
	} else {
		/* STA mode */
		psecuritypriv->extauth_status = params->status;
	}
}

#ifdef CONFIG_AP_MODE
#ifdef CONFIG_ECSA_PHL
static bool rtw_ap_check_csa_setting(_adapter* a, u8 new_ch, u8 new_bw, u8 new_offset)
{
	struct dvobj_priv *d = adapter_to_dvobj(a);
	struct rtw_chset *chset = adapter_to_chset(a);
	struct rtw_chan_def u_chdef = {0};
	u8 c_ch, c_bw, c_offset, u_ch, u_bw, u_offset;
#ifdef CONFIG_MCC_MODE
	struct rtw_phl_com_t *phl_com = GET_PHL_COM(d);
	u8 mcc_sup = phl_com->dev_cap.mcc_sup;
#else
	u8 mcc_sup = _FALSE;
#endif
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	struct link_mlme_ext_priv *pmlmeext = &(alink->mlmeextpriv);

	if (rtw_phl_mr_get_chandef(d->phl, a->phl_role, alink->wrlink, &u_chdef)
							!= RTW_PHL_STATUS_SUCCESS) {
		RTW_ERR("CSA : "FUNC_ADPT_FMT" get union chandef failed\n", FUNC_ADPT_ARG(a));
		rtw_warn_on(1);
		return _FALSE;
	}

	u_ch = u_chdef.chan;
	u_bw = u_chdef.bw;
	u_offset = u_chdef.offset;
	c_ch = pmlmeext->chandef.chan;
	c_bw = pmlmeext->chandef.bw;
	c_offset = pmlmeext->chandef.offset;

	if (rtw_chset_search_ch(chset, new_ch) < 0
			|| rtw_chset_is_ch_non_ocp(chset, new_ch)) {
		RTW_INFO("CSA : reject, channel not legal csa_setting:%u,%u,%u\n", new_ch, new_bw, new_offset);
		return _FALSE;
	}

	/* Need to group with chanctx if not support MCC */
	if (mcc_sup == _FALSE &&
			rtw_mi_get_ld_sta_ifbmp(a) &&
			rtw_is_chbw_grouped(new_ch, new_bw, new_offset, u_ch, u_bw, u_offset) == _FALSE) {
		RTW_INFO("CSA : reject, can't group with STA mode, csa_setting:%u,%u,%u, union:%u,%u,%u\n",
			new_ch, new_bw, new_offset, u_ch, u_bw, u_offset);
		return _FALSE;
	}

	return _TRUE;
}
#endif /* CONFIG_ECSA_PHL */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0))
static int cfg80211_rtw_channel_switch(struct wiphy *wiphy,
				struct net_device *dev,
				struct cfg80211_csa_settings *params)
{
#ifdef CONFIG_ECSA_PHL
	_adapter *a = (_adapter *)rtw_netdev_priv(dev);
	struct rf_ctl_t *rfctl = adapter_to_rfctl(a);
	struct rtw_chan_def csa_chdef = {0};
	u8 mode = 0, count = 0, ht = 0;
	u8 ecsa_op_class;
	u8 csa_ch_width = 0, csa_ch_freq_seg0 = 0, csa_ch_freq_seg1 = 0;

	if (!(CHK_MLME_STATE(a, WIFI_AP_STATE | WIFI_MESH_STATE)
			&& MLME_IS_ASOC(a))) {
		RTW_ERR("CSA : "FUNC_ADPT_FMT" not AP/Mesh, so return -ENOTCONN\n", FUNC_ADPT_ARG(a));
		return -ENOTCONN;
	}

	if (rtw_mr_is_ecsa_running(a)) {
		RTW_INFO("CSA : "FUNC_ADPT_FMT" someone is switching channel, so return -EBUSY\n", FUNC_ADPT_ARG(a));
		return -EBUSY;
	}

	rtw_get_chdef_from_cfg80211_chan_def(&params->chandef, &ht, &csa_chdef);

	if (rtw_ap_check_csa_setting(a, csa_chdef.chan, csa_chdef.bw, csa_chdef.offset) == _FALSE)
		return -EINVAL;

	mode = params->block_tx;
	count = params->count;
	RTW_INFO("CSA : Get from cfg80211_csa_settings, block_tx = %s, switch count = %u\n",
			mode ? "True" : "Flase", count);

	/* Parsing channel switch related IEs from cfg80211_csa_settings */
	rtw_cfg80211_build_csa_beacon(a, params, csa_chdef);

	ecsa_op_class = rtw_phl_get_operating_class(csa_chdef);
	rtw_hal_trigger_csa_start(a, CSA_AP_CFG80211_SWITCH_CH,
		mode, ecsa_op_class, count,
		csa_chdef.band, csa_chdef.chan, csa_chdef.bw, csa_chdef.offset,
		csa_ch_width, csa_ch_freq_seg0, csa_ch_freq_seg1);

#endif /* CONFIG_ECSA_PHL */
	return 0;
}
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)) */
#endif /* CONFIG_AP_MODE */

struct cfg80211_ops rtw_cfg80211_ops = {
	.change_virtual_intf = cfg80211_rtw_change_iface,
	.add_key = cfg80211_rtw_add_key,
	.get_key = cfg80211_rtw_get_key,
	.del_key = cfg80211_rtw_del_key,
	.set_default_key = cfg80211_rtw_set_default_key,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30))
	.set_default_mgmt_key = cfg80211_rtw_set_default_mgmt_key,
#endif
#if defined(CONFIG_GTK_OL) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0))
	.set_rekey_data = cfg80211_rtw_set_rekey_data,
#endif /*CONFIG_GTK_OL*/
	.get_station = cfg80211_rtw_get_station,
	.scan = cfg80211_rtw_scan,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0))
	.abort_scan = cfg80211_rtw_abort_scan,
#endif /* LINUX_VERSION_CODE 4.5.0 */
	.set_wiphy_params = cfg80211_rtw_set_wiphy_params,
	.connect = cfg80211_rtw_connect,
	.disconnect = cfg80211_rtw_disconnect,
	.join_ibss = cfg80211_rtw_join_ibss,
	.leave_ibss = cfg80211_rtw_leave_ibss,
	.set_tx_power = cfg80211_rtw_set_txpower,
	.get_tx_power = cfg80211_rtw_get_txpower,
	.set_power_mgmt = cfg80211_rtw_set_power_mgmt,
	.set_pmksa = cfg80211_rtw_set_pmksa,
	.del_pmksa = cfg80211_rtw_del_pmksa,
	.flush_pmksa = cfg80211_rtw_flush_pmksa,

#ifdef CONFIG_AP_MODE
	.add_virtual_intf = cfg80211_rtw_add_virtual_intf,
	.del_virtual_intf = cfg80211_rtw_del_virtual_intf,

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)) && !defined(COMPAT_KERNEL_RELEASE)
	.add_beacon = cfg80211_rtw_add_beacon,
	.set_beacon = cfg80211_rtw_set_beacon,
	.del_beacon = cfg80211_rtw_del_beacon,
#else
	.start_ap = cfg80211_rtw_start_ap,
	.change_beacon = cfg80211_rtw_change_beacon,
	.stop_ap = cfg80211_rtw_stop_ap,
#endif

#if CONFIG_RTW_MACADDR_ACL && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	.set_mac_acl = cfg80211_rtw_set_mac_acl,
#endif

	.add_station = cfg80211_rtw_add_station,
	.del_station = cfg80211_rtw_del_station,
	.change_station = cfg80211_rtw_change_station,
	.dump_station = cfg80211_rtw_dump_station,
	.change_bss = cfg80211_rtw_change_bss,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29))
	.set_txq_params = cfg80211_rtw_set_txq_params,
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))
	.set_channel = cfg80211_rtw_set_channel,
#endif
	/* .auth = cfg80211_rtw_auth, */
	/* .assoc = cfg80211_rtw_assoc,	 */
#endif /* CONFIG_AP_MODE */

#if defined(CONFIG_RTW_MESH) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38))
	.get_mesh_config = cfg80211_rtw_get_mesh_config,
	.update_mesh_config = cfg80211_rtw_update_mesh_config,
	.join_mesh = cfg80211_rtw_join_mesh,
	.leave_mesh = cfg80211_rtw_leave_mesh,
	.add_mpath = cfg80211_rtw_add_mpath,
	.del_mpath = cfg80211_rtw_del_mpath,
	.change_mpath = cfg80211_rtw_change_mpath,
	.get_mpath = cfg80211_rtw_get_mpath,
	.dump_mpath = cfg80211_rtw_dump_mpath,
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0))
	.get_mpp = cfg80211_rtw_get_mpp,
	.dump_mpp = cfg80211_rtw_dump_mpp,
	#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	.set_monitor_channel = cfg80211_rtw_set_monitor_channel,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0))
	.get_channel = cfg80211_rtw_get_channel,
#endif

	.remain_on_channel = cfg80211_rtw_remain_on_channel,
	.cancel_remain_on_channel = cfg80211_rtw_cancel_remain_on_channel,
#ifdef CONFIG_P2P
	#if defined(RTW_DEDICATED_P2P_DEVICE)
	.start_p2p_device = cfg80211_rtw_start_p2p_device,
	.stop_p2p_device = cfg80211_rtw_stop_p2p_device,
	#endif
#endif /* CONFIG_P2P */

#ifdef CONFIG_RTW_80211R
	.update_ft_ies = cfg80211_rtw_update_ft_ies,
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || defined(COMPAT_KERNEL_RELEASE)
	.mgmt_tx = cfg80211_rtw_mgmt_tx,

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0))
	.mgmt_frame_register = cfg80211_rtw_mgmt_frame_register,
#else
	.update_mgmt_frame_registrations = cfg80211_rtw_update_mgmt_frame_register,
#endif

#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34) && LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))
	.action = cfg80211_rtw_mgmt_tx,
#endif

#if defined(CONFIG_TDLS) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	.tdls_mgmt = cfg80211_rtw_tdls_mgmt,
	.tdls_oper = cfg80211_rtw_tdls_oper,
#endif /* CONFIG_TDLS */

#if defined(CONFIG_PNO_SUPPORT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
	.sched_scan_start = cfg80211_rtw_sched_scan_start,
	.sched_scan_stop = cfg80211_rtw_sched_scan_stop,
	.suspend = cfg80211_rtw_suspend,
	.resume = cfg80211_rtw_resume,
#endif /* CONFIG_PNO_SUPPORT */
#ifdef CONFIG_RFKILL_POLL
	.rfkill_poll = cfg80211_rtw_rfkill_poll,
#endif
#if defined(CONFIG_RTW_HOSTAPD_ACS) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))
	.dump_survey = rtw_hostapd_acs_dump_survey,
#endif
#if defined(CPTCFG_VERSION) || (KERNEL_VERSION(4, 17, 0) <= LINUX_VERSION_CODE) \
    || defined(CONFIG_KERNEL_PATCH_EXTERNAL_AUTH)
	.external_auth = cfg80211_rtw_external_auth,
#endif
#ifdef CONFIG_AP_MODE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0))
	.channel_switch = cfg80211_rtw_channel_switch,
#endif
#endif /* #ifdef CONFIG_AP_MODE */
};

struct wiphy *rtw_wiphy_alloc(_adapter *padapter, struct device *dev)
{
	struct wiphy *wiphy;
	struct rtw_wiphy_data *wiphy_data;

	/* wiphy */
	wiphy = wiphy_new(&rtw_cfg80211_ops, sizeof(struct rtw_wiphy_data));
	if (!wiphy) {
		RTW_ERR("Couldn't allocate wiphy device\n");
		goto exit;
	}
	set_wiphy_dev(wiphy, dev);

	/* wiphy_data */
	wiphy_data = rtw_wiphy_priv(wiphy);
	wiphy_data->dvobj = adapter_to_dvobj(padapter);

	/*wiphy_data->txpwr_total_lmt_mbm = UNSPECIFIED_MBM;*/
	/*wiphy_data->txpwr_total_target_mbm = UNSPECIFIED_MBM;*/

	if (rtw_cfg80211_init_wiphy(padapter, wiphy) != _SUCCESS) {
		rtw_wiphy_free(wiphy);
		wiphy = NULL;
		goto exit;
	}

	rtw_regd_init(wiphy);

	RTW_INFO(FUNC_WIPHY_FMT"\n", FUNC_WIPHY_ARG(wiphy));

exit:
	return wiphy;
}

void rtw_wiphy_free(struct wiphy *wiphy)
{
	if (!wiphy)
		return;

	RTW_INFO(FUNC_WIPHY_FMT"\n", FUNC_WIPHY_ARG(wiphy));

	rtw_regd_deinit(wiphy);

	if (wiphy->bands[NL80211_BAND_2GHZ]) {
		rtw_spt_band_free(wiphy->bands[NL80211_BAND_2GHZ]);
		wiphy->bands[NL80211_BAND_2GHZ] = NULL;
	}
#if CONFIG_IEEE80211_BAND_5GHZ
	if (wiphy->bands[NL80211_BAND_5GHZ]) {
		rtw_spt_band_free(wiphy->bands[NL80211_BAND_5GHZ]);
		wiphy->bands[NL80211_BAND_5GHZ] = NULL;
	}
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	if (wiphy->bands[NL80211_BAND_6GHZ]) {
		rtw_spt_band_free(wiphy->bands[NL80211_BAND_6GHZ]);
		wiphy->bands[NL80211_BAND_6GHZ] = NULL;
	}
#endif

	wiphy_free(wiphy);
}

int rtw_wiphy_register(struct wiphy *wiphy)
{
	struct rtw_chset *chset = dvobj_to_chset(wiphy_to_dvobj(wiphy));
	struct get_chplan_resp *chplan;
	int ret;

	RTW_INFO(FUNC_WIPHY_FMT"\n", FUNC_WIPHY_ARG(wiphy));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)) || defined(RTW_VENDOR_EXT_SUPPORT)
	rtw_cfgvendor_attach(wiphy);
#endif

	#if !RTW_PER_ADAPTER_WIPHY
	rtw_chset_hook_os_channels(chset, wiphy);
	#endif

	ret = wiphy_register(wiphy);
	if (ret != 0) {
		RTW_INFO(FUNC_WIPHY_FMT" wiphy_register() return %d\n", FUNC_WIPHY_ARG(wiphy), ret);
		goto exit;
	}

	if (rtw_get_chplan_cmd(wiphy_to_adapter(wiphy), RTW_CMDF_DIRECTLY, &chplan) == _SUCCESS)
		rtw_regd_change_complete_sync(wiphy, chplan, 1);
	else
		rtw_warn_on(1);

exit:
	return ret;
}

void rtw_wiphy_unregister(struct wiphy *wiphy)
{
	RTW_INFO(FUNC_WIPHY_FMT"\n", FUNC_WIPHY_ARG(wiphy));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)) || defined(RTW_VENDOR_EXT_SUPPORT)
	rtw_cfgvendor_detach(wiphy);
#endif

	#if defined(RTW_DEDICATED_P2P_DEVICE)
	rtw_pd_iface_free(wiphy);
	#endif

	#if CONFIG_RTW_CFG80211_CAC_EVENT
	rtw_regd_free_du_wdev(wiphy);
	#endif

	return wiphy_unregister(wiphy);
}

int rtw_wdev_alloc(_adapter *padapter, struct wiphy *wiphy)
{
	int ret = 0;
	struct net_device *pnetdev = padapter->pnetdev;
	struct wireless_dev *wdev;
	struct rtw_wdev_priv *pwdev_priv;

	RTW_INFO("%s(padapter=%p)\n", __func__, padapter);

	/*  wdev */
	wdev = (struct wireless_dev *)rtw_zmalloc(sizeof(struct wireless_dev));
	if (!wdev) {
		RTW_INFO("Couldn't allocate wireless device\n");
		ret = -ENOMEM;
		goto exit;
	}
	wdev->wiphy = wiphy;
	wdev->netdev = pnetdev;
	wdev->iftype = NL80211_IFTYPE_STATION;
	padapter->rtw_wdev = wdev;
	pnetdev->ieee80211_ptr = wdev;

	/* init pwdev_priv */
	pwdev_priv = adapter_wdev_data(padapter);
	pwdev_priv->rtw_wdev = wdev;
	pwdev_priv->pmon_ndev = NULL;
	pwdev_priv->ifname_mon[0] = '\0';
	pwdev_priv->padapter = padapter;
	pwdev_priv->scan_request = NULL;
	_rtw_spinlock_init(&pwdev_priv->scan_req_lock);
	pwdev_priv->connect_req = NULL;
	_rtw_spinlock_init(&pwdev_priv->connect_req_lock);

	pwdev_priv->p2p_enabled = _FALSE;
	pwdev_priv->probe_resp_ie_update_time = rtw_get_current_time();
	rtw_wdev_invit_info_init(&pwdev_priv->invit_info);
	rtw_wdev_nego_info_init(&pwdev_priv->nego_info);

	_rtw_mutex_init(&pwdev_priv->roch_mutex);

#ifdef CONFIG_RTW_CFGVENDOR_RSSIMONITOR
        pwdev_priv->rssi_monitor_enable = 0;
        pwdev_priv->rssi_monitor_max = 0;
        pwdev_priv->rssi_monitor_min = 0;
#endif


exit:
	return ret;
}

void rtw_wdev_free(struct wireless_dev *wdev)
{
	if (!wdev)
		return;

	RTW_INFO("%s(wdev=%p)\n", __func__, wdev);

	if (wdev_to_ndev(wdev)) {
		_adapter *adapter = (_adapter *)rtw_netdev_priv(wdev_to_ndev(wdev));
		struct rtw_wdev_priv *wdev_priv = adapter_wdev_data(adapter);

		_rtw_spinlock_free(&wdev_priv->scan_req_lock);

		_rtw_spinlock_bh(&wdev_priv->connect_req_lock);
		rtw_wdev_free_connect_req(wdev_priv);
		_rtw_spinunlock_bh(&wdev_priv->connect_req_lock);
		_rtw_spinlock_free(&wdev_priv->connect_req_lock);

		_rtw_mutex_free(&wdev_priv->roch_mutex);
	}

	rtw_mfree((u8 *)wdev, sizeof(struct wireless_dev));
}

void rtw_wdev_unregister(struct wireless_dev *wdev)
{
	struct net_device *ndev;
	_adapter *adapter;
	struct rtw_wdev_priv *pwdev_priv;

	if (!wdev)
		return;

	RTW_INFO("%s(wdev=%p)\n", __func__, wdev);

	ndev = wdev_to_ndev(wdev);
	if (!ndev)
		return;

	adapter = (_adapter *)rtw_netdev_priv(ndev);
	pwdev_priv = adapter_wdev_data(adapter);

	rtw_cfg80211_indicate_scan_done(adapter, _TRUE);

	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0)) || defined(COMPAT_KERNEL_RELEASE)
	#if (defined(CONFIG_MLD_KERNEL_PATCH) || (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 2)))
	/* ToDo CONFIG_RTW_MLD */
	if (wdev->valid_links && wdev->links[0].client.current_bss)
	#else
	if (wdev->current_bss)
	#endif
	{
		RTW_INFO(FUNC_ADPT_FMT" clear current_bss by cfg80211_disconnected\n", FUNC_ADPT_ARG(adapter));
		rtw_cfg80211_indicate_disconnect(adapter, 0, 1);
	}
	#endif

	if (pwdev_priv->pmon_ndev) {
		RTW_INFO("%s, unregister monitor interface\n", __func__);
		unregister_netdev(pwdev_priv->pmon_ndev);
	}
}

int rtw_cfg80211_ndev_res_alloc(_adapter *adapter)
{
	int ret = _FAIL;

	if (rtw_wdev_alloc(adapter, adapter_to_wiphy(adapter)) == 0)
		ret = _SUCCESS;

	return ret;
}

void rtw_cfg80211_ndev_res_free(_adapter *adapter)
{
	rtw_wdev_free(adapter->rtw_wdev);
	adapter->rtw_wdev = NULL;
}

int rtw_cfg80211_ndev_res_register(_adapter *adapter)
{
	return _SUCCESS;
}

void rtw_cfg80211_ndev_res_unregister(_adapter *adapter)
{
	rtw_wdev_unregister(adapter->rtw_wdev);
}

int rtw_cfg80211_dev_res_alloc(struct dvobj_priv *dvobj)
{
	int ret = _FAIL;
	struct wiphy *wiphy;
	struct device *dev = dvobj_to_dev(dvobj);

	wiphy = rtw_wiphy_alloc(dvobj_get_primary_adapter(dvobj), dev);
	if (wiphy == NULL)
		return ret;

	dvobj->wiphy = wiphy;

	ret = _SUCCESS;
	return ret;
}

void rtw_cfg80211_dev_res_free(struct dvobj_priv *dvobj)
{
	rtw_wiphy_free(dvobj_to_wiphy(dvobj));
	dvobj->wiphy = NULL;
}

int rtw_cfg80211_dev_res_register(struct dvobj_priv *dvobj)
{
	int ret = _FAIL;

	if (rtw_wiphy_register(dvobj_to_wiphy(dvobj)) != 0)
		return ret;

#ifdef CONFIG_RFKILL_POLL
	rtw_cfg80211_init_rfkill(dvobj_to_wiphy(dvobj));
#endif

	ret = _SUCCESS;

	return ret;
}

void rtw_cfg80211_dev_res_unregister(struct dvobj_priv *dvobj)
{
#ifdef CONFIG_RFKILL_POLL
	rtw_cfg80211_deinit_rfkill(dvobj_to_wiphy(dvobj));
#endif
	rtw_wiphy_unregister(dvobj_to_wiphy(dvobj));
}

#endif /* CONFIG_IOCTL_CFG80211 */
