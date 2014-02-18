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
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <cutils/properties.h>

#include <mcu_ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <mpeg_burning.h>
#include <sd_ctrl_def.h>

#include <common.h>
#include <install.h>



static char mpeg_dev[] = "/dev/ttymxc2";
static char log_dev[] = "/dev/ttymxc0";
static int mpeg_fd;

static uint32_t read_buf[2];

/*aligned in 2-bytes*/
static uint16_t transfer_buf[4102];
static unsigned char clear_buf[256];

static char boot_bin[]  = "/sdcard/tractor/boot_flash.bin";
static char rom_bin[]   = "/sdcard/tractor/rom.bin";
static char sload_bin[] = "/sdcard/tractor/sloader.bin";

static int init_serial3(void)
{
	
	struct termios termios_new;
	
	mpeg_fd = open(mpeg_dev,O_RDWR | O_NOCTTY); // O_RDWR | O_CREAT | O_TRUNC);
	tcflush(mpeg_fd, TCIOFLUSH);
	tcgetattr(mpeg_fd, &termios_new);		/* get config */
	bzero(&termios_new, sizeof(termios_new));	/* clear old config */
	
	termios_new.c_cflag |= CREAD | CLOCAL;  	/* open read and link local */
	termios_new.c_cflag &= ~CSIZE;			
	termios_new.c_cflag |= CS8;			/* 8-bit data */
	termios_new.c_cflag &= ~PARENB;			/* no parity bit */
	cfsetispeed(&termios_new, B230400);	
	cfsetospeed(&termios_new, B230400);		/* set baudrate of IN/OUT */
//	tcsetattr(mpeg_fd, TCSANOW, &termios_new);
	termios_new.c_cflag &=  ~CSTOPB;		/* 1-bit stop */
	termios_new.c_cflag &= ~CRTSCTS;
	termios_new.c_cc[VTIME]  = 0;			/* wait forever while 0 */
	termios_new.c_cc[VMIN] = 0;			/* wait for more than 1 byte */
	tcflush(mpeg_fd, TCIFLUSH);
	if((tcsetattr(mpeg_fd, TCSANOW, &termios_new)) != 0)
	{
		LOGE("serial set error\n");
		return 1;
	}
//	LOGE("open and set success!\n");
	return 0;
}

static int init_serial3_min(void)
{
	
	struct termios termios_new;
	
	mpeg_fd = open(mpeg_dev,O_RDWR | O_NOCTTY); // O_RDWR | O_CREAT | O_TRUNC);
	tcflush(mpeg_fd, TCIOFLUSH);
	tcgetattr(mpeg_fd, &termios_new);		/* get config */
	bzero(&termios_new, sizeof(termios_new));	/* clear old config */
	
	termios_new.c_cflag |= CREAD | CLOCAL;  	/* open read and link local */
	termios_new.c_cflag &= ~CSIZE;			
	termios_new.c_cflag |= CS8;			/* 8-bit data */
	termios_new.c_cflag &= ~PARENB;			/* no parity bit */
	cfsetispeed(&termios_new, B115200);	
	cfsetospeed(&termios_new, B115200);		/* set baudrate of IN/OUT */
//	tcsetattr(mpeg_fd, TCSANOW, &termios_new);
	termios_new.c_cflag &=  ~CSTOPB;		/* 1-bit stop */
	termios_new.c_cflag &= ~CRTSCTS;
	termios_new.c_cc[VTIME]  = 0;			/* wait forever while 0 */
	termios_new.c_cc[VMIN] = 1;			/* wait for more than 1 byte */
	tcflush(mpeg_fd, TCIFLUSH);
	if((tcsetattr(mpeg_fd, TCSANOW, &termios_new)) != 0)
	{
		LOGE("serial set error\n");
		return 1;
	}
//	printf("open and set success!\n");
	return 0;
}


/* input file name, and return file size */
unsigned int mpeg_get_file_size(char *file_name)
{
	FILE * File;
	unsigned int size;
	File = fopen(&file_name[0], "rb");
	if(File == NULL) {
		LOGE("Error while open file: %s\n", file_name);
		return 0;
	}
	/* point to the end of file */
	fseek(File, 0, SEEK_END);
	/* get the offset of the point and the start of file */
	size = ftell(File);
	fclose(File);
	return size;
}

