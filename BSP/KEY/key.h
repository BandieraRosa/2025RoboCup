/**
 ****************************************************************************************************
 * @file        key.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-01-01
 * @brief       按键输入 驱动代码
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

#ifndef __KEY_H
#define __KEY_H
#include <stdio.h>
#include "gpiohs.h"

/******************************************************************************************/
/* 硬件IO口，与原理图对应 */
#define PIN_KEY_0              (18)
#define PIN_KEY_1              (19)
#define PIN_KEY_2              (16)
/*****************************SOFTWARE-GPIO********************************/
/* 软件GPIO口，与程序对应 */
#define KEY0_GPIONUM           (0)
#define KEY1_GPIONUM           (1)
#define KEY2_GPIONUM           (2)
/*****************************FUNC-GPIO************************************/
/* GPIO口的功能，绑定到硬件IO口 */
#define FUNC_KEY0              (FUNC_GPIOHS0 + KEY0_GPIONUM)
#define FUNC_KEY1              (FUNC_GPIOHS0 + KEY1_GPIONUM)
#define FUNC_KEY2              (FUNC_GPIOHS0 + KEY2_GPIONUM)
/******************************************************************************************/

#define KEY0         gpiohs_get_pin(KEY0_GPIONUM)      /* 读取KEY0引脚 */
#define KEY1         gpiohs_get_pin(KEY1_GPIONUM)      /* 读取KEY1引脚 */
#define KEY2         gpiohs_get_pin(KEY2_GPIONUM)      /* 读取KEY2引脚 */

#define KEY0_PRES    1              /* KEY0按下 */
#define KEY1_PRES    2              /* KEY1按下 */
#define KEY2_PRES    3              /* KEY2按下 */

void key_init(void);                /* 按键初始化函数 */
uint8_t key_scan(uint8_t mode);     /* 按键扫描函数 */

#endif


















