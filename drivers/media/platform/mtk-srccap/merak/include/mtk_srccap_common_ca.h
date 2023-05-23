/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#if IS_ENABLED(CONFIG_OPTEE)
#ifndef MTK_SRCCAP_COMMON_CA_H
#define MTK_SRCCAP_COMMON_CA_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/tee_drv.h>
#include <uapi/linux/tee.h>
#include <tee_client_api.h>

extern struct tee_context *tee_client_open_context(struct tee_context *start,
			int (*match)(struct tee_ioctl_version_data *, const void *),
			const void *data, struct tee_ioctl_version_data *vers);
extern void tee_client_close_context(struct tee_context *ctx);
extern int tee_client_open_session(struct tee_context *ctx,
			struct tee_ioctl_open_session_arg *arg, struct tee_param *param);
extern int tee_client_close_session(struct tee_context *ctx, u32 session);
extern int tee_client_invoke_func(struct tee_context *ctx,
			struct tee_ioctl_invoke_arg *arg, struct tee_param *param);
extern struct tee_shm *tee_shm_alloc(struct tee_context *ctx, size_t size, u32 flags);
extern void *tee_shm_get_va(struct tee_shm *shm, size_t offs);

/* ============================================================================================== */
/* ---------------------------------------- Definess -------------------------------------------- */
/* ============================================================================================== */
#define DYNAMIC_OPTEE_VERSION_SUPPORT (0)
#define MULTI_INVOKE_TYPE_SUPPORT (0)
#define BOOTARG_SIZE     (2048)
#define BOOTARG_OFFSET_2 (2)
#define BOOTARG_OFFSET_1 (1)
#define BOOTARG_CMP_SIZE (2)

#define MTK_OPTEE_VER_1     (1)
#define MTK_OPTEE_VER_2     (2)
#define MTK_OPTEE_VER_3     (3)

#define DEFAULT_SHM_SIZE    (8)

/* ============================================================================================== */
/* ------------------------------------------ Enums --------------------------------------------- */
/* ============================================================================================== */
enum srccap_ca_smc_param_idx {
	SRCCAP_CA_SMC_PARAM_IDX_0 = 0,
	SRCCAP_CA_SMC_PARAM_IDX_1,
	SRCCAP_CA_SMC_PARAM_IDX_2,
	SRCCAP_CA_SMC_PARAM_IDX_3,
	SRCCAP_CA_SMC_PARAM_IDX_MAX,
};

/* ============================================================================================== */
/* ----------------------------------------- Structs -------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ---------------------------------------- Functions ------------------------------------------- */
/* ============================================================================================== */
void mtk_common_ca_close_session(struct mtk_srccap_dev *srccap_dev, TEEC_Session *session);
int mtk_common_ca_open_session(struct mtk_srccap_dev *srccap_dev, TEEC_Context *context,
	TEEC_Session *session, const TEEC_UUID *destination,
	TEEC_Operation *operation, u32 *error_origin);
void mtk_common_ca_finalize_context(struct mtk_srccap_dev *srccap_dev, TEEC_Context *context);
int mtk_common_ca_init_context(struct mtk_srccap_dev *srccap_dev, TEEC_Context *context);
int mtk_common_ca_teec_invoke_cmd(struct mtk_srccap_dev *srccap_dev, TEEC_Session *session,
	u32 cmd_id, TEEC_Operation *operation, u32 *error_origin);

#endif  // MTK_SRCCAP_COMMON_CA_H
#endif // IS_ENABLED(CONFIG_OPTEE)
