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

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <stdbool.h>

#include "bsp_std.h"
#include "bsp_cpld_control.h"
#include "sensorAlarm_control.h"
#include "bsp_phydev.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define BSP_MAX_MAC             2
#define BSP_MAC_LENGTH          17
#define BSP_EEPROM_SIZE         0x1000
#define BSP_EEPROM_VER          0x1
#define BSP_EEPROM_SIGN         0x1F0C92BD

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

#define BSP_INVALID_VALUE (0xffffffff)
#define ALARM_OUT_MAX_NUM 4
#define BSP_LED_ON  1
#define BSP_LED_OFF  0

#define BSP_UART485_NUM_MAX 2
#define BSP_UART232_NUM_MAX 2
    struct BSP_boardInfo {
        int sensorIn_num;//当前board 输入IO的个数
        int alarmOut_num;//当前board 输出IO的个数
        int netPort_num;//网口个数
        int uart485_num;//当前board的uart485个数
        int uart232_num;//当前board的uart232个数
        char uart485_dev[BSP_UART485_NUM_MAX][32];//当前board的uart485设备名称
        char uart232_dev[BSP_UART232_NUM_MAX][32];//当前board uart232设备名称
    };
    //board 外接的端口信息
extern  struct BSP_boardInfo gBSP_boardInfo;

    
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
//  Description:  获取看门狗超时时间 调用过 BSP_EnableWatchdog 才能调用此函数
//  Input Args :
//  Output Args:  pu32Time  超时时间，单位秒
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_GetWatchdogTimeout(unsigned int * const pu32Time);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_SetWatchdogTimeout
//  Description:  设置看门狗超时时间 调用过 BSP_EnableWatchdog 才能调用此函数
//  Input Args :
//  Output Args:  u32Timeout 超时时间，单位秒
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_SetWatchdogTimeout(const unsigned int u32Timeout);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_FeedWatchdog
//  Description:  喂狗 调用过 BSP_EnableWatchdog 才能调用此函数
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
extern int BSP_SetSystemTimeToRTC(void);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetCPUTemperature
//  Description:  获取温度
//  Input Args :
//  Output Args:  pf32Temp  温度值，单位摄氏度，精确到小数点后3位
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_GetCPUTemperature(float * const pf32Temp);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetEthMAC
//  Description:  获取MAC地址
//  Input Args :
//  Output Args:  ps8MAC 存放MAC地址; pu32MACNum 存放MAC地址数量
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_GetEthMAC ( char * const ps8MAC, unsigned int * const pu32MACNum );

// ===  FUNCTION  ======================================================================
//         Name:  BSP_SetEthMAC
//  Description:  设置MAC地址
//  Input Args :  ps8MAC 存放MAC地址; u32MACNum 存放MAC地址数量
//  Output Args:
//  Return     :  成功时返回BSP_OK；错误时返回其他错误码
// =====================================================================================
extern int BSP_SetEthMAC ( char * const ps8MAC, const unsigned int u32MACNum );

// ===  FUNCTION  ======================================================================
//         Name:  BSP_Reboot
//  Description:  reboot system safely
//  Input Args :
//  Output Args:
//  Return     :  BSP_ERR_BUSY: device busy, system is not permitted to reboot
//                BSP_OK: reboot system
// =====================================================================================
extern int BSP_Reboot ( );

// ===  FUNCTION  ======================================================================
//         Name:  BSP_Halt
//  Description:  halt system safely
//  Input Args :
//  Output Args:
//  Return     :  BSP_ERR_BUSY: device busy, system is not permitted to reboot
//                BSP_OK: reboot system
// =====================================================================================
extern int BSP_Halt ( );

// ===  FUNCTION  ======================================================================
//         Name:  BSP_SetRebootFlag
//  Description:  set reboot flag to 1 when reboot or halt should be disable
//  Input Args :  1: disable reboot or halt
//                0: enable reboot and halt
//  Output Args:
//  Return     :
// =====================================================================================
extern void BSP_SetRebootFlag ( int s32Flag );

// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetRebootFlag
//  Description:  get reboot flag to 1 when reboot or halt should be disable
//  Input Args :
//  Output Args:
//  Return     :  1: disable reboot or halt
//                0: enable reboot and halt
// =====================================================================================
extern int BSP_GetRebootFlag ( );

// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetSensorIn
//  Description:  获取外部sensor输入的报警信号
//  Input Args :
//  Output Args:  低16bit（iDVR）/4bit（DECE）有效，bit位为1表示有信号输入，非法值为BSP_INVALID_VALUE (~0)
//  Return     :  BSP 错误馬
// =====================================================================================
    int BSP_GetSensorIn(int *sensor_in);

// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetAlarmOut
//  Description:  读取当前设置的报警输出的状态
//  Input Args :
//  Output Args:  低4bit 有效，bit为1 表示报警信号输出，非法值为BSP_INVALID_VALUE (~0)
//  Return     :  BSP 错误馬
// =====================================================================================
    int  BSP_GetAlarmOut(int *alarm_out);


// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetAlarmOut
//  Description:  设置当前设置的报警输出的状态
//  Input Args :   低4bit 有效，bit为1 表示报警信号输出，0表示无报警信号输出
//  Output Args:  
//  Return     :  BSP 错误馬
// =====================================================================================
    int BSP_SetAlarmOut(int alarm_out);


    // ===  FUNCTION  ======================================================================
//         Name:  BSP_runLedSet
//  Description:  设置运行指示灯状态,调用一次状态更改一次
//  Input Args :   输入有效值为 BSP_LED_ON、BSP_LED_OFF
//  Output Args:  
//  Return     :  BSP 错误馬
//  Time       :2014-12-10
// =====================================================================================
    int BSP_runLedSet(int value);

    // ===  FUNCTION  ======================================================================
//         Name:  BSP_errLedSet
//  Description:  设置ERR指示灯状态，,调用一次状态更改一次
//  Input Args :   输入有效值为 BSP_LED_ON、BSP_LED_OFF
//  Output Args:  
//  Return     :  BSP 错误馬
//  Time       :  2014-12-10
// =====================================================================================
    int BSP_errLedSet(int value);
    
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

// =====================================================================================
//    End of bsp.h
// =====================================================================================:

