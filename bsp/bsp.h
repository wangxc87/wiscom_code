// =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  bsp.h
//
//    Description:
//
//        Version:  1.0
//        Created:  2014-07-24
//
//         Author:  jianhuifu
//
// =====================================================================================
#ifndef __BSP_H__
#define __BSP_H__

#include <errno.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



// ===  ERROR CODE  ===
// 错误号从 0xF100F001 开始
#define BSP_OK                  0x0

#define BSP_ERR_DEV_OPEN        0xF100F001
#define BSP_ERR_DEV_CLOSE       0xF100F002
#define BSP_ERR_WRITE           0xF100F003
#define BSP_ERR_READ            0xF100F004
#define BSP_ERR_DEV_CTRL        0xF100F005
#define BSP_ERR_SIGN            0xF100F006
#define BSP_ERR_CHECK           0xF100F007
#define BSP_ERR_ARG             0xF100F008
#define BSP_ERR_SET_TIME        0xF100F009
#define BSP_ERR_BUSY            0xF100F00A
#define BSP_ERR_TIMEOUT         0xF100F00B
#define BSP_ERR_SEEK            0xF100F00C
#define BSP_ERR_EXTERNAL        0xF100F00D

// BSP打印级别
typedef enum
{
    BSP_PRINT_LEVEL_NONE = 0,
    BSP_PRINT_LEVEL_ERROR,
    BSP_PRINT_LEVEL_IMPORTANT,
    BSP_PRINT_LEVEL_COMMON,
    BSP_PRINT_LEVEL_DEBUG,

    BSP_PRINT_LEVEL_MAX
}BSP_PRINT_LEVEL;

#define BSP_PRINT_MAX_LEN   1400  // BSP打印最大长度


// ===  FUNCTION  ======================================================================
//         Name:  BSP_LocalPrint
//  Description:  BSP本地打印
//  Input Args :  enLevel  打印级别
//                ps8File  文件名
//                u32Line  行号
//                ps8Format  格式化字符串
//                ...      可变参数表
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_LocalPrint(const BSP_PRINT_LEVEL enLevel,
                    const char *ps8File, const unsigned int u32Line,
                    const char *ps8Format, ...);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_PrintSetLevel
//  Description:  设置打印级别
//  Input Args :  enLevel  打印级别
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_PrintSetLevel(const BSP_PRINT_LEVEL enLevel);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_PrintGetLevel
//  Description:  获取打印级别
//  Input Args :
//  Output Args:
//  Return     :  打印级别
// =====================================================================================
extern BSP_PRINT_LEVEL BSP_PrintGetLevel(void);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_Print
//  Description:  BSP打印
//  Input Args :  u8Level   打印级别，数值大于设置级别将不打印
//                ps8Format 格式化字符串
//                args      参数表
// =====================================================================================
#define BSP_Print(u8Level, ps8Format, args...)    \
    do{   \
        BSP_LocalPrint(u8Level, __FILE__, __LINE__, ps8Format, ##args);      \
        if ( BSP_PRINT_LEVEL_ERROR != u8Level )   \
        {                                       \
            errno = 0;  /* 恢复错误号 */        \
        }                                       \
    }while(0)



// ===  FUNCTION  ======================================================================
//         Name:  BSP_Init
//  Description:  初始化BSP，调用其他库函数前必须调用此初始化函数
//  Input Args :
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_Init (void);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_Uninit
//  Description:  程序退出前必需调用此函数反初始化BSP
//  Input Args :
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_Uninit (void);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_EnableWatchdog
//  Description:  开启看门狗，使用看门狗必须先调用此函数
//  Input Args :
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_EnableWatchdog(void);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_DisableWatchdog
//  Description:  关闭看门狗，停用看门狗必须调用此函数
//  Input Args :
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_DisableWatchdog(void);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetWatchdogTimeout
//  Description:  获取看门狗超时时间
//  Input Args :
//  Output Args:  pu32Time  超时时间，单位秒
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_GetWatchdogTimeout(unsigned int * const pu32Time);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_SetWatchdogTimeout
//  Description:  设置看门狗超时时间
//  Input Args :
//  Output Args:  u32Timeout 超时时间，单位秒
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_SetWatchdogTimeout(const unsigned int u32Timeout);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_FeedWatchdog
//  Description:  喂狗
//  Input Args :
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_FeedWatchdog(void);


// ===  FUNCTION  ======================================================================
//         Name:  BSP_SetSystemTimeToRTC
//  Description:  保存系统时间到RTC
//  Input Args :
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_SetSystemTimeToRTC ( );


// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetTemperature
//  Description:  获取温度
//  Input Args :
//  Output Args:  pf32Temp  温度值，单位摄氏度，精确到小数点后3位
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_GetTemperature(float * const pf32Temp);


// ===  FUNCTION  ======================================================================
//         Name:  BSP_OpenEeprom
//  Description:  打开EEPROM，操作EEPROM前必须调用次函数
//  Input Args :
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_OpenEeprom(void);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_CloseEeprom
//  Description:  关闭EEPROM
//  Input Args :
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_CloseEeprom(void);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_ReadEeprom
//  Description:  读EEPROM
//  Input Args :  u16DataLen  读数据长度
//  Output Args:  pu8Data     读数据
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_ReadEeprom(unsigned char * const pu8Data, const unsigned short u16DataLen);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_WriteEeprom
//  Description:  写EEPROM
//  Input Args :  pu8Data  写数据     u16DataLen 写数据长度
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_WriteEeprom(const unsigned char * const pu8Data, const unsigned short u16DataLen);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_LseekEeprom
//  Description:  设置EEPROM读写位置
//  Input Args :  s16Offset  偏移量，可正可负（向前移，向后移）
//                s32Whence  起始位置，0（文件头），1（当前位置），2（文件尾）
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_LseekEeprom(const short s16Offset, const int s32Whence);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

// =====================================================================================
//    End of bsp.h
// =====================================================================================:

