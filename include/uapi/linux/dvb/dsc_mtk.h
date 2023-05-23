/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef DSC_MTK_H
#define DSC_MTK_H

#include <linux/types.h>
////

typedef enum {
	E_DVR_KEY,
	E_DVR_IV,
	E_DVR_MULTI2_KEY,
	E_DVR_MULTI2_ROUND,
	E_DVR_KEY_IV_TYPE_MAX
} key_data_type;

typedef enum {
	/// Descrambler key type : clear
	E_DSC_KEY_CLEAR = 0,
	/// Descrambler key type : even
	E_DSC_KEY_EVEN,
	/// Descrambler key type : odd
	E_DSC_KEY_ODD,
	/// Enable FSCB
	E_DSC_KEY_FSCB_ENABLE =   1 << 4,
	/// Enable KL Key
	E_DSC_KEY_TO_KL_ENABLE =  1 << 5,
	/// Enable Secure Key
	E_DSC_KEY_SECURE_KEYS_ENABLE =   1 << 7,
} dscmb_key_type;

typedef enum {
	// Odd, Even, and Clear Key
	E_DSC_FLT_3_KEYS        = 0x00000000,
	// Odd and Even Key
	E_DSC_FLT_2_KEYS        = 0x00000001,
	// Odd and Even Key (share key)
	E_DSC_FLT_2_KEYS_SHARE  = 0x00000002,
	// Always use the Key
	E_DSC_FLT_1_KEYS        = 0x00000003,
    //Particular pid slot configurations
	// Enable Privilege pid slot
	E_DSC_FLT_PRIV_KEYS_ENABLE     = 1 << 6,
	// Enable Secure pid slot
	E_DSC_FLT_SECURE_KEYS_ENABLE   = 1 << 7,
} dscmb_flt_type;

typedef enum {
	// Meaning, Main Algo, Sub Algo, Residual block
	E_DSC_TYPE_CSA = 0,    // CSA
	E_DSC_TYPE_NSA_AS_ESA, // NSA as ESA
	E_DSC_TYPE_DES,        // DES
	E_DSC_TYPE_AES,        // AES + CBC + CLR
	E_DSC_TYPE_AES_ECB,    // AES + ECB
	E_DSC_TYPE_AES_SCTE52, // AES + CBC, DBook
	E_DSC_TYPE_AES_CTR,    // AES Counter mode
	E_DSC_TYPE_TDES_ECB,   // TDES + ECB
	E_DSC_TYPE_TDES_SCTE52,// TDES + CBC
	// Synamedia AES, AES Leading CLR EBC
	E_DSC_TYPE_SYN_AES,
	E_DSC_TYPE_MULTI2,     // Multi2 + CBC
	E_DSC_TYPE_CSA3,       // CSA3
	// CSA conformance mode
	E_DSC_TYPE_CSA_CONF,
	E_DSC_TYPE_OC,         // Open Cable
	E_DSC_TYPE_MMT_TLV,         // TLV
} dscmb_type;

typedef enum {
	E_DSC_MAIN_ALGO_AES,
	E_DSC_MAIN_ALGO_CSA2,
	E_DSC_MAIN_ALGO_DES,
	E_DSC_MAIN_ALGO_TDES,
	E_DSC_MAIN_ALGO_MULTI2,
	E_DSC_MAIN_ALGO_CSA2_CONF,
	E_DSC_MAIN_ALGO_CSA3,
	E_DSC_MAIN_ALGO_ASA,
	E_DSC_MAIN_ALGO_TCSA3,
	E_DSC_MAIN_ALGO_ESSA,
	E_DSC_MAIN_ALGO_DEFAULT = 0xF,
	E_DSC_MAIN_ALGO_NUM,
} dscmb_mainalgo;

struct dscmb_engine {
	__u32           engine_id;
	dscmb_mainalgo  main_algo;
};

struct dscmb_connect {
	__u32           engine_id;
	__u32           keyslot;
	__u32           dmx_flt_id;
};

struct dscmb_key_iv {
	__u32           engine_id;
	__u32           keyslot;
	dscmb_key_type  key_type;
	__u64           key;
	__u64           iv;
};

struct dscmb_reset_key {
	__u32           engine_id;
	__u32           keyslot;
	dscmb_key_type  key_type;
};

struct dscmb_keyslot {
	__u32           engine_id;
	dscmb_flt_type  flt_type;
	__u32           keyslot;

};


struct dscmb_set_type {
	__u32                engine_id;
	__u32                keyslot;
	dscmb_type           dsc_type;
};

struct dscmb_mult2 {
	__u32           engine_id;
	__u64           key;
	__u32           round;
};

struct dscmb_ca_crtl {
	__u32           engine_id;
	__u32           tsif;
};

/* IP filter */
struct dsc_ip_addr {
	union {
		__u8 v4[4];
		__u8 v6[16];
	} src_ip_addr;

	union {
		__u8 v4[4];
		__u8 v6[16];
	} dst_ip_addr;

	__u16 src_port;
	__u16 dst_port;
};

struct dscmb_tlvpid {
	__u32                pid;
	struct dsc_ip_addr   ip_addr;
};


struct dscmb_slot_info {
	int num;
	int type;
#define CA_CI            1
#define CA_CI_LINK       2
#define CA_CI_PHYS       4
#define CA_DESCR         8
#define CA_SCRB         16
#define CA_SC          128
	unsigned int flags;
#define CA_DES_UNUSED        0
#define CA_CI_MODULE_PRESENT 1
#define CA_CI_MODULE_READY   2
#define CA_DES_USED          4
#define CA_SCRB_USED         8

};


#ifndef CA_GET_SLOT_INFO
#define CA_GET_SLOT_INFO                _IOR('o', 130, struct dscmb_slot_info)
#endif

#define CA_DSC_SET_ENGINE               _IOW('o', 135, struct dscmb_engine)
#define CA_DSC_FREE_ENGINE              _IOW('o', 136, struct dscmb_engine)
#define CA_DSC_GET_KEYSLOT              _IOWR('o', 137, struct dscmb_keyslot)
#define CA_DSC_FREE_KEYSLOT             _IOW('o', 138, struct dscmb_keyslot)
#define CA_DSC_CONNECT_FLT              _IOW('o', 139, struct dscmb_connect)
#define CA_DSC_DISCONNECT_FLT           _IOW('o', 140, struct dscmb_connect)
#define CA_DSC_RESET_KEY                _IOW('o', 141, struct dscmb_reset_key)
#define CA_DSC_SETPATH                  _IOW('o', 142, struct dscmb_ca_crtl)



#define CA_DSC_SETTYPE                  _IOW('o', 143, struct dscmb_set_type)
#define CA_DSC_SETKEY                   _IOW('o', 144, struct dscmb_key_iv)
#define CA_DSC_SETIV                    _IOW('o', 145, struct dscmb_key_iv)
#define CA_DSC_SETMULT2                 _IOW('o', 146, struct dscmb_mult2)
#define CA_DSC_SETHDCPIV                _IOW('o', 147, struct dscmb_key_iv)

#define CA_SET_SOURCE                   _IOW('o', 120, unsigned int)
#define CA_SET_KEYTOKEN                 _IOW('o', 121, unsigned long long)
#define CA_ADD_PID                      _IOW('o', 122, unsigned int)
#define CA_REMOVE_PID                   _IOW('o', 123, unsigned int)
#define CA_ADD_TLVPID                   _IOW('o', 124, struct dscmb_tlvpid)
#define CA_REMOVE_TLVPID                _IOW('o', 125, struct dscmb_tlvpid)


////
#endif /* DSC_MTK_H */
