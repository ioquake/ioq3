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

#include "g_local.h"

level_locals_t level;

typedef struct {
	vmCvar_t *vmCvar;
	const char *cvarName;
	const char *defaultString;
	int cvarFlags;
	int modificationCount; // for tracking changes
	qboolean trackChange;  // track this variable, and announce if changed
} cvarTable_t;

gentity_t g_entities[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];

vmCvar_t g_gametype;
vmCvar_t g_dmflags;
vmCvar_t g_fraglimit;
vmCvar_t g_timelimit;
vmCvar_t g_capturelimit;
vmCvar_t g_friendlyFire;
vmCvar_t g_password;
vmCvar_t g_needpass;
vmCvar_t g_maxclients;
vmCvar_t g_maxGameClients;
vmCvar_t g_dedicated;
vmCvar_t g_speed;
vmCvar_t g_gravity;
vmCvar_t g_cheats;
vmCvar_t g_knockback;
vmCvar_t g_quadfactor;
vmCvar_t g_forcerespawn;
vmCvar_t g_inactivity;
vmCvar_t g_debugMove;
vmCvar_t g_debugDamage;
vmCvar_t g_debugAlloc;
vmCvar_t g_weaponRespawn;
vmCvar_t g_weaponTeamRespawn;
vmCvar_t g_motd;
vmCvar_t g_synchronousClients;
vmCvar_t g_warmup;
vmCvar_t g_doWarmup;
vmCvar_t g_warmupReady;
vmCvar_t g_curWarmupReady;
vmCvar_t g_restarted;
vmCvar_t g_log;
vmCvar_t g_logSync;
vmCvar_t g_podiumDist;
vmCvar_t g_podiumDrop;
vmCvar_t g_allowVote;
vmCvar_t g_teamAutoJoin;
vmCvar_t g_teamForceBalance;
vmCvar_t g_banIPs;
vmCvar_t g_filterBan;
vmCvar_t g_smoothClients;
vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;
vmCvar_t g_listEntity;

vmCvar_t g_q3Items;
vmCvar_t g_sky;
vmCvar_t g_skyLensflare;
vmCvar_t g_LPS_startlives;
vmCvar_t g_LPS_flags;
vmCvar_t g_KillerduckHealth;
vmCvar_t nextmapBackUp;
vmCvar_t g_transmitSVboastermissiles;
vmCvar_t g_suddenDeath;

// freezetag
vmCvar_t g_ft_numRounds;
vmCvar_t g_ft_playAllRounds;
vmCvar_t g_ft_lateJoinTime;
vmCvar_t g_ft_useWeaponSet;
vmCvar_t g_ft_weaponSetAmmoRatio;
vmCvar_t g_ft_debug;

// Modifiers
vmCvar_t g_modInstagib;
vmCvar_t g_modInstagib_WeaponJump;

vmCvar_t g_logDamage;

