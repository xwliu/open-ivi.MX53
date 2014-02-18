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
#include "tw8832.h"
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>
#include <i2c.h>

#include "mcu.h"


extern uint32_t i2c_port_num;
s32 tw8832_read(u8 reg);

s32 tw8832_write_reg(u8 reg, u8 val)
{
	u8 u8Buf[2] = {0};
	int try_num = 10;
	u8Buf[0] = reg;
	u8Buf[1] = val;
	i2c_port_num = 2;
	i2c_write(TW8832_I2C_ADDRESS, 0, 0, u8Buf, 2);
	return 0;
}

s32 tw8832_read(u8 reg)
{
	u8 val = 0;
	i2c_port_num = 2;
	if (i2c_read(TW8832_I2C_ADDRESS, reg, 0, &val, 1) < 0) {
		return -1;
	}
	//printf("tw8832 read 0x%x\n", val);
	return val;
}

static int tw8832_cvbs_auto_detection()
{
	const int MAX_RETRY = 100;	
	int retry = 0;
	int count = 0;
	int cvbs_type = 0;
	int delay = 10;	
	tw8832_write_reg(0xff, 0x01);	
	do{			
		cvbs_type = tw8832_read(0x1c);
		if((cvbs_type & 0x80) == 0){
			count++;
			if(count > 3){	// we confirm three times as the tw8832 sometimes may give us wrong info...
				break;
			}else{
				delay = 45; // in this case, we will check the tw8832 very quickly to save some time
			}
		}else{
			count = 0;
			delay = 10;
		}
		udelay(1000*delay);
		retry++;
	}while(retry < MAX_RETRY);
	
	return ((cvbs_type & 0x70) >> 4);
}

static int tw8832_cvbs_pal_ntsc_select()
{
	u32 i = 0;
	int cvbs_type = 0;
	//autodetection
	cvbs_type = tw8832_cvbs_auto_detection();
	switch(cvbs_type){
	case 1://TVin_PAL_BGHID;
	case 4://TVin_PAL_M;
	case 5://TVin_PAL_CombinationN;
	case 6://TVin_PAL_60;
		for(i = 0; i < sizeof(tw8832_pal_channel) / sizeof(tw8832_pal_channel[0]); i++){
			tw8832_write_reg(tw8832_pal_channel[i].tregister, tw8832_pal_channel[i].tvalue);
		}
		printf("P\r\n");
	    break;
	default:
		for(i = 0; i < sizeof(tw8832_ntsc_channel) / sizeof(tw8832_ntsc_channel[0]); i++){
			tw8832_write_reg(tw8832_ntsc_channel[i].tregister, tw8832_ntsc_channel[i].tvalue);
		}
		printf("N\r\n");
		break;		
	}
	return 0;
}


void tw8832_reverse()
{

}

//#define GPIO_TRACTOR_TW8832_RST				(2*32 + 14)	/* GPIO_3_14 */

 void tw8832_reset()
 {
	unsigned char tw8832_id = 0;
	int i = 0;
	printf("tw8832 reset\n");	
	// reset 8832 first
	mcu_write_reg(MCU_TW8832_RESET_ADDRESS, 1);
	i2c_port_num  = 2;
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
#if 1	
	tw8832_write_reg(0xff, 0x00);	
	tw8832_id = tw8832_read(0);
	if(tw8832_id  != 0x30){
		printf("i2c initialize got first byte wrong: 0x%x, it should be 0x%x\n", tw8832_id, 0x30);
		return -1;
	}
#endif	
	else{
		printf("tw8832 id is 0x%x\n", tw8832_id);
	}

	for(i = 0; i < sizeof(tw8832_para_pages) / sizeof(tw8832_para_pages[0]); i++){
		tw8832_write_reg(tw8832_para_pages[i].tregister, tw8832_para_pages[i].tvalue);
		//printf("w %x %x\n", tw8832_para_pages[i].tregister, tw8832_para_pages[i].tvalue);
	}
	tw8832_cvbs_pal_ntsc_select();
	
	printf("tw8832 reset end\n");	
	return 0;
 }


