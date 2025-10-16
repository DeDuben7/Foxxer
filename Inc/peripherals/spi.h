/**
 ******************************************************************************
 * @file      spi.h
 * @brief     SPI4 driver with DMA support for LCD display
 * @version   version
 * @author    R. van Renswoude
 * @date      2025
 ******************************************************************************
 * @details   Provides blocking and DMA-based SPI transmit functions for LCD.
 ******************************************************************************
 */

#ifndef __SPI_H
#define __SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize SPI4 peripheral and its DMA channel.
 */
void display_spi_init(void);

/**
 * @brief Transmit data over SPI (blocking).
 */
uint32_t display_spi_transmit(const uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * @brief Receive data over SPI (blocking).
 */
uint32_t display_spi_receive(uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * @brief Start non-blocking DMA transmit.
 * @return HAL_OK if started, HAL_BUSY if ongoing, HAL_ERROR on failure.
 */
HAL_StatusTypeDef display_spi_transmit_dma(const uint8_t *data, uint16_t size);

/**
 * @brief Check if SPI TX DMA is currently busy.
 * @return true if transfer still ongoing, false if idle.
 */
bool display_spi_is_tx_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H */
