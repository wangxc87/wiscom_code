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
#include <drivers/char/pcie_common.h>
#include <signal.h>


#include "pcie_std.h"
#include "debug_msg.h"
#include "ti81xx_rc.h"
#include "pcie_common.h"

#define PCIE_RC_DEVICE  "/dev/ti81xx_ep_hlpr"


static Int32 gPciedev_master_fd = -1;

static char *gDataBuf_base = NULL;  //dataBuf baseAddr
static char *gDataBuf_recvSend = NULL;

Int32 gEp_nums = 0;
static Int32 gSelf_id = 0;

static struct pciedev_info gPciedev_info[PCIE_EP_MAX_NUMBER];
static struct mgmt_info *gMgmt_infoPtr[PCIE_EP_MAX_NUMBER];
static struct pciedev_databuf_head *gDatabuf_headPtr;

struct pcieRc_info {
    struct pciedev_info ep_info[PCIE_EP_MAX_NUMBER];
    char *local_resv_buf;
    int ep_nums;
    int initted_eps;
};

struct pcieRc_info gPciedev_obj;

//Get EP info in pcie_subsys and mapping the mgmt_buf
Int32 get_pcie_subsys_info(void)
{
    Int32  i=0, j=0;
    Int32 initted_eps = 0;
    Int32 ret;    

    struct ti81xx_outb_miscinfo misc_info;
        
    ret = ioctl(gPciedev_master_fd, TI81XX_RC_GET_EPNUMS, &gEp_nums);
    if(ret < 0){
        err_print("IOCTL: get eps nums fails\n");
        return -1;
    }

    ret = ioctl(gPciedev_master_fd, TI81XX_RC_GET_INITTED_EPNUMS, &initted_eps);
    if(ret < 0){
        err_print("IOCTL: get initted eps nums fails\n");
        return -1;
    }
    if(initted_eps == 0){
        err_print("There is %d Eps,but no registered\n", gEp_nums);
        return -1;
    }
    
    gPciedev_obj.ep_nums = gEp_nums;
    gPciedev_obj.initted_eps = initted_eps;
    
    printf("%s: There is %d Eps in system, registered %d .\n",__func__, gEp_nums, initted_eps);
    
    printf("PCIe resouce Info: \n");
    printf("Index\tBar\t  Start\t\t  Length\tdev_id\n");
    for(i = 0; i < gEp_nums; i++){
        misc_info.ep_index = i;
        ret = ioctl(gPciedev_master_fd, TI81XX_RC_GET_MISCINFO, &misc_info);
        if(ret < 0){
            err_print("RC_GET_MISCINFO %d Error.\n", i);
            return -1;
        }
        gPciedev_info[i].dev_id = misc_info.dev_id;
        memcpy(gPciedev_info[i].res_value, misc_info.res_value, sizeof(misc_info.res_value));
        for(j = 0; j < 6; j ++){
            printf(" %d \t %d \t0x%08x\t0x%08x\t %d\n",
                   i,j,gPciedev_info[i].res_value[j][0],gPciedev_info[i].res_value[j][1], gPciedev_info[i].dev_id);
        }
    }
    
    for(i = 0; i < gEp_nums; i ++){
        //mapping bar 2
        gPciedev_info[i].mgmt_buf_base = mmap(0, gPciedev_info[i].res_value[2][1],
                          PROT_READ | PROT_WRITE, MAP_SHARED,
                          gPciedev_master_fd, (off_t) gPciedev_info[i].res_value[2][0]);
        if((void *)-1 == (void *)gPciedev_info[i].mgmt_buf_base){
            err_print("Mmap Ep-%d Bar 0x%x memory Error.\n",
                      i, gPciedev_info[i].res_value[2][0]);
            goto error_exit;
        }
        gMgmt_infoPtr[i] = (struct mgmt_info *)(gPciedev_info[i].mgmt_buf_base);
        
    }
    return 0;

 error_exit:
    for(j = 0; j < i; j ++){
        munmap(gPciedev_info[j].mgmt_buf_base, gPciedev_info[j].res_value[2][1]);
    }
    return -1;
}


