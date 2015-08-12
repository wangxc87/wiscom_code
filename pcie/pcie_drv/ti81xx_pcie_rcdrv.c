/**
 * kernel module for root complex to provide communication interface between
 * RC and EPs. Applcication running on Root Complex will use this module for
 * communication with any EP.
 *
 * Copyright (C) 2011, Texas Instruments, Incorporated
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#ifndef _DM81XX_EXCLUDE_
#include <mach/irqs.h>
#endif

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <asm/irq.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/atomic.h>
#include "ti81xx_pcie_rcdrv.h"

#define DEV_MINOR			0
#define DEVICENO			1
#define DRIVER_NAME			"ti81xx_ep_hlpr"
#define TI81XX_EP_HLPR			"ti81xx_ep_hlpr"

#define MB				(1024 * 1024)

#define TI81XX_PCI_VENDOR_ID		0x104c
#define TI816X_PCI_DEVICE_ID		0xb800
#define TI814X_PCI_DEVICE_ID		0xb801
#define TI813X_PCI_DEVICE_ID		0xb802

#ifndef _DM81XX_EXCLUDE_
#define PCI_NON_PREFETCH_START		0x20000000
#define PCI_NON_PREFETCH_SIZE		(256 * MB)
#endif


dev_t			ti81xx_ep_hlpr_dev;
static struct cdev	ti81xx_ep_hlpr_cdev;
static struct class	*ti81xx_ep_hlpr_class;
static u32		*mgmt_area_start;
static u32		device_ep;

#ifndef _DM81XX_EXCLUDE_
static u32		gLocal_outboud_mem;
#endif

static atomic_t         irq_raised = ATOMIC_INIT(0);


wait_queue_head_t	readQ;
static struct semaphore sem_poll;


/**
 * Function Declarations
 *
 */

static int ti81xx_ep_hlpr_mmap(struct file *filp, struct vm_area_struct *vma);
static long ti81xx_ep_hlpr_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg);
static unsigned int ti81xx_ep_hlpr_poll(struct file *filp, poll_table *wait);
static int check_device(u32 device_id);
static int msi_on(u32 device_id);
static int msi_off(u32 device_id);

#ifdef CONFIG_WISCOM
struct pcieRc_info {
    struct pciedev_info ep_info[PCIE_EP_MAX_NUMBER];
    wait_queue_head_t data_wq_h;
    wait_queue_head_t cmd_wq_h;
    struct pciedev_bufObj recvBufObj; //received data fifo
    struct pciedev_databuf_head *data_head_ptr;
};

static struct pcieRc_info gPciedev_info;

#define RC_RESV_MEM_SIZE_DEFUALT (6*1024*1024)

static struct  fasync_struct *ep_hlpr_async_queue = NULL;

static int resv_mem_size = RC_RESV_MEM_SIZE_DEFUALT;
module_param(resv_mem_size,int, S_IRUGO);
MODULE_PARM_DESC(resv_mem_size, "The reserved mem size of RC [default 6M]");

static int cmd_fifo_size = CMD_BUF_SIZE;
module_param(cmd_fifo_size, int , S_IRUGO);
MODULE_PARM_DESC(cmd_fifo_size,"The buf size of cmdFifo [default 2k].");

static int cmd_fifo_num = CMD_BUF_FIFOS;//default 64
module_param(cmd_fifo_num, int , S_IRUGO);
MODULE_PARM_DESC(cmd_fifo_num,"The buf nums of cmdFifo [default 64].");

