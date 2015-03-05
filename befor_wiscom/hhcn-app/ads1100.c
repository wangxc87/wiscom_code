#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#define ADS_IOCTL_BASE                     'W'
#define ADS_CONVERT_AND_GET_CMD             _IO(ADS_IOCTL_BASE, 0)
#define ADS_CONVERT_CMD                     _IO(ADS_IOCTL_BASE, 1)
#define ADS_GET_OUTPUT_CMD                  _IO(ADS_IOCTL_BASE, 2)
#define ADS_SINGLE_CONVERT_CMD              _IO(ADS_IOCTL_BASE, 3)
#define ADS_CONTIN_CONVERT_CMD              _IO(ADS_IOCTL_BASE, 4)
#define ADS_SET_DATARATE_CMD                _IO(ADS_IOCTL_BASE, 5)
#define ADS_SET_GAIN_CMD                    _IO(ADS_IOCTL_BASE, 6)

// #define SLEEP(x)       sleep(x)
#define SLEEP(x)       

static void printValue(const unsigned int v)
{
	int value, mask;
	double volt;

	switch ((v & 0x0c) >> 2) {
		case 0:
			mask = 0x0FFF;
			break;
		case 1:
			mask = 0x3FFF;
			break;
		case 2:
			mask = 0x7FFF;
			break;
		case 3:
			mask = 0xFFFF;
			break;
		default:
			fprintf(stderr, "error: %08X\n", (v & 0x0c) >> 2);
			exit(-1);
	}

	value = mask & (int) (v >> 8);
	
	if (value != (0xFFFF & (int) (v >> 8))) {
		fprintf(stderr, "internal error, value: %08X, v: %08X\n", value, v >> 8);
	}

	if (value & ((mask + 1) >> 1)) {
		value = -1 - ((~value) & (mask >> 1));
	}

	volt = (double) value;
	volt /= (double) (mask >> 1);
	volt /= (double) (1 << (v & 0x03));
	volt *= 3.30 * 4.0;

	fprintf(stdout, "Con: %02X, data: %04X, volt: %lf\n", v & 0xff, (v >> 8) & 0xffff, volt);
}

int main(int argc, char *argv[])
{
	int fd;
	char dev_path[0x40];
	unsigned int value, v;

	if (argc != 0x02) {
		fprintf(stderr, "Usage: %s (0 | 1)\n", argv[0]);
		return -1;
	}

	snprintf(dev_path, 0x40, "/dev/ads1100a%d", atoi(argv[1]) ? 1 : 0);
	fprintf(stdout, "Reading value from ADC device \"%s\" ...\n", dev_path);
	
	if ((fd = open(dev_path, O_RDWR)) == -1) {
		perror("open");
		return -1;
	}
#if defined(ENABLE_CONTINUE_CONVERSION)
	if (ioctl(fd, ADS_CONTIN_CONVERT_CMD, NULL) == -1) {
#else
	if (ioctl(fd, ADS_SINGLE_CONVERT_CMD, NULL) == -1) {
#endif
		perror("ioctl0");
#if !defined(ENABLE_CONTINUE_CONVERSION)
		goto exit_prog;
#endif
	}
	SLEEP(1);

	value = 0;
	if (ioctl(fd, ADS_SET_GAIN_CMD, &value) == -1) {
		perror("ioctl1");
		goto exit_prog;
	}
	sleep(1);

	for (v = 0; v < 0x04; ++v) {
		value = v;
		if (ioctl(fd, ADS_SET_DATARATE_CMD, &value) == -1) {
			perror("ioctl1");
			goto exit_prog;
		}
		SLEEP(1);
		// sleep(1);
		value = 0;
#if defined(ENABLE_CONTINUE_CONVERSION)
		if (ioctl(fd, ADS_GET_OUTPUT_CMD, &value) == -1)  {
#else
		if (ioctl(fd, ADS_CONVERT_AND_GET_CMD, &value) == -1) {
#endif
			perror("ioctl2");
			goto exit_prog;
		}
		printValue(value);
		SLEEP(1);
		sleep(1);
#if 0
		value = 0;
		if (ioctl(fd, ADS_SET_GAIN_CMD, &value) == -1) {
			perror("ioctl1");
			goto exit_prog;
		}
		SLEEP(1);
		value = 0;
		if (ioctl(fd, ADS_CONVERT_AND_GET_CMD, &value) == -1) {
			perror("ioctl2");
			goto exit_prog;
		}
		printValue(value);
		SLEEP(1);
		sleep(1);
#endif
	}

	fputs("Done.\n", stdout);

exit_prog :
	close(fd);
	return 0;
}

