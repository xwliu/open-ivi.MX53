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

#ifndef __TRACTOR_CANBOX_HAL_H
#define __TRACTOR_CANBOX_HAL_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <hardware/hardware.h>
#include <cutils/native_handle.h>
#include <linux/ioctl.h>
#include "canboxprotocal.h"
/**
 * The id of this module
 */
#define CANBOX_HARDWARE_MODULE_ID "canbox"

/**
 * Name of the sensors device to open
 */
#define CANBOX_HARDWARE_DEVICE    "canbox"


/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
struct canbox_module_t {
    struct hw_module_t common;

	/* module api interface */
};


/**
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
struct canbox_device_t {
    struct hw_device_t common;
	/* device handle */
	int fd;
	/* device api interface */
	int (*canbox_open)(struct canbox_device_t *dev);
	/* get canbox notification */
	int (*canbox_get_notification)(struct canbox_device_t *dev, canbox_notification* p_notification); 
	int (*canbox_send_command)(struct canbox_device_t *dev, canbox_command* p_command);
};

/** convenience API for opening and closing a device */
static inline int canbox_control_open(const struct hw_module_t* module, struct canbox_device_t** device) 
{
    return module->methods->open(module, CANBOX_HARDWARE_DEVICE, (struct hw_device_t**)device);
}

static inline int canbox_control_close(struct canbox_device_t* device) 
{
    return device->common.close(&device->common);
}

#endif 
