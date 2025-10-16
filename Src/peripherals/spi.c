/**
 ******************************************************************************
 * @file      spi.c
 * @brief     SPI4 driver with DMA support for LCD display
 * @version   version
 * @author    R. van Renswoude
 * @date      2025
 ******************************************************************************
 */

#include <stdbool.h>

#include "stm32h7xx_hal.h"
#include "spi.h"

/* -------------------------------------------------------------------------- */
/* Private variables                                                          */
/* -------------------------------------------------------------------------- */

SPI_HandleTypeDef display_spi_handle;
DMA_HandleTypeDef hdma_spi4_tx;

static volatile bool spi_tx_busy = false;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

void display_spi_init(void)
{
    /* SPI4 parameter configuration */
    display_spi_handle.Instance = SPI4;
    display_spi_handle.Init.Mode = SPI_MODE_MASTER;
    display_spi_handle.Init.Direction = SPI_DIRECTION_1LINE;
    display_spi_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    display_spi_handle.Init.CLKPolarity = SPI_POLARITY_LOW;
    display_spi_handle.Init.CLKPhase = SPI_PHASE_1EDGE;
    display_spi_handle.Init.NSS = SPI_NSS_SOFT;
    display_spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    display_spi_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    display_spi_handle.Init.TIMode = SPI_TIMODE_DISABLE;
    display_spi_handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    display_spi_handle.Init.CRCPolynomial = 0;
    display_spi_handle.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    display_spi_handle.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    display_spi_handle.Init.IOSwap = SPI_IO_SWAP_DISABLE;

    if (HAL_SPI_Init(&display_spi_handle) != HAL_OK)
    {
        //Error_Handler();
    }
}

/* -------------------------------------------------------------------------- */
/* Blocking SPI transfers                                                     */
/* -------------------------------------------------------------------------- */

uint32_t display_spi_transmit(const uint8_t *data, uint16_t size, uint32_t timeout)
{
    return HAL_SPI_Transmit(&display_spi_handle, (uint8_t *)data, size, timeout);
}

uint32_t display_spi_receive(uint8_t *data, uint16_t size, uint32_t timeout)
{
    return HAL_SPI_Receive(&display_spi_handle, data, size, timeout);
}

/* -------------------------------------------------------------------------- */
/* DMA-based non-blocking transmit                                            */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef display_spi_transmit_dma(const uint8_t *data, uint16_t size)
{
    if (data == NULL || size == 0)
        return HAL_ERROR;

    if (spi_tx_busy)
        return HAL_BUSY;

    spi_tx_busy = true;
    return HAL_SPI_Transmit_DMA(&display_spi_handle, (uint8_t *)data, size);
}

bool display_spi_is_tx_busy(void)
{
    return spi_tx_busy;
}

/* -------------------------------------------------------------------------- */
/* HAL Callbacks                                                              */
/* -------------------------------------------------------------------------- */

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI4)
        spi_tx_busy = false;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI4)
        spi_tx_busy = false;
}

/* -------------------------------------------------------------------------- */
/* MSP Initialization / DeInitialization                                      */
/* -------------------------------------------------------------------------- */

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    if (hspi->Instance == SPI4)
    {
        /* SPI4 Clock Source */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI4;
        PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
        HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_SPI4_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();

        /* SPI4 GPIO Configuration: PE12 -> SCK, PE14 -> MOSI */
        GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_14;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        /* SPI4 TX DMA Init */
        hdma_spi4_tx.Instance = DMA1_Stream4;
        hdma_spi4_tx.Init.Request = DMA_REQUEST_SPI4_TX;
        hdma_spi4_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi4_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi4_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi4_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi4_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi4_tx.Init.Mode = DMA_NORMAL;
        hdma_spi4_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi4_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

        if (HAL_DMA_Init(&hdma_spi4_tx) != HAL_OK)
        {
            //Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmatx, hdma_spi4_tx);
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI4)
    {
        __HAL_RCC_SPI4_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_12 | GPIO_PIN_14);
        HAL_DMA_DeInit(&hdma_spi4_tx);
    }
}
