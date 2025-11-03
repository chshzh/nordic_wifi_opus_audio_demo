/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi_credentials.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <string.h>

#include "wifi_utils.h"
#if defined(CONFIG_SOCKET_ROLE_CLIENT)
#include "socket_utils.h"
#endif

LOG_MODULE_REGISTER(wifi_utils, CONFIG_LOG_DEFAULT_LEVEL);

#define GATEWAY_SOFTAP_SSID     "GatewayAP"
#define GATEWAY_SOFTAP_PASSWORD "wifi1234"

static char last_connected_ssid[WIFI_SSID_MAX_LEN + 1];

const char *wifi_utils_get_last_ssid(void)
{
	if (last_connected_ssid[0] == '\0') {
		return NULL;
	}

	return last_connected_ssid;
}

int wifi_utils_ensure_gateway_softap_credentials(void)
{
#if !defined(CONFIG_WIFI_CREDENTIALS)
	return -ENOTSUP;
#else
	struct wifi_credentials_personal creds = {0};
	size_t ssid_len = strlen(GATEWAY_SOFTAP_SSID);
	int ret =
		wifi_credentials_get_by_ssid_personal_struct(GATEWAY_SOFTAP_SSID, ssid_len, &creds);

	if (ret == 0) {
		return 0;
	}

	if (ret != -ENOENT) {
		LOG_ERR("Failed to read stored credentials for %s: %d", GATEWAY_SOFTAP_SSID, ret);
		return ret;
	}

	creds.header.type = WIFI_SECURITY_TYPE_PSK;
	memcpy(creds.header.ssid, GATEWAY_SOFTAP_SSID, ssid_len);
	creds.header.ssid_len = ssid_len;
	creds.header.flags = WIFI_CREDENTIALS_FLAG_FAVORITE | WIFI_CREDENTIALS_FLAG_2_4GHz;
	creds.password_len = strlen(GATEWAY_SOFTAP_PASSWORD);
	memcpy(creds.password, GATEWAY_SOFTAP_PASSWORD, creds.password_len);
	creds.password[creds.password_len] = '\0';

	ret = wifi_credentials_set_personal_struct(&creds);
	if (ret == 0) {
		LOG_INF("Stored default Wi-Fi credentials for %s", GATEWAY_SOFTAP_SSID);
	} else {
		LOG_ERR("Failed to store default credentials for %s: %d", GATEWAY_SOFTAP_SSID, ret);
	}

	return ret;
#endif
}

int wifi_utils_auto_connect_stored(void)
{

#if !defined(CONFIG_WIFI_CREDENTIALS_CONNECT_STORED)
	return -ENOTSUP;
#else
	struct net_if *iface = net_if_get_first_wifi();
	int ret;

	if (iface == NULL) {
		return -ENODEV;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);
	if (ret == 0) {
		LOG_INF("Auto-connect request issued for stored Wi-Fi credentials");
	} else if (ret != -EALREADY) {
		LOG_WRN("Auto-connect request failed: %d", ret);
	}

	return ret;
#endif
}

int wifi_set_mode(int mode)
{
	struct net_if *iface;
	struct wifi_mode_info mode_info = {0};
	int ret;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return -ENODEV;
	}

	mode_info.oper = WIFI_MGMT_SET;
	mode_info.if_index = net_if_get_by_iface(iface);
	mode_info.mode = mode;

	ret = net_mgmt(NET_REQUEST_WIFI_MODE, iface, &mode_info, sizeof(mode_info));
	if (ret) {
		LOG_ERR("Mode setting failed: %d", ret);
		return ret;
	}

	LOG_INF("Wi-Fi mode set to %d", mode);
	return 0;
}

int wifi_set_channel(int channel)
{
	struct net_if *iface;
	struct wifi_channel_info channel_info = {0};
	int ret;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return -ENODEV;
	}

	channel_info.oper = WIFI_MGMT_SET;
	channel_info.if_index = net_if_get_by_iface(iface);
	channel_info.channel = channel;

	if ((channel_info.channel < WIFI_CHANNEL_MIN) ||
	    (channel_info.channel > WIFI_CHANNEL_MAX)) {
		LOG_ERR("Invalid channel number: %d. Range is (%d-%d)", channel, WIFI_CHANNEL_MIN,
			WIFI_CHANNEL_MAX);
		return -EINVAL;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_CHANNEL, iface, &channel_info, sizeof(channel_info));
	if (ret) {
		LOG_ERR("Channel setting failed: %d", ret);
		return ret;
	}

	LOG_INF("Wi-Fi channel set to %d", channel_info.channel);
	return 0;
}

int wifi_set_tx_injection_mode(void)
{
	struct net_if *iface;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return -ENODEV;
	}

	if (net_eth_txinjection_mode(iface, true)) {
		LOG_ERR("TX Injection mode enable failed");
		return -1;
	}

	LOG_INF("TX Injection mode enabled");
	return 0;
}

#if IS_ENABLED(CONFIG_WIFI_NM_WPA_SUPPLICANT_AP)
int wifi_set_reg_domain(void)
{
	struct net_if *iface;
	struct wifi_reg_domain regd = {0};
	int ret = -1;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return ret;
	}

	regd.oper = WIFI_MGMT_SET;
#ifdef CONFIG_SOFTAP_REG_DOMAIN
	strncpy(regd.country_code, CONFIG_SOFTAP_REG_DOMAIN, (WIFI_COUNTRY_CODE_LEN + 1));
#else
	strncpy(regd.country_code, "US", (WIFI_COUNTRY_CODE_LEN + 1));
