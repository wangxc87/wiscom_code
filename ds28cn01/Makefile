include ../Rules.make

CFLAGS += -Wall -g -O2
#CFLAGS += -Wall -g

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

APP := ds28cn01_test

.PHONY: all clean

all: $(APP)

$(APP): $(OBJS)
	@echo Creating $@
#	mkdir -p $(EXE_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_DIR) $(LDFLAGS) -o $@ $^ $(LIBS)

include $(SRCS:.c=.d)

%.d: %.c
	set -e; rm -f $@; \
	$(CC) -MM $(INCLUDE_DIR) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

#%.o: %.c
#	$(CC) -c $< $(CFLAGS) $(INCLUDE_DIR)

clean:
	@echo ""
	@echo "Cleaning binaries..."
	@echo ""
	@rm -f $(APP)
	@rm -f $(OBJS)
	@rm -f *.d
	
