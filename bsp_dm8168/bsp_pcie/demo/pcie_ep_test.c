#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include "pcie_std.h"
#include "ti81xx_ep.h"
#include "pcie_common.h"
#include "osa_pcie.h"

#define RECV_MAX_SIZE (3<<20)
#define START_SEND_DATA   0xfe

static UInt32 cur_time,prev_time,total_time;
#define TEST_PCIE_MSI

void *sendCmd_thread(void *arg)
{
    int ret, i = 0;
    char cmd_buf[2*1024];
    printf("%s create OK.\n", __func__);
    while(1){
        sprintf(cmd_buf,"This Cmd-%d from pcie-%d.", i, gLocal_id);
        ret = OSA_pcieSendCmd(RC_ID, cmd_buf, 128, NULL);
        if(ret < 0){
            printf("%s: send cmd to pcie-%d failed.\n", __func__, i);
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
    printf("%s create Ok.\n", __func__);
    while(1){
        recv_size = OSA_pcieRecvCmd(recv_cmd, &from_id, NULL);
        if(recv_size > 0){
            printf("%s: From pcie-%d info [size: %d] : %s.\n", __func__, from_id, recv_size, recv_cmd);
        }else
            printf("%s: recvCmd failed.\n", __func__);

    }
    return 0;
}

int usage(char *string)
{
    printf("%s usage:\n", string);
    printf("\t %s <-c>\n", string);
    printf("\t -c: enable compare ,[default disable]\n");
    printf("\t -C: enable/disable send cmd, default enable\n");
    printf("\t -f:  test forever\n");
    printf("\t -p: print info interval,0: no print, default 256\n");
    printf("\t -h: help info \n");
    return 0;
}
static int gTest_exit = 0;
void signal_fxn(int signo)
{
    if(signo == SIGQUIT)
        gTest_exit = 1;
}
Int32 main(Int32 argc,char *argv[])
{
    Int32 ret;
    char *local_data_buf;
    unsigned long long total_data_size = 0;
    
    char *optstring = "fcp:hC:";
    int opt;
    int test_forever, test_compare,test_speed, test_cmd;
    char exe_name[64];
    UInt32 test_count = 0;
    UInt32 print_int = 256;
    
    test_forever = 0;
    test_compare = 0;
    test_speed = 0;
    test_cmd = 1;
    strcpy(exe_name, argv[0]);
    printf("Version: %s %s\n",__TIME__,__DATE__);
    signal(SIGQUIT, signal_fxn);
    while(1){
        opt = getopt(argc,argv,optstring);
        if(opt < 0)
            break;
        switch(opt){
        case 'f':
            test_forever = 1;
            break;
        case 'c':
            test_compare = 1;
            break;
        /* case 'p': */
        /*     test_speed = 1; */
        /*     break; */
        case 'C':
            test_cmd = atoi(optarg);
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
    

    local_data_buf = (char *)malloc(RECV_MAX_SIZE);
    if(local_data_buf == NULL){
        printf("local_databuf malloc Error, Exit.\n");
        return -1;
    }

    ret = OSA_pcieInit(NULL);
    if(ret < 0){
        printf("pcie slave init Error.\n");
        return -1;
    }
    printf("pci slave init successful.\n");

    unsigned int i = 0,j;
    UInt32 recv_size;
    Int32 *tmp_ptr;
    cur_time = 0;
    total_time = 0;
    prev_time = 0;

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
    }

    ret = pthread_create(&pcie_waitCmd_thread, 0, (void *)waitCmd_thread,0);
    if(ret < 0){
        printf("waitCmd thread Create Error %s\n", strerror(errno));
        return -1;
    }
    /* pthread_join(pcie_sendCmd_thread, 0); */
    /* pthread_join(pcie_waitCmd_thread, 0); */
#endif

    int channel_id;
    do     {
            total_time = 0;
            total_data_size = 0;
            *local_data_buf = 0;
            i = 0;
            while(1){
                    ret = OSA_pcieRecvData(local_data_buf, RECV_MAX_SIZE, &channel_id,0);
                    if(ret < 0){
                            //         if(ret != PCIEDEV_EBUSY)
                            printf("pcie slave receive data Error.\n");
                            break;
                    }
                    if(START_SEND_DATA == *local_data_buf)
                            break;
            }
            printf("#####Pcie-%d recieve START_SEND_DATA flag####\n", gLocal_id);

            ret = pcie_slave_getCurTime(&prev_time);
            if(ret < 0){
                    printf("get Cur time Error, exit.\n");
                    goto err_exit;
            } 

            while(!gTest_exit){
                    /* ret = pcie_slave_getCurTime(&prev_time); */
                    /* if(ret < 0){ */
                    /*         printf("get Cur time Error, exit.\n"); */
                    /*         goto err_exit; */
                    /* }  */
                    ret = OSA_pcieRecvData(local_data_buf, RECV_MAX_SIZE, &channel_id,0);
                    if(ret < 0){
//            if(ret != PCIEDEV_EBUSY)
                            printf("pcie slave receive data Error.\n");
                            break;
                    }
                    if(ret == 0)
                            break;  //EOF flag

                    recv_size = ret;
                    total_data_size += recv_size;
                    tmp_ptr = (Int32 *)local_data_buf; 
#ifndef THPT_TEST
                    if((i%print_int == 0) && print_int)
                            printf("Pcie slave has recieved Data %u-%d.\n", i, recv_size);
#endif

                    /* ret = pcie_slave_getCurTime(&cur_time); */
                    /* if(ret < 0){ */
                    /*         printf("get Cur time Error, exit.\n"); */
                    /*         goto err_exit; */
                    /* } */
                    /* total_time +=cur_time - prev_time; */
                    if(test_compare){
                            for(j = 0; j < recv_size/4; j++){
                                    if(*tmp_ptr != j ){
                                            printf("Pcie slave Check Recieved data Error ,%u-%u-0x%x.\n", i, j, *tmp_ptr);
                                            goto err_exit;      
                                    }else{
                                            *tmp_ptr = 0;
                                            tmp_ptr ++;
                                    }
                            }
                    }
                    i ++;
                    /* if(test_speed) */
                    /*         usleep(10000); //waiting rc send data */
            }//end of while(1)
            ret = pcie_slave_getCurTime(&cur_time);
            if(ret < 0){
                    printf("get Cur time Error, exit.\n");
                    goto err_exit;
            }
            total_time =cur_time - prev_time;

#define HZ 100
            test_count ++;
        unsigned int data_size_tmp;
        data_size_tmp =(UInt32)( total_data_size /(1<<20));
//        if(!test_speed)
        printf("THTP TEST [%u]:\n", test_count);
        printf("\tReceive data loop %u,total size %u MBit in %u jiffies.\n",
               i,data_size_tmp ,total_time);
        if(total_time > 0)
                printf("\tTHPT calculated in RX is: %f MBPS\n ",
                       (float)((UInt32)data_size_tmp) * HZ/total_time);

    }while(test_forever && !gTest_exit);

        
 err_exit:
        
#ifdef TEST_PCIE_MSI
    if(test_cmd){
        pthread_cancel(pcie_sendCmd_thread);
    }
    pthread_cancel(pcie_waitCmd_thread);
#endif
    
    free(local_data_buf);
    OSA_pcieDeInit();

    printf("Pcie slave Test exit.\n");

    return 0;
}
