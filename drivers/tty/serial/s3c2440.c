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

static inline void *reg_uart(struct uart_port *port, int offset)
{
	return (void *)(port->mapbase + offset);
}

static int s3c2440_uart_startup(struct uart_port *port)
{
	return 0;
}

static void s3c2440_uart_shutdown(struct uart_port *port)
{

}

static void s3c2440_uart_start_tx(struct uart_port *port)
{

}

static void s3c2440_uart_stop_tx(struct uart_port *port)
{

}

static void s3c2440_uart_stop_rx(struct uart_port *port)
{

}

#ifdef CONFIG_CONSOLE_POLL
static int s3c2440_uart_poll_getchar(struct uart_port *port)
{
	unsigned long ufstat;
	do {
		ufstat = ioread32(reg_uart(port, UFSTAT));
		if ((ufstat & 0x3F) > 0)
			return ioread32(reg_uart(port, URXH));
	} while(1);
}
#endif

static void s3c2440_uart_poll_putchar(struct uart_port *port, unsigned char *c)
{
	unsigned long ufstat;
	do {
		ufstat = ioread32(reg_uart(port, UFSTAT));
		if (!(ufstat & (1 << 14))) {
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
	debug("s3c2440_uart_baud=%d\n", baud);
	ubrdiv = (pclk_rate / (baud * 16)) - 1;
	debug("s3c2440 ubrdiv = %d\n", ubrdiv);
	iowrite16(ubrdiv, reg_uart(port, UBRDIV));

	ulcon = 0;
	if (new->c_cflag & CSTOPB) {
		ulcon |= 1 << 2;
	}

	if (new->c_cflag & PARENB) {
		if (new->c_cflag & PARODD)
			ulcon |= 4 << 3;
		else
			ulcon |= 5 << 3;
	}

	switch(new->c_cflag & CSIZE) {
		case CS5:
			ulcon |= 0;
			break;
		case CS6:
			ulcon |= 1;
			break;
		case CS7:
			ulcon |= 2;
			break;
		case CS8:
			ulcon |= 3;
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
	clkcon |= 0x00000400 << port->line;
	iowrite32(clkcon, REG_CLK(CLKCON));
	iowrite32((2 << 10) | (1 << 2) | 1, reg_uart(port, UCON));
	iowrite8((3 << 6) | (3 << 4) | 7, reg_uart(port, UFCON));
	iowrite8(0x0, reg_uart(port, UMCON));
}

static void s3c2440_uart_break_ctl(struct uart_port *port, int ctl)
{
	unsigned long ulcon;
	ulcon = ioread32(reg_uart(port, ULCON));
	ulcon |= 1 << 4;
	iowrite32(ulcon, reg_uart(port, ULCON));
}

static struct uart_driver s3c2440_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= "s3c2440-uart",
	.dev_name	= "ttySAC",
	.major		= 4,
	.minor		= 64,
	.nr		= 3,
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
};

static struct uart_port s3c2440_uart_port[] = {
	[0] = {
		.line		= 0,
		.mapbase	= S3C2440_VA_UART0,
		.fifosize	= 32,
		.ops		= &s3c2440_uart_ops,
	},
	[1] = {
		.line		= 1,
		.mapbase	= S3C2440_VA_UART1,
		.fifosize	= 32,
		.ops		= &s3c2440_uart_ops,
	},
	[2] = {
		.line		= 2,
		.mapbase	= S3C2440_VA_UART2,
		.fifosize	=32,
		.ops		= &s3c2440_uart_ops,
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

static int s3c2440_uart_driver_init(void)
{
	int ret;
	int i;

	s3c2440_uart_gpio_init();

	ret = uart_register_driver(&s3c2440_uart_driver);
	if (ret < 0)
		goto error_reg_drv;
	
	for (i = 0; i < ARRAY_SIZE(s3c2440_uart_port); i++) {
		ret = uart_add_one_port(&s3c2440_uart_driver,
				&s3c2440_uart_port[i]);
		if (ret < 0) {
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
