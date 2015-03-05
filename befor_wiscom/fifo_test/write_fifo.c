#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
//#define BUFES PIPE_BUF
#define BUFES 0xfff
int main(void)
{
        int fd ;
        int n, i ;
        char buf[BUFES];
        time_t tp;
        int ret = 0;
        time(&tp); /*取系统当前时间*/
        printf("I am write:%d,%s.\n",getpid(),ctime(&tp)); /*说明进程的ID*/
        fd=open("fifo1",O_WRONLY );
        printf("I am write:%d,%s.\n",getpid(),ctime(&tp)); /*说明进程的ID*/
        if(fd<0){ /*以写打开一个FIFO1*/
                printf("open error.\n");
//                exit(1);
                ret =0 ;
                goto exit1;
        }else
                printf("Open successfully ...\n");
        
        
        for ( i=0 ; i<10; i++){ /*循环10次向FIFO中写入数据*/
                time(&tp); /*取系统当前时间*/
                /*使用sprintf 函数向buf中格式化写入进程ID 和时间值*/
                n=sprintf(buf,"write_fifo %d sends %s",getpid(),ctime(&tp));
                printf("Send msg:%s\n",buf);
                if((write(fd, buf, n+1))<0) { /*写入到FIFO中*/
                        printf("write error.\n");
                        //                close(fd); /* 关闭FIFO文件 */
                        //                exit(1);
                        ret = 1;
                        goto exit1;
                        
                }
                sleep(1); /*进程睡眠3秒*/
                printf("write Loop %d...\n",i);
                
        }
exit1:        printf("FIFO Write process exit (%d)..\n",ret);
        
        close(fd); /* 关闭FIFO文件 */
        exit(0);
}
