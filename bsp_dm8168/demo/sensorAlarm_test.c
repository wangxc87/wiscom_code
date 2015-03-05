#include <stdio.h>
#include <unistd.h>
#include "bsp.h"


int main(int argc,char **argv)
{
    Int32 ret;
    Int32 debug_level ;
    Int32 done = 10;
    Int32 sensor_in,alarm_out;
    Int32 i = 0;

    fprintf(stdout,"INFO:\"sensorAlarm_test 4 \" enable debug mode.\n\n");
    if(argc < 2)
        debug_level = BSP_PRINT_LEVEL_COMMON; // level 3
    else
        debug_level = atoi(argv[1]);

    BSP_PrintSetLevel(debug_level);
    BSP_Init();
    fprintf(stdout,"\n");

    while(done--){
        if( BSP_sensorAlarmGetSensorIn(&sensor_in) == BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_COMMON, "Get sensor input status is 0x%02x.\n",sensor_in);
        else
            BSP_Print(BSP_PRINT_LEVEL_ERROR, "Get sensor input status Error.\n");

        if(i < ALARM_OUT_MAX_NUM){
            alarm_out = 1 << i;
            i ++;
        }
        else{
            i = 0;
            continue;
        }

        if(BSP_sensorAlarmSetAlarmOut(&alarm_out) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"Set alarm out Error.\n");
        else
            BSP_Print(BSP_PRINT_LEVEL_COMMON,"Set alarm out 0x%02x.\n",alarm_out);
        

        if( BSP_sensorAlarmGetAlarmOut(&alarm_out) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"Get alarm out status Error.\n");
        else
            BSP_Print(BSP_PRINT_LEVEL_COMMON,"Get alarm out status is 0x%02x.\n",alarm_out);

        fprintf(stdout,"\n");
        sleep(2);
    }

    BSP_Uninit();
    return 0;    
}
