struct CONFIG
{
	char output[100];	  // 输出字符串
	unsigned int period;  // 时间间隔，单位为毫秒
	char mqtt_broker[64]; // MQTT 代理IP地址
	char gateway_id[64];  // 网关 ID
	char group_id[64];	  // 组 ID
};

int GetConfig(struct CONFIG *config);
