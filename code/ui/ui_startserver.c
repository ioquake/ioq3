/*
####################### ####################### ####################### ####################### #######################

  StartServer2 by #@ ... should replace the vq3-startserver

####################### ####################### ####################### ####################### #######################
*/

#include "ui_local.h"

/*
BGP_STARTSERVERMAPS
BGP_STARTSERVERBOTS
*/

#define STARTSERVER_FIGHT0 "menu/startserver/fight0"
#define STARTSERVER_FIGHT1 "menu/startserver/fight1"
#define STARTSERVER_ARROWLEFT0 "menu/startserver/arrowleft0"
#define STARTSERVER_ARROWLEFT1 "menu/startserver/arrowleft1"
#define STARTSERVER_ARROWRIGHT0 "menu/startserver/arrowright0"
#define STARTSERVER_ARROWRIGHT1 "menu/startserver/arrowright1"
#define STARTSERVER_BACK0 "menu/BtnBack0"
#define STARTSERVER_BACK1 "menu/BtnBack1"
#define STARTSERVER_SELECTBOTS0 "menu/startserver/selectbot0"
#define STARTSERVER_SELECTBOTS1 "menu/startserver/selectbot1"
#define GAMESERVER_UNKNOWNMAP "menu/art/unknownmap"

#define MAX_MAPROWS 3
#define MAX_MAPCOLS 2
#define MAX_MAPSPERPAGE 6

#define MAX_SERVERSTEXT 8192

#define MAX_SERVERMAPS 64
#define MAX_NAMELENGTH 32 // old: 16 ... wop has some longer names(than vq3)
#define MAX_LONGNAMELENGTH 32
#define MAX_MAPSINLOOP 32

#define ID_GAMETYPE 10
#define ID_PICTURES 11 // 12, 13, 14, 15, 16
#define ID_PREVPAGE 17
#define ID_NEXTPAGE 18
#define ID_STARTSERVERBACK 19

#define ID_STARTSERVER 21
#define ID_SELECTBOTS 32

#define MAX_SELECTBOTS MAX_CLIENTS
#define MAX_BOTNAME 32

typedef struct {
	char name[MAX_BOTNAME];
	int team;
} selectbotinfo_t;

typedef struct {
	menuframework_s menu;

	menulist_s gametype;
	menubitmap_s mappics[MAX_MAPSPERPAGE];
	//	menubitmap_s	mapbuttons[MAX_MAPSPERPAGE];
	menubitmap1024s_s arrowleft;
	menubitmap1024s_s arrowright;

	menulist_s dedicated;
	menufield_s timelimit;
	menufield_s fraglimit;
	menufield_s maxclients;
	menuradiobutton_s friendlyfire;
	menuradiobutton_s instagib;
	menufield_s hostname;
	menuradiobutton_s pure;
	menufield_s StartLives;

	menubitmap_s selectbots;

	menubitmap_s back;
	menubitmap1024s_s fight;

	menubitmap_s item_null;

	qboolean multiplayer;
	int currentmap;
	int nummaps;
	int page;
	int maxpages;
	char maplist[MAX_SERVERMAPS][MAX_NAMELENGTH];
	char longnamemaplist[MAX_SERVERMAPS][MAX_LONGNAMELENGTH];
	int mapGamebits[MAX_SERVERMAPS];
	selectbotinfo_t selectbotinfos[MAX_SELECTBOTS];
	int botskill;
	int maploop[MAX_MAPSINLOOP];
	qhandle_t mapNumbers[10];
} startserver_t;

static startserver_t s_startserver;

// NOTE: The different order is intentional, thus the remapping
static const char *gametype_items[] = {GAMETYPE_NAME(GT_SPRAYFFA), GAMETYPE_NAME(GT_SPRAY),		GAMETYPE_NAME(GT_CTF),
									   GAMETYPE_NAME(GT_BALLOON),  GAMETYPE_NAME(GT_LPS),		GAMETYPE_NAME(GT_FFA),
									   GAMETYPE_NAME(GT_TEAM),	   GAMETYPE_NAME(GT_FREEZETAG), NULL};

static int gametype_remap[] = {GT_SPRAYFFA, GT_SPRAY, GT_CTF, GT_BALLOON, GT_LPS, GT_FFA, GT_TEAM, GT_FREEZETAG};
// NOTE: Maps g_gametype to gametype_items. Make sure these are defined for all GT_MAX_GAME_TYPE-1 gametypes
//       and remap to something reasonable for gametypes that do not have an entry in gametype_items.
static int gametype_remap2[] = {5, 5, 5, 0, 4, 6, 2, 1, 3, 3};
static const char *teamstrs[] = {"free", "Red", "Blue", "spectator", 0};

// static void UI_ServerOptionsMenu( qboolean multiplayer );

/*
=================
GametypeBits
=================
*/
static int GametypeBits(const char *string) {
	int bits;
	const char *p;
	const char *token;

	bits = 0;
	p = string;
	while (1) {
		token = COM_ParseExt(&p, qfalse);
		if (token[0] == 0) {
			break;
		}

		if (Q_stricmp(token, GAMETYPE_NAME_SHORT(GT_FFA)) == 0) {
			bits |= 1 << GT_FFA;
			continue;
		}

		if (Q_stricmp(token, "tourney") == 0) {
			bits |= 1 << GT_TOURNAMENT;
			continue;
		}

		if (Q_stricmp(token, "single") == 0) {
			bits |= 1 << GT_SINGLE_PLAYER;
			continue;
		}

		if (Q_stricmp(token, "team") == 0) {
			bits |= 1 << GT_TEAM;
			bits |= 1 << GT_FREEZETAG;
			continue;
		}

		if (Q_stricmp(token, GAMETYPE_NAME_SHORT(GT_FREEZETAG)) == 0) {
			bits |= 1 << GT_FREEZETAG;
			continue;
		}

		if (Q_stricmp(token, "ctf") == 0 || Q_stricmp(token, GAMETYPE_NAME_SHORT(GT_CTF)) == 0) {
			bits |= 1 << GT_CTF;
			continue;
		}
		if (Q_stricmp(token, "SyC_ffa") == 0) {
			bits |= 1 << GT_SPRAYFFA;
			continue;
		}
		if (Q_stricmp(token, "SyC_tp") == 0) {
			bits |= 1 << GT_SPRAY;
			continue;
		}
		if (Q_stricmp(token, GAMETYPE_NAME_SHORT(GT_LPS)) == 0) {
			bits |= 1 << GT_LPS;
			continue;
		}
		if (Q_stricmp(token, GAMETYPE_NAME_SHORT(GT_BALLOON)) == 0) {
			bits |= 1 << GT_BALLOON;
			continue;
		}
	}

	return bits;
}

/*
=================
StartServer_Update
=================
*/
static void StartServer_Update(void) {
	int i;
	int top;
	static char picname[MAX_MAPSPERPAGE][64];
	static char focuspicnames[MAX_MAPSPERPAGE][64];

	top = s_startserver.page * MAX_MAPSPERPAGE;

	for (i = 0; i < MAX_MAPSPERPAGE; i++) {
		if (top + i >= s_startserver.nummaps)
			break;

		// try to load special levelshot first, fallback to default
		Com_sprintf(picname[i], sizeof(picname[i]), "levelshots/%sA", s_startserver.maplist[top + i]);
		if (!trap_R_RegisterShaderNoMip(picname[i])) {
			Com_sprintf(picname[i], sizeof(picname[i]), "levelshots/%s", s_startserver.maplist[top + i]);
		}
		Com_sprintf(focuspicnames[i], sizeof(focuspicnames[i]), "levelshots/%sB", s_startserver.maplist[top + i]);

		s_startserver.mappics[i].generic.flags &= ~(QMF_HIGHLIGHT | QMF_INACTIVE | QMF_HIDDEN);
		s_startserver.mappics[i].generic.name = picname[i];
		s_startserver.mappics[i].shader = 0;

		s_startserver.mappics[i].focuspic = focuspicnames[i];
		s_startserver.mappics[i].focusshader = 0;

		// reset
		//		s_startserver.mapbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
		//		s_startserver.mapbuttons[i].generic.flags &= ~QMF_INACTIVE;
	}

	for (; i < MAX_MAPSPERPAGE; i++) {
		s_startserver.mappics[i].generic.flags &= ~QMF_HIGHLIGHT;
		s_startserver.mappics[i].generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		s_startserver.mappics[i].generic.name = NULL;
		s_startserver.mappics[i].shader = 0;

		// disable
		//		s_startserver.mapbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
		//		s_startserver.mapbuttons[i].generic.flags |= QMF_INACTIVE;
	}

	// no servers to start
	if (!s_startserver.nummaps) {
		//		s_startserver.next.generic.flags |= QMF_INACTIVE;//noch den fight-button einbinden ;)

		// set the map name
		//		strcpy( s_startserver.mapname.string, "NO MAPS FOUND" );
	} else if (s_startserver.currentmap != -1) {
		// set the highlight
		//		s_startserver.next.generic.flags &= ~QMF_INACTIVE;
		i = s_startserver.currentmap - top;
		if (i >= 0 && i < MAX_MAPSPERPAGE) {
			s_startserver.mappics[i].generic.flags |= QMF_HIGHLIGHT;
			//			s_startserver.mapbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
		}

		// set the map name
		//		strcpy( s_startserver.mapname.string, s_startserver.maplist[s_startserver.currentmap] );
	}

	//	Q_strupr( s_startserver.mapname.string );
}

