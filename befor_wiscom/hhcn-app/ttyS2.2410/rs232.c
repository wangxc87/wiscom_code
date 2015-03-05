/*
** File: rs232.c
**
** Description:
**      Provides an RS-232 interface that is very similar to the CVI provided
**      interface library
*/
#include <stdio.h>
#include <assert.h>
#include "rs232.h" 
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#define DEBUG

struct PortInfo ports[MAX_PORTS];

/*
** Function: OpenCom
**
** Description:
**    Opens a serial port with default parameters
**
** Arguments:
**    portNo - handle used for further access
**    deviceName - the name of the device to open
**
** Returns:
**    -1 on failure
*/
int OpenCom(int portNo, const char deviceName[],long baudRate)
{
    return OpenComConfig(portNo, deviceName, baudRate, 1, 8, 1, 0, 0);
}

/*

*/
long GetBaudRate(long baudRate)
{
    long BaudR;
    switch(baudRate)
    {
	case 115200:
		BaudR=B115200;
		break;
	case 57600:
		BaudR=B57600;
		break;
	case 19200:
		BaudR=B19200;
		break;
	case 9600:
		BaudR=B9600;
		break;
	default:
		BaudR=B0;
    }
    return BaudR;
}

/*
** Function: OpenComConfig
**
** Description:
**    Opens a serial port with the specified parameters
**
** Arguments:
**    portNo - handle used for further access
**    deviceName - the name of the device to open
**    baudRate - rate to open (57600 for example)
**    parity - 0 for no parity
**    dataBits - 7 or 8
**    stopBits - 1 or 2
**    iqSize - ignored
**    oqSize - ignored
**
** Returns:
**    -1 on failure
**
** Limitations:
**    parity, stopBits, iqSize, and oqSize are ignored
*/
int OpenComConfig(int port,
                  const char deviceName[],
                  long baudRate,
                  int parity,
                  int dataBits,
                  int stopBits,
                  int iqSize,
                  int oqSize)
{
    struct termios newtio;
    long BaudR;

    ports[port].busy = 1;
    strcpy(ports[port].name,deviceName);
    if ((ports[port].handle = open(deviceName, O_RDWR, 0666)) == -1)
    {
        perror("open");
       // assert(0);
        return -1;
    }
    
    /* set the port to raw I/O */
    newtio.c_cflag = CS8 | CLOCAL | CREAD ;
    newtio.c_iflag = IGNPAR;
//    newtio.c_oflag = 0;
//    newtio.c_lflag = 0;
    newtio.c_oflag = ~OPOST;
    newtio.c_lflag = ~(ICANON | ECHO | ECHOE | ISIG);

    newtio.c_cc[VINTR]    = 0;
    newtio.c_cc[VQUIT]    = 0;
    newtio.c_cc[VERASE]   = 0;
    newtio.c_cc[VKILL]    = 0;
    newtio.c_cc[VEOF]     = 4;
    newtio.c_cc[VTIME]    = 0;
    newtio.c_cc[VMIN]     = 1;
    newtio.c_cc[VSWTC]    = 0;
    newtio.c_cc[VSTART]   = 0;
    newtio.c_cc[VSTOP]    = 0;
    newtio.c_cc[VSUSP]    = 0;
    newtio.c_cc[VEOL]     = 0;
    newtio.c_cc[VREPRINT] = 0;
    newtio.c_cc[VDISCARD] = 0;
    newtio.c_cc[VWERASE]  = 0;
    newtio.c_cc[VLNEXT]   = 0;
    newtio.c_cc[VEOL2]    = 0;
    cfsetospeed(&newtio, GetBaudRate(baudRate));
    cfsetispeed(&newtio, GetBaudRate(baudRate));
    tcsetattr(ports[port].handle, TCSANOW, &newtio);
    return 0;
}

/*
** Function: CloseCom
**
** Description:
**    Closes a previously opened port
**
** Arguments:
**    The port handle to close
**
**    Returns:
**    -1 on failure
*/
int CloseCom(int portNo)
{
    if (ports[portNo].busy)
    {
        close(ports[portNo].handle);
        ports[portNo].busy = 0;
        return 0;
    }
    else
    {
        return -1;
    }
}

/*
** Function: ComRd
**
** Description:
**    Reads the specified number of bytes from the port.
**    Returns when these bytes have been read, or timeout occurs.
**
** Arguments:
**    portNo - the handle
**    buf - where to store the data
**    maxCnt - the maximum number of bytes to read
**
** Returns:
**    The actual number of bytes read
*/
int ComRd(int portNo, char buf[], int maxCnt,int Timeout)
{
    int actualRead = 0;
    fd_set rfds;
    struct timeval tv;
    int retval;

    if (!ports[portNo].busy)
    {
        assert(0);
    }

    /* camp on the port until data appears or 5 seconds have passed */
    FD_ZERO(&rfds);
    FD_SET(ports[portNo].handle, &rfds);
    tv.tv_sec = Timeout/1000;
    tv.tv_usec = (Timeout%1000)*1000;
    retval = select(16, &rfds, NULL, NULL, &tv);

    if (retval)
    {
        actualRead = read(ports[portNo].handle, buf, maxCnt);
    }
    
#ifdef DEBUG
	if(actualRead>0)
    {
        unsigned int i;
        for (i = 0; i < actualRead; ++i)
        {
            if ((buf[i] > 0x20) && (buf[i] < 0x7f))
            {
//                printf("<'%c'", buf[i]);
                  printf("%c",buf[i]);
            }
            else
            {
//                printf("<%02X", buf[i]);
                  printf("%02X",buf[i]);
            }
        }
	printf("\n");
    }
    fflush(stdout);
#endif /* DEBUG */

    return actualRead;
}

/*
** Function: ComWrt
**
** Description:
**    Writes out the specified bytes to the port
**
** Arguments:
**    portNo - the handle of the port
**    buf - the bytes to write
**    maxCnt - how many to write
**
** Returns:
**    The actual number of bytes written
*/
int ComWrt(int portNo, const char *buf, int maxCnt)
{
    int written;

    if (!ports[portNo].busy)
    {
        assert(0);
    }
#ifdef DEBUG
    {
        int i;
        for (i = 0; i < maxCnt; ++i)
        {
            if ((buf[i] > 0x20) && (buf[i] < 0x7f))
            {
//                printf(">'%c'", buf[i]);
                  printf("%c",buf[i]);
            }
            else
            {
//                printf(">%02X", buf[i]);
                  printf("%02X",buf[i]);
            }
        }
	printf("\n");
    }
    fflush(stdout);
#endif /* DEBUG */
    
    written = write(ports[portNo].handle, buf, maxCnt);
    return written;
}


