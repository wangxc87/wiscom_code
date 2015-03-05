/*
 *  Copyright (C) 2013 Hefei huaheng tech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/string.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

#define ADS_IOCTL_BASE                     'W'

#define ADS_CONVERT_AND_GET_CMD             _IO(ADS_IOCTL_BASE, 0)
#define ADS_CONVERT_CMD                     _IO(ADS_IOCTL_BASE, 1)
#define ADS_GET_OUTPUT_CMD                  _IO(ADS_IOCTL_BASE, 2)
#define ADS_SINGLE_CONVERT_CMD              _IO(ADS_IOCTL_BASE, 3)
#define ADS_CONTIN_CONVERT_CMD              _IO(ADS_IOCTL_BASE, 4)
#define ADS_SET_DATARATE_CMD                _IO(ADS_IOCTL_BASE, 5)
#define ADS_SET_GAIN_CMD                    _IO(ADS_IOCTL_BASE, 6)

#define NUM_WRITE_REGS                      0x04

struct ads1100 {
	struct i2c_client        *i2c_client;
	int                     (*i2c_send) (struct i2c_client *client, const char *buf, int count);
	int                     (*i2c_recv) (struct i2c_client *client, char * buf, int count);

	u32                       ads1100_config_reg;
	struct file *             thefile;
	atomic_t                  users;
	struct class  *           theclass;
	struct device *           dev;
	struct cdev               ads1100_dev;
	dev_t                     dev_num;
};

#define MAX_ADS1100_DEV         0x02
#define ADS1100_DEV0            0
#define ADS1100_DEV1            1
static struct ads1100 * pads1100[MAX_ADS1100_DEV];

static struct ads1100 * find_ads1100_device(struct i2c_client * client)
{
	int i;
	struct ads1100 * retval;

	retval = NULL;
	if (NULL != client) {
		for (i = 0; i < MAX_ADS1100_DEV; ++i) {
			if (NULL == pads1100[i])
				continue;
			if (client == pads1100[i]->i2c_client) {
				retval = pads1100[i];
				break;
			}
		}
	}

	return retval;
}

static struct ads1100 * find_ads1100_device_file(struct file * filp)
{
	int i;
	struct ads1100 * retval;

	retval = NULL;
	if (likely(NULL != filp)) {
		for (i = 0; i < MAX_ADS1100_DEV; ++i) {
			if (NULL == pads1100[i])
				continue;
			if (filp == pads1100[i]->thefile) {
				retval = pads1100[i];
				break;
			}
		}
	}

	return retval;
}

long ads_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{
	s32 ret;
	u32 config_reg;
	u8 regs[NUM_WRITE_REGS];
	unsigned int argvalue;
	struct ads1100 * ads1100;

	if ((ads1100 = find_ads1100_device_file(filp)) == NULL) {
		return -EINVAL;
	}

	config_reg = ads1100->ads1100_config_reg & 0x9f;
	ret = 0;
	switch (cmd) {
	case ADS_CONVERT_AND_GET_CMD:
	case ADS_CONVERT_CMD:
		if (!(config_reg & (1 << 4))) {
			/* oops, the ads1100 does not seem to be working in single mode, refer to page 9/23 */
			ret = -1;
			break;
		}

		config_reg |= (1 << 7);
		regs[3] = (u8) config_reg;
		ret = ads1100->i2c_send(ads1100->i2c_client, (const char *) regs, 0x01);
		if ((ret < 0) || (cmd != ADS_CONVERT_AND_GET_CMD))
			break;

	case ADS_GET_OUTPUT_CMD:
		regs[0] = 0x0, regs[1] = 0x0, regs[2] = 0x0, regs[3] = 0x0; 
		ret = ads1100->i2c_recv(ads1100->i2c_client, (char *) regs, 0x03);
		if (ret != 0x03) {
			ret = -1;
			break;
		}
		argvalue = (unsigned int) regs[0];
		argvalue = (argvalue << 8) | ((unsigned int) regs[1]);
		argvalue = (argvalue << 8) | ((unsigned int) regs[2]);

		if (0 != (ret = copy_to_user((void *) arg, (void *) &argvalue, sizeof(argvalue)))) {
			printk(KERN_ERR "In [%s]: copy_to_user has failed: %d\n", __func__, ret);
			ret = -1;
		}
		break;

	case ADS_SINGLE_CONVERT_CMD:
		config_reg |= (1 << 4);
		regs[0] = (u8) config_reg;
		ret = ads1100->i2c_send(ads1100->i2c_client, (char *) regs, 0x01);
		if (ret >= 0) {
			ads1100->ads1100_config_reg = config_reg;
		} else {
			printk(KERN_ERR "internal error at line %d: i2c_client: %p\n", __LINE__, (void *) ads1100->i2c_client);
		}
		break;

	case ADS_CONTIN_CONVERT_CMD:
		config_reg &= ~(1 << 4);
		regs[0] = (u8) config_reg;
		ret = ads1100->i2c_send(ads1100->i2c_client, (char *) regs, 0x01);
		if (ret >= 0) {
			ads1100->ads1100_config_reg = config_reg;
		}
		break;

	case ADS_SET_DATARATE_CMD:
		if (!copy_from_user(&argvalue, (void *) arg, sizeof(argvalue))) {
			// printk(KERN_INFO "argvalue: %08X\n", argvalue);
		} else {
			printk(KERN_ERR "inaccessible user space pointer: %p\n", (void *) arg);
			ret = -1;
			break;
		}
		config_reg = (config_reg & 0xF3) | ((argvalue & 0x03) << 2);
		regs[0] = (u8) config_reg;
		ret = ads1100->i2c_send(ads1100->i2c_client, (char *) regs, 0x01);
		if (ret >= 0) {
			ads1100->ads1100_config_reg = config_reg;
		}

		break;

	case ADS_SET_GAIN_CMD:
		if (!copy_from_user(&argvalue, (void *) arg, sizeof(argvalue))) {
			// printk(KERN_INFO "argvalue: %08X\n", argvalue);
		} else {
			printk(KERN_ERR "inaccessible user space pointer: %p\n", (void *) arg);
			ret = -1;
			break;
		}
		config_reg = (config_reg & 0xFC) | (argvalue & 0x03);
		regs[0] = (u8) config_reg;
		ret = ads1100->i2c_send(ads1100->i2c_client, (char *) regs, 0x01);
		if (ret >= 0) {
			ads1100->ads1100_config_reg = config_reg;
		}
		break;

	default:
		printk(KERN_ERR "Unknown ioctl cmd: %08X\n", cmd);
		return -EINVAL;
	}

	if (ret < 0) {
		printk(KERN_ERR "i2c operation has failed in [%s]\n", __func__);
		return -EIO;
	}
	return 0;
}

