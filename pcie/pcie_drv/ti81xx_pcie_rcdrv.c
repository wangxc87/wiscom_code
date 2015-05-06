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
static u32		pci_mem;
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
static struct  fasync_struct *ep_hlpr_async_queue = NULL;
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
		__raw_writel(0, pci_mem + offset + 0x54);//MSI_IRQ
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
	pci_mem = (u32)ioremap_nocache(PCI_NON_PREFETCH_START,
						PCI_NON_PREFETCH_SIZE);

	if (!pci_mem) {
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
	iounmap((void *)pci_mem);
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
	kfree(mgmt_area_start);
	msi_off(device_ep);
	device_destroy(ti81xx_ep_hlpr_class, ti81xx_ep_hlpr_dev);
	class_destroy(ti81xx_ep_hlpr_class);
	#ifndef _DM81XX_EXCLUDE_
	iounmap((void *)pci_mem);
	#endif
	cdev_del(&ti81xx_ep_hlpr_cdev);
	unregister_chrdev_region(ti81xx_ep_hlpr_dev, DEVICENO);

	pr_info(DRIVER_NAME ": Exiting TI81XX_EP_HLPR _module\n");
}


module_init(ep_hlpr_init);
module_exit(ep_hlpr_exit);

MODULE_AUTHOR("Texas Instruments");
MODULE_LICENSE("GPL");
