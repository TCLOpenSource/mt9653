// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <media/dvb_ca_en50221.h>

#include <linux/module.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/spi/spi.h>

#include "mdrv_adapter.h"
#include "mtk_pcmcia_core.h"
#include "mtk_pcmcia_drv.h"
extern struct dvb_adapter *mdrv_get_dvb_adapter(void);


typedef enum {
	OK = 0,
	NOT_OK = -1,
} CI_STATUS_T;

#define MLP_CI_CORE_DBG(fmt, arg...)	//MLP_KLOG_DETAIL(EN_MLP_MODULE_CI, "[CI]" fmt, ##arg)
#define MLP_CI_CORE_INFO(fmt, arg...)	//pr_info("[mdbginfo_CADD_CI]" fmt, ##arg)
#define MLP_CI_CORE_ERR(fmt, arg...)	//pr_info("[mdbgerr_CADD_CI]" fmt, ##arg)
#define MLP_CI_CORE_ENTRY(fmt, arg...)	//pr_info("[mdbgin_CADD_CI_CORE]" fmt, ##arg)
#define MLP_CI_CORE_EXIT()	//MLP_KLOG_TRACE(EN_MLP_MODULE_CI, "[CI Exit]\n")
#define MLP_CI_ENABLE 1


#define DBG_TSD_CORE(fmt, arg...)	//MLP_KLOG_DETAIL(EN_MLP_MODULE_TSD, "[TSD]" fmt, ##arg)
#define DBG_TSD_CORE_ERR(fmt, arg...)	//MLP_KLOG_ERR(EN_MLP_MODULE_TSD, "[TSD Error]" fmt, ##arg)


#define MsOS_DelayTask(x)  msleep_interruptible((unsigned int)x)
#define PCMCIA_WAIT_READY_POLLING_TIME				10
#define PCMCIA_WAIT_READY_POLLING_TIMEOUT_COUNT		300
#define DRIVER_NAME "mlp-tsd"
#define MAXBUFFERSIZE 4096
#define CI_RESET_DELAY 5
U8 gGotCIS;			// If 1 means alreay got it, otherwise need to get CIS first
U8 CISBuf[MAX_CIS_SIZE];
int _CardState;
bool bPCMCIA_Ready = FALSE;
U8 pu8ReadBuffer[MAXBUFFERSIZE];

static U32 u32IsCardInsert;
static U32 u32PreCardStatus;
static bool bCAMInsert = FALSE;
struct spi_device *mtk_pcmcia_to_spi;
bool polling_flg = FALSE;


static int mtk_pcmcia_remove(struct spi_device *pdev)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	struct mtk_dvb_ci *dvb = spi_get_drvdata(pdev);

	dvb_ca_en50221_release(&dvb->ca);

	pr_info("[%s][%d]\n", __func__, __LINE__);
	return 0;
}

//=======MLP layer

int mlp_ci_slot_reset(struct dvb_ca_en50221 *ca, int slot)
{
	MLP_CI_CORE_ENTRY("[%s][%d] slot=%d\n", __func__, __LINE__, slot);//check

	mdrv_pcmcia_reset_hw(slot);
	if (mdrv_pcmcia_ready_status())
		pr_info("[%s][%d] CardReady=TRUE\n", __func__, __LINE__);
	else
		pr_info("[%s][%d] CardReady=FALSE\n", __func__, __LINE__);

	return OK;
}

int mlp_ci_poll_slot_status(struct dvb_ca_en50221 *ca, int slot, int open)
{
	MLP_CI_CORE_ENTRY("[%s][%d] slot=%d open=%d\n", __func__, __LINE__, slot, open);//check

	U32 bDetectStatus = 0;
	struct mtk_dvb_ci *dvb = spi_get_drvdata(mtk_pcmcia_to_spi);

	if (polling_flg) {
		mdrv_pcmcia_polling(slot);
		bDetectStatus = mdrv_pcmcia_is_module_still_plugged(slot);

		if (bDetectStatus) {
			//pr_info("[%s][%d] Card IN\n", __func__, __LINE__);
			/* set the gpio to high */
			gpiod_set_value(dvb->ci_power_en, 1);

			_CardState = DVB_CA_EN50221_POLL_CAM_PRESENT;
			if (mdrv_pcmcia_ready_status())
				_CardState |= DVB_CA_EN50221_POLL_CAM_READY;
		} else {
			//pr_info("[%s][%d] Card OUT\n", __func__, __LINE__);
			/* set the gpio to low */
			gpiod_set_value(dvb->ci_power_en, 0);

			_CardState = 0;
		}

		/* MLP_CI_CORE_EXIT(); */
		//pr_info("[%s][%d] _CardState=%d\n", __func__, __LINE__, _CardState);
		return _CardState;
	}

	//pr_info("[%s][%d]polling_flg=%d\n", __func__, __LINE__, polling_flg);
	return 0;

}

