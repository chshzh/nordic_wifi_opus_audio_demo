/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <zephyr/kernel.h>
#include <zephyr/net/net_mgmt.h>

/**
 * @brief Run SoftAP mode
 *
 * @return 0 on success, negative error code on failure
 */
int wifi_run_softap_mode(void);

/**
 * @brief Print detailed Wi-Fi status information
 *
 * @return 0 on success, negative error code on failure
 */
int wifi_print_status(void);

/**
 * @brief Print DHCP IP address when bound
 *
 * @param cb Network management event callback containing DHCP info
 */
void wifi_print_dhcp_ip(struct net_mgmt_event_callback *cb);

/**
 * @brief Get the last connected SSID string
 *
 * @return Pointer to the last SSID stored, or NULL if no SSID is available
 */
const char *wifi_utils_get_last_ssid(void);

/**
 * @brief Ensure default SoftAP credentials exist in persistent storage.
 *
 * Stores GatewayAP WPA2-PSK credentials if they are not already present.
 *
 * @return 0 on success or if credentials already exist; negative errno otherwise.
 */
int wifi_utils_ensure_gateway_softap_credentials(void);

/**
 * @brief Request connection using stored Wi-Fi credentials.
 *
 * Triggers NET_REQUEST_WIFI_CONNECT_STORED when supported, so the station
 * automatically connects to previously stored networks.
 *
 * @return 0 on success, -EALREADY if a connection attempt is already in progress,
 *         or a negative errno code on failure.
 */
int wifi_utils_auto_connect_stored(void);

/**
 * @brief Set Wi-Fi regulatory domain
 *
 * @return 0 on success, negative error code on failure
 */
int wifi_set_reg_domain(void);

/**
 * @brief Set Wi-Fi channel for raw packet operations
 *
 * @param channel Channel number to set
 * @return 0 on success, negative error code on failure
 */
int wifi_set_channel(int channel);

/**
 * @brief Set Wi-Fi mode
 *
 * @param mode Mode value to set
 * @return 0 on success, negative error code on failure
 */
int wifi_set_mode(int mode);

/**
 * @brief Enable TX injection mode
 *
 * @return 0 on success, negative error code on failure
 */
int wifi_set_tx_injection_mode(void);

#endif /* WIFI_UTILS_H */
