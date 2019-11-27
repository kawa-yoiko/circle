#include <circle/sysconfig.h>

/*
 * Helper macro to enter SVC mode cleanly and mask interrupts. reg is
 * a scratch register for the macro to overwrite.
 *
 * This macro is intended for forcing the CPU into SVC mode at boot time.
 * you cannot return to the original mode.
 */
	.macro safe_svcmode_maskall reg:req

	mrs	\reg , cpsr
	eor	\reg, \reg, #0x1A		/* test for HYP mode */
	tst	\reg, #0x1F
	bic	\reg , \reg , #0x1F		/* clear mode bits */
	orr	\reg , \reg , #0xC0 | 0x13	/* mask IRQ/FIQ bits and set SVC mode */
	bne	1f				/* branch if not HYP mode */
	orr	\reg, \reg, #0x100		/* mask Abort bit */
	adr	lr, 2f
	msr	spsr_cxsf, \reg
	.word	0xE12EF30E			/* msr ELR_hyp, lr */
	.word	0xE160006E			/* eret */
1:	msr	cpsr_c, \reg
2:

	.endm

	.sect	.init

	.globl	_start
_start:
#ifndef USE_RPI_STUB_AT
	safe_svcmode_maskall r0

	mov	r0, #0
	mcr	p15, 0, r0, c12, c0, 0		/* reset VBAR (if changed by u-boot) */
#endif
	cps	#0x11				/* set fiq mode */
	ldr	sp, =MEM_FIQ_STACK
	cps	#0x12				/* set irq mode */
	ldr	sp, =MEM_IRQ_STACK
	cps	#0x17				/* set abort mode */
	ldr	sp, =MEM_ABORT_STACK
	cps	#0x1B				/* set "undefined" mode */
	ldr	sp, =MEM_ABORT_STACK
	cps	#0x1F				/* set system mode */
	ldr	sp, =MEM_KERNEL_STACK
	b	sysinit
