#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "bsp.h"


#define EEPROM_DEVICE       "/sys/devices/platform/omap/omap_i2c.2/i2c-2/2-0053/eeprom"


typedef struct
{
    unsigned int    u32Ver;         // version of storage format
    unsigned int    u32CRC;         // checksum
    unsigned int    u32Sign;        // signature

    unsigned int    u32MACNum;      // number of mac
    char            s8MAC[BSP_MAX_MAC*BSP_MAC_LENGTH]; // mac
}EEPROM_INFO;


// ===  FUNCTION  ======================================================================
//         Name:  EEPROM_CalcCRC
//  Description:  calculate CRC of information
//  Input Args :  EEPROM_INFO
//  Output Args:
//  Return     :  CRC
// =====================================================================================
static unsigned int EEPROM_CalcCRC ( EEPROM_INFO * const pstInfo )
{
    unsigned char   *pu8Data = (unsigned char*)&(pstInfo->u32Sign);
    unsigned int    u32CRC = 0;

    u32CRC += *pu8Data;

    return u32CRC;
}
// -----  end of function EEPROM_CalcCRC  -----


// ===  FUNCTION  ======================================================================
//         Name:  EEPROM_Read
//  Description:  read information header from eeprom
//  Input Args :
//  Output Args:
//  Return     :  BSP_OK on success, otherwise error occurs
// =====================================================================================
static int EEPROM_Read ( EEPROM_INFO * const pstInfo )
{
    int     s32Fd;

    s32Fd = open(EEPROM_DEVICE, O_RDONLY);
    if ( -1 == s32Fd )
        return BSP_ERR_DEV_OPEN;

    memset((void*)pstInfo, 0, sizeof(EEPROM_INFO));

    if ( sizeof(EEPROM_INFO) != read(s32Fd, (void*)pstInfo, (size_t)sizeof(EEPROM_INFO)) )
    {
        close(s32Fd);
        return BSP_ERR_READ;
    }

    close(s32Fd);

    if ( BSP_EEPROM_VER != pstInfo->u32Ver
        || BSP_EEPROM_SIGN != pstInfo->u32Sign )
        return BSP_ERR_SIGN;

    if ( EEPROM_CalcCRC(pstInfo) != pstInfo->u32CRC )
        return BSP_ERR_CHECK;

    return BSP_OK;
}
// -----  end of function EEPROM_ReadHeader  -----


// ===  FUNCTION  ======================================================================
//         Name:  EEPROM_Write
//  Description:  check EEPROM_INFO and write to EEPROM
//  Input Args :
//  Output Args:
//  Return     :  BSP_OK on Success, otherwise error occurs
// =====================================================================================
static int EEPROM_Write ( EEPROM_INFO * const pstInfo )
{
    int     s32Fd;

    s32Fd = open(EEPROM_DEVICE, O_WRONLY);
    if ( -1 == s32Fd )
        return BSP_ERR_DEV_OPEN;

    pstInfo->u32Ver = BSP_EEPROM_VER;
    pstInfo->u32Sign = BSP_EEPROM_SIGN;

    pstInfo->u32CRC = EEPROM_CalcCRC(pstInfo);

    if ( sizeof(EEPROM_INFO) != write(s32Fd, (void*)pstInfo, (size_t)sizeof(EEPROM_INFO)) )
    {
        close(s32Fd);
        return BSP_ERR_WRITE;
    }

    close(s32Fd);
    return BSP_OK;
}
// -----  end of function EEPROM_Write  -----

// ===  FUNCTION  ======================================================================
//         Name:  BSP_GetEthMAC
//  Description:  get MAC address from eeprom
//  Input Args :
//  Output Args:  ps8MAC: buff to save MAC address ,
//                        its length must be larget than BSP_MAC_LENGTH * BSP_MAX_MAC
//                        every MAC address is saved as a character string
//                pu32MACNum: number of MAC address
//  Return     :  BSP_OK on success, otherwise error occurs
// =====================================================================================
int BSP_GetEthMAC ( char * const ps8MAC, unsigned int * const pu32MACNum )
{
    EEPROM_INFO     stInfo;
    int             s32Ret;

    memset((void*)&stInfo, 0, sizeof(stInfo));

    s32Ret = EEPROM_Read(&stInfo);
    if ( BSP_OK != s32Ret )
        return s32Ret;

    if ( BSP_MAX_MAC < stInfo.u32MACNum )
        return BSP_ERR_ARG;

    *pu32MACNum = stInfo.u32MACNum;

    memset((void*)ps8MAC, 0, BSP_MAC_LENGTH);

    memcpy((void*)ps8MAC, (void*)&stInfo.s8MAC, stInfo.u32MACNum * BSP_MAC_LENGTH);

    return BSP_OK;
}
// -----  end of function BSP_GetEthMAC  -----


// ===  FUNCTION  ======================================================================
//         Name:  BSP_SetEthMAC
//  Description:  set all MAC address
//  Input Args :  ps8MAC: all eth MAC address in character string format
//                u32MACNum: number of MAC address
//  Output Args:
//  Return     :  BSP_OK on Success, otherwise error occurs
// =====================================================================================
int BSP_SetEthMAC ( char * const ps8MAC, const unsigned int u32MACNum )
{
    EEPROM_INFO     stInfo;
    int             s32Ret;

    if ( u32MACNum > BSP_MAX_MAC )
        return BSP_ERR_ARG;

    memset((void*)&stInfo, 0, sizeof(stInfo));

    memcpy((void*)&stInfo.s8MAC, (void*)ps8MAC, u32MACNum*BSP_MAC_LENGTH);
    stInfo.u32MACNum = u32MACNum;

    return EEPROM_Write(&stInfo);
}
// -----  end of function BSP_SetEthMAC  -----

