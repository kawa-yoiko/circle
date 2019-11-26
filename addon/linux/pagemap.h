#ifndef _linux_pagemap_h
#define _linux_pagemap_h

#include <linux/envdefs.h>

#define PAGE_ALIGN(val)		(((val) + PAGE_SIZE-1) & ~(PAGE_SIZE-1))

#endif
