// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#include <linux/module.h>
#include <linux/slab.h>
#include <linux/irqreturn.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/dma-mapping.h>
#include <linux/blkdev.h>
#include <dt-bindings/clock/mt5876_ckgen00_reg.h>

#include "mhal_zdec.h"
#include "mtk_zdec.h"

#define ZMEM_SIZE                       0x8000UL    // 32K
#define ZMEM_ALIGNMENT                  16UL
#define DICTIONARY_ALIGNMENT            16UL
#define MIU_WORD_BITS                   128UL
#define LX_BASE							0x20000000

#define GET_REG_PAGE_SIZE(byte_size)    (byte_size >> 10)
// see 0x46, reg_zdec_page_size, reg_zdec_out_page_size

#define TRACE(s, args...)	pr_debug("ZDEC: "s, ## args)

#define SET_0xffffffff 0xffffffff

#define SET_32 32

#define SET_0x03 3

#define EMMC_SIZE 512

unsigned char    zmem_miu;
void *zmem_virt;
phys_addr_t zmem_phys;

enum merak_regbank_type {
        CLK_PM,
        CLK_NONPM,
        CLK_BANK_TYPE_MAX,
};


#if defined(MSOS_TYPE_NOS)
extern zdec_hw_cfg_t g_hwcfg;
#endif

extern int clock_write(enum merak_regbank_type bank_type, unsigned int reg_addr, u16 value,
		unsigned int start, unsigned int end);

static struct zdec_data zdec_control;

struct mtk_unzip_buf *zdec_input_buf;

spinlock_t zdec_buffer_lock;

static int buffer_size;

int mtk_zdec_allocate_zmem(void)
{
	int ret = 0;

	if (zmem_virt == NULL) {
		zmem_virt = kmalloc(ZMEM_SIZE + ZMEM_ALIGNMENT, GFP_KERNEL);
		if (zmem_virt == NULL)
			ret = ZDEC_E_OUT_OF_MEMORY;
	}

	if (!ret) {
		zmem_phys = virt_to_phys(zmem_virt);
		zmem_phys = (zmem_phys + ZMEM_ALIGNMENT - 1) & ~(ZMEM_ALIGNMENT - 1);
		zmem_miu = 0;
		zmem_phys -= LX_BASE;
	}

	return ret;
}


int mtk_zdec_init(
	enum zdec_mode_e   zmode,
	unsigned int  preset_dict_size,
	unsigned char fcie_handshake,
	unsigned int  shift_byte,
	unsigned char shift_bit,
	unsigned char obuf_miu,
	phys_addr_t obuf_addr,
	unsigned int  obuf_size,
	unsigned int  in_page_size,
	unsigned int  out_page_size)
{
	unsigned char dec_mode = CONTIGUOUS_MODE;
	unsigned char tbl_fmt = EMMC_TABLE;
	unsigned char op_mode = DECODING;

	switch (zmode) {
	case ZMODE_DEC_CONT:
			break;
	case ZMODE_DEC_SCATTER_EMMC:
			dec_mode = SCATTER_MODE;
			break;
	case ZMODE_DEC_SCATTER_NAND:
			dec_mode = SCATTER_MODE;
			tbl_fmt = NAND_TABLE;
			break;
	case ZMODE_LOAD_DICT_MIU:
			op_mode = LOADING_PRESET_DICT_MIU;
			break;
	case ZMODE_LOAD_DICT_RIU:
			op_mode = LOADING_PRESET_DICT_RIU;
			break;
	default:
			return ZDEC_E_INVALID_MODE;
	}

	if (((preset_dict_size % DICTIONARY_ALIGNMENT) != 0)
	    || (op_mode != DECODING && preset_dict_size == 0)) {
		return ZDEC_E_INVALID_DICTIONARY_SIZE;
	}

	if (mtk_zdec_allocate_zmem() != 0)
		return ZDEC_E_OUT_OF_MEMORY;

	MHal_ZDEC_Conf_Reset();
	MHal_ZDEC_Conf_Zmem(zmem_miu, zmem_phys, ZMEM_SIZE);
	MHal_ZDEC_Conf_FCIE_Handshake(fcie_handshake == 0 ? 0 : 1);
	MHal_ZDEC_Conf_Input_Shift(
	    shift_byte / BITS2BYTE(MIU_WORD_BITS),
	    shift_byte % BITS2BYTE(MIU_WORD_BITS),
	    shift_bit);
	MHal_ZDEC_Conf_Preset_Dictionary((unsigned int)preset_dict_size);

	int ret;

	if (op_mode == DECODING) {
		if (dec_mode == CONTIGUOUS_MODE) {
			ret = ZDEC_E_INVALID_MODE;
		} else {
			MHal_ZDEC_Conf_Scatter_Mode(
			obuf_miu,
			obuf_addr,
			tbl_fmt,
			GET_REG_PAGE_SIZE(in_page_size),
			GET_REG_PAGE_SIZE(out_page_size));
			ret = MHal_ZDEC_Start_Operation(op_mode);
		}
	} else
		ret = ZDEC_E_INVALID_MODE;
	return ret;
}
EXPORT_SYMBOL(mtk_zdec_init);


