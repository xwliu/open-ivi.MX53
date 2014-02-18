/*
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <spi.h>
#include <asm/errno.h>
#include <linux/types.h>
#include <asm/io.h>
#include <malloc.h>

#include <imx_spi.h>

extern s32 spi_get_cfg(struct imx_spi_dev_t *dev);

static inline struct imx_spi_dev_t *to_imx_spi_slave(struct spi_slave *slave)
{
	return container_of(slave, struct imx_spi_dev_t, slave);
}

static s32 spi_reset(struct spi_slave *slave)
{
	u32 clk_src = mxc_get_clock(MXC_CSPI_CLK);
	s32 pre_div = 0, post_div = 0, i, reg_ctrl, reg_config;
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
	struct spi_reg_t *reg = &(dev->reg);

	if (dev->freq == 0) {
		printf("Error: desired clock is 0\n");
		return 1;
	}

	reg_ctrl = readl(dev->base + SPI_CON_REG);
	/* Reset spi */
	writel(0, dev->base + SPI_CON_REG);
	writel((reg_ctrl | 0x1), dev->base + SPI_CON_REG);

	/* Control register setup */
	if (clk_src > dev->freq) {
		pre_div = clk_src / dev->freq;
		if (pre_div > 16) {
			post_div = pre_div / 16;
			pre_div = 15;
		}
		if (post_div != 0) {
			for (i = 0; i < 16; i++) {
				if ((1 << i) >= post_div)
					break;
			}
			if (i == 16) {
				printf("Error: no divider can meet the freq: %d\n",
					dev->freq);
				return -1;
			}
			post_div = i;
		}
	}
	//printf("pre_div = %d, post_div=%d, dev->base: 0x%x\n", pre_div, post_div, dev->base);
	reg_ctrl = (reg_ctrl & ~(3 << 18)) | dev->ss << 18;
	reg_ctrl = (reg_ctrl & ~(0xF << 12)) | pre_div << 12;
	reg_ctrl = (reg_ctrl & ~(0xF << 8)) | post_div << 8;
	reg_ctrl |= 1 << (dev->ss + 4);	/* always set to master mode !!!! */
	if(slave->bus == 0)
		reg_ctrl |= (0x7 << 20);	// for mcu transfer we are using 8 bit mode
	else if(slave->bus == 1)
		reg_ctrl |= (0x1F << 20);	// for mc34708 we are using 32 bit mode

	reg_config = readl(dev->base + SPI_CFG_REG);
	/* configuration register setup */
	reg_config = (reg_config & ~(1 << ((dev->ss + 12)))) |
		(dev->ss_pol << (dev->ss + 12));
	reg_config = (reg_config & ~(1 << ((dev->ss + 20)))) |
		(dev->in_sctl << (dev->ss + 20));
	reg_config = (reg_config & ~(1 << ((dev->ss + 16)))) |
		(dev->in_dctl << (dev->ss + 16));
	reg_config = (reg_config & ~(1 << ((dev->ss + 8)))) |
		(dev->ssctl << (dev->ss + 8));
	reg_config = (reg_config & ~(1 << ((dev->ss + 4)))) |
		(dev->sclkpol << (dev->ss + 4));
	reg_config = (reg_config & ~(1 << ((dev->ss + 0)))) |
		(dev->sclkpha << (dev->ss + 0));

	reg_ctrl |= 0x1;
	writel(reg_ctrl, dev->base + SPI_CON_REG);
	writel(reg_config, dev->base + SPI_CFG_REG);
	
	/* clear interrupt reg */
	writel(0, dev->base + SPI_INT_REG);
	writel(3 << 6, dev->base + SPI_STAT_REG);

	//reg_ctrl |= 0x1;
	//writel(reg_ctrl, dev->base + SPI_CON_REG);

	/* save config register and control register */
	reg->cfg_reg  = reg_config;
	reg->ctrl_reg = reg_ctrl;
	printf("reg_ctrl = 0x%x\n", readl(dev->base + SPI_CON_REG));
	printf("reg_config = 0x%x\n", readl(dev->base + SPI_CFG_REG));	
	
	return 0;
}

void spi_init(void) {}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode)
{
	struct imx_spi_dev_t *imx_spi_slave = NULL;

	if (!spi_cs_is_valid(bus, cs))
		return NULL;

	imx_spi_slave = (struct imx_spi_dev_t *)malloc(sizeof(struct imx_spi_dev_t));
	if (!imx_spi_slave)
		return NULL;

	imx_spi_slave->slave.bus = bus;
	imx_spi_slave->slave.cs = cs;
	if(mode == SPI_MODE_1){
		imx_spi_slave->sclkpha = 1;	
	}else if(mode == SPI_MODE_2){
		imx_spi_slave->sclkpol = 1;
		imx_spi_slave->in_sctl = 1;
		imx_spi_slave->ssctl = 1;
	}else if(mode == SPI_MODE_3){
		imx_spi_slave->sclkpha = 1;	
		imx_spi_slave->sclkpol = 1;
	}

	spi_get_cfg(imx_spi_slave);

	spi_io_init(imx_spi_slave);

	spi_reset(&(imx_spi_slave->slave));

	return &(imx_spi_slave->slave);
}

