#include "uart.h"
#include "fpioa.h"

void uart_bsp_io_mux_init(void)
{
    fpioa_set_function(UART_RX_PIN, UART_FUNC_RX);
    fpioa_set_function(UART_TX_PIN, UART_FUNC_TX);
}

void usart_init(uint32_t baudrate)
{
    uart_bsp_io_mux_init();
    uart_init(UART_NUM);
    uart_configure(UART_NUM, baudrate, 8, UART_STOP_1, UART_PARITY_NONE);
}