CFLAGS = -Wall
INCLUDES := -I../../sysroot/include
LDFLAGS := -L../../sysroot/lib
LIBS := -lnx-drm-allocator -lnx-v4l2

CROSS_COMPILE := aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

TARGET := nx-camera-test

.cpp.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $^

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install: $(TARGET)
	cp $^ ../../sysroot/bin

all: $(TARGET)

.PHONY: clean

clean:
	rm -f *.o
	rm -f $(TARGET)
