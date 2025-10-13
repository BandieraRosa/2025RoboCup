// /*
// 二维码01表示：
// 1：
// 1 1 1 1 1 1 1 0 1 0 1 1 1 0 1 1 1 1 1 1 1
// 1 0 0 0 0 0 1 0 0 0 1 1 0 0 1 0 0 0 0 0 1
// 1 0 1 1 1 0 1 0 1 1 0 1 0 0 1 0 1 1 1 0 1
// 1 0 1 1 1 0 1 0 1 1 0 0 1 0 1 0 1 1 1 0 1
// 1 0 1 1 1 0 1 0 1 0 0 1 0 0 1 0 1 1 1 0 1
// 1 0 0 0 0 0 1 0 0 1 1 1 1 0 1 0 0 0 0 0 1
// 1 1 1 1 1 1 1 0 1 0 1 0 1 0 1 1 1 1 1 1 1
// 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0 0 0
// 1 1 1 1 0 0 1 0 1 1 1 1 1 1 0 0 1 1 1 0 1
// 0 1 1 0 1 0 0 0 1 1 1 1 1 1 1 0 0 1 0 0 0
// 0 1 1 1 1 0 1 1 0 1 0 1 0 0 0 0 0 1 1 0 1
// 1 1 0 1 0 1 0 1 1 0 1 1 0 0 1 1 1 1 1 0 0
// 0 0 0 0 1 0 1 1 1 0 1 0 1 0 0 1 0 0 1 0 0
// 0 0 0 0 0 0 0 0 1 1 0 1 0 0 1 0 0 1 0 1 0
// 1 1 1 1 1 1 1 0 0 1 1 1 1 0 0 1 0 1 0 1 0
// 1 0 0 0 0 0 1 0 0 1 0 0 0 0 0 1 1 0 1 1 0
// 1 0 1 1 1 0 1 0 0 1 0 0 1 1 1 1 1 0 0 0 1
// 1 0 1 1 1 0 1 0 1 1 1 1 0 0 1 1 1 1 1 1 0
// 1 0 1 1 1 0 1 0 1 0 0 0 1 0 1 1 0 0 0 0 0
// 1 0 0 0 0 0 1 0 1 1 1 0 0 1 0 1 0 0 1 0 1
// 1 1 1 1 1 1 1 0 1 0 1 0 0 1 0 0 1 0 0 0 0

// 2：
// 1 1 1 1 1 1 1 0 0 0 1 1 1 0 1 1 1 1 1 1 1
// 1 0 0 0 0 0 1 0 1 1 1 0 1 0 1 0 0 0 0 0 1
// 1 0 1 1 1 0 1 0 0 0 1 1 1 0 1 0 1 1 1 0 1
// 1 0 1 1 1 0 1 0 1 1 0 0 1 0 1 0 1 1 1 0 1
// 1 0 1 1 1 0 1 0 0 1 0 0 1 0 1 0 1 1 1 0 1
// 1 0 0 0 0 0 1 0 1 0 0 1 0 0 1 0 0 0 0 0 1
// 1 1 1 1 1 1 1 0 1 0 1 0 1 0 1 1 1 1 1 1 1 
// 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0
// 1 1 1 1 1 0 1 1 1 0 0 1 0 1 0 1 0 1 0 1 0
// 1 1 1 0 1 1 0 1 0 0 0 1 1 1 1 0 0 1 0 0 0
// 0 0 1 1 0 1 1 1 1 0 1 0 1 0 1 1 0 0 0 0 0
// 1 0 1 0 0 1 0 0 1 0 1 1 1 1 1 0 0 1 0 0 1
// 0 0 1 0 0 0 1 1 1 1 1 0 1 0 0 1 0 0 1 0 0
// 0 0 0 0 0 0 0 0 1 0 1 0 1 0 0 1 0 0 1 1 1
// 1 1 1 1 1 1 1 0 1 0 0 1 0 1 0 0 1 1 1 0 0
// 1 0 0 0 0 0 1 0 0 1 0 0 0 0 0 1 1 0 1 1 0
// 1 0 1 1 1 0 1 0 1 1 0 1 0 1 0 0 1 1 1 0 0
// 1 0 1 1 1 0 1 0 1 1 1 1 1 1 1 0 0 1 0 0 0 
// 1 0 1 1 1 0 1 0 1 0 1 0 1 0 1 1 0 0 0 0 0 
// 1 0 0 0 0 0 1 0 1 0 1 1 1 1 1 0 0 1 0 0 0
// 1 1 1 1 1 1 1 0 1 1 1 0 1 0 0 1 0 0 1 1 0

