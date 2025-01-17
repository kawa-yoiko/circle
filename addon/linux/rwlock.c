#include <linux/rwlock.h>
#include <linux/bug.h>
#include <linux/env.h>

#define WRITE_LOCK	(1U << 31)

void read_lock_bh (rwlock_t *lock)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	lock->lock++;

	while (lock->lock >= WRITE_LOCK)
	{
		SchedulerYield();
	}
}

void read_unlock_bh (rwlock_t *lock)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	lock->lock--;
}

void write_lock_bh (rwlock_t *lock)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	lock->lock |= WRITE_LOCK;

	while ((lock->lock & ~WRITE_LOCK) != 0)
	{
		SchedulerYield();
	}
}

void write_unlock_bh (rwlock_t *lock)
{
#ifdef ARM_ALLOW_MULTI_CORE
	BUG_ON (CMultiCoreSupport::ThisCore () != 0);
#endif

	lock->lock &= ~WRITE_LOCK;
}
