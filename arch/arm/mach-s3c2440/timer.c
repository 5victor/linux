#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <asm/param.h>
#include <asm/io.h>
#include <asm/mach/time.h>
#include <mach/map.h>
#include <mach/regs-timer.h>
#include <mach/regs-int.h>

irqreturn_t s3c2440_timer_interrupt(int irq, void *dev_id)
{
	timer_tick();
	return IRQ_HANDLED;
}

static struct irqaction s3c2440_timer_irq = {
	.name = "S3C2440 TIMER TICK",
	.flags = IRQF_TIMER | IRQF_IRQPOLL,
	.handler = s3c2440_timer_interrupt,
};

static void s3c2440_timer_setup(void)
{
	int pclk_rate;
	unsigned long tcfg0, tcfg1;
	unsigned int tcnt;
	unsigned long intmsk;
	struct clk *pclk;

	intmsk = ioread32(REG_INT(INTMSK));
	intmsk &= 1 << 15;
	iowrite32(intmsk, REG_INT(INTMSK));

	pclk = clk_get(NULL, "pclk");
	if (IS_ERR(pclk))
		panic("setup timer fail");
	pclk_rate = clk_get_rate(pclk);
	
	tcfg0 = ioread32(REG_TIMER(TCFG0));
	tcfg1 = ioread32(REG_TIMER(TCFG1));

	tcfg1 &= ~(0xF << 16);
	tcfg1 |= (2 << 16);
	tcfg0 = 10 << 8;
	iowrite32(tcfg0, REG_TIMER(TCFG0));
	iowrite32(tcfg1, REG_TIMER(TCFG1));

	tcnt = pclk_rate / 8 / 10;
	tcnt /= HZ;
	iowrite16(tcnt, REG_TIMER(TCNTB4));

	iowrite32((1 << 22) | (1 << 20), REG_TIMER(TCON));
}

static void s3c2440_timer_init(void)
{
	s3c2440_timer_setup();
	setup_irq(IRQ_TIMER4, &s3c2440_timer_irq);
}

struct sys_timer s3c2440_timer = {
	.init = s3c2440_timer_init,
};