int mlp_ci_read_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address)
{
	MLP_CI_CORE_ENTRY
		("[%s][%d] slot=%d address=%d\n",
		__func__, __LINE__, slot, address);//check

	int attrAdress = address/2;
	unsigned char value;

	mdrv_pcmcia_read_attrib_mem(slot, attrAdress, &value);

	MLP_CI_CORE_INFO
		("[%s][%d] address=0x%02x value=0x%02x\n",
		__func__, __LINE__, address, value);
	return value;
}

int mlp_ci_write_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address, u8 value)
{
	MLP_CI_CORE_ENTRY
		("[%s][%d] slot=%d address=%d value=%x\n",
		__func__, __LINE__, slot, address, value);//check


	//No need to do address/2 ?

	mdrv_pcmcia_write_attrib_mem(slot, address, value);
	//Can not get return value here


	return 0;
}

int mlp_ci_read_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address)
{
	MLP_CI_CORE_ENTRY
		("[%s][%d] slot=%d address=%d\n",
		__func__, __LINE__, slot, address);//Check
	int Data = 0;


	Data = mdrv_pcmcia_read_io_mem(slot, address);
	MLP_CI_CORE_ENTRY
		("[%s][%d] slot=%d address=0x%02x Data=0x%02x\n",
		__func__, __LINE__, slot, address, Data);

	return Data;
}

int mlp_ci_write_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address, u8 value)
{
	MLP_CI_CORE_ENTRY
		("[%s][%d] slot=%d address=0x%02x value=0x%02x\n",
		__func__, __LINE__, slot, address, value);//check

	mdrv_pcmcia_write_io_mem(slot, address, value);
	//Can not get return value here

	return 0;
}

int mlp_ci_slot_ts_enable(struct dvb_ca_en50221 *ca, int slot)
{
	MLP_CI_CORE_ENTRY("[%s][%d] not support\n", __func__, __LINE__);//check

	return 0;
}

int mlp_ci_slot_shutdown(struct dvb_ca_en50221 *ca, int slot)
{
	MLP_CI_CORE_ENTRY("[%s][%d] slot=%d\n", __func__, __LINE__, slot);
	pr_info("[%s][%d] not support\n", __func__, __LINE__, slot);

	return OK;

}

void mtk_pcmcia_set_spi(u32	max_speed_hz)
{
	MLP_CI_CORE_ENTRY("[%s][%d]max_speed_hz : %d\n", __func__, __LINE__, max_speed_hz);//check
	struct spi_device *spi = mtk_pcmcia_to_spi;

	spi->max_speed_hz = max_speed_hz;
	//pr_info("[%s][%d]max_speed_hz : %d\n", __func__, __LINE__, spi->max_speed_hz);
}


ssize_t mtk_pcmcia_to_spi_read(const char *txbuf, size_t txlen, char *buf, size_t len)
{

	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	struct spi_device *spi = mtk_pcmcia_to_spi;
	//int len = 16;
	int ret = 0;
	//int i;

	//pr_info("[%s][%d]spi device\n", __func__, __LINE__);

	// spi bus get data
	//ret = spi_read(spi, buf, len);
	if ((spi->mode)&SPI_MODE_1) {
		//pr_info("\tmax_speed_hz : %d\n", spi->max_speed_hz);
		ret = spi_write_then_read(spi, txbuf, txlen, buf, len);
		/*
		 *pr_info("SPI get data %d bytes %s\n", len, (ret >= 0)?"success":"failed");
		 *
		 *for( i=0; i<txlen; i++)
		 *	pr_info("send_buf[%u]=%02x", i, txbuf[i]);
		 *pr_info("\n");
		 *
		 *for( i=0; i<len; i++)
		 *	pr_info("read_buf[%u]=%02x", i, buf[i]);
		 *pr_info("\n");
		 */
		if (ret >= 0)
			ret = len;
	}

	return ret;
}

ssize_t mtk_pcmcia_to_spi_write(const char *buf, size_t n)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	struct spi_device *spi = mtk_pcmcia_to_spi;
	int ret = -EINVAL;
	//int i;
	if ((spi->mode)&SPI_MODE_1) {
		if (buf != NULL) {
			// spi bus send data
			ret = spi_write(spi, buf, n);
	/*
	 *		pr_info("SPI send %zd bytes data %s\n", n, (ret >= 0)?"success":"failed");
	 *		for( i=0; i<n; i++)
	 *			pr_info("send_buf[%u]=%02x", i, buf[i]);
	 *		pr_info("\n");
	 */
			if (ret >= 0)
				ret = n;

		}
	}
	return ret;
}

