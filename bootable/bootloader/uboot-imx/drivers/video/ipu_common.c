/*
 * Porting to u-boot:
 *
 * (C) Copyright 2010
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de
 *
 * Linux IPU driver
 *
 * (C) Copyright 2005-2011 Freescale Semiconductor, Inc.
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

/* #define DEBUG */
#include <common.h>
#include <ipu.h>
#include <linux/types.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/errno.h>
#include "ipu_regs.h"

extern struct mxc_ccm_reg *mxc_ccm;
extern u32 *ipu_cpmem_base;/* Set to the follow using IC direct channel, default non */
static ipu_channel_t using_ic_dirct_ch;



struct ipu_ch_param_word {
	uint32_t data[5];
	uint32_t res[3];
};

struct ipu_ch_param {
	struct ipu_ch_param_word word[2];
};

#define ipu_ch_param_addr(ch) (((struct ipu_ch_param *)ipu_cpmem_base) + (ch))

#define _param_word(base, w) \
	(((struct ipu_ch_param *)(base))->word[(w)].data)

#define ipu_ch_param_set_field(base, w, bit, size, v) { \
	int i = (bit) / 32; \
	int off = (bit) % 32; \
	_param_word(base, w)[i] |= (v) << off; \
	if (((bit) + (size) - 1) / 32 > i) { \
		_param_word(base, w)[i + 1] |= (v) >> (off ? (32 - off) : 0); \
	} \
}

#define ipu_ch_param_mod_field(base, w, bit, size, v) { \
	int i = (bit) / 32; \
	int off = (bit) % 32; \
	u32 mask = (1UL << size) - 1; \
	u32 temp = _param_word(base, w)[i]; \
	temp &= ~(mask << off); \
	_param_word(base, w)[i] = temp | (v) << off; \
	if (((bit) + (size) - 1) / 32 > i) { \
		temp = _param_word(base, w)[i + 1]; \
		temp &= ~(mask >> (32 - off)); \
		_param_word(base, w)[i + 1] = \
			temp | ((v) >> (off ? (32 - off) : 0)); \
	} \
}

#define ipu_ch_param_read_field(base, w, bit, size) ({ \
	u32 temp2; \
	int i = (bit) / 32; \
	int off = (bit) % 32; \
	u32 mask = (1UL << size) - 1; \
	u32 temp1 = _param_word(base, w)[i]; \
	temp1 = mask & (temp1 >> off); \
	if (((bit)+(size) - 1) / 32 > i) { \
		temp2 = _param_word(base, w)[i + 1]; \
		temp2 &= mask >> (off ? (32 - off) : 0); \
		temp1 |= temp2 << (off ? (32 - off) : 0); \
	} \
	temp1; \
})

static inline int _ipu_is_smfc_chan(uint32_t dma_chan)
{
	return ((dma_chan >= 0) && (dma_chan <= 3));
}

void clk_enable(struct clk *clk)
{
	if (clk) {
		if (clk->usecount++ == 0) {
			clk->enable(clk);
		}
	}
}

void clk_disable(struct clk *clk)
{
	if (clk) {
		if (!(--clk->usecount)) {
			if (clk->disable)
				clk->disable(clk);
		}
	}
}

int clk_get_usecount(struct clk *clk)
{
	if (clk == NULL)
		return 0;

	return clk->usecount;
}

u32 clk_get_rate(struct clk *clk)
{
	if (!clk)
		return 0;

	return clk->rate;
}

struct clk *clk_get_parent(struct clk *clk)
{
	if (!clk)
		return 0;

	return clk->parent;
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	if (clk && clk->set_rate)
		clk->set_rate(clk, rate);
	return clk->rate;
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (clk == NULL || !clk->round_rate)
		return 0;

	return clk->round_rate(clk, rate);
}

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	clk->parent = parent;
	if (clk->set_parent)
		return clk->set_parent(clk, parent);
	return 0;
}

static int clk_ipu_enable(struct clk *clk)
{
	ipu_clk_enable();
	return 0;
}

static void clk_ipu_disable(struct clk *clk)
{
}

static struct clk ipu_clk = {
	.name = "ipu_clk",
#if defined(CONFIG_IPU_CLKRATE)
	.rate = CONFIG_IPU_CLKRATE,
#endif
	.enable = clk_ipu_enable,
	.disable = clk_ipu_disable,
	.usecount = 0,
};

/* Globals */
struct clk *g_ipu_clk;
unsigned char g_ipu_clk_enabled;
struct clk *g_di_clk[2];
struct clk *g_pixel_clk[2];
struct clk *g_csi_clk[2];
unsigned char g_dc_di_assignment[10];
unsigned char g_chan_is_interlaced[52];
ipu_channel_t g_ipu_csi_channel[2];
uint32_t g_channel_init_mask;
uint32_t g_channel_enable_mask;

static int ipu_dc_use_count;
static int ipu_dp_use_count;
static int ipu_dmfc_use_count;
static int ipu_smfc_use_count;
static int ipu_di_use_count[2];
static int ipu_csi_use_count[2];
static int ipu_ic_use_count;

u32 *ipu_cpmem_base;
u32 *ipu_dc_tmpl_reg;
u32 *ipu_csi_reg[2];
u32 *ipu_smfc_reg;
u32 *ipu_ic_reg;

void _ipu_ic_init_prpvf(ipu_channel_params_t *params, int src_is_csi)
{
	uint32_t ic_conf;

	ic_conf = __raw_readl(IC_CONF);
	ic_conf &= ~IC_CONF_PRPVF_CMB;

	if (src_is_csi)
		ic_conf &= ~IC_CONF_RWS_EN;
	else
		ic_conf |= IC_CONF_RWS_EN;

	__raw_writel(ic_conf, IC_CONF);
}

void _ipu_ic_init_prpenc(ipu_channel_params_t *params, int src_is_csi)
{
	uint32_t reg, ic_conf;
	uint32_t downsizeCoeff, resizeCoeff;
	ipu_color_space_t in_fmt, out_fmt;

	ic_conf = __raw_readl(IC_CONF);
	if (src_is_csi)
		ic_conf &= ~IC_CONF_RWS_EN;
	else
		ic_conf |= IC_CONF_RWS_EN;

	__raw_writel(ic_conf, IC_CONF);
}


void _ipu_ic_enable_task(ipu_channel_t channel)
{
	uint32_t ic_conf;

	ic_conf = __raw_readl(IC_CONF);
	switch (channel) {
	case CSI_PRP_ENC_MEM:
		ic_conf |= IC_CONF_PRPENC_EN;
		break;
	case CSI_PRP_VF_MEM:
		ic_conf |= IC_CONF_PRPVF_EN;
		break;
		
	default:
		break;
	}
	__raw_writel(ic_conf, IC_CONF);
}

void _ipu_ic_disable_task(ipu_channel_t channel)
{
	uint32_t ic_conf;

	ic_conf = __raw_readl(IC_CONF);
	switch (channel) {
	case CSI_PRP_ENC_MEM:
		ic_conf &= ~IC_CONF_PRPENC_EN;
		break;
	case CSI_PRP_VF_MEM:
		ic_conf &= ~IC_CONF_PRPVF_EN;
		break;
	default:
		break;
	}
	__raw_writel(ic_conf, IC_CONF);
}

static inline int __ipu_ch_get_third_buf_cpmem_num(int ch)
{
	switch (ch) {
	case 8:
		return 64;
	case 9:
		return 65;
	case 10:
		return 66;
	case 13:
		return 67;
	case 21:
		return 68;
	case 23:
		return 69;
	case 27:
		return 70;
	case 28:
		return 71;
	default:
		return -EINVAL;
	}
	return 0;
}

