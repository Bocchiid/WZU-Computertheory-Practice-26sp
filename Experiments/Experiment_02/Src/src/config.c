#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <uci.h>
#include "config.h"

int GetValue(struct uci_context *ctx, char *key, char *value, int n)
{
	char strKey[100];
	struct uci_ptr ptr;
	snprintf(strKey, sizeof(strKey), "mygateway.parameter.%s", key); // 组成完整的选项名
	if (uci_lookup_ptr(ctx, &ptr, strKey, true) == UCI_OK)			 // 查找选项
	{
		strncpy(value, ptr.o->v.string, n - 1); // 复制选项的值
		return 1;
	}

	return 0;
}

int GetConfig(struct CONFIG *config)
{
	struct uci_context *ctx = uci_alloc_context(); // 获得UCI上下文环境对象

	if (!ctx) // 出错
	{
		printf("UCI alloc error\n");
		return -1;
	}

	char period[20];

	if (GetValue(ctx, "period", period, sizeof(period)))
	{
		config->period = atoi(period);
	}
	else
	{
		uci_free_context(ctx);
		return -2;
	}

	if (GetValue(ctx, "output", config->output, sizeof(config->output)) == 0)
	{
		uci_free_context(ctx);
		return -3;
	}

	if (GetValue(ctx, "mqtt_broker", config->mqtt_broker, sizeof(config->mqtt_broker)) == 0)
	{
		uci_free_context(ctx);
		return -4;
	}

	if (GetValue(ctx, "gateway_id", config->gateway_id, sizeof(config->gateway_id)) == 0)
	{
		uci_free_context(ctx);
		return -5;
	}

	if (GetValue(ctx, "group_id", config->group_id, sizeof(config->group_id)) == 0)
	{
		uci_free_context(ctx);
		return -6;
	}

	uci_free_context(ctx);

	return 1;
}
