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

#define LOG_TAG "CarStatus"

#include <hardware/hardware.h>
#include <hardware/CarStatusHal.h>

#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <linux/input.h>
#include <utils/Log.h>
#include <cutils/atomic.h>
#include <time.h>

#define CARSTATUS_DEBUG 0
#if CARSTATUS_DEBUG
#define dprintf(format, arg...) LOGI(format, ## arg)
#else
#define dprintf(format, arg...)
#endif

static int file_handle = 0;

/*****************************************************************************/

static int carstatus_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);

static struct hw_module_methods_t carstatus_module_methods = {
    open: carstatus_device_open
};

struct carstatus_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
		id: CARSTATUS_HARDWARE_MODULE_ID,
		name: "carstatus module",
		author: "lchen@Tokenwireless",
		methods: &carstatus_module_methods,
	},
	/* module api interface */
};

static int carstatus_open(struct carstatus_device_t *dev)
{	
	LOGE("carstatus_open\n");

	file_handle = open("/dev/carstatus", O_RDWR);
	if(file_handle == -1){
		LOGI("HAL carstatus_open, open fail\n");
		return -1;
	}
	
	dprintf("HAL carstatus_open,file_handle:0x%x\n",file_handle);
	return 0;
}

static int carstatus_poll_car_status(struct carstatus_device_t *dev, unsigned int * carstatus_status)
{
	int ret = 0;
	unsigned int status;
	struct pollfd event;
		
	dprintf("carstatus_poll working\n");

	while(1){
		memset(&event, 0x00, sizeof(event));
		event.fd = file_handle;
		event.events = 1;

		ret = poll((struct pollfd *)&event, 1, 100);
		if(ret < 0){
			LOGI("carstatus_poll fail\n");
			return -1;
			break;
		}
		else if(ret == 0){
		//	dprintf("carstatus_poll timeout\n");
			return -2;
		}
		if(event.revents & POLLERR){
			LOGI("carstatus_poll device error\n");
			break;
		}
		else if(event.revents & POLLIN){
			ret = ioctl(file_handle, CARSTATUS_POP_STATUS, &status);
			if(ret <0)
				return ret;
			*carstatus_status = status;
			//LOGI("carstatus_poll_car_status  in status =0x%x \n",status);
			
			break;
		}
	}

	return ret;
}

static int carstatus_get_status(struct carstatus_device_t *dev,unsigned char * status)
{
	int ret = 0;
	unsigned char status_t;		
	ret = ioctl(file_handle, CARSTATUS_GET_STATUS, &status_t);
	if(ret <0)
		return ret;
	*status = status_t;
	dprintf("carstatus_get_status  in status =0x%x \n",status_t);
	return 0;
}

static int carstatus_get_battery_vol(struct carstatus_device_t *dev,unsigned char * vol_real)
{
	int ret = 0;
	unsigned char vol_real_t;		
	ret = ioctl(file_handle, CARSTATUS_BATTERY_VOL, &vol_real_t);
	if(ret <0)
		return ret;
	*vol_real = vol_real_t;
	dprintf("carstatus_get_battery_vol  in status =0x%x \n",vol_real_t);
	return 0;
}

static int carstatus_get_battery_high(struct carstatus_device_t *dev,unsigned char * vol_high)
{
	int ret = 0;
	unsigned char vol_high_t;		
	ret = ioctl(file_handle, CARSTATUS_BATTERY_HIGH, &vol_high_t);
	if(ret <0)
		return ret;
	*vol_high = vol_high_t;
	dprintf("carstatus_get_battery_vol  in status =0x%x \n",vol_high_t);
	return 0;
}

static int carstatus_get_boot_with_media_card(struct carstatus_device_t *dev,unsigned char * card_status)
{
	int ret = 0;
	unsigned char status;
	ret = ioctl(file_handle, CARSTATUS_BOOT_WITH_MEDIA_CARD, &status);
	if(ret <0)
		return ret;
	*card_status = status;
	dprintf("carstatus_get_boot_with_media_card  in status =0x%x \n", status);
	return 0;
}
static int carstatus_get_boot_with_media_usb(struct carstatus_device_t *dev,unsigned char * usb_status)
{
	int ret = 0;
	unsigned char status;
	ret = ioctl(file_handle, CARSTATUS_BOOT_WITH_MEDIA_USB,  &status);
	if(ret <0)
		return ret;
	*usb_status = status;
	dprintf("carstatus_get_boot_with_media_usb  in status =0x%x \n", status);
	return 0;
}
static int carstatus_get_boot_with_media_ipod(struct carstatus_device_t *dev,unsigned char * ipod_status)
{
	int ret = 0;
	unsigned char status;
	//dprintf("carstatus_get_boot_with_media_ipod+ \n");    
	ret = ioctl(file_handle, CARSTATUS_BOOT_WITH_MEDIA_IPOD,  &status);
	if(ret <0)
		return ret;
	*ipod_status = status;
	dprintf("carstatus_get_boot_with_media_ipod  in status =0x%x \n", status);
	return 0;
}

