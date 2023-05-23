// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/string.h>
#include <linux/namei.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/ctype.h>
#include <linux/firmware.h>
#else
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#endif
//#include "MsCommon.h"
//#include "MsOS.h"
#include "drvLDM_sti.h"
#include <linux/vmalloc.h>
#include "drv_scriptmgt.h"
#include "pqu_msg.h"
#include "ext_command_render_if.h"

//#include "ULog.h"
//#include "mdrv_ldm_algorithm.h"



//#include "regLDM.h"
//#include "halLDM.h"
//#include "drvMMIO.h"
//#include "utopia.h"
//#include "sti_msos.h"
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);


//#if defined(MSOS_TYPE_LINUX)
//#include <sys/ioctl.h>
//#endif
//FIXME
#define LDMMAPLENGTH                0x300000    //mmap length


//FIXME
#define CHIP_MIU0_BUS_BASE                      0x20000000//FIXME
#define CHIP_MIU1_BUS_BASE                      1//MIU1_BASE
#define CHIP_MIU1_BUS_BASE_L                    0xA0000000UL
#define CHIP_MIU1_BUS_BASE_H                    0x200000000UL
#define CHIP_MIU2_BUS_BASE                      2//MIU2_BASE
#define CHIP_MIU2_BUS_BASE_L                    0xFFFFFFFFUL
#define CHIP_MIU2_BUS_BASE_H                    0xFFFFFFFFFUL

#define ARM_MIU0_BUS_BASE                      CHIP_MIU0_BUS_BASE
#define ARM_MIU1_BUS_BASE                      CHIP_MIU1_BUS_BASE
#define ARM_MIU2_BUS_BASE                      CHIP_MIU2_BUS_BASE

#define ARM_MIU3_BUS_BASE                      0xFFFFFFFFFFFFFFFFUL

#define ARM_MIU0_BASE_ADDR                     0x00000000UL
#define ARM_MIU1_BASE_ADDR                     0x80000000UL
#define ARM_MIU2_BASE_ADDR                     0xFFFFFFFFUL
#define ARM_MIU3_BASE_ADDR                     0xFFFFFFFFFFFFFFFFUL

#define BDMA_CONFIG_REG_NUM 7

#define MAX_BDMA_REG_NUM 8

#define MASK_START_BIT0	0
#define MASK_LENGTH_4BYTE 32

#define SleepTime 200


#define MAX_PATH_LEN	256
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))
#endif

#define LD_COMPENSATION_LENGTH      0x200       //Compensation.bin length
#define MEMALIGN                    8           //align bits
#define LD_BIN_OFFSET			0x20        //bin header length
#define LDM_BIN_CHECK_POSITION	4    //according bin file
#define GAMMA_TABLE_LEN     256

#define COMPENSATION_BIN    "Compensation.bin"
#define EDGE2D_BIN          "Edge2D.bin"
#define BACKLIGHTGAMMA_BIN  "BacklightGamma.bin"
#define EDGE2D_OLED_BIN     "Edge2D_Oled.bin"

#define LD_AHB3_WA0_BIN     "ld_ahb3_wa0.bin"
#define LD_AHB4_WA0_BIN     "ld_ahb4_wa0.bin"
#define LD_AHB5_WA0_BIN     "ld_ahb5_wa0.bin"
#define LD_AHB3_WA1_BIN     "ld_ahb3_wa1_bdma.bin"
#define LD_AHB4_WA1_BIN     "ld_ahb4_wa1_bdma.bin"
#define LD_AHB5_WA1_BIN     "ld_ahb5_wa1_bdma.bin"

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define WAIT_CNT	200
//-------------------------------------------------------------------------------------------------
//  Local Structurs
//-------------------------------------------------------------------------------------------------
static int LDM_fd;

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
//ST_DRV_LD_CUS_PATH stCusPath;
struct LD_CUS_PATH stCusPath;

struct pqu_render_ldm_init_param ldm_init_param;
struct pqu_render_ldm_init_reply_param ldm_init_reply_param;

struct file *filp;
char *bufferAHB0;
char *buffer;
void __iomem *u64Vaddr;
u64 ld_addr_base;
u64 u64Compensation_Addr;
u64 u64Compensation_mem_size;
u64 u64Compensation_offset;
u64 u64Edge2D_Addr;
u64 u64Edge2D_mem_size;
u64 u64Edge2D_bin_size;
u64 u64Edge2D_offset;
u64 u64Backlight_Gamma_Addr;
u64 u64Backlight_Gamma_mem_size;
u64 u64Backlight_offset;

/* dts parameter addr*/
u64 u64Dts_ld_panel_Addr;
u64 u64Dts_ld_panel_mem_size = 0x100;
u64 u64Dts_ld_panel_offset;

u64 u64Dts_ld_misc_Addr;
u64 u64Dts_ld_misc_mem_size = 0x100;
u64 u64Dts_ld_misc_offset;

u64 u64Dts_ld_mspi_Addr;
u64 u64Dts_ld_mspi_mem_size = 0x100;
u64 u64Dts_ld_mspi_offset;

u64 u64Dts_ld_dma_Addr;
u64 u64Dts_ld_dma_mem_size = 0x100;
u64 u64Dts_ld_dma_offset;

u64 u64Dts_ld_led_device_Addr;
u64 u64Dts_ld_led_device_mem_size = 0x100;
u64 u64Dts_ld_led_device_offset;

u64 u64LDF_Addr;





const __u8 *pu8CompTable; // Compensation table  SIZE=256x2
const __u8 *pu8CompTable1; // Compensation table  SIZE=256x2
const __u8 *pu8CompTable2; // Compensation table  SIZE=256x2
const __u8 *pu8Edge2DTable; // Edge2D table  SIZE=(u8LEDWidth*u8LEDHeight)*(u8LDFWidth*u8LDFHeight)
__u8 *pu8GammaTable;

__u8 *pTbl_LD_Gamma[16] = {
NULL, // NULL indicates linear
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL
};

__u8 *pTbl_LD_Remap[16] = {
NULL, // NULL indicates linear
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL
};

