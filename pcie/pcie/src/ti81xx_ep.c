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

#include "pcie_std.h"
#include "ti81xx_ep_lib.h"
#include "debug_msg.h"
#include "pcie_common.h"

#define PCIE_SLAVE_DEVICE "/dev/ti81xx_pcie_ep"
#define TI81XX_EDMA_DEVICE "/dev/ti81xx_edma_ep"



static Int32 gSelf_id = PCIE_INVALID_ID;
static Int32 gPciedev_slave_fd = -1;
static Int32 gPciedev_edma_fd = -1;

static struct pciedev_info gPciedev_slave_info;
static struct ti81xx_pcie_mem_info gPciedev_slave_rsv_mem;//start_addr;

static struct mgmt_info *gPciedev_mgmt_infoPtr;
static char *gMgmt_map_buf = NULL;
static char *gData_map_buf = NULL;
static char *gData_buf = NULL;
static char *gData_edma_recvBuf = NULL;
static char *gData_edma_sendBuf = NULL;

static struct pciedev_databuf_head *gDatabuf_headPtr = NULL;


static Int32 edma_init(void)
{
    Int32 ret = 0;

    gPciedev_edma_fd = open(TI81XX_EDMA_DEVICE, O_RDWR);
    if(gPciedev_edma_fd < 0){
        err_print("edma device %s open Error.\n", TI81XX_EDMA_DEVICE);
        gPciedev_edma_fd = -1;
        return -1;
    }
    return 0;
}
#define RSVMEM_MGMT_SIZE   (0x200000)
#define RSVMEM_EDMA_RECV_BUF_SIZE (0x300000)
#define RSVMEM_MGMT_OFFSET (0)
#define RSVMEM_EDMA_RECV_BUF_OFFSET (RSVMEM_MGMT_OFFSET + RSVMEM_MGMT_SIZE)
#define RSVMEM_EDMA_SEND_BUF_OFFSET (RSVMEM_EDMA_RECV_BUF_OFFSET + RSVMEM_EDMA_RECV_BUF_SIZE)

static Int32 edma_config(struct ti81xx_pcie_mem_info *start_addr)
{
    Int32 ret = 0;
    struct dma_cnt_conf dma_cnt;
    struct dma_buf_info	dma_b;

#if 1
	dma_cnt.acnt = 128;//256;
	dma_cnt.bcnt = 4096*2;
	dma_cnt.ccnt = 1;
	dma_cnt.mode = 1; //1: AB-mode
#else
	dma_cnt.acnt = 510;
	dma_cnt.bcnt = 1;
	dma_cnt.ccnt = 1 ;
	dma_cnt.mode = 0;
#endif
    if(gPciedev_edma_fd < 0){
        err_print("edma device not inited.\n");
        return -1;
    }
    ret = ioctl(gPciedev_edma_fd, TI81XX_EDMA_SET_CNT, &dma_cnt);
	if (ret < 0) {
		err_print("dma count setting ioctl failed\n");
        goto dma_err_exit;
    }

	dma_b.size = 0x100000;
	dma_b.send_buf = (unsigned int)start_addr->base + RSVMEM_EDMA_SEND_BUF_OFFSET;
	dma_b.recv_buf = (unsigned int)start_addr->base + RSVMEM_EDMA_RECV_BUF_OFFSET;

    ret = ioctl(gPciedev_edma_fd, TI81XX_EDMA_SET_BUF_INFO, &dma_b);
	if (ret < 0) {
        err_print("dma buffer setting ioctl failed\n");
        goto dma_err_exit;
	}

    return 0;

 dma_err_exit:
    close(gPciedev_edma_fd);
    gPciedev_edma_fd = -1;
    return -1;
    
}

//buf_size not used
Int32 edma_recvData(UInt32 buf_size, UInt32 offset)
{
    struct dma_info info;
    int ret;

    info.size =buf_size;//1<<20;//3M
    info.user_buf =gData_edma_recvBuf;//NULL;
    info.dest = 0;
    info.src = (unsigned int *)(0x20000000 + offset);

    ret = ioctl(gPciedev_edma_fd, TI81XX_EDMA_READM, &info);
    if (ret < 0) {
        err_print("edma ioctl failed, error in dma data transfer\n");
        return -1;
    }

    return ret;
}
static Int32 edma_deInit()
{
    Int32 ret;
    if(gPciedev_edma_fd < 0)
        return 0;
    close(gPciedev_edma_fd);
    return 0;
}

