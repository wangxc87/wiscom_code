/*
 * Application to Test EP driver and it's functionality along with EDMA.
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include "ti81xx_ep_lib.h"
#include "ti81xx_mgmt_lib.h"
#include "ti81xx_trans.h"
#include "debug_msg.h"
#define HZ	100


/* for test case summary */
struct test_case {
	char test_case_id[15];
	char result[50];
};

struct test_case test_summary[20];




#if defined(INTEGRITY) || defined(THPT)

unsigned long long byte_recv; /* send by remote peer */
unsigned long long byte_rx; /* read from remote peer */

/* information for dedicated buffer */
struct fix_trans {
	struct dedicated_buf bufd;
	unsigned int out_phy;
	char *user_buf;
	unsigned int size_buf;
};

#endif

#ifdef INTEGRITY
/* file used for data integrity testing */
FILE                    *fp1, *fp2;
#endif


/*
 * global variable ( each bit of this 32 bit integer represents
 * status of corresponding outbound region.
 * if bit is set -  free
 * else -- in use
 */

unsigned int		status;

/* other globals*/
char			*mapped_buffer;
char			*mapped_pci;
char			*mapped_dma, *mapped_dma_recv;
unsigned int		*rm_info;
struct ti81xx_mgmt_area  mgmt_area;
struct pollfd		fds[1];
int			fd_dma;
int			fd;
struct ti81xx_ptrs	ptr;
pthread_mutex_t mutex_int = PTHREAD_MUTEX_INITIALIZER;
int			counter;
sem_t			mutex;
unsigned int		intr_cap; /* interrupt capability of this peer */
unsigned int fin_time1, cur_time1, fin_time2, cur_time2;
struct dma_cnt_conf	dma_cnt;
struct dma_buf_info	dma_b;
int integrity_test;


void send_data_by_cpu()
{
	int ret;
	unsigned int offset;

ACCESS_MGMT:
	while ((ret = access_mgmt_area((u32 *)mapped_pci,
					mgmt_area.my_unique_id)) < 0) {
		printf("mgmt_area access not granted\n");
		sleep(2);
		continue;
	}

	while ((ret = get_free_buffer((u32 *)mapped_pci)) < 0) {
		printf("buffer not available\n");
		release_mgmt_area((u32 *)mapped_pci);
		sleep(2);
		goto ACCESS_MGMT;
	}

	offset = offset_to_buffer((u32 *)mapped_pci, ret);

	send_to_remote_buf_by_cpu((u32 *)mapped_pci, offset, ret);
	printf("send by cpu successful\n");
	release_mgmt_area((u32 *)mapped_pci);
}


void send_data_by_dma()
{
	int ret;
	unsigned int offset;

ACCESS_MGMT:
	while ((ret = access_mgmt_area((u32 *)mapped_pci,
					mgmt_area.my_unique_id)) < 0) {
		printf("mgmt area access not granted\n");
		sleep(3);
		continue;
	}

	while ((ret = get_free_buffer((u32 *)mapped_pci)) < 0) {
		printf("buffer not available\n");
		release_mgmt_area((u32 *)mapped_pci);
		sleep(3);
		goto ACCESS_MGMT;
	}

	offset = offset_to_buffer((u32 *)mapped_pci, ret);
	send_to_remote_buf_by_dma((u32 *)mapped_pci, 0x20000000,
							offset, ret, fd_dma);
	printf("send by dma successful\n");
	release_mgmt_area((u32 *)mapped_pci);
}

void process_local_rmt_bufs()
{
	int ret;
	while ((ret = access_mgmt_area((u32 *)mapped_pci,
					mgmt_area.my_unique_id)) < 0) {
		printf("mgmt area access not granted\n");
		sleep(4);
		continue;
	}

	process_remote_buffer_for_data((u32 *)mapped_pci, EDMA, fd_dma);
	release_mgmt_area((u32 *)mapped_pci);

	while ((ret = access_mgmt_area((u32 *)mapped_buffer,
					mgmt_area.my_unique_id)) < 0) {
		printf("mgmt area access not granted\n");
		sleep(4);
		continue;
	}

	ti81xx_poll_for_data(&ptr, &mgmt_area, mapped_buffer, NULL, NULL);
	release_mgmt_area((u32 *)mapped_buffer);

}


/**
 * scan_mgmt_area(): search local management area for the information(start
 * address of dedicated memory)
 * of remote peer having unique id represented by muid.
 * And fill up outbound details in structure accordingly.
 * @rm_info: pointer on locla management area for remote peer system
 * @muid: uniqued id of remote peer
 * @ob: pointer to outbound region staructure
 */

int scan_mgmt_area(unsigned int *rm_info, unsigned int muid,
						struct ti81xx_outb_region *ob)
{
	unsigned int *ep_info = rm_info + 2;
	int j = 0, i = 0;
	int bar_map;
	if (muid == 1) {
		ob->ob_offset_hi = 0;
		/*start address of management area on RC*/
		ob->ob_offset_idx = rm_info[1];
		return 0;
	}
	for (i = 1; i <= rm_info[0] ; i++) {

		if (ep_info[j] == muid) {
			bar_map = ep_info[j + 13];
			ob->ob_offset_hi = 0;
			ob->ob_offset_idx = ep_info[j + bar_map * 2 + 1];
			return 0;
		}
		j += 14;
	}

	return -1;
}


#if defined(INTEGRITY) || defined(THPT)
/* send data to a dedicated buffer on remote peer*/

void *send_to_dedicated_buf(void *arg)
{

	struct fix_trans *tx = (struct fix_trans *) arg;
	struct dma_info info;
	int count;
    unsigned int i = 0;

#ifdef INTEGRITY
	int choice = EDMA;
#endif
	unsigned int chunk_trans;
	int ret;
	info.size = tx->size_buf;
	info.user_buf = (unsigned char *)tx->user_buf;  //要发送数据区
	info.src = 0;
	info.dest = (unsigned int *)(tx->out_phy + (tx->bufd).off_st);//要发的目的地址，outboud
	/* take size_buf to be multiple of page size */
	chunk_trans = (1024 * 1024) / (tx->size_buf);

#ifdef THPT
    unsigned int time_tmp =0;
    fin_time1 = 0;
	/* 5BG devided by buffer size */
	chunk_trans = THPT_DATA_SIZE/ tx->size_buf;//5368709120llu / tx->size_buf;
	printf("total iteration to send data will be %u\n", chunk_trans);
	memset(mapped_dma, 78, tx->size_buf);

#endif

    //	ioctl(fd, TI81XX_CUR_TIME, &cur_time1);

	for (count = 0; count < chunk_trans; count++) {

#ifdef THPT
		while (*((tx->bufd).wr_ptr) != 0){
            //			printf("waiting for wr index to be zero\n");
            usleep(1);
            i ++;
        }
#if 0
		if (count == chunk_trans-1)
			memset(tx->user_buf, 78, tx->size_buf);
     

		ret = ioctl(fd_dma, TI81XX_EDMA_WRITE, &info);
		if (ret < 0) {
			err_print("edma ioctl failed, error in dma data "
								"transfer\n");
			pthread_exit(NULL);
		}
#endif

		if (count == chunk_trans-1)
			memset(mapped_dma, 78, tx->size_buf);

        ioctl(fd, TI81XX_CUR_TIME, &cur_time1);
#define THPT_DMA_COPY 1
#if THPT_DMA_COPY
		ret = ioctl(fd_dma, TI81XX_EDMA_WRITEM, &info);
		if (ret < 0) {
			err_print("edma ioctl failed, error in dma data "
								"transfer\n");
			pthread_exit(NULL);
		}
		*((tx->bufd).wr_ptr) = ret;
#else
        memcpy(mapped_pci + (tx->bufd).off_st,
						mapped_dma, tx->size_buf);
        *((tx->bufd).wr_ptr) = tx->size_buf;

#endif

		ioctl(fd, TI81XX_SEND_MSI, 0);
        ioctl(fd, TI81XX_CUR_TIME, &time_tmp);
        fin_time1 += time_tmp - cur_time1;
#endif
	}
    //	ioctl(fd, TI81XX_CUR_TIME, &fin_time1);
#ifdef THPT
	printf("tx test case executed with tx %llu bytes in  %u[%u - %u] jiffies,wait delay i %u\n",
           THPT_DATA_SIZE/*5368709120llu*/,fin_time1/*( fin_time1 - cur_time1)*/,fin_time1,cur_time1,i);
	printf("THPT calculated in TX direction is : %f MBPS\n\n",
           (float)(((THPT_DATA_SIZE >> 20) * HZ) / fin_time1/*(fin_time1 - cur_time1)*/));
           //			(float)((5.0 * 1024 * HZ) / (fin_time1 - cur_time1)));
#endif
	/* HZ=100 */
	pthread_exit(NULL);
}

