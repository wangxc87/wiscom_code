/**
 * TI81XX PCIe EP driver.
 *
 * Copyright (C) 2011 Texas Instruments, Incorporated
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


#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <asm/atomic.h>
#include <linux/interrupt.h>
#include<linux/workqueue.h>

#include "ti81xx_pcie_epdrv.h"
#include "pcie_common.h"

/* offsets of application registers  starting from 0x51000000 */
#define CMD_STATUS                      0x004
#define CFG_SETUP                       0x008
#define IB_BAR(x)                       (0x300 + (0x10 * x))
#define IB_START_LO(x)                  (0x304 + (0x10 * x))
#define IB_START_HI(x)                  (0x308 + (0x10 * x))
#define IB_OFFSET(x)                    (0x30c + (0x10 * x))
#define OB_SIZE                         0x30
#define CFG_PCIM_WIN_CNT                32
#define CFG_PCIM_WIN_SZ_IDX             3
#define OB_OFFSET_INDEX(n)              (0x200 + (8 * n))
#define OB_OFFSET_HI(n)                 (0x204 + (8 * n))
#define MSI_IRQ                         0x54
#define MSI0_IRQ_STATUS                 0x104
#define MSI0_IRQ_ENABLE_SET             0x108
#define MSI0_IRQ_ENABLE_CLR             0x10c
#define IRQ_EOI                         0x50
#define GPR0				0x70

/* these three application register are for alternate mechanism
 * to send interrupt from ep to rc
 */

#define EP_IRQ_SET                      0x64
#define EP_IRQ_CLR                      0x68
#define EP_IRQ_STATUS                   0x6c

/* these are for monitoring error status */
#define ERR_IRQ_STATUS_RAW              0x1c0
#define ERR_IRQ_STATUS                  0x1C4
#define ERR_IRQ_ENABLE_SET              0x1C8
#define ERR_IRQ_ENABLE_CLR              0x1CC

/*configuration register these are at offset 0x1000 from 0x51000000*/
#define STATUS_COMMAND                  0x4
#define VENDOR_DEVICE_ID                0x0
#define  BAR0                           0x10
#define  BAR1                           0x14
#define  BAR2                           0x18
#define  BAR3                           0x1C

/* msi capability registers-- these register are at 0x1000+0x50
 * from 0x51000000
 */

#define MSI_CAP                         0x0
#define MSI_LOW32                       0x4
#define MSI_UP32                        0x8
#define MSI_DATA                        0xC

#define MSI_OFF                         0x50

/* power management capability register-- thesew are at offset
 * 0x1000+0x40 from 0x51000000
 */

#define  PMCAP                          0x0
#define  PM_CTL_STAT                    0x4
/* add registers for error lokkup*/

/* other general macro related to driver*/

#define DEV_MINOR			0
#define DEVICENO			1
#define DRIVER_NAME			"ti81xx_ep_pcie"
#define IRQ_LINE			49
#define LOCAL_CONFIG_OFFSET		0x1000

#define CFG_PCIM_WIN_CNT		32
#define CFG_PCIM_WIN_SZ_IDX		3
#define PCIE_NON_PREFETCH_START		0x20000000
#ifdef CONFIG_WISCOM
#define PCIE_NON_PREFETCH_SIZE		(64 * 1024 * 1024)
#else
#define PCIE_NON_PREFETCH_SIZE		(256 * 1024 * 1024)
#endif

#define PCIE_REGS_START			0x51000000
#define PCIE_REGS_SIZE			(16 * 1024 * 1024)

#define TI81XX_PCIE_MODFILE		"ti81xx_pcie_ep"

#define TI816X_PCI_VENDOR_ID		0x104c
#define TI816X_PCI_DEVICE_ID		0xb800
#define SET_REGS			1
#define GET_REGS			2
#define MB				(1024 * 1024)
#define NMEM				1
#define INOB				2
#define FAIL				3
#define XCEEDREG			31

/* Used for locking of outbound configurations */
/* static DEFINE_SPINLOCK(ti81xx_pcie_ob_lock); */

/**
 * ti81xx_outb_info: this structure represents current outbound mappings info.
 * @list: to form a list using standard list methods in kernel
 * @ob_offset_hi: higher 32 bit address, as send in outbound setup
 * @ob_offset_idx: lower 32 bit address, as send in outbound setup
 * @start_region: region no. from which this mapping start
 * @regions: represents no of regions used
 * @size_req: size requested by this mapping.
 *
 * Each outbound related request, if successfull,  become part of a list
 * in which every node is of ti81xx_outb_info type.
 *
 * this information is helpfull when application request for clearing outbound
 * mapping done earlier.
 *
 * It is the responsibility of application to clear unused mapping.
 *
 */


struct ti81xx_outb_info {
	struct list_head list;
	u32 ob_offset_hi;
	u32 ob_offset_idx;
	u32 start_region;
	u32 regions;
	u32 size_req;
};

/**
 * start and end address of memory used by EP driver and EDMA driver
 */

extern u32 ti81xx_ep_mem_start;
extern u32 ti81xx_ep_mem_size;

dev_t ti81xx_ep_pcie_dev;
static struct cdev ti81xx_ep_pcie_cdev;
static u32 reg_vir;
static u16 device_id;
static struct resource *res_mem;
/* 6MB : default value of size to be reserved for
 * application buffers and MGMT area
 */
/* !@@@X static unsigned int resv_size = (6 * 1024 * 1024);*/
static struct class *ti81xx_pci_class;

/* status global variable.
 * represent status of outbound regions
 * each bit of this integer is correspond to a outbound region
 * if bit set-- free
 * else-- in use
 *
 * NOTE: Always keep the 32nd outbound window reserved for MSI.
 */

static u32 status_out;
static atomic_t         irq_raised = ATOMIC_INIT(0);
/* !@@@X static unsigned int *mgmt_area_start;*/
static unsigned int *gLocal_resv_buf;
static unsigned int *gLocal_outboud_buf;
static struct list_head outb_req;
wait_queue_head_t readq;
static struct semaphore sem_poll;
static atomic_t          counter = ATOMIC_INIT(0);

#ifdef CONFIG_WISCOM
struct fasync_struct *epdrv_async_queue = NULL;

static int data_fifo_num = DATA_BUF_FIFOS;//default 16
module_param(data_fifo_num, int , S_IRUGO);
MODULE_PARM_DESC(data_fifo_num,"The buf nums of dataFifo [default 16].");

static int data_fifo_size = DATA_BUF_SIZE;
module_param(data_fifo_size, int , S_IRUGO);
MODULE_PARM_DESC(data_fifo_size,"The buf size of dataFifo [default 1M].");

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

#endif
/* passed at module load time, default value is 6M */
/* !@@@X module_param(resv_size, uint , S_IRUGO);*/

