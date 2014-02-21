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



static char mpeg_dev[] = "/dev/ttymxc2";
static char log_dev[] = "/dev/ttymxc0";
static int log_fd;
static int mpeg_fd;

static uint32_t read_buf[2];

/*aligned in 2-bytes*/
static uint16_t transfer_buf[2048];



static char boot_bin[]  = "/sdcard/tractor/boot_flash.bin";
static char rom_bin[]   = "/sdcard/tractor/rom.bin";
static char sload_bin[] = "/sdcard/tractor/sloader.bin";


static void log_write(const char *fmt, ...)
{
	char buf[512];
	va_list ap;

	if (log_fd < 0) return;

	va_start(ap, fmt);
	vsnprintf(buf, 512, fmt, ap);
	buf[511] = 0;
	va_end(ap);
	write(log_fd, buf, strlen(buf));
}

static int log_open(void)
{
	log_fd = open(log_dev, O_WRONLY | O_CREAT | O_TRUNC);
	return 0;
}

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
	tcsetattr(mpeg_fd, TCSANOW, &termios_new);
	termios_new.c_cflag &=  ~CSTOPB;		/* 1-bit stop */
	termios_new.c_cflag &= ~CRTSCTS;
	termios_new.c_cc[VTIME]  = 0;			/* wait forever while 0 */
	termios_new.c_cc[VMIN] = 0;			/* wait for more than 1 byte */
	tcflush(mpeg_fd, TCIFLUSH);
	if((tcsetattr(mpeg_fd, TCSANOW, &termios_new)) != 0)
	{
		log_write("serial set error\n");
		return 1;
	}
	log_write("open and set success!\n");
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
	cfsetispeed(&termios_new, B230400);	
	cfsetospeed(&termios_new, B230400);		/* set baudrate of IN/OUT */
	tcsetattr(mpeg_fd, TCSANOW, &termios_new);
	termios_new.c_cflag &=  ~CSTOPB;		/* 1-bit stop */
	termios_new.c_cflag &= ~CRTSCTS;
	termios_new.c_cc[VTIME]  = 0;			/* wait forever while 0 */
	termios_new.c_cc[VMIN] = 1;			/* wait for more than 1 byte */
	tcflush(mpeg_fd, TCIFLUSH);
	if((tcsetattr(mpeg_fd, TCSANOW, &termios_new)) != 0)
	{
		log_write("serial set error\n");
		return 1;
	}
	log_write("open and set success!\n");
	return 0;
}



/* input file name, and return file size */
unsigned int get_file_size(char *file_name)
{
	FILE * File;
	unsigned int size;
	File = fopen(&file_name[0], "rb");
	if(File == NULL) {
		log_write("Error while open file: %s\n", file_name);
		return 0;
	}
	/* point to the end of file */
	fseek(File, 0, SEEK_END);
	/* get the offset of the point and the start of file */
	size = ftell(File);
	fclose(File);
	return size;
}


