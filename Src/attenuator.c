#include "stm32h7xx_hal.h"
#include "gpio.h"
#include "attenuator.h"

// internal variable to hold current attenuation
static float current_attenuation_db = 0.0f;

// attenuation step lookup table (value + pin bit)
static const struct {
    float value;
    uint16_t pin;
} att_steps[] = {
    {0.5f,  ATT_05_Pin},
    {1.0f,  ATT_1_Pin},
    {2.0f,  ATT_2_Pin},
    {4.0f,  ATT_4_Pin},
    {8.0f,  ATT_8_Pin},
    {16.0f, ATT_16_Pin},
};

void attenuator_init(void)
{
//    GPIO_InitTypeDef gpio_init = {0};
//
//    gpio_init.Pin = ATT_05_Pin | ATT_1_Pin | ATT_2_Pin |
//                    ATT_4_Pin | ATT_8_Pin | ATT_16_Pin;
//    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
//    gpio_init.Pull = GPIO_NOPULL;
//    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
//
//    HAL_GPIO_Init(ATT_05_GPIO_Port, &gpio_init);

    // default to 0 dB attenuation
    attenuator_set(0.0f);
}

void attenuator_set(float attenuation_db)
{
    if (attenuation_db < ATTENUATOR_MIN_DB)
        attenuation_db = ATTENUATOR_MIN_DB;
    else if (attenuation_db > ATTENUATOR_MAX_DB)
        attenuation_db = ATTENUATOR_MAX_DB;

    current_attenuation_db = attenuation_db;

    // compute pin mask
    uint16_t gpio_mask = 0;
    float remaining = attenuation_db;

    for (int i = 5; i >= 0; i--) {
        if (remaining >= att_steps[i].value) {
            gpio_mask |= att_steps[i].pin;
            remaining -= att_steps[i].value;
        }
    }

    // clear all attenuator pins
    ATT_05_GPIO_Port->BSRR = ((ATT_05_Pin | ATT_1_Pin | ATT_2_Pin |
                               ATT_4_Pin | ATT_8_Pin | ATT_16_Pin) << 16);
    // set pins corresponding to current attenuation
    ATT_05_GPIO_Port->BSRR = gpio_mask;
}

float attenuator_get(void)
{
    return current_attenuation_db;
}
