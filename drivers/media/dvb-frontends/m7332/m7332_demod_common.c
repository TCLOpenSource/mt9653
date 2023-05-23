// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <asm/io.h>
#include <linux/delay.h>
#include "m7332_demod_common.h"


#define RIU_READ_BYTE(addr)       (readb(virt_riu_addr + (addr)))
#define RIU_WRITE_BYTE(addr, val)   { writeb(val, virt_riu_addr + (addr)); }

#define RIU_READBYTE(reg)   RIU_READ_BYTE(((reg) << 1) - ((reg) & 1))

#define RIU_WRITEBYTE(reg, val) \
		(RIU_WRITE_BYTE(((reg) << 1) - ((reg) & 1), val))

#define RIU_WRITE_BIT(reg, enable, mask) \
	(RIU_WRITE_BYTE((((reg) << 1) - ((reg) & 1)), (enable) \
		? (RIU_READ_BYTE((((reg) << 1) - ((reg) & 1))) | (mask)) \
		: (RIU_READ_BYTE((((reg) << 1) - ((reg) & 1))) & ~(mask))))

#define RIU_WRITE_BYTE_MASK(reg, u8Val, mask)  \
	(RIU_WRITE_BYTE((((reg) << 1) - ((reg) & 1)), \
		(RIU_READ_BYTE((((reg) << 1) - ((reg) & 1))) & ~(mask)) \
		| ((u8Val) & (mask))))


#define DMD_MBX_TIMEOUT      200
#define MAX_INT              0x7FFFFFFF
#define POW2_62              0x4000000000000000  /*  2^62  */

static u8 *virt_riu_addr;


void mtk_demod_init_riuaddr(struct dvb_frontend *fe)
{
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;

	virt_riu_addr = demod_prvi->virt_riu_base_addr;
}

/*-----------------------------------------------------------------------------
 * Suspend the calling task for u32Ms milliseconds
 * @param  time_ms  \b IN: delay 1 ~ MSOS_WAIT_FOREVER ms
 * @return None
 * @note   Not allowed in interrupt context; otherwise, exception will occur.
 *----------------------------------------------------------------------------
 */
void mtk_demod_delay_ms(u32 time_ms)
{
	mtk_demod_delay_us(time_ms * 1000UL);
}

/*-----------------------------------------------------------------------------
 *  Delay for u32Us microseconds
 *  @param  time_us  \b IN: delay 0 ~ 999 us
 *  @return None
 *  @note   implemented by "busy waiting".
 *         Plz call MsOS_DelayTask directly for ms-order delay
 *----------------------------------------------------------------------------
 */
void mtk_demod_delay_us(u32 time_us)
{
	/*
	 * Refer to Documentation/timers/timers-howto.txt
	 * Non-atomic context
	 *    < 10us  : udelay()
	 * 10us ~ 20ms  : usleep_range()
	 *    > 10ms  : msleep() or msleep_interruptible()
	 */
	if (time_us < 10UL)
		udelay(time_us);
	else if (time_us < 20UL * 1000UL)
		usleep_range(time_us, time_us+1);
	else
		msleep_interruptible((unsigned int)(time_us / 1000UL));
}

/*----------------------------------------------------------------------------
 * Get current system time in ms
 * @return system time in ms
 *----------------------------------------------------------------------------
 */
unsigned int mtk_demod_get_time(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);
	return ts.tv_sec*1000 + ts.tv_nsec/1000000;
}

/*-----------------------------------------------------------------------------
 *  Time difference between current time and task time
 *  @return system time diff in ms
 *----------------------------------------------------------------------------
 */
int mtk_demod_dvb_time_diff(u32 time)
{
	return (mtk_demod_get_time() - time);
}


static int mtk_demod_mbx_dvb_wait_ready(void)
{
	u32 start_time = mtk_demod_get_time();

	while (RIU_READBYTE(MB_REG_BASE + 0x00)) {
		/* wait VDMCU ready */
		if (mtk_demod_dvb_time_diff(start_time)
			> DMD_MBX_TIMEOUT) {
			pr_err("HAL_SYS_DMD_VD_MBX_DVB_WaitReady Timeout\n");
			return -ETIMEDOUT;
		}
	}
	return 0;
}

