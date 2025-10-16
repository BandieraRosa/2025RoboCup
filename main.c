#include "image_process.h"
#include "plic.h"
#include "sysctl.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./BSP/CAMERA/camera.h"
#include "./BSP/KEY/key.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/UART/uart.h"

#define DEBUG
// #define PIXEL_DEBUG

#define MAX_BLOBS 50

/* ---------------- LUT 与分类器 ---------------- */

static uint8_t R5_TO_8[32];
static uint8_t G6_TO_8[64];
static uint8_t B5_TO_8[32];

static inline void rgb565_luts_init(void) {
  for (int v = 0; v < 32; ++v) {
    R5_TO_8[v] = (uint8_t)((v << 3) | (v >> 2));
    B5_TO_8[v] = (uint8_t)((v << 3) | (v >> 2));
  }
  for (int v = 0; v < 64; ++v) {
    G6_TO_8[v] = (uint8_t)((v << 2) | (v >> 4));
  }
}

static inline uint8_t classify_rgb565(uint16_t p) {
  uint8_t r = R5_TO_8[(p >> 11) & 0x1F];
  uint8_t g = G6_TO_8[(p >> 5) & 0x3F];
  uint8_t b = B5_TO_8[p & 0x1F];

  uint8_t max_c = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
  uint8_t min_c = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
  uint8_t sat = (uint8_t)(max_c - min_c);

  if (sat < SATURATION_THRESHOLD || max_c < BRIGHTNESS_THRESHOLD)
    return 0;
  if ((int)r > (int)b + COLOR_DIFF_THRESHOLD && r > g)
    return 1;
  if ((int)b > (int)r + COLOR_DIFF_THRESHOLD && b > g)
    return 2;
  return 0;
}

/* --------------------------------------------------------------------------*/

void init() {
  sysctl_pll_set_freq(SYSCTL_PLL0, 800000000);
  sysctl_pll_set_freq(SYSCTL_PLL1, 400000000);
  sysctl_pll_set_freq(SYSCTL_PLL2, 45158400);
  sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
  sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
  sysctl_set_spi0_dvp_data(1);
  plic_init();
  sysctl_enable_irq();

  usart_init(115200);
  lcd_init();
  key_init();
  camera_init(0);
  camera_set_pixformat(PIXFORMAT_RGB565);
  camera_set_framesize(CAMERA_WIDTH, CAMERA_HEIGHT);
  camera_set_hmirror(0);
  camera_set_vflip(0);
  camera_set_light(1);
  rgb565_luts_init();
}

