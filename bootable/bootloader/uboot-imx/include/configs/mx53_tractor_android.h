/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX53-TRACTOR board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <asm/arch/mx53.h>

 /* High Level Configuration Options */
#define CONFIG_ARMV7		/* This is armv7 Cortex-A8 CPU core */
#define CONFIG_MXC
#define CONFIG_MX53
#define CONFIG_MX53_TRACTOR
#define CONFIG_FLASH_HEADER
#define CONFIG_FLASH_HEADER_OFFSET 0x400
#define CONFIG_MX53_CLK32	32768

#define CONFIG_SKIP_RELOCATE_UBOOT

#define CONFIG_ARCH_CPU_INIT
#define CONFIG_ARCH_MMU

#define CONFIG_MX53_HCLK_FREQ	24000000
#define CONFIG_SYS_PLL2_FREQ    600
#define CONFIG_SYS_AHB_PODF     4
#define CONFIG_SYS_AXIA_PODF    1
#define CONFIG_SYS_AXIB_PODF    2

//#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_SYS_CONSOLE_INFO_QUIET
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SYS_64BIT_VSPRINTF

#define BOARD_LATE_INIT
/*
 * Disabled for now due to build problems under Debian and a significant
 * increase in the final file size: 144260 vs. 109536 Bytes.
 */

#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_REVISION_TAG		1
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG		1

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 2 * 1024 * 1024)
/* size in bytes reserved for initial data */
#define CONFIG_SYS_GBL_DATA_SIZE	128

/*
 * Hardware drivers
 */
#define CONFIG_MXC_UART 1
#define CONFIG_UART_BASE_ADDR   UART1_BASE_ADDR


// Add for gpio operation
#define CONFIG_MX53_GPIO				1
/*
 * Android support Configs
 */

#define CONFIG_USB_DEVICE
#define CONFIG_FASTBOOT		1
#define CONFIG_IMX_UDC		1
#define CONFIG_FASTBOOT_STORAGE_EMMC
#define CONFIG_FASTBOOT_VENDOR_ID	0xbb4
#define CONFIG_FASTBOOT_PRODUCT_ID	0xc01
#define CONFIG_FASTBOOT_BCD_DEVICE	0x311
#define CONFIG_FASTBOOT_MANUFACTURER_STR  "TokenWireless"
#define CONFIG_FASTBOOT_PRODUCT_NAME_STR "i.mx53 Tractor"
#define CONFIG_FASTBOOT_CONFIGURATION_STR  "Android fastboot"
#define CONFIG_FASTBOOT_INTERFACE_STR    "Android fastboot"
#define CONFIG_FASTBOOT_SERIAL_NUM	 "12345"
#define CONFIG_FASTBOOT_TRANSFER_BUF	 0x80000000
#define CONFIG_FASTBOOT_TRANSFER_BUF_SIZE 0x8000000 /* 128M byte */

#define CONFIG_ANDROID_RECOVERY

#define CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC \
	"setenv bootargs ${bootargs} recovery=1 init=/init root=/dev/mmcblk0p5 rootwait rootfs=ext4"

#define CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC  \
	"run bootargs_base bootargs_android_recovery;mmc read 1 ${loadaddr} 0x800 0x1A00;bootm"

#define CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC_BACKUP \
	"setenv bootargs ${bootargs} recovery=2 init=/init root=/dev/mmcblk0p7 rootwait rootfs=ext4"
	
#define CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC_BACKUP  \
	"run bootargs_base bootargs_android_recovery;mmc read 1 ${loadaddr} 0xA000 0x1A00;bootm"

#define CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC_CLEANBOOT \
	"setenv bootargs ${bootargs} recovery=3 init=/init root=/dev/mmcblk0p5 rootwait rootfs=ext4"
	
#define CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC_CLEANBOOT  \
	"run bootargs_base bootargs_android_recovery;mmc read 1 ${loadaddr} 0x800 0x1A00;bootm"

#define CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC_RECOVERY \
	"setenv bootargs ${bootargs} recovery=4 init=/init root=/dev/mmcblk0p5 rootwait rootfs=ext4"

