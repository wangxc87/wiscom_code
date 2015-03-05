/*
 * This application is written to test the protocol developed for communication
 * between RC--multiple EP scenario. This application use RC side additional
 * module.
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
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>
#include <poll.h>
#include <string.h>

#include "ti81xx_mgmt_lib.h"
#include "ti81xx_pci_info.h"
#include "ti81xx_trans.h"
#include <drivers/char/ti81xx_pcie_rcdrv.h>


#define PAGE_SIZE_EP	4096
#define USED		1
#define INPROCESS	1
#define CPU				2

/* for test case summary */
struct test_case {
	char test_case_id[10];
	char result[10];
};

struct test_case test_summary[19];


/****** globals*********/

char			*mapped_buffer;
char			*mapped_pci;
struct ti81xx_mgmt_area	mgmt_area;
struct ti81xx_ptrs	ptr;
int			fd;
struct pci_sys_info	*start;
struct pollfd		fds[1];
unsigned int		int_cap;
int total_fld;
unsigned int *id_alloc;
int test_integrity;
unsigned int  bar0_addr;

#ifdef INTEGRITY
FILE *fp1, *fp2;
#endif

#if defined(INTEGRITY) || defined(THPT)

unsigned long long byte_recv;
struct fix_trans {
	struct dedicated_buf bufd;
	char *user_buf;
	unsigned int size_buf;
};

#endif

