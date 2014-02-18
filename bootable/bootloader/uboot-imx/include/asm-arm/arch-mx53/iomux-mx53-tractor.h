/*
 * Copyright (C) 2009-2010 Amit Kucheria <amit.kucheria@canonical.com>
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * 
 * Copyright (C) 2011/2012 TokenWireless Tech, Inc 
 */

#ifndef __MACH_IOMUX_MX53_TRACTOR_H__
#define __MACH_IOMUX_MX53_TRACTOR_H__

#include "iomux-tractor.h"

typedef struct
{
    unsigned GPR0;                // 0x0000
    unsigned GPR1;                // 0x0004
    unsigned GPR2;                // 0x0008
    unsigned OBSERVE[5];          // 0x000C~0x001C
    unsigned SW_MUX_CTL[202];     // 0x0020~0x0344
    unsigned SW_PAD_CTL[234];     // 0x0348~0x06EC
    unsigned SW_PAD_GRP_CTL[16];  // 0x06F0~0x072C
    unsigned SELECT_INPUT[94];    // 0x0730~0x08A4
} csp_iomux_mx53_regs;

typedef enum {
    DDK_IOMUX_PIN_GPIO_19                     = (0),
    DDK_IOMUX_PIN_KEY_COL0                    = (1),
    DDK_IOMUX_PIN_KEY_ROW0                    = (2),
    DDK_IOMUX_PIN_KEY_COL1                    = (3),
    DDK_IOMUX_PIN_KEY_ROW1                    = (4),
    DDK_IOMUX_PIN_KEY_COL2                    = (5),
    DDK_IOMUX_PIN_KEY_ROW2                    = (6),
    DDK_IOMUX_PIN_KEY_COL3                    = (7),
    DDK_IOMUX_PIN_KEY_ROW3                    = (8),
    DDK_IOMUX_PIN_KEY_COL4                    = (9),
    DDK_IOMUX_PIN_KEY_ROW4                    = (10),
    DDK_IOMUX_PIN_DI0_DISP_CLK                = (11),
    DDK_IOMUX_PIN_DI0_PIN15                   = (12),
    DDK_IOMUX_PIN_DI0_PIN2                    = (13),
    DDK_IOMUX_PIN_DI0_PIN3                    = (14),
    DDK_IOMUX_PIN_DI0_PIN4                    = (15),
    DDK_IOMUX_PIN_DISP0_DAT0                  = (16),
    DDK_IOMUX_PIN_DISP0_DAT1                  = (17),
    DDK_IOMUX_PIN_DISP0_DAT2                  = (18),
    DDK_IOMUX_PIN_DISP0_DAT3                  = (19),
    DDK_IOMUX_PIN_DISP0_DAT4                  = (20),
    DDK_IOMUX_PIN_DISP0_DAT5                  = (21),
    DDK_IOMUX_PIN_DISP0_DAT6                  = (22),
    DDK_IOMUX_PIN_DISP0_DAT7                  = (23),
    DDK_IOMUX_PIN_DISP0_DAT8                  = (24),
    DDK_IOMUX_PIN_DISP0_DAT9                  = (25),
    DDK_IOMUX_PIN_DISP0_DAT10                 = (26),
    DDK_IOMUX_PIN_DISP0_DAT11                 = (27),
    DDK_IOMUX_PIN_DISP0_DAT12                 = (28),
    DDK_IOMUX_PIN_DISP0_DAT13                 = (29),
    DDK_IOMUX_PIN_DISP0_DAT14                 = (30),
    DDK_IOMUX_PIN_DISP0_DAT15                 = (31),
    DDK_IOMUX_PIN_DISP0_DAT16                 = (32),
    DDK_IOMUX_PIN_DISP0_DAT17                 = (33),
    DDK_IOMUX_PIN_DISP0_DAT18                 = (34),
    DDK_IOMUX_PIN_DISP0_DAT19                 = (35),
    DDK_IOMUX_PIN_DISP0_DAT20                 = (36),
    DDK_IOMUX_PIN_DISP0_DAT21                 = (37),
    DDK_IOMUX_PIN_DISP0_DAT22                 = (38),
    DDK_IOMUX_PIN_DISP0_DAT23                 = (39),
    DDK_IOMUX_PIN_CSI0_PIXCLK                 = (40),
    DDK_IOMUX_PIN_CSI0_MCLK                   = (41),
    DDK_IOMUX_PIN_CSI0_DATA_EN                = (42),
    DDK_IOMUX_PIN_CSI0_VSYNC                  = (43),
    DDK_IOMUX_PIN_CSI0_DAT4                   = (44),
    DDK_IOMUX_PIN_CSI0_DAT5                   = (45),
    DDK_IOMUX_PIN_CSI0_DAT6                   = (46),
    DDK_IOMUX_PIN_CSI0_DAT7                   = (47),
    DDK_IOMUX_PIN_CSI0_DAT8                   = (48),
    DDK_IOMUX_PIN_CSI0_DAT9                   = (49),
    DDK_IOMUX_PIN_CSI0_DAT10                  = (50),
    DDK_IOMUX_PIN_CSI0_DAT11                  = (51),
    DDK_IOMUX_PIN_CSI0_DAT12                  = (52),
    DDK_IOMUX_PIN_CSI0_DAT13                  = (53),
    DDK_IOMUX_PIN_CSI0_DAT14                  = (54),
    DDK_IOMUX_PIN_CSI0_DAT15                  = (55),
    DDK_IOMUX_PIN_CSI0_DAT16                  = (56),
    DDK_IOMUX_PIN_CSI0_DAT17                  = (57),
    DDK_IOMUX_PIN_CSI0_DAT18                  = (58),
    DDK_IOMUX_PIN_CSI0_DAT19                  = (59),
    DDK_IOMUX_PIN_EIM_A25                     = (60),
    DDK_IOMUX_PIN_EIM_EB2                     = (61),
    DDK_IOMUX_PIN_EIM_D16                     = (62),
    DDK_IOMUX_PIN_EIM_D17                     = (63),
    DDK_IOMUX_PIN_EIM_D18                     = (64),
    DDK_IOMUX_PIN_EIM_D19                     = (65),
    DDK_IOMUX_PIN_EIM_D20                     = (66),
    DDK_IOMUX_PIN_EIM_D21                     = (67),
    DDK_IOMUX_PIN_EIM_D22                     = (68),
    DDK_IOMUX_PIN_EIM_D23                     = (69),
    DDK_IOMUX_PIN_EIM_EB3                     = (70),
    DDK_IOMUX_PIN_EIM_D24                     = (71),
    DDK_IOMUX_PIN_EIM_D25                     = (72),
    DDK_IOMUX_PIN_EIM_D26                     = (73),
    DDK_IOMUX_PIN_EIM_D27                     = (74),
    DDK_IOMUX_PIN_EIM_D28                     = (75),
    DDK_IOMUX_PIN_EIM_D29                     = (76),
    DDK_IOMUX_PIN_EIM_D30                     = (77),
    DDK_IOMUX_PIN_EIM_D31                     = (78),
    DDK_IOMUX_PIN_EIM_A24                     = (79),
    DDK_IOMUX_PIN_EIM_A23                     = (80),
    DDK_IOMUX_PIN_EIM_A22                     = (81),
    DDK_IOMUX_PIN_EIM_A21                     = (82),
    DDK_IOMUX_PIN_EIM_A20                     = (83),
    DDK_IOMUX_PIN_EIM_A19                     = (84),
    DDK_IOMUX_PIN_EIM_A18                     = (85),
    DDK_IOMUX_PIN_EIM_A17                     = (86),
    DDK_IOMUX_PIN_EIM_A16                     = (87),
    DDK_IOMUX_PIN_EIM_CS0                     = (88),
    DDK_IOMUX_PIN_EIM_CS1                     = (89),
    DDK_IOMUX_PIN_EIM_OE                      = (90),
    DDK_IOMUX_PIN_EIM_RW                      = (91),
    DDK_IOMUX_PIN_EIM_LBA                     = (92),
    DDK_IOMUX_PIN_EIM_EB0                     = (93),
    DDK_IOMUX_PIN_EIM_EB1                     = (94),
    DDK_IOMUX_PIN_EIM_DA0                     = (95),
    DDK_IOMUX_PIN_EIM_DA1                     = (96),
    DDK_IOMUX_PIN_EIM_DA2                     = (97),
    DDK_IOMUX_PIN_EIM_DA3                     = (98),
    DDK_IOMUX_PIN_EIM_DA4                     = (99),
    DDK_IOMUX_PIN_EIM_DA5                     = (100),
    DDK_IOMUX_PIN_EIM_DA6                     = (101),
    DDK_IOMUX_PIN_EIM_DA7                     = (102),
    DDK_IOMUX_PIN_EIM_DA8                     = (103),
    DDK_IOMUX_PIN_EIM_DA9                     = (104),
    DDK_IOMUX_PIN_EIM_DA10                    = (105),
    DDK_IOMUX_PIN_EIM_DA11                    = (106),
    DDK_IOMUX_PIN_EIM_DA12                    = (107),
    DDK_IOMUX_PIN_EIM_DA13                    = (108),
    DDK_IOMUX_PIN_EIM_DA14                    = (109),
    DDK_IOMUX_PIN_EIM_DA15                    = (110),
    DDK_IOMUX_PIN_NANDF_WE_B                  = (111),
    DDK_IOMUX_PIN_NANDF_RE_B                  = (112),
    DDK_IOMUX_PIN_EIM_WAIT                    = (113),
    DDK_IOMUX_PIN_EIM_BCLK                    = (114),
    DDK_IOMUX_PIN_LVDS1_TX3_P                 = (115),
    DDK_IOMUX_PIN_LVDS1_TX2_P                 = (116),
    DDK_IOMUX_PIN_LVDS1_CLK_P                 = (117),
    DDK_IOMUX_PIN_LVDS1_TX1_P                 = (118),
    DDK_IOMUX_PIN_LVDS1_TX0_P                 = (119),
    DDK_IOMUX_PIN_LVDS0_TX3_P                 = (120),
    DDK_IOMUX_PIN_LVDS0_CLK_P                 = (121),
    DDK_IOMUX_PIN_LVDS0_TX2_P                 = (122),
    DDK_IOMUX_PIN_LVDS0_TX1_P                 = (123),
    DDK_IOMUX_PIN_LVDS0_TX0_P                 = (124),
    DDK_IOMUX_PIN_GPIO_10                     = (125),
    DDK_IOMUX_PIN_GPIO_11                     = (126),
    DDK_IOMUX_PIN_GPIO_12                     = (127),
    DDK_IOMUX_PIN_GPIO_13                     = (128),
    DDK_IOMUX_PIN_GPIO_14                     = (129),
    DDK_IOMUX_PIN_NANDF_CLE                   = (130),
    DDK_IOMUX_PIN_NANDF_ALE                   = (131),
    DDK_IOMUX_PIN_NANDF_WP_B                  = (132),
    DDK_IOMUX_PIN_NANDF_RB0                   = (133),
    DDK_IOMUX_PIN_NANDF_CS0                   = (134),
    DDK_IOMUX_PIN_NANDF_CS1                   = (135),
    DDK_IOMUX_PIN_NANDF_CS2                   = (136),
    DDK_IOMUX_PIN_NANDF_CS3                   = (137),
    DDK_IOMUX_PIN_FEC_MDIO                    = (138),
    DDK_IOMUX_PIN_FEC_REF_CLK                 = (139),
    DDK_IOMUX_PIN_FEC_RX_ER                   = (140),
    DDK_IOMUX_PIN_FEC_CRS_DV                  = (141),
    DDK_IOMUX_PIN_FEC_RXD1                    = (142),
    DDK_IOMUX_PIN_FEC_RXD0                    = (143),
    DDK_IOMUX_PIN_FEC_TX_EN                   = (144),
    DDK_IOMUX_PIN_FEC_TXD1                    = (145),
    DDK_IOMUX_PIN_FEC_TXD0                    = (146),
    DDK_IOMUX_PIN_FEC_MDC                     = (147),
    DDK_IOMUX_PIN_PATA_DIOW                   = (148),
    DDK_IOMUX_PIN_PATA_DMACK                  = (149),
    DDK_IOMUX_PIN_PATA_DMARQ                  = (150),
    DDK_IOMUX_PIN_PATA_BUFFER_EN              = (151),
    DDK_IOMUX_PIN_PATA_INTRQ                  = (152),
    DDK_IOMUX_PIN_PATA_DIOR                   = (153),
    DDK_IOMUX_PIN_PATA_RESET_B                = (154),
    DDK_IOMUX_PIN_PATA_IORDY                  = (155),
    DDK_IOMUX_PIN_PATA_DA_0                   = (156),
    DDK_IOMUX_PIN_PATA_DA_1                   = (157),
    DDK_IOMUX_PIN_PATA_DA_2                   = (158),
    DDK_IOMUX_PIN_PATA_CS_0                   = (159),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_CS_1    = (160),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA0   = (161),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA1   = (162),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA2   = (163),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA3   = (164),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA4   = (165),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA5   = (166),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA6   = (167),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA7   = (168),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA8   = (169),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA9   = (170),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA10  = (171),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA11  = (172),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA12  = (173),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA13  = (174),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA14  = (175),    
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_PATA_DATA15  = (176),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD1_DATA0    = (177),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD1_DATA1    = (178),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD1_CMD      = (179),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD1_DATA2    = (180),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD1_CLK      = (181),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD1_DATA3    = (182),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD2_CLK      = (183),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD2_CMD      = (184),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD2_DATA3    = (185),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD2_DATA2    = (186),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD2_DATA1    = (187),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_SD2_DATA0    = (188),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_0       = (189),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_1       = (190),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_9       = (191),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_3       = (192),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_6       = (193),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_2       = (194),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_4       = (195),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_5       = (196),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_7       = (197),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_8       = (198),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_16      = (199),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_17      = (200),
    DDK_IOMUX_PIN_SW_MUX_CTL_PAD_GPIO_18      = (201),
} ddk_iomux_pin;


