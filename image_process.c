/**
 ****************************************************************************************************
 * @file        image_process.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-01-01
 * @brief       图片处理代码
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "image_process.h"
#include "iomem.h"
#include "./BSP/LCD/lcdfont.h"


/**
 * @brief       图片初始化，为图片申请内存
 * @param       image:图片的结构体指针
 * @retval      返回值  : 0，成功
 *                       -1，失败  
 */
int image_init(image_t *image)
{
    image->addr = iomem_malloc(image->width * image->height * image->pixel);
    if (image->addr == NULL)
        return -1;
    return 0;
}

/**
 * @brief       删除图片初始化，释放图片内存
 * @param       image:图片的结构体指针
 * @retval      无 
 */
void image_deinit(image_t *image)
{
    iomem_free(image->addr);
}

/**
 * @brief       图片切割
 * @param       image_src:源图片的结构体指针
 * @param       image_dst:目标图片的结构体指针
 * @param       x_offset:x方向偏移量
 * @param       y_offset:y方向偏移量
 * @retval      无
 */
void image_crop(image_t *image_src, image_t *image_dst, uint16_t x_offset, uint16_t y_offset) 
{
    uint8_t *src, *r_src, *g_src, *b_src, *dst, *r_dst, *g_dst, *b_dst;
    uint16_t w_src, h_src, w_dst, h_dst;

    src = image_src->addr;
    w_src = image_src->width;
    h_src = image_src->height;
    dst = image_dst->addr;
    w_dst = image_dst->width;
    h_dst = image_dst->height;

    r_src = src + y_offset * w_src + x_offset;
    g_src = r_src + w_src * h_src;
    b_src = g_src + w_src * h_src;

    r_dst = dst;
    g_dst = r_dst + w_dst * h_dst;
    b_dst = g_dst + w_dst * h_dst;

    // for (uint16_t y = 0; y < h_dst; y++)
    // {
    //     for (uint16_t x = 0; x < w_dst; x++)
    //     {
    //         *r_dst++ = r_src[x];
    //         *g_dst++ = g_src[x];
    //         *b_dst++ = b_src[x];
    //     }
    //     r_src += w_src;
    //     g_src += w_src;
    //     b_src += w_src;
    // }
    if ((h_src - y_offset) > h_dst)
    {
        for (uint16_t y = 0; y < h_dst; y++)
        {
            for (uint16_t x = 0; x < w_dst; x++)
            {
                if (x <= (w_src - x_offset))
                {
                    *r_dst++ = r_src[x];
                    *g_dst++ = g_src[x];
                    *b_dst++ = b_src[x];
                }
                else
                {
                    *r_dst++ = 0;
                    *g_dst++ = 0;
                    *b_dst++ = 0;
                } 
            }
            r_src += w_src;
            g_src += w_src;
            b_src += w_src;
        }
    }
    else
    {
        for (uint16_t y = 0; y < (h_src - y_offset); y++)
        {
            for (uint16_t x = 0; x < w_dst; x++)
            {
                if (x <= (w_src - x_offset))
                {
                    *r_dst++ = r_src[x];
                    *g_dst++ = g_src[x];
                    *b_dst++ = b_src[x];
                }
                else
                {
                    *r_dst++ = 0;
                    *g_dst++ = 0;
                    *b_dst++ = 0;
                } 
            }
            r_src += w_src;
            g_src += w_src;
            b_src += w_src;
        }
    }
}

/**
 * @brief       将image_src图片写入image_dst图片
 * @param       image_src:源图片的结构体指针
 * @param       image_dst:目标图片的结构体指针
 * @param       x_start:写入目标图片x方向的起点
 * @param       y_start:写入目标图片y方向的起点
 * @retval      无
 */
