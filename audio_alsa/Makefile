include ../Rules.make

# dvr_rdk_BASE     :=/home/wxc/dsp/DVRRDK_04.01.00.02
# TARGET_FS     := $(dvr_rdk_BASE)/target/rfs_816x
# EXE_DIR :=$(TARGET_FS)/home/root/audio/

# COMPILER_PREFIX = arm-arago-linux-gnueabi
# #INCLUDE = ../bsp/
# CROSS_COMPILE = $(COMPILER_PREFIX)-


# CC=arm-arago-linux-gnueabi-gcc -I $(INC_DIR)

EXE_DIR :=$(ROOTFS_DIR)/home/root/audio/

CFLAG = -I $(INC_DIR) -Wall -lasound

source = audio_alsa_loopback.c
obj = $(source: %.c = %.o)

EXE = audio_alsa_loopback

.PHONY:all clean install

ALL:$(EXE)

$(EXE): $(source)
	$(CC)  $(CFLAG) $(source) -o $(EXE)

%.o:%.c
	$(CC) -c $< -o $@  $(CFLAG) 

install:
	cp -f $(EXE) $(EXE_DIR) 

clean:
	rm *.o $(EXE)