static int debug=0;
module_param(debug, int , S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(debug,"The debug print enable [default 0].");
#define debug_print(fmt, arg...) do{ if(debug) printk(KERN_DEBUG fmt, ##arg);}while(0)


static int ti81xx_ep_hlpr_open(struct inode *inodp,struct file *filp)
{
    return 0;
}
static int ti81xx_ep_hlpr_fasync(int fd, struct file *filp, int mode);

static int ti81xx_ep_hlpr_release(struct inode *inodp,struct file *filp)
{
    ti81xx_ep_hlpr_fasync(-1,filp, 0);
    return 0;
}

DECLARE_WORK(gPciedev_workque_rc, (void *)rc_do_work);
static void rc_do_work(unsigned long arg);

static int put_cmdBuf(struct pcie_bufHndl *bufHndl, struct pciedev_info *ep_info)
{
    char *buf_put = NULL;
    int buf_size, ret;
    char *cmd_src = ep_info->recv_cmd;

    if(wait_event_interruptible(gPciedev_info.cmd_wq_h, !pcie_buf_isFull(bufHndl)) < 0){
        printk(KERN_ERR "wait cmdfifo Empty event interrupte.\n");
        return pcie_slave_sendAck(CMD_ACK_ERR, FALSE);
    }

    buf_put = pcie_buf_getEmtpy(bufHndl);
    if(!buf_put){
        debug_print("[pcie%d] get cmd EmptyBuf failed.\n", gSelf_id);
        return pcie_slave_sendAck(CMD_ACK_ERR, FALSE);
    }

    buf_size = (int *)cmd_src;
    memcpy(buf_put, cmd_src, buf_size);

    ret = pcie_buf_setFull(bufHndl, buf_put, buf_size);
    if(ret < 0){
        pr_err("[pcie%d] put FullBuf failed.\n ", gSelf_id);
        return pcie_slave_sendAck(CMD_ACK_ERR, FALSE);
    }

    ret = pcie_slave_sendAck(CMD_ACK, FALSE);
    if(ret < 0){
        pr_err("[pcie%d] send CMD_ACK failed.\n", gSelf_id);
    }
    
    return 0;
    
}

static int deQue_cmdBuf(struct pcie_bufHndl *bufHndl, struct pciedev_buf_info *buf_info)
{
    char *buf_temp = NULL;
    u32 buf_size;

    /* wait_event_interruptible_timeout(gPciedev_info.data_wq_h,!pcie_buf_isEmpty(bufHndl), jiffies + 30*HZ); */
    wait_event_interruptible(gPciedev_info.cmd_wq_h,!pcie_buf_isEmpty(bufHndl));
    if(pcie_buf_isEmpty(bufHndl))
    {
        printk(KERN_DEBUG "%s: wait event timeout.\n", __func__);
        return -1;
    }

    buf_size = pcie_buf_getFull(bufHndl, &buf_temp);
    if(buf_size < 0){
        pr_debug("[pcie%d] get cmd FullBuf failed.\n", gSelf_id);
        return -1;
    }

    debug_print( "%s: GET cmdBuf :ptr-0x%lx size:%u.\n",__func__, (unsigned long )buf_temp, (unsigned int)buf_size);
    
    buf_info->buf_kptr = buf_temp;
    buf_info->buf_ptr_offset = buf_temp - gPciedev_info.recvBufObj.cmd_buf_base;
    buf_info->buf_size = buf_size;
    return 0;
}

static int que_cmdBuf(struct pcie_bufHndl *bufHndl, struct pciedev_buf_info *buf_info)
{
    int ret = 0;
    if (pcie_buf_setEmpty(bufHndl, buf_info->buf_kptr) <0)
        ret = -1;
    wake_up(&gPciedev_info.cmd_wq_h);
    return ret;
}
void rc_do_work(unsigned long arg)
{
    int i, dev_id;
    u32 *ep_cmd;
    atomic_t loop_exit = ATOMIC_INIT(0);
    while(!atomic_read(loop_exit)){
        atomic_set(&loop_exit,1);
        for(i = 0; i < gEp_nums; i++){
            dev_id = gPciedev_info.ep_info[i].dev_id;
            if(!dev_id)
                continue;
        
            ep_cmd = (UInt32 *)&gPciedev_info.data_head_ptr->ep_rd_flag[devid_to_index(dev_id)];
        
            if((*ep_cmd & SEND_CMD) == SEND_CMD){
                *ep_cmd &= ~SEND_CMD;
                put_cmdBuf(&gPciedev_bufObj.cmd_bufHndl, &gPciedev_info.info[i]);
                atomic_set(&loop_exit, 0);
            }
        }
    }
}
#endif
/**
 * ti81xx_ep_hlpr_mmap() -Provide userspace mapping for specified kernel memory
 *
 * @filp: File private data
 * @vma: User virtual memory area to map to
 *
 */

static int ti81xx_ep_hlpr_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = -EINVAL;
	unsigned long sz = vma->vm_end - vma->vm_start;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	ret = remap_pfn_range(vma, vma->vm_start,
					vma->vm_pgoff,
						sz, vma->vm_page_prot);
	return ret;
}

/**
 * ti81xx_ep_hlpr_ioctl()- interface for application
 *
 * application can query about start address of management area
 * using specified IOCTL and genrate interrupt to EPS if RC is NETRA.
 */


