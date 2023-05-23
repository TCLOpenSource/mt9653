/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __COMM_TEEC_H
#define __COMM_TEEC_H

#include <linux/types.h>
#include <linux/tee_drv.h>

#define BOOTARG_SIZE 2048

/**
 * Flag constants indicating the type of parameters encoded inside the
 * operation payload (TEEC_Operation), Type is uint32_t.
 *
 * TEEC_NONE			The Parameter is not used
 *
 * TEEC_VALUE_INPUT		The Parameter is a TEEC_Value tagged as input.
 *
 * TEEC_VALUE_OUTPUT		The Parameter is a TEEC_Value tagged as output.
 *
 * TEEC_VALUE_INOUT		The Parameter is a TEEC_Value tagged as both as
 *				input and output, i.e., for which both the
 *				behaviors of TEEC_VALUE_INPUT and
 *				TEEC_VALUE_OUTPUT apply.
 *
 * TEEC_MEMREF_TEMP_INPUT	The Parameter is a TEEC_TempMemoryReference
 *				describing a region of memory which needs to be
 *				temporarily registered for the duration of the
 *				Operation and is tagged as input.
 *
 * TEEC_MEMREF_TEMP_OUTPUT	Same as TEEC_MEMREF_TEMP_INPUT, but the Memory
 *				Reference is tagged as output. The
 *				Implementation may update the size field to
 *				reflect the required output size in some use
 *				cases.
 *
 * TEEC_MEMREF_TEMP_INOUT	A Temporary Memory Reference tagged as both
 *				input and output, i.e., for which both the
 *				behaviors of TEEC_MEMREF_TEMP_INPUT and
 *				TEEC_MEMREF_TEMP_OUTPUT apply.
 *
 * TEEC_MEMREF_WHOLE		The Parameter is a Registered Memory Reference
 *				that refers to the entirety of its parent Shared
 *				Memory block. The parameter structure is a
 *				TEEC_MemoryReference. In this structure, the
 *				Implementation MUST read only the parent field
 *				and MAY update the size field when the operation
 *				completes.
 *
 * TEEC_MEMREF_PARTIAL_INPUT	A Registered Memory Reference structure that
 *				refers to a partial region of its parent Shared
 *				Memory block and is tagged as input.
 *
 * TEEC_MEMREF_PARTIAL_OUTPUT	Registered Memory Reference structure that
 *				refers to a partial region of its parent Shared
 *				Memory block and is tagged as output.
 *
 * TEEC_MEMREF_PARTIAL_INOUT	The Registered Memory Reference structure that
 *				refers to a partial region of its parent Shared
 *				Memory block and is tagged as both input and
 *				output, i.e., for which both the behaviors of
 *				TEEC_MEMREF_PARTIAL_INPUT and
 *				TEEC_MEMREF_PARTIAL_OUTPUT apply.
 */
#define TEEC_NONE			0x00000000
#define TEEC_VALUE_INPUT		0x00000001
#define TEEC_VALUE_OUTPUT		0x00000002
#define TEEC_VALUE_INOUT		0x00000003
#define TEEC_MEMREF_TEMP_INPUT		0x00000005
#define TEEC_MEMREF_TEMP_OUTPUT		0x00000006
#define TEEC_MEMREF_TEMP_INOUT		0x00000007
#define TEEC_MEMREF_WHOLE		0x0000000C
#define TEEC_MEMREF_PARTIAL_INPUT	0x0000000D
#define TEEC_MEMREF_PARTIAL_OUTPUT	0x0000000E
#define TEEC_MEMREF_PARTIAL_INOUT	0x0000000F

/**
 * Function error origins, of type TEEC_ErrorOrigin. These indicate where in
 * the software stack a particular return value originates from.
 *
 * TEEC_ORIGIN_API          The error originated within the TEE Client API
 *                          implementation.
 * TEEC_ORIGIN_COMMS        The error originated within the underlying
 *                          communications stack linking the rich OS with
 *                          the TEE.
 * TEEC_ORIGIN_TEE          The error originated within the common TEE code.
 * TEEC_ORIGIN_TRUSTED_APP  The error originated within the Trusted Application
 *                          code.
 */
#define TEEC_ORIGIN_API          0x00000001
#define TEEC_ORIGIN_COMMS        0x00000002
#define TEEC_ORIGIN_TEE          0x00000003
#define TEEC_ORIGIN_TRUSTED_APP  0x00000004

