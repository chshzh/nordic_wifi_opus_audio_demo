/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HEAPS_MONITOR_H_
#define _HEAPS_MONITOR_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/heap_listener.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize heap monitoring system
 *
 * This function sets up heap listeners for system and WiFi data heaps.
 * Should be called early in main() before any significant heap allocations.
 *
 * @return 0 on success, negative error code on failure
 */
int heaps_monitor_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _HEAPS_MONITOR_H_ */
