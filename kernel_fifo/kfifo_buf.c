/***********************
  ************************
 ***wangxc 2015-04-16****
 ************************/
#include <linux/init.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/kernel.h>
/* #include <asm/unistd.h> */
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include "kfifo_buf.h"

static int debug_en = 1;
#define debug_print(fmt, arg...) do{ if(debug_en) printk(KERN_DEBUG fmt,##arg);}while(0)

static int OSA_queDelete(struct kfifo_que_obj *kque_hndl)
{
    struct kfifo *kfifo_hndl = &kque_hndl->kfifo_hndl;
    kfifo_free(kfifo_hndl);
    return 0;
}

static int OSA_queCreate(struct kfifo_que_obj *kque_hndl, int nums)
{
    int ret;
    struct kfifo *kfifo_hndl = &kque_hndl->kfifo_hndl;
    
    ret = kfifo_alloc(kfifo_hndl, nums, GFP_KERNEL);
	if (ret) {
		printk(KERN_ERR "error kfifo_alloc\n");
		return ret;
	}
    kfifo_reset(kfifo_hndl); //clear read and write ptr
    init_waitqueue_head(&kque_hndl->kfifo_que_wq_h);
    return 0;
}

static int OSA_quePut(struct kfifo_que_obj *kque_hndl, int value, int timeout)
{
    char data = (char) value;
    struct kfifo *kfifo_hndl = &kque_hndl->kfifo_hndl;
    while(1){
        if(kfifo_is_full(kfifo_hndl)){
            if(timeout == OSA_TIMEOUT_NONE)
                return -1;
            wait_event_interruptible(kque_hndl->kfifo_que_wq_h, !kfifo_is_full(kfifo_hndl));
            //            msleep(100);
            debug_print( "%s: kfifo is full,len-%d.\n",
                         __func__, kfifo_len(kfifo_hndl));
            /* printk(KERN_DEBUG "%s: kfifo is full,len-%d.\n", */
            /*        __func__, kfifo_len(kfifo_hndl)); */
        }else {
            if(!kfifo_put(kfifo_hndl, &data)){
                return -1;
            }
            wake_up(&kque_hndl->kfifo_que_wq_h);
            break;
        }
    }
    return 0;
}

static int OSA_queGet(struct kfifo_que_obj *kque_hndl, int *value, int timeout)
{
    char data;
    struct kfifo *kfifo_hndl = &kque_hndl->kfifo_hndl;
    if(!value){
        printk(KERN_ERR "%s: invalide value addr.\n", __func__);
        return -1;
    }
    while(1){
        if(kfifo_is_empty(kfifo_hndl)){
            if(timeout == OSA_TIMEOUT_NONE)
                return -1;
            debug_print(KERN_DEBUG "%s: kfifo is empty,len-%d.\n",
                   __func__, kfifo_len(kfifo_hndl));
            wait_event_interruptible(kque_hndl->kfifo_que_wq_h, !kfifo_is_empty(kfifo_hndl));
            //            msleep(100);
        }else {
            
            if(!kfifo_get(kfifo_hndl, &data)){
                return -1;
            }
            *value = data;
            wake_up(&kque_hndl->kfifo_que_wq_h);
            break;
        }
    }
    return 0;
}

int OSA_bufDelete(OSA_BufHndl *hndl)
{
    int status=OSA_SOK;

    if(hndl==NULL)
        return OSA_EFAIL;

    status = OSA_queDelete(&hndl->emptyQue);
    status |= OSA_queDelete(&hndl->fullQue);

    return status;
}

int OSA_bufCreate(OSA_BufHndl *hndl,
                  struct kfifo_buf_create_arg *bufInit)
{
    int status = OSA_SOK;
    int i;

    if(hndl==NULL || bufInit==NULL)
        return OSA_EFAIL;

    if(  bufInit->numBuf >  KFIFO_BUF_NUM_MAX)
        return OSA_EFAIL;

    memset(hndl, 0, sizeof(OSA_BufHndl));

    status = OSA_queCreate(&hndl->emptyQue, bufInit->numBuf);

    if(status!=OSA_SOK) {
        /* OSA_ERROR("OSA_bufCreate() = %d \r\n", status); */
        return status;
    }

    status = OSA_queCreate(&hndl->fullQue, bufInit->numBuf);

    if(status!=OSA_SOK) {
        OSA_queDelete(&hndl->emptyQue);
        return status;
    }

    hndl->numBuf   = bufInit->numBuf;

    for(i=0; i<hndl->numBuf; i++) {
        hndl->bufInfo[i].size = 0;
        hndl->bufInfo[i].physAddr = bufInit->physAddr[i];
        hndl->bufInfo[i].virtAddr = bufInit->virtAddr[i];
        if(OSA_quePut(&hndl->emptyQue, i, OSA_TIMEOUT_FOREVER) < 0){
            OSA_queDelete(&hndl->emptyQue);
            OSA_queDelete(&hndl->fullQue);
            return -1;
        }
    }

    printk(KERN_DEBUG "%s OK.\n", __func__);

    return status;    
}

int OSA_bufGetEmpty(OSA_BufHndl *hndl, int *bufId, u32 timeout)
{
    int status;

    if(hndl==NULL || bufId==NULL)
        return OSA_EFAIL;

    status = OSA_queGet(&hndl->emptyQue, bufId, timeout);

    if(status!=OSA_SOK) {
        *bufId = OSA_BUF_ID_INVALID;
    }

    return status;
}

int OSA_bufPutFull (OSA_BufHndl *hndl, int bufId)
{
    int status;

    if(hndl==NULL)
        return OSA_EFAIL;

    if(bufId >= hndl->numBuf || bufId < 0)
        return OSA_EFAIL;

    status = OSA_quePut(&hndl->fullQue, bufId, OSA_TIMEOUT_FOREVER);

    return status;
}

int OSA_bufGetFull(OSA_BufHndl *hndl, int *bufId, u32 timeout)
{
    int status;

    if(hndl==NULL || bufId==NULL)
        return OSA_EFAIL;

    status = OSA_queGet(&hndl->fullQue, bufId, timeout);

    if(status!=OSA_SOK) {
        *bufId = OSA_BUF_ID_INVALID;
    }

    return status;
}

int OSA_bufPutEmpty(OSA_BufHndl *hndl, int bufId)
{
    int status;

    if(hndl==NULL)
        return OSA_EFAIL;

    if(bufId >= hndl->numBuf || bufId < 0)
        return OSA_EFAIL;

    status = OSA_quePut(&hndl->emptyQue, bufId, OSA_TIMEOUT_FOREVER);

    return status;
}

OSA_BufInfo *OSA_bufGetBufInfo(OSA_BufHndl *hndl, int bufId)
{
    if(hndl==NULL)
        return NULL;

    if(bufId>=hndl->numBuf)
        return NULL;

    return &hndl->bufInfo[bufId];
}

