/*
 * Copyright 2004-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @brief bootloader tractor capture file
 * 
 * TokenWireless Co.Ltd, All rights reserved 2011-2012
 */
#include <common.h>
#include <ipu.h>
#include <linux/types.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/errno.h>
#include "ipu_regs.h"
#include "mxc_v4l2_capture.h"

static cam_data *g_cam = 0;
extern void *lcd_base;	

static void camera_callback(u32 mask, void *dev)
{	
	return;
}

static void init_camera_struct(cam_data *cam)
{
//	printf("Init Camera struct\n");

	/* Default everything to 0 */
	memset(cam, 0, sizeof(cam_data));

	/* setup cropping */
	cam->current_input = 1;	// using CSI MEM now
	cam->crop_bounds.left = 0;
	cam->crop_bounds.width = 843;
	cam->crop_bounds.top = 0;
	cam->crop_bounds.height = 520;
	cam->crop_current = cam->crop_defrect = cam->crop_bounds;
	cam->v2f.fmt.pix.width = 800;
	cam->v2f.fmt.pix.height = 480;
	cam->v2f.fmt.pix.pixelformat = IPU_PIX_FMT_RGB565;
	cam->v2f.fmt.pix.sizeimage = 800 * 480 *  2;
	cam->v2f.fmt.pix.bytesperline = 800 * 2;
	cam->win.w.width = 800;
	cam->win.w.height = 480;
	cam->win.w.left = 0;
	cam->win.w.top = 0;
	cam->crop_current.top = 0x1f;
	cam->crop_current.left = 0x03;
	cam->crop_current.width = 800;
	cam->crop_current.height = 480;

	cam->csi = 1;  
	cam->enc_callback = camera_callback;
}

void mxc_camera_init(int way)
{
	u32 buffer_num = 0;
	ipu_csi_signal_cfg_t csi_param;
	csi_param.sens_clksrc = 0;
	csi_param.clk_mode = 0;
	csi_param.data_pol = 0;
	csi_param.ext_vsync = 1;
	csi_param.pack_tight = 0;
	csi_param.force_eof = 0;
	csi_param.data_en_pol = 0;
	csi_param.pixclk_pol = 1;
	csi_param.Vsync_pol = 0;
	csi_param.Hsync_pol = 1;
	csi_param.csi = 1;		
	csi_param.mclk = 35000000;
	csi_param.data_width = IPU_CSI_DATA_WIDTH_8;
	g_cam = malloc(sizeof(cam_data));
	if (g_cam == NULL) {
		printf("ERROR: v4l2 capture: failed to register camera\n");
		return -1;
	}

	printf("mxc_camera_init\n");
	init_camera_struct(g_cam);
	ipu_csi_set_window_size(g_cam->crop_current.width, g_cam->crop_current.height, g_cam->csi);
	ipu_csi_set_window_pos(g_cam->crop_current.left, g_cam->crop_current.top, g_cam->csi);
	ipu_csi_init_interface(g_cam->crop_bounds.width, g_cam->crop_bounds.height, IPU_PIX_FMT_RGB24, csi_param);
	if(way == 0)
		csi_enc_select(g_cam);
	//else if(way ==1)
		//prp_enc_select(g_cam);
	//else
		//prp_vf_sdc_select(g_cam);
	
	if(way < 2){
		g_cam->enc_enable(g_cam);
		buffer_num = 0;
		g_cam->enc_update_eba(0x97B00000 , &buffer_num);
		g_cam->enc_enable_csi(g_cam);
	}else{
		//g_cam->vf_start_sdc(g_cam);
		//g_cam->vf_enable_csi(g_cam);
	}
	printf("enable!\n");
}

void mxc_camera_deinit()
{
	g_cam->enc_disable_csi(g_cam);
	g_cam->enc_disable(g_cam);
}

void updateBufferAddr(u32 newaddr, int way)
{
	u32 buffer_num = 0;
	if(g_cam == 0)
		mxc_camera_init(way);
	g_cam->enc_update_eba(newaddr, &buffer_num);
}	

void ipu_cleanint_stat10()
{
	int i = __raw_readl(IPU_INT_STAT(10));
	printk(KERN_INFO "IPU_INT_STAT_10 = \t0x%08X\n", i);
	if(i != 0)
		__raw_writel(i, IPU_INT_STAT(10));
	printk(KERN_INFO "IPU_INT_STAT_10 = \t0x%08X\n",	__raw_readl(IPU_INT_STAT(10)));
}

