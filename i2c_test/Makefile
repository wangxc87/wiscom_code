dvr_rdk_BASE     :=/home/wxc/dsp/DVRRDK_04.01.00.02
TARGET_FS     := $(dvr_rdk_BASE)/target/rfs_816x
EXE_DIR :=$(TARGET_FS)/home/root/

COMPILER_PREFIX = arm-arago-linux-gnueabi
CROSS_COMPILE = $(COMPILER_PREFIX)-


CC=arm-arago-linux-gnueabi-gcc

source = i2c_wr.c
obj = $(source: %.c = %.o)

EXE = $(basename $(source))

.PHONY:all clean install

ALL:$(EXE)

$(EXE): $(obj)
	$(CC) $(obj) -o $(EXE)

%.o:%.c
	$(CC) -c $< -o $@

install:
	cp -f $(EXE) $(EXE_DIR) 

clean:
	rm *.o $(EXE)