int _ipu_ic_idma_init(int dma_chan, uint16_t width, uint16_t height, int burst_size)
{
	u32 ic_idmac_1, ic_idmac_2, ic_idmac_3;
	u32 temp_rot = 0;
	int need_hor_flip = 0;

	if ((burst_size != 8) && (burst_size != 16)) {
		printf("Illegal burst length for IC\n");
		return -EINVAL;
	}

	width--;
	height--;

	if (temp_rot & 0x2)	/* Need horizontal flip */
		need_hor_flip = 1;

	ic_idmac_1 = __raw_readl(IC_IDMAC_1);
	ic_idmac_2 = __raw_readl(IC_IDMAC_2);
	ic_idmac_3 = __raw_readl(IC_IDMAC_3);
	if (dma_chan == 22) {	/* PP output - CB2 */
		if (burst_size == 16)
			ic_idmac_1 |= IC_IDMAC_1_CB2_BURST_16;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_CB2_BURST_16;

		if (need_hor_flip)
			ic_idmac_1 |= IC_IDMAC_1_PP_FLIP_RS;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_PP_FLIP_RS;

		ic_idmac_2 &= ~IC_IDMAC_2_PP_HEIGHT_MASK;
		ic_idmac_2 |= height << IC_IDMAC_2_PP_HEIGHT_OFFSET;

		ic_idmac_3 &= ~IC_IDMAC_3_PP_WIDTH_MASK;
		ic_idmac_3 |= width << IC_IDMAC_3_PP_WIDTH_OFFSET;

	} else if (dma_chan == 11) {	/* PP Input - CB5 */
		if (burst_size == 16)
			ic_idmac_1 |= IC_IDMAC_1_CB5_BURST_16;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_CB5_BURST_16;
	} else if (dma_chan == 47) {	/* PP Rot input */
		ic_idmac_1 &= ~IC_IDMAC_1_PP_ROT_MASK;
		ic_idmac_1 |= temp_rot << IC_IDMAC_1_PP_ROT_OFFSET;
	}

	if (dma_chan == 12) {	/* PRP Input - CB6 */
		if (burst_size == 16)
			ic_idmac_1 |= IC_IDMAC_1_CB6_BURST_16;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_CB6_BURST_16;
	}

	if (dma_chan == 20) {	/* PRP ENC output - CB0 */
		if (burst_size == 16)
			ic_idmac_1 |= IC_IDMAC_1_CB0_BURST_16;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_CB0_BURST_16;

		if (need_hor_flip)
			ic_idmac_1 |= IC_IDMAC_1_PRPENC_FLIP_RS;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_PRPENC_FLIP_RS;

		ic_idmac_2 &= ~IC_IDMAC_2_PRPENC_HEIGHT_MASK;
		ic_idmac_2 |= height << IC_IDMAC_2_PRPENC_HEIGHT_OFFSET;

		ic_idmac_3 &= ~IC_IDMAC_3_PRPENC_WIDTH_MASK;
		ic_idmac_3 |= width << IC_IDMAC_3_PRPENC_WIDTH_OFFSET;

	} else if (dma_chan == 45) {	/* PRP ENC Rot input */
		ic_idmac_1 &= ~IC_IDMAC_1_PRPENC_ROT_MASK;
		ic_idmac_1 |= temp_rot << IC_IDMAC_1_PRPENC_ROT_OFFSET;
	}

	if (dma_chan == 21) {	/* PRP VF output - CB1 */
		if (burst_size == 16)
			ic_idmac_1 |= IC_IDMAC_1_CB1_BURST_16;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_CB1_BURST_16;

		if (need_hor_flip)
			ic_idmac_1 |= IC_IDMAC_1_PRPVF_FLIP_RS;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_PRPVF_FLIP_RS;

		ic_idmac_2 &= ~IC_IDMAC_2_PRPVF_HEIGHT_MASK;
		ic_idmac_2 |= height << IC_IDMAC_2_PRPVF_HEIGHT_OFFSET;

		ic_idmac_3 &= ~IC_IDMAC_3_PRPVF_WIDTH_MASK;
		ic_idmac_3 |= width << IC_IDMAC_3_PRPVF_WIDTH_OFFSET;

	} else if (dma_chan == 46) {	/* PRP VF Rot input */
		ic_idmac_1 &= ~IC_IDMAC_1_PRPVF_ROT_MASK;
		ic_idmac_1 |= temp_rot << IC_IDMAC_1_PRPVF_ROT_OFFSET;
	}

	if (dma_chan == 14) {	/* PRP VF graphics combining input - CB3 */
		if (burst_size == 16)
			ic_idmac_1 |= IC_IDMAC_1_CB3_BURST_16;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_CB3_BURST_16;
	} else if (dma_chan == 15) {	/* PP graphics combining input - CB4 */
		if (burst_size == 16)
			ic_idmac_1 |= IC_IDMAC_1_CB4_BURST_16;
		else
			ic_idmac_1 &= ~IC_IDMAC_1_CB4_BURST_16;
	}

	__raw_writel(ic_idmac_1, IC_IDMAC_1);
	__raw_writel(ic_idmac_2, IC_IDMAC_2);
	__raw_writel(ic_idmac_3, IC_IDMAC_3);

	printf("IC_IDMAC_1: 0x%x\n", __raw_readl(IC_IDMAC_1));
	printf("IC_IDMAC_2: 0x%x\n", __raw_readl(IC_IDMAC_2));
	printf("IC_IDMAC_3: 0x%x\n", __raw_readl(IC_IDMAC_3));

	return 0;
}

static inline void _ipu_ch_param_set_burst_size(uint32_t ch,
						uint16_t burst_pixels)
{
	int32_t sub_ch = 0;
	ipu_ch_param_mod_field(ipu_ch_param_addr(ch), 1, 78, 7, burst_pixels - 1);
	sub_ch = __ipu_ch_get_third_buf_cpmem_num(ch);
	if (sub_ch <= 0)
		return;
	ipu_ch_param_mod_field(ipu_ch_param_addr(sub_ch), 1, 78, 7, burst_pixels - 1);
};


static inline int _ipu_is_ic_chan(uint32_t dma_chan)
{
	return ((dma_chan >= 11) && (dma_chan <= 22) && (dma_chan != 17) && (dma_chan != 18));
}

/* Static functions */

static inline void ipu_ch_param_set_high_priority(uint32_t ch)
{
	ipu_ch_param_mod_field(ipu_ch_param_addr(ch), 1, 93, 2, 1);
};

static inline uint32_t channel_2_dma(ipu_channel_t ch, ipu_buffer_t type)
{
	return ((uint32_t) ch >> (6 * type)) & 0x3F;
};

/* Either DP BG or DP FG can be graphic window */
static inline int ipu_is_dp_graphic_chan(uint32_t dma_chan)
{
	return (dma_chan == 23 || dma_chan == 27);
}

static inline int ipu_is_dmfc_chan(uint32_t dma_chan)
{
	return ((dma_chan >= 23) && (dma_chan <= 29));
}


static inline void ipu_ch_param_set_buffer(uint32_t ch, int bufNum,
					    dma_addr_t phyaddr)
{
	ipu_ch_param_mod_field(ipu_ch_param_addr(ch), 1, 29 * bufNum, 29,
			       phyaddr / 8);
};

#define idma_is_valid(ch)	(ch != NO_DMA)
#define idma_mask(ch)		(idma_is_valid(ch) ? (1UL << (ch & 0x1F)) : 0)
#define idma_is_set(reg, dma)	(__raw_readl(reg(dma)) & idma_mask(dma))

static void ipu_pixel_clk_recalc(struct clk *clk)
{
	u32 div = __raw_readl(DI_BS_CLKGEN0(clk->id));
	if (div == 0)
		clk->rate = 0;
	else
		clk->rate = (clk->parent->rate * 16) / div;
}

