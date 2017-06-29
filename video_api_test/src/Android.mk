LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
USE_DRM			:= false

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
	main.cpp

#LOCAL_SRC_FILE	+= \
#	VideoEncTest.cpp

#LOCAL_SRC_FILES	+=	\
#	VpuJpgTest.cpp		\
#	img_proc_main.cpp

LOCAL_C_INCLUDES += \
	$(TOP)/frameworks/native/include		\
	$(TOP)/system/core/include				\
	$(TOP)/hardware/libhardware/include	\
	$(TOP)/device/nexell/library/nx-video-api/src \
	$(TOP)/device/nexell/library/nx-video-api/src/include \
	$(TOP)/device/nexell/library/nx-video-api/src/include/linux \
	$(NX_HW_TOP)/gralloc

#	$(LOCAL_PATH)/ffmpeg/include	\
#	$(LOCAL_PATH)
#	linux/platform/$(TARGET_CPU_VARIANT2)/library/include	\

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

#LOCAL_SHARED_LIBRARIES += \
#	libnx_deinterlace	\
#	libnxgraphictools

#LOCAL_STATIC_LIBRARIES := \
#	libnxmalloc

#LOCAL_STATIC_LIBRARIES += \
#	libnx_dsp \

#LOCAL_LDFLAGS += \
#	-Llinux/platform/$(TARGET_CPU_VARIANT2)/library/lib	\
#	-L$(LOCAL_PATH)/ffmpeg/libs	\
#	-Lhardware/samsung_slsi/slsiap/omx/codec/ffmpeg/libs \
#	-lavutil-2.1.4 		\
#	-lavcodec-2.1.4 	\
#	-lavformat-2.1.4	\
#	-lavdevice-2.1.4	\
#	-lavfilter-2.1.4	\
#	-lswresample-2.1.4	\
#	-ltheoraparser_and

LOCAL_32_BIT_ONLY := true

#ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 21 && echo OK),OK)
#	LOCAL_MODULE_RELATIVE_PATH := hw
#else
#	LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
#endif

LOCAL_CFLAGS := -DLOG_TAG=\"gralloc\" -DGRALLOC_32_BITS -DSTANDARD_LINUX_SCREEN -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifeq ($(USE_DRM),true)
LOCAL_CFLAGS += -DENABLE_DRM_DISPLAY=1
else
LOCAL_CFLAGS += -DENABLE_DRM_DISPLAY=0
endif

LOCAL_MODULE := video_api_test
LOCAL_MODULE_PATH := $(LOCAL_PATH)

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
