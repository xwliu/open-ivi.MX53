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
 * tractor tw8832 Utilities
 */
#include "mcu.h"
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>
#include <i2c.h>

extern uint32_t i2c_port_num;

int mcu_i2c_init()
{
	i2c_port_num = 2;
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);	
}

s32 mcu_write_reg(u8 reg, u8 val)
{
	u8 u8Buf[2] = {0};
	u8Buf[0] = reg;
	u8Buf[1] = val;
	i2c_port_num = 2;
	i2c_write(MCU_I2C_ADDRESS, 0, 0, u8Buf, 2);
	return 0;
}

s32 mcu_read(u8 reg)
{
	u8 val = 0;
	i2c_port_num = 2;
	if (i2c_read(MCU_I2C_ADDRESS, reg, 0, &val, 1) < 0) {
		return -1;
	}
	//printf("mcu read 0x%x\n", val);
	return val;
}