//默认等待时间120s
Int32 pcieRc_init(struct pciedev_init_config *config)
{
    struct ti81xx_start_addr_area start_addr;
    Int32 s32Ret = 0;
    UInt32 i;
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

    memset(&gPciedev_obj, 0, sizeof(struct pcieRc_info));
    
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
    printf("RC address of reserve mem is virt--0x%x phy--0x%x.\n",
                start_addr.start_addr_virt,start_addr.start_addr_phy);

    if(en_clearBuf){
        if(ioctl(gPciedev_master_fd, TI81XX_RC_RESET_CMDQUE, NULL) < 0)
            err_print("Ioctl : RESET_DATAQUE error.\n"); 
    }

    if(ioctl(gPciedev_master_fd, TI81XX_RC_GET_SENDCMD_INFO, &buf_info) < 0){
        err_print("Ioctl : GET_SENDCMD_INFO error.\n");
        return -1;
    } 

    gDataBuf_base = mmap(0,RC_RESV_MEM_SIZE_DEFAULT,
                         PROT_READ | PROT_WRITE, MAP_SHARED,
                         gPciedev_master_fd, (off_t)start_addr.start_addr_phy);

    if ((void *)-1 == (void *) gDataBuf_base) {
		err_print("RC MMAP of reserve memory fail\n");
		goto ERROR;
	}
    
    memset(gDataBuf_base, 0, RC_RESV_MEM_SIZE_DEFAULT);
    
    gPciedev_obj.local_resv_buf = gDataBuf_base;
    
    gDataBuf_recvSend = (char *)(gDataBuf_base + sizeof(struct pciedev_databuf_head ));
    
    //get pcie subsys info
    s32Ret = get_pcie_subsys_info();
    if(s32Ret < 0){
        err_print("Get pcie subsys info Error.\n");
        goto ERROR;
    }

    
    for(i = 0; i < gEp_nums; i++){
        gPciedev_info[i].send_cmd = gPciedev_info[i].mgmt_buf_base + RC_SEND_CMD_OFFSET(buf_info.buf_size);
        gPciedev_info[i].cmd_buf_size_max = buf_info.buf_size;
    }
    gDatabuf_headPtr = (struct pciedev_databuf_head *)gDataBuf_base;
    memset(gDatabuf_headPtr, 0, sizeof(struct pciedev_databuf_head));
  
    debug_print("rc init success\n");
    return 0;

 ERROR:
    munmap(gDataBuf_base, RC_RESV_MEM_SIZE_DEFAULT);
    close(gPciedev_master_fd);
    gPciedev_master_fd = -1;
    return s32Ret;
}

static Int32 pcieRc_sendAck(Int32 ep_id, UInt32 cmd, Int32 en_int)
{
    int ret;
    int msi_addr,i=0;
    while(i < PCIE_EP_MAX_NUMBER){
        if(gPciedev_info[i].dev_id == ep_id)
            break;
        i ++;
        if(i == PCIE_EP_MAX_NUMBER){
            err_print("Invalid Ep id.\n");
            return -1;
        }
    }

    gMgmt_infoPtr[i]->cmd |= cmd;

    if(en_int){
        msi_addr = gPciedev_info[i].res_value[0][0];
        ret = ioctl(gPciedev_master_fd, TI81XX_RC_SEND_MSI, msi_addr);
        if(ret < 0){
            debug_print("RC send msi Error, addr-0x%x", msi_addr);
            return -1;
        }
    }
    
    return 0;
}


static UInt32 frame_count = 0;

