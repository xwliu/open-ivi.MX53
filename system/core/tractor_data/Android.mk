# Copyright 2005 The Android Open Source Project
#
# Android.mk for tractor_data
#

ifeq ($(TARGET_TS_CALIBRATION),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	tractor_data.c

LOCAL_MODULE := tractor_data
LOCAL_MODULE_TAGS := eng

LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES += libcutils libc

include $(BUILD_EXECUTABLE)

endif
