#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/drivers/pwm/pwm_fake.h>

#include "actuators/zbook_pwm.h"

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(zbook_pwm, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_pwm, test_00_invalid_label_null_and_empty)
{
	struct zbook_pwm_cfg cfg = {.frequency_hz = 1000, .duty_cycle_percent = 50};

	zassert_equal(-EINVAL, zbook_pwm_init(NULL));
	zassert_equal(-EINVAL, zbook_pwm_init(""));
	zassert_equal(-EINVAL, zbook_pwm_start(NULL));
	zassert_equal(-EINVAL, zbook_pwm_start(""));
	zassert_equal(-EINVAL, zbook_pwm_stop(NULL));
	zassert_equal(-EINVAL, zbook_pwm_stop(""));
	zassert_equal(-EINVAL, zbook_pwm_set_cfg(NULL, &cfg));
	zassert_equal(-EINVAL, zbook_pwm_set_cfg("", &cfg));
	zassert_equal(-EINVAL, zbook_pwm_get_cfg(NULL, &cfg));
	zassert_equal(-EINVAL, zbook_pwm_get_cfg("", &cfg));

	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_pwm, test_01_unknown_label)
{
	struct zbook_pwm_cfg cfg = {.frequency_hz = 1000, .duty_cycle_percent = 50};

	zassert_equal(-ENODEV, zbook_pwm_init("does_not_exist"));
	zassert_equal(-ENODEV, zbook_pwm_start("does_not_exist"));
	zassert_equal(-ENODEV, zbook_pwm_stop("does_not_exist"));
	zassert_equal(-ENODEV, zbook_pwm_set_cfg("does_not_exist", &cfg));
	zassert_equal(-ENODEV, zbook_pwm_get_cfg("does_not_exist", &cfg));

	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_pwm, test_02_not_ready_before_init)
{
	struct zbook_pwm_cfg cfg = {.frequency_hz = 1000, .duty_cycle_percent = 50};

	/* "test_a" exists in devicetree but zbook_pwm_init() hasn't run yet --
	 * distinct from test_01's "unknown label" (ch == NULL) branch.
	 */
	zassert_equal(-ENODEV, zbook_pwm_start("test_a"));
	zassert_equal(-ENODEV, zbook_pwm_stop("test_a"));
	zassert_equal(-ENODEV, zbook_pwm_set_cfg("test_a", &cfg));
	zassert_equal(-ENODEV, zbook_pwm_get_cfg("test_a", &cfg));

	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_pwm, test_03_init_ok_seeds_default_cfg)
{
	struct zbook_pwm_cfg cfg;

	zassert_ok(zbook_pwm_init("test_a"));

	/* init() applies the default cfg (0% duty) once, on channel 0. */
	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg1_val);
	zassert_equal(1000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
	zassert_equal(PWM_POLARITY_NORMAL, fake_pwm_set_cycles_fake.arg4_val);

	/* Default frequency seeded from the devicetree period (1000000ns -> 1000Hz). */
	zassert_ok(zbook_pwm_get_cfg("test_a", &cfg));
	zassert_equal(1000, cfg.frequency_hz);
	zassert_equal(0, cfg.duty_cycle_percent);
	zassert_equal(ZBOOK_PWM_POLARITY_NORMAL, cfg.polarity);
}

ZTEST(zbook_pwm, test_04_init_second_channel_uses_its_own_index)
{
	struct zbook_pwm_cfg cfg;

	zassert_ok(zbook_pwm_init("test_b"));

	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(1, fake_pwm_set_cycles_fake.arg1_val);
	zassert_equal(2000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);

	zassert_ok(zbook_pwm_get_cfg("test_b", &cfg));
	zassert_equal(500, cfg.frequency_hz);
	zassert_equal(0, cfg.duty_cycle_percent);
}

