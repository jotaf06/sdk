/*******************************************************************
 * @file zbook_pwm.h
 *
 * @brief Defines the interface for zbook generic PWM output channels.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 16/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#ifndef ZBOOK_PWM_H
#define ZBOOK_PWM_H

#include <stdint.h>

/** @brief Output polarity for a zbook PWM channel. */
enum zbook_pwm_polarity {
	ZBOOK_PWM_POLARITY_NORMAL = 0, /**< Duty cycle is time spent high. */
	ZBOOK_PWM_POLARITY_INVERTED,   /**< Duty cycle is time spent low. */
};

/** @brief Runtime-configurable zbook PWM channel parameters. */
struct zbook_pwm_cfg {
	uint32_t frequency_hz;            /**< PWM frequency in Hz. Must be nonzero. */
	uint8_t duty_cycle_percent;       /**< Duty cycle, 0 to 100. */
	enum zbook_pwm_polarity polarity; /**< Output polarity. */
};

/**
 * @brief Check that the zbook PWM channel @p label is ready to use.
 *
 * @p label is not a pin number or a compile-time constant: every channel is
 * a child node of the "zbook_pwm_channels" node (`compatible =
 * "pwm-leds"`) in the active board/application overlay, identified by its
 * `label` property, e.g.:
 *
 * @code{.dts}
 * / {
 *         zbook_pwm_channels: zbook-pwm-channels {
 *                 compatible = "pwm-leds";
 *
 *                 my_dimmer: my-dimmer {
 *                         label = "dimmer";
 *                         pwms = <&pwm 1 PWM_USEC(20000) PWM_POLARITY_NORMAL>;
 *                 };
 *         };
 * };
 * @endcode
 *
 * Any number of channels, on any pin, can be added by a consumer just by
 * adding more children to its own "zbook_pwm_channels" node.
 *
 * Leaves the channel silent (0% duty) on success.
 *
 * @param label Channel label, as set by the `label` property of the
 *              matching child of "zbook_pwm_channels".
 * @retval 0 Success.
 * @retval -EINVAL @p label is NULL or empty.
 * @retval -ENODEV No channel with this label exists in the active overlay,
 *                 or its underlying PWM device is not ready.
 */
int zbook_pwm_init(const char *label);

/**
 * @brief Start channel @p label at its currently configured
 * frequency/duty cycle/polarity.
 *
 * @param label Channel label.
 * @retval 0 Success.
 * @retval -EINVAL @p label is NULL or empty.
 * @retval -ENODEV Unknown @p label, or zbook_pwm_init() has not been called
 *                 successfully for it yet.
 */
int zbook_pwm_start(const char *label);

/**
 * @brief Stop channel @p label.
 *
 *
 * @param label Channel label.
 * @retval 0 Success.
 * @retval -EINVAL @p label is NULL or empty.
 * @retval -ENODEV Unknown @p label, or zbook_pwm_init() has not been called
 *                 successfully for it yet.
 */
int zbook_pwm_stop(const char *label);

/**
 * @brief Apply @p cfg to channel @p label.
 *
 *
 * @param label Channel label.
 * @param cfg   Desired configuration. All fields are required.
 * @retval 0 Success.
 * @retval -EINVAL @p label is NULL or empty, @p cfg is NULL,
 *                 @p cfg->frequency_hz is 0, or @p cfg->duty_cycle_percent
 *                 is greater than 100.
 * @retval -ENODEV Unknown @p label, or zbook_pwm_init() has not been called
 *                 successfully for it yet.
 */
int zbook_pwm_set_cfg(const char *label, const struct zbook_pwm_cfg *cfg);

/**
 * @brief Read back the configuration last applied to channel @p label.
 *
 *
 * @param label Channel label.
 * @param cfg   Destination for the current configuration.
 * @retval 0 Success.
 * @retval -EINVAL @p label is NULL or empty, or @p cfg is NULL.
 * @retval -ENODEV Unknown @p label, or zbook_pwm_init() has not been called
 *                 successfully for it yet.
 */
int zbook_pwm_get_cfg(const char *label, struct zbook_pwm_cfg *cfg);

#endif /* ZBOOK_PWM_H */
