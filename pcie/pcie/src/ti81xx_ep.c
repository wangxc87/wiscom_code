#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/ti81xx_pcie_epdrv.h>
#include <unistd.h>

#include "pcie_std.h"
#include "ti81xx_ep.h"
//#include "debug_msg.h"
#include "pcie_common.h"

#define PCIE_SLAVE_DEVICE "/dev/ti81xx_pcie_ep"
#define TI81XX_EDMA_DEVICE "/dev/ti81xx_edma_ep"



static Int32 gSelf_id = PCIE_INVALID_ID;
static Int32 gPciedev_slave_fd = -1;


static struct pciedev_info gPciedev_slave_info;
static struct ti81xx_pcie_mem_info gPciedev_slave_rsv_mem; //start_addr;

static Int32 pcie_slave_sendAck(UInt32 cmd, Int32 en_int);

Int32 pcie_slave_getInfo(UInt32 timeout)
{
    char *mapped_resv_buffer, *mapped_pci;
    Int32 ret;
    Int32 map_rsv_size ;

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
    printf("INFO: Slave start address of reserv_area is phy--0x%x,size--0x%x\n",
            gPciedev_slave_rsv_mem.base, gPciedev_slave_rsv_mem.size);
    map_rsv_size = gPciedev_slave_rsv_mem.size;

    mapped_resv_buffer = (char *) mmap(0, map_rsv_size, PROT_READ | PROT_WRITE,
            MAP_SHARED, gPciedev_slave_fd,
            (off_t) gPciedev_slave_rsv_mem.base);
    if ((void *) - 1 == (void *) mapped_resv_buffer) {
        err_print("pcie slave mapping dedicated memory fail\n");
        return -1;
    }
    //    gPciedev_mgmt_infoPtr = (struct mgmt_info *) mapped_resv_buffer;
    memset(&gPciedev_slave_info, 0, sizeof (struct pciedev_info));
    gPciedev_slave_info.dev_id = gSelf_id;
    gPciedev_slave_info.mgmt_buf_base = mapped_resv_buffer;
     
    //设置outboud
    mapped_pci = (char *) mmap(0, PCIE_NON_PREFETCH_SIZE,
            PROT_READ | PROT_WRITE, MAP_SHARED,
            gPciedev_slave_fd, 0x20000000);
    if ((void *) - 1 == (void *) mapped_pci) {
        err_print("mapping PCIE_NON_PREFETCH memory fail\n");
        goto ERROR;
    }

    gPciedev_slave_info.data_buf_base = (char *) mapped_pci;

    return 0;

ERROR:
    munmap(mapped_resv_buffer, map_rsv_size);
    return -1;
}
static UInt32 int_count = 0;


Int32 pcie_slave_init(struct pciedev_init_config *config)
{
    Int32 ret;
    UInt32 timeout;
    struct pciedev_buf_info buf_info;
    struct timeval *tv = NULL;
    Int32 en_clearBuf = TRUE;
    
    set_printLevel();

    gSelf_id = get_selfId();
    if (gSelf_id < 0) {
        err_print("Dev get selfid Error.\n");
        return -1;
    }

    if(config){
        tv = &config->tv;
        en_clearBuf = config->en_clearBuf;
    }
    
    if(!tv)
        timeout = ~0;
    else
        timeout = tv->tv_sec * 1000 + tv->tv_usec/1000;
    if(timeout == 0)
        timeout = 1000* DEFAULT_INIT_TIMEOUT;
    
    gPciedev_slave_fd = open(PCIE_SLAVE_DEVICE, O_RDWR);
    if (gPciedev_slave_fd < 0) {
        err_print("Open Pciedev slave %s Error.\n", PCIE_SLAVE_DEVICE);
        return -1;
    }

    if(en_clearBuf){
        if(ioctl(gPciedev_slave_fd, TI81XX_RESET_DATAQUE, NULL) < 0)
            err_print("Ioctl : RESET_DATAQUE error.\n");
        if(ioctl(gPciedev_slave_fd, TI81XX_RESET_CMDQUE, NULL) < 0)
            err_print("Ioctl : RESET_DATAQUE error.\n"); 
    }
    
    ret = pcie_slave_getInfo(timeout);
    if (ret < 0) {
        err_print("Pcie configure Error.\n");
        goto init_err_exit;
    }
    debug_print("pcie slave get info successfully.\n");

    if(ioctl(gPciedev_slave_fd, TI81XX_GET_SENDCMD_INFO, &buf_info) < 0){
        err_print("Ioctl : GET_SENDCMD_INFO error.\n");
        goto init_err_exit;
    }    
    gPciedev_slave_info.send_cmd = gPciedev_slave_info.mgmt_buf_base + buf_info.buf_ptr_offset;//EP_SEND_CMD_OFFSET(buf_info.buf_size);
    gPciedev_slave_info.cmd_buf_size_max = buf_info.buf_size;
    
    if(ioctl(gPciedev_slave_fd, TI81XX_GET_SENDDATA_INFO, &buf_info) < 0){
        err_print("Ioctl : GET_SENDDATA_INFO error.\n");
        goto init_err_exit;
    }    
    gPciedev_slave_info.data_buf_size_max = buf_info.buf_size;

    pcieDev_lockInit(&gPciedev_slave_info);

    return 0;

init_err_exit:
    pcieDev_lockDeInit(&gPciedev_slave_info);

    close(gPciedev_slave_fd);
    return -1;

}

