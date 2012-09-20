#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <asm/leds.h>
#include <mach/regs-gpio.h>

static void mini2440_leds_timer(void)
{
	unsigned long int gpbdat, tmp;
	
	gpbdat = ioread16(REG_GPIO(GPBDAT));
	tmp = ~(gpbdat & 0x1E0);
	gpbdat &= ~0x1E0;
	gpbdat |= 0x1E0;
	gpbdat &= tmp;
	iowrite16(gpbdat, REG_GPIO(GPBDAT));
}

static void mini2440_leds_timer_init(void)
{
	iowrite32(0x15400, REG_GPIO(GPBCON));
	iowrite16(0xC0, REG_GPIO(GPBDAT));
}

static void mini2440_leds_event(led_event_t event)
{
	unsigned long flags;
	
	local_irq_save(flags);
	switch(event)
	{
#ifdef CONFIG_LEDS_TIMER
	case led_timer:
		mini2440_leds_timer();
		break;
#endif
	default:
		break;
	}
	local_irq_restore(flags);
}

static int __init leds_init(void)
{
	leds_event = mini2440_leds_event;
	mini2440_leds_timer_init();
	return 0;
}
__initcall(leds_init);
