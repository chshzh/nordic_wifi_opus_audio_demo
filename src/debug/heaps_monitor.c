/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Heap Memory Monitoring Module
 */

#define MODULE heaps_monitor

#include <zephyr/kernel.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/logging/log.h>

#include "heaps_monitor.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_HEAPS_MONITOR_LOG_LEVEL);

#ifdef CONFIG_HEAPS_MONITOR

/* Update interval: 10 seconds in milliseconds */
#define HEAP_UPDATE_INTERVAL_MS 10000

/* Heap monitoring structure */
struct heap_monitor_entry {
	struct k_heap *heap_ptr;       /* Pointer to the k_heap */
	const char *heap_name;         /* Human-readable name for logging */
	uint32_t heap_size;            /* Total heap size in bytes */
	uint32_t heap_free;            /* Current free bytes */
	uint32_t heap_used;            /* Current used bytes */
	uint32_t heap_max_used;        /* Peak usage in bytes */
	uint32_t heap_last_peak;       /* Last logged peak value */
	struct sys_memory_stats stats; /* Memory statistics */
	bool is_registered;            /* True if this entry is in use */
};

/* Forward declaration for timer callback */
static void periodic_heap_report(struct k_timer *timer_id);

/* System heap monitoring */
extern struct sys_heap _system_heap;
static struct heap_monitor_entry system_heap_monitor = {.heap_ptr = NULL,
							.heap_name = "System Heap",
							.heap_size = K_HEAP_MEM_POOL_SIZE,
							.is_registered = true};

/* WiFi driver data memory pool monitoring */
extern struct k_heap wifi_drv_data_mem_pool;
static struct heap_monitor_entry wifi_heap_monitor = {.heap_ptr = &wifi_drv_data_mem_pool,
						      .heap_name = "WiFi DATA Heap",
						      .heap_size = CONFIG_NRF_WIFI_DATA_HEAP_SIZE,
						      .is_registered = true};

/* Periodic timer for heap status reporting */
static K_TIMER_DEFINE(heap_report_timer, periodic_heap_report, NULL);

/**
 * @brief Update heap statistics for a monitor entry
 * @param monitor Pointer to heap monitor entry
 */
static void update_heap_stats(struct heap_monitor_entry *monitor)
{
	if (monitor->heap_ptr != NULL) {
		/* k_heap case */
		sys_heap_runtime_stats_get(&monitor->heap_ptr->heap, &monitor->stats);
	} else {
		/* System heap case */
		sys_heap_runtime_stats_get(&_system_heap, &monitor->stats);
	}

	monitor->heap_used = (uint32_t)monitor->stats.allocated_bytes;
	monitor->heap_max_used = (uint32_t)monitor->stats.max_allocated_bytes;
	monitor->heap_free = (uint32_t)monitor->stats.free_bytes;
}

/**
 * @brief Periodic heap status reporter - prints current heap status every 10 seconds
 * @param timer_id Timer that triggered this callback
 */
static void periodic_heap_report(struct k_timer *timer_id)
{
	/* Update and report system heap status */
	update_heap_stats(&system_heap_monitor);
	LOG_INF("%s PERIODIC, Peak/Total: %u/%u", system_heap_monitor.heap_name,
		system_heap_monitor.heap_max_used, system_heap_monitor.heap_size);

	/* Update and report WiFi heap status */
	update_heap_stats(&wifi_heap_monitor);
	LOG_INF("%s PERIODIC, Peak/Total: %u/%u", wifi_heap_monitor.heap_name,
		wifi_heap_monitor.heap_max_used, wifi_heap_monitor.heap_size);
}

/**
 * @brief Standardized heap status logging with intelligent update frequency
 * @param monitor Pointer to heap monitor entry
 * @param bytes Bytes allocated/freed in this operation
 * @param mem Memory pointer (for alloc operations, NULL for free)
 * @param is_alloc True for allocation, false for free
 */
static void log_heap_status(struct heap_monitor_entry *monitor, size_t bytes, void *mem,
			    bool is_alloc)
{
	bool peak_changed = (monitor->heap_max_used != monitor->heap_last_peak);

	/* Log immediately only if peak changed - periodic logging handled by timer */
	if (peak_changed) {
		LOG_INF("%s ALLOC, Peak/Total: %u/%u", monitor->heap_name, monitor->heap_max_used,
			monitor->heap_size);
		monitor->heap_last_peak = monitor->heap_max_used;
	}
}

/**
 * @brief Find heap monitor entry by heap pointer
 * @param heap_ptr Pointer to the heap
 * @return Pointer to monitor entry or NULL if not found
 */
static struct heap_monitor_entry *find_heap_monitor(void *heap_ptr)
{
	/* Check system heap first - compare with HEAP_ID_FROM_POINTER(&_system_heap) */
	if (heap_ptr == (void *)HEAP_ID_FROM_POINTER(&_system_heap)) {
		return &system_heap_monitor;
	}

	/* Check WiFi heap - compare with HEAP_ID_FROM_POINTER(&wifi_drv_data_mem_pool.heap) */
	if (heap_ptr == (void *)HEAP_ID_FROM_POINTER(&wifi_drv_data_mem_pool.heap)) {
		return &wifi_heap_monitor;
	}

	return NULL;
}

/**
 * @brief Generic heap allocation callback
 */
static void on_heap_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
	struct heap_monitor_entry *monitor = find_heap_monitor((void *)heap_id);

	if (monitor != NULL) {
		update_heap_stats(monitor);
		log_heap_status(monitor, bytes, mem, true);
	}
}

/**
 * @brief Generic heap free callback
 */
static void on_heap_free(uintptr_t heap_id, void *mem, size_t bytes)
{
	struct heap_monitor_entry *monitor = find_heap_monitor((void *)heap_id);

	if (monitor != NULL) {
		update_heap_stats(monitor);
		log_heap_status(monitor, bytes, mem, false);
	}
}

/* System heap listeners */
HEAP_LISTENER_ALLOC_DEFINE(on_system_heap_alloc, HEAP_ID_FROM_POINTER(&_system_heap),
			   on_heap_alloc);
HEAP_LISTENER_FREE_DEFINE(on_system_heap_free, HEAP_ID_FROM_POINTER(&_system_heap), on_heap_free);

/* WiFi heap listeners */
HEAP_LISTENER_ALLOC_DEFINE(on_target_k_heap_alloc,
			   HEAP_ID_FROM_POINTER(&wifi_drv_data_mem_pool.heap), on_heap_alloc);
HEAP_LISTENER_FREE_DEFINE(on_target_k_heap_free, HEAP_ID_FROM_POINTER(&wifi_drv_data_mem_pool.heap),
			  on_heap_free);

int heaps_monitor_init(void)
{
	/* Register system heap listeners */
	heap_listener_register(&on_system_heap_alloc);
	heap_listener_register(&on_system_heap_free);

	/* Register WiFi heap listeners */
	heap_listener_register(&on_target_k_heap_alloc);
	heap_listener_register(&on_target_k_heap_free);

	/* Start periodic heap reporting timer */
	k_timer_start(&heap_report_timer, K_MSEC(HEAP_UPDATE_INTERVAL_MS),
		      K_MSEC(HEAP_UPDATE_INTERVAL_MS));

	LOG_INF("Heap monitoring system initialized (System + WiFi Data heaps)");
	return 0;
}

#endif /* CONFIG_HEAPS_MONITOR */
