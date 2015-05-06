/* 
 * File:   osa_pcie.h
 * Author: wxc
 *
 * Created on 2015年1月27日, 上午9:30
 */

#ifndef OSA_PCIE_H
#define	OSA_PCIE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

#include "pcie_std.h"
#include "pcie_common.h"
    //#include <drivers/char/pcie_common.h>    

#define PCIE_DATA_BUF_SIZE (1<<20)

#define DATA_SEND_SYNC_MODE   1
#define DATA_SEND_NOSYNC_MODE 0

extern int gLocal_id;

Int32 OSA_pcieInit(struct pciedev_init_config *config);

Int32 OSA_pcieSendData(Int32 to_id, char *buf_ptr, Int32 buf_siz, Int32 buf_channel, Int32 sync_mode, struct timeval *tv); 

Int32 OSA_pcieRecvData(char *recv_buf, Int32 buf_size, Int32 *buf_channel, struct timeval *tv);

Int32 OSA_pcieDeInit(void);

Int32 OSA_pcieRecvCmd(char *buf,Int32 *from_id, struct timeval *tv);

Int32 OSA_pcieSendCmd(Int32 to_id, char *buf, UInt32 buf_size, struct timeval *tv);


#ifdef	__cplusplus
}
#endif

#endif	/* OSA_PCIE_H */