/**
 * declaration of functions used
 */

static int ti81xx_set_inbound(struct ti81xx_inb_window *in);
static u32  ti81xx_check_regions(unsigned int  regions);
static u32 ti81xx_check_req_status(struct ti81xx_outb_region *out,
							u32 ob_size);
static int ti81xx_set_out_region(struct ti81xx_outb_region *out);
static int ti81xx_clr_outb_map(struct ti81xx_outb_region *out);
static int ti81xx_pciess_register_access(void __user *arg);
#if 0
/*** this will be supported only if driver is built staticaly in kernel ******/
static int ti816x_pcie_fault(unsigned long addr, unsigned int fsr,
							struct pt_regs *regs);
#endif
static int ti81xx_send_msi(void __user *argp);
static int ti81xx_ep_pcie_open(struct inode *inode, struct file *file);
static long ti81xx_ep_pcie_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg);
static unsigned int ti81xx_pcie_poll(struct file *filp, poll_table *wait);
static int ti81xx_ep_pcie_mmap(struct file *filp, struct vm_area_struct *vma);
static int ti81xx_ep_pcie_close(struct inode *inode, struct file *filp);
static int ti81xx_setup_msi(void);
static int query_bar_info(void __user *arg);
static irqreturn_t ti81xx_ep_pcie_handle(int irq, void *dev);

#ifdef CONFIG_WISCOM
static int ti81xx_ep_pcie_fasync(int fd, struct file *filp, int mode);
#endif
/**
 * ti81xx_set_inbound() - Setup inbound window
 * @in: address of struct ti81xx_inb_window
 *
 * _NOTE_: in-bound region are fixed for BARs
 * in case of 32 bit: PCIE ADDRESS HIGHER 32 BITS ARE 0
 *
 * IB_REGION 0: BAR1
 * IB_REGION 1: BAR2
 * IB_REGION 2: BAR3
 * IB_REGION 3: BAR4
 *
 * in case of 64 bit: BAR0 BAR2 BAR4 will be asssigned to IB_REGIONS
 *
 * IB_REGION 0: BAR0-BAR1: --BAR0
 * IB_REGION 1: BAR2-BAR3: --BAR2
 * IB_REGION 2: BAR4-BAR5: --BAR4
 */

static int ti81xx_set_inbound(struct ti81xx_inb_window *in)
{
	u32 ib_region;
	if (in->ib_start_hi == 0)/*32 bit mode*/
		ib_region = in->BAR_num-1;
	else/*64 bit mode*/
		ib_region = in->BAR_num;
	__raw_writel(in->internal_addr, reg_vir + IB_OFFSET(ib_region));
	__raw_writel(in->ib_start_hi, reg_vir + IB_START_HI(ib_region));
	__raw_writel(in->ib_start_lo, reg_vir + IB_START_LO(ib_region));
	__raw_writel(in->BAR_num, reg_vir + IB_BAR(ib_region));
	return 0;
}


/**
 * ti81xx_check_region()-check availability of continuous available regions.
 * @regions: no of continuous regions you are looking for.
 *
 * Returns 0-30 as the start of reagion(s) that can be allocated. Else, returns
 * 31 indicating unavailability of region(s).
 */

static u32  ti81xx_check_regions(unsigned int  regions)
{
	int i = 0, j = 0;
	for (i = 0; i <= 31; i++) {
		unsigned  count = 0;
		if (status_out & 1 << i) {
			count++;
			if (count == regions)
				return i;
			for (j = i + 1; j <= 31; j++) {
				if (status_out & 1 << j) {
					count++;
					if (count == regions)
						return i;
				} else
					break;
				}
		}
	}
	pr_debug(DRIVER_NAME ": outbound regions not available");
	return XCEEDREG;
}


/**
 * ti81xx_check_req_status()- this function check wether current request can
 * be served by increasing ob_size.
 * @out: pointer to struct ti81xx_outb_region that contain request detail.
 * @ob_size: current ob_size.
 *
 * This function checks wether current request can be served along with older
 * mapping by increasing OB_SIZE.
 * Return values:
 * NMEM: if not able to serve.
 * INOB: if can be served by increasing OB_SIZE.
 *
 * NOTE: We only conside 8MB (OB SIZTE = 3) as limit here since the other sizes
 * beyond that anyway don't occupy all the outbound windows.
 */

static u32 ti81xx_check_req_status(struct ti81xx_outb_region *out, u32 ob_size)
{
	if (ob_size < 3) {
		struct list_head *tmp;
		u32 reg_current = 0;
		u32 reg_req = 0;
		list_for_each(tmp, &outb_req) {
			struct ti81xx_outb_info *ptr =
						(struct ti81xx_outb_info *)tmp;
				if ((ptr->size_req) % (8 * MB) != 0)
					reg_current +=
						(ptr->size_req) / (8 * MB) + 1;
				else
					reg_current +=
						(ptr->size_req) / (8 * MB);
		}
		if ((out->size) % (8 * MB) != 0)
			reg_req += (out->size) / (8 * MB) + 1;
		else
			reg_req += (out->size) / (8 * MB);
		if ((reg_req + reg_current) <= 31)
			return INOB;
		else
			return NMEM;
	} else
		return NMEM;
}

/**
 * ti81xx_set_out_region()- serves request for outbound region mapping
 * to pcie address
 * @out: point to structure ti81xx_outb_region passed by user space to
 * do a outbound mapping.
 */

static int ti81xx_set_out_region(struct ti81xx_outb_region *out)
{

	u32 regions;
	u32 start_region;
	struct ti81xx_outb_info *info;
	struct list_head *tmp;
	u32 region_in_use = 0;
	u32 ob_size = __raw_readl(reg_vir + OB_SIZE);
	u32 region_size = (1 << ob_size) * MB;

	list_for_each(tmp, &outb_req) {
		region_in_use += ((struct ti81xx_outb_info *) tmp)->regions;
	}

	/* Also keep in mind that some regions may be disabled. */

	if ((out->size) % (region_size) != 0)
		regions = ((out->size) / (region_size)) + 1;
	else
		regions = ((out->size) / (region_size));

	if (!status_out || ((region_in_use + regions)  > 31))
		return ti81xx_check_req_status(out, ob_size);

	start_region = ti81xx_check_regions(regions);

	if (start_region < XCEEDREG) {
		u32 i, tr_size;
		u32 start_idx = out->ob_offset_idx;
		u32 hi_idx_add = out->ob_offset_hi;
		u32 max_lim = 0xFFFFFFFF;
		u32 carry = 0;
		tr_size = region_size;
		info = kmalloc(sizeof(struct ti81xx_outb_info), GFP_KERNEL);
		if (info  == NULL)
			return FAIL;

		info->ob_offset_hi = out->ob_offset_hi;
		info->ob_offset_idx = out->ob_offset_idx;
		info->start_region = start_region;
		info->regions = regions;
		info->size_req = out->size;
		list_add_tail(&(info->list), &outb_req);

		for (i = 0; (i < regions) ; i++) {
			__raw_writel(start_idx | 0x1,
				reg_vir + OB_OFFSET_INDEX((start_region + i)));
			__raw_writel(hi_idx_add,
				reg_vir + OB_OFFSET_HI((start_region + i)));
			if ((max_lim - tr_size) < start_idx)
				carry = 1;
			start_idx += tr_size;
			hi_idx_add += carry;
			carry = 0;
			status_out = status_out & (~(1 << (start_region + i)));
		}

	} else
		return ti81xx_check_req_status(out, ob_size);
	return 0;

}


