
LOCAL_PATH := $(call my-dir)

um_target := $(TARGET_OUT)/bin


um_file := usb_modeswitch
#
# Application Firmware
include $(CLEAR_VARS)
LOCAL_MODULE := $(um_file)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(um_target)
LOCAL_MODULE_TAGS := eng
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


