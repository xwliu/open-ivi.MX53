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
 * MC34708 PMIC SPI Read/Write Utilities
 */

#include <common.h>
#include <command.h>
#include <spi.h>
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>

#include <asm/arch/iomux-mx53-tractor.h>
#include <asm/arch/tractor_mx53_default_mux.h>
#include <asm/arch/tractor_mx53_default_pad.h>
#include <asm/arch/tractor_mx53_default_select_set.h>


/*-----------------------------------------------------------------------
 * Definitions
 */

#ifndef MAX_SPI_BYTES
#define MAX_SPI_BYTES 32	/* Maximum number of bytes we can handle */
#endif

/* Values from last command.
 */
static unsigned int	device;
static int   		bitlen;
static uchar 		dout[MAX_SPI_BYTES];
static uchar 		din[MAX_SPI_BYTES];
struct spi_slave * mc34708_slave;

typedef struct {
/*!
	 * Touch Screen X position
	 */
	unsigned int x_position;
	/*!
	 * Touch Screen X position1
	 */
	unsigned int x_position1;
	/*!
	 * Touch Screen X position2
	 */
	unsigned int x_position2;
	/*!
	 * Touch Screen X position3
	 */
	unsigned int x_position3;
	/*!
	 * Touch Screen Y position
	 */
	unsigned int y_position;
	/*!
	 * Touch Screen Y position1
	 */
	unsigned int y_position1;
	/*!
	 * Touch Screen Y position2
	 */
	unsigned int y_position2;
	/*!
	 * Touch Screen Y position3
	 */
	unsigned int y_position3;
	/*!
	 * Touch Screen contact value
	 */
	unsigned int contact_resistance;
} t_touch_screen;


