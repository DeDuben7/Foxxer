#ifndef ROTARY_H
#define ROTARY_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

// ===== STRUCT DEFINITION =====
typedef struct {
    GPIO_TypeDef *port_clk;
    uint16_t      pin_clk;

    GPIO_TypeDef *port_dt;
    uint16_t      pin_dt;

    GPIO_TypeDef *port_sw;
    uint16_t      pin_sw;

    // Debounced stable states
    uint8_t stable_clk;
    uint8_t stable_dt;
    uint8_t stable_sw;

    // Last stable states (for edge detection)
    uint8_t last_clk;
    uint8_t last_sw;

    // Timestamp tracking (for debounce)
    uint32_t last_clk_change;
    uint32_t last_dt_change;
    uint32_t last_sw_change;

    // Optional counter (increment/decrement on rotation)
    int32_t counter;
} rotary_encoder_t;

// ===== API =====
void rotary_init(void);
void rotary_add(GPIO_TypeDef *port_clk, uint16_t pin_clk,
                GPIO_TypeDef *port_dt,  uint16_t pin_dt,
                GPIO_TypeDef *port_sw,  uint16_t pin_sw);
void rotary_task(void);
void rotary_scan(rotary_encoder_t *enc);

#endif // ROTARY_H
