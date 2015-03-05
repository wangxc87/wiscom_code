/*          dvrrdk_server 端程序             */
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
#include <signal.h>
#include <sys/ioctl.h>
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
#define END_FRAME_CMD  0xff55ff56
#define SERVER_PORT_NUM 8888

Int8 buffer[MAX_BUF_SIZE + DEFINE_HEAD_SIZE];


Int32 udp_send(Int32 sockfd,Int32 sockfd_cmd_c,const struct sockaddr_in *addr,Int8 *srcbuf,Int32 bufsize)
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
    do{
        memset(buffer,0,(MAX_BUF_SIZE + DEFINE_HEAD_SIZE));
         /*   发送数据 */
        *(UInt16 *)buffer = i;
        if(tempsize < max_send_size){
            done = 1;
            lengh = tempsize % max_send_size;
            *(Int8 *)(buffer + 2) = 0xff;//define the end of buffer
        }else if(tempsize > max_send_size){
            tempsize -= max_send_size;
            lengh = max_send_size;
            *(Int8 *)(buffer + 2) = 0;
            done = 0;
        }else
            break;
        
        //        bzero(&recv_buffer,sizeof(struct udp_ackMsg));
        memset(&recv_buffer,0,sizeof(struct udp_ackMsg));        
        /* 拷贝要发送的数据*/        
        memcpy((buffer + DEFINE_HEAD_SIZE),(srcbuf + i * max_send_size),lengh);

        retsend = sendto(sockfd,buffer,(lengh + DEFINE_HEAD_SIZE),0,(struct sockaddr *)addr,sizeof(struct sockaddr_in));
        if(retsend != (lengh + DEFINE_HEAD_SIZE)){
            fprintf(stderr,"data send Error retsend-lengh: %d-%d.",retsend,lengh);
            ret = retsend;
            break;
        }

        /*等待反馈 */
        ret = read(sockfd_cmd_c,&recv_buffer,RECV_BUFFER_SIZE);
        if(ret <= 0)
        {
            fprintf(stderr, "Recv Error %s\n", strerror(errno));
            break;
        }else{
            if(frame_count != recv_buffer.frame_num){
                    fprintf(stderr,"Recv frame_num Error recv/send: %d/%d.\n",recv_buffer.frame_num,frame_count);
                    ret = -1;
                    break;
                }
            if(i != recv_buffer.send_num){
                fprintf(stderr,"Recv msg_num Error recv-send:%d-%d.\n",recv_buffer.send_num,i);
                ret = -1;
                break;
            }
            if (retsend != recv_buffer.send_size){
                fprintf(stderr,"Recv msg_size Error recv-send:%d-%d.\n",recv_buffer.send_size,retsend);
                ret = -1;
                break;
            }
            fprintf(stdout,"Frame %d:%d: get ACK from %s.\n",recv_buffer.frame_num,recv_buffer.send_num,inet_ntoa(addr->sin_addr));
        }
        i++;
    }while(!done);

    if(ret > 0) {
        do{
            *(UInt32 *)buffer = END_FRAME_CMD;
            ret = write(sockfd_cmd_c,buffer,sizeof(UInt32));
        }while(ret != 4 );
    if(ret = 4)
        ret = 0;
    }
    return ret;
}
static Int32 gDone = 0;

void signal_handle(int iSignNo)
{
    switch(iSignNo){
    case SIGINT:
        //        done = 1;
        fprintf(stdout,"SIGINT (Ctrl + c) capture..\n");
        break;
    case SIGQUIT:
        gDone = 1;
        fprintf(stdout,"SIGQUIT (Ctrl + \\)Capture....\n");
        break;
    default:
        break;
    }
}

Int32 main(Int32 argc,Int8 **argv)
{
    Int32 sockfd,port,sockfd_cmd,sockfd_cmd_connect;
    Int32 ret;
    struct sockaddr_in addr,addr_cmd,addr_cmd_client;
    Int32 addr_len;

    struct BufInfo bufinfo,*pbufinfo;
    UInt32 addrlen;
    struct timeval tv_out;

    pbufinfo = & bufinfo;

    //    signal(SIGINT,signal_handle); //抓取ctrl+c
    signal(SIGQUIT,signal_handle);//抓取ctrl+\正常退出信号 

    bitsRead_init(pbufinfo); //初始化读写文件

    sockfd=socket(AF_INET,SOCK_DGRAM,0); //创建udp等待请求套接字
    if(sockfd < 0) {
        fprintf(stderr,"Socket0  Error:%s\n",strerror(errno));
        exit(1);
    }
#if 0    
    //    fcntl(sockfd,F_SETFL,O_NONBLOCK);//非阻塞,默认为阻塞模式

         tv_out.tv_sec = 3;//等待10秒
         tv_out.tv_usec = 0;
         setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));
