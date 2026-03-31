#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdint.h>
/**
 * 消息到达时的回调函数类型
 * @param topic 主题 (字符串)
 * @param message 消息内容 (以 '\0' 结尾的字符串, 长度由 len 给出)
 * @param len 消息长度
 */
typedef void (*MQTT_MessageCallback)(const char *topic, const char *message, int len);

/**
 * 初始化并连接 MQTT 服务器
 * @param host MQTT 服务器地址, 如 "broker.emqx.io"
 * @param port MQTT 端口号, 如 1883
 * @return 0 成功, -1 失败
 */
int MQTT_Connect(const char *host, int port);

/**
 * 发布一条消息到指定主题
 * @param topic 主题
 * @param message 要发送的字符串 (自动以 '\0' 结尾)
 * @return 0 成功, -1 失败
 */
int MQTT_Publish(const char *topic, const char *message);

/**
 * 订阅一个主题, 并设置全局消息回调 (可多次订阅不同主题)
 * @param topic 要订阅的主题
 * @param cb 消息到达时调用的回调函数
 * @return 0 成功, -1 失败
 */
int MQTT_Subscribe(const char *topic, MQTT_MessageCallback cb);

/**
 * 断开与 MQTT 连接并释放所有资源
 */
void MQTT_Disconnect(void);

#endif // MQTT_CLIENT_H