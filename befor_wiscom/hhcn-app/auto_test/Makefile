

CROSS = arm-none-linux-gnueabi-
CC    = $(CROSS)gcc
CFLAGS = -Wall -ggdb -O2


APPS_obj = main.c dac8571.c ads1100.c motor_test.c 
APPS = auto_test

all : $(APPS)
	
$(APPS):$(APP_obj)
	$(CC) $(CFLAGS)  $(APPS_obj) -o $@
	
$(APP_obj):main.o app-dac8571.o app-ads1100.o app-motor.o

main.o:main.c
	$(CC)  $(CFLAGS) -o $@ $^

dac8571.o : dac8571.c
	$(CC)  $(CFLAGS) -o $@ $^

ads1100.o : ads1100.c
	$(CC)  $(CFLAGS) -o $@ $^

motor_test.o : motor_test.c
	$(CC)  $(CFLAGS) -DSET_DIRECTION -o $@ $^

.PHONY : clean

clean : 
	rm -rf $(APPS)
install:
	cp $(APPS) ~/dsp/DVRRDK_02.80.00.10/target/rfs_814x/home/root/app-test/