#ifdef DEBUG
int main(void) {
  init();

  uint8_t *binary_buf = (uint8_t *)malloc(CAMERA_WIDTH * CAMERA_HEIGHT);
  if (binary_buf == NULL) {
    printf("Error: Failed to allocate binary buffer\n");
    return -1;
  }

  uint8_t *debug_buf = (uint8_t *)malloc(CAMERA_WIDTH * CAMERA_HEIGHT * 2);
  if (debug_buf == NULL) {
    printf("Error: Failed to allocate debug buffer\n");
    free(binary_buf);
    return -1;
  }

  uint8_t *color_buf = (uint8_t *)malloc(CAMERA_WIDTH * CAMERA_HEIGHT);
  if (color_buf == NULL) {
    printf("Error: Failed to allocate color buffer\n");
    free(binary_buf);
    free(debug_buf);
    return -1;
  }

  BlobInfo blobs[MAX_BLOBS];
  uint8_t *camera_buf;

  uint8_t key;

  while (1) {
    key = key_scan(0);
    if (key) {
      switch (key) {
      case KEY2_PRES:
        camera_set_light(1);
        break;
      case KEY1_PRES:
        camera_set_light(0);
        break;
      }
    }

    if (camera_snapshot(&camera_buf, NULL) == 0) {
      uint16_t *pixel_ptr = (uint16_t *)camera_buf;

      const int N = CAMERA_WIDTH * CAMERA_HEIGHT;
      uint16_t *p = pixel_ptr;
      uint16_t *p_end = p + N;
      uint8_t *bin = binary_buf;
      uint8_t *cbuf = color_buf;
      int red_pixel_count = 0;
      int blue_pixel_count = 0;

      while (p < p_end) {
        uint16_t pix = *p++;
        uint8_t cls = classify_rgb565(pix); // 0/1/2
        *cbuf++ = cls;
        uint8_t v = (uint8_t)(cls ? 255 : 0);
        *bin++ = v;
        red_pixel_count += (cls == 1);
        blue_pixel_count += (cls == 2);
      }

      image_binary_open(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, 5);
      image_binary_close(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, 5);
      image_binary_close(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, 3);

      int blob_count =
          find_blobs(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, blobs, MAX_BLOBS);

      BlobInfo largest_blob = {0};
      int max_pixels = 0;
      if (blob_count > 0) {
        for (int i = 0; i < blob_count; i++) {
          if (blobs[i].pixel_count > max_pixels) {
            max_pixels = blobs[i].pixel_count;
            largest_blob = blobs[i];
          }
        }
        printf("Largest Blob: Top=%d, Left=%d, Right=%d, Bottom=%d\n",
               largest_blob.min_y, largest_blob.min_x, largest_blob.max_x,
               largest_blob.max_y);
      }

      int ball_found = 0;
      printf("Largest Blob Pixel Count: %d\n", largest_blob.pixel_count);
      if (largest_blob.pixel_count >= MIN_BALL_PIXELS) {
        int width = largest_blob.max_x - largest_blob.min_x;
        int height = largest_blob.max_y - largest_blob.min_y;

        if (height > 0 && width > 0) {
          float aspect_ratio = (float)width / height;
          printf("Blob WxH: %d x %d\n", width, height);
          printf("Aspect Ratio: %.2f\n", aspect_ratio);
          if (aspect_ratio > ASPECT_RATIO_MIN &&
              aspect_ratio < ASPECT_RATIO_MAX) {
            ball_found = 1;
          }
        }
      }

      char status_msg[50];
      if (ball_found) {
        int red_in_blob = 0, blue_in_blob = 0;

        int x1 = largest_blob.min_x < 0 ? 0 : largest_blob.min_x;
        int y1 = largest_blob.min_y < 0 ? 0 : largest_blob.min_y;
        int x2 = largest_blob.max_x > (CAMERA_WIDTH - 1) ? (CAMERA_WIDTH - 1)
                                                         : largest_blob.max_x;
        int y2 = largest_blob.max_y > (CAMERA_HEIGHT - 1) ? (CAMERA_HEIGHT - 1)
                                                          : largest_blob.max_y;

        for (int y = y1; y <= y2; ++y) {
          int base = y * CAMERA_WIDTH;
          const uint8_t *bin_row = &binary_buf[base];
          const uint8_t *col_row = &color_buf[base];
          for (int x = x1; x <= x2; ++x) {
            if (bin_row[x] != 255)
              continue;
            uint8_t c = col_row[x];
            red_in_blob += (c == 1);
            blue_in_blob += (c == 2);
          }
        }

        int red =
            (red_in_blob + blue_in_blob > 0) ? red_in_blob : red_pixel_count;
        int blue =
            (red_in_blob + blue_in_blob > 0) ? blue_in_blob : blue_pixel_count;

        const char *color_str = (red > blue) ? "RED" : "BLUE";
        sprintf(status_msg, "Ball: %s | Area: %d", color_str,
                largest_blob.pixel_count);

        enum COLOR color = (red > blue) ? BALL_RED : BALL_BLUE;
        uint8_t response = 0x00;

        if (color == BALL_RED) {
          response = 0x01;
        } else if (color == BALL_BLUE) {
          response = 0x02;
        }

        if (response) {
          for (int tries = 0; tries < 50; ++tries) {
            if (uart_send_data(UART_NUM, &response, 1) == 1)
              break;
          }
        }
      } else {
        sprintf(status_msg, "No Ball Found");
        printf("NONE\n");
      }

      gray_to_rgb565(binary_buf, debug_buf, CAMERA_WIDTH, CAMERA_HEIGHT);
      if (ball_found) {
        draw_box_rgb565_image((uint16_t *)debug_buf, CAMERA_WIDTH,
                              largest_blob.min_x, largest_blob.min_y,
                              largest_blob.max_x, largest_blob.max_y, 0x07E0);
      }
      draw_fill_rectangle_image((uint16_t *)debug_buf, CAMERA_WIDTH, 0, 0,
                                CAMERA_WIDTH, 22, 0x0000);
      draw_string_rgb565_image((uint16_t *)debug_buf, CAMERA_WIDTH,
                               CAMERA_HEIGHT, 5, 5, status_msg, 0xFFFF);
      lcd_draw_picture(0, 0, CAMERA_WIDTH, CAMERA_HEIGHT,
                       (uint16_t *)debug_buf);

      camera_snapshot_release();
    }
  }
  free(binary_buf);
  free(debug_buf);
  free(color_buf);
  return 0;
}

#elif defined(PIXEL_DEBUG)
int main(void) {
  init();
  uint8_t *camera_buf;
  while (1) {
    if (camera_snapshot(&camera_buf, NULL) == 0) {
      uint16_t *pixel_ptr = (uint16_t *)camera_buf;

      uint16_t center_pixel =
          pixel_ptr[(CAMERA_HEIGHT / 2) * CAMERA_WIDTH + (CAMERA_WIDTH / 2)];
      uint8_t r = R5_TO_8[(center_pixel >> 11) & 0x1F];
      uint8_t g = G6_TO_8[(center_pixel >> 5) & 0x3F];
      uint8_t b = B5_TO_8[center_pixel & 0x1F];
      printf("Center Pixel: R=%d, G=%d, B=%d\n", r, g, b);
      camera_snapshot_release();
    }
  }
  free(camera_buf);
  return 0;
}

