/*           ·þÎñ¶Ë³ÌÐò  server.c           */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define SERVER_PORT     8888
#define MAX_MSG_SIZE    1024

void udps_respon(int sockfd)
{
    struct sockaddr_in addr;
    int    n;
    socklen_t addrlen;
    char    msg[MAX_MSG_SIZE];
        
    while(1)
    {       /* 清理缓存   */
        memset(msg, 0, sizeof(msg));
        addrlen = sizeof(struct sockaddr);
        n=recvfrom(sockfd,msg,MAX_MSG_SIZE,0,
                   (struct sockaddr*)&addr,&addrlen);
        /* 打印接受到的数据 */
        fprintf(stdout,"I have received %s from %s.\n",msg,inet_ntoa(addr.sin_addr));
        sendto(sockfd,msg,n,0,(struct sockaddr*)&addr,addrlen);
    }
}

int main(void)
{
    int sockfd;
    struct sockaddr_in      addr;

    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<0)
    {
        fprintf(stderr,"Socket Error:%s\n",strerror(errno));
        exit(1);
    }
    bzero(&addr,sizeof(struct sockaddr_in));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(SERVER_PORT);
    if(bind(sockfd,(struct sockaddr *)&addr,sizeof(struct sockaddr_in))<0)
    {
        fprintf(stderr,"Bind Error:%s\n",strerror(errno));
        exit(1);
    }
    udps_respon(sockfd);
    close(sockfd);
}
