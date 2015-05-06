CROSS_COMPILE := arm-arago-linux-gnueabi-
LINUXKERNEL_INSTALL_DIR :=/home/wxc/dsp/DVRRDK_04.01.00.02/ti_tools/linux_lsp/kernel/linux-dvr-rdk
KERNEL_DIR := $(LINUXKERNEL_INSTALL_DIR)

DEST_DIR := /home/wxc/dsp/DVRRDK_04.01.00.02/target/rfs_816x/home/root

ARCH := arm
.PHONY:all clean install

export CROSS_COMPILE
export ARCH

# OBJ_MOD := max3107
EP_MOD := ti81xx_epdrv
obj-m := $(EP_MOD).o
# obj-m += ti81xx_pcie_rcdrv.o
$(EP_MOD)-objs := ti81xx_pcie_epdrv.o kbuf_utils.o kfifo_buf.o
# module-objs :=$(OBJ_MOD).o
PWD := $(shell pwd)

all:module

module :
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

install:
	mkdir -p $(DEST_DIR)
	cp *.ko $(DEST_DIR)/pcie

clean:
	rm -f  *.o  *.ko *.order  *.ko Module.symvers *.mod.c