//local dimming miu read / write  use va:u64Vaddr, not pa:addr_base_L
#define MDrv_LD_MIUReadByte(addr_base_L, offset)  (*((unsigned char *)\
(u64Vaddr + (addr_base_L) + (offset))))
#define MDrv_LD_MIURead2Bytes(addr_base_L, offset) (*((__u16 *)\
(u64Vaddr + (addr_base_L) + (offset))))
#define MDrv_LD_MIUWriteByte(addr_base_L, offset, value) ((*((unsigned char *)\
(u64Vaddr + (addr_base_L) + (offset)))) = ((__u8)(value)))
#define MDrv_LD_MIUWrite2Bytes(addr_base_L, offset, val) ((*((__u16 *)\
(u64Vaddr + (addr_base_L) + (offset)))) = (__u16)(val))

//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
static enum EN_LDM_DEBUG_LEVEL gLDMDbgLevel = E_LDM_DEBUG_LEVEL_ERROR;
//static bool gbLDMInitialized = FALSE;
//static u8 gu8LDMStatus;
//static u8 gLDMTemp;

//static bool bLdm_Inited = FALSE;
//static MS_S32 _gs32LDM_Mutex;
//LDM mutex wait time
//#define LDM_MUTEX_WAIT_TIME    3000
static DEFINE_MUTEX(Semutex_LD_BDMA);


//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------
#define LDM_DBG_FUNC()	\
do {				\
	if (gLDMDbgLevel >= E_LDM_DEBUG_LEVEL_ALL)\
		printf("[LDM]: %s: %d:\n", __func__, __LINE__);\
} while (0)

#define LDM_DBG_INFO(msg...)	\
do {\
	if (gLDMDbgLevel >= E_LDM_DEBUG_LEVEL_ERROR/*E_LDM_DEBUG_LEVEL_INFO */)\
		printf("[LDM INFO]: %s: %d: ", __func__, __LINE__);\
		printf(msg);\
} while (0)

#define LDM_DBG_ERR(msg...)	\
do {\
	if (gLDMDbgLevel >= E_LDM_DEBUG_LEVEL_ERROR)\
		printf("[LDM ERROR]: %s: %d: ", __func__, __LINE__);\
		printf(msg);\
} while (0)

#define LDM_DBG_WARN(msg...)	\
do {\
	if (gLDMDbgLevel >= E_LDM_DEBUG_LEVEL_WARNING) \
		printf("[LDM WARNING]: %s: %d: ", __func__, __LINE__);\
		printf(msg); \
} while (0)


static char *ld_config_paths[] = {
	"/mnt/vendor/config/ldm/",
	"/vendor/tvconfig/config/ldm/",
	"/tvconfig/config/ldm/",
	"/config/ldm/",
	"/vendor/firmware/",
	"/lib/firmware/",
};

static char *ld_config_paths_root[] = {
	"../../mnt/vendor/config/ldm/",
	"../../vendor/tvconfig/config/ldm/",
	"../../tvconfig/config/ldm/",
	"../../config/ldm/",
	"../../vendor/firmware/",
	"../../lib/firmware/",
};

static struct ConfigFile_t stLdConfig[4] = {
{
.size = 0,
.p    = NULL,
.name = COMPENSATION_BIN,
},
{
.size = 0,
.p    = NULL,
.name = EDGE2D_BIN,
},
{
.size = 0,
.p    = NULL,
.name = BACKLIGHTGAMMA_BIN,
},
{
.size = 0,
.p    = NULL,
.name = EDGE2D_OLED_BIN,
},
};


/* firmware upload */
static int firmware_load(struct mtk_tv_kms_context *pctx, char *filename,
			 char **fw, size_t *fw_len)
{
	const struct firmware *fw_entry;
	int ret, i;

	DRM_INFO("%s: LDM Bin device inserted  %s\n", __func__, filename);

	do {
		ret = request_firmware(&fw_entry, filename, pctx->dev);
	} while (ret == -EAGAIN);

	if (ret) {
		DRM_ERROR("%s: Firmware not available,error:%d\n", __func__, ret);
		return ret;
	}

	DRM_INFO("%s: Firmware: Data:%x, size:%x\n", __func__, fw_entry->data, fw_entry->size);

	*fw_len = fw_entry->size;

	*fw = vmalloc(fw_entry->size);

	if (*fw == NULL) {
		DRM_ERROR("vmalloc error\n");
		ret = -ENOMEM;
		return ret;
	}

	memcpy(*fw, fw_entry->data, fw_entry->size);

	DRM_INFO("%s: Firmware: memcpy  new_va:%x, data:%x\n", __func__, *fw, fw_entry->data);

	release_firmware(fw_entry);

	return ret;
}

bool is_path_exist(const char *path)
{
	struct path stPath;
	int s32Error;

	if (!path || !*path)
		return false;

	s32Error = kern_path(path, LOOKUP_FOLLOW, &stPath);
	if (s32Error)
		return false;

	path_put(&stPath);
	return true;
}

bool MDrv_LD_PrepareConfigFile(struct mtk_tv_kms_context *pctx)
{
	int ret = 0, i, j;
	char configPath[MAX_PATH_LEN];
	char path[MAX_PATH_LEN] = {0};
	int n;

	memset(configPath, 0, MAX_PATH_LEN);

	for (i = 0; i < ARRAY_SIZE(ld_config_paths); i++) {
		if (is_path_exist(ld_config_paths[i])) {
			DRM_INFO("trace! find configPath path:%s\n", ld_config_paths[i]);
			strncpy(configPath, ld_config_paths_root[i], MAX_PATH_LEN-1);
			configPath[sizeof(configPath) - 1] = '\0';
			break;
		}
		DRM_INFO("trace! skip configPath path:%s\n", ld_config_paths[i]);
	}

	if (strlen(configPath) == 0) {
		DRM_INFO("Config path error:null\n");
		return false;
	}

	DRM_INFO("trace! using configPath path:%s\n", configPath);

	for (i = 0; i < ARRAY_SIZE(stLdConfig); i++) {
		n = snprintf(path, MAX_PATH_LEN-1, "%s/%s", configPath, stLdConfig[i].name);
		if (n < 0 || n >= sizeof(path)) {
			/* Handle snprintf() error */
			DRM_INFO("configPath unknown error");
			return false;
		}
		ret = firmware_load(
			pctx, path,
			&stLdConfig[i].p, &stLdConfig[i].size);
		if (ret) {
			DRM_ERROR("Load Bin error file name:%s not exists\n", stLdConfig[i].name);
			DRM_ERROR("[%s,%5d] fail\n", __func__, __LINE__);
			return false;
		}
		DRM_INFO("data:%x  size:%x\n", stLdConfig[i].p, stLdConfig[i].size);
	}
	return E_LDM_RET_SUCCESS;
}

struct ConfigFile_t *MDrv_LD_GetConfigFile(const char *configName)
{
	int i;

	if (configName == NULL) {
		DRM_ERROR("configName is null\n");
		return NULL;
	}
	for (i = 0; i < ARRAY_SIZE(stLdConfig); i++) {
		if (strcmp(configName, stLdConfig[i].name) == 0)
			return &stLdConfig[i];
	}
	DRM_ERROR("config %s not found\n", configName);
	return NULL;
}

bool MDrv_LD_CheckData(char *buff, int buff_len)
{
	__u16 u16Checked = 0;
	__u32 u32Counter = 0;
	__u64 u64Sum = 0;

	if (buff == NULL) {
		DRM_ERROR("error! %s:%d, parametre error Pointer is null\n", __func__, __LINE__);
		return E_LDM_RET_FAIL;
	}

	//before checked data
	for (; u32Counter < LDM_BIN_CHECK_POSITION; u32Counter++)
		u64Sum += *(buff+u32Counter);

	u16Checked = *(buff + LDM_BIN_CHECK_POSITION) + ((*(buff+LDM_BIN_CHECK_POSITION+1))<<8);

	//after checked to 0xbuff_len
	for (u32Counter = LDM_BIN_CHECK_POSITION + 2; u32Counter < buff_len; u32Counter++)
		u64Sum += *(buff + u32Counter);

	DRM_INFO(" buff_len:%d, u16Checked:0x%x, u64Sum:0x%lx\n", buff_len, u16Checked, u64Sum);
	if (u16Checked != (u64Sum&0xFFFF))
		return E_LDM_RET_FAIL;

	return E_LDM_RET_SUCCESS;
}


void MDrv_LD_ADL_ALL_CompensationTable(__u32 addr, const void *pCompTable, const void *pCompTable2)
{
	int i = 0;

	const __u16 *pu16CompTable = pCompTable;
	const __u16 *pu16CompTable2 = pCompTable2;
	__u8 u8Byte0, u8Byte1, u8Byte2;

	if (pu16CompTable2 && pu16CompTable) {
		for (i = 0; i < LD_COMPENSATION_LENGTH / 2; i++) {
			u8Byte0 = (pu16CompTable[i]) & 0xFF;
			u8Byte1 = ((pu16CompTable[i] >> 8) & 0xF)
			| (((pu16CompTable2[i]) & 0xF) << 4);
			u8Byte2 =  (pu16CompTable2[i] >> 4) & 0xFF;
			MDrv_LD_MIUWriteByte(addr - ld_addr_base, (32 * i) + 0, u8Byte0);
			MDrv_LD_MIUWriteByte(addr - ld_addr_base, (32 * i) + 1, u8Byte1);
			MDrv_LD_MIUWriteByte(addr - ld_addr_base, (32 * i) + 2, u8Byte2);
		}
	} else if (pu16CompTable) {
		for (i = 0; i < LD_COMPENSATION_LENGTH / 2; i++) {
			u8Byte0 = (pu16CompTable[i]) & 0xFF;
			u8Byte1 = ((pu16CompTable[i] >> 8) & 0xF);
			MDrv_LD_MIUWriteByte(addr - ld_addr_base, (32 * i) + 0, u8Byte0);
			MDrv_LD_MIUWriteByte(addr - ld_addr_base, (32 * i) + 1, u8Byte1);
		}
	} else {
		DRM_ERROR("LD Compensation Table is NULL!\n");
		//return addr /= psDrvLdInfo->u16PackLength;
	}
	//MDrv_LD_FlushDRAMData((unsigned long)u64Vaddr + addr - ld_addr_base, addr, 8*1024);

	DRM_INFO("LD ADL CompensationTable done:%x\n", addr);

}


