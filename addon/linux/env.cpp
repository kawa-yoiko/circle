#include <linux/env.h>
#include <circle/sched/scheduler.h>
#include <circle/timer.h>
#include <circle/interrupt.h>
#include <circle/bcmpropertytags.h>
#include <circle/util.h>
#include <circle/types.h>
#include <circle/logger.h>
#include <circle/stdarg.h>

class CKThread : public CTask
{
public:
	CKThread (int (*threadfn) (void *data), void *data)
	:	m_threadfn (threadfn),
		m_data (data)
	{
	}

	void Run (void)
	{
		(*m_threadfn) (m_data);
	}

private:
	int (*m_threadfn) (void *data);
	void *m_data;
};

static int next_pid = 1;

void SchedulerInitialize ()
{
	CTask *ctask = CScheduler::Get ()->GetCurrentTask ();
	ctask->SetUserData (0);
}

int SchedulerCreateThread (int (*fn) (void *), void *param)
{
	int pid = next_pid++;

	CTask *ctask = new CKThread (fn, param);
	ctask->SetUserData ((void *) pid);

	return pid;
}

void (*switch_handler) (int) = 0;

static void switch_handler_wrapper (CTask *task)
{
	if (switch_handler != 0)
		switch_handler((int) task->GetUserData ());
}

void SchedulerRegisterSwitchHandler (void (*fn) (int))
{
	switch_handler = fn;
	CScheduler::Get ()->RegisterTaskSwitchHandler (switch_handler_wrapper);
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
