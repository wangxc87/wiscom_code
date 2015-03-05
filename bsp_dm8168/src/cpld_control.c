/*
 =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  cpld_control.c
//
//    Description:CPLD 控制接口函数
//
//        Version:  1.0
//        Created:  2014-09-09
//
//         Author:  xuecaiwang
//
// =====================================================================================
*/
#include <stdio.h>
#include "bsp.h"
#include "i2c_wr.h"
#include "bsp_cpld_control.h"

#define CPLD_SLAVE_DEVADDR                0x76
#define CPLD_CONTROL_RESET_REG            0x00
#define CPLD_CONTROL_WD_ENABLE_REG        0x01
#define CPLD_CONTROL_WD_FEED_REG          0x02
#define CPLD_CONTROL_LED_REGL             0x03
#define CPLD_CONTROL_LED_REGH             0x04
#define CPLD_CONTROL_SENSOR_REG0          0x05
#define CPLD_CONTROL_PCB_VERSION_REG      0x06
#define CPLD_CONTROL_SENSOR_REG1          0x07

//Only DECD support position_reg 
#define CPLD_CONTROL_POSITION_REG         0x07
#define CPLD_CONTROL_POWER_STATUS_REG     0x08
#define CPLD_CONTROL_VERION_UPDATE_REG    0x09
#define CPLD_CONTROL_PWMFAN_SPEED_REG     0x0A
#define CPLD_CONTROL_POWER_SHUTDOWN_REG   0x0B
#define CPLD_CONTROL_NETCONFIG_REG       (0x0C)

#define DEFINE_CPLD_POWERDOWN_SIGNAL     (0x5a)
#define DEFINE_CPLD_UPDATE_SIGNAL        (0x5a)
#define DEFINE_CPLD_PWDFAN_DEFUALT_SPEED (0x80)


#define PCB_VERSION_DEFAULT_ID   (PCB_VERSION_IDVR_ID)
#define FUN_VERSION_DEFAULT_ID   (FUN_VERSION_KTM6102_ID)

UInt8 gPCB_VersionID  = PCB_VERSION_DEFAULT_ID;
UInt8 gFUNC_VersionID = FUN_VERSION_DEFAULT_ID;
char gBoardName[32];
char gPcbName[32];

#define TVP5158_DEVICE_MAX_NUM  4
struct tvp5158_dev {
        UInt32 id;
        Int32 fd_i2c;
        UInt8 slave_addr;
        UInt8 signal_status[4];
    };

static Int32 gcpld_fd = -1;
static struct tvp5158_dev gTvp5158_device[TVP5158_DEVICE_MAX_NUM];
static UInt8 tvp5158_slave_addr[TVP5158_DEVICE_MAX_NUM] = {0x5c,0x5d,0x5e,0x5f };

struct BoardId_obj {
    UInt8 boardId;
    char boardName[32];
    char pcbName[32];
};

static struct BoardId_obj gBoardId_obj[] = {
    {        
        GET_BOARDID(PCB_VERSION_DECD_ID,FUN_VERSION_IVSA_ID),
        "IVSA",
        "DECD"
    },
    {
        GET_BOARDID(PCB_VERSION_DECD_ID, FUN_VERSION_HDCA_ID),
        "HDCA",
        "DECD"
    },
    {
        GET_BOARDID(PCB_VERSION_DECD_ID, FUN_VERSION_IVSD_ID),
        "IVSD",
        "DECD"
    },
    {
        GET_BOARDID(PCB_VERSION_DECE_ID, FUN_VERSION_KDC9204EH_ID),
        "KDC9204EH",
        "DECE"
    },
    {
        GET_BOARDID(PCB_VERSION_IDVR_ID, FUN_VERSION_KTM6102_ID),
        "KTM6102",
        "iDVR"
    },
    {
        GET_BOARDID(PCB_VERSION_IDVR_ID, FUN_VERSION_KDVR1608_ID),
        "KDV1608",
        "iDVR"
    },
    {
        GET_BOARDID(PCB_VERSION_IAMB_ID, FUN_VERSION_EPS6000DM_ID),
        "EPS6000DM",
        "IAMB"
    },
    {
        GET_BOARDID(PCB_VERSION_IAMC_ID, FUN_VERSION_EPS6000DMG_ID),
        "EPS6000EM",
        "IAMC"
    },
    {
        GET_BOARDID(PCB_VERSION_IAMC_ID, FUN_VERSION_KTM6202_ID),
        "KTM6202",
        "IAMC"
    }
};

static Int32 Tvp5158_init(void)
{
    Int32 fd;
    Int32 i;
    fd = i2cOpen(I2C1_DEVICE_NAME);  //tvp5158 use i2c1 bus
    if(fd < 0)
        return BSP_ERR_DEV_OPEN;
    for(i = 0;i < TVP5158_DEVICE_MAX_NUM;i++){
        gTvp5158_device[i].fd_i2c = fd;
        gTvp5158_device[i].slave_addr = tvp5158_slave_addr[i];
        gTvp5158_device[i].id = i;
    }
    return BSP_OK;
}
static Int32 Tvp5158_getStatus(UInt16 *status)
{
    Int32 ret,i,j;
    UInt16 temp,temp1;
    UInt8 buf[4],lengh;

    temp = 0;
    /*configure tvp5158 work mode*/
    for(i = 0;i < TVP5158_DEVICE_MAX_NUM; i++){
        buf[0] = 0xff; //read config reg
        buf[1] = 0x1f;//auto decoder inc,en
        lengh = 2;
        ret = i2cRawWrite8(gTvp5158_device[i].fd_i2c,gTvp5158_device[i].slave_addr,buf,lengh);
        if(ret != BSP_OK)
            return BSP_ERR_WRITE;
        //        msleep(10);
    }

    /* read signal status */
    for(i = 0;i < TVP5158_DEVICE_MAX_NUM;i ++){
        buf[0] = 0x01;
        lengh = 1;
        ret = i2cRawWrite8(gTvp5158_device[i].fd_i2c,gTvp5158_device[i].slave_addr,buf,lengh);
        if(ret != BSP_OK)
            return BSP_ERR_WRITE;
        //        msleep(10);
        lengh = 4;
        ret = i2cRawRead8(gTvp5158_device[i].fd_i2c,gTvp5158_device[i].slave_addr,gTvp5158_device[i].signal_status,4);
        if(ret != BSP_OK)
            return BSP_ERR_READ;
        //        msleep(10);
        temp1 = 0;
        for(j =0;j < 4;j++){
            temp1 |= (gTvp5158_device[i].signal_status[j] & 0x80) >>(7 - j);
        }
        temp |= (temp1 << (i * 4));
    }
    *status = temp;
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"read VideoInput status is 0x%04x.\n",temp);
    return BSP_OK;
}

static Int32 Tvp5158_deinit(void)
{
    int ret;
    ret = i2cClose(gTvp5158_device[0].fd_i2c);
    if(ret < 0)
        return BSP_ERR_DEV_CLOSE;
    return BSP_OK;
}

Int32 BSP_cpldInit(void)
{
    gcpld_fd = i2cOpen(I2C0_DEVICE_NAME);
    if(gcpld_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"CPLD init failed.\n");
        return BSP_ERR_DEV_OPEN;
    }
    return BSP_OK;
}

static Int32 cpld_write(Int8 reg,Int8 value)
{
    Int8 buf[2];
    Int32 ret = BSP_OK;
    buf[0] = reg;  //cpld仅支持单字节读写
    buf[1] = value;
/*
  ret = i2cRawWrite8(gcpld_fd,CPLD_SLAVE_DEVADDR,buf,2);
*/if(gcpld_fd > 0){
        ret = i2cWrite8(gcpld_fd,CPLD_SLAVE_DEVADDR,&reg,&value,1);
#if 0
        if(ret != BSP_OK){
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"CPLD Write ERROR(0x%x)",ret);
        }
#endif
        return ret;
    }else{
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"CPLD need to init firstly.\n");
        return BSP_ERR_WRITE;
    }
}

