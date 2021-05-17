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
#include "match.h"
#include "inv.h"

extern vmCvar_t bot_developer;

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage(const gentity_t *ent) {
	char entry[1024];
	char string[1000];
	int stringlength;
	int i, j;
	gclient_t *cl;
	int numSorted, scoreFlags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;

	for (i = 0; i < numSorted; i++) {
		int ping;

		cl = &level.clients[level.sortedClients[i]];

		if (cl->pers.connected == CON_CONNECTING) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if (cl->accuracy_shots) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		} else {
			accuracy = 0;
		}
		perfect = (cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0) ? 1 : 0;

		Com_sprintf(entry, sizeof(entry), " %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
					cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime) / 60000, scoreFlags,
					g_entities[level.sortedClients[i]].s.powerups, accuracy, cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
					cl->ps.persistant[PERS_EXCELLENT_COUNT], cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
					cl->ps.persistant[PERS_DEFEND_COUNT], cl->ps.persistant[PERS_ASSIST_COUNT], perfect,
					cl->ps.persistant[PERS_CAPTURES], (cl->ps.persistant[PERS_SPRAYAWARDS_COUNT] >> 8),
					cl->ps.persistant[PERS_SPRAYAWARDS_COUNT] & 0xFF,
					(cl->sess.livesleft < 0 ? 0 : cl->sess.livesleft));
		j = strlen(entry);
		if (stringlength + j >= sizeof(string))
			break;
		strcpy(string + stringlength, entry);
		stringlength += j;
	}

	trap_SendServerCommand(ent - g_entities,
						   va("scores %i %i %i%s", i, level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], string));
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f(gentity_t *ent) {
	DeathmatchScoreboardMessage(ent);
}

/*
==================
CheatsOk
==================
*/
static qboolean CheatsOk(const gentity_t *ent) {
	if (!g_cheats.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return qfalse;
	}
	if (ent->health <= 0) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be alive to use this command.\n\"");
		return qfalse;
	}
	return qtrue;
}

