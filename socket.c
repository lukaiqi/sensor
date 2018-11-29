#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#define DEST_PORT 80
#define DEST_IP_ADDR "47.106.69.171"
#define DEST_IP_BY_NAME "lukaiqi.site"

float temperature()
{
	int fd;
	unsigned char result[2];    // 从ds18b20读出的结果，result[0]存放低八位
	unsigned char  integer_value = 0;
	float decimal_value = 0;    // 温度数值,decimal_value为小数部分的值
	float temperature = 0;
	float tt = 0;
	int temp = 0;
	fd = open("/dev/ds18b20", 0);
	if (fd < 0)
	{
		perror("open device failed\n");
		exit(1);
	}
	read(fd, &result, sizeof(result));
	temp = result[1];
	temp = temp << 8;
	temp = temp | result[0];
	tt = temp*0.0625;
	temp = tt*100+0.5;
	integer_value = temp/100;
	decimal_value = (temp%100/10)*0.1 + (temp%10)*0.01;
	temperature = (float)integer_value + decimal_value;
	close(fd);
	return temperature;
}

int mq2()
{
	int result;
	int fd = open("/dev/mq2", 0);
	if (fd < 0) {
		perror("open MQ2 device:");
		return 1;
	}
	for(;;) {
		char buffer[30];
		int len = read(fd, buffer, sizeof buffer -1);
		if (len > 0) {
			buffer[len] = '\0';
			int value = -1;
			sscanf(buffer, "%d", &value);
			result = (value*9700/1024)+300;

		} else {
			perror("read ADC device:");
			return 1;
		}
		close(fd);
		return result;
	}
}
void process_info(int fd)
{
	int send_num;
	char send_buf [10];
	char recv_buf [4096];
	char str1[4096];
	char str2[4096];
	float data;
	int data1;
	char temp[10];
	char smoke[10];
	char len[50];
	data=temperature();
	gcvt(data,6,temp);
	data1=mq2();
	gcvt(data1,4,smoke);
	memset(str1,0,4096);
	memset(str2,0,4096);
	strcat(str2,"ds18b20value=");
	strcat(str2,temp);
	strcat(str2,"&mq2value=");
	strcat(str2,smoke);
	strcat(str1, "POST http://lukaiqi.site/sensor/write HTTP/1.1\r\n");
	strcat(str1,"Host: lukaiqi.site\r\n");
	strcat(str1,"Content-Type: application/x-www-form-urlencoded\r\n");
	sprintf(len,"Content-Length: %d\r\n\r\n",strlen(str2));
	strcat(str1,len);
	strcat(str1,str2);
	send(fd, str1,strlen(str1),0);
	printf("ds18b20:%6.2f\n",data);
	printf("mq2:%d\n",data1);
}
int main()
{
	int i = 0;
	while(1)
	{
		int sock_fd;
		struct sockaddr_in addr_serv;
		sock_fd=socket(AF_INET, SOCK_STREAM, 0);
		if (sock_fd < 0)
		{
			perror("sock error");
			exit(1);
		}
		memset(&addr_serv, 0, sizeof(addr_serv));
		addr_serv.sin_family = AF_INET;
		addr_serv.sin_port = htons(DEST_PORT);
		addr_serv.sin_addr.s_addr = inet_addr(DEST_IP_ADDR);


		while (connect(sock_fd, (struct sockaddr*)(&addr_serv), sizeof(addr_serv)) < 0)
		{
			printf("try connection again\n");
			sleep(3);
		}
		process_info(sock_fd);
		close(sock_fd);
		i += 1;
		printf("i=%d\n",i);
		sleep(5);
	}
}