ZTEST(zbook_pwm, test_05_set_cfg_while_stopped_forces_zero_duty)
{
	struct zbook_pwm_cfg cfg = {
		.frequency_hz = 500,
		.duty_cycle_percent = 75,
		.polarity = ZBOOK_PWM_POLARITY_NORMAL,
	};

	zassert_ok(zbook_pwm_set_cfg("test_a", &cfg));

	/* Not started yet -- duty is forced to 0 regardless of cfg. */
	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(2000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
	zassert_equal(PWM_POLARITY_NORMAL, fake_pwm_set_cycles_fake.arg4_val);
}

ZTEST(zbook_pwm, test_06_start_applies_configured_duty)
{
	zassert_ok(zbook_pwm_start("test_a"));

	/* Same cfg as the previous test (500Hz, 75%), now actually applied. */
	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(2000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(1500000, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_pwm, test_07_set_cfg_live_while_playing_updates_duty_and_polarity)
{
	struct zbook_pwm_cfg cfg = {
		.frequency_hz = 250,
		.duty_cycle_percent = 25,
		.polarity = ZBOOK_PWM_POLARITY_INVERTED,
	};

	zassert_ok(zbook_pwm_set_cfg("test_a", &cfg));

	/* Still playing -- new duty is applied immediately, not deferred to
	 * a start() call, and the new polarity reaches pwm_set() as flags.
	 */
	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(4000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(1000000, fake_pwm_set_cycles_fake.arg3_val);
	zassert_equal(PWM_POLARITY_INVERTED, fake_pwm_set_cycles_fake.arg4_val);
}

ZTEST(zbook_pwm, test_08_stop_zeroes_duty_keeps_rest_of_cfg)
{
	zassert_ok(zbook_pwm_stop("test_a"));

	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(4000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
	zassert_equal(PWM_POLARITY_INVERTED, fake_pwm_set_cycles_fake.arg4_val);
}

ZTEST(zbook_pwm, test_09_get_cfg_reflects_last_set_not_hardware)
{
	struct zbook_pwm_cfg cfg;

	/* Duty at the hardware is 0 right now (stopped), but get_cfg() must
	 * still report the last requested 25%, not the applied 0%.
	 */
	zassert_ok(zbook_pwm_get_cfg("test_a", &cfg));
	zassert_equal(250, cfg.frequency_hz);
	zassert_equal(25, cfg.duty_cycle_percent);
	zassert_equal(ZBOOK_PWM_POLARITY_INVERTED, cfg.polarity);

	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_pwm, test_10_set_cfg_invalid_args)
{
	struct zbook_pwm_cfg zero_freq = {.frequency_hz = 0, .duty_cycle_percent = 50};
	struct zbook_pwm_cfg bad_duty = {.frequency_hz = 100, .duty_cycle_percent = 101};

	zassert_equal(-EINVAL, zbook_pwm_set_cfg("test_a", NULL));
	zassert_equal(-EINVAL, zbook_pwm_set_cfg("test_a", &zero_freq));
	zassert_equal(-EINVAL, zbook_pwm_set_cfg("test_a", &bad_duty));

	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_pwm, test_11_get_cfg_invalid_args)
{
	zassert_equal(-EINVAL, zbook_pwm_get_cfg("test_a", NULL));

	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_pwm, test_12_second_channel_independent_of_first)
{
	struct zbook_pwm_cfg cfg = {
		.frequency_hz = 100,
		.duty_cycle_percent = 50,
		.polarity = ZBOOK_PWM_POLARITY_NORMAL,
	};
	struct zbook_pwm_cfg cfg_a;

	zassert_ok(zbook_pwm_set_cfg("test_b", &cfg));
	zassert_equal(1, fake_pwm_set_cycles_fake.arg1_val);
	zassert_equal(10000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);

	zassert_ok(zbook_pwm_start("test_b"));
	zassert_equal(1, fake_pwm_set_cycles_fake.arg1_val);
	zassert_equal(10000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(5000000, fake_pwm_set_cycles_fake.arg3_val);

	/* "test_a" (channel 0) must be untouched by any of the "test_b"
	 * (channel 1) calls above.
	 */
	zassert_ok(zbook_pwm_get_cfg("test_a", &cfg_a));
	zassert_equal(250, cfg_a.frequency_hz);
	zassert_equal(25, cfg_a.duty_cycle_percent);
	zassert_equal(ZBOOK_PWM_POLARITY_INVERTED, cfg_a.polarity);
}
