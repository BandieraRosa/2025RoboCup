/**
 ****************************************************************************************************
 * @file        led.c
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

#include "led.h"
#include "fpioa.h"
#include "gpio.h"

/**
 * @brief   初始化LED
 * @param   无
 * @retval  无
 */
void led_init(void)
{
    gpio_init();     /* 使能GPIO时钟 */
    
    fpioa_set_function(PIN_LED_R, FUNC_LEDR);
    fpioa_set_function(PIN_LED_B, FUNC_LEDB);
    
    /* 设置LEDR和LEDB的GPIO模式为输出 */
    gpio_set_drive_mode(LEDR_GPIONUM, GPIO_DM_OUTPUT);
    gpio_set_drive_mode(LEDB_GPIONUM, GPIO_DM_OUTPUT);
    
    /* 先关闭LEDR和LEDB */
    gpio_set_pin(LEDR_GPIONUM, GPIO_PV_HIGH);
    gpio_set_pin(LEDB_GPIONUM, GPIO_PV_HIGH);
}