#endif
        /*等待请求套结字*/
         /*
        bzero(&addr,sizeof(struct sockaddr_in));
        addr.sin_family=AF_INET;
        addr.sin_addr.s_addr=htonl(INADDR_ANY);
        addr.sin_port=htons(SERVER_PORT_NUM);
        if(bind(sockfd,(struct sockaddr *)&addr,sizeof(struct sockaddr_in))<0)
        {
            fprintf(stderr,"Bind Error:%s\n",strerror(errno));
            exit(1);
        }        
         */

        /* 创建命令通信的tcp socket */
        sockfd_cmd = socket(AF_INET,SOCK_STREAM,0);
        if(sockfd_cmd < 0){
            fprintf(stderr,"Socket error:%s\n\a",strerror(errno));
            goto exit_t;
        }
        /*CMD等待请求套结字*/
        bzero(&addr_cmd,sizeof(struct sockaddr_in));
        addr_cmd.sin_family=AF_INET;
        addr_cmd.sin_addr.s_addr=htonl(INADDR_ANY);
        addr_cmd.sin_port = htons(SERVER_PORT_NUM + 1);
        ret = bind(sockfd_cmd,(struct sockaddr *)(&addr_cmd),sizeof(struct sockaddr));
        if(ret < 0){
            fprintf(stderr,"Bind error:%s\n\a",strerror(errno));
            goto exit_t;
        }
        ret = listen(sockfd_cmd,5);
        if(ret < 0){
            fprintf(stderr,"Listen error:%s\n\a",strerror(errno));
            goto exit_t;
        }

        //        bzero(&addr_cmd_client,sizeof(struct sockaddr_in));
        bzero(&addr_cmd_client,sizeof(struct sockaddr_in));
        addr_len = sizeof(struct sockaddr_in);
        sockfd_cmd_connect = accept(sockfd_cmd,(struct sockaddr *)(&addr_cmd_client),&addr_len);
        if(sockfd_cmd_connect < 0){
            fprintf(stderr,"Accept error:%s\n\a",strerror(errno));
            goto exit_t;
        }else
            fprintf(stdout,"Server get connection from %s\n",
                        inet_ntoa(addr_cmd_client.sin_addr));

        /*修改为UDP的端口号 */
        addr_cmd_client.sin_port = htons(SERVER_PORT_NUM);

        bzero(&addr,sizeof(struct sockaddr_in));
        do{
            while(!gDone) {  //等待下一帧的数据请求
                //                       fprintf(stdout,"wait cmd  1.\n");
                /*
                bzero(&addr_cmd_client,sizeof(struct sockaddr_in));
                sockfd_cmd_connect = accept(sockfd_cmd,(struct sockaddr *)(&addr_cmd_client),&addr_len);
                if(sockfd_cmd_connect < 0){
                    fprintf(stderr,"Accept error:%s\n\a",strerror(errno));
                    goto exit_t;
                }else
                    fprintf(stdout,"Server get connection from %s\n",
                            inet_ntoa(addr_cmd_client.sin_addr));
                */
                ret = read(sockfd_cmd_connect,buffer,10);
                if(ret > 0){
                    //                      printf("recv 0x%x .\n",*(UInt32 *)buffer);
                      if(*(Int32 *)buffer == NEXT_FRAME_CMD)
                          break;
                  }else
                    if(errno == EAGAIN)
                        fprintf(stderr,"EAGAIN...\n");
            }

            if(gDone){
                ret = SIGQUIT;
                goto exit_t0;
            }
            fprintf(stdout,"Recv NEXT_FRAME_CMD from %s.\n",inet_ntoa(addr_cmd_client.sin_addr));

            /* 读取本地文件数据 */
            ret = bitsRead_process(pbufinfo);
            if((ret != ERR_FILE_END) &&(ret < 0)){
                fprintf(stderr,"bitsRead_process error..\n");
                break;
            }            

            /*发送已读出的数据 */
            udp_send(sockfd,sockfd_cmd_connect,&addr_cmd_client,pbufinfo->pBuffer,pbufinfo->filledBufSize);

        }while(!gDone &&(ret == 0));

 exit_t0:
        sleep(1);
        if((ret == SIGQUIT)||(ret == ERR_FILE_END)){
            *(UInt32 *)buffer = ERR_FILE_END;
            //            ret = write(sockfd_cmd_connect,buffer,sizeof(UInt32));
            ret = sendto(sockfd,buffer,sizeof(UInt32 ),0,(struct sockaddr *)&addr,sizeof(struct sockaddr_in));
            if(ret == (sizeof(UInt32)))
                ret = 0;
            fprintf(stdout,"%s:Reach the end of file.\n",__func__);
        }
 exit_t:
        close(sockfd);
        close(sockfd_cmd_connect);
        close(sockfd_cmd);
        bitsRead_deinit(pbufinfo);
        fprintf(stdout,"EXIT %d:test run over..\n",ret);
        return ret;
}
