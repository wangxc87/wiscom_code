#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/phy.h>
#include <linux/device.h>
//#include <mach/mux.h>

MODULE_AUTHOR("WANG");
MODULE_LICENSE("GPL");

#define PHYDEV_MAJOR 0
#define NAME_LENGHT 16
#define PHYDEV_WRITE 0x00
#define PHYDEV_READ 0x01

static int phydev_major = PHYDEV_MAJOR;


struct phydev_dev 
{
        char *name;
        struct cdev cdev;
        int state ;
        int gpio;
        
};
struct phydev_param {
    char phy_id;
    int cmd;
    int regaddr;
    int value;
};

struct phydev_dev *phydev_devp;
struct phy_device *phydev;

static int phydev_open(struct inode *inodep,struct file *filp)
{
        return 0;
}

static int phydev_release(struct inode *inode,struct file *filp)
{
        return 0;
}


//static int phydev_ioctl(struct inode *inodep,struct file *filp,
//                   unsigned int cmd,unsigned long arg)
static int phydev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
//        struct phydev_dev *devp = filp->private_data;
        struct phydev_param *param,phy_param ;
        int ret;
        param = & phy_param;
        switch (cmd){
        case PHYDEV_WRITE:
            if(copy_from_user(param,(struct phydev_param *)arg,sizeof(struct phydev_param)))
                printk("copy_from_user failed..\n");
            ret = phy_write(phydev,param->regaddr,param->value);
            if(ret < 0)
                printk("phydev reg:0x%x write 0x%x fail...\n",param->regaddr,param->value);
                break;
        case PHYDEV_READ:
            if(copy_from_user(param,(struct phydev_param *)arg,sizeof(struct phydev_param)))
                printk("copy_from_user failed..\n");

            param->value = phy_read(phydev,param->regaddr);
            if(copy_to_user((struct phydev_param *)arg,param,sizeof(struct phydev_param)))
                printk("copy_to_user failed...\n");
                break;
        default:
                printk("Invalid CMD...\n");
        }

        return 0;
}

static struct file_operations phydev_fops = {
        .owner = THIS_MODULE,
        .open = phydev_open,
        .release = phydev_release,
        .unlocked_ioctl = phydev_ioctl,
};

static int phydev_setup_cdev(struct phydev_dev *devp,int index)
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
struct device *d;        
        if(phydev_major)
                ret = register_chrdev_region(devnum,1,"phydev");
        else {
                ret = alloc_chrdev_region(&devnum,0,1,"phydev");
                phydev_major = MAJOR(devnum);
        }
        if(ret < 0){
                printk("Error:register Devnum fail...\n");
                return ret;
        }
        printk("phydev MAJOR NUM is %d\n",phydev_major);

        
        phydev_devp = kmalloc(SUBDEV_NUMBER*sizeof(struct phydev_dev),GFP_KERNEL);
        if(!phydev_devp){
                ret = -ENOMEM;
                goto fail_malloc;
        }

        memset(phydev_devp,0,SUBDEV_NUMBER*sizeof(struct phydev_dev));


        phydev_devp->name = "phydev";
        //        phydev_devp->gpio = PHYDEV1;
        ret = phydev_setup_cdev(&phydev_devp[0],0);
        if(ret < 0){
                printk("Error:add phydev1 fail..\n");
        }

	d = bus_find_device_by_name(&mdio_bus_type, NULL, "0:02");
	if (!d) {
		printk("ERR:PHY %s not found\n", "0:02");
        ret = -1;
        goto fail_malloc;
	}
    phydev = to_phy_device(d);

printk("phydev 0x%x init...\n",phydev->phy_id);
        
        return 0;
fail_malloc:
        unregister_chrdev_region(devnum,SUBDEV_NUMBER);
        return ret;
}
void __exit phydev_exit(void)
{
        int i;

        for(i=0;i<SUBDEV_NUMBER;i++){
                
                cdev_del(&phydev_devp[i].cdev);
        }
        
        kfree(phydev_devp);
        unregister_chrdev_region(MKDEV(phydev_major,0),SUBDEV_NUMBER);
        printk("phydev module exit...\n");

}

module_init(phydev_init);
module_exit(phydev_exit);
