#
#	libNX_OMX_Core.a
#

######################################################################

include ../buildcfg.mk

#
#	Target Information
#
TARGET  := test_video_alloc

#	Install Path
INSTALL_PATH := ../../bin

#	Sources
COBJS  	:=
CPPOBJS	:= test_video_alloc.o 
OBJS	:= $(COBJS) $(CPPOBJS)

#	Include Path
INCLUDE += -I./ -I../include -I../src

#	Add dependent libraries
LIBRARY += -L../src -lnx_video_alloc

#	Compile Options
CFLAGS	+= -fPIC

all: $(TARGET) install

$(TARGET):	depend $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJS) -o $@ $(LIBRARY)

install :
	@echo "$(ColorMagenta)[[[ Intall $(TARGET) ]]]$(ColorEnd)"
	install -m 755 -d $(INSTALL_PATH)
	install -m 775 $(TARGET) $(INSTALL_PATH)

clean:
	@echo "$(ColorMagenta)[[[ Clean $(TARGET) ]]]$(ColorEnd)"
	rm -f $(COBJS) $(CPPOBJS) $(TARGET) .depend
	rm -f $(INSTALL_PATH)/$(TARGET)

distclean: clean
	@echo "$(ColorMagenta)[[[ Dist Clean $(TARGET) ]]]$(ColorEnd)"
	rm -f $(INSTALL_PATH)/$(TARGET)

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	@echo "$(ColorMagenta)[[[ Bild $(TARGET) ]]]$(ColorEnd)"
	$(quiet)$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend
