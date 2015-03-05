// =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  i2c_wr.h
//
//    Description:
//
//        Version:  1.0
//        Created:  2014-09-09
//
//         Author:  xuecaiwang
//
// =====================================================================================

#ifndef __I2C_WR_H__
#define __I2C_WR_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "bsp_std.h"

#define I2C0_DEVICE_NAME "/dev/i2c-1"
#define I2C1_DEVICE_NAME "/dev/i2c-2"

    Int32 i2cOpen(Char *devname);

    Int32 i2cRead8(Int32 fd, UInt16 devAddr, UInt8 *reg, UInt8 *value, UInt32 count);

    Int32 i2cWrite8(Int32 fd, UInt16 devAddr,  UInt8 *reg, UInt8 *value, UInt32 count);

/*direct write data*/
    Int32 i2cRawWrite8(Int32 fd, UInt16 devAddr, UInt8 *value, UInt32 count);

/*direct read */
    Int32 i2cRawRead8(Int32 fd, UInt16 devAddr, UInt8 *value, UInt32 count);

    Int32 i2cClose(Int32 fd);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

// =====================================================================================
//    End of i2c_wr.h
// =====================================================================================:

