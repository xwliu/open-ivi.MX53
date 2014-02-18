/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
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

#include <common.h>
#include <asm/io.h>
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>
#include <asm/errno.h>
#include <netdev.h>

#if CONFIG_I2C_MXC
#include <i2c.h>
#endif

#ifdef CONFIG_IMX_ECSPI
#include <imx_spi.h>
#endif

#if defined(CONFIG_VIDEO_MX5)
#include <linux/list.h>
#include <ipu.h>
#include <lcd.h>
#include <linux/fb.h>
#include <linux/mxcfb.h>
#endif

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
#include <asm/imx_iim.h>
#endif

#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include "../common/recovery.h"
#include <part.h>
#include <ext2fs.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <ubi_uboot.h>
#include <jffs2/load_kernel.h>
#endif

#include <asm/arch/iomux-mx53-tractor.h>
#include <asm/arch/tractor_mx53_default_mux.h>
#include <asm/arch/tractor_mx53_default_pad.h>
#include <asm/arch/tractor_mx53_default_select_set.h>
#include <../drivers/i2c/mcu.h>

#ifdef CONFIG_KERNEL_FB
#include <watchdog.h>
#include <bmp_layout.h>
#endif


DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_VIDEO_MX5
extern unsigned char tractor_bmp_480x240_16bit[];
extern int tractor_bmp_480x240_16bit_size;

extern uint32_t i2c_port_num;
extern uint32_t i2c_port_addr[]; 

#if defined(CONFIG_BMP_8BPP)
short colormap[256];
#elif defined(CONFIG_BMP_16BPP)
short colormap[65536];
#else
short colormap[16777216];
#endif

extern int ipuv3_fb_init(struct fb_videomode *mode, int di, int interface_pix_fmt,
		  ipu_di_clk_parent_t di_clk_parent, int di_clk_val);

static struct fb_videomode HannStar6 = {
	 /* 800x480 @ 60 Hz , pixel clk @ 27MHz */
	 "HannStar6", 60, 800, 480, 32000, 88, 40, 32, 13, 48, 3,
	 FB_SYNC_CLK_LAT_FALL,
	 FB_VMODE_NONINTERLACED,
	 0,
};

vidinfo_t panel_info;
#endif


static u32 system_rev;
static enum boot_device boot_dev;

static u32 boot_reason;

u32 get_board_rev_from_fuse(void)
{
	u32 board_rev = readl(IIM_BASE_ADDR + 0x878);

	return board_rev;
}

static inline void setup_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = ONE_NAND_BOOT;
		else
			boot_dev = WEIM_NOR_BOOT;
		break;
	case 0x2:
		if (bt_mem_type)
			boot_dev = SATA_BOOT;
		else
			boot_dev = PATA_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = SPI_NOR_BOOT;
		else
			boot_dev = I2C_BOOT;
		break;
	case 0x4:
	case 0x5:
		boot_dev = SD_BOOT;
		break;
	case 0x6:
	case 0x7:
		boot_dev = MMC_BOOT;
		break;
	case 0x8 ... 0xf:
		boot_dev = NAND_BOOT;
		break;
	default:
		boot_dev = UNKNOWN_BOOT;
		break;
	}
}

enum boot_device get_boot_device(void)
{
	return boot_dev;
}

u32 get_board_rev(void)
{
	return system_rev;
}

static inline void setup_soc_rev(void)
{
	int reg;

	/* Si rev is obtained from ROM */
	reg = __REG(ROM_SI_REV);

	switch (reg) {
	case 0x10:
		system_rev = 0x53000 | CHIP_REV_1_0;
		break;
	case 0x20:
		system_rev = 0x53000 | CHIP_REV_2_0;
		break;
	case 0x21:
		system_rev = 0x53000 | CHIP_REV_2_1;
		break;
	default:
		system_rev = 0x53000 | CHIP_REV_UNKNOWN;
	}
}

static inline void setup_board_rev(int rev)
{
	system_rev |= (rev & 0xF) << 8;
}

inline int is_soc_rev(int rev)
{
	return (system_rev & 0xFF) - rev;
}

#ifdef CONFIG_ARCH_MMU
void board_mmu_init(void)
{
	unsigned long ttb_base = PHYS_SDRAM_1 + 0x4000;
	unsigned long i;

	/*
	* Set the TTB register
	*/
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb_base) /*:*/);

	/*
	* Set the Domain Access Control Register
	*/
	i = ARM_ACCESS_DACR_DEFAULT;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/*
	* First clear all TT entries - ie Set them to Faulting
	*/
	memset((void *)ttb_base, 0, ARM_FIRST_LEVEL_PAGE_TABLE_SIZE);
	/* Actual   Virtual  Size   Attributes          Function */
	/* Base     Base     MB     cached? buffered?  access permissions */
	/* xxx00000 xxx00000 */
	X_ARM_MMU_SECTION(0x000, 0x000, 0x010,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* ROM, 16M */
	X_ARM_MMU_SECTION(0x010, 0x010, 0x060,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* Reserved, 96M */
	X_ARM_MMU_SECTION(0x070, 0x070, 0x010,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IRAM, 16M */
	X_ARM_MMU_SECTION(0x080, 0x080, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* Reserved region + TZIC. 1M */
	X_ARM_MMU_SECTION(0x100, 0x100, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SATA */
	X_ARM_MMU_SECTION(0x140, 0x140, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* Reserved, 64M */
	X_ARM_MMU_SECTION(0x180, 0x180, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IPUv3M */
	X_ARM_MMU_SECTION(0x200, 0x200, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* GPU */
	X_ARM_MMU_SECTION(0x400, 0x400, 0x300,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* periperals */
	X_ARM_MMU_SECTION(0x700, 0x700, 0x200,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
	X_ARM_MMU_SECTION(0x700, 0x900, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
#if 0	
	X_ARM_MMU_SECTION(0xB00, 0xB00, 0x200,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
	X_ARM_MMU_SECTION(0xB00, 0xD00, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 512M */
	X_ARM_MMU_SECTION(0xF00, 0xF00, 0x07F,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
	X_ARM_MMU_SECTION(0xF7F, 0xF7F, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* NAND Flash buffer */
	X_ARM_MMU_SECTION(0xF80, 0xF80, 0x080,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* iRam + GPU3D + Reserved */
#endif 

	/* Workaround for arm errata #709718 */
	/* Setup PRRR so device is always mapped to non-shared */
	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r"(i) : /*:*/);
	i &= (~(3 << 0x10));
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(i) /*:*/);

	/* Enable MMU */
	MMU_ON();
}
#endif

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	return 0;
}

static void setup_uart(void)
{
	/* UART1 RXD */
	mxc_request_iomux(MX53_PIN_CSI0_D11, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX53_PIN_CSI0_D11, 0x1E4);
	mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);

	/* UART1 TXD */
	mxc_request_iomux(MX53_PIN_CSI0_D10, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX53_PIN_CSI0_D10, 0x1E4);

	/* UART3 RXD */
	mxc_request_iomux(MX53_PIN_EIM_D25, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX53_PIN_EIM_D25, 0x1E4);
	mxc_iomux_set_input(MUX_IN_UART3_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);

	/* UART3 TXD */
	mxc_request_iomux(MX53_PIN_EIM_D24, IOMUX_CONFIG_ALT2);
	mxc_iomux_set_pad(MX53_PIN_EIM_D24, 0x1E4);

}

#ifdef CONFIG_IMX_ECSPI
s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
	switch(dev->slave.bus){
	case 0:
		if(dev->slave.cs != 0){
			printf("Invalid SPI CS Config for ECSP1: %d\n", dev->slave.cs);
			return;
		}
		dev->base = CSPI1_BASE_ADDR;
		dev->freq = 500000;
		dev->ss_pol = IMX_SPI_ACTIVE_LOW;
		dev->ss = 0;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		printf("ECSPI1 CS0 Initialized for MCU\n");
		break;
	case 1:
		if(dev->slave.cs != 0){
			printf("Invalid SPI CS Config for ECSP2: %d\n", dev->slave.cs);
			return;
		}
		dev->base = CSPI2_BASE_ADDR;
		dev->freq = 1000000;
		dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
		dev->ss = 0;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		printf("ECSPI2 CS0 Initialized for MC34708\n");		
		break;
	}
	return 0;
}
// Later when we enable spi in uboot for mc34708, we should set the right according to the parameter dev
void spi_io_init(struct imx_spi_dev_t *dev)
{
	switch(dev->base){
	case CSPI1_BASE_ADDR: 
		// first pull high to disable mcu spi interface
		mx53_gpio_direction(2, 2, 1);	// config gpio3_2 as output SPI_CSS
		mx53_gpio_direction(3, 5, 0);	// config gpio4_5 as the SPI Ready pin
		break;
	case CSPI2_BASE_ADDR:
		// Nothing to do as we initialize all pins in the mxc_iomux_tractor_setup
		break;
	default:
		break;
	}
}
#endif


#ifdef CONFIG_I2C_MXC
static void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
	case I2C1_BASE_ADDR:
		/* i2c1 SDA */
		mxc_request_iomux(MX53_PIN_CSI0_D8,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_CSI0_D8, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		/* i2c1 SCL */
		mxc_request_iomux(MX53_PIN_CSI0_D9,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_CSI0_D9, PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	case I2C2_BASE_ADDR:
		/* i2c2 SDA */
		mxc_request_iomux(MX53_PIN_KEY_ROW3,
				IOMUX_CONFIG_ALT4 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_KEY_ROW3,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

		/* i2c2 SCL */
		mxc_request_iomux(MX53_PIN_KEY_COL3,
				IOMUX_CONFIG_ALT4 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH0);
		mxc_iomux_set_pad(MX53_PIN_KEY_COL3,
				PAD_CTL_SRE_FAST |
				PAD_CTL_ODE_OPENDRAIN_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);

		break;
	case I2C3_BASE_ADDR:
		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}

void setup_pmic_voltages(void)
{
}

/* DA9053 I2C SDA stuck low issue: the I2C block in DA9053 may not correctly
 * receive a Power On Reset and device is in unknown state during start-up.
 * The only way to get the chip into known state before any communication
 * with the Chip via I2C is to dummy clock the I2C and bring it in a state
 * where I2C can communicate. Dialog suggested to provide 9 clock on SCL.
 * Dialog don't know the exact reason for the fault and assume it is because
 * some random noise or spurious behaviour.
 * This has to been done in host platform specific I2C driver during
 * start-up when the I2C is being configured at platform level to supply with
 * dummy 9 clock on SCL. Dialog I2C driver has no control to provide dummy 9
 * clock on SCL.
 */
#define I2C1_SDA_GPIO5_26_BIT_MASK  (1 << 26)
#define I2C1_SCL_GPIO5_27_BIT_MASK  (1 << 27)
void i2c_failed_handle(void)
{
	unsigned int reg, i, retry = 10;

	do {
		/* set I2C1_SDA as GPIO input */
		mxc_request_iomux(MX53_PIN_CSI0_D8, IOMUX_CONFIG_ALT1);
		reg = readl(GPIO5_BASE_ADDR + 0x4);
		reg &= ~I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x4);

		/* set I2C1_SCL as GPIO output */
		mxc_request_iomux(MX53_PIN_CSI0_D9, IOMUX_CONFIG_ALT1);
		reg = readl(GPIO5_BASE_ADDR + 0x0);
		reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x0);

		reg = readl(GPIO5_BASE_ADDR + 0x4);
		reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x4);
		udelay(10000);

		for (i = 0; i < 10; i++) {
			reg = readl(GPIO5_BASE_ADDR + 0x0);
			reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
			writel(reg, GPIO5_BASE_ADDR + 0x0);
			udelay(5000);

			reg = readl(GPIO5_BASE_ADDR + 0x0);
			reg &= ~I2C1_SCL_GPIO5_27_BIT_MASK;
			writel(reg, GPIO5_BASE_ADDR + 0x0);
			udelay(5000);
		}
		reg = readl(GPIO5_BASE_ADDR + 0x0);
		reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + 0x0);
		udelay(1000);

		reg = readl(GPIO5_BASE_ADDR + 0x8);
		if (reg & I2C1_SDA_GPIO5_26_BIT_MASK) {
			printf("***I2C1_SDA = hight***\n");
			return;
		} else {
			printf("***I2C1_SDA = low***\n");
		}
	} while (retry--);
}

int i2c_read_check(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int ret = 0;

	ret = i2c_read(chip, addr, alen, buf, len);
	if (ret == 0) {
		return 0;
	} else {
	i2c_failed_handle();
//	setup_i2c(CONFIG_SYS_I2C_PORT);
	setup_i2c(i2c_port_addr[i2c_port_num]);
	ret = i2c_read(chip, addr, alen, buf, len);
	if (ret != 0) {
		printf("[I2C-DA9053]read i2c fail\n");
		return -1;
	}
	return 0;
	}
}

int i2c_write_check(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int ret = 0;

	ret = i2c_write(chip, addr, alen, buf, len);
	if (ret == 0) {
		return 0;
	} else {
		i2c_failed_handle();
//		setup_i2c(CONFIG_SYS_I2C_PORT);
		setup_i2c(i2c_port_addr[i2c_port_num]);
		i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
		ret = i2c_write(chip, addr, alen, buf, len);
		if (ret != 0) {
			printf("[I2C-DA9053]write i2c fail\n");
			return -1;
		}
		return 0;
	}
}

#endif


#ifdef CONFIG_CMD_MMC

struct fsl_esdhc_cfg esdhc_cfg[3] = {
	{MMC_SDHC2_BASE_ADDR, 1, 1},
	{MMC_SDHC3_BASE_ADDR, 1, 1},
	{MMC_SDHC1_BASE_ADDR, 1, 1},
};

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	return (soc_sbmr & 0x00300000) ? 1 : 0;
}
#endif

#ifdef CONFIG_EMMC_DDR_PORT_DETECT
int detect_mmc_emmc_ddr_port(struct fsl_esdhc_cfg *cfg)
{
	return (MMC_SDHC3_BASE_ADDR == cfg->esdhc_base) ? 1 : 0;
}
#endif

int esdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

	for (index = 0; index <= CONFIG_SYS_FSL_ESDHC_NUM;
		++index) {
		switch (index) {
		case 0:
/* //  has been set over
			mxc_request_iomux(MX53_PIN_SD1_CMD, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_CLK, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA0,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA1,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA2,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA3,
						IOMUX_CONFIG_ALT0);

			mxc_iomux_set_pad(MX53_PIN_SD1_CMD, 0x1E4);
			mxc_iomux_set_pad(MX53_PIN_SD1_CLK, 0xD4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA0, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA1, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA2, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA3, 0x1D4);
			break;
*/
		case 1:
/*			mxc_request_iomux(MX53_PIN_ATA_RESET_B,
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_IORDY,
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_DATA8,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA9,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA10,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA11,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA0,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA1,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA2,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA3,
						IOMUX_CONFIG_ALT4);

			mxc_iomux_set_pad(MX53_PIN_ATA_RESET_B, 0x1E4);
			mxc_iomux_set_pad(MX53_PIN_ATA_IORDY, 0xD4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA8, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA9, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA10, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA11, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA0, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA1, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA2, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA3, 0x1D4);
*/
		case 2:
			break;
		default:
			printf("Warning: you configured more ESDHC controller"
				"(%d) as supported by the board(2)\n",
				CONFIG_SYS_FSL_ESDHC_NUM);
			return status;
		}
		status |= fsl_esdhc_initialize(bis, &esdhc_cfg[index]);
	}

	return status;
}

int board_mmc_init(bd_t *bis)
{
	if (!esdhc_gpio_init(bis))
		return 0;
	else
		return -1;
}

#endif

void lcd_enable(void)
{
#ifdef CONFIG_LCD
	int ret;
	unsigned int reg;

	printf("lcd_enable\n");
	ret = ipuv3_fb_init(&HannStar6, 0, IPU_PIX_FMT_RGB24, DI_PCLK_PLL3, 0);	
	if (ret)
		printf("LCD cannot be configured\n");
#endif	
	// by default, when boot, we set the screen to max brightness
}

#ifdef CONFIG_VIDEO_MX5
static void panel_info_init(void)
{
	panel_info.vl_bpix = LCD_BPP;
	panel_info.vl_col = HannStar6.xres;
	panel_info.vl_row = HannStar6.yres;
	panel_info.cmap = colormap; 
}
#endif

#ifdef CONFIG_SPLASH_SCREEN
void setup_splash_image(void)
{
	char *s;
	ulong addr;
#if 0
	s = getenv("splashimage");
	if (s != NULL) {
		addr = simple_strtoul (s, NULL, 16);
		//printf("splash: 0x%08x, fb_base: 0x%08x\n", addr, gd->fb_base);
		struct mmc *mmc = find_mmc_device(1);
		if (!mmc) {
			printf("mmc device not found\n\r");
			return 1;
		}
		mmc_init(mmc);
		
		addr = ioremap_nocache(iomem_to_phys(addr), 512);
		mmc->block_dev.block_read(1, 0x5000, 1, (int *)addr);
		bmp_image_t *bmp_info=(bmp_image_t *)(addr);
		int file_size = bmp_info->header.file_size;
		
#if defined(CONFIG_ARCH_MMU)
		//addr = ioremap_nocache(iomem_to_phys(addr), tractor_bmp_480x240_16bit_size);	
		addr = ioremap_nocache(iomem_to_phys(addr), file_size);
#endif
#if 1
		//dev:1	store_addr:0x5000 * 1024 * 512  data_size: 0x1000 * 512  run_addr: &addr
		mmc->block_dev.block_read(1, 0x5000, (file_size/512) + 1, (int *)addr);
#else
		memcpy((char *)addr, (char *)tractor_bmp_480x240_16bit, tractor_bmp_480x240_16bit_size);
#endif
	}
#else
	lcd_base = (void *)(gd->fb_base);
	addr = ioremap_nocache(iomem_to_phys(lcd_base), 800 * 480 * 6);
	struct mmc *mmc = find_mmc_device(1);
	if (!mmc) {
		printf("mmc device not found\n\r");
		return 1;
	}
	mmc_init(mmc);
	mmc->block_dev.block_read(1, 0x5000, (800 * 480 * 2/512), (int *)addr);
#endif
}
#endif


int board_init(void)
{
#ifdef CONFIG_MFG
/* MFG firmware need reset usb to avoid host crash firstly */
#define USBCMD 0x140
	int val = readl(OTG_BASE_ADDR + USBCMD);
	val &= ~0x1; /*RS bit*/
	writel(val, OTG_BASE_ADDR + USBCMD);
#endif
	setup_boot_device();
	setup_soc_rev();

	gd->bd->bi_arch_number = MACH_TYPE_MX53_TRACTOR;	/* board id for linux */

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	mxc_iomux_v3_tractor_init(IOMUXC_BASE_ADDR);
	mxc_iomux_tractor_setup(tractor_default_mux_tab, sizeof(tractor_default_mux_tab) / sizeof(iomux_set), 
							tractor_default_pad_set, sizeof(tractor_default_pad_set)/sizeof(pad_set),
							tractor_default_select_set, sizeof(tractor_default_select_set)/sizeof(selectmux_set));
	setup_uart();

#ifdef CONFIG_I2C_MXC
	// We connect MC34708 with SPI so comment all the MC34708 I2C code here
	//setup_i2c(CONFIG_SYS_I2C_PORT);
	//setup_i2c(i2c_port_addr[i2c_port_num]);
	//i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
    	/* delay pmic i2c access to board_late_init()
       due to i2c_probe fail here on loco/ripley board. */
	/* Increase VDDGP voltage */
	/* setup_pmic_voltages(); */
	/* Switch to 1GHZ */
	/* clk_config(CONFIG_REF_CLK_FREQ, 1000, CPU_CLK); */
#endif
	//puts("lcd initialize memory\n");
#ifdef CONFIG_VIDEO_MX5
	panel_info_init();

	gd->fb_base = CONFIG_FB_BASE;
#ifdef CONFIG_ARCH_MMU
	gd->fb_base = ioremap_nocache(iomem_to_phys(gd->fb_base), 0);
#endif
#endif

	
	mcu_i2c_init();
		
	return 0;
}


#ifdef CONFIG_ANDROID_RECOVERY
struct reco_envs supported_reco_envs[BOOT_DEV_NUM] = {
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
};

int cleanup_before_recovery(void)
{
	mcu_write_reg(TRACTOR_REG_OFFSET_V14CTRL_PWN, 0);
}
#endif

/*must make the same with the defines in recovery.h*/
#define UPDATE_RECOVERY		4
#define CLEAN_MODE			3
#define BACKUP_MODE			2
#define UPDATE_SYSTEM		1



unsigned int tractor_check_key_pressing(void)
{
	unsigned int key_value_1 = mcu_read(MCU_AKEY1_ADDRESS);
	unsigned int key_value_2 = mcu_read(MCU_AKEY2_ADDRESS);
	unsigned int boot_mode = boot_reason;
	/*menu: update*/	
	if ((0x02 == key_value_1) || (0x06 == key_value_1)){
		return UPDATE_RECOVERY;
	} 
	/*dvd out key : clean boot*/
	if(0x01 == key_value_2) {
		return CLEAN_MODE;
	}
	/*navi and dvd, backup*/
	if (0x03 == key_value_1){
		if (0x05 == key_value_2) {
			return BACKUP_MODE;
		}
	} 
	if(boot_mode & MCU_BOOT_MODE_CLEAN_BOOT) {
		return CLEAN_MODE;
	} else if(boot_mode & MCU_BOOT_MODE_RECOVERY) {
		return UPDATE_RECOVERY;
	} else if(boot_mode & MCU_BOOT_MODE_UPDATE) {
		return UPDATE_SYSTEM;
	}
	return 0;
}

unsigned int tractor_check_reverse_start(void)
{
	unsigned int key_value = 0;
	key_value = mcu_read(MCU_VEHICLES_STATUS);
	if (0x02 & key_value) {
		return 1;
	} else {
		return 0;
	}
}

#ifdef CONFIG_I2C_MXC
/* Note: udelay() is not accurate for i2c timing */
static void __udelay(int time)
{
	int i, j;

	for (i = 0; i < time; i++) {
		for (j = 0; j < 200; j++) {
			asm("nop");
			asm("nop");
		}
	}
}
#define I2C1_SDA_GPIO5_26_BIT_MASK  (1 << 26)
#define I2C1_SCL_GPIO5_27_BIT_MASK  (1 << 27)
#define I2C2_SCL_GPIO4_12_BIT_MASK  (1 << 12)
#define I2C2_SDA_GPIO4_13_BIT_MASK  (1 << 13)
#define I2C3_SCL_GPIO1_3_BIT_MASK   (1 << 3)
#define I2C3_SDA_GPIO1_6_BIT_MASK   (1 << 6)
static void mx53_i2c_gpio_scl_direction(int bus, int output)
{
	u32 reg;

	switch (bus) {
	case 1:
		mxc_request_iomux(MX53_PIN_CSI0_D9, IOMUX_CONFIG_ALT1);
		reg = readl(GPIO5_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		else
			reg &= ~I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_GDIR);
		break;
	case 2:
		mxc_request_iomux(MX53_PIN_KEY_COL3, IOMUX_CONFIG_ALT1);
		reg = readl(GPIO4_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C2_SCL_GPIO4_12_BIT_MASK;
		else
			reg &= ~I2C2_SCL_GPIO4_12_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_GDIR);
		break;
	case 3:
		mxc_request_iomux(MX53_PIN_GPIO_3, IOMUX_CONFIG_ALT1);
		reg = readl(GPIO1_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C3_SCL_GPIO1_3_BIT_MASK;
		else
			reg &= I2C3_SCL_GPIO1_3_BIT_MASK;
		writel(reg, GPIO1_BASE_ADDR + GPIO_GDIR);
		break;
	}
}

/* set 1 to output, sent 0 to input */
static void mx53_i2c_gpio_sda_direction(int bus, int output)
{
	u32 reg;

	switch (bus) {
	case 1:
		mxc_request_iomux(MX53_PIN_CSI0_D8, IOMUX_CONFIG_ALT1);

		reg = readl(GPIO5_BASE_ADDR + GPIO_GDIR);
		if (output) {
			mxc_iomux_set_pad(MX53_PIN_CSI0_D8,
					  PAD_CTL_ODE_OPENDRAIN_ENABLE |
					  PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU);
			reg |= I2C1_SDA_GPIO5_26_BIT_MASK;
		} else
			reg &= ~I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_GDIR);
		break;
	case 2:
		mxc_request_iomux(MX53_PIN_KEY_ROW3, IOMUX_CONFIG_ALT1);

		mxc_iomux_set_pad(MX53_PIN_KEY_ROW3,
				  PAD_CTL_SRE_FAST |
				  PAD_CTL_ODE_OPENDRAIN_ENABLE |
				  PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				  PAD_CTL_HYS_ENABLE);

		reg = readl(GPIO4_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C2_SDA_GPIO4_13_BIT_MASK;
		else
			reg &= ~I2C2_SDA_GPIO4_13_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_GDIR);
	case 3:
		mxc_request_iomux(MX53_PIN_GPIO_6, IOMUX_CONFIG_ALT1);
		mxc_iomux_set_pad(MX53_PIN_GPIO_6,
				  PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				  PAD_CTL_DRV_HIGH | PAD_CTL_360K_PD |
				  PAD_CTL_HYS_ENABLE);
		reg = readl(GPIO1_BASE_ADDR + GPIO_GDIR);
		if (output)
			reg |= I2C3_SDA_GPIO1_6_BIT_MASK;
		else
			reg &= ~I2C3_SDA_GPIO1_6_BIT_MASK;
		writel(reg, GPIO1_BASE_ADDR + GPIO_GDIR);
	default:
		break;
	}
}

/* set 1 to high 0 to low */
static void mx53_i2c_gpio_scl_set_level(int bus, int high)
{
	u32 reg;
	switch (bus) {
	case 1:
		reg = readl(GPIO5_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C1_SCL_GPIO5_27_BIT_MASK;
		else
			reg &= ~I2C1_SCL_GPIO5_27_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_DR);
		break;
	case 2:
		reg = readl(GPIO4_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C2_SCL_GPIO4_12_BIT_MASK;
		else
			reg &= ~I2C2_SCL_GPIO4_12_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_DR);
		break;
	case 3:
		reg = readl(GPIO1_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C3_SCL_GPIO1_3_BIT_MASK;
		else
			reg &= ~I2C3_SCL_GPIO1_3_BIT_MASK;
		writel(reg, GPIO1_BASE_ADDR + GPIO_DR);
		break;
	}
}

/* set 1 to high 0 to low */
static void mx53_i2c_gpio_sda_set_level(int bus, int high)
{
	u32 reg;

	switch (bus) {
	case 1:
		reg = readl(GPIO5_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C1_SDA_GPIO5_26_BIT_MASK;
		else
			reg &= ~I2C1_SDA_GPIO5_26_BIT_MASK;
		writel(reg, GPIO5_BASE_ADDR + GPIO_DR);
		break;
	case 2:
		reg = readl(GPIO4_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C2_SDA_GPIO4_13_BIT_MASK;
		else
			reg &= ~I2C2_SDA_GPIO4_13_BIT_MASK;
		writel(reg, GPIO4_BASE_ADDR + GPIO_DR);
		break;
	case 3:
		reg = readl(GPIO1_BASE_ADDR + GPIO_DR);
		if (high)
			reg |= I2C3_SDA_GPIO1_6_BIT_MASK;
		else
			reg &= ~I2C3_SDA_GPIO1_6_BIT_MASK;
		writel(reg, GPIO1_BASE_ADDR + GPIO_DR);
		break;
	}
}

static int mx53_i2c_gpio_check_sda(int bus)
{
	u32 reg;
	int result = 0;

	switch (bus) {
	case 1:
		reg = readl(GPIO5_BASE_ADDR + GPIO_PSR);
		result = !!(reg & I2C1_SDA_GPIO5_26_BIT_MASK);
		break;
	case 2:
		reg = readl(GPIO4_BASE_ADDR + GPIO_PSR);
		result = !!(reg & I2C2_SDA_GPIO4_13_BIT_MASK);
		break;
	case 3:
		reg = readl(GPIO1_BASE_ADDR + GPIO_PSR);
		result = !!(reg & I2C3_SDA_GPIO1_6_BIT_MASK);
		break;
	}

	return result;
}


 /* Random reboot cause i2c SDA low issue:
  * the i2c bus busy because some device pull down the I2C SDA
  * line. This happens when Host is reading some byte from slave, and
  * then host is reset/reboot. Since in this case, device is
  * controlling i2c SDA line, the only thing host can do this give the
  * clock on SCL and sending NAK, and STOP to finish this
  * transaction.
  *
  * How to fix this issue:
  * detect if the SDA was low on bus send 8 dummy clock, and 1
  * clock + NAK, and STOP to finish i2c transaction the pending
  * transfer.
  */
int i2c_bus_recovery(void)
{
	int i, bus, result = 0;

	for (bus = 1; bus <= 3; bus++) {
		mx53_i2c_gpio_sda_direction(bus, 0);

		if (mx53_i2c_gpio_check_sda(bus) == 0) {
			printf("i2c: I2C%d SDA is low, start i2c recovery...\n", bus);
			mx53_i2c_gpio_scl_direction(bus, 1);
			mx53_i2c_gpio_scl_set_level(bus, 1);
			__udelay(10000);

			for (i = 0; i < 9; i++) {
				mx53_i2c_gpio_scl_set_level(bus, 1);
				__udelay(5);
				mx53_i2c_gpio_scl_set_level(bus, 0);
				__udelay(5);
			}

			/* 9th clock here, the slave should already
			   release the SDA, we can set SDA as high to
			   a NAK.*/
			mx53_i2c_gpio_sda_direction(bus, 1);
			mx53_i2c_gpio_sda_set_level(bus, 1);
			__udelay(1); /* Pull up SDA first */
			mx53_i2c_gpio_scl_set_level(bus, 1);
			__udelay(5); /* plus pervious 1 us */
			mx53_i2c_gpio_scl_set_level(bus, 0);
			__udelay(5);
			mx53_i2c_gpio_sda_set_level(bus, 0);
			__udelay(5);
			mx53_i2c_gpio_scl_set_level(bus, 1);
			__udelay(5);
			/* Here: SCL is high, and SDA from low to high, it's a
			 * stop condition */
			mx53_i2c_gpio_sda_set_level(bus, 1);
			__udelay(5);

			mx53_i2c_gpio_sda_direction(bus, 0);
			if (mx53_i2c_gpio_check_sda(bus) == 1)
				printf("I2C%d Recovery success\n", bus);
			else {
				printf("I2C%d Recovery failed, I2C1 SDA still low!!!\n", bus);
				result |= 1 << bus;
			}
		}
		/* configure back to i2c */
		switch (bus) {
		case 1:
			setup_i2c(I2C1_BASE_ADDR);
			break;
		case 2:
			setup_i2c(I2C2_BASE_ADDR);
			break;
		case 3:
			setup_i2c(I2C3_BASE_ADDR);
			break;
		}
	}

	return result;
}
#endif

unsigned int len_config;
char buf_config[128];
void ext2load_USBconfig_file(void)
{
	char buf[128];
	char addr[32], filename[32];
	char slot_no[32];
	char limitsize[32]; 
	char *argv[6] = {"ext2load", "mmc", slot_no, addr, filename, limitsize};
	char *str_filesize;
	uint size;	
	sprintf(slot_no,"%d:2", 1); //frist parition on source disk
	sprintf(addr, "0x%x", &(buf[0]));
	sprintf(filename, "%s", "/config/config");
	sprintf(limitsize, "0x%x", 128);
	if(0 == do_ext2load(NULL, 0, 6, argv)){
		if(len_config >= 1) {
			buf[9] = '\0';
			sprintf(buf_config, " %s", buf);
		}
		printf("config file find\n");
		return 1;
	}else{
		return 0;
	}

}

/* restore VUSB 2V5 active after suspend */
#define BUCKPERI_RESTORE_SW_STEP   (0x55)
/* restore VUSB 2V5 power supply after suspend */
#define SUPPLY_RESTORE_VPERISW_EN  (0x20)

#ifdef CONFIG_KERNEL_FB
int bmp_data_buf[2 * 1024 * 1024];
#endif
int board_late_init(void)
{
	uchar value;
	unsigned char buf[4] = { 0 };
	int retries = 10, ret = -1;
	int try = 0;
	// by default, when boot, we set the screen to max brightness
	// the lcd is first draw in the stdio_init->drv_lcd_init->lcd_init->lcd_clean->lcd_logo
	// we could enable the baclight there for the earlyest time, but for the code arch define it here now....
	printf("board_late_init\n");
#ifdef CONFIG_OPEN_REVERSE_FUNCTION	
	if (tractor_check_reverse_start()) {
		tw8832_reset();
		mxc_camera_init(0);
		//for normal mode, after reverse is ending, it start to bringup kernel
		//for reverse mode, the mcu will handle the event, so nothing to do
		//enable backlight here
		mcu_write_reg(MCU_PWM_BKL_ADDRESS, 0xFE);
		unsigned int boot_mode = 0;
		boot_mode = mcu_read(MCU_BOOT_MODE);
		if (boot_mode == MCU_BOOT_MODE_VALUE_NORMAL) {
			 while(mcu_read(MCU_VEHICLES_STATUS) & 0x02);
		} else if(boot_mode == MCU_BOOT_MODE_VALUE_REVE) {
			while(1);
		}
	}
#endif

	//mxc_request_iomux(MX53_PIN_NANDF_CS1, IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
	//mxc_request_iomux(MX53_PIN_GPIO_2, IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
	//mxc_request_iomux(MX53_PIN_FEC_MDC, IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
	//mxc_request_iomux(MX53_PIN_FEC_RX_ER, IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
	//mxc_request_iomux(MX53_PIN_FEC_RXD0, IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
	//mxc_request_iomux(MX53_PIN_FEC_TXD1, IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);

	//mx53_gpio_direction(5,14,0);
	//mx53_gpio_direction(0,2,0);
	//mx53_gpio_direction(0,31,0);
	//mx53_gpio_direction(0,24,0);
	//mx53_gpio_direction(0,27,0);
	//mx53_gpio_direction(0,29,0);

	// We connect MC34708 with SPI so comment all the MC34708 I2C code here
	
#ifdef CONFIG_KERNEL_FB
	//printf("kernel logo start\n\r");
	struct mmc *mmc = find_mmc_device(1);
	if (!mmc) {
		printf("mmc device not found\n\r");
		return 1;
	}
	mmc_init(mmc);
#if 0	
	int *bmp_data = ioremap_nocache(&(bmp_data_buf[0]), sizeof(bmp_data_buf));
	mmc->block_dev.block_read(1, 0x6800, 1, (int *)bmp_data);
	bmp_image_t *bmp_info=(bmp_image_t *)(bmp_data);
	int file_size = bmp_info->header.file_size;
	int offset = bmp_info->header.data_offset;
	int width = bmp_info->header.width;
	int height = bmp_info->header.height;
	
	printf("offset: 0x%x, width:%d, height:%d\n\r",offset,width, height);
	//dev:1 store_addr:0x6800 * 512	data_size: * 512  run_addr: bmp_data
	printf("1\n\r");
	mmc->block_dev.block_read(1, 0x6800, (file_size/512) + 1, bmp_data);
	printf("2\n\r");
	int *bmp_data_addr = (char *)bmp_data + offset;

	int *fb_kernel = ioremap_nocache(CONFIG_KERNEL_FB_BASE, 1536000);
	
	printf("1\n\r");
	memset(fb_kernel, 255, 1536000);	//1536000 = 800 * 480 * 4, only init the first fb
	
	printf("2\n\r");
	int fb_offset = ((((480-height)/2) + height - 1) * 3200) + (((800-width)/2) * 4);
	fb_kernel = (char *)fb_kernel + fb_offset;
	
	int i = 0;
	
	printf("1\n\r");
	for (i = 0; i < height; ++i) {
		//WATCHDOG_RESET();
		memcpy(fb_kernel, bmp_data_addr, width*4);
		bmp_data_addr += width;
		fb_kernel -=  800;
	}
	
	printf("2\n\r");
	
#endif
	char *fb_kernel = ioremap_nocache(CONFIG_KERNEL_FB_BASE, 2*1024*1024);
	mmc->block_dev.block_read(1, 0x6800, (800 * 480 * 4/512), fb_kernel);
	//printf("kernel logo end\n\r");
#endif
	//enable backlight here
	mcu_write_reg(MCU_PWM_BKL_ADDRESS, 0xFE);

#if 0
#ifdef CONFIG_I2C_MXC
	/* first recovery I2C bus in case other device in some i2c
	 * transcation */
	i2c_bus_recovery();
#endif

	if (!i2c_probe(0x8)) {
		if (i2c_read(0x8, 7, 1, &buf[0], 3)) {
			printf("%s:i2c_read:error\n", __func__);
			return -1;
		}
		printf("i2c_read id got: 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2]);
#if 0		
		if (i2c_read(0x8, 24, 1, &buf[0], 3)) {
			printf("%s:i2c_read:error\n", __func__);
			return -1;
		}
		printf("i2c_read SW1 got: 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2]);		
		/* increase VDDGP as 1.25V for 1GHZ on SW1 */
		buf[2] = 0x30;
		if (i2c_write(0x8, 24, 1, buf, 3)) {
			printf("%s:i2c_write:error\n", __func__);
			return -1;
		}
		buf[0] = 0x0; 	buf[1] = 0x0; buf[2] = 0x0;
		if (i2c_read(0x8, 24, 1, &buf[0], 3)) {
			printf("%s:i2c_read:error\n", __func__);
			return -1;
		}
		printf("i2c_read SW1 AGAIN: 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2]);		
		if (i2c_read(0x8, 25, 1, &buf[0], 3)) {
			printf("%s:i2c_read:error\n", __func__);
			return -1;
		}
		printf("i2c_read SW2 got: 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2]);				
		/* increase VCC as 1.3V on SW2 */
		buf[2] = 0x34;
		if (i2c_write(0x8, 25, 1, buf, 3)) {
			printf("%s:i2c_write:error\n", __func__);
			return -1;
		}
		buf[0] = 0x0; 	buf[1] = 0x0; buf[2] = 0x0;
		if (i2c_read(0x8, 25, 1, &buf[0], 3)) {
			printf("%s:i2c_read:error\n", __func__);
			return -1;
		}
		printf("i2c_read SW2 AGAIN: 0x%x, 0x%x, 0x%x\n", buf[0], buf[1], buf[2]);				

		/*change global reset time as 4s*/
		if (i2c_read(0x8, 15, 1, &buf[0], 3)) {
			printf("%s:i2c_read:error\n", __func__);
			return -1;
		}
		buf[1] |= 0x1;
		buf[1] &= ~0x2;
		if (i2c_write(0x8, 15, 1, buf, 3)) {
			printf("%s:i2c_write:error\n", __func__);
			return -1;
		}

		/* set up rev #1 for loco/ripley board */
		setup_board_rev(get_board_rev_from_fuse());
		/* Switch to 1GHZ */
		clk_config(CONFIG_REF_CLK_FREQ, 1000, CPU_CLK);
#endif		
	} 
	else{
		printf("i2c detected mc34708 failed\n");
	}
#endif	
	ext2load_USBconfig_file();
	

	return 0;
}

int checkboard(void)
{
#if 0   // disable for boot more fast...
	printf("Board: ");
	printf("MX53-TRACTOR 1.0 ");
	switch (get_board_rev_from_fuse()) {
	case 0x3:
		printf("Rev. B\n");
		break;
	case 0x1:
	default:
		printf("Rev. A\n");
		break;
	}
	printf("Boot Reason: [");
	switch (__REG(SRC_BASE_ADDR + 0x8)) {
	case 0x0001:
		printf("POR");
		break;
	case 0x0009:
		printf("RST");
		break;
	case 0x0010:
	case 0x0011:
		printf("WDOG");
		break;
	default:
		printf("unknown");
	}
	printf("]\n");

	printf("Boot Device: ");
	switch (get_boot_device()) {
	case WEIM_NOR_BOOT:
		printf("NOR\n");
		break;
	case ONE_NAND_BOOT:
		printf("ONE NAND\n");
		break;
	case PATA_BOOT:
		printf("PATA\n");
		break;
	case SATA_BOOT:
		printf("SATA\n");
		break;
	case I2C_BOOT:
		printf("I2C\n");
		break;
	case SPI_NOR_BOOT:
		printf("SPI NOR\n");
		break;
	case SD_BOOT:
		printf("SD\n");
		break;
	case MMC_BOOT:
		printf("MMC\n");
		break;
	case NAND_BOOT:
		printf("NAND\n");
		break;
	case UNKNOWN_BOOT:
	default:
		printf("UNKNOWN\n");
		break;
	}
#endif
	{
		int pin_flip = 0;
		int MPU_BOOT_CONFIRM_FLIP = 6;	// the mcu will need two time flip to enable the 5V device power, for safe, we will send 6 times, mcu will auto ignore the reset...
		mxc_request_iomux(MX53_PIN_KEY_ROW2, IOMUX_CONFIG_ALT1 | IOMUX_CONFIG_SION);
		mx53_gpio_direction(3, 11, 1);
		for(pin_flip = 0; pin_flip < MPU_BOOT_CONFIRM_FLIP; pin_flip++){
			mx53_gpio_set(3, 11, 0);
			__udelay(1000);
			mx53_gpio_set(3, 11, 1);
			__udelay(1000);
		}
		mx53_gpio_direction(3, 11, 0);	// reset to the input
		printf("MPU BOOT PIN FLIP SEND\n");
	}

#if CONFIG_SYS_I2C_MCU
	{
		int MAX_MCU_ID_RETRY = 10;
		int mcu_try = 0;		
		mcu_write_reg(MCU_5V_POWER_ADDRESS, 0x1);	// 5V_DEVICE power must be enable first...Only availabe after MCU V29
		// insure I2C is ready...
		do{
			s32 hVerID = mcu_read(MCU_ID_ADDRESS);
			if(hVerID != 0xFF){ 		
				break;
			}else{
				printf("##MCU I2C ID ERR##");
			}
			mcu_try++;
		}while(mcu_try < MAX_MCU_ID_RETRY);	
		printf("MCU Version: 0x%x : 0x%x\n", ((mcu_read(MCU_VER_ADDRESS_HI) << 8) | (mcu_read(MCU_VER_ADDRESS_LO))), mcu_read(MCU_ID_ADDRESS));
		mcu_write_reg(MCU_BT_POWER_ADDRESS, 0x1);	// bt power on					
		mcu_write_reg(MCU_MPEG_RESET_ADDRESS, 0x0); // mpeg reset off		
		mcu_write_reg(MCU_MPEG_POWER_ADDRESS, 0x1); // mpeg power on			
	}
#endif

	boot_reason = mcu_read(MCU_BOOT_MODE);
	printf("System Boot Reason: %d\n", boot_reason);
	return 0;
}
