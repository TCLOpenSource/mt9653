#ifndef _DT_BINDINGS_CLK_MT5870_H
#define _DT_BINDINGS_CLK_MT5870_H


#define CLKGEN_0_BANK	0x100B00
#define	CLKGEN_1_BANK	0x103300
#define CLKGEN_2_BANK	0x100A00
#define CHIP_BANK	0x101E00

/* clock list  provided by topckgen */
#define CLK_TOP_DMDPLL					1
#define CLK_TOP_SCPLL					2
#define CLK_TOP_UPLL					3
#define CLK_TOP_XTAL					4
#define CLK_TOP_EVDPLL					5
#define CLK_TOP_XTAL_24M				6
#define CLK_TOP_MIU_128BUS_PLL				7
#define CLK_TOP_MIU_256BUS_PLL				8
#define CLK_TOP_DDR_PLL_A				9
#define CLK_TOP_DDR_PLL_B				10

#define CLK_TOP_FIX_RATE_NR				11

#define CLK_TOP_DMDPLL_D2				1
#define CLK_TOP_DMDPLL_D2P5				2
#define CLK_TOP_DMDPLL_D3				3
#define CLK_TOP_DMDPLL_D4				4
#define CLK_TOP_DMDPLL_D5				5
#define CLK_TOP_DMDPLL_D6				6
#define CLK_TOP_DMDPLL_D7				7
#define CLK_TOP_DMDPLL_D8				8
#define CLK_TOP_DMDPLL_D10				9
#define CLK_TOP_DMDPLL_D12				10
#define CLK_TOP_DMDPLL_D14				11
#define CLK_TOP_DMDPLL_D16				12
#define CLK_TOP_DMDPLL_D20				13
#define CLK_TOP_DMDPLL_D24				14
#define CLK_TOP_DMDPLL_D32				15
#define CLK_TOP_DMDPLL_D160				16

#define CLK_TOP_SCPLL_D1				17
#define CLK_TOP_SCPLL_D2				18

#define CLK_TOP_UPLL_D2					19
#define CLK_TOP_UPLL_D2p5				20
#define CLK_TOP_UPLL_D3					21
#define CLK_TOP_UPLL_D4					22
#define CLK_TOP_UPLL_D5					23
#define CLK_TOP_UPLL_D6					24
#define CLK_TOP_UPLL_D10				25
#define CLK_TOP_UPLL_D20				26
#define CLK_TOP_UPLL_D24				27
#define CLK_TOP_UPLL_D30				28 /* clk_96_buf */
#define CLK_TOP_UPLL_D48				29 /* clk_48_buf */

#define CLK_TOP_XTAL_D2					30
#define CLK_TOP_XTAL_D4					31
#define CLK_TOP_XTAL_D8					32
#define CLK_TOP_XTAL_D16				33
#define CLK_TOP_XTAL_D32				34
#define CLK_TOP_XTAL_D40				35
#define CLK_TOP_XTAL_D64				36
#define CLK_TOP_XTAL_D128				37

#define CLK_TOP_EVDPLL_D1				38
#define CLK_TOP_EVDPLL_D2p5				39
#define CLK_TOP_XTAL_12M_D1				40
#define CLK_TOP_XTAL_24M_D1				41
#define CLK_TOP_MIU_128BUS_PLL_D1			42
#define CLK_TOP_MIU_128BUS_PLL_D2			43
#define CLK_TOP_MIU_256BUS_PLL_D1			44
#define CLK_TOP_MIU_256BUS_PLL_D2			45
#define CLK_TOP_DDR_PLL_A_D4				46
#define CLK_TOP_DDR_PLL_A_D8				47
#define CLK_TOP_DDR_PLL_B_D4				48
#define CLK_TOP_DDR_PLL_B_D8				49

#define CLK_TOP_FIX_FACTOR_NR				50

/* clock list provided by topgen_mux_gate */
#define CLK_SRAM_PWCTL					1
#define CLK_MIIC0					2
#define CLK_MIIC1					3
#define CLK_MIIC2					4
#define CLK_MIIC3					5
#define CLK_MIIC4					6
#define CLK_MIIC5					7
#define CLK_ZDEC_VLD					8
#define CLK_ZDEC_LZD					9
#define CLK_MFE						10
#define CLK_TSP						11
#define CLK_MVD						12

