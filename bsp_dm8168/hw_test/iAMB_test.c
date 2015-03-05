#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bsp.h"

static struct BSP_boardInfo gLocal_boardInfo;

static void usage(char *argv)
{
    fprintf(stdout,"Info:\n");
    fprintf(stdout,"   -l : set local ip addr in net test.\n");
    fprintf(stdout,"   -r : set remote ip addr in net test.\n");
    fprintf(stdout,"   -p : bsp_printf level 0/1/2/3/4,default BSP_PRINT_LEVEL_COMMON [3].\n");
    fprintf(stdout,"   -h : help info\n");
    fprintf(stdout,"Example: %s [-p print_level ]  [-l 1.1.9.43] -r 1.1.9.22 \n",argv);
}

char gTest_runMenu[] = {
    "\r\n"
    "\r\n ======================="
    "\r\n  %s-%s Test Menu"
    "\r\n  %s %s"
    "\r\n ======================="
    "\r\n"
    "\r\n a: auto test item"
    "\r\n 0: eeprom test"
    "\r\n 1: run-status led Test"
    "\r\n 2: cpld sub-function Test"
    "\r\n 3: uart485 Test"
    "\r\n 4: uart232 Test"
    "\r\n 5: disk Test"
    "\r\n 6: RTC IC Test"
    "\r\n 7: Alarm out/in Test"
    "\r\n 8: net Port [0/1] test"
    "\r\n 9: display test"
    "\r\n"
    "\r\n e: Stop Demo"
    "\r\n"
    "\r\n Enter Choice: "
};
int print_runMenu(void)
{
    printf(gTest_runMenu,gBoardName, gPcbName, __TIME__,__DATE__);
    return 0;
}

void clear_stdin(void)
{
    char ch_temp;
    while((ch_temp = getchar()) != '\n' && ch_temp != EOF);
}

//return press char, Or 0
static char get_char(struct timeval *tv)
{
    fd_set fds;
    int ret;
    char ch = 0;
    FD_ZERO(&fds);
    FD_SET(0, &fds);

    ret = select(1, &fds, NULL, NULL, tv);
    if(ret > 0){
        if(FD_ISSET(0, &fds)){
            ch = getchar();
        }
    }else   if(ret < 0){
        printf("select Error.\n");
        //        return -1;
    }
    return ch;
}

static void wait(unsigned int u32Sec, unsigned int u32uSec)
{
    struct timeval  stTime;

    stTime.tv_sec = u32Sec;
    stTime.tv_usec= u32uSec;
    select(0, 0, 0, 0, &stTime);

    return;
}

int test_eeprom(void)
{
    char eeprom_char[32],temp[32];
    int ret,num_mac;
    char e2prom_test = 'n';
    fprintf(stdout,"***Warnning: This step will Delet EEPROM data****\n");
    fprintf(stdout,"Select if Continue or not [y/n defalut:n]:");
    memcpy(eeprom_char,"5E:A2:F7:81:43:43", BSP_MAC_LENGTH);
    clear_stdin();
    scanf("%[yn]", &e2prom_test);
    if(e2prom_test == 'n'){
        fprintf(stdout,"Select 'n' and Quit eeprom test.\n");
        return 0;
    }
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
    fprintf(stdout, "eeprom test Over.\n");
    return BSP_OK;
 loop:
    return -1;

}
int test_temp(void)
{
    float temp;
    int i = 0;
    while(i < 2)
    {
        if(BSP_GetCPUTemperature(&temp) != BSP_OK)
            return -1;

        printf("[%d]: CPU temperature: %5.3f\n",i, temp);
        i ++;
        wait(1, 0);
    };
    return BSP_OK;
}

extern int test_ds28cn01(void);

