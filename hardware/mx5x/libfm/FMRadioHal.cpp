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

#define LOG_TAG "FMRadio"

#include <hardware/hardware.h>
#include <hardware/FMRadioHal.h>

#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <linux/input.h>
#include <utils/Log.h>
#include <cutils/atomic.h>
#include <time.h>
#include "RDA5990_FM_drv.h"

#define FMRADIO_DEBUG 0
#if FMRADIO_DEBUG
#define RADIOD(fmt, arg...)  LOGI("Radio HAL : %s " fmt , __func__ , ## arg)
#else
#define RADIOD(fmt, arg...) 
#endif
#define RADIOI(fmt, arg...)  LOGI("Radio HAL : %s " fmt , __func__ , ## arg)
#define RADIOE(fmt, arg...)  LOGE("Radio HAL : %s " fmt , __func__ , ## arg)
#define RADIOW(fmt, arg...)  LOGW("Radio HAL : %s " fmt , __func__ , ## arg)

#define RDA5990FM_PATH_NAME "/dev/RDA5990_FM"

static int file_handle = 0;
static int rds_handle = -1;

static struct fm_tune_parm tune_parm;
static struct fm_scan_parm scan_parm;

static int radio_chip_model = CHIP_TEF6621;


/*****************************************************************************/

static int fm_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);

static struct hw_module_methods_t fm_module_methods = {
    open: fm_device_open
};

struct fm_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
		id: FM_HARDWARE_MODULE_ID,
		name: "fm module",
		author: "Yishan@Tokenwireless",
		methods: &fm_module_methods,
	},
	/* module api interface */
};

static int fm_open(struct fm_device_t *dev)
{	
	int chip_model = CHIP_TEF6621;
	RADIOI("\n");

	file_handle = open("/dev/FMRadio", O_RDWR);
	if(file_handle == -1){
		RADIOI("open fail\n");
		return -1;
	}
	if (ioctl(file_handle, RADIO_IOCTL_GET_CHIP_MODEL, &chip_model) < 0) {
		chip_model = CHIP_TEF6621;
	}
	radio_chip_model = chip_model;
	if (radio_chip_model == CHIP_TEF6621) {
		LOGI("Radio Chip TEF6621 DETECT!");
	} else if (radio_chip_model == CHIP_TEF6624) {
		LOGI("Radio Chip TEF6624 DETECT!");
	}
	RADIOD("file_handle:0x%x\n",file_handle);
	return 0;
}

#if 0

int fm_poll_radio_pointer(struct fm_device_t *dev, radio_pointer_package *radio_pointer_package_ptr)
{
	int ret = 0;
	radio_pointer_package package_t;
	struct pollfd event;
		

	while(1){
		memset(&event, 0x00, sizeof(event));
		event.fd = file_handle;
		event.events = 1;

		RADIOD("radio working\n");

		ret = poll((struct pollfd *)&event, 1, 200);
		if(ret < 0){
			LOGI(" poll fail\r\n");
			return -1;
			break;
		}
		else if(ret == 0){
			RADIOD("poll timeout\n");
			return 1;
		}
		if(event.revents & POLLERR){
			RADIOI("device error\n");
			return -1;
		}
		else if(event.revents & POLLIN){
			ret = ioctl(file_handle, RADIO_IOCTL_GET_POINTER, &package_t);
			if(ret <0){
				RADIOI(" FM_RADIO_GET_RADIO_POINTER error\n");
				return ret;
			}
		   /* BIT0 		valid 
 			* BIT1 		stereo
 			* BIT2 		local
 			* BIT3 		band
 			* BIT4-BIT7 	status
 			* BIT8-BIT31   freq
			* unsigned int freq;
			* BIT0-BIT7 	level
 			* BIT8-BIT15   	usn;
 			* BIT16-BIT23 	wam;
 			* BIT24-BIT31   ifc;
			* unsigned int info;
 			*/
			radio_pointer_package_ptr->freq = package_t.freq;
			radio_pointer_package_ptr->info = package_t.info;
			LOGI("Freq:%d, Level:%d, USN:%d, WAM:%d, IFC:%d, Valid:%d, Stereo:%d, Local:%d, Band:%d, Status:%d\n", 	
				(package_t.freq >> 8),(package_t.info & 0xff),((package_t.info >> 8) & 0xff),
				((package_t.info >> 16) & 0xff), ((package_t.info >> 24) & 0xff), (package_t.freq & 0x01), 
				((package_t.freq >> 1) & 0x01), ((package_t.freq >> 2) & 0x01), ((package_t.freq >> 3) & 0x01),
				((package_t.freq >> 4) & 0x0f)); 					
			return 0;
		}
	}
	return ret;
}
#endif


