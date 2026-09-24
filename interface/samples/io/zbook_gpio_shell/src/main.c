/*******************************************************************
 * @file main.c
 *
 * @brief Real-hardware bring-up sample for zbook generic GPIO channels:
 * exposes init/configure/write/read/watch as shell commands, addressed by
 * IO name ("io01"/"io39"/"io45"/"io46"), for manual testing over the
 * console.
 *
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 23/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "io/zbook_gpio.h"

static const struct {
	const char *name;
	enum zbook_gpio_io io;
} io_table[] = {
	{"io01", ZBOOK_GPIO_IO01},
	{"io39", ZBOOK_GPIO_IO39},
	{"io45", ZBOOK_GPIO_IO45},
	{"io46", ZBOOK_GPIO_IO46},
};

static int parse_io(const char *name, enum zbook_gpio_io *io)
{
	for (size_t i = 0; i < ARRAY_SIZE(io_table); i++) {
		if (strcmp(io_table[i].name, name) == 0) {
			*io = io_table[i].io;
			return 0;
		}
	}

	return -EINVAL;
}

static const char *io_name(enum zbook_gpio_io io)
{
	for (size_t i = 0; i < ARRAY_SIZE(io_table); i++) {
		if (io_table[i].io == io) {
			return io_table[i].name;
		}
	}

	return "?";
}

static int cmd_gpio_init(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_gpio_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	int ret = zbook_gpio_init(io);

	if (ret != 0) {
		shell_error(sh, "zbook_gpio_init failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" initialized", argv[1]);

	return 0;
}

static int cmd_gpio_configure(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_gpio_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	struct zbook_gpio_cfg cfg;

	if (strcmp(argv[2], "in") == 0) {
		cfg.direction = ZBOOK_GPIO_DIR_INPUT;
	} else if (strcmp(argv[2], "out") == 0) {
		cfg.direction = ZBOOK_GPIO_DIR_OUTPUT;
	} else {
		shell_error(sh, "usage: gpio configure <io> <in|out> <none|up|down>");
		return -EINVAL;
	}

	if (strcmp(argv[3], "none") == 0) {
		cfg.pull = ZBOOK_GPIO_PULL_NONE;
	} else if (strcmp(argv[3], "up") == 0) {
		cfg.pull = ZBOOK_GPIO_PULL_UP;
	} else if (strcmp(argv[3], "down") == 0) {
		cfg.pull = ZBOOK_GPIO_PULL_DOWN;
	} else {
		shell_error(sh, "usage: gpio configure <io> <in|out> <none|up|down> <high|low>");
		return -EINVAL;
	}

	if (strcmp(argv[4], "high") == 0) {
		cfg.active = ZBOOK_GPIO_ACTIVE_HIGH;
	} else if (strcmp(argv[4], "low") == 0) {
		cfg.active = ZBOOK_GPIO_ACTIVE_LOW;
	} else {
		shell_error(sh, "usage: gpio configure <io> <in|out> <none|up|down> <high|low>");
		return -EINVAL;
	}

	int ret = zbook_gpio_configure(io, &cfg);

	if (ret != 0) {
		shell_error(sh, "zbook_gpio_configure failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" configured as %s, pull %s, active %s", argv[1], argv[2], argv[3],
		    argv[4]);

	return 0;
}

static int cmd_gpio_write(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_gpio_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	bool value;

	if (strcmp(argv[2], "0") == 0) {
		value = false;
	} else if (strcmp(argv[2], "1") == 0) {
		value = true;
	} else {
		shell_error(sh, "usage: gpio write <io> <0|1>");
		return -EINVAL;
	}

	int ret = zbook_gpio_write(io, value);

	if (ret != 0) {
		shell_error(sh, "zbook_gpio_write failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" set to %d", argv[1], value);

	return 0;
}

static int cmd_gpio_read(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_gpio_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	bool value;
	int ret = zbook_gpio_read(io, &value);

	if (ret != 0) {
		shell_error(sh, "zbook_gpio_read failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\": %d", argv[1], value);

	return 0;
}

static void gpio_watch_cb(enum zbook_gpio_io io, void *user_data)
{
	ARG_UNUSED(user_data);

	printk("\"%s\" edge triggered\n", io_name(io));
}

static int cmd_gpio_watch(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_gpio_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	enum zbook_gpio_edge edge;

	if (strcmp(argv[2], "rising") == 0) {
		edge = ZBOOK_GPIO_EDGE_RISING;
	} else if (strcmp(argv[2], "falling") == 0) {
		edge = ZBOOK_GPIO_EDGE_FALLING;
	} else if (strcmp(argv[2], "both") == 0) {
		edge = ZBOOK_GPIO_EDGE_BOTH;
	} else {
		shell_error(sh, "usage: gpio watch <io> <rising|falling|both>");
		return -EINVAL;
	}

	int ret = zbook_gpio_set_callback(io, edge, gpio_watch_cb, NULL);

	if (ret != 0) {
		shell_error(sh, "zbook_gpio_set_callback failed: %d", ret);
		return ret;
	}

	shell_print(sh, "watching \"%s\" for %s edges", argv[1], argv[2]);

	return 0;
}

static int cmd_gpio_unwatch(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	enum zbook_gpio_io io;

	if (parse_io(argv[1], &io) != 0) {
		shell_error(sh, "unknown IO \"%s\" -- try io01/io39/io45/io46", argv[1]);
		return -EINVAL;
	}

	int ret = zbook_gpio_remove_callback(io);

	if (ret != 0) {
		shell_error(sh, "zbook_gpio_remove_callback failed: %d", ret);
		return ret;
	}

	shell_print(sh, "\"%s\" no longer watched", argv[1]);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gpio,
	SHELL_CMD_ARG(init, NULL, "Initialize an IO.\nUsage: gpio init <io>", cmd_gpio_init, 2, 0),
	SHELL_CMD_ARG(configure, NULL,
		      "Configure an IO.\n"
		      "Usage: gpio configure <io> <in|out> <none|up|down> <high|low>",
		      cmd_gpio_configure, 5, 0),
	SHELL_CMD_ARG(write, NULL, "Write an IO.\nUsage: gpio write <io> <0|1>", cmd_gpio_write, 3,
		      0),
	SHELL_CMD_ARG(read, NULL, "Read an IO.\nUsage: gpio read <io>", cmd_gpio_read, 2, 0),
	SHELL_CMD_ARG(watch, NULL,
		      "Watch an IO for edges.\nUsage: gpio watch <io> <rising|falling|both>",
		      cmd_gpio_watch, 3, 0),
	SHELL_CMD_ARG(unwatch, NULL, "Stop watching an IO.\nUsage: gpio unwatch <io>",
		      cmd_gpio_unwatch, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(gpio, &sub_gpio, "ZBook generic GPIO channel commands.", NULL);

int main(void)
{
	printk("ZBook GPIO shell sample ready. Try: 'gpio init io01', then 'gpio' to see "
	       "available commands.\n");

	return 0;
}
