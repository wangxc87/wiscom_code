#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "bsp.h"


static BSP_PRINT_LEVEL enPrintLevel = BSP_PRINT_LEVEL_COMMON;

int BSP_PrintSetLevel(const BSP_PRINT_LEVEL enLevel)
{
    if ( enLevel >= BSP_PRINT_LEVEL_MAX )
    {
        return BSP_ERR_ARG;
    }

    BSP_Print(BSP_PRINT_LEVEL_IMPORTANT, "change print level %d -> %d\n", enPrintLevel, enLevel);

    enPrintLevel = enLevel;

    return BSP_OK;
}

BSP_PRINT_LEVEL BSP_PrintGetLevel(void)
{
    return enPrintLevel;
}

int BSP_LocalPrint(const BSP_PRINT_LEVEL enLevel,
                    const char *ps8File, const unsigned int u32Line,
                    const char *ps8Format, ...)
{
    va_list     argList;
    unsigned int    u32Len;
    char s8Msg[BSP_PRINT_MAX_LEN];

    memset(s8Msg, 0, BSP_PRINT_MAX_LEN);

    va_start(argList, ps8Format);
    u32Len = vsprintf((char*)(s8Msg), (const char*)ps8Format, argList);
    va_end(argList);

    if ( enLevel > enPrintLevel )   // sunling
        return BSP_OK;

    s8Msg[u32Len - 1] = 0;

    if ( BSP_PRINT_LEVEL_ERROR != enLevel )
    {
        fprintf(stdout, "[%d] %s [%s - %d]\n", enLevel, s8Msg, ps8File, u32Line);
    }
    else
    {
        fprintf(stdout, "[%d] %s [%d - %s] [%s - %d]\n",
                    enLevel, s8Msg, errno, strerror(errno), ps8File, u32Line);
    }

    fflush(stdout);
    return BSP_OK;
}

//store board info
 struct BSP_boardInfo gBSP_boardInfo;

static int BSP_getBoardInfo( void )
{
    memset(&gBSP_boardInfo, 0, sizeof(struct BSP_boardInfo));
    switch (gPCB_VersionID){
    case PCB_VERSION_IAMB_ID:
        gBSP_boardInfo.uart485_num = 1;
        gBSP_boardInfo.uart232_num = 1;
        strcpy(gBSP_boardInfo.uart485_dev[0], "/dev/ttyO0");
        strcpy(gBSP_boardInfo.uart232_dev[0], "/dev/ttyO1");
        gBSP_boardInfo.sensorIn_num = 2;
        gBSP_boardInfo.alarmOut_num = 3;
        gBSP_boardInfo.netPort_num = 1;
        break;
    case PCB_VERSION_IAMC_ID:
        gBSP_boardInfo.uart485_num = 2;
        gBSP_boardInfo.uart232_num = 2;
        strcpy(gBSP_boardInfo.uart485_dev[0], "/dev/ttyO0");
        strcpy(gBSP_boardInfo.uart485_dev[1], "/dev/ttyMAX0");
        strcpy(gBSP_boardInfo.uart232_dev[0], "/dev/ttyO1");
        strcpy(gBSP_boardInfo.uart232_dev[1], "/dev/ttyMAX1");
        gBSP_boardInfo.sensorIn_num = 2;
        gBSP_boardInfo.alarmOut_num = 2;
        gBSP_boardInfo.netPort_num = 2;
        break;
    case PCB_VERSION_IDVR_ID:
        gBSP_boardInfo.uart485_num = 2;
        gBSP_boardInfo.uart232_num = 2;
        strcpy(gBSP_boardInfo.uart485_dev[0], "/dev/ttyO0");
        strcpy(gBSP_boardInfo.uart485_dev[1], "/dev/ttyMAX0");
        strcpy(gBSP_boardInfo.uart232_dev[0], "/dev/ttyMAX1");
        strcpy(gBSP_boardInfo.uart232_dev[1], "/dev/ttyO1");
        gBSP_boardInfo.sensorIn_num = 16;
        gBSP_boardInfo.alarmOut_num = 4;
        gBSP_boardInfo.netPort_num = 2;

        if(gFUNC_VersionID == FUN_VERSION_KDVR1608_ID)
            gBSP_boardInfo.uart232_num = 0;
            
        break;
    case PCB_VERSION_DECE_ID:
        gBSP_boardInfo.uart485_num = 1;
        gBSP_boardInfo.uart232_num = 0;
        strcpy(gBSP_boardInfo.uart485_dev[0], "/dev/ttyO0");
        gBSP_boardInfo.sensorIn_num = 4;
        gBSP_boardInfo.alarmOut_num = 4;
        gBSP_boardInfo.netPort_num = 1;
        break;
    case PCB_VERSION_DECD_ID:
        gBSP_boardInfo.uart485_num = 0;
        gBSP_boardInfo.uart232_num = 0;
        gBSP_boardInfo.sensorIn_num = 0;
        gBSP_boardInfo.alarmOut_num = 0;
        gBSP_boardInfo.netPort_num = 2;
        break;
    default:
        BSP_Print(BSP_PRINT_LEVEL_COMMON,"%s: Invalid PCB_version ID [0x%x].\n", __func__, gPCB_VersionID);
    }
    return BSP_OK;
}
int BSP_Init(void)
{
    int ret = BSP_OK;
    unsigned int versionID;
    ret = BSP_cpldInit();
    
    if(ret != BSP_OK)
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"BSP_cpldInit Error.\n");
    else{
        if( BSP_cpldReadBoardID(&versionID) == BSP_OK){
            BSP_Print(BSP_PRINT_LEVEL_COMMON,"Get Board Name \"%s\",PCB Name \"%s\",Version ID 0x%x.\n",gBoardName,gPcbName,versionID);
        BSP_getBoardInfo();//get board info,such as uart232\uart483,gpio num.
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"BoardInfo: uart485-%d, uart232-%d, sensorIn_num-%d, alarmOut_num-%d, netPort_num-%d.\n",
                  gBSP_boardInfo.uart485_num,gBSP_boardInfo.uart232_num,
                  gBSP_boardInfo.sensorIn_num,gBSP_boardInfo.alarmOut_num, gBSP_boardInfo.netPort_num);
        }
        BSP_Print(BSP_PRINT_LEVEL_COMMON,"BSP_cpldInit OK.\n");

    }
    
    ret = BSP_sensorAlarmInit();
    if(ret != BSP_OK)
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"BSP_sensorAlarmInit Error.\n");
    else
        BSP_Print(BSP_PRINT_LEVEL_COMMON,"BSP_sensorAlarmInit OK.\n");
    return ret;
}


