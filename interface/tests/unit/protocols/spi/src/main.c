#include <zephyr/ztest.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#include "protocols/zbook_spi.h"
#include "zbook_spi_loopback_emul.h"

ZTEST_SUITE(zbook_spi, NULL, NULL, NULL, NULL, NULL);

ZTEST(zbook_spi, test_00_not_ready_before_init)
{
	uint8_t byte = 0;

	zassert_equal(-ENODEV, zbook_spi_read(&byte, 1));
}

ZTEST(zbook_spi, test_00a_configure_before_init)
{
	struct zbook_spi_cfg cfg = {.frequency = 1000000, .word_size = 8};

	zassert_equal(-ENODEV, zbook_spi_configure(&cfg));
}

ZTEST(zbook_spi, test_01_init_ok)
{
	zassert_ok(zbook_spi_init());
}

ZTEST(zbook_spi, test_configure_invalid_args)
{
	struct zbook_spi_cfg cfg = {.frequency = 1000000, .word_size = 8};

	zassert_equal(-EINVAL, zbook_spi_configure(NULL));

	cfg.word_size = 0;
	zassert_equal(-EINVAL, zbook_spi_configure(&cfg));

	cfg.word_size = 64;
	zassert_equal(-EINVAL, zbook_spi_configure(&cfg));

	cfg.word_size = 8;
	cfg.mode = (enum zbook_spi_mode)99;
	zassert_equal(-EINVAL, zbook_spi_configure(&cfg));
}

ZTEST(zbook_spi, test_configure_covers_all_spi_modes)
{
	static const struct {
		enum zbook_spi_mode mode;
		bool cpol;
		bool cpha;
	} cases[] = {
		{ZBOOK_SPI_MODE_0, false, false},
		{ZBOOK_SPI_MODE_1, false, true},
		{ZBOOK_SPI_MODE_2, true, false},
		{ZBOOK_SPI_MODE_3, true, true},
	};
	uint8_t tx = 0x00;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct zbook_spi_cfg cfg = {
			.frequency = 1000000,
			.mode = cases[i].mode,
			.bit_order = ZBOOK_SPI_MSB_FIRST,
			.word_size = 8,
		};

		zassert_ok(zbook_spi_configure(&cfg));
		zassert_ok(zbook_spi_write(&tx, 1));

		zassert_equal(cases[i].cpol,
			      (zbook_spi_test_emul_last_operation() & SPI_MODE_CPOL) != 0);
		zassert_equal(cases[i].cpha,
			      (zbook_spi_test_emul_last_operation() & SPI_MODE_CPHA) != 0);
	}
}

ZTEST(zbook_spi, test_configure_msb_first_bit_order)
{
	struct zbook_spi_cfg cfg = {
		.frequency = 1000000,
		.mode = ZBOOK_SPI_MODE_0,
		.bit_order = ZBOOK_SPI_MSB_FIRST,
		.word_size = 8,
	};
	uint8_t tx = 0x00;

	zassert_ok(zbook_spi_configure(&cfg));
	zassert_ok(zbook_spi_write(&tx, 1));

	zassert_false(zbook_spi_test_emul_last_operation() & SPI_TRANSFER_LSB);
}

ZTEST(zbook_spi, test_configure_applies_operation_bits)
{
	struct zbook_spi_cfg cfg = {
		.frequency = 250000,
		.mode = ZBOOK_SPI_MODE_3,
		.bit_order = ZBOOK_SPI_LSB_FIRST,
		.word_size = 8,
	};
	uint8_t tx = 0xAA;

	zassert_ok(zbook_spi_configure(&cfg));
	zassert_ok(zbook_spi_write(&tx, 1));

	zassert_equal(250000, zbook_spi_test_emul_last_frequency());
	zassert_true(zbook_spi_test_emul_last_operation() & SPI_MODE_CPOL);
	zassert_true(zbook_spi_test_emul_last_operation() & SPI_MODE_CPHA);
	zassert_true(zbook_spi_test_emul_last_operation() & SPI_TRANSFER_LSB);
}

ZTEST(zbook_spi, test_write_invalid_args)
{
	uint8_t byte = 0;

	zassert_equal(-EINVAL, zbook_spi_write(NULL, 1));
	zassert_equal(-EINVAL, zbook_spi_write(&byte, 0));
}

ZTEST(zbook_spi, test_write_captures_tx_bytes)
{
	uint8_t tx[3] = {0x01, 0x02, 0x03};
	uint8_t captured[3] = {0};

	zassert_ok(zbook_spi_write(tx, sizeof(tx)));
	zassert_equal(sizeof(tx), zbook_spi_test_emul_last_tx(captured, sizeof(captured)));
	zassert_mem_equal(tx, captured, sizeof(tx));
}

ZTEST(zbook_spi, test_read_invalid_args)
{
	uint8_t byte = 0;

	zassert_equal(-EINVAL, zbook_spi_read(NULL, 1));
	zassert_equal(-EINVAL, zbook_spi_read(&byte, 0));
}

ZTEST(zbook_spi, test_read_returns_canned_pattern)
{
	uint8_t rx[4] = {0};

	zbook_spi_test_emul_set_read_byte(0x5A);
	zassert_ok(zbook_spi_read(rx, sizeof(rx)));
	for (size_t i = 0; i < sizeof(rx); i++) {
		zassert_equal(0x5A, rx[i]);
	}
}

ZTEST(zbook_spi, test_transceive_full_duplex_loopback)
{
	uint8_t tx[4] = {0xDE, 0xAD, 0xBE, 0xEF};
	uint8_t rx[4] = {0};

	zassert_ok(zbook_spi_transceive(tx, rx, sizeof(tx)));
	zassert_mem_equal(tx, rx, sizeof(tx));
}

ZTEST(zbook_spi, test_transceive_invalid_args)
{
	uint8_t byte = 0;

	zassert_equal(-EINVAL, zbook_spi_transceive(NULL, NULL, 1));
	zassert_equal(-EINVAL, zbook_spi_transceive(&byte, NULL, 0));
}