//----------------------------------------------------------------
// MDrv_LDM_Init_SetLDC - Set LD path
/// @ingroup G_LDM_CONTROL
// @return: E_LDM_RET_SUCCESS is initial
//----------------------------------------------------------------
__u8 MDrv_LDM_Load_Bin_File(__u64 phyMMAPAddr,
__u64 u64Addr_Offset, struct mtk_tv_kms_context *pctx)
{
	//int ret = 0;
	//load Compensation.bin and write them to regs
	int i, j;

	struct ConfigFile_t *CompensationBin = MDrv_LD_GetConfigFile("Compensation.bin");
	struct ConfigFile_t *Edge2DBin = MDrv_LD_GetConfigFile("Edge2D.bin");
	struct ConfigFile_t *BacklightGammaBin = MDrv_LD_GetConfigFile("BacklightGamma.bin");

	struct drm_st_ld_panel_info *pld_panel_info = &(pctx->ld_priv.ld_panel_info);
	struct drm_st_ld_misc_info *pld_misc_info = &(pctx->ld_priv.ld_misc_info);
	struct drm_st_ld_mspi_info *pld_mspi_info = &(pctx->ld_priv.ld_mspi_info);
	struct drm_st_ld_dma_info  *pld_dma_info = &(pctx->ld_priv.ld_dma_info);
	struct drm_st_ld_led_device_info *pld_led_device_info = &(pctx->ld_priv.ld_led_device_info);

	if (CompensationBin == NULL || CompensationBin->p == NULL) {
		DRM_ERROR(" load Compensation bin fail\n");
		return E_LDM_RET_PARAMETER_ERROR;
	}
	if (MDrv_LD_CheckData(CompensationBin->p, CompensationBin->size) == E_LDM_RET_FAIL) {
		DRM_ERROR("Compensation bin format error\n");
		return E_LDM_RET_PARAMETER_ERROR;
	}
	u64Compensation_mem_size = LD_COMPENSATION_LENGTH * 0x20;
	u64Compensation_mem_size = (
	u64Compensation_mem_size + 0xFF) >> MEMALIGN << MEMALIGN; // align at 0x100

	pu8CompTable = CompensationBin->p + LD_BIN_OFFSET;
	DRM_INFO("trace! Compensation bin addr buf:0x%p\n", CompensationBin->p);
	DRM_INFO("addr comp:0x%p\n", pu8CompTable);
	DRM_INFO("size: 0x%x , size: 0x%x\n", CompensationBin->size, u64Compensation_mem_size);

	pu8CompTable1 = pu8CompTable + LD_COMPENSATION_LENGTH;
	pu8CompTable2 = pu8CompTable1 + LD_COMPENSATION_LENGTH;

	//load Edge2D.bin and write them to address of edge2D

	if (pld_misc_info->bOLEDEn)
		Edge2DBin = MDrv_LD_GetConfigFile("Edge2D_Oled.bin");

	if (Edge2DBin == NULL || Edge2DBin->p == NULL) {
		DRM_ERROR(" load Edge2D bin fail\n");
		return E_LDM_RET_PARAMETER_ERROR;
	}
	if (MDrv_LD_CheckData(Edge2DBin->p, Edge2DBin->size) == E_LDM_RET_FAIL) {
		DRM_ERROR("Edge2D bin format error\n");
		return E_LDM_RET_PARAMETER_ERROR;
	}

	u64Edge2D_bin_size = Edge2DBin->size;
	u64Edge2D_mem_size = Edge2DBin->size;
	u64Edge2D_mem_size = (
	u64Edge2D_mem_size + 0xFF) >> MEMALIGN << MEMALIGN; // align at 0x100

	pu8Edge2DTable = Edge2DBin->p + LD_BIN_OFFSET;
	DRM_INFO("trace! Edge2D bin  addr buf:0x%p\n", Edge2DBin->p);
	DRM_INFO("addr edge:0x%p\n", pu8Edge2DTable);
	DRM_INFO("u64Edge2D_mem_size:0x%x\n", u64Edge2D_mem_size);
	DRM_INFO("u64Edge2D_bin_size:0x%x\n", u64Edge2D_bin_size);

	/*load BacklightGamma.bin and write them to dram ,the address is pGamma_blocks*/

	if (BacklightGammaBin == NULL || BacklightGammaBin->p == NULL) {
		DRM_ERROR(" load BacklightGamma bin fail\n");
		return E_LDM_RET_PARAMETER_ERROR;
	}
	if (MDrv_LD_CheckData(BacklightGammaBin->p, BacklightGammaBin->size) == E_LDM_RET_FAIL) {
		DRM_ERROR("BacklightGamma bin format error\n");
		return E_LDM_RET_PARAMETER_ERROR;
	}
	u64Backlight_Gamma_mem_size = BacklightGammaBin->size;
	u64Backlight_Gamma_mem_size = (
	u64Backlight_Gamma_mem_size + 0xFF) >> MEMALIGN << MEMALIGN; // align at 0x100

	pu8GammaTable = BacklightGammaBin->p + LD_BIN_OFFSET;
	DRM_INFO("trace! BacklightGamma bin  addr buf:0x%p\n", BacklightGammaBin->p);
	DRM_INFO("addr edge:0x%p\n", pu8GammaTable);
	DRM_INFO("size:0x%x\n", u64Backlight_Gamma_mem_size);

	/*MDrv_LD_SetupGammaTable(BacklightGammaBin->p + LD_BIN_OFFSET);*/
	for (i = 0; i < 16; i++) {
		pTbl_LD_Gamma[i] = pu8GammaTable + i * GAMMA_TABLE_LEN;
		pTbl_LD_Remap[i] = pu8GammaTable + (16 + i) * GAMMA_TABLE_LEN;
	}

	/*------------------------Layout-----------------------*/
	/*Compensation Bin*/
	/*Edge2d Bin*/
	/*Backlight_Gamma Bin*/
	/*DTS Panel info*/
	/*DTS MISC info*/
	/*DTS MSPI info*/
	/*DTS DMA info*/
	/*DTS LED info*/
	/*LDF*/
	/*LDB*/
	/*-----------------------------------------------------*/


	/*BIN MEN ADDR*/
	u64Compensation_Addr = phyMMAPAddr;
	u64Edge2D_Addr = u64Compensation_Addr + u64Compensation_mem_size;
	u64Backlight_Gamma_Addr = u64Edge2D_Addr + u64Edge2D_mem_size;
	/*DTS MEN ADDR*/
	u64Dts_ld_panel_Addr = u64Backlight_Gamma_Addr + u64Backlight_Gamma_mem_size;
	u64Dts_ld_misc_Addr = u64Dts_ld_panel_Addr + u64Dts_ld_panel_mem_size;
	u64Dts_ld_mspi_Addr = u64Dts_ld_misc_Addr + u64Dts_ld_misc_mem_size;
	u64Dts_ld_dma_Addr = u64Dts_ld_mspi_Addr + u64Dts_ld_mspi_mem_size;
	u64Dts_ld_led_device_Addr = u64Dts_ld_dma_Addr + u64Dts_ld_dma_mem_size;
	/*LDF ADDR */
	u64LDF_Addr = u64Dts_ld_led_device_Addr + u64Dts_ld_dma_mem_size;


	u64Compensation_offset = u64Compensation_Addr - ld_addr_base;
	u64Edge2D_offset = u64Edge2D_Addr - ld_addr_base;
	u64Backlight_offset = u64Backlight_Gamma_Addr - ld_addr_base;
	u64Dts_ld_panel_offset = u64Dts_ld_panel_Addr - ld_addr_base;
	u64Dts_ld_misc_offset = u64Dts_ld_misc_Addr - ld_addr_base;
	u64Dts_ld_mspi_offset = u64Dts_ld_mspi_Addr - ld_addr_base;
	u64Dts_ld_dma_offset = u64Dts_ld_dma_Addr - ld_addr_base;
	u64Dts_ld_led_device_offset = u64Dts_ld_led_device_Addr - ld_addr_base;

	memset(u64Vaddr + u64Compensation_offset, 0, u64Compensation_mem_size);
	MDrv_LD_ADL_ALL_CompensationTable(u64Compensation_Addr, pu8CompTable1, pu8CompTable2);
	/*memcpy(u64Vaddr + u64Compensation_offset, pu8CompTable, u64Compensation_mem_size);*/

	dma_direct_sync_single_for_device(
	pctx->dev,
	u64Compensation_Addr + CHIP_MIU0_BUS_BASE,
	u64Compensation_mem_size,
	1);
	DRM_INFO("u64Compensation_Addr:0x%llx", u64Compensation_Addr);

	memset(u64Vaddr + u64Edge2D_offset, 0, u64Edge2D_mem_size);
	memcpy(u64Vaddr + u64Edge2D_offset, pu8Edge2DTable, u64Edge2D_mem_size);
	dma_direct_sync_single_for_device(
	pctx->dev,
	u64Edge2D_Addr + CHIP_MIU0_BUS_BASE,
	u64Edge2D_mem_size,
	1);
	DRM_INFO("u64Edge2D_Addr:0x%llx", u64Edge2D_Addr);

	memset(u64Vaddr + u64Backlight_offset, 0, u64Backlight_Gamma_mem_size);
	memcpy(u64Vaddr + u64Backlight_offset, pu8GammaTable, u64Backlight_Gamma_mem_size);
	dma_direct_sync_single_for_device(
	pctx->dev,
	u64Backlight_Gamma_Addr + CHIP_MIU0_BUS_BASE,
	u64Backlight_Gamma_mem_size,
	1);
	DRM_INFO("u64Backlight_Gamma_Addr:0x%llx", u64Backlight_Gamma_Addr);


	memset(u64Vaddr + u64Dts_ld_panel_offset, 0, u64Dts_ld_panel_mem_size);
	memcpy(u64Vaddr + u64Dts_ld_panel_offset, pld_panel_info, u64Dts_ld_panel_mem_size);
	dma_direct_sync_single_for_device(
	pctx->dev,
	u64Dts_ld_panel_Addr + CHIP_MIU0_BUS_BASE,
	u64Dts_ld_panel_mem_size,
	1);
	DRM_INFO("u64Dts_ld_panel_Addr:0x%llx", u64Dts_ld_panel_Addr);

	memset(u64Vaddr + u64Dts_ld_misc_offset, 0, u64Dts_ld_misc_mem_size);
	memcpy(u64Vaddr + u64Dts_ld_misc_offset, pld_misc_info, u64Dts_ld_misc_mem_size);
	dma_direct_sync_single_for_device(
	pctx->dev,
	u64Dts_ld_misc_Addr + CHIP_MIU0_BUS_BASE,
	u64Dts_ld_misc_mem_size,
	1);
	DRM_INFO("u64Dts_ld_misc_Addr:0x%llx", u64Dts_ld_misc_Addr);

	memset(u64Vaddr + u64Dts_ld_mspi_offset, 0, u64Dts_ld_mspi_mem_size);
	memcpy(u64Vaddr + u64Dts_ld_mspi_offset, pld_mspi_info, u64Dts_ld_mspi_mem_size);
	dma_direct_sync_single_for_device(
	pctx->dev,
	u64Dts_ld_mspi_Addr + CHIP_MIU0_BUS_BASE,
	u64Dts_ld_mspi_mem_size,
	1);
	DRM_INFO("u64Dts_ld_mspi_Addr:0x%llx", u64Dts_ld_mspi_Addr);

	memset(u64Vaddr + u64Dts_ld_dma_offset, 0, u64Dts_ld_dma_mem_size);
	memcpy(u64Vaddr + u64Dts_ld_dma_offset, pld_dma_info, u64Dts_ld_dma_mem_size);
	dma_direct_sync_single_for_device(
	pctx->dev,
	u64Dts_ld_dma_Addr + CHIP_MIU0_BUS_BASE,
	u64Dts_ld_dma_mem_size,
	1);
	DRM_INFO("u64Dts_ld_dma_Addr:0x%llx", u64Dts_ld_dma_Addr);

	memset(u64Vaddr + u64Dts_ld_led_device_offset, 0, u64Dts_ld_led_device_mem_size);
	memcpy(
	u64Vaddr + u64Dts_ld_led_device_offset,
	pld_led_device_info,
	u64Dts_ld_led_device_mem_size);

	dma_direct_sync_single_for_device(
	pctx->dev,
	u64Dts_ld_led_device_Addr + CHIP_MIU0_BUS_BASE,
	u64Dts_ld_led_device_mem_size,
	1);
	DRM_INFO("u64Dts_led_device_Addr:0x%llx", u64Dts_ld_led_device_Addr);

	return E_LDM_RET_SUCCESS;
}