/*
=================
StartServer_MapEvent
=================
*/
static void StartServer_MapEvent(void *ptr, int event) {
	int i, tmpi, currentmap;

	currentmap = (s_startserver.page * MAX_MAPSPERPAGE) + (((menucommon_s *)ptr)->id - ID_PICTURES);

	if (event == QM_GOTFOCUS) {
		s_startserver.currentmap = currentmap;

		return;
	}

	if (event != QM_ACTIVATED) {
		return;
	}

	for (i = 0; i < MAX_MAPSINLOOP; i++) {
		if (s_startserver.maploop[i] == currentmap) {
			s_startserver.maploop[i] = -1;
			for (; i < MAX_MAPSINLOOP; i++) {
				if (s_startserver.maploop[i + 1] == -1)
					break;

				tmpi = s_startserver.maploop[i + 1];
				s_startserver.maploop[i + 1] = s_startserver.maploop[i];
				s_startserver.maploop[i] = tmpi;
			}
			break;
		}

		if (s_startserver.maploop[i] == -1) {
			s_startserver.maploop[i] = currentmap;
			break;
		}
	}

	StartServer_Update();
}

/*
=================
StartServer_GametypeEvent
=================
*/
static void StartServer_GametypeEvent(void *ptr, int event) {
	int i, j;
	int count;
	int gamebits;
	int matchbits;
	const char *info;
	char tmpmaplist[MAX_MAPSINLOOP][MAX_NAMELENGTH];

	if (event != QM_ACTIVATED) {
		return;
	}

	memset(tmpmaplist, 0, sizeof(tmpmaplist));
	for (i = 0; i < MAX_MAPSINLOOP; i++) {
		if (s_startserver.maploop[i] < 0)
			break;

		Q_strncpyz(tmpmaplist[i], s_startserver.maplist[s_startserver.maploop[i]], sizeof(tmpmaplist[i]));
	}

	trap_Cvar_Set("g_gametype", va("%i", gametype_remap[s_startserver.gametype.curvalue]));
	trap_Cvar_Set("ui_createGametype", va("%i", gametype_remap[s_startserver.gametype.curvalue]));

	if (gametype_remap[s_startserver.gametype.curvalue] >= GT_TEAM)
		s_startserver.friendlyfire.generic.flags &= ~QMF_GRAYED;
	else
		s_startserver.friendlyfire.generic.flags |= QMF_GRAYED;

	if (gametype_remap[s_startserver.gametype.curvalue] == GT_LPS)
		s_startserver.StartLives.generic.flags &= ~QMF_GRAYED;
	else
		s_startserver.StartLives.generic.flags |= QMF_GRAYED;

	count = UI_GetNumArenas();
	s_startserver.nummaps = 0;
	matchbits = 1 << gametype_remap[s_startserver.gametype.curvalue];

	if (gametype_remap[s_startserver.gametype.curvalue] == GT_FFA) {
		matchbits |= (1 << GT_SINGLE_PLAYER);
	}

	for (i = 0; i < count; i++) {
		info = UI_GetArenaInfoByNumber(i);

		gamebits = GametypeBits(Info_ValueForKey(info, "type"));
		if (!(gamebits & matchbits)) {
			continue;
		}

		Q_strncpyz(s_startserver.maplist[s_startserver.nummaps], Info_ValueForKey(info, "map"), MAX_NAMELENGTH);
		Q_strncpyz(s_startserver.longnamemaplist[s_startserver.nummaps], Info_ValueForKey(info, "longname"),
				   MAX_LONGNAMELENGTH);
		// Q_strupr( s_startserver.maplist[s_startserver.nummaps] );
		s_startserver.mapGamebits[s_startserver.nummaps] = gamebits;
		++s_startserver.nummaps;
		if (s_startserver.nummaps >= MAX_SERVERMAPS)
			break;
	}

	s_startserver.maxpages = (s_startserver.nummaps + MAX_MAPSPERPAGE - 1) / MAX_MAPSPERPAGE;
	s_startserver.page = 0;
	s_startserver.currentmap = -1;

	for (i = 0; i < MAX_MAPSINLOOP; i++)
		s_startserver.maploop[i] = -1;

	for (i = 0, j = 0; i < MAX_MAPSINLOOP; ++i) {
		int k;

		if (tmpmaplist[i][0] == '\0')
			break;

		for (k = 0; k < s_startserver.nummaps; ++k) {
			if (s_startserver.maplist[k][0] == '\0')
				continue;

			if (!Q_stricmp(tmpmaplist[i], s_startserver.maplist[k])) {
				if (j >= MAX_MAPSINLOOP)
					goto DOUBLEBRAKE;
				s_startserver.maploop[j++] = k;
				break;
			}
		}
	}
DOUBLEBRAKE:

	StartServer_Update();
}

/*
=================
StartServer_MenuEvent
=================
*/
static void StartServer_MenuEvent(void *ptr, int event) {
	if (event != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s *)ptr)->id) {
	case ID_PREVPAGE:
		if (s_startserver.page > 0) {
			s_startserver.page--;
			StartServer_Update();
		}
		break;

	case ID_NEXTPAGE:
		if (s_startserver.page < s_startserver.maxpages - 1) {
			s_startserver.page++;
			StartServer_Update();
		}
		break;

	case ID_SELECTBOTS:
		UI_BotSelectMenu();
		break;

	case ID_STARTSERVER:
		trap_Cvar_SetValue("g_gameType", gametype_remap[s_startserver.gametype.curvalue]);

		{
			int timelimit;
			int fraglimit;
			int maxclients;
			int dedicated;
			int gametype;
			int friendlyfire;
			int instagib;
			int pure;
			//			int		skill;
			int n;
			char buf[64];

			if (s_startserver.maploop[0] == -1) {
				Com_Printf("You must select a map to start a server!\n");
				break;
			}

			timelimit = atoi(s_startserver.timelimit.field.buffer);
			fraglimit = atoi(s_startserver.fraglimit.field.buffer);
			maxclients = atoi(s_startserver.maxclients.field.buffer);
			dedicated = s_startserver.dedicated.curvalue;
			friendlyfire = s_startserver.friendlyfire.curvalue;
			instagib = s_startserver.instagib.curvalue;
			pure = s_startserver.pure.curvalue;
			gametype = gametype_remap[s_startserver.gametype.curvalue];
			//			skill		 = s_startserver.botSkill.curvalue + 1;

			// set maxclients
			//			for( n = 0, maxclients = 0; n < PLAYER_SLOTS; n++ ) {
			//				if( s_serveroptions.playerType[n].curvalue == 2 ) {
			//					continue;
			//				}
			//				if( (s_serveroptions.playerType[n].curvalue == 1) && (s_serveroptions.playerNameBuffers[n][0] ==
			//0) ) { 					continue;
			//				}
			//				maxclients++;
			//			}

			switch (gametype) {
			case GT_FFA:
			default:
				trap_Cvar_SetValue("ui_ffa_fraglimit", fraglimit);
				trap_Cvar_SetValue("ui_ffa_timelimit", timelimit);
				break;

			case GT_TOURNAMENT:
				trap_Cvar_SetValue("ui_tourney_fraglimit", fraglimit);
				trap_Cvar_SetValue("ui_tourney_timelimit", timelimit);
				break;

			case GT_TEAM:
				trap_Cvar_SetValue("ui_team_fraglimit", fraglimit);
				trap_Cvar_SetValue("ui_team_timelimit", timelimit);
				trap_Cvar_SetValue("ui_team_friendlt", friendlyfire);
				break;

			case GT_CTF:
				trap_Cvar_SetValue("ui_ctf_fraglimit", fraglimit);
				trap_Cvar_SetValue("ui_ctf_timelimit", timelimit);
				trap_Cvar_SetValue("ui_ctf_friendlt", friendlyfire);
				break;
			}

			trap_Cvar_SetValue("sv_maxclients", Com_Clamp(1, MAX_CLIENTS, maxclients));
			trap_Cvar_SetValue("dedicated", Com_Clamp(0, 2, dedicated));
			trap_Cvar_SetValue("timelimit", Com_Clamp(0, timelimit, timelimit));
			trap_Cvar_SetValue("pointlimit", Com_Clamp(0, fraglimit, fraglimit));
			//			trap_Cvar_SetValue ("capturelimit", Com_Clamp( 0, flaglimit, flaglimit ) );
			trap_Cvar_SetValue("g_friendlyfire", friendlyfire);
			trap_Cvar_SetValue("g_instaPad", instagib);
			if (gametype == GT_LPS)
				trap_Cvar_Set("g_LPS_startlives", s_startserver.StartLives.field.buffer);
			trap_Cvar_SetValue("sv_pure", pure);
			trap_Cvar_Set("sv_hostname", s_startserver.hostname.field.buffer);

			for (n = 0; n < MAX_MAPSINLOOP; n++) {
				char buffer[16];

				if (s_startserver.maploop[n] == -1)
					break;

				Com_sprintf(buffer, 16, "automaploop%i", n);

				//			trap_Cvar_Create(buffer,
				//				va("map %s; set nextmap vstr
				//automaploop%i",s_startserver.maplist[s_startserver.maploop[n]],s_startserver.maploop[n+1]==-1?0:n+1),
				//				CVAR_ARCHIVE|CVAR_ROM);
				trap_Cvar_Set(buffer, va("map %s; set nextmap vstr automaploop%i",
										 s_startserver.maplist[s_startserver.maploop[n]],
										 s_startserver.maploop[n + 1] == -1 ? 0 : n + 1));
			}

			trap_Cmd_ExecuteText(EXEC_APPEND, "wait ; wait ; vstr automaploop0\n");

			// the wait commands will allow the dedicated to take effect
			//			trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n",
			//s_startserver.maplist[s_startserver.currentmap] ) );

			// add bots
			trap_Cmd_ExecuteText(EXEC_APPEND, "wait 3\n");
			for (n = 0; n < MAX_SELECTBOTS && s_startserver.selectbotinfos[n].name[0] != '\0'; n++) {
				Com_sprintf(buf, sizeof(buf), "addbot %s %i %s\n", s_startserver.selectbotinfos[n].name,
							s_startserver.botskill, teamstrs[s_startserver.selectbotinfos[n].team]);
				trap_Cmd_ExecuteText(EXEC_APPEND, buf);
			}

			//			// set player's team
			//			if( dedicated == 0 && gametype >= GT_TEAM ) {
			//				trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait 5; team %s\n",
			//playerTeam_list[s_serveroptions.playerTeam[0].curvalue] ) );
			//			}
		}
		break;

	case ID_STARTSERVERBACK:
		UI_PopMenu();
		break;
	}
}

