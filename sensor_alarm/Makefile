CROSS_COMPILE := arm-arago-linux-gnueabi-
LINUXKERNEL_INSTALL_DIR :=/home/wxc/dsp/DVRRDK_04.01.00.02/ti_tools/linux_lsp/kernel/linux-dvr-rdk
KERNEL_DIR := $(LINUXKERNEL_INSTALL_DIR)

DEST_DIR := /home/wxc/dsp/DVRRDK_04.01.00.02/target/rfs_816x/home/root

ARCH := arm
.PHONY:all clean install

export CROSS_COMPILE
export ARCH

OBJ_MOD := sensor_alarm
obj-m := $(OBJ_MOD).o
module-objs :=$(OBJ_MOD).o
PWD := $(shell pwd)

all:module app

module :
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
app:
	$(CROSS_COMPILE)gcc $(OBJ_MOD)_test.c -o $(OBJ_MOD)_test
install:
	mkdir -p $(DEST_DIR)
	cp $(OBJ_MOD).ko $(DEST_DIR)
	cp $(OBJ_MOD)_test $(DEST_DIR)
#	cp phydev_install.sh $(DEST_DIR)
clean:
	rm -f  *.o  *.ko *.order $(OBJ_MOD)_test $(OBJ_MOD).ko
