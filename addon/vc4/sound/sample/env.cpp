#include <linux/env.h>
#include <circle/timer.h>
#include <circle/interrupt.h>
#include <circle/bcmpropertytags.h>
#include <circle/util.h>
#include <circle/types.h>
#include <circle/logger.h>
#include <circle/stdarg.h>

#include "coroutine.h"
#include "common.h"

void SchedulerInitialize ()
{
}

int SchedulerCreateThread (int (*fn) (void *), void *param)
{
	// Such function pointer casts are safe for most platforms including ARM
	return co_create((void (*)(void *))(fn), param);
}

void (*switch_handler) (int) = 0;

void SchedulerRegisterSwitchHandler (void (*fn) (int))
{
	co_callback((void (*)(int8_t))fn);
}

void SchedulerYield ()
{
	co_yield();
}

void usDelay (unsigned nMicroSeconds)
{
	DMB(); DSB();
	uint32_t val = *SYSTMR_CLO + nMicroSeconds;
	while (*SYSTMR_CLO < val) { }
	DMB(); DSB();
}

void MsDelay (unsigned nMilliSeconds)
{
	usDelay(nMilliSeconds * 1000);
}

static TPeriodicTimerHandler *periodic = NULL;

void RegisterPeriodicHandler (TPeriodicTimerHandler *pHandler)
{
	periodic = pHandler;
}

// Interrupts

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
	uint32_t pend_base = *INT_IRQBASPEND;
	uint8_t source;
	if (pend_base & (1 << 8)) {
		source = 0 + __builtin_ctz(*INT_IRQPEND1);
	} else if (pend_base & (1 << 9)) {
		source = 32 + __builtin_ctz(*INT_IRQPEND2);
	} else if (pend_base & 0xff) {
		source = 64 + __builtin_ctz(pend_base & 0xff);
	} else {
		//LogWrite("int", LOG_NOTICE, "??? %x %x %x", *INT_IRQPEND1, *INT_IRQPEND2, *INT_IRQBASPEND);
		DMB(); DSB();
		return;
	}
	if (source >= 4) LogWrite("int", LOG_NOTICE, "interrupt %d", (int)source);

	if (handlers[source]) (*handlers[source])(args[source]);
	DMB(); DSB();
}

void ConnectInterrupt (unsigned nIRQ, TInterruptHandler *pHandler, void *pParam)
{
	CInterruptSystem::Get ()->ConnectIRQ (nIRQ, pHandler, pParam);
}

#define T1_INTV	(1000000 / 100)

void timer1_handler(void *_unused)
{
	DMB(); DSB();
	uint32_t cur = *SYSTMR_CLO;
	uint32_t val = *SYSTMR_C1 + T1_INTV;
	if (val <= cur) val = cur + T1_INTV;
	*SYSTMR_C1 = val;
	*SYSTMR_CS = (1 << 1);
	DMB(); DSB();

	if (periodic) periodic();
}

void env_init()
{
	*SYSTMR_C1 = *SYSTMR_CLO + T1_INTV;
	ConnectInterrupt(1, timer1_handler, NULL);
}


struct TPropertyTagVCHIQInit
{
	TPropertyTag	Tag;
	u32		Data;
};

uint32_t EnableVCHIQ (uint32_t buf)
{
	TPropertyTagVCHIQInit tag;
	tag.Data = buf;

	CBcmPropertyTags Tags;
	if (!Tags.GetTag (0x48010, &tag, sizeof (TPropertyTagVCHIQInit)))
	{
		return 0;   // XXX: Maybe ENXIO
	}

	return tag.Data;
}

void LogWrite (const char *pSource,
               unsigned    Severity,
               const char *pMessage, ...)
{
	va_list var;
	va_start (var, pMessage);

	CLogger::Get ()->WriteV (pSource, (TLogSeverity) Severity, pMessage, var);

va_end (var);
}

#define COHERENT_SLOT_VCHIQ_START       (MEGABYTE / PAGE_SIZE / 2)
#define COHERENT_SLOT_VCHIQ_END         (MEGABYTE / PAGE_SIZE - 1)

// TODO: Make this work with 64-bit
u32 CMemorySystem_GetCoherentPage (unsigned nSlot)
{
	u32 nPageAddress = MEM_COHERENT_REGION;
	nPageAddress += nSlot * PAGE_SIZE;
	return nPageAddress;
}

void *GetCoherentRegion512K ()
{
	return (void *) CMemorySystem_GetCoherentPage (COHERENT_SLOT_VCHIQ_START);
}