/**
 * ti81xx_clr_outb_map()- clear outbound mapping, specified by application
 * @out- point to structure passed by user space to free specified mapping.
 *
 * It is application's responsibility to clear all the mappings.
 *
 * TODO: Probably need to add locking for the list/status manipulation
 */

static int ti81xx_clr_outb_map(struct ti81xx_outb_region *out)
{
	struct list_head *pos, *q;
	struct ti81xx_outb_info *temp;
	u32 i;
	u32 start;
	list_for_each_safe(pos, q, &outb_req) {
		 struct ti81xx_outb_info *ptr = (struct ti81xx_outb_info *)pos;
		 if ((ptr->ob_offset_hi == out->ob_offset_hi) &&
				(ptr->ob_offset_idx == out->ob_offset_idx)) {

			temp = list_entry(pos, struct ti81xx_outb_info, list);
			start = ptr->start_region;
			for (i = 0; (i < ptr->regions) ; i++) {
				__raw_writel(0,
					reg_vir + OB_OFFSET_INDEX(start + i));
				__raw_writel(0,
					reg_vir + OB_OFFSET_HI(start + i));
				status_out = status_out | ((1 << (start + i)));
			}
			list_del(pos);
			kfree(temp);
		}
	}
	return 0;
}

/**
 * free_outb_map()-- iterate over the list of successfull outbound requests
 * and free them.
 */

static int ti81xx_free_outb_map(void)
{
	struct list_head *pos, *q;
	struct ti81xx_outb_info *temp;
	list_for_each_safe(pos, q, &outb_req) {
		temp = list_entry(pos, struct ti81xx_outb_info, list);
		list_del(pos);
		kfree(temp);
	}
	return 0;
}


/**
 * ti81xx_pciess_register_access() - access pciess registers
 * @arg: Userspace pointer holding struct ti81xx_pciess_regs data.
 *		IN	: in case mode is write register
 *		INOUT	: in case mode is read register
 */

static int ti81xx_pciess_register_access(void __user *arg)
{
	struct ti81xx_pciess_regs regs;

	if (copy_from_user(&regs, arg, sizeof(regs)))
		return -EFAULT;

	if (regs.mode == SET_REGS)
		__raw_writel(regs.value, reg_vir + regs.offset);
	else
	{
		regs.value = __raw_readl(reg_vir + regs.offset);
		if(copy_to_user(arg, &regs, sizeof(regs)))
			return -EFAULT;
	}
	return 0;
}

/**
 * query_bar_info() - to get bar size and address from user space
 * @arg: Userspace pointer to struct ti81xx_bar_info structure
 *
 * This is INOUT parameter.
 *	IN	: BAR number
 *	OUT	: BAR info
 */

static int query_bar_info(void __user *arg)
{
	u32 addr;
	u32 offset = 4;
	struct ti81xx_bar_info bar_info;

	if (copy_from_user(&bar_info, arg, sizeof(bar_info)))
		return -EFAULT;

	addr = __raw_readl(reg_vir + LOCAL_CONFIG_OFFSET +
					BAR0 + (bar_info.bar_num) * offset);
	bar_info.bar_addr = addr & 0XFFFFFFF0;
	__raw_writel(0xFFFFFFFF, reg_vir + LOCAL_CONFIG_OFFSET +
					BAR0 + (bar_info.bar_num) * offset);
	bar_info.bar_size = __raw_readl(reg_vir + LOCAL_CONFIG_OFFSET +
					BAR0 + (bar_info.bar_num) * offset);
	bar_info.bar_size = ~(bar_info.bar_size & 0xFFFFFFF0) + 1;
	__raw_writel(addr, reg_vir + LOCAL_CONFIG_OFFSET +
					BAR0 + (bar_info.bar_num) * offset);

	if(copy_to_user(arg, &bar_info, sizeof(bar_info)))
		return -EFAULT;

	return 0;
}


/**
 * ti816x_pcie_fault() - ARM abort handler for PCIe nonposted completion aborts
 * @addr: Address target on which the fault generated
 * @fsr: CP15 fault status register value
 * @regs: Pointer to register structure on abort
 *
 * Handles precise abort caused due to PCIe operation.
 *
 * Note that we are relying on virtual address filtering to determine if the
 * target of the precise aborts was a PCIe module access (i.e., config, I/O,
 * register) and only handle such aborts. We could check PCIe error status to
 * confirm if the abort was caused due to non-posted completion status received
 * by PCIESS, but this may not always be true and aborts from some downstream
 * devices, such as PCI-PCI bridges etc may not result into error status bit
 * getting set.
 *
 * Ignores and returns abort as unhandled otherwise.
 *
 * Also note that, using error status check (as was done in earlier
 * implementation) would also handle failed memory accesses (non-posted), but
 * address filerting based handling will cause aborts for memory accesses as the
 * addresses will be outside the PCIESS module space. This seems OK, as any
 * memory access after enumeration is sole responsibility of the driver and the
 * system integrator (e.g., access failures due to hotplug, suspend etc). If
 * using error check based handling, we also need to clear PCIe error status on
 * detecting errors.
 *
 * Note: Due to specific h/w implementation, we can't be sure of what kind of
 * error occurred (UR Completion, CA etc) and all we get is raw error IRQ status
 * and probably SERR which indicate 'some kind of' error - fatal or non-fatal is
 * received/happened.
 */
#if 0
static int ti816x_pcie_fault(unsigned long addr, unsigned int fsr,
						struct pt_regs *regs)
{
	unsigned long instr = *(unsigned long *)regs->ARM_pc;

	pr_debug(DRIVER_NAME ": Data abort: address = 0x%08lx "
				"fsr = 0x%03x PC = 0x%08lx LR = 0x%08lx",
				addr, fsr, regs->ARM_pc, regs->ARM_lr);

	/* Note: Only handle PCIESS module space access */
	if ((addr < reg_vir) || (addr >= (reg_vir + SZ_16K)))
		return -1;

