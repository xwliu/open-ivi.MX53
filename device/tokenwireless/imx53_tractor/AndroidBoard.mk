LOCAL_PATH := $(call my-dir)

include device/tokenwireless/imx5x/AndroidBoardCommon.mk

ifeq ($(PREBUILT_FSL_IMX_CODEC),true)
include device/tokenwireless/proprietary/codec/fsl-codec.mk
endif

include device/tokenwireless/proprietary/gpu/fsl-gpu.mk
include device/tokenwireless/proprietary/omx/fsl-omx.mk
include device/tokenwireless/proprietary/render/fsl-render.mk

