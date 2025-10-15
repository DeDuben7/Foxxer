/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_3
#define LED_GPIO_Port GPIOE
#define SA818_AUDIO_IN_Pin GPIO_PIN_5
#define SA818_AUDIO_IN_GPIO_Port GPIOA
#define SA818_AUDIO_OUT_Pin GPIO_PIN_6
#define SA818_AUDIO_OUT_GPIO_Port GPIOA
#define SA818_PTT_Pin GPIO_PIN_0
#define SA818_PTT_GPIO_Port GPIOB
#define SA818_PD_Pin GPIO_PIN_1
#define SA818_PD_GPIO_Port GPIOB
#define SA818_HL_Pin GPIO_PIN_2
#define SA818_HL_GPIO_Port GPIOB
#define LCD_CS_Pin GPIO_PIN_11
#define LCD_CS_GPIO_Port GPIOE
#define LCD_WR_RS_Pin GPIO_PIN_13
#define LCD_WR_RS_GPIO_Port GPIOE
#define SA818_UART_TX_Pin GPIO_PIN_10
#define SA818_UART_TX_GPIO_Port GPIOB
#define SA818_UART_RX_Pin GPIO_PIN_11
#define SA818_UART_RX_GPIO_Port GPIOB
#define ATT_05_Pin GPIO_PIN_10
#define ATT_05_GPIO_Port GPIOD
#define ATT_1_Pin GPIO_PIN_11
#define ATT_1_GPIO_Port GPIOD
#define ATT_2_Pin GPIO_PIN_12
#define ATT_2_GPIO_Port GPIOD
#define ATT_4_Pin GPIO_PIN_13
#define ATT_4_GPIO_Port GPIOD
#define ATT_8_Pin GPIO_PIN_14
#define ATT_8_GPIO_Port GPIOD
#define ATT_16_Pin GPIO_PIN_15
#define ATT_16_GPIO_Port GPIOD
#define ROT1CLK_Pin GPIO_PIN_1
#define ROT1CLK_GPIO_Port GPIOD
#define ROT1DAT_Pin GPIO_PIN_2
#define ROT1DAT_GPIO_Port GPIOD
#define ROT1SW_Pin GPIO_PIN_3
#define ROT1SW_GPIO_Port GPIOD
#define ROT2CLK_Pin GPIO_PIN_4
#define ROT2CLK_GPIO_Port GPIOD
#define ROT2DAT_Pin GPIO_PIN_5
#define ROT2DAT_GPIO_Port GPIOD
#define ROT2SW_Pin GPIO_PIN_6
#define ROT2SW_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
