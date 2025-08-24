/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <stdlib.h>
#include <stdio.h>

#ifdef USE_INTERNAL_SDL_HEADERS
#	include "SDL3/SDL.h"
#else
#	include <SDL3/SDL.h>
#endif

#include "../qcommon/q_shared.h"
#include "../client/snd_local.h"
#include "../client/client.h"

qboolean snd_inited = qfalse;

cvar_t *s_sdlBits;
cvar_t *s_sdlSpeed;
cvar_t *s_sdlChannels;
cvar_t *s_sdlDevSamps;
cvar_t *s_sdlMixSamps;

/* The audio callback. All the magic happens here. */
static int dmapos = 0;
static int dmasize = 0;

static SDL_AudioDeviceID sdlPlaybackDevice;
static SDL_AudioStream *sdlPlaybackStream;

#ifdef USE_VOIP
#define USE_SDL_AUDIO_CAPTURE

static SDL_AudioDeviceID sdlCaptureDevice;
static SDL_AudioStream *sdlCaptureStream;
static cvar_t *s_sdlCapture;
#endif


/*
===============
SNDDMA_AudioCallback
===============
*/
static void SDLCALL SNDDMA_AudioCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	if (!snd_inited || additional_amount <= 0) {
		return;
	}

	int pos = (dmapos * (dma.samplebits/8));
	if (pos >= dmasize)
		dmapos = pos = 0;

	int tobufend = dmasize - pos;  /* bytes to buffer's end. */
	int len1 = additional_amount;
	int len2 = 0;

	if (len1 > tobufend)
	{
		len1 = tobufend;
		len2 = additional_amount - len1;
	}
	SDL_PutAudioStreamData(stream, dma.buffer + pos, len1);
	if (len2 <= 0)
		dmapos += (len1 / (dma.samplebits/8));
	else  /* wraparound? */
	{
		SDL_PutAudioStreamData(stream, dma.buffer, len2);
		dmapos = (len2 / (dma.samplebits/8));
	}

	if (dmapos >= dmasize)
		dmapos = 0;
}

static struct
{
	Uint16	enumFormat;
	char		*stringFormat;
} formatToStringTable[ ] =
{
	{ SDL_AUDIO_U8,     "SDL_AUDIO_U8" },
	{ SDL_AUDIO_S8,     "SDL_AUDIO_S8" },
	{ SDL_AUDIO_S16LE,  "SDL_AUDIO_S16LE" },
	{ SDL_AUDIO_S16BE,  "SDL_AUDIO_S16BE" },
	{ SDL_AUDIO_F32LE,  "SDL_AUDIO_F32LE" },
	{ SDL_AUDIO_F32BE,  "SDL_AUDIO_F32BE" }
};

static int formatToStringTableSize = ARRAY_LEN( formatToStringTable );

/*
===============
SNDDMA_PrintAudiospec
===============
*/
static void SNDDMA_PrintAudiospec(const char *str, const SDL_AudioSpec *spec)
{
	int		i;
	char	*fmt = NULL;

	Com_Printf("%s:\n", str);

	for( i = 0; i < formatToStringTableSize; i++ ) {
		if( spec->format == formatToStringTable[ i ].enumFormat ) {
			fmt = formatToStringTable[ i ].stringFormat;
		}
	}

	if( fmt ) {
		Com_Printf( "  Format:   %s\n", fmt );
	} else {
		Com_Printf( "  Format:   " S_COLOR_RED "UNKNOWN\n");
	}

	Com_Printf( "  Freq:     %d\n", (int) spec->freq );
	Com_Printf( "  Channels: %d\n", (int) spec->channels );
}

