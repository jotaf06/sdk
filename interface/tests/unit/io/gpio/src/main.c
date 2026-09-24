#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/ztest.h>

#include "io/zbook_gpio.h"

static bool cb_called;
static int cb_call_count;
static enum zbook_gpio_io cb_io;
static void *cb_user_data;

static void test_cb(enum zbook_gpio_io io, void *user_data)
{
	cb_called = true;
	cb_call_count++;
	cb_io = io;
	cb_user_data = user_data;
}

static void reset_cb_state(void *fixture)
{
	ARG_UNUSED(fixture);
	cb_called = false;
	cb_call_count = 0;
	cb_io = 0;
	cb_user_data = NULL;
}

ZTEST_SUITE(zbook_gpio, NULL, NULL, reset_cb_state, NULL, NULL);

ZTEST(zbook_gpio, test_00_not_wired_up)
{
	struct zbook_gpio_cfg cfg = {.direction = ZBOOK_GPIO_DIR_OUTPUT,
				     .pull = ZBOOK_GPIO_PULL_NONE};
	bool value;

	zassert_equal(-ENODEV, zbook_gpio_init(ZBOOK_GPIO_IO45));
	zassert_equal(-ENODEV, zbook_gpio_configure(ZBOOK_GPIO_IO45, &cfg));
	zassert_equal(-ENODEV, zbook_gpio_write(ZBOOK_GPIO_IO45, true));
	zassert_equal(-ENODEV, zbook_gpio_read(ZBOOK_GPIO_IO45, &value));
	zassert_equal(-ENODEV, zbook_gpio_set_callback(ZBOOK_GPIO_IO45, ZBOOK_GPIO_EDGE_RISING,
						       test_cb, NULL));
	zassert_equal(-ENODEV, zbook_gpio_remove_callback(ZBOOK_GPIO_IO45));
}

ZTEST(zbook_gpio, test_01_not_ready_before_init)
{
	struct zbook_gpio_cfg cfg = {.direction = ZBOOK_GPIO_DIR_OUTPUT,
				     .pull = ZBOOK_GPIO_PULL_NONE};
	bool value;

	zassert_equal(-ENODEV, zbook_gpio_configure(ZBOOK_GPIO_IO01, &cfg));
	zassert_equal(-ENODEV, zbook_gpio_write(ZBOOK_GPIO_IO01, true));
	zassert_equal(-ENODEV, zbook_gpio_read(ZBOOK_GPIO_IO01, &value));
	zassert_equal(-ENODEV, zbook_gpio_set_callback(ZBOOK_GPIO_IO01, ZBOOK_GPIO_EDGE_RISING,
						       test_cb, NULL));
	zassert_equal(-ENODEV, zbook_gpio_remove_callback(ZBOOK_GPIO_IO01));
}

ZTEST(zbook_gpio, test_02_init_defaults_to_input)
{
	bool value;

	zassert_ok(zbook_gpio_init(ZBOOK_GPIO_IO01));
	zassert_equal(-EACCES, zbook_gpio_write(ZBOOK_GPIO_IO01, true));

	const struct gpio_dt_spec spec = GPIO_DT_SPEC_GET(DT_NODELABEL(zbook_gpio_test_a), gpios);

	zassert_ok(gpio_emul_input_set(spec.port, spec.pin, 0));
	zassert_ok(zbook_gpio_read(ZBOOK_GPIO_IO01, &value));
	zassert_false(value);
}

ZTEST(zbook_gpio, test_03_configure_output_write_read_roundtrip)
{
	struct zbook_gpio_cfg cfg = {.direction = ZBOOK_GPIO_DIR_OUTPUT,
				     .pull = ZBOOK_GPIO_PULL_NONE};
	const struct gpio_dt_spec spec = GPIO_DT_SPEC_GET(DT_NODELABEL(zbook_gpio_test_a), gpios);

	zassert_ok(zbook_gpio_configure(ZBOOK_GPIO_IO01, &cfg));

	zassert_ok(zbook_gpio_write(ZBOOK_GPIO_IO01, true));
	zassert_equal(1, gpio_emul_output_get(spec.port, spec.pin));

	zassert_ok(zbook_gpio_write(ZBOOK_GPIO_IO01, false));
	zassert_equal(0, gpio_emul_output_get(spec.port, spec.pin));

	bool value;

	zassert_equal(-EACCES, zbook_gpio_read(ZBOOK_GPIO_IO01, &value));
}