void image_draw(image_t *image_src, image_t *image_dst, uint16_t x_start, uint16_t y_start) /*画图*/
{
    uint8_t *src, *r_src, *g_src, *b_src, *dst, *r_dst, *g_dst, *b_dst;
    uint16_t w_src, h_src, w_dst, h_dst;
    uint16_t x_offset = x_start;
    uint16_t y_offset = y_start;

    src = image_src->addr;
    w_src = image_src->width;
    h_src = image_src->height;
    dst = image_dst->addr;
    w_dst = image_dst->width;
    h_dst = image_dst->height;

    r_src = src;
    g_src = r_src + w_src * h_src;
    b_src = g_src + w_src * h_src;

    r_dst = dst + y_offset * w_dst + x_offset;
    g_dst = r_dst + w_dst * h_dst;
    b_dst = g_dst + w_dst * h_dst;

    for (uint32_t y = 0; y < h_dst * w_dst * 3; y++)
    {
        *dst++ = 0;
    }

    for (uint16_t y = 0; y < h_src; y++)  /*高度为源图片最大宽度，宽度需要用于偏移*/
    {
        for (uint16_t x = 0; x < (w_dst - x_offset); x++)
        {
            if (x <= w_src)
            {
                *r_dst++ = r_src[x];
                *g_dst++ = g_src[x];
                *b_dst++ = b_src[x];
            }
            else
            {
                *r_dst++ = 0;
                *g_dst++ = 0;
                *b_dst++ = 0;
            }  
        }
        r_dst += x_offset ;
        g_dst += x_offset ;
        b_dst += x_offset ;

        r_src += w_src ;
        g_src += w_src ;
        b_src += w_src ;
    }
}

/**
 * @brief       图片缩放
 * @param       image_src:源图片的结构体指针
 * @param       image_dst:目标图片的结构体指针
 * @retval      无
 */
void image_resize(image_t *image_src, image_t *image_dst) /* 缩放，缩放大小与两个结构体有关*/
{
    uint16_t x1, x2, y1, y2;
    float w_scale, h_scale;
    float temp1, temp2;
    float x_src, y_src;

    uint8_t *r_src, *g_src, *b_src, *r_dst, *g_dst, *b_dst;
    uint16_t w_src, h_src, w_dst, h_dst;

    w_src = image_src->width;
    h_src = image_src->height;
    r_src = image_src->addr;
    g_src = r_src + w_src * h_src;
    b_src = g_src + w_src * h_src;
    w_dst = image_dst->width;
    h_dst = image_dst->height;
    r_dst = image_dst->addr;
    g_dst = r_dst + w_dst * h_dst;
    b_dst = g_dst + w_dst * h_dst;

    w_scale = (float)w_src / w_dst;
    h_scale = (float)h_src / h_dst;

    for (uint16_t y = 0; y < h_dst; y++)
    {
        for (uint16_t x = 0; x < w_dst; x++)
        {
            x_src = (x + 0.5f) * w_scale - 0.5f;
            x1 = (uint16_t)x_src;
            x2 = x1 + 1;
            y_src = (y + 0.5f) * h_scale - 0.5f;
            y1 = (uint16_t)y_src;
            y2 = y1 + 1;

            if (x2 >= w_src || y2 >= h_src)
            {   
                *(r_dst + x + y * w_dst) = *(r_src + x1 + y1 * w_src);
                *(g_dst + x + y * w_dst) = *(g_src + x1 + y1 * w_src);
                *(b_dst + x + y * w_dst) = *(b_src + x1 + y1 * w_src);
                continue;
            }

            temp1 = (x2 - x_src) * *(r_src + x1 + y1 * w_src) + (x_src - x1) * *(r_src + x2 + y1 * w_src);
            temp2 = (x2 - x_src) * *(r_src + x1 + y2 * w_src) + (x_src - x1) * *(r_src + x2 + y2 * w_src);
            *(r_dst + x + y * w_dst) = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
            temp1 = (x2 - x_src) * *(g_src + x1 + y1 * w_src) + (x_src - x1) * *(g_src + x2 + y1 * w_src);
            temp2 = (x2 - x_src) * *(g_src + x1 + y2 * w_src) + (x_src - x1) * *(g_src + x2 + y2 * w_src);
            *(g_dst + x + y * w_dst) = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
            temp1 = (x2 - x_src) * *(b_src + x1 + y1 * w_src) + (x_src - x1) * *(b_src + x2 + y1 * w_src);
            temp2 = (x2 - x_src) * *(b_src + x1 + y2 * w_src) + (x_src - x1) * *(b_src + x2 + y2 * w_src);
            *(b_dst + x + y * w_dst) = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
        }
    }
}

/**
 * @brief       RGB888图片翻转
 * @param       image_addr:RGB888图片起始地址
 * @param       image_width:图片宽度
 * @param       image_height:图片高度
 * @param       vflip:垂直翻转
 * @param       hmirror:水平翻转
 * @retval      无
 */
void image_replace(uint8_t *image_addr, uint16_t image_width, uint16_t image_height, uint8_t vflip, uint8_t hmirror)
{
    uint8_t *src, *r_src, *g_src, *b_src;
    uint8_t temp;
    uint16_t t1, t2;
    uint32_t offset1, offset2;
    src = image_addr;

    if (vflip == 1)
    {
        t1 = image_height >> 1;

        r_src = src;
        g_src = r_src + image_width * image_height;
        b_src = g_src + image_width * image_height;

        for (uint16_t j = 0; j < t1; j++)
        {
            for (uint16_t i = 0; i < image_width; i++)
            {
                offset1 = (image_width * j)  + i;
                offset2 = image_width * (image_height - 1 - j)  + i;

                temp = *(r_src + offset1);
                *(r_src + offset1) = *(r_src + offset2);
                *(r_src + offset2) = temp;

                temp = *(g_src + offset1);
                *(g_src + offset1) = *(g_src + offset2);
                *(g_src + offset2) = temp;

                temp = *(b_src + offset1);
                *(b_src + offset1) = *(b_src + offset2);
                *(b_src + offset2) = temp;
            }
        }    
    }

    if (hmirror == 1)
    {
        t2 = image_width >> 1;
        for (uint16_t j = 0; j < image_height * 3; j++)
        {
            for (uint16_t i = 0; i < t2; i++)
            {
                offset1 = image_width - i - 1;

                temp = *(src + i);
                *(src + i) = *(src + offset1);
                *(src + offset1) = temp;
            }
            src = src + image_width;
        }      
    }
}

/**
 * @brief       RGB888转灰度图
 * @param       image_addr:RGB888图片起始地址
 * @param       image_width:图片宽度
 * @param       image_height:图片高度
 * @retval      无
 */
void image_rgb888_to_gray(uint8_t *image_addr, uint16_t image_width, uint16_t image_height)
{
    uint8_t *src, *r_src, *g_src, *b_src;
    int gray_temp;

    src = image_addr;
    r_src = src;
    g_src = r_src + image_width * image_height;
    b_src = g_src + image_width * image_height;

    for (uint32_t j = 0; j < image_width * image_height; j++)
    {
        gray_temp = (306 * (int)*r_src + 601 * (int)*g_src + 117 * (int)*b_src) >> 10;  /* 转换公式 */

        *r_src = (uint8_t)gray_temp;
        *g_src = (uint8_t)gray_temp;
        *b_src = (uint8_t)gray_temp;

        r_src++;
        g_src++;
        b_src++;
    }    
}

/**
 * @brief       RGB888颜色值翻转
 * @param       image_addr:RGB888图片起始地址
 * @param       image_width:图片宽度
 * @param       image_height:图片高度
 * @retval      无
 */
void image_invert(uint8_t *image_addr, uint16_t image_width, uint16_t image_height)
{
    uint8_t *r_src, *g_src, *b_src;
    r_src = image_addr;
    g_src = r_src + image_width * image_height;
    b_src = g_src + image_width * image_height;

    for (uint32_t j = 0; j < image_width * image_height; j++)
    {
        *r_src = ~*r_src;
        *g_src = ~*g_src;
        *b_src = ~*b_src;

        r_src++;
        g_src++;
        b_src++;
    }    
}

/**
 * @brief       RGB888去除暗角
 * @param       image_addr:RGB888图片起始地址
 * @param       image_width:图片宽度
 * @param       image_height:图片高度
 * @param       de_dark : 删除暗角
 * @retval      无
 */
