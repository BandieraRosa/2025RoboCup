#include "image_process.h"
#include "./BSP/LCD/lcdfont.h"
#include "iomem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- 内部工具 ---------------- */

static inline int image_buffer_ready(const image_t *image) {
  return (image != NULL) && (image->addr != NULL) && (image->width > 0U) &&
         (image->height > 0U) && (image->pixel > 0U);
}

static inline size_t image_plane_size(const image_t *image) {
  return (size_t)image->width * (size_t)image->height;
}

/* ---------------- 基础分配/释放 ---------------- */

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

/* ---------------- RGB888 图像操作 ---------------- */

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

void image_resize(image_t *image_src, image_t *image_dst) {
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

/* ---------------- 颜色空间转换：像素级 ---------------- */

void rgb888_to_rgb565_pixel(uint8_t r, uint8_t g, uint8_t b, uint16_t *rgb565) {
  *rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void rgb565_to_rgb888_pixel(uint16_t rgb565, uint8_t *r, uint8_t *g,
                            uint8_t *b) {
  uint8_t r5 = (rgb565 >> 11) & 0x1F;
  uint8_t g6 = (rgb565 >> 5) & 0x3F;
  uint8_t b5 = rgb565 & 0x1F;
  *r = (r5 << 3) | (r5 >> 2);
  *g = (g6 << 2) | (g6 >> 4);
  *b = (b5 << 3) | (b5 >> 2);
}

void rgb888_to_gray_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t *gray) {
  /* Y ≈ 0.299R + 0.587G + 0.114B，使用整数近似： (306*r + 601*g + 117*b) >> 10
   */
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

void gray_to_rgb565_pixel(uint8_t gray, uint16_t *rgb565) {
  uint8_t r, g, b;
  gray_to_rgb888_pixel(gray, &r, &g, &b);
  rgb888_to_rgb565_pixel(r, g, b, rgb565);
}

void rgb888_to_hsv_pixel(uint8_t r, uint8_t g, uint8_t b, uint16_t *h,
                         uint8_t *s, uint8_t *v) {
  uint8_t min_val = r < g ? r : g;
  min_val = min_val < b ? min_val : b;
  uint8_t max_val = r > g ? r : g;
  max_val = max_val > b ? max_val : b;

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
    *r = *g = *b = v;
    return;
  }

  h %= 360;
  uint8_t region = h / 60;
  uint16_t remainder = (h % 60) * 255 / 60;

  uint16_t p = (v * (255 - s)) / 255;
  uint16_t q = (v * (255 * 255 - s * remainder)) / (255 * 255);
  uint16_t t = (v * (255 * 255 - s * (255 - remainder))) / (255 * 255);

  switch (region) {
  case 0:
    *r = v;
    *g = t;
    *b = p;
    break;
  case 1:
    *r = q;
    *g = v;
    *b = p;
    break;
  case 2:
    *r = p;
    *g = v;
    *b = t;
    break;
  case 3:
    *r = p;
    *g = q;
    *b = v;
    break;
  case 4:
    *r = t;
    *g = p;
    *b = v;
    break;
  default:
    *r = v;
    *g = p;
    *b = q;
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

/* ---------------- 颜色空间转换：图像级 ---------------- */

void rgb888_to_gray(const void *src_addr, void *dst_addr, int width,
                    int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint8_t *p_src_r = (const uint8_t *)src_addr;
  size_t plane_size = (size_t)width * (size_t)height;
  const uint8_t *p_src_g = p_src_r + plane_size;
  const uint8_t *p_src_b = p_src_g + plane_size;

  uint8_t *p_dst = (uint8_t *)dst_addr;

  for (size_t i = 0; i < plane_size; i++) {
    rgb888_to_gray_pixel(p_src_r[i], p_src_g[i], p_src_b[i], &p_dst[i]);
  }
}

void gray_to_rgb888(const void *src_addr, void *dst_addr, int width,
                    int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint8_t *p_src = (const uint8_t *)src_addr;
  size_t plane_size = (size_t)width * (size_t)height;

  uint8_t *p_dst_r = (uint8_t *)dst_addr;
  uint8_t *p_dst_g = p_dst_r + plane_size;
  uint8_t *p_dst_b = p_dst_g + plane_size;

  for (size_t i = 0; i < plane_size; i++) {
    p_dst_r[i] = p_src[i];
    p_dst_g[i] = p_src[i];
    p_dst_b[i] = p_src[i];
  }
}

void rgb888_to_rgb565(const void *src_addr, void *dst_addr, int width,
                      int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint8_t *p_src_r = (const uint8_t *)src_addr;
  size_t plane_size = (size_t)width * (size_t)height;
  const uint8_t *p_src_g = p_src_r + plane_size;
  const uint8_t *p_src_b = p_src_g + plane_size;

  uint16_t *p_dst = (uint16_t *)dst_addr;

  for (size_t i = 0; i < plane_size; i++) {
    rgb888_to_rgb565_pixel(p_src_r[i], p_src_g[i], p_src_b[i], &p_dst[i]);
  }
}

void rgb565_to_rgb888(const void *src_addr, void *dst_addr, int width,
                      int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint16_t *p_src = (const uint16_t *)src_addr;
  size_t plane_size = (size_t)width * (size_t)height;

  uint8_t *p_dst_r = (uint8_t *)dst_addr;
  uint8_t *p_dst_g = p_dst_r + plane_size;
  uint8_t *p_dst_b = p_dst_g + plane_size;

  for (size_t i = 0; i < plane_size; i++) {
    rgb565_to_rgb888_pixel(p_src[i], &p_dst_r[i], &p_dst_g[i], &p_dst_b[i]);
  }
}

void gray_to_rgb565(const void *src_addr, void *dst_addr, int width,
                    int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint8_t *p_src = (const uint8_t *)src_addr;
  uint16_t *p_dst = (uint16_t *)dst_addr;
  size_t num_pixels = (size_t)width * (size_t)height;

  for (size_t i = 0; i < num_pixels; i++) {
    gray_to_rgb565_pixel(p_src[i], &p_dst[i]);
  }
}

void rgb565_to_gray(const void *src_addr, void *dst_addr, int width,
                    int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint16_t *p_src = (const uint16_t *)src_addr;
  uint8_t *p_dst = (uint8_t *)dst_addr;
  size_t num_pixels = (size_t)width * (size_t)height;

  for (size_t i = 0; i < num_pixels; i++) {
    rgb565_to_gray_pixel(p_src[i], &p_dst[i]);
  }
}

void rgb888_to_hsv(const void *src_addr, void *dst_addr, int width,
                   int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint8_t *p_src_r = (const uint8_t *)src_addr;
  size_t plane_size = (size_t)width * (size_t)height;
  const uint8_t *p_src_g = p_src_r + plane_size;
  const uint8_t *p_src_b = p_src_g + plane_size;

  uint16_t *p_dst_h = (uint16_t *)dst_addr;
  uint8_t *p_dst_s = (uint8_t *)(p_dst_h + plane_size);
  uint8_t *p_dst_v = p_dst_s + plane_size;

  for (size_t i = 0; i < plane_size; i++) {
    rgb888_to_hsv_pixel(p_src_r[i], p_src_g[i], p_src_b[i], &p_dst_h[i],
                        &p_dst_s[i], &p_dst_v[i]);
  }
}

void hsv_to_rgb888(const void *src_addr, void *dst_addr, int width,
                   int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint16_t *p_src_h = (const uint16_t *)src_addr;
  size_t plane_size = (size_t)width * (size_t)height;
  const uint8_t *p_src_s = (const uint8_t *)(p_src_h + plane_size);
  const uint8_t *p_src_v = p_src_s + plane_size;

  uint8_t *p_dst_r = (uint8_t *)dst_addr;
  uint8_t *p_dst_g = p_dst_r + plane_size;
  uint8_t *p_dst_b = p_dst_g + plane_size;

  for (size_t i = 0; i < plane_size; i++) {
    hsv_to_rgb888_pixel(p_src_h[i], p_src_s[i], p_src_v[i], &p_dst_r[i],
                        &p_dst_g[i], &p_dst_b[i]);
  }
}

void rgb565_to_hsv(const void *src_addr, void *dst_addr, int width,
                   int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint16_t *p_src = (const uint16_t *)src_addr;
  size_t plane_size = (size_t)width * (size_t)height;

  uint16_t *p_dst_h = (uint16_t *)dst_addr;
  uint8_t *p_dst_s = (uint8_t *)(p_dst_h + plane_size);
  uint8_t *p_dst_v = p_dst_s + plane_size;

  for (size_t i = 0; i < plane_size; i++) {
    rgb565_to_hsv_pixel(p_src[i], &p_dst_h[i], &p_dst_s[i], &p_dst_v[i]);
  }
}

void hsv_to_rgb565(const void *src_addr, void *dst_addr, int width,
                   int height) {
  if (src_addr == NULL || dst_addr == NULL || width <= 0 || height <= 0)
    return;

  const uint16_t *p_src_h = (const uint16_t *)src_addr;
  size_t plane_size = (size_t)width * (size_t)height;
  const uint8_t *p_src_s = (const uint8_t *)(p_src_h + plane_size);
  const uint8_t *p_src_v = p_src_s + plane_size;

  uint16_t *p_dst = (uint16_t *)dst_addr;

  for (size_t i = 0; i < plane_size; i++) {
    hsv_to_rgb565_pixel(p_src_h[i], p_src_s[i], p_src_v[i], &p_dst[i]);
  }
}

/* ---------------- RGB565 绘图操作 ---------------- */

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
      src = origin + i * w_src;
      for (j = 0; j < 8; j++) {
        if (data & 0x80) {
          src[j] = color;
        }
        data <<= 1;
      }
    }
    str++;
    origin += 8;
  }
}

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
    *addr1++ = data;
    *addr2++ = data;
    *addr3++ = data;
    *addr4++ = data;
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