/*
==================
ConcatArgs
==================
*/
const char *ConcatArgs(int start) {
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for (i = start; i < c; i++) {
		trap_Argv(i, arg, sizeof(arg));
		tlen = strlen(arg);
		if (len + tlen >= MAX_STRING_CHARS - 1) {
			break;
		}
		memcpy(line + len, arg, tlen);
		len += tlen;
		if (i != c - 1) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
static void SanitizeString(const char *in, char *out) {
	while (*in) {
		if (*in == 27) {
			in += 2; // skip color code
			continue;
		}
		if (*in < ' ') {
			in++;
			continue;
		}
		*out++ = tolower(*in++);
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
static int ClientNumberFromString(const gentity_t *to, const char *s) {
	gclient_t *cl;
	int idnum;
	char s2[MAX_STRING_CHARS];
	char n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi(s);
		if (idnum < 0 || idnum >= level.maxclients) {
			trap_SendServerCommand(to - g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString(s, s2);
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++) {
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		SanitizeString(cl->pers.netname, n2);
		if (!strcmp(n2, s2)) {
			return idnum;
		}
	}

	trap_SendServerCommand(to - g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
static void Cmd_Give_f(gentity_t *ent) {
	const char *name;
	gitem_t *it;
	int i;
	qboolean give_all;
	gentity_t *it_ent;
	trace_t trace;

	if (!CheatsOk(ent)) {
		return;
	}

	name = ConcatArgs(1);

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_stricmp(name, "health") == 0) {
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_NUM_WEAPONS) - 1 - (1 << WP_GRAPPLING_HOOK) - (1 << WP_NONE) -
											   (1 << WP_KILLERDUCKS) - (1 << WP_SPRAYPISTOL) - (1 << WP_BAMBAM_MISSILE);
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0) {
		for (i = 0; i < MAX_WEAPONS; i++) {
			if (i == WP_SPRAYPISTOL)
				continue;
			if (i == WP_KILLERDUCKS)
				continue;
			if (i == WP_BAMBAM_MISSILE)
				continue;
			ent->client->ps.ammo[i] = 999;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0) {
		ent->client->ps.stats[STAT_ARMOR] = 200;

		if (!give_all)
			return;
	}

	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "spraygod") == 0) {
		ent->client->ps.persistant[PERS_SPRAYAWARDS_COUNT] += 0x100;
		return;
	}
	if (Q_stricmp(name, "spraykiller") == 0) {
		ent->client->ps.persistant[PERS_SPRAYAWARDS_COUNT] =
			(ent->client->ps.persistant[PERS_SPRAYAWARDS_COUNT] & 0xFF00) |
			((ent->client->ps.persistant[PERS_SPRAYAWARDS_COUNT] + 1) & 0xFF);
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		BerserkerCheck(ent);
		return;
	}
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	if (Q_stricmp(name, "bambam") == 0) {
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = MODELINDEX_BAMBAM;
		ent->client->ps.stats[STAT_HOLDABLEVAR] = 5;
		return;
	}

	if (Q_stricmp(name, "boomie") == 0) {
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = MODELINDEX_BOOMIE;
		ent->client->ps.stats[STAT_HOLDABLEVAR] = 5;
		return;
	}

	// spawn a specific item right on the player
	if (!give_all) {
		it = BG_FindItem(name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy(ent->r.currentOrigin, it_ent->s.origin);
		it_ent->classname = it->classname;
		G_SpawnItem(it_ent, it);
		FinishSpawningItem(it_ent);
		memset(&trace, 0, sizeof(trace));
		Touch_Item(it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity(it_ent);
		}
	}
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f(gentity_t *ent) {
	char *msg;

	if (!CheatsOk(ent)) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE))
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f(gentity_t *ent) {
	char *msg;

	if (!CheatsOk(ent)) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f(gentity_t *ent) {
	char *msg;

	if (!CheatsOk(ent)) {
		return;
	}

	if (ent->client->noclip) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

void Cmd_Thaw_f(gentity_t *ent) {
	if (!CheatsOk(ent)) {
		return;
	}

	ent->client->freezeTime = level.time - 6000;
	FT_ProgressThawing(ent, ent);
}

/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f(gentity_t *ent) {
	if (!ent->client->pers.localClient) {
		trap_SendServerCommand(ent - g_entities,
							   "print \"The levelshot command must be executed by a local client\n\"");
		return;
	}

	if (!CheatsOk(ent))
		return;

	// doesn't work in single player
	if (g_gametype.integer == GT_SINGLE_PLAYER) {
		trap_SendServerCommand(ent - g_entities, "print \"Must not be in singleplayer mode for levelshot\n\"");
		return;
	}

	BeginIntermission();
	trap_SendServerCommand(ent - g_entities, "clientLevelShot");
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f(gentity_t *ent) {
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		return;
	}
	if (ent->health <= 0) {
		return;
	}

	if (G_FreezeTag()) {
		FT_FreezePlayer(ent, ent);
		return;
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	if (ent->client->lastSentFlying > -1) {
		// If player is in the air because of knockback, we give credit to the person who sent him flying
		player_die(ent, ent, &g_entities[ent->client->lastSentFlying], 100000, MOD_SUICIDE);
	} else {
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);
	}
}

/*
=================
BroadcastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange(gclient_t *client, int oldTeam) {
	if (client->sess.sessionTeam == TEAM_RED) {
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the red team.\n\"", client->pers.netname));
	} else if (client->sess.sessionTeam == TEAM_BLUE) {
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the blue team.\n\"", client->pers.netname));
	} else if (client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR) {
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the spectators.\n\"", client->pers.netname));
	} else if (client->sess.sessionTeam == TEAM_FREE) {
		trap_SendServerCommand(-1, va("cp \"%s" S_COLOR_WHITE " joined the battle.\n\"", client->pers.netname));
	}
}

/*
=================
SetTeam
=================
*/
void SetTeam(gentity_t *ent, const char *s) {
	int team, oldTeam;
	gclient_t *client;
	int clientNum;
	spectatorState_t specState;
	int specClient;
	int teamLeader;
	qboolean lpsNoLives = qfalse;

	//
	// see what change is requested
	//
	client = ent->client;

	// DeadSpec() does not really apply here, since it's meant to be universal, while this assignment is not
	if ((g_gametype.integer == GT_LPS) && (client->sess.livesleft <= 0)) {
		lpsNoLives = qtrue;
	}

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if (!Q_stricmp(s, "scoreboard") || !Q_stricmp(s, "score")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if (!Q_stricmp(s, "follow1")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if (!Q_stricmp(s, "follow2")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if (!Q_stricmp(s, "spectator") || !Q_stricmp(s, "s")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if (g_gametype.integer >= GT_TEAM) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if (!Q_stricmp(s, "red") || !Q_stricmp(s, "r")) {
			team = TEAM_RED;
		} else if (!Q_stricmp(s, "blue") || !Q_stricmp(s, "b")) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam(clientNum);
		}

		// freezetag
		if (G_FreezeTag()) {
			if (!FT_CanSwitchTeam(ent, team))
				return;

			if (ent->client->sess.sessionTeam != team)
				FT_ThawPlayer(ent, NULL);
		}

		if (g_teamForceBalance.integer && !client->pers.localClient && !(ent->r.svFlags & SVF_BOT)) {
			int counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount(clientNum, TEAM_BLUE);
			counts[TEAM_RED] = TeamCount(clientNum, TEAM_RED);

			// We allow a spread of two
			if (team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1) {
				trap_SendServerCommand(clientNum, "cp \"Red team has too many players.\n\"");
				return; // ignore the request
			}
			if (team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1) {
				trap_SendServerCommand(clientNum, "cp \"Blue team has too many players.\n\"");
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	// override decision if limiting the players
	if ((g_gametype.integer == GT_TOURNAMENT) && level.numNonSpectatorClients >= 2) {
		team = TEAM_SPECTATOR;
	} else if (g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer) {
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if (team == oldTeam && team != TEAM_SPECTATOR) {
		return;
	}

	//
	// execute the team change
	//

	// if the player was dead leave the body
	if (client->ps.stats[STAT_HEALTH] <= 0 && !lpsNoLives && client->pers.connected == CON_CONNECTED) {
		CopyToBodyQue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if (oldTeam != TEAM_SPECTATOR && !lpsNoLives) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);
		RemoveOwnedItems(ent);
	}

	// they go to the end of the line for tournements
	if (team == TEAM_SPECTATOR && oldTeam != team)
		AddTournamentQueue(client);

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	if (g_gametype.integer == GT_LPS) {
		if (!level.warmupTime) {
			client->sess.livesleft = -1;
			ent->health = client->ps.stats[STAT_HEALTH] = 0;
		} else
			client->sess.livesleft = g_LPS_startlives.integer;
	}

	// noch mal aendern falls MG nicht standard waffe is
	if (client->ps.weapon == WP_SPRAYPISTOL)
		trap_SendServerCommand(client->ps.clientNum, va("srwc %i", WP_NIPPER));

	client->sess.teamLeader = qfalse;
	if (team == TEAM_RED || team == TEAM_BLUE) {
		teamLeader = TeamLeader(team);
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if (teamLeader == -1 ||
			(!(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT))) {
			SetLeader(team, clientNum);
		}
	}
	// make sure there is a team leader on the team the player came from
	if (oldTeam == TEAM_RED || oldTeam == TEAM_BLUE) {
		CheckTeamLeader(oldTeam);
	}

	BroadcastTeamChange(client, oldTeam);

	// get and distribute relevant parameters
	ClientUserinfoChanged(clientNum);

	// client hasn't spawned yet, they sent an early team command
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}

	ClientBegin(clientNum);
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing(gentity_t *ent) {
	// TODO: think about this if ...
	// (in debug I saw: on real spec sess.sessionTeam stays TEAM_SPEC...)
	// ... is there any case where sess.sessionTeam realy needs to be set again?
	if (g_gametype.integer != GT_LPS || ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		ent->client->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
	}

	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;

	SetClientViewAngle(ent, ent->client->ps.viewangles);

	// don't use dead view angles
	if (ent->client->ps.stats[STAT_HEALTH] <= 0) {
		ent->client->ps.stats[STAT_HEALTH] = 1;
	}
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f(gentity_t *ent) {
	int oldTeam;
	char s[MAX_TOKEN_CHARS];

	if (trap_Argc() != 2) {
		oldTeam = ent->client->sess.sessionTeam;
		switch (oldTeam) {
		case TEAM_BLUE:
			trap_SendServerCommand(ent - g_entities, "print \"Blue team\n\"");
			break;
		case TEAM_RED:
			trap_SendServerCommand(ent - g_entities, "print \"Red team\n\"");
			break;
		case TEAM_FREE:
			trap_SendServerCommand(ent - g_entities, "print \"Free team\n\"");
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand(ent - g_entities, "print \"Spectator team\n\"");
			break;
		}
		return;
	}

	if (ent->client->switchTeamTime > level.time) {
		trap_SendServerCommand(ent - g_entities, "print \"May not switch teams more than once per 5 seconds.\n\"");
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ((g_gametype.integer == GT_TOURNAMENT) && ent->client->sess.sessionTeam == TEAM_FREE) {
		ent->client->sess.losses++;
	}

	trap_Argv(1, s, sizeof(s));

	SetTeam(ent, s);

	ent->client->switchTeamTime = level.time + 5000;
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f(gentity_t *ent) {
	int i;
	char arg[MAX_TOKEN_CHARS];

	if (trap_Argc() != 2) {
		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
			StopFollowing(ent);
		}
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	i = ClientNumberFromString(ent, arg);
	if (i == -1) {
		return;
	}

	// can't follow self
	if (&level.clients[i] == ent->client) {
		return;
	}

	// can't follow another spectator
	if ((level.clients[i].sess.sessionTeam == TEAM_SPECTATOR) || LPSDeadSpec(&level.clients[i])) {
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ((g_gametype.integer == GT_TOURNAMENT) && ent->client->sess.sessionTeam == TEAM_FREE) {
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ((ent->client->sess.sessionTeam != TEAM_SPECTATOR) && !LPSDeadSpec(ent->client)) {
		SetTeam(ent, "spectator");
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f(gentity_t *ent, int dir) {
	int clientnum;
	int original;

	// if they are playing a tournement game, count as a loss
	if ((g_gametype.integer == GT_TOURNAMENT) && ent->client->sess.sessionTeam == TEAM_FREE) {
		ent->client->sess.losses++;
	}

	if (g_gametype.integer == GT_LPS && (g_LPS_flags.integer & LPSF_PPOINTLIMIT) &&
		ent->client->sess.sessionTeam == TEAM_FREE && ent->client->sess.livesleft < 0)
		return; // kein following in dem mod ... sonst verlier ist die scores =/

	// first set them to spectator
	if (ent->client->sess.spectatorState == SPECTATOR_NOT) {
		SetTeam(ent, "spectator");
	}

	if (dir != 1 && dir != -1) {
		G_Error("Cmd_FollowCycle_f: bad dir %i", dir);
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;
		if (clientnum >= level.maxclients) {
			clientnum = 0;

			{
				vec3_t spawn_origin, spawn_angles;

				ent->client->sess.spectatorClient = -1; // damit es beim naechsten follow wieder bei 0 los geht ...
														// vielleicht gings auch ohne das *shrug*
				StopFollowing(ent);

				SelectSpectatorSpawnPoint(spawn_origin, spawn_angles);
				G_SetOrigin(ent, spawn_origin);
				VectorCopy(spawn_origin, ent->client->ps.origin);
				memset(ent->client->ps.velocity, 0, sizeof(vec3_t));
				SetClientViewAngle(ent, spawn_angles);
				return;
			}
		}
		if (clientnum < 0) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if (level.clients[clientnum].pers.connected != CON_CONNECTED) {
			continue;
		}

		// can't follow another spectator
		if ((level.clients[clientnum].sess.sessionTeam == TEAM_SPECTATOR) || LPSDeadSpec(&level.clients[clientnum])) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while (clientnum != original);

	// leave it where it was
}

/*
==================
G_Say
==================
*/

static void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message) {
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if (other->client->pers.connected != CON_CONNECTED) {
		return;
	}
	if (mode == SAY_TEAM && !OnSameTeam(ent, other)) {
		return;
	}
	// no chatting to players in tournements
	if ((g_gametype.integer == GT_TOURNAMENT) && other->client->sess.sessionTeam == TEAM_FREE &&
		ent->client->sess.sessionTeam != TEAM_FREE) {
		return;
	}

	trap_SendServerCommand((other - g_entities), va("say %d %ld \"%s%c%c%s\"", mode, (ent ? (ent - g_entities) : -1),
													name, Q_COLOR_ESCAPE, color, message));
}

void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText) {
	int j, cid;
	gentity_t *other;
	int color;
	char name[64], *namesrc;
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];
	char location[64];
	qboolean realEnt;

	if (g_gametype.integer < GT_TEAM && mode == SAY_TEAM) {
		mode = SAY_ALL;
	}

	// server calls don't have a valid entity
	if (!ent) {
		namesrc = "server";
		realEnt = qfalse;
		cid = -1;
	} else {
		namesrc = ent->client->pers.netname;
		realEnt = qtrue;
		cid = ent - g_entities;
	}

	//
	if (!realEnt && (SAY_TEAM == mode)) {
		ent = target;
		target = NULL;
	}

	switch (mode) {
	default: // fall through

	case SAY_ALL:
		G_LogPrintf("Say: %i %s\n", cid, chatText);
		Com_sprintf(name, sizeof(name), "%s%c%c" EC ": ", namesrc, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_YELLOW;
		break;

	case SAY_TEAM:
		G_LogPrintf("SayTeam: %i %s\n", cid, chatText);
		if (realEnt && Team_GetLocationMsg(ent, location, sizeof(location))) {
			Com_sprintf(name, sizeof(name), EC "(%s%c%c" EC ") (%s)" EC ": ", namesrc, Q_COLOR_ESCAPE, COLOR_WHITE,
						location);
		} else {
			Com_sprintf(name, sizeof(name), EC "(%s%c%c" EC ")" EC ": ", namesrc, Q_COLOR_ESCAPE, COLOR_WHITE);
		}
		color = COLOR_CYAN;
		break;

	case SAY_TELL:
		G_LogPrintf("Tell: %d %ld %s\n", cid, (target - g_entities), chatText);
		if (target && (g_gametype.integer >= GT_TEAM) &&
			(target->client->sess.sessionTeam == ent->client->sess.sessionTeam) && realEnt &&
			Team_GetLocationMsg(ent, location, sizeof(location))) {
			Com_sprintf(name, sizeof(name), EC "[%s%c%c" EC "] (%s)" EC ": ", namesrc, Q_COLOR_ESCAPE, COLOR_WHITE,
						location);
		} else {
			Com_sprintf(name, sizeof(name), EC "[%s%c%c" EC "]" EC ": ", namesrc, Q_COLOR_ESCAPE, COLOR_WHITE);
		}

		color = COLOR_YELLOW;
		break;
	}

	Q_strncpyz(text, chatText, sizeof(text));

	if (target) {
		G_SayTo(ent, target, mode, color, name, text);
		return;
	}

	// echo the text to the console
	if (g_dedicated.integer) {
		G_Printf("%s%s\n", name, text);
	}

	// send it to all the appropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo(ent, other, mode, color, name, text);
	}
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0) {
	const char *p;

	if (trap_Argc() < 2 && !arg0) {
		return;
	}

	if (arg0) {
		p = ConcatArgs(0);
	} else {
		p = ConcatArgs(1);
	}

	G_Say(ent, NULL, mode, p);
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f(gentity_t *ent) {
	int targetNum;
	gentity_t *target;
	const char *p;
	char arg[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2) {
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	targetNum = atoi(arg);
	if (targetNum < 0 || targetNum >= level.maxclients) {
		return;
	}

	target = &g_entities[targetNum];
	if (!target || !target->inuse || !target->client) {
		return;
	}

	p = ConcatArgs(2);

	G_Say(ent, target, SAY_TELL, p);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if (ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say(ent, ent, SAY_TELL, p);
	}
}

static char *gc_orders[] = {"hold your position", "hold this position", "come here", "cover me",
							"guard location",	  "search and destroy", "report"};

void Cmd_GameCommand_f(gentity_t *ent) {
	int player;
	int order;
	char str[MAX_TOKEN_CHARS];

	trap_Argv(1, str, sizeof(str));
	player = atoi(str);
	trap_Argv(2, str, sizeof(str));
	order = atoi(str);

	if (player < 0 || player >= MAX_CLIENTS) {
		return;
	}
	if (order < 0 || order > ARRAY_LEN(gc_orders)) {
		return;
	}
	G_Say(ent, &g_entities[player], SAY_TELL, gc_orders[order]);
	G_Say(ent, ent, SAY_TELL, gc_orders[order]);
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f(gentity_t *ent) {
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", vtos(ent->s.origin)));
}

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f(gentity_t *ent) {
	int i;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];
	const char *c;

	if (!g_allowVote.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Voting not allowed here.\n\"");
		return;
	}

	if (level.voteTime) {
		trap_SendServerCommand(ent - g_entities, "print \"A vote is already in progress.\n\"");
		return;
	}
	if (ent->client->pers.voteCount >= MAX_VOTE_COUNT) {
		trap_SendServerCommand(ent - g_entities, "print \"You have called the maximum number of votes.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent - g_entities, "print \"Not allowed to call a vote as spectator.\n\"");
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv(1, arg1, sizeof(arg1));
	trap_Argv(2, arg2, sizeof(arg2));

	// http://bugzilla.icculus.org/show_bug.cgi?id=3593
	// check for command separators in arg2, as it is directly executed
	for (c = arg2; *c; ++c) {
		switch (*c) {
		case '\n':
		case '\r':
		case ';':
			trap_SendServerCommand((ent - g_entities), "print \"Invalid vote string.\n\"");
			return;
		}
	}

	if (!Q_stricmp(arg1, "map_restart")) {
	} else if (!Q_stricmp(arg1, "nextmap")) {
	} else if (!Q_stricmp(arg1, "map")) {
	} else if (!Q_stricmp(arg1, "g_gametype")) {
	} else if (!Q_stricmp(arg1, "kick")) {
	} else if (!Q_stricmp(arg1, "clientkick")) {
	} else if (!Q_stricmp(arg1, "timelimit")) {
	} else if (!Q_stricmp(arg1, "pointlimit")) {
	} else if (!Q_stricmp(arg1, "setgametype")) {
	} else {
		trap_SendServerCommand(ent - g_entities, "print \"Invalid vote string.\n\"");
		trap_SendServerCommand(
			ent - g_entities,
			"print \"Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, clientkick "
			"<clientnum>, timelimit <time>, pointlimit <points>, setgametype <keyword>.\n\"");
		return;
	}

	if ((Q_stricmp(arg1, "map") == 0) || (Q_stricmp(arg1, "g_gametype") == 0) || (Q_stricmp(arg1, "kick") == 0) ||
		(Q_stricmp(arg1, "clientkick") == 0) || (Q_stricmp(arg1, "timelimit") == 0) ||
		(Q_stricmp(arg1, "pointlimit") == 0) || (Q_stricmp(arg1, "setgametype") == 0)) {
		if (trap_Argc() < 3) {
			trap_SendServerCommand((ent - g_entities), "print \"You need to supply a second argument.\n\"");
			return;
		}
	}

	if (!Q_stricmp(arg1, "setgametype")) {
		int gt = convertGTStringToGTNumber(ConcatArgs(2));

		if (gt == -1) {
			trap_SendServerCommand(ent - g_entities, "print \"Couldn't find a GameType with the keywords\n\"");
			return;
		}

		Q_strncpyz(arg1, "g_gametype", sizeof(arg1));
		Com_sprintf(arg2, sizeof(arg2), "%d", gt);
	}

	// if there is still a vote to be executed
	if (level.voteExecuteTime) {
		// don't start a vote when map change or restart is in progress
		if (!Q_stricmpn(level.voteString, "map", 3) || !Q_stricmpn(level.voteString, "nextmap", 7)) {
			trap_SendServerCommand(ent - g_entities, "print \"Vote after map change.\n\"");
			return;
		}
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteString));
	}

	// special case for g_gametype, check for bad values
	if (!Q_stricmp(arg1, "g_gametype")) {
		i = atoi(arg2);
		if (i == GT_SINGLE_PLAYER || i < GT_FFA || i >= GT_MAX_GAME_TYPE) {
			trap_SendServerCommand(ent - g_entities, "print \"Invalid gametype.\n\"");
			return;
		}

		Com_sprintf(level.voteString, sizeof(level.voteString), "set g_gametype %i", i);
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "Gametype %s", GametypeName(i));
	} else if (!Q_stricmp(arg1, "map")) {
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char s[MAX_STRING_CHARS];

		if (!trap_FS_FOpenFile(va("maps/%s.bsp", arg2), NULL, FS_READ)) {
			trap_SendServerCommand((ent - g_entities), "print \"Map not found.\"");
			return;
		}

		trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
		if (*s) {
			Com_sprintf(level.voteString, sizeof(level.voteString), "map %s; set nextmap \"%s\"", arg2, s);
		} else {
			Com_sprintf(level.voteString, sizeof(level.voteString), "map %s", arg2);
		}
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "Map %s", arg2);
	} else if (!Q_stricmp(arg1, "nextmap")) {
		char s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
		if (!*s) {
			trap_SendServerCommand(ent - g_entities, "print \"nextmap not set.\n\"");
			return;
		}
		Com_sprintf(level.voteString, sizeof(level.voteString), "vstr nextmap");
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "Next map");
	} else if (Q_stricmp(arg1, "kick") == 0) {
		gclient_t *player;
		char cleanName[64];
		int id = -1, matches = 0;

		// this is nasty, no need to have it voteable
		// kick allbots might be legitimate
		if (Q_stricmp(arg2, "all") == 0) {
			trap_SendServerCommand((ent - g_entities), "print \"Only admin is allowed to do this. If a player is named "
													   "'all', use 'callvote clientkick' instead.\n\"");
			return;
		}

		// try to circumvent renaming exploit and use cid, which can not change
		for (i = 0; i < level.maxclients; i++) {
			player = &level.clients[i];

			// TODO: Maybe we should only allow kicking of fully connected players?
			if (player->pers.connected == CON_DISCONNECTED) {
				continue;
			}

			// exact match
			if (Q_stricmp(player->pers.netname, arg2) == 0) {
				id = i;
				matches++;
				continue;
			}

			// match without colorcodes
			Q_strncpyz(cleanName, player->pers.netname, sizeof(cleanName));
			Q_CleanStr(cleanName);
			if (Q_stricmp(cleanName, arg2) == 0) {
				id = i;
				matches++;
				continue;
			}
		}

		if (id == -1) {
			trap_SendServerCommand(
				(ent - g_entities),
				"print \"No player found with that name. Check for typos or use 'callvote clientkick' instead.\n\"");
			return;
		}

		if (matches > 1) {
			trap_SendServerCommand(
				(ent - g_entities),
				"print \"Multiple players found with that name. Use 'callvote clientkick' instead.\n\"");
			return;
		}

		// TODO: Display old and current name in votestring
		Com_sprintf(level.voteString, sizeof(level.voteString), "clientkick %i", id);
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "Kick %i %s", id, arg2);

	} else if (Q_stricmp(arg1, "map_restart") == 0) {
		Com_sprintf(level.voteString, sizeof(level.voteString), "map_restart");
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "Restart map");
	} else if (Q_stricmp(arg1, "clientkick") == 0) {
		i = atoi(arg2);
		if ((i < 0) || (i >= MAX_CLIENTS)) {
			trap_SendServerCommand((ent - g_entities), "print \"Not a valid client number.\n\"");
			return;
		}
		if (level.clients[i].pers.connected == CON_DISCONNECTED) {
			trap_SendServerCommand((ent - g_entities), "print \"Client not connected.\n\"");
			return;
		}

		Com_sprintf(level.voteString, sizeof(level.voteString), "clientkick %i", i);
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "Kick %i %s", i,
					level.clients[i].pers.netname);
	} else if ((Q_stricmp(arg1, "pointlimit") == 0) || (Q_stricmp(arg1, "timelimit") == 0)) {
		i = atoi(arg2);
		if (i < 0) {
			trap_SendServerCommand((ent - g_entities), va("print \"Not a valid %s.\n\"", arg1));
			return;
		}

		Com_sprintf(level.voteString, sizeof(level.voteString), "set %s %i", arg1, i);
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s %i", arg1, i);
	} else {
		// No votes should be left
		return;
	}

	trap_SendServerCommand(-1, va("print \"%s " S_COLOR_WHITE "called a vote: %s.\n\"", ent->client->pers.netname,
								  level.voteDisplayString));
	G_LogPrintf("Callvote: %ld %s\n", (ent - g_entities), level.voteString);

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for (i = 0; i < level.maxclients; i++) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->client->ps.eFlags |= EF_VOTED;
	ent->client->pers.voteCount++;

	trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.voteTime));
	trap_SetConfigstring(CS_VOTE_STRING, level.voteDisplayString);
	trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f(gentity_t *ent) {
	char msg[64];

	if (!level.voteTime) {
		trap_SendServerCommand(ent - g_entities, "print \"No vote in progress.\n\"");
		return;
	}
	if (ent->client->ps.eFlags & EF_VOTED) {
		trap_SendServerCommand(ent - g_entities, "print \"Vote already cast.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent - g_entities, "print \"Not allowed to vote as spectator.\n\"");
		return;
	}

	trap_SendServerCommand(ent - g_entities, "print \"Vote cast.\n\"");

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv(1, msg, sizeof(msg));

	if (msg[0] == 'y' || msg[0] == 'Y' || msg[0] == '1') {
		level.voteYes++;
		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	} else {
		level.voteNo++;
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f(gentity_t *ent) {
	int i, team, cs_offset;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];
	char *c;

	team = ent->client->sess.sessionTeam;
	if (team == TEAM_RED)
		cs_offset = 0;
	else if (team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (!g_allowVote.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Voting not allowed here.\n\"");
		return;
	}

	if (level.teamVoteTime[cs_offset]) {
		trap_SendServerCommand(ent - g_entities, "print \"A team vote is already in progress.\n\"");
		return;
	}
	if (ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT) {
		trap_SendServerCommand(ent - g_entities, "print \"You have called the maximum number of team votes.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent - g_entities, "print \"Not allowed to call a vote as spectator.\n\"");
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv(1, arg1, sizeof(arg1));
	arg2[0] = '\0';
	for (i = 2; i < trap_Argc(); i++) {
		if (i > 2)
			strcat(arg2, " ");
		trap_Argv(i, &arg2[strlen(arg2)], sizeof(arg2) - strlen(arg2));
	}

	// http://bugzilla.icculus.org/show_bug.cgi?id=3593
	// check for command separators in arg2
	for (c = arg2; *c; ++c) {
		switch (*c) {
		case '\n':
		case '\r':
		case ';':
			trap_SendServerCommand((ent - g_entities), "print \"Invalid vote string.\n\"");
			return;
		}
	}

	if (!Q_stricmp(arg1, "leader")) {
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if (!arg2[0]) {
			i = ent->client->ps.clientNum;
		} else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if (!arg2[i] || arg2[i] < '0' || arg2[i] > '9')
					break;
			}
			if (i >= 3 || !arg2[i]) {
				i = atoi(arg2);
				if (i < 0 || i >= level.maxclients) {
					trap_SendServerCommand(ent - g_entities, va("print \"Bad client slot: %i\n\"", i));
					return;
				}

				if (!g_entities[i].inuse) {
					trap_SendServerCommand(ent - g_entities, va("print \"Client %i is not active\n\"", i));
					return;
				}
			} else {
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader);
				for (i = 0; i < level.maxclients; i++) {
					if (level.clients[i].pers.connected == CON_DISCONNECTED)
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname);
					if (!Q_stricmp(netname, leader)) {
						break;
					}
				}
				if (i >= level.maxclients) {
					trap_SendServerCommand(ent - g_entities,
										   va("print \"%s is not a valid player on your team.\n\"", arg2));
					return;
				}
			}
		}
		Com_sprintf(arg2, sizeof(arg2), "%d", i);
	} else {
		trap_SendServerCommand(ent - g_entities, "print \"Invalid vote string.\n\"");
		trap_SendServerCommand(ent - g_entities, "print \"Team vote commands are: leader <player>.\n\"");
		return;
	}

	Com_sprintf(level.teamVoteString[cs_offset], sizeof(level.teamVoteString[cs_offset]), "%s %s", arg1, arg2);

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand(i, va("print \"%s called a team vote.\n\"", ent->client->pers.netname));
	}

	// start the voting, the caller automatically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
	}
	ent->client->ps.eFlags |= EF_TEAMVOTED;
	ent->client->pers.teamVoteCount++;

	trap_SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset]));
	trap_SetConfigstring(CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset]);
	trap_SetConfigstring(CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset]));
	trap_SetConfigstring(CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset]));
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f(gentity_t *ent) {
	int team, cs_offset;
	char msg[64];

	team = ent->client->sess.sessionTeam;
	if (team == TEAM_RED)
		cs_offset = 0;
	else if (team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (!level.teamVoteTime[cs_offset]) {
		trap_SendServerCommand(ent - g_entities, "print \"No team vote in progress.\n\"");
		return;
	}
	if (ent->client->ps.eFlags & EF_TEAMVOTED) {
		trap_SendServerCommand(ent - g_entities, "print \"Team vote already cast.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent - g_entities, "print \"Not allowed to vote as spectator.\n\"");
		return;
	}

	trap_SendServerCommand(ent - g_entities, "print \"Team vote cast.\n\"");

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv(1, msg, sizeof(msg));

	if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1') {
		level.teamVoteYes[cs_offset]++;
		trap_SetConfigstring(CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset]));
	} else {
		level.teamVoteNo[cs_offset]++;
		trap_SetConfigstring(CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset]));
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}

