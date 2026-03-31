#include "mqtt_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <mosquitto.h>

static struct mosquitto *g_mosq = NULL;             // MQTT 客户端实例
static int g_connected = 0;                         // 连接状态
static MQTT_MessageCallback g_user_callback = NULL; // 用户消息回调

// 生成一个随机客户端 ID, 格式: "student_12345_6789"
static void generate_client_id(char *buf, size_t len)
{
    unsigned int pid = (unsigned int)getpid();
    unsigned int rand_num;
    // 使用时间和 PID 作为随机种子
    srand((unsigned int)time(NULL) ^ (pid << 16));
    rand_num = (unsigned int)rand() % 10000;
    snprintf(buf, len, "student_%u_%04u", pid, rand_num);
}

// 连接成功回调
static void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    (void)mosq;
    (void)obj;

    if (rc == 0)
    {
        g_connected = 1;
        printf("? MQTT connected.\n");
    }
    else
    {
        printf("× MQTT connection failed: %s\n", mosquitto_connack_string(rc));
    }
}

// 断开连接回调
static void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
    (void)mosq;
    (void)obj;
    (void)rc;

    g_connected = 0;
    printf("? MQTT disconnected.\n");
}

// 消息到达回调 (内部处理, 转换为 C 字符串后调用用户回调)
static void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    (void)mosq;
    (void)obj;

    if (g_user_callback && msg->payloadlen > 0)
    {
        // 为消息末尾添加 '\0', 方便作为字符串使用
        char *text = (char *)malloc(msg->payloadlen + 1);
        if (text)
        {
            memcpy(text, msg->payload, msg->payloadlen);
            text[msg->payloadlen] = '\0';
            g_user_callback(msg->topic, text, msg->payloadlen);
            free(text);
        }
    }
}

int MQTT_Connect(const char *host, int port)
{
    if (g_mosq)
    {
        fprintf(stderr, "Already connected.\n");
        return -1;
    }

    // 初始化 libmosquitto
    mosquitto_lib_init();

    // 生成随机客户端 ID
    char client_id[64];
    generate_client_id(client_id, sizeof(client_id));

    // 创建客户端实例 (clean_session = true)
    g_mosq = mosquitto_new(client_id, true, NULL);

    if (!g_mosq)
    {
        fprintf(stderr, "Failed to create mosquitto client.\n");
        mosquitto_lib_cleanup();
        return -1;
    }

    // 设置回调函数
    mosquitto_connect_callback_set(g_mosq, on_connect);
    mosquitto_disconnect_callback_set(g_mosq, on_disconnect);
    mosquitto_message_callback_set(g_mosq, on_message);

    // 启用自动重连 (断线后自动尝试重连)
    mosquitto_reconnect_delay_set(g_mosq, 1, 10, true);

    // 连接服务器 (非阻塞, 后台线程会完成真正的连接)
    int rc = mosquitto_connect(g_mosq, host, port, 60);

    if (rc != 0)
    {
        fprintf(stderr, "Connect failed: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
        mosquitto_lib_cleanup();
        return -1;
    }

    // 启动后台网络线程 (自动处理收发和重连)
    rc = mosquitto_loop_start(g_mosq);

    if (rc != 0)
    {
        fprintf(stderr, "Loop start failed: %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(g_mosq);
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
        mosquitto_lib_cleanup();
        return -1;
    }

    return 0;
}

int MQTT_Publish(const char *topic, const char *message)
{
    if (!g_mosq)
    {
        fprintf(stderr, "MQTT not connected.\n");
        return -1;
    }

    int len = (int)strlen(message);
    int mid;
    int rc = mosquitto_publish(g_mosq, &mid, topic, len, message, 0, false);

    if (rc != 0)
    {
        fprintf(stderr, "Publish failed: %s\n", mosquitto_strerror(rc));
        return -1;
    }

    return 0;
}

int MQTT_Subscribe(const char *topic, MQTT_MessageCallback cb)
{
    if (!g_mosq)
    {
        fprintf(stderr, "MQTT not connected.\n");
        return -1;
    }

    if (cb)
    {
        g_user_callback = cb; // 记录用户回调 (最后一次设置生效)
    }

    int mid;
    int rc = mosquitto_subscribe(g_mosq, &mid, topic, 0); // QoS 0

    if (rc != 0)
    {
        fprintf(stderr, "Subscribe failed: %s\n", mosquitto_strerror(rc));
        return -1;
    }

    return 0;
}

void MQTT_Disconnect(void)
{
    if (g_mosq)
    {
        mosquitto_loop_stop(g_mosq, true); // 等待后台线程结束
        mosquitto_disconnect(g_mosq);
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
        g_connected = 0;
        g_user_callback = NULL;
    }

    mosquitto_lib_cleanup();
}