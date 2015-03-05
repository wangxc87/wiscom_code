/**********************************
 * Copyright (c) 2014, Wiscom Vision Technology Co. Ltd.
 * PHY device register control driver
 * 2014 年 10 月 09 日
 * xuecaiwang
 *********************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/phy.h>
#include <linux/device.h>

MODULE_AUTHOR("XCWANG");
MODULE_LICENSE("GPL v2");

#define PHYDEV_MAJOR 0
#define NAME_LENGHT 16
#define PHYDEV_NAME "phydev"

struct phydev
{
    char   *name;
    char   phy_addr[16];
    atomic_t  users;
    struct phy_device *phy_device;
    struct cdev cdev;
    struct class *classp;
    struct device *devicep;
};

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

struct phydev *phydevp;
static int phydev_major = PHYDEV_MAJOR;

static int phydev_open(struct inode *inodep,struct file *filp)
{
    if (atomic_inc_return(&phydevp->users) >= 0x02) {
        atomic_dec(&phydevp->users);
        return -EBUSY;
    }
    return 0;
}

static int phydev_release(struct inode *inode,struct file *filp)
{
    if(atomic_read(&phydevp->users) > 0){
        atomic_dec(&phydevp->users);
    }
    return 0;
}


static long phydev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        struct phydev_param phy_param ;
        int ret;
        struct device *d;        

        switch (cmd){
            /* set current phy device */
        case PHYDEV_ID_SET:
            if(copy_from_user(&phy_param,(struct phydev_param *)arg,sizeof(struct phydev_param))){
                printk(KERN_ERR "[KERN]copy_from_user Error..\n");
                return -EINVAL;
            }
            
            d = bus_find_device_by_name(&mdio_bus_type, NULL, phy_param.phy_addr);
            if (!d) {
                printk(KERN_ERR "[KERN]:PHY %s not found.\n", phy_param.phy_addr);
                return -EINVAL;
            }

            phydevp->phy_device = to_phy_device(d);
            if(phydevp->phy_device == NULL){
                printk(KERN_ERR "[KERN]:PHY %s not found.\n", phy_param.phy_addr);
                return -EINVAL;
            }
            memcpy(phydevp->phy_addr,phy_param.phy_addr,16);

            printk(KERN_DEBUG "[KERN] Set phy_addr is %s,phyIC_id 0x%x.\n", phy_param.phy_addr,phydevp->phy_device->phy_id);
            
            break;
        case PHYDEV_ID_GET:
            memcpy(phy_param.phy_addr, phydevp->phy_addr, 16);
            
            if(copy_to_user((struct phydev_param *)arg, &phy_param, sizeof(struct phydev_param))){
                printk(KERN_ERR "[KERN] copy_to_user Error.\n");
                return -EINVAL;
            }
            break;
        case PHYDEV_REG_WRITE:
            if(copy_from_user(&phy_param,(struct phydev_param *)arg,sizeof(struct phydev_param))){
                printk(KERN_ERR "[KERN]copy_from_user Error.\n");
                return -EINVAL;
            }
            ret = phy_write(phydevp->phy_device, phy_param.regaddr,phy_param.value);
            if(ret < 0){
                //                printk([KERN] "[KERN]phydev reg:0x%x write 0x%x fail...\n",param.regaddr,param.value);
                return -EINVAL;
            }
            break;
        case PHYDEV_REG_READ:
            if(copy_from_user(&phy_param,(struct phydev_param *)arg,sizeof(struct phydev_param))){
                printk(KERN_ERR "[KERN]copy_from_user Error.\n");
                return -EINVAL;
            }
            phy_param.value = phy_read(phydevp->phy_device, phy_param.regaddr);
            if(copy_to_user((struct phydev_param *)arg, &phy_param, sizeof(struct phydev_param))){
                printk(KERN_ERR "[KERN]copy_to_user Error.\n");
                return -EINVAL;
            }
            break;
        default:
                printk(KERN_ERR "[KERN] Invalid CMD.\n");
                return -EINVAL;
        }
        return 0;
}

static struct file_operations phydev_fops = {
        .owner = THIS_MODULE,
        .open = phydev_open,
        .release = phydev_release,
        .unlocked_ioctl = phydev_ioctl,
};

static int phydev_setup_cdev(struct phydev *devp,int index)
{
        int err;
        dev_t devnum = MKDEV(phydev_major,index);
        struct cdev *cdevp = &devp->cdev;

        cdev_init(cdevp,&phydev_fops);
        cdevp->owner = THIS_MODULE;
        cdevp->ops = &phydev_fops;

        err = cdev_add(cdevp,devnum,1);
        if(err){
                printk(KERN_ERR "Error %d adding phydev %d\n",err,index);
                return -1;
        }
        return 0;
        
}
#define SUBDEV_NUMBER 1
int __init  phydev_init(void)
{
        dev_t devnum = MKDEV(phydev_major,0);
        int ret;

        if(phydev_major)
                ret = register_chrdev_region(devnum,1,"phydev");
        else {
                ret = alloc_chrdev_region(&devnum,0,1,"phydev");
                phydev_major = MAJOR(devnum);
        }
        if(ret < 0){
                printk(KERN_ERR "Error:register Devnum fail...\n");
                return ret;
        }
        
        phydevp = kmalloc(SUBDEV_NUMBER*sizeof(struct phydev),GFP_KERNEL);
        if(!phydevp){
                ret = -ENOMEM;
                goto fail_malloc;
        }

        memset(phydevp,0,SUBDEV_NUMBER*sizeof(struct phydev));

        phydevp->name = PHYDEV_NAME;
        ret = phydev_setup_cdev(&phydevp[0],0);
        if(ret < 0){
                printk(KERN_ERR "Error:add phydev fail..\n");
        }

        if((phydevp->classp = class_create(THIS_MODULE,PHYDEV_NAME)) == NULL) {
            printk(KERN_ERR "Create phydev class fail.\n");
            ret = -EINVAL;
            goto fail_malloc;
        }

        phydevp->devicep = device_create(phydevp->classp, NULL, devnum, NULL, PHYDEV_NAME);

        if(phydevp->devicep == NULL){
            printk(KERN_ERR "Create phydev device fail.\n");
            class_destroy(phydevp->classp);
            ret = -EINVAL;
            goto quit1;
        }
        atomic_set(&phydevp->users, 0);
        printk(KERN_INFO "phydev init,register /dev/%s.\n",PHYDEV_NAME);
        return 0;
quit1:
        cdev_del(&phydevp->cdev);
        kfree(phydevp);

fail_malloc:
        unregister_chrdev_region(devnum,SUBDEV_NUMBER);
        return ret;
}
void __exit phydev_exit(void)
{
        int i;
        device_destroy(phydevp->classp,MKDEV(phydev_major,0));
        class_destroy(phydevp->classp);

        for(i=0;i<SUBDEV_NUMBER;i++){
                cdev_del(&phydevp[i].cdev);
        }
        
        kfree(phydevp);
        unregister_chrdev_region(MKDEV(phydev_major,0),SUBDEV_NUMBER);
        printk(KERN_INFO "phydev module exit.\n");

}

module_init(phydev_init);
module_exit(phydev_exit);