static int fm_init_work(struct fm_device_t *dev)
{
	RADIOD("\n");
	int ret = 0;
	ret = ioctl(file_handle, RADIO_IOCTL_FM_INIT, NULL);
	return ret;
}

static int am_init_work(struct fm_device_t *dev)
{
	RADIOD("\n");
	int ret = 0;
	ret = ioctl(file_handle, RADIO_IOCTL_AM_INIT, NULL);
	return ret;
}

static int fm_radio_jump_freq(struct fm_device_t *dev, int freq)
{
	int ret = 0;
	int freq_set = freq;
	
	RADIOD("Freq: %d \n",freq);
	ret = ioctl(file_handle, RADIO_IOCTL_JUMP_FREQUENCE, &freq_set);
	return ret;
}

static int fm_radio_seek(struct fm_device_t *dev, bool seek_up_down)
{
	int ret = 0;
	bool up_down = seek_up_down;
	
	RADIOD(": %s\n",(seek_up_down == 1 ? "UP" : "DOWN"));
	if(seek_up_down)
		ret = ioctl(file_handle, RADIO_IOCTL_SEEK_UP, &up_down);
	else
		ret = ioctl(file_handle, RADIO_IOCTL_SEEK_DOWN, &up_down);
	return ret;
}

static int fm_radio_switch_mode(struct fm_device_t *dev, char radio_mode)
{
	int ret = 0;
	bool mode = radio_mode ==1 ? 1 : 0;	
	RADIOD(": %s\n",(radio_mode == 0 ? "AM" : "FM"));
	ret = ioctl(file_handle, RADIO_IOCTL_SWITCH_BAND, &mode);
	return ret;
}

static int fm_radio_as(struct fm_device_t *dev)
{
	int ret = 0;
	RADIOD("\n");
	ret = ioctl(file_handle, RADIO_IOCTL_AUTO_SEARCH, NULL);
	return ret;
}

static int fm_radio_loc(struct fm_device_t *dev, bool loc)
{
	int ret = 0;
	bool loc_remote = loc;
	RADIOD(": %s\n",(loc == 0 ? "Local" : "Remote"));
	ret = ioctl(file_handle, RADIO_IOCTL_SET_LOCAL, &loc_remote);
	return ret;
}

static int fm_radio_stereo(struct fm_device_t *dev, bool stereo)
{
	int ret = 0;
	bool stereo_t = stereo;
	RADIOD(": %s\n",(stereo_t == 1 ? "Stereo" : "Mono"));
	ret = ioctl(file_handle, RADIO_IOCTL_MONO_FORCE, &stereo_t);
	return ret;
}

static int fm_device_close(struct hw_device_t *device)
{
    RADIOD("%s\n",__func__);
	
    struct fm_device_t *dev = (struct fm_device_t*)device;
    if(dev){
      /* free all resources associated with this device here */
        free(dev);
    }
    return 0;
}

static int fm_search_stop(struct fm_device_t *dev)
{
	int ret = 0;
    RADIOD("%s\n",__func__);

	ret = ioctl(file_handle, RADIO_IOCTL_SEARCH_STOP, NULL);
	return ret;
}

