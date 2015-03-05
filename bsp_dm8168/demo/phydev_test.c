#include "bsp.h"

Int32 usage(char *str)
{
    fprintf(stdout,"usage:\n");
    fprintf(stdout,"%s [-p print_level ] [-d 1/2][-m 0/1/2]\n",str);
    fprintf(stdout,"   -p : bsp_printf level 0/1/2/3/4,default BSP_PRINT_LEVEL_COMMON [3].\n");
    fprintf(stdout,"   -d :select PHY device addr [defualt 1].\n");
    fprintf(stdout,"   -m :select fiber/copper port.\n");
    fprintf(stdout,"       0 fiber/copper port auto-select [defalut]\n");
    fprintf(stdout,"       1 select fiber port.\n");
    fprintf(stdout,"       2 select copper port.\n");
    fprintf(stdout,"       3 copper/fiber select according to HardWare Configure.\n");
    fprintf(stdout,"   -h : help info\n");
    return 0;
}

Int32 main(int argc, char **argv)
{
    Int32 ret;
    Int32 debug_level = 3 ;
    Int32 mode = 0,config_port;
    Int32 phy_addr = 1;

    char *optstring = "p:d:m:h";
    int opt;
    UInt32 net_hwconfig;
    if(argc < 2){
        usage(argv[0]);
    }

    while (1){
        opt = getopt(argc, argv, optstring);
        if(opt == -1)
            break;
        switch (opt){
        case 'p':
            debug_level = atoi(optarg);
            break;
        case 'm':
            mode = atoi(optarg);
            break;
        case 'd':
            phy_addr = atoi(optarg);
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            fprintf(stdout,"Invalid arg,exit.\n");
            usage(argv[0]);
            return 0;
        }
    }
    BSP_PrintSetLevel(debug_level);
    BSP_Init();

    if(phy_addr > 2){
        fprintf(stdout,"Invalid phy_addr,exit.\n");
        usage(argv[0]);
    }
    switch (mode ) {
    case 0:
        config_port = DEFINE_AUTO_SELECT;
        fprintf(stdout,"\n***fiber/copper auto-select mode***.\n");
        break;
    case 1:
        config_port = DEFINE_FIBER_PORT;
        fprintf(stdout,"\n***select fiber   mode***.\n");
        break;
    case 2:
        config_port = DEFINE_COPPER_PORT;
        fprintf(stdout,"\n***select copper  mode***.\n");
        break;
    case 3:
        if(gPCB_VersionID != PCB_VERSION_DECD_ID){
            fprintf(stdout,"Current Board do not support this function,exit.\n");
                goto exit1;
        }

        ret = BSP_cpldReadNetConfig(&net_hwconfig);
        if(ret < 0){
            fprintf(stdout,"Get net HW config Error.\n");
            goto exit1;
        }
        if(net_hwconfig == 0){
            config_port = DEFINE_COPPER_PORT;
            fprintf(stdout,"\n***HW select copper  mode***.\n");
        }else if(net_hwconfig == 1){
            config_port = DEFINE_FIBER_PORT;
            fprintf(stdout,"\n***HW select fiber   mode***.\n");
        }else{
            fprintf(stdout, "HW net config Invalid.\n");
                goto exit1;            
        }
        break;
    defualt :
        fprintf(stdout,"Invalid mode,exit.\n");
        usage(argv[0]);
        goto exit1;
    }
    if(mode == 3){
        while(mode --){
            ret = BSP_netPortConfig(config_port);
            if(ret < 0)
                fprintf(stdout,"BSP_netPortConfig Error.\n");
            fprintf(stdout,"\n");
            sleep(1);
        }
    }else
        BSP_netPortConfig(config_port);

 exit1:
    BSP_Uninit();
    fprintf(stdout,"test exit.\n");
    return 0;
}



