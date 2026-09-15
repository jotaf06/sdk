/*******************************************************************
 * @file main.c
 *
 * @brief Pure-Zephyr GPIO bit-bang SPI slave for the zbook SPI ping-pong
 * sample. No Zephyr SPI driver for RP2350 supports SPI_OP_MODE_SLAVE
 * (spi_pl022.c and spi_rpi_pico_pio.c both return -ENOTSUP for it), so this
 * shifts bits by hand on GPIO edges instead -- mode 0 (CPOL=0/CPHA=0),
 * MSB-first, matching zbook_master's zbook_spi_cfg. It always keeps "PONG"
 * preloaded and shifts it out on every 4-byte frame, regardless of what it
 * receives.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 15/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#include <errno.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

/* Wiring (jumper wires to the zbook board):
 *   GP2 <- SCK  (zbook GPIO header H1 pin 3,  SPI_SCLK)
 *   GP3 <- MOSI (zbook GPIO header H1 pin 13, SPI_MOSI)
 *   GP8 -> MISO (zbook GPIO header H1 pin 12, SPI_MISO)
 *   GP9 <- CS   (zbook GPIO39, active low -- not on the header, run a
 *                dedicated wire from that pin on the zbook board)
 */
#define SCK_PIN  2U
#define MOSI_PIN 3U
#define MISO_PIN 8U
#define CS_PIN   9U

#define FRAME_LEN     4U
#define FRAME_LEN_BIT (FRAME_LEN * 8U)

static const uint8_t tx_frame[FRAME_LEN] = "PONG";
static volatile uint8_t rx_frame[FRAME_LEN];
static volatile uint8_t rx_shift;
static volatile uint32_t bit_pos;
static volatile bool cs_active;
static volatile bool frame_ready;

static const struct device *gpio_dev;
static struct gpio_callback cs_cb_data;
static struct gpio_callback sck_cb_data;

static void drive_miso_bit(uint32_t pos)
{
	uint32_t byte_idx = pos / 8U;
	uint32_t bit_idx = 7U - (pos % 8U);

	gpio_pin_set(gpio_dev, MISO_PIN, (tx_frame[byte_idx] >> bit_idx) & 0x1U);
}

static void cs_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	if (gpio_pin_get(gpio_dev, CS_PIN) == 1) {
		bit_pos = 0U;
		rx_shift = 0U;
		cs_active = true;
		drive_miso_bit(0U);
	} else {
		cs_active = false;
	}
}

static void sck_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	if (!cs_active || bit_pos >= FRAME_LEN_BIT) {
		return;
	}

	if (gpio_pin_get(gpio_dev, SCK_PIN) == 1) {
		rx_shift = (uint8_t)((rx_shift << 1) | (uint8_t)gpio_pin_get(gpio_dev, MOSI_PIN));
		bit_pos++;

		if (bit_pos % 8U == 0U) {
			rx_frame[(bit_pos / 8U) - 1U] = rx_shift;
			rx_shift = 0U;

			if (bit_pos == FRAME_LEN_BIT) {
				frame_ready = true;
			}
		}
	} else {
		drive_miso_bit(bit_pos);
	}
}

int main(void)
{
	gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

	if (!device_is_ready(gpio_dev)) {
		printk("gpio0 not ready\n");
		return -ENODEV;
	}

	gpio_pin_configure(gpio_dev, SCK_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
	gpio_pin_configure(gpio_dev, MOSI_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
	gpio_pin_configure(gpio_dev, MISO_PIN, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(gpio_dev, CS_PIN, GPIO_INPUT | GPIO_PULL_UP | GPIO_ACTIVE_LOW);

	gpio_init_callback(&cs_cb_data, cs_isr, BIT(CS_PIN));
	gpio_add_callback(gpio_dev, &cs_cb_data);
	gpio_pin_interrupt_configure(gpio_dev, CS_PIN, GPIO_INT_EDGE_BOTH);

	gpio_init_callback(&sck_cb_data, sck_isr, BIT(SCK_PIN));
	gpio_add_callback(gpio_dev, &sck_cb_data);
	gpio_pin_interrupt_configure(gpio_dev, SCK_PIN, GPIO_INT_EDGE_BOTH);

	printk("zbook SPI ping-pong slave: waiting for PING, always answering PONG.\n");

	while (1) {
		if (frame_ready) {
			frame_ready = false;
			printk("got %c%c%c%c -> replied PONG\n", rx_frame[0], rx_frame[1],
			       rx_frame[2], rx_frame[3]);
		}

		k_sleep(K_MSEC(50));
	}
}