	/*
	* Mimic aborted read of all 1's as required to detect device/function
	* absence - check if the instruction that caused abort was a LOAD,
	*/
	if ((instr & 0x0c100000) == 0x04100000) {
		int reg = (instr >> 12) & 15;
		unsigned long val;

		if (instr & 0x00400000)
			val = 255;
		else
			val = -1;

		regs->uregs[reg] = val;
	}

	regs->ARM_pc += 4;

	pr_debug(DRIVER_NAME ": Handled PCIe abort\n");
	return 0;
}

#endif

/**
 * ti81xx_send_msi()-- send msi interrupt to RC/EP
 * @argp: User pointer to structure containing address and data for MSI
 * generation. Can be NULL when generating MSI to RC.
 *
 * Configures PCIESS registers so that it can send MSI to any RC or EP. In case
 * the msi_data is NULL, we use the MSI information configured by the RC to
 * generate MSI.
 *
 * Uses 32nd outbound window to generate MSI.
 */
static int ti81xx_send_msi(void __user *argp)
{
	u32 offset, rounded_addr_ls, msi_ob_addr;
	u32 ob_size = __raw_readl(reg_vir + OB_SIZE);
	u32 region_size = (1 << ob_size) * MB;
	struct ti81xx_msi_info msi;

	if (argp) {
		if (copy_from_user(&msi, argp, sizeof(msi)))
			return -EFAULT;
	} else {
		msi.msi_data = __raw_readl(reg_vir +
				LOCAL_CONFIG_OFFSET + MSI_DATA + MSI_OFF);
		msi.msi_addr_low = __raw_readl(reg_vir +
				LOCAL_CONFIG_OFFSET + MSI_LOW32 + MSI_OFF);
		msi.msi_addr_hi = __raw_readl(reg_vir +
				LOCAL_CONFIG_OFFSET + MSI_UP32 + MSI_OFF);
	}

	/*
	 * Arrive at rounded LS address & offset combination as we cannot
	 * configure exact outbound address translation.
	 */
	rounded_addr_ls = msi.msi_addr_low & ~(region_size - 1);
	offset = msi.msi_addr_low & (region_size - 1);

	msi_ob_addr = (u32) ioremap((0x20000000 + 31 * region_size),
						region_size);
	if (!msi_ob_addr)
		pr_err(DRIVER_NAME ":Outbound mapping for MSI failed");

	__raw_writel(msi.msi_addr_hi,  reg_vir + OB_OFFSET_HI(31));
	__raw_writel(rounded_addr_ls | 0x1, reg_vir + OB_OFFSET_INDEX(31));
	__raw_writel(msi.msi_data, msi_ob_addr + offset);

	iounmap((void *)msi_ob_addr);

	return 0;
}


static int ti81xx_ep_pcie_open(struct inode *inode, struct file *file)
{
	pr_debug(DRIVER_NAME ": file opened successfull");
	status_out = 0x7fffffff;
	atomic_set(&counter, 0);
#ifdef CONFIG_WISCOM

#endif
	return 0;
}
#ifdef CONFIG_WISCOM
extern int gPcieDev_id;
static struct pciedev_info gPciedev_info;
static int gSelf_id = 1;
struct dma_cnt_conf {
	int acnt;
	int bcnt;
	int ccnt;
	int mode;
};
static struct dma_cnt_conf gDma_cnt_conf = {
    .acnt = 128,
    .bcnt = 4096 * 2, //not fixed
    .ccnt = 1,
    .mode = 1,//AB-mode
};

extern int ti81xx_edma_rx(struct dma_cnt_conf *cnt, unsigned long dma_phys_src, unsigned long dma_phys_dest, u32 size);

static void pciedev_do_work(unsigned long arg);
#define WORK_QUEQUE_MODE
#ifdef WORK_QUEQUE_MODE
DECLARE_WORK(gPciedev_workque, (void *)pciedev_do_work);
#else
DECLARE_TASKLET(gPciedev_tasklet, pciedev_do_work, 0);
#endif
static int pcie_slave_sendAck(u32 cmd, int en_int)
{
    struct pciedev_databuf_head *gDatabuf_headPtr = gPciedev_info.data_head_ptr;
    gDatabuf_headPtr->ep_rd_flag[devid_to_index(gSelf_id)] |= cmd;

    if(en_int){
        //发送中断通知
        return ti81xx_send_msi(NULL);
    }
    return 0;    
}
static int put_dataBuf(struct pcie_bufHndl *bufHndl)
{
    char *buf_put = NULL;
    int buf_size = 0, ret;
    unsigned long src_phyaddr, dest_phyaddr;
    struct pciedev_databuf_head gDatabuf_headInfo;
    memcpy(&gDatabuf_headInfo, gPciedev_info.data_head_ptr, sizeof(struct pciedev_databuf_head));

#ifdef WORK_QUEQUE_MODE1

    if(wait_event_interruptible(gPciedev_info.data_wq_h, !pcie_buf_isFull(bufHndl)) < 0){
        printk(KERN_ERR "wait datafifo Empty event interrupte.\n");
        return pcie_slave_sendAck(DATA_ACK, FALSE);
    }

#else
    if(pcie_buf_isFull(bufHndl) == TRUE){
        debug_print("[pcie%d] recv dataBuf fifo is full, no free buf.\n", gSelf_id);
        return pcie_slave_sendAck(DATA_ACK, FALSE);
    }
#endif
    buf_put = pcie_buf_getEmtpy(bufHndl);
    if(!buf_put){
        debug_print("[pcie%d] get EmptyBuf failed.\n", gSelf_id);
        return    pcie_slave_sendAck(DATA_ACK_ERR, FALSE);
    }
    buf_size = gDatabuf_headInfo.buf_size;
    *buf_put = (char )gDatabuf_headInfo.buf_channal;

    src_phyaddr = PCIE_NON_PREFETCH_START + (unsigned long)gPciedev_info.recv_data - (unsigned long)gLocal_outboud_buf;
    
    dest_phyaddr = ti81xx_ep_mem_start + (unsigned long)buf_put - (unsigned long)gPciedev_info.recvBufObj.data_buf_base; 

#ifdef WORK_QUEQUE_MODE
#if 1
    ret = ti81xx_edma_rx(&gDma_cnt_conf, dest_phyaddr + 4, src_phyaddr, buf_size);
 #else
    ret = ti81xx_edma_rx(&gDma_cnt_conf, (unsigned long)virt_to_phys((unsigned long)buf_put + 4),
                         (unsigned long)virt_to_phys((unsigned long)gPciedev_info.recv_data), buf_size);
#endif
    if(ret != buf_size){
        pr_err("[pcie%d] ti81xx_edma_rx failed,return-%d.\n", gSelf_id,ret);
        goto put_data_err;        
    }
        
#else
    memcpy(buf_put + 4, (char *)gPciedev_info.recv_data, buf_size);
#endif
    ret = pcie_buf_setFull(bufHndl, buf_put, buf_size + 4);
    if(ret < 0){
        pr_err("[pcie%d] put FullBuf failed******.\n ", gSelf_id);
        goto put_data_err;
    }

    debug_print("%s: putData %u-%d.\n", __func__, gDatabuf_headInfo.frame_id, buf_size);

    return  pcie_slave_sendAck(DATA_ACK, FALSE);

 put_data_err:
    pcie_buf_setEmpty(bufHndl, buf_put);   
    pcie_slave_sendAck(DATA_ACK_ERR, FALSE);
    return  -1;
}

static int deQue_dataBuf(struct pcie_bufHndl *bufHndl, struct pciedev_buf_info *buf_info)
{
    char *buf_temp = NULL;
    u32 buf_size;

    /* wait_event_interruptible_timeout(gPciedev_info.data_wq_h,!pcie_buf_isEmpty(bufHndl), jiffies + 30*HZ); */
    wait_event_interruptible(gPciedev_info.data_wq_h,!pcie_buf_isEmpty(bufHndl));
    if(pcie_buf_isEmpty(bufHndl))
    {
        printk(KERN_DEBUG "%s: wait event timeout.\n", __func__);
        return -1;
    }

    buf_size = pcie_buf_getFull(bufHndl, &buf_temp);
    if(buf_size < 0){
        pr_debug("[pcie%d] get FullBuf failed.\n", gSelf_id);
        return -1;
    }

    debug_print( "%s: GET buf :ptr-0x%lx size:%u.\n",__func__, (unsigned long )buf_temp, (unsigned int)buf_size);
    
    buf_info->buf_kptr = buf_temp;
    buf_info->buf_ptr_offset = buf_temp - gPciedev_info.recvBufObj.data_buf_base;
    buf_info->buf_size = buf_size;
    return 0;
}

static int que_dataBuf(struct pcie_bufHndl *bufHndl, struct pciedev_buf_info *buf_info)
{
    int ret = 0;
    if (pcie_buf_setEmpty(bufHndl, buf_info->buf_kptr) <0)
        ret = -1;
    wake_up(&gPciedev_info.data_wq_h);
    return ret;
}


//cmd operation
static int put_cmdBuf(struct pcie_bufHndl *bufHndl)
{
    char *buf_put = NULL;
    int buf_size, ret;
    char *cmd_src = gPciedev_info.recv_cmd;

    if(wait_event_interruptible(gPciedev_info.cmd_wq_h, !pcie_buf_isFull(bufHndl)) < 0){
        printk(KERN_ERR "wait cmdfifo Empty event interrupte.\n");
        return pcie_slave_sendAck(CMD_ACK_ERR, FALSE);
    }

    buf_put = pcie_buf_getEmtpy(bufHndl);
    if(!buf_put){
        debug_print("[pcie%d] get cmd EmptyBuf failed.\n", gSelf_id);
        return pcie_slave_sendAck(CMD_ACK_ERR, FALSE);
    }

    buf_size = *(int *)cmd_src;
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

void pciedev_do_work(unsigned long arg)
{
    struct mgmt_info *gPciedev_mgmt_infoPtr = gPciedev_info.cmd_head_ptr;
    do {
        if((gPciedev_mgmt_infoPtr->cmd & SEND_CMD)== SEND_CMD){
            gPciedev_mgmt_infoPtr->cmd &= ~SEND_CMD;//clear SEND_CMD bit
            put_cmdBuf(&gPciedev_info.recvBufObj.cmd_bufHndl);
            wake_up(&gPciedev_info.cmd_wq_h);
        }
    
        if((gPciedev_mgmt_infoPtr->cmd & SEND_DATA)== SEND_DATA){
            gPciedev_mgmt_infoPtr->cmd &= ~SEND_DATA;//clear SEND_CMD bit
            put_dataBuf(&gPciedev_info.recvBufObj.data_bufHndl);
            wake_up(&gPciedev_info.data_wq_h);
        }
    }while((gPciedev_mgmt_infoPtr->cmd & SEND_CMD) || (gPciedev_mgmt_infoPtr->cmd & SEND_DATA));    
}

#endif
/**
 * ti81xx_pcie_ioctl() - Application interface for application
 *
 * Provides interface to the application code to initialize management area.
 * There are interfaces for setting in-bound and out-bound window as and when
 * needed. Application can query start address of dedecated physical area and
 * management area info. Application can also access/update the status of
 * different registers in PCIESS. There is an interface for MSI genertaion too.
 *
 * TODO: Move copy_*s inside respective handlers.
 */

static long ti81xx_ep_pcie_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case TI81XX_SET_INBOUND:
	{
		struct ti81xx_inb_window inb;
		if (copy_from_user(&inb, argp, sizeof(inb)))
			return -EFAULT;
		ti81xx_set_inbound(&inb);
	}
	break;

	case TI81XX_SET_OUTBOUND_SIZE:
	{
		unsigned int size;
		if (get_user(size, (unsigned int *)arg))
			return -EFAULT;
		__raw_writel(size, reg_vir + OB_SIZE);
	}
	break;

	case TI81XX_SET_OUTBOUND:
	{
		struct ti81xx_outb_region outb;
		if (copy_from_user(&outb, argp, sizeof(outb)))
			return -EFAULT;
		return ti81xx_set_out_region(&outb);
	}
	break;

	case TI81XX_ACCESS_REGS:
		ti81xx_pciess_register_access(argp);
	break;

	case TI81XX_SEND_MSI:
	{
		return ti81xx_send_msi(argp);
	}
	break;

	case TI81XX_GET_PCIE_MEM_INFO:
	{
		struct ti81xx_pcie_mem_info mem;
		mem.base = ti81xx_ep_mem_start;
		mem.size = ti81xx_ep_mem_size;
		if (copy_to_user(argp, &mem, sizeof(mem)))
			return -EFAULT;
	}
	break;

	case TI81XX_CUR_TIME:
		if (put_user(jiffies, (unsigned int *)arg))
			return -EFAULT;
	break;

	case TI81XX_GET_OUTBOUND_STATUS:
		if (put_user(status_out, (unsigned int *)arg))
			return -EFAULT;
	break;

	case TI81XX_CLR_OUTBOUND_MAP:
	{
		struct ti81xx_outb_region outb;
		if (copy_from_user(&outb, argp, sizeof(outb)))
			return -EFAULT;
		ti81xx_clr_outb_map(&outb);
	}
	break;

	case TI81XX_GET_INTR_CNTR:
	{
		int temp = atomic_read(&counter);
		if (put_user(temp, (int *)arg))
			return -EFAULT;
	}
	break;

	case TI81XX_GET_BAR_INFO:
		query_bar_info(argp);
	break;
#ifdef CONFIG_WISCOM
    case TI81XX_RESET_DATAQUE:
        {
            int ret = 0;
            ret = pcie_buf_reset(&gPciedev_info.recvBufObj.data_bufHndl);
            if(ret < 0)
                return ret;
        }
        break;
    case TI81XX_DEQUE_DATA:
        {
            struct pciedev_buf_info buf_info;
            int ret;
            if(!arg){
                pr_err("ioctrl: DEQUE_DATA invalid arg.\n");
                return -1;
            }
            ret = deQue_dataBuf(&gPciedev_info.recvBufObj.data_bufHndl, &buf_info);

            if(ret < 0){
                pr_err("Ioctrl: DEQUE_DATA  get data buf Error.\n");
                return -1;
            }

            if(copy_to_user((char *)argp, &buf_info, sizeof(struct pciedev_buf_info))){
                pr_err("IOctrl: DEQUE_DATA  copy_to_user failed.\n");
                return -1;
            }
        }
        break;
    case TI81XX_QUE_DATA:
        {
            struct pciedev_buf_info buf_info;
            if(!arg){
                pr_err("ioctrl: QUE_DATA invalid arg.\n");
                return -1;
            }
            if(copy_from_user(&buf_info, (char *)argp, sizeof(struct pciedev_buf_info)))        {
                pr_err("ioctrl: QUE_DATA copy_from_user failed.\n");
                return -1;
            }
            if(que_dataBuf(&gPciedev_info.recvBufObj.data_bufHndl, &buf_info) < 0){
                pr_err("deQue_dataBuf error.\n");
                return -1;
            }
        }
        break;
    case TI81XX_RESET_CMDQUE:
        {
            int ret = 0;
            ret = pcie_buf_reset(&gPciedev_info.recvBufObj.cmd_bufHndl);
            if(ret < 0)
                return ret;
        }
        break;
    case TI81XX_DEQUE_CMD:
        {
            struct pciedev_buf_info buf_info;
            int ret;
            if(!arg){
                pr_err("ioctrl: DEQUE_CMD invalid arg.\n");
                return -1;
            }
            ret = deQue_cmdBuf(&gPciedev_info.recvBufObj.cmd_bufHndl, &buf_info);

            if(ret < 0){
                pr_err("Ioctrl: DEQUE_CMD  get data buf Error.\n");
                return -1;
            }

            if(copy_to_user((char *)argp, &buf_info, sizeof(struct pciedev_buf_info))){
                pr_err("IOctrl: DEQUE_CMD  copy_to_user failed.\n");
                return -1;
            }
        }
        break;
    case TI81XX_QUE_CMD:
        {
            struct pciedev_buf_info buf_info;
            if(!arg){
                pr_err("ioctrl: QUE_CMD invalid arg.\n");
                return -1;
            }
            if(copy_from_user(&buf_info, (char *)argp, sizeof(struct pciedev_buf_info)))        {
                pr_err("ioctrl: QUE_CMD copy_from_user failed.\n");
                return -1;
            }
            if(que_cmdBuf(&gPciedev_info.recvBufObj.cmd_bufHndl, &buf_info) < 0){
                pr_err("deQue_cmdBuf error.\n");
                return -1;
            }
        }
        break;
    case TI81XX_GET_SENDCMD_INFO:
        {
            struct pciedev_buf_info buf_info;
            if(!arg){
                pr_err("ioctrl: QUE_CMD invalid arg.\n");
                return -1;
            }
            buf_info.buf_ptr_offset = (char *)gPciedev_info.send_cmd - (char *)gLocal_resv_buf;
            buf_info.buf_size = cmd_fifo_size;

            if(copy_to_user((char *)argp, &buf_info, sizeof(struct pciedev_buf_info))){
                pr_err("IOctrl: DEQUE_CMD  copy_to_user failed.\n");
                return -1;
            }
        }
        break;
    case TI81XX_GET_SENDDATA_INFO:
        {
            struct pciedev_buf_info buf_info;
            if(!arg){
                pr_err("ioctrl: QUE_CMD invalid arg.\n");
                return -1;
            }
            buf_info.buf_size = data_fifo_size;

            if(copy_to_user((char *)argp, &buf_info, sizeof(struct pciedev_buf_info))){
                pr_err("IOctrl: DEQUE_CMD  copy_to_user failed.\n");
                return -1;
            }
        }
        break;        
#endif
	default:
		pr_err("Invalid cmd passed\n");
	}

	return 0;
}

