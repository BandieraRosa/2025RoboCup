/**
 * @file main.c
 * @brief 红色和蓝色球体识别与跟踪的主程序
 *
 * 该程序通过摄像头捕捉图像，对图像进行处理以识别红色或蓝色的球体。
 * 主要流程包括：
 * 1. 初始化系统硬件。
 * 2. 进入主循环，不断从摄像头获取RGB565格式的图像帧。
 * 3. 对每个像素进行颜色分类，生成一个二值图像，其中目标颜色像素为前景。
 * 4. 使用形态学开运算和闭运算清理二值图像，去除噪声并填充空洞。
 * 5. 在二值图像上运行连通区域分析，找出所有独立的色块。
 * 6. 筛选并找到最大的Blob，通过面积和宽高比等条件判断其是否为目标球体。
 * 7. 如果找到球体，判断其主要颜色，并通过UART发送识别结果。
 * 8. （调试模式下）在LCD上显示处理后的二值图像、识别到的球体边界框和状态信息。
 */

#include "./BSP/CAMERA/camera.h"
#include "./BSP/KEY/key.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/UART/uart.h"
#include "image_process.h"
#include "plic.h"
#include "sysctl.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @def DEBUG_FLAG
 * @brief 调试标志宏
 *
 * 如果定义了此宏，程序将在LCD上显示处理过程的可视化结果，
 * 包括二值化图像、找到的最大Blob的边界框以及状态信息。
 * 这对于参数调优和问题诊断非常有用。
 */
#define DEBUG_FLAG

/**
 * @def DEBUG_PIXEL
 * @brief 单像素调试宏
 *
 * 如果定义此宏，程序将进入一个简化的循环，仅打印图像中心像素的RGB值。
 * 用于检查摄像头颜色读数是否准确。
 */
// #define DEBUG_PIXEL

#define MAX_BLOBS 50 ///< 定义最大可检测的Blob数量

// --- RGB565到RGB888快速转换的查找表 ---
static uint8_t R5_TO_8[32]; ///< R通道5位到8位的映射表
static uint8_t G6_TO_8[64]; ///< G通道6位到8位的映射表
static uint8_t B5_TO_8[32]; ///< B通道5位到8位的映射表

/**
 * @brief 初始化RGB565到RGB888转换的查找表
 *
 * 预先计算所有可能的5位和6位颜色值到8位值的映射，
 * 避免在循环中重复计算，从而提高颜色转换效率。
 */
static inline void rgb565_luts_init(void) {
  for (int v = 0; v < 32; ++v) {
    R5_TO_8[v] = (uint8_t)((v << 3) | (v >> 2));
    B5_TO_8[v] = (uint8_t)((v << 3) | (v >> 2));
  }
  for (int v = 0; v < 64; ++v) {
    G6_TO_8[v] = (uint8_t)((v << 2) | (v >> 4));
  }
}

/**
 * @brief 对单个RGB565像素进行颜色分类
 *
 * 将输入的RGB565像素转换为RGB888，然后根据饱和度、亮度和颜色差异阈值
 * 判断其属于红色、蓝色还是非目标背景。
 *
 * @param[in] p 输入的RGB565像素值。
 * @return uint8_t 分类结果: 0=背景, 1=红色, 2=蓝色。
 */
static inline uint8_t classify_rgb565(uint16_t p) {
  // 使用查找表快速将RGB565转换为RGB888
  uint8_t r = R5_TO_8[(p >> 11) & 0x1F];
  uint8_t g = G6_TO_8[(p >> 5) & 0x3F];
  uint8_t b = B5_TO_8[p & 0x1F];
  
  // 计算亮度和饱和度（近似值）
  uint8_t max_c = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
  uint8_t min_c = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
  uint8_t sat = (uint8_t)(max_c - min_c);
  
  // 过滤掉饱和度和亮度过低的像素
  if (sat < SATURATION_THRESHOLD || max_c < BRIGHTNESS_THRESHOLD)
    return 0; // 背景
    
  // 根据颜色差异判断是红色还是蓝色
  if ((int)r > (int)b + COLOR_DIFF_THRESHOLD && r > g)
    return 1; // 红色
  if ((int)b > (int)r + COLOR_DIFF_THRESHOLD && b > g)
    return 2; // 蓝色
    
  return 0; // 其他颜色，归为背景
}

/**
 * @brief 系统硬件初始化
 *
 * 配置系统时钟、电源、中断、UART、LCD、按键和摄像头等外设。
 */
static inline void init() {
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
  camera_set_vflip(1);
  camera_set_light(1);
  rgb565_luts_init();
}

#ifdef DEBUG_FLAG
/**
 * @brief 主函数（调试模式）
 *
 * 在此模式下，程序执行完整的识别流程，并将中间结果和最终状态
 * 可视化地显示在LCD屏幕上。
 * @return int 程序退出代码。
 */