static Int32 pcie_slave_sendAck(UInt32 cmd, Int32 en_int)
{
    int ret = 0;

    struct ti81xx_ack_info ack_info;
    ack_info.cmd = cmd;
    ack_info.en_int = en_int;
    ret = ioctl(gPciedev_slave_fd, TI81XX_SEND_ACK, &ack_info);
    if( ret < 0){
        return -1;
    }
    return 0;    
}

#define DATA_WAIT_TIMEOUT (30000)


Int32 pcie_slave_recvData(char *buf, UInt32 buf_size, Int32 *buf_channel, struct timeval *tv)
{

    Int32 data_size = 0;
    char *data_buf;
    struct pciedev_buf_info buf_info;
 
    memset((char *)&buf_info, 0, sizeof(struct pciedev_buf_info));

    if(!tv){
        buf_info.tv_sec = 0xffffff;
    } else{
        buf_info.tv_sec = tv->tv_sec;
        buf_info.tv_usec = tv->tv_usec;
    }
      
    if(!buf_info.tv_sec && !buf_info.tv_usec)//defualt waiting time
        buf_info.tv_sec = DEFAULT_RECV_TIMEOUT;

    
    if(ioctl(gPciedev_slave_fd, TI81XX_DEQUE_DATA, &buf_info) < 0){
        err_print("ioctl error.\n");
        return -1;
    }
    
    data_buf = gPciedev_slave_info.mgmt_buf_base + buf_info.buf_ptr_offset;
    data_size = buf_info.buf_size;
    
    data_size -= 4;
    *buf_channel = *data_buf;
    memcpy_neon(buf, data_buf + 4, data_size);
    debug_print("get dataInfo: chnID-%d buf_size-%d buf_ptr_offset-0x%x.\n", 
    *buf_channel, data_size, buf_info.buf_ptr_offset);
    
   if(ioctl(gPciedev_slave_fd, TI81XX_QUE_DATA, &buf_info) < 0){
        err_print("ioctl error.\n");
        return -1;
    }
    return data_size;

}


