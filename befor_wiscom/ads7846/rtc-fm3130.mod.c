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
	{ 0x4c0c5f0f, "i2c_register_driver" },
	{ 0xad41728, "kmalloc_caches" },
	{ 0x86f58661, "rtc_device_register" },
	{ 0xf90652d1, "dev_warn" },
	{ 0x82bb795e, "dev_set_drvdata" },
	{ 0xcd91aa11, "kmem_cache_alloc" },
	{ 0x5838f6c9, "rtc_valid_tm" },
	{ 0x5edd0762, "bin2bcd" },
	{ 0xfec3c2f2, "bcd2bin" },
	{ 0x42064352, "dev_err" },
	{ 0xaabc8cff, "i2c_transfer" },
	{ 0x167e6672, "i2c_smbus_write_byte_data" },
	{ 0x153c4539, "i2c_smbus_read_byte_data" },
	{ 0x37a0cba, "kfree" },
	{ 0xb9934fff, "rtc_device_unregister" },
	{ 0x82fa3cda, "dev_get_drvdata" },
	{ 0x3c5c4750, "i2c_del_driver" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("i2c:fm3130");