// bk001129 - made static to avoid aliasing
static cvarTable_t gameCvarTable[] = {
	// don't override the cheat state set by the system
	{&g_cheats, "sv_cheats", "", 0, 0, qfalse},

	// noset vars
	{NULL, "gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_ROM, 0, qfalse},
	{NULL, "gamedate", __DATE__, CVAR_ROM, 0, qfalse},
	{&g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse},
	{NULL, "sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse},

	// latched vars
	{&g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse},

	{&g_maxclients, "sv_maxclients", "12", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse},
	{&g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse},

	// change anytime vars
	{&g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue},
	{&g_fraglimit, "pointlimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue},
	{&g_timelimit, "timelimit", "10", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue},
	// FIXME: Properly replace capturelimit with pointlimit everywhere
	{&g_capturelimit, "pointlimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qfalse},

	{&g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse},

	{&g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, 0, qtrue},

	{&g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE, 0, qfalse},
	{&g_teamForceBalance, "g_teamForceBalance", "1", CVAR_ARCHIVE, 0, qfalse},

	{&g_warmup, "g_warmup", "20", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qtrue},
	{&g_doWarmup, "g_doWarmup", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue},
	// TODO: Remove g_doWarmup and simply use g_warmup > 0 ?
	{&g_warmupReady, "g_warmupReady", "0.0", (CVAR_SYSTEMINFO | CVAR_ARCHIVE | CVAR_LATCH), 0, qtrue},
	{&g_curWarmupReady, "g_curWarmupReady", "0.0", (CVAR_SYSTEMINFO | CVAR_ROM), 0, qfalse},
	{&g_log, "g_log", "games.log", CVAR_ARCHIVE, 0, qfalse},
	{&g_logSync, "g_logSync", "0", CVAR_ARCHIVE, 0, qfalse},

	{&g_password, "g_password", "", CVAR_USERINFO, 0, qfalse},

	{&g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse},
	{&g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse},

	{&g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse},

	{&g_dedicated, "dedicated", "0", 0, 0, qfalse},

	{&g_speed, "g_speed", DEFAULT_GSPEED_STR, 0, 0, qtrue}, // q3: 320, ef?: 250 ... new wop 280?
	{&g_gravity, "g_gravity", "800", 0, 0, qtrue},
	{&g_knockback, "g_knockback", "1000", 0, 0, qtrue},
	{&g_quadfactor, "g_quadfactor", "2", 0, 0, qtrue},
	{&g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue},
	{&g_weaponTeamRespawn, "g_weaponTeamRespawn", "5", 0, 0, qtrue}, // q3: 30
	{&g_forcerespawn, "g_forcerespawn", "20", 0, 0, qtrue},
	{&g_inactivity, "g_inactivity", "600", 0, 0, qtrue},
	{&g_debugMove, "g_debugMove", "0", 0, 0, qfalse},
	{&g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse},
	{&g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse},
	{&g_motd, "g_motd", "", 0, 0, qfalse},

	{&g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse},
	{&g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse},

	{&g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse},
	{&g_listEntity, "g_listEntity", "0", 0, 0, qfalse},

	{&g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
	/* Still causing issues, maybe use pmove_float from OpenArena instead */
	{&pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qfalse},
	{&pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse},

	{&g_KillerduckHealth, "g_KillerduckHealth", "-1", CVAR_ARCHIVE, 0, qfalse},

	{&g_q3Items, "g_q3Items", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse},
	{&g_sky, "g_sky", "", (CVAR_SYSTEMINFO | CVAR_ROM), 0, qfalse},
	{&g_skyLensflare, "g_skyLensflare", "", (CVAR_SYSTEMINFO | CVAR_ROM), 0, qfalse},
	{&g_LPS_startlives, "g_LPS_startlives", "10", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse},
	// TODO: Remove g_LPS_flags and use dmflags instead, at least as cvarname
	{&g_LPS_flags, "g_LPS_flags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse},
	{&nextmapBackUp, "nextmapBackUp", "", 0, 0, qfalse},
	{&g_transmitSVboastermissiles, "g_transmitSVboastermissiles", "0", 0, 0, qtrue},
	{&g_suddenDeath, "g_suddenDeath", "1", CVAR_ARCHIVE, 0, qfalse},

	// Modifiers
	{&g_modInstagib, "g_instaPad", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qtrue},
	// TODO: Either rename again or allow instapad-weaponjump in non-instapad gameplay :)
	{&g_modInstagib_WeaponJump, "g_weaponJump", "1", CVAR_ARCHIVE, 0, qtrue},

	{&g_ft_numRounds, "g_ft_numRounds", "3", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse},
	{&g_ft_playAllRounds, "g_ft_playAllRounds", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse},
	{&g_ft_lateJoinTime, "g_FT_lateJoinTime", "30", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse},
	{&g_ft_useWeaponSet, "g_FT_useWeaponSet", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse}, // freezetag
	{&g_ft_weaponSetAmmoRatio, "g_FT_weaponSetAmmoRatio", "0.5", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse},
	{&g_ft_debug, "g_FT_debug", "0", 0, 0, qfalse},

	{NULL, PLAYERINFO_TEAM, PLAYERINFO_NONE, (CVAR_SERVERINFO | CVAR_ROM), 0, qfalse},
	{NULL, PLAYERINFO_BOT, PLAYERINFO_NONE, (CVAR_SERVERINFO | CVAR_ROM), 0, qfalse},

	{&g_logDamage, "g_logDamage", "0", CVAR_ARCHIVE, 0, qfalse}};

static const int gameCvarTableSize = ARRAY_LEN(gameCvarTable);

static void G_InitGame(int levelTime, int randomSeed, int restart);
static void G_RunFrame(int levelTime);
static void G_ShutdownGame(int restart);
static void CheckExitRules(void);

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
	case GAME_INIT:
		G_InitGame(arg0, arg1, arg2);
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame(arg0);
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect(arg0, arg1, arg2);
	case GAME_CLIENT_THINK:
		ClientThink(arg0);
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged(arg0);
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect(arg0);
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin(arg0);
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand(arg0);
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame(arg0);
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
		return BotAIStartFrame(arg0);
	}

	return -1;
}

int G_GetCvarInt(const char *cvar) {
	char buffer[32];

	trap_Cvar_VariableStringBuffer(cvar, buffer, 32);

	return atoi(buffer);
}

void QDECL G_Printf(const char *fmt, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Print(text);
}

void QDECL G_Error(const char *fmt, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_Error(text);
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
static void G_FindTeams(void) {
	gentity_t *e, *e2;
	int i, j;
	int c, c2;

	c = 0;
	c2 = 0;
	for (i = 1, e = g_entities + i; i < level.num_entities; i++, e++) {
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for (j = i + 1, e2 = e + 1; j < level.num_entities; j++, e2++) {
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team)) {
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if (e2->targetname) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	G_Printf("%i teams with %i entities\n", c, c2);
}

/*
=================
G_RegisterCvars
=================
*/
static void G_RegisterCvars(void) {
	int i;
	cvarTable_t *cv;

	for (i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++) {
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar)
			cv->modificationCount = cv->vmCvar->modificationCount;
	}

	// check some things
	if (g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE) {
		G_Printf("g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer);
		trap_Cvar_Set("g_gametype", "0");
	}

	level.warmupModificationCount = g_warmup.modificationCount;
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars(void) {
	int i;
	cvarTable_t *cv;

	for (i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++) {
		if (cv->vmCvar) {
			trap_Cvar_Update(cv->vmCvar);

			if (cv->modificationCount != cv->vmCvar->modificationCount) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if (cv->trackChange) {
					// FIXME: This will display the current value instead of the latched one
					trap_SendServerCommand(
						-1, va("print \"Server: %s changed to %s\n\"", cv->cvarName, cv->vmCvar->string));
					G_LogPrintf("CvarChange: %s %s\n", cv->cvarName, cv->vmCvar->string);
				}
			}
		}
	}
}

/*
============
G_InitGame

============
*/
static void G_InitGame(int levelTime, int randomSeed, int restart) {
	int i;

	G_Printf("------- Game Initialization -------\n");
	G_Printf("gamename: %s\n", GAMEVERSION);

	srand(randomSeed);

	G_RegisterCvars();

	G_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	memset(&level, 0, sizeof(level));
	level.time = levelTime;
	level.startTime = levelTime;

	// the wrong vote-cs-info hasn't disrupted the game<->cgame infos because the info in cgs is also cleaned up
	// ... in wop we use this CS directly from the UI
	trap_SetConfigstring(CS_VOTE_TIME, "");
	trap_SetConfigstring(CS_VOTE_STRING, "");

	level.snd_fry = G_SoundIndex("sounds/player/fry"); // FIXME standing in lava / slime

	if (g_gametype.integer != GT_SINGLE_PLAYER && g_log.string[0]) {
		if (g_logSync.integer) {
			trap_FS_FOpenFile(g_log.string, &level.logFile, FS_APPEND_SYNC);
		} else {
			trap_FS_FOpenFile(g_log.string, &level.logFile, FS_APPEND);
		}
		if (!level.logFile) {
			G_Printf("WARNING: Couldn't open logfile: %s\n", g_log.string);
		} else {
			char serverinfo[MAX_INFO_STRING];
			qtime_t qt;

			trap_RealTime(&qt);

			trap_GetServerinfo(serverinfo, sizeof(serverinfo));

			G_LogPrintf("------------------------------------------------------------\n");
			G_LogPrintf("InitGame: %s\n", serverinfo);
			G_LogPrintf("Local time: %02i:%02i:%02i, %02i.%02i.%4i\n", qt.tm_hour, qt.tm_min, qt.tm_sec, qt.tm_mday,
						(qt.tm_mon + 1), (qt.tm_year + 1900));
		}
	} else {
		G_Printf("Not logging to disk.\n");
	}

	G_InitWorldSession();

	// initialize all entities for this game
	memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset(g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]));
	level.clients = g_clients;

	// set client fields on player ents
	for (i = 0; i < level.maxclients; i++) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for (i = 0; i < MAX_CLIENTS; i++) {
		g_entities[i].classname = "clientslot";
	}

	// let the server system know where the entites are
	trap_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_t), &level.clients[0].ps,
						sizeof(level.clients[0]));

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if (g_gametype.integer >= GT_TEAM) {
		G_CheckTeamItems();
	}

	// freezetag
	if (G_FreezeTag()) {
		FT_InitFreezeTag();
	}

	SaveRegisteredItems();

	G_Printf("-----------------------------------\n");

	if (trap_Cvar_VariableIntegerValue("bot_enable")) {
		BotAISetup(restart);
		BotAILoadMap(restart);
		G_LoadBots();
	}

	if (strlen(nextmapBackUp.string) > 0) {
		trap_Cvar_Set("nextmap", nextmapBackUp.string);
		trap_Cvar_Set("nextmapBackUp", "");
	}

	G_InitGameinfo();
}

/*
=================
G_ShutdownGame
=================
*/
static void G_ShutdownGame(int restart) {
	char buff[MAX_CVAR_VALUE_STRING];

	// backup the nextmap so we don't loose maploops on map_restart
	trap_Cvar_VariableStringBuffer("nextmap", buff, sizeof(buff));
	if (!strstr(buff, "map_restart"))
		trap_Cvar_Set("nextmapBackUp", buff);
	else
		trap_Cvar_Set("nextmapBackUp", "");

	G_Printf("==== ShutdownGame ====\n");

	if (level.logFile) {
		G_LogPrintf("ShutdownGame:\n");
		G_LogPrintf("------------------------------------------------------------\n");
		trap_FS_FCloseFile(level.logFile);
		level.logFile = 0;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	if (trap_Cvar_VariableIntegerValue("bot_enable")) {
		BotAIShutdown(restart);
	}
}

//===================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

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
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
static void AddTournamentPlayer(void) {
	int i;
	gclient_t *client;
	gclient_t *nextInLine;

	if (level.numPlayingClients >= 2) {
		return;
	}

	// never change during intermission
	if (level.intermissiontime) {
		return;
	}

	nextInLine = NULL;

	for (i = 0; i < level.maxclients; i++) {
		client = &level.clients[i];
		if (client->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (client->sess.sessionTeam != TEAM_SPECTATOR) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if (client->sess.spectatorState == SPECTATOR_SCOREBOARD || client->sess.spectatorClient < 0) {
			continue;
		}

		if (!nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum)
			nextInLine = client;
	}

	if (!nextInLine) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam(&g_entities[nextInLine - level.clients], "f");
}

/*
=======================
AddTournamentQueue

Add client to end of tournament queue
=======================
*/

void AddTournamentQueue(gclient_t *client) {
	int index;
	gclient_t *curclient;

	for (index = 0; index < level.maxclients; index++) {
		curclient = &level.clients[index];

		if (curclient->pers.connected != CON_DISCONNECTED) {
			if (curclient == client)
				curclient->sess.spectatorNum = 0;
			else if (curclient->sess.sessionTeam == TEAM_SPECTATOR)
				curclient->sess.spectatorNum++;
		}
	}
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
static void RemoveTournamentLoser(void) {
	int clientNum;

	if (level.numPlayingClients != 2) {
		return;
	}

	clientNum = level.sortedClients[1];

	if (level.clients[clientNum].pers.connected != CON_CONNECTED) {
		return;
	}

	// make them a spectator
	SetTeam(&g_entities[clientNum], "s");
}

/*
=======================
AdjustTournamentScores
=======================
*/
static void AdjustTournamentScores(void) {
	int clientNum;

	clientNum = level.sortedClients[0];
	if (level.clients[clientNum].pers.connected == CON_CONNECTED) {
		level.clients[clientNum].sess.wins++;
		ClientUserinfoChanged(clientNum);
	}

	clientNum = level.sortedClients[1];
	if (level.clients[clientNum].pers.connected == CON_CONNECTED) {
		level.clients[clientNum].sess.losses++;
		ClientUserinfoChanged(clientNum);
	}
}

/*
=============
SortRanks

=============
*/
static int QDECL SortRanks(const void *a, const void *b) {
	gclient_t *ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if (ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0) {
		return 1;
	}
	if (cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0) {
		return -1;
	}

	// then connecting clients
	if (ca->pers.connected == CON_CONNECTING) {
		return 1;
	}
	if (cb->pers.connected == CON_CONNECTING) {
		return -1;
	}

	// then spectators
	if (ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR) {
		if (ca->sess.spectatorNum > cb->sess.spectatorNum) {
			return -1;
		}
		if (ca->sess.spectatorNum < cb->sess.spectatorNum) {
			return 1;
		}
		return 0;
	}
	if (ca->sess.sessionTeam == TEAM_SPECTATOR) {
		return 1;
	}
	if (cb->sess.sessionTeam == TEAM_SPECTATOR) {
		return -1;
	}

	if ((GT_LPS == g_gametype.integer) && !(g_LPS_flags.integer & LPSF_PPOINTLIMIT)) {
		// PERS_SCORE already equals livesleft, see fix in WoP_RunFrame
		goto LPS_NOPOINTS;
	}

	// then sort by score
	if (ca->ps.persistant[PERS_SCORE] > cb->ps.persistant[PERS_SCORE]) {
		return -1;
	}
	if (ca->ps.persistant[PERS_SCORE] < cb->ps.persistant[PERS_SCORE]) {
		return 1;
	}

LPS_NOPOINTS:

	if (g_gametype.integer == GT_LPS) {
		if (ca->sess.livesleft > cb->sess.livesleft) {
			return -1;
		}
		if (ca->sess.livesleft < cb->sess.livesleft) {
			return 1;
		}

		// same amount of lives, check for none
		if (ca->sess.livesleft <= 0) {
			if (ca->lastDeathTime > cb->lastDeathTime) {
				return -1;
			}
			if (ca->lastDeathTime < cb->lastDeathTime) {
				return 1;
			}
		}
	}

	return 0;
}

static int QDECL SortRanksLPS(const void *a, const void *b) {
	gclient_t *ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if (ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0) {
		return 1;
	}
	if (cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0) {
		return -1;
	}

	// then connecting clients
	if (ca->pers.connected == CON_CONNECTING) {
		return 1;
	}
	if (cb->pers.connected == CON_CONNECTING) {
		return -1;
	}

	// then spectators
	if (ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR) {
		if (ca->sess.spectatorNum < cb->sess.spectatorNum) {
			return -1;
		}
		if (ca->sess.spectatorNum > cb->sess.spectatorNum) {
			return 1;
		}
		return 0;
	}
	if (ca->sess.sessionTeam == TEAM_SPECTATOR) {
		return 1;
	}
	if (cb->sess.sessionTeam == TEAM_SPECTATOR) {
		return -1;
	}

	// Do NOT sort by score. Furthermore this is always LPS
	if (ca->sess.livesleft > cb->sess.livesleft) {
		return -1;
	}
	if (ca->sess.livesleft < cb->sess.livesleft) {
		return 1;
	}

	// same amount of lives, check for none
	if (ca->sess.livesleft <= 0) {
		if (ca->lastDeathTime > cb->lastDeathTime) {
			return -1;
		}
		if (ca->lastDeathTime < cb->lastDeathTime) {
			return 1;
		}
	}

	return 0;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks(void) {
	int i;
	int rank;
	int score;
	int newScore;
	gclient_t *cl;
	char teaminfo[MAX_CVAR_VALUE_STRING];
	char botinfo[MAX_CVAR_VALUE_STRING];

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0; // don't count bots

	teaminfo[0] = '\0';
	botinfo[0] = '\0';

	for (i = 0; i < ARRAY_LEN(level.numteamVotingClients); i++)
		level.numteamVotingClients[i] = 0;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected != CON_DISCONNECTED) {
			int team = level.clients[i].sess.sessionTeam;
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			// TODO: Use bot skill level instead of qtrue/qfalse?
			Q_strcat(botinfo, sizeof(botinfo), va("%d ", ((g_entities[i].r.svFlags & SVF_BOT) ? 1 : 0)));
			// TODO: Special "team" for connecting players?
			Q_strcat(teaminfo, sizeof(teaminfo), va("%d ", team));

			if (TEAM_SPECTATOR != team) {
				level.numNonSpectatorClients++;

				// decide if this should be auto-followed
				if (level.clients[i].pers.connected == CON_CONNECTED) {
					level.numPlayingClients++;
					if (!(g_entities[i].r.svFlags & SVF_BOT)) {
						level.numVotingClients++;
						if (TEAM_RED == team)
							level.numteamVotingClients[0]++;
						else if (TEAM_BLUE == team)
							level.numteamVotingClients[1]++;
					}
					if (level.follow1 == -1) {
						level.follow1 = i;
					} else if (level.follow2 == -1) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	if ('\0' == teaminfo[0]) {
		Q_strncpyz(teaminfo, PLAYERINFO_NONE, sizeof(teaminfo));
	}
	if ('\0' == botinfo[0]) {
		Q_strncpyz(botinfo, PLAYERINFO_NONE, sizeof(botinfo));
	}

	trap_Cvar_Set(PLAYERINFO_TEAM, teaminfo);
	trap_Cvar_Set(PLAYERINFO_BOT, botinfo);

	qsort(level.sortedClients, level.numConnectedClients, sizeof(level.sortedClients[0]), SortRanks);

	// set the rank value for all clients that are connected and not spectators
	if (g_gametype.integer >= GT_TEAM) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for (i = 0; i < level.numConnectedClients; i++) {
			cl = &level.clients[level.sortedClients[i]];
			if (level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if (level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {
		rank = -1;
		score = 0;
		for (i = 0; i < level.numPlayingClients; i++) {
			cl = &level.clients[level.sortedClients[i]];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ((GT_LPS == g_gametype.integer) && !(g_LPS_flags.integer & LPSF_PPOINTLIMIT)) {
				newScore = cl->sess.livesleft;
			}

			if (i == 0 || newScore != score) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[level.sortedClients[i - 1]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if (g_gametype.integer == GT_SINGLE_PLAYER && level.numPlayingClients == 1) {
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if (g_gametype.integer >= GT_TEAM) {
		trap_SetConfigstring(CS_SCORES1, va("%i", level.teamScores[TEAM_RED]));
		trap_SetConfigstring(CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE]));
	}
	// FIXME: LPS needs more special treatment, also in cgame (x points left etc.)
	else if ((g_gametype.integer == GT_LPS) && !(g_LPS_flags.integer & LPSF_PPOINTLIMIT)) {
		if (level.numConnectedClients == 0) {
			trap_SetConfigstring(CS_SCORES1, va("%i", SCORE_NOT_PRESENT));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		} else if (level.numConnectedClients == 1) {
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].sess.livesleft < 0
														  ? 0
														  : level.clients[level.sortedClients[0]].sess.livesleft));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		} else {
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].sess.livesleft < 0
														  ? 0
														  : level.clients[level.sortedClients[0]].sess.livesleft));
			trap_SetConfigstring(CS_SCORES2, va("%i", level.clients[level.sortedClients[1]].sess.livesleft < 0
														  ? 0
														  : level.clients[level.sortedClients[1]].sess.livesleft));
		}
	} else {
		if (level.numConnectedClients == 0) {
			trap_SetConfigstring(CS_SCORES1, va("%i", SCORE_NOT_PRESENT));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		} else if (level.numConnectedClients == 1) {
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE]));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		} else {
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE]));
			trap_SetConfigstring(CS_SCORES2, va("%i", level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE]));
		}
	}

	if (G_FreezeTag())
		FT_CalculateRoundScores();

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission, send the new info to everyone
	if (level.intermissiontime) {
		SendScoreboardMessageToAllClients();
	}

	// TODO: Test whether this is correct behaviour: _Always_ send scoreboard..
	// SendScoreboardMessageToAllClients();
}

/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients(void) {
	int i;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected == CON_CONNECTED) {
			DeathmatchScoreboardMessage(g_entities + i);
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission(gentity_t *ent) {
	// take out of follow mode if needed
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
		StopFollowing(ent);
	}

	// move to the spot
	VectorCopy(level.intermission_origin, ent->s.origin);
	VectorCopy(level.intermission_origin, ent->client->ps.origin);
	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset(ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups));

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint(void) {
	gentity_t *ent, *target;
	vec3_t dir;

	// find the intermission spot
	ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
	if (!ent) { // the map creator forgot to put in an intermission point...
		SelectSpawnPoint(vec3_origin, level.intermission_origin, level.intermission_angle, qfalse);
	} else {
		VectorCopy(ent->s.origin, level.intermission_origin);
		VectorCopy(ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if (ent->target) {
			target = G_PickTarget(ent->target);
			if (target) {
				VectorSubtract(target->s.origin, level.intermission_origin, dir);
				vectoangles(dir, level.intermission_angle);
			}
		}
	}
}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission(void) {
	int i;
	gentity_t *client;

	if (level.intermissiontime) {
		return; // already active
	}

	// if in tournement mode, change the wins / losses
	if (g_gametype.integer == GT_TOURNAMENT) {
		AdjustTournamentScores();
	}

	level.intermissiontime = level.time;
	FindIntermissionPoint();

	// if single player game
	if (g_gametype.integer == GT_SINGLE_PLAYER) {
		UpdateTournamentInfo();
		SpawnModelsOnVictoryPads();
	}

	// move all clients to the intermission point
	for (i = 0; i < level.maxclients; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0) {
			respawn(client);
		}
		MoveClientToIntermission(client);
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();
}

/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
void ExitLevel(void) {
	int i;
	gclient_t *cl;

	// bot interbreeding
	BotInterbreedEndMatch();

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if (g_gametype.integer == GT_TOURNAMENT) {
		if (!level.restarted) {
			RemoveTournamentLoser();
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			level.restarted = qtrue;
			level.changemap = NULL;
			level.intermissiontime = 0;
		}
		return;
	}

	if (G_FreezeTag()) {
		char mapname[256];
		char info[1024];

		trap_GetServerinfo(info, sizeof(info));
		Q_strncpyz(mapname, Info_ValueForKey(info, "mapname"), sizeof(mapname));

		// save rounds status
		trap_Cvar_Set("ftrounds", va("played:%i:redwins:%i:bluewins:%i:map:%s", level.ftNumRoundsPlayed,
									 level.ftNumRoundsWon[TEAM_RED], level.ftNumRoundsWon[TEAM_BLUE], mapname));
	}

	if (G_FreezeTag() && !level.allRoundsPlayed) {
		if (!level.restarted) {
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			level.restarted = qtrue;
			level.changemap = NULL;
			level.intermissiontime = 0;
		}
		return; // freezetag round ended
	}

	if (G_FreezeTag()) {
		// clear rounds status
		trap_Cvar_Set("ftrounds", va("played:%i:redwins:%i:bluewins:%i:map:%s", 0, 0, 0, ""));
	}

	trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before changing to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i = 0; i < g_maxclients.integer; i++) {
		if (level.clients[i].pers.connected == CON_CONNECTED) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}
}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf(const char *fmt, ...) {
	va_list argptr;
	char string[1024];
	int min, tens, sec;

	sec = (level.time - level.startTime) / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf(string, sizeof(string), "%3i:%i%i ", min, tens, sec);

	va_start(argptr, fmt);
	Q_vsnprintf(string + 7, sizeof(string) - 7, fmt, argptr);
	va_end(argptr);

	if (g_dedicated.integer) {
		G_Printf("%s", string + 7);
	}

	if (!level.logFile) {
		return;
	}

	trap_FS_Write(string, strlen(string), level.logFile);
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit(const char *string) {
	int i, numSorted;
	gclient_t *cl;
	G_LogPrintf("Exit: %s\n", string);

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring(CS_INTERMISSION, "1");

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if (numSorted > 32) {
		numSorted = 32;
	}

	if (g_gametype.integer >= GT_TEAM) {
		G_LogPrintf("Teamscores: red %i  blue %i\n", level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE]);
	}

	for (i = 0; i < numSorted; i++) {
		cl = &level.clients[level.sortedClients[i]];

		if (cl->sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}
		if (cl->pers.connected == CON_CONNECTING) {
			continue;
		}

		G_LogPrintf("Score: %i %i\n", level.sortedClients[i], cl->ps.persistant[PERS_SCORE]);
	}
}

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
static void CheckIntermissionExit(void) {
	int ready, notReady;
	int i;
	gclient_t *cl;
	int readyMask;

	if (g_gametype.integer == GT_SINGLE_PLAYER) {
		return;
	}

	// never stay more than 30 seconds in intermission
	if (level.time > level.intermissiontime + MAX_INTERMISSIONTIME) {
		ExitLevel();
		return;
	}

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) {
			continue;
		}

		if (cl->readyToExit) {
			ready++;
			if (i < 16) {
				readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if (level.time < level.intermissiontime + 5000) {
		return;
	}

	// if nobody wants to go, clear timer
	if (!ready) {
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if (!notReady) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if (!level.readyToExit) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if (level.time < level.exitTime + 10000) {
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
static qboolean ScoreIsTied(void) {
	int a, b;

	if (level.numPlayingClients < 2) {
		return qfalse;
	}

	// NOTE: Players can indeed be tied by livesleft, points and wins (from previous rounds). But if only two
	//       (or more in LPS) players are left who both are tied and do not score intentionally, the whole server
	//       is blocked. Others can not join or vote, since they are not allowed to by the game rules.
	// FIXME: This also applies to GT_TOURNAMENT(and maybe other situations where others are forced to spectate)!
	// TODO: Maybe introduce a general timelimit to sudden death, so the game does not last forever?
	//       One could also allow sudden death if there are only two players connected.
	if (g_gametype.integer == GT_LPS) {
		return qfalse;
	}

	if (g_gametype.integer >= GT_TEAM) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
static void CheckExitRules(void) {
	int i;
	gclient_t *cl;
	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if (level.intermissiontime) {
		CheckIntermissionExit();
		return;
	}

	if (level.intermissionQueued) {
		if (level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}

	if (level.warmupTime)
		return;

	// check for sudden death
	if (g_suddenDeath.integer && ScoreIsTied() && !G_FreezeTag()) {
		// always wait for sudden death
		return;
	}

	if (g_timelimit.integer < 0 || g_timelimit.integer > INT_MAX / 60000) {
		G_Printf("timelimit %i is out of range, defaulting to 0\n", g_timelimit.integer);
		trap_Cvar_Set("timelimit", "0");
		trap_Cvar_Update(&g_timelimit);
	}

	if (g_timelimit.integer && !level.warmupTime) {
		if (level.time - level.startTime >= g_timelimit.integer * 60000) {
			trap_SendServerCommand(-1, "print \"Timelimit hit.\n\"");
			LogExit("Timelimit hit.");
			return;
		}
	}

	/* Is this a feature? At least causes a bug in BigBalloon */
	/*
	if ( level.numPlayingClients < 2 ) {
		return;
	}
	*/

	if (g_fraglimit.integer < 0) {
		G_Printf("fraglimit %i is out of range, defaulting to 0\n", g_fraglimit.integer);
		trap_Cvar_Set("fraglimit", "0");
		trap_Cvar_Update(&g_fraglimit);
	}

	if ((g_gametype.integer == GT_LPS) && !(g_LPS_flags.integer & LPSF_PPOINTLIMIT)) {
		int playersWithLivesLeft = 0;
		gclient_t *tmpcl = NULL;

		for (i = 0; i < level.maxclients; i++) {
			cl = (level.clients + i);
			if (cl->pers.connected != CON_CONNECTED) {
				continue;
			}
			if (cl->sess.sessionTeam != TEAM_FREE) {
				continue;
			}

			if (cl->sess.livesleft > 0) {
				playersWithLivesLeft++;
				tmpcl = cl;
			}
		}

		// After warmup, i.e. map_restart, no clients are connected, which
		// causes above logic to yield 0 playersWithLivesLeft.
		if (playersWithLivesLeft <= 1 && (playersWithLivesLeft < level.numPlayingClients)) {
			if (tmpcl) {
				trap_SendServerCommand(
					-1, va("print \"%s" S_COLOR_WHITE " is the Last Standing Pad!\n\"", tmpcl->pers.netname));
			} else {
				// Should actually never happen, thus remove tmpcl?
				trap_SendServerCommand(-1, "print \"Last Pad Standing ends, no player has lives left.\n\"");
			}

			LogExit("Pointlimit hit.");
			return;
		}
	} else if (G_FreezeTag()) {
		int numRounds;
		int thisRoundWinner = 0;
		int allRoundsWinner = 0;

		numRounds = g_ft_numRounds.integer;

		// if the teams have players, check if anyone won yet ;)
		if (TeamCount(-1, TEAM_BLUE) >= 1 && TeamCount(-1, TEAM_RED) >= 1) {
			if (FT_WholeTeamIsFrozen(TEAM_RED)) {
				thisRoundWinner = TEAM_BLUE;
				level.ftNumRoundsWon[TEAM_BLUE]++;
			} else if (FT_WholeTeamIsFrozen(TEAM_BLUE)) {
				thisRoundWinner = TEAM_RED;
				level.ftNumRoundsWon[TEAM_RED]++;
			}
		}
		if (!thisRoundWinner)
			return; // no winner for the round yet

		level.ftNumRoundsPlayed++;

		if (g_ft_playAllRounds.integer) {
			// We will  play all rounds regardless if it's mathematically
			// impossible for one of the teams to still win.
			if (level.ftNumRoundsPlayed >= numRounds) {
				level.allRoundsPlayed = qtrue;

				// check who gets ZHE PRIZE!!11oneone
				allRoundsWinner =
					level.ftNumRoundsWon[TEAM_BLUE] > level.ftNumRoundsWon[TEAM_RED] ? TEAM_BLUE : TEAM_RED;
			}
		} else {
			// check if one team has won enough rounds so that the other can't
			// possible catch up within the maximum number of rounds
			int minWins;

			minWins = (numRounds / 2) + 1;

			if (level.ftNumRoundsWon[TEAM_RED] >= minWins) {
				level.allRoundsPlayed = qtrue;
				allRoundsWinner = TEAM_RED;
			} else if (level.ftNumRoundsWon[TEAM_BLUE] >= minWins) {
				level.allRoundsPlayed = qtrue;
				allRoundsWinner = TEAM_BLUE;
			}
		}

		if (level.allRoundsPlayed) {
			// All rounds are over, announce the overall winner and exit

			FT_CalculateRoundScores();

			if (allRoundsWinner == TEAM_BLUE)
				trap_SendServerCommand(-1, va("print \"^4Blue^7 wins the game. With %i Rounds of %i total.\n\"",
											  level.teamScores[TEAM_BLUE], numRounds));
			else
				trap_SendServerCommand(-1, va("print \"^1Red^7 wins the game. With %i Rounds of %i total.\n\"",
											  level.teamScores[TEAM_RED], numRounds));

			LogExit("AllRoundsPlayed");
			return;
		} else if (thisRoundWinner) {
			// This round ended announce the winner and exit

			FT_CalculateRoundScores();

			if (thisRoundWinner == TEAM_BLUE)
				trap_SendServerCommand(-1, va("print \"^4Blue^7 wins this round.\n\""));
			else
				trap_SendServerCommand(-1, va("print \"^1Red^7 wins this round.\n\""));

			LogExit("FreezeTag round ended.");
			return;
		}
	} else if (g_gametype.integer < GT_CTF && g_fraglimit.integer) {

		if (level.teamScores[TEAM_RED] >= g_fraglimit.integer) {
			trap_SendServerCommand(-1, "print \"Red hit the pointlimit.\n\"");
			LogExit("Pointlimit hit.");
			return;
		}

		if (level.teamScores[TEAM_BLUE] >= g_fraglimit.integer) {
			trap_SendServerCommand(-1, "print \"Blue hit the pointlimit.\n\"");
			LogExit("Pointlimit hit.");
			return;
		}

		for (i = 0; i < g_maxclients.integer; i++) {
			cl = level.clients + i;
			if (cl->pers.connected != CON_CONNECTED) {
				continue;
			}
			if (cl->sess.sessionTeam != TEAM_FREE) {
				continue;
			}

			if (cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer) {
				LogExit("Pointlimit hit.");
				trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " hit the pointlimit.\n\"", cl->pers.netname));
				return;
			}
		}
	}

	if (g_capturelimit.integer < 0) {
		G_Printf("capturelimit %i is out of range, defaulting to 0\n", g_capturelimit.integer);
		trap_Cvar_Set("capturelimit", "0");
		trap_Cvar_Update(&g_capturelimit);
	}

	if (g_gametype.integer >= GT_CTF && g_capturelimit.integer) {

		if (level.teamScores[TEAM_RED] >= g_capturelimit.integer) {
			trap_SendServerCommand(-1, "print \"Red hit the pointlimit.\n\"");
			LogExit("Pointlimit hit.");
			return;
		}

		if (level.teamScores[TEAM_BLUE] >= g_capturelimit.integer) {
			trap_SendServerCommand(-1, "print \"Blue hit the pointlimit.\n\"");
			LogExit("Pointlimit hit.");
			return;
		}
	}
}

/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

/*
=============
CheckTournament

Once a frame, check for changes in tournement player state
=============
*/
static void CheckTournament(void) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if (level.numPlayingClients == 0) {
		return;
	}

	if (g_gametype.integer == GT_TOURNAMENT) {

		// pull in a spectator if needed
		if (level.numPlayingClients < 2) {
			AddTournamentPlayer();
		}

		// if we don't have two players, go back to "waiting for players"
		if (level.numPlayingClients != 2) {
			if (level.warmupTime != -1) {
				level.warmupTime = -1;
				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
				G_LogPrintf("Warmup:\n");
			}
			return;
		}

		if (level.warmupTime == 0) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if (g_warmup.modificationCount != level.warmupModificationCount) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if (level.warmupTime < 0) {
			if (level.numPlayingClients == 2) {
				// fudge by -1 to account for extra delays
				level.warmupTime = level.time + (g_warmup.integer - 1) * 1000;
				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
			}
			return;
		}

		// if the warmup time has counted down, restart
		if (level.time > level.warmupTime) {
			level.warmupTime += 10000;
			trap_Cvar_Set("g_restarted", "1");
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			level.restarted = qtrue;
			return;
		}
	} else if (g_gametype.integer != GT_SINGLE_PLAYER && level.warmupTime != 0) {
		int counts[TEAM_NUM_TEAMS];
		qboolean notEnough = qfalse;

		if (g_gametype.integer >= GT_TEAM) {
			counts[TEAM_BLUE] = TeamCount(-1, TEAM_BLUE);
			counts[TEAM_RED] = TeamCount(-1, TEAM_RED);

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		} else if (level.numPlayingClients < 2) {
			notEnough = qtrue;
		}

		if (G_FreezeTag()) {
			counts[TEAM_BLUE] = TeamCount(-1, TEAM_BLUE);
			counts[TEAM_RED] = TeamCount(-1, TEAM_RED);

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		}

		if (notEnough) {
			if (level.warmupTime != -1) {
				level.warmupTime = -1;
				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
				G_LogPrintf("Warmup:\n");
			}
			return; // still waiting for team members
		}

		if (level.warmupTime == 0) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if (g_warmup.modificationCount != level.warmupModificationCount) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if (level.warmupTime < 0) {
			// fudge by -1 to account for extra delays
			level.warmupTime = level.time + (g_warmup.integer - 1) * 1000;
			trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
			return;
		}

		// if the warmup time has counted down, restart
		if (level.time > level.warmupTime) {
			level.warmupTime += 10000;
			trap_Cvar_Set("g_restarted", "1");
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			level.restarted = qtrue;
			return;
		}
	}
}

/*
==================
CheckVote
==================
*/
static void CheckVote(void) {
	if (level.voteExecuteTime && level.voteExecuteTime < level.time) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteString));
	}
	if (!level.voteTime) {
		return;
	}
	if (level.time - level.voteTime >= VOTE_TIME) {
		G_LogPrintf("Vote: failed timeout\n");
		trap_SendServerCommand(-1, "print \"Vote failed.\n\"");
	} else {
		// ATVI Q3 1.32 Patch #9, WNF
		if (level.voteYes > level.numVotingClients / 2) {
			// execute the command, then remove the vote
			G_LogPrintf("Vote: passed\n");
			trap_SendServerCommand(-1, "print \"Vote passed.\n\"");
			level.voteExecuteTime = level.time + 3000;
		} else if (level.voteNo >= level.numVotingClients / 2) {
			// same behavior as a timeout
			G_LogPrintf("Vote: failed\n");
			trap_SendServerCommand(-1, "print \"Vote failed.\n\"");
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring(CS_VOTE_TIME, "");
}

/*
==================
PrintTeam
==================
*/
static void PrintTeam(int team, char *message) {
	int i;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap_SendServerCommand(i, message);
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(int team, int client) {
	int i;

	if (level.clients[client].pers.connected == CON_DISCONNECTED) {
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname));
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname));
		return;
	}
	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged(client);
	// This is not used in WoP (yet). Left code intact, but don't spam the world
	// PrintTeam(team, va( "print \"%s" S_COLOR_WHITE " is the new team leader\n\"", level.clients[client].pers.netname
	// ) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader(int team) {
	int i;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients) {
		for (i = 0; i < level.maxclients; i++) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}

		if (i >= level.maxclients) {
			for (i = 0; i < level.maxclients; i++) {
				if (level.clients[i].sess.sessionTeam != team)
					continue;
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
static void CheckTeamVote(int team) {
	int cs_offset;

	if (team == TEAM_RED)
		cs_offset = 0;
	else if (team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (!level.teamVoteTime[cs_offset]) {
		return;
	}
	if (level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME) {
		trap_SendServerCommand(-1, "print \"Team vote failed.\n\"");
	} else {
		if (level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset] / 2) {
			// execute the command, then remove the vote
			trap_SendServerCommand(-1, "print \"Team vote passed.\n\"");
			//
			if (!Q_strncmp("leader", level.teamVoteString[cs_offset], 6)) {
				// set the team leader
				SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
			} else {
				trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset]));
			}
		} else if (level.teamVoteNo[cs_offset] >= level.numteamVotingClients[cs_offset] / 2) {
			// same behavior as a timeout
			trap_SendServerCommand(-1, "print \"Team vote failed.\n\"");
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.teamVoteTime[cs_offset] = 0;
	trap_SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, "");
}

/*
==================
CheckCvars
==================
*/
static void CheckCvars(void) {
	static int lastMod = -1;

	if (g_password.modificationCount != lastMod) {
		lastMod = g_password.modificationCount;
		if (*g_password.string && Q_stricmp(g_password.string, "none")) {
			trap_Cvar_Set("g_needpass", "1");
		} else {
			trap_Cvar_Set("g_needpass", "0");
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink(gentity_t *ent) {
	float thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}
	if (thinktime > level.time) {
		return;
	}

	ent->nextthink = 0;
	if (!ent->think) {
		G_Error("NULL ent->think");
	}
	ent->think(ent);
}

/*
#######################

  WoP_RunFrame

some things which we check each frame ;)
#######################
*/

// FIXME: Most of this should NOT be done every frame, rather once each second per client
//        or even in a special function. Entire code is horrible

static void WoP_RunFrame(void) {
	gentity_t *playersWithLives[2]; // for LPS
	float maxReady, curReady;
	int i;

	playersWithLives[0] = playersWithLives[1] = NULL;
	maxReady = curReady = 0.0f;
	for (i = 0; i < level.maxclients; ++i) {
		gentity_t *ent;
		int tmptime;

		ent = g_entities + i;

		if (!ent->inuse)
			continue; // ich glaub das hat keinen sinn ... ich vermutte das alle möglichen client-ents auf inuse
					  // geschaltet sind ...

		if (level.clients[i].pers.connected != CON_CONNECTED) {
			ent->r.svFlags &= ~SVF_BROADCAST;
			continue;
		}

		if (!ent->client) {
			continue;
		}

		ent->client->ps.stats[STAT_HB_EFLAGS] = ent->s.time2 =
			(ent->s.eFlags >> 16); // missbrauch !!! ... it's a hack to bring the higher bytes from server to client (it
								   // seems the engine just sends 16bit)

		// broadcast boasteruser (why?)
		if (level.clients[i].ps.weapon == WP_BOASTER)
			ent->r.svFlags |= SVF_BROADCAST;
		else
			ent->r.svFlags &= ~SVF_BROADCAST;

		if ((level.warmupTime > 0) && (g_warmupReady.value > 0.0f) &&
			(TEAM_SPECTATOR != level.clients[i].sess.sessionTeam) && !(ent->r.svFlags & SVF_BOT)) {
			maxReady++;
			if (level.clients[i].readyToFight) {
				curReady++;
			}
		}

		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {

			// Speedy Knockback (throwing other players away that are in the way)
			if (ent->client->ps.powerups[PW_SPEEDY] && !InSprayroom(ent->client)) {
				trace_t tr;
				vec3_t endOrigin;

				VectorMA(ent->r.currentOrigin, 0.05, ent->client->ps.velocity, endOrigin);

				trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, endOrigin, i, MASK_SHOT);

				// speedy carier hit another entity
				if (tr.fraction < 1.0) {
					gentity_t *hitEnt = (g_entities + tr.entityNum);

					if (hitEnt->client) {
						vec3_t tmp;

						VectorSubtract(hitEnt->r.currentOrigin, ent->r.currentOrigin, tmp);
						VectorNormalize(tmp);

						if (tmp[2] < 0.0) {
							tmp[2] *= -1.0;
						} // up only, inverse

						// FIXME: Local gravity
						VectorMA(hitEnt->client->ps.velocity, 800.0, tmp, hitEnt->client->ps.velocity);
						hitEnt->client->ps.velocity[2] += 100.0;
					}
				}
			}

			// Boaster-DamageOverTime
			if (ent->client->lastBoasterHitAttacker) {
				if (((level.time - ent->client->lastBoasterHitTime) > BOASTER_DOT_MAXTIME) || (ent->health <= 0)) {
					ent->client->lastBoasterHitAttacker = NULL;
				} else if (level.clients[i].nextBoasterDoTTick < level.time) {
					G_Damage(ent, ent->client->lastBoasterHitAttacker, ent->client->lastBoasterHitAttacker, NULL, NULL,
							 DAMAGE_DOT_BOASTER, 0, MOD_BOASTER);
					ent->client->nextBoasterDoTTick += ADDTIME_DOT_BOASTER;
				}
			}

			// collecting infos of players with livesleft ... used at the end oft WoP_RunFrame
			if (g_gametype.integer == GT_LPS) {
				if (!level.warmupTime && (ent->client->sess.livesleft > 0)) {
					if (!playersWithLives[0]) {
						playersWithLives[0] = ent;
					} else {
						playersWithLives[1] = ent;
					}

					// FIXME: Whut?
					if (ent->health > 0) {
						ent->r.svFlags |= SVF_BROADCAST;
					}
				}

				level.clients[i].ps.stats[STAT_LIVESLEFT] = level.clients[i].sess.livesleft;

				// Right inside that ugliness, another brutal hotfix!
				// In normal LPS without LPSF_PPOINTLIMIT, PERS_SCORE is never set while it should equal livesleft
				if (!(g_LPS_flags.integer & LPSF_PPOINTLIMIT)) {
					ent->client->ps.persistant[PERS_SCORE] = ent->client->sess.livesleft;
				}

				if (ent->client->sess.livesleft > 0) {
					ent->s.eFlags &= ~EF_NOLIFESLEFT;
				} else {
					ent->s.eFlags |= EF_NOLIFESLEFT;
				}
			}

			// reset slick-touch-flag
			if ((ent->client->last_slickent_touch + 50) < level.time) {
				ent->client->ps.pm_flags &= ~PMF_TOUCHSLICKENT;
			}

			// check if there is a spraylogo selected
			// FIXME: Remove. No logo should default to some safe default logo (in game&cgame!)
			if (IsSyc()) {
				if ((ent->client->sess.selectedlogo[0] == '\0') &&
					(!ent->client->logoasktime || ((ent->client->logoasktime + 1000) < level.time))) {
					trap_SendServerCommand(i, "cdi " XSTRING(CLIENT_DO_IT_SELECT_LOGO));
					ent->client->logoasktime = level.time;
				}
			}

			// check if the player was to long inside the sprayroom
			if (InSprayroom(ent->client)) {
				tmptime = (ent->client->sprayroomleavetime - level.time);
				ent->client->ps.stats[STAT_SPRAYROOMSECS] = (1 + (int)(tmptime / 1000));

				if (tmptime <= 0) {
					if (level.sr_tl_tele) {
						VectorCopy(level.sr_tl_tele->s.origin, ent->client->ps.origin);
						ent->client->ps.origin[2] += 9;

						SetClientViewAngle(ent, level.sr_tl_tele->s.angles);

						trap_SendServerCommand(i, va("srwc %i", ent->client->last_nonspray_weapon));
					}
					// No killroom in map. Kill player right in place
					else {
						if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
							continue;
						}
						if (ent->health <= 0) {
							continue;
						}

						ent->flags &= ~FL_GODMODE;
						ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
						player_die(ent, ent, ent, 100000, MOD_SUICIDE);
					}

					trap_SendServerCommand(i, "cdi " XSTRING(CLIENT_DO_IT_SPRAYROOM_GOODBYE));
					ent->client->ps.stats[STAT_SPRAYROOMSECS] = 0;
				}
			}

		} // <-- end of if !Spectator
	}	  // <-- end of foreach client

	// FIXME: This _really_ needs to be done in CheckExitRules()!!1!
	if ((g_gametype.integer == GT_LPS) && (g_LPS_flags.integer & LPSF_PPOINTLIMIT) && !level.warmupTime &&
		(playersWithLives[1] == NULL)) {
		static int roundendtime = 0;
		int cNum;
		int nextpoints;
		qboolean player0done = qfalse;
		int roundSorted[MAX_CLIENTS];

		if (playersWithLives[0] != NULL) {
			// NOTE: We need another list of sorted clients, which is sorted by livesleft
			//       (in fact by lastDeathTime) instead of points.
			memcpy(roundSorted, level.sortedClients, sizeof(roundSorted));
			qsort(roundSorted, level.numConnectedClients, sizeof(roundSorted[0]), SortRanksLPS);

			if (g_LPS_flags.integer & LPSF_MULTIPOINTS) {
				nextpoints = g_LPS_startlives.integer;
			} else {
				nextpoints = 1;
			}

			for (i = 0; i < level.maxclients; i++) {
				cNum = roundSorted[i];
				if (cNum == 0) {
					if (!player0done) {
						player0done = qtrue;
					} else {
						break;
					}
				}

				if (cNum == playersWithLives[0]->client->ps.clientNum) {
					trap_SendServerCommand(cNum, "cp \"You have won this Round.\n\"");
				} else {
					trap_SendServerCommand(cNum, va("cp \"%s" S_COLOR_WHITE " has won this Round.\n\"",
													playersWithLives[0]->client->pers.netname));
				}

				if (level.clients[cNum].sess.sessionTeam == TEAM_SPECTATOR) {
					continue;
				}

				if (nextpoints > 0) {
					AddScore((g_entities + cNum), (g_entities + cNum)->s.origin, nextpoints, SCORE_SURVIVE_S);
					nextpoints--;
				}
			}

			playersWithLives[0]->client->sess.livesleft = 0;
			roundendtime = level.time;
		}
		// FIXME: Pseudo restart of match. Should this use map_restart instead? See tournament
		//        Also define constants!
		else if ((level.time - roundendtime) > 10000) {
			for (i = 0; i < level.maxclients; i++) {
				if (level.clients[i].pers.connected != CON_CONNECTED) {
					continue;
				}
				if (level.clients[i].sess.sessionTeam != TEAM_FREE) {
					continue;
				}

				level.clients[i].sess.livesleft = g_LPS_startlives.integer;
				ClientSpawn((g_entities + i));
			}
		} else if ((level.time - roundendtime) > 5000 && !(level.time % 500)) {
			trap_SendServerCommand(
				-1, va("cp \"Next Round starts in %i seconds.\n\"", (10000 - (level.time - roundendtime)) / 1000));
		}
	}

	if (level.warmupTime && maxReady) {
		if ((curReady / maxReady) < g_warmupReady.value) {
			// We've got enough ready players, stop warmup
			level.warmupTime = -1;
		}

		trap_Cvar_Set("g_curWarmupReady", va("%f", (curReady / maxReady)));
	}
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
static void G_RunFrame(int levelTime) {
	int i;
	gentity_t *ent;

	// if we are waiting for the level to restart, do nothing
	if (level.restarted) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	// get any cvar changes
	G_UpdateCvars();

	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for (i = 0; i < level.num_entities; i++, ent++) {
		if (!ent->inuse) {
			continue;
		}

		// clear events that are too old
		if (level.time - ent->eventTime > EVENT_VALID_MSEC) {
			if (ent->s.event) {
				ent->s.event = 0; // &= EV_EVENT_BITS;
				if (ent->client) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					// ent->client->ps.events[0] = 0;
					// ent->client->ps.events[1] = 0;
				}
			}
			if (ent->freeAfterEvent) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity(ent);
				continue;
			} else if (ent->unlinkAfterEvent) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity(ent);
			}
		}

		// temporary entities don't think
		if (ent->freeAfterEvent) {
			continue;
		}

		if (!ent->r.linked && ent->neverFree) {
			continue;
		}

		if (ent->s.eType == ET_MISSILE) {
			G_RunMissile(ent);
			continue;
		}

		if (ent->s.eType == ET_EXPLOSION) {
			G_RunExplosion(ent);
			continue;
		}

		if (ent->s.eType == ET_ITEM || ent->physicsObject) {
			G_RunItem(ent);
			continue;
		}

		if (ent->s.eType == ET_MOVER) {
			G_RunMover(ent);
			continue;
		}

		if (i < MAX_CLIENTS) {
			G_RunClient(ent);
			continue;
		}

		G_RunThink(ent);
	}

	// perform final fixups on the players
	ent = &g_entities[0];
	for (i = 0; i < level.maxclients; i++, ent++) {
		if (ent->inuse) {
			ClientEndFrame(ent);
		}
	}

	// see if it is time to do a tournement restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote(TEAM_RED);
	CheckTeamVote(TEAM_BLUE);

	// for tracking changes
	CheckCvars();

	if (g_listEntity.integer) {
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity", "0");
	}

	if (!level.intermissiontime)
		WoP_RunFrame();
}
