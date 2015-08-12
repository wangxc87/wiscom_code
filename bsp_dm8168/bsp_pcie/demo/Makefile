
TARGET = pcie_ep_test  pcie_file_op  pcie_rc_test
TARGET += pcie_net_test
CFLAGS += -I ../inc  -lbsp4pcie -L../src -lpthread
CFLAGS += -I${KERNELDIR}/include

.PHONY: all clean install $(TARGET)

all: $(TARGET)

$(TARGET):
	$(CC) -o $@ $@.c $(CFLAGS) -fPIC

clean:
	-rm -rf $(TARGET) *.o

install:
#	cp $(TARGET) $(ROOT)/usr/bin/
	cp $(TARGET) $(EXE_INSTALL_DIR)/

