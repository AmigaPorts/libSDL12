/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_ahiaudio_h
#define _SDL_ahiaudio_h

#include <exec/exec.h>
#include <dos/dos.h>

#if defined(__SASC) || defined(WARPOS)
#include <proto/exec.h>
#else
#ifdef MORPHOS
#include <ppcinline/exec.h>
#else

#include <inline/exec.h>
#include <proto/dos.h>
#include <inline/dos.h>

#endif
#endif

#include <stdlib.h>
#include <string.h>

#include <devices/ahi.h>
#include "SDL_os3debug.h"

#include "../SDL_sysaudio.h"

/* Hidden "this" pointer for the audio functions */
#define _THIS    SDL_AudioDevice *this

struct SDL_PrivateAudioData {
	/* The handle for the audio device */
	struct AHIRequest	*ahi_IORequest[2];
	struct MsgPort		*ahi_ReplyPort;
	Sint32 freq, type, bytespersample, size;
	Uint8 *audio_MixBuffer[2];           /* The app mixing buffer */
	int current_buffer;
	Uint32 playing;
};

/* Old variable names */
#define audio_port		(data->ahi_ReplyPort)
#define audio_req		(data->ahi_IORequest)
#define mixbuf			(data->audio_MixBuffer)
#define current_buffer	(data->current_buffer)
#define playing			(data->playing)

typedef struct SDL_PrivateAudioData OS3AudioData;

#endif /* _SDL_ahiaudio_h */
