/**
 ****************************************************************************************************
 * @file        camera.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-01-01
 * @brief       摄像头 驱动代码
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

#include "camera.h"
#include "fpioa.h"
#include "plic.h"
#include "iomem.h"
#include <stddef.h>
#include <string.h>
#ifdef CAMERA_SENSOR_OV2640
#include "ov2640.h"
#endif
#ifdef CAMERA_SENSOR_OV5640
#include "ov5640.h"
#endif
#ifdef CAMERA_SENSOR_GC0308
#include "gc0308.h"
#endif


/* Variable for sensor */
static const camera_sensor_t *sensor;
static const camera_sensor_t *sensors[] =
{
#ifdef CAMERA_SENSOR_OV2640
    &camera_ov2640,
#endif
#ifdef CAMERA_SENSOR_OV5640
    &camera_ov5640,
#endif
#ifdef CAMERA_SENSOR_GC0308
    &camera_gc0308,
#endif
};

/* Variable for framebuffer */
#if CAMERA_FRAMEBUFFER_NUM < 1
#error The number of framebuffer must be greater than 0
#endif
typedef struct camera_fb
{
    /* Base parameter */
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    pixformat_t pixformat;

    /* Pointer to memory */
    uint8_t *disp[CAMERA_FRAMEBUFFER_NUM];
    uint8_t *ai[CAMERA_FRAMEBUFFER_NUM];

    /* Index for access */
    volatile uint8_t read_index;
    volatile uint8_t write_index;

    /* Status */
    volatile uint8_t full;
    volatile uint8_t empty;
} camera_fb_t;
static camera_fb_t fb;

/**
 * @brief       DVP接口初始化
 * @param       reg_len   ：指令长度
 * @param       xclk_rate ：DVP时钟速率
 * @retval      无
 */
static void camera_dvp_lowlevel_init(uint8_t reg_len, uint32_t xclk_rate)
{
    /* Initialize connect pin */
    fpioa_set_function(CAMERA_SDA_PIN, FUNC_SCCB_SDA);
    fpioa_set_function(CAMERA_SCL_PIN, FUNC_SCCB_SCLK);
    fpioa_set_function(CAMERA_RST_PIN, FUNC_CMOS_RST);
    fpioa_set_function(CAMERA_VSYNC_PIN, FUNC_CMOS_VSYNC);
    fpioa_set_function(CAMERA_PWDN_PIN, FUNC_CMOS_PWDN);
    fpioa_set_function(CAMERA_HSYNC_PIN, FUNC_CMOS_HREF);
    fpioa_set_function(CAMERA_XCLK_PIN, FUNC_CMOS_XCLK);
    fpioa_set_function(CAMERA_PCLK_PIN, FUNC_CMOS_PCLK);

    /* Pre-initialize DVP */
    dvp_init(reg_len);
    dvp_set_xclk_rate(xclk_rate);
}

/**
 * @brief       摄像头传感器初始化
 * @param       xclk_rate ：DVP时钟速率
 * @retval      0, 成功  1，失败
 */
static int camera_sensor_init(uint32_t xclk_rate)
{
    uint8_t index;

    /* Set sensor to NULL by default */
    sensor = (camera_sensor_t *)NULL;

    /* Initialize each supported sensor until successful */
    for (index=0; index<(sizeof(sensors)/sizeof(sensors[0])); index++)
    {
        if (xclk_rate == 0)
        {
            xclk_rate = sensors[index]->xclk_rate;
        }
        camera_dvp_lowlevel_init(sensors[index]->reg_len, xclk_rate);

        if (sensors[index]->init() == 0)
        {
            sensor = sensors[index];
            return 0;
        }
    }

    return 1;
}

/**
 * @brief       重置摄像头
 * @param       无
 * @retval      无
 */
static void camera_fb_reset(void)
{
    uint8_t index;

    /* Reset base parameter */
    fb.width = 0;
    fb.height = 0;
    fb.bpp = 0;
    fb.pixformat = PIXFORMAT_INVLAID;

    /* Free memory */
    for (index=0; index<CAMERA_FRAMEBUFFER_NUM; index++)
    {
        if (fb.disp[index] != NULL)
        {
            iomem_free(fb.disp[index]);
            fb.disp[index] = NULL;
        }
        if (fb.ai[index] != NULL)
        {
            iomem_free(fb.ai[index]);
            fb.ai[index] = NULL;
        }
    }

    /* Reset access index */
    fb.read_index = 0;
    fb.write_index = 0;

    /* Reset status */
    fb.full = 0;
    fb.empty = 1;
}

