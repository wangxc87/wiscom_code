#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#define SECOND_DEV  "/dev/second"

int quit = 1;
void signal_quit(int signum)
{
        quit = 0;
        printf("signum is %d.\n",signum);
}

int main(void)
{
        int ret;
        int fd;
        int second;
        int flags;
        
        fd = open(SECOND_DEV,O_RDWR);
        
        if(fd < 0){
                printf("Err:open %s failed..\n",SECOND_DEV);
                return -1;
        }

        signal(SIGIO,signal_quit);
        fcntl(fd,F_SETOWN,getpid());
        flags = fcntl(fd,F_GETFL);
        fcntl(fd,F_SETFL,flags | FASYNC);
        
        while(quit){
                ret = read(fd,&second,1);
                if(ret != sizeof(unsigned int)){
                        printf("Err:read failed...\n");
                        goto err_out;
                }
                printf("second is %d\n",second);
                
        }
        close(fd);
        return 0;
err_out:
        close(fd);
        return -1;
}