//return size of cmd or -1
Int32 pcie_slave_recvCmd(char *buf, int *from_id, struct timeval *tv)
{

    Int32 buf_size = 0;  
    char *cmd_buf = NULL;
    struct pciedev_buf_info buf_info;
    
    if(gPciedev_slave_fd < 0)
        return -1;

    if(!buf){
        err_print("Invalid input buf pointer.\n");
        return -1;
    }

    memset((char *)&buf_info, 0, sizeof(struct pciedev_buf_info));
    
   if(!tv){
       buf_info.tv_sec = 0xffffff;
   } else{
       buf_info.tv_sec = tv->tv_sec;
       buf_info.tv_usec = tv->tv_usec;
    }
      
    if(!buf_info.tv_sec && !buf_info.tv_usec)//defualt waiting time
        buf_info.tv_sec = DEFAULT_RECV_TIMEOUT;

    if(ioctl(gPciedev_slave_fd, TI81XX_DEQUE_CMD, &buf_info) < 0){
        err_print("ioctl error.\n");
        return -1;
    }
       
    cmd_buf = gPciedev_slave_info.mgmt_buf_base + buf_info.buf_ptr_offset;
    
    buf_size = buf_info.buf_size - 8;      
    *from_id = *(int *)(cmd_buf + 4);
    memcpy_neon(buf, cmd_buf + 8, buf_size );
    
    debug_print("get cmdInfo: buf_size-%d buf_ptr_offset-0x%x.\n", 
                buf_size, buf_info.buf_ptr_offset);
    
   if(ioctl(gPciedev_slave_fd, TI81XX_QUE_CMD, &buf_info) < 0){
        err_print("ioctl error.\n");
        return -1;
    }
       
    return buf_size;

}

//to_id: not used , only support rc
Int32 pcie_slave_sendCmd(char *buf, Int32 to_id, UInt32 buf_size, struct timeval *tv)
{
    char *send_buf = NULL;
    Int32 *buf_sizep, *buf_idp;
    Int32 ret = 0;
    struct ti81xx_ack_info ack_info;

    
    if(gPciedev_slave_fd < 0)
        return -1;
    
    memset(&ack_info, 0, sizeof(struct ti81xx_ack_info));

    if(buf_size + 8 > gPciedev_slave_info.cmd_buf_size_max){
        err_print("Send buf size too large.\n");
        return -1;
    }
    
    send_buf = gPciedev_slave_info.send_cmd;
    buf_sizep = (int *)send_buf;
    buf_idp = (int *)(send_buf + 4);
    *buf_sizep = buf_size + 8;
    *buf_idp = gSelf_id;

    memcpy_neon(send_buf + 8, buf, buf_size);
    
    if(pcie_slave_sendAck(SEND_CMD, TRUE) < 0){
        err_print("[pcie-%d] send ACK failed.\n", gSelf_id);
        return -1;
    }
    
   if(!tv){
       ack_info.tv_sec = 0xffffff;
   } else{
       ack_info.tv_sec = tv->tv_sec;
       ack_info.tv_usec = tv->tv_usec;
    }
      
    if(!ack_info.tv_sec && !ack_info.tv_usec)//defualt waiting time
        ack_info.tv_sec = DEFAULT_SEND_TIMEOUT;

    ret = ioctl(gPciedev_slave_fd, TI81XX_WAIT_CMD_ACK, &ack_info);
    if( ret < 0){
        err_print("send cmd error.\n");
        return -1;
    }

    if(!ack_info.ack_status)
        return -1;

    return 0;
}

Int32 pcie_slave_getCurTime(UInt32 *cur_jiffies)
{
    Int32 ret;
    UInt32 cur_time;
    if (gPciedev_slave_fd < 0)
        return -1;

    ret = ioctl(gPciedev_slave_fd, TI81XX_CUR_TIME, &cur_time);
    if (ret < 0) {
        err_print("Pcie slave ioctl TI81XX_CUR_TIME Error.\n");
        return -1;
    }

    *cur_jiffies = cur_time;

    return 0;
}

Int32 pcie_slave_deInit(void)
{

    if (gPciedev_slave_fd < 0)
        return 0;

    pcieDev_lock(&gPciedev_slave_info);

    close(gPciedev_slave_fd);
    gPciedev_slave_fd = -1;
    
    munmap(gPciedev_slave_info.mgmt_buf_base, SIZE_AREA);

    munmap(gPciedev_slave_info.data_buf_base, PCIE_NON_PREFETCH_SIZE);
    gPciedev_slave_info.mgmt_buf_base = NULL;
    gPciedev_slave_info.data_buf_base= NULL;
    //    gPciedev_mgmt_infoPtr = NULL;
    pcieDev_unlock(&gPciedev_slave_info);
    
    pcieDev_lockDeInit(&gPciedev_slave_info);
    
    debug_print("%s: *** receive %u inters**.\n", __func__, int_count);
    
    return 0;
}