int BSP_Uninit(void)
{
    int ret = BSP_OK;
    ret = BSP_cpldDeinit();
    if(ret != BSP_OK)
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"BSP_cpldDeInit Error.\n");

    ret = BSP_sensorAlarmDeInit();
    if(ret != BSP_OK)
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"BSP_sensorAlarmDeInit Error.\n");
    BSP_Print(BSP_PRINT_LEVEL_COMMON,"BSP_Uninit over.\n");
    return ret;
}

// ===  FUNCTION  ======================================================================
//         Name:  BSP_Reboot
//  Description:  reboot system safely
//  Input Args :
//  Output Args:
//  Return     :  BSP_ERR_BUSY: device busy, system is not permitted to reboot
//                BSP_OK: reboot system
// =====================================================================================
int BSP_Reboot ( )
{
    if ( 1 == BSP_GetRebootFlag() )
    {
        return BSP_ERR_BUSY;
    }	// ----- end if -----

    reboot(LINUX_REBOOT_CMD_RESTART);

    return BSP_OK;
}
// -----  end of function BSP_Reboot  -----


// ===  FUNCTION  ======================================================================
//         Name:  BSP_Halt
//  Description:  halt system safely
//  Input Args :
//  Output Args:
//  Return     :  BSP_ERR_BUSY: device busy, system is not permitted to reboot
//                BSP_OK: reboot system
// =====================================================================================
int BSP_Halt ( )
{
    if ( 1 == BSP_GetRebootFlag() )
    {
        return BSP_ERR_BUSY;
    }	// ----- end if -----

    system("halt");

    return BSP_OK;
}
// -----  end of function BSP_Halt  -----

#ifndef EPLD_REBOOT_FLAG
static int s32RebootFlag = 0;
#endif

// ===  FUNCTION  ======================================================================
//         Name:  BSP_SetRebootFlag
//  Description:  set reboot flag to 1 when reboot or halt should be disable
//  Input Args :  1: disable reboot or halt
//                0: enable reboot and halt
//  Input Args :
//  Output Args:
//  Return     :
// =====================================================================================
void BSP_SetRebootFlag ( int s32Flag )
{
    unsigned char   u8Value;

    if ( s32Flag != 0 )
        s32Flag = 1;

#ifdef EPLD_REBOOT_FLAG
    ioctl(s32EPLDFd, EPLD_READ_LOGIC, &u8Value);
    EPLD_LOGIC_SET_REBOOT(u8Value, s32Flag);
    ioctl(s32EPLDFd, EPLD_WRITE_LOGIC, &u8Value);
#else
    s32RebootFlag = s32Flag;
#endif

    return;
}
// -----  end of function BSP_SetRebootFlag  -----


// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetRebootFlag
//  Description:  get reboot flag to 1 when reboot or halt should be disable
//  Input Args :
//  Output Args:
//  Return     :  1: disable reboot or halt
//                0: enable reboot and halt
// =====================================================================================
int BSP_GetRebootFlag ( )
{
    unsigned char   u8Value;

#ifdef EPLD_REBOOT_FLAG
    ioctl(s32EPLDFd, EPLD_READ_LOGIC, &u8Value);
    return EPLD_LOGIC_GET_REBOOT(u8Value);
#else
    return s32RebootFlag;
#endif

}
// -----  end of function BSP_GetRebootFlag  -----

//get DVR/DECE sensor in 

//修正 DECE 机箱丝印错序
#define DECE_GPIO_NUM_FIXED

int BSP_GetSensorIn(int *sensor_in)
{
    int sensorAlarm_in,ret, i;
    unsigned int  cpldSensor_in,temp0,temp1;

    if((gPCB_VersionID != PCB_VERSION_DECE_ID) && (gPCB_VersionID != PCB_VERSION_IDVR_ID) &&
       (gPCB_VersionID != PCB_VERSION_IAMC_ID) &&(gPCB_VersionID != PCB_VERSION_IAMB_ID)){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
        *sensor_in = BSP_INVALID_VALUE;
        return BSP_OK;
    }
    temp0 = 0;
    temp1 = 0;
    
    ret = BSP_cpldReadSensor(&cpldSensor_in);
    if(ret != BSP_OK)
        return ret;
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[dbg]Get cpldSensorIn is 0x%x.\n",cpldSensor_in);

    //DECE
    if(gPCB_VersionID == PCB_VERSION_DECE_ID){
#ifdef DECE_GPIO_NUM_FIXED
        temp0 = cpldSensor_in & 0x0f;
        temp1 = 0;
        for(i = 0;i < 4; i ++){
            if(temp0 & (1 << i))
                temp1 |= 1 << (3 - i);
        }
        *sensor_in = temp1;
#else
        *sensor_in = cpldSensor_in & 0x0f;
#endif
        return BSP_OK;
    }

    ret = BSP_sensorAlarmGetSensorIn(&sensorAlarm_in);
    if(ret != BSP_OK)
        return ret;
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[dbg]Get sensorAlarm_in 0x%x.\n",sensorAlarm_in);

    //IDVR
    if(gPCB_VersionID == PCB_VERSION_IDVR_ID){ 
        temp0 = ((sensorAlarm_in & 0x0e)<< 8) | ((sensorAlarm_in & 0x01) << 7);
        if((sensorAlarm_in & 0x10) == 0x10)
            temp0 |= 1 << 13;
        if((sensorAlarm_in & 0x20) == 0x20)
            temp0 |= 1 << 12;

        temp1 = ((cpldSensor_in & 0x03) << 14) | ((cpldSensor_in & 0x8000) >> 7) | ((cpldSensor_in & 0x7f00) >> 8);
        *sensor_in = temp0 | temp1;
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[dbg]Fixed sensorAlarm_temp [0x%x],cpldSensor_temp [0x%x] sensor_in [0x%x].\n",
                  temp0,temp1,*sensor_in);
    }

    //iAMC 
    if(gPCB_VersionID == PCB_VERSION_IAMC_ID){
        *sensor_in = sensorAlarm_in & 0x03; //only has two Sensor_input
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[dbg]Fixed sensorAlarm is 0x%x [0x%x].\n",*sensor_in, sensorAlarm_in);
    }

    //iAMB
    if(gPCB_VersionID == PCB_VERSION_IAMB_ID){
        temp0 = 0;
        if((sensorAlarm_in & 0x1))
            temp0 |= 0x02;
        if(sensorAlarm_in & 0x02)
            temp0 |= 0x01;
        *sensor_in = temp0;
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[dbg]Fixed sensor in is 0x%x [0x%x].\n",
                  *sensor_in,sensorAlarm_in);
    }
    
    return BSP_OK;
}

int  BSP_GetAlarmOut(int *alarm_out)
{
    int ret;
    int temp,i;

    if((gPCB_VersionID != PCB_VERSION_DECE_ID) && (gPCB_VersionID != PCB_VERSION_IDVR_ID) &&
       (gPCB_VersionID != PCB_VERSION_IAMC_ID) && (gPCB_VersionID != PCB_VERSION_IAMB_ID)){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
        *alarm_out = BSP_INVALID_VALUE;
        return BSP_OK;
    }

        ret = BSP_sensorAlarmGetAlarmOut(&temp);
        if(ret < 0){
            return ret;
        }

    //DECE board
    if(gPCB_VersionID == PCB_VERSION_DECE_ID){
#ifdef DECE_GPIO_NUM_FIXED
        *alarm_out = 0;
        for(i = 0; i < 4; i++){
            if(temp & (1 << i))
                *alarm_out |= 1 << (3 - i);
        }
        //        ret = BSP_sensorAlarmGetAlarmOut(&temp0);
    
#else
        *alarm_out = temp;
#endif
    }

    //DVR
    if(gPCB_VersionID == PCB_VERSION_IDVR_ID)
        *alarm_out = temp;

    //iAMC 
    if(gPCB_VersionID == PCB_VERSION_IAMC_ID){
        *alarm_out = temp & 0x03;
    }

    //iAMB
    if(gPCB_VersionID == PCB_VERSION_IAMB_ID)
        *alarm_out = temp & 0x07;
    
    if(ret != BSP_OK)
        return ret;

    return BSP_OK;
        
}

