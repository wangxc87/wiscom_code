include ../Rules.make

TARGET = bsp_demo read_mac cpld_control_test sensorAlarm_test cpld_control_reset_test phydev_test 
CFLAGS += -I ../inc  -lbsp4dm8168 -L../src -lpthread

TARGET += videoInputStatus_test

.PHONY: all clean install $(TARGET)

all: $(TARGET)

$(TARGET):
	$(CC) -o $@ $@.c $(CFLAGS) -fPIC

clean:
	-rm -rf $(TARGET) *.o

install:
	cp $(TARGET) $(EXE_INSTALL_DIR)/