//data_ptr: not used
//timeout 0:表示默认时间，默认3s, NULL 一直等待
Int32 pcieRc_sendData(char *data_ptr, UInt32 buf_size, UInt32 pciedev_id, Int32 buf_channel, Int32 sync_mode, struct timeval *tv)
{
    Int32 ret;
    UInt32 i = 0,time_out,ep_index;
    if(buf_size >= RC_RESV_MEM_SIZE_DEFAULT){
        err_print("Send buf length too large.\n");
        return -1;
    }
    
    if(gPciedev_master_fd < 0)
        return -1;

    if(!tv)
        time_out = ~0;
    else
        time_out = tv->tv_sec * 5000 + tv->tv_usec/200;
 
    if(time_out == 0)
        time_out = 5000*DEFAULT_SEND_TIMEOUT;
     
    memcpy_neon(gDataBuf_recvSend, data_ptr, buf_size);

    gDatabuf_headPtr->data_from_id = RC_ID;
    gDatabuf_headPtr->frame_id = frame_count;
    gDatabuf_headPtr->rd_index = gEp_nums;
    gDatabuf_headPtr->buf_size = buf_size;
    gDatabuf_headPtr->buf_channal = buf_channel;
    gDatabuf_headPtr->sync_mode = sync_mode;
    
    for(i = 0; i < PCIE_EP_MAX_NUMBER; i ++)
        gDatabuf_headPtr->ep_rd_flag[i] &= ~DATA_ACK;    

    if(pciedev_id == EP_ID_ALL){
        gDatabuf_headPtr->data_to_id = EP_ID_ALL;
        gDatabuf_headPtr->wr_index = gEp_nums ;
        for(i = 0; i < gEp_nums; i ++){
            if(!gPciedev_info[i].dev_id)
                continue;
            ret = pcieRc_sendAck(gPciedev_info[i].dev_id, SEND_DATA, TRUE);
            if(ret < 0){
                err_print("send EP-%d cmd 0x%x failed.\n", i, i);
                gDatabuf_headPtr->data_to_id = 0;
                return -1;
            }
        }
    }else{
        gDatabuf_headPtr->data_to_id = pciedev_id;
        gDatabuf_headPtr->wr_index = 1;
        ret = pcieRc_sendAck(pciedev_id, SEND_DATA, TRUE);
        if(ret < 0){
            gDatabuf_headPtr->data_to_id = 0;
            return -1;
        }
    }

    if(frame_count != 0xfffffff0)
        frame_count ++;
    else
        frame_count = 0;

    i = 0;
   while(1){  //wait  send complete
   retry_test:

        if(!gDatabuf_headPtr->data_to_id)  //first send
            break;

        if(gDatabuf_headPtr->data_to_id == EP_ID_ALL){
            for(ep_index=0;ep_index < gEp_nums; ep_index ++){
            
                if(!gPciedev_info[ep_index].dev_id)
                    continue;
                
                if((gDatabuf_headPtr->ep_rd_flag[devid_to_index(gPciedev_info[ep_index].dev_id)] & DATA_ACK_ERR) == DATA_ACK_ERR){
                    //if ep read err
                    err_print("Ep%d Read Error*****\n", gPciedev_info[ep_index].dev_id);
                    return -1;
                }
                if((gDatabuf_headPtr->ep_rd_flag[devid_to_index(gPciedev_info[ep_index].dev_id)] & DATA_ACK) != DATA_ACK){
                    //if ep read or not
                     ep_index = 0;
                     break;        
                }                     
            }

            if(ep_index == gPciedev_obj.initted_eps){
                break;
            }
            
            if(i && (i%20 == 0))
                debug_print("[%u]  waiting [ALL_EPS] free buf %u [unit:200us]\n", frame_count, i);

        } else {
            
            if((gDatabuf_headPtr->ep_rd_flag[devid_to_index(gPciedev_info[ep_index].dev_id)] & DATA_ACK_ERR) == DATA_ACK_ERR){
                    //if ep read err
                    err_print("Ep%d Read Error*****\n", gDatabuf_headPtr->data_to_id);
                    return -1;
            }
            
            if(gDatabuf_headPtr->ep_rd_flag[devid_to_index(gDatabuf_headPtr->data_to_id)] & DATA_ACK)
                break;
            if(i && (i%20 == 0))
                debug_print("[%u] waiting [ep%d] free buf %u [unit:200us]\n", frame_count, gDatabuf_headPtr->data_to_id,i);
        }
        
        i ++;
        if(i == time_out){
            err_print("[%u] RC send data Timeout,No free buf.\n", frame_count);
            return  PCIEDEV_EBUSY;
        }
        usleep(200);
        goto retry_test;
    }
    
    debug_print("[%u] send data size %u.\n", frame_count, buf_size);

    return buf_size;
}