/* read data from a dedicated buffer on remote peer */
void *read_from_dedicated_buf(void *arg)
{
	struct fix_trans *rx = (struct fix_trans *)arg;
	struct dma_info info;
	int count;
	unsigned int chunk_trans;
	int ret;
	info.size = rx->size_buf;
	info.user_buf = (unsigned char *)rx->user_buf;
	info.dest = 0;
	info.src = (unsigned int *)(rx->out_phy + (rx->bufd).off_st);
	/* take size_buf to be multiple of page size */
	chunk_trans = (1024 * 1024) / (rx->size_buf);

#ifdef THPT
    static int i,j;
    unsigned int *p;
    unsigned int time_tmp =0;
    fin_time2 = 0;
	chunk_trans = THPT_DATA_SIZE/*5368709120llu*/ / rx->size_buf; /* 5GB devided by buf size*/
	printf("total iteration to read data will be %u\n", chunk_trans);
	byte_rx = 0;
#endif

    //	ioctl(fd, TI81XX_CUR_TIME, &cur_time2);
	for (count = 0; count < chunk_trans; count++) {
#ifdef THPT
#ifdef WISCOM_THPT_EPREAD_SYNS
		while (*((rx->bufd).wr_ptr) == 0);
            //            printf("read wait,i %u...\n",i);//等待对方设置发送的数据大小
#endif
		if (count == chunk_trans-1) {
			memset(rx->user_buf, 0, rx->size_buf);
			printf("last read\n");
		}
        //		ret = ioctl(fd_dma, TI81XX_EDMA_READM, &info);//mmap 空间
		ioctl(fd, TI81XX_CUR_TIME, &cur_time2);
#if THPT_DMA_COPY        
        ret = ioctl(fd_dma, TI81XX_EDMA_READM, &info);
		if (ret < 0) {
			err_print("edma ioctl failed, error in dma "
							"data transfer\n");
			pthread_exit(NULL);
		}
		byte_rx += ret;
#else
        memcpy(mapped_dma_recv, mapped_pci + (rx->bufd).off_st,
            rx->size_buf);
		byte_rx += rx->size_buf;

#endif
        ioctl(fd, TI81XX_CUR_TIME, &time_tmp);
        fin_time2 += time_tmp - cur_time2;
		/*ioctl(fd, TI81XX_SEND_MSI, 0);*/
#ifdef WISCOM_THPT_EPREAD_SYNS
		  *((rx->bufd).wr_ptr) = 0;
#endif
#ifdef CONFIG_WISCOM_COMPARE
          j = (rx->size_buf>>2);
          p = (unsigned int *)mapped_dma_recv;//rx->user_buf;
          for(i = 0; i < j; i ++){
              if(*p != 0x61616161){
                  printf("****EP Read Error 0x%x, Stopp!***.\n",*p);
                  printf("****Press any Key to Continue***\n");
                  getchar();
              }else
                  *p = 0;
              p++;
          }
#endif
		if (byte_rx == THPT_DATA_SIZE/*0x140000000llu*/) {
			break;
		}

#endif
	}

    //	ioctl(fd, TI81XX_CUR_TIME, &fin_time2);
    printf("read from dedicated buffer completed\n");
	printf("RX test case executed with receving %llu bytes in "
           "%u jiffies\n", byte_rx, fin_time2/*-cur_time2*/);
#ifdef THPT
#if 0
	dump_data_in_file_n(rx->user_buf, ret, "recv.txt");
#endif

    dump_data_in_file_n(mapped_dma_recv, ret, "recv.txt");
	printf("THPT calculated in RX direction is : %f MBPS\n\n",
           (float)(((byte_rx >> 20) * HZ) / (fin_time2/* - cur_time2*/)));
           //           (float)(((THPT_DATA_SIZE >> 20) * HZ) / (fin_time2 - cur_time2)));
    //			(float)((5.0 * 1024 * HZ) / (fin_time2 - cur_time2)));
	/*sprintf(test_summary[17].result,"rx thpt is %f MBPS",
	* (5.0 * 1024 * HZ) / (fin_time2 - cur_time2));
	*/
	/* HZ=100*/
#endif
	pthread_exit(NULL);
}

int read_from_dedicated_buf_func(void *arg)
{
	struct fix_trans *rx = (struct fix_trans *)arg;
	struct dma_info info;
	int ret;
	info.size = rx->size_buf;
	info.user_buf = (unsigned char *)rx->user_buf;
	info.dest = 0;
	info.src = (unsigned int *)(rx->out_phy + (rx->bufd).off_st);

	if (*((rx->bufd).wr_ptr) != 0) {
		ret = ioctl(fd_dma, TI81XX_EDMA_READ, &info);
		if (ret < 0) {
			err_print("edma ioctl failed, error in dma "
							"data transfer\n");
			return -1;
		}
		*((rx->bufd).wr_ptr) = 0;
		/*ioctl(fd, TI81XX_SEND_MSI, 0);*/
		byte_rx += ret;
	}
	return 0;
}



#endif

// #define TEST_MEMCPY_SPEED
#ifdef  TEST_MEMCPY_SPEED
 void *test_memcpy_speed_func(void *arg);
