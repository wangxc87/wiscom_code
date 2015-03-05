#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "ds28cn01.h"

#define I2C_ADAPTER         "/dev/i2c-2"

#define DS28CN01_ADDR       0x55



#define ROM_ID_ADDR     0xA0
#define LFS             0xA0                //Load first secret
#define CNS(page)       ((0xB<<4) | page)   // Compute next secret
#define CFS(page)       ((0xC<<4) | page)   // Computer final secret
#define CPM(page)       ((0xD<<4) | page)   // Computer page MAC
#define ACPM(page)      ((0xE<<4) | page)   // Anonymous Computer page MAC
#define COMM_ADDR		0xA8                //Communication mode register address
#define CMD_ADDR		0xA9                //Command register address
#define MAC_ADDR        0xB0                //MAC Register address



#define CSHA_TIME           3000  // 3ms
#define PROG_TIME           45000   // 45ms


static pthread_mutex_t s_ds28cn01_mutex = PTHREAD_MUTEX_INITIALIZER;

static ds_u8 CRC8;

//define master secret for page 3 as extension secret
static const ds_u8 ExtensionSecret[32] = {0x11,0x24,0x4d,0x2a,0x43,0x29,0x4d,0x2a,
                                         0x22,0x25,0x62,0x21,0x43,0x39,0x4e,0x2b,
                                         0x33,0x26,0x4d,0x2a,0x43,0x9,0x4d,0x5e,
                                         0x44,0x27,0x5d,0x20,0x3,0x29,0xd,0x43};

//define basic 64-bit secret for DE28CN01
static const ds_u8 DeviceSecret[SECRET_LEN] = {0x12,0x29,0xfd,0x23,0x43,0x22,0x44,0x52};
static const ds_u8 dscrc_table[] = {
        0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
       35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
       70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
       17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
       50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
       87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

static ds_u8 SHAVM_Message[64];		//SHA input buffer

// Hash buffer, in wrong order for Dallas MAC
static ds_u32 SHAVM_Hash[5];

// MAC buffer, in right order for Dallas MAC
static ds_u8 SHAVM_MAC[20];			//MAC Buffer

// Temp vars for SHA calculation
static ds_u32 SHAVM_MTword[80];
static ds_s32 SHAVM_Temp;
static ds_s16 SHAVM_cnt;
static ds_s32 SHAVM_KTN[4];


//----------------------------------------------------------------------
// computes a SHA given the 64 byte MT digest buffer.  The resulting 5
// long values are stored in the long array, hash.
//
// 'SHAVM_Message' - buffer containing the message digest
// 'SHAVM_Hash'    - result buffer
// 'SHAVM_MAC'     - result buffer, in order for Dallas part
//
static void SHAVM_Compute(void)
{
   SHAVM_KTN[0]=(ds_s32)0x5a827999;
   SHAVM_KTN[1]=(ds_s32)0x6ed9eba1;
   SHAVM_KTN[2]=(ds_s32)0x8f1bbcdc;
   SHAVM_KTN[3]=(ds_s32)0xca62c1d6;
   for(SHAVM_cnt=0; SHAVM_cnt<16; SHAVM_cnt++)
   {
      SHAVM_MTword[SHAVM_cnt]
         = (((ds_s32)SHAVM_Message[SHAVM_cnt*4]&0x00FF) << 24L)
         | (((ds_s32)SHAVM_Message[SHAVM_cnt*4+1]&0x00FF) << 16L)
         | (((ds_s32)SHAVM_Message[SHAVM_cnt*4+2]&0x00FF) << 8L)
         |  ((ds_s32)SHAVM_Message[SHAVM_cnt*4+3]&0x00FF);
   }

   for(; SHAVM_cnt<80; SHAVM_cnt++)
   {
      SHAVM_Temp
         = SHAVM_MTword[SHAVM_cnt-3]  ^ SHAVM_MTword[SHAVM_cnt-8]
         ^ SHAVM_MTword[SHAVM_cnt-14] ^ SHAVM_MTword[SHAVM_cnt-16];
      SHAVM_MTword[SHAVM_cnt]
         = ((SHAVM_Temp << 1) & 0xFFFFFFFE)
         | ((SHAVM_Temp >> 31) & 0x00000001);
   }

   SHAVM_Hash[0] = 0x67452301;
   SHAVM_Hash[1] = 0xEFCDAB89;
   SHAVM_Hash[2] = 0x98BADCFE;
   SHAVM_Hash[3] = 0x10325476;
   SHAVM_Hash[4] = 0xC3D2E1F0;

   for(SHAVM_cnt=0; SHAVM_cnt<80; SHAVM_cnt++)
   {
      SHAVM_Temp
         = ((SHAVM_Hash[0] << 5) & 0xFFFFFFE0)
         | ((SHAVM_Hash[0] >> 27) & 0x0000001F);
      if(SHAVM_cnt<20)
         SHAVM_Temp += ((SHAVM_Hash[1]&SHAVM_Hash[2])|((~SHAVM_Hash[1])&SHAVM_Hash[3]));
      else if(SHAVM_cnt<40)
         SHAVM_Temp += (SHAVM_Hash[1]^SHAVM_Hash[2]^SHAVM_Hash[3]);
      else if(SHAVM_cnt<60)
         SHAVM_Temp += ((SHAVM_Hash[1]&SHAVM_Hash[2])
                       |(SHAVM_Hash[1]&SHAVM_Hash[3])
                       |(SHAVM_Hash[2]&SHAVM_Hash[3]));
      else
         SHAVM_Temp += (SHAVM_Hash[1]^SHAVM_Hash[2]^SHAVM_Hash[3]);
      SHAVM_Temp += SHAVM_Hash[4] + SHAVM_KTN[SHAVM_cnt/20]
                  + SHAVM_MTword[SHAVM_cnt];
      SHAVM_Hash[4] = SHAVM_Hash[3];
      SHAVM_Hash[3] = SHAVM_Hash[2];
      SHAVM_Hash[2]
         = ((SHAVM_Hash[1] << 30) & 0xC0000000)
         | ((SHAVM_Hash[1] >> 2) & 0x3FFFFFFF);
      SHAVM_Hash[1] = SHAVM_Hash[0];
      SHAVM_Hash[0] = SHAVM_Temp;
   }

   //iButtons use LSB first, so we have to turn
   //the result around a little bit.  Instead of
   //result A-B-C-D-E, our result is E-D-C-B-A,
   //where each letter represents four bytes of
   //the result.
   for (SHAVM_cnt=0; SHAVM_cnt<5; SHAVM_cnt++) {
		SHAVM_Temp = SHAVM_Hash[4-SHAVM_cnt];
      	SHAVM_MAC[((SHAVM_cnt)*4)+0] = (ds_u8)SHAVM_Temp;
      	SHAVM_Temp >>= 8;
      	SHAVM_MAC[((SHAVM_cnt)*4)+1] = (ds_u8)SHAVM_Temp;
      	SHAVM_Temp >>= 8;
      	SHAVM_MAC[((SHAVM_cnt)*4)+2] = (ds_u8)SHAVM_Temp;
      	SHAVM_Temp >>= 8;
      	SHAVM_MAC[((SHAVM_cnt)*4)+3] = (ds_u8)SHAVM_Temp;
   }

}

/*=========================================================================
  Name:    initUpdate
  Function:
  Input:
  Output:
  Return:
  Others:
  =========================================================================*/
/*=========================================================================
  函数名称: Ds28cn01Read
  函数功能: DS28CN01 I2C 读操作
  输入参数: u8Addr  读地址
            u8Len   读数据长度
  输出参数: pu8Data 读数据缓存
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
static ds_s32 Ds28cn01Read(const ds_u8 u8Addr, ds_u8 * const pu8Data, const ds_u8 u8Len)
{
#define READ_MSG_CNT    2
    ds_s32 s32Fd;
    ds_s32 s32Ret = 0;
    struct i2c_rdwr_ioctl_data stI2CData;
    struct i2c_msg stMsgBuf[READ_MSG_CNT];

    if (NULL == pu8Data)
    {
        printf("ds28cn01 read buf is NULL!\n");
        return -1;
    }

    s32Fd = open(I2C_ADAPTER, O_RDWR);
    if (s32Fd < 0)
    {
        printf("open %s fail\n", I2C_ADAPTER);
        return -1;
    }

    pthread_mutex_lock(&s_ds28cn01_mutex);

    ioctl(s32Fd, I2C_TIMEOUT, 2);   // set timeout
    ioctl(s32Fd, I2C_RETRIES, 1);   // set retry cnt

    memset(&stI2CData, 0, sizeof(stI2CData));
    stI2CData.nmsgs = READ_MSG_CNT;    // read msg number, i.e., start signal nubmer
    stI2CData.msgs = stMsgBuf;

    memset(stMsgBuf, 0, sizeof(stMsgBuf));

    // msg: read addr
    stMsgBuf[0].len = 1;
    stMsgBuf[0].addr = DS28CN01_ADDR;
    stMsgBuf[0].buf = (ds_u8 *)&u8Addr;

    // msg: read data
    stMsgBuf[1].len = u8Len;
    stMsgBuf[1].flags = I2C_M_RD;
    stMsgBuf[1].addr = DS28CN01_ADDR;
    stMsgBuf[1].buf = pu8Data;

    s32Ret = ioctl(s32Fd, I2C_RDWR, &stI2CData);
    if (s32Ret < 0)
    {
        printf("ds28cn01 i2c sr read fail!\n");
        goto ERROR_EXIT;
    }

ERROR_EXIT:
    pthread_mutex_unlock(&s_ds28cn01_mutex);
    close(s32Fd);

    return s32Ret;
}

/*=========================================================================
  函数名称: Ds28cn01Write
  函数功能: DS28CN01 I2C 写操作
  输入参数: u8Addr  写地址
            pu8Data 写数据缓存
            u8Len   写数据长度
  输出参数:
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
static ds_s32 Ds28cn01Write(const ds_u8 u8Addr, const ds_u8 * const pu8Data, const ds_u8 u8Len)
{
    ds_s32 s32Fd;
    ds_s32 s32Ret = 0;
    struct i2c_rdwr_ioctl_data stI2CData;
    struct i2c_msg stMsgBuf;
    ds_u8 au8Buf[64];

    if (NULL == pu8Data)
    {
        printf("ds28cn01 read buf is NULL!\n");
        return -1;
    }

    if (u8Len >= sizeof(au8Buf))
    {
        printf("write data to long!\n");
        return -1;
    }

    s32Fd = open(I2C_ADAPTER, O_RDWR);
    if (s32Fd < 0)
    {
        printf("open %s fail\n", I2C_ADAPTER);
        return -1;
    }

    pthread_mutex_lock(&s_ds28cn01_mutex);

    ioctl(s32Fd, I2C_TIMEOUT, 2);   // set timeout
    ioctl(s32Fd, I2C_RETRIES, 1);   // set retry cnt

    memset(&stI2CData, 0, sizeof(stI2CData));
    stI2CData.nmsgs = 1;    // msg number, i.e., start signal nubmer
    stI2CData.msgs = &stMsgBuf;

    // send msg: write data
    memset(&stMsgBuf, 0, sizeof(stMsgBuf));
    memset(au8Buf, 0, sizeof(au8Buf));
    au8Buf[0] = u8Addr;  // write addr
    memcpy(&au8Buf[1], pu8Data, u8Len);  // write data
    stMsgBuf.len = 1 + u8Len;
    stMsgBuf.addr = DS28CN01_ADDR;
    stMsgBuf.buf = au8Buf;

    s32Ret = ioctl(s32Fd, I2C_RDWR, &stI2CData);
    if (s32Ret < 0)
    {
        printf("ds28cn01 i2c send write data fail!\n");
        goto ERROR_EXIT;
    }

    usleep(1000);  // 1ms

ERROR_EXIT:
    pthread_mutex_unlock(&s_ds28cn01_mutex);
    close(s32Fd);

    return s32Ret;
}

static void dowcrc(const ds_u8 x)
{
    CRC8 = dscrc_table[CRC8 ^ x];
}

/*=========================================================================
  函数名称: CheckCRC
  函数功能: CRC校验
  输入参数: pu8Data 校验数据缓存
            u8Len   校验数据长度
  输出参数:
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
static ds_s32 CheckCRC(const ds_u8 * const pu8Data, const ds_u8 u8Len)
{
    ds_u8 i;

    CRC8 = 0;

    for(i=0; i<u8Len; i++)
    {
        dowcrc(pu8Data[i]); //check if reading ROM ID is right by CRC8 result
    }

    if( CRC8 != 0)
    {
        return -1;
    }

    return 0;
}

/*=========================================================================
  函数名称: ReadRomID
  函数功能: 读DS28CN01的64位序列号
  输入参数:
  输出参数: pu8ID   64位序列号，空间不得小于ROM_ID_LEN
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
static ds_s32 ReadRomID(ds_u8 * const pu8ID)
{
    ds_s32 s32Ret;

    if (NULL == pu8ID)
    {
        printf("Read rom ID buf is NULL!\n");
        return -1;
    }

    s32Ret = Ds28cn01Read(ROM_ID_ADDR, pu8ID, ROM_ID_LEN);
    if (s32Ret < 0)
    {
        printf("Read Rom ID err!\n");
        return s32Ret;
    }

    s32Ret = CheckCRC(pu8ID, ROM_ID_LEN);
    if (s32Ret < 0)
    {
        printf("Rom ID CRC8 check err!\n");
        memset(pu8ID, 0, ROM_ID_LEN);
        return s32Ret;
    }

    return 0;
}

/*=========================================================================
  函数名称: ComputeBasicSecret
  函数功能: 通过device secret计算basic secret
  输入参数: bUseUniSec  计算密钥标志，DS_TRUE 计算生成密钥， DS_FALSE 使用DeviceSecret做密钥
  输出参数: DeviceBasicSecret   计算出的basic secret，空间不得小于SECRET_LEN
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
static ds_s32 ComputeBasicSecret(ds_u8 * const DeviceBasicSecret, const DS_BOOL bUseUniSec)
{
    ds_u8 au8RomID[ROM_ID_LEN];
    ds_s32 s32Ret;

    if (bUseUniSec)
    {
        s32Ret = ReadRomID(au8RomID);
        if (s32Ret < 0)
        {
            return -1;
        }

        //calculate unique 64-bit secret in the DS28E01-100
    	memcpy(SHAVM_Message, DeviceSecret, 4); //Copy the basicsecret to input buffer.
        memset(&SHAVM_Message[4], 0x00, 32);    //Fill the page of 32 bytes to input buffer.
    	memset(&SHAVM_Message[36], 0xff, 4);    //Fill 0xff begin from SP+4 to SP+7.
        SHAVM_Message[40] = au8RomID[0] & 0x3f; //Set MPX.
    	memcpy(&SHAVM_Message[41], &au8RomID[1], 7);    //Input ROMID
    	memcpy(&SHAVM_Message[48], &DeviceSecret[4], 4);    //Input another 4 bytes of basic secret to input buffer.
        memset(&SHAVM_Message[52], 0xff, 3);    //Set the input buffer as defined by datasheet.
    	SHAVM_Message[55] = 0x80;   //Set the input buffer as defined by datasheet.
    	memset(&SHAVM_Message[56], 0x00, 6);    //Set the input buffer as defined by datasheet.
    	SHAVM_Message[62] = 0x01;   //Set the input buffer as defined by datasheet.
    	SHAVM_Message[63] = 0xB8;
        SHAVM_Compute();     //unique basic secret now in SHAVM_MAC[]
        memcpy(DeviceBasicSecret, SHAVM_MAC, SECRET_LEN);  //now unique 64-bit basic secret in DeviceBasicSecret
    }
    else
    {
        memcpy(DeviceBasicSecret, DeviceSecret, SECRET_LEN);  //load basic 64-bit secret into DeviceBasicSecret
    }

    return 0;
}

//Compute unique extension secret
//input parameter: 64-bit unique ROM ID
//the result in 32-byte DeviceExtensionSecret
static ds_s32 ComputeExtensionSecret(ds_u8 *DeviceExtensionSecret)
{
    ds_u8 au8RomID[ROM_ID_LEN];
    ds_s32 s32Ret;

// calculate device secret reserved in page 3
    memcpy(&SHAVM_Message[4], ExtensionSecret, 32);
    if(1/* ExtensionSecretOption */)
    {
        s32Ret = ReadRomID(au8RomID);
        if (s32Ret < 0)
        {
            return -1;
        }

        memset(&SHAVM_Message[36], 0xff, 12);
        memset(&SHAVM_Message[52], 0xff, 3);
        memcpy(SHAVM_Message, au8RomID, 4);
        memcpy(&SHAVM_Message[48], &au8RomID[4], 4);
        SHAVM_Message[55] = 0x80;
        memset(&SHAVM_Message[56], 0x00, 6);
        SHAVM_Message[62] = 0x01;
        SHAVM_Message[63] = 0xB8;
        SHAVM_Compute();     //extension secret now in SHAVM_MAC[]
//authenticate DS28E01 based on page 3

        memcpy(&SHAVM_Message[4], SHAVM_MAC, 20);
        memset(&SHAVM_Message[24], 0x00, 12);
    }

    memcpy(DeviceExtensionSecret,&SHAVM_Message[4], 32);

    return 0;
}

