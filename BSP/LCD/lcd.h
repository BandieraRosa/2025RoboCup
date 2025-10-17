#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>
#include "lcdfont.h"

#define LCD_SPI_CLK_RATE 15000000

/* Configure connect pin */
#if !defined(LCD_CSX_PIN)
#define LCD_CSX_PIN 36
#endif
#if !defined(LCD_RST_PIN)
#define LCD_RST_PIN 37
#endif
#if !defined(LCD_DCX_PIN)
#define LCD_DCX_PIN 38
#endif
#if !defined(LCD_WRX_PIN)
#define LCD_WRX_PIN 39
#endif
#if !defined(LCD_DCX_GPIOHS_NUM)
#define LCD_DCX_GPIOHS_NUM 31
#endif
#if !defined(LCD_RST_GPIOHS_NUM)
#define LCD_RST_GPIOHS_NUM 30
#endif

/* Configure SPI interface */
#if !defined(LCD_SPI)
#define LCD_SPI SPI_DEVICE_0
#endif
#if !defined(LCD_SPI_CS_NUM)
#define LCD_SPI_CS_NUM SPI_CHIP_SELECT_3
#endif
#if !defined(LCD_SPI_DMA_CH)
#define LCD_SPI_DMA_CH DMAC_CHANNEL2
#endif
#if !defined(LCD_SPI_CLK_RATE)
#define LCD_SPI_CLK_RATE 15000000
#endif

/* LCD information */
#define LCD_WIDTH   320
#define LCD_HEIGHT  240

/* 颜色定义 */
#define BLACK       0x0000 /* 黑色 */
#define NAVY        0x000F /* 深蓝色 */
#define DARKGREEN   0x03E0 /* 墨绿色 */
#define DARKCYAN    0x03EF /* 深青色 */
#define MAROON      0x7800 /* 褐红色 */
#define PURPLE      0x780F /* 紫色 */
#define OLIVE       0x7BE0 /* 浅褐色 */
#define LIGHTGREY   0xC618 /* 浅灰色  */
#define DARKGREY    0x7BEF /* 深灰色 */
#define BLUE        0x001F /* 蓝色 */
#define GREEN       0x07E0 /* 绿色 */
#define CYAN        0x07FF /* 青色 */
#define RED         0xF800 /* 红色 */
#define MAGENTA     0xF81F /* 品红色 */
#define YELLOW      0xFFE0 /* 黄色 */
#define BRRED       0xFC07 /* 棕红色 */
#define WHITE       0xFFFF /* 白色 */
#define ORANGE      0xFD20 /* 橙色 */
#define GREENYELLOW 0xAFE5 /* 黄绿色 */
#define PINK        0xF81F /* 粉红色 */
#define BROWN       0xBC40  /* 棕色 */
#define GRAY        0x8430  /* 灰色 */
#define USER_COLOR  0xAA55

/* LCD扫描方向定义 */
typedef enum
{
    DIR_XY_RLUD = 0x00,
    DIR_YX_RLUD = 0x20,
    DIR_XY_LRUD = 0x40,
    DIR_YX_LRUD = 0x60,
    DIR_XY_RLDU = 0x80,
    DIR_YX_RLDU = 0xA0,
    DIR_XY_LRDU = 0xC0,
    DIR_YX_LRDU = 0xE0,
} lcd_dir_t;

/* 函数声明 */
void lcd_init(void);
void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_clear(uint16_t color);
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color);
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void lcd_draw_fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_picture(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *pic);
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color);
void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color);
void lcd_set_direction(lcd_dir_t dir);
#endif
