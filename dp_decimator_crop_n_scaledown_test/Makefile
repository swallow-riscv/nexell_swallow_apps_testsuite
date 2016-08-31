CFLAGS = -Wall
INCLUDES := -I../../sysroot/include \
		-I../../sysroot/include/nexell \
		-I../../sysroot/include/libdrm
LDFLAGS := -L../../sysroot/lib
# LIBS += -lkms -ldrm -ldrm_nexell -lnx-v4l2 -lnx-renderer
LIBS += -lkms -ldrm -ldrm_nexell -lnx_v4l2 -lnx_renderer -lpthread

CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc

SRCS_C := $(wildcard *.c)
OBJS_C := $(SRCS_C:.c=.o)

TARGET := dp-decimator-crop-n-scaledown-test

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $^

$(TARGET): $(OBJS_C)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)


install: $(TARGET)
	cp $^ ../../sysroot/bin

all: $(TARGET)

.PHONY: clean

clean:
	rm -f *.o
	rm -f $(TARGET)
