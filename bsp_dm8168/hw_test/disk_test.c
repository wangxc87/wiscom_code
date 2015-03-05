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

#define FORMAT_TOOL "/usr/bin/hw_format_disk"
#define TEST_FILE       "/testfile"
//#define FILE_MAX_SIZE   (1<<30)
#define FILE_MAX_SIZE   (256<<20)

#define TEST_RECORD_FILE "/home/root/disk_test_err.tmp"
void system_record(char *string)
{
    char err_record[512];
    sprintf(err_record,"date >>%s", TEST_RECORD_FILE);
    system(err_record);

    sprintf(err_record,"echo %s >>%s",string, TEST_RECORD_FILE);
    //    printf("cmd string %s.\n",err_record);
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
    for (i=4; i< file_size +4; i+=sizeof(u32WriteData))
    {
        if (0 == i%(1024*1024*64))
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

    for (i=4; i< file_size + 4; i+=sizeof(u32ReadData))
    {
        if (0 == i%(1024*1024*64))
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
#define DISK_NUM_MAX (20)
static char gDisk_moutPath[DISK_NUM_MAX][64];
static char gDisk_devName[DISK_NUM_MAX][64];
static char gDisk_mounIndex[DISK_NUM_MAX];

static int gDisk_num = 0;

#define DISK_DEV_TMP_FILE ("/home/root/disk_info.tmp")

extern void clear_stdin(void);

static int check_diskInfo(void)
{
    int ret = 0;
    FILE *filp;
    char cmd_buf[128];
    char char_buf[64];
    int loop = 1;
    int disk_num = 0;
    char disk_path[DISK_NUM_MAX][64];

    memset(gDisk_mounIndex, DISK_NUM_MAX -1, DISK_NUM_MAX);
    
    sprintf(cmd_buf, "ls /dev/ | grep 'sd' >%s", DISK_DEV_TMP_FILE);
    system(cmd_buf);
    filp = fopen(DISK_DEV_TMP_FILE,"r");
    if(filp == NULL){
        printf("Open file %s Error,%s.\n",DISK_DEV_TMP_FILE, strerror(errno));
        ret = -1;
        goto check_err;
    }

    while(!feof(filp)){
        memset(char_buf, '\0', sizeof(char_buf));
        ret = fscanf(filp, "%s", char_buf);
#ifdef DISK_TEST_DEBUG
        printf("[1]: get %d chars in a line,%s.\n",ret,char_buf);
#endif
        if((ret == 1) && (strlen(char_buf) == 3)){  //sdX
            sprintf(disk_path[disk_num],"/dev/%s",char_buf);
            sprintf(gDisk_devName[disk_num],"/dev/%s",char_buf);
            disk_num ++;
        }
    }
    fclose(filp);
#ifdef DISK_TEST_DEBUG
    printf("There is %d disks.\n",disk_num);
#endif
    gDisk_num = disk_num;
    
    sprintf(cmd_buf, "mount | awk '{print $1\" \" $3}' >%s", DISK_DEV_TMP_FILE);
    system(cmd_buf);
    
    filp = fopen(DISK_DEV_TMP_FILE,"r");
    if(filp == NULL){
        printf("Open file %s Error,%s.\n",DISK_DEV_TMP_FILE, strerror(errno));
        ret = -1;
        goto check_err;
    }

    int i = 0,j= 0,len;
    char char_buf1[64];
    char char_tmp0[64];
    
    while(!feof(filp)){
        memset(char_buf,'\0', sizeof(char_buf));
        memset(char_buf1,0, sizeof(char_buf1));

        //read mount path
        ret = fscanf(filp, "%s %s", char_buf,char_buf1);
#ifdef DISK_TEST_DEBUG
        printf("[2-0]:get %d chars in a line,%s-%s\n",ret,char_buf,char_buf1);
#endif
        for( i = 0; i < disk_num; i ++){
            len = strlen(disk_path[i]);
            if(len == 0)
                continue;

            memset(char_tmp0, '\0', sizeof(char_tmp0));
            strncpy(char_tmp0, char_buf, len);
            if(strcmp(char_tmp0, disk_path[i]) == 0){
#ifdef DISK_TEST_DEBUG
                printf("[2-1]:get %d chars in a line,%s\n",ret,char_buf1);
#endif
                strcpy(gDisk_moutPath[j], char_buf1);
                //                strcpy(gDisk_devName[j], char_buf);
                gDisk_mounIndex[i] = j;//硬盘设备对应的挂载路径后
                *disk_path[i] = '\0';
                j ++;
            }
         }
     }
    fclose(filp);
    sprintf(cmd_buf, "rm %s", DISK_DEV_TMP_FILE);
    system(cmd_buf);     
     return 0;
 check_err:
     return -1;
}

int disk_format(char *disk_dev, char *oMount_path)
{
    int ret;
    char cmd_buf[128];
    char mount_path[64];
    char disk_part[64];
    char char_tmp[64];
    pid_t status;
    if(strlen(disk_dev) < 5){
        fprintf(stderr,"Invalid disk Dev %s.\n", disk_dev);
        return -1;
    }
    strcpy(char_tmp, disk_dev + 5);
    sprintf(mount_path, "/media/%s1", char_tmp);
    sprintf(cmd_buf,"%s.sh %s %s.ini %s",FORMAT_TOOL, disk_dev, FORMAT_TOOL, mount_path);
    status = system(cmd_buf);
     if (-1 == status){
         fprintf(stderr,"%s: system error!",__func__);
        return -1;
    } else {  
         //        printf("exit status value = [0x%x]\n", status);  
         if (WIFEXITED(status)) {  
            if (0 == WEXITSTATUS(status)) {  
                //                printf("run shell script successfully.\n");
                strcpy(oMount_path,mount_path);
                clear_stdin();
                return 0;
            } else {  
                fprintf(stderr,"%s:run shell script fail, script exit code: %d\n",
                        __func__,WEXITSTATUS(status));
                return -1;
            }  
        }  else {  
             fprintf(stderr, "%s:exit status = [%d]\n", __func__, WEXITSTATUS(status));
            return -1;
        }  
    }  
    return 0;
}
static unsigned int gSingle_file_size = FILE_MAX_SIZE;
static __test_disk(char *device_name)
{
	unsigned int u32WriteData[2] = {0xaaaaaaaa, 0x55555555};
	unsigned long long u32FileSize;
    int s32Ret;
	unsigned long long u32FlashRemainSpace;
	int i,j;
	unsigned long long u32TestCnt = 0;
    struct statfs diskInfo;
    char rm_cmd[32];
    char string_tmp[128];
    char testFile[128];
    unsigned int single_file_size = gSingle_file_size;
    unsigned int break_flag = 0;
    int opt;
    //    char device_name[64];
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
    if(u32FlashRemainSpace >= single_file_size/1024) {
        /* 
          file_num = ((u32FlashRemainSpace - 2)*1024)/single_file_size;
        u32LastFileSize = (u32FlashRemainSpace - 2)*1024%single_file_size;
        */
        file_num = 1; //produce test, only test once [512M]
        u32LastFileSize = 0;
    }else {
        file_num = 0;
        u32LastFileSize = (u32FlashRemainSpace - 2)*1024%single_file_size;        
    }
    
    printf("test file \"%s\" size is %uMB-%uMB,number is %u.\n",
        testFile,single_file_size/(1024*1024),u32LastFileSize/(1024*1024),file_num);

    
	//	printf("\n*****************Disk %s test no.%llu start*****************\n", device_name, u32TestCnt);

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

        if(u32LastFileSize > 0){
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
        }
        
		sleep(1);	// delay 5s
        system(rm_cmd); //delet test file
        u32TestCnt ++;

        return 0;    
}
static void disk_usage(void)
{
    int i = 0;
    char disk_menu1[] = {
        "\r\n"
        "\r\n ==================="
        "\r\n   Disk Test menu"
        "\r\n ==================="
        "\r\n [0~%x]: select test_disk num "
    };
    char disk_menu2[] = {
        "\r\n      disk%d: %s - %s"
    };
    char disk_menu3[] = {
        "\r\n   w  : Test all disk once"
        "\r\n     p: Test all disk forever (pressure test)"
        "\r\n   r  : rescan disk info"
        "\r\n   q  : return to prev  menu"
        "\r\n"
        "\r\n Enter choice:"
    };
    printf(disk_menu1, gDisk_num -1);
    for(i = 0; i< gDisk_num; i++){
        printf(disk_menu2, i, gDisk_devName[i], gDisk_moutPath[gDisk_mounIndex[i]]);
    }
    printf(disk_menu3);
    
}

//研发测试
#define TEST_ALL_FOREVER

int test_disk(void)
{
	int i,j;
    int loop = 1;
    char device_name[64];
    int ret;
    
    ret = check_diskInfo();
    if(ret < 0){
        printf("check diskInfo Error.\n");
        return -1;
    }

    if(gDisk_num == 0){
        printf("There is no disk. Please connect Disk.\n");
        return 0;
    }

    printf("\nThere is %d disks.\n",gDisk_num);
    
    int disk_index = 0;
    char ch;
    char format_flag = 'y';
    int  test_all_disk = 0,pressure_test = 0;
    unsigned int test_forever_num = 0;
    char flag = 'y';
    while(1){
        while (loop){
        input_loop:
            ch = getchar();
            //        clear_stdin();
            flag = 'y';
            format_flag = 'y';
            test_all_disk = 0;
            switch (ch){
            case 'a'...'f':
            case '0'...'9':
                if((ch >= '0') && (ch <= '9'))
                    disk_index = atoi(&ch);
                else
                    disk_index = ch -'a' + 10;
                
                if(disk_index < gDisk_num){
                    if(strlen(gDisk_moutPath[gDisk_mounIndex[disk_index]]) == 0){ //no mount path, need format
                        printf("\nSelect Disk%d can Not test, No mount Path.\n", disk_index);
                        clear_stdin();
                        printf("Need format Disk <y/n default:y>:");
                        scanf("%[yn]", &format_flag);
                        if(format_flag == 'y'){
                            fprintf(stdout,"The Disk%d - %s will be Formated.\n", disk_index, gDisk_devName[disk_index]);
                            ret = disk_format(gDisk_devName[disk_index], gDisk_moutPath[gDisk_mounIndex[disk_index]]);
                            if(ret < 0)
                                fprintf(stderr,"***disk_format %s Error***\n",gDisk_devName[disk_index]);
                            else
                                fprintf(stdout,"***disk_format %s OK***\n",gDisk_devName[disk_index]);
                        }else
                            printf("input format_flag is <%c>.\n", format_flag);
                        
                        loop = 1;
                    }else{  //test 
                        strcpy(device_name,gDisk_moutPath[gDisk_mounIndex[disk_index]]);
                        printf("\n****Select disk%d - %s****\n",disk_index, device_name);
                        loop = 0;
                    }
                }else {
                    printf("Invalid Disk num.");
                    disk_usage();
                    goto input_loop;
                }
                break;
            case 'w':
                test_all_disk = 1;
                loop = 0;
                break;
            case 'p':
                fprintf(stdout,"\nif Test all disks[y/n default:y]:");
                clear_stdin();
                scanf("%[yn]", &flag);
                if(flag == 'y')
                    test_all_disk = 1;  //test all disk
                else {  //test one disk forever
                retry_select_disk:
                    fprintf(stdout, "\nSelect disk index to test forever [0~%d]", gDisk_num - 1);
                    scanf("%d", &disk_index);
                    if(disk_index >= gDisk_num)
                        goto retry_select_disk;
                    else{
                        strcpy(device_name,gDisk_moutPath[gDisk_mounIndex[disk_index]]);
                        printf("\n****Select disk%d - %s****\n",disk_index, device_name);
                    }
                }
                    
                loop = 0;
                pressure_test = 1;
                gSingle_file_size = 1<<30; //test single file 1G
                break;
            case 'r':  //rescan disk info
                ret = check_diskInfo();
                if(ret < 0){
                    printf("check diskInfo Error.\n");
                    return -1;
                }
                disk_usage();
                break;
            case 'q':
                return 0;
            default:
                //                printf(disk_menu,gDisk_num -1);
                disk_usage();
                goto input_loop;
            }
        }
        if(test_all_disk == 1){ ///test all disk auto
            if(strlen(gDisk_moutPath[gDisk_mounIndex[disk_index]]) == 0){ //no mount path, need format
                printf("\nSelect Disk%d can Not test, No mount Path.\n", disk_index);
                    fprintf(stdout,"The Disk%d - %s will be Formated.\n", disk_index, gDisk_devName[disk_index]);




                    ret = disk_format(gDisk_devName[disk_index], gDisk_moutPath[gDisk_mounIndex[disk_index]]);
                    if(ret < 0)
                        fprintf(stderr,"***disk_format %s Error***\n",gDisk_devName[disk_index]);
                    else{
                        fprintf(stdout,"***disk_format %s OK***\n",gDisk_devName[disk_index]);
                        strcpy(device_name,gDisk_moutPath[gDisk_mounIndex[disk_index]]);
                    }
                        
            }else{  // direct test 
                strcpy(device_name,gDisk_moutPath[gDisk_mounIndex[disk_index]]);
                printf("\n****Select disk%d - %s****\n",disk_index, device_name);
            }

        }

        fprintf(stdout,"\n************disk%d <%s> Test-%u stating**********\n",
                disk_index, gDisk_devName[disk_index], test_forever_num);
        ret = __test_disk(device_name);
        if(ret < 0){
            fprintf(stderr,"\n**********disk%d <%s> Test-%u Error*********\n",
                    disk_index, gDisk_devName[disk_index], test_forever_num);
            clear_stdin();
            fprintf(stdout,"\n**********Press any key to continue Test*********\n");
            getchar();
        }else
            fprintf(stdout,"\n**********disk%d <%s> Test-%u OK.**********\n",
                    disk_index, gDisk_devName[disk_index], test_forever_num);

        if(test_all_disk == 1){  //test all disk
            disk_index ++;
            if(disk_index >= gDisk_num){
                if(pressure_test == 0){  //test all disk once
                    loop = 1;
                    test_all_disk = 0;
                } else  //test forever
                    test_forever_num ++;                
                disk_index  = 0;
            }
        }else {   //test single disk
            if(pressure_test == 0){  //test once
                loop = 1;
                test_all_disk = 0;
            } else  //test forever
                test_forever_num ++;                
        }
    }
    return 0;
}