static unsigned long ipu_pixel_clk_round_rate(struct clk *clk,
	unsigned long rate)
{
	u32 div, div1;
	u32 tmp;
	/*
	 * Calculate divider
	 * Fractional part is 4 bits,
	 * so simply multiply by 2^4 to get fractional part.
	 */
	tmp = (clk->parent->rate * 16);
	div = tmp / rate;

	if (div < 0x10)            /* Min DI disp clock divider is 1 */
		div = 0x10;
	if (div & ~0xFEF)
		div &= 0xFF8;
	else {
		div1 = div & 0xFE0;
		if ((tmp/div1 - tmp/div) < rate / 4)
			div = div1;
		else
			div &= 0xFF8;
	}
	return (clk->parent->rate * 16) / div;
}

static int ipu_pixel_clk_set_rate(struct clk *clk, unsigned long rate)
{
	u32 div = (clk->parent->rate * 16) / rate;

	__raw_writel(div, DI_BS_CLKGEN0(clk->id));

	/* Setup pixel clock timing */
	__raw_writel((div / 16) << 16, DI_BS_CLKGEN1(clk->id));

	clk->rate = (clk->parent->rate * 16) / div;
	return 0;
}

static int ipu_pixel_clk_enable(struct clk *clk)
{
	u32 disp_gen = __raw_readl(IPU_DISP_GEN);
	disp_gen |= clk->id ? DI1_COUNTER_RELEASE : DI0_COUNTER_RELEASE;
	__raw_writel(disp_gen, IPU_DISP_GEN);

	return 0;
}

static void ipu_pixel_clk_disable(struct clk *clk)
{
	u32 disp_gen = __raw_readl(IPU_DISP_GEN);
	disp_gen &= clk->id ? ~DI1_COUNTER_RELEASE : ~DI0_COUNTER_RELEASE;
	__raw_writel(disp_gen, IPU_DISP_GEN);

}

static int ipu_pixel_clk_set_parent(struct clk *clk, struct clk *parent)
{
	u32 di_gen = __raw_readl(DI_GENERAL(clk->id));

	if (parent == g_ipu_clk)
		di_gen &= ~DI_GEN_DI_CLK_EXT;
	else if (!IS_ERR(g_di_clk[clk->id]) && parent == g_di_clk[clk->id])
		di_gen |= DI_GEN_DI_CLK_EXT;
	else
		return -EINVAL;

	__raw_writel(di_gen, DI_GENERAL(clk->id));
	ipu_pixel_clk_recalc(clk);
	return 0;
}

static struct clk pixel_clk[] = {
	{
	.name = "pixel_clk",
	.id = 0,
	.recalc = ipu_pixel_clk_recalc,
	.set_rate = ipu_pixel_clk_set_rate,
	.round_rate = ipu_pixel_clk_round_rate,
	.set_parent = ipu_pixel_clk_set_parent,
	.enable = ipu_pixel_clk_enable,
	.disable = ipu_pixel_clk_disable,
	.usecount = 0,
	},
	{
	.name = "pixel_clk",
	.id = 1,
	.recalc = ipu_pixel_clk_recalc,
	.set_rate = ipu_pixel_clk_set_rate,
	.round_rate = ipu_pixel_clk_round_rate,
	.set_parent = ipu_pixel_clk_set_parent,
	.enable = ipu_pixel_clk_enable,
	.disable = ipu_pixel_clk_disable,
	.usecount = 0,
	},
};

static struct clk di_clk[] = {
	{
	.name = "ipu_di_clk",
	.id = 0,
	},
	{
	.name = "ipu_di_clk",
	.id = 1,
	},
};


static struct clk csi_clk[] = {
	{
	.name = "ipu_csi_clk",
	.id = 0,
	},
	{
	.name = "ipu_csi_clk",
	.id = 1,
	},
};

/*
 * This function resets IPU
 */
void ipu_reset(void)
{
	u32 *reg;
	u32 value;

	reg = (u32 *)SRC_BASE_ADDR;
	value = __raw_readl(reg);
	value = value | SW_IPU_RST;
	__raw_writel(value, reg);
}

/*
 * This function is called by the driver framework to initialize the IPU
 * hardware.
 *
 * @param	dev	The device structure for the IPU passed in by the
 *			driver framework.
 *
 * @return      Returns 0 on success or negative error code on error
 */
int ipu_probe(int di, ipu_di_clk_parent_t di_clk_parent, int di_clk_val)
{
	unsigned long ipu_base;

#if defined(CONFIG_MXC_HSC)
	u32 temp;
	u32 *reg_hsc_mcd = (u32 *)MIPI_HSC_BASE_ADDR;
	u32 *reg_hsc_mxt_conf = (u32 *)(MIPI_HSC_BASE_ADDR + 0x800);

	 __raw_writel(0xF00, reg_hsc_mcd);

	/* CSI mode reserved*/
	temp = __raw_readl(reg_hsc_mxt_conf);
	 __raw_writel(temp | 0x0FF, reg_hsc_mxt_conf);

	temp = __raw_readl(reg_hsc_mxt_conf);
	__raw_writel(temp | 0x10000, reg_hsc_mxt_conf);
#endif

	ipu_base = IPU_CTRL_BASE_ADDR;
	ipu_cpmem_base = (u32 *)(ipu_base + IPU_CPMEM_REG_BASE);
	ipu_dc_tmpl_reg = (u32 *)(ipu_base + IPU_DC_TMPL_REG_BASE);
	ipu_csi_reg[0] =  (u32 *)(ipu_base + IPU_CSI0_REG_BASE);
	ipu_csi_reg[1] =  (u32 *)(ipu_base + IPU_CSI1_REG_BASE);
	ipu_smfc_reg = (u32 *)(ipu_base +  IPU_SMFC_REG_BASE);
	ipu_ic_reg = (u32*)(ipu_base + IPU_IC_REG_BASE);
	g_pixel_clk[0] = &pixel_clk[0];
	g_pixel_clk[1] = &pixel_clk[1];

	g_di_clk[0] = &di_clk[0];
	g_di_clk[1] = &di_clk[1];
	g_di_clk[di]->rate = di_clk_val;

	g_ipu_clk = &ipu_clk;
	//printf("ipu_clk = %u\n", clk_get_rate(g_ipu_clk));

	ipu_reset();

	if (di_clk_parent == DI_PCLK_LDB) {
		clk_set_parent(g_pixel_clk[di], g_di_clk[di]);
	} else {
		clk_set_parent(g_pixel_clk[0], g_ipu_clk);
//		clk_set_parent(g_pixel_clk[1], g_ipu_clk);
	}

	clk_enable(g_ipu_clk);

	__raw_writel(0x807FFFFF, IPU_MEM_RST);
	while (__raw_readl(IPU_MEM_RST) & 0x80000000)
		;

	ipu_init_dc_mappings();

	__raw_writel(0, IPU_INT_CTRL(5));
	__raw_writel(0, IPU_INT_CTRL(6));
	__raw_writel(0, IPU_INT_CTRL(9));
	__raw_writel(0, IPU_INT_CTRL(10));

	/* DMFC Init */
	ipu_dmfc_init(DMFC_NORMAL, 1);

	/* Set sync refresh channels as high priority */
	__raw_writel(0x18900000L, IDMAC_CHA_PRI(0));

	/* Set MCU_T to divide MCU access window into 2 */
	__raw_writel(0x00400000L | (IPU_MCU_T_DEFAULT << 18), IPU_DISP_GEN);

	clk_disable(g_ipu_clk);

	return 0;
}

