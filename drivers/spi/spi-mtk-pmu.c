// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Owen Tseng <Owen.Tseng@mediatek.com>
 */

#include <linux/spi/spi.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/dma-mapping.h>
#include <linux/of_reserved_mem.h>

#include <linux/rpmsg.h>
#include <linux/completion.h>

#define MSPI_IPCM_MAX_DATA_SIZE	64
#define MSPI_MAX_DATA_SIZE	32
#define MSPI_REQUEST_TIMEOUT (5 * HZ)
#define MSPI_MODE_BITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_NO_CS | SPI_LSB_FIRST)
#define SPI_MAX_BUF_SIZE	4096

enum mspi_ipcm_ack_t {
	MSPI_IPCM_CMD_OP = 0,
	MSPI_IPCM_CMD_BAD_ACK,
	MSPI_IPCM_CMD_ACK,
};

struct mspi_ipcm_msg {
	uint8_t cmd;
	uint32_t size;
	uint8_t data[MSPI_IPCM_MAX_DATA_SIZE];
};

enum mspi_cmd_t {
	MSPI_CMD_OPEN = 0,
	MSPI_CMD_XFER,
};

struct spi_xfer_cmd_pkt {
	uint8_t cmd;
	uint32_t xfer_msgs_pa;
	int32_t num;
	uint16_t flags;
};

struct spi_open_cmd_pkt {
	uint8_t cmd;
	char name[MSPI_IPCM_MAX_DATA_SIZE - sizeof(uint8_t)];
};

struct spi_no_data_res_pkt {
	uint8_t cmd;
};

struct mtk_pmu_mspi_rpmsg {
	struct device *dev;
	struct rpmsg_device *rpdev;
	int (*txn)(struct mtk_pmu_mspi_rpmsg *rpmsg_spi, void *data,
		    size_t len);
	int txn_result;
	struct mspi_ipcm_msg receive_msg;
	struct completion ack;
	struct mtk_pmu_mspi *spi_dev;
	struct list_head list;
};

struct mtk_pmu_mspi {
	raw_spinlock_t lock;
	void __iomem *reg_base;
	struct device *dev;
	int trlen;
	const char *mspi_rpmsg_ept_name;
	struct mtk_pmu_mspi_rpmsg	*rpmsg_dev;
	const char *pmu_spi_name;
	struct rpmsg_device *rpdev;
	bool pmu_spi_opened;
	struct completion rpmsg_bind;
	struct list_head list;
	struct spi_device *slave_dev;
};

/* A list of spi_dev_list */
static LIST_HEAD(spi_dev_list);
/* A list of rpmsg_dev */
static LIST_HEAD(rpmsg_dev_list);
/* Lock to serialise RPMSG device and PWM device binding */
static DEFINE_MUTEX(rpmsg_spi_bind_lock);

/*
 * spi device and rpmsg device binder
 */
static int _mtk_bind_rpmsg_spi(struct mtk_pmu_mspi_rpmsg *rpmsg_spi_dev,
				struct mtk_pmu_mspi *spi_n_dev)
{
	mutex_lock(&rpmsg_spi_bind_lock);

	if (rpmsg_spi_dev) {
		struct mtk_pmu_mspi *spi_chip_dev, *n;

		// check spi chip device list,
		// link 1st spi chip dev which not link to rpmsg dev
		list_for_each_entry_safe(spi_chip_dev, n, &spi_dev_list,
					 list) {
			if (spi_chip_dev->rpmsg_dev == NULL) {
				spi_chip_dev->rpmsg_dev = rpmsg_spi_dev;
				rpmsg_spi_dev->spi_dev = spi_chip_dev;
				complete(&spi_chip_dev->rpmsg_bind);
				break;
			}
		}
	} else {
		struct mtk_pmu_mspi_rpmsg *rpmsg_dev, *n;

		// check rpmsg device list,
		// link 1st rpmsg dev which not link to spi dev
		list_for_each_entry_safe(rpmsg_dev, n, &rpmsg_dev_list, list) {
			if (rpmsg_dev->spi_dev == NULL) {
				rpmsg_dev->spi_dev = spi_n_dev;
				spi_n_dev->rpmsg_dev = rpmsg_dev;
				complete(&spi_n_dev->rpmsg_bind);
				break;
			}
		}
	}

