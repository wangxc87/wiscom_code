#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "bsp.h"

#define I2C0_DEVICE_NAME "/dev/i2c-1"
#define I2C1_DEVICE_NAME "/dev/i2c-2"
#define CPLD_SLAVE_DEVADDR 0x76
#define CPLD_STATUS_REG 0x02

#define TVP5158_SLAVE0_DEVADDR 0x5c
#define TVP5158_SLAVE1_DEVADDR 0x5d
#define TVP5158_SLAVE2_DEVADDR 0x5e
#define TVP5158_SLAVE3_DEVADDR 0x5f


#define Uint8 unsigned char
#define Uint16 unsigned short
#define Uint32 unsigned int
#define Int32 int
#define Int16 short
#define Int8 char
int i2cOpen(char *devname)
{
    int fd;
    fd = open(devname,O_RDONLY);
    if(fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"open %s ERROR.\n",devname);
        return BSP_ERR_DEV_OPEN;
    }
    return fd;
}
int i2cRead8(int fd, Uint16 devAddr, Uint8 *reg, Uint8 *value, Uint32 count)
{
    int i, j;
    int status = 0;
    struct i2c_msg * msgs = NULL;
    struct i2c_rdwr_ioctl_data data;

    msgs = (struct i2c_msg *) malloc(sizeof(struct i2c_msg) * count * 2);

    if(msgs==NULL)
    {
        BSP_Print(BSP_PRINT_LEVEL_DEBUG," I2C (0x%02x): Malloc ERROR during Read !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        return BSP_ERR_ARG;
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
        status = BSP_ERR_DEV_CTRL;
        #ifdef OSA_I2C_DEBUG
        BSP_Print(BSP_PRINT_LEVEL_DEBUG," I2C (0x%02x): Read ERROR !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        #endif
    }
    else
        status = BSP_OK;

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
        BSP_Print(BSP_PRINT_LEVEL_COMMON," I2C (0x%02x): Malloc ERROR during Write !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        return BSP_ERR_DEV_CTRL;
    }

    bufAddr = (unsigned char *) malloc(sizeof(unsigned char) * count * 2);

    if(bufAddr == NULL)
    {
        free(msgs);

        BSP_Print(BSP_PRINT_LEVEL_COMMON," I2C (0x%02x): Malloc ERROR during Write !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        return BSP_ERR_ARG;

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
        status = BSP_ERR_DEV_CTRL;
        #ifdef OSA_I2C_DEBUG
        BSP_Print(BSP_PRINT_LEVEL_COMMON," I2C (0x%02x): Write ERROR !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        #endif
    }
    else
        status = BSP_OK;

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
        status = BSP_ERR_DEV_CTRL;
        #ifdef OSA_I2C_DEBUG
        BSP_Print(BSP_PRINT_LEVEL_COMMON," I2C (0x%02x): Raw Write ERROR !!! (count = %d)\n", devAddr, count);
        #endif
    }
    else
        status = BSP_OK;

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
        status = BSP_ERR_DEV_CTRL;
        #ifdef OSA_I2C_DEBUG
        BSP_Print(BSP_PRINT_LEVEL_COMMON," I2C (0x%02x): Raw Read ERROR !!! count = %d)\n", devAddr, count);
        #endif
    }
    else
        status = BSP_OK;

    return status;
}

void msleep(int time)
{
        long temp;
        //        temp = time * 1000;
        usleep(time*1000);            
}

 struct tvp5158_dev {
        Uint32 id;
        Int32 fd_i2c;
        Uint8 slave_addr;
        Uint8 signal_status[4];
    };

#define TVP5158_DEVICE_MAX_NUM  4

struct tvp5158_dev gTvp5158_device[TVP5158_DEVICE_MAX_NUM];
Uint8 tvp5158_slave_addr[TVP5158_DEVICE_MAX_NUM] = {0x5c,0x5d,0x5e,0x5f };

Int32 Tvp5158_init(void)
{
    Int32 fd;
    Int32 i;
    fd = open(I2C1_DEVICE_NAME,O_RDONLY);  //tvp5158 use i2c1 bus
    if(fd < 0)
        return BSP_ERR_DEV_OPEN;
    for(i = 0;i < TVP5158_DEVICE_MAX_NUM;i++){
        gTvp5158_device[i].fd_i2c = fd;
        gTvp5158_device[i].slave_addr = tvp5158_slave_addr[i];
        gTvp5158_device[i].id = i;
    }
    return BSP_OK;
}
Int32 Tvp5158_getStatus(Uint16 *status)
{
    Int32 ret,i,j;
    Uint16 temp,temp1;
    Uint8 buf[4],lengh;

    temp = 0;

    /*configure tvp5158 work mode*/
    for(i = 0;i < TVP5158_DEVICE_MAX_NUM; i++){
        buf[0] = 0xff; //read config reg
        buf[1] = 0x1f;//auto decoder inc,en
        lengh = 2;
        ret = i2cRawWrite8(gTvp5158_device[i].fd_i2c,gTvp5158_device[i].slave_addr,buf,lengh);
        if(ret != BSP_OK)
            return BSP_ERR_WRITE;
        msleep(10);
    }

    /* read signal status */
    for(i = 0;i < TVP5158_DEVICE_MAX_NUM;i ++){
        buf[0] = 0x01;
        lengh = 1;
        ret = i2cRawWrite8(gTvp5158_device[i].fd_i2c,gTvp5158_device[i].slave_addr,buf,lengh);
        if(ret != BSP_OK)
            return BSP_ERR_WRITE;
        msleep(10);
        lengh = 4;
        ret = i2cRawRead8(gTvp5158_device[i].fd_i2c,gTvp5158_device[i].slave_addr,gTvp5158_device[i].signal_status,4);
        if(ret != BSP_OK)
            return BSP_ERR_READ;
        msleep(10);
        temp1 = 0;
        for(j =0;j < 4;j++){
            temp1 |= (gTvp5158_device[i].signal_status[j] & 0x80) >>(7 - j);
        }
        temp |= (temp1 << (i * 4));
    }
    *status = temp;
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"read signal status is 0x%04x.\n",temp);
    return BSP_OK;
        
}

Int32 Tvp5158_deinit(void)
{
    int ret;
    ret = close(gTvp5158_device[0].fd_i2c);
    if(ret < 0)
        return BSP_ERR_DEV_CLOSE;
    return BSP_OK;
}

int main(int argc,char **argv)
{
    int fd0,ret,i;
    int devaddr = 0x39;
    char buf[8];
    int debug_level ;
    long count = 0;
    Uint16 status = 0;
    fprintf(stdout,"INFO:rd_tvp5158_status 4 enable debug mode.\n");
    if(argc < 2)
        debug_level = BSP_PRINT_LEVEL_COMMON; // level 3
    else
        debug_level = atoi(argv[1]);
    BSP_PrintSetLevel(debug_level);

    ret = Tvp5158_init();
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"tvp5158 init ERROR (0x%x).\n",ret);
        return -1;
    }

    fd0 = open(I2C0_DEVICE_NAME,O_RDONLY);
    if(fd0 < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Open %s ERROR.\n",I2C0_DEVICE_NAME);
        ret = -1;
        goto error_exit;
    }

    while(1){
        ret = Tvp5158_getStatus(&status);
        if(ret < 0){
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"Tvp5158 get status ERROR(0x%x)",ret);
            ret = -1;
            goto error_exit;
        }
#define CPLD_PRESENT
#define CPLD_LED_REGL 0x00
#define CPLD_LED_REGH 0x01
        
#ifdef CPLD_PRESENT
        for(i = 0;i < 2;i ++){
#if 1   //test OK
            buf[0] = CPLD_LED_REGL + i;  //cpld仅支持单字节读写
            buf[1] = status >> (8 * i);
            ret = i2cRawWrite8(fd0,CPLD_SLAVE_DEVADDR,buf,2);
            if(ret != BSP_OK){
                BSP_Print(BSP_PRINT_LEVEL_ERROR,"CPLD Write ERROR(0x%x)",ret);
                break;
            }
            msleep(10);
#else
            buf[0] = CPLD_LED_REGL + i;
            buf[1] = status >> (8 * i);
            ret = i2cWrite8(fd0,CPLD_SLAVE_DEVADDR,buf,buf+1,1);
            if(ret != BSP_OK){
                BSP_Print(BSP_PRINT_LEVEL_ERROR,"CPLD Write ERROR(0x%x)",ret);
                break;
            }
            msleep(10);
            
#endif
        }
#endif
        msleep(100);
    }
    
error_exit:
    Tvp5158_deinit();
    return ret;    
}
 