//-----------------------------------------------------------------------------
//
//  Type: DDK_IOMUX_SELECT_INPUT
//
//  Specifies the ports that have select input configuration.
//
//-----------------------------------------------------------------------------
typedef enum {
    DDK_IOMUX_AUDMUX_P4_INPUT_DA_AMX_SELECT_INPUT     = (0),
    DDK_IOMUX_AUDMUX_P4_INPUT_DB_AMX_SELECT_INPUT     = (1),
    DDK_IOMUX_AUDMUX_P4_INPUT_RXCLK_AMX_SELECT_INPUT  = (2),
    DDK_IOMUX_AUDMUX_P4_INPUT_RXFS_AMX_SELECT_INPUT   = (3),
    DDK_IOMUX_AUDMUX_P4_INPUT_TXCLK_AMX_SELECT_INPUT  = (4),
    DDK_IOMUX_AUDMUX_P4_INPUT_TXFS_AMX_SELECT_INPUT   = (5),
    DDK_IOMUX_AUDMUX_P5_INPUT_DA_AMX_SELECT_INPUT     = (6),
    DDK_IOMUX_AUDMUX_P5_INPUT_DB_AMX_SELECT_INPUT     = (7),
    DDK_IOMUX_AUDMUX_P5_INPUT_RXCLK_AMX_SELECT_INPUT  = (8),
    DDK_IOMUX_AUDMUX_P5_INPUT_RXFS_AMX_SELECT_INPUT   = (9),
    DDK_IOMUX_AUDMUX_P5_INPUT_TXCLK_AMX_SELECT_INPUT  = (10),
    DDK_IOMUX_AUDMUX_P5_INPUT_TXFS_AMX_SELECT_INPUT   = (11),
    DDK_IOMUX_CAN1_IPP_IND_CANRX_SELECT_INPUT         = (12),
    DDK_IOMUX_CAN2_IPP_IND_CANRX_SELECT_INPUT         = (13),
    DDK_IOMUX_CCM_IPP_ASRC_EXT_SELECT_INPUT           = (14),
    DDK_IOMUX_CCM_IPP_DI1_CLK_SELECT_INPUT            = (15),
    DDK_IOMUX_CCM_PLL1_BYPASS_CLK_SELECT_INPUT        = (16),
    DDK_IOMUX_CCM_PLL2_BYPASS_CLK_SELECT_INPUT        = (17),
    DDK_IOMUX_CCM_PLL3_BYPASS_CLK_SELECT_INPUT        = (18),
    DDK_IOMUX_CCM_PLL4_BYPASS_CLK_SELECT_INPUT        = (19),
    DDK_IOMUX_CSPI_IPP_CSPI_CLK_IN_SELECT_INPUT       = (20),
    DDK_IOMUX_CSPI_IPP_IND_MISO_SELECT_INPUT          = (21),
    DDK_IOMUX_CSPI_IPP_IND_MOSI_SELECT_INPUT          = (22),
    DDK_IOMUX_CSPI_IPP_IND_SS0_B_SELECT_INPUT         = (23),
    DDK_IOMUX_CSPI_IPP_IND_SS1_B_SELECT_INPUT         = (24),
    DDK_IOMUX_CSPI_IPP_IND_SS2_B_SELECT_INPUT         = (25),
    DDK_IOMUX_CSPI_IPP_IND_SS3_B_SELECT_INPUT         = (26),
    DDK_IOMUX_ECSPI1_IPP_CSPI_CLK_IN_SELECT_INPUT     = (27),
    DDK_IOMUX_ECSPI1_IPP_IND_MISO_SELECT_INPUT        = (28),
    DDK_IOMUX_ECSPI1_IPP_IND_MOSI_SELECT_INPUT        = (29),
    DDK_IOMUX_ECSPI1_IPP_IND_SS_B_0_SELECT_INPUT      = (30),
    DDK_IOMUX_ECSPI1_IPP_IND_SS_B_1_SELECT_INPUT      = (31),
    DDK_IOMUX_ECSPI1_IPP_IND_SS_B_2_SELECT_INPUT      = (32),
    DDK_IOMUX_ECSPI1_IPP_IND_SS_B_3_SELECT_INPUT      = (33),
    DDK_IOMUX_ECSPI2_IPP_CSPI_CLK_IN_SELECT_INPUT     = (34),
    DDK_IOMUX_ECSPI2_IPP_IND_MISO_SELECT_INPUT        = (35),
    DDK_IOMUX_ECSPI2_IPP_IND_MOSI_SELECT_INPUT        = (36),
    DDK_IOMUX_ECSPI2_IPP_IND_SS_B_0_SELECT_INPUT      = (37),
    DDK_IOMUX_ECSPI2_IPP_IND_SS_B_1_SELECT_INPUT      = (38),
    DDK_IOMUX_ESAI1_IPP_IND_FSR_SELECT_INPUT          = (39),
    DDK_IOMUX_ESAI1_IPP_IND_FST_SELECT_INPUT          = (40),
    DDK_IOMUX_ESAI1_IPP_IND_HCKR_SELECT_INPUT         = (41),
    DDK_IOMUX_ESAI1_IPP_IND_HCKT_SELECT_INPUT         = (42),
    DDK_IOMUX_ESAI1_IPP_IND_SCKR_SELECT_INPUT         = (43),
    DDK_IOMUX_ESAI1_IPP_IND_SCKT_SELECT_INPUT         = (44),
    DDK_IOMUX_ESAI1_IPP_IND_SDO0_SELECT_INPUT         = (45),
    DDK_IOMUX_ESAI1_IPP_IND_SDO1_SELECT_INPUT         = (46),
    DDK_IOMUX_ESAI1_IPP_IND_SDO2_SDI3_SELECT_INPUT    = (47),
    DDK_IOMUX_ESAI1_IPP_IND_SDO3_SDI2_SELECT_INPUT    = (48),
    DDK_IOMUX_ESAI1_IPP_IND_SDO4_SDI1_SELECT_INPUT    = (49),
    DDK_IOMUX_ESAI1_IPP_IND_SDO5_SDI0_SELECT_INPUT    = (50),
    DDK_IOMUX_ESDHC1_IPP_WP_ON_SELECT_INPUT           = (51),
    DDK_IOMUX_FEC_FEC_COL_SELECT_INPUT                = (52),
    DDK_IOMUX_FEC_FEC_MDI_SELECT_INPUT                = (53),
    DDK_IOMUX_FEC_FEC_RX_CLK_SELECT_INPUT             = (54),
    DDK_IOMUX_FIRI_IPP_IND_RXD_SELECT_INPUT           = (55),
    DDK_IOMUX_GPC_PMIC_RDY_SELECT_INPUT               = (56),
    DDK_IOMUX_I2C1_IPP_SCL_IN_SELECT_INPUT            = (57),
    DDK_IOMUX_I2C1_IPP_SDA_IN_SELECT_INPUT            = (58),
    DDK_IOMUX_I2C2_IPP_SCL_IN_SELECT_INPUT            = (59),
    DDK_IOMUX_I2C2_IPP_SDA_IN_SELECT_INPUT            = (60),
    DDK_IOMUX_I2C3_IPP_SCL_IN_SELECT_INPUT            = (61),
    DDK_IOMUX_I2C3_IPP_SDA_IN_SELECT_INPUT            = (62),
    DDK_IOMUX_IPU_IPP_DI_0_IND_DISPB_SD_D_SELECT_INPUT= (63),
    DDK_IOMUX_IPU_IPP_DI_1_IND_DISPB_SD_D_SELECT_INPUT= (64),
    DDK_IOMUX_IPU_IPP_IND_SENS1_DATA_EN_SELECT_INPUT  = (65),
    DDK_IOMUX_IPU_IPP_IND_SENS1_HSYNC_SELECT_INPUT    = (66),
    DDK_IOMUX_IPU_IPP_IND_SENS1_VSYNC_SELECT_INPUT    = (67),
    DDK_IOMUX_KPP_IPP_IND_COL_5_SELECT_INPUT          = (68),
    DDK_IOMUX_KPP_IPP_IND_COL_6_SELECT_INPUT          = (69),
    DDK_IOMUX_KPP_IPP_IND_COL_7_SELECT_INPUT          = (70),
    DDK_IOMUX_KPP_IPP_IND_ROW_5_SELECT_INPUT          = (71),
    DDK_IOMUX_KPP_IPP_IND_ROW_6_SELECT_INPUT          = (72),
    DDK_IOMUX_KPP_IPP_IND_ROW_7_SELECT_INPUT          = (73),
    DDK_IOMUX_MLB_MLBCLK_IN_SELECT_INPUT              = (74),
    DDK_IOMUX_MLB_MLBDAT_IN_SELECT_INPUT              = (75),
    DDK_IOMUX_MLB_MLBSIG_IN_SELECT_INPUT              = (76),
    DDK_IOMUX_OWIRE_BATTERY_LINE_IN_SELECT_INPUT      = (77),
    DDK_IOMUX_SDMA_EVENTS_14_SELECT_INPUT             = (78),
    DDK_IOMUX_SDMA_EVENTS_15_SELECT_INPUT             = (79),
    DDK_IOMUX_SPDIF_SPDIF_IN1_SELECT_INPUT            = (80),
    DDK_IOMUX_UART1_IPP_UART_RTS_B_SELECT_INPUT       = (81),
    DDK_IOMUX_UART1_IPP_UART_RXD_MUX_SELECT_INPUT     = (82),
    DDK_IOMUX_UART2_IPP_UART_RTS_B_SELECT_INPUT       = (83),
    DDK_IOMUX_UART2_IPP_UART_RXD_MUX_SELECT_INPUT     = (84),
    DDK_IOMUX_UART3_IPP_UART_RTS_B_SELECT_INPUT       = (85),
    DDK_IOMUX_UART3_IPP_UART_RXD_MUX_SELECT_INPUT     = (86),
    DDK_IOMUX_UART4_IPP_UART_RTS_B_SELECT_INPUT       = (87),
    DDK_IOMUX_UART4_IPP_UART_RXD_MUX_SELECT_INPUT     = (88),
    DDK_IOMUX_UART5_IPP_UART_RTS_B_SELECT_INPUT       = (89),
    DDK_IOMUX_UART5_IPP_UART_RXD_MUX_SELECT_INPUT     = (90),
    DDK_IOMUX_USBOH3_IPP_IND_OTG_OC_SELECT_INPUT      = (91),
    DDK_IOMUX_USBOH3_IPP_IND_UH1_OC_SELECT_INPUT      = (92),
    DDK_IOMUX_USBOH3_IPP_IND_UH2_OC_SELECT_INPUT      = (93),
} ddk_iomux_select_input;


