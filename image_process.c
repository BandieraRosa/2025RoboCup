#include "image_process.h"
#include "./BSP/LCD/lcdfont.h"
#include "iomem.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int image_buffer_ready(const image_t *image) {
  return (image != NULL) && (image->addr != NULL) && (image->width > 0U) &&
         (image->height > 0U) && (image->pixel > 0U);
}

static inline size_t image_plane_size(const image_t *image) {
  return (size_t)image->width * (size_t)image->height;
}

int image_init(image_t *image) {
  if (!image_buffer_ready(image))
    return -1;

  size_t plane = image_plane_size(image);
  if (plane == 0U || plane > (SIZE_MAX / image->pixel))
    return -1;

  image->addr = iomem_malloc(plane * image->pixel);
  if (image->addr == NULL)
    return -1;

  return 0;
}

void image_deinit(image_t *image) {
  if (image != NULL && image->addr != NULL) {
    iomem_free(image->addr);
    image->addr = NULL;
  }
}

void image_crop(image_t *image_src, image_t *image_dst, uint16_t x_offset,
                uint16_t y_offset) {
  if (!image_buffer_ready(image_src) || !image_buffer_ready(image_dst))
    return;

  size_t dst_plane = image_plane_size(image_dst);
  if (dst_plane == 0U || dst_plane > (SIZE_MAX / image_dst->pixel))
    return;

  memset(image_dst->addr, 0, dst_plane * image_dst->pixel);

  uint16_t w_src = image_src->width;
  uint16_t h_src = image_src->height;
  uint16_t w_dst = image_dst->width;
  uint16_t h_dst = image_dst->height;

  if (x_offset >= w_src || y_offset >= h_src)
    return;

  size_t copy_w =
      (size_t)((w_src - x_offset) < w_dst ? (w_src - x_offset) : w_dst);
  size_t copy_h =
      (size_t)((h_src - y_offset) < h_dst ? (h_src - y_offset) : h_dst);

  size_t src_line_step = (size_t)w_src;
  size_t dst_line_step = (size_t)w_dst;

  uint8_t *r_src = image_src->addr + y_offset * src_line_step + x_offset;
  uint8_t *g_src = r_src + src_line_step * h_src;
  uint8_t *b_src = g_src + src_line_step * h_src;

  uint8_t *r_dst = image_dst->addr;
  uint8_t *g_dst = r_dst + dst_line_step * h_dst;
  uint8_t *b_dst = g_dst + dst_line_step * h_dst;

  for (size_t y = 0U; y < copy_h; ++y) {
    memcpy(r_dst + y * dst_line_step, r_src + y * src_line_step, copy_w);
    memcpy(g_dst + y * dst_line_step, g_src + y * src_line_step, copy_w);
    memcpy(b_dst + y * dst_line_step, b_src + y * src_line_step, copy_w);
  }
}

void image_draw(image_t *image_src, image_t *image_dst, uint16_t x_start,
                uint16_t y_start) {
  if (!image_buffer_ready(image_src) || !image_buffer_ready(image_dst))
    return;

  uint16_t w_src = image_src->width;
  uint16_t h_src = image_src->height;
  uint16_t w_dst = image_dst->width;
  uint16_t h_dst = image_dst->height;

  if (x_start >= w_dst || y_start >= h_dst)
    return;

  size_t copy_w =
      (size_t)(w_src < (w_dst - x_start) ? w_src : (w_dst - x_start));
  size_t copy_h =
      (size_t)(h_src < (h_dst - y_start) ? h_src : (h_dst - y_start));

  size_t src_line_step = (size_t)w_src;
  size_t dst_line_step = (size_t)w_dst;

  size_t dst_offset = (size_t)y_start * dst_line_step + x_start;

  uint8_t *r_src = image_src->addr;
  uint8_t *g_src = r_src + src_line_step * h_src;
  uint8_t *b_src = g_src + src_line_step * h_src;

  uint8_t *r_dst = image_dst->addr + dst_offset;
  uint8_t *g_dst = image_dst->addr + dst_line_step * h_dst + dst_offset;
  uint8_t *b_dst = image_dst->addr + 2U * dst_line_step * h_dst + dst_offset;

  for (size_t y = 0U; y < copy_h; ++y) {
    memcpy(r_dst + y * dst_line_step, r_src + y * src_line_step, copy_w);
    memcpy(g_dst + y * dst_line_step, g_src + y * src_line_step, copy_w);
    memcpy(b_dst + y * dst_line_step, b_src + y * src_line_step, copy_w);
  }
}

