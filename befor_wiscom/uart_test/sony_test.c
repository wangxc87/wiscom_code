#include   <stdio.h>
#include   <stdlib.h>
#include   <string.h>   
#include   <unistd.h>   
#include   <fcntl.h>   
#include   <errno.h>   
#include   <termios.h>   
#include   <sys/time.h> 


#define UART_DEVICE0 "/dev/ttyO0"
#define UART_DEVICE1 "/dev/ttyO1"
#define UART_DEVICE2 "/dev/ttyO2"
#define UART_DEVICE3 "/dev/ttyO3"
#define UART_DEVICE4 "/dev/ttyO4"
#define UART_DEVICE5 "/dev/ttyO5"

char dev_name[30];
unsigned char charA[7] = {0x60,0x10,0x01,0x01,0x01,0x01,0x74};
//unsigned char charB[7] = {0x60,0x20,0x01,0x01,0x01,0x01,0x84};
unsigned char charB[7] = {'A','B','C','D','E','4',' '};
unsigned char charC[7] = {0x60,0x30,0x01,0x01,0x01,0x01,0x94};
unsigned char charD[7] = {0x60,0x40,0x01,0x01,0x01,0x01,0xA4};
unsigned char charE[7] = {0x60,0x50,0x01,0x01,0x01,0x01,0xB4};

int openport(char *Dev)
{
    int fd = open( Dev, O_RDWR|O_NOCTTY ); 
    if (-1 == fd) 
    {    
        perror("Can''t Open Serial Port");
        return -1;  
    } 
    else 
        return fd;
 }   

int setport(int fd, int baud,int databits,int stopbits,int parity)
{
    int baudrate;
    int status;
    struct termios newtio;   
    switch(baud)
    {
        case 300:
            baudrate=B300;
            break;
        case 600:
            baudrate=B600;
            break;
        case 1200:
            baudrate=B1200;
            break;
        case 2400:
            baudrate=B2400;
            break;
        case 4800:
            baudrate=B4800;
            break;
        case 9600:
            baudrate=B9600;
            break;
        case 19200:
            baudrate=B19200;
            break;
        case 38400:
            baudrate=B38400;
            break;
        default :
            baudrate=B9600;  
            break;
    }
    
if(status=tcgetattr(fd,&newtio) != 0)
	{
		printf("tcgetattr error=%d\n",status);
		return -1; 
	}

	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	newtio.c_oflag = 0;
	newtio.c_iflag = 0;
	newtio.c_cflag &= ~(CREAD|CLOCAL);//change by huangling
	newtio.c_cflag |= CREAD|CLOCAL; //change by huangling
	newtio.c_cflag &= ~CSIZE;
	printf("databits=%d\n",databits);     
   

  switch (databits) /*设置数据位数*/
    {   
        case 7:  
            newtio.c_cflag |= CS7; //7位数据位
            break;
        case 8:     
            newtio.c_cflag |= CS8; //8位数据位
            break;   
        default:    
            newtio.c_cflag |= CS8;
            break;    
     }
    
    switch (parity) //设置校验
    {   
        case 'n':
        case 'N':    
            newtio.c_cflag &= ~PARENB;   /* Clear parity enable */
            newtio.c_iflag &= ~INPCK;     /* Enable parity checking */ 
            break;  
        case 'o':   
        case 'O':     
            newtio.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/  
            newtio.c_iflag |= INPCK;             /* Disnable parity checking */ 
            break;  
        case 'e':  
        case 'E':   
            newtio.c_cflag |= PARENB;     /* Enable parity */    
            newtio.c_cflag &= ~PARODD;   /* 转换为偶效验*/     
            newtio.c_iflag |= INPCK;       /* Disnable parity checking */
            break;
        case 'S': 
        case 's':  /*as no parity*/   
            newtio.c_cflag &= ~PARENB;
            newtio.c_cflag &= ~CSTOPB;
            break;  
        default:   
            newtio.c_cflag &= ~PARENB;   /* Clear parity enable */
            newtio.c_iflag &= ~INPCK;     /* Enable parity checking */ 
            break;   
    } 
    
    switch (stopbits)//设置停止位
    {   
        case 1:    
            newtio.c_cflag &= ~CSTOPB;  //1
            break;  
        case 2:    
            newtio.c_cflag |= CSTOPB;  //2
            break;
        default:  
            newtio.c_cflag &= ~CSTOPB;  
            break;  
    } 
    //newtio.c_cc[VTIME] = 0;    
    //newtio.c_cc[VMIN] = 0; 
    //newtio.c_cflag   |=   (CLOCAL|CREAD);
    //newtio.c_oflag|=OPOST; 
    //newtio.c_iflag   &=~(IXON|IXOFF|IXANY);                     
    cfsetispeed(&newtio,baudrate);   
    cfsetospeed(&newtio,baudrate);   
    tcflush(fd,   TCIFLUSH); 
    if (tcsetattr(fd,TCSANOW,&newtio) != 0)   
    { 
        perror("SetupSerial 3");  
        return -1;  
    }  
    return 0;
 }

 int readport(int fd,char *buf,int len,int maxwaittime)//读数据，参数为串口，BUF，长度，超时时间
 {
     int no=0;int rc;int rcnum=len;
     struct timeval tv;
     fd_set readfd;
     tv.tv_sec=maxwaittime/1000;    //SECOND
     tv.tv_usec=maxwaittime%1000*1000;  //USECOND
     FD_ZERO(&readfd);
     FD_SET(fd,&readfd);
     rc=select(fd+1,&readfd,NULL,NULL,&tv);
     if(rc>0)
     {
         while(len)
         {
             rc=read(fd,&buf[no],1);
             if(rc>0)
              no=no+1;
             len=len-1;   
         }
         if(no!=rcnum)
                 return -1;      //如果收到的长度与期望长度不一样，返回-1

         return rcnum;           //收到长度与期望长度一样，返回长度
     }
     else
     {
         return -1;
     }
     return -1;
 }

 int writeport(int fd,char *buf,int len)  //发送数据
 {
     write(fd,buf,len);
 }

 void clearport(int fd)      //如果出现数据与规约不符合，可以调用这个函数来刷新串口读写数据
 {
     tcflush(fd,TCIOFLUSH);
 }

 void SendMsgToPortA(int iType)   
 {   
     int fd,rc,ret;   

     fd = openport("/dev/ttyO2");         //打开串口
     if(fd>0)
     {
         ret=setport(fd,9600,8,1,'n');  //设置串口，波特率，数据位，停止位，校验
         if(ret<0)
         {
             printf("Can't Set Serial Port!/n");
             exit(0);
         }
         printf ("\n");
         printf ("port set successful\n");
         rc = write(fd,charA,7);
         printf("transmit:%s/n",charA);
     }
     else
     {
         printf("Can't Open Serial Port!/n");
         exit(0);
     }

     close(fd);   
 } 

 void SendMsgToPortB(int iType)   
 {   
     int fd,rc,ret;   

     fd = openport(dev_name);         //打开串口
     if(fd>0)
     {
         ret=setport(fd,9600,8,1,'n');  //设置串口，波特率，数据位，停止位，校验
         if(ret<0)
         {
             printf("Can't Set Serial Port!\n");
             exit(0);
         }
         printf ("\n");
         printf ("port set successful\n");
         rc = write(fd,charB,7);
         printf("transmit:%s\n",charB); 
     }
     else
     {
         printf("Can't Open Serial Port!\n");
         exit(0);
     }

     close(fd);   
 } 
 int read_port(void)
 {
     int fd,rc,ret;
     char data[20];

     fd = openport(dev_name);         //打开串口
     if(fd>0)
     {
         ret=setport(fd,9600,8,1,'n');  //设置串口，波特率，数据位，停止位，校验
         if(ret<0)
         {
             printf("Can't Set Serial Port!\n");
             exit(0);
         }
         printf ("\n");
         printf ("port set successful\n");
 //        rc = write(fd,charB,7);
         rc = read(fd,data,20);
         if(rc >0)
                 printf("read:%s\n",data);
         else
                 return -1;

     }
     else
     {
         printf("Can't Open Serial Port!\n");
         exit(0);
     }

     close(fd);
     return 0;

 } 

 int cmd_write(int fd,char *cmd)
 {
         int rc,count;
         int loop =1;
         char *p;
         char data[20];
         count = 6;

         rc = write(fd, cmd,6);
         rc = readport(fd,data,3,10);
         if(rc >0){
                 printf("read rc is %d ",rc);
                 p =data;
                while(count--){
                        printf("read:0x%x\t",*(p++));
                }
        }
        else
                return -1;
        printf("\n");

/*        while(loop--){
                rc = read(fd,data,20);
                p =data;
               
                if(rc >0){
                        
                        printf("read rc is %d ",rc);
                        while(count--){
                                printf("read:0x%x\t",*(p++));
                        }
                }
                
                else
                        return -1;
                count = 6;
                printf("\n");
                
        }
*/
      return 0;
        

}

