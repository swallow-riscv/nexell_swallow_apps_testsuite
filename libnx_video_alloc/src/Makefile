#
#	libNX_OMX_Core.a
#

######################################################################

include ../buildcfg.mk

#
#	Target Information
#
LIBNAME := libnx_video_alloc
TARGET  := $(LIBNAME).a

#	Install Path
INSTALL_PATH := ../../libs

#	Sources
COBJS  	:= nx_video_alloc.o
CPPOBJS	:=  
OBJS	:= $(COBJS) $(CPPOBJS)

#	Include Path
INCLUDE += -I./ -I../include

#	Add dependent libraries
LIBRARY += 

#	Compile Options
CFLAGS	+= -fPIC

all: $(TARGET) install

$(TARGET):	depend $(OBJS)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS)
#	$(quiet)$(CC) $(LDFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $(OBJS) $(LIBRARY)

install :
	@echo "$(ColorMagenta)[[[ Intall $(LIBNAME) ]]]$(ColorEnd)"
	install -m 755 -d $(INSTALL_PATH)
	install -m 644 $(TARGET) $(INSTALL_PATH)

clean:
	@echo "$(ColorMagenta)[[[ Clean $(LIBNAME) ]]]$(ColorEnd)"
	rm -f $(COBJS) $(CPPOBJS) $(TARGET) .depend
	rm -f $(INSTALL_PATH)/$(TARGET)

distclean: clean
	@echo "$(ColorMagenta)[[[ Dist Clean $(LIBNAME) ]]]$(ColorEnd)"
	rm -f $(INSTALL_PATH)/$(TARGET)

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	@echo "$(ColorMagenta)[[[ Bild $(LIBNAME) ]]]$(ColorEnd)"
	$(quiet)$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend
