#include "image_process.h"
#include "./BSP/LCD/lcdfont.h"
#include "iomem.h"
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
  if (image && image->addr) {
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

  uint16_t w_src = image_src->width, h_src = image_src->height;
  uint16_t w_dst = image_dst->width, h_dst = image_dst->height;
  if (x_offset >= w_src || y_offset >= h_src)
    return;

  size_t copy_w =
      (size_t)((w_src - x_offset) < w_dst ? (w_src - x_offset) : w_dst);
  size_t copy_h =
      (size_t)((h_src - y_offset) < h_dst ? (h_src - y_offset) : h_dst);

  size_t src_line = (size_t)w_src, dst_line = (size_t)w_dst;

  uint8_t *r_src = image_src->addr + y_offset * src_line + x_offset;
  uint8_t *g_src = r_src + src_line * h_src;
  uint8_t *b_src = g_src + src_line * h_src;

  uint8_t *r_dst = image_dst->addr;
  uint8_t *g_dst = r_dst + dst_line * h_dst;
  uint8_t *b_dst = g_dst + dst_line * h_dst;

  for (size_t y = 0; y < copy_h; ++y) {
    memcpy(r_dst + y * dst_line, r_src + y * src_line, copy_w);
    memcpy(g_dst + y * dst_line, g_src + y * src_line, copy_w);
    memcpy(b_dst + y * dst_line, b_src + y * src_line, copy_w);
  }
}

void image_draw(image_t *image_src, image_t *image_dst, uint16_t x_start,
                uint16_t y_start) {
  if (!image_buffer_ready(image_src) || !image_buffer_ready(image_dst))
    return;

  uint16_t w_src = image_src->width, h_src = image_src->height;
  uint16_t w_dst = image_dst->width, h_dst = image_dst->height;
  if (x_start >= w_dst || y_start >= h_dst)
    return;

  size_t copy_w =
      (size_t)(w_src < (w_dst - x_start) ? w_src : (w_dst - x_start));
  size_t copy_h =
      (size_t)(h_src < (h_dst - y_start) ? h_src : (h_dst - y_start));

  size_t src_line = (size_t)w_src, dst_line = (size_t)w_dst;
  size_t dst_off = (size_t)y_start * dst_line + x_start;

  uint8_t *r_src = image_src->addr;
  uint8_t *g_src = r_src + src_line * h_src;
  uint8_t *b_src = g_src + src_line * h_src;

  uint8_t *r_dst = image_dst->addr + dst_off;
  uint8_t *g_dst = image_dst->addr + dst_line * h_dst + dst_off;
  uint8_t *b_dst = image_dst->addr + 2U * dst_line * h_dst + dst_off;

  for (size_t y = 0; y < copy_h; ++y) {
    memcpy(r_dst + y * dst_line, r_src + y * src_line, copy_w);
    memcpy(g_dst + y * dst_line, g_src + y * src_line, copy_w);
    memcpy(b_dst + y * dst_line, b_src + y * src_line, copy_w);
  }
}

void image_resize(image_t *image_src, image_t *image_dst) {
  uint16_t w_src = image_src->width, h_src = image_src->height;
  uint8_t *r_src = image_src->addr;
  uint8_t *g_src = r_src + w_src * h_src;
  uint8_t *b_src = g_src + w_src * h_src;

  uint16_t w_dst = image_dst->width, h_dst = image_dst->height;
  uint8_t *r_dst = image_dst->addr;
  uint8_t *g_dst = r_dst + w_dst * h_dst;
  uint8_t *b_dst = g_dst + w_dst * h_dst;

  float w_scale = (float)w_src / (float)w_dst;
  float h_scale = (float)h_src / (float)h_dst;

  for (uint16_t y = 0; y < h_dst; y++) {
    for (uint16_t x = 0; x < w_dst; x++) {
      float x_src = (x + 0.5f) * w_scale - 0.5f;
      float y_src = (y + 0.5f) * h_scale - 0.5f;
      uint16_t x1 = (uint16_t)x_src, y1 = (uint16_t)y_src;
      uint16_t x2 = x1 + 1, y2 = y1 + 1;

      if (x2 >= w_src || y2 >= h_src) {
        r_dst[x + y * w_dst] = r_src[x1 + y1 * w_src];
        g_dst[x + y * w_dst] = g_src[x1 + y1 * w_src];
        b_dst[x + y * w_dst] = b_src[x1 + y1 * w_src];
        continue;
      }

      float t1 = (x2 - x_src) * r_src[x1 + y1 * w_src] +
                 (x_src - x1) * r_src[x2 + y1 * w_src];
      float t2 = (x2 - x_src) * r_src[x1 + y2 * w_src] +
                 (x_src - x1) * r_src[x2 + y2 * w_src];
      r_dst[x + y * w_dst] = (uint8_t)((y2 - y_src) * t1 + (y_src - y1) * t2);

      t1 = (x2 - x_src) * g_src[x1 + y1 * w_src] +
           (x_src - x1) * g_src[x2 + y1 * w_src];
      t2 = (x2 - x_src) * g_src[x1 + y2 * w_src] +
           (x_src - x1) * g_src[x2 + y2 * w_src];
      g_dst[x + y * w_dst] = (uint8_t)((y2 - y_src) * t1 + (y_src - y1) * t2);

      t1 = (x2 - x_src) * b_src[x1 + y1 * w_src] +
           (x_src - x1) * b_src[x2 + y1 * w_src];
      t2 = (x2 - x_src) * b_src[x1 + y2 * w_src] +
           (x_src - x1) * b_src[x2 + y2 * w_src];
      b_dst[x + y * w_dst] = (uint8_t)((y2 - y_src) * t1 + (y_src - y1) * t2);
    }
  }
}

void image_replace(uint8_t *image_addr, uint16_t image_width,
                   uint16_t image_height, uint8_t vflip, uint8_t hmirror) {
  uint8_t *src = image_addr, *r_src, *g_src, *b_src, temp;
  uint16_t t1, t2;
  uint32_t o1, o2;

  if (vflip == 1) {
    t1 = image_height >> 1;
    r_src = src;
    g_src = r_src + image_width * image_height;
    b_src = g_src + image_width * image_height;

    for (uint16_t j = 0; j < t1; j++) {
      for (uint16_t i = 0; i < image_width; i++) {
        o1 = (uint32_t)image_width * j + i;
        o2 = (uint32_t)image_width * (image_height - 1 - j) + i;

        temp = r_src[o1];
        r_src[o1] = r_src[o2];
        r_src[o2] = temp;
        temp = g_src[o1];
        g_src[o1] = g_src[o2];
        g_src[o2] = temp;
        temp = b_src[o1];
        b_src[o1] = b_src[o2];
        b_src[o2] = temp;
      }
    }
  }

  if (hmirror == 1) {
    t2 = image_width >> 1;
    for (uint16_t j = 0; j < image_height * 3; j++) {
      for (uint16_t i = 0; i < t2; i++) {
        o1 = image_width - i - 1;
        temp = src[i];
        src[i] = src[o1];
        src[o1] = temp;
      }
      src += image_width;
    }
  }
}

void image_invert(uint8_t *image_addr, uint16_t image_width,
                  uint16_t image_height) {
  uint8_t *r = image_addr;
  uint8_t *g = r + image_width * image_height;
  uint8_t *b = g + image_width * image_height;

  for (uint32_t j = 0; j < (uint32_t)image_width * image_height; j++) {
    *r = (uint8_t)~(*r);
    *g = (uint8_t)~(*g);
    *b = (uint8_t)~(*b);
    r++;
    g++;
    b++;
  }
}

void image_strech_chart(uint8_t *image_addr, uint16_t image_width,
                        uint16_t image_height, uint8_t de_dark) {
  uint8_t *r = image_addr, *g = r + image_width * image_height,
          *b = g + image_width * image_height;
  uint8_t *in = r;
  uint32_t idx;
  uint16_t graymax = 0;
  int sx = 0, sx2 = 0;

  for (idx = 0; idx < (uint32_t)image_width * image_height; idx++) {
    if (in[idx] > graymax)
      graymax = in[idx];
    sx += in[idx];
    sx2 += ((int)in[idx] * (int)in[idx]);
  }

  int ex = sx / image_width / image_height;
  int gate = ex;

  for (idx = 0; idx < (uint32_t)image_width * image_height; idx++) {
    uint16_t x = (uint16_t)(idx % image_width);
    uint16_t y = (uint16_t)(idx / image_width);
    int dat = in[idx];
    int denom = (int)graymax - gate;
    if (denom == 0)
      denom = 1;

    dat = (dat - gate) * 255 / denom;
    dat = dat < 0 ? 0 : (dat > 255 ? 255 : dat);

    int r2 = (x - image_width / 2) * (x - image_width / 2) +
             (y - image_height / 2) * (y - image_height / 2);
    if (de_dark)
      dat = (int)(dat / (1.0 + 32.0 * r2 * r2 / image_width / image_width /
                                   image_height / image_height));

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

void gray_to_rgb565_pixel(uint8_t gray, uint16_t *rgb565) {
  uint8_t r, g, b;
  gray_to_rgb888_pixel(gray, &r, &g, &b);
  rgb888_to_rgb565_pixel(r, g, b, rgb565);
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

void draw_string_rgb565_image(uint16_t *image_addr, uint16_t image_width,
                              uint16_t image_height, uint16_t x, uint16_t y,
                              char *str, uint16_t color) {
  uint16_t *src = image_addr, *origin;
  uint16_t w = image_width, h = image_height;

  uint16_t slen = (uint16_t)strlen(str);
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
    for (uint8_t i = 0; i < 16; i++) {
      uint8_t data = ascii0816[(uint8_t)(*str) * 16 + i];
      src = origin + (size_t)i * w;
      for (uint8_t j = 0; j < 8; j++) {
        if (data & 0x80)
          src[j] = color;
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
  uint32_t *a1, *a2, *a3, *a4;

  if (x1 < 1)
    x1 = 0;
  if (x2 > 319)
    x2 = 319;
  if (y1 < 1)
    y1 = 0;
  if (y2 > 239)
    y2 = 239;

  a1 = (uint32_t *)image_addr + ((size_t)image_width * y1 + x1) / 2;
  a2 = (uint32_t *)image_addr + ((size_t)image_width * (y1 + 1) + x1) / 2;
  a3 = (uint32_t *)image_addr + ((size_t)image_width * y2 + x1) / 2;
  a4 = (uint32_t *)image_addr + ((size_t)image_width * (y2 - 1) + x1) / 2;

  for (uint8_t i = 0; i < (uint8_t)((x2 - x1) / 2); i++) {
    *a1++ = data;
    *a2++ = data;
    *a3++ = data;
    *a4++ = data;
  }

  a1 = (uint32_t *)image_addr + ((size_t)image_width * y1 + x1) / 2;
  a2 = (uint32_t *)image_addr + ((size_t)image_width * y1 + x2) / 2 - 1;
  for (uint16_t i = 0; i < (uint16_t)(y2 - y1); i++) {
    *a1 = data;
    *a2 = data;
    a1 += image_width / 2;
    a2 += image_width / 2;
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

  addr = (uint32_t *)image_addr + ((size_t)image_width * y1 + x1) / 2;
  for (uint8_t j = 0; j < (uint8_t)(y2 - y1); j++) {
    for (uint8_t i = 0; i < (uint8_t)((x2 - x1) / 2); i++)
      addr[i] = data;
    addr += image_width / 2;
  }
}

void draw_point_rgb565_image(uint16_t *image_addr, uint16_t image_width,
                             uint16_t x, uint16_t y, uint16_t color) {
  if (x > 319)
    x = 319;
  if (y > 239)
    y = 239;
  image_addr[(size_t)y * image_width + x] = color;
}

static inline int clamp_kernel_size(int k) {
  if (k < 1)
    k = 1;
  if ((k & 1) == 0)
    k += 1;
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

static inline void dilate_3(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_3(s, d, W, H, 1);
}
static inline void erode_3(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_3(s, d, W, H, 0);
}

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

static inline void dilate_5(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_5(s, d, W, H, 1);
}
static inline void erode_5(const uint8_t *s, uint8_t *d, int W, int H) {
  morph_5(s, d, W, H, 0);
}

void image_erode(const uint8_t *src, uint8_t *dst, int width, int height,
                 int kernel_size) {
  if (!src || !dst || width <= 0 || height <= 0)
    return;
  kernel_size = clamp_kernel_size(kernel_size);
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
  image_dilate(binary_img, tmp, width, height, kernel_size);
  image_erode(tmp, binary_img, width, height, kernel_size);
  free(tmp);
}

static inline int find_root(int *parent, int x) {
  int root = x;
  while (parent[root] != root)
    root = parent[root];
  while (x != root) {
    int p = parent[x];
    parent[x] = root;
    x = p;
  }
  return root;
}

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

  enum {
    MAX_PIXELS = (size_t)CAMERA_WIDTH * (size_t)CAMERA_HEIGHT,
    MAX_LABELS = (int)(MAX_PIXELS / 2) + 128
  };
  if (N == 0 || N > MAX_PIXELS)
    return 0;

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

  for (int y = 0; y < H; ++y) {
    const int yW = y * W;
    for (int x = 0; x < W; ++x) {
      const int idx = yW + x;
      if (binary_img[idx] != 255)
        continue;

      int nl[4];
      int ncnt = 0;
      if (y > 0) {
        int t = labels[(y - 1) * W + x];
        if (t)
          nl[ncnt++] = t;
      }
      if (x > 0) {
        int t = labels[yW + (x - 1)];
        if (t)
          nl[ncnt++] = t;
      }
      if (y > 0 && x > 0) {
        int t = labels[(y - 1) * W + x - 1];
        if (t)
          nl[ncnt++] = t;
      }
      if (y > 0 && x < W - 1) {
        int t = labels[(y - 1) * W + x + 1];
        if (t)
          nl[ncnt++] = t;
      }

      if (ncnt == 0) {
        if (next_label >= MAX_LABELS)
          return 0;
        labels[idx] = next_label;
        parent[next_label] = next_label;
        rank[next_label] = 0;
        next_label++;
      } else {
        int mr = find_root(parent, nl[0]);
        for (int k = 1; k < ncnt; ++k) {
          int rk = find_root(parent, nl[k]);
          if (rk != mr) {
            if (rk < mr) {
              union_sets(parent, rank, mr, rk);
              mr = find_root(parent, mr);
            } else {
              union_sets(parent, rank, rk, mr);
              mr = find_root(parent, mr);
            }
          }
        }
        labels[idx] = mr;
      }
    }
  }

  if (next_label == 1)
    return 0;

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
