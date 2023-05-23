// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/kernel.h>

#include "mtk_pcmcia_hal_diff.h"

#include "mtk_pcmcia_drv.h"
#include "mtk_pcmcia_hal.h"
#include "mtk_pcmcia_private.h"


#include "mtk_pcmcia_stub.h"

#define STUB_CIS_ADDR_FF 0xff
#define SIZE_HIGH_DATA 0x00
#define SIZE_LOW_DATA 0x02

#define DATA_TRANS_SIZE_LOW_CHECK 5


static const unsigned char golden_cis[105] = {
						0x1D, 0x04, 0x00, 0xDB, 0x08, 0xFF, 0x1C, 0x03,
						0x00, 0x08, 0xFF, 0x15, 0x12, 0x05, 0x00, 0x4E,
						0x45, 0x4F, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43,
						0x41, 0x4D, 0x00, 0x43, 0x49, 0x00, 0xFF, 0x20,
						0x04, 0xFF, 0xFF, 0x01, 0x00, 0x1A, 0x15, 0x01,
						0x0F, 0xFE, 0x01, 0x01, 0xC0, 0x0E, 0x41, 0x02,
						0x44, 0x56, 0x42, 0x5F, 0x43, 0x49, 0x5F, 0x56,
						0x31, 0x2E, 0x30, 0x30, 0x1B, 0x26, 0xCF, 0x04,
						0x19, 0x37, 0x55, 0x4D, 0x5D, 0x56, 0x56, 0x22,
						0x20, 0xC0, 0x09, 0x44, 0x56, 0x42, 0x5F, 0x48,
						0x4F, 0x53, 0x54, 0x00, 0xC1, 0x0E, 0x44, 0x56,
						0x42, 0x5F, 0x43, 0x49, 0x5F, 0x4D, 0x4F, 0x44,
						0x55, 0x4C, 0x45, 0x00, 0x14, 0x00, 0xff, 0x44};

unsigned short cis_addr;
unsigned char  low_addr;
unsigned char  high_addr;
unsigned char  write_io_addr;
unsigned char  write_io_value;
unsigned char  read_io_addr;
unsigned char  read_io_value;

unsigned char  data_trans_flg;

unsigned char  status_trans_value_index;
#define STATUS_TRANS_MAX_SIZE 4
#define STATUS_TRANS_INDEX_CHECK 2
unsigned char  status_trans_value[STATUS_TRANS_MAX_SIZE] = {
				PCMCIA_STATUS_DATAAVAILABLE,
				PCMCIA_STATUS_DATAAVAILABLE, // CHECK RE=0
				PCMCIA_STATUS_FREE,
				PCMCIA_STATUS_FREE
				};

unsigned char  size_high_trans_value_index;
unsigned char  size_high_trans_value[1] = {
				0x00};//size read

unsigned char  size_low_trans_value_index;
unsigned char  size_low_trans_value[1] = {
				0x09};//size read

unsigned char  data_trans_value_index;
#define DATA_TRANS_MAX_SIZE 9
static const unsigned char  data_trans_value[9] = {
				0x01, 0x00, 0x83, 0x01, 0x01, 0x80, 0x02, 0x01, 0x00}; //size read
//============================================================

unsigned char  status_init_value_index;
#define STATUS_INIT_MAX_SIZE 14
#define STATUS_INIT_INDEX_CHECK 12
unsigned char  status_init_value[STATUS_INIT_MAX_SIZE] = {
				PCMCIA_STATUS_FREE, //PCMCIA_COMMAND_RESET
				PCMCIA_STATUS_DATAAVAILABLE, //PCMCIA_COMMAND_SIZEREAD
				PCMCIA_STATUS_DATAAVAILABLE,
				PCMCIA_STATUS_BUSY, // CHECK RE=0
				PCMCIA_STATUS_FREE, //PCMCIA_COMMAND_SIZEWRITE
				PCMCIA_STATUS_FREE,
				PCMCIA_STATUS_FREE, //PCMCIA_COMMAND_HOSTCONTROL
				PCMCIA_STATUS_FREE,
				PCMCIA_STATUS_BUSY, // CHECK WE=0
				PCMCIA_STATUS_FREE,
				PCMCIA_STATUS_FREE,
				PCMCIA_STATUS_FREE,
				PCMCIA_STATUS_FREE,
				PCMCIA_STATUS_FREE
				};

unsigned char  size_high_init_value_index;
unsigned char  size_high_init_value[1] = {
				SIZE_HIGH_DATA};//size read

unsigned char  size_low_init_value_index;
unsigned char  size_low_init_value[1] = {
				SIZE_LOW_DATA};//size read

