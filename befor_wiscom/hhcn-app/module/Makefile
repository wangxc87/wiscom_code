#include Rules.make
CROSS_COMPILE 	:= arm-none-linux-gnueabi-
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

obj-m := ads1100.o
obj-m += dac8571.o
obj-m += pwm.o

module-objs :=ads1100.o
module-objs +=dac8571.o
module-objs +=pwm.o

PWD := $(shell pwd)

all:build_module

build_module :
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

install:
	mkdir -p $(DEST_DIR)
	cp ads1100.ko  $(DEST_DIR)
	cp dac8571.ko  $(DEST_DIR)
	cp pwm.ko  $(DEST_DIR)

#	cp module_install.shell $(DEST_DIR)

clean:
	rm -f  *.o  *.ko *.order
