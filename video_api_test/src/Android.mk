LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
USE_ION_ALLOCATOR	:= true
USE_DRM				:= false
USE_CAMERA			:= false

NX_HW_TOP 		:= $(TOP)/hardware/nexell/s5pxx18
NX_HW_INCLUDE	:= $(NX_HW_TOP)/include
OMX_TOP			:= $(NX_HW_TOP)/omx
FFMPEG_PATH		:= $(OMX_TOP)/codec/ffmpeg

LOCAL_SRC_FILES	:= \
	MediaExtractor.cpp		\
	CodecInfo.cpp			\
	NX_Queue.cpp			\
	NX_Semaphore.cpp		\
	NX_AndroidRenderer.cpp	\
	Util.cpp				\
	VideoDecTest.cpp		\
	VideoEncTest.cpp		\
	NX_V4l2Utils.cpp		\
	main.cpp

LOCAL_C_INCLUDES += \
	$(TOP)/frameworks/native/include		\
	$(TOP)/system/core/include				\
	$(TOP)/hardware/libhardware/include	\
	$(TOP)/device/nexell/library/nx-video-api/src \
	$(TOP)/device/nexell/library/nx-video-api/src/include \
	$(TOP)/device/nexell/library/nx-video-api/src/include/linux \
	$(NX_HW_TOP)/gralloc

LOCAL_C_INCLUDES_32 += \
	$(FFMPEG_PATH)/32bit/include

LOCAL_C_INCLUDES_64 += \
	$(FFMPEG_PATH)/64bit/include

LOCAL_SHARED_LIBRARIES := \
	libcutils		\
	libbinder		\
	libutils		\
	libgui			\
	libui			\
	libion			\
	libhardware		\
	libnx_video_api

LOCAL_LDFLAGS_32 += \
	-L$(FFMPEG_PATH)/32bit/libs	\
	-lavutil 			\
	-lavcodec  		\
	-lavformat		\
	-lavdevice		\
	-lavfilter		\
	-lswresample

LOCAL_LDFLAGS_64 += \
	-L$(FFMPEG_PATH)/64bit/libs	\
	-lavutil 			\
	-lavcodec  		\
	-lavformat		\
	-lavdevice		\
	-lavfilter		\
	-lswresample

LOCAL_32_BIT_ONLY := true

LOCAL_CFLAGS :=

ifeq ($(USE_ION_ALLOCATOR),true)
LOCAL_CFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) -DUSE_ION_ALLOCATOR
endif

ifeq ($(USE_DRM),true)
LOCAL_CFLAGS += -DENABLE_DRM_DISPLAY=1
else
LOCAL_CFLAGS += -DENABLE_DRM_DISPLAY=0
endif

ifeq ($(USE_CAMERA),true)
LOCAL_SRC_FILES += NX_CV4l2Camera.cpp
LOCAL_CFLAGS += -DENABLE_CAMERA=1
else
LOCAL_CFLAGS += -DENABLE_CAMERA=0
endif

LOCAL_MODULE := video_api_test
LOCAL_MODULE_PATH := $(LOCAL_PATH)

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