/*
===============
SNDDMA_Init
===============
*/
qboolean SNDDMA_Init(void)
{
	SDL_AudioSpec desired;
	SDL_AudioSpec obtained;
	int sample_frames = 0;
	int tmp;

	if (snd_inited)
		return qtrue;

	if (!s_sdlBits) {
		s_sdlBits = Cvar_Get("s_sdlBits", "16", CVAR_ARCHIVE);
		s_sdlSpeed = Cvar_Get("s_sdlSpeed", "0", CVAR_ARCHIVE);
		s_sdlChannels = Cvar_Get("s_sdlChannels", "2", CVAR_ARCHIVE);
		s_sdlDevSamps = Cvar_Get("s_sdlDevSamps", "0", CVAR_ARCHIVE);
		s_sdlMixSamps = Cvar_Get("s_sdlMixSamps", "0", CVAR_ARCHIVE);
	}

	Com_Printf( "SDL_Init( SDL_INIT_AUDIO )... " );

	if (!SDL_Init(SDL_INIT_AUDIO))
	{
		Com_Printf( "FAILED (%s)\n", SDL_GetError( ) );
		return qfalse;
	}

	Com_Printf( "OK\n" );

	Com_Printf( "SDL audio driver is \"%s\".\n", SDL_GetCurrentAudioDriver( ) );

	memset(&desired, '\0', sizeof (desired));
	memset(&obtained, '\0', sizeof (obtained));

	tmp = ((int) s_sdlBits->value);
	if ((tmp != 16) && (tmp != 8))
		tmp = 16;

	desired.freq = (int) s_sdlSpeed->value;
	if(!desired.freq) desired.freq = 44100;
	desired.format = ((tmp == 16) ? SDL_AUDIO_S16 : SDL_AUDIO_U8);
	desired.channels = (int) s_sdlChannels->value;

	sdlPlaybackStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, SNDDMA_AudioCallback, NULL);
	if (sdlPlaybackStream == NULL)
	{
		Com_Printf("SDL_OpenAudioDeviceStream() failed: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return qfalse;
	}

	sdlPlaybackDevice = SDL_GetAudioStreamDevice(sdlPlaybackStream);
	if (!SDL_GetAudioDeviceFormat(sdlPlaybackDevice, &obtained, &sample_frames))
	{
		Com_Printf("SDL_GetAudioDeviceFormat() failed: %s\n", SDL_GetError());
		// Use desired format if we can't get the actual format
		obtained = desired;
		sample_frames = 1024;
	}

	SNDDMA_PrintAudiospec("SDL_AudioSpec", &obtained);

	// dma.samples needs to be big, or id's mixer will just refuse to
	//  work at all; we need to keep it significantly bigger than the
	//  amount of SDL callback samples, and just copy a little each time
	//  the callback runs.
	// 32768 is what the OSS driver filled in here on my system. I don't
	//  know if it's a good value overall, but at least we know it's
	//  reasonable...this is why I let the user override.
	tmp = s_sdlMixSamps->value;
	if (!tmp)
		tmp = (sample_frames * obtained.channels) * 10;

	// samples must be divisible by number of channels
	tmp -= tmp % obtained.channels;

	dmapos = 0;
	dma.samplebits = SDL_AUDIO_BITSIZE(obtained.format);
	dma.isfloat = SDL_AUDIO_ISFLOAT(obtained.format);
	dma.channels = obtained.channels;
	dma.samples = tmp;
	dma.fullsamples = dma.samples / dma.channels;
	dma.submission_chunk = 1;
	dma.speed = obtained.freq;
	dmasize = (dma.samples * (dma.samplebits/8));
	dma.buffer = calloc(1, dmasize);

#ifdef USE_SDL_AUDIO_CAPTURE
	// !!! FIXME: some of these SDL_OpenAudioDevice() values should be cvars.
	s_sdlCapture = Cvar_Get( "s_sdlCapture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	if (!s_sdlCapture->integer)
	{
		Com_Printf("SDL audio capture support disabled by user ('+set s_sdlCapture 1' to enable)\n");
	}
#if USE_MUMBLE
	else if (cl_useMumble->integer)
	{
		Com_Printf("SDL audio capture support disabled for Mumble support\n");
	}
#endif
	else
	{
		/* !!! FIXME: list available devices and let cvar specify one, like OpenAL does */
		SDL_AudioSpec spec;
		SDL_zero(spec);
		spec.freq = 48000;
		spec.format = SDL_AUDIO_S16;
		spec.channels = 1;
		sdlCaptureStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, NULL);
		if (sdlCaptureStream != NULL) {
			sdlCaptureDevice = SDL_GetAudioStreamDevice(sdlCaptureStream);
		} else {
			sdlCaptureDevice = 0;
		}
		Com_Printf( "SDL capture device %s.\n",
				    (sdlCaptureDevice == 0) ? "failed to open" : "opened");
	}
#endif

	Com_Printf("Starting SDL audio stream...\n");
	SDL_ResumeAudioDevice(sdlPlaybackDevice);
	// don't unpause the capture device; we'll do that in StartCapture.

	Com_Printf("SDL audio initialized.\n");
	snd_inited = qtrue;
	return qtrue;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int SNDDMA_GetDMAPos(void)
{
	return dmapos;
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void)
{
	if (sdlPlaybackStream != NULL)
	{
		Com_Printf("Closing SDL audio playback stream...\n");
		SDL_DestroyAudioStream(sdlPlaybackStream);
		Com_Printf("SDL audio playback stream closed.\n");
		sdlPlaybackStream = NULL;
		sdlPlaybackDevice = 0;
	}

#ifdef USE_SDL_AUDIO_CAPTURE
	if (sdlCaptureStream != NULL)
	{
		Com_Printf("Closing SDL audio capture stream...\n");
		SDL_DestroyAudioStream(sdlCaptureStream);
		Com_Printf("SDL audio capture stream closed.\n");
		sdlCaptureStream = NULL;
		sdlCaptureDevice = 0;
	}
#endif

	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	free(dma.buffer);
	dma.buffer = NULL;
	dmapos = dmasize = 0;
	snd_inited = qfalse;
	Com_Printf("SDL audio shut down.\n");
}

/*
===============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting (void)
{
}


#ifdef USE_VOIP
void SNDDMA_StartCapture(void)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	if (sdlCaptureDevice)
	{
		SDL_ResumeAudioDevice(sdlCaptureDevice);
	}
#endif
}

int SNDDMA_AvailableCaptureSamples(void)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	// divided by 2 to convert from bytes to (mono16) samples.
	return sdlCaptureStream ? (SDL_GetAudioStreamAvailable(sdlCaptureStream) / 2) : 0;
#else
	return 0;
#endif
}

void SNDDMA_Capture(int samples, byte *data)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	// multiplied by 2 to convert from (mono16) samples to bytes.
	if (sdlCaptureStream)
	{
		SDL_GetAudioStreamData(sdlCaptureStream, data, samples * 2);
	}
	else
#endif
	{
		SDL_memset(data, '\0', samples * 2);
	}
}

void SNDDMA_StopCapture(void)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	if (sdlCaptureDevice)
	{
		SDL_PauseAudioDevice(sdlCaptureDevice);
	}
#endif
}

void SNDDMA_MasterGain( float val )
{
#ifdef USE_SDL_AUDIO_CAPTURE
	SDL_SetAudioStreamGain( sdlPlaybackStream, val );
#endif
}
#endif