#define CLK_UART0					13
#define CLK_UART1					14
#define CLK_UART2					15
#define CLK_UART3					16
#define CLK_UART4					17
#define CLK_VE						18
#define CLK_NJPD					19
#define CLK_VP8						20
#define CLK_HVD_AEC					21
#define CLK_HVD						22

#define CLK_HVD_IDB					23
#define CLK_HVD_R2					24
#define CLK_HVD_IDB_R2					25

#define CLK_EVD						26
#define CLK_EVD_PPU					27
#define CLK_EVD_FW					28
#define CLK_EVD_PPU_FW					29
#define CLK_VD_MHEG5_DFS				30
#define CLK_DQMEM_DMA					31
#define CLK_ECC						32
#define CLK_GPD						33
#define CLK_R2_SECURE					34
#define CLK_MSPI_MCARD					35
#define CLK_PARSER					36
#define CLK_STAMP					37
#define CLK_FCLK					38
#define CLK_S2_FCLK					39
#define CLK_EDCLK_F1					40
#define CLK_EDCLK_F2					41
#define CLK_SC_SLOW					42
#define CLK_VDMCU					43
#define CLK_DMDMCU					44
#define CLK_PCM						45
#define CLK_SMART					46
#define CLK_AESDMA					47
#define CLK_RIU						48
#define CLK_M2_RIU					49
#define CLK_MCU_AU					50
#define CLK_MCU						51
#define CLK_MIU						52
#define CLK_XTAL_12M					53 /* same as CLK_TOP_XTAL_12M_D1*/
#define CLK_MIU_256BUS					54
#define CLK_MIU2X_FRO_TOP_A				55
#define CLK_MIU2X_FRO_TOP_B				56
#define CLK_MIU_REC					57
#define CLK_FUART0_SYNTH_IN				58

#define CLK_SYN_STC0					59
#define CLK_SYN_STC1					60
#define CLK_SYN_STC2					61
#define CLK_SYN_STC3					62
#define CLK_DDR_SYN					63
#define CLK_FCIE_SYN					64


#define CLK_GE						65
#define CLK_SPI						66
#define CLK_DMDANA_RIU					67
#define CLK_FLOCK_CLK_SYNTH_IN				68
#define CLK_SMART_CA_SYN_27				69
#define CLK_SMART_CA_SYN_432				70
#define CLK_AUSDM_3M					71
#define CLK_SPI_M_PRE					72
#define CLK_SPI_M_1_PRE					73
#define CLK_SPI_M_2_PRE					74
#define CLK_BAT						75
#define CLK_FRC_R2					76
#define CLK_MCU_FRC					77
#define CLK_FCLK_FRC_FSC				78
#define CLK_FDCLK					79
#define CLK_MIU_HIGHWAY_A				80
#define CLK_MIU_HIGHWAY_B				81
#define CLK_IMI						82
#define CLK_TSO_OUT_DIV_MN_SRC				83
#define CLK_LZMA					84
#define CLK_HDMIRX_TIMESTAMP				85
//#define CLK_GOP_AFBC					86
//#define CLK_MIU_AIA					87
#define CLK_NFIE					86
#define CLK_TOP_MG_TEST					87
#define	CLK_TOP_MG_NR					88

/* clock list provided by subsys_mux_gate */
#define CLK_SUBSYS_MG_TEST				1
#define CLK_SUBSYS_MG_STC0				2
#define CLK_SUBSYS_MG_STC1				3
#define CLK_SUBSYS_MG_STC_TSIF0			4
#define CLK_SUBSYS_MG_STC_MM0			5
#define CLK_SUBSYS_MG_STC_PVR1			6
#define CLK_SUBSYS_MG_STC_PVR2			7
#define CLK_SUBSYS_MG_STC_FIQ0			8
#define CLK_SUBSYS_MG_TSP_FORCE_EN0		9
#define CLK_SUBSYS_MG_TSP_FORCE_EN1		10
#define CLK_SUBSYS_MG_TSO_FORCE_EN0		11
#define CLK_SUBSYS_MG_TSO_FORCE_EN1		12
#define CLK_SUBSYS_MG_TSP_GATING		13
#define CLK_SUBSYS_MG_TSP_MIU_GATING	14
#define CLK_SUBSYS_MG_TSO_GATING		15
#define CLK_SUBSYS_MG_TSO_MIU_GATING	16
#define CLK_SUBSYS_MG_NR				17

#endif /* _DT_BINDINGS_CLK_MT5870_H */