int test_alarmInOut(void)
{
    int ret;
    char ch;
    int sensor_in,alarm_out;
    int i = 0;
    fd_set fds;
    struct timeval tv;
    if( (  gLocal_boardInfo.alarmOut_num == 0) && ( gLocal_boardInfo.sensorIn_num == 0)){
        fprintf(stdout, "This board has No Alarm In/Out port.EXIT.\n");
        return 0;
    }
    fprintf(stdout, "This board has %d alarmOut and %d sensorIn.\n",
            gLocal_boardInfo.alarmOut_num, gLocal_boardInfo.sensorIn_num);

    while(1){
        
        if(i < gLocal_boardInfo.alarmOut_num){
            alarm_out = 1 << i;
            i ++;
        } else{
            i = 0;
            continue;
        }
        //        printf("setting alarm out 0x%x.\n",alarm_out);
        if(BSP_GetSensorIn(&sensor_in) != BSP_OK){
            printf("BSP_getSensorIn Error.\n");
            goto err_exit;
        }else{
            printf("BSP_getSensor in is 0x%04x.\n",sensor_in);
        }

        if(BSP_SetAlarmOut( alarm_out ) != BSP_OK)
            goto err_exit;

        if(BSP_GetAlarmOut(&alarm_out) != BSP_OK){
            printf("BSP_getAlarmOunt Error.\n");
            goto err_exit;
        }else
            printf("Current Alarm Out is 0x%02x\n",alarm_out);
        
        printf("\n");

        tv.tv_sec = 4;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(0, &fds);

        ret = select(1, &fds, NULL, NULL,&tv);
        if(ret > 0){
            if(FD_ISSET(0, &fds)){
                ch = getchar();
                //   clear_stdin();
                if(ch == 'q'){
                    break;
                } else
                    printf("***Press 'q' to quit Test****\n\n");
            }
        }else   if(ret < 0){
            printf("select Error.\n");
            goto err_exit;
        }
    }
    return BSP_OK;

 err_exit:
    return -1;
}
int test_statusled(void)
{
    int ret = BSP_OK;
    char ch;
    struct timeval tv;
    printf("***Press 'q' to Quit current test****\n");
    while(1){
 
        tv.tv_sec = 1;
        tv.tv_usec = 0;
#define LED_TEST
#ifdef LED_TEST
        if(BSP_runLedSet(BSP_LED_ON) != BSP_OK){
            printf("BSP_runLedSet Error.\n");
            ret = -1;
            break;
        }
#endif
        if(BSP_errLedSet(BSP_LED_ON) != BSP_OK){
            printf("BSP_errLedSet Error.\n");
            ret = -1;
            break;
        }

        wait(1, 0);

#ifdef LED_TEST
        //test led off
        if(BSP_runLedSet(BSP_LED_OFF) != BSP_OK){
            printf("BSP_runLedSet Error.\n");
            ret = -1;
            break;
        }
#endif
        if(BSP_errLedSet(BSP_LED_OFF) != BSP_OK){
            printf("BSP_errLedSet Error.\n");
            ret = -1;
            break;
        }
        ch = get_char( &tv);
        if(ch == 'q')
            break;
        else{
            printf("***Press 'q' to Quit current test****\n");
            //            clear_stdin();            
        }
        wait(0, 5000);
    }

    return ret;    
}
int test_cpldfunc(void)
{
    int ret = BSP_OK;
    char ch;
    int update_signal;
    int powerdown_signal;
    int i;
    char gTest_cpldMenu[] = {
        /* /\* /\\* "\r\n" *\\/ *\/ */
        "\r\n"
        "\r\n =========================="
        "\r\n     cpldFun Test Menu"
        "\r\n =========================="
        "\r\n"
        "\r\n 1: update/restore Key test"
        "\r\n 2: videoIn status led test [Only iDVR support]"
        "\r\n 3: check Power down key signal"
        "\r\n 4: shut down function test (will shut down Power)"
        "\r\n"
        "\r\n q: return to prev Menu"
        "\r\n"
        "\r\n Enter Choice: "
    };
    printf(gTest_cpldMenu);
    fflush(stdout);
    int test_flag = 0;
    struct timeval tv;
    char halt_flag = 'n';
    while(1){
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        ch = get_char( &tv);
        if(ch > 0){
            i = 0;
            switch(ch){
            case 'q':
                ret = BSP_OK;
                goto err_cpld;
            case '1':
                test_flag = 1;
                break;
            case '2':
                test_flag = 2;
                break;
            case '3':
                test_flag = 3;
                break;
            case '4':
                test_flag = 4;
                break;
            default:
                test_flag = 0;
                printf(gTest_cpldMenu);
                fflush(stdout);
            }
        }

        switch (test_flag ){
        case 1:
            if(BSP_cpldReadUpdateSignal(&update_signal) != BSP_OK){
                BSP_Print(BSP_PRINT_LEVEL_ERROR,"\nSOFT Version update signal read Error.");
                ret = -1;
                goto err_cpld;
            } 
            if(update_signal == BSP_INVALID_VALUE){
                printf("\nCurrent Board do Not support this Function.");
                ret = -1;
                goto err_cpld;
            }
            printf("\n[%4d]: SOFT Version update stauts is 0x%02x.",i,update_signal);
            usleep(500*1000);
            i ++;
            break;
        case 2:
            if(BSP_cpldShowVideoInputStatus() != BSP_OK){
                BSP_Print(BSP_PRINT_LEVEL_ERROR,"[%4d]:cpld Get Video input status Error.\n",i);
                ret = -1;
                goto err_cpld;
            }else
                fprintf(stdout, "[%4d]: Get Video input status Ok.\n", i);
            sleep(1);
            i ++;
            break;
        case 3:
            if(BSP_cpldReadPowerDownSignal(&powerdown_signal) != BSP_OK){
                fprintf(stderr,"[%4d]: cplde Get powerdown singnal Error.\n",i);
                ret  = -1;
                goto err_cpld;
            }

            if(powerdown_signal == BSP_INVALID_VALUE)
                fprintf(stdout, "[%4d]: Current Board Not support this Function.\n", i);
            else{
                if(powerdown_signal == TRUE)
                    fprintf(stdout, "[%4d]: Power Key has Pressed down Once.\n", i);
                else
                    fprintf(stdout, "[%4d]: Power key Not Pressed down.\n", i);
            }
            i ++;
            sleep(1);
            break;
        case 4:
            fprintf(stdout, "****WARNNING: This Test will power down board.*****\n");
            fprintf(stdout, "if Continue Or not [y/n default:n]:");
            clear_stdin();
            scanf("%[yn]", &halt_flag);
            if(halt_flag == 'y'){
                fprintf(stdout, "\n*****WARNING: System Power down right now****\n");
                BSP_cpldShutdownPower();
                fprintf(stdout, "\n******halt*****\n");
            }
            else {
                fprintf(stdout, "\n****Power down Concel****\n");
                test_flag = 0;
            }
            break;
        default:
            break;
        }
            
        usleep(50000);
    }
    
 err_cpld:
    return ret;
}

