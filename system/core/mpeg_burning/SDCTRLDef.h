//
// SDCTRL_CFG0      timing control #0
// [xsr]    [13:10]
// [mrd]    [9:8]
// [rc]     [7:3]
// [ras]    [3:0]
//
#define SDCTRL_CFG0_RAS(n)          ((n-1)<<0)
#define SDCTRL_CFG0_RC(n)           ((n-1)<<3)
#define SDCTRL_CFG0_MRD(n)          ((n-1)<<8)
#define SDCTRL_CFG0_XSR(n)          ((n-1)<<10)

#define SDCTRL_CFG0_VAL_0       SDCTRL_CFG0_RAS(6)           \
                                | SDCTRL_CFG0_RC(8)           \
                                | SDCTRL_CFG0_MRD(2)		\
								| SDCTRL_CFG0_XSR(8)		\
//
// SDCTRL_CFG1      timing control #1
// [wr]     [13:12]
// [cl]     [11:10]
// [rrd]    [9:8]
// [rp]     [7:6]
// [rcd]    [5:4]
// [rfc]    [3:0]
//
#define SDCTRL_CFG1_RFC(n)          ((n-1)<<0)
#define SDCTRL_CFG1_RCD(n)          ((n-1)<<4)
#define SDCTRL_CFG1_RP(n)           ((n-1)<<6)
#define SDCTRL_CFG1_RRD(n)          ((n-1)<<8)
#define SDCTRL_CFG1_CL(n)           ((n-1)<<10)
#define SDCTRL_CFG1_WR(n)           ((n-1)<<12)



#define SDCTRL_CFG1_VAL_0       SDCTRL_CFG1_RFC(7)           \
                                | SDCTRL_CFG1_RCD(3)         \
                                | SDCTRL_CFG1_RP(3)           \
                                | SDCTRL_CFG1_RRD(3)         \
                                | SDCTRL_CFG1_WR(1)			\
								| SDCTRL_CFG1_CL(3)	\
								| SDCTRL_CFG1_WR(3)	\
//
// SDCTRL_CFG2      mrs
//
#define SDCTRL_CFG2_BL_1            (0x00<<0)
#define SDCTRL_CFG2_BL_2            (0x01<<0)
#define SDCTRL_CFG2_BL_4            (0x02<<0)
#define SDCTRL_CFG2_BL_8            (0x03<<0)
#define SDCTRL_CFG2_BL_FULLPAGE     (0x07<<0)

#define SDCTRL_CFG2_BT_SEQUENTIAL   (0x00<<3)
#define SDCTRL_CFG2_BT_INTERLEAVED  (0x01<<3)

#define SDCTRL_CFG2_CL_2            (0x02<<4)
#define SDCTRL_CFG2_CL_3            (0x03<<4)

#define SDCTRL_CFG2_OP_NORMAL       (0x00<<7)

#define SDCTRL_CFG2_BE_PROGRAMMED   (0x00<<9)
#define SDCTRL_CFG2_BE_SINGLE       (0x01<<9)

#define SDCTRL_CFG2_VAL_0       SDCTRL_CFG2_BL_FULLPAGE         \
                                | SDCTRL_CFG2_BT_SEQUENTIAL     \
                                | SDCTRL_CFG2_OP_NORMAL         \
                                | SDCTRL_CFG2_BE_PROGRAMMED		\
								| SDCTRL_CFG2_CL_3
// 
// SDCTRL_CFG3      sdram type
//

// [0]
#define SDCTRL_CFG3_INF_32B         (0<<0)
#define SDCTRL_CFG3_INF_16B         (1<<0)

#if 1
// [3:1]
#define SDCTRL_CFG3_16MB            (0<<1)
#define SDCTRL_CFG3_32MB            (1<<1)
#define SDCTRL_CFG3_64MB            (2<<1)
#define SDCTRL_CFG3_128MB           (3<<1)
#define SDCTRL_CFG3_256MB           (4<<1)
#define SDCTRL_CFG3_512MB           (5<<1)
// [5:4]
#define SDCTRL_CFG3_NIBBLE          (0<<4)
#define SDCTRL_CFG3_BYTE            (1<<4)
#define SDCTRL_CFG3_SHORT           (2<<4)
#define SDCTRL_CFG3_WORD            (3<<4)
#endif

// [7:6]
#define SDCTRL_CFG3_1PS             (0<<6)
#define SDCTRL_CFG3_2PS             (1<<6)
#define SDCTRL_CFG3_4PS             (2<<6)
#define SDCTRL_CFG3_8PS             (3<<6)
// [8]
#define SDCTRL_CFG3_CKE_DIS         (0<<8)
#define SDCTRL_CFG3_CKE_EN          (1<<8)
// [9]
#define SDCTRL_CFG3_PRERAS_EN       (0<<9)
#define SDCTRL_CFG3_PRERAS_DIS      (1<<9)
// [10]
#define SDCTRL_CFG3_BURST_256       (0<<10)
#define SDCTRL_CFG3_BURST_8         (1<<10)     

#define SDCTRL_CFG3_VAL_0       (SDCTRL_CFG3_CKE_EN |  \
								 SDCTRL_CFG3_PRERAS_EN | \
								 SDCTRL_CFG3_BURST_256|  \
								 SDCTRL_CFG3_INF_16B |\
								 SDCTRL_CFG3_16MB|   \
								 SDCTRL_CFG3_4PS )\

#define SDCTRL_CFG4_CW(n)           ((n)<<0)            // column width 7:256 8:512 9:1024
#define SDCTRL_CFG4_RW(n)           ((n)<<4)            // row width 8:512 9:1024 10:2048
#define SDCTRL_CFG4_PALL(n)         ((n)<<8)            // normal at 10
#define SDCTRL_CFG4_BANK4(n)        ((n)<<12)           // 0: 2-bank  1: 4-bank

#define SDCTRL_CFG4_VAL (SDCTRL_CFG4_CW(8)|SDCTRL_CFG4_RW(11)|SDCTRL_CFG4_PALL(10)|SDCTRL_CFG4_BANK4(1))

//
// SDCTRL_SREF  (SREF)
//
#define SDCTRL_SREF_SEL(n)          (n & 0x3ff)         // n = 0~1023, cycle=(n<<5) + 31


#define SDCTRL_SREF_VAL SDCTRL_SREF_SEL(1) 

//
// SDCTRL_AREF1
//
#define SDCTRL_AREF1_SEL(n)         (n & 0x3ff)         // n = 0~1023, cycle=(n<<5) + 31
#define SDCTRL_AREF1_DISABLE        (0<<10)
#define SDCTRL_AREF1_ENABLE         (1<<10)
#define SDCTRL_AREF1_AUTOREF        (0<<11)             // normal refresh mode
#define SDCTRL_AREF1_SELFREF        (1<<11)             // self-refresh mode
#define SDCTRL_AREF1_REFCNT(n)      ((n-1)<<12)         // 1~4

#define SDCLK                       100000000           // 100-mhz
#define AREF1_SEL(clk, n)           ((156 * clk * n / 10000000) / 32)

#define SDCTRL_AREF1_VAL SDCTRL_AREF1_ENABLE|SDCTRL_AREF1_SELFREF|SDCTRL_AREF1_REFCNT(4)|SDCTRL_AREF1_SEL(AREF1_SEL(SDCLK,4))