/*=========================================================================
  函数名称: ReadMemPage
  函数功能: 读memory page数据
  输入参数: u8MemAddr  读数据地址
            u8DataLen  读数据长度，不超过PAGE_DATA_LEN
  输出参数: pu8DataBuf 读数据缓存
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
ds_s32 ReadMemPage(const ds_u8 u8MemAddr, ds_u8 * const pu8DataBuf, const ds_u8 u8DataLen)
{
    ds_s32 s32Ret;

    if (NULL == pu8DataBuf)
    {
        printf("Read memory page data buf is NULL!\n");
        return -1;
    }

    s32Ret = Ds28cn01Read(u8MemAddr, pu8DataBuf, u8DataLen);
    if (s32Ret < 0)
    {
        printf("Read memory page %d err!\n", u8MemAddr);
        return s32Ret;
    }

    return 0;
}

/*=========================================================================
  函数名称: WriteMemPageBy8Byte
  函数功能: 写memory page数据，一次8byte
  输入参数: u8MemAddr  写数据地址
            pu8DataBuf 写数据缓存，空间不得小于PAGE_WRITE_LEN
            bUseUniSec  计算密钥标志，DS_TRUE 计算生成密钥， DS_FALSE 使用DeviceSecret做密钥
  输出参数:
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
ds_s32 WriteMemPageBy8Byte(const ds_u8 u8MemAddr, const ds_u8 * const pu8DataBuf, const DS_BOOL bUseUniSec)
{
    ds_s32 s32Ret;
    ds_u8 au8Buf[32];
    ds_u8 au8Secret[SECRET_LEN];
    ds_u8 au8RomID[ROM_ID_LEN];
    ds_u8 au8PageData[PAGE_DATA_LEN];
    ds_u8 u8PageNo;

    if (NULL == pu8DataBuf)
    {
        printf("write data buf is NULL!\n");
    }

    if (u8MemAddr%8 != 0)
    {
        printf("write addr must be 8 integer times!\n");
        return -1;
    }


    // read page data
    u8PageNo = u8MemAddr/PAGE_DATA_LEN;
    memset(au8PageData, 0, sizeof(au8PageData));
    s32Ret = ReadMemPage(u8PageNo*PAGE_DATA_LEN, au8PageData, PAGE_DATA_LEN);
    if (s32Ret < 0)
    {
        printf("read page %d data fail!\n", u8PageNo);
        return -1;
    }


    //the Mater compute the SHA-1 algorithm
    //fill the SHA-1 input buffer
    ComputeBasicSecret(au8Secret, bUseUniSec);
    s32Ret = ReadRomID(au8RomID);
    if (s32Ret < 0)
    {
        return s32Ret;
    }

    // send write data
    memset(au8Buf, 0, sizeof(au8Buf));
    memcpy(&au8Buf[0], pu8DataBuf, PAGE_WRITE_LEN);  // write data
    s32Ret = Ds28cn01Write(u8MemAddr, au8Buf, PAGE_WRITE_LEN);
    if (s32Ret < 0)
    {
        printf("Send write data err!\n");
        return -1;
    }

    usleep(CSHA_TIME);  // delay 3ms for DS28CN01 completing SHA-1 algorithm

    if(u8MemAddr < 0x80)
    {
        memcpy(SHAVM_Message, au8Secret, 4);
        memcpy(&SHAVM_Message[4], au8PageData, 28);
        memcpy(&SHAVM_Message[32], au8Buf, 8);
        SHAVM_Message[40] = (u8MemAddr>>5) & 0x03;
        memcpy(&SHAVM_Message[41], au8RomID, 7);
        memcpy(&SHAVM_Message[48], &au8Secret[4], 4);
        memset(&SHAVM_Message[52],0xff, 3);
        SHAVM_Message[55] = 0x80;
        memset(&SHAVM_Message[56], 0x00, 6);
        SHAVM_Message[62] = 0x01;
        SHAVM_Message[63] = 0xB8;
    }
    else if(u8MemAddr >= 0x88)
    {
        memcpy(SHAVM_Message, au8Secret, 4);
        memset(&SHAVM_Message[4], 0xff, 8);
        memcpy(&SHAVM_Message[12], au8PageData, 8);
        memcpy(&SHAVM_Message[20], au8RomID, 8);
        memset(&SHAVM_Message[28], 0xff, 4);
        memcpy(&SHAVM_Message[32], au8Buf, 8);
        SHAVM_Message[40]=0x04;
        memcpy(&SHAVM_Message[41], au8RomID, 7);
        memcpy(&SHAVM_Message[48], &au8Secret[4], 4);
        memset(&SHAVM_Message[52], 0xff, 3);
        SHAVM_Message[55] = 0x80;
        memset(&SHAVM_Message[56], 0x00, 6);
        SHAVM_Message[62] = 0x01;
        SHAVM_Message[63] = 0xB8;
    }

    SHAVM_Compute();  //MAC generated in SHAVM_MAC[]

    //sending the calculating MAC to DS28CN01
    memset(au8Buf, 0, sizeof(au8Buf));
    memcpy(au8Buf, SHAVM_MAC, MAC_LEN);

    s32Ret = Ds28cn01Write(MAC_ADDR, au8Buf, MAC_LEN);
    if (s32Ret < 0)
    {
        printf("Send write data err!\n");
        return -1;
    }

    usleep(PROG_TIME);  //delay 12ms for completing to program EEPROM

    return 0;
}

/*=========================================================================
  函数名称: LoadFirstSecret
  函数功能: 设置密钥
  输入参数: bUseUniSec  计算密钥标志，DS_TRUE 计算生成密钥， DS_FALSE 使用DeviceSecret做密钥
  输出参数:
  返 回 值: 0:成功   -1:失败
  其他说明: 在进行其他操作之前必须先设置密钥
  =========================================================================*/