static long ti81xx_ep_hlpr_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	int ret = 0;
	switch (cmd) {
	case TI81XX_RC_START_ADDR_AREA:
	{

		((struct ti81xx_start_addr_area *)arg)->start_addr_virt =
							(u32)mgmt_area_start;
		((struct ti81xx_start_addr_area *)arg)->start_addr_phy =
				(u32) virt_to_phys((void *)mgmt_area_start);
	}
	break;

#ifndef _DM81XX_EXCLUDE_
	case TI81XX_RC_SEND_MSI:
	{
		unsigned int offset = 0;
		offset = arg - PCI_NON_PREFETCH_START;
		__raw_writel(0, gLocal_outboud_mem + offset + 0x54);//MSI_IRQ
	}
	break;
#endif
#ifdef CONFIG_WISCOM
    case TI81XX_RC_SET_MISCINFO:
        {
            struct ti81xx_outb_miscinfo misc_info;
            struct pciedev_info *pcie_info = NULL;
            if(!arg){
                printk(KERN_ERR "IOCTL: RC_SET_MISCINFO error.\n");
                return -1;
            }

            if(copy_from_user((char *)&misc_info, (char *)arg, sizeof(struct ti81xx_outb_miscinfo))){
                printk(KERN_ERR "IOCTL: copy_from_user error.\n");
                return -1;
            }
            if(misc_info.ep_index > PCIE_EP_MAX_NUMBER){
                printk(KERN_ERR "IOCTL: invalide ep_index.\n");
                return -1;
            }
                
            pcie_info = &gPciedev_info.ep_info[misc_info.ep_index];
            pcie_info->dev_id = misc_info.dev_id;
            pcie_info->recv_cmd = gLocal_outboud_mem + misc_info.cmd_recv_offset;
            pcie_info->send_cmd = gLocal_outboud_mem + misc_info.cmd_send_offset;
            memcpy(pcie_info->res_value, misc_info.res_value, sizeof(mic_info.res_value));
            pcie_info->cmd_head_ptr = gLocal_outboud_mem + pcie_info->res_value[2][0] - PCIE_NON_PREFETCH_START;
        }
        break;
#endif
	default:
		ret = -1;
	}
	return ret;
}


/**
 * ti81xx_ep_hlpr_poll()-- this function supports poll call from user space.
 *
 * on receving interrupt from other EP, it wake up and
 * check data availabilty, if data is available for reading
 * it return POLLIN otherwise it return 0.
 *
 */


static unsigned int ti81xx_ep_hlpr_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	down(&sem_poll);
	poll_wait(filp, &readQ, wait);
	if (atomic_read(&irq_raised) == 1)
		mask |= POLLIN;    /* readable */
	atomic_set(&irq_raised, 0);
	up(&sem_poll);
	return mask;
}

/**
 * msi_handler()- interrupt handler to be registered.
 */

static irqreturn_t msi_handler(int irq, void *dev)
{
	atomic_set(&irq_raised, 1);
	wake_up_interruptible(&readQ);

#ifdef CONFIG_WISCOM
    if(ep_hlpr_async_queue)
        kill_fasync(&ep_hlpr_async_queue, SIGIO, POLL_IN);
#endif
    
	return IRQ_HANDLED;

}

/**
 * msi_on()- iterate over list of pci end point devices and if there is a EP
 * then enable msi and register an interrupt handler.
 *
 * @device_id: device id of End point DM816X/DM814X
 */

static int msi_on(u32 device_id)
{
	struct pci_dev *dev = pci_get_device(TI81XX_PCI_VENDOR_ID,
							device_id, NULL);
	int ret;
    
	while (NULL != dev) {
#ifndef _DM81XX_EXCLUDE_
		if ((dev->class >> 8) == PCI_CLASS_BRIDGE_PCI) {
			pr_info(TI81XX_EP_HLPR
					": skipping TI81XX PCIe RC...\n");

			dev = pci_get_device(TI81XX_PCI_VENDOR_ID,
							device_id, dev);

			continue;
		}
#endif

		pr_info(TI81XX_EP_HLPR ": Found TI81XX PCIe EP @0x%08x\n",
								(int)dev);

		if (pci_enable_msi(dev) != 0) {
			pr_info(TI81XX_EP_HLPR ": Enable msi failed for "
						" @0x%08x device\n", (int)dev);
		}

		ret = request_irq(dev->irq, msi_handler, 0, TI81XX_EP_HLPR ,
							&ti81xx_ep_hlpr_cdev);
		if (ret != 0) {
			pr_err(DRIVER_NAME ": interrupt register failed for "
						" @0x%08x device\n", (int)dev);
		}

		dev = pci_get_device(TI81XX_PCI_VENDOR_ID,
						device_id, dev);

	}

	pr_info(TI81XX_EP_HLPR ": No more TI81XX PCIe EP found\n");
	return 0;
}