ZTEST(zbook_gpio, test_04_configure_invalid_args)
{
	struct zbook_gpio_cfg bad_dir = {.direction = 99, .pull = ZBOOK_GPIO_PULL_NONE};
	struct zbook_gpio_cfg bad_pull = {.direction = ZBOOK_GPIO_DIR_INPUT, .pull = 99};
	struct zbook_gpio_cfg bad_active = {
		.direction = ZBOOK_GPIO_DIR_INPUT, .pull = ZBOOK_GPIO_PULL_NONE, .active = 99};

	zassert_equal(-EINVAL, zbook_gpio_configure(ZBOOK_GPIO_IO01, NULL));
	zassert_equal(-EINVAL, zbook_gpio_configure(ZBOOK_GPIO_IO01, &bad_dir));
	zassert_equal(-EINVAL, zbook_gpio_configure(ZBOOK_GPIO_IO01, &bad_pull));
	zassert_equal(-EINVAL, zbook_gpio_configure(ZBOOK_GPIO_IO01, &bad_active));
}

ZTEST(zbook_gpio, test_05_write_eacces_after_back_to_input)
{
	struct zbook_gpio_cfg cfg = {.direction = ZBOOK_GPIO_DIR_INPUT,
				     .pull = ZBOOK_GPIO_PULL_NONE};

	zassert_ok(zbook_gpio_configure(ZBOOK_GPIO_IO01, &cfg));
	zassert_equal(-EACCES, zbook_gpio_write(ZBOOK_GPIO_IO01, true));
}

ZTEST(zbook_gpio, test_06_read_invalid_args)
{
	zassert_equal(-EINVAL, zbook_gpio_read(ZBOOK_GPIO_IO01, NULL));
}

ZTEST(zbook_gpio, test_07_callback_fires_once_after_debounce)
{
	const struct gpio_dt_spec spec = GPIO_DT_SPEC_GET(DT_NODELABEL(zbook_gpio_test_b), gpios);

	zassert_ok(zbook_gpio_init(ZBOOK_GPIO_IO39));
	zassert_ok(gpio_emul_input_set(spec.port, spec.pin, 0));
	zassert_ok(zbook_gpio_set_callback(ZBOOK_GPIO_IO39, ZBOOK_GPIO_EDGE_RISING, test_cb,
					   (void *)0x1234));

	/* Bouncy signal: several transitions right after the first rising
	 * edge, all inside the 20ms debounce-interval-ms window from
	 * boards/native_sim.overlay -- only the first one is observed (the
	 * interrupt is masked until the debounce timer fires), so exactly
	 * one callback comes out the other end.
	 */
	zassert_ok(gpio_emul_input_set(spec.port, spec.pin, 1));
	zassert_ok(gpio_emul_input_set(spec.port, spec.pin, 0));
	zassert_ok(gpio_emul_input_set(spec.port, spec.pin, 1));
	k_sleep(K_MSEC(40));

	zassert_true(cb_called);
	zassert_equal(1, cb_call_count);
	zassert_equal(ZBOOK_GPIO_IO39, cb_io);
	zassert_equal_ptr((void *)0x1234, cb_user_data);
}

ZTEST(zbook_gpio, test_08_remove_callback_stops_firing)
{
	const struct gpio_dt_spec spec = GPIO_DT_SPEC_GET(DT_NODELABEL(zbook_gpio_test_b), gpios);

	zassert_ok(zbook_gpio_remove_callback(ZBOOK_GPIO_IO39));

	zassert_ok(gpio_emul_input_set(spec.port, spec.pin, 0));
	zassert_ok(gpio_emul_input_set(spec.port, spec.pin, 1));
	k_sleep(K_MSEC(40));

	zassert_false(cb_called);
}

