/*
  知识点：定时器使用  等待队列  阻塞访问  字符设备  gpio中断 异步通知fasync机制
  采用linux定时器实现秒设备,也采用来等待队列实现阻塞读写
  实现功能：读秒显示，以及采用一个按键中断通知应用程序停读秒
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h> //定义file_operation
#include <linux/uaccess.h>  //定义内核空间和用户空间传输数据的函数

#include <linux/device.h>

#include <linux/interrupt.h>
#include <linux/irq.h>

#include <linux/cdev.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#define DM365_GPIO_NUM  0
#define DM365_GPIO_INT  44

#define SECOND_MAJOR 0
static int second_major = SECOND_MAJOR;

struct second_dev 
{
        struct cdev cdev;
        atomic_t counter;//一共经历来多少秒
        struct timer_list s_timer;//设备使用的定时器
        wait_queue_head_t second_wq;
        int second_flag;
        struct fasync_struct *stop_fasync;//定义异步通知结构体
        struct tasklet_struct *second_taskletp;//定义tasklet指针
        struct class *second_classp;
        
};

struct second_dev *second_devp;

static int gpio_fasync(int fd,struct file *filp,int mode);


//定时器处理函数
static void second_timer_handle(unsigned long arg)
{
        mod_timer(&second_devp->s_timer,jiffies + HZ);//int mod_timer(struct timer_list *timer, unsigned long expires); 用于修改定时器的到期时间，在新的被传入的expire到来后才会执行定时器函数
        atomic_inc(&second_devp->counter);

        second_devp->second_flag = 1;
        wake_up_interruptible(&second_devp->second_wq);//唤醒等待队列
        
        printk("Current jiffies is %ld\n",jiffies);
}

int second_open(struct inode *inodp,struct file *filp)
{
        init_waitqueue_head(&second_devp->second_wq);//初始化等待队列头
        
        init_timer(&second_devp->s_timer);//初始化定时器
        second_devp->s_timer.function = &second_timer_handle;
        second_devp->s_timer.expires  =jiffies + HZ;

        add_timer(&second_devp->s_timer);//添加注册定时器

        atomic_set(&second_devp->counter,0);//计数器清0

        filp->private_data =(void*)second_devp;
        
        printk("Open second_dev OK...\n");
        return 0;
}

int second_release(struct inode *inodp,struct file *filp)
{


        gpio_fasync(-1,filp,0);
        filp->private_data = NULL;        
        del_timer(&second_devp->s_timer);

        printk("release second_dev...\n");
        return 0;
}

//读函数
static ssize_t second_read(struct file *filp,char __user *buf,
                           size_t count,loff_t *ppos)
{
        int counter;
        if(!(filp->f_flags & O_NONBLOCK)){//阻塞访问则等待
                for(;;){
                        wait_event_interruptible(second_devp->second_wq,second_devp->second_flag);//等待一秒种的到来
                        if(second_devp->second_flag){
                                second_devp->second_flag = 0;
                                break;
                        }
                }
        }
        
        counter = atomic_read(&second_devp->counter);//原子读
        if(put_user(counter,(int __user *)buf))
                return -EFAULT;
        else
                return sizeof(unsigned int);
}
static int gpio_fasync(int fd,struct file *filp,int mode)
{
        struct second_dev *devp = (struct second_dev *)filp->private_data;
        
        return fasync_helper(fd,filp,mode,&devp->stop_fasync);//注册异步通知结构体
}

void gpio_tasklet_func(unsigned long data)
{
        unsigned int *p = (unsigned int *)data;

        struct second_dev *devp = (struct second_dev *)(*p);
        struct fasync_struct *fasyncp = devp->stop_fasync;

        if(fasyncp)
                kill_fasync(&fasyncp,SIGIO,POLL_IN);//发送异步通知信号
}

DECLARE_TASKLET(gpio_tasklet,gpio_tasklet_func,
                (unsigned long)&(second_devp));

static irqreturn_t gpio_interrupt(int irq,void *dev_id)
{
        struct second_dev *devp = (struct second_dev *)dev_id;

        tasklet_schedule(devp->second_taskletp);

        return IRQ_HANDLED;
}
        
static int gpio_setup(void)
{
        int ret;
        ret = gpio_is_valid(DM365_GPIO_NUM);//判断gpio是否有效
        if(ret < 0){
                printk("Err:Gpio %d is invalid.\n",DM365_GPIO_NUM);
                return -EINVAL;
        }
        
        ret = gpio_request(DM365_GPIO_NUM,"gpio_stop");
        if(ret < 0){
                printk("Err:Requset GPIO %d failed.\n",DM365_GPIO_NUM);
                return -EINVAL;
        }
        
/*        tasklet_init(second_devp->second_tasklet,gpio_tasklet_func,
                     (unsigned long)second_devp);        
*/

        gpio_direction_input(DM365_GPIO_NUM);

        set_irq_type(DM365_GPIO_INT,IRQ_TYPE_EDGE_RISING);
        disable_irq(DM365_GPIO_INT);

        ret = request_irq(DM365_GPIO_INT,gpio_interrupt,
                          IRQF_DISABLED,"gpio_irq_stop",(void *)second_devp);
        if(ret < 0){
                printk("Err:request irq failed.\n");
                goto fail_out;
        }
        disable_irq(DM365_GPIO_INT);
        enable_irq(DM365_GPIO_INT);

        printk("gpio init over.\n");
        return 0;
