/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DEMOD_HAL_REG_VIF_H_
#define _DEMOD_HAL_REG_VIF_H_

// ========== Register Definition =====================
/*
 * #define TMP_RIU_BASE_ADDRESS   0x1F200000
 * #define ADC_REG_BASE                0x12800UL
 * #define DMDTOP_REG_BASE             0x12000UL
 * #define RF_REG_BASE                 0x12100UL
 * #define DBB1_REG_BASE               0x12200UL
 * #define DBB2_REG_BASE               0x12300UL
 * #define DBB3_REG_BASE               0x12400UL
 */
//#define TMP_RIU_BASE_ADDRESS   0x1F000000
#define ADC_REG_BASE                0x5C00//0x112800UL
#define DMDTOP_REG_BASE             0x4000//0x112000UL
#define RF_REG_BASE                 0x3C00//0x112100UL
#define DBB1_REG_BASE               0x3000//0x112200UL
#define DBB2_REG_BASE               0x3200//0x112300UL
#define DBB3_REG_BASE               0x3400//0x112400UL

#define VIF_SLAVE_ADC_REG_BASE      0x2E00
#define VIF_SLAVE_RF_REG_BASE       0x1E00
#define VIF_SLAVE_DBB1_REG_BASE     0x1800
#define VIF_SLAVE_DBB2_REG_BASE     0x1900
#define VIF_SLAVE_DBB3_REG_BASE     0x1A00
#define VIF_SLAVE_DMDTOP_REG_BASE   0x2000

///////////////////////////////////////////////////////
// [ADC bank register]
///////////////////////////////////////////////////////

#define RFAGC_ENABLE                (ADC_REG_BASE+0x6C)
#define RFAGC_ODMODE                (ADC_REG_BASE+0x6C)
#define RFAGC_DATA_SEL              (ADC_REG_BASE+0x6C)
#define IFAGC_ENABLE                (ADC_REG_BASE+0x6C)
#define IFAGC_ODMODE                (ADC_REG_BASE+0x6C)
#define IFAGC_DATA_SEL              (ADC_REG_BASE+0x6C)
///////////////////////////////////////////////////////
// [RF bank register]
///////////////////////////////////////////////////////

#define RF_LOAD                     (RF_REG_BASE+0x02)
#define IFAGC_POLARITY              (RF_REG_BASE+0x14)
#define IFAGC_SEL_SECONDER          (RF_REG_BASE+0x15)
#define IFAGC_DITHER_EN             (RF_REG_BASE+0x15)
#define RFAGC_POLARITY              (RF_REG_BASE+0x16)
#define RFAGC_SEL_SECONDER          (RF_REG_BASE+0x17)
#define RFAGC_DITHER_EN             (RF_REG_BASE+0x17)
#define OREN_PGA2_S                 (RF_REG_BASE+0x19)
#define OREN_PGA1_S                 (RF_REG_BASE+0x19)
#define OREN_PGA2_V                 (RF_REG_BASE+0x19)
#define OREN_PGA1_V                 (RF_REG_BASE+0x19)
#define OREN_RFAGC                  (RF_REG_BASE+0x19)
#define OREN_IFAGC                  (RF_REG_BASE+0x19)
#define CLAMPGAIN_CLAMP_OVERWRITE   (RF_REG_BASE+0x46)
#define CLAMPGAIN_GAIN_OVERWRITE    (RF_REG_BASE+0x48)
#define VIF_RF_RESERVED_1           (RF_REG_BASE+0xD2)
#define VIF_RF_RESERVED_2           (RF_REG_BASE+0xD4)
#define VIF_RF_RESERVED_3           (RF_REG_BASE+0xD6)
/////////////////////////////////////////////////////////
// [DBB1 bank register]
/////////////////////////////////////////////////////////

