#include "common.h"
#include <stddef.h>

void send_mail(uint32_t data, uint8_t channel)
{
    DMB(); DSB();
    while ((*MAIL0_STATUS) & (1u << 31)) { }
    *MAIL0_WRITE = (data << 4) | (channel & 15);
    DMB(); DSB();
}

uint32_t recv_mail(uint8_t channel)
{
    DMB(); DSB();
    do {
        while ((*MAIL0_STATUS) & (1u << 30)) { }
        uint32_t data = *MAIL0_READ;
        if ((data & 15) == channel) {
            DMB();
            return (data >> 4);
        //} else {
        //    printf("Incorrect channel (expected %u got %u)\n", channel, data & 15);
        }
    } while (1);
}

#define MAX_HANDLERS    128

static irq_handler handlers[MAX_HANDLERS] = { NULL };
static void *args[MAX_HANDLERS] = { NULL };

void set_irq_handler(uint8_t source, irq_handler f, void *arg)
{
    if (source < 0 || source >= MAX_HANDLERS) return;
    handlers[source] = f;
    args[source] = arg;
    DMB(); DSB();
    if (f) *INT_IRQENAB1 = (1 << source);
    else *INT_IRQDISA1 = (1 << source);
    DMB(); DSB();
}

void /*__attribute__((interrupt("IRQ")))*/ _int_irq()
{
    DMB(); DSB();
    uint32_t lr;
    __asm__ __volatile__ ("mov %0, r0" : "=r" (lr));

    DMB(); DSB();
    // Check interrupt source
    uint32_t pend_bitmask;
	uint8_t source;
recheck:
	source = 0;
	pend_bitmask = *INT_IRQPEND1;
    if (pend_bitmask == 0) {
        goto recheck;
    }
    source += __builtin_ctz(pend_bitmask);

    DMB(); DSB();
    if (handlers[source]) (*handlers[source])(args[source]);
    DMB(); DSB();
}
