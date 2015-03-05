
#ifndef __TI81XX_EP_H__
#define __TI81XX_EP_H__

#include "pcie_std.h"

Int32 pcie_slave_init(void);

char  *pcie_slave_reqDatabuf(UInt32 buf_size);

//timeout:0 表示系统默认等待时间3s，单位ms
Int32 pcie_slave_recvData(char *buf, UInt32 buf_size, UInt32 timeout);

Int32 pcie_slave_getCurTime(UInt32 *cur_jiffies);

Int32 pcie_slave_deInit(void);

Int32 pcie_slave_sendCmd(void *arg);

Int32 pcie_slave_waitCmd(void *arg);

#endif
