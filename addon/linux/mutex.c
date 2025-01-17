#include <linux/mutex.h>
#include <linux/bug.h>
#include <linux/env.h>

void mutex_lock (struct mutex *lock)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	while (lock->lock != 0)
	{
		SchedulerYield();
	}

	lock->lock = 1;
}

void mutex_unlock (struct mutex *lock)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	lock->lock = 0;
}
