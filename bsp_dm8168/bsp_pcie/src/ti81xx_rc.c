#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>
#include <linux/ti81xx_pcie_rcdrv.h>

#include "ti81xx_rc.h"
#include "pcie_common.h"

#define PCIE_RC_DEVICE  "/dev/ti81xx_ep_hlpr"


static Int32 gPciedev_master_fd = -1;

static char *gDataBuf_base = NULL;  //dataBuf baseAddr
static char *gDataBuf_recvSend = NULL;

Int32 gEp_nums = 0;
static Int32 gSelf_id = 0;

static struct pciedev_info gPciedev_info[PCIE_EP_MAX_NUMBER];
//static struct mgmt_info *gMgmt_infoPtr[PCIE_EP_MAX_NUMBER];
static struct pciedev_databuf_head *gDatabuf_headPtr;

struct pcieRc_info {
    struct pciedev_info ep_info[PCIE_EP_MAX_NUMBER];
    char *local_resv_buf;
    char *local_outb_buf;
    int ep_nums;
    int initted_eps;
};

struct pcieRc_info gPciedev_obj;

//Get EP info in pcie_subsys and mapping the mgmt_buf
#define DEFAULT_RC_INIT_TIMEOUT_INSECOND   (180)
Int32 get_pcie_subsys_info(struct timeval *tv)
{
    Int32  i=0, j=0;
    Int32 initted_eps = 0;
    Int32 ret = 0;
    Int32 timeout_in_500ms = 0;
    Int32 poll_interlace_ms = 500;
    struct ti81xx_outb_miscinfo misc_info;

    if(!tv)
            timeout_in_500ms = DEFAULT_RC_INIT_TIMEOUT_INSECOND *1000/poll_interlace_ms;
    else 
            timeout_in_500ms = (tv->tv_sec * 1000 + tv->tv_usec/1000)/poll_interlace_ms;

    if(timeout_in_500ms == 0)
            timeout_in_500ms = DEFAULT_RC_INIT_TIMEOUT_INSECOND *1000/poll_interlace_ms;
    
    ret = ioctl(gPciedev_master_fd, TI81XX_RC_GET_EPNUMS, &gEp_nums);
    if(ret < 0){
        err_print("IOCTL: get eps nums fails\n");
        return -1;
    }

    i = 0;
    while(1){
            ret = ioctl(gPciedev_master_fd, TI81XX_RC_GET_INITTED_EPNUMS, &initted_eps);
            if(ret < 0){
                    err_print("IOCTL: get initted eps nums fails\n");
                    return -1;
            }
            if(initted_eps == gEp_nums)
                    break;

            if(i == timeout_in_500ms){
                    err_print("There is %d Eps,but Only %d registered\n", gEp_nums);
                    return -1;
            }
            i ++;
            usleep(poll_interlace_ms * 1000);
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

    gPciedev_obj.local_outb_buf = (char *)mmap(0, PCI_NON_PREFETCH_SIZE,/* gPciedev_info[i].res_value[2][1], */
                                              PROT_READ | PROT_WRITE, MAP_SHARED,
                                               gPciedev_master_fd, (off_t)PCI_NON_PREFETCH_START);// gPciedev_info[i].res_value[2][0]);
    if((void *)-1 == (void *)gPciedev_obj.local_outb_buf){
        err_print("Mmap outb 0x%x memory Error.\n", PCI_NON_PREFETCH_START);
        return -1;
    }

    return 0;

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


    gDataBuf_base = mmap(0,RC_RESV_MEM_SIZE_DEFAULT,
                         PROT_READ | PROT_WRITE, MAP_SHARED,
                         gPciedev_master_fd, (off_t)start_addr.start_addr_phy);

    if ((void *)-1 == (void *) gDataBuf_base) {
		err_print("RC MMAP of reserve memory fail\n");
		goto ERROR;
	}
    
    memset(gDataBuf_base, 0, RC_RESV_MEM_SIZE_DEFAULT);
    
    
    //get pcie subsys info
    s32Ret = get_pcie_subsys_info(tv);
    if(s32Ret < 0){
        err_print("Get pcie subsys info Error.\n");
        goto ERROR;
    }

    for(i = 0; i < gEp_nums; i++){

        if(!gPciedev_info[i].dev_id)
            continue;

        buf_info.ep_id = gPciedev_info[i].dev_id;
        if(ioctl(gPciedev_master_fd, TI81XX_RC_GET_SENDCMD_INFO, &buf_info) < 0){
            err_print("Ioctl : GET_SENDCMD_INFO error.\n");
            goto ERROR;
        } 

        gPciedev_info[i].send_cmd = (char *)gPciedev_obj.local_outb_buf + buf_info.buf_ptr_offset;
        gPciedev_info[i].cmd_buf_size_max = buf_info.buf_size;
        debug_print("EP%d send_cmd buf_ptr_offset is 0x%08x\n", gPciedev_info[i].dev_id, buf_info.buf_ptr_offset);
    }

    
    if(ioctl(gPciedev_master_fd, TI81XX_RC_GET_SENDDATA_INFO, &buf_info) < 0){
        err_print("Ioctl : GET_SENDDATA_INFO error.\n");
        goto ERROR;
    } 
    
    gDataBuf_recvSend = (char *)gDataBuf_base + buf_info.buf_ptr_offset;//sizeof(struct pciedev_databuf_head ));

    gPciedev_obj.local_resv_buf = gDataBuf_base;
    

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
    int i=0;
    struct ti81xx_ack_info ack_info;
    while(i < PCIE_EP_MAX_NUMBER){
        if(gPciedev_info[i].dev_id == ep_id)
            break;
        i ++;
        if(i == PCIE_EP_MAX_NUMBER){
            err_print("Invalid Ep id.\n");
            return -1;
        }
    }
    memset((char *)&ack_info, 0, sizeof(struct ti81xx_ack_info));
    ack_info.ep_id = ep_id;
    ack_info.cmd = cmd;
    ack_info.en_int = en_int;
    if(ioctl(gPciedev_master_fd, TI81XX_RC_SEND_ACK, &ack_info) <0){
        debug_print("RC send ack Error \n");
        return -1;
    }
    return 0;
}


static UInt32 frame_count = 0;

//data_ptr: not used
//timeout 0:表示默认时间，默认3s, NULL 一直等待
Int32 pcieRc_sendData(char *data_ptr, UInt32 buf_size, UInt32 pciedev_id, Int32 buf_channel, Int32 sync_mode, struct timeval *tv)
{
    Int32 ret;
    UInt32 i = 0,ep_index;
    struct ti81xx_ack_info ack_info;
    struct timeval tv_out;
    
    memset(&ack_info, 0, sizeof(struct ti81xx_ack_info));

    if(buf_size >= RC_RESV_MEM_SIZE_DEFAULT){
        err_print("Send buf length too large.\n");
        return -1;
    }
    
    if(gPciedev_master_fd < 0)
        return -1;

    if(!tv){
            tv_out.tv_sec = 0x7fffffff;
            tv_out.tv_usec = 0x7fffffff;
    }else    if((tv->tv_sec == 0) &&(tv->tv_usec == 0)){
            tv_out.tv_sec = DEFAULT_SEND_TIMEOUT;
            tv_out.tv_usec = 0;
    }else {
            tv_out.tv_sec = tv->tv_sec;
            tv_out.tv_usec = tv->tv_usec;
    }
     
    memcpy_neon((char *)gDataBuf_recvSend, (char *)data_ptr, buf_size);

    gDatabuf_headPtr->data_from_id = RC_ID;
    gDatabuf_headPtr->frame_id = frame_count;
    gDatabuf_headPtr->rd_index = gEp_nums;
    gDatabuf_headPtr->buf_size = buf_size;
    gDatabuf_headPtr->buf_channal = buf_channel;
    gDatabuf_headPtr->sync_mode = sync_mode;
    
    if(pciedev_id == EP_ID_ALL){
        gDatabuf_headPtr->data_to_id = EP_ID_ALL;
        gDatabuf_headPtr->wr_index = gEp_nums ;
        for(i = 0; i < gEp_nums; i ++){
            if(!gPciedev_info[i].dev_id)
                continue;
            ret = pcieRc_sendAck(gPciedev_info[i].dev_id, SEND_DATA, TRUE);
            if(ret < 0){
                err_print("send EP-%d cmd 0x%x failed.\n", gPciedev_info[i].dev_id, SEND_DATA);
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
            err_print("send EP-%d cmd 0x%x failed.\n", pciedev_id, SEND_DATA);            
            return -1;
        }
    }

    if(frame_count != 0xfffffff0)
        frame_count ++;
    else
        frame_count = 0;

    do {

        if(gDatabuf_headPtr->data_to_id == EP_ID_ALL){
            for(ep_index=0;ep_index < gEp_nums; ep_index ++){

                if(!gPciedev_info[ep_index].dev_id)
                    continue;

                ack_info.ep_id = gPciedev_info[ep_index].dev_id;
                ack_info.tv_sec = tv_out.tv_sec;
                ack_info.tv_usec = tv_out.tv_usec;
                if(ioctl(gPciedev_master_fd, TI81XX_RC_WAIT_DATA_ACK, &ack_info) < 0){
                    //if ep read or not
                        err_print("ioctl: wait ep%d data ack error.\n", gPciedev_info[ep_index].dev_id);
                        ep_index = 0;
                        return -1;
                }
                
                if(!ack_info.ack_status){
                    //if ep read or not
                     ep_index = 0;
                     err_print("wait ep%d data ackStatus error-%d.\n", gPciedev_info[ep_index].dev_id, ack_info.ack_status);
                     return -1;
                }
            }

            if(ep_index == gPciedev_obj.initted_eps){
                break;
            } 
        } else {
            ack_info.ep_id = gDatabuf_headPtr->data_to_id;
            ack_info.tv_sec = tv_out.tv_sec;
            ack_info.tv_usec = tv_out.tv_usec;
            if(ioctl(gPciedev_master_fd, TI81XX_RC_WAIT_DATA_ACK, &ack_info) < 0){
                //if ep read or not
                    err_print("ioctl: wait ep%d data ack error.\n",  gDatabuf_headPtr->data_to_id);
                    return -1;
            }

            if(ack_info.ack_status)
                break;
        }
    }while(0);  //wait  send complete
    debug_print("[%u] send data size %u.\n", frame_count, buf_size);
    return buf_size;
}

Int32 pcieRc_deInit(void)
{
    if(gPciedev_master_fd < 0)
        return 0;
    
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
    char *send_buf = NULL;
    Int32 ret = 0;
    UInt32 cmd_head[2];
    struct ti81xx_ack_info ack_info;
    
    if(gPciedev_master_fd < 0)
        return -1;

    memset(&ack_info, 0, sizeof(struct ti81xx_ack_info));
    
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
    
    send_buf = (char *)gPciedev_info[i].send_cmd;   

    cmd_head[0] = buf_size + 8;
    cmd_head[1] = gSelf_id;
    memcpy_neon((char *)send_buf, (char *)cmd_head, 8);
    memcpy_neon(send_buf + 8, buf, buf_size);   
    
    if(pcieRc_sendAck(to_id, SEND_CMD, TRUE) < 0){
            err_print("SendAck Error.\n");
            return -1;
    }

    if(!tv){
        ack_info.tv_sec =(UInt32)0xffffff;
        ack_info.tv_usec = 0;
    } else{
       ack_info.tv_sec = tv->tv_sec;
       ack_info.tv_usec = tv->tv_usec;
    }
      
    if(!ack_info.tv_sec && !ack_info.tv_usec){ //defualt waiting time
        ack_info.tv_sec = DEFAULT_SEND_TIMEOUT;
        ack_info.tv_usec = 0;
    }
    ack_info.ep_id = to_id;

    ret = ioctl(gPciedev_master_fd, TI81XX_RC_WAIT_CMD_ACK, &ack_info);
    if( ret < 0){
        err_print("wait_cmd_ack ioctl failed.\n");
        return -1;
    }

    if(!ack_info.ack_status){
        err_print("Error: wait no cmd_ack.\n");
        return -1;
    }

    return 0;
}


