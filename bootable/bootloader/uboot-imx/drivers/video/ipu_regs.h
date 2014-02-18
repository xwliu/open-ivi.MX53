/*
 * Porting to u-boot:
 *
 * (C) Copyright 2010
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de
 *
 * Linux IPU driver:
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

#ifndef __IPU_REGS_INCLUDED__
#define __IPU_REGS_INCLUDED__

#define IPU_DISP0_BASE		0x00000000
#define IPU_MCU_T_DEFAULT	8
#define IPU_DISP1_BASE		(IPU_MCU_T_DEFAULT << 25)
#define IPU_CM_REG_BASE		0x1E000000
#define IPU_STAT_REG_BASE	0x1E000200
#define IPU_IDMAC_REG_BASE	0x1E008000
#define IPU_ISP_REG_BASE	0x1E010000
#define IPU_DP_REG_BASE		0x1E018000
#define IPU_IC_REG_BASE		0x1E020000
#define IPU_IRT_REG_BASE	0x1E028000
#define IPU_CSI0_REG_BASE	0x1E030000
#define IPU_CSI1_REG_BASE	0x1E038000
#define IPU_DI0_REG_BASE	0x1E040000
#define IPU_DI1_REG_BASE	0x1E048000
#define IPU_SMFC_REG_BASE	0x1E050000
#define IPU_DC_REG_BASE		0x1E058000
#define IPU_DMFC_REG_BASE	0x1E060000
#define IPU_CPMEM_REG_BASE	0x1F000000
#define IPU_LUT_REG_BASE	0x1F020000
#define IPU_SRM_REG_BASE	0x1F040000
#define IPU_TPM_REG_BASE	0x1F060000
#define IPU_DC_TMPL_REG_BASE	0x1F080000
#define IPU_ISP_TBPR_REG_BASE	0x1F0C0000
#define IPU_VDI_REG_BASE	0x1E068000


extern u32 *ipu_dc_tmpl_reg;

#define DC_EVT_NF		0
#define DC_EVT_NL		1
#define DC_EVT_EOF		2
#define DC_EVT_NFIELD		3
#define DC_EVT_EOL		4
#define DC_EVT_EOFIELD		5
#define DC_EVT_NEW_ADDR		6
#define DC_EVT_NEW_CHAN		7
#define DC_EVT_NEW_DATA		8

#define DC_EVT_NEW_ADDR_W_0	0
#define DC_EVT_NEW_ADDR_W_1	1
#define DC_EVT_NEW_CHAN_W_0	2
#define DC_EVT_NEW_CHAN_W_1	3
#define DC_EVT_NEW_DATA_W_0	4
#define DC_EVT_NEW_DATA_W_1	5
#define DC_EVT_NEW_ADDR_R_0	6
#define DC_EVT_NEW_ADDR_R_1	7
#define DC_EVT_NEW_CHAN_R_0	8
#define DC_EVT_NEW_CHAN_R_1	9
#define DC_EVT_NEW_DATA_R_0	10
#define DC_EVT_NEW_DATA_R_1	11

/* Software reset for ipu */
#define SW_IPU_RST	8

