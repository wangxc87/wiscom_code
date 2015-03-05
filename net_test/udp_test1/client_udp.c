/*          客户端程序             */
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
#define MAX_BUF_SIZE    1024
void udp_requ(int sockfd,const struct sockaddr_in *addr,socklen_t len)
{
        char buffer[MAX_BUF_SIZE];
        int n;
        fprintf(stdout,"Input please :\n");
        while(fgets(buffer,MAX_BUF_SIZE,stdin))        
        {        /*   发送输入的字符 */
            sendto(sockfd,buffer,(strlen(buffer)-1),0,(struct sockaddr *)addr,len);
            bzero(buffer,MAX_BUF_SIZE);

                /*   清零buffer    */
            memset(buffer, 0, sizeof(buffer));
            n=recvfrom(sockfd,buffer,MAX_BUF_SIZE, 0, NULL, NULL);
            if(n <= 0)
            {
                fprintf(stderr, "Recv Error %s\n", strerror(errno));
                return;
            }
            buffer[n]=0;
            fprintf(stderr, "get %s from %s.\n", buffer,inet_ntoa(addr->sin_addr));
        }
}


int main(int argc,char **argv)
{
        int sockfd,port;
        struct sockaddr_in      addr;
        
        if(argc!=3)
        {
                fprintf(stderr,"Usage:%s server_ip server_port(8888)\n",argv[0]);
                exit(1);
        }
        
        if((port=atoi(argv[2]))<0)  //读取输入的端口号，服务器端口号为8888
        {
                fprintf(stderr,"Usage:%s server_ip server_port\n",argv[0]);
                exit(1);
        }
        
        sockfd=socket(AF_INET,SOCK_DGRAM,0); //创建udp套接字
        if(sockfd<0)
        {
                fprintf(stderr,"Socket  Error:%s\n",strerror(errno));
                exit(1);
        }       
        /*      Ìî³ä·þÎñ¶ËµÄ×ÊÁÏ      */
        bzero(&addr,sizeof(struct sockaddr_in));
        addr.sin_family=AF_INET;
        addr.sin_port=htons(port); //转换端口号
        if(inet_aton(argv[1],&addr.sin_addr)<0)  //转换输入的ip地址
        {
                fprintf(stderr,"Ip error:%s\n",strerror(errno));
                exit(1);
        }
        if(connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) //建立连接
        {
            fprintf(stderr, "connect error %s\n", strerror(errno));
            exit(1);
        }
        udp_requ(sockfd,&addr,sizeof(struct sockaddr_in));
        close(sockfd);
}
