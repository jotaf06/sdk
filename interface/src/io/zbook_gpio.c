/*******************************************************************
 * @file zbook_gpio.c
 *
 * @brief Implements the interface for zbook generic digital GPIO channels.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 23/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include "io/zbook_gpio.h"

#ifdef CONFIG_ZBOOK_GPIO

#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zbook_gpio, CONFIG_ZBOOK_GPIO_LOG_LEVEL);

#define ZBOOK_GPIO_CHANNEL_ENTRY(node_id)                                                          \
	{                                                                                          \
		.spec = GPIO_DT_SPEC_GET(node_id, gpios),                                          \
	},

#if DT_NODE_EXISTS(DT_NODELABEL(zbook_gpio_channels))
#define ZBOOK_GPIO_DEBOUNCE_MS DT_PROP(DT_NODELABEL(zbook_gpio_channels), debounce_interval_ms)
#else
#define ZBOOK_GPIO_DEBOUNCE_MS 0
#endif

struct zbook_gpio_channel {
	struct gpio_dt_spec spec;
	bool ready;
	enum zbook_gpio_direction direction;
	struct gpio_callback gpio_cb;
	zbook_gpio_callback_t user_cb;
	void *user_data;
	gpio_flags_t int_flags;
	struct k_work_delayable debounce_work;
};

#if DT_NODE_EXISTS(DT_NODELABEL(zbook_gpio_channels))

static struct zbook_gpio_channel channels[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(zbook_gpio_channels), ZBOOK_GPIO_CHANNEL_ENTRY)};

#else

static struct zbook_gpio_channel channels[] = {};

#endif

static struct zbook_gpio_channel *find_channel(enum zbook_gpio_io io)
{
	if ((size_t)io >= ARRAY_SIZE(channels)) {
		return NULL;
	}

	return &channels[io];
}

static void disable_callback(struct zbook_gpio_channel *ch)
{
	if (ch->user_cb == NULL) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&ch->spec, GPIO_INT_DISABLE);
	k_work_cancel_delayable(&ch->debounce_work);
	gpio_remove_callback(ch->spec.port, &ch->gpio_cb);
	ch->user_cb = NULL;
	ch->user_data = NULL;
}

static void zbook_gpio_debounce_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct zbook_gpio_channel *ch =
		CONTAINER_OF(dwork, struct zbook_gpio_channel, debounce_work);

	ch->user_cb((enum zbook_gpio_io)(ch - channels), ch->user_data);

	gpio_pin_interrupt_configure_dt(&ch->spec, ch->int_flags);
}

static void zbook_gpio_isr(const struct device *port, struct gpio_callback *cb,
			   gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct zbook_gpio_channel *ch = CONTAINER_OF(cb, struct zbook_gpio_channel, gpio_cb);

	if (ZBOOK_GPIO_DEBOUNCE_MS == 0) {
		ch->user_cb((enum zbook_gpio_io)(ch - channels), ch->user_data);
		return;
	}

	gpio_pin_interrupt_configure_dt(&ch->spec, GPIO_INT_DISABLE);
	k_work_reschedule(&ch->debounce_work, K_MSEC(ZBOOK_GPIO_DEBOUNCE_MS));
}

int zbook_gpio_init(enum zbook_gpio_io io)
{
	struct zbook_gpio_channel *ch = find_channel(io);

	if (ch == NULL) {
		LOG_ERR("zbook_gpio io %d not wired up in this build", io);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&ch->spec)) {
		LOG_ERR("zbook_gpio io %d device not ready", io);
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&ch->spec, GPIO_INPUT);

	if (ret < 0) {
		LOG_ERR("zbook_gpio io %d configure failed (%d)", io, ret);
		return ret;
	}

	ch->ready = true;
	ch->direction = ZBOOK_GPIO_DIR_INPUT;
	ch->user_cb = NULL;
	ch->user_data = NULL;

	LOG_DBG("zbook_gpio io %d initialized", io);

	return 0;
}

int zbook_gpio_configure(enum zbook_gpio_io io, const struct zbook_gpio_cfg *cfg)
{
	if (cfg == NULL || cfg->direction > ZBOOK_GPIO_DIR_OUTPUT ||
	    cfg->pull > ZBOOK_GPIO_PULL_DOWN || cfg->active > ZBOOK_GPIO_ACTIVE_LOW) {
		LOG_ERR("invalid zbook_gpio config for io %d", io);
		return -EINVAL;
	}

	struct zbook_gpio_channel *ch = find_channel(io);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_gpio io %d not initialized", io);
		return -ENODEV;
	}

	gpio_flags_t flags = cfg->direction == ZBOOK_GPIO_DIR_OUTPUT ? GPIO_OUTPUT : GPIO_INPUT;

	if (cfg->pull == ZBOOK_GPIO_PULL_UP) {
		flags |= GPIO_PULL_UP;
	} else if (cfg->pull == ZBOOK_GPIO_PULL_DOWN) {
		flags |= GPIO_PULL_DOWN;
	}

	if (cfg->active == ZBOOK_GPIO_ACTIVE_LOW) {
		flags |= GPIO_ACTIVE_LOW;
	}

	int ret = gpio_pin_configure_dt(&ch->spec, flags);

	if (ret < 0) {
		LOG_ERR("zbook_gpio io %d configure failed (%d)", io, ret);
		return ret;
	}

	if (cfg->direction != ZBOOK_GPIO_DIR_INPUT) {
		disable_callback(ch);
	}

	ch->direction = cfg->direction;

	return 0;
}

int zbook_gpio_write(enum zbook_gpio_io io, bool value)
{
	struct zbook_gpio_channel *ch = find_channel(io);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_gpio io %d not initialized", io);
		return -ENODEV;
	}

	if (ch->direction != ZBOOK_GPIO_DIR_OUTPUT) {
		LOG_ERR("zbook_gpio io %d not configured as output", io);
		return -EACCES;
	}

	return gpio_pin_set_dt(&ch->spec, value);
}

int zbook_gpio_read(enum zbook_gpio_io io, bool *value)
{
	if (value == NULL) {
		LOG_ERR("NULL zbook_gpio value output for io %d", io);
		return -EINVAL;
	}

	struct zbook_gpio_channel *ch = find_channel(io);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_gpio io %d not initialized", io);
		return -ENODEV;
	}

	if (ch->direction != ZBOOK_GPIO_DIR_INPUT) {
		LOG_ERR("zbook_gpio io %d not configured as input", io);
		return -EACCES;
	}

	int ret = gpio_pin_get_dt(&ch->spec);

	if (ret < 0) {
		return ret;
	}

	*value = (bool)ret;

	return 0;
}

int zbook_gpio_set_callback(enum zbook_gpio_io io, enum zbook_gpio_edge edge,
			    zbook_gpio_callback_t cb, void *user_data)
{
	if (cb == NULL || edge > ZBOOK_GPIO_EDGE_BOTH) {
		LOG_ERR("invalid zbook_gpio callback args for io %d", io);
		return -EINVAL;
	}

	struct zbook_gpio_channel *ch = find_channel(io);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_gpio io %d not initialized", io);
		return -ENODEV;
	}

	if (ch->direction != ZBOOK_GPIO_DIR_INPUT) {
		LOG_ERR("zbook_gpio io %d not configured as input", io);
		return -EACCES;
	}

	static const gpio_flags_t edge_flags[] = {
		[ZBOOK_GPIO_EDGE_RISING] = GPIO_INT_EDGE_RISING,
		[ZBOOK_GPIO_EDGE_FALLING] = GPIO_INT_EDGE_FALLING,
		[ZBOOK_GPIO_EDGE_BOTH] = GPIO_INT_EDGE_BOTH,
	};

	disable_callback(ch);

	ch->user_cb = cb;
	ch->user_data = user_data;
	ch->int_flags = edge_flags[edge];
	k_work_init_delayable(&ch->debounce_work, zbook_gpio_debounce_handler);
	gpio_init_callback(&ch->gpio_cb, zbook_gpio_isr, BIT(ch->spec.pin));

	int ret = gpio_add_callback(ch->spec.port, &ch->gpio_cb);

	if (ret < 0) {
		ch->user_cb = NULL;
		ch->user_data = NULL;
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&ch->spec, ch->int_flags);
	if (ret < 0) {
		gpio_remove_callback(ch->spec.port, &ch->gpio_cb);
		ch->user_cb = NULL;
		ch->user_data = NULL;
		return ret;
	}

	return 0;
}

int zbook_gpio_remove_callback(enum zbook_gpio_io io)
{
	struct zbook_gpio_channel *ch = find_channel(io);

	if (ch == NULL || !ch->ready) {
		LOG_ERR("zbook_gpio io %d not initialized", io);
		return -ENODEV;
	}

	disable_callback(ch);

	return 0;
}

#endif /* CONFIG_ZBOOK_GPIO */