/*
===============
StartServer_LevelshotDraw
===============
*/
static void StartServer_LevelshotDraw(void *self) {
	menubitmap_s *b;
	int x;
	int y;
	int w;
	int h;
	int n;
	int i;

	b = (menubitmap_s *)self;

	if (!b->generic.name) {
		return;
	}

	if (b->generic.name && !b->shader) {
		b->shader = trap_R_RegisterShaderNoMip(b->generic.name);
		if (!b->shader && b->errorpic) {
			b->shader = trap_R_RegisterShaderNoMip(b->errorpic);
		}
	}

	if (b->focuspic && !b->focusshader) {
		b->focusshader = trap_R_RegisterShaderNoMip(b->focuspic);
	}

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h = b->height;
	if (Menu_ItemAtCursor(b->generic.parent) == b && b->focusshader) {
		UI_DrawHandlePic(x, y, w, h, b->focusshader);
	} else if (b->shader) {
		UI_DrawHandlePic(x, y, w, h, b->shader);
	}

	n = s_startserver.page * MAX_MAPSPERPAGE + b->generic.id - ID_PICTURES;

	for (i = 0; i < MAX_MAPSINLOOP; i++)
		if (s_startserver.maploop[i] == n) {
			if (i + 1 > 9) {
				UI_DrawHandlePic(x + 8, y + 6, 20, 35, s_startserver.mapNumbers[((i + 1) / 10) % 10]);
				UI_DrawHandlePic(x + 8 + 23, y + 6, 20, 35, s_startserver.mapNumbers[(i + 1) % 10]);
			} else
				UI_DrawHandlePic(x + 8, y + 6, 20, 35, s_startserver.mapNumbers[(i + 1) % 10]);

			break;
		}

	x = b->generic.x;
	y = b->generic.y + b->height;

	x += b->width / 2;
	y += 4;
	//	UI_DrawString( x, y, s_startserver.maplist[n], UI_CENTER|UI_SMALLFONT, color_white );
	if (Menu_ItemAtCursor(b->generic.parent) == b) {
		// Draw mapname in UPPERCASE, maplist remains untouched so "map mapname" still works
		// TODO: Only do strupr once and save into list while initialising
		char mapUpper[MAX_NAMELENGTH];
		Q_strncpyz(mapUpper, s_startserver.maplist[n], sizeof(mapUpper));
		Q_strupr(mapUpper);

		UI_DrawStringNS(x, y, mapUpper, UI_CENTER, 14.0f, color_white);
	} else
		UI_DrawStringNS(x, y, s_startserver.longnamemaplist[n], UI_CENTER, 14.0f, color_white);
}

static void StartServer_Draw(void) {
	// standard menu drawing
	Menu_Draw(&s_startserver.menu);
}

static void GameType_Draw(void *voidptr) {
	const float *color;
	int x, y;
	int style;
	qboolean focus;
	menulist_s *s = voidptr;

	x = s->generic.x;
	y = s->generic.y;

	style = UI_SMALLFONT;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	color = color_black;

	if (focus)
		UI_FillRect(s->generic.left, s->generic.top, ((s->generic.right - s->generic.left + 1) * 15) / 16,
					s->generic.bottom - s->generic.top + 1, listbar_color);

	UI_DrawStringNS(x, y, s->itemnames[s->curvalue], style, 15.0f, color);
}

/*
=================
StartServer_MenuInit
=================
*/

static const char *dedicated_list[] = {"No", "LAN", "Internet", 0};

