#ifndef _IMAGE_PROCESS_H
#define _IMAGE_PROCESS_H

#include <stdint.h>

/* ---------------- 应用/相机配置 ---------------- */
#ifndef CAMERA_WIDTH
#define CAMERA_WIDTH 320    /**< 相机图像宽度（像素） */
#endif
#ifndef CAMERA_HEIGHT
#define CAMERA_HEIGHT 240   /**< 相机图像高度（像素） */
#endif

/* ---------------- 颜色分割阈值 ---------------- */
#define SATURATION_THRESHOLD 40   /**< 最小色彩饱和度阈值，场景变暗/阴影下调，场景变亮/反光上调 */
#define BRIGHTNESS_THRESHOLD 60   /**< 最小亮度阈值，目标颜色变浅下调，背景杂色多上调 */
#define COLOR_DIFF_THRESHOLD 45   /**< 颜色主导阈值，目标颜色变浅下调，背景大块近似红蓝上调*/

/* ---------------- 球体候选过滤条件 ---------------- */
#define MIN_BALL_PIXELS 7000
#define ASPECT_RATIO_MIN  0.3f    /**< 包围盒宽高比下限 */
#define ASPECT_RATIO_MAX  1.6f    /**< 包围盒宽高比上限 */
#define FILL_FACTOR_MIN   0.25f   /**< 填充率下限：像素数/包围盒面积 */
#define FILL_FACTOR_MAX   1.0f    /**< 填充率上限 */

/* ---------------- 基本图像容器 ---------------- */
typedef struct {
    uint8_t  *addr;   /**< 通道平面交织缓冲区基址（RGB888 为 R、G、B 三平面顺序相连） */
    uint16_t  width;  /**< 图像宽度（像素） */
    uint16_t  height; /**< 图像高度（像素） */
    uint16_t  pixel;  /**< 每通道每像素字节数（例如 8 位灰度为 1） */
} image_t;

/* ---------------- 内存辅助 ---------------- */
int  image_init(image_t *image);     /**< 使用 iomem 分配器初始化图像缓冲，成功返回 0 */
void image_deinit(image_t *image);   /**< 释放由 iomem 分配的图像缓冲 */

/* ---------------- RGB888 图像操作 ---------------- */
void image_crop(image_t *image_src, image_t *image_dst, uint16_t x_offset, uint16_t y_offset);     /**< 裁剪（越界区域补 0） */
void image_draw(image_t *image_src, image_t *image_dst, uint16_t x_start, uint16_t y_start);       /**< 绘制到目标图像的指定起点（先清空目标） */
void image_resize(image_t *image_src, image_t *image_dst);                                         /**< 双线性缩放 */
void image_replace(uint8_t *image_addr, uint16_t image_width, uint16_t image_height, uint8_t vflip, uint8_t hmirror); /**< 垂直翻转/水平镜像（就地） */
void image_invert(uint8_t *image_addr, uint16_t image_width, uint16_t image_height);               /**< 反相（就地） */
void image_strech_chart(uint8_t *image_addr, uint16_t image_width, uint16_t image_height, uint8_t de_dark); /**< 简单拉伸+暗角补偿 */


/*
 * =================================================================================================
 * 颜色空间转换
 * =================================================================================================
 */

/* --- 像素级转换 --- */
void rgb888_to_rgb565_pixel(uint8_t r, uint8_t g, uint8_t b, uint16_t *rgb565);
void rgb565_to_rgb888_pixel(uint16_t rgb565, uint8_t *r, uint8_t *g, uint8_t *b);
void rgb888_to_gray_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t *gray);
void gray_to_rgb888_pixel(uint8_t gray, uint8_t *r, uint8_t *g, uint8_t *b);
void rgb565_to_gray_pixel(uint16_t rgb565, uint8_t *gray);
void gray_to_rgb565_pixel(uint8_t gray, uint16_t *rgb565);
void rgb888_to_hsv_pixel(uint8_t r, uint8_t g, uint8_t b, uint16_t *h, uint8_t *s, uint8_t *v);
void hsv_to_rgb888_pixel(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);
void rgb565_to_hsv_pixel(uint16_t rgb565, uint16_t *h, uint8_t *s, uint8_t *v);
void hsv_to_rgb565_pixel(uint16_t h, uint8_t s, uint8_t v, uint16_t *rgb565);

/* --- 图像级转换 --- */
void rgb888_to_gray (const void *src_addr, void *dst_addr, int width, int height);
void gray_to_rgb888 (const void *src_addr, void *dst_addr, int width, int height);
void rgb888_to_rgb565(const void *src_addr, void *dst_addr, int width, int height);
void rgb565_to_rgb888(const void *src_addr, void *dst_addr, int width, int height);
void gray_to_rgb565 (const void *src_addr, void *dst_addr, int width, int height);
void rgb565_to_gray (const void *src_addr, void *dst_addr, int width, int height);
void rgb888_to_hsv  (const void *src_addr, void *dst_addr, int width, int height);
void hsv_to_rgb888  (const void *src_addr, void *dst_addr, int width, int height);
void rgb565_to_hsv  (const void *src_addr, void *dst_addr, int width, int height);
void hsv_to_rgb565  (const void *src_addr, void *dst_addr, int width, int height);

/* ---------------- RGB565 绘图操作 ---------------- */
void draw_string_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t image_height, uint16_t x, uint16_t y, char *str, uint16_t color); /**< 绘制 8x16 ASCII 文本 */
void draw_box_rgb565_image   (uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);       /**< 绘制矩形边框 */
void draw_point_rgb565_image (uint16_t *image_addr, uint16_t image_width, uint16_t x, uint16_t y, uint16_t color);                                   /**< 绘制像素点 */
void draw_fill_rectangle_image(uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);     /**< 填充矩形 */

/* ---------------- 形态学（0/255 二值图） ---------------- */
void image_binary_open (uint8_t *binary_img, int width, int height, int kernel_size);   /**< 开运算：先腐蚀后膨胀 */
void image_binary_close(uint8_t *binary_img, int width, int height, int kernel_size);   /**< 闭运算：先膨胀后腐蚀 */

/* ---------------- 连通域（Blob）检测 ---------------- */
typedef struct {
    int id;               /**< 标签 ID（压缩后 1 起始） */
    int pixel_count;      /**< 像素数 */
    int min_x, min_y;     /**< 包围盒左上角 */
    int max_x, max_y;     /**< 包围盒右下角 */
} BlobInfo;

int find_blobs(const uint8_t *binary_img, int width, int height, BlobInfo *blobs, int max_blobs); /**< 返回实际检测到的 blob 数 */

enum COLOR{
    BALL_UNKNOWN = 0,
    BALL_BLUE    = 1,
    BALL_RED     = 2
};

#endif /* _IMAGE_PROCESS_H */
