LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:=	\
	iAP2Link/iAP2Link.c				\
	iAP2Link/iAP2FileTransfer.c		\
	iAP2Link/iAP2LinkAccessory.c	\
	iAP2Link/iAP2LinkRunLoop.c		\
	iAP2Link/iAP2Packet.c

LOCAL_SRC_FILES+=	\
	iAP2Utility/iAP2BuffPool.c	\
	iAP2Utility/iAP2FSM.c		\
	iAP2Utility/iAP2ListArray.c

LOCAL_SRC_FILES+=	\
	iAP2UtilityImplementation/iAP2BuffPoolImplementation.c	\
	iAP2UtilityImplementation/iAP2Log.c						\
	iAP2UtilityImplementation/iAP2Time.c

LOCAL_SRC_FILES+=	\
	utils/gpio.c		\
	utils/NX_Semaphore.c 	\
	utils/ux_timer.c

LOCAL_SRC_FILES+=	\
	main.c		\
	iAP_Auth.c 	\
	Cpchip.c	

LOCAL_C_INCLUDES += \
	frameworks/native/include		\
	hardware/libhardware/include  	\
	system/core/include				\
	external/tinyalsa/include		\
	$(LOCAL_PATH)					\
	$(LOCAL_PATH)/utils \
	$(LOCAL_PATH)/iAP2Link			\
	$(LOCAL_PATH)/iAP2Utility		\
	$(LOCAL_PATH)/iAP2UtilityImplementation


LOCAL_SHARED_LIBRARIES := \
	libcutils		\
	libbinder		\
	libutils		\
	libhardware_legacy

LOCAL_STATIC_LIBRARIES +=

LOCAL_LDFLAGS += -g -O1

LOCAL_MODULE:= ipod_service
LOCAL_MODULE_PATH:= $(LOCAL_PATH)

include $(BUILD_EXECUTABLE)
