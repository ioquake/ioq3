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
// tr_models.c -- model loading and caching

#include "tr_local.h"
#include "tr_globals.h"

#include "tr_model.h"
#include "ref_import.h"


model_t* R_GetModelByHandle( qhandle_t index )
{
	if ( (index < 0) || (index >= tr.numModels) )
    {
        ri.Printf(PRINT_WARNING, "index = %d, out of range gets the defualt model.\n", index);
		return tr.models[0];
	}

	return tr.models[index];
}


///////////////////////////////////////////////////////////////////////////////


void R_ModelInit( void )
{
    ri.Printf( PRINT_ALL, "R_ModelInit: \n");

	// leave a space for NULL model
	model_t* mod = ri.Hunk_Alloc( sizeof( model_t ), h_low );
	mod->index = tr.numModels = 0;
    mod->type = MOD_BAD;

	tr.models[tr.numModels] = mod;
	tr.numModels++;
}


void R_Modellist_f( void )
{
	int	i;
	int	total = 0;

	for ( i = 1 ; i < tr.numModels; i++ )
    {
		model_t* mod = tr.models[i];
		int lods = 1;
        int j;
		for ( j = 1 ; j < MD3_MAX_LODS ; j++ )
        {
			if ( mod->md3[j] && mod->md3[j] != mod->md3[j-1] )
            {
				lods++;
			}
		}
		ri.Printf( PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, lods, mod->name );
		total += mod->dataSize;
	}
	ri.Printf( PRINT_ALL, "%8i : Total models\n", total );
}