void send_data_by_cpu()
{
	int ret;
	unsigned int offset;

 ACCESS_MGMT:
	while ((ret = access_mgmt_area((u32 *) mapped_pci,
                                   mgmt_area.my_unique_id)) < 0) {
		printf("mgmt area access not granted\n");
		sleep(3);
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
	printf("send by cpu success full\n");
	release_mgmt_area((u32 *)mapped_pci);
}



void process_local_rmt_bufs()
{
	int ret;
	/*unsigned int offset;*/
	while ((ret = access_mgmt_area((u32 *)mapped_pci,
                                   mgmt_area.my_unique_id)) < 0) {
		printf("access to mgmt area not granted\n");
		sleep(4);
		continue;
	}
	process_remote_buffer_for_data((u32 *)mapped_pci, CPU, 0);
	release_mgmt_area((u32 *)mapped_pci);

	while ((ret = access_mgmt_area((u32 *)mapped_buffer,
                                   mgmt_area.my_unique_id)) < 0) {
		printf("access to mgmt_area not granted\n");
		sleep(4);
		continue;
	}

	ti81xx_poll_for_data(&ptr, &mgmt_area, mapped_buffer, NULL, NULL);
	release_mgmt_area((u32 *)mapped_buffer);

}


void push_data()
{
	int ret;
	unsigned int offset;

 ACCESS_MGMT:
	while ((ret = access_mgmt_area((u32 *)mapped_buffer,
                                   mgmt_area.my_unique_id)) < 0) {
		printf("access to mgmt_area not granted\n");
		sleep(2);
		continue;
	}
	ret = get_free_buffer((u32 *)mapped_buffer);
	if (ret < 0) {
		printf("buffer not available\n");
		release_mgmt_area((u32 *)mapped_buffer);
		sleep(2);
		goto ACCESS_MGMT;
	}
	offset = offset_to_buffer((u32 *)mapped_buffer, ret);
	put_data_in_local_buffer((u32 *)mapped_buffer, offset, ret);
	release_mgmt_area((u32 *)mapped_buffer);

}

#ifdef INTEGRITY
void *send_to_dedicated_buf(void *arg)
{
	struct fix_trans *tx = (struct fix_trans *) arg;
	int count;
	unsigned int chunk_trans = (1024 * 1024) / (tx->size_buf);
	printf("offset of buffer is 0x%X, total iteration to send data "
           "will be %u\n", (tx->bufd).off_st, chunk_trans);

	for (count = 0; count < chunk_trans ; count++) {
		memset(tx->user_buf, 0, tx->size_buf);
		set_data_to_buffer(tx->user_buf, tx->size_buf, fp1);
		while (*((tx->bufd).wr_ptr) != 0) { /*wr_idx to zero again.*/
			printf("polling for write index--[%u] to be "
                   "zero again\n", *((tx->bufd).wr_ptr));
			sleep(1);
		}
		memcpy(mapped_pci + (tx->bufd).off_st,
               tx->user_buf, tx->size_buf);
		*((tx->bufd).wr_ptr) = tx->size_buf;
		if (int_cap == 1) /* send interrupt if have int capability */
			ioctl(fd, TI81XX_RC_SEND_MSI, bar0_addr);
		printf("data sent %d times, bytes sent in "
               "this chunk are %d\n",
               count + 1, tx->size_buf);
	}
	printf("exiting from send to dedicated buf after "
           "transmitting 1MB data\n");
	pthread_exit(NULL);
}
#endif


#ifdef THPT

void *thpt_buf_read_data(void *arg)
{
	struct fix_trans *tx = (struct fix_trans *) arg;
	int count = 0;
    //	unsigned int chunk_trans = 5368709120llu / tx->size_buf;
    unsigned int chunk_trans = THPT_DATA_SIZE / tx->size_buf;
	/* 5 GB devided by buffer size */
	printf("offset of buffer is 0x%X total iteration of pushing data "
           "in buffer will be %u\n",
           (tx->bufd).off_st, chunk_trans);
    *((tx->bufd).wr_ptr) = tx->size_buf;
	while (1) {
		while (*((tx->bufd).wr_ptr) != 0)
			/*poll*/usleep(10);
		*((tx->bufd).wr_ptr) = tx->size_buf;
        //        memset(tx->user_buf,0x61,tx->size_buf);
		ioctl(fd, TI81XX_RC_SEND_MSI, bar0_addr);
        //		if (++count == chunk_trans) test
        //			break; test
	}
	printf("this buffer has been read by rmt peer %d times\n", count);
	pthread_exit(NULL);
}

#endif


int main(int argc, char **argv)
{
	int  i;
	int eps = 0;
#ifdef THPT
	unsigned int size_buf = 1024 * 1024;
#else
	unsigned int size_buf = 4 * 1024;
#endif
	struct ti81xx_start_addr_area start_addr;
	unsigned int *test;
	pthread_t t1, t2, t3;
	struct pci_sys_info *temp;
	void *push_data_in_local(void *);
	void *send_data(void *);
	void *wait_for_data(void *arg);
	void *wait_for_int(void *arg);

	pthread_attr_t attr_t1, attr_t2, attr_t3;
	pthread_attr_init(&attr_t1);
	pthread_attr_init(&attr_t2);
	pthread_attr_init(&attr_t3);

#ifdef INTEGRITY
	struct fix_trans wr_buf;
#endif

#ifdef THPT
	struct fix_trans rd_buf;
#endif

	fds[0].events = POLLIN;
	start = NULL;
	bar0_addr = 0;
#if defined(INTEGRITY) || defined(THPT)
	byte_recv = 0;
#endif

#ifdef INTEGRITY
	fp1 = fopen("rc_tx.cap", "w+");
	if (fp1 == NULL) {
		err_print("file to be transfered open failed\n");
		return -1;
	}

	fp2 = fopen("rc_rx.cap", "w+");
	if (fp2 == NULL) {
		err_print("file to recv data open failed\n");
		return -1;
	}

	create_random_pattern((1024 * 1024), 4, fp1);
	for (i = 0; i <= 18; i++) {
		sprintf(test_summary[i].test_case_id, "%s%03d", "DM81XX", i+1);
		sprintf(test_summary[i].result, "%s", "NE/C");
	}

#endif
	fd = open("/dev/ti81xx_ep_hlpr", O_RDWR);
	if (fd == -1) {
		err_print("device file open fail\n");
#ifdef INTEGRITY
		fclose(fp1);
		fclose(fp2);
#endif
		return -1;
	}


	fds[0].fd = fd;

	if (ioctl(fd, TI81XX_RC_START_ADDR_AREA, &start_addr) < 0) {
		err_print("ioctl START_ADDR failed\n");
		goto ERROR;
	}
	eps = get_devices(&start);
	if (eps < 0) {
		err_print("fetching pci sub system info on rc fails\n");
		goto ERROR;
	}

	printf("no of ep in system is %u\n", eps);

	printf("start address of mgmt_area is virt--%x  phy--%x\n",
           start_addr.start_addr_virt,
           start_addr.start_addr_phy);
	propagate_system_info(start, fd, eps, start_addr.start_addr_phy);

	printf("pci subsystem info propagated\n");
	print_list(start);

	/* assumed that remote peer id is 2,
     * may be dynamicaly entered in a multi EP setup
     */
	for (temp = start; temp != NULL; temp = temp->next) {
		if (temp->res_value[0][0] == 2) {
			printf("bar 2 address of EP2 is %x size is %x\n",
                   temp->res_value[3][0],
                   temp->res_value[3][1]);
			bar0_addr = temp->res_value[1][0];
			printf("bar 0 address is %x\n", bar0_addr);
			break;
		}
	}

	if (temp == NULL) {
		printf("NO ep in this setup have muid 2\n");
		goto ERROR;
	}

	/* 4 MB size kmalloc buffer in kernel for management area and buffers
     * working on both NETRA and AMD
     */
    //映射到内核中分配的地址
	mapped_buffer = mmap(0, 4 * 1024 * 1024,
                         PROT_READ | PROT_WRITE, MAP_SHARED,
                         fd, (off_t)start_addr.start_addr_phy);

	if ((void *)-1 == (void *) mapped_buffer) {
		err_print("MMAP of dedicated memory fail\n");
		goto FREELIST;
	}
	id_alloc = (unsigned int *) mapped_buffer;

    //映射到EP的bar地址
	mapped_pci = mmap(0, temp->res_value[3][1],
                      PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd, (off_t) temp->res_value[3][0]);

	if ((void *)-1 == (void *) mapped_pci) {
		err_print("MMAP EP's BAR  memory fail\n");
		goto FREELIST;
	}

	test = (unsigned int *)mapped_pci;

	if (ti81xx_prepare_mgmt_info(&mgmt_area, size_buf) < 0) {
		err_print("prepare_mgmt_info failed\n");
		goto FREELIST;
	}

    //将设置的mgmt_area的参数按序复制到mapped_buffer中
	ti81xx_set_mgmt_area(&mgmt_area, (unsigned int *)mapped_buffer);

	id_alloc[0] = 0;
	id_alloc[1] = 1; /* rc unique id 1 always */

#if defined(INTEGRITY) || defined(THPT)
	/* dedicate buffer 0 to remote ep having my uniue id
     * field 2  for writing and buffer 1 for reading
     */
	dedicate_buffer((unsigned int *)mapped_buffer,
                    2, mgmt_area.no_blk, 0, RD);

#ifdef THPT
	dedicate_buffer((unsigned int *)mapped_buffer,
                    2, mgmt_area.no_blk, 1, WR);
#endif

#endif

	printf("initialization of management mgmt_area complete\n");
	ti81xx_calculate_ptr(&mgmt_area, mapped_buffer, &ptr);
	mgmt_area.my_unique_id = 1;

#if 1
	printf("notify EP about interrupt capability 1--int 2--polling\n");
	scanf("%u", &int_cap);
#else
	printf("***Forcing polling (interrupt communication disabled, "
           "RC will not generate interrupt)\n");
	int_cap = 2;
#endif

	test[5] = int_cap;
	/* As for now app is demonstrating interrupt notification from peer*/
	printf("management area before "
           "int cap recevied from remote peer\n\n\n");

	total_fld = 6 + mgmt_area.no_blk * 2 + mgmt_area.no_blk * 5;

#if 0
	printf("Management area dump:\n");
	for (i = 0; i < total_fld ; i++) {
		printf("0x%X ", id_alloc[i]);
		if ((i + 1) % 6 == 0)
			printf("\n");
	}
#endif
	printf("waiting for interrupt capability from remote end\n");
	while (id_alloc[5] == 0) {
		printf("waiting for remote end point "
               "to interrupt capability\n");
#if 0
		for (i = 0; i < total_fld; i++) {
			printf("0x%X ", id_alloc[i]);
			if ((i + 1) % 6 == 0)
				printf("\n");
		}
#endif

		printf("int capability recevied from "
               "rmt peer is 0x%X\n", id_alloc[5]);
		sleep(1);
	}
	printf("int capability recevied from "
           "rmt peer is 0x%X\n", id_alloc[5]);


#if defined(INTEGRITY) || defined(THPT)
	/* here demonstrating THPT and INTEGRITY only receving interrupt
     * mechanism polling will have same effect except it have to poll
     * rather then wait for interrupt.
     */
	if (id_alloc[5] != 1) {
		err_print("Exiting the application since EP doesn't have "
                  "interrupt capability and THROUGHPUT/INTEGRITY "
                  "tets require it.\n");
		goto FREELIST;
	}

#ifdef INTEGRITY
	wr_buf.size_buf = size_buf;
	printf("size of buffer on RC is %u\n", wr_buf.size_buf);
	wr_buf.user_buf = malloc(size_buf);
	if (wr_buf.user_buf == NULL) {
		printf("user buffer allocation failed\n");
		goto FREELIST;
	}
	if (find_dedicated_buffer((unsigned int *)mapped_pci,
                              mgmt_area.my_unique_id, &wr_buf.bufd, RD) < 0) {
		err_print("no dedicated buffer for TX on remote peer\n");
		goto FREELIST;
	}

	printf("offset of buffer is %x\n", wr_buf.bufd.off_st);
#endif

#ifdef THPT
	rd_buf.size_buf = size_buf;
	rd_buf.user_buf = NULL;

	if (find_dedicated_buffer((unsigned int *)mapped_buffer,
                              2, &rd_buf.bufd, WR) < 0) {
		err_print("no dedicated buffer on local peer to be RX "
                  "by remote peer\n");
		goto FREELIST;
	}

	/* populate source buffer to be read by remote peer */
	memset(mapped_buffer + (rd_buf.bufd).off_st, 97, rd_buf.size_buf);
#endif

	if (id_alloc[5] == INT) {
#ifdef INTEGRITY
		/* SEND TO DEDICATED BUF AND RECEIVE NOTOFICATION FROM REMOTE
         * PEER ABOUT DATA RECEPTION
         */
		pthread_create(&t1, &attr_t1, send_to_dedicated_buf, &wr_buf);
		pthread_create(&t2, &attr_t2, wait_for_int, NULL);
#endif

#ifdef THPT
		printf("\nexecuting test case TX from EP\n");
		pthread_create(&t2, &attr_t2, wait_for_int, NULL);
        //		pthread_join(t2, NULL);
        //		sleep(4);
		printf("\nexecuting test case RX/TX from EP\n");
#ifdef WISCOM_THPT_EPREAD_SYNS
		pthread_create(&t3, &attr_t3, thpt_buf_read_data , &rd_buf);
#else
        pthread_create(&t2, &attr_t2, wait_for_int, NULL);
#endif
#endif
		goto JOIN;
	} else
		goto FREELIST;
#endif

	if (id_alloc[5] == INT) {
		printf("interrupt will be working on this end\n");
		pthread_create(&t1, &attr_t1, send_data, NULL);
		pthread_create(&t2, &attr_t2, wait_for_int, NULL);
		pthread_create(&t3, &attr_t3, push_data_in_local, NULL);
	} else {
		printf("polling will be working on this end\n");
		pthread_create(&t1, &attr_t1, send_data, NULL);
		pthread_create(&t2, &attr_t2, wait_for_data, NULL);
		pthread_create(&t3, &attr_t3, push_data_in_local, NULL);
	}

	/*never executed in demo mode */
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);
	/*currently all thread are running a while loop */
#if defined(INTEGRITY) || defined(THPT)
 JOIN:
#endif
	pthread_join(t2, NULL);

#ifdef INTEGRITY
	pthread_join(t1, NULL);
#endif
	/*
      #ifdef THPT
      pthread_join(t3, NULL);
      #endif*/

 FREELIST:
	free_list(start);

 ERROR:
#ifdef INTEGRITY
	while (id_alloc[0] != 2) {
        printf("waiting for turn\n");
        sleep(1);
    }

	sleep(3);
	/* send 1000 interrupt to EP for stress testing */
	for (i = 0; i < 1000; i++) {
        ioctl(fd, TI81XX_RC_SEND_MSI, bar0_addr/*bar 0 of ep2 */);
        /*printf("interrupt send %u times\n",i+1);*/
    }
	close(fd);
	fclose(fp1);
	fclose(fp2);
	if (test_integrity == 0) {
        sprintf(test_summary[1].result, "%s", "Pass");
        sprintf(test_summary[5].result, "%s", "Pass");
        sprintf(test_summary[7].result, "%s", "Pass");
        sprintf(test_summary[9].result, "%s", "Pass");
        sprintf(test_summary[12].result, "%s", "Pass");
        sprintf(test_summary[15].result, "%s", "Pass");
    }

	printf("test summary:\nNE/C : means not executed or "
           "confirmed at this end:\n other test cases "
           "will be covered by demo mode\n");
	for (i = 0; i <= 18; i++)
		printf("%s\t %s\n", test_summary[i].test_case_id,
               test_summary[i].result);

#endif

	return 0;
}


