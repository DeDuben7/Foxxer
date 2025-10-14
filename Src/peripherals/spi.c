/**
 ******************************************************************************
 * @file      spi.c
 * @brief     short description
 * @version   version
 * @author    R. van Renswoude
 * @date      2025
 ******************************************************************************
 * @details detailed description
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2025 Ruben van Renswoude.
 * All rights reserved.</center></h2>
 *
 ******************************************************************************
 */
/**
 * @addtogroup  Peripherals
 * @{
 */


/* Includes -------------------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "spi.h"

/* Defines -------------------------------------------------------------------*/

/* Typedefs -------------------------------------------------------------------*/

/* Variables -------------------------------------------------------------------*/

SPI_HandleTypeDef display_spi_handle;

/* Function prototypes ---------------------------------------------------------*/

/* Functions -------------------------------------------------------------------*/

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
void display_spi_init(void)
{
  /* SPI4 parameter configuration*/
  // PE12 -> SPI4_SCK
  // PE14 -> SPI4_MOSI
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
  display_spi_handle.Init.CRCPolynomial = 0x0;
  display_spi_handle.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  display_spi_handle.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  display_spi_handle.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  display_spi_handle.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  display_spi_handle.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  display_spi_handle.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  display_spi_handle.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  display_spi_handle.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  display_spi_handle.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  display_spi_handle.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&display_spi_handle) != HAL_OK)
  {
    //Error_Handler();
  }
}

uint32_t display_spi_transmit(const uint8_t *data, uint16_t size, uint32_t timeout) {
  return HAL_SPI_Transmit(&display_spi_handle, data, size, timeout);
}

uint32_t display_spi_receive(uint8_t *data, uint16_t size, uint32_t timeout) {
  return HAL_SPI_Receive(&display_spi_handle, data, size, timeout);
}

/**
  * @brief SPI MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(hspi->Instance==SPI4)
  {
    //Initializes the peripherals clock
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI4;
    PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      //Error_Handler();
    }

    __HAL_RCC_SPI4_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  }
}

/**
  * @brief SPI MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
  if(hspi->Instance==SPI4)
  {
    __HAL_RCC_SPI4_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_12|GPIO_PIN_14);
  }
}