int main(int argc,char *argp[])
{
        int ret = 0;
        char dev_id = 2;
        char shortoptions[] = "n:m:";
        int c,index;
        int mode = 0;
        char AE_gc[] = {0x81,0x01,0x04,0x39,0x0b,0xff};
        char gc_dec[]= {0x81,0x01,0x04,0x0b,0x03,0xff};
        char bj_far[]= {0x81,0x01,0x04,0x07,0x33,0xff};
        char bj_near[]= {0x81,0x01,0x04,0x07,0x23,0xff};
        char bj_stop[] ={0x81,0x01,0x04,0x07,0x00,0xff};
        int fd,rc;           
        char data[20];
        strcpy(dev_name,UART_DEVICE2);
        printf("Uart device is %s\n",dev_name);
        printf("Mode is %d:(0 write :1 read)\n",mode);


        
        fd = openport(dev_name);         //打开串口
        if(fd>0){
                ret=setport(fd,9600,8,1,'n');  //设置串口，波特率，数据位，停止位，校验
                if(ret<0){
                        printf("Can't Set Serial Port!\n");
                        exit(0);
                }
                printf ("\n");
                printf ("port set successful\n");
        }
        else{
                printf("Can't Open Serial Port!\n");
                exit(0);
        }
        int count = 6;
        char *p;
        int loop =1;
        
        while(1){
                cmd_write(fd,bj_far);
                //   printf("\n");
                
                usleep(200);
                cmd_write(fd,bj_near);
                
                usleep(200);
                
                cmd_write(fd,bj_stop);
                printf("\n");
                
                usleep(200);
        }
        
        return 0;

}
