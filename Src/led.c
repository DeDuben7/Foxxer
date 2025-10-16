/**
 ******************************************************************************
 * @file      led.c
 * @brief     LED control and heartbeat task
 * @author    R. van Renswoude
 * @date      2025
 ******************************************************************************
 */

#include "stm32h7xx_hal.h"
#include "led.h"
#include "board.h"   // for board_led_init(), board_led_set()

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static uint32_t led_interval_ms = LED_NORMAL_INTERVAL;  // default blink interval
static uint32_t led_next_toggle = 0;
static bool led_state = false;
static bool led_blink_enabled = true;

// ---------------------------------------------------------------------------
// Public functions
// ---------------------------------------------------------------------------

void led_init(void)
{
    board_led_init();
    led_state = false;
    board_led_set(led_state);
    led_next_toggle = HAL_GetTick() + led_interval_ms;
}

void led_task(void)
{
    if (!led_blink_enabled)
        return;

    uint32_t now = HAL_GetTick();

    if (now >= led_next_toggle)
    {
        led_state = !led_state;
        board_led_set(led_state);
        led_next_toggle = now + led_interval_ms;
    }
}

void led_set(bool on)
{
    led_state = on;
    board_led_set(on);
}

void led_set_interval(uint32_t interval_ms)
{
    if (interval_ms < 50) interval_ms = 50;  // safety clamp
    led_interval_ms = interval_ms;
}

void led_enable_blink(bool enable)
{
    led_blink_enabled = enable;
}
