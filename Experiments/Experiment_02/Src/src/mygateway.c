#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "config.h"
#include "comport.h"
#include "comport_receiver.h"
#include "mqtt_client.h"

#define BANDRATE 115200
#define COMPORT_NUMBER "/dev/ttyS1"
#define MQTT_PORT 1883
#define DEVICE_COUNT (sizeof(device_config) / sizeof(device_config[0]))

// 全局串口文件描述符
static int g_serial_fd = -1;

// 全局配置变量
static struct CONFIG config;

// 设备类型枚举
typedef enum
{
	TYPE_FLOAT,
	TYPE_BOOL
} DataType;

// 设备配置结构
typedef struct
{
	const char *name; // 既用作设备名, 也用作 JSON 键名
	DataType type;
	int writable; // 0: 只读, 1: 可写
} DeviceConfig;

// 设备配置表 - 利用数组索引作为 ID
static const DeviceConfig device_config[] = {
	{"temperature", TYPE_FLOAT, 0}, // ID 0 - 只读
	{"humidity", TYPE_FLOAT, 0},	// ID 1 - 只读
	{"illuminance", TYPE_FLOAT, 0}, // ID 2 - 只读
	{"door", TYPE_BOOL, 1},			// ID 3 - 可写
	{"light", TYPE_BOOL, 1},		// ID 4 - 可写
	{"ac_power", TYPE_BOOL, 1},		// ID 5 - 可写
	{"curtain", TYPE_FLOAT, 1}		// ID 6 - 可写
};

// 串口接收回调函数示例
void ComRxHandler(uint8_t device_id, uint8_t *data, uint8_t data_len)
{
	char topic[64], payload[64];

	// 参数验证
	if (device_id < 0 || device_id >= DEVICE_COUNT)
	{
		printf("Invalid device ID: %d (valid range: 0-%d)\n", device_id, DEVICE_COUNT - 1);
		return;
	}

	snprintf(topic, sizeof(topic), "%s/%s/%u/status", config.group_id, config.gateway_id, device_id);

	// 根据数据类型处理
	if (device_config[device_id].type == TYPE_FLOAT)
	{
		if (data_len == 4)
		{
			float value = *((float *)data);
			snprintf(payload, sizeof(payload), "{\"%s\":%.1f}",
					 device_config[device_id].name, value);
			MQTT_Publish(topic, payload);
		}
		else
		{
			printf("Device %s: expected 4 bytes for float, got %d\n",
				   device_config[device_id].name, data_len);
		}
	}
	else // TYPE_BOOL
	{
		if (data_len == 1)
		{
			snprintf(payload, sizeof(payload), "{\"%s\":%s}",
					 device_config[device_id].name,
					 *data ? "true" : "false");
			MQTT_Publish(topic, payload);
		}
		else
		{
			printf("Device %s: expected 1 byte for bool, got %d\n",
				   device_config[device_id].name, data_len);
		}
	}
}