static int carstatus_get_battery_low(struct carstatus_device_t *dev,unsigned char * vol_low)
{
	int ret = 0;
	unsigned char vol_low_t;		
	ret = ioctl(file_handle, CARSTATUS_BATTERY_LOW, &vol_low_t);
	if(ret <0)
		return ret;
	*vol_low = vol_low_t;
	dprintf("carstatus_get_battery_vol  in status =0x%x \n",vol_low_t);
	return 0;
}

static int carstatus_notify_ready(struct carstatus_device_t *dev)
{
	int ret = 0;
	ret = ioctl(file_handle, CARSTATUS_NOTIFY_READY);
	if(ret <0)
		return ret;
	return 0;
}

static int carstatus_setting_spec(struct carstatus_device_t *dev, char * setting_val)
{
	int ret = 0;
	ret = ioctl(file_handle, CARSTATUS_SETTING_SPEC, setting_val);
	if(ret <0)
		return ret;
	return 0;
}


static int carstatus_get_touchpanel_type(struct carstatus_device_t *dev,unsigned int * touchpanel_type)
{
	int ret = 0;
	unsigned int touchpanel_type_t;		
	ret = ioctl(file_handle, CARSTATUS_GET_TOUCHPANEL_TYPE, &touchpanel_type_t);
	if(ret <0)
		return ret;
	*touchpanel_type = touchpanel_type_t;
	dprintf("carstatus_get_calibration_flags  in status =0x%x \n",touchpanel_type_t);
	return 0;
}

static int carstatus_get_ipod_status(struct carstatus_device_t *dev, unsigned int * p_ipod_status)
{
	int ret = 0;
	unsigned int ipod_status;
	ret = ioctl(file_handle, CARSTATUS_GET_IPOD_CONNECT_STATUS, &ipod_status);
	if(ret <0)
		return ret;
	*p_ipod_status = ipod_status;
	dprintf("carstatus_get_ipod_status in status =0x%x \n", ipod_status);
	return 0;
}

static int carstatus_set_usb_debug_mode(struct carstatus_device_t *dev, unsigned int * p_reserve)
{
	int ret = 0;
	unsigned int reserve;
	ret = ioctl(file_handle, CARSTATUS_SET_USB_DEBUG_MODE, &reserve);
	if(ret <0)
		return ret;
	*p_reserve = reserve;
	dprintf("carstatus_set_usb_debug_mode\n");
	return 0;
}


static int carstatus_device_close(struct hw_device_t *device)
{
    dprintf("HAL carstatus_device_close");
	
    struct carstatus_device_t *dev = (struct carstatus_device_t*)device;
    if(dev){
      /* free all resources associated with this device here */
        free(dev);
    }
    return 0;
}

static int carstatus_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
    int status = -EINVAL;

	dprintf("carstatus_device_open %s", name);
	
    struct carstatus_device_t *dev;
    dev = (struct carstatus_device_t*)malloc(sizeof(*dev));

    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = carstatus_device_close;

	dev->carstatus_open = carstatus_open;
	dev->carstatus_poll_car_status = carstatus_poll_car_status;
	dev->carstatus_get_status = carstatus_get_status;
	dev->carstatus_notify_ready = carstatus_notify_ready;
	dev->carstatus_get_battery_vol = carstatus_get_battery_vol;
	dev->carstatus_get_battery_high = carstatus_get_battery_high;
	dev->carstatus_get_battery_low = carstatus_get_battery_low;
	dev->carstatus_get_boot_with_media_card = carstatus_get_boot_with_media_card;
	dev->carstatus_get_boot_with_media_usb = carstatus_get_boot_with_media_usb;
	dev->carstatus_setting_spec = carstatus_setting_spec;
	dev->carstatus_get_touchpanel_type = carstatus_get_touchpanel_type;
	dev->carstatus_get_ipod_status = carstatus_get_ipod_status;    
	dev->carstatus_set_usb_debug_mode = carstatus_set_usb_debug_mode;
	dev->carstatus_get_boot_with_media_ipod = carstatus_get_boot_with_media_ipod;    
	*device = &dev->common;

	status = 0;
    return status;
}
