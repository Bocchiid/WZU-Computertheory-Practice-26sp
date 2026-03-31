#ifndef COMPORT_RECEIVER_H
#define COMPORT_RECEIVER_H

#include <stdint.h>

// 回调函数类型定义 - 当接收到完整一帧数据时调用
// device_id: 设备标识
// data: 数据缓冲区指针
// data_len: 数据字节数
typedef void (*ComRxCallback)(uint8_t device_id, uint8_t *data, uint8_t data_len);

// 启动串口接收线程
// fd: 串口文件描述符
// callback: 接收完整帧后的回调函数
// 返回值: 0 成功, -1 失败
int ComReceiverStart(int fd, ComRxCallback callback);

// 停止串口接收线程
// 返回值: 0 成功, -1 失败
int ComReceiverStop(void);

#endif // COMPORT_RECEIVER_H