static Int32 cpld_read(Int8 reg,Int8 *value)
{
    Int32 ret = BSP_OK;
    if(gcpld_fd > 0){
        ret = i2cRead8(gcpld_fd,CPLD_SLAVE_DEVADDR,&reg,value,1);
#if 0
        if(ret != BSP_OK){
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"CPLD Read ERROR(0x%x)",ret);
        }
#endif
        return ret;
    }else{
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"CPLD need to init firstly.\n");
        return BSP_ERR_READ;
    }
}

Int32 BSP_cpldDeinit(void)
{
    i2cClose(gcpld_fd);
    return BSP_OK;
}
Int32 BSP_cpldResetBoard(BSP_cpldResetDev reset_cmd,Int32 dev_num)
{
    Int32 ret;
    Int8 tmp0,mask = 0;

    ret = cpld_read(CPLD_CONTROL_RESET_REG,&tmp0);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s senorReg0 Error .\n",__func__);
        return BSP_ERR_READ;
    }
    if( reset_cmd > BSP_CPLD_RESET_MAX){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s invalid reset_cmd.\n",__func__);
        return BSP_ERR_ARG;
    }

    switch (reset_cmd){
    case BSP_CPLD_RESET_FPGA:
        if((gPCB_VersionID != PCB_VERSION_DECD_ID) && (gPCB_VersionID != PCB_VERSION_DECE_ID)){
            BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
            return BSP_OK;
        }
        mask = 0x01;
        break;
    case BSP_CPLD_RESET_NETPHY:
        if((gPCB_VersionID != PCB_VERSION_IAMB_ID) && (gPCB_VersionID != PCB_VERSION_IAMC_ID) && (gPCB_VersionID != PCB_VERSION_DECD_ID)){
            BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);            
            return BSP_OK;
        }            
        if((dev_num < 0) || (dev_num > 2)){
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s invalid arg,dev_num should be 0 1 2.\n",__func__);
            return BSP_ERR_ARG;
        }
        if(! dev_num)
            mask = (1<< 1) | (1 << 2);
        else 
            mask = 1 << dev_num;
        break;
    case BSP_CPLD_RESET_SII9022:
        if((gPCB_VersionID != PCB_VERSION_DECE_ID) && (gPCB_VersionID != PCB_VERSION_DECD_ID)){
            BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);            
            return BSP_OK;
        }            
        if((dev_num > 4) || (dev_num < 0)){
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s invalid arg, dev_num should be 0 1 2 3 4.\n",__func__);
            return BSP_ERR_ARG;
        }
        if(! dev_num)
            mask = 0xf0;
        else
            mask = 1 << (dev_num + 3);
        break;
    case BSP_CPLD_RESET_ALL:
        mask = 1 << 3;
        break;
    default:
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Invalid reset_cmd.\n",__func__);
        break;
    }
    ret = cpld_write(CPLD_CONTROL_RESET_REG,(tmp0 & ~mask));
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s write Error.\n",__func__);
        return BSP_ERR_WRITE;
    }
    if(reset_cmd == BSP_CPLD_RESET_ALL)
        return ret;

    usleep(2000);
    
    ret = cpld_write(CPLD_CONTROL_RESET_REG,(tmp0 | mask));
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s write Error.\n",__func__);
        return BSP_ERR_WRITE;
    }
        
    return ret;
}
//enable 30s Hw watchdog
Int32 BSP_cpldEnableHwWatchdog(void)
{
    Int8 tmp;
    Int32 ret;
    ret = cpld_read(CPLD_CONTROL_WD_ENABLE_REG,&tmp);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s read Error.\n",__func__);
        return ret;
    }
    ret = cpld_write(CPLD_CONTROL_WD_ENABLE_REG,tmp | 0x80);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s write Error.\n",__func__);
        return ret;
    }
    return BSP_OK;
}
//disable 30s Hw watchdog
Int32 BSP_cpldDisableHwWatchdog(void)
{
    Int8 tmp;
    Int32 ret;
    ret = cpld_read(CPLD_CONTROL_WD_ENABLE_REG,&tmp);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s read Error.\n",__func__);
        return ret;
    }
    ret = cpld_write(CPLD_CONTROL_WD_ENABLE_REG,tmp & 0x7f);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s write Error.\n",__func__);
        return ret;
    }
    return BSP_OK;
}

