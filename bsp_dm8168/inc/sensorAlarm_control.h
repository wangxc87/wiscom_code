// =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  bsp_sensorAlarm_control.h
//
//       Description:iDVR 告警信号输入输出板级支持功能
//
//        Version:  1.0
//        Created:  2014-09-18
//
//         Author:  xuecaiwang
//
// =====================================================================================
#include "bsp_std.h"

//告警输入（通过光耦）
#define    SENSOR_IN8_GPO_23    23
#define    SENSOR_IN10_GPO_25   25
#define    SENSOR_IN11_GPO_26   26
#define    SENSOR_IN12_GPO_27   27
#define    SENSOR_IN13_GPO_28   28
#define    SENSOR_IN14_GPO_29   29
//告警输出（通过继电器）
#define   ALARM_OUT1    (1)
#define   ALARM_OUT2    (1 << 1)
#define   ALARM_OUT3    (1 << 2)
#define   ALARM_OUT4    (1 << 3)


/*
 ===  FUNCTION  ======================================================================
         Name:  BSP_sensorAlarmInit
  Description:  初始化sensorAlarm设备，操作sensorAlarm操作时应先执行该函数，被bsp_init()函数调用
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/

Int32 BSP_sensorAlarmInit(void);



/*
 ===  FUNCTION  ======================================================================
         Name:  BSP_sensorAlarmGetSensor
  Description:  读取dm8168检测到外部传感器信号的输入,仅被BSP_GetSensorIn(bsp.h)使用
  Input Args :  无
  Output Args:  sensor_in,返回当前传感器输入信号，仅低6bit有效，当前bit位为1，则表示有传感器信号输入。
                board不支持功能时，返回非法值BSP_INVALID_VALUE
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
Int32 BSP_sensorAlarmGetSensorIn(Int32 *sensor_in);

/*
 ===  FUNCTION  ======================================================================
         Name:  BSP_sensorAlarmGetAlarmOut
  Description:  读取当前告警信号输出的状态,仅被BSP_GetAlarmOut（bsp.h）使用
  Input Args :  无
  Output Args:  指针型参数alarm_out，返回当前告警信号输出状态，低4bit有效；当前bit位为高，表示
                相应端口有告警信号输出；board不支持功能时，返回非法值BSP_INVALID_VALUE
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
Int32 BSP_sensorAlarmGetAlarmOut(Int32 *alarm_out);


/*
 ===  FUNCTION  ======================================================================
         Name:  BSP_sensorAlarmSetAlarmOut
  Description:  设置告警信号输出的,仅被BSP_SetAlarmOut（bsp.h）使用
  Input Args :  指针型参数alarm_out，当前告警信号输出状态，低4bit有效；每个bit位表示一个告警信号输出，
                1：表示有告警信号输出， 0：表示无告警信号输出
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
Int32 BSP_sensorAlarmSetAlarmOut(Int32 *alarm_out);


/*
 ===  FUNCTION  ======================================================================
         Name:  BSP_sensorAlarmDeInit
  Description:  关闭sensorAlarm 设备
  Input Args :  无
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
 =====================================================================================
*/
Int32 BSP_sensorAlarmDeInit(void);
