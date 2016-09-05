TARGET := sched
INCLUDES_LIBS_HEADERS := -I/usr/local/include/ -I/usr/include/ -I$(PWD)/include
GCC := gcc
DFLAGS := -g -O0 -fno-omit-frame-pointer -pipe -Wall -D$(SCHEDULE_TYPE)
LIBS := -lm -lpthread

SRCF= $(wildcard *.c)
SRCF+= $(wildcard kernel/*.c)
OBJF= $(SRCF:.c=.o)

%.o: %.c
	@echo "CC" $@
	@@$(GCC) $(DFLAGS) $(INCLUDES_LIBS_HEADERS) -c -fmessage-length=0 -o $@ $<

test: $(OBJF)
	$(GCC) $(OBJF) -o $(TARGET) $(LIBS)

all: $(OBJF)
	$(GCC) $(OBJF) -o $(TARGET) $(LIBS)
	rm -f $(OBJF)
	rm -f *.o

clean:
	rm -f $(OBJF) rm -f $(TARGET)