static int mtk_pcmcia_reset_companion_chip(struct gpio_desc *ci_reset)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	gpiod_set_value(ci_reset, 1);
	//MsOS_DelayTask(CI_RESET_DELAY); //change to 5ms
	gpiod_set_value(ci_reset, 0);
	pr_info("[%s][%d] Reset Companion PASS\n", __func__, __LINE__);
	return 0;

}
static int mtk_pcmcia_probe(struct spi_device *pdev)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);//check
	struct mtk_dvb_ci *dvb = NULL;
	int ret = 0;
	int slot_count = 0;
	struct device_node *dn;

	pr_info("[%s][%d]\n", __func__, __LINE__);
	pr_info("probe start\n");

	pr_info("spi device (%p):\n", pdev);
	pr_info("\tmax_speed_hz : %d\n", pdev->max_speed_hz);
	pr_info("\tchip_select : %d\n", pdev->chip_select);
	pr_info("\tbits_per_word : %d\n", pdev->bits_per_word);
	pr_info("\tmode : 0x%04X\n", pdev->bits_per_word);
	pr_info("\tmodalias : '%s'\n", pdev->modalias);
	pr_info("[%s][%d]\n", __func__, __LINE__);

	//spi_set_drvdata(pdev, pdev);
	mtk_pcmcia_to_spi = pdev;
	pr_info("mtk_pcmcia_to_spi (%p):\n", mtk_pcmcia_to_spi);

	dvb = kzalloc(sizeof(struct mtk_dvb_ci), GFP_KERNEL);
	if (!dvb) {
		MLP_CI_CORE_ERR("[%s][%d]kzalloc failed\n", __func__, __LINE__);
		return -ENOMEM;
	}


	pr_info("[%s][%d]\n", __func__, __LINE__);
	dvb->pdev = pdev;

	/* "ci_reset" label is matching the device tree declaration.
	 * OUT_LOW is the value at init
	 */
	dvb->ci_reset = devm_gpiod_get(&pdev->dev, "ci_reset", GPIOD_OUT_HIGH);
	if (IS_ERR(dvb->ci_reset)) {
		dev_err(&pdev->dev, "Unable to get ci_reset from dtsi\n");
		MLP_CI_CORE_ERR("[%s][%d]Unable to get ci_reset from dtsi\n", __func__, __LINE__);
		ret = -ENONET;
		goto mtk_pcmcia_probe_err_exit;
	}
	mtk_pcmcia_reset_companion_chip(dvb->ci_reset);


	pdev->mode |= SPI_MODE_1;
	pr_info("[%s][%d]spi set mode 1\n", __func__, __LINE__);
	mdrv_pcmcia_init_sw(0, 0, FALSE);

	pr_info("[%s][%d]\n", __func__, __LINE__);
	mdrv_pcmcia_init_hw();
	polling_flg = TRUE;
	pr_info("[%s][%d]\n", __func__, __LINE__);

	dvb->adapter = mdrv_get_dvb_adapter();
	if (!dvb->adapter) {
		MLP_CI_CORE_ERR
			("[TSD Error][%s][%d] mdrv_get_dvb_adapter error.\n",
			 __func__, __LINE__);
		ret = -ENODEV;
		goto mtk_pcmcia_probe_err_exit;
	}
	pr_info("[%s][%d]\n", __func__, __LINE__);

	dvb->ca.data = dvb;
	dvb->ca.owner = THIS_MODULE;
	dvb->ca.slot_reset = mlp_ci_slot_reset;
	dvb->ca.poll_slot_status = mlp_ci_poll_slot_status;
	dvb->ca.read_attribute_mem = mlp_ci_read_attribute_mem;
	dvb->ca.write_attribute_mem = mlp_ci_write_attribute_mem;
	dvb->ca.read_cam_control = mlp_ci_read_cam_control;
	dvb->ca.write_cam_control = mlp_ci_write_cam_control;

	pr_info("[%s][%d]\n", __func__, __LINE__);

	dvb->ca.slot_ts_enable = mlp_ci_slot_ts_enable;
	dvb->ca.slot_shutdown = mlp_ci_slot_shutdown;
	//pr_info("[%s][%d] DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE mode\n",__func__, __LINE__);
	//ret = dvb_ca_en50221_init(dvb->adapter, &dvb->ca, DVB_CA_EN50221_FLAG_IRQ_CAMCHANGE, 1);
	pr_info("[%s][%d]\n", __func__, __LINE__);

	dn = pdev->dev.of_node;
	pr_info("[%s][%d]\n", __func__, __LINE__);
	ret = of_property_read_u32(dn, "slot_count", &slot_count);
	if (ret) {
		MLP_CI_CORE_ERR
			("[%s][%d] Failed to get slot_count\n",
			__func__, __LINE__);
		ret = -ENONET;
		goto mtk_pcmcia_probe_err_exit;
	}
	pr_info("[%s][%d] slot_count=%d\n", __func__, __LINE__, slot_count);

	/* "ci_power_en" label is matching the device tree declaration.
	 * OUT_LOW is the value at init
	 */
	dvb->ci_power_en = devm_gpiod_get(&pdev->dev, "ci_power_en", GPIOD_OUT_LOW);
	if (IS_ERR(dvb->ci_power_en)) {
		dev_err(&pdev->dev, "Unable to get ci_power_en from dtsi\n");
		MLP_CI_CORE_ERR
			("[%s][%d] Unable to get ci_power_en from dtsi\n",
			__func__, __LINE__);
		ret = -ENONET;
		goto mtk_pcmcia_probe_err_exit;
	}
	pr_info("[%s][%d] ci_power_en get pass\n", __func__, __LINE__);

	pr_info("[%s][%d] 0 mode(pollng)\n", __func__, __LINE__);
	ret = dvb_ca_en50221_init(dvb->adapter, &dvb->ca, 0, slot_count);

	if (ret != 0) {
		dvb_ca_en50221_release(&dvb->ca);
		MLP_CI_CORE_ERR("ci initialisation failed.\r\n");
	}

	pr_info("[%s][%d]\n", __func__, __LINE__);
	spi_set_drvdata(pdev, dvb);

	pr_info("[%s][%d] succeeded\n", __func__, __LINE__);
	return ret;

