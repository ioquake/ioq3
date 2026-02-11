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

#include "tr_globals.h"
#include "tr_shader.h"
#include "ref_import.h"

/*
===============
Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void R_ShaderList_f (void)
{
	int			i;
	int			count = 0;
	shader_t*   pShader;

	ri.Printf (PRINT_ALL, "-----------------------\n");

	for ( i = 0 ; i < tr.numShaders ; i++ )
    {
		if ( ri.Cmd_Argc() > 1 )
			pShader = tr.sortedShaders[i];
		else
			pShader = tr.shaders[i];

		ri.Printf( PRINT_ALL, "%i ", pShader->numUnfoggedPasses );

		if (pShader->lightmapIndex >= 0 )
			ri.Printf (PRINT_ALL, "L ");
		else
			ri.Printf (PRINT_ALL, "  ");


		if ( pShader->multitextureEnv == GL_ADD )
			ri.Printf( PRINT_ALL, "MT(a) " );
		else if ( pShader->multitextureEnv == GL_MODULATE )
			ri.Printf( PRINT_ALL, "MT(m) " );
		else
			ri.Printf( PRINT_ALL, "      " );
	

		if ( pShader->explicitlyDefined )
			ri.Printf( PRINT_ALL, "E " );
		else
			ri.Printf( PRINT_ALL, "  " );


		if ( !pShader->isSky )
			ri.Printf( PRINT_ALL, "gen " );
		else
			ri.Printf( PRINT_ALL, "sky " );


		if ( pShader->defaultShader )
			ri.Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", pShader->name);
		else
			ri.Printf (PRINT_ALL,  ": %s\n", pShader->name);

		count++;
	}
	ri.Printf (PRINT_ALL, "%i total shaders\n", count);
	ri.Printf (PRINT_ALL, "------------------\n");
}

