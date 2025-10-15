#include "stm32h7xx_hal.h"

#include "lcd_brightness_timer.h"
#include "spi.h"
#include "gpio.h"

#include "font.h"
#include "lcd.h"

#include "board.h"

//LCD_RST
#define LCD_RST_SET     
#define LCD_RST_RESET  
//LCD_RS
#define LCD_RS_SET      HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port,LCD_WR_RS_Pin,GPIO_PIN_SET)//PC4 
#define LCD_RS_RESET    HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port,LCD_WR_RS_Pin,GPIO_PIN_RESET)
//LCD_CS  
#define LCD_CS_SET      HAL_GPIO_WritePin(LCD_CS_GPIO_Port,LCD_CS_Pin,GPIO_PIN_SET)
#define LCD_CS_RESET    HAL_GPIO_WritePin(LCD_CS_GPIO_Port,LCD_CS_Pin,GPIO_PIN_RESET)

static int32_t lcd_gettick(void);
static int32_t lcd_writereg(uint8_t reg,uint8_t* pdata,uint32_t length);
static int32_t lcd_readreg(uint8_t reg,uint8_t* pdata);
static int32_t lcd_senddata(uint8_t* pdata,uint32_t length);
static int32_t lcd_recvdata(uint8_t* pdata,uint32_t length);

ST7735_IO_t st7735_pIO = {
	NULL,
	NULL,
	0,
	lcd_writereg,
	lcd_readreg,
	lcd_senddata,
	lcd_recvdata,
	lcd_gettick
};

ST7735_Object_t st7735_pObj;
uint32_t st7735_id;

void lcd_init(void) {
  lcd_brightness_timer_init();
  lcd_brightness_timer_start();

  display_spi_init();

  ST7735Ctx.Orientation = ST7735_ORIENTATION_LANDSCAPE_ROT180;
  ST7735Ctx.Panel = HannStar_Panel;
  ST7735Ctx.Type = ST7735_0_9_inch_screen;

  ST7735_RegisterBusIO(&st7735_pObj,&st7735_pIO);
  ST7735_LCD_Driver.Init(&st7735_pObj,ST7735_FORMAT_RBG565,&ST7735Ctx);
  ST7735_LCD_Driver.ReadID(&st7735_pObj,&st7735_id);
}

void lcd_show_bootlogo(void)
{
	uint8_t text[20];
	
	extern unsigned char foxxer_logo_160_80[];
	ST7735_LCD_Driver.DrawBitmap(&st7735_pObj,0,0,foxxer_logo_160_80);

  uint32_t tick = HAL_GetTick();
	while (1)
	{
	  HAL_Delay(10);

		if (HAL_GetTick() - tick <= 1000)
		  lcd_set_brightness((HAL_GetTick() - tick) * 100 / 1000);
		else
			break;
	}
	lcd_light(0, 300);

	ST7735_LCD_Driver.FillRect(&st7735_pObj, 0, 0, ST7735Ctx.Width,ST7735Ctx.Height, BLACK);

	lcd_light(100, 200);
}

void lcd_clear(void) {
  ST7735_LCD_Driver.FillRect(&st7735_pObj, 0, 0, ST7735Ctx.Width,ST7735Ctx.Height, BLACK);
}

void lcd_set_brightness(uint32_t brightness) {
  lcd_brightness_timer_set_brightness(brightness);
}

uint32_t lcd_get_brightness(void) {
  return lcd_brightness_timer_get_brightness();
}

uint32_t lcd_get_width(void) {
  return ST7735Ctx.Width;
}

uint32_t lcd_get_height(void) {
  return ST7735Ctx.Height;
}

// Simple outline rectangle (1-pixel border)
void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // Top line
    ST7735_LCD_Driver.FillRect(&st7735_pObj, x, y, w, 1, color);
    // Bottom line
    ST7735_LCD_Driver.FillRect(&st7735_pObj, x, y + h - 1, w, 1, color);
    // Left line
    ST7735_LCD_Driver.FillRect(&st7735_pObj, x, y, 1, h, color);
    // Right line
    ST7735_LCD_Driver.FillRect(&st7735_pObj, x + w - 1, y, 1, h, color);
}

// Filled rectangle
void lcd_draw_filled_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ST7735_LCD_Driver.FillRect(&st7735_pObj, x, y, w, h, color);
}