static Int32 get_selfId(void)
{
    Int32 ret = 0;

    gSelf_id = EP_ID_O;
    
    return ret;
}

Int32 pcie_slave_getInfo(void)
{
    char *mapped_resv_buffer,*mapped_pci;
    Int32 ret;
	struct ti81xx_pciess_regs pcie_regs;
    
    ret = ioctl(gPciedev_slave_fd, TI81XX_GET_PCIE_MEM_INFO, &gPciedev_slave_rsv_mem);
    if (ret < 0) {
		err_print("START_MGMT_AREA ioctl failed\n");
		return -1;
	}

    if (!gPciedev_slave_rsv_mem.size || (gPciedev_slave_rsv_mem.size < SIZE_AREA)) {
		if (!gPciedev_slave_rsv_mem.size)
			err_print("No reserved memory available for PCIe "
                      "transfers, quitting...\n");
		else
			err_print("Minimum %#x bytes required as reserved "
                      "memory, quitting...\n", SIZE_AREA);
		return -1;
	}
	debug_print("INFO: Slave start address of reserv_area is phy--0x%x,size--0x%x\n",
                gPciedev_slave_rsv_mem.base,gPciedev_slave_rsv_mem.size);


    /*inbound setup to be done for inbound to be enabled. by default BAR 2*/

	pcie_regs.offset = LOCAL_CONFIG_OFFSET + BAR2;
	pcie_regs.mode = GET_REGS;
	if (ioctl(gPciedev_slave_fd, TI81XX_ACCESS_REGS, &pcie_regs) < 0) {
		err_print("GET_REGS mode ioctl failed\n");
        return -1;
	}

	struct ti81xx_inb_window in;
	in.BAR_num = 2;
	/*by default BAR2 will be used to get inbound access by RC. */
	in.internal_addr = gPciedev_slave_rsv_mem.base;
	in.ib_start_hi = 0;
	in.ib_start_lo = pcie_regs.value;

	if (ti81xx_set_inbound(&in, gPciedev_slave_fd) < 0) {
		err_print("setting in bound config failed\n");
        return -1;
	}

	if (ti81xx_enable_in_translation(gPciedev_slave_fd) < 0) {
		err_print("enable in bound failed\n");
        return -1;
	}
    mapped_resv_buffer = (char *)mmap(0, SIZE_AREA, PROT_READ|PROT_WRITE,
                                 MAP_SHARED, gPciedev_slave_fd ,
                                 (off_t) gPciedev_slave_rsv_mem.base);
	if ((void *)-1 == (void *) mapped_resv_buffer) {
		err_print("pcie slave mapping dedicated memory fail\n");
		close(gPciedev_slave_fd);
		return -1;
	}

    memset(&gPciedev_slave_info, 0, sizeof(struct pciedev_info));
    gPciedev_slave_info.id = gSelf_id;
    gPciedev_slave_info.mgmt_buf = mapped_resv_buffer;

    UInt32 out_addrbase;
    gPciedev_mgmt_infoPtr = (struct mgmt_info *)mapped_resv_buffer;
    if((gPciedev_mgmt_infoPtr->ep_id == gSelf_id) || (gPciedev_mgmt_infoPtr->ep_id == EP_ID_ALL)){
        out_addrbase = gPciedev_mgmt_infoPtr->ep_outbase;
    }
    debug_print("Recv ep_outbound addr is 0x%x.\n", out_addrbase);

	Int32 ob_size = 0; /* by default assuming 1 MB outbound window */
	ret = ioctl(gPciedev_slave_fd, TI81XX_SET_OUTBOUND_SIZE, &ob_size);
    if(ret < 0){
        err_print("Pcie slave ioctl set_outboud_size Error.\n");
        goto ERROR;
    }

    struct ti81xx_outb_region ob;
    ob.ob_offset_hi = 0;
    ob.ob_offset_idx = out_addrbase;
    ob.size = PCIE_RC_RESERVE_MEM_SIZE;
    ret = ti81xx_set_outbound_region(&ob, gPciedev_slave_fd);
    if(ret < 0){
        err_print("Pcie slave set outboud Error.\n");
        goto ERROR;
    }
   	if (ti81xx_enable_out_translation(gPciedev_slave_fd) < 0) {
		err_print("Pcie slave enable outbound failed\n");
		goto ERROR;
    }

    //使能EP读写请求的功能
	if (ti81xx_enable_bus_master(gPciedev_slave_fd) < 0) {
		err_print("Pcie slave enable bus master fail\n");
		goto ERROR;
	}
    
    //设置outboud
	mapped_pci = (char *)mmap(0, PCIE_NON_PREFETCH_SIZE,
                              PROT_READ|PROT_WRITE, MAP_SHARED,
                              gPciedev_slave_fd, 0x20000000);
	if ((void *)-1 == (void *) mapped_pci) {
		err_print("mapping PCIE_NON_PREFETCH memory fail\n");
		goto ERROR;
	}

    gData_edma_recvBuf = (char *)mmap(0, RSVMEM_EDMA_RECV_BUF_SIZE,PROT_READ|PROT_WRITE,
                                      MAP_SHARED, gPciedev_slave_fd,
                                      (off_t)(gPciedev_slave_rsv_mem.base + RSVMEM_EDMA_RECV_BUF_OFFSET));
    if(gData_edma_recvBuf == NULL){
        err_print("dma_recvBuf mmap phyaddr 0x%x Error.\n", gPciedev_slave_rsv_mem.base + RSVMEM_EDMA_RECV_BUF_OFFSET);
        munmap(mapped_pci, PCIE_NON_PREFETCH_SIZE);
        goto ERROR;
    }

    /*
    gData_edma_sendBuf = (char *)mmap(0, dma_b.size, PROT_READ | PROT_WRITE,
                                      MAP_SHARED, gPciedev_slave_fd, (off_t)dma_b.send_buf);
    if(gData_edma_sendBuf == NULL){
        err_print("dma_sendBuf mmap phyaddr 0x%x Error.\n",dma_b.send_buf);
        munmap(gData_edma_recvBuf, dma_b.size);
        goto dma_err_exit;
    }

    */

    gPciedev_slave_info.data_buf = (char *)mapped_pci;
    gPciedev_mgmt_infoPtr->wr_index --;
    gPciedev_mgmt_infoPtr->rd_index ++;
    gPciedev_mgmt_infoPtr->ep_initted = TRUE;
    
    gMgmt_map_buf = gPciedev_slave_info.mgmt_buf;  //ep DDR
    gData_map_buf = gPciedev_slave_info.data_buf;  //ep outbound
    //    gData_edma_recvBuf = gPciedev_slave_info.mgmt_buf + RSVMEM_EDMA_RECV_BUF_OFFSET;
    gData_edma_sendBuf = gPciedev_slave_info.mgmt_buf + RSVMEM_EDMA_SEND_BUF_OFFSET;

    return 0;

 ERROR:
    munmap(mapped_resv_buffer, SIZE_AREA);
    return -1;
}

Int32 pcie_slave_init(void)
{
    Int32 ret ;
    ret = get_selfId();
    if(ret < 0){
        err_print("Dev get selfid Error.\n");
        return -1;
    }

    gPciedev_slave_fd = open(PCIE_SLAVE_DEVICE, O_RDWR);
    if(gPciedev_slave_fd < 0){
        err_print("Open Pciedev slave %s Error.\n",PCIE_SLAVE_DEVICE);
        return -1;
    }

    ret = pcie_slave_getInfo();
    if(ret < 0){
        err_print("Pcie configure Error.\n");
        goto init_err_exit;
    }
    debug_print("pcie slave get info successfully.\n");

    ret = edma_init();
    if(ret < 0){
        err_print("edma init Error.\n");
        goto init_err_exit;
    }
    debug_print("edma init OK.\n");

    ret = edma_config(&gPciedev_slave_rsv_mem);
    if(ret < 0){
        err_print("edma config Error.\n");
        edma_deInit();
        goto init_err_exit;
    }
    return 0;

 init_err_exit:
        close(gPciedev_slave_fd);
        return -1;
    
}

char  *pcie_slave_reqDatabuf(UInt32 buf_size)
{
    Int32 ret;
    char *ptr_tmp;

    if(buf_size >= PCIE_RC_RESERVE_MEM_SIZE){
        err_print("PCI slave require size too large,Maxsize [0x%x]",PCIE_RC_RESERVE_MEM_SIZE);
        return NULL;
    }

    gDatabuf_headPtr = (struct pciedev_databuf_head *)gData_map_buf;

    gData_buf = (char *)(gData_map_buf + sizeof(struct pciedev_databuf_head ));

#ifdef EDMA_TRANSMISSION
    ptr_tmp = gData_edma_recvBuf;
    debug_print("edma databuf pointer requred.\n");
#else
    ptr_tmp = gData_buf;
#endif

    return ptr_tmp;
}