/**
 * msi_off()- iterate over list of pci end point devices and if there is an EP
 * then free irq and disable MSI on it.
 *
 * @device_id: device id of End point DM81XX
 */

static int msi_off(u32 device_id)
{

	struct pci_dev *dev = pci_get_device(TI81XX_PCI_VENDOR_ID,
							device_id, NULL);
    int i = 0;
	while (NULL != dev) {
#ifndef _DM81XX_EXCLUDE_
		if ((dev->class >> 8) == PCI_CLASS_BRIDGE_PCI) {
			pr_info(TI81XX_EP_HLPR
					": skipping TI81XX PCIe RC...\n");

			dev = pci_get_device(TI81XX_PCI_VENDOR_ID,
							device_id, dev);

			continue;
		}
#endif

		pr_info(TI81XX_EP_HLPR ": Found TI81XX PCIe EP%d @0x%08x\n",
                i, (int)dev);

		free_irq(dev->irq, &ti81xx_ep_hlpr_cdev);

		pci_disable_msi(dev);

		dev = pci_get_device(TI81XX_PCI_VENDOR_ID,
						device_id, dev);
        i ++;
	}

	pr_info(TI81XX_EP_HLPR ": Found %d EPs, No more TI81XX PCIe EP found\n", i);
	return 0;
}

/**
 * check_device: checks wether a particular EP is present on this system or not
 *
 * @device_id: device id of end-point DM81XX
 */

static int check_device(u32 device_id)
{
	struct pci_dev *dev = pci_get_device(TI81XX_PCI_VENDOR_ID,
							device_id, NULL);

	if (NULL != dev) {
		#ifndef _DM81XX_EXCLUDE_
		if ((dev->class >> 8) == PCI_CLASS_BRIDGE_PCI) {
			pr_info(TI81XX_EP_HLPR
					": skipping TI81XX PCIe RC...\n");

			dev = pci_get_device(TI81XX_PCI_VENDOR_ID,
							device_id, dev);

		}
		#endif
	} else
		return -1;

	if (NULL != dev)
		return 0;
	else
		return -1;
}


#ifdef CONFIG_WISCOM
static int ti81xx_ep_hlpr_fasync(int fd, struct file *filp, int mode)
{
    //    struct
    return fasync_helper(fd, filp, mode, &ep_hlpr_async_queue);
}
#endif

/**
 * ti81xx_ep_hlpr_fops - Declares supported file access functions
 */

static const struct file_operations ti81xx_ep_hlpr_fops = {
	.owner          = THIS_MODULE,
	.mmap           = ti81xx_ep_hlpr_mmap,
	.poll           = ti81xx_ep_hlpr_poll,
	.unlocked_ioctl = ti81xx_ep_hlpr_ioctl,
#ifdef CONFIG_WISCOM
    .open   = ti81xx_ep_hlpr_open,
    .release = ti81xx_ep_hlpr_release,
    .fasync = ti81xx_ep_hlpr_fasync,
#endif
};

/*
 * init part of module
 */

