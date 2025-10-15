#include <stdio.h>
#include <string.h>

#include "stm32h7xx_hal.h"

#include "menu.h"
#include "lcd.h"
#include "sa818.h"
#include "attenuator.h"



// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
#define MENU_COMMIT_DELAY_MS     200
#define MENU_REDRAW_INTERVAL_MS  100
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
static menu_item_t current_menu = menu_item_bandwidth;
static uint8_t top_menu_index = 0;

static uint32_t last_value_change_time = 0;
static uint32_t last_draw_time = 0;

static uint8_t local_value = 0;
static uint8_t update_display_async = 1;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void draw_home_screen(void);
static void draw_menu_screen(void);
static void draw_scrollbar(uint8_t top, uint8_t total, uint8_t visible);
static void menu_commit_if_pending(void);
static void menu_on_value_committed(void);

// SA818 menu handlers
static const char* menu_atten_get_value(void);
static void menu_atten_step_value(int step);

static const char* menu_bandwidth_get_value(void);
static void menu_bandwidth_step_value(int step);

static const char* menu_tx_freq_get_value(void);
static void menu_tx_freq_step_value(int step);

static const char* menu_rx_freq_get_value(void);
static void menu_rx_freq_step_value(int step);

static const char* menu_tx_sub_get_value(void);
static void menu_tx_sub_step_value(int step);

static const char* menu_rx_sub_get_value(void);
static void menu_rx_sub_step_value(int step);

static const char* menu_squelch_get_value(void);
static void menu_squelch_step_value(int step);

static const char* menu_volume_get_value(void);
static void menu_volume_step_value(int step);

static const char* menu_pre_get_value(void);
static void menu_pre_step_value(int step);

static const char* menu_high_get_value(void);
static void menu_high_step_value(int step);

static const char* menu_low_get_value(void);
static void menu_low_step_value(int step);

static const char* menu_tail_get_value(void);
static void menu_tail_step_value(int step);

static const char* menu_mode_get_value(void);
static void menu_mode_step_value(int step);

static const char* menu_power_get_value(void);
static void menu_power_step_value(int step);

// ---------------------------------------------------------------------------
// Menu table
// ---------------------------------------------------------------------------
typedef struct {
    const char* name;
    const char* (*get_value)(void);
    void (*step_value)(int step);
} menu_descriptor_t;