#endif

	ret = net_mgmt(NET_REQUEST_WIFI_REG_DOMAIN, iface, &regd, sizeof(regd));
	if (ret) {
		LOG_ERR("Cannot %s Regulatory domain: %d", "SET", ret);
	} else {
#ifdef CONFIG_SOFTAP_REG_DOMAIN
		LOG_INF("Regulatory domain set to %s", CONFIG_SOFTAP_REG_DOMAIN);
#else
		LOG_INF("Regulatory domain set to US");
#endif
	}

	return ret;
}

static int wifi_set_softap(const char *ssid, const char *psk)
{
	struct net_if *iface;
	struct wifi_connect_req_params params = {0};
	int ret;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	/* Configure AP parameters */
	params.ssid = (uint8_t *)ssid;
	params.ssid_length = strlen(params.ssid);
	if (params.ssid_length > WIFI_SSID_MAX_LEN) {
		LOG_ERR("SSID length is too long, expected is %d characters long",
			WIFI_SSID_MAX_LEN);
		return -1;
	}
	params.psk = (uint8_t *)psk;
	params.psk_length = strlen(params.psk);
	params.band = WIFI_FREQ_BAND_2_4_GHZ;
	params.channel = 1;
	params.security = WIFI_SECURITY_TYPE_PSK;

	/* Enable AP mode */
	ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &params,
		       sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("AP mode enable failed: %s", strerror(-ret));
	} else {
		LOG_INF("AP mode enabled");
	}

	return 0;
}

static bool dhcp_server_started;
static int setup_dhcp_server(void)
{
	struct net_if *iface;
	struct in_addr pool_start;
	int ret;

	if (dhcp_server_started) {
		LOG_WRN("DHCP server already started");
		return 0;
	}

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	/* Set DHCP pool start address */
	ret = inet_pton(AF_INET, "192.168.1.2", &pool_start);
	if (ret != 1) {
		LOG_ERR("Invalid DHCP pool start address");
		return -1;
	}

	ret = net_dhcpv4_server_start(iface, &pool_start);
	if (ret == -EALREADY) {
		LOG_INF("DHCP server already running");
		dhcp_server_started = true;
		return 0;
	} else if (ret < 0) {
		LOG_ERR("Failed to start DHCP server: %d", ret);
		return ret;
	}

	dhcp_server_started = true;
	LOG_INF("DHCP server started with pool starting at 192.168.1.2");
	return 0;
}

int wifi_run_softap_mode(void)
{
	int ret;

	LOG_INF("Setting up SoftAP mode");

	/* Set regulatory domain */
	ret = wifi_set_reg_domain();
	if (ret) {
		LOG_ERR("Failed to set regulatory domain: %d", ret);
		return ret;
	}

	/* Setup DHCP server */
	ret = setup_dhcp_server();
	if (ret) {
		LOG_ERR("Failed to setup DHCP server: %d", ret);
		return ret;
	}

	/* Setup SoftAP */
	ret = wifi_set_softap(CONFIG_SOFTAP_SSID, CONFIG_SOFTAP_PASSWORD);
	if (ret) {
		LOG_ERR("Failed to setup SoftAP: %d", ret);
		return ret;
	}

	return 0;
}

#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_AP */

int wifi_print_status(void)
{
	struct net_if *iface;
	struct wifi_iface_status status = {0};
	int ret;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -ENODEV;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
		       sizeof(struct wifi_iface_status));
	if (ret) {
		LOG_ERR("Status request failed: %d", ret);
		return ret;
	}

	LOG_INF("Wi-Fi Status: successful");
	LOG_INF("==================");
	LOG_INF("State: %s", wifi_state_txt(status.state));

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		strncpy(last_connected_ssid, status.ssid, WIFI_SSID_MAX_LEN);
		last_connected_ssid[WIFI_SSID_MAX_LEN] = '\0';
		LOG_INF("Interface Mode: %s", wifi_mode_txt(status.iface_mode));
		LOG_INF("SSID: %.32s", status.ssid);
		LOG_INF("BSSID: %02x:%02x:%02x:%02x:%02x:%02x", status.bssid[0], status.bssid[1],
			status.bssid[2], status.bssid[3], status.bssid[4], status.bssid[5]);
		LOG_INF("Band: %s", wifi_band_txt(status.band));
		LOG_INF("Channel: %d", status.channel);
		LOG_INF("Security: %s", wifi_security_txt(status.security));
		LOG_INF("RSSI: %d dBm", status.rssi);
	} else {
		last_connected_ssid[0] = '\0';
	}

	return 0;
}

#if defined(CONFIG_NET_DHCPV4)
void wifi_print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
	const struct net_if_dhcpv4 *dhcpv4 = cb->info;
	const struct in_addr *addr = &dhcpv4->requested_ip;
	char dhcp_info[128];

	net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));
	LOG_INF("\r\n\r\nDevice IP address: %s\r\n", dhcp_info);
#if defined(CONFIG_SOCKET_ROLE_CLIENT)
	const char *connected_ssid = wifi_utils_get_last_ssid();

	if ((connected_ssid != NULL) &&
	    (strncmp(connected_ssid, GATEWAY_SOFTAP_SSID, WIFI_SSID_MAX_LEN) == 0) &&
	    !socket_utils_is_target_set()) {
		struct in_addr gateway_addr;

		if (inet_pton(AF_INET, "192.168.1.1", &gateway_addr) == 1) {
			socket_utils_set_target_ipv4(&gateway_addr);
		} else {
			LOG_WRN("Failed to parse static gateway address");
		}
	}
#endif
}
#else
void wifi_print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
	ARG_UNUSED(cb);
}
#endif
