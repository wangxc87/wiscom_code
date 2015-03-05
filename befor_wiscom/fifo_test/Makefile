dvr_rdk_BASE     :=/home/wxc/dsp/DVRRDK_02.80.00.10
TI_SW_ROOT       := $(dvr_rdk_BASE)/ti_tools
CODEGEN_PATH_A8  := $(TI_SW_ROOT)/cgt_a8/arm-2009q1

TARGET_FS     := $(dvr_rdk_BASE)/target/rfs_814x
LSPDIR        := $(TI_SW_ROOT)/linux_lsp/TI814X_DVR/TI814X-LINUX-PSP-04.01.00.06
KERNELDIR     := $(LSPDIR)/src/kernel/linux-04.01.00.06

TOOLCHAIN_DIR :=/opt/CodeSourcery/arm-2009q1-arm/
COMPILER_PREFIX = arm-none-linux-gnueabi
CROSS_COMPILE = $(COMPILER_PREFIX)-


#INCLUDE=
INCLUDE+=$(KERNELDIR)/include/ -I$(KERNELDIR)/usr/include/ -I$(TOOLCHAIN_DIR)/$(COMPILER_PREFIX)/libc/usr/include
#INCLUDE+=I$(KERNELDIR)/usr/include/
#INCLUDE+=I$(KERNELDIR)/include -I$(KERNELDIR)/arch/arm/include/ -I$(KERNELDIR)/arch/arm/plat-omap/include/

EXE_DIR :=$(TARGET_FS)/opt/dvr_rdk/ti814x

CC=$(CODEGEN_PATH_A8)/bin/arm-none-linux-gnueabi-gcc  -I$(INCLUDE)
AR=$(CODEGEN_PATH_A8)/bin/arm-none-linux-gnueabi-ar
LD=$(CODEGEN_PATH_A8)/bin/arm-none-linux-gnueabi-gcc

source = $(wildcard *.c)
#source = saFbdevScalingDisplay.c
#source = qt_start.c 
#source = saFbdevDisplay.c
obj = $(source: %.c = %.o)

EXE = $(basename $(source))

.PHONY:all clean install

ALL:$(EXE)

$(EXE): $(obj)
	$(CC) write_fifo.c -o write_fifo
	$(CC) read_fifo.c -o read_fifo


install:
	cp -f $(EXE) $(EXE_DIR) 

clean:
	rm *.o $(EXE)
