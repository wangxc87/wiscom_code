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
	{ 0xf7499e8b, "i2c_del_driver" },
	{ 0x25b64486, "i2c_unregister_device" },
	{ 0x4091ffaf, "i2c_register_driver" },
	{ 0xf2291a42, "i2c_new_device" },
	{ 0x1d47acb3, "i2c_get_adapter" },
	{ 0x9d669763, "memcpy" },
	{ 0x5f754e5a, "memset" },
	{ 0x97307686, "misc_register" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0xd57d82b5, "i2c_master_recv" },
	{ 0x133130e7, "i2c_master_send" },
	{ 0xb3ad031, "misc_deregister" },
	{ 0xca6ad8d, "i2c_transfer" },
	{ 0xea147363, "printk" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

