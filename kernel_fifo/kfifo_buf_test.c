#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "kfifo_buf.h"

static struct task_struct *kthread_wr = NULL;
static struct task_struct *kthread_rd = NULL;

static int kthread_exit = 0;

static OSA_BufHndl gFifo_buf;

int thread_wr(void *p)
{
    int bufid,ret = 0;
    OSA_BufInfo *buf_info;
    u32 count = 0;
    printk("%s run ...\n", __func__);

    while(!kthread_exit){
        ret = OSA_bufGetEmpty(&gFifo_buf, &bufid, OSA_TIMEOUT_FOREVER);
        if(ret < 0){
            printk("%s: get Empty Error.\n", __func__);
            continue;
        }
        
        buf_info = OSA_bufGetBufInfo(&gFifo_buf, bufid);
        if(!buf_info){
            printk("%s: get bufInfo error.\n", __func__);
            OSA_bufPutEmpty(&gFifo_buf, bufid);
        }

        sprintf(buf_info->virtAddr, "thread_wr write %u.\n", count ++);
        if(OSA_bufPutFull(&gFifo_buf, bufid) < 0)
            printk("%s: put Full failed.\n",__func__);

        msleep(100);
    }
    return 0;
}
int thread_rd(void *p)
{
    int ret, bufid;
    OSA_BufInfo *buf_info;
    printk("%s run ...\n", __func__);
    //   msleep(1*1000);
    while(!kthread_exit){
        ret = OSA_bufGetFull(&gFifo_buf, &bufid, OSA_TIMEOUT_FOREVER);
        if(ret < 0){
            printk("%s: get Full Error.\n", __func__);
            continue;
        }
        
        buf_info = OSA_bufGetBufInfo(&gFifo_buf, bufid);
        if(!buf_info){
            printk("%s: get bufInfo error.\n", __func__);
            OSA_bufPutEmpty(&gFifo_buf, bufid);
            continue;
        }

        printk("%s: %s\n",__func__, (char *)buf_info->virtAddr);

        if(OSA_bufPutEmpty(&gFifo_buf, bufid) < 0)
            printk("%s: put Full failed.\n", __func__);

        msleep(2*100);
        
    }
    return 0;
}
static int __init test_init(void)
{
    int ret;
    int i = 0;
    struct kfifo_buf_create_arg buf_init;
    memset(&buf_init, 0, sizeof(buf_init));

    buf_init.numBuf = 8;

    for(i = 0; i < 8; i++){
        buf_init.virtAddr[i] = kzalloc(128, GFP_KERNEL);
        if(!buf_init.virtAddr[i]){
            printk(KERN_ERR "malloc mem failed.\n");
            return -1;
        }
    }

    printk("[%s %s] %s enter, create bufsize-128,fifo-8...\n",
           __TIME__, __DATE__,__func__);
    
    ret = OSA_bufCreate(&gFifo_buf, &buf_init);
    if(ret < 0){
        printk("OSA_bufCreate failed.\n");
        goto init_err;
    }

    printk("%s: OSA_buf create success.\n", __func__);
    
    kthread_wr = kthread_run(thread_wr, NULL, "thread_wr");
    if(!kthread_wr)
       printk("[kern] thread_wr create failed.\n");
   else
       printk("[kern] thread_wr create success.\n");

    kthread_rd = kthread_run(thread_rd, NULL, "thread_rd");
    if(!kthread_wr)
       printk("[kern] thread_rd create failed.\n");
   else
       printk("[kern] thread_rd create success.\n");

    return 0;

 init_err:
    for(i = 0; i < 10; i++){
        if(buf_init.virtAddr[i]){
            kfree(buf_init.virtAddr[i]);
        }
    }
    return -1;
    
}
static void __exit test_exit(void)
{
    int i;
    printk("TEST exit ..\n");
    if(kthread_wr){
        kthread_exit = 1;
        kthread_stop(kthread_wr);
    }

    if(kthread_rd){
        kthread_exit = 1;
        kthread_stop(kthread_rd);
    }
    
    for(i = 0; i < gFifo_buf.numBuf; i++)
        kfree(gFifo_buf.bufInfo[i].virtAddr);
}

module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");
