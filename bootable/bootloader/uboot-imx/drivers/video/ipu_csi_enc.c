/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc. All Rights Reserved.
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

extern void *lcd_base;	

/*! * csi ENC callback function.
 *
 * @param irq       int irq line
 * @param dev_id    void * device id
 *
 * @return status   IRQ_HANDLED for handled
 */
static void csi_enc_callback(int irq, void *dev_id)
{
	cam_data *cam = (cam_data *) dev_id;

	if (cam->enc_callback == NULL)
		return ;

	cam->enc_callback(irq, dev_id);
	return ;
}

/*!
 * CSI ENC enable channel setup function
 *
 * @param cam       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int csi_enc_setup(cam_data *cam)
{
	ipu_channel_params_t params;
	u32 pixel_fmt;
	int err = 0, sensor_protocol = 0;
	dma_addr_t dummy0 = 0x97BBB800;
	dma_addr_t dummy1 = 0x97B00000;	

	if (!cam) {
		printf("cam private is NULL\n");
		return -ENXIO;
	}
	//printf("csi_enc_setup\n");
	memset(&params, 0, sizeof(ipu_channel_params_t));
	params.csi_mem.csi = cam->csi;

	sensor_protocol = ipu_csi_get_sensor_protocol(cam->csi);
//	printf("csi_enc_setup: sensor_protocol: %d, pixelformat: 0x%x\n", sensor_protocol, cam->v2f.fmt.pix.pixelformat);	
	switch (sensor_protocol) {
	case IPU_CSI_CLK_MODE_GATED_CLK:
	case IPU_CSI_CLK_MODE_NONGATED_CLK:
	case IPU_CSI_CLK_MODE_CCIR656_PROGRESSIVE:
	case IPU_CSI_CLK_MODE_CCIR1120_PROGRESSIVE_DDR:
	case IPU_CSI_CLK_MODE_CCIR1120_PROGRESSIVE_SDR:
		params.csi_mem.interlaced = 0;
		break;
	case IPU_CSI_CLK_MODE_CCIR656_INTERLACED:
	case IPU_CSI_CLK_MODE_CCIR1120_INTERLACED_DDR:
	case IPU_CSI_CLK_MODE_CCIR1120_INTERLACED_SDR:
		params.csi_mem.interlaced = 1;
		break;
	default:
		printf("sensor protocol unsupported\n");
		return -EINVAL;
	}
	if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420)
		pixel_fmt = IPU_PIX_FMT_YUV420P;
	else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV422P)
		pixel_fmt = IPU_PIX_FMT_YUV422P;
	else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY)
		pixel_fmt = IPU_PIX_FMT_UYVY;
	else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
		pixel_fmt = IPU_PIX_FMT_YUYV;
	else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12)
		pixel_fmt = IPU_PIX_FMT_NV12;
	else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_BGR24)
		pixel_fmt = IPU_PIX_FMT_BGR24;
	else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24)
		pixel_fmt = IPU_PIX_FMT_RGB24;
	else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565)
		pixel_fmt = IPU_PIX_FMT_RGB565;
	else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_BGR32)
		pixel_fmt = IPU_PIX_FMT_BGR32;
	else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB32)
		pixel_fmt = IPU_PIX_FMT_RGB32;
	else if(cam->v2f.fmt.pix.pixelformat == IPU_PIX_FMT_GENERIC)
		pixel_fmt = IPU_PIX_FMT_GENERIC;
	else {
		printf("format not supported\n");
		return -EINVAL;
	}

	ipu_csi_enable_mclk(cam->csi, 1, 1);

	err = ipu_init_channel(CSI_MEM, &params);
	if (err != 0) {
		printf("ipu_init_channel %d\n", err);
		return err;
	}
//	printf("csi_enc_setup -> ipu_init_channel_buffer \n");
	err = ipu_init_channel_buffer(CSI_MEM, IPU_OUTPUT_BUFFER,
				      pixel_fmt, cam->v2f.fmt.pix.width,
				      cam->v2f.fmt.pix.height,
				      cam->v2f.fmt.pix.width,
				      dummy1, 0,
				      cam->offset.u_offset,
				      cam->offset.v_offset);
	if (err != 0) {
		printf("CSI_MEM output buffer\n");
		return err;
	}
	err = ipu_enable_channel(CSI_MEM);
	if (err < 0) {
		printf("ipu_enable_channel CSI_MEM\n");
		return err;
	}

	return err;
}

/*!
 * function to update physical buffer address for encorder IDMA channel
 *
 * @param eba         physical buffer address for encorder IDMA channel
 * @param buffer_num  int buffer 0 or buffer 1
 *
 * @return  status
 */
static int csi_enc_eba_update(dma_addr_t eba, int *buffer_num)
{
	int err = 0;
	//printf("csi_enc_eba_update, addr: 0x%x, buffer_num: %d\n", eba, *buffer_num);
	err = ipu_update_channel_buffer(CSI_MEM, IPU_OUTPUT_BUFFER,
					*buffer_num, eba);
	if (err != 0) {
		ipu_clear_buffer_ready(CSI_MEM, IPU_OUTPUT_BUFFER,
				       *buffer_num);

		err = ipu_update_channel_buffer(CSI_MEM, IPU_OUTPUT_BUFFER,
						*buffer_num, eba);
		if (err != 0) {
			printf("ERROR: v4l2 capture: fail to update buf %d\n", *buffer_num);
			return err;
		}
	}

	ipu_select_buffer(CSI_MEM, IPU_OUTPUT_BUFFER, *buffer_num);

	*buffer_num = (*buffer_num == 0) ? 1 : 0;

	return 0;
}

/*!
 * Enable encoder task
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int csi_enc_enabling_tasks(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;
//	printf("csi_enc_enabling_tasks\n");
	err = csi_enc_setup(cam);
	if (err != 0) {
		printf("csi_enc_setup %d\n", err);
		return err;
	}

	return err;
}

/*!
 * Disable encoder task
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  int
 */
static int csi_enc_disabling_tasks(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;

	err = ipu_disable_channel(CSI_MEM);

	ipu_uninit_channel(CSI_MEM);

	ipu_csi_enable_mclk(cam->csi, 0, 0);

	return err;
}

/*!
 * Enable csi
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int csi_enc_enable_csi(void *private)
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
static int csi_enc_disable_csi(void *private)
{
	cam_data *cam = (cam_data *) private;

	return ipu_disable_csi(cam->csi);
}

/*!
 * function to select CSI ENC as the working path
 *
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  int
 */
int csi_enc_select(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;

	if (cam) {
		cam->enc_update_eba = csi_enc_eba_update;
		cam->enc_enable = csi_enc_enabling_tasks;
		cam->enc_disable = csi_enc_disabling_tasks;
		cam->enc_enable_csi = csi_enc_enable_csi;
		cam->enc_disable_csi = csi_enc_disable_csi;
	} else {
		err = -EIO;
	}

	return err;
}

/*!
 * function to de-select CSI ENC as the working path
 *
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  int
 */
int csi_enc_deselect(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;

	if (cam) {
		cam->enc_update_eba = NULL;
		cam->enc_enable = NULL;
		cam->enc_disable = NULL;
		cam->enc_enable_csi = NULL;
		cam->enc_disable_csi = NULL;
	}

	return err;
}
