/*******************************************************************
 * @file main.c
 *
 * @brief Real-hardware bring-up sample for the zbook buzzer actuator:
 * initializes the buzzer once at boot, then exposes all zbook_buzzer API
 * functions as shell commands for manual testing.
 *
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 15/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "actuators/zbook_buzzer.h"

#define NOTE_C4  262U
#define NOTE_D4  294U
#define NOTE_E4  330U
#define NOTE_F4  349U
#define NOTE_G4  392U
#define NOTE_A4  440U
#define NOTE_AS4 466U
#define NOTE_C5  523U

#define BUZZER_MUSIC_TEMPO_BPM     120U
#define BUZZER_MUSIC_WHOLE_NOTE_MS ((60000U / BUZZER_MUSIC_TEMPO_BPM) * 4U)

struct buzzer_note {
	uint32_t frequency_hz;
	uint8_t duration_divisor;
};

/* "Happy Birthday to You", in C. */
static const struct buzzer_note happy_birthday[] = {
	{NOTE_C4, 4},  {NOTE_C4, 8}, {NOTE_D4, 4}, {NOTE_C4, 4}, {NOTE_F4, 4},
	{NOTE_E4, 2},  {NOTE_C4, 4}, {NOTE_C4, 8}, {NOTE_D4, 4}, {NOTE_C4, 4},
	{NOTE_G4, 4},  {NOTE_F4, 2}, {NOTE_C4, 4}, {NOTE_C4, 8}, {NOTE_C5, 4},
	{NOTE_A4, 4},  {NOTE_F4, 4}, {NOTE_E4, 4}, {NOTE_D4, 4}, {NOTE_AS4, 4},
	{NOTE_AS4, 8}, {NOTE_A4, 4}, {NOTE_F4, 4}, {NOTE_G4, 4}, {NOTE_F4, 2},
};

static int cmd_buzzer_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = zbook_buzzer_on();

	if (ret != 0) {
		shell_error(sh, "zbook_buzzer_on failed: %d", ret);
		return ret;
	}

	shell_print(sh, "buzzer on");

	return 0;
}

static int cmd_buzzer_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = zbook_buzzer_off();

	if (ret != 0) {
		shell_error(sh, "zbook_buzzer_off failed: %d", ret);
		return ret;
	}

	shell_print(sh, "buzzer off");

	return 0;
}

static int cmd_buzzer_volume(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int err = 0;
	unsigned long percent = shell_strtoul(argv[1], 10, &err);

	if (err != 0 || percent > 100) {
		shell_error(sh, "usage: buzzer volume <0-100>");
		return -EINVAL;
	}

	int ret = zbook_buzzer_set_volume((uint8_t)percent);

	if (ret != 0) {
		shell_error(sh, "zbook_buzzer_set_volume failed: %d", ret);
		return ret;
	}

	shell_print(sh, "buzzer volume set to %lu%%", percent);

	return 0;
}

static int cmd_buzzer_frequency(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int err = 0;
	unsigned long frequency_hz = shell_strtoul(argv[1], 10, &err);

	if (err != 0) {
		shell_error(sh, "usage: buzzer frequency <hz>");
		return -EINVAL;
	}

	int ret = zbook_buzzer_set_frequency((uint32_t)frequency_hz);

	if (ret != 0) {
		shell_error(sh, "zbook_buzzer_set_frequency failed: %d", ret);
		return ret;
	}

	shell_print(sh, "buzzer frequency set to %lu Hz", frequency_hz);

	return 0;
}

