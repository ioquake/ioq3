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


// the window surface needs to be createdd right after the instance creation
// because it can actually influence the physical device selection

#include "VKimpl.h"
#include "vk_instance.h"

#include "tr_cvar.h"
#include "icon_oa.h"
#include "glConfig.h"


#ifdef _WIN32
    #include "../SDL2/include/SDL.h"
    #include "../SDL2/include/SDL_vulkan.h"
#else

  #ifdef USE_LOCAL_HEADERS
    #include "../SDL2/include/SDL.h"
    #include "../SDL2/include/SDL_vulkan.h"
  #else
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_syswm.h>
    #include <SDL2/SDL_vulkan.h>
  #endif

#endif


static SDL_Window* window_sdl = NULL;


// TODO: multi display support
static cvar_t* r_displayIndex;


static void VKimp_DetectAvailableModes(void)
{
	int i, j;
	char buf[ MAX_STRING_CHARS ] = { 0 };

	SDL_DisplayMode windowMode;
    
	// If a window exists, note its display index
	if( window_sdl != NULL )
	{
		r_displayIndex->integer = SDL_GetWindowDisplayIndex( window_sdl );
		if( r_displayIndex->integer < 0 )
		{
			ri.Printf(PRINT_ALL, "SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError() );
            return;
		}
	}

	int numSDLModes = SDL_GetNumDisplayModes( r_displayIndex->integer );

	if( SDL_GetWindowDisplayMode( window_sdl, &windowMode ) < 0 || numSDLModes <= 0 )
	{
		ri.Printf(PRINT_ALL, "Couldn't get window display mode, no resolutions detected: %s\n", SDL_GetError() );
		return;
	}

	int numModes = 0;
	SDL_Rect* modes = SDL_calloc(numSDLModes, sizeof( SDL_Rect ));
	if ( !modes )
	{
        ////////////////////////////////////
		ri.Error(ERR_FATAL, "Out of memory" );
        ////////////////////////////////////
	}

	for( i = 0; i < numSDLModes; i++ )
	{
		SDL_DisplayMode mode;

		if( SDL_GetDisplayMode( r_displayIndex->integer, i, &mode ) < 0 )
			continue;

		if( !mode.w || !mode.h )
		{
			ri.Printf(PRINT_ALL,  " Display supports any resolution\n" );
			SDL_free( modes );
			return;
		}

		if( windowMode.format != mode.format )
			continue;

		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for( j = 0; j < numModes; j++ )
		{
			if( (mode.w == modes[ j ].w) && (mode.h == modes[ j ].h) )
				break;
		}

		if( j != numModes )
			continue;

		modes[ numModes ].w = mode.w;
		modes[ numModes ].h = mode.h;
		numModes++;
	}

	for( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );

		if( strlen( newModeString ) < (int)sizeof( buf ) - strlen( buf ) )
			Q_strcat( buf, sizeof( buf ), newModeString );
		else
			ri.Printf(PRINT_ALL,  " Skipping mode %ux%u, buffer too small\n", modes[ i ].w, modes[ i ].h );
	}

	if( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		ri.Printf(PRINT_ALL, " Available modes: '%s'\n", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}
	SDL_free( modes );
}


static int VKimp_SetMode(int mode, qboolean fullscreen)
{
	SDL_DisplayMode desktopMode;

	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;

	if ( r_allowResize->integer )
		flags |= SDL_WINDOW_RESIZABLE;


	ri.Printf(PRINT_ALL,  "\n...VKimp_SetMode()...\n");

    SDL_GetNumVideoDisplays();

	int display_mode_count = SDL_GetNumDisplayModes(r_displayIndex->integer);
	if (display_mode_count < 1)
	{
		ri.Printf(PRINT_ALL, " SDL_GetNumDisplayModes failed: %s", SDL_GetError());
	}


    int tmp = SDL_GetDesktopDisplayMode(r_displayIndex->integer, &desktopMode);
	if( (tmp == 0) && (desktopMode.h > 0) )
    {
    	Uint32 f = desktopMode.format;
        ri.Printf(PRINT_ALL, " bpp %i\t%s\t%i x %i, refresh_rate: %dHz\n", SDL_BITSPERPIXEL(f), SDL_GetPixelFormatName(f), desktopMode.w, desktopMode.h, desktopMode.refresh_rate);
    }
    else if (SDL_GetDisplayMode(r_displayIndex->integer, 0, &desktopMode) != 0)
	{
    	//mode = 0: use the first display mode SDL return;
        ri.Printf(PRINT_ALL," SDL_GetDisplayMode failed: %s\n", SDL_GetError());
        mode = 3;
        desktopMode.w = 640;
        desktopMode.h = 480;
        desktopMode.refresh_rate = 60;
        fullscreen = 0;
	}

    if(fullscreen)
    {
        // prevent crush the OS
        r_mode->integer = mode = -2;
        		
        flags |= SDL_WINDOW_FULLSCREEN;
		flags |= SDL_WINDOW_BORDERLESS;
    }

    R_SetWinMode( mode, desktopMode.w, desktopMode.h, desktopMode.refresh_rate );
    


    if( window_sdl != NULL )
	{
		// SDL_GetWindowPosition( window_sdl, &x, &y );
		SDL_DestroyWindow( window_sdl );
		window_sdl = NULL;
        ri.Printf(PRINT_ALL, "Existing window being destroyed\n");
	}

    int width = 640;
    int height = 480;

    R_GetWinResolution(&width, &height);

	window_sdl = SDL_CreateWindow( 
					CLIENT_WINDOW_TITLE, 
					SDL_WINDOWPOS_CENTERED,
					SDL_WINDOWPOS_CENTERED, 
					width, 
					height, 
					flags );


	if( window_sdl )
    {
        VKimp_DetectAvailableModes();
        return 0;
    }

	ri.Printf(PRINT_WARNING, " Couldn't create a window: %s\n", SDL_GetError() );
    return -1;
}



/*
 * This routine is responsible for initializing the OS specific portions of Vulkan
 */
void vk_createWindow(void)
{
	ri.Printf(PRINT_ALL, "\n...Creating window (using SDL2)...\n");

	// Print SDL2 Version ....
	SDL_version v;
	SDL_version *sdl_version = &v;
	SDL_GetVersion(&v);
	ri.Printf(PRINT_ALL, " Found SDL version %i.%i.%i\n",
		sdl_version->major,sdl_version->minor,sdl_version->patch);

	r_displayIndex = ri.Cvar_Get( "r_displayIndex", "0", CVAR_ARCHIVE | CVAR_LATCH );

	SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(
			(void *)CLIENT_WINDOW_ICON.pixel_data,
			CLIENT_WINDOW_ICON.width,
			CLIENT_WINDOW_ICON.height,
			CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
			CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
            0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
			0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
			);

	if (icon == NULL)
	{
		ri.Printf(PRINT_ALL, " SDL_CreateRGBSurface Failed. \n" );
	}

	if (ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri.Cvar_Set( "r_fullscreen", "0" );
        ri.Cvar_Set( "r_mode", "3" );
		ri.Cvar_Set( "com_abnormalExit", "0" );
	}

	// Use this function to get a mask of the specified
	// subsystems which have previously been initialized.
	// If flags is 0 it returns a mask of all initialized subsystems,
	// otherwise it returns the initialization status of the specified subsystems.
	if (0 == SDL_WasInit(SDL_INIT_VIDEO))
	{
		ri.Printf(PRINT_ALL, " Video is not initialized before, so initial it.\n");

		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			ri.Printf(PRINT_ALL, " SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n", SDL_GetError());
		}
		else
		{
			ri.Printf(PRINT_ALL, " SDL using driver \"%s\"\n", SDL_GetCurrentVideoDriver( ));
		}
	}
	else
	{
		ri.Printf(PRINT_ALL, " Video is already initialized.\n");
	}

	if( 0 == VKimp_SetMode(r_mode->integer, r_fullscreen->integer) )
	{
		goto success;
	}
	else
	{
		ri.Printf(PRINT_ALL, " Setting r_mode=%d, r_fullscreen=%d failed, falling back on r_mode=%d\n",
				r_mode->integer, r_fullscreen->integer, 3 );

		if( 0 == VKimp_SetMode(3, qfalse) )
		{
			goto success;
		}
		else
		{
			ri.Error(ERR_FATAL, "VKimp_Init() - could not load Vulkan subsystem: %s", SDL_GetError());
		}
	}


success:

	SDL_SetWindowIcon( window_sdl, icon );

	SDL_FreeSurface( icon );

	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init(window_sdl);
}


void vk_getInstanceProcAddrImpl(void)
{
	ri.Printf(PRINT_ALL, " *** Vulkan Initialization ***\n");
    int code = SDL_Vulkan_LoadLibrary(NULL);
    if (code) {
        ri.Error(ERR_FATAL, "Failed to load Vulkan library (code %d): %s", code, SDL_GetError());
    }
    // Create the window 

    qvkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) SDL_Vulkan_GetVkGetInstanceProcAddr();
    if( qvkGetInstanceProcAddr == NULL)
    {
        ri.Error(ERR_FATAL, "Failed to find entrypoint vkGetInstanceProcAddr\n"); 
    }
    
    ri.Printf(PRINT_ALL,  " Get instance proc address. (using SDL2)\n");
}


void vk_destroyWindow( void )
{
	ri.Printf(PRINT_ALL, " Destroy Window Subsystem.\n");

	ri.IN_Shutdown();
	SDL_QuitSubSystem( SDL_INIT_VIDEO );

    SDL_DestroyWindow( window_sdl );
    window_sdl = NULL;
}


void vk_createSurfaceImpl(void)
{
    ri.Printf(PRINT_ALL, " Create Surface: vk.surface.\n");

    if(!SDL_Vulkan_CreateSurface(window_sdl, vk.instance, &vk.surface))
    {
        vk.surface = VK_NULL_HANDLE;
        ri.Error(ERR_FATAL, "SDL_Vulkan_CreateSurface(): %s\n", SDL_GetError());
    }
}




/*
===============
Minimize the game so that user is back at the desktop
===============
*/
void vk_minimizeWindow( void )
{
    VkBool32 toggleWorked = 1;
    ri.Printf( PRINT_ALL, " Minimizing Window (SDL). \n");

	VkBool32 isWinFullscreen = ( SDL_GetWindowFlags( window_sdl ) & SDL_WINDOW_FULLSCREEN );
    

    if( isWinFullscreen )
	{
		toggleWorked = (SDL_SetWindowFullscreen( window_sdl, 0 ) >= 0);
	}

    // SDL_WM_ToggleFullScreen didn't work, so do it the slow way
    if( toggleWorked )
    {
        // ri.IN_Shutdown( );
        SDL_MinimizeWindow( window_sdl );
        // SDL_HideWindow( window_sdl );
    }
    else
    {
        ri.Printf( PRINT_ALL, " SDL_SetWindowFullscreen didn't work, so do it the slow way \n");

        ri.Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");
    }
}

/*
	if( r_fullscreen->modified )
	{
		qboolean    needToToggle;
		qboolean    sdlToggled = qfalse;

		// Find out the current state
		int fullscreen = !!( SDL_GetWindowFlags( window_sdl ) & SDL_WINDOW_FULLSCREEN );

		if( r_fullscreen->integer && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
		{
			ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
			ri.Cvar_Set( "r_fullscreen", "0" );
			r_fullscreen->modified = qfalse;
		}

		// Is the state we want different from the current state?
		needToToggle = !!r_fullscreen->integer != fullscreen;

		if( needToToggle )
		{
			sdlToggled = SDL_SetWindowFullscreen( window_sdl, r_fullscreen->integer ) >= 0;

			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if( !sdlToggled )
				ri.Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");

			ri.IN_Restart( );
		}

		r_fullscreen->modified = qfalse;
	}
*/