void draw_point_rgb565_image(uint16_t *image_addr, uint16_t image_width,
                             uint16_t x, uint16_t y, uint16_t color) {
  if (x > 319)
    x = 319;
  if (y > 239)
    y = 239;
  *(image_addr + y * image_width + x) = color;
}

/* ---------------- 形态学、连通域等函数 ---------------- */

static inline int clamp_kernel_size(int k) {
  if (k < 1)
    k = 1;
  if ((k & 1) == 0)
    k += 1; // force odd
  return k;
}

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

    /* 横向滚动计数（把 255 当作 1） */
    int run = 0;
    for (int x = 0; x < W; ++x) {
      run += (srow[x] != 0);
      if (x >= k)
        run -= (srow[x - k] != 0);
      uint8_t out = 0;
      if (x >= k - 1) {
        if (is_dilate)
          out = (run > 0);
        else
          out = (run == k);
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
        for (int x = hk; x < W - hk; ++x) {
          drow[x] = (is_dilate ? (colsum[x] > 0) : (colsum[x] == k)) ? 255 : 0;
        }
      }
    }
  }

  free(rowbuf);
  free(ringbuf);
  free(colsum);
}

/* ------------------------ 3×3：位操作/极简缓冲 ------------------------ */
/* 思路：先对每行做 3 点 OR/AND，得到 h(x)；保留最近 3 行 h 到环缓，纵向再
 * OR/AND 三行。*/
