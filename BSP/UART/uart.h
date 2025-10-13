// #ifndef __UART_H
// #define __UART_H

// #include <stdio.h>
// #include "fpioa.h"
// #include "uart.h"   // Kendryte 标准 UART0/1/2

// /* ====== USB 调试口（可选）：留给板载 UARTHS 的 IO4/IO5，不在本文件初始化 ====== */
// #define PIN_UART_USB_RX   (4)   // 板载 USB-转串口接的是 UARTHS，不是 UART2
// #define PIN_UART_USB_TX   (5)

// /*软件gpio,与程序对应*/
// #define UART_USB_NUM     UART_DEVICE_2  // USB 调试口使用 UART2

// #define FUNC_UART_USB_RX   (FUNC_UART1_RX + UART_USB_NUM * 2)
// #define FUNC_UART_USB_TX   (FUNC_UART1_TX + UART_USB_NUM * 2)

// /*****************************HARDWARE-PIN*********************************/
// /* 硬件IO口，与原理图对应（这里指 K210 的 IO 号，而非物理座子编号） */
// #define PIN_UART2_RX           (8)   /* U2_RX -> IO8  (物理引脚号 36) */
// #define PIN_UART2_TX           (6)   /* U2_TX -> IO6  (物理引脚号 38) */

// /*****************************SOFTWARE-GPIO********************************/
// /* 选择 UART 控制器编号 */
// #define UART_CH_NUM            UART_DEVICE_1   /* 即“串口2” */

// /*****************************FUNC-GPIO************************************/
// /* GPIO 复用到 UART 功能：每个 UART 有 RX/TX 两个功能位，因此 + UART_CH_NUM*2 */
// #define FUNC_UART2_RX          (FUNC_UART1_RX + UART_CH_NUM * 2)
// #define FUNC_UART2_TX          (FUNC_UART1_TX + UART_CH_NUM * 2)



// /* API */
// void usart_init(uint32_t baudrate);   /* 初始化业务口 UART2 */

// #endif

// #ifndef __UART_BSP_H
// #define __UART_BSP_H

// #include "fpioa.h"
// #include "../../../../lib/drivers/include/uart.h"

// #define UART_CH_NUM      UART_DEVICE_2

// #define PIN_UART_RX      (8)
// #define PIN_UART_TX      (6)

// #define FUNC_UART_RX     FUNC_UART2_RX
// #define FUNC_UART_TX     FUNC_UART2_TX

// void usart_init(uint32_t baudrate);

// #endif

#ifndef __BSP_UART_H__
#define __BSP_UART_H__

#include <stdint.h>
#include <stddef.h>
#include "fpioa.h"

/* 明确引用官方 UART 驱动头：请按你的工程实际路径校正 */
#include "../../../../lib/drivers/include/uart.h"

/* ====== 严格参考示例：固定 UART2，固定 IO 映射方式 ====== */
#define UART_NUM        UART_DEVICE_2   /* 即示例中的 UART_DEVICE_2 */
#define UART_RX_PIN     8               /* IO8  */
#define UART_TX_PIN     6               /* IO6  */

/* 参考示例里用“算术偏移”的方式得出功能枚举 */
#define UART_FUNC_RX    (FUNC_UART1_RX + UART_NUM * 2)
#define UART_FUNC_TX    (FUNC_UART1_TX + UART_NUM * 2)

/* 只做 UART 的 IO 映射（LED/GPIOHS 的 24→GPIOHS3 请留在应用层按需配置） */
void uart_bsp_io_mux_init(void);

/* 严格按参考例程的初始化/配置流程（波特率参数可传入） */
void usart_init(uint32_t baudrate);

/* 便捷封装（非必须） */
int  uart_bsp_send(const void *buf, size_t len);
int  uart_bsp_recv(void *buf, size_t len);

#endif /* __BSP_UART_H__ */
