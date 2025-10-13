/**
 ****************************************************************************************************
 * @file        uart_queue_util.c
 * @author      正点原子(ALIENTEK)
 * @version     V1.1
 * @date        2024-01-01
 * @brief       UART队列工具，用于数据包的接收和发送
 * @license     Copyright (c) 2020-2032, 正点原子(广州)科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 K210开发板
 * 数据包格式: 包头(0xAA) + 数据长度 + 数据内容 + 校验和
 *
 ****************************************************************************************************
 */

#include "uart_queue_util.h"
#include "plic.h"

/* 静态函数声明 */
static plic_irq_t uart_get_irq_number(uart_device_number_t uart_device);

/* 全局变量定义 */
static uart_rx_manager_t g_uart_rx_manager; /* UART接收管理器 */
static volatile bool g_packet_ready = false;      /* 数据包准备就绪标志 */
static uart_packet_t g_ready_packet;              /* 存储准备好的数据包 */


/**
 * @brief       获取UART设备对应的中断号
 * @param[in]   uart_device: UART设备号
 * @retval      plic_irq_t: PLIC中断号
 */
static plic_irq_t uart_get_irq_number(uart_device_number_t uart_device)
{
    switch (uart_device) {
        case UART_DEVICE_1:
            return IRQN_UART1_INTERRUPT;
        case UART_DEVICE_2:
            return IRQN_UART2_INTERRUPT;
        case UART_DEVICE_3:
            return IRQN_UART3_INTERRUPT;
        default:
            return IRQN_UART1_INTERRUPT; /* 默认为UART1中断 */
    }
}

/**
 * @brief       初始化UART队列相关功能
 * @param[in]   uart_device: 要初始化的UART设备号
 * @retval      无
 */
void uart_queue_init(uart_device_number_t uart_device)
{
    /* 记录UART设备号 */
    g_uart_rx_manager.uart_device = uart_device;
    
    /* 初始化环形缓冲区 */
    g_uart_rx_manager.ring_buffer.head = 0;
    g_uart_rx_manager.ring_buffer.tail = 0;
    g_uart_rx_manager.ring_buffer.count = 0;
    
    /* 初始化数据包解析状态机 */
    g_uart_rx_manager.state = PACKET_STATE_HEADER;
    g_uart_rx_manager.data_index = 0;
    g_uart_rx_manager.calculated_checksum = 0;
    
    /* 初始化当前数据包状态 */
    g_uart_rx_manager.current_packet.is_valid = false;
    
    g_packet_ready = false;
    
    /* 设置UART接收FIFO触发阈值为1个字节 */
    uart_set_receive_trigger(uart_device, UART_RECEIVE_FIFO_1);
    
    /* 注册接收中断服务函数 */
    uart_irq_register(uart_device, UART_RECEIVE, uart_queue_irq_handler, NULL, 1);
}

/**
 * @brief       UART接收中断服务函数
 * @param[in]   ctx: 中断上下文指针 (未使用)
 * @retval      0: 表示中断处理完成
 */
int uart_queue_irq_handler(void *ctx)
{
    uint8_t received_data;
    
    /* 使用while循环读取，确保一次性清空FIFO中的所有数据 */
    while (uart_receive_data(g_uart_rx_manager.uart_device, (char*)&received_data, 1) > 0) {
        /* 将接收到的数据放入环形缓冲区 */
        uart_queue_ring_buffer_put(received_data);
    }
    
    return 0; /* 返回0表示中断处理完成 */
}

/**
 * @brief       向环形缓冲区中放入一个字节
 * @param[in]   data: 要放入的数据
 * @retval      无
 */
void uart_queue_ring_buffer_put(uint8_t data)
{
    if (g_uart_rx_manager.ring_buffer.count < UART_RX_BUFFER_SIZE) {
        g_uart_rx_manager.ring_buffer.buffer[g_uart_rx_manager.ring_buffer.tail] = data;
        g_uart_rx_manager.ring_buffer.tail = (g_uart_rx_manager.ring_buffer.tail + 1) % UART_RX_BUFFER_SIZE;
        g_uart_rx_manager.ring_buffer.count++;
    }
    /* 如果缓冲区已满，则覆盖最旧的数据（丢弃旧数据） */
    else {
        g_uart_rx_manager.ring_buffer.buffer[g_uart_rx_manager.ring_buffer.tail] = data;
        g_uart_rx_manager.ring_buffer.tail = (g_uart_rx_manager.ring_buffer.tail + 1) % UART_RX_BUFFER_SIZE;
        g_uart_rx_manager.ring_buffer.head = (g_uart_rx_manager.ring_buffer.head + 1) % UART_RX_BUFFER_SIZE;
        /* count保持不变，因为同时添加了一个新数据并移除了一个旧数据 */
    }
}

