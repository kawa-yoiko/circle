#ifndef _linux_envdefs_h
#define _linux_envdefs_h

// If RASPPI == 1 and GPU_L2_CACHE_ENABLED; otherwise use 0xc0000000
// See circle/bcm2835.h
#define GPU_MEM_BASE 0x40000000

#undef ARM_ALLOW_MULTI_CORE

#define PAGE_SIZE 4096

#endif
