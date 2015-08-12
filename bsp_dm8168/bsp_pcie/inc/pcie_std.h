// =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  bsp_std.h
//
//    Description:
//
//        Version:  1.0
//        Created:  2014-11-25
//
//         Author:  xuecaiwang
//
// =====================================================================================

#ifndef __PCIE_STD_H__
#define __PCIE_STD_H__

#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif
    
#if 1

typedef char              Char;
typedef unsigned char     UChar;
typedef short             Short;
typedef unsigned short    UShort;
typedef int               Int;
typedef unsigned int      UInt;
typedef long              Long;
typedef unsigned long     ULong;
typedef float             Float;
typedef double            Double;
typedef long double       LDouble;
typedef void              Void;


typedef unsigned short    Bool;
typedef void            * Ptr;       /* data pointer */
typedef char            * String;    /* null terminated string */


typedef int            *  IArg;
typedef unsigned int   *  UArg;
typedef char              Int8;
typedef short             Int16;
typedef int               Int32;

typedef unsigned char     UInt8;
typedef unsigned short    UInt16;
typedef unsigned int      UInt32;
typedef unsigned int      SizeT;
typedef unsigned char     Bits8;
typedef unsigned short    Bits16;
typedef UInt32            Bits32;
typedef unsigned long long UInt64;

#else
    
#define Char     char
#define Int8     char       
#define UInt8    unsigned char
#define Int16    short
#define UInt16   unsigned short
#define Int32    int
#define UInt32   unsigned int
#define Int64    long long
#define UInt64   unsigned long long
#define Float    float
#define Double   double
#endif

#define TRUE  1
#define FALSE 0

#ifdef __cplusplus
}
#endif    

#endif
