/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LATENCY_MEASURE_H
#define LATENCY_MEASURE_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize latency measurement GPIO pins
 *
 * @return 0 on success, negative error code on failure
 */
int latency_measure_init(void);

/**
 * @brief Trigger timing point for audio input capture (T1)
 * Gateway device only - Arduino pin D2
 */
void latency_measure_t1_audio_capture(void);

/**
 * @brief Trigger timing point for encoding start (T2)
 * Gateway device only - Arduino pin D3
 */
void latency_measure_t2_encode_start(void);

/**
 * @brief Trigger timing point for encoding complete (T3)
 * Gateway device only - Arduino pin D4
 */
void latency_measure_t3_encode_complete(void);

/**
 * @brief Trigger timing point for network transmission (T4)
 * Gateway device only - Arduino pin D5
 */
void latency_measure_t4_network_tx(void);

/**
 * @brief Trigger timing point for network reception (T5)
 * Headset device only - Arduino pin D2
 */
void latency_measure_t5_network_rx(void);

/**
 * @brief Trigger timing point for decoding start (T6)
 * Headset device only - Arduino pin D3
 */
void latency_measure_t6_decode_start(void);

/**
 * @brief Trigger timing point for decoding complete (T7)
 * Headset device only - Arduino pin D4
 */
void latency_measure_t7_decode_complete(void);

/**
 * @brief Trigger timing point for audio output (T8)
 * Headset device only - Arduino pin D5
 */
void latency_measure_t8_audio_output(void);

#endif /* LATENCY_MEASURE_H */
