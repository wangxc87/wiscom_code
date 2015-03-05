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



int Demo_runEx(int demoId)
{
	int status;
	Bool done = FALSE;
	char ch;
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