static int get_a_reply(void)
{
	char out_buf[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	char in_buf[255] =  {0};
	uint32_t len = 0;
	unsigned int ret, i;

	while (1) {
		/* send a out */
		ret = write(mpeg_fd, out_buf, strlen(out_buf));
		for ( i = 0; i < 9 ; i++ ) {
			/* read from mpeg, while a return, clear fifo */
			ret = read(mpeg_fd, in_buf, 1);
			log_write("--------------------little a ret : %d\n",ret);
			usleep(10);
			if (ret) {
				if (in_buf[0] == A_CMD_L) {
					log_write("--------------------Get Response a From Device,%x\n", in_buf[0]);
					ret = read(mpeg_fd, in_buf, 255);
					return 1;
				}
			}
		}

	}
	return 0;
}

uint32_t  mpeg_wait_rebootX(void)
{
	int ret = 0, i;
	unsigned char cmd_fb;
	for ( i = 0; i < 1000 ; i++ ) {
		/* read from mpeg, while REBOOT_X_CMD return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
		log_write("--------------------RebootX ret : %d\n",ret);
		if (ret) {
			if (cmd_fb == REBOOT_X_CMD) {
				log_write("--------------------Get Response REBOOT_X_CMD From Device,%x\n", cmd_fb);
				return 1;
			}
		}
		usleep(1000);
	}
	log_write("++++++++++++++++++++Do not get Response REBOOT_X_CMD From Device\n");

/*	
	ret = read(mpeg_fd, &cmd_fb, 1);
	if (cmd_fb == REBOOT_X_CMD) {
		log_write("wait for rebootX success\n");
	}
*/
	return 0;
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
		log_write("--------------------read_cmd ret : %d\n",ret);
		usleep(1000);
		if (cmd_fb == READ_CMD_L) {
			log_write("--------------------Get Response READ_CMD_L From Device,%x\n", cmd_fb);
			ret = read(mpeg_fd, read_buf, 8);
			return read_buf[0];
		}
	}
	log_write("++++++++++++++++++++Do not get Response READ_CMD_L From Device\n");
	return 0;
}


uint32_t mpeg_read_command(unsigned int addr)
{
	int ret = 0,i;
	unsigned char cmd[5];
	unsigned char cmd_fb;
	log_write("--------------------read addr is 0x%08x\n", addr);

	cmd[0] = READ_CMD;
	cmd[1] = addr & 0xff;
	cmd[2] = (addr >> 8) & 0xff;
	cmd[3] = (addr >> 16) & 0xff;
	cmd[4] = (addr >> 24) & 0xff;
	ret = write(mpeg_fd, cmd, 5);
	
	for ( i = 0; i < 100 ; i++ ) {
		/* read from mpeg, while READ_CMD return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
		log_write("--------------------read_cmd ret : %d\n",ret);
		usleep(1000);
		if (ret) {
			log_write("--------------------read_cmd cmd_fb : %x\n",cmd_fb);
			if (cmd_fb == READ_CMD) {
				log_write("--------------------Get Response READ_CMD From Device,%x\n", cmd_fb);
				ret = read(mpeg_fd, read_buf, 8);
				return read_buf[0];
			}
		}
	}
	log_write("++++++++++++++++++++Do not get Response READ_CMD From Device\n");
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
		log_write("--------------------write_cmd_l ret : %d\n",ret);
		usleep(1000);
		if (ret) {
			if (cmd_fb == WRITE_CMD_L) {
				log_write("--------------------Get Response WRITE_CMD_L From Device,%x\n", cmd_fb);
				return 1;
			}
		}
	}
	log_write("++++++++++++++++++++Do not get Response WRITE_CMD_L From Device\n");

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
		log_write("--------------------single cmd ret : %d\n",ret);
		usleep(1000);
		if (ret) {
			if (cmd_fb == cmd) {
				log_write("--------------------Get Response single cmd From Device,%x\n", cmd_fb);
				return 1;
			}
		}
	}
	log_write("++++++++++++++++++++Do not get Response single cmd From Device\n");

	
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
		log_write("--------------------write_cmd ret : %d\n",ret);
		usleep(1000);
		if (ret) {
			if (cmd_fb == WRITE_CMD) {
				log_write("--------------------Get Response WRITE_CMD From Device,%x\n", cmd_fb);
				return 1;
			}
		}
	}
	log_write("++++++++++++++++++++Do not get Response WRITE_CMD From Device\n");

	return 0;
}

int wait_BIGA_reply_from_device(void)
{
	log_write("Wait for device power on/reset!\n");
	int ret = 0, i;
	unsigned char cmd;
	unsigned char cmd_fb;

	cmd = A_CMD;
	
	while (1) {
		ret = write(mpeg_fd, &cmd, 1);
//		for ( i = 0; i < 9 ; i++ ) {
			ret = read(mpeg_fd, &cmd_fb, 1);
//			log_write("--------------------BIGA ret : %d\n",ret);
			if (ret) {
				log_write("BIGA cmd_fb : %c\n",cmd_fb);
				if (cmd_fb == A_CMD) {
					log_write("--------------------Get Response A From Device, A_CMD, %x\n", cmd_fb);
					return 1;
				} 
			}
			usleep(50);
//		}
	}
	return 0;
}


uint32_t wait_LACK_from_device(void)
{
	log_write("Wait LACK from device!\n");
	int ret = 0, i;
	unsigned char cmd_fb;

	for ( i = 0; i < 1000; i++ ) {
		/* read from mpeg, while LACK_CMD return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
		log_write("--------------------lack_cmd ret : %d\n",ret);
		if (ret) {
			log_write("BIGA cmd_fb : %c\n",cmd_fb);
			if (cmd_fb == LACK_CMD) {
				log_write("--------------------Get Response LACK_CMD From Device,%x\n", cmd_fb);
				return 1;
			}
		}
		usleep(1000);
	}
	log_write("++++++++++++++++++++Do not get Response LACK_CMD From Device\n");

	return 0;
}

uint32_t wait_EACK_from_device(void)
{
	log_write("Wait EACK from device!\n");
	int ret = 0, i;
	unsigned char cmd_fb;

	for ( i = 0; i < 1000; i++ ) {
		/* read from mpeg, while EACK_CMD return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
		log_write("--------------------lack_cmd ret : %d\n",ret);
		if (ret) {
			log_write("BIGA cmd_fb : %c\n",cmd_fb);
			if (cmd_fb == EACK_CMD) {
				log_write("--------------------Get Response EACK_CMD From Device,%x\n", cmd_fb);
				return 1;
			}
		}
		usleep(1000);
	}
	log_write("++++++++++++++++++++Do not get Response EACK_CMD From Device\n");

	return 0;
}



static int get_device_id(void)
{
	unsigned int id = 0;
	id = mpeg_read_command(RF_STAMP);
	
	log_write("id: 0x%08x \n", id);
	if ( 0xD0 == (unsigned char)id) {
		log_write("read id success\n");
	} else {
		log_write("read id fail\n");
	}
	return 0;
}

static int confirm_device(void)
{
	get_a_reply();
	get_device_id();
	return 0;
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
	log_write("get %d frame\n",frameid);
//	memset(transfer_ptr, 0x00, 4096);
	*(transfer_ptr + 0) = frameid & 0xff;
	*(transfer_ptr + 1) = 0;
	
	log_write("memcpy start\n");
	memcpy(transfer_ptr + 2, buf_ptr + frameid * 4090, 4090);
	log_write("memcpy end, and strt to check sum\n");
	checksum_frame = get_checksum( (uint16_t *)(transfer_ptr +2), 4090);

	*(transfer_ptr + 4092) = (checksum_frame >> 0) & 0xff;
	*(transfer_ptr + 4093) = (checksum_frame >> 8) & 0xff;
	*(transfer_ptr + 4094) = (checksum_frame >> 16) & 0xff;
	*(transfer_ptr + 4095) = (checksum_frame >> 24) & 0xff;
}

//#define MIN((a),(b))  ((a) < (b) ? (a) : (b))
#define MIN(a,b)  (a < b ? a : b)

void mpeg_send_frame(unsigned char *transfer_ptr, unsigned int frame_size)
{
//	unsigned int frame_size_all = 4096;
	unsigned int frame_send_end = 0;
	unsigned int frame_send_remain = 4096;

	
	while (frame_send_remain) {
		frame_send_end = write(mpeg_fd, (transfer_ptr + frame_send_end), 
				       MIN(frame_size, frame_send_remain));
		frame_send_remain -= frame_send_end;
	}
}

unsigned char mpeg_wait_return(void)
{
	int ret = 0, i;
	unsigned char cmd_fb;

	for ( i = 0; i < 1000; i++ ) {
		/* read from mpeg, while feedback return, clear fifo */
		ret = read(mpeg_fd, &cmd_fb, 1);
		log_write("--------------------mpeg wait ret : %d\n",ret);
		if (ret) {
			log_write("mpeg wait cmd_fb : %c\n",cmd_fb);
			switch (cmd_fb) {
			case 'O':
			case 'X':
			case 'E':
			case 'L':
				return cmd_fb;
			default:
				break;
			}
		}
		usleep(1000);
	}
	log_write("++++++++++++++++++++Do not get Response mpeg wait From Device\n");
	return 0;
}

/* frame_size maybe not equal to frame size need to be send once */
static int transfer(uint32_t frame_size)
{
	log_write("start to transfer\n");
//	tcflush(mpeg_fd, TCIFLUSH);
	
	unsigned int romsize = get_file_size(rom_bin);
	romsize = ((romsize + 4089) / 4090) * 4090;
	log_write("romsize 0x%08x\n",romsize);
	unsigned char *ptr = malloc( sizeof(char) * romsize );
	if (ptr == NULL) {
		log_write("malloc failure\n");
		return 0;
	}
	FILE *fp = fopen(rom_bin, "rb");
	if (fp == NULL) {
		log_write("read rom data error\n");
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
	romsize -= 4090 ;
	while (1) {
		log_write("transfer frame %d\n", frameid);
		mpeg_send_frame(transfer_ptr, frame_size);

		mpeg_fb = mpeg_wait_return();
		switch (mpeg_fb) {
			case 'O':
				log_write("romsize %x\n", romsize);
				if (romsize == 0) {
					wait_EACK_from_device();
					goto transfer_success;
				}
				next_frame = 1;
				break;
			case 'X':
			case 'L':
				next_frame = 0;
				break;
			case 'E':
				goto transfer_success;
		}
		if (1 == next_frame) {
			frameid += 1;
			get_frame(transfer_ptr, buf_ptr, frameid);
			romsize -= 4090;

		}
	}
	return 0;
transfer_success:
	free(ptr);
	return 1;
}

static int test_serial(void)
{
	char out_byte = 0;
	char in_byte = 0;
	int i;
	for (i = 65; i <= 122; i++) {
		out_byte = i;
		write(mpeg_fd, &out_byte, 1);
		read(mpeg_fd, &in_byte, 1);

		log_write("out 0x%08x, in 0x%08x\n", out_byte, in_byte);
	}
	return 0;
}

uint32_t init_sdram(void)
{

	unsigned int memaddr,data;
	unsigned int edi_bef = 0;

	unsigned int count =0;
	int ret = 0;
	unsigned int times = 0;
	log_write("Begin to configure SDRAM...\n");
	log_write("Warning:Current value define for winbond W9812G6JH \n");
	log_write("Warning:if you changed sdram please reconfig parameters \n");
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

		//log_write("RF_LBC_CONTROL [1FFE8130]%08x\n",data);

		data = mpeg_read_command( 0x0 );
		log_write("[%d] %08x\n",count,data);
		//sleep(10);
		usleep(10 * 1000);

		if(data == 0x1234567)
		{
			times++;
			if(times > 13){
				ret = 1;

				//mpeg_write_command(RF_SDCTRL_CFG4,0x00000a68);	//0x00000a68);//0x00001a8c
				log_write("found magic number @0x%08x\n",0x00000200<<count);
				log_write("init sdram done\n");
				return 1;
				break;
			}
		}
	}

	log_write("Init SDRAM failed!\n");
	return 0;
}

uint32_t send_sloader(void)
{	
	unsigned int romsize, romsize1;
	log_write("send sloader start..\n");
	unsigned int check_fb;
	uint32_t len = 0;

#define LOADER_BIN	boot_bin
	len = get_file_size(LOADER_BIN);
	log_write("sloader true len : 0x%08x\n",len);
	/* len be aligned to 4-byte. so malloc size will be enough to read in data */
	if (len & 3) {
		len = len | (~3);
		len = len + 4;
	}
	log_write("sloader len after aligned 4-bytes:  0x%08x\n",len);
	int *ptr = malloc(sizeof(char) * len);
	FILE *fp = fopen(LOADER_BIN, "rb");
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
	while (romsize-- ) {
		mpeg_write_command(addr, *buf_ptr);
		check_fb = mpeg_read_command(addr);
		if (check_fb != *buf_ptr) {
			log_write("++++++++++++++++++++verify SLOARD failed@%x\n", (len >> 2) - romsize);
		}
		log_write("verify SLOARD check_fb:0x%08x, buf:0x%08x, addr:0x%08x, buf_ptr:0x%08x\n",
					check_fb, *buf_ptr, addr, buf_ptr);
		addr += 4;
		buf_ptr++;
	}
	log_write("verify success\n");
#if 0
	mpeg_write_command(SLOADER_JUMP_POINT, *buf_ptr++);
	romsize = len >> 2;
	while( --romsize ) {
		mpeg_write_sequence_command( *buf_ptr );
		buf_ptr++;
		log_write("--------------------length still as 0x%08x\n", romsize);
	}

	romsize = len >> 2;
	buf_ptr = ptr;
	log_write("verify sloader\n");

	check_fb = mpeg_read_command(SLOADER_JUMP_POINT);

	if(check_fb != *buf_ptr++) {
		log_write("++++++++++++++++++++verify SLOARD failed@%x\n",SLOADER_JUMP_POINT);
//		return 0;
	}
	
	romsize1 = romsize;
	while( --romsize ) {
		check_fb = mpeg_read_sequence_command();
		
		if(check_fb != *buf_ptr) {
			log_write("++++++++++++++++++++verify SLOARD failed@%x\n",(romsize1-romsize)<<2);
//			return 0;
		}
		buf_ptr++;
	}	
#endif
	free(ptr);
	return 1;
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

	if((id_high & 0xff) == 0x1C)
	{
		ret = 1;
		log_write("EON 0x%08x flash found\n ",id_high >> 8);
	}
	
	mpeg_write_command(RF_SPI_DATA_HIGH, 0x0);
	mpeg_write_command(RF_SPI_DATA_LOW, 0x0);
	mpeg_write_command(RF_SPI_ADDR_LOW, 0x0);
	mpeg_write_command(RF_SPI_ADDR_HIGH, 0x0);
	mpeg_write_command(RF_SPI_CUST_CMD,0x9F93);
	while(0x80 & mpeg_read_command(RF_SPI_CUST_CMD));

	id_high = mpeg_read_command(RF_SPI_DATA_HIGH);
	id_low  = mpeg_read_command(RF_SPI_DATA_LOW);

	log_write("size = <%s> Type = <%08x>\n ", ((id_high&0xff)==0x15) ? "2MB" : "unknown" ,id_low >> 8);
	//if((dwHigh&0xff)==0x1C)
	//{

	//	log_write("EON %dK flash found ",(1<<(dwHigh>>8)));
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

int reboot_into_sloader(void)
{
	unsigned int data;
	unsigned int sizewithcrc;
	unsigned char response[1];
	int ret =1;
	int romsize = get_file_size(rom_bin);
	romsize = ((romsize + 4089) / 4090) * 4090;
	mpeg_write_single_command('C');


	//write 
	mpeg_write_command(0x000257fc, romsize);
	log_write("rom size set:0x000257fc 0x%08x[0x%08x]\n",romsize,mpeg_read_command(0x000257fc));
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
	log_write("0x08000000 = 0x%08x\n", data);
	if(data != 0xabcd)
		ret =  0;
	mpeg_write_command(0x00000000, 0x08006400);
	mpeg_write_command(0x00000004, 0x00000000);
	mpeg_write_command(0x00000008, 0x00000000);

#define SFTCFG1_D_BOOT_SDRAM 	 (0x1 << 13)	
	data = mpeg_read_command(RF_SFT_CFG1);
	mpeg_write_command(RF_SFT_CFG1,data | SFTCFG1_D_BOOT_SDRAM);

#define SFTCFG2_D_BRP_EN (0x1 << 13)	

	data = mpeg_read_command(RF_SFT_CFG2);
	log_write("sft cfg 2 = %08x\n", data);
	log_write("Reboot from SDRAM\n");
	data = 0x8444;
	mpeg_write_command(RF_SFT_CFG2, data | 0x00000800);
	log_write("sft cfg 2 = %08x\n",   data | 0x00000800);

	mpeg_write_command(RF_RESET, 0x00000000);

	mpeg_wait_rebootX();

	return ret;
}


int main(void)
{
	uint32_t ret = 0, len = 0;
	
	log_open();

	init_serial3();
#if 0
	test_serial();
#else
	/* get a feedback, and then check device ID 
	   to confirm transfer successfully or not*/
	confirm_device();
	
	init_sdram();
	
	ret = send_sloader();
	if ( 0 == ret ) {
		log_write("send sloader error, stop running \n");
//		return 0;
	}

	if (reboot_into_sloader() == 0) {
//		log_write("Warning:may have potental issue there,Enter to Continue!\n");
		
	}
	
	close(mpeg_fd);
	init_serial3_min();
	
	wait_BIGA_reply_from_device();

	wait_LACK_from_device();
	
	transfer(2048);
	log_write("transfer end\n");

#endif
	close(mpeg_fd);
	close(log_fd);
	return 0;
}