#else
int main(void) {
  init();
  uint8_t *binary_buf = (uint8_t *)malloc(CAMERA_WIDTH * CAMERA_HEIGHT);
  if (binary_buf == NULL) {
    printf("Error: Failed to allocate binary buffer\n");
    return -1;
  }

  /* 新增：颜色缓存 */
  uint8_t *color_buf = (uint8_t *)malloc(CAMERA_WIDTH * CAMERA_HEIGHT);
  if (color_buf == NULL) {
    printf("Error: Failed to allocate color buffer\n");
    free(binary_buf);
    return -1;
  }

  BlobInfo blobs[MAX_BLOBS];
  uint8_t *camera_buf;

  uint8_t key;

  while (1) {
    key = key_scan(0);
    if (key) {
      switch (key) {
      case KEY2_PRES:
        camera_set_light(1);
        break;
      case KEY1_PRES:
        camera_set_light(0);
        break;
      }
    }
    if (camera_snapshot(&camera_buf, NULL) == 0) {
      uint16_t *pixel_ptr = (uint16_t *)camera_buf;

      /* ---- 单趟线性扫描：分类 + 计数 + 生成二值图/颜色缓存 ---- */
      const int N = CAMERA_WIDTH * CAMERA_HEIGHT;
      uint16_t *p = pixel_ptr;
      uint16_t *p_end = p + N;
      uint8_t *bin = binary_buf;
      uint8_t *cbuf = color_buf;
      int red_pixel_count = 0;
      int blue_pixel_count = 0;

      while (p < p_end) {
        uint16_t pix = *p++;
        uint8_t cls = classify_rgb565(pix); // 0/1/2
        *cbuf++ = cls;
        uint8_t v = (uint8_t)(cls ? 255 : 0);
        *bin++ = v;
        red_pixel_count += (cls == 1);
        blue_pixel_count += (cls == 2);
      }

      image_binary_open(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, 5);
      image_binary_close(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, 5);
      image_binary_close(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, 3);

      int blob_count =
          find_blobs(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, blobs, MAX_BLOBS);

      BlobInfo largest_blob = {0};
      int max_pixels = 0;
      if (blob_count > 0) {
        for (int i = 0; i < blob_count; i++) {
          if (blobs[i].pixel_count > max_pixels) {
            max_pixels = blobs[i].pixel_count;
            largest_blob = blobs[i];
          }
        }
      }

      int ball_found = 0;
      if (largest_blob.pixel_count >= MIN_BALL_PIXELS) {
        int width = largest_blob.max_x - largest_blob.min_x;
        int height = largest_blob.max_y - largest_blob.min_y;

        if (height > 0 && width > 0) {
          float aspect_ratio = (float)width / height;
          if (aspect_ratio > ASPECT_RATIO_MIN &&
              aspect_ratio < ASPECT_RATIO_MAX) {
            ball_found = 1;
          }
        }
      }

      if (ball_found) {
        /* ROI 内直接统计颜色（避免二次解码） */
        int red_in_blob = 0, blue_in_blob = 0;

        int x1 = largest_blob.min_x < 0 ? 0 : largest_blob.min_x;
        int y1 = largest_blob.min_y < 0 ? 0 : largest_blob.min_y;
        int x2 = largest_blob.max_x > (CAMERA_WIDTH - 1) ? (CAMERA_WIDTH - 1)
                                                         : largest_blob.max_x;
        int y2 = largest_blob.max_y > (CAMERA_HEIGHT - 1) ? (CAMERA_HEIGHT - 1)
                                                          : largest_blob.max_y;

        for (int y = y1; y <= y2; ++y) {
          int base = y * CAMERA_WIDTH;
          const uint8_t *bin_row = &binary_buf[base];
          const uint8_t *col_row = &color_buf[base];
          for (int x = x1; x <= x2; ++x) {
            if (bin_row[x] != 255)
              continue;
            uint8_t c = col_row[x];
            red_in_blob += (c == 1);
            blue_in_blob += (c == 2);
          }
        }

        enum COLOR color = ((red_in_blob + blue_in_blob > 0)
                                ? (red_in_blob > blue_in_blob)
                                : (red_pixel_count > blue_pixel_count))
                               ? BALL_RED
                               : BALL_BLUE;

        uint8_t response = 0x00;
        if (color == BALL_RED) {
          response = 0x01;
        } else if (color == BALL_BLUE) {
          response = 0x02;
        }
        if (response) {
          for (int tries = 0; tries < 50; ++tries) {
            if (uart_send_data(UART_NUM, &response, 1) == 1)
              break;
          }
        }
      }

      if (ball_found) {
        draw_box_rgb565_image(pixel_ptr, CAMERA_WIDTH, largest_blob.min_x,
                              largest_blob.min_y, largest_blob.max_x,
                              largest_blob.max_y, 0x07E0);
      }
      draw_fill_rectangle_image(pixel_ptr, CAMERA_WIDTH, 0, 0, CAMERA_WIDTH, 22,
                                0x0000);
      lcd_draw_picture(0, 0, CAMERA_WIDTH, CAMERA_HEIGHT, pixel_ptr);

      camera_snapshot_release();
    }
  }

  /* （理论上不可达） */
  free(binary_buf);
  free(color_buf);
  return 0;
}

#endif
