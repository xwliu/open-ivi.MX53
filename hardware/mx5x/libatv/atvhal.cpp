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
#define LOG_TAG "ATVHAL"

#include <hardware/hardware.h>
#include <hardware/atvhal.h>

#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <linux/input.h>
#include <utils/Log.h>
#include <cutils/atomic.h>

#define ATV_INIT                             _IOWR('A', 0xa0, void*)
#define ATV_SET_FREQ                         _IOWR('A', 0xa1, void*)
#define ATV_AUTO_SCAN                        _IOWR('A', 0xa2, void*)
#define ATV_SEEK                             _IOWR('A', 0xa3, void*)
#define ATV_FINE_TUNE                        _IOWR('A', 0xa4, void*)
#define ATV_GET_TV_STATUS                    _IOWR('A', 0xa5, void*)
#define ATV_GET_NUM_CHANNELS                 _IOWR('A', 0xa6, void*)
#define ATV_GET_CHANNEL                      _IOWR('A', 0xa7, void*)
#define ATV_GET_CURR_FREQ                    _IOWR('A', 0xa8, void*)
#define ATV_IS_AVAIL                         _IOWR('A', 0xa9, void*)
#define ATV_SELECT_STANDARD                  _IOWR('A', 0xaa, void*)

static int file_handle = 0;

static int atv_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);

static struct hw_module_methods_t atv_module_methods = {
    open: atv_device_open
};

struct atv_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
		id: ATV_HARDWARE_MODULE_ID,
		name: "atv module",
		author: "TokenWireless",
		methods: &atv_module_methods,
	},
	/* module api interface */
};

static int atv_open(struct atv_device_t *dev)
{	
	LOGI("atv_open\r\n");

	file_handle = open("/dev/TNJ3324", O_RDWR);
	if(file_handle == -1){
		LOGI("atv_open open fail\r\n");
		return -1;
	}
	return 0;
}

static int atv_device_close(struct hw_device_t *device)
{
    struct atv_device_t *dev = (struct atv_device_t*)device;
    if(dev){
        free(dev);
    }
    return 0;
}

static int atv_poll(struct atv_device_t *dev, int *status)
{
	int ret = 0;
	struct pollfd event;

	while(1){
		memset(&event, 0x00, sizeof(event));
		event.fd = file_handle;
		event.events = 1;

		ret = poll((struct pollfd *)&event, 1, 2000);
		if(ret < 0){
			LOGI("atv_poll poll fail\r\n");
			break;
		}
		else if(ret == 0){
			LOGI("atv_poll poll timeout\r\n");
			continue;
		}
		
		if(event.revents & POLLERR){
			LOGI("atv_poll device error\r\n");
			break;
		}
		else if(event.revents & POLLIN){
			LOGI("atv_poll ready\r\n");
			int stat;
			ioctl(file_handle, ATV_GET_TV_STATUS, &stat);

			LOGI("atv_poll ready, stat:%d\r\n", stat);
			*status = stat;
			break;
		}
	}

	return ret;
}

static int setATVFreq(struct atv_device_t *dev, int freq)
{
	int ret = 0;
	int params[2];
	
	params[0] = freq;

	LOGI("setATVFreq\n");
	
	ret = ioctl(file_handle, ATV_SET_FREQ, params);
	
	return ret;
}

static int prevPoint(struct atv_device_t *dev)
{
	int ret = 0;
	int upDown = 0;
	
	LOGI("prevPoint\n");
	
	ret = ioctl(file_handle, ATV_SEEK, &upDown);
	
	return ret;
}

static int nextPoint(struct atv_device_t *dev)
{
	int ret = 0;
	int upDown = 1;
	
	LOGI("nextPoint\n");
	
	ret = ioctl(file_handle, ATV_SEEK, &upDown);
	
	return ret;
}

static int autoScan(struct atv_device_t *dev, int start)
{
	int ret = 0;
	
	LOGI("autoScan\n");
	
	ret = ioctl(file_handle, ATV_AUTO_SCAN, &start);
	
	return ret;
}

static int fineTune(struct atv_device_t *dev, int up)
{
	int ret = 0;
	
	LOGI("fineTune\n");
	
	ret = ioctl(file_handle, ATV_FINE_TUNE, &up);
	
	return ret;
}

static int getNumChannels(struct atv_device_t *dev)
{
	int ret = 0;
	int num = 0;
	
	LOGI("getNumChannels\n");
	
	ret = ioctl(file_handle, ATV_GET_NUM_CHANNELS, &num);
	
	return num;
}

static int getTVStatus(struct atv_device_t *dev)
{
	int ret = 0;
	int status = 0;
	
	LOGI("getTVStatus\n");
	
	ret = ioctl(file_handle, ATV_GET_TV_STATUS, &status);
	
	return status;
}

static int getChannel(struct atv_device_t *dev, int channel, int *frequency)
{
	int ret = 0;
	int param[2] = {0};
	
	LOGI("getChannel\n");
	param[0] = channel;
	ret = ioctl(file_handle, ATV_GET_CHANNEL, param);

	*frequency = param[1];
	
	return ret;
}

static int getCurFreq(struct atv_device_t *dev)
{
	int ret = 0;
	int freq = 0;
	
	LOGI("getCurFreq\n");

	ret = ioctl(file_handle, ATV_GET_CURR_FREQ, &freq);
	
	return freq;
}

static int isTVAvail(struct atv_device_t *dev)
{
	int ret = 0;
	int avail = 0;
	
	LOGI("isTVAvail\n");

	ret = ioctl(file_handle, ATV_IS_AVAIL, &avail);
	
	return avail;
}

static int selectStandard(struct atv_device_t *dev, int standard)
{
	int ret = 0;
	
	LOGI("selectStandard\n");

	ret = ioctl(file_handle, ATV_SELECT_STANDARD, &standard);
	
	return ret;
}

static int atv_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
    int status = -EINVAL;
    struct atv_device_t *dev;
    LOGI("atv_device_open\n");
    dev = (struct atv_device_t*)malloc(sizeof(*dev));
    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));
    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = atv_device_close;
    dev->atv_open = atv_open;
	dev->atv_setFreq = setATVFreq;
	dev->atv_prevPoint = prevPoint;
	dev->atv_nextPoint = nextPoint;
	dev->atv_auto_scan = autoScan;
	dev->atv_fine_tune = fineTune;
	dev->atv_get_tv_status = getTVStatus;
	dev->atv_get_num_channels = getNumChannels;
	dev->atv_get_channel = getChannel;
	dev->atv_poll = atv_poll;
	dev->atv_getCurFreq = getCurFreq;
	dev->atv_isTVAvail = isTVAvail;
	dev->atv_selectStandard = selectStandard;
		
    *device = &dev->common;
    status = 0;
    return status;
}
