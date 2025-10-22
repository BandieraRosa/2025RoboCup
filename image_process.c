#include "image_process.h"
#include "./BSP/LCD/lcdfont.h"
#include "iomem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 检查图像缓冲区是否已准备就绪
 *
 * 验证图像对象的有效性，包括检查图像指针、数据地址、宽度、高度和像素字节数是否有效。
 *
 * @param[in] image 指向要检查的图像对象的指针。
 * @return int 如果图像缓冲区准备就绪返回1(真)，否则返回0(假)。
 */
static inline int image_buffer_ready(const image_t *image) {
  return (image != NULL) && (image->addr != NULL) && (image->width > 0U) &&
         (image->height > 0U) && (image->pixel > 0U);
}

/**
 * @brief 计算图像单个颜色平面的像素数量
 *
 * 计算图像中单个颜色平面（如R、G或B平面）所需的像素数量。
 * 在平面格式存储的图像中，每个颜色通道都有独立的平面，此函数用于计算单个平面的像素数。
 *
 * @param[in] image 指向图像对象的指针。
 * @return size_t 返回单个颜色平面的像素数量（宽度×高度）。
 */
static inline size_t image_plane_size(const image_t *image) {
  return (size_t)image->width * (size_t)image->height;
}

/**
 * @brief 初始化图像对象并分配内存
 *
 * 为图像对象分配所需的内存空间。函数首先检查图像对象的有效性，
 * 然后计算所需内存大小，进行溢出检查，最后分配内存。
 *
 * @param[in,out] image 指向要初始化的图像对象的指针。其 width, height, pixel
 * 成员应已设置。
 * @return int 成功返回0，失败返回-1。
 * @note 失败原因可能包括：
 *       - 图像对象无效（`image_buffer_ready`检查失败）。
 *       - 计算的内存大小为0或超出系统限制。
 *       - 内存分配失败 (`iomem_malloc` 返回 NULL)。
 */
int image_init(image_t *image) {
  if (!image_buffer_ready(image))
    return -1;
  size_t plane = image_plane_size(image);
  // 检查乘法是否会导致溢出
  if (plane == 0U || plane > (SIZE_MAX / image->pixel))
    return -1;
  image->addr = iomem_malloc(plane * image->pixel);
  if (image->addr == NULL)
    return -1;
  return 0;
}

/**
 * @brief 释放图像对象占用的内存
 *
 * 释放图像对象的数据缓冲区内存，并将地址指针设置为NULL。
 * 这是 `image_init` 函数的对应操作，用于清理不再使用的图像资源。
 *
 * @param[in,out] image 指向要释放的图像对象的指针。
 * @note 函数会先检查图像对象和地址指针的有效性，确保安全地释放内存。
 *       释放后将 `image->addr` 设为 `NULL`，以防止悬挂指针。
 */
void image_deinit(image_t *image) {
  if (image && image->addr) {
    iomem_free(image->addr);
    image->addr = NULL;
  }
}

/**
 * @brief 从源图像中裁剪一个区域到目标图像
 *
 * 从源图像中裁剪指定偏移位置开始的区域，并将其复制到目标图像中。
 * 函数会先验证源图像和目标图像的有效性，然后清空目标图像，
 * 最后按照RGB平面格式逐行复制像素数据。
 *
 * @param[in]  image_src 指向源图像对象的指针。
 * @param[out] image_dst 指向目标图像对象的指针，其缓冲区将被覆盖。
 * @param[in]  x_offset  源图像中裁剪区域的X轴偏移量。
 * @param[in]  y_offset  源图像中裁剪区域的Y轴偏移量。
 * @note 假定图像数据为Planar RGB格式。
 */
void image_crop(image_t *image_src, image_t *image_dst, uint16_t x_offset,
                uint16_t y_offset) {
  // 检查源图像和目标图像是否准备就绪
  if (!image_buffer_ready(image_src) || !image_buffer_ready(image_dst))
    return;

  // 计算目标图像平面大小并检查内存分配条件
  size_t dst_plane = image_plane_size(image_dst);
  if (dst_plane == 0U || dst_plane > (SIZE_MAX / image_dst->pixel))
    return;
  // 清空目标图像
  memset(image_dst->addr, 0, dst_plane * image_dst->pixel);

  // 获取源图像和目标图像的尺寸
  uint16_t w_src = image_src->width, h_src = image_src->height;
  uint16_t w_dst = image_dst->width, h_dst = image_dst->height;
  // 检查偏移量是否超出源图像范围
  if (x_offset >= w_src || y_offset >= h_src)
    return;

  // 计算实际可复制的宽度和高度，防止越界
  size_t copy_w =
      (size_t)((w_src - x_offset) < w_dst ? (w_src - x_offset) : w_dst);
  size_t copy_h =
      (size_t)((h_src - y_offset) < h_dst ? (h_src - y_offset) : h_dst);

  // 计算源图像和目标图像的行字节数（对于单平面）
  size_t src_line = (size_t)w_src;
  size_t dst_line = (size_t)w_dst;

  // 计算源图像中各颜色平面的裁剪起始位置
  uint8_t *r_src = image_src->addr + y_offset * src_line + x_offset;
  uint8_t *g_src = r_src + src_line * h_src; // G平面在R平面之后
  uint8_t *b_src = g_src + src_line * h_src; // B平面在G平面之后

  // 计算目标图像中各颜色平面的起始位置
  uint8_t *r_dst = image_dst->addr;
  uint8_t *g_dst = r_dst + dst_line * h_dst;
  uint8_t *b_dst = g_dst + dst_line * h_dst;

  // 逐行复制RGB像素数据
  for (size_t y = 0; y < copy_h; ++y) {
    memcpy(r_dst + y * dst_line, r_src + y * src_line, copy_w);
    memcpy(g_dst + y * dst_line, g_src + y * src_line, copy_w);
    memcpy(b_dst + y * dst_line, b_src + y * src_line, copy_w);
  }
}

/**
 * @brief 将源图像绘制到目标图像的指定位置
 *
 * 将源图像完整地绘制到目标图像的指定起始位置。
 * 函数会先验证源图像和目标图像的有效性，然后计算实际可绘制的区域，
 * 最后按照RGB平面格式逐行复制像素数据。
 *
 * @param[in]  image_src 指向源图像对象的指针。
 * @param[out] image_dst 指向目标图像对象的指针。
 * @param[in]  x_start   目标图像中绘制区域左上角的X轴起始位置。
 * @param[in]  y_start   目标图像中绘制区域左上角的Y轴起始位置。
 * @note 假定图像数据为Planar RGB格式。
 */