static int cmd_buzzer_beep(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int err = 0;
	unsigned long duration_ms = shell_strtoul(argv[1], 10, &err);

	if (err != 0) {
		shell_error(sh, "usage: buzzer beep <duration_ms>");
		return -EINVAL;
	}

	shell_print(sh, "beeping for %lu ms...", duration_ms);

	int ret = zbook_buzzer_beep_ms((uint32_t)duration_ms);

	if (ret != 0) {
		shell_error(sh, "zbook_buzzer_beep_ms failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_buzzer_beep_repeated(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int err = 0;
	unsigned long times = shell_strtoul(argv[1], 10, &err);

	if (err != 0) {
		shell_error(sh, "usage: buzzer beep_repeated <times>");
		return -EINVAL;
	}

	shell_print(sh, "beeping %lu times...", times);

	int ret = zbook_buzzer_beep_repeated((uint32_t)times);

	if (ret != 0) {
		shell_error(sh, "zbook_buzzer_beep_repeated failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_buzzer_beep_preset(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_buzzer_beep_duration length;

	if (strcmp(argv[1], "short") == 0) {
		length = ZBOOK_BUZZER_BEEP_DURATION_SHORT;
	} else if (strcmp(argv[1], "normal") == 0) {
		length = ZBOOK_BUZZER_BEEP_DURATION_NORMAL;
	} else if (strcmp(argv[1], "long") == 0) {
		length = ZBOOK_BUZZER_BEEP_DURATION_LONG;
	} else {
		shell_error(sh, "usage: buzzer beep_preset <short|normal|long>");
		return -EINVAL;
	}

	int ret = zbook_buzzer_beep(length);

	if (ret != 0) {
		shell_error(sh, "zbook_buzzer_beep failed: %d", ret);
		return ret;
	}

	shell_print(sh, "beeped (%s)", argv[1]);

	return 0;
}

static int cmd_buzzer_beep_full(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int err = 0;
	unsigned long duration_ms = shell_strtoul(argv[1], 10, &err);
	unsigned long times = shell_strtoul(argv[2], 10, &err);

	if (err != 0) {
		shell_error(sh, "usage: buzzer beep_full <duration_ms> <times>");
		return -EINVAL;
	}

	shell_print(sh, "beeping %lu times, %lu ms each...", times, duration_ms);

	int ret = zbook_buzzer_beep_full((uint32_t)duration_ms, (uint32_t)times);

	if (ret != 0) {
		shell_error(sh, "zbook_buzzer_beep_full failed: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_buzzer_music(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "playing happy birthday...");

	for (size_t i = 0; i < ARRAY_SIZE(happy_birthday); i++) {
		uint32_t note_ms = BUZZER_MUSIC_WHOLE_NOTE_MS / happy_birthday[i].duration_divisor;
		uint32_t play_ms = (note_ms * 9U) / 10U;
		int ret = zbook_buzzer_set_frequency(happy_birthday[i].frequency_hz);

		if (ret != 0) {
			shell_error(sh, "zbook_buzzer_set_frequency failed: %d", ret);
			return ret;
		}

		ret = zbook_buzzer_beep_ms(play_ms);
		if (ret != 0) {
			shell_error(sh, "zbook_buzzer_beep_ms failed: %d", ret);
			return ret;
		}

		k_msleep(note_ms - play_ms);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_buzzer, SHELL_CMD_ARG(on, NULL, "Turn the buzzer on.", cmd_buzzer_on, 1, 0),
	SHELL_CMD_ARG(off, NULL, "Turn the buzzer off.", cmd_buzzer_off, 1, 0),
	SHELL_CMD_ARG(volume, NULL, "Set the buzzer volume.\nUsage: buzzer volume <0-100>",
		      cmd_buzzer_volume, 2, 0),
	SHELL_CMD_ARG(frequency, NULL, "Set the buzzer tone.\nUsage: buzzer frequency <hz>",
		      cmd_buzzer_frequency, 2, 0),
	SHELL_CMD_ARG(beep, NULL,
		      "Beep the buzzer for a duration.\n"
		      "Usage: buzzer beep <duration_ms>",
		      cmd_buzzer_beep, 2, 0),
	SHELL_CMD_ARG(beep_repeated, NULL,
		      "Beep the buzzer N times, at the normal preset duration.\n"
		      "Usage: buzzer beep_repeated <times>",
		      cmd_buzzer_beep_repeated, 2, 0),
	SHELL_CMD_ARG(beep_preset, NULL,
		      "Beep once, at a preset duration.\n"
		      "Usage: buzzer beep_preset <short|normal|long>",
		      cmd_buzzer_beep_preset, 2, 0),
	SHELL_CMD_ARG(beep_full, NULL,
		      "Beep the buzzer N times, each for a given duration.\n"
		      "Usage: buzzer beep_full <duration_ms> <times>",
		      cmd_buzzer_beep_full, 3, 0),
	SHELL_CMD_ARG(music, NULL, "Play happy birthday.", cmd_buzzer_music, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(buzzer, &sub_buzzer, "ZBook buzzer actuator commands.", NULL);

int main(void)
{
	int ret = zbook_buzzer_init();

	if (ret != 0) {
		printk("zbook_buzzer_init failed: %d\n", ret);
		return ret;
	}

	printk("ZBook buzzer shell sample, try: 'buzzer' to see available commands.\n");

	return 0;
}
