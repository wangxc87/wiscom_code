/**********************************
 *
 * DVR Sensor  Alarm device driver
 *（1）告警输入（通过光耦）
    SENSOR_IN8_GPO_23
    SENSOR_IN10_GPO_25
    SENSOR_IN11_GPO_26
    SENSOR_IN12_GPO_27
    SENSOR_IN13_GPO_28
    SENSOR_IN14_GPO_29
（2）告警输出（通过继电器）
    ALARM_OUT1_GP1_12
    ALARM_OUT2_GP1_13
    ALARM_OUT3_GP1_14
    ALARM_OUT4_GP1_15
 * 2014 年 09 月 15 日
 * xuecaiwang
 *********************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/ioctl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sensor_alarm.h>

//告警输入（通过光耦）
#define    SENSOR_IN8_GPO_23    23
#define    SENSOR_IN10_GPO_25   25
#define    SENSOR_IN11_GPO_26   26
#define    SENSOR_IN12_GPO_27   27
#define    SENSOR_IN13_GPO_28   28
#define    SENSOR_IN14_GPO_29   29
#define    SENSOR_IN_NUM         6

//告警输出（通过继电器）
#define   ALARM_OUT1_GP1_12    12
#define   ALARM_OUT2_GP1_13    13
#define   ALARM_OUT3_GP1_14    14
#define   ALARM_OUT4_GP1_15    15
#define   ALARM_OUT_NUM        4

#define  PINCTRL_MUX_GP0_23  (0x48140b14)
#define  PINCTRL_MUX_GP0_25  (0x48140b1c)
#define  PINCTRL_MUX_GP0_26  (0x48140b20)
#define  PINCTRL_MUX_GP0_27  (0x48140b24)
#define  PINCTRL_MUX_GP0_28  (0x48140b28)
#define  PINCTRL_MUX_GP0_29  (0x48140b2c)

#define  PINCTRL_MUX_GP1_12  (0x48140afc)
#define  PINCTRL_MUX_GP1_13  (0x48140b00)
#define  PINCTRL_MUX_GP1_14  (0x48140b04)
#define  PINCTRL_MUX_GP1_15  (0x48140b08)

#define GPIO_0_BASEADDR      (0x48032000)
#define GPIO_0_MMAPSIZE      (0x00000200)
#define GPIO_1_BASEADDR      (0x4804c000)
#define GPIO_1_MMAPSIZE      (0x00000200)
#define GPIO_OE              (0x134)
#define GPIO_DATAIN          (0x138)
#define GPIO_DATAOUT         (0x13c)

#define OMAP2_L4_IO_OFFSET	0xb2000000
#define IO_ADDR(reg)      ((const volatile void __iomem *)(reg + OMAP2_L4_IO_OFFSET))

#define DEVICE_RUNNING_STATE   1
#define DEVICE_STOPPING_STATE  0
#define SENSOR_ALARM_NAME  "sensor_alarm"
#define IDVR_SENSOR_MAJOR  0
static int sensor_alarm_major = IDVR_SENSOR_MAJOR;


struct sensor_alarm_dev 
{
    char *name;
    dev_t     device_no;
    struct cdev cdev;
    u32  *sensor_reg;
    u32  *alarm_reg;
    u32 sensor_in;
    u32 alarm_out;
	atomic_t      users;
    u32 device_state;
    struct class *classp;
    struct device *devicep;
        
};
static long sensor_alarm_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

struct sensor_alarm_dev *sensor_alarm_devp;

static void reg_writel(u32 value,u32 reg)
{
    __raw_writel(value,IO_ADDR(reg));
}
static u32 reg_readl(u32 reg)
{
    return __raw_readl(IO_ADDR(reg));
}

static int sensor_alarm_open(struct inode *inodep,struct file *filp)
{
        if (atomic_inc_return(&sensor_alarm_devp->users) >= 0x02) {
            atomic_dec(&sensor_alarm_devp->users);
            return -EBUSY;
        }
        sensor_alarm_devp->device_state = DEVICE_RUNNING_STATE;
        return 0;
}

static int sensor_alarm_release(struct inode *inode,struct file *filp)
{
    
    if(atomic_read(&sensor_alarm_devp->users) > 0){
        atomic_dec(&sensor_alarm_devp->users);
    }
    
    sensor_alarm_devp->device_state = DEVICE_STOPPING_STATE;
        return 0;
}

//no Sensor Input gpio high;
static int __sensor_in_get(void)
{
    u32 value,i,j,offset;
    offset = SENSOR_IN8_GPO_23;
    value = *sensor_alarm_devp->sensor_reg;
    j = 0;
    sensor_alarm_devp->sensor_in = 0;
    for(i = 0; i < ( SENSOR_IN_NUM + 1); i++ ){
        if(i == 1)
            continue;
        if((value & (1 << ( offset + i))) == 0)  //signal in ,gpio 0
            sensor_alarm_devp->sensor_in |= 1 << j;
        j ++;
    }
    return sensor_alarm_devp->sensor_in;
}
static int __alarm_out_set(u32 value)
{
    u32 tmp;
    u32 i = 1;
    tmp = *sensor_alarm_devp->alarm_reg;
    for(i = 0; i < 4; i++ ){
        if(value & (1 << i))
            tmp |= 1 << (i + 12);
        else
            tmp &= ~(1 << (i + 12));
    }
    *sensor_alarm_devp->alarm_reg = tmp;
    sensor_alarm_devp->alarm_out = (tmp >> 12) & 0x0f;
    return 0;
}

static long sensor_alarm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        u32 sensor_in;
        u32 alarm_out;
        int ret = 0;

        switch (cmd){
        case SENSOR_IN_GET_CMD:
            sensor_in = __sensor_in_get();
            if (copy_to_user((void __user *) arg, &sensor_in, sizeof(sensor_in))) {
                printk(KERN_ERR "Invalid user space pointer: %p\n", (void *) arg);
                ret = -EINVAL;
            }
            break;

        case ALARM_OUT_SET_CMD:
            if(!copy_from_user(&alarm_out, (void __user *)arg, sizeof(arg))){
                __alarm_out_set(alarm_out);
            }else {
                printk(KERN_ERR "copy_from user Error.\n");
                ret = -EINVAL;
            }
            break;

        case ALARM_OUT_GET_CMD:
            if(copy_to_user((void __user *)arg, &sensor_alarm_devp->alarm_out,sizeof(sensor_alarm_devp->alarm_out))){
                printk(KERN_ERR "Invalid user space pointer: %p\n", (void *) arg);
                ret = -EINVAL;
            }
            break;

        default :
            printk(KERN_ERR "%s Unknown  cmd: %08x\n",__func__, cmd);
            ret = -EINVAL;
            break;
        }
        
        return ret;
}

static struct file_operations sensor_alarm_fops = {
     .owner = THIS_MODULE,
     .open = sensor_alarm_open,
     .release = sensor_alarm_release,
     .unlocked_ioctl = sensor_alarm_ioctl,
};

static int sensor_alarm_setup_cdev(struct sensor_alarm_dev *devp,int index)
{
        int err;
        dev_t devnum = MKDEV(sensor_alarm_major,index);
        struct cdev *cdevp = &devp->cdev;

        cdev_init(cdevp,&sensor_alarm_fops);
        cdevp->owner = THIS_MODULE;
        cdevp->ops = &sensor_alarm_fops;

        err = cdev_add(cdevp,devnum,1);
        if(err){
                printk(KERN_ERR "Error %d adding sensor_alarm %d\n",err,index);
                return -1;
        }
        return 0;
        
}
static int gpio_init(void)
{
    u32 value;
    //disable inter-pull up/down
    reg_writel(0x0a,PINCTRL_MUX_GP0_23);
    reg_writel(0x09,PINCTRL_MUX_GP0_25);
    reg_writel(0x0a,PINCTRL_MUX_GP0_26);
    reg_writel(0x0a,PINCTRL_MUX_GP0_27);
    reg_writel(0x09,PINCTRL_MUX_GP0_28);
    reg_writel(0x09,PINCTRL_MUX_GP0_29);
    reg_writel(0x0a,PINCTRL_MUX_GP1_12);
    reg_writel(0x09,PINCTRL_MUX_GP1_13);
    reg_writel(0x0a,PINCTRL_MUX_GP1_14);
    reg_writel(0x0a,PINCTRL_MUX_GP1_15);

    //set gpio0 input
    value = reg_readl(GPIO_0_BASEADDR + GPIO_OE);
    value |= (1 << SENSOR_IN8_GPO_23) | (1 << SENSOR_IN10_GPO_25) | (1 << SENSOR_IN11_GPO_26) |
        (1 << SENSOR_IN12_GPO_27) | (1 << SENSOR_IN13_GPO_28) | (1 << SENSOR_IN14_GPO_29);
    reg_writel(value, GPIO_0_BASEADDR + GPIO_OE);

    //set gpio1 output
    value = reg_readl(GPIO_1_BASEADDR + GPIO_OE);
    value &= ~(1 << ALARM_OUT1_GP1_12)
        & ~(1 << ALARM_OUT2_GP1_13) & ~(1 << ALARM_OUT3_GP1_14) & ~(1 << ALARM_OUT4_GP1_15);
    reg_writel(value, GPIO_1_BASEADDR + GPIO_OE);

    return 0;   
}

#define SUBDEV_NUMBER 1

int __init  sensor_alarm_init(void)
{
        dev_t devnum = MKDEV(sensor_alarm_major,0);
        int ret;
        
        if(sensor_alarm_major)
                ret = register_chrdev_region(devnum,SUBDEV_NUMBER,SENSOR_ALARM_NAME);
        else {
                ret = alloc_chrdev_region(&devnum,0,SUBDEV_NUMBER,SENSOR_ALARM_NAME);
                sensor_alarm_major = MAJOR(devnum);
        }
        if(ret < 0){
                printk(KERN_ERR "Error:register Devnum fail...\n");
                return ret;
        }
        printk(KERN_DEBUG "sensor_alarm MAJOR NUM is %d\n",sensor_alarm_major);

        
        sensor_alarm_devp = kmalloc(SUBDEV_NUMBER * sizeof(struct sensor_alarm_dev),GFP_KERNEL);
        if(!sensor_alarm_devp){
                ret = -ENOMEM;
                goto fail_malloc;
        }

        memset(sensor_alarm_devp,0,SUBDEV_NUMBER*sizeof(struct sensor_alarm_dev));

        sensor_alarm_devp->device_no = devnum;
        sensor_alarm_devp->name = SENSOR_ALARM_NAME;
        ret = sensor_alarm_setup_cdev(sensor_alarm_devp,0);
        if(ret < 0){
                printk(KERN_ERR"Error:add sensor_alarm1 fail..\n");
                goto fail_malloc;
        }

        if((sensor_alarm_devp->classp = class_create(THIS_MODULE,SENSOR_ALARM_NAME)) == NULL) {
            printk(KERN_ERR "Create sensor_alarm class fail.\n");
            ret = -EINVAL;
            goto fail_malloc;
        }else
            sensor_alarm_devp->devicep = device_create(sensor_alarm_devp->classp, NULL, devnum, NULL, SENSOR_ALARM_NAME);

        if(sensor_alarm_devp->devicep == NULL){
            printk(KERN_ERR "Create sensor_alarm device fail.\n");
            class_destroy(sensor_alarm_devp->classp);
            ret = -EINVAL;
            goto quit1;
        }
            
        sensor_alarm_devp->sensor_in = 0;
        sensor_alarm_devp->alarm_out = 0;

        sensor_alarm_devp->sensor_reg = ioremap(GPIO_0_BASEADDR+GPIO_DATAIN, 4);
        if((NULL == sensor_alarm_devp->sensor_reg) || ((void *)(-1) == sensor_alarm_devp->sensor_reg)){
            printk(KERN_ERR "%s:ioremap failed.\n ",__func__);
            ret = -ENOMEM;
            goto quit1;
        }

        sensor_alarm_devp->alarm_reg = ioremap(GPIO_1_BASEADDR+GPIO_DATAOUT, 4);
        if((NULL == sensor_alarm_devp->alarm_reg) || ((void *)(-1) == sensor_alarm_devp->alarm_reg)){
            printk(KERN_ERR "%s:ioremap failed.\n ",__func__);
            iounmap(sensor_alarm_devp->sensor_reg);
            ret = -ENOMEM;            
            goto quit1;
        }

        gpio_init();        
        atomic_set(&sensor_alarm_devp->users, 0);

        printk(KERN_INFO "sensor_alarm module init,register \"/dev/%s\".\n",SENSOR_ALARM_NAME);
        
        return 0;
        
quit1:
        cdev_del(&sensor_alarm_devp->cdev);
        kfree(sensor_alarm_devp);

 fail_malloc:        
        unregister_chrdev_region(devnum,SUBDEV_NUMBER);
        return ret;
}
void __exit sensor_alarm_exit(void)
{
        device_destroy(sensor_alarm_devp->classp,sensor_alarm_devp->device_no);
        class_destroy(sensor_alarm_devp->classp);
        cdev_del(&sensor_alarm_devp->cdev);
        iounmap(sensor_alarm_devp->sensor_reg);
        iounmap(sensor_alarm_devp->alarm_reg);
        
        kfree(sensor_alarm_devp);
        unregister_chrdev_region(MKDEV(sensor_alarm_major,0),SUBDEV_NUMBER);
        printk(KERN_DEBUG "sensor_alarm module exit.\n");

}

MODULE_AUTHOR("XUECAIWANG");
MODULE_LICENSE("GPL");
module_init(sensor_alarm_init);
module_exit(sensor_alarm_exit);
        
