/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _VOC_COMMON_REG_H
#define _VOC_COMMON_REG_H

#ifndef BIT0
#define BIT0  0x1
#endif
#ifndef BIT1
#define BIT1  0x2
#endif
#ifndef BIT2
#define BIT2  0x4
#endif
#ifndef BIT3
#define BIT3  0x8
#endif
#ifndef BIT4
#define BIT4  0x10
#endif
#ifndef BIT5
#define BIT5  0x20
#endif
#ifndef BIT6
#define BIT6  0x40
#endif
#ifndef BIT7
#define BIT7  0x80
#endif
#ifndef BIT8
#define BIT8  0x100
#endif
#ifndef BIT9
#define BIT9  0x200
#endif
#ifndef BIT10
#define BIT10 0x400
#endif
#ifndef BIT11
#define BIT11 0x800
#endif
#ifndef BIT12
#define BIT12 0x1000
#endif
#ifndef BIT13
#define BIT13 0x2000
#endif
#ifndef BIT14
#define BIT14 0x4000
#endif
#ifndef BIT15
#define BIT15 0x8000
#endif

#define OFFSET(x)       ((x) << 1)
#define OFFSET8(x)      ((x) * 2 - ((x) & 1)) // for readb & writeb

/* read register by word */
#define ms_readw(a)                 readw((void __iomem *)(a))
/* write register by word */
#define ms_writew(v, a)              writew(v, (void __iomem *)(a))
 /* read register by byte */
#define ms_readb(a)                 readb((void __iomem *)(a))
/* write register by byte */
#define ms_writeb(v, a)              writeb(v, (void __iomem *)(a))

#define INREG8(x)                ms_readb(x)
#define OUTREG8(x, y)            ms_writeb((unsigned char)(y), x)
#define SETREG8(x, y)            OUTREG8(x, INREG8(x)|(y))
#define CLRREG8(x, y)            OUTREG8(x, INREG8(x)&~(y))
#define INSREG8(addr, mask, val) OUTREG8(addr, ((INREG8(addr)&(~(mask))) | val))

#define INREG16(x)                ms_readw(x)
#define OUTREG16(x, y)            ms_writew((unsigned short)(y), x)
#define SETREG16(x, y)            OUTREG16(x, INREG16(x)|(y))
#define CLRREG16(x, y)            OUTREG16(x, INREG16(x)&~(y))
#define INSREG16(addr, mask, val) OUTREG16(addr, ((INREG16(addr)&(~(mask))) | val))

#define VOC_BIT_0XF0 0xF0
#define VOC_BIT_0X0F 0x0F
#define VOC_BIT_0XFF 0xFF

#define VOC_VAL_0 0
#define VOC_VAL_1 1
#define VOC_VAL_2 2
#define VOC_VAL_3 3
#define VOC_VAL_4 4
#define VOC_VAL_5 5
#define VOC_VAL_6 6
#define VOC_VAL_7 7
#define VOC_VAL_8 8
#define VOC_VAL_9 9
#define VOC_VAL_10 10
#define VOC_VAL_11 11
#define VOC_VAL_12 12
#define VOC_VAL_13 13
#define VOC_VAL_14 14
#define VOC_VAL_15 15
#define VOC_VAL_16 16
#define VOC_VAL_17 17
#define VOC_VAL_18 18
#define VOC_VAL_19 19
#define VOC_VAL_20 20

#define VOC_REG_0X00    (0x00)
#define VOC_REG_0X01    (0x01)
#define VOC_REG_0X02    (0x02)
#define VOC_REG_0X03    (0x03)
#define VOC_REG_0X04    (0x04)
#define VOC_REG_0X05    (0x05)
#define VOC_REG_0X06    (0x06)
#define VOC_REG_0X07    (0x07)
#define VOC_REG_0X08    (0x08)
#define VOC_REG_0X09    (0x09)
#define VOC_REG_0X0A    (0x0A)
#define VOC_REG_0X0B    (0x0B)
#define VOC_REG_0X0C    (0x0C)
#define VOC_REG_0X0D    (0x0D)
#define VOC_REG_0X0E    (0x0E)
#define VOC_REG_0X0F    (0x0F)
#define VOC_REG_0X10    (0x10)
#define VOC_REG_0X11    (0x11)
#define VOC_REG_0X12    (0x12)
#define VOC_REG_0X13    (0x13)
#define VOC_REG_0X14    (0x14)
#define VOC_REG_0X15    (0x15)
#define VOC_REG_0X16    (0x16)
#define VOC_REG_0X17    (0x17)
#define VOC_REG_0X18    (0x18)
#define VOC_REG_0X19    (0x19)
#define VOC_REG_0X1A    (0x1A)
#define VOC_REG_0X1B    (0x1B)
#define VOC_REG_0X1C    (0x1C)
#define VOC_REG_0X1D    (0x1D)
#define VOC_REG_0X1E    (0x1E)
#define VOC_REG_0X1F    (0x1F)
#define VOC_REG_0X20    (0x20)
#define VOC_REG_0X21    (0x21)
#define VOC_REG_0X22    (0x22)
#define VOC_REG_0X23    (0x23)
#define VOC_REG_0X24    (0x24)
#define VOC_REG_0X25    (0x25)
#define VOC_REG_0X26    (0x26)
#define VOC_REG_0X27    (0x27)
#define VOC_REG_0X28    (0x28)
#define VOC_REG_0X29    (0x29)
#define VOC_REG_0X2A    (0x2A)
#define VOC_REG_0X2B    (0x2B)
#define VOC_REG_0X2C    (0x2C)
#define VOC_REG_0X2D    (0x2D)
#define VOC_REG_0X2E    (0x2E)
#define VOC_REG_0X2F    (0x2F)
#define VOC_REG_0X30    (0x30)
#define VOC_REG_0X31    (0x31)
#define VOC_REG_0X32    (0x32)
#define VOC_REG_0X33    (0x33)
#define VOC_REG_0X34    (0x34)
#define VOC_REG_0X35    (0x35)
#define VOC_REG_0X36    (0x36)
#define VOC_REG_0X37    (0x37)
#define VOC_REG_0X38    (0x38)
#define VOC_REG_0X39    (0x39)
#define VOC_REG_0X3A    (0x3A)
#define VOC_REG_0X3B    (0x3B)
#define VOC_REG_0X3C    (0x3C)
#define VOC_REG_0X3D    (0x3D)
#define VOC_REG_0X3E    (0x3E)
#define VOC_REG_0X3F    (0x3F)
#define VOC_REG_0X40    (0x40)
#define VOC_REG_0X41    (0x41)
#define VOC_REG_0X42    (0x42)
#define VOC_REG_0X43    (0x43)
#define VOC_REG_0X44    (0x44)
#define VOC_REG_0X45    (0x45)
#define VOC_REG_0X46    (0x46)
#define VOC_REG_0X47    (0x47)
#define VOC_REG_0X48    (0x48)
#define VOC_REG_0X49    (0x49)
#define VOC_REG_0X4A    (0x4A)
#define VOC_REG_0X4B    (0x4B)
#define VOC_REG_0X4C    (0x4C)
#define VOC_REG_0X4D    (0x4D)
#define VOC_REG_0X4E    (0x4E)
#define VOC_REG_0X4F    (0x4F)
#define VOC_REG_0X50    (0x50)
#define VOC_REG_0X51    (0x51)
#define VOC_REG_0X52    (0x52)
#define VOC_REG_0X53    (0x53)
#define VOC_REG_0X54    (0x54)
#define VOC_REG_0X55    (0x55)
#define VOC_REG_0X56    (0x56)
#define VOC_REG_0X57    (0x57)
#define VOC_REG_0X58    (0x58)
#define VOC_REG_0X59    (0x59)
#define VOC_REG_0X5A    (0x5A)
#define VOC_REG_0X5B    (0x5B)
#define VOC_REG_0X5C    (0x5C)
#define VOC_REG_0X5D    (0x5D)
#define VOC_REG_0X5E    (0x5E)
#define VOC_REG_0X5F    (0x5F)
#define VOC_REG_0X60    (0x60)
#define VOC_REG_0X61    (0x61)
#define VOC_REG_0X62    (0x62)
#define VOC_REG_0X63    (0x63)
#define VOC_REG_0X64    (0x64)
#define VOC_REG_0X65    (0x65)
#define VOC_REG_0X66    (0x66)
#define VOC_REG_0X67    (0x67)
#define VOC_REG_0X68    (0x68)
#define VOC_REG_0X69    (0x69)
#define VOC_REG_0X6A    (0x6A)
#define VOC_REG_0X6B    (0x6B)
#define VOC_REG_0X6C    (0x6C)
#define VOC_REG_0X6D    (0x6D)
#define VOC_REG_0X6E    (0x6E)
#define VOC_REG_0X6F    (0x6F)
#define VOC_REG_0X70    (0x70)
#define VOC_REG_0X71    (0x71)
#define VOC_REG_0X72    (0x72)
#define VOC_REG_0X73    (0x73)
#define VOC_REG_0X74    (0x74)
#define VOC_REG_0X75    (0x75)
#define VOC_REG_0X76    (0x76)
#define VOC_REG_0X77    (0x77)
#define VOC_REG_0X78    (0x78)
#define VOC_REG_0X79    (0x79)
#define VOC_REG_0X7A    (0x7A)
#define VOC_REG_0X7B    (0x7B)
#define VOC_REG_0X7C    (0x7C)
#define VOC_REG_0X7D    (0x7D)
#define VOC_REG_0X7E    (0x7E)
#define VOC_REG_0X7F    (0x7F)
#define VOC_REG_0X80    (0x80)
#define VOC_REG_0X81    (0x81)
#define VOC_REG_0X82    (0x82)
#define VOC_REG_0X83    (0x83)
#define VOC_REG_0X84    (0x84)
#define VOC_REG_0X85    (0x85)
#define VOC_REG_0X86    (0x86)
#define VOC_REG_0X87    (0x87)
#define VOC_REG_0X88    (0x88)
#define VOC_REG_0X89    (0x89)
#define VOC_REG_0X8A    (0x8A)
#define VOC_REG_0X8B    (0x8B)
#define VOC_REG_0X8C    (0x8C)
#define VOC_REG_0X8D    (0x8D)
#define VOC_REG_0X8E    (0x8E)
#define VOC_REG_0X8F    (0x8F)
#define VOC_REG_0X90    (0x90)
#define VOC_REG_0X91    (0x91)
#define VOC_REG_0X92    (0x92)
#define VOC_REG_0X93    (0x93)
#define VOC_REG_0X94    (0x94)
#define VOC_REG_0X95    (0x95)
#define VOC_REG_0X96    (0x96)
#define VOC_REG_0X97    (0x97)
#define VOC_REG_0X98    (0x98)
#define VOC_REG_0X99    (0x99)
#define VOC_REG_0X9A    (0x9A)
#define VOC_REG_0X9B    (0x9B)
#define VOC_REG_0X9C    (0x9C)
#define VOC_REG_0X9D    (0x9D)
#define VOC_REG_0X9E    (0x9E)
#define VOC_REG_0X9F    (0x9F)
#define VOC_REG_0XA0    (0xA0)
#define VOC_REG_0XA1    (0xA1)
#define VOC_REG_0XA2    (0xA2)
#define VOC_REG_0XA3    (0xA3)
#define VOC_REG_0XA4    (0xA4)
#define VOC_REG_0XA5    (0xA5)
#define VOC_REG_0XA6    (0xA6)
#define VOC_REG_0XA7    (0xA7)
#define VOC_REG_0XA8    (0xA8)
#define VOC_REG_0XA9    (0xA9)
#define VOC_REG_0XAA    (0xAA)
#define VOC_REG_0XAB    (0xAB)
#define VOC_REG_0XAC    (0xAC)
#define VOC_REG_0XAD    (0xAD)
#define VOC_REG_0XAE    (0xAE)
#define VOC_REG_0XAF    (0xAF)
#define VOC_REG_0XB0    (0xB0)
#define VOC_REG_0XB1    (0xB1)
#define VOC_REG_0XB2    (0xB2)
#define VOC_REG_0XB3    (0xB3)
#define VOC_REG_0XB4    (0xB4)
#define VOC_REG_0XB5    (0xB5)
#define VOC_REG_0XB6    (0xB6)
#define VOC_REG_0XB7    (0xB7)
#define VOC_REG_0XB8    (0xB8)
#define VOC_REG_0XB9    (0xB9)
#define VOC_REG_0XBA    (0xBA)
#define VOC_REG_0XBB    (0xBB)
#define VOC_REG_0XBC    (0xBC)
#define VOC_REG_0XBD    (0xBD)
#define VOC_REG_0XBE    (0xBE)
#define VOC_REG_0XBF    (0xBF)
#define VOC_REG_0XC0    (0xC0)
#define VOC_REG_0XC1    (0xC1)
#define VOC_REG_0XC2    (0xC2)
#define VOC_REG_0XC3    (0xC3)
#define VOC_REG_0XC4    (0xC4)
#define VOC_REG_0XC5    (0xC5)
#define VOC_REG_0XC6    (0xC6)
#define VOC_REG_0XC7    (0xC7)
#define VOC_REG_0XC8    (0xC8)
#define VOC_REG_0XC9    (0xC9)
#define VOC_REG_0XCA    (0xCA)
#define VOC_REG_0XCB    (0xCB)
#define VOC_REG_0XCC    (0xCC)
#define VOC_REG_0XCD    (0xCD)
#define VOC_REG_0XCE    (0xCE)
#define VOC_REG_0XCF    (0xCF)
#define VOC_REG_0XD0    (0xD0)
#define VOC_REG_0XD1    (0xD1)
#define VOC_REG_0XD2    (0xD2)
#define VOC_REG_0XD3    (0xD3)
#define VOC_REG_0XD4    (0xD4)
#define VOC_REG_0XD5    (0xD5)
#define VOC_REG_0XD6    (0xD6)
#define VOC_REG_0XD7    (0xD7)
#define VOC_REG_0XD8    (0xD8)
#define VOC_REG_0XD9    (0xD9)
#define VOC_REG_0XDA    (0xDA)
#define VOC_REG_0XDB    (0xDB)
#define VOC_REG_0XDC    (0xDC)
#define VOC_REG_0XDD    (0xDD)
#define VOC_REG_0XDE    (0xDE)
#define VOC_REG_0XDF    (0xDF)
#define VOC_REG_0XE0    (0xE0)
#define VOC_REG_0XE1    (0xE1)
#define VOC_REG_0XE2    (0xE2)
#define VOC_REG_0XE3    (0xE3)
#define VOC_REG_0XE4    (0xE4)
#define VOC_REG_0XE5    (0xE5)
#define VOC_REG_0XE6    (0xE6)
#define VOC_REG_0XE7    (0xE7)
#define VOC_REG_0XE8    (0xE8)
#define VOC_REG_0XE9    (0xE9)
#define VOC_REG_0XEA    (0xEA)
#define VOC_REG_0XEB    (0xEB)
#define VOC_REG_0XEC    (0xEC)
#define VOC_REG_0XED    (0xED)
#define VOC_REG_0XEE    (0xEE)
#define VOC_REG_0XEF    (0xEF)
#define VOC_REG_0XF0    (0xF0)
#define VOC_REG_0XF1    (0xF1)
#define VOC_REG_0XF2    (0xF2)
#define VOC_REG_0XF3    (0xF3)
#define VOC_REG_0XF4    (0xF4)
#define VOC_REG_0XF5    (0xF5)
#define VOC_REG_0XF6    (0xF6)
#define VOC_REG_0XF7    (0xF7)
#define VOC_REG_0XF8    (0xF8)
#define VOC_REG_0XF9    (0xF9)
#define VOC_REG_0XFA    (0xFA)
#define VOC_REG_0XFB    (0xFB)
#define VOC_REG_0XFC    (0xFC)
#define VOC_REG_0XFD    (0xFD)
#define VOC_REG_0XFE    (0xFE)
#define VOC_REG_0XFF    (0xFF)

#endif /* _VOC_COMMON_H */
