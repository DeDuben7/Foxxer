#include <stdio.h>

#include "rotary_encoders.h"
#include "lcd.h"
#include "menu.h"
#include "gpio.h"

// Optional time structure (if you already have RTC code)
extern RTC_TimeTypeDef stimestructureget;

// Internal storage
#define MAX_ROTARIES     4
#define ROT_DEBOUNCE_MS  5
#define BTN_DEBOUNCE_MS 20

static rotary_encoder_t rotaries[MAX_ROTARIES];
static uint8_t rotary_count = 0;

void rotary_add(GPIO_TypeDef *port_clk, uint16_t pin_clk,
                GPIO_TypeDef *port_dt,  uint16_t pin_dt,
                GPIO_TypeDef *port_sw,  uint16_t pin_sw)
{
    if (rotary_count >= MAX_ROTARIES) return;

    rotary_encoder_t *enc = &rotaries[rotary_count++];

    enc->port_clk = port_clk;
    enc->pin_clk  = pin_clk;
    enc->port_dt  = port_dt;
    enc->pin_dt   = pin_dt;
    enc->port_sw  = port_sw;
    enc->pin_sw   = pin_sw;

    enc->stable_clk = HAL_GPIO_ReadPin(port_clk, pin_clk);
    enc->stable_dt  = HAL_GPIO_ReadPin(port_dt,  pin_dt);
    enc->stable_sw  = HAL_GPIO_ReadPin(port_sw,  pin_sw);

    enc->last_clk = enc->stable_clk;
    enc->last_sw  = enc->stable_sw;
    enc->counter  = 0;

    enc->last_clk_change = HAL_GetTick();
    enc->last_dt_change  = HAL_GetTick();
    enc->last_sw_change  = HAL_GetTick();
}

void rotary_init(void)
{
  rotary_count = 0;

  // === Rotary Encoder 1 ===
  rotary_add(ROT1CLK_GPIO_Port, ROT1CLK_Pin,
             ROT1DAT_GPIO_Port, ROT1DAT_Pin,
             ROT1SW_GPIO_Port,  ROT1SW_Pin);

  // === Rotary Encoder 2 ===
  rotary_add(ROT2CLK_GPIO_Port, ROT2CLK_Pin,
             ROT2DAT_GPIO_Port, ROT2DAT_Pin,
             ROT2SW_GPIO_Port,  ROT2SW_Pin);
}

void rotary_task(void)
{
    for (uint8_t i = 0; i < rotary_count; i++) {
        rotary_scan(&rotaries[i]);
    }
}

void rotary_scan(rotary_encoder_t *enc)
{
    uint32_t now = HAL_GetTick();
    uint8_t raw_clk = HAL_GPIO_ReadPin(enc->port_clk, enc->pin_clk);
    uint8_t raw_dt  = HAL_GPIO_ReadPin(enc->port_dt,  enc->pin_dt);
    uint8_t raw_sw  = HAL_GPIO_ReadPin(enc->port_sw,  enc->pin_sw);

    // --- Debounce CLK ---
    if (raw_clk != enc->stable_clk) {
        if ((now - enc->last_clk_change) >= ROT_DEBOUNCE_MS) {
            enc->stable_clk = raw_clk;
        }
    } else {
        enc->last_clk_change = now;
    }

    // --- Debounce DT ---
    if (raw_dt != enc->stable_dt) {
        if ((now - enc->last_dt_change) >= ROT_DEBOUNCE_MS) {
            enc->stable_dt = raw_dt;
        }
    } else {
        enc->last_dt_change = now;
    }

    // --- Debounce SW ---
    if (raw_sw != enc->stable_sw) {
        if ((now - enc->last_sw_change) >= BTN_DEBOUNCE_MS) {
            enc->stable_sw = raw_sw;
        }
    } else {
        enc->last_sw_change = now;
    }

    // --- Handle rotation ---
    if (enc->stable_clk != enc->last_clk) {
        if (enc->stable_dt != enc->stable_clk) {
            enc->counter++;
            enc == &rotaries[0] ? menu_step_through(1) : menu_value_step(1);
        } else {
            enc->counter--;
            enc == &rotaries[0] ? menu_step_through(-1) : menu_value_step(-1);
        }
    }
    enc->last_clk = enc->stable_clk;

    // --- Handle button press (reset counter) ---
    if (enc->stable_sw != enc->last_sw) {
        if (enc->stable_sw == GPIO_PIN_RESET) {
            enc->counter = 0;
            if(enc == &rotaries[0]) {
            	menu_toggle();
            }
        }
    }
    enc->last_sw = enc->stable_sw;
}