//-----------------------------------------------------------------------------
//
//  Type: DDK_IOMUX_PAD
//
//  Specifies the functional pad name used to configure the IOMUX SW_PAD_CTL.
//
//-----------------------------------------------------------------------------
typedef enum {
    DDK_IOMUX_PAD_GPIO_19          = (0),
    DDK_IOMUX_PAD_KEY_COL0         = (1),
    DDK_IOMUX_PAD_KEY_ROW0         = (2),
    DDK_IOMUX_PAD_KEY_COL1         = (3),
    DDK_IOMUX_PAD_KEY_ROW1         = (4),
    DDK_IOMUX_PAD_KEY_COL2         = (5),
    DDK_IOMUX_PAD_KEY_ROW2         = (6),
    DDK_IOMUX_PAD_KEY_COL3         = (7),
    DDK_IOMUX_PAD_KEY_ROW3         = (8),
    DDK_IOMUX_PAD_KEY_COL4         = (9),
    DDK_IOMUX_PAD_KEY_ROW4         = (10),
    DDK_IOMUX_PAD_NVCC_KEYPAD      = (11),
    DDK_IOMUX_PAD_DI0_DISP_CLK     = (12),
    DDK_IOMUX_PAD_DI0_PIN15        = (13),
    DDK_IOMUX_PAD_DI0_PIN2         = (14),
    DDK_IOMUX_PAD_DI0_PIN3         = (15),
    DDK_IOMUX_PAD_DI0_PIN4         = (16),
    DDK_IOMUX_PAD_DISP0_DAT0       = (17),
    DDK_IOMUX_PAD_DISP0_DAT1       = (18),
    DDK_IOMUX_PAD_DISP0_DAT2       = (19),
    DDK_IOMUX_PAD_DISP0_DAT3       = (20),
    DDK_IOMUX_PAD_DISP0_DAT4       = (21),
    DDK_IOMUX_PAD_DISP0_DAT5       = (22),
    DDK_IOMUX_PAD_DISP0_DAT6       = (23),
    DDK_IOMUX_PAD_DISP0_DAT7       = (24),
    DDK_IOMUX_PAD_DISP0_DAT8       = (25),
    DDK_IOMUX_PAD_DISP0_DAT9       = (26),
    DDK_IOMUX_PAD_DISP0_DAT10      = (27),
    DDK_IOMUX_PAD_DISP0_DAT11      = (28),
    DDK_IOMUX_PAD_DISP0_DAT12      = (29),
    DDK_IOMUX_PAD_DISP0_DAT13      = (30),
    DDK_IOMUX_PAD_DISP0_DAT14      = (31),
    DDK_IOMUX_PAD_DISP0_DAT15      = (32),
    DDK_IOMUX_PAD_DISP0_DAT16      = (33),
    DDK_IOMUX_PAD_DISP0_DAT17      = (34),
    DDK_IOMUX_PAD_DISP0_DAT18      = (35),
    DDK_IOMUX_PAD_DISP0_DAT19      = (36),
    DDK_IOMUX_PAD_DISP0_DAT20      = (37),
    DDK_IOMUX_PAD_DISP0_DAT21      = (38),
    DDK_IOMUX_PAD_DISP0_DAT22      = (39),
    DDK_IOMUX_PAD_DISP0_DAT23      = (40),
    DDK_IOMUX_PAD_CSI0_PIXCLK      = (41),
    DDK_IOMUX_PAD_CSI0_MCLK        = (42),
    DDK_IOMUX_PAD_CSI0_DATA_EN     = (43),
    DDK_IOMUX_PAD_CSI0_VSYNC       = (44),
    DDK_IOMUX_PAD_CSI0_DAT4        = (45),
    DDK_IOMUX_PAD_CSI0_DAT5        = (46),
    DDK_IOMUX_PAD_CSI0_DAT6        = (47),
    DDK_IOMUX_PAD_CSI0_DAT7        = (48),
    DDK_IOMUX_PAD_CSI0_DAT8        = (49),
    DDK_IOMUX_PAD_CSI0_DAT9        = (50),
    DDK_IOMUX_PAD_CSI0_DAT10       = (51),
    DDK_IOMUX_PAD_CSI0_DAT11       = (52),
    DDK_IOMUX_PAD_CSI0_DAT12       = (53),
    DDK_IOMUX_PAD_CSI0_DAT13       = (54),
    DDK_IOMUX_PAD_CSI0_DAT14       = (55),
    DDK_IOMUX_PAD_CSI0_DAT15       = (56),
    DDK_IOMUX_PAD_CSI0_DAT16       = (57),
    DDK_IOMUX_PAD_CSI0_DAT17       = (58),
    DDK_IOMUX_PAD_CSI0_DAT18       = (59),
    DDK_IOMUX_PAD_CSI0_DAT19       = (60),
    DDK_IOMUX_PAD_NVCC_CSI__0      = (61),
    DDK_IOMUX_PAD_JTAG_TMS         = (62),
    DDK_IOMUX_PAD_JTAG_MOD         = (63),
    DDK_IOMUX_PAD_JTAG_TRSTB       = (64),
    DDK_IOMUX_PAD_JTAG_TDI         = (65),
    DDK_IOMUX_PAD_JTAG_TCK         = (66),
    DDK_IOMUX_PAD_JTAG_TDO         = (67),
    DDK_IOMUX_PAD_EIM_A25          = (68),
    DDK_IOMUX_PAD_EIM_EB2          = (69),
    DDK_IOMUX_PAD_EIM_D16          = (70),
    DDK_IOMUX_PAD_EIM_D17          = (71),
    DDK_IOMUX_PAD_EIM_D18          = (72),
    DDK_IOMUX_PAD_EIM_D19          = (73),
    DDK_IOMUX_PAD_EIM_D20          = (74),
    DDK_IOMUX_PAD_EIM_D21          = (75),
    DDK_IOMUX_PAD_EIM_D22          = (76),
    DDK_IOMUX_PAD_EIM_D23          = (77),
    DDK_IOMUX_PAD_EIM_EB3          = (78),
    DDK_IOMUX_PAD_EIM_D24          = (79),
    DDK_IOMUX_PAD_EIM_D25          = (80),
    DDK_IOMUX_PAD_EIM_D26          = (81),
    DDK_IOMUX_PAD_EIM_D27          = (82),
    DDK_IOMUX_PAD_EIM_D28          = (83),
    DDK_IOMUX_PAD_EIM_D29          = (84),
    DDK_IOMUX_PAD_EIM_D30          = (85),
    DDK_IOMUX_PAD_EIM_D31          = (86),
    DDK_IOMUX_PAD_NVCC_EIM__1      = (87),
    DDK_IOMUX_PAD_EIM_A24          = (88),
    DDK_IOMUX_PAD_EIM_A23          = (89),
    DDK_IOMUX_PAD_EIM_A22          = (90),
    DDK_IOMUX_PAD_EIM_A21          = (91),
    DDK_IOMUX_PAD_EIM_A20          = (92),
    DDK_IOMUX_PAD_EIM_A19          = (93),
    DDK_IOMUX_PAD_EIM_A18          = (94),
    DDK_IOMUX_PAD_EIM_A17          = (95),
    DDK_IOMUX_PAD_EIM_A16          = (96),
    DDK_IOMUX_PAD_EIM_CS0          = (97),
    DDK_IOMUX_PAD_EIM_CS1          = (98),
    DDK_IOMUX_PAD_EIM_OE           = (99),
    DDK_IOMUX_PAD_EIM_RW           = (100),
    DDK_IOMUX_PAD_EIM_LBA          = (101),
    DDK_IOMUX_PAD_NVCC_EIM__4      = (102),
    DDK_IOMUX_PAD_EIM_EB0          = (103),
    DDK_IOMUX_PAD_EIM_EB1          = (104),
    DDK_IOMUX_PAD_EIM_DA0          = (105),
    DDK_IOMUX_PAD_EIM_DA1          = (106),
    DDK_IOMUX_PAD_EIM_DA2          = (107),
    DDK_IOMUX_PAD_EIM_DA3          = (108),
    DDK_IOMUX_PAD_EIM_DA4          = (109),
    DDK_IOMUX_PAD_EIM_DA5          = (110),
    DDK_IOMUX_PAD_EIM_DA6          = (111),
    DDK_IOMUX_PAD_EIM_DA7          = (112),
    DDK_IOMUX_PAD_EIM_DA8          = (113),
    DDK_IOMUX_PAD_EIM_DA9          = (114),
    DDK_IOMUX_PAD_EIM_DA10         = (115),
    DDK_IOMUX_PAD_EIM_DA11         = (116),
    DDK_IOMUX_PAD_EIM_DA12         = (117),
    DDK_IOMUX_PAD_EIM_DA13         = (118),
    DDK_IOMUX_PAD_EIM_DA14         = (119),
    DDK_IOMUX_PAD_EIM_DA15         = (120),
    DDK_IOMUX_PAD_NANDF_WE_B       = (121),
    DDK_IOMUX_PAD_NANDF_RE_B       = (122),
    DDK_IOMUX_PAD_EIM_WAIT         = (123),
    DDK_IOMUX_PAD_EIM_BCLK         = (124),
    DDK_IOMUX_PAD_NVCC_EIM__7      = (125),
    DDK_IOMUX_PAD_GPIO_10          = (126),
    DDK_IOMUX_PAD_GPIO_11          = (127),
    DDK_IOMUX_PAD_GPIO_12          = (128),
    DDK_IOMUX_PAD_GPIO_13          = (129),
    DDK_IOMUX_PAD_GPIO_14          = (130),
    DDK_IOMUX_PAD_DRAM_DQM3        = (131),
    DDK_IOMUX_PAD_DRAM_SDQS3       = (132),
    DDK_IOMUX_PAD_DRAM_SDCKE1      = (133),
    DDK_IOMUX_PAD_DRAM_DQM2        = (134),
    DDK_IOMUX_PAD_DRAM_SDODT1      = (135),
    DDK_IOMUX_PAD_DRAM_SDQS2       = (136),
    DDK_IOMUX_PAD_DRAM_RESET       = (137),
    DDK_IOMUX_PAD_DRAM_SDCLK_1     = (138),
    DDK_IOMUX_PAD_DRAM_CAS         = (139),
    DDK_IOMUX_PAD_DRAM_SDCLK_0     = (140),
    DDK_IOMUX_PAD_DRAM_SDQS0       = (141),
    DDK_IOMUX_PAD_DRAM_SDODT0      = (142),
    DDK_IOMUX_PAD_DRAM_DQM0        = (143),
    DDK_IOMUX_PAD_DRAM_RAS         = (144),
    DDK_IOMUX_PAD_DRAM_SDCKE0      = (145),
    DDK_IOMUX_PAD_DRAM_SDQS1       = (146),
    DDK_IOMUX_PAD_DRAM_DQM1        = (147),
    DDK_IOMUX_PAD_PMIC_ON_REQ      = (148),
    DDK_IOMUX_PAD_PMIC_STBY_REQ    = (149),
    DDK_IOMUX_PAD_NANDF_CLE        = (150),
    DDK_IOMUX_PAD_NANDF_ALE        = (151),
    DDK_IOMUX_PAD_NANDF_WP_B       = (152),
    DDK_IOMUX_PAD_NANDF_RB0        = (153),
    DDK_IOMUX_PAD_NANDF_CS0        = (154),
    DDK_IOMUX_PAD_NANDF_CS1        = (155),
    DDK_IOMUX_PAD_NANDF_CS2        = (156),
    DDK_IOMUX_PAD_NANDF_CS3        = (157),
    DDK_IOMUX_PAD_NVCC_NANDF       = (158),
    DDK_IOMUX_PAD_FEC_MDIO         = (159),
    DDK_IOMUX_PAD_FEC_REF_CLK      = (160),
    DDK_IOMUX_PAD_FEC_RX_ER        = (161),
    DDK_IOMUX_PAD_FEC_CRS_DV       = (162),
    DDK_IOMUX_PAD_FEC_RXD1         = (163),
    DDK_IOMUX_PAD_FEC_RXD0         = (164),
    DDK_IOMUX_PAD_FEC_TX_EN        = (165),
    DDK_IOMUX_PAD_FEC_TXD1         = (166),
    DDK_IOMUX_PAD_FEC_TXD0         = (167),
    DDK_IOMUX_PAD_FEC_MDC          = (168),
    DDK_IOMUX_PAD_NVCC_FEC         = (169),
    DDK_IOMUX_PAD_PATA_DIOW        = (170),
    DDK_IOMUX_PAD_PATA_DMACK       = (171),
    DDK_IOMUX_PAD_PATA_DMARQ       = (172),
    DDK_IOMUX_PAD_PATA_BUFFER_EN   = (173),
    DDK_IOMUX_PAD_PATA_INTRQ       = (174),
    DDK_IOMUX_PAD_PATA_DIOR        = (175),
    DDK_IOMUX_PAD_PATA_RESET_B     = (176),
    DDK_IOMUX_PAD_PATA_IORDY       = (177),
    DDK_IOMUX_PAD_PATA_DA_0        = (178),
    DDK_IOMUX_PAD_PATA_DA_1        = (179),
    DDK_IOMUX_PAD_PATA_DA_2        = (180),
    DDK_IOMUX_PAD_PATA_CS_0        = (181),
    DDK_IOMUX_PAD_PATA_CS_1        = (182),
    DDK_IOMUX_PAD_NVCC_PATA__2     = (183),
    DDK_IOMUX_PAD_PATA_DATA0       = (184),
    DDK_IOMUX_PAD_PATA_DATA1       = (185),
    DDK_IOMUX_PAD_PATA_DATA2       = (186),
    DDK_IOMUX_PAD_PATA_DATA3       = (187),
    DDK_IOMUX_PAD_PATA_DATA4       = (188),
    DDK_IOMUX_PAD_PATA_DATA5       = (189),
    DDK_IOMUX_PAD_PATA_DATA6       = (190),
    DDK_IOMUX_PAD_PATA_DATA7       = (191),
    DDK_IOMUX_PAD_PATA_DATA8       = (192),
    DDK_IOMUX_PAD_PATA_DATA9       = (193),
    DDK_IOMUX_PAD_PATA_DATA10      = (194),
    DDK_IOMUX_PAD_PATA_DATA11      = (195),
    DDK_IOMUX_PAD_PATA_DATA12      = (196),
    DDK_IOMUX_PAD_PATA_DATA13      = (197),
    DDK_IOMUX_PAD_PATA_DATA14      = (198),
    DDK_IOMUX_PAD_PATA_DATA15      = (199),
    DDK_IOMUX_PAD_NVCC_PATA__0     = (200),
    DDK_IOMUX_PAD_SD1_DATA0        = (201),
    DDK_IOMUX_PAD_SD1_DATA1        = (202),
    DDK_IOMUX_PAD_SD1_CMD          = (203),
    DDK_IOMUX_PAD_SD1_DATA2        = (204),
    DDK_IOMUX_PAD_SD1_CLK          = (205),
    DDK_IOMUX_PAD_SD1_DATA3        = (206),
    DDK_IOMUX_PAD_NVCC_SD1         = (207),
    DDK_IOMUX_PAD_SD2_CLK          = (208),
    DDK_IOMUX_PAD_SD2_CMD          = (209),
    DDK_IOMUX_PAD_SD2_DATA3        = (210),
    DDK_IOMUX_PAD_SD2_DATA2        = (211),
    DDK_IOMUX_PAD_SD2_DATA1        = (212),
    DDK_IOMUX_PAD_SD2_DATA0        = (213),
    DDK_IOMUX_PAD_NVCC_SD2         = (214),
    DDK_IOMUX_PAD_GPIO_0           = (215),
    DDK_IOMUX_PAD_GPIO_1           = (216),
    DDK_IOMUX_PAD_GPIO_9           = (217),
    DDK_IOMUX_PAD_GPIO_3           = (218),
    DDK_IOMUX_PAD_GPIO_6           = (219),
    DDK_IOMUX_PAD_GPIO_2           = (220),
    DDK_IOMUX_PAD_GPIO_4           = (221),
    DDK_IOMUX_PAD_GPIO_5           = (222),
    DDK_IOMUX_PAD_GPIO_7           = (223),
    DDK_IOMUX_PAD_GPIO_8           = (224),
    DDK_IOMUX_PAD_GPIO_16          = (225),
    DDK_IOMUX_PAD_GPIO_17          = (226),
    DDK_IOMUX_PAD_GPIO_18          = (227),
    DDK_IOMUX_PAD_NVCC_GPIO        = (228),
    DDK_IOMUX_PAD_POR_B            = (229),
    DDK_IOMUX_PAD_BOOT_MODE1       = (230),
    DDK_IOMUX_PAD_RESET_IN_B       = (231),
    DDK_IOMUX_PAD_BOOT_MODE0       = (232),
    DDK_IOMUX_PAD_TEST_MODE        = (233),
} ddk_iomux_pad;

