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
	{ 0x6980fe91, "param_get_int" },
	{ 0xff964b25, "param_set_int" },
	{ 0xb6c70a7d, "__wake_up" },
	{ 0xc9242666, "mod_timer" },
	{ 0x7d11c268, "jiffies" },
	{ 0x2014fad2, "add_timer" },
	{ 0x9f72c2eb, "init_timer_key" },
	{ 0xfb58121b, "__init_waitqueue_head" },
	{ 0x528c911c, "del_timer" },
	{ 0x47a26c18, "kill_fasync" },
	{ 0xf397b9aa, "__tasklet_schedule" },
	{ 0xa85c710, "kmalloc_caches" },
	{ 0x3339b076, "device_create" },
	{ 0x20134770, "__class_create" },
	{ 0xfcec0987, "enable_irq" },
	{ 0x859c6dc7, "request_threaded_irq" },
	{ 0xde75b689, "set_irq_type" },
	{ 0x65d6d0f0, "gpio_direction_input" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x642958d, "cdev_add" },
	{ 0x7ceee258, "cdev_init" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xbe5f0fe1, "kmem_cache_alloc" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0xbc10dd97, "__put_user_4" },
	{ 0x51493d94, "finish_wait" },
	{ 0x1000e51, "schedule" },
	{ 0x8085c7b1, "prepare_to_wait" },
	{ 0x5f754e5a, "memset" },
	{ 0xf4b1a66a, "fasync_helper" },
	{ 0xea147363, "printk" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x37a0cba, "kfree" },
	{ 0xdd216ffa, "cdev_del" },
	{ 0x6b2ad429, "class_destroy" },
	{ 0xac76e8e9, "device_destroy" },
	{ 0xfe990052, "gpio_free" },
	{ 0xf20dabd8, "free_irq" },
	{ 0x3ce4ca6f, "disable_irq" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

