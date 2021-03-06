# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifeq ($(BOARD_SOC_CLASS),IMX5X)
ifeq ($(USE_CAMERA_STUB),false)
ifeq ($(TARGET_PRODUCT),imx53_tractor)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=    \
	TractorCameraHal.cpp    \
	TractorCamera_pmem.cpp  \
	TractorCaptureDeviceInterface.cpp \
	TractorV4l2CsiDevice.cpp \
	TractorV4l2CapDeviceBase.cpp  \	

LOCAL_CPPFLAGS +=

LOCAL_SHARED_LIBRARIES:= \
    libcamera_client \
    libui \
    libutils \
    libcutils \
    libbinder \
    libmedia \
    libhardware_legacy \
    libdl \
    libc \
    libipu

LOCAL_C_INCLUDES += \
	frameworks/base/include/binder \
	frameworks/base/include/ui \
	frameworks/base/camera/libcameraservice \
	external/linux-lib/ipu
	
LOCAL_MODULE:= libcamera

LOCAL_CFLAGS += -fno-short-enums

LOCAL_MODULE_TAGS := eng

include $(BUILD_SHARED_LIBRARY)
endif
endif
endif