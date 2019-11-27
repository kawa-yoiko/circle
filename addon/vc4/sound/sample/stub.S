#include <circle/sysconfig.h>

.section .init
.global _start
_start:
    cps     #0x11                /* set fiq mode */
    ldr     sp, =MEM_FIQ_STACK
    cps     #0x12                /* set irq mode */
    ldr     sp, =MEM_IRQ_STACK
    cps     #0x17                /* set abort mode */
    ldr     sp, =MEM_ABORT_STACK
    cps     #0x1B                /* set "undefined" mode */
    ldr     sp, =MEM_ABORT_STACK
    cps     #0x1F                /* set system mode */
    ldr     sp, =MEM_KERNEL_STACK
    b	    sysinit

.global qwq_irq_stub
qwq_irq_stub:
    sub		lr, lr, #4            /* lr: return address */
    stmfd	sp!, {r0-r3, r12, lr}        /* save r0-r3, r12 and return address */
    bl		_int_irq
    ldmfd   sp!, {r0-r3, r12, pc}^        /* restore registers and return */

.global EnableMMU
EnableMMU:
/*
	mrc     15, 0, r3, c1, c0, 1
	orr     r3, r3, #64, 0
	mcr     15, 0, r3, c1, c0, 1
	mrc     15, 0, r3, c0, c0, 3
	mov     r3, #0, 0
	mcr     15, 0, r3, c2, c0, 2
	mcr     15, 0, r0, c2, c0, 0
	mov     r2, #1, 0
	mcr     15, 0, r2, c3, c0, 0
	mcr     15, 0, r3, c7, c6, 0
	mcr     15, 0, r3, c7, c10, 4
	mcr     15, 0, r3, c7, c5, 0
	mcr     15, 0, r3, c7, c5, 6
	mcr     15, 0, r3, c8, c7, 0
	mcr     15, 0, r3, c7, c10, 4
	mcr     15, 0, r3, c7, c5, 4
	mrc     15, 0, r2, c1, c0, 0
	ldr     r3, =#0x00c01805
	bic     r2, r2, #2, 0
	orr     r3, r2, r3
	mcr     15, 0, r3, c1, c0, 0
	bx      lr
*/
    mov     r1, #0
    # Invalidate d/i/unified caches (ARM ARM p. B6-21)
    mcr     p15, 0, r1, c7, c7, 0
    # Invalidate d/i/unified TLBs (ARM ARM p. B4-45)
    mcr     p15, 0, r1, c8, c7, 0
    # Tell the coprocessor about the table address (ARM ARM p. B4-41/B5-18)
    orr     r0, #1
    mcr     p15, 0, r0, c2, c0, 0
    mcr     p15, 0, r0, c2, c0, 1
    # Set domain control access to Manager (ARM ARM p. B4-10/B5-18)
    # Also http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360f/CHDGIJFB.html
    mov     r1, #0xf
    mcr     p15, 0, r1, c3, c0, 0
    # Turn on MMU, with i/d caches (ARM ARM p. B3-12/B4-40/B5-18/B6-18)
    mrc     p15, 0, r1, c1, c0, 0
    orr     r1, r1, #0xd    // MMU & d-cache (B3-12)
    orr     r1, r1, #0x1f00 // i-cache & branch prediction (B3-12)
    mcr     p15, 0, r1, c1, c0, 0
    bx      lr