void image_draw(image_t *image_src, image_t *image_dst, uint16_t x_start,
                uint16_t y_start) {
  // 检查源图像和目标图像是否准备就绪
  if (!image_buffer_ready(image_src) || !image_buffer_ready(image_dst))
    return;

  // 获取源图像和目标图像的尺寸
  uint16_t w_src = image_src->width, h_src = image_src->height;
  uint16_t w_dst = image_dst->width, h_dst = image_dst->height;
  // 检查起始位置是否超出目标图像范围
  if (x_start >= w_dst || y_start >= h_dst)
    return;

  // 计算实际可绘制的宽度和高度，防止越界
  size_t copy_w =
      (size_t)(w_src < (w_dst - x_start) ? w_src : (w_dst - x_start));
  size_t copy_h =
      (size_t)(h_src < (h_dst - y_start) ? h_src : (h_dst - y_start));

  // 计算源图像和目标图像的行字节数
  size_t src_line = (size_t)w_src;
  size_t dst_line = (size_t)w_dst;
  // 计算目标图像中的一维偏移量
  size_t dst_off = (size_t)y_start * dst_line + x_start;

  // 计算源图像中各颜色平面的起始位置
  uint8_t *r_src = image_src->addr;
  uint8_t *g_src = r_src + src_line * h_src;
  uint8_t *b_src = g_src + src_line * h_src;

  // 计算目标图像中各颜色平面的绘制起始位置（考虑偏移量）
  uint8_t *r_dst = image_dst->addr + dst_off;
  uint8_t *g_dst = image_dst->addr + dst_line * h_dst + dst_off;
  uint8_t *b_dst = image_dst->addr + 2U * dst_line * h_dst + dst_off;

  // 逐行复制RGB像素数据
  for (size_t y = 0; y < copy_h; ++y) {
    memcpy(r_dst + y * dst_line, r_src + y * src_line, copy_w);
    memcpy(g_dst + y * dst_line, g_src + y * src_line, copy_w);
    memcpy(b_dst + y * dst_line, b_src + y * src_line, copy_w);
  }
}

/**
 * @brief 调整图像尺寸
 *
 * 使用双线性插值算法将源图像缩放到目标图像的尺寸。
 * 该算法通过计算目标图像中每个像素对应的源图像坐标，
 * 并使用周围四个像素的加权平均值来确定目标像素的值。
 *
 * @param[in]  image_src 指向源图像对象的指针。
 * @param[out] image_dst 指向目标图像对象的指针，其尺寸决定了缩放的目标大小。
 */
void image_resize(image_t *image_src, image_t *image_dst) {
  // 获取源图像的尺寸和RGB平面指针
  uint16_t w_src = image_src->width, h_src = image_src->height;
  uint8_t *r_src = image_src->addr;
  uint8_t *g_src = r_src + w_src * h_src;
  uint8_t *b_src = g_src + w_src * h_src;

  // 获取目标图像的尺寸和RGB平面指针
  uint16_t w_dst = image_dst->width, h_dst = image_dst->height;
  uint8_t *r_dst = image_dst->addr;
  uint8_t *g_dst = r_dst + w_dst * h_dst;
  uint8_t *b_dst = g_dst + w_dst * h_dst;

  // 计算缩放比例
  float w_scale = (float)w_src / (float)w_dst;
  float h_scale = (float)h_src / (float)h_dst;

  // 遍历目标图像的每个像素
  for (uint16_t y = 0; y < h_dst; y++) {
    for (uint16_t x = 0; x < w_dst; x++) {
      // 计算目标像素在源图像中的对应浮点坐标（对齐像素中心）
      float x_src = (x + 0.5f) * w_scale - 0.5f;
      float y_src = (y + 0.5f) * h_scale - 0.5f;
      // 获取周围四个像素的整数坐标
      uint16_t x1 = (uint16_t)x_src;
      uint16_t y1 = (uint16_t)y_src;
      uint16_t x2 = x1 + 1;
      uint16_t y2 = y1 + 1;

      // 边界检查：如果周围像素超出源图像范围，则使用最近邻插值
      if (x2 >= w_src || y2 >= h_src) {
        r_dst[x + y * w_dst] = r_src[x1 + y1 * w_src];
        g_dst[x + y * w_dst] = g_src[x1 + y1 * w_src];
        b_dst[x + y * w_dst] = b_src[x1 + y1 * w_src];
        continue;
      }

      // 对红色通道进行双线性插值
      // 1. 在x方向上进行两次线性插值
      float t1 = (x2 - x_src) * r_src[x1 + y1 * w_src] +
                 (x_src - x1) * r_src[x2 + y1 * w_src];
      float t2 = (x2 - x_src) * r_src[x1 + y2 * w_src] +
                 (x_src - x1) * r_src[x2 + y2 * w_src];
      // 2. 在y方向上对前两次插值的结果进行线性插值
      r_dst[x + y * w_dst] = (uint8_t)((y2 - y_src) * t1 + (y_src - y1) * t2);

      // 对绿色通道进行双线性插值
      t1 = (x2 - x_src) * g_src[x1 + y1 * w_src] +
           (x_src - x1) * g_src[x2 + y1 * w_src];
      t2 = (x2 - x_src) * g_src[x1 + y2 * w_src] +
           (x_src - x1) * g_src[x2 + y2 * w_src];
      g_dst[x + y * w_dst] = (uint8_t)((y2 - y_src) * t1 + (y_src - y1) * t2);

      // 对蓝色通道进行双线性插值
      t1 = (x2 - x_src) * b_src[x1 + y1 * w_src] +
           (x_src - x1) * b_src[x2 + y1 * w_src];
      t2 = (x2 - x_src) * b_src[x1 + y2 * w_src] +
           (x_src - x1) * b_src[x2 + y2 * w_src];
      b_dst[x + y * w_dst] = (uint8_t)((y2 - y_src) * t1 + (y_src - y1) * t2);
    }
  }
}

/**
 * @brief 对图像进行垂直翻转和/或水平镜像处理
 *
 * 根据参数对图像进行垂直翻转和/或水平镜像处理。
 * 垂直翻转通过交换上下对称行的像素实现，水平镜像通过交换左右对称列的像素实现。
 * 函数处理RGB平面格式的图像数据。
 *
 * @param[in,out] image_addr 图像数据缓冲区地址。
 * @param[in] image_width  图像宽度（像素数）。
 * @param[in] image_height 图像高度（像素数）。
 * @param[in] vflip        垂直翻转标志，1表示执行垂直翻转，0表示不执行。
 * @param[in] hmirror      水平镜像标志，1表示执行水平镜像，0表示不执行。
 */
