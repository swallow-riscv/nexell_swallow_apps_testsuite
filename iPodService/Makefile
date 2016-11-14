ifndef TOPDIR
TOPDIR := ./
endif

include $(TOPDIR)/env.mk

######################################################################
# Build options
INCLUDE += -I./
INCLUDE += -I./iAP2Link
INCLUDE += -I./iAP2Utility
INCLUDE += -I./iAP2UtilityImplementation
INCLUDE += -I./utils
INCLUDE_EXT =

LIBRARY += -pthread -lrt
LIBRARY_EXT =

CFLAGS 	:= -static

######################################################################
# Target
TARGET	:= ipod_service

######################################################################
# Build
COBJS	:=
COBJS	+= iAP2Link/iAP2Link.o iAP2Link/iAP2FileTransfer.o iAP2Link/iAP2LinkAccessory.o iAP2Link/iAP2LinkRunLoop.o iAP2Link/iAP2Packet.o
COBJS	+= iAP2Utility/iAP2BuffPool.o iAP2Utility/iAP2FSM.o iAP2Utility/iAP2ListArray.o 
COBJS	+= iAP2UtilityImplementation/iAP2BuffPoolImplementation.o iAP2UtilityImplementation/iAP2Log.o iAP2UtilityImplementation/iAP2Time.o
COBJS	+= utils/gpio.o utils/NX_Semaphore.o utils/ux_timer.o
COBJS	+= main.o iAP_Auth.o Cpchip.o uevent.o

CPPOBJS	:=

OBJS	:= $(COBJS) $(CPPOBJS)

all: $(TARGET)

clean :
	rm -f $(TARGET) $(UPDATE) *.o iAP2Link/*.o iAP2Utility/*.o iAP2UtilityImplementation/*.o utils/*.o .depend

$(TARGET): depend $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBRARY) $(LIBRARY_EXT)
	@echo "done"

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif
SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE) $(INCLUDE_EXT)
depend dep:
	$(CC)  -M  $(CFLAGS)   $(INCS) $(SRCS) > .depend
