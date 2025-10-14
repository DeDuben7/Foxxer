/**
 ******************************************************************************
 * @file      sa818.h
 * @brief     init and control functions for the ADC
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
 * @addtogroup  sa818
 * @{
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SA818_H
#define __SA818_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes -------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

#include "stm32h7xx_hal.h"
#include "gpio.h"

/* Defines -------------------------------------------------------------------*/

/* Typedefs -------------------------------------------------------------------*/

// SA818 command result
typedef enum {
    SA818_OK = 0,
    SA818_ERROR,
    SA818_TIMEOUT
} sa818_status_t;

// Operating parameters
typedef struct {
    uint8_t bandwidth;      // 0=12.5kHz, 1=25kHz
    float tx_freq;          // MHz
    float rx_freq;          // MHz
    char tx_subaudio[5];    // e.g. "0000", "023I", "0012"
    uint8_t squelch;        // 0–8
    char rx_subaudio[5];    // e.g. "0000"
} sa818_group_t;

/* Functions -------------------------------------------------------------------*/

extern sa818_status_t sa818_init();
extern sa818_status_t sa818_handshake();
extern sa818_status_t sa818_set_volume(uint8_t volume);
extern sa818_status_t sa818_set_filter(uint8_t preemph, uint8_t highpass, uint8_t lowpass);
extern sa818_status_t sa818_set_tail(uint8_t tail_on);
extern sa818_status_t sa818_read_rssi(uint8_t *rssi);
extern sa818_status_t sa818_read_version(char *version, uint16_t maxlen);

extern void sa818_set_power_on_off(pin_level_t level);
extern void sa818_set_ptt_level(pin_level_t level);
extern void sa818_set_low_power(void);
extern void sa818_set_high_power(void);

#ifdef __cplusplus
}
#endif

#endif /* __SA818_H */
