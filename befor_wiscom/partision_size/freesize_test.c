#include <sys/statvfs.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
        struct statvfs info;
        char path_name[0xff];
        
       if(argc>1){
               strcpy(path_name,"/home/video/");
       }else
               strcpy(path_name,"/home/video/VBITS_HDR_0.bin");
 

       if (-1 == statvfs(path_name, &info))
                perror("statvfs() error");
        else {
                printf("statvfs() returned the following information\n");
                printf("about the ('%s') file system:\n",path_name);
                printf("  f_bsize    : %u(0x%x)\n", info.f_bsize,info.f_bsize);
                printf("  f_blocks   : %u(0x%x)\n", info.f_blocks,info.f_blocks);
                printf("  f_bfree    : %u(0x%x)\n", info.f_bfree,info.f_bfree);
                printf("  f_files    : %u(0x%lx)\n", info.f_files,info.f_files);
                printf("  f_ffree    : %u(0x%lx)\n", info.f_ffree,info.f_ffree);
                printf("  f_fsid     : %u(0x%lx)\n", info.f_fsid,info.f_fsid);
                printf("  f_flag     : 0x%X(0x%lx)\n", info.f_flag,info.f_flag);
                printf("  f_namemax  : %u(0x%lx)\n", info.f_namemax,info.f_namemax);
        }

        return 0;
}
