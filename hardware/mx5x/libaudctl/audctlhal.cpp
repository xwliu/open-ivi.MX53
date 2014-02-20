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
#define LOG_TAG "AUDCTLHAL"

#include <hardware/hardware.h>
#include <hardware/audctlhal.h>

#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <linux/input.h>
#include <utils/Log.h>
#include <cutils/atomic.h>

#define UDA1388_ADJUST_FB_VOLUME                  _IOWR('U', 0xa2, void*)

static int file_handle = 0;

static int audctl_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);

static struct hw_module_methods_t audctl_module_methods = {
    open: audctl_device_open
};

struct audctl_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
		id: AUDCTL_HARDWARE_MODULE_ID,
		name: "audio control module",
		author: "TokenWireless",
		methods: &audctl_module_methods,
	},
	/* module api interface */
};

static int audctl_open(struct audctl_device_t *dev)
{	
	LOGI("audctl_open\r\n");

	file_handle = open("/dev/uda1388_1", O_RDWR);
	if(file_handle == -1){
		LOGI("audctl_open open fail\r\n");
		return -1;
	}
	return 0;
}

static int audctl_device_close(struct hw_device_t *device)
{
    struct audctl_device_t *dev = (struct audctl_device_t*)device;
    if(dev){
        free(dev);
    }
    return 0;
}

static int get_FbChnNum(struct audctl_device_t *dev)
{
	int ret = 4;
	return ret;
}

static int get_FbChnRangeMax(struct audctl_device_t *dev, int fbchn)
{
	int ret = 9;
	return ret;
}

static int adjFbVolume(struct audctl_device_t *dev, int fbchn, int vol)
{
	int ret = 0;
	int params[2];
	params[0] = fbchn;
	params[1] = vol;

	LOGI("adjFbVolume\n");
	
	ret = ioctl(file_handle, UDA1388_ADJUST_FB_VOLUME, params);
	
	return ret;
}

static int audctl_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
    int status = -EINVAL;
    struct audctl_device_t *dev;
    LOGI("audctl_device_open\n");
    dev = (struct audctl_device_t*)malloc(sizeof(*dev));
    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));
    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = audctl_device_close;
    dev->audctl_open = audctl_open;
	dev->get_FbChnNum = get_FbChnNum;
	dev->get_FbChnRangeMax = get_FbChnRangeMax;
	dev->adjFBVolume = adjFbVolume;
	
    *device = &dev->common;
    status = 0;
    return status;
}
