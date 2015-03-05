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
	{ 0x8b28cb9c, "module_layout" },
	{ 0x3339b076, "device_create" },
	{ 0x20134770, "__class_create" },
	{ 0x642958d, "cdev_add" },
	{ 0x7ceee258, "cdev_init" },
	{ 0xea147363, "printk" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x6b2ad429, "class_destroy" },
	{ 0xac76e8e9, "device_destroy" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xdd216ffa, "cdev_del" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

