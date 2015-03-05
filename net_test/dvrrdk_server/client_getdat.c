/*           ·þÎñ¶Ë³ÌÐò  server.c           */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>

#include "bitsRead.h"

#define MAX_MSG_SIZE    1030

#define UInt32 unsigned int
#define UInt16 unsigned short
#define UInt8 unsigned char
#define Int32 int
#define Int16 short
#define Int8  char

#define SAVE_FILE_NAME "./data.h264"
#define ACK_BUFFER_SIZE 10
#define DEFINE_HEAD_SIZE 3 /* 0-1:send_num,2-3: 0xff eof the buf */
#define NEXT_FRAME_CMD 0xff55ff55
#define END_FRAME_CMD  0xff55ff56
#define SERVER_PORT_NUM 8888
struct udp_ackMsg {
    UInt32 frame_num;
    UInt16 send_num;
    UInt32 send_size;
};

Int8 msg[MAX_MSG_SIZE];
static Int32 done = 0;

Int32 udps_process(Int32 sockfd ,Int32 sockfd_cmd,Int32 fd)
{
    struct sockaddr_in addr;
    UInt32 n,frame_num = 0;
    Int32 ret;
    socklen_t addrlen;
    struct udp_ackMsg ackbuf;
    static UInt32 prev_frame_size,cur_frame_size;
    UInt8 new_frame = 0;
    while(1){
        new_frame = 0;
        memset(msg, 0, sizeof(msg));
        addrlen = sizeof(struct sockaddr);
        n = recvfrom(sockfd,msg,MAX_MSG_SIZE,0,  (struct sockaddr*)&addr,&addrlen);
        if(n > 0){
            if(*(UInt32 *)msg == ERR_FILE_END) //收到数据结束
                break;
            prev_frame_size += n - DEFINE_HEAD_SIZE; //统计单帧的大小
            if(*(Int16 *)msg == 0)  //新的一帧数据到来
                frame_num++;   

            if(*(Int8 *)(msg + 2) == 0xff){ //判断时候为该buffer的最后一 个报文
                cur_frame_size = prev_frame_size;
                prev_frame_size = 0; 
                fprintf(stdout,"Recv Frame %d,msg_num %d,size %d.\n",frame_num,*(UInt16 *)msg,cur_frame_size);
                new_frame = 1;  
            }

            ackbuf.frame_num = frame_num ;
            ackbuf.send_num = *(UInt16 *)msg;
            ackbuf.send_size = n;

            /*send to ack msg */
            ret = write(sockfd_cmd,&ackbuf,sizeof(struct udp_ackMsg));
            //            fprintf(stdout,"Recv Frame %d,msg_num %d,recv_size %d,%d.\n",ackbuf.frame_num,ackbuf.send_num,ackbuf.send_size,n);
        }else
            break;

        /* write to file */
        ret = write(fd,(msg+DEFINE_HEAD_SIZE),(n-DEFINE_HEAD_SIZE));
        if (ret != (n - DEFINE_HEAD_SIZE)){
            fprintf(stderr,"File write Error.\n");
            return -1;
        }
        /* send to cmd to require next frame  */
        if(new_frame == 1){
            if(done)
                break;
            /*  等待帧结束命令*/
            do{
                ret = read(sockfd_cmd,msg,10);
                if(ret < 0)
                    fprintf(stderr,"read error..\n");
            }while(*(UInt32 *)msg != END_FRAME_CMD);
            *(UInt32 *)msg = NEXT_FRAME_CMD;
           ret = write(sockfd_cmd,msg,4); //send to cmd to require next frame
            
        }
    }
    return 0;
}


void signal_handle(int iSignNo)
{
    switch(iSignNo){
    case SIGINT:
        //        done = 1;
        fprintf(stdout,"SIGINT (Ctrl + c) capture..\n");
        break;
    case SIGQUIT:
        done = 1;
        fprintf(stdout,"SIGQUIT (Ctrl + \\)Capture....\n");
        break;
    default:
        break;
    }
}

Int32 main(void)
{
    Int32 sockfd,sockfd_cmd;
    Int32 fd;
    Int32 lengh;
    int ret;
    struct sockaddr_in  addr,addr1;

    signal(SIGQUIT,signal_handle);//ctrl+\ quit
    
    fd = open(SAVE_FILE_NAME,O_RDWR | O_CREAT);
    if(fd < 0){
        fprintf(stderr,"open file %s Error.\n",SAVE_FILE_NAME);
        return -1;
    }
    
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<0)
    {
        fprintf(stderr,"Socket Error:%s\n",strerror(errno));
        exit(1);
    }

    bzero(&addr,sizeof(struct sockaddr_in));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(SERVER_PORT_NUM);
    if(bind(sockfd,(struct sockaddr *)&addr,sizeof(struct sockaddr_in))<0)
    {
        fprintf(stderr,"Bind Error:%s\n",strerror(errno));
        exit(1);
    }


    sockfd_cmd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd_cmd == -1){
        fprintf(stderr,"Socket stream Error:%s\n",strerror(errno));
        ret = -1;
        goto error_exit;
    }
        
    bzero(&addr1,sizeof(struct sockaddr_in));
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(SERVER_PORT_NUM + 1);
    inet_aton("192.168.1.10",&addr1.sin_addr);
    ret = connect(sockfd_cmd,(struct sockaddr *)(&addr1),sizeof(struct sockaddr));
    if(ret < 0){
        fprintf(stderr,"Connet stream Error:%s\a\n",strerror(errno));
        goto error_exit;
    }

        /*  请求下一帧数据*/
    *(UInt32 *)msg = NEXT_FRAME_CMD;
    lengh = 4;
    ret = write(sockfd_cmd,msg,lengh);
    if(ret == lengh){
        printf("send num %d.\n",ret);
        ret = 0;
    }
    else {
        printf("send error.\n");
        ret = -1;
        goto error_exit;
    }
    //    ret = 
    ret = udps_process(sockfd,sockfd_cmd,fd);
error_exit:
    close(sockfd);
    close(fd);
    fprintf(stdout,"EXIT %d:%s exit ...\n",ret,__func__);
    return ret;
}
