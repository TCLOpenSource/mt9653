/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_FCIE_CQE_H
#define MTK_FCIE_CQE_H

//-------------------------------------------------------
#define CQE_VER_15_0                  0x40
#define CQE_VER_31_16                 0x41
#define CQE_CAP                       0x42
#define CQE_CFG                       0x44
#define CQE_CTL                       0x46
#define CQE_INT_STATUS                0x48
#define CQE_INT_STATUS_EN             0x4A
#define CQE_INT_SIGNAL_EN             0x4C
#define CQE_INT_COAL_15_0             0x4E
#define CQE_INT_COAL_31_16            0x4F
#define CQE_TASK_DESC_ADDR_15_0       0x50
#define CQE_TASK_DESC_ADDR_31_16      0x51
#define CQE_TASK_DESC_ADDRU_15_0      0x52
#define CQE_TASK_DESC_ADDRU_31_16     0x53
#define CQE_DOORBELL_15_0             0x54
#define CQE_DOORBELL_31_16            0x55
#define CQE_TASK_COM_NOTIFY_15_0      0x56
#define CQE_TASK_COM_NOTIFY_31_16     0x57
#define CQE_DEVICE_QUEUE_ST_15_0      0x58
#define CQE_DEVICE_QUEUE_ST_31_16     0x59
#define CQE_DEVICE_PEND_TASK_15_0     0x5A
#define CQE_DEVICE_PEND_TASK_31_16    0x5B
#define CQE_TASK_CLR_15_0             0x5C
#define CQE_TASK_CLR_31_16            0x5D
#define CQE_SEND_ST_CFG1_15_0         0x60
#define CQE_SEND_ST_CFG1_31_16        0x61
#define CQE_SEND_ST_CFG2              0x62
#define CQE_RSP_FOR_DCMD_TASK_15_0    0x64
#define CQE_RSP_FOR_DCMD_TASK_31_16   0x65
#define CQE_RSP_MODE_ERR_MASK_15_0    0x68
#define CQE_RSP_MODE_ERR_MASK_31_16   0x69
#define CQE_TASK_ERR_INFO_15_0        0x6A
#define CQE_TASK_ERR_INFO_31_16       0x6B
#define CQE_RSP_INDEX                 0x6C
#define CQE_CMD_RSP_ARG_15_0          0x6E
#define CQE_CMD_RSP_ARG_31_16         0x6F
#define CQE_DEBUG_MODE                0x70
#define CQE_CMD45_CHK_EN              0x7F
//-------------------------------------------------------

/* CQE_CFG        0x44 */
#define BIT_DCMD_EN                     BIT(12)
#define BIT_DESC_SIZE                   BIT(8)
#define BIT_CQE_EN                      BIT(0)

/* CQE_CTL        0x46 */
#define BIT_CLR_ALL_TASK                BIT(8)
#define BIT_HALT_EN                     BIT(0)

/* CQE_INT_STATUS          0x48 */
/* CQE_INT_STATUS_EN     0x4A */
/* CQE_INT_SIGNAL_EN      0x4C */
#define BIT_INT_TCL                     BIT(3)
#define BIT_INT_RED                     BIT(2)
#define BIT_INT_TCC                     BIT(1)
#define BIT_INT_HAC                     BIT(0)
#define BIT_INT_MASK                    (BIT_INT_TCC|BIT_INT_RED)

/* CQE_INT_COAL_15_0          0x4E */
#define BIT_INTC_CNT_TH_EN              BIT(15)
#define BIT_INTC_CNT_TH_MASK            (BIT(12)|BIT(11)|BIT(10)|BIT(9)|BIT(8))
#define BIT_INTC_CNT_TH_SHIFT           8
#define BIT_INTC_TIMEOUT_VAL_EN         BIT(7)
#define BIT_INTC_TIMEOUT_VAL_MASK       (BIT(6)|BIT(5)|BIT(4)|BIT(3)|BIT(2)|BIT(1)|BIT(0))

/* CQE_INT_COAL_31_16          0x4F */
#define BIT_INTC_EN                     BIT(15)
#define BIT_INTC_RET                    BIT(0)

/* TASK_COM_NOTIFY_15_0          0x56 */
/* TASK_COM_NOTIFY_31_16          0x57 */
#define BIT_TASK_COM_NOTIFY_SHIFT      16


/* CQE_SEND_ST_CFG1_15_0    0x60 */
#define BIT_SEND_ST_IDEL_TIMER_MASK     (BIT(16)-1)

/* CQE_SEND_ST_CFG1_31_16   0x61 */
#define BIT_SEND_ST_BLKCNT_MASK         (BIT(3)|BIT(2)|BIT(1)|BIT(0))