ds_s32 LoadFirstSecret(const DS_BOOL bUseUniSec)
{
    ds_u8 au8BasicSecret[SECRET_LEN];
    ds_u8 au8DataBuf[16];
	ds_s32 s32Ret;

    memset(au8BasicSecret, 0, sizeof(au8BasicSecret));
    ComputeBasicSecret(au8BasicSecret, bUseUniSec);

    memset(au8DataBuf, 0, sizeof(au8DataBuf));
    au8DataBuf[0] = LFS;
    memcpy(&au8DataBuf[1], au8BasicSecret, SECRET_LEN);

    s32Ret = Ds28cn01Write(CMD_ADDR, au8DataBuf, 1+SECRET_LEN);
    if (s32Ret < 0)
    {
        printf("Set new secret err!\n");
        return -1;
    }

    usleep(2*PROG_TIME);  //delay for copying data to the secret address of DS28CN01

    return 0;
}

/*=========================================================================
  函数名称: ReadPageMAC
  函数功能: 读Page MAC
  输入参数: u8PageNo  页号 0~3
            enMode    计算方式
            pu8Challenge   随机报文
  输出参数: pu8Mac   Page MAC缓冲，空间不得小于MAC_LEN
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
ds_s8 ReadPageMAC(const ds_u8 u8PageNo, const COMPUTE_MODE enMode, const ds_u8 * const pu8Challenge,
                         ds_u8 * const pu8Mac, const ds_u8 u8MacLen)
{
	ds_s32 s32Ret;
    ds_u8 au8Buf[16];

    if (NULL == pu8Challenge)
    {
        printf("Challenge code is NULL!\n");
        return -1;
    }

    if (NULL == pu8Mac)
    {
        printf("Read page mac buf is NULL!\n");
        return -1;
    }

    memset(au8Buf, 0, sizeof(au8Buf));

    // send command and challenge
    if (MODE_ROMID == enMode)
    {
        au8Buf[0] = CPM(u8PageNo);
    }
    else if (MODE_ANONY == enMode)
    {
        au8Buf[0] = ACPM(u8PageNo);
    }
    else
    {
        printf("MAC compute mode is not supoort!\n");
        return -1;
    }

    memcpy(&au8Buf[1], pu8Challenge, CHALLENGE_LEN);

    s32Ret = Ds28cn01Write(CMD_ADDR, au8Buf, 1 + CHALLENGE_LEN);
    if (s32Ret < 0)
    {
        printf("Send write data err!\n");
        return -1;
    }

    usleep(CSHA_TIME);

    // read MAC
    s32Ret = Ds28cn01Read(MAC_ADDR, pu8Mac, u8MacLen);
    if (s32Ret < 0)
    {
        printf("Read page %d MAC fail!\n", u8PageNo);
        return -1;
    }

    return 0;
}


/*=========================================================================
  函数名称: AuthenticateBy64bitSecret
  函数功能: 通过64位密钥进行验证页数据
  输入参数: pu8Challenge  随机报文
            bUseUniSec  计算密钥标志，DS_TRUE 计算生成密钥， DS_FALSE 使用DeviceSecret做密钥
            u8PageNo    页号
            enMode   计算方法
  输出参数:
  返 回 值: 0:匹配   -1:不匹配
  其他说明:
  =========================================================================*/
