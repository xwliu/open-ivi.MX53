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
 * @file ipu_prp_enc.c
 *
 * @brief IPU Use case for PRP-ENC
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
 * IPU ENC callback function.
 *
 * @param irq       int irq line
 * @param dev_id    void * device id
 *
 * @return status   IRQ_HANDLED for handled
 */
static  void prp_enc_callback(int irq, void *dev_id)
{
	cam_data *cam = (cam_data *) dev_id;

	if (cam->enc_callback == NULL)
		return ;

	cam->enc_callback(irq, dev_id);

	return ;
}

/*!
 * PrpENC enable channel setup function
 *
 * @param cam       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int prp_enc_setup(cam_data *cam)
{
	ipu_channel_params_t enc;
	int err = 0;
	dma_addr_t dummy = 0x97B00000;
	printf("In prp_enc_setup\n");
	if (!cam) {
		printk(KERN_ERR "cam private is NULL\n");
		return -ENXIO;
	}
	memset(&enc, 0, sizeof(ipu_channel_params_t));
	
	enc.csi_prp_enc_mem.in_width = cam->v2f.fmt.pix.width;
	enc.csi_prp_enc_mem.in_height = cam->v2f.fmt.pix.height;
	if(LCD_BPP == LCD_COLOR16){
		enc.csi_prp_enc_mem.in_pixel_fmt = IPU_PIX_FMT_RGB565;
	}else if(LCD_BPP == LCD_COLOR24){
		enc.csi_prp_enc_mem.in_pixel_fmt = IPU_PIX_FMT_RGB24;	
	}
	enc.csi_prp_enc_mem.out_width = cam->v2f.fmt.pix.width;
	enc.csi_prp_enc_mem.out_height = cam->v2f.fmt.pix.height;
	enc.csi_prp_enc_mem.csi = cam->csi;

	if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_YUV420P;
		printf("YUV420\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV422P) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_YUV422P;
		printf("YUV422P\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_YUYV;
		printf("YUYV\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_UYVY;
		printf("UYVY\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_NV12;
		printf("NV12\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_BGR24) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_BGR24;
		printf("BGR24\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_RGB24;
		printf("RGB24\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB565) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_RGB565;
		printf("RGB565\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_BGR32) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_BGR32;
		printf("BGR32\n");
	} else if (cam->v2f.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB32) {
		enc.csi_prp_enc_mem.out_pixel_fmt = IPU_PIX_FMT_RGB32;
		printf("RGB32\n");
	} else {
		printf( "format not supported\n");
		return -EINVAL;
	}

	ipu_csi_enable_mclk(cam->csi, 1, 1);
	err = ipu_init_channel(CSI_PRP_ENC_MEM, &enc);
	if (err != 0) {
		printk(KERN_ERR "ipu_init_channel %d\n", err);
		return err;
	}

	err = ipu_init_channel_buffer(CSI_PRP_ENC_MEM, IPU_OUTPUT_BUFFER,
					    enc.csi_prp_enc_mem.out_pixel_fmt,
					    enc.csi_prp_enc_mem.out_width,
					    enc.csi_prp_enc_mem.out_height,
					    cam->v2f.fmt.pix.width,
					    dummy, 0,
					    cam->offset.u_offset,
					    cam->offset.v_offset);
	if (err != 0) {
		printk(KERN_ERR "CSI_PRP_ENC_MEM output buffer\n");
		return err;
	}
	err = ipu_enable_channel(CSI_PRP_ENC_MEM);
	if (err < 0) {
		printk(KERN_ERR "ipu_enable_channel CSI_PRP_ENC_MEM\n");
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
static int prp_enc_eba_update(dma_addr_t eba, int *buffer_num)
{
	int err = 0;
	static int once = 0;

	if(once == 0)
		once++;
	else{
		printf("fake prp update eba %x\n", eba);	
		//return 0;
	}
	printf("eba %x\n", eba);
	err = ipu_update_channel_buffer(CSI_PRP_ENC_MEM,  IPU_OUTPUT_BUFFER, *buffer_num, eba);
	if (err != 0) {
		ipu_clear_buffer_ready(CSI_PRP_ENC_MEM, IPU_OUTPUT_BUFFER, *buffer_num);
		err = ipu_update_channel_buffer(CSI_PRP_ENC_MEM, IPU_OUTPUT_BUFFER, *buffer_num, 	eba);
		if (err != 0) {
			printf("ERROR: v4l2 capture: fail to update "
			       "buf%d\n", *buffer_num);
			return err;
		}
	}
	ipu_select_buffer(CSI_PRP_ENC_MEM, IPU_OUTPUT_BUFFER,  *buffer_num);
	*buffer_num = (*buffer_num == 0) ? 1 : 0;
	return 0;
}

/*!
 * Enable encoder task
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int prp_enc_enabling_tasks(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;
	printf("IPU:In prp_enc_enabling_tasks\n");
	err = prp_enc_setup(cam);
	if (err != 0) {
		printf("prp_enc_setup %d\n", err);
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
static int prp_enc_disabling_tasks(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;
	err = ipu_disable_channel(CSI_PRP_ENC_MEM);

	ipu_uninit_channel(CSI_PRP_ENC_MEM);

	ipu_csi_enable_mclk(cam->csi, 0, 0);

	return err;
}

/*!
 * Enable csi
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  status
 */
static int prp_enc_enable_csi(void *private)
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
static int prp_enc_disable_csi(void *private)
{
	cam_data *cam = (cam_data *) private;

	return ipu_disable_csi(cam->csi);
}

/*!
 * function to select PRP-ENC as the working path
 *
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  int
 */
int prp_enc_select(void *private)
{
	cam_data *cam = (cam_data *) private;
	int err = 0;

	if (cam) {
		cam->enc_update_eba = prp_enc_eba_update;
		cam->enc_enable = prp_enc_enabling_tasks;
		cam->enc_disable = prp_enc_disabling_tasks;
		cam->enc_enable_csi = prp_enc_enable_csi;
		cam->enc_disable_csi = prp_enc_disable_csi;
	} else {
		err = -EIO;
	}

	return err;
}

/*!
 * function to de-select PRP-ENC as the working path
 *
 * @param private       struct cam_data * mxc capture instance
 *
 * @return  int
 */
int prp_enc_deselect(void *private)
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