typedef enum
{
    DDK_IOMUX_PAD_GRP_ADDDS                               = (0),
    DDK_IOMUX_PAD_GRP_DDRMODE_CTL                    = (1),
    DDK_IOMUX_PAD_GRP_DUMMY0                               = (2),
    DDK_IOMUX_PAD_GRP_DDRPKE                              = (3),
    DDK_IOMUX_PAD_GRP_DUMMY1                                = (4),
    DDK_IOMUX_PAD_GRP_DUMMY2                                = (5),
    DDK_IOMUX_PAD_GRP_DDRPK                                = (6),
    DDK_IOMUX_PAD_GRP_DUMMY3                                = (7),
    DDK_IOMUX_PAD_GRP_DDRHYS                              = (8),
    DDK_IOMUX_PAD_GRP_DDRMODE                           = (9),
    DDK_IOMUX_PAD_GRP_B0DS                                  = (10),
    DDK_IOMUX_PAD_GRP_B1DS					     = (11),
    DDK_IOMUX_PAD_GRP_CTLDS				     = (12),
    DDK_IOMUX_PAD_GRP_DDR_TYPE				     = (13),
    DDK_IOMUX_PAD_GRP_B2DS					     = (14),
    DDK_IOMUX_PAD_GRP_B3DS					     = (15),
}ddk_iomux_grp_pad;