static int fm_get_curr_conf(struct fm_device_t *dev, int *conf_array_t)
{
	int ret = 0;
	int i;
	RADIOD("\n");
	int conf_array[8];

	ret = ioctl(file_handle, RADIO_IOCTL_GET_CURR_CONF, conf_array);
	for(i = 0; i < 8; i++){
		conf_array_t[i] = conf_array[i];
	}
						
	return ret;
}

static int fm_set_curr_conf(struct fm_device_t *dev, int *conf_array_t)
{
	int ret = 0;
	int i;
	RADIOD("\n");
	int conf_array[8];

	for(i = 0; i < 8; i++){
		conf_array[i] = conf_array_t[i];
	}
	ret = ioctl(file_handle, RADIO_IOCTL_SET_CURR_CONF, conf_array);
						
	return ret;
}

static int fm_set_rds_enable(struct fm_device_t *dev, bool enable)
{
	int ret = -1;	
	RADIOI(" enable: %d\n", enable);

	if (radio_chip_model == CHIP_TEF6624) {
		if (enable) {
			ret = ioctl(file_handle, RADIO_IOCTL_TEF6624_RDSTH_START, NULL);
			if (ret < 0) {
				RADIOE("RADIO_IOCTL_TEF6624_RDSTH_START failed, reason: %s\n",  strerror(errno));
				return ret;
			}		
		} else {
			ret = ioctl(file_handle, RADIO_IOCTL_TEF6624_RDSTH_STOP, NULL);
			if (ret < 0) {
				RADIOE("RADIO_IOCTL_TEF6624_RDSTH_STOP failed, reason: %s\n",  strerror(errno));
				return ret;
			}		
		}
	} else if (radio_chip_model == CHIP_TEF6621) {
		if (enable) {
			ret = ioctl(rds_handle, FM_IOCTL_RDS_POLL_START, NULL);
			if (ret < 0) {
				RADIOE("FM_IOCTL_RDS_POLL_START failed, reason: %s\n",  strerror(errno));
				return ret;
			}		
		} else {
			ret = ioctl(rds_handle, FM_IOCTL_RDS_POLL_STOP, NULL);
			if (ret < 0) {
				RADIOE("FM_IOCTL_RDS_POLL_STOP failed, reason: %s\n",  strerror(errno));
				return ret;
			}		
		}
	}
	return ret;
}

static int fm_tef6621_rds_enable(struct fm_device_t *dev, bool enable)
{
	int ret = -1;	
	RADIOI(" enable: %d\n", enable);
	bool enable_t = enable;

	ret = ioctl(file_handle, RADIO_IOCTL_RDS_ENABLE, &enable);
	if (ret < 0) {
		RADIOE("RADIO_IOCTL_RDS_ENABLE failed, reason: %s\n",  strerror(errno));
		return ret;
	}
	return ret;
}

static int fm_rda_power_onoff(struct fm_device_t *dev, bool on)
{
	int ret = -1;	
	RADIOI(" on: %d\n", on);

	if (radio_chip_model == CHIP_TEF6624) {
		return 0;
	}
	
	tune_parm.band = FM_BAND_UE;
	tune_parm.space = FM_SPACE_100K;
	tune_parm.freq = 875;

	if (on) {
		if (rds_handle < 0) {
			rds_handle = open(RDA5990FM_PATH_NAME, O_RDWR);
			if(rds_handle < 0){
				RADIOE("open: %s failed, reason: %s\n", RDA5990FM_PATH_NAME, strerror(errno));
				return rds_handle;
			}
			ret = ioctl(rds_handle, FM_IOCTL_POWERUP, &tune_parm);
			if (ret < 0) {
				RADIOE("FM_IOCTL_POWERUP failed, reason: %s\n",  strerror(errno));
				return ret;
			}
			ret = ioctl(rds_handle, FM_IOCTL_RDS_ONOFF, &on);
			if (ret < 0) {
				RADIOE("FM_IOCTL_RDS_ON failed, reason: %s\n",  strerror(errno));
				return ret;
			}
		}

	} else {
		ret = ioctl(rds_handle, FM_IOCTL_RDS_ONOFF, &on);
		if (ret < 0) {
			RADIOE("FM_IOCTL_RDS_OFF failed, reason: %s\n",  strerror(errno));
			return ret;
		}		ret = ioctl(rds_handle, FM_IOCTL_POWERDOWN, &tune_parm);		
		if (ret < 0) {
			RADIOE("FM_IOCTL_POWERDOWN failed, reason: %s\n",  strerror(errno));
			return ret;
		}
		ret = close(rds_handle);		
		if (ret < 0) {
			RADIOE("close: %s failed, reason: %s\n", RDA5990FM_PATH_NAME, strerror(errno));
			return ret;
		}
		rds_handle = -1;	
	}

	return ret;
}

