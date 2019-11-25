#include <linux/env.h>
#include <circle/sched/scheduler.h>
#include <circle/timer.h>
#include <circle/interrupt.h>
#include <circle/bcmpropertytags.h>
#include <circle/util.h>
#include <circle/types.h>
#include <circle/logger.h>
#include <circle/stdarg.h>

int SchedulerCreateThread (void (*fn) (void *), void *param)
{
}

void SchedulerRegisterSwitchHandler (void (*fn) (int))
{
}

void SchedulerYield ()
{
	CScheduler::Get ()->Yield ();
}

void usDelay (unsigned nMicroSeconds)
{
	CTimer::Get ()->usDelay (nMicroSeconds);
}

void MsDelay (unsigned nMilliSeconds)
{
	CScheduler::Get ()->MsSleep (nMilliSeconds);
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
