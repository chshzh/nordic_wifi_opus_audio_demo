/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(socket_utils, CONFIG_SOCKET_UTILS_LOG_LEVEL);

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/shell/shell.h>
#include "socket_utils.h"
#include "net_event_mgmt.h"
#include "wifi_utils.h"

#include <zephyr/net/dns_resolve.h>

/* size of stack area used by each thread */
#define STACKSIZE 8192
/* scheduling priority used by each thread */
#define PRIORITY  3

// #define pc_port  60000
#define socket_port 60010 // UDP audio transport port

/**********External Resources START**************/
#if IS_ENABLED(CONFIG_WIFI_NM_WPA_SUPPLICANT_AP)
extern struct k_sem station_connected_sem;
#endif
/**********External Resources END**************/
static int udp_socket;
struct sockaddr_in self_addr;
char target_addr_str[32];
struct sockaddr_in target_addr;
socklen_t target_addr_len = sizeof(target_addr);

static socket_receive_t socket_receive;

K_MSGQ_DEFINE(socket_recv_queue, sizeof(socket_receive), 1, 4);

static net_util_socket_rx_callback_t socket_rx_cb;

#if defined(CONFIG_SOCKET_ROLE_CLIENT)
volatile bool serveraddr_set_signall = false;

static socket_utils_target_ready_cb_t target_ready_cb;
static bool socket_ready;
static bool target_ready_notified;

static void socket_utils_notify_target_ready(void)
{
	if (!serveraddr_set_signall || !socket_ready || target_ready_notified ||
	    target_ready_cb == NULL) {
		return;
	}

	target_ready_notified = true;
	target_ready_cb();
}

void socket_utils_set_target_ready_callback(socket_utils_target_ready_cb_t cb)
{
	target_ready_cb = cb;
	socket_utils_notify_target_ready();
}
#endif /*#if defined(CONFIG_SOCKET_ROLE_CLIENT)*/

volatile bool socket_connected_signall = false;

void socket_utils_set_rx_callback(net_util_socket_rx_callback_t socket_rx_callback)
{
	socket_rx_cb = socket_rx_callback;

	// If any messages are waiting in the queue, forward them immediately
	socket_receive_t socket_receive;

	while (k_msgq_get(&socket_recv_queue, &socket_receive, K_NO_WAIT) == 0) {
		socket_rx_cb(socket_receive.buf, socket_receive.len);
	}
}

static void socket_utils_trigger_rx_callback_if_set(void)
{
	LOG_DBG("Socket received %d bytes", socket_receive.len);
	// LOG_HEXDUMP_DBG(socket_receive.buf, socket_receive.len, "Buffer contents(HEX):");
	if (socket_rx_cb != 0) {
		socket_rx_cb(socket_receive.buf, socket_receive.len);
	} else {
		k_msgq_put(&socket_recv_queue, &socket_receive, K_NO_WAIT);
	}
}

#if defined(CONFIG_SOCKET_ROLE_CLIENT)
bool socket_utils_is_target_set(void)
{
	return serveraddr_set_signall;
}

void socket_utils_set_target_ipv4(const struct in_addr *addr)
{
	if ((addr == NULL) || (addr->s_addr == 0U)) {
		return;
	}

	if (serveraddr_set_signall && (target_addr.sin_addr.s_addr == addr->s_addr)) {
		return;
	}

	target_addr.sin_family = AF_INET;
	target_addr.sin_port = htons(socket_port);
	target_addr.sin_addr = *addr;

	inet_ntop(target_addr.sin_family, &target_addr.sin_addr, target_addr_str,
		  sizeof(target_addr_str));
	LOG_INF("Target address set to %s:%d", target_addr_str, socket_port);
	serveraddr_set_signall = true;
	target_ready_notified = false;
	socket_utils_notify_target_ready();
}

void socket_utils_clear_target(void)
{
	serveraddr_set_signall = false;
	target_ready_notified = false;
	socket_ready = false;
	LOG_INF("Cleared socket target state");
}
#endif /* CONFIG_SOCKET_ROLE_CLIENT */

