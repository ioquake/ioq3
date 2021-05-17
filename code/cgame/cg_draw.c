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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int numSortedTeamPlayers;

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles,
					float scale, byte rgba[4]) {
	refdef_t refdef;
	refEntity_t ent;

	CG_AdjustFrom640(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof(refdef));

	memset(&ent, 0, sizeof(ent));
	AnglesToAxis(angles, ent.axis);
	ent.axis[0][0] *= scale;
	ent.axis[0][1] *= scale;
	ent.axis[0][2] *= scale;
	ent.axis[1][0] *= scale;
	ent.axis[1][1] *= scale;
	ent.axis[1][2] *= scale;
	ent.axis[2][0] *= scale;
	ent.axis[2][1] *= scale;
	ent.axis[2][2] *= scale;
	VectorCopy(origin, ent.origin);
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW; // no stencil shadows

	if (rgba) {
		memcpy(&ent.shaderRGBA, rgba, sizeof(ent.shaderRGBA));
	}

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead(float x, float y, float w, float h, int clientNum, vec3_t headAngles) {
	clipHandle_t cm;
	clientInfo_t *ci;
	float len;
	vec3_t origin;
	vec3_t mins, maxs;

	ci = &cgs.clientinfo[clientNum];

	cm = ci->headModel;
	if (!cm) {
		return;
	}

	// offset the origin y and z to center the head
	trap_R_ModelBounds(cm, mins, maxs);

	origin[2] = -0.5 * (mins[2] + maxs[2]);
	origin[1] = 0.5 * (mins[1] + maxs[1]);

	// calculate distance so the head nearly fills the box
	// assume heads are taller than wide
	len = 0.7 * (maxs[2] - mins[2]);
	origin[0] = len / 0.268; // len / tan( fov/2 )

	// allow per-model tweaking
	VectorAdd(origin, ci->headOffset, origin);

	if (ci->glowModel) {
		CG_Draw3DModel(x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles, ci->headScale, ci->glowColor);
	} else {
		CG_Draw3DModel(x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles, ci->headScale, NULL);
	}

	// if they are deferred, draw a cross out
	if (ci->deferred) {
		CG_DrawPic(x, y, w, h, cgs.media.deferShader);
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel(float x, float y, float w, float h, int team, qboolean force2D) {
	qhandle_t cm;
	float len;
	vec3_t origin, angles;
	vec3_t mins, maxs;
	qhandle_t handle;

	if (!force2D && cg_draw3dIcons.integer) {

		VectorClear(angles);

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds(cm, mins, maxs);

		// old:		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[2] = -1.1 * (mins[2] + maxs[2]);
		origin[1] = 0.5 * (mins[1] + maxs[1]);

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * (maxs[2] - mins[2]);
		origin[0] = len / 0.268; // len / tan( fov/2 )

		// old:		angles[YAW] = 60 * sin( cg.time / 2000.0 );;
		angles[YAW] = 100 * (cg.time / 2000.0);

		if (team == TEAM_RED) {
			handle = cgs.media.redFlagModel;
		} else if (team == TEAM_BLUE) {
			handle = cgs.media.blueFlagModel;
		} else if (team == TEAM_FREE) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel(x, y, w, h, handle, 0, origin, angles, 1.15f, NULL); // old: 1.0f
	} else if (cg_drawIcons.integer) {
		gitem_t *item;

		if (team == TEAM_RED) {
			item = BG_FindItemForPowerup(PW_REDFLAG);
		} else if (team == TEAM_BLUE) {
			item = BG_FindItemForPowerup(PW_BLUEFLAG);
		} else {
			return;
		}
		if (item) {
			CG_DrawPic(x, y, w, h, cg_items[ITEM_INDEX(item)].icon);
		}
	}
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team) {
	vec4_t hcolor;

	hcolor[3] = alpha;
	if (team == TEAM_RED) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if (team == TEAM_BLUE) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor(hcolor);
	CG_DrawPic(x, y, w, h, cgs.media.whiteShader);
	trap_R_SetColor(NULL);
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker(float y) {
	int t;
	float size;
	vec3_t angles;
	const char *info;
	const char *name;
	int clientNum;

	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) {
		return y;
	}

	if (!cg.attackerTime) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if (clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum) {
		return y;
	}

	if (!cgs.clientinfo[clientNum].infoValid) {
		cg.attackerTime = 0;
		return y;
	}

	t = cg.time - cg.attackerTime;
	if (t > ATTACKER_HEAD_TIME) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead(640 - size, y, size, size, clientNum, angles);

	info = CG_ConfigString(CS_PLAYERS + clientNum);
	name = Info_ValueForKey(info, "n");
	y += size;
	CG_DrawBigString(640 - (Q_PrintStrlen(name) * BIGCHAR_WIDTH), y, name, 0.5);

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot(float y) {
	char *s;
	int w;

	s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime, cg.latestSnapshotNum, cgs.serverCommandSequence);
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define FPS_FRAMES 4
static float CG_DrawFPS(float y) {
	char *s;
	int w;
	static int previousTimes[FPS_FRAMES];
	static int index;
	int i, total;
	int fps;
	static int previous;
	int t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if (index > FPS_FRAMES) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for (i = 0; i < FPS_FRAMES; i++) {
			total += previousTimes[i];
		}
		if (!total) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va("%ifps", fps);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

		CG_DrawBigString(635 - w, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer(float y) {
	char *s;
	int w;
	int mins, seconds, tens;
	int msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va("%i:%i%i", mins, tens, seconds);
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawRealTime
=================
*/
static float CG_DrawRealTime(float y) {
	char *s;
	int w;
	qtime_t qtime;

	trap_RealTime(&qtime);

	if (cg_drawRealTime.integer == 2) {
		qtime_t time;

		trap_RealTime(&time);

		DrawTurnableString(320, 240, "    ## hours ##>", colorBlue, 16,
						   90.0f - ((float)(time.tm_hour % 12) * 60.0f + time.tm_min) / 720.0f * 360.0f,
						   TURNORIGIN_MIDDLELEFT);
		DrawTurnableString(320, 240, "    === minutes ===>", colorGreen, 16,
						   90.0f - (time.tm_min * 60.0f + time.tm_sec) / 10.0f, TURNORIGIN_MIDDLELEFT);
		DrawTurnableString(320, 240, "    ----- seconds -----", colorRed, 16, 90.0f - time.tm_sec / 60.0f * 360.0f,
						   TURNORIGIN_MIDDLELEFT);

		return y;
	}

	s = va("[%i:%i%i]", qtime.tm_hour, (qtime.tm_min / 10) % 10, qtime.tm_min % 10);
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

// ups anfang
/*
=================
CG_Drawups
=================
*/
static float CG_Drawups(float y) {
	char *s;
	int w;

	s = va("%.0fups", cg.xyspeed);
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}
// ups ende

/*
=================
CG_DrawTimeLeft
=================
*/
static float CG_DrawTimeLeft(float y) {
	int timeleft;
	vec4_t bgcolor = {0.0f, 0.0f, 0.0f, 0.5f};
	char textcolor, textcolor2;
	int secs;

	if (cgs.timelimit <= 0)
		return y;

	timeleft = (cgs.timelimit * 60000 - (cg.time - cgs.levelStartTime));

	if (timeleft >= 120000) {
		bgcolor[0] = 0.0f;
		bgcolor[1] = 1.0f;
		bgcolor[2] = 0.0f;
		textcolor = '0';
		textcolor2 = '7';
	} else if (timeleft >= 60000) {
		bgcolor[0] = 1.0f - ((float)(timeleft - 60000) / 60000.0f); // 1.0f;
		bgcolor[1] = 1.0f - ((float)(120000 - timeleft) / 120000.0f);
		bgcolor[2] = 0.0f;
		textcolor = '0';
		textcolor2 = '7';
	} else {
		bgcolor[0] = 1.0f;
		bgcolor[1] = ((float)timeleft / 120000.0f); // 0.0f;
		bgcolor[2] = 0.0f;
		if (timeleft < 30000) {
			textcolor = ((cg.time / 500) % 2) ? '0' : '7';
			textcolor2 = ((cg.time / 500) % 2) ? '7' : '0';
		} else {
			textcolor = '0';
			textcolor2 = '7';
		}
	}

	if (timeleft < 0) {
		bgcolor[0] = 1.0f;
		bgcolor[1] = 0.0f;
		bgcolor[2] = 0.0f;

		textcolor = '0';
		textcolor2 = '7';
	}

	CG_FillRect(640 - 150, y, 150, SMALLCHAR_HEIGHT + 4, bgcolor);
	CG_DrawRect(640 - 150, y, 150, SMALLCHAR_HEIGHT + 4, 1, colorBlack);

	if (timeleft >= 0) {
		secs = (timeleft / 1000) % 60;
		CG_DrawStringExt(
			640 - 148, y + 2,
			va("^%ctime left: ^%c%3i:%i%i", textcolor, textcolor2, (timeleft / 60000), secs / 10, secs % 10),
			colorWhite, qfalse, qfalse, 8, 16, 18);
	} else
		CG_DrawStringExt(640 - 148, y + 2, "^0time left:  ^3--:--", colorWhite, qfalse, qfalse, 8, 16, 18);

	return y + BIGCHAR_HEIGHT + 4;
}

//:HERBY:ea
/*
==============
CG_MessagePrint
==============
*/
void CG_MessagePrint(const char *str) {
	//	char	*s;

	Q_strncpyz(cg.messagePrint, str, sizeof(cg.messagePrint));
	cg.messagePrintTime = cg.time;
}

/*
=================
CG_DrawMessages
=================
*/
#define MESSAGE_TIME 5000
#define MESSAGE_FADETIME 1000
static float CG_DrawMessages(float y) {
	int w;

	if (cg.time - cg.messagePrintTime > MESSAGE_TIME + MESSAGE_FADETIME)
		return y;

	w = CG_DrawStrlen(cg.messagePrint) * SMALLCHAR_WIDTH;
	CG_DrawSmallString(635 - w, y + 2, cg.messagePrint,
					   cg.time - cg.messagePrintTime < MESSAGE_TIME
						   ? 1.0f
						   : (float)(MESSAGE_TIME + cg.messagePrintTime - cg.time) / MESSAGE_FADETIME);
	return y + SMALLCHAR_HEIGHT + 4;
}
//:HERBY:ee

/*
#######################

  CG_DrawServerInfos

#######################
*/
static const char *gametype_strs[] = {
	GAMETYPE_NAME_SHORT(GT_FFA),	   GAMETYPE_NAME_SHORT(GT_TOURNAMENT),	 GAMETYPE_NAME_SHORT(GT_SINGLE_PLAYER),
	GAMETYPE_NAME_SHORT(GT_SPRAYFFA),  GAMETYPE_NAME_SHORT(GT_LPS),			 GAMETYPE_NAME_SHORT(GT_TEAM),
	GAMETYPE_NAME_SHORT(GT_FREEZETAG), GAMETYPE_NAME_SHORT(GT_CTF),			 GAMETYPE_NAME_SHORT(GT_SPRAY),
	GAMETYPE_NAME_SHORT(GT_BALLOON),   GAMETYPE_NAME_SHORT(GT_MAX_GAME_TYPE)};

static float CG_DrawServerInfos(float y) {
	const char *s;
	//	char tmpstr[MAX_CVAR_VALUE_STRING];//256
	int tmpi;
	float scrollLen;
	vec4_t tblack33 = {0.0f, 0.0f, 0.0f, 0.33f};
	static int xpos = 0;
	int numLines;

	if (!cg_drawServerInfos.integer) {
		if (xpos > 0)
			xpos -= (cg.frametime / 2);
		else {
			xpos = 0;
			return y;
		}
	} else if (xpos < 150)
		xpos += (cg.frametime / 2);
	else
		xpos = 150;

	s = CG_ConfigString(CS_MOTD);

	if (*s != '\0')
		numLines = 6;
	else
		numLines = 5;

	if ((cgs.gametype == GT_LPS) && (cgs.lpsflags & LPSF_PPOINTLIMIT)) {
		numLines++;
	}

	CG_FillRect(640 - xpos, y, 150, numLines * SMALLCHAR_HEIGHT + 4, tblack33);
	CG_DrawRect(640 - xpos, y, 150, numLines * SMALLCHAR_HEIGHT + 4, 1, colorBlack);

	y += 2;
	//	trap_Cvar_VariableStringBuffer("sv_hostname",tmpstr,MAX_CVAR_VALUE_STRING);
	CG_DrawStringExt(640 - xpos + 2, y, cgs.servername, colorWhite, qfalse, qfalse, 8, 16, 18);

	if (*s != '\0') {
		y += SMALLCHAR_HEIGHT;
		tmpi = strlen(s);
		if (tmpi <= 18)
			CG_DrawStringExt(640 - xpos + 2, y, s, colorWhite, qfalse, qfalse, 8, 16, 18);
		else {
			/*			if(tmpi>(255-2*18)) tmpi=219;

						memset(tmpstr,' ',18);
						Q_strncpyz((tmpstr+18),s,tmpi+1);
						memset((tmpstr+tmpi+18),' ',18);
						tmpstr[tmpi+2*18]='\0';

						tmpi=(cg.time/175)%(tmpi+18);

						CG_DrawStringExt(640-xpos+2,y,tmpstr+tmpi,colorWhite,qfalse,qfalse,8,16,18);
			*/

			// scrollwidth ~> chars*8 + 2*framewidth
			// 150 + 8*tmpi + somespace
			DrawStringWithCutFrame(640 - ((cg.time % ((200 + 8 * tmpi) * 20)) / 20.0f), y, s, colorWhite, 8, 16,
								   640 - xpos + 2, 0, 640 - 1, 480);
		}
	}

	CG_FillRect(640 - xpos, y + 16, 150, 1, colorBlack);
	y += 2;

	y += SMALLCHAR_HEIGHT;
	//	trap_Cvar_VariableStringBuffer("g_gametype",tmpstr,MAX_CVAR_VALUE_STRING);
	tmpi = cgs.gametype;
	tmpi = (tmpi < 0 || tmpi >= GT_MAX_GAME_TYPE) ? GT_MAX_GAME_TYPE : tmpi;
	CG_DrawStringExt(640 - xpos + 2, y, va("gametype: %s", gametype_strs[tmpi]), colorWhite, qfalse, qfalse, 8, 16, 18);

	y += SMALLCHAR_HEIGHT;
	//	trap_Cvar_VariableStringBuffer("mapname",tmpstr,MAX_CVAR_VALUE_STRING);//<-- geht scheinbar nur local o_O
	scrollLen = tmpi = strlen(cgs.shortmapname) - 13;
	/*	if(tmpi>0)
			tmpi=(int)( (float)(tmpi)*(0.5f+0.5f*sin(0.01f/scrollLen*cg.time)) );
		else
			tmpi=0;
		CG_DrawStringExt(640-xpos+2,y,va("map: ^5%-13s", (cgs.shortmapname+tmpi)),colorWhite,qfalse,qfalse,8,16,18);
	*/
	CG_DrawStringExt(640 - xpos + 2, y, "map: ", colorWhite, qfalse, qfalse, 8, 16, 18);
	if (scrollLen > 0)
		DrawStringWithCutFrame(640 - xpos + 45 - (8 * scrollLen * (0.5f + 0.5f * sin(0.01f / scrollLen * cg.time))), y,
							   cgs.shortmapname, colorCyan, 8, 16, 640 - xpos + 8 * 5, 0, 640 - 1, 480);
	else
		CG_DrawStringExt(640 - xpos + 42, y, cgs.shortmapname, colorCyan, qfalse, qfalse, 8, 16, 18);

	//	trap_Cvar_VariableStringBuffer("pointlimit",tmpstr,MAX_CVAR_VALUE_STRING);

	if ((cgs.gametype != GT_LPS) || (cgs.lpsflags & LPSF_PPOINTLIMIT)) {
		y += SMALLCHAR_HEIGHT;
		CG_DrawStringExt((640 - xpos + 2), y, va("pointlimit: " S_COLOR_GREEN "%i", cgs.fraglimit), colorWhite, qfalse,
						 qfalse, 8, 16, 18);
	}

	if (cgs.gametype == GT_LPS) {
		y += SMALLCHAR_HEIGHT;
		CG_DrawStringExt((640 - xpos + 2), y, va("startlives: " S_COLOR_GREEN "%i", cgs.lpsStartLives), colorWhite,
						 qfalse, qfalse, 8, 16, 18);
	}

	y += SMALLCHAR_HEIGHT;
	//	trap_Cvar_VariableStringBuffer("timelimit",tmpstr,MAX_CVAR_VALUE_STRING);
	CG_DrawStringExt(640 - xpos + 2, y, va("timelimit: ^2%i", cgs.timelimit), colorWhite, qfalse, qfalse, 8, 16, 18);

	return y + SMALLCHAR_HEIGHT + 2;
}

static float CG_DrawBotInfo(float y_in) {
	const char *info;
	// char	key[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	int width, x, y;

	y = y_in;
	info = CG_ConfigString(CS_BOTINFO);

	y += 30;

	while (1) {
		StringDump_GetNext(&info, value);
		if (!value[0])
			break;

		// draw value
		width = SMALLCHAR_WIDTH * CG_DrawStrlen(value);
		if (!width || width > 635)
			break;
		x = 635 - width;
		CG_DrawSmallString(x, y, value, 1.0F);

		y += SMALLCHAR_HEIGHT + 4;
	}

	return y;
}

#define WOPTEAMOVERLAY_X 5
#define WOPTEAMOVERLAY_Y 90
#define WOPTEAMOVERLAY_H 225
#define WOPTOL_HSPLIT 5

#define WOPTOL_ITEMHEIGHT_GOOD 52
#define WOPTOL_LOCHEIGHT 12
#define WOPTOL_ITEMHEIGHT_SMALL ? // TODO

static void CG_WoPTeamOverlay(void) {
	int team = cg.snap->ps.persistant[PERS_TEAM];
	int i, j;
	int slotID;
	int maxSlots;
	float itemh; // old/test: = (WOPTEAMOVERLAY_H-WOPTOL_HSPLIT*(cnt-1))/cnt;
	vec4_t hcolor;
	const char *loc;
	qboolean withLocLine = qfalse;

	if (!cg_drawTeamOverlay.integer) {
		return;
	}

	if (team != TEAM_RED && team != TEAM_BLUE) {
		return; // Not on any team
	}

	for (i = 1; i < MAX_LOCATIONS; i++) {
		loc = CG_ConfigString(CS_LOCATIONS + i);
		if (loc && *loc) {
			withLocLine = qtrue;
			break;
		}
	}

	// TODO: buildin a small-teaminfo for more than 4 players
	itemh = WOPTOL_ITEMHEIGHT_GOOD;
	if (!withLocLine)
		itemh -= WOPTOL_LOCHEIGHT;

	maxSlots = (WOPTEAMOVERLAY_H + WOPTOL_HSPLIT) / (itemh + WOPTOL_HSPLIT);

	if (team == TEAM_RED) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}

	slotID = 0;
	for (i = 0; i < numSortedTeamPlayers; i++) {
		qhandle_t weaponShader = cgs.media.deferShader;
		int strLen;
		float scrollLen;
		clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[i];
		float x = WOPTEAMOVERLAY_X;
		float y = (WOPTEAMOVERLAY_Y + (slotID * (itemh + WOPTOL_HSPLIT)));
		float w = 65;
		float h = itemh;
		float scale = 1.0f;

		// Don't draw self in overlay..
		// TODO: Also apply this to default overlay!
		if (sortedTeamPlayers[i] == cg.snap->ps.clientNum) {
			continue;
		}

		CG_FillRect(x, y, w, h, hcolor);
		CG_DrawPic(x + 1, y + 1, 25, 25, ci->modelIcon);
		scale = ci->health / 100.0f;
		if (scale > 1.0f)
			scale = 1.0f;
		CG_FillRect(x + 28, y + 1, 35 * scale, 5, colorRed); // health
		scale = ci->armor / 100.0f;
		if (scale > 1.0f)
			scale = 1.0f;
		CG_FillRect(x + 28, y + 7, 35 * scale, 5, colorBlue); // armor

		if (cg_weapons[ci->curWeapon].weaponIcon) {
			if (ci->curWeapon == WP_SPRAYPISTOL && ci->team == TEAM_BLUE)
				weaponShader = cgs.media.blueSpraypistolicon;
			else if (ci->curWeapon == WP_SPRAYPISTOL && ci->team == TEAM_FREE)
				weaponShader = cgs.media.neutralSpraypistolicon;
			else
				weaponShader = cg_weapons[ci->curWeapon].weaponIcon;
		}
		CG_DrawPic(x + 30, y + 14, 10, 10, weaponShader);

		switch (cgs.gametype) {
		case GT_SPRAY:
			CG_DrawPic(
				x + 46, y + 14, 10, 10,
				cg_items[(team == TEAM_RED ? cgs.media.redCartridgeEntNum : cgs.media.blueCartridgeEntNum)].icon);
			DrawStringWithCutFrame(x + 46 + 11, y + 14, va("%i", ci->numCartridges), colorWhite, 8, 10, x, y, x + w,
								   y + h);
			break;
		case GT_CTF:
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (j != PW_REDFLAG && j != PW_BLUEFLAG)
					continue;

				if (ci->powerups & (1 << j)) {
					gitem_t *item;

					item = BG_FindItemForPowerup(j);

					if (item) {
						CG_DrawPic(x + 46, y + 14, 10, 10, trap_R_RegisterShader(item->icon));
					}
				}
			}
			break;
		default:
			break;
		}

		strLen = CG_DrawStrlen(ci->name);
		scrollLen = (strLen * 6) - (w - 4) + 2;
		if (scrollLen > 0.0f)
			DrawStringWithCutFrame(x + 2 - scrollLen * (0.5f + 0.5f * sin(0.01f / (scrollLen + 1) * cg.time)), y + 27,
								   ci->name, colorWhite, 6, 12, x + 2, y, x + w - 2, y + h);
		else
			DrawStringWithCutFrame(x + 2, y + 27, ci->name, colorWhite, 6, 12, x + 2, y, x + w - 2, y + h);

		if (withLocLine) {
			loc = CG_ConfigString(CS_LOCATIONS + ci->location);
			if (!loc || !*loc)
				loc = ""; //"unknown"
			strLen = CG_DrawStrlen(loc);
			if (strLen * 6 > (w - 4)) {
				scrollLen = (strLen * 6) + w;
				DrawStringWithCutFrame(x + (w - 4) - ((cg.time % (int)(scrollLen * 40)) / 40.0f), y + 27 + 13, loc,
									   colorWhite, 6, 12, x + 2, y, x + w, y + h);
			} else
				DrawStringWithCutFrame(x + 2, y + 27 + 13, loc, colorWhite, 6, 12, x + 2, y, x + w, y + h);
		}

		// all slots are filled?
		if (++slotID >= maxSlots)
			break;
	}
}

#if 0
/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
/*	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
*/
	CG_FillRect(x,y,w,h,hcolor);

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
//				len = CG_DrawStrlen(p);
//				if (len > lwidth)
//					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth +
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon )
			{
				if(ci->curWeapon==WP_SPRAYPISTOL && ci->team==TEAM_BLUE)
					CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
						cgs.media.blueSpraypistolicon );
				else if(ci->curWeapon==WP_SPRAYPISTOL && ci->team==TEAM_FREE)
					CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
						cgs.media.neutralSpraypistolicon );
				else
					CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
						cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}
#endif

static float CG_DrawVoiceNames(float y) {
	const char *info;
	const char *name;
	float alpha;
	int i;
	float durationFraction;
	int lines = 0;
	int nameLeft;
	int iconLeft;
	static const int duration_max = 2000;
	static const float alpha_max = 1.0f;
	static const float alpha_min = 0.0f;
	static const int lines_max = 3;
	static const int icon2textSpacing = 3;
	static const int iconWidth = BIGCHAR_HEIGHT;
	static const int iconHeight = BIGCHAR_HEIGHT;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (!cg.lastVoiceTime[i])
			continue;

		if (cg.lastVoiceTime[i] < cg.time - duration_max)
			continue;

		if (i == cg.predictedPlayerState.clientNum)
			continue;

		if (lines++ > lines_max)
			break;

		info = CG_ConfigString(CS_PLAYERS + i);
		name = Info_ValueForKey(info, "n");
		durationFraction = (float)(cg.time - cg.lastVoiceTime[i]) / duration_max;
		alpha = alpha_max - (durationFraction * (alpha_max - alpha_min));

		nameLeft = 640 - (Q_PrintStrlen(name) * BIGCHAR_WIDTH);
		CG_DrawBigString(nameLeft, y, name, alpha);

		iconLeft = nameLeft - icon2textSpacing - iconWidth;
		CG_DrawPic(iconLeft, y, iconWidth, iconHeight, cgs.media.voiceIcon);

		y += BIGCHAR_HEIGHT + 2;
	}
	return y;
}

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame) {
	float y;

	y = 0;

	// if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
	//	y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	//}
	if (cg_drawSnapshot.integer) {
		y = CG_DrawSnapshot(y);
	}
	if (cg_drawFPS.integer && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT)) {
		y = CG_DrawFPS(y);
	}
	if (cg_drawTimer.integer) {
		y = CG_DrawTimer(y);
	}
	if (cg_drawRealTime.integer) {
		y = CG_DrawRealTime(y);
	}
	// ups anfang
	if (cg_drawups.integer) {
		y = CG_Drawups(y);
	}
	// ups ende
	if (cg_drawTimeLeft.integer) {
		y = CG_DrawTimeLeft(y);
	}
	//	if(cg_drawServerInfos.integer) // <-- moved into the function
	{ y = CG_DrawServerInfos(y); }
	//:HERBY:ea
	if (cg_drawMessages.integer) {
		y = CG_DrawMessages(y);
	}
	//:HERBY:ee
	if (CG_GetCvarInt("bot_developer")) {
		y = CG_DrawBotInfo(y);
	}
	if (cg_drawAttacker.integer) {
		y = CG_DrawAttacker(y);
	}
	if (cg_drawVoiceNames.integer) {
		y = CG_DrawVoiceNames(y);
	}
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawPowerups
================
*/
static float CG_DrawPowerups(float y) {
	int sorted[MAX_POWERUPS];
	int sortedTime[MAX_POWERUPS];
	int i, j, k;
	int active;
	playerState_t *ps;
	int t;
	gitem_t *item;
	float size;
	float f;

	ps = &cg.snap->ps;

	if (ps->stats[STAT_HEALTH] <= 0) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for (i = 0; i < MAX_POWERUPS; i++) {
		if (!ps->powerups[i]) {
			continue;
		}
		t = ps->powerups[i] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if (t < 0 || t > 999000) {
			continue;
		}

		// insert into the list
		for (j = 0; j < active; j++) {
			if (sortedTime[j] >= t) {
				for (k = active - 1; k >= j; k--) {
					sorted[k + 1] = sorted[k];
					sortedTime[k + 1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	for (i = 0; i < active; i++) {
		item = BG_FindItemForPowerup(sorted[i]);

		if (item) {
			y -= ICON_SIZE;

			t = ps->powerups[sorted[i]];
			if (t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME) {
				trap_R_SetColor(NULL);
			} else {
				vec4_t modulate;

				f = (float)(t - cg.time) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor(modulate);
			}

			if (cg.powerupActive == sorted[i] && cg.time - cg.powerupTime < PULSE_TIME) {
				f = 1.0f - (((float)cg.time - cg.powerupTime) / PULSE_TIME);
				size = ICON_SIZE * (1.0 + (PULSE_SCALE - 1.0) * f);
			} else {
				size = ICON_SIZE;
			}

			CG_DrawPic(640 - size, y + ICON_SIZE / 2 - size / 2, size, size, trap_R_RegisterShader(item->icon));
			CG_DrawStringExt(640 - 24 - 2, y + ICON_SIZE - 16, va("%3i", sortedTime[i] / 1000), colorWhite, qtrue,
							 qtrue, 8, 16, 4);
		}
	}
	trap_R_SetColor(NULL);

	return y;
}

int CG_DrawCartridgeStatus(int y) {
	char s1[16];								 //,s2[16];
	vec4_t bgcolor = {1.0f, 0.33f, 0.0f, 0.33f}; // I know this isn't red =)
	//	vec4_t	blue = {0.0f,0.33f,1.0f,0.33f};//I know this isn't blue =)

	Com_sprintf(s1, 16, "%i", cg.snap->ps.ammo[WP_SPRAYPISTOL]);

	y -= 25;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
		bgcolor[0] = 0.0f;
		bgcolor[1] = 0.33f;
		bgcolor[2] = 1.0f;

		CG_DrawPic(640 - 20, y, 20, 20, cg_items[cgs.media.blueCartridgeEntNum].icon);
	} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
		bgcolor[0] = 1.0f;
		bgcolor[1] = 0.33f;
		bgcolor[2] = 0.0f;

		CG_DrawPic(640 - 20, y, 20, 20, cg_items[cgs.media.redCartridgeEntNum].icon);
	} else {
		// sprayffa
		bgcolor[0] = cgs.clientinfo[cg.snap->ps.clientNum].color1[0];
		bgcolor[1] = cgs.clientinfo[cg.snap->ps.clientNum].color1[1];
		bgcolor[2] = cgs.clientinfo[cg.snap->ps.clientNum].color1[2];

		CG_DrawPic(640 - 20, y, 20, 20, cg_items[cgs.media.neutralCartridgeEntNum].icon);
	}

	CG_FillRect(640 - 30 - 20, y, 30, 20, bgcolor);
	CG_DrawStringExt(640 - 21 - (CG_DrawStrlen(s1) * 9), y + 1, s1, colorWhite, qfalse, qfalse, 9, 18, 32);
	CG_DrawRect(640 - 30 - 20, y, 30, 20, 1, colorBlack);

	return y;
}

static void CG_DrawHoldableItem(float y);

/*
=====================
CG_DrawLowerRight

=====================
*/
static void CG_DrawLowerRight(void) {
	float y;

	y = 360;

	// if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2 ) {
	//	y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	//}

	y = CG_DrawPowerups(y);
	CG_DrawHoldableItem(y);
}

/*
===================
CG_DrawPickupItem
===================
*/
static int CG_DrawPickupItem(int y) {
	int value;
	float *fadeColor;

	if (cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		return y;
	}

	y -= ICON_SIZE;

	value = cg.itemPickup;
	if (value) {
		fadeColor = CG_FadeColor(cg.itemPickupTime, 3000);
		if (fadeColor) {
			CG_RegisterItemVisuals(value);
			trap_R_SetColor(fadeColor);
			CG_DrawPic(8, y, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
			CG_DrawBigString(ICON_SIZE + 16, y + (ICON_SIZE / 2 - BIGCHAR_HEIGHT / 2), bg_itemlist[value].pickup_name,
							 fadeColor[0]);
			trap_R_SetColor(NULL);
		}
	}

	return y;
}

/*
=====================
CG_DrawLowerLeft

=====================
*/
static void CG_DrawLowerLeft(void) {
	float y;

	y = 360;

	// if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3 ) {
	//	y = CG_DrawTeamOverlay( y, qfalse, qfalse );
	//}

	y = CG_DrawPickupItem(y);

	if (cg_drawTeamOverlay.integer)
		CG_WoPTeamOverlay();
}

//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
static void CG_DrawTeamInfo(void) {
	int h;
	int i;
	vec4_t hcolor;
	int chatHeight;

#define CHATLOC_Y 360 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		/*		trap_R_SetColor( hcolor );
				CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar );
				trap_R_SetColor( NULL );
		*/
		CG_FillRect(CHATLOC_X, CHATLOC_Y - h, 640, h, hcolor);

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt(CHATLOC_X + TINYCHAR_WIDTH, CHATLOC_Y - (cgs.teamChatPos - i) * TINYCHAR_HEIGHT,
							 cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
							 0);
		}
	}
}

/*
===================
CG_DrawHoldableItem
===================
*/
static void CG_DrawHoldableItem(float y) {
	int value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if (value) {
		const int itemState = cg.snap->ps.stats[STAT_HOLDABLEVAR];
		CG_RegisterItemVisuals(value);

		y -= ICON_SIZE;
		CG_DrawPic(640 - ICON_SIZE, y, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
		if (cg.snap->ps.stats[STAT_FORBIDDENITEMS] & (1 << bg_itemlist[value].giTag)) {
			CG_DrawPic(640 - ICON_SIZE, y, ICON_SIZE, ICON_SIZE, cgs.media.noammoShader);
		}

		if (bg_itemlist[value].giTag == HI_FLOATER) {
			const vec4_t barColor = {0.33f, 0.33f, 1.0f, 0.66f};
			const float barFactor = 1.0f / (float)MAX_FLOATER;
			const int barHeight = (int)(ICON_SIZE * itemState * barFactor);
			const float barX = 640 - ICON_SIZE - 10;
			CG_FillRect(barX, y + ICON_SIZE - barHeight, 10, barHeight, barColor);
			CG_DrawRect(barX, y, 10, ICON_SIZE, 1.0f, colorWhite);
		} else if (bg_itemlist[value].giTag == HI_KILLERDUCKS) {
			CG_DrawStringExt(640 - 28, y + 8, va("%i", itemState), colorWhite, qtrue, qtrue, 8, 16, 1);
		} else if (bg_itemlist[value].giTag == HI_BOOMIES) {
			CG_DrawStringExt(640 - 28, y + 8, va("%i", itemState), colorWhite, qtrue, qtrue, 8, 16, 1);
		}
	}
}

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward(void) {
	float *color;
	int i, count;
	float x, y;
	char buf[32];

	if (!cg_drawRewards.integer) {
		return;
	}

	color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
	if (!color) {
		if (cg.rewardStack > 0) {
			for (i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i + 1];
				cg.rewardShader[i] = cg.rewardShader[i + 1];
				cg.rewardCount[i] = cg.rewardCount[i + 1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
			if (cg.rewardSound[0] !=
				cgs.media.impressiveSound) // etwas umstaendlich (aber so braucht man wenig neuen src ;)
				trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap_R_SetColor(color);

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if (cg.rewardCount[0] >= 10) {
		y = 56;
		x = 320 - ICON_SIZE / 2;
		CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = (SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen(buf)) / 2;
		CG_DrawStringExt(x, y + ICON_SIZE, buf, color, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
	} else {

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * ICON_SIZE / 2;
		for (i = 0; i < count; i++) {
			CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor(NULL);
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES 128

typedef struct {
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo(void) {
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo(snapshot_t *snap) {
	// dropped packet
	if (!snap) {
		lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = -1;
		lagometer.snapshotCount++;
		return;
	}

	if (cg.demoPlayback && (PM_INTERMISSION != snap->ps.pm_type)) {
		if (cg.snap) {
			int serverFrameTime = (snap->serverTime - cg.snap->serverTime);
			// NOTE: "-15 dependent on fps and maxpackets"
			snap->ping = ((snap->serverTime - snap->ps.commandTime) - serverFrameTime - 15);
		} else {
			snap->ping = (snap->serverTime - snap->ps.commandTime);
		}
	}

	// add this snapshot's info
	lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->ping;
	lagometer.snapshotFlags[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect(void) {
	float x, y;
	int cmdNum;
	usercmd_t cmd;
	const char *s;
	int w; // bk010215 - FIXME char message[1024];

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime ||
		cmd.serverTime > cg.time) { // special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 100, s, 1.0F);

	// blink the icon
	if ((cg.time >> 9) & 1) {
		return;
	}

	if ((cgs.gametype == GT_SPRAY) || (cgs.gametype == GT_SPRAYFFA)) {
		x = (640 - 48 - 94);
	} else if (cgs.gametype == GT_BALLOON) {
		x = (640 - 48 - 64);
	} else if (cgs.gametype == GT_CTF) {
		x = (640 - 48 - 78);
	} else {
		x = (640 - 48);
	}

	y = 480 - 48;

	CG_DrawPic(x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net"));
}

#define MAX_LAGOMETER_PING 900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer(void) {
	int a, x, y, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int color;
	float vscale;

	if (!cg_lagometer.integer || cgs.localServer) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	if ((cgs.gametype == GT_SPRAY) || (cgs.gametype == GT_SPRAYFFA)) {
		x = (640 - 48 - 94);
	} else if (cgs.gametype == GT_BALLOON) {
		x = (640 - 48 - 64);
	} else if (cgs.gametype == GT_CTF) {
		x = (640 - 48 - 78);
	} else {
		x = (640 - 48);
	}

	y = 480 - 48;

	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, 48, 48, cgs.media.lagometerShader);
	//	CG_FillRect(x,y,48,48,colorDkRed);

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for (a = 0; a < aw; a++) {
		i = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if (v > 0) {
			if (color != 1) {
				color = 1;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
			}
			if (v > range) {
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		} else if (v < 0) {
			if (color != 2) {
				color = 2;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_BLUE)]);
			}
			v = -v;
			if (v > range) {
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for (a = 0; a < aw; a++) {
		i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if (v > 0) {
			if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED) {
				if (color != 5) {
					color = 5; // YELLOW for rate delay
					trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
				}
			} else {
				if (color != 3) {
					color = 3;
					trap_R_SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
				}
			}
			v = v * vscale;
			if (v > range) {
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		} else if (v < 0) {
			if (color != 4) {
				color = 4; // RED for dropped snapshots
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(NULL);

	if (cg_nopredict.integer || cg_synchronousClients.integer) {
		CG_DrawBigString(x, y, "snc", 1.0);
	}

	CG_DrawDisconnect();
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint(const char *str, int y, int charWidth) {
	char *s;

	Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while (*s) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString(void) {
	char *start;
	int l;
	int x, y, w;
	float *color;

	if (!cg.centerPrintTime) {
		return;
	}

	color = CG_FadeColor(cg.centerPrintTime, 1000 * cg_centertime.value);
	if (!color) {
		return;
	}

	trap_R_SetColor(color);

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while (1) {
		char linebuffer[1024];

		for (l = 0; l < 50; l++) {
			if (!start[l] || start[l] == '\n') {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.centerPrintCharWidth * CG_DrawStrlen(linebuffer);

		x = (SCREEN_WIDTH - w) / 2;

		CG_DrawStringExt(x, y, linebuffer, color, qfalse, qtrue, cg.centerPrintCharWidth,
						 (int)(cg.centerPrintCharWidth * 1.5), 0);

		y += cg.centerPrintCharWidth * 1.5;
		while (*start && (*start != '\n')) {
			start++;
		}
		if (!*start) {
			break;
		}
		start++;
	}

	trap_R_SetColor(NULL);
}

/*
================================================================================

CROSSHAIR

================================================================================
*/

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void) {
	float w, h;
	qhandle_t hShader;
	float f;
	float x, y;
	int ca;

	if (!cg_drawCrosshair.integer) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if (cg.renderingThirdPerson) {
		return;
	}

	// set color based on health
	if (cg_crosshairHealth.integer) {
		vec4_t hcolor;

		CG_ColorForHealth(hcolor);
		trap_R_SetColor(hcolor);
	} else {
		trap_R_SetColor(NULL);
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if (f > 0 && f < ITEM_BLOB_TIME) {
		f /= ITEM_BLOB_TIME;
		w *= (1 + f);
		h *= (1 + f);
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640(&x, &y, &w, &h);

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ca % NUM_CROSSHAIRS];

	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - w), y + cg.refdef.y + 0.5 * (cg.refdef.height - h),
						  w, h, 0, 0, 1, 1, hShader);

	trap_R_SetColor(NULL);
}

/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D(void) {
	float w;
	qhandle_t hShader;
	float f;
	int ca;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if (!cg_drawCrosshair.integer) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if (cg.renderingThirdPerson) {
		return;
	}

	w = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if (f > 0 && f < ITEM_BLOB_TIME) {
		f /= ITEM_BLOB_TIME;
		w *= (1 + f);
	}

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ca % NUM_CROSSHAIRS];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);

	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);

	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;

	VectorCopy(trace.endpos, ent.origin);

	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;

	trap_R_AddRefEntityToScene(&ent);
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity(void) {
	trace_t trace;
	vec3_t start, end;
	int content;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY);
	if (trace.entityNum >= MAX_CLIENTS) {
		return;
	}

	// if the player is in fog, don't show it
	content = trap_CM_PointContents(trace.endpos, 0);
	if (content & CONTENTS_FOG) {
		return;
	}

	// if the player is invisible, don't show it
	if (cg_entities[trace.entityNum].currentState.powerups & (1 << PW_VISIONLESS)) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames(void) {
	float *color;
	char *name;
	float w;

	if (!cg_drawCrosshair.integer) {
		return;
	}
	if (!cg_drawCrosshairNames.integer) {
		return;
	}
	if (cg.renderingThirdPerson) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor(cg.crosshairClientTime, 1000);
	if (!color) {
		trap_R_SetColor(NULL);
		return;
	}

	name = cgs.clientinfo[cg.crosshairClientNum].name;
	w = CG_DrawStrlen(name) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 170, name, color[3] * 0.5f);
	trap_R_SetColor(NULL);
}

//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
	static char msgTeam[] = "press ESC and click START to join";
	static char msgTournament[] = "waiting to play";

	CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if (cgs.gametype == GT_TOURNAMENT) {
		CG_DrawBigString(320 - strlen(msgTournament) * 8, 460, msgTournament, 1.0F);
	} else if (cgs.gametype >= GT_TEAM) {
		CG_DrawBigString(320 - strlen(msgTeam) * 8, 460, msgTeam, 1.0F);
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char *s;
	int sec;

	if (!cgs.voteTime) {
		return;
	}

	// play a talk beep whenever it is modified
	if (cgs.voteModified) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
	if (sec < 0) {
		sec = 0;
	}

	s = va("VOTE: %s", cgs.voteString);
	CG_DrawSmallString(0, 58, s, 1.0);
	s = va("%is: %i yes - %i no", sec, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString(0, (58 + SMALLCHAR_HEIGHT), s, 1.0);
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char *s;
	int sec, cs_offset;

	if (cgs.clientinfo->team == TEAM_RED)
		cs_offset = 0;
	else if (cgs.clientinfo->team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (!cgs.teamVoteTime[cs_offset]) {
		return;
	}

	// play a talk beep whenever it is modified
	if (cgs.teamVoteModified[cs_offset]) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.teamVoteTime[cs_offset])) / 1000;
	if (sec < 0) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset], cgs.teamVoteYes[cs_offset],
		   cgs.teamVoteNo[cs_offset]);
	CG_DrawSmallString(0, 90, s, 1.0F);
}

static qboolean CG_DrawScoreboard(void) {
	return CG_DrawOldScoreboard();
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission(void) {
	if (cgs.gametype == GT_SINGLE_PLAYER) {
		CG_DrawCenterString();
		return;
	}
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow(void) {
	float x;
	vec4_t color;
	const char *name;

	if (!(cg.snap->ps.pm_flags & PMF_FOLLOW)) {
		return qfalse;
	}
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	CG_DrawBigString(320 - 9 * 8, 24, "following", 1.0F);

	name = cgs.clientinfo[cg.snap->ps.clientNum].name;

	x = 0.5 * (640 - 16 * CG_DrawStrlen(name));

	CG_DrawStringExt(x, 40, name, color, qtrue, qtrue, 16, 32, 0);

	return qtrue;
}

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning(void) {
	const char *s;
	int w;

	if (cg_drawAmmoWarning.integer == 0) {
		return;
	}

	if (!cg.lowAmmoWarning) {
		return;
	}

	if (cg.lowAmmoWarning == 2) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup(void) {
	int w;
	int sec;
	int i;
	int cw;
	clientInfo_t *ci1, *ci2;
	const char *s;
	float WarmupReady, curWarmupReady;

	sec = cg.warmup;
	if (!sec) {
		return;
	}

	if (sec < 0) {
		s = "Waiting for players";
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for (i = 0; i < cgs.maxclients; i++) {
			if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE) {
				if (!ci1) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if (ci1 && ci2) {
			s = va("%s" S_COLOR_WHITE " vs %s", ci1->name, ci2->name);
			w = CG_DrawStrlen(s);
			if (w > 640 / GIANT_WIDTH) {
				cw = 640 / w;
			} else {
				cw = GIANT_WIDTH;
			}
			CG_DrawStringExt(320 - w * cw / 2, 20, s, colorWhite, qfalse, qtrue, cw, (int)(cw * 1.5f), 0);
		}
	} else {
		if (cgs.gametype == GT_FFA) {
			s = GAMETYPE_NAME(GT_FFA);
		} else if (cgs.gametype == GT_TEAM) {
			s = GAMETYPE_NAME(GT_TEAM);
		} else if (cgs.gametype == GT_FREEZETAG) {
			s = GAMETYPE_NAME(GT_FREEZETAG);
		} else if (cgs.gametype == GT_CTF) {
			s = GAMETYPE_NAME(GT_CTF);
		} else {
			s = "";
		}
		w = CG_DrawStrlen(s);
		if (w > 640 / GIANT_WIDTH) {
			cw = 640 / w;
		} else {
			cw = GIANT_WIDTH;
		}
		CG_DrawStringExt(320 - w * cw / 2, 25, s, colorWhite, qfalse, qtrue, cw, (int)(cw * 1.1f), 0);
	}

	sec = (sec - cg.time) / 1000;
	if (sec < 0) {
		cg.warmup = 0;
		sec = 0;
	}

	WarmupReady = cg_warmupReady.value;
	curWarmupReady = cg_curWarmupReady.value;

	if (WarmupReady > 0.0f && curWarmupReady < WarmupReady)
		s = va("Waiting for %.0f%% ready (current %.0f%%)", 100.0f * WarmupReady, 100.0f * curWarmupReady);
	else
		s = va("Starts in: %i", sec + 1);
	if (sec != cg.warmupCount) {
		cg.warmupCount = sec;
		switch (sec) {
		case 0:
			trap_S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
			break;
		case 1:
			trap_S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
			break;
		case 2:
			trap_S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
			break;
		default:
			break;
		}
	}
	switch (cg.warmupCount) {
	case 0:
		cw = 28;
		break;
	case 1:
		cw = 24;
		break;
	case 2:
		cw = 20;
		break;
	default:
		cw = 16;
		break;
	}

	w = CG_DrawStrlen(s);
	CG_DrawStringExt(320 - w * cw / 2, 70, s, colorWhite, qfalse, qtrue, cw, (int)(cw * 1.5), 0);

	if (WarmupReady > 0.0f && curWarmupReady < WarmupReady) {
		s = va("type \"/ready\" into the console, if you are ready");
		w = CG_DrawStrlen(s);
		CG_DrawStringExt(320 - w * 4, 72 + (int)(cw * 1.5), s, colorWhite, qfalse, qtrue, 8, 16, 0);
	}
}

//==================================================================================

/**

  scrolling 5 letter score ...
  0..3...7			<-- char#
  ---12345---...	<-- contant
  |   |->			<-- "window"
*/
static void DrawBigScore(int y, int score) {
	switch (cg_bigScoreType.integer) {
	default:
	case 0: // squeeze
	{
		int x = 25;
		char *str = va("%i", score);
		CG_DrawStringExt(x, y, str, colorWhite, qtrue, qfalse, 28 / strlen(str), 16, 10);
	} break;
	case 1: // scrolling charbychar
	{
		int x = 30;
		char buff[32];
		int i, j, chr;
		memset(buff, ' ', sizeof(buff));
		buff[sizeof(buff) - 1] = '\0';

		for (i = 7, j = 1; i > 0 && 0 < (chr = (score / j)); j *= 10, --i) {
			chr = (chr % 10) + '0';
			buff[i] = chr;
		}

		CG_DrawStringExt(x, y, buff + ((cg.time / 250) & 7), colorWhite, qtrue, qfalse, 8, 16, 3);
	} break;
	case 2: // scrolling with cutframe
	{
		// 95 ~> chars*8 + 2*framewidth
		DrawStringWithCutFrame(54 - ((cg.time % 2500) * (95.0f / 2500.0f)), y, va("%i", score), colorWhite, 8, 16, 26,
							   0, 52, 480);
	} break;
	}
}

// assign balloon color
// capured -> team color
// half captured -> alternate between white and team color
// uncaptured -> white
// something is wrong: yellow
void CG_GetBalloonStateColor(entityState_t *s, vec4_t col) {
	static const vec4_t red = {1.0f, 0.0f, 0.0f, 1.0f};
	static const vec4_t blue = {0.0f, 0.0f, 1.0f, 1.0f};
	static const vec4_t white = {1.0f, 1.0f, 1.0f, 1.0f};
	static const vec4_t yellow = {1.0f, 0.9f, 0.1f, 1.0f};
	int team;

	if (!s || s->eType != ET_BALLOON) { // indicate error
		Vector4Copy(yellow, col);
		return;
	}

	// uncaptured?
	if (s->frame == 0) {
		Vector4Copy(white, col);
		return;
	}

	// captured or getting captured
	team = s->generic1;
	if (team == TEAM_RED)
		Vector4Copy(red, col);
	else if (team == TEAM_BLUE)
		Vector4Copy(blue, col);
	else // indicate error
		Vector4Copy(yellow, col);

	// flicker if not fully captured
	if (s->frame < 11 && ((cg.time / 400) & 1))
		Vector4Copy(white, col);
}

static void CG_GetBalloonColor(int index, vec4_t color) {
	static const vec4_t red = {1.0, 0.0, 0.0, 1.0};
	static const vec4_t blue = {0.0, 0.0, 1.0, 1.0};
	static const vec4_t white = {1.0, 1.0, 1.0, 1.0};
	static const vec4_t yellow = {1.0f, 0.9f, 0.1f, 1.0f};
	// FIXME: Make this buffer safe
	char status = cgs.balloonState[index];

	switch (status) {
	case '0':
		// uncaptured
		Vector4Copy(white, color);
		return;

	case '1' + TEAM_RED - 1:
	case 'a' + TEAM_RED - 1:
		// fully or partially captured by red
		Vector4Copy(red, color);
		break;

	case '1' + TEAM_BLUE - 1:
	case 'a' + TEAM_BLUE - 1:
		// fully or partially captured by blue
		Vector4Copy(blue, color);
		break;

	default:
		Vector4Copy(yellow, color);
		return;
	}

	if (((cg.time / 400) & 1) && ((('a' + TEAM_RED - 1) == status) || (('a' + TEAM_BLUE - 1) == status))) {
		// flicker if not fully captured
		Vector4Copy(white, color);
	}
}

static void CG_DrawBalloonIcon(centity_t *cent) {
	vec4_t iconColor;
	vec3_t iconPos;
	trace_t trace;
	float squaredDist;
	float size, x, y;

	if (!(cg_icons.integer & ICON_BALLOON)) {
		return;
	}

	CG_GetBalloonStateColor(&cent->currentState, iconColor);

	VectorCopy(cent->currentState.origin, iconPos);
	iconPos[2] += 40; // draw icon above visible box model
	squaredDist = Calculate_2DOf3D(iconPos, &cg.refdef, &x, &y);
	if (!squaredDist) {
		return;
	}

	// don't draw the icon if the box model is visible and close
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, cent->currentState.origin, cg.snap->ps.clientNum, MASK_OPAQUE);
	if (1.0 == trace.fraction) {
		if (DistanceSquared(cg.refdef.vieworg, cent->currentState.origin) < Square(250)) {
			return;
		}
	}

	size = Com_Clamp(0.5, 1, (1 / (sqrt(squaredDist) * 0.002)));

	trap_R_SetColor(iconColor);
	CG_DrawPic((x - 16.0 * size), (y - 16.0 * size), (32.0 * size), (32.0 * size), cgs.media.bbBoxIcon);
	trap_R_SetColor(NULL);
}

static void CG_DrawHealthstationIcon(centity_t *cent) {
	vec3_t iconPos;
	float size, x, y;
	float squaredDist;

	if (!(cg_icons.integer & ICON_HEALTHSTATION)) {
		return;
	}

	if (cg.snap->ps.stats[STAT_HEALTH] > (cg.snap->ps.stats[STAT_MAX_HEALTH] * 0.5)) {
		// Don't draw any healthstation icons if not low on health
		// FIXME: Handicap. Instagib.
		return;
	}

	VectorCopy(cent->lerpOrigin, iconPos);
	iconPos[2] += 120; // draw icon above visible model

	squaredDist = Calculate_2DOf3D(iconPos, &cg.refdef, &x, &y);
	if (!squaredDist) {
		return;
	}

	// TODO: don't draw the icon if the healthstation is visible and close

	size = Com_Clamp(0.5, 1, (1 / (sqrt(squaredDist) * 0.002)));

	trap_R_SetColor(NULL);
	CG_DrawPic((x - 16.0 * size), (y - 16.0 * size), (32.0 * size), (32.0 * size), cgs.media.healthstationIcon);
}

static void CG_DrawSprayroomIcon(centity_t *cent) {
	vec3_t iconPos;
	float size, x, y;
	float squaredDist;
	trace_t trace;

	if (!(cg_icons.integer & ICON_SPRAYROOM)) {
		return;
	}

	if (cg.snap->ps.ammo[WP_SPRAYPISTOL] <= 0) {
		// Only draw icon when carrying cartridges
		return;
	}

	VectorCopy(cent->currentState.origin2, iconPos);
	iconPos[2] += 100; // draw icon above visible model

	squaredDist = Calculate_2DOf3D(iconPos, &cg.refdef, &x, &y);
	if (!squaredDist) {
		return;
	}

	// don't draw the icon if the sprayroom teleporter is visible and close
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, cent->currentState.origin2, cg.snap->ps.clientNum, MASK_OPAQUE);
	if (1.0 == trace.fraction) {
		if (DistanceSquared(cg.refdef.vieworg, cent->currentState.origin2) < Square(250)) {
			return;
		}
	}

	size = Com_Clamp(0.5, 1, (1 / (sqrt(squaredDist) * 0.002)));

	trap_R_SetColor(NULL);
	CG_DrawPic((x - 16.0 * size), (y - 16.0 * size), (32.0 * size), (32.0 * size), cgs.media.sprayroomIcon);
}

static void CG_DrawBambamIcon(centity_t *cent) {
	trace_t trace;
	vec3_t start, end;
	float x, y;

	VectorCopy(cg.refdef.vieworg, start);
	VectorCopy(cent->lerpOrigin, end);
	end[2] += 60; // upper end of bambam

	CG_Trace(&trace, start, NULL, NULL, end, cg.snap->ps.clientNum, MASK_OPAQUE);
	if (1.0 == trace.fraction) {
		if (CG_WorldToScreen(end, &x, &y)) {
			float squaredDistance = DistanceSquared(start, end);

			float size = Com_Clamp(0, 1, (1.f / (sqrt(squaredDistance) * 0.005)));
			float hb_x = (x - 48.0 * size);
			float hb_y = (y - 12.0 * size);
			float hb_w = (96.0 * size);
			float hb_h = (24.0 * size);
			float hb_scale = cent->currentState.angles2[2]; // Bambam's scaled health is stored in this unused var :)
			vec4_t color = {0.0f, 0.0f, 0.0f, 1.0f};

			CG_AdjustFrom640(&hb_x, &hb_y, &hb_w, &hb_h);

			trap_R_DrawStretchPic(hb_x, hb_y, hb_w, hb_h, 0, 0, 1, 1, cgs.media.bambamHealthIconBG);
			switch (cent->currentState.time2) {
			case TEAM_RED:
				color[0] = 1.0f;
				break;
			default:
				color[2] = 1.0f;
				break;
			}
			trap_R_SetColor(color);
			trap_R_DrawStretchPic(hb_x, hb_y, hb_w * hb_scale, hb_h, 0, 0, hb_scale, 1, cgs.media.bambamHealthIcon);
			trap_R_SetColor(NULL);
		}
	}
}

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(stereoFrame_t stereoFrame) {
	// if we are taking a levelshot for the menu, don't draw anything
	if (cg.levelShot) {
		return;
	}

	if (cg_draw2D.integer == 0) {
		return;
	}

	if (cg_cineHideHud.integer) {
		CG_DrawCenterString();
		return;
	}

	//	{ vec4_t	twhite = { 1,1,1,0.5f}; CG_DrawStringExt(300,0,GAME_VERSION,twhite,qtrue,qfalse,8,16,32); }
	//	Com_Printf(":=%i\n",trap_Key_IsDown(':'));

	if (cg.first2dtime == 0)
		cg.first2dtime = cg.time;
	if (cg.time - cg.first2dtime < 8000 && 1) // noch cvar einfuegen ;P
	{
		int lenOfDrawStr, tmpi;
		vec4_t twhite = {1.0f, 1.0f, 1.0f, 1.0f};
		const char *s;
		const char *gametype_strs[] = {
			GAMETYPE_NAME(GT_FFA),		 GAMETYPE_NAME(GT_TOURNAMENT),	 GAMETYPE_NAME(GT_SINGLE_PLAYER),
			GAMETYPE_NAME(GT_SPRAYFFA),	 GAMETYPE_NAME(GT_LPS),			 GAMETYPE_NAME(GT_TEAM),
			GAMETYPE_NAME(GT_FREEZETAG), GAMETYPE_NAME(GT_CTF),			 GAMETYPE_NAME(GT_SPRAY),
			GAMETYPE_NAME(GT_BALLOON),	 GAMETYPE_NAME(GT_MAX_GAME_TYPE)};

		if (cg.time - cg.first2dtime < 5000)
			twhite[3] = 1.0f;
		else
			twhite[3] = (float)(8000 - (cg.time - cg.first2dtime)) / 3000.0f;

		tmpi = cgs.gametype;
		tmpi = (tmpi < 0 || tmpi >= GT_MAX_GAME_TYPE) ? GT_MAX_GAME_TYPE : tmpi;
		lenOfDrawStr = strlen(gametype_strs[tmpi]);
		CG_DrawStringExt(320 - lenOfDrawStr * 6, 240 - 26, gametype_strs[tmpi], twhite, qfalse, qfalse, 12, 24, 30);
		CG_DrawStringExt(320 - lenOfDrawStr * 6, 240 - 26, gametype_strs[tmpi], twhite, qfalse, qfalse, 12, 24, 30);

		s = CG_ConfigString(CS_MESSAGE); // longmapname (or the msg from the mapper ;) )
		lenOfDrawStr = strlen(s);
		CG_DrawStringExt(320 - lenOfDrawStr * 4, 240, s, twhite, qfalse, qfalse, 8, 16, 64);
	}

	if (CG_FreezeTag() && FT_LocalIsFrozen()) {
		int x, y, barposy, barposx;
		int iconsize = 64;
		int barheight = 12;
		int barsegments = 6;
		int segmentwidth = 10;
		int distance = 5;
		vec4_t teamcolor;
		const char *message;

		vec4_t blue = {0.75f, 0.75f, 0.75f, 1.0f};
		vec4_t orange = {0.75f, 0.0f, 0.0f, 1.0f};
		int i;

		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			teamcolor[0] = 1.0f;
			teamcolor[1] = 0.0f;
			teamcolor[2] = 0.0f;
		} else {
			teamcolor[0] = 0.0f;
			teamcolor[1] = 0.0f;
			teamcolor[2] = 1.0f;
		}

		teamcolor[3] = 0.50f;

		x = 640 / 2;
		y = 100;

		barposy = y + iconsize / 2 + distance + barheight / 2;
		barposx = x - barsegments * segmentwidth / 2;

		// chat background
		// CG_FillRect( 0, 0, 450, 60, teamcolor );

		i = cg.predictedPlayerState.stats[STAT_CHILL];
		i = barsegments - i;
		if (i >= 0) {

			if (i > 6)
				i = 6;

			// draw the icon
			CG_DrawPic(x - iconsize / 2, y - iconsize / 2, iconsize, iconsize, cgs.media.thawIcon);

			// draw bar border
			CG_FillRect(barposx - 2, barposy - 2, barsegments * segmentwidth + 4, barheight + 4, colorWhite);

			// draw the bar background
			CG_FillRect(barposx, barposy, barsegments * segmentwidth, barheight, blue);

			// draw the bar
			if (i > 0)
				CG_FillRect(barposx, barposy, i * segmentwidth, barheight, orange);
		}
		trap_R_SetColor(NULL);

		message = "^7You are ^4frozen^7, wait for a teammate to ^3thaw^7 you.";
		CG_FillRect(x - (CG_DrawStrlen(message) * 12) / 2, 358, CG_DrawStrlen(message) * 12, 22, teamcolor);
		CG_DrawStringExt(x - (CG_DrawStrlen(message) * 12) / 2, 360, message, colorWhite, qfalse, qtrue, 12,
						 (int)(12 * 1.5), 0);
	}
	/* Thawer's progress bar */
	else if (CG_FreezeTag()) {
		int x, y, barposy, barposx;
		int iconsize = 50;
		int barheight = 10;
		int barsegments = 6;
		int segmentwidth = 10;
		int distance = 5;

		vec4_t blue = {0.75f, 0.75f, 0.75f, 1.0f};
		vec4_t orange = {0.75f, 0.0f, 0.0f, 1.0f};
		int i;

		x = cg_ft_thawerIconX.integer;
		y = cg_ft_thawerIconY.integer;

		barposy = y + iconsize / 2 + distance + barheight / 2;
		barposx = x - barsegments * segmentwidth / 2;

		i = cg.predictedPlayerState.stats[STAT_CHILL];
		i = barsegments - i;
		if (i < 6) {

			if (i > 6)
				i = 6;

			CG_DrawPic(x - iconsize / 2, y - iconsize / 2, iconsize, iconsize, cgs.media.thawIcon);

			// draw bar border
			CG_FillRect(barposx - 2, barposy - 2, barsegments * segmentwidth + 4, barheight + 4, colorWhite);

			CG_FillRect(barposx, barposy, barsegments * segmentwidth, barheight, blue);

			if (i > 0)
				CG_FillRect(barposx, barposy, i * segmentwidth, barheight, orange);
		}
		trap_R_SetColor(NULL);
	}

	// draw the menu above all
	if (cg.wantSelectLogo) {
		ActiveChooseLogoMenu();
	}

	if (cg.snap->ps.pm_type == PM_INTERMISSION) {
		if (cg.zoomed)
			CG_ZoomDown_f();

		CG_DrawIntermission();
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		CG_DrawUpperRight(stereoFrame);
		CG_DrawSpectator();

		if (stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();

		CG_DrawCrosshairNames();

		cg.scoreBoardShowing = CG_DrawScoreboard();
		if (!cg.scoreBoardShowing)
			CG_DrawCenterString();

		return;
	} else if (cgs.gametype == GT_LPS && cg.snap->ps.stats[STAT_LIVESLEFT] < 0 &&
			   !(cg.snap->ps.pm_flags & PMF_FOLLOW)) {
		CG_DrawStringExt(320 - 4 * 23, 50, "Waiting for next Round!", colorGreen, qtrue, qfalse, 8, 16, 32);
		CG_DrawUpperRight(stereoFrame);
		CG_DrawSpectator();

		if (stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();

		CG_DrawCrosshairNames();

		cg.scoreBoardShowing = CG_DrawScoreboard();
		if (!cg.scoreBoardShowing)
			CG_DrawCenterString();

		return;
	} else if (cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		cg.scoreBoardShowing = CG_DrawScoreboard();
		if (!cg.scoreBoardShowing)
			CG_DrawCenterString();

		return;
	} else if (cg.zoomed) {
		vec4_t ammoColor = {1.0f, 0.5f, 0.0f, 1.0f};
		char tmpstr[16];
		//		int		ammoWidth;

		CG_DrawCrosshairNames();
		/*
				CG_DrawStatusBar();
				CG_DrawAmmoWarning();
				CG_DrawWeaponSelect();
				CG_DrawHoldableItem();
				CG_DrawReward();
				if ( cgs.gametype >= GT_TEAM )
					CG_DrawTeamInfo();

				CG_DrawVote();
				CG_DrawTeamVote();
				CG_DrawLagometer();
				CG_DrawUpperRight(stereoFrame);
				CG_DrawLowerRight();
				CG_DrawLowerLeft();
		*/
		//		CG_FillRect(240-40,360,52,20,colorWhite);
		//		CG_DrawRect(240-40,360,52,20,1,colorBlack);

		// Draw Ammo count on screen
		if (cg.snap->ps.weapon == WP_SPLASHER) {
			Com_sprintf(tmpstr, 16, "%i", cg.snap->ps.ammo[WP_SPLASHER]);
			ammoColor[3] = 1.0f;
			UI_DrawProportionalString(237, 361, tmpstr, UI_RIGHT | UI_SMALLFONT, ammoColor);
		}

		if (!CG_DrawFollow())
			CG_DrawWarmup();

		return;
	} else {
		float x, y, w, h;
		int weaponNum, team;
		int hudnum;
		/*
				trap_R_DrawStretchPic(cg.refdef.x,cg.refdef.y+(int)((float)cg.refdef.height*0.759f),
					(int)((float)cg.refdef.width*0.128f),(int)((float)cg.refdef.height*0.241f),0,0,1,1,cgs.media.hud_bl);

				trap_R_DrawStretchPic(cg.refdef.x+(int)((float)cg.refdef.width*0.334f),cg.refdef.y+(int)((float)cg.refdef.height*0.846f),
					(int)((float)cg.refdef.width*0.332f),(int)((float)cg.refdef.height*0.139f),0,0,1,1,cgs.media.hud_bc);

				trap_R_DrawStretchPic(cg.refdef.x+(int)((float)cg.refdef.width*0.857f),cg.refdef.y+(int)((float)cg.refdef.height*0.764f),
					(int)((float)cg.refdef.width*0.143f),(int)((float)cg.refdef.height*0.236f),0,0,1,1,cgs.media.hud_br);
		*/

		if (stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();
		CG_DrawCrosshairNames();
		//		CG_DrawStatusBar();
		CG_DrawAmmoWarning();
		CG_DrawWeaponSelect();
		//		CG_DrawHoldableItem(); // -> jetzt im lower right
		CG_DrawReward();
		if (cgs.gametype >= GT_TEAM)
			CG_DrawTeamInfo();

		CG_DrawVote();
		CG_DrawTeamVote();
		CG_DrawLagometer();
		CG_DrawUpperRight(stereoFrame);
		CG_DrawLowerRight();

		CG_DrawLowerLeft();
		if (!CG_DrawFollow())
			CG_DrawWarmup();
		else if (cgs.gametype == GT_LPS && cgs.clientinfo[cg.clientNum].team == TEAM_FREE)
			CG_DrawStringExt(320 - 4 * 23, 90, "Waiting for next Round!", colorGreen, qtrue, qfalse, 8, 16, 32);

		if (((cgs.gametype == GT_SPRAYFFA) || (cgs.gametype == GT_SPRAY)) &&
			(cg.snap->ps.stats[STAT_SPRAYROOMSECS] <= 12)) {
			static int lastsprayroomtime = 0;
			int tmpi;

			// NOTE: STAT_SPRAYROOMSECS is ( level.maxsprayroomtime + 1 )

			tmpi = cg.snap->ps.stats[STAT_SPRAYROOMSECS];

			if (lastsprayroomtime != tmpi) {
				if ((tmpi > 0) && (tmpi <= 10)) {
					trap_S_StartLocalSound(cgs.media.CountDown_CountDown[(tmpi - 1)], CHAN_ANNOUNCER);
				} else if (tmpi == 12) {
					trap_S_StartLocalSound(cgs.media.CountDown_TenSecondsToLeave, CHAN_ANNOUNCER);
				}

				lastsprayroomtime = tmpi;
			}

			if ((tmpi > 0) && (tmpi <= 10)) {
				vec4_t tmpcolor = {1.0, 0.0, 0.0, 1.0};

				tmpcolor[1] = tmpcolor[2] = (0.25 + (0.25 * sin(cg.time * 0.02)));

				CG_DrawStringExt((320 - 64), 100, "You have", colorWhite, qtrue, qfalse, 16, 32, 0);
				CG_DrawStringExt((320 - 8), 132, va("%i", tmpi), tmpcolor, qtrue, qfalse, 16, 32, 0);
				CG_DrawStringExt((320 - 56), 164, "seconds", colorWhite, qtrue, qfalse, 16, 32, 0);
				CG_DrawStringExt((320 - 144), 196, "to leave this room!", colorWhite, qtrue, qfalse, 16, 32, 0);
			}
		}

		if (cgs.gametype >= GT_TEAM)
			//			hudnum=cgs.clientinfo[cg.clientNum].team;//realteam -> wrong if following
			hudnum = cg.snap->ps.persistant[PERS_TEAM];
		else {
			hudnum = cg_wopFFAhud.integer;
			if (hudnum > 9 || hudnum < 0)
				hudnum = 0;
		}
		// vv bl
		weaponNum = cg.snap->ps.weapon;
		team = cg.snap->ps.persistant[PERS_TEAM];
		{
			int hudscore2 = SCORE_NOT_PRESENT;

			if (weaponNum > WP_NONE) {
				if (weaponNum == WP_SPRAYPISTOL && team == TEAM_BLUE)
					CG_DrawPic(10, 375, 42, 42, cgs.media.blueSpraypistolicon);
				else if (weaponNum == WP_SPRAYPISTOL && team == TEAM_FREE)
					CG_DrawPic(10, 375, 42, 42, cgs.media.neutralSpraypistolicon);
				else
					CG_DrawPic(10, 375, 42, 42, cg_weapons[weaponNum].weaponIcon);
			}

			x = 0;
			y = 364;
			w = 82;
			h = 116;
			CG_AdjustFrom640(&x, &y, &w, &h);
			trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, cgs.media.hud_bl[hudnum]);

			if (cg.snap->ps.ammo[weaponNum] >= 0)
				CG_DrawStringExt(20, 425, va("%3i", cg.snap->ps.ammo[weaponNum]), colorBlack, qtrue, qfalse, 8, 16, 3);

			if (cgs.gametype < GT_TEAM) {
				// LPS has livesleft as primary score only when LPSF_PPOINTLIMIT is not enabled
				if ((cgs.gametype == GT_LPS) && !(cgs.lpsflags & LPSF_PPOINTLIMIT)) {
					int ownLives = ((cg.snap->ps.stats[STAT_LIVESLEFT] < 0) ? 0 : cg.snap->ps.stats[STAT_LIVESLEFT]);

					if (cgs.scores1 != ownLives) {
						hudscore2 = ownLives;
						CG_DrawPic(8.75f, (364 + 98), 14, 14, cgs.media.hud_teammarker);
					} else {
						hudscore2 = cgs.scores2;
						CG_DrawPic(8.75f, (364 + 79), 14, 14, cgs.media.hud_teammarker);
					}
				} else {
					if (cgs.scores1 != cg.snap->ps.persistant[PERS_SCORE]) {
						hudscore2 = cg.snap->ps.persistant[PERS_SCORE];
						CG_DrawPic(8.75f, 364 + 98, 14, 14, cgs.media.hud_teammarker);
					} else {
						hudscore2 = cgs.scores2;
						CG_DrawPic(8.75f, 364 + 79, 14, 14, cgs.media.hud_teammarker);
					}
				}
			} else {
				hudscore2 = cgs.scores2;
				if (team == TEAM_RED)
					CG_DrawPic(8.75f, 364 + 79, 14, 14, cgs.media.hud_teammarker);
				else if (team == TEAM_BLUE)
					CG_DrawPic(8.75f, 364 + 98, 14, 14, cgs.media.hud_teammarker);
			}

			if (cgs.scores1 != SCORE_NOT_PRESENT) {
				if (cgs.scores1 < 1000)
					CG_DrawStringExt(30, 442, va("%2i", cgs.scores1), colorWhite, qtrue, qfalse, 8, 16, 3);
				else
					DrawBigScore(442, cgs.scores1);
			}
			if (hudscore2 != SCORE_NOT_PRESENT) {
				if (hudscore2 < 1000)
					CG_DrawStringExt(30, 461, va("%2i", hudscore2), colorWhite, qtrue, qfalse, 8, 16, 3);
				else
					DrawBigScore(442, hudscore2);
			}
		}
		//^^ bl
		// vv bc
		// draw head
		{
			vec3_t angles;
			float size; //, stretch;
			float frac, tmpx;

			tmpx = 295; // 296;

			VectorClear(angles);

			/* dmg bewegung (erst mal raus genommen)
						if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
							frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
							size = ICON_SIZE * 1.0f * ( 1.5 - frac * 0.5 );

							stretch = size - ICON_SIZE * 1.25;
							// kick in the direction of damage
							tmpx -= stretch * 0.5 + cg.damageX * stretch * 0.5;

							cg.headStartYaw = 180 + cg.damageX * 45;

							cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
							cg.headEndPitch = 5 * cos( crandom()*M_PI );

							cg.headStartTime = cg.time;
							cg.headEndTime = cg.time + 100 + random() * 2000;
						} else {
			*/
			if (cg.time >= cg.headEndTime) {
				// select a new head angle
				cg.headStartYaw = cg.headEndYaw;
				cg.headStartPitch = cg.headEndPitch;
				cg.headStartTime = cg.headEndTime;
				cg.headEndTime = cg.time + 100 + random() * 2000;

				cg.headEndYaw = 180 + 20 * cos(crandom() * M_PI);
				cg.headEndPitch = 5 * cos(crandom() * M_PI);
			}

			size = 50; // ICON_SIZE * 1.0f;//1.25->1.0f ... ICON_SIZE=48
			//			}

			// if the server was frozen for a while we may have a bad head start time
			if (cg.headStartTime > cg.time) {
				cg.headStartTime = cg.time;
			}

			frac = (cg.time - cg.headStartTime) / (float)(cg.headEndTime - cg.headStartTime);
			frac = frac * frac * (3 - 2 * frac);
			angles[YAW] = cg.headStartYaw + (cg.headEndYaw - cg.headStartYaw) * frac;
			angles[PITCH] = cg.headStartPitch + (cg.headEndPitch - cg.headStartPitch) * frac;

			CG_DrawHead(tmpx, 407, size, size, cg.snap->ps.clientNum, angles);

			//			CG_DrawRect(tmpx,407,size,size,1,colorGreen);
		}

		{
			float tmpf, tmpf2;

			tmpf2 = 0.0f;
			tmpf = (float)cg.snap->ps.stats[STAT_HEALTH] * 0.01f;
			x = 214 + 10;
			y = 406 + 39;
			w = 69;
			h = 20;
			CG_AdjustFrom640(&x, &y, &w, &h);
			if (tmpf > 1.0f) {
				tmpf2 = tmpf - 1.0f;
				tmpf = 1.0f;
			}
			x += (1.0f - tmpf) * w;
			if (tmpf < 1.0f)
				trap_R_DrawStretchPic(x - w * (1.0f - tmpf), y, w * (1.0f - tmpf), h, 0, 0, (1.0f - tmpf), 1,
									  cgs.media.hud_energyglass);
			trap_R_DrawStretchPic(x, y, w * tmpf, h, (1.0f - tmpf), 0, 1, 1, cgs.media.hud_energybar);
			if (tmpf2 > 0.0f) {
				//				x=214+10+(1.0f-tmpf2)*w; // hier braeuchte man ein neues AdjustFrom640 ... aber x ist
				// bei tmpf=1 eh schon da wo es sein soll
				x += (1.0f - tmpf2) * w;
				trap_R_DrawStretchPic(x, y, w * tmpf2, h, (1.0f - tmpf2), 0, 1, 1, cgs.media.hud_energybar2);
			}

			tmpf2 = 0.0f;
			tmpf = (float)cg.snap->ps.stats[STAT_ARMOR] * 0.01f;
			x = 214 + 132;
			y = 406 + 39;
			w = 69;
			h = 20;
			CG_AdjustFrom640(&x, &y, &w, &h);
			if (tmpf > 1.0f) {
				tmpf2 = tmpf - 1.0f;
				tmpf = 1.0f;
			}
			if (tmpf < 1.0f)
				trap_R_DrawStretchPic(x + w * tmpf, y, w * (1.0f - tmpf), h, tmpf, 0, 1, 1, cgs.media.hud_shieldglass);
			trap_R_DrawStretchPic(x, y, w * tmpf, h, 0, 0, tmpf, 1, cgs.media.hud_shieldbar);
			if (tmpf2 > 0.0f)
				trap_R_DrawStretchPic(x, y, w * tmpf2, h, 0, 0, tmpf2, 1, cgs.media.hud_shieldbar2);

			x = 214;
			y = 406;
			w = 212;
			h = 67;
			CG_AdjustFrom640(&x, &y, &w, &h);
			trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, cgs.media.hud_bc[hudnum]);
		}
		//^^ bc

		{

			int i;
			centity_t *cent = NULL;

			// Iterate over all entities and draw their icons where required
			for (i = 0; i < MAX_GENTITIES; i++) {
				cent = &cg_entities[i];

				if (!cent->currentValid) {
					// Only draw entities which have valid data from snapshots,
					// otherwise we'd be using stale data
					continue;
				}

				switch (cent->currentState.eType) {
				case ET_BAMBAM:
					CG_DrawBambamIcon(cent);
					break;
				case ET_BALLOON:
					CG_DrawBalloonIcon(cent);
					break;
				case ET_STATION:
					CG_DrawHealthstationIcon(cent);
					break;
				case ET_TELEPORT_TRIGGER:
					// Magical constant is set for sprayroom teleporter in SP_trigger_teleport
					if (0x23 == cent->currentState.generic1) {
						CG_DrawSprayroomIcon(cent);
					}
					break;

				default:
					break;
				}
			}
		}

		// vv br
		if ((cgs.gametype == GT_SPRAY || cgs.gametype == GT_SPRAYFFA) && team != TEAM_SPECTATOR) {
			vec3_t tmporigin, tmpangles;
			int carts;

			tmpangles[0] = tmpangles[2] = tmporigin[1] = tmporigin[2] = 0.0f;

			tmporigin[0] = 70;
			//			tmpangles[1] = (float)( cg.time & 3999 ) * 0.09f;// 360/4000=0.09...//( cg.time & 2047 ) * 360 /
			// 2048.0f;
			tmpangles[1] = (float)(cg.time) * 0.09f;

			x = 548 + 16;
			y = 367 + 58;
			w = 54;
			h = 54;
			if (team == TEAM_BLUE)
				CG_Draw3DModel(x, y, w, h, cg_items[cgs.media.blueCartridgeEntNum].models[0], 0, tmporigin, tmpangles,
							   1.0f, NULL);
			else if (team == TEAM_RED)
				CG_Draw3DModel(x, y, w, h, cg_items[cgs.media.redCartridgeEntNum].models[0], 0, tmporigin, tmpangles,
							   1.0f, NULL);
			else
				CG_Draw3DModel(x, y, w, h, cg_items[cgs.media.neutralCartridgeEntNum].models[0], 0, tmporigin,
							   tmpangles, 1.0f, NULL);

			x = 548;
			y = 367;
			w = 92;
			h = 113;
			CG_AdjustFrom640(&x, &y, &w, &h);
			trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, cgs.media.hud_br[hudnum]);

			carts = cg.snap->ps.ammo[WP_SPRAYPISTOL];

			CG_DrawPic(548 + 78, 367 + 15, 9, 9, carts > 0 ? cgs.media.hud_dotfull : cgs.media.hud_dotempty);
			CG_DrawPic(548 + 78, 367 + 26, 9, 9, carts > 1 ? cgs.media.hud_dotfull : cgs.media.hud_dotempty);
			CG_DrawPic(548 + 78, 367 + 38, 9, 9, carts > 2 ? cgs.media.hud_dotfull : cgs.media.hud_dotempty);
			CG_DrawPic(548 + 78, 367 + 49, 9, 9, carts > 3 ? cgs.media.hud_dotfull : cgs.media.hud_dotempty);

			CG_DrawPic(548 + 78, 367 + 60, 9, 9, carts > 4 ? cgs.media.hud_dotfull : cgs.media.hud_dotempty);
			CG_DrawPic(548 + 78, 367 + 72, 9, 9, carts > 5 ? cgs.media.hud_dotfull : cgs.media.hud_dotempty);
			CG_DrawPic(548 + 78, 367 + 83, 9, 9, carts > 6 ? cgs.media.hud_dotfull : cgs.media.hud_dotempty);
			CG_DrawPic(548 + 78, 367 + 94, 9, 9, carts > 7 ? cgs.media.hud_dotfull : cgs.media.hud_dotempty);
		}
		//:HERBY:ea
		if (cgs.gametype == GT_BALLOON && team != TEAM_SPECTATOR) {
			vec4_t yellow = {1.0f, 0.9f, 0.1f, 1.0f};
			vec4_t orange = {1.0f, 0.4f, 0.1f, 1.0f};
			vec4_t iconColor;
			int i;

			// draw hud icon
			for (i = 0; i < MAX_BALLOONS; i++) {
				CG_GetBalloonColor(i, iconColor);
				trap_R_SetColor(iconColor);
				CG_DrawPic((640 - 34), (480 - 33 - 36 * i), 27, 27, cgs.media.hud_balloon);
			}
			trap_R_SetColor(NULL);

			// draw status bar
			i = cg.predictedPlayerState.stats[STAT_BALLOONTIME];
			if (i) {
				if (i > 0)
					trap_R_SetColor(yellow);
				else {
					i = -i;
					trap_R_SetColor(orange);
				}
				if (i > 11)
					i = 11;
				CG_DrawPic(640 - 55, 480 - 5 - i * 3.1f, 8, i * 3.1f, cgs.media.hud_balloon_bar);
			}
			trap_R_SetColor(NULL);

			// draw the background
			if (team == TEAM_RED)
				CG_DrawPic(640 - 64, 480 - 128, 64, 128, cgs.media.hud_bk_balloon_red);
			else
				CG_DrawPic(640 - 64, 480 - 128, 64, 128, cgs.media.hud_bk_balloon_blue);
		}
		//:HERBY:ee
		if (cgs.gametype == GT_CTF && team != TEAM_SPECTATOR) {
			// TODO: clarify whether position(red/blue) is fixed or per team?
			//			CG_DrawFlagModel(605,375,30,30,TEAM_RED,qfalse);

			//			CG_DrawFlagModel(605,445,30,30,TEAM_BLUE,qfalse);

#define CTL_BG_WIDTH 78
#define CTL_BG_HEIGHT 115

#define CTL_LOLLYMDLX 576 // 573
#define CTL_LOLLYMDLY 428 // 426
#define CTL_LOLLYMDLW 46 // 50
#define CTL_LOLLYMDLH 50

#define CTL_STATX 612
#define CTL_STATY1 370
#define CTL_STATY2 402
#define CTL_STATWH 25

			// draw the background
			if (team == TEAM_RED) {
				if (!cgs.media.hud_CTL_bg_red) { // keep old stuff, as fallback
					CG_DrawPic(605, 375, 25, 25, cgs.media.redFlagShader[cgs.redflag]);
					CG_DrawPic(605, 445, 25, 25, cgs.media.blueFlagShader[cgs.blueflag]);

					if (cg.predictedPlayerState.powerups[PW_BLUEFLAG])
						CG_DrawFlagModel(602, 407, 35, 35, TEAM_BLUE, qfalse);

					CG_DrawPic(640 - 64, 480 - 128, 64, 128, cgs.media.hud_bk_balloon_red);
				} else {
					CG_DrawPic(CTL_STATX, CTL_STATY1, CTL_STATWH, CTL_STATWH, cgs.media.redFlagShader[cgs.redflag]);
					CG_DrawPic(CTL_STATX, CTL_STATY2, CTL_STATWH, CTL_STATWH, cgs.media.blueFlagShader[cgs.blueflag]);

					if (cg.predictedPlayerState.powerups[PW_BLUEFLAG])
						CG_DrawFlagModel(CTL_LOLLYMDLX, CTL_LOLLYMDLY, CTL_LOLLYMDLW, CTL_LOLLYMDLH, TEAM_BLUE, qfalse);

					CG_DrawPic(640 - CTL_BG_WIDTH, 480 - CTL_BG_HEIGHT, CTL_BG_WIDTH, CTL_BG_HEIGHT,
							   cgs.media.hud_CTL_bg_red);
				}
			} else {
				if (!cgs.media.hud_CTL_bg_blue) { // keep old stuff, as fallback
					CG_DrawPic(605, 375, 25, 25, cgs.media.blueFlagShader[cgs.blueflag]);
					CG_DrawPic(605, 445, 25, 25, cgs.media.redFlagShader[cgs.redflag]);

					if (cg.predictedPlayerState.powerups[PW_REDFLAG])
						CG_DrawFlagModel(602, 407, 35, 35, TEAM_RED, qfalse);

					CG_DrawPic(640 - 64, 480 - 128, 64, 128, cgs.media.hud_bk_balloon_blue);
				} else {
					CG_DrawPic(CTL_STATX, CTL_STATY1, CTL_STATWH, CTL_STATWH, cgs.media.blueFlagShader[cgs.blueflag]);
					CG_DrawPic(CTL_STATX, CTL_STATY2, CTL_STATWH, CTL_STATWH, cgs.media.redFlagShader[cgs.redflag]);

					if (cg.predictedPlayerState.powerups[PW_REDFLAG])
						CG_DrawFlagModel(CTL_LOLLYMDLX, CTL_LOLLYMDLY, CTL_LOLLYMDLW, CTL_LOLLYMDLH, TEAM_RED, qfalse);

					CG_DrawPic(640 - CTL_BG_WIDTH, 480 - CTL_BG_HEIGHT, CTL_BG_WIDTH, CTL_BG_HEIGHT,
							   cgs.media.hud_CTL_bg_blue);
				}
			}
		}
		//^^ br
		if ((cgs.gametype == GT_LPS) && !(cgs.lpsflags & LPSF_NOARROWS) && (cg_icons.integer & ICON_ARROW)) {
			int i;
			int mostLives;

			// Since we are playing, there is at least one client
			mostLives = cg.scores[0].livesleft;
			// With LPSF_PPOINTLIMIT, first players are those
			// with highest score ( from previous rounds ).
			// Thus we need to iterate through all scores
			// to find the current highest number of lives.
			// TODO: Use lpsIconLead for the one with most lives as well?
			if (cgs.lpsflags & LPSF_PPOINTLIMIT) {
				for (i = 0; i < cg.numScores; i++) {
					if (cg.scores[i].livesleft > mostLives) {
						mostLives = cg.scores[i].livesleft;
					}
				}
			}

			for (i = 0; i < cg.numScores; i++) {
				score_t *score = &cg.scores[i];
				clientInfo_t *ci = &cgs.clientinfo[score->client];
				vec4_t strColor = {1.0, 1.0, 1.0, 1.0};

				// FIXME: Magical constants!
				if ((ci->lastPosSaveTime > 0) && ((cg.time - ci->lastPosSaveTime) < 1500)) {
					float x, y;
					float squaredDistance;

					// fading out, if the player disappeared
					strColor[3] = ((1500 - (cg.time - ci->lastPosSaveTime)) / 1000);
					if (strColor[3] > 1.0) {
						strColor[3] = 1.0;
					} else if (strColor[3] < 0.0) {
						strColor[3] = 0.0;
					}

					// FIXME: Remove cg_LPSwallhackAlpha
					strColor[3] *= cg_LPSwallhackAlpha.value;

					// FIXME: Calculate_2DOf3D is more than ugly!
					squaredDistance = Calculate_2DOf3D(ci->curPos, &cg.refdef, &x, &y);
					if (squaredDistance) {
						float size;

						size = (1 / (sqrt(squaredDistance) * 0.002));
						if (size > 1.0) {
							size = 1.0;
						} else if (size < 0.5) {
							size = 0.5;
						}

						// FIXME: Remove cg_LPSwallhackSize
						size *= cg_LPSwallhackSize.value;

						strColor[3] *= (0.3 + 0.7 * (1 / (1 + squaredDistance * 0.000004)));
						if (strColor[3] > 1.0) {
							strColor[3] = 1.0;
						}

						trap_R_SetColor(strColor);

						if (score->livesleft < mostLives) {
							CG_DrawPic((x - 16.0 * size), (y - 16.0 * size), (32.0 * size), (32.0 * size),
									   cgs.media.lpsIcon);
						} else {
							CG_DrawPic((x - 16.0 * size), (y - 16.0 * size), (32.0 * size), (32.0 * size),
									   cgs.media.lpsIconLead);
						}

						trap_R_SetColor(NULL);
					}
				}
			}
		}

		cg.scoreBoardShowing = CG_DrawScoreboard();
		if (!cg.scoreBoardShowing) {
			CG_DrawCenterString();
		}
	}
}

static void CG_DrawTourneyScoreboard(void) {
	CG_DrawOldTourneyScoreboard();
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive(stereoFrame_t stereoView) {
	int ltiT, waterTime;
	int notifytime;

	// optionally draw the info screen instead
	if (!cg.snap) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && (cg.snap->ps.pm_flags & PMF_SCOREBOARD)) {
		CG_DrawTourneyScoreboard();
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	/// draw kma zoom bluescreen-effect (this need to be in real 3d, so it can be overlayed by a higher sorted
	/// shader-aura)
	if (cg.zoomed && cg.snap->ps.weapon == WP_KMA97 && cgs.media.zoomKMAbluescreen) {
		float xscale, yscale;
		polyVert_t verts[4];

		memset(verts, 0, sizeof(verts));
		verts[0].modulate[2] = verts[1].modulate[2] = verts[2].modulate[2] = verts[3].modulate[2] = 255;

		verts[0].modulate[3] = verts[1].modulate[3] = verts[2].modulate[3] = verts[3].modulate[3] = 125;

		/// there seems to be a clipping at 4 units in front of the view ... so we move 5 to front
		VectorMA(cg.refdef.vieworg, 5, cg.refdef.viewaxis[0], verts[0].xyz);
		VectorCopy(verts[0].xyz, verts[2].xyz);

		xscale = tan(cg.refdef.fov_x * 0.5f * M_PI / 180.0f) * 5;
		yscale = tan(cg.refdef.fov_y * 0.5f * M_PI / 180.0f) * 5;

		VectorMA(verts[0].xyz, -xscale, cg.refdef.viewaxis[1], verts[0].xyz);
		VectorMA(verts[2].xyz, xscale, cg.refdef.viewaxis[1], verts[2].xyz);

		VectorCopy(verts[0].xyz, verts[1].xyz);
		VectorCopy(verts[2].xyz, verts[3].xyz);

		VectorMA(verts[0].xyz, yscale, cg.refdef.viewaxis[2], verts[0].xyz);
		VectorMA(verts[1].xyz, -yscale, cg.refdef.viewaxis[2], verts[1].xyz);
		VectorMA(verts[2].xyz, -yscale, cg.refdef.viewaxis[2], verts[2].xyz);
		VectorMA(verts[3].xyz, yscale, cg.refdef.viewaxis[2], verts[3].xyz);

		// old shader: cgs.media.whiteShader
		trap_R_AddPolyToScene(cgs.media.zoomKMAbluescreen, 4, verts);
	}

	if (stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	// draw 3D view
	trap_R_RenderScene(&cg.refdef);

	//	CG_Printf("org={%1.3f|%1.3f|%1.3f}\n",cg.refdef.vieworg[0],cg.refdef.vieworg[1],cg.refdef.vieworg[2]);
	if (cg_printDir.integer) {
		CG_Printf("dir={%1.3f|%1.3f|%1.3f}\n", cg.refdef.viewaxis[0][0], cg.refdef.viewaxis[0][1],
				  cg.refdef.viewaxis[0][2]);
	}

	AddLFsToScreen();

	// zoom specific drawing
	if (cg.zoomed) {
		trace_t tr;
		vec3_t tmpv3;
		vec4_t transwhite = {1, 1, 1, 0.1f}, colorOrange = {1, 0.5f, 0, 1};
		float x, y, w, h;

		if ((cg.snap->ps.weapon != WP_SPLASHER && cg.snap->ps.weapon != WP_KMA97) ||
			cg.snap->ps.stats[STAT_HEALTH] <= 0) {
			// we are either dead or the active weapon can't zoom
			// suppress zoom
			CG_ZoomDown_f();
		} else {
			// note: there is additional drawing related to the HUDs themselfs
			// going on inside CG_Draw2d()

			// we are alive and we have a zoomable weapon active
			// figure out which one
			if (cg.snap->ps.weapon == WP_SPLASHER) {
				// splasher Zoom-HUD stuff
				if (cg.zoomSoundStat > 0 && cg.zoomSoundStat <= 9 &&
					(cg.time - cg.zoomTime) >= 200 * (cg.zoomSoundStat - 1)) {
					trap_S_StartLocalSound(cgs.media.zoomsound[cg.zoomSoundStat - 1], CHAN_LOCAL_SOUND);
					cg.zoomSoundStat++;
				}

				trap_R_DrawStretchPic(cg.refdef.x, cg.refdef.y, cg.refdef.width, cg.refdef.height, 0, 0, 1, 1,
									  cgs.media.zoomhud);
				tmpv3[0] = -0.125f - cg.snap->ps.viewangles[1] * 0.0027778f; // 1/360= ~0.0027778

				DrawPic2Color(280, 100, 40, 20, tmpv3[0], 0, tmpv3[0] + 0.25f, 1, transwhite, colorWhite,
							  cgs.media.zoomcompass);
				DrawPic2Color(280, 100, 40, 20, tmpv3[0] + 1.0f, 0, tmpv3[0] + 1.25f, 1, transwhite, colorWhite,
							  cgs.media.zoomcompass);
				DrawPic2Color(320, 100, 40, 20, tmpv3[0] + 0.25f, 0, tmpv3[0] + 0.5f, 1, colorWhite, transwhite,
							  cgs.media.zoomcompass);
				DrawPic2Color(320, 100, 40, 20, tmpv3[0] + 1.25f, 0, tmpv3[0] + 1.5f, 1, colorWhite, transwhite,
							  cgs.media.zoomcompass);

				x = 248.0f;
				y = 360.0f;
				w = 148.0f;
				h = 23.0f;

				CG_AdjustFrom640(&x, &y, &w, &h);
				trap_R_DrawStretchPic(x, y, w, h, 0.78f * cg.zoomfactor, 0, (0.78f * cg.zoomfactor) + 0.22f, 1,
									  cgs.media.zoomruler);

				tmpv3[0] = cg.refdef.vieworg[0] + cg.refdef.viewaxis[0][0] * 100000.0f;
				tmpv3[1] = cg.refdef.vieworg[1] + cg.refdef.viewaxis[0][1] * 100000.0f;
				tmpv3[2] = cg.refdef.vieworg[2] + cg.refdef.viewaxis[0][2] * 100000.0f;
				CG_Trace(&tr, cg.refdef.vieworg, NULL, NULL, tmpv3, cg.snap->ps.clientNum, MASK_SHOT);
				tmpv3[0] = tr.endpos[0] - cg.refdef.vieworg[0];
				tmpv3[1] = tr.endpos[1] - cg.refdef.vieworg[1];
				tmpv3[2] = tr.endpos[2] - cg.refdef.vieworg[2];
				tmpv3[0] = VectorLength(tmpv3);
				CG_DrawStringExt(320, 190, va("%-4.0f", tmpv3[0]), colorOrange, qtrue, qfalse, 8, 16, 32);
			} else if (cg.snap->ps.weapon == WP_KMA97) {
				// kma Zoom-HUD stuff
				trap_R_DrawStretchPic(cg.refdef.x, cg.refdef.y, cg.refdef.width, cg.refdef.height, 0, 0, 1, 1,
									  cgs.media.zoomhud_kma);
			}
		}
	}

	// *******************
	// FULLSCREEN EFFECTS
	// *******************

	ltiT = cgs.clientinfo[cg.snap->ps.clientNum].lastTeleInTime;
	waterTime = cgs.clientinfo[cg.snap->ps.clientNum].lastWaterClearTime;
#define WETSCREEN_FADETIME 3000 // hm.. where could this go.. :X

	// teleport effect
	if (!cg.renderingThirdPerson && !(cg.snap->ps.eFlags & EF_DEAD) && ltiT && cg.time - ltiT < 500) {
		vec4_t tmpcolor = {1, 1, 1, 1};
		int team = cg.snap->ps.persistant[PERS_TEAM];

		tmpcolor[0] = tmpcolor[1] = tmpcolor[2] = tmpcolor[3] = 1.0f - ((float)(cg.time - ltiT) / 500.0f);

		trap_R_SetColor(tmpcolor);
		if (team == TEAM_RED)
			trap_R_DrawStretchPic(cg.refdef.x, cg.refdef.y, cg.refdef.width, cg.refdef.height, 0, 0, 1, 1,
								  cgs.media.teleEffectFPRedShader);
		else if (team == TEAM_BLUE)
			trap_R_DrawStretchPic(cg.refdef.x, cg.refdef.y, cg.refdef.width, cg.refdef.height, 0, 0, 1, 1,
								  cgs.media.teleEffectFPBlueShader);
		else if (team == TEAM_FREE)
			trap_R_DrawStretchPic(cg.refdef.x, cg.refdef.y, cg.refdef.width, cg.refdef.height, 0, 0, 1, 1,
								  cgs.media.teleEffectFPGreenShader);
		trap_R_SetColor(NULL);
	}

	// berserk effect
	if (!cg.renderingThirdPerson && cg.snap->ps.powerups[PW_BERSERKER]) {
		trap_R_DrawStretchPic(cg.refdef.x, cg.refdef.y, cg.refdef.width, cg.refdef.height, 0, 0, 1, 1,
							  cgs.media.BerserkerScreenShader);
	} else if (cg.damageValue && (cg.time - cg.damageTime) < DAMAGE_TIME) {
		vec4_t tmpcolor = {1, 1, 1, 1};

		tmpcolor[3] = 0.75f - 0.75f * ((float)(cg.time - cg.damageTime) / (float)DAMAGE_TIME);

		trap_R_SetColor(tmpcolor);
		trap_R_DrawStretchPic(cg.refdef.x, cg.refdef.y, cg.refdef.width, cg.refdef.height, 0, 0, 1, 1,
							  cgs.media.BloodScreenShader);
		trap_R_SetColor(NULL);
	}

	// wet-screen effect
	// (only when no other effect -besides teleport- is already being drawn, since these are kind of heavy)
	else if (cg.time > WETSCREEN_FADETIME + 1 && !cg.renderingThirdPerson && cg.time - waterTime < WETSCREEN_FADETIME) {
		vec4_t color;

		// fade out all channels uniformly ( sounds like a good idea for an "add" shader.. )
		color[0] = color[1] = color[2] = color[3] = 1.0f - ((float)((cg.time - waterTime) / (float)WETSCREEN_FADETIME));

		trap_R_SetColor(color);
		trap_R_DrawStretchPic(cg.refdef.x, cg.refdef.y, cg.refdef.width, cg.refdef.height, 0, 0, 1, 1,
							  cgs.media.WetScreenShader);
		trap_R_SetColor(NULL);
	} else if (!cg.renderingThirdPerson && FT_LocalIsFrozen()) {
		trap_R_DrawStretchPic(cg.refdef.x, cg.refdef.y, cg.refdef.width, cg.refdef.height, 0, 0, 1, 1,
							  cgs.media.FreezeScreenShader);
	}

	// ******************

	// draw status bar and other floating elements
	CG_Draw2D(stereoView);

	// Draw chat messages and icons
	notifytime = CG_GetCvarInt("con_notifytime");
	// We draw console messages ourself, if >0 it's engine's task!
	if (notifytime <= 0) {
		int noprint = CG_GetCvarInt("cl_noprint");
		// needed to calculate properly
		notifytime *= -1;

		if (cg_draw2D.integer && !(noprint) && !(trap_Key_GetCatcher() & KEYCATCH_MESSAGE)) {
			int i = cg.lastchatmsg;
			int j = 0;

			do {
				i++;
				if (i >= MAX_CHATMESSAGES) {
					i = 0;
				}

				// skip inital messages
				if (cg.chatmsgtime[i] && (cg.time - cg.chatmsgtime[i]) < (notifytime * 1000)) {
// TODO: Add a new cvar cg_drawChatIcons?
#define CHAT_ICONSIZE 14
#define CHAT_PADDING 1
#define CHAT_CHARHEIGHT (CHAT_ICONSIZE - (2 * CHAT_PADDING))
#define CHAT_CHARWIDTH (CHAT_CHARHEIGHT / 2)
					if (cg.chaticons[i]) {
						CG_DrawPic(CHAT_PADDING, (j * CHAT_ICONSIZE), CHAT_ICONSIZE, CHAT_ICONSIZE, cg.chaticons[i]);
					}
					// TODO: Create a cvar for fontsize (also adjust icon-size)?
					// TODO: This does not support newlines (see "hotfix" in CG_DrawChar() )
					//       or linewrapping. On the other hand, long text crashes the game anyways..
					CG_DrawStringExt((cg.chaticons[i] ? (CHAT_ICONSIZE + (2 * CHAT_PADDING)) : CHAT_PADDING),
									 ((j * CHAT_ICONSIZE) + ((CHAT_ICONSIZE - CHAT_CHARHEIGHT) / 2)), cg.chattext[i],
									 colorWhite, qfalse, qtrue, CHAT_CHARWIDTH, CHAT_CHARHEIGHT,
									 strlen(cg.chattext[i]));

					j++;
				}
			} while (i != cg.lastchatmsg);
		}
	}

	if (cg_cineDrawLetterBox.integer) {
		vec4_t color = {0, 0, 0, 1}; // black
		int height = 70;

		CG_FillRect(0, 0, 640, height, color);
		CG_FillRect(0, 480 - height, 640, height, color);
	}

	// FIXME? move it, so it isn't usable outside of cutscenes
	CG_Cutscene2d_UpdateTrans();
	CG_Cutscene2d_Draw();
}