void image_replace(uint8_t *image_addr, uint16_t image_width,
                   uint16_t image_height, uint8_t vflip, uint8_t hmirror) {
  uint8_t *src = image_addr, *r_src, *g_src, *b_src, temp;
  uint16_t t1, t2;
  uint32_t o1, o2;

  // --- 垂直翻转 ---
  if (vflip == 1) {
    // 只需要遍历图像高度的一半
    t1 = image_height >> 1;
    // 获取RGB三个平面的起始地址
    r_src = src;
    g_src = r_src + image_width * image_height;
    b_src = g_src + image_width * image_height;

    // 逐行交换上下对称的像素
    for (uint16_t j = 0; j < t1; j++) {
      for (uint16_t i = 0; i < image_width; i++) {
        // 计算上下对称像素的一维偏移量
        o1 = (uint32_t)image_width * j + i;
        o2 = (uint32_t)image_width * (image_height - 1 - j) + i;

        // 交换红色通道的像素
        temp = r_src[o1];
        r_src[o1] = r_src[o2];
        r_src[o2] = temp;
        // 交换绿色通道的像素
        temp = g_src[o1];
        g_src[o1] = g_src[o2];
        g_src[o2] = temp;
        // 交换蓝色通道的像素
        temp = b_src[o1];
        b_src[o1] = b_src[o2];
        b_src[o2] = temp;
      }
    }
  }

  // --- 水平镜像 ---
  if (hmirror == 1) {
    // 只需要处理图像宽度的一半
    t2 = image_width >> 1;
    // 遍历所有颜色平面的所有行 (共 image_height * 3 行)
    for (uint16_t j = 0; j < image_height * 3; j++) {
      // 逐列交换左右对称的像素
      for (uint16_t i = 0; i < t2; i++) {
        // 计算左右对称像素的列偏移量
        o1 = image_width - i - 1;
        // 交换像素
        temp = src[i];
        src[i] = src[o1];
        src[o1] = temp;
      }
      // 移动到下一行的起始位置
      src += image_width;
    }
  }
}

/**
 * @brief 对图像进行颜色反转（负片效果）
 *
 * 将图像中每个像素的RGB值取反，实现负片效果。
 * 对于每个颜色通道，使用按位取反操作（`~`）将0变为255，255变为0等。
 * 函数处理RGB平面格式的图像数据。
 *
 * @param[in,out] image_addr 图像数据缓冲区地址。
 * @param[in] image_width  图像宽度（像素数）。
 * @param[in] image_height 图像高度（像素数）。
 */
void image_invert(uint8_t *image_addr, uint16_t image_width,
                  uint16_t image_height) {
  // 获取RGB三个平面的起始地址
  uint8_t *r = image_addr;
  uint8_t *g = r + image_width * image_height;
  uint8_t *b = g + image_width * image_height;
  uint32_t total_pixels = (uint32_t)image_width * image_height;

  // 遍历所有像素
  for (uint32_t j = 0; j < total_pixels; j++) {
    // 对每个颜色通道进行按位取反操作
    *r = (uint8_t)~(*r);
    *g = (uint8_t)~(*g);
    *b = (uint8_t)~(*b);
    // 移动到下一个像素
    r++;
    g++;
    b++;
  }
}

/**
 * @brief 对图像进行灰度拉伸和可选的中心亮度衰减处理
 *
 * 这是一个两遍(Two-pass)算法：
 * 1. 第一遍：遍历图像，统计最大灰度值、灰度总和等信息，并计算平均灰度值。
 * 2. 第二遍：再次遍历图像，根据第一遍的统计结果对每个像素执行灰度拉伸，
 *    并可选择性地应用一个中心加权的亮度衰减效果。
 *
 * @param[in,out] image_addr
 * 图像数据缓冲区地址，应为RGB平面格式。函数会就地修改数据。
 * @param[in] image_width    图像宽度（像素数）。
 * @param[in] image_height   图像高度（像素数）。
 * @param[in] de_dark        中心亮度衰减标志，1表示应用，0表示不应用。
 * @note 该函数假定输入是彩色图像，但会将其转换为灰度图（R, G, B三通道值相同）。
 *       它仅使用R通道作为计算灰度拉伸的输入。
 */
void image_strech_chart(uint8_t *image_addr, uint16_t image_width,
                        uint16_t image_height, uint8_t de_dark) {
  // 获取RGB三个平面的起始地址
  uint8_t *r = image_addr;
  uint8_t *g = r + image_width * image_height;
  uint8_t *b = g + image_width * image_height;
  // 使用红色通道作为输入数据进行处理
  uint8_t *in = r;
  uint32_t idx;
  uint16_t graymax = 0; // 图像中的最大灰度值
  int sx = 0, sx2 = 0;  // 灰度值的总和与平方和，用于统计计算
  uint32_t total_pixels = (uint32_t)image_width * image_height;

  // --- 第一遍遍历：统计图像的灰度信息 ---
  for (idx = 0; idx < total_pixels; idx++) {
    // 找出图像中的最大灰度值
    if (in[idx] > graymax)
      graymax = in[idx];
    // 计算灰度值的总和
    sx += in[idx];
    // 计算灰度值的平方和 (当前版本未使用，可用于计算方差等)
    sx2 += ((int)in[idx] * (int)in[idx]);
  }

  // 计算图像的平均灰度值
  int ex = sx / total_pixels;
  // 使用平均灰度值作为拉伸的下限阈值
  int gate = ex;

  // --- 第二遍遍历：执行灰度拉伸和可选的中心亮度衰减 ---
  for (idx = 0; idx < total_pixels; idx++) {
    // 计算当前像素的(x, y)坐标
    uint16_t x = (uint16_t)(idx % image_width);
    uint16_t y = (uint16_t)(idx / image_width);
    // 获取当前像素的灰度值
    int dat = in[idx];
    // 计算拉伸公式的分母 (graymax - gate)
    int denom = (int)graymax - gate;
    // 防止除零错误
    if (denom == 0)
      denom = 1;

    // 执行灰度拉伸：将[gate, graymax]范围线性映射到[0, 255]范围
    dat = (dat - gate) * 255 / denom;
    // 钳位操作，确保结果在有效范围内[0, 255]
    dat = dat < 0 ? 0 : (dat > 255 ? 255 : dat);

    // 如果启用中心亮度衰减
    if (de_dark) {
      // 计算当前像素到图像中心的距离的平方
      int r2 = (x - image_width / 2) * (x - image_width / 2) +
               (y - image_height / 2) * (y - image_height / 2);
      // 应用中心亮度衰减：距离中心越远，亮度衰减越强
      // 衰减因子与r2的平方成正比，衰减效果非常显著
      dat = (int)(dat / (1.0 + 32.0 * r2 * r2 / image_width / image_width /
                                   image_height / image_height));
    }

    // 将处理后的灰度值写入RGB三个通道，生成灰度图像
    in[idx] = (uint8_t)dat;
    g[idx] = (uint8_t)dat;
    b[idx] = (uint8_t)dat;
  }
}

