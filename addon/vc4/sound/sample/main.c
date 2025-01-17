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
#include "circle/synchronize.h"

#include <vc4/vchiq/vchiqdevice.h>
#include <vc4/sound/vchiqsoundbasedevice.h>

#include <math.h>
#include <linux/assert.h>
#include <linux/env.h>
#include "coroutine.h"
#include "common.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

CVCHIQDevice		    m_VCHIQ;
CVCHIQSoundBaseDevice   m_VCHIQSound;

static void Initialize ();
static void Run ();
void qwq_irq_stub ();

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
#define MEM_PAGE_TABLE1 0x128000    // Must be 16K aligned

// stub.S
void EnableMMU (void *base_address);

void InitializePageTable (void)
{
	uint32_t *m_pTable = (uint32_t *)MEM_PAGE_TABLE1;

	for (unsigned nEntry = 0; nEntry < 4096; nEntry++)
	{
		u32 nBaseAddress = nEntry << 20;

		// Fault
		u32 nAttributes = 0;

		extern u8 _etext;
		if (nBaseAddress < (u32) &_etext)
		{
			// Normal
			nAttributes = 0x0040E;
		}
		else if (nBaseAddress == 0x1c00000)
		{
			// Strongly ordered
			nAttributes = 0x10412;
		}
		else if (nBaseAddress < 256 * 1024 * 1024)
		{
			// Normal, no execution
			nAttributes = 0x0041E;
		}
		else if (nBaseAddress < 0x20FFFFFF)
		{
			// Shared device
			nAttributes = 0x10416;
		}

		m_pTable[nEntry] = nBaseAddress | nAttributes;
	}

	CleanDataCache ();
}

void Initialize (void)
{
	InitializePageTable ();
	EnableMMU ((void *)MEM_PAGE_TABLE1);

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
