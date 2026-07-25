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

/*
============================================================================

SKINS

============================================================================
*/

#include "tr_local.h"
#include "tr_globals.h"
#include "ref_import.h"
#include "tr_shader.h"


/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p )
{
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while( (c = *data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	if (len == MAX_TOKEN_CHARS)
	{
		ri.Printf (PRINT_ALL, "Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}



qhandle_t RE_RegisterSkin( const char *name )
{
    skinSurface_t parseSurfaces[MAX_SKIN_SURFACES]; 
	qhandle_t	hSkin;
	skin_t		*skin;
	skinSurface_t	*surf;
	char		*text, *text_p;
	char		*token;
	char		surfName[MAX_QPATH];

	if ( !name || !name[0] ) {
		ri.Printf(PRINT_ALL, "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( (int)strlen( name ) >= MAX_QPATH ) {
		ri.Printf(PRINT_ALL, "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}


	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, name ) ) {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	skin = (skin_t*) ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;


	// If not a .skin file, load as a single shader
	if ( strcmp( name + (int)strlen( name ) - 5, ".skin" ) ) {
		skin->numSurfaces = 1;
		skin->pSurfaces = (skinSurface_t *) ri.Hunk_Alloc( sizeof(skinSurface_t), h_low );
		skin->pSurfaces[0].shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
		return hSkin;
	}

	// load and parse the skin file
    ri.FS_ReadFile( name, (void**)&text );
	if ( !text ) {
		return 0;
	}

	text_p = text;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( strstr( token, "tag_" ) ) {
			continue;
		}
		
		// parse the shader name
		token = CommaParse( &text_p );

//		surf = skin->surfaces[ skin->numSurfaces ] = (skinSurface_t*) ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		surf = &parseSurfaces[skin->numSurfaces]; 
		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text );


	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;		// use default skin
	}

// copy surfaces to skin
	skin->pSurfaces = ri.Hunk_Alloc( skin->numSurfaces * sizeof( skinSurface_t ), h_low );
	memcpy( skin->pSurfaces, parseSurfaces, skin->numSurfaces * sizeof( skinSurface_t ) );

    

	return hSkin;
}

/*
===============
R_InitSkins
===============
*/
void R_InitSkins( void )
{
	skin_t		*skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = (skin_t*) ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
//	skin->surfaces[0] = (skinSurface_t*) ri.Hunk_Alloc( sizeof( *skin->surfaces ), h_low );
//	skin->surfaces[0]->shader = tr.defaultShader;
	skin->pSurfaces = ri.Hunk_Alloc( sizeof( skinSurface_t ), h_low );
	skin->pSurfaces[0].shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t* R_GetSkinByHandle( qhandle_t hSkin )
{
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void R_SkinList_f( void )
{
	int			i, j;
	skin_t		*skin;

	ri.Printf (PRINT_ALL, "------------------\n");

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

//		ri.Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
        ri.Printf( PRINT_ALL, "%3i:%s (%d surfaces)\n", i, skin->name, skin->numSurfaces );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri.Printf( PRINT_ALL, "       %s = %s\n", 
				skin->pSurfaces[j].name, skin->pSurfaces[j].shader->name );
		}
	}
	ri.Printf (PRINT_ALL, "------------------\n");
}