/**
 * Flag constants indicating the data transfer direction of memory in
 * TEEC_Parameter. TEEC_MEM_INPUT signifies data transfer direction from the
 * client application to the TEE. TEEC_MEM_OUTPUT signifies data transfer
 * direction from the TEE to the client application. Type is uint32_t.
 *
 * TEEC_MEM_INPUT   The Shared Memory can carry data from the client
 *                  application to the Trusted Application.
 * TEEC_MEM_OUTPUT  The Shared Memory can carry data from the Trusted
 *                  Application to the client application.
 */
#define TEEC_MEM_INPUT	0x00000001
#define TEEC_MEM_OUTPUT	0x00000002

/*
 * Defines the number of available memory references in an open session or
 * invoke command operation payload.
 */
#define TEEC_CONFIG_PAYLOAD_REF_COUNT 4

/**
 * Encode the paramTypes according to the supplied types.
 *
 * @param p0 The first param type.
 * @param p1 The second param type.
 * @param p2 The third param type.
 * @param p3 The fourth param type.
 */
#define TEEC_PARAM_TYPES(p0, p1, p2, p3) \
	((p0) | ((p1) << 4) | ((p2) << 8) | ((p3) << 12))

/**
 * Get the i_th param type from the paramType.
 *
 * @param p The paramType.
 * @param i The i-th parameter to get the type for.
 */
#define TEEC_PARAM_TYPE_GET(p, i) (((p) >> (i * 4)) & 0xF)

/**
 * This type contains a Universally Unique Resource Identifier (UUID) type as
 * defined in RFC4122. These UUID values are used to identify Trusted
 * Applications.
 */
struct TEEC_UUID {
	uint32_t timeLow;
	uint16_t timeMid;
	uint16_t timeHiAndVersion;
	uint8_t clockSeqAndNode[8];
};

/**
 * struct TEEC_Context - Represents a connection between a client application
 * and a TEE.
 */
struct TEEC_Context {
	union {
		struct tee_context *ctx;
#ifdef CONFIG_ARM64
		uintptr_t fd;
#else
		int fd;
#endif
	};
	bool reg_mem; //optee 3.2
};

/**
 * struct TEEC_Session - Represents a connection between a client application
 * and a trusted application.
 */
struct TEEC_Session {
	uint32_t session_id;
	union {
#ifdef CONFIG_ARM64
		uintptr_t fd;
#else
		int fd;
#endif
		struct tee_session *session;
	};
	struct TEEC_Context ctx;
};

/**
 * struct TEEC_SharedMemory - Memory to transfer data between a client
 * application and trusted code.
 *
 * @param buffer      The memory buffer which is to be, or has been, shared
 *                    with the TEE.
 * @param size        The size, in bytes, of the memory buffer.
 * @param flags       Bit-vector which holds properties of buffer.
 *                    The bit-vector can contain either or both of the
 *                    TEEC_MEM_INPUT and TEEC_MEM_OUTPUT flags.
 *
 * A shared memory block is a region of memory allocated in the context of the
 * client application memory space that can be used to transfer data between
 * that client application and a trusted application. The user of this struct
 * is responsible to populate the buffer pointer.
 */
struct TEEC_SharedMemory {
	void *buffer;
	size_t size;
	uint32_t flags;
	/*
	 * Implementation-Defined
	 */
	int id;
	size_t alloced_size;
	void *shadow_buffer;
	int registered_fd;
};

/**
 * struct TEEC_TempMemoryReference - Temporary memory to transfer data between
 * a client application and trusted code, only used for the duration of the
 * operation.
 *
 * @param buffer  The memory buffer which is to be, or has been shared with
 *                the TEE.
 * @param size    The size, in bytes, of the memory buffer.
 *
 * A memory buffer that is registered temporarily for the duration of the
 * operation to be called.
 */

struct TEEC_TempMemoryReference {
	void *buffer;
	size_t size;
};

/**
 * struct TEEC_RegisteredMemoryReference - use a pre-registered or
 * pre-allocated shared memory block of memory to transfer data between
 * a client application and trusted code.
 *
 * @param parent  Points to a shared memory structure. The memory reference
 *                may utilize the whole shared memory or only a part of it.
 *                Must not be NULL
 *
 * @param size    The size, in bytes, of the memory buffer.
 *
 * @param offset  The offset, in bytes, of the referenced memory region from
 *                the start of the shared memory block.
 *
 */
