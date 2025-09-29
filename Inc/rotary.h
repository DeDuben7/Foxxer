#ifndef ROTARY_H
#define ROTARY_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

// --- Rotary encoder struct ---
typedef struct {
    GPIO_TypeDef *port_clk;
    uint16_t      pin_clk;

    GPIO_TypeDef *port_dt;
    uint16_t      pin_dt;

    GPIO_TypeDef *port_sw;
    uint16_t      pin_sw;

    // Debounced states
    uint8_t stable_clk;
    uint8_t last_stable_clk;

    uint8_t stable_dt;

    uint8_t stable_sw;
    uint8_t last_stable_sw;

    // Timing
    uint32_t last_clk_change;
    uint32_t last_dt_change;
    uint32_t last_sw_change;
} rotary_encoder_t;

#define ROTARY_MAX_ENCODERS   4   // adjust as needed

// --- API ---
void rotary_init(void);
int  rotary_add(GPIO_TypeDef *port_clk, uint16_t pin_clk,
                GPIO_TypeDef *port_dt,  uint16_t pin_dt,
                GPIO_TypeDef *port_sw,  uint16_t pin_sw);
void rotary_task(void);

#endif // ROTARY_H
