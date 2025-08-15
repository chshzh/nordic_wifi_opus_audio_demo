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
