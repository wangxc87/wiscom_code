#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <asm/ioctl.h>
#include <errno.h>

struct sensor_alarm_param {
    int sensor_in;
    int alarm_out;
    int cmd;
};

#if 1
#define SENSOR_ALARM_IOCTL_MAGIC 'M'
#define SENSOR_IN_GET_CMD    (_IOR(SENSOR_ALARM_IOCTL_MAGIC,0x10,int))
#define ALARM_OUT_SET_CMD    (_IOR(SENSOR_ALARM_IOCTL_MAGIC,0x11,int))
#define ALARM_OUT_GET_CMD    (_IOW(SENSOR_ALARM_IOCTL_MAGIC,0x12,int))
#else
#define SENSOR_IN_GET_CMD    0x00
#define ALARM_OUT_SET_CMD    0x01
#define ALARM_OUT_GET_CMD    0x02
#endif

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

typedef enum
{
    BSP_PRINT_LEVEL_NONE = 0,
    BSP_PRINT_LEVEL_ERROR,
    BSP_PRINT_LEVEL_IMPORTANT,
    BSP_PRINT_LEVEL_COMMON,
    BSP_PRINT_LEVEL_DEBUG,

    BSP_PRINT_LEVEL_MAX
}BSP_PRINT_LEVEL;

#define Int32 int

static Int32 gPrint_level = BSP_PRINT_LEVEL_DEBUG;


#define BSP_Print(u8Level, ps8Format, args...)    \
    do{   \
       if ( u8Level < gPrint_level )   \
        {                                       \
            printf(ps8Format,##args);        \
        }                                       \
    }while(0)

static Int32 gSensor_alarm_fd = -1;
#define SENSOR_ALARM_NAME  "/dev/sensor_alarm"

Int32 BSP_sensorAlarmInit(void)
{
    gSensor_alarm_fd = open(SENSOR_ALARM_NAME,O_RDONLY);
    if(gSensor_alarm_fd < 0)
        return BSP_ERR_BUSY;
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"Open %s Ok.\n");
    return BSP_OK;
}

Int32 BSP_sensorAlarmGetSensorIn(Int32 *sensor_in)
{
    Int32  ret;
    
    if(gSensor_alarm_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Sensor_Alarm not initted.\n");
        return BSP_ERR_READ;
    }
    ret = ioctl(gSensor_alarm_fd, SENSOR_IN_GET_CMD,sensor_in);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"ioctl SENSOR_IN_GET_CMD Error,[%d--%s]].\n",errno, strerror(errno));
            return BSP_ERR_DEV_CTRL;
    }
    return BSP_OK;
}

Int32 BSP_sensorAlarmGetAlarmOut(Int32 *alarm_out)
{
    Int32 ret;
    if(gSensor_alarm_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Sensor_Alarm not initted.\n");
        return BSP_ERR_READ;
    }

    ret = ioctl(gSensor_alarm_fd, ALARM_OUT_GET_CMD,alarm_out);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"ioctl ALARM_OUT_GET_CMD Error,[%d--%s]].\n",errno, strerror(errno));
            return BSP_ERR_DEV_CTRL;
    }
    return BSP_OK;
}

Int32 BSP_sensorAlarmSetAlarmOut(Int32 *alarm_out)
{
    Int32 ret;
    if(gSensor_alarm_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Sensor_Alarm not initted.\n");
        return BSP_ERR_READ;
    }

    ret = ioctl(gSensor_alarm_fd, ALARM_OUT_SET_CMD, alarm_out);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"ioctl ALARM_OUT_SET_CMD Error,[%d--%s]].\n",errno, strerror(errno));
        return BSP_ERR_DEV_CTRL;
    }
    return BSP_OK;
}

Int32 BSP_sensorAlarmDeInit(void)
{
    return close(gSensor_alarm_fd);
}

int main(int argc,char **argv)
{
    Int32 ret;
    Int32 debug_level ;
    Int32 done = 20;
    Int32 sensor_in,alarm_out;
    Int32 i = 0;
    Int32 cmd0,cmd1,cmd2;
    cmd0 = SENSOR_IN_GET_CMD;
    cmd1 = ALARM_OUT_GET_CMD;
    cmd2 = ALARM_OUT_SET_CMD;

    if(BSP_sensorAlarmInit() < 0){
        fprintf(stdout,"sensorAlarm init Error.\n");
        return -1;
    }else
        fprintf(stdout,"sensorAlarm init OK,fd [%d].\n",gSensor_alarm_fd);
    /*
    fprintf(stdout,"SENSOR_IN_GET [0x%x],ALARM_OUT_GET [0x%x],ALARM_OUT_SET [0x%x].\n",
            cmd0,cmd1,cmd2);
    */
    fprintf(stdout,"\n");

    while(done--){
        
        if( BSP_sensorAlarmGetSensorIn(&sensor_in) == BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_COMMON, "Get sensor input status is 0x%02x.\n",sensor_in);
        else
            BSP_Print(BSP_PRINT_LEVEL_ERROR, "Get sensor input status Error.\n");

#if 1        
        alarm_out = 1 << i;
        if(BSP_sensorAlarmSetAlarmOut(&alarm_out) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"Set alarm out Error.\n");
        else
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"Set alarm out 0x%02x.\n",alarm_out);
        i ++;
        if(i >3)
            i = 0;

        if( BSP_sensorAlarmGetAlarmOut(&alarm_out) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"Get alarm out status Error.\n");
        else
            BSP_Print(BSP_PRINT_LEVEL_COMMON,"Get alarm out status is 0x%02x.\n",alarm_out);
#endif
        fprintf(stdout,"\n");
        sleep(2);
    }

    BSP_sensorAlarmDeInit();
    return 0;    
}
