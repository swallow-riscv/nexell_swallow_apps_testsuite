#
#	video_api_test
#

######################################################################

#
#	Target Information
#
TARGET  := video_api_test

FFMPEG_VERSION := ffmpeg-2.8.6-arm64

#	Include Common Build Enviroments
include ../buildcfg.mk

#	Install Path
INSTALL_PATH := ../../../bin

#	Sources
COBJS  	:=
CPPOBJS	:= \
		MediaExtractor.o	\
		CodecInfo.o			\
		NX_Queue.o			\
		NX_Semaphore.o		\
		Util.o				\
		VideoDecTest.o		\
		VideoEncTest.o		\
		main.o

OBJS	:= $(COBJS) $(CPPOBJS)

#	Include Path
INCLUDE += \
		-I./				\
		-I../include		\
		-I../src			\
		-I../src/nx_video_alloc \
		-I./ffmpeg/$(FFMPEG_VERSION)/include

#	Add dependent libraries
LIBRARY += -lstdc++ -lm

LIBRARY += \
		-L../src	\
		-lnx_video_api

#	Add FFMPEG libraries
LIBRARY += \
		-L./ffmpeg/$(FFMPEG_VERSION)/lib	\
		-lavcodec -lavformat -lavutil -lswresample


#	Compile Options
CFLAGS	+= -g -pthread


######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS)

all:	\
	$(THEORAPARSER) \
	$(TARGET)

$(TARGET):	depend $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJS) -o $@ $(LIBRARY)

install:
	install -m 755 -d $(INSTALL_PATH)
	install -m 644 $(TARGET) $(INSTALL_PATH)

clean:
	@echo "$(ColorMagenta)[[[ Clean $(TARGET) ]]]$(ColorEnd)"
	rm -f $(COBJS) $(CPPOBJS) $(TARGET) .depend

distclean: clean
	rm -f $(INSTALL_PATH)/$(TARGET)

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	@echo "$(ColorMagenta)[[[ Build $(TARGET) ]]]$(ColorEnd)"
	$(quiet)$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend
