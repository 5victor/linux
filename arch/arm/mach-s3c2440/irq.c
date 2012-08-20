#include <linux/irq.h>
#include <linux/module.h>
#include <asm/io.h>
#include <mach/map.h>
#include <mach/regs-int.h>
#include <mach/regs-gpio.h>

static void s3c2440_init_reg(void)
{
	iowrite16(0xFFFF, REG_INT(INTSUBMSK));
	iowrite16(0xFFFF, REG_INT(SUBSRCPND));
	iowrite32(0xFFFFFFFF, REG_INT(SRCPND));
	iowrite32(0XFFFFFFFF, REG_INT(INTPND));
	iowrite32(0xFFFFFFFF, REG_INT(INTMSK));
	iowrite32(0xFFFFFFFF, REG_GPIO(EINTMASK));
	iowrite32(0xFFFFFFFF, REG_GPIO(EINTPEND));
}

static void s3c2440_irq_mask(struct irq_data *data)
{
	unsigned int intmsk = ioread32(REG_INT(INTMSK));
	iowrite32(intmsk | (1 << data->irq), REG_INT(INTMSK));
}

static void s3c2440_irq_unmask(struct irq_data *data)
{
	unsigned int intmsk = ioread32(REG_INT(INTMSK));
	iowrite32(intmsk & ~(1 << data->irq), REG_INT(INTMSK));
}

static void s3c2440_irq_ack(struct irq_data *data)
{
	iowrite32(1 << data->irq, REG_INT(SRCPND));
	iowrite32(1 << data->irq, REG_INT(INTPND));
}

static void s3c2440_irq_disable(struct irq_data *data)
{
	s3c2440_irq_mask(data);
	/* I don't known what irq disable need do clear INTPND? */
}

static struct irq_chip s3c2440_irq_chip = {
	.irq_mask	= s3c2440_irq_mask,
	.irq_unmask	= s3c2440_irq_unmask,
	.irq_ack	= s3c2440_irq_ack,
	.irq_disable	= s3c2440_irq_disable,
};

static void s3c2440_uart_demux(unsigned int irq, struct irq_desc *desc)
{
	unsigned long subsrcpnd;
	int offset;
	int i;

	subsrcpnd = ioread32(REG_INT(SUBSRCPND));
	offset = irq - IRQ_UART0;
	offset *= 3;

	for (i = 0; i < 3; i++) {
		if (subsrcpnd & (1 << (offset + i)))
			generic_handle_irq(IRQ_RXD0 + offset + i);
	}

}

static void s3c2440_uart_mask(struct irq_data *data)
{
	unsigned long intsubmsk;
	int offset = data->irq - IRQ_RXD0;

	intsubmsk = ioread32(REG_INT(INTSUBMSK));
	intsubmsk |= 1 << offset;
	iowrite32(intsubmsk, REG_INT(INTSUBMSK));
}

static void s3c2440_uart_unmask(struct irq_data *data)
{
	unsigned long intsubmsk;
	int offset = data->irq - IRQ_RXD0;

	intsubmsk = ioread32(REG_INT(INTSUBMSK));
	intsubmsk &= ~(1 << offset);
	iowrite32(intsubmsk, REG_INT(INTSUBMSK));
}

static void s3c2440_uart_ack(struct irq_data *data)
{
	int offset = data->irq - IRQ_RXD0;
	iowrite32(1 << offset, REG_INT(SUBSRCPND));
}

static struct irq_chip s3c2440_uart_chip = {
	.irq_mask	= s3c2440_uart_mask,
	.irq_unmask	= s3c2440_uart_unmask,
	.irq_ack	= s3c2440_uart_ack,
};

static void s3c2440_eint_demux(unsigned int irq, struct irq_desc *desc)
{
	int i;
	unsigned long eintpend = ioread32(REG_GPIO(EINTPEND));
	unsigned long eintmask = ioread32(REG_GPIO(EINTMASK));
	eintpend &= eintmask;

	for (i = 4; i < 24; i++) {
		if (eintpend & (1 << i)) {
			generic_handle_irq(IRQ_EINT4 + i - 4);
			break;
		}
	}
}

static void s3c2440_eint_mask(struct irq_data *data)
{
	int eintmask = ioread32(REG_GPIO(EINTMASK));
	eintmask |= 1 << (data->irq - IRQ_EINT4 + 4);
	iowrite32(eintmask, REG_GPIO(EINTMASK));
}

static void s3c2440_eint_unmask(struct irq_data *data)
{
	int eintmask = ioread32(REG_GPIO(EINTMASK));
	eintmask &= ~(1 << (data->irq - IRQ_EINT4 + 4));
	iowrite32(eintmask, REG_GPIO(EINTMASK));

}

static void s3c2440_eint_ack(struct irq_data *data)
{
	iowrite32(1 << (data->irq - IRQ_EINT4 + 4), REG_GPIO(EINTPEND));
}

static void s3c2440_eint_enable(struct irq_data *data)
{
	unsigned long gpcon;

	if (data->irq < IRQ_EINT8) {
		gpcon = ioread32(REG_GPIO(GPFCON));
		gpcon &= ~(3 << 2 * (data->irq - IRQ_EINT4));
		gpcon |= 2 << 2 * (data->irq - IRQ_EINT4);
		iowrite32(gpcon, REG_GPIO(GPFCON));
	} else {
		gpcon = ioread32(REG_GPIO(GPGCON));
		gpcon &= ~(3 << 2 * (data->irq - IRQ_EINT8));
		gpcon |= 2 << 2 * (data->irq - IRQ_EINT8);
		iowrite32(gpcon, REG_GPIO(GPGCON));
	}
	s3c2440_eint_unmask(data);
}

static void s3c2440_eint_disable(struct irq_data *data)
{
	unsigned long gpcon;

	if (data->irq < IRQ_EINT8) {
		gpcon = ioread32(REG_GPIO(GPFCON));
		gpcon &= ~(3 << 2 * (data->irq - IRQ_EINT4));
		iowrite32(gpcon, REG_GPIO(GPFCON));
	} else {
		gpcon = ioread32(REG_GPIO(GPGCON));
		gpcon &= ~(3 << 2 * (data->irq - IRQ_EINT8));
		iowrite32(gpcon, REG_GPIO(GPGCON));
	}
	s3c2440_eint_mask(data);
}

static struct irq_chip s3c2440_eint_chip = {
	.irq_enable	= s3c2440_eint_enable,
	.irq_disable	= s3c2440_eint_disable,
	.irq_mask	= s3c2440_eint_mask,
	.irq_unmask	= s3c2440_eint_unmask,
	.irq_ack	= s3c2440_eint_ack,
};

void s3c2440_init_irq(void)
{
	int i;
	s3c2440_init_reg();
	for (i = 0; i <= IRQ_ADC; i++) {
		switch(i) {
		case IRQ_UART0:
		case IRQ_UART1:
		case IRQ_UART2:
			irq_set_chip_and_handler(i,
					&s3c2440_irq_chip, handle_level_irq);
			break;
		
		case IRQ_EINT0:
		case IRQ_EINT1:
		case IRQ_EINT2:
			irq_set_chip_and_handler(i,
				&s3c2440_irq_chip, handle_level_irq);
			set_irq_flags(i, IRQF_VALID);
			break;
		default:
			irq_set_chip_and_handler(i,
				&s3c2440_irq_chip, handle_edge_irq);
			set_irq_flags(i, IRQF_VALID | IRQF_NOAUTOEN);
		}
	}
	irq_set_chained_handler(IRQ_UART0, s3c2440_uart_demux);
	irq_set_chained_handler(IRQ_UART1, s3c2440_uart_demux);
	irq_set_chained_handler(IRQ_UART2, s3c2440_uart_demux);

	for (i = IRQ_RXD0; i < IRQ_TC; i++) {
		irq_set_chip_and_handler(i, &s3c2440_uart_chip,
				handle_level_irq);
		set_irq_flags(i, IRQF_VALID | IRQF_NOAUTOEN);
	}

	irq_set_chained_handler(IRQ_EINT4_7, s3c2440_eint_demux);
	irq_set_chained_handler(IRQ_EINT8_23, s3c2440_eint_demux);

	for (i = IRQ_EINT4; i <= IRQ_EINT23; i++) {
		irq_set_chip_and_handler(i, &s3c2440_eint_chip,
				handle_level_irq);
		set_irq_flags(i, IRQF_VALID | IRQF_NOAUTOEN);
	}
}

void s3c2440_eint_trigger(unsigned int irq, enum eint_trigger trigger)
{
	int reg_offset = (irq - IRQ_EINT4) / 8;
	int bit_offset = 4 * ((irq - IRQ_EINT4) % 8);
	int reg = REG_GPIO(EXTINT0) + 4 * reg_offset;

	int extint = ioread32(reg);
	
	extint &= ~(7 << bit_offset);
	extint |= trigger << bit_offset;
	iowrite32(extint, reg);
}

EXPORT_SYMBOL(s3c2440_eint_trigger);