void *test_memcpy_speed_func(void *arg)
 {
     unsigned int buf_size1 = 1<< 20;
     int i,j;
     unsigned int cur_time,pre_time,total_time;
     char *buf_src = malloc(buf_size1);
     char *buf_dest = malloc(buf_size1);
     memset(buf_src, 0x43, buf_size1);

     total_time = 0;
     unsigned long long int loop = 1024llu;

     ioctl(fd, TI81XX_CUR_TIME, &pre_time);

     for(i = 0; i < loop; i ++){
         memcpy(buf_dest, buf_src, buf_size1);
     }

     ioctl(fd, TI81XX_CUR_TIME, &cur_time);

     total_time = cur_time - pre_time;

     printf("Total memcpy data %llu in %u[%u - %u]  jiffies.\n",loop*buf_size1,total_time,cur_time,pre_time);

     printf("memcpy test speed is %f Mbps.\n",(float)(loop*HZ/total_time));

     pthread_exit(NULL);
 }
#endif

int outbound_regn_mgmt_test()
{
	struct ti81xx_outb_region ob, ob1;
	unsigned int ob_size = 0;
	int i;
	ob.ob_offset_hi = 0;
	ob.ob_offset_idx = 0;

	printf("executing outbound test for clear mapping and "
							"request success\n");

	fd = open("/dev/ti81xx_pcie_ep", O_RDWR);
	if (fd == -1) {
		err_print("EP device file open fail\n");
		return -1;
	}

	printf("setting outbound region size 1 MB\n");
	ioctl(fd, TI81XX_SET_OUTBOUND_SIZE, ob_size);
	status = 0;
	if (ioctl(fd, TI81XX_GET_OUTBOUND_STATUS, &status) < 0) {
		err_print("ioctl to retrieve outbound region status failed\n");
		return -1;
	}

	printf("status of outbound regions before  request "
							"is OX%X\n", status);
	printf("request for a outbound mapping of 8 MB\n");

	ob.size = 0x800000; /* request  for outbound mapping of 8MB */

	if (ti81xx_set_outbound_region(&ob, fd) == 0) {
		printf("outbound mapping request successfull\n");
		ioctl(fd, TI81XX_GET_OUTBOUND_STATUS, &status);
		printf("status of outbound regions after request "
							"is OX%X\n", status);
		if (status == 0x7fffff00)
			sprintf(test_summary[2].result, "%s", "Pass");
		printf("clearing mapping of outbound region\n");
		ioctl(fd, TI81XX_CLR_OUTBOUND_MAP, &ob);
		ioctl(fd, TI81XX_GET_OUTBOUND_STATUS, &status);
		if (status == 0x7fffffff)
			sprintf(test_summary[11].result, "%s", "Pass");
		printf("status of outbound regions after "
				"clearing request is OX%X\n", status);

	}

	printf("executing  test case for EINOB (increase "
					"outbound region size)\n");

	ioctl(fd, TI81XX_GET_OUTBOUND_STATUS, &status);
	printf("status of outbound regions before request "
						"is OX%X\n", status);
	printf("sending a request for 28 MB request\n");
	ob.ob_offset_hi = 1;
	ob.ob_offset_idx = 1;
	ob.size = 0x1C00000;/* request 28 MB */
	if (ti81xx_set_outbound_region(&ob, fd) == 0) {
		printf("outbound mapping request successfull\n");
		ioctl(fd, TI81XX_GET_OUTBOUND_STATUS, &status);
		printf("status of outbound regions after 28 MB "
					"request is OX%X\n", status);
	}

	ob1.ob_offset_hi = 2;
	ob1.ob_offset_idx = 2;
	ob1.size = 0x600000;/* request 6 MB */
	printf("sending a request for 6 MB request\n");
	if (ti81xx_set_outbound_region(&ob1, fd) == EINOB) {
		printf("outbound mapping request return with EINOB "
							"error code\n");
		printf("outbound mapping test case of "
						"EINOB is passed\n");
		ioctl(fd, TI81XX_CLR_OUTBOUND_MAP, &ob);
		sprintf(test_summary[4].result, "%s", "Pass");
	}

	printf("executing test case of ENMEM\n");
	ioctl(fd, TI81XX_GET_OUTBOUND_STATUS, &status);
	printf("status of outbound regions before is OX%X\n", status);
	printf("sending 28 request for 1 MB each\n");

	for (i = 0; i <= 27; i++) {
		ob.ob_offset_hi = i;
		ob.ob_offset_idx = i;
		ob.size = 0x100000;
		if (ti81xx_set_outbound_region(&ob, fd) != 0)
			printf("test case execution failed\n");
	}


	printf("sending a request of 32 MB\n");
	ob1.ob_offset_hi = 1024;
	ob1.ob_offset_idx = 1024;
	ob1.size = 0x2000000;
	if (ti81xx_set_outbound_region(&ob1, fd) == ENMEM) {
		printf("ENMEM passed\n");
		sprintf(test_summary[3].result, "%s", "Pass");
	}
	close(fd);
	return 0;
}

void *wait_for_int_test(void *arg);

