// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author jefferry.yen <jefferry.yen@mediatek.com>
 */
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#if defined(CONFIG_SERIAL_8250_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>

#include "8250.h"

#define UART_DEFAULT_BAUD		(115200)
#define UART_BAUD_DIV_SCALE		(16)

#define CONSOLE_TTY_FLUSH_RETRY		(10)
#define CONSOLE_TTY_FLUSH_PERIOD_MS	(20)

#define UART_SYNTH_DIV_BOUNDARY		(32)
#define UART_SYNTH_DECIMAL_BITS		(11)

#define UART_SYNTH_REG_SIZE		(8)
#define UART_SYNTH_REG_CTRL		(0)
#define UART_SYNTH_SW_RSTZ_MSK		0x0001
#define UART_SYNTH_NF_EN_MSK		0x0100
#define UART_SYNTH_REG_NF		(4)

#define MTK_UART_FIFO_SZ		(16)

#define MTK_UART_USR			(7)
#define MTK_UART_RST			(10)

#define MTK_UART_IIR_MSK		(0x3F)

#define UART_REG_SW_RSTZ_MSK		(0x01)
#define UART_REG_FORCE_CTS_ENABLE_MSK	(0x10)

struct mtk8250_dtv_data {
	void __iomem *synth_base;
	int line;
	int switched_line;
	struct uart_8250_port *port;
	struct clk_bulk_data *clks;
	int clk_num;
	struct clk *suspend_clk;
	struct clk *resume_clk;
	bool high_speed;
	struct timer_list silent_tmr;
};

/*
 *  UART proprietary control
 */
static bool console_need_disable;
static DEFINE_SPINLOCK(mtk8250_console_disable_lock);

/*
 * This "device" covers _all_ UART proprietary control
 */
static struct platform_device *uat_control_dev;

#ifdef MODULE
module_param_named(log_disable, console_need_disable, bool, 0644);
MODULE_PARM_DESC(log_disable, "Module parameter to switch uart console enable/disable.");
#else
static int __init mtk_console_lock(char *unused)
{
	console_need_disable = true;
	return 1;
}
__setup("log_disable", mtk_console_lock);
#endif

/*
 * Override io function, proprietary console enable/disable control
 */
#define UART_KEY_SIZE		(8)

static unsigned char keyword[] = "00112233";
static unsigned char keypool[UART_KEY_SIZE] = {0};
static u32 mtk_console_verbose = 1;
static u32 keyidx;

static unsigned int mtk8250_dtv_mem_serial_in(struct uart_port *p, int offset)
{
	unsigned int data;
	int i;
	struct mtk8250_dtv_data *mtk_up = dev_get_drvdata(p->dev);

	offset = offset << p->regshift;
	data = readb(p->membase + offset);

	/* check switch uart silent */
	if (unlikely(uart_console(p)) && unlikely(offset == (UART_RX << p->regshift))) {
		keypool[keyidx++] = data;
		keyidx %= UART_KEY_SIZE;

		for (i = 0; i < UART_KEY_SIZE; i++)
			if (keypool[(keyidx + i) % UART_KEY_SIZE] != keyword[i])
				break;

		if (i >= UART_KEY_SIZE) {
			mtk_console_verbose = !mtk_console_verbose;
			if (!mtk_console_verbose)
				// trigger timer in console disable(silent) state
				mod_timer(&mtk_up->silent_tmr, jiffies + uart_poll_timeout(p));
		}
	}

	return data;
}

static void mtk8250_dtv_mem_serial_out(struct uart_port *p, int offset, int value)
{
	/* check silent out */
	if (unlikely((!mtk_console_verbose || console_need_disable) && uart_console(p) &&
	    (offset == UART_TX)))
		return;

	offset = offset << p->regshift;
	writeb(value, p->membase + offset);
}

/*
 * In console disable, UART neither transmitter empty nor transmit-hold-register empty interrupts
 * will be triggered since UART_TX is not written. Need a timer to flush out uart circular buffer.
 */
static inline void __mtk8250_do_stop_tx(struct uart_8250_port *p)
{
	if (serial8250_clear_THRI(p))
		serial8250_rpm_put_tx(p);
}

static void mtk8250_silent_flush_xmit(struct timer_list *t)
{
	struct mtk8250_dtv_data *mtk_up = from_timer(mtk_up, t, silent_tmr);
	struct uart_8250_port *up = mtk_up->port;
	struct uart_port *port = &up->port;
	struct circ_buf *xmit = &port->state->xmit;
	unsigned long flags;

	spin_lock_irqsave(&up->port.lock, flags);

	if (port->x_char) {
		port->icount.tx++;
		port->x_char = 0;
		goto silent_flush_xmit_end;
	}
	if (uart_tx_stopped(port)) {
		serial8250_rpm_get(up);
		__mtk8250_do_stop_tx(up);
		serial8250_rpm_put(up);
		goto silent_flush_xmit_end;
	}
	if (uart_circ_empty(xmit)) {
		__mtk8250_do_stop_tx(up);
		goto silent_flush_xmit_end;
	}

	while (!uart_circ_empty(xmit)) {
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}

	uart_write_wakeup(port);

	/*
	 * With RPM enabled, we have to wait until the FIFO is empty before the
	 * HW can go idle. So we get here once again with empty FIFO and disable
	 * the interrupt and RPM in __stop_tx()
	 */
	if (uart_circ_empty(xmit))
		__mtk8250_do_stop_tx(up);

silent_flush_xmit_end:

	spin_unlock_irqrestore(&up->port.lock, flags);

	/* only register timer operation for console port, no need check console port status */
	if (unlikely(!mtk_console_verbose || console_need_disable)) {
		/* need trigger timer again in console disable(silent) state */
		mod_timer(&mtk_up->silent_tmr, jiffies + uart_poll_timeout(&up->port));
	}
}

/*
 *  8250 driver framework
 */
static void mtk8250_dtv_set_termios(struct uart_port *port, struct ktermios *termios,
				    struct ktermios *old)
{
	struct mtk8250_dtv_data *mtk_up = dev_get_drvdata(port->dev);
	struct circ_buf *xmit = &port->state->xmit;

	dev_info(port->dev, "set termios with remain %ld.\n", uart_circ_chars_pending(xmit));

	if (mtk_up->high_speed) {
		speed_t req_speed = tty_termios_baud_rate(termios);
		unsigned long clk_speed = clk_get_rate(mtk_up->clks[0].clk);
		unsigned long uartclk_tmp;
		uint16_t synth_div;
		unsigned long synth_div_rem;
		int i;

		/* high speed uart, calculate clock synthesizer according requested baud rate */

		dev_info(port->dev, "set synthesizer for high speed uart port\n");
		dev_info(port->dev, "request baudrate %d at clk %ld\n", req_speed, clk_speed);

		/* Special case: B0 rate. */
		if (req_speed == 0)
			req_speed = UART_DEFAULT_BAUD;

		/* DLM-DLL will be (synth_tmp / UART_BAUD_DIV_SCALE) + 1 */
		uartclk_tmp = clk_speed;
		uartclk_tmp /= (req_speed * UART_BAUD_DIV_SCALE * UART_SYNTH_DIV_BOUNDARY);
		uartclk_tmp++;
		uartclk_tmp *= (req_speed * UART_BAUD_DIV_SCALE);
		if (uartclk_tmp >= U32_MAX) {
			dev_err(port->dev, "report uart clock overflow %ld\n", uartclk_tmp);
			uartclk_tmp &= U32_MAX;
		}
		port->uartclk = uartclk_tmp;
		dev_info(port->dev, "report uart clock %d\n", port->uartclk);

		/* calculate uart synthesizer register setting */
		/* integer part */
		synth_div = clk_speed / port->uartclk;
		if (synth_div >= UART_SYNTH_DIV_BOUNDARY)
			dev_err(port->dev, "synth, divider out of range (%d)\n", synth_div);

		/* decimal part, port->uartclk = uartclk_tmp now */
		synth_div_rem = clk_speed % port->uartclk;
		for (i = 0; i < UART_SYNTH_DECIMAL_BITS; i++) {
			dev_info(port->dev, "synth. divider %d, remain %ld\n", synth_div,
				 synth_div_rem);
			uartclk_tmp >>= 1;
			synth_div <<= 1;
			if (synth_div_rem >= uartclk_tmp) {
				synth_div |= 1;
				synth_div_rem -= uartclk_tmp;
			}
		}
		dev_info(port->dev, "synth. register setting 0x%04X\n", synth_div);

		/* setup synthesizer */
		writew(~(UART_SYNTH_SW_RSTZ_MSK | UART_SYNTH_NF_EN_MSK),
		       mtk_up->synth_base + UART_SYNTH_REG_CTRL);
		writew(synth_div, mtk_up->synth_base + UART_SYNTH_REG_NF);
		writew(UART_SYNTH_SW_RSTZ_MSK | UART_SYNTH_NF_EN_MSK,
		       mtk_up->synth_base + UART_SYNTH_REG_CTRL);
	}

	port->status &= ~UPSTAT_AUTOCTS;
	if (termios->c_cflag & CRTSCTS)
		port->status |= UPSTAT_AUTOCTS;

	if (!uart_circ_empty(xmit)) {
		int retry = CONSOLE_TTY_FLUSH_RETRY;

		while (!uart_circ_empty(xmit) && retry) {
			dev_info(port->dev, "buffer remain %ld\n", uart_circ_chars_pending(xmit));
			msleep(CONSOLE_TTY_FLUSH_PERIOD_MS);
			retry--;
		}

		if (!uart_circ_empty(xmit)) {
			dev_warn(port->dev, "clear buffer remain %ld\n",
				 uart_circ_chars_pending(xmit));
			uart_circ_clear(xmit);
			port->ops->stop_tx(port);
		}
	}

	serial8250_do_set_termios(port, termios, old);
}

static int mtk8250_dtv_startup(struct uart_port *port)
{
	int ret;
	struct mtk8250_dtv_data *mtk_up = dev_get_drvdata(port->dev);

	ret = serial8250_do_startup(port);
	if (uart_console(port)) {
		// timer for flush uart xmit buffer when console disabled
		timer_setup(&mtk_up->silent_tmr, mtk8250_silent_flush_xmit, 0);
		// start timer if console already disabled
		if (unlikely(!mtk_console_verbose || console_need_disable))
			mod_timer(&mtk_up->silent_tmr, jiffies + uart_poll_timeout(port));
	}

	return ret;
}

static void mtk8250_dtv_shutdown(struct uart_port *port)
{
	struct mtk8250_dtv_data *mtk_up = dev_get_drvdata(port->dev);

	serial8250_do_shutdown(port);
	if (uart_console(port))
		// delete timer for flush uart xmit buffer when console disabled
		del_timer_sync(&mtk_up->silent_tmr);
}

static void mtk8250_dtv_do_pm(struct uart_port *port, unsigned int state, unsigned int old)
{
	if (state == UART_PM_STATE_ON)
		// might trigger device runtime resume (enable module clock)
		pm_runtime_get_sync(port->dev);

	serial8250_do_pm(port, state, old);

	if (state == UART_PM_STATE_OFF)
		// might trigger device runtime suspend (disable module clock)
		pm_runtime_put_sync_suspend(port->dev);
}

static int mtk8250_dtv_handle_irq(struct uart_port *p)
{
	unsigned int iir;
	unsigned char status;
	unsigned long flags;
	struct uart_8250_port *up = up_to_u8250p(p);

	iir = p->serial_in(p, UART_IIR);

	/*
	 * There are ways to get Designware-based UARTs into a state where
	 * they are asserting UART_IIR_RX_TIMEOUT but there is no actual
	 * data available.  If we see such a case then we'll do a bogus
	 * read.  If we don't do this then the "RX TIMEOUT" interrupt will
	 * fire forever.
	 *
	 * This problem has only been observed so far when not in DMA mode
	 * so we limit the workaround only to non-DMA mode.
	 */
	if (!up->dma && ((iir & MTK_UART_IIR_MSK) == UART_IIR_RX_TIMEOUT)) {
		status = p->serial_in(p, UART_LSR);

		if (!(status & (UART_LSR_DR | UART_LSR_BI)))
			(void) p->serial_in(p, UART_RX);
	}

	/*
	 * procedure of serial8250_handle_irq(p, iir), intercept console RX data when console is
	 * disabled
	 */
	if (iir & UART_IIR_NO_INT)
		goto mtk8250_dtv_handle_no_int_irq;

	spin_lock_irqsave(&p->lock, flags);

	status = serial_port_in(p, UART_LSR);

	if (unlikely(!mtk_console_verbose || console_need_disable) && unlikely(uart_console(p))) {
		/* console is in disabled, only handle rx data for checking enable password */
		do {
			if (likely(status & UART_LSR_DR))
				(void)p->serial_in(p, UART_RX);

			if (mtk_console_verbose)
				/* console re-enabled, remain rest data for next iteration */
				break;

			status = p->serial_in(p, UART_LSR);

		} while (status & UART_LSR_DR);
	} else if (status & (UART_LSR_DR | UART_LSR_BI))
		/* normal status, original procedure for handle uart (without DMA) */
		status = serial8250_rx_chars(up, status);

	serial8250_modem_status(up);
	if ((status & UART_LSR_THRE) && (up->ier & UART_IER_THRI))
		serial8250_tx_chars(up);

	uart_unlock_and_check_sysrq(p, flags);

	return 1;

mtk8250_dtv_handle_no_int_irq:

	if ((iir & UART_IIR_BUSY) == UART_IIR_BUSY) {
		/* Clear the USR */
		(void)p->serial_in(p, MTK_UART_USR);
		return 1;
	}

	return 0;
}

/*
 * switch uart disable sysfs group, in /sys/devices/platform/uart_control
 */
static ssize_t console_disable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;

	str += scnprintf(str, end - str, "%d\n", console_need_disable);
	return (str - buf);
}

static ssize_t console_disable_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long flags;
	bool console_disable = false;
	int ret;

	if (!buf)
		return -EINVAL;

	ret = kstrtobool(buf, &console_disable);
	if (ret == 0) {
		spin_lock_irqsave(&mtk8250_console_disable_lock, flags);
		console_need_disable = console_disable;
		spin_unlock_irqrestore(&mtk8250_console_disable_lock, flags);

		if (console_need_disable) {
			int i;
			struct uart_8250_port *up;
			struct mtk8250_dtv_data *mtk_up;

			// get console uart port handle, traverse all uart_8250_port
			for (i = 0 ; i < CONFIG_SERIAL_8250_NR_UARTS ; i++) {
				up = serial8250_get_port(i);
				if (uart_console(&up->port))
					break;
			}

			if (i < CONFIG_SERIAL_8250_NR_UARTS) {
				/*
				 * console uart port found, trigger timer in console disable(silent)
				 * state
				 */
				mtk_up = dev_get_drvdata(up->port.dev);
				mod_timer(&mtk_up->silent_tmr,
					  jiffies + uart_poll_timeout(&up->port));
			}
		}
	}

	return (ret < 0) ? ret : count;
}

static DEVICE_ATTR_RW(console_disable);

/*
 * exchange UART port ttySx and ttySy sysfs group, in /sys/devices/platform/uart_control
 */
static int uart_check_swap_8250_port(int line1, int line2, struct uart_8250_port *up1,
				     struct uart_8250_port *up2)
{
	struct mtk8250_dtv_data *mtk_up1, *mtk_up2;

	// check ports have the same status (both activate or deactivate)
	if (tty_port_active(&up1->port.state->port) != tty_port_active(&up2->port.state->port)) {
		dev_err(up1->port.dev, "can not switch uart port with different status.\n");
		return -EBUSY;
	}

	if (!up1->port.mapbase || !up2->port.mapbase) {
		dev_err(up1->port.dev, "can not switch uart port with invalid uart port.\n");
		return -EINVAL;
	}

	// only allow switch out or switch back for single UART IP
	mtk_up1 = up1->port.private_data;
	if ((mtk_up1->switched_line != mtk_up1->line) &&
	    (mtk_up1->switched_line != up2->port.line)) {
		dev_err(up1->port.dev, "only can switch back to ttyS%d.\n", mtk_up1->line);
		return -EINVAL;
	}
	mtk_up2 = up2->port.private_data;
	if ((mtk_up2->switched_line != mtk_up2->line) &&
	    (mtk_up2->switched_line != up1->port.line)) {
		dev_err(up2->port.dev, "only can switch back to ttyS%d.\n", mtk_up2->line);
		return -EINVAL;
	}

	return 0;
}

static int uart_swap_8250_port(struct device *dev, struct uart_8250_port *up1,
			       struct uart_8250_port *up2, bool port_active)
{
	unsigned long flags1, flags2;
	struct mtk8250_dtv_data *mtk_up1, *mtk_up2;
	struct uart_port *p1, *p2;
	unsigned char __iomem *tmp_membase;
	resource_size_t tmp_mapbase;
	unsigned int tmp_irq;
	bool tmp_high_speed;
	struct clk_bulk_data *tmp_clks;
	int tmp_clk_num;
	struct clk *tmp_clk;
	int res1 = 0, res2 = 0;

	p1 = &up1->port;
	p2 = &up2->port;
	mtk_up1 = p1->private_data;
	mtk_up2 = p2->private_data;

	// switch ports, lock tty mutex for switching
	mutex_lock(&(p1->state->port.mutex));
	mutex_lock(&(p2->state->port.mutex));
	spin_lock_irqsave(&p1->lock, flags1);
	spin_lock_irqsave(&p2->lock, flags2);

	// hack swap irq: unlink original irq
	if (port_active) {
		up1->ops->release_irq(up1);
		up2->ops->release_irq(up2);
	}

	// hack swap irq: link new irq
	// hack swap irq for link
	tmp_irq = p1->irq;
	p1->irq = p2->irq;
	p2->irq = tmp_irq;

	if (port_active) {
		res1 = up1->ops->setup_irq(up1);
		res2 = up2->ops->setup_irq(up2);
	}

	// hack swap uart port membase
	tmp_membase = p1->membase;
	p1->membase = p2->membase;
	p2->membase = tmp_membase;

	// hack swap uart port mapbase
	tmp_mapbase = p1->mapbase;
	p1->mapbase = p2->mapbase;
	p2->mapbase = tmp_mapbase;

	// UART IP module clocks
	tmp_clks = mtk_up1->clks;
	mtk_up1->clks = mtk_up2->clks;
	mtk_up2->clks = tmp_clks;

	tmp_clk_num = mtk_up1->clk_num;
	mtk_up1->clk_num = mtk_up2->clk_num;
	mtk_up2->clk_num = tmp_clk_num;

	// switched_line id, use tmp_irq variable
	tmp_irq = mtk_up1->switched_line;
	mtk_up1->switched_line = mtk_up2->switched_line;
	mtk_up2->switched_line = tmp_irq;

	// high speed synthesizer setting
	tmp_membase = mtk_up1->synth_base;
	mtk_up1->synth_base = mtk_up2->synth_base;
	mtk_up2->synth_base = tmp_membase;

	// high speed clock parent
	tmp_clk = mtk_up1->suspend_clk;
	mtk_up1->suspend_clk = mtk_up2->suspend_clk;
	mtk_up2->suspend_clk = tmp_clk;

	tmp_clk = mtk_up1->resume_clk;
	mtk_up1->resume_clk = mtk_up2->resume_clk;
	mtk_up2->resume_clk = tmp_clk;

	tmp_high_speed = mtk_up1->high_speed;
	mtk_up1->high_speed = mtk_up2->high_speed;
	mtk_up2->high_speed = tmp_high_speed;

	// unlock tty mutex after switch port finish
	spin_unlock_irqrestore(&p2->lock, flags2);
	spin_unlock_irqrestore(&p1->lock, flags1);
	mutex_unlock(&(p2->state->port.mutex));
	mutex_unlock(&(p1->state->port.mutex));

	dev_notice(dev, "switch done, res1 %d res2 %d\n", res1, res2);

	return (!res1 && !res2) ? 0 : -1;
}

static ssize_t uart_switch_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned int line1, line2;
	char *str1, *str2 = (char *)buf;
	struct uart_8250_port *up = NULL, *up1 = NULL, *up2 = NULL;
	struct uart_port *p1 = NULL, *p2 = NULL;
	int i;

	if (!buf)
		return -EINVAL;

	// get input string
	str1 = strsep(&str2, ",");
	if (!str1 || !str2 || kstrtouint(str1, 10, &line1) || kstrtouint(str2, 10, &line2))
		return -EINVAL;

	dev_notice(dev, "prepare swap ttyS%d and ttyS%d UART IP\n", line1, line2);

	if (line1 == line2)
		return count;

	// get uart port handle, traverse all uart_8250_port
	for (i = 0 ; i < CONFIG_SERIAL_8250_NR_UARTS ; i++) {
		up = serial8250_get_port(i);
		if (up->port.line == line1)
			up1 = up;
		if (up->port.line == line2)
			up2 = up;
	}

	if (!up1 || !up2)
		// port not found
		return -EINVAL;

	if (uart_check_swap_8250_port(line1, line2, up1, up2) < 0)
		return -EINVAL;

	p1 = &up1->port;
	p2 = &up2->port;

	if (uart_swap_8250_port(dev, up1, up2, tty_port_active(&p1->state->port)) < 0) {
		// switch failed, try switch back
		(void)uart_swap_8250_port(dev, up1, up2, tty_port_active(&p1->state->port));
		return -EINVAL;
	}

	dev_info(dev, "framework line %d switched to line %d\n", p1->line,
		 ((struct mtk8250_dtv_data *)(p1->private_data))->switched_line);
	dev_info(dev, "framework line %d switched to line %d\n", p2->line,
		 ((struct mtk8250_dtv_data *)(p2->private_data))->switched_line);

	return count;
}

static DEVICE_ATTR_WO(uart_switch);

/*
 * get UART port switch status sysfs group, in /sys/devices/platform/uart_control
 */
static ssize_t uart_switch_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
	int i;
	struct uart_8250_port *up;
	struct uart_port *p;
	struct mtk8250_dtv_data *mtk_up;

	for (i = 0 ; i < CONFIG_SERIAL_8250_NR_UARTS ; i++) {
		up = serial8250_get_port(i);
		p = &up->port;
		mtk_up = p->private_data;

		if (!mtk_up)
			dev_info(dev, "ttyS%d is unused\n", p->line);
		else {
			dev_info(dev, "ttyS%d is original ttyS%d use ttyS%d's reg is %sactivate\n",
				 p->line, mtk_up->line, mtk_up->switched_line,
				 tty_port_active(&p->state->port) ? "" : "de");
			str += scnprintf(str, end - str, "%d%d%d", mtk_up->line,
					 mtk_up->switched_line, tty_port_active(&p->state->port));
		}
	}

	return (str - buf);
}

static DEVICE_ATTR_RO(uart_switch_status);

static struct attribute *mtk8250_uart_control_attrs[] = {
	&dev_attr_console_disable.attr,
	&dev_attr_uart_switch.attr,
	&dev_attr_uart_switch_status.attr,
	NULL,
};

static const struct attribute_group mtk8250_control_attr_group = {
	.attrs = mtk8250_uart_control_attrs,
};

/*
 * driver probe function
 */
static int mtk8250_dtv_probe_of(struct platform_device *pdev, struct uart_port *p,
				struct mtk8250_dtv_data *mtk_up)
{
	struct device_node *np = pdev->dev.of_node;
	unsigned int prop_value = 0;
	int ret;
	struct resource res;

	memset(p, 0, sizeof(*p));

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "no registers defined\n");
		return -EINVAL;
	}
	p->mapbase = res.start;
	p->mapsize = resource_size(&res);
	p->membase = devm_ioremap(&pdev->dev, res.start, resource_size(&res));
	if (!p->membase)
		return -ENOMEM;

	ret = of_property_read_u32(np, "clock-frequency", &prop_value);
	if (ret < 0) {
		dev_err(&pdev->dev, "module clock frequency not defined in dts(%d)\n", ret);
		return ret;
	}
	p->uartclk = prop_value;

	ret = of_property_read_u32(np, "uart-device-id", &prop_value);
	if (ret < 0)
		dev_warn(&pdev->dev, "dynamic assign port number(%d)\n", ret);
	p->line = prop_value;

	ret = of_property_read_u32(np, "reg-shift", &prop_value);
	if (ret < 0) {
		dev_err(&pdev->dev, "module reg-shift not defined in dts(%d)\n", ret);
		return ret;
	}
	p->regshift = prop_value;

	/* whether support high speed UART */
	mtk_up->high_speed =
	    of_property_read_bool(pdev->dev.of_node, "high-speed-uart");
	dev_info(&pdev->dev, "%ssupport high speed uart\n", mtk_up->high_speed ? "" : "not ");

	/* module clock sources */
	mtk_up->clk_num = devm_clk_bulk_get_all(&pdev->dev, &mtk_up->clks);
	if (mtk_up->clk_num < 0) {
		dev_err(&pdev->dev, "fail to get bulk clks(%d)\n", mtk_up->clk_num);
		return mtk_up->clk_num;
	}
	dev_info(&pdev->dev, "got %d clks\n", mtk_up->clk_num);

	if (mtk_up->high_speed) {
		struct clk_hw *clk_hw;
		unsigned int synth_reg_addr;

		/* high speed uart */

		/* need enable UART clock synthesizer */
		ret = of_property_read_u32(np, "uart-synth-reg", &synth_reg_addr);
		if (ret < 0) {
			dev_err(&pdev->dev, "synth_reg_addr not defined in dts(%d)\n", ret);
			mtk_up->high_speed = false;
			return 0;
		}

		/* UART clock synthesizer clock parent index */
		ret = of_property_read_u32(np, "uart-synth-clk-index", &prop_value);
		if (ret < 0) {
			dev_err(&pdev->dev, "synth_reg_addr not defined in dts(%d)\n", ret);
			mtk_up->high_speed = false;
			return 0;
		}

		/* get suspend/resume clock parent */
		clk_hw = __clk_get_hw(mtk_up->clks[0].clk);
		if (!clk_hw) {
			dev_err(&pdev->dev, "Can not get clk_hw of high speed uart clk0\n");
			mtk_up->high_speed = false;
			return 0;
		}

		mtk_up->resume_clk = clk_hw_get_parent_by_index(__clk_get_hw(mtk_up->clks[0].clk),
						 prop_value)->clk;
		mtk_up->suspend_clk = clk_hw_get_parent_by_index(__clk_get_hw(mtk_up->clks[0].clk),
						 0)->clk;
		if (!mtk_up->resume_clk || !mtk_up->suspend_clk) {
			dev_err(&pdev->dev, "Neither suspend nor resume clk parent found\n");
			mtk_up->high_speed = false;
			return 0;
		}

		mtk_up->synth_base = devm_ioremap(&pdev->dev, synth_reg_addr, UART_SYNTH_REG_SIZE);
		if (IS_ERR(mtk_up->synth_base)) {
			dev_err(&pdev->dev, "failed to map synthesizer ioresource (%ld)\n",
				PTR_ERR(mtk_up->synth_base));
			mtk_up->high_speed = false;
		}
	}

	return 0;
}

static int mtk8250_dtv_probe(struct platform_device *pdev)
{
	struct uart_8250_port uart = {};
	struct mtk8250_dtv_data *mtk_up;
	int err;
	int irq_no;
	u32 ori_console_verbose;
	bool ori_console_need_disable;

	/* get the interrupt */
	irq_no = platform_get_irq(pdev, 0);
	if (irq_no < 0) {
		dev_err(&pdev->dev, "no irq defined\n");
		return -EINVAL;
	}
	dev_info(&pdev->dev, "irq %d\n", irq_no);

	mtk_up = devm_kzalloc(&pdev->dev, sizeof(*mtk_up), GFP_KERNEL);
	if (!mtk_up)
		return -ENOMEM;

	err = mtk8250_dtv_probe_of(pdev, &uart.port, mtk_up);
	if (err < 0)
		return err;

	spin_lock_init(&uart.port.lock);
	uart.port.serial_in = mtk8250_dtv_mem_serial_in;
	uart.port.serial_out = mtk8250_dtv_mem_serial_out;
	uart.port.handle_irq = mtk8250_dtv_handle_irq;
	uart.port.set_termios = mtk8250_dtv_set_termios;
	uart.port.startup = mtk8250_dtv_startup;
	uart.port.shutdown = mtk8250_dtv_shutdown;
	uart.port.pm = mtk8250_dtv_do_pm;
	uart.port.irq = (unsigned int)irq_no;
	uart.port.type = PORT_16550A;
	uart.port.flags = UPF_FIXED_PORT | UPF_FIXED_TYPE | UPF_NO_THRE_TEST;
	uart.port.fifosize = MTK_UART_FIFO_SZ;
	uart.tx_loadsz = MTK_UART_FIFO_SZ;
	uart.capabilities = UART_CAP_AFE | UART_CAP_FIFO;
	uart.port.dev = &pdev->dev;
	uart.port.iotype = UPIO_MEM;
	uart.port.private_data = mtk_up;

	platform_set_drvdata(pdev, mtk_up);

	pm_runtime_enable(&pdev->dev);

	if (uart.port.line == 0) {
		// temporary enable console because 8250 initial sequence may write UART TXD/RXD
		ori_console_verbose = mtk_console_verbose;
		ori_console_need_disable = console_need_disable;
		mtk_console_verbose = 1;
		console_need_disable = false;
	}

	dev_info(&pdev->dev, "try register port %d\n", uart.port.line);
	mtk_up->switched_line = mtk_up->line = serial8250_register_8250_port(&uart);
	if (mtk_up->line < 0) {
		dev_err(&pdev->dev, "register 8250 port failed (%d)\n", mtk_up->line);

		if (uart.port.line == 0) {
			// restore console enable/disable status
			mtk_console_verbose = ori_console_verbose;
			console_need_disable = ori_console_need_disable;
		}

		pm_runtime_disable(&pdev->dev);

		return mtk_up->line;
	}

	dev_info(&pdev->dev, "success register to port %d type %d flags 0x%08X\n", mtk_up->line,
		 uart.port.type, uart.port.flags);

	if (uart.port.line == 0) {
		// restore console enable/disable status
		mtk_console_verbose = ori_console_verbose;
		console_need_disable = ori_console_need_disable;
	}

	mtk_up->port = serial8250_get_port(mtk_up->line);

	/*
	 * create sysfs group for proprietary control, if sysfs group creation failed, proprietary
	 * control function can not work but original UART still works.
	 */
	err = sysfs_create_group(&pdev->dev.kobj, &mtk8250_control_attr_group);
	if (err < 0)
		dev_err(&pdev->dev, "failed to create file group for auxiliary control. (%d)\n",
			err);

	return 0;
}

static int mtk8250_dtv_remove(struct platform_device *pdev)
{
	struct mtk8250_dtv_data *mtk_up = platform_get_drvdata(pdev);

	serial8250_unregister_port(mtk_up->line);

	pm_runtime_get_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);

	sysfs_remove_group(&pdev->dev.kobj, &mtk8250_control_attr_group);

	return 0;
}

static int __maybe_unused mtk8250_dtv_suspend(struct device *dev)
{
	struct mtk8250_dtv_data *mtk_up = dev_get_drvdata(dev);
	struct uart_port *port = &(serial8250_get_port(mtk_up->line)->port);
	struct circ_buf *xmit = &port->state->xmit;

	dev_info(dev, "%s\n", __func__);

	if (uart_console(port) && !console_suspend_enabled && !uart_circ_empty(xmit)) {
		int retry = CONSOLE_TTY_FLUSH_RETRY;

		while (!uart_circ_empty(xmit) && retry) {
			dev_info(dev, "buffer remain %ld\n", uart_circ_chars_pending(xmit));
			msleep(CONSOLE_TTY_FLUSH_PERIOD_MS);
			retry--;
		}

		if (!uart_circ_empty(xmit)) {
			dev_warn(dev, "clear buffer remain %ld\n", uart_circ_chars_pending(xmit));
			uart_circ_clear(xmit);
			port->ops->stop_tx(port);
		}
	}

	serial8250_suspend_port(mtk_up->line);

	return 0;
}

static int __maybe_unused mtk8250_dtv_resume(struct device *dev)
{
	struct mtk8250_dtv_data *mtk_up = dev_get_drvdata(dev);

	serial8250_resume_port(mtk_up->line);
	dev_info(dev, "%s\n", __func__);

	return 0;
}

static int __maybe_unused mtk8250_dtv_runtime_suspend(struct device *dev)
{
	struct mtk8250_dtv_data *mtk_up = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	/* disable all clks, include sw_en*/
	clk_bulk_disable_unprepare(mtk_up->clk_num, mtk_up->clks);

	if (mtk_up->high_speed) {
		int ret;

		/* high speed uart, 1st clk is clk mux, select clk mux to default parent */
		ret = clk_set_parent(mtk_up->clks[0].clk, mtk_up->suspend_clk);
		if (ret < 0)
			dev_err(dev, "set baudrate parent clk to default clk failed(%d)\n", ret);

		/* disable synthesizer */
		writew(~(UART_SYNTH_SW_RSTZ_MSK | UART_SYNTH_NF_EN_MSK),
		       mtk_up->synth_base + UART_SYNTH_REG_CTRL);
	}

	return 0;
}

static int __maybe_unused mtk8250_dtv_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct mtk8250_dtv_data *mtk_up = dev_get_drvdata(dev);

	if (mtk_up->high_speed) {
		/* high speed uart, 1st clk is clk mux, select clk mux to uart clk synth. */
		ret = clk_set_parent(mtk_up->clks[0].clk, mtk_up->resume_clk);
		if (ret < 0) {
			dev_err(dev, "set baudrate parent clk to synth. clk failed(%d)\n", ret);
			return ret;
		}

		/* enable synthesizer */
		writew(UART_SYNTH_SW_RSTZ_MSK | UART_SYNTH_NF_EN_MSK,
		       mtk_up->synth_base + UART_SYNTH_REG_CTRL);
	}

	/* enable all clks, include sw_en*/
	ret = clk_bulk_prepare_enable(mtk_up->clk_num, mtk_up->clks);
	if (ret < 0) {
		dev_err(dev, "bulk clk enable failed(%d)\n", ret);
		return ret;
	}

	dev_info(dev, "%s\n", __func__);

	return ret;
}

static const struct dev_pm_ops mtk8250_dtv_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk8250_dtv_suspend, mtk8250_dtv_resume)
	SET_RUNTIME_PM_OPS(mtk8250_dtv_runtime_suspend, mtk8250_dtv_runtime_resume, NULL)
};

static const struct of_device_id mtk8250_dtv_of_match[] = {
	{ .compatible = "mediatek_tv,ns16550" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, mtk8250_dtv_of_match);

static struct platform_driver mtk8250_dtv_platform_driver = {
	.driver = {
		.name = "mtk16550",
		.pm = &mtk8250_dtv_pm_ops,
		.of_match_table = mtk8250_dtv_of_match,
	},
	.probe = mtk8250_dtv_probe,
	.remove = mtk8250_dtv_remove,
};

#ifdef CONFIG_SERIAL_8250_CONSOLE
static int __init early_mtk8250_dtv_setup(struct earlycon_device *device, const char *options)
{
	return early_serial8250_setup(device, NULL);
}
OF_EARLYCON_DECLARE(mtk8250_dtv, "mediatek_tv,ns16550", early_mtk8250_dtv_setup);
#endif

/*
 *  UART module init function, include proprietary control
 */
static int __init mtk8250_dtv_init(void)
{
	int ret = 0;

	if (console_need_disable)
		// disable uart and set loglevel is 0
		console_silent();

	/*
	 * create sysfs group for proprietary control, if sysfs group creation failed, proprietary
	 * uart control function can not work but UART still works.
	 */
	uat_control_dev = platform_device_alloc("uart_control", -1);
	if (!uat_control_dev) {
		pr_err("Can not create platform device for uart control.\n");
		goto end_ctrl_dev;
	}

	ret = platform_device_add(uat_control_dev);
	if (ret) {
		pr_err("Can not add platform device for uart control.\n");
		goto put_ctrl_dev;
	}

	ret = sysfs_create_group(&uat_control_dev->dev.kobj, &mtk8250_control_attr_group);
	if (ret >= 0)
		goto end_ctrl_dev;

	pr_err("failed to create file group for uart control.(%d)\n", ret);
	platform_device_del(uat_control_dev);

put_ctrl_dev:
	platform_device_put(uat_control_dev);
	uat_control_dev = NULL;

end_ctrl_dev:
	ret = platform_driver_register(&mtk8250_dtv_platform_driver);
	if (ret) {
		pr_err("8250 platform driver register failed (%d)\n", ret);
		return ret;
	}

	return ret;
}
module_init(mtk8250_dtv_init);

static void __exit mtk8250_dtv_exit(void)
{
	platform_driver_unregister(&mtk8250_dtv_platform_driver);
	platform_device_del(uat_control_dev);
	platform_device_put(uat_control_dev);
	uat_control_dev = NULL;
}
module_exit(mtk8250_dtv_exit);

MODULE_AUTHOR("jefferry.yen@mediatek.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek dtv 8250 serial port driver");
