#include "bsp.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/sensor_alarm.h>

static Int32 gSensor_alarm_fd = -1;
#define SENSOR_ALARM_NAME  "/dev/sensor_alarm"

//support return 1;not return 0
static Int32 notSupport_alarmOut(void)
{
    if((gPCB_VersionID != PCB_VERSION_DECE_ID) && (gPCB_VersionID != PCB_VERSION_IDVR_ID) &&
       (gPCB_VersionID != PCB_VERSION_IAMC_ID) && (gPCB_VersionID != PCB_VERSION_IAMB_ID))
        return 1;
    else
        return 0;
}

//support return 1:not return 0
static Int32 notSupport_sensorIn(void)
{
    if((gPCB_VersionID != PCB_VERSION_IDVR_ID) && (gPCB_VersionID != PCB_VERSION_IAMC_ID) &&
       (gPCB_VersionID != PCB_VERSION_IAMB_ID))
        return 1;
    else
        return 0;
}

Int32 BSP_sensorAlarmInit(void)
{
    if(notSupport_alarmOut() && notSupport_sensorIn()){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
        return BSP_OK;
    }

    gSensor_alarm_fd = open(SENSOR_ALARM_NAME,O_RDONLY);
    if(gSensor_alarm_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Open %s Error.\n",SENSOR_ALARM_NAME);
        if(gSensor_alarm_fd == -EBUSY)
            return BSP_ERR_BUSY;
        return BSP_ERR_DEV_OPEN;
    }
    
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"Open %s Ok,fd [%d].\n",SENSOR_ALARM_NAME,gSensor_alarm_fd);
    return BSP_OK;
}

//no signal gpio level 1, driver module has change
Int32 BSP_sensorAlarmGetSensorIn(Int32 *sensor_in)
{
    Int32  ret;
    if(notSupport_sensorIn()){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
        *sensor_in = BSP_INVALID_VALUE;
        return BSP_OK;
    }
    
    if(gSensor_alarm_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Sensor_Alarm not initted.\n");
        return BSP_ERR_READ;
    }

    ret = ioctl(gSensor_alarm_fd, SENSOR_IN_GET_CMD,sensor_in);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"ioctl SENSOR_IN_GET_CMD Error.\n");
            return BSP_ERR_DEV_CTRL;
    }
    return BSP_OK;
}

Int32 BSP_sensorAlarmGetAlarmOut(Int32 *alarm_out)
{
    Int32 ret;
    if(notSupport_alarmOut()){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
        *alarm_out = BSP_INVALID_VALUE;
        return BSP_OK;
    }

    if(gSensor_alarm_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Sensor_Alarm not initted.\n");
        return BSP_ERR_READ;
    }

    ret = ioctl(gSensor_alarm_fd, ALARM_OUT_GET_CMD,alarm_out);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"ioctl ALARM_OUT_GET_CMD Error.\n");
            return BSP_ERR_DEV_CTRL;
    }
    return BSP_OK;
}

Int32 BSP_sensorAlarmSetAlarmOut(Int32 *alarm_out)
{
    Int32 ret;
    if(notSupport_alarmOut()){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
        return BSP_OK;
    }

    if(gSensor_alarm_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Sensor_Alarm not initted.\n");
        return BSP_ERR_READ;
    }

    ret = ioctl(gSensor_alarm_fd, ALARM_OUT_SET_CMD, alarm_out);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"ioctl ALARM_OUT_SET_CMD Error.\n");
        return BSP_ERR_DEV_CTRL;
    }
    return BSP_OK;
}

Int32 BSP_sensorAlarmDeInit(void)
{
    if(notSupport_alarmOut()&&notSupport_sensorIn()){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"[Warnning]%s:Current board [%s] NOT support this function.\n",__func__,gPcbName);
        return BSP_OK;
    }
    return close(gSensor_alarm_fd);
}
