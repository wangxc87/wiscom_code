#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <poll.h>
#include <string.h>
#include <drivers/char/ti81xx_pcie_rcdrv.h>

#include "pcie_std.h"
#include "ti81xx_pci_info.h"
#include "pcie_common.h"
#include "debug_msg.h"

#define PCIE_RC_DEVICE  "/dev/ti81xx_ep_hlpr"
#define PCIE_EP_MAX_NUMBER 3

static Int32 gPciedev_master_fd = -1;

//static char *gMgmt_buf = NULL;
static char *gDataBuf_base = NULL;
static char *gDataBuf_recvSend = NULL;

Int32 gEp_nums = 0;

static struct pciedev_info gPciedev_info[PCIE_EP_MAX_NUMBER];
static struct mgmt_info *gMgmt_infoPtr[PCIE_EP_MAX_NUMBER];
static struct pciedev_databuf_head *gDatabuf_headPtr;

//Get EP info in pcie_subsys and mapping the mgmt_buf
Int32 get_pcie_subsys_info(void)
{
    Int32 ret, i, j;
    struct pci_sys_info	*start=NULL,*temp;

    gEp_nums = get_devices(&start);
    if (gEp_nums < 0) {
		err_print("fetching pci sub system info on rc fails\n");
        return -1;
	}
    debug_print("no of Eps in system is %d.\n", gEp_nums);
    
    i = 0;
	for (temp = start; temp != NULL; temp = temp->next) {
        for(j = 0; j < 6; j ++){
            if((temp->res_value[j + 1][0] != 0) && (temp->res_value[j + 1][1] != 0)){
                gPciedev_info[i].res_value[j][0] = temp->res_value[j + 1][0];
                gPciedev_info[i].res_value[j][1] = temp->res_value[j + 1][1];          
                debug_print("Res address of EP-%d is 0x%x size is 0x%x\n",
                            i,temp->res_value[j][0],temp->res_value[j][1]);
            }
        }
        i ++;
    }

    for(i = 0; i < gEp_nums; i ++){
        //mapping bar 2
        gPciedev_info[i].mgmt_buf = mmap(0, gPciedev_info[i].res_value[2][1],
                          PROT_READ | PROT_WRITE, MAP_SHARED,
                          gPciedev_master_fd, (off_t) gPciedev_info[i].res_value[2][0]);
        if((void *)-1 == (void *)gPciedev_info[i].mgmt_buf){
            err_print("Mmap Ep-%d Bar 0x%x memory Error.\n",
                      i, gPciedev_info[i].res_value[2][0]);
            goto error_exit;
        }
        memset(gPciedev_info[i].mgmt_buf, 0, gPciedev_info[i].res_value[2][1]);
        gMgmt_infoPtr[i] = (struct mgmt_info *)(gPciedev_info[i].mgmt_buf);
        
    }
    return 0;
 error_exit:
    for(i; i < 0; i --){
        munmap(gPciedev_info[i].mgmt_buf, gPciedev_info[i].res_value[2][1]);
    }
    return -1;
}


Int32 pcieRc_init(void)
{
    struct ti81xx_start_addr_area start_addr;
    Int32 s32Ret = 0;
    UInt32 i, j;
    gPciedev_master_fd = open(PCIE_RC_DEVICE, O_RDWR);
    if(gPciedev_master_fd < 0){
        err_print("Open device %s Error.\n", PCIE_RC_DEVICE);
        return -1;
    }
    
    s32Ret = ioctl(gPciedev_master_fd, TI81XX_RC_START_ADDR_AREA, &start_addr); 
	if (s32Ret < 0) {
		err_print("ioctl START_ADDR failed\n");
		goto ERROR;
	}
    debug_print("RC address of reserve mem is virt--0x%x phy--0x%x.\n",
                start_addr.start_addr_virt,start_addr.start_addr_phy);

    gDataBuf_base = mmap(0,PCIE_RC_RESERVE_MEM_SIZE,
                         PROT_READ | PROT_WRITE, MAP_SHARED,
                         gPciedev_master_fd, (off_t)start_addr.start_addr_phy);

	if ((void *)-1 == (void *) gDataBuf_base) {
		err_print("RC MMAP of reserve memory fail\n");
		goto ERROR;
	}
    memset(gDataBuf_base, 0, PCIE_RC_RESERVE_MEM_SIZE);
    gDataBuf_recvSend = (char *)(gDataBuf_base + sizeof(struct pciedev_databuf_head ));

    //get pcie subsys info
    s32Ret = get_pcie_subsys_info();
    if(s32Ret < 0){
        err_print("Get pcie subsys info Error.\n");
        goto ERROR;
    }
    printf("Pcie get subsys info successfully.\n");
    
    for(i = 0; i < gEp_nums; i ++){
        gMgmt_infoPtr[i]->ep_id = EP_ID_ALL;
        gMgmt_infoPtr[i]->wr_index = 1;
        gMgmt_infoPtr[i]->ep_outbase = start_addr.start_addr_phy;
    }
#define INIT_TIMEOUT (3*100000)
    j = 0;
    printf("Wait slave respond.\n");
    while(j < INIT_TIMEOUT){
        usleep(10);
        for(i = 0; i < gEp_nums; i ++){
            if(gMgmt_infoPtr[i]->ep_initted != TRUE)
                break;
        }
        if(i == gEp_nums)
            break;
        j++;
    }
    
    if(INIT_TIMEOUT == j){
        err_print("Wait PCIe subsys init timeout.\n");
        return -1;
    }
   return 0;

 ERROR:
    munmap(gDataBuf_base, PCIE_RC_RESERVE_MEM_SIZE);
    close(gPciedev_master_fd);
    gPciedev_master_fd = -1;
    return s32Ret;
}