/* CQE_RSP_INDEX            0x6C */
#define BIT_LAST_CMD_RSP_INDEX_MASK      (BIT(6)-1)

/* CQE_DEBUG_MODE         0x70*/
#define BIT_CQE_DEBUG_MODE_MASK          (BIT(4)-1)


/* CQE_CMD45_CHK_EN       0x7F*/
#define BIT_AUTO_CMD13_AFTER_CMD45        BIT(0)


/* attribute fields */
#define MTK_CQE_VALID(x)        (x & 1)
#define MTK_CQE_END(x)          ((x & 1) << 1)
#define MTK_CQE_INT(x)          ((x & 1) << 2)
#define MTK_CQE_ACT(x)          ((x & 7) << 3)

/* data command task descriptor fields */
#define MTK_CQE_FORCED_PROG(x)      ((x & 1) << 6)
#define MTK_CQE_CONTEXT(x)          ((x & 0xF) << 7)
#define MTK_CQE_DATA_TAG(x)         ((x & 1) << 11)
#define MTK_CQE_DATA_DIR(x)         ((x & 1) << 12)
#define MTK_CQE_PRIORITY(x)         ((x & 1) << 13)
#define MTK_CQE_QBR(x)              ((x & 1) << 14)
#define MTK_CQE_REL_WRITE(x)        ((x & 1) << 15)
#define MTK_CQE_BLK_COUNT(x)        ((x & 0xFFFF) << 16)
#define MTK_CQE_BLK_ADDR(x)         ((x & 0xFFFFFFFF) << 32)

/* direct command task descriptor fields */
#define MTK_CQE_CMD_INDEX(x)        ((x & 0x3F) << 16)
#define MTK_CQE_CMD_TIMING(x)       ((x & 1) << 22)
#define MTK_CQE_RESP_TYPE(x)        ((x & 3) << 23)
#define MTK_CQE_CMD_ARG(x)          ((x & 0xFFFFFFFF) << 32)

/* task error info */
#define MTK_CQE_TERRI_INDEX(x)        (x & 0x3F)
#define MTK_CQE_TERRI_TASK(x)         ((x >> 8) & 0x1F)
#define MTK_CQE_TERRI_VALID(x)        (x & BIT(15))


#define CQE_DCMD_SLOT 31
#define CQE_NUM_SLOTS 32
#define CQE_TASK_COM_NOTIFY_SHIFT 16

//===========================================================
// CQE Task Descriptor
//===========================================================

struct task_descriptor {
	bool valid;
	bool end;
	bool intr_mode;
	u8   act;
	bool force_prg;
	u8   context;
	bool tag_req;
	bool dir;       /*0:wrtie, 1:read*/
	bool priority;
	bool qbr;
	bool rel_write;
	u16  blk_cnt;
	u32  blk_addr;
};


//===========================================================
// CQE Transfer Descriptor
//===========================================================
struct transfer_descriptor {
	u32 valid         : 1;
	u32 end           : 1;
	u32 intr_mode     : 1;
	u32 act           : 3;
	u32 dir           : 1;  /*1:wrtie, 0:read*/
	u32 fde           : 1;
	u32 miu_sel       : 2;
	u32:6;
	u32 dmalen        : 16;
	u32 address1;
	u32 address2;
	u32 blk_addr;
};


struct mtk_cqe_slot {
	struct mmc_request *mrq;
	int flags;
#define MTK_CQE_EXTERNAL_TIMEOUT    BIT(0)
#define MTK_CQE_COMPLETED           BIT(1)
#define MTK_CQE_HOST_CRC            BIT(2)
#define MTK_CQE_HOST_TIMEOUT        BIT(3)
#define MTK_CQE_HOST_OTHER          BIT(4)
};


struct mtk_cqe_host {
	void __iomem *cqebase;
	void __iomem *taskdesc;

	struct mmc_host *mmc;

	/* relative card address of device */
	unsigned int rca;

	int num_slots;
	int dcmd_slot;
	int qcnt;

	bool enabled;
	bool halted;
	bool init_done;
	bool waiting_for_idle;
	bool recovery_halt;

	size_t link_desc_size;
	size_t trans_desc_size;
	/* 64/128 bit depends on CQHCI_CFG */
	u8 link_desc_len;
	u8 trans_desc_len;

	struct transfer_descriptor *link_desc_base;
	dma_addr_t link_desc_dma_base;

	struct transfer_descriptor *trans_desc_base;
	dma_addr_t trans_desc_dma_base;

	wait_queue_head_t wait_queue;
	struct mtk_cqe_slot *slot;
	u16 sd_status;
};

#define TASK_COMP_NOTIFY_MASK 0xFFFF
#define TASK_COMP_NOTIFYH_SHIFT 16

#endif
