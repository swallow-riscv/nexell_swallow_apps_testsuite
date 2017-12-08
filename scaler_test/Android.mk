LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
USE_ION_ALLOCATOR	:= true
USE_DRM				:= false
USE_CAMERA			:= false

NX_LIBRARY_TOP := $(TOP)/device/nexell/library
NX_HW_TOP := $(TOP)/hardware/nexell/s5pxx18

LOCAL_SRC_FILES	:= \
	file-test.cpp

LOCAL_C_INCLUDES += \
	$(TOP)/frameworks/native/include		\
	$(TOP)/system/core/include				\
	$(TOP)/hardware/libhardware/include	\
	$(TOP)/device/nexell/library/nx-video-api/src \
	$(TOP)/device/nexell/library/nx-video-api/src/include \
	$(TOP)/device/nexell/library/nx-video-api/src/include/linux \
	$(NX_HW_TOP)/gralloc \
	$(NX_LIBRARY_TOP)/nx-scaler \

LOCAL_SHARED_LIBRARIES := \
	libcutils		\
	libbinder		\
	libutils		\
	libgui			\
	libui			\
	libion			\
	libhardware		\
	libnx_scaler	\
	libnx_video_api

LOCAL_CFLAGS :=

ifeq ($(USE_ION_ALLOCATOR),true)
LOCAL_CFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) -DUSE_ION_ALLOCATOR
endif

LOCAL_MODULE := scaler-test-file
LOCAL_MODULE_PATH := $(LOCAL_PATH)

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
