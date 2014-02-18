# This is a TokenWireless Android Design platform based on i.MX53 board
# It will inherit from FSL core product which in turn inherit from Google generic

$(call inherit-product, device/tokenwireless/imx5x/imx5x.mk)

# Overrides
PRODUCT_NAME := imx53_tractor
PRODUCT_DEVICE := imx53_tractor

PRODUCT_COPY_FILES += \
	device/tokenwireless/imx53_tractor/required_hardware.xml:system/etc/permissions/required_hardware.xml \
	device/tokenwireless/imx53_tractor/init.rc:root/init.freescale.rc \
       device/tokenwireless/imx53_tractor/vold.fstab:system/etc/vold.fstab \
       device/tokenwireless/imx53_tractor/asound.conf:system/etc/asound.conf \
       external/bluetooth/bluez/audio/audio.conf:system/etc/bluetooth/audio.conf \
       device/tokenwireless/imx53_tractor/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
       device/tokenwireless/imx53_tractor/easyconnect/adb:system/xbin/adb \
       device/tokenwireless/imx53_tractor/easyconnect/usb_modeswitch:system/xbin/usb_modeswitch
       
PRODUCT_PACKAGES += \
	com.tokenwireless.tractor.sdkaddon \
	com.tokenwireless.tractor.mpeg \
	com.tokenwireless.tractor.ipod \
	com.tokenwireless.tractor.pbap \
	com.tokenwireless.tractor.carstatus \
	com.tokenwireless.tractor.audiomonitor \
	com.tokenwireless.tractor.canbox \
	com.tokenwireless.tractor.fmradio \
	com.tokenwireless.tractor.atv
	
	
PRODUCT_COPY_FILES += \
	device/tokenwireless/app/NativeSettings.apk:system/app/NativeSettings.apk \
	device/tokenwireless/app/SettingsProvider.apk:system/app/SettingsProvider.apk \
	device/tokenwireless/app/SystemUI.apk:system/app/SystemUI.apk
	
ifeq ($(GOOGLE_PLAY_STORE_ENABLE),true)	
PRODUCT_COPY_FILES += \
	device/tokenwireless/app/GooglePlayStore3_8_15.apk:system/app/GooglePlayStore3_8_15.apk \
	device/tokenwireless/app/GoogleServicesFramework.apk:system/app/GoogleServicesFramework.apk
endif
	
conf3g_src_files := $(shell ls external/usb_modeswitch/conf3g/ )
PRODUCT_COPY_FILES += $(foreach file, $(conf3g_src_files), \
        external/usb_modeswitch/conf3g/$(file):system/etc/conf3g/$(file))
	
$(warning Makeing Target: $(MAKECMDGOALS))
ifeq ($(strip $(MAKECMDGOALS)),sdk_addon)
PRODUCT_SDK_ADDON_STUB_DEFS := device/tokenwireless/sdk_addon/sdkaddon.defs
   
PRODUCT_COPY_FILES += \
    device/tokenwireless/framework/PlatformLibrary/com.tokenwireless.tractor.sdkaddon.xml:system/etc/permissions/com.tokenwireless.tractor.sdkaddon.xml
   
# name of the add-on
PRODUCT_SDK_ADDON_NAME := com.tokenwireless.tractor.sdkaddon

# Copy the jar files for the optional libraries that are exposed as APIs.
PRODUCT_SDK_ADDON_COPY_MODULES := \
    com.tokenwireless.tractor.sdkaddon:libs/com.tokenwireless.tractor.sdkaddon.jar  

# Copy the manifest and hardware files for the SDK add-on.
# The content of those files is manually created for now.
PRODUCT_SDK_ADDON_COPY_FILES := \
    device/tokenwireless/sdk_addon/manifest.ini:manifest.ini \
    device/tokenwireless/sdk_addon/hardware.ini:hardware.ini \
    
# Name of the doc to generate and put in the add-on. This must match the name defined
# in the optional library with the tag
#    LOCAL_MODULE:= platform_library
# in the documentation section.
PRODUCT_SDK_ADDON_DOC_MODULE := com.tokenwireless.tractor.sdkaddon

$(warning sdk ENV ready)
endif

ifeq ($(strip $(BSP_HARDWARE_VERSION)), TRACTOR_HARDWARE_TRACTORPLUSOSV)
RADIO_CONFIG_FILE := frameworks/base/tractor/FMRadio/config/radio_conf_tractorplusosv.xml
else
RADIO_CONFIG_FILE := frameworks/base/tractor/FMRadio/config/radio_conf_default.xml
endif

PRODUCT_COPY_FILES += \
        $(RADIO_CONFIG_FILE):system/etc/radio_conf.xml