#define MODULATION_TYPE             (DBB1_REG_BASE+0x02)
#define DBB1_LOAD                   (DBB1_REG_BASE+0x10)
#define CR_K_SEL                    (DBB1_REG_BASE+0x20)
#define CR_KP_SW                    (DBB1_REG_BASE+0x22)
#define CR_RATE                     (DBB1_REG_BASE+0x24)
#define CR_IIR_COEF_G               (DBB1_REG_BASE+0x50)
#define CR_IIR_COEF_A1              (DBB1_REG_BASE+0x52)
#define CR_IIR_COEF_A2              (DBB1_REG_BASE+0x54)
#define CR_IIR_COEF_B1              (DBB1_REG_BASE+0x56)
#define CR_IIR_COEF_B2              (DBB1_REG_BASE+0x58)
#define N_A1_C0_L                   (DBB1_REG_BASE+0x84)
#define N_A1_C0_H                   (DBB1_REG_BASE+0x85)
#define N_A1_C1_L                   (DBB1_REG_BASE+0x86)
#define N_A1_C1_H                   (DBB1_REG_BASE+0x87)
#define N_A1_C2_L                   (DBB1_REG_BASE+0x88)
#define N_A1_C2_H                   (DBB1_REG_BASE+0x89)
#define N_A2_C0_L                   (DBB1_REG_BASE+0x8A)
#define N_A2_C0_H                   (DBB1_REG_BASE+0x8B)
#define N_A2_C1_L                   (DBB1_REG_BASE+0x8C)
#define N_A2_C1_H                   (DBB1_REG_BASE+0x8D)
#define N_A2_C2_L                   (DBB1_REG_BASE+0x8E)
#define N_A2_C2_H                   (DBB1_REG_BASE+0x8F)
#define SOS11_C0_L                  (DBB1_REG_BASE+0x96)
#define SOS11_C0_H                  (DBB1_REG_BASE+0x97)
#define SOS11_C1_L                  (DBB1_REG_BASE+0x98)
#define SOS11_C1_H                  (DBB1_REG_BASE+0x99)
#define SOS11_C2_L                  (DBB1_REG_BASE+0x9A)
#define SOS11_C2_H                  (DBB1_REG_BASE+0x9B)
#define SOS11_C3_L                  (DBB1_REG_BASE+0x9C)
#define SOS11_C3_H                  (DBB1_REG_BASE+0x9D)
#define SOS11_C4_L                  (DBB1_REG_BASE+0x9E)
#define SOS11_C4_H                  (DBB1_REG_BASE+0x9F)
#define SOS12_C0_L                  (DBB1_REG_BASE+0xA0)
#define SOS12_C0_H                  (DBB1_REG_BASE+0xA1)
#define SOS12_C1_L                  (DBB1_REG_BASE+0xA2)
#define SOS12_C1_H                  (DBB1_REG_BASE+0xA3)
#define SOS12_C2_L                  (DBB1_REG_BASE+0xA4)
#define SOS12_C2_H                  (DBB1_REG_BASE+0xA5)
#define SOS12_C3_L                  (DBB1_REG_BASE+0xB2)
#define SOS12_C3_H                  (DBB1_REG_BASE+0xB3)
#define SOS12_C4_L                  (DBB1_REG_BASE+0xB4)
#define SOS12_C4_H                  (DBB1_REG_BASE+0xB5)
#define SOS21_C0_L                  (DBB1_REG_BASE+0xB6)
#define SOS21_C0_H                  (DBB1_REG_BASE+0xB7)
#define SOS21_C1_L                  (DBB1_REG_BASE+0xB8)
#define SOS21_C1_H                  (DBB1_REG_BASE+0xB9)
#define SOS21_C2_L                  (DBB1_REG_BASE+0xBA)
#define SOS21_C2_H                  (DBB1_REG_BASE+0xBB)
#define SOS21_C3_L                  (DBB1_REG_BASE+0xBC)
#define SOS21_C3_H                  (DBB1_REG_BASE+0xBD)
#define SOS21_C4_L                  (DBB1_REG_BASE+0xBE)
#define SOS21_C4_H                  (DBB1_REG_BASE+0xBF)
#define SOS22_C0_L                  (DBB1_REG_BASE+0xC0)
#define SOS22_C0_H                  (DBB1_REG_BASE+0xC1)
#define SOS22_C1_L                  (DBB1_REG_BASE+0xC2)
#define SOS22_C1_H                  (DBB1_REG_BASE+0xC3)
#define SOS22_C2_L                  (DBB1_REG_BASE+0xC4)
#define SOS22_C2_H                  (DBB1_REG_BASE+0xC5)
#define SOS22_C3_L                  (DBB1_REG_BASE+0xC6)
#define SOS22_C3_H                  (DBB1_REG_BASE+0xC7)
#define SOS22_C4_L                  (DBB1_REG_BASE+0xC8)
#define SOS22_C4_H                  (DBB1_REG_BASE+0xC9)
#define SOS31_C0_L                  (DBB1_REG_BASE+0xCA)
#define SOS31_C0_H                  (DBB1_REG_BASE+0xCB)
#define SOS31_C1_L                  (DBB1_REG_BASE+0xCC)
#define SOS31_C1_H                  (DBB1_REG_BASE+0xCD)
#define SOS31_C2_L                  (DBB1_REG_BASE+0xCE)
#define SOS31_C2_H                  (DBB1_REG_BASE+0xCF)
#define SOS31_C3_L                  (DBB1_REG_BASE+0xD0)
#define SOS31_C3_H                  (DBB1_REG_BASE+0xD1)
#define SOS31_C4_L                  (DBB1_REG_BASE+0xD2)
#define SOS31_C4_H                  (DBB1_REG_BASE+0xD3)
#define SOS32_C0_L                  (DBB1_REG_BASE+0xD4)
#define SOS32_C0_H                  (DBB1_REG_BASE+0xD5)
#define SOS32_C1_L                  (DBB1_REG_BASE+0xD6)
#define SOS32_C1_H                  (DBB1_REG_BASE+0xD7)
#define SOS32_C2_L                  (DBB1_REG_BASE+0xD8)
#define SOS32_C2_H                  (DBB1_REG_BASE+0xD9)
#define SOS32_C3_L                  (DBB1_REG_BASE+0xDA)
#define SOS32_C3_H                  (DBB1_REG_BASE+0xDB)
#define SOS32_C4_L                  (DBB1_REG_BASE+0xDC)
#define SOS32_C4_H                  (DBB1_REG_BASE+0xDD)
#define SOS33_C0_L                  (DBB1_REG_BASE+0xDE)
#define SOS33_C0_H                  (DBB1_REG_BASE+0xDF)
#define SOS33_C1_L                  (DBB1_REG_BASE+0xE0)
#define SOS33_C1_H                  (DBB1_REG_BASE+0xE1)
#define SOS33_C2_L                  (DBB1_REG_BASE+0xE2)
#define SOS33_C2_H                  (DBB1_REG_BASE+0xE3)
#define SOS33_C3_L                  (DBB1_REG_BASE+0xE4)
#define SOS33_C3_H                  (DBB1_REG_BASE+0xE5)
#define SOS33_C4_L                  (DBB1_REG_BASE+0xE6)
#define SOS33_C4_H                  (DBB1_REG_BASE+0xE7)
#define CR_PD_IMAG_INV              (DBB1_REG_BASE+0xF4)
#define FIRMWARE_VERSION_L          (DBB1_REG_BASE+0xFA)
#define FIRMWARE_VERSION_H          (DBB1_REG_BASE+0xFB)
/////////////////////////////////////////////////////////
// [DBB2 bank register]
/////////////////////////////////////////////////////////

#define AGC_GAIN_SLOPE               (DBB2_REG_BASE+0x00)
#define AGC_REF                     (DBB2_REG_BASE+0x08)
#define AGC_PGA2_OREN               (DBB2_REG_BASE+0x0C)
#define AGC_PGA2_OV                 (DBB2_REG_BASE+0x0F)
#define AGC_PGA2_MIN                (DBB2_REG_BASE+0x14)
#define DBB2_LOAD                   (DBB2_REG_BASE+0x1E)
#define AGC_MEAN256                 (DBB2_REG_BASE+0x24)
#define CR_RATE_INV                 (DBB2_REG_BASE+0x4B)
#define IF_RATE                     (DBB2_REG_BASE+0x58)
#define A_DAGC_SEL                  (DBB2_REG_BASE+0x5C)
#define BYPASS_EQFIR                (DBB2_REG_BASE+0x96)
#define AAGC_PGA2_MIN               (DBB2_REG_BASE+0xAA)
//////////////////////////////////////////////////////////
// [DBB3 bank register]
//////////////////////////////////////////////////////////
#define VAGC_IF_VGA_L			(DBB3_REG_BASE+0x60)
#define VAGC_IF_VGA_H			(DBB3_REG_BASE+0x61)


#endif //_DEMOD_HAL_REG_VIF_H_
