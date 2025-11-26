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

#ifdef USE_INTERNAL_SDL_HEADERS
#	include "SDL.h"
#	include "SDL_metal.h"
#else
#	include <SDL.h>
#	include <SDL_metal.h>
#endif

#include "../qcommon/q_shared.h"

static SDL_Window *g_window = nullptr;
static SDL_MetalView g_view = nullptr;
static void *g_layer = nullptr;

// Public API with C linkage
extern "C" {

/*
===============
SDLMetal_Init

Initialize SDL video subsystem and create Metal window/view
===============
*/
qboolean SDLMetal_Init(int width, int height, qboolean fullscreen)
{
	if (g_window)
	{
		// Already initialized
		return qtrue;
	}

	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
		{
			return qfalse;
		}
	}

	g_window = SDL_CreateWindow("ioquake3 (Metal)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
	                            SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE |
	                                (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
	if (!g_window)
	{
		return qfalse;
	}

	g_view = SDL_Metal_CreateView(g_window);
	if (!g_view)
	{
		SDL_DestroyWindow(g_window);
		g_window = nullptr;
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		return qfalse;
	}

	g_layer = SDL_Metal_GetLayer(g_view);

	return qtrue;
}

/*
===============
SDLMetal_Shutdown

Cleanup SDL Metal resources
===============
*/
void SDLMetal_Shutdown(qboolean destroyWindow)
{
	if (g_view)
	{
		SDL_Metal_DestroyView(g_view);
		g_view = nullptr;
	}
	if (destroyWindow && g_window)
	{
		SDL_DestroyWindow(g_window);
		g_window = nullptr;
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
	g_layer = nullptr;
}

/*
===============
SDLMetal_GetWindow

Returns the SDL window
===============
*/
SDL_Window *SDLMetal_GetWindow(void)
{
	return g_window;
}

/*
===============
SDLMetal_GetView

Returns the SDL Metal view
===============
*/
void *SDLMetal_GetView(void)
{
	return g_view;
}

/*
===============
SDLMetal_GetLayer

Returns the Metal layer (CA::MetalLayer*)
===============
*/
void *SDLMetal_GetLayer(void)
{
	return g_layer;
}

/*
===============
SDLMetal_GetDrawableSize

Get the current drawable size
===============
*/
void SDLMetal_GetDrawableSize(int *w, int *h)
{
	if (g_window)
	{
		::SDL_Metal_GetDrawableSize(g_window, w, h);
	}
	else
	{
		if (w) *w = 0;
		if (h) *h = 0;
	}
}

} // extern "C"
