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
#include <getopt.h>
#include "errno.h"

#define TEST_FILE       "/testfile"
#define FILE_MAX_SIZE   (1<<29)

void usage(char *argv)
{
    printf("Info:\n");
    printf("\t   i: define if test forever [default:not]\n");
    printf("\t   l: write/read total size,[default whole free space],unit 512Mb]\n");
    printf("Example:%s /media/sda1 -l 1.\n",argv);
    
}

void system_record(char *string)
{
    char err_record[512];
    strcpy(err_record,"echo ");
    //    sprintf(err_record,"%s:write test size [%llu] ",argv[1], i);
    strcat(err_record,string);
    strcat(err_record," >> /home/root/error_record.txt");
    printf("cmd string %s.\n",err_record);
    system(err_record);
}

int write_file(unsigned long long file_size, int u32WriteData, char *testFile)
{
    char string_tmp[128];
    FILE *pfTestFile;
    unsigned long long i;
    unsigned int u32Len;

    pfTestFile = fopen(testFile, "wb");
    if (NULL == pfTestFile)
    {
        printf("open %s fail!\n", testFile);
        return -1;
    }

    //write data
    for (i=0; i< file_size; i+=sizeof(u32WriteData))
    {
        if (0 == i%(1024*1024*16))
        {
            printf("%s:write data at %lluMB\n",testFile, i/(1024*1024));
        }

        u32Len = fwrite(&u32WriteData, 1, sizeof(u32WriteData), pfTestFile);
        if (u32Len != sizeof(u32WriteData))
        {
            printf("%s:write test file fail!\n",testFile);

            memset(string_tmp, 0, sizeof(string_tmp));
            sprintf(string_tmp, "%s:write test fail fail, size %lluKb.",testFile, i/1024);
            system_record(string_tmp);
            return -1;
        }
    }
    fclose(pfTestFile);
    return 0;
}

int check_data(unsigned long long file_size, int u32WriteData, char *testFile)
{
	unsigned int u32ReadData = 0;
    char string_tmp[128];
    FILE *pfTestFile;
    unsigned long long i;
    unsigned int u32Len;

    pfTestFile = fopen(testFile, "rb");
    if (NULL == pfTestFile)
    {
        printf("open %s fail!\n", testFile);
        return -1;
    }

    for (i=0; i< file_size; i+=sizeof(u32ReadData))
    {
        if (0 == i%(1024*1024*16))
        {
            printf("%s:check data at %lluMB\n", testFile, i/(1024*1024));
        }

        u32Len = fread(&u32ReadData, 1, sizeof(u32ReadData), pfTestFile);
        if (u32Len != sizeof(u32ReadData))
        {
            printf("read test file fail!\n");
            memset(string_tmp, 0, sizeof(string_tmp));
            sprintf(string_tmp, "%s:check test %s fail, size %lluKb.",testFile, i/1024);
            system_record(string_tmp);

            return -1;
        }

        if (u32ReadData != u32WriteData)
        {
            printf("%s:check test data err at %llu,data:0x%x\n", testFile,i, u32ReadData);

            memset(string_tmp, 0, sizeof(string_tmp));
            sprintf(string_tmp, "%s:check test data err at %llu,data:0x%x", testFile,i, u32ReadData);
            system_record(string_tmp);

            return -1;
        }
    }

    fclose(pfTestFile);
    return 0;
}
int main(int argc, char *argv[])
{
	unsigned int u32WriteData[2] = {0xaaaaaaaa, 0x55555555};
	unsigned long long u32FileSize;
    int s32Ret;
	unsigned long long u32FlashRemainSpace;
	unsigned long long i,j;
	unsigned long long u32TestCnt = 0;
    struct statfs diskInfo;
    char rm_cmd[32];
    char string_tmp[128];
    char testFile[128];
    unsigned int single_file_size = FILE_MAX_SIZE;
    unsigned int break_flag = 1;
    int opt,loop= 0;
    char *optstring = "l:ih";
    char device_name[64];
    
    if(argc < 2){
        usage(argv[0]);
        printf("Arg Error.\n");
        return -1;
    }

    strcpy(device_name,argv[1]);
    while (1){
        opt = getopt(argc, argv, optstring);
        if(opt == -1)
            break;
        switch (opt){
        case 'l':
            loop = atoi(optarg);
         break;
        case 'i':
            break_flag = 0;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        defalut:
            usage(argv[0]);
            break;
        }
    }
    printf("\nDisk %s Test ..\n",device_name);

    strcpy(testFile, device_name);
    strcat(testFile, TEST_FILE);

    sprintf(rm_cmd,"rm %s*",testFile);

	s32Ret = statfs(device_name, &diskInfo);	// »ñÈ¡nandflashµÄÐÅÏ¢
	if(s32Ret < 0)
	{
		printf("Get %s info error!\n",device_name);
		return -1;
	}

    unsigned long long blocksize = diskInfo.f_bsize;    //每个block里包含的字节数  
    unsigned long long totalsize = blocksize * diskInfo.f_blocks;   //总的字节数，f_blocks为block的数目  
    unsigned long long freeDisk = diskInfo.f_bfree * blocksize; //剩余空间的大小  
    unsigned long long availableDisk = diskInfo.f_bavail * blocksize;   //可用空间大小  

    printf("Block_size = %d KB = %dB.\n",blocksize,blocksize);
    printf("Total_size = %llu B = %llu KB = %llu MB = %llu GB\n",   
        totalsize, totalsize>>10, totalsize>>20, totalsize>>30);  
    printf("Disk_free =%llu B = %llu KB %llu MB = %llu GB\nDisk_available = %llu B = %llu KB = %llu MB = %llu GB\n",   
           freeDisk,freeDisk>>10,freeDisk>>20, freeDisk>>30, availableDisk,availableDisk>>10,availableDisk>>20, availableDisk>>30);

    memset(string_tmp, 0, sizeof(string_tmp));
    sprintf(string_tmp, "%s:test file, size %llu.",device_name, freeDisk>>10);
    system_record(string_tmp);

	u32FlashRemainSpace = freeDisk/1024;//nandflash_stat.f_bsize*nandflash_stat.f_bfree/1024;	// ¼ì²â´ÅÅÌ¿Õ¼ä
	
    unsigned int file_num,u32LastFileSize;
	file_num = ((u32FlashRemainSpace - 2)*1024)/single_file_size;
    u32LastFileSize = (u32FlashRemainSpace - 2)*1024%single_file_size;

    if((loop != 0) && (file_num != 0))
        file_num = loop;
    
    printf("test file \"%s\" size is %uMB-%uMB,number is %u.\n",
        testFile,single_file_size/(1024*1024),u32LastFileSize/(1024*1024),file_num);

    
	while (1)
	{
		printf("\n*****************Disk %s test no.%llu start*****************\n", device_name, u32TestCnt);

        for(j = 0 ;j < file_num; j++){
            sprintf(string_tmp,"%s%d",testFile,j);
            printf("\nwrite file %s to disk...\n",string_tmp);
            if(write_file(single_file_size,u32WriteData[u32TestCnt % 2],string_tmp) != 0){
                printf("%s [%u]:write data Error.\n",string_tmp, u32TestCnt);
                return -1;
            }
            printf("\ncheck file %s data...\n",string_tmp);
            if(check_data(single_file_size, u32WriteData[u32TestCnt % 2],string_tmp) != 0){
                printf("%s [%u]:check data Error.\n",string_tmp, u32TestCnt);
                return -1;
            }
        }

        //write last file
        sprintf(string_tmp,"%s%d",testFile,j);
        printf("\nwrite last file %s to disk...\n",string_tmp);
        if(write_file(u32LastFileSize,u32WriteData[u32TestCnt % 2],string_tmp) != 0){
            printf("%s [%u]:write last data Error.\n",string_tmp, u32TestCnt);
            return -1;
        }

        printf("\ncheck last file %s data...\n",string_tmp);
        if(check_data(u32LastFileSize, u32WriteData[u32TestCnt % 2],string_tmp) != 0){
            printf("%s [%u]:check last data Error.\n",string_tmp, u32TestCnt);
            return -1;
        }
        
		sleep(1);	// delay 5s
        system(rm_cmd);
        if(break_flag == 1)
            break;
#if 0
		printf("\n%s:check test file...\n",device_name);

        for(j = 0 ;j < file_num; j++){
            sprintf(string_tmp,"%s%d",testFile,j);
            if(check_data(single_file_size, u32WriteData[u32TestCnt % 2],string_tmp) != 0){
                printf("%s [%u]:check data Error.\n",string_tmp, u32TestCnt);
                return -1;
            }
        }

        //read last file
        sprintf(string_tmp,"%s%d",testFile,j);
        if(check_data(u32LastFileSize,u32WriteData[u32TestCnt % 2],string_tmp) != 0){
            printf("%s [%u]:check data Error.\n",string_tmp, u32TestCnt);
            return -1;
        }
        
        sleep(5);	// delay 5s

		printf("*****************Disk %s check no.%llu finish*****************\n",device_name ,u32TestCnt++);
#else
        u32TestCnt ++;
#endif
    }
    printf("******disk Test OK.*****\n");
    return 0;
}
