/*******************************************************************
 * @file zbook_buzzer.h
 *
 * @brief Defines the interface for the zbook piezo buzzer.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 15/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#ifndef ZBOOK_BUZZER_H
#define ZBOOK_BUZZER_H

#include <stdint.h>

/** @brief Preset beep durations, in CONFIG_ZBOOK_BUZZER_BEEP_<name>_MS. */
enum zbook_buzzer_beep_duration {
	ZBOOK_BUZZER_BEEP_DURATION_SHORT,  /**< Short beep duration */
	ZBOOK_BUZZER_BEEP_DURATION_NORMAL, /**< Normal beep duration */
	ZBOOK_BUZZER_BEEP_DURATION_LONG,   /**< Long beep duration */
};

/**
 * @brief Check that the zbook buzzer is ready to use.
 *
 * Leaves the buzzer silent on success.
 *
 * @retval 0 Success.
 * @retval -ENODEV device is not ready.
 */
int zbook_buzzer_init(void);

/**
 * @brief Turn the zbook buzzer on, at the currently configured frequency and
 * volume.
 *
 * @retval 0 Success.
 * @retval -ENODEV zbook_buzzer_init() has not been called successfully yet.
 */
int zbook_buzzer_on(void);

/**
 * @brief Turn the zbook buzzer off.
 *
 * @retval 0 Success.
 * @retval -ENODEV zbook_buzzer_init() has not been called successfully yet.
 */
int zbook_buzzer_off(void);

/**
 * @brief Set the zbook buzzer's volume via PWM duty cycle.
 *
 * @param percent Volume, 0 (silent) to 100 (loudest).
 *
 * @retval 0 Success.
 * @retval -ENODEV zbook_buzzer_init() has not been called successfully yet.
 * @retval -EINVAL @p percent is greater than 100.
 */
int zbook_buzzer_set_volume(uint8_t percent);

/**
 * @brief Set the zbook buzzer's tone.
 *
 * @param frequency_hz Tone frequency in Hz. Must be nonzero.
 *
 * @retval 0 Success.
 * @retval -ENODEV zbook_buzzer_init() has not been called successfully yet.
 * @retval -EINVAL @p frequency_hz is zero.
 */
int zbook_buzzer_set_frequency(uint32_t frequency_hz);

/**
 * @brief Turn the zbook buzzer on for @p duration_ms, then off.
 *
 * Blocks for the duration of the beep.
 *
 * @param duration_ms How long to keep the buzzer on, in milliseconds.
 *
 * @retval 0 Success.
 * @retval -ENODEV zbook_buzzer_init() has not been called successfully yet.
 */
int zbook_buzzer_beep_ms(uint32_t duration_ms);

/**
 * @brief Beep @p times times, at the CONFIG_ZBOOK_BUZZER_BEEP_NORMAL_MS
 * duration.
 *
 * Blocks for the duration of the whole sequence -- see zbook_buzzer_beep_full().
 *
 * @param times How many times to beep. Must be nonzero.
 *
 * @retval 0 Success.
 * @retval -ENODEV zbook_buzzer_init() has not been called successfully yet.
 * @retval -EINVAL @p times is zero.
 */
int zbook_buzzer_beep_repeated(uint32_t times);

/**
 * @brief Beep once, at a preset duration.
 *
 * @param duration Which preset duration to beep for.
 *
 * @retval 0 Success.
 * @retval -ENODEV zbook_buzzer_init() has not been called successfully yet.
 * @retval -EINVAL @p duration is not a valid enum zbook_buzzer_beep_duration.
 */
int zbook_buzzer_beep(enum zbook_buzzer_beep_duration duration);

/**
 * @brief Beep @p times times, each for @p duration_ms.
 *
 * Blocks for the duration of the whole sequence: each beep is followed by a
 * @p duration_ms silent gap before the next one, except the last.
 *
 * @param duration_ms How long each beep lasts, in milliseconds.
 * @param times How many times to beep. Must be nonzero.
 *
 * @retval 0 Success.
 * @retval -ENODEV zbook_buzzer_init() has not been called successfully yet.
 * @retval -EINVAL @p times is zero.
 */
int zbook_buzzer_beep_full(uint32_t duration_ms, uint32_t times);

#endif /* ZBOOK_BUZZER_H */