static int __init ep_hlpr_init(void)
{
	int ret = 0;

	if (check_device(TI816X_PCI_DEVICE_ID) == 0) {
		pr_info(DRIVER_NAME ": TI816X device is working as EP\n");
		device_ep = TI816X_PCI_DEVICE_ID;
	} else if (check_device(TI814X_PCI_DEVICE_ID) == 0) {
		pr_info(DRIVER_NAME ": TI814X device is working as EP\n");
		device_ep = TI814X_PCI_DEVICE_ID;
	} else if (check_device(TI813X_PCI_DEVICE_ID) == 0) {
		pr_info(DRIVER_NAME ": TI813X device is working as EP\n");
		device_ep = TI813X_PCI_DEVICE_ID;
	} else {
		pr_err(DRIVER_NAME ": there is no DM81XX EP detected\n");
		return -1;
	}

	ret = alloc_chrdev_region(&ti81xx_ep_hlpr_dev, DEV_MINOR,
						DEVICENO, TI81XX_EP_HLPR);
	if (ret < 0) {

		pr_err(DRIVER_NAME ": could'nt allocate the character driver");
		return -1;
	}

	cdev_init(&ti81xx_ep_hlpr_cdev, &ti81xx_ep_hlpr_fops);
	ret = cdev_add(&ti81xx_ep_hlpr_cdev, ti81xx_ep_hlpr_dev, DEVICENO);
	if (ret < 0) {
		pr_err(DRIVER_NAME ": cdev add failed");
		unregister_chrdev_region(ti81xx_ep_hlpr_dev, DEVICENO);
		return -1;
	}
#ifndef _DM81XX_EXCLUDE_
	gLocal_outboud_mem = (u32)ioremap_nocache(PCI_NON_PREFETCH_START,
						PCI_NON_PREFETCH_SIZE);

	if (!gLocal_outboud_mem) {
		pr_err(DRIVER_NAME ": pci memory remap failed");
		goto ERROR_POST_REMAP;
	}
#endif

	ti81xx_ep_hlpr_class = class_create(THIS_MODULE, TI81XX_EP_HLPR);
	if (!ti81xx_ep_hlpr_class) {
		pr_err(DRIVER_NAME ":failed to add device to sys fs");
		goto ERROR_POST_CLASS;
	}

	device_create(ti81xx_ep_hlpr_class, NULL, ti81xx_ep_hlpr_dev,
						NULL, TI81XX_EP_HLPR);

	mgmt_area_start = kmalloc(4 * MB, GFP_KERNEL);
	if (mgmt_area_start == NULL) {
		printk(KERN_WARNING "KMALLOC failed");
		goto ERROR_POST_BUFFER;
	}
#ifdef CONFIG_WISCOM
    struct kfifo_buf_create_arg buf_init;
    char *local_cmdfifo_base = (char *)gLocal_resv_buf + resv_mem_size - cmd_fifo_num*cmd_fifo_size;

    gSelf_id = gPcieDev_id;

    memset(&gPciedev_info, 0, sizeof(gPciedev_info));

    gPciedev_info.data_head_ptr = (struct pciedev_databuf_head *)gLocal_resv_buf;
    
    buf_init.numBuf = cmd_fifo_num;
    for(i = 0; i < cmd_fifo_num; i ++){
        buf_init.virtAddr[i] = local_cmdfifo_base + cmd_headinfo_size + cmd_fifo_size * i;
        debug_print(KERN_DEBUG "%s: cmdFifo map virtAddr %d-0x%lx", __func__, i, (unsigned long)buf_init.virtAddr[i]);
    }

    ret = pcie_buf_init(&gPciedev_info.recvBufObj.cmd_bufHndl, &buf_init);
    if(ret < 0){
        pr_err(DRIVER_NAME ":init cmd buffifo failed.\n");
        pcie_buf_deInit(&gPciedev_info.recvBufObj.cmd_bufHndl);
        goto ERROR_POST_BUFFER;
    }

    init_waitqueue_head(&gPciedev_info.cmd_wq_h);
        
#endif

    init_waitqueue_head(&readQ);
	sema_init(&sem_poll, 1);
	pr_info(DRIVER_NAME ":Initialization complete load successful [%s %s]\n", __TIME__, __DATE__);
	msi_on(device_ep);
	return 0;

ERROR_POST_BUFFER:

	device_destroy(ti81xx_ep_hlpr_class, ti81xx_ep_hlpr_dev);
	class_destroy(ti81xx_ep_hlpr_class);

ERROR_POST_CLASS:
#ifndef _DM81XX_EXCLUDE_
	iounmap((void *)gLocal_outboud_mem);
ERROR_POST_REMAP:
#endif
	cdev_del(&ti81xx_ep_hlpr_cdev);
	unregister_chrdev_region(ti81xx_ep_hlpr_dev, DEVICENO);
	return -1;
}


/*
 * exit part of module
 */


void ep_hlpr_exit(void)
{
#ifdef CONFIG_WISCOM
    pcie_buf_deInit(&gPciedev_info.cmd_bufHndl);
#endif
	kfree(mgmt_area_start);
	msi_off(device_ep);
	device_destroy(ti81xx_ep_hlpr_class, ti81xx_ep_hlpr_dev);
	class_destroy(ti81xx_ep_hlpr_class);
#ifndef _DM81XX_EXCLUDE_
	iounmap((void *)gLocal_outboud_mem);
#endif
	cdev_del(&ti81xx_ep_hlpr_cdev);
	unregister_chrdev_region(ti81xx_ep_hlpr_dev, DEVICENO);

	pr_info(DRIVER_NAME ": Exiting TI81XX_EP_HLPR _module\n");
}


module_init(ep_hlpr_init);
module_exit(ep_hlpr_exit);

MODULE_AUTHOR("Texas Instruments");
MODULE_LICENSE("GPL");
