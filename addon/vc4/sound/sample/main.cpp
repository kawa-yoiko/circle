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

#include <circle/memory.h>
#include <circle/screen.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/sched/scheduler.h>
#include <vc4/vchiq/vchiqdevice.h>
#include <vc4/sound/vchiqsoundbasedevice.h>
#include <circle/types.h>

#include <math.h>
#include "coroutine.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

CMemorySystem       m_Memory;
CScreenDevice       m_Screen (800, 480);
CInterruptSystem    m_Interrupt;
CTimer              m_Timer (&m_Interrupt);
CLogger             m_Logger (LogDebug, 0);
CScheduler          m_Scheduler;

CVCHIQDevice		    m_VCHIQ;
CVCHIQSoundBaseDevice   m_VCHIQSound;

static void Initialize ();
static void Run ();

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
	for (unsigned i = 0; i < chunk_size; i += 2) {
		//seed = ((seed * 1103515245) + 12345) & 0x7fffffff;
		//buf[i] = (int16_t)(seed & 0xffff);
		int16_t sample = (int16_t)(32767 * sin(phase / 255.0 * M_PI * 2));
		buf[i] = buf[i + 1] = sample;
		phase += 2; // Folds over to 0 ~ 255, generates 344.5 Hz (F4 - ~1/4 semitone)
	}
	return chunk_size;
}

void Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Logger.Initialize (&m_Screen);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = CVCHIQDevice_Initialize(&m_VCHIQ);
		CVCHIQSoundBaseDevice_Ctor(&m_VCHIQSound, &m_VCHIQ, 44100, 4000, VCHIQSoundDestinationAuto);
	}
}

void PlaybackThread (void *_unused)
{
	while (1)
	{
		CVCHIQSoundBaseDevice_Start(&m_VCHIQSound);

		m_Logger.Write (FromKernel, LogNotice, "Playback started");

		for (unsigned nCount = 0; CVCHIQSoundBaseDevice_IsActive (&m_VCHIQSound); nCount++)
		{
			m_Screen.Rotor (0, nCount);

			m_Scheduler.Yield ();
		}

		m_Logger.Write (FromKernel, LogNotice, "Playback completed");

		m_Scheduler.Sleep (2);
	}
}

__attribute__((noinline)) void iterate(int count)
{
	for (int i = 0, s = 0; i < count; i++) {
		s += i;
		m_Logger.Write(FromKernel, LogNotice, "> %d", s);
		co_yield();
	}
}

void fn(void *_unused)
{
	for (int i = 1; ; i++) {
		m_Logger.Write(FromKernel, LogNotice, "== %d ==", i);
		iterate(i);
	}
}

void Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_VCHIQSound.chunk_cb = synth;

	int id = co_create(fn, 0);
	while (1) {
		for (int i = 1; i <= 16; i++) co_next(i);
		m_Scheduler.Sleep(1);
	}
}