#define CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC_RECOVERY  \
	"run bootargs_base bootargs_android_recovery;mmc read 1 ${loadaddr} 0x800 0x1A00;bootm"
	
#define CONFIG_ANDROID_RECOVERY_CMD_FILE "/tractor/update.zip"

#define CONFIG_ANDROID_CACHE_PARTITION_MMC 6

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX		1
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}

/***********************************************************
 * Command definition
 ***********************************************************/

#include <config_cmd_default.h>

//#define CONFIG_CMD_PING
//#define CONFIG_CMD_DHCP
//#define CONFIG_CMD_MII
//#define CONFIG_CMD_NET
//#define CONFIG_NET_RETRY_COUNT  100
//#define CONFIG_NET_MULTI 1
//#define CONFIG_BOOTP_SUBNETMASK
//#define CONFIG_BOOTP_GATEWAY
//#define CONFIG_BOOTP_DNS

#define CONFIG_CMD_MMC
#define CONFIG_CMD_ENV

//#define CONFIG_CMD_IIM

#define CONFIG_CMD_CLOCK
#define CONFIG_REF_CLK_FREQ CONFIG_MX53_HCLK_FREQ

//#define CONFIG_CMD_SATA
#undef CONFIG_CMD_IMLS

#define CONFIG_BOOTDELAY	0
#define CONFIG_AUTOBOOT_KEYED 1
#define CONFIG_AUTOBOOT_PROMPT "ESC abort in %d seconds\n"
#define CONFIG_AUTOBOOT_STOP_STR "\x1b"

//#define CONFIG_PRIME	"FEC0"

#define CONFIG_LOADADDR		0x70800000	/* loadaddr env var */
#define CONFIG_RD_LOADADDR	(CONFIG_LOADADDR + 0x400000)

// IPOD FEATURE need using the debug uart so we have to disable the debug uart when ipod enable
// cause CONFIG_EXTRA_ENV_SETTINGS is a long string, it is impossible only condition define "console" 
// TODO: make the code seems more reasonable...
#ifdef CONFIG_IPOD_FEATURE
#define CONFIG_SILENT_CONSOLE   1
#define	CONFIG_EXTRA_ENV_SETTINGS					\
		"bootdelay=0\0" \
		"kernel=uImage\0"				\
		"loadaddr=0x70800000\0"					\
		"rd_loadaddr=0x70D00000\0"				\
		"splashimage=0xaf000000\0"	\ 
		"splashpos=m,m\0"	\
		"silent=true\0"	    \
		"bootargs_base=setenv bootargs console=none,115200 di0_primary calibration video=mxcdi0fb:RGB24,bpp=32 video=mxcdi1fb:YUV444,TV-PAL tve ip=off\0"		\
		"bootcmd_android_recovery=run bootargs_base bootargs_android_recovery; mmc read 1 ${loadaddr} 0x800 0x1A00;bootm\0"	\
		"bootargs_android_recovery=setenv bootargs ${bootargs}  init=/init androidboot.console=ttymxc0 root=/dev/mmcblk0p5 rootfs=ext4 show_text rootwait\0"	\
		"bootargs_android=setenv bootargs ${bootargs} init=/init androidboot.console=none \0"			\
		"bootcmd=run bootcmd_SD\0"				\
		"bootcmd_SD=run bootcmd_SD1 bootcmd_SD2\0"				\
		"bootcmd_SD1=run bootargs_base bootargs_android\0"	\
		"bootcmd_SD2=mmc read 1 ${loadaddr} 0x800 0x1A00; mmc read 1 ${rd_loadaddr} 0x3000 0x300;bootm ${loadaddr} ${rd_loadaddr}\0"