/* CMOS Sensor Interface Registers */
#define CSI_SENS_CONF(csi)	(ipu_csi_reg[csi])
#define CSI_SENS_FRM_SIZE(csi)	(ipu_csi_reg[csi] + 0x0004/4)
#define CSI_ACT_FRM_SIZE(csi)	(ipu_csi_reg[csi] + 0x0008/4)
#define CSI_OUT_FRM_CTRL(csi)	(ipu_csi_reg[csi] + 0x000C/4)
#define CSI_TST_CTRL(csi)	(ipu_csi_reg[csi] + 0x0010/4)
#define CSI_CCIR_CODE_1(csi)	(ipu_csi_reg[csi] + 0x0014/4)
#define CSI_CCIR_CODE_2(csi)	(ipu_csi_reg[csi] + 0x0018/4)
#define CSI_CCIR_CODE_3(csi)	(ipu_csi_reg[csi] + 0x001C/4)
#define CSI_MIPI_DI(csi)	(ipu_csi_reg[csi] + 0x0020/4)
#define CSI_SKIP(csi)		(ipu_csi_reg[csi] + 0x0024/4)
#define CSI_CPD_CTRL(csi)	(ipu_csi_reg[csi] + 0x0028/4)
#define CSI_CPD_RC(csi, n)	(ipu_csi_reg[csi] + 0x002C/4 + n)
#define CSI_CPD_RS(csi, n)	(ipu_csi_reg[csi] + 0x004C/4 + n)
#define CSI_CPD_GRC(csi, n)	(ipu_csi_reg[csi] + 0x005C/4 + n)
#define CSI_CPD_GRS(csi, n)	(ipu_csi_reg[csi] + 0x007C/4 + n)
#define CSI_CPD_GBC(csi, n)	(ipu_csi_reg[csi] + 0x008C/4 + n)
#define CSI_CPD_GBS(csi, n)	(ipu_csi_reg[csi] + 0x00AC/4 + n)
#define CSI_CPD_BC(csi, n)	(ipu_csi_reg[csi] + 0x00BC/4 + n)
#define CSI_CPD_BS(csi, n)	(ipu_csi_reg[csi] + 0x00DC/4 + n)
#define CSI_CPD_OFFSET1(csi)	(ipu_csi_reg[csi] + 0x00EC/4)
#define CSI_CPD_OFFSET2(csi)	(ipu_csi_reg[csi] + 0x00F0/4)

/*SMFC Registers */
#define SMFC_MAP	(ipu_smfc_reg)
#define SMFC_WMC	(ipu_smfc_reg + 0x0004/4)
#define SMFC_BS		(ipu_smfc_reg + 0x0008/4)

/* Image Converter Registers */
#define IC_CONF			(ipu_ic_reg)
#define IC_PRP_ENC_RSC		(ipu_ic_reg + 0x0004/4)
#define IC_PRP_VF_RSC		(ipu_ic_reg + 0x0008/4)
#define IC_PP_RSC		(ipu_ic_reg + 0x000C/4)
#define IC_CMBP_1		(ipu_ic_reg + 0x0010/4)
#define IC_CMBP_2		(ipu_ic_reg + 0x0014/4)
#define IC_IDMAC_1		(ipu_ic_reg + 0x0018/4)
#define IC_IDMAC_2		(ipu_ic_reg + 0x001C/4)
#define IC_IDMAC_3		(ipu_ic_reg + 0x0020/4)
#define IC_IDMAC_4		(ipu_ic_reg + 0x0024/4)

enum {
	IPU_CONF_CSI0_EN = 0x00000001,
	IPU_CONF_CSI1_EN = 0x00000002,
	IPU_CONF_IC_EN = 0x00000004,
	IPU_CONF_ROT_EN = 0x00000008,
	IPU_CONF_ISP_EN = 0x00000010,
	IPU_CONF_DP_EN = 0x00000020,
	IPU_CONF_DI0_EN = 0x00000040,
	IPU_CONF_DI1_EN = 0x00000080,
	IPU_CONF_DMFC_EN = 0x00000400,
	IPU_CONF_SMFC_EN = 0x00000100,
	IPU_CONF_DC_EN = 0x00000200,
	IPU_CONF_VDI_EN = 0x00001000,
	IPU_CONF_IDMAC_DIS = 0x00400000,
	IPU_CONF_IC_DMFC_SEL = 0x02000000,
	IPU_CONF_IC_DMFC_SYNC = 0x04000000,
	IPU_CONF_VDI_DMFC_SYNC = 0x08000000,
	IPU_CONF_CSI0_DATA_SOURCE = 0x10000000,
	IPU_CONF_CSI0_DATA_SOURCE_OFFSET = 28,
	IPU_CONF_CSI1_DATA_SOURCE = 0x20000000,
	IPU_CONF_IC_INPUT = 0x40000000,
	IPU_CONF_CSI_SEL = 0x80000000,

	DI0_COUNTER_RELEASE = 0x01000000,
	DI1_COUNTER_RELEASE = 0x02000000,
	FS_VF_IN_VALID = 0x80000000,
	FS_ENC_IN_VALID = 0x40000000,
	