struct TEEC_RegisteredMemoryReference {
	struct TEEC_SharedMemory *parent;
	size_t size;
	size_t offset;
};

/**
 * struct TEEC_Value - Small raw data container
 *
 * Instead of allocating a shared memory buffer this structure can be used
 * to pass small raw data between a client application and trusted code.
 *
 * @param a  The first integer value.
 *
 * @param b  The second integer value.
 */
struct TEEC_Value {
	uint32_t a;
	uint32_t b;
};

/**
 * union TEEC_Parameter - Memory container to be used when passing data between
 *                        client application and trusted code.
 *
 * Either the client uses a shared memory reference, parts of it or a small raw
 * data container.
 *
 * @param tmpref  A temporary memory reference only valid for the duration
 *                of the operation.
 *
 * @param memref  The entire shared memory or parts of it.
 *
 * @param value   The small raw data container to use
 */
union TEEC_Parameter {
	struct TEEC_TempMemoryReference tmpref;
	struct TEEC_RegisteredMemoryReference memref;
	struct TEEC_Value value;
};

/**
 * struct TEEC_Operation - Holds information and memory references used in
 * InvokeCommand().
 *
 * @param   started     Client must initialize to zero if it needs to cancel
 *                      an operation about to be performed.
 * @param   paramTypes  Type of data passed. Use TEEC_PARAMS_TYPE macro to
 *                      create the correct flags.
 *                      0 means TEEC_NONE is passed for all params.
 * @param   params      Array of parameters of type TEEC_Parameter.
 * @param   session     Internal pointer to the last session used by
 *                      TEEC_InvokeCommand with this operation.
 *
 */
struct TEEC_Operation {
	uint32_t started;
	uint32_t paramTypes;
	union TEEC_Parameter params[TEEC_CONFIG_PAYLOAD_REF_COUNT];
	struct TEEC_Session *session;
};

#define MTK_MDLA_OPTEE_HANDLER_VERSION (1) //Need to aligned with TEE driver header

struct comm_teec_context {
	struct device *dev;
	spinlock_t spinlock;
	int optee_version;
	struct TEEC_Context stContext;
	struct TEEC_Session stSession;
	struct TEEC_UUID stUuid;
	bool svp_enable;
	bool initialized;
};

#define AI_TA_UUID {0xb9dbbfa1, 0x8b01, 0x4643, \
	{0x81, 0x87, 0x7f, 0x84, 0x57, 0x88, 0xcd, 0x45} }

#define CPUFREQ_TA_UUID \
{0x1fc17090, 0xbbdf, 0x4648,\
	{0xbc, 0xa9, 0x3a, 0xf9, 0x7a, 0xd8, 0x8e, 0x63} }

#define LTP_TA_UUID {0x7cac5480, 0xa1fb, 0x11e5, \
	{0xac, 0xb4, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b} }

enum {
	AI_TA_UUID_IDX,
	CPUFREQ_TA_UUID_IDX,
	LTP_TA_UUID_IDX,
	MAX_UUID_IDX,
};

/******************************************************************************************/
/****************************        AI-TA struct/functions      **************************/
/******************************************************************************************/

#define TEEC_AI_MAX_BUF_RES_NUM    (16)
#define ST_TEEC_AI_CONTROL_VERSION   (1)

// define AI TA command id
#define AI_LOAD_MODEL		0x01		// Load AI models
#define AI_UNLOAD_MODEL		0x02		// Unload AI models
#define AI_CREATE_SVP		0x03		// Create SVP pipeline
#define AI_DESTROY_SVP		0x04		// Destroy SVP pipeline
#define AI_UPDATE_PARAM		0x05		// Update AI parameters
#define AI_RESET_PARAM		0x06		// Reset AI parameters
#define AI_CONTROL		0x07		// Control AI operation
#define AI_BIND_PIPELINE	0x08		// Bind AI pipeline with playback’s pipeline
#define AI_SET_PATTERN		0x09		// Set AI alpha pattern for GPU only
#define AI_CHECK_RESULT		0x0A		// Check AI result for GPU only
#define AI_GET_CAPABILITY	0x0B		// Get AI capability

typedef enum {
	E_AI_ENG_MDLA,
	E_AI_ENG_GPU,
	E_AI_ENG_AIE,
} EN_TEEC_AI_ENGINE;

