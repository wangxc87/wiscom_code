include ../Rules.make

TARGET = iAMB_test 
TARGET_SH = hw_net_test.sh hw_format_disk.sh hw_format_disk.ini hw_display_test.sh

CFLAGS += -I ../inc  -lbsp4dm8168 -L../src -lpthread

SOURCE := $(wildcard *.c)
OBJECTS =$(SOURCE:%.c=%.o)


.PHONY: all clean install $(TARGET)

all: $(TARGET)

%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(TARGET):clean_target $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(CFLAGS) -fPIC

clean_target:
	@rm -f $(TARGET).o

clean:
	-rm -rf $(TARGET) *.o

install:
#	cp $(TARGET) $(EXE_INSTALL_DIR)/
	cp $(TARGET) $(ROOTFS_DIR)/usr/bin/hw_test
	sudo chmod +x $(TARGET_SH)
	cp $(TARGET_SH) $(ROOTFS_DIR)/usr/bin/

