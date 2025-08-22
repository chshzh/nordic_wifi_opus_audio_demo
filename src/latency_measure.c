/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "latency_measure.h"

#ifdef CONFIG_LATENCY_MEASUREMENT

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(latency_measure, CONFIG_LOG_DEFAULT_LEVEL);

/* GPIO pin definitions using new pin assignments */
#define LATENCY_PIN_T1_T5 26 /* P0.26 */
#define LATENCY_PIN_T2_T6 25 /* P0.25 */
#define LATENCY_PIN_T3_T7 7  /* P0.07 */
#define LATENCY_PIN_T4_T8 28 /* P0.28 */

/* GPIO device references */
static const struct device *gpio0_dev;
static const struct device *gpio1_dev;

/* GPIO pin specifications */
static const struct gpio_dt_spec pin_t1_t5 = {.port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
					      .pin = 26, /* P0.26 */
					      .dt_flags = GPIO_ACTIVE_HIGH};

static const struct gpio_dt_spec pin_t2_t6 = {.port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
					      .pin = 25, /* P0.25 */
					      .dt_flags = GPIO_ACTIVE_HIGH};

static const struct gpio_dt_spec pin_t3_t7 = {.port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
					      .pin = 7, /* P0.07 */
					      .dt_flags = GPIO_ACTIVE_HIGH};

static const struct gpio_dt_spec pin_t4_t8 = {.port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
					      .pin = 28, /* P0.28 */
					      .dt_flags = GPIO_ACTIVE_HIGH};

/* Pulse duration from configuration */
#define PULSE_DURATION_US CONFIG_LATENCY_PULSE_DURATION_MS

/* Work items for timing pulses */
static struct k_work_delayable pulse_work_t1_t5;
static struct k_work_delayable pulse_work_t2_t6;
static struct k_work_delayable pulse_work_t3_t7;
static struct k_work_delayable pulse_work_t4_t8;

/* Work handlers to turn off pins after pulse */
static void pulse_work_handler_t1_t5(struct k_work *work)
{
	gpio_pin_set_dt(&pin_t1_t5, 0);
}

static void pulse_work_handler_t2_t6(struct k_work *work)
{
	gpio_pin_set_dt(&pin_t2_t6, 0);
}

static void pulse_work_handler_t3_t7(struct k_work *work)
{
	gpio_pin_set_dt(&pin_t3_t7, 0);
}

static void pulse_work_handler_t4_t8(struct k_work *work)
{
	gpio_pin_set_dt(&pin_t4_t8, 0);
}

int latency_measure_init(void)
{
	int ret;

	LOG_INF("Initializing latency measurement GPIO pins");

	/* Check if GPIO devices are ready */
	if (!gpio_is_ready_dt(&pin_t1_t5)) {
		LOG_ERR("GPIO device for pin T1/T5 (D2) not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&pin_t2_t6)) {
		LOG_ERR("GPIO device for pin T2/T6 (D3) not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&pin_t3_t7)) {
		LOG_ERR("GPIO device for pin T3/T7 (D4) not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&pin_t4_t8)) {
		LOG_ERR("GPIO device for pin T4/T8 (D5) not ready");
		return -ENODEV;
	}

	/* Configure GPIO pins as outputs */
	ret = gpio_pin_configure_dt(&pin_t1_t5, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Failed to configure GPIO pin T1/T5 (D2): %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&pin_t2_t6, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Failed to configure GPIO pin T2/T6 (D3): %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&pin_t3_t7, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Failed to configure GPIO pin T3/T7 (D4): %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&pin_t4_t8, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Failed to configure GPIO pin T4/T8 (D5): %d", ret);
		return ret;
	}

	/* Initialize work items */
	k_work_init_delayable(&pulse_work_t1_t5, pulse_work_handler_t1_t5);
	k_work_init_delayable(&pulse_work_t2_t6, pulse_work_handler_t2_t6);
	k_work_init_delayable(&pulse_work_t3_t7, pulse_work_handler_t3_t7);
	k_work_init_delayable(&pulse_work_t4_t8, pulse_work_handler_t4_t8);

	LOG_INF("Latency measurement GPIO pins initialized successfully");
	LOG_INF("Pin mapping:");
	LOG_INF("  T1/T5 (Audio Capture/Network RX): P0.26");
	LOG_INF("  T2/T6 (Encode Start/Decode Start): P0.25");
	LOG_INF("  T3/T7 (Encode Complete/Decode Complete): P0.07");
	LOG_INF("  T4/T8 (Network TX/Audio Output): P0.28");
	return 0;
}

/* Helper function to trigger a pin pulse */
static void trigger_pin_pulse(const struct gpio_dt_spec *pin, struct k_work_delayable *work)
{
	/* Set pin high */
	gpio_pin_set_dt(pin, 1);

	/* Cancel any pending work and schedule new work to turn off the pin */
	k_work_cancel_delayable(work);
	k_work_schedule(work, K_MSEC(PULSE_DURATION_US));
}

/* Gateway device timing functions */
void latency_measure_t1_audio_capture(void)
{
#if IS_ENABLED(CONFIG_AUDIO_GATEWAY)
	trigger_pin_pulse(&pin_t1_t5, &pulse_work_t1_t5);
#ifdef CONFIG_LATENCY_MEASUREMENT_DEBUG
	LOG_DBG("T1: Audio capture timing trigger");
#endif
#endif
}

void latency_measure_t2_encode_start(void)
{
#if IS_ENABLED(CONFIG_AUDIO_GATEWAY)
	trigger_pin_pulse(&pin_t2_t6, &pulse_work_t2_t6);
#endif
}

void latency_measure_t3_encode_complete(void)
{
#if IS_ENABLED(CONFIG_AUDIO_GATEWAY)
	trigger_pin_pulse(&pin_t3_t7, &pulse_work_t3_t7);
#endif
}

void latency_measure_t4_network_tx(void)
{
#if IS_ENABLED(CONFIG_AUDIO_GATEWAY)
	trigger_pin_pulse(&pin_t4_t8, &pulse_work_t4_t8);
#endif
}

/* Headset device timing functions */
void latency_measure_t5_network_rx(void)
{
#if IS_ENABLED(CONFIG_AUDIO_HEADSET)
	trigger_pin_pulse(&pin_t1_t5, &pulse_work_t1_t5);
#endif
}

void latency_measure_t6_decode_start(void)
{
#if IS_ENABLED(CONFIG_AUDIO_HEADSET)
	trigger_pin_pulse(&pin_t2_t6, &pulse_work_t2_t6);
#endif
}

void latency_measure_t7_decode_complete(void)
{
#if IS_ENABLED(CONFIG_AUDIO_HEADSET)
	trigger_pin_pulse(&pin_t3_t7, &pulse_work_t3_t7);
#endif
}

void latency_measure_t8_audio_output(void)
{
#if IS_ENABLED(CONFIG_AUDIO_HEADSET)
	trigger_pin_pulse(&pin_t4_t8, &pulse_work_t4_t8);
#ifdef CONFIG_LATENCY_MEASUREMENT_DEBUG
	LOG_DBG("T8: Audio output timing trigger");
#endif
#endif
}

#endif /* CONFIG_LATENCY_MEASUREMENT */
