#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/ioctl.h>

struct phydev_param {
    char phy_addr[16];// format "0:01"
    int regaddr;
    int value;
};
#define PHYDEV_IOCTL_MAGIC 'M'
#define PHYDEV_ID_SET     (_IOW(PHYDEV_IOCTL_MAGIC,0x13,struct phydev_param))
#define PHYDEV_ID_GET     (_IOR(PHYDEV_IOCTL_MAGIC,0x14,struct phydev_param))
#define PHYDEV_REG_WRITE  (_IOW(PHYDEV_IOCTL_MAGIC,0x15,struct phydev_param))
#define PHYDEV_REG_READ   (_IOR(PHYDEV_IOCTL_MAGIC,0x16,struct phydev_param))

#define TRUE 1
#define FALSE 0

int phy_TestShowUsage(char *str)
{
    printf(" \n");
    printf(" PHY Test Utility \r\n");
    printf(" Usage: %s -r|-w  <phy_addr>  <regAddrInHex> <regValueInHex read not valid>  <numRegsToReadInDec> \r\n", str);
    printf(" Example: %s -r 1/2  0x0 1 \n",str);
    printf(" \n");
    return 0;
}

static char xtod(char c) {
    if (c>='0' && c<='9') return c-'0';
    if (c>='A' && c<='F') return c-'A'+10;
    if (c>='a' && c<='f') return c-'a'+10;
    return c=0;        // not Hex digit
}
  
static int HextoDec(char *hex, int l)
{
    if (*hex==0) 
        return(l);

    return HextoDec(hex+1, l*16+xtod(*hex)); // hex+1?
}
  
int xstrtoi(char *hex)      // hex string to integer
{
    return HextoDec(hex,0);
}

int main(int argc, char **argv)
{
    unsigned char devAddr, numRegs;
    int doRead;
    int status, i;
    int fd,ret;
    int index;
    struct phydev_param param,param_id;

    if(argc < 4) {
        phy_TestShowUsage(argv[0]);
        return -1;
    }

    if(strcmp(argv[1], "-r")==0)
        doRead=TRUE;
    else
        if(strcmp(argv[1], "-w")==0)
            doRead=FALSE;
        else {
            phy_TestShowUsage(argv[0]);
            return -1;
        }

    devAddr = 0;
    numRegs = 1;
    if(argc > 2)
        index = atoi(argv[2]);
    
    if( argc > 3)
        param.regaddr = xstrtoi(argv[3]);//reg addr

    if(argc> 4 )
        param.value = xstrtoi(argv[4]);//write value

    if(argc> 5 ) {
        if(doRead)
        {
            numRegs = atoi(argv[5]);
        }
    }
    switch ( index ){
    case 1 :
        memcpy((char *)param_id.phy_addr, "0:01", 16);
        break;
    case 2:
        memcpy((char *)param_id.phy_addr, "0:02", 16);
        break;
    default:
        phy_TestShowUsage(argv[0]);
        return 0;
    }
  
    char *dev_name = "/dev/phydev";
    fd = open(dev_name,O_RDWR);
    if(fd < 0){
        printf("Open %s failed ..\n",dev_name);
        return -1;
    }
    printf("Open %s Ok..\n",dev_name);
  
    ///    char *phy_addr = "0:01";
  
    ret = ioctl(fd, PHYDEV_ID_SET, &param_id);
    if(ret < 0){
        printf("ioctl:PHYDEV_ID_SET failed..\n");
        return 0;
    }
    memset(&param_id, 0, 16);
    ret = ioctl(fd, PHYDEV_ID_GET, &param_id);
    if(ret < 0)
        printf("ioctl:PHYDEV_ID_GET failed..\n");
    else
        printf("ioctl:Get phyId is %s.\n",param_id.phy_addr);
  
    if(doRead) {
        for(i=0; i<numRegs; i++){
            ret = ioctl(fd,PHYDEV_REG_READ,&param);
            if(ret < 0)
                printf("ioctl:read failed..\n");
            printf("read REG 0x%x-0x%x.\n",param.regaddr,param.value);
            param.regaddr ++;

        }
    } 
    else {
        ret = ioctl(fd,PHYDEV_REG_WRITE,&param);
        if(ret < 0)
            printf("ioctl:write failed..\n");
    }

    close(fd);
    return 0;
}
