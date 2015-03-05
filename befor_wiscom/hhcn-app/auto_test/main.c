//#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>


#include <linux/input.h>
#include <sys/types.h>
//#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
//#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <time.h>
#include <malloc.h>

extern int ads1100_open(int dev_num);
extern int ads1100_conv(int fd);

extern int dac8571_open(int dev_num);
extern int dac8571_conv(int fd,float value);

int ctrl_loop(int demoId);

int main(int argc,char *arg[])
{
	int ads1100_fd0,ads1100_fd1;
	int dac8571_fd0,dac8571_fd1;
	float vot_value = 0;
	int keys_fd;

	struct input_event keyEvent;
	keys_fd = open("/dev/input/event0", O_RDONLY);
	fcntl(keys_fd, F_SETFL, O_NONBLOCK);
	printf("Test starting...\n");
	ads1100_fd0 = ads1100_open(0);
	ads1100_fd1 = ads1100_open(1);	
	
	dac8571_fd0 = dac8571_open(0);
	dac8571_fd1 = dac8571_open(1);
	printf("\n\n");
	sleep(1);
	while (1) {
		usleep(50);
#ifdef AUTO_TEST
		if (read(keys_fd, &keyEvent, sizeof(keyEvent)) == sizeof(keyEvent)) {
			if (keyEvent.type == 4) {
				printf("keyEvent.value %d\n", keyEvent.value);

				if (keyEvent.value == 8) //图像/拍照
				{

				} else if (keyEvent.value == 9) //紫外
				{

                        
				} else if (keyEvent.value == 10) //确认
				{
					break;
                        
				} else if (keyEvent.value == 11)//功能
				{

                        
				} else if (keyEvent.value == 16)//光子计数
				{

				} else if (keyEvent.value == 17)//+号
				{
					vot_value += 0.5;
					if(vot_value > 11.0){
						printf("Volt Value is Heigher than 11.0,Reset to 0.\n");
						vot_value = 0;
					}
				} else if (keyEvent.value == 25)//-号
				{
					vot_value -= 0.5;
					if(vot_value < 0.0){
						printf("Volt Value is Lower than 0.0,Reset to 11.\n");
						vot_value = 11.0;
					}
				} else if (keyEvent.value == 18)//视频
				{

				} else if (keyEvent.value == 19) //增益
				{

				} else if (keyEvent.value == 24) //显示(三屏切换)
				{
					dac8571_conv(dac8571_fd0,vot_value);
					dac8571_conv(dac8571_fd1,vot_value);
					ads1100_conv(ads1100_fd0);
					ads1100_conv(ads1100_fd1);
				} else if (keyEvent.value == 26) //可见
				{

				} else if (keyEvent.value == 27) //OSD
				{
				}
			}
		}
#else
		vot_value += 0.5;
		if(vot_value > 11.0){
			printf("Volt Value is Heigher than 11.0,Reset to 0.\n");
			vot_value = 0;
		}
			
		dac8571_conv(dac8571_fd0,vot_value);
		dac8571_conv(dac8571_fd1,vot_value);
		printf("test\n\n");
		ads1100_conv(ads1100_fd0);
		ads1100_conv(ads1100_fd1);
		sleep(5);
#endif

	}
	close(keys_fd);
	close(dac8571_fd0);
	close(dac8571_fd1);
	close(ads1100_fd0);
	close(ads1100_fd1);
	printf("TEST Exit....\n");
	
	return 0;
        
}


int ctrl_loop(int demoId)
{
//	int status;
//	bool done = FALSE;
//	char ch;
	int keys_fd;

	struct input_event keyEvent;
	keys_fd = open("/dev/input/event0", O_RDONLY);
	fcntl(keys_fd, F_SETFL, O_NONBLOCK);

	while (1) {
		usleep(50);
		if (read(keys_fd, &keyEvent, sizeof(keyEvent)) == sizeof(keyEvent)) {
			if (keyEvent.type == 4) {
				printf("keyEvent.value %d\n", keyEvent.value);

				if (keyEvent.value == 8) //图像/拍照
				{

				} else if (keyEvent.value == 9) //紫外
				{

                        
				} else if (keyEvent.value == 10) //确认
				{

                        
				} else if (keyEvent.value == 11)//功能
				{

                        
				} else if (keyEvent.value == 16)//光子计数
				{

				} else if (keyEvent.value == 17)//+号
				{


				} else if (keyEvent.value == 25)//-号
				{

				} else if (keyEvent.value == 18)//视频
				{

				} else if (keyEvent.value == 19) //增益
				{

				} else if (keyEvent.value == 24) //显示(三屏切换)
				{
				} else if (keyEvent.value == 26) //可见
				{

				} else if (keyEvent.value == 27) //OSD
				{
				}
			}
		}

	}
	close(keys_fd);

	return 0;
}