/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f(gentity_t *ent) {
	vec3_t origin, angles;
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if (!g_cheats.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return;
	}
	if (trap_Argc() != 5) {
		trap_SendServerCommand(ent - g_entities, "print \"usage: setviewpos x y z yaw\n\"");
		return;
	}

	VectorClear(angles);
	for (i = 0; i < 3; i++) {
		trap_Argv(i + 1, buffer, sizeof(buffer));
		origin[i] = atof(buffer);
	}

	trap_Argv(4, buffer, sizeof(buffer));
	angles[YAW] = atof(buffer);

	TeleportPlayer(ent, origin, angles);
}

/*
=================
Cmd_Stats_f
=================
*/
void Cmd_Stats_f(gentity_t *ent) {
	int i;
	char buffer[3072];

	Q_strncpyz(buffer, "id bot name\n", sizeof(buffer));

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected >= CON_CONNECTING) {
			Q_strcat(buffer, sizeof(buffer),
					 va("%2i %3i %s%s\n", i, ((g_entities[i].r.svFlags & SVF_BOT) ? 1 : 0),
						level.clients[i].pers.netname, S_COLOR_WHITE));
		}
	}

	trap_SendServerCommand((ent - g_entities), va("print \"%s\n\"", buffer));
}

/*
#######################
Cmd_SelectLogo_f
#######################
*/
void Cmd_SelectLogo_f(gentity_t *ent) {
	// FIXME: This might needs some checks. Should not be empty or contain spaces
	trap_Argv(1, ent->client->sess.selectedlogo, sizeof(ent->client->sess.selectedlogo));

	ClientUserinfoChanged((ent - g_entities));

	trap_SendServerCommand((ent - g_entities),
						   va("print \"You selected '%s' as spraylogo\n\"", ent->client->sess.selectedlogo));
}