typedef enum 
{
    DDK_IOMUX_PIN_MUXMODE_ALT0  = (IOMUX_SW_MUX_CTL_MUX_MODE_ALT0 << IOMUX_SW_MUX_CTL_MUX_MODE_LSH),
    DDK_IOMUX_PIN_MUXMODE_ALT1  = (IOMUX_SW_MUX_CTL_MUX_MODE_ALT1 << IOMUX_SW_MUX_CTL_MUX_MODE_LSH),
    DDK_IOMUX_PIN_MUXMODE_ALT2  = (IOMUX_SW_MUX_CTL_MUX_MODE_ALT2 << IOMUX_SW_MUX_CTL_MUX_MODE_LSH),
    DDK_IOMUX_PIN_MUXMODE_ALT3  = (IOMUX_SW_MUX_CTL_MUX_MODE_ALT3 << IOMUX_SW_MUX_CTL_MUX_MODE_LSH),
    DDK_IOMUX_PIN_MUXMODE_ALT4  = (IOMUX_SW_MUX_CTL_MUX_MODE_ALT4 << IOMUX_SW_MUX_CTL_MUX_MODE_LSH),
    DDK_IOMUX_PIN_MUXMODE_ALT5  = (IOMUX_SW_MUX_CTL_MUX_MODE_ALT5 << IOMUX_SW_MUX_CTL_MUX_MODE_LSH),
    DDK_IOMUX_PIN_MUXMODE_ALT6  = (IOMUX_SW_MUX_CTL_MUX_MODE_ALT6 << IOMUX_SW_MUX_CTL_MUX_MODE_LSH),
    DDK_IOMUX_PIN_MUXMODE_ALT7  = (IOMUX_SW_MUX_CTL_MUX_MODE_ALT7 << IOMUX_SW_MUX_CTL_MUX_MODE_LSH)
} ddk_iomux_pin_muxmode;