int i2c_mc34708id(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 0;
	int j = 10;
	unsigned char buf[4] = { 0 };	
	if(argc < 2){
		printf("default loop is 10\n", argc);
	}else{
		j = simple_strtoul(argv[1], NULL, 10);
	}
	//setup_i2c(CONFIG_SYS_I2C_PORT);
	i2c_init(CONFIG_SYS_I2C_SPEED, 0);
	for(i = 0; i < j; i++){
		memset(&buf[0], 0, 4);
		i2c_read(0x8, 7, 1, &buf[0], 3);
		printf("i2c_read id got: 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2]);
	}
}

int do_mc34708_init(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("spi_mc34708 init\n");
	mc34708_slave = spi_setup_slave(1, 0, 100000, SPI_MODE_0);
}

int spi_Read34708reg(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u32 out_id = 0;
	int j = 0;
	
	if(argc < 2){
		printf("need reg addr\n");
		return;
	}else{
		j = simple_strtoul(argv[1], NULL, 10);
	}

	u32 reg_addr = j << 25;
	printf("MC34708 reg prepare addr: 0x%x\n", reg_addr);
	spi_xfer(mc34708_slave, 32, &reg_addr, &out_id, SPI_XFER_BEGIN | SPI_XFER_END);
	printf("MC34708 reg value: 0x%x\n", out_id);	
}

int spi_Write34708reg(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u32 reg_addr = 0;
	int value = 0;
	int old_value = 0;
	int j = 0;
	if(argc < 3){
		printf("need reg addr and value\n");
		return;
	}else{
		j = simple_strtoul(argv[1], NULL, 10);
		value = simple_strtoul(argv[2], NULL, 10);
	}

	reg_addr = (j << 25) | 0x80000000;
	reg_addr |= (value & 0x00FFFFFF);
	printf("MC34708 reg value: 0x%x\n", reg_addr);	
	spi_xfer(mc34708_slave, 32, &reg_addr, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	printf("MC34708 reg old value: 0x%x, value: 0x%x\n", old_value, value);
}

int spi_Capture34708reg(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u32 reg_value = 0;
	u32 old_value = 0;
	u32 value[8];
	static int continue_touchuup = 0;
	t_touch_screen touch = {0};

	int i = 0;
	continue_touchuup = 0;
	// enable the TPEN DOWN and ADC Done interrupt bit for polling
	reg_value = (0x80000000) | (1 << 25) | (0xFFFFF8);
	spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	// Enable the ADC and Touch Interface
	reg_value = (0x80000000) | (0x2B << 25) | (0x170000);	// ADC0
	spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	reg_value = (0x80000000) | (0x2C << 25) | (0xFFF000);	// ADC1
	spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	reg_value = (0x80000000) | (0x2D << 25) | (0x0);		// ADC2
	spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	reg_value = (0x80000000) | (0x2E << 25) | (0xF28500);	// ADC3
	spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	printf("Tap the Screen to enable the Touch detect process\n");
	for(i = 0; i < 1000; i++){
		reg_value = (0x0 << 25);
		spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
		if(old_value & 0x4){
			printf("@@Touch Pen Down Found!@@\n");
			reg_value = (0x80000000) | (0x0 << 25) | (0x4);	// clean the touch down interrupt
			spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
			reg_value = (0x0 << 25);
			spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
			if(old_value & 0x4){
				printf("Clean Pen Down Interrupt Error!\n");
			}
			break;
		}else{
			i++;
			udelay(10000);
			printf(".0x%x\n", old_value);
		}		
	}

	if(i == 1000){
		printf("Touch Pen Process End as no Touch Down Event\n");
		return -1;
	}
	reg_value = (0x80000000) | (0x2C << 25) | (0xFFF000);		
	spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	reg_value = (0x80000000) | (0x2D << 25) | (0x0);
	spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	reg_value = (0x80000000) | (0x2E << 25) | (0xF28500);
	spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	reg_value = (0x80000000) | (0x2B << 25) | (0x173000);	// Let's enable touch adc here
	spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
	do{
		printf("Waiting Touch ADC Done\n");
		reg_value = (0x80000000) | (0x2B << 25) | (0x170000);	// DISABLE FIRST AND THEN REENABLE 
		spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
		reg_value = (0x80000000) | (0x2B << 25) | (0x173000);	// Let's enable touch adc here
		spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
		for(i = 0; i < 1000; i++){
			reg_value = (0x0 << 25);
			spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
			if(old_value & 0x2){
				printf("##Touch ADC Done Found! 0x%x##\n", old_value);
				reg_value = (0x80000000) | (0x0 << 25) | (0x2);	// clean the touch down interrupt
				spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
				reg_value = (0x0 << 25);
				spi_xfer(mc34708_slave, 32, &reg_value, &old_value, SPI_XFER_BEGIN | SPI_XFER_END);
				if(old_value & 0x2){
					printf("Clean ADC Interrupt Error!\n");
				}
				break;
			}else{
				i++;
				udelay(1000);
				printf(".0x%x\n", old_value);
			}		
		}

		if(i == 1000){
			printf("Touch Pen Process End as no Touch ADC Done Event\n");
			return -1;
		}

		for (i = 0; i < 4; i++) {
			int reg = 0x2F + i;		// ADC4 First
			int result = 0;
			reg = (reg << 25);
			spi_xfer(mc34708_slave, 32, &reg, &result, SPI_XFER_BEGIN | SPI_XFER_END);
			value[i * 2] = (result & (0x3FF << 2)) >> 2;
			value[i * 2 + 1] = (result & (0x3FF << 14)) >> 14;
		}

		touch.x_position = value[0];
		touch.x_position1 = value[0];
		touch.x_position2 = value[1];
		touch.y_position = value[3];
		touch.y_position1 = value[3];
		touch.y_position2 = value[4];
		touch.contact_resistance = value[6];
		printf("[X: 0x%x, 0x%x, 0x%x]\r\n", touch.x_position,  touch.x_position1, touch.x_position2);
		printf("[Y: 0x%x, 0x%x, 0x%x]\r\n", touch.y_position, touch.y_position1, touch.y_position2);
		printf("[C: 0x%x]\r\n", touch.contact_resistance);		
		if((touch.contact_resistance > 1000) || (touch.contact_resistance == 0)){
			continue_touchuup++;
			if(continue_touchuup > 2){
			touch.contact_resistance = 0;
			touch.x_position = 0;
			touch.y_position = 0;		
			}else{
				touch.contact_resistance = 10;	// fake still touch down
			}
		}else{
			continue_touchuup = 0;	// we got any actual touch down event, so we clear all the previous "wrong" touch up
		}
	}while(touch.contact_resistance != 0);
}

int spi_All34708reg(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 0;
	u32 out_id = 0;
	for(i = 0; i < 64; i++){
		u32 reg_addr = i << 25;
		//printf("MC34708 reg prepare addr: 0x%x\n", reg_addr);
		spi_xfer(mc34708_slave, 32, &reg_addr, &out_id, SPI_XFER_BEGIN | SPI_XFER_END);
		printf("MC34708 index: %d, reg addr: 0x%x, reg value: 0x%x\n", i, reg_addr, out_id);
	}
}	



int spi_mc34708id(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u32 out_id = 0;
	int j = 10;
	int i = 0;
	u32 reg_addr = 7 << 25;
	if(argc < 2){
		printf("default loop is 10\n", argc);
	}else{
		j = simple_strtoul(argv[1], NULL, 10);
	}
	printf("MC34708 id reg prepare addr: 0x%x\n", reg_addr);
	for(i = 0; i < j ; i++){
		spi_xfer(mc34708_slave, 32, &reg_addr, &out_id, SPI_XFER_BEGIN | SPI_XFER_END);
		printf("MC34708 id reg value: 0x%x\n", out_id);	
	}
}

int do_mc34708_id(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
#ifdef CONFIG_SYS_I2C_MC34708
	i2c_mc34708id(cmdtp, flag, argc, argv);
#else
	//mxc_request_iomux(MX53_PIN_EIM_CS1, IOMUX_CONFIG_ALT2);
	//mxc_iomux_set_input(MUX_IN_ECSPI2_IPP_IND_MOSI_SELECT_INPUT, INPUT_CTL_PATH2);
	spi_mc34708id(cmdtp, flag, argc, argv);
#endif
}

int do_i2c_pad(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("1Get CSI0_D8 PAD value: 0x%x\n", mxc_iomux_get_pad(MX53_PIN_CSI0_D8));
	printf("1Get CSI0_D9 PAD value: 0x%x\n", mxc_iomux_get_pad(MX53_PIN_CSI0_D9));

		mxc_iomux_set_pad(MX53_PIN_CSI0_D8, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

		mxc_iomux_set_pad(MX53_PIN_CSI0_D9, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
	printf("2Get CSI0_D8 PAD value: 0x%x\n", mxc_iomux_get_pad(MX53_PIN_CSI0_D8));
	printf("2Get CSI0_D9 PAD value: 0x%x\n", mxc_iomux_get_pad(MX53_PIN_CSI0_D9));
}


/***************************************************/
// only for ecspi interface
U_BOOT_CMD(
	mc34708init,	1,	1,	do_mc34708_init,
	"tractor mc34708 communicate",
	"Call the commmand to test communicate with mc34708\n"
);

// this command support two connection way: i2c and ECSPI
U_BOOT_CMD(
	mc34708id,	4,	1,	do_mc34708_id,
	"tractor mc34708 communicate",
	"Call the commmand to test communicate with mc34708\n"
);

U_BOOT_CMD(
	i2cpad,	4, 	1,	do_i2c_pad,
	"tractor mc34708 communicate",
	"Call the commmand to test communicate with mc34708\n"
);

U_BOOT_CMD(
	r34708,	4,	1,	spi_Read34708reg,
	"tractor mc34708 communicate",
	"Call the commmand to test communicate with mc34708\n"
);


U_BOOT_CMD(
	w34708,	4,	1,	spi_Write34708reg,
	"tractor mc34708 communicate",
	"Call the commmand to test communicate with mc34708\n"
);

U_BOOT_CMD(
	c34708,	4,	1,	spi_Capture34708reg,
	"tractor mc34708 communicate",
	"Call the commmand to test communicate with mc34708\n"
);

U_BOOT_CMD(
	a34708,	4,	1,	spi_All34708reg,
	"tractor mc34708 communicate",
	"Call the commmand to test communicate with mc34708\n"
);