/**
 * @brief       摄像头中断使能
 * @param       enable ：其他，使能  0，不使能
 * @retval      无
 */
static void camera_dvp_run(uint8_t enable)
{
    if (enable != 0)
    {
        /* Enable DVP interrupt */
        dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
        plic_irq_enable(IRQN_DVP_INTERRUPT);
    }
    else
    {
        /* Disable DVP interrupt */
        plic_irq_disable(IRQN_DVP_INTERRUPT);
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    }
}

/**
 * @brief       DVP中断回调函数
 * @param       ctx ：回调的参数
 * @retval      0：操作成功
 */
static int camera_dvp_irq_callback(void *ctx)
{
    if (dvp_get_interrupt(DVP_STS_FRAME_START))
    {
        /* Start DVP capture if framebuffer is not full */
        if (fb.full == 0)
        {
            dvp_start_convert();
        }

        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }
    else if (dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        /* Clean framebuffer empty flag, cause DVP capture is finished */
        fb.empty = 0;

        /* Update framebuffer write index for next DVP capture */
        fb.write_index++;
        if (fb.write_index == CAMERA_FRAMEBUFFER_NUM)
        {
            fb.write_index = 0;
        }

        /* Check framebuffer is full or not */
        if (fb.write_index == fb.read_index)
        {
            /* 
             * If write index is increased to equal read index,
             * it means framebuffer is full
             */
            fb.full = 1;
        }
        else
        {
            /* 
             * Set next capture's framebuffer only when framebuffer is not full,
             * otherwise do it after the first time framebuffer read release after this full status
             */
            dvp_set_ai_addr((uint32_t)fb.ai[fb.write_index] + (0 * fb.width * fb.height),
                            (uint32_t)fb.ai[fb.write_index] + (1 * fb.width * fb.height),
                            (uint32_t)fb.ai[fb.write_index] + (2 * fb.width * fb.height));
            dvp_set_display_addr((uint32_t)fb.disp[fb.write_index]);
        }

        dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
    }
    else
    {
        /* Impossible situation */
    }

    return 0;
}

/**
 * @brief       DVP配置初始化
 * @param       无
 * @retval      无
 */