int main(int argc, char **argv)
{
	/*by default my_unique id has been set by driver and uniqueid is
	* set to lock management area at startup.
	*/

	int ret;
	unsigned int ob_size;
	struct ti81xx_pcie_mem_info start_addr;
	struct ti81xx_inb_window in;
	struct ti81xx_outb_region ob;
	unsigned int *test;
	unsigned int *int_cap;
	unsigned int muid;
#ifdef THPT
	unsigned int size_buf = 1024 * 1024;
#else
	unsigned int size_buf = 4 * 1024;
#endif

#if defined(INTEGRITY) || defined(THPT)
	dma_cnt.acnt = 256;
	dma_cnt.bcnt = 4096;
	dma_cnt.ccnt = 1;
	dma_cnt.mode = 1;
#else
	dma_cnt.acnt = 510;
	dma_cnt.bcnt = 1;
	dma_cnt.ccnt = 1 ;
	dma_cnt.mode = 0;
#endif

	struct ti81xx_pciess_regs pcie_regs;

#if defined(INTEGRITY) || defined(THPT)

	struct fix_trans rd_buf, wr_buf;
	void *send_to_dedicated_buf(void *arg);
	void *read_frm_dedicate_buf(void *arg);
	void *cpu_utilize(void *arg);

#endif

	pthread_t t1, t2, t3, t4;

	void *wait_for_int(void *);
	void *send_data(void *);
	void *wait_for_data(void *arg);
	void *process_rmt_buffer(void *arg);
	void *process_local_buffer(void *arg);

	pthread_attr_t attr_t1, attr_t2, attr_t3, attr_t4;

	pthread_attr_init(&attr_t1);
	pthread_attr_init(&attr_t2);
	pthread_attr_init(&attr_t3);
	pthread_attr_init(&attr_t4);

	fds[0].events = POLLIN;
#if defined(INTEGRITY) || defined(THPT)
	byte_recv = 0;
	byte_rx = 0;
#endif
	status = 0;
	counter = 0;

	if (sem_init(&mutex, 0, 1) < 0) {
		perror("semaphore initilization failed");
		exit(0);
	}

	fd = open("/dev/ti81xx_pcie_ep", O_RDWR);
	if (fd == -1) {
		err_print("EP device file open fail\n");
		return -1;
	}

	/*local management area lock has been hold by EP itself.*/

    //读取ep 本地内存保留的内存信息，地址及大小
	ret = ioctl(fd, TI81XX_GET_PCIE_MEM_INFO, &start_addr);

	if (ret < 0) {
		err_print("START_MGMT_AREA ioctl failed\n");
		close(fd);
		return -1;
	}

	if (!start_addr.size || (start_addr.size < SIZE_AREA)) {
		if (!start_addr.size)
			err_print("No reserved memory available for PCIe "
					"transfers, quitting...\n");
		else
			err_print("Minimum %#x bytes required as reserved "
					"memory, quitting...\n", SIZE_AREA);

		close(fd);
		return -1;
	}

	dma_b.size = 0x100000;
	dma_b.send_buf = (unsigned int)start_addr.base + 0x6000000;
	dma_b.recv_buf = (unsigned int)dma_b.send_buf + dma_b.size;

	mapped_buffer = (char *)mmap(0, SIZE_AREA, PROT_READ|PROT_WRITE,
							MAP_SHARED, fd ,
					(off_t) start_addr.base);
	if ((void *)-1 == (void *) mapped_buffer) {
		err_print("mapping dedicated memory fail\n");
		close(fd);
		return -1;

	}

	test = (unsigned int *)mapped_buffer;

	pcie_regs.offset = GPR0;
	pcie_regs.mode = GET_REGS;
	if (ioctl(fd, TI81XX_ACCESS_REGS, &pcie_regs) < 0) {
		err_print("GET_REGS mode ioctl failed\n unable to fetch "
						"unique id from GPR0\n");
		close(fd);
		return -1;
	}

	if (pcie_regs.value < 2) {
		err_print("Not a valid id assigned to it\n still continue by "
					"manually assigning valid id -- 2\n");
	}
	/******* by default uid 2 is assigned to This EP ****/

	test[0] = 2;
	test[1] = 2;
	fd_dma = open("/dev/ti81xx_edma_ep", O_RDWR);
	if (fd_dma == -1) {
		err_print("EP DMA device file open fail\n");
		close(fd);
		close(fd);
		return -1;
	}

	ret = ioctl(fd_dma, TI81XX_EDMA_SET_CNT, &dma_cnt);
	if (ret < 0) {
		printf("dma count setting ioctl failed\n");
		goto ERROR;
	}
	ret = ioctl(fd_dma, TI81XX_EDMA_SET_BUF_INFO, &dma_b);
	if (ret < 0) {
		printf("dma buffer setting ioctl failed\n");
		goto ERROR;
	}

	fds[0].fd = fd;

	printf("INFO: start address of mgmt_area is virt--%x  phy--%x\n",
			start_addr.base, start_addr.base);


	/*inbound setup to be done for inbound to be enabled. by default BAR 2*/

	pcie_regs.offset = LOCAL_CONFIG_OFFSET + BAR2;
	pcie_regs.mode = GET_REGS;
	if (ioctl(fd, TI81XX_ACCESS_REGS, &pcie_regs) < 0) {
		err_print("GET_REGS mode ioctl failed\n");
		goto ERROR;
	}

	in.BAR_num = 2;
	/*by default BAR2 will be used to get
	* inbound access by RC.
	*/
    //指定的内存区域为inboud区域
	in.internal_addr = start_addr.base;
	in.ib_start_hi = 0;
	in.ib_start_lo = pcie_regs.value;

	if (ti81xx_set_inbound(&in, fd) < 0) {
		err_print("setting in bound config failed\n");
		goto ERROR;
	}

	if (ti81xx_enable_in_translation(fd) < 0) {
		err_print("enable in bound failed\n");
		goto ERROR;
	}

	/*inbound setup complete.*/

    //设置outboud
	mapped_pci = (char *)mmap(0, PCIE_NON_PREFETCH_SIZE,
				PROT_READ|PROT_WRITE, MAP_SHARED,
						fd, (off_t)0x20000000);
	if ((void *)-1 == (void *) mapped_pci) {
		err_print("mapping PCIE_NON_PREFETCH memory fail\n");
		goto ERROR;
	}


	mapped_dma = (char *)mmap(0, dma_b.size,
				PROT_READ|PROT_WRITE, MAP_SHARED,
					fd, (off_t)dma_b.send_buf);
	if ((void *)-1 == (void *) mapped_dma) {
		err_print("mapping DMA tx memory fail\n");
		goto ERROR;
	}

	mapped_dma_recv = (char *)mmap(0, dma_b.size, PROT_READ|PROT_WRITE,
					MAP_SHARED, fd, (off_t)dma_b.recv_buf);
	if ((void *)-1 == (void *) mapped_dma) {
		err_print("mapping DMA rx memory fail\n");
		goto ERROR;
	}

	int_cap = (unsigned int *)mapped_pci;

	if (ti81xx_prepare_mgmt_info(&mgmt_area, size_buf) < 0) {
		err_print("prepare_mgmt_info failed\n");
		goto ERROR;
	}

	rm_info = (u32 *)(mapped_buffer + mgmt_area.size);

	ti81xx_set_mgmt_area(&mgmt_area, (unsigned int *)mapped_buffer);
	printf("initialization of management mgmt_area complete\n");


	test[0] = 1; /*set lock to be accessed by RC*/
	mgmt_area.my_unique_id = test[1];

	while ((ret = access_mgmt_area((u32 *)mapped_buffer,
						test[1])) < 0) {
		/*trying to get access to it's own management area.*/
		printf("mgmt area access not granted\n");
		sleep(1);
		continue;
	}


	printf("total no of ep on system is %u\n", rm_info[0]);
	printf("Mgmt area's start address on RC is %u\n", rm_info[1]);



	ti81xx_calculate_ptr(&mgmt_area, mapped_buffer, &ptr);
	ob_size = 0; /* by default assuming 1 MB outbound window */

	ret = ioctl(fd, TI81XX_SET_OUTBOUND_SIZE, &ob_size);

	muid = 1;
	/* RC unique id -- assuming communcation have to be
	* done with remote peer having muid=1
	*/

	if ((scan_mgmt_area(rm_info, muid, &ob) == -1)) {
		/*this function will fillup outbound structure accordingly.*/
		printf("no remote peer of this muid exist\n");
		/*handle error free resource or continue without outbound.*/
	}

	printf("outbound config is %x %x\n",
					ob.ob_offset_hi, ob.ob_offset_idx);
	ob.size = 4194304;/* 4 MB*/




	ti81xx_set_outbound_region(&ob, fd); /*outbound will be setup here.*/


	if (ti81xx_enable_out_translation(fd) < 0) {
		err_print("enable outbound failed\n");
		goto ERROR;
	}

    //使能EP读写请求的功能
	if (ti81xx_enable_bus_master(fd) < 0) {
		err_print("enable bus master fail\n");
		goto ERROR;
	}

	mgmt_area.size = test[4]; /* updated by rc application */

	release_mgmt_area((u32 *)mapped_buffer); /*release management area */


	while (test[5] == 0) {
		printf("INFO: waiting for interrupt capability "
					"info from remote peer\n");
		sleep(1);
		continue;
	}

	printf("***Advertised interrupt capability, EP will generate MSI.\n");
	intr_cap = 1;

	int_cap[5] = intr_cap;


#if defined(INTEGRITY) || defined(THPT)
	/*  ////////
	    if (test[5] != 1) {
		printf("Exiting from application\n");
		goto ERROR;
	    }*/
	/* user buffer allocation */
	rd_buf.size_buf = size_buf;
	rd_buf.out_phy = 0x20000000;
	wr_buf.size_buf = size_buf;
	wr_buf.out_phy = 0x20000000;

	rd_buf.user_buf = malloc(size_buf);
	if (rd_buf.user_buf == NULL) {
		printf("user buffer allocation failed\n");
		goto ERROR;
	}

	wr_buf.user_buf =
        malloc(size_buf);
	if (wr_buf.user_buf == NULL) {
		printf("user buffer allocation failed\n");
		free(rd_buf.user_buf);
		goto ERROR;
	}

	memset(rd_buf.user_buf, 66, size_buf);
	memset(wr_buf.user_buf, 66, size_buf);

#ifdef THPT
	/* find dedicated buffer for reading */
    //申请一个写buf空间
	if (find_dedicated_buffer((unsigned int *)mapped_pci,
				mgmt_area.my_unique_id, &rd_buf.bufd, WR) < 0) {
		err_print("no dedicated buffer for RX on remote peer\n");
		goto FREE_BUFFER;
	}
	printf("offset of buffer dedicated for RX on "
				"remote peer is %X\n", rd_buf.bufd.off_st);

#endif
	/* find dedicated buffer for transmitting data */
    //申请一个读buf空间
	if (find_dedicated_buffer((unsigned int *)mapped_pci,
				mgmt_area.my_unique_id, &wr_buf.bufd, RD) < 0) {
		err_print("no dedicated buffer for TX on remote peer\n");
		goto FREE_BUFFER;
	}

	printf("offset of buffer dedicated for TX on remote "
					"peer is  %X\n", wr_buf.bufd.off_st);


#ifdef THPT
#ifdef TEST_MEMCPY_SPEED
    printf("\nExecuting test case DDR Memcpy speed.\n");
    pthread_create(&t2,&attr_t2,test_memcpy_speed_func, NULL);
    pthread_join(t2,NULL);
    sleep(2);
#endif
    unsigned long long i = 0;
    while(1){
        printf("\n****Test Loop %llu***",i++);
	printf("\nExecuting test case THPT TX (write to RC from EP)\n");
	/*pthread_create(&t2, &attr_t2, cpu_utilize, NULL);*/
	pthread_create(&t1, &attr_t1, send_to_dedicated_buf, &wr_buf);
	pthread_join(t1, NULL);
	sleep(1);

	printf("\nExecuting test case THPT RX (RC read from EP)\n");
	pthread_create(&t4, &attr_t4, read_from_dedicated_buf, &rd_buf);
	pthread_join(t4, NULL);
	sleep(1);
    }
    /* wiscom
	printf("\nExecuting test case THPT TX/RX "
			"(simultaneous read & write from EP)\n");
	pthread_create(&t4, &attr_t4, read_from_dedicated_buf, &rd_buf);
	pthread_create(&t1, &attr_t1, send_to_dedicated_buf, &wr_buf);
	pthread_join(t1, NULL);
	pthread_join(t4, NULL);
    */
#endif

	goto CLEAR_MAP;

#endif

	if (test[5] == INT) {
		printf("INTERRUPT will be working\n");
		sleep(5);
		pthread_create(&t1, &attr_t1, send_data, &fd);
		pthread_create(&t2, &attr_t2, wait_for_int, NULL);
        pthread_create(&t3, &attr_t3, wait_for_int_test, &fd);
		/* pthread_create(&t3, &attr_t3, process_local_buffer, NULL); */
		/* pthread_create(&t4, &attr_t4, process_rmt_buffer, &fd); */
		/*send interrupt to indicate rmt buffer to recycle this buffer*/
	} else {
		printf("polling will be working\n");
		pthread_create(&t1, &attr_t1, send_data, &fd);
		pthread_create(&t2, &attr_t2, wait_for_data, NULL);
        pthread_create(&t3, &attr_t3, wait_for_int_test, &fd);
	}

	/* thread running infinite loops, in Demo mode they never joined*/

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);
	/* never executed  in demo mode*/
