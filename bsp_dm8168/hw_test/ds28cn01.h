#ifndef __DS28CN01_H__
#define __DS28CN01_H__

#include "ds28cn01_def.h"


#define ROM_ID_LEN      8   // 8 byte
#define SECRET_LEN      8   // 8 byte
#define PAGE_WRITE_LEN  8   // 8 byte
#define MAC_LEN         20  // 20 byte
#define CHALLENGE_LEN   7   // 7 byte
#define PAGE_DATA_LEN   32  // 32 byte
#define LICENCE_LEN     (PAGE_DATA_LEN*2)

typedef enum
{
    MODE_ROMID = 0, // unique registration number
    MODE_ANONY,  // anonymously

    MODE_MAX
}COMPUTE_MODE;


ds_s32 GetHardID(ds_u8 * const pID);
ds_s32 GetLicence(ds_u8 * const pLicence);
ds_s32 ReadMemPage(const ds_u8 u8MemAddr, ds_u8 * const pu8DataBuf, const ds_u8 u8DataLen);
ds_s32 WriteMemPageBy8Byte(const ds_u8 u8WriteAddr, const ds_u8 * const pu8WriteData, const DS_BOOL bUseUniSec);
ds_s8 ReadPageMAC(const ds_u8 u8PageNo, const COMPUTE_MODE enMode, const ds_u8 * const pu8Challenge,
                         ds_u8 * const pu8Mac, const ds_u8 u8MacLen);
ds_s32 LoadFirstSecret(const DS_BOOL bUseUniSec);
ds_s32 Ds28cn01Init(void);
ds_s32 Ds28cn01Exit(void);
ds_s32 testprintf(void);

#endif