//cpu software watchdog
Int32 BSP_cpldEnableSoftWatchdog(void)
{
    Int8 tmp;
    Int32 ret;
    ret = cpld_read(CPLD_CONTROL_WD_ENABLE_REG,&tmp);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s read Error.\n",__func__);
        return ret;
    }
    ret = cpld_write(CPLD_CONTROL_WD_ENABLE_REG,tmp & 0xf7);//bit3 0 disable cpld auto feed dog,need cpu to do
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s write Error.\n",__func__);
        return ret;
    }
    return BSP_OK;
}

/*feed watchdog once,every less 1.5s*/
Int32 BSP_cpldFeedSoftWatchdog(void)
{
    Int32 ret = BSP_OK;
    ret = cpld_write(CPLD_CONTROL_WD_FEED_REG,0xff);
    if(ret != BSP_OK)
        return BSP_ERR_WRITE;
    ret = cpld_write(CPLD_CONTROL_WD_FEED_REG,0x00);
    if(ret != BSP_OK)
        return BSP_ERR_WRITE;
    return BSP_OK;
}

Int32 BSP_cpldDisableSoftWatchdog(void)
{
    Int8 tmp;
    Int32 ret;
    ret = cpld_read(CPLD_CONTROL_WD_ENABLE_REG,&tmp);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s read Error.\n",__func__);
        return ret;
    }
    ret = cpld_write(CPLD_CONTROL_WD_ENABLE_REG,tmp | 0x08);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s write Error.\n",__func__);
        return ret;
    }
    return BSP_OK;
}

Int32 BSP_cpldShowVideoInputStatus(void)
{
    Int32 ret = BSP_OK;
    if(gPCB_VersionID != PCB_VERSION_IDVR_ID){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);            
        return BSP_OK;
    }
                
    UInt16 videoInput_status;
    ret = Tvp5158_init();
    if(ret != BSP_OK)
        return ret;
    ret = Tvp5158_getStatus(&videoInput_status);
    if(ret != BSP_OK)
        goto error_exit;
    ret = cpld_write(CPLD_CONTROL_LED_REGL,(UInt8 )videoInput_status);
    if(ret != BSP_OK)
        goto error_exit;
    ret = cpld_write(CPLD_CONTROL_LED_REGH,(UInt8 )(videoInput_status >>8));
    if(ret != BSP_OK)
        goto error_exit;
 error_exit:
    if(ret != BSP_OK)
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
    Tvp5158_deinit();
    return ret;
}

/*
  LSB 8:sensor_reg 0x05
  MSB 8:sensor_reg 0x07
  return :
 */
