#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#include "osa_pcie.h"
#include "ti81xx_ep.h"

#define HZ 100
#define START_SEND_DATA   0xfe
#define RECV_MAX_SIZE (1<<20)

struct test_params {
        int recv_port;
        int num_udps;
        int size_per_send;
        int to_id;
        char buf[1<<20];
        int sync_mode;
        int print_int;
        int test_done;
};
struct test_params gTest_params;
extern int gLocal_id;
static char epRecv_buf[1<<20];
static int startSend_flag[5] = {0};

static int gSocketfd = -1;
static struct sockaddr_in  gSocketAddr;
static unsigned long long  gRecv_udps = 0;

int socket_init(unsigned short recvport)
{
        int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if ( -1 == s )
                return -1;

        int opt = 1;
        if ( -1 == setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) )
        {
                close(s);
                return -1;
        }

        struct sockaddr_in  addr;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(recvport);
        if ( -1 == bind(s, (struct sockaddr *) &addr, (socklen_t)sizeof(addr)) )
        {
                close(s);
                return -1;
        }
        gSocketfd = s;
        memcpy(&gSocketAddr, &addr, sizeof(struct sockaddr_in));
        return  0;
}

int socket_deInit(void)
{
        close(gSocketfd);
        return 0;
}

void signal_handle(int iSignNo)
{
    switch(iSignNo){
    case SIGINT:
        //        done = 1;
        fprintf(stdout,"SIGINT (Ctrl + c) capture..\n");
        break;
    case SIGQUIT:
        /* fprintf(stdout,"SIGQUIT (Ctrl + \\)Capture....\n"); */
        printf("###Running :Recieve Total FrameNum is %llu ####.\n", gRecv_udps);
            gTest_params.test_done = 1;
            break;
    default:
        break;
    }
}
static pthread_mutex_t  gSend_mutex = PTHREAD_MUTEX_INITIALIZER;
static int OSA_pcieSendDataMutex(Int32 to_id, char *buf_ptr, Int32 buf_siz, Int32 buf_channel, Int32 sync_mode, struct timeval *tv)
{
        int ret = 0;
        pthread_mutex_lock(&gSend_mutex);
        ret =  OSA_pcieSendData(to_id, buf_ptr, buf_siz,  buf_channel,  sync_mode, tv);
        pthread_mutex_unlock(&gSend_mutex);
        return ret;
}
int rc_send_data(struct test_params *params)
{
        int ret = 0;
        int to_id = 0;
        to_id = params->to_id;
        
        switch(to_id){
        case 0:
        {
                for(to_id = EP_ID_0; to_id < EP_ID_0 + 3; to_id ++){
                        if(startSend_flag[to_id] == 0){
                                startSend_flag[to_id] = 1;
                                params->buf[0] = START_SEND_DATA;
                                ret = OSA_pcieSendDataMutex(to_id, params->buf, 64, 0, params->sync_mode, NULL);
                                if(ret <0){
                                        printf("%s: send startFlag to Pcie-%d failed#####\n", __func__, params->to_id);
                                        return -1;
                                }
                        }
                
                        ret = OSA_pcieSendDataMutex(to_id, params->buf, params->size_per_send, 0, params->sync_mode, NULL);
                        if(ret <0){
                                printf("%s: sendData to Pcie-%d failed#####\n", __func__, params->to_id);
                                return -1;
                        }
                }
        }
        break;
        case 4:
        {
                if(startSend_flag[to_id] == 0){
                        startSend_flag[to_id] = 1;
                        params->buf[0] = START_SEND_DATA;
                        ret = OSA_pcieSendDataMutex(EP_ID_ALL, params->buf, 64, 0, params->sync_mode, NULL);
                        if(ret <0){
                                printf("%s: send startFlag to Pcie-%d failed#####\n", __func__, params->to_id);
                                return -1;
                        }
                }

                ret = OSA_pcieSendDataMutex(EP_ID_ALL, params->buf, params->size_per_send, 0, params->sync_mode, NULL);
                if(ret <0){
                        printf("%s: sendData to Pcie-%d failed#####\n", __func__, params->to_id);
                        return -1;
                }
        }
        break;
        default:                
        {
                if(startSend_flag[to_id] == 0){
                        startSend_flag[to_id] = 1;
                        params->buf[0] = START_SEND_DATA;
                        ret = OSA_pcieSendDataMutex(to_id, params->buf, 64, 0, params->sync_mode, NULL);
                        if(ret <0){
                                printf("%s: send startFlag to Pcie-%d failed#####\n", __func__, params->to_id);
                                return -1;
                        }
                }

                ret = OSA_pcieSendDataMutex(to_id, params->buf, params->size_per_send, 0, params->sync_mode, NULL);
                if(ret <0){
                        printf("%s: sendData to Pcie-%d failed#####\n", __func__, params->to_id);
                        return -1;
                }

        }
        }
        return 0;
}

