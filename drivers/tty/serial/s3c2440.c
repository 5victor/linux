#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/module.h>
#include <linux/serial_core.h>
#include <linux/termios.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <mach/map.h>
#include <mach/regs-uart.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clk.h>

MODULE_LICENSE("GPL");

#define REG_GPIO(x)	(S3C2440_VA_GPIO + (x))

#ifdef	DEBUG
#define	debug(format,arg...)	printk(format,##arg)
#else
#define	debug(format,arg...)
#endif

enum state { STATE_OPEN, STATE_CLOSE};

struct uart_info {
	enum state state;
	int irq_tx;
	int irq_rx;
};

static inline void *reg_uart(struct uart_port *port, int offset)
{
	return (void *)(port->mapbase + offset);
}

static irqreturn_t s3c2440_uart_int_rx(int irq, void *dev_id)
{
	int size;
	unsigned long ufstat;
	unsigned long uerstat;
	int ch, flag;
	struct uart_port *port = dev_id;
	struct uart_info *info = port->private_data;
	struct tty_struct *tty = port->state->port.tty;

	debug(KERN_INFO "%s\n", __func__);

	ufstat = ioread32(reg_uart(port, UFSTAT));
	size = UFSTAT_RXCNT(ufstat);

	while (size--) {
		port->icount.rx++;
		flag = TTY_NORMAL;
		ch = ioread8(reg_uart(port, URXH));
		uerstat = ioread8(reg_uart(port, UERSTAT));

		if (uerstat & UERSTAT_BREAK) {
			port->icount.brk++;
			uart_handle_break(port);
			debug(KERN_INFO "s3c2440 uart break char\n");
			goto ignore_char;
		}

		if (uerstat & UERSTAT_FRAME) {
			port->icount.frame++;
			flag = TTY_FRAME;
		}

		if (uerstat & UERSTAT_OVERRUN) {
			port->icount.overrun++;
			flag = TTY_OVERRUN;
		}

		if (uerstat & UERSTAT_PARITY) {
			port->icount.parity++;
			flag = TTY_PARITY;
		}

		if (uart_handle_sysrq_char(port, ch)) {
			debug(KERN_INFO "s3c2440 uart handle sysrq char");
			goto ignore_char;
		}
		
		debug(KERN_INFO "s3c2440_uart:insert char:%c uerstat 0x%x"
				"flag 0x%x\n", ch, uerstat, flag);
		uart_insert_char(port, uerstat, UERSTAT_OVERRUN, ch,
					flag);

	ignore_char:
		continue;
	}

	tty_flip_buffer_push(tty);

	return IRQ_HANDLED;
}

static int s3c2440_uart_startup(struct uart_port *port)
{
	struct uart_info *info = port->private_data;
	
	debug(KERN_INFO "%s\n", __func__);

	enable_irq(info->irq_rx);
	return 0;
}

static void s3c2440_uart_shutdown(struct uart_port *port)
{
	return;
}

static void s3c2440_uart_start_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	unsigned long ufstat;

	debug(KERN_INFO "s3c2440_uart:%s\n", __func__);

	if (port->x_char) {
		iowrite8(port->x_char, reg_uart(port, UTXH));
		port->x_char = 0;
	}

	while (!uart_circ_empty(xmit)) {
		ufstat = ioread32(reg_uart(port, UFSTAT));
		if (ufstat & (1 << 14))
			continue;

		iowrite8(xmit->buf[xmit->tail], reg_uart(port, UTXH));
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
	}
}

static void s3c2440_uart_stop_tx(struct uart_port *port)
{
	struct uart_info *info = port->private_data;
	dump_stack();
}

static void s3c2440_uart_stop_rx(struct uart_port *port)
{
	/* to handle sysrq when uart is not open like boot time */
	struct uart_info *info = port->private_data;

	disable_irq(info->irq_rx);
}

#ifdef CONFIG_CONSOLE_POLL
static int s3c2440_uart_poll_getchar(struct uart_port *port)
{
	unsigned long ufstat;
	do {
		ufstat = ioread32(reg_uart(port, UFSTAT));
		if (UFSTAT_RXCNT(ufstat) > 0)
			return ioread32(reg_uart(port, URXH));
	} while(1);
}
#endif

static void s3c2440_uart_poll_putchar(struct uart_port *port, unsigned char c)
{
	unsigned long ufstat;
	do {
		ufstat = ioread32(reg_uart(port, UFSTAT));
		if (!(ufstat & UFSTAT_TXFULL)) {
			iowrite32(c, reg_uart(port, UTXH));
			return;
		}
	} while(1);
}

static void s3c2440_uart_set_termios(struct uart_port *port,
				struct ktermios *new, struct ktermios *old)
{
	int baud;
	struct clk *pclk;
	unsigned int ubrdiv;
	int pclk_rate;
	unsigned int ulcon;
	
	pclk = clk_get(NULL, "pclk");
	pclk_rate = clk_get_rate(pclk);
	baud = uart_get_baud_rate(port, new, old, 0, 8 * 115200);
	debug(KERN_INFO "s3c2440_uart_baud=%d\n", baud);
	ubrdiv = (pclk_rate / (baud * 16)) - 1;
	debug(KERN_INFO "s3c2440 ubrdiv = %d\n", ubrdiv);
	iowrite16(ubrdiv, reg_uart(port, UBRDIV));

	ulcon = 0;
	if (new->c_cflag & CSTOPB) {
		ulcon |= ULCON_STOPBIT(1);
	}

	if (new->c_cflag & PARENB) {
		if (new->c_cflag & PARODD)
			ulcon |= ULCON_PARITY(4);
		else
			ulcon |= ULCON_PARITY(5);
	}

	switch(new->c_cflag & CSIZE) {
		case CS5:
			ulcon |= ULCON_WORDLEN(0);
			break;
		case CS6:
			ulcon |= ULCON_WORDLEN(1);
			break;
		case CS7:
			ulcon |= ULCON_WORDLEN(2);
			break;
		case CS8:
			ulcon |= ULCON_WORDLEN(3);
			break;
	};

	if (new->c_cflag & CRTSCTS) {
		iowrite32(1 << 4, reg_uart(port, UMCON));
	}

	iowrite32(ulcon, reg_uart(port, ULCON));
}

static void s3c2440_uart_config_port(struct uart_port *port, int flag)
{
	unsigned long clkcon = ioread32(REG_CLK(CLKCON));

	debug(KERN_INFO "s3c2440_uart:%s\n", __func__);

	clkcon |= 0x00000400 << port->line;
	iowrite32(clkcon, REG_CLK(CLKCON));
	iowrite32(UCON_CLOCKSEL(2) | UCON_TXMODE(1) | UCON_RXTIMEOUT(1) |
			UCON_RXMODE(1), reg_uart(port, UCON));
	iowrite8(UFCON_TXTRIGGER(0) | UFCON_RXTRIGGER(3) | UFCON_TXFIFORST |
			UFCON_RXFIFORST | UFCON_FIFOEN, reg_uart(port, UFCON));
	iowrite8(0x0, reg_uart(port, UMCON));

	port->type = PORT_S3C2440;
}

static void s3c2440_uart_break_ctl(struct uart_port *port, int ctl)
{
	unsigned long ucon;
	ucon = ioread32(reg_uart(port, UCON));
	ucon |= UCON_SENDBRK;
	iowrite32(ucon, reg_uart(port, UCON));
}

static void s3c2440_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{

}

static unsigned int s3c2440_uart_get_mctrl(struct uart_port *prot)
{
	return TIOCM_CAR | TIOCM_DSR;
}

static void s3c2440_uart_enable_ms(struct uart_port *port)
{

}

static unsigned int s3c2440_uart_tx_empty(struct uart_port *port)
{
	unsigned long utrstat = ioread32(reg_uart(port, UTRSTAT));

	return utrstat & (1 << 2);
}

static struct console s3c2440_console;

static struct uart_driver s3c2440_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= "s3c2440-uart",
	.dev_name	= "ttySAC",
	.major		= 204,
	.minor		= 64,
	.nr		= 3,
	.cons		= &s3c2440_console,
};

static struct uart_ops s3c2440_uart_ops = {
	.startup	= s3c2440_uart_startup,
	.shutdown	= s3c2440_uart_shutdown,
	.start_tx	= s3c2440_uart_start_tx,
	.stop_tx	= s3c2440_uart_stop_tx,
	.stop_rx	= s3c2440_uart_stop_rx,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char	= s3c2440_uart_poll_getchar,
	.poll_put_char	= s3c2440_uart_poll_putchar,
#endif
	.break_ctl	= s3c2440_uart_break_ctl,
	.set_termios	= s3c2440_uart_set_termios,
	.config_port	= s3c2440_uart_config_port,
	.set_mctrl	= s3c2440_uart_set_mctrl,
	.get_mctrl	= s3c2440_uart_get_mctrl,
	.enable_ms	= s3c2440_uart_enable_ms,
	.tx_empty	= s3c2440_uart_tx_empty,
};

static struct uart_port s3c2440_uart_port[] = {
	[0] = {
		.line		= 0,
		.iotype		= UPIO_MEM,
		.mapbase	= S3C2440_VA_UART0,
		.fifosize	= 32,
		.flags		= UPF_BOOT_AUTOCONF,
		.ops		= &s3c2440_uart_ops,
	},
	[1] = {
		.line		= 1,
		.iotype		= UPIO_MEM,
		.mapbase	= S3C2440_VA_UART1,
		.fifosize	= 32,
		.flags		= UPF_BOOT_AUTOCONF,
		.ops		= &s3c2440_uart_ops,
	},
	[2] = {
		.line		= 2,
		.iotype		= UPIO_MEM,
		.mapbase	= S3C2440_VA_UART2,
		.fifosize	=32,
		.flags		= UPF_BOOT_AUTOCONF,
		.ops		= &s3c2440_uart_ops,
	},
};

struct uart_info s3c2440_uart_info[] = {
	[0] = {
		.state = STATE_CLOSE,
	},
	[1] = {
		.state = STATE_CLOSE,
	},
	[2] = {
		.state = STATE_CLOSE,
	},
};

static void s3c2440_uart_gpio_init(void)
{
	unsigned long gphcon;
	
	gphcon = ioread32(REG_GPIO(GPHCON));
	gphcon &= ~0x0000FFFF;
	gphcon |= 0x0000AAAA;
	iowrite32(gphcon, REG_GPIO(GPHCON));
}

static int s3c2440_uart_int_init(void)
{
	int i, j;
	int ret;
	j = 0;

	for (i = IRQ_RXD0; i <= IRQ_RXD2; i += 3, j++) {
		ret = request_irq(i, s3c2440_uart_int_rx, 0, "rx irq",
				&s3c2440_uart_port[j]);
		if (ret) {
			printk(KERN_INFO "s3c2440 uart request_irq error:%d\n",
					ret);
			goto request_irq_error;
		}
		s3c2440_uart_info[j].irq_rx = i;
	}

	return ret;

request_irq_error:
	for (i -= 3, j--; i >= IRQ_RXD0; i -= 3, j--) {
		free_irq(i, &s3c2440_uart_port[j]);
	}
	
	return ret;
}

static int s3c2440_uart_driver_init(void)
{
	int ret;
	int i;

	s3c2440_uart_gpio_init();
	
	ret = s3c2440_uart_int_init();

	if (ret) {
		debug(KERN_INFO "s3c2440 uart request irq error %d\n", ret);
		goto error_int_rx_init;
	}

	ret = uart_register_driver(&s3c2440_uart_driver);
	if (ret)
		goto error_reg_drv;

	debug(KERN_INFO "s3c2440_uart: register uart_driver success\n");

	for (i = 0; i < ARRAY_SIZE(s3c2440_uart_port); i++) {
		ret = uart_add_one_port(&s3c2440_uart_driver,
				&s3c2440_uart_port[i]);
		s3c2440_uart_port[i].private_data = &s3c2440_uart_info[i];

		if (ret) {
			printk(KERN_INFO "s3c2440_uart: add uart port fail\n");
			for (i--; i >= 0; i--) {
				uart_remove_one_port(&s3c2440_uart_driver,
					&s3c2440_uart_port[i]);
			}
			goto error_reg_port;
		}
	}

	return ret;

error_reg_port:
	uart_unregister_driver(&s3c2440_uart_driver);
error_reg_drv:
error_int_rx_init:
	return ret;
}

static void s3c2440_uart_driver_exit(void)
{
	int i;
	uart_unregister_driver(&s3c2440_uart_driver);
	for (i = 0; i < ARRAY_SIZE(s3c2440_uart_port); i++) {
		uart_remove_one_port(&s3c2440_uart_driver,
				&s3c2440_uart_port[i]);
	}
}

module_init(s3c2440_uart_driver_init);
module_exit(s3c2440_uart_driver_exit);

static void s3c2440_console_write(struct console *con, const char *s,
		unsigned count)
{
	struct uart_port *port = &s3c2440_uart_port[con->index];
	uart_console_write(port, s, count, 
		(void (*)(struct uart_port *, int))s3c2440_uart_poll_putchar);
}

static int s3c2440_console_setup(struct console *con, char *option)
{
	struct uart_port *port = &s3c2440_uart_port[con->index];
	int baud = 9600, parity = 'n', bits = 8, flow = 0;

	s3c2440_uart_gpio_init();
	s3c2440_uart_config_port(port, 0);
	uart_parse_options(option, &baud, &parity, &bits, &flow);
	uart_set_options(port, con, baud, parity, bits, flow);
	return 0;
}

static struct console s3c2440_console = {
	.name	= "ttySAC",
	.write	= s3c2440_console_write,
	.device = uart_console_device,
	.index	= 0,
	.flags	= CON_PRINTBUFFER,
	.setup	= s3c2440_console_setup,
	.data	= &s3c2440_uart_driver,
};

static int s3c2440_console_init(void)
{
	register_console(&s3c2440_console);
	return 0;
}

console_initcall(s3c2440_console_init);