//uart_type :1 485 0 232
extern int setup_port(int fd, int baud, int databits, int parity, int stopbits);
extern int read_data(int fd, void *buf, int len);
extern int write_data(int fd, void *buf, int len);

int test_uart(int uart_type)
{
    int ret = BSP_OK;
    int baud,databits,stopbits,parity;
    char uart_dev[32];
    int fd;
    char set_default = 'y';
    int dev_num;

 input_port:
    if(uart_type){ // uart485 test
        if(gLocal_boardInfo.uart485_num == 0){
            printf("\nThis Board has no uart485 port, Return.\n");
            return 0;
        }

        printf("\nInput 485 port num <0~%d>:",gLocal_boardInfo.uart485_num -1);
        clear_stdin();
        scanf("%d",&dev_num);
        if(dev_num < gLocal_boardInfo.uart485_num)
            strcpy(uart_dev,gLocal_boardInfo.uart485_dev[dev_num]);
        else {
            printf("\nInvalid port num.There is Only %d ports.",
                gLocal_boardInfo.uart485_num);
            goto input_port;
        }
    } else { //uart232 test
        if(gLocal_boardInfo.uart232_num == 0){
            printf("\nThis Board has no uart232 port, Return.\n");
            return 0;
        }

        printf("\nInput 232 port num <0~%d>:", gLocal_boardInfo.uart232_num -1);
        clear_stdin();
        scanf("%d",&dev_num);
        if(dev_num < gLocal_boardInfo.uart232_num)
            strcpy(uart_dev, gLocal_boardInfo.uart232_dev[dev_num]);
        else {
            printf("\nInvalid port num.There is Only %d ports.",
                gLocal_boardInfo.uart232_num);
            goto input_port;
        }
    }

    printf("\n<%s>if using uart default param <115200-8-0-1> <y/n default:y>:",uart_dev);
    clear_stdin();
    //scanf("%[yn]", &set_default);
    scanf("%c", &set_default);
    printf("\n %s test input set_default is '%c'\n",uart_dev, set_default);

    if(set_default != 'n'){
        baud = 115200;
        databits = 8;
        stopbits = 1;
        parity = 0;
    } else {

    input_baud:
        printf("\nInput uart baudrate:");
        clear_stdin();
        scanf("%d",&baud);
        if ((baud < 0) || (baud > 921600)) {
            fprintf(stderr, "\nInvalid baudrate!");
            goto input_baud;
        }

    input_databits:
        printf("\nInput databits(5~8) :");
        clear_stdin();
        scanf("%d",&databits);
        if ((databits < 5) || (databits > 8))
        {
            fprintf(stderr, "\nInvalid databits!");
            goto input_databits;
        }

    input_parity:
        printf("\nInput parity(0:none,1:Odd,2:Even):");
        clear_stdin();
        scanf("%d", &parity);
        if ((parity < 0) || (parity > 2))
        {
            fprintf(stderr, "\nInvalid parity!");
            goto input_parity;
        }

    input_stopbits:
        printf("\nInput stopbits (1,2):");
        clear_stdin();
        scanf("%d", &stopbits);
        if ((stopbits < 1) || (stopbits > 2))
        {
            fprintf(stderr, "\nInvalid stopbits!");
            goto input_stopbits;
        }
    }

    fd = open(uart_dev, O_RDWR);
    if(fd < 0){
        fprintf(stderr, "\nopen <%s> Error:%s.\n",uart_dev, strerror(errno));
        return -1;
    }
    printf("\nuart <%s> setting: baud <%d>,databits <%d>,parity <%d>,stopbits <%d>.\n",
           uart_dev,baud, databits, parity, stopbits);
    if (setup_port(fd, baud, databits, parity, stopbits))
    {
        fprintf(stderr, "setup_port error %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    char ch;
    char test_uartMenu[] = {
        "\r\n"
        "\r\n ============="
        "\r\n UART-RUN Menu"
        "\r\n ============="
        "\r\n"
        "\r\n s: uart Send test"
        "\r\n r: uart Recieve test"
        "\r\n"
        "\r\n q: return to prev Menu"
        "\r\n"
        "\r\n Enter Choice: "
    };
        
    int read_or_write = 0xff;
    int len;
#define MAX_BUF_SIZE 1024
    char buf[MAX_BUF_SIZE + 2];
#define MY_END_CHAR      0x13
    printf(test_uartMenu);
    //    clear_stdin();
    while(1){
        ch = getchar();
        //        clear_stdin();
        switch (ch) {
        case 's':
            read_or_write = 1;
            break;
        case 'r':
            read_or_write = 0;
            break;
        case 'q':
            close(fd);
            return 0;    
        default:
            printf(test_uartMenu);      
        }
        
 
       if(read_or_write == 1){
             fprintf(stderr, "\nPlease Input data to send:\n");
             while (1){
                 len = read(0, buf, MAX_BUF_SIZE);
                 if (len == 1) {
                     buf[0] = MY_END_CHAR;
                     buf[1] = 0;
                     write_data(fd, buf, len);
                     read_or_write = 0xff;
                     break;
                 }

                 /* send a pack */
                 ret = write_data(fd, buf, len);
                 if (ret == 0) {
                     fprintf(stderr, "Send data error!\n");
                     read_or_write = 0xff;
                     break;
                 }
                 usleep(10000);
             }
       }
       
       if(read_or_write == 0) { //recv data
           fd_set rds;
           
           struct timeval tv;
 
           len = MAX_BUF_SIZE;
           printf("Begin to recv:\n");
           printf("***Press 'q' to quit Recieve Test****\n");
           while(1){
               tv.tv_sec = 0;
               tv.tv_usec = 10000;
               FD_ZERO(&rds);
               FD_SET(fd, &rds);
               FD_SET(0, &rds);
               ret = select(fd +1, &rds, NULL, NULL, &tv);
               if(ret > 0){
                   if(FD_ISSET(fd, &rds)){   // if has data to read
                       ret = read_data(fd, buf, len);
                       if (ret > 0) {
                           printf("%s", buf);
                           fflush(stdout);
                           if (buf[ret-1] == MY_END_CHAR) {
                               break;
                           }
                       }
                   }

                   if(FD_ISSET(0, &rds)){
                       ch = getchar();
                       //   clear_stdin();
                       if(ch == 'q'){
                           read_or_write = 0xff;
                           break;
                       }else
                           printf("***Press 'q' to quit Recieve Test****\n");
                   }
               }else   if(ret < 0){
                   printf("select Error.\n");
                   goto uart_exit;
               }
               //               usleep(20);
           }
       }
       usleep(5000);            
    }

    close(fd);
    return 0;    
 uart_exit:
    close(fd);
    return -1;    
}
extern int test_disk(void);

int test_rtc(void)
{
    char cmd_buf[64];
    char *time = "2014-12-17 14:19:00";
    printf("\n====Test rtc IC===\n");
    //    printf("****Orignal system Time is: ");
    //    system("date");
    //    usleep(10000);
    printf("****Setting new time is: <%s>\n",time);
    sprintf(cmd_buf,"date -s \"%s\"",time);
    system(cmd_buf);
    printf("****Saving to hardware IC.\n");
    system("hwclock -wu");
    system("hwclock -ru");
    return 0;
}

#define NET_TEST_SH "/usr/bin/hw_net_test.sh"

static char gRemote_ipaddr[64];
static char gLocal_ipaddr[64];

static int test_net(void)
{
    int ret;
    char cmd_buf[256];
    int port_num = 0;
    char hname[128];
    char *ptemp;
    char default_config = 'y';

    if(strlen(gRemote_ipaddr) == 0){
        fprintf(stdout, "\n *****Not configure remote ipaddr, EXIT****\n");
        return -1;
    }
    
 netPort_input:
    clear_stdin();
    fprintf(stdout,"\nInput test NetPort <0~%d>:", gLocal_boardInfo.netPort_num -1);
    scanf("%d", &port_num);
    if(port_num >= gLocal_boardInfo.netPort_num){
        fprintf(stderr,"\nInvalid netPort num, Select again.\n");
        goto netPort_input;
    }
    if(strlen(gLocal_ipaddr) == 0){
        fprintf(stdout, "\nLocal board ip is default, remote ip is %s.\n",gRemote_ipaddr);
        sprintf(cmd_buf, "%s eth%d %s",NET_TEST_SH, port_num, gRemote_ipaddr);    
        
    }else {
        fprintf(stdout, "\nLocal board ip is %s, remote ip is %s.\n",gLocal_ipaddr,gRemote_ipaddr);
        sprintf(cmd_buf, "%s eth%d %s %s",NET_TEST_SH, port_num, gLocal_ipaddr, gRemote_ipaddr);    
    }
    fprintf(stdout,"\n");
    system(cmd_buf);
    return 0;
}

int test_display(void)
{
#define HW_DISPLAY_TEST_FILE "/usr/bin/hw_display_test.sh"
    char cmd_buf[128];
    int ret;
    //    BSP_Print(BSP_PRINT_LEVEL_DEBUG, " Current FunVersion is 0x%x.\n", )
    switch (gPCB_VersionID){
    case PCB_VERSION_DECD_ID:
        switch (gFUNC_VersionID){
        case FUN_VERSION_IVSA_ID:
        case FUN_VERSION_IVSD_ID:
            sprintf(cmd_buf, "%s ivs 1", HW_DISPLAY_TEST_FILE);
            break;
        case FUN_VERSION_HDCA_ID:
            sprintf(cmd_buf, "%s dec 1", HW_DISPLAY_TEST_FILE);
            break;
        default:
            break;
        }
        break;
    case PCB_VERSION_DECE_ID:
        sprintf(cmd_buf, "%s dec 1", HW_DISPLAY_TEST_FILE);
        break;
    case PCB_VERSION_IDVR_ID:
        switch (gFUNC_VersionID){
        case FUN_VERSION_KDVR1608_ID:
            sprintf(cmd_buf, "%s dvr 0", HW_DISPLAY_TEST_FILE);
            break;
        case FUN_VERSION_KTM6102_ID:
            sprintf(cmd_buf, "%s ivs 0", HW_DISPLAY_TEST_FILE);
            break;
        default:
            sprintf(cmd_buf, "%s", HW_DISPLAY_TEST_FILE);
            break;
        }
        break;
        if(gFUNC_VersionID == FUN_VERSION_KDVR1608_ID)
        break;
    case PCB_VERSION_IAMB_ID:
    case PCB_VERSION_IAMC_ID:
        sprintf(cmd_buf, "%s ivs 0", HW_DISPLAY_TEST_FILE);
        break;
    default:
        fprintf(stdout, "***Invalid Board ID***\n");
        sprintf(cmd_buf, "%s", HW_DISPLAY_TEST_FILE);
        //                return -1;
    }

    ret = system(cmd_buf);
    if(ret < 0){
        fprintf(stdout, "%s: system run Error.\n");
        return -1;
    }
    return 0;
}

int main(int argc,char *argv[])
{
    int ret;
    int debug_level = BSP_PRINT_LEVEL_COMMON;
    char *optstring = "l:r:p:h";
    int opt;
    char ch;
    memset(gLocal_ipaddr,  0, sizeof(gLocal_ipaddr));
    memset(gRemote_ipaddr, 0, sizeof(gRemote_ipaddr));
    memset((char *)&gLocal_boardInfo, 0, sizeof(struct BSP_boardInfo));

    while (1){
        opt = getopt(argc, argv, optstring);
        if(opt == -1)
            break;
        switch (opt){
        case 'p':
            debug_level = atoi(optarg);
            break;
        case 'l':
            strcpy(gLocal_ipaddr,optarg);
            break;
        case 'r':
            strcpy(gRemote_ipaddr, optarg);
            break;
        case 'h':
        default:
            usage(argv[0]);
            return 0;
        }
    }
    
    fprintf(stdout,"\n*******Test Starting**********\n");

    BSP_PrintSetLevel(debug_level);

    if ( BSP_OK != BSP_Init() )
    {
        printf("BSP init error\n");
        return -1;
    }

#ifdef DEFINE_DVR_BOARD
    gLocal_boardInfo.uart485_num = 2;
    gLocal_boardInfo.uart232_num = 2;
    strcpy(gLocal_boardInfo.uart485_dev[0], "/dev/ttyO0");
    strcpy(gLocal_boardInfo.uart485_dev[1], "/dev/ttyMAX0");
    strcpy(gLocal_boardInfo.uart232_dev[0], "/dev/ttyMAX1");
    strcpy(gLocal_boardInfo.uart232_dev[1], "/dev/ttyO1");
    gLocal_boardInfo.sensorIn_num = 16;
    gLocal_boardInfo.alarmOut_num = 4;
    gLocal_boardInfo.netPort_num = 2;
#else
    memcpy((char *)&gLocal_boardInfo, (char *)&gBSP_boardInfo, sizeof(struct BSP_boardInfo));
#endif

    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"BoardInfo: uart485-%d, uart232-%d, sensorIn_num-%d, alarmOut_num-%d, netPort_num-%d.\n",
              gLocal_boardInfo.uart485_num, gLocal_boardInfo.uart232_num, gLocal_boardInfo.sensorIn_num, gLocal_boardInfo.alarmOut_num, gLocal_boardInfo.netPort_num);        

    print_runMenu();

    while(1){
        ch = getchar();
        //                clear_stdin();
        switch(ch ){
        case 'a'://auto-run test

            ret = test_temp();
            if(ret != 0)
                printf("\n\t****[2]:Temperature test Error****\n");
            else
                printf("\n\t****[2]:Temperture test OK****\n");

            ret = test_ds28cn01();
            if(ret != 0)
                printf("\n\t****[3]:ds28cn01 test Error****\n");
            else
                printf("\n\t****[3]:ds28cn01 test OK******\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
        case '0':
            ret = test_eeprom();
            if(ret != 0)
                printf("\n\t****[1]:EEPROM TEST ERROR.********\n");
            else
                printf("\n\t****[1]:EEPROM TEST OK.********\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;        
        case '1': // run-status led
            printf("run-status led start testing...\n");
            ret = test_statusled();
            if(ret != 0)
                printf("\n\t****[4]:status LED test Error*****\n");
            else
                printf("\n\t****[4]:status LED test Over******\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
        case '2'://cpld fun led
            ret = test_cpldfunc();
            if(ret != 0)
                printf("\n*****[5]:cpld fun test Error****\n");
            else
                printf("\n*****[5]:cpld fun test Over*****\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
        case '3'://uart485 test
            ret = test_uart(1);
            if(ret != 0)
                printf("\n*****[6]:uart485 test Error*******\n");
            else
                printf("\n*****[6]:uart485 test Over*******\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
        case '4'://uart232 test
            ret = test_uart(0);
            if(ret != 0)
                printf("\n*****[6]:uart485 test Error*******\n");
            else
                printf("\n*****[6]:uart485 test Over*******\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
        case '5':
            ret = test_disk();
            if(ret < 0)
                printf("\n*****[7]: Disk test Error*******\n");
            else
                printf("\n*****[7]: Disk test Over*******\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
        case '6':
            ret = test_rtc();
            if(ret < 0)
                printf("\n*****[8]: RTC test Error*******\n");
            else
                printf("\n*****[8]: RTC test Over*******\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
        case '7':
            ret = test_alarmInOut();
            if(ret < 0)
                printf("\n*****[9]: Alarm out/in test Error*******\n");
            else
                printf("\n*****[9]: Alarm out/in test Over*******\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
        case '8':
            ret = test_net();
            if(ret < 0)
                printf("\n*****[10]: net test Error*******\n");
            else
                printf("\n*****[10]: net test Over*******\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
        case '9':
            ret = test_display();
            if(ret < 0)
                printf("\n*****[10]: display test Error*******\n");
            else
                printf("\n*****[10]: display test Over*******\n");
            fprintf(stdout, "\n***** Press Any Key to Continue****\n");
            getchar();
            break;
            
        case 'e':
            goto test_exit;
        default:
            print_runMenu();
        }
        usleep(10000);
    }

 test_exit:
    BSP_Uninit();
    return 0;
}

