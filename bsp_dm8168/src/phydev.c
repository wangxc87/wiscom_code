#include "bsp.h"
//#include "bsp_phydec.h"

struct phydev_param {
    char phy_addr[16];// format "0:01"
    int  reg_addr;
    int  reg_value;
};
#define PHYDEV_IOCTL_MAGIC 'M'
#define PHYDEV_ID_SET     (_IOW(PHYDEV_IOCTL_MAGIC,0x13,struct phydev_param))
#define PHYDEV_ID_GET     (_IOR(PHYDEV_IOCTL_MAGIC,0x14,struct phydev_param))
#define PHYDEV_REG_WRITE  (_IOW(PHYDEV_IOCTL_MAGIC,0x15,struct phydev_param))
#define PHYDEV_REG_READ   (_IOR(PHYDEV_IOCTL_MAGIC,0x16,struct phydev_param))

#define PHYDEV_NAME "/dev/phydev"

//eth0 phy need to config 
#define DEFINE_PHYDEV_ADDR  1

static Int32 gPhydev_fd = -1;
static Int32 gPhydev_addr;
static char  *phy_addr_table[2] = {"0:01","0:02" };
UInt32 gCurNetPortConfig = 0;

Int32 BSP_phydevInit(void)
{
    gPhydev_fd = open( PHYDEV_NAME, O_RDONLY);
    if(gPhydev_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Open %s Error.\n",PHYDEV_NAME);
        return BSP_ERR_DEV_OPEN;
    }
    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"%s Ok.\n",__func__);
    return BSP_OK;
}

Int32 BSP_phydevDeInit(void)
{
    Int32 ret = BSP_OK;
    ret = close( gPhydev_fd );
    if( ret < 0 ){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"close %s Error.\n",PHYDEV_NAME);
        return BSP_ERR_DEV_CLOSE;
    }
    gPhydev_fd = -1;
    return BSP_OK;
}

Int32 BSP_phydevRegWrite(Int32 phy_addr, Int32 reg_addr, Int32 reg_value)
{
    Int32 ret = BSP_OK;
    struct phydev_param param;
    if(gPhydev_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"phydev must init firstly.\n");
        return BSP_ERR_DEV_OPEN;
    }
    if(phy_addr > 2){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"phy_addr must be 1 or 2.\n");
        return BSP_ERR_ARG;
    }
        
    if(gPhydev_addr != phy_addr){
        memcpy(param.phy_addr, *(phy_addr_table + phy_addr - 1),16);
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"set phy_addr is %s.\n",param.phy_addr);
        ret = ioctl(gPhydev_fd, PHYDEV_ID_SET, &param);
        if(ret < 0){
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"PHYDEV_ID_SET Error.\n");
            return BSP_ERR_DEV_CTRL;
        }
        gPhydev_addr = phy_addr;
    }

    param.reg_addr = reg_addr;
    param.reg_value = reg_value;
    ret = ioctl(gPhydev_fd, PHYDEV_REG_WRITE, &param);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"PHYDEV_REG_WRITE cmd Error.\n ");
        return BSP_ERR_DEV_CTRL;
    }
    return BSP_OK;
}

Int32 BSP_phydevRegRead(Int32 phy_addr, Int32 reg_addr, Int32 *reg_value)
{
    Int32 ret = BSP_OK;
    struct phydev_param param;
    if(gPhydev_fd < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"phydev must init firstly.\n");
        return BSP_ERR_DEV_OPEN;
    }
    if(phy_addr > 2){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"phy_addr must be 1 or 2.\n");
        return BSP_ERR_ARG;
    }
        
    if(gPhydev_addr != phy_addr){
        memcpy( param.phy_addr, *(phy_addr_table + phy_addr - 1),16);
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"set phy_addr is %s.\n",param.phy_addr);

        ret = ioctl(gPhydev_fd, PHYDEV_ID_SET, &param);
        if(ret < 0){
            BSP_Print(BSP_PRINT_LEVEL_ERROR,"PHYDEV_ID_SET Error.\n");
            return BSP_ERR_DEV_CTRL;
        }
        gPhydev_addr = phy_addr;
    }

    param.reg_addr = reg_addr;
    ret = ioctl(gPhydev_fd, PHYDEV_REG_READ, &param);
    if(ret < 0){
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"PHYDEV_REG_READ cmd Error.\n ");
        return BSP_ERR_DEV_CTRL;
    }
    *reg_value = param.reg_value;
    return BSP_OK;
}

#define CONFIG_COPPER_PORT (0x800f)
#define CONFIG_FIBER_PORT  (0x8007)
#define CONFIG_AUTO_SELECT (0x7fff)

#define DEFINE_AUTO_SELECT  (0xf0)
#define DEFINE_COPPER_PORT  (0xf1)
#define DEFINE_FIBER_PORT   (0xf2)
Int32 BSP_netPortConfig(Int32 port)
{
    Int32 ret = BSP_OK;
    Int32 reg_addr,reg_value;
    Int32 tmp;

    if(gPCB_VersionID != PCB_VERSION_DECD_ID){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"Current Board do not support this function,exit.\n");
        return BSP_OK;
    }

    BSP_Print(BSP_PRINT_LEVEL_DEBUG,"Current port is 0x%x, set port is 0x%x.\n",gCurNetPortConfig, port);

    //if configured, return
    if(gCurNetPortConfig == port){
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"Current port already set.\n");
        return BSP_OK;
    }

    ret = BSP_phydevInit();
    if(ret < 0)
        return BSP_ERR_DEV_OPEN;
    else
        BSP_Print(BSP_PRINT_LEVEL_COMMON,"phydev init Ok.\n");

    reg_addr = 0x1b;

    ret = BSP_phydevRegRead(DEFINE_PHYDEV_ADDR,reg_addr, &reg_value);
    if(ret < 0)
        return ret;

    switch (port){
    case DEFINE_AUTO_SELECT :
        tmp = reg_value & CONFIG_AUTO_SELECT;
        break;
    case DEFINE_FIBER_PORT :
        tmp = (reg_value & 0xff70) | CONFIG_FIBER_PORT;
        break;
    case DEFINE_COPPER_PORT :
        tmp = (reg_value & 0xff70) | CONFIG_COPPER_PORT;
        break;
    defalut:
        BSP_Print(BSP_PRINT_LEVEL_ERROR,"Invalid config port.\n");
        break;
    }

    ret = BSP_phydevRegWrite(DEFINE_PHYDEV_ADDR,reg_addr,tmp);
    if(ret < 0)
        return ret;

    //phy soft reset
    reg_addr = 0x0;
    ret = BSP_phydevRegRead(DEFINE_PHYDEV_ADDR,reg_addr, &reg_value);
    if(ret < 0)
        return ret;
    ret = BSP_phydevRegWrite(DEFINE_PHYDEV_ADDR,reg_addr,reg_value | 0x8000);
    if(ret < 0)
        return ret;

    ret = BSP_phydevDeInit();
    if(ret < 0)
        return ret;
    else
        BSP_Print(BSP_PRINT_LEVEL_DEBUG,"phydev DeInit Ok.\n");

    gCurNetPortConfig = port;

    return BSP_OK;    
}