	mutex_unlock(&rpmsg_spi_bind_lock);

	return 0;
}

static int mtk_pmu_mspi_rpmsg_callback(struct rpmsg_device *rpdev,
				       void *data, int len, void *priv, u32 src)
{
	struct mtk_pmu_mspi_rpmsg *pdata_rpmsg;
	struct mspi_ipcm_msg *msg;

	if (!rpdev) {
		dev_err(NULL, "rpdev is NULL\n");
		return -EINVAL;
	}

	pdata_rpmsg = dev_get_drvdata(&rpdev->dev);
	if (!pdata_rpmsg) {
		dev_err(&rpdev->dev, "private data is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		dev_err(&rpdev->dev, "Invalid data or length from src:%d\n",
			src);
		return -EINVAL;
	}

	msg = (struct mspi_ipcm_msg *)data;

	switch (msg->cmd) {
	case MSPI_IPCM_CMD_ACK:
		dev_err(&rpdev->dev, "Got OK ack, response for cmd %d\n",
			msg->data[0]);
		/* store received data for transaction checking */
		memcpy(&pdata_rpmsg->receive_msg, data,
		       sizeof(struct mspi_ipcm_msg));
		pdata_rpmsg->txn_result = 0;
		complete(&pdata_rpmsg->ack);
		break;
	case MSPI_IPCM_CMD_BAD_ACK:
		dev_err(&rpdev->dev, "Got BAD ack, result = %d\n",
			*(int *)&msg->data);
		complete(&pdata_rpmsg->ack);
		break;
	default:
		dev_err(&rpdev->dev, "Invalid command %d from src:%d\n",
			msg->cmd, src);
		return -EINVAL;
	}
	return 0;
}


/*
 * Common procedures for communicate linux subsystem and rpmsg
 */
static inline int __mtk_spi_rpmsg_txn_sync(struct mtk_pmu_mspi *spi_chip_dev,
				 uint8_t cmd)
{
	struct mtk_pmu_mspi_rpmsg *rpmsg_spi = spi_chip_dev->rpmsg_dev;
	struct spi_no_data_res_pkt *res_pk =
	    (struct spi_no_data_res_pkt *)&rpmsg_spi->receive_msg.data;

	if (res_pk->cmd != cmd) {
		dev_err(spi_chip_dev->dev,
			"Out of sync command, send %d get %d\n", cmd,
			res_pk->cmd);
		return -ENOTSYNC;
	}

	return 0;
}

static int __mtk_spi_open_pmu_handle(struct mtk_pmu_mspi *spi_chip_dev,
				       const char *pmu_dev_name)
{
	int ret;
	struct mtk_pmu_mspi_rpmsg *rpmsg_spi = spi_chip_dev->rpmsg_dev;
	struct spi_open_cmd_pkt *cmd_pk;
	struct mspi_ipcm_msg open = {0};

	open.cmd = MSPI_IPCM_CMD_OP;
	open.size = sizeof(struct spi_open_cmd_pkt);
	cmd_pk = (struct spi_open_cmd_pkt *)&open.data;
	cmd_pk->cmd = MSPI_CMD_OPEN;
	strncpy(cmd_pk->name, pmu_dev_name, sizeof(cmd_pk->name));
	cmd_pk->name[sizeof(cmd_pk->name)-1] = '\0';
	/* execute RPMSG command */
	ret = rpmsg_spi->txn(rpmsg_spi, &open, sizeof(struct mspi_ipcm_msg));
	if (ret)
		return ret;

	// check received result
	ret = __mtk_spi_rpmsg_txn_sync(spi_chip_dev, cmd_pk->cmd);
	if (ret)
		return ret;

	return rpmsg_spi->txn_result;
}