void lcd_light(uint32_t Brightness_Dis,uint32_t time) {
	uint32_t Brightness_Now;
	uint32_t time_now;
	float temp1,temp2;
	float k,set;
	
	Brightness_Now = lcd_get_brightness();
	time_now = 0;
	if(Brightness_Now == Brightness_Dis)
		return;
	
	if(time == time_now)
		return;
	
	temp1 = Brightness_Now;
	temp1 = temp1 - Brightness_Dis;
	temp2 = time_now;
	temp2 = temp2 - time;
	
	k = temp1 / temp2;
	
	uint32_t tick=HAL_GetTick();
	while(1)
	{
		HAL_Delay(1);
		
		time_now = HAL_GetTick()-tick;
		
		temp2 = time_now - 0;
		
		set = temp2*k + Brightness_Now;
		
		lcd_set_brightness((uint32_t)set);
		
		if(time_now >= time) break;
		
	}
}
	
uint16_t POINT_COLOR=0xFFFF;
uint16_t BACK_COLOR=BLACK;

void lcd_show_char(uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint8_t mode)
{  							  
  uint8_t temp,t1,t;
	uint16_t y0=y;
	uint16_t x0=x;
	uint16_t colortemp=POINT_COLOR; 
  uint32_t h,w;
	
	uint16_t write[size][size==12?6:8];
	uint16_t count;
	
  ST7735_GetXSize(&st7735_pObj,&w);
	ST7735_GetYSize(&st7735_pObj,&h);

	num=num-' ';
	count = 0;
	
	if(!mode)
	{
		for(t=0;t<size;t++)
		{   
			if(size==12)temp=asc2_1206[num][t];
			else temp=asc2_1608[num][t];
			
			for(t1=0;t1<8;t1++)
			{			    
				if(temp&0x80)
					POINT_COLOR=(colortemp&0xFF)<<8|colortemp>>8;
				else 
					POINT_COLOR=(BACK_COLOR&0xFF)<<8|BACK_COLOR>>8;
				
				write[count][t/2]=POINT_COLOR;
				count ++;
				if(count >= size) count =0;
				
				temp<<=1;
				y++;
				if(y>=h){POINT_COLOR=colortemp;return;}
				if((y-y0)==size)
				{
					y=y0;
					x++;
					if(x>=w){POINT_COLOR=colortemp;return;}
					break;
				}
			}
		}
	}
	else
	{
		for(t=0;t<size;t++)
		{   
			if(size==12)temp=asc2_1206[num][t];
			else temp=asc2_1608[num][t];
			for(t1=0;t1<8;t1++)
			{			    
				if(temp&0x80)
					write[count][t/2]=(POINT_COLOR&0xFF)<<8|POINT_COLOR>>8;
				count ++;
				if(count >= size) count =0;
				
				temp<<=1;
				y++;
				if(y>=h){POINT_COLOR=colortemp;return;}
				if((y-y0)==size)
				{
					y=y0;
					x++;
					if(x>=w){POINT_COLOR=colortemp;return;}
					break;
				}
			}  	 
		}     
	}
	ST7735_FillRGBRect(&st7735_pObj,x0,y0,(uint8_t *)&write,size==12?6:8,size); 
	POINT_COLOR=colortemp;	    	   	 	  
}   

void lcd_show_string(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t size,uint8_t *p)
{         
	uint8_t x0=x;
	width+=x;
	height+=y;
    while((*p<='~')&&(*p>=' '))
    {       
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;
        lcd_show_char(x,y,*p,size,0);
        x+=size/2;
        p++;
    }  
}

static int32_t lcd_gettick(void)
{
	return HAL_GetTick();
}

static int32_t lcd_writereg(uint8_t reg,uint8_t* pdata,uint32_t length)
{
	int32_t result;
	LCD_CS_RESET;
	LCD_RS_RESET;
	result = display_spi_transmit(&reg, 1, 100);
	LCD_RS_SET;
	if(length > 0)
		result += display_spi_transmit(pdata, length, 500);
	LCD_CS_SET;
	if(result>0){
		result = -1;}
	else{
		result = 0;}
	return result;
}

static int32_t lcd_readreg(uint8_t reg,uint8_t* pdata)
{
	int32_t result;
	LCD_CS_RESET;
	LCD_RS_RESET;
	
	result = display_spi_transmit(&reg, 1, 100);
	LCD_RS_SET;
	result += display_spi_receive(pdata, 1, 500);
	LCD_CS_SET;
	if(result>0){
		result = -1;}
	else{
		result = 0;}
	return result;
}

static int32_t lcd_senddata(uint8_t* pdata,uint32_t length)
{
	int32_t result;
	LCD_CS_RESET;
	//LCD_RS_SET;
	result =display_spi_transmit(pdata, length, 100);
	LCD_CS_SET;
	if(result>0){
		result = -1;}
	else{
		result = 0;}
	return result;
}

static int32_t lcd_recvdata(uint8_t* pdata,uint32_t length)
{
	int32_t result;
	LCD_CS_RESET;
	//LCD_RS_SET;
	result = display_spi_receive(pdata, length, 500);
	LCD_CS_SET;
	if(result>0){
		result = -1;}
	else{
		result = 0;}
	return result;
}