#if defined(INTEGRITY) || defined(THPT)
CLEAR_MAP:
#endif

#ifdef THPT
	pthread_join(t1, NULL);
	pthread_join(t4, NULL);
	printf("bytes recv from  dedicated buffer are %llu\n", byte_rx);
#endif
#if defined(THPT) || defined(INTEGRITY)
FREE_BUFFER:
	free(wr_buf.user_buf);
	free(rd_buf.user_buf);
#endif
#ifdef THPT

	printf("Total thpt calculated tx+rx is:  %f MBPS\n",
           (float)(((THPT_DATA_SIZE>>20) * HZ) / (fin_time2 - cur_time2))
           + (float)(((THPT_DATA_SIZE>>20) * HZ) / (fin_time1 - cur_time1)));
#endif

ERROR:
	close(fd);
	close(fd_dma);

	return 0;
}





void *wait_for_int(void *arg)
{
	int ret;
	while (1) {
		ret = poll(fds, 1, 3000); /*3 sec wait time out*/

		if (ret == POLLIN) {
			sem_wait(&mutex);
			counter = 2;
			sem_post(&mutex);


		} else
			printf("%s: no data in buffers -- timed out\n", __func__);

#ifdef THPT
		if (byte_rx == THPT_DATA_SIZE/*0x140000000llu*/)
			break;
#endif

	}

	pthread_exit(NULL);
}



void *wait_for_int_test(void *arg)
{
	int ret;
	unsigned int wake_up = 0;
	unsigned int intr_cntr = 0;
	unsigned int i = 0;
	sleep(2);
	while (1) {
		ret = poll(fds, 1, 1000); /*1 mili sec wait time out*/
		if (ret == POLLIN) {
			wake_up++;
		} else {
			ioctl(fd, TI81XX_GET_INTR_CNTR, &intr_cntr);
			if (++i >= 10) { /* 60 % of sent interrupt */
				printf("interrupt occur %u times "
						"application wakeup %u times\n",
							intr_cntr, wake_up);
				sprintf(test_summary[19].result,
						"stress_int %u wakeup %u",
							intr_cntr, wake_up);
                //				pthread_exit(NULL); wiscom
			}
		}
        //        usleep()
	}
}



void *process_local_buffer(void *arg)
{
	int ret = 0;
	while (1) {
		sem_wait(&mutex);
		if (counter > 0) {

			while ((ret = access_mgmt_area((u32 *)mapped_buffer,
						mgmt_area.my_unique_id)) < 0) {
				printf("management area access "
							"not granted\n");
				continue;
			}
#ifdef INTEGRITY
			ti81xx_poll_for_data(&ptr, &mgmt_area,
						mapped_buffer, fp2, &byte_recv);
#elif defined(THPT)
			ti81xx_poll_for_data(&ptr, &mgmt_area,
						mapped_buffer,
							NULL, &byte_recv);
#elif defined(DISPLAY)
			ti81xx_poll_for_data(&ptr, &mgmt_area,
						mapped_buffer,
							NULL, NULL);
#endif
			release_mgmt_area((u32 *)mapped_buffer);
			counter--;
		}
		sem_post(&mutex);
	}
	pthread_exit(NULL);
}


