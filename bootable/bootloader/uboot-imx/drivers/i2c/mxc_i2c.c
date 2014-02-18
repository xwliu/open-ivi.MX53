/*
 * i2c driver for Freescale mx31
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#if defined(CONFIG_HARD_I2C)

#define IADR	0x00
#define IFDR	0x04
#define I2CR	0x08
#define I2SR	0x0c
#define I2DR	0x10

#define I2CR_IEN	(1 << 7)
#define I2CR_IIEN	(1 << 6)
#define I2CR_MSTA	(1 << 5)
#define I2CR_MTX	(1 << 4)
#define I2CR_TX_NO_AK	(1 << 3)
#define I2CR_RSTA	(1 << 2)


#define I2SR_ICF	(1 << 7)
#define I2SR_IBB	(1 << 5)
#define I2SR_IIF	(1 << 1)
#define I2SR_RX_NO_AK	(1 << 0)

//#ifdef CONFIG_SYS_I2C_PORT
//# define I2C_BASE	CONFIG_SYS_I2C_PORT
//#else
//# error "define CONFIG_SYS_I2C_PORT(I2C base address) to use the I2C driver"
//#endif

#define I2C_MAX_TIMEOUT		100000
#define I2C_TIMEOUT_TICKET	1

#undef DEBUG

//#define  DEBUG

#ifdef DEBUG
#define DPRINTF(args...)  printf(args)
#else
#define DPRINTF(args...)
#endif

uint32_t i2c_port_num = 0;

uint32_t i2c_port_addr[] = {
	I2C1_BASE_ADDR,
	I2C2_BASE_ADDR,
	I2C3_BASE_ADDR
};

static u16 div[] = { 30, 32, 36, 42, 48, 52, 60, 72, 80, 88, 104, 128, 144,
	160, 192, 240, 288, 320, 384, 480, 576, 640, 768, 960,
	1152, 1280, 1536, 1920, 2304, 2560, 3072, 3840
};

static inline void i2c_reset(void)
{
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = 0;	/* Reset module */
	__REG16(i2c_port_addr[i2c_port_num] + I2SR) = 0;
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = I2CR_IEN;
}

void i2c_init(int speed, int unused)
{
	int freq;
	int i;

#ifdef CONFIG_MX31
	freq = mx31_get_ipg_clk();
#else
	freq = mxc_get_clock(MXC_IPG_CLK);
#endif
	for (i = 0; i < 0x1f; i++)
		if (freq / div[i] <= speed)
			break;

	__REG16(i2c_port_addr[i2c_port_num] + IFDR) = i;
	i2c_reset();
}

static int wait_idle(void)
{
	int timeout = I2C_MAX_TIMEOUT;

	while ((__REG16(i2c_port_addr[i2c_port_num] + I2SR) & I2SR_IBB) && --timeout) {
		__REG16(i2c_port_addr[i2c_port_num] + I2SR) = 0;
		udelay(I2C_TIMEOUT_TICKET);
		//printf("%");
	}
	DPRINTF("%s:%x, timeout: %d\n", __func__, __REG16(i2c_port_addr[i2c_port_num] + I2SR), timeout);
	return timeout ? timeout : (!(__REG16(i2c_port_addr[i2c_port_num] + I2SR) & I2SR_IBB));
}

static int wait_busy(void)
{
	int timeout = I2C_MAX_TIMEOUT;

	while ((!(__REG16(i2c_port_addr[i2c_port_num] + I2SR) & I2SR_IBB) && (--timeout))) {
		__REG16(i2c_port_addr[i2c_port_num] + I2SR) = 0;
		udelay(I2C_TIMEOUT_TICKET);
		//printf("@");
	}
	return timeout ? timeout : (__REG16(i2c_port_addr[i2c_port_num] + I2SR) & I2SR_IBB);
}