/**
 * @brief       图片缩放
 * @param       image_src:源图片的结构体指针
 * @param       image_dst:目标图片的结构体指针
 * @retval      无
 */
void image_resize(image_t *image_src,
                  image_t *image_dst) /* 缩放，缩放大小与两个结构体有关*/
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

  for (uint16_t y = 0; y < h_dst; y++) {
    for (uint16_t x = 0; x < w_dst; x++) {
      x_src = (x + 0.5f) * w_scale - 0.5f;
      x1 = (uint16_t)x_src;
      x2 = x1 + 1;
      y_src = (y + 0.5f) * h_scale - 0.5f;
      y1 = (uint16_t)y_src;
      y2 = y1 + 1;

      if (x2 >= w_src || y2 >= h_src) {
        *(r_dst + x + y * w_dst) = *(r_src + x1 + y1 * w_src);
        *(g_dst + x + y * w_dst) = *(g_src + x1 + y1 * w_src);
        *(b_dst + x + y * w_dst) = *(b_src + x1 + y1 * w_src);
        continue;
      }

      temp1 = (x2 - x_src) * *(r_src + x1 + y1 * w_src) +
              (x_src - x1) * *(r_src + x2 + y1 * w_src);
      temp2 = (x2 - x_src) * *(r_src + x1 + y2 * w_src) +
              (x_src - x1) * *(r_src + x2 + y2 * w_src);
      *(r_dst + x + y * w_dst) =
          (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
      temp1 = (x2 - x_src) * *(g_src + x1 + y1 * w_src) +
              (x_src - x1) * *(g_src + x2 + y1 * w_src);
      temp2 = (x2 - x_src) * *(g_src + x1 + y2 * w_src) +
              (x_src - x1) * *(g_src + x2 + y2 * w_src);
      *(g_dst + x + y * w_dst) =
          (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
      temp1 = (x2 - x_src) * *(b_src + x1 + y1 * w_src) +
              (x_src - x1) * *(b_src + x2 + y1 * w_src);
      temp2 = (x2 - x_src) * *(b_src + x1 + y2 * w_src) +
              (x_src - x1) * *(b_src + x2 + y2 * w_src);
      *(b_dst + x + y * w_dst) =
          (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
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
void image_replace(uint8_t *image_addr, uint16_t image_width,
                   uint16_t image_height, uint8_t vflip, uint8_t hmirror) {
  uint8_t *src, *r_src, *g_src, *b_src;
  uint8_t temp;
  uint16_t t1, t2;
  uint32_t offset1, offset2;
  src = image_addr;

  if (vflip == 1) {
    t1 = image_height >> 1;

    r_src = src;
    g_src = r_src + image_width * image_height;
    b_src = g_src + image_width * image_height;

    for (uint16_t j = 0; j < t1; j++) {
      for (uint16_t i = 0; i < image_width; i++) {
        offset1 = (image_width * j) + i;
        offset2 = image_width * (image_height - 1 - j) + i;

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

  if (hmirror == 1) {
    t2 = image_width >> 1;
    for (uint16_t j = 0; j < image_height * 3; j++) {
      for (uint16_t i = 0; i < t2; i++) {
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
void image_rgb888_to_gray(uint8_t *image_addr, uint16_t image_width,
                          uint16_t image_height) {
  uint8_t *src, *r_src, *g_src, *b_src;
  int gray_temp;

  src = image_addr;
  r_src = src;
  g_src = r_src + image_width * image_height;
  b_src = g_src + image_width * image_height;

  for (uint32_t j = 0; j < image_width * image_height; j++) {
    gray_temp = (306 * (int)*r_src + 601 * (int)*g_src + 117 * (int)*b_src) >>
                10; /* 转换公式 */

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
void image_invert(uint8_t *image_addr, uint16_t image_width,
                  uint16_t image_height) {
  uint8_t *r_src, *g_src, *b_src;
  r_src = image_addr;
  g_src = r_src + image_width * image_height;
  b_src = g_src + image_width * image_height;

  for (uint32_t j = 0; j < image_width * image_height; j++) {
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
void image_strech_chart(uint8_t *image_addr, uint16_t image_width,
                        uint16_t image_height, uint8_t de_dark) {
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
  for (index = 0; index < image_width * image_height; index++) {
    if (in[index] > graymax)
      graymax = in[index];
    sx += in[index];
    sx2 += ((int)in[index] * (int)in[index]);
  }

  ex = sx / image_width / image_height;
  gate = ex;

  for (index = 0; index < image_width * image_height; index++) {
    x = index % image_width;
    y = index / image_width;
    dat = in[index];
    int denom = (int)graymax - (int)gate;
    if (denom == 0)
      denom = 1;
    dat = (dat - gate) * 255 / denom;
    dat = dat < 0 ? 0 : (dat > 255 ? 255 : dat);
    r2 = (x - image_width / 2) * (x - image_width / 2) +
         (y - image_height / 2) * (y - image_height / 2);

    if (de_dark) {
      dat = (int)(dat / (1.0 + 32.0 * r2 * r2 / image_width / image_width /
                                   image_height / image_height));
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
void rgb888_to_rgb565(uint8_t *src_addr, uint16_t *dest_addr,
                      uint16_t image_width, uint16_t image_height) {
  size_t chn_size = image_width * image_height;
  for (size_t i = 0; i < image_width * image_height; i++) {
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
void draw_string_rgb565_image(uint16_t *image_addr, uint16_t image_width,
                              uint16_t image_height, uint16_t x, uint16_t y,
                              char *str, uint16_t color) {
  uint16_t *src, *origin;
  uint16_t w_src, h_src;

  uint16_t slen = strlen(str);
  uint8_t i = 0;
  uint8_t j = 0;
  uint8_t data = 0;

  src = image_addr;
  w_src = image_width;
  h_src = image_height;

  if ((slen * 8 + x) > w_src) {
    x = w_src - slen * 8;
    printf("x out of range!");
  }
  if (y > (h_src - 16)) {
    y = h_src - 16;
    printf("y out of range!");
  }

  src += y * w_src + x;
  origin = src;

  while (*str) {
    for (i = 0; i < 16; i++) {
      data = ascii0816[*str * 16 + i];
      src = origin + i * w_src; /*写完8个点后偏移到下一行*/
      for (j = 0; j < 8; j++) {
        if (data & 0x80) {
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
void draw_box_rgb565_image(uint16_t *image_addr, uint16_t image_width,
                           uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                           uint16_t color) {
  uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
  uint32_t *addr1, *addr2, *addr3, *addr4;

  if (x1 < 1)
    x1 = 0;
  if (x2 > 319)
    x2 = 319;
  if (y1 < 1)
    y1 = 0;
  if (y2 > 239)
    y2 = 239;

  addr1 = (uint32_t *)image_addr + (image_width * y1 + x1) / 2;
  addr2 = (uint32_t *)image_addr + (image_width * (y1 + 1) + x1) / 2;
  addr3 = (uint32_t *)image_addr + (image_width * y2 + x1) / 2;
  addr4 = (uint32_t *)image_addr + (image_width * (y2 - 1) + x1) / 2;

  for (uint8_t i = 0; i < (x2 - x1) / 2; i++) {
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
  for (uint16_t i = 0; i < y2 - y1; i++) {
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

  addr = (uint32_t *)image_addr + (image_width * y1 + x1) / 2;

  for (uint8_t j = 0; j < y2 - y1; j++) {

    for (uint8_t i = 0; i < (x2 - x1) / 2; i++) {
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
void draw_point_rgb565_image(uint16_t *image_addr, uint16_t image_width,
                             uint16_t x, uint16_t y, uint16_t color) {
  if (x > 319)
    x = 319;
  if (y > 239)
    y = 239;

  *(image_addr + y * image_width + x) = color;
}

// --- 小球识别部分相关函数与宏 ---

/**
 * @brief       将8位灰度图像转换为RGB565格式的图像
 * @param       gray: 输入，指向源8位灰度图像数据的指针
 * @param       rgb565: 输出，指向目标16位RGB565图像缓冲区的指针
 * @param       width: 图像的宽度（像素）
 * @param       height: 图像的高度（像素）
 */
void gray_to_rgb565(const uint8_t *gray, uint8_t *rgb565, int width,
                    int height) {
  if (gray == NULL || rgb565 == NULL) {
    return;
  }

  uint32_t num_pixels = (uint32_t)width * height;
  const uint8_t *p_src = gray;
  uint16_t *p_dst = (uint16_t *)rgb565;

  for (uint32_t i = 0; i < num_pixels; i++) {
    uint8_t gray_value = p_src[i];
    uint16_t r5 = gray_value >> 3;
    uint16_t g6 = gray_value >> 2;
    uint16_t b5 = gray_value >> 3;
    p_dst[i] = (r5 << 11) | (g6 << 5) | b5;
  }
}

/**
 * @brief       对二值图像进行腐蚀操作
 * @param       src:         源二值图像数据
 * @param       dst:         目标二值图像缓冲区
 * @param       width:       图像宽度
 * @param       height:      图像高度
 * @param       kernel_size: 结构元素大小（奇数，如3, 5）
 */
void image_binary_erode(const uint8_t *src, uint8_t *dst, int width, int height,
                        int kernel_size) {
  int half_k = kernel_size / 2;
  for (int y = half_k; y < height - half_k; y++) {
    for (int x = half_k; x < width - half_k; x++) {
      int is_eroded = 1;
      for (int ky = -half_k; ky <= half_k; ky++) {
        for (int kx = -half_k; kx <= half_k; kx++) {
          if (src[(y + ky) * width + (x + kx)] == 0) {
            is_eroded = 0;
            break;
          }
        }
        if (!is_eroded)
          break;
      }
      dst[y * width + x] = is_eroded ? 255 : 0;
    }
  }
}

/**
 * @brief       对二值图像进行膨胀操作
 * @param       src:         源二值图像数据
 * @param       dst:         目标二值图像缓冲区
 * @param       width:       图像宽度
 * @param       height:      图像高度
 * @param       kernel_size: 结构元素大小（奇数，如3, 5）
 */
void image_binary_dilate(const uint8_t *src, uint8_t *dst, int width,
                         int height, int kernel_size) {
  int half_k = kernel_size / 2;
  for (int y = half_k; y < height - half_k; y++) {
    for (int x = half_k; x < width - half_k; x++) {
      int is_dilated = 0;
      for (int ky = -half_k; ky <= half_k; ky++) {
        for (int kx = -half_k; kx <= half_k; kx++) {
          if (src[(y + ky) * width + (x + kx)] == 255) {
            is_dilated = 1;
            break;
          }
        }
        if (is_dilated)
          break;
      }
      dst[y * width + x] = is_dilated ? 255 : 0;
    }
  }
}

/**
 * @brief       对二值图像进行开运算（先腐蚀后膨胀），用于去噪
 * @param       binary_img:  要处理的二值图像数据（会被就地修改）
 * @param       width:       图像宽度
 * @param       height:      图像高度
 * @param       kernel_size: 结构元素大小（建议为3）
 */
void image_binary_open(uint8_t *binary_img, int width, int height,
                       int kernel_size) {
  // K210内存有限，为了节约内存，我们需要一个临时缓冲区
  uint8_t *temp_buf = (uint8_t *)malloc(width * height);
  if (temp_buf == NULL) {
    printf("Failed to allocate temp buffer for morph open\n");
    return;
  }
  memset(temp_buf, 0, (size_t)width * height);
  // 1. 腐蚀: binary_img -> temp_buf
  image_binary_erode(binary_img, temp_buf, width, height, kernel_size);

  // 2. 膨胀: temp_buf -> binary_img
  image_binary_dilate(temp_buf, binary_img, width, height, kernel_size);

  free(temp_buf);
}

/**
 * @brief       对二值图像进行闭运算（先膨胀后腐蚀），用于填充小孔
 * @param       binary_img:  要处理的二值图像数据（会被就地修改）
 * @param       width:       图像宽度
 * @param       height:      图像高度
 * @param       kernel_size: 结构元素大小（建议为3）
 */
void image_binary_close(uint8_t *binary_img, int width, int height,
                        int kernel_size) {
  // K210内存有限，为了节约内存，我们需要一个临时缓冲区
  uint8_t *temp_buf = (uint8_t *)malloc(width * height);
  if (temp_buf == NULL) {
    printf("Failed to allocate temp buffer for morph close\n");
    return;
  }
  memset(temp_buf, 0, (size_t)width * height);
  // 1. 膨胀: binary_img -> temp_buf
  image_binary_dilate(binary_img, temp_buf, width, height, kernel_size);

  // 2. 腐蚀: temp_buf -> binary_img
  image_binary_erode(temp_buf, binary_img, width, height, kernel_size);

  free(temp_buf);
}

// --- 并查集辅助函数 ---
// 查找根节点，并进行路径压缩
static int find_root(int *parent, int i) {
  if (parent[i] == i)
    return i;
  parent[i] = find_root(parent, parent[i]); // 路径压缩
  return parent[i];
}
// 合并两个集合
static void union_sets(int *parent, int i, int j) {
  int root_i = find_root(parent, i);
  int root_j = find_root(parent, j);
  if (root_i != root_j) {
    // 将较小的根作为父节点
    if (root_i < root_j)
      parent[root_j] = root_i;
    else
      parent[root_i] = root_j;
  }
}
// --- 并查集结束 ---

/**
 * @brief       在二值图像中查找连通域（Blobs）
 * @param       binary_img:  输入的二值图像 (255为前景, 0为背景)
 * @param       width:       图像宽度
 * @param       height:      图像高度
 * @param       blobs:       用于存储找到的Blob信息的数组
 * @param       max_blobs:   blobs数组的最大容量
 * @retval      找到的Blob数量
 */
int find_blobs(const uint8_t *binary_img, int width, int height,
               BlobInfo *blobs, int max_blobs) {
  int *labels = (int *)malloc(width * height * sizeof(int));
  if (labels == NULL)
    return 0;
  memset(labels, 0, width * height * sizeof(int));

  // 假设标签数量不会超过图像像素的1/4
  int max_labels = width * height / 4;
  int *parent = (int *)malloc(max_labels * sizeof(int));
  if (parent == NULL) {
    free(labels);
    return 0;
  }

  int next_label = 1;

  // --- 第一遍扫描 ---
  for (int y = 1; y < height; y++) {
    for (int x = 1; x < width; x++) {
      if (binary_img[y * width + x] == 255) {
        int up = labels[(y - 1) * width + x];
        int left = labels[y * width + (x - 1)];

        if (up == 0 && left == 0) { // 新区域
          labels[y * width + x] = next_label;
          parent[next_label] = next_label;
          next_label++;
          if (next_label >= max_labels) { // 标签池耗尽
            goto end_pass1;
          }
        } else if (up != 0 && left == 0) { // 继承上方
          labels[y * width + x] = up;
        } else if (up == 0 && left != 0) { // 继承左方
          labels[y * width + x] = left;
        } else { // 上方和左方都有
          labels[y * width + x] = (up < left) ? up : left;
          if (up != left) {
            union_sets(parent, up, left);
          }
        }
      }
    }
  }
end_pass1:;

  // --- 第二遍扫描 + 信息统计 ---
  // 初始化Blob信息
  for (int i = 0; i < max_blobs; i++) {
    blobs[i] = (BlobInfo){.id = i,
                          .pixel_count = 0,
                          .min_x = width,
                          .min_y = height,
                          .max_x = -1,
                          .max_y = -1};
  }

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (labels[y * width + x] != 0) {
        int root = find_root(parent, labels[y * width + x]);
        labels[y * width + x] = root; // 更新为根标签

        if (root < max_blobs) {
          blobs[root].pixel_count++;
          if (x < blobs[root].min_x)
            blobs[root].min_x = x;
          if (y < blobs[root].min_y)
            blobs[root].min_y = y;
          if (x > blobs[root].max_x)
            blobs[root].max_x = x;
          if (y > blobs[root].max_y)
            blobs[root].max_y = y;
        }
      }
    }
  }

  // --- 清理和整理结果 ---
  int blob_count = 0;
  for (int i = 1; i < next_label; i++) {
    // 只保留有实际像素的、根标签对应的Blob
    if (parent[i] == i && blobs[i].pixel_count > 0) {
      blobs[blob_count] = blobs[i];
      blobs[blob_count].id = blob_count + 1; // 重新编号
      blob_count++;
    }
  }

  free(labels);
  free(parent);

  return blob_count;
}
