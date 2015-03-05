#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "pcie_std.h"
#include "ti81xx_ep.h"
#include "pcie_common.h"

#define PCIE_DATA_BUF_SIZE (3<<20)

static UInt32 cur_time,prev_time,total_time;


void *sendCmd_thread(void *arg)
{
    int ret;
    printf("%s create OK.\n", __func__);
    while(1){
        pcie_slave_sendCmd(NULL);
        usleep(50*1000);
    }
    return 0;
}

void *waitCmd_thread(void *arg)
{
    int ret;
    printf("%s create Ok.\n", __func__);
    sleep(5);
    while(1){
        pcie_slave_waitCmd(NULL);
        usleep(50*1000);
    }
    return 0;
}

Int32 main(Int32 argc,char *argv)
{
    Int32 ret;
    char *data_buf;
    char *local_data_buf;
    printf("Version: %s %s\n",__TIME__,__DATE__);

    local_data_buf = malloc(PCIE_DATA_BUF_SIZE);
    if(local_data_buf == NULL){
        printf("local_databuf malloc Error, Exit.\n");
        return -1;
    }

    ret = pcie_slave_init();
    if(ret < 0){
        printf("pcie slave init Error.\n");
        return -1;
    }
    printf("pci slave init successful.\n");

    data_buf = pcie_slave_reqDatabuf(PCIE_DATA_BUF_SIZE);
    if(data_buf == NULL){
        printf("pcie slave require data buf Error.\n");
        return -1;
    }

    unsigned int i = 0,j;
    UInt32 recv_size;
    Int32 *tmp_ptr;
    cur_time = 0;
    total_time = 0;
    prev_time = 0;

#ifdef TEST_PCIE_MSI
    pthread_t pcie_sendCmd_thread;
    pthread_t pcie_waitCmd_thread;
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
    pthread_join(pcie_sendCmd_thread, 0);
    pthread_join(pcie_waitCmd_thread, 0);
#endif
    
    while(1){
        ret = pcie_slave_getCurTime(&prev_time);
        if(ret < 0){
            printf("get Cur time Error, exit.\n");
            goto err_exit;
        } 
            
        ret = pcie_slave_recvData(data_buf, PCIE_DATA_BUF_SIZE, 0);
        if(ret < 0){
            printf("pcie slave receive data Error.\n");
            break;
        }
        recv_size = ret;

#ifndef THPT_TEST
        if(i%128 == 0)
            printf("Pcie slave has recieved Data %u-%d.\n", i, recv_size);
#endif

#ifdef EDMA_TRANSMISSION
        tmp_ptr = (Int32 *)data_buf;
#else
        memcpy(local_data_buf,data_buf, PCIE_DATA_BUF_SIZE);
        tmp_ptr = (Int32 *)local_data_buf;
#endif

        ret = pcie_slave_getCurTime(&cur_time);
        if(ret < 0){
            printf("get Cur time Error, exit.\n");
            goto err_exit;
        }
        total_time +=cur_time - prev_time;
#if 0
        for(j = 0; j < recv_size/4; j++){
            if(*tmp_ptr != 0x61616161 ){
                printf("Pcie slave Check Recieved data Error ,%u-%u-0x%x.\n", i, j, *tmp_ptr);
                goto err_exit;      
            }else{
                *tmp_ptr = 0;
                tmp_ptr ++;
            }
        }
#endif
        i ++;
        //        usleep(5000);
    }

    unsigned long long total_data_size;
    total_data_size = i*recv_size;
    
#define HZ 100
    printf("receive data loop %u,total size %lluMBit in %u jiffies.\n",
           i, total_data_size >> 20,total_time);
    printf("THPT calculated in RX is: %f MBPS\n ",
           (float)((((UInt32)total_data_size >> 20) * HZ)/total_time));

 err_exit:
    free(local_data_buf);
    pcie_slave_deInit();

    printf("Pcie slave Test exit.\n");

    return 0;
}
