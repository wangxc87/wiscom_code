#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define I2C_DEVICE_NAME "/dev/i2c-1"
#define I2C_SLAVE_DEVADDR 0x76

#define OSA_SOK 0
#define OSA_EFAIL -1
#define OSA_I2C_DEBUG

#define Uint8 unsigned char
#define Uint16 unsigned short
#define Uint32 unsigned int
int i2cRead8(int fd, Uint16 devAddr, Uint8 *reg, Uint8 *value, Uint32 count)
{
    int i, j;
    int status = 0;
    struct i2c_msg * msgs = NULL;
    struct i2c_rdwr_ioctl_data data;

    msgs = (struct i2c_msg *) malloc(sizeof(struct i2c_msg) * count * 2);

    if(msgs==NULL)
    {
        printf(" I2C (0x%02x): Malloc ERROR during Read !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        return OSA_EFAIL;
    }

    for (i = 0, j = 0; i < count * 2; i+=2, j++)
    {
        msgs[i].addr  = devAddr;
        msgs[i].flags = 0;
        msgs[i].len   = 1;
        msgs[i].buf   = &reg[j];

        msgs[i+1].addr  = devAddr;
        msgs[i+1].flags = I2C_M_RD /* | I2C_M_REV_DIR_ADDR */;
        msgs[i+1].len   = 1;
        msgs[i+1].buf   = &value[j];
    }

    data.msgs = msgs;
    data.nmsgs = count * 2;

    status = ioctl(fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
        #ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Read ERROR !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        #endif
    }
    else
        status = OSA_SOK;

    free(msgs);

    return status;
}

int i2cWrite8(int fd, Uint16 devAddr,  Uint8 *reg, Uint8 *value, Uint32 count)
{ 
    int i,j;
    unsigned char * bufAddr;
    int status = 0;

    struct i2c_msg * msgs = NULL;
    struct i2c_rdwr_ioctl_data data;

    msgs = (struct i2c_msg *) malloc(sizeof(struct i2c_msg) * count);

    if(msgs==NULL)
    {
        printf(" I2C (0x%02x): Malloc ERROR during Write !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        return OSA_EFAIL;
    }

    bufAddr = (unsigned char *) malloc(sizeof(unsigned char) * count * 2);

    if(bufAddr == NULL)
    {
        free(msgs);

        printf(" I2C (0x%02x): Malloc ERROR during Write !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        return OSA_EFAIL;

    }

    for (i = 0, j = 0; i < count; i++, j+=2)
    {
        bufAddr[j] = reg[i];
        bufAddr[j + 1] = value[i];

        msgs[i].addr  = devAddr;
        msgs[i].flags = 0;
        msgs[i].len   = 2;
        msgs[i].buf   = &bufAddr[j];
    }
    data.msgs = msgs;
    data.nmsgs = count;

    status = ioctl(fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
        #ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Write ERROR !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        #endif
    }
    else
        status = OSA_SOK;

    free(msgs);
    free(bufAddr);

    return status;
}

/*direct write data*/
int i2cRawWrite8(int fd, Uint16 devAddr, Uint8 *value, Uint32 count)
{
    int status = 0;

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data data;

    msgs[0].addr  = devAddr;
    msgs[0].flags = 0;
    msgs[0].len   = count;
    msgs[0].buf   = value;

    data.msgs = msgs;
    data.nmsgs = 1;

    status = ioctl(fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
        #ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Raw Write ERROR !!! (count = %d)\n", devAddr, count);
        #endif
    }
    else
        status = OSA_SOK;

    return status;
}

/*direct read */
int i2cRawRead8(int fd, Uint16 devAddr, Uint8 *value, Uint32 count)
{
    int status = 0;

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data data;

    msgs[0].addr  = devAddr;
    msgs[0].flags = I2C_M_RD;
    msgs[0].len   = count;
    msgs[0].buf   = value;

    data.msgs = msgs;
    data.nmsgs = 1;

    status = ioctl(fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
        #ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Raw Read ERROR !!! count = %d)\n", devAddr, count);
        #endif
    }
    else
        status = OSA_SOK;

    return status;
}
int open_watchdog(int fd)
{
    int ret;
    Uint8 regaddr,value;
    regaddr = 0x1;
    value = 0x03;
    ret = i2cWrite8(fd,I2C_SLAVE_DEVADDR,&regaddr,&value,1);//enable wathdog
    if(ret< 0)
        return -1;
    
}
int feed_watchdog(int fd)
{
    int ret;
    Uint8 regaddr,value;
    regaddr = 0x1;
    value = 0x02;
    ret = i2cWrite8(fd,I2C_SLAVE_DEVADDR,&regaddr,&value,1);//enable wathdog
    if(ret< 0)
        return -1;

    regaddr = 0x1;
    value = 0x03;
    ret = i2cWrite8(fd,I2C_SLAVE_DEVADDR,&regaddr,&value,1);//enable wathdog
    if(ret< 0)
        return -1;
    return 0;    
}
 void msleep(int time)
 {
        long temp;
        //        temp = time * 1000;
        usleep(time*1000);            
    }
int main(int argc,char **argv)
{
    int fd,ret;
    //    int devaddr = 0x39;
    char buf[8];
    int time ;
    long count = 0;
    if(argc < 2)
        time = 1000;
    else
        time = atoi(argv[1]);
    printf("Watchdog time is %d mS.\n",time);
    
    fd = open(I2C_DEVICE_NAME,O_RDWR);
    
    if(fd < 0)
        printf("Open device %s fail..\n",I2C_DEVICE_NAME);
    else
        printf ("Open device %s OK..\n",I2C_DEVICE_NAME);
    /*
      buf[0]= 0x00;
      ret = i2cRawWrite8(fd,devaddr,buf,1);
      if(ret < 0)
      printf("I2c device 0x%x write Error.\n",devaddr);
      ret = i2cRawRead8(fd,devaddr,buf,1);
      if(ret < 0 )
      printf ("I2c device 0x%x read Error.\n",devaddr);
      else
        printf ("I2c device read:0x%x\n",buf[0]);
    */
    while(1)        {
        feed_watchdog(fd);
        printf("Wathdog feeding times %d...\n",count++);
        msleep(time);
    }
    return 0;

}
