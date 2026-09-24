/*******************************************************************
 * @file zbook_gpio.h
 *
 * @brief Defines the interface for zbook generic digital GPIO channels.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 23/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#ifndef ZBOOK_GPIO_H
#define ZBOOK_GPIO_H

#include <stdbool.h>

/**
 * @brief One of zbook's GPIO-capable external header IOs.
 *
 */
enum zbook_gpio_io {
	ZBOOK_GPIO_IO01,
	ZBOOK_GPIO_IO39,
	ZBOOK_GPIO_IO45,
	ZBOOK_GPIO_IO46,
};

/** @brief Pin direction for a zbook GPIO channel. */
enum zbook_gpio_direction {
	ZBOOK_GPIO_DIR_INPUT,
	ZBOOK_GPIO_DIR_OUTPUT,
};

/** @brief Pull configuration for a zbook GPIO channel. */
enum zbook_gpio_pull {
	ZBOOK_GPIO_PULL_NONE,
	ZBOOK_GPIO_PULL_UP,
	ZBOOK_GPIO_PULL_DOWN,
};

/** @brief Interrupt trigger edge for a zbook GPIO channel. */
enum zbook_gpio_edge {
	ZBOOK_GPIO_EDGE_RISING,
	ZBOOK_GPIO_EDGE_FALLING,
	ZBOOK_GPIO_EDGE_BOTH,
};

/** @brief Which physical level counts as logical 1 for a zbook GPIO channel. */
enum zbook_gpio_active {
	ZBOOK_GPIO_ACTIVE_HIGH,
	ZBOOK_GPIO_ACTIVE_LOW,
};

/** @brief Runtime-configurable zbook GPIO channel parameters. */
struct zbook_gpio_cfg {
	enum zbook_gpio_direction direction;
	enum zbook_gpio_pull pull;
	enum zbook_gpio_active active;
};

/**
 * @brief Signature for a zbook GPIO edge callback.
 *
 * Called from interrupt context.
 *
 * @param io        The IO that triggered.
 * @param user_data Whatever was passed to zbook_gpio_set_callback().
 */
typedef void (*zbook_gpio_callback_t)(enum zbook_gpio_io io, void *user_data);

/**
 * @brief Claim @p io and get it ready to use as a GPIO.
 *
 * Leaves the channel as an input, no pull, active-high, on success.
 *
 * @param io One of zbook's GPIO-capable header IOs.
 * @retval 0 Success.
 * @retval -ENODEV @p io isn't wired up as GPIO in this build, or its
 *                 underlying device isn't ready.
 */
int zbook_gpio_init(enum zbook_gpio_io io);

/**
 * @brief Apply @p cfg to @p io.
 *
 * @param io  One of zbook's GPIO-capable header IOs.
 * @param cfg Desired configuration. All fields are required.
 * @retval 0 Success.
 * @retval -EINVAL @p cfg is NULL, or one of its fields is out of range.
 * @retval -ENODEV @p io isn't wired up, or zbook_gpio_init() hasn't been
 *                 called successfully for it yet.
 */
int zbook_gpio_configure(enum zbook_gpio_io io, const struct zbook_gpio_cfg *cfg);

/**
 * @brief Drive @p io to @p value.
 *
 * @param io    One of zbook's GPIO-capable header IOs.
 * @param value Logical level to drive (active/inactive, not raw electrical).
 * @retval 0 Success.
 * @retval -EACCES @p io isn't configured as an output.
 * @retval -ENODEV @p io isn't wired up, or zbook_gpio_init() hasn't been
 *                 called successfully for it yet.
 */
int zbook_gpio_write(enum zbook_gpio_io io, bool value);

/**
 * @brief Read the current logical level of @p io.
 *
 * @param io    One of zbook's GPIO-capable header IOs.
 * @param value Destination for the current level.
 * @retval 0 Success.
 * @retval -EINVAL @p value is NULL.
 * @retval -EACCES @p io isn't configured as an input.
 * @retval -ENODEV @p io isn't wired up, or zbook_gpio_init() hasn't been
 *                 called successfully for it yet.
 */
int zbook_gpio_read(enum zbook_gpio_io io, bool *value);

/**
 * @brief Register @p cb to run on every @p edge transition of @p io.
 *
 * Replaces any callback previously registered for @p io. Debounced by the
 * "debounce-interval-ms" devicetree property on the "zbook_gpio_channels"
 * node -- not a runtime parameter, same convention as "gpio-keys".
 *
 * @param io        One of zbook's GPIO-capable header IOs.
 * @param edge      Trigger edge.
 * @param cb        Callback to run.
 * @param user_data Passed through to @p cb.
 * @retval 0 Success.
 * @retval -EINVAL @p cb is NULL, or @p edge is out of range.
 * @retval -EACCES @p io isn't configured as an input.
 * @retval -ENODEV @p io isn't wired up, or zbook_gpio_init() hasn't been
 *                 called successfully for it yet.
 */
int zbook_gpio_set_callback(enum zbook_gpio_io io, enum zbook_gpio_edge edge,
			    zbook_gpio_callback_t cb, void *user_data);

/**
 * @brief Unregister @p io's callback, if any.
 *
 * @param io One of zbook's GPIO-capable header IOs.
 * @retval 0 Success (idempotent -- also returned if nothing was registered).
 * @retval -ENODEV @p io isn't wired up, or zbook_gpio_init() hasn't been
 *                 called successfully for it yet.
 */
int zbook_gpio_remove_callback(enum zbook_gpio_io io);

#endif /* ZBOOK_GPIO_H */
