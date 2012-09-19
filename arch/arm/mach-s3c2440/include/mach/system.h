#include <asm/proc-fns.h>

static void arch_reset(char mode, const char *cmd)
{
	panic("not impletion");
}

static void arch_idle(void)
{
	cpu_do_idle();
}
