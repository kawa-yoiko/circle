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
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/sched/scheduler.h>
#include <vc4/vchiq/vchiqdevice.h>
#include <vc4/sound/vchiqsounddevice.h>
#include <circle/types.h>

CMemorySystem		m_Memory;
CActLED			m_ActLED;
CKernelOptions		m_Options;
CDeviceNameService	m_DeviceNameService;
CScreenDevice		m_Screen (m_Options.GetWidth (), m_Options.GetHeight ());
CSerialDevice		m_Serial;
CExceptionHandler	m_ExceptionHandler;
CInterruptSystem	m_Interrupt;
CTimer			m_Timer (&m_Interrupt);
CLogger			m_Logger (m_Options.GetLogLevel (), &m_Timer);
CScheduler		m_Scheduler;

CVCHIQDevice		m_VCHIQ (&m_Memory, &m_Interrupt);
CVCHIQSoundDevice	m_VCHIQSound (&m_VCHIQ, (TVCHIQSoundDestination) m_Options.GetSoundOption ());

static void Initialize ();
static void Run ();

int main (void)
{
	// cannot return here because some destructors used in CKernel are not implemented
	Initialize ();
	Run ();
	while (1) { }
}

#include "sound.h"

#define SOUND_SAMPLES		(sizeof Sound / sizeof Sound[0] / SOUND_CHANNELS)

static const char FromKernel[] = "kernel";

void Initialize (void)
{
	m_ActLED.Blink (5);	// show we are alive

	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
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
		bOK = m_VCHIQ.Initialize ();
	}
}

void Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	while (1)
	{
		if (!m_VCHIQSound.Playback (Sound, SOUND_SAMPLES, SOUND_CHANNELS, SOUND_BITS))
		{
			m_Logger.Write (FromKernel, LogPanic, "Cannot start playback");
		}

		m_Logger.Write (FromKernel, LogNotice, "Playback started");

		for (unsigned nCount = 0; m_VCHIQSound.PlaybackActive (); nCount++)
		{
			m_Screen.Rotor (0, nCount);

			m_Scheduler.Yield ();
		}

		m_Logger.Write (FromKernel, LogNotice, "Playback completed");

		m_Scheduler.Sleep (2);
	}
}