static void StartServer_MenuInit(void) {
	int i;
	int x;
	int y;

	// zero set all our globals
	memset(&s_startserver, 0, sizeof(startserver_t));

	for (i = 0; i < MAX_MAPSINLOOP; i++)
		s_startserver.maploop[i] = -1;

	StartServer_Cache();

	s_startserver.menu.wrapAround = qtrue;
	s_startserver.menu.fullscreen = qtrue;
	s_startserver.menu.bgparts = BGP_STARTSERVERMAPS | BGP_SIMPLEBG;
	s_startserver.menu.draw = StartServer_Draw;

	s_startserver.gametype.generic.type = MTYPE_SPINCONTROL;
	//	s_startserver.gametype.generic.name		= "Game Type:";
	s_startserver.gametype.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT | QMF_FORCEDROPDOWN;
	s_startserver.gametype.generic.callback = StartServer_GametypeEvent;
	s_startserver.gametype.generic.id = ID_GAMETYPE;
	s_startserver.gametype.generic.x = 300;
	s_startserver.gametype.generic.y = 430;
	s_startserver.gametype.itemnames = gametype_items;
	s_startserver.gametype.generic.ownerdraw = GameType_Draw;
	s_startserver.gametype.curvalue = gametype_remap2[(int)Com_Clamp(
		0, (sizeof(gametype_remap2) / sizeof(gametype_remap[0]) - 1), trap_Cvar_VariableValue("ui_createGametype"))];
	// NOTE: Old behaviour: Always select sycffa
	//	s_startserver.gametype.curvalue			= 0;

	for (i = 0; i < MAX_MAPSPERPAGE; i++) {
		x = (i % MAX_MAPCOLS) * (128 + 10) + 60;
		y = (i / MAX_MAPCOLS /* % MAX_MAPROWS*/) * (110 + 8) + 40; // was 128+8

		s_startserver.mappics[i].generic.type = MTYPE_BITMAP;
		s_startserver.mappics[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_startserver.mappics[i].generic.id = ID_PICTURES + i;
		s_startserver.mappics[i].generic.callback = StartServer_MapEvent;
		s_startserver.mappics[i].generic.x = x;
		s_startserver.mappics[i].generic.y = y;
		s_startserver.mappics[i].width = 128;
		s_startserver.mappics[i].height = 96;
		//		s_startserver.mappics[i].focuspic			= GAMESERVER_SELECTED;
		s_startserver.mappics[i].errorpic = GAMESERVER_UNKNOWNMAP;
		s_startserver.mappics[i].generic.ownerdraw = StartServer_LevelshotDraw;
	}

	s_startserver.arrowleft.generic.type = MTYPE_BITMAP1024S;
	s_startserver.arrowleft.x = (308 - 10 - 99);
	s_startserver.arrowleft.y = 640;
	s_startserver.arrowleft.w = 99;
	s_startserver.arrowleft.h = 38;
	s_startserver.arrowleft.shader = trap_R_RegisterShaderNoMip(STARTSERVER_ARROWLEFT0);
	s_startserver.arrowleft.mouseovershader = trap_R_RegisterShaderNoMip(STARTSERVER_ARROWLEFT1);
	s_startserver.arrowleft.generic.callback = StartServer_MenuEvent;
	s_startserver.arrowleft.generic.id = ID_PREVPAGE;

	s_startserver.arrowright.generic.type = MTYPE_BITMAP1024S;
	s_startserver.arrowright.x = (308 + 10);
	s_startserver.arrowright.y = 640;
	s_startserver.arrowright.w = 98;
	s_startserver.arrowright.h = 38;
	s_startserver.arrowright.shader = trap_R_RegisterShaderNoMip(STARTSERVER_ARROWRIGHT0);
	s_startserver.arrowright.mouseovershader = trap_R_RegisterShaderNoMip(STARTSERVER_ARROWRIGHT1);
	s_startserver.arrowright.generic.callback = StartServer_MenuEvent;
	s_startserver.arrowright.generic.id = ID_NEXTPAGE;

	s_startserver.back.generic.type = MTYPE_BITMAP;
	s_startserver.back.generic.name = STARTSERVER_BACK0;
	s_startserver.back.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_startserver.back.generic.callback = StartServer_MenuEvent;
	s_startserver.back.generic.id = ID_STARTSERVERBACK;
	s_startserver.back.focuspic = STARTSERVER_BACK1;
	s_startserver.back.generic.x = 8;
	s_startserver.back.generic.y = 440;
	s_startserver.back.width = 80;
	s_startserver.back.height = 40;
	s_startserver.back.focuspicinstead = qtrue;

	// 502,212 -> name of the marked map

#define OPTIONS_XPOS 502

	y = 176;
	s_startserver.hostname.generic.type = MTYPE_FIELD;
	s_startserver.hostname.generic.name = "Host Name:";
	s_startserver.hostname.generic.flags = QMF_SMALLFONT;
	s_startserver.hostname.generic.x = OPTIONS_XPOS;
	s_startserver.hostname.generic.y = y;
	s_startserver.hostname.field.widthInChars = 12;
	s_startserver.hostname.field.maxchars = 64;

	//	if(0)//TODO: change me // s_serveroptions.gametype >= GT_TEAM )
	//	{
	y += BIGCHAR_HEIGHT + 2;
	s_startserver.friendlyfire.generic.type = MTYPE_RADIOBUTTON;
	s_startserver.friendlyfire.generic.flags = QMF_SMALLFONT | QMF_GRAYED;
	s_startserver.friendlyfire.generic.x = OPTIONS_XPOS;
	s_startserver.friendlyfire.generic.y = y;
	s_startserver.friendlyfire.generic.name = "Friendly Fire:";
	//	}
	y += BIGCHAR_HEIGHT + 2;
	s_startserver.instagib.generic.type = MTYPE_RADIOBUTTON;
	s_startserver.instagib.generic.flags = QMF_SMALLFONT;
	s_startserver.instagib.generic.x = OPTIONS_XPOS;
	s_startserver.instagib.generic.y = y;
	s_startserver.instagib.generic.name = "InstaPad:";

	y += BIGCHAR_HEIGHT + 2;
	s_startserver.pure.generic.type = MTYPE_RADIOBUTTON;
	s_startserver.pure.generic.flags = QMF_SMALLFONT;
	s_startserver.pure.generic.x = OPTIONS_XPOS;
	s_startserver.pure.generic.y = y;
	s_startserver.pure.generic.name = "Pure Server:";
	s_startserver.pure.curvalue = Com_Clamp(0, 1, trap_Cvar_VariableValue("sv_pure"));

	y += BIGCHAR_HEIGHT + 2;
	s_startserver.dedicated.generic.type = MTYPE_SPINCONTROL;
	//	s_startserver.dedicated.generic.id			= ID_DEDICATED;
	s_startserver.dedicated.generic.flags = QMF_SMALLFONT;
	//	s_startserver.dedicated.generic.callback	= ServerOptions_Event;
	s_startserver.dedicated.generic.x = OPTIONS_XPOS;
	s_startserver.dedicated.generic.y = y;
	s_startserver.dedicated.generic.name = "Dedicated:";
	s_startserver.dedicated.itemnames = dedicated_list;

	y += BIGCHAR_HEIGHT + 2;
	s_startserver.fraglimit.generic.type = MTYPE_FIELD;
	s_startserver.fraglimit.generic.name = "Point Limit:";
	s_startserver.fraglimit.generic.flags = QMF_NUMBERSONLY | QMF_SMALLFONT;
	s_startserver.fraglimit.generic.x = OPTIONS_XPOS;
	s_startserver.fraglimit.generic.y = y;
	s_startserver.fraglimit.field.widthInChars = 3;
	s_startserver.fraglimit.field.maxchars = 3;

	y += BIGCHAR_HEIGHT + 2;
	s_startserver.timelimit.generic.type = MTYPE_FIELD;
	s_startserver.timelimit.generic.name = "Time Limit:";
	s_startserver.timelimit.generic.flags = QMF_NUMBERSONLY | QMF_SMALLFONT;
	s_startserver.timelimit.generic.x = OPTIONS_XPOS;
	s_startserver.timelimit.generic.y = y;
	s_startserver.timelimit.field.widthInChars = 3;
	s_startserver.timelimit.field.maxchars = 3;

	y += BIGCHAR_HEIGHT + 2;
	s_startserver.maxclients.generic.type = MTYPE_FIELD;
	s_startserver.maxclients.generic.name = "Player Slots:";
	s_startserver.maxclients.generic.flags = (QMF_NUMBERSONLY | QMF_SMALLFONT);
	s_startserver.maxclients.generic.x = OPTIONS_XPOS;
	s_startserver.maxclients.generic.y = y;
	s_startserver.maxclients.field.widthInChars = 2;
	s_startserver.maxclients.field.maxchars = 2;

	y += BIGCHAR_HEIGHT + 2;
	s_startserver.StartLives.generic.type = MTYPE_FIELD;
	s_startserver.StartLives.generic.name = "Start Lives:";
	s_startserver.StartLives.generic.flags = QMF_NUMBERSONLY | QMF_SMALLFONT;
	s_startserver.StartLives.generic.x = OPTIONS_XPOS;
	s_startserver.StartLives.generic.y = y;
	//	s_startserver.StartLives.generic.statusbar  = ServerOptions_StatusBar;
	s_startserver.StartLives.field.widthInChars = 3;
	s_startserver.StartLives.field.maxchars = 3;

	y += BIGCHAR_HEIGHT + 4;
	s_startserver.selectbots.generic.type = MTYPE_BITMAP;
	s_startserver.selectbots.generic.name = STARTSERVER_SELECTBOTS0;
	s_startserver.selectbots.generic.flags = QMF_LEFT_JUSTIFY | QMF_HIGHLIGHT_IF_FOCUS;
	s_startserver.selectbots.generic.callback = StartServer_MenuEvent;
	s_startserver.selectbots.generic.id = ID_SELECTBOTS;
	s_startserver.selectbots.focuspic = STARTSERVER_SELECTBOTS1;
	s_startserver.selectbots.generic.x = OPTIONS_XPOS - 59;
	s_startserver.selectbots.generic.y = y;
	s_startserver.selectbots.width = 118;
	s_startserver.selectbots.height = 22;
	s_startserver.selectbots.focuspicinstead = qtrue;

	s_startserver.fight.generic.type = MTYPE_BITMAP1024S;
	s_startserver.fight.x = 870; // 845;
	s_startserver.fight.y = 655; // 658;
	s_startserver.fight.w = 135;
	s_startserver.fight.h = 97;
	s_startserver.fight.shader = trap_R_RegisterShaderNoMip(STARTSERVER_FIGHT0);
	s_startserver.fight.mouseovershader = trap_R_RegisterShaderNoMip(STARTSERVER_FIGHT1);
	s_startserver.fight.generic.callback = StartServer_MenuEvent;
	s_startserver.fight.generic.id = ID_STARTSERVER;

	s_startserver.item_null.generic.type = MTYPE_BITMAP;
	s_startserver.item_null.generic.flags = QMF_LEFT_JUSTIFY | QMF_MOUSEONLY | QMF_SILENT;
	s_startserver.item_null.generic.x = 0;
	s_startserver.item_null.generic.y = 0;
	s_startserver.item_null.width = 640;
	s_startserver.item_null.height = 480;

	Menu_AddItem(&s_startserver.menu, &s_startserver.item_null);
	Menu_AddItem(&s_startserver.menu, &s_startserver.gametype);
	s_startserver.gametype.generic.right -= SMALLCHAR_WIDTH;
	s_startserver.gametype.generic.left += SMALLCHAR_WIDTH;
	for (i = 0; i < MAX_MAPSPERPAGE; i++) {
		Menu_AddItem(&s_startserver.menu, &s_startserver.mappics[i]);
	}

	Menu_AddItem(&s_startserver.menu, &s_startserver.arrowleft);
	Menu_AddItem(&s_startserver.menu, &s_startserver.arrowright);
	Menu_AddItem(&s_startserver.menu, &s_startserver.back);
	Menu_AddItem(&s_startserver.menu, &s_startserver.fraglimit);
	Menu_AddItem(&s_startserver.menu, &s_startserver.timelimit);
	Menu_AddItem(&s_startserver.menu, &s_startserver.maxclients);
	Menu_AddItem(&s_startserver.menu, &s_startserver.friendlyfire);
	Menu_AddItem(&s_startserver.menu, &s_startserver.instagib);
	Menu_AddItem(&s_startserver.menu, &s_startserver.pure);
	Menu_AddItem(&s_startserver.menu, &s_startserver.dedicated);
	Menu_AddItem(&s_startserver.menu, &s_startserver.hostname);
	Menu_AddItem(&s_startserver.menu, &s_startserver.StartLives);
	Menu_AddItem(&s_startserver.menu, &s_startserver.selectbots);

	Menu_AddItem(&s_startserver.menu, &s_startserver.fight);

	Q_strncpyz(s_startserver.hostname.field.buffer, UI_Cvar_VariableString("sv_hostname"),
			   sizeof(s_startserver.hostname.field.buffer));
	Q_strncpyz(s_startserver.timelimit.field.buffer, UI_Cvar_VariableString("timelimit"),
			   sizeof(s_startserver.timelimit.field.buffer));
	Q_strncpyz(s_startserver.fraglimit.field.buffer, UI_Cvar_VariableString("pointlimit"),
			   sizeof(s_startserver.fraglimit.field.buffer));
	Q_strncpyz(s_startserver.maxclients.field.buffer, UI_Cvar_VariableString("sv_maxclients"),
			   sizeof(s_startserver.maxclients.field.buffer));
	Q_strncpyz(s_startserver.StartLives.field.buffer, UI_Cvar_VariableString("g_LPS_startlives"),
			   sizeof(s_startserver.StartLives.field.buffer));

	StartServer_GametypeEvent(NULL, QM_ACTIVATED);
}

/*
=================
StartServer_Cache
=================
*/
void StartServer_Cache(void) {
	int i;
	const char *info;
	char picname[MAX_QPATH];

	trap_R_RegisterShaderNoMip(STARTSERVER_FIGHT0);
	trap_R_RegisterShaderNoMip(STARTSERVER_FIGHT1);
	trap_R_RegisterShaderNoMip(STARTSERVER_ARROWLEFT0);
	trap_R_RegisterShaderNoMip(STARTSERVER_ARROWLEFT1);
	trap_R_RegisterShaderNoMip(STARTSERVER_ARROWRIGHT0);
	trap_R_RegisterShaderNoMip(STARTSERVER_ARROWRIGHT1);
	trap_R_RegisterShaderNoMip(STARTSERVER_BACK0);
	trap_R_RegisterShaderNoMip(STARTSERVER_BACK1);
	trap_R_RegisterShaderNoMip(STARTSERVER_SELECTBOTS0);
	trap_R_RegisterShaderNoMip(STARTSERVER_SELECTBOTS1);
	trap_R_RegisterShaderNoMip(GAMESERVER_UNKNOWNMAP);

	for (i = 0; i < 10; i++)
		s_startserver.mapNumbers[i] = trap_R_RegisterShaderNoMip(va("menu/startserver/%i", i));

	s_startserver.nummaps = UI_GetNumArenas();

	for (i = 0; i < s_startserver.nummaps; i++) {
		info = UI_GetArenaInfoByNumber(i);

		Q_strncpyz(s_startserver.maplist[i], Info_ValueForKey(info, "map"), sizeof(s_startserver.maplist[i]));
		s_startserver.mapGamebits[i] = GametypeBits(Info_ValueForKey(info, "type"));

		Com_sprintf(picname, sizeof(picname), "levelshots/%sA", s_startserver.maplist[i]);
		if (!trap_R_RegisterShaderNoMip(picname)) {
			Com_sprintf(picname, sizeof(picname), "levelshots/%s", s_startserver.maplist[i]);
			trap_R_RegisterShaderNoMip(picname);
		}
		Com_sprintf(picname, sizeof(picname), "levelshots/%sB", s_startserver.maplist[i]);
		trap_R_RegisterShaderNoMip(picname);
	}
	s_startserver.maxpages = ((s_startserver.nummaps + MAX_MAPSPERPAGE - 1) / MAX_MAPSPERPAGE);
}

/*
=================
UI_StartServerMenu
=================
*/
void UI_StartServerMenu(qboolean multiplayer) {
	StartServer_MenuInit();
	s_startserver.multiplayer = multiplayer;
	UI_PushMenu(&s_startserver.menu);
}

/*
=============================================================================

BOT SELECT MENU *****

=============================================================================
*/

#define BOTSELECT_BACK0 "menu/BtnBack0"
#define BOTSELECT_BACK1 "menu/BtnBack1"
#define BOTSELECT_ACCEPT0 "menu/art/accept_0"
#define BOTSELECT_ACCEPT1 "menu/art/accept_1"
#define BOTSELECT_SELECT "menu/art/opponents_select"
#define BOTSELECT_SELECTED "menu/art/opponents_selected"
#define BOTSELECT_ARROWS "menu/art/gs_arrows_0"
#define BOTSELECT_ARROWSL "menu/art/gs_arrows_l"
#define BOTSELECT_ARROWSR "menu/art/gs_arrows_r"

#define PLAYERGRID_COLS 4 // 3
#define PLAYERGRID_ROWS 4
#define MAX_MODELSPERPAGE (PLAYERGRID_ROWS * PLAYERGRID_COLS)

#define MAX_SELECTLISTBOTS 12 // 10

typedef struct {
	menuframework_s menu;

	menutext_s banner;

	menubitmap_s pics[MAX_MODELSPERPAGE];
	menubitmap_s picbuttons[MAX_MODELSPERPAGE];
	menutext_s picnames[MAX_MODELSPERPAGE];
	menubitmap1024s_s arrowleft;
	menubitmap1024s_s arrowright;

	menutext_s selectedbotnames[MAX_SELECTLISTBOTS];
	menutext_s selectedbotteams[MAX_SELECTLISTBOTS];

	menubitmap1024s_s arrowup;
	menubitmap1024s_s arrowdown;

	menulist_s BotSkill;
	menutext_s slotsleft;

	menubitmap_s back;

	int topbot; // firstbot in list ;)
	int numBots;
	int modelpage;
	int numpages;
	int selectedmodel;
	int sortedBotNums[MAX_BOTS];
	char boticons[MAX_MODELSPERPAGE][MAX_QPATH];
	char botnames[MAX_MODELSPERPAGE][16];
} botSelectInfo_t;

static botSelectInfo_t botSelectInfo;

static const char *botSkill_list[] = {"Kindergarden", "Flower power",	   "Mosquito Bite",
									  "Peacemaker",	  "Brutal Blue Noses", 0};

/*
=================
UI_BotSelectMenu_BuildList
=================
*/
static void UI_BotSelectMenu_BuildList(void) {
	int n;

	botSelectInfo.modelpage = 0;
	botSelectInfo.numBots = UI_GetNumBots();
	botSelectInfo.numpages = botSelectInfo.numBots / MAX_MODELSPERPAGE;
	if (botSelectInfo.numBots % MAX_MODELSPERPAGE) {
		botSelectInfo.numpages++;
	}

	// initialize the array
	for (n = 0; n < botSelectInfo.numBots; n++) {
		botSelectInfo.sortedBotNums[n] = n;
	}

	// now sort it
	qsort(botSelectInfo.sortedBotNums, botSelectInfo.numBots, sizeof(botSelectInfo.sortedBotNums[0]), BotListCompare);
}

/*
=================
ServerPlayerIcon
=================
*/
static void ServerPlayerIcon(const char *modelAndSkin, char *iconName, int iconNameMaxSize) {
	char *skin;
	char model[MAX_QPATH];

	Q_strncpyz(model, modelAndSkin, sizeof(model));
	skin = strrchr(model, '/');
	if (skin) {
		*skin++ = '\0';
	} else {
		skin = "default";
	}

	Com_sprintf(iconName, iconNameMaxSize, "models/wop_players/%s/icon_%s", model, skin);

	if (!trap_R_RegisterShaderNoMip(iconName) && Q_stricmp(skin, "default") != 0) {
		Com_sprintf(iconName, iconNameMaxSize, "models/wop_players/%s/icon_default", model);
	}
}

/*
=================
UI_BotSelectMenu_UpdateGrid
=================
*/
static void UI_BotSelectMenu_UpdateGrid(void) {
	const char *info;
	int i;
	int j;

	j = botSelectInfo.modelpage * MAX_MODELSPERPAGE;
	for (i = 0; i < (PLAYERGRID_ROWS * PLAYERGRID_COLS); i++, j++) {
		if (j < botSelectInfo.numBots) {
			info = UI_GetBotInfoByNumber(botSelectInfo.sortedBotNums[j]);
			ServerPlayerIcon(Info_ValueForKey(info, "model"), botSelectInfo.boticons[i], MAX_QPATH);
			Q_strncpyz(botSelectInfo.botnames[i], Info_ValueForKey(info, "name"), 16);
			Q_CleanStr(botSelectInfo.botnames[i]);
			botSelectInfo.pics[i].generic.name = botSelectInfo.boticons[i];
			botSelectInfo.picnames[i].color = color_orange;
			botSelectInfo.picbuttons[i].generic.flags &= ~QMF_INACTIVE;
		} else {
			// dead slot
			botSelectInfo.pics[i].generic.name = NULL;
			botSelectInfo.picbuttons[i].generic.flags |= QMF_INACTIVE;
			botSelectInfo.botnames[i][0] = 0;
		}

		botSelectInfo.pics[i].generic.flags &= ~QMF_HIGHLIGHT;
		botSelectInfo.pics[i].shader = 0;
		botSelectInfo.picbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
	}

	// set selected model
	i = botSelectInfo.selectedmodel % MAX_MODELSPERPAGE;
	botSelectInfo.pics[i].generic.flags |= QMF_HIGHLIGHT;
	botSelectInfo.picbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;

	if (botSelectInfo.numpages > 1) {
		if (botSelectInfo.modelpage > 0) {
			botSelectInfo.arrowleft.generic.flags &= ~QMF_INACTIVE;
		} else {
			botSelectInfo.arrowleft.generic.flags |= QMF_INACTIVE;
		}

		if (botSelectInfo.modelpage < (botSelectInfo.numpages - 1)) {
			botSelectInfo.arrowright.generic.flags &= ~QMF_INACTIVE;
		} else {
			botSelectInfo.arrowright.generic.flags |= QMF_INACTIVE;
		}
	} else {
		// hide left/right markers
		botSelectInfo.arrowleft.generic.flags |= QMF_INACTIVE;
		botSelectInfo.arrowright.generic.flags |= QMF_INACTIVE;
	}
}

#if 0
/*
=================
UI_BotSelectMenu_Default
=================
*/
static void UI_BotSelectMenu_Default( char *bot ) {
	const char	*botInfo;
	const char	*test;
	int			n;
	int			i;

	for( n = 0; n < botSelectInfo.numBots; n++ ) {
		botInfo = UI_GetBotInfoByNumber( n );
		test = Info_ValueForKey( botInfo, "name" );
		if( Q_stricmp( bot, test ) == 0 ) {
			break;
		}
	}
	if( n == botSelectInfo.numBots ) {
		botSelectInfo.selectedmodel = 0;
		return;
	}

	for( i = 0; i < botSelectInfo.numBots; i++ ) {
		if( botSelectInfo.sortedBotNums[i] == n ) {
			break;
		}
	}
	if( i == botSelectInfo.numBots ) {
		botSelectInfo.selectedmodel = 0;
		return;
	}

	botSelectInfo.selectedmodel = i;
}
#endif

/*
=================
UI_BotSelectMenu_LeftEvent
=================
*/
static void UI_BotSelectMenu_LeftEvent(void *ptr, int event) {
	if (event != QM_ACTIVATED) {
		return;
	}
	if (botSelectInfo.modelpage > 0) {
		botSelectInfo.modelpage--;
		botSelectInfo.selectedmodel = botSelectInfo.modelpage * MAX_MODELSPERPAGE;
		UI_BotSelectMenu_UpdateGrid();
	}
}

/*
=================
UI_BotSelectMenu_RightEvent
=================
*/
static void UI_BotSelectMenu_RightEvent(void *ptr, int event) {
	if (event != QM_ACTIVATED) {
		return;
	}
	if (botSelectInfo.modelpage < botSelectInfo.numpages - 1) {
		botSelectInfo.modelpage++;
		botSelectInfo.selectedmodel = botSelectInfo.modelpage * MAX_MODELSPERPAGE;
		UI_BotSelectMenu_UpdateGrid();
	}
}

int getNumSelectedBots(void) {
	int i, numBots = 0;

	for (i = 0; i < MAX_SELECTBOTS; i++) {
		if (s_startserver.selectbotinfos[i].name[0] != '\0') {
			numBots++;
		}
	}

	return numBots;
}

int getSlotsLeft(void) {
	int maxclients = atoi(s_startserver.maxclients.field.buffer);

	if (s_startserver.dedicated.curvalue == 0) {
		// running locally
		return (maxclients - getNumSelectedBots() - 1);
	} else {
		return (maxclients - getNumSelectedBots());
	}
}

void UI_BotSelectMenu_UpdateList(void) {
	int i, j;
	char *tmpptr;

	j = 0;
	for (i = 0; i < MAX_SELECTLISTBOTS; i++) {
		tmpptr = "";
		for (; j < MAX_SELECTBOTS; j++) {
			if (j < botSelectInfo.topbot)
				continue;

			if (s_startserver.selectbotinfos[j].name[0] != '\0') {
				tmpptr = s_startserver.selectbotinfos[j].name;
				j++; // damit es +1 ist wenn die äußere schleife looped
				break;
			}
		}

		if (*tmpptr != '\0') {
			botSelectInfo.selectedbotnames[i].string = tmpptr;
			botSelectInfo.selectedbotnames[i].generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);
			if (gametype_remap[s_startserver.gametype.curvalue] >= GT_TEAM) {
				// this happens when switching gametype
				if (s_startserver.selectbotinfos[j - 1].team == TEAM_FREE) {
					s_startserver.selectbotinfos[j - 1].team = TEAM_RED;
				}

				botSelectInfo.selectedbotteams[i].string = (char *)teamstrs[s_startserver.selectbotinfos[j - 1].team];
				botSelectInfo.selectedbotteams[i].generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);
				botSelectInfo.selectedbotteams[i].color =
					s_startserver.selectbotinfos[j - 1].team == TEAM_RED ? color_red : color_blue;
			} else {
				char *skinname;
				const float *skincolor; // FIXME: vec4_t

				switch (s_startserver.selectbotinfos[j - 1].team) {
				default:
				case TEAM_FREE:
					skinname = "Default";
					skincolor = color_white;
					break;
				case TEAM_RED:
					skinname = "Red";
					skincolor = color_red;
					break;
				case TEAM_BLUE:
					skinname = "Blue";
					skincolor = color_blue;
					break;
				}

				botSelectInfo.selectedbotteams[i].generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);

				botSelectInfo.selectedbotteams[i].string = skinname;
				botSelectInfo.selectedbotteams[i].color = skincolor;
			}
		} else {
			botSelectInfo.selectedbotnames[i].generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
			botSelectInfo.selectedbotteams[i].generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		}
	}
	botSelectInfo.slotsleft.string = va("Open Slots: %2d", getSlotsLeft());
}

/*
=================
UI_BotSelectMenu_BotEvent
=================
*/
static void UI_BotSelectMenu_BotEvent(void *ptr, int event) {
	int i, j;

	if (event != QM_ACTIVATED) {
		return;
	}

	for (i = 0; i < (PLAYERGRID_ROWS * PLAYERGRID_COLS); i++) {
		botSelectInfo.pics[i].generic.flags &= ~QMF_HIGHLIGHT;
		botSelectInfo.picbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
	}

	// set selected
	i = ((menucommon_s *)ptr)->id;
	botSelectInfo.pics[i].generic.flags |= QMF_HIGHLIGHT;
	botSelectInfo.picbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
	botSelectInfo.selectedmodel = botSelectInfo.modelpage * MAX_MODELSPERPAGE + i;

	if (getSlotsLeft() <= 0) {
		return;
	}

	for (j = 0; j < MAX_SELECTBOTS; j++) {
		if (s_startserver.selectbotinfos[j].name[0] != '\0')
			continue;

		Q_strncpyz(s_startserver.selectbotinfos[j].name, botSelectInfo.botnames[i], MAX_BOTNAME);

		if (gametype_remap[s_startserver.gametype.curvalue] >= GT_TEAM) {
			s_startserver.selectbotinfos[j].team = TEAM_RED;
		} else {
			s_startserver.selectbotinfos[j].team = TEAM_FREE;
		}

		break;
	}
	UI_BotSelectMenu_UpdateList();
}

