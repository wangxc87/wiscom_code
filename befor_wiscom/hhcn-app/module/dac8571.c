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

#define DAC_IOCTL_BASE                     'W'
#define DAC_STORE_DATA_CMD                  _IO(DAC_IOCTL_BASE, 0)
#define DAC_UPDATE_DATA_CMD                 _IO(DAC_IOCTL_BASE, 1)
#define DAC_UPDATE_STORED_DATA_CMD          _IO(DAC_IOCTL_BASE, 2)
#define DAC_BROADCAST_UPDATE_CMD            _IO(DAC_IOCTL_BASE, 3)
#define DAC_BROADCAST_UPDATE_DATA_CMD       _IO(DAC_IOCTL_BASE, 4)
#define DAC_PWD_LPM_CMD                     _IO(DAC_IOCTL_BASE, 5)
#define DAC_PWD_FAST_SETTING_CMD            _IO(DAC_IOCTL_BASE, 6)
#define DAC_PWD_1K_CMD                      _IO(DAC_IOCTL_BASE, 7)
#define DAC_PWD_100K_CMD                    _IO(DAC_IOCTL_BASE, 8)
#define DAC_PWD_OUTPUT_CMD                  _IO(DAC_IOCTL_BASE, 9)

#define NUM_WRITE_REGS                      0x04

/* DAC8571 i2c control byte definition, refer to DAC8571 manual, page 19-20/31 */
#define DAC_STORE_DATA                      0x00       /* Store I2C data. The contents of MS byte and LS byte data (or power-down information) */
                                                       /* are stored into the temporary register. This mode does not change the DAC output. */
#define DAC_UPDATE_DATA                     0x10       /* Update DAC with I2C data. Most common mode. The contents of MS byte and LS byte data */
                                                       /* (or power-down information) are stored into the temporary data register and into the DAC */
                                                       /* register. This mode changes the DAC output with the contents of I2C MS byte and LS byte data. */
#define DAC_UPDATE_STORED_DATA              0x20       /* Update with previously stored data. The contents of MS byte and LS byte data (or power-down */
                                                       /* information) are ignored. The DAC is updated with the contents of the data previously stored in */
                                                       /* the temporary register. This mode changes the DAC output. */
#define DAC_BROADCAST_UPDATE_DATA           0x34
#define DAC_BROADCAST_UPDATE                0x30       /* Broadcast update, If C<2>=0, DAC is updated with the contents of its temporary register. */
                                                       /* If C<2>=1, DAC is updated with I2C MS byte and LS byte data. C<7> and C<6> do not have to be zeroes in */
                                                       /* order for DAC8571 to update. This mode is intended to help DAC8571 work with other DAC857x and */
                                                       /* DAC757x devices for multichannel synchronous update applications. */

/* Power Settings for the DAC8571, refer to page 22/31, note that the below are not control bytes */
#define DAC_PD_LOW_POWER_MODE               0x00       /* Low power mode, default */
#define DAC_PD_FAST_SETTING                 0x20       /* Fast settling mode */
#define DAC_PD_PWD_1K                       0x40       /* PWD. 1kΩ to GND */
#define DAC_PD_PWD_100K                     0x80       /* PWD. 100 kΩ to GND */
#define DAC_PD_PWD_OUTPUT                   0xc0       /* PWD. Output Hi-Z */

struct dac8571 {
	struct i2c_client        *i2c_client;
	s32                     (*read_data) (struct i2c_client *client, u8 command);
	s32                     (*write_data) (struct i2c_client *client, u8 command, const u8 value);

	s32                     (*read_block_data) (struct i2c_client *client, u8 command, u8 length, u8 *values);
	s32                     (*write_block_data) (struct i2c_client *client, u8 command, u8 length, const u8 *values);

	atomic_t                  users;
	struct device *           dev;
	struct cdev               dac8571_dev;
	dev_t                     dev_num;

	struct class  *           dac_class;
	struct inode  *           inodp;
	struct file   *           filp;
};

#define MAX_NUM_DAC8571           0x02
static struct dac8571 * pdac8571s[MAX_NUM_DAC8571];

