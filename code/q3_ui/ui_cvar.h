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

#ifdef UI_CVAR_TABLE
#define UI_CVAR( var, name, defaultVal, flags ) \
	{ &var, name, defaultVal, flags },
#endif

#ifdef UI_CVAR_DECLARATION
#define UI_CVAR( var, name, defaultVal, flags ) \
	vmCvar_t	var;
#endif

#ifdef UI_CVAR_EXTERN
#define UI_CVAR( var, name, defaultVal, flags ) \
	extern	vmCvar_t	var;
#endif

UI_CVAR( ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE )
UI_CVAR( ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE )

UI_CVAR( ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE )
UI_CVAR( ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE )

UI_CVAR( ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE )
UI_CVAR( ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE )
UI_CVAR( ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE )

UI_CVAR( ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE )
UI_CVAR( ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE )
UI_CVAR( ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE )

UI_CVAR( ui_arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM )
UI_CVAR( ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM )
UI_CVAR( ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE )
UI_CVAR( ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE )
UI_CVAR( ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE )
UI_CVAR( ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE )
UI_CVAR( ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE )
UI_CVAR( ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE )
UI_CVAR( ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE )
UI_CVAR( ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH )

UI_CVAR( ui_spSelection, "ui_spSelection", "", CVAR_ROM )

UI_CVAR( ui_browserMaster, "ui_browserMaster", "1", CVAR_ARCHIVE )
UI_CVAR( ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE )
UI_CVAR( ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE )
UI_CVAR( ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE )
UI_CVAR( ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE )

UI_CVAR( ui_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE )
UI_CVAR( ui_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE )
UI_CVAR( ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE )
UI_CVAR( ui_marks, "cg_marks", "1", CVAR_ARCHIVE )

UI_CVAR( ui_server1, "server1", "", CVAR_ARCHIVE )
UI_CVAR( ui_server2, "server2", "", CVAR_ARCHIVE )
UI_CVAR( ui_server3, "server3", "", CVAR_ARCHIVE )
UI_CVAR( ui_server4, "server4", "", CVAR_ARCHIVE )
UI_CVAR( ui_server5, "server5", "", CVAR_ARCHIVE )
UI_CVAR( ui_server6, "server6", "", CVAR_ARCHIVE )
UI_CVAR( ui_server7, "server7", "", CVAR_ARCHIVE )
UI_CVAR( ui_server8, "server8", "", CVAR_ARCHIVE )
UI_CVAR( ui_server9, "server9", "", CVAR_ARCHIVE )
UI_CVAR( ui_server10, "server10", "", CVAR_ARCHIVE )
UI_CVAR( ui_server11, "server11", "", CVAR_ARCHIVE )
UI_CVAR( ui_server12, "server12", "", CVAR_ARCHIVE )
UI_CVAR( ui_server13, "server13", "", CVAR_ARCHIVE )
UI_CVAR( ui_server14, "server14", "", CVAR_ARCHIVE )
UI_CVAR( ui_server15, "server15", "", CVAR_ARCHIVE )
UI_CVAR( ui_server16, "server16", "", CVAR_ARCHIVE )

UI_CVAR( ui_cdkeychecked, "ui_cdkeychecked", "0", CVAR_ROM )
UI_CVAR( ui_ioq3, "ui_ioq3", "1", CVAR_ROM )

#undef UI_CVAR
