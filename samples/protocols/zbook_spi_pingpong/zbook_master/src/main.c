/*******************************************************************
 * @file main.c
 *
 * @brief Real-hardware bring-up sample for the zbook SPI interface: sends
 * "PING" every 3 seconds to a second SPI-slave board and checks the reply
 * is "PONG". The slave is a plain GPIO bit-bang implementation (see
 * samples/spi_pingpong/rp2350a_slave/) since no Zephyr SPI driver for
 * RP2350 supports peripheral/slave mode.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 15/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "protocols/zbook_spi.h"

#define SPI_PINGPONG_LEN    4U
#define SPI_PINGPONG_PERIOD K_MSEC(3000)

static const uint8_t ping[SPI_PINGPONG_LEN] = "PING";
static const uint8_t pong[SPI_PINGPONG_LEN] = "PONG";
static uint8_t rx_buf[SPI_PINGPONG_LEN];

int main(void)
{
	int ret = zbook_spi_init();

	if (ret != 0) {
		printk("zbook_spi_init failed: %d\n", ret);
		return ret;
	}

	/* 10 kHz: slow enough for the slave's GPIO-interrupt bit-bang to keep up
	 * (no Zephyr SPI driver on RP2350 supports SPI_OP_MODE_SLAVE, so the
	 * other side can't use hardware SPI at all).
	 */
	const struct zbook_spi_cfg cfg = {
		.frequency = 10000U,
		.mode = ZBOOK_SPI_MODE_0,
		.bit_order = ZBOOK_SPI_MSB_FIRST,
		.word_size = 8U,
	};

	ret = zbook_spi_configure(&cfg);
	if (ret != 0) {
		printk("zbook_spi_configure failed: %d\n", ret);
		return ret;
	}

	printk("zbook SPI ping-pong sample: sending PING every 3s, watch this console.\n");

	while (1) {
		memset(rx_buf, 0, SPI_PINGPONG_LEN);

		ret = zbook_spi_transceive(ping, rx_buf, SPI_PINGPONG_LEN);
		if (ret != 0) {
			printk("transceive failed: %d\n", ret);
		} else if (memcmp(rx_buf, pong, SPI_PINGPONG_LEN) == 0) {
			printk("PING -> PONG  (slave alive)\n");
		} else {
			printk("PING -> %02x %02x %02x %02x  (no/garbage reply)\n", rx_buf[0],
			       rx_buf[1], rx_buf[2], rx_buf[3]);
		}

		k_sleep(SPI_PINGPONG_PERIOD);
	}
}