int mtk_zdec_feed(unsigned char last, unsigned char miu, phys_addr_t sadr, unsigned int size)
{
	MHal_ZDEC_Feed_Data(last, miu, sadr, size);
	return ZDEC_OK;
}
EXPORT_SYMBOL(mtk_zdec_feed);


int mtk_zdec_check_internal_buffer(void)
{
	return MHal_ZDEC_Check_Internal_Buffer();
}
EXPORT_SYMBOL(mtk_zdec_check_internal_buffer);


int mtk_zdec_check_adma_table_done(void)
{
	return MHal_ZDEC_Check_ADMA_Table_Done();
}
EXPORT_SYMBOL(mtk_zdec_check_adma_table_done);


int mtk_zdec_check_dict_miu_load_done(void)
{
	return MHal_ZDEC_Check_MIU_Load_Dict_Done();
}
EXPORT_SYMBOL(mtk_zdec_check_dict_miu_load_done);


int mtk_zdec_check_decode_done(void)
{
	return MHal_ZDEC_Check_Decode_Done();
}
EXPORT_SYMBOL(mtk_zdec_check_decode_done);


int mtk_zdec_riu_load_dict(unsigned char *dict)
{
	return MHal_ZDEC_RIU_Load_Preset_Dictionary(dict);
}
EXPORT_SYMBOL(mtk_zdec_riu_load_dict);


void mtk_zdec_release(void)
{
#if defined(__KERNEL__)
	if (!zmem_virt)
		kfree(zmem_virt);
#endif
	zmem_phys = 0;
	zmem_virt = NULL;
}
EXPORT_SYMBOL(mtk_zdec_release);

int mtk_zdec_wait(void *uzpriv)
{
	struct mtk_zdec_priv_data *saved_data = (struct mtk_zdec_priv_data *) (uzpriv);

	wait_for_completion(&zdec_control.wq);
	int err = 0;

	if (zdec_control.err)
		err = zdec_control.err;
	zdec_adma_table_free(saved_data->input_table);
	zdec_adma_table_free(saved_data->output_table);
	kfree(saved_data);

//	handle hash result?

	mtk_zdec_unlock();
	return err;
}
EXPORT_SYMBOL(mtk_zdec_wait);

void mtk_zdec_lock(void)
{
	mutex_lock(&zdec_control.lock);
}
EXPORT_SYMBOL(mtk_zdec_lock);

int try_mtk_zdec_lock(void)
{
	return mutex_trylock(&zdec_control.lock);
}
EXPORT_SYMBOL(try_mtk_zdec_lock);

