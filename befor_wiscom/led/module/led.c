#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <mach/gpio.h>
#include <mach/mux.h>

#include "led.h"                              \
        
MODULE_AUTHOR("WANG");
MODULE_LICENSE("GPL");

#define LED_MAJOR 0
#define LED1  58
#define LED2  57
#define NAME_LENGHT 16
static int led_major = LED_MAJOR;


struct led_dev 
{
        char *name;
        struct cdev cdev;
        int state ;
        int gpio;
        
};

struct led_dev *led_devp;

static int led_open(struct inode *inodep,struct file *filp)
{
        int major,minor;
        
        struct led_dev *led_dev = container_of(inodep->i_cdev,
                                               struct led_dev,
                                               cdev);
        major = MAJOR(inodep->i_rdev);
        minor = MINOR(inodep->i_rdev);
        printk("open device:major %d,minor %d\n",major,minor);

        gpio_request(led_dev->gpio,led_dev->name);
        gpio_direction_output(led_dev->gpio,0);
        
        filp->private_data = led_dev;

        return 0;
}

static int led_release(struct inode *inode,struct file *filp)
{
        filp->private_data = NULL;
        return 0;
}

int led_set(struct led_dev *devp)
{
        int state;
        int gpio;

        state = devp->state;
        gpio = devp->gpio;

        gpio_set_value(gpio,state);
        return 0;
}

static int led_ioctl(struct inode *inodep,struct file *filp,
                     unsigned int cmd,unsigned long arg)
{
        struct led_dev *devp = filp->private_data;

        switch (cmd){
        case LED_ON:
                devp->state = 1;
                led_set(devp);
                break;
        case LED_OFF:
                devp->state = 0;
                led_set(devp);
                break;
        default:
                printk("Invalid CMD...\n");
        }

        return 0;
}

static struct file_operations led_fops = {
        .owner = THIS_MODULE,
        .open = led_open,
        .release = led_release,
        .ioctl = led_ioctl,
};

static int led_setup_cdev(struct led_dev *devp,int index)
{
        int err;
        dev_t devnum = MKDEV(led_major,index);
        struct cdev *cdevp = &devp->cdev;

        cdev_init(cdevp,&led_fops);
        cdevp->owner = THIS_MODULE;
        cdevp->ops = &led_fops;

        err = cdev_add(cdevp,devnum,1);
        if(err){
                printk(KERN_ERR "Error %d adding led %d\n",err,index);
                return -1;
        }
        return 0;
        
}
#define SUBDEV_NUMBER 2
int __init  led_init(void)
{
        dev_t devnum = MKDEV(led_major,0);
        int ret;
        
        if(led_major)
                ret = register_chrdev_region(devnum,SUBDEV_NUMBER,"led");
        else {
                ret = alloc_chrdev_region(&devnum,0,SUBDEV_NUMBER,"led");
                led_major = MAJOR(devnum);
        }
        if(ret < 0){
                printk("Error:register Devnum fail...\n");
                return ret;
        }
        printk("led MAJOR NUM is %d\n",led_major);

        
        led_devp = kmalloc(SUBDEV_NUMBER*sizeof(struct led_dev),GFP_KERNEL);
        if(!led_devp){
                ret = -ENOMEM;
                goto fail_malloc;
        }

        memset(led_devp,0,SUBDEV_NUMBER*sizeof(struct led_dev));


        led_devp->name = "led1";
        led_devp->gpio = LED1;
        ret = led_setup_cdev(&led_devp[0],0);
        if(ret < 0){
                printk("Error:add led1 fail..\n");
        }
        
        led_devp[1].name = "led2";
        led_devp[1].gpio = LED2;
        ret = led_setup_cdev(&led_devp[1],1);
        if(ret < 0){
                printk("Error:add led2 fail..\n");
        }
        printk("led module init...\n");
        
        return 0;
fail_malloc:
        unregister_chrdev_region(devnum,SUBDEV_NUMBER);
        return ret;
}
void __exit led_exit(void)
{
        int i;

        for(i=0;i<SUBDEV_NUMBER;i++){
                
                cdev_del(&led_devp[i].cdev);
        }
        
        kfree(led_devp);
        unregister_chrdev_region(MKDEV(led_major,0),SUBDEV_NUMBER);
        printk("led module exit...\n");

}

module_init(led_init);
module_exit(led_exit);

        
                           
        
