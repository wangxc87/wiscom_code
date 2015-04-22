#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <getopt.h>

#include "pcie_std.h"
#include "pcie_common.h"
#include "ti81xx_rc_lib.h"

#define TEST_DATA_SIZE (1<<20)
#define TEST_TOTAL_DATA (500llu*(1<<20))

#ifdef TEST_PCIE_MSI
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
#endif

int usage(char *string)
{
    printf("Usage:\n");
    printf("%s <-m mode> <-s testData_size> <-f> <-c>\n", string);
    printf("\t -m: rc init mode, init all or some eps, 0:all 1:some default: 0\n");
    printf("\t -s: send data size per loop, default 1 [unit:500M]\n");
    printf("\t -f: enable forever test, default disable\n");
    printf("\t -c: enable compare test,default disable\n");
    return 0;
}

Int32 main(Int32 argc,char *argv[])
{
    Int32 ret;
    unsigned long long i,loop_count,total_data;
    char *data_bufPtr;
    char send_data[TEST_DATA_SIZE];    
    unsigned int *u32Ptr = NULL;
    int mode = INIT_ALL_EPS;

    unsigned int loop = 0;
    int j;
    char *optstring = "m:s:fch";
    int opt;
    int test_forever, test_compare;
    char exe_name[64];
    test_forever = 0;
    test_compare = 0;
    strcpy(exe_name, argv[0]);

    total_data = TEST_TOTAL_DATA;
    printf("Version: %s %s\n",__TIME__,__DATE__);
    
    while(1){
        opt = getopt(argc,argv,optstring);
        if(opt < 0)
            break;
        switch(opt){
        case 'm':
            switch(atoi(optarg)){
            case  1:
                mode = INIT_SOME_EPS;
                break;
            case  0:
            default:
                mode = INIT_ALL_EPS;
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
        case 'h':
            usage(exe_name);
            return -1;
        default:
            usage(exe_name);
            return -1;
        }
    }
    
    
    ret = pcieRc_init(mode);
    if(ret < 0){
        printf("Pcie master init Error.\n");
        return -1;
    }
    printf("Pcie master init OK.\n");

    loop_count = total_data/TEST_DATA_SIZE;
    printf("Test will send data size %llu, loop %llu.\n",
           total_data, loop_count);
    
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
    
    do{
        
        for(i = 0; i < loop_count; i++){
            /* printf("Pcie send data %llu.\n", i); */
            if(test_compare){
                u32Ptr = (unsigned int *)send_data;
                for(j = 0; j < TEST_DATA_SIZE/4; j++){
                    *u32Ptr = j;
                    u32Ptr ++;
                }
            }
            
            ret = OSA_pcieSendData(send_data, TEST_DATA_SIZE, EP_ID_ALL,0);
            if(ret < 0){
                printf("Pcie send data %llu Error.\n", i);
                goto err_exit;
            }
#ifndef THPT_TEST
            if(i%128 == 0)
                printf("master has send data %llu-%u.\n", i,ret);
#endif
            /* OSA_pcieSendCmd(NULL); */
        }
        printf("[%u]:Total send data loop %llu, size is %llu MBit.\n",
               loop++, i, (i*TEST_DATA_SIZE)>>20);
    }while(test_forever);

    //send 0 data to stop transfer
    ret = OSA_pcieSendData(send_data, 0, EP_ID_ALL,0);
    if(ret < 0){
        printf("Pcie send data %llu Error.\n", i);
        goto err_exit;
    }

 err_exit:
    pcieRc_deInit();    
    printf("Pcie master Test exit.\n");

    return 0;
}









