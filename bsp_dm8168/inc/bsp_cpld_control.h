// =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  cpld_control.h
//
//    Description:CPLD 控制接口函数
//
//        Version:  1.0
//        Created:  2014-09-09
//
//         Author:  xuecaiwang
//
// =====================================================================================
#ifndef __BSP_CPLD_CONTROL_H__
#define __BSP_CPLD_CONTROL_H__

#include "bsp_std.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PCB_VERSION_DECD_ID      (0x01)
#define FUN_VERSION_IVSA_ID      (0x01)  /*智能行为分析板*/
#define FUN_VERSION_HDCA_ID      (0x02)  /*高清解码板*/
#define FUN_VERSION_IVSD_ID      (0x03)  /*智能视频诊断板*/

#define PCB_VERSION_DECE_ID         (0x02)
#define FUN_VERSION_KDC9204EH_ID    (0x01)  /*四路高清解码器*/

#define PCB_VERSION_IDVR_ID        (0x03)
#define FUN_VERSION_KTM6102_ID     (0x01)  /*智能交通终端管理服务器*/
#define FUN_VERSION_KDVR1608_ID    (0x02)  /* DVR */

#define PCB_VERSION_IAMB_ID         (0x04)
#define FUN_VERSION_EPS6000DM_ID    (0x01)  /*智能视频分析终端*/

#define PCB_VERSION_IAMC_ID      (0x05)
#define FUN_VERSION_EPS6000DMG_ID   (0x01)  /*智能视频分析终端（无风扇）,名称调整为EPS6000EM*/
#define FUN_VERSION_KTM6202_ID      (0x02)  /*智能交通终端管理服务器*/
    
#define GET_BOARDID(pcb_id, fun_id)  ((pcb_id << 4) | fun_id)

    extern UInt8 gPCB_VersionID;
    extern UInt8 gFUNC_VersionID;
    extern char gBoardName[32];
    extern char gPcbName[32];

    /*设备复位命令*/
typedef enum
{
    BSP_CPLD_RESET_FPGA = 0,  //复位fpga
    BSP_CPLD_RESET_NETPHY,  //复位网络phy芯片
    BSP_CPLD_RESET_SII9022,    //复位显示芯片
    BSP_CPLD_RESET_ALL,        //复位整板器件
    
    BSP_CPLD_RESET_MAX,
}BSP_cpldResetDev;