	/* Image Converter Register bits */
	IC_CONF_PRPENC_EN = 0x00000001,
	IC_CONF_PRPENC_CSC1 = 0x00000002,
	IC_CONF_PRPENC_ROT_EN = 0x00000004,
	IC_CONF_PRPVF_EN = 0x00000100,
	IC_CONF_PRPVF_CSC1 = 0x00000200,
	IC_CONF_PRPVF_CSC2 = 0x00000400,
	IC_CONF_PRPVF_CMB = 0x00000800,
	IC_CONF_PRPVF_ROT_EN = 0x00001000,
	IC_CONF_PP_EN = 0x00010000,
	IC_CONF_PP_CSC1 = 0x00020000,
	IC_CONF_PP_CSC2 = 0x00040000,
	IC_CONF_PP_CMB = 0x00080000,
	IC_CONF_PP_ROT_EN = 0x00100000,
	IC_CONF_IC_GLB_LOC_A = 0x10000000,
	IC_CONF_KEY_COLOR_EN = 0x20000000,
	IC_CONF_RWS_EN = 0x40000000,
	IC_CONF_CSI_MEM_WR_EN = 0x80000000,

	IC_IDMAC_1_CB0_BURST_16 = 0x00000001,
	IC_IDMAC_1_CB1_BURST_16 = 0x00000002,
	IC_IDMAC_1_CB2_BURST_16 = 0x00000004,
	IC_IDMAC_1_CB3_BURST_16 = 0x00000008,
	IC_IDMAC_1_CB4_BURST_16 = 0x00000010,
	IC_IDMAC_1_CB5_BURST_16 = 0x00000020,
	IC_IDMAC_1_CB6_BURST_16 = 0x00000040,
	IC_IDMAC_1_CB7_BURST_16 = 0x00000080,
	IC_IDMAC_1_PRPENC_ROT_MASK = 0x00003800,
	IC_IDMAC_1_PRPENC_ROT_OFFSET = 11,
	IC_IDMAC_1_PRPVF_ROT_MASK = 0x0001C000,
	IC_IDMAC_1_PRPVF_ROT_OFFSET = 14,
	IC_IDMAC_1_PP_ROT_MASK = 0x000E0000,
	IC_IDMAC_1_PP_ROT_OFFSET = 17,
	IC_IDMAC_1_PP_FLIP_RS = 0x00400000,
	IC_IDMAC_1_PRPVF_FLIP_RS = 0x00200000,
	IC_IDMAC_1_PRPENC_FLIP_RS = 0x00100000,

	IC_IDMAC_2_PRPENC_HEIGHT_MASK = 0x000003FF,
	IC_IDMAC_2_PRPENC_HEIGHT_OFFSET = 0,
	IC_IDMAC_2_PRPVF_HEIGHT_MASK = 0x000FFC00,
	IC_IDMAC_2_PRPVF_HEIGHT_OFFSET = 10,
	IC_IDMAC_2_PP_HEIGHT_MASK = 0x3FF00000,
	IC_IDMAC_2_PP_HEIGHT_OFFSET = 20,

	IC_IDMAC_3_PRPENC_WIDTH_MASK = 0x000003FF,
	IC_IDMAC_3_PRPENC_WIDTH_OFFSET = 0,
	IC_IDMAC_3_PRPVF_WIDTH_MASK = 0x000FFC00,
	IC_IDMAC_3_PRPVF_WIDTH_OFFSET = 10,
	IC_IDMAC_3_PP_WIDTH_MASK = 0x3FF00000,
	IC_IDMAC_3_PP_WIDTH_OFFSET = 20,

	CSI_SENS_CONF_DATA_FMT_SHIFT = 8,
	CSI_SENS_CONF_DATA_FMT_MASK = 0x00000700,
	CSI_SENS_CONF_DATA_FMT_RGB_YUV444 = 0L,
	CSI_SENS_CONF_DATA_FMT_YUV422_YUYV = 1L,
	CSI_SENS_CONF_DATA_FMT_YUV422_UYVY = 2L,
	CSI_SENS_CONF_DATA_FMT_BAYER = 3L,
	CSI_SENS_CONF_DATA_FMT_RGB565 = 4L,
	CSI_SENS_CONF_DATA_FMT_RGB555 = 5L,
	CSI_SENS_CONF_DATA_FMT_RGB444 = 6L,
	CSI_SENS_CONF_DATA_FMT_JPEG = 7L,

