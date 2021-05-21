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
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

static int forceModelModificationCount = -1;
static int glowModelModificationCount = -1;
static int glowModelTeamModificationCount = -1;
static int drawTeamOverlayModificationCount = -1;

void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum);
void CG_Shutdown(void);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7,
						 int arg8, int arg9, int arg10, int arg11) {
	switch (command) {
	case CG_INIT:
		CG_Init(arg0, arg1, arg2);
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame(arg0, arg1, arg2);
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	default:
		CG_Error("vmMain: unknown command %i", command);
		break;
	}
	return -1;
}

cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];
itemInfo_t cg_items[MAX_ITEMS];

vmCvar_t cg_centertime;
vmCvar_t cg_runpitch;
vmCvar_t cg_runroll;
vmCvar_t cg_bobup;
vmCvar_t cg_bobpitch;
vmCvar_t cg_bobroll;
vmCvar_t cg_swingSpeed;
vmCvar_t cg_shadows;
vmCvar_t cg_drawTimer;
vmCvar_t cg_drawFPS;
vmCvar_t cg_drawSnapshot;
vmCvar_t cg_draw3dIcons;
vmCvar_t cg_drawIcons;
vmCvar_t cg_drawAmmoWarning;
vmCvar_t cg_drawCrosshair;
vmCvar_t cg_drawCrosshairNames;
vmCvar_t cg_drawRewards;
vmCvar_t cg_crosshairSize;
vmCvar_t cg_crosshairX;
vmCvar_t cg_crosshairY;
vmCvar_t cg_crosshairHealth;
vmCvar_t cg_draw2D;
vmCvar_t cg_drawStatus;
vmCvar_t cg_animSpeed;
vmCvar_t cg_debugAnim;
vmCvar_t cg_debugPosition;
vmCvar_t cg_debugEvents;
vmCvar_t cg_errorDecay;
vmCvar_t cg_nopredict;
vmCvar_t cg_noPlayerAnims;
vmCvar_t cg_showmiss;
vmCvar_t cg_footsteps;
vmCvar_t cg_addMarks;
vmCvar_t cg_drawGun;
vmCvar_t cg_viewsize;
vmCvar_t cg_gun_frame;
vmCvar_t cg_gun_x;
vmCvar_t cg_gun_y;
vmCvar_t cg_gun_z;
vmCvar_t cg_tracerChance;
vmCvar_t cg_tracerWidth;
vmCvar_t cg_tracerLength;
vmCvar_t cg_autoswitch;
vmCvar_t cg_ignore;
vmCvar_t cg_simpleItems;
vmCvar_t cg_fov;
vmCvar_t cg_zoomFov;
vmCvar_t cg_thirdPerson;
vmCvar_t cg_thirdPersonRange;
vmCvar_t cg_thirdPersonAngle;
vmCvar_t cg_lagometer;
vmCvar_t cg_drawAttacker;
vmCvar_t cg_synchronousClients;
vmCvar_t cg_chatBeep;
vmCvar_t cg_teamChatTime;
vmCvar_t cg_teamChatHeight;
vmCvar_t cg_stats;
vmCvar_t cg_buildScript;
vmCvar_t cg_forceModel;
vmCvar_t cg_paused;
vmCvar_t cg_predictItems;
vmCvar_t cg_deferPlayers;
vmCvar_t cg_drawTeamOverlay;
vmCvar_t cg_teamOverlayUserinfo;
vmCvar_t cg_drawFriend;
vmCvar_t cg_teamChatsOnly;
vmCvar_t cg_hudFiles;
vmCvar_t cg_scorePlum;
vmCvar_t cg_smoothClients;
vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;
vmCvar_t cg_pmove_msec;
vmCvar_t cg_cameraMode;
vmCvar_t cg_cameraOrbit;
vmCvar_t cg_cameraOrbitDelay;
vmCvar_t cg_timescaleFadeEnd;
vmCvar_t cg_timescaleFadeSpeed;
vmCvar_t cg_timescale;
vmCvar_t cg_smallFont;
vmCvar_t cg_bigFont;
vmCvar_t cg_noTaunt;
vmCvar_t cg_noProjectileTrail;

vmCvar_t cg_correctedWeaponPos;
vmCvar_t cg_drawRealTime;
vmCvar_t cg_printDir;
vmCvar_t cg_wopFFAhud;
vmCvar_t cg_drawups;
vmCvar_t cg_drawMessages;
vmCvar_t cg_drawServerInfos;
vmCvar_t cg_drawTimeLeft;
vmCvar_t cg_thirdPersonAutoSwitch;
vmCvar_t cg_bigScoreType;
vmCvar_t cg_LPSwallhackSize;
vmCvar_t cg_LPSwallhackAlpha;

vmCvar_t cg_cineHideHud; // FIXME: would be better to rename it to "cutscene"-mode
vmCvar_t cg_cineDrawLetterBox;

vmCvar_t cg_glowModel;
vmCvar_t cg_glowModelTeam;

vmCvar_t cg_warmupReady;
vmCvar_t cg_curWarmupReady;

vmCvar_t cg_sky;
vmCvar_t cg_skyLensflare;

vmCvar_t cg_timestamps;

vmCvar_t cg_drawLensflare;
vmCvar_t cg_drawVoiceNames;

vmCvar_t cg_drawBBox;

vmCvar_t cg_ambient;

vmCvar_t cg_icons;

// freezetag
vmCvar_t cg_ft_thawerIconX;
vmCvar_t cg_ft_thawerIconY;

typedef struct {
	vmCvar_t *vmCvar;
	const char *cvarName;
	const char *defaultString;
	int cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = {	   // bk001129
	{&cg_ignore, "cg_ignore", "0", 0}, // used for debugging
	{&cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE},
	{&cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE},
	{&cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE},
	{&cg_fov, "cg_fov", "90", CVAR_ARCHIVE},
	{&cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE},
	{&cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE},
	{&cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE},
	{&cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE},
	{&cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE},
	{&cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE},
	{&cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE},
	{&cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE},
	{&cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE},
	{&cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE},
	{&cg_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE},
	{&cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE},
	{&cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE},
	{&cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE},
	{&cg_crosshairHealth, "cg_crosshairHealth", "0", CVAR_ARCHIVE},
	{&cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE},
	{&cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE},
	{&cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE},
	{&cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE},
	{&cg_lagometer, "cg_lagometer", "0", CVAR_ARCHIVE},
	{&cg_gun_x, "cg_gunX", "0", CVAR_CHEAT},
	{&cg_gun_y, "cg_gunY", "0", CVAR_CHEAT},
	{&cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT},
	{&cg_centertime, "cg_centertime", "3", CVAR_CHEAT},
	{&cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{&cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE},
	{&cg_bobup, "cg_bobup", "0.005", CVAR_CHEAT},
	{&cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE},
	{&cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE},
	{&cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT},
	{&cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT},
	{&cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT},
	{&cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT},
	{&cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT},
	{&cg_errorDecay, "cg_errordecay", "100", 0},
	{&cg_nopredict, "cg_nopredict", "0", 0},
	{&cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT},
	{&cg_showmiss, "cg_showmiss", "0", 0},
	{&cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT},
	{&cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT},
	{&cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT},
	{&cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT},
	{&cg_thirdPersonRange, "cg_thirdPersonRange", "100", CVAR_CHEAT},
	{&cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT},
	{&cg_thirdPerson, "cg_thirdPerson", "0", 0},
	{&cg_chatBeep, "cg_chatBeep", "7", CVAR_ARCHIVE},
	{&cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE},
	{&cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE},
	{&cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE},
	{&cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE},
	{&cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE},
	{&cg_drawTeamOverlay, "cg_drawTeamOverlay", "4", CVAR_ARCHIVE},
	{&cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO},
	{&cg_stats, "cg_stats", "0", 0},
	{&cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE},
	{&cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE},
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{&cg_buildScript, "com_buildScript", "0", 0}, // force loading of all possible data amd error on failures
	{&cg_paused, "cl_paused", "0", CVAR_ROM},
	{&cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO}, // communicated by systeminfo
	{&cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{&cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{&cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{&cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{&cg_timescale, "timescale", "1", 0},
	{&cg_scorePlum, "cg_scorePlums", "1", CVAR_ARCHIVE},
	{&cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{&pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO},
	{&pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO},
	{&cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},
	{&cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE},
	{&cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{&cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},

	{&cg_correctedWeaponPos, "cg_correctedWeaponPos", "1", CVAR_ARCHIVE},
	{&cg_drawRealTime, "cg_drawRealTime", "0", CVAR_ARCHIVE},
	{&cg_printDir, "cg_printDir", "0", CVAR_ARCHIVE},
	{&cg_wopFFAhud, "cg_wopFFAhud", "0", CVAR_ARCHIVE},
	{&cg_drawups, "cg_drawups", "0", CVAR_ARCHIVE},
	{&cg_drawMessages, "cg_drawMessages", "1", CVAR_ARCHIVE},
	{&cg_drawServerInfos, "cg_drawServerInfos", "0", CVAR_ARCHIVE},
	{&cg_drawTimeLeft, "cg_drawTimeLeft", "0", CVAR_ARCHIVE},
	{&cg_thirdPersonAutoSwitch, "cg_thirdPersonAutoSwitch", "0", CVAR_ARCHIVE},
	{&cg_bigScoreType, "cg_bigScoreType", "0", 0},
	{&cg_LPSwallhackSize, "cg_LPSwallhackSize", "1.0", CVAR_ARCHIVE},
	{&cg_LPSwallhackAlpha, "cg_LPSwallhackAlpha", "1.0", CVAR_ARCHIVE},

	{&cg_cineHideHud, "cg_CINE_hideHUD", "0", CVAR_TEMP},
	{&cg_cineDrawLetterBox, "cg_CINE_drawLetterBox", "0", CVAR_TEMP},

	{&cg_glowModel, "cg_glowModel", "", CVAR_ARCHIVE},
	{&cg_glowModelTeam, "cg_glowModelTeam", "", CVAR_ARCHIVE},

	/* NOTE: We can't easily extend CS_WARMUP as SV_MapRestart_f() directly sets it */
	{&cg_warmupReady, "g_warmupReady", "", CVAR_SYSTEMINFO},
	{&cg_curWarmupReady, "g_curWarmupReady", "", CVAR_SYSTEMINFO},

	{&cg_sky, "g_sky", "", CVAR_SYSTEMINFO},
	{&cg_skyLensflare, "g_skyLensflare", "", CVAR_SYSTEMINFO},

	{&cg_timestamps, "cg_timestamps", "0", CVAR_ARCHIVE},

	{&cg_drawLensflare, "cg_drawLensflare", "1", CVAR_ARCHIVE},
	{&cg_drawVoiceNames, "cg_drawVoiceNames", "1", CVAR_ARCHIVE},

	{&cg_drawBBox, "cg_drawBBox", "0", CVAR_CHEAT},

	{&cg_ambient, "cg_ambient", "1", CVAR_ARCHIVE},

	// freezetag
	{&cg_ft_thawerIconX, "cg_FT_thawerIconX", "320", CVAR_ARCHIVE},
	{&cg_ft_thawerIconY, "cg_FT_thawerIconY", "290", CVAR_ARCHIVE},

	// Should match ICON_ALL
	{&cg_icons, "cg_icons", "31", CVAR_ARCHIVE}};

static const int cvarTableSize = ARRAY_LEN(cvarTable);

int CG_GetCvarInt(const char *cvar) {
	char buffer[32];

	trap_Cvar_VariableStringBuffer(cvar, buffer, 32);

	return atoi(buffer);
}

float CG_GetCvarFloat(const char *cvar) {
	char buffer[32];

	trap_Cvar_VariableStringBuffer(cvar, buffer, 32);

	return atof(buffer);
}

/*
=================
CG_RegisterCvars
=================
*/
static void CG_RegisterCvars(void) {
	int i;
	cvarTable_t *cv;
	char var[MAX_TOKEN_CHARS];

	for (i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++) {
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
	cgs.localServer = atoi(var);

	forceModelModificationCount = cg_forceModel.modificationCount;

	glowModelModificationCount = cg_glowModel.modificationCount;
	glowModelTeamModificationCount = cg_glowModelTeam.modificationCount;

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "headmodel", DEFAULT_HEADMODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEADMODEL, CVAR_USERINFO | CVAR_ARCHIVE);
}

/*
===================
CG_ForceModelChange
===================
*/
void CG_ForceModelChange(void) {
	int i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		const char *clientInfo;

		clientInfo = CG_ConfigString(CS_PLAYERS + i);
		if (!clientInfo[0]) {
			continue;
		}
		CG_NewClientInfo(i);
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars(void) {
	int i;
	cvarTable_t *cv;

	for (i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++) {
		trap_Cvar_Update(cv->vmCvar);
	}

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If it's off,
	// let the server know so we don't receive it
	if (drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if (cg_drawTeamOverlay.integer > 0) {
			trap_Cvar_Set("teamoverlay", "1");
		} else {
			trap_Cvar_Set("teamoverlay", "0");
		}
	}

	// if force model changed
	if (forceModelModificationCount != cg_forceModel.modificationCount) {
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}

	// if glow colors changed
	if (glowModelModificationCount != cg_glowModel.modificationCount) {
		glowModelModificationCount = cg_glowModel.modificationCount;
		CG_ForceModelChange();
	}
	if (glowModelTeamModificationCount != cg_glowModelTeam.modificationCount) {
		glowModelTeamModificationCount = cg_glowModelTeam.modificationCount;
		CG_ForceModelChange();
	}

	CG_LimitCvars();
}

int CG_CrosshairPlayer(void) {
	if (cg.time > (cg.crosshairClientTime + 1000)) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker(void) {
	if (!cg.attackerTime) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Printf(const char *msg, ...) {
	va_list argptr;
	char text[1024];
	char timestamp[16];
	char buff[1024];

	// FIXME: Buffer sizes need to be adjusted. Also note that chattext is only MAX_SAY_TEXT long

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	/*
		{

			char	*msgptr;
			int		msglen;
			qboolean firstpart;
			int		lastcolor;
			char	*tmpcp, *tmpcp2;

			msgptr=text;
			msglen=strlen(text);
			firstpart=qtrue;
			lastcolor=-1;

			do {
				cg.lastchatmsg++;
				if ( cg.lastchatmsg > MAX_CHATMESSAGES ) {
					cg.lastchatmsg = 0;
				}

				if(lastcolor==-1)
					Q_strncpyz(cg.chattext[cg.lastchatmsg],msgptr,70);//old:62 //61 Zeichen + '\0'
				else
				{
					cg.chattext[cg.lastchatmsg][0]='^';
					cg.chattext[cg.lastchatmsg][1]=lastcolor;
					Q_strncpyz((cg.chattext[cg.lastchatmsg]+2),msgptr,70);
				}

				if(!firstpart || cg.time!=cg.chatmsgtime[cg.lastchatmsg])
				{
					cg.chaticons[cg.lastchatmsg]=0;
					cg.chatmsgtime[cg.lastchatmsg]=cg.time;
				}

				msgptr+=70;
				msglen-=70;
				firstpart=qfalse;

				tmpcp2=NULL;
				tmpcp=cg.chattext[cg.lastchatmsg];
				while(tmpcp=strchr(tmpcp+1,'^'))
				{
					tmpcp2=tmpcp;
				}
				if(tmpcp2!=NULL)
				{
					if(tmpcp2[1]>='0' && tmpcp2[1]<='9') lastcolor=tmpcp2[1];
					else if(tmpcp2[1]='^') tmpcp++;
				}

			} while ( msglen > 0 );

			cg.chattext[cg.lastchatmsg][strlen(cg.chattext[cg.lastchatmsg])-1]='\0';//da ist noch ein '_' am ende
		}
	*/

	// If we draw messages ourself (with player icons etc.),
	// when con_notifytime is negative,
	// we also need do draw any prints ourself.
	// TODO: Add an if for copy (trap_Print needs still to be called for console, logfile etc.)?
	cg.lastchatmsg++;
	if (cg.lastchatmsg >= MAX_CHATMESSAGES) {
		cg.lastchatmsg = 0;
	}
	cg.chaticons[cg.lastchatmsg] = 0;
	if (Q_strncmp(text, "[skipnotify]", 12) == 0) {
		cg.chatmsgtime[cg.lastchatmsg] = 0;
	} else {
		cg.chatmsgtime[cg.lastchatmsg] = cg.time;
	}

	CG_Timestamp(timestamp, sizeof(timestamp));
	Q_strncpyz(buff, timestamp, sizeof(buff));

	Q_strcat(buff, sizeof(buff), text);

	Q_strncpyz(cg.chattext[cg.lastchatmsg], buff, sizeof(cg.chattext[cg.lastchatmsg]));

	trap_Print(buff);
}

void QDECL CG_Error(const char *msg, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Error(text);
}

#ifndef CGAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link (FIXME)

void QDECL Com_Error(int level, const char *error, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL Com_Printf(const char *msg, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

#endif

/*
================
CG_Argv
================
*/
const char *CG_Argv(int arg) {
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds(int itemNum) {
	gitem_t *item;
	char data[MAX_QPATH];
	const char *s, *start;
	int len;

	item = &bg_itemlist[itemNum];

	if (item->pickup_sound) {
		trap_S_RegisterSound(item->pickup_sound, qfalse);
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s - start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error("PrecacheItem: %s has bad precache string", item->classname);
			return;
		}
		memcpy(data, start, len);
		data[len] = 0;
		if (*s) {
			s++;
		}

		trap_S_RegisterSound(data, qfalse);
	}
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds(void) {
	int i;
	char items[MAX_ITEMS + 1];
	char name[MAX_QPATH];
	const char *soundName;

	cgs.media.CountDown_FiveMinutesToPlay = trap_S_RegisterSound("sounds/countdown/five_minutes", qtrue);
	cgs.media.CountDown_OneMinuteToPlay = trap_S_RegisterSound("sounds/countdown/one_minute", qtrue);
	cgs.media.CountDown_GameOver = trap_S_RegisterSound("sounds/countdown/game_over", qtrue);

	cgs.media.Announcer_Welcome[0] = trap_S_RegisterSound("sounds/gametype/welcome_padworld", qtrue);
	cgs.media.Announcer_Welcome[1] = trap_S_RegisterSound("sounds/gametype/welcome", qtrue);
	cgs.media.Announcer_Welcome[2] = trap_S_RegisterSound("sounds/gametype/lets_rock", qtrue);

	cgs.media.count3Sound = trap_S_RegisterSound("sounds/countdown/numbers/three", qtrue);
	cgs.media.count2Sound = trap_S_RegisterSound("sounds/countdown/numbers/two", qtrue);
	cgs.media.count1Sound = trap_S_RegisterSound("sounds/countdown/numbers/one", qtrue);
	cgs.media.countFightSound = trap_S_RegisterSound("sounds/gametype/your_game", qtrue);

	cgs.media.pickupSound = trap_S_RegisterSound("sounds/weapons/weapon_pickup", qfalse);
	cgs.media.HIpickupSound = trap_S_RegisterSound("sounds/items/holdable_pickup", qfalse);
	cgs.media.ARpickupSound = trap_S_RegisterSound("sounds/items/armor/padshield_pickup", qfalse);

	cgs.media.suddenDeathSound = trap_S_RegisterSound("sounds/gametype/sudden_death", qtrue);

	if ((cgs.gametype == GT_SPRAYFFA || cgs.gametype == GT_SPRAY) || cg_buildScript.integer) {
		cgs.media.Announcer_SRfrag[0] = trap_S_RegisterSound("sounds/gametype/syc/nice_day", qtrue);
		cgs.media.Announcer_SRfrag[1] = trap_S_RegisterSound("sounds/gametype/syc/goodbye", qtrue);

		cgs.media.Announcer_SprayYourColor = trap_S_RegisterSound("sounds/gametype/syc/syc", qtrue);

		cgs.media.wrongLogo[0] = trap_S_RegisterSound("sounds/gametype/syc/colorblind", qtrue);
		cgs.media.wrongLogo[1] = trap_S_RegisterSound("sounds/gametype/syc/omg", qtrue);
		cgs.media.wrongLogo[2] = trap_S_RegisterSound("sounds/gametype/syc/stupid_sprayer", qtrue);
		cgs.media.wrongLogo[3] = trap_S_RegisterSound("sounds/gametype/syc/who_is", qtrue);

		cgs.media.CountDown_CountDown[0] = cgs.media.count1Sound;
		cgs.media.CountDown_CountDown[1] = cgs.media.count2Sound;
		cgs.media.CountDown_CountDown[2] = cgs.media.count3Sound;
		cgs.media.CountDown_CountDown[3] = trap_S_RegisterSound("sounds/countdown/numbers/four", qtrue);
		cgs.media.CountDown_CountDown[4] = trap_S_RegisterSound("sounds/countdown/numbers/five", qtrue);
		cgs.media.CountDown_CountDown[5] = trap_S_RegisterSound("sounds/countdown/numbers/six", qtrue);
		cgs.media.CountDown_CountDown[6] = trap_S_RegisterSound("sounds/countdown/numbers/seven", qtrue);
		cgs.media.CountDown_CountDown[7] = trap_S_RegisterSound("sounds/countdown/numbers/eight", qtrue);
		cgs.media.CountDown_CountDown[8] = trap_S_RegisterSound("sounds/countdown/numbers/nine", qtrue);
		cgs.media.CountDown_CountDown[9] = trap_S_RegisterSound("sounds/countdown/numbers/ten", qtrue);
		cgs.media.CountDown_TenSecondsToLeave = trap_S_RegisterSound("sounds/countdown/syc/leave", qtrue);

		cgs.media.spraygodSound = trap_S_RegisterSound("sounds/gametype/syc/spraygod", qtrue);
		cgs.media.spraykillerSound = trap_S_RegisterSound("sounds/gametype/syc/spraykiller", qtrue);
	}

	if (cgs.gametype == GT_SPRAYFFA || cg_buildScript.integer) {
		cgs.media.oneLogoLeft = trap_S_RegisterSound("sounds/countdown/syc/one_logo", qtrue);
		cgs.media.twoLogosLeft = trap_S_RegisterSound("sounds/countdown/syc/two_logos", qtrue);
		cgs.media.threeLogosLeft = trap_S_RegisterSound("sounds/countdown/syc/three_logos", qtrue);
	}

	if (cgs.gametype < GT_TEAM || cg_buildScript.integer) {
		cgs.media.loseFFASound = trap_S_RegisterSound("sounds/gametype/dm/better_luck", qtrue);

		cgs.media.takenLeadSound = trap_S_RegisterSound("sounds/gametype/dm/lead_taken", qtrue);
		cgs.media.tiedLeadSound = trap_S_RegisterSound("sounds/gametype/dm/lead_tied", qtrue);
		cgs.media.lostLeadSound = trap_S_RegisterSound("sounds/gametype/dm/lead_lost", qtrue);
	}

	if (cgs.gametype < GT_CTF || cg_buildScript.integer) {
		if (cgs.gametype != GT_SPRAYFFA || cg_buildScript.integer) {
			cgs.media.oneFragSound = trap_S_RegisterSound("sounds/countdown/dm/one_point", qtrue);
			cgs.media.twoFragSound = trap_S_RegisterSound("sounds/countdown/dm/two_points", qtrue);
			cgs.media.threeFragSound = trap_S_RegisterSound("sounds/countdown/dm/three_points", qtrue);
		}
	}

	if (cgs.gametype == GT_LPS || cg_buildScript.integer) {
		cgs.media.winLPSSounds[0] = trap_S_RegisterSound("sounds/gametype/lps/you_win", qtrue);
		cgs.media.winLPSSounds[1] = trap_S_RegisterSound("sounds/gametype/lps/won_night", qtrue);
		cgs.media.YouLooseSound = trap_S_RegisterSound("sounds/gametype/lps/you_lose", qtrue);
	}

	if (cgs.gametype == GT_BALLOON || cg_buildScript.integer) {
		cgs.media.Announcer_RedBalloon = trap_S_RegisterSound("sounds/gametype/bb/red_balloon", qtrue);
		cgs.media.Announcer_BlueBalloon = trap_S_RegisterSound("sounds/gametype/bb/blue_balloon", qtrue);
		cgs.media.Announcer_BigBalloonRed = trap_S_RegisterSound("sounds/gametype/bb/bigballoon_red", qtrue);
		cgs.media.Announcer_BigBalloonBlue = trap_S_RegisterSound("sounds/gametype/bb/bigballoon_blue", qtrue);
		cgs.media.Announcer_BalloonDestroyed = trap_S_RegisterSound("sounds/gametype/bb/balloon_destroyed", qtrue);

		cgs.media.ballonAufblasSound = trap_S_RegisterSound("sounds/gametype/bb/balloon_inflate", qfalse);
		cgs.media.ballonPlatztSound = trap_S_RegisterSound("sounds/gametype/bb/balloon_burst", qfalse);
	}

	if (cgs.gametype >= GT_TEAM || cg_buildScript.integer) {
		cgs.media.redLeadsSound[0] = trap_S_RegisterSound("sounds/gametype/team/red_leads", qtrue);
		cgs.media.redLeadsSound[1] = trap_S_RegisterSound("sounds/gametype/team/red_rules", qtrue);
		cgs.media.redLeadsSound[2] = trap_S_RegisterSound("sounds/gametype/team/red_best", qtrue);
		cgs.media.blueLeadsSound[0] = trap_S_RegisterSound("sounds/gametype/team/blue_leads", qtrue);
		cgs.media.blueLeadsSound[1] = trap_S_RegisterSound("sounds/gametype/team/blue_rules", qtrue);
		cgs.media.blueLeadsSound[2] = trap_S_RegisterSound("sounds/gametype/team/blue_best", qtrue);
		cgs.media.teamsTiedSound = trap_S_RegisterSound("sounds/gametype/team/teams_tied", qtrue);
		cgs.media.hitTeamSound = trap_S_RegisterSound("sounds/gametype/team/hit_teammate", qtrue);

		cgs.media.redScoredSound = trap_S_RegisterSound("sounds/gametype/team/red_rules", qtrue);
		cgs.media.blueScoredSound = trap_S_RegisterSound("sounds/gametype/team/blue_rules", qtrue);

		if (cgs.gametype == GT_CTF || cg_buildScript.integer) {
			cgs.media.captureYourTeamSound = trap_S_RegisterSound("sounds/gametype/ctl/capture_team", qtrue);
			cgs.media.captureOpponentSound = trap_S_RegisterSound("sounds/gametype/ctl/capture_opponent", qtrue);

			cgs.media.returnYourTeamSound = trap_S_RegisterSound("sounds/gametype/ctl/return_team", qtrue);
			cgs.media.returnOpponentSound = trap_S_RegisterSound("sounds/gametype/ctl/return_opponent", qtrue);

			cgs.media.takenYourTeamSound = trap_S_RegisterSound("sounds/gametype/ctl/taken_team", qtrue);
			cgs.media.takenOpponentSound = trap_S_RegisterSound("sounds/gametype/ctl/taken_opponent", qtrue);

			cgs.media.redFlagReturnedSound = trap_S_RegisterSound("sounds/gametype/ctl/red_returned", qtrue);
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound("sounds/gametype/ctl/blue_returned", qtrue);
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound("sounds/gametype/ctl/enemy_lolly", qtrue);
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound("sounds/gametype/ctl/team_lolly", qtrue);

			cgs.media.stolenlollySound = trap_S_RegisterSound("sounds/gametype/ctl/lolly_stolen", qfalse);
			cgs.media.youHaveFlagSound = trap_S_RegisterSound("sounds/gametype/ctl/lolly_you", qtrue);

			cgs.media.bambamExplosionSound = trap_S_RegisterSound("sounds/items/bambam/explosion", qfalse);
			cgs.media.bambamMissileImpact = trap_S_RegisterSound("sounds/items/bambam/impact", qfalse);
		}

		if ((cgs.gametype == GT_CTF) || (cgs.gametype == GT_BALLOON) || cg_buildScript.integer) {
			cgs.media.boomiesExplosionSound = trap_S_RegisterSound("sounds/items/boomies/explosion", qfalse);
		}
	}

	cgs.media.selectSound = trap_S_RegisterSound("sounds/weapons/change", qfalse);
	cgs.media.wearOffSound = trap_S_RegisterSound("sounds/items/powerup_end", qfalse);
	cgs.media.useNothingSound = trap_S_RegisterSound("sounds/items/holdable_nothing", qfalse);

	cgs.media.teleInSound = trap_S_RegisterSound("sounds/world/teleporter_in", qfalse);
	cgs.media.teleOutSound = trap_S_RegisterSound("sounds/world/teleporter_out", qfalse);
	cgs.media.respawnSound = trap_S_RegisterSound("sounds/items/item_respawn", qfalse);

	cgs.media.noAmmoSound = trap_S_RegisterSound("sounds/weapons/noammo", qfalse);

	cgs.media.talkSound = trap_S_RegisterSound("sounds/player/talk", qfalse);
	cgs.media.landSound = trap_S_RegisterSound("sounds/player/land", qfalse);

	cgs.media.hitSound = trap_S_RegisterSound("sounds/hit", qfalse);

	cgs.media.excellentSound = trap_S_RegisterSound("sounds/awards/excellent", qtrue);
	cgs.media.humiliationSound = trap_S_RegisterSound("sounds/awards/padkiller", qtrue);
	cgs.media.padstarSound = trap_S_RegisterSound("sounds/awards/padstar", qtrue);

	cgs.media.watrInSound = trap_S_RegisterSound("sounds/player/water_in", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound("sounds/player/water_out", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound("sounds/player/water_under", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound("sounds/world/jumppad", qfalse);
	cgs.media.DropCartridgeSound = trap_S_RegisterSound("sounds/weapons/ammo/spraypistol/drop", qfalse);
	cgs.media.jumperSound = trap_S_RegisterSound("sounds/items/jumper/boing", qfalse);

	CG_ChangeLoadingProgress(0.4f);

	for (i = 0; i < 4; i++) {
		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/step%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/boot%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound(name, qfalse);

		/*		Com_sprintf (name, sizeof(name), "sounds/player/footsteps/flesh%i", i+1);
				cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound (name, qfalse);

				Com_sprintf (name, sizeof(name), "sounds/player/footsteps/mech%i", i+1);
				cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound (name, qfalse);

				Com_sprintf (name, sizeof(name), "sounds/player/footsteps/energy%i", i+1);
				cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound (name, qfalse);
		*/
		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/splash%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/clank%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/carpet%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_CARPET][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/lattice%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_LATTICE][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/sand%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_SAND][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/soft%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_SOFT][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/wood%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sounds/player/footsteps/snowstep%i", (i + 1));
		cgs.media.footsteps[FOOTSTEP_SNOW][i] = trap_S_RegisterSound(name, qfalse);
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for (i = 1; i < bg_numItems; i++) {
		if (items[i] == '1' || cg_buildScript.integer) {
			CG_RegisterItemSounds(i);
		}
	}

	for (i = 1; i < MAX_SOUNDS; i++) {
		soundName = CG_ConfigString(CS_SOUNDS + i);
		if (!soundName[0]) {
			break;
		}
		if (soundName[0] == '*') {
			continue; // custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound(soundName, qfalse);
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound("sounds/items/floater/flight", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound("sounds/items/padpower/damage3", qfalse);
	cgs.media.BettyExplosion = trap_S_RegisterSound("sounds/weapons/betty/explosion", qfalse);
	cgs.media.pumperexpSound = trap_S_RegisterSound("sounds/weapons/pumper/explosion", qfalse);

	cgs.media.regenSound = trap_S_RegisterSound("sounds/items/revival/heartbeat", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound("sounds/items/powerup_pickup", qfalse);

	cgs.media.station_start = trap_S_RegisterSound("sounds/healthstation/station_start", qfalse);
	cgs.media.station_loop = trap_S_RegisterSound("sounds/healthstation/station_loop", qfalse);
	cgs.media.station_end = trap_S_RegisterSound("sounds/healthstation/station_end", qfalse);
	cgs.media.station_empty = trap_S_RegisterSound("sounds/healthstation/station_empty", qfalse);

	cgs.media.speedyTaunt = trap_S_RegisterSound("sounds/items/speedy/trumpet", qfalse);
}

//===================================================================================

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics(void) {
	int i;
	char items[MAX_ITEMS + 1];
	static const char *sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",	"gfx/2d/numbers/one_32b",  "gfx/2d/numbers/two_32b",   "gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",	"gfx/2d/numbers/five_32b", "gfx/2d/numbers/six_32b",   "gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b", "gfx/2d/numbers/nine_32b", "gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	trap_R_ClearScene();

	CG_LoadingString(cgs.mapname);

	trap_R_LoadWorldMap(cgs.mapname);

	// precache status bar pics
	CG_LoadingString("game media");

	for (i = 0; i < 11; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader(sb_nums[i]);
	}

	for (i = 0; i <= 4; i++) {
		cgs.media.botSkillShaders[i] = trap_R_RegisterShader(va("menu/art/skill%i", (i + 1)));
	}

	cgs.media.BloodScreenShader = trap_R_RegisterShaderNoMip("blood_screen");
	cgs.media.BerserkerScreenShader = trap_R_RegisterShaderNoMip("berserker_screen");
	cgs.media.WetScreenShader = trap_R_RegisterShaderNoMip("wet_screen");

	cgs.media.deferShader = trap_R_RegisterShaderNoMip("gfx/2d/defer");

	cgs.media.scoreboardBG = trap_R_RegisterShaderNoMip("scoreboard/bg");
	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip("scoreboard/name");
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip("scoreboard/ping");
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip("scoreboard/score");
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip("scoreboard/time");
	cgs.media.voiceIcon = trap_R_RegisterShaderNoMip("hud/hint_voicechat");

	if (cgs.gametype == GT_FREEZETAG) {
		cgs.media.FreezeScreenShader = trap_R_RegisterShaderNoMip("gfx/screen/ice");
		cgs.media.freezeIconShader = trap_R_RegisterShader("icons/hint_freeze");
		cgs.media.snowMarkShader = trap_R_RegisterShader("gfx/damage/snow_mrk");
		cgs.media.thawIcon = trap_R_RegisterShader("icons/hint_thaw");
		cgs.media.iceblockModel = trap_R_RegisterModel("models/iceblock");
	}

	cgs.media.healthstationIcon = trap_R_RegisterShaderNoMip("icons/hint_healthstation");

	if (cgs.gametype == GT_LPS || cg_buildScript.integer) {
		cgs.media.scoreboardlivesleft = trap_R_RegisterShaderNoMip("scoreboard/livesleft");
		cgs.media.scoreboardscore_lives = trap_R_RegisterShaderNoMip("scoreboard/score_lives");

		cgs.media.lpsIcon = trap_R_RegisterShaderNoMip("icons/hint_lpsarrow");
		cgs.media.lpsIconLead = trap_R_RegisterShaderNoMip("icons/hint_lpsarrowlead");
	}

	if (cgs.gametype == GT_BALLOON)
		cgs.media.bbBoxIcon = trap_R_RegisterShaderNoMip("icons/hint_balloonbox");

	cgs.media.smokePuffShader = trap_R_RegisterShader("powerupeffect/puff");
	cgs.media.revivalParticleShader = trap_R_RegisterShader("powerupeffect/revival");
	cgs.media.lagometerShader = trap_R_RegisterShader("gfx/2d/lag");
	cgs.media.connectionShader = trap_R_RegisterShader("disconnected");

	cgs.media.waterBubbleShader = trap_R_RegisterShader("waterBubble");

	cgs.media.selectShader = trap_R_RegisterShaderNoMip("gfx/2d/select");

	for (i = 0; i < NUM_CROSSHAIRS; i++) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShaderNoMip(va("gfx/2d/crosshair%c", ('a' + i)));
	}

	cgs.media.noammoShader = trap_R_RegisterShaderNoMip("gfx/2d/noammo");

	// powerup shaders
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility");

	cgs.media.PadPowerShader = trap_R_RegisterShader("powerups/padpower");
	cgs.media.PunchyPadPowerSkin = trap_R_RegisterSkin("punchySkins/PadPowerPunchy.skin");
	cgs.media.BerserkerAura = trap_R_RegisterShader("powerups/berserkerAura");
	cgs.media.BerserkerPunchy = trap_R_RegisterSkin("punchySkins/BerserkerPunchy.skin");
	cgs.media.deadfadeSkin = trap_R_RegisterShader("deadfadeSkin");

	cgs.media.StationRingShader = trap_R_RegisterShader("station/ring");
	cgs.media.HealthStation_Base = trap_R_RegisterModel("models/mapobjects/pad_healthstation/pad_hs_base");
	cgs.media.HealthStation_Cross = trap_R_RegisterModel("models/mapobjects/pad_healthstation/pad_hs_cross");
	cgs.media.HealthStation_Ring = trap_R_RegisterModel("models/mapobjects/pad_healthstation/pad_hs_ring");
	cgs.media.StationLoadingRings = trap_R_RegisterModel("models/teleporter");

	cgs.media.SchaumShader = trap_R_RegisterShader("boaster/schaum");

	CG_ChangeLoadingProgress(0.6f);

	cgs.media.hud_bl[1] = trap_R_RegisterShaderNoMip("hud/bl_red");
	cgs.media.hud_bc[1] = trap_R_RegisterShaderNoMip("hud/bc_red");

	cgs.media.hud_bl[2] = trap_R_RegisterShaderNoMip("hud/bl_blue");
	cgs.media.hud_bc[2] = trap_R_RegisterShaderNoMip("hud/bc_blue");

	if ((cgs.gametype == GT_SPRAY || cgs.gametype == GT_SPRAYFFA) || cg_buildScript.integer) {
		cgs.media.hud_br[0] = trap_R_RegisterShaderNoMip("hud/br");
		cgs.media.hud_br[1] = trap_R_RegisterShaderNoMip("hud/br_red");
		cgs.media.hud_br[2] = trap_R_RegisterShaderNoMip("hud/br_blue");
		cgs.media.hud_br[3] = trap_R_RegisterShaderNoMip("hud/br_green");
		cgs.media.hud_br[4] = trap_R_RegisterShaderNoMip("hud/br_chrome");
		cgs.media.hud_br[5] = trap_R_RegisterShaderNoMip("hud/br_whitemetal");
		cgs.media.hud_br[6] = trap_R_RegisterShaderNoMip("hud/br_rust");
		cgs.media.hud_br[7] = trap_R_RegisterShaderNoMip("hud/br_flower");
		cgs.media.hud_br[8] = trap_R_RegisterShaderNoMip("hud/br_wood");
		cgs.media.hud_br[9] = trap_R_RegisterShaderNoMip("hud/br_airforce");

		cgs.media.sprayroomIcon = trap_R_RegisterShaderNoMip("icons/hint_sprayroom");
	}

	if (cgs.gametype < GT_TEAM || cg_buildScript.integer) {
		cgs.media.hud_bl[0] = trap_R_RegisterShaderNoMip("hud/bl");
		cgs.media.hud_bc[0] = trap_R_RegisterShaderNoMip("hud/bc");

		cgs.media.hud_bl[3] = trap_R_RegisterShaderNoMip("hud/bl_green");
		cgs.media.hud_bc[3] = trap_R_RegisterShaderNoMip("hud/bc_green");

		cgs.media.hud_bl[4] = trap_R_RegisterShaderNoMip("hud/bl_chrome");
		cgs.media.hud_bc[4] = trap_R_RegisterShaderNoMip("hud/bc_chrome");

		cgs.media.hud_bl[5] = trap_R_RegisterShaderNoMip("hud/bl_whitemetal");
		cgs.media.hud_bc[5] = trap_R_RegisterShaderNoMip("hud/bc_whitemetal");

		cgs.media.hud_bl[6] = trap_R_RegisterShaderNoMip("hud/bl_rust");
		cgs.media.hud_bc[6] = trap_R_RegisterShaderNoMip("hud/bc_rust");

		cgs.media.hud_bl[7] = trap_R_RegisterShaderNoMip("hud/bl_flower");
		cgs.media.hud_bc[7] = trap_R_RegisterShaderNoMip("hud/bc_flower");

		cgs.media.hud_bl[8] = trap_R_RegisterShaderNoMip("hud/bl_wood");
		cgs.media.hud_bc[8] = trap_R_RegisterShaderNoMip("hud/bc_wood");

		cgs.media.hud_bl[9] = trap_R_RegisterShaderNoMip("hud/bl_airforce");
		cgs.media.hud_bc[9] = trap_R_RegisterShaderNoMip("hud/bc_airforce");
	} else {
		cgs.media.friendShader = trap_R_RegisterShader("icons/hint_friend");

		if (cgs.gametype == GT_BALLOON || cg_buildScript.integer) {
			cgs.media.hud_bk_balloon_red = trap_R_RegisterShaderNoMip("hud/bk_balloon_red");
			cgs.media.hud_bk_balloon_blue = trap_R_RegisterShaderNoMip("hud/bk_balloon_blue");
			cgs.media.hud_balloon = trap_R_RegisterShaderNoMip("icons/hud_balloon");
			cgs.media.hud_balloon_bar = trap_R_RegisterShaderNoMip("hud/balloonbar");
			cgs.media.boomiesSphereModel = trap_R_RegisterModel("models/weaponsfx/boomiessphere");
			cgs.media.boomiesCoreShader = trap_R_RegisterShader("boomiesCore");
		} else if (cgs.gametype == GT_CTF || cg_buildScript.integer) {
			cgs.media.hud_CTL_bg_red = trap_R_RegisterShaderNoMip("hud/CTL_red");
			cgs.media.hud_CTL_bg_blue = trap_R_RegisterShaderNoMip("hud/CTL_blue");

			cgs.media.redFlagModel = trap_R_RegisterModel("models/ctl/lollipop_red");
			cgs.media.blueFlagModel = trap_R_RegisterModel("models/ctl/lollipop_blue");
			cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip("icons/hud_lolly_red1");
			cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip("icons/hud_lolly_red2");
			cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip("icons/hud_lolly_red3");
			cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip("icons/hud_lolly_blue1");
			cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip("icons/hud_lolly_blue2");
			cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip("icons/hud_lolly_blue3");

			cgs.media.bambamMissileRedShader = trap_R_RegisterShader("bambamMissileRed");
			cgs.media.bambamMissileBlueShader = trap_R_RegisterShader("bambamMissileBlue");
			trap_R_RegisterModel("models/weapons2/bambam/bambam_red");
			trap_R_RegisterModel("models/weapons2/bambam/bambam_blue");

			cgs.media.bambamExplosionLeg = trap_R_RegisterModel("models/weapons2/bambam/bambamExplosionLeg");
			cgs.media.bambamExplosionTorso = trap_R_RegisterModel("models/weapons2/bambam/bambamExplosionTorso");
			cgs.media.bambamImpactDrops = trap_R_RegisterModel("models/weaponsfx/bambamdrop");

			cgs.media.bambamImpactDropsRedShader = trap_R_RegisterShader("models/weaponsfx/bambamdrop_red");
			cgs.media.bambamImpactDropsBlueShader = trap_R_RegisterShader("models/weaponsfx/bambamdrop_blue");

			cgs.media.bambamHealthIconBG = trap_R_RegisterShaderNoMip("models/weapons2/bambam/bamhealth01");
			cgs.media.bambamHealthIcon = trap_R_RegisterShaderNoMip("models/weapons2/bambam/bamhealth02");
		}

		if ((cgs.gametype == GT_CTF) || (cgs.gametype == GT_BALLOON) || cg_buildScript.integer) {
			cgs.media.boomiesSphereModel = trap_R_RegisterModel("models/weaponsfx/boomiessphere");
			cgs.media.boomiesCoreShader = trap_R_RegisterShader("boomiesCore");
		}
	}

	CG_ChangeLoadingProgress(0.7f);

	cgs.media.hud_teammarker = trap_R_RegisterShaderNoMip("hud/teammarker");
	cgs.media.hud_shieldbar = trap_R_RegisterShaderNoMip("hud/shieldbar");
	cgs.media.hud_energybar = trap_R_RegisterShaderNoMip("hud/energybar");
	cgs.media.hud_shieldbar2 = trap_R_RegisterShaderNoMip("hud/shieldbar2");
	cgs.media.hud_energybar2 = trap_R_RegisterShaderNoMip("hud/energybar2");
	cgs.media.hud_shieldglass = trap_R_RegisterShaderNoMip("hud/shield_glass");
	cgs.media.hud_energyglass = trap_R_RegisterShaderNoMip("hud/energy_glass");
	cgs.media.hud_dotfull = trap_R_RegisterShaderNoMip("hud/dotfull");
	cgs.media.hud_dotempty = trap_R_RegisterShaderNoMip("hud/dotempty");

	cgs.media.star = trap_R_RegisterModel("models/weaponsfx/star");

	cgs.media.balloonShader = trap_R_RegisterShader("icons/hint_textchat");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/boom");

	cgs.media.teleportEffectModel = trap_R_RegisterModel("models/special/teleport");
	if (cgs.gametype < GT_TEAM || cg_buildScript.integer) {
		cgs.media.teleportEffectGreenShader = trap_R_RegisterShader("teleEffectGreen");
		cgs.media.teleEffectFPGreenShader = trap_R_RegisterShader("teleEffectGreenFP");
	} else {
		cgs.media.teleportEffectBlueShader = trap_R_RegisterShader("teleEffect");
		cgs.media.teleportEffectRedShader = trap_R_RegisterShader("teleEffectRed");

		cgs.media.teleEffectFPBlueShader = trap_R_RegisterShader("teleEffectBlueFP");
		cgs.media.teleEffectFPRedShader = trap_R_RegisterShader("teleEffectRedFP");
	}

	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip("icons/medal_excellent");
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip("icons/medal_snackattack");
	cgs.media.medalSpraygod = trap_R_RegisterShaderNoMip("icons/medal_spraygod");
	cgs.media.medalSpraykiller = trap_R_RegisterShaderNoMip("icons/medal_spraykiller");
	cgs.media.medalPadStar = trap_R_RegisterShaderNoMip("icons/medal_padstar");

	memset(cg_items, 0, sizeof(cg_items));
	memset(cg_weapons, 0, sizeof(cg_weapons));

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for (i = 1; i < bg_numItems; i++) {
		if (items[i] == '1' || cg_buildScript.integer) {
			CG_LoadingItem(i);
			CG_RegisterItemVisuals(i);
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader("gfx/damage/bullet_mrk");
	cgs.media.burnMarkShader = trap_R_RegisterShader("gfx/damage/burn_med_mrk");
	cgs.media.holeMarkShader = trap_R_RegisterShader("gfx/damage/hole_lg_mrk");
	cgs.media.energyMarkShader = trap_R_RegisterShader("gfx/damage/plasma_mrk");
	cgs.media.shadowMarkShader = trap_R_RegisterShader("markShadow");
	cgs.media.wakeMarkShader = trap_R_RegisterShader("wake");
	cgs.media.waterMarkShader = trap_R_RegisterShader("waterMark");
	cgs.media.kmaMarkShader = trap_R_RegisterShader("kmaMark");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for (i = 1; i < cgs.numInlineModels; i++) {
		char name[10];
		vec3_t mins, maxs;
		int j;

		Com_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlineDrawModel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for (j = 0; j < 3; j++) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
	}

	// register all the server specified models
	for (i = 1; i < MAX_MODELS; i++) {
		const char *modelName;

		modelName = CG_ConfigString(CS_MODELS + i);
		if (!modelName[0]) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel(modelName);
	}

	CG_ClearParticles();
	/*
		for (i=1; i<MAX_PARTICLES_AREAS; i++)
		{
			{
				int rval;

				rval = CG_NewParticleArea ( CS_PARTICLES + i);
				if (!rval)
					break;
			}
		}
	*/
}

/*
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString() {
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}

/*
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients(void) {
	int i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for (i = 0; i < MAX_CLIENTS; i++) {
		const char *clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString(CS_PLAYERS + i);
		if (!clientInfo[0]) {
			continue;
		}
		CG_LoadingClient(i);
		CG_NewClientInfo(i);
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString(int index) {
	if (index < 0 || index >= MAX_CONFIGSTRINGS) {
		CG_Error("CG_ConfigString: bad index: %i", index);
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic(void) {
	const char *s;
	char parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString(CS_MUSIC);
	Q_strncpyz(parm1, COM_Parse(&s), sizeof(parm1));
	Q_strncpyz(parm2, COM_Parse(&s), sizeof(parm2));

	trap_S_StartBackgroundTrack(parm1, parm2);
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum) {
	const char *s;

	// clear everything
	memset(&cgs, 0, sizeof(cgs));
	memset(&cg, 0, sizeof(cg));
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_weapons, 0, sizeof(cg_weapons));
	memset(cg_items, 0, sizeof(cg_items));

	/*
	if ( CG_GetCvarInt( "r_picmip" ) > 2 ) {
		trap_Cvar_Set( "r_picmip", "2" );
	}
	*/

	CG_ChangeLoadingProgress(0.0f);

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader = trap_R_RegisterShader("gfx/2d/WoPascii");
	cgs.media.charsetProp = trap_R_RegisterShaderNoMip("menu/art/font1_prop");
	cgs.media.whiteShader = trap_R_RegisterShader("white");

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	cg.weaponSelect = WP_NIPPER;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;
	cgs.scale1024X = cgs.glconfig.vidWidth / 1024.0f;
	cgs.scale1024Y = cgs.glconfig.vidHeight / 768.0f;

	CG_ChangeLoadingProgress(0.1f);

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);

	// check version
	s = CG_ConfigString(CS_GAME_VERSION);
	if (strcmp(s, GAME_VERSION)) {
		CG_Error("Client/Server game mismatch: %s/%s", GAME_VERSION, s);
	}

	s = CG_ConfigString(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);

	CG_ParseServerinfo();
	CG_ChangeLoadingProgress(0.2f);

	// load the new map
	CG_LoadingString("collision map");

	trap_CM_LoadMap(cgs.mapname);

	CG_ChangeLoadingProgress(0.3f);

	cg.loading = qtrue; // force players to load instead of defer

	CG_LoadingString("sounds");

	CG_RegisterSounds();
	CG_ChangeLoadingProgress(0.5f);

	CG_LoadingString("graphics");

	CG_RegisterGraphics();
	CG_ChangeLoadingProgress(0.8f);

	if (IsSyc() || cg_buildScript.integer) {
		CG_LoadingString("spraylogos");
		Init_SprayLogoSys();
	}

	Init_LensFlareSys();

	{
		char *tmpcptr;

		s = CG_ConfigString(CS_SERVERINFO);
		Q_strncpyz(cg.wopSky, cg_sky.string, sizeof(cg.wopSky));

		tmpcptr = strchr(cg.wopSky, '<');
		if (tmpcptr) {
			*tmpcptr = '\0';
			tmpcptr++;
			sscanf(tmpcptr, "%f %f %f %f %f %f", &cg.wopSky_Angles[0], &cg.wopSky_TimeFactors[0], &cg.wopSky_Angles[1],
				   &cg.wopSky_TimeFactors[1], &cg.wopSky_Angles[2], &cg.wopSky_TimeFactors[2]);
		}
	}

	Init_SpriteParticles();

	CG_LoadingString("clients");

	CG_RegisterClients(); // if low on memory, some clients will be deferred

	cg.loading = qfalse; // future players will be deferred

	CG_ChangeLoadingProgress(0.9f);

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString("");

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds(qtrue);

	CG_ChangeLoadingProgress(1.0f);
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown(void) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}

/*
==================
CG_EventHandling
==================
 type 0 - no event handling
	  1 - team menu
	  2 - hud editor

*/
void CG_EventHandling(int type) {
}

void CG_KeyEvent(int key, qboolean down) {
	if (cg.handleInputMillis < cg.millis) {
		cgs.lastusedkey = key;
	}
}

void CG_MouseEvent(int x, int y) {
	cgs.cursorX += x;
	if (cgs.cursorX < 0) {
		cgs.cursorX = 0;
	} else if (cgs.cursorX > 640) {
		cgs.cursorX = 640;
	}

	cgs.cursorY += y;
	if (cgs.cursorY < 0) {
		cgs.cursorY = 0;
	} else if (cgs.cursorY > 480) {
		cgs.cursorY = 480;
	}
}
