#ifndef ZBOOK_SPI_LOOPBACK_EMUL_H
#define ZBOOK_SPI_LOOPBACK_EMUL_H

#include <stddef.h>
#include <stdint.h>

/** @brief Reset all captured state (last tx, last config, read byte) to zero. */
void zbook_spi_test_emul_reset(void);

/** @brief Bus frequency the controller was asked to use on the last transfer. */
uint32_t zbook_spi_test_emul_last_frequency(void);

/** @brief Operation flags (mode/bit-order/word-size bits) from the last transfer. */
uint32_t zbook_spi_test_emul_last_operation(void);

/**
 * @brief Copy the bytes captured from the last transfer's tx buffer(s).
 *
 * @param out     Destination buffer.
 * @param max_len Capacity of @p out.
 * @return Number of bytes copied.
 */
size_t zbook_spi_test_emul_last_tx(uint8_t *out, size_t max_len);

/** @brief Byte value the emulator stuffs into rx on a read-only (tx == NULL) transfer. */
void zbook_spi_test_emul_set_read_byte(uint8_t value);

#endif /* ZBOOK_SPI_LOOPBACK_EMUL_H */