typedef enum 
{
    DDK_IOMUX_PIN_SION_REGULAR  = (IOMUX_SW_MUX_CTL_SION_REGULAR << IOMUX_SW_MUX_CTL_SION_LSH),
    DDK_IOMUX_PIN_SION_FORCE    = (IOMUX_SW_MUX_CTL_SION_FORCE << IOMUX_SW_MUX_CTL_SION_LSH)
} ddk_iomux_pin_sion;

typedef enum 
{
    DDK_IOMUX_PAD_SLEW_SLOW = (IOMUX_SW_PAD_CTL_SRE_SLOW << IOMUX_SW_PAD_CTL_SRE_LSH),
    DDK_IOMUX_PAD_SLEW_FAST = (IOMUX_SW_PAD_CTL_SRE_FAST << IOMUX_SW_PAD_CTL_SRE_LSH),
} ddk_iomux_pad_slew;

typedef enum 
{
    DDK_IOMUX_PAD_DRIVE_NORMAL  = (IOMUX_SW_PAD_CTL_DSE_NORMAL << IOMUX_SW_PAD_CTL_DSE_LSH),
    DDK_IOMUX_PAD_DRIVE_MEDIUM  = (IOMUX_SW_PAD_CTL_DSE_MEDIUM << IOMUX_SW_PAD_CTL_DSE_LSH),
    DDK_IOMUX_PAD_DRIVE_HIGH    = (IOMUX_SW_PAD_CTL_DSE_HIGH << IOMUX_SW_PAD_CTL_DSE_LSH),
    DDK_IOMUX_PAD_DRIVE_MAX     = (IOMUX_SW_PAD_CTL_DSE_MAX << IOMUX_SW_PAD_CTL_DSE_LSH)
} ddk_iomux_pad_drive;

