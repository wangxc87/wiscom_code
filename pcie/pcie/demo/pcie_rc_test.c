#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "pcie_std.h"
#include "pcie_common.h"
#include "ti81xx_rc_lib.h"

#define TEST_DATA_SIZE (1<<20)
#define TEST_TOTAL_DATA (500llu*(1<<20))

void *sendCmd_thread(void *arg)
{
    int ret;
    printf("%s create OK.\n", __func__);
    while(1){
        OSA_pcieSendCmd(NULL);
        usleep(50*1000);
    }
    return 0;
}

void *waitCmd_thread(void *arg)
{
    int ret;
    printf("%s create Ok.\n", __func__);
    sleep(1);
    while(1){
        OSA_pcieWaitCmd(NULL);
        usleep(50*1000);
    }
    return 0;
}

Int32 main(Int32 argc,char *argv)
{
    Int32 ret;
    unsigned long long i,loop_count;
    char *data_bufPtr;
    char send_data[TEST_DATA_SIZE];    
    unsigned int *u32Ptr = NULL;
    
    printf("Version: %s %s\n",__TIME__,__DATE__);

    ret = pcieRc_init();
    if(ret < 0){
        printf("Pcie master init Error.\n");
        return -1;
    }
    printf("Pcie master init OK.\n");

    loop_count = TEST_TOTAL_DATA/TEST_DATA_SIZE;
    printf("Test will send data size %llu, loop %llu.\n",
           TEST_TOTAL_DATA, loop_count);
    
    data_bufPtr = OSA_pciReqDataBuf(TEST_DATA_SIZE);
    if(data_bufPtr == NULL){
        printf("requre data buf Error.\n");
        return -1;
    }

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
    
    i = 0;
    //    while(1){
    u32Ptr = (unsigned int *)send_data;
    for(i = 0; i < TEST_DATA_SIZE/4; i++){
        *u32Ptr = i;
        u32Ptr ++;
    }
    
    for(i = 0; i < loop_count; i++){
        //        memset(data_bufPtr, 97, TEST_DATA_SIZE);
        printf("Pcie send data %llu.\n", i);
        ret = OSA_pcieSendData(send_data, TEST_DATA_SIZE, EP_ID_ALL,0);
        if(ret < 0){
            printf("Pcie send data %llu Error.\n", i);
            goto err_exit;
        }
#ifndef THPT_TEST
        if(i%128 == 0)
            printf("master has send data %llu-%u.\n", i,ret);
#endif
    }
    printf("Total send data loop %llu, size is %llu MBit.\n", i, (i*TEST_DATA_SIZE)>>20);
 err_exit:
    pcieRc_deInit();    
    printf("Pcie master Test exit.\n");

    return 0;
}