void image_strech_chart(uint8_t *image_addr, uint16_t image_width, uint16_t image_height, uint8_t de_dark)
{
    uint8_t *r_src, *g_src, *b_src;
    uint8_t *in;
    uint32_t index;
    uint16_t x, y, graymax;
    int sx2, sx, ex, r2;
    int gate, dat;

    r_src = image_addr;
    g_src = r_src + image_width * image_height;
    b_src = g_src + image_width * image_height;
    sx = 0;
    sx2 = 0;
    graymax = 0;
    
    in = r_src;
    for(index = 0; index < image_width * image_height; index++)
    {
        if(in[index] > graymax) graymax=in[index];
        sx += in[index];
        sx2 += ((int)in[index] * (int)in[index]);
    }
    
    ex = sx / image_width / image_height;
    gate = ex;

    for(index = 0; index < image_width * image_height; index++)
    {
        x = index % image_width;
        y = index / image_width;
        dat = in[index];
        dat = (dat - gate) * 255 / (graymax - gate);
        dat = dat < 0? 0 : (dat > 255? 255 : dat);
        r2 = (x - image_width / 2) * (x - image_width / 2) + (y - image_height / 2) * (y - image_height / 2);

        if(de_dark)
        {
            dat = (int)(dat / (1.0 + 32.0 * r2 * r2 / image_width / image_width / image_height / image_height)); 
        }
        in[index] = dat;
        g_src[index] = dat;
        b_src[index] = dat;
    }  
}

/**
 * @brief       RGB888转RGB565
 * @param       src_addr:RGB888图片起始地址
 * @param       dest_addr:RGB565图片起始地址
 * @param       image_width:图片宽度
 * @param       image_height:图片高度
 * @retval      无
 */
void rgb888_to_rgb565(uint8_t *src_addr, uint16_t *dest_addr, uint16_t image_width, uint16_t image_height)
{
    size_t chn_size = image_width * image_height;
    for (size_t i = 0; i < image_width * image_height; i++)
    {
        uint8_t r = src_addr[i];
        uint8_t g = src_addr[chn_size + i];
        uint8_t b = src_addr[chn_size * 2 + i];

        uint16_t rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
        size_t d_i = i % 2 ? (i - 1) : (i + 1);
        dest_addr[d_i] = rgb;
    }
}

/**
 * @brief       在图片上写入字符串
 * @param       image_addr:RGB565图片起始地址
 * @param       x,y:起始坐标
 * @param       image_width:图片宽度
 * @param       image_height:图片高度
 * @param       str:字符数据
 * @param       color:字体颜色
 * @retval      无
 */
void draw_string_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t image_height, uint16_t x, uint16_t y, char *str, uint16_t color)
{
    uint16_t *src,*origin;
    uint16_t w_src, h_src;

    uint16_t slen = strlen(str);
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t data = 0;
    
    src = image_addr;
    w_src = image_width;
    h_src = image_height;

    if ((slen * 8 + x) > w_src)
    {
        x = w_src - slen * 8;
        printf("x out of range!");
    }
    if (y > (h_src - 16))
    {
        y = h_src - 16;
        printf("y out of range!");
    }

    src += y * w_src + x;
    origin = src;

    while (*str)
    {
        for (i = 0; i < 16; i++)
        {
            data = ascii0816[*str * 16 + i];
            src = origin + i * w_src; /*写完8个点后偏移到下一行*/
            for (j = 0; j < 8; j++)
            {
                if (data & 0x80)
                {
                    src[j] = color;
                    // lcd_draw_point(x + j, y + i + 16, color);
                }
                data <<= 1;
            }
            // y++;
            // src += 2 * w_src;
        }
        str++;
        origin += 8; /*偏移下个字符的起点*/
        // x += 8;
    }
}

/**
 * @brief       在图片上写矩形框
 * @param       image_addr:RGB565图片起始地址
 * @param       image_width:图片宽度
 * @param       x1,y1:起点坐标
 * @param       x2,y2:终点坐标
 * @param       color:字体颜色
 * @retval      无
 */
