#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "pcie_std.h"
#include "ti81xx_ep.h"
#include "ti81xx_rc_lib.h"
#include "pcie_common.h"
#include "debug_msg.h"

int gDebug_enable = 0;

/* #define debug_print(fmt, ...) \ */
/* 	do { if (gDebug_enable) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \ */
/* 				__LINE__, __func__, ##__VA_ARGS__); } while (0) */

#define OPER_SEND 1
#define OPER_SAVE  2
#define BUF_SIZE 1024
char buf[BUF_SIZE];
#define PCIE_DATA_BUF_SIZE (1<<20)
#define HEAD_DATA 0x00
#define HEAD_END  0xff

static  int rc_init_mode = INIT_ALL_EPS;

int usage(char *string)
{
    printf("%s usage: \n", string);
    printf("Ver: %s %s \n", __TIME__,__DATE__);
    printf("   %s <op> <-m init_mode>  <-f file> \n", string);
    printf("\t op: -s send to eps(0/1/2,or 3:all)\n");
    printf("\t -m: rc init mode, 0:wait all, 1: wait some,default:0\n");
    printf("\t     -r recieve \n");
    printf("\t -f: send or save filename\n");
    /* printf("\t -d: enable/disalbe debug mode [default disalbe] \n"); */
    printf("example: %s -s  -f ./test.bin\n", string);
    return 0;
}


int send_file(char *file_name, int to_id)
{
    int ret;
    FILE *filep;
    unsigned int file_size = 0;
    char *data_bufPtr = NULL;
    int status,i;

    filep = fopen(file_name, "r");
    if(!filep){
        printf("open %s error.\n", file_name);
        return -1;
    }
    printf("open %s ok.\n", file_name);
    
    ret = pcieRc_init(rc_init_mode);
    if(ret < 0){
        printf("Pcie master init Error.\n");
        fclose(filep);
        return -1;
    }
    printf("Pcie master init OK.\n");

    data_bufPtr = OSA_pciReqDataBuf(PCIE_DATA_BUF_SIZE);
    if(data_bufPtr == NULL){
        printf("requre data buf Error.\n");
            ret = -1;
            goto exit0;
    }

    i = 0;
    do {
        *buf = HEAD_DATA;
        status = fread(buf+1, 1, BUF_SIZE-1, filep);
        debug_print("send %d-%d.\n",i++, status+1);
        ret = OSA_pcieSendData(buf, status+1, to_id,0);
        if(ret < 0){
            printf("Pcie send data %llu Error.\n", i);
            ret = -1;
            goto exit0;
        }
        file_size += status;
    } while(!feof(filep)&&(status == (BUF_SIZE -1)));
    
    *buf = HEAD_END;
    ret = OSA_pcieSendData(buf, 4, to_id,0);
    if(ret < 0){
        printf("Pcie send data %llu Error.\n", i);
        ret = -1;
        goto exit0;
    }
    
    printf("file %s send.Total size %u\n", file_name, file_size);

 exit0:
    pcieRc_deInit();        
    fclose(filep);
    return ret;
    
}

int recv_file(char *file_name)
{
    int ret;
    FILE *filep;
    unsigned int file_size = 0;
    char *data_buf;
    int status,i;

    filep = fopen(file_name, "wb");
    if(!filep){
        printf("open %s error.\n", file_name);
        return -1;
    }
    printf("open %s ok.\n", file_name);
    
    ret = pcie_slave_init();
    if(ret < 0){
        printf("pcie slave init Error.\n");
        fclose(filep);
        return -1;
    }
    printf("pci slave init successful.\n");

    data_buf = pcie_slave_reqDatabuf(PCIE_DATA_BUF_SIZE);
    if(data_buf == NULL){
        printf("pcie slave require data buf Error.\n");
        ret = -1;
        goto exit0;
    }

    i = 0;
    do{
        status = pcie_slave_recvData(data_buf, PCIE_DATA_BUF_SIZE, 0);
        if(status < 0){
            printf("pcie slave receive data Error.\n");
            break;
        }
        if(*data_buf == HEAD_END)
            break;

        debug_print("%s: recieve %d-%d.\n",__func__, i++,status-1);
        ret = fwrite(data_buf+1, 1, status-1, filep);
        if(ret <0){
            printf("write file %s error.\n", file_name);
            goto exit0;
        }
        file_size += status-1;
    }while(1);

    printf("file %s recieved. Total size %u\n", file_name, file_size);

 exit0:
    pcie_slave_deInit();
    fclose(filep);
    return ret;
}


int main(int argc,char *argv[])
{
    char *optstring  = "f:s:m:drh";
    int opt;
    int ret;
    int operation = 0, to_id = 0;
    char file_name[128];
    char exe_name[32];

    strcpy(exe_name, argv[0]);
    memset(file_name, 0 ,sizeof(file_name));
    
    if(argc < 2){
        usage(exe_name);
        return -1;
    }

    while(1) {
        opt = getopt(argc, argv, optstring);
        if(-1 == opt)
            break;

        switch(opt) {
        case 'm':
            switch(atoi(optarg)){
            case 1:
                rc_init_mode = INIT_SOME_EPS;
                break;
            case 0:
            default:
                rc_init_mode = INIT_ALL_EPS;
                break;
            }
            break;
        case 's':
            operation = OPER_SEND;
            switch (atoi(optarg)){
            case 0:
                to_id = EP_ID_O;
                break;
            case 1:
                to_id = EP_ID_1;
                break;
            case 2:
                to_id = EP_ID_2;
                break;
            case 3:
                to_id = EP_ID_ALL;
                break;
            default:
                printf("Invalid Arg.\n");
                usage(exe_name);
                return -1;
            }
            break;
        case 'r':
            operation = OPER_SAVE;
            break;
        case 'f':
            strcpy(file_name, optarg);
            break;
        case 'd':
            gDebug_enable = 1;
            break;
        case 'h':
            usage(exe_name);
            return 0;
        default:
            usage(exe_name);
            return -1;
        }
    }

    if(strlen(file_name) == 0){
        printf("Invalid file name.\n");
        usage(exe_name);
        return -1;
    }

    switch(operation){
    case OPER_SEND:
        ret = send_file(file_name, to_id);
        return ret;
    case OPER_SAVE:
        ret = recv_file(file_name);
        sprintf(buf,"chmod +x %s", file_name);
        system(buf);
        return ret;
    default:
        printf("Invalid arg.\n");
        usage(exe_name);
        return -1;
    }
    return 0;
}


