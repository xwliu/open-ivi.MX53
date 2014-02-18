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
 * tractor UDA1338 Utilities
 */
#include "uda1338.h"
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>

#include "mcu.h"

#define UDA1338_L3_WRITE_ADDRESS  0x2A
#define UDA1338_L3_READ_ADDRESS 0x2B

#ifdef CONFIG_TRACTOR_PLUS_OSV
#define GPIO_L3_MODE             (2*32 + 7)     /* GPIO_3_7 */
#define GPIO_L3_CLK                 (2*32 + 8)     /* GPIO_3_8 */
#define GPIO_L3_DAT                 (2*32 + 9)     /* GPIO_3_9 */
#else   // by default we using following pins
#define GPIO_L3_MODE             (2*32 + 16)     /* GPIO_3_16 */
#define GPIO_L3_CLK                 (2*32 + 17)     /* GPIO_3_17 */
#define GPIO_L3_DAT                 (2*32 + 18)     /* GPIO_3_18 */
#endif

#define L3_DELAY                        5                   // 5us

int uda1338_clock_high()
{
    mx53_gpio_set(2, 17, 1);
}

int uda1338_clock_low()
{
    mx53_gpio_set(2, 17, 0);
}

int uda1338_dat_high()
{
    mx53_gpio_set(2, 18, 1);
}

int uda1338_dat_low()
{
    mx53_gpio_set(2, 18, 0);
}

int uda1338_dat_input()
{
    mx53_gpio_direction(2, 18, 0);       /* GPIO_3_18 */
}

int uda1338_dat_output()
{
    mx53_gpio_direction(2, 18, 1);       /* GPIO_3_18 */
}

int uda1338_dat_input_value()
{
    if(mx53_gpio_get(2, 18) == 1)
        return 1;
    else
        return 0;
}

int uda1338_mode_device()
{
    mx53_gpio_set(2, 16, 0);
}

int uda1338_mode_other()
{
    mx53_gpio_set(2, 16, 1);
}

int uda1338_write_dev_address(u8 is_write)
{
    u32 i = 0;
    u8 byte = 0;
    if(is_write)
        byte = UDA1338_L3_WRITE_ADDRESS;
    else
        byte = UDA1338_L3_READ_ADDRESS;
   
    uda1338_clock_high();
    uda1338_mode_other();
    udelay(L3_DELAY);   // keep clock and mode both high sometime to make it stable...

    uda1338_mode_device();  // pull low to start...
    for(i = 0; i < 8; i++){
        uda1338_clock_low();
        if(byte & 0x1){
            uda1338_dat_high();
        }else{
            uda1338_dat_low();
        }
        udelay(L3_DELAY);
        uda1338_clock_high();
        udelay(L3_DELAY);
        byte >>= 1;
    }
    uda1338_clock_high();       // ensure be high
    uda1338_mode_other();   // ensure leave the device mode
    uda1338_dat_high(); // option set..
}

int uda1338_write_reg_address(u8 reg_address, u8 is_write)
{
    u32 i = 0;
    u8 byte = reg_address;
    if(!is_write)
        byte = reg_address | 0x80;
    uda1338_mode_other();   // ensure we are not in the device mode
    uda1338_clock_high();       // ensure be high
    for(i =0 ; i < 8; i++){
        uda1338_clock_low();
        if(byte & 0x80){
            uda1338_dat_high();
        }else{
            uda1338_dat_low();
        }
        udelay(L3_DELAY);
        uda1338_clock_high();
        udelay(L3_DELAY);
        byte <<= 1;
    }
    uda1338_clock_high();       // ensure be high
    uda1338_mode_other();   // ensure leave the device mode
    uda1338_dat_high(); // option set..
}