mtk_pcmcia_probe_err_exit:
	if (dvb != NULL)
		kfree(dvb);
	return ret;
}


static int mtk_pcmcia_suspend(struct device *pdev)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	struct mtk_dvb_ci *dvb = spi_get_drvdata(mtk_pcmcia_to_spi);
	mdrv_pcmcia_set_power_state(E_POWER_SUSPEND);
	dvb->pdev->mode |= SPI_MODE_0;
	pr_info("[%s][%d]spi set mode 0\n", __func__, __LINE__);
	polling_flg = FALSE; //dvb core stop polling when suspend
	pr_info("[%s][%d]polling_flg=%d\n", __func__, __LINE__, polling_flg);
	return 0;
}

static int mtk_pcmcia_resume(struct device *pdev)
{
	MLP_CI_CORE_ENTRY("[%s][%d]\n", __func__, __LINE__);
	pr_info("[%s][%d]\n", __func__, __LINE__);
	struct mtk_dvb_ci *dvb = spi_get_drvdata(mtk_pcmcia_to_spi);

	mtk_pcmcia_reset_companion_chip(dvb->ci_reset);
	dvb->pdev->mode |= SPI_MODE_1;
	pr_info("[%s][%d]spi set mode 1\n", __func__, __LINE__);
	mdrv_pcmcia_set_power_state(E_POWER_RESUME);
	polling_flg = TRUE;
	pr_info("[%s][%d]polling_flg=%d\n", __func__, __LINE__, polling_flg);

	return 0;
}

static const struct dev_pm_ops mtk_pcmcia_pm_ops = {
	.suspend = mtk_pcmcia_suspend,
	.resume = mtk_pcmcia_resume,
};


static const struct of_device_id mtk_pcmcia_dt_match[] = {
	{.compatible = DRV_NAME},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_pcmcia_dt_match);



static struct platform_device mlp_dvb_tsd_device = {
	.name = "ci",
	.id = -1,
};

static struct platform_device *dvb_devices[] __initdata = {
	&mlp_dvb_tsd_device,
};

//static struct platform_driver mtk_pcmcia_driver = {
static struct spi_driver mtk_pcmcia_driver = {
	.probe = mtk_pcmcia_probe,
	.remove = mtk_pcmcia_remove,
	.driver = {
		   .name = DRIVER_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = mtk_pcmcia_dt_match,
		   .pm = &mtk_pcmcia_pm_ops
		   },
};

//module_platform_driver(mtk_pcmcia_driver);
module_spi_driver(mtk_pcmcia_driver);

MODULE_DESCRIPTION("Mediatek PCMCIA driver");
MODULE_AUTHOR("Jimmy-CY Chen <Jimmy-CY.Chen@mediatek.com>");
MODULE_LICENSE("GPL");
