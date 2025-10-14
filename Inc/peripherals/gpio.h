/**
 ******************************************************************************
 * @file      gpio.h
 * @brief     init functions for gpio
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H
#define __GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -------------------------------------------------------------------*/

/* Defines -------------------------------------------------------------------*/

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

/* Typedefs -------------------------------------------------------------------*/

typedef enum {
    LOW = 0,
    HIGH,
} pin_level_t;

/* Functions -------------------------------------------------------------------*/

extern void gpio_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_H */
