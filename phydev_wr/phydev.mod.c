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
	{ 0x9738c73f, "module_layout" },
	{ 0x9161fe64, "device_destroy" },
	{ 0x23fad408, "kmalloc_caches" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x37a0cba, "kfree" },
	{ 0xe49465a5, "cdev_del" },
	{ 0xd23873fc, "class_destroy" },
	{ 0xfed8b69, "device_create" },
	{ 0x76b141b6, "__class_create" },
	{ 0xc343643d, "cdev_add" },
	{ 0xe1f7e251, "cdev_init" },
	{ 0x342234be, "kmem_cache_alloc" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0xb5bec987, "mdio_bus_type" },
	{ 0x5d25f973, "mdiobus_read" },
	{ 0x7aaf206d, "mdiobus_write" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0xea147363, "printk" },
	{ 0xd2f3eb62, "bus_find_device_by_name" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xfbc74f64, "__copy_from_user" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

