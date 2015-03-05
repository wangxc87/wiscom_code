#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "../module/led.h"

#define LED1_DEV "/dev/led1"
#define LED2_DEV "/dev/led2"

int main(void)
{
        int led1_fd,led2_fd;
        int ret;

        led1_fd = open(LED1_DEV,O_RDWR);
        if(led1_fd < 0){
                printf("Error:cannot open %s\n",LED1_DEV);
                return -1;
        }
        
        led2_fd = open(LED2_DEV,O_RDWR);
        if(led2_fd <0){
                printf("Error:cannot open %s\n",LED1_DEV);
                return -1;
        }
        int i=20;
        while(i--){
                ioctl(led1_fd,LED_ON,NULL);
                //       ioctl(led2_fd,LED_ON,NULL);
                sleep(1);
                ioctl(led1_fd,LED_OFF,NULL);
//                ioctl(led2_fd,LED_OFF,NULL);
                sleep(1);
                printf("loop %d...\t",i);
                if(0 == i%5)
                        printf("\n");
                
                
        }
        
        close(led1_fd);
        close(led2_fd);
        printf("led is closed..\n");
         
        return 0;
}
