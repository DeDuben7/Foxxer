/**
 ******************************************************************************
 * @file      tim.h
 * @brief     init and control functions for the timers
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
#ifndef __TIM_H
#define __TIM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -------------------------------------------------------------------*/

#include <stdint.h>

/* Defines -------------------------------------------------------------------*/

/* Typedefs -------------------------------------------------------------------*/

/* Functions -------------------------------------------------------------------*/

extern void lcd_brightness_timer_init(void);
extern void lcd_brightness_timer_start(void);
extern void lcd_brightness_timer_set_brightness(int brightness);
extern uint32_t lcd_brightness_timer_get_brightness(void);

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H */
