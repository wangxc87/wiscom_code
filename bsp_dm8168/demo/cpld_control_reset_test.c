#include <stdio.h>
#include "bsp.h"
Int32 usage(void)
{
    fprintf(stdout,"./cpld_control_reset_test -cmd  \n");
    fprintf(stdout,"cmd should be 0 1 2 3 4.\n");
    fprintf(stdout,"\t0: reset whole board.\n");
    fprintf(stdout,"\t1: reset fpga only.\n");
    fprintf(stdout,"\t2: reset net Phy device only.\n");
    fprintf(stdout,"\t3: reset sii9022 only.\n");
    return 0;
}
Int32 main(Int32 argc,Char **argv)
{
    Int32 debug_level = BSP_PRINT_LEVEL_COMMON;
    char *args,char_tmp;
    Int32 reset_cmd = 0;
    if(argc < 2){
        usage();
        return 0;
    }
    else
        args = argv[1];
    char_tmp = *(args + 1);
    switch (char_tmp) {
    case '0':
        reset_cmd = BSP_CPLD_RESET_ALL;
        break;
    case '1':
        reset_cmd = BSP_CPLD_RESET_FPGA;
        fprintf(stdout,"reset fpga.\n");
        break;
    case '2':
        reset_cmd = BSP_CPLD_RESET_NETPHY;
        break;
    case '3':
        reset_cmd = BSP_CPLD_RESET_SII9022;
        break;
    default:
        fprintf(stdout,"Invalid cmd %c.\n",char_tmp);
        usage();
        return 0;
    }
    BSP_PrintSetLevel(debug_level);
    BSP_Init();

    BSP_cpldResetBoard(reset_cmd, 0);

    BSP_Uninit();

    return 0;
}
