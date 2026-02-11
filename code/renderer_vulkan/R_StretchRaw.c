/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2017-2025 Artem Kharytoniuk
Copyright (C) 2019-2024 Sui Jingfeng and the vkQuake3 contributors

This file is part of the Vulkan renderer of ioquake3, ported from vkQuake3
(https://github.com/suijingfeng/vkQuake3), whose Vulkan code is a C
conversion of the Vulkan backend of Quake-III-Arena-Kenny-Edition
(https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition).

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

#include "ref_import.h"
#include "tr_globals.h"


extern void RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
extern void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader );


/*
=============
FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const unsigned char *data, int client, qboolean dirty)
{
	int			i, j;

	if ( !tr.registered ) {
		return;
	}
	
    // make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ )
    {
        ;
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ )
    {
        ;
	}
    
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri.Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

    RE_UploadCinematic(w, h, cols, rows, data, client, dirty);

    tr.cinematicShader->stages[0]->bundle[0].image[0] = tr.scratchImage[client];
    RE_StretchPic(x, y, w, h,  0.5f / cols, 0.5f / rows,  1.0f - 0.5f / cols, 1.0f - 0.5 / rows, tr.cinematicShader->index);
}
