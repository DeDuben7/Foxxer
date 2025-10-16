#include <string.h>
#include <stdio.h>

#include "error_window.h"
#include "lcd.h"
#include "stm32h7xx_hal.h"

#define LCD_FONT_SIZE        16
#define LCD_LINE_SPACING     18

void error_window_show(const char *message, uint32_t delay_ms)
{
    if (!message) return;
    if (delay_ms == 0) delay_ms = 1000;

    // --- Draw blocking error overlay ---
    lcd_clear();

    // Optional: draw red border or box
    uint16_t width = lcd_get_width();
    uint16_t height = lcd_get_height();

    // Red rectangle border
    lcd_draw_rect(0, 0, width - 1, height - 1, RED);

    // Title
    lcd_show_string_centered(0, 20, width, LCD_FONT_SIZE,
                             LCD_FONT_SIZE, (uint8_t *)"ERROR");

    // Message text
    lcd_show_string_centered(0, height / 2 - 8, width, LCD_FONT_SIZE,
                             LCD_FONT_SIZE, (uint8_t *)message);

    // Optional: blinking “please wait” line
    lcd_show_string_centered(0, height - 24, width, LCD_FONT_SIZE,
                             LCD_FONT_SIZE, (uint8_t *)"System Halted");

    // --- Blocking delay ---
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < delay_ms)
    {
        // If you want, add a LED blink here to indicate life
    }

    // Clear display after timeout
    lcd_clear();
}
