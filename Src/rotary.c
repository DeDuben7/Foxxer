#include "rotary.h"
#include <stdio.h>

// --- Configurable debounce times (ms) ---
#define ROTARY_DEBOUNCE_TIME   5    // rotation debounce
#define BUTTON_DEBOUNCE_TIME   20   // button debounce

// Encoder storage
static rotary_encoder_t encoders[ROTARY_MAX_ENCODERS];
static uint8_t encoder_count = 0;

// --- API implementations ---

void rotary_init(void) {
    encoder_count = 0;
}

int rotary_add(GPIO_TypeDef *port_clk, uint16_t pin_clk,
               GPIO_TypeDef *port_dt,  uint16_t pin_dt,
               GPIO_TypeDef *port_sw,  uint16_t pin_sw)
{
    if (encoder_count >= ROTARY_MAX_ENCODERS) return -1;

    rotary_encoder_t *e = &encoders[encoder_count];

    e->port_clk = port_clk;
    e->pin_clk  = pin_clk;
    e->port_dt  = port_dt;
    e->pin_dt   = pin_dt;
    e->port_sw  = port_sw;
    e->pin_sw   = pin_sw;

    // Read initial states
    e->stable_clk = e->last_stable_clk = HAL_GPIO_ReadPin(port_clk, pin_clk);
    e->stable_dt  = HAL_GPIO_ReadPin(port_dt, pin_dt);
    e->stable_sw  = e->last_stable_sw = HAL_GPIO_ReadPin(port_sw, pin_sw);

    e->last_clk_change = HAL_GetTick();
    e->last_dt_change  = HAL_GetTick();
    e->last_sw_change  = HAL_GetTick();

    encoder_count++;
    return encoder_count - 1; // return index
}

// --- Internal scan for a single encoder ---
static void rotary_scan(rotary_encoder_t *e) {
    uint32_t now = HAL_GetTick();

    // --- Debounce CLK ---
    uint8_t rawCLK = HAL_GPIO_ReadPin(e->port_clk, e->pin_clk);
    if (rawCLK != e->stable_clk) {
        if ((now - e->last_clk_change) > ROTARY_DEBOUNCE_TIME) {
            e->stable_clk = rawCLK;
            e->last_clk_change = now;
        }
    } else {
        e->last_clk_change = now;
    }

    // --- Debounce DT ---
    uint8_t rawDT = HAL_GPIO_ReadPin(e->port_dt, e->pin_dt);
    if (rawDT != e->stable_dt) {
        if ((now - e->last_dt_change) > ROTARY_DEBOUNCE_TIME) {
            e->stable_dt = rawDT;
            e->last_dt_change = now;
        }
    } else {
        e->last_dt_change = now;
    }

    // --- Debounce SW ---
    uint8_t rawSW = HAL_GPIO_ReadPin(e->port_sw, e->pin_sw);
    if (rawSW != e->stable_sw) {
        if ((now - e->last_sw_change) > BUTTON_DEBOUNCE_TIME) {
            e->stable_sw = rawSW;
            e->last_sw_change = now;
        }
    } else {
        e->last_sw_change = now;
    }

    // --- Handle rotation events ---
    if (e->stable_clk != e->last_stable_clk) {
        if (e->stable_dt != e->stable_clk) {
            printf("Encoder %ld: Rotated clockwise\r\n", (long)(e - encoders));
        } else {
            printf("Encoder %ld: Rotated counter-clockwise\r\n", (long)(e - encoders));
        }
    }
    e->last_stable_clk = e->stable_clk;

    // --- Handle button events ---
    if (e->stable_sw != e->last_stable_sw) {
        if (e->stable_sw == GPIO_PIN_RESET) {
            printf("Encoder %ld: Button pressed\r\n", (long)(e - encoders));
        } else {
            printf("Encoder %ld: Button released\r\n", (long)(e - encoders));
        }
    }
    e->last_stable_sw = e->stable_sw;
}

// --- Task: scan all encoders ---
void rotary_task(void) {
    for (uint8_t i = 0; i < encoder_count; i++) {
        rotary_scan(&encoders[i]);
    }
}
