/*******************************************************************
 * @file main.c
 *
 * @brief Real-hardware bring-up sample for a zbook generic PWM channel:
 * initializes the sample's default "dimmer" channel at boot, then exposes
 * start/stop/set/get as shell commands for manual testing by label.
 *
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 16/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/printk.h>

#include "actuators/zbook_pwm.h"

#define DEFAULT_CHANNEL_LABEL "dimmer"

static int cmd_pwm_start(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int ret = zbook_pwm_start(argv[1]);

	if (ret != 0) {
		shell_error(sh, "zbook_pwm_start failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" started", argv[1]);

	return 0;
}

static int cmd_pwm_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int ret = zbook_pwm_stop(argv[1]);

	if (ret != 0) {
		shell_error(sh, "zbook_pwm_stop failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" stopped", argv[1]);

	return 0;
}

static int cmd_pwm_set(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int err = 0;
	struct zbook_pwm_cfg cfg = {
		.frequency_hz = (uint32_t)shell_strtoul(argv[2], 10, &err),
		.duty_cycle_percent = (uint8_t)shell_strtoul(argv[3], 10, &err),
	};

	if (strcmp(argv[4], "normal") == 0) {
		cfg.polarity = ZBOOK_PWM_POLARITY_NORMAL;
	} else if (strcmp(argv[4], "inverted") == 0) {
		cfg.polarity = ZBOOK_PWM_POLARITY_INVERTED;
	} else {
		err = -EINVAL;
	}

	if (err != 0) {
		shell_error(sh, "usage: pwm set <label> <freq_hz> <duty 0-100> <normal|inverted>");
		return -EINVAL;
	}

	int ret = zbook_pwm_set_cfg(argv[1], &cfg);

	if (ret != 0) {
		shell_error(sh, "zbook_pwm_set_cfg failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" set to %u Hz, %u%% duty, %s", argv[1], cfg.frequency_hz,
		    cfg.duty_cycle_percent, argv[4]);

	return 0;
}

static int cmd_pwm_get(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	struct zbook_pwm_cfg cfg;
	int ret = zbook_pwm_get_cfg(argv[1], &cfg);

	if (ret != 0) {
		shell_error(sh, "zbook_pwm_get_cfg failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\": %u Hz, %u%% duty, %s", argv[1], cfg.frequency_hz,
		    cfg.duty_cycle_percent,
		    cfg.polarity == ZBOOK_PWM_POLARITY_INVERTED ? "inverted" : "normal");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_pwm,
	SHELL_CMD_ARG(start, NULL, "Start a channel.\nUsage: pwm start <label>", cmd_pwm_start, 2,
		      0),
	SHELL_CMD_ARG(stop, NULL, "Stop a channel.\nUsage: pwm stop <label>", cmd_pwm_stop, 2, 0),
	SHELL_CMD_ARG(set, NULL,
		      "Configure a channel.\n"
		      "Usage: pwm set <label> <freq_hz> <duty 0-100> <normal|inverted>",
		      cmd_pwm_set, 5, 0),
	SHELL_CMD_ARG(get, NULL, "Read back a channel's config.\nUsage: pwm get <label>",
		      cmd_pwm_get, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pwm, &sub_pwm, "ZBook generic PWM channel commands.", NULL);

int main(void)
{
	int ret = zbook_pwm_init(DEFAULT_CHANNEL_LABEL);

	if (ret != 0) {
		printk("zbook_pwm_init(\"%s\") failed: %d\n", DEFAULT_CHANNEL_LABEL, ret);
		return ret;
	}

	printk("ZBook PWM shell sample, channel \"%s\" ready. Try: 'pwm' to see available "
	       "commands.\n",
	       DEFAULT_CHANNEL_LABEL);

	return 0;
}