// 3：
// 1 1 1 1 1 1 1 0 0 0 1 1 1 0 1 1 1 1 1 1 1 
// 1 0 0 0 0 0 1 0 1 1 1 0 1 0 1 0 0 0 0 0 1
// 1 0 1 1 1 0 1 0 0 0 1 1 1 0 1 0 1 1 1 0 1
// 1 0 1 1 1 0 1 0 1 1 0 0 1 0 1 0 1 1 1 0 1
// 1 0 1 1 1 0 1 0 0 1 0 0 1 0 1 0 1 1 1 0 1
// 1 0 0 0 0 0 1 0 1 0 0 1 0 0 1 0 0 0 0 0 1
// 1 1 1 1 1 1 1 0 1 0 1 0 1 0 1 1 1 1 1 1 1 
// 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0
// 1 1 1 1 1 0 1 1 1 0 0 1 0 1 0 1 0 1 0 1 0
// 1 1 1 0 0 1 0 0 0 1 0 1 1 1 1 0 0 1 0 0 0
// 0 1 0 1 1 0 1 1 1 1 1 0 1 0 1 1 0 0 0 0 0
// 0 1 0 1 0 1 0 0 1 0 0 1 1 1 1 0 0 1 0 1 1
// 0 0 1 0 1 1 1 1 1 0 0 0 1 0 0 1 0 0 1 0 0 
// 0 0 0 0 0 0 0 0 1 1 1 0 1 0 0 1 0 0 1 1 1
// 1 1 1 1 1 1 1 0 1 1 1 1 0 1 0 0 1 1 1 0 0
// 1 0 0 0 0 0 1 0 0 0 1 0 0 0 0 1 1 0 1 1 0
// 1 0 1 1 1 0 1 0 1 0 1 1 0 1 0 0 1 1 1 0 0
// 1 0 1 1 1 0 1 0 1 0 1 1 1 1 1 0 0 1 0 0 0 
// 1 0 1 1 1 0 1 0 1 1 1 0 1 0 1 1 0 0 0 0 0
// 1 0 0 0 0 0 1 0 1 1 0 1 1 1 1 0 0 1 0 0 0
// 1 1 1 1 1 1 1 0 1 1 0 0 1 0 0 1 0 0 1 1 0
// */


/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK) & Gemini AI
 * @version     V4.0 (Adapted to official ALIENTEK BSP)
 * @date        2024-01-01
 * @brief       基于状态机的视觉识别与通信任务 (使用官方BSP)
 ****************************************************************************************************
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysctl.h"
#include "plic.h"
#include "iomem.h"
#include "image_process.h"

// 包含您提供的BSP头文件
#include "./BSP/LCD/lcd.h"
#include "./BSP/CAMERA/camera.h"
#include "./BSP/LED/led.h"
#include "./BSP/UART/uart.h"

#include "third_party/quirc.h"
#include "uart.h"

#define CAMERA_WIDTH    320
#define CAMERA_HEIGHT   240
#define QR_IMG_WIDTH    (CAMERA_WIDTH / 2)
#define QR_IMG_HEIGHT   (CAMERA_HEIGHT / 2)

int fast_identify_qr(const struct quirc *q, int index);
/**
 * @brief 初始化QR码解码器
 */
struct quirc *qrcode_decoder_init(void)
{
    struct quirc *qr = quirc_new();
    if (!qr) return NULL;
    if (quirc_resize(qr, QR_IMG_WIDTH, QR_IMG_HEIGHT) < 0)
    {
        quirc_destroy(qr);
        return NULL;
    }
    return qr;
}

/**
 * @brief 主函数
 */
