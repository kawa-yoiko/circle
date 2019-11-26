//
// main.c
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <circle/startup.h>

#include <circle/logger.h>
#include <vc4/vchiq/vchiqdevice.h>
#include <vc4/sound/vchiqsoundbasedevice.h>

#include <math.h>
#include <linux/assert.h>
#include <linux/env.h>
#include "coroutine.h"
#include "common.h"

#include <circle/pagetable.h>
#include <circle/armv6mmu.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

CVCHIQDevice		    m_VCHIQ;
CVCHIQSoundBaseDevice   m_VCHIQSound;

static void Initialize ();
static void Run ();
extern "C" void qwq_irq_stub ();

int main (void)
{
	// cannot return here because some destructors used in CKernel are not implemented
	Initialize ();
	Run ();
	while (1) { }
}

static const char FromKernel[] = "kernel";

unsigned synth(int16_t *buf, unsigned chunk_size)
{
	//uint32_t seed = 20191118;
	static uint8_t phase = 0;
	static uint32_t count = 0;
	if (count >= 131072) { count = 0; return 0; }
	for (unsigned i = 0; i < chunk_size; i += 2) {
		//seed = ((seed * 1103515245) + 12345) & 0x7fffffff;
		//buf[i] = (int16_t)(seed & 0xffff);
		int16_t sample = (int16_t)(32767 * sin(phase / 255.0 * M_PI * 2));
		buf[i] = buf[i + 1] = sample;
		phase += 2; // Folds over to 0 ~ 255, generates 344.5 Hz (F4 - ~1/4 semitone)
	}
	count += (chunk_size >> 1);
	return chunk_size;
}

static CPageTable m_pageTable(256 * 1024 * 1024);
static CPageTable *m_pPageTable = &m_pageTable;

#if RASPPI == 1
#define MMU_MODE	(  ARM_CONTROL_MMU			\
			 | ARM_CONTROL_L1_CACHE			\
			 | ARM_CONTROL_L1_INSTRUCTION_CACHE	\
			 | ARM_CONTROL_BRANCH_PREDICTION	\
			 | ARM_CONTROL_EXTENDED_PAGE_TABLE)
#else
#define MMU_MODE	(  ARM_CONTROL_MMU			\
			 | ARM_CONTROL_L1_CACHE			\
			 | ARM_CONTROL_L1_INSTRUCTION_CACHE	\
			 | ARM_CONTROL_BRANCH_PREDICTION)
#endif

#define TTBCR_SPLIT	0

void EnableMMU (void)
{
#if RASPPI <= 3
	u32 nAuxControl;
	asm volatile ("mrc p15, 0, %0, c1, c0,  1" : "=r" (nAuxControl));
#if RASPPI == 1
	nAuxControl |= ARM_AUX_CONTROL_CACHE_SIZE;	// restrict cache size (no page coloring)
#else
	nAuxControl |= ARM_AUX_CONTROL_SMP;
#endif
	asm volatile ("mcr p15, 0, %0, c1, c0,  1" : : "r" (nAuxControl));

	u32 nTLBType;
	asm volatile ("mrc p15, 0, %0, c0, c0,  3" : "=r" (nTLBType));
	assert (!(nTLBType & ARM_TLB_TYPE_SEPARATE_TLBS));

	// set TTB control
	asm volatile ("mcr p15, 0, %0, c2, c0,  2" : : "r" (TTBCR_SPLIT));

	// set TTBR0
	assert (m_pPageTable != 0);
	asm volatile ("mcr p15, 0, %0, c2, c0,  0" : : "r" (m_pPageTable->GetBaseAddress ()));
#else	// RASPPI <= 3
	// set MAIR0
	u32 nMAIR0 =   LPAE_MAIR_NORMAL   << ATTRINDX_NORMAL*8
                     | LPAE_MAIR_DEVICE   << ATTRINDX_DEVICE*8
	             | LPAE_MAIR_COHERENT << ATTRINDX_COHERENT*8;
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r" (nMAIR0));

	// set TTBCR
	asm volatile ("mcr p15, 0, %0, c2, c0,  2" : : "r" (
		        LPAE_TTBCR_EAE
		      | LPAE_TTBCR_EPD1
		      | ATTRIB_SH_INNER_SHAREABLE << LPAE_TTBCR_SH0__SHIFT
		      | LPAE_TTBCR_ORGN0_WR_BACK_ALLOCATE << LPAE_TTBCR_ORGN0__SHIFT
		      | LPAE_TTBCR_IRGN0_WR_BACK_ALLOCATE << LPAE_TTBCR_IRGN0__SHIFT
		      | LPAE_TTBCR_T0SZ_4GB));

	// set TTBR0
	assert (m_pPageTable != 0);
	u64 nBaseAddress = m_pPageTable->GetBaseAddress ();
	asm volatile ("mcrr p15, 0, %0, %1, c2" : : "r" ((u32) nBaseAddress),
						    "r" ((u32) (nBaseAddress >> 32)));
#endif	// RASPPI <= 3

	// set Domain Access Control register (Domain 0 to client)
	asm volatile ("mcr p15, 0, %0, c3, c0,  0" : : "r" (DOMAIN_CLIENT << 0));

#ifndef ARM_ALLOW_MULTI_CORE
	InvalidateDataCache ();
#else
	InvalidateDataCacheL1Only ();
#endif

	// required if MMU was previously enabled and not properly reset
	InvalidateInstructionCache ();
	FlushBranchTargetCache ();
	asm volatile ("mcr p15, 0, %0, c8, c7,  0" : : "r" (0));	// invalidate unified TLB
	DataSyncBarrier ();
	FlushPrefetchBuffer ();

	// enable MMU
	u32 nControl;
	asm volatile ("mrc p15, 0, %0, c1, c0,  0" : "=r" (nControl));
#if RASPPI == 1
#ifdef ARM_STRICT_ALIGNMENT
	nControl &= ~ARM_CONTROL_UNALIGNED_PERMITTED;
	nControl |= ARM_CONTROL_STRICT_ALIGNMENT;
#else
	nControl &= ~ARM_CONTROL_STRICT_ALIGNMENT;
	nControl |= ARM_CONTROL_UNALIGNED_PERMITTED;
#endif
#endif
	nControl |= MMU_MODE;
	asm volatile ("mcr p15, 0, %0, c1, c0,  0" : : "r" (nControl) : "memory");
}

void Initialize (void)
{
	EnableMMU ();

	boolean bOK = TRUE;

	if (bOK)
	{
		uint32_t *irq = (uint32_t *)0x18;
		*irq = 0xea000000 | ((uint32_t *)qwq_irq_stub - irq - 2);

		//SyncDataAndInstructionCache();
		__asm__ __volatile__ ("cpsie i");
	}

	if (bOK)
	{
		bOK = CVCHIQDevice_Initialize(&m_VCHIQ);
		CVCHIQSoundBaseDevice_Ctor(&m_VCHIQSound, &m_VCHIQ, 44100, 4000, VCHIQSoundDestinationAuto);
	}

	env_init();
}

void PlaybackThread (void *_unused)
{
	while (1)
	{
		CVCHIQSoundBaseDevice_Start(&m_VCHIQSound);

		for (unsigned nCount = 0; CVCHIQSoundBaseDevice_IsActive (&m_VCHIQSound); nCount++)
		{
			co_yield();
		}

		MsDelay (2000);
	}
}

void Run (void)
{
	m_VCHIQSound.chunk_cb = synth;

	int id = co_create(PlaybackThread, 0);
	while (1) {
		for (int i = 1; i <= 16; i++) co_next(i);
	}
}
