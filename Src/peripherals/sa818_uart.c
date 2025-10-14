/**
 ******************************************************************************
 * @file      sa818_uart.c
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

#include <string.h>

#include "stm32h7xx_hal.h"
#include "gpio.h"

/* Defines -------------------------------------------------------------------*/

#define SA818_UART_TIMEOUT (200)

/* Typedefs -------------------------------------------------------------------*/

/* Variables -------------------------------------------------------------------*/

UART_HandleTypeDef sa818_uart_handle;

/* Function prototypes ---------------------------------------------------------*/

/* Functions -------------------------------------------------------------------*/

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
void sa818_uart_init(void)
{
  sa818_uart_handle.Instance = USART3;
  sa818_uart_handle.Init.BaudRate = 9600;
  sa818_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
  sa818_uart_handle.Init.StopBits = UART_STOPBITS_1;
  sa818_uart_handle.Init.Parity = UART_PARITY_NONE;
  sa818_uart_handle.Init.Mode = UART_MODE_TX_RX;
  sa818_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  sa818_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;
  sa818_uart_handle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  sa818_uart_handle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  sa818_uart_handle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&sa818_uart_handle) != HAL_OK)
  {
    //Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&sa818_uart_handle, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    //Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&sa818_uart_handle, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    //Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&sa818_uart_handle) != HAL_OK)
  {
    //Error_Handler();
  }
}

void sa818_uart_transmit(const char* cmd) {
  HAL_UART_Transmit(&sa818_uart_handle, (uint8_t*)cmd, strlen(cmd), SA818_UART_TIMEOUT);
}

size_t sa818_uart_receive(char *rxbuf, size_t size, uint32_t timeout)
{
    uint16_t rx_len = 0;
    HAL_StatusTypeDef status;

    memset(rxbuf, 0, size);

    status = HAL_UARTEx_ReceiveToIdle(&sa818_uart_handle,
                                      (uint8_t *)rxbuf,
                                      size,
                                      &rx_len,
                                      timeout);

    if (status == HAL_OK || status == HAL_TIMEOUT) {
        return rx_len;  // number of bytes actually received
    } else {
        return 0;
    }
}

void sa818_uart_flush(void)
{
    __HAL_UART_CLEAR_OREFLAG(&sa818_uart_handle);  // clear overrun
    __HAL_UART_CLEAR_NEFLAG(&sa818_uart_handle);   // clear noise flag
    __HAL_UART_CLEAR_FEFLAG(&sa818_uart_handle);   // clear framing error
    __HAL_UART_CLEAR_PEFLAG(&sa818_uart_handle);   // clear parity error

    // Read out any pending bytes in RX FIFO
    uint8_t dummy;
    while (__HAL_UART_GET_FLAG(&sa818_uart_handle, UART_FLAG_RXNE) != RESET) {
        dummy = (uint8_t)(sa818_uart_handle.Instance->RDR);
        (void)dummy;
    }
}

/**
  * @brief UART MSP Initialization
  * This function configures the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(huart->Instance==USART3)
  {
    // Initializes the peripherals clock
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      //Error_Handler();
    }

    // Peripheral clock enable
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    //PB10     ------> USART3_TX
    //PB11     ------> USART3_RX
    GPIO_InitStruct.Pin = SA818_UART_TX_Pin|SA818_UART_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

/**
  * @brief UART MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance==USART3)
  {
    // Peripheral clock disable
    __HAL_RCC_USART3_CLK_DISABLE();

    //PB10     ------> USART3_TX
    //PB11     ------> USART3_RX
    HAL_GPIO_DeInit(GPIOB, SA818_UART_TX_Pin|SA818_UART_RX_Pin);
  }
}