	CSI_SENS_CONF_VSYNC_POL_SHIFT = 0,
	CSI_SENS_CONF_HSYNC_POL_SHIFT = 1,
	CSI_SENS_CONF_DATA_POL_SHIFT = 2,
	CSI_SENS_CONF_PIX_CLK_POL_SHIFT = 3,
	CSI_SENS_CONF_SENS_PRTCL_MASK = 0x00000070L,
	CSI_SENS_CONF_SENS_PRTCL_SHIFT = 4,
	CSI_SENS_CONF_PACK_TIGHT_SHIFT = 7,
	CSI_SENS_CONF_DATA_WIDTH_SHIFT = 11,
	CSI_SENS_CONF_EXT_VSYNC_SHIFT = 15,
	CSI_SENS_CONF_DIVRATIO_SHIFT = 16,

	CSI_SENS_CONF_DIVRATIO_MASK = 0x00FF0000L,
	CSI_SENS_CONF_DATA_DEST_SHIFT = 24,
	CSI_SENS_CONF_DATA_DEST_MASK = 0x07000000L,
	CSI_SENS_CONF_JPEG8_EN_SHIFT = 27,
	CSI_SENS_CONF_JPEG_EN_SHIFT = 28,
	CSI_SENS_CONF_FORCE_EOF_SHIFT = 29,
	CSI_SENS_CONF_DATA_EN_POL_SHIFT = 31,

	CSI_DATA_DEST_ISP = 1L,
	CSI_DATA_DEST_IC = 2L,
	CSI_DATA_DEST_IDMAC = 4L,

	CSI_CCIR_ERR_DET_EN = 0x01000000L,
	CSI_HORI_DOWNSIZE_EN = 0x80000000L,
	CSI_VERT_DOWNSIZE_EN = 0x40000000L,
	CSI_TEST_GEN_MODE_EN = 0x01000000L,

	CSI_HSC_MASK = 0x1FFF0000,
	CSI_HSC_SHIFT = 16,
	CSI_VSC_MASK = 0x00000FFF,
	CSI_VSC_SHIFT = 0,

	CSI_TEST_GEN_R_MASK = 0x000000FFL,
	CSI_TEST_GEN_R_SHIFT = 0,
	CSI_TEST_GEN_G_MASK = 0x0000FF00L,
	CSI_TEST_GEN_G_SHIFT = 8,
	CSI_TEST_GEN_B_MASK = 0x00FF0000L,
	CSI_TEST_GEN_B_SHIFT = 16,

	CSI_MIPI_DI0_MASK = 0x000000FFL,
	CSI_MIPI_DI0_SHIFT = 0,
	CSI_MIPI_DI1_MASK = 0x0000FF00L,
	CSI_MIPI_DI1_SHIFT = 8,
	CSI_MIPI_DI2_MASK = 0x00FF0000L,
	CSI_MIPI_DI2_SHIFT = 16,
	CSI_MIPI_DI3_MASK = 0xFF000000L,
	CSI_MIPI_DI3_SHIFT = 24,

	CSI_MAX_RATIO_SKIP_ISP_MASK = 0x00070000L,
	CSI_MAX_RATIO_SKIP_ISP_SHIFT = 16,
	CSI_SKIP_ISP_MASK = 0x00F80000L,
	CSI_SKIP_ISP_SHIFT = 19,
	CSI_MAX_RATIO_SKIP_SMFC_MASK = 0x00000007L,
	CSI_MAX_RATIO_SKIP_SMFC_SHIFT = 0,
	CSI_SKIP_SMFC_MASK = 0x000000F8L,
	CSI_SKIP_SMFC_SHIFT = 3,
	CSI_ID_2_SKIP_MASK = 0x00000300L,
	CSI_ID_2_SKIP_SHIFT = 8,

	CSI_COLOR_FIRST_ROW_MASK = 0x00000002L,
	CSI_COLOR_FIRST_COMP_MASK = 0x00000001L,