int mtk_demod_mbx_dvb_wait_handshake(void)
{
	u32 start_time = mtk_demod_get_time();

	while (RIU_READBYTE(MB_REG_BASE + 0x00) != 0xFF) {
		/* wait MB_CNTL set done */
		if (mtk_demod_dvb_time_diff(start_time)
			> DMD_MBX_TIMEOUT) {
			pr_err("HAL_SYS_DMD_VD_MBX_DVB_WaitHandShake Timeout\n");
			return -ETIMEDOUT;
		}
	}
	return 0;
}

int mtk_demod_mbx_dvb_write(u16 addr, u8 value)
{
	if (mtk_demod_mbx_dvb_wait_ready())
		return -ETIMEDOUT;

	/* ADDR_H */
	RIU_WRITEBYTE(MB_REG_BASE + 0x02, (u8)(addr >> 8));
	/* ADDR_L */
	RIU_WRITEBYTE(MB_REG_BASE + 0x01, (u8)addr);
	/* REG_DATA */
	RIU_WRITEBYTE(MB_REG_BASE + 0x03, value);
	/* MB_CNTL set write mode */
	RIU_WRITEBYTE(MB_REG_BASE + 0x00, 0x02);

	/* assert interrupt to DMD51 */
	RIU_WRITEBYTE(DMD51_REG_BASE + 0x03, 0x02);
	/* de-assert interrupt to DMD51 */
	RIU_WRITEBYTE(DMD51_REG_BASE + 0x03, 0x00);

	if (mtk_demod_mbx_dvb_wait_handshake())
		return -ETIMEDOUT;

	/* MB_CNTL clear */
	RIU_WRITEBYTE(MB_REG_BASE + 0x00, 0x00);
	return 0;
}

int mtk_demod_mbx_dvb_read(u16 addr, u8 *value)
{
	if (mtk_demod_mbx_dvb_wait_ready())
		return -ETIMEDOUT;

	/* ADDR_H */
	RIU_WRITEBYTE(MB_REG_BASE + 0x02, (u8)(addr >> 8));
	/* ADDR_L */
	RIU_WRITEBYTE(MB_REG_BASE + 0x01, (u8)addr);
	/* MB_CNTL set read mode */
	RIU_WRITEBYTE(MB_REG_BASE + 0x00, 0x01);

	/* assert interrupt to DMD51 */
	RIU_WRITEBYTE(DMD51_REG_BASE + 0x03, 0x02);
	/* de-assert interrupt to DMD51 */
	RIU_WRITEBYTE(DMD51_REG_BASE + 0x03, 0x00);

	if (mtk_demod_mbx_dvb_wait_handshake())
		return -ETIMEDOUT;

	/* REG_DATA get */
	*value = RIU_READBYTE(MB_REG_BASE + 0x03);
	/* MB_CNTL clear */
	RIU_WRITEBYTE(MB_REG_BASE + 0x00, 0x00);

	return 0;
}

int mtk_demod_mbx_dvb_write_dsp(u16 addr, u8 value)
{
	if (mtk_demod_mbx_dvb_wait_ready())
		return -ETIMEDOUT;

	/* ADDR_H */
	RIU_WRITEBYTE(MB_REG_BASE + 0x02, (u8)(addr >> 8));
	/* ADDR_L */
	RIU_WRITEBYTE(MB_REG_BASE + 0x01, (u8)addr);
	/* REG_DATA */
	RIU_WRITEBYTE(MB_REG_BASE + 0x03, value);
	/* MB_CNTL set write mode */
	RIU_WRITEBYTE(MB_REG_BASE + 0x00, 0x04);

	/* assert interrupt to DMD51 */
	RIU_WRITEBYTE(DMD51_REG_BASE + 0x03, 0x02);
	/* de-assert interrupt to DMD51 */
	RIU_WRITEBYTE(DMD51_REG_BASE + 0x03, 0x00);

	if (mtk_demod_mbx_dvb_wait_handshake())
		return -ETIMEDOUT;

	/* MB_CNTL clear */
	RIU_WRITEBYTE(MB_REG_BASE + 0x00, 0x00);

	return 0;
}

