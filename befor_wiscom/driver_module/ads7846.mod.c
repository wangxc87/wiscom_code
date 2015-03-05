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
	{ 0xc322dcf5, "spi_bus_type" },
	{ 0xfdfb8481, "spi_register_driver" },
	{ 0xc4c01d5a, "driver_unregister" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x8893fa5d, "finish_wait" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0x75a17bed, "prepare_to_wait" },
	{ 0x5f754e5a, "memset" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0xacaf1438, "input_event" },
	{ 0x9e7d6bd0, "__udelay" },
	{ 0xc27487dd, "__bug" },
	{ 0xf9a482f9, "msleep" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0x6c8d5ae8, "__gpio_get_value" },
	{ 0x4665d8b, "input_free_device" },
	{ 0x9b98d1c5, "device_create" },
	{ 0x543712b0, "__class_create" },
	{ 0x2bdc2dd1, "cdev_add" },
	{ 0x1a669a95, "cdev_init" },
	{ 0xea147363, "printk" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x7f2c69d0, "input_register_device" },
	{ 0x9cd34592, "hwmon_device_register" },
	{ 0x713fa2d1, "sysfs_create_group" },
	{ 0xf90652d1, "dev_warn" },
	{ 0x92fef852, "_dev_info" },
	{ 0x859c6dc7, "request_threaded_irq" },
	{ 0x42c67fc6, "regulator_get" },
	{ 0xdb71c901, "input_set_abs_params" },
	{ 0x701d0ebd, "snprintf" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x42064352, "dev_err" },
	{ 0xf6288e02, "__init_waitqueue_head" },
	{ 0xdc798d37, "__mutex_init" },
	{ 0x82bb795e, "dev_set_drvdata" },
	{ 0x9c8c19a2, "input_allocate_device" },
	{ 0x244e4278, "spi_setup" },
	{ 0xbc10dd97, "__put_user_4" },
	{ 0xad41728, "kmalloc_caches" },
	{ 0x6907142b, "spi_sync" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xcd91aa11, "kmem_cache_alloc" },
	{ 0xfe990052, "gpio_free" },
	{ 0x680e25b0, "regulator_put" },
	{ 0x7183df5b, "input_unregister_device" },
	{ 0xf20dabd8, "free_irq" },
	{ 0x10fadef9, "device_init_wakeup" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x37a0cba, "kfree" },
	{ 0x9360f9fa, "cdev_del" },
	{ 0xb03e36e0, "class_destroy" },
	{ 0x97ad189a, "device_destroy" },
	{ 0xd9954cae, "hwmon_device_unregister" },
	{ 0x7ccd801a, "sysfs_remove_group" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0x19a8f8a9, "strict_strtoul" },
	{ 0x9269bfdc, "regulator_disable" },
	{ 0x3ce4ca6f, "disable_irq" },
	{ 0xb9e52429, "__wake_up" },
	{ 0x62b72b0d, "mutex_unlock" },
	{ 0x1b9981cc, "set_irq_wake" },
	{ 0xe16b893b, "mutex_lock" },
	{ 0x82fa3cda, "dev_get_drvdata" },
	{ 0x5fa6297e, "regulator_enable" },
	{ 0xfcec0987, "enable_irq" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

