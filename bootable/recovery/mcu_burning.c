/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <termios.h>
#include <unistd.h>

#include <linux/fb.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <cutils/properties.h>

#include <common.h>
#include <install.h>
#include <mcu_ioctl.h>

#define FILE_MAX_SIZE		32 * 1024
#define NUM_ONCE		128
//static const char mcu_file[] = "/sdcard/tractor/tractor_debug.bin";
static const char mcu_file[] = "/sdcard/tractor/tractor_mcu.bin";
static char file_buf[FILE_MAX_SIZE];
static int file_len;

const char mcu_dev[] = "/dev/mcu_i2c";
int mcu_fd;

static char *transfer_ptr;
MCU_BURN_CHECKSUM sum_data;
MCU_BURN_DATA	  data_once;

static int get_checksum(char *buf_pt, uint32_t len)
{
	uint32_t i = 0;
	uint32_t sum_frame = 0;
	for( i = 0; i < len; i++ ) {
		sum_frame += *(buf_pt + i);
	}
	return sum_frame;
}

static int get_burning_data(void)
{	
	FILE *fp;
	int num_zero = 0;
	int ret = INSTALL_SUCCESS;
	fp = fopen_path( mcu_file, "r" );
	if (NULL == fp) {
		LOGE("burning file does not exist or can't be open, \
		file pwd should be %s\n", mcu_file);
		goto data_error;
	}

	file_len = fread( file_buf, sizeof(char), FILE_MAX_SIZE, fp );
	if (file_len <= 0) {
		LOGE("file read may be error, file lenght is not more than 0\n");
		goto data_error;
	} else {
		printf("file length is 0x%x\n", file_len);
	}
	check_and_fclose(fp, mcu_file);
	/* if file_len is not aligned in NUM_ONCE, put 0 followed */	
	num_zero = file_len % NUM_ONCE;
	if ( 0 != num_zero ) {
		num_zero = NUM_ONCE - num_zero;
		for (; num_zero > 0; num_zero--) {
			*(char *)(file_buf + file_len) = 0;
			file_len += 1;
		}
	}
	
	printf("by adding zero following, file length is 0x%x\n", file_len);
	
	goto data_exit;
data_error:
	ret = INSTALL_ERROR;
data_exit:
	return ret;
}

static int mcu_data_transfer(void)
{
	int i = 0;
	int len = file_len;
	
	int transfer_times = (len + NUM_ONCE - 1) / NUM_ONCE;
	int dsp_10p = transfer_times / 10;
	int dsp_2p = transfer_times / 50;
	
	int icon_ind = 0;
	int out_times = 0;
	MCU_BURN_CHECKSUM sum_fb;	/* sum from mcu feedback */
	for ( ;len > 0; len -= NUM_ONCE, i++ ) {
		/* add NUM_ONCE to address after transfer once */
		transfer_ptr = file_buf + i * NUM_ONCE;
		/* copy to data_once */
		memcpy(data_once.bytes, transfer_ptr, NUM_ONCE);
		/* get check sum */
		sum_data.checksum = get_checksum(data_once.bytes, NUM_ONCE);

		/* send 128 bytes, and then get checksum from mcu */
		if ( ioctl(mcu_fd, MCU_BURN_WRITE, &data_once) != 0 ) {
			LOGE("send data error\n");
			goto burning_error;
		}
		if ( ioctl(mcu_fd, MCU_BURN_READ, &sum_fb) != 0 ) {
			LOGE("get sum error");
			goto burning_error;
		}

		/* if checksum isn't equal, error */
		if (sum_data.checksum != sum_fb.checksum) {
			LOGE("check sum not equal, sum_imx:0x%x	sum_mcu:%x\n", \
						        sum_data.checksum, sum_fb.checksum);
			goto burning_error;
		}
		printf("%d frame success, len: 0x%x\n", i, len);
		out_times++;
		if(((out_times % dsp_2p) == 0) && ((out_times / dsp_2p) < 50)) {
			ui_set_burning_status_detail();
		}
		if(((out_times % dsp_10p) == 0) && ((out_times / dsp_10p) < 10)) {
			icon_ind++;
			ui_set_burning_status();
		}

	}
	ui_set_burning_status_detail();
	ui_set_burning_status();
	return INSTALL_SUCCESS;
burning_error:
	return INSTALL_ERROR;
}

int mcu_reset_cpu(void)
{
	printf("mcu start to reboot cpu\n");
	int ret = INSTALL_SUCCESS;
	mcu_fd = open( mcu_dev, O_RDWR | O_CREAT | O_TRUNC );
	if (mcu_fd < 0) {
		LOGE("open mcu dev error\n");
		ret = INSTALL_ERROR;
		sleep(10);
		goto reset_error;
	}

	ioctl(mcu_fd, MCU_RESET);
//	close(mcu_fd);
reset_error:
	return ret;
}

int mcu_update(void)
{
	int ret = INSTALL_SUCCESS;
	printf("-->start\n");
	printf("-->Try to update mcu..\n");
	/* get data from file, and set it NUM_ONCE aligned */
	ui_set_burning_type(BURNING_TYPE_MCU);
	ret = get_burning_data();
	if ( ret != INSTALL_SUCCESS) {
		return INSTALL_NOT_EXISTED;
	} 
	ui_set_burning_status();
	ui_set_burning_status_detail();
#if 1
	/* open dev */
	mcu_fd = open( mcu_dev, O_RDWR | O_CREAT | O_TRUNC );
	if (mcu_fd < 0) {
		LOGE("open mcu dev error\n");
		ret = INSTALL_ERROR;
		goto mcu_error;
	}
	/* test mcu by reading verison */
	MCU_VERSION mcu_version;
	ioctl(mcu_fd, MCU_GET_VERSION, &mcu_version);
	printf("mcu verison hardware: %d, software: %d\n", \
			       mcu_version.hardware_ver, mcu_version.software_ver);
	/* continue to transfer until it success */
	/* start transfer */
	
	ret = mcu_data_transfer();
	
	while(ret != INSTALL_SUCCESS) {
		LOGE("transfer error, and try again\n");
		ioctl(mcu_fd, MCU_BURN_RESTART);
		ret = mcu_data_transfer();
	}
	close(mcu_fd);
mcu_error: 
#endif
file_error:
mcu_exit:
	if (ret == INSTALL_SUCCESS) {
		printf("-->Transfer mcu burning image success\n");
	} else {
		printf("-->Fail to update mcu\n");
	}
	return ret;
}