void ipu_dump_registers(void)
{
#if DEBUG
	printf("IPU_CONF = \t0x%08X\n", __raw_readl(IPU_CONF));
	printf("IC_CONF = \t0x%08X\n",	__raw_readl(IC_CONF));
	
	printf("IDMAC_CONF = \t0x%08X\n", __raw_readl(IDMAC_CONF));
	printf("IDMAC_CHA_EN1 = \t0x%08X\n",
	       __raw_readl(IDMAC_CHA_EN(0)));
	printf("IDMAC_CHA_EN2 = \t0x%08X\n",
	       __raw_readl(IDMAC_CHA_EN(32)));
	printf("IDMAC_CHA_PRI1 = \t0x%08X\n",
	       __raw_readl(IDMAC_CHA_PRI(0)));
	printf("IDMAC_CHA_PRI2 = \t0x%08X\n",
	       __raw_readl(IDMAC_CHA_PRI(32)));
	printf("IPU_CHA_DB_MODE_SEL0 = \t0x%08X\n",
	       __raw_readl(IPU_CHA_DB_MODE_SEL(0)));
	printf("IPU_CHA_DB_MODE_SEL1 = \t0x%08X\n",
	       __raw_readl(IPU_CHA_DB_MODE_SEL(32)));
	printf("DMFC_WR_CHAN = \t0x%08X\n",
	       __raw_readl(DMFC_WR_CHAN));
	printf("DMFC_WR_CHAN_DEF = \t0x%08X\n",
	       __raw_readl(DMFC_WR_CHAN_DEF));
	printf("DMFC_DP_CHAN = \t0x%08X\n",
	       __raw_readl(DMFC_DP_CHAN));
	printf("DMFC_DP_CHAN_DEF = \t0x%08X\n",
	       __raw_readl(DMFC_DP_CHAN_DEF));
	printf("DMFC_IC_CTRL = \t0x%08X\n",
	       __raw_readl(DMFC_IC_CTRL));
	printf("IPU_FS_PROC_FLOW1 = \t0x%08X\n",
	       __raw_readl(IPU_FS_PROC_FLOW1));
	printf("IPU_FS_PROC_FLOW2 = \t0x%08X\n",
	       __raw_readl(IPU_FS_PROC_FLOW2));
	printf("IPU_FS_PROC_FLOW3 = \t0x%08X\n",
	       __raw_readl(IPU_FS_PROC_FLOW3));
	printf("IPU_FS_DISP_FLOW1 = \t0x%08X\n",
	       __raw_readl(IPU_FS_DISP_FLOW1));
#endif
}

/*
 * This function is called to initialize a logical IPU channel.
 *
 * @param       channel Input parameter for the logical channel ID to init.
 *
 * @param       params  Input parameter containing union of channel
 *                      initialization parameters.
 *
 * @return      Returns 0 on success or negative error code on fail
 */
int32_t ipu_init_channel(ipu_channel_t channel, ipu_channel_params_t *params)
{
	int ret = 0;
	uint32_t ipu_conf;
	u32 reg;

	debug("init channel = %d\n", IPU_CHAN_ID(channel));

	if (g_ipu_clk_enabled == 0) {
		g_ipu_clk_enabled = 1;
		clk_enable(g_ipu_clk);
	}


	if (g_channel_init_mask & (1L << IPU_CHAN_ID(channel))) {
		printf("Warning--0: channel already initialized %d\n",
			IPU_CHAN_ID(channel));
	}

	ipu_conf = __raw_readl(IPU_CONF);

	switch (channel) {
	case CSI_MEM:
		if (params->csi_mem.csi > 1) {
			ret = -EINVAL;
			goto err;
		}
		//printf("csi_mem interlaced= %d, csi_mem mipi_en: %d\n", params->csi_mem.interlaced, params->csi_mem.mipi_en);
		if (params->csi_mem.interlaced)
			g_chan_is_interlaced[channel_2_dma(channel,
				IPU_OUTPUT_BUFFER)] = 0x1;
		else
			g_chan_is_interlaced[channel_2_dma(channel,
				IPU_OUTPUT_BUFFER)] = 0x0;

		ipu_smfc_use_count++;
		g_ipu_csi_channel[params->csi_mem.csi] = channel;

		/*SMFC setting*/
		if (params->csi_mem.mipi_en) {
			ipu_conf |= (1 << (IPU_CONF_CSI0_DATA_SOURCE_OFFSET +
				params->csi_mem.csi));
			_ipu_smfc_init(channel, params->csi_mem.mipi_id,
				params->csi_mem.csi);
		} else {
			ipu_conf &= ~(1 << (IPU_CONF_CSI0_DATA_SOURCE_OFFSET +
				params->csi_mem.csi));
			_ipu_smfc_init(channel, 0, params->csi_mem.csi);
		}

		/*CSI data (include compander) dest*/
		_ipu_csi_init(channel, params->csi_mem.csi);
		break;
	case CSI_PRP_ENC_MEM:
		if (g_channel_init_mask & (1L << IPU_CHAN_ID(channel))) {
			printf("Warning--1: channel already initialized %d\n", IPU_CHAN_ID(channel));
			/*CSI data (include compander) dest*/
			_ipu_csi_init(channel, params->csi_prp_enc_mem.csi);
		}else{
			if (params->csi_prp_enc_mem.csi > 1) {
					ret = -EINVAL;
					goto err;
			}
			using_ic_dirct_ch = CSI_PRP_ENC_MEM;

			ipu_ic_use_count++;
			g_ipu_csi_channel[params->csi_prp_enc_mem.csi] = channel;

			/*Without SMFC, CSI only support parallel data source*/
			ipu_conf &= ~(1 << (IPU_CONF_CSI0_DATA_SOURCE_OFFSET +
				params->csi_prp_enc_mem.csi));

			/*CSI0/1 feed into IC*/
			ipu_conf &= ~IPU_CONF_IC_INPUT;
			if (params->csi_prp_enc_mem.csi)
				ipu_conf |= IPU_CONF_CSI_SEL;
			else
				ipu_conf &= ~IPU_CONF_CSI_SEL;

			/*PRP skip buffer in memory, only valid when RWS_EN is true*/
			reg = __raw_readl(IPU_FS_PROC_FLOW1);
			__raw_writel(reg & ~FS_ENC_IN_VALID, IPU_FS_PROC_FLOW1);
			/*CSI data (include compander) dest*/
			_ipu_csi_init(channel, params->csi_prp_enc_mem.csi);
			_ipu_ic_init_prpenc(params, 1);
		}
		break;
	case CSI_PRP_VF_MEM:
		if (params->csi_prp_vf_mem.csi > 1) {
			ret = -EINVAL;
			goto err;
		}
		using_ic_dirct_ch = CSI_PRP_VF_MEM;

		ipu_ic_use_count++;
		g_ipu_csi_channel[params->csi_prp_vf_mem.csi] = channel;

		/*Without SMFC, CSI only support parallel data source*/
		ipu_conf &= ~(1 << (IPU_CONF_CSI0_DATA_SOURCE_OFFSET +
			params->csi_prp_vf_mem.csi));

		/*CSI0/1 feed into IC*/
		ipu_conf &= ~IPU_CONF_IC_INPUT;
		if (params->csi_prp_vf_mem.csi)
			ipu_conf |= IPU_CONF_CSI_SEL;
		else
			ipu_conf &= ~IPU_CONF_CSI_SEL;

		/*PRP skip buffer in memory, only valid when RWS_EN is true*/
		reg = __raw_readl(IPU_FS_PROC_FLOW1);
		__raw_writel(reg & ~FS_VF_IN_VALID, IPU_FS_PROC_FLOW1);

		/*CSI data (include compander) dest*/
		_ipu_csi_init(channel, params->csi_prp_vf_mem.csi);
		_ipu_ic_init_prpvf(params, 1);
		break;

	case MEM_DC_SYNC:
		if (params->mem_dc_sync.di > 1) {
			ret = -EINVAL;
			goto err;
		}

		g_dc_di_assignment[1] = params->mem_dc_sync.di;
		ipu_dc_init(1, params->mem_dc_sync.di,
			     params->mem_dc_sync.interlaced);
		ipu_di_use_count[params->mem_dc_sync.di]++;
		ipu_dc_use_count++;
		ipu_dmfc_use_count++;
		break;
	case MEM_BG_SYNC:
		if (params->mem_dp_bg_sync.di > 1) {
			ret = -EINVAL;
			goto err;
		}
		//printf("in_pixel_fmt: 0x%x, out_pixel_fmt: 0x%x\n", params->mem_dp_bg_sync.in_pixel_fmt, params->mem_dp_bg_sync.out_pixel_fmt);
		g_dc_di_assignment[5] = params->mem_dp_bg_sync.di;
		ipu_dp_init(channel, params->mem_dp_bg_sync.in_pixel_fmt,
			     params->mem_dp_bg_sync.out_pixel_fmt);
		ipu_dc_init(5, params->mem_dp_bg_sync.di,
			     params->mem_dp_bg_sync.interlaced);
		ipu_di_use_count[params->mem_dp_bg_sync.di]++;
		ipu_dc_use_count++;
		ipu_dp_use_count++;
		ipu_dmfc_use_count++;
		break;
	case MEM_FG_SYNC:
		ipu_dp_init(channel, params->mem_dp_fg_sync.in_pixel_fmt,
			     params->mem_dp_fg_sync.out_pixel_fmt);

		ipu_dc_use_count++;
		ipu_dp_use_count++;
		ipu_dmfc_use_count++;
		break;
	default:
		printf("Missing channel initialization\n");
		break;
	}

	if (g_channel_init_mask & (1L << IPU_CHAN_ID(channel))) {
		printf("Warning--2: channel already initialized %d\n", IPU_CHAN_ID(channel));

	}else{
		/* Enable IPU sub module */
		g_channel_init_mask |= 1L << IPU_CHAN_ID(channel);
		if (ipu_dc_use_count == 1)
			ipu_conf |= IPU_CONF_DC_EN;
		if (ipu_dp_use_count == 1)
			ipu_conf |= IPU_CONF_DP_EN;
		if (ipu_dmfc_use_count == 1)
			ipu_conf |= IPU_CONF_DMFC_EN;
		if (ipu_di_use_count[0] == 1) {
			ipu_conf |= IPU_CONF_DI0_EN;
		}
		if (ipu_di_use_count[1] == 1) {
			ipu_conf |= IPU_CONF_DI1_EN;
		}
		if (ipu_ic_use_count > 0)
			ipu_conf |= IPU_CONF_IC_EN;
		
		__raw_writel(ipu_conf, IPU_CONF);
	}

err:
	return ret;
}