static void camera_dvp_init(void)
{
    /* Configure DVP base parameter */
    dvp_set_output_enable(DVP_OUTPUT_AI, 1);
    dvp_set_output_enable(DVP_OUTPUT_DISPLAY, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_image_size(fb.width, fb.height);
    dvp_set_ai_addr((uint32_t)fb.ai[fb.write_index] + (0 * fb.width * fb.height),
                    (uint32_t)fb.ai[fb.write_index] + (1 * fb.width * fb.height),
                    (uint32_t)fb.ai[fb.write_index] + (2 * fb.width * fb.height));
    dvp_set_display_addr((uint32_t)fb.disp[fb.write_index]);
    dvp_disable_burst();
    dvp_disable_auto();

    /* Configure interrupt */
    plic_irq_disable(IRQN_DVP_INTERRUPT);
    plic_set_priority(IRQN_DVP_INTERRUPT, 1);
    plic_irq_register(IRQN_DVP_INTERRUPT, camera_dvp_irq_callback, NULL);
}

/**
 * @brief       摄像头初始化
 * @param       xclk_rate ：DVP时钟速率
 * @retval      0，操作成功
 */
int camera_init(uint32_t xclk_rate)
{
    /* Reset framebuffer */
    camera_fb_reset();

    /* Automatically detect and initialize sensor */
    if (camera_sensor_init(xclk_rate) != 0)
    {
        return 1;
    }

    /* Configure DVP and disable DVP capture */
    camera_dvp_init();
    camera_dvp_run(0);

    return 0;
}

/**
 * @brief       设置摄像头输出的图像像素格式
 * @param       format : 像素格式枚举值
 * @retval      0, 操作成功; 1, 失败
 */
int camera_set_pixformat(pixformat_t format)
{
    if (format == PIXFORMAT_INVLAID)
    {
        return 1; // 无效格式
    }

    /* 如果新的格式与当前格式不同，则需要重新配置 */
    if (fb.pixformat != format)
    {
        // 调用具体传感器的函数来设置像素格式
        if (sensor->set_pixformat(format) != 0)
        {
            return 1; // 传感器设置失败
        }

        /* 记录新的格式参数 */
        fb.pixformat = format;
        switch (fb.pixformat)
        {
            case PIXFORMAT_RGB565:
            {
                fb.bpp = 2; // RGB565每个像素占2字节
                break;
            }
            default:
            {
                // 其他格式可以在此添加
                break;
            }
        }
    }

    return 0;
}

/**
 * @brief       设置图像分辨率大小
 * @param       width  : 宽度
 * @param       height : 高度
 * @retval      0, 设置成功; 1, 设置失败
 */
int camera_set_framesize(uint16_t width, uint16_t height)
{
    uint8_t index;
    uint8_t rec_index;

    /* 调用具体传感器的函数来设置分辨率 */
    if (sensor->set_framesize(width, height) != 0)
    {
        return 1; // 传感器设置失败
    }

    /* 如果分辨率发生变化，需要重新分配所有帧缓冲区的内存 */
    if ((fb.width != width) || (fb.height != height))
    {
        for (index=0; index<CAMERA_FRAMEBUFFER_NUM; index++)
        {
            /* 释放当前缓冲区的内存 */
            if (fb.disp[index] != NULL)
            {
                iomem_free(fb.disp[index]);
                fb.disp[index] = NULL;
            }
            if (fb.ai[index] != NULL)
            {
                iomem_free(fb.ai[index]);
                fb.ai[index] = NULL;
            }

            /* 为显示缓冲区分配新内存 */
            fb.disp[index] = (uint8_t *)iomem_malloc(width * height * fb.bpp);
            if (fb.disp[index] == NULL)
            {
                // 内存分配失败，需要回滚操作：释放已分配的，并尝试恢复到旧的设置
                for (rec_index=0; rec_index<index; rec_index++)
                {
                    iomem_free(fb.disp[rec_index]);
                    fb.disp[rec_index] = NULL;
                    iomem_free(fb.ai[rec_index]);
                    fb.ai[rec_index] = NULL;
                }
                // 尝试将传感器恢复到旧的分辨率
                sensor->set_framesize(fb.width, fb.height);
                return 1; // 返回失败
            }

            /* 为AI缓冲区分配新内存 (RGB888, 3字节/像素) */
            fb.ai[index] = (uint8_t *)iomem_malloc(width * height * 3);
            if (fb.ai[index] == NULL)
            {
                // 内存分配失败，同样需要回滚
                for (rec_index=0; rec_index<index; rec_index++)
                {
                    iomem_free(fb.disp[rec_index]);
                    fb.disp[rec_index] = NULL;
                    iomem_free(fb.ai[rec_index]);
                    fb.ai[rec_index] = NULL;
                }
                iomem_free(fb.disp[index]); // 释放刚刚分配成功的disp内存
                fb.disp[index] = NULL;
                sensor->set_framesize(fb.width, fb.height);
                return 1; // 返回失败
            }
        }

        /* 记录新的分辨率参数 */
        fb.width = width;
        fb.height = height;
    }

    /* 重新配置DVP的图像尺寸和内存地址 */
    dvp_set_image_size(width, height);
    dvp_set_ai_addr((uint32_t)fb.ai[fb.write_index] + (0 * fb.width * fb.height),
                    (uint32_t)fb.ai[fb.write_index] + (1 * fb.width * fb.height),
                    (uint32_t)fb.ai[fb.write_index] + (2 * fb.width * fb.height));
    dvp_set_display_addr((uint32_t)fb.disp[fb.write_index]);

    /* 所有配置完成，启动DVP采集 */
    camera_dvp_run(1);

    return 0;
}

/**
 * @brief       设置图像水平翻转
 * @param       enable  : 1, 翻转; 0, 不翻转
 * @retval      0, 成功; 其他, 失败 (取决于传感器驱动的返回值)
 */
int camera_set_hmirror(uint8_t enable)
{
    // 直接调用传感器驱动的对应函数
    return sensor->set_hmirror(enable);
}

/**
 * @brief       设置图像垂直翻转
 * @param       enable  : 1, 翻转; 0, 不翻转
 * @retval      0, 成功; 其他, 失败 (取决于传感器驱动的返回值)
 */
int camera_set_vflip(uint8_t enable)
{
    // 直接调用传感器驱动的对应函数
    return sensor->set_vflip(enable);
}

/**
 * @brief       设置补光灯开关
 * @param       enable  : 1, 开启补光灯; 0, 关闭补光灯
 * @retval      0, 成功; 其他, 失败 (取决于传感器驱动的返回值)
 */
int camera_set_light(uint8_t enable)
{
    // 直接调用传感器驱动的对应函数
    return sensor->set_light(enable);
}



/**
 * @brief       获取一帧图像的指针（快照）
 * @param       display : 用于接收RGB565数据指针的地址
 * @param       ai      : 用于接收RGB888数据指针的地址
 * @retval      0, 捕获成功; 1, 捕获失败 (缓冲区为空)
 * @note        这是一个零拷贝(Zero-copy)操作，只返回指针，不复制数据。
 */
int camera_snapshot(uint8_t **display, uint8_t **ai)
{
    /* 如果缓冲区为空，没有新图像可读，返回失败 */
    if (fb.empty == 1)
    {
        return 1;
    }

    /* 获取数据指针 */
    if (display != NULL)
    {
        // K210 DVP输出的RGB565数据大小端可能与LCD不匹配，需要进行字节序转换
        if (fb.pixformat == PIXFORMAT_RGB565)
        {
            uint32_t index;
            uint32_t size = fb.width * fb.height * fb.bpp;
            uint32_t loop = size >> 2; // 每次处理4字节(2个像素)
            uint32_t *data = (uint32_t *)fb.disp[fb.read_index];
            for (index=0; index<loop; index++)
            {
                // 将 0xAABBCCDD 转换为 0xCCDDABBB
                // 即将两个像素(P1, P2)的字节序(B1B2, B3B4) 交换为 (B3B4, B1B2)
                data[index] = ((data[index] & 0x0000FFFF) << 16) | ((data[index] & 0xFFFF0000) >> 16);
            }
        }
        *display = fb.disp[fb.read_index];
    }
    if (ai != NULL)
    {
        // AI数据通常不需要转换，直接返回指针
        *ai = fb.ai[fb.read_index];
    }

    return 0;
}

/**
 * @brief       释放当前已读取的图像缓冲区
 * @param       无
 * @retval      0, 成功; 1, 失败 (缓冲区为空)
 * @note        应用在处理完一帧图像后，必须调用此函数来释放缓冲区，
 *              否则环形缓冲区会变满，导致摄像头停止采集。
 */
int camera_snapshot_release(void)
{
    /* 如果缓冲区为空，没有可释放的，返回失败 */
    if (fb.empty == 1)
    {
        return 1;
    }

    /* 更新读索引，指向下一个可读的缓冲区 */
    fb.read_index++;
    if (fb.read_index == CAMERA_FRAMEBUFFER_NUM)
    {
        fb.read_index = 0; // 环形处理
    }

    /* 检查缓冲区是否变空 */
    if (fb.read_index == fb.write_index)
    {
        /* 读索引追上了写索引，表示所有新数据都已被读取，
         * 缓冲区变为空。
         */
        fb.empty = 1;
    }

    /* 如果之前缓冲区是满的，现在释放了一个，就需要做两件事：
     * 1. 清除满标志位
     * 2. 重新配置DVP的地址，让它开始向刚刚被释放的那个缓冲区（现在是新的写缓冲区）写入数据
     */
    if (fb.full == 1)
    {
        dvp_set_ai_addr((uint32_t)fb.ai[fb.write_index] + (0 * fb.width * fb.height),
                        (uint32_t)fb.ai[fb.write_index] + (1 * fb.width * fb.height),
                        (uint32_t)fb.ai[fb.write_index] + (2 * fb.width * fb.height));
        dvp_set_display_addr((uint32_t)fb.disp[fb.write_index]);
        fb.full = 0;
    }

    return 0;
}

/**
 * @brief       复制一帧图像数据到用户指定的内存地址
 * @param       display : 存放RGB565数据的目标地址
 * @param       ai      : 存放RGB888数据的目标地址
 * @retval      0, 成功; 1, 失败 (缓冲区为空)
 * @note        此函数会进行内存拷贝，然后自动释放缓冲区。
 */
int camera_snapshot_copy(uint8_t *display, uint8_t *ai)
{
    /* 如果缓冲区为空，没有可复制的数据，返回失败 */
    if (fb.empty == 1)
    {
        return 1;
    }

    /* 复制数据 */
    if (display != NULL)
    {
        // 注意：这里没有执行camera_snapshot中的字节序转换，如果需要，应先调用snapshot获取转换后的指针再拷贝
        memcpy(display, fb.disp[fb.read_index], fb.width * fb.height * fb.bpp);
    }
    if (ai != NULL)
    {
        memcpy(ai, fb.ai[fb.read_index], fb.width * fb.height * 3);
    }

    /* 复制完成后，释放该帧缓冲区 */
    camera_snapshot_release();

    return 0;
}