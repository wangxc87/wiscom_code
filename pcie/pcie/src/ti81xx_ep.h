
#ifndef __TI81XX_EP_H__
#define __TI81XX_EP_H__
#include <sys/time.h>
#include "pcie_std.h"
#include "pcie_common.h"

#define MB		(1024 * 1024)
/* #define PCIE_NON_PREFETCH_START	0x20000000 */
/* #define PCIE_NON_PREFETCH_SIZE	(64* MB) */
#define SIZE_AREA		(8 * MB)


Int32 pcie_slave_init(struct pciedev_init_config *config);

char  *pcie_slave_reqDatabuf(UInt32 buf_size);

//timeout:0 表示系统默认等待时间3s，单位ms
Int32 pcie_slave_recvData(char *buf, UInt32 buf_size, Int32 *buf_channel, struct timeval *tv);

//Int32 pcie_slave_recvDataAck(void);

Int32 pcie_slave_getCurTime(UInt32 *cur_jiffies);

Int32 pcie_slave_deInit(void);

Int32 pcie_slave_sendCmd(char *buf, Int32 to_id, UInt32 buf_size, struct timeval *tv);

Int32 pcie_slave_recvCmd(char *buf, Int32 *from_id, struct timeval *tv);

#endif

