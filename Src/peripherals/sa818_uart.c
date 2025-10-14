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

void sa818_uart_receive(const char* rxbuf) {
  HAL_UART_Receive(&sa818_uart_handle, (uint8_t*)rxbuf, sizeof(rxbuf), SA818_UART_TIMEOUT);
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
  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      //Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = SA818_UART_TX_Pin|SA818_UART_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
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
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX
    */
    HAL_GPIO_DeInit(GPIOB, SA818_UART_TX_Pin|SA818_UART_RX_Pin);

    /* USART3 interrupt DeInit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  }

}
