

CROSS = arm-none-linux-gnueabi-
CC    = $(CROSS)gcc
CFLAGS = -Wall -ggdb -O2


APPS = app-dac8571 app-ads1100 app-motor

all : $(APPS)

app-dac8571 : dac8571.c
	$(CC)  $(CFLAGS) -o $@ $^

app-ads1100 : ads1100.c
	$(CC)  $(CFLAGS) -o $@ $^

app-motor : motor_test.c
	$(CC)  $(CFLAGS) -DSET_DIRECTION -o $@ $^

.PHONY : clean

clean : 
	rm -rf $(APPS)
install:
	cp $(APPS) ~/dsp/DVRRDK_02.80.00.10/target/rfs_814x/home/root/app-test/
