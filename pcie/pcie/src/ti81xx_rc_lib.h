
#ifndef __TI81XX_RC_LIB_H__
#define __TI81XX_RC_LIB_H__

#include "pcie_std.h"

Int32 pcieRc_init(int mode);

char *OSA_pciReqDataBuf(UInt32 buf_size);

//timeout 0:表示默认时间,单位ms
Int32 OSA_pcieSendData(char *data_ptr, UInt32 buf_size,UInt32 pciedev_id, UInt32 timeout);

Int32 pcieRc_deInit(void);

Int32 OSA_pcieWaitCmd(void *arg);

Int32 OSA_pcieSendCmd(void *arg);

#endif
