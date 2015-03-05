/*
 * Application to boot TI816X or TI814X PCIe Endpoint set for booting in 32-bit
 * PCIe mode. Runs on PCIe RC (host) and uses character device interface
 * provided by TI81XX PCIe EP boot driver to boot first detected TI816X/TI814X
 * Endpoint.
 *
 * Details applicable for both TI816X and TI814X devices are prefixed with
 * TI81XX/ti81xx while specific defails use TI816X/TI814X as applicable.
 *  
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/stat.h>
#include <linux/ioctl.h>

#include <sys/mman.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define u32	u_int32_t

#include <ti81xx_pcie_bootdrv.h>

/*
 * Set this flag to '0' if you don't want to continue when U-Boot (running on
 * EP) doesn't clear the BOOTFLAG.
 *
 * Note: By default we are setting this to '1' since current U-Boot doesn't
 * clear the BOOTFLAG (e.g., after setting DDR on EP). Thus we add some delay
 * after flag check fail and continue loading bootscript, kernel and ramdisk. If
 * you face problems with this, then TRY INCREASING DELAY in seconds as set
 * below with CONFIG_BOOTFLAG_FAIL_DELAY (default is 3 seconds).
 */
#define CONFIG_IGNORE_BOOTFLAG_FAIL	1
#define CONFIG_BOOTFLAG_FAIL_DELAY	3

#define TI816X_EP_UBOOT_BAR		1
#define TI814X_EP_UBOOT_STG1_BAR	1
#define TI814X_EP_UBOOT_BAR		2
#define TI816X_EP_BOOTSCRIPT_BAR	2
#define TI816X_EP_KERNEL_BAR		2
#define TI816X_EP_RAMDISK_BAR		2

#if CONFIG_IGNORE_BOOTFLAG_FAIL
static int ignore_bootflag_fail	= 1;
#else
static int ignore_bootflag_fail	= 0;
#endif

static int dev_desc = -1;
static char *mapped_buffer;
static unsigned int device_id;

/*
 *  Map BAR and transfer the specified file
 */
static int map_file(char *file_name, unsigned int bar_number,
		unsigned int offset, unsigned int max_size)
{
	int result = -1, ret;
	FILE *file_desc = NULL;
	unsigned int file_length = 0;
	struct ti81xx_bar_info bar;

	bar.num = bar_number;
	result = ioctl(dev_desc, TI81XX_PCI_GET_BAR_INFO, &bar);
	if (result) {
		printf("PCIe: Getting BAR%d info failed\n", bar.num);
		return -1;
	}

	if (max_size > bar.size) {
		printf("Error: Maximum size of image (%uB) greater than "
				"BAR size (%uB). This is "
				"not supported currently.\n",
				max_size, bar.size);
		return -1;
	}

	mapped_buffer = (char *)mmap(NULL, bar.size, (PROT_READ | PROT_WRITE),
			MAP_SHARED, dev_desc, (off_t)bar.addr);

	if (MAP_FAILED == (void *)mapped_buffer) {
		printf("Error: Cannot mmap = %d bytes buffer\n", bar.size);
		return -1;
	}

	file_desc = fopen(file_name, "rb");
	if (!file_desc) {
		printf("Specified file \"%s\" not found\n", file_name);
		munmap(mapped_buffer, bar.size);
		return -1;
	}

	printf("%s file opened\n", file_name);
	fflush(stdout);

	fseek(file_desc, 0, SEEK_END);
	file_length = ftell(file_desc);
	fseek(file_desc, 0, SEEK_SET);

	if (file_length > max_size) {
		printf("Error: Filesize (%uB) greater than max allowed (%uB)\n",
				file_length, max_size);
		ret = -1;
		goto lbl_cleanup;
	}

	printf("Size of %s file = %d\n", file_name, (int)file_length);
	fflush(stdout);

	result = fread((mapped_buffer + offset), 1, file_length, file_desc);

	if (result < file_length) {
		printf("Could only read %d bytes from the file \"%s\"",
				result, file_name);
		ret = -1;
		goto lbl_cleanup;
	}

	printf("%d bytes read from file %s\n", file_length, file_name);
	fflush(stdout);
	ret = 0;

lbl_cleanup:
	fclose(file_desc);
	munmap(mapped_buffer, bar.size);
	return ret;
}

int main(int argc, char *argv[])
{
	int result = 0;
	struct ti81xx_bar_info bar;
	char dev_name[128] = "/dev/";

	if (argc < 4) {
		printf("Usage: [linux-prompt]# "
			"./saBootApp.o "
			"<u-boot-binary> <bootscript> <kernel> [ramdisk]\n");
		printf("\teg: # ./saBootApp.o "
			"u-boot.bin boot.scr uImage ramdisk.ext2\n");
		printf("\t*** Note: For booting TI814X devices, 1st stage "
				"U-Boot binary named MLO needs to be present "
				"in current directory and u-boot binary "
				"passed as 1st argument will be used as 2nd "
				"stage.\n");
		return -1;
	}

	strcat(dev_name, TI81XX_PCIE_MODFILE);
	dev_desc = open(dev_name, O_RDWR);
	if (-1 == dev_desc) {
		printf("Device \"%s\" could not opened\n", dev_name);
		return -1;
	}

	ioctl(dev_desc, TI81XX_PCI_GET_DEVICE_ID, &device_id);

	if (device_id == TI814X_PCI_DEVICE_ID) {
		FILE *file_desc = fopen("MLO", "rb");

		if (!file_desc) {
			printf("Error: first stage TI814X U-Boot binary "
					"\"MLO\" not found in cwd\n");
			close(dev_desc);
			return -1;
		}

		fclose (file_desc);

		bar.num = TI814X_EP_UBOOT_STG1_BAR;
		bar.addr = TI814X_EP_UBOOT_STG1_IB_OFFSET;
		result = ioctl(dev_desc, TI81XX_PCI_SET_BAR_WINDOW, &bar);
		if (result) {
			printf("PCIe: Setting BAR%d window failed\n", bar.num);
			close(dev_desc);
			return -1;
		}

		result = map_file("MLO", bar.num, TI814X_EP_UBOOT_STG1_OFFSET,
				TI814X_EP_UBOOT_STG1_MAX_SIZE);
		if (result) {
			printf("Mapping of uBoot to BAR1 (OCMC1) failed\n");
			close(dev_desc);
			return result;
		}

	} else if (device_id == TI816X_PCI_DEVICE_ID) {
		bar.num = TI816X_EP_UBOOT_BAR;
		bar.addr = TI816X_EP_UBOOT_IB_OFFSET;
		result = ioctl(dev_desc, TI81XX_PCI_SET_BAR_WINDOW, &bar);
		if (result) {
			printf("PCIe: Setting BAR%d window failed\n", bar.num);
			close(dev_desc);
			return -1;
		}

		result = map_file(argv[1], bar.num, TI81XX_EP_UBOOT_OFFSET,
				TI81XX_EP_UBOOT_MAX_SIZE);
		if (result) {
			printf("Mapping of uBoot to BAR1 (OCMC1) failed\n");
			close(dev_desc);
			return result;
		}
	} else {
		printf("*** Error: Unsupported device %04x\n", device_id);
		close(dev_desc);
		return -1;
	}

	/*
	 * FIXME: Increase this delay from 3 sec if U-Boot takes more time to
	 * setup DDR and clear boot flag.
	 */
	result = ioctl(dev_desc, TI81XX_PCI_SET_DWNLD_DONE,
			CONFIG_BOOTFLAG_FAIL_DELAY);
	if (result) {
		printf("PCIe: U-Boot failed to execute properly\n");

		if (ignore_bootflag_fail) {
			printf("Still continuing...\n");
		} else {
			close(dev_desc);
			return -1;
		}
	}

	if (device_id == TI814X_PCI_DEVICE_ID) {
		bar.num = TI814X_EP_UBOOT_BAR;
		bar.addr = TI814X_EP_UBOOT_IB_OFFSET;
		result = ioctl(dev_desc, TI81XX_PCI_SET_BAR_WINDOW, &bar);
		if (result) {
			printf("PCIe: Setting BAR%d window failed\n", bar.num);
			close(dev_desc);
			return -1;
		}

		result = map_file(argv[1], bar.num, TI81XX_EP_UBOOT_OFFSET,
				TI81XX_EP_UBOOT_MAX_SIZE);
		if (result) {
			printf("Mapping of uBoot to BAR1 (OCMC1) failed\n");
			close(dev_desc);
			return result;
		}
	}

	bar.num = TI816X_EP_BOOTSCRIPT_BAR;
	bar.addr = TI81XX_EP_BOOTSCRIPT_IB_OFFSET;
	result = ioctl(dev_desc, TI81XX_PCI_SET_BAR_WINDOW, &bar);
	if (result) {
		printf("PCIe: Setting BAR%d window failed\n", bar.num);
		close(dev_desc);
		return -1;
	}

	result = map_file(argv[2], bar.num, TI81XX_EP_BOOTSCRIPT_OFFSET,
			TI81XX_EP_BOOTSCRIPT_MAX_SIZE);
	if (result) {
		printf("Mapping of boot script to BAR2 (DDR) failed\n");
		close(dev_desc);
		return result;
	}

	bar.num = TI816X_EP_KERNEL_BAR;
	bar.addr = 0xc0900000;//TI81XX_EP_KERNEL_IB_OFFSET; orig
	result = ioctl(dev_desc, TI81XX_PCI_SET_BAR_WINDOW, &bar);
	if (result) {
		printf("PCIe: Setting BAR%d window failed\n", bar.num);
		close(dev_desc);
		return -1;
	}

	result = map_file(argv[3], bar.num, TI81XX_EP_KERNEL_OFFSET,
			TI81XX_EP_KERNEL_MAX_SIZE);
	if (result) {
		printf("Mapping of Kernel to BAR2 (DDR) failed\n");
		close(dev_desc);
		return result;
	}

	if (argc > 4) {
		bar.num = TI816X_EP_RAMDISK_BAR;
		bar.addr = 0xc1000000;//TI81XX_EP_RAMDISK_IB_OFFSET;
		result = ioctl(dev_desc, TI81XX_PCI_SET_BAR_WINDOW, &bar);
		if (result) {
			printf("PCIe: Setting BAR%d window failed\n", bar.num);
			close(dev_desc);
			return -1;
		}

		result = map_file(argv[4], bar.num, TI81XX_EP_RAMDISK_OFFSET,
				TI816X_EP_RAMDISK_MAX_SIZE);
		if (result) {
			printf("Mapping of RAMDISK to BAR2 (DDR) failed\n");
			close(dev_desc);
			return result;
		}
	}

	/* FIXME: We are not waiting for U-Boot indication */
	result = ioctl(dev_desc, TI81XX_PCI_SET_DWNLD_DONE, 0);
	if (result) {
		printf("PCIe: U-Boot failed to boot kernel\n");

		if (ignore_bootflag_fail) {
			printf("Still continuing...\n");
		} else {
			close(dev_desc);
			return -1;
		}
	}

	printf("TI81XX EP Boot completed\n");

	close(dev_desc);
	return result;
}