void rgb888_to_rgb565_pixel(uint8_t r, uint8_t g, uint8_t b, uint16_t *rgb565) {
  *rgb565 = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

void rgb565_to_rgb888_pixel(uint16_t rgb565, uint8_t *r, uint8_t *g,
                            uint8_t *b) {
  uint8_t r5 = (uint8_t)((rgb565 >> 11) & 0x1F);
  uint8_t g6 = (uint8_t)((rgb565 >> 5) & 0x3F);
  uint8_t b5 = (uint8_t)(rgb565 & 0x1F);
  *r = (uint8_t)((r5 << 3) | (r5 >> 2));
  *g = (uint8_t)((g6 << 2) | (g6 >> 4));
  *b = (uint8_t)((b5 << 3) | (b5 >> 2));
}

void rgb888_to_gray_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t *gray) {
  *gray = (uint8_t)((306 * r + 601 * g + 117 * b) >> 10);
}

void gray_to_rgb888_pixel(uint8_t gray, uint8_t *r, uint8_t *g, uint8_t *b) {
  *r = gray;
  *g = gray;
  *b = gray;
}

void rgb565_to_gray_pixel(uint16_t rgb565, uint8_t *gray) {
  uint8_t r, g, b;
  rgb565_to_rgb888_pixel(rgb565, &r, &g, &b);
  rgb888_to_gray_pixel(r, g, b, gray);
}

void rgb888_to_hsv_pixel(uint8_t r, uint8_t g, uint8_t b, uint16_t *h,
                         uint8_t *s, uint8_t *v) {
  uint8_t min_val = (uint8_t)((r < g) ? r : g);
  min_val = (uint8_t)((min_val < b) ? min_val : b);
  uint8_t max_val = (uint8_t)((r > g) ? r : g);
  max_val = (uint8_t)((max_val > b) ? max_val : b);

  *v = max_val;
  if (max_val == 0) {
    *s = 0;
    *h = 0;
    return;
  }

  int32_t delta = (int32_t)max_val - (int32_t)min_val;
  if (delta == 0) {
    *s = 0;
    *h = 0;
    return;
  }

  *s = (uint8_t)(255 * delta / max_val);

  int32_t hue;
  if (r == max_val)
    hue = (int32_t)(g - b) * 60 / delta;
  else if (g == max_val)
    hue = 120 + (int32_t)(b - r) * 60 / delta;
  else
    hue = 240 + (int32_t)(r - g) * 60 / delta;

  if (hue < 0)
    hue += 360;
  *h = (uint16_t)hue;
}

void hsv_to_rgb888_pixel(uint16_t h, uint8_t s, uint8_t v, uint8_t *r,
                         uint8_t *g, uint8_t *b) {
  if (s == 0) {
    *r = v;
    *g = v;
    *b = v;
    return;
  }

  h %= 360;
  uint8_t region = (uint8_t)(h / 60);
  uint16_t rem = (uint16_t)((h % 60) * 255 / 60);

  uint16_t p = (uint16_t)(v * (255 - s) / 255);
  uint16_t q = (uint16_t)(v * (255 * 255 - s * rem) / (255 * 255));
  uint16_t t = (uint16_t)(v * (255 * 255 - s * (255 - rem)) / (255 * 255));

  switch (region) {
  case 0:
    *r = v;
    *g = (uint8_t)t;
    *b = (uint8_t)p;
    break;
  case 1:
    *r = (uint8_t)q;
    *g = v;
    *b = (uint8_t)p;
    break;
  case 2:
    *r = (uint8_t)p;
    *g = v;
    *b = (uint8_t)t;
    break;
  case 3:
    *r = (uint8_t)p;
    *g = (uint8_t)q;
    *b = v;
    break;
  case 4:
    *r = (uint8_t)t;
    *g = (uint8_t)p;
    *b = v;
    break;
  default:
    *r = v;
    *g = (uint8_t)p;
    *b = (uint8_t)q;
    break;
  }
}

void rgb565_to_hsv_pixel(uint16_t rgb565, uint16_t *h, uint8_t *s, uint8_t *v) {
  uint8_t r, g, b;
  rgb565_to_rgb888_pixel(rgb565, &r, &g, &b);
  rgb888_to_hsv_pixel(r, g, b, h, s, v);
}

void hsv_to_rgb565_pixel(uint16_t h, uint8_t s, uint8_t v, uint16_t *rgb565) {
  uint8_t r, g, b;
  hsv_to_rgb888_pixel(h, s, v, &r, &g, &b);
  rgb888_to_rgb565_pixel(r, g, b, rgb565);
}

void rgb888_to_gray(const void *src_addr, void *dst_addr, int width,
                    int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint8_t *r = (const uint8_t *)src_addr;
  size_t plane = (size_t)width * (size_t)height;
  const uint8_t *g = r + plane;
  const uint8_t *b = g + plane;
  uint8_t *dst = (uint8_t *)dst_addr;

  for (size_t i = 0; i < plane; i++)
    rgb888_to_gray_pixel(r[i], g[i], b[i], &dst[i]);
}

void gray_to_rgb888(const void *src_addr, void *dst_addr, int width,
                    int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint8_t *src = (const uint8_t *)src_addr;
  size_t plane = (size_t)width * (size_t)height;
  uint8_t *r = (uint8_t *)dst_addr;
  uint8_t *g = r + plane;
  uint8_t *b = g + plane;

  for (size_t i = 0; i < plane; i++) {
    r[i] = src[i];
    g[i] = src[i];
    b[i] = src[i];
  }
}

void rgb888_to_rgb565(const void *src_addr, void *dst_addr, int width,
                      int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint8_t *r = (const uint8_t *)src_addr;
  size_t plane = (size_t)width * (size_t)height;
  const uint8_t *g = r + plane;
  const uint8_t *b = g + plane;
  uint16_t *dst = (uint16_t *)dst_addr;

  for (size_t i = 0; i < plane; i++)
    rgb888_to_rgb565_pixel(r[i], g[i], b[i], &dst[i]);
}

void rgb565_to_rgb888(const void *src_addr, void *dst_addr, int width,
                      int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint16_t *src = (const uint16_t *)src_addr;
  size_t plane = (size_t)width * (size_t)height;
  uint8_t *r = (uint8_t *)dst_addr;
  uint8_t *g = r + plane;
  uint8_t *b = g + plane;

  for (size_t i = 0; i < plane; i++)
    rgb565_to_rgb888_pixel(src[i], &r[i], &g[i], &b[i]);
}

void gray_to_rgb565(const void *src_addr, void *dst_addr, int width,
                    int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint8_t *src = (const uint8_t *)src_addr;
  uint16_t *dst = (uint16_t *)dst_addr;
  size_t n = (size_t)width * (size_t)height;

  for (size_t i = 0; i < n; i++)
    gray_to_rgb565_pixel(src[i], &dst[i]);
}

void gray_to_rgb565_pixel(uint8_t gray, uint16_t *rgb565) {
  *rgb565 =
      (uint16_t)(((gray & 0xF8) << 8) | ((gray & 0xFC) << 3) | (gray >> 3));
}