	SMFC_MAP_CH0_MASK = 0x00000007L,
	SMFC_MAP_CH0_SHIFT = 0,
	SMFC_MAP_CH1_MASK = 0x00000038L,
	SMFC_MAP_CH1_SHIFT = 3,
	SMFC_MAP_CH2_MASK = 0x000001C0L,
	SMFC_MAP_CH2_SHIFT = 6,
	SMFC_MAP_CH3_MASK = 0x00000E00L,
	SMFC_MAP_CH3_SHIFT = 9,

	SMFC_WM0_SET_MASK = 0x00000007L,
	SMFC_WM0_SET_SHIFT = 0,
	SMFC_WM1_SET_MASK = 0x000001C0L,
	SMFC_WM1_SET_SHIFT = 6,
	SMFC_WM2_SET_MASK = 0x00070000L,
	SMFC_WM2_SET_SHIFT = 16,
	SMFC_WM3_SET_MASK = 0x01C00000L,
	SMFC_WM3_SET_SHIFT = 22,

	SMFC_WM0_CLR_MASK = 0x00000038L,
	SMFC_WM0_CLR_SHIFT = 3,
	SMFC_WM1_CLR_MASK = 0x00000E00L,
	SMFC_WM1_CLR_SHIFT = 9,
	SMFC_WM2_CLR_MASK = 0x00380000L,
	SMFC_WM2_CLR_SHIFT = 19,
	SMFC_WM3_CLR_MASK = 0x0E000000L,
	SMFC_WM3_CLR_SHIFT = 25,

	SMFC_BS0_MASK = 0x0000000FL,
	SMFC_BS0_SHIFT = 0,
	SMFC_BS1_MASK = 0x000000F0L,
	SMFC_BS1_SHIFT = 4,
	SMFC_BS2_MASK = 0x00000F00L,
	SMFC_BS2_SHIFT = 8,
	SMFC_BS3_MASK = 0x0000F000L,
	SMFC_BS3_SHIFT = 12,

	DI_DW_GEN_ACCESS_SIZE_OFFSET = 24,
	DI_DW_GEN_COMPONENT_SIZE_OFFSET = 16,

	DI_GEN_DI_CLK_EXT = 0x100000,
	DI_GEN_POLARITY_1 = 0x00000001,
	DI_GEN_POLARITY_2 = 0x00000002,
	DI_GEN_POLARITY_3 = 0x00000004,
	DI_GEN_POLARITY_4 = 0x00000008,
	DI_GEN_POLARITY_5 = 0x00000010,
	DI_GEN_POLARITY_6 = 0x00000020,
	DI_GEN_POLARITY_7 = 0x00000040,
	DI_GEN_POLARITY_8 = 0x00000080,
	DI_GEN_POL_CLK = 0x20000,

	DI_POL_DRDY_DATA_POLARITY = 0x00000080,
	DI_POL_DRDY_POLARITY_15 = 0x00000010,
	DI_VSYNC_SEL_OFFSET = 13,

	DC_WR_CH_CONF_FIELD_MODE = 0x00000200,
	DC_WR_CH_CONF_PROG_TYPE_OFFSET = 5,
	DC_WR_CH_CONF_PROG_TYPE_MASK = 0x000000E0,
	DC_WR_CH_CONF_PROG_DI_ID = 0x00000004,
	DC_WR_CH_CONF_PROG_DISP_ID_OFFSET = 3,
	DC_WR_CH_CONF_PROG_DISP_ID_MASK = 0x00000018,

	DP_COM_CONF_FG_EN = 0x00000001,
	DP_COM_CONF_GWSEL = 0x00000002,
	DP_COM_CONF_GWAM = 0x00000004,
	DP_COM_CONF_GWCKE = 0x00000008,
	DP_COM_CONF_CSC_DEF_MASK = 0x00000300,
	DP_COM_CONF_CSC_DEF_OFFSET = 8,
	DP_COM_CONF_CSC_DEF_FG = 0x00000300,
	DP_COM_CONF_CSC_DEF_BG = 0x00000200,
	DP_COM_CONF_CSC_DEF_BOTH = 0x00000100,
	DP_COM_CONF_GAMMA_EN = 0x00001000,
	DP_COM_CONF_GAMMA_YUV_EN = 0x00002000,
};

enum di_pins {
	DI_PIN11 = 0,
	DI_PIN12 = 1,
	DI_PIN13 = 2,
	DI_PIN14 = 3,
	DI_PIN15 = 4,
	DI_PIN16 = 5,
	DI_PIN17 = 6,
	DI_PIN_CS = 7,

	DI_PIN_SER_CLK = 0,
	DI_PIN_SER_RS = 1,
};

enum di_sync_wave {
	DI_SYNC_NONE = -1,
	DI_SYNC_CLK = 0,
	DI_SYNC_INT_HSYNC = 1,
	DI_SYNC_HSYNC = 2,
	DI_SYNC_VSYNC = 3,
	DI_SYNC_DE = 5,
};

struct ipu_cm {
	u32 conf;
	u32 sisg_ctrl0;
	u32 sisg_ctrl1;
	u32 sisg_set[6];
	u32 sisg_clear[6];
	u32 int_ctrl[15];
	u32 sdma_event[10];
	u32 srm_pri1;
	u32 srm_pri2;
	u32 fs_proc_flow[3];
	u32 fs_disp_flow[2];
	u32 skip;
	u32 disp_alt_conf;
	u32 disp_gen;
	u32 disp_alt[4];
	u32 snoop;
	u32 mem_rst;
	u32 pm;
	u32 gpr;
	u32 reserved0[26];
	u32 ch_db_mode_sel[2];
	u32 reserved1[16];
	u32 alt_ch_db_mode_sel[2];
	u32 reserved2[2];
	u32 ch_trb_mode_sel[2];
};

struct ipu_idmac {
	u32 conf;
	u32 ch_en[2];
	u32 sep_alpha;
	u32 alt_sep_alpha;
	u32 ch_pri[2];
	u32 wm_en[2];
	u32 lock_en[2];
	u32 sub_addr[5];
	u32 bndm_en[2];
	u32 sc_cord[2];
	u32 reserved[45];
	u32 ch_busy[2];
};

struct ipu_com_async {
	u32 com_conf_async;
	u32 graph_wind_ctrl_async;
	u32 fg_pos_async;
	u32 cur_pos_async;
	u32 cur_map_async;
	u32 gamma_c_async[8];
	u32 gamma_s_async[4];
	u32 dp_csca_async[4];
	u32 dp_csc_async[2];
};

struct ipu_dp {
	u32 com_conf_sync;
	u32 graph_wind_ctrl_sync;
	u32 fg_pos_sync;
	u32 cur_pos_sync;
	u32 cur_map_sync;
	u32 gamma_c_sync[8];
	u32 gamma_s_sync[4];
	u32 csca_sync[4];
	u32 csc_sync[2];
	u32 cur_pos_alt;
	struct ipu_com_async async[2];
};

struct ipu_di {
	u32 general;
	u32 bs_clkgen0;
	u32 bs_clkgen1;
	u32 sw_gen0[9];
	u32 sw_gen1[9];
	u32 sync_as;
	u32 dw_gen[12];
	u32 dw_set[48];
	u32 stp_rep[4];
	u32 stp_rep9;
	u32 ser_conf;
	u32 ssc;
	u32 pol;
	u32 aw0;
	u32 aw1;
	u32 scr_conf;
	u32 stat;
};

struct ipu_stat {
	u32 int_stat[15];
	u32 cur_buf[2];
	u32 alt_cur_buf_0;
	u32 alt_cur_buf_1;
	u32 srm_stat;
	u32 proc_task_stat;
	u32 disp_task_stat;
	u32 triple_cur_buf[4];
	u32 ch_buf0_rdy[2];
	u32 ch_buf1_rdy[2];
	u32 alt_ch_buf0_rdy[2];
	u32 alt_ch_buf1_rdy[2];
	u32 ch_buf2_rdy[2];
};

struct ipu_dc_ch {
	u32 wr_ch_conf;
	u32 wr_ch_addr;
	u32 rl[5];
};

