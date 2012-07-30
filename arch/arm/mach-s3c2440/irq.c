#include <asm/io.h>
#include <mach/map.h>
#include <mach/regs-irq.h>

#define REG_INT(x)	(S3C2440_VA_INT_CTRL + (x))

static void s3c2440_init_reg(void)
{
	iowrite16(0x0, REG_INT(SUBINTMSK));
	iowrite16(0xFFFF, REG_INT(SUBSRCPND));
	iowrite32(0xFFFFFFFF, REG_INT(SRCPND));
	iowrite32(0XFFFFFFFF, REG_INT(INTPND));
	iowrite32(0x0, REG_INT(INTMSK));
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
	.mask	= s3c2440_irq_mask,
	.unmask	= s3c240_irq_unmask,
	.ack	= s3c2440_irq_ack,
};

void s3c2440_init_irq(void)
{
	s3c2440_init_reg();
	for (int i = 0; i <= IRQ_ADC; i++) {
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
