#include <stdio.h>
#include <string.h>

#include "pcie_common.h"
#include "ti81xx_rc.h"
#include "ti81xx_ep.h"

int gLocal_id = 0;

Int32 OSA_pcieInit(struct pciedev_init_config *config)
{
    Int32 ret;
    gLocal_id = get_selfId();
    if (gLocal_id < 0) {
        fprintf(stderr,"%s: Dev get selfid Error.\n", __func__);
        return -1;
    }
    fprintf(stdout, "%s: Get devId is %d.\n", __func__, gLocal_id);
    
    switch(gLocal_id){
        case RC_ID:
            ret = pcieRc_init(config);
            break;
        case EP_ID_0:
        case EP_ID_1:
        case EP_ID_2:            
            ret = pcie_slave_init(config);     
            break;
        default :
            fprintf(stderr, "Invalid localID-0x%x.\n", gLocal_id);
            return -1;
    }
    if(ret < 0){
        //OSA_printf("Pcie-%d:pcie init Error.\n", gLocal_id);
        return -1;
    }
    return 0;
}

Int32 OSA_pcieSendData(Int32 to_id, char *buf_ptr, Int32 buf_size, Int32 buf_channel, Int32 sync_mode, struct timeval *tv)
{
    Int32 ret;
    if(gLocal_id == to_id){
        fprintf(stderr, "pcie-%d:Can Not send to itself.", gLocal_id);
        return -1;
    }
    if((gLocal_id != RC_ID)&&(to_id == EP_ID_ALL)){
        fprintf(stderr, "pcie-%d:slave can not send to EP_ALL.\n",gLocal_id);
        return 0;
    }
        
    if(to_id == RC_ID){
        //slave send function to do
        return 0;
    }else{
        ret = pcieRc_sendData(buf_ptr, buf_size, to_id, buf_channel,sync_mode, tv);
        if(ret < 0){
            //OSA_printf("pcie-%d:send data to pcieId-%d Error.\n", gLocal_id, to_id);
            return -1;
        }
       
    }
    return ret;
}

Int32 OSA_pcieRecvData(char *recv_buf, Int32 buf_size, Int32 *buf_channel, struct timeval *tv)
{
    Int32 ret;

    if(gLocal_id == RC_ID){
        //to do 
        return 0;
    }else{
        ret = pcie_slave_recvData(recv_buf, buf_size, buf_channel, tv);
        if(ret < 0){
            //OSA_printf("pcie-%d:recv data Error.\n", gLocal_id);
            return -1;
        }
    }
    return ret;
}

Int32 OSA_pcieDeInit(void)
{
    Int32 ret = 0;
    if(gLocal_id == RC_ID)
        ret = pcieRc_deInit();
    else
        ret = pcie_slave_deInit();
    return ret;
}

Int32 OSA_pcieRecvCmd(char *buf,Int32 *from_id, struct timeval *tv)
{
    Int32 ret;

    if(gLocal_id == RC_ID)
        ret = pcieRc_recvCmd(buf,from_id, tv);
    else
        ret = pcie_slave_recvCmd(buf, from_id, tv);
    return ret;
}

Int32 OSA_pcieSendCmd(Int32 to_id, char *buf, UInt32 buf_size, struct timeval *tv)
{
    Int32 ret;

    if(gLocal_id == to_id){
        fprintf(stderr, "pcie-%d:Can Not send to itself.", gLocal_id);
        return -1;
    }

    if(gLocal_id == RC_ID)
        ret = pcieRc_sendCmd(buf, to_id, buf_size, tv);
    else
        ret = pcie_slave_sendCmd(buf, RC_ID, buf_size, tv);

    return ret;
}