int BSP_SetAlarmOut(int alarm_out)
{
    int ret,temp0;
    if((gPCB_VersionID != PCB_VERSION_DECE_ID) && (gPCB_VersionID != PCB_VERSION_IDVR_ID) &&
       (gPCB_VersionID != PCB_VERSION_IAMC_ID) && (gPCB_VersionID != PCB_VERSION_IAMB_ID)){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
        return BSP_OK;
    }

    //DECE
    if(gPCB_VersionID == PCB_VERSION_DECE_ID){
#ifdef DECE_GPIO_NUM_FIXED
        int i;
        temp0 = 0;
        for(i = 0; i < 4; i++){
            if(alarm_out & (1 << i))
                temp0 |= 1 << (3 - i);
        }
        ret = BSP_sensorAlarmSetAlarmOut(&temp0);
#else
        ret = BSP_sensorAlarmSetAlarmOut(&alarm_out);
#endif
    }

    //DVR
    if(gPCB_VersionID == PCB_VERSION_IDVR_ID)
        ret = BSP_sensorAlarmSetAlarmOut(&alarm_out);

    //IAMC
    if(gPCB_VersionID == PCB_VERSION_IAMC_ID){
        temp0 = alarm_out & 0x03;
        ret = BSP_sensorAlarmSetAlarmOut(&temp0);
    }

    //iAMB
    if(gPCB_VersionID == PCB_VERSION_IAMB_ID){
        temp0 = alarm_out & 0x07;
        ret = BSP_sensorAlarmSetAlarmOut(&temp0);
    }

    if(ret != BSP_OK)
        return ret;
    return BSP_OK;
}

#define BSP_LED_WRONG_DEVICE "gpio52"
#define BSP_LED_RUN_DEVICE "gpio55"

int BSP_runLedSet(int value)
{
    char cmd[64];
    int ret = BSP_OK;
    if((value != BSP_LED_ON) && (value != BSP_LED_OFF)){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"runLed set value invalid.\n");
        return BSP_ERR_ARG;
    }

    if((gPCB_VersionID == PCB_VERSION_IAMB_ID) || (gPCB_VersionID == PCB_VERSION_DECD_ID)){  //iAMB board low level turn on led
        if(value != BSP_LED_ON)
            sprintf(cmd,"echo %d > /sys/class/gpio/%s/value",
                    BSP_LED_ON, BSP_LED_RUN_DEVICE);
        else
            sprintf(cmd,"echo %d > /sys/class/gpio/%s/value",
                    BSP_LED_OFF, BSP_LED_RUN_DEVICE);
    } else {
        sprintf(cmd,"echo %d > /sys/class/gpio/%s/value",
                value, BSP_LED_RUN_DEVICE);
    }
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"%s:cmd is %s.\n",__func__, cmd);

    ret = system(cmd);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"runLed set cmd Error.\n");
        return BSP_ERR_WRITE;
    }

    return BSP_OK;
}

int BSP_errLedSet(int value)
{
    char cmd[64];
    int ret = BSP_OK;
    if((value != BSP_LED_ON) && (value != BSP_LED_OFF)){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"errLed set value invalid.\n");
        return BSP_ERR_ARG;
    }

    if((gPCB_VersionID == PCB_VERSION_IAMB_ID) || (gPCB_VersionID == PCB_VERSION_DECD_ID)){  //iAMB board low level turn on led
        if(value != BSP_LED_ON)
            sprintf(cmd,"echo %d > /sys/class/gpio/%s/value",
                    BSP_LED_ON, BSP_LED_WRONG_DEVICE);
        else
            sprintf(cmd,"echo %d > /sys/class/gpio/%s/value",
                    BSP_LED_OFF, BSP_LED_WRONG_DEVICE);
    } else {
        sprintf(cmd,"echo %d > /sys/class/gpio/%s/value",
                value, BSP_LED_WRONG_DEVICE);
    }
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"%s: cmd is %s.\n", __func__, cmd);

    ret = system(cmd);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"errLed set cmd Error.\n");
        return BSP_ERR_WRITE;
    }

    return BSP_OK;
}