unsigned char  data_init_value_index;
#define DATA_INIT_MAX_SIZE 2
static const unsigned char  data_init_value[DATA_INIT_MAX_SIZE] = {
				0x10, 0x00}; //size read





void set_stub_statue(STUB_STATE stub_state, STUB_ACCESS stub_access, unsigned int u32Addr,
				   unsigned char *value, unsigned char expert_value)
{
	switch (stub_state) {
	case STUB_STATE_CARD_DETECT:
		*value = expert_value;
		//pr_info("[%s][%d] *value = %x\n", __func__, __LINE__, expert_value);
		break;
	case STUB_STATE_CARD_RESET:
		*value = expert_value;
		//pr_info("[%s][%d] *value = %x\n", __func__, __LINE__, expert_value);
		break;
	case STUB_STATE_READ_CIS:
		switch (stub_access) {
		case STUB_WRITE:
			switch (u32Addr) {
			case REG_PCMCIA_ADDR1:
				high_addr = *value;
				break;
			case REG_PCMCIA_ADDR0:
				low_addr = *value;
				break;
			}
			break;
		case STUB_READ:
			cis_addr = low_addr;
			if (cis_addr == STUB_CIS_ADDR_FF)
				*value = 0x0f;
			else
				*value = golden_cis[cis_addr];
			//pr_info("[%s][%d] cis[%d] = %x\n", __func__, __LINE__, cis_addr, *value);
			break;
		}
		break;
	case STUB_STATE_WRITE_IO_MEM:
		if (u32Addr == REG_PCMCIA_ADDR1)
			high_addr = *value;
		if (u32Addr == REG_PCMCIA_ADDR0)
			low_addr = *value;
		if (u32Addr == REG_PCMCIA_WRITE_DATA) {
			write_io_value = *value;
			write_io_addr = low_addr;
			switch (write_io_addr) {
			case PCMCIA_PHYS_REG_COMMANDSTATUS:
				if (write_io_value & PCMCIA_COMMAND_RESET) {
					status_init_value_index = 0;
					size_high_init_value_index = 0;
					size_low_init_value_index = 0;
					data_init_value_index = 0;

					data_trans_flg = 0;
					status_trans_value_index = 0;
					size_high_trans_value_index = 0;
					size_low_trans_value_index = 0;
					data_trans_value_index = 0;
				}
				break;
			case PCMCIA_PHYS_REG_SIZELOW:
				if (write_io_value == DATA_TRANS_SIZE_LOW_CHECK)
					data_trans_flg = 1;
				break;

			}

		}
		break;
	case STUB_STATE_READ_IO_MEM:
		switch (stub_access) {
		case STUB_WRITE:
			switch (u32Addr) {
			case REG_PCMCIA_ADDR1:
				high_addr = *value;
				break;
			case REG_PCMCIA_ADDR0:
				low_addr = *value;
				break;
			}
			break;
		case STUB_READ:
			read_io_addr = low_addr;
			switch (read_io_addr) {
			case PCMCIA_PHYS_REG_COMMANDSTATUS:
				if (data_trans_flg) {
					//pr_info("[%s][%d] status_trans_value_index=%d\n",
					//__func__, __LINE__, status_trans_value_index);
					if (status_trans_value_index >= STATUS_TRANS_INDEX_CHECK)
						*value =
						status_trans_value[status_trans_value_index];
					else
						*value =
						status_trans_value[status_trans_value_index++];
				} else {
					//pr_info("[%s][%d] status_init_value_index=%d\n",
					//__func__, __LINE__, status_init_value_index);
					if (status_init_value_index >= STATUS_INIT_INDEX_CHECK)
						*value =
						status_init_value[status_init_value_index];
					else
						*value =
						status_init_value[status_init_value_index++];
				}
				break;
			case PCMCIA_PHYS_REG_SIZELOW:
				if (data_trans_flg)
					*value =
					size_low_trans_value[size_low_trans_value_index++];
				else
					*value =
					size_low_init_value[size_low_init_value_index++];
				break;
			case PCMCIA_PHYS_REG_SIZEHIGH:
				if (data_trans_flg)
					*value =
					size_high_trans_value[size_high_trans_value_index++];
				else
					*value =
					size_high_init_value[size_high_init_value_index++];
				break;
			case PCMCIA_PHYS_REG_DATA:
				if (data_trans_flg)
					*value =
					data_trans_value[data_trans_value_index++];
				else
					*value =
					data_init_value[data_init_value_index++];
				break;
			}

			break;
		}
		break;
	}
}