/*
=================
UI_BotSelectMenu_BackEvent
=================
*/
static void UI_BotSelectMenu_BackEvent(void *ptr, int event) {
	if (event != QM_ACTIVATED) {
		return;
	}
	UI_PopMenu();
}

/*
=================
UI_BotSelectMenu_Cache
=================
*/
void UI_BotSelectMenu_Cache(void) {
	trap_R_RegisterShaderNoMip(BOTSELECT_BACK0);
	trap_R_RegisterShaderNoMip(BOTSELECT_BACK1);
	trap_R_RegisterShaderNoMip(BOTSELECT_ACCEPT0);
	trap_R_RegisterShaderNoMip(BOTSELECT_ACCEPT1);
	trap_R_RegisterShaderNoMip(BOTSELECT_SELECT);
	trap_R_RegisterShaderNoMip(BOTSELECT_SELECTED);
	trap_R_RegisterShaderNoMip(BOTSELECT_ARROWS);
	trap_R_RegisterShaderNoMip(BOTSELECT_ARROWSL);
	trap_R_RegisterShaderNoMip(BOTSELECT_ARROWSR);
}

/*
=================
UI_BotSelectMenu_SelectBotName
=================
*/
static void UI_BotSelectMenu_SelectBotName(void *ptr, int event) {
	selectbotinfo_t tmpcpy;
	int i;

	if (event != QM_ACTIVATED)
		return;

	i = ((menucommon_s *)ptr)->id + botSelectInfo.topbot;
	Q_strncpyz(s_startserver.selectbotinfos[i].name, "", MAX_BOTNAME);
	for (; i < MAX_SELECTBOTS - 1; i++) {
		if (s_startserver.selectbotinfos[i + 1].name[0] == '\0')
			break;

		memcpy(&tmpcpy, &s_startserver.selectbotinfos[i], sizeof(selectbotinfo_t));
		memcpy(&s_startserver.selectbotinfos[i], &s_startserver.selectbotinfos[i + 1], sizeof(selectbotinfo_t));
		memcpy(&s_startserver.selectbotinfos[i + 1], &tmpcpy, sizeof(selectbotinfo_t));
	}
	UI_BotSelectMenu_UpdateList();
}

/*
#######################
UI_BotSelectMenu_SelectBotTeam
#######################
*/
static void UI_BotSelectMenu_SelectBotTeam(void *ptr, int event) {
	int i;
	if (event != QM_ACTIVATED)
		return;

	i = ((menucommon_s *)ptr)->id + botSelectInfo.topbot;
	if (gametype_remap[s_startserver.gametype.curvalue] >= GT_TEAM) {
		s_startserver.selectbotinfos[i].team =
			(s_startserver.selectbotinfos[i].team == TEAM_RED ? TEAM_BLUE : TEAM_RED);
	} else {
		// NOTE: We're abusing the team field for the bot's skin
		int t = s_startserver.selectbotinfos[i].team;
		t++;
		if (t > TEAM_BLUE) {
			t = TEAM_FREE;
		}
		s_startserver.selectbotinfos[i].team = t;
	}

	UI_BotSelectMenu_UpdateList();
}

/*
#######################
UI_BotSelectMenu_ListUp
#######################
*/
static void UI_BotSelectMenu_ListUp(void *ptr, int event) {
	if (event != QM_ACTIVATED)
		return;

	if (botSelectInfo.topbot > 0)
		botSelectInfo.topbot--;

	UI_BotSelectMenu_UpdateList();
}

/*
#######################
UI_BotSelectMenu_ListDown
#######################
*/
static void UI_BotSelectMenu_ListDown(void *ptr, int event) {
	if (event != QM_ACTIVATED)
		return;

	if (botSelectInfo.topbot < MAX_SELECTBOTS - MAX_SELECTLISTBOTS)
		botSelectInfo.topbot++;

	UI_BotSelectMenu_UpdateList();
}

/*
#######################
UI_BotSelectMenu_BotSkillChanged
#######################
*/
static void UI_BotSelectMenu_BotSkillChanged(void *ptr, int event) {
	if (event != QM_ACTIVATED)
		return;

	s_startserver.botskill = ((menulist_s *)ptr)->curvalue + 1;
}

static void UI_BotSelectMenu_DrawBotIcon(void *self) {
	menubitmap_s *b = (menubitmap_s *)self;
	float x;
	float y;
	float w;
	float h;
	//	vec4_t	tempcolor;
	//	float*	color;

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h = b->height;

	// used to refresh shader
	if (b->generic.name && !b->shader) {
		b->shader = trap_R_RegisterShaderNoMip(b->generic.name);
		if (!b->shader && b->errorpic)
			b->shader = trap_R_RegisterShaderNoMip(b->errorpic);
	}

	//	if (b->focuspic && !b->focusshader)
	//		b->focusshader = trap_R_RegisterShaderNoMip( b->focuspic );

	if (b->shader) // wenn kein icon da ist soll auch kein shadow da sein
	{
		if (!(Menu_ItemAtCursor(b->generic.parent) == b)) {
			//		UI_DrawHandlePic( x, y, w, h, b->focusshader );
			UI_DrawNamedPic(x, y, w + 8, h + 8, "menu/player/micon_shadow");
		}

		UI_DrawHandlePic(x, y, w, h, b->shader);
	}
}