int main(void) {
  init();

  // 分配用于存储二值化图像的缓冲区
  uint8_t *binary_buf = (uint8_t *)malloc(CAMERA_WIDTH * CAMERA_HEIGHT);
  if (binary_buf == NULL) {
    printf("Error: Failed to allocate binary buffer\n");
    return -1;
  }

  // 分配用于LCD显示的调试缓冲区
  uint8_t *debug_buf = (uint8_t *)malloc(CAMERA_WIDTH * CAMERA_HEIGHT * 2);
  if (debug_buf == NULL) {
    printf("Error: Failed to allocate debug buffer\n");
    free(binary_buf);
    return -1;
  }
  
  // 分配用于存储每个像素颜色分类结果的缓冲区
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
    // 按键控制摄像头补光灯
    key = key_scan(0);
    if (key) {
      switch (key) {
      case KEY2_PRES: camera_set_light(1); break;
      case KEY1_PRES: camera_set_light(0); break;
      }
    }

    // 从摄像头获取一帧图像
    if (camera_snapshot(&camera_buf, NULL) == 0) {
      uint16_t *pixel_ptr = (uint16_t *)camera_buf;
      const int N = CAMERA_WIDTH * CAMERA_HEIGHT;
      
      // --- 1. 像素级处理：颜色分类与二值化 ---
      uint16_t *p = pixel_ptr;
      uint16_t *p_end = p + N;
      uint8_t *bin = binary_buf;
      uint8_t *cbuf = color_buf;
      int red_pixel_count = 0;
      int blue_pixel_count = 0;

      while (p < p_end) {
        uint16_t pix = *p++;
        uint8_t cls = classify_rgb565(pix);
        *cbuf++ = cls; // 存储分类结果 (0, 1, 2)
        *bin++ = (uint8_t)(cls ? 255 : 0); // 生成二值图像
        red_pixel_count += (cls == 1);
        blue_pixel_count += (cls == 2);
      }

      // --- 2. 形态学处理：去噪和填充 ---
      image_binary_open(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, 5);
      image_binary_close(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, 5);
      image_binary_close(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, 3);

      // --- 3. Blob检测：寻找连通区域 ---
      int blob_count = find_blobs(binary_buf, CAMERA_WIDTH, CAMERA_HEIGHT, blobs, MAX_BLOBS);

      // --- 4. Blob筛选：找到最大Blob并判断是否为球 ---
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

      // --- 5. 颜色判断与通信 ---
      char status_msg[50];
      if (ball_found) {
        // 统计Blob内部的红蓝像素数量以确定颜色
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
            if (bin_row[x] != 255) // 只统计在Blob内的像素
              continue;
            uint8_t c = col_row[x];
            red_in_blob += (c == 1);
            blue_in_blob += (c == 2);
          }
        }

        // 确定最终颜色并准备发送数据
        int red =
            (red_in_blob + blue_in_blob > 0) ? red_in_blob : red_pixel_count;
        int blue =
            (red_in_blob + blue_in_blob > 0) ? blue_in_blob : blue_pixel_count;
        
        enum COLOR color = (red > blue) ? BALL_RED : BALL_BLUE;
        uint8_t response = 0x00;

        if (color == BALL_RED) {
          response = 0x01;
        } else if (color == BALL_BLUE) {
          response = 0x02;
        }

        // 通过UART发送识别结果
        if (response) {
          for (int tries = 0; tries < 50; ++tries) {
            if (uart_send_data(UART_NUM, &response, 1) == 1)
              break;
          }
        }
        
        const char *color_str = (color == BALL_RED) ? "RED" : "BLUE";
        sprintf(status_msg, "Ball: %s | Area: %d", color_str,
                largest_blob.pixel_count);
      } else {
        sprintf(status_msg, "No Ball Found");
        printf("NONE\n");
      }

      // --- 6. 调试信息显示 ---
      gray_to_rgb565(binary_buf, debug_buf, CAMERA_WIDTH, CAMERA_HEIGHT); // 将二值图转为可显示格式
      if (ball_found) {
        // 在球的位置画一个绿色的框
        draw_box_rgb565_image((uint16_t *)debug_buf, CAMERA_WIDTH,
                              largest_blob.min_x, largest_blob.min_y,
                              largest_blob.max_x, largest_blob.max_y, 0x07E0); // 绿色
      }
      // 在屏幕顶部绘制状态信息栏
      draw_fill_rectangle_image((uint16_t *)debug_buf, CAMERA_WIDTH, 0, 0,
                                CAMERA_WIDTH, 22, 0x0000); // 黑色背景
      draw_string_rgb565_image((uint16_t *)debug_buf, CAMERA_WIDTH,
                               CAMERA_HEIGHT, 5, 5, status_msg, 0xFFFF); // 白色文字
      
      // 将最终的调试画面推送到LCD
      lcd_draw_picture(0, 0, CAMERA_WIDTH, CAMERA_HEIGHT,
                       (uint16_t *)debug_buf);

      camera_snapshot_release(); // 释放摄像头缓冲区
    }
  }
  
  // 释放所有动态分配的内存
  free(binary_buf);
  free(debug_buf);
  free(color_buf);
  return 0;
}

#elif defined(DEBUG_PIXEL)
/**
 * @brief 主函数（单像素调试模式）
 *
 * 循环读取并打印图像中心像素的RGB值，用于快速验证摄像头颜色采集是否正常。
 * @return int 程序退出代码。
 */
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
/**
 * @brief 主函数（非调试/发布模式）
 *
 * 在此模式下，程序执行核心识别逻辑，但不进行LCD可视化显示，
 * 仅通过UART发送结果，以获得最高性能。
 * @return int 程序退出代码。
 */
int main(void) {
  init();
  uint8_t *binary_buf = (uint8_t *)malloc(CAMERA_WIDTH * CAMERA_HEIGHT);
  if (binary_buf == NULL) {
    printf("Error: Failed to allocate binary buffer\n");
    return -1;
  }

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
      const int N = CAMERA_WIDTH * CAMERA_HEIGHT;
      uint16_t *p = pixel_ptr;
      uint16_t *p_end = p + N;
      uint8_t *bin = binary_buf;
      uint8_t *cbuf = color_buf;
      int red_pixel_count = 0;
      int blue_pixel_count = 0;

      while (p < p_end) {
        uint16_t pix = *p++;
        uint8_t cls = classify_rgb565(pix);
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
      
      // 在原始图像上绘制信息，用于非DEBUG_FLAG但仍需基本显示的场景
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
  free(color_buf);
  return 0;
}

#endif