int socket_utils_tx_data(uint8_t *data, size_t length)
{
#if defined(CONFIG_SOCKET_ROLE_SERVER)
	if (!socket_connected_signall || target_addr.sin_addr.s_addr == 0) {
		errno = ENOTCONN;
		LOG_DBG("Socket target not ready, dropping %zu byte payload", length);
		return -ENOTCONN;
	}
#else
	if (!serveraddr_set_signall || target_addr.sin_addr.s_addr == 0) {
		errno = ENOTCONN;
		LOG_DBG("Socket target unknown, dropping %zu byte payload", length);
		return -ENOTCONN;
	}
#endif

	size_t chunk_size = 1024;
	ssize_t bytes_sent = 0;

	while (length > 0) {
		size_t to_send = (length >= chunk_size) ? chunk_size : length;

		bytes_sent = sendto(udp_socket, data, to_send, 0, (struct sockaddr *)&target_addr,
				    sizeof(target_addr));

		if (bytes_sent == -1) {
			perror("Sending failed");
			return bytes_sent;
		}

		data += bytes_sent;
		length -= bytes_sent;
	}
	return bytes_sent;
}

#ifdef CONFIG_MDNS_RESOLVER
int do_mdns_query(void)
{
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_socktype = SOCK_DGRAM,
		.ai_family = AF_INET,
	};

	char addr_str[NET_IPV6_ADDR_LEN];
	int err;

	for (int i = 1; i <= CONFIG_MDNS_QUERY_ATTEMPTS; i++) {
		err = getaddrinfo(CONFIG_MDNS_QUERY_NAME, NULL, &hints, &result);
		if (!err) {
			LOG_INF("Got address from mDNS at attempt %d", i);
			break;
		}
		LOG_DBG("Failed to get address from mDNS at attempt %d, error %d", i, err);
	}
	if (err) {
		LOG_ERR("getaddrinfo() failed, error %d", err);
		return err;
	}

	for (addr = result; addr; addr = addr->ai_next) {
		if (addr->ai_family == AF_INET) {
			struct sockaddr_in *addr4 = (struct sockaddr_in *)addr->ai_addr;

			inet_ntop(AF_INET, &addr4->sin_addr, addr_str, sizeof(addr_str));
			// LOG_INF("IPv4 address: %s", addr_str);

			if (addr4->sin_addr.s_addr == 0) {
				LOG_ERR("Invalid IP address");
				continue;
			}

			socket_utils_set_target_ipv4(&addr4->sin_addr);

		} else if (addr->ai_family == AF_INET6) {
			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr->ai_addr;

			inet_ntop(AF_INET6, &addr6->sin6_addr, addr_str, sizeof(addr_str));
			LOG_INF("IPv6 address: %s", addr_str);
		}
	}
	return 0;
}
#endif /* CONFIG_MDNS_RESOLVER */

#if defined(CONFIG_SOCKET_ROLE_SERVER)
void socket_utils_softap_handle_disconnect(void)
{
	socket_connected_signall = false;
	memset(&target_addr, 0, sizeof(target_addr));
	target_addr_len = sizeof(target_addr);
	LOG_INF("SoftAP client disconnected, socket target cleared");
}
#endif

