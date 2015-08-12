#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
/* #include "i2c_wr.h" */
#define OSA_EFAIL -1
#define OSA_SOK 0

#define I2C_DEVICE_NAME "/dev/i2c-1"
#define ACCESS_MODE_READ   0
#define ACCESS_MODE_WRITE  1
#define PEX8605_READ  0x04 
#define PEX8605_WRITE  0x03
#define PEX8605_SLAVE_DEVADDR  0x58

int usage(char *argv)
{
    printf(" \n");
    printf(" Pex8506 regWR Utility \r\n");
    printf(" Ver %s %s \r\n", __TIME__, __DATE__);
    printf(" Usage: %s -r|-w  <portNum> <regAddrInHex>  <regValueInHex or numRegsToReadInDec> \r\n", argv);
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

int pex8605_write(int fd, int port_num, int reg_addr, int value)
{
    int status = 0;

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data data;

    char buf[16];

    memset((char *)buf, 0, sizeof(buf));
    //cmd
    buf[0] |= PEX8605_WRITE;
    buf[1] = (port_num & 0x07)>>1;
    buf[2] = (port_num & 0x01) << 7;
    buf[2] |= 0x3c;//enable all byte be modified
    buf[2] |= reg_addr >> 10;
    buf[3] =  (reg_addr >> 2) & 0xff;

    //data
    buf[4] = (value >> 24) & 0xff;
    buf[5] = (value >> 16) && 0xff;
    buf[6] = (value >> 8) && 0xff;
    buf[7] = value & 0xff;
    
    msgs[0].addr  = PEX8605_SLAVE_DEVADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 8;
    msgs[0].buf   = (void *)buf;

    data.msgs = msgs;
    data.nmsgs = 1;

    status = ioctl(fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
#ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Raw Write ERROR !!! (count = %d)\n", devAddr, count);
#endif
    }
    else
        status = OSA_SOK;
    return status;
}

int pex8605_read(int fd, int port_num, int reg_addr, int *value)
{
    int status = 0;

    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data data;
    char buf[16];

    memset((char *)buf, 0, sizeof(buf));

    //cmd
    buf[0] |= PEX8605_READ;
    buf[1] = (port_num & 0x07)>>1;
    buf[2] = (port_num & 0x01) << 7;
    buf[2] |= 0x3c;//enable all byte be modified
    buf[2] |= reg_addr >> 10;
    buf[3] =  (reg_addr >> 2) & 0xff;

    
    //write addr
    msgs[0].addr  = PEX8605_SLAVE_DEVADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 4;
    msgs[0].buf   = (void *)buf;

    //read data
    msgs[1].addr = PEX8605_SLAVE_DEVADDR;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = 4;
    msgs[1].buf = (void *)&buf[4];
    data.msgs = msgs;
    data.nmsgs = 2;

    status = ioctl(fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
#ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Raw Write ERROR !!! (count = %d)\n", devAddr, count);
#endif
    }
    else
        status = OSA_SOK;
    
#ifdef OSA_I2C_DEBUG
    printf("i2c read data 0x%x 0x%x 0x%x 0x%x.\n", buf[4], buf[5], buf[6], buf[7]);
#endif

    *value = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
    return status;
}

int main(int argc, char *argv[])
{
    int ret;
    int access_mode = 0xff;
    int port_num, reg_addr, value, count;
    int fd;
    if(argc < 4){
        usage(argv[0]);
        return -1;
    }

    if(!strcmp(argv[1], "-r"))
        access_mode = ACCESS_MODE_READ;
    else if(!strcmp(argv[1], "-w")){
        if(argc >4)        
            access_mode = ACCESS_MODE_WRITE;
        else {
            usage(argv[0]);
            return -1;
            
        }
    } else {
        usage(argv[0]);
        return -1;
    }

    port_num = atoi(argv[2]);
    reg_addr = xstrtoi(argv[3]);
    count = 1;
    reg_addr &= ~0x03; //32bit reg addr assign

    fd = open(I2C_DEVICE_NAME,O_RDWR);
    
    if(fd < 0)
        printf("Open %s fail..\n",I2C_DEVICE_NAME);
    else
        printf ("Open %s OK..\n",I2C_DEVICE_NAME);

    switch(access_mode){
    case ACCESS_MODE_READ:
        if(argc == 5)
            count = xstrtoi(argv[4]);
        count -= 1;
        do {
            ret = pex8605_read(fd, port_num, reg_addr, &value);
            if(ret < 0){
                printf("pex8605 read port-reg: %d-0x%x error.\n", port_num,reg_addr);
                close(fd);
                return -1;
            }
            printf("read %d-0x%03x: 0x%04x %04x\n", port_num, reg_addr, (value>>16)&0xffff, (value & 0xffff));
            reg_addr += 4;
        }while(count --);

        break;
        
    case ACCESS_MODE_WRITE:
        value = xstrtoi(argv[4]);
        ret = pex8605_write(fd, port_num, reg_addr, value);
        if(ret < 0){
            printf("pex8605 write port-reg: %d-0x%x error.\n", port_num, reg_addr);
            close(fd);
            return -1;
        }
        printf("write %d-0x%x to 0x%x Ok.\n", port_num, reg_addr, value);
        break;

    default:
        break;        
    }

    close(fd);
    printf("close.\n");
    return 0;

}