/**
 * ti81xx_pcie_poll()-- this function supports poll call from user space.
 *
 * on receving interrupt from other EP/RC, it wake up and
 * check data availabilty, if data is available for reading
 * it return POLLIN otherwise it return 0.
 *
 */

static unsigned int ti81xx_pcie_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	down(&sem_poll);
	poll_wait(filp, &readq, wait);
	if (atomic_read(&irq_raised) == 1)
		mask |= POLLIN;    /* readable */
	atomic_set(&irq_raised, 0);
	up(&sem_poll);
	return mask;
}


/**
 * ti81xx_ep_pcie_mmap() - Provide userspace mapping for specified kernelmemory
 *
 * @filp: File private data
 * @vma: User virtual memory area to map to
 *
 * At present, we are mapping entire physical contiguous dedicated area for EP
 * and pci space(0x20000000-0x2fffffff).
 * application has to know start address of dedicated area, it can query about
 * it, using specified ioctl.
 */

static int ti81xx_ep_pcie_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = -EINVAL;
	unsigned long sz = vma->vm_end - vma->vm_start;
	unsigned int addr = (unsigned int)vma->vm_pgoff << PAGE_SHIFT;
	pr_debug(DRIVER_NAME ":Mapping %#lx bytes from address %#x\n"
			, sz, addr);
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	ret = remap_pfn_range(vma, vma->vm_start,
					vma->vm_pgoff,
						sz, vma->vm_page_prot);
	return ret;
}

/**
 * ti81xx_ep_pcie_close()-- free all the successfull outbound request and set regios
 * status to default.
 */

static int ti81xx_ep_pcie_close(struct inode *inode, struct file *filp)
{
	/* free all list of outbound mapping*/
	ti81xx_free_outb_map();
    ti81xx_ep_pcie_fasync(-1, filp, 0);
    epdrv_async_queue = NULL;
	return 0;
}


/**
 * ti81xx_setup_msi() - it sets up msi related configuration.
 *
 * _NOTE_:  disable legacy interrupt and enable MSi capability.
 *
 * __TO_DO__:
 */

