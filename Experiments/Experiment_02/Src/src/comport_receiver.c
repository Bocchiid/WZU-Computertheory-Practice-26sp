#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include "comport_receiver.h"

// 同步码
#define SYNC_BYTE_1 0x58
#define SYNC_BYTE_2 0x59

// 最大数据长度
#define MAX_DATA_LEN 255

// 接收状态机状态
typedef enum
{
    STATE_SYNC1 = 0, // 等待同步码 1 (0x58)
    STATE_SYNC2,     // 等待同步码 2 (0x59)
    STATE_LENGTH,    // 等待长度字节
    STATE_DEVICE_ID, // 等待设备标识
    STATE_DATA,      // 等待数据
    STATE_CHECKSUM   // 等待校验和码
} RxState;

// 接收帧结构
typedef struct
{
    uint8_t length;             // 长度字节 (设备标识 + 数据)
    uint8_t device_id;          // 设备标识
    uint8_t data[MAX_DATA_LEN]; // 数据缓冲区
} RxFrame;

// 接收器控制结构
typedef struct
{
    int fd;                 // 串口文件描述符
    ComRxCallback callback; // 回调函数
    pthread_t thread_id;    // 接收线程 ID
    int running;            // 运行标志
} ComReceiver;

// 全局接收器实例
static ComReceiver g_receiver;

// 接收线程函数
static void *ComReceiverThread(void *arg)
{
    (void)arg;

    uint8_t data;
    RxState state = STATE_SYNC1;
    RxFrame frame;
    uint8_t checksum = 0;
    uint8_t data_idx = 0; // 数据字节索引

    while (g_receiver.running)
    {
        // 读取一个字节 (阻塞模式, 最多等待 100ms)
        int n = read(g_receiver.fd, &data, 1);

        if (n <= 0)
        {
            usleep(1000); // 1ms 延迟, 避免 CPU 占用过高
            continue;
        }

        switch (state)
        {
        case STATE_SYNC1:
            if (data == SYNC_BYTE_1)
            {
                state = STATE_SYNC2;
            }
            break;

        case STATE_SYNC2:
            if (data == SYNC_BYTE_2)
            {
                state = STATE_LENGTH;
                checksum = 0;
                data_idx = 0;
            }
            else
            {
                state = STATE_SYNC1;
            }
            break;

        case STATE_LENGTH:
            frame.length = data;
            checksum += data;
            // 长度 = 设备标识 (1) + 数据 (n), 所以长度至少为1
            if (frame.length >= 1 && frame.length <= (MAX_DATA_LEN + 1))
            {
                state = STATE_DEVICE_ID;
            }
            else
            {
                state = STATE_SYNC1;
            }
            break;

        case STATE_DEVICE_ID:
            frame.device_id = data;
            checksum += data;
            data_idx = 0;
            // 如果长度只有1, 说明没用数据部分, 直接跳到校验码
            if (frame.length == 1)
            {
                state = STATE_CHECKSUM;
            }
            else
            {
                state = STATE_DATA;
            }
            break;

        case STATE_DATA:
            frame.data[data_idx] = data;
            checksum += data;
            data_idx++;
            // 检查是否已经接收完所有数据
            if (data_idx >= frame.length - 1)
            {
                state = STATE_CHECKSUM;
            }
            break;

        case STATE_CHECKSUM:
            if (checksum == data)
            {
                // 校验成功, 调用回调函数
                if (g_receiver.callback)
                {
                    g_receiver.callback(frame.device_id, frame.data, frame.length - 1);
                }
            }
            state = STATE_SYNC1;
            break;
        }
    }

    return NULL;
}

int ComReceiverStart(int fd, ComRxCallback callback)
{
    // 初始化接收器
    g_receiver.fd = fd;
    g_receiver.callback = callback;
    g_receiver.running = 1;

    // 创建接收线程
    int ret = pthread_create(&g_receiver.thread_id, NULL, ComReceiverThread, NULL);
    if (ret != 0)
    {
        g_receiver.running = 0;
        return -1;
    }

    return 0;
}

int ComReceiverStop(void)
{
    if (!g_receiver.running)
    {
        return -1;
    }

    g_receiver.running = 0;
    pthread_join(g_receiver.thread_id, NULL);

    return 0;
}