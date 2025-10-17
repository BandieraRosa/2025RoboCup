#ifndef _IMAGE_PROCESS_H
#define _IMAGE_PROCESS_H

#include <stdint.h>

#ifndef CAMERA_WIDTH
#define CAMERA_WIDTH 320
#endif
#ifndef CAMERA_HEIGHT
#define CAMERA_HEIGHT 240
#endif

#define SATURATION_THRESHOLD 50
#define BRIGHTNESS_THRESHOLD 50
#define COLOR_DIFF_THRESHOLD 60

#define MIN_BALL_PIXELS 5000
#define ASPECT_RATIO_MIN 0.3f
#define ASPECT_RATIO_MAX 1.6f
#define FILL_FACTOR_MIN 0.25f
#define FILL_FACTOR_MAX 1.0f

typedef struct {
  uint8_t *addr;
  uint16_t width;
  uint16_t height;
  uint16_t pixel;
} image_t;

typedef struct {
  int id;
  int pixel_count;
  int min_x, min_y;
  int max_x, max_y;
} BlobInfo;

enum COLOR { BALL_UNKNOWN = 0, BALL_BLUE = 1, BALL_RED = 2 };

int image_init(image_t *image);
void image_deinit(image_t *image);

void image_crop(image_t *image_src, image_t *image_dst, uint16_t x_offset, uint16_t y_offset);
void image_draw(image_t *image_src, image_t *image_dst, uint16_t x_start, uint16_t y_start);
void image_resize(image_t *image_src, image_t *image_dst);
void image_replace(uint8_t *image_addr, uint16_t image_width, uint16_t image_height, uint8_t vflip, uint8_t hmirror);
void image_invert(uint8_t *image_addr, uint16_t image_width, uint16_t image_height);
void image_strech_chart(uint8_t *image_addr, uint16_t image_width, uint16_t image_height, uint8_t de_dark);

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

void draw_string_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t image_height, uint16_t x, uint16_t y, char *str, uint16_t color);
void draw_box_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void draw_point_rgb565_image(uint16_t *image_addr, uint16_t image_width, uint16_t x, uint16_t y, uint16_t color);
void draw_fill_rectangle_image(uint16_t *image_addr, uint16_t image_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void image_binary_open(uint8_t *binary_img, int width, int height, int kernel_size);
void image_binary_close(uint8_t *binary_img, int width, int height, int kernel_size);

int find_blobs(const uint8_t *binary_img, int width, int height, BlobInfo *blobs, int max_blobs);

#endif