////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_GetData - get local dimming status
/// @ingroup G_LDM_CONTROL
// @param: void
// @return: EN_LDM_STATUS
//----------------------------------------------------------------
//u8 MDrv_LDM_GetStatus(void)
//{
//	LDM_DBG_FUNC();
//	LDM_DBG_INFO("status: %d\n", gu8LDMStatus);
//
//	return gu8LDMStatus;
//}
////remove-end trunk ld need refactor


////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_Enable - start local dimming algorithm
/// @ingroup G_LDM_CONTROL
// @param: na
// @return: E_LDM_RET_SUCCESS is enable
//----------------------------------------------------------------
//u8 MDrv_LDM_Enable(void)
//{
//	u8 iResult = 0;
//
//	iResult = MDrv_LD_Enable(TRUE, 0);
//
//	if (iResult == E_LDM_RET_FAIL) {
//		LDM_DBG_INFO("LDM Enable Fail\n");
//
//		return E_LDM_RET_FAIL;
//	}
//	gu8LDMStatus = E_LDM_STATUS_ENABLE;
//
//	LDM_DBG_INFO("OK\n");
//
//	return E_LDM_RET_SUCCESS;
//}
////remove-end trunk ld need refactor


////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_Disable - stop local dimming algorithm, send constant luminance  to led
/// @ingroup G_LDM_CONTROL
// @param: u8Lumma: constant luminance range from 00 to 255
// @return: E_LDM_RET_SUCCESS is disable
//----------------------------------------------------------------
//u8 MDrv_LDM_Disable(u8 u8Lumma)
//{
//	u8 iResult = 0;
//
//	iResult = MDrv_LD_Enable(FALSE, u8Lumma);
//
//	if (iResult == E_LDM_RET_FAIL) {
//		LDM_DBG_INFO("LDM Disable Fail\n");
//		return E_LDM_RET_FAIL;
//	}
//	gu8LDMStatus = E_LDM_STATUS_DISNABLE;
//
//	LDM_DBG_INFO("OK\n");
//
//	return E_LDM_RET_SUCCESS;
//
//}
////remove-end trunk ld need refactor




