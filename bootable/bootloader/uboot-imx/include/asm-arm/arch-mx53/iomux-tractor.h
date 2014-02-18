/*
 * Copyright (C) 2009 by Jan Weitzel Phytec Messtechnik GmbH,
 *			<armlinux@phytec.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Copyright (C) 2011/2012 TokenWireless Tech, Inc
 */

#ifndef __MACH_IOMUX_TRACTOR_H__
#define __MACH_IOMUX_TRACTOR_H__

//#define IOMUX_TO_IRQ_V3(pin)	(MXC_GPIO_IRQ_START + pin)

#define IOMUX_SW_MUX_CTL_MUX_MODE_LSH       0
#define IOMUX_SW_MUX_CTL_SION_LSH           4

#define IOMUX_SW_PAD_CTL_SRE_LSH            0
#define IOMUX_SW_PAD_CTL_DSE_LSH            1
#define IOMUX_SW_PAD_CTL_ODE_LSH            3
#define IOMUX_SW_PAD_CTL_PUS_LSH            4
#define IOMUX_SW_PAD_CTL_PUE_LSH            6
#define IOMUX_SW_PAD_CTL_PKE_LSH            7
#define IOMUX_SW_PAD_CTL_HYS_LSH            8
#define IOMUX_SW_PAD_CTL_DDR_INPUT_LSH      9
#define IOMUX_SW_PAD_CTL_HVE_LSH            13

#define IOMUX_SELECT_INPUT_DAISY_LSH        0

// SW_MUX_CTL
#define IOMUX_SW_MUX_CTL_MUX_MODE_ALT0      0   // Select ALT0 mux mode
#define IOMUX_SW_MUX_CTL_MUX_MODE_ALT1      1   // Select ALT1 mux mode
#define IOMUX_SW_MUX_CTL_MUX_MODE_ALT2      2   // Select ALT2 mux mode
#define IOMUX_SW_MUX_CTL_MUX_MODE_ALT3      3   // Select ALT3 mux mode
#define IOMUX_SW_MUX_CTL_MUX_MODE_ALT4      4   // Select ALT4 mux mode
#define IOMUX_SW_MUX_CTL_MUX_MODE_ALT5      5   // Select ALT5 mux mode
#define IOMUX_SW_MUX_CTL_MUX_MODE_ALT6      6   // Select ALT6 mux mode
#define IOMUX_SW_MUX_CTL_MUX_MODE_ALT7      7   // Select ALT7 mux mode

#define IOMUX_SW_MUX_CTL_SION_REGULAR       0   // Input is determined by mux mode
#define IOMUX_SW_MUX_CTL_SION_FORCE         1   // Force input of some pad

// SW_PAD_CTL
#define IOMUX_SW_PAD_CTL_SRE_SLOW           0   // Slow slew rate
#define IOMUX_SW_PAD_CTL_SRE_FAST           1   // Fast slew rate

#define IOMUX_SW_PAD_CTL_DSE_NORMAL         0   // Normal drive strength
#define IOMUX_SW_PAD_CTL_DSE_MEDIUM         1   // Medium drive strength
#define IOMUX_SW_PAD_CTL_DSE_HIGH           2   // High drive strength
#define IOMUX_SW_PAD_CTL_DSE_MAX            3   // Maximum drive strength

#define IOMUX_SW_PAD_CTL_ODE_DISABLE        0   // Disable open drain
#define IOMUX_SW_PAD_CTL_ODE_ENABLE         1   // Enable open drain

#define IOMUX_SW_PAD_CTL_PUS_360K_DOWN      0   // 100K Ohm pull down
#define IOMUX_SW_PAD_CTL_PUS_75K_UP         1   // 47K Ohm pull up
#define IOMUX_SW_PAD_CTL_PUS_100K_UP        2   // 100K Ohm pull up
#define IOMUX_SW_PAD_CTL_PUS_22K_UP         3   // 22K Ohm pull up

#define IOMUX_SW_PAD_CTL_PUE_KEEPER         0  // Keeper enable
#define IOMUX_SW_PAD_CTL_PUE_PULL           1  // Pull up/down enable

#define IOMUX_SW_PAD_CTL_PKE_DISABLE        0   // Pull up/down/keeper disabled
#define IOMUX_SW_PAD_CTL_PKE_ENABLE         1   // Pull up/down/keeper enabled

#define IOMUX_SW_PAD_CTL_HYS_DISABLE        0   // Disable hysteresis
#define IOMUX_SW_PAD_CTL_HYS_ENABLE         1   // Enable hysteresis

#define IOMUX_SW_PAD_CTL_DDR_INPUT_CMOS     0   // CMOS input
#define IOMUX_SW_PAD_CTL_DDR_INPUT_DDR      1   // DDR input

#define IOMUX_SW_PAD_CTL_HVE_LOW            0   // Low output voltage
#define IOMUX_SW_PAD_CTL_HVE_HIGH           1   // High output voltage

#endif /* __MACH_IOMUX_TRACTOR_H__*/