int main(void)
{
    /* 系统初始化 */
    sysctl_pll_set_freq(SYSCTL_PLL0, 800000000);
    sysctl_pll_set_freq(SYSCTL_PLL1, 400000000);
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400);
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
    sysctl_set_spi0_dvp_data(1);
    plic_init();
    sysctl_enable_irq();
    /* 硬件初始化 (使用您提供的BSP函数) */
    led_init();
    usart_init(115200); // 初始化串口
    // uart_queue_init(UART_NUM); // 初始化串口数据包接收队列
    lcd_init();
    camera_init(0);
    camera_set_pixformat(PIXFORMAT_RGB565);
    camera_set_framesize(CAMERA_WIDTH, CAMERA_HEIGHT);
    camera_set_hmirror(1); ///
    camera_set_vflip(1);
    camera_set_light(1); // 开启补光灯

    /* 二维码解码器初始化 */
    struct quirc *qr = qrcode_decoder_init();
    if (qr == NULL) {
        printf("QRcode decoder init failed!\n");
        while(1);
    }

    uint8_t *disp_buf;
    static uint8_t gray_buf[CAMERA_WIDTH * CAMERA_HEIGHT];
    static uint8_t small_gray_buf[QR_IMG_WIDTH * QR_IMG_HEIGHT];
    
    // 状态机变量
    uint8_t work_mode = 0x01; // 0: 空闲, 1: 小球检测, 2: 二维码检测
    uint8_t last_mode = 0x01;
    // uart_packet_t rx_packet; // 用于存储接收到的数据包
    uint8_t data[1];
    int len = 0;
    uint8_t tmp = 0;
    /* 主循环 - 状态机 */
    while (1){
        /* 1. 轮询检查是否接收到完整、有效的数据包 */
        if ((len = uart_receive_data(UART_NUM, data, sizeof(data))) == 1) {
            if (data[0] == 0x01 || data[0] == 0x02) {
                work_mode = data[0];
            }
        }
        
        // // 2. 当模式发生切换时，更新LCD提示和LED状态
        // if (work_mode != last_mode)
        // {
        //     lcd_clear(BLACK);
        //     switch(work_mode)
        //     {
        //         case 1: // 小球模式
        //             LEDR(0); // 红灯亮
        //             LEDB(1); // 蓝灯灭
        //             uart_send_data(UART_NUM, (uint8_t *)&work_mode, 1); // 发送模式确认
        //             lcd_draw_string(10, 10, "MODE: Ball Detect", RED);
        //             break;
        //         case 2: // 二维码模式
        //             LEDR(1); // 红灯灭
        //             LEDB(0); // 蓝灯亮
        //             uart_send_data(UART_NUM, (uint8_t *)&work_mode, 1); // 发送模式确认
        //             lcd_draw_string(10, 10, "MODE: QR Code Detect", BLUE);
        //             break;
        //     }
        //     last_mode = work_mode;
        // }

        /* 3. 根据当前模式执行核心任务 */
        switch (work_mode)
        {
            case 1: // 小球检测模式
            {
                if (camera_snapshot(&disp_buf, NULL) == 0)
                {
                    uint8_t response = 0;
                    BallInfo red_ball = find_ball(disp_buf, BALL_RED);
                    if (red_ball.found)
                    {
                        response = 0x01;
                    }
                    else
                    {
                        BallInfo blue_ball = find_ball(disp_buf, BALL_BLUE);
                        if (blue_ball.found)
                        {
                            response = 0x02;
                        }
                    }

                    if (response != 0) {
                        // 简单的重试/等待（根据你的 UART 驱动语义调整）
                        for (int tries = 0; tries < 50; ++tries) {
                            if (uart_send_data(UART_NUM, &response, 1) == 1) break;
                            // 可适当 busy-wait / 小延时
                        }
                    }

                    lcd_draw_picture(0, 0, CAMERA_WIDTH, CAMERA_HEIGHT, (uint16_t *)disp_buf);
                    uint8_t t[4];
                    lcd_draw_string(10, 30, itoa(tmp, t, 10), RED);                    
                    camera_snapshot_release();
                }
                break;
            }

            case 2: // 二维码检测模式
            {
                if (camera_snapshot(&disp_buf, NULL) == 0)
                {
                    rgb565_to_gray(disp_buf, gray_buf, CAMERA_WIDTH, CAMERA_HEIGHT);
                    downscale_image(gray_buf, small_gray_buf, CAMERA_WIDTH, CAMERA_HEIGHT);

                    uint8_t *qr_image = quirc_begin(qr, NULL, NULL);
                    memcpy(qr_image, small_gray_buf, QR_IMG_WIDTH * QR_IMG_HEIGHT);
                    quirc_end(qr);

                    int num_codes = quirc_count(qr);
                    for (int i = 0; i < num_codes; i++)
                    {
                        int result = fast_identify_qr(qr, i);
                        if (result > 0)
                        {
                            uint8_t response = (uint8_t)result;
                            // 简单的重试/等待（根据你的 UART 驱动语义调整）
                            for (int tries = 0; tries < 50; ++tries) {
                                if (uart_send_data(UART_NUM, &response, 1) == 1) break;
                                // 可适当 busy-wait / 小延时
                            }
                        }
                    }
                    lcd_draw_picture(0, 0, CAMERA_WIDTH, CAMERA_HEIGHT, (uint16_t *)disp_buf);
                    uint8_t t[4];
                    lcd_draw_string(10, 30, itoa(tmp, t, 10), RED);                    
                    camera_snapshot_release();
                }
                break;
            }

            default: // 空闲模式
                // 不做任何事情，等待新指令
                break;
        }
    }
}



/*



*/