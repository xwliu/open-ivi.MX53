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

#include <mcu_ioctl.h>

#define FILE_MAX_SIZE		32 * 1024
#define NUM_ONCE		128
//static const char mcu_file[] = "/sdcard/tractor/tractor_debug.bin";
static const char mcu_file[] = "/sdcard/tractor/tractor_mcu.bin";
static char file_buf[FILE_MAX_SIZE];
static int file_len;

static const char mcu_dev[] = "/dev/mcu_i2c";
int mcu_fd;

static char *transfer_ptr;
MCU_BURN_CHECKSUM sum_data;
MCU_BURN_DATA	  data_once;


static char mpeg_dev[] = "/dev/ttymxc2";
static char log_dev[] = "/dev/ttymxc0";

static int log_fd;
static int mpeg_fd;

static void log_write(const char *fmt, ...)
{
	char buf[512];
	va_list ap;

	if (log_fd < 0) return;

	va_start(ap, fmt);
	vsnprintf(buf, 512, fmt, ap);
	buf[511] = 0;
	va_end(ap);
	write(mpeg_fd, buf, strlen(buf));
}


static int open_serial3(void)
{
	
	struct termios termios_new, termios_old;

	log_fd = open(log_dev, O_WRONLY | O_CREAT | O_TRUNC);
	
	tcgetattr(log_fd, &termios_old);

	termios_new = termios_old;
	
	mpeg_fd = open(mpeg_dev, O_RDWR | O_CREAT | O_TRUNC);
	
	tcsetattr(mpeg_fd, TCSAFLUSH, &termios_new);
	
	close(log_fd);
	return 0;
}


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
	int ret = 0;
	fp = fopen( mcu_file, "r" );
	if (NULL == fp) {
		log_write("burning file does not exist or can't be open, \
			   file pwd should be %s\n", mcu_file);
		goto file_error;
	}

	file_len = fread( file_buf, sizeof(char), FILE_MAX_SIZE, fp );
	if (file_len <= 0) {
		log_write("file read may be error, file lenght is not more than 0\n");
		goto file_error;
	} else {
		log_write("file length is 0x%x\n", file_len);
	}
	/* if file_len is not aligned in NUM_ONCE, put 0 followed */	
	num_zero = file_len % NUM_ONCE;
	if ( 0 != num_zero ) {
		num_zero = NUM_ONCE - num_zero;
		for (; num_zero > 0; num_zero--) {
			*(char *)(file_buf + file_len) = 0;
			file_len += 1;
		}
	}
	
	log_write("by adding zero following, file length is 0x%x\n", file_len);
	goto exit;

file_error:
	ret = 1;
exit:
	fclose(fp);
	return ret;
}

static int mcu_data_transfer(void)
{
	int i = 0;
	int len = file_len;
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
			log_write("send data error\n");
			goto error;
		}
		if ( ioctl(mcu_fd, MCU_BURN_READ, &sum_fb) != 0 ) {
			log_write("get sum error");
			goto error;
		}

		/* if checksum isn't equal, error */
		if (sum_data.checksum != sum_fb.checksum) {
			log_write("check sum not equal, sum_imx:0x%x	sum_mcu:%x\n", \
						        sum_data.checksum, sum_fb.checksum);
			goto error;
		}
		log_write("%d frame success, len: 0x%x\n", i, len);
	}
	return 0;
error:
	return 1;
}

int main(void)
{
	int ret = 0;
	/* open debug port */
	ret = open_serial3();
	if (ret != 0) {
		goto log_error;
	}
#if 1	
	/* get data from file, and set it NUM_ONCE aligned */
	if ( get_burning_data() != 0) {
		log_write("get burning data error\n");
		goto file_error;
	} 
#else
	/* for test, set data as 0~128 loop */
	unsigned int i;
	for (i = 0; i < 2 * 1024; i++) {
		file_buf[i] = i;
	}
	file_len = 2 * 1024;
#endif
	/* open dev */
	mcu_fd = open( mcu_dev, O_RDWR | O_CREAT | O_TRUNC );
	if (mcu_fd < 0) {
		log_write("open mcu dev error\n");
		goto mcu_error;
	}
	/* test mcu by reading verison */
	MCU_VERSION mcu_version;
	ioctl(mcu_fd, MCU_GET_VERSION, &mcu_version);
	log_write("mcu verison hardware: %d, software: %d\n", \
			       mcu_version.hardware_ver, mcu_version.software_ver);
	/* continue to transfer until it success */
	/* start transfer */
	
	ret = mcu_data_transfer();
#if 0
	while( ret != 0) {
		log_write("transfer error, and try again\n");
		ioctl(mcu_fd, MCU_BURN_RESTART);
		ret = mcu_data_transfer();
	}
#endif
	log_write("transfer success, stop\n");
mcu_error: 
	close(mcu_fd);
	log_write("close mcu\n");
file_error:
log_error: 
	close(mpeg_fd);
exit:
	return 0;
}