static int _mtk_open_pmu_spi_device(struct mtk_pmu_mspi *spi_chip_dev)
{
	int ret = 0;

	if (spi_chip_dev->rpmsg_dev == NULL) {
		dev_err(spi_chip_dev->dev,
			"Can not find spi valid rpmsg device(s)\n");
		return -EBUSY;
	}

	if (spi_chip_dev->pmu_spi_opened == false) {
		// open pmu side spi handle
		ret =
		    __mtk_spi_open_pmu_handle(spi_chip_dev,
						spi_chip_dev->pmu_spi_name);
		if (ret) {
			dev_err(spi_chip_dev->dev,
				"failed to open pmu mspi device %s\n",
				spi_chip_dev->pmu_spi_name);
			return ret;
		}
		dev_err(spi_chip_dev->dev, "open spi dev!!!\n");
		spi_chip_dev->pmu_spi_opened = true;
	}

	return ret;
}

/*
 * RPMSG functions
 */
static int mtk_pmu_spi_txn(struct mtk_pmu_mspi_rpmsg *rpmsg_spi, void *data,
			   size_t len)
{
	int ret;

	/* sanity check */
	if (!rpmsg_spi) {
		dev_err(NULL, "spi rpmsg dev is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		dev_err(rpmsg_spi->dev, "Invalid data or length\n");
		return -EINVAL;
	}
	reinit_completion(&rpmsg_spi->ack);
	rpmsg_spi->txn_result = -EIO;
	ret = rpmsg_send(rpmsg_spi->rpdev->ept, data, len);
	if (ret) {
		dev_err(rpmsg_spi->dev, "spi rpmsg send failed, ret=%d\n", ret);
		return ret;
	}
	// wait for ack
	wait_for_completion_interruptible(&rpmsg_spi->ack);

	return rpmsg_spi->txn_result;
}


static int mtk_pmu_mspi_xfer(struct spi_master *ms, struct spi_message *mesg)
{
	struct mtk_pmu_mspi *mspi_dev = spi_master_get_devdata(ms);
	struct mtk_pmu_mspi_rpmsg *rpmsg_spi = mspi_dev->rpmsg_dev;
	struct mspi_ipcm_msg msg = { 0 };
	struct spi_xfer_cmd_pkt *cmd_pk;
	int ret = 0;
	struct spi_transfer *tfr;
	struct spi_device *spi = mesg->spi;
	// physical address of mspi_msg
	dma_addr_t msg_pa;
	// buffer pointers for dma
	uint8_t **dma_buf_va;
	int i = 0;
	// buffer size for dma_alloc_coherent
	size_t dma_alloc_size;
	uint8_t *dma_buf_va_st;
	dma_addr_t dma_buf_pa_st;

	ret = _mtk_open_pmu_spi_device(mspi_dev);
	if (ret)
		return ret;


	list_for_each_entry(tfr, &mesg->transfers, transfer_list) {

		// allocate for buffer pointers for dma
		dma_alloc_size = (tfr->len);
		void *va = dma_alloc_coherent(mspi_dev->dev, dma_alloc_size,
					&msg_pa, GFP_KERNEL);
		if ((va == NULL) || (dma_alloc_size > SPI_MAX_BUF_SIZE)) {
			dev_err(mspi_dev->dev, "failed allocate memory for msgs[]\n");
			return -ENOMEM;
		}
		if ((tfr->tx_buf))
			memcpy(va, tfr->tx_buf, tfr->len);

		/* packing RPMSG command package */
		cmd_pk = (struct spi_xfer_cmd_pkt *)&msg.data;
		msg.cmd = MSPI_IPCM_CMD_OP;
		msg.size = sizeof(struct spi_xfer_cmd_pkt);
		cmd_pk->cmd = MSPI_CMD_XFER;
		cmd_pk->num = (tfr->len);
		cmd_pk->xfer_msgs_pa = (uint32_t)msg_pa;
		cmd_pk->flags = (tfr->rx_buf) ? 1:0;

		//dev_err(mspi_dev->dev, "cmd_pk->tx_buf: %s!!!\n",cmd_pk->tx_buf);
		ret = rpmsg_spi->txn(rpmsg_spi, (void *)&msg, sizeof(msg));
		if (ret)
			goto mtk_spi_xfer_end;

		// check received result
		ret = __mtk_spi_rpmsg_txn_sync(mspi_dev, cmd_pk->cmd);


mtk_spi_xfer_end:

		// traverse all msgs, retrieve SPI read result for SPI read
		// retrieve all dma_alloc/mapp_...
		if ((tfr->rx_buf) && (ret == 0)) {
			// copy to dma_rdwr_buf_array[i]
			memcpy(tfr->rx_buf, va,  (tfr->len));
			dev_err(mspi_dev->dev, "@@tfr->rx_buf:%s\n", tfr->rx_buf);
		}

		dma_free_coherent(mspi_dev->dev, dma_alloc_size, va, msg_pa);
	}

	mesg->status = ret;
	spi_finalize_current_message(ms);
	return ret;
}

static int mtk_pmu_mspi_rpmsg_probe(struct rpmsg_device *rpdev)
{
	int ret;
	struct mtk_pmu_mspi_rpmsg *pdata_rpmsg;

	pdata_rpmsg = devm_kzalloc(&rpdev->dev,
				   sizeof(struct mtk_pmu_mspi_rpmsg),
				   GFP_KERNEL);

	if (!pdata_rpmsg)
		return -ENOMEM;

	pdata_rpmsg->rpdev = rpdev;
	pdata_rpmsg->dev = &rpdev->dev;
	init_completion(&pdata_rpmsg->ack);
	pdata_rpmsg->txn = mtk_pmu_spi_txn;

	// add to rpmsg_dev list
	INIT_LIST_HEAD(&pdata_rpmsg->list);
	list_add_tail(&pdata_rpmsg->list, &rpmsg_dev_list);

	// check if there is a valid spi dev
	ret = _mtk_bind_rpmsg_spi(pdata_rpmsg, NULL);
	if (ret) {
		dev_err(&rpdev->dev, "binding rpmsg-spi failed! (%d)\n", ret);
		goto bind_fail;
	}

	dev_set_drvdata(&rpdev->dev, pdata_rpmsg);

	return 0;

bind_fail:
	list_del(&pdata_rpmsg->list);
	return ret;
}

static int spi_add_thread(void *param)
{
	int ret;
	//struct mtk_pmu_mspi *mspi_dev = (struct mtk_pmu_mspi *)param;
	struct mtk_pmu_mspi *mspi_dev = spi_master_get_devdata(param);
	struct spi_master *master = (struct spi_master *)param;
	struct spi_board_info board_info = {
		.modalias   = "spidev",
	};

	// complete when rpmsg device and pwm chip device binded
	wait_for_completion(&mspi_dev->rpmsg_bind);
	ret = spi_register_master(master);
	if (ret) {
		dev_err(mspi_dev->dev, "probe add data error: ret=%d\n", ret);
		return ret;
	}

	mspi_dev->slave_dev = spi_new_device(master, &board_info);
	if (!mspi_dev->slave_dev)
		dev_warn(mspi_dev->dev, "spi pmu add device failed\n");

	return 0;
}

static int mtk_pmu_mspi_probe(struct platform_device *pdev)
{
	struct mtk_pmu_mspi *pdata;
	static struct task_struct *t;
	int ret, of_base, of_nmspi;
	struct spi_master *master;

	master = spi_alloc_master(&pdev->dev, sizeof(*pdata));
	if (!master) {
		dev_err(&pdev->dev, "spi_alloc_master() failed\n");
		return -ENOMEM;
	}

	pdata = spi_master_get_devdata(master);
	platform_set_drvdata(pdev, pdata);

	// add to spi dev list
	INIT_LIST_HEAD(&pdata->list);
	list_add_tail(&pdata->list, &spi_dev_list);
	init_completion(&pdata->rpmsg_bind);

	// check if there is a valid pdata
	ret = _mtk_bind_rpmsg_spi(NULL, pdata);
	if (ret) {
		dev_err(&pdev->dev, "binding rpmsg-mspi failed! (%d)\n", ret);
		goto bind_fail;
	}

	raw_spin_lock_init(&pdata->lock);

	master->mode_bits = MSPI_MODE_BITS;
	master->bus_num = -1;
	master->num_chipselect = 1;
	master->transfer_one_message = mtk_pmu_mspi_xfer;
	master->dev.of_node = pdev->dev.of_node;

	pdata->dev = &pdev->dev;
	pdata->pmu_spi_name = of_node_full_name(pdev->dev.of_node);
	pdata->pmu_spi_opened = false;

	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "init reserved memory failed\n");
		return ret;
	}
	dev_dbg(&pdev->dev, "coherent_dma_mask %llX, dma_mask %llX\n",
	pdev->dev.coherent_dma_mask, *(pdev->dev.dma_mask));

	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32))) {
		dev_err(&pdev->dev, "dma_set_mask_and_coherent failed\n");
		ret = -ENODEV;
		goto dma_set_mask_fail;
	}

	/* thread for waiting rpmsg binded, continue rest initial sequence after
	 * rpmsg device binded.
	 */
	t = kthread_run(spi_add_thread, (void *)master, "spi_register");
	if (IS_ERR(t)) {
		dev_err(&pdev->dev, "create thread for add spi failed!\n");
		ret = IS_ERR(t);
		goto bind_fail;
	}

	//ret = _mtk_open_pmu_spi_device(pdata);

	return 0;