void ipu_dump()
{
	ipu_dump_registers();
	printk(KERN_INFO "IPU_INT_STAT_1 = \t0x%08X\n", __raw_readl(IPU_STAT));
	printk(KERN_INFO "IPU_INT_CTRL_1 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(1)));
	printk(KERN_INFO "IPU_INT_CTRL_2 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(2)));
	printk(KERN_INFO "IPU_INT_CTRL_3 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(3)));
	printk(KERN_INFO "IPU_INT_CTRL_4 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(4)));
	printk(KERN_INFO "IPU_INT_CTRL_5 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(5)));
	printk(KERN_INFO "IPU_INT_CTRL_6 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(6)));
	printk(KERN_INFO "IPU_INT_CTRL_7 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(7)));
	printk(KERN_INFO "IPU_INT_CTRL_8 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(8)));
	printk(KERN_INFO "IPU_INT_CTRL_9 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(9)));
	printk(KERN_INFO "IPU_INT_CTRL_10 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(10)));
	printk(KERN_INFO "IPU_INT_CTRL_11 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(11)));
	printk(KERN_INFO "IPU_INT_CTRL_12 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(12)));
	printk(KERN_INFO "IPU_INT_CTRL_13 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(13)));
	printk(KERN_INFO "IPU_INT_CTRL_14 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(14)));
	printk(KERN_INFO "IPU_INT_CTRL_15 = \t0x%08X\n",	__raw_readl(IPU_INT_CTRL(15)));

	printk(KERN_INFO "IPU_INT_STAT_1 = \t0x%08X\n",		__raw_readl(IPU_INT_STAT(1)));
	printk(KERN_INFO "IPU_INT_STAT_2 = \t0x%08X\n",		__raw_readl(IPU_INT_STAT(2)));
	printk(KERN_INFO "IPU_INT_STAT_3 = \t0x%08X\n",		__raw_readl(IPU_INT_STAT(3)));
	printk(KERN_INFO "IPU_INT_STAT_4 = \t0x%08X\n",		__raw_readl(IPU_INT_STAT(4)));
	printk(KERN_INFO "IPU_INT_STAT_5 = \t0x%08X\n",		__raw_readl(IPU_INT_STAT(5)));
	printk(KERN_INFO "IPU_INT_STAT_6 = \t0x%08X\n",		__raw_readl(IPU_INT_STAT(6)));
	printk(KERN_INFO "IPU_INT_STAT_7 = \t0x%08X\n",		__raw_readl(IPU_INT_STAT(7)));
	printk(KERN_INFO "IPU_INT_STAT_8 = \t0x%08X\n",		__raw_readl(IPU_INT_STAT(8)));
	printk(KERN_INFO "IPU_INT_STAT_9 = \t0x%08X\n",		__raw_readl(IPU_INT_STAT(9)));
	printk(KERN_INFO "IPU_INT_STAT_10 = \t0x%08X\n",	__raw_readl(IPU_INT_STAT(10)));
	printk(KERN_INFO "IPU_INT_STAT_11 = \t0x%08X\n",	__raw_readl(IPU_INT_STAT(11)));
	printk(KERN_INFO "IPU_INT_STAT_12 = \t0x%08X\n",	__raw_readl(IPU_INT_STAT(12)));
	printk(KERN_INFO "IPU_INT_STAT_13 = \t0x%08X\n",	__raw_readl(IPU_INT_STAT(13)));
	printk(KERN_INFO "IPU_INT_STAT_14 = \t0x%08X\n",	__raw_readl(IPU_INT_STAT(14)));
	printk(KERN_INFO "IPU_INT_STAT_15 = \t0x%08X\n",	__raw_readl(IPU_INT_STAT(15)));
	
	printk(KERN_INFO "IPU_CHA_BUF0_RDY(27) = \t0x%08X\n", __raw_readl(IPU_CHA_BUF0_RDY(27)));
	printk(KERN_INFO "IPU_CHA_BUF0_RDY(23) = \t0x%08X\n", __raw_readl(IPU_CHA_BUF0_RDY(23)));	
	printk(KERN_INFO "IPU_CHA_CUR_BUF(27) = \t0x%08X\n", __raw_readl(IPU_CHA_CUR_BUF(27)));	
	printk(KERN_INFO "IDMAC_CH_BUSY1(27) = \t0x%08X\n", __raw_readl(IDMAC_CH_BUSY1));
	ipu_ch_param_dump(23);
	ipu_ch_param_dump(27);	
	ipu_ch_param_dump(15);	
	ipu_ch_param_dump(20);			
	ipu_ch_param_dump(0);		
}