/*
 * This function is called to uninitialize a logical IPU channel.
 *
 * @param       channel Input parameter for the logical channel ID to uninit.
 */
void ipu_uninit_channel(ipu_channel_t channel)
{
	uint32_t reg;
	uint32_t in_dma, out_dma = 0;
	uint32_t ipu_conf;

	if ((g_channel_init_mask & (1L << IPU_CHAN_ID(channel))) == 0) {
		debug("Channel already uninitialized %d\n",
			IPU_CHAN_ID(channel));
		return;
	}

	/*
	 * Make sure channel is disabled
	 * Get input and output dma channels
	 */
	in_dma = channel_2_dma(channel, IPU_OUTPUT_BUFFER);
	out_dma = channel_2_dma(channel, IPU_VIDEO_IN_BUFFER);

	if (idma_is_set(IDMAC_CHA_EN, in_dma) ||
	    idma_is_set(IDMAC_CHA_EN, out_dma)) {
		printf(
			"Channel %d is not disabled, disable first\n",
			IPU_CHAN_ID(channel));
		return;
	}

	ipu_conf = __raw_readl(IPU_CONF);

	/* Reset the double buffer */
	reg = __raw_readl(IPU_CHA_DB_MODE_SEL(in_dma));
	__raw_writel(reg & ~idma_mask(in_dma), IPU_CHA_DB_MODE_SEL(in_dma));
	reg = __raw_readl(IPU_CHA_DB_MODE_SEL(out_dma));
	__raw_writel(reg & ~idma_mask(out_dma), IPU_CHA_DB_MODE_SEL(out_dma));

	switch (channel) {
	case MEM_DC_SYNC:
		ipu_dc_uninit(1);
		ipu_di_use_count[g_dc_di_assignment[1]]--;
		ipu_dc_use_count--;
		ipu_dmfc_use_count--;
		break;
	case MEM_BG_SYNC:
		ipu_dp_uninit(channel);
		ipu_dc_uninit(5);
		ipu_di_use_count[g_dc_di_assignment[5]]--;
		ipu_dc_use_count--;
		ipu_dp_use_count--;
		ipu_dmfc_use_count--;
		break;
	case MEM_FG_SYNC:
		ipu_dp_uninit(channel);
		ipu_dc_use_count--;
		ipu_dp_use_count--;
		ipu_dmfc_use_count--;
		break;
	default:
		break;
	}

	g_channel_init_mask &= ~(1L << IPU_CHAN_ID(channel));

	if (ipu_dc_use_count == 0)
		ipu_conf &= ~IPU_CONF_DC_EN;
	if (ipu_dp_use_count == 0)
		ipu_conf &= ~IPU_CONF_DP_EN;
	if (ipu_dmfc_use_count == 0)
		ipu_conf &= ~IPU_CONF_DMFC_EN;
	if (ipu_di_use_count[0] == 0) {
		ipu_conf &= ~IPU_CONF_DI0_EN;
	}
	if (ipu_di_use_count[1] == 0) {
		ipu_conf &= ~IPU_CONF_DI1_EN;
	}

	__raw_writel(ipu_conf, IPU_CONF);

	if (ipu_conf == 0) {
		clk_disable(g_ipu_clk);
		g_ipu_clk_enabled = 0;
	}

}

