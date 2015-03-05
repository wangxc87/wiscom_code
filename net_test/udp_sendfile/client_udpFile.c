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

#include "bitsRead.h"

#define MAX_BUF_SIZE    1024

#define UInt32 unsigned int
#define UInt16 unsigned short
#define UInt8 unsigned char
#define Int32 int
#define Int16 short
#define Int8  char

struct udp_ackMsg {
    UInt32 frame_num;
    UInt16 send_num;
    UInt32 send_size;
};

#define RECV_BUFFER_SIZE 16
#define DEFINE_HEAD_SIZE 3 /* 0-1:send_num,2-3: 0xff eof the buf */
#define NEXT_FRAME_CMD 0xff55ff55

Int8 buffer[MAX_BUF_SIZE + DEFINE_HEAD_SIZE];


Int32 udp_send(Int32 sockfd,const struct sockaddr_in *addr,Int8 *srcbuf,Int32 bufsize)
{
    struct udp_ackMsg recv_buffer;
    UInt16 i = 0,n;
    Int32 ret;
    UInt32 lengh,retsend,count;
    static UInt32 frame_count = 0;
    Int32 tempsize,done;
    Int32 max_send_size = MAX_BUF_SIZE;

    n = bufsize / max_send_size;
    tempsize = bufsize;
    count = bufsize % max_send_size;
    frame_count ++;
    done = 0;
    memset(buffer,0,(MAX_BUF_SIZE + DEFINE_HEAD_SIZE));
    do{
         /*   发送数据 */
        *(UInt16 *)buffer = i;
        
        if(tempsize < max_send_size){
            done = 1;
            lengh = tempsize % max_send_size;
            *(Int8 *)(buffer + 2) = 0xff;//define the end of buffer
        }
        else{
            tempsize -= max_send_size;
            lengh = max_send_size;
            *(Int8 *)(buffer + 2) = 0;
        }
        
        /* 拷贝要发送的数据*/        
        memcpy((buffer + DEFINE_HEAD_SIZE),(srcbuf + i * max_send_size),lengh);

        retsend = sendto(sockfd,buffer,(lengh + DEFINE_HEAD_SIZE),0,(struct sockaddr *)addr,sizeof(struct sockaddr_in));
        if(retsend != (lengh + DEFINE_HEAD_SIZE)){
            fprintf(stderr,"data send Error retsend-lengh: %d-%d.",retsend,lengh);
            return -1;
        }

        /*等待反馈 */
        ret = recvfrom(sockfd,&recv_buffer,RECV_BUFFER_SIZE, 0, NULL, NULL);
        if(ret <= 0)
        {
            fprintf(stderr, "Recv Error %s\n", strerror(errno));
            return -1;
        }else{
            if(frame_count != recv_buffer.frame_num){
                    fprintf(stderr,"Recv frame_num Error recv/send: %d/%d.\n",recv_buffer.frame_num,frame_count);
                    return -1;
                }
            if(i != recv_buffer.send_num){
                        fprintf(stderr,"Recv msg_num Error recv-send:%d-%d.\n",recv_buffer.send_num,i);
                        return -1;
                    }
                if (retsend != recv_buffer.send_size){
                    fprintf(stderr,"Recv msg_size Error recv-send:%d-%d.\n",recv_buffer.send_size,retsend);
                    return -1;
                }
                fprintf(stdout,"Frame %d: get ACK from %s.\n",recv_buffer.frame_num,inet_ntoa(addr->sin_addr));
        }
        i++;
        //        fprintf(stderr, "get %s from %s.\n", buffer,inet_ntoa(addr->sin_addr));
    }while(!done);
    return 0;
}


Int32 main(Int32 argc,Int8 **argv)
{
    Int32 sockfd,port,sockfd0;
        Int32 ret;
        struct sockaddr_in      addr,addr0;

        struct BufInfo bufinfo,*pbufinfo;
        pbufinfo = & bufinfo;

        bitsRead_init(pbufinfo); //初始化读写文件
        
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
        /*     发送数据套结字      */
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

        
        sockfd0=socket(AF_INET,SOCK_DGRAM,0); //创建udp等待请求套接字
        if(sockfd0 < 0)
        {
                fprintf(stderr,"Socket0  Error:%s\n",strerror(errno));
                exit(1);
        }

        /*等待请求套结字*/
        bzero(&addr0,sizeof(struct sockaddr_in));
        addr0.sin_family=AF_INET;
        addr0.sin_addr.s_addr=htonl(INADDR_ANY);
        addr0.sin_port=htons(8889);
        if(bind(sockfd0,(struct sockaddr *)&addr0,sizeof(struct sockaddr_in))<0)
        {
            fprintf(stderr,"Bind Error:%s\n",strerror(errno));
            exit(1);
        }        
        while(1) {
            ret = recvfrom(sockfd0,buffer,1024,0,NULL,NULL);
            if(ret > 0)
                printf("recv %s .\n",buffer);
        }
        
        //udp_requ(sockfd,&addr,sizeof(struct sockaddr_in));
        do{
            //            getchar();
            ret = bitsRead_process(pbufinfo);
            udp_send(sockfd,&addr,pbufinfo->pBuffer,pbufinfo->filledBufSize);

            while(1) {  //等待下一帧的数据请求
                  ret = recvfrom(sockfd,buffer,10,0,NULL,NULL);
                  if(ret > 0){
                      ret = 0;
                      //                      fprintf(stdout,"recv CMD 0x%x.\n",*(Int32 *)buffer);
                      if(*(Int32 *)buffer == NEXT_FRAME_CMD)
                          break;
                  }
            }
    
        }while(ret == 0);

        if(ret < 0)
            fprintf(stderr,"bitsRead_process error..\n");

        close(sockfd);
        bitsRead_deinit(pbufinfo);
        fprintf(stdout,"test run over..\n");
        return 0;
}