// MQTT 消息回调函数
void MQTTMessageCallback(const char *topic, const char *message, int len)
{
	int device_id;
	uint8_t frame[10];
	int frame_len;
	uint8_t checksum;
	float value_float;
	uint8_t value_bool;
	char json_key[32];

	// 1. 从 topic 中解析 device_id: "groupx/gateway001/4/command" -> 4
	if (sscanf(topic, "%*[^/]/%*[^/]/%d/command", &device_id) != 1)
	{
		printf("Failed to parse device_id from topic: %s\n", topic);
		return;
	}

	// 2. 检查设备 ID 是否有效
	if (device_id < 0 || device_id >= DEVICE_COUNT)
	{
		printf("Invalid device_id: %d\n", device_id);
		return;
	}

	// 3. 检查是否可写
	if (device_config[device_id].writable == 0)
	{
		printf("Device %s is not writable\n", device_config[device_id].name);
		return;
	}

	// 4. 解析 JSON 获取设备名和值
	// 格式: {"device_name":value}
	// 例如: {"light":true} 或 {"curtain":50.0}
	if (sscanf(message, "{ \"%31[^\"]\" :", json_key) != 1)
	{
		printf("Failed to parse key from message: %.*s\n", len, message);
		return;
	}

	// 验证 JSON 中的设备名是否与 topic 中的 device_id 对应
	if (strcmp(json_key, device_config[device_id].name) != 0)
	{
		printf("Key mismatch: expected %s, got %s\n",
			   device_config[device_id].name, json_key);
		return;
	}

	// 根据设备类型解析值
	if (device_config[device_id].type == TYPE_BOOL)
	{
		// 解析布尔值
		if (strstr(message, "true"))
		{
			value_bool = 1;
		}
		else if (strstr(message, "false"))
		{
			value_bool = 0;
		}
		else
		{
			printf("Invalid boolean value in message: %.*s\n", len, message);
			return;
		}
	}
	else
	{
		// 解析浮点数
		if (sscanf(message, "{ \"%*[^\"]\" : %f", &value_float) != 1)
		{
			printf("Failed to parse float value from message: %.*s\n", len, message);
			return;
		}
	}

	// 5. 构建串口帧
	// 帧格式: [0x58][0x59][长度][设备 ID][数据...][校验和]
	frame[0] = 0x58; // 同步码 1
	frame[1] = 0x59; // 同步码 2

	if (device_config[device_id].type == TYPE_BOOL)
	{
		// 布尔值: 1 字节数据
		frame[2] = 2;							   // 长度 = 设备 ID(1) + 数据(1)
		frame[3] = device_id;					   // 设备 ID
		frame[4] = value_bool;					   // 数据
		frame[5] = frame[2] + frame[3] + frame[4]; // 校验和
		frame_len = 6;
	}
	else
	{
		// 浮点数: 4 字节数据
		frame[2] = 5;						// 长度 = 设备 ID(1) + 数据(4)
		frame[3] = device_id;				// 设备 ID
		memcpy(&frame[4], &value_float, 4); // 数据
		checksum = frame[2] + frame[3];
		int i;
		for (i = 0; i < 4; i++)
		{
			checksum += frame[4 + i];
		}
		frame[8] = checksum; // 校验和
		frame_len = 9;
	}

	// 6. 发送
	int ret = write(g_serial_fd, frame, frame_len);
	if (ret != frame_len)
	{
		printf("Write failed: expected %d bytes, wrote %d bytes\n", frame_len, ret);
	}
	else
	{
		printf("Sent command to device %d: %.*s\n", device_id, len, message);
	}
}

int main(void)
{
	// 读取配置并判断配置合理性
	int rtn = GetConfig(&config);

	if (rtn < 0) // 读取配置失败
	{
		printf("Package configuration error: %d\n", rtn);
		return 0;
	}

	// 以阻塞读和写的方式打开并设置串口
	if ((g_serial_fd = open(COMPORT_NUMBER, O_RDWR | O_NOCTTY)) < 0)
	{ // 打开窗口失败
		printf("Open %s failed\n", COMPORT_NUMBER);
		return 0;
	}

	SetComPort(g_serial_fd, 115200, 8, 'N', 1); // 设置成8N1 115200bps

	// 启动串口接收线程
	printf("Start comport receiving\n");
	ComReceiverStart(g_serial_fd, ComRxHandler);

	// 连接到公共MQTT代理
	if (MQTT_Connect(config.mqtt_broker, MQTT_PORT) != 0)
	{
		printf("MQTT connection fail\n");
		return -1;
	}

	// 订阅主题: GROUP_ID/GATEWAY_ID/+/command
	char subscribe_topic[64];
	snprintf(subscribe_topic, sizeof(subscribe_topic), "%s/%s/+/command", config.group_id, config.gateway_id);
	MQTT_Subscribe(subscribe_topic, MQTTMessageCallback);

	// 主循环 (MQTT 由后台线程处理)
	while (1)
	{
		usleep(100000); // 100ms 延迟, 避免 CPU 占用过高
	}

	// 停止接收线程 (永远不会执行到这里)
	ComReceiverStop();
	MQTT_Disconnect();
	return 0;
}
