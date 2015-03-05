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
        time(&tp); /*ȡϵͳ��ǰʱ��*/
        printf("I am write:%d,%s.\n",getpid(),ctime(&tp)); /*˵�����̵�ID*/
        fd=open("fifo1",O_WRONLY );
        printf("I am write:%d,%s.\n",getpid(),ctime(&tp)); /*˵�����̵�ID*/
        if(fd<0){ /*��д��һ��FIFO1*/
                printf("open error.\n");
//                exit(1);
                ret =0 ;
                goto exit1;
        }else
                printf("Open successfully ...\n");
        
        
        for ( i=0 ; i<10; i++){ /*ѭ��10����FIFO��д������*/
                time(&tp); /*ȡϵͳ��ǰʱ��*/
                /*ʹ��sprintf ������buf�и�ʽ��д�����ID ��ʱ��ֵ*/
                n=sprintf(buf,"write_fifo %d sends %s",getpid(),ctime(&tp));
                printf("Send msg:%s\n",buf);
                if((write(fd, buf, n+1))<0) { /*д�뵽FIFO��*/
                        printf("write error.\n");
                        //                close(fd); /* �ر�FIFO�ļ� */
                        //                exit(1);
                        ret = 1;
                        goto exit1;
                        
                }
                sleep(1); /*����˯��3��*/
                printf("write Loop %d...\n",i);
                
        }
exit1:        printf("FIFO Write process exit (%d)..\n",ret);
        
        close(fd); /* �ر�FIFO�ļ� */
        exit(0);
}