typedef enum 
{
    DDK_IOMUX_PAD_OPENDRAIN_DISABLE = (IOMUX_SW_PAD_CTL_ODE_DISABLE << IOMUX_SW_PAD_CTL_ODE_LSH),
    DDK_IOMUX_PAD_OPENDRAIN_ENABLE  = (IOMUX_SW_PAD_CTL_ODE_ENABLE << IOMUX_SW_PAD_CTL_ODE_LSH)
} ddk_iomux_pad_opendrain;


typedef enum 
{
    DDK_IOMUX_PAD_PULL_NONE         = (IOMUX_SW_PAD_CTL_PKE_DISABLE << IOMUX_SW_PAD_CTL_PKE_LSH),
        
    DDK_IOMUX_PAD_PULL_KEEPER       = (IOMUX_SW_PAD_CTL_PUE_KEEPER << IOMUX_SW_PAD_CTL_PUE_LSH) |
                                      (IOMUX_SW_PAD_CTL_PKE_ENABLE << IOMUX_SW_PAD_CTL_PKE_LSH),
                                      
    DDK_IOMUX_PAD_PULL_DOWN_360K    = (IOMUX_SW_PAD_CTL_PUS_360K_DOWN << IOMUX_SW_PAD_CTL_PUS_LSH) |
                                      (IOMUX_SW_PAD_CTL_PKE_ENABLE << IOMUX_SW_PAD_CTL_PKE_LSH) |
                                      (IOMUX_SW_PAD_CTL_PUE_PULL << IOMUX_SW_PAD_CTL_PUE_LSH),
                                      
    DDK_IOMUX_PAD_PULL_UP_100K      = (IOMUX_SW_PAD_CTL_PUS_100K_UP << IOMUX_SW_PAD_CTL_PUS_LSH) |
                                      (IOMUX_SW_PAD_CTL_PKE_ENABLE << IOMUX_SW_PAD_CTL_PKE_LSH) |
                                      (IOMUX_SW_PAD_CTL_PUE_PULL << IOMUX_SW_PAD_CTL_PUE_LSH),
                                      
    DDK_IOMUX_PAD_PULL_UP_75K       = (IOMUX_SW_PAD_CTL_PUS_75K_UP << IOMUX_SW_PAD_CTL_PUS_LSH) |
                                      (IOMUX_SW_PAD_CTL_PKE_ENABLE << IOMUX_SW_PAD_CTL_PKE_LSH) |
                                      (IOMUX_SW_PAD_CTL_PUE_PULL << IOMUX_SW_PAD_CTL_PUE_LSH),
                                      
    DDK_IOMUX_PAD_PULL_UP_22K       = (IOMUX_SW_PAD_CTL_PUS_22K_UP << IOMUX_SW_PAD_CTL_PUS_LSH) |
                                      (IOMUX_SW_PAD_CTL_PKE_ENABLE << IOMUX_SW_PAD_CTL_PKE_LSH) |
                                      (IOMUX_SW_PAD_CTL_PUE_PULL << IOMUX_SW_PAD_CTL_PUE_LSH)
    
}ddk_iomux_pad_pull;