ds_s32 AuthenticateBy64bitSecret(const ds_u8 * const pu8Challenge, const DS_BOOL bUseUniSec,
                                         const ds_u8 u8PageNo, const COMPUTE_MODE enMode)
{
    ds_s32 i;
    ds_u8 au8RomID[ROM_ID_LEN];
    ds_s32 s32Ret;
    ds_u8 au8PageMAC[MAC_LEN];
    ds_u8 au8PageData[PAGE_DATA_LEN];
    ds_u8 au8Secret[SECRET_LEN];

    if (enMode >= MODE_MAX)
    {
        printf("Authenticate mode is not support!\n");
        return -1;
    }

    // read rom id
    s32Ret = ReadRomID(au8RomID);
    if (s32Ret < 0)
    {
        return s32Ret;
    }

    // identify the 64-bit basic secret in the DS28CN01 and load into basic 64-bit secret
    ComputeBasicSecret(au8Secret, bUseUniSec);

    //read page MAC from the DS28CN01
    memset(au8PageMAC, 0, sizeof(au8PageMAC));
    s32Ret = ReadPageMAC(u8PageNo, enMode, pu8Challenge, au8PageMAC, MAC_LEN);
    if (s32Ret != 0)
    {
//        printf("Read page %d MAC fail!\n", u8PageNo);
        return s32Ret;
    }


    //read the given page data from DS28CN01
    memset(au8PageData, 0, sizeof(au8PageData));
    s32Ret = ReadMemPage(u8PageNo*PAGE_DATA_LEN, au8PageData, PAGE_DATA_LEN);
    if (s32Ret < 0)
    {
        printf("read page addr 0x%x fail!\n", 0);
        return s32Ret;
    }


    //calculate the corresponding MAC by the host, device secret reserved in SHAVM_MAC[]
    memcpy(SHAVM_Message, au8Secret, 4);
    memcpy(&SHAVM_Message[4], au8PageData, 32);
    memcpy(&SHAVM_Message[36], pu8Challenge, 4);
    SHAVM_Message[40] = (u8PageNo & 0x0F) | 0x40; // MPX, page number
    if (MODE_ROMID == enMode)
    {
        memcpy(&SHAVM_Message[41], au8RomID, 7);
    }
    else
    {
        memset(&SHAVM_Message[41], 0xff, 7);  //filled with 0xff and mask 64-bit ROM ID
    }
    memcpy(&SHAVM_Message[48], &au8Secret[4], 4);
    memcpy(&SHAVM_Message[52], &pu8Challenge[4], 3);
    SHAVM_Message[55] = 0x80;
    memset(&SHAVM_Message[56], 0x00, 6);
    SHAVM_Message[62] = 0x01;
    SHAVM_Message[63] = 0xB8;
    SHAVM_Compute();     //MAC generated based on authenticated page now in SHAVM_MAC[]


    //Compare calculated MAC with the MAC from the DS28E01-100
    for(i=0; i<MAC_LEN; i++)
    {
        if( SHAVM_MAC[i] != au8PageMAC[i] )
        {
            break;
        }
    }

    if(MAC_LEN == i)
    {
//        printf("Authenticate by 64bit Secret match MAC!\n");
        return 0;
    }
    else
    {
        printf("Authenticate by 64bit Secret is not match MAC!\n");
        return -1;
    }
}


