#include <linux/env.h>
#include <circle/timer.h>
#include <circle/interrupt.h>
#include <circle/bcmpropertytags.h>
#include <circle/util.h>
#include <circle/types.h>
#include <circle/logger.h>
#include <circle/stdarg.h>

#include "coroutine.h"

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
	CTimer::Get ()->usDelay (nMicroSeconds);
}

void MsDelay (unsigned nMilliSeconds)
{
	CTimer::Get ()->usDelay (nMilliSeconds * 1000);
}

void RegisterPeriodicHandler (TPeriodicTimerHandler *pHandler)
{
	CTimer::Get ()->RegisterPeriodicHandler (pHandler);
}

void ConnectInterrupt (unsigned nIRQ, TInterruptHandler *pHandler, void *pParam)
{
	CInterruptSystem::Get ()->ConnectIRQ (nIRQ, pHandler, pParam);
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