#else
#define	CONFIG_EXTRA_ENV_SETTINGS					\
		"bootdelay=0\0" \
		"kernel=uImage\0"				\
		"loadaddr=0x70800000\0"					\
		"rd_loadaddr=0x70D00000\0"				\
		"splashimage=0xaf000000\0"	\ 
		"splashpos=m,m\0"	\
		"bootargs_base=setenv bootargs console=ttymxc0,115200 di0_primary calibration video=mxcdi0fb:RGB24,bpp=32 video=mxcdi1fb:YUV444,TV-PAL tve ip=off\0"		\
		"bootcmd_android_recovery=run bootargs_base bootargs_android_recovery; mmc read 1 ${loadaddr} 0x800 0x1A00;bootm\0"	\
		"bootargs_android_recovery=setenv bootargs ${bootargs}  init=/init androidboot.console=ttymxc0 root=/dev/mmcblk0p5 rootfs=ext4 show_text rootwait\0"	\
		"bootargs_android=setenv bootargs ${bootargs} init=/init androidboot.console=ttymxc0 \0"			\
		"bootcmd=run bootcmd_SD\0"				\
		"bootcmd_SD=run bootcmd_SD1 bootcmd_SD2\0"				\
		"bootcmd_SD1=run bootargs_base bootargs_android\0"	\
		"bootcmd_SD2=mmc read 1 ${loadaddr} 0x800 0x1A00; mmc read 1 ${rd_loadaddr} 0x3000 0x300;bootm ${loadaddr} ${rd_loadaddr}\0"
#endif
		
//#define CONFIG_ARP_TIMEOUT	200UL

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_PROMPT		"Tractor U-Boot > "
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_CBSIZE		512	/* Console I/O Buffer Size */
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS	32	/* max number of command args */
#define CONFIG_SYS_BARGSIZE CONFIG_SYS_CBSIZE /* Boot Argument Buffer Size */

#define CONFIG_SYS_MEMTEST_START	0	/* memtest works on */
#define CONFIG_SYS_MEMTEST_END		0x10000

#undef	CONFIG_SYS_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_HZ				1000

#define CONFIG_CMDLINE_EDITING	1

//#define CONFIG_FEC0_IOBASE	FEC_BASE_ADDR
//#define CONFIG_FEC0_PINMUX	-1
//#define CONFIG_FEC0_PHY_ADDR	-1
//#define CONFIG_FEC0_MIIBASE	-1

//#define CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
//#define CONFIG_IIM_MAC_ADDR_OFFSET      0x24

//#define CONFIG_MXC_FEC
//#define CONFIG_MII
//#define CONFIG_MII_GASKET
//#define CONFIG_DISCOVER_PHY

/*
 * I2C Configs
 */
#define CONFIG_CMD_I2C          1
#define CONFIG_HARD_I2C        1
#define CONFIG_I2C_MXC          1
//#define CONFIG_SYS_I2C_PORT		 I2C2_BASE_ADDR
#define CONFIG_SYS_I2C_SPEED            100000
//#if (CONFIG_SYS_I2C_PORT  == I2C1_BASE_ADDR)
//#define CONFIG_SYS_I2C_MC34708	1
//#endif
//#if (CONFIG_SYS_I2C_PORT == I2C2_BASE_ADDR)
#define CONFIG_SYS_I2C_MCU	1
//#endif

/*
 * SPI Configs
 * */
#define CONFIG_CMD_SPI
#define CONFIG_IMX_ECSPI
#define IMX_CSPI_VER_2_3        1
#define MAX_SPI_BYTES		(64 * 4)


/*
 * Tractor MCU Configs
 * */
#define CONFIG_CMD_MCU		1

/*
 * MMC Configs
 */
#ifdef CONFIG_CMD_MMC
	#define CONFIG_MMC				1
	#define CONFIG_GENERIC_MMC
	#define CONFIG_IMX_MMC
	#define CONFIG_SYS_FSL_ESDHC_NUM        2
	#define CONFIG_SYS_FSL_ESDHC_ADDR       0
	#define CONFIG_SYS_MMC_ENV_DEV  0
	#define CONFIG_DOS_PARTITION	1
	#define CONFIG_CMD_FAT		1
	#define CONFIG_CMD_EXT2		1

	/* detect whether ESDHC1 or ESDHC3 is boot device */
	#define CONFIG_DYNAMIC_MMC_DEVNO

	//#define CONFIG_BOOT_PARTITION_ACCESS
	//#define CONFIG_EMMC_DDR_PORT_DETECT
	//#define CONFIG_EMMC_DDR_MODE
	/* port 1 (ESDHC3) is 8 bit */
	#define CONFIG_MMC_8BIT_PORTS	0x2
