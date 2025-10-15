#include <math.h>

#include "stm32h7xx_hal.h"
#include "test_tone.h"
#include "gpio.h"  // for GPIO definitions if you have one


// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static bool tone_enabled = false;
static uint32_t last_toggle_tick = 0;
static bool pin_state = false;

// Half-period in milliseconds for 500 Hz tone
// Full period = 1 / f = 1 / 500 = 0.002 s = 2 ms
// Half-period = 1 ms
static uint32_t tone_half_period_ms = (uint32_t)(1000.0f / (TESTTONE_FREQUENCY_HZ * 2.0f));

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void testtone_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SA818_AUDIO_IN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SA818_AUDIO_IN_GPIO_Port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SA818_AUDIO_IN_GPIO_Port, SA818_AUDIO_IN_Pin, GPIO_PIN_RESET);
    tone_enabled = false;
    pin_state = false;
    last_toggle_tick = HAL_GetTick();
}

void testtone_enable(bool enable)
{
    tone_enabled = enable;

    if (!enable) {
        // Ensure pin is low when tone disabled
        HAL_GPIO_WritePin(SA818_AUDIO_IN_GPIO_Port, SA818_AUDIO_IN_Pin, GPIO_PIN_RESET);
        pin_state = false;
    }
}

bool testtone_is_enabled(void)
{
    return tone_enabled;
}

void testtone_task(void)
{
    if (!tone_enabled)
        return;

    uint32_t now = HAL_GetTick();

    if ((now - last_toggle_tick) >= tone_half_period_ms)
    {
        last_toggle_tick = now;
        pin_state = !pin_state;
        HAL_GPIO_WritePin(SA818_AUDIO_IN_GPIO_Port, SA818_AUDIO_IN_Pin, pin_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}
