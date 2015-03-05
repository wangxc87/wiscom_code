
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define DAC_IOCTL_BASE                     'W'

#define DAC_STORE_DATA_CMD                  _IO(DAC_IOCTL_BASE, 0)
#define DAC_UPDATE_DATA_CMD                 _IO(DAC_IOCTL_BASE, 1)
#define DAC_UPDATE_STORED_DATA_CMD          _IO(DAC_IOCTL_BASE, 2)
#define DAC_BROADCAST_UPDATE_CMD            _IO(DAC_IOCTL_BASE, 3)
#define DAC_BROADCAST_UPDATE_DATA_CMD       _IO(DAC_IOCTL_BASE, 4)
#define DAC_PWD_LPM_CMD                     _IO(DAC_IOCTL_BASE, 5)
#define DAC_PWD_FAST_SETTING_CMD            _IO(DAC_IOCTL_BASE, 6)
#define DAC_PWD_1K_CMD                      _IO(DAC_IOCTL_BASE, 7)
#define DAC_PWD_100K_CMD                    _IO(DAC_IOCTL_BASE, 8)
#define DAC_PWD_OUTPUT_CMD                  _IO(DAC_IOCTL_BASE, 9)

int main(int argc, char *argv[])
{
	int fd;
	float value;
	char dev_name[256];
	if (argc != 0x03) {
            fprintf(stderr, "usage: %s devname Value,eg %s 0/1 Vott_value (0~10).\n", argv[0],argv[0]);
		return -1;
	}
	snprintf(dev_name, 256, "/dev/dac8571_%s", argv[1]);
//	value = atoi(argv[2]);
    value = atof(argv[2]);
    printf("Input Vott Value is %f.\n",value);
    
	//fprintf(stdout, "updating device \"%s\" to value %08X...\n", dev_name, value);

	if ((fd = open(dev_name, O_RDWR)) == -1) {
		perror("open");
		return -1;
	}
    int value_hex;
    value_hex = (value/10)*65535;
    printf("Updating device Reg Value is 0x%x.\n",value_hex);
    
	if (ioctl(fd, DAC_UPDATE_DATA_CMD, &value_hex) == -1) {
		perror("ioctl0");
		goto exit_prog;
	}
	fprintf(stdout, "Done: %d\n", __LINE__);

#if 0
	value++;
	if (ioctl(fd, DAC_STORE_DATA_CMD, NULL) == -1) {
		perror("ioctl1");
		goto exit_prog;
	}
	fprintf(stdout, "At line: %d\n", __LINE__);

	value++;
	if (ioctl(fd, DAC_UPDATE_STORED_DATA_CMD, NULL) == -1) {
		perror("ioctl2");
		goto exit_prog;
	}
	fprintf(stdout, "At line: %d\n", __LINE__);

	value++;
	if (ioctl(fd, DAC_PWD_LPM_CMD, NULL) == -1) {
		perror("ioctl3");
		goto exit_prog;
	}
	fprintf(stdout, "At line: %d\n", __LINE__);

	value++;
	if (ioctl(fd, DAC_PWD_FAST_SETTING_CMD, NULL) == -1) {
		perror("ioctl4");
		goto exit_prog;
	}
	fprintf(stdout, "At line: %d\n", __LINE__);

	value++;
	if (ioctl(fd, DAC_UPDATE_DATA_CMD, &value) == -1) {
		perror("ioctl5");
		goto exit_prog;
	}
	fprintf(stdout, "At line: %d\n", __LINE__);
#endif

exit_prog :
	close(fd);
	return 0;
}