void ipu_ch_param_dump(int ch)
{
#ifdef DEBUG
	struct ipu_ch_param *p = ipu_ch_param_addr(ch);
	printf("ch %d word 0 - %08X %08X %08X %08X %08X\n", ch,
		 p->word[0].data[0], p->word[0].data[1], p->word[0].data[2],
		 p->word[0].data[3], p->word[0].data[4]);
	printf("ch %d word 1 - %08X %08X %08X %08X %08X\n", ch,
		 p->word[1].data[0], p->word[1].data[1], p->word[1].data[2],
		 p->word[1].data[3], p->word[1].data[4]);
	printf("PFS 0x%x, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 85, 4));
	printf("BPP 0x%x, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 0, 107, 3));
	printf("NPB 0x%x\n",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 78, 7));

	printf("FW %d, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 0, 125, 13));
	printf("FH %d, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 0, 138, 12));
	printf("Stride %d\n",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 102, 14));

	printf("Width0 %d+1, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 116, 3));
	printf("Width1 %d+1, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 119, 3));
	printf("Width2 %d+1, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 122, 3));
	printf("Width3 %d+1, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 125, 3));
	printf("Offset0 %d, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 128, 5));
	printf("Offset1 %d, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 133, 5));
	printf("Offset2 %d, ",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 138, 5));
	printf("Offset3 %d\n",
		 ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 143, 5));
#endif
}

static inline void ipu_ch_params_set_packing(struct ipu_ch_param *p,
					      int red_width, int red_offset,
					      int green_width, int green_offset,
					      int blue_width, int blue_offset,
					      int alpha_width, int alpha_offset)
{
	/* Setup red width and offset */
	ipu_ch_param_set_field(p, 1, 116, 3, red_width - 1);
	ipu_ch_param_set_field(p, 1, 128, 5, red_offset);
	/* Setup green width and offset */
	ipu_ch_param_set_field(p, 1, 119, 3, green_width - 1);
	ipu_ch_param_set_field(p, 1, 133, 5, green_offset);
	/* Setup blue width and offset */
	ipu_ch_param_set_field(p, 1, 122, 3, blue_width - 1);
	ipu_ch_param_set_field(p, 1, 138, 5, blue_offset);
	/* Setup alpha width and offset */
	ipu_ch_param_set_field(p, 1, 125, 3, alpha_width - 1);
	ipu_ch_param_set_field(p, 1, 143, 5, alpha_offset);
}

static void ipu_ch_param_init(int ch,
			      uint32_t pixel_fmt, uint32_t width,
			      uint32_t height, uint32_t stride,
			      uint32_t u, uint32_t v,
			      uint32_t uv_stride, dma_addr_t addr0,
			      dma_addr_t addr1)
{
	uint32_t u_offset = 0;
	uint32_t v_offset = 0;

	ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 125, 13, width - 1);

	if ((ch == 8) || (ch == 9) || (ch == 10)) {
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 138, 12, (height / 2) - 1);
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 102, 14, (stride * 2) - 1);
	} else {
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 138, 12, height - 1);
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 102, 14, stride - 1);
	}

	ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 0, 29, addr0 >> 3);
	ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 29, 29, addr1 >> 3);

	switch (pixel_fmt) {
	case IPU_PIX_FMT_GENERIC:
		/*Represents 8-bit Generic data */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 107, 3, 5);	/* bits/pixel */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 6);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 63);	/* burst size */

		break;
	case IPU_PIX_FMT_GENERIC_32:
		/*Represents 32-bit Generic data */
		break;
	case IPU_PIX_FMT_RGB565:
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 107, 3, 3);	/* bits/pixel */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 7);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 15);	/* burst size */

		ipu_ch_params_set_packing(ipu_ch_param_addr(ch), 5, 0, 6, 5, 5, 11, 8, 16);
		break;
	case IPU_PIX_FMT_BGR24:
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 107, 3, 1);	/* bits/pixel */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 7);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 19);	/* burst size */

		ipu_ch_params_set_packing(ipu_ch_param_addr(ch), 8, 0, 8, 8, 8, 16, 8, 24);
		break;
	case IPU_PIX_FMT_RGB24:
	case IPU_PIX_FMT_YUV444:
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 107, 3, 1);	/* bits/pixel */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 7);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 19);	/* burst size */

		ipu_ch_params_set_packing(ipu_ch_param_addr(ch), 8, 16, 8, 8, 8, 0, 8, 24);
		break;
	case IPU_PIX_FMT_BGRA32:
	case IPU_PIX_FMT_BGR32:
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 107, 3, 0);	/* bits/pixel */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 7);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 15);	/* burst size */

		ipu_ch_params_set_packing(ipu_ch_param_addr(ch), 8, 8, 8, 16, 8, 24, 8, 0);
		break;
	case IPU_PIX_FMT_RGBA32:
	case IPU_PIX_FMT_RGB32:
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 107, 3, 0);	/* bits/pixel */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 7);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 15);	/* burst size */

		ipu_ch_params_set_packing(ipu_ch_param_addr(ch), 8, 24, 8, 16, 8, 8, 8, 0);
		break;
	case IPU_PIX_FMT_ABGR32:
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 107, 3, 0);	/* bits/pixel */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 7);	/* pix format */

		ipu_ch_params_set_packing(ipu_ch_param_addr(ch), 8, 0, 8, 8, 8, 16, 8, 24);
		break;
	case IPU_PIX_FMT_UYVY:
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 107, 3, 3);	/* bits/pixel */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 0xA);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 15);	/* burst size */
		break;
	case IPU_PIX_FMT_YUYV:
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 107, 3, 3);	/* bits/pixel */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 0x8);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 31);	/* burst size */
		break;
	case IPU_PIX_FMT_YUV420P2:
	case IPU_PIX_FMT_YUV420P:
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 2);	/* pix format */

		if (uv_stride < stride / 2)
			uv_stride = stride / 2;

		u_offset = stride * height;
		v_offset = u_offset + (uv_stride * height / 2);
		/* burst size */
		if ((ch == 8) || (ch == 9) || (ch == 10)) {
			ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 15);
			uv_stride = uv_stride*2;
		} else {
			ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 31);
		}
		break;
	case IPU_PIX_FMT_YVU422P:
		/* BPP & pixel format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 1);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 31);	/* burst size */

		if (uv_stride < stride / 2)
			uv_stride = stride / 2;

		v_offset = (v == 0) ? stride * height : v;
		u_offset = (u == 0) ? v_offset + v_offset / 2 : u;
		break;
	case IPU_PIX_FMT_YUV422P:
		/* BPP & pixel format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 1);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 31);	/* burst size */

		if (uv_stride < stride / 2)
			uv_stride = stride / 2;

		u_offset = (u == 0) ? stride * height : u;
		v_offset = (v == 0) ? u_offset + u_offset / 2 : v;
		break;
	case IPU_PIX_FMT_NV12:
		/* BPP & pixel format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 85, 4, 4);	/* pix format */
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 78, 7, 31);	/* burst size */
		uv_stride = stride;
		u_offset = (u == 0) ? stride * height : u;
		break;
	default:
		puts("mxc ipu: unimplemented pixel format\n");
		break;
	}


	if (uv_stride)
		ipu_ch_param_set_field(ipu_ch_param_addr(ch), 1, 128, 14, uv_stride - 1);

	/* Get the uv offset from user when need cropping */
	if (u || v) {
		u_offset = u;
		v_offset = v;
	}

	/* UBO and VBO are 22-bit */
	if (u_offset/8 > 0x3fffff)
		puts("The value of U offset exceeds IPU limitation\n");
	if (v_offset/8 > 0x3fffff)
		puts("The value of V offset exceeds IPU limitation\n");

	ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 46, 22, u_offset / 8);
	ipu_ch_param_set_field(ipu_ch_param_addr(ch), 0, 68, 22, v_offset / 8);

	//printf("initializing idma ch %d @ %p\n", ch, ipu_ch_param_addr(ch));
};

static inline int _ipu_ch_param_get_burst_size(uint32_t ch)
{
	return ipu_ch_param_read_field(ipu_ch_param_addr(ch), 1, 78, 7) + 1;
};

static inline int _ipu_ch_param_get_bpp(uint32_t ch)
{
	return ipu_ch_param_read_field(ipu_ch_param_addr(ch), 0, 107, 3);
};



/*
 * This function is called to initialize a buffer for logical IPU channel.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       type            Input parameter which buffer to initialize.
 *
 * @param       pixel_fmt       Input parameter for pixel format of buffer.
 *                              Pixel format is a FOURCC ASCII code.
 *
 * @param       width           Input parameter for width of buffer in pixels.
 *
 * @param       height          Input parameter for height of buffer in pixels.
 *
 * @param       stride          Input parameter for stride length of buffer
 *                              in pixels.
 *
 * @param       phyaddr_0       Input parameter buffer 0 physical address.
 *
 * @param       phyaddr_1       Input parameter buffer 1 physical address.
 *                              Setting this to a value other than NULL enables
 *                              double buffering mode.
 *
 * @param       u		private u offset for additional cropping,
 *				zero if not used.
 *
 * @param       v		private v offset for additional cropping,
 *				zero if not used.
 *
 * @return      Returns 0 on success or negative error code on fail
 */
