/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __TRACTOR_CARSTATUS_RADIO_HAL_H
#define __TRACTOR_CARSTATUS_RADIO_HAL_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>
#include <cutils/native_handle.h>

#include <linux/ioctl.h>


/**
 * The id of this module
 */
#define CARSTATUS_HARDWARE_MODULE_ID "CarStatus"

/**
 * Name of the sensors device to open
 */
#define CARSTATUS_HARDWARE_DEVICE    "CarStatusDevice"


/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
struct carstatus_module_t {
    struct hw_module_t common;

	/* module api interface */
};


/**
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
struct carstatus_device_t {
    struct hw_device_t common;

	/* device handle */
	int fd;
	
	/* device api interface */
	int (*carstatus_open)(struct carstatus_device_t *dev);
	
	int (*carstatus_poll_car_status)(struct carstatus_device_t *dev,unsigned int* carstatus_status);
	
	int (*carstatus_get_status)(struct carstatus_device_t *dev,unsigned char * status);

	int (*carstatus_notify_ready)(struct carstatus_device_t *dev);

	int (*carstatus_get_battery_vol)(struct carstatus_device_t *dev,unsigned char * vol_real);
	int (*carstatus_get_battery_high)(struct carstatus_device_t *dev,unsigned char * vol_high);
	int (*carstatus_get_battery_low)(struct carstatus_device_t *dev,unsigned char * vol_low);
	int (*carstatus_get_boot_with_media_card)(struct carstatus_device_t *dev,unsigned char * status);
	int (*carstatus_get_boot_with_media_usb)(struct carstatus_device_t *dev,unsigned char * status);
	int (*carstatus_setting_spec)(struct carstatus_device_t *dev,char * setting_val);
	int (*carstatus_get_touchpanel_type)(struct carstatus_device_t *dev,unsigned int * touchpanel_type);
	int (*carstatus_get_ipod_status)(struct carstatus_device_t *dev,unsigned int * p_ipod_status);
	int (*carstatus_set_usb_debug_mode)(struct carstatus_device_t *dev, unsigned int * p_reserve);
	int (*carstatus_get_boot_with_media_ipod)(struct carstatus_device_t *dev,unsigned char * status);    
};

#define CARSTATUS_GET_STATUS        _IOWR('F', 0xa0, void*)
#define CARSTATUS_POP_STATUS        _IOWR('F', 0xa1, void*)
#define CARSTATUS_NOTIFY_READY        _IOWR('F', 0xa2, void*)
#define CARSTATUS_BATTERY_VOL        _IOWR('F', 0xa3, void*)
#define CARSTATUS_BATTERY_HIGH        _IOWR('F', 0xa4, void*)
#define CARSTATUS_BATTERY_LOW        _IOWR('F', 0xa5, void*)
#define CARSTATUS_BOOT_WITH_MEDIA_CARD _IOWR('F', 0xa6, void*)
#define CARSTATUS_BOOT_WITH_MEDIA_USB _IOWR('F', 0xa7, void*)
#define CARSTATUS_SETTING_SPEC		 _IOWR('F', 0xa8, void*)
#define CARSTATUS_GET_TOUCHPANEL_TYPE	_IOWR('F', 0xa9, void*)
#define CARSTATUS_GET_IPOD_CONNECT_STATUS	_IOWR('F', 0xaa, void*)
#define CARSTATUS_SET_USB_DEBUG_MODE	_IOWR('F', 0xab, void*)
#define CARSTATUS_BOOT_WITH_MEDIA_IPOD	_IOWR('F', 0xac, void*)
/** convenience API for opening and closing a device */

static inline int carstatus_control_open(const struct hw_module_t* module, struct carstatus_device_t** device) 
{
    return module->methods->open(module, CARSTATUS_HARDWARE_DEVICE, (struct hw_device_t**)device);
}

static inline int carstatus_control_close(struct carstatus_device_t* device) 
{
    return device->common.close(&device->common);
}

#endif  // ANDROID_CARSTATUS_HAL_H

