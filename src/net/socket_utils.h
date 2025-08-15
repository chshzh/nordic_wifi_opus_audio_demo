/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef _SOCKET_UTILS_H_
#define _SOCKET_UTILS_H_

#include <zephyr/kernel.h>

/*Wi-Fi default MTU*/
#define BUFFER_MAX_SIZE 1500

typedef struct {
	uint8_t buf[BUFFER_MAX_SIZE];
	size_t len;
} socket_receive_t;

extern struct k_msgq socket_recv_queue;

typedef void (*net_util_socket_rx_callback_t)(uint8_t *data, size_t len);

void socket_utils_set_rx_callback(net_util_socket_rx_callback_t socket_rx_callback);
int socket_utils_tx_data(uint8_t *data, size_t length);
void socket_utils_thread(void);

/* SoftAP helper functions */
bool wifi_softap_has_connected_stations(void);
int wifi_softap_wait_for_station(k_timeout_t timeout);

#endif
