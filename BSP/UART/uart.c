/*
 * @Author: BandieraRosa 3132716198@qq.com
 * @Date: 2025-07-12 23:58:02
 * @LastEditors: BandieraRosa 3132716198@qq.com
 * @LastEditTime: 2025-09-30 18:34:26
 * @FilePath: \kendryte-standalone-sdk\src\14_Camera\BSP\UART\uart.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
// /**
//  ****************************************************************************************************
//  * @file        uart.c
//  * @author      正点原子团队(ALIENTEK)
//  * @version     V1.0
//  * @date        2024-01-01
//  * @brief       UART 驱动代码
//  * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
//  ****************************************************************************************************
//  * @attention
//  * 
//  * 实验平台:正点原子 K210开发板
//  * 在线视频:www.yuanzige.com
//  * 技术论坛:www.openedv.com
//  * 公司网址:www.alientek.com
//  * 购买地址:openedv.taobao.com
//  *
//  ****************************************************************************************************
//  */

// #include "uart.h"
// #include "sysctl.h"
// #include "fpioa.h"
// #include "plic.h"
// #include "../../../../lib/drivers/include/uart.h"

// /**
//  * @brief   初始化USART
//  * @param   baudrate：波特率
//  * @retval  无
//  */
// void usart_init(uint32_t baudrate)
// {
//     fpioa_set_function(PIN_UART_2_RX, FUNC_UART_USB_RX);
//     fpioa_set_function(PIN_UART_2_TX, FUNC_UART_USB_TX);
//     /* 初始化串口号，设置波特率,8位数据格式，1个停止位，无奇偶校验位 */
//     uart_init(UART_USB_NUM);
//     uart_configure(UART_USB_NUM, baudrate, UART_BITWIDTH_8BIT, UART_STOP_1, UART_PARITY_NONE);
// }



#include "uart.h"  /* 本目录下的 BSP 头 */
#include "../../../../lib/drivers/include/uart.h"  /* 官方驱动头，路径按项目调整 */
#include "fpioa.h"

/* 仅做 UART2 的 IO 复用映射：IO8→RX，IO6→TX
   注：参考例程使用算术偏移（FUNC_UART1_RX + UART_NUM*2）这套写法，这里保持一致 */
void uart_bsp_io_mux_init(void)
{
    fpioa_set_function(UART_RX_PIN, UART_FUNC_RX);
    fpioa_set_function(UART_TX_PIN, UART_FUNC_TX);
}

/* 严格参考示例：先映射，再 init & configure；
   其它如 plic_init()/sysctl_enable_irq() 留给 main() 做（与示例一致） */
void usart_init(uint32_t baudrate)
{
    uart_bsp_io_mux_init();

    uart_init(UART_NUM);
    /* 参考例程里第三个参数直接用 8（数据位），保持一致；8N1，无校验 */
    uart_configure(UART_NUM, baudrate, 8, UART_STOP_1, UART_PARITY_NONE);
}

/* 简单便捷封装：发/收（轮询） */
int uart_bsp_send(const void *buf, size_t len)
{
    if (!buf || !len) return 0;
    uart_send_data(UART_NUM, (const char *)buf, (int)len);
    return (int)len;
}

int uart_bsp_recv(void *buf, size_t len)
{
    if (!buf || !len) return 0;
    return uart_receive_data(UART_NUM, (char *)buf, (int)len);
}