void rgb565_to_gray(const void *src_addr, void *dst_addr, int width,
                    int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint16_t *src = (const uint16_t *)src_addr;
  uint8_t *dst = (uint8_t *)dst_addr;
  size_t n = (size_t)width * (size_t)height;

  for (size_t i = 0; i < n; i++)
    rgb565_to_gray_pixel(src[i], &dst[i]);
}

void rgb888_to_hsv(const void *src_addr, void *dst_addr, int width,
                   int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint8_t *r = (const uint8_t *)src_addr;
  size_t plane = (size_t)width * (size_t)height;
  const uint8_t *g = r + plane;
  const uint8_t *b = g + plane;

  uint16_t *h = (uint16_t *)dst_addr;
  uint8_t *s = (uint8_t *)(h + plane);
  uint8_t *v = s + plane;

  for (size_t i = 0; i < plane; i++)
    rgb888_to_hsv_pixel(r[i], g[i], b[i], &h[i], &s[i], &v[i]);
}

void hsv_to_rgb888(const void *src_addr, void *dst_addr, int width,
                   int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint16_t *h = (const uint16_t *)src_addr;
  size_t plane = (size_t)width * (size_t)height;
  const uint8_t *s = (const uint8_t *)(h + plane);
  const uint8_t *v = s + plane;
  uint8_t *r = (uint8_t *)dst_addr;
  uint8_t *g = r + plane;
  uint8_t *b = g + plane;

  for (size_t i = 0; i < plane; i++)
    hsv_to_rgb888_pixel(h[i], s[i], v[i], &r[i], &g[i], &b[i]);
}

void rgb565_to_hsv(const void *src_addr, void *dst_addr, int width,
                   int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint16_t *src = (const uint16_t *)src_addr;
  size_t plane = (size_t)width * (size_t)height;
  uint16_t *h = (uint16_t *)dst_addr;
  uint8_t *s = (uint8_t *)(h + plane);
  uint8_t *v = s + plane;

  for (size_t i = 0; i < plane; i++)
    rgb565_to_hsv_pixel(src[i], &h[i], &s[i], &v[i]);
}

void hsv_to_rgb565(const void *src_addr, void *dst_addr, int width,
                   int height) {
  if (!src_addr || !dst_addr || width <= 0 || height <= 0)
    return;

  const uint16_t *h = (const uint16_t *)src_addr;
  size_t plane = (size_t)width * (size_t)height;
  const uint8_t *s = (const uint8_t *)(h + plane);
  const uint8_t *v = s + plane;
  uint16_t *dst = (uint16_t *)dst_addr;

  for (size_t i = 0; i < plane; i++)
    hsv_to_rgb565_pixel(h[i], s[i], v[i], &dst[i]);
}

/**
 * @brief 在RGB565格式的图像上绘制字符串
 *
 * 使用预定义的8x16 ASCII字模 (ascii0816)
 * 在图像的指定位置绘制一个空结尾的字符串。
 *
 * @param[in,out] image_addr 指向RGB565图像数据缓冲区的指针。
 * @param[in] image_width  图像的宽度（单位：像素）。
 * @param[in] image_height 图像的高度（单位：像素）。
 * @param[in] x            字符串左上角的起始X坐标。
 * @param[in] y            字符串左上角的起始Y坐标。
 * @param[in] str          要绘制的C风格字符串 (以'\0'结尾)。
 * @param[in] color        字符串的颜色，采用RGB565格式。
 * @note 该函数会自动进行边界检查。如果字符串超出右边界或下边界，
 *       起始坐标会被调整以确保字符串尽可能完整地显示在图像内。
 *       字体大小是固定的8x16像素。
 */
void draw_string_rgb565_image(uint16_t *image_addr, uint16_t image_width,
                              uint16_t image_height, uint16_t x, uint16_t y,
                              char *str, uint16_t color) {
  uint16_t *src = image_addr, *origin;
  uint16_t w = image_width, h = image_height;

  uint16_t slen = (uint16_t)strlen(str);
  // 边界检查与坐标调整
  if ((uint16_t)(slen * 8 + x) > w) {
    x = (uint16_t)(w - slen * 8);
    printf("x out of range!");
  }
  if (y > (uint16_t)(h - 16)) {
    y = (uint16_t)(h - 16);
    printf("y out of range!");
  }

  src += (size_t)y * w + x;
  origin = src;

  while (*str) {
    // 遍历字符的16行像素
    for (uint8_t i = 0; i < 16; i++) {
      // 获取当前行的字模数据
      uint8_t data = ascii0816[(uint8_t)(*str) * 16 + i];
      src = origin + (size_t)i * w;
      // 遍历当前行的8列像素
      for (uint8_t j = 0; j < 8; j++) {
        // 如果字模的最高位为1，则绘制像素
        if (data & 0x80)
          src[j] = color;
        data <<= 1;
      }
    }
    str++;
    origin += 8; // 移动到下一个字符的绘制位置
  }
}

/**
 * @brief 在RGB565格式的图像上绘制一个2像素粗细的矩形框
 *
 * @param[in,out] image_addr 指向RGB565图像数据缓冲区的指针。
 * @param[in] image_width  图像的宽度（单位：像素）。
 * @param[in] x1           矩形框左上角的X坐标。
 * @param[in] y1           矩形框左上角的Y坐标。
 * @param[in] x2           矩形框右下角的X坐标。
 * @param[in] y2           矩形框右下角的Y坐标。
 * @param[in] color        矩形框的颜色，采用RGB565格式。
 * @note 此函数包含针对320x240分辨率的硬编码边界检查。
 *       为了提高效率，它通过将两个16位像素合并为一个32位整数来一次性写入两个像素，
 *       这要求操作的起始地址和宽度是偶数像素对齐的。
 */
void draw_box_rgb565_image(uint16_t *image_addr, uint16_t image_width,
                           uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                           uint16_t color) {
  // 将颜色值复制到32位变量的高16位和低16位，用于一次写入两个像素
  uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
  uint32_t *a1, *a2, *a3, *a4;

  // 硬编码的边界检查
  if (x1 < 1)
    x1 = 0;
  if (x2 > 319)
    x2 = 319;
  if (y1 < 1)
    y1 = 0;
  if (y2 > 239)
    y2 = 239;

  // 计算上下边框的起始地址（作为uint32_t指针）
  a1 = (uint32_t *)image_addr + ((size_t)image_width * y1 + x1) / 2;
  a2 = (uint32_t *)image_addr + ((size_t)image_width * (y1 + 1) + x1) / 2;
  a3 = (uint32_t *)image_addr + ((size_t)image_width * y2 + x1) / 2;
  a4 = (uint32_t *)image_addr + ((size_t)image_width * (y2 - 1) + x1) / 2;

  // 绘制上下两条2像素宽的水平线
  for (uint8_t i = 0; i < (uint8_t)((x2 - x1) / 2); i++) {
    *a1++ = data;
    *a2++ = data;
    *a3++ = data;
    *a4++ = data;
  }

  // 计算左右边框的起始地址
  a1 = (uint32_t *)image_addr + ((size_t)image_width * y1 + x1) / 2;
  a2 = (uint32_t *)image_addr + ((size_t)image_width * y1 + x2) / 2 - 1;
  // 绘制左右两条2像素宽的垂直线
  for (uint16_t i = 0; i < (uint16_t)(y2 - y1); i++) {
    *a1 = data;
    *a2 = data;
    a1 += image_width / 2;
    a2 += image_width / 2;
  }
}

