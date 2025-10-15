#ifndef MENU_H
#define MENU_H

#include <stdint.h>

typedef enum {
    menu_item_attenuator = 0,
    menu_item_bandwidth,
    menu_item_tx_freq,
    menu_item_rx_freq,
    menu_item_tx_sub,
    menu_item_rx_sub,
    menu_item_squelch,
    menu_item_volume,
    menu_item_pre,
    menu_item_high,
    menu_item_low,
    menu_item_tail,
    menu_item_mode,
    menu_item_power,
    menu_item_count
} menu_item_t;

void menu_init(void);
void menu_step_through(int step);
void menu_value_step(int step);
void menu_toggle(void);   // toggles between home view and menu view
void menu_task(void);     // run from main loop
void menu_update_display_async(void);

#endif // MENU_H