static int fm_set_rds_freq(struct fm_device_t *dev, short freq)
{
	unsigned short freq_t = (unsigned short)freq;
	if (radio_chip_model == CHIP_TEF6624) {
		return 0;
	}
	RADIOI(" frequency: %d\n", freq);
	tune_parm.band = FM_BAND_UE;
	tune_parm.space = FM_SPACE_100K;
	tune_parm.freq = freq;
	int ret = ioctl(rds_handle, FM_IOCTL_TUNE, &tune_parm);	
	if (ret < 0) {
		RADIOE("ret: %d, error: %s\n", ret, strerror(errno));
	}
	return 0;
}

static int fm_poll_rds_group(struct fm_device_t *dev, radio_rds_group *radio_rds_data)
{
		int ret = 0;
		radio_rds_group rds_group_t;
		struct pollfd event;
			
	
		while(1){
			memset(&event, 0x00, sizeof(event));			
			if (radio_chip_model == CHIP_TEF6621) {
				event.fd = rds_handle;
			} else if (radio_chip_model == CHIP_TEF6624) {
				event.fd = file_handle;
			} else {
				LOGE("HAL No RDS chip detect error\n");
				return -1;
			}	
			event.events = 1;
	
			ret = poll((struct pollfd *)&event, 1, 0);
			if(ret < 0){
				LOGE(" poll fail\r\n");
				return -1;
				break;
			}
			else if(ret == 0){
				RADIOD("poll timeout\n");
				return 1;
			}
			if(event.revents & POLLERR){
				RADIOI("device error\n");
				return -1;
			}
			else if(event.revents & POLLIN){

				if (radio_chip_model == CHIP_TEF6621) {
					ret = ioctl(rds_handle, FM_IOCTL_RDS_GROUP_POPUP, &rds_group_t);
				} else if (radio_chip_model == CHIP_TEF6624) {
					ret = ioctl(file_handle, RADIO_IOCTL_TEF6624_RDSGP_POPUP, &rds_group_t);
				} else {
					LOGE("HAL No RDS chip detect error\n");
					return -1;
				}				
				if(ret <0){
					LOGE("HAL FM_IOCTL_RDS_GROUP_POPUP error\n");
					return ret;
				}
/*				LOGE("HAL -> Block A: 0x%04x, B: 0x%04x, C: 0x%04x, D: 0x%04x\n",
				rds_group_t.block_A,
				rds_group_t.block_B,
				rds_group_t.block_C,
				rds_group_t.block_D);
				LOGE("HAL -> Bler A: 0x%04x, B: 0x%04x, C: 0x%04x, D: 0x%04x\n",
				rds_group_t.bler_A,
				rds_group_t.bler_B,
				rds_group_t.bler_C,
				rds_group_t.bler_D);
*/				memcpy(radio_rds_data, &rds_group_t, sizeof(radio_rds_group));
				return 0;
			}
		}
		return ret;
	}


static int fm_get_runtime_state(struct fm_device_t *dev, radio_pointer_info *state)
{
	int ret = 0;
	radio_pointer_info state_t;
	ret = ioctl(file_handle, RADIO_IOCTL_RUNTIME_STATE, &state_t);
	memcpy(state, &state_t, sizeof(radio_pointer_info));
	return ret;
}

static int fm_get_tune_results(struct fm_device_t *dev, tune_result *results)
{
	int ret = 0;
	ret = ioctl(file_handle, RADIO_IOCTL_TUNE_RESULTS, results);
	return ret;
}

