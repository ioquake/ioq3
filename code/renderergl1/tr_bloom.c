/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// tr_bloom.c: 2D lighting post process effect

#include "tr_local.h"


static cvar_t *r_bloom;
static cvar_t *r_bloom_sample_size;
static cvar_t *r_bloom_fast_sample;
static cvar_t *r_bloom_alpha;
static cvar_t *r_bloom_darken;
static cvar_t *r_bloom_intensity;
static cvar_t *r_bloom_diamond_size;

/* 
============================================================================== 
 
						LIGHT BLOOMS
 
============================================================================== 
*/ 

static float Diamond8x[8][8] =
{ 
	{ 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, },
	{ 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, },
	{ 0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, },
	{ 0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, },
	{ 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, },
	{ 0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f  }
};

static float Diamond6x[6][6] =
{ 
	{ 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, },
	{ 0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f, }, 
	{ 0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f, },
	{ 0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f, },
	{ 0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f, },
	{ 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f  }
};

static float Diamond4x[4][4] =
{  
	{ 0.3f, 0.4f, 0.4f, 0.3f, },
	{ 0.4f, 0.9f, 0.9f, 0.4f, },
	{ 0.4f, 0.9f, 0.9f, 0.4f, },
	{ 0.3f, 0.4f, 0.4f, 0.3f  }
};

static struct {
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} effect;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} screen;
	struct {
		int		width, height;
	} work;
	qboolean started;
} bloom;


static void ID_INLINE R_Bloom_Quad( int width, int height, float texX, float texY, float texWidth, float texHeight ) {
	int x = 0;
	int y = 0;
	x = 0;
	y += glConfig.vidHeight - height;
	width += x;
	height += y;
	
	texWidth += texX;
	texHeight += texY;

	qglBegin( GL_QUADS );							
	qglTexCoord2f(	texX,						texHeight	);	
	qglVertex2f(	x,							y	);

	qglTexCoord2f(	texX,						texY	);				
	qglVertex2f(	x,							height	);	

	qglTexCoord2f(	texWidth,					texY	);				
	qglVertex2f(	width,						height	);	

	qglTexCoord2f(	texWidth,					texHeight	);	
	qglVertex2f(	width,						y	);				
	qglEnd ();
}


/*
=================
R_Bloom_InitTextures
=================
*/
static void R_Bloom_InitTextures( void )
{
	byte	*data;

	// find closer power of 2 to screen size 
	for (bloom.screen.width = 1;bloom.screen.width< glConfig.vidWidth;bloom.screen.width *= 2);
	for (bloom.screen.height = 1;bloom.screen.height < glConfig.vidHeight;bloom.screen.height *= 2);

	bloom.screen.readW = glConfig.vidWidth / (float)bloom.screen.width;
	bloom.screen.readH = glConfig.vidHeight / (float)bloom.screen.height;

	// find closer power of 2 to effect size 
	bloom.work.width = r_bloom_sample_size->integer;
	bloom.work.height = bloom.work.width * ( glConfig.vidWidth / glConfig.vidHeight );

	for (bloom.effect.width = 1;bloom.effect.width < bloom.work.width;bloom.effect.width *= 2);
	for (bloom.effect.height = 1;bloom.effect.height < bloom.work.height;bloom.effect.height *= 2);

	bloom.effect.readW = bloom.work.width / (float)bloom.effect.width;
	bloom.effect.readH = bloom.work.height / (float)bloom.effect.height;


	// disable blooms if we can't handle a texture of that size
	if( bloom.screen.width > glConfig.maxTextureSize ||
		bloom.screen.height > glConfig.maxTextureSize ||
		bloom.effect.width > glConfig.maxTextureSize ||
		bloom.effect.height > glConfig.maxTextureSize ||
		bloom.work.width > glConfig.vidWidth ||
		bloom.work.height > glConfig.vidHeight
	) {
		ri.Cvar_Set( "r_bloom", "0" );
		Com_Printf( S_COLOR_YELLOW"WARNING: 'R_InitBloomTextures' too high resolution for light bloom, effect disabled\n" );
		return;
	}

	data = ri.Hunk_AllocateTempMemory( bloom.screen.width * bloom.screen.height * 4 );
	Com_Memset( data, 0, bloom.screen.width * bloom.screen.height * 4 );
	bloom.screen.texture = R_CreateImage( "***bloom screen texture***", data, bloom.screen.width, bloom.screen.height, IMGTYPE_COLORALPHA, IMGFLAG_NONE, 0 );
	ri.Hunk_FreeTempMemory( data );

	data = ri.Hunk_AllocateTempMemory( bloom.effect.width * bloom.effect.height * 4 );
	Com_Memset( data, 0, bloom.effect.width * bloom.effect.height * 4 );
	bloom.effect.texture = R_CreateImage( "***bloom effect texture***", data, bloom.effect.width, bloom.effect.height, IMGTYPE_COLORALPHA, IMGFLAG_NONE, 0 );
	ri.Hunk_FreeTempMemory( data );
	bloom.started = qtrue;
}

/*
=================
R_InitBloomTextures
=================
*/
void R_InitBloomTextures( void )
{
	if( !r_bloom->integer )
		return;
	memset( &bloom, 0, sizeof( bloom ));
	R_Bloom_InitTextures ();
}

/*
=================
R_Bloom_DrawEffect
=================
*/
static void R_Bloom_DrawEffect( void )
{
	GL_Bind( bloom.effect.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglColor4f( r_bloom_alpha->value, r_bloom_alpha->value, r_bloom_alpha->value, 1.0f );
	R_Bloom_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
}


/*
=================
R_Bloom_GeneratexDiamonds
=================
*/
static void R_Bloom_WarsowEffect( void )
{
	int		i, j, k;
	float	intensity, scale, *diamond;


	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	//Take the backup texture and downscale it
	GL_Bind( bloom.screen.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0, bloom.screen.readW, bloom.screen.readH );
	//Copy downscaled framebuffer into a texture
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	// darkening passes with repeated filter
	if( r_bloom_darken->integer ) {
		int i;
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

		for( i = 0; i < r_bloom_darken->integer; i++ ) {
			R_Bloom_Quad( bloom.work.width, bloom.work.height, 
				0, 0, 
				bloom.effect.readW, bloom.effect.readH );
		}
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	}
	/* Copy the result to the effect texture */
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );

	// bluring passes, warsow uses a repeated semi blend on a selectable diamond grid
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR );
	if( r_bloom_diamond_size->integer > 7 || r_bloom_diamond_size->integer <= 3 ) {
		if( r_bloom_diamond_size->integer != 8 )
			ri.Cvar_Set( "r_bloom_diamond_size", "8" );
	} else if( r_bloom_diamond_size->integer > 5 ) {
		if( r_bloom_diamond_size->integer != 6 )
			ri.Cvar_Set( "r_bloom_diamond_size", "6" );
	} else if( r_bloom_diamond_size->integer > 3 ) {
		if( r_bloom_diamond_size->integer != 4 )
			ri.Cvar_Set( "r_bloom_diamond_size", "4" );
	}

	switch( r_bloom_diamond_size->integer ) {
		case 4:
			k = 2;
			diamond = &Diamond4x[0][0];
			scale = r_bloom_intensity->value * 0.8f;
			break;
		case 6:
			k = 3;
			diamond = &Diamond6x[0][0];
			scale = r_bloom_intensity->value * 0.5f;
			break;
		default:
//		case 8:
			k = 4;
			diamond = &Diamond8x[0][0];
			scale = r_bloom_intensity->value * 0.3f;
			break;
	}

	for( i = 0; i < r_bloom_diamond_size->integer; i++ ) {
		for( j = 0; j < r_bloom_diamond_size->integer; j++, diamond++ ) {
			float x, y;
			intensity =  *diamond * scale;
			if( intensity < 0.01f )
				continue;
			qglColor4f( intensity, intensity, intensity, 1.0 );
			x = (i - k) * ( 2 / 640.0f ) * bloom.effect.readW;
			y = (j - k) * ( 2 / 480.0f ) * bloom.effect.readH;

			R_Bloom_Quad( bloom.work.width, bloom.work.height, x, y, bloom.effect.readW, bloom.effect.readH );
		}
	}
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
}											

/*
=================
R_Bloom_BackupScreen
Backup the full original screen to a texture for downscaling and later restoration
=================
*/
static void R_Bloom_BackupScreen( void ) {
	GL_Bind( bloom.screen.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight );
}
/*
=================
R_Bloom_RestoreScreen
Restore the temporary framebuffer section we used with the backup texture
=================
*/
static void R_Bloom_RestoreScreen( void ) {
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( bloom.screen.texture );
	qglColor4f( 1, 1, 1, 1 );
	R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0,
		bloom.work.width / (float)bloom.screen.width,
		bloom.work.height / (float)bloom.screen.height );
}
 

/*
=================
R_Bloom_DownsampleView
Scale the copied screen back to the sample size used for subsequent passes
=================
*/
/*static void R_Bloom_DownsampleView( void )
{
	// TODO, Provide option to control the color strength here /
//	qglColor4f( r_bloom_darken->value, r_bloom_darken->value, r_bloom_darken->value, 1.0f );
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	GL_Bind( bloom.screen.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	//Downscale it
	R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0, bloom.screen.readW, bloom.screen.readH );
#if 1
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	// darkening passes
	if( r_bloom_darken->integer ) {
		int i;
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

		for( i = 0; i < r_bloom_darken->integer; i++ ) {
			R_Bloom_Quad( bloom.work.width, bloom.work.height, 
				0, 0, 
				bloom.effect.readW, bloom.effect.readH );
		}
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	}
#endif
	// Copy the result to the effect texture /
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
}

static void R_Bloom_CreateEffect( void ) {
	int dir, x;
	int range;

	//First step will zero dst, rest will one add
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
//	GL_Bind( bloom.screen.texture );
	GL_Bind( bloom.effect.texture );
	range = 4;
	for (dir = 0;dir < 2;dir++)
	{
		// blend on at multiple vertical offsets to achieve a vertical blur
		// TODO: do offset blends using GLSL
		for (x = -range;x <= range;x++)
		{
			float xoffset, yoffset, r;
			if (!dir){
				xoffset = 0;
				yoffset = x*1.5;
			} else {
				xoffset = x*1.5;
				yoffset = 0;
			}
			xoffset /= bloom.work.width;
			yoffset /= bloom.work.height;
			// this r value looks like a 'dot' particle, fading sharply to
			// black at the edges
			// (probably not realistic but looks good enough)
			//r = ((range*range+1)/((float)(x*x+1)))/(range*2+1);
			//r = (dir ? 1.0f : brighten)/(range*2+1);
			r = 2.0f /(range*2+1)*(1 - x*x/(float)(range*range));
//			r *= r_bloom_darken->value;
			qglColor4f(r, r, r, 1);
			R_Bloom_Quad( bloom.work.width, bloom.work.height, 
				xoffset, yoffset, 
				bloom.effect.readW, bloom.effect.readH );
//				bloom.screen.readW, bloom.screen.readH );
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		}
	}
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
}*/

/*
=================
R_BloomScreen
=================
*/
void R_BloomScreen( void )
{
	if( !r_bloom->integer )
		return;
	if ( backEnd.doneBloom )
		return;
	if ( !backEnd.doneSurfaces )
		return;
	backEnd.doneBloom = qtrue;
	if( !bloom.started ) {
		R_Bloom_InitTextures();
		if( !bloom.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();
#if 0
	// set up full screen workspace
	GL_TexEnv( GL_MODULATE );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();
	qglOrtho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity ();

	GL_Cull( CT_TWO_SIDED );
#endif

	qglColor4f( 1, 1, 1, 1 );

	//Backup the old screen in a texture
	R_Bloom_BackupScreen();
	// create the bloom texture using one of a few methods
	R_Bloom_WarsowEffect ();
//	R_Bloom_CreateEffect();
	// restore the screen-backup to the screen
	R_Bloom_RestoreScreen();
	// Do the final pass using the bloom texture for the final effect
	R_Bloom_DrawEffect ();
}


void R_BloomInit( void ) {
	memset( &bloom, 0, sizeof( bloom ));

	r_bloom = ri.Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );
	r_bloom_alpha = ri.Cvar_Get( "r_bloom_alpha", "0.3", CVAR_ARCHIVE );
	r_bloom_diamond_size = ri.Cvar_Get( "r_bloom_diamond_size", "8", CVAR_ARCHIVE );
	r_bloom_intensity = ri.Cvar_Get( "r_bloom_intensity", "1.3", CVAR_ARCHIVE );
	r_bloom_darken = ri.Cvar_Get( "r_bloom_darken", "4", CVAR_ARCHIVE );
	r_bloom_sample_size = ri.Cvar_Get( "r_bloom_sample_size", "128", CVAR_ARCHIVE|CVAR_LATCH );
	r_bloom_fast_sample = ri.Cvar_Get( "r_bloom_fast_sample", "0", CVAR_ARCHIVE|CVAR_LATCH );
}