#define BLOCK_DATA_MAX_TRIES 10

static s32 dac8571_read_block_data_once(struct i2c_client *client, u8 command, u8 length, u8 *values)
{
	s32 i, data;

	for (i = 0; i < length; i++) {
		data = i2c_smbus_read_byte_data(client, command + i);
		if (data < 0)
			return data;
		values[i] = data;
	}
	return i;
}

static s32 dac8571_read_block_data(struct i2c_client *client, u8 command, u8 length, u8 *values)
{
	u8 oldvalues[I2C_SMBUS_BLOCK_MAX];
	s32 ret;
	int tries = 0;

	dev_dbg(&client->dev, "dac8571_read_block_data (length=%d)\n", length);
	ret = dac8571_read_block_data_once(client, command, length, values);
	if (ret < 0)
		return ret;
	do {
		if (++tries > BLOCK_DATA_MAX_TRIES) {
			dev_err(&client->dev,
				"dac8571_read_block_data failed\n");
			return -EIO;
		}
		memcpy(oldvalues, values, length);
		ret = dac8571_read_block_data_once(client, command, length,
						  values);
		if (ret < 0)
			return ret;
	} while (memcmp(oldvalues, values, length));
	return length;
}

static s32 dac8571_write_block_data(struct i2c_client *client, u8 command, u8 length, const u8 *values)
{
	u8 currvalues[I2C_SMBUS_BLOCK_MAX];
	int tries = 0;

	dev_dbg(&client->dev, "dac8571_write_block_data (length=%d)\n", length);
	do {
		s32 i, ret;

		if (++tries > BLOCK_DATA_MAX_TRIES) {
			dev_err(&client->dev,
				"dac8571_write_block_data failed\n");
			return -EIO;
		}
		for (i = 0; i < length; i++) {
			ret = i2c_smbus_write_byte_data(client, command + i,
							values[i]);
			if (ret < 0)
				return ret;
		}
		ret = dac8571_read_block_data_once(client, command, length,
						  currvalues);
		if (ret < 0)
			return ret;
	} while (memcmp(currvalues, values, length));
	return length;
}

static struct dac8571 * find_dac_file(struct file * filp)
{
	int i;
	struct dac8571 * retval;

	retval = NULL;
	for (i = 0; i < MAX_NUM_DAC8571; i++) {
		if ((NULL != pdac8571s[i]) && (filp == pdac8571s[i]->filp)) {
			retval = pdac8571s[i];
			break;
		}
	}
	return retval;
}

long dac_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{
	s32 ret;
	u8 regs[NUM_WRITE_REGS];
	unsigned int argvalue, valid_arg;
	struct dac8571 * dac8571;
	if (NULL == (dac8571 = find_dac_file(filp))) {
		return -ENODEV;
	}

	valid_arg = 0;
	if (!copy_from_user(&argvalue, (void *) arg, sizeof(argvalue))) {
		regs[0] = (u8) (argvalue >> 8);
		regs[1] = (u8) argvalue;
		valid_arg = 1;
		// printk(KERN_INFO "argvalue: %08X\n", argvalue);
	} else {
		regs[0] = regs[1] = 0x0;
	}

	ret = 0;	
	switch (cmd) {
	case DAC_STORE_DATA_CMD:
		/* refer to page 21, EXAMPLE 7 */
		regs[0] = 0xff, regs[1] = 0xff; 
		ret = dac8571->write_block_data(dac8571->i2c_client, DAC_STORE_DATA, 0x02, regs);
		break;
	case DAC_UPDATE_DATA_CMD:
		if (!valid_arg) {
			ret = -1;
			break;
		}
		ret = dac8571->write_block_data(dac8571->i2c_client, DAC_UPDATE_DATA, 0x02, regs);
		break;

	case DAC_UPDATE_STORED_DATA_CMD:
		/* refer to page 21, EXAMPLE 8 */
		ret = dac8571->write_block_data(dac8571->i2c_client, DAC_UPDATE_STORED_DATA, 0x02, regs);
		break;

	case DAC_BROADCAST_UPDATE_CMD:
		/* refer to page 21, EXAMPLE 10 */
		ret = dac8571->write_block_data(dac8571->i2c_client, DAC_BROADCAST_UPDATE, 0x02, regs);
		break;

	case DAC_BROADCAST_UPDATE_DATA_CMD:
		if (!valid_arg) {
			ret = -1;
			break;
		}
		ret = dac8571->write_block_data(dac8571->i2c_client, DAC_BROADCAST_UPDATE_DATA, 0x02, regs);
		break;

	case DAC_PWD_LPM_CMD:
		/* refer to page 21, EXAMPLE 3 */
		regs[0] = DAC_PD_LOW_POWER_MODE;
		regs[1] = 0x0;
		ret = dac8571->write_block_data(dac8571->i2c_client, 0x11, 0x02, regs);
		break;

	case DAC_PWD_FAST_SETTING_CMD:
		/* refer to page 21, EXAMPLE 2 */
		regs[0] = DAC_PD_FAST_SETTING;
		regs[1] = 0x0;
		ret = dac8571->write_block_data(dac8571->i2c_client, 0x11, 0x02, regs);
		break;

	case DAC_PWD_1K_CMD:
		/* refer to page 21, EXAMPLE 5 */
		regs[0] = DAC_PD_PWD_1K;
		regs[1] = 0x0;
		ret = dac8571->write_block_data(dac8571->i2c_client, 0x11, 0x02, regs);
		break;

	case DAC_PWD_100K_CMD:
		/* refer to page 21, EXAMPLE 6 */
		regs[0] = DAC_PD_PWD_100K;
		regs[1] = 0x0;
		ret = dac8571->write_block_data(dac8571->i2c_client, 0x11, 0x02, regs);
		break;
	case DAC_PWD_OUTPUT_CMD:
		/* refer to page 21, EXAMPLE 4 */
		regs[0] = DAC_PD_PWD_OUTPUT;
		regs[1] = 0x0;
		ret = dac8571->write_block_data(dac8571->i2c_client, 0x11, 0x02, regs);
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

int dac_open(struct inode * inodp, struct file * filp)
{
	struct dac8571 * dac8571;
	struct cdev    * c_dev;
	
	c_dev = inodp->i_cdev;
	dac8571 = container_of(c_dev, struct dac8571, dac8571_dev);

	if (atomic_inc_return(&(dac8571->users)) >= 0x02) {
		atomic_dec(&(dac8571->users));
		return -EAGAIN;
	}

	dac8571->inodp = inodp;
	dac8571->filp  = filp;

	return 0;
}

int dac_release(struct inode * inodp, struct file * filp)
{
	struct dac8571 * dac8571;
	struct cdev    * c_dev;
	
	c_dev = inodp->i_cdev;
	dac8571 = container_of(c_dev, struct dac8571, dac8571_dev);

	if (atomic_read(&(dac8571->users)) > 0) {
		atomic_dec(&(dac8571->users));
	}
	
	dac8571->inodp = NULL;
	dac8571->filp  = NULL;
	return 0;
}

static const struct file_operations dac8571_ops = {
	.owner             = THIS_MODULE,
	.unlocked_ioctl    = dac_ioctl,
	.open              = dac_open,
	.release           = dac_release,
};

static int __devinit dac8571_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	dev_t dev_num;
	static struct dac8571 dac8571_data[MAX_NUM_DAC8571];
	struct i2c_adapter  *adapter = to_i2c_adapter(client->dev.parent);
	struct dac8571 * dac8571p;
	char dev_name[256];
	const char * devname = dev_name;

	ret = (int) id->driver_data;
	snprintf(dev_name, 256, "dac8571_%d", ret);
	if ((ret < 0) || (ret >= MAX_NUM_DAC8571) || (NULL != pdac8571s[ret])) {
		return -EINVAL;
	}
	dac8571p = &dac8571_data[ret];

	memset(dac8571p, 0, sizeof(struct dac8571));
	dev_num = 0;

	atomic_set(&(dac8571p->users), 0);
	ret = alloc_chrdev_region(&dev_num, 0, 1, devname);
	if (ret) {
		printk(KERN_ERR "In [%s]: `allo_chr_dev_region has failed: %d\n",
			__func__, ret);
		goto exit_init;
	}

	dac8571p->dac_class = class_create(THIS_MODULE, devname);
	if (IS_ERR(dac8571p->dac_class)) {
		ret = PTR_ERR(dac8571p->dac_class);
		goto exit_init1;
	}
	dac8571p->dev = device_create(dac8571p->dac_class, &adapter->dev, dev_num, NULL, devname);
	
	cdev_init(&(dac8571p->dac8571_dev), &dac8571_ops);
	ret = cdev_add(&(dac8571p->dac8571_dev), dev_num, 1);
	if (ret) {
		printk(KERN_ERR "In [%s]: `cdev_add has failed: %d\n", __func__, ret);
		goto exit_init1;
	}
	
	dac8571p->dev_num     = dev_num;
	dac8571p->i2c_client  = client;

	dac8571p->read_data  = i2c_smbus_read_byte_data;
	dac8571p->write_data = i2c_smbus_write_byte_data;
	if (i2c_check_functionality(adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
		printk(KERN_INFO "In [%s]: block operation\n", __func__);
		dac8571p->read_block_data = i2c_smbus_read_i2c_block_data;
		dac8571p->write_block_data = i2c_smbus_write_i2c_block_data;
	} else {
		printk(KERN_INFO "In [%s]: no block operation\n", __func__);
		dac8571p->read_block_data = dac8571_read_block_data;
		dac8571p->write_block_data = dac8571_write_block_data;
	}

	pdac8571s[id->driver_data] = dac8571p;
	printk( "Device %s probe successfully: major: %d, minor: %d\n", dev_name,MAJOR(dev_num), MINOR(dev_num));
	return 0;

exit_init1 :
	unregister_chrdev_region(dev_num, 1);
exit_init :
	return ret;
}

static struct dac8571 * find_dac_client(struct i2c_client *client, int * index)
{
	struct dac8571 * retval;
	int i;

	retval = NULL;
	for (i = 0; i < MAX_NUM_DAC8571; i++) {
		if ((NULL != pdac8571s[i]) && (pdac8571s[i]->i2c_client == client)) {
			retval = pdac8571s[i];
			*index = i;
			break;
		}
	}

	return retval;
}

static int __devexit dac8571_remove(struct i2c_client *client)
{
	dev_t dev_num;
	int index;
	struct dac8571 * dac8571p;

	if ((dac8571p = find_dac_client(client, &index)) == NULL) {
		return -ENODEV;
	}
	pdac8571s[index] = NULL;

	dev_num = dac8571p->dev_num;
	if (NULL != dac8571p->dac_class) {
		device_destroy(dac8571p->dac_class, dev_num);
		class_destroy(dac8571p->dac_class);
	}
	
	cdev_del(&(dac8571p->dac8571_dev));
	unregister_chrdev_region(dev_num, 1);
	memset(dac8571p, 0, sizeof(struct dac8571));
	return 0;
}

static const struct i2c_device_id dac8571_id[] = {
	{ "dac8571_0", 0 },
	{ "dac8571_1", 1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, dac8571_id);

static struct i2c_driver dac8571_driver = {
	.driver = {
		.name          = "dac8571",
		.owner         = THIS_MODULE,
	},
	.probe             = dac8571_probe,
	.remove            = dac8571_remove,
	.id_table          = dac8571_id,
};

static int __init dac8571_init(void)
{
	pdac8571s[0] = pdac8571s[1] = NULL;
	return i2c_add_driver(&dac8571_driver);
}
module_init(dac8571_init);

static void __exit dac8571_exit(void)
{
	i2c_del_driver(&dac8571_driver);
}
module_exit(dac8571_exit);

MODULE_DESCRIPTION("DAC driver for DAC8571");
MODULE_LICENSE("GPL");

