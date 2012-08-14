
#include <mach/map.h>
#include <mach/regs-int.h>

	.macro disable_fiq

	.endm

	.macro get_irqnr_preamble, base, tmp
		ldr \base, =S3C2440_VA_INT_CTRL
	.endm

	.macro get_irqnr_and_base, irqnr, irqstat, base, tmp
		ldr \irqnr, [\base, #INTOFFSET]
		tst \base, \base	@clear Z flag
	.endm

	.macro arch_ret_to_user, tmp1, tmp2

	.endm
