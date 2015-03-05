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
struct udp_ackMsg {
    UInt32 frame_num;
    UInt16 send_num;
    UInt32 send_size;
};

Int8 msg[MAX_MSG_SIZE];

Int32 udps_process(Int32 sockfd ,Int32 fd)
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
        n=recvfrom(sockfd,msg,MAX_MSG_SIZE,0,  (struct sockaddr*)&addr,&addrlen);
        if(n > 0){
            prev_frame_size += n - DEFINE_HEAD_SIZE; //统计单帧的大小
            //            fprintf(stdout,"Recv EOF 0x%x.\n",*(Int8 *)(msg + 2));
            if(*(Int16 *)msg == 0)  //新的一帧数据到来
                frame_num++;   

            if(*(Int8 *)(msg + 2) == 0xff){ //判断时候为该buffer的最后一个报文
                cur_frame_size = prev_frame_size;
                prev_frame_size = 0;
                fprintf(stdout,"Recv Frame %d,size %d.\n",frame_num,cur_frame_size);
                new_frame = 1;  
            }

            ackbuf.frame_num = frame_num ;
            ackbuf.send_num = *(UInt16 *)msg;
            ackbuf.send_size = n;
            //            fprintf(stdout,"Recv Frame %d,msg_num %d,recv_size %d,%d.\n",ackbuf.frame_num,ackbuf.send_num,ackbuf.send_size,n);
        }else
            break;

        /*send to ack msg */
        sendto(sockfd,&ackbuf,sizeof(struct udp_ackMsg),0,(struct sockaddr*)&addr,addrlen);

        /* write to file */
        ret = write(fd,(msg+DEFINE_HEAD_SIZE),(n-DEFINE_HEAD_SIZE));
        if (ret != (n - DEFINE_HEAD_SIZE)){
            fprintf(stderr,"File write Error.\n");
            return -1;
        }
        /* send to cmd to require next frame  */
        if(new_frame == 1){
            usleep(1000);

            *(UInt32 *)msg = NEXT_FRAME_CMD;
            sendto(sockfd,msg,4,0,(struct sockaddr *)&addr,addrlen); //send to cmd to require next frame
        }

    }
    return 0;
}
Int32 main(void)
{
    Int32 sockfd,sockfd0;
    Int32 fd;
    Int32 lengh;
    struct sockaddr_in  addr,addr1;
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
#define SERVER_PORT     8880

    bzero(&addr,sizeof(struct sockaddr_in));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(SERVER_PORT);
    if(bind(sockfd,(struct sockaddr *)&addr,sizeof(struct sockaddr_in))<0)
    {
        fprintf(stderr,"Bind Error:%s\n",strerror(errno));
        exit(1);
    }


    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(8888);
    inet_aton("192.168.1.10",&addr1.sin_addr);
    int ret;
    /*
    char *tmp_buf = "this is server -> client test ....";
    while(1){
        
        ret = sendto(sockfd,tmp_buf,10,0,(struct sockaddr *)(&addr1),sizeof(struct sockaddr_in));
        if(ret > 0)
            printf("send num %d.\n",ret);
        else
            printf("send error.\n");
        usleep(50*1000);
        }
    */
    //    udps_respon(sockfd);

    /*  请求下一帧数据*/
    *(UInt32 *)msg = NEXT_FRAME_CMD;
    lengh = 4;
    ret = sendto(sockfd,msg,lengh,0,(struct sockaddr *)(&addr1),sizeof(struct sockaddr_in));
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
    udps_process(sockfd,fd);
error_exit:
    close(sockfd);
    close(fd);
    return ret;
}
