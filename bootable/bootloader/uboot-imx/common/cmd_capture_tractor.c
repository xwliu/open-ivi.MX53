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
 * tractor capture interface test Utilities
 */

#include <common.h>
#include <command.h>
#include <ipu.h>
struct fb_videomode {
	const char *name;	/* optional */
	u32 refresh;		/* optional */
	u32 xres;
	u32 yres;
	u32 pixclock;
	u32 left_margin;
	u32 right_margin;
	u32 upper_margin;
	u32 lower_margin;
	u32 hsync_len;
	u32 vsync_len;
	u32 sync;
	u32 vmode;
	u32 flag;
};



/*-----------------------------------------------------------------------
 * Definitions
 */
 #define FB_SYNC_CLK_LAT_FALL	0x40000000

static struct fb_videomode HannStar6 = {
	 /* 800x480 @ 60 Hz , pixel clk @ 27MHz */
	 "HannStar6", 60, 800, 480, 30000, 88, 40, 32, 13, 48, 3,
	 FB_SYNC_CLK_LAT_FALL,
	 0,
	 0,
};

/*
 * Values from last command.
 */
extern void *lcd_base;

int do_colorfill24bit(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i,j = 0;
	u8* p = lcd_base;
	printf("++do colorfill start....\n");
	for(i = 0; i < 160; i++){
		for(j = 0; j < 800; j++){
			*p = 0xFF;
			p++;
			*p = 0x00;
			p++;
			*p = 0x00;
			p++;
		}
	}
	for(i = 160; i < 320; i++){
		for(j = 0; j < 800; j++){
			*p = 0x00;
			p++;
			*p = 0xFF;  
			p++;
			*p = 0x00;
			p++;
		}
	}
	for(i = 320; i < 480; i++){
		for(j = 0; j < 800; j++){
			*p = 0x00;
			p++;
			*p = 0x00;
			p++;
			*p = 0xFF;
			p++;
		}
	}
	printf("--do colorfill stop....\n");	
	return 0;
}

int do_colorfill(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i,j = 0;
	u16* p = lcd_base;
	printf("++do colorfill start....\n");
	for(i = 0; i < 160; i++){
		for(j = 0; j < 800; j++){
			*p = 0xF800;   // R
			p++;
		}
	}

	for(i = 160; i < 320; i++){
		for(j = 0; j < 800; j++){
			*p = 0x07E0;	// G
			p++;
		}
	}

	for(i = 320; i < 480; i++){
		for(j = 0; j < 800; j++){
			*p = 0x001F;	// B
			p++;
		}
	}

	printf("--do colorfill stop....\n");	
	return 0;
}

int do_colorfill2(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i,j = 0;
	u16* p = 0x97B00000;
	printf("++do colorfill start....\n");
	for(i = 0; i < 160; i++){
		for(j = 0; j < 800; j++){
			*p = 0x07E0;   // G
			p++;
		}
	}

	for(i = 160; i < 320; i++){
		for(j = 0; j < 800; j++){
			*p = 0xF800;	// R
			p++;
		}
	}

	for(i = 320; i < 480; i++){
		for(j = 0; j < 800; j++){
			*p = 0x001F;	// B
			p++;
		}
	}

	printf("--do colorfill stop....\n");	
	return 0;
}

int do_colorfill3(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i,j = 0;
	u16* p = 0x97B00000;
	printf("++do colorfill start....\n");
	for(i = 0; i < 160; i++){
		for(j = 0; j < 800; j++){
			*p = 0xFF7F;   // G
			p++;
		}
	}

	for(i = 160; i < 320; i++){
		for(j = 0; j < 800; j++){
			*p = 0xF800;	// R
			p++;
		}
	}

	for(i = 320; i < 480; i++){
		for(j = 0; j < 800; j++){
			*p = 0x001F;	// B
			p++;
		}
	}

	printf("--do colorfill stop....\n");	
	return 0;
}

int do_csi(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("++do csi start....\n");
	mxc_camera_init(0);
	printf("--do csi stop....\n");	
	return 0;
}

int do_stopcsi(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("++STOP csi start....\n");
	mxc_camera_deinit();
	printf("--STOP csi stop....\n");	
	return 0;
}


U_BOOT_CMD(
	colorfill,	1,	1,	do_colorfill,
	"tricycle screen fill command",
	"try to fill screen with red\n"
);

U_BOOT_CMD(
	colorfill2,	1,	1,	do_colorfill2,
	"tricycle screen fill command",
	"try to fill screen with red\n"
);

U_BOOT_CMD(
	colorfill3,	1,	1,	do_colorfill3,
	"tricycle screen fill command",
	"try to fill screen with red\n"
);

U_BOOT_CMD(
	colorfill24,	1,	1,	do_colorfill24bit,
	"tricycle screen fill command",
	"try to fill screen with red\n"
);

U_BOOT_CMD(
	csi,	1,	1,	do_csi,
	"tricycle screen fill command",
	"try to fill screen with red\n"
);

U_BOOT_CMD(
	scsi,	1,	1,	do_stopcsi,
	"tricycle screen fill command",
	"try to fill screen with red\n"
);

