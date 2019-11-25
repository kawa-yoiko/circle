#include <linux/delay.h>
#include <linux/env.h>

void udelay (unsigned long usecs)
{
	usDelay(usecs);
}

void msleep (unsigned msecs)
{
	MsDelay(msecs);
}