struct ipu_dc {
	struct ipu_dc_ch dc_ch0_1_2[3];
	u32 cmd_ch_conf_3;
	u32 cmd_ch_conf_4;
	struct ipu_dc_ch dc_ch5_6[2];
	struct ipu_dc_ch dc_ch8;
	u32 rl6_ch_8;
	struct ipu_dc_ch dc_ch9;
	u32 rl6_ch_9;
	u32 gen;
	u32 disp_conf1[4];
	u32 disp_conf2[4];
	u32 di0_conf[2];
	u32 di1_conf[2];
	u32 dc_map_ptr[15];
	u32 dc_map_val[12];
	u32 udge[16];
	u32 lla[2];
	u32 r_lla[2];
	u32 wr_ch_addr_5_alt;
	u32 stat;
};

struct ipu_dmfc {
	u32 rd_chan;
	u32 wr_chan;
	u32 wr_chan_def;
	u32 dp_chan;
	u32 dp_chan_def;
	u32 general[2];
	u32 ic_ctrl;
	u32 wr_chan_alt;
	u32 wr_chan_def_alt;
	u32 general1_alt;
	u32 stat;
};

#define IPU_CM_REG		((struct ipu_cm *)(IPU_CTRL_BASE_ADDR + \
				IPU_CM_REG_BASE))
#define IPU_CONF		(&IPU_CM_REG->conf)
#define IPU_SRM_PRI1		(&IPU_CM_REG->srm_pri1)
#define IPU_SRM_PRI2		(&IPU_CM_REG->srm_pri2)
#define IPU_FS_PROC_FLOW1	(&IPU_CM_REG->fs_proc_flow[0])
#define IPU_FS_PROC_FLOW2	(&IPU_CM_REG->fs_proc_flow[1])
#define IPU_FS_PROC_FLOW3	(&IPU_CM_REG->fs_proc_flow[2])
#define IPU_FS_DISP_FLOW1	(&IPU_CM_REG->fs_disp_flow[0])
#define IPU_DISP_GEN		(&IPU_CM_REG->disp_gen)
#define IPU_MEM_RST		(&IPU_CM_REG->mem_rst)
#define IPU_PM                  (&IPU_CM_REG->pm)
#define IPU_GPR			(&IPU_CM_REG->gpr)
#define IPU_CHA_DB_MODE_SEL(ch)	(&IPU_CM_REG->ch_db_mode_sel[ch / 32])

#define IPU_STAT		((struct ipu_stat *)(IPU_CTRL_BASE_ADDR + \
				IPU_STAT_REG_BASE))
#define IPU_CHA_CUR_BUF(ch)	(&IPU_STAT->cur_buf[ch / 32])
#define IPU_CHA_BUF0_RDY(ch)	(&IPU_STAT->ch_buf0_rdy[ch / 32])
#define IPU_CHA_BUF1_RDY(ch)	(&IPU_STAT->ch_buf1_rdy[ch / 32])
#define IPU_INT_STAT(n)	(&IPU_STAT->int_stat[(n) - 1])

#define IPU_INT_CTRL(n)		(&IPU_CM_REG->int_ctrl[(n) - 1])
#define IPU_SDMA_CTRL(n)		(&IPU_CM_REG->sdma_event[(n) - 1])

#define IDMAC_REG		((struct ipu_idmac *)(IPU_CTRL_BASE_ADDR + \
				IPU_IDMAC_REG_BASE))
#define IDMAC_CONF		(&IDMAC_REG->conf)
#define IDMAC_CHA_EN(ch)	(&IDMAC_REG->ch_en[ch / 32])
#define IDMAC_CHA_PRI(ch)	(&IDMAC_REG->ch_pri[ch / 32])
#define IDMAC_WM_EN(ch)		(&IDMAC_REG->wm_en[ch / 32])
#define IDMAC_CH_BUSY1		(&IDMAC_REG->ch_busy[0])

#define DI_REG(di)		((struct ipu_di *)(IPU_CTRL_BASE_ADDR + \
				((di == 1) ? IPU_DI1_REG_BASE : \
				IPU_DI0_REG_BASE)))
#define DI_GENERAL(di)		(&DI_REG(di)->general)
#define DI_BS_CLKGEN0(di)	(&DI_REG(di)->bs_clkgen0)
#define DI_BS_CLKGEN1(di)	(&DI_REG(di)->bs_clkgen1)

