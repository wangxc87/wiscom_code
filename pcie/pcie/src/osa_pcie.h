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

#define PCIE_DATA_BUF_SIZE (1<<20)

#define DATA_SEND_SYNC_MODE   1
#define DATA_SEND_NOSYNC_MODE 0

extern int gLocal_id;

/* #define EP_ID_0     1 */
/* #define EP_ID_1     2 */
/* #define EP_ID_2     3 */
/* #define RC_ID       4 */
/* #define EP_ID_ALL   (0xff) */
/* #define PCIE_INVALID_ID  (0) */
    
 /* ===  FUNCTION  ====================================================================== */
 /*         Name:  OSA_pcieInit */
 /*  Description:  pcie初始化 */
 /*  Input Args :  config初始化参数， */
 /*                config->en_clearBuf：TRUE清除驱动曾缓冲（建议）, FALSE 不清除驱动成缓冲区 */
 /*  Output Args:  无 */
 /*  Return     :  成功时返回0；错误时返回-1 */
 /* ===================================================================================== */
Int32 OSA_pcieInit(struct pciedev_init_config *config);


/* ===  FUNCTION  ====================================================================== */
/*         Name:  OSA_pcieSendData */
/*  Description:  pcie发送数据，仅主机支持该接口 */
/*  Input Args :  to_id: 发送的目标ID，EP_ID_0,EP_ID_1,EP_ID_2, EP_ID_ALL */
/*                buf_ptr:   发送数据区指针 */
/*                buf_size:  要发送的数据最大长度 */
/*                buf_channel:  发送的数据通道号 */
/*                sync_mode:    发送数据模式， TRUE：确保数据被接收方正确接收  FALSE：数据可能被接收方丢弃 */
/*                tv:    发送超时， NULL：阻塞等待发送完成,  0：默认等待发送时间, 其他值即为设置的等待时间  */
/*  Output Args:  无 */
/*  Return     :  返回成功发送数据长度；错误时返回-1 */
/* ===================================================================================== */
Int32 OSA_pcieSendData(Int32 to_id, char *buf_ptr, Int32 buf_siz, Int32 buf_channel, Int32 sync_mode, struct timeval *tv); 


 /* ===  FUNCTION  ====================================================================== */
 /*         Name:  OSA_pcieRecvData */
 /*  Description:  pcie接收数据，仅从机支持该接口 */
 /*  Input Args :  buf_size:   接收的数据最大长度 */
 /*                tv:    接收等待超时， NULL：阻塞等待完成,  0：默认等待时间, 其他值即为设置的等待时间  */
 /* Output Args:   recv_buf:   数据接收存放指针 */
 /*                buf_channel:  接收的数据的通道号 */
 /*  Return     :  返回成功发送数据长度；错误时返回-1 */
 /* ===================================================================================== */
Int32 OSA_pcieRecvData(char *recv_buf, Int32 buf_size, Int32 *buf_channel, struct timeval *tv);


 /* ===  FUNCTION  ====================================================================== */
 /*         Name:  OSA_pcieRecvCmd */
 /*  Description:  pcie命令通道接收接口，支持主从机间通信，不支持从机间通信 */
 /*  Input Args :  tv:    接收等待超时， NULL：阻塞等待完成,  0：默认等待时间, 其他值即为设置的等待时间 */
 /*  Output Args:  buf:   命令接收存放指针 */
 /*                from_id:  命令的发送端id */
 /*  Return     :  返回接收命令的长度；错误时返回-1 */
 /* ===================================================================================== */
Int32 OSA_pcieRecvCmd(char *buf,Int32 *from_id, struct timeval *tv);


 /* ===  FUNCTION  ====================================================================== */
 /*         Name:  OSA_pcieSendCmd */
 /*  Description:  pcie命令通道接收接口，支持主从机之间收发命令，不支持从机间通信  */
 /*  Input Args :  to_id: 发送的目标ID */
 /*                buf: 要发送的命令字buf地址 */
 /*                buf_size: 要发送的命令字长度 */
 /*                tv:    接收等待超时， NULL：阻塞等待完成,  0：默认等待时间, 其他值即为设置的等待时间 */
 /*  Output Args:  无 */
 /*  Return     :  成功返回0；错误时返回-1 */
 /* ===================================================================================== */    
Int32 OSA_pcieSendCmd(Int32 to_id, char *buf, UInt32 buf_size, struct timeval *tv);


 /* ===  FUNCTION  ====================================================================== */
 /*         Name:  OSA_pcieDeInit */
 /*  Description:  pcie通信退出函数接口*/
 /*  Input Args :  无 */
 /*  Output Args:  无 */
 /*  Return     :  成功返回0；错误时返回-1 */
 /* ===================================================================================== */    
Int32 OSA_pcieDeInit(void);

#ifdef	__cplusplus
}
#endif

#endif	/* OSA_PCIE_H */
