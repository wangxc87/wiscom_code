
#ifndef __PCIE_COMMON_H__
#define __PCIE_COMMON_H__

#include <stdio.h>
#include "pcie_std.h"

#define PAGE_SIZE_EP	4096

#define EP_ID_O     1
#define EP_ID_1     2
#define EP_ID_2     3
#define RC_ID       4
#define EP_ID_ALL   (0xff)
#define PCIE_INVALID_ID  (0)

#define PCIE_RC_RESERVE_MEM_SIZE ( 4 * 1024 * 1024)

#define TEST_PCIE_MSI

//#define THPT_TEST
 
#define EDMA_TRANSMISSION
 
struct mgmt_info {
    UInt32 ep_id;
    UInt32 wr_index;
    UInt32 rd_index;
    UInt32 ep_outbase;
    UInt32 ep_initted;
};

struct pciedev_databuf_head{
    UInt32 data_to_id;
    UInt32 data_from_id;
    UInt32 frame_id;
    UInt32 wr_index;
    UInt32 rd_index;
    UInt32 buf_size;
};

struct pciedev_info {
    UInt32 id;
    UInt32 rd_index;//read times by EPs
    UInt32 res_value[6][2];//0:baseaddr 1:size
    char *data_buf;//data buf
    char *mgmt_buf;//management buf
};


void *memcpy_neon(void *, const void *, size_t);
#endif