int ep_recv_loop(struct test_params *params)
{
        int ret = 0;
        int print_int = params->print_int;
        char *local_data_buf = epRecv_buf;
        int channel_id = 0;
        Int32 *tmp_ptr = NULL;
        int i, test_count, test_forever, gTest_exit;
        UInt32 cur_time,prev_time,total_time;
        unsigned long long total_data_size = 0;
        int recv_size = 0;

        test_count = 0;
        test_forever = 1;
        gTest_exit = 0;
        
        do {
                cur_time = 0;
                total_time = 0;
                prev_time = 0;
                total_data_size = 0;
                i = 0;                
                *local_data_buf = 0;
                while(1){
                        ret = OSA_pcieRecvData(local_data_buf, RECV_MAX_SIZE, &channel_id,0);
                        if(ret < 0){
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
                        return -1;
                }

                while(!gTest_exit){
                        ret = OSA_pcieRecvData(local_data_buf, RECV_MAX_SIZE, &channel_id,0);
                        if(ret < 0){
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

                        i ++;
                }//end of while(1)
        
                ret = pcie_slave_getCurTime(&cur_time);
                if(ret < 0){
                        printf("get Cur time Error, exit.\n");
                        return -1;
                }
                total_time =cur_time - prev_time;

                test_count ++;

                unsigned int data_size_tmp;
                data_size_tmp =(UInt32)( total_data_size /(1<<20));
                printf("THTP TEST [%u]:\n", test_count);
                printf("\tReceive data loop %u,total size %u MBit in %u jiffies.\n",
                       i,data_size_tmp ,total_time);

                if(total_time > 0)
                        printf("\tTHPT calculated in RX is: %f MBPS\n ",
                               (float)((UInt32)data_size_tmp) * HZ/total_time);

        }while(!params->test_done );

        return 0;
}

static int gEnPcie_send = 1;
int rc_send_loop(struct test_params *params)
{
        int ret = 0;
        int size = 0;
        int i = 0;
        unsigned long long total_size_send = 0;
        char *p = params->buf;
        unsigned int loops = 0;

        struct timeval tv;
        unsigned long long prev_time, cur_time, time_delay = 0;
        
        socklen_t socklen = sizeof(gSocketAddr);
        memset(startSend_flag, 0, sizeof(startSend_flag));
        if(params->num_udps <= 0){
                printf("%s: Invalid num_udps %d.\n", __func__, params->num_udps);
                return -1;
        }

        printf("%s: rc net_recv_port-%d,  to_id-%d, num_udps-%d,  sync_mode-%d, enPcie_send-%d ####\n",
               __func__, params->recv_port, params->to_id, params->num_udps, params->sync_mode, gEnPcie_send);
        
        /* p = params->buf; */
        /* while(1){ */
        /*         size = recvfrom(gSocketfd, p, 2048, 0, (struct sockaddr *)&gSocketAddr, (socklen_t *)&socklen); */
        /*         if(size <= 0) */
        /*                 continue; */

        /*         if(*(int *)p == 0xf5f5f5f5){ */
        /*                 break; */
        /*         } */
        /* } */

        gettimeofday(&tv, 0);
        prev_time = tv.tv_sec * 1000*1000 + tv.tv_usec;

        while(!params->test_done){
                p = params->buf;
                params->size_per_send = 0;
                for(i = 0; i < params->num_udps; i++){
                        size = recvfrom(gSocketfd, p, 2048, 0, (struct sockaddr *)&gSocketAddr, (socklen_t *)&socklen);
                        if(size <= 0)
                                continue;

                        if(*(int *)p == 0xffffffff){
                                params->test_done = 1;
                                break;
                        }
                        
                        p += size;
                        params->size_per_send += size;
                        gRecv_udps ++;
                }

                if(gEnPcie_send){
                        ret = rc_send_data(params);
                        if(ret < 0){
                                printf("%s: rc send data Error.\n", __func__);
                                //                               return -1;
                        }
                }

                if(loops%params->print_int == 0)
                        printf("%s: RC send data %u-%d.\n", __func__, loops, params->size_per_send);
                loops ++;
                total_size_send += params->size_per_send;
        }
        gettimeofday(&tv, 0);
        cur_time = tv.tv_sec * 1000*1000 + tv.tv_usec;
        time_delay = cur_time - prev_time;
        
//send stop flag
        memset(params->buf, 0, 2048);
        params->size_per_send = 0;
        ret = rc_send_data(params);
        if(ret < 0){
                printf("%s: rc send data Error.\n", __func__);
                return -1;
        }

        printf("%s: Total received UDP packeges-%llu, send data size-%llu MB [loops-%u] in  %llu mS.\n",
               __func__, gRecv_udps, total_size_send>>20, loops, time_delay/1000);
        printf("\t Rc received UDP packeges-%llu, speed is %f Mbit/s.\n", gRecv_udps, (float)(total_size_send>>20) * 8000000/(time_delay) );
        return 0;
}

sem_t  sem_recv0, sem_recv1, sem_recv2;
sem_t sem_send0, sem_send1,sem_send2;
pthread_t  gSend_pthreadId[3];

static int  init_sem(void)
{
        sem_init(&sem_recv0, 0, 0);
        sem_init(&sem_recv1, 0, 0);
        sem_init(&sem_recv2, 0, 0);

        sem_init(&sem_send0, 0, 1);
        sem_init(&sem_send1, 0, 1);
        sem_init(&sem_send2, 0, 1);
        return 0;
}

void pthread0(void *arg)
{
        int ret = 0;
        struct test_params params;

        memcpy(&params, &gTest_params, sizeof(gTest_params));

        params.to_id = 1;
        while(1)        {
                sem_wait(&sem_recv0);
                params.size_per_send = gTest_params.size_per_send;
                ret = rc_send_data(&params);
                if(ret < 0){
                        printf("%s: rc send data Error.\n", __func__);
                }
                sem_post(&sem_send0);
        }
}
void pthread1(void *arg)
{
        int ret = 0;
        struct test_params params;

        memcpy(&params, &gTest_params, sizeof(gTest_params));

        params.to_id = 2;
        while(1)        {
                sem_wait(&sem_recv1);
                params.size_per_send = gTest_params.size_per_send;
                ret = rc_send_data(&params);
                if(ret < 0){
                        printf("%s: rc send data Error.\n", __func__);
                }
                sem_post(&sem_send1);
        }
}

void pthread2(void *arg)
{
        int ret = 0;
        struct test_params params;

        memcpy(&params, &gTest_params, sizeof(gTest_params));

        params.to_id = 3;
        while(1)        {
                sem_wait(&sem_recv2);
                params.size_per_send = gTest_params.size_per_send;
                ret = rc_send_data(&params);
                if(ret < 0){
                        printf("%s: rc send data Error.\n", __func__);
                }
                sem_post(&sem_send2);
        }
}


int rc_send_loop_multiPthread(struct test_params *params)
{
        int ret = 0;
        int size = 0;
        int i = 0;
        unsigned long long total_size_send = 0;
        char *p = params->buf;
        unsigned int loops = 0;
        unsigned long long  udp_count = 0;
        /* pthread_t  pthread_id0, pthread_id1, pthread_id2; */
                
        struct timeval tv;
        unsigned long long prev_time, cur_time, time_delay = 0;
        
        socklen_t socklen = sizeof(gSocketAddr);
        memset(startSend_flag, 0, sizeof(startSend_flag));

        init_sem();
        
        if(params->num_udps <= 0){
                printf("%s: Invalid num_udps %d.\n", __func__, params->num_udps);
                return -1;
        }

        if(pthread_create(&gSend_pthreadId[0], 0, (void *)pthread0, 0) != 0){
                printf("%s: pthread0 create failed.\n", __func__);
                return -1;
        }
        if(pthread_create(&gSend_pthreadId[1], 0, (void *)pthread1, 0) != 0){
                printf("%s: pthread1 create failed.\n", __func__);
                return -1;
        }
        if(pthread_create(&gSend_pthreadId[2], 0, (void *)pthread2, 0) != 0){
                printf("%s: pthread2 create failed.\n", __func__);
                return -1;
        }
        
        printf("%s: rc net_recv_port-%d,  to_id-%d, num_udps-%d,  sync_mode-%d, enPcie_send-%d ####\n",
               __func__, params->recv_port, params->to_id, params->num_udps, params->sync_mode, gEnPcie_send);

        p = params->buf;
        
        /* while(1){ */
        /*         size = recvfrom(gSocketfd, p, 2048, 0, (struct sockaddr *)&gSocketAddr, (socklen_t *)&socklen); */
        /*         if(size <= 0) */
        /*                 continue; */

        /*         if(*(int *)p == 0xf5f5f5f5){ */
        /*                 break; */
        /*         } */
        /* } */
        
        gettimeofday(&tv, 0);
        prev_time = tv.tv_sec * 1000*1000 + tv.tv_usec;

        while(!params->test_done){

                sem_wait(&sem_send0);
                sem_wait(&sem_send1);
                sem_wait(&sem_send2);
                        
                p = params->buf;
                params->size_per_send = 0;
                for(i = 0; i < params->num_udps; i++){
                        size = recvfrom(gSocketfd, p, 2048, 0, (struct sockaddr *)&gSocketAddr, (socklen_t *)&socklen);

                        if(size <= 0)
                                continue;

                        if(*(int *)p == 0xffffffff){
                                params->test_done = 1;
                                break;
                        }

                        p += size;
                        params->size_per_send += size;
                        gRecv_udps ++;
                }

                sem_post(&sem_recv0);
                sem_post(&sem_recv1);
                sem_post(&sem_recv2);

                if(loops%params->print_int == 0)
                        printf("%s: RC send data %u-%d.\n", __func__, loops, params->size_per_send);
                loops ++;
                total_size_send += params->size_per_send;
        }
        gettimeofday(&tv, 0);
        cur_time = tv.tv_sec * 1000*1000 + tv.tv_usec;
        time_delay = cur_time - prev_time;
        
//send stop flag
        memset(params->buf, 0, 2048);
        params->size_per_send = 0;
        params->to_id = 4;
        ret = rc_send_data(params);
        if(ret < 0){
                printf("%s: rc send data Error.\n", __func__);
                return -1;
        }

        printf("%s: Total received UDP packeges-%llu, send data size-%llu MB [loops-%u] in  %llu mS.\n",
               __func__, gRecv_udps, total_size_send>>20, loops, time_delay/1000);
        if(time_delay)
                printf("\t Rc received UDP packeges-%llu, speed is %f Mbit/s.\n", gRecv_udps, (float)(total_size_send>>20) * 8000000/(time_delay) );

        pthread_cancel(gSend_pthreadId[0]);
        pthread_cancel(gSend_pthreadId[1]);
        pthread_cancel(gSend_pthreadId[2]);

        return 0;
}

int usage(char *arg)
{
        printf("Usage: %s-%s\n", __TIME__,__DATE__);
        printf("\t  -t: pcie to Ep id [0,1,2,3,4], 0: to all ep[p2p],4: to ep_all default [0]\n");
        printf("\t  -m: pcie send sync mode; 0-nosync, 1-sync, default [0]\n");
        printf("\t  -s:  per_size_pcie, unit:udps, defualt [20]\n");
        printf("\t  -p:  net receive Portid, default [20000].\n");
        printf("\t  -e: enbale/disable pcie send, 0/1, default [1]\n");
        printf("\t  -i: print info interval, default [256]\n");
        printf("\t  -M: Pcie send data in multi-thread mode, default in single-thread mode\n");
        printf("\t  -h: help info.\n");
        return 0;
}

int main(int argc, char **argv)
{
        int ret = 0;
        char *optstring = "t:m:s:p:e:i:hM";
        int opt;
        char exe_name[32];
        int enMulti_thread = 0;

       signal(SIGQUIT,signal_handle);//ctrl+\ quit
 
       memset(&gTest_params, 0, sizeof(gTest_params));
        memset(&epRecv_buf, 0, sizeof(epRecv_buf));
        gTest_params.num_udps = 20;
        gTest_params.recv_port = 20000;
        gTest_params.to_id = 0;
        gTest_params.sync_mode = 0;
        gTest_params.print_int = 256;

        strcpy(exe_name, argv[0]);
        
        while(1){
                opt = getopt(argc, argv, optstring);
                if(opt < 0)
                        break;
                switch(opt){
                case 't':
                        gTest_params.to_id = atoi(optarg);
                        break;
                case 'm':
                        gTest_params.sync_mode = atoi(optarg);
                        break;
                case 's':
                        gTest_params.num_udps = atoi(optarg);
                        break;
                case 'p':
                        gTest_params.recv_port = atoi(optarg);
                        break;
                case 'e':
                        gEnPcie_send = atoi(optarg);
                        break;
                case 'i':
                        gTest_params.print_int = atoi(optarg);
                        break;
                case 'M':
                        enMulti_thread = 1;
                        break;
                case 'h':
                default:
                        usage(exe_name);
                        return 0;
                }
        }

        pthread_mutex_init(&gSend_mutex, NULL);

        ret = OSA_pcieInit(NULL);
        if(ret < 0){
                printf("%s: PcieInit failed####\n", __func__);
                return -1;
        }

        if(gLocal_id != RC_ID){
                ret = ep_recv_loop(&gTest_params);
                if(ret <0){
                        printf("%s: ep_recv_loop Error.\n", __func__);
                        ret = -1;
                        goto err_exit;
                }
        }else {
                ret = socket_init(gTest_params.recv_port);
                if(ret < 0){
                        printf("socket init faled####\n");
                        goto err_exit;
                }
                
                if(enMulti_thread == 0) {
                        ret = rc_send_loop(&gTest_params);
                        if(ret <0){
                                printf("%s: rc_send_loop Error.\n", __func__);
                                ret = -1;
                                goto err_exit;
                        }
                }else {
                        ret = rc_send_loop_multiPthread(&gTest_params);
                        if(ret <0){
                                printf("%s: rc_send_loop_multiPthread Error.\n", __func__);
                                ret = -1;
                                goto err_exit;
                        }
                }
                socket_deInit();
        }

        printf("Test exit.\n");

err_exit:        
        OSA_pcieDeInit();
        return ret;        
}
