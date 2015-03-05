/*
 * Application to test TI816X Watchdog Timer.
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/


#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/watchdog.h>
#include <sys/stat.h>
#include <signal.h> 

int fd;

void catch_int(int signum)
{
	signal(SIGINT, catch_int);

	printf("In signal handler\n");
	if(0 != close(fd))
		printf("Close failed in signal handler\n");
	else
		printf("Close succeeded in signal handler\n");
}

int main(int argc, const char *argv[]) {
        
	int sleep_time = atoi(argv[1]);	

	int data = 0;
        int ret_val;

	signal(SIGINT, catch_int);

        fd=open("/dev/watchdog",O_WRONLY);
        if (fd==-1) {
                perror("watchdog");
                return 1;
        }

        ret_val = ioctl (fd, WDIOC_GETTIMEOUT, &data);
        if (ret_val) {
	        printf ("\nWatchdog Timer : WDIOC_GETTIMEOUT failed");
        }
        else {
	        printf ("\nCurrent timeout value is : %d seconds\n", data);
        }

	data = 10;

        ret_val = ioctl (fd, WDIOC_SETTIMEOUT, &data);
        if (ret_val) {
	        printf ("\nWatchdog Timer : WDIOC_SETTIMEOUT failed");
        }
        else {
	        printf ("\nNew timeout value is : %d seconds", data);
        }

        ret_val = ioctl (fd, WDIOC_GETTIMEOUT, &data);
        if (ret_val) {
	        printf ("\nWatchdog Timer : WDIOC_GETTIMEOUT failed");
        }
        else {
	        printf ("\nCurrent timeout value is : %d seconds\n", data);
        }

        while(1) 
        {
                if (1 != write(fd, "\0", 1))
		{
			printf("Write failed\n");
			break;
		}
		else
			printf("Write succeeded\n");

                sleep(sleep_time);
        }

        if (0 != close (fd))
		printf("Close failed\n");
	else
		printf("Close succeeded\n");
	
	return 0;
}
