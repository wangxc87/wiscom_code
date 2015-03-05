// =====================================================================================
//            Copyright (c) 2014, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  read_mac.c
//
//    Description:
//
//        Version:  1.0
//        Created:  2014-09-10
//
//         Author:  jianhuifu
//
// =====================================================================================


#include	<stdlib.h>
#include <bsp.h>

// ===  FUNCTION  ======================================================================
//         Name:  main
//  Description:
// =====================================================================================
int main ( int argc, char *argv[] )
{
    unsigned int    u32Num = 0;
    char            s8Cmd[128];
    char            s8MACAddr[BSP_MAC_LENGTH+1];
    int             s32Ret;
    char            s8MAC[BSP_MAX_MAC*BSP_MAC_LENGTH];

    s32Ret = BSP_Init();
    if ( BSP_OK != s32Ret )
    {
        printf("bsp init error 0x%08x\n", s32Ret);
        return -1;
    }

    s32Ret = BSP_GetEthMAC(s8MAC, &u32Num);
    if ( BSP_OK == s32Ret && u32Num == BSP_MAX_MAC )
    {
        memset((void*)&s8MACAddr, 0, sizeof(s8MACAddr));

        memcpy((void*)&s8MACAddr, (void*)&s8MAC[0], BSP_MAC_LENGTH);
        sprintf((void*)&s8Cmd, "ifconfig eth0 hw ether %s", &s8MACAddr);
        system(s8Cmd);
        printf("%s\n", s8Cmd);

        memcpy((void*)&s8MACAddr, (void*)&s8MAC[1*BSP_MAC_LENGTH], BSP_MAC_LENGTH);
        sprintf((void*)&s8Cmd, "ifconfig eth1 hw ether %s", &s8MACAddr);
        system(s8Cmd);
        printf("%s\n", s8Cmd);

        printf("set MAC address MAC number: %d\n", u32Num);
    }
    else
    {
        printf("read MAC address error 0x%08x\n", s32Ret);
    }

    return EXIT_SUCCESS;
}
// ----------  end of function main  ----------
// =====================================================================================
//    End of read_mac.c
// =====================================================================================

