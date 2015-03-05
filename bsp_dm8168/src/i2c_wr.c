#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "bsp.h"
#include "i2c_wr.h"

Int32 i2cOpen(Char *devname)
{
    Int32 fd;
    fd = open(devname,O_RDONLY);
    if(fd < 0){
#ifdef   BSP_I2C_DEBUG     
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"open %s ERROR.\n",devname);
#endif
        return BSP_ERR_DEV_OPEN;
    }
    return fd;
}
Int32 i2cRead8(Int32 fd, UInt16 devAddr, UInt8 *reg, UInt8 *value, UInt32 count)
{
    Int32 i, j;
    Int32 status = 0;
    struct i2c_msg * msgs = NULL;
    struct i2c_rdwr_ioctl_data data;

    msgs = (struct i2c_msg *) malloc(sizeof(struct i2c_msg) * count * 2);

    if(msgs==NULL)
    {
#ifdef   BSP_I2C_DEBUG     
        BSP_Print(BSP_PRINT_LEVEL_ERROR," I2C (0x%02x): Malloc ERROR during Read !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
#endif
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
#ifdef BSP_I2C_DEBUG
        BSP_Print(BSP_PRINT_LEVEL_ERROR," I2C (0x%02x): Read ERROR !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
#endif
    }
    else
        status = BSP_OK;

    free(msgs);

    return status;
}

Int32 i2cWrite8(Int32 fd, UInt16 devAddr,  UInt8 *reg, UInt8 *value, UInt32 count)
{ 
    Int32 i,j;
    UInt8 *bufAddr;
    Int32 status = 0;

    struct i2c_msg * msgs = NULL;
    struct i2c_rdwr_ioctl_data data;

    msgs = (struct i2c_msg *) malloc(sizeof(struct i2c_msg) * count);

    if(msgs==NULL)
    {
#ifdef   BSP_I2C_DEBUG     
        BSP_Print(BSP_PRINT_LEVEL_ERROR," I2C (0x%02x): Malloc ERROR during Write !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
#endif
        return BSP_ERR_DEV_CTRL;
    }

    bufAddr = (UInt8 *) malloc(sizeof(UInt8) * count * 2);

    if(bufAddr == NULL)
    {
        free(msgs);
#ifdef   BSP_I2C_DEBUG     
        BSP_Print(BSP_PRINT_LEVEL_ERROR," I2C (0x%02x): Malloc ERROR during Write !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
#endif
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
#ifdef BSP_I2C_DEBUG
        BSP_Print(BSP_PRINT_LEVEL_ERROR," I2C (0x%02x): Write ERROR !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
#endif
    }
    else
        status = BSP_OK;

    free(msgs);
    free(bufAddr);

    return status;
}

/*direct write data*/
Int32 i2cRawWrite8(Int32 fd, UInt16 devAddr, UInt8 *value, UInt32 count)
{
    Int32 status = 0;

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
#ifdef BSP_I2C_DEBUG
        BSP_Print(BSP_PRINT_LEVEL_ERROR," I2C (0x%02x): Raw Write ERROR !!! (count = %d)\n", devAddr, count);
#endif
    }
    else
        status = BSP_OK;

    return status;
}

/*direct read */
Int32 i2cRawRead8(Int32 fd, UInt16 devAddr, UInt8 *value, UInt32 count)
{
    Int32 status = 0;

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
#ifdef BSP_I2C_DEBUG
        BSP_Print(BSP_PRINT_LEVEL_ERROR," I2C (0x%02x): Raw Read ERROR !!! count = %d)\n", devAddr, count);
#endif
    }
    else
        status = BSP_OK;

    return status;
}

Int32 i2cClose(Int32 fd)
{
    close(fd);
    return BSP_OK;
}
