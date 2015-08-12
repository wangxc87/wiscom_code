
#ifndef __TI81XX_RC_LIB_H__
#define __TI81XX_RC_LIB_H__
#include <sys/time.h>

#include "pcie_std.h"
#include "pcie_common.h"

Int32 pcieRc_init(struct pciedev_init_config *config);

char *pcieRc_reqDataBuf(UInt32 buf_size);

//timeout 0:表示默认时间,单位ms
Int32 pcieRc_sendData(char *data_ptr, UInt32 buf_size,UInt32 pciedev_id, Int32 buf_channel, Int32 sync_mode,struct timeval *tv);

Int32 pcieRc_deInit(void);

Int32 pcieRc_recvCmd(char *buf,Int32 *from_id, struct timeval *tv);

Int32 pcieRc_sendCmd(char *buf, Int32 to_id, UInt32 buf_size, struct timeval *tv);

#endif