#define DI_SW_GEN0(di, gen)	(&DI_REG(di)->sw_gen0[gen - 1])
#define DI_SW_GEN1(di, gen)	(&DI_REG(di)->sw_gen1[gen - 1])
#define DI_STP_REP(di, gen)	(&DI_REG(di)->stp_rep[(gen - 1) / 2])
#define DI_SYNC_AS_GEN(di)	(&DI_REG(di)->sync_as)
#define DI_DW_GEN(di, gen)	(&DI_REG(di)->dw_gen[gen])
#define DI_DW_SET(di, gen, set)	(&DI_REG(di)->dw_set[gen + 12 * set])
#define DI_POL(di)		(&DI_REG(di)->pol)
#define DI_SCR_CONF(di)		(&DI_REG(di)->scr_conf)

#define DMFC_REG		((struct ipu_dmfc *)(IPU_CTRL_BASE_ADDR + \
				IPU_DMFC_REG_BASE))
#define DMFC_WR_CHAN		(&DMFC_REG->wr_chan)
#define DMFC_WR_CHAN_DEF	(&DMFC_REG->wr_chan_def)
#define DMFC_DP_CHAN		(&DMFC_REG->dp_chan)
#define DMFC_DP_CHAN_DEF	(&DMFC_REG->dp_chan_def)
#define DMFC_GENERAL1		(&DMFC_REG->general[0])
#define DMFC_IC_CTRL		(&DMFC_REG->ic_ctrl)


#define DC_REG			((struct ipu_dc *)(IPU_CTRL_BASE_ADDR + \
				IPU_DC_REG_BASE))
#define DC_MAP_CONF_PTR(n)	(&DC_REG->dc_map_ptr[n / 2])
#define DC_MAP_CONF_VAL(n)	(&DC_REG->dc_map_val[n / 2])


static inline struct ipu_dc_ch *dc_ch_offset(int ch)
{
	switch (ch) {
	case 0:
	case 1:
	case 2:
		return &DC_REG->dc_ch0_1_2[ch];
	case 5:
	case 6:
		return &DC_REG->dc_ch5_6[ch - 5];
	case 8:
		return &DC_REG->dc_ch8;
	case 9:
		return &DC_REG->dc_ch9;
	default:
		printf("%s: invalid channel %d\n", __func__, ch);
		return NULL;
	}

}

#define DC_RL_CH(ch, evt)	(&dc_ch_offset(ch)->rl[evt / 2])

#define DC_WR_CH_CONF(ch)	(&dc_ch_offset(ch)->wr_ch_conf)
#define DC_WR_CH_ADDR(ch)	(&dc_ch_offset(ch)->wr_ch_addr)

#define DC_WR_CH_CONF_1		DC_WR_CH_CONF(1)
#define DC_WR_CH_CONF_5		DC_WR_CH_CONF(5)

#define DC_GEN			(&DC_REG->gen)
#define DC_DISP_CONF2(disp)	(&DC_REG->disp_conf2[disp])
#define DC_STAT			(&DC_REG->stat)

#define DP_SYNC 0
#define DP_ASYNC0 0x60
#define DP_ASYNC1 0xBC

#define DP_REG			((struct ipu_dp *)(IPU_CTRL_BASE_ADDR + \
				IPU_DP_REG_BASE))
#define DP_COM_CONF(flow)	(&DP_REG->com_conf_sync)
#define DP_GRAPH_WIND_CTRL(flow) (&DP_REG->graph_wind_ctrl_sync)
#define DP_CSC_A_0(flow)	(&DP_REG->csca_sync[0])
#define DP_CSC_A_1(flow)	(&DP_REG->csca_sync[1])
#define DP_CSC_A_2(flow)	(&DP_REG->csca_sync[2])
#define DP_CSC_A_3(flow)	(&DP_REG->csca_sync[3])

#define DP_CSC_0(flow)		(&DP_REG->csc_sync[0])
#define DP_CSC_1(flow)		(&DP_REG->csc_sync[1])

/* DC template opcodes */
#define WROD(lf)		(0x18 | (lf << 1))

#endif