bind_fail:
	list_del(&pdata->list);

dma_set_mask_fail:
	of_reserved_mem_device_release(&pdev->dev);
	return ret;
}

static void mtk_pmu_mspi_rpmsg_remove(struct rpmsg_device *rpdev)
{
	dev_set_drvdata(&rpdev->dev, NULL);
	dev_info(&rpdev->dev, "mspi rpmsg remove done\n");
	of_reserved_mem_device_release(&rpdev->dev);
}

static struct rpmsg_device_id mtk_pmu_mspi_rpmsg_id_table[] = {
	{ .name	= "mspi-pmu-ept" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, mtk_pmu_mspi_rpmsg_id_table);

static const struct of_device_id mtk_pmu_mspi_match[] = {
	{
		.compatible = "mediatek,mtk-pmu-mspi",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_pmu_mspi_match);

static struct rpmsg_driver mtk_pmu_mspi_rpmsg_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= mtk_pmu_mspi_rpmsg_id_table,
	.probe		= mtk_pmu_mspi_rpmsg_probe,
	.callback	= mtk_pmu_mspi_rpmsg_callback,
	.remove		= mtk_pmu_mspi_rpmsg_remove,
};


static struct platform_driver mtk_pmu_mspi_driver = {
	.probe		= mtk_pmu_mspi_probe,
	.driver = {
		.name	= "mtk_pmu_mspi",
		.of_match_table = of_match_ptr(mtk_pmu_mspi_match),
	},
};

static int __init mtk_rpmsg_mspi_init_driver(void)
{
	int ret = 0;

	ret = platform_driver_register(&mtk_pmu_mspi_driver);
	if (ret) {
		pr_err("pwm chip driver register failed (%d)\n", ret);
		return ret;
	}

	ret = register_rpmsg_driver(&mtk_pmu_mspi_rpmsg_driver);
	if (ret) {
		pr_err("rpmsg bus driver register failed (%d)\n", ret);
		platform_driver_unregister(&mtk_pmu_mspi_driver);
		return ret;
	}

	return ret;
}

module_init(mtk_rpmsg_mspi_init_driver);

static void __exit mtk_rpmsg_mspi_exit_driver(void)
{
	unregister_rpmsg_driver(&mtk_pmu_mspi_rpmsg_driver);
	platform_driver_unregister(&mtk_pmu_mspi_driver);
}

module_exit(mtk_rpmsg_mspi_exit_driver);

MODULE_DESCRIPTION("MediaTek DTV SoC based MSPI Driver");
MODULE_AUTHOR("OwenTseng <Owen.Tseng@mediatek.com>");
MODULE_LICENSE("GPL");