typedef enum {
	EN_AI_BUF_TYPE_INPUT = 1,    // image buffer
	EN_AI_BUF_TYPE_OUTPUT,       // output result buffer
	EN_AI_BUF_TYPE_TEMP,         // activation buffer for MDLA
	EN_AI_BUF_TYPE_STATIC,       // weight, quantization buffer for MDLA
	EN_AI_BUF_TYPE_CODE,         // command buffer for MDLA
	EN_AI_BUF_TYPE_CONFIG        // config buffer for AIE
} EN_TEEC_AI_BUF_TYPE;

typedef enum {
	EN_BUF_SOURCE_DRAM = 0,   // buffer comes from DRAM
	EN_BUF_SOURCE_GSM         // buffer comes from AIA internal SRAM
} EN_TEEC_AI_BUF_SOURCE;

typedef struct {
	EN_TEEC_AI_BUF_TYPE enBufType;      // AI buffer type
	EN_TEEC_AI_BUF_SOURCE enBufSource;  // AI buffer source
	u64 u64BufAddr;                  // buffer address
	u32 u32BufSize;                  // buffer size
} ST_TEEC_AI_RES;

typedef struct {
	u8 u8Version;          // Version
	u32 u32PipelineID;     // Pipeline ID
	EN_TEEC_AI_ENGINE enEng;  // 0: GPU, 1: MDLA, 2: AIE
	u8 u8ModelIndex;       // Model index; 0xFF if non-secure path
	bool bIsSecurity;      // A flag indicates current request is security or not
	u8 u8BufCnt;           // buffer count
	ST_TEEC_AI_RES stBufRes[TEEC_AI_MAX_BUF_RES_NUM];
	// buffer resource (Input and Output); Config buffer if non-secure path
} ST_TEEC_AI_CONTROL;

typedef enum {
	E_HAL_CFG_MDLA = 0,
	E_HAL_CFG_EDMA = 1,
	E_HAL_CFG_POWER = 2,
	E_HAL_CFG_STATUS = 3,
	E_HAL_CFG_NONE = 4
} EN_HAL_CFG_TYPE;

typedef enum {
	E_MDLA_PWR_CMD_INIT_POWER,		// 0
	E_MDLA_PWR_CMD_SET_BOOT_UP,		// 1
	E_MDLA_PWR_CMD_SET_SHUT_DOWN,		// 2
	E_MDLA_PWR_CMD_SET_VOLT,		// 3
	E_MDLA_PWR_CMD_SET_REGULATOR_MODE,	// 4
	E_MDLA_PWR_CMD_SET_FREQ,		// 5
	E_MDLA_PWR_CMD_PM_HANDLER,		// 6
	E_MDLA_PWR_CMD_GET_POWER_INFO,		// 7
	E_MDLA_PWR_CMD_REG_DUMP,		// 8
	E_MDLA_PWR_CMD_UNINIT_POWER,		// 9
	E_MDLA_PWR_CMD_DEBUG_FUNC,		//10
	E_MDLA_PWR_CMD_SEGMENT_CHECK,		//11
	E_MDLA_PWR_CMD_DUMP_FAIL_STATE,		//12
	E_MDLA_PWR_CMD_BINNING_CHECK,		//13
} EN_MDLA_POWER_CMD;

struct mdla_cmd {
	u32 cmd_id;
	u64 mva;
	u32 count;
	u32 size;
};

struct edma_cmd {
	u32 sw_rst;
	u32 ext_addr;
	u32 num_desp;
	u32 desp_iommu_en;
};

struct power_config {
	u32 user;
	u32 cmd;
	u32 num;
	u32 data[0x10];
};

enum {
	CONFIG_EFUSE = 0,
	CONFIG_LOGLVL,
	CONFIG_MDLA,
	CONFIG_EDMA,
};

struct device_config {
	u32 cfg_mask;
	u32 efuse;
	u32 log_level;
	u32 num_mdla;
	u32 num_edma;
};

typedef struct {
	u8  version;
	u32 cmd_type;
	union {
		struct mdla_cmd mdla_hnd;
		struct edma_cmd edma_hnd;
		struct power_config pwr_cfg;
		struct device_config dev_cfg;
	} cmd;
} ST_MDLA_HAL_CONFIG;

// function
int comm_teec_open_context(u32 uuid_idx);
void comm_teec_close_context(u32 uuid_idx);
int comm_teec_send_command(u32 uuid_idx, u32 cmd_id, struct TEEC_Operation *op, u32 *error);

#endif
