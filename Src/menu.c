#include <stdio.h>
#include <string.h>

#include "stm32h7xx_hal.h"
#include "menu.h"
#include "lcd.h"

#define MENU_COMMIT_DELAY_MS     200   // inactivity before commit
#define MENU_REDRAW_INTERVAL_MS  100   // minimum time between redraws

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static menu_item_t current_menu = menu_item_brightness;

static uint32_t last_value_change_time = 0;
static uint32_t last_draw_time = 0;

static uint8_t local_value = 0;           // local change pending commit
static uint8_t update_display_async = 1;  // redraw requested

static uint8_t menu_line_y[4] = {4, 22, 40, 58};
static uint8_t menu_text_x = 4;

// Example menu data
static int brightness = 50;  // 0–100
static int volume = 5;       // 0–10
static int mode = 0;         // 0=Off, 1=On

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static const char* menu_brightness_get_value(void);
static const char* menu_volume_get_value(void);
static const char* menu_mode_get_value(void);

static void menu_brightness_step_value(int step);
static void menu_volume_step_value(int step);
static void menu_mode_step_value(int step);

typedef struct {
    const char* name;
    const char* (*get_value)(void);
    void (*step_value)(int step);
} menu_descriptor_t;

static const menu_descriptor_t menu_table[menu_item_count] = {
    { "Brightness", menu_brightness_get_value, menu_brightness_step_value },
    { "Volume",     menu_volume_get_value,     menu_volume_step_value     },
    { "Mode",       menu_mode_get_value,       menu_mode_step_value       },
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void menu_init(void)
{
    current_menu = menu_item_brightness;
    last_value_change_time = 0;
    local_value = 0;
    update_display_async = 1;
}

void menu_step_through(int step)
{
    int next = (int)current_menu + step;
    if (next < 0)
        next = menu_item_count - 1;
    if (next >= menu_item_count)
        next = 0;

    current_menu = (menu_item_t)next;
    update_display_async = 1;
}

void menu_value_step(int step)
{
    if (step == 0)
        return;

    menu_table[current_menu].step_value(step);
    local_value = 1;
    update_display_async = 1;
    last_value_change_time = HAL_GetTick();
}

const char* menu_get_value(void)
{
    return menu_table[current_menu].get_value();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
static void menu_draw(void)
{
  uint8_t text[16];
  lcd_clear();
  strncpy((char*)text, menu_table[current_menu].name, sizeof(text));
  lcd_show_string(menu_text_x, menu_line_y[0], lcd_get_width(), 16, 16, text);
  snprintf((char*)text, sizeof(text), "%s", menu_get_value());
  lcd_show_string(menu_text_x, menu_line_y[1], lcd_get_width(), 16, 16, text);
}

static void menu_on_value_committed(void)
{
    printf("[commit] %s = %s\n",
           menu_table[current_menu].name,
           menu_get_value());

    // Example UART output
    // char msg[32];
    // snprintf(msg, sizeof(msg), "%s=%s\r\n",
    //          menu_table[current_menu].name, menu_get_value());
    // HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

// ---------------------------------------------------------------------------
// Cooperative task
// ---------------------------------------------------------------------------
void menu_task(void)
{
    uint32_t now = HAL_GetTick();

    // Handle deferred commit
    if (local_value && (now - last_value_change_time >= MENU_COMMIT_DELAY_MS)) {
        local_value = 0;
        update_display_async = 1;
        menu_on_value_committed();
    }

    // Handle deferred display update
    if (update_display_async && (now - last_draw_time >= MENU_REDRAW_INTERVAL_MS)) {
        menu_draw();
        last_draw_time = now;
        update_display_async = 0;
    }
}

// ---------------------------------------------------------------------------
// Menu item logic
// ---------------------------------------------------------------------------
static const char* menu_brightness_get_value(void)
{
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", brightness);
    return buf;
}

static void menu_brightness_step_value(int step)
{
    brightness += step * 5;
    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;
}

static const char* menu_volume_get_value(void)
{
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d", volume);
    return buf;
}

static void menu_volume_step_value(int step)
{
    volume += step;
    if (volume < 0) volume = 0;
    if (volume > 10) volume = 10;
}

static const char* menu_mode_get_value(void)
{
    static const char* modes[] = {"Off", "On"};
    return modes[mode];
}

static void menu_mode_step_value(int step)
{
    mode += step;
    if (mode < 0) mode = 1;
    if (mode > 1) mode = 0;
}
