/**
 ******************************************************************************
 * @file      gpio.c
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
#include "gpio.h"

/* Defines -------------------------------------------------------------------*/

/* Typedefs -------------------------------------------------------------------*/

/* Variables -------------------------------------------------------------------*/

/* Function prototypes ---------------------------------------------------------*/

/* Functions -------------------------------------------------------------------*/

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void gpio_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LED_Pin|LCD_CS_Pin|LCD_WR_RS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SA818_PTT_Pin|SA818_PD_Pin|SA818_HL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, ATT_05_Pin|ATT_1_Pin|ATT_2_Pin|ATT_4_Pin
                          |ATT_8_Pin|ATT_16_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_Pin LCD_CS_Pin LCD_WR_RS_Pin */
  GPIO_InitStruct.Pin = LED_Pin|LCD_CS_Pin|LCD_WR_RS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : SA818_PTT_Pin SA818_PD_Pin SA818_HL_Pin */
  GPIO_InitStruct.Pin = SA818_PTT_Pin|SA818_PD_Pin|SA818_HL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : ATT_05_Pin ATT_1_Pin ATT_2_Pin ATT_4_Pin
                           ATT_8_Pin ATT_16_Pin */
  GPIO_InitStruct.Pin = ATT_05_Pin|ATT_1_Pin|ATT_2_Pin|ATT_4_Pin
                          |ATT_8_Pin|ATT_16_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : ROT1CLK_Pin ROT1DAT_Pin ROT1SW_Pin ROT2CLK_Pin
                           ROT2DAT_Pin ROT2SW_Pin */
  GPIO_InitStruct.Pin = ROT1CLK_Pin|ROT1DAT_Pin|ROT1SW_Pin|ROT2CLK_Pin
                          |ROT2DAT_Pin|ROT2SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}