int uda1338_write_reg_value(u8 reg_val_h, u8 reg_val_l)
{
    u32 i = 0;
    u32 j = 0;
    u8 byte = 0;
    uda1338_mode_other();   // ensure we are not in the device mode
    uda1338_clock_high();       // ensure be high
    byte = reg_val_h;
    for(i =0 ; i < 8; i++){
        uda1338_clock_low();
        if(byte & 0x80){
            uda1338_dat_high();
        }else{
            uda1338_dat_low();
        }

        udelay(L3_DELAY);
        uda1338_clock_high();
        udelay(L3_DELAY);
        byte <<= 1;
    }
    byte = reg_val_l;
    for(i =0 ; i < 8; i++){
        uda1338_clock_low();
        if(byte & 0x80){
            uda1338_dat_high();
        }else{
            uda1338_dat_low();
        }
        udelay(L3_DELAY);
        uda1338_clock_high();
        udelay(L3_DELAY);
        byte <<= 1;
    }
}

int uda1338_read_reg_address(u8* p_reg_address)
{
    u32 i = 0;
    u8 byte = 0;
    uda1338_dat_input();        // make dat line as input
    uda1338_mode_other();   // ensure we are not in the device mode
    uda1338_clock_high();       // ensure be high
    for(i =0 ; i < 8; i++){
        uda1338_clock_low();
        udelay(L3_DELAY);
        uda1338_clock_high();
        if(uda1338_dat_input_value() == 1){
            byte |= (1 << (7-i));
        }        
        udelay(L3_DELAY);
    }
    uda1338_clock_high();       // ensure be high
    uda1338_mode_other();   // ensure leave the device mode
    *p_reg_address = byte;
}

int uda1338_read_reg_val(u8* p_reg_val_h, u8* p_reg_val_l)
{
    u32 i = 0;
    u8 byte = 0;
    uda1338_dat_input();        // make dat line as input
    uda1338_mode_other();   // ensure we are not in the device mode
    uda1338_clock_high();       // ensure be high
    for(i =0 ; i < 8; i++){
        uda1338_clock_low();
        udelay(L3_DELAY);
        uda1338_clock_high();
        if(uda1338_dat_input_value() == 1){
            byte |= (1 << (7-i));
        }        
        udelay(L3_DELAY);
    }
    *p_reg_val_h = byte;
    for(i =0 ; i < 8; i++){
        uda1338_clock_low();
        udelay(L3_DELAY);
        uda1338_clock_high();
        if(uda1338_dat_input_value() == 1){
            byte |= (1 << (7-i));
        }        
        udelay(L3_DELAY);
    }
    *p_reg_val_l = byte;
    uda1338_clock_high();       // ensure be high
    uda1338_mode_other();   // ensure leave the device mode
    uda1338_dat_output();
    uda1338_dat_high(); // option set..
}


int uda1338_write(u8 reg_address, u8 reg_val_h, u8 reg_val_l)
{
    printf("uda1338_read try write address: 0x%x, H: 0x%x, L: 0x%x\n", reg_address, reg_val_h, reg_val_l);
    uda1338_write_dev_address(1);   // is_write is true
    uda1338_write_reg_address(reg_address, 1);    // we are write
    uda1338_write_reg_value(reg_val_h, reg_val_l);
}

int uda1338_read(u8 reg_address)
{
    u8 verify_address = 0xFF;
    u8 reg_val_h = 0xFF;
    u8 reg_val_l = 0xFF;
    printf("uda1338_read try reg address: 0x%x\n", reg_address);
    uda1338_write_dev_address(1);   // is_write is true
    uda1338_write_reg_address(reg_address, 0);    // we are read
    uda1338_write_dev_address(0);   // is_write is false
    uda1338_read_reg_address(&verify_address); // get the address and valid
    printf("uda1338_read got reg address: 0x%x\n", verify_address);
    uda1338_read_reg_val(&reg_val_h, &reg_val_l);
    printf("uda1338_read val H: 0x%x, L: 0x%x\n", reg_val_h, reg_val_l);
}

int uda1338_init()
{
        // mux config already done during board_init mxc_iomux_tractor_setup
        // default all set to output
        mx53_gpio_direction(2, 16, 1);       /* GPIO_3_16 */
        mx53_gpio_direction(2, 17, 1);       /* GPIO_3_17 */
        mx53_gpio_direction(2, 18, 1);       /* GPIO_3_18 */
}