static int fm_radio_as_pty(struct fm_device_t *dev, int pty)
{
	int ret = 0;
	RADIOD("\n");
	int pty_t = pty;
	ret = ioctl(file_handle, RADIO_IOCTL_AUTO_SEARCH_PTY, &pty_t);
	return ret;
}

static int fm_radio_seek_pty(struct fm_device_t *dev, bool up, int pty)
{
	int ret = 0;
	bool up_t = up;
	int pty_t = pty;
	
	RADIOD(": %s\n",(up_t == 1 ? "UP" : "DOWN"));
	if(up_t)
		ret = ioctl(file_handle, RADIO_IOCTL_SEEK_UP_PTY, &pty);
	else
		ret = ioctl(file_handle, RADIO_IOCTL_SEEK_DOWN_PTY, &pty);
	return ret;
}

static int fm_radio_seek_tp(struct fm_device_t *dev, bool up)
{
	bool up_t = up;
	int ret = -1;
	if(up_t)
		ret = ioctl(file_handle, RADIO_IOCTL_SEEK_UP_TP, NULL);
	else
		ret = ioctl(file_handle, RADIO_IOCTL_SEEK_DOWN_TP, NULL);
	return ret;
}

static int fm_radio_get_level(struct fm_device_t *dev)
{
	int level = 0;
	int ret = ioctl(file_handle, RADIO_IOCTL_GET_LEVEL, &level);
	if (ret < 0) {
		return ret;
	}
	return level;
}

static int fm_radio_get_frequency(struct fm_device_t *dev)
{
	int frequency = 0;
	int ret = ioctl(file_handle, RADIO_IOCTL_GET_FREQUENCY, &frequency);
	if (ret < 0) {
		return ret;
	}
	return frequency;
}

static int fm_get_tuning_results(struct fm_device_t *dev, tune_result *results)
{
	int ret = 0;
	ret = ioctl(file_handle, RADIO_IOCTL_TUNING_RESULTS, results);
	return ret;
}


/*****************************************************************************/

static int fm_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
    int status = -EINVAL;

	RADIOD("%s %s", __func__, name);
	
    struct fm_device_t *dev;
    dev = (struct fm_device_t*)malloc(sizeof(*dev));

    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = fm_device_close;

	dev->fm_open = fm_open;
	dev->fm_init_fm = fm_init_work;
	dev->fm_init_am = am_init_work;
	dev->fm_radio_jump_freq = fm_radio_jump_freq;
	dev->fm_radio_seek = fm_radio_seek;
	dev->fm_radio_switch_mode = fm_radio_switch_mode;
	dev->fm_radio_as = fm_radio_as;
	dev->fm_radio_loc = fm_radio_loc;
	dev->fm_radio_stereo = fm_radio_stereo;
	dev->fm_search_stop = fm_search_stop;
	dev->fm_get_curr_conf = fm_get_curr_conf;
	dev->fm_set_curr_conf = fm_set_curr_conf;
	dev->fm_set_rds_enable = fm_set_rds_enable;
	dev->fm_set_rds_freq = fm_set_rds_freq;
	dev->fm_rda_power_onoff = fm_rda_power_onoff;	
	dev->fm_poll_rds_group = fm_poll_rds_group;
	dev->fm_get_runtime_state = fm_get_runtime_state;
	dev->fm_get_tune_results = fm_get_tune_results;
	dev->fm_radio_as_pty = fm_radio_as_pty;
	dev->fm_radio_seek_pty = fm_radio_seek_pty;
	dev->fm_tef6621_rds_enable = fm_tef6621_rds_enable;
	dev->fm_radio_seek_tp = fm_radio_seek_tp;
	dev->fm_radio_get_level = fm_radio_get_level;
	dev->fm_radio_get_frequency = fm_radio_get_frequency;
	dev->fm_get_tuning_results = fm_get_tuning_results;
	
    *device = &dev->common;

	status = 0;
    return status;
}
