#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/mach/map.h>
#include <mach/map.h>
#include <mach/regs-clk.h>

#define CLOCK_TICK_RATE	12000000

#define REG_CLK(x)	(S3C2440_VA_CLK_PW + (x))

#ifdef DEBUG
#define debug(format,args...)	printk(format,##args)
#else
#define debug(format,args...)
#endif

struct clk {
	char	*name;
	int	id;
	int	rate;
	struct clk *parent;
	void (*setup)(void);
	int (*get_rate)(struct clk *clk);
};

static void s3c2440_fclk_setup(void)
{
	unsigned long mpllcon = (0x7f << 12) | (2 << 4) | 0x1;
	unsigned long clkslow = ioread32(REG_CLK(CLKSLOW));
	iowrite32(mpllcon, REG_CLK(MPLLCON));
	clkslow = clkslow & ~((1 << 5) | (1 << 4));
	iowrite32(clkslow, REG_CLK(CLKSLOW));
	__asm__("mrc p15, 0, r0, c1, c0, 0\n"
		"orr r0, r0, #0xC0000000\n"
		"mcr p15, 0, r0, c1, c0, 0\n":::"r0");
}

static int s3c2440_fclk_get_rate(struct clk *clk)
{
	unsigned long mpllcon = ioread32(REG_CLK(MPLLCON));
	int m, p, s;
	m = ((mpllcon >> 12) & 0xFF) + 8;
	p = ((mpllcon >> 4) & 0x3F) + 2;
	s = mpllcon & 0x3;
	return 2 * m * (CLOCK_TICK_RATE / (p << s));
}

static void s3c2440_hclk_setup(void)
{
	unsigned long clkdivn = ioread32(REG_CLK(CLKDIVN));
	unsigned long camdivn = ioread32(REG_CLK(CAMDIVN));
	clkdivn &= ~(3 << 1);
	clkdivn |= 2 << 1;
	camdivn &= ~(1 << 9);
	iowrite32(clkdivn, REG_CLK(CLKDIVN));
	iowrite32(camdivn, REG_CLK(CAMDIVN));
}

static int s3c2440_hclk_get_rate(struct clk *clk)
{
	struct clk *fclk = clk_get_parent(clk);
	int fclk_rate = clk_get_rate(fclk);
	unsigned long clkdivn = ioread32(REG_CLK(CLKDIVN));
	unsigned long camdivn = ioread32(REG_CLK(CAMDIVN));
	int rate;
	debug("fclk_rate:%d\n", fclk_rate);
	switch ((clkdivn >> 1) & 0x3) {
	case 0:
		rate = fclk_rate;
		break;
	case 1:
		rate = fclk_rate / 2;
		break;
	case 2:
		rate = (camdivn & (1 << 9)) ? fclk_rate / 8 : fclk_rate / 4;
		break;
	case 3:
		rate = (camdivn & (1 << 8)) ? fclk_rate / 6 : fclk_rate / 3;
		break;
	}
	return rate;
}

static void s3c2440_pclk_setup(void)
{
	unsigned long clkdivn = ioread32(REG_CLK(CLKDIVN));
	iowrite32(clkdivn | 1, REG_CLK(CLKDIVN));
}

static int s3c2440_pclk_get_rate(struct clk *clk)
{
	struct clk *hclk = clk_get_parent(clk);
	int hclk_rate = clk_get_rate(hclk);
	unsigned long clkdivn = ioread32(REG_CLK(CLKDIVN));
	debug("hclk_rate:%d\n", hclk_rate);
	return (clkdivn & 0x1) ? hclk_rate / 2 : hclk_rate;
}

static struct clk s3c2440_clk[] = {
	{
		.name	= "fclk",
		.id	= -1,
		.rate	= -1,
		.setup	= s3c2440_fclk_setup,
		.get_rate	= s3c2440_fclk_get_rate,
	}, {
		.name	= "hclk",
		.id	= -1,
		.rate	= -1,
		.parent	= &s3c2440_clk[0],
		.setup	= s3c2440_hclk_setup,
		.get_rate	= s3c2440_hclk_get_rate,
	}, {
		.name	= "pclk",
		.id	= -1,
		.rate	= -1,
		.parent	= &s3c2440_clk[1],
		.setup	= s3c2440_pclk_setup,
		.get_rate	= s3c2440_pclk_get_rate,
	},
};


int clk_enable(struct clk *clk)
{
	unsigned int clkcon = ioread32(REG_CLK(CLKCON));
	iowrite32(clkcon | (1 << clk->id), REG_CLK(CLKCON));
	return 0;
}

void clk_disable(struct clk *clk)
{
	unsigned int clkcon = ioread32(REG_CLK(CLKCON));
	iowrite32(clkcon & ~(1 << clk->id), REG_CLK(CLKCON));
}

struct clk *clk_get(struct device *dev, const char *id)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(s3c2440_clk); i++) {
		if (!strcmp(s3c2440_clk[i].name, id))
			return &s3c2440_clk[i];
	}
	return (void *)-1;
}

unsigned long clk_get_rate(struct clk *clk)
{
	if (clk->rate < 0)
		return clk->get_rate(clk);
	else
		return clk->rate;
}

struct clk *clk_get_parent(struct clk *clk)
{
	return clk->parent;
}

static void clk_setup(struct clk *clk)
{
	clk->setup();
}

void s3c2440_clk_init(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(s3c2440_clk); i++) {
		clk_setup(&s3c2440_clk[i]);
	}
}