void *wait_for_data(void *arg)
{
	int try = 0;
	/* while (1) {  wiscom*/
    /*     sleep(1); */
    /*     process_local_rmt_bufs(); */
    /*     try++; */
    /*     printf("data for reading is available " */
    /*            "called %d times\n", try); */
    /*     /\*sleep(5);*\/ */
    /* } */
	pthread_exit(NULL);
}

void *send_data(void *arg)
{
	int try = 0;
    int i = 0;
	while (1) {
        sleep(2);
        //        send_data_by_cpu();by wiscom
        if (int_cap == 1){
            ioctl(fd, TI81XX_RC_SEND_MSI, bar0_addr/*bar 0 of ep2 */);printf("***%s: rc send msi times-%d***", __func__, i++);            
        }
            
        printf("INFO: send data %d times\n", try);
        try++;
    }
	sleep(5);
	printf("INFO: sending exits\n\n\n\n\n");
	pthread_exit(NULL);
}


void *push_data_in_local(void *arg)
{
	int try = 0;
    int i =0 ;
	while (1) {
        sleep(3);
        push_data();
        if (int_cap == 1){
            ioctl(fd, TI81XX_RC_SEND_MSI, bar0_addr/*bar 0 of ep2*/);
            printf("****%s: send msi, times-%d**\n",__func__, i++);
        }
            
        /* send MSI */
        printf("%s:INFO: send data %d times\n", __func__, try);
        try++;
    }
	pthread_exit(NULL);
}


