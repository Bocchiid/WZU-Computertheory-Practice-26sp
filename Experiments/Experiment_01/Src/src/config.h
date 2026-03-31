struct CONFIG
{
	char output[100]; // 输出字符串
	unsigned int period; // 时间间隔，单位为毫秒
};

int GetConfig(struct CONFIG *config);