/* Thread to setup WiFi, Sockets step by step */
void socket_utils_thread(void)
{
	int ret;

	ret = init_network_events();
	if (ret) {
		LOG_ERR("Failed to initialize network events: %d", ret);
		return;
	}
	k_sem_take(&wpa_supplicant_ready_sem, K_FOREVER);

#if IS_ENABLED(CONFIG_WIFI_NM_WPA_SUPPLICANT_AP)
	/* Device in SoftAP mode */
	LOG_INF("Wi-Fi Mode: SoftAP mode");

	ret = wifi_run_softap_mode();
	if (ret) {
		LOG_ERR("Failed to setup SoftAP mode: %d", ret);
		return;
	}

	ret = k_sem_take(&ipv4_dhcp_bond_sem, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to wait for SoftAP network setup: %d", ret);
		return;
	}

	wifi_print_status();

	LOG_INF("SoftAP setup complete, waiting for station to connect...");
	LOG_INF("SSID: %s", CONFIG_SOFTAP_SSID);
	LOG_INF("Password: %s", CONFIG_SOFTAP_PASSWORD);
	LOG_INF("Socket server will start once a station connects");

	ret = k_sem_take(&station_connected_sem, K_FOREVER);
	if (ret) {
		LOG_ERR("Error waiting for station connection: %d", ret);
		return;
	}

	LOG_INF("Station connected! Starting socket server...");

#else
	/* Device in station mode */
	LOG_INF("Wi-Fi Mode: Station mode");
#if IS_ENABLED(CONFIG_WIFI_CREDENTIALS_STATIC)
	LOG_INF("Static Wi-Fi credentials configured for connection.");
#elif IS_ENABLED(CONFIG_WIFI_CREDENTIALS_SHELL)
	LOG_INF("Please use \"wifi cred\" shell commands set up Wi-Fi connection.");
#else
	LOG_INF("No Proper Wi-Fi credentials configured, try to configure with "
		"CONFIG_WIFI_CREDENTIALS_STATIC or CONFIG_WIFI_CREDENTIALS_SHELL");
#endif /* IS_ENABLED(CONFIG_WIFI_CREDENTIALS_STATIC) */
#if IS_ENABLED(CONFIG_SOCKET_ROLE_CLIENT)
	int cred_ret = wifi_utils_ensure_gateway_softap_credentials();
	if (cred_ret && cred_ret != -ENOTSUP) {
		LOG_WRN("Provisioning default GatewayAP credentials failed: %d", cred_ret);
	}

	int auto_ret = wifi_utils_auto_connect_stored();
	if (auto_ret && auto_ret != -EALREADY && auto_ret != -ENOTSUP) {
		LOG_WRN("Auto-connect to stored credentials failed: %d", auto_ret);
	}
#endif
	ret = conn_mgr_all_if_connect(true);
	if (ret) {
		LOG_ERR("Failed to initiate network connection: %d", ret);
		return;
	}
	ret = k_sem_take(&ipv4_dhcp_bond_sem, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to wait for network connectivity: %d", ret);
		return;
	}

	LOG_INF("Network connectivity established, setting up sockets...");

#endif /* IS_ENABLED(CONFIG_WIFI_NM_WPA_SUPPLICANT_AP) */

	self_addr.sin_family = AF_INET;
	self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	self_addr.sin_port = htons(socket_port);

	target_addr.sin_family = AF_INET;

#if defined(CONFIG_SOCKET_ROLE_CLIENT)
	if (!socket_utils_is_target_set()) {
#ifdef CONFIG_MDNS_RESOLVER
		ret = do_mdns_query();
#else
		ret = -ENOTSUP;
#endif /* CONFIG_MDNS_RESOLVER */

		if (ret < 0) {
			LOG_INF("mDNS lookup unavailable (err %d); waiting for DHCP-based target "
				"configuration",
				ret);
		}
	} else {
		LOG_DBG("Target address already provisioned; skipping mDNS lookup");
	}

	while (!serveraddr_set_signall) {
		k_sleep(K_MSEC(100));
	}
	LOG_INF("Target address is set. Initializing socket transport");
#elif defined(CONFIG_SOCKET_ROLE_SERVER)
	LOG_INF("\r\n\r\nDevice works as socket server, wait for socket client connection...\r\n");
#endif // #if defined(CONFIG_SOCKET_ROLE_CLIENT)

#if defined(CONFIG_SOCKET_ROLE_CLIENT)
	socket_ready = false;
#endif

	for (;;) {
		udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (udp_socket < 0) {
			LOG_ERR("Failed to create socket: %d", -errno);
			k_sleep(K_SECONDS(1));
			continue;
		}

		ret = bind(udp_socket, (struct sockaddr *)&self_addr, sizeof(self_addr));
		if (ret < 0) {
			LOG_ERR("bind, error: %d", -errno);
			close(udp_socket);
			k_sleep(K_SECONDS(1));
			continue;
		}

#if defined(CONFIG_SOCKET_ROLE_CLIENT)
		if (serveraddr_set_signall && !socket_ready) {
			socket_ready = true;
			socket_utils_notify_target_ready();
		}
#endif

		while (true) {
			target_addr_len = sizeof(target_addr);
			socket_receive.len =
				recvfrom(udp_socket, socket_receive.buf, BUFFER_MAX_SIZE, 0,
					 (struct sockaddr *)&target_addr, &target_addr_len);
			if (socket_receive.len <= 0) {
				break;
			}
#if defined(CONFIG_SOCKET_ROLE_CLIENT)
			if (!serveraddr_set_signall) {
				inet_ntop(target_addr.sin_family, &target_addr.sin_addr,
					  target_addr_str, sizeof(target_addr_str));
				LOG_INF("Discovered socket server at %s:%d", target_addr_str,
					ntohs(target_addr.sin_port));
				socket_utils_set_target_ipv4(&target_addr.sin_addr);
			}

			if (!socket_ready) {
				socket_ready = true;
				socket_utils_notify_target_ready();
			}

#endif
			if (!socket_connected_signall) {
				inet_ntop(target_addr.sin_family, &target_addr.sin_addr,
					  target_addr_str, sizeof(target_addr_str));
				LOG_INF("Connect socket to IP Address %s:%d\n", target_addr_str,
					ntohs(target_addr.sin_port));
				socket_connected_signall = true;
			}
			socket_utils_trigger_rx_callback_if_set();
		}

		if (socket_receive.len == -1) {
			LOG_ERR("Receiving failed");
		} else if (socket_receive.len == 0) {
			LOG_INF("Client disconnected.\n");
		}

		close(udp_socket);
		socket_connected_signall = false;
#if defined(CONFIG_SOCKET_ROLE_CLIENT)
		socket_ready = false;
		target_ready_notified = false;
#endif
		k_sleep(K_SECONDS(1));
	}
}

