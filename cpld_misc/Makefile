CROSS_COMPILE := arm-arago-linux-gnueabi-
LINUXKERNEL_INSTALL_DIR :=/home/wxc/dsp/DVRRDK_04.01.00.02/ti_tools/linux_lsp/kernel/linux-dvr-rdk
KERNEL_DIR := $(LINUXKERNEL_INSTALL_DIR)

DEST_DIR := /home/wxc/dsp/DVRRDK_04.01.00.02/target/rfs_816x/home/root

ARCH := arm
.PHONY:all clean install

export CROSS_COMPILE
export ARCH

OBJ_MOD := cpld_misc
obj-m := $(OBJ_MOD).o
module-objs :=$(OBJ_MOD).o
PWD := $(shell pwd)
APP := phydev_regWR

all:module app

module :
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
app:
	$(CROSS_COMPILE)gcc $(APP).c -o $(APP) 
install:
	mkdir -p $(DEST_DIR)
	cp $(OBJ_MOD).ko $(DEST_DIR)
	cp $(APP) $(DEST_DIR)
clean:
	rm -f  *.o  *.ko *.order
