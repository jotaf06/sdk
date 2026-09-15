/*******************************************************************
 * @file zbook_buzzer.c
 *
 * @brief Implements the interface for the zbook piezo buzzer.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 15/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include "actuators/zbook_buzzer.h"

#ifdef CONFIG_ZBOOK_BUZZER

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/clock.h>

LOG_MODULE_REGISTER(zbook_buzzer, CONFIG_ZBOOK_BUZZER_LOG_LEVEL);

#define BUZZER_NODE                 DT_ALIAS(pwm_buzzer)
#define BUZZER_DEFAULT_FREQUENCY_HZ (NSEC_PER_SEC / DT_PWMS_PERIOD(BUZZER_NODE))
#define BUZZER_MAX_DUTY_PERCENT     50U

static const struct pwm_dt_spec buzzer_pwm = PWM_DT_SPEC_GET(BUZZER_NODE);

struct {
	bool buzzer_ready;
	bool buzzer_playing;
	uint32_t buzzer_frequency_hz;
	uint8_t buzzer_volume_percent;
} self = {
	.buzzer_frequency_hz = BUZZER_DEFAULT_FREQUENCY_HZ,
	.buzzer_volume_percent = 100U,
};

static int buzzer_apply(void)
{
	uint32_t period_ns = NSEC_PER_SEC / self.buzzer_frequency_hz;
	uint32_t duty_percent =
		self.buzzer_playing ? (self.buzzer_volume_percent * BUZZER_MAX_DUTY_PERCENT) / 100U : 0U;
	uint32_t pulse_ns = ((uint64_t)period_ns * duty_percent) / 100U;

	return pwm_set(buzzer_pwm.dev, buzzer_pwm.channel, period_ns, pulse_ns, buzzer_pwm.flags);
}

int zbook_buzzer_init(void)
{
	if (!pwm_is_ready_dt(&buzzer_pwm)) {
		LOG_ERR("zbook_buzzer PWM device not ready");
		return -ENODEV;
	}

	self.buzzer_ready = true;
	self.buzzer_playing = false;
	LOG_DBG("zbook_buzzer initialized");

	return buzzer_apply();
}

int zbook_buzzer_on(void)
{
	if (!self.buzzer_ready) {
		LOG_ERR("zbook_buzzer not initialized");
		return -ENODEV;
	}

	self.buzzer_playing = true;

	return buzzer_apply();
}

int zbook_buzzer_off(void)
{
	if (!self.buzzer_ready) {
		LOG_ERR("zbook_buzzer not initialized");
		return -ENODEV;
	}

	self.buzzer_playing = false;

	return buzzer_apply();
}

int zbook_buzzer_set_volume(uint8_t percent)
{
	if (!self.buzzer_ready) {
		LOG_ERR("zbook_buzzer not initialized");
		return -ENODEV;
	}

	if (percent > 100U) {
		LOG_ERR("invalid zbook_buzzer volume: %u", percent);
		return -EINVAL;
	}

	self.buzzer_volume_percent = percent;

	return buzzer_apply();
}

int zbook_buzzer_set_frequency(uint32_t frequency_hz)
{
	if (!self.buzzer_ready) {
		LOG_ERR("zbook_buzzer not initialized");
		return -ENODEV;
	}

	if (frequency_hz == 0U) {
		LOG_ERR("invalid zbook_buzzer frequency: 0");
		return -EINVAL;
	}

	self.buzzer_frequency_hz = frequency_hz;

	return buzzer_apply();
}

int zbook_buzzer_beep_ms(uint32_t duration_ms)
{
	int ret = zbook_buzzer_on();

	if (ret < 0) {
		return ret;
	}

	k_msleep(duration_ms);

	return zbook_buzzer_off();
}

int zbook_buzzer_beep_repeated(uint32_t times)
{
	return zbook_buzzer_beep_full(CONFIG_ZBOOK_BUZZER_BEEP_DURATION_NORMAL_MS, times);
}

int zbook_buzzer_beep(enum zbook_buzzer_beep_duration duration)
{
	uint32_t duration_ms;

	switch (duration) {
	case ZBOOK_BUZZER_BEEP_DURATION_SHORT:
		duration_ms = CONFIG_ZBOOK_BUZZER_BEEP_DURATION_SHORT_MS;
		break;
	case ZBOOK_BUZZER_BEEP_DURATION_NORMAL:
		duration_ms = CONFIG_ZBOOK_BUZZER_BEEP_DURATION_NORMAL_MS;
		break;
	case ZBOOK_BUZZER_BEEP_DURATION_LONG:
		duration_ms = CONFIG_ZBOOK_BUZZER_BEEP_DURATION_LONG_MS;
		break;
	default:
		LOG_ERR("invalid zbook_buzzer beep duration: %d", duration);
		return -EINVAL;
	}

	return zbook_buzzer_beep_full(duration_ms, 1U);
}

int zbook_buzzer_beep_full(uint32_t duration_ms, uint32_t times)
{
	if (times == 0U) {
		LOG_ERR("invalid zbook_buzzer beep repeat count: 0");
		return -EINVAL;
	}

	for (uint32_t i = 0; i < times; i++) {
		int ret = zbook_buzzer_beep_ms(duration_ms);

		if (ret < 0) {
			return ret;
		}

		if (i + 1U < times) {
			k_msleep(duration_ms);
		}
	}

	return 0;
}

#endif /* CONFIG_ZBOOK_BUZZER */