typedef enum 
{
    DDK_IOMUX_PAD_HYSTERESIS_DISABLE    = (IOMUX_SW_PAD_CTL_HYS_DISABLE << IOMUX_SW_PAD_CTL_HYS_LSH),
    DDK_IOMUX_PAD_HYSTERESIS_ENABLE     = (IOMUX_SW_PAD_CTL_HYS_ENABLE << IOMUX_SW_PAD_CTL_HYS_LSH)
} ddk_iomux_pad_hysteresis;

typedef enum 
{
    DDK_IOMUX_PAD_INMODE_CMOS       = (IOMUX_SW_PAD_CTL_DDR_INPUT_CMOS << IOMUX_SW_PAD_CTL_DDR_INPUT_LSH),
    DDK_IOMUX_PAD_INMODE_DDR        = (IOMUX_SW_PAD_CTL_DDR_INPUT_DDR << IOMUX_SW_PAD_CTL_DDR_INPUT_LSH)
} ddk_iomux_pad_inmode;

typedef enum 
{
    DDK_IOMUX_PAD_OUTVOLT_LOW       = (IOMUX_SW_PAD_CTL_HVE_LOW << IOMUX_SW_PAD_CTL_HVE_LSH),
    DDK_IOMUX_PAD_OUTVOLT_HIGH      = (IOMUX_SW_PAD_CTL_HVE_HIGH << IOMUX_SW_PAD_CTL_HVE_LSH)
} ddk_iomux_pad_outvolt;

typedef struct {
	ddk_iomux_pin pinId;
    	ddk_iomux_pin_muxmode pinMuxMode;
	ddk_iomux_pin_sion pinSION;
}iomux_set;

typedef struct{
	ddk_iomux_pad padname;
	ddk_iomux_pad_slew slew;
	ddk_iomux_pad_drive drive;
	ddk_iomux_pad_opendrain opendrain;
	ddk_iomux_pad_pull pull;
	ddk_iomux_pad_hysteresis hysteresis;
	ddk_iomux_pad_inmode  inmode;
	ddk_iomux_pad_outvolt outvolt;
}pad_set;

#ifndef __MACH_MX53_IOMUX_H__
/*!
 * various IOMUX input functions
 */
typedef enum iomux_input_config {
	INPUT_CTL_PATH0 = 0x0,
	INPUT_CTL_PATH1,
	INPUT_CTL_PATH2,
	INPUT_CTL_PATH3,
	INPUT_CTL_PATH4,
	INPUT_CTL_PATH5,
	INPUT_CTL_PATH6,
	INPUT_CTL_PATH7,
} iomux_input_config_t;
#endif

typedef struct{
	ddk_iomux_select_input pinselectid;
	iomux_input_config_t	value;
}selectmux_set;

int mxc_iomux_tractor_setup(iomux_set* default_mux_tab, int muxcount, pad_set* default_pad_tab, int tabcount, selectmux_set* default_selected_tab, int inputcount);

int mxc_iomux_tractor_set_mux(ddk_iomux_pin pin, ddk_iomux_pin_muxmode muxmode, ddk_iomux_pin_sion sion);
int mxc_iomux_tractor_get_mux(ddk_iomux_pin pin);
int mxc_iomux_tractor_set_pad(ddk_iomux_pad pad, ddk_iomux_pad_slew slew, ddk_iomux_pad_drive drive, ddk_iomux_pad_opendrain openDrain, ddk_iomux_pad_pull pull, ddk_iomux_pad_hysteresis hysteresis, ddk_iomux_pad_inmode inputMode, ddk_iomux_pad_outvolt outputVolt);
int mxc_iomux_tractor_get_pad(ddk_iomux_pad pad);


#endif /* __MACH_IOMUX_MX53_TRACTOR_H__ */