#if defined(CONFIG_SOCKET_ROLE_CLIENT)

static int cmd_set_target_address(const struct shell *shell, size_t argc, const char **argv)
{
	// Ensure the command is provided with exactly one argument
	if (argc != 2) {
		shell_print(shell, "Usage: socket set_target_addr <IP:Port>");
		return -1;
	}

	char *target_addr_str = (char *)k_malloc(22); // Allocate memory for the string

	if (target_addr_str == NULL) {
		shell_print(shell, "Memory allocation failed");
		return -1;
	}

	// Get the target address string from the command argument
	strncpy(target_addr_str, argv[1], 22); // Use strncpy instead of strcpy for safety
	target_addr_str[21] = '\0';            // Ensure null-termination

	char ip_str[INET_ADDRSTRLEN];
	int port;

	// Split into IP address and port
	if (sscanf(target_addr_str, "%[^:]:%d", ip_str, &port) != 2) {
		shell_print(shell, "Invalid format. Expected <IP>:<Port>");
		k_free(target_addr_str); // Free the allocated memory
		return -1;
	}

	// Set up the sockaddr_in structure
	target_addr.sin_family = AF_INET;
	target_addr.sin_port = htons(port); // Convert port to network byte order

	if (inet_pton(AF_INET, ip_str, &(target_addr.sin_addr)) <= 0) {
		shell_print(shell, "Invalid IP address format: %s", ip_str);
		k_free(target_addr_str);
		return -1;
	}

	shell_print(shell, "Target address set to: %s:%d", ip_str, port);
	serveraddr_set_signall = true;
	target_ready_notified = false;
	socket_utils_notify_target_ready();
	k_free(target_addr_str);
	return 0;
}

// Existing shell command definitions
SHELL_STATIC_SUBCMD_SET_CREATE(socket_cmd,
			       SHELL_CMD(set_target_addr, NULL,
					 "Get and set target address in format <IP:Port>",
					 cmd_set_target_address),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(socket, &socket_cmd, "Socket commands", NULL);

#endif // #if defined(CONFIG_SOCKET_ROLE_CLIENT)
