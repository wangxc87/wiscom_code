/**************serialport**************************/
/*************Designed by Suo*********************/
/*
  此程序实现的功能是：运行程序时，输入要发送的次数及待发送数据，就向串口2发送数据，如果有数据过了，程序会把接收到的数据显示在终端上。
  打开开发板系统中的终端Terminal，设置串口COM1属性为9600波特率，8N1，然后等待接收,输入./serialport 1 abcdefghijk ,即可运行此程序，程序向PC串口发送发送10次“abcdefghijk”，发送完成后，如果你向超级终端输入了数据，程序会把数据显示到Terminal上
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
//#include <

#define UART_DEVICE "/dev/ttyO2"
//#define UART_DEVICE "/dev/ttyO0"

//serial port set function
void setTermios(struct termios *pNewtio, unsigned short uBaudRate)
{
      bzero(pNewtio,sizeof(struct termios));
      pNewtio->c_cflag = uBaudRate|CS8|CREAD|CLOCAL;
      pNewtio->c_iflag = IGNPAR;
      pNewtio->c_oflag = 0;
      pNewtio->c_lflag = 0;
      pNewtio->c_cc[VINTR] = 0;
      pNewtio->c_cc[VQUIT] = 0;
      pNewtio->c_cc[VERASE] = 0;
      pNewtio->c_cc[VKILL] = 0;
      pNewtio->c_cc[VEOF] = 4;
      pNewtio->c_cc[VTIME] = 5;
      pNewtio->c_cc[VMIN] = 0;
      pNewtio->c_cc[VSWTC] = 0;
      pNewtio->c_cc[VSTART] = 0;
      pNewtio->c_cc[VSTOP] = 0;
      pNewtio->c_cc[VSUSP] = 0;
      pNewtio->c_cc[VEOL] = 0;
      pNewtio->c_cc[VREPRINT] = 0;
      pNewtio->c_cc[VDISCARD] = 0;
      pNewtio->c_cc[VWERASE] = 0;
      pNewtio->c_cc[VLNEXT] = 0;
      pNewtio->c_cc[VEOL2] = 0;
}

int main(int argc,char **argv)
{
      int fd;
      int nCount,nTotal;
      int i,j,m;
      int ReadByte = 0;
      struct termios oldtio,newtio;
      char *dev =UART_DEVICE; 
      char Buffer[1024];
      
      if((argc!=3)||(sscanf(argv[1],"%d",&nTotal)!=1))
      {
             printf("Usage:COMSend count datat!\n");
             return -1;
      }
  
      if((fd=open(dev,O_RDWR|O_NOCTTY|O_NDELAY))<0) //open serial COM2
      {
             printf("Can't open serial port!\n");
             return -1;
      }
      tcgetattr(fd,&oldtio);
      setTermios(&newtio,B115200);
      tcflush(fd,TCIFLUSH);

      memcpy(&newtio,&oldtio,sizeof(struct termios));
      
      tcsetattr(fd,TCSANOW,&newtio);
       
      //Send data
      for(i=0;i<nTotal;i++)
      {
             nCount = write(fd,argv[2],strlen(argv[2]));
             printf("send data OK!count=%d\n",nCount);
             sleep(1);
      }
     
      //receive data
      for(j=0;j<20;j++)
      {
             ReadByte = read(fd,Buffer,512);
             if(ReadByte>0)
             {
                    printf("readlength=%d\n",ReadByte);
                    Buffer[ReadByte]='\0';
                    printf("%s\n",Buffer);
                    sleep(3);
             }
             else printf("Read data failure times=%d\n",j);
      }     
      printf("Receive data finished!\n");
      tcsetattr(fd,TCSANOW,&oldtio);
      close(fd);
      return 0;
}
