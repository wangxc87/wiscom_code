#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <getopt.h>
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
int usage(char *string)
{
    printf("%s usage:\n", string);
    printf("\t %s <-c>\n", string);
    printf("\t -c: enable compare ,[default disable]\n");
    printf("\t -h: help info \n");
    return 0;
}
Int32 main(Int32 argc,char *argv[])
{
    Int32 ret;
    char *data_buf;
    char *local_data_buf;
    unsigned long long total_data_size = 0;
    
    char *optstring = "fch";
    int opt;
    int test_forever, test_compare;
    char exe_name[64];

    test_forever = 0;
    test_compare = 0;
    strcpy(exe_name, argv[0]);
    
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
        case 'h':
            usage(exe_name);
            return -1;
        default:
            usage(exe_name);
            return -1;
        }
    }
    
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
            if(ret != PCIEDEV_EBUSY)
                printf("pcie slave receive data Error.\n");
            break;
        }
        recv_size = ret;
        total_data_size += recv_size;

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
        usleep(10000); //waiting rc send data
    }

    
#define HZ 100
        unsigned int data_size_tmp;

        data_size_tmp =(UInt32)( total_data_size /(1<<20));
    printf("receive data loop %u,total size %u MBit in %u jiffies.\n",
           i,data_size_tmp ,total_time);
    printf("THPT calculated in RX is: %f MBPS\n ",
           (float)((((UInt32)data_size_tmp) * HZ)/total_time));

 err_exit:
    free(local_data_buf);
    pcie_slave_deInit();

    printf("Pcie slave Test exit.\n");

    return 0;
}
