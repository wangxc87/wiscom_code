
#ifndef __PCIE_COMMON_H__
#define __PCIE_COMMON_H__

#include "kbuf_utils.h"

#define FALSE 0
#define TRUE  1

#define PAGE_SIZE_EP	4096

#define EP_ID_O     1
#define EP_ID_1     2
#define EP_ID_2     3
#define RC_ID       4
#define EP_ID_ALL   (0xff)
#define PCIE_INVALID_ID  (0)

#define PCIE_RC_RESERVE_MEM_SIZE ( 4 * 1024 * 1024)
#define CMD_BUF_SIZE (128)
#define CMD_BUF_RESERVE_SIZE (16)

#define RSVMEM_CMD_BUF_SIZE (2*1024*1024)
#define RSVMEM_DATA_BUF_OFFSET (RSVMEM_CMD_BUF_SIZE)

//fifo define
#define  DATA_BUF_SIZE (1024*1024)
#define  DATA_BUF_FIFOS 16
#define  CMD_BUF_SIZE (1*1024)
#define  CMD_BUF_FIFOS 16



/* #define RSVMEM_MGMT_SIZE   (0x200000) */
/* #define RSVMEM_EDMA_RECV_BUF_SIZE (0x300000) */
/* #define RSVMEM_MGMT_OFFSET (0) */
/* #define RSVMEM_EDMA_RECV_BUF_OFFSET (RSVMEM_MGMT_OFFSET + RSVMEM_MGMT_SIZE) */
/* #define RSVMEM_EDMA_SEND_BUF_OFFSET (RSVMEM_EDMA_RECV_BUF_OFFSET + RSVMEM_EDMA_RECV_BUF_SIZE) */


#define PCIEDEV_EBUSY (0xffffff00)
#define PCIEDEV_ETIMEOUT (0xffffff01)

#define SEND_DATA (0x01)
#define SEND_CMD  (0x02)
#define DATA_ACK  (0x04)
#define CMD_ACK   (0x08)
#define DATA_ACK_ERR (0x10 | DATA_ACK)
#define CMD_ACK_ERR  (0x20 | CMD_ACK)
//#define THPT_TEST
 
#define EDMA_TRANSMISSION
 
//#define WAIT_FOREVER_EP
//#define WAIT_FORVER_RC
#define PCIE_PRINT_INTERVAL 50

#define devid_to_index(x)  ((x)-1)
#define INIT_ALL_EPS   0
#define INIT_SOME_EPS  1

#define DEFAULT_INIT_TIMEOUT (150)
#define DEFAULT_SEND_TIMEOUT (5)
#define DEFAULT_RECV_TIMEOUT (5)

struct mgmt_info {
    int cmd; //RC interrupt flag
    u32 ep_id;
    u32 wr_index;
    u32 rd_index;
    u32 ep_outbase;
    u32 ep_initted;
    u32 buf_size;
};
#define PCIE_EP_MAX_NUMBER 3
struct pciedev_databuf_head{
    int ep_rd_flag[PCIE_EP_MAX_NUMBER]; //ep int cmd
    u32 data_to_id;
    u32 data_from_id;
    u32 frame_id;//数据帧ID
    u32 wr_index;
    u32 rd_index;    
    u32 buf_size;
    int  buf_channal;//display buf channal id
 };

//pcie buf struct
struct pciedev_bufObj {
    char *cmd_buf_base;
    struct pcie_bufHndl cmd_bufHndl;
    char *data_buf_base;
    struct pcie_bufHndl data_bufHndl;
};

 //dev info struct
struct pciedev_info {
    u32 dev_id; //hw_id
    u32 rd_index;//read times by EPs
    u32 res_value[6][2];//0:baseaddr 1:size
    wait_queue_head_t data_wq_h;
    wait_queue_head_t cmd_wq_h;
    struct pciedev_bufObj recvBufObj; //received data fifo
#if 0
    struct pcie_bufHndl cmd_bufHndl;
    struct pcie_bufHndl data_bufHndl;
#endif
    char *data_buf_base;//data buf
    char *mgmt_buf_base;//management buf
    struct mgmt_info *cmd_head_ptr;
    char *send_cmd;
    char *recv_cmd;
    struct pciedev_databuf_head *data_head_ptr;
    char *send_data;
    char *recv_data;
};



//ioctrl arguments,
struct pciedev_buf_info {
    char *buf_kptr;
    u32 buf_ptr_offset;
    u32 buf_size;
};

#if 0
extern void *memcpy_neon(void *, const void *, size_t);
extern int get_selfId(void);


inline static int pcieDev_lockInit(struct pciedev_info *pciedev)
{
    sem_init(&pciedev->dev_lock, 0, 1);
    return 0;
}
inline static int pcieDev_lockDeInit(struct pciedev_info *pciedev)
{
    return 0;
}
inline static int pcieDev_lock(struct pciedev_info *pciedev)
{
    sem_wait(&pciedev->dev_lock);
    return 0;
}

inline static int pcieDev_unlock(struct pciedev_info *pciedev)
{
    sem_post(&pciedev->dev_lock);
    return 0;
}
#endif

#endif
