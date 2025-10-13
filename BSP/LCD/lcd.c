/**
 ****************************************************************************************************
 * @file        lcd.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-01-01
 * @brief       LCD 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 K210开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "lcd.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "spi.h"
#include "sleep.h"
#include "lcdfont.h"

/**
 * @brief       SPI写命令
 * @param       cmd：7789指令
 * @retval      无
 */
static void lcd_write_command(uint8_t cmd)
{
    gpiohs_set_pin(LCD_DCX_GPIOHS_NUM, GPIO_PV_LOW);
    spi_init(LCD_SPI, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(LCD_SPI, 8, 0, 0, SPI_AITM_AS_FRAME_FORMAT);
    spi_send_data_normal_dma(LCD_SPI_DMA_CH, LCD_SPI, LCD_SPI_CS_NUM, &cmd, 1, SPI_TRANS_CHAR);
}

/**
 * @brief       SPI写数据（uint8_t类型）
 * @param       dat ：数据地址
 * @param       len ：数据长度
 * @retval      无
 */
static void lcd_write_data_8b(uint8_t *dat, uint32_t len)
{
    gpiohs_set_pin(LCD_DCX_GPIOHS_NUM, GPIO_PV_HIGH);
    spi_init(LCD_SPI, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(LCD_SPI, 8, 0, 0, SPI_AITM_AS_FRAME_FORMAT);
    spi_send_data_normal_dma(LCD_SPI_DMA_CH, LCD_SPI, LCD_SPI_CS_NUM, dat, len, SPI_TRANS_CHAR);
}

/**
 * @brief       SPI写数据（uint16_t类型）
 * @param       dat ：数据地址
 * @param       len ：数据长度
 * @retval      无
 */
static void lcd_write_data_16b(uint16_t *dat, uint32_t len)
{
    gpiohs_set_pin(LCD_DCX_GPIOHS_NUM, GPIO_PV_HIGH);
    spi_init(LCD_SPI, SPI_WORK_MODE_0, SPI_FF_OCTAL, 16, 0);
    spi_init_non_standard(LCD_SPI, 16, 0, 0, SPI_AITM_AS_FRAME_FORMAT);
    spi_send_data_normal_dma(LCD_SPI_DMA_CH, LCD_SPI, LCD_SPI_CS_NUM, dat, len, SPI_TRANS_SHORT);
}

/**
 * @brief       SPI写数据（uint32_t类型）
 * @param       dat ：数据地址
 * @param       len ：数据长度
 * @retval      无
 */
static void lcd_write_data_32b_fill(uint32_t *dat, uint32_t len)
{
    gpiohs_set_pin(LCD_DCX_GPIOHS_NUM, GPIO_PV_HIGH);
    spi_init(LCD_SPI, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    spi_init_non_standard(LCD_SPI, 0, 32, 0, SPI_AITM_AS_FRAME_FORMAT);
    spi_fill_data_dma(LCD_SPI_DMA_CH, LCD_SPI, LCD_SPI_CS_NUM, dat, len);
}

/**
 * @brief       设置显示区域
 * @param       x1,y1 ：起点坐标
 * @param       x2,y2 ：终点坐标
 * @retval      无
 */
void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4] = {0};

    data[0] = (uint8_t)(x1 >> 8);
    data[1] = (uint8_t)(x1);
    data[2] = (uint8_t)(x2 >> 8);
    data[3] = (uint8_t)(x2);
    lcd_write_command(0x2A);
    lcd_write_data_8b(data, 4);

    data[0] = (uint8_t)(y1 >> 8);
    data[1] = (uint8_t)(y1);
    data[2] = (uint8_t)(y2 >> 8);
    data[3] = (uint8_t)(y2);
    lcd_write_command(0x2B);
    lcd_write_data_8b(data, 4);

    lcd_write_command(0x2C);
}

/**
 * @brief       LCD初始化
 * @param       无
 * @retval      无
 */
void lcd_init(void)
{
    uint8_t data;

    /* Initialize DCX pin */
    fpioa_set_function(LCD_DCX_PIN, FUNC_GPIOHS0 + LCD_DCX_GPIOHS_NUM);
    gpiohs_set_drive_mode(LCD_DCX_GPIOHS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(LCD_DCX_GPIOHS_NUM, GPIO_PV_HIGH);
    
    /* Initialize RESET pin */
    fpioa_set_function(LCD_RST_PIN, FUNC_GPIOHS0 + LCD_RST_GPIOHS_NUM);
    gpiohs_set_drive_mode(LCD_RST_GPIOHS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(LCD_RST_GPIOHS_NUM, GPIO_PV_HIGH);

    /* Initialize CSX pin */
    fpioa_set_function(LCD_CSX_PIN, FUNC_SPI0_SS0 + LCD_SPI_CS_NUM);

    /* Initialize WRX pin */
    fpioa_set_function(LCD_WRX_PIN, FUNC_SPI0_SCLK);

    /* Initialize SPI interface */
    spi_init(LCD_SPI, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_set_clk_rate(LCD_SPI, LCD_SPI_CLK_RATE);
    
    /* Hardware reset */
    gpiohs_set_pin(LCD_RST_GPIOHS_NUM, GPIO_PV_LOW);
    msleep(50);
    gpiohs_set_pin(LCD_RST_GPIOHS_NUM, GPIO_PV_HIGH);
    msleep(50);

    /* Software reset */
    lcd_write_command(0x01);
    msleep(50);

    /* Exit sleep */
    lcd_write_command(0x11);
    msleep(120);

    /* Set pixel format */
    lcd_write_command(0x3A);
    data = 0x55;
    lcd_write_data_8b(&data, 1);
    msleep(10);

    /* Set direction */
    lcd_write_command(0x36);
    data = 0xA0;
    lcd_write_data_8b(&data, 1);
    
    /* Inversion display */
    lcd_write_command(0x21);
    msleep(10);
    
    /* Display on */
    lcd_write_command(0x13);
    msleep(10);
    lcd_write_command(0x29);

    /* Clear display */
    lcd_clear(0xFFFF);
}

/**
 * @brief       清屏
 * @param       color ：清屏的颜色
 * @retval      无
 */
void lcd_clear(uint16_t color)
{
    lcd_draw_fill_rectangle(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color);
}

/**
 * @brief   画矩形
 * @param   x1   : 矩形左上角X坐标
 * @param   y1   : 矩形左上角Y坐标
 * @param   x2   : 矩形右下角X坐标
 * @param   y2   : 矩形右下角Y坐标
 * @param   width: 画线宽度
 * @param   color: 矩形的颜色
 * @retval  无
 */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color)
{
    lcd_draw_fill_rectangle(x1, y1, x2, y1 + width - 1, color);
    lcd_draw_fill_rectangle(x2 - width + 1, y1, x2, y2, color);
    lcd_draw_fill_rectangle(x1, y2 - width + 1, x2, y2, color);
    lcd_draw_fill_rectangle(x1, y1, x1 + width - 1, y2, color);
}

/**
 * @brief   画空心圆
 * @param   x0   : 圆心的X坐标
 * @param   y0   : 圆心的Y坐标
 * @param   r    : 圆的半径
 * @param   color: 圆的颜色
 * @retval  无
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1);       /* 判断下个点位置的标志 */

    while (a <= b)
    {
        lcd_draw_point(x0 + a, y0 - b, color);  /* 5 */
        lcd_draw_point(x0 + b, y0 - a, color);  /* 0 */
        lcd_draw_point(x0 + b, y0 + a, color);  /* 4 */
        lcd_draw_point(x0 + a, y0 + b, color);  /* 6 */
        lcd_draw_point(x0 - a, y0 + b, color);  /* 1 */
        lcd_draw_point(x0 - b, y0 + a, color);
        lcd_draw_point(x0 - a, y0 - b, color);  /* 2 */
        lcd_draw_point(x0 - b, y0 - a, color);  /* 7 */
        a++;

        /* 使用Bresenham算法画圆 */
        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

/**
 * @brief   画矩形线条
 * @param   x1   : 矩形左上角X坐标
 * @param   y1   : 矩形左上角Y坐标
 * @param   x2   : 矩形右下角X坐标
 * @param   y2   : 矩形右下角Y坐标
 * @param   color: 线条的颜色
 * @retval  无
 */
void lcd_draw_fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | color;
    uint32_t length = (x2 - x1 + 1) * (y2 - y1 + 1);

    lcd_set_area(x1, y1, x2, y2);
    if (length & 1UL)
    {
        lcd_write_data_32b_fill(&data, length - 1);
    }
    else
    {
        lcd_write_data_32b_fill(&data, length);
    }
}

/**
 * @brief   设置显示某个点的颜色
 * @param   x   : X坐标
 * @param   y   : Y坐标
 * @param   color: 点的颜色
 * @retval  无
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_area(x, y, x, y);
    lcd_write_data_16b(&color, 1);
}

/**
 * @brief   LCD显示字符
 * @param   x    : X坐标
 * @param   y    : Y坐标
 * @param   c    : 要显示的字符
 * @param   color: 字符的颜色
 * @retval  无
 */
void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t data = 0;

    for (i = 0; i < 16; i++)
    {
        data = ascii0816[c * 16 + i];
        for (j = 0; j < 8; j++)
        {
            if (data & 0x80)
                lcd_draw_point(x + j, y, color);
            data <<= 1;
        }
        y++;
    }
}

/**
 * @brief   LCD显示字符串
 * @param   x    : X坐标
 * @param   y    : Y坐标
 * @param   str  : 要显示的字符串
 * @param   color: 字符串的颜色
 * @retval  无
 */
void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color)
{
    while (*str)
    {
        lcd_draw_char(x, y, *str, color);
        str++;
        x += 8;
    }
}

/**
 * @brief   设置LCD扫描方向
 * @param   dir  : 结构体指针
 * @retval  无
 */
void lcd_set_direction(lcd_dir_t dir)
{
    lcd_write_command(0x36);
    lcd_write_data_8b((uint8_t *)&dir, 1);
}

/**
 * @brief   LCD显示图片
 * @param   x     : X坐标
 * @param   y     : Y坐标
 * @param   width : 图片的宽度
 * @param   height: 图片的高度
 * @param   pic   : 图片的起始地址
 * @retval  无
 */
void lcd_draw_picture(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *pic)
{
    uint32_t length = width * height;

    lcd_set_area(x, y, x + width - 1, y + height - 1);
    lcd_write_data_16b(pic, length);
}