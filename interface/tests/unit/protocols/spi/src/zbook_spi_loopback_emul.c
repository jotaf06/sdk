/*
 * SPI loopback emulator backing the "zbook_spi" devicetree node for unit
 * tests: on a full-duplex transfer it echoes tx into rx, on a write-only
 * transfer it just records tx, and on a read-only transfer it stuffs rx with
 * a fixed byte set via zbook_spi_test_emul_set_read_byte(). It also captures
 * the last spi_config (frequency/operation) so a test can verify
 * zbook_spi_configure() actually reached the bus.
 */

#include "zbook_spi_loopback_emul.h"

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/sys/util.h>

#define ZBOOK_SPI_TEST_NODE DT_NODELABEL(zbook_spi)
#define MAX_CAPTURE_LEN     32

struct zbook_spi_loopback_data {
	struct spi_config last_config;
	uint8_t last_tx[MAX_CAPTURE_LEN];
	size_t last_tx_len;
	uint8_t read_byte;
};

static struct zbook_spi_loopback_data emul_data;

void zbook_spi_test_emul_reset(void)
{
	memset(&emul_data, 0, sizeof(emul_data));
}

uint32_t zbook_spi_test_emul_last_frequency(void)
{
	return emul_data.last_config.frequency;
}

uint32_t zbook_spi_test_emul_last_operation(void)
{
	return emul_data.last_config.operation;
}

size_t zbook_spi_test_emul_last_tx(uint8_t *out, size_t max_len)
{
	size_t len = MIN(emul_data.last_tx_len, max_len);

	memcpy(out, emul_data.last_tx, len);

	return len;
}

void zbook_spi_test_emul_set_read_byte(uint8_t value)
{
	emul_data.read_byte = value;
}

static int zbook_spi_loopback_io(const struct emul *target, const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	struct zbook_spi_loopback_data *data = target->data;

	data->last_config = *config;
	data->last_tx_len = 0;

	if (tx_bufs != NULL) {
		for (size_t i = 0; i < tx_bufs->count; i++) {
			const struct spi_buf *buf = &tx_bufs->buffers[i];
			size_t copy_len = MIN(buf->len, MAX_CAPTURE_LEN - data->last_tx_len);

			memcpy(&data->last_tx[data->last_tx_len], buf->buf, copy_len);
			data->last_tx_len += copy_len;
		}
	}

	if (rx_bufs != NULL) {
		size_t rx_off = 0;

		for (size_t i = 0; i < rx_bufs->count; i++) {
			const struct spi_buf *buf = &rx_bufs->buffers[i];

			for (size_t j = 0; j < buf->len; j++) {
				((uint8_t *)buf->buf)[j] = (tx_bufs != NULL)
								   ? data->last_tx[rx_off + j]
								   : data->read_byte;
			}
			rx_off += buf->len;
		}
	}

	return 0;
}

static int zbook_spi_loopback_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

/*
 * zbook_spi has no production driver of its own (it's a bare bus endpoint,
 * only ever touched via spi_dt_spec) so there is normally no struct device at
 * this node. EMUL_DT_DEFINE()'s .dev = DEVICE_DT_GET(node_id) dereferences it
 * (spi_emul_register() logs ->name), so this no-op device exists purely to
 * satisfy the emulator framework in this test binary.
 */
static int zbook_spi_loopback_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

DEVICE_DT_DEFINE(ZBOOK_SPI_TEST_NODE, zbook_spi_loopback_dev_init, NULL, NULL, NULL, POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static const struct spi_emul_api zbook_spi_loopback_api = {
	.io = zbook_spi_loopback_io,
};

EMUL_DT_DEFINE(ZBOOK_SPI_TEST_NODE, zbook_spi_loopback_init, &emul_data, NULL,
	       &zbook_spi_loopback_api, NULL);