int mtk_demod_mbx_dvb_read_dsp(u16 addr, u8 *value)
{
	if (mtk_demod_mbx_dvb_wait_ready())
		return -ETIMEDOUT;

	/* ADDR_H */
	RIU_WRITEBYTE(MB_REG_BASE + 0x02, (u8)(addr >> 8));
	/* ADDR_L */
	RIU_WRITEBYTE(MB_REG_BASE + 0x01, (u8)addr);
	/* MB_CNTL set read mode */
	RIU_WRITEBYTE(MB_REG_BASE + 0x00, 0x03);

	/* assert interrupt to DMD51 */
	RIU_WRITEBYTE(DMD51_REG_BASE + 0x03, 0x02);
	/* de-assert interrupt to DMD51 */
	RIU_WRITEBYTE(DMD51_REG_BASE + 0x03, 0x00);

	if (mtk_demod_mbx_dvb_wait_handshake())
		return -ETIMEDOUT;

	/* REG_DATA get */
	*value = RIU_READBYTE(MB_REG_BASE + 0x03);
	/* MB_CNTL clear */
	RIU_WRITEBYTE(MB_REG_BASE + 0x00, 0x00);

	return 0;
}

unsigned int mtk_demod_read_byte(u32 addr)
{
	return RIU_READBYTE(addr);
}

void mtk_demod_write_byte(u32 addr, u8 value)
{
	RIU_WRITEBYTE(addr, value);
}

void mtk_demod_write_bit(u32 reg, bool enable_b, u8 mask)
{
	RIU_WRITE_BIT(reg, enable_b, mask);
}

void mtk_demod_write_byte_mask(u32 reg, u8 val, u8 msk)
{
	RIU_WRITE_BYTE_MASK(reg, val, msk);
}

static u32 abs_32(s32 input)
{
	u32 result = 0;

	if (input < 0)
		result = (-1)*input;
	else
		result = input;

	return result;
}

static u64 abs_64(s64 input)
{
	u64 result = 0;

	if (input < 0)
		result = (-1)*input;
	else
		result = input;

	return result;
}

static u8 find_msb(s64 input)
{
	s8 iter = -1;
	u64 data = abs_64(input);

	while (data != 0) {
		++iter;
		data >>= 1;
	}

	if (iter >= 0)
		return iter;


	return 0;
}

static void normalize(struct ms_float_st *input)
{
	u8 lsb = 0, sign_flag = 0;
	s8 exp = 0;
	u32 data = 0;

	if ((*input).data == 0) {
		(*input).exp = 0;
	} else {
		lsb = 0;

		if ((*input).data < 0) /* negative value */
			sign_flag = 1;
		else
			sign_flag = 0;

		data = abs_32((*input).data);
		exp = (*input).exp;

		if (exp != 0) {
			while ((data & 0x01) == 0x00) {
				++lsb;
				data >>= 1;
			}

			exp += lsb;

			(*input).data = data;
			(*input).exp = exp;

			if (sign_flag == 1)
				(*input).data *= (-1);
		}
	}
}

struct ms_float_st mtk_demod_float_op(struct ms_float_st st_rn,
			struct ms_float_st st_rd, enum op_type op_code)
{
	struct ms_float_st result = {.data = 0, .exp = 0};
	s32 data1 = 0, data2 = 0;
	u32 udata1 = 0, udata2 = 0;
	s8 exp_1 = 0, exp_2 = 0, exp_diff;
	s8 iter = 0, msb = 0, msb_tmp = 0;

	s64 temp = 0;

	normalize(&st_rn);
	normalize(&st_rd);

	data1 = st_rn.data;
	data2 = st_rd.data;

	udata1 = abs_32(data1);
	udata2 = abs_32(data2);

	exp_1 = st_rn.exp;
	exp_2 = st_rd.exp;

