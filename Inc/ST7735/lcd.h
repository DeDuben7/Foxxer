#ifndef __LCD_H
#define __LCD_H		

#include <stdio.h>

#include "st7735.h"

#define WHITE      0xFFFF
#define BLACK      0x0000
#define BLUE       0x001F
#define BRED       0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED        0xF800
#define MAGENTA    0xF81F
#define GREEN      0x07E0
#define CYAN       0x7FFF
#define YELLOW     0xFFE0
#define BROWN 		 0XBC40
#define BRRED 		 0XFC07
#define GRAY  		 0X8430
#define DARKBLUE   0X01CF
#define LIGHTBLUE  0X7D7C
#define GRAYBLUE   0X5458

extern ST7735_Object_t st7735_pObj;
extern uint32_t st7735_id;

extern uint16_t POINT_COLOR;
extern uint16_t BACK_COLOR;

extern void lcd_init(void);
extern void lcd_show_bootlogo(void);
extern void lcd_clear(void);

extern void lcd_set_brightness(uint32_t Brightness);
extern uint32_t lcd_get_brightness(void);

extern uint32_t lcd_get_width(void);

extern void lcd_light(uint32_t Brightness_Dis,uint32_t time);
extern void lcd_show_char(uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint8_t mode);
extern void lcd_show_string(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t size,uint8_t *p);
extern ST7735_Ctx_t ST7735Ctx;

#endif  
