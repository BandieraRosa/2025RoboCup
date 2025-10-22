/**
 * @file image_process.h
 * @brief 图像处理库的头文件
 *
 * 该文件定义了图像处理相关的核心数据结构（image_t, BlobInfo）、
 * 颜色枚举、可配置的宏定义参数以及所有图像处理函数的原型。
 * 这些函数涵盖了图像的初始化、裁剪、缩放、颜色空间转换、
 * 绘图操作、形态学处理以及连通区域分析（Blob检测）等功能。
 */

#ifndef _IMAGE_PROCESS_H
#define _IMAGE_PROCESS_H

#include <stdint.h>

// --- 摄像头与图像尺寸定义 ---
#define CAMERA_WIDTH 320 // 摄像头或图像的宽度（单位：像素）。
#define CAMERA_HEIGHT 240 // 摄像头或图像的高度（单位：像素）。


// --- 颜色识别阈值定义 ---
// 注意：这些阈值参数对光照、摄像头白平衡和目标物体的具体颜色非常敏感。

/**
 * @brief 饱和度阈值
 * 用于过滤掉颜色不鲜明的像素（如灰色、白色、黑色）。
 * - **调优建议**:
 *   - **环境光线暗或目标颜色浅**: 可适当降低此值，以保留更多颜色信息。
 *   - **环境中有多种颜色干扰**: 可适当提高此值，以更精确地提取目标颜色。
 *   - **取值范围**: 0-255。
 */
#define SATURATION_THRESHOLD 50

/**
 * @brief 亮度阈值
 * 用于过滤掉过暗的像素。
 * - **调优建议**:
 *   - **环境光线昏暗**: 应降低此值，否则可能无法识别到目标。
 *   - **环境光线充足且有反光**: 可适当提高此值，以排除阴影或暗部区域的干扰。
 *   - **取值范围**: 0-255。
 */
#define BRIGHTNESS_THRESHOLD 50

/**
 * @brief 红蓝颜色差异阈值
 * 用于区分红色和蓝色。定义了一个像素的R通道值需要比B通道值大多少才被认为是红色（反之亦然）。
 * - **调优建议**:
 *   - **红色和蓝色目标颜色很纯正**: 可以提高此值，以减少误判。
 *   - **目标颜色偏紫或受环境光影响**: 可能需要降低此值，以适应不纯的颜色。
 *   - **取值范围**: 建议 10-100。
 */
#define COLOR_DIFF_THRESHOLD 60


// --- Blob（连通区域）筛选参数定义 ---
// 注意：这些参数用于从识别出的所有色块中筛选出符合条件的球体目标。
// 需要根据球体在图像中的大小、形状等实际情况进行调整。

/**
 * @brief 最小球体像素数
 * 一个连通区域（Blob）被认为是有效球体所需的最小像素数量。
 * - **调优建议**:
 *   - **球体距离摄像头较远，成像小**: 需要减小此值。
 *   - **场上存在大量小的颜色噪点**: 需要增大此值，以过滤掉这些噪点。
 */
#define MIN_BALL_PIXELS 5000

/**
 * @brief 最小宽高比
 * 用于筛选目标的形状。宽高比 = 宽度 / 高度。
 */
#define ASPECT_RATIO_MIN 0.3f

/**
 * @brief 最大宽高比
 * @see ASPECT_RATIO_MIN
 */
#define ASPECT_RATIO_MAX 1.6f

/**
 * @brief 最小填充因子
 * 填充因子 = Blob像素数 / (宽度 * 高度)，用于描述物体轮廓的饱满程度。
 * - **调优建议**:
 *   - **一个实心圆形填充因子约为 PI/4 ≈ 0.785**: 此参数可用于排除中空或不规则形状的物体。
 *   - **如果目标物体有反光或部分遮挡**: 可能需要降低此值。
 */
#define FILL_FACTOR_MIN 0.25f

/**
 * @brief 最大填充因子
 * 通常保持为1.0即可。
 * @see FILL_FACTOR_MIN
 */
#define FILL_FACTOR_MAX 1.0f

/**
 * @brief 图像数据结构
 *
 * 描述一个图像对象，采用平面（Planar）格式存储。
 */
typedef struct {
  uint8_t *addr;    /**< 指向图像数据缓冲区的指针 */
  uint16_t width;   /**< 图像宽度 */
  uint16_t height;  /**< 图像高度 */
  uint16_t pixel;   /**< 每个像素占用的字节数 */
} image_t;

/**
 * @brief Blob（连通区域）信息结构体
 *
 * 存储一个检测到的连通区域（色块）的属性信息。
 */
typedef struct {
  int id;           /**< Blob的唯一标识ID */
  int pixel_count;  /**< Blob包含的像素总数 */
  int min_x, min_y; /**< Blob的边界框左上角坐标 (min_x, min_y) */
  int max_x, max_y; /**< Blob的边界框右下角坐标 (max_x, max_y) */
} BlobInfo;

/**
 * @brief 颜色枚举
 * 定义了球体可能的颜色状态。
 */
enum COLOR { BALL_UNKNOWN = 0, BALL_BLUE = 1, BALL_RED = 2 };

// --- 图像内存管理 ---
int image_init(image_t *image);
void image_deinit(image_t *image);

// --- 图像几何变换与操作 ---
void image_crop(image_t *image_src, image_t *image_dst, uint16_t x_offset, uint16_t y_offset);
void image_draw(image_t *image_src, image_t *image_dst, uint16_t x_start, uint16_t y_start);
void image_resize(image_t *image_src, image_t *image_dst);
void image_replace(uint8_t *image_addr, uint16_t image_width, uint16_t image_height, uint8_t vflip, uint8_t hmirror);
void image_invert(uint8_t *image_addr, uint16_t image_width, uint16_t image_height);
void image_strech_chart(uint8_t *image_addr, uint16_t image_width, uint16_t image_height, uint8_t de_dark);

// --- 像素级颜色空间转换 ---
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

// --- 图像级颜色空间转换 ---
void rgb888_to_gray(const void *src_addr, void *dst_addr, int width, int height);
void gray_to_rgb888(const void *src_addr, void *dst_addr, int width, int height);
void rgb888_to_rgb565(const void *src_addr, void *dst_addr, int width, int height);
void rgb565_to_rgb888(const void *src_addr, void *dst_addr, int width, int height);
void gray_to_rgb565(const void *src_addr, void *dst_addr, int width, int height);
void rgb565_to_gray(const void *src_addr, void *dst_addr, int width, int height);
void rgb888_to_hsv(const void *src_addr, void *dst_addr, int width, int height);
void hsv_to_rgb888(const void *src_addr, void *dst_addr, int width, int height);
void rgb565_to_hsv(const void *src_addr, void *dst_addr, int width, int height);
void hsv_to_rgb565(const void *src_addr, void *dst_addr, int width, int height);

// --- 绘图函数 ---
void draw_string_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t image_height, uint16_t x, uint16_t y, char *str, uint16_t color);
void draw_box_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void draw_point_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t x, uint16_t y, uint16_t color);
void draw_fill_rectangle_image(uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

// --- 二值图像形态学操作 ---
void image_binary_open(uint8_t *binary_img, int width, int height, int kernel_size);
void image_binary_close(uint8_t *binary_img, int width, int height, int kernel_size);

// --- 连通区域分析 ---
int find_blobs(const uint8_t *binary_img, int width, int height, BlobInfo *blobs, int max_blobs);

#endif
