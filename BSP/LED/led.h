/**
 ****************************************************************************************************
 * @file        led.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-01-01
 * @brief       LED驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 K210开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#ifndef __LED_H
#define __LED_H

#include <stdio.h>
#include "gpio.h"

/*****************************HARDWARE-PIN*********************************/
/* 硬件IO口，与原理图对应 */
#define PIN_LED_B             (25)
#define PIN_LED_R             (24)

/*****************************SOFTWARE-GPIO********************************/
/* 软件GPIO口，与程序对应 */
#define LEDR_GPIONUM          (0)
#define LEDB_GPIONUM          (1)

/*****************************FUNC-GPIO************************************/
/* GPIO口的功能，绑定到硬件IO口 */
#define FUNC_LEDR             (FUNC_GPIO0 + LEDR_GPIONUM)
#define FUNC_LEDB             (FUNC_GPIO0 + LEDB_GPIONUM)

/* IO操作 */
#define LEDR(x)                 do { (x) ?                                     \
                                    gpio_set_pin(LEDR_GPIONUM, GPIO_PV_HIGH):  \
                                    gpio_set_pin(LEDR_GPIONUM, GPIO_PV_LOW);   \
                                } while (0)
#define LEDB(x)                 do { (x) ?                                     \
                                    gpio_set_pin(LEDB_GPIONUM, GPIO_PV_HIGH):  \
                                    gpio_set_pin(LEDB_GPIONUM, GPIO_PV_LOW);   \
                                } while (0)


/* 函数声明 */
void led_init(void);     /* 初始化LED */

#endif