/*
 ===  FUNCTION  ======================================================================
         Name:  BSP_cpldInit
  Description:  初始化cpld，cpld有关操作时应先执行该函数，被bsp_init()函数调用
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldInit(void);


/*
 ===  FUNCTION  ======================================================================
         Name:  BSP_cpldDenit
  Description:  关闭cpld，cpld操作结束后应调用该函数，被bsp_uninit()函数调用
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldDeinit(void);

/*
 ===  FUNCTION  ======================================================================
         Name:  BSP_cpldResetBoard
  Description:  通过cpld实现对器件复位功能，建议仅使用BSP_CPLD_RESET_ALL命令做整板复位
  Input Args :
              reset_cmd 复位命令,不同命令实现对不同设备复位，合法参数有
                       BSP_CPLD_RESET_FPGA
                      BSP_CPLD_RESET_MV88E1111,
                      BSP_CPLD_RESET_SII9022,
                      BSP_CPLD_RESET_ALL,
               dev_num  复位命令的第二级参数，实现同一类设备下单个设备的复位，仅部分命令支持该功能，如下：
                     BSP_CPLD_RESET_MV88E1111  0、1、2
                     BSP_CPLD_RESET_SII9022    0、1、2、3、4
                  该值为0，表示复位该类设备下的所有设备，建议该值为0                 
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldResetBoard(BSP_cpldResetDev reset_cmd,Int32 dev_num);

/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldEnableHwWatchdog
  Description:  打开硬件30s定时喂狗程序
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldEnableHwWatchdog(void);

/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldDisableHwWatchdog
  Description:  关闭硬件30s定时喂狗程序，此函数需要开机一启动立即执行，否则将自动复位重起
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldDisableHwWatchdog(void);    
/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldEnableSoftWatchdog
  Description:  打开cpu软件看门狗，需要软件主动喂狗
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldEnableSoftWatchdog(void);

/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldFeedSoftWatchdog
  Description:  cpu软件看门狗喂狗，每1.5s必须要喂一次狗
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldFeedSoftWatchdog(void);


/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldDisableSoftWatchdog
  Description:  disable cpu软件看门狗
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldDisableSoftWatchdog(void);


/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldShowVideoInputStatus
  Description:  DVR面板点灯函数，需轮询监测模拟video信号的输入，仅DVR支持,此函数需要在DVRRDK程序执行完后执行，否则会导致DVRRDK无视频输入
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldShowVideoInputStatus(void);

    
/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldReadSensor
  Description:  读入DVR传感器的cpld检测的输入状态，仅被BSP_GetSensorIn(bsp.h)使用
  Input Args :  无
  Output Args:  status带回读回的传感器输入值；board不支持功能时，返回非法值BSP_INVALID_VALUE
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldReadSensor(UInt32 *status);

/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldReadBoardId
  Description:  读出pcb的版本ID
  Input Args :  无
  Output Args:  board_id返回读出的版本id，PCBID：bit7～bit4  FUNCID：bit3～bit0
                物理单板类型	PCBID	功能单板类型	FUNCID
                    DECD	0001	    IVSA	0001
                              		    HDCA	0010
      DECE（四路高清解码器板）	0010	KDC9204EH	0001
                    iDVR	0011	KTM6102-F	0001
                            		KDVR1616E	0010
                     iAMB	0100	ESP6000DM	0001
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldReadBoardID(UInt32 *board_id);

/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldReadPowerDownSignal
  Description:  读入关机按下状态
  Input Args :  无
  Output Args:  powerStatus 带回关机按键按下状态，1标示按下 0表示未按下；board不支持功能时，返回非法值BSP_INVALID_VALUE
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldReadPowerDownSignal(UInt32 *powerStatus);


/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldReadUpdateSignal
  Description:  读入是否有版本升级请求
  Input Args :  无
  Output Args:  updateSignal 返回版本升级请求信号，1表示有版本升级请求 0表示无；；board不支持功能时，返回非法值BSP_INVALID_VALUE
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldReadUpdateSignal(UInt32 *updateSignal);


/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldGetPWMFanSpeed
  Description:  获取当前风扇转速
  Input Args :  无
  Output Args:  pwmSpeed返回转速值；board不支持功能时，返回非法值BSP_INVALID_VALUE
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldGetPWMFanSpeed(UInt32 *pwmSpeed);

/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldSetPWMFanSpeed
  Description:  设置当前风扇转速
  Input Args :  pwmSpeed为设置的转速值，值无效则自动设置为默认值0x80，有效值为0x10，0x20，0x30,0x40,0x50,0x60,0x70,0x80
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldSetPWMFanSpeed(UInt32 *pwmSpeed);


/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldShutdownPower
  Description:  通过cpld关闭电路板电源
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
    Int32 BSP_cpldShutdownPower(void);

/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldReadNetConfig
  Description:  读取硬件网络端口配置，仅DECD板支持该功能
  Input Args :  无
  Output Args:  net_conifg ,0: copper, 1: fiber；board不支持功能时，返回非法值BSP_INVALID_VALUE
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
  Date       :  2014.10.10
 =====================================================================================
*/
    Int32 BSP_cpldReadNetConfig(UInt32 *net_config);

/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldReadPosition
  Description:  读取板子框位、槽位号,仅DECD板支持该功能,从0开始计数
  Input Args :  无
  Output Args:  *subsys_pos 子系统框位号   *board_pos 板子的槽位号；board不支持功能时，返回非法值BSP_INVALID_VALUE
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
  Date       :  2014.10.15
 =====================================================================================
*/
    Int32 BSP_cpldReadPosition(UInt32 *subsys_pos,UInt32 *board_pos);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
