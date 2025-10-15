#ifndef TESTTONE_H
#define TESTTONE_H

#include <stdbool.h>
#include "stm32h7xx_hal.h"

// Default tone frequency (in Hz)
#define TESTTONE_FREQUENCY_HZ  500.0f

// Initialize the PA5 pin as output
void testtone_init(void);

// Enable or disable the tone generator
void testtone_enable(bool enable);

// Check if tone is active
bool testtone_is_enabled(void);

// Periodic task to be called from main loop
void testtone_task(void);

#endif // TESTTONE_H