void *wait_for_int(void * arg)
{
	int ret;
    int i = 0;
#if defined(INTEGRITY) || defined(THPT)
	byte_recv = 0;
#endif
	while (1) {
        usleep(20);
		ret = poll(fds, 1, 3000); /*3 sec wait time out*/
		if (ret == POLLIN) {
            printf("****%s: recv interrupt-%d****\n", __func__, i++); //wiscom
#ifdef INTEGRITY
			ti81xx_poll_for_data(&ptr, &mgmt_area,
                                 mapped_buffer, fp2, &byte_recv);
#endif
#ifdef THPT
			ti81xx_poll_for_data(&ptr, &mgmt_area,
                                 mapped_buffer, NULL, &byte_recv);
#endif
#ifdef DISPLAY
			ti81xx_poll_for_data(&ptr, &mgmt_area,
                                 mapped_buffer, NULL, NULL);
#endif
			/*process_remote_buffer_for_data(mapped_pci, CPU, 0);*/

#ifdef INTEGRITY
			if (byte_recv == (1024 * 1024))
				break;
#endif

#ifdef THPT
			if (byte_recv == THPT_DATA_SIZE/*0x140000000llu*/)
				break;
#endif
		} else
			printf("%s:no notification from remote peer -- "
                   "timed out byte_recv\n", __func__);
	}
#if defined(INTEGRITY) || defined(THPT)
	printf("byte recv from EP is %llu\n", byte_recv);
#endif
#ifdef INTEGRITY
	fseek(fp2, 0 , SEEK_SET);
	test_integrity = check_integrity(fp1, fp2);
#endif
	pthread_exit(NULL);
}

void *wait_for_int_test(void *arg)
{
	int ret;
	unsigned int wake_up = 0;
	unsigned int intr_cntr = 0;
	unsigned int i = 0;
	sleep(5);
	while (1) {
		ret = poll(fds, 1, 1); /*1 mili sec wait time out*/
		if (ret == POLLIN) {
			wake_up++;
            printf("****%s: recv interrupt times-%u***\n",__func__, wake_up);
		} /* else { */
		/* 	ioctl(fd, TI81XX_GET_INTR_CNTR, &intr_cntr); */
		/* 	if (++i >= 10) { /\* 60 % of sent interrupt *\/ */
		/* 		printf("interrupt occur %u times " */
		/* 				"application wakeup %u times\n", */
		/* 					intr_cntr, wake_up); */
		/* 		sprintf(test_summary[19].result, */
		/* 				"stress_int %u wakeup %u", */
		/* 					intr_cntr, wake_up); */
		/* 		pthread_exit(NULL); */
		/* 	} */
		/* } */
	}
}