static int ti81xx_setup_msi()
{
	__raw_writel(0x0, reg_vir + 0x18c);//IRQ_ENABLE_CLR
	__raw_writel(__raw_readl(reg_vir + STATUS_COMMAND +
			LOCAL_CONFIG_OFFSET) | (1 << 10),
			reg_vir + STATUS_COMMAND + LOCAL_CONFIG_OFFSET);
	__raw_writel(__raw_readl(reg_vir + LOCAL_CONFIG_OFFSET +
			MSI_CAP + MSI_OFF) | (1 << 16),
			reg_vir + LOCAL_CONFIG_OFFSET + MSI_CAP + MSI_OFF);
	__raw_writel(0xFFFFFFFF, reg_vir + MSI0_IRQ_ENABLE_SET);
	pr_debug(DRIVER_NAME ":msi capability setup done");
	return 0;
}


/**
 * ti81xx_ep_pcie_handle()- interrupt handler
 *
 * notify about data in buffers, and signal select/poll
 * implementation about data availability.
 */

static irqreturn_t ti81xx_ep_pcie_handle(int irq, void *dev)
{
	int i = 0;
	while (i <= 1500) {
		if (__raw_readl(reg_vir + MSI0_IRQ_STATUS) != 0) {
			if (device_id >= 0xb801)
				__raw_writel(0xFFFFFFFF,
					reg_vir + MSI0_IRQ_STATUS);
			if (device_id == 0xb800)
				__raw_writel(0x0,
					reg_vir + MSI0_IRQ_STATUS);
			__raw_writel(1, reg_vir + IRQ_EOI);
			atomic_set(&irq_raised, 1);
			atomic_inc(&counter);
			wake_up_interruptible(&readq);
#ifdef CONFIG_WISCOM
#ifdef WORK_QUEQUE_MODE
                schedule_work(&gPciedev_workque);
#else
                tasklet_schedule(&gPciedev_tasklet);
#endif
            if(epdrv_async_queue)
                kill_fasync(&epdrv_async_queue, SIGIO, POLL_IN);
#endif
		}
		i++;
	}
	return IRQ_HANDLED;
}

#ifdef CONFIG_WISCOM
static int ti81xx_ep_pcie_fasync(int fd, struct file *filp, int mode)
{
    return fasync_helper(fd, filp, mode, &epdrv_async_queue);
}
#endif

/**
 * ti81xx_pci_fops - Declares supported file access functions
 */
static const struct file_operations ti81xx_ep_pcie_fops = {
	.owner          = THIS_MODULE,
	.mmap           = ti81xx_ep_pcie_mmap,
	.open           = ti81xx_ep_pcie_open,
	.release        = ti81xx_ep_pcie_close,
	.poll           = ti81xx_pcie_poll,
	.unlocked_ioctl = ti81xx_ep_pcie_ioctl,
#ifdef CONFIG_WISCOM
    .fasync = ti81xx_ep_pcie_fasync,
#endif
};


/**
 * ti81xx_ep_pci_init() - init part of driver.
 *
 * get necessary resources.
 */