Int32 BSP_cpldReadSensor(UInt32 *status)
{
    Int32 ret;
    UInt8 tmp0,tmp1;
    if((gPCB_VersionID != PCB_VERSION_IDVR_ID) && (gPCB_VersionID != PCB_VERSION_DECE_ID) && (gPCB_VersionID != PCB_VERSION_IAMC_ID)){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);            
        *status = BSP_INVALID_VALUE;
        return BSP_OK;
     }

    ret = cpld_read(CPLD_CONTROL_SENSOR_REG0,&tmp0);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s senorReg0 Error .\n",__func__);
        return BSP_ERR_READ;
    }
    ret = cpld_read(CPLD_CONTROL_SENSOR_REG1,&tmp1);
    if(ret != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s senorReg1 Error .\n",__func__);
        return BSP_ERR_READ;
    }
    tmp0 = ~tmp0;//no input，Hw IO is high
    tmp1 = ~tmp1;
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[dbg] cpldRead sensor reg5 [0x%02x], reg7 [0x%02x].\n",tmp0,tmp1);
    *status = (tmp1 << 8)|tmp0;
    return BSP_OK;
}

    static Int32 getBoardName(UInt8 board_id, char *boardName, char *pcbName)
{
    struct BoardId_obj *pBoardId_obj;
    Int32 i, boardTpye_numMax;
    boardTpye_numMax = sizeof(gBoardId_obj)/sizeof(struct BoardId_obj);
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"Total boardType num is %d.\n",boardTpye_numMax);

    if(!boardName){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Error:boardName poiter is NULL.\n");
        return BSP_ERR_ARG;
    }
    for(i = 0;i < boardTpye_numMax; i ++){
        pBoardId_obj = &gBoardId_obj[i];
        if(pBoardId_obj->boardId == board_id){
            strcpy((char *)boardName, (char *)pBoardId_obj->boardName);
            strcpy((char *)pcbName,(char *)pBoardId_obj->pcbName);
            break;
        }
    }
    if(i == boardTpye_numMax){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Unknown board_id 0x%x.\n",board_id);
        return BSP_ERR_ARG;
    }
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"Get boardName is %s [0x%x],\n",boardName,board_id);
    return BSP_OK;
}

Int32 BSP_cpldReadBoardID(UInt32 *board_id)
{
    UInt8 version_value;
    strcpy((char *)gBoardName, "Unknown Board");
    strcpy((char *)gPcbName,"Unkown PCB");
    if(cpld_read(CPLD_CONTROL_PCB_VERSION_REG,&version_value) != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
        return BSP_ERR_READ;
    }
    if((version_value != 0)&& (version_value != 0xff)){
        gPCB_VersionID   = version_value >> 4;
        gFUNC_VersionID  = version_value & 0x0f;
    }
    if(board_id != NULL)
        *board_id = version_value;
    if(getBoardName(version_value, gBoardName, gPcbName) != BSP_OK)
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"getBoardName Error.\n");
    
    return BSP_OK;
}

/*
  arg:
      1:power down
      0:power up
      return: BSP ERR CODE 
*/
Int32 BSP_cpldReadPowerDownSignal(UInt32 *powerStatus)
{
    UInt8 tmp;
    if((gPCB_VersionID != PCB_VERSION_IDVR_ID) && (gPCB_VersionID != PCB_VERSION_IAMC_ID) && (gPCB_VersionID != PCB_VERSION_DECE_ID)){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
        *powerStatus = BSP_INVALID_VALUE;
        return BSP_OK;
    }

    if(cpld_read(CPLD_CONTROL_POWER_STATUS_REG,&tmp) != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
        return BSP_ERR_READ;
    }
    if(tmp != DEFINE_CPLD_POWERDOWN_SIGNAL)
        *powerStatus = FALSE;
    else
        *powerStatus = TRUE;
    return BSP_OK;
}
/*
  arg: 1:recieve update signal 0:no signal recieve
  return:BSP ERR CODE

 */
Int32 BSP_cpldReadUpdateSignal(UInt32 *updateSignal)
{
    UInt8 tmp;
    if(cpld_read(CPLD_CONTROL_VERION_UPDATE_REG,&tmp) != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
        return BSP_ERR_READ;
    }
    if(tmp != DEFINE_CPLD_UPDATE_SIGNAL)
        *updateSignal = FALSE;
    else
        *updateSignal = TRUE;

    //clear update flag
    if(cpld_write(CPLD_CONTROL_VERION_UPDATE_REG,0x0) != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
        return BSP_ERR_WRITE;
    }    
    return BSP_OK;
}

