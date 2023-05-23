// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/nospec.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>

#include <media/dvb_ca_en50221.h>
#include <media/dvb_ringbuffer.h>

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
//#include <linux/delay.h>
//#include <media/dvb_ca_en50221.h>
//#include <media/dvb_ringbuffer.h>
//#include <linux/vmalloc.h>

#include "mdrv_adapter.h"
#include "mtk_sc_core.h"
#include "mtk_sc_drv.h"
extern struct dvb_adapter *mdrv_get_dvb_adapter(void);


typedef enum {
	OK = 0,
	NOT_OK = -1,
} CI_STATUS_T;

#define MLP_CI_CORE_DBG(fmt, arg...)	//MLP_KLOG_DETAIL(EN_MLP_MODULE_CI, "[CI]" fmt, ##arg)
#define MLP_CI_CORE_INFO(fmt, arg...)	//MLP_KLOG_DEBUG(EN_MLP_MODULE_CI, "[CI Info ]" fmt, ##arg)
#define MLP_CI_CORE_ERR(fmt, arg...)	//MLP_KLOG_ERR(EN_MLP_MODULE_CI, "[CI Error]" fmt, ##arg)
#define MLP_CI_CORE_ENTRY()	//MLP_KLOG_TRACE(EN_MLP_MODULE_CI, "[CI Entry]\n")
#define MLP_CI_CORE_EXIT()	//MLP_KLOG_TRACE(EN_MLP_MODULE_CI, "[CI Exit]\n")
#define MLP_CI_ENABLE 1


#define DBG_TSD_CORE(fmt, arg...)	//MLP_KLOG_DETAIL(EN_MLP_MODULE_TSD, "[TSD]" fmt, ##arg)
#define DBG_TSD_CORE_ERR(fmt, arg...)	//MLP_KLOG_ERR(EN_MLP_MODULE_TSD, "[TSD Error]" fmt, ##arg)
#define DBG_TSD_CORE_ENTRY()	//MLP_KLOG_TRACE(EN_MLP_MODULE_TSD, "[TSD Entry]\n")
#define DBG_TSD_CORE_EXIT()	//MLP_KLOG_TRACE(EN_MLP_MODULE_TSD, "[TSD Exit]\n")
#define MsOS_DelayTask(x)  msleep_interruptible((unsigned int)x)
#define sc_WAIT_READY_POLLING_TIME				10
#define sc_WAIT_READY_POLLING_TIMEOUT_COUNT		300
//#define DRIVER_NAME "mlp-tsd"
#define DRIVER_NAME "mtk-sc"
#define MAXBUFFERSIZE 4096
#define		NAD		0x00
#define MAX_READ_DATA_LEN 255
SC_Param			sInitParams;
MS_U8				_pu8ReadData[255];
MS_U16				_u16ReadDataLen = 255;


/* Information on a CA slot */
struct dvb_ca_slot {
	/* current state of the CAM */
	int slot_state;

	/* mutex used for serializing access to one CI slot */
	struct mutex slot_lock;

	/* Number of CAMCHANGES that have occurred since last processing */
	atomic_t camchange_count;

	/* Type of last CAMCHANGE */
	int camchange_type;

	/* base address of CAM config */
	u32 config_base;

	/* value to write into Config Control register */
	u8 config_option;

	/* if 1, the CAM supports DA IRQs */
	u8 da_irq_supported:1;

	/* size of the buffer to use when talking to the CAM */
	int link_buf_size;

	/* buffer for incoming packets */
	struct dvb_ringbuffer rx_buffer;

	/* timer used during various states of the slot */
	unsigned long timeout;
};

/* Private CA-interface information */
struct dvb_ca_private {
	struct kref refcount;

	/* pointer back to the public data structure */
	struct dvb_ca_en50221 *pub;

	/* the DVB device */
	struct dvb_device *dvbdev;

	/* Flags describing the interface (DVB_CA_FLAG_*) */
	u32 flags;

	/* number of slots supported by this CA interface */
	unsigned int slot_count;

	/* set CI or SMC ctrl flow */
	unsigned int flow_ctrl;

	/* information on each slot */
	struct dvb_ca_slot *slot_info;

	/* wait queues for read() and write() operations */
	wait_queue_head_t wait_queue;

	/* PID of the monitoring thread */
	struct task_struct *thread;

	/* Flag indicating if the CA device is open */
	unsigned int open:1;

	/* Flag indicating the thread should wake up now */
	unsigned int wakeup:1;

	/* Delay the main thread should use */
	unsigned long delay;

	/*
	 * Slot to start looking for data to read from in the next user-space
	 * read operation
	 */
	int next_read_slot;

	/* mutex serializing ioctls */
	struct mutex ioctl_mutex;
};
static void dvb_ca_private_free(struct dvb_ca_private *ca)
{
	unsigned int i;

	dvb_free_device(ca->dvbdev);
	for (i = 0; i < ca->slot_count; i++)
		vfree(ca->slot_info[i].rx_buffer.data);

	kfree(ca->slot_info);
	kfree(ca);
}

static void dvb_ca_private_release(struct kref *ref)
{
	struct dvb_ca_private *ca;

	ca = container_of(ref, struct dvb_ca_private, refcount);
	dvb_ca_private_free(ca);
}

static void dvb_ca_private_get(struct dvb_ca_private *ca)
{
	kref_get(&ca->refcount);
}

static void dvb_ca_private_put(struct dvb_ca_private *ca)
{
	kref_put(&ca->refcount, dvb_ca_private_release);
}
static MS_U8	ns;
static void _Mt_SMC_PrintData(MS_U8 *pu8Data, MS_U16 u16DataLen)
{
	int i;

	pr_info("Rx :\n		  ");
	for (i = 0; i < u16DataLen; i++) {
		pr_info("%02x ", pu8Data[i]);
		if (((i+1) % 16) == 0)
			pr_info("\n");
	}
	pr_info("\n");
}
static int mtk_sc_remove(struct platform_device *pdev)
{
	struct mtk_dvb_sc *dvb = platform_get_drvdata(pdev);

	DBG_TSD_CORE_ENTRY();

	//mdrv_sc_Enable_Interrupt(FALSE);
	//Can not get return value here
	//mdrv_sc_Set_InterruptStatus(FALSE);
	//Can not get return value here

	//mdrv_sc_exit();
	//Can not get return value here

	dvb_ca_en50221_release(&dvb->ca);

	DBG_TSD_CORE_EXIT();

	return 0;
}

//=======MLP layer




static MS_U8 _getlrc(MS_U8 *pbData, MS_U16 iCount)
{
	//TRACE_ENTRY ();
	MS_U8	   chLRC = 0;
	MS_U8	   i = 0;

	for (i = 0; i < iCount; ++i)
		chLRC ^= pbData[i];

	//TRACE_EXIT ();
	return chLRC;
}

int mlp_sc_write_data(struct dvb_ca_en50221 *ca, int slot, u8 *ebuf, int ecount)
{
	MS_U8 au1ActSendBuf[259];
	MS_U16 u16SendBlockLen;
	MS_U8			pcb;

	pr_info("[%s][%d] slot=%d\n", __func__, __LINE__, slot);

	u16SendBlockLen = ecount+4;

	/*
	 * inside the info block ins code is the instruction to be passed
	 * filling the PCB with the send sequence number
	 */
	pcb = ns << 6;

	au1ActSendBuf[0] = NAD;
	au1ActSendBuf[1] = pcb;
	au1ActSendBuf[2] = ecount;

	memcpy(au1ActSendBuf+3, ebuf, ecount);

	au1ActSendBuf[u16SendBlockLen-1] = _getlrc(au1ActSendBuf, ecount+3);

	//_Mt_SMC_PrintData(au1ActSendBuf, u16SendBlockLen);
	/*MLP_CI_CORE_ENTRY();*/
	_u16ReadDataLen = MAX_READ_DATA_LEN;
	if (MDrv_SC_T1_SendRecv(slot,
							au1ActSendBuf,
							&u16SendBlockLen,
							_pu8ReadData,
							&_u16ReadDataLen) != E_SC_OK) {
		pr_err("MDrv_SC_T1SendRecv  Error\n");
		return NOT_OK;
	}
	//_Mt_SMC_PrintData(_pu8ReadData, _u16ReadDataLen);

	ns = !ns;
	/*MLP_CI_CORE_EXIT();*/
	return OK;
}

int mlp_sc_read_data(struct dvb_ca_en50221 *ca, int slot, u8 *ebuf, int ecount)
{
	pr_info("[%s][%d] slot=%d\n", __func__, __LINE__, slot);
	/*MLP_CI_CORE_ENTRY();*/
	*ebuf = (MS_U8)_u16ReadDataLen - (3+1);
	memcpy((ebuf+1), &_pu8ReadData[3], (*ebuf));

	/*MLP_CI_CORE_EXIT();*/
	return OK;
}


int mlp_sc_slot_reset(struct dvb_ca_en50221 *ca, int slot)
{
	pr_info("[%s][%d] slot=%d\n", __func__, __LINE__, slot);
	/*MLP_CI_CORE_ENTRY();*/
	SC_Status status;
	SC_Result  _scresult = E_SC_FAIL;
	MS_U8  _u8AtrBuf[SC_ATR_LEN_MAX];
	MS_U16 _u16AtrLen;
	MS_U8				history[SC_HIST_LEN_MAX];
	MS_U16				history_len = 200;
	MS_U8				u8Protocol = 0xff;
	MS_U8 retry_count = 0;
	MS_U32 delay_ms = 0;
	MS_U8 SetMaxLen[] = {0x00, 0xC1, 0x01, 0xFE, 0x3E};
	MS_U8 _au8ReceivePattern[] = {0x00, 0xE1, 0x01, 0xFE, 0x1E};
	MS_U16 _uCmdLen;
	MS_U8 pu8ReadData[255];
	MS_U16 u16ReadDataLen = 255;

	memset(_u8AtrBuf, 0, SC_ATR_LEN_MAX);
	memset(history, 0, SC_HIST_LEN_MAX);

	do {
		//delay T2s
		delay_ms = history[1]*1000;
		pr_info("[%s][%d] delay %dms\n", __func__, __LINE__, delay_ms);
		msleep_interruptible((unsigned int)delay_ms);

		//COLD RESET
		MDrv_SC_Deactivate(slot);
		MDrv_SC_Activate(slot);
		pr_info("[%s][%d] Activation pass\n", __func__, __LINE__);


		MDrv_SC_GetStatus(slot, &status);
		if (status.bCardIn) {
			pr_info("[%s][%d] card in\n", __func__, __LINE__);
		} else {
			pr_err("[%s][%d] card out\n", __func__, __LINE__);
			return NOT_OK;
		}

		// reset
		if (MDrv_SC_Reset(slot, &sInitParams) != E_SC_OK) {
			pr_err("[%s][%d] failed\n", __func__, __LINE__);
			return NOT_OK;
		}
		pr_info("[%s][%d] reset pass\n", __func__, __LINE__);

		MDrv_SC_ClearState(slot);


		// Timeout value can be tuned
		_scresult = MDrv_SC_GetATR(slot,
									100*5,
									_u8AtrBuf,
									&_u16AtrLen,
									history,
									&history_len);
		if (_scresult != E_SC_OK) {// the timeout value should be tumed by user
			pr_err("[%s][%d] _scresult %d\n", __func__, __LINE__, _scresult);
			pr_info("[%s][%d]retrun failed\n", __func__, __LINE__);
			return NOT_OK;
		}

		if (history_len > 0) {
			pr_info("[%s][%d] T1=0x%02x\n", __func__, __LINE__, history[0]);
			pr_info("[%s][%d] T2=0x%02x\n", __func__, __LINE__, history[1]);
		} else {
			pr_info("[%s][%d] T2=0x%02x\n", __func__, __LINE__, history[1]);
			break;
		}

		retry_count++;
		pr_info("[%s][%d] retry_count=0x%02x\n", __func__, __LINE__, retry_count);
	} while (history[1] > 0 && retry_count < 3);


	// Setup UART mode
	if (_u8AtrBuf[0] == 0x3b) {
		sInitParams.u8UartMode = SC_UART_CHAR_8 | SC_UART_STOP_2 | SC_UART_PARITY_EVEN;
	} else if (_u8AtrBuf[0] == 0x3f) {
		sInitParams.u8UartMode = SC_UART_CHAR_8 | SC_UART_STOP_2 | SC_UART_PARITY_ODD;
	} else {
		pr_err("[%s][%d] Setup UART mode failed\n", __func__, __LINE__);
		return FALSE;
	}

	if (MDrv_SC_Config(slot, &sInitParams) != E_SC_OK) {
		pr_err("[%s][%d] failed\n", __func__, __LINE__);
		return FALSE;
	}

	// Parsing the protocol from ATR
	int i;

	if (_u8AtrBuf[1] & 0x80) {
		MS_U8 u8T0 = _u8AtrBuf[1] >> 4;

		i = 1;
		while (u8T0) {
			if (u8T0 & 1)
				i++;
			u8T0 >>= 1;
		}
		u8Protocol = _u8AtrBuf[i] & 0xF;
	} else {
		u8Protocol = 0;
	}

	if (MDrv_SC_PPS(slot) != E_SC_OK) {
		pr_err("[%s][%d] failed\n", __func__, __LINE__);
		return FALSE;
	}

	pr_info("\n ***Smartcard%d  T=%d	***\n", slot, u8Protocol);
	pr_info("SMC%d: ATR Message :\n", slot);
	pr_info("Rx :\n");
	for (i = 0; i < _u16AtrLen; i++) {
		pr_info("%02x ", _u8AtrBuf[i]);
		if ((i+1) % 16 == 0)
			pr_info("\n");
	}
	pr_info("\nend\n");


	//IFS
	_uCmdLen = sizeof(SetMaxLen);
	u16ReadDataLen = 5;
	if (MDrv_SC_T1_SendRecv(slot,
							SetMaxLen,
							&_uCmdLen,
							pu8ReadData,
							&u16ReadDataLen) != E_SC_OK){
		pr_err("MDrv_SC_T1SendRecv  Error\n");
		return NOT_OK;
	}

	_Mt_SMC_PrintData(pu8ReadData, u16ReadDataLen);

	//compare IFS response
	if (memcmp(pu8ReadData, _au8ReceivePattern, sizeof(_au8ReceivePattern)) != 0) {
		pr_err("IFS response	failed\n");
		return NOT_OK;
	}

	pr_info("IFS response	 PASS\n");
	/*MLP_CI_CORE_EXIT();*/
	return OK;
}

int mlp_sc_poll_slot_status(struct dvb_ca_en50221 *ca, int slot, int open)
{
	return OK;
}

int mlp_sc_slot_shutdown(struct dvb_ca_en50221 *ca, int slot)
{
	pr_info("[%s][%d] slot=%d\n", __func__, __LINE__, slot);
	/*MLP_CI_CORE_ENTRY();*/

	/*MLP_CI_CORE_EXIT();*/
	return OK;

}

static void _Mt_SMC_Notify(MS_U8 u8SCID, SC_Event eEvent)
{
	pr_info("[%s][%d]enable\n", __func__, __LINE__);
}

static int dvb_ca_sc_io_do_ioctl(struct file *file,
				      unsigned int cmd, void *parg)
{
	struct dvb_device *dvbdev = file->private_data;
	struct dvb_ca_private *ca = dvbdev->priv;
	int err = 0;
	int slot = 0;

	pr_info("[%s][%d] slot=%d\n", __func__, __LINE__, slot);

	if (mutex_lock_interruptible(&ca->ioctl_mutex))
		return -ERESTARTSYS;

	switch (cmd) {
	case CA_RESET:
		for (slot = 0; slot < ca->slot_count; slot++)
			err = ca->pub->slot_reset(ca->pub, slot);
		break;

	case CA_GET_CAP: {
		struct ca_caps *caps = parg;

		caps->slot_num = ca->slot_count;
		caps->slot_type = CA_SC;
		caps->descr_num = 0;
		caps->descr_type = 0;
		break;
	}

	case CA_GET_SLOT_INFO: {
		struct ca_slot_info *info = parg;
		struct dvb_ca_slot *sl;

		slot = info->num;
		if ((slot >= ca->slot_count) || (slot < 0)) {
			err = -EINVAL;
			goto out_unlock;
		}

		info->flags = 0;
		info->type = CA_SC;
		break;
	}

	case CA_SEND_MSG: {
		struct ca_msg *send_msg = parg;

		for (slot = 0; slot < ca->slot_count; slot++)
			ca->pub->write_data(ca->pub, slot, send_msg->msg, send_msg->length);

		break;
	}

	case CA_GET_MSG: {
		struct ca_msg *get_msg = parg;
		unsigned char rx_msg[256];

		for (slot = 0; slot < ca->slot_count; slot++) {
			ca->pub->read_data(ca->pub, slot, rx_msg, 0);
			get_msg->length = rx_msg[0];
			memcpy(get_msg->msg, (rx_msg + 1), get_msg->length);
		}

		break;
	}
	default:
		err = -EINVAL;
		break;
	}

out_unlock:
	mutex_unlock(&ca->ioctl_mutex);
	return err;
}
static int dvb_ca_sc_usercopy(struct file *file,
		     unsigned int cmd, unsigned long arg,
		     int (*func)(struct file *file,
		     unsigned int cmd, void *arg))
{
	char    sbuf[128];
	void    *mbuf = NULL;
	void    *parg = NULL;
	int     err  = -EINVAL;

	/*  Copy arguments into temp kernel buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		/*
		 * For this command, the pointer is actually an integer
		 * argument.
		 */
		parg = (void *) arg;
		break;
	case _IOC_READ: /* some v4l ioctls are marked wrong ... */
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
		if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
			/* too big to allocate from stack */
			mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (mbuf == NULL)
				return -ENOMEM;
			parg = mbuf;
		}

		err = -EFAULT;
		if (copy_from_user(parg, (void __user *)arg, _IOC_SIZE(cmd)))
			goto out;
		break;
	}

	/* call driver */
	err = func(file, cmd, parg);
	if (err == -ENOIOCTLCMD)
		err = -ENOTTY;

	if (err < 0)
		goto out;

	/*  Copy results into user buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):
		if (copy_to_user((void __user *)arg, parg, _IOC_SIZE(cmd)))
			err = -EFAULT;
		break;
	}

out:
	kfree(mbuf);
	return err;
}
static long dvb_ca_sc_io_ioctl(struct file *file,
				    unsigned int cmd, unsigned long arg)
{
	return dvb_ca_sc_usercopy(file, cmd, arg, dvb_ca_sc_io_do_ioctl);
}

static int dvb_ca_sc_io_open(struct inode *inode, struct file *file)
{
	struct dvb_device *dvbdev = file->private_data;
	struct dvb_ca_private *ca = dvbdev->priv;
	int err;
	int i;

	pr_info("[%s][%d]\n", __func__, __LINE__);

	if (!try_module_get(ca->pub->owner))
		return -EIO;

	err = dvb_generic_open(inode, file);
	if (err < 0) {
		module_put(ca->pub->owner);
		return err;
	}

	ca->open = 1;
	dvb_ca_private_get(ca);

	return 0;
}

static int dvb_ca_sc_io_release(struct inode *inode, struct file *file)
{
	struct dvb_device *dvbdev = file->private_data;
	struct dvb_ca_private *ca = dvbdev->priv;
	int err;

	pr_info("[%s][%d]\n", __func__, __LINE__);

	/* mark the CA device as closed */
	ca->open = 0;


	err = dvb_generic_release(inode, file);

	module_put(ca->pub->owner);

	dvb_ca_private_put(ca);

	return err;
}

static const struct file_operations dvb_ca_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = dvb_ca_sc_io_ioctl,
	.open = dvb_ca_sc_io_open,
	.release = dvb_ca_sc_io_release,
	.llseek = noop_llseek,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dvb_ca_sc_io_ioctl,
#endif
};

static const struct dvb_device dvbdev_ca = {
	.priv = NULL,
	.users = 1,
	.readers = 1,
	.writers = 1,
#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
	.name = "dvb-ca-sc",
#endif
	.fops = &dvb_ca_fops,
};
static int mtk_sc_probe(struct platform_device *pdev)
{

	struct mtk_dvb_sc *dvb = NULL;
	int ret = 0;
	int slot_count = 0;
	struct device_node *dn;
	int vcc_high_active;
	struct dvb_ca_private *ca = NULL;
	int rf_pop_delay = 0;

	dev_info(&pdev->dev, "[%s][%d]enable\n", __func__, __LINE__);

	DBG_TSD_CORE_ENTRY();

	dvb = devm_kzalloc(&pdev->dev, sizeof(struct mtk_dvb_sc), GFP_KERNEL);
	if (!dvb)
		return -ENOMEM;

	dvb->pdev = pdev;
	dn = pdev->dev.of_node;

	ret = of_property_read_u32(dn, "slot_count", &slot_count);
	if (ret) {
		dev_err(&pdev->dev, "[%s][%d] Failed to get slot_count\n", __func__, __LINE__);
		//of_node_put(bank_node);
		return -ENONET;
	}
	dev_info(&pdev->dev, "[%s][%d] slot_count=%d\n", __func__, __LINE__, slot_count);

	ret = of_property_read_u32(dn, "rf_pop_delay", &rf_pop_delay);
	if (ret) {
		dev_err(&pdev->dev, "[%s][%d] Failed to get rf_pop_delay\n", __func__, __LINE__);
		//of_node_put(bank_node);
		return -ENONET;
	}
	dev_info(&pdev->dev, "[%s][%d] rf_pop_delay=%d\n", __func__, __LINE__, rf_pop_delay);

	ret = of_property_read_u32(dn, "vcc_high_active", &vcc_high_active);
	if (ret) {
		dev_err(&pdev->dev, "[%s][%d] Failed to get vcc_high_active\n", __func__, __LINE__);
		//of_node_put(bank_node);
		return -ENONET;
	}
	dev_err(&pdev->dev, "[%s][%d] vcc_high_active=%d\n", __func__, __LINE__, vcc_high_active);
	ret = mdrv_sc_vcc_init(pdev, vcc_high_active);
	if (ret < 0) {
		dev_err(&pdev->dev, "[%s][%d] vcc_high_active init fail\n", __func__, __LINE__);
		return NOT_OK;
	}
	ret = mdrv_sc_res_init(pdev, &dvb->resource);
	if (ret < 0) {
		dev_err(&pdev->dev, "[%s][%d] resoucre init fail\n", __func__, __LINE__);
		return NOT_OK;
	}

	//mdrv_sc_init_sw(dvb->resource.regmaps->riu_vaddr, dvb->resource.regmaps->ext_riu_vaddr);
	mdrv_sc_init_sw(dvb->resource.regmaps->riu_vaddr);
	ret = mdrv_sc_init(pdev, &dvb->interrupt);
	if (ret < 0) {
		dev_err(&pdev->dev, "[%s][%d] sc init fail\n", __func__, __LINE__);
		return NOT_OK;
	}
	ret = mdrv_sc_clock_init(pdev, &dvb->clock);
	if (ret < 0) {
		dev_err(&pdev->dev, "[%s][%d] sc clk fail\n", __func__, __LINE__);
		return NOT_OK;
	}

	//mdrv_sc_init_hw(0);// u8SCID=0; TBD
	MDrv_SC_Init(0);

	sInitParams.eCardClk = E_SC_CLK_4M;//E_SC_CLK_4P5M
	sInitParams.u8UartMode = SC_UART_CHAR_8 | SC_UART_STOP_2 | SC_UART_PARITY_NO;
	sInitParams.u16ClkDiv = 372;
	sInitParams.u8Convention = 0;
	sInitParams.eVccCtrl = E_SC_VCC_CTRL_8024_ON;
	sInitParams.eVoltage = E_SC_VOLTAGE_3_POINT_3V;
	sInitParams.u8CardDetInvert = E_SC_HIGH_ACTIVE;
	sInitParams.eTsioCtrl = E_SC_TSIO_DISABLE;
	sInitParams.pfOCPControl = NULL;
	MDrv_SC_Open(0, 1, &sInitParams, _Mt_SMC_Notify);
	mdrv_sc_set_rf_pop_delay(rf_pop_delay);
	dvb->adapter = mdrv_get_dvb_adapter();
	if (!dvb->adapter)
		return -ENODEV;

	//mdrv_sc_Init(FALSE); not yat


	//FIX ME: temporarily init gpio here. need to remove in the future
	//mdrv_gpio_init(); ??maybe unused

	dvb->ca.data = dvb;
	dvb->ca.owner = THIS_MODULE;
	dvb->ca.slot_reset = mlp_sc_slot_reset;
	dvb->ca.poll_slot_status = mlp_sc_poll_slot_status;// can't mark??
	dvb->ca.write_data = mlp_sc_write_data;
	dvb->ca.read_data = mlp_sc_read_data;

	//dvb->ca.slot_ts_enable = mlp_ci_slot_ts_enable;
	dvb->ca.slot_shutdown = mlp_sc_slot_shutdown;

	/* initialise the system data */
	ca = kzalloc(sizeof(*ca), GFP_KERNEL);
	if (!ca)
		return -ENONET;

	kref_init(&ca->refcount);
	ca->pub = &dvb->ca;
	ca->flags = 0;
	ca->slot_count = slot_count;

	/* register the DVB device */
	ret = dvb_register_device(dvb->adapter, &ca->dvbdev, &dvbdev_ca, ca,
				  DVB_DEVICE_CA, 0);
	if (ret) {
		dev_err(&pdev->dev, "[%s][%d] dvb register failed\n", __func__, __LINE__);
		return -ENONET;
	}

	mutex_init(&ca->ioctl_mutex);

	platform_set_drvdata(pdev, dvb);

	dev_info(&pdev->dev, "[%s][%d]done\n", __func__, __LINE__);
	return ret;
}


static int mtk_sc_suspend(struct device *pdev)
{

	pr_info("[%s][%d]\n", __func__, __LINE__);

	MDrv_SC_SetPowerState(E_POWER_SUSPEND);
	pr_info("[%s][%d]done\n", __func__, __LINE__);
	return 0;
}

static int mtk_sc_resume(struct device *pdev)
{

	pr_info("[%s][%d]\n", __func__, __LINE__);

	MDrv_SC_SetPowerState(E_POWER_RESUME);
	pr_info("[%s][%d]done\n", __func__, __LINE__);
	return 0;
}

static const struct dev_pm_ops mtk_sc_pm_ops = {
	.suspend = mtk_sc_suspend,
	.resume = mtk_sc_resume,
};


static const struct of_device_id mtk_sc_dt_match[] = {
	{.compatible = DRV_NAME},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_sc_dt_match);



static struct platform_device mlp_dvb_tsd_device = {
	.name = "ci",
	.id = -1,
};

static struct platform_device *dvb_devices[] __initdata = {
	&mlp_dvb_tsd_device,
};

static struct platform_driver mtk_sc_driver = {
	.probe = mtk_sc_probe,
	.remove = mtk_sc_remove,
	.driver = {
		   .name = DRIVER_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = mtk_sc_dt_match,
		   .pm = &mtk_sc_pm_ops
		   },
};

module_platform_driver(mtk_sc_driver);

MODULE_DESCRIPTION("Mediatek SC driver");
MODULE_AUTHOR("Jimmy-CY Chen <Jimmy-CY.Chen@mediatek.com>");
MODULE_LICENSE("GPL");
