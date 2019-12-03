#include <circle/synchronize.h>

static void vfpinit (void)
{
	// Coprocessor Access Control Register
	unsigned nCACR;
	__asm volatile ("mrc p15, 0, %0, c1, c0, 2" : "=r" (nCACR));
	nCACR |= 3 << 20;	// cp10 (single precision)
	nCACR |= 3 << 22;	// cp11 (double precision)
	__asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r" (nCACR));
	InstructionMemBarrier ();

#define VFP_FPEXC_EN	(1 << 30)
	__asm volatile ("fmxr fpexc, %0" : : "r" (VFP_FPEXC_EN));

#define VFP_FPSCR_DN	(1 << 25)	// enable Default NaN mode
	__asm volatile ("fmxr fpscr, %0" : : "r" (VFP_FPSCR_DN));
}

void sysinit (void)
{
	EnableFIQs ();		// go to IRQ_LEVEL, EnterCritical() will not work otherwise

	vfpinit ();

	// clear BSS
	extern unsigned char __bss_start;
	extern unsigned char _end;
	for (unsigned char *pBSS = &__bss_start; pBSS < &_end; pBSS++)
	{
		*pBSS = 0;
	}

	extern int main (void);
	main ();
	while (1) { }
}
