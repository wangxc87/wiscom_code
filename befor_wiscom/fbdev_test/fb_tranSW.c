#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <linux/ti81xxfb.h>

#define DISPLAY_DEVICE0  "/dev/fb0"
#define DISPLAY_DEVICE1  "/dev/fb1"
#define DISPLAY_DEVICE2  "/dev/fb2"

int main(int argc, char ** argv)
{
        struct ti81xxfb_region_params regp;
        int display_fd;
        char *device_name; 
        strcpy(device_name,DISPLAY_DEVICE1);
        
        display_fd = open(device_name, O_RDWR);
        if (display_fd <= 0) {
                printf("ERR:Could not open device:%s\n", device_name);
                return -1;
        }
        printf("Open device %s OK+++++\n", device_name);


        if (ioctl(display_fd, TIFB_GET_PARAMS, &regp) < 0) {
                printf("ERR:TIFB_GET_PARAMS !!!\n");
        }

        regp.transen = TI81XXFB_FEATURE_DISABLE;
        if (ioctl(display_fd, TIFB_SET_PARAMS, &regp) < 0) {
                printf("ERR:TIFB_SET_PARAMS !!!\n");
        }
        close(display_fd);
        return 0;
        
}

