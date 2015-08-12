# Makefile for audio applications

#KERNEL_PATH=linux-davinci
KERNEL_PATH=/media/hdc1/opt/dvsdk_1_40_02_33/linux-2.6
CROSS_COMPILE=arm-none-linux-gnueabi-
#ALSA_LIB_PATH=/opt/arago-2009.09/arm-none-linux-gnueabi/usr
ALSA_LIB_PATH=/tmp/build

CC = $(CROSS_COMPILE)gcc -I$(KERNEL_PATH)/include -I$(ALSA_LIB_PATH)/include
LDFLAGS=-L$(ALSA_LIB_PATH)/lib

all: audio_loopback_alsa

audio_loopback_alsa: audio_loopback_alsa.o
	$(CC) -o audio_loopback_alsa $(LDFLAGS) -lasound $^

%.o:%.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f *.o *~ core audio_loopback_alsa