/**
 * @brief 在RGB565格式的图像上绘制一个填充矩形
 *
 * @param[in,out] image_addr 指向RGB565图像数据缓冲区的指针。
 * @param[in] image_width  图像的宽度（单位：像素）。
 * @param[in] x1           矩形左上角的X坐标。
 * @param[in] y1           矩形左上角的Y坐标。
 * @param[in] x2           矩形右下角的X坐标。
 * @param[in] y2           矩形右下角的Y坐标。
 * @param[in] color        填充颜色，采用RGB565格式。
 * @note 此函数同样包含硬编码的边界检查，并使用32位写操作进行优化。
 */
void draw_fill_rectangle_image(uint16_t *image_addr, uint16_t image_width,
                               uint16_t x1, uint16_t y1, uint16_t x2,
                               uint16_t y2, uint16_t color) {
  uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
  uint32_t *addr;

  if (x1 < 1)
    x1 = 0;
  if (x2 > 319)
    x2 = 319;
  if (y1 < 1)
    y1 = 0;
  if (y2 > 239)
    y2 = 239;

  addr = (uint32_t *)image_addr + ((size_t)image_width * y1 + x1) / 2;
  for (uint8_t j = 0; j < (uint8_t)(y2 - y1); j++) {
    for (uint8_t i = 0; i < (uint8_t)((x2 - x1) / 2); i++)
      addr[i] = data;
    addr += image_width / 2;
  }
}

/**
 * @brief 在RGB565格式的图像上绘制一个点
 *
 * @param[in,out] image_addr 指向RGB565图像数据缓冲区的指针。
 * @param[in] image_width  图像的宽度（单位：像素）。
 * @param[in] x            点的X坐标。
 * @param[in] y            点的Y坐标。
 * @param[in] color        点的颜色，采用RGB565格式。
 * @note 包含硬编码的320x240边界检查。
 */
void draw_point_rgb565_image(uint16_t *image_addr, uint16_t image_width,
                             uint16_t x, uint16_t y, uint16_t color) {
  if (x > 319)
    x = 319;
  if (y > 239)
    y = 239;
  image_addr[(size_t)y * image_width + x] = color;
}

/**
 * @brief 规范化形态学核心的大小
 * @internal
 * 这是一个内部辅助函数，用于确保形态学操作的kernel_size是有效的（正奇数）。
 * @param[in] k 原始的核心大小。
 * @return int 返回一个大于等于1的奇数。
 */
static inline int clamp_kernel_size(int k) {
  if (k < 1)
    k = 1;
  if ((k & 1) == 0)
    k += 1;
  return k;
}

/**
 * @brief 通用可分离形态学操作
 * @internal
 * 使用可分离的滤波思想高效地执行腐蚀或膨胀操作。
 * 它首先对每一行进行一维操作，然后使用环形缓冲区对每一列进行一维操作。
 * @param[in]  src       源二值图像数据 (前景为非0)。
 * @param[out] dst       目标二值图像数据 (前景为255, 背景为0)。
 * @param[in]  W         图像宽度。
 * @param[in]  H         图像高度。
 * @param[in]  k         核心大小 (必须是奇数)。
 * @param[in]  is_dilate 模式选择：1 表示膨胀，0 表示腐蚀。
 */
static void morph_separable(const uint8_t *src, uint8_t *dst, int W, int H,
                            int k, int is_dilate) {
  k = clamp_kernel_size(k);
  const int hk = k >> 1;

  uint8_t *rowbuf = (uint8_t *)malloc((size_t)W);
  uint8_t *ringbuf = (uint8_t *)malloc((size_t)W * (size_t)k);
  int *colsum = (int *)malloc((size_t)W * sizeof(int));
  if (!rowbuf || !ringbuf || !colsum) {
    if (rowbuf)
      free(rowbuf);
    if (ringbuf)
      free(ringbuf);
    if (colsum)
      free(colsum);
    memset(dst, 0, (size_t)W * (size_t)H);
    return;
  }

  memset(dst, 0, (size_t)W * (size_t)H);
  memset(ringbuf, 0, (size_t)W * (size_t)k);
  memset(colsum, 0, (size_t)W * sizeof(int));

  for (int y = 0; y < H; ++y) {
    const uint8_t *srow = src + (size_t)y * (size_t)W;

    int run = 0;
    for (int x = 0; x < W; ++x) {
      run += (srow[x] != 0);
      if (x >= k)
        run -= (srow[x - k] != 0);
      if (x >= k - 1) {
        uint8_t out = is_dilate ? (uint8_t)(run > 0) : (uint8_t)(run == k);
        int xc = x - hk;
        if (xc >= 0 && xc < W)
          rowbuf[xc] = out;
      }
    }

    uint8_t *slot = ringbuf + (size_t)(y % k) * (size_t)W;
    if (y >= k) {
      const uint8_t *old = ringbuf + (size_t)((y - k) % k) * (size_t)W;
      for (int x = 0; x < W; ++x)
        colsum[x] -= old[x];
    }
    for (int x = 0; x < W; ++x) {
      slot[x] = rowbuf[x];
      colsum[x] += rowbuf[x];
    }

    if (y >= k - 1) {
      int yc = y - hk;
      if (yc >= hk && yc < H - hk) {
        uint8_t *drow = dst + (size_t)yc * (size_t)W;
        for (int x = hk; x < W - hk; ++x)
          drow[x] = (is_dilate ? (colsum[x] > 0) : (colsum[x] == k)) ? 255 : 0;
      }
    }
  }

  free(rowbuf);
  free(ringbuf);
  free(colsum);
}

/**
 * @brief 优化的3x3形态学操作
 * @internal
 * 针对3x3核心的特殊优化版本，使用3行环形缓冲区来减少内存访问。
 * @param[in]  src       源二值图像。
 * @param[out] dst       目标二值图像。
 * @param[in]  W         图像宽度。
 * @param[in]  H         图像高度。
 * @param[in]  is_dilate 1 表示膨胀，0 表示腐蚀。
 */
