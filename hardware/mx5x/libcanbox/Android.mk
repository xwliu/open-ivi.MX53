
LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# hw/<GPS_HARDWARE_MODULE_ID>.<ro.product.board>.so

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE := canbox.default

LOCAL_SRC_FILES := canbox.cpp

LOCAL_MODULE_TAGS := optional


LOCAL_SHARED_LIBRARIES += liblog \
					libutils \
					libcutils \
					libc \
					libhardware

include $(BUILD_SHARED_LIBRARY)

