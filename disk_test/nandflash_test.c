#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/statfs.h>  //for statfs
#include <sys/vfs.h>   //for statfs
#include <sys/types.h>
#include <sys/stat.h>  //for stat
#include <sys/time.h>
#include "errno.h"

#define TEST_FILE       "./testfile"

int main(int argc, char *argv[])
{
    FILE *pfTestFile = NULL;
	unsigned int u32WriteData[2] = {0xaaaaaaaa, 0x55555555};
	unsigned int u32ReadData = 0;
//	unsigned int u32DataLen = sizeof(u32WriteData[0]);
	unsigned int u32FileSize;
    int s32Ret;
	size_t u32Len;
	struct statfs nandflash_stat;
	unsigned int u32FlashRemainSpace;
	unsigned int i;
	unsigned int u32TestCnt = 0;

	s32Ret = statfs("/", &nandflash_stat);	// 获取nandflash的信息
	if(s32Ret < 0)
	{
		printf("get nandflash info error!\n");
		return -1;
	}

	u32FlashRemainSpace = nandflash_stat.f_bsize*nandflash_stat.f_bfree/1024;	// 检测磁盘空间
	printf("nand flash remain space is %dKB\n", u32FlashRemainSpace);

	u32FileSize = (u32FlashRemainSpace - 2)*1024;	// 留2K
	printf("test file size is %dKB\n", u32FileSize/1024);

	while (1)
	{
		printf("*****************nand flash check no.%d start*****************\n", u32TestCnt);
		printf("write file to nand flash...\n");
	    pfTestFile = fopen(TEST_FILE, "wb");
	    if (NULL == pfTestFile)
	    {
	        printf("open %s fail!\n", TEST_FILE);
	        return -1;
	    }

		for (i=0; i<u32FileSize; i+=sizeof(u32WriteData[u32TestCnt%2]))
		{
			if (0 == i%(1024*1024))
			{
				printf("write data at %dMB\n", i/(1024*1024));
			}

			u32Len = fwrite(&u32WriteData[u32TestCnt%2], 1, sizeof(u32WriteData[u32TestCnt%2]), pfTestFile);
			if (u32Len != sizeof(u32WriteData[u32TestCnt%2]))
			{
				printf("write test file fail!\n");
				return -1;
			}
		}

		fclose(pfTestFile);
		sleep(5);	// delay 5s

		printf("check test file...\n");
	    pfTestFile = fopen(TEST_FILE, "rb");
	    if (NULL == pfTestFile)
	    {
	        printf("open %s fail!\n", TEST_FILE);
	        return -1;
	    }

		for (i=0; i<u32FileSize; i+=sizeof(u32ReadData))
		{
			if (0 == i%(1024*1024))
			{
				printf("check data at %dMB\n", i/(1024*1024));
			}

			u32Len = fread(&u32ReadData, 1, sizeof(u32ReadData), pfTestFile);
			if (u32Len != sizeof(u32ReadData))
			{
				printf("read test file fail!\n");
				return -1;
			}

			if (u32ReadData != u32WriteData[u32TestCnt%2])
			{
				printf("check test data err at %d,data:0x%x\n", i, u32ReadData);
				return -1;
			}
		}

		fclose(pfTestFile);
		sleep(5);	// delay 5s

		printf("*****************nand flash check no.%d finish*****************\n", u32TestCnt++);
	}

    return 0;
}
