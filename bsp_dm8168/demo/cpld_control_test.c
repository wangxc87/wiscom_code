#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include "bsp.h"

int usage(void)
{
    fprintf(stdout,"Info: <%s %s>\n",__TIME__, __DATE__);
    fprintf(stdout,"   -p : bsp_printf level 0/1/2/3/4,default BSP_PRINT_LEVEL_COMMON [3].\n");
    fprintf(stdout,"   -l : test run times,defult: 10.\n");
    fprintf(stdout,"   -t :value 0 :disalbe HW watchdog Test [defalut].\n");
    fprintf(stdout,"       non-zero: enbale HW watchdog and systerm will reboot in 30s.\n");
    fprintf(stdout,"   -s :slect close 30s HW watchdog on startup.\n");
    fprintf(stdout,"   -h : help info\n");
    fprintf(stdout,"Example: ./cpld_control_test [-p print_level ] [-t 0/1] [-l 5]\n");
    return 0;
}
int main(int argc,char **argv)
{
    Int32 ret;
    Int32 debug_level ;
    Int32 done = 10;
    UInt32 senor_value;
    UInt32 board_id,power_status,update_signal,pwm_speed,subsys_pos,board_pos;

    UInt32 enHwWatchdog = 0;
    char *optstring = "p:t:l:sh";
    int opt;    
    Int32 startup_flag = 0;
    if(argc < 2)
        usage();

    while (1){
        opt = getopt(argc, argv, optstring);
        if(opt == -1)
            break;
        switch (opt){
        case 'p':
            debug_level = atoi(optarg);
            break;
        case 't':
            enHwWatchdog = atoi(optarg);
            break;
        case 'l':
            done = atoi(optarg);
            break;
        case 's':
            BSP_Init();
            if(BSP_cpldDisableHwWatchdog() != BSP_OK)   //disable HW timer reset,this function should run on startup.
                BSP_Print(BSP_PRINT_LEVEL_ERROR,"bsp_cpld disable HW watchdog error.\n");
            else
                printf("bsp_cpld disable HW watchdog Ok.\n");
            BSP_Uninit();
            return 0;
        case 'h':
            usage();
            return 0;
        default:
            fprintf(stdout,"Invalid arg.\n");
            usage();
            break;
        }
    }
    if(enHwWatchdog)
        fprintf(stdout,"\nWARRNING:\n\tEnable HW timer reset,System will reboot in 30s.\n");
    else
        fprintf(stdout,"\n\tTest will run %d times.\n",done);
    fprintf(stdout,"\tPress any key to continue or 'q' EXIT test...\n");
    if( getchar() == 'q'){
        fprintf(stdout,"Test exit.\n");
            return 0;
    }

    BSP_PrintSetLevel(debug_level);
    BSP_Init();

    if(BSP_cpldDisableHwWatchdog() != BSP_OK)   //disable HW timer reset,this function should run on startup.
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"bsp_cpld disable HW watchdog error.\n");
    else
        printf("bsp_cpld disable HW watchdog Ok.\n");

    if(enHwWatchdog){
        if(BSP_cpldEnableHwWatchdog() != BSP_OK)   //enable HW timer reset
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"bsp_cpld enable HW watchdog error.\n");
        else{
            printf("bsp_cpld enable HW watchdog Ok.\n");
            printf("board will reboot in 30s.\n");
        }
        sleep(30);
    }


    //开启software看门狗功能
    if(BSP_cpldEnableSoftWatchdog() != BSP_OK)
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"bsp_cpld enable watchdog error.\n");
    else
        printf("bsp_cpld enable watchdog Ok.\n");
    fprintf(stdout,"\n");

    while(done--){
        //feed watchdog every 1.5s
        if(BSP_cpldFeedSoftWatchdog() != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"bsp_cpld feed watchdog Error.\n");

        //read sensor value
        if(BSP_cpldReadSensor(&senor_value) != BSP_OK)
            printf("bsp_cpld read Error.\n");
        else
            if(senor_value != BSP_INVALID_VALUE)
                printf("Sensor read vaule is 0x%04x",senor_value);

        //read powerstatus
        if(BSP_cpldReadPowerDownSignal(&power_status) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"Power button status Error.\n");
        else
            if(power_status != BSP_INVALID_VALUE)
                printf("Power button status is 0x%02x",power_status);

        //read version update signal
        if(BSP_cpldReadUpdateSignal(&update_signal) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"SOFT Version update signal read Error.\n");
        else
            if(update_signal != BSP_INVALID_VALUE)
                printf("SOFT Version update stauts is 0x%02x.\n",update_signal);

        //read board id
        if(BSP_cpldReadBoardID(&board_id) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"cpld Read board Id Error.\n");
        else
            if(board_id != BSP_INVALID_VALUE)
                printf("Read board Name \"%s\",PCB Name \"%s\", boardID 0x%02x.\n",
                       gBoardName, gPcbName, board_id);
            else
                printf("Unkown Board,boardId 0x%02x.\n",board_id);

        //get PWMFan Speed 
        if(BSP_cpldGetPWMFanSpeed(&pwm_speed) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"cpld Get PwmFan speed Error.\n");
        else
            if(pwm_speed != BSP_INVALID_VALUE)
                printf("cpld Get PwmFan speed is 0x%02x.\n",pwm_speed);

        //set PWMFan Speed
        if(BSP_cpldSetPWMFanSpeed(&pwm_speed) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"cpld Set PwmFan speed Error.\n");
        else
            if(pwm_speed != BSP_INVALID_VALUE)
                printf("cpld Set PwmFan speed is 0x%02x.\n",pwm_speed);

        //get Video Input status
        if(BSP_cpldShowVideoInputStatus() != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"cpld Get Video input status Error.\n");

        //get board position
        if(BSP_cpldReadPosition(&subsys_pos,&board_pos) != BSP_OK)
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"cpld Get Board position Error.\n");
        else
            if(board_pos != BSP_INVALID_VALUE)
                printf("cpld Get Board Position is 0x%x-0x%x.\n",subsys_pos,board_pos);

        fprintf(stdout,"\n");
        

        sleep(1);
    }

    BSP_cpldDisableSoftWatchdog();


    BSP_Uninit();
    return 0;    
}
