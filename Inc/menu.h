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
void menu_task(void);  // call this once per main loop iteration

const char* menu_get_value(void);
void menu_item_step_value(int step);

#endif // MENU_H
