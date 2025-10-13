#include "image_process.h"
#include "plic.h"
#include "sysctl.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./BSP/CAMERA/camera.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/UART/uart.h"
#include "./BSP/KEY/key.h"

// #define DEBUG

#define MAX_BLOBS 100
#define MIN_BALL_PIXELS 4000

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
  camera_set_hmirror(1);
  camera_set_light(1);
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

      int red_pixel_count = 0;
      int blue_pixel_count = 0;

      for (int i = 0; i < CAMERA_WIDTH * CAMERA_HEIGHT; i++) {
        uint16_t pixel_color = pixel_ptr[i];
        uint8_t r = ((pixel_color & 0xF800) >> 11) << 3;
        uint8_t g = ((pixel_color & 0x07E0) >> 5) << 2;
        uint8_t b = (pixel_color & 0x001F) << 3;

        uint8_t max_c = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
        uint8_t min_c = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
        uint8_t sat = max_c - min_c;

        binary_buf[i] = 0;

        if (sat >= SATURATION_THRESHOLD && max_c >= BRIGHTNESS_THRESHOLD) {
          if (r > (b + COLOR_DIFF_THRESHOLD) && r > g) {
            red_pixel_count++;
            binary_buf[i] = 255;
          } else if (b > (r + COLOR_DIFF_THRESHOLD) && b > g) {
            blue_pixel_count++;
            binary_buf[i] = 255;
          }
        }
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
          if (aspect_ratio > 0.3f && aspect_ratio < 1.6f) {
            ball_found = 1;
          }
        }
      }

      char status_msg[50];
      if (ball_found) {
        const char *color_str =
            (red_pixel_count > blue_pixel_count) ? "RED" : "BLUE";
        sprintf(status_msg, "Ball: %s | Area: %d", color_str,
                largest_blob.pixel_count);
        printf("FOUND %s BALL at (%d, %d)\n", color_str,
               (largest_blob.min_x + largest_blob.max_x) / 2,
               (largest_blob.min_y + largest_blob.max_y) / 2);
        enum COLOR color =
            (red_pixel_count > blue_pixel_count) ? BALL_RED : BALL_BLUE;
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

      int red_pixel_count = 0;
      int blue_pixel_count = 0;

      for (int i = 0; i < CAMERA_WIDTH * CAMERA_HEIGHT; i++) {
        uint16_t pixel_color = pixel_ptr[i];
        uint8_t r = ((pixel_color & 0xF800) >> 11) << 3;
        uint8_t g = ((pixel_color & 0x07E0) >> 5) << 2;
        uint8_t b = (pixel_color & 0x001F) << 3;

        uint8_t max_c = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
        uint8_t min_c = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
        uint8_t sat = max_c - min_c;

        binary_buf[i] = 0;

        if (sat >= SATURATION_THRESHOLD && max_c >= BRIGHTNESS_THRESHOLD) {
          if (r > (b + COLOR_DIFF_THRESHOLD) && r > g) {
            red_pixel_count++;
            binary_buf[i] = 255;
          } else if (b > (r + COLOR_DIFF_THRESHOLD) && b > g) {
            blue_pixel_count++;
            binary_buf[i] = 255;
          }
        }
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
          if (aspect_ratio > 0.3f && aspect_ratio < 1.6f) {
            ball_found = 1;
          }
        }
      }

      if (ball_found) {
        enum COLOR color =
            (red_pixel_count > blue_pixel_count) ? BALL_RED : BALL_BLUE;
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

  free(binary_buf);
  return 0;
}

#endif