#if 0
//authenticate DS28CN01 by 32-byte extension secret(achieved by configuring Page 3 as Read-Protection)
//input parameter is random number in Challenge, Unique Secret enbled by UniqueSecret,
//Unique extension Secret enbled by UniqueExtensionSecret
//Output code to indicate the result in No_Device, CRC_Error, UnMatch_MAC and Match_MAC
ds_s32 AuthenticateByExtSecret(ds_u8 *Challenge, ds_u8 UniqueSecret, ds_u8 UniqueExtensionSecret, DS_BOOL bAnony)
{
   ds_s32 i,j1;
   ds_u8 pbuf[40], PageData[32], cnt,flag,DeviceBasicSecret[8];
   ds_u8 AcctPageNum=3;
// read rom id
   if( (ReadRomID())==false )  return No_Device;

// identify the 64-bit basic secret in the DS28CN01 and load into basic 64-bit secret
   if( UniqueBasicSecret==true )
   {
//calculate unique 64-bit secret in the DS28E01-100
  	memset(&SHAVM_Message[4], 0x00, 32);
   	memcpy(SHAVM_Message, DeviceSecret, 4);
   	memset(&SHAVM_Message[36], 0xff, 4);
        SHAVM_Message[40]=OW_RomID[0]&0x3f;
  	memcpy(&SHAVM_Message[41], &OW_RomID[1], 7);
    	memcpy(&SHAVM_Message[48], &DeviceSecret[4], 4);
        memset(&SHAVM_Message[52],0xff, 3);
   	SHAVM_Message[55] = 0x80;
   	memset(&SHAVM_Message[56], 0x00, 6);
   	SHAVM_Message[62] = 0x01;
   	SHAVM_Message[63] = 0xB8;

        SHAVM_Compute();     //unique basic secret now in SHAVM_MAC[]
	memcpy(DeviceBasicSecret, SHAVM_MAC, 8);   //now unique 64-bit basic secret in DeviceBasicSecret
     }
     else
     {
       memcpy(DeviceBasicSecret, DeviceSecret, 8);     //load basic 64-bit secret into DeviceBasicSecret
     }
// calculate device secret reserved in page 3
   memcpy(&SHAVM_Message[4], ExtensionSecret, 32);
   if( UniqueExtensionSecret==true )
   {
        memset(&SHAVM_Message[36], 0xff, 12);
   	memset(&SHAVM_Message[52], 0xff, 3);
   	memcpy(SHAVM_Message, OW_RomID, 4);
   	memcpy(&SHAVM_Message[48], &OW_RomID[4], 4);
   	SHAVM_Message[55] = 0x80;
   	memset(&SHAVM_Message[56], 0x00, 6);
   	SHAVM_Message[62] = 0x01;
   	SHAVM_Message[63] = 0xB8;
        SHAVM_Compute();     //extension secret now in SHAVM_MAC[]
//authenticate DS28E01 based on page 3

   	memcpy(&SHAVM_Message[4], SHAVM_MAC, 20);
   	memset(&SHAVM_Message[24], 0x00, 12);
     }
        memcpy(&SHAVM_Message[36], Challenge, 4);
   	SHAVM_Message[40] = (AcctPageNum & 0x0F)|0x40; // MPX, page number
  	memcpy(&SHAVM_Message[41], OW_RomID, 7);
   	memcpy(&SHAVM_Message[52], &Challenge[4], 3);
   	memcpy(SHAVM_Message, DeviceBasicSecret, 4);
   	memcpy(&SHAVM_Message[48], &DeviceBasicSecret[4], 4);
   	SHAVM_Message[55] = 0x80;
   	memset(&SHAVM_Message[56], 0x00, 6);
   	SHAVM_Message[62] = 0x01;
   	SHAVM_Message[63] = 0xB8;
        SHAVM_Compute();     //MAC generated based on page 3 now in SHAVM_MAC[]


//read MAC from the DS28CN01
/****************************************************************/
//write Challenge to DS28CN01
     I2CStart();
     flag=I2CWrite(I2CSlaveAddress);
     flag|=I2CWrite(CommandAddress);
     flag|=I2CWrite(ComputePageMAC|AcctPageNum);   //select which page to be used in SHA-1 algorithm
     for(i=0;i<8;i++) flag|=I2CWrite(Challenge[i]);
     I2CStop();
     if(flag!=0) return No_Device;  //not ack by the device DS28CN01
     I2CDelay_us(3000);   //delay 3 ms for the DS28CN01 to complete SHA-1 calculation
//begin to read the MAC from DS28CN01
     I2CStart();
     flag=I2CWrite(I2CSlaveAddress);
     flag|=I2CWrite(MAC_Address);
     I2CRepeatStart();
     flag|=I2CWrite(I2CSlaveAddress|0x01);
     for(i=0;i<19;i++) pbuf[i]=I2CRead(0x00);  //reserve DS28CN01's MAC in pbuf
     pbuf[i]=I2CRead(0x01);      //not ack in the last byte of MAC
     I2CStop();
     if( flag!=0 ) return No_Device;

//Compare calculated MAC with the MAC from the DS28E01-100
        for(i=0;i<20;i++){ if( SHAVM_MAC[i]!=pbuf[i] )  break;}
        if( i==20 ){ return Match_MAC;}
        else return UnMatch_MAC;
}

#endif

/*=========================================================================
  函数名称: GetHardID
  函数功能: 获取Hard ID，即序列号
  输入参数:
  输出参数: pID   64位序列号，空间不得小于ROM_ID_LEN
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
ds_s32 GetHardID(ds_u8 * const pID)
{
    ds_s32 s32Ret;

    s32Ret = ReadRomID(pID);
    if (s32Ret < 0)
    {
        return s32Ret;
    }

    return 0;
}

/*=========================================================================
  函数名称: GetLicence
  函数功能: 获取授权码
  输入参数:
  输出参数: pLicence  授权码
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
ds_s32 GetLicence(ds_u8 * const pLicence)
{
    ds_s32 s32Ret;
    ds_u8 au8PageData[PAGE_DATA_LEN];
    ds_u8 u8PageNo;

    printf("in GetLicence!\n");

    u8PageNo = 0;
    memset(au8PageData, 0, sizeof(au8PageData));
    s32Ret = ReadMemPage(u8PageNo*PAGE_DATA_LEN, au8PageData, PAGE_DATA_LEN);
    if (s32Ret < 0)
    {
        printf("GetLicence read page 0x%x fail!\n", u8PageNo);
        return s32Ret;
    }

    s32Ret = AuthenticateBy64bitSecret(au8PageData, DS_TRUE, u8PageNo, MODE_ROMID);
    if (s32Ret < 0)
    {
        return s32Ret;
    }

    memcpy(pLicence, au8PageData, PAGE_DATA_LEN);


    u8PageNo = 1;
    memset(au8PageData, 0, sizeof(au8PageData));
    s32Ret = ReadMemPage(u8PageNo*PAGE_DATA_LEN, au8PageData, PAGE_DATA_LEN);
    if (s32Ret < 0)
    {
        printf("GetLicence read page 0x%x fail!\n", u8PageNo);
        return s32Ret;
    }

    s32Ret = AuthenticateBy64bitSecret(au8PageData, DS_TRUE, u8PageNo, MODE_ROMID);
    if (s32Ret < 0)
    {
        return s32Ret;
    }

    memcpy(pLicence+PAGE_DATA_LEN, au8PageData, PAGE_DATA_LEN);


    return 0;
}

ds_s32 testprintf(void)
{
    ds_u8 au8data[64];
    ds_s32 s32Ret;
    ds_u8 i;

    for (i=0x90; i<=0x95; i++)
    {
        memset(au8data, 0 ,sizeof(au8data));
        s32Ret = Ds28cn01Read(i, au8data, 1);
        if (s32Ret < 0)
        {
            return -1;
        }
        printf("reg 0x%x data = 0x%x\n", i, au8data[0]);
    }


    memset(au8data, 0 ,sizeof(au8data));
    s32Ret = Ds28cn01Read(0x98, au8data, 1);
    if (s32Ret < 0)
    {
        return -1;
    }
    printf("0x98 data = 0x%x\n", au8data[0]);

    memset(au8data, 0 ,sizeof(au8data));
    s32Ret = Ds28cn01Read(0x9B, au8data, 2);
    if (s32Ret < 0)
    {
        return -1;
    }
    printf("Manufacturer ID = 0x%x_0x%x\n", au8data[0], au8data[1]);

    memset(au8data, 0 ,sizeof(au8data));
    s32Ret = Ds28cn01Read(0xa8, au8data, 1);
    if (s32Ret < 0)
    {
        return -1;
    }
    printf("0xa8 data = 0x%x\n", au8data[0]);


    memset(au8data, 0 ,sizeof(au8data));
    s32Ret = Ds28cn01Read(0x94, au8data, 1);
    if (s32Ret < 0)
    {
        return -1;
    }
    printf("0x94 data = 0x%x\n", au8data[0]);


    memset(au8data, 0 ,sizeof(au8data));
    s32Ret = Ds28cn01Read(0x95, au8data, 1);
    if (s32Ret < 0)
    {
        return -1;
    }
    printf("0x95 data = 0x%x\n", au8data[0]);


    printf("write 0x96 0x97!\n");
    memset(au8data, 0 ,sizeof(au8data));
    au8data[0] = 0x12;
    au8data[1] = 0x34;
    s32Ret = Ds28cn01Write(0x96, au8data, 2);
    if (s32Ret < 0)
    {
        return -1;
    }
    usleep(10000);

    memset(au8data, 0 ,sizeof(au8data));
    s32Ret = Ds28cn01Read(0x96, au8data, 2);
    if (s32Ret < 0)
    {
        return -1;
    }
    printf("0x96 0x97 data = 0x%x_0x%x\n", au8data[0], au8data[1]);


    memset(au8data, 0 ,sizeof(au8data));
    s32Ret = Ds28cn01Read(0x80, au8data, 8);
    if (s32Ret < 0)
    {
        return -1;
    }
    printf("0x80 data = 0x%x\n", au8data[0]);
    printf("0x81 data = 0x%x\n", au8data[1]);
    printf("0x82 data = 0x%x\n", au8data[2]);
    printf("0x83 data = 0x%x\n", au8data[3]);
    printf("0x84 data = 0x%x\n", au8data[4]);
    printf("0x85 data = 0x%x\n", au8data[5]);



/*    printf("**********test write MAC!\n");
    sleep(2);
    s32Ret = Ds28cn01Write(0xB0, DeviceSecret, MAC_LEN);
    if (s32Ret < 0)
    {
        printf("Send write data err!\n");
        return -1;
    }*/

    usleep(PROG_TIME);  //delay 12ms for completing to program EEPROM

    return 0;
}

/*=========================================================================
  函数名称: Ds28cn01Init
  函数功能: 初始化Ds28cn01Init
  输入参数:
  输出参数:
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
ds_s32 Ds28cn01Init(void)
{
    ds_s32 s32Ret;
    ds_u8 u8CommData = 0;  // I2C mode

    s32Ret = Ds28cn01Write(COMM_ADDR, &u8CommData, sizeof(u8CommData));
    if (s32Ret < 0)
    {
        return -1;
    }

    return 0;
}

/*=========================================================================
  函数名称: Ds28cn01Init
  函数功能: 反初始化Ds28cn01Init
  输入参数:
  输出参数:
  返 回 值: 0:成功   -1:失败
  其他说明:
  =========================================================================*/
ds_s32 Ds28cn01Exit(void)
{
    return 0;
}

