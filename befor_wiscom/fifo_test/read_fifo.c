#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#define BUFES PIPE_BUF
int main(void)
{
        int fd;
        int len;
        char buf[BUFES];
        mode_t mode = 0666; /* FIFO�ļ���Ȩ�� */
        printf("I am read:%d.\n",getpid()); /*˵�����̵�ID*/
        if((fd=open("fifo1",O_RDONLY))<0) /* ��FIFO�ļ� */
        {
                perror("open");
                exit(1);
        }
        while((len=read(fd,buf, BUFES))>0) /* ��ʼ����ͨ�� */
                printf("read_fifo read: %s",buf);
        close(fd); /* �ر�FIFO�ļ� */
        exit(0);
}
