#ifndef MENU_H
#define MENU_H

#include <stdint.h>

typedef enum {
    menu_item_brightness = 0,
    menu_item_volume,
    menu_item_mode,
    menu_item_count
} menu_item_t;

void menu_init(void);
void menu_step_through(int step);
void menu_value_step(int step);
void menu_toggle(void);   // toggles between home view and menu view
void menu_task(void);     // run from main loop

#endif // MENU_H
