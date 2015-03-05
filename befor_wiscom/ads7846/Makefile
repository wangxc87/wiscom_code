#include Rules.make
CROSS_COMPILE 	:= /opt/CodeSourcery/arm-2009q1-arm/bin/arm-none-linux-gnueabi-
ARCH :=arm

dvr_rdk_BASE     :=/home/wxc/dsp/DVRRDK_02.80.00.10/
TI_SW_ROOT       := $(dvr_rdk_BASE)/ti_tools

LSPDIR        := $(TI_SW_ROOT)/linux_lsp/TI814X_DVR/TI814X-LINUX-PSP-04.01.00.06
KERNEL_DIR     := $(LSPDIR)/src/kernel/linux-04.01.00.06

#KERNEL_DIR = $(LINUXKERNEL_INSTALL_DIR)
TARGET_FS     := $(dvr_rdk_BASE)/target/rfs_814x

DEST_DIR = $(TARGET_FS)/home/root/module

.PHONY:all clean install

export CROSS_COMPILE 
export ARCH

obj-m := ads7846.o
obj-m += omap2_mcspi.o
obj-m += rtc-fm3130.o

module-objs :=ads7846.o
module-objs +=omap2_mcspi.o
module-objs +=rtc-fm3130.o

PWD := $(shell pwd)

all:build_module

build_module :
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

install:
	mkdir -p $(DEST_DIR)
	cp ads7846.ko  $(DEST_DIR)
	cp omap2_mcspi.ko  $(DEST_DIR)
	cp rtc-fm3130.ko  $(DEST_DIR)

#	cp module_install.shell $(DEST_DIR)

clean:
	rm -f  *.o  *.ko *.order