void draw_box_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    uint32_t *addr1, *addr2, *addr3, *addr4;

    if (x1 < 1) x1 = 0;
    if (x2 > 319) x2 = 319;
    if (y1 < 1) y1 = 0;
    if (y2 > 239) y2 = 239;

    addr1 = (uint32_t *)image_addr + (image_width * y1 + x1) / 2;
    addr2 = (uint32_t *)image_addr + (image_width * (y1 + 1) + x1) / 2;
    addr3 = (uint32_t *)image_addr + (image_width * y2 + x1) / 2;
    addr4 = (uint32_t *)image_addr + (image_width * (y2 - 1) + x1) / 2;

    for (uint8_t i = 0; i < (x2 - x1) / 2; i++)
    {
        *addr1 = data;
        *addr2 = data;
        *addr3 = data;
        *addr4 = data;

        addr1++;
        addr2++;
        addr3++;
        addr4++;
    }

    addr1 = (uint32_t *)image_addr + (image_width * y1 + x1) / 2;
    addr2 = (uint32_t *)image_addr + (image_width * y1 + x2) / 2 - 1;
    for (uint16_t i = 0; i < y2 - y1; i++)
    {
        *addr1 = data;
        *addr2 = data;
        addr1 += image_width / 2;
        addr2 += image_width / 2;
    }
    
}

/**
 * @brief       在图片上填充矩形颜色条
 * @param       image_addr:RGB565图片起始地址
 * @param       image_width:图片宽度
 * @param       x1,y1:起点坐标
 * @param       x2,y2:终点坐标
 * @param       color:颜色
 * @retval      无
 */
void draw_fill_rectangle_image(uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    uint32_t *addr;

    if (x1 < 1) x1 = 0;
    if (x2 > 319) x2 = 319;
    if (y1 < 1) y1 = 0;
    if (y2 > 239) y2 = 239;

    addr = (uint32_t *)image_addr + (image_width * y1 + x1) / 2;

    for (uint8_t j = 0; j < y2 - y1; j++)
    {

        for (uint8_t i = 0; i < (x2 - x1) / 2; i++)
        {
            addr[i] = data;
        }
        addr += image_width / 2;
    }    
}

/**
 * @brief       在图片上画点
 * @param       image_addr:RGB565图片起始地址
 * @param       image_width:图片宽度
 * @param       x,y:点的坐标
 * @param       color:点的颜色
 * @retval      无
 */
void draw_point_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t x, uint16_t y, uint16_t color)
{
    if (x > 319) x = 319;
    if (y > 239) y = 239;

    *(image_addr + y * image_width + x) = color; 
}


// --- 小球识别部分相关函数与宏 ---


int start_color_recognize(uint8_t *snapshot_img_rgb565) {
    int blue_cnt = 0;
    int red_cnt = 0;

    uint16_t *pixel_ptr = (uint16_t *)snapshot_img_rgb565;

    for (int i = 0; i < CAMERA_WIDTH * CAMERA_HEIGHT; i++) {
        uint16_t pixel_color = pixel_ptr[i];
        uint8_t r = (pixel_color & 0xF800) >> 11;
        uint8_t g = (pixel_color & 0x07E0) >> 5;
        uint8_t b = (pixel_color & 0x001F);
        g = g >> 1;

        if (abs(r - b) < SATURATION_THRESHOLD || (r < BRIGHTNESS_THRESHOLD && b < BRIGHTNESS_THRESHOLD)) {
            continue;
        }
        if (r > b && r > g) {
            red_cnt++;
        }
        else if (b > r && b > g) {
            blue_cnt++;
        }
    }
    const int MIN_COUNT_THRESHOLD = 100;
    if (red_cnt > blue_cnt && red_cnt > MIN_COUNT_THRESHOLD) {
        return BALL_RED;
    } else if (blue_cnt > red_cnt && blue_cnt > MIN_COUNT_THRESHOLD) {
        return BALL_BLUE;
    } else {
        return BALL_UNKNOWN;
    }
}

void fill_color_area(uint8_t *disp, enum COLOR color) {
    uint16_t *pixel_ptr = (uint16_t *)disp;
    const uint16_t GREEN_COLOR_565 = 0x07E0;

    for (int i = 0; i < CAMERA_WIDTH * CAMERA_HEIGHT; i++) {
        uint16_t pixel_color = pixel_ptr[i];
        uint8_t r = (pixel_color & 0xF800) >> 11;
        uint8_t g = (pixel_color & 0x07E0) >> 5;
        uint8_t b = (pixel_color & 0x001F);
        uint8_t g_scaled = g >> 1;

        if (abs(r - b) < SATURATION_THRESHOLD || (r < BRIGHTNESS_THRESHOLD && b < BRIGHTNESS_THRESHOLD)) {
            continue;
        }

        int should_paint = 0;
        if (color == BALL_BLUE) {
            if (b > r && b > g_scaled) {
                should_paint = 1;
            }
        } else if (color == BALL_RED) {
            if (r > b && r > g_scaled) {
                should_paint = 1;
            }
        }
        if (should_paint) {
            pixel_ptr[i] = GREEN_COLOR_565;
        }
    }
}

