/*******************************************************************
 * @file zbook_pwm.c
 *
 * @brief Implements the interface for zbook generic PWM output channels.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 16/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include "actuators/zbook_pwm.h"

#ifdef CONFIG_ZBOOK_PWM

#include <errno.h>
#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zbook_pwm, CONFIG_ZBOOK_PWM_LOG_LEVEL);

#define ZBOOK_PWM_CHECK_LABEL(label)                                                               \
	do {                                                                                       \
		if ((label) == NULL || (label)[0] == '\0') {                                       \
			LOG_ERR("NULL or empty channel label");                                    \
			return -EINVAL;                                                            \
		}                                                                                  \
	} while (0)

/* One entry per child of the well-known "zbook_pwm_channels" node (a plain
 * "pwm-leds" node, reusing Zephyr's own binding instead of a custom one).
 * Count and pin assignment come entirely from devicetree -- nothing about
 * which or how many pins can be used is hardcoded here.
 */
#define ZBOOK_PWM_CHANNEL_ENTRY(node_id)                                                           \
	{                                                                                          \
		.label = DT_PROP(node_id, label),                                                  \
		.pwm = PWM_DT_SPEC_GET(node_id),                                                   \
	},

struct zbook_pwm_channel {
	const char *label;
	struct pwm_dt_spec pwm;
	bool ready;
	bool playing;
	struct zbook_pwm_cfg cfg;
};

#if DT_NODE_EXISTS(DT_NODELABEL(zbook_pwm_channels))

static struct zbook_pwm_channel channels[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(zbook_pwm_channels), ZBOOK_PWM_CHANNEL_ENTRY)};

#else

static struct zbook_pwm_channel channels[] = {};

#endif

static struct zbook_pwm_channel *find_channel(const char *label)
{
	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		if (strcmp(channels[i].label, label) == 0) {
			return &channels[i];
		}
	}

	return NULL;
}

static int apply(const struct zbook_pwm_channel *ch)
{
	uint32_t period_ns = NSEC_PER_SEC / ch->cfg.frequency_hz;
	uint32_t pulse_ns =
		ch->playing ? ((uint64_t)period_ns * ch->cfg.duty_cycle_percent) / 100U : 0U;
	pwm_flags_t flags = ch->cfg.polarity == ZBOOK_PWM_POLARITY_INVERTED ? PWM_POLARITY_INVERTED
									    : PWM_POLARITY_NORMAL;

	return pwm_set(ch->pwm.dev, ch->pwm.channel, period_ns, pulse_ns, flags);
}

int zbook_pwm_init(const char *label)
{
	ZBOOK_PWM_CHECK_LABEL(label);

	struct zbook_pwm_channel *ch = find_channel(label);

	if (ch == NULL) {
		LOG_ERR("no zbook_pwm_channels child with label \"%s\"", label);
		return -ENODEV;
	}

	if (!pwm_is_ready_dt(&ch->pwm)) {
		LOG_ERR("zbook_pwm \"%s\" device not ready", label);
		return -ENODEV;
	}

	ch->ready = true;
	ch->playing = false;
	ch->cfg = (struct zbook_pwm_cfg){
		.frequency_hz = NSEC_PER_SEC / ch->pwm.period,
		.duty_cycle_percent = 0,
		.polarity = ZBOOK_PWM_POLARITY_NORMAL,
	};

	LOG_DBG("zbook_pwm \"%s\" initialized", label);

	return apply(ch);
}

int zbook_pwm_start(const char *label)
{
	ZBOOK_PWM_CHECK_LABEL(label);

	struct zbook_pwm_channel *ch = find_channel(label);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_pwm \"%s\" not initialized", label);
		return -ENODEV;
	}

	ch->playing = true;

	return apply(ch);
}

int zbook_pwm_stop(const char *label)
{
	ZBOOK_PWM_CHECK_LABEL(label);

	struct zbook_pwm_channel *ch = find_channel(label);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_pwm \"%s\" not initialized", label);
		return -ENODEV;
	}

	ch->playing = false;

	return apply(ch);
}

int zbook_pwm_set_cfg(const char *label, const struct zbook_pwm_cfg *cfg)
{
	ZBOOK_PWM_CHECK_LABEL(label);

	if (cfg == NULL || cfg->frequency_hz == 0 || cfg->duty_cycle_percent > 100) {
		LOG_ERR("invalid zbook_pwm config for \"%s\"", label);
		return -EINVAL;
	}

	struct zbook_pwm_channel *ch = find_channel(label);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_pwm \"%s\" not initialized", label);
		return -ENODEV;
	}

	ch->cfg = *cfg;

	return apply(ch);
}

int zbook_pwm_get_cfg(const char *label, struct zbook_pwm_cfg *cfg)
{
	ZBOOK_PWM_CHECK_LABEL(label);

	if (cfg == NULL) {
		LOG_ERR("NULL zbook_pwm cfg output for \"%s\"", label);
		return -EINVAL;
	}

	struct zbook_pwm_channel *ch = find_channel(label);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_pwm \"%s\" not initialized", label);
		return -ENODEV;
	}

	*cfg = ch->cfg;

	return 0;
}

#endif /* CONFIG_ZBOOK_PWM */
