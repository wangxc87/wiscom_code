#ifndef __DS28CN01__DEF_H__
#define __DS28CN01__DEF_H__

typedef unsigned char   ds_u8;
typedef signed char     ds_s8;
typedef unsigned short  ds_u16;
typedef signed short    ds_s16;
typedef unsigned int    ds_u32;
typedef signed int      ds_s32;

typedef enum
{
    DS_FALSE    = 0,
    DS_TRUE     = 1,
}DS_BOOL;

#define DS_OK       0
#define DS_FAIL     (-1)

#endif
