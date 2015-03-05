//此测试需要在DVRRDK程序起来后执行
#include <stdio.h>
//#include <string.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <unistd.h>
//#include <errno.h>
#include "bsp.h"


int main(int argc,char **argv)
{
    Int32 ret;
    int debug_level ;
    Int32 done = 500;
    fprintf(stdout,"INFO:rd_tvp5158_status 4 enable debug mode.\n");
    fprintf(stdout,"\t*****this test should run after DVRRDK*****");
    if(argc < 2)
        debug_level = BSP_PRINT_LEVEL_COMMON; // level 3
    else
        debug_level = atoi(argv[1]);

    BSP_PrintSetLevel(debug_level);
    BSP_Init();

    while(done){
        ret = BSP_cpldShowVideoInputStatus();
        if(ret != BSP_OK){
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"GET video input status Error..\n");
            break;
        }
        usleep(500000);
    }
    BSP_Uninit();
    return 0;
}


