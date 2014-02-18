/*
 * (C) Copyright 2002
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/*
 * Copyright 2010-2012 TokenWireless Comm Co., Ltd. All Rights Reserved.
 */
/*
 * MCU Read/Write Utilities
 */

#include <common.h>
#include <command.h>
#include <spi.h>
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>
#include <i2c.h>

#ifndef CONFIG_SYS_I2C_MCU
/*-----------------------------------------------------------------------
 * Definitions
 */

#ifndef MAX_SPI_BYTES
#   define MAX_SPI_BYTES 32	/* Maximum number of bytes we can handle */
#endif

#ifndef CONFIG_DEFAULT_SPI_BUS
#   define CONFIG_DEFAULT_SPI_BUS	0
#endif
#ifndef CONFIG_DEFAULT_SPI_MODE
#   define CONFIG_DEFAULT_SPI_MODE	SPI_MODE_2
#endif

/*
 * Values from last command.
 */
static unsigned int	device;
static int   		bitlen;
static uchar 		dout[MAX_SPI_BYTES];
static uchar 		din[MAX_SPI_BYTES];
struct spi_slave *mcu_slave;

int mcu_ready()
{
	u32 timeout = 0;
	const u32 MAX_TIMEOUT = 1000;

	timeout = 0;
	while((mx53_gpio_get(3,5) == 1) && timeout < MAX_TIMEOUT){
		udelay(100);
		printf(".");
		timeout++;
	}
	if(timeout < MAX_TIMEOUT)
		return 1;
	else
		return 0;
}

int do_mcu_xfer(int reg_add, int reg_value)
{
	u32 p;
	u32 q;
	u32 tmp;
	int i = 0;

	if(!mcu_ready()){
		printf("MCU PIN IS NOT READY!Exit\n");
		goto Err;
	}
	
	p = reg_add;
	//printf("++Spi Send p value: 0x%x\n", p);
	spi_xfer(mcu_slave, 8, &p, &tmp, SPI_XFER_BEGIN | SPI_XFER_END);
	//printf("--Spi Got q value: 0x%x\n", q);
	udelay(10000);
	if(!mcu_ready()){
		printf("MCU PIN IS NOT READY!Exit\n");
		goto Err;
	}
	udelay(10000);
		
	p = reg_value;
	//printf("++Spi Send p value: 0x%x\n", p);			
	// send 0x80 means we want to read
	spi_xfer(mcu_slave, 8, &p, &q, SPI_XFER_BEGIN | SPI_XFER_END);
	//printf("--Spi Got q value: 0x%x, tmp value: 0x%x\n", q, tmp);
	udelay(10000);
	if(!mcu_ready()){
		printf("MCU PIN IS NOT READY!Exit\n");
		goto Err;
	}
	udelay(10000);

Err:	
	return q;
}

int mcu_init()
{
	mcu_slave = spi_setup_slave(0, 0, 100000, SPI_MODE_2);
	// When working with MCU, after initialized the SPI interface, we found the first byte that MCU receive will be wrong value
	// So in this case, let's do a dummy read/write operation to bypass this issue
	// TODO: Check this issue later...
	do_mcu_xfer(0x00,0xFF);
}

int do_mcu_init(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	mcu_init();
}

int do_mcu_id(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("MCU ID is:  0x%x\n", do_mcu_xfer(0x91, 0xFF));
}

int do_mcu_akey(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("AKEY1 VALUE is:  0x%x\n", do_mcu_xfer(0x89, 0xFF));
	printf("AKEY2 VALUE is:  0x%x\n", do_mcu_xfer(0x8A, 0xFF));	
}

#else
int do_mcu_init(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("do_mcu_init\n");
	mcu_i2c_init();
	//mcu_read(0x91);
	return 0;
}
int do_mcu_id(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("do_mcu_id\n");
	return 0;
}
int do_mcu_akey(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{			
	printf("do_mcu_akey\n");
	return 0;	
}

int do_mcuread(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int reg_addr = 0;
	int times = 1;
	int i = 0;
	if(argc < 2){
		printf("mcu read arg is too less 0x%x\n", argc);
		return;
	}
	reg_addr = simple_strtoul(argv[1], NULL, 10);
	if(argc > 2){
		times = simple_strtoul(argv[2], NULL, 10);
	}
	printf("mcu read try %d\n", times);
	for(i = 0 ; i < times; i++)
		printf("mcu read 0x%x value: 0x%x\n", reg_addr, mcu_read(reg_addr));
}

int do_mcuwrite(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int reg_addr = 0;
	int reg_value = 0;
	if(argc < 2){
		printf("mcu read arg is too less 0x%x\n", argc);
		return;
	}
	reg_addr = simple_strtoul(argv[1], NULL, 10);
	reg_value = simple_strtoul(argv[2], NULL, 10); 
	printf("mcu write reg: 0x%x, value: 0x%x\n", reg_addr, reg_value);	
	mcu_write_reg(reg_addr, reg_value);
	//printf("mcu confirm 0x%x value: 0x%x\n", reg_addr, tw8832_read(reg_addr));
}

int do_mcubkl(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int reg_addr = 0;
	int reg_value = 0;
	int i = 0;
	for(i = 0; i < 0xFF; i++){
		mcu_write_reg(0x10, i);
		udelay(50000);
	}	
}

int do_mcuburn(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 0;
	int j = 0;
	printf("mcu read reg: 0x14, value: 0x%x\n", mcu_read(0x94));
	mcu_write_reg(0x14, 0x5a);
	for(j = 0; j < 128; j++){
		for(i = 0; i < 128; i++){
			mcu_write_reg(0x14,i);
			printf("mcu loop %d\n", i);	
		}
		//udelay(1000 * 1000);
	}
}



U_BOOT_CMD(
	wmcu,	4,	1,	do_mcuwrite,
	"mcu write command",
	"wmcu\n"
);
U_BOOT_CMD(
	rmcu,	4,	1,	do_mcuread,
	"mcu read command",
	"rmcu\n"
);

U_BOOT_CMD(
	bmcu,	4,	1,	do_mcubkl,
	"mcu read command",
	"rmcu\n"
);
U_BOOT_CMD(
	umcu,	4,	1,	do_mcuburn,
	"mcu read command",
	"rmcu\n"
);

#endif

/***************************************************/

U_BOOT_CMD(
	mcui,	1,	1,	do_mcu_init,
	"tractor mcu communicate",
	"Call the commmand to test communicate with mcu\n"
);

U_BOOT_CMD(
	mcud,	1,	1,	do_mcu_id,
	"tractor mcu communicate",
	"Call the commmand to test communicate with mcu\n"
);

U_BOOT_CMD(
	mcuk,	1,	1,	do_mcu_akey,
	"tractor mcu communicate",
	"Call the commmand to test communicate with mcu\n"
);