static void morph_3(const uint8_t *src, uint8_t *dst, int W, int H,
                    int is_dilate) {
  if (!src || !dst || W <= 0 || H <= 0)
    return;

  uint8_t *h0 = (uint8_t *)malloc((size_t)W);
  uint8_t *h1 = (uint8_t *)malloc((size_t)W);
  uint8_t *h2 = (uint8_t *)malloc((size_t)W);
  if (!h0 || !h1 || !h2) {
    if (h0)
      free(h0);
    if (h1)
      free(h1);
    if (h2)
      free(h2);
    memset(dst, 0, (size_t)W * (size_t)H);
    return;
  }
  memset(dst, 0, (size_t)W * (size_t)H);

  uint8_t *ring[3] = {h0, h1, h2};
  int head = 0;

  for (int y = 0; y < H; ++y) {
    const uint8_t *srow = src + (size_t)y * (size_t)W;
    uint8_t *hrow = ring[head];

    hrow[0] = 0;
    hrow[W - 1] = 0;

    if (is_dilate)
      for (int x = 1; x < W - 1; ++x)
        hrow[x] = (uint8_t)((srow[x - 1] | srow[x] | srow[x + 1]) ? 1 : 0);
    else
      for (int x = 1; x < W - 1; ++x)
        hrow[x] = (uint8_t)((srow[x - 1] & srow[x] & srow[x + 1]) ? 1 : 0);

    if (y >= 2) {
      int yc = y - 1;
      uint8_t *drow = dst + (size_t)yc * (size_t)W;
      for (int x = 1; x < W - 1; ++x) {
        uint8_t v = is_dilate
                        ? (uint8_t)((ring[(head + 1) % 3][x] |
                                     ring[(head + 2) % 3][x] | ring[head][x])
                                        ? 255
                                        : 0)
                        : (uint8_t)((ring[(head + 1) % 3][x] &
                                     ring[(head + 2) % 3][x] & ring[head][x])
                                        ? 255
                                        : 0);
        drow[x] = v;
      }
    }
    head = (head + 1) % 3;
  }

  free(h0);
  free(h1);
  free(h2);
}

/** @internal @brief 3x3膨胀的内联包装器 */
static inline void dilate_3(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_3(s, d, W, H, 1);
}
/** @internal @brief 3x3腐蚀的内联包装器 */
static inline void erode_3(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_3(s, d, W, H, 0);
}

/**
 * @brief 优化的5x5形态学操作
 * @internal
 * 针对5x5核心的特殊优化版本，使用5行环形缓冲区。
 * @param[in]  src       源二值图像。
 * @param[out] dst       目标二值图像。
 * @param[in]  W         图像宽度。
 * @param[in]  H         图像高度。
 * @param[in]  is_dilate 1 表示膨胀，0 表示腐蚀。
 */
static void morph_5(const uint8_t *src, uint8_t *dst, int W, int H,
                    int is_dilate) {
  if (!src || !dst || W <= 0 || H <= 0)
    return;

  uint8_t *ring = (uint8_t *)malloc((size_t)W * 5);
  int *vsum = (int *)malloc((size_t)W * sizeof(int));
  if (!ring || !vsum) {
    if (ring)
      free(ring);
    if (vsum)
      free(vsum);
    memset(dst, 0, (size_t)W * (size_t)H);
    return;
  }

  memset(dst, 0, (size_t)W * (size_t)H);
  memset(ring, 0, (size_t)W * 5);
  memset(vsum, 0, (size_t)W * sizeof(int));

  for (int y = 0; y < H; ++y) {
    const uint8_t *srow = src + (size_t)y * (size_t)W;
    uint8_t *hrow = ring + (size_t)(y % 5) * (size_t)W;

    memset(hrow, 0, (size_t)W);
    int run = 0;
    for (int x = 0; x < W; ++x) {
      run += (srow[x] != 0);
      if (x >= 5)
        run -= (srow[x - 5] != 0);
      if (x >= 4) {
        int xc = x - 2;
        if (xc >= 0 && xc < W)
          hrow[xc] = (uint8_t)(is_dilate ? (run > 0) : (run == 5));
      }
    }

    if (W > 0) {
      hrow[0] = 0;
      hrow[W - 1] = 0;
    }
    if (W > 1) {
      hrow[1] = 0;
      hrow[W - 2] = 0;
    }

    if (y >= 5) {
      const uint8_t *old = ring + (size_t)((y - 5) % 5) * (size_t)W;
      for (int x = 0; x < W; ++x)
        vsum[x] -= old[x];
    }
    for (int x = 0; x < W; ++x)
      vsum[x] += hrow[x];

    if (y >= 4) {
      int yc = y - 2;
      if (yc >= 2 && yc < H - 2) {
        uint8_t *drow = dst + (size_t)yc * (size_t)W;
        for (int x = 2; x < W - 2; ++x)
          drow[x] = (is_dilate ? (vsum[x] > 0) : (vsum[x] == 5)) ? 255 : 0;
      }
    }
  }

  free(ring);
  free(vsum);
}

/** @internal @brief 5x5膨胀的内联包装器 */
static inline void dilate_5(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_5(s, d, W, H, 1);
}
/** @internal @brief 5x5腐蚀的内联包装器 */
static inline void erode_5(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_5(s, d, W, H, 0);
}

void image_erode(const uint8_t *src, uint8_t *dst, int width, int height,
                 int kernel_size) {
  if (!src || !dst || width <= 0 || height <= 0)
    return;
  kernel_size = clamp_kernel_size(kernel_size);
  // 根据核心大小选择最优化的实现
  if (kernel_size == 3)
    erode_3(src, dst, width, height);
  else if (kernel_size == 5)
    erode_5(src, dst, width, height);
  else
    morph_separable(src, dst, width, height, kernel_size, 0);
}

void image_dilate(const uint8_t *src, uint8_t *dst, int width, int height,
                  int kernel_size) {
  if (!src || !dst || width <= 0 || height <= 0)
    return;
  kernel_size = clamp_kernel_size(kernel_size);
  // 根据核心大小选择最优化的实现
  if (kernel_size == 3)
    dilate_3(src, dst, width, height);
  else if (kernel_size == 5)
    dilate_5(src, dst, width, height);
  else
    morph_separable(src, dst, width, height, kernel_size, 1);
}

void image_binary_open(uint8_t *binary_img, int width, int height,
                       int kernel_size) {
  uint8_t *tmp = (uint8_t *)malloc((size_t)width * (size_t)height);
  if (!tmp) {
    printf("Failed to allocate temp buffer for morph open\n");
    return;
  }
  memset(tmp, 0, (size_t)width * (size_t)height);
  // 先腐蚀，再膨胀
  image_erode(binary_img, tmp, width, height, kernel_size);
  image_dilate(tmp, binary_img, width, height, kernel_size);
  free(tmp);
}

