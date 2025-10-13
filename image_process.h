/**
 ****************************************************************************************************
 * @file        image_process.h
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
#ifndef _IMAGE_PROCESS_H
#define _IMAGE_PROCESS_H

#define QRCODE_COUNT 3
#define CAMERA_WIDTH 320
#define CAMERA_HEIGHT 240

// --- incbin ������� ---
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#define INCBIN_PREFIX
#define INVALID_DATA -1


// --- 小球识别部分相关参数 ---
#define SATURATION_THRESHOLD 5 
#define BRIGHTNESS_THRESHOLD 6 

#define MIN_PIXEL_COUNT 50
#define ASPECT_RATIO_MIN 0.7f
#define ASPECT_RATIO_MAX 1.4f
#define FILL_FACTOR_MIN 0.6f



#include <stdint.h>


typedef struct
{
    uint8_t *addr;
    uint16_t width;
    uint16_t height;
    uint16_t pixel;
} image_t;


int image_init(image_t *image);
void image_deinit(image_t *image);
void image_crop(image_t *image_src, image_t *image_dst, uint16_t x_offset, uint16_t y_offset);
void image_draw(image_t *image_src, image_t *image_dst, uint16_t x_start, uint16_t y_start);
void image_resize(image_t *image_src, image_t *image_dst);

void image_rgb888_to_gray(uint8_t *image_addr, uint16_t image_width, uint16_t image_height);
void image_replace(uint8_t *image_addr, uint16_t image_width, uint16_t image_height,uint8_t vflip,uint8_t hmirror);
void image_invert(uint8_t *image_addr, uint16_t image_width, uint16_t image_height);
void image_strech_chart(uint8_t *image_addr, uint16_t image_width, uint16_t image_height, uint8_t de_dark);
void rgb888_to_rgb565(uint8_t *src_addr, uint16_t *dest_addr, uint16_t image_width, uint16_t image_height);

void draw_string_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t image_height, uint16_t x, uint16_t y, char *str, uint16_t color);
void draw_box_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void draw_point_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t x, uint16_t y, uint16_t color);
void draw_fill_rectangle_image(uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);




// --- 小球识别部分相关结构体与函数 ---
enum COLOR{
    BALL_UNKNOWN = 0,
    BALL_BLUE = 1,
    BALL_RED = 2
};

typedef struct {
    int cx;          // 小球中心点的 x 坐标
    int cy;          // 小球中心点的 y 坐标
    int radius;      // 小球的估算半径
    int pixel_count; //组成小球的像素点数量
    int found;      // 是否找到了小球
} BallInfo;



int start_color_recognize(uint8_t *snapshot_img_rgb565);
void fill_color_area(uint8_t *disp, enum COLOR color);
void fill_color_area_by_distance(uint8_t *disp, enum COLOR color);
BallInfo find_ball(uint8_t *img_rgb565, enum COLOR ball_color);
void downscale_image(const uint8_t *src, uint8_t *dst, int src_w, int src_h);
void rgb565_to_gray(const uint8_t *rgb565, uint8_t *gray, int width, int height);
void gray_to_rgb565(const uint8_t *gray, uint8_t *rgb565, int width, int height);

#endif /* _IMAGE_PROCESS_H */
