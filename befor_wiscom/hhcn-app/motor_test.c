

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_NAME                        "pwm-motor"
#define MOTOR_SELE_LEFT                     0x04
#define MOTOR_SELE_RIGHT                    0x08
#define MOTOR_SELECTION_MASK                (MOTOR_SELE_LEFT | MOTOR_SELE_RIGHT)
#define MOTOR_CONTROL_LEFT                  0x01
#define MOTOR_CONTROL_RIGHT                 0x02

#define PWM_MOTOR_IOCTL_BASE               'M'
#define PWM_MOTOR_RESET_CMD                _IO(PWM_MOTOR_IOCTL_BASE, 0)
#define PWM_MOTOR_SET_DIRECTION_CMD        _IO(PWM_MOTOR_IOCTL_BASE, 1)
#define PWM_MOTOR_GET_COUNTER_CMD          _IO(PWM_MOTOR_IOCTL_BASE, 2)
#define PWM_MOTOR_START_CMD                _IO(PWM_MOTOR_IOCTL_BASE, 3)
#define PWM_MOTOR_STOP_CMD                 _IO(PWM_MOTOR_IOCTL_BASE, 4)
#define PWM_MOTOR_STATE_CMD                _IO(PWM_MOTOR_IOCTL_BASE, 5)
#define PWM_MOTOR_TLDR_CMD                 _IO(PWM_MOTOR_IOCTL_BASE, 6)

int main(int argc, char *argv[])
{
        int fd, i=4;
    int speed = 0x0FFF0000;
    
    unsigned int value, counter[0x02];

/*	if (argc != 0x02) {
		fprintf(stderr, "Usage: %s times\n", argv[0]);
		return -1;
	}
*/
    if(argc == 0x3){
            i = atoi(argv[1]);
            if (i <= 0 || i > 0x020) {
                    i = 4;
                    fprintf(stdout, "argument out of range, setting times to %d\n", i);
            }
            speed = atoi(argv[2]);
    }else if(0x2 == argc){
            i = atoi(argv[1]);
        /*    if (i <= 0 || i > 0x020) {
                    i = 4;
                    fprintf(stdout, "argument out of range, setting times to %d\n", i);
            }
*/
    }
    
    
    
	if ((fd = open("/dev/" DEVICE_NAME, O_RDWR)) == -1) {
		perror("open");
		return -1;
	}
#if 0
	if (ioctl(fd, PWM_MOTOR_RESET_CMD, NULL) == -1) {
		perror("ioctl0");
		goto exit_prog;
	}
	fputs("RESETED\n", stdout);
	fflush(stdout);
	sleep(1);
#endif

	if (ioctl(fd, PWM_MOTOR_START_CMD, NULL) == -1) {
		perror("ioctl1");
		goto exit_prog;
	}

  value = MOTOR_SELE_LEFT | MOTOR_SELE_RIGHT | MOTOR_CONTROL_LEFT;
    if (ioctl(fd, PWM_MOTOR_SET_DIRECTION_CMD, &value) == -1) {
			perror("ioctl2");
			goto exit_prog;
    }
        
    if (ioctl(fd,PWM_MOTOR_TLDR_CMD, &speed) == -1) {
			perror("ioctl2");
			goto exit_prog;
    }
    sleep(1);
if(i){
    while(i--)
            sleep(1);
    }
else {
  while(1)
     sleep(1);
  }
//#ifdef SET_DIRECTION
#if 0
	if (ioctl(fd, PWM_MOTOR_START_CMD, NULL) == -1) {
		perror("ioctl1");
		goto exit_prog;
	}

	while (i--) {
		value = MOTOR_SELE_LEFT | MOTOR_SELE_RIGHT | MOTOR_CONTROL_LEFT;
		if (ioctl(fd, PWM_MOTOR_SET_DIRECTION_CMD, &value) == -1) {
			perror("ioctl2");
			goto exit_prog;
		}
		sleep(1);
		counter[0] = counter[1] = 0;
		if (ioctl(fd, PWM_MOTOR_GET_COUNTER_CMD, counter) == -1) {
			perror("ioctl4");
			goto exit_prog;
		}
		fprintf(stdout, "counter value: %08X, %08X, %08X\n", counter[0], counter[1],
			(counter[1] > counter[0]) ? (counter[1] - counter[0]) : (counter[0] - counter[1]));
		fflush(stdout);
		sleep(1);

		value = MOTOR_SELE_LEFT | MOTOR_SELE_RIGHT | MOTOR_CONTROL_RIGHT;
		if (ioctl(fd, PWM_MOTOR_SET_DIRECTION_CMD, &value) == -1) {
			perror("ioctl2");
			goto exit_prog;
		}
		sleep(1);
		counter[0] = counter[1] = 0;
		if (ioctl(fd, PWM_MOTOR_GET_COUNTER_CMD, counter) == -1) {
			perror("ioctl4");
			goto exit_prog;
		}
		fprintf(stdout, "counter value: %08X, %08X, %08X\n", counter[0], counter[1],
			(counter[1] > counter[0]) ? (counter[1] - counter[0]) : (counter[0] - counter[1]));
		fflush(stdout);
		sleep(1);
	}

	value = 0;
	if (ioctl(fd, PWM_MOTOR_STATE_CMD, &value) == -1) {
		perror("ioctl3");
		goto exit_prog;
	}
	fprintf(stdout, "MOTOR state: %08x\n", value);

	if (value != (MOTOR_CONTROL_LEFT | MOTOR_CONTROL_RIGHT)) {
		fprintf(stderr, "motor direction error: %d\n", value);
	}
#endif

	if (ioctl(fd, PWM_MOTOR_STOP_CMD, NULL) == -1) {
		fprintf(stderr, "Failed to stop motor: %s\n", strerror(errno));
		goto exit_prog;
	}

	fputs("Done\n", stdout);

exit_prog :
	close(fd);
	return 0;
}