void mpeg_power_on(void)
{
	int mcu_fd;
	int ret = 0;
	mcu_fd = open( "/dev/mcu_i2c", O_RDWR | O_CREAT | O_TRUNC );
	int value = 0;	// keep reset as low before power on
	ret = ioctl(mcu_fd, MPEG_RESET, &value);	
	//DEBUG_LOGI("mpeg_reset_disable\r\n");	
	//value = 0;	// insure mpeg power off
	//ret = ioctl(mcu_fd, MPEG_POWER, &value);
	//DEBUG_LOGI("mpeg_power_disable\r\n");	
	usleep(30000);
	//value = 1;	// power on mpeg
	//ret = ioctl(mcu_fd, MPEG_POWER, &value);
	//DEBUG_LOGI("mpeg_reset_enable\r\n");	
	//usleep(20000);
	value = 1;	// enable mpeg
	ret = ioctl(mcu_fd, MPEG_RESET, &value);	
	//DEBUG_LOGI("mpeg_power_enable\r\n");	
	close(mcu_fd);

}
static int get_a_reply(void)
{
	char out_buf[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	char in_buf[1024] =  {0};
	uint32_t len = 0;
	unsigned int ret, i;
	
	unsigned int try_time = 10;
	while (try_time--) {
		/* send a out */
		/* reset mpeg to get a return */
		mpeg_power_on();
		init_serial3();
		for ( i = 0; i < 50; i++ ) {
			/* read from mpeg, while a return, clear fifo */
			ret = write(mpeg_fd, out_buf, 5);
			ret = read(mpeg_fd, in_buf, 1);
			usleep(100);
			if (ret) {
				if (in_buf[0] == A_CMD_L) {
//					printf("--------------------Get Response a From Device,%x\n", in_buf[0]);
					ret = read(mpeg_fd, in_buf, 1024);
					return INSTALL_SUCCESS;
				}
			}
		}
	}
	LOGE("++++++++++++++++++++Do not get Response a From Device,%x\n", in_buf[0]);
	return INSTALL_ERROR;
}

uint32_t  mpeg_wait_rebootX(void)
{
	int ret = 0, i;
	unsigned char cmd_fb;
//	unsigned char cmd_s = 'D';
//	write(mpeg_fd, &cmd_s, 1);
	for ( i = 0; i < 1000 ; i++ ) {
		/* read from mpeg, while REBOOT_X_CMD return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
		//printf("--------------------RebootB ret : %c\n",ret);
		if (ret) {
			printf("--------------------rebootB %x\n", cmd_fb);
			if (cmd_fb == REBOOT_B) {
				return INSTALL_SUCCESS;
			}
		}
		usleep(1000);
	}
	LOGE("++++++++++++++++++++Do not get Response REBOOT_X_CMD From Device\n");
	return INSTALL_ERROR;
}

int mpeg_read_sequence_command(void)
{
	unsigned char cmd;
	unsigned char cmd_fb;
	unsigned int value;
	int ret, i;
	cmd = READ_CMD_L;
	ret = write(mpeg_fd, &cmd, 1);
	for ( i = 0; i < 100 ; i++ ) {
		/* read from mpeg, while READ_CMD_L return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
//		printf("--------------------read_cmd ret : %d\n",ret);
		usleep(1000);
		if (cmd_fb == READ_CMD_L) {
//			printf("--------------------Get Response READ_CMD_L From Device,%x\n", cmd_fb);
			ret = read(mpeg_fd, read_buf, 8);
			return read_buf[0];
		}
	}
	printf("++++++++++++++++++++Do not get Response READ_CMD_L From Device\n");
	return 0;
}


uint32_t mpeg_read_command(unsigned int addr)
{
	int ret = 0,i;
	unsigned char cmd[5];
	unsigned char cmd_fb;
//	printf("--------------------read addr is 0x%08x\n", addr);

	cmd[0] = READ_CMD;
	cmd[1] = addr & 0xff;
	cmd[2] = (addr >> 8) & 0xff;
	cmd[3] = (addr >> 16) & 0xff;
	cmd[4] = (addr >> 24) & 0xff;
	ret = write(mpeg_fd, cmd, 5);
	
	for ( i = 0; i < 100 ; i++ ) {
		/* read from mpeg, while READ_CMD return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
//		printf("--------------------read_cmd ret : %d\n",ret);
		usleep(1000);
		if (ret) {
//			printf("--------------------read_cmd cmd_fb : %x\n",cmd_fb);
			if (cmd_fb == READ_CMD) {
//				printf("--------------------Get Response READ_CMD From Device,%x\n", cmd_fb);
				ret = read(mpeg_fd, read_buf, 8);
				return read_buf[0];
			}
		}
	}
	LOGE("++++++++++++++++++++Do not get Response READ_CMD From Device\n");
	return 0;
}

int mpeg_write_sequence_command(unsigned int value)
{

	unsigned char cmd[9];
	unsigned char cmd_fb;
	int ret, i;
	cmd[0] = WRITE_CMD_L;

	cmd[1] = value & 0xff;
	cmd[2] = (value >> 8) & 0xff;
	cmd[3] = (value >> 16) & 0xff;
	cmd[4] = (value >> 24) & 0xff;

	ret = write(mpeg_fd, cmd, 5);
	for ( i = 0; i < 100; i++ ) {
		/* read from mpeg, while WRITE_CMD_L return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
//		printf("--------------------write_cmd_l ret : %d\n",ret);
		usleep(1000);
		if (ret) {
			if (cmd_fb == WRITE_CMD_L) {
//				printf("--------------------Get Response WRITE_CMD_L From Device,%x\n", cmd_fb);
				return 1;
			}
		}
	}
	LOGE("++++++++++++++++++++Do not get Response WRITE_CMD_L From Device\n");

	return 0;
}

int mpeg_write_single_command(unsigned char cmd) 
{ 
	unsigned char cmd_sig = cmd; 
	unsigned char cmd_fb;
	int ret, i;

	ret = write(mpeg_fd, &cmd_sig, 1);
	for ( i = 0; i < 100; i++ ) {
		/* read from mpeg, while single cmd return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
//		printf("--------------------single cmd ret : %d\n",ret);
		usleep(1000);
		if (ret) {
			if (cmd_fb == cmd) {
//				printf("--------------------Get Response single cmd From Device,%x\n", cmd_fb);
				return 1;
			}
		}
	}
	LOGE("++++++++++++++++++++Do not get Response single cmd From Device\n");

	return 0; 
}



int mpeg_write_command(unsigned int addr,unsigned int value)
{

	unsigned char cmd[9];
	unsigned char cmd_fb;
	int ret, i;
	cmd[0] = WRITE_CMD;
	cmd[1] = addr & 0xff;
	cmd[2] = (addr >> 8) & 0xff;
	cmd[3] = (addr >> 16) & 0xff;
	cmd[4] = (addr >> 24) & 0xff;
	cmd[5] = value & 0xff;
	cmd[6] = (value >> 8) & 0xff;
	cmd[7] = (value >> 16) & 0xff;
	cmd[8] = (value >> 24) & 0xff;

	ret = write(mpeg_fd, cmd, 9);
	
	for ( i = 0; i < 100; i++ ) {
		/* read from mpeg, while WRITE_CMD return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
//		printf("--------------------write_cmd ret : %d\n",ret);
		usleep(1000);
		if (ret) {
			if (cmd_fb == WRITE_CMD) {
//				printf("--------------------Get Response WRITE_CMD From Device,%x\n", cmd_fb);
				return INSTALL_SUCCESS;
			}
		}
	}
	LOGE("++++++++++++++++++++Do not get Response WRITE_CMD From Device\n");

	return INSTALL_ERROR;
}

int wait_BIGA_reply_from_device(void)
{
	printf("Wait for device power on/reset!\n");
	int ret = 0, i;
	unsigned char cmd;
	unsigned char cmd_fb;
	unsigned int try_time = 5;
	cmd = A_CMD;
	ret = write(mpeg_fd, &cmd, 1);
	while (try_time--) {
		usleep(2000 * 10);
		ret = read(mpeg_fd, &cmd_fb, 1);
		if (ret) {
			printf("BIGA cmd_fb : %c\n",cmd_fb);
			if (cmd_fb == A_CMD) {
//				printf("--------------------Get Response A From Device, A_CMD, %x\n", cmd_fb);
				ret = read(mpeg_fd, clear_buf, 256);
				return INSTALL_SUCCESS;
			} 
		}
		ret = write(mpeg_fd, &cmd, 1);
	}
	return INSTALL_ERROR; 
}


uint32_t wait_LACK_from_device(void)
{
	printf("Wait LACK from device!\n");
	int ret = 0, i;
	unsigned char cmd_fb;

	for ( i = 0; i < 100; i++ ) {
		/* read from mpeg, while LACK_CMD return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
//		printf("--------------------lack_cmd ret : %d\n",ret);
		if (ret) {
			printf("LACK cmd_fb : %c\n",cmd_fb);
			if (cmd_fb == LACK_CMD) {
//				printf("--------------------Get Response LACK_CMD From Device,%x\n", cmd_fb);
				return 1;
			}
		}
		usleep(1000);
	}
	LOGE("++++++++++++++++++++Do not get Response LACK_CMD From Device\n");

	return 0;
}

void send_start_wait_ready()
{
	int ret = 0, i;
	unsigned char cmd_out;
	unsigned char cmd_in;
	cmd_out = 'S';
//	do {
		ret  = write(mpeg_fd, &cmd_out, 1);
//	} while(ret != 1);
	while(1) {
		ret = read(mpeg_fd, &cmd_in, 1);
		if(ret == 1) {
			if(cmd_in == 'R') {
				printf("Mpeg is ready\n\r");
				break;
			}
		}
	}
}

uint32_t wait_EACK_from_device(void)
{
	printf("Wait EACK from device!\n");
	int ret = 0, i;
	unsigned char cmd_fb;

	for ( i = 0; i < 10000; i++ ) {
		/* read from mpeg, while EACK_CMD return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
//		printf("--------------------lack_cmd ret : %d\n",ret);
		if (ret) {
//			printf("BIGA cmd_fb : %c\n",cmd_fb);
			if (cmd_fb == EACK_CMD) {
//				printf("--------------------Get Response EACK_CMD From Device,%x\n", cmd_fb);
				return 1;
			}
		}
		usleep(1000);
	}
	LOGE("++++++++++++++++++++Do not get Response EACK_CMD From Device\n");

	return 0;
}



static int get_device_id(void)
{
	unsigned int id = 0;
	int ret;
	id = mpeg_read_command(RF_STAMP);
	
//	printf("id: 0x%08x \n", id);
	if ( 0xD0 == (unsigned char)id) {
		ret = INSTALL_SUCCESS;
//		printf("read id success\n");
	} else {
		ret = INSTALL_ERROR;
//		printf("read id fail\n");
	}
	return ret;
}

static int confirm_device(void)
{
	int ret;
	ret = get_a_reply();
	if (ret != INSTALL_SUCCESS) {
		return ret;
	}
	ret = get_device_id();
	return ret;
}

/* 4090-bytes aligned, not need to think about 1-byte remained */
static uint32_t get_checksum(uint16_t *ptr, uint32_t size)
{
	uint32_t checksum = 0;
	uint16_t *buf_ptr = ptr;
	while (size) {
		checksum += *buf_ptr++;
		size -= 2;
	}
	return checksum;
}

void get_frame(unsigned char *transfer_ptr, unsigned char *buf_ptr, unsigned short frameid)
{
	unsigned int checksum_frame = 0;
	*(transfer_ptr + 0) = frameid & 0xff;
	*(transfer_ptr + 1) = frameid >> 8;
	
	memcpy(transfer_ptr + 2, buf_ptr + frameid * 4096, 4096);
	checksum_frame = get_checksum( (uint16_t *)(transfer_ptr +2), 4096);
	*(transfer_ptr + 4098) = (checksum_frame >> 0) & 0xff;
	*(transfer_ptr + 4099) = (checksum_frame >> 8) & 0xff;
	*(transfer_ptr + 4100) = (checksum_frame >> 16) & 0xff;
	*(transfer_ptr + 4101) = (checksum_frame >> 24) & 0xff;
//	printf("+++++++++  %x %x %x %x \n", *(transfer_ptr + 4098), *(transfer_ptr + 4099), *(transfer_ptr + 4100), *(transfer_ptr + 4101));
}

//#define MIN((a),(b))  ((a) < (b) ? (a) : (b))
#define MIN(a,b)  (a < b ? a : b)

void mpeg_send_frame(unsigned char *transfer_ptr, unsigned int frame_size)
{
	unsigned int frame_send_end = 0;
	unsigned int frame_send_remain = 4102;
	unsigned int frame_send_out = 0;
	
	while (frame_send_remain) {
		usleep(2000);
		frame_send_end = write(mpeg_fd, (transfer_ptr + frame_send_out), 
				       MIN(frame_size, frame_send_remain));
		frame_send_remain -= frame_send_end;
		frame_send_out += frame_send_end;
	}
}

unsigned char mpeg_wait_return(void)
{
	int ret = 0, i;
	unsigned char cmd_fb;
	for ( i = 0; i < 10000; i++ ) {
		/* read from mpeg, while feedback return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
//		printf("--------------------mpeg wait ret : %d\n",ret);
//		if (ret) {
			printf("mpeg wait cmd_fb :%c\n",cmd_fb);
			//printf("mpeg wait cmd_fb : %c  %x\n",cmd_fb, cmd_fb);
#if 1
			switch (cmd_fb) {
			case 'O':
			case 'X':
			case 'E':
			case 'L':
				return cmd_fb;
			default:
				break;
			}
			
#endif
//		}
//		usleep(1000);
	}
	LOGE("++++++++++++++++++++Do not get Response mpeg wait From Device\n");
	return 0;
}

void clear_rx_buff(void)
{
	tcflush(mpeg_fd, TCIFLUSH);
}
/* frame_size maybe not equal to frame size need to be send once */
static int transfer(uint32_t frame_size)
{
	printf("start to transfer\n");
//	tcflush(mpeg_fd, TCIFLUSH);
	
	unsigned int romsize = mpeg_get_file_size(rom_bin);
	romsize = ((romsize + 4095) / 4096) * 4096;
	int dsp_10p = (romsize / 4096) / 5;
	int dsp_2p =  (romsize / 4096) / 50; 
	int out_times = 0;
	int i = 0;
//	printf("romsize 0x%08x\n",romsize);
	unsigned char *ptr = malloc( sizeof(char) * romsize );
	if (ptr == NULL) {
		LOGE("malloc failure\n");
		return 0;
	}
	FILE *fp = fopen(rom_bin, "rb");
	if (fp == NULL) {
		LOGE("read rom data error\n");
		return 0;
	}
	fread(ptr, sizeof(char), romsize,  fp);
	fclose(fp);
	
	unsigned short frameid = 0;
	unsigned char *buf_ptr = ptr;
	unsigned char *transfer_ptr = (unsigned char *)transfer_buf;
	unsigned char mpeg_fb;
	unsigned int  next_frame = 1;
	
	/* get buffer to transfer, 4096 + frameid + checksum */
	
	get_frame(transfer_ptr, buf_ptr, frameid);
	romsize -= 4096 ;
//	clear_rx_buff();
	ui_set_burning_status();
	while (1) {
		printf("romsize %x		transfer frame %d	", romsize, frameid);
		clear_rx_buff();

		mpeg_send_frame(transfer_ptr, frame_size);

		mpeg_fb = mpeg_wait_return();
//		printf("%x",mpeg_fb);
		switch (mpeg_fb) {
			case 'O':
				if (romsize == 0) {
					//wait_EACK_from_device();
					goto transfer_success;
				}
				next_frame = 1;
				break;
			case 'L':
			case 'X':
				next_frame = 0;
				break;
			case 'E':
				goto transfer_success;
			default:
				printf("update mpeg err\n");
				return 0;
		}
		if (1 == next_frame) {
			frameid += 1;
			get_frame(transfer_ptr, buf_ptr, frameid);
			romsize -= 4096;
			out_times++;
			if(((out_times % dsp_2p) == 0) && ((out_times / dsp_2p) < 50)) {
				//printf("%d	%d\n\r", out_times, dsp_2p);
				ui_set_burning_status_detail();
			}
			if(((out_times % dsp_10p) == 0) && ((out_times / dsp_10p) < 5)) {
				ui_set_burning_status();
			}
		}
	}
	ui_set_burning_status_detail();
	ui_set_burning_status();
	return 0;
transfer_success:
	free(ptr);
	return 1;
}

uint32_t init_sdram(void)
{

	unsigned int memaddr,data;
	unsigned int edi_bef = 0;

	unsigned int count =0;
	int ret = 0;
	unsigned int times = 0;
//	printf("Begin to configure SDRAM...\n");
//	printf("Warning:Current value define for winbond W9812G6JH \n");
//	printf("Warning:if you changed sdram please reconfig parameters \n");
	mpeg_write_command(RF_SDC_REQ_T_RESET,0);
	
	usleep(50000);
	
	mpeg_write_command(RF_HW_CFG_CHG, 0x581f);
	mpeg_write_command(RF_PAD_CTRL, 0x5556);
	mpeg_write_command(RF_SDRAM_CLKO_CFG, 0x003a);
	mpeg_write_command(RF_SDRAM_CLKI_CFG, 0x003a);

	mpeg_write_command(RF_SDCTRL_CFG0, SDCTRL_CFG0_VAL_0);//0x013d);
	mpeg_write_command(RF_SDCTRL_CFG1, SDCTRL_CFG1_VAL_0);//0x19a7);
	mpeg_write_command(RF_SDCTRL_CFG2, SDCTRL_CFG2_VAL_0);//0x0033);
	
	mpeg_write_command(RF_SDCTRL_SREF_CFG, SDCTRL_SREF_VAL);//0x0001);
	mpeg_write_command(RF_SDCTRL_AREF1_CFG, SDCTRL_AREF1_VAL);//0x34c3);

	mpeg_write_command(RF_SDCTRL_CFG3,SDCTRL_CFG3_VAL_0);//0x0541);

	mpeg_write_command(RF_SDCTRL_MRS, 0x0001);
	usleep(50000);

	mpeg_write_command(RF_SDCTRL_CFG4,SDCTRL_CFG4_VAL); //0x1add

	mpeg_write_command(0x00000000, 0x0000);

	for(count = 0; count < 40; count++)
	{
		mpeg_write_command(0x00001 << count, 0x1234567);
		mpeg_write_command(RF_LBC_CONTROL, 0x03);

		//mpeg_write_command(RF_REC_END,0x1A15)
		do{
			data = mpeg_read_command(RF_LBC_CONTROL);
		} while(data == 0);

		//printf("RF_LBC_CONTROL [1FFE8130]%08x\n",data);

		data = mpeg_read_command( 0x0 );
//		printf("[%d] %08x\n",count,data);
		//sleep(10);
		usleep(10 * 1000);

		if(data == 0x1234567)
		{
			times++;
			if(times > 13){
				ret = INSTALL_SUCCESS;
				//mpeg_write_command(RF_SDCTRL_CFG4,0x00000a68);	//0x00000a68);//0x00001a8c
				printf("found magic number @0x%08x\n",0x00000200<<count);
//				printf("init sdram done\n");
				return ret;
//				break;
			}
		}
	}

	printf("Init SDRAM failed!\n");
	return INSTALL_ERROR;
}

uint32_t send_sloader(void)
{	
	unsigned int romsize, romsize1;
	printf("send sloader start..\n");
	int check_fb;
	uint32_t len = 0;
	int ret;
#define LOADER_BIN	boot_bin
	len = mpeg_get_file_size(LOADER_BIN);
//	printf("sloader true len : 0x%08x\n",len);
	/* len be aligned to 4-byte. so malloc size will be enough to read in data */
	if (len & 3) {
		len = len | (~3);
		len = len + 4;
	}
//	printf("sloader len after aligned 4-bytes:  0x%08x\n",len);
	int *ptr = malloc(sizeof(char) * len);
	if (ptr == NULL) {
		goto mpeg_malloc_error;
	}
	FILE *fp = fopen(LOADER_BIN, "rb");
	if (fp == NULL) {
		goto mpeg_open_error;
	}
	fread(ptr, sizeof(int), len,  fp);
	fclose(fp);
	
	int *buf_ptr = ptr; 
#define SLOADER_JUMP_POINT 0x00019000
	mpeg_write_single_command('C');
	mpeg_write_command(0x00018ffc, 0x0042EC44);
	mpeg_write_command(0x00018ff8, 0x0042EC48);
	mpeg_write_command(0x00018ff4, 0x0042EC58);
	mpeg_write_command(0x00018ff0, 0x0042EC4C);
	mpeg_write_command(0x00018fec, 0x0042EC50);
	mpeg_write_command(0x00018fe8, 230400);
	
	unsigned int addr = SLOADER_JUMP_POINT;
	romsize = len >> 2;
	int half_size = romsize / 2;
	while (romsize-- ) {
write_flow:
		mpeg_write_command(addr, *buf_ptr);
		check_fb = mpeg_read_command(addr);
		if (check_fb != *buf_ptr) {
			printf("++++++++++++++++++++verify SLOARD failed@%x\n", (len >> 2) - romsize);
			goto write_flow;
//			ret = INSTALL_ERROR;			
		}
		if(half_size == romsize) {
			ui_set_burning_status();
		}
//		printf("verify SLOARD check_fb:0x%08x, buf:0x%08x, addr:0x%08x, buf_ptr:0x%08x\n",
//					check_fb, *buf_ptr, addr, buf_ptr);
		addr += 4;
		buf_ptr++;
	}
	
	ui_set_burning_status();
	ret = INSTALL_SUCCESS;
	printf("verify success\n");
#if 0
	mpeg_write_command(SLOADER_JUMP_POINT, *buf_ptr++);
	romsize = len >> 2;
	while( --romsize ) {
		mpeg_write_sequence_command( *buf_ptr );
		buf_ptr++;
		printf("--------------------length still as 0x%08x\n", romsize);
	}

	romsize = len >> 2;
	buf_ptr = ptr;
	printf("verify sloader\n");

	check_fb = mpeg_read_command(SLOADER_JUMP_POINT);

	if(check_fb != *buf_ptr++) {
		printf("++++++++++++++++++++verify SLOARD failed@%x\n",SLOADER_JUMP_POINT);
//		return 0;
	}
	
	romsize1 = romsize;
	while( --romsize ) {
		check_fb = mpeg_read_sequence_command();
		
		if(check_fb != *buf_ptr) {
			printf("++++++++++++++++++++verify SLOARD failed@%x\n",(romsize1-romsize)<<2);
//			return 0;
		}
		buf_ptr++;
	}	
#endif

	free(ptr);
	return ret;
mpeg_open_error:
	free(ptr);
mpeg_malloc_error:
	ret = INSTALL_ERROR;
	return ret;
}

int read_flash_id(void)
{
	uint32_t ret = 0;
	mpeg_write_command(RF_SPI_DATA_HIGH, 0x0);
	mpeg_write_command(RF_SPI_DATA_LOW, 0x0);
	mpeg_write_command(RF_SPI_ADDR_LOW, 0x0);
	mpeg_write_command(RF_SPI_ADDR_HIGH, 0x0);
	mpeg_write_command(RF_SPI_CUST_CMD, 0x9093);
	while(0x80 & mpeg_read_command(RF_SPI_CUST_CMD));

	int id_high = mpeg_read_command(RF_SPI_DATA_HIGH);
	int id_low  = mpeg_read_command(RF_SPI_DATA_LOW);
//	printf("1 read_flash_id high:0x%08x, low:0x%08x\n\r", id_high, id_low);
	if((id_high & 0xff) == 0x1C)
	{
		ret = 1;
//		printf("EON 0x%08x flash found\n ",id_high >> 8);
	}
	
	mpeg_write_command(RF_SPI_DATA_HIGH, 0x0);
	mpeg_write_command(RF_SPI_DATA_LOW, 0x0);
	mpeg_write_command(RF_SPI_ADDR_LOW, 0x0);
	mpeg_write_command(RF_SPI_ADDR_HIGH, 0x0);
	mpeg_write_command(RF_SPI_CUST_CMD,0x9F93);
	while(0x80 & mpeg_read_command(RF_SPI_CUST_CMD));

	id_high = mpeg_read_command(RF_SPI_DATA_HIGH);
	id_low  = mpeg_read_command(RF_SPI_DATA_LOW);
	
//	printf("2 read_flash_id high:0x%08x, low:0x%08x\n\r", id_high, id_low);

//	printf("size = <%s> Type = <%08x>\n ", ((id_high&0xff)==0x15) ? "2MB" : "unknown" ,id_low >> 8);
	//if((dwHigh&0xff)==0x1C)
	//{

	//	printf("EON %dK flash found ",(1<<(dwHigh>>8)));
	//}
	return ret;
}

int get_flash_id(void)
{
	mpeg_write_command(RF_SFT_CFG19, 0x5400);
	mpeg_write_command(RF_SFT_CFG19, 0x3400);
	mpeg_write_command(RF_SPI_CTRL, 0x0c1b);

	read_flash_id();
	if(read_flash_id() == 0)
	{
		mpeg_write_command(RF_SPI_CTRL,  0x0cfb);
		mpeg_write_command(RF_SFT_CFG19, 0x3400);
		mpeg_write_command(RF_SFT_CFG19, 0x5400);
		if((read_flash_id() & 0xffff) == 0xffff)
		{
			return read_flash_id();
		}
	}
	return 0;
}
#if 0
uint32_t  mpeg_wait_return_test(void)
{
	int ret = 0, i;
	int out = 0;
	unsigned char cmd_fb;
	char *test_buf = "012345678\0";
	char test_cmp[10];
	while(1) {	
		/*
		ret = 10;
		i = 0;
		out = 0;
		while(ret > 0) {
			i = write(mpeg_fd, test_buf + out, ret);
			ret -= i;
			out += i;
		}
		ret = 10;
		i = 0;
		out = 0;
		while(ret > 0) {
			i = read(mpeg_fd, test_cmp + out, ret);
			ret -= i;
			out += i;
		}*/
		i = 0;
		do{
			ret = write(mpeg_fd, test_buf + i, 1);
			if(ret == 1) {
				i++;
			}
			usleep(10);
		} while(i < 10);
		i = 0;
		do{
			ret = read(mpeg_fd, test_cmp + i, 1);
			if(ret == 1) {
				i++;
			}
			usleep(10);
		} while(i < 10);
		printf("%s\n", test_cmp);
		usleep(1000);
	}
}
#endif

int reboot_into_sloader(void)
{
	unsigned int data;
	unsigned int sizewithcrc;
	unsigned char response[1];
	int ret = INSTALL_SUCCESS;
	int romsize = mpeg_get_file_size(rom_bin);
	romsize = ((romsize + 4095) / 4096) * 4096;
	mpeg_write_single_command('C');


	//write 
	mpeg_write_command(0x000257fc, romsize);
	printf("rom size set:0x000257fc 0x%08x[0x%08x]\n",romsize,mpeg_read_command(0x000257fc));
	mpeg_write_command(RF_RESET, 0x00000002);
	mpeg_write_command(RF_ADT_4, 0x00000000);
	mpeg_write_command(RF_DAT_1, 0x00000000);
	mpeg_write_command(RF_DAT_2, 0x00000000);
	mpeg_write_command(RF_DAT_3, 0x00000000);
	mpeg_write_command(RF_DAT_4, 0x00000000);
	mpeg_write_command(RF_DAT_5, 0x00000000);
	mpeg_write_command(RF_DAT_6, 0x00000000);
	mpeg_write_command(RF_ADM_1, 0x00000000);
	mpeg_write_command(RF_ADM_2, 0x00000000);
	mpeg_write_command(RF_ADM_3, 0x00000000);
	mpeg_write_command(RF_DAR,   0x00000000);


	get_flash_id();

	mpeg_write_command(0x1FFE9700, 0x2c1b);
	mpeg_write_command(0x1FFE9820, 0x3400);
	mpeg_write_command(0x1FFE9708, 0x6A0);

	while(mpeg_read_command(0x1FFE9708) & 128);
	mpeg_write_command(0x1FFE9714, 0);
	mpeg_write_command(0x1FFE9718, 0);
	mpeg_write_command(0x1FFE9708, 420);
	while(mpeg_read_command(0x1FFE9708) & 128);
/*FIXME, wrong read from 0x08000000 */
	data = mpeg_read_command(0x08000000);
	printf("0x08000000 = 0x%08x\n", data);
	if(data != 0xabcd)
		ret =  INSTALL_ERROR;
	mpeg_write_command(0x00000000, 0x08006400);
	mpeg_write_command(0x00000004, 0x00000000);
	mpeg_write_command(0x00000008, 0x00000000);

#define SFTCFG1_D_BOOT_SDRAM 	 (0x1 << 13)	
	data = mpeg_read_command(RF_SFT_CFG1);
	mpeg_write_command(RF_SFT_CFG1,data | SFTCFG1_D_BOOT_SDRAM);

#define SFTCFG2_D_BRP_EN (0x1 << 13)	

	data = mpeg_read_command(RF_SFT_CFG2);
//	printf("sft cfg 2 = %08x\n", data);
//	printf("Reboot from SDRAM\n");
	data = 0x8444;
	mpeg_write_command(RF_SFT_CFG2, data | 0x00000800);
//	printf("sft cfg 2 = %08x\n",   data | 0x00000800);

	mpeg_write_command(RF_RESET, 0x00000000);
	init_serial3_min();
	usleep(2000);

#if 0
	mpeg_wait_return_test();
#endif
//	return INSTALL_SUCCESS;
	return mpeg_wait_rebootX();
}

int mpeg_update(void)
{
	uint32_t ret = 0;
	uint32_t try_time = 0;
	FILE *fp;
burning:
	ret = 0;
	printf("-->MPEG update\n");
	ui_set_burning_type(BURNING_TYPE_MPEG);

	fp = fopen_path(boot_bin,"rb");
	if (fp == NULL) {
		printf("%s not existed\n",boot_bin);
		return INSTALL_NOT_EXISTED;
	}
	fp = fopen_path(rom_bin,"rb");
	if (fp == NULL) {
		printf("%s not existed\n",rom_bin);
		return INSTALL_NOT_EXISTED;
	}
	
	ui_set_burning_status();
	ui_set_burning_status_detail();
	/* get a feedback, and then check device ID 
	   to confirm transfer successfully or not*/
	ret = confirm_device();
	if (ret != INSTALL_SUCCESS) {
		LOGE("confirm device error\n");
		goto mpeg_error;
	}
	
	ret = init_sdram();
	if (ret != INSTALL_SUCCESS) {
		LOGE("init_sdram error\n");
		goto mpeg_error;
	}
	
	ui_set_burning_status();
	
	ret = send_sloader();
	if ( INSTALL_SUCCESS != ret ) {
		LOGE("send sloader error, stop running \n");
		goto mpeg_error;
//		return 0;
	}

	ret = reboot_into_sloader();
	if (ret != INSTALL_SUCCESS) {
		goto mpeg_error;
//		LOGE("Warning:may have potental issue there,Enter to Continue!\n");
	}
#if 0		//change for new download flow
	close(mpeg_fd);
	/* the reason to reset serial  is that while mpeg is reset ,it will cause serial DMA error*/
	init_serial3();
	usleep(2000);
	ret = wait_BIGA_reply_from_device();
	if ( INSTALL_SUCCESS != ret ) {
		LOGE("wait A error, stop running \n");
		goto mpeg_error;
	//		return 0;
	}
	/* change serial work mode ,that it will wait for fb forever and if no fb, never return */
	init_serial3_min();	
	usleep(2000);

	wait_LACK_from_device();
#endif

	send_start_wait_ready();
	ui_set_burning_status();

	transfer(1400);
//	printf("transfer end\n");
	printf("update mpeg success\n");
	close(mpeg_fd);
	return INSTALL_SUCCESS;
mpeg_error:
	//LOGE("check file /tractor/boot_flash.bin or /tractor/rom.bin in sdcard\n");
	printf("update mpeg error\n");
	close(mpeg_fd);
	try_time++;
	if(try_time < 3) {
		goto burning;
	}
	ret = INSTALL_ERROR;
	return ret;
}
