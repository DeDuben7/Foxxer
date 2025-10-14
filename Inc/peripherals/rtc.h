/**
 ******************************************************************************
 * @file      rtc.h
 * @brief     init and control functions for the RTC
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
#ifndef __RTC_H
#define __RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -------------------------------------------------------------------*/

#include "stm32h7xx_hal.h"

/* Defines -------------------------------------------------------------------*/

/* Typedefs -------------------------------------------------------------------*/

/* Functions -------------------------------------------------------------------*/

extern void rtc_init(void);
extern void rtc_read_date_time(RTC_DateTypeDef *sdatestructureget,RTC_TimeTypeDef *stimestructureget);

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H */
