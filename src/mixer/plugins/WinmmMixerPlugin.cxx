/*
 * Copyright 2003-2016 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "mixer/MixerInternal.hxx"
#include "output/OutputAPI.hxx"
#include "output/plugins/WinmmOutputPlugin.hxx"
#include "util/Error.hxx"
#include "util/Domain.hxx"

#include <mmsystem.h>

#include <assert.h>
#include <math.h>
#include <windows.h>

class WinmmMixer final : public Mixer {
	WinmmOutput &output;

public:
	WinmmMixer(WinmmOutput &_output, MixerListener &_listener)
		:Mixer(winmm_mixer_plugin, _listener),
		output(_output) {
	}

	/* virtual methods from class Mixer */
	virtual bool Open(gcc_unused Error &error) override {
		return true;
	}

	virtual void Close() override {
	}

	virtual int GetVolume(Error &error) override;
	virtual bool SetVolume(unsigned volume, Error &error) override;
};

static constexpr Domain winmm_mixer_domain("winmm_mixer");

static inline int
winmm_volume_decode(DWORD volume)
{
	return lround((volume & 0xFFFF) / 655.35);
}

static inline DWORD
winmm_volume_encode(int volume)
{
	int value = lround(volume * 655.35);
	return MAKELONG(value, value);
}

static Mixer *
winmm_mixer_init(gcc_unused EventLoop &event_loop, AudioOutput &ao,
		 MixerListener &listener,
		 gcc_unused const ConfigBlock &block,
		 gcc_unused Error &error)
{
	return new WinmmMixer((WinmmOutput &)ao, listener);
}

int
WinmmMixer::GetVolume(Error &error)
{
	DWORD volume;
	HWAVEOUT handle = winmm_output_get_handle(output);
	MMRESULT result = waveOutGetVolume(handle, &volume);

	if (result != MMSYSERR_NOERROR) {
		error.Set(winmm_mixer_domain, "Failed to get winmm volume");
		return -1;
	}

	return winmm_volume_decode(volume);
}

bool
WinmmMixer::SetVolume(unsigned volume, Error &error)
{
	DWORD value = winmm_volume_encode(volume);
	HWAVEOUT handle = winmm_output_get_handle(output);
	MMRESULT result = waveOutSetVolume(handle, value);

	if (result != MMSYSERR_NOERROR) {
		error.Set(winmm_mixer_domain, "Failed to set winmm volume");
		return false;
	}

	return true;
}

const MixerPlugin winmm_mixer_plugin = {
	winmm_mixer_init,
	false,
};