int ads_open(struct inode * inodp, struct file * filp)
{
	struct cdev * thiscdev;
	struct ads1100 * ads1100;

	thiscdev = inodp->i_cdev;
	ads1100 = container_of(thiscdev, struct ads1100, ads1100_dev);
	printk(KERN_INFO "inode number: %lu, cdev: %p\n", inodp->i_ino, inodp->i_cdev);

	if (atomic_inc_return(&(ads1100->users)) >= 0x02) {
		atomic_dec(&(ads1100->users));
		return -EAGAIN;
	}

	ads1100->thefile = filp;
	return 0;
}

int ads_release(struct inode * inodp, struct file * filp)
{
	struct ads1100 * ads1100;

	if ((ads1100 = find_ads1100_device_file(filp)) == NULL) {
		return -EINVAL;
	}

	if (atomic_read(&(ads1100->users)) > 0) {
		atomic_dec(&(ads1100->users));
	}

	ads1100->thefile = NULL;
	return 0;
}

static const struct file_operations ads1100_ops = {
	.owner             = THIS_MODULE,
	.unlocked_ioctl    = ads_ioctl,
	.open              = ads_open,
	.release           = ads_release,
};

static int __devinit ads1100_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	dev_t dev_num;
	struct class * ads_class;
	static struct ads1100 ads1100_data[MAX_ADS1100_DEV];
	struct i2c_adapter  *adapter = to_i2c_adapter(client->dev.parent);
	struct ads1100 * ads1100p;
	char class_dev[32];

	if (NULL != pads1100[id->driver_data]) {
		printk(KERN_ERR "In [%s]: pads1100[%d]: %p\n", __func__,
               (int) id->driver_data, (void *) pads1100[id->driver_data]);
		return -EINVAL;
	}

	ads1100p = &ads1100_data[id->driver_data];
	memset(ads1100p, 0, sizeof(struct ads1100));
	dev_num = 0;
	snprintf(class_dev, 0x100, "ads1100a%d", (int) id->driver_data);

	atomic_set(&(ads1100p->users), 0);
	ret = alloc_chrdev_region(&dev_num, 0, 1, class_dev);
	if (ret) {
		printk(KERN_ERR "In [%s]: `allo_chr_dev_region has failed: %d\n", __func__, ret);
		goto exit_init;
	}
	ads_class = class_create(THIS_MODULE, class_dev);
	if (IS_ERR(ads_class)) {
		ret = PTR_ERR(ads_class);
		goto exit_init1;
	}

	ads1100p->theclass = ads_class;
	ads1100p->dev = device_create(ads_class, &adapter->dev, dev_num, NULL, class_dev);
	
	cdev_init(&(ads1100p->ads1100_dev), &ads1100_ops);
	ret = cdev_add(&(ads1100p->ads1100_dev), dev_num, 1);
	if (ret) {
		printk(KERN_ERR "In [%s]: `cdev_add has failed: %d\n", __func__, ret);
		goto exit_init1;
	}
	
	ads1100p->dev_num     = dev_num;
	ads1100p->i2c_client  = client;

	ads1100p->i2c_send   = i2c_master_send;
	ads1100p->i2c_recv   = i2c_master_recv;

	ads1100p->ads1100_config_reg = 0x8c;   /* default setting of configure register */
	pads1100[id->driver_data] = ads1100p;
	printk( "Device %s probe successfully, major: %d, minor: %d\n",
            class_dev, MAJOR(dev_num), MINOR(dev_num));
	printk(KERN_INFO "In [%s]: struct device *: %p\n", __func__, (void *) &ads1100p->ads1100_dev);
	return 0;

