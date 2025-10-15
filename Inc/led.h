/**
 ******************************************************************************
 * @file      led.h
 * @brief     LED control and heartbeat task
 * @author    R. van Renswoude
 * @date      2025
 ******************************************************************************
 */

#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "stm32h7xx_hal.h"

/**
 * @brief Initialize LED hardware (calls board_led_init internally)
 */
void led_init(void);

/**
 * @brief Cooperative LED task — toggles LED based on interval
 * @note  Should be called regularly from the main loop.
 */
void led_task(void);

/**
 * @brief Manually set LED on/off state
 */
void led_set(bool on);

/**
 * @brief Change blinking interval (ms)
 */
void led_set_interval(uint32_t interval_ms);

/**
 * @brief Enable or disable blinking
 */
void led_enable_blink(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */
