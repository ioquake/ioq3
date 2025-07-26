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
#include "tr_local.h"
#include "tr_globals.h"
#include "tr_backend.h"
#include "tr_cvar.h"
#include "ref_import.h"

#include "vk_instance.h"
#include "vk_frame.h"
#include "vk_screenshot.h"
#include "vk_shade_geometry.h"
#include "RB_ShowImages.h"
#include "R_PrintMat.h"
#include "tr_light.h"

static renderCommandList_t	BE_Commands;

/*
============
R_GetCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void* R_GetCommandBuffer( int bytes )
{
	renderCommandList_t	*cmdList = &BE_Commands;

	// always leave room for the end of list command
	if ( cmdList->used + bytes + 4 > MAX_RENDER_COMMANDS )
    {
		if ( bytes > MAX_RENDER_COMMANDS - 4 ) {
			ri.Error( ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
		}
		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}


/*
=============
submits a single 'draw' command into the command queue
=============
*/
void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	drawSurfsCommand_t* cmd = (drawSurfsCommand_t*) R_GetCommandBuffer( sizeof(drawSurfsCommand_t) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_SURFS;

	cmd->drawSurfs = drawSurfs;
	cmd->numDrawSurfs = numDrawSurfs;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;

}


/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void RE_SetColor( const float *rgba )
{
    if ( !tr.registered ) {
        return;
    }
    
    setColorCommand_t* cmd = (setColorCommand_t*) R_GetCommandBuffer( sizeof(setColorCommand_t) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SET_COLOR;
	
    if(rgba)
    {
        cmd->color[0] = rgba[0];
        cmd->color[1] = rgba[1];
        cmd->color[2] = rgba[2];
        cmd->color[3] = rgba[3];
    }
    else
    {
        // color white
        cmd->color[0] = 1.0f;
        cmd->color[1] = 1.0f;
        cmd->color[2] = 1.0f;
        cmd->color[3] = 1.0f;
    }
}


void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader )
{
    if (!tr.registered) {
        return;
    }
    stretchPicCommand_t* cmd = (stretchPicCommand_t*) R_GetCommandBuffer(sizeof(stretchPicCommand_t));
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_STRETCH_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}


void RE_BeginFrame( stereoFrame_t stereoFrame )
{

	if ( !tr.registered ) {
		return;
	}

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	// draw buffer stuff
	drawBufferCommand_t* cmd = (drawBufferCommand_t*) R_GetCommandBuffer(sizeof(drawBufferCommand_t));
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_DRAW_BUFFER;
}


/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec )
{
	if ( !tr.registered ) {
		return;
	}
	swapBuffersCommand_t* cmd = (swapBuffersCommand_t*) R_GetCommandBuffer(sizeof(swapBuffersCommand_t));
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SWAP_BUFFERS;

	R_IssueRenderCommands( qtrue );

    R_InitNextFrame();

	if ( frontEndMsec ) {
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}




/*
==================
RB_RenderDrawSurfList
==================
*/
static void RB_RenderDrawSurfList( drawSurf_t* drawSurfs, int numDrawSurfs )
{
	shader_t		*shader, *oldShader;
	int				fogNum, oldFogNum;
	int				dlighted, oldDlighted;
	// save original time for entity shader offsets
	float originalTime = backEnd.refdef.floatTime;

    // Any mirrored or portaled views have already been drawn, 
    // so prepare to actually render the visible surfaces for this view
	// clear the z buffer, set the modelview, etc
	// RB_BeginDrawingView ();

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;


	// ensures that depth writes are enabled for the depth clear
    

	// VULKAN
    vk_clearDepthStencilAttachments();

	if ( backEnd.refdef.rd.rdflags & RDF_HYPERSPACE )
	{
		//RB_Hyperspace();
        // A player has predicted a teleport, but hasn't arrived yet
        const float c = ( backEnd.refdef.rd.time & 255 ) / 255.0f;
        const float color[4] = { c, c, c, 1 };

        // so short, do we really need this?
	    vk_clearColorAttachments(color);

	    backEnd.isHyperspace = qtrue;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}


	// draw everything
    int entityNum;
	int oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDlighted = qfalse;
	int oldSort = -1;

	backEnd.pc.c_surfaces += numDrawSurfs;

    drawSurf_t* drawSurf;

    int	i;

	for (i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++)
    {
		if ( (int)drawSurf->sort == oldSort ) {
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		oldSort = drawSurf->sort;
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if (shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted 
			|| ( entityNum != oldEntityNum && !shader->entityMergable ) ) {
			if (oldShader != NULL) {
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum );
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum )
        {
			if ( entityNum != REFENTITYNUM_WORLD )
            {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.or );


				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights ) {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
				}
			}
            else
            {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.or = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
			}


			// VULKAN
            set_modelview_matrix(backEnd.or.modelMatrix);
            oldEntityNum = entityNum;
        }

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	// go back to the world modelview matrix
    set_modelview_matrix(backEnd.viewParms.world.modelMatrix);


	// darken down any stencil shadows
	RB_ShadowFinish();		
}



void RB_StretchPic( const stretchPicCommand_t * const cmd )
{

	if ( qfalse == backEnd.projection2D )
    {

		backEnd.projection2D = qtrue;

        // set 2D virtual screen size
        // set time for 2D shaders
	    int t = ri.Milliseconds();
        
        backEnd.refdef.rd.time = t;
	    backEnd.refdef.floatTime = t * 0.001f;
	}


	if ( cmd->shader != tess.shader )
    {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface(cmd->shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );


	const unsigned int n0 = tess.numVertexes;
	const unsigned int n1 = n0 + 1;
	const unsigned int n2 = n0 + 2;
	const unsigned int n3 = n0 + 3;

	
    uint32_t numIndexes = tess.numIndexes;

    tess.indexes[ numIndexes ] = n3;
    tess.indexes[ numIndexes + 1 ] = n0;
    tess.indexes[ numIndexes + 2 ] = n2;
    tess.indexes[ numIndexes + 3 ] = n2;
    tess.indexes[ numIndexes + 4 ] = n0;
    tess.indexes[ numIndexes + 5 ] = n1;
	

    // TODO: verify does coding this way run faster in release mode ?
    // coding this way do harm to debug version because of
    // introduce additional 4 function call.
    memcpy(tess.vertexColors[ n0 ], backEnd.Color2D, 4);
    memcpy(tess.vertexColors[ n1 ], backEnd.Color2D, 4);
    memcpy(tess.vertexColors[ n2 ], backEnd.Color2D, 4);
    memcpy(tess.vertexColors[ n3 ], backEnd.Color2D, 4);


    tess.xyz[ n0 ][0] = cmd->x;
    tess.xyz[ n0 ][1] = cmd->y;
    tess.xyz[ n0 ][2] = 0;
    tess.xyz[ n1 ][0] = cmd->x + cmd->w;
    tess.xyz[ n1 ][1] = cmd->y;
    tess.xyz[ n1 ][2] = 0;
    tess.xyz[ n2 ][0] = cmd->x + cmd->w;
    tess.xyz[ n2 ][1] = cmd->y + cmd->h;
    tess.xyz[ n2 ][2] = 0;
    tess.xyz[ n3 ][0] = cmd->x;
    tess.xyz[ n3 ][1] = cmd->y + cmd->h;
    tess.xyz[ n3 ][2] = 0;


    tess.texCoords[ n0 ][0][0] = cmd->s1;
    tess.texCoords[ n0 ][0][1] = cmd->t1;

    tess.texCoords[ n1 ][0][0] = cmd->s2;
    tess.texCoords[ n1 ][0][1] = cmd->t1;

    tess.texCoords[ n2 ][0][0] = cmd->s2;
    tess.texCoords[ n2 ][0][1] = cmd->t2;

    tess.texCoords[ n3 ][0][0] = cmd->s1;
    tess.texCoords[ n3 ][0][1] = cmd->t2;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

}



static void R_PerformanceCounters( void )
{
    
	if (r_speeds->integer == 1) {
		ri.Printf (PRINT_ALL, "%i/%i shaders/surfs %i leafs %i verts %i/%i tris\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes, 
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3); 
	} else if (r_speeds->integer == 2) {
		ri.Printf (PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out, 
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		ri.Printf (PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out, 
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	} else if (r_speeds->integer == 3) {
		ri.Printf (PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
	} else if (r_speeds->integer == 4) {
		if ( backEnd.pc.c_dlightVertexes ) {
			ri.Printf (PRINT_ALL, "dlight srf:%i  culled:%i  verts:%i  tris:%i\n", 
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
		}
	} 

	memset( &tr.pc, 0, sizeof( tr.pc ) );
	memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}

/*
====================
This function will be called synchronously if running without
smp extensions, or asynchronously by another thread.
====================
*/
void R_IssueRenderCommands( qboolean runPerformanceCounters )
{

    if(runPerformanceCounters)
    {
        R_PerformanceCounters();
    }

    // actually start the commands going
    // let it start on the new batch
    // RB_ExecuteRenderCommands( cmdList->cmds );
    int	t1 = ri.Milliseconds ();

    // add an end-of-list command
    *(int *)(BE_Commands.cmds + BE_Commands.used) = RC_END_OF_LIST;


    const void * data = BE_Commands.cmds;


    while(1)
    {   
        const int T = *(const int *)data;
        switch ( T )
        {
            case RC_SET_COLOR:
            {
                const setColorCommand_t * const cmd = data;

                backEnd.Color2D[0] = cmd->color[0] * 255;
                backEnd.Color2D[1] = cmd->color[1] * 255;
                backEnd.Color2D[2] = cmd->color[2] * 255;
                backEnd.Color2D[3] = cmd->color[3] * 255;

                data += sizeof(setColorCommand_t);
            } break;

            case RC_STRETCH_PIC:
            {
                const stretchPicCommand_t * const cmd = data;

                RB_StretchPic( cmd );

                data += sizeof(stretchPicCommand_t);
            } break;

            case RC_DRAW_SURFS:
            {  
                const drawSurfsCommand_t * const cmd = (const drawSurfsCommand_t *)data;

                // RB_DrawSurfs( cmd );
                // finish any 2D drawing if needed
                if ( tess.numIndexes ) {
                    RB_EndSurface();
                }

                backEnd.refdef = cmd->refdef;
                backEnd.viewParms = cmd->viewParms;

                RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
                
                data += sizeof(drawSurfsCommand_t);
            } break;

            case RC_DRAW_BUFFER:
            {
                // data = RB_DrawBuffer( data ); 
                // const drawBufferCommand_t * const cmd = (const drawBufferCommand_t *)data;
                vk_resetGeometryBuffer();
                
                // VULKAN
                vk_begin_frame();

                data += sizeof(drawBufferCommand_t);

                // begin_frame_called = qtrue;
            } break;

            case RC_SWAP_BUFFERS:
            {
                // data = RB_SwapBuffers( data );
                // finish any 2D drawing if needed
                RB_EndSurface();

                // texture swapping test
                if ( r_showImages->integer ) {
                    RB_ShowImages(tr.images, tr.numImages);
                }

                // VULKAN
                vk_end_frame();

                data += sizeof(swapBuffersCommand_t);
            } break;

            case RC_SCREENSHOT:
            {   
                const screenshotCommand_t * const cmd = data;

                RB_TakeScreenshot( cmd->width, cmd->height, cmd->fileName, cmd->jpeg);

                data += sizeof(screenshotCommand_t);
            } break;


            case RC_VIDEOFRAME:
            {
                const videoFrameCommand_t * const cmd = data;

                RB_TakeVideoFrameCmd( cmd );

                data += sizeof(videoFrameCommand_t);
            } break;

            case RC_END_OF_LIST:
                // stop rendering on this thread
                backEnd.pc.msec = ri.Milliseconds () - t1;

                BE_Commands.used = 0;
                return;
        }
    }
}


/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
void FixRenderCommandList( int newShader )
{
	renderCommandList_t	*cmdList = &BE_Commands;

	if( cmdList ) {
		const void *curCmd = cmdList->cmds;

		while ( 1 ) {
			switch ( *(const int *)curCmd ) {
			case RC_SET_COLOR:
				{
				const setColorCommand_t *sc_cmd = (const setColorCommand_t *)curCmd;
				curCmd = (const void *)(sc_cmd + 1);
				break;
				}
			case RC_STRETCH_PIC:
				{
				const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)curCmd;
				curCmd = (const void *)(sp_cmd + 1);
				break;
				}
			case RC_DRAW_SURFS:
				{
				int i;
				drawSurf_t	*drawSurf;
				shader_t	*shader;
				int			fogNum;
				int			entityNum;
				int			dlightMap;
				int			sortedIndex;
				const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t *)curCmd;

				for( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ ) {
					R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlightMap );
                    sortedIndex = (( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1));
					if( sortedIndex >= newShader ) {
						sortedIndex++;
						drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT) | entityNum | ( fogNum << QSORT_FOGNUM_SHIFT ) | (int)dlightMap;
					}
				}
				curCmd = (const void *)(ds_cmd + 1);
				break;
				}
			case RC_DRAW_BUFFER:
				{
				const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)curCmd;
				curCmd = (const void *)(db_cmd + 1);
				break;
				}
			case RC_SWAP_BUFFERS:
				{
				const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)curCmd;
				curCmd = (const void *)(sb_cmd + 1);
				break;
				}
			case RC_END_OF_LIST:
			default:
				return;
			}
		}
	}
}
