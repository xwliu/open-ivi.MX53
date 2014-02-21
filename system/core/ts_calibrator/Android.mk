# Copyright 2005 The Android Open Source Project
#
# Android.mk for ts_calibrator
#

ifeq ($(TARGET_TS_CALIBRATION),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ts_calibrator.c

LOCAL_CFLAGS += -DTS_DEVICE=$(TARGET_TS_DEVICE)

ifdef TARGET_TS_DEVICE_ALT
LOCAL_CFLAGS += -DTS_DEVICE_ALT=$(TARGET_TS_DEVICE_ALT)
endif

ifdef IPOD_FEATURE
LOCAL_CFLAGS += -DIPOD_FEATURE
endif
LOCAL_MODULE := ts_calibrator
LOCAL_MODULE_TAGS := eng

LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES += libcutils libc

LOCAL_C_INCLUDES += kernel_imx/drivers/mxc/carstatus/

include $(BUILD_EXECUTABLE)

endif
