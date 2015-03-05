dvr_rdk_BASE     :=/home/wxc/dsp/DVRRDK_04.01.00.02
TARGET_FS     := $(dvr_rdk_BASE)/target/rfs_816x
EXE_DIR :=$(TARGET_FS)/home/root/

COMPILER_PREFIX = arm-arago-linux-gnueabi
INCLUDE = ../bsp/
CROSS_COMPILE = $(COMPILER_PREFIX)-


CC=arm-arago-linux-gnueabi-gcc -I $(INCLUDE)

source = rd_tvp5158_status.c ../bsp/bsp.c
obj = $(source: %.c = %.o)

EXE = rd_tvp5158_status

.PHONY:all clean install

ALL:$(EXE)

$(EXE): $(source)
	$(CC) $(source) -o $(EXE)

%.o:%.c
	$(CC) -c $< -o $@

install:
	cp -f $(EXE) $(EXE_DIR) 

clean:
	rm *.o $(EXE)
