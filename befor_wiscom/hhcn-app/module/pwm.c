/**********************************
 *
 * Hefei Huaheng Electronic Co, LTD
 *
 * Created by yejq, OLD
 *
 * PWM motor device driver
 *
 * 2013 年 09 月 09 日
 *********************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL v2");

#define TIMER_5_BASEADDR                   0x48046000    /* TIMER5 Peripheral Registers */
#define TIMER_5_SUPPADDR                   0x48047000    /* TIMER5 Support Registers */
#define TIMER_6_BASEADDR                   0x48048000    /* TIMER6 Peripheral Registers */
#define TIMER_6_SUPPADDR                   0x48490000    /* TIMER6 Support Registers */
#define TIMER_REGISTERS_MAP_SIZE           0x00001000

#define PWM_OUT(val, addr) (iowrite32(val, addr))
#define PWM_IN(addr)       (ioread32(addr))


#define DEVICE_NAME                        "pwm-motor"
#define MOTOR_SELE_LEFT                     0x04
#define MOTOR_SELE_RIGHT                    0x08
#define MOTOR_SELECTION_MASK                (MOTOR_SELE_LEFT | MOTOR_SELE_RIGHT)
#define MOTOR_CONTROL_LEFT                  0x01
#define MOTOR_CONTROL_RIGHT                 0x02

#define GPIO_0_BASEADDR                     0x48032000
#define GPIO_0_MMAPSIZE                     0x00000200
#define PIN_CONTROL_BASE                    0x48140000
#define PIN_CONTROL_MMAPSIZE                0x00001000

#define PWM_MOTOR_IOCTL_BASE               'M'
#define PWM_MOTOR_RESET_CMD                _IO(PWM_MOTOR_IOCTL_BASE, 0)
#define PWM_MOTOR_SET_DIRECTION_CMD        _IO(PWM_MOTOR_IOCTL_BASE, 1)
#define PWM_MOTOR_GET_COUNTER_CMD          _IO(PWM_MOTOR_IOCTL_BASE, 2)
#define PWM_MOTOR_START_CMD                _IO(PWM_MOTOR_IOCTL_BASE, 3)
#define PWM_MOTOR_STOP_CMD                 _IO(PWM_MOTOR_IOCTL_BASE, 4)
#define PWM_MOTOR_STATE_CMD                _IO(PWM_MOTOR_IOCTL_BASE, 5)
#define PWM_MOTOR_TLDR_CMD                 _IO(PWM_MOTOR_IOCTL_BASE, 6)

struct timer_regs {
	u32            tidr;              /* 0x00 Identification Register */
	u32            res0[0x03];
	u32            tiocp_cfg;         /* 0x10 Timer OCP Configuration Register */
	u32            res1[0x03];        
	u32            irq_eoi;           /* 0x20 Timer IRQ End-Of-Interrupt Register */
	u32            irqstatus_raw;     /* 0x24 Timer IRQSTATUS Raw Register */
	u32            irqstatus;         /* 0x28 Timer IRQSTATUS Register */
	u32            irqenable_set;     /* 0x2c Timer IRQENABLE Set Register */
	u32            irqenable_clr;     /* 0x30 Timer IRQENABLE Clear Register */
	u32            irqwakeen;         /* 0x34 Timer IRQ Wakeup Enable Register */
	u32            tclr;              /* 0x38 Timer Control Register */
	u32            tcrr;              /* 0x3c Timer Counter Register */
	u32            tldr;              /* 0x40 Timer Load Register */
	u32            ttgr;              /* 0x44 Timer Trigger Register */
	u32            twps;              /* 0x48 Timer Write Posted Status Register */
	u32            tmar;              /* 0x4c Timer Match Register */
	u32            tcar1;             /* 0x50 Timer Capture Register */
	u32            tsicr;             /* 0x54 Timer Synchronous Interface Control Register */
	u32            tcar2;             /* 0x58 Timer Capture Register */
};

enum motor_state {
	MOTOR_STOPPED,
	MOTOR_NOTRUNNING,
	MOTOR_RUNNING
};

enum motor_direction {
	MOTOR_DIRECTION_LEFT    = 0,
	MOTOR_DIRECTION_RIGHT   = 1,
	MOTOR_DIRECTION_NONE /* never used */
};

struct pwm_motor {
	struct cdev          motor_device;
	dev_t                device_no;
	struct timer_regs *  regs_right;
	struct timer_regs *  regs_left;
	enum motor_state     state;
	u32                  tldr;
	u32                  tldr_4;
	enum motor_direction direct;
	u32                  tclr;
	u32                  tmar_l;  /* match registers currently are not used, but maybe later */
	u32                  tmar_r;  /* so leave it here */

	u32               *  gpio_control;
	atomic_t             users;
	struct class      *  motor_class;
	struct device     *  dev;
	struct inode      *  inodp;
	struct file       *  filp;
};

static struct pwm_motor pwm_motor_device;

static int pwm_motor_open(struct inode * pinode, struct file * pfile)
{
	struct pwm_motor * motor;
	motor = &pwm_motor_device;
	
	if (atomic_inc_return(&motor->users) >= 0x02) {
		atomic_dec(&motor->users);
		return -EBUSY;
	}
	motor->inodp = pinode;
	motor->filp  = pfile;
	return 0;
}

static int pwm_motor_release(struct inode * pin, struct file * pfile)
{
	struct pwm_motor * motor;
	motor = &pwm_motor_device;
	if (atomic_read(&motor->users) > 0)  {
		atomic_dec(&motor->users);
		
		motor->inodp = NULL;
		motor->filp  = NULL;
	}
	return 0;
}

/* initialize timer registers */
static int init_pwm_motor(struct timer_regs * regs, u32 tldr, u32 tmar)
{
	u32 regvalue;
	printk(KERN_INFO "TIDR: %08X: TCLR: %08X\n", PWM_IN(&regs->tidr), PWM_IN(&regs->tclr));
	/* first disable all interrupt */
	PWM_OUT(0, &regs->irqenable_set);

	/* set tiocp_cfg register to smart-idle, frozen in emulation mode */
	PWM_OUT(0x02, &regs->tiocp_cfg);

	/* stop the timer */
	PWM_OUT(0, &regs->tclr);

	/* set timer counter value loaded on overflow */
	PWM_OUT(tldr, &regs->tldr);
	PWM_OUT(tmar, &regs->tmar);
	PWM_OUT(0xFFFFFFFF, &regs->ttgr);

	/* set Timer Control Register, refer to page 2673 */
	regvalue = (1 << 12) | (1 << 10) | (1 << 7) | (1 << 5) | (1 << 1); 
	PWM_OUT(regvalue, &regs->tclr);

	return 0;
}

static void reset_pwm_motor(struct timer_regs * regs, u32 tldr, u32 tmar)
{
	PWM_OUT(0x02, &regs->tsicr);         /* enable software reset */
	PWM_OUT(1, &regs->tiocp_cfg);        /* reset module */
	init_pwm_motor(regs, tldr, tmar);          /* initialize module */
}

static void control_direct(struct pwm_motor * motor)
{
	u32 value, tclr, tldr, tldr_4;
	struct timer_regs * regs0, * regs1;

	tldr      = motor->tldr;
	tldr_4    = motor->tldr_4;
	tclr      = motor->tclr;

	if (motor->direct) {
		regs0    = motor->regs_right;
		regs1    = motor->regs_left;
	} else {
		regs0    = motor->regs_left;
		regs1    = motor->regs_right;
	}

	/* stop the motor */
	PWM_OUT(0, &regs0->tclr);
	PWM_OUT(0, &regs1->tclr);

	/* update TLDR if necessary */
	value = PWM_IN(&regs0->tldr);
	if (value != tldr) 
		PWM_OUT(tldr, &regs0->tldr);
	
	value = PWM_IN(&regs1->tldr);
	if (value != tldr) 
		PWM_OUT(tldr, &regs1->tldr);

	/* load the current counter value */
	PWM_OUT(tldr_4, &regs0->tcrr);
	PWM_OUT(tldr, &regs1->tcrr);

	/* run the motor */
	value = tclr | 0x01;
	PWM_OUT(value, &regs0->tclr);
	PWM_OUT(value, &regs1->tclr);
}

static long pwm_motor_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{
	long ret;
	u32 value, argv;
	struct pwm_motor * motor;

	ret = 0;
	motor = &pwm_motor_device;

	switch (cmd) {
	case PWM_MOTOR_RESET_CMD:
		reset_pwm_motor(motor->regs_right, motor->tldr, motor->tmar_r);
		reset_pwm_motor(motor->regs_left, motor->tldr, motor->tmar_l);
		break;

	case PWM_MOTOR_SET_DIRECTION_CMD:
		if (!copy_from_user(&argv, (void *) arg, sizeof(argv))) {
			argv &= (MOTOR_CONTROL_LEFT | MOTOR_CONTROL_RIGHT);
			if (!argv || (argv == (MOTOR_CONTROL_LEFT | MOTOR_CONTROL_RIGHT))) {
				ret = -EINVAL;
				break;
			}

			if (argv & MOTOR_CONTROL_LEFT) {
				motor->direct = MOTOR_DIRECTION_LEFT;
			} else {
				motor->direct = MOTOR_DIRECTION_RIGHT;
			}
			control_direct(motor);
		} else {
			ret = -EINVAL;
		}
		break;
		
	case PWM_MOTOR_GET_COUNTER_CMD: {
		u32 counters[0x02];
		counters[0] = PWM_IN(&motor->regs_right->tcrr);
		counters[1] = PWM_IN(&motor->regs_left->tcrr);
		if (!copy_to_user((void *) arg, counters, sizeof(counters))) {
		} else {
			printk(KERN_ERR "Invalid user space pointer: %p\n", (void *) arg);
			ret = -EINVAL;
		}
	}
	break;

	case PWM_MOTOR_START_CMD:
		value = 1 << 18;
		PWM_OUT(value, motor->gpio_control + (0x194 >> 2));
		motor->state = MOTOR_RUNNING;
		break;

	case PWM_MOTOR_STOP_CMD:
		value = 1 << 18;
		PWM_OUT(0, &motor->regs_left->tclr);
		PWM_OUT(0, &motor->regs_right->tclr);
		PWM_OUT(value, motor->gpio_control + (0x190 >> 2));
		motor->state = MOTOR_STOPPED;
		break;

	case PWM_MOTOR_STATE_CMD: {
		u32 retval;
		retval = 0;
		if (PWM_IN(&motor->regs_left->tclr) & 1) {
			retval |= MOTOR_CONTROL_LEFT;
		}

		if (PWM_IN(&motor->regs_right->tclr) & 1) {
			retval |= MOTOR_CONTROL_RIGHT;
		}

		if (!retval && motor->state != MOTOR_STOPPED)
			motor->state = MOTOR_NOTRUNNING;

		if (!copy_to_user((void *) arg, &retval, sizeof(retval))) {
		} else {
			ret = -EINVAL;
		}
	}
	break;

	case PWM_MOTOR_TLDR_CMD:
		if (!copy_from_user(&argv, (void *) arg, sizeof(argv))) {
			if (argv >= 0xFFFFFFF0) {
				ret = -EINVAL;
				break;
			}
			motor->tldr      = argv;
			motor->tldr_4    = argv + ((~ argv) >> 1);
			motor->tclr      = (1 << 12) | (1 << 10) | (1 << 7) | (1 << 5) | (1 << 1);
			motor->tmar_l    = argv + (~ argv) / 3;
			motor->tmar_r    = argv + 2 * (~ argv) / 3;
		} else {
			ret = -EINVAL;
		}
		break;
		
	default :
		printk(KERN_ERR "Unknown pwm_motor_ioctl cmd: %08x\n", cmd);
		ret = -EINVAL;
		break;
	}

	return ret;	
}

static const struct file_operations pwm_ops = {
	.owner             = THIS_MODULE,
	.open              = pwm_motor_open,
	.unlocked_ioctl    = pwm_motor_ioctl,
	.release           = pwm_motor_release,
};

static void __exit pwm_motor_exit(void)
{
	struct pwm_motor * ppwm;
	u32 value;
	
	printk(KERN_INFO "In [%s]\n", __func__);
	ppwm = &pwm_motor_device;

	value = 1 << 18;
	PWM_OUT(value, ppwm->gpio_control + (0x190 >> 2));
	PWM_OUT(0, &ppwm->regs_left->tclr);
	PWM_OUT(0, &ppwm->regs_right->tclr);

	iounmap(ppwm->regs_right);
	iounmap(ppwm->regs_left);
	iounmap(ppwm->gpio_control);
	device_destroy(ppwm->motor_class, ppwm->device_no);
	class_destroy(ppwm->motor_class);
	cdev_del(&(ppwm->motor_device));
	unregister_chrdev_region(ppwm->device_no, 1);
	
	memset(ppwm, 0, sizeof(struct pwm_motor));
}
module_exit(pwm_motor_exit);

static int enable_gpio(void)
{
	u32 * prcm, * pincntl;
	pincntl = ioremap(PIN_CONTROL_BASE, PIN_CONTROL_MMAPSIZE);
	if ((NULL == pincntl) || ((void *)(-1) == pincntl))
		return -1;
	
	/* pin mux, set gpio output to ENA, timer 5, 6 OUT to IN1, IN2 */
	PWM_OUT(0x80, pincntl + (0x8b8 >> 2));
	PWM_OUT(0x40, pincntl + (0x8bc >> 2));
	PWM_OUT(0x40, pincntl + (0x8d4 >> 2));

	iounmap(pincntl);
	
	/* get gpio clock to work */
	prcm = ioremap(0x48181400, 0x200);
	if ((NULL == prcm) || ((void *)(-1) == prcm)) {
		return -1;
	}

	PWM_OUT(0x0102, prcm + (0x15c >> 2));

	iounmap(prcm);
	return 0;
}

static u32 * init_gpio(void)
{
	u32 v, * gpio_0;

	if (enable_gpio() < 0) {
		return NULL;
	}

	gpio_0 = ioremap(GPIO_0_BASEADDR, GPIO_0_MMAPSIZE);
	if ((NULL == gpio_0) || ((void *)(-1) == gpio_0)) {
		return NULL;
	}

	/* set GPIO0_18 to output mode */
	v = PWM_IN(gpio_0 + (0x134 >> 2));        /* read GPIO_OE register */
	v &= ~(1 << 18);
	PWM_OUT(v, gpio_0 + (0x134 >> 2));        /* write the value back */

#if 0
	/* set GPIO_18 level high */
	v |= (1 << 18);
	PWM_OUT(v, gpio_0 + (0x194 >> 2)); /* corresponds to GPIO_SETDATAOUT */
#endif

	return gpio_0;
}

static int __init pwm_motor_init(void)
{
	int ret;
	dev_t device_no;
	struct pwm_motor * ppwm;
	struct timer_regs * regs_r, * regs_l;

	device_no = 0;
	ppwm = &pwm_motor_device;
	ppwm->tldr      = 0xFFFF0000;
	ppwm->tldr_4    = ppwm->tldr + ((~ ppwm->tldr) >> 1);

	ppwm->tclr      = (1 << 12) | (1 << 10) | (1 << 7) | (1 << 5) | (1 << 1);
	ppwm->tmar_l    = ppwm->tldr + (~ ppwm->tldr) / 3;
	ppwm->tmar_r    = ppwm->tldr + 2 * (~ ppwm->tldr) / 3;
	regs_r = regs_l = NULL;

	if ((ppwm->gpio_control = init_gpio()) == NULL) {
		printk(KERN_ERR "Failed to initialize gpio for PWM motor\n");
		return -EINVAL;
	}

	ret = alloc_chrdev_region(&device_no, 0, 1, DEVICE_NAME);
	if (ret) {
		printk(KERN_ERR "In [%s]: `allo_chr_dev_region has failed: %d\n",
			__func__, ret);
		goto quit;
	}
	
	cdev_init(&(ppwm->motor_device), &pwm_ops);
	ret = cdev_add(&(ppwm->motor_device), device_no, 1);
	if (ret) {
		printk(KERN_ERR "In [%s]: `cdev_add has failed: %d\n", __func__, ret);
		goto quit1;
	}

	if ((ppwm->motor_class = class_create(THIS_MODULE, DEVICE_NAME)) == NULL) {
		printk(KERN_ERR "Failed to create class for PWM motor\n");
		ret = -EINVAL;
		goto quit1;
	}
	ppwm->dev = device_create(ppwm->motor_class, NULL, device_no, NULL, DEVICE_NAME);

	regs_r = (struct timer_regs *) ioremap(TIMER_5_BASEADDR, TIMER_REGISTERS_MAP_SIZE);
	regs_l = (struct timer_regs *) ioremap(TIMER_6_BASEADDR, TIMER_REGISTERS_MAP_SIZE);
	if ((NULL == regs_r) || ((void *)(-1) == regs_r) || (NULL == regs_l) || ((void *)(-1) == regs_l)) {
		printk(KERN_ERR "ioremap has failed while initializing pwm motor moduble: %p, %p\n", 
			regs_r, regs_l);
		ret = -EINVAL;
		goto quit2;
	}

	ppwm->device_no = device_no;
	ppwm->regs_right = regs_r;
	ppwm->regs_left = regs_l;
	ppwm->state = MOTOR_STOPPED;
	atomic_set(&ppwm->users, 0);
	init_pwm_motor(regs_r, ppwm->tldr, ppwm->tmar_r);
	init_pwm_motor(regs_l, ppwm->tldr, ppwm->tmar_l);
    printk("Device %s init successfully,Major %d .\n",DEVICE_NAME,device_no);
    
	return 0;

quit2:
	device_destroy(ppwm->motor_class, device_no);
	class_destroy(ppwm->motor_class);

quit1:
	if ((NULL != regs_r) && ((void *)(-1) != regs_r))
		iounmap(regs_r);
	if ((NULL != regs_l) && ((void *)(-1) != regs_l))
		iounmap(regs_l);
	unregister_chrdev_region(device_no, 1);

quit:
	iounmap(ppwm->gpio_control);
	return ret;
}
module_init(pwm_motor_init);