Int32 pcieRc_deInit(void)
{
    Int32 i = 0;
    
    if(gPciedev_master_fd < 0)
        return 0;
    

    for(i = 0; i < gEp_nums; i++){

        memset(gMgmt_infoPtr[i], 0, sizeof(struct mgmt_info));
        munmap(gPciedev_info[i].mgmt_buf_base, gPciedev_info[i].res_value[2][1]);
    }

    munmap(gDataBuf_base, RC_RESV_MEM_SIZE_DEFAULT);
    
    close(gPciedev_master_fd);
    gPciedev_master_fd = -1;
    
    return 0;
}


//timeout: 0:wait forever, others waiting time Unit [200us]
//return size of cmd or -1
Int32 pcieRc_recvCmd(char *buf,int *from_id, struct timeval *tv)
{
    Int32 buf_size = 0;  
    char *cmd_buf = NULL;
    struct pciedev_buf_info buf_info;
    
    if(gPciedev_master_fd < 0)
        return -1;

    if(!buf){
        err_print("Invalid input buf pointer.\n");
        return -1;
    }

    memset((char *)&buf_info, 0, sizeof(struct pciedev_buf_info));
 
    if(!tv){
        buf_info.tv_sec =(UInt32)0xffffff;
        buf_info.tv_usec = 0;
    } else{
       buf_info.tv_sec = tv->tv_sec;
       buf_info.tv_usec = tv->tv_usec;
    }
      
    if(!buf_info.tv_sec && !buf_info.tv_usec){ //defualt waiting time
        buf_info.tv_sec = DEFAULT_RECV_TIMEOUT;
        buf_info.tv_usec = 0;
    }

    if(ioctl(gPciedev_master_fd, TI81XX_RC_DEQUE_CMD, &buf_info) < 0){
        err_print("ioctl DEQUE_CMD error.\n");
        return -1;
    }
  
    cmd_buf = gPciedev_obj.local_resv_buf + buf_info.buf_ptr_offset;
    
    buf_size = buf_info.buf_size - 8;      
    *from_id = *(int *)(cmd_buf + 4);
    memcpy_neon(buf, cmd_buf + 8, buf_size );
    
    debug_print("get cmdInfo: buf_size-%d buf_ptr_offset-0x%x.\n", 
            buf_size, buf_info.buf_ptr_offset);
    
   if(ioctl(gPciedev_master_fd, TI81XX_RC_QUE_CMD, &buf_info) < 0){
        err_print("ioctl QUE_CMD error.\n");
        return -1;
    }
       
    return buf_size;

}

Int32 pcieRc_sendCmd(char *buf, Int32 to_id, UInt32 buf_size, struct timeval *tv)
{
    Int32 i = 0;
    UInt32 timeout;
    Int32 *buf_sizep, *buf_idp;
    
    char *send_buf = NULL;
    volatile Int32 *ep_cmd = NULL;
    
    if(gPciedev_master_fd < 0)
        return -1;
    
    while(i < PCIE_EP_MAX_NUMBER){
        if(gPciedev_info[i].dev_id == to_id)
            break;

        i ++;
        if(i == PCIE_EP_MAX_NUMBER){
            debug_print("Invalid EpId [%d].\n", to_id);
            return -1;
        }
    }
    
    if(buf_size + 4 > gPciedev_info[i].cmd_buf_size_max){
        err_print("Send buf size too large.\n");
        return -1;
    }
    
    send_buf = gPciedev_info[i].send_cmd;   
    buf_sizep = (int *)send_buf;
    buf_idp = (int *)(send_buf + 4);
    *buf_sizep = buf_size + 8;
    *buf_idp = gSelf_id;

    memcpy_neon(send_buf + 8, buf, buf_size);   
    
    if(pcieRc_sendAck(to_id, SEND_CMD, TRUE) < 0)
        return -1;

    ep_cmd = &gDatabuf_headPtr->ep_rd_flag[devid_to_index(to_id)];
    
    if(!tv)
        timeout = ~0;
    else
        timeout = tv->tv_sec*5000 + tv->tv_usec/200;
  
    if(timeout == 0)
        timeout = 5000*DEFAULT_SEND_TIMEOUT;
    
    while(timeout){
        if(*ep_cmd & CMD_ACK){
            *ep_cmd &= ~CMD_ACK;
            break;
        }
            
        usleep(200);
        timeout --;
        if(!timeout){
            err_print("rc send cmd timeout, failed.\n");
            return -1;
        }
    }
    return 0;
}


