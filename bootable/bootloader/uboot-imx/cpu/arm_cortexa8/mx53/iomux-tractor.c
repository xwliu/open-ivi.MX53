/*
 * Copyright 2004-2010 Freescale Semiconductor, Inc.
 * Copyright (C) 2008 by Sascha Hauer <kernel@pengutronix.de>
 * Copyright (C) 2009 by Jan Weitzel Phytec Messtechnik GmbH,
 *                       <armlinux@phytec.de>
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
 * 
 * Copyright (C) 2011 TokenWireless Tech, Inc
 */
#include <linux/string.h>
#include <common.h>
#include <asm/io.h>

#include <asm/arch/iomux-mx53-tractor.h>

static csp_iomux_mx53_regs *  g_pIOMGR_IOMUX; 
/*
 * setups a single pad in the iomuxer
 */
int mxc_iomux_tractor_set_mux(ddk_iomux_pin pin, ddk_iomux_pin_muxmode muxmode, ddk_iomux_pin_sion sion)
{
	// BE CAREFUL WHEN YOU WANT TO ENABLE THE DEBUG MSG! "printf" may still could not be used
	//printf("mux id: %d, value: 0x%x, offset: 0x%x\n", pin, (muxmode | sion), ((int)(&g_pIOMGR_IOMUX->SW_MUX_CTL[pin])) - ((int)(&g_pIOMGR_IOMUX->GPR0)));
	__raw_writel((muxmode | sion), &g_pIOMGR_IOMUX->SW_MUX_CTL[pin]);
	return 0;
}
EXPORT_SYMBOL(mxc_iomux_tractor_set_mux);


int mxc_iomux_tractor_get_mux(ddk_iomux_pin pin)
{
	return __raw_readl(&g_pIOMGR_IOMUX->SW_MUX_CTL[pin]);
}
EXPORT_SYMBOL(mxc_iomux_tractor_get_mux);



int mxc_iomux_tractor_set_pad(ddk_iomux_pad pad, ddk_iomux_pad_slew slew, 
    ddk_iomux_pad_drive drive, ddk_iomux_pad_opendrain openDrain, 
    ddk_iomux_pad_pull pull, ddk_iomux_pad_hysteresis hysteresis, 
    ddk_iomux_pad_inmode inputMode, ddk_iomux_pad_outvolt outputVolt)
	
{
	// BE CAREFUL WHEN YOU WANT TO ENABLE THE DEBUG MSG! "printf" may still could not be used
	//printf("pad id: %d, value: 0x%x, offset: 0x%x\n", pad, (slew | drive | openDrain | pull | hysteresis |inputMode | outputVolt), ((int)(&g_pIOMGR_IOMUX->SW_PAD_CTL[pad])) - ((int)(&g_pIOMGR_IOMUX->GPR0)));
	__raw_writel((slew | drive | openDrain | pull | hysteresis |inputMode | outputVolt), &g_pIOMGR_IOMUX->SW_PAD_CTL[pad]);
	return 0;
}
EXPORT_SYMBOL(mxc_iomux_tractor_set_pad);


int mxc_iomux_tractor_get_pad(ddk_iomux_pad pad)
{
	return __raw_readl(&g_pIOMGR_IOMUX->SW_PAD_CTL[pad]);
}
EXPORT_SYMBOL(mxc_iomux_tractor_get_pad);

int mxc_iomux_tractor_set_inputpath(ddk_iomux_select_input inputpath, u8 value)
{
	__raw_writel(value, &g_pIOMGR_IOMUX->SELECT_INPUT[inputpath]);
	return 0;
}
EXPORT_SYMBOL(mxc_iomux_tractor_set_inputpath);

int mxc_iomux_tractor_setup(iomux_set* default_mux_tab, int muxcount, pad_set* default_pad_tab, int tabcount, selectmux_set* default_selected_tab, int inputcount)
{
	unsigned i = 0;
	// BE CAREFUL WHEN YOU WANT TO ENABLE THE DEBUG MSG! "printf" may still could not be used	
	//printf("mux_count: %d, pad_count: %d, select_count: %d\n", muxcount, tabcount, inputcount);
	for(i = 0; i < muxcount; i++)
		mxc_iomux_tractor_set_mux(default_mux_tab[i].pinId, default_mux_tab[i].pinMuxMode, default_mux_tab[i].pinSION);
	
	for(i = 0; i < tabcount; i++){
		mxc_iomux_tractor_set_pad(default_pad_tab[i].padname,default_pad_tab[i].slew, 
		default_pad_tab[i].drive, default_pad_tab[i].opendrain, 
		default_pad_tab[i].pull, default_pad_tab[i].hysteresis, 
		default_pad_tab[i].inmode, default_pad_tab[i].outvolt);
	}
	
	
	for(i =0;i < inputcount; i++){
		mxc_iomux_tractor_set_inputpath(default_selected_tab[i].pinselectid, default_selected_tab[i].value);   //select SDA input		
	}
	
	return 0;
}
EXPORT_SYMBOL(mxc_iomux_tractor_setup);

void mxc_iomux_tractor_init(void  *iomux_v3_base)
{
	g_pIOMGR_IOMUX = (csp_iomux_mx53_regs*)iomux_v3_base;
}

void mxc_iomux_v3_tractor_init(void  *iomux_v3_base)
{
	mxc_iomux_tractor_init(iomux_v3_base);
}