void spi_free_slave(struct spi_slave *slave)
{
	struct imx_spi_dev_t *imx_spi_slave;

	if (slave) {
		imx_spi_slave = to_imx_spi_slave(slave);
		free(imx_spi_slave);
	}
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{

}

/*
 * SPI transfer:
 */
int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
	s32 val = SPI_RETRY_TIMES;
	u32 *p_buf;
	u32 reg;
	s32 len = 0,
		ret_val = 0;
	s32 burst_bytes = bitlen >> 3;
	s32 tmp = 0;
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
	struct spi_reg_t *spi_reg = &(dev->reg);
	s32 reg_test = 0;
	if (!slave)
		return -1;

	if (burst_bytes > (MAX_SPI_BYTES)) {
		printf("Error: maximum burst size is 0x%x bytes, asking 0x%x\n",
				MAX_SPI_BYTES, burst_bytes);
		return -1;
	}

	if (flags & SPI_XFER_BEGIN) {

		if (spi_reg->ctrl_reg == 0) {
			printf("Error: spi(base=0x%x) has not been initialized yet\n",
					dev->base);
			return -1;
		}
		// Following initialize action will be adjust move to spi_reset function
		//spi_reg->ctrl_reg = (spi_reg->ctrl_reg & ~0xFFF00000) | ((burst_bytes * 8 - 1) << 20);
		//writel(spi_reg->ctrl_reg | 0x1, dev->base + SPI_CON_REG);
		//writel(spi_reg->cfg_reg, dev->base + SPI_CFG_REG);
		
		/*
		printf("ctrl_reg=0x%x, cfg_reg=0x%x, rx_data: 0x%x, int: 0x%x, dma: 0x%x, stat: 0x%x, period: 0x%x, TEST: 0x%x\n",
					 readl(dev->base + SPI_CON_REG),
					 readl(dev->base + SPI_CFG_REG), 
					 readl(dev->base + SPI_RX_DATA),
					 readl(dev->base + SPI_INT_REG),
					 readl(dev->base + SPI_DMA_REG),
					 readl(dev->base + SPI_STAT_REG),					 
					 readl(dev->base + SPI_PERIOD_REG),
					 readl(dev->base + SPI_TEST_REG));
		*/
	
		/* move data to the tx fifo */
		if (dout) {
			for (p_buf = (u32 *)dout, len = burst_bytes; len > 0; p_buf++, len -= 4)
				//printf("DATA WRITTEN = 0x%x\n", *p_buf);		
				writel(*p_buf, dev->base + SPI_TX_DATA);
				//printf("DATA CONFIRM = 0x%x\n", readl(dev->base + SPI_TX_DATA));
				
		} else {
			for (len = burst_bytes; len > 0; len -= 4){
				writel(tmp, dev->base + SPI_TX_DATA);
			}
		}
		spi_cs_activate(slave);
		
		reg = readl(dev->base + SPI_CON_REG);
		reg |= (1 << 2); /* set xch bit */
		//printf("control reg = 0x%08x\n", reg);
		writel(reg, dev->base + SPI_CON_REG);

		/* poll on the TC bit (transfer complete) */
		while ((val-- > 0) &&
			(readl(dev->base + SPI_STAT_REG) & (1 << 7)) == 0) {
			udelay(100);
		}
		/*
		printf("ctrl_reg=0x%x, cfg_reg=0x%x, rx_data: 0x%x, int: 0x%x, dma: 0x%x, stat: 0x%x, period: 0x%x, TEST: 0x%x\n",
					 readl(dev->base + SPI_CON_REG),
					 readl(dev->base + SPI_CFG_REG), 
					 readl(dev->base + SPI_RX_DATA),
					 readl(dev->base + SPI_INT_REG),
					 readl(dev->base + SPI_DMA_REG),
					 readl(dev->base + SPI_STAT_REG),					 
					 readl(dev->base + SPI_PERIOD_REG),
					 readl(dev->base + SPI_TEST_REG));
		*/
		
		/* clear the TC bit */
		writel(3 << 6, dev->base + SPI_STAT_REG);
		if (val <= 0) {
			printf("Error: re-tried %d times without response. Give up\n",
					SPI_RETRY_TIMES);
			ret_val = -1;
			goto error;
		}
	}

	/* move data in the rx buf */
	if (flags & SPI_XFER_END) {
		if (din) {
			for (p_buf = (u32 *)din, len = burst_bytes; len > 0;	++p_buf, len -= 4){
				tmp = readl(dev->base + SPI_RX_DATA);
				*p_buf = tmp;
			}
		} else {
			for (len = burst_bytes; len > 0; len -= 4)
				tmp = readl(dev->base + SPI_RX_DATA);
		}

		spi_cs_deactivate(slave);
	}
	return ret_val;

error:
	spi_cs_deactivate(slave);
	return ret_val;
}

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return 1;
}


// Later, when we enable SPI in uboot for the MC34708, this two function should the right by the slave parameter...
void spi_cs_activate(struct spi_slave *slave)
{
	if(slave->bus == 0){
		//struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
		mx53_gpio_set(2, 2, 0); // make low to enable mcu spi interface
	}
}

void spi_cs_deactivate(struct spi_slave *slave)
{	
	if(slave->bus == 0){
		mx53_gpio_set(2, 2, 1); // make high to disable mcu spi interface}
	}
}