//get data buf Ptr
char *OSA_pciReqDataBuf(UInt32 buf_size)
{
    Int32 ret = 0;
    char *ptr_tmp;
    if(buf_size >= PCIE_RC_RESERVE_MEM_SIZE){
        err_print("Require buf size not avaliable,Maxsiz is 0x%x.",PCIE_RC_RESERVE_MEM_SIZE);
        return NULL;
    }

    gDatabuf_headPtr = (struct pciedev_databuf_head *)gDataBuf_base;
    memset(gDatabuf_headPtr, 0, sizeof(struct pciedev_databuf_head));

    ptr_tmp = gDataBuf_recvSend;
        
    return ptr_tmp;
}
static UInt32 frame_count = 0;

//data_ptr: not used timeout 0:表示默认时间,单位ms
Int32 OSA_pcieSendData(char *data_ptr, UInt32 buf_size,UInt32 pciedev_id, UInt32 timeout)
{
    Int32 ret;
    UInt32 i = 0,time_out;
    
    if(buf_size >= PCIE_RC_RESERVE_MEM_SIZE){
        err_print("Send buf length too large.\n");
        return -1;
    }
    if(timeout == 0)
        time_out = INIT_TIMEOUT;
    else
        time_out = timeout * 500;

    while(gDatabuf_headPtr->wr_index != 0){
        usleep(2);
        i ++;
        if(i == time_out){
            err_print("RC send data Timeout,No free buf.\n");
            return -1;
        }
    }

    memcpy_neon(gDataBuf_recvSend, data_ptr, buf_size);
    
    gDatabuf_headPtr->data_from_id = RC_ID;
    gDatabuf_headPtr->frame_id = frame_count;
    gDatabuf_headPtr->rd_index = gEp_nums;
    gDatabuf_headPtr->buf_size = buf_size;

    if(pciedev_id == EP_ID_ALL){
        gDatabuf_headPtr->data_to_id = EP_ID_ALL;
        gDatabuf_headPtr->wr_index = gEp_nums ;
    }else{
        gDatabuf_headPtr->data_to_id = pciedev_id;
        gDatabuf_headPtr->wr_index = 1;
    }

    if(frame_count != 0xfffffff0)
        frame_count ++;
    else
        frame_count = 0;

    return buf_size;
}
Int32 OSA_pcieSendCmd(void *arg)
{
    int ret;
    int msi_addr;
    msi_addr = 0x21800000;
    ret = ioctl(gPciedev_master_fd, TI81XX_RC_SEND_MSI, msi_addr);
    if(ret < 0){
        debug_print("RC send msi Error, addr-0x%x", msi_addr);
        return -1;
    }
    return 0;
}
Int32 OSA_pcieWaitCmd(void *arg)
{
    int i = 0;
    int ret;
    struct pollfd poll_fd;
    poll_fd.events == POLLIN;
    poll_fd.fd = gPciedev_master_fd;

    //poll wait
    printf("RC start recv msi.\n");
    while(1){
        ret = poll(&poll_fd, 1, 3000);//3 sec wait timeout
        if(( ret == POLLIN) || (poll_fd.revents == POLLIN)){
            debug_print("recv msi.\n");
            printf("%s: recieved msi-%d.\n", __func__,i);
            //            break;
        }
        printf("wait msi, timeout-%d.\n", i);
        i ++;
        //        usleep(10000);
    }
    return 0;    
}

#if 0
Int32 OSA_pcieRecvData(char *data_ptr,UInt32 *buf_size)
{
    Int32 ret;
    UInt32 i = 0;
    UInt32 recv_size;
    
    if(buf_size >= PCIE_RC_RESERVE_MEM_SIZE){
        err_print("Recv buf length too large.\n");
        return -1;
    }

    while(gDatabuf_headPtr->data_id != RC_ID){
        usleep(2);
        i ++;
        if(i == INIT_TIMEOUT){
            err_print("RC Recv data Timeout,No Data buf.\n");
            return -1;
        }
    }
    
    gDatabuf_headPtr->data_id = INVALID_ID;
    recv_size = gDatabuf_headPtr->buf_size;

    return recv_size;
}
#endif

Int32 pcieRc_deInit(void)
{
    Int32 ret;
    Int32 i;
    if(gPciedev_master_fd < 0)
        return 0;
    for(i = 0; i < gEp_nums; i++){
        munmap(gPciedev_info[i].mgmt_buf, gPciedev_info[i].res_value[2][1]);
    }
    munmap(gDataBuf_base, PCIE_RC_RESERVE_MEM_SIZE);
    return 0;
}