//----------------------------------------------------------------
// MDrv_LDM_Init - Set  mmap address to register base
/// @ingroup G_LDM_CONTROL
// @param: phyAddr: local dimming mmap address in mmap.ini
// @return: E_LDM_RET_SUCCESS is initial
//----------------------------------------------------------------
__u8 MDrv_LDM_Init(__u64 phyAddr, struct mtk_tv_kms_context *pctx)
{
	int ret = 0;
	__u64 phyMMAPAddr = 0;
	__u64 u64Addr_Offset = 0;

	phyMMAPAddr = phyAddr;

	DRM_INFO("init mmap address: 0x%llx\n", phyAddr);

	ld_addr_base = phyAddr;

	if (u64Vaddr == NULL) {
		u64Addr_Offset = CHIP_MIU0_BUS_BASE;
		LDM_DBG_INFO(
			"profile u64Addr_Offset %tx: %tx\n",
			(size_t)u64Addr_Offset,
			(size_t)(ld_addr_base+u64Addr_Offset));
		u64Vaddr = ioremap_cache((size_t)(ld_addr_base + u64Addr_Offset), LDMMAPLENGTH);
	}
	if ((u64Vaddr == NULL) || (u64Vaddr == (void *)((size_t)-1))) {
		LDM_DBG_ERR(
			"error! ioremap_cached paddr:0x%tx, length:%d\n",
			(size_t)(ld_addr_base+u64Addr_Offset),
			LDMMAPLENGTH);
		return E_LDM_RET_FAIL;
	}

	LDM_DBG_INFO(
		"profile LD_VA_BASE = 0x:%tx, length = 0x:%x\n",
		u64Vaddr, LDMMAPLENGTH);

	ret = MDrv_LD_PrepareConfigFile(pctx);
	if (ret != E_LDM_RET_SUCCESS) {
		DRM_ERROR(" MDrv_LD_PrepareConfigFile fail:%d\n ", ret);
		return -1;
	}

	ret = MDrv_LDM_Load_Bin_File(phyMMAPAddr, u64Addr_Offset, pctx);
	if (ret != E_LDM_RET_SUCCESS) {
		DRM_ERROR(" MDrv_LDM_Load_Bin_File fail:%d\n ", ret);
		return -1;
	}

	ldm_init_param.u64Compensation_Addr = u64Compensation_Addr;
	ldm_init_param.u32Compensation_mem_size = u64Compensation_mem_size;
	ldm_init_param.u32Edge2D_mem_size = u64Edge2D_mem_size;
	ldm_init_param.u32Edge2D_bin_size = u64Edge2D_bin_size;
	ldm_init_param.u32Backlight_Gamma_mem_size = u64Backlight_Gamma_mem_size;
	ldm_init_param.u16DTS_Mem_Size = u64Dts_ld_panel_mem_size;

	DRM_INFO("rpmsg mmap address: 0x%llx, 0x%llx, 0x%llx, 0x%llx, 0x%llx, 0x%llx\n",
		u64Compensation_Addr,
		u64Edge2D_Addr,
		u64Backlight_Gamma_Addr,
		u64Dts_ld_panel_Addr,
		u64Dts_ld_panel_mem_size,
		u64LDF_Addr);


	pqu_msg_send(PQU_MSG_SEND_LDM_INIT, &ldm_init_param);

	DRM_INFO(
	"[DRM][LDM][%s][%d] LD init reply  %x, %x !!\n",
	__func__,
	__LINE__,
	ret,
	ldm_init_reply_param.ret);

/* ret = pqu_render_ldm_init(&ldm_init_param, &ldm_init_reply_param);*/

	DRM_INFO(
	"[DRM][LDM][%s][%d] LD init reply 1 %x, %x !!\n",
	__func__,
	__LINE__,
	ret,
	ldm_init_reply_param.ret);

	DRM_INFO("OK\n");
	return E_LDM_RET_SUCCESS;
}



//----------------------------------------------------------------
// MDrv_LDM_Init_SetLDC - Set LD path
/// @ingroup G_LDM_CONTROL
// @return: E_LDM_RET_SUCCESS is initial
//----------------------------------------------------------------
u8 MDrv_LDM_Init_SetLDC(u32 u32LdmType)
{
	int ret = 0;
	struct reg_info reg[5];
	struct hwregLDCIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregLDCIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.enLDMSupportType = u32LdmType;

	ret = drv_hwreg_render_ldm_set_LDC(paramIn, &paramOut);

	LDM_DBG_INFO("SetLDC done\n");
	return E_LDM_RET_SUCCESS;
}


//----------------------------------------------------------------
// MDrv_LDM_Init_SetSwSetCtrl - SW set Ctrl
/// @ingroup G_LDM_CONTROL
// @return: E_LDM_RET_SUCCESS
//----------------------------------------------------------------
u8 MDrv_LDM_Init_SetSwSetCtrl(
	struct hwregSWSetCtrlIn *stSwSetCtrl)
{
	int ret = 0;
	struct reg_info reg[2];
	struct hwregSWSetCtrlIn paramIn;
	struct hwregOut paramOut;

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregSWSetCtrlIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.enLDMSwSetCtrlType = stSwSetCtrl->enLDMSwSetCtrlType;
	paramIn.u8Value = stSwSetCtrl->u8Value;

