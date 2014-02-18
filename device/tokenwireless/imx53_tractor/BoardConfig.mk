#
# Product-specific compile-time definitions.
#

include device/tokenwireless/imx5x/BoardConfigCommon.mk

TARGET_BOOTLOADER_BOARD_NAME := Tractor
BOARD_WLAN_RDA_COMBO := true



BOARD_SOC_CLASS := IMX5X
BOARD_SOC_TYPE := IMX53

TARGET_BUILD_KERNEL := false
ifeq ($(BOARD_WLAN_RDA_COMBO), true)
BOARD_HAVE_WIFI := true
BOARD_WPA_SUPPLICANT_DRIVER := WEXT
WPA_SUPPLICANT_VERSION      := VER_0_6_X
HAVE_CUSTOM_WIFI_DRIVER_2 := true
endif
#WIFI_DRIVER_MODULE_NAME     := "ar6000"
#WPA_SUPPLICANT_VERSION      := VER_0_6_ATHEROS
#BOARD_WLAN_ATHEROS_SDK      := system/wlan/atheros/AR6kSDK
#WPA_SUPPLICANT_VERSION      := VER_0_6_ATHEROS
#BOARD_WPA_SUPPLICANT_DRIVER := AR6000
#BOARD_WLAN_CHIP_AR6102	    := true
#BOARD_WLAN_CHIP_AR6003	    := true
#BOARD_WPA_SUPPLICANT_DRIVER := WEXT
# Select Wake on wireless mode for AR6003 suspend/resume policy
#BOARD_WLAN_PM_SUSPEND       := 2

BOARD_HAVE_VPU := true
HAVE_FSL_IMX_GPU := true
HAVE_FSL_IMX_IPU := true

# Ripley (MC34708) pmic
TARGET_TS_DEVICE_ALT := "mxc_ts"


#for accelerator sensor, need to define sensor type here
BOARD_HAS_SENSOR := false
SENSOR_MMA8450 := false

# for recovery service
TARGET_SELECT_KEY := 28
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 209715200
# Used to create the system partition (even if we are not using flash)
BOARD_FLASH_BLOCK_SIZE := 262144

# TokenWireless Feature
BOARD_USES_TOKENWIRELESS_CONSOLE_KEY := true
BOARD_USES_TOKENWIRELESS_PREBUILT_CARAUDIO := true
#enable RDA_5990 supporting jiteng 2012/7/18
RDA_BT_SUPPORT := true
#RDA_WLAN_SUPPORT = yes
RDA_WLAN_CHIP := RDA5990
ifeq ($(RDA_BT_SUPPORT), true)
BOARD_HAVE_BLUETOOTH := true
endif

include device/tokenwireless/imx53_tractor/Factory.mk
include device/tokenwireless/imx53_tractor/GooglePlayStore.mk
