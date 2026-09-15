#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/drivers/pwm/pwm_fake.h>

#include "actuators/zbook_buzzer.h"

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(zbook_buzzer, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_buzzer, test_00_not_ready_before_init)
{
	zassert_equal(-ENODEV, zbook_buzzer_on());
	zassert_equal(-ENODEV, zbook_buzzer_off());
	zassert_equal(-ENODEV, zbook_buzzer_set_volume(50));
	zassert_equal(-ENODEV, zbook_buzzer_set_frequency(1000));
	zassert_equal(-ENODEV, zbook_buzzer_beep_ms(1));
	zassert_equal(-ENODEV, zbook_buzzer_beep_repeated(1));
	zassert_equal(-ENODEV, zbook_buzzer_beep(ZBOOK_BUZZER_BEEP_DURATION_SHORT));
	zassert_equal(-ENODEV, zbook_buzzer_beep_full(1, 1));
}

ZTEST(zbook_buzzer, test_01_init_ok)
{
	zassert_ok(zbook_buzzer_init());
}

ZTEST(zbook_buzzer, test_02_on_sets_half_duty)
{
	/* 50% duty (not 100%, which is a constant DC level -- silent on a
	 * piezo buzzer): the loudest audible square wave at this fixed
	 * carrier frequency, at the default volume (100).
	 */
	zassert_ok(zbook_buzzer_on());

	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(500000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(250000, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_03_set_volume_scales_duty)
{
	/* Still on() from the previous test: volume 50 -> half of the
	 * audible 0-50% duty range -> 25% duty, applied live.
	 */
	zassert_ok(zbook_buzzer_set_volume(50));

	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(125000, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_04_off_sets_zero_duty)
{
	zassert_ok(zbook_buzzer_off());

	zassert_equal(1, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_05_set_volume_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_set_volume(101));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_06_set_frequency_changes_period)
{
	/* The buzzer is passive: retuning it to 1kHz (half the board's
	 * default 2kHz) doubles the period, and at full volume the pulse is
	 * still exactly half of that new period (50% duty).
	 */
	zassert_ok(zbook_buzzer_on());
	zassert_ok(zbook_buzzer_set_volume(100));
	zassert_ok(zbook_buzzer_set_frequency(1000));

	zassert_equal(1000000, fake_pwm_set_cycles_fake.arg2_val);
	zassert_equal(500000, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_07_set_frequency_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_set_frequency(0));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_08_beep_ms_ends_off)
{
	zassert_ok(zbook_buzzer_beep_ms(5));

	zassert_equal(2, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_09_beep_repeated_calls_multiple_times)
{
	/* Each beep is an on+off pair, so 3 repeats make 6 pwm_set() calls. */
	zassert_ok(zbook_buzzer_beep_repeated(3));

	zassert_equal(6, fake_pwm_set_cycles_fake.call_count);
	zassert_equal(0, fake_pwm_set_cycles_fake.arg3_val);
}

ZTEST(zbook_buzzer, test_10_beep_repeated_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_beep_repeated(0));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_11_beep_preset_lengths)
{
	zassert_ok(zbook_buzzer_beep(ZBOOK_BUZZER_BEEP_DURATION_SHORT));
	zassert_ok(zbook_buzzer_beep(ZBOOK_BUZZER_BEEP_DURATION_NORMAL));
	zassert_ok(zbook_buzzer_beep(ZBOOK_BUZZER_BEEP_DURATION_LONG));
}

ZTEST(zbook_buzzer, test_12_beep_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_beep((enum zbook_buzzer_beep_duration)99));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_13_beep_full_calls_multiple_times)
{
	zassert_ok(zbook_buzzer_beep_full(5, 4));

	zassert_equal(8, fake_pwm_set_cycles_fake.call_count);
}

ZTEST(zbook_buzzer, test_14_beep_full_invalid_args)
{
	zassert_equal(-EINVAL, zbook_buzzer_beep_full(10, 0));
	zassert_equal(0, fake_pwm_set_cycles_fake.call_count);
}
