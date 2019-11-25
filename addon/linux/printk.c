#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/env.h>

#define PRINTK_MAX_MSG_LEN	1000

static const char From[] = "printk";

int printk (const char *fmt, ...)
{
	va_list var;
	va_start (var, fmt);

	char Buffer[PRINTK_MAX_MSG_LEN+1];
	int len = snprintf (Buffer, sizeof Buffer, fmt, var);

	va_end (var);

	// NOTE: Does not take multi-line messages into account
	LogWrite(From, LOG_DEBUG, "%s", Buffer);

	return len;
}
