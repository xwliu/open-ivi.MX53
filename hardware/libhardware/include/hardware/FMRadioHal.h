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

#ifndef __TRACTOR_FM_RADIO_HAL_H
#define __TRACTOR_FM_RADIO_HAL_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>
#include <cutils/native_handle.h>

#include <linux/ioctl.h>

#include "main.h"
#include "RDA5990_FM_drv.h"
#include "radio.h"


/**
 * The id of this module
 */
#define FM_HARDWARE_MODULE_ID "FMRadio"

/**
 * Name of the sensors device to open
 */
#define FM_HARDWARE_DEVICE    "FMRadioDevice"


/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
struct fm_module_t {
    struct hw_module_t common;

	/* module api interface */
};


/**
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
struct fm_device_t {
    struct hw_device_t common;

	/* device handle */
	int fd;
	
	/* device api interface */
	int (*fm_open)(struct fm_device_t *dev);

	int (*fm_init_fm)(struct fm_device_t *dev);

	int (*fm_init_am)(struct fm_device_t *dev);

	int (*fm_radio_jump_freq)(struct fm_device_t *dev, int freq);

	int (*fm_radio_seek)(struct fm_device_t *dev, bool seek_up_down);

	int (*fm_radio_switch_mode)(struct fm_device_t *dev, char radio_mode);

	int (*fm_radio_as)(struct fm_device_t *dev);

	int (*fm_radio_loc)(struct fm_device_t *dev, bool loc);
	
	int (*fm_radio_stereo)(struct fm_device_t *dev, bool stereo);
	
	int (*fm_search_stop)(struct fm_device_t *dev);
	
	int (*fm_get_curr_conf)(struct fm_device_t *dev, int *conf_array_t);
	
	int (*fm_set_curr_conf)(struct fm_device_t *dev, int *conf_array_t);

	int (*fm_rda_power_onoff)(struct fm_device_t *dev, bool on);

	int (*fm_set_rds_enable)(struct fm_device_t *dev, bool enable);

	int (*fm_set_rds_freq)(struct fm_device_t *dev, short freq);

	int (*fm_poll_rds_group)(struct fm_device_t *dev, radio_rds_group *radio_rds_data);

	int (*fm_get_runtime_state)(struct fm_device_t *dev, radio_pointer_info *state);

	int (*fm_get_tune_results)(struct fm_device_t *dev, tune_result *results);

	int (*fm_radio_as_pty)(struct fm_device_t *dev, int pty);

	int (*fm_radio_seek_pty)(struct fm_device_t *dev, bool up, int pty);

	int (*fm_tef6621_rds_enable)(struct fm_device_t *dev, bool enable);

	int (*fm_radio_seek_tp)(struct fm_device_t *dev, bool up);

	int (*fm_radio_get_level)(struct fm_device_t *dev);

	int (*fm_radio_get_frequency)(struct fm_device_t *dev);

	int (*fm_get_tuning_results)(struct fm_device_t *dev, tune_result *results);
	
};


/** convenience API for opening and closing a device */

static inline int fm_control_open(const struct hw_module_t* module, struct fm_device_t** device) 
{
    return module->methods->open(module, FM_HARDWARE_DEVICE, (struct hw_device_t**)device);
}

static inline int fm_control_close(struct fm_device_t* device) 
{
    return device->common.close(&device->common);
}

#endif  // ANDROID_FM_HAL_H

