/**
 ******************************************************************************
 * @file      spi.h
 * @brief     SPI4 driver with DMA queue and GPIO control for LCD display
 * @version   2.0
 * @author    R. van Renswoude
 * @date      2025
 ******************************************************************************
 * @details   Provides both blocking and queued DMA SPI transfers.
 *            Handles CS/RS automatically for LCD operations.
 ******************************************************************************
 */

#ifndef __SPI_H
#define __SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

#define SPI_DMA_MAX_PAYLOAD  400

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

void display_spi_init(void);

/* Blocking SPI transfers */
uint32_t display_spi_transmit(const uint8_t *data, uint16_t size, uint32_t timeout);
uint32_t display_spi_receive(uint8_t *data, uint16_t size, uint32_t timeout);

void display_spi_rs_set(void);
void display_spi_rs_reset(void);
void display_spi_cs_set(void);
void display_spi_cs_reset(void);


/* Queued DMA transmit */
void display_spi_enqueue(const uint8_t *data, uint16_t size, bool rs_data, bool release_cs);

/* Helpers */
bool display_spi_is_tx_busy(void);
void display_spi_flush_blocking(void);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H */