ZTEST(zbook_gpio, test_09_set_callback_requires_input_direction)
{
	struct zbook_gpio_cfg cfg = {.direction = ZBOOK_GPIO_DIR_OUTPUT,
				     .pull = ZBOOK_GPIO_PULL_NONE};

	zassert_ok(zbook_gpio_configure(ZBOOK_GPIO_IO39, &cfg));
	zassert_equal(-EACCES, zbook_gpio_set_callback(ZBOOK_GPIO_IO39, ZBOOK_GPIO_EDGE_RISING,
						       test_cb, NULL));
}

ZTEST(zbook_gpio, test_10_set_callback_invalid_args)
{
	struct zbook_gpio_cfg cfg = {.direction = ZBOOK_GPIO_DIR_INPUT,
				     .pull = ZBOOK_GPIO_PULL_NONE};

	zassert_ok(zbook_gpio_configure(ZBOOK_GPIO_IO39, &cfg));
	zassert_equal(-EINVAL,
		      zbook_gpio_set_callback(ZBOOK_GPIO_IO39, ZBOOK_GPIO_EDGE_RISING, NULL, NULL));
	zassert_equal(-EINVAL, zbook_gpio_set_callback(ZBOOK_GPIO_IO39, 99, test_cb, NULL));
}

ZTEST(zbook_gpio, test_11_second_channel_independent_of_first)
{
	bool value;

	zassert_ok(zbook_gpio_read(ZBOOK_GPIO_IO01, &value));
	zassert_false(value);
}

ZTEST(zbook_gpio, test_12_configure_pull_up_and_down)
{
	struct zbook_gpio_cfg cfg_up = {.direction = ZBOOK_GPIO_DIR_INPUT,
					.pull = ZBOOK_GPIO_PULL_UP};
	struct zbook_gpio_cfg cfg_down = {.direction = ZBOOK_GPIO_DIR_INPUT,
					  .pull = ZBOOK_GPIO_PULL_DOWN};
	const struct gpio_dt_spec spec = GPIO_DT_SPEC_GET(DT_NODELABEL(zbook_gpio_test_a), gpios);
	gpio_flags_t flags;

	zassert_ok(zbook_gpio_configure(ZBOOK_GPIO_IO01, &cfg_up));
	zassert_ok(gpio_emul_flags_get(spec.port, spec.pin, &flags));
	zassert_true(flags & GPIO_PULL_UP);

	zassert_ok(zbook_gpio_configure(ZBOOK_GPIO_IO01, &cfg_down));
	zassert_ok(gpio_emul_flags_get(spec.port, spec.pin, &flags));
	zassert_true(flags & GPIO_PULL_DOWN);
}

ZTEST(zbook_gpio, test_13_configure_active_low_inverts_read)
{
	struct zbook_gpio_cfg cfg = {.direction = ZBOOK_GPIO_DIR_INPUT,
				     .pull = ZBOOK_GPIO_PULL_NONE,
				     .active = ZBOOK_GPIO_ACTIVE_LOW};
	const struct gpio_dt_spec spec = GPIO_DT_SPEC_GET(DT_NODELABEL(zbook_gpio_test_a), gpios);
	bool value;

	zassert_ok(zbook_gpio_configure(ZBOOK_GPIO_IO01, &cfg));

	zassert_ok(gpio_emul_input_set(spec.port, spec.pin, 0));
	zassert_ok(zbook_gpio_read(ZBOOK_GPIO_IO01, &value));
	zassert_true(value);

	zassert_ok(gpio_emul_input_set(spec.port, spec.pin, 1));
	zassert_ok(zbook_gpio_read(ZBOOK_GPIO_IO01, &value));
	zassert_false(value);
}
