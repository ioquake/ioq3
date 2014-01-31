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
//
#include "ui_local.h"

#define SLUGROCK_FRAMEL	"menu/art/frame2_l"
#define SLUGROCK_FRAMER	"menu/art/frame1_r"
#define SLUGROCK_BACK0	"menu/art/back_0"
#define SLUGROCK_BACK1	"menu/art/back_1"

static char* slugrock_artlist[] =
{
	SLUGROCK_FRAMEL,	
	SLUGROCK_FRAMER,
	SLUGROCK_BACK0,
	SLUGROCK_BACK1,
	NULL
};

#define ID_BACK	 100

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menubitmap_s	back;
	char			info[MAX_INFO_STRING];
	int				numlines;
	menulist_s		g_forceWeaponMode;
} slugrock_t;
static slugrock_t	s_slugrock;

char *g_forceWeaponMode_names[] = { "no", "alternate", "random", "rl", "rg", "both", 0 };


/*
=================
SlugRock_Event
=================
*/
static void SlugRock_Event( void* ptr, int event )
{
	switch (((menucommon_s*)ptr)->id)
	{
		case ID_BACK:
			if (event != QM_ACTIVATED)
				break;

			UI_PopMenu();
			break;
	}
}

/*
=================
SlugRock_MenuDraw
=================
*/
static void SlugRock_MenuDraw( void )
{
	const char		*s;
	char			key[MAX_INFO_KEY];
	char			value[MAX_INFO_VALUE];
	int				i = 0, y;

	y = SCREEN_HEIGHT/2 - s_slugrock.numlines*(SMALLCHAR_HEIGHT)/2 - 20;
	s = s_slugrock.info;
	while ( s && i < s_slugrock.numlines ) {
		Info_NextPair( &s, key, value );
		if ( !key[0] ) {
			break;
		}

		Q_strcat( key, MAX_INFO_KEY, ":" ); 

		UI_DrawString(SCREEN_WIDTH*0.50 - 8,y,key,UI_RIGHT|UI_SMALLFONT,color_red);
		UI_DrawString(SCREEN_WIDTH*0.50 + 8,y,value,UI_LEFT|UI_SMALLFONT,text_color_normal);

		y += SMALLCHAR_HEIGHT;
		i++;
	}

	Menu_Draw( &s_slugrock.menu );
}

/*
=================
SlugRock_MenuKey
=================
*/
static sfxHandle_t SlugRock_MenuKey( int key )
{
	return ( Menu_DefaultKey( &s_slugrock.menu, key ) );
}

/*
=================
SlugRock_Cache
=================
*/
void SlugRock_Cache( void )
{
	int	i;

	// touch all our pics
	for (i=0; ;i++)
	{
		if (!slugrock_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(slugrock_artlist[i]);
	}
}

/*
=================
UI_SlugRockMenu
=================
*/
void UI_SlugRockMenu( void )
{
	const char		*s;
	char			key[MAX_INFO_KEY];
	char			value[MAX_INFO_VALUE];
	int				y;
	int				id;

	// zero set all our globals
	memset( &s_slugrock, 0 ,sizeof(slugrock_t) );

	SlugRock_Cache();

	s_slugrock.menu.draw			= SlugRock_MenuDraw;
	s_slugrock.menu.key				= SlugRock_MenuKey;
	s_slugrock.menu.wrapAround		= qtrue;
	s_slugrock.menu.fullscreen		= qtrue;

	s_slugrock.banner.generic.type  = MTYPE_BTEXT;
	s_slugrock.banner.generic.x		= 320;
	s_slugrock.banner.generic.y		= 16;
	s_slugrock.banner.string		= "SLUGROCK SETTINGS";
	s_slugrock.banner.color			= color_white;
	s_slugrock.banner.style			= UI_CENTER;

	s_slugrock.framel.generic.type  = MTYPE_BITMAP;
	s_slugrock.framel.generic.name  = SLUGROCK_FRAMEL;
	s_slugrock.framel.generic.flags = QMF_INACTIVE;
	s_slugrock.framel.generic.x		= 0;  
	s_slugrock.framel.generic.y		= 78;
	s_slugrock.framel.width  	    = 256;
	s_slugrock.framel.height  		= 329;

	s_slugrock.framer.generic.type  = MTYPE_BITMAP;
	s_slugrock.framer.generic.name  = SLUGROCK_FRAMER;
	s_slugrock.framer.generic.flags = QMF_INACTIVE;
	s_slugrock.framer.generic.x		= 376;
	s_slugrock.framer.generic.y		= 76;
	s_slugrock.framer.width  	    = 256;
	s_slugrock.framer.height  		= 334;

	s_slugrock.back.generic.type	 = MTYPE_BITMAP;
	s_slugrock.back.generic.name     = SLUGROCK_BACK0;
	s_slugrock.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_slugrock.back.generic.callback = SlugRock_Event;
	s_slugrock.back.generic.id		 = ID_BACK;
	s_slugrock.back.generic.x		 = 0;
	s_slugrock.back.generic.y		 = 480-64;
	s_slugrock.back.width  			 = 128;
	s_slugrock.back.height  		 = 64;
	s_slugrock.back.focuspic         = SLUGROCK_BACK1;

	id = 0;
	y = 100;
	s_slugrock.g_forceWeaponMode.generic.type		= MTYPE_SPINCONTROL;
	s_slugrock.g_forceWeaponMode.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_slugrock.g_forceWeaponMode.generic.x			= 312;
	s_slugrock.g_forceWeaponMode.generic.y			= y;
	s_slugrock.g_forceWeaponMode.generic.name		= "g_forceWeaponMode";
	s_slugrock.g_forceWeaponMode.generic.id			= id;
	s_slugrock.g_forceWeaponMode.generic.callback	= SlugRock_Event;
	s_slugrock.g_forceWeaponMode.itemnames			= g_forceWeaponMode_names;
	id++;
	y += SMALLCHAR_HEIGHT;

	trap_GetConfigString( CS_SERVERINFO, s_slugrock.info, MAX_INFO_STRING );

	s_slugrock.numlines = 0;
	s = s_slugrock.info;
	while ( s ) {
		Info_NextPair( &s, key, value );
		if ( !key[0] ) {
			break;
		}
		s_slugrock.numlines++;
	}

	if (s_slugrock.numlines > 16)
		s_slugrock.numlines = 16;

	Menu_AddItem( &s_slugrock.menu, (void*) &s_slugrock.banner );
	Menu_AddItem( &s_slugrock.menu, (void*) &s_slugrock.framel );
	Menu_AddItem( &s_slugrock.menu, (void*) &s_slugrock.framer );
	Menu_AddItem( &s_slugrock.menu, (void*) &s_slugrock.back );
	Menu_AddItem( &s_slugrock.menu, (void*) &s_slugrock.g_forceWeaponMode );

	UI_PushMenu( &s_slugrock.menu );
}