Int32 BSP_cpldGetPWMFanSpeed(UInt32 *pwdSpeed)
{
    UInt8 temp;
    if((gPCB_VersionID != PCB_VERSION_IDVR_ID) && (gPCB_VersionID != PCB_VERSION_IAMC_ID) && (gPCB_VersionID != PCB_VERSION_DECE_ID)){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);            
        *pwdSpeed = BSP_INVALID_VALUE;
        return BSP_OK;
    }

    if(cpld_read(CPLD_CONTROL_PWMFAN_SPEED_REG,&temp) != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
        return BSP_ERR_READ;
    }
    *pwdSpeed = temp;
    return BSP_OK;
}

Int32 BSP_cpldSetPWMFanSpeed(UInt32 *pwdSpeed)
{
    UInt8 temp;
    if((gPCB_VersionID != PCB_VERSION_IDVR_ID) && (gPCB_VersionID != PCB_VERSION_IAMC_ID) && (gPCB_VersionID != PCB_VERSION_DECE_ID)){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);            
        return BSP_OK;
    }

    if((*pwdSpeed > 0x80) || (*pwdSpeed < 0x10)){
            BSP_Print(BSP_PRINT_LEVEL_COMMON,"Set value invalid,reset to defualt 0x80 (0x10~0x80)");
            *pwdSpeed = DEFINE_CPLD_PWDFAN_DEFUALT_SPEED;
    }
    temp = *pwdSpeed;
    if(cpld_write(CPLD_CONTROL_PWMFAN_SPEED_REG,temp) != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
        return BSP_ERR_WRITE;
    }
    return BSP_OK;
}

Int32 BSP_cpldShutdownPower(void)
{
    if((gPCB_VersionID != PCB_VERSION_IDVR_ID) && (gPCB_VersionID != PCB_VERSION_IAMC_ID) && (gPCB_VersionID != PCB_VERSION_DECE_ID)){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);            
        return BSP_OK;
    }

    if(cpld_write(CPLD_CONTROL_POWER_SHUTDOWN_REG, 0xf0) != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
        return BSP_ERR_WRITE;
    }
    return BSP_OK;
}

/*
  ===  FUNCTION  ======================================================================
         Name:  BSP_cpldReadNetConfig
  Description:  读取硬件网络端口配置
  Input Args :  无
  Output Args:  net_conifg ,0: copper, 1: fiber
  Return     :  成功时返回BSP_OK；错误时返回其他错误码
  Date       :  2014.10.10
 =====================================================================================
*/
Int32 BSP_cpldReadNetConfig(UInt32 *net_config)
{
    UInt8 temp;
    if(gPCB_VersionID != PCB_VERSION_DECD_ID){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);            
        *net_config = BSP_INVALID_VALUE;
        return BSP_OK;
    }

    if(cpld_read(CPLD_CONTROL_NETCONFIG_REG, &temp) != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
        *net_config = 0xff;
        return BSP_ERR_READ;
    }
    *net_config = temp;
    return BSP_OK;
    
}
//MSB 3 :subsystem position LSB 5:board postion
Int32 BSP_cpldReadPosition(UInt32 *subsys_pos,UInt32 *board_pos)
{
    UInt8 position,temp;
    if(gPCB_VersionID != PCB_VERSION_DECD_ID){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);            
        *subsys_pos = BSP_INVALID_VALUE;
        *board_pos = BSP_INVALID_VALUE;
        return BSP_OK;
    }
    if(cpld_read(CPLD_CONTROL_POSITION_REG, &position) != BSP_OK){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"%s Error.\n",__func__);
        return BSP_ERR_READ;
    }
    temp = ~position;
    *subsys_pos = (temp >> 5) & 0x07;
    *board_pos  = temp & 0x1f;
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"Get board Position is 0x%x-0x%x [reg 0x%x].\n",*subsys_pos,*board_pos,temp);
    return BSP_OK;    
}
