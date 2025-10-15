#include "menu.h"
#include "lcd.h"
#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
#define MENU_COMMIT_DELAY_MS     200   // inactivity before commit
#define MENU_REDRAW_INTERVAL_MS  100   // minimum time between redraws
#define MENU_VISIBLE_LINES       4

#define LCD_FONT_SIZE            16
#define LCD_LINE_SPACING         18

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
typedef enum {
    ui_state_home = 0,
    ui_state_menu
} ui_state_t;

static ui_state_t ui_state = ui_state_home;
static menu_item_t current_menu = menu_item_brightness;
static uint8_t top_menu_index = 0; // first visible menu line

static uint32_t last_value_change_time = 0;
static uint32_t last_draw_time = 0;

static uint8_t local_value = 0;          // uncommitted local change
static uint8_t update_display_async = 1; // schedule redraw

// home screen data
static char sa818_mode[8] = "RX";
static char sa818_freq[16] = "145.500";
static int sa818_rssi = -120;
static char attenuator_text[16] = "Atten: 10dB";

// ---------------------------------------------------------------------------
// Example menu data
// ---------------------------------------------------------------------------
static int brightness = 50;  // 0–100
static int volume = 5;       // 0–10
static int mode = 0;         // 0=Off, 1=On

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void draw_home_screen(void);
static void draw_menu_screen(void);
static void draw_scrollbar(uint8_t top, uint8_t total, uint8_t visible);
static void menu_on_value_committed(void);

// menu item logic
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
    top_menu_index = 0;
    last_value_change_time = 0;
    local_value = 0;
    update_display_async = 1;
    ui_state = ui_state_home;
}

void menu_toggle(void)
{
    ui_state = (ui_state == ui_state_home) ? ui_state_menu : ui_state_home;
    update_display_async = 1;
}

void menu_step_through(int step)
{
    if (ui_state != ui_state_menu) return;

    int next = (int)current_menu + step;
    if (next < 0)
        next = menu_item_count - 1;
    if (next >= menu_item_count)
        next = 0;

    current_menu = (menu_item_t)next;

    // scroll window
    if (current_menu < top_menu_index)
        top_menu_index = current_menu;
    else if (current_menu >= top_menu_index + MENU_VISIBLE_LINES)
        top_menu_index = current_menu - MENU_VISIBLE_LINES + 1;

    update_display_async = 1;
}

void menu_value_step(int step)
{
    if (ui_state != ui_state_menu || step == 0)
        return;

    menu_table[current_menu].step_value(step);
    local_value = 1;
    update_display_async = 1;
    last_value_change_time = HAL_GetTick();
}

void menu_set_sa818_info(const char* mode, const char* freq, int rssi, const char* atten)
{
    strncpy(sa818_mode, mode, sizeof(sa818_mode));
    strncpy(sa818_freq, freq, sizeof(sa818_freq));
    sa818_rssi = rssi;
    strncpy(attenuator_text, atten, sizeof(attenuator_text));
    update_display_async = 1;
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
        if (ui_state == ui_state_home)
            draw_home_screen();
        else
            draw_menu_screen();

        last_draw_time = now;
        update_display_async = 0;
    }
}

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------
static void draw_home_screen(void)
{
    char line[32];
    lcd_clear();

    snprintf(line, sizeof(line), "SA818: %s", attenuator_text);
    lcd_show_string(4, 4, lcd_get_width(), 16, LCD_FONT_SIZE, (uint8_t*)line);

    snprintf(line, sizeof(line), "%s %s MHz", sa818_mode, sa818_freq);
    lcd_show_string(4, 22, lcd_get_width(), 16, LCD_FONT_SIZE, (uint8_t*)line);

    snprintf(line, sizeof(line), "RSSI: %d dBm", sa818_rssi);
    lcd_show_string(4, 40, lcd_get_width(), 16, LCD_FONT_SIZE, (uint8_t*)line);
}

static void draw_menu_screen(void)
{
    lcd_clear();

    for (uint8_t i = 0; i < MENU_VISIBLE_LINES; i++) {
        uint8_t index = top_menu_index + i;
        if (index >= menu_item_count)
            break;

        uint16_t y = 4 + i * LCD_LINE_SPACING;
        uint8_t text[32];

        uint16_t item_color = WHITE;
        uint16_t bg_color   = BLACK;

        // Selected line highlight
        if (index == current_menu) {
            bg_color = BLUE;       // highlight background
            item_color = WHITE;    // white text on blue
            lcd_draw_filled_rect(0, y - 2, lcd_get_width(), LCD_LINE_SPACING, bg_color);
        }

        // Draw menu name
        POINT_COLOR = item_color;
        BACK_COLOR  = bg_color;
        snprintf((char*)text, sizeof(text), "%s", menu_table[index].name);
        lcd_show_string(4, y, lcd_get_width() - 20, 16, LCD_FONT_SIZE, text);

        // Draw menu value
        snprintf((char*)text, sizeof(text), "%s", menu_table[index].get_value());
        lcd_show_string(100, y, lcd_get_width(), 16, LCD_FONT_SIZE, text);
    }

    draw_scrollbar(top_menu_index, menu_item_count, MENU_VISIBLE_LINES);
}

static void draw_scrollbar(uint8_t top, uint8_t total, uint8_t visible)
{
    if (total <= visible) return;
    uint16_t bar_x = lcd_get_width() - 4;
    uint16_t bar_height = lcd_get_height() - 4;
    uint16_t bar_y = 2;

    float ratio = (float)visible / (float)total;
    float pos = (float)top / (float)total;

    uint16_t thumb_height = (uint16_t)(bar_height * ratio);
    uint16_t thumb_y = (uint16_t)(bar_y + bar_height * pos);

    lcd_draw_filled_rect(bar_x, bar_y, 2, bar_height, GRAY);
    lcd_draw_filled_rect(bar_x, thumb_y, 2, thumb_height, WHITE);
}

// ---------------------------------------------------------------------------
// Commit callback
// ---------------------------------------------------------------------------
static void menu_on_value_committed(void)
{
    printf("[commit] %s = %s\n",
           menu_table[current_menu].name,
           menu_table[current_menu].get_value());
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
