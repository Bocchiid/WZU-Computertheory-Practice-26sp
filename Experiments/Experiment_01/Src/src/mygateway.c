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

#define BANDRATE 115200
#define COMPORT_NUMBER "/dev/ttyS1"

int main(void)
{
	int fd;
	struct CONFIG config;

	// 读取配置并判断配置合理性
	int rtn = GetConfig(&config);

	if (rtn < 0) // 读取配置失败
	{
		printf("Package configuration error: %d\n", rtn);
		return 0;
	}

	// 判断发送间隔是否合理, 后面的条件以1个自己的16位算 (留余量) 判断发送时间
	if ((config.period <= 0) || (16000 * rtn / BANDRATE >= config.period))
	{
		printf("Period error: %d ms\n", config.period);
		return 0;
	}

	if (rtn == 0) // 发送消息为空
	{
		printf("No message to be send!\n");
		return 0;
	}

	// 以阻塞读和写的方式打开并设置串口
	if ((fd = open(COMPORT_NUMBER, O_RDWR | O_NOCTTY)) < 0)
	{
		// 打开窗口失败
		printf("Open %s failed\n", COMPORT_NUMBER);
		return 0;
	}

	SetComPort(fd, 115200, 8, 'N', 1); // 设置成8N1 115200bps
	// 循环发送数据
	char tail[] = "\r\n";
	while (1)
	{
		write(fd, config.output, strlen(config.output) + 1);
		write(fd, tail, 2);
		usleep(1000 * config.period);
	}

	return 0;
}