/*
#######################
Cmd_ReadyToFight_f
#######################
*/
void Cmd_ReadyToFight_f(gentity_t *ent) {
	ent->client->readyToFight = qtrue;
}

/*
#######################
Cmd_TeamReady_f
#######################
*/
void Cmd_TeamReady_f(gentity_t *ent) {
	int i;

	if (g_gametype.integer < GT_TEAM)
		return;

	if (ent->client->sess.sessionTeam != TEAM_RED && ent->client->sess.sessionTeam != TEAM_BLUE)
		return;

	for (i = 0; i < g_maxclients.integer; i++) {
		if (level.clients[i].sess.sessionTeam == ent->client->sess.sessionTeam)
			Cmd_ReadyToFight_f(i + g_entities);
	}
}

static void CartridgePickUpAbleForOwner(gentity_t *ent) {
	ent->s.otherEntityNum = -1;
	ent->nextthink = level.time + 120000;
	ent->think = G_FreeEntity;
}

/*
#######################
Cmd_DropCartridge_f
#######################
*/
// FIXME: Rename, since it also works for flags!
// TODO: This entire code is almost the same as Drop_Item(), except for
//       higher velocity and no random z
// TODO: Enforce a cooldown time
void Cmd_DropCartridge_f(gentity_t *ent) {
	gentity_t *tmpGE = NULL;
	vec3_t velocity, angles;

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		return;
	}

	// Spray your Color Team
	if (g_gametype.integer == GT_SPRAY) {
		if (ent->client->ps.ammo[WP_SPRAYPISTOL] <= 0) {
			return;
		}

		G_TempEntity(ent->s.pos.trBase, EV_DROP_CARTRIDGE);

		VectorCopy(ent->s.apos.trBase, angles);
		AngleVectors(angles, velocity, NULL, NULL);
		VectorScale(velocity, 600, velocity);
		velocity[2] += 200;

		if (ent->client->sess.sessionTeam == TEAM_RED) {
			tmpGE = LaunchItem(BG_FindItem("red Cartridge"), ent->s.pos.trBase, velocity);
		} else {
			tmpGE = LaunchItem(BG_FindItem("blue Cartridge"), ent->s.pos.trBase, velocity);
		}

		tmpGE->s.otherEntityNum = ent->s.number;
		tmpGE->nextthink = (level.time + 1000);
		tmpGE->think = CartridgePickUpAbleForOwner;

		ent->client->ps.ammo[WP_SPRAYPISTOL]--;
		ent->client->ps.generic1--;
	} else if (g_gametype.integer == GT_CTF) {
		gitem_t *item;
		vec3_t origin;
		int j;

		if (ent->client->ps.powerups[PW_REDFLAG]) {
			item = BG_FindItemForPowerup(PW_REDFLAG);
			j = PW_REDFLAG;
		} else if (ent->client->ps.powerups[PW_BLUEFLAG]) {
			item = BG_FindItemForPowerup(PW_BLUEFLAG);
			j = PW_BLUEFLAG;
		} else {
			return;
		}

		if (item) {
			if (ent->client->dropTime > level.time) {
				// TODO: Print some info to player?
				return;
			}
			ent->client->dropTime = (level.time + DROP_DELAY_LOLLY);

			VectorCopy(ent->s.pos.trBase, origin);
			origin[2] += 50;

			VectorCopy(ent->s.apos.trBase, angles);

			AngleVectors(angles, velocity, NULL, NULL);
			VectorScale(velocity, 600, velocity);
			velocity[2] += 200;
			tmpGE = LaunchItem(item, origin, velocity);

			ent->client->ps.powerups[j] = 0;
		}
	}

	if (tmpGE)
		G_LogPrintf("DropItem: %ld %s\n", (ent - g_entities), tmpGE->classname);
}

