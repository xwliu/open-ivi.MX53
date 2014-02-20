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
#define LOG_TAG "MCUHAL"

#include <hardware/hardware.h>
#include <hardware/mcuhal.h>

#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <linux/input.h>
#include <utils/Log.h>
#include <cutils/atomic.h>
#include "mcu_ioctl.h"

static int file_handle = 0;

static int mcu_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);

static struct hw_module_methods_t mcu_module_methods = {
    open: mcu_device_open
};

struct mcu_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
		id: MCU_HARDWARE_MODULE_ID,
		name: "mcu i2c module",
		author: "TokenWireless",
		methods: &mcu_module_methods,
	},
	/* module api interface */
};

static int mcu_open(struct mcu_device_t *dev)
{	
	LOGI("MCUHAL mcu_open\r\n");

	file_handle = open("/dev/mcu_i2c", O_RDWR);
	if(file_handle == -1){
		LOGI("MCUHAL open fail\r\n");
		return -1;
	}
	return 0;
}

static int mcu_device_close(struct hw_device_t *device)
{
    struct mcu_device_t *dev = (struct mcu_device_t*)device;
    if(dev){
        free(dev);
    }
    return 0;
}

static int mcu_get_version(struct mcu_device_t *dev, int* hardware_ver, int* software_ver)
{
	int ret = 0;
	LOGI("MCUHAL get Version\n");
	if(file_handle == 0){
		LOGE("MCUHAL Device Not Open!\n");
		return -EINVAL;
	}
	MCU_VERSION ver = {0,0};
	ret = ioctl(file_handle, MCU_GET_VERSION, &ver);
	*hardware_ver = ver.hardware_ver;
	*software_ver = ver.software_ver;
	return ret;
}

static int mcu_main_board_uniqueid(struct mcu_device_t *dev, void *uniqueid_data)
{
	int ret = 0;
	MCU_UNIQUEID_DATA *uniqueid_data_tmp = (MCU_UNIQUEID_DATA *)uniqueid_data;
	LOGI("Try to get Main board uniqueid\n");
	if(file_handle == 0){
		LOGE("MCUHAL Device Not Open!\n");
		return -EINVAL;
	}
	ret = ioctl(file_handle, MAIN_BOARD_UNIQUEID, uniqueid_data_tmp);
	return ret;
}

static int mcu_get_batch(struct mcu_device_t *dev, int* batch_val)
{
	int ret = 0;
	int batch_data = 0;
	LOGI("MCUHAL get batch number\n");
	if(file_handle == 0){
		LOGE("MCUHAL Device Not Open!\n");
		return -EINVAL;
	}
	ret = ioctl(file_handle, MCU_BATCH, &batch_data);
	*batch_val = batch_data;
	return ret;
}

static int mcu_get_serial(struct mcu_device_t *dev, int* serial_val)
{
	int ret = 0;
	int serial_data = 0;
	LOGI("MCUHAL get serial number\n");
	if(file_handle == 0){
		LOGE("MCUHAL Device Not Open!\n");
		return -EINVAL;
	}
	ret = ioctl(file_handle, MCU_SERIAL, &serial_data);
	*serial_val = serial_data;
	return ret;
}


static int mcu_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
    int status = -EINVAL;
    struct mcu_device_t *dev;
    LOGI("mcu_device_open\n");
    dev = (struct mcu_device_t*)malloc(sizeof(*dev));
    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));
    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = mcu_device_close;
    dev->mcu_open = mcu_open;
    dev->mcu_get_version = mcu_get_version;
    dev->mcu_main_board_uniqueid = mcu_main_board_uniqueid;
	dev->mcu_get_batch = mcu_get_batch;
	dev->mcu_get_serial = mcu_get_serial;
    *device = &dev->common;
    status = 0;
    return status;
}