#define TARGET_BLUE_R 4
#define TARGET_BLUE_G 8  // 注意G是6位，但为了计算方便可以统一或加权
#define TARGET_BLUE_B 28

#define TARGET_RED_R 30
#define TARGET_RED_G 8
#define TARGET_RED_B 5

// 颜色距离的平方阈值，需要调试来确定
#define COLOR_DISTANCE_THRESHOLD 200 

void fill_color_area_by_distance(uint8_t *disp, enum COLOR color) {
    uint16_t *pixel_ptr = (uint16_t *)disp;
    const uint16_t GREEN_COLOR_565 = 0x07E0;

    int target_r, target_g, target_b;

    if (color == BALL_BLUE) {
        target_r = TARGET_BLUE_R;
        target_g = TARGET_BLUE_G;
        target_b = TARGET_BLUE_B;
    } else if (color == BALL_RED) {
        target_r = TARGET_RED_R;
        target_g = TARGET_RED_G;
        target_b = TARGET_RED_B;
    } else {
        return;
    }

    for (int i = 0; i < CAMERA_WIDTH * CAMERA_HEIGHT; i++) {
        uint16_t pixel_color = pixel_ptr[i];
        
        // 1. 解码RGB565像素
        uint8_t r = (pixel_color & 0xF800) >> 11;
        uint8_t g = (pixel_color & 0x07E0) >> 5; // g是6位 (0-63)
        uint8_t b = (pixel_color & 0x001F);
        
        // 为了公平比较，可以将6位的g缩放到5位范围
        uint8_t g_scaled = g >> 1; // 变为 (0-31)

        // 2. 计算颜色距离的平方
        long dist_sq = (r - target_r) * (r - target_r) +
                       (g_scaled - target_g) * (g_scaled - target_g) + // 使用缩放后的g
                       (b - target_b) * (b - target_b);

        // 3. 判断是否在阈值内
        if (dist_sq < COLOR_DISTANCE_THRESHOLD) {
            pixel_ptr[i] = GREEN_COLOR_565;
        }
    }
}


BallInfo find_ball(uint8_t *img_rgb565, enum COLOR ball_color) {
    BallInfo ball = {0, 0, 0, 0, 0};
    uint16_t *pixel_ptr = (uint16_t *)img_rgb565;

    // 用于计算质心和边界框的变量
    long sum_x = 0;
    long sum_y = 0;
    int pixel_count = 0;
    int min_x = CAMERA_WIDTH, max_x = 0;
    int min_y = CAMERA_HEIGHT, max_y = 0;

    // 1. 颜色分割 和 2. 聚类 (简化版：假设所有目标色像素属于同一个物体)
    for (int y = 0; y < CAMERA_HEIGHT; y++) {
        for (int x = 0; x < CAMERA_WIDTH; x++) {
            uint16_t pixel_color = pixel_ptr[y * CAMERA_WIDTH + x];

            uint8_t r = (pixel_color & 0xF800) >> 11;
            uint8_t g = (pixel_color & 0x07E0) >> 5;
            uint8_t b = (pixel_color & 0x001F);
            uint8_t g_scaled = g >> 1;

            if (abs(r - b) < SATURATION_THRESHOLD || (r < BRIGHTNESS_THRESHOLD && b < BRIGHTNESS_THRESHOLD)) {
                continue; // 忽略灰度/暗像素
            }

            int is_target_color = 0;
            if (ball_color == BALL_RED) {
                if (r > b && r > g_scaled) is_target_color = 1;
            } else {
                if (b > r && b > g_scaled) is_target_color = 1;
            }

            if (is_target_color) {
                // 累加坐标和像素数，用于计算质心
                sum_x += x;
                sum_y += y;
                pixel_count++;

                // 更新边界框
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }

    // 3. 特征提取 和 4. 目标筛选
    if (pixel_count > MIN_PIXEL_COUNT) {
        int width = max_x - min_x + 1;
        int height = max_y - min_y + 1;

        // 形状检查1: 宽高比
        float aspect_ratio = (float)width / height;
        if (aspect_ratio < ASPECT_RATIO_MIN || aspect_ratio > ASPECT_RATIO_MAX) {
            return ball; // 形状太扁或太长，不是球
        }

        // 形状检查2: 填充率
        int bbox_area = width * height;
        float fill_factor = (float)pixel_count / bbox_area;
        if (fill_factor < FILL_FACTOR_MIN) {
            return ball; // 物体太稀疏（如'L'形），不是实心球
        }

        // 所有检查通过，计算最终结果
        ball.cx = sum_x / pixel_count;
        ball.cy = sum_y / pixel_count;
        ball.pixel_count = pixel_count;
        // 估算半径：取宽高平均值的一半
        ball.radius = (width + height) / 4;
        ball.found = 1;
    }

    return ball;
}


void downscale_image(const uint8_t *src, uint8_t *dst, int src_w, int src_h) {
    int dst_w = src_w / 2;
    int dst_h = src_h / 2;
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            int src_x = x * 2;
            int src_y = y * 2;
            uint32_t sum = src[(src_y * src_w) + src_x] +           // 左上
                           src[(src_y * src_w) + src_x + 1] +       // 右上
                           src[((src_y + 1) * src_w) + src_x] +     // 左下
                           src[((src_y + 1) * src_w) + src_x + 1];  // 右下
            dst[y * dst_w + x] = sum / 4;
        }
    }
}


/**
 * @brief       将RGB565格式的图像转换为8位灰度图像。
 * @param       rgb565  输入，指向源RGB565图像数据的指针。
 *                      (函数名中的rgb556应为rgb565，这里按实际功能编写)
 * @param       gray    输出，指向目标8位灰度图像缓冲区的指针。
 *                      该缓冲区的大小必须至少为 width * height 字节。
 * @param       width   图像的宽度（像素）。
 * @param       height  图像的高度（像素）。
 */
void rgb565_to_gray(const uint8_t *rgb565, uint8_t *gray, int width, int height)
{
    if (rgb565 == NULL || gray == NULL)
    {
        return;
    }
    uint32_t num_pixels = (uint32_t)width * height;
    const uint16_t *p_src = (const uint16_t *)rgb565;
    uint8_t *p_dst = gray;

    for (uint32_t i = 0; i < num_pixels; i++)
    {
        uint16_t color16 = p_src[i];
        uint8_t r8 = (color16 & 0xF800) >> 8;
        uint8_t g8 = (color16 & 0x07E0) >> 3;
        uint8_t b8 = (color16 & 0x001F) << 3;
        uint32_t gray_temp = (uint32_t)r8 * 77 + (uint32_t)g8 * 150 + (uint32_t)b8 * 29;
        p_dst[i] = (uint8_t)(gray_temp >> 8);
    }
}

/**
 * @brief       将8位灰度图像转换为RGB565格式的图像。
 * @param       gray    输入，指向源8位灰度图像数据的指针。
 * @param       rgb565  输出，指向目标16位RGB565图像缓冲区的指针。
 *                      该缓冲区大小必须至少为 width * height * 2 字节。
 * @param       width   图像的宽度（像素）。
 * @param       height  图像的高度（像素）。
 */
void gray_to_rgb565(const uint8_t *gray, uint8_t *rgb565, int width, int height)
{
    if (gray == NULL || rgb565 == NULL)
    {
        return;
    }

    uint32_t num_pixels = (uint32_t)width * height;
    const uint8_t *p_src = gray;
    uint16_t *p_dst = (uint16_t *)rgb565;

    for (uint32_t i = 0; i < num_pixels; i++)
    {
        uint8_t gray_value = p_src[i];
        uint16_t r5 = gray_value >> 3;
        uint16_t g6 = gray_value >> 2;
        uint16_t b5 = gray_value >> 3;
        p_dst[i] = (r5 << 11) | (g6 << 5) | b5;
    }
}