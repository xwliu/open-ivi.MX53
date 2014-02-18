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
 * @file ipu_prp_vf.c
 *
 * @brief IPU Use case for PRP-VF
 *
 * @ingroup IPU
 */

#include <common.h>
#include <ipu.h>
#include <linux/types.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/errno.h>
#include "ipu_regs.h"
#include "mxc_v4l2_capture.h"
#include "ipu_prp_sw.h"
#include "lcd.h"


/*
 * Function definitions
 */

/*!
 * prpvf_start - start the vf task
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 */
static int prpvf_start(void *private)
{
	cam_data *cam = (cam_data *) private;
	ipu_channel_params_t vf;
	u32 size = 3, temp = 0;
	int err = 0, i = 0;
	dma_addr_t dummy = 0x97B00000;

	short *tmp, color;

	if (!cam) {
		printk(KERN_ERR "private is NULL\n");
		return -EIO;
	}

	memset(&vf, 0, sizeof(ipu_channel_params_t));
	ipu_csi_get_window_size(&vf.csi_prp_vf_mem.in_width,  &vf.csi_prp_vf_mem.in_height, cam->csi);
	vf.csi_prp_vf_mem.out_pixel_fmt = vf.csi_prp_vf_mem.in_pixel_fmt = IPU_PIX_FMT_RGB24;
	vf.csi_prp_vf_mem.in_width = vf.csi_prp_vf_mem.out_width = cam->win.w.width;
	vf.csi_prp_vf_mem.in_height = vf.csi_prp_vf_mem.out_height = cam->win.w.height;
	vf.csi_prp_vf_mem.csi = cam->csi;
	size = cam->win.w.width * cam->win.w.height * size;
	ipu_csi_enable_mclk(cam->csi, 1, 1);
	err = ipu_init_channel(CSI_PRP_VF_MEM, &vf);

	err = ipu_init_channel_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER,
				      vf.csi_prp_vf_mem.out_pixel_fmt, cam->win.w.width,
				      cam->win.w.height,
				      cam->win.w.width,
				      dummy, 0, 0, 0);
	if (err != 0) {
		printk(KERN_ERR "Error initializing CSI_PRP_VF_MEM\n");
	}

	ipu_enable_channel(CSI_PRP_VF_MEM);
	ipu_select_buffer(CSI_PRP_VF_MEM, IPU_OUTPUT_BUFFER, 0);

	return err;
}

/*!
 * prpvf_stop - stop the vf task
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 */
static int prpvf_stop(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0, i = 0;

	ipu_disable_channel(CSI_PRP_VF_MEM);

	ipu_uninit_channel(CSI_PRP_VF_MEM);

	return err;
}

/*!
 * Enable csi
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int prp_vf_enable_csi(void *private)
{
	cam_data *cam = (cam_data *) private;

	return ipu_enable_csi(cam->csi);
}

/*!
 * Disable csi
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int prp_vf_disable_csi(void *private)
{
	cam_data *cam = (cam_data *) private;

	return ipu_disable_csi(cam->csi);
}

/*!
 * function to select PRP-VF as the working path
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 * @return  status
 */
int prp_vf_sdc_select(void *private)
{
	cam_data *cam;
	int err = 0;
	if (private) {
		cam = (cam_data *) private;
		cam->vf_start_sdc = prpvf_start;
		cam->vf_stop_sdc = prpvf_stop;
		cam->vf_enable_csi = prp_vf_enable_csi;
		cam->vf_disable_csi = prp_vf_disable_csi;
	} else
		err = -EIO;

	return err;
}

/*!
 * function to de-select PRP-VF as the working path
 *
 * @param private    cam_data * mxc v4l2 main structure
 *
 * @return  int
 */
int prp_vf_sdc_deselect(void *private)
{
	cam_data *cam;
	int err = 0;
	err = prpvf_stop(private);

	if (private) {
		cam = (cam_data *) private;
		cam->vf_start_sdc = NULL;
		cam->vf_stop_sdc = NULL;
		cam->vf_enable_csi = NULL;
		cam->vf_disable_csi = NULL;
	}
	return err;
}
