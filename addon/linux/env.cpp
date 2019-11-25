#include <linux/env.h>
#include <circle/sched/scheduler.h>
#include <circle/timer.h>

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