// cyr{
void EditPlayerInventory(gentity_t *ent, int arg_offset) {
	char itstr[MAX_TOKEN_CHARS];
	char arg[MAX_TOKEN_CHARS];

	trap_Argv(arg_offset, itstr, sizeof(itstr));

	if (Q_stricmp(itstr, "reset") == 0) {
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
		memset(&ent->client->ps.powerups, 0, sizeof(int) * MAX_POWERUPS);
		memset(&ent->client->ps.ammo, 0, sizeof(int) * MAX_WEAPONS);
	} else if (Q_stricmp(itstr, "ammo_spray") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.ammo[WP_SPRAYPISTOL] += atoi(arg);
	} else if (Q_stricmp(itstr, "health") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		G_Damage(ent, NULL, NULL, NULL, NULL, atoi(arg), 0, MOD_UNKNOWN);
		// ent->client->ps.stats[STAT_HEALTH] -= atoi(arg);
	} else if (Q_stricmp(itstr, "armor") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.stats[STAT_ARMOR] += atoi(arg);
	}
	// give powerup
	// ------------
	else if (Q_stricmp(itstr, "pu_revival") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.powerups[PW_REVIVAL] = level.time + 1000 * atoi(arg);
	} else if (Q_stricmp(itstr, "pu_speedy") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.powerups[PW_SPEEDY] = level.time + 1000 * atoi(arg);
	} else if (Q_stricmp(itstr, "pu_visionless") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.powerups[PW_VISIONLESS] = level.time + 1000 * atoi(arg);
	} else if (Q_stricmp(itstr, "pu_jumper") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.powerups[PW_JUMPER] = level.time + 1000 * atoi(arg);
		// G_Printf("jumper %d\n", ent->client->ps.powerups[PW_JUMPER] );
	} else if (Q_stricmp(itstr, "pu_padpower") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.powerups[PW_PADPOWER] = level.time + 1000 * atoi(arg);
	} else if (Q_stricmp(itstr, "pu_berserker") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.powerups[PW_BERSERKER] = level.time + 30000;
	} else if (Q_stricmp(itstr, "pu_floater") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = MODELINDEX_FLOATER;
		ent->client->ps.stats[STAT_HOLDABLEVAR] = level.time + 1000 * atoi(arg);
	} else if (Q_stricmp(itstr, "pu_killerducks") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = MODELINDEX_KILLERDUCKS;
		ent->client->ps.stats[STAT_HOLDABLEVAR] = 5;
	} else if (Q_stricmp(itstr, "pu_bambam") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = MODELINDEX_BAMBAM;
		ent->client->ps.stats[STAT_HOLDABLEVAR] = 3;
	} else if (Q_stricmp(itstr, "pu_boomie") == 0) {
		trap_Argv(arg_offset + 1, arg, sizeof(arg));
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = MODELINDEX_BOOMIE;
		ent->client->ps.stats[STAT_HOLDABLEVAR] = 3;
	}
	// give weapon
	//------------
	else if (Q_stricmp(itstr, "weap_boaster") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_BOASTER);
		ent->client->ps.ammo[WP_BOASTER] = 50;
	} else if (Q_stricmp(itstr, "weap_splasher") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_SPLASHER);
		ent->client->ps.ammo[WP_SPLASHER] = 50;
	} else if (Q_stricmp(itstr, "weap_pumper") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_PUMPER);
		ent->client->ps.ammo[WP_PUMPER] = 50;
	} else if (Q_stricmp(itstr, "weap_balloony") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_BALLOONY);
		ent->client->ps.ammo[WP_BALLOONY] = 50;
	} else if (Q_stricmp(itstr, "weap_betty") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_BETTY);
		ent->client->ps.ammo[WP_BETTY] = 50;
	} else if (Q_stricmp(itstr, "weap_bubbleg") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_BUBBLEG);
		ent->client->ps.ammo[WP_BUBBLEG] = 50;
	} else if (Q_stricmp(itstr, "weap_imp") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << WP_IMPERIUS);
		ent->client->ps.ammo[WP_IMPERIUS] = 50;
	}
	// fire a duck
	// -----------
	else if (Q_stricmp(itstr, "fire_duck") == 0) {
		vec3_t forward, right, up, muzzle;

		// get muzzle and forward
		AngleVectors(ent->client->ps.viewangles, forward, right, up);
		CalcMuzzlePoint(ent, forward, right, up, muzzle);
		// fire
		fire_duck(ent, muzzle, forward);
	}
}

void Cmd_EditBotInv_f(gentity_t *ent) {
	gentity_t *ent2;
	int i;
	gentity_t *spec_ent;

	if (!bot_developer.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"this cmd is only for bot testing\n\"");
		return;
	}

	spec_ent = &g_entities[ent->client->sess.spectatorClient];

	if (spec_ent != ent) {
		if (!(ent->r.svFlags & SVF_BOT)) {
			trap_SendServerCommand(ent - g_entities, "print \"error: target is human \n\"");
			return;
		}

		EditPlayerInventory(spec_ent, 1);
	} else {
		// item for all bots
		ent2 = &g_entities[0];
		for (i = 0; i < MAX_CLIENTS; i++, ent2++) {
			if (!(ent2->r.svFlags & SVF_BOT)) {
				continue;
			}

			EditPlayerInventory(ent2, 1);
		}
	}
}
// cyr}

/*
=================
ClientCommand
=================
*/
void ClientCommand(int clientNum) {
	gentity_t *ent;
	char cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if (!ent->client || ent->client->pers.connected != CON_CONNECTED) {
		if (ent->client && ent->client->pers.localClient) {
			// Handle early team command sent by UI when starting a local
			// team play game.
			trap_Argv(0, cmd, sizeof(cmd));
			if (Q_stricmp(cmd, "team") == 0) {
				Cmd_Team_f(ent);
			}
		}
		return; // not fully in game yet
	}

	trap_Argv(0, cmd, sizeof(cmd));

	if (Q_stricmp(cmd, "say") == 0) {
		Cmd_Say_f(ent, SAY_ALL, qfalse);
		return;
	}
	if (Q_stricmp(cmd, "say_team") == 0) {
		Cmd_Say_f(ent, SAY_TEAM, qfalse);
		return;
	}
	if (Q_stricmp(cmd, "tell") == 0) {
		Cmd_Tell_f(ent);
		return;
	}
	if (Q_stricmp(cmd, "score") == 0) {
		Cmd_Score_f(ent);
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime) {
		// This just causes spam
		// Cmd_Say_f( ent, qfalse, qtrue );
		return;
	}

	if (Q_stricmp(cmd, "give") == 0)
		Cmd_Give_f(ent);
	else if (Q_stricmp(cmd, "god") == 0)
		Cmd_God_f(ent);
	else if (Q_stricmp(cmd, "notarget") == 0)
		Cmd_Notarget_f(ent);
	else if (Q_stricmp(cmd, "noclip") == 0)
		Cmd_Noclip_f(ent);
	else if (Q_stricmp(cmd, "kill") == 0)
		Cmd_Kill_f(ent);
	else if (Q_stricmp(cmd, "levelshot") == 0)
		Cmd_LevelShot_f(ent);
	else if (Q_stricmp(cmd, "follow") == 0)
		Cmd_Follow_f(ent);
	else if (Q_stricmp(cmd, "follownext") == 0)
		Cmd_FollowCycle_f(ent, 1);
	else if (Q_stricmp(cmd, "followprev") == 0)
		Cmd_FollowCycle_f(ent, -1);
	else if (Q_stricmp(cmd, "team") == 0)
		Cmd_Team_f(ent);
	else if (Q_stricmp(cmd, "where") == 0)
		Cmd_Where_f(ent);
	else if (Q_stricmp(cmd, "callvote") == 0 || Q_stricmp(cmd, "cv") == 0)
		Cmd_CallVote_f(ent);
	else if (Q_stricmp(cmd, "vote") == 0)
		Cmd_Vote_f(ent);
	else if (Q_stricmp(cmd, "gc") == 0)
		Cmd_GameCommand_f(ent);
	else if (Q_stricmp(cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f(ent);
	else if (Q_stricmp(cmd, "stats") == 0)
		Cmd_Stats_f(ent);
	else if (Q_stricmp(cmd, "selectlogo") == 0)
		Cmd_SelectLogo_f(ent);
	else if (Q_stricmp(cmd, "ready") == 0)
		Cmd_ReadyToFight_f(ent);
	else if (Q_stricmp(cmd, "TeamReady") == 0)
		Cmd_TeamReady_f(ent);
	else if (Q_stricmp(cmd, "dropCartridge") == 0 || Q_stricmp(cmd, "sv_dropCartridge") == 0 ||
			 Q_stricmp(cmd, "dropTeamItem") == 0)
		Cmd_DropCartridge_f(ent);
	else if (Q_stricmp(cmd, "editbotinv") == 0)
		Cmd_EditBotInv_f(ent);
	else if (Q_stricmp(cmd, "thaw") == 0)
		Cmd_Thaw_f(ent);
	else
		trap_SendServerCommand(clientNum, va("print \"unknown cmd %s\n\"", cmd));
}
