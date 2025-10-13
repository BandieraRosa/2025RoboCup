#ifndef __BSP_UART_H__
#define __BSP_UART_H__

#include <stdint.h>
#include <stddef.h>
#include "fpioa.h"

#include "../../../../lib/drivers/include/uart.h"

#define UART_NUM        UART_DEVICE_2   /* 即示例中的 UART_DEVICE_2 */
#define UART_RX_PIN     8               /* IO8  */
#define UART_TX_PIN     6               /* IO6  */

#define UART_FUNC_RX    (FUNC_UART1_RX + UART_NUM * 2)
#define UART_FUNC_TX    (FUNC_UART1_TX + UART_NUM * 2)

void uart_bsp_io_mux_init(void);

void usart_init(uint32_t baudrate);

#endif /* __BSP_UART_H__ */
