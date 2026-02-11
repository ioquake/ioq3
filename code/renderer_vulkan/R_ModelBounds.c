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

#include "tr_local.h"
#include "tr_model.h"


void RE_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs )
{
	model_t* model = R_GetModelByHandle( handle );

	if(model->type == MOD_BRUSH)
    {
		VectorCopy( model->bmodel->bounds[0], mins );
		VectorCopy( model->bmodel->bounds[1], maxs );
		return;
	}
    else if (model->type == MOD_MESH)
    {
		md3Header_t* header = model->md3[0];
		md3Frame_t* frame = (md3Frame_t *) ((unsigned char *)header + header->ofsFrames);

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		return;
	}
    else if (model->type == MOD_MDR)
    {
		mdrHeader_t* header = (mdrHeader_t *)model->modelData;
		mdrFrame_t* frame = (mdrFrame_t *) ((unsigned char *)header + header->ofsFrames);

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		
		return;
	}
    else if(model->type == MOD_IQM)
    {
		iqmData_t* iqmData = model->modelData;

		if(iqmData->bounds)
		{
			VectorCopy(iqmData->bounds, mins);
			VectorCopy(iqmData->bounds + 3, maxs);
			return;
		}
	}

	VectorClear( mins );
	VectorClear( maxs );
}