	switch (op_code) {
	case ADD:
	{
		if (exp_1 == exp_2) {
			temp = data1;
			temp += data2;

			/* DATA overflow (32 bits) */
			if ((temp > MAX_INT) || (temp < (-1)*MAX_INT)) {
				temp >>= 1;
				result.data = temp;
				result.exp = (exp_1+1);
			} else {
				result.data = (data1+data2);
				result.exp = exp_1;
			}
		} else {
			if (exp_1 > exp_2) {
				/* get the EXP difference of st_rn and st_rd */
				exp_diff = exp_1-exp_2;

				/* get the MSB of st_rn */
				temp = data1;
				msb = find_msb(temp);

				if ((msb + exp_diff) < 62) {
					temp = data1;

					for (iter = 0; iter < exp_diff; ++iter)
						temp = (temp<<1);

					temp += data2;
				} else {
					temp = data1;

					for (iter = 0; iter < (62-msb); ++iter)
						temp = (temp<<1);

					exp_1 = exp_1-(62-msb);

					for (iter = 0; iter < (exp_1-exp_2);
						++iter) {
						data2 = (data2>>1);
					}

					exp_2 = exp_1;

					temp += data2;
				}

				/* DATA overflow (32 bits) */
				if ((temp > MAX_INT) || (temp < (-1)*MAX_INT)) {
					msb = find_msb(temp);

					if (msb >= 30)
						temp >>= (msb-30);

					result.data = temp;
					result.exp = (exp_2+(msb-30));
				} else {
					result.data = temp;
					result.exp = exp_2;
				}
			} else {
				return mtk_demod_float_op(st_rd,
						st_rn, ADD);
			}
		}
	}
		break;

	case MINUS:
	{
		st_rd.data *= (-1);
		return mtk_demod_float_op(st_rn, st_rd, ADD);
	}
		break;

	case MULTIPLY:
	{
		if (data1 == 0 || data2 == 0) {
			result.data = 0;
			result.exp = 0;
		} else {
			temp = data1;
			temp *= data2;

			if ((temp <= MAX_INT) && (temp >= (-1*MAX_INT))) {
				result.data = data1*data2;
				result.exp = exp_1+exp_2;
			} else {
				msb = find_msb(temp);
				if (msb-30 >= 0) {
					temp = temp>>(msb-30);
					result.data = (s32)temp;
					result.exp = exp_1+exp_2+(msb-30);
				} else {
					result.data = (s32)0;
					result.exp = 0;
				}
			}
		}
	}
		break;

	case DIVIDE:
	{
		if (data1 != 0 && data2 != 0) {
			if (udata1 < udata2) {
				temp = POW2_62;
				do_div(temp, data2);
				temp = temp*data1;

				msb = find_msb(temp);

				if (msb > 30) {
					temp >>= (msb-30);
					result.data = temp;
					result.exp = exp_1-exp_2+(msb-30)-62;
				} else {
					result.data = temp;
					result.exp = exp_1-exp_2-62;
				}
			} else if (udata1 == udata2) {
				result.data = data1/data2;
				result.exp = exp_1-exp_2;
			} else {
				msb = find_msb(data1);
				msb_tmp = find_msb(data2);

				exp_2 -= ((msb-msb_tmp) + 1);

				temp = POW2_62;
				if ((msb-msb_tmp) >= 0) {
					do_div(temp,
					(((s64)data2)<<((msb-msb_tmp+1))));
					temp = temp*data1;
				}

				msb = find_msb(temp);

				if (msb > 30) {
					temp >>= (msb-30);
					result.data = temp;
					result.exp = exp_1-exp_2+(msb-30)-62;
				} else {
					result.data = temp;
					result.exp = exp_1-exp_2-62;
				}
			}
		} else {
			result.data = 0;
			result.exp = 0;
		}
	}
		break;

	default:
		break;
	}

	normalize(&result);

	return result;
}

s64 mtk_demod_float_to_s64(struct ms_float_st msf_input)
{
	s64 s64result;
	u32 u32result;
	u8  offset_num;

	u32result = abs_32(msf_input.data);
	s64result = u32result;

	if (msf_input.exp > 0) {
		offset_num = (u8)msf_input.exp;
		s64result = s64result<<offset_num;
	} else {
		offset_num = (u8)(-1*msf_input.exp);
		s64result = s64result>>offset_num;
	}

	if (msf_input.data < 0)
		s64result = s64result*(-1);

	return s64result;
}
