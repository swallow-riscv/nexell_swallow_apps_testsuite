CFLAGS = -Wall
INCLUDES := -I../../sysroot/include
LDFLAGS := -L../../sysroot/lib
LIBS += -lnx_v4l2 -lpthread

CROSS_COMPILE ?= aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

SRCS_C := $(wildcard *.c)
OBJS_C := $(SRCS_C:.c=.o)

TARGET := nx-mpegts-test

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $^

$(TARGET): $(OBJS_C)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(LFLAGS)

install: $(TARGET)
	cp $^ ../../sysroot/bin

all: $(TARGET)

.PHONY: clean

clean:
	rm -f *.o
	rm -f $(TARGET)
