
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>


#define WATCHDOG_IOCTL_BASE 'W'
#define WDIOC_KEEPALIVE     _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define WDIOC_SETTIMEOUT        _IOWR(WATCHDOG_IOCTL_BASE, 6, int)

int main(int argc, char *argv[])
{
	int fd;
    int timeout = 1;

	if ((fd = open("/dev/watchdog", O_RDWR)) == -1) {
		perror("open");
		return -1;
	}
	ioctl(fd,WDIOC_SETTIMEOUT,&timeout);
	while(1)
    {
		ioctl(fd,WDIOC_KEEPALIVE,NULL);
		printf("keep alive\n");
		usleep(2000);
	}

	close(fd);
	return 0;
}

