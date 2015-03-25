INC=../src
INCLIB=../src

SRC_EP=pcie_ep_test.c
SRC_RC=pcie_rc_test.c
SRC_FILE =pcie_file_op.c

OBJS_EP=$(SRC_EP:%.c=%.o)
OBJS_RC=$(SRC_RC:%.c=%.o)
OBJS_FILE=$(SRC_FILE:%.c=%.o)

EXE_EP=$(basename $(SRC_EP))
EXE_RC=$(basename $(SRC_RC))
EXE_FILE=$(basename $(SRC_FILE))

EXE = $(EXE_EP) $(EXE_RC) $(EXE_FILE)

LIBS=$(INCLIB)/lib_ti81xx.a $(INCLIB)/lib_memcpy_neon.a
INCLUDE_FILE = $(wildcard $(INC)/*.h)

INTCFLAGS =-I$(INC)

all:ep rc pcie_file

$(OBJS_EP): $(SRC_EP) 
	$(CC)gcc $(INTCFLAGS) $(CFLAGS) -c $< -o $@

$(OBJS_RC): $(SRC_RC)
	$(CC)gcc $(INTCFLAGS) $(CFLAGS) -c $< -o $@

$(OBJS_FILE):$(SRC_FILE)
	$(CC)gcc $(INTCFLAGS) $(CFLAGS) -c $< -o $@

$(OBJS_RC):$(INCLUDE_FILE)

$(OBJS_EP):$(INCLUDE_FILE)

$(OBJS_FILE):$(INCLUDE_FILE)


ep: $(OBJS_EP) $(LIBS)
	$(CC)gcc $(INTCFLAGS) $(CFLAGS) -o $(EXE_EP) $(OBJS_EP) -L. $(LIBS) -lpthread

rc: $(OBJS_RC) $(LIBS)
	$(CC)gcc $(INTCFLAGS) $(CFLAGS) -o $(EXE_RC) $(OBJS_RC) -L. $(LIBS) -lpthread

pcie_file: $(OBJS_FILE) $(LIBS)
	$(CC)gcc $(INTCFLAGS) $(CFLAGS) -o $(EXE_RC) $(OBJS_RC) -L. $(LIBS) -lpthread
	$(CC)gcc $(INTCFLAGS) $(CFLAGS) -o $(EXE_FILE) $(OBJS_FILE) -L. $(LIBS) -lpthread


install:
	cp $(EXE) $(ROOTFS_DIR)/home/root/pcie/
clean:
	-rm -f *.o $(EXE)