static void morph_3(const uint8_t *src, uint8_t *dst, int W, int H,
                    int is_dilate) {
  if (!src || !dst || W <= 0 || H <= 0)
    return;
  const int hk = 1; // 3x3
  uint8_t *hbuf0 = (uint8_t *)malloc((size_t)W);
  uint8_t *hbuf1 = (uint8_t *)malloc((size_t)W);
  uint8_t *hbuf2 = (uint8_t *)malloc((size_t)W);
  if (!hbuf0 || !hbuf1 || !hbuf2) {
    if (hbuf0)
      free(hbuf0);
    if (hbuf1)
      free(hbuf1);
    if (hbuf2)
      free(hbuf2);
    memset(dst, 0, (size_t)W * (size_t)H);
    return;
  }
  memset(dst, 0, (size_t)W * (size_t)H);

  uint8_t *ring[3] = {hbuf0, hbuf1, hbuf2};
  int rhead = 0;

  for (int y = 0; y < H; ++y) {
    const uint8_t *srow = src + (size_t)y * (size_t)W;
    uint8_t *hrow = ring[rhead];

    hrow[0] = 0;
    hrow[W - 1] = 0;
    if (is_dilate) {
      for (int x = 1; x < W - 1; ++x)
        hrow[x] = (srow[x - 1] | srow[x] | srow[x + 1]) ? 1 : 0;
    } else {
      for (int x = 1; x < W - 1; ++x)
        hrow[x] = (srow[x - 1] & srow[x] & srow[x + 1]) ? 1 : 0;
    }

    if (y >= 2) {
      int yc = y - hk;
      uint8_t *drow = dst + (size_t)yc * (size_t)W;
      /* 仅对内区写值（边界清零） */
      for (int x = 1; x < W - 1; ++x) {
        uint8_t v;
        if (is_dilate)
          v = (ring[(rhead + 1) % 3][x] | ring[(rhead + 2) % 3][x] |
               ring[rhead][x])
                  ? 255
                  : 0;
        else
          v = (ring[(rhead + 1) % 3][x] & ring[(rhead + 2) % 3][x] &
               ring[rhead][x])
                  ? 255
                  : 0;
        drow[x] = v;
      }
    }
    rhead = (rhead + 1) % 3;
  }

  free(hbuf0);
  free(hbuf1);
  free(hbuf2);
}

static inline void dilate_3(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_3(s, d, W, H, 1);
}
static inline void erode_3(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_3(s, d, W, H, 0);
}

/* ------------------------ 5×5：滚动计数/环缓（O(W·H)）
 * ------------------------ */
/* 横向：窗口宽 5 的布尔计数（把 255 当作 1），得到 hrow(x)（0/1）；
 * 纵向：对 hrow 做列计数（窗口高 5），得到最终 0/255。
 */
static void morph_5(const uint8_t *src, uint8_t *dst, int W, int H,
                    int is_dilate) {
  if (!src || !dst || W <= 0 || H <= 0)
    return;
  const int hk = 2; // 5x5

  uint8_t *ring = (uint8_t *)malloc((size_t)W * 5); // 保存 5 行横向结果（0/1）
  int *vsum = (int *)malloc((size_t)W * sizeof(int)); // 列窗口 5 的计数
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

    /* 横向滚动计数（宽 5）；边界列清零 */
    memset(hrow, 0, (size_t)W);
    int run = 0;
    for (int x = 0; x < W; ++x) {
      run += (srow[x] != 0);
      if (x >= 5)
        run -= (srow[x - 5] != 0);
      if (x >= 4) {
        int xc = x - hk;
        if (xc >= 0 && xc < W) {
          if (is_dilate)
            hrow[xc] = (run > 0); // 任一为 1
          else
            hrow[xc] = (run == 5); // 全为 1
        }
      }
    }
    /* 清边界 */
    if (W > 0) {
      hrow[0] = 0;
      hrow[W - 1] = 0;
    }
    if (W > 1) {
      hrow[1] = 0;
      hrow[W - 2] = 0;
    }

    /* 纵向列计数窗口 5：先移除挤出行，再加入当前行 */
    if (y >= 5) {
      const uint8_t *old = ring + (size_t)((y - 5) % 5) * (size_t)W;
      for (int x = 0; x < W; ++x)
        vsum[x] -= old[x];
    }
    for (int x = 0; x < W; ++x)
      vsum[x] += hrow[x];

    /* 达到 5 行后输出中心行；只写内区（上下左右各留 2 像素为 0） */
    if (y >= 4) {
      int yc = y - hk;
      if (yc >= hk && yc < H - hk) {
        uint8_t *drow = dst + (size_t)yc * (size_t)W;
        for (int x = hk; x < W - hk; ++x) {
          drow[x] = (is_dilate ? (vsum[x] > 0) : (vsum[x] == 5)) ? 255 : 0;
        }
      }
    }
  }

  free(ring);
  free(vsum);
}

/* 语义封装 */
static inline void dilate_5(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_5(s, d, W, H, 1);
}
static inline void erode_5(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_5(s, d, W, H, 0);
}

/* ------------------------ Public APIs (auto-dispatch) ------------------------
 */

void image_erode(const uint8_t *src, uint8_t *dst, int width, int height,
                 int kernel_size) {
  if (!src || !dst || width <= 0 || height <= 0)
    return;
  kernel_size = clamp_kernel_size(kernel_size);

  if (kernel_size == 3) {
    erode_3(src, dst, width, height);
  } else if (kernel_size == 5) {
    erode_5(src, dst, width, height);
  } else {
    /* 其他尺寸回退到通用可分离实现（窗口 k 的滚动计数） */
    morph_separable(src, dst, width, height, kernel_size, 0);
  }
}

void image_dilate(const uint8_t *src, uint8_t *dst, int width, int height,
                  int kernel_size) {
  if (!src || !dst || width <= 0 || height <= 0)
    return;
  kernel_size = clamp_kernel_size(kernel_size);

  if (kernel_size == 3) {
    dilate_3(src, dst, width, height);
  } else if (kernel_size == 5) {
    dilate_5(src, dst, width, height);
  } else {
    morph_separable(src, dst, width, height, kernel_size, /*is_dilate=*/1);
  }
}

/* ------------------------ Open / Close (unchanged) ------------------------ */
/* 与您原有流程一致：开=腐蚀后膨胀；闭=膨胀后腐蚀:contentReference[oaicite:3]{index=3}
 */
void image_binary_open(uint8_t *binary_img, int width, int height,
                       int kernel_size) {
  uint8_t *temp_buf = (uint8_t *)malloc((size_t)width * (size_t)height);
  if (temp_buf == NULL) {
    printf("Failed to allocate temp buffer for morph open\n");
    return;
  }
  memset(temp_buf, 0, (size_t)width * (size_t)height);
  image_erode(binary_img, temp_buf, width, height, kernel_size);
  image_dilate(temp_buf, binary_img, width, height, kernel_size);
  free(temp_buf);
}