int32_t ipu_init_channel_buffer(ipu_channel_t channel, ipu_buffer_t type,
				uint32_t pixel_fmt,
				uint16_t width, uint16_t height,
				uint32_t stride,
				dma_addr_t phyaddr_0, dma_addr_t phyaddr_1,
				uint32_t u, uint32_t v)
{
	uint32_t reg;
	uint32_t dma_chan;
	uint32_t burst_size;

	static int once = 0;
	if(IPU_CHAN_ID(channel) == 19){
		if (once == 0){
			once++;
		}else{
			printf("ipu_init_channel_buffer fake reinit\n");
			return 0;
		}
	}

	//printf("ipu_init_channel_buffer: channel: %d, type: %d, pixel_fmt: 0x%x, width: %d, height: %d, stride: %d, phyaddr_0: 0x%x, phyaddr_1: 0x%x, u: 0x%x, v: 0x%x\r\n",
	//	IPU_CHAN_ID(channel), type, pixel_fmt, width, height,stride, phyaddr_0, phyaddr_1, u,v);
	
	dma_chan = channel_2_dma(channel, type);
	if (!idma_is_valid(dma_chan))
		return -EINVAL;

	if (stride < width * bytes_per_pixel(pixel_fmt)){
		stride = width * bytes_per_pixel(pixel_fmt);
//		printf("stride is update with bpp: 0x%x\n", bytes_per_pixel(pixel_fmt));
	}

	if (stride % 4) {
		printf(
			"Stride not 32-bit aligned, stride = %d\n", stride);
		return -EINVAL;
	}
	/* Build parameter memory data for DMA channel */
	ipu_ch_param_init(dma_chan, pixel_fmt, width, height, stride, u, v, 0,
			   phyaddr_0, phyaddr_1);

	if (ipu_is_dmfc_chan(dma_chan)) {
		burst_size = _ipu_ch_param_get_burst_size(dma_chan);
		//printf("burst_size: 0x%x\n", burst_size);
		ipu_dmfc_set_wait4eot(dma_chan, width);
		ipu_dmfc_set_burst_size(dma_chan, burst_size);
	}

	/* IC and ROT channels have restriction of 8 or 16 pix burst length */
	if (_ipu_is_ic_chan(dma_chan)) {
		if ((width % 16) == 0)
			_ipu_ch_param_set_burst_size(dma_chan, 16);
		else
			_ipu_ch_param_set_burst_size(dma_chan, 8);
	} 

	if (_ipu_is_ic_chan(dma_chan) ) {
		burst_size = _ipu_ch_param_get_burst_size(dma_chan);
		_ipu_ic_idma_init(dma_chan, width, height, burst_size);
	} else if (_ipu_is_smfc_chan(dma_chan)) {
		burst_size = _ipu_ch_param_get_burst_size(dma_chan);
		if ((pixel_fmt == IPU_PIX_FMT_GENERIC) &&
			((_ipu_ch_param_get_bpp(dma_chan) == 5) ||
			(_ipu_ch_param_get_bpp(dma_chan) == 3)))
			burst_size = burst_size >> 4;
		else
			burst_size = burst_size >> 2;
		_ipu_smfc_set_burst_size(channel, burst_size-1);
	}

	if (idma_is_set(IDMAC_CHA_PRI, dma_chan))
		ipu_ch_param_set_high_priority(dma_chan);

	ipu_ch_param_dump(dma_chan);

	reg = __raw_readl(IPU_CHA_DB_MODE_SEL(dma_chan));
	if (phyaddr_1)
		reg |= idma_mask(dma_chan);
	else
		reg &= ~idma_mask(dma_chan);
	__raw_writel(reg, IPU_CHA_DB_MODE_SEL(dma_chan));

	/* Reset to buffer 0 */
	__raw_writel(idma_mask(dma_chan), IPU_CHA_CUR_BUF(dma_chan));

	return 0;
}

/*
 * This function enables a logical channel.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int32_t ipu_enable_channel(ipu_channel_t channel)
{
	uint32_t reg;
	uint32_t in_dma;
	uint32_t out_dma;
	uint32_t ipu_conf;
	//printf("ipu_enable_channel %d\n", IPU_CHAN_ID(channel));

	if (g_channel_enable_mask & (1L << IPU_CHAN_ID(channel))) {
		printf("Warning: channel already enabled %d\n",
			IPU_CHAN_ID(channel));
		return 0;
	}
	
	ipu_conf = __raw_readl(IPU_CONF);
	if (ipu_di_use_count[0] > 0)
		ipu_conf |= IPU_CONF_DI0_EN;
	if (ipu_di_use_count[1] > 0)
		ipu_conf |= IPU_CONF_DI1_EN;
	if (ipu_dp_use_count > 0)
		ipu_conf |= IPU_CONF_DP_EN | IPU_CONF_IC_EN | IPU_CONF_ISP_EN | IPU_CONF_ROT_EN;
	if (ipu_dc_use_count > 0)
		ipu_conf |= IPU_CONF_DC_EN;
	if (ipu_dmfc_use_count > 0)
		ipu_conf |= IPU_CONF_DMFC_EN;
	if (ipu_smfc_use_count > 0)
		ipu_conf |= IPU_CONF_SMFC_EN;
	if (ipu_ic_use_count > 0)
		ipu_conf |= IPU_CONF_IC_EN;
	__raw_writel(ipu_conf, IPU_CONF);

	/* Get input and output dma channels */
	out_dma = channel_2_dma(channel, IPU_OUTPUT_BUFFER);
	in_dma = channel_2_dma(channel, IPU_VIDEO_IN_BUFFER);

	if (idma_is_valid(in_dma)) {
		reg = __raw_readl(IDMAC_CHA_EN(in_dma));
		__raw_writel(reg | idma_mask(in_dma), IDMAC_CHA_EN(in_dma));
	}
	if (idma_is_valid(out_dma)) {
		reg = __raw_readl(IDMAC_CHA_EN(out_dma));
		__raw_writel(reg | idma_mask(out_dma), IDMAC_CHA_EN(out_dma));
	}

	if ((channel == MEM_DC_SYNC) || (channel == MEM_BG_SYNC) ||
	    (channel == MEM_FG_SYNC)) {
		reg = __raw_readl(IDMAC_WM_EN(in_dma));
		__raw_writel(reg | idma_mask(in_dma), IDMAC_WM_EN(in_dma));
		ipu_dp_dc_enable(channel);
	}

	if (_ipu_is_ic_chan(in_dma) || _ipu_is_ic_chan(out_dma))
		_ipu_ic_enable_task(channel);

	g_channel_enable_mask |= 1L << IPU_CHAN_ID(channel);
	ipu_dump_registers();
	//printf("--ipu_enable_channel: int_ctrl: 0x%x\n");				
	return 0;
}

/*
 * This function clear buffer ready for a logical channel.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       type            Input parameter which buffer to clear.
 *
 * @param       bufNum          Input parameter for which buffer number clear
 *				ready state.
 *
 */
void ipu_clear_buffer_ready(ipu_channel_t channel, ipu_buffer_t type,
		uint32_t bufNum)
{
	uint32_t dma_ch = channel_2_dma(channel, type);

	if (!idma_is_valid(dma_ch))
		return;

	__raw_writel(0xF0000000, IPU_GPR); /* write one to clear */
	if (bufNum == 0) {
		if (idma_is_set(IPU_CHA_BUF0_RDY, dma_ch)) {
			__raw_writel(idma_mask(dma_ch),
					IPU_CHA_BUF0_RDY(dma_ch));
		}
	} else {
		if (idma_is_set(IPU_CHA_BUF1_RDY, dma_ch)) {
			__raw_writel(idma_mask(dma_ch),
					IPU_CHA_BUF1_RDY(dma_ch));
		}
	}
	__raw_writel(0x0, IPU_GPR); /* write one to set */
}

