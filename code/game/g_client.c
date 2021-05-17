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

// g_client.c -- client functions that don't happen every frame

static vec3_t playerMins = {-15, -15, -24};
static vec3_t playerMaxs = {15, 15, 32};

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch(gentity_t *ent) {
	int i;

	G_SpawnInt("nobots", "0", &i);
	if (i) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt("nohumans", "0", &i);
	if (i) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivalent to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch(ent);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission(gentity_t *ent) {
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag(gentity_t *spot) {
	int i, num;
	int touch[MAX_GENTITIES];
	gentity_t *hit;
	vec3_t mins, maxs;

	VectorAdd(spot->s.origin, playerMins, mins);
	VectorAdd(spot->s.origin, playerMaxs, maxs);
	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for (i = 0; i < num; i++) {
		hit = &g_entities[touch[i]];
		// if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if (hit->client) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define MAX_SPAWN_POINTS 128
gentity_t *SelectNearestDeathmatchSpawnPoint(vec3_t from) {
	gentity_t *spot;
	vec3_t delta;
	float dist, nearestDist;
	gentity_t *nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract(spot->s.origin, from, delta);
		dist = VectorLength(delta);
		if (dist < nearestDist) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

// this should be used by the instaGib-Bots to have any idea where they could run to
// previously they used the normal spawn-code, but this is a real heavy-weight-call
qboolean GetASpawnPosition(vec3_t currentPos, vec3_t output) {
	gentity_t *spot;
	vec3_t delta;
	float squareddist, farest_squareddist;
	gentity_t *farestSpot;

	int randomBreakCnt = (rand() & 0x7f);

	farest_squareddist = 0;
	farestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract(spot->s.origin, currentPos, delta);
		squareddist = VectorLengthSquared(delta);
		if (squareddist > farest_squareddist) {
			farest_squareddist = squareddist;
			farestSpot = spot;
		}

		if (--randomBreakCnt <= 0 && farestSpot)
			break;
	}

	if (!farestSpot || randomBreakCnt > 0) {
		spot = NULL;
		while ((spot = G_Find(spot, FOFS(classname), "team_redspawn")) != NULL) {

			VectorSubtract(spot->s.origin, currentPos, delta);
			squareddist = VectorLengthSquared(delta);
			if (squareddist > farest_squareddist) {
				farest_squareddist = squareddist;
				farestSpot = spot;
			}

			if (--randomBreakCnt <= 0 && farestSpot)
				break;
		}
	}

	if (!farestSpot || randomBreakCnt > 0) {
		spot = NULL;
		while ((spot = G_Find(spot, FOFS(classname), "team_bluespawn")) != NULL) {

			VectorSubtract(spot->s.origin, currentPos, delta);
			squareddist = VectorLengthSquared(delta);
			if (squareddist > farest_squareddist) {
				farest_squareddist = squareddist;
				farestSpot = spot;
			}

			if (--randomBreakCnt <= 0 && farestSpot)
				break;
		}
	}

	if (farestSpot) {
		VectorCopy(farestSpot->s.origin, output);

		return qtrue;
	}

	return qfalse;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_SPAWN_POINTS 128
gentity_t *SelectRandomDeathmatchSpawnPoint(qboolean isbot) {
	gentity_t *spot;
	int count;
	int selection;
	gentity_t *spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL && count < MAX_SPAWN_POINTS) {
		if (SpotWouldTelefrag(spot))
			continue;

		if (((spot->flags & FL_NO_BOTS) && isbot) || ((spot->flags & FL_NO_HUMANS) && !isbot)) {
			// spot is not for this human/bot player
			continue;
		}

		spots[count] = spot;
		count++;
	}

	if (!count) { // no spots that won't telefrag
		return G_Find(NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[selection];
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
static gentity_t *SelectRandomFurthestSpawnPoint(const vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot) {
	gentity_t *spot;
	vec3_t delta;
	float squareddist;
	float list_squareddist[64];
	gentity_t *list_spot[64];
	int numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if (SpotWouldTelefrag(spot))
			continue;

		if (((spot->flags & FL_NO_BOTS) && isbot) || ((spot->flags & FL_NO_HUMANS) && !isbot)) {
			// spot is not for this human/bot player
			continue;
		}

		VectorSubtract(spot->s.origin, avoidPoint, delta);

		squareddist = VectorLengthSquared(delta);
		for (i = 0; i < numSpots; i++) {
			if (squareddist > list_squareddist[i]) {
				if (numSpots >= 64)
					numSpots = 64 - 1;
				for (j = numSpots; j > i; j--) {
					list_squareddist[j] = list_squareddist[j - 1];
					list_spot[j] = list_spot[j - 1];
				}
				list_squareddist[i] = squareddist;
				list_spot[i] = spot;

				numSpots++;
				break;
			}
		}

		if (i >= numSpots && numSpots < MAX_SPAWN_POINTS) {
			list_squareddist[numSpots] = squareddist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots) {
		// to make Telefrags impossible, we need to generate some extra "spawnpoints"
		// ... this is done by looking above the normale spawnpoints (if there is place for some extra spawning)
		// TODO: this code hasn't been build into the team-spawn-point-code

		spot = NULL;
		j = 1;

		for (i = 1; j > 0; i++) {
			j = 0;
			while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
				gentity_t tmpent;
				trace_t tr;

				tmpent.s.origin[0] = spot->s.origin[0];
				tmpent.s.origin[1] = spot->s.origin[1];
				tmpent.s.origin[2] = spot->s.origin[2] + 9 + 64 * i;
				trap_Trace(&tr, spot->s.origin, playerMins, playerMaxs, tmpent.s.origin, ENTITYNUM_NONE,
						   (CONTENTS_SOLID | CONTENTS_PLAYERCLIP));
				if (tr.fraction != 1.0f)
					continue;

				j++;
				if (SpotWouldTelefrag(&tmpent))
					continue;

				VectorCopy(tmpent.s.origin, origin);
				//				origin[2] += 9;//sind jetzt vor dem ueberpruefen
				VectorCopy(spot->s.angles, angles);
				return spot;
			}
		}

		// TODO: this should be the first thing checked
		spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");
		if (!spot)
			G_Error("Couldn't find info_player_deathmatch entity (SelectRandomFurthestSpawnPoint)");

		VectorCopy(spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy(spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy(list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy(list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint(const vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot) {
#if 1
	return SelectRandomFurthestSpawnPoint(avoidPoint, origin, angles, isbot);
#else
	gentity_t *spot;
	gentity_t *nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint(avoidPoint);

	spot = SelectRandomDeathmatchSpawnPoint();
	if (spot == nearestSpot) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint();
		if (spot == nearestSpot) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint();
		}
	}

	// find a single player start spot
	if (!spot) {
		G_Error("Couldn't find a spawn point");
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);

	return spot;
#endif
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
static gentity_t *SelectInitialSpawnPoint(vec3_t origin, vec3_t angles, qboolean isbot) {
	gentity_t *spot;

	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if (((spot->flags & FL_NO_BOTS) && isbot) || ((spot->flags & FL_NO_HUMANS) && !isbot)) {
			continue;
		}

		if ((spot->spawnflags & 0x01))
			break;
	}

	if (!spot || SpotWouldTelefrag(spot))
		return SelectSpawnPoint(vec3_origin, origin, angles, isbot);

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint(vec3_t origin, vec3_t angles) {
	FindIntermissionPoint();

	VectorCopy(level.intermission_origin, origin);
	VectorCopy(level.intermission_angle, angles);

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
===============
InitBodyQue
===============
*/
void InitBodyQue(void) {
	int i;
	gentity_t *ent;

	level.bodyQueIndex = 0;
	for (i = 0; i < BODY_QUEUE_SIZE; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and disappear
wop: we no longer sink ... we fly XD (this is done on the clientside, to make it smooth)
=============
*/
void BodySink(gentity_t *ent) {
	if (LPSFinalDead(&level.clients[ent->s.otherEntityNum2]) &&
		(level.clients[ent->s.otherEntityNum2].pers.connected == CON_CONNECTED)) {
		ent->nextthink = (level.time + 5000);
		ent->timestamp = level.time;
		return;
	}
	if (level.time - ent->timestamp > 6500) {
		trap_UnlinkEntity(ent); // the body ques are never actually freed, they are just unlinked
		ent->physicsObject = qfalse;
		return;
	}
	ent->nextthink = level.time + 200;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue(gentity_t *ent) {
	gentity_t *body;
	int contents;

	trap_UnlinkEntity(ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents(ent->s.origin, -1);
	if (contents & CONTENTS_NODROP) {
		return;
	}

	if (level.intermissiontime)
		return; // maybe this was the reason for deadbodys in the lps-intermission-view

	// grab a body que and cycle to the next one
	body = level.bodyQue[ent->s.number]; // noch zu ändern falls BODY_QUEUE_SIZE geändert wird

	trap_UnlinkEntity(body);

	body->s = ent->s;
	body->s.eFlags = EF_DEAD | EF_HURT; // clear EF_TALK, etc
	if (LPSFinalDead(ent->client) && (ent->client->pers.connected == CON_CONNECTED)) {
		body->s.time = 0;
	} else {
		body->s.time = level.time;
	}
	body->s.powerups = 0;  // clear powerups
	body->s.loopSound = 0; // clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0; // don't bounce
	if (body->s.groundEntityNum == ENTITYNUM_NONE) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy(ent->client->ps.velocity, body->s.pos.trDelta);
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch (body->s.legsAnim & ~ANIM_TOGGLEBIT) {
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy(ent->r.mins, body->r.mins);
	VectorCopy(ent->r.maxs, body->r.maxs);
	VectorCopy(ent->r.absmin, body->r.absmin);
	VectorCopy(ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->s.otherEntityNum2 = ent->s.number;
	body->nextthink = level.time + 1000;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if (ent->health <= GIB_HEALTH) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}

	VectorCopy(body->s.pos.trBase, body->r.currentOrigin);
	trap_LinkEntity(body);
}

//======================================================================

/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle(gentity_t *ent, vec3_t angle) {
	int i;

	// set the delta angle
	for (i = 0; i < 3; i++) {
		int cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy(angle, ent->s.angles);
	VectorCopy(ent->s.angles, ent->client->ps.viewangles);
}

/*
================
respawn
================
*/
void respawn(gentity_t *ent) {
	gentity_t *tent;

	CopyToBodyQue(ent);
	ClientSpawn(ent);

	// add a teleportation effect
	if (g_gametype.integer < GT_TEAM)
		tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
	else
		tent = G_TempEntity(ent->client->ps.origin, ent->client->sess.sessionTeam == TEAM_RED
														? EV_PLAYER_TELEPORT_RED_IN
														: EV_PLAYER_TELEPORT_BLUE_IN);
	tent->s.clientNum = ent->s.clientNum;
}

/*
================
TeamCount

Returns number of players on a team
================
*/
int TeamCount(int ignoreClientNum, int team) {
	int i;
	int count = 0;

	for (i = 0; i < level.maxclients; i++) {
		if (i == ignoreClientNum) {
			continue;
		}
		if (level.clients[i].pers.connected == CON_DISCONNECTED) {
			continue;
		}
		if (level.clients[i].sess.sessionTeam == team) {
			count++;
		}
	}

	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader(int team) {
	int i;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected == CON_DISCONNECTED) {
			continue;
		}
		if (level.clients[i].sess.sessionTeam == team) {
			if (level.clients[i].sess.teamLeader)
				return i;
		}
	}

	return -1;
}

/*
================
PickTeam

================
*/
team_t PickTeam(int ignoreClientNum) {
	int counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount(ignoreClientNum, TEAM_BLUE);
	counts[TEAM_RED] = TeamCount(ignoreClientNum, TEAM_RED);

	if (counts[TEAM_BLUE] > counts[TEAM_RED]) {
		return TEAM_RED;
	}
	if (counts[TEAM_RED] > counts[TEAM_BLUE]) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if (level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED]) {
		return TEAM_RED;
	}
	return TEAM_BLUE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = strrchr(model, '/')) != 0) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientCleanName
============
*/
static void ClientCleanName(const char *in, char *out, int outSize) {
	int outpos = 0, colorlessLen = 0, spaces = 0;

	// discard leading spaces
	for (; *in == ' '; in++)
		;

	for (; *in && outpos < outSize - 1; in++) {
		out[outpos] = *in;

		if (*in == ' ') {
			// don't allow too many consecutive spaces
			if (spaces > 2)
				continue;

			spaces++;
		} else if (outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE) {
			if (Q_IsColorString(&out[outpos - 1])) {
				colorlessLen--;

				if (ColorIndex(*in) == 0) {
					// Disallow color black in names to prevent players
					// from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
			} else {
				spaces = 0;
				colorlessLen++;
			}
		} else {
			spaces = 0;
			colorlessLen++;
		}

		outpos++;
	}

	out[outpos] = '\0';

	// don't allow empty names
	if (*out == '\0' || colorlessLen == 0)
		Q_strncpyz(out, "UnnamedPlayer", outSize);
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged(int clientNum) {
	gentity_t *ent;
	int teamLeader, team, health;
	const char *s;
	char model[MAX_QPATH];
	char headModel[MAX_QPATH];
	char oldname[MAX_STRING_CHARS];
	gclient_t *client;
	char c1[MAX_INFO_STRING];
	char c2[MAX_INFO_STRING];
	char userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo)) {
		strcpy(userinfo, "\\name\\badinfo");
		// Why keep those clients and userinfo? Get out!
		trap_DropClient(clientNum, "Invalid userinfo");
	}

	// check for local client
	s = Info_ValueForKey(userinfo, "ip");
	if (!strcmp(s, "localhost") || !strcmp(s, "loopback")) {
		client->pers.localClient = qtrue;
	}

	// check the item prediction
	s = Info_ValueForKey(userinfo, "cg_predictItems");
	if (!atoi(s)) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}

	// set name
	Q_strncpyz(oldname, client->pers.netname, sizeof(oldname));
	s = Info_ValueForKey(userinfo, "name");
	ClientCleanName(s, client->pers.netname, sizeof(client->pers.netname));

	if ((client->sess.sessionTeam == TEAM_SPECTATOR) || LPSDeadSpec(client)) {
		if (client->sess.spectatorState == SPECTATOR_SCOREBOARD) {
			Q_strncpyz(client->pers.netname, "scoreboard", sizeof(client->pers.netname));
		}
	}

	if (client->pers.connected == CON_CONNECTED) {
		if (strcmp(oldname, client->pers.netname) != 0) {
			trap_SendServerCommand(-1,
								   va("print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname, client->pers.netname));
		}
	}

	health = atoi(Info_ValueForKey(userinfo, "handicap"));
	client->pers.maxHealth = health;
	if (client->pers.maxHealth < 1 || client->pers.maxHealth > 100) {
		client->pers.maxHealth = 100;
	}
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// set model
	if (g_gametype.integer >= GT_TEAM) {
		Q_strncpyz(model, Info_ValueForKey(userinfo, "team_model"), sizeof(model));
		Q_strncpyz(headModel, Info_ValueForKey(userinfo, "team_headmodel"), sizeof(headModel));
	} else {
		Q_strncpyz(model, Info_ValueForKey(userinfo, "model"), sizeof(model));
		Q_strncpyz(headModel, Info_ValueForKey(userinfo, "headmodel"), sizeof(headModel));
	}

	team = client->sess.sessionTeam;

	// teamInfo
	s = Info_ValueForKey(userinfo, "teamoverlay");
	if (!*s || atoi(s) != 0) {
		client->pers.teamInfo = qtrue;
	} else {
		client->pers.teamInfo = qfalse;
	}

	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
	Q_strncpyz(c1, Info_ValueForKey(userinfo, "color1"), sizeof(c1));
	Q_strncpyz(c2, Info_ValueForKey(userinfo, "syc_color"), sizeof(c2));

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if (ent->r.svFlags & SVF_BOT) {
		// cyr{
		char *rnd_str;
		int rnd;
		rnd = random() * 5.9;
		rnd_str = va("%d", rnd);
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tl\\%"
			   "d\\sl\\%s",
			   client->pers.netname, team, model, headModel, c1, rnd_str, client->pers.maxHealth, client->sess.wins,
			   client->sess.losses, Info_ValueForKey(userinfo, "skill"), teamLeader,
			   client->sess.selectedlogo);
		// cyr}
	} else {
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tl\\%d\\sl\\%s",
			   client->pers.netname, client->sess.sessionTeam, model, headModel, c1, c2, client->pers.maxHealth,
			   client->sess.wins, client->sess.losses, teamLeader, client->sess.selectedlogo);
	}

	trap_SetConfigstring(CS_PLAYERS + clientNum, s);

	// this is not the userinfo, more like the configstring actually
	G_LogPrintf("ClientUserinfoChanged: %i %s\n", clientNum, s);
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect(int clientNum, qboolean firstTime, qboolean isBot) {
	const char *value;
	//	const char *areabits;
	gclient_t *client;
	char userinfo[MAX_INFO_STRING];
	gentity_t *ent;

	ent = &g_entities[clientNum];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	// There's no point in letting those clients connect
	if (!Info_Validate(userinfo)) {
		return "Invalid userinfo.";
	}

	// IP filtering
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
	// check to see if they are on the banned IP list
	value = Info_ValueForKey(userinfo, "ip");
	if (G_FilterPacket(value)) {
		return "You are banned from this server.";
	}

	// we don't check password for bots and local client
	// NOTE: local client <-> "ip" "localhost"
	//   this means this client is not running in our current process
	if (!(isBot) && (strcmp(value, "localhost") != 0)) {
		// check for a password
		value = Info_ValueForKey(userinfo, "password");
		if (g_password.string[0] && Q_stricmp(g_password.string, "none") && strcmp(g_password.string, value) != 0) {
			return "Invalid password";
		}
	}

	// if a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get
	// lost in the ether
	if (ent->inuse) {
		const int clientNum = (int)(ent - g_entities);
		G_LogPrintf("Forcing disconnect on active client: %i\n", clientNum);
		// so lets just fix up anything that should happen on a disconnect
		ClientDisconnect(clientNum);
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	//	areabits = client->areabits;

	memset(client, 0, sizeof(*client));

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if (firstTime || level.newSession) {
		G_InitSessionData(client, userinfo);
	}
	G_ReadSessionData(client);

	if (isBot) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if (!G_BotConnect(clientNum, !firstTime)) {
			return "BotConnectfailed";
		}
	}

	// get and distribute relevant parameters
	G_LogPrintf("ClientConnect: %i %s %s\n", clientNum, Info_ValueForKey(userinfo, "cl_guid"),
				Info_ValueForKey(userinfo, "ip"));
	ClientUserinfoChanged(clientNum);

	// don't do the "xxx connected" messages if they were caried over from previous level
	if (firstTime) {
		trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname));
	}

	if (g_gametype.integer >= GT_TEAM && client->sess.sessionTeam != TEAM_SPECTATOR) {
		BroadcastTeamChange(client, -1);
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	if (g_gametype.integer == GT_LPS)
		SendScoreboardMessageToAllClients(); // LPS uses scores to draw its wallhack icons

	// for statistics
	//	client->areabits = areabits;
	//	if ( !client->areabits )
	//		client->areabits = G_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );

	return NULL;
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin(int clientNum) {
	gentity_t *ent;
	gclient_t *client;
	gentity_t *tent;
	int flags;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if (ent->r.linked) {
		trap_UnlinkEntity(ent);
	}
	G_InitGentity(ent);
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// freezetag
	client->pers.ftLateJoin = qfalse;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	memset(&client->ps, 0, sizeof(client->ps));
	client->ps.eFlags = flags;

	// locate ent at a spawn point
	ClientSpawn(ent);

	if (level.cammode) // disable teleport effect during cammode
		return;

	if ((client->sess.sessionTeam != TEAM_SPECTATOR) && !LPSDeadSpec(client)) {
		if (g_gametype.integer < GT_TEAM)
			tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
		else
			tent = G_TempEntity(ent->client->ps.origin, ent->client->sess.sessionTeam == TEAM_RED
															? EV_PLAYER_TELEPORT_RED_IN
															: EV_PLAYER_TELEPORT_BLUE_IN);
		tent->s.clientNum = ent->s.clientNum;

		if (g_gametype.integer != GT_TOURNAMENT) {
			trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname));
		}
	}
	G_LogPrintf("ClientBegin: %i\n", clientNum);

	/*
	 * players joining late in the game will be flagged this way to
	 * get frozen automatically
	 */
	if (G_FreezeTag()) {
		if (client->sess.sessionTeam != TEAM_SPECTATOR && FT_MatchInProgress() &&
			(level.time - level.startTime) > g_ft_lateJoinTime.integer * 1000)
			client->pers.ftLateJoin = qtrue;
	}

	// count current clients and rank for scoreboard
	CalculateRanks();
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn(gentity_t *ent) {
	int index;
	vec3_t spawn_origin, spawn_angles;
	gclient_t *client;
	int i;
	clientPersistant_t saved;
	clientSession_t savedSess;
	int persistant[MAX_PERSISTANT];
	gentity_t *spawnPoint;
	int flags;
	int savedPing;
	//	char	*savedAreaBits;
	int accuracy_hits, accuracy_shots;
	int eventSequence;
	char userinfo[MAX_INFO_STRING];

	index = ent - g_entities;
	client = ent->client;

	if (level.warmupTime)
		ent->client->sess.livesleft = g_LPS_startlives.integer;

	VectorClear(spawn_origin);

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client

	if (level.cammode && (ent->r.svFlags & SVF_BOT)) {
		VectorCopy(level.cam_spawnpos, spawn_origin);
		VectorCopy(level.cam_spawnangles, spawn_angles);
		spawnPoint = NULL;
	} else if ((client->sess.sessionTeam == TEAM_SPECTATOR) || LPSDeadSpec(client)) {
		spawnPoint = SelectSpectatorSpawnPoint(spawn_origin, spawn_angles);
	} else if (g_gametype.integer >= GT_TEAM) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint(client->sess.sessionTeam, client->pers.teamState.state, spawn_origin,
										 spawn_angles, !!(ent->r.svFlags & SVF_BOT));
	} else {
		// the first spawn should be at a good looking spot
		if (!client->pers.initialSpawn && client->pers.localClient) {
			client->pers.initialSpawn = qtrue;
			spawnPoint = SelectInitialSpawnPoint(spawn_origin, spawn_angles, !!(ent->r.svFlags & SVF_BOT));
		} else {
			// don't spawn near existing origin if possible
			spawnPoint = SelectSpawnPoint(client->ps.origin, spawn_origin, spawn_angles, !!(ent->r.svFlags & SVF_BOT));
		}
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for (i = 0; i < MAX_PERSISTANT; i++) {
		persistant[i] = client->ps.persistant[i];
	}
	eventSequence = client->ps.eventSequence;

	memset(client, 0, sizeof(*client)); // bk FIXME: Com_Memset?

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;

	for (i = 0; i < MAX_PERSISTANT; i++) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo(index, userinfo, sizeof(userinfo));
	// set max health
	client->pers.maxHealth = atoi(Info_ValueForKey(userinfo, "handicap"));
	if (client->pers.maxHealth < 1 || client->pers.maxHealth > 100) {
		client->pers.maxHealth = 100;
	}
	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	// No one has hit the client yet!
	client->lastSentFlying = -1;

	VectorCopy(playerMins, ent->r.mins);
	VectorCopy(playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	// weapon inventory / Modifiers
	if (g_modInstagib.integer) {
		int weapon;
		weapon = Instagib_getSpawnWeapon();

		// add instagib weapon to client's inventory
		client->ps.stats[STAT_WEAPONS] = (1 << weapon);
		client->ps.ammo[weapon] = INFINITE;
	} else if (G_FreezeTag()) {
		FT_AddStartWeapons(client);
	} else {
		// add normal wop weapons to client's inventory
		client->ps.stats[STAT_WEAPONS] = (1 << WP_NIPPER);
		if (g_gametype.integer == GT_TEAM) {
			client->ps.ammo[WP_NIPPER] = 50;
		} else {
			client->ps.ammo[WP_NIPPER] = 100;
		}

		client->ps.stats[STAT_WEAPONS] |= (1 << WP_PUNCHY);
		client->ps.ammo[WP_PUNCHY] = -1;
	}

	// client->ps.ammo[WP_GRAPPLING_HOOK] = -1;

	// in SYC gametypes, add spraypistol to inventory
	if (IsSyc()) {
		client->ps.stats[STAT_WEAPONS] |= (1 << WP_SPRAYPISTOL);
	}

	// health will count down towards max_health
	ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] + 25;
	if (g_gametype.integer == GT_LPS) {
		client->ps.stats[STAT_ARMOR] = 100;
	}

	// freezetag
	if (G_FreezeTag())
		client->ps.stats[STAT_ARMOR] = 125;

	G_SetOrigin(ent, spawn_origin);
	VectorCopy(spawn_origin, client->ps.origin);

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd(client - level.clients, &ent->client->pers.cmd);
	SetClientViewAngle(ent, spawn_angles);

	if (ent->client->sess.livesleft == 0) {
		ent->client->sess.livesleft = -1;
		ent->client->buttons |=
			(BUTTON_ATTACK | BUTTON_USE_HOLDABLE); // add attack to oldButtons, so we don't get into follow-mode
	}

	if ((ent->client->sess.sessionTeam == TEAM_SPECTATOR) || LPSDeadSpec(ent->client)) {
	} else {
		G_KillBox(ent);
		trap_LinkEntity(ent);

		// force the base weapon up
		client->ps.weapon = WP_NIPPER;
		client->ps.weaponstate = WEAPON_READY;
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if (level.intermissiontime) {
		MoveClientToIntermission(ent);
	} else {
		// fire the targets of the spawn point
		G_UseTargets(spawnPoint, ent);

		// select the highest weapon number available, after any
		// spawn given items have fired
		client->ps.weapon = WP_NONE;
		for (i = WP_NUM_WEAPONS - 1; i > 0; i--) {
			if (i == WP_SPRAYPISTOL)
				continue;
			if (client->ps.stats[STAT_WEAPONS] & (1 << i)) {
				client->ps.weapon = i;
				break;
			}
		}
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink(ent - g_entities);

	// positively link the client, even if the command times are weird
	if ((ent->client->sess.sessionTeam != TEAM_SPECTATOR) && !LPSDeadSpec(ent->client)) {
		BG_PlayerStateToEntityState(&client->ps, &ent->s, qtrue);
		VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);
		trap_LinkEntity(ent);
	}

	// run the presend to set anything else
	ClientEndFrame(ent);

	// clear entity state values
	BG_PlayerStateToEntityState(&client->ps, &ent->s, qtrue);
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect(int clientNum) {
	gentity_t *ent;
	gentity_t *tent;
	int i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin(clientNum);

	ent = g_entities + clientNum;
	if (!ent->client || ent->client->pers.connected == CON_DISCONNECTED) {
		return;
	}

	// stop any following clients
	for (i = 0; i < level.maxclients; i++) {
		if (((level.clients[i].sess.sessionTeam == TEAM_SPECTATOR) || LPSDeadSpec(&level.clients[i])) &&
			level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW &&
			level.clients[i].sess.spectatorClient == clientNum) {
			StopFollowing(&g_entities[i]);
		}
	}

	if (!level.cammode) {
		// send effect if they were completely connected
		if (ent->client->pers.connected == CON_CONNECTED &&
			((ent->client->sess.sessionTeam != TEAM_SPECTATOR) && !LPSDeadSpec(ent->client))) {
			if (g_gametype.integer < GT_TEAM)
				tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT);
			else
				tent = G_TempEntity(ent->client->ps.origin, ent->client->sess.sessionTeam == TEAM_RED
																? EV_PLAYER_TELEPORT_RED_OUT
																: EV_PLAYER_TELEPORT_BLUE_OUT);
			VectorCopy(ent->client->ps.viewangles, tent->s.angles);

			tent->s.clientNum = ent->s.clientNum;

			// They don't get to take powerups with them!
			// Especially important for stuff like CTF flags
			TossClientItems(ent);
			RemoveOwnedItems(ent);
		}

		G_LogPrintf("ClientDisconnect: %i\n", clientNum);
	}

	// if we are playing in tourney mode and losing, give a win to the other player
	if ((g_gametype.integer == GT_TOURNAMENT) && !level.intermissiontime && !level.warmupTime &&
		level.sortedClients[1] == clientNum) {
		level.clients[level.sortedClients[0]].sess.wins++;
		ClientUserinfoChanged(level.sortedClients[0]);
	}

	trap_UnlinkEntity(ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring(CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if (ent->r.svFlags & SVF_BOT) {
		BotAIShutdownClient(clientNum, qfalse);
	}
}
