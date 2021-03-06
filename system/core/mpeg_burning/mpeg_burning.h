#ifndef ___MPEG_BURNING_H__
#define ___MPEG_BURNING_H__


/*cmd value */
#define READ_CMD				'R'
#define READ_CMD_L				'r'
#define WRITE_CMD				'W'
#define WRITE_CMD_L				'w'
#define REBOOT_X_CMD				'X'
#define A_CMD					'A'
#define A_CMD_L					'a'

#define LACK_CMD				'L'
#define EACK_CMD				'E'


/* addr value */
#define	RF_STAMP				  0x1FFE8000
#define	RF_EMULATION	                          0x1FFE8004
#define	RF_RESET                                  0x1FFE8008
#define	RF_RESET2                                 0x1FFE800C
#define	RF_PAD_CTRL                               0x1FFE8010
#define	RF_SDRAM_CLKO_CFG                         0x1FFE8014
#define	RF_SDRAM_CLKI_CFG                         0x1FFE8018
#define	RF_SDRAM_CLKI_DLY_CFG                     0x1FFE801C
#define	RF_CLK_MON_SEL                            0x1FFE8020
#define	RF_CLK_MON_RESULT                         0x1FFE8024
#define	RF_CLKEN0                                 0x1FFE8028
#define	RF_CLKEN1                                 0x1FFE802C
#define	RF_GCLKEN0                                0x1FFE8030
#define	RF_GCLKEN1                                0x1FFE8034
#define	RF_SYSCLK_DIV_SEL                         0x1FFE8038
#define	RF_SYSCLK_SEL                             0x1FFE803C
#define	RF_PLLV_CFG                               0x1FFE8040
#define	RF_SFT_CFG0                               0x1FFE8044
#define	RF_SFT_CFG1                               0x1FFE8048
#define	RF_SFT_CFG2                               0x1FFE804C
#define	RF_SFT_CFG3                               0x1FFE8050
#define	RF_SFT_CFG4                               0x1FFE8054
#define	RF_SFT_CFG5                               0x1FFE8058
#define	RF_SFT_CFG6                               0x1FFE805C
#define	RF_SFT_CFG7                               0x1FFE8060
#define	RF_SFT_CFG8                               0x1FFE8064
#define	RF_HW_BO                                  0x1FFE8068
#define	RF_HW_CFG                                 0x1FFE806C
#define	RF_HW_CFG_CHG                             0x1FFE8070
#define	RF_SFT_CFG9                               0x1FFE8074
#define	RF_SFT_CFGA                               0x1FFE8078
#define	RF_G0_RESERVED_1_1                        0x1FFE807C
#define	RF_STC_15_0                               0x1FFE8080
#define	RF_STC_31_16                              0x1FFE8084
#define	RF_STC_32                                 0x1FFE8088
#define	RF_STC_DIVISOR                            0x1FFE808C
#define	RF_RTC_15_0                               0x1FFE8090
#define	RF_RTC_31_16                              0x1FFE8094
#define	RF_RTC_DIVISOR                            0x1FFE8098
#define	RF_STC_CONFIG                             0x1FFE809C
#define	RF_TIMER0_CTRL                            0x1FFE80A0
#define	RF_TIMER0_CNT                             0x1FFE80A4
#define	RF_TIMER1_CTRL                            0x1FFE80A8
#define	RF_TIMER1_CNT                             0x1FFE80AC
#define	RF_TIMERW_CTRL                            0x1FFE80B0
#define	RF_TIMERW_CNT                             0x1FFE80B4
#define	RF_G1_UNUSED_1_2                          0x1FFE80B8
#define	RF_TIMER2_CTRL                            0x1FFE80C0
#define	RF_TIMER2_DIVISOR                         0x1FFE80C4
#define	RF_TIMER2_RELOAD                          0x1FFE80C8
#define	RF_TIMER2_CNT                             0x1FFE80CC
#define	RF_TIMER3_CTRL                            0x1FFE80D0
#define	RF_TIMER3_DIVISOR                         0x1FFE80D4
#define	RF_TIMER3_RELOAD                          0x1FFE80D8
#define	RF_TIMER3_CNT                             0x1FFE80DC
#define	RF_STCL_0                                 0x1FFE80E0
#define	RF_STCL_1                                 0x1FFE80E4
#define	RF_STCL_2                                 0x1FFE80E8
#define	RF_ATC_0                                  0x1FFE80EC
#define	RF_ATC_1                                  0x1FFE80F0
#define	RF_ATC_2                                  0x1FFE80F4
#define	RF_G1_RESERVED_2                          0x1FFE80F8
#define	RF_INTR_MASK                              0x1FFE8100
#define	RF_INTR_FLAG                              0x1FFE8104
#define	RF_INTR_MASKED_FLAG                       0x1FFE8108
#define	RF_INTR_CLEAR                             0x1FFE810C
#define	RF_INTR_POLARITY                          0x1FFE8110
#define	RF_INTR_EDGE                              0x1FFE8114
#define	RF_INTR1_MASK                             0x1FFE8118
#define	RF_INTR1_FLAG                             0x1FFE811C
#define	RF_INTR1_MASKED_FLAG                      0x1FFE8120
#define	RF_INTR1_CLEAR                            0x1FFE8124
#define	RF_INTR1_POLARITY                         0x1FFE8128
#define	RF_INTR1_EDGE                             0x1FFE812C
#define	RF_LBC_CONTROL                            0x1FFE8130
#define	RF_LBC_WATCHDOG                           0x1FFE8134
#define	RF_REC_START                              0x1FFE8138
#define	RF_REC_END                                0x1FFE813C
#define	RF_REC_L_M                                0x1FFE8140
#define	RF_G2_RESERVED_13                         0x1FFE8144
#define	RF_RI_MISC_B0                             0x1FFE8178
#define	RF_RI_MISC_B1                             0x1FFE817C
#define	RF_SEQ_EXT                                0x1FFE8180
#define	RF_H_SIZE                                 0x1FFE8184
#define	RF_V_SIZE                                 0x1FFE8188
#define	RF_G3_UNUSED                              0x1FFE818C
#define	RF_PIC_CODING_TYPE                        0x1FFE8190
#define	RF_PIC_F_CODE                             0x1FFE8194
#define	RF_PIC_CODING_EXT0                        0x1FFE8198
#define	RF_PIC_CODING_EXT1                        0x1FFE819C
#define	RF_PIC_START                              0x1FFE81A0
#define	RF_DIS_PIC_ID                             0x1FFE81A4
#define	RF_DIS_SEQ_TAG                            0x1FFE81A8
#define	RF_G3_RESERVED_21                         0x1FFE81AC
#define	RF_VLD_CTRL                               0x1FFE8200
#define	RF_VLD_STATUS                             0x1FFE8204
#define	RF_VLD_DECODE_TIME                        0x1FFE8208
#define	RF_G4_UNUSED                              0x1FFE820C
#define	RF_VLD_MON0                               0x1FFE8210
#define	RF_VLD_MON1                               0x1FFE8214
#define	RF_VLD_MON2                               0x1FFE8218
#define	RF_VLD_MON3                               0x1FFE821C
#define	RF_JPEG_CONTROL                           0x1FFE8220
#define	RF_JPEG_WRITE_ADDR                        0x1FFE8224
#define	RF_JPEG_WRITE_DATA                        0x1FFE8228
#define	RF_JPEG_READ_ADDR                         0x1FFE822C
#define	RF_JPEG_READ_DATA                         0x1FFE8230
#define	RF_ERROR_MB_THRESHOLD                     0x1FFE8234
#define	RF_VLD_CONFIG                             0x1FFE8238
#define	RF_VLD_TRB_TRD                            0x1FFE823C
#define	RF_VLD_TRB_TRD_EXT                        0x1FFE8240
#define	RF_VLD_2TRBP_2DP                          0x1FFE8244
#define	RF_VLD_2TRBM_2DM                          0x1FFE8248
#define	RF_VLD_PACKET_HEADER                      0x1FFE824C
#define	RF_VLD_VOL_HEADER                         0x1FFE8250
#define	RF_VLD_VOP_HEADER                         0x1FFE8254
#define	RF_G4_RESERVED_9                          0x1FFE8258
#define	RF_VLD_GOB_NUM                            0x1FFE827C
#define	RF_MC_CONFIG                              0x1FFE8280
#define	RF_REF0_LUMA                              0x1FFE8284
#define	RF_REF0_CHROMA                            0x1FFE8288
#define	RF_REF1_LUMA                              0x1FFE828C
#define	RF_REF1_CHROMA                            0x1FFE8290
#define	RF_BIDIR_LUMA                             0x1FFE8294
#define	RF_BIDIR_CHROMA                           0x1FFE8298
#define	RF_G5_RESERVED_0_5                        0x1FFE829C
#define	RF_DIVX_MW_PAD                            0x1FFE82B0
#define	RF_VOP_ROUND_TYPE                         0x1FFE82B4
#define	RF_G5_RESERVED_1_10                       0x1FFE82B8
#define	RF_MC_SPARE_0                             0x1FFE82E0
#define	RF_MC_COMPRESS                            0x1FFE82E4
#define	RF_MC_PARDEC_START                        0x1FFE82E8
#define	RF_MC_PARDEC_END                          0x1FFE82EC
#define	RF_MC_MBWIDTH                             0x1FFE82F0
#define	RF_MC_PARDEC_EN                           0x1FFE82F4
#define	RF_MC_SPARE_6                             0x1FFE82F8
#define	RF_MC_SPARE_7                             0x1FFE82FC
#define	RF_SDCTRL_CFG0                            0x1FFE8300
#define	RF_SDCTRL_CFG1                            0x1FFE8304
#define	RF_SDCTRL_CFG2                            0x1FFE8308
#define	RF_SDCTRL_MRS                             0x1FFE830C
#define	RF_SDCTRL_SREF_CFG                        0x1FFE8310
#define	RF_SDCTRL_CFG3                            0x1FFE8314
#define	RF_SDCTRL_DATA_MON_ST                     0x1FFE8318
#define	RF_SDCTRL_DATA_CYC_L                      0x1FFE831C
#define	RF_SDCTRL_DATA_CYC_H                      0x1FFE8320
#define	RF_SDCTRL_DATA_CNT_L                      0x1FFE8324
#define	RF_SDCTRL_DATA_CNT_H                      0x1FFE8328
#define	RF_SDCTRL_GCLK_DIS                        0x1FFE832C
#define	RF_SDCTRL_AREF1_CFG                       0x1FFE8330
#define	RF_SDCTRL_INT                             0x1FFE8334
#define	RF_SDCTRL_INT_MASK                        0x1FFE8338
#define	RF_SDCTRL_INT_STATUS                      0x1FFE833C
#define	RF_SDCTRL_LMEM_BASE                       0x1FFE8340
#define	RF_SDCTRL_LMEM_ROW_ST                     0x1FFE8344
#define	RF_G6_UNUSED_0                            0x1FFE8348
#define	RF_SDCTRL_CFG4                            0x1FFE834C
#define	RF_G6_RESERVED_10                         0x1FFE8350
#define	RF_SDCTRL_MISC_B0                         0x1FFE8378
#define	RF_SDCTRL_MISC_B1                         0x1FFE837C
#define	RF_DIS_X_START                            0x1FFE8380
#define	RF_DIS_Y_START                            0x1FFE8384
#define	RF_DIS_X_SIZE                             0x1FFE8388
#define	RF_DIS_Y_SIZE                             0x1FFE838C
#define	RF_V_OFFSET                               0x1FFE8390
#define	RF_H_OFFSET                               0x1FFE8394
#define	RF_VPP_BG_Y                               0x1FFE8398
#define	RF_VPP_BG_CB_CR                           0x1FFE839C
#define	RF_V_FILTER0_SETUP                        0x1FFE83A0
#define	RF_V_FILTER1_SETUP                        0x1FFE83A4
#define	RF_H_FILTER_SETUP                         0x1FFE83A8
#define	RF_VPP_CONFIG                             0x1FFE83AC
#define	RF_VPP_CONFIG1                            0x1FFE83B0
#define	RF_DS_FIELD_CONFIG                        0x1FFE83B4
#define	RF_DIP_CONFIG                             0x1FFE83B8
#define	RF_DIP_PARAM                              0x1FFE83BC
#define	RF_DIP_MV_PTR                             0x1FFE83C0
#define	RF_VPPREF0_LUMA                           0x1FFE83C4
#define	RF_VPPREF0_CHROMA                         0x1FFE83C8
#define	RF_VPPREF1_LUMA                           0x1FFE83CC
#define	RF_VPPREF1_CHROMA                         0x1FFE83D0
#define	RF_VPPBIDIR_LUMA                          0x1FFE83D4
#define	RF_VPPBIDIR_CHROMA                        0x1FFE83D8
#define	RF_DIP_REF_BASE                           0x1FFE83DC
#define	RF_DIP_PARAM_THRESHOLD                    0x1FFE83E0
#define	RF_DIP_PARAM_FADING                       0x1FFE83E4
#define	RF_VPP_ACT_REGION_TOP                     0x1FFE83E8
#define	RF_VPP_ACT_REGION_BOTTOM                  0x1FFE83EC
#define	RF_VPP_SPARE_0                            0x1FFE83F0
#define	RF_VPP_CLUT_AY                            0x1FFE83F4
#define	RF_VPP_CLUT_UV                            0x1FFE83F8
#define	RF_VPP_SPARE_1                            0x1FFE83FC
#define	RF_TV_MODE_6                              0x1FFE8400
#define	RF_TV_SUBC_F_2                            0x1FFE8418
#define	RF_TV_SUBC_P                              0x1FFE8420
#define	RF_TV_LINE_T_2                            0x1FFE8424
#define	RF_TV_LINE_B_2                            0x1FFE842C
#define	RF_TV_CC_T                                0x1FFE8434
#define	RF_TV_CC_B                                0x1FFE8438
#define	RF_TV_CGMS_2                              0x1FFE843C
#define	RF_TV_ID_STATUS                           0x1FFE8444
#define	RF_TV_DAC_2                               0x1FFE8448
#define	RF_TV_MISC_12                             0x1FFE8450
#define	RF_DSP24_CONTROL                          0x1FFE8480
#define	RF_DSP3DBG_MODE                           0x1FFE8484
#define	RF_DSP3DBG_BREAK                          0x1FFE8488
#define	RF_DSP3DBG_ADDR                           0x1FFE848C
#define	RF_DSP3DBG_STEP                           0x1FFE8490
#define	RF_DSP3DBG_RD_L                           0x1FFE8494
#define	RF_DSP3DBG_RD_H                           0x1FFE8498
#define	RF_DSP3DBG_WR_L                           0x1FFE849C
#define	RF_DSP3DBG_WR_H                           0x1FFE84A0
#define	RF_DSP3DBG_PC                             0x1FFE84A4
#define	RF_ROM_L                                  0x1FFE84A8
#define	RF_ROM_H                                  0x1FFE84AC
#define	RF_DSP24YA                                0x1FFE84B0
#define	RF_PCMYA                                  0x1FFE84B4
#define	RF_AUDYA                                  0x1FFE84B8
#define	RF_G9_RESERVED                            0x1FFE84BC
#define	RF_DSP24_PORT_16                          0x1FFE84C0
#define	RF_OSD_TV_STD                             0x1FFE8500
#define	RF_OSD_TV_OUT                             0x1FFE8504
#define	RF_OSD_MIXER                              0x1FFE8508
#define	RF_OSD_DISPLAY_STATUS                     0x1FFE850C
#define	RF_OSD_EN                                 0x1FFE8510
#define	RF_OSD_TLINK_ADDR                         0x1FFE8514
#define	RF_OSD_BLINK_ADDR                         0x1FFE8518
#define	RF_Y_CLIP                                 0x1FFE851C
#define	RF_CB_CLIP                                0x1FFE8520
#define	RF_OSD_BASE_ADDR                          0x1FFE8524
#define	RF_OSD_MODE_12                            0x1FFE8528
#define	RF_G10_RESERVED_10                        0x1FFE8558
#define	RF_GRAPH_MODE                             0x1FFE8580
#define	RF_GRAPH_STATUS                           0x1FFE8584
#define	RF_DMA_MODE                               0x1FFE8588
#define	RF_DMA_XADDR                              0x1FFE858C
#define	RF_DMA_YADDR                              0x1FFE8590
#define	RF_DMA_DONE                               0x1FFE8594
#define	RF_G11_RESERVED_26                        0x1FFE8598
#define	RF_G12_DVDDSP_VY                          0x1FFE8600
#define	RF_G12_DVDDSP_VX                          0x1FFE8604
#define	RF_DVDDSP_FUNCTION                        0x1FFE8608
#define	RF_DVDDSP_ATA_CONFIG                      0x1FFE860C
#define	RF_DVDDSP_BLOCKSIZE                       0x1FFE8610
#define	RF_DVDDSP_BLOCKLENGTH                     0x1FFE8614
#define	RF_G12_DVDDSP_RY                          0x1FFE8618
#define	RF_DVDDSP_RX                              0x1FFE861C
#define	RF_DVDDSP_PUBLIC                          0x1FFE8620
#define	RF_DVDDSP_UDE_CONFIG                      0x1FFE8624
#define	RF_DVDDSP_ATA_PIO_CYCLE                   0x1FFE8628
#define	RF_DVDDSP_ATA_UDMA_CYCLE                  0x1FFE862C
#define	RF_G12_RESERVED_4                         0x1FFE8630
#define	RF_DVDDSP_MON_4                           0x1FFE8640
#define	RF_G12_RESERVED1_12                       0x1FFE8650
#define	RF_CSS_TK0                                0x1FFE8680
#define	RF_CSS_TK1                                0x1FFE8684
#define	RF_CSS_TK2                                0x1FFE8688
#define	RF_CSS_TK3                                0x1FFE868C
#define	RF_CSS_TBYTE                              0x1FFE8690
#define	RF_CSS_PUBLIC                             0x1FFE8694
#define	RF_CSS_CONFIG                             0x1FFE8698
#define	RF_CSS_L0                                 0x1FFE869C
#define	RF_CSS_L1                                 0x1FFE86A0
#define	RF_CSS_R0                                 0x1FFE86A4
#define	RF_CSS_R1                                 0x1FFE86A8
#define	RF_CPPM_AUKEY3                            0x1FFE86AC
#define	RF_CPPM_AUKEY2                            0x1FFE86B0
#define	RF_CPPM_AUKEY1                            0x1FFE86B4
#define	RF_CPPM_AUKEY0                            0x1FFE86B8
#define	RF_G13_RESERVED_17                        0x1FFE86BC
#define	RF_IOP_CONTROL                            0x1FFE8700
#define	RF_IOP_STATUS                             0x1FFE8704
#define	RF_IOP_BP                                 0x1FFE8708
#define	RF_IOP_REGSEL                             0x1FFE870C
#define	RF_IOP_REGOUT                             0x1FFE8710
#define	RF_IOP_MEMLIMIT                           0x1FFE8714
#define	RF_G14_RESERVED_2                         0x1FFE8718
#define	RF_IOP_DATA_8                             0x1FFE8720
#define	RF_G14_RESERVED1_16                       0x1FFE8740
#define	RF_SUP_FST_CMD_ADDR                       0x1FFE8780
#define	RF_SUP_SND_CMD_ADDR                       0x1FFE8784
#define	RF_SUP_H_SIZE                             0x1FFE8788
#define	RF_SUP_MODE                               0x1FFE878C
#define	RF_SUP_TV_MODE                            0x1FFE8790
#define	RF_SUP_PANNING                            0x1FFE8794
#define	RF_SUP_ASPECT_RATIO                       0x1FFE8798
#define	RF_SUP_MON_5                              0x1FFE879C
#define	RF_SUP_CONFIG                             0x1FFE87B0
#define	RF_SUP_BUFFER_LIMIT                       0x1FFE87B4
#define	RF_G15_RESERVED_18                        0x1FFE87B8
#define	RF_G16_RESERVED_32                        0x1FFE8800
#define	RF_G17_RESERVED_32                        0x1FFE8880
#define	RF_UART0_DATA                             0x1FFE8900
#define	RF_UART0_LSR                              0x1FFE8904
#define	RF_UART0_MSR                              0x1FFE8908
#define	RF_UART0_LCR                              0x1FFE890C
#define	RF_UART0_MCR                              0x1FFE8910
#define	RF_UART0_DIV_L                            0x1FFE8914
#define	RF_UART0_DIV_H                            0x1FFE8918
#define	RF_UART0_ISC                              0x1FFE891C
#define	RF_UART1_DATA                             0x1FFE8920
#define	RF_UART1_LSR                              0x1FFE8924
#define	RF_UART1_MSR                              0x1FFE8928
#define	RF_UART1_LCR                              0x1FFE892C
#define	RF_UART1_MCR                              0x1FFE8930
#define	RF_UART1_DIV_L                            0x1FFE8934
#define	RF_UART1_DIV_H                            0x1FFE8938
#define	RF_UART1_ISC                              0x1FFE893C
#define	RF_G18_RESERVED_16                        0x1FFE8940
#define	RF_GPIO_MASTER_8                          0x1FFE8980
#define	RF_GPIO_OE_8                              0x1FFE89A0
#define	RF_GPIO_OUT_8                             0x1FFE89C0
#define	RF_GPIO_IN_8                              0x1FFE89E0
#define	RF_CDDSP_CONFIG                           0x1FFE8A00
#define	RF_CDDSP_CONTROL                          0x1FFE8A04
#define	RF_CDDSP_MM_BCD                           0x1FFE8A08
#define	RF_CDDSP_SS_BCD                           0x1FFE8A0C
#define	RF_CDDSP_FF_BCD                           0x1FFE8A10
#define	RF_CDDSP_STATUS                           0x1FFE8A14
#define	RF_CDDSP_MMSS                             0x1FFE8A18
#define	RF_CDDSP_FFMM                             0x1FFE8A1C
#define	RF_G20_RESERVED_24                        0x1FFE8A20
#define	RF_TV_GAMMA_5                             0x1FFE8A80
#define	RF_TV_PCCON_18                            0x1FFE8A94
#define	RF_G21_RESERVED_9                         0x1FFE8ADC
#define	RF_MBUS_BRIDGE_CONFIG                     0x1FFE8B00
#define	RF_EVBYA                                  0x1FFE8B04
#define	RF_OSDYA                                  0x1FFE8B08
#define	RF_CDWYA                                  0x1FFE8B0C
#define	RF_CDRYA                                  0x1FFE8B10
#define	RF_SUPYA                                  0x1FFE8B14
#define	RF_EVBYA_LIMIT                            0x1FFE8B18
#define	RF_OSDYA_LIMIT                            0x1FFE8B1C
#define	RF_CDWYA_LIMIT                            0x1FFE8B20
#define	RF_CDRYA_LIMIT                            0x1FFE8B24
#define	RF_SUPYA_LIMIT                            0x1FFE8B28
#define	RF_BS_YSTOP                               0x1FFE8B2C
#define	RF_BS_RY                                  0x1FFE8B30
#define	RF_BS_YINIT                               0x1FFE8B34
#define	RF_BS_XINIT                               0x1FFE8B38
#define	RF_DVDDSP_VY                              0x1FFE8B3C
#define	RF_DVDDSP_VX                              0x1FFE8B40
#define	RF_DVDDSP_RY                              0x1FFE8B44
#define	RF_CDR_VY                                 0x1FFE8B48
#define	RF_CDR_VX                                 0x1FFE8B4C
#define	RF_SUPYA2                                 0x1FFE8B50
#define	RF_SUPYA2_LIMIT                           0x1FFE8B54
#define	RF_IOPYA                                  0x1FFE8B58
#define	RF_CDRYA_XLIMIT                           0x1FFE8B5C
#define	RF_MVCYA                                  0x1FFE8B60
#define	RF_G22_RESERVED_7                         0x1FFE8B64
#define	RF_G23_VPP_CONTRAST_ADJ_2                 0x1FFE8B80
#define	RF_G23_VPP_CONTRAST_SLOPE_3               0x1FFE8B88
#define	RF_G23_VPP_HISTOGRAM_8                    0x1FFE8B94
#define	RF_G23_VPP_CHKSUM                         0x1FFE8BB4
#define	RF_G23_VPP_MV_PTR                         0x1FFE8BB8
#define	RF_G23_VPP_HUE_ADJ_2                      0x1FFE8BBC
#define	RF_G23_RESERVED_15                        0x1FFE8BC4
#define	RF_BUFCTL_8                               0x1FFE8C00
#define	RF_G24_RESERVED_24                        0x1FFE8C20
#define	RF_INVQ_QMX_PAR                           0x1FFE8C80
#define	RF_INVQ_MODE                              0x1FFE8C84
#define	RF_INVQ_CHKSUM                            0x1FFE8C88
#define	RF_G25_RESERVED_0_2                       0x1FFE8C8C
#define	RF_INVQ_VOL_HEADER                        0x1FFE8C94
#define	RF_G25_RESERVED_1_26                      0x1FFE8C98
#define	RF_ROM_CONFIG                             0x1FFE8D00
#define	RF_WAIT_CYC1_0                            0x1FFE8D04
#define	RF_WAIT_CYC3_2                            0x1FFE8D08
#define	RF_OE_WAIT_CYC1_0                         0x1FFE8D0C
#define	RF_OE_WAIT_CYC3_2                         0x1FFE8D10
#define	RF_WE_WAIT_CYC1_0                         0x1FFE8D14
#define	RF_WE_WAIT_CYC3_2                         0x1FFE8D18
#define	RF_IOCHRDY_WAIT_CYC                       0x1FFE8D1C
#define	RF_ROM1_BASE                              0x1FFE8D20
#define	RF_ROM2_BASE                              0x1FFE8D24
#define	RF_ROM3_BASE                              0x1FFE8D28
#define	RF_PCMCIA_IORW_WAIT                       0x1FFE8D2C
#define	RF_PCMCIA_CTRL                            0x1FFE8D30
#define	RF_G26_RESERVED1_8                        0x1FFE8D34
#define	RF_ADT_4                                  0x1FFE8D54
#define	RF_DAT_1                                  0x1FFE8D58
#define	RF_DAT_2                                  0x1FFE8D5C
#define	RF_DAT_3                                  0x1FFE8D60
#define	RF_DAT_4                                  0x1FFE8D64
#define	RF_DAT_5                                  0x1FFE8D68
#define	RF_DAT_6                                  0x1FFE8D6C
#define	RF_ADM_1                                  0x1FFE8D6C
#define	RF_ADM_2                                  0x1FFE8D74
#define	RF_ADM_3                                  0x1FFE8D78
#define	RF_DAR                                    0x1FFE8D7C
#define	RF_SBAR_CONFIG                            0x1FFE8D80
#define	RF_SBAR_PRR_16                            0x1FFE8D84
#define	RF_G27_RESERVED_2                         0x1FFE8DC4
#define	RF_SBAR_SDRAM_ROM                         0x1FFE8DCC
#define	RF_G27_RESERVED1_12                       0x1FFE8DD0
#define	RF_RF_SDRAMIF_TBYA                        0x1FFE8E00
#define	RF_RF_SERVO_BAND_EN                       0x1FFE8E04
#define	RF_RF_SERVO_BAND_VAL                      0x1FFE8E08
#define	RF_G28_RESERVED_29                        0x1FFE8E0C
#define	RF_RF_REGIF_ADDR                          0x1FFE8E80
#define	RF_RF_REGIF_WDATA                         0x1FFE8E84
#define	RF_RF_REGIF_RDATA                         0x1FFE8E88
#define	RF_RF_REGIF_SAMPLE_CTRL                   0x1FFE8E8C
#define	RF_RF_REGIF_INTR_ADDR                     0x1FFE8E90
#define	RF_RF_REGIF_INTR_WDATA                    0x1FFE8E94
#define	RF_RF_REGIF_INTR_RDATA                    0x1FFE8E98
#define	RF_G29_RESERVED_25                        0x1FFE8E9C
#define	RF_EMU_CFG_32                             0x1FFE8F00
#define	RF_AUD_RESET                              0x1FFE8F80
#define	RF_AUD_PCM_CFG                            0x1FFE8F84
#define	RF_AUD_SPDIF_CFG                          0x1FFE8F88
#define	RF_AUD_ENABLE                             0x1FFE8F8C
#define	RF_AUD_ADC_STEREO_CFG                     0x1FFE8F90
#define	RF_AUD_ADC_MONO_CFG                       0x1FFE8F94
#define	RF_AUD_PCM_RAMP_DELTA                     0x1FFE8F98
#define	RF_AUD_PCM_RAMP_CFG                       0x1FFE8F9C
#define	RF_AUD_PCM_RAMP_VALUE                     0x1FFE8FA0
#define	RF_AUD_SPDIF_PERIOD                       0x1FFE8FA4
#define	RF_AUD_FIFO_FLAG                          0x1FFE8FA8
#define	RF_AUD_CHN_PCM_CNT_10                     0x1FFE8FAC
#define	RF_AUD_XCK_CFG                            0x1FFE8FD4
#define	RF_AUD_PCM_BCK_CFG                        0x1FFE8FD8
#define	RF_AUD_IEC_BCLK_CFG                       0x1FFE8FDC
#define	RF_AUD_ADC_MCLK_CFG                       0x1FFE8FE0
#define	RF_AUD_DSP_RUN_CNT                        0x1FFE8FE4
#define	RF_AUD_DSP_STALL_CNT                      0x1FFE8FE8
#define	RF_AUD_DSP_RESET_FLAG                     0x1FFE8FEC
#define	RF_AUD_DSP_DEC_CNT_TOGGLE                 0x1FFE8FF0
#define	RF_AUD_DSP_DEC_CNT                        0x1FFE8FF4
#define	RF_AUD_FPGA_V2_2                          0x1FFE8FF8
#define	RF_G32_RESERVED_32                        0x1FFE9000
#define	RF_G33_RESERVED_32                        0x1FFE9080
#define	RF_G34_RESERVED_32                        0x1FFE9100
#define	RF_G35_RESERVED_32                        0x1FFE9180
#define	RF_G36_RESERVED_32                        0x1FFE9200
#define	RF_G37_RESERVED_32                        0x1FFE9280
#define	RF_G38_RESERVED_32                        0x1FFE9300
#define	RF_G39_RESERVED_32                        0x1FFE9380
#define	RF_SDC_REQ_T_RESET                        0x1FFE9400
#define	RF_SDC_REQ_TIME_14_2                      0x1FFE9404
#define	RF_G40_RESERVED_3                         0x1FFE9474
#define	RF_SDC_DATA_CNT_14_2                      0x1FFE9480
#define	RF_SDC_N_REQ_CNT_2                        0x1FFE94F0
#define	RF_SDC_CKE_CNT_2                          0x1FFE94F8
#define	RF_G42_RESERVED_32                        0x1FFE9500
#define	RF_G43_RESERVED_32                        0x1FFE9580
#define	RF_G44_RESERVED_32                        0x1FFE9600
#define	RF_G45_RESERVED_32                        0x1FFE9680
#define	RF_SPI_CTRL                               0x1FFE9700
#define RF_SPI_WAIT						0x1ffe9704
#define RF_SPI_CUST_CMD           0x1ffe9708
#define RF_SPI_ADDR_LOW           0x1ffe970c
#define RF_SPI_ADDR_HIGH          0x1ffe9710
#define RF_SPI_DATA_LOW           0x1ffe9714
#define RF_SPI_DATA_HIGH          0x1ffe9718
#define RF_SPI_STATUS             0x1ffe971c
#define RF_SPI_CFG0               0x1ffe9720
#define RF_SPI_CFG1               0x1ffe9724
#define RF_SPI_CFG2               0x1ffe9728
#define RF_SPI_CFG3               0x1ffe972c
#define RF_SPI_CFG4               0x1ffe9730
#define	RF_G47_RESERVED_32            0x1FFE9780
#define RF_SFT_CFG11                  0x1ffe9800
#define RF_SFT_CFG12                  0x1ffe9804
#define RF_SFT_CFG13                  0x1ffe9808
#define RF_SFT_CFG14                  0x1ffe980c
#define RF_SFT_CFG15                  0x1ffe9810
#define RF_SFT_CFG16                  0x1ffe9814
#define RF_SFT_CFG17                  0x1ffe9818
#define RF_SFT_CFG18                  0x1ffe981c
#define RF_SFT_CFG19                  0x1ffe9820
#define RF_SFT_CFG20                  0x1ffe9824
#define RF_SFT_CFG21                  0x1ffe9828
#define RF_SFT_CFG22                  0x1ffe982c
#define RF_SFT_CFG23                  0x1ffe9830
#define RF_SFT_CFG24                  0x1ffe9834
#define RF_SFT_CFG25                  0x1ffe9838
#define RF_SFT_CFG26                  0x1ffe983c
#define RF_SFT_CFG27                  0x1ffe9840  
#define RF_SFT_CFG28                  0x1ffe9844
#define RF_SFT_CFG29                  0x1ffe9848
#define RF_SFT_CFG30                  0x1ffe984c
#define RF_SFT_CFG31                  0x1ffe9850
#define	RF_G49_RESERVED_32                        0x1FFE9880
#define	RF_G50_RESERVED_29                        0x1FFE9900
#define	RF_RISC_FPGA_VERSION_3                    0x1FFE9974
#define	RF_G51_RESERVED_29                        0x1FFE9980
#define	RF_BLOCK_FPGA_VERSION_3                   0x1FFE99F4
#define	RF_G52_RESERVED_29                        0x1FFE9A00
#define	RF_AUD_FPGA_VERSION_3                     0x1FFE9A74
#define	RF_G53_RESERVED_32                        0x1FFE9A80
#define	RF_G54_RESERVED_32                        0x1FFE9B00
#define	RF_G55_RESERVED_32                        0x1FFE9B80
#define	RF_G56_RESERVED_32                        0x1FFE9C00
#define	RF_G57_RESERVED_32                        0x1FFE9C80
#define	RF_G58_RESERVED_32                        0x1FFE9D00
#define	RF_G59_RESERVED_32                        0x1FFE9D80
#define	RF_G60_RESERVED_32                        0x1FFE9E00
#define	RF_G61_RESERVED_32                        0x1FFE9E80
#define	RF_G62_RESERVED_32                        0x1FFE9F00
#define	RF_G63_RESERVED_32                        0x1FFE9F80
#define	RF_GXX_RESERVED_192_32                    0x1FFEA000


#endif