static int wait_complete(void)
{
	int timeout = I2C_MAX_TIMEOUT;

	while ((!(__REG16(i2c_port_addr[i2c_port_num] + I2SR) & I2SR_ICF)) && (--timeout)) {
		__REG16(i2c_port_addr[i2c_port_num] + I2SR) = 0;
		udelay(I2C_TIMEOUT_TICKET);
		//printf("#");
	}
	udelay(500);

	/*
	while( (__REG16(i2c_port_addr[i2c_port_num] + I2SR) & I2SR_RX_NO_AK) && (--timeout)){
		__REG16(i2c_port_addr[i2c_port_num] + I2SR) = 0;
		udelay(I2C_TIMEOUT_TICKET);
		printf("$");
	}
	*/
	DPRINTF("%s:%x\n", __func__, __REG16(i2c_port_addr[i2c_port_num] + I2SR));
	__REG16(i2c_port_addr[i2c_port_num] + I2SR) = 0;	/* clear interrupt */

	return timeout;
}

int i2c_probe(uchar chip)
{
	int ret = 0;
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = 0;	/* Reset module */
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = I2CR_IEN;
	udelay(1000);
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = I2CR_IEN | I2CR_MSTA | I2CR_MTX;
	__REG16(i2c_port_addr[i2c_port_num] + I2DR) = (chip << 1);
	wait_complete();
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = I2CR_IEN;
	return ret;
}

int i2c_read(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int timeout = I2C_MAX_TIMEOUT;
	char i = 0;
	uchar temp = 0;
	uchar temp2 = 0;
	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n", __func__, chip, addr, alen, len);

	__REG16(i2c_port_addr[i2c_port_num] + I2SR) = 0;
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = I2CR_IEN;
	/* Wait controller to be stable */
	udelay(50);
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) |= I2CR_MSTA;
	/* Start I2C transaction */
	wait_busy();
	
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) |= I2CR_IIEN | I2CR_MTX | I2CR_TX_NO_AK;
	__REG16(i2c_port_addr[i2c_port_num] + I2DR) = chip << 1;
	wait_complete();
	__REG16(i2c_port_addr[i2c_port_num] + I2DR) = addr;		// address 0 -> version
	wait_complete();	// write finish: address and addr
	//DPRINTF("i2c_read: write addr done\n");
	udelay(500);
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = I2CR_IEN | I2CR_MSTA | I2CR_MTX | I2CR_RSTA;
	/* Restart I2C transaction */
	wait_busy();
	__REG16(i2c_port_addr[i2c_port_num] + I2DR) = (chip << 1) | 0x01;
	wait_complete();
	//DPRINTF("i2c_read: read action send\n");	
	udelay(500);
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = I2CR_IEN | I2CR_MSTA | I2CR_TX_NO_AK;
	temp = __REG16(i2c_port_addr[i2c_port_num] + I2DR);
	wait_complete();
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) &= ~(I2CR_MSTA | I2CR_MTX);
	wait_idle();
	*buf =  __REG16(i2c_port_addr[i2c_port_num] + I2DR);
	DPRINTF("i2c_read temp: 0x%x, buf: 0x%x\n", temp, *buf);
	__REG16(i2c_port_addr[i2c_port_num] + I2CR)  = 0;
	return 0;
}

int i2c_write(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	int timeout = I2C_MAX_TIMEOUT;
	char i = 0;
	//printf("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d, data: %d\n", __func__, chip, addr, alen, len, *buf);
	__REG16(i2c_port_addr[i2c_port_num] + I2SR) = 0;
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) = I2CR_IEN;
	/* Wait controller to be stable */
	udelay(50);
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) |= I2CR_MSTA;
	/* Start I2C transaction */
	wait_busy();
	
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) |= I2CR_IIEN | I2CR_MTX | I2CR_TX_NO_AK;
	__REG16(i2c_port_addr[i2c_port_num] + I2DR) = chip << 1;
	wait_complete();
	for(i = 0; i < len; i++){
		__REG16(i2c_port_addr[i2c_port_num] + I2DR) = *(buf+i);
		wait_complete();
	}	
	__REG16(i2c_port_addr[i2c_port_num] + I2CR) &= ~(I2CR_MSTA | I2CR_MTX);
	wait_idle();
	__REG16(i2c_port_addr[i2c_port_num] + I2CR)  = 0;
	return 0;
}
#endif
