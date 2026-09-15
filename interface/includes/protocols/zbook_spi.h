/*******************************************************************
 * @file zbook_spi.h
 *
 * @brief Defines the interface for the zbook SPI bus.
 * @author José Félix de Oliveira Neto (josefelix.neto@edge.ufal.br)
 * @version 0.1
 * @date 14/09/26
 *
 * @copyright Copyright (c) 2026
 *
 *******************************************************************/

#ifndef ZBOOK_SPI_H
#define ZBOOK_SPI_H

#include <stddef.h>
#include <stdint.h>

/** @brief SPI clock polarity/phase combination (standard SPI modes 0-3). */
enum zbook_spi_mode {
	ZBOOK_SPI_MODE_0 = 0, /**< CPOL=0, CPHA=0 */
	ZBOOK_SPI_MODE_1,     /**< CPOL=0, CPHA=1 */
	ZBOOK_SPI_MODE_2,     /**< CPOL=1, CPHA=0 */
	ZBOOK_SPI_MODE_3,     /**< CPOL=1, CPHA=1 */
};

/** @brief Bit order used to shift each word over the wire. */
enum zbook_spi_bit_order {
	ZBOOK_SPI_MSB_FIRST = 0,
	ZBOOK_SPI_LSB_FIRST,
};

/** @brief Runtime-configurable zbook SPI bus parameters. */
struct zbook_spi_cfg {
	uint32_t frequency;                 /**< Clock frequency in Hz. */
	enum zbook_spi_mode mode;           /**< CPOL/CPHA combination. */
	enum zbook_spi_bit_order bit_order; /**< MSB or LSB first. */
	uint8_t word_size;                  /**< Bits per word (e.g. 8). */
};

/**
 * @brief Check that the zbook SPI bus is ready to use.
 *
 * @return 0 on success, -errno on error.
 */
int zbook_spi_init(void);

/**
 * @brief Reconfigure the zbook SPI bus (frequency, mode, bit order, word size).
 *
 * @param cfg Desired configuration. All fields are required.
 * @return 0 on success, -errno on error.
 */
int zbook_spi_configure(const struct zbook_spi_cfg *cfg);

/**
 * @brief Exchange @p len bytes over the zbook SPI bus in a single transaction
 * (one CS assertion).
 *
 * At least one of @p tx / @p rx must be non-NULL. Passing NULL for @p tx
 * sends dummy bytes; passing NULL for @p rx discards the received bytes.
 *
 * @param tx  Buffer to send, or NULL to send dummy bytes.
 * @param rx  Buffer to receive into, or NULL to discard received bytes.
 * @param len Number of bytes to exchange.
 * @return 0 on success, -errno on error.
 */
int zbook_spi_transceive(const uint8_t *tx, uint8_t *rx, size_t len);

/**
 * @brief Write @p len bytes from @p data over the zbook SPI bus.
 *
 * @param data Buffer to send.
 * @param len  Number of bytes to send.
 * @return 0 on success, -errno on error.
 */
int zbook_spi_write(const uint8_t *data, size_t len);

/**
 * @brief Read @p len bytes into @p data from the zbook SPI bus.
 *
 * @param data Buffer to receive into.
 * @param len  Number of bytes to receive.
 * @return 0 on success, -errno on error.
 */
int zbook_spi_read(uint8_t *data, size_t len);

#endif /* ZBOOK_SPI_H */