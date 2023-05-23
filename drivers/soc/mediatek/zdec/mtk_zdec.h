/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef	__mtk_ZDEC_H__
#define	__mtk_ZDEC_H__

#define	ZDEC_OK						   (0L)
#define	ZDEC_E_NOT_READY			   (-1L)
#define	ZDEC_E_INVALID_MODE			   (-2L)
#define	ZDEC_E_INVALID_DICTIONARY_SIZE (-3L)
#define	ZDEC_E_OUT_OF_MEMORY		   (-4L)
#define	ZDEC_E_IRQ_REGISTER			   (-5L)

#define	EMMC_SIZE 512
#define	BUFFER_NUM 2
#define	HW_MAX_IBUFF_SZ	(128 * 1024)

extern void __iomem *zdec_base;

enum mtk_hash_type {
		   HW_IOVEC_HASH_NONE = 1<<1,
		   HW_IOVEC_HASH_SHA1 = 1<<2,
		   HW_IOVEC_HASH_SHA256 = 1<<3,
		   HW_IOVEC_HASH_MD5 = 1<<4,
};

enum mtk_comp_type {
		   HW_IOVEC_COMP_UNCOMPRESSED = 1<<1,
		   HW_IOVEC_COMP_GZIP = 1<<2,
		   HW_IOVEC_COMP_ZLIB = 1<<3,
		   HW_IOVEC_COMP_LZO = 1<<4,
		   HW_IOVEC_COMP_MAX = 1<<5,
};

struct mtk_req_hw {
		enum mtk_comp_type compr_type;
		enum mtk_hash_type hash_type;
		u8 *hashdata;	// calculated hash data	is saved
};

struct zdec_adma_table {
	struct zdec_adma_table_entry *entry;
	int	cur_id;
	int	max_id;
};

struct zdec_adma_table_entry {
	union {
		struct {
			uint8_t	last_flag:1;
			uint8_t	addr_msb:2;
			uint16_t nand_page_cnt:13;
			uint16_t emmc_job_cnt;
			uint32_t addr_lsb;
		};
		uint64_t entry;
	};
};

struct mtk_zdec_priv_data {
	struct zdec_adma_table *input_table;
	struct zdec_adma_table *output_table;
	int	shift_byte;
};

struct mtk_unzip_buf {
	void	   **page_vaddr;
	void		*vaddr;
	struct page	**pages;
	dma_addr_t	*paddr;
	int			ref_count;
	size_t		max_size;
	size_t		size; //input compressed size
};

enum zdec_mode_e {
	ZMODE_DEC_CONT = 0,		 //	decode in contiguous mode
	ZMODE_DEC_SCATTER_EMMC,	 //	decode in scatter mode with	eMMC table
	ZMODE_DEC_SCATTER_NAND,	 //	decode in scatter mode with	NAND table
	ZMODE_LOAD_DICT_MIU,	 //	load preset	dictionary in MIU mode
	ZMODE_LOAD_DICT_RIU,	 //	load preset	dictionary in RIU mode
};

struct zdec_data {
	struct completion wq;
	struct mutex lock;
	int	err;
	void (*cb)(int err,	int	decompressed, void *arg);
	void *cb_arg;
};

// Initializes ZDEC	HW.
// Parameters:
//zmode:			 see enum definition
//preset_dict_size: byte size of preset dictionary
//fcie_handshake:	 ZDEC-FCIE handshake mode. 1 to	turn on	and	0 to turn off.
//shift_byte:		 byte shift
//shift_bit:		 bit shift
//obuf_miu:		 miu bank of output	buffer
//obuf_addr:		 physical memory address of	output buffer
//obuf_size:		 byte size of output buffer
//in_page_size:
//byte size per NAND	page for input NAND	table;
//only used with ZMODE_DEC_SCATTER_NAND
//out_page_size:
//byte size per NAND	page for output	NAND table;
//only used with ZMODE_DEC_SCATTER_NAND
// Note:
//The NAND page sizes
//(in_page_size, out_page_size) must be obtainable	by left	shifting 512.
//	   i.e.	Valid values include (512 << 0), (512 << 1), (512 << 2), (512 << 3), etc.
// Return:
//	   0 if	successful,	otherwise a	negative error code.
int	mtk_zdec_init(
	enum zdec_mode_e	  zmode,
	unsigned int  preset_dict_size,
	unsigned char fcie_handshake,
	unsigned int  shift_byte,
	unsigned char shift_bit,
	unsigned char obuf_miu,
	phys_addr_t	 obuf_addr,
	unsigned int  obuf_size,
	unsigned int  in_page_size,
	unsigned int  out_page_size);

// Feeds data buffer to	ZDEC HW.
// Parameters:
//last: 1 if this is the last buffer or ADMA table	for	the	bitstream. 0 otherwise.
//miu:	 miu bank of the data buffer
//sadr: starting physical address of the buffer
//size: byte size of the buffer
// Return:
//	   0 if	successful,	otherwise a	negative error code.
int	mtk_zdec_feed
	(unsigned char	last,
	unsigned char	miu,
	phys_addr_t sadr,
	unsigned int size);

// Checks ZDEC buffer availability when	using contiguous mode
//	to decode bitstream	or load	preset dictionary.
// Return:
//	   0 if	an available buffer	exists.
int	mtk_zdec_check_internal_buffer(void);

// Checks whether ADMA table is	completely processed when using	scatter	mode.
// Return:
//	   0 if	ADMA table is completely processed.
int	mtk_zdec_check_adma_table_done(void);

// Checks whether preset dictionary	is loaded when using MIU mode.
// Return:
//	   0 if	preset dictionary is completely	loaded.
int	mtk_zdec_check_dict_miu_load_done(void);

// Checks whether decoding is completed.
// Return:
//	   -1 if decoding is not complete.
//	   Otherwise, byte size	of decoded data.
int	mtk_zdec_check_decode_done(void);

// Loads dictionary	in RIU mode.
// Return:
//	   0 if	preset dictionary is successfully loaded.
int	mtk_zdec_riu_load_dict(unsigned	char *dict);

// Releases	resources allocated	by ZDEC	api.
void mtk_zdec_release(void);

void mtk_zdec_lock(void);

void mtk_zdec_unlock(void);

int	mtk_zdec_wait(void *uzpriv);

int	try_mtk_zdec_lock(void);

void mtk_zdec_dump_all_register(void);

int	register_zdec_irq(int irq);

struct zdec_adma_table *zdec_adma_table_create(int	etry_cnt);

int	fill_zdec_adma_table
	(struct zdec_adma_table *table,
	phys_addr_t addr,
	unsigned long size,
	bool last);

void zdec_adma_table_free(struct zdec_adma_table *table);

int	mtk_init_decompress_async(
	struct scatterlist *sgl,
	unsigned int comp_bytes,
	struct page	**opages,
	int	npages,
	void **uzpriv,
	void (*cb)(int err,	int	decompressed, void *arg),
	void *arg,
	bool may_wait,
	int	shift_byte,
	struct mtk_req_hw *rq_hw);

void mtk_fire_decompress_async(void	*uzpriv);

struct mtk_unzip_buf *mtk_unzip_alloc(size_t len);

void mtk_unzip_free(struct mtk_unzip_buf *buf);

#endif