	ret = drv_hwreg_render_ldm_set_swsetctrl(paramIn, &paramOut);

	LDM_DBG_INFO("SetSwSetCtrl %d %d done\n", paramIn.enLDMSwSetCtrlType, paramIn.u8Value);
	return E_LDM_RET_SUCCESS;
}

u8 MDrv_LDM_Init_updateprofileAHB0(
	struct mtk_tv_kms_context *pctx)
{

	int ret = 0;
	size_t pfilesize;
	int i;
	u32 AHB0_reg;
	u32 AHB0_value;

	struct reg_info reg[1];
	struct hwregAHBIn paramIn;
	struct hwregOut paramOut;

	char *filename;
	char configPath[MAX_PATH_LEN];
	char path[MAX_PATH_LEN] = {0};
	int n;

	memset(configPath, 0, MAX_PATH_LEN);

	if (pctx->panel_priv.v_cfg.timing == E_4K2K_60HZ) {
		filename = LD_AHB3_WA0_BIN;
	} else if (pctx->panel_priv.v_cfg.timing == E_4K2K_120HZ) {
		filename = LD_AHB4_WA0_BIN;
	} else if (pctx->panel_priv.v_cfg.timing == E_8K4K_60HZ) {
		filename = LD_AHB5_WA0_BIN;
	} else {
		filename = "";
		LDM_DBG_ERR("Config path error:null\n");
		return E_LDM_RET_PARAMETER_ERROR;
	}

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregAHBIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	for (i = 0; i < ARRAY_SIZE(ld_config_paths); i++) {
		if (is_path_exist(ld_config_paths[i])) {
			DRM_INFO("trace! find configPath path:%s\n", ld_config_paths[i]);
			strncpy(configPath, ld_config_paths_root[i], MAX_PATH_LEN-1);
			configPath[sizeof(configPath) - 1] = '\0';
			break;
		}
		DRM_INFO("trace! skip configPath path:%s\n", ld_config_paths[i]);
	}

	if (strlen(configPath) == 0) {
		DRM_INFO("Config path error:null\n");
		return false;
	}

	DRM_INFO("trace! using configPath path:%s\n", configPath);

	n = snprintf(path, MAX_PATH_LEN-1, "%s/%s", configPath, filename);
	if (n < 0 || n >= sizeof(path)) {
		/* Handle snprintf() error */
		DRM_INFO("configPath unknown error");
		return false;
	}

	ret = firmware_load(pctx, path, &bufferAHB0, &pfilesize);
	if (ret) {
		DRM_ERROR("Load Bin error file name:%s not exists\n", filename);
		DRM_ERROR("[%s,%5d]  fail\n", __func__, __LINE__);
		return false;
	}

	for (i = 0; i < (pfilesize/8); i++) {
		AHB0_value = ((*(bufferAHB0+(i*8+3)))<<24)+
			((*(bufferAHB0+(i*8+2)))<<16)+
			((*(bufferAHB0+(i*8+1)))<<8)+
			(*(bufferAHB0+(i*8)));
		AHB0_reg = ((*(bufferAHB0+(i*8+7)))<<24)+
			((*(bufferAHB0+(i*8+6)))<<16)+
			((*(bufferAHB0+(i*8+5)))<<8)+
			(*(bufferAHB0+(i*8+4)));

		paramOut.reg = reg;

		paramIn.RIU = 1;
		paramIn.u32AHBReg = AHB0_reg;
		paramIn.u32AHBData = AHB0_value;
		paramIn.u8BitStart = MASK_START_BIT0;
		paramIn.u8BitLength = MASK_LENGTH_4BYTE;
		drv_hwreg_render_ldm_write_ahb0(paramIn, &paramOut);
	}

		return E_LDM_RET_SUCCESS;

}

u8 MDrv_LDM_Init_updateprofileAHB1(
	u64 phyAddr,
	struct mtk_tv_kms_context *pctx)
{
	struct hwregWriteMethodInfo writeInfo;
	struct ldmDatebaseIn dbIn;
	u64 u64Addr_Offset = 0;
	int ret = 0;
	size_t pfilesize;
	int i;

	char *filename;
	struct reg_info reg[2];
	struct hwregSWSetCtrlIn paramIn;
	struct hwregOut paramOut;
	char configPath[MAX_PATH_LEN];
	char path[MAX_PATH_LEN] = {0};
	int n;

	memset(configPath, 0, MAX_PATH_LEN);

	if (pctx->panel_priv.v_cfg.timing == E_4K2K_60HZ) {
		filename = LD_AHB3_WA1_BIN;
	} else if (pctx->panel_priv.v_cfg.timing == E_4K2K_120HZ) {
		filename = LD_AHB4_WA1_BIN;
	} else if (pctx->panel_priv.v_cfg.timing == E_8K4K_60HZ) {
		filename = LD_AHB5_WA1_BIN;
	} else {
		filename = "";
		LDM_DBG_ERR("Config path error:null\n");
		return E_LDM_RET_PARAMETER_ERROR;
	}

	memset(reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregSWSetCtrlIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&writeInfo, 0, sizeof(struct hwregWriteMethodInfo));
	memset(&dbIn, 0, sizeof(struct ldmDatebaseIn));

	paramOut.reg = reg;

	paramIn.RIU = 1;
	paramIn.enLDMSwSetCtrlType = E_LDM_SW_LDC_XIU2AHB_SEL1;
	paramIn.u8Value = 1;


	ret = drv_hwreg_render_ldm_set_swsetctrl(paramIn, &paramOut);


	ld_addr_base = phyAddr;


