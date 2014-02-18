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
 * tractor tw8832  test Utilities
 */

#include <common.h>
#include <command.h>
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

int do_i2c3test(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 0;
	int j = 9;
	if(argc < 2){		
		printf("no count!!\n", argc);		
	} 
	else{
		j = simple_strtoul(argv[1], NULL, 10); 
	}
	mxc_request_iomux(MX53_PIN_GPIO_6, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_GPIO_3, IOMUX_CONFIG_ALT1);
	//mxc_iomux_set_pad(MX53_PIN_GPIO_6,  PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH | PAD_CTL_360K_PD |PAD_CTL_HYS_ENABLE);
	mx53_gpio_direction(0, 6, 1);
	mx53_gpio_direction(0, 3, 1);
	for (i = 0; i < j; i++) {
		mx53_gpio_set(0, 6, 1);
		mx53_gpio_set(0, 3, 1);
		udelay(5);
		mx53_gpio_set(0, 6, 0);
		mx53_gpio_set(0, 3, 0);
		udelay(5);
	}

	mx53_gpio_set(0, 6, 1);
	mx53_gpio_set(0, 3, 1);
}

int do_i2c3testi(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	mxc_request_iomux(MX53_PIN_GPIO_6, IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
	mxc_request_iomux(MX53_PIN_GPIO_3, IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
}

int do_i2c3testii(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	mxc_iomux_v3_tractor_init(IOMUXC_BASE_ADDR);
	mxc_iomux_tractor_set_mux(DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_6, DDK_IOMUX_PIN_MUXMODE_ALT2, DDK_IOMUX_PIN_SION_FORCE);
	mxc_iomux_tractor_set_mux(DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_3, DDK_IOMUX_PIN_MUXMODE_ALT2, DDK_IOMUX_PIN_SION_FORCE);
}



int do_i2c3tests(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	mxc_iomux_set_input(MUX_IN_I2C3_IPP_SCL_IN_SELECT_INPUT, INPUT_CTL_PATH1);
	mxc_iomux_set_input(MUX_IN_I2C3_IPP_SCL_IN_SELECT_INPUT, INPUT_CTL_PATH1);
}

/***************************************************/

U_BOOT_CMD(
	i2c3,	4,	1,	do_i2c3test,
	"i2c test command",
	"i2c\n"
);


U_BOOT_CMD(
	i2c3i,	4,	1,	do_i2c3testi,
	"i2c test command",
	"i2c\n"
);

U_BOOT_CMD(
	i2c3s,	4,	1,	do_i2c3tests,
	"i2c test command",
	"i2c\n"
);

U_BOOT_CMD(
	i2c3i2,	4,	1,	do_i2c3testii,
	"i2c test command",
	"i2c\n"
);

int do_tw8832Init(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	tw8832_reset();
}

int do_tw8832read(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int reg_addr = 0;
	if(argc < 2){
		printf("tw8832 read arg is too less 0x%x\n", argc);
		return;
	}
	reg_addr = simple_strtoul(argv[1], NULL, 10);
	printf("tw8832 read 0x%x\n", reg_addr);
	printf("tw8832 read 0x%x value: 0x%x\n", reg_addr, tw8832_read(reg_addr));
}

int do_tw8832write(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int reg_addr = 0;
	int reg_value = 0;
	if(argc < 2){
		printf("tw8832 read arg is too less 0x%x\n", argc);
		return;
	}
	reg_addr = simple_strtoul(argv[1], NULL, 10);
	reg_value = simple_strtoul(argv[2], NULL, 10); 
	printf("tw8832 write reg: 0x%x, value: 0x%x\n", reg_addr, reg_value);	
	tw8832_write_reg(reg_addr, reg_value);
	printf("tw8832 confirm 0x%x value: 0x%x\n", reg_addr, tw8832_read(reg_addr));
}

/***************************************************/

U_BOOT_CMD(
	i8832,	4,	1,	do_tw8832Init,
	"8832 initialize command",
	"i8832\n"
);

U_BOOT_CMD(
	r8832,	4,	1,	do_tw8832read,
	"8832 read command",
	"r8832\n"
);

U_BOOT_CMD(
	w8832,	4,	1,	do_tw8832write,
	"8832 read command",
	"r8832\n"
);
