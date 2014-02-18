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
 * tractor mcu Utilities
 */

#ifndef __TRACTOR_MCU_H
#define __TRACTOR_MCU_H
#include <common.h>
#include <command.h>
#define MCU_I2C_ADDRESS	0x29

#define MCU_PWM_BKL_ADDRESS 0x10
#define MCU_5V_POWER_ADDRESS 0x19
#define MCU_BT_POWER_ADDRESS 0x18
#define MCU_MPEG_POWER_ADDRESS 0x17
#define MCU_MPEG_RESET_ADDRESS 0x2D
#define TRACTOR_REG_OFFSET_V14CTRL_PWN	0x3B
#define MCU_TW8832_RESET_ADDRESS	0x29


#define MCU_VEHICLES_STATUS 0x86

#define MCU_ID_ADDRESS 0x91
#define MCU_AKEY1_ADDRESS 0x89
#define MCU_AKEY2_ADDRESS 0x8a
#define MCU_VER_ADDRESS_HI	0x92
#define MCU_VER_ADDRESS_LO	0x93

#define MCU_BOOT_MODE		0xAE
#define MCU_BOOT_MODE_VALUE_NORMAL	0
#define MCU_BOOT_MODE_VALUE_REVE		2

#define MCU_BOOT_MODE_CLEAN_BOOT		(1 << 7)
#define MCU_BOOT_MODE_RECOVERY			(1 << 6)
#define MCU_BOOT_MODE_UPDATE			(1 << 5)

#define MCU_HARDWARE_ID_VERIFY 0x5a

int mcu_init();
s32 mcu_write_reg(u8 reg, u8 val);
s32 mcu_read(u8 reg);
#endif