/**
 * @brief       从环形缓冲区中获取一个字节
 * @param[out]  data: 用于存储获取到的数据的指针
 * @retval      true: 获取成功, false: 缓冲区为空
 */
bool uart_queue_ring_buffer_get(uint8_t *data)
{
    if (g_uart_rx_manager.ring_buffer.count > 0) {
        *data = g_uart_rx_manager.ring_buffer.buffer[g_uart_rx_manager.ring_buffer.head];
        g_uart_rx_manager.ring_buffer.head = (g_uart_rx_manager.ring_buffer.head + 1) % UART_RX_BUFFER_SIZE;
        g_uart_rx_manager.ring_buffer.count--;
        return true;
    }
    return false;
}

/**
 * @brief       计算校验和 (简单求和)
 * @param[in]   data: 数据缓冲区指针
 * @param[in]   length: 数据长度
 * @retval      计算出的8位校验和
 */
uint8_t uart_queue_calculate_checksum(uint8_t *data, uint8_t length)
{
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

/**
 * @brief       处理接收到的数据，解析数据包
 * @details     此函数应在主循环中被周期性调用，以处理环形缓冲区中的数据
 * @param       无
 * @retval      无
 */
void uart_queue_process_received_data(void)
{
    uint8_t data;
    
    while (uart_queue_ring_buffer_get(&data)) {
        switch (g_uart_rx_manager.state) {
            case PACKET_STATE_HEADER:
                if (data == UART_PACKET_HEADER) {
                    g_uart_rx_manager.current_packet.header = data;
                    g_uart_rx_manager.state = PACKET_STATE_LENGTH;
                    g_uart_rx_manager.calculated_checksum = data; /* 校验和从包头开始计算 */
                }
                break;
                
            case PACKET_STATE_LENGTH:
                if (data > 0 && data <= UART_MAX_PACKET_SIZE) {
                    g_uart_rx_manager.current_packet.length = data;
                    g_uart_rx_manager.data_index = 0;
                    g_uart_rx_manager.state = PACKET_STATE_DATA;
                    g_uart_rx_manager.calculated_checksum += data; /* 累加长度到校验和 */
                } else {
                    /* 无效的长度，重置状态机以寻找下一个包头 */
                    g_uart_rx_manager.state = PACKET_STATE_HEADER;
                }
                break;
                
            case PACKET_STATE_DATA:
                g_uart_rx_manager.current_packet.data[g_uart_rx_manager.data_index] = data;
                g_uart_rx_manager.calculated_checksum += data; /* 累加数据到校验和 */
                g_uart_rx_manager.data_index++;
                
                if (g_uart_rx_manager.data_index >= g_uart_rx_manager.current_packet.length) {
                    g_uart_rx_manager.state = PACKET_STATE_CHECKSUM; /* 数据接收完毕，等待校验和 */
                }
                break;
                
            case PACKET_STATE_CHECKSUM:
                g_uart_rx_manager.current_packet.checksum = data;
                
                /* 比较计算的校验和与接收到的校验和 */
                if (g_uart_rx_manager.calculated_checksum == data) {
                    g_uart_rx_manager.current_packet.is_valid = true;
                    /* 复制到全局就绪包，并设置标志位 */
                    g_ready_packet = g_uart_rx_manager.current_packet;
                    g_packet_ready = true;
                } else {
                    g_uart_rx_manager.current_packet.is_valid = false;
                }
                
                /* 无论校验是否成功，都重置状态机以准备接收下一个数据包 */
                g_uart_rx_manager.state = PACKET_STATE_HEADER;
                g_uart_rx_manager.data_index = 0;
                g_uart_rx_manager.calculated_checksum = 0;
                break;
        }
    }
}

/**
 * @brief       获取一个完整的数据包
 * @param[out]  packet: 用于存储数据包的结构体指针
 * @retval      true: 成功获取一个有效数据包, false: 没有可用的数据包
 */
bool uart_queue_get_packet(uart_packet_t *packet)
{
    /* 进入临界区：暂时禁用中断以保护共享数据 */
    plic_irq_disable(uart_get_irq_number(g_uart_rx_manager.uart_device));
    
    /* 处理缓冲区中的数据，尝试解析数据包 */
    uart_queue_process_received_data();
    
    if (g_packet_ready && g_ready_packet.is_valid) {
        *packet = g_ready_packet;
        g_packet_ready = false; /* 清除标志，表示数据包已被取走 */
        /* 退出临界区：重新使能中断 */
        plic_irq_enable(uart_get_irq_number(g_uart_rx_manager.uart_device));
        return true;
    }
    
    /* 退出临界区：重新使能中断 */
    plic_irq_enable(uart_get_irq_number(g_uart_rx_manager.uart_device));
    return false;
}

/**
 * @brief       复位接收队列和状态机
 * @param       无
 * @retval      无
 */
void uart_queue_reset(void)
{
    /* 进入临界区，防止在复位过程中被中断干扰 */
    plic_irq_disable(uart_get_irq_number(g_uart_rx_manager.uart_device));
    
    /* 清空环形缓冲区 */
    g_uart_rx_manager.ring_buffer.head = 0;
    g_uart_rx_manager.ring_buffer.tail = 0;
    g_uart_rx_manager.ring_buffer.count = 0;
    
    /* 复位状态机 */
    g_uart_rx_manager.state = PACKET_STATE_HEADER;
    g_uart_rx_manager.data_index = 0;
    g_uart_rx_manager.calculated_checksum = 0;
    
    /* 复位数据包状态 */
    g_uart_rx_manager.current_packet.is_valid = false;
    g_packet_ready = false;
    
    /* 退出临界区 */
    plic_irq_enable(uart_get_irq_number(g_uart_rx_manager.uart_device));
}

/**
 * @brief       获取环形缓冲区中的数据字节数
 * @param       无
 * @retval      缓冲区中当前未处理的字节数
 */
uint16_t uart_queue_get_buffer_count(void)
{
    /* 进入临界区，安全地读取count值 */
    plic_irq_disable(uart_get_irq_number(g_uart_rx_manager.uart_device));
    uint16_t count = g_uart_rx_manager.ring_buffer.count;
    plic_irq_enable(uart_get_irq_number(g_uart_rx_manager.uart_device));
    
    return count;
}

/**
 * @brief       检查环形缓冲区是否已满
 * @param       无
 * @retval      true: 缓冲区已满, false: 缓冲区未满
 */
bool uart_queue_is_buffer_full(void)
{
    return (g_uart_rx_manager.ring_buffer.count >= UART_RX_BUFFER_SIZE);
}

/**
 * @brief       构建数据包
 * @param[out]  packet: 用于存储构建好的数据包的结构体指针
 * @param[in]   data:   要发送的数据的指针
 * @param[in]   length: 数据长度
 * @retval      true: 构建成功, false: 构建失败 (参数无效)
 */
bool uart_queue_build_packet(uart_packet_t *packet, uint8_t *data, uint8_t length)
{
    /* 检查参数有效性 */
    if (packet == NULL || data == NULL || length == 0 || length > UART_MAX_PACKET_SIZE) {
        return false;
    }
    
    /* 构建数据包 */
    packet->header = UART_PACKET_HEADER;
    packet->length = length;
    
    /* Bug fix: 初始化校验和 */
    packet->checksum = 0;
    
    /* 复制数据并计算数据的校验和 */
    for (uint8_t i = 0; i < length; i++) {
        packet->data[i] = data[i];
        packet->checksum += packet->data[i];
    }
    
    /* 计算最终校验和：包头 + 长度 + 数据 */
    packet->checksum += packet->header + packet->length;
    
    packet->is_valid = true;
    return true;
}

/**
 * @brief       发送一个已构建好的数据包
 * @param[in]   packet: 要发送的数据包的指针
 * @retval      true: 发送成功, false: 发送失败 (参数无效或发送错误)
 */
bool uart_queue_send_packet(uart_packet_t *packet)
{
    /* 检查参数有效性 */
    if (packet == NULL || !packet->is_valid) {
        return false;
    }
    
    /* 发送包头 */
    if (uart_send_data(g_uart_rx_manager.uart_device, (char*)&packet->header, 1) != 1) {
        return false;
    }
    
    /* 发送长度 */
    if (uart_send_data(g_uart_rx_manager.uart_device, (char*)&packet->length, 1) != 1) {
        return false;
    }
    
    /* 发送数据 */
    if (packet->length > 0) {
        if (uart_send_data(g_uart_rx_manager.uart_device, (char*)packet->data, packet->length) != packet->length) {
            return false;
        }
    }
    
    /* 发送校验和 */
    if (uart_send_data(g_uart_rx_manager.uart_device, (char*)&packet->checksum, 1) != 1) {
        return false;
    }
    
    return true;
}

/**
 * @brief       发送数据 (自动构建数据包)
 * @param[in]   data: 要发送的数据的指针
 * @param[in]   length: 数据长度
 * @retval      true: 发送成功, false: 发送失败
 */
bool uart_queue_send_data(uint8_t *data, uint8_t length)
{
    uart_packet_t packet;
    
    /* 构建数据包 */
    if (!uart_queue_build_packet(&packet, data, length)) {
        return false;
    }
    
    /* 发送数据包 */
    return uart_queue_send_packet(&packet);
}