void *process_rmt_buffer(void *arg)
{
	int ret = 0;
	/*
#ifdef THPT
ioctl(fd, TI81XX_CUR_TIME, &cur_time2);
#endif*/
	while (1) {
		sem_wait(&mutex);
		if (counter > 0) {
			/*
#ifdef THPT
read_from_dedicated_buf_func(arg);
#endif*/

			while ((ret = access_mgmt_area((u32 *)mapped_pci,
						mgmt_area.my_unique_id)) < 0) {
				printf("access not granted\n");
				continue;
			}

			process_remote_buffer_for_data((u32 *)mapped_pci,
								EDMA, fd_dma);
			release_mgmt_area((u32 *)mapped_pci);
			if (intr_cap == 1)
				ioctl(*(unsigned int *)arg,
							TI81XX_SEND_MSI, 0);

			counter--;
		}
		sem_post(&mutex);
		/*
		   if (byte_rx == 0x140000000llu)
		   break;*/
	}
	/*
#ifdef THPT
ioctl(fd, TI81XX_CUR_TIME, &fin_time2);
printf("test case RX completed with reading byte_rx=%llu "
			"in %u jiffies\n",byte_rx, fin_time2-cur_time2);
dump_data_in_file_n(((struct fix_trans *)arg)->user_buf ,
		((struct fix_trans *)arg)->size_buf , "recv.txt");
#endif*/
	pthread_exit(NULL);
}


void *wait_for_data(void *arg)
{	int try = 0;
	/* while (1) {  wiscom*/
	/* 	sleep(3); */
	/* 	process_local_rmt_bufs(); */
	/* 	try++; */
	/* 	printf("data for reading is available " */
	/* 				"called %d times\n\n", try); */
	/* } */
	pthread_exit(NULL);
}




void *send_data(void *arg)
{
	int try = 0;
    int i = 0;
	while (1) {

		sleep(3);
        //		send_data_by_cpu(); wiscom
		if (intr_cap == 1){
    		ioctl(*(unsigned int *)arg, TI81XX_SEND_MSI, 0);
            printf("***%s: send msi times-%d***",__func__, i++);
}
	
        //		send_data_by_dma(); wiscom
		if (intr_cap == 1){
			ioctl(*(unsigned int *)arg, TI81XX_SEND_MSI, 0);
            printf("***%s: send msi times-%d***",__func__, i++);
}
		printf("send data %d times\n", try);
		try++;
	}
	printf("sending data exits");
	pthread_exit(NULL);

}

void *cpu_utilize(void *arg)
{
	system("top >> cpuutilize");
	pthread_exit(NULL);
}