#define DATA_WAIT_TIMEOUT (3000)

static UInt32 gFrame_count_prev = 0xffffffff;

//timeout:0 表示系统默认等待时间3s，单位ms
Int32 pcie_slave_recvData(char *buf, UInt32 buf_size, UInt32 timeout)
{
    Int32 ret;
    UInt32 i = 0,time_out; 
    struct pciedev_databuf_head databuf_head;

    if(timeout == 0)
        time_out = DATA_WAIT_TIMEOUT*10;
    else
        time_out = time_out*10;
    while(1){

        memcpy_neon((char *)&databuf_head, gDatabuf_headPtr, sizeof(struct pciedev_databuf_head));

        if((databuf_head.data_to_id == gSelf_id) || (databuf_head.data_to_id == EP_ID_ALL)){
            if((gFrame_count_prev != databuf_head.frame_id) && (databuf_head.wr_index > 0))//if has been recieved
                break;
        }
        if(i != time_out)
            i ++;
        else
            break;
        usleep(100);
    }

    if(i == time_out){
        err_print("Pcie slave recieve data Error,Timeout.\n");
        return -1;
    }

#ifdef EDMA_TRANSMISSION
    ret = edma_recvData(databuf_head.buf_size, sizeof(struct pciedev_databuf_head));
    if(ret < 0){
        err_print("edma recv data Error.\n");
        return -1;
    }
    buf = gData_edma_recvBuf;
    ret = databuf_head.buf_size;
#else
    buf = gData_buf;
    ret = databuf_head.buf_size;
#endif

    gFrame_count_prev = databuf_head.frame_id;
    databuf_head.wr_index --;
    memcpy_neon((char *)gDatabuf_headPtr, (char *)&databuf_head, sizeof(struct pciedev_databuf_head));
    return ret;
}

Int32 pcie_slave_sendCmd(void *arg)
{
    //发送中断通知
    if( ti81xx_send_msi(NULL, gPciedev_slave_fd) < 0){
        err_print("send msi Error.\n");
        return -1;
    }
    return 0;    
}

Int32 pcie_slave_waitCmd(void *arg)
{
    int i = 0;
    int ret;
    struct pollfd poll_fd;
    poll_fd.events == POLLIN;
    poll_fd.fd = gPciedev_slave_fd;
    
    int intr_cntr;

    //poll wait
    printf("RC start recv msi.\n");
    while(1){
        ret = poll(&poll_fd, 1, 3000);//3 sec wait timeout
        if(( ret == POLLIN) || (poll_fd.revents == POLLIN)){
            debug_print("recv msi.\n");
            printf("%s: recieved msi-%d.\n", __func__, i);  
            //            break;
        }
        ioctl(gPciedev_slave_fd, TI81XX_GET_INTR_CNTR, &intr_cntr);
        printf("Pcie-%d: wait msi ret-0x%x, interrupt occur times-%d timeout-%d.\n", gSelf_id, ret, intr_cntr, i);
        i ++;
        //        usleep(10000);
    }
    return 0;    
}

Int32 pcie_slave_getCurTime(UInt32 *cur_jiffies)
{
    Int32 ret;
    UInt32  cur_time;
    if(gPciedev_slave_fd < 0)
        return -1;
    
    ret = ioctl(gPciedev_slave_fd, TI81XX_CUR_TIME, &cur_time);
    if(ret < 0){
        err_print("Pcie slave ioctl TI81XX_CUR_TIME Error.\n");
        return -1;
    }

    *cur_jiffies = cur_time;

    return 0;
}

Int32 pcie_slave_deInit(void)
{
    Int32 ret;

    if(gPciedev_slave_fd < 0)
        return 0;

    munmap(gMgmt_map_buf, SIZE_AREA);

    munmap(gData_map_buf, PCIE_NON_PREFETCH_SIZE);

    close(gPciedev_edma_fd);
    close(gPciedev_slave_fd);

    return 0;
}