void mtk_zdec_unlock(void)
{
	mutex_unlock(&zdec_control.lock);
}
EXPORT_SYMBOL(mtk_zdec_unlock);

struct zdec_adma_table *zdec_adma_table_create(int etry_cnt)
{
	struct zdec_adma_table *new_table =
			kmalloc(sizeof(struct zdec_adma_table), GFP_KERNEL | __GFP_ZERO);

	if (!new_table)
		return NULL;

	int entry_size = sizeof(struct zdec_adma_table_entry) * etry_cnt;

	new_table->entry =
			kmalloc(entry_size,
					GFP_KERNEL | __GFP_ZERO);

	if (!new_table->entry) {
		kfree(new_table);
		return NULL;
	}
	new_table->cur_id = 0;
	new_table->max_id = etry_cnt - 1;
	return new_table;
}
EXPORT_SYMBOL(zdec_adma_table_create);

void zdec_adma_table_free(struct zdec_adma_table *table)
{
	if (table) {
		kfree(table->entry);
		kfree(table);
	}
}

int fill_zdec_adma_table(
	struct zdec_adma_table *table,
	phys_addr_t addr,
	unsigned long size,
	bool last)
{
	if (table->cur_id > table->max_id)
		return -1;
	table->entry[table->cur_id].last_flag = last;
	if (addr > SET_0xffffffff)
		table->entry[table->cur_id].addr_msb = ((addr >> SET_32) & SET_0x03);
	table->entry[table->cur_id].addr_lsb = addr & SET_0xffffffff;
	table->entry[table->cur_id].emmc_job_cnt = DIV_ROUND_UP(size, EMMC_SIZE);
	table->cur_id++;
	return 0;
}
EXPORT_SYMBOL(fill_zdec_adma_table);

void mtk_zdec_dump_all_register(void)
{
	dump_all_val();
}

static irqreturn_t mtk_zdec_irq_handler(int irq, void *data)
{
	int zdec_outlen;

	int err = MHal_ZDEC_handler(&zdec_outlen);

	zdec_control.err = err;
	if (zdec_control.cb)
		zdec_control.cb(err, zdec_outlen, zdec_control.cb_arg);
	complete(&zdec_control.wq);
	return IRQ_HANDLED;
}

int register_zdec_irq(int zdec_irq)
{
	int ret = 0;

	if (zdec_irq) {
		init_completion(&zdec_control.wq);
		int irq_flag = IRQF_SHARED | IRQF_ONESHOT;

		if (request_threaded_irq
			(zdec_irq,
			mtk_zdec_irq_handler,
			NULL,
			irq_flag,
			"zdec_irq_handler",
			&zdec_control)) {
			TRACE("register irq fail\n");
			ret = -1;
		}
		ret = 0;
	} else
		ret = -1;
	return ret;
}

struct mtk_unzip_buf *mtk_unzip_alloc(size_t len)
{
	if (len > HW_MAX_IBUFF_SZ) {
		TRACE("[ZDEC] error %d\n", __LINE__);
		return NULL;
	}
	spin_lock(&zdec_buffer_lock);
	int i;

	struct mtk_unzip_buf *ret;

	for (i = 0; i < BUFFER_NUM; i++) {
		if (!zdec_input_buf[i].ref_count) {
			zdec_input_buf[i].ref_count = 1;
			zdec_input_buf[i].size = len;
			break;
		}
	}
	spin_unlock(&zdec_buffer_lock);
	if (i < BUFFER_NUM)
		ret = &(zdec_input_buf[i]);
	else {
		TRACE("[ZDEC] error %d\n", __LINE__);
		ret = NULL;
	}
	return ret;
}
EXPORT_SYMBOL(mtk_unzip_alloc);

void mtk_unzip_free(struct mtk_unzip_buf *buf)
{
	spin_lock(&zdec_buffer_lock);
	buf->size = 0;
	buf->ref_count = 0;
	spin_unlock(&zdec_buffer_lock);
}
EXPORT_SYMBOL(mtk_unzip_free);

