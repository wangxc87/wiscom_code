#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf004d205, "module_layout" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0x4c0c5f0f, "i2c_register_driver" },
	{ 0x9d669763, "memcpy" },
	{ 0x71c90087, "memcmp" },
	{ 0x42064352, "dev_err" },
	{ 0x2db583, "i2c_smbus_write_i2c_block_data" },
	{ 0x5c2d8509, "i2c_smbus_read_i2c_block_data" },
	{ 0x167e6672, "i2c_smbus_write_byte_data" },
	{ 0x153c4539, "i2c_smbus_read_byte_data" },
	{ 0x2bdc2dd1, "cdev_add" },
	{ 0x1a669a95, "cdev_init" },
	{ 0x9b98d1c5, "device_create" },
	{ 0x543712b0, "__class_create" },
	{ 0xea147363, "printk" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x701d0ebd, "snprintf" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x9360f9fa, "cdev_del" },
	{ 0xb03e36e0, "class_destroy" },
	{ 0x97ad189a, "device_destroy" },
	{ 0x3c5c4750, "i2c_del_driver" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("i2c:dac8571_0");
MODULE_ALIAS("i2c:dac8571_1");
