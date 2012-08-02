#include <linux/irq.h>
#include <asm/io.h>
#include <mach/map.h>
#include <mach/regs-int.h>

static void s3c2440_init_reg(void)
{
	iowrite16(0xFFFF, REG_INT(INTSUBMSK));
	iowrite16(0xFFFF, REG_INT(SUBSRCPND));
	iowrite32(0xFFFFFFFF, REG_INT(SRCPND));
	iowrite32(0XFFFFFFFF, REG_INT(INTPND));
	iowrite32(0xFFFFFFFF, REG_INT(INTMSK));
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

static struct irq_chip s3c2440_irq_chip = {
	.irq_mask	= s3c2440_irq_mask,
	.irq_unmask	= s3c2440_irq_unmask,
	.irq_ack	= s3c2440_irq_ack,
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

void s3c2440_init_irq(void)
{
	int i;
	s3c2440_init_reg();
	for (i = 0; i <= IRQ_ADC; i++) {
		switch(i) {
		case IRQ_EINT0:
		case IRQ_EINT1:
		case IRQ_EINT2:
			irq_set_chip_and_handler(i,
				&s3c2440_irq_chip, handle_level_irq);
			break;
		default:
			irq_set_chip_and_handler(i,
				&s3c2440_irq_chip, handle_edge_irq);
		}
	}
	irq_set_chained_handler(IRQ_UART0, s3c2440_uart_demux);
	irq_set_chained_handler(IRQ_UART1, s3c2440_uart_demux);
	irq_set_chained_handler(IRQ_UART2, s3c2440_uart_demux);

	for (i = IRQ_RXD0; i < IRQ_TC; i++) {
		irq_set_chip_and_handler(i, &s3c2440_uart_chip,
				handle_level_irq);
	}
}
