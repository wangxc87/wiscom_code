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
	{ 0x60f71cfa, "complete" },
	{ 0xad41728, "kmalloc_caches" },
	{ 0xf6288e02, "__init_waitqueue_head" },
	{ 0xa61e4362, "omap_request_dma" },
	{ 0xa4610bc6, "omap_rev" },
	{ 0xcd91aa11, "kmem_cache_alloc" },
	{ 0x6eca0298, "queue_work" },
	{ 0x4a37af33, "___dma_single_cpu_to_dev" },
	{ 0x8cd8c339, "omap_free_dma" },
	{ 0x9e7d6bd0, "__udelay" },
	{ 0x42064352, "dev_err" },
	{ 0xdb3877d, "___dma_single_dev_to_cpu" },
	{ 0x5baaba0, "wait_for_completion" },
	{ 0x407a3275, "omap_start_dma" },
	{ 0x4a39e5a1, "omap_set_dma_src_params" },
	{ 0xc52da066, "omap_set_dma_dest_params" },
	{ 0x69b6f8d9, "omap_set_dma_transfer_params" },
	{ 0x7d11c268, "jiffies" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0x1991b29b, "put_device" },
	{ 0xb0f6f6d, "spi_register_master" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xad83a8d, "clk_get" },
	{ 0xe9ce8b95, "omap_ioremap" },
	{ 0xadf42bd5, "__request_region" },
	{ 0x82bb795e, "dev_set_drvdata" },
	{ 0xde7c4be2, "spi_alloc_master" },
	{ 0xfc5caa72, "platform_driver_probe" },
	{ 0x99f8381, "__alloc_workqueue_key" },
	{ 0x788fe103, "iomem_resource" },
	{ 0x37a0cba, "kfree" },
	{ 0x15331242, "omap_iounmap" },
	{ 0x22e16799, "spi_unregister_master" },
	{ 0x9bce482f, "__release_region" },
	{ 0xb60848f7, "platform_get_resource" },
	{ 0x2e1ca751, "clk_put" },
	{ 0x63c23776, "clk_disable" },
	{ 0x440d05a6, "clk_enable" },
	{ 0x82fa3cda, "dev_get_drvdata" },
	{ 0xce8c973d, "destroy_workqueue" },
	{ 0x1f1e12ae, "platform_driver_unregister" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

