// =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  bsp_phydev.h
//
//       Description:config eth0 phydev media port
//
//        Version:  1.0
//        Created:  2014-10-10
//
//         Author:  xuecaiwang
//
// =====================================================================================
#include "bsp_std.h"

#define DEFINE_AUTO_SELECT  (0xf0)
#define DEFINE_COPPER_PORT  (0xf1)
#define DEFINE_FIBER_PORT   (0xf2)
/*
Int32 BSP_phydevInit(void);


Int32 BSP_phydevDeInit(void);

Int32 BSP_phydevRegWrite(Int32 phy_addr, Int32 reg_addr, Int32 reg_value);

Int32 BSP_phydevRegRead(Int32 phy_addr, Int32 reg_addr, Int32 *reg_value);
*/
/*
 ===  FUNCTION  ======================================================================
         Name:  BSP_netPortConfig
  Description:  读取当前硬件配置，设置eth0 phy 工作接口，需要轮询读取硬件的配置设置，获取配置函数 BSP_cpldReadNetConfig()
  Input Args :  定义当前工作模式 ,DEFINE_AUTO_SELECT, DEFINE_COPPER_PORT,DEFINE_FIBER_PORT
  Output Args:  无
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
  Date       :  2014.10.10
 =====================================================================================
*/
Int32 BSP_netPortConfig(Int32 port);