/*
#######################
UI_BotSelectMenu_Init
#######################
*/
static void UI_BotSelectMenu_Init(void) {
	int i, j, k;
	int x, y;

	memset(&botSelectInfo, 0, sizeof(botSelectInfo));
	botSelectInfo.menu.wrapAround = qtrue;
	botSelectInfo.menu.fullscreen = qtrue;
	botSelectInfo.menu.bgparts = BGP_STARTSERVERBOTS | BGP_SIMPLEBG;

	UI_BotSelectMenu_Cache();

	y = 54;
	for (i = 0, k = 0; i < PLAYERGRID_ROWS; i++) {
		x = 51;
		for (j = 0; j < PLAYERGRID_COLS; j++, k++) {
			botSelectInfo.pics[k].generic.type = MTYPE_BITMAP;
			//			botSelectInfo.pics[k].generic.flags				= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
			botSelectInfo.pics[k].generic.flags = QMF_LEFT_JUSTIFY;
			botSelectInfo.pics[k].generic.x = x;
			botSelectInfo.pics[k].generic.y = y;
			botSelectInfo.pics[k].generic.name = botSelectInfo.boticons[k];
			botSelectInfo.pics[k].width = 64;
			botSelectInfo.pics[k].height = 64;
			botSelectInfo.pics[k].focuspic = BOTSELECT_SELECTED;
			botSelectInfo.pics[k].focuscolor = colorRed;
			botSelectInfo.pics[k].generic.callback = UI_BotSelectMenu_BotEvent;
			botSelectInfo.pics[k].generic.id = k;
			botSelectInfo.pics[k].generic.ownerdraw = UI_BotSelectMenu_DrawBotIcon;

			botSelectInfo.picbuttons[k].generic.type = MTYPE_BITMAP;
			botSelectInfo.picbuttons[k].generic.flags = QMF_LEFT_JUSTIFY | QMF_NODEFAULTINIT | QMF_PULSEIFFOCUS;
			botSelectInfo.picbuttons[k].generic.callback = UI_BotSelectMenu_BotEvent;
			botSelectInfo.picbuttons[k].generic.id = k;
			botSelectInfo.picbuttons[k].generic.x = x - 16;
			botSelectInfo.picbuttons[k].generic.y = y - 16;
			botSelectInfo.picbuttons[k].generic.left = x;
			botSelectInfo.picbuttons[k].generic.top = y;
			botSelectInfo.picbuttons[k].generic.right = x + 64;
			botSelectInfo.picbuttons[k].generic.bottom = y + 64;
			botSelectInfo.picbuttons[k].width = 128;
			botSelectInfo.picbuttons[k].height = 128;
			botSelectInfo.picbuttons[k].focuspic = BOTSELECT_SELECT;
			botSelectInfo.picbuttons[k].focuscolor = colorRed;

			botSelectInfo.picnames[k].generic.type = MTYPE_TEXT;
			botSelectInfo.picnames[k].generic.flags = QMF_SMALLFONT;
			botSelectInfo.picnames[k].generic.x = x + 32;
			botSelectInfo.picnames[k].generic.y = y + 64;
			botSelectInfo.picnames[k].string = botSelectInfo.botnames[k];
			botSelectInfo.picnames[k].color = color_orange;
			botSelectInfo.picnames[k].style = UI_CENTER | UI_SMALLFONT;

			x += (64 + 27);
		}
		y += (64 + 21);
	}

	botSelectInfo.arrowleft.generic.type = MTYPE_BITMAP1024S;
	botSelectInfo.arrowleft.x = 352 - 20 - 99;
	botSelectInfo.arrowleft.y = 640;
	botSelectInfo.arrowleft.w = 99;
	botSelectInfo.arrowleft.h = 38;
	botSelectInfo.arrowleft.shader = trap_R_RegisterShaderNoMip(STARTSERVER_ARROWLEFT0);
	botSelectInfo.arrowleft.mouseovershader = trap_R_RegisterShaderNoMip(STARTSERVER_ARROWLEFT1);
	botSelectInfo.arrowleft.generic.callback = UI_BotSelectMenu_LeftEvent;
	//	botSelectInfo.arrowleft.generic.id			= ID_PREVPAGE;

	botSelectInfo.arrowright.generic.type = MTYPE_BITMAP1024S;
	botSelectInfo.arrowright.x = 352 + 20;
	botSelectInfo.arrowright.y = 640;
	botSelectInfo.arrowright.w = 98;
	botSelectInfo.arrowright.h = 38;
	botSelectInfo.arrowright.shader = trap_R_RegisterShaderNoMip(STARTSERVER_ARROWRIGHT0);
	botSelectInfo.arrowright.mouseovershader = trap_R_RegisterShaderNoMip(STARTSERVER_ARROWRIGHT1);
	botSelectInfo.arrowright.generic.callback = UI_BotSelectMenu_RightEvent;
	//	botSelectInfo.arrowright.generic.id			= ID_NEXTPAGE;

	y = (350 - MAX_SELECTLISTBOTS * 16 + 64);

	for (i = 0; i < MAX_SELECTLISTBOTS; i++) {
		botSelectInfo.selectedbotnames[i].generic.type = MTYPE_TEXTS;
		botSelectInfo.selectedbotnames[i].fontHeight = 16.0f;
		botSelectInfo.selectedbotnames[i].generic.flags = QMF_SMALLFONT;
		botSelectInfo.selectedbotnames[i].generic.x = 430;
		botSelectInfo.selectedbotnames[i].generic.y = y + i * 16;
		botSelectInfo.selectedbotnames[i].string = "               "; // MonsterPad_blue
		botSelectInfo.selectedbotnames[i].color = color_white;
		botSelectInfo.selectedbotnames[i].focuscolor = color_orange;
		botSelectInfo.selectedbotnames[i].generic.id = i;
		botSelectInfo.selectedbotnames[i].generic.callback = UI_BotSelectMenu_SelectBotName;

		botSelectInfo.selectedbotteams[i].generic.type = MTYPE_TEXTS;
		botSelectInfo.selectedbotteams[i].fontHeight = 16.0f;
		botSelectInfo.selectedbotteams[i].generic.flags = QMF_SMALLFONT;
		botSelectInfo.selectedbotteams[i].generic.x = 550;
		botSelectInfo.selectedbotteams[i].generic.y = y + i * 16;
		botSelectInfo.selectedbotteams[i].string = "    "; // blue
		botSelectInfo.selectedbotteams[i].color = color_blue;
		botSelectInfo.selectedbotteams[i].focuscolor = color_white;
		botSelectInfo.selectedbotteams[i].generic.id = i;
		botSelectInfo.selectedbotteams[i].generic.callback = UI_BotSelectMenu_SelectBotTeam;
	}

	y = y * 768 / 480;
	botSelectInfo.arrowup.generic.type = MTYPE_BITMAP1024S;
	botSelectInfo.arrowup.x = 1024 - 50;
	botSelectInfo.arrowup.y = y; // 480-(10*MAX_SELECTLISTBOTS)-20;
	botSelectInfo.arrowup.w = 38;
	botSelectInfo.arrowup.h = 100;
	botSelectInfo.arrowup.shader = trap_R_RegisterShaderNoMip("menu/server/arrowup0");
	botSelectInfo.arrowup.mouseovershader = trap_R_RegisterShaderNoMip("menu/server/arrowup1");
	botSelectInfo.arrowup.generic.callback = UI_BotSelectMenu_ListUp;

	botSelectInfo.arrowdown.generic.type = MTYPE_BITMAP1024S;
	botSelectInfo.arrowdown.x = 1024 - 50;
	botSelectInfo.arrowdown.y = y + (25.6f * MAX_SELECTLISTBOTS) - 99; // 480+20;
	botSelectInfo.arrowdown.w = 38;
	botSelectInfo.arrowdown.h = 99;
	botSelectInfo.arrowdown.shader = trap_R_RegisterShaderNoMip("menu/server/arrowdown0");
	botSelectInfo.arrowdown.mouseovershader = trap_R_RegisterShaderNoMip("menu/server/arrowdown1");
	botSelectInfo.arrowdown.generic.callback = UI_BotSelectMenu_ListDown;

	botSelectInfo.BotSkill.generic.type = MTYPE_SPINCONTROL;
	botSelectInfo.BotSkill.generic.flags = QMF_SMALLFONT;
	botSelectInfo.BotSkill.generic.name = "Skill:";
	botSelectInfo.BotSkill.generic.x = 485;
	botSelectInfo.BotSkill.generic.y = 420;
	botSelectInfo.BotSkill.itemnames = botSkill_list;
	botSelectInfo.BotSkill.curvalue = 1;
	botSelectInfo.BotSkill.generic.callback = UI_BotSelectMenu_BotSkillChanged;
	s_startserver.botskill = 2;

	botSelectInfo.slotsleft.generic.type = MTYPE_TEXTS;
	botSelectInfo.slotsleft.generic.flags = QMF_SMALLFONT;
	botSelectInfo.slotsleft.string = va("Open Slots: %2d", getSlotsLeft());
	botSelectInfo.slotsleft.generic.x = 430;
	botSelectInfo.slotsleft.generic.y = 440;
	botSelectInfo.slotsleft.color = color_yellow;

	botSelectInfo.back.generic.type = MTYPE_BITMAP;
	botSelectInfo.back.generic.name = BOTSELECT_BACK0;
	botSelectInfo.back.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	botSelectInfo.back.generic.callback = UI_BotSelectMenu_BackEvent;
	botSelectInfo.back.generic.x = 552;
	botSelectInfo.back.generic.y = 440;
	botSelectInfo.back.width = 80;
	botSelectInfo.back.height = 40;
	botSelectInfo.back.focuspic = BOTSELECT_BACK1;

	for (i = 0; i < MAX_MODELSPERPAGE; i++) {
		Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.pics[i]);
		//		Menu_AddItem( &botSelectInfo.menu,	&botSelectInfo.picbuttons[i] );
		//		Menu_AddItem( &botSelectInfo.menu,	&botSelectInfo.picnames[i] );
	}

	Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.arrowleft);
	Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.arrowright);

	for (i = 0; i < MAX_SELECTLISTBOTS; i++) {
		Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.selectedbotnames[i]);
		Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.selectedbotteams[i]);
	}
	Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.arrowup);
	Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.arrowdown);
	Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.BotSkill);
	Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.slotsleft);
	Menu_AddItem(&botSelectInfo.menu, &botSelectInfo.back);

	UI_BotSelectMenu_BuildList();
	//	UI_BotSelectMenu_Default( bot );
	UI_BotSelectMenu_UpdateList(); // selectbot-list
	botSelectInfo.modelpage = botSelectInfo.selectedmodel / MAX_MODELSPERPAGE;
	UI_BotSelectMenu_UpdateGrid();
}

/*
=================
UI_BotSelectMenu
=================
*/
void UI_BotSelectMenu() {
	UI_BotSelectMenu_Init();
	UI_PushMenu(&botSelectInfo.menu);
}