#endif

//reverse function
#define CONFIG_OPEN_REVERSE_FUNCTION
/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128 * 1024)	/* regular stack */

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1		CSD0_BASE_ADDR
#define PHYS_SDRAM_1_SIZE	(512 * 1024 * 1024)
//#define PHYS_SDRAM_2		CSD1_BASE_ADDR
//#define PHYS_SDRAM_2_SIZE	(512 * 1024 * 1024)
#define iomem_valid_addr(addr, size) \
	(addr >= PHYS_SDRAM_1 && addr <= (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE))

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CONFIG_SYS_NO_FLASH

/* Monitor at beginning of flash */
#define CONFIG_FSL_ENV_IN_MMC
/* #define CONFIG_FSL_ENV_IN_SATA */

#define CONFIG_ENV_SECT_SIZE    (128 * 1024)
#define CONFIG_ENV_SIZE         CONFIG_ENV_SECT_SIZE

#if defined(CONFIG_FSL_ENV_IN_NAND)
	#define CONFIG_ENV_IS_IN_NAND 1
	#define CONFIG_ENV_OFFSET	0x100000
#elif defined(CONFIG_FSL_ENV_IN_MMC)
	#define CONFIG_ENV_IS_IN_MMC	1
	#define CONFIG_ENV_OFFSET	(768 * 1024)
#elif defined(CONFIG_FSL_ENV_IN_SATA)
	#define CONFIG_ENV_IS_IN_SATA   1
	#define CONFIG_SATA_ENV_DEV     0
	#define CONFIG_ENV_OFFSET       (768 * 1024)
#elif defined(CONFIG_FSL_ENV_IN_SF)
	#define CONFIG_ENV_IS_IN_SPI_FLASH	1
	#define CONFIG_ENV_SPI_CS		1
	#define CONFIG_ENV_OFFSET       (768 * 1024)
#else
	#define CONFIG_ENV_IS_NOWHERE	1
#endif
#endif				/* __CONFIG_H */

// Most simple memory test
#ifndef CONFIG_CMD_MEMORY 
#define CONFIG_CMD_MEMORY			1
#endif
#ifdef CONFIG_CMD_MEMORY 
// More complicate memory test
//#define CONFIG_SYS_ALT_MEMTEST	1
#endif
// Most complicate memory test will be use following diag mode
// UBoot Diag Module
//#define CONFIG_POST				1
//#define CONFIG_SYS_POST_MEMORY	1
//#define CONFIG_CMD_DIAG			1

#define CONFIG_SPLASH_SCREEN
#ifdef CONFIG_SPLASH_SCREEN
	/*
	 * Framebuffer and LCD
	 */
	#define CONFIG_LCD
	#define CONFIG_VIDEO_MX5
	#define CONFIG_MXC_HSC
	#define CONFIG_IPU_CLKRATE	200000000
	#define CONFIG_SYS_CONSOLE_ENV_OVERWRITE
	#define CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE
	#define CONFIG_SYS_CONSOLE_IS_IN_ENV
	#define LCD_BPP		LCD_COLOR16
	#define CONFIG_CMD_BMP
	#define CONFIG_BMP_8BPP	
	#define CONFIG_BMP_16BPP
	#define CONFIG_FB_BASE	(TEXT_BASE + 0x300000)
	#define CONFIG_KERNEL_FB
#ifdef CONFIG_KERNEL_FB
	#define CONFIG_KERNEL_FB_SIZE	(5 * 1024 * 1024)
	#define CONFIG_KERNEL_FB_BASE  (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE - CONFIG_KERNEL_FB_SIZE)
#endif
	#define CONFIG_SPLASH_SCREEN_ALIGN
	//#define CONFIG_SYS_WHITE_ON_BLACK
#endif

#ifdef CONFIG_SPLASH_SCREEN
#define CONFIG_CMD_MX53_CAPTURE
#define CONFIG_VIDEO_MX5_TRACTOR_CAPTURE
#define CONFIG_CMD_MX53_TW8832
#define CONFIG_CMD_MX53_UDA1338
#endif
