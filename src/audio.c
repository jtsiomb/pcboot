/*
pcboot - bootable PC demo/game kernel
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY, without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include "audio.h"
#include "au_sb.h"

struct audrv {
	void *(*get_buffer)(int *size);
	void (*start)(int rate, int nchan);
	void (*pause)(void);
	void (*cont)(void);
	void (*stop)(void);
	void (*volume)(int vol);
};

static struct audrv drv;

static audio_callback_func cbfunc;
static void *cbcls;

void audio_init(void)
{
	if(sb_detect()) {
		drv.get_buffer = sb_buffer;
		drv.start = sb_start;
		drv.pause = sb_pause;
		drv.cont = sb_continue;
		drv.stop = sb_stop;
		drv.volume = sb_volume;
		return;
	}

	printf("No supported audio device detected\n");
}

void audio_set_callback(audio_callback_func func, void *cls)
{
	cbfunc = func;
	cbcls = cls;
}

int audio_callback(void *buf, int sz)
{
	if(!cbfunc) {
		return 0;
	}
	return cbfunc(buf, sz, cbcls);
}

void audio_play(int rate, int nchan)
{
	drv.start(rate, nchan);
}

void audio_pause(void)
{
	drv.pause();
}

void audio_resume(void)
{
	drv.cont();
}

void audio_stop(void)
{
	drv.stop();
}

void audio_volume(int vol)
{
	drv.volume(vol);
}
