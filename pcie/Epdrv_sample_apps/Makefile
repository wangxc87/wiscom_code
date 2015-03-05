MODE := DISPLAY
CFLAGS = -D$(MODE) -I $(KERNSRC)
CROSS_COMPILE = arm-arago-linux-gnueabi-

DVRRDK_ROOT=/home/wxc/dsp/DVRRDK_04.01.00.02
KERNSRC = $(DVRRDK_ROOT)/ti_tools/linux_lsp/kernel/linux-dvr-rdk

#root filesystem
ROOTFS_DIR=$(DVRRDK_ROOT)/target/rfs_816x
#EXE INSTALL DIR
EXE_INSTALL_DIR = $(ROOTFS_DIR)/home/root

export EXE_INSTALL_DIR
export ROOTFS_DIR

# Uncomment to turn verbose mode on
ifneq ($(debug),)
CFLAGS += -D$(debug)
endif
#CFLAGS += -DDEBUG

all: rc ep

rc \
ep:
ifeq ($(KERNSRC),)
	$(error Pass full path to kernel source directory for RC or EP as KERNSRC=<full-path>)
endif

	@make -C lib "CC=$(CROSS_COMPILE)" "CFLAGS=$(CFLAGS)"
	@make -C application "CC=$(CROSS_COMPILE)" "CFLAGS=$(CFLAGS)" $@
help:
	@echo "make [MODE=THPT/INTEGRITY/DISPLAY] [debug=DEBUG]"

install:
	@make -C application install
clean:
	@make -C lib clean 
	@make -C application clean