fail_out:
        gpio_free(DM365_GPIO_NUM);
        return ret;
        
}
        
//文件操作结构体
static const struct file_operations second_fops = {
        .owner = THIS_MODULE,
        .open  = second_open,
        .release = second_release,
        .read = second_read,
        .fasync = gpio_fasync,
};

static void setup_cdev(struct cdev *cdevp,int index)
{
        int err;
        dev_t devnum = MKDEV(second_major,index);

        cdev_init(cdevp,&second_fops);
        cdevp->owner = THIS_MODULE;
        cdevp->ops = &second_fops;

        err = cdev_add(cdevp,devnum,1);
        if(err)
                printk("Error %d adding second_dev%d\n",err,index);
}

int __init second_init(void)
{
        int ret;
        dev_t devnum = MKDEV(second_major,0);

        //申请设备号
        if(second_major)
                ret = register_chrdev_region(devnum,1,"second");
        else {
                ret = alloc_chrdev_region(&devnum,0,1,"second");
                second_major = MAJOR(devnum);
        }
        if(ret < 0 ){
                printk("ERR:register devnum failed...\n");
                return ret;
        }

        second_devp = kmalloc(sizeof(struct second_dev),GFP_KERNEL);
        if(!second_devp){
                ret = -ENOMEM;
                printk("ERR:kmalloc failed...\n");
                goto fail_malloc;
        }

        memset(second_devp,0,sizeof(struct second_dev));

        setup_cdev(&second_devp->cdev,0);
        second_devp->second_taskletp = &gpio_tasklet;
        
        ret = gpio_setup();
        if(ret < 0){
                printk("Err:gpio init failed.\n");
//                goto fail_malloc;
        }
        //加载模块自动创建设备节点
        second_devp->second_classp = class_create(THIS_MODULE,"second");
        if(!second_devp->second_classp)
                printk("Error:class_create failed...\n");
         else
                device_create(second_devp->second_classp,NULL,
                              devnum,NULL,"second");
        

        printk("second_dev %d init...\n",second_major);
        
        return 0;
fail_malloc:
        unregister_chrdev_region(devnum,1);
        return ret;
        
}

void __exit second_exit(void)
{
        disable_irq(DM365_GPIO_INT);
        free_irq(DM365_GPIO_INT,(void *)second_devp);
        gpio_free(DM365_GPIO_NUM);

        device_destroy(second_devp->second_classp,MKDEV(second_major,0));
        class_destroy(second_devp->second_classp);
        
        cdev_del(&second_devp->cdev);
        kfree(second_devp);
        unregister_chrdev_region(MKDEV(second_major,0),1);//释放设备号

        printk("second exit...\n");
        
}

MODULE_AUTHOR("WANG");
MODULE_LICENSE("GPL");

module_param(second_major,int,S_IRUGO);

module_init(second_init);
module_exit(second_exit);

        
        
        
        
        
        
        
        
