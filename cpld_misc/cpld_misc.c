/************************
// =====================================================================================
//            Copyright (c) 2015, Wiscom Vision Technology Co,. Ltd.
//
//       Filename:  cpld_misc.c
//
//    Description:
//
//        Version:  1.0
//        Created:  2015-01-14
//
//         Author:  xuecaiwang
//
 =====================================================================================*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>

#include <asm/ioctl.h>

#define DEBUG 1
#include <linux/device.h>


#define CPLD_SLAVE_DEVADDR                0x76
#define CPLD_CONTROL_RESET_REG            0x00
#define CPLD_CONTROL_WD_ENABLE_REG        0x01
#define CPLD_CONTROL_WD_FEED_REG          0x02
#define CPLD_CONTROL_LED_REGL             0x03
#define CPLD_CONTROL_LED_REGH             0x04
#define CPLD_CONTROL_SENSOR_REG0          0x05
#define CPLD_CONTROL_PCB_VERSION_REG      0x06
#define CPLD_CONTROL_SENSOR_REG1          0x07
#define CPLD_CONTROL_POSITION_REG         0x07
#define CPLD_CONTROL_POWER_STATUS_REG     0x08
#define CPLD_CONTROL_VERION_UPDATE_REG    0x09
#define CPLD_CONTROL_PWMFAN_SPEED_REG     0x0A
#define CPLD_CONTROL_POWER_SHUTDOWN_REG   0x0B
#define CPLD_CONTROL_NETCONFIG_REG       (0x0C)

struct cpld_msg {
    int reg_addr;
    int reg_value;
};

#define CPLD_MISC_IOCTL_MAGIC 'M'
#define CPLD_MISC_SET_REG_IOCMD    (_IOW(CPLD_MISC_IOCTL_MAGIC,0x13, struct cpld_msg))
#define CPLD_MISC_GET_REG_IOCMD    (_IOR(CPLD_MISC_IOCTL_MAGIC,0x14, struct cpld_msg))

#define CPLD_MISC_MINIOR  (MISC_DYNAMIC_MINOR)


struct cpld_misc {
    int board_id;
    struct i2c_client *i2c_slave;
};

static struct cpld_misc gCpld_misc;
static gCpld_misc_exist = 1;

static int cpld_write(struct i2c_client *client, int reg_addr, int reg_value)
{
    char buf[2];
    int ret = 0;

    if(0 == gCpld_misc_exist){
        printk(KERN_DEBUG "%s: cpld not exist, return.\n", __func__);
        return 0;
    }

    buf[0] = (char )reg_addr;
    buf[1] = (char )reg_value;
    ret = i2c_master_send(client, buf, 2);
    if(ret < 0){
        printk(KERN_DEBUG "%s: i2c write reg-0x%x failed.\n", __func__, reg_addr);
    }
    return 0;
}

static int cpld_read(struct i2c_client *client, int reg_addr)
{
    int ret = 0;
    char buf = 0;

    if(0 == gCpld_misc_exist){
        printk(KERN_DEBUG "%s: cpld not exist, return.\n", __func__);
        return 0;
    }

    ret = i2c_master_send(client, (char *)&reg_addr, 1);
    if(ret < 0){
        printk(KERN_DEBUG "%s: i2c write failed.\n", __func__);
        return ret;
    }

    ret = i2c_master_recv(client, &buf, 1);
    if(ret < 0){
        printk(KERN_DEBUG "%s: i2c read faild.\n", __func__);
        return ret;
    }
    return buf;
}
static int cpld_misc_open(struct inode *inode, struct file *filp)
{
        
    return 0;    
}

static int cpld_misc_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long cpld_misc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct cpld_msg msg;

    if(copy_from_user(&msg, (void __user *)arg, sizeof(struct cpld_msg))){
        printk(KERN_ERR "%s: copy_from_user Error.\n", __func__);
        return -EINVAL;
    }
    switch (cmd){
    
    case CPLD_MISC_SET_REG_IOCMD:
        ret = cpld_write(gCpld_misc.i2c_slave, msg.reg_addr, msg.reg_value);
        if(ret < 0){
            printk(KERN_ERR "%s: cpld_write Error.\n", __func__);
            return -EIO;
        }
        break;
    
    case CPLD_MISC_GET_REG_IOCMD:
        ret = cpld_read(gCpld_misc.i2c_slave, msg.reg_addr);
        if(ret < 0){
            printk(KERN_ERR "%s: cpld_read Error.\n", __func__);
            return -EIO;
        }
        msg.reg_value = ret;
        if(copy_to_user((void __user *)arg, &msg, sizeof(struct cpld_msg))){
            printk(KERN_ERR "%s: copy_to_user Error.\n", __func__);
            return -EINVAL;
        }
        break;
    default:
        printk(KERN_ERR "%s: Invalid cmd.\n", __func__);
        return -EINVAL;
    }
    return 0;
}

static struct file_operations cpld_misc_fops = {
    .open = cpld_misc_open,
    .release = cpld_misc_release,
    .unlocked_ioctl = cpld_misc_ioctl,
};

static struct miscdevice cpld_misc_dev = {
    .minor = CPLD_MISC_MINIOR,
    .name = "cpld_misc",
    .fops = &cpld_misc_fops,
};

static int cpld_misc_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    int board_id;
    struct i2c_msg msg[2];
    char buf[2];
    msg[0].addr = client->addr;//info->addr;//i2c addr
    msg[0].flags &=I2C_M_TEN;
    msg[0].flags |= I2C_M_RD;
    msg[0].len = 1;
    msg[0].buf = &buf[0];
    dev_dbg(&gCpld_misc.i2c_slave->dev, "%s...\n", __func__);
    printk(KERN_DEBUG "%s..\n", __func__);

    gCpld_misc.i2c_slave = client;
    gCpld_misc_exist = 1; //define cpld exist

    ret = cpld_read(client, CPLD_CONTROL_RESET_REG);
    if(ret < 0){
        printk(KERN_ERR "%s: cpld Not detected .\n", __func__);
        gCpld_misc_exist = 0;
        //        return -ENODEV;
    }


    ret = misc_register(&cpld_misc_dev);
    if(ret < 0){
        printk(KERN_ERR "misc register failed.\n");
        return -1;
    }

    board_id = cpld_read(gCpld_misc.i2c_slave, CPLD_CONTROL_PCB_VERSION_REG);
    if(board_id < 0){
        printk(KERN_ERR "Get current boardId failed.\n");
        return -1;
    }
    gCpld_misc.board_id = board_id;
    printk("Get current boardId is 0x%x.\n", board_id);
    return 0;
}

static int cpld_misc_remove(struct i2c_client *client)
{
    return misc_deregister(&cpld_misc_dev);
}
static   struct i2c_adapter *adapter;

static int cpld_misc_detect(struct i2c_client *client, struct i2c_board_info *info)
{  //not used in dym-register mode
    int ret;
    struct i2c_msg msg[2];
    char buf[2];
    msg[0].addr = info->addr;//i2c addr
    msg[0].flags &=I2C_M_TEN;
    msg[0].flags |= I2C_M_RD;
    msg[0].len = 1;
    msg[0].buf = &buf[0];
    printk(KERN_DEBUG "%s..\n", __func__);
    ret = i2c_transfer(adapter, msg, 1);
    if(ret < 0){
        printk(KERN_ERR "%s: cpld detect Error.\n", __func__);
        return -1;
    }

    return 0;
}
static const struct i2c_device_id cpld_misc_id[] = {
    {"cpld_misc", 0},
    { },
};

static struct i2c_driver cpld_misc_driver = {
    .driver = {
        .name = "cpld_misc",
    },
    .probe = cpld_misc_probe,
    .remove = cpld_misc_remove,
    .id_table = cpld_misc_id,

    .detect = cpld_misc_detect,
};


static int __init cpld_misc_init(void)
{
    int bus_id = 1;
    int ret;
    struct i2c_board_info info = {
        I2C_BOARD_INFO("cpld_misc", 0x76),
    };
    adapter = i2c_get_adapter(bus_id);
    if(adapter == NULL){
        printk(KERN_ERR "**get i2c adapter %d failed..\n", bus_id);
        return -1;
    }

    //    gCpld_misc.i2c_slave = i2c_new_device(adapter, &info);
    if(i2c_new_device(adapter, &info) == NULL){
        printk(KERN_ERR "%s: i2c add new device failed.\n", __func__);
        return -1;
    }

    ret = i2c_add_driver(&cpld_misc_driver);
    if(ret < 0){
        printk(KERN_ERR "%s: i2c add driver failed.\n", __func__);
        i2c_unregister_device(gCpld_misc.i2c_slave);
        return ret;
    }
    dev_dbg(&gCpld_misc.i2c_slave->dev, "%s OK.\n", __func__);
    printk("%s OK.\n", __func__);
    return ret;
}

static void __exit cpld_misc_exit(void)
{
    i2c_unregister_device(gCpld_misc.i2c_slave);
    i2c_del_driver(&cpld_misc_driver);
    
}

MODULE_AUTHOR("wiscom");
MODULE_DESCRIPTION("wiscom cpld misc driver");
MODULE_LICENSE("GPL");

module_init(cpld_misc_init);
module_exit(cpld_misc_exit);
