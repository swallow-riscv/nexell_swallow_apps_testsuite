CFLAGS = -Wall
INCLUDES := -I./
LDFLAGS := -L../../sysroot/lib
LIBS := -lnx_v4l2 -lnx_allocator -lpthread

SRCS := cam_test_onedevice.cpp
OBJS := cam_test_onedevice.o
#SRCS := cam_test_thread.cpp
#OBJS := cam_test_thread.o
TARGET := cam_test

%.o: %.cpp
	$(CC) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) -c $^

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install: $(TARGET)
	cp $^ $(DESTDIR)

all: $(TARGET)

.PHONY: clean

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
