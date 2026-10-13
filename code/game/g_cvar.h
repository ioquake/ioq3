/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2023 LegendaryGuard
Copyright (C) 2026 WofWca

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

#ifdef G_CVAR_TABLE
#define G_CVAR( var, name, defaultVal, flags, trackChange ) \
	{ &var, name, defaultVal, flags, 0, trackChange, qfalse },
#define G_CVAR_EXT( var, name, defaultVal, flags, modificationCount, trackChange, teamShader ) \
	{ &var, name, defaultVal, flags, modificationCount, trackChange, teamShader },
#endif

#ifdef G_CVAR_DECLARATION
#define G_CVAR( var, name, defaultVal, flags, trackChange ) \
	vmCvar_t	var;
#define G_CVAR_EXT( var, name, defaultVal, flags, modificationCount, trackChange, teamShader ) \
	vmCvar_t	var;
#endif

#ifdef G_CVAR_EXTERN
#define G_CVAR( var, name, defaultVal, flags, trackChange ) \
	extern	vmCvar_t	var;
#define G_CVAR_EXT( var, name, defaultVal, flags, modificationCount, trackChange, teamShader ) \
	extern	vmCvar_t	var;
#endif

#define NO_FLAGS 0
// trackChange
#define TRACK qtrue
// Don't trackChange
#define NO_TRACK qfalse

// don't override the cheat state set by the system
G_CVAR( g_cheats, "sv_cheats", "", NO_FLAGS, NO_TRACK )

G_CVAR( g_restarted, "g_restarted", "0", CVAR_ROM, NO_TRACK )

// latched vars
G_CVAR( g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, NO_TRACK )

// allow this many total, including spectators
G_CVAR( g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, NO_TRACK )
// allow this many active
G_CVAR( g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, NO_TRACK )

// change anytime vars
G_CVAR( g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, TRACK )
G_CVAR( g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, TRACK )
G_CVAR( g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, TRACK )
G_CVAR( g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, TRACK )

G_CVAR( g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, NO_TRACK )

G_CVAR( g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, TRACK )

G_CVAR( g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE, NO_TRACK )
G_CVAR( g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE, NO_TRACK )

G_CVAR( g_warmup, "g_warmup", "20", CVAR_ARCHIVE, TRACK )
G_CVAR( g_doWarmup, "g_doWarmup", "0", CVAR_ARCHIVE, TRACK )
G_CVAR( g_logfile, "g_log", "games.log", CVAR_ARCHIVE, NO_TRACK )
G_CVAR( g_logfileSync, "g_logsync", "0", CVAR_ARCHIVE, NO_TRACK )

G_CVAR( g_password, "g_password", "", CVAR_USERINFO, NO_TRACK )

G_CVAR( g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, NO_TRACK )
G_CVAR( g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, NO_TRACK )

G_CVAR( g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, NO_TRACK )

G_CVAR( g_dedicated, "dedicated", "0", NO_FLAGS, NO_TRACK )

G_CVAR( g_speed, "g_speed", "320", NO_FLAGS, TRACK )
G_CVAR( g_gravity, "g_gravity", "800", NO_FLAGS, TRACK )
G_CVAR( g_knockback, "g_knockback", "1000", NO_FLAGS, TRACK )
G_CVAR( g_quadfactor, "g_quadfactor", "3", NO_FLAGS, TRACK )
G_CVAR( g_weaponRespawn, "g_weaponrespawn", "5", NO_FLAGS, TRACK )
G_CVAR( g_weaponTeamRespawn, "g_weaponTeamRespawn", "30", NO_FLAGS, TRACK )
G_CVAR( g_forcerespawn, "g_forcerespawn", "20", NO_FLAGS, TRACK )
G_CVAR( g_inactivity, "g_inactivity", "0", NO_FLAGS, TRACK )
G_CVAR( g_debugMove, "g_debugMove", "0", NO_FLAGS, NO_TRACK )
G_CVAR( g_debugDamage, "g_debugDamage", "0", NO_FLAGS, NO_TRACK )
G_CVAR( g_debugAlloc, "g_debugAlloc", "0", NO_FLAGS, NO_TRACK )
G_CVAR( g_motd, "g_motd", "", NO_FLAGS, NO_TRACK )
G_CVAR( g_blood, "com_blood", "1", NO_FLAGS, NO_TRACK )

G_CVAR( g_podiumDist, "g_podiumDist", "80", NO_FLAGS, NO_TRACK )
G_CVAR( g_podiumDrop, "g_podiumDrop", "70", NO_FLAGS, NO_TRACK )

G_CVAR( g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, NO_TRACK )
G_CVAR( g_listEntity, "g_listEntity", "0", NO_FLAGS, NO_TRACK )

#ifdef MISSIONPACK
G_CVAR( g_obeliskHealth, "g_obeliskHealth", "2500", NO_FLAGS, NO_TRACK )
G_CVAR( g_obeliskRegenPeriod, "g_obeliskRegenPeriod", "1", NO_FLAGS, NO_TRACK )
G_CVAR( g_obeliskRegenAmount, "g_obeliskRegenAmount", "15", NO_FLAGS, NO_TRACK )
G_CVAR( g_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO, NO_TRACK )

G_CVAR( g_cubeTimeout, "g_cubeTimeout", "30", NO_FLAGS, NO_TRACK )
G_CVAR_EXT( g_redteam, "g_redteam", "Stroggs", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, TRACK, qtrue )
G_CVAR_EXT( g_blueteam, "g_blueteam", "Pagans", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, TRACK, qtrue )
G_CVAR( g_singlePlayer, "ui_singlePlayerActive", "", NO_FLAGS, NO_TRACK )

G_CVAR( g_enableDust, "g_enableDust", "0", CVAR_SERVERINFO, TRACK )
G_CVAR( g_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO, TRACK )
G_CVAR( g_proxMineTimeout, "g_proxMineTimeout", "20000", NO_FLAGS, NO_TRACK )
#endif
G_CVAR( g_smoothClients, "g_smoothClients", "1", NO_FLAGS, NO_TRACK )
G_CVAR( pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, NO_TRACK )
G_CVAR( pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, NO_TRACK )

G_CVAR( g_rankings, "g_rankings", "0", NO_FLAGS, NO_TRACK )
G_CVAR( g_localTeamPref, "g_localTeamPref", "", NO_FLAGS, NO_TRACK )

#undef G_CVAR
#undef G_CVAR_EXT
#undef NO_FLAGS
#undef TRACK
#undef NO_TRACK
