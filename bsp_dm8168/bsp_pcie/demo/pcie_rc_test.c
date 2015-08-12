#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include "pcie_std.h"
#include "pcie_common.h"
#include "osa_pcie.h"
/* #include "ti81xx_rc_lib.h" */

#define TEST_DATA_SIZE (500*1024)
#define TEST_TOTAL_DATA (100llu*(1<<20))

#define TEST_PCIE_MSI
#ifdef TEST_PCIE_MSI
extern int gLocal_id;

static  int to_id = EP_ID_ALL;

void *sendCmd_thread(void *arg)
{
    int ret, i = 0;
    int ep_id = 0;
    char cmd_buf[2*1024];
    printf("%s create OK.\n", __func__);
    while(1){
        sprintf(cmd_buf,"This Cmd-%d from pcie-%d.",i, gLocal_id);
        if(to_id == EP_ID_ALL){
            ep_id = EP_ID_0;
            while(1){
                if(ep_id > EP_ID_2)
                    break;
                ret = OSA_pcieSendCmd(ep_id, cmd_buf, 128, NULL);
                if(ret < 0){
                    printf("%s: send cmd to pcie-%d failed.\n", __func__, ep_id);
                }
                ep_id ++;
            }
        }else {
            ret = OSA_pcieSendCmd(to_id, cmd_buf, 128, NULL);
            if(ret < 0){
                printf("%s: send cmd to pcie-%d failed.\n", __func__, ep_id);
            }
        }
        i ++;
        sleep(1);
    }
    return 0;
}

 void *waitCmd_thread(void *arg)
 {
     int recv_size, from_id;
     char recv_cmd[2*1024];
     struct timeval tv;
     printf("%s create Ok.\n", __func__);
     tv.tv_sec = 60;
     tv.tv_usec = 0;
     while(1){
#if 1
         recv_size = OSA_pcieRecvCmd(recv_cmd, &from_id, &tv);//NULL);
         if(recv_size > 0){
             printf("%s: From pcie-%d info [size: %d] : %s.\n", __func__, from_id, recv_size, recv_cmd);
         }else {
             printf("%s: recvCmd failed.\n", __func__);
         }
#else
         sleep(1);
#endif
     }
     return 0;
 }
#endif

int usage(char *string)
{
    printf("Usage:\n");
    printf("%s [-t to_id][-m sync_mode] [-s testData_size] <-f> <-c> <-C>\n", string);
    printf("\t -t: to_id, 0:all eps, 1:EP1, 2:EP2, 3:EP3, 4:all eps in p2p, default:0.\n");
    printf("\t -m: data send sync mode 1/0, default: 0-nosync \n");
    printf("\t -s: Total send size , default 5 [unit:100M]\n");
    printf("\t -u: single send buf size, default 200 [unit:1k]\n");
    printf("\t -f: enable forever test, default disable\n");
    printf("\t -c: enable compare test,default disable\n");
    printf("\t -C: enable/disable send cmd 1/0, default enable\n");
    printf("\t -d: enable/disable send data 1/0, default enable\n");
    printf("\t -p: print info interval,0: no print, default 256\n");
    return 0;
}

static int gTest_exit = 0;
void signal_fxn(int signo)
{
    if(signo == SIGQUIT)
        gTest_exit = 1;
}

#define START_SEND_DATA   0xfe

Int32 main(Int32 argc,char *argv[])
{
    Int32 ret;
    unsigned long long i = 0,loop_count,total_data;
    char send_data[1<<20];      //1M buf
    unsigned int *u32Ptr = NULL;
    unsigned int size_per_send = 200;
    
    unsigned int loop = 0;
    int j;
    char *optstring = "m:s:fchC:d:t:u:p:";
    int opt;
    int test_forever, test_compare,test_cmd, test_data, sync_mode;
    char exe_name[64];
    int print_int = 256;
    test_forever = 0;
    test_compare = 0;
    test_cmd = 1;
    test_data = 1;
    sync_mode = DATA_SEND_NOSYNC_MODE;
    
    strcpy(exe_name, argv[0]);

    total_data = TEST_TOTAL_DATA *  5;
    printf("Version: %s %s\n",__TIME__,__DATE__);
    
    while(1){
        opt = getopt(argc,argv,optstring);
        if(opt < 0)
            break;
        switch(opt){
        case 't':
            switch(atoi(optarg)){
            case 0:
                to_id = EP_ID_ALL;
                break;
            case 1:
            case 2:
            case 3:
            case 4:
                to_id = atoi(optarg);
                break;
            default:
                usage(exe_name);
                return -1;
            }
            break;
        case 'm':
            switch(atoi(optarg)){
            case  1:
                sync_mode = DATA_SEND_SYNC_MODE;
                break;
            case  0:
            default:
                sync_mode = DATA_SEND_NOSYNC_MODE;
                break;
            }
            break;
        case 's':
            total_data = TEST_TOTAL_DATA * atoi(optarg);
            break;
        case 'f':
            test_forever = 1;
            break;
        case 'c':
            test_compare = 1;
            break;
        case 'C':
            test_cmd = atoi(optarg);
            break;
        case 'd':
            test_data = atoi(optarg);
            break;
        case 'u':
                size_per_send = atoi(optarg);
                break;
        case 'p':
                print_int = atoi(optarg);
                break;
        case 'h':
            usage(exe_name);
            return -1;
        default:
            usage(exe_name);
            return -1;
        }
    }

    size_per_send *= 1024;
    
    signal(SIGQUIT, signal_fxn);
    ret = OSA_pcieInit(NULL);
    if(ret < 0){
        printf("Pcie master init Error.\n");
        return -1;
    }
    printf("Pcie master init OK.\n");

    loop_count = total_data/size_per_send;
    printf("Test will send data size %lluMB, loop %llu, size_per_send  %uKB.\n",
           total_data>>20, loop_count, size_per_send>>10);
    sleep(1);    

#ifdef TEST_PCIE_MSI
    pthread_t pcie_sendCmd_thread;
    pthread_t pcie_waitCmd_thread;
    if(test_cmd){
            printf("***Test pcie msi*****\n");
            ret = pthread_create(&pcie_sendCmd_thread, 0, (void *)sendCmd_thread,0);
            if(ret < 0){
                    printf("sendCmd thread Create Error %s\n", strerror(errno));
                    return -1;
            }

            ret = pthread_create(&pcie_waitCmd_thread, 0, (void *)waitCmd_thread,0);
            if(ret < 0){
                    printf("waitCmd thread Create Error %s\n", strerror(errno));
                    return -1;
            }
    }

#endif
    send_data[0] = START_SEND_DATA;
    if(to_id == 4){
            ret = OSA_pcieSendData(EP_ID_ALL, send_data, 512, 0, sync_mode, NULL);
            if(ret < 0){
                    printf("Pcie send data %llu Error.\n", i);
                    /* goto err_exit; */
            }
            
    }else {
            ret = OSA_pcieSendData(to_id, send_data, 512, 0, sync_mode, NULL);
            if(ret < 0){
                    printf("Pcie send data %llu Error.\n", i);
                    /* goto err_exit; */
            }
            
    }
    send_data[0] = 0;
    do{
        if(test_data) {
            for(i = 0; i < loop_count; i++){
                /* printf("Pcie send data %llu.\n", i); */
                if(test_compare){
                    u32Ptr = (unsigned int *)send_data;
                    for(j = 0; j < size_per_send/4; j++){
                        *u32Ptr = j;
                        u32Ptr ++;
                    }
                }

                if(to_id ==4){
                        for(j = EP_ID_0; j < EP_ID_0 + 3; j ++)   {
                                ret = OSA_pcieSendData(j , send_data, size_per_send, 0, sync_mode, NULL);
                                if(ret < 0){
                                        printf("Pcie send data %llu Error.\n", i);
                                        /* goto err_exit; */
                                }
                        }
                }else {
                        ret = OSA_pcieSendData(to_id, send_data, size_per_send, 0, sync_mode, NULL);
                        if(ret < 0){
                                printf("Pcie send data %llu Error.\n", i);
                                /* goto err_exit; */
                        }
                }
#ifndef THPT_TEST
                if((i%print_int == 0) && print_int)
                    printf("master has send data %llu-%u.\n", i,ret);
#endif
                if(gTest_exit)
                    break;
            }
            printf("[%u]:Total send data loop %llu, size is %llu MB\n",
                   loop++, i, (i*size_per_send)>>20);
            //            usleep(10*1000);
        }else
            sleep(1);
        
    }while(test_forever && !gTest_exit);

    //send 0 data to stop transfer
    if(to_id == 4){
            ret = OSA_pcieSendData(EP_ID_ALL, send_data, 0, 0, sync_mode, NULL);
            if(ret < 0){
                    printf("Pcie send data %llu Error.\n", i);
                    goto err_exit;
            }
    }else {
            ret = OSA_pcieSendData(to_id, send_data, 0, 0, sync_mode, NULL);
            if(ret < 0){
                    printf("Pcie send data %llu Error.\n", i);
                    goto err_exit;
            }
    }

 err_exit:
    
#ifdef TEST_PCIE_MSI
    if(test_cmd){
        pthread_cancel(pcie_sendCmd_thread);
        pthread_cancel(pcie_waitCmd_thread);
    }

#endif
    
    OSA_pcieDeInit();
    printf("Pcie master Test exit.\n");
    exit(0);
}