/*
 * This function disables a logical channel.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       wait_for_stop   Flag to set whether to wait for channel end
 *                              of frame or return immediately.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int32_t ipu_disable_channel(ipu_channel_t channel)
{
	uint32_t reg;
	uint32_t in_dma;
	uint32_t out_dma;

	if ((g_channel_enable_mask & (1L << IPU_CHAN_ID(channel))) == 0) {
		debug("Channel already disabled %d\n",
			IPU_CHAN_ID(channel));
		return 0;
	}

	/* Get input and output dma channels */
	out_dma = channel_2_dma(channel, IPU_OUTPUT_BUFFER);
	in_dma = channel_2_dma(channel, IPU_VIDEO_IN_BUFFER);

	if ((idma_is_valid(in_dma) &&
		!idma_is_set(IDMAC_CHA_EN, in_dma))
		&& (idma_is_valid(out_dma) &&
		!idma_is_set(IDMAC_CHA_EN, out_dma)))
		return -EINVAL;

	if ((channel == MEM_BG_SYNC) || (channel == MEM_FG_SYNC) ||
	    (channel == MEM_DC_SYNC)) {
		ipu_dp_dc_disable(channel, 0);
	}

	/* Disable DMA channel(s) */
	if (idma_is_valid(in_dma)) {
		reg = __raw_readl(IDMAC_CHA_EN(in_dma));
		__raw_writel(reg & ~idma_mask(in_dma), IDMAC_CHA_EN(in_dma));
		__raw_writel(idma_mask(in_dma), IPU_CHA_CUR_BUF(in_dma));
	}
	if (idma_is_valid(out_dma)) {
		reg = __raw_readl(IDMAC_CHA_EN(out_dma));
		__raw_writel(reg & ~idma_mask(out_dma), IDMAC_CHA_EN(out_dma));
		__raw_writel(idma_mask(out_dma), IPU_CHA_CUR_BUF(out_dma));
	}

	g_channel_enable_mask &= ~(1L << IPU_CHAN_ID(channel));

	/* Set channel buffers NOT to be ready */
	if (idma_is_valid(in_dma)) {
		ipu_clear_buffer_ready(channel, IPU_VIDEO_IN_BUFFER, 0);
		ipu_clear_buffer_ready(channel, IPU_VIDEO_IN_BUFFER, 1);
	}
	if (idma_is_valid(out_dma)) {
		ipu_clear_buffer_ready(channel, IPU_OUTPUT_BUFFER, 0);
		ipu_clear_buffer_ready(channel, IPU_OUTPUT_BUFFER, 1);
	}

	return 0;
}

uint32_t bytes_per_pixel(uint32_t fmt)
{
	switch (fmt) {
	case IPU_PIX_FMT_GENERIC:	/*generic data */
	case IPU_PIX_FMT_RGB332:
	case IPU_PIX_FMT_YUV420P:
	case IPU_PIX_FMT_YUV422P:
		return 1;
		break;
	case IPU_PIX_FMT_RGB565:
	case IPU_PIX_FMT_YUYV:
	case IPU_PIX_FMT_UYVY:
		return 2;
		break;
	case IPU_PIX_FMT_BGR24:
	case IPU_PIX_FMT_RGB24:
		return 3;
		break;
	case IPU_PIX_FMT_GENERIC_32:	/*generic data */
	case IPU_PIX_FMT_BGR32:
	case IPU_PIX_FMT_BGRA32:
	case IPU_PIX_FMT_RGB32:
	case IPU_PIX_FMT_RGBA32:
	case IPU_PIX_FMT_ABGR32:
		return 4;
		break;
	default:
		return 1;
		break;
	}
	return 0;
}

ipu_color_space_t format_to_colorspace(uint32_t fmt)
{
	switch (fmt) {
	case IPU_PIX_FMT_RGB666:
	case IPU_PIX_FMT_RGB565:
	case IPU_PIX_FMT_BGR24:
	case IPU_PIX_FMT_RGB24:
	case IPU_PIX_FMT_BGR32:
	case IPU_PIX_FMT_BGRA32:
	case IPU_PIX_FMT_RGB32:
	case IPU_PIX_FMT_RGBA32:
	case IPU_PIX_FMT_ABGR32:
	case IPU_PIX_FMT_LVDS666:
	case IPU_PIX_FMT_LVDS888:
		return RGB;
		break;

	default:
		return YCbCr;
		break;
	}
	return RGB;
}


int32_t ipu_enable_csi(uint32_t csi)
{
	uint32_t reg;
	unsigned long lock_flags;

	if (csi > 1) {
		printf("Wrong csi num_%d\n", csi);
		return -EINVAL;
	}
	ipu_csi_use_count[csi]++;
	if (ipu_csi_use_count[csi] == 1) {
		reg = __raw_readl(IPU_CONF);
		if (csi == 0){
			__raw_writel(reg | IPU_CONF_CSI0_EN, IPU_CONF);
		}
		else{
			__raw_writel(reg | IPU_CONF_CSI1_EN, IPU_CONF);
		}
//		printf("ENABLE CSI: 0x%08x\n", __raw_readl(IPU_CONF));			
	}

	return 0;
}

int32_t ipu_disable_csi(uint32_t csi)
{
	uint32_t reg;
	unsigned long lock_flags;

	if (csi > 1) {
		printf("Wrong csi num_%d\n", csi);
		return -EINVAL;
	}
	ipu_csi_use_count[csi]--;
	if (ipu_csi_use_count[csi] == 0) {
		reg = __raw_readl(IPU_CONF);
		if (csi == 0)
			__raw_writel(reg & ~IPU_CONF_CSI0_EN, IPU_CONF);
		else
			__raw_writel(reg & ~IPU_CONF_CSI1_EN, IPU_CONF);
	}
	return 0;
}


int32_t ipu_select_buffer(ipu_channel_t channel, ipu_buffer_t type,
			  uint32_t bufNum)
{
	uint32_t dma_chan = channel_2_dma(channel, type);

	if (dma_chan == IDMA_CHAN_INVALID)
		return -EINVAL;
	if (bufNum == 0)
		__raw_writel(idma_mask(dma_chan),
			     IPU_CHA_BUF0_RDY(dma_chan));
	else if (bufNum == 1)
		__raw_writel(idma_mask(dma_chan),
			     IPU_CHA_BUF1_RDY(dma_chan));
	return 0;
}

static inline void _ipu_ch_param_set_buffer(uint32_t ch, int bufNum,
					    dma_addr_t phyaddr)
{
	if (phyaddr%8)
		printf("IDMAC%d's EBA%d is not 8-byte aligned: 0x%x\n", ch, bufNum, phyaddr);

	//printf("IDMAC%d's set Buf Index: %d to address: 0x%x\n", ch, bufNum, phyaddr);
	ipu_ch_param_mod_field(ipu_ch_param_addr(ch), 1, 29 * bufNum, 29, phyaddr / 8);
};


int32_t ipu_update_channel_buffer(ipu_channel_t channel, ipu_buffer_t type,
				  uint32_t bufNum, dma_addr_t phyaddr)
{
	uint32_t reg;
	int ret = 0;
	unsigned long lock_flags;
	uint32_t dma_chan = channel_2_dma(channel, type);
	if (dma_chan == IDMA_CHAN_INVALID)
		return -EINVAL;

	if (bufNum == 0)
		reg = __raw_readl(IPU_CHA_BUF0_RDY(dma_chan));
	else if (bufNum == 1)
		reg = __raw_readl(IPU_CHA_BUF1_RDY(dma_chan));

	if ((reg & idma_mask(dma_chan)) == 0)
		_ipu_ch_param_set_buffer(dma_chan, bufNum, phyaddr);
	else
		ret = -EACCES;

	return ret;
}