static const menu_descriptor_t menu_table[menu_item_count] = {
    { "Attenuator", menu_atten_get_value, menu_atten_step_value }, // NEW
    { "Bandwidth",  menu_bandwidth_get_value, menu_bandwidth_step_value },
    { "TX Freq",    menu_tx_freq_get_value,   menu_tx_freq_step_value   },
    { "RX Freq",    menu_rx_freq_get_value,   menu_rx_freq_step_value   },
    { "TX Sub",     menu_tx_sub_get_value,    menu_tx_sub_step_value    },
    { "RX Sub",     menu_rx_sub_get_value,    menu_rx_sub_step_value    },
    { "Squelch",    menu_squelch_get_value,   menu_squelch_step_value   },
    { "Volume",     menu_volume_get_value,    menu_volume_step_value    },
    { "Pre-Deemph", menu_pre_get_value,       menu_pre_step_value       },
    { "Highpass",   menu_high_get_value,      menu_high_step_value      },
    { "Lowpass",    menu_low_get_value,       menu_low_step_value       },
    { "Tail Tone",  menu_tail_get_value,      menu_tail_step_value      },
    { "Mode",       menu_mode_get_value,      menu_mode_step_value      },
    { "Power",      menu_power_get_value,     menu_power_step_value     },
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void menu_init(void)
{
    current_menu = menu_item_bandwidth;
    top_menu_index = 0;
    last_value_change_time = 0;
    local_value = 0;
    update_display_async = 1;
    ui_state = ui_state_home;
}

void menu_toggle(void)
{
    if (ui_state == ui_state_menu)
    {
        // Commit any uncommitted changes before exiting menu
        menu_commit_if_pending();
        ui_state = ui_state_home;
    }
    else
    {
        ui_state = ui_state_menu;
    }

    update_display_async = 1;
}

void menu_step_through(int step)
{
    if (ui_state != ui_state_menu) return;

    menu_commit_if_pending();

    int next = (int)current_menu + step;
    if (next < 0) next = menu_item_count - 1;
    if (next >= menu_item_count) next = 0;

    current_menu = (menu_item_t)next;

    if (current_menu < top_menu_index)
        top_menu_index = current_menu;
    else if (current_menu >= top_menu_index + MENU_VISIBLE_LINES)
        top_menu_index = current_menu - MENU_VISIBLE_LINES + 1;

    update_display_async = 1;
}

void menu_value_step(int step)
{
    if (ui_state != ui_state_menu || step == 0) return;

    menu_table[current_menu].step_value(step);
    local_value = 1;
    update_display_async = 1;
    last_value_change_time = HAL_GetTick();
}

void menu_task(void)
{
    uint32_t now = HAL_GetTick();

    if (local_value && (now - last_value_change_time >= MENU_COMMIT_DELAY_MS)) {
        menu_commit_if_pending();
        update_display_async = 1;
    }

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
    const sa818_settings_t *s = sa818_get_settings();
    char line[32];

    // --- Line 1: Title / version ---
    lcd_draw_filled_rect(0, 4, lcd_get_width(), LCD_LINE_SPACING, BLACK);
    snprintf(line, sizeof(line), "SA818 v%s", s->version);
    lcd_show_string(4, 4, lcd_get_width(), 16, LCD_FONT_SIZE, (uint8_t*)line);

    // --- Line 2: Mode and frequency ---
    lcd_draw_filled_rect(0, 22, lcd_get_width(), LCD_LINE_SPACING, BLACK);
    snprintf(line, sizeof(line), "%s %.4f MHz",
             s->mode == SA818_MODE_RX ? "RX" : "TX",
             (s->mode == SA818_MODE_RX) ? s->rx_frequency : s->tx_frequency);
    lcd_show_string(4, 22, lcd_get_width(), 16, LCD_FONT_SIZE, (uint8_t*)line);

    // --- Line 3: RSSI ---
    lcd_draw_filled_rect(0, 40, lcd_get_width(), LCD_LINE_SPACING, BLACK);
    snprintf(line, sizeof(line), "RSSI: %d dBm", s->rssi);
    lcd_show_string(4, 40, lcd_get_width(), 16, LCD_FONT_SIZE, (uint8_t*)line);

    // --- Line 4: Attenuator ---
    lcd_draw_filled_rect(0, 58, lcd_get_width(), LCD_LINE_SPACING, BLACK);
    snprintf(line, sizeof(line), "Atten: %.1f dB", attenuator_get());
    lcd_show_string(4, 58, lcd_get_width(), 16, LCD_FONT_SIZE, (uint8_t*)line);
}

static void draw_menu_screen(void)
{
    // Clear scrollbar area only (right edge)
    lcd_draw_filled_rect(lcd_get_width() - 4, 0, 4, lcd_get_height(), BLACK);

    for (uint8_t i = 0; i < MENU_VISIBLE_LINES; i++) {
        uint8_t index = top_menu_index + i;
        if (index >= menu_item_count)
            break;

        uint16_t y = 4 + i * LCD_LINE_SPACING;
        uint8_t text[32];

        bool selected = (index == current_menu);

        uint16_t bg_color   = selected ? BLUE : BLACK;
        uint16_t text_color = WHITE;

        // Clear and fill line background
        lcd_draw_filled_rect(0, y - 2, lcd_get_width(), LCD_LINE_SPACING, bg_color);

        // Draw menu name
        POINT_COLOR = text_color;
        BACK_COLOR  = bg_color;
        snprintf((char*)text, sizeof(text), "%s", menu_table[index].name);
        lcd_show_string(4, y, 96, 16, LCD_FONT_SIZE, text);

        // Draw menu value
        snprintf((char*)text, sizeof(text), "%s", menu_table[index].get_value());
        lcd_show_string(100, y, lcd_get_width() - 100, 16, LCD_FONT_SIZE, text);
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
static void menu_commit_if_pending(void)
{
    if (local_value)
    {
        local_value = 0;
        menu_on_value_committed();
    }
}

static void menu_on_value_committed(void)
{
    printf("[commit] %s = %s\n",
           menu_table[current_menu].name,
           menu_table[current_menu].get_value());
}

// ---------------------------------------------------------------------------
// SA818 MENU LOGIC
// ---------------------------------------------------------------------------
#define S sa818_get_settings()

static const char* menu_atten_get_value(void)
{
    static char buf[16];
    snprintf(buf, sizeof(buf), "%.1f dB", attenuator_get());
    return buf;
}

static void menu_atten_step_value(int step)
{
    float val = attenuator_get() + (step * 0.5f);  // 0.5 dB per tick
    if (val < ATTENUATOR_MIN_DB) val = ATTENUATOR_MIN_DB;
    if (val > ATTENUATOR_MAX_DB) val = ATTENUATOR_MAX_DB;
    attenuator_set(val);
}

static const char* menu_bandwidth_get_value(void) {
    return S->bandwidth ? "25 kHz" : "12.5 kHz";
}
static void menu_bandwidth_step_value(int step) {
    sa818_set_bandwidth(S->bandwidth ^ 1);
}

static const char* menu_tx_freq_get_value(void) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%.4f", S->tx_frequency);
    return buf;
}
static void menu_tx_freq_step_value(int step) {
    sa818_set_tx_frequency(S->tx_frequency + (step * 0.005f));
}

static const char* menu_rx_freq_get_value(void) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%.4f", S->rx_frequency);
    return buf;
}
static void menu_rx_freq_step_value(int step) {
    sa818_set_rx_frequency(S->rx_frequency + (step * 0.005f));
}

static const char* menu_tx_sub_get_value(void) {
    return S->tx_subaudio;
}
static void menu_tx_sub_step_value(int step) {
    (void)step; // placeholder for editing subaudio
}

static const char* menu_rx_sub_get_value(void) {
    return S->rx_subaudio;
}
static void menu_rx_sub_step_value(int step) {
    (void)step; // placeholder
}

static const char* menu_squelch_get_value(void) {
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d", S->squelch);
    return buf;
}
static void menu_squelch_step_value(int step) {
    sa818_set_squelch(S->squelch + step);
}

static const char* menu_volume_get_value(void) {
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d", S->volume);
    return buf;
}
static void menu_volume_step_value(int step) {
    sa818_set_volume_level(S->volume + step);
}

static const char* menu_pre_get_value(void) {
    return S->pre_de_emph ? "Bypass" : "Normal";
}
static void menu_pre_step_value(int step) {
    (void)step;
    sa818_set_pre_de_emph(!S->pre_de_emph);
}

static const char* menu_high_get_value(void) {
    return S->highpass ? "Bypass" : "Normal";
}
static void menu_high_step_value(int step) {
    (void)step;
    sa818_set_highpass(!S->highpass);
}

static const char* menu_low_get_value(void) {
    return S->lowpass ? "Bypass" : "Normal";
}
static void menu_low_step_value(int step) {
    (void)step;
    sa818_set_lowpass(!S->lowpass);
}

static const char* menu_tail_get_value(void) {
    return S->tail_tone ? "On" : "Off";
}
static void menu_tail_step_value(int step) {
    (void)step;
    sa818_set_tail_tone(!S->tail_tone);
}

static const char* menu_mode_get_value(void) {
    return S->mode == SA818_MODE_TX ? "TX" : "RX";
}
static void menu_mode_step_value(int step) {
    (void)step;
    sa818_set_mode(S->mode == SA818_MODE_TX ? SA818_MODE_RX : SA818_MODE_TX);
}

static const char* menu_power_get_value(void) {
    return S->power == SA818_POWER_HIGH ? "High" : "Low";
}
static void menu_power_step_value(int step) {
    (void)step;
    sa818_set_power_level(S->power == SA818_POWER_HIGH ? SA818_POWER_LOW : SA818_POWER_HIGH);
}
