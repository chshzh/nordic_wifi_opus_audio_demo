/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NET_EVENT_MGMT_H
#define NET_EVENT_MGMT_H

#include <zephyr/kernel.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>

/**
 * @brief Initialize network event handlers
 *
 * Sets up all network management event callbacks for different layers
 *
 * @return 0 on success, negative error code on failure
 */
int init_network_events(void);

/**
 * @brief Check if network is connected with an IP address assigned
 *
 * @return true if WiFi is connected and DHCP has assigned an IP address
 */
bool net_event_mgmt_is_connected(void);

/* External semaphores for network events */
extern struct k_sem iface_up_sem;
extern struct k_sem wpa_supplicant_ready_sem;
extern struct k_sem ipv4_dhcp_bond_sem;

#if IS_ENABLED(CONFIG_WIFI_NM_WPA_SUPPLICANT_AP)
extern struct k_sem station_connected_sem;
#endif

#endif /* NET_EVENT_MGMT_H */
