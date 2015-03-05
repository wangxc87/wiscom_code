// =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  demo.c
//
//    Description:
//
//        Version:  1.0
//        Created:  2014-08-19
//
//         Author:  jianhuifu
//
// =====================================================================================

#include <pthread.h>
#include <errno.h>
#include <sys/select.h>
#include <bsp.h>

void wait(unsigned int u32Sec, unsigned int u32uSec)
{
    struct timeval  stTime;

    stTime.tv_sec = u32Sec;
    stTime.tv_usec= u32uSec;
    select(0, 0, 0, 0, &stTime);

    return;
}

void *temp_thread(void *arg)
{
    float temp;

    while(1)
    {
        BSP_GetCPUTemperature(&temp);

        BSP_Print(BSP_PRINT_LEVEL_COMMON, "CPU temperature: %5.3f\n", temp);

        wait(4, 0);
    };

    return 0;
}

void *watchdog_thread(void *arg)
{
    int t;

    BSP_EnableWatchdog();
    BSP_SetWatchdogTimeout(2);
    BSP_GetWatchdogTimeout(&t);
    BSP_Print(BSP_PRINT_LEVEL_COMMON, "Watchdog timeout: %d\n", t);

    while(1)
    {
        BSP_FeedWatchdog();

        wait(1, 0);
    };

    BSP_DisableWatchdog();

    return 0;
}

void *sensorAlarm_thread(void *arg)
{
    int sensor_in,alarm_out;
    int i = 0;
    while(1){
        if(i < ALARM_OUT_MAX_NUM){
            alarm_out = 1 << i;
            i ++;
        }
        else{
            i = 0;
            continue;
        }

        if(BSP_GetSensorIn(&sensor_in) != BSP_OK){
            printf("BSP_getSensorIn Error.\n");
            break;
        }else{
            printf("BSP_getSensor in is 0x%04x.\n",sensor_in);
        }

        if(BSP_SetAlarmOut( alarm_out ) != BSP_OK)
            break;

        if(BSP_GetAlarmOut(&alarm_out) != BSP_OK){
            printf("BSP_getAlarmOunt Error.\n");
            break;
        }else
            printf("Current Alarm Out is 0x%02x\n",alarm_out);
        
        printf("\n");
        wait(4, 0);
    }
    return 0;
}

void *statusLed_thread(void *arg)
{
    int ret;
    while(1){
        //test led on
        if(BSP_runLedSet(BSP_LED_ON) != BSP_OK){
            printf("BSP_runLedSet Error.\n");
            break;
        }
        if(BSP_errLedSet(BSP_LED_ON) != BSP_OK){
            printf("BSP_errLedSet Error.\n");
            break;
        }

        wait(1, 0);

        //test led off
        if(BSP_runLedSet(BSP_LED_OFF) != BSP_OK){
            printf("BSP_runLedSet Error.\n");
            break;
        }

        if(BSP_errLedSet(BSP_LED_OFF) != BSP_OK){
            printf("BSP_errLedSet Error.\n");
            break;
        }
        wait(1, 0);
    }
    return 0;
}

int main(int argc, char **argv)
{

    pthread_t   stTempThread;
    pthread_t   stWatchdogThread;
    pthread_t   stSensorAlarmThread;
    pthread_t   stSatusLedThread;

    if ( BSP_OK != BSP_Init() )
    {
        printf("BSP init error\n");
        return -1;
    }

    BSP_PrintSetLevel(BSP_PRINT_LEVEL_COMMON);
#define TEST_EEPROM
#ifdef TEST_EEPROM
    char eeprom_char[32],temp[32];
    int ret,num_mac;
    memcpy(eeprom_char,"5E:A2:F7:81:43:43", BSP_MAC_LENGTH);
    ret = BSP_SetEthMAC(eeprom_char,1);
    if(ret != BSP_OK){
        printf("eeprom write Error.\n");
        goto loop;
    }
    ret = BSP_GetEthMAC(temp, &num_mac);
    if(ret != BSP_OK){
        printf("eeprom read Error.\n");
        goto loop;
    }
        
    for(ret = 0;ret < BSP_MAC_LENGTH;ret ++){
        if(eeprom_char[ret] != temp[ret]){
            printf("eeprom test %d Error write-read:%s - %s.\n",
                   num_mac,eeprom_char,temp);
            goto loop;
        }
    }
    printf("\n\t****Eeprom Test OK****\n");
        
#endif
 loop:    
    printf("\tPress any key to continue...\n");
    getchar();
    if ( 0 != pthread_create(&stTempThread, 0, (void*)temp_thread, 0) )
    {
        printf("temp thread error %s\n", strerror(errno));
        return -1;
    }

    if ( 0 != pthread_create(&stWatchdogThread, 0, (void*)watchdog_thread, 0) )
    {
        printf("led thread error %s\n", strerror(errno));
        return -1;
    }

    if ( 0 != pthread_create(&stSensorAlarmThread, 0, (void*)sensorAlarm_thread, 0) )
    {
        printf("sensorAlarm thread error %s\n", strerror(errno));
        return -1;
    }

    if( 0 != pthread_create(&stSatusLedThread, 0,(void *)statusLed_thread, 0))
    {
        printf("statusLed thread error %s\n", strerror(errno));
        return -1;
    }

    pthread_join(stWatchdogThread, 0);
    pthread_join(stTempThread, 0);
    pthread_join(stSensorAlarmThread, 0);
    pthread_join(stSatusLedThread, 0);

    BSP_Uninit();

    return 0;
}

// =====================================================================================
//    End of demo.c
// =====================================================================================:

