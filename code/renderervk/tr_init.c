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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"
#include "tr_globals.h"
#include "tr_model.h"
#include "tr_cvar.h"

#include "vk_init.h"

#include "vk_screenshot.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"
#include "vk_image.h"

#include "tr_fog.h"
#include "tr_backend.h"
#include "glConfig.h"
#include "ref_import.h"

extern void RE_ClearScene( void );



void R_Init( void )
{	
	int i;

	ri.Printf( PRINT_ALL, "----- R_Init -----\n" );


	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );
	memset( &tess, 0, sizeof( tess ) );

    R_ClearBackendState();

	if ( (intptr_t)tess.xyz & 15 ) {
		ri.Printf( PRINT_ALL, "WARNING: tess.xyz not 16 byte aligned\n" );
	}


	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}

    R_InitDisplayResolution();

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	// make sure all the commands added here are also
	// removed in R_Shutdown
    ri.Cmd_AddCommand( "displayResoList", R_DisplayResolutionList_f );

    ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );

    ri.Cmd_AddCommand( "vkinfo", vulkanInfo_f );
    ri.Cmd_AddCommand( "minimize", vk_minimizeWindow );

    ri.Cmd_AddCommand( "pipelineList", R_PipelineList_f );

    ri.Cmd_AddCommand( "gpuMem", gpuMemUsageInfo_f );

    ri.Cmd_AddCommand( "printOR", R_PrintBackEnd_OR_f );

    R_InitScene();

    R_glConfigInit();

    // VULKAN
	if ( !isVKinitialied() )
	{
		vk_initialize();
        
        // print info
        vulkanInfo_f();
	}


	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

    ri.Printf( PRINT_ALL, "----- R_Init finished -----\n" );
}




void RE_Shutdown( qboolean destroyWindow )
{	

	ri.Printf( PRINT_ALL, "\nRE_Shutdown( %i )\n", destroyWindow );
    
    ri.Cmd_RemoveCommand("displayResoList");

	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshotJPEG");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("shaderlist");
	ri.Cmd_RemoveCommand("skinlist");

    ri.Cmd_RemoveCommand("minimize");
	
	ri.Cmd_RemoveCommand("vkinfo");
    ri.Cmd_RemoveCommand("pipelineList");
    ri.Cmd_RemoveCommand("gpuMem");
    ri.Cmd_RemoveCommand("printOR");

	R_DoneFreeType();

    // VULKAN
    // Releases vulkan resources allocated during program execution.
    // This effectively puts vulkan subsystem into initial state 
    // (the state we have after vk_initialize call).

    // contains vulkan resources/state, reinitialized on a map change.

    vk_destroyShaderStagePipeline();
 

    vk_resetGeometryBuffer();

	if ( tr.registered )
    {	
		vk_destroyImageRes();
        tr.registered = qfalse;
	}

    if (destroyWindow)
    {
        vk_shutdown();
        vk_destroyWindow();
        
        // It is cleared not for renderer_vulkan,
        // but fot rendergl1, renderergl2 to create the window
        R_glConfigClear();
    }
}


void RE_BeginRegistration(glconfig_t * pGlCfg)
{
	R_Init();

    R_GetGlConfig(pGlCfg);

	tr.viewCluster = -1; // force markleafs to regenerate

	RE_ClearScene();

	tr.registered = qtrue;

   	ri.Printf(PRINT_ALL, "RE_BeginRegistration finished.\n");
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void )
{
	if ( tr.registered ) {
		R_IssueRenderCommands( qfalse );
	}
}
