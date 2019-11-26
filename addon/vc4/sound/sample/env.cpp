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
