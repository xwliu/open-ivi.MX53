ifeq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := \
		lib_omx_ac3_dec_v2_arm11_elinux.so \
		lib_ac3_dec_v2_arm11_elinux.so

LOCAL_MODULE_TAGS := eng
include $(BUILD_MULTI_PREBUILT)

endif