void image_binary_close(uint8_t *binary_img, int width, int height,
                        int kernel_size) {
  uint8_t *tmp = (uint8_t *)malloc((size_t)width * (size_t)height);
  if (!tmp) {
    printf("Failed to allocate temp buffer for morph close\n");
    return;
  }
  memset(tmp, 0, (size_t)width * (size_t)height);
  // 先膨胀，再腐蚀
  image_dilate(binary_img, tmp, width, height, kernel_size);
  image_erode(tmp, binary_img, width, height, kernel_size);
  free(tmp);
}

/**
 * @brief 查找并查集中的根节点（带路径压缩）
 * @internal
 * @param[in,out] parent 父节点数组。
 * @param[in]     x      要查找的元素。
 * @return int 元素x所在集合的根节点。
 */
static inline int find_root(int *parent, int x) {
  int root = x;
  while (parent[root] != root)
    root = parent[root];
  // 路径压缩
  while (x != root) {
    int p = parent[x];
    parent[x] = root;
    x = p;
  }
  return root;
}

/**
 * @brief 合并两个集合（按秩合并）
 * @internal
 * @param[in,out] parent 父节点数组。
 * @param[in,out] rank   集合的秩（或深度）数组。
 * @param[in]     a      第一个元素。
 * @param[in]     b      第二个元素。
 */
static inline void union_sets(int *parent, uint8_t *rank, int a, int b) {
  int ra = find_root(parent, a), rb = find_root(parent, b);
  if (ra == rb)
    return;
  if (rank[ra] < rank[rb])
    parent[ra] = rb;
  else if (rank[ra] > rank[rb])
    parent[rb] = ra;
  else {
    parent[rb] = ra;
    rank[ra]++;
  }
}

int find_blobs(const uint8_t *binary_img, int width, int height,
               BlobInfo *blobs, int max_blobs) {
  if (!binary_img || !blobs || width <= 0 || height <= 0 || max_blobs <= 0)
    return 0;

  const int W = width, H = height;
  const size_t N = (size_t)W * (size_t)H;

  // 预定义最大像素和标签数量，以使用静态内存
  enum {
    MAX_PIXELS = (size_t)CAMERA_WIDTH * (size_t)CAMERA_HEIGHT,
    MAX_LABELS = (int)(MAX_PIXELS / 2) + 128
  };
  if (N == 0 || N > MAX_PIXELS)
    return 0;

  // 使用静态数组避免在嵌入式系统上栈溢出或频繁的动态内存分配，
  // 但这使得函数不可重入。
  static int labels_static[MAX_PIXELS];
  static int parent_static[MAX_LABELS];
  static uint8_t rank_static[MAX_LABELS];
  static int pix_cnt_static[MAX_LABELS];
  static int min_x_static[MAX_LABELS];
  static int min_y_static[MAX_LABELS];
  static int max_x_static[MAX_LABELS];
  static int max_y_static[MAX_LABELS];

  int *labels = labels_static;
  int *parent = parent_static;
  uint8_t *rank = rank_static;

  memset(labels, 0, N * sizeof(int));

  int next_label = 1;
  parent[0] = 0;
  rank[0] = 0;

  // 第一遍扫描：标记像素并记录标签等价关系
  for (int y = 0; y < H; ++y) {
    const int yW = y * W;
    for (int x = 0; x < W; ++x) {
      const int idx = yW + x;
      if (binary_img[idx] != 255)
        continue;

      // 检查8-邻域内的已标记像素
      int nl[4];
      int ncnt = 0;
      if (y > 0) { // 上
        int t = labels[(y - 1) * W + x];
        if (t)
          nl[ncnt++] = t;
      }
      if (x > 0) { // 左
        int t = labels[yW + (x - 1)];
        if (t)
          nl[ncnt++] = t;
      }
      if (y > 0 && x > 0) { // 左上
        int t = labels[(y - 1) * W + x - 1];
        if (t)
          nl[ncnt++] = t;
      }
      if (y > 0 && x < W - 1) { // 右上
        int t = labels[(y - 1) * W + x + 1];
        if (t)
          nl[ncnt++] = t;
      }

      if (ncnt == 0) {
        // 如果邻域没有标签，分配一个新标签
        if (next_label >= MAX_LABELS)
          return 0; // 标签池耗尽
        labels[idx] = next_label;
        parent[next_label] = next_label;
        rank[next_label] = 0;
        next_label++;
      } else {
        // 如果邻域有标签，选择一个作为当前像素标签，并合并所有邻域标签
        int mr = find_root(parent, nl[0]);
        for (int k = 1; k < ncnt; ++k) {
          int rk = find_root(parent, nl[k]);
          if (rk != mr) {
            union_sets(parent, rank, mr, rk);
            mr = find_root(parent, mr); // 更新根，以防变化
          }
        }
        labels[idx] = mr;
      }
    }
  }

  if (next_label == 1) // 没有找到任何前景像素
    return 0;

  // 第二遍扫描：解析等价关系，并计算每个blob的属性
  const int L = next_label;
  memset(pix_cnt_static, 0, (size_t)L * sizeof(int));
  for (int i = 0; i < L; ++i) {
    min_x_static[i] = 0x7FFFFFFF;
    min_y_static[i] = 0x7FFFFFFF;
    max_x_static[i] = -0x7FFFFFFF;
    max_y_static[i] = -0x7FFFFFFF;
  }

  for (int y = 0; y < H; ++y) {
    const int yW = y * W;
    for (int x = 0; x < W; ++x) {
      const int idx = yW + x;
      int lab = labels[idx];
      if (!lab)
        continue;

      // 将每个像素的标签更新为其所在集合的根标签
      int r = find_root(parent, lab);
      labels[idx] = r;

      // 累加根标签对应的blob属性
      pix_cnt_static[r]++;
      if (x < min_x_static[r])
        min_x_static[r] = x;
      if (y < min_y_static[r])
        min_y_static[r] = y;
      if (x > max_x_static[r])
        max_x_static[r] = x;
      if (y > max_y_static[r])
        max_y_static[r] = y;
    }
  }

  // 整理结果，填充blobs数组
  int blob_count = 0;
  for (int r = 1; r < L && blob_count < max_blobs; ++r) {
    if (pix_cnt_static[r] > 0) { // 这是一个有效的blob
      BlobInfo bi;
      bi.id = blob_count + 1;
      bi.pixel_count = pix_cnt_static[r];
      bi.min_x = (min_x_static[r] == 0x7FFFFFFF) ? 0 : min_x_static[r];
      bi.min_y = (min_y_static[r] == 0x7FFFFFFF) ? 0 : min_y_static[r];
      bi.max_x = (max_x_static[r] == -0x7FFFFFFF) ? 0 : max_x_static[r];
      bi.max_y = (max_y_static[r] == -0x7FFFFFFF) ? 0 : max_y_static[r];
      blobs[blob_count++] = bi;
    }
  }
  return blob_count;
}