	if (u64Vaddr == NULL) {
		u64Addr_Offset = CHIP_MIU0_BUS_BASE;
		LDM_DBG_INFO(
			"profile u64Addr_Offset %tx: %tx\n",
			(size_t)u64Addr_Offset,
			(size_t)(ld_addr_base+u64Addr_Offset));

	u64Vaddr = ioremap_cache((ld_addr_base+u64Addr_Offset),
		LDMMAPLENGTH);

	}
	if ((u64Vaddr == NULL) || (u64Vaddr == (void *)((size_t)-1))) {
		LDM_DBG_ERR(
			"error! ioremap_cached paddr:0x%tx, length:%d\n",
			(size_t)(ld_addr_base+u64Addr_Offset),
			LDMMAPLENGTH);
		return E_LDM_RET_FAIL;
	}

	LDM_DBG_INFO(
		"profile LD_VA_BASE = 0x:%tx, length = 0x:%x\n",
		u64Vaddr, LDMMAPLENGTH);

	for (i = 0; i < ARRAY_SIZE(ld_config_paths); i++) {
		if (is_path_exist(ld_config_paths[i])) {
			DRM_INFO("trace! find configPath path:%s\n", ld_config_paths[i]);
			strncpy(configPath, ld_config_paths_root[i], MAX_PATH_LEN-1);
			configPath[sizeof(configPath) - 1] = '\0';
			break;
		}
		DRM_INFO("trace! skip configPath path:%s\n", ld_config_paths[i]);
	}

	if (strlen(configPath) == 0) {
		DRM_INFO("Config path error:null\n");
		return false;
	}

	DRM_INFO("trace! using configPath path:%s\n", configPath);

	n = snprintf(path, MAX_PATH_LEN-1, "%s/%s", configPath, filename);
	if (n < 0 || n >= sizeof(path)) {
		/* Handle snprintf() error */
		DRM_INFO("configPath unknown error");
		return false;
	}

	ret = firmware_load(pctx, path, &buffer, &pfilesize);
	if (ret) {
		DRM_ERROR("Load Bin error file name:%s not exists\n", filename);
		DRM_ERROR("[%s,%5d]  fail\n", __func__, __LINE__);
		return false;
	}
	memcpy(u64Vaddr, buffer, pfilesize);

	LDM_DBG_INFO(" trace! Start dma_direct_sync\n");

	dma_direct_sync_single_for_device(
		pctx->dev,
		ld_addr_base+CHIP_MIU0_BUS_BASE,
		pfilesize,
		1);

	writeInfo.eMethod = HWREG_WRITE_BY_RIU;
	writeInfo.hwOut.reg = vmalloc(sizeof(struct reg_info)*BDMA_CONFIG_REG_NUM);
	memset(writeInfo.hwOut.reg, 0, sizeof(struct reg_info)*BDMA_CONFIG_REG_NUM);
	writeInfo.maxRegNum = MAX_BDMA_REG_NUM;
	dbIn.srcAddr = phyAddr;
	dbIn.dataSize = pfilesize;//size
	dbIn.dstAddr = 0x0;//start address, no offset

	LDM_DBG_INFO(" bdma debug filesize %x\n", pfilesize);

	drv_hwreg_render_ldm_update_database(dbIn, &writeInfo);

	msleep(SleepTime);
	paramIn.u8Value = 0;
	ret = drv_hwreg_render_ldm_set_swsetctrl(paramIn, &paramOut);



	paramIn.enLDMSwSetCtrlType = E_LDM_SW_LDC_PATH_SEL;
	paramIn.u8Value = 1;
	ret = drv_hwreg_render_ldm_set_swsetctrl(paramIn, &paramOut);

	vfree(writeInfo.hwOut.reg);

	return E_LDM_RET_SUCCESS;


}


u8 MDrv_LDM_Init_updateprofile(u64 pa, struct mtk_tv_kms_context *pctx)
{
	int ret = 0;

	ret = MDrv_LDM_Init_updateprofileAHB0(pctx);
	if (ret != 0)
		return ret;

	ret = MDrv_LDM_Init_updateprofileAHB1(pa, pctx);
	if (ret != 0)
		return ret;

	return E_LDM_RET_SUCCESS;

}


////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_SetStrength - Set back light percent
/// @ingroup G_LDM_CONTROL
// @param: u8Percent: the percent ranged from 0 to 100
// @return: E_LDM_RET_SUCCESS is setted
//----------------------------------------------------------------
//u8 MDrv_LDM_SetStrength(u8 u8Percent)
//{
//	u8 iResult = 0;
//
//	iResult = MDrv_LD_SetGlobalStrength(u8Percent);
//
//	if (iResult == E_LDM_RET_FAIL) {
//		LDM_DBG_INFO("LDM setStrength Fail\n");
//		return E_LDM_RET_FAIL;
//	}
//
//	LDM_DBG_INFO("OK\n");
//	return E_LDM_RET_SUCCESS;
//
//}
////remove-end trunk ld need refactor



////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_DemoPattern - demo pattern from customer
/// @ingroup G_LDM_CONTROL
// @param: stPattern: demo type: turn on led, left-right half show
// @return: E_LDM_RET_SUCCESS is demonstrative
//----------------------------------------------------------------
//u8 MDrv_LDM_DemoPattern(ST_LDM_DEMO_PATTERN stPattern)
//{
//
//	MDrv_LD_SetDemoPattern(
//		stPattern.enDemoPattern,
//		stPattern.bOn,
//		stPattern.u16LEDNum
//		);
//
//	LDM_DBG_INFO("Demo OK\n");
//
//	return E_LDM_RET_SUCCESS;
//
//}
////remove-end trunk ld need refactor



////remove-start trunk ld need refactor
//----------------------------------------------------------------
// MDrv_LDM_GetData - get LDF/LDB/SPI data pre frame in buffer
/// @ingroup G_LDM_CONTROL
// @param: stData:  data type and mmap address filled with the requied type
// @return: E_LDM_RET_SUCCESS is getted
//----------------------------------------------------------------
//u8 MDrv_LDM_GetData(ST_LDM_GET_DATA *stData)
//{
//	stData->phyAddr = MDrv_LD_GetDataAddr(stData->enDataType);
//	LDM_DBG_INFO("OK\n");
//	return E_LDM_RET_SUCCESS;
//
//}
////remove-end trunk ld need refactor