exit_init1 :
	unregister_chrdev_region(dev_num, 1);
exit_init :
	pads1100[id->driver_data] = NULL;
	return ret;
}

static int __devexit ads1100_remove(struct i2c_client *client)
{
	dev_t dev_num;
	struct ads1100 * ads1100p;
	if ((ads1100p = find_ads1100_device(client)) == NULL) {
		return 0;
	}

	dev_num = ads1100p->dev_num;

	if (NULL != ads1100p->theclass) {
		device_destroy(ads1100p->theclass, dev_num);
		class_destroy(ads1100p->theclass);
		ads1100p->theclass = NULL;
	}
	
	cdev_del(&(ads1100p->ads1100_dev));
	unregister_chrdev_region(dev_num, 1);
	return 0;
}

static const struct i2c_device_id ads1100_id[] = {
	{ "ads1100a0", 0 },
	{ "ads1100a1", 1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ads1100_id);

static struct i2c_driver ads1100_driver = {
	.driver = {
		.name          = "ads1100",
		.owner         = THIS_MODULE,
	},
	.probe             = ads1100_probe,
	.remove            = ads1100_remove,
	.id_table          = ads1100_id,
};

static int __init ads1100_init(void)
{
	pads1100[ADS1100_DEV0] = pads1100[ADS1100_DEV1] = NULL;
	return i2c_add_driver(&ads1100_driver);
}
module_init(ads1100_init);

static void __exit ads1100_exit(void)
{
	i2c_del_driver(&ads1100_driver);
}
module_exit(ads1100_exit);

MODULE_DESCRIPTION("ADS driver for ADS1100");
MODULE_LICENSE("GPL");

