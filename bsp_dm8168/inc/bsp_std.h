// =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  bsp_std.h
//
//    Description:
//
//        Version:  1.0
//        Created:  2014-09-09
//
//         Author:  xuecaiwang
//
// =====================================================================================

#ifndef __BSP_STD_H__
#define __BSP_STD_H__

#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif    
#if 0
    typedef char             Char;
    typedef char             Int8;
    typedef unsigned char    UInt8;
    typedef short            Int16;
    typedef unsigned short   UInt16;
    typedef int              Int32;
    typedef unsigned int     UInt32;
    typedef long             Int64;
    typedef unsigned long    UInt64;
    typedef float            Float;
    typedef double           Double;
#else
#define Char     char
#define Int8     char       
#define UInt8    unsigned char
#define Int16    short
#define UInt16   unsigned short
#define Int32    int
#define UInt32   unsigned int
#define Int64    long
#define UInt64   unsigned long
#define Float    float
#define Double   double
    
#endif

#define TRUE  1
#define FALSE 0


#ifdef __cplusplus
}
#endif    

#endif