static s32 zdec_buffer_init(int size, struct device *dev)
{
	int i;

	int j;

	int count = DIV_ROUND_UP(size, PAGE_SIZE);

	int buf_page_size = count * sizeof(struct page *);

	int vaddr_size = count * sizeof(void *);

	int paddr_size = count * sizeof(dma_addr_t);

	buffer_size = size;

	zdec_input_buf = kzalloc(sizeof(struct mtk_unzip_buf) * BUFFER_NUM, GFP_KERNEL);
	for (i = 0; i < BUFFER_NUM; i++) {
		zdec_input_buf[i].pages = kmalloc(buf_page_size, GFP_KERNEL | __GFP_ZERO);
		zdec_input_buf[i].page_vaddr = kmalloc(vaddr_size, GFP_KERNEL | __GFP_ZERO);
		zdec_input_buf[i].paddr = kmalloc(paddr_size, GFP_KERNEL | __GFP_ZERO);
		for (j = 0; j < count; j++) {
			zdec_input_buf[i].pages[j] = alloc_page(GFP_KERNEL);
			if (!zdec_input_buf[i].pages[j])
				return -ENOMEM;
			zdec_input_buf[i].page_vaddr[j] = page_address(zdec_input_buf[i].pages[j]);
		}
		zdec_input_buf[i].vaddr = vmap(zdec_input_buf[i].pages, count, VM_MAP, PAGE_KERNEL);
		if (!zdec_input_buf[i].vaddr)
			return -ENOMEM;
		zdec_input_buf[i].max_size = size;
		zdec_input_buf[i].size = 0;
		zdec_input_buf[i].ref_count = 0;
	}
	return 0;
}

void zdec_buffer_free(struct device *dev, int size)
{
	int i;

	int count = DIV_ROUND_UP(size, PAGE_SIZE);

	int j;

	for (i = 0; i < BUFFER_NUM; i++) {
		vunmap(zdec_input_buf[i].vaddr);
		for (j = 0; j < count; j++)
			free_pages(zdec_input_buf[i].pages[j], 0);
	}
}

static const struct of_device_id mtk_zdec_match[] = {
	{ .compatible = "ZDEC" },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_zdec_match);

static s32 zdec_probe(struct platform_device *pdev)
{
	int zdec_irq = platform_get_irq(pdev, 0);

	spin_lock_init(&zdec_buffer_lock);
	struct device *dev = &pdev->dev;

	static const u64 dmamask = DMA_BIT_MASK(SET_32);

	int res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!res)
		return -EINVAL;
	zdec_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(zdec_base)) {
		dev_err(&pdev->dev, "err zdec=%#lx\n",
				(unsigned long)zdec_base);
		return PTR_ERR(zdec_base);
	}


	dev->dma_mask = (u64 *)&dmamask;
	dev->coherent_dma_mask = DMA_BIT_MASK(SET_32);

	if (clock_write(CLK_NONPM, REG_SW_EN_ZDEC_VLD2ZDEC, 1, REG_SW_EN_ZDEC_VLD2ZDEC_S, REG_SW_EN_ZDEC_VLD2ZDEC_E) < 0)
		return -1;

	if (!zdec_irq)
		return -1;
	if (register_zdec_irq(zdec_irq))
		return -1;

	if (zdec_buffer_init(HW_MAX_IBUFF_SZ, dev)) {
		TRACE("[ZDEC] init failed %d\n", __LINE__);
		return -1;
	}

	return 0;
}

static s32 zdec_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	zdec_buffer_free(dev, HW_MAX_IBUFF_SZ);
	return 0;
}

static struct platform_driver zdec_driver = {

	.probe	= zdec_probe,
	.remove	= zdec_remove,


	.driver = {
		.name = "ZDEC",
		.owner = THIS_MODULE,
		.of_match_table = mtk_zdec_match
	},
};

static int __init zdec_driver_init(void)
{
	int ret;

	MHal_ZDEC_RIU_Bank_Init();
	mutex_init(&zdec_control.lock);
	ret = platform_driver_register(&zdec_driver);
	return ret;
}

static void __exit zdec_driver_exit(void)
{
	mtk_zdec_release();
	platform_driver_unregister(&zdec_driver);
}

int mtk_init_decompress_async(
	struct scatterlist *sgl,
	unsigned int comp_bytes,
	struct page **opages,
	int npages,
	void **uzpriv,
	void (*cb)(int err,	int	decompressed, void *arg),
	void *arg,
	bool may_wait,
	int shift_byte,
	struct mtk_req_hw *rq_hw)
{
	int err;

	if (!may_wait) {
		if (!try_mtk_zdec_lock())
			return -1;
	} else {
		mtk_zdec_lock();
	}
	zdec_control.err = 0;
	zdec_control.cb = NULL;
	zdec_control.cb_arg = NULL;
	struct mtk_zdec_priv_data *saved_data =
				kmalloc(sizeof(struct mtk_zdec_priv_data), GFP_KERNEL | __GFP_ZERO);

	if (!saved_data)
		goto decompress_fail;
	struct scatterlist *sg;

	int input_sg_nents = sg_nents(sgl);

	struct zdec_adma_table *input_table = zdec_adma_table_create(input_sg_nents);
	int idx;

//  handle input first
	for_each_sg(sgl, sg, input_sg_nents, idx) {
		err = fill_zdec_adma_table(input_table,
				sg_phys(sg), sg->length, (idx == input_sg_nents - 1));
		if (err) {
			zdec_adma_table_free(input_table);
			goto decompress_fail;
		}
	}
	saved_data->input_table = input_table;

// handle output
	struct zdec_adma_table *output_table = zdec_adma_table_create(npages);

	if (!output_table) {
		zdec_adma_table_free(input_table);
		goto decompress_fail;
	}
	for (idx = 0; idx < npages; idx++) {
		fill_zdec_adma_table(output_table,
				page_to_phys(opages[idx]), PAGE_SIZE, (idx == npages - 1));
		if (err) {
			zdec_adma_table_free(input_table);
			zdec_adma_table_free(output_table);
			goto decompress_fail;
		}
	}
	saved_data->output_table = output_table;
	saved_data->shift_byte = shift_byte;
	*uzpriv = saved_data;
// save input & output
	if (cb) {
		zdec_control.cb = cb;
		zdec_control.cb_arg = arg;
	}
//  handle hash
//	if (rq_hw){
//		;
//	}
	return 0;
decompress_fail:
	mtk_zdec_unlock();
	kfree(saved_data);
	return -1;
}
EXPORT_SYMBOL(mtk_init_decompress_async);

void mtk_fire_decompress_async(void *uzpriv)
{
	struct mtk_zdec_priv_data *saved_data = (struct mtk_zdec_priv_data *) uzpriv;

	mtk_zdec_init(
		ZMODE_DEC_SCATTER_EMMC, //zdec_mode_e   zmode
		0,                            //preset_dict_size
		0,                            //unsigned char fcie_handshake
		saved_data->shift_byte,                            //unsigned int  shift_byte
		0,                            //unsigned char shift_bit
		0,                     //unsigned char obuf_miu
		virt_to_phys(saved_data->output_table->entry), //unsigned int  obuf_addr
		0,
		EMMC_SIZE,
		EMMC_SIZE);
	mtk_zdec_feed(
		1,                            //unsigned char last
		0,                      //unsigned char miu
		virt_to_phys(saved_data->input_table->entry), //input entry
		sizeof(struct zdec_adma_table_entry) * saved_data->input_table->cur_id);
}
EXPORT_SYMBOL(mtk_fire_decompress_async);

module_init(zdec_driver_init);
module_exit(zdec_driver_exit);
MODULE_LICENSE("GPL");
