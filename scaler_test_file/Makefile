CFLAGS = -Wall
INCLUDES := -I./
LIBS := -lnx_scaler -lnx_allocator

SRCS := scaler-test-file.cpp
OBJS := scaler-test-file.o
TARGET := scaler-test-file

%.o: %.cpp
	$(CC) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) -c $^

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

install: $(TARGET)
	cp $^ $(DESTDIR)
	cp input.yuv $(DESTDIR)

all: $(TARGET)

.PHONY: clean

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