void image_binary_close(uint8_t *binary_img, int width, int height,
                        int kernel_size) {
  uint8_t *temp_buf = (uint8_t *)malloc((size_t)width * (size_t)height);
  if (temp_buf == NULL) {
    printf("Failed to allocate temp buffer for morph close\n");
    return;
  }
  memset(temp_buf, 0, (size_t)width * (size_t)height);
  image_dilate(binary_img, temp_buf, width, height, kernel_size);
  image_erode(temp_buf, binary_img, width, height, kernel_size);
  free(temp_buf);
}

static inline int find_root(int *parent, int x) {
  // 迭代式路径压缩
  int root = x;
  while (parent[root] != root) {
    root = parent[root];
  }
  while (x != root) {
    int p = parent[x];
    parent[x] = root;
    x = p;
  }
  return root;
}

static inline void union_sets(int *parent, uint8_t *rank, int a, int b) {
  int ra = find_root(parent, a);
  int rb = find_root(parent, b);
  if (ra == rb)
    return;
  if (rank[ra] < rank[rb]) {
    parent[ra] = rb;
  } else if (rank[ra] > rank[rb]) {
    parent[rb] = ra;
  } else {
    parent[rb] = ra;
    rank[ra]++;
  }
}

int find_blobs(const uint8_t *binary_img, int width, int height,
               BlobInfo *blobs, int max_blobs) {
  if (!binary_img || !blobs || width <= 0 || height <= 0 || max_blobs <= 0) {
    return 0;
  }

  const int W = width;
  const int H = height;
  const size_t N = (size_t)W * (size_t)H;

  /* ---------- 1) 编译期静态工作区（上限由 CAMERA_* 与宏推导） ---------- */
  enum {
    MAX_PIXELS = (size_t)CAMERA_WIDTH * (size_t)CAMERA_HEIGHT,
    MAX_LABELS = (int)(MAX_PIXELS / 2) + 128
  };

  /* 分辨率越界保护：如果调用传入的尺寸大于编译期上限，直接拒绝，避免越界 */
  if (N == 0 || N > MAX_PIXELS) {
    return 0;
  }

  /* 所有工作区静态分配（.bss/.data），运行时不再分配 */
  static int labels_static[MAX_PIXELS];   /* N * 4B */
  static int parent_static[MAX_LABELS];   /* MAX_LABELS * 4B */
  static uint8_t rank_static[MAX_LABELS]; /* MAX_LABELS * 1B */
  /* 统计容器，按“出现过的 root 标签的上界 L”索引；为简化也用 MAX_LABELS 上限 */
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

  for (int y = 0; y < H; ++y) {
    const int yW = y * W;
    for (int x = 0; x < W; ++x) {
      const int idx = yW + x;
      if (binary_img[idx] != 255)
        continue;

      int nlabels[4];
      int ncnt = 0;
      if (y > 0) {
        int t = labels[(y - 1) * W + x];
        if (t)
          nlabels[ncnt++] = t;
      }
      if (x > 0) {
        int t = labels[yW + (x - 1)];
        if (t)
          nlabels[ncnt++] = t;
      }
      if (y > 0 && x > 0) {
        int t = labels[(y - 1) * W + x - 1];
        if (t)
          nlabels[ncnt++] = t;
      }
      if (y > 0 && x < W - 1) {
        int t = labels[(y - 1) * W + x + 1];
        if (t)
          nlabels[ncnt++] = t;
      }

      if (ncnt == 0) {
        if (next_label >= MAX_LABELS) {
          return 0;
        }
        labels[idx] = next_label;
        parent[next_label] = next_label;
        rank[next_label] = 0;
        next_label++;
      } else {
        int min_root = nlabels[0];
        min_root = find_root(parent, min_root);
        for (int k = 1; k < ncnt; ++k) {
          int rk = find_root(parent, nlabels[k]);
          if (rk != min_root) {
            if (rk < min_root) {
              union_sets(parent, rank, min_root, rk);
              min_root = find_root(parent, min_root);
            } else {
              union_sets(parent, rank, rk, min_root);
              min_root = find_root(parent, min_root);
            }
          }
        }
        labels[idx] = min_root;
      }
    }
  }

  if (next_label == 1) {
    return 0;
  }

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
      int r = find_root(parent, lab);
      labels[idx] = r;

      pix_cnt_static[r] += 1;
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

  int blob_count = 0;
  for (int r = 1; r < L && blob_count < max_blobs; ++r) {
    if (pix_cnt_static[r] > 0) {
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