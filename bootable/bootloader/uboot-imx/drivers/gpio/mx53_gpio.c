/*
 * Copyright (C) 2009
 * Guennadi Liakhovetski, DENX Software Engineering, <lg@denx.de>
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
#include <config.h>
#include <common.h>
#include <asm/errno.h>
#include <linux/types.h>
#include <asm/io.h>
#include <malloc.h>

#define GPIO_DR		0x00
#define GPIO_GDIR	0x04
#define GPIO_PSR	0x08


/* GPIO port description */
static unsigned long gpio_ports[] = {
	[0] = GPIO1_BASE_ADDR,
	[1] = GPIO2_BASE_ADDR,
	[2] = GPIO3_BASE_ADDR,
	[3] = GPIO4_BASE_ADDR,
	[4] = GPIO5_BASE_ADDR,
	[5] = GPIO6_BASE_ADDR,
	[6] = GPIO7_BASE_ADDR,
};

int mx53_gpio_direction(unsigned int group, unsigned int gpio_id, int bout)
{
	u32 l;
//	if(group  > 3)
//		return -1;
	if(gpio_id > 31){
		return -1;;
	}
	
	l = readl(gpio_ports[group] + GPIO_GDIR);
	if(bout){
		l |= 1 << gpio_id;
	}else{
		l &= ~(1 << gpio_id);
	}
	writel(l, gpio_ports[group] + GPIO_GDIR);
	return 0;
}

int mx53_gpio_direction_get(unsigned int group, unsigned int gpio_id)
{
	int value = 0;
//	if(group > 3)
//		return -1;
	if(gpio_id > 31){
		return -1;
	}
	value = readl(gpio_ports[group] + GPIO_GDIR);
	//printf("mx51_gpio_direction_get: group: %d, gpio_id: %d, actual: 0x%x\n", group, gpio_id, value);
	return (value >> gpio_id) & 1;
}

void mx53_gpio_set(unsigned int group, unsigned int gpio_id, unsigned int value)
{
	u32 reg;
	u32 l;
//	if(group > 3)
//		return ;
	if(gpio_id > 31){
		return ;
	}	
	reg = readl(gpio_ports[group] + 0);
	l = (readl(gpio_ports[group] + GPIO_DR) & (~(1 << gpio_id))) | (value << gpio_id);
	//printf("mx51_gpio_set: group: %d, gpio_id: %d, value: %d, actual: 0x%x, reg: 0x%x\n", group, gpio_id, value, l, reg);
	writel(l, gpio_ports[group] + GPIO_DR);
}

int mx53_gpio_get(unsigned int group, unsigned int gpio_id)
{	
	int value = 0;
//	if(group > 3)
//		return -1;
	if(gpio_id > 31){
		return -1;
	}
	value = readl(gpio_ports[group] + GPIO_PSR);
	//printf("mx51_gpio_get: group: %d, gpio_id: %d, actual: 0x%x\n", group, gpio_id, value);
	return (value >> gpio_id) & 1;

}
