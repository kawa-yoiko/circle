.global qwq_irq_stub
qwq_irq_stub:
    sub		lr, lr, #4            /* lr: return address */
    stmfd	sp!, {r0-r3, r12, lr}        /* save r0-r3, r12 and return address */
    bl		InterruptHandler
    ldmfd   sp!, {r0-r3, r12, pc}^        /* restore registers and return */

