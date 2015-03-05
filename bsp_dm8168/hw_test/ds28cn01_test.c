#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ds28cn01.h"

#define EEPROM_SIZE     128
//#define TEST_DS28CN01_DEBUG
//int main(int argc, char *argv[])
int test_ds28cn01(void)
{
    ds_s32 s32Ret;
    ds_u8 au8ID[ROM_ID_LEN];
    ds_u8 au8MAC[MAC_LEN];
    ds_u8 au8Challenge[CHALLENGE_LEN] = {1, 2, 3, 4, 5, 6, 7};
    ds_u8 au8RWData[EEPROM_SIZE] = {0};
    ds_u8 u8WriteAddr;
    ds_u8 au8Licence[LICENCE_LEN];
    ds_u8 i;

    s32Ret = Ds28cn01Init();
    if (s32Ret != 0)
    {
        printf("ds28cn01 init fail!\n");
        return s32Ret;
    }

    // config secret
    s32Ret = LoadFirstSecret(DS_TRUE);
    if (s32Ret != 0)
    {
        printf("load secret fail!\n");
        return s32Ret;
    }

//    testprintf();

    // read Hard ID
    memset(au8ID, 0, sizeof(au8ID));
    s32Ret = GetHardID(au8ID);
    if (s32Ret < 0)
    {
        printf("Get Hard ID fail!\n");
    }
#ifdef TEST_DS28CN01_DEBUG
    printf("Hard ID is: ");
    for (i=0; i<sizeof(au8ID); i++)
    {
        printf("%02x ", au8ID[i]);

    }
    printf("\n");
#endif

    // read page MAC
    memset(au8MAC, 0, sizeof(au8MAC));
    s32Ret = ReadPageMAC(0, MODE_ROMID, au8Challenge, au8MAC, MAC_LEN);
    if (s32Ret != 0)
    {
        return s32Ret;
    }

#ifdef TEST_DS28CN01_DEBUG
    printf("Page 0 MAC is:");
    for (i=0; i<MAC_LEN; i++)
    {
        printf("%02x ", au8MAC[i]);
    }
    printf("\n");
#endif


    // test write all data memory page
    // 1.write data
    for (i=0; i<sizeof(au8RWData); i++)
    {
        au8RWData[i] = i;
    }

    for (i=0; i<sizeof(au8RWData); i+=8)
    {
        s32Ret = WriteMemPageBy8Byte(i, au8RWData+i, DS_TRUE);
        if (s32Ret < 0)
        {
            printf("write page addr 0x%x fail!\n", 0);
            return -1;
        }
    }

    // 2.read data
    memset(au8RWData, 0, sizeof(au8RWData));
    for (i=0; i<4; i++)
    {
        s32Ret = ReadMemPage(i*PAGE_DATA_LEN, au8RWData+i*PAGE_DATA_LEN, PAGE_DATA_LEN);
        if (s32Ret < 0)
        {
            printf("read page addr 0x%x fail!\n", 0);
            return -1;
        }
    }
#ifdef TEST_DS28CN01_DEBUG
    printf("\nread page data is:");
    for (i=0; i<sizeof(au8RWData); i++)
    {
        if (0 == i%PAGE_DATA_LEN)
        {
            printf("\n");
        }
        printf("%02x ", au8RWData[i]);
    }
    printf("\n\n");
#endif

    // read licence
    memset(au8Licence, 0, sizeof(au8Licence));
    s32Ret = GetLicence(au8Licence);
    if (s32Ret < 0)
    {
        printf("get licence fail!\n");
        return s32Ret;
    }

#ifdef TEST_DS28CN01_DEBUG
    printf("Licence is:");
    for (i=0; i<sizeof(au8Licence); i++)
    {
        if (0 == i%PAGE_DATA_LEN)
        {
            printf("\n");
        }
        printf("%02x ", au8Licence[i]);
    }
    printf("\n");
#endif
    //    return 0;


    u8WriteAddr = 0x00;
    s32Ret = WriteMemPageBy8Byte(u8WriteAddr, au8RWData, DS_TRUE);
    if (s32Ret < 0)
    {
        printf("write page addr 0x%x fail!\n", 0);
        return -1;
    }

    memset(au8RWData, 0, sizeof(au8RWData));
    s32Ret = ReadMemPage(u8WriteAddr/PAGE_DATA_LEN*PAGE_DATA_LEN, au8RWData, PAGE_DATA_LEN);
    if (s32Ret < 0)
    {
        printf("read page addr 0x%x fail!\n", 0);
        return -1;
    }
#ifdef TEST_DS28CN01_DEBUG
    for (i=0; i<PAGE_DATA_LEN; i++)
    {
        printf("page addr 0x%x data:0x%x\n", u8WriteAddr/PAGE_DATA_LEN*PAGE_DATA_LEN+i, au8RWData[i]);
    }
#endif

    Ds28cn01Exit();

    return 0;
}

