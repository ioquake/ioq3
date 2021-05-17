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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"

void CG_TargetCommand_f(void) {
	int targetNum;
	char test[4];

	targetNum = CG_CrosshairPlayer();
	if (targetNum == -1) {
		return;
	}

	trap_Argv(1, test, 4);
	trap_SendConsoleCommand(va("gc %i %i", targetNum, atoi(test)));
}

/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f(void) {
	trap_Cvar_Set("cg_viewsize", va("%i", (int)(cg_viewsize.integer + 10)));
}

/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f(void) {
	trap_Cvar_Set("cg_viewsize", va("%i", (int)(cg_viewsize.integer - 10)));
}

/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f(void) {
	CG_Printf("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0], (int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2],
			  (int)cg.refdefViewAngles[YAW]);
}

static void CG_ScoresDown_f(void) {
	if (cg.scoresRequestTime + 2000 < cg.time) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand("score");

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if (!cg.showScores) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f(void) {
	if (cg.showScores) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

static void CG_TellTarget_f(void) {
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_CrosshairPlayer();
	if (clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_TellAttacker_f(void) {
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_LastAttacker();
	if (clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_VoiceTellTarget_f(void) {
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_CrosshairPlayer();
	if (clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "vtell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_VoiceTellAttacker_f(void) {
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_LastAttacker();
	if (clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "vtell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f(void) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("developer", var, sizeof(var));
	if (!atoi(var)) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/

static void CG_ReChooseLogo_f(void) {
	cg.wantSelectLogo = qtrue;
	cg.handleInputMillis = cg.millis + 1000;
}

static const char *gameNames[] = {GAMETYPE_NAME(GT_FFA),
								  GAMETYPE_NAME(GT_TOURNAMENT),
								  GAMETYPE_NAME(GT_SINGLE_PLAYER),
								  GAMETYPE_NAME(GT_SPRAYFFA),
								  GAMETYPE_NAME(GT_LPS),
								  GAMETYPE_NAME(GT_TEAM),
								  GAMETYPE_NAME(GT_FREEZETAG),
								  GAMETYPE_NAME(GT_CTF),
								  GAMETYPE_NAME(GT_SPRAY),
								  GAMETYPE_NAME(GT_BALLOON),
								  NULL};

#define MAX_BUFFERLEN 256
static void CG_HelpCmd_f(void) {
	char buffer[MAX_BUFFERLEN];

	if (trap_Argc() > 1) {
		trap_Args(buffer, MAX_BUFFERLEN);
		if (!Q_stricmp(buffer, "g_gametype")) {
			int i;

			Com_Printf("GT# -> gametype:\n");
			for (i = 0; gameNames[i] != NULL; i++)
				Com_Printf("%3i -> %s\n", i, gameNames[i]);
		}
	} else {
		Com_Printf("available help:\n");
		Com_Printf(" g_gametype\n");
	}
}

static void CG_DropCartridge_f(void) {
	//	trap_SendClientCommand("dropCartridge"); // also goes over spamm filter
	trap_SendConsoleCommand("+button12;-button12\n"); // BUTTON_DROPCART
}

static void CG_Cam(void) {
	// SP only
	// if( cgs.gametype != GT_SINGLE_PLAYER )
	//	return;

	// toggle cam mode
	cg.Cam = cg.Cam ? 0 : 1;
	VectorCopy(cg.refdef.vieworg, cg.CamPos);
	VectorCopy(cg.refdefViewAngles, cg.CamAngles);
}

void Cmd_SetFreecamPos_f(void) {
	vec3_t origin, angles;
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if (trap_Argc() < 3) {
		CG_Printf("usage: setviewpos x y z pitch yaw roll\n");
		return;
	}

	VectorClear(angles);

	for (i = 0; i < 3; i++) {
		trap_Argv(i + 1, buffer, sizeof(buffer));
		origin[i] = atof(buffer);
	}

	trap_Argv(5, buffer, sizeof(buffer));
	angles[YAW] = atof(buffer);
	trap_Argv(4, buffer, sizeof(buffer));
	angles[PITCH] = atof(buffer);
	trap_Argv(6, buffer, sizeof(buffer));
	angles[ROLL] = atof(buffer);

	VectorCopy(origin, cg.CamPos);
	VectorCopy(angles, cg.CamAngles);
}

typedef struct {
	const char *cmd;
	void (*function)(void);
} consoleCommand_t;

static consoleCommand_t commands[] = {{"testgun", CG_TestGun_f},
									  {"testmodel", CG_TestModel_f},
									  {"nextframe", CG_TestModelNextFrame_f},
									  {"prevframe", CG_TestModelPrevFrame_f},
									  {"nextskin", CG_TestModelNextSkin_f},
									  {"prevskin", CG_TestModelPrevSkin_f},
									  {"viewpos", CG_Viewpos_f},
									  {"+scores", CG_ScoresDown_f},
									  {"-scores", CG_ScoresUp_f},
									  {"+zoom", CG_ZoomDown_f},
									  {"-zoom", CG_ZoomUp_f},
									  {"sizeup", CG_SizeUp_f},
									  {"sizedown", CG_SizeDown_f},
									  {"weapnext", CG_NextWeapon_f},
									  {"weapprev", CG_PrevWeapon_f},
									  {"weapon", CG_Weapon_f},
									  {"tcmd", CG_TargetCommand_f},
									  {"tell_target", CG_TellTarget_f},
									  {"tell_attacker", CG_TellAttacker_f},
									  {"vtell_target", CG_VoiceTellTarget_f},
									  {"vtell_attacker", CG_VoiceTellAttacker_f},
									  {"startOrbit", CG_StartOrbit_f},
									  //{ "camera", CG_Camera_f },
									  {"loaddeferred", CG_LoadDeferredPlayers},
									  {"help", CG_HelpCmd_f},
									  {"rechooselogo", CG_ReChooseLogo_f},
									  {"dropCartridge", CG_DropCartridge_f},
									  {"dropTeamItem", CG_DropCartridge_f},
									  {"freecamsetpos", Cmd_SetFreecamPos_f},
									  {"freecam", CG_Cam},
									  {"spraydump", DumpPolyInfo}};

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand(void) {
	const char *cmd;
	int i;

	cmd = CG_Argv(0);

	for (i = 0; i < ARRAY_LEN(commands); i++) {
		if (!Q_stricmp(cmd, commands[i].cmd)) {
			commands[i].function();
			return qtrue;
		}
	}

	if (CG_Cutscene2d_CheckCmd(cmd))
		return qtrue;

	return qfalse;
}

/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands(void) {
	int i;

	for (i = 0; i < ARRAY_LEN(commands); i++) {
		trap_AddCommand(commands[i].cmd);
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand("kill");
	trap_AddCommand("say");
	trap_AddCommand("say_team");
	trap_AddCommand("tell");
	trap_AddCommand("give");
	trap_AddCommand("god");
	trap_AddCommand("notarget");
	trap_AddCommand("noclip");
	trap_AddCommand("where");
	trap_AddCommand("team");
	trap_AddCommand("follow");
	trap_AddCommand("follownext");
	trap_AddCommand("followprev");
	trap_AddCommand("levelshot");
	trap_AddCommand("addbot");
	trap_AddCommand("setviewpos");
	trap_AddCommand("callvote");
	trap_AddCommand("cv");
	trap_AddCommand("vote");
	trap_AddCommand("stats");
	trap_AddCommand("loaddefered"); // spelled wrong, but not changing for demo
	trap_AddCommand("rechooselogo");
	trap_AddCommand("ready");
	trap_AddCommand("TeamReady");
	trap_AddCommand("dropCartridge");
	trap_AddCommand("dropTeamItem");
	trap_AddCommand("help");
	trap_AddCommand("spraydump");
}
