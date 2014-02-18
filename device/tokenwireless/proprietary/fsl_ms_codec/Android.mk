ifeq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := \
		lib_omx_wma_dec_v2_arm11_elinux.so \
		lib_omx_wmv_dec_v2_arm11_elinux.so \
		lib_asf_parser_arm11_elinux.3.0.so \
		lib_wma10_dec_v2_arm12_elinux.so \
		lib_WMV789_dec_v2_arm11_elinux.so

LOCAL_MODULE_TAGS := eng
include $(BUILD_MULTI_PREBUILT)

endif