static int ti81xx_ep_pcie_init(void)
{
	int ret = 0;
    int i = 0;
    struct kfifo_buf_create_arg buf_init;
    u32 data_fifo_total_size = data_fifo_num * data_fifo_size;
    u32 cmd_fifo_total_size = cmd_fifo_num * cmd_fifo_size;
    u32 cmd_headinfo_size = sizeof(struct mgmt_info) + cmd_fifo_size*2;
    
    if (!ti81xx_ep_mem_size)
		pr_warn(DRIVER_NAME ": *** WARNING: "
				"No memory reserved for PCIe transfers.\n\t"
				"Append pcie_mem<size> to kernel command line "
				"to use mmap/DMA on reserved memory.\n");

    if(data_fifo_total_size + RSVMEM_DATA_BUF_OFFSET > ti81xx_ep_mem_size){
        printk(KERN_ERR "Error: request data fifo size is too large, more than %u", ti81xx_ep_mem_size - RSVMEM_DATA_BUF_OFFSET);
        return -1;
    }

    if(cmd_fifo_total_size + cmd_headinfo_size > RSVMEM_CMD_BUF_SIZE){
        printk(KERN_ERR "Error: request cmd fifo size is too large, more than %u", RSVMEM_CMD_BUF_SIZE - cmd_headinfo_size);
        return -1;
        
    }
        
	ret = alloc_chrdev_region(&ti81xx_ep_pcie_dev, DEV_MINOR, DEVICENO,
							TI81XX_PCIE_MODFILE);
	if (ret < 0) {

		pr_err(DRIVER_NAME ": could not allocate the "
							"character driver\n");
		return -1;
	}

	cdev_init(&ti81xx_ep_pcie_cdev, &ti81xx_ep_pcie_fops);
	ret = cdev_add(&ti81xx_ep_pcie_cdev, ti81xx_ep_pcie_dev, DEVICENO);
	if (ret < 0) {
		pr_err(DRIVER_NAME ": cdev add failed\n");
		unregister_chrdev_region(ti81xx_ep_pcie_dev, DEVICENO);
		return -1;
	}

	reg_vir = (u32)ioremap_nocache(PCIE_REGS_START, PCIE_REGS_SIZE);
	if (!reg_vir) {
		pr_err(DRIVER_NAME ": register remapping failed\n");
		cdev_del(&ti81xx_ep_pcie_cdev);
		unregister_chrdev_region(ti81xx_ep_pcie_dev, DEVICENO);
		return -1;
	}

	res_mem = (struct resource *)request_mem_region(PCIE_NON_PREFETCH_START,
				PCIE_NON_PREFETCH_SIZE, "pcie-nonprefetch");
	if (!res_mem) {
		pr_err(DRIVER_NAME ":request-mem failed in pcie prefetch\n");
		iounmap((void *)reg_vir);
		cdev_del(&ti81xx_ep_pcie_cdev);
		unregister_chrdev_region(ti81xx_ep_pcie_dev, DEVICENO);
		return -1;
	}

	gLocal_outboud_buf = (u32 *) ioremap_nocache(PCIE_NON_PREFETCH_START,
						PCIE_NON_PREFETCH_SIZE);
	if (!gLocal_outboud_buf) {
		pr_err(DRIVER_NAME ": PCIE_NON_PREFETCH remapping failed\n");
		iounmap((void *)reg_vir);
		release_mem_region(PCIE_NON_PREFETCH_START,
						PCIE_NON_PREFETCH_SIZE);
		cdev_del(&ti81xx_ep_pcie_cdev);
		unregister_chrdev_region(ti81xx_ep_pcie_dev, DEVICENO);
		return -1;
	}
    gLocal_resv_buf = (unsigned int *) ioremap_nocache(ti81xx_ep_mem_start,
								ti81xx_ep_mem_size);
	if (!gLocal_resv_buf) {
		pr_err(DRIVER_NAME ": mgmt_area remapping in driver failed\n");
		iounmap((void *)reg_vir);
		iounmap((void *)gLocal_outboud_buf);
		release_mem_region(PCIE_NON_PREFETCH_START,
						PCIE_NON_PREFETCH_SIZE);
		cdev_del(&ti81xx_ep_pcie_cdev);
		unregister_chrdev_region(ti81xx_ep_pcie_dev, DEVICENO);
		return -1;
	}

	pr_info(DRIVER_NAME ": Memory chunk used by EP DRIVER:start addr "
				"0%x size 0%x\n",
				ti81xx_ep_mem_start, ti81xx_ep_mem_size);

	if (request_irq(IRQ_LINE, ti81xx_ep_pcie_handle, IRQF_SHARED,
			TI81XX_PCIE_MODFILE, &ti81xx_ep_pcie_cdev) != 0) {
		pr_err(DRIVER_NAME ":interrupt registration failed\n");
		iounmap((void *)reg_vir);
		iounmap((void *)gLocal_outboud_buf);
		iounmap((void *)gLocal_resv_buf);
		release_mem_region(PCIE_NON_PREFETCH_START,
					PCIE_NON_PREFETCH_SIZE);
		cdev_del(&ti81xx_ep_pcie_cdev);
		unregister_chrdev_region(ti81xx_ep_pcie_dev, DEVICENO);
		return -1;
	}

	ti81xx_pci_class = class_create(THIS_MODULE, TI81XX_PCIE_MODFILE);
	if (!ti81xx_pci_class) {
		pr_err(DRIVER_NAME ":failed to add device to sys fs");
        goto ep_init_err;
	}
	
    device_create(ti81xx_pci_class, NULL, ti81xx_ep_pcie_dev, NULL,
						TI81XX_PCIE_MODFILE);
    
#ifdef CONFIG_WISCOM

    gSelf_id = gPcieDev_id;
    memset(&gPciedev_info, 0, sizeof(gPciedev_info));
    gPciedev_info.dev_id = gPcieDev_id;
    gPciedev_info.recv_data = (char *)gLocal_outboud_buf + sizeof(struct pciedev_databuf_head);
    gPciedev_info.cmd_head_ptr = (struct mgmt_info *)gLocal_resv_buf;
    gPciedev_info.data_head_ptr = (struct pciedev_databuf_head *)gLocal_outboud_buf;
    gPciedev_info.recv_cmd = (char *)gLocal_resv_buf + sizeof(struct mgmt_info);
    gPciedev_info.send_cmd = (char *)gLocal_resv_buf + sizeof(struct mgmt_info) + cmd_fifo_size;
    
    buf_init.numBuf = data_fifo_num;
    for(i = 0; i < data_fifo_num; i ++){
        buf_init.virtAddr[i] = (char *)gLocal_resv_buf + RSVMEM_DATA_BUF_OFFSET + data_fifo_size * i;
        debug_print(KERN_DEBUG "%s: dataFifo map virtAddr %d-0x%lx", __func__, i, (unsigned long)buf_init.virtAddr[i]);
    }
    
    ret = pcie_buf_init(&gPciedev_info.recvBufObj.data_bufHndl, &buf_init);
    if(ret < 0){
        pr_err(DRIVER_NAME ":init data buffifo failed.\n");
        pcie_buf_deInit(&gPciedev_info.recvBufObj.data_bufHndl);
        goto ep_init_err;
    }
    gPciedev_info.recvBufObj.data_buf_base = (char *)gLocal_resv_buf;

    buf_init.numBuf = cmd_fifo_num;
    for(i = 0; i < cmd_fifo_num; i ++){
        buf_init.virtAddr[i] = (char *)gLocal_resv_buf + cmd_headinfo_size + cmd_fifo_size * i;
        debug_print(KERN_DEBUG "%s: cmdFifo map virtAddr %d-0x%lx", __func__, i, (unsigned long)buf_init.virtAddr[i]);
    }
    ret = pcie_buf_init(&gPciedev_info.recvBufObj.cmd_bufHndl, &buf_init);
    if(ret < 0){
        pr_err(DRIVER_NAME ":init cmd buffifo failed.\n");
        pcie_buf_deInit(&gPciedev_info.recvBufObj.data_bufHndl);
        pcie_buf_deInit(&gPciedev_info.recvBufObj.cmd_bufHndl);
        goto ep_init_err;
    }
    gPciedev_info.recvBufObj.cmd_buf_base = (char *)gLocal_resv_buf;
    
    
    init_waitqueue_head(&gPciedev_info.data_wq_h);
    init_waitqueue_head(&gPciedev_info.cmd_wq_h);

#endif
	ti81xx_setup_msi();
	INIT_LIST_HEAD(&outb_req);

	/*
	 * set all regions available
	 */

	status_out = 0x7fffffff;
	init_waitqueue_head(&readq);
	sema_init(&sem_poll, 1);
	device_id = (__raw_readl(reg_vir + LOCAL_CONFIG_OFFSET +
						VENDOR_DEVICE_ID)) >> 16;


	pr_info(DRIVER_NAME ":EP%d Initialization complete load successful [%s %s]\n",gSelf_id, __TIME__, __DATE__);

	return 0;

 ep_init_err:
    iounmap((void *)reg_vir);
    iounmap((void *)gLocal_outboud_buf);
    iounmap((void *)gLocal_resv_buf);
    release_mem_region(PCIE_NON_PREFETCH_START,
                       PCIE_NON_PREFETCH_SIZE);
    cdev_del(&ti81xx_ep_pcie_cdev);
    unregister_chrdev_region(ti81xx_ep_pcie_dev, DEVICENO);
    return -1;

}


/**
 * ti81xx_ep_pci_exit() - exit part of driver.
 *
 * release resources.
 */

static void ti81xx_ep_pcie_exit(void)
{
	free_irq(IRQ_LINE, &ti81xx_ep_pcie_cdev);
	device_destroy(ti81xx_pci_class, ti81xx_ep_pcie_dev);
	class_destroy(ti81xx_pci_class);
#ifdef CONFIG_WISCOM
    pcie_buf_deInit(&gPciedev_info.recvBufObj.data_bufHndl);
    pcie_buf_deInit(&gPciedev_info.recvBufObj.cmd_bufHndl);
#endif
	iounmap((void *)reg_vir);
	iounmap((void *)gLocal_outboud_buf);
    iounmap((void *)gLocal_resv_buf);
	release_mem_region(PCIE_NON_PREFETCH_START, PCIE_NON_PREFETCH_SIZE);
	cdev_del(&ti81xx_ep_pcie_cdev);
	unregister_chrdev_region(ti81xx_ep_pcie_dev, DEVICENO);
	pr_info(DRIVER_NAME ":module unload successful\n");
}

module_init(ti81xx_ep_pcie_init);
module_exit(ti81xx_ep_pcie_exit);
MODULE_LICENSE("GPL");
