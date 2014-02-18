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

/***************************************************/

int do_uda1338read(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    u32 reg_addr = 0xFF;
    uda1338_init();
    if(argc < 1){
        printf("uda1338 read arg is too less 0x%x\n", argc);
        return;
    }
    reg_addr = simple_strtoul(argv[1], NULL, 10);
    uda1338_read(reg_addr); 
}

int do_uda1338dump(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    u32 reg_addr = 0xFF;
    uda1338_init();
    uda1338_read(0x00); 
    uda1338_read(0x01); 
    uda1338_read(0x02); 
    uda1338_read(0x0F); 
    uda1338_read(0x10); 
    uda1338_read(0x11); 
    uda1338_read(0x12); 
    uda1338_read(0x13); 
    uda1338_read(0x14); 
    uda1338_read(0x15); 
    
    uda1338_read(0x16); 
    uda1338_read(0x17); 
    uda1338_read(0x18); 
    uda1338_read(0x19); 
    uda1338_read(0x1A); 
    uda1338_read(0x1B); 
    uda1338_read(0x1C); 
    uda1338_read(0x1D); 
    uda1338_read(0x20); 
    uda1338_read(0x21); 

    uda1338_read(0x30); 
    uda1338_read(0x31); 

}


int do_uda1338write(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    u32 reg_addr = 0xFF;
    u32 value_h = 0xFF;
    u32 value_l = 0xFF;
    uda1338_init();

    if(argc < 4){
        printf("uda1338 read arg is too less 0x%x\n", argc);
        return;
    }
    reg_addr = simple_strtoul(argv[1], NULL, 10);
    value_h = simple_strtoul(argv[2], NULL, 10);
    value_l = simple_strtoul(argv[3], NULL, 10);
    uda1338_write(reg_addr, value_h, value_l);
}

/***************************************************/

U_BOOT_CMD(
	ruda,	4,	1,	do_uda1338read,
	"1338 read command",
	"ruda\n"
);

U_BOOT_CMD(
	duda,	4,	1,	do_uda1338dump,
	"1338 read command",
	"ruda\n"
);


U_BOOT_CMD(
	wuda,	4,	1,	do_uda1338write,
	"1338 read command",
	"wuda\n"
);
