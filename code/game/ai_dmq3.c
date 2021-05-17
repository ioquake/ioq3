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

/*****************************************************************************
 * name:		ai_dmq3.c
 *
 * desc:		WoP bot AI
 *
 * $Archive: /MissionPack/code/game/ai_dmq3.c $
 *
 *****************************************************************************/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_chat.h"
#include "../botlib/be_ai_gen.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/be_ai_weap.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
#include "ai_team.h"
//
#include "chars.h" //characteristics
#include "inv.h"   //indexes into the inventory
#include "syn.h"   //synonyms
#include "match.h" //string matching types and vars

// from aasfile.h
#define AREACONTENTS_MOVER 1024
#define AREACONTENTS_MODELNUMSHIFT 24
#define AREACONTENTS_MAXMODELNUM 0xFF
#define AREACONTENTS_MODELNUM (AREACONTENTS_MAXMODELNUM << AREACONTENTS_MODELNUMSHIFT)

#define IDEAL_ATTACKDIST 140

float scorealert[3]; // for both teams, how alarming the current situation is
extern bot_state_t *botstates[MAX_CLIENTS];
int BotGetTeammates(bot_state_t *bs, int *teammates, int maxteammates);

//

// NOTE: not using a cvars which can be updated because the game should be reloaded anyway
int gametype;	// game type
int maxclients; // maximum number of clients

vmCvar_t bot_grapple;
vmCvar_t bot_rocketjump;
vmCvar_t bot_fastchat;
vmCvar_t bot_nochat;
vmCvar_t bot_testrchat;
vmCvar_t bot_challenge;
vmCvar_t bot_predictobstacles;
vmCvar_t g_spSkill;

vec3_t lastteleport_origin; // last teleport event origin
float lastteleport_time;	// last teleport event time
int max_bspmodelindex;		// maximum BSP model index

// cyr {
bot_goal_t ctf_redflag;
bot_goal_t ctf_blueflag;
bot_goal_t balloongoal[MAX_BALLOONS];
bot_goal_t rwall, bwall, spraytele, outtele;
bot_goal_t hstations[MAX_HSTATIONS];
int num_hstations;
// cyr }

qboolean EntityIsChatting(aas_entityinfo_t *entinfo);

qboolean ClientInSprayroom(int clId) {
	if (gametype != GT_SPRAYFFA && gametype != GT_SPRAY)
		return qfalse;

	if (clId < 0 || clId >= MAX_CLIENTS)
		return qfalse;
	if (!g_entities[clId].client)
		return qfalse;

	if (0 >= g_entities[clId].client->ps.stats[STAT_SPRAYROOMSECS])
		return qfalse;

	return qtrue;
}

qboolean BotCTFCarryingFlag(bot_state_t *bs) {
	if (gametype != GT_CTF)
		return CTF_FLAG_NONE;

	if (bs->inventory[INVENTORY_REDFLAG] > 0)
		return CTF_FLAG_RED;
	else if (bs->inventory[INVENTORY_BLUEFLAG] > 0)
		return CTF_FLAG_BLUE;
	return CTF_FLAG_NONE;
}

int BotGetNumClientCarts(bot_state_t *bs, int clientnum) {
	// evt noch visibility pruefen
	if (gametype == GT_SPRAY || gametype == GT_SPRAYFFA)
		return g_entities[clientnum].client->ps.ammo[WP_SPRAYPISTOL];
	else
		return 0;
}

int BotGetVisTeamPlayers(bot_state_t *bs, int *players, int maxplayers, qboolean sameteam) {
	int i, numplayers;
	// char buf[MAX_INFO_STRING];
	static int maxclients;
	aas_entityinfo_t entinfo;
	float alertness, easyfragger;

	alertness = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_ALERTNESS, 0, 1);
	easyfragger = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_EASY_FRAGGER, 0, 1);

	if (!maxclients)
		maxclients = trap_Cvar_VariableIntegerValue("sv_maxclients");

	numplayers = 0;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		float squaredist, vis;
		vec3_t dir;

		if (i == bs->client)
			continue;
		if (BotSameTeam(bs, i) != sameteam)
			continue;

		BotEntityInfo(i, &entinfo);
		// if the player is dead or invisible and not shooting, skip it
		if (!entinfo.valid || EntityIsDead(&entinfo) || (EntityIsInvisible(&entinfo) && !EntityIsShooting(&entinfo)))
			continue;
		// if not an easy fragger don't shoot at chatting players
		if (easyfragger < 0.5 && EntityIsChatting(&entinfo))
			continue;

		// calculate the distance towards the enemy
		VectorSubtract(entinfo.origin, bs->origin, dir);
		squaredist = VectorLengthSquared(dir);
		// just out of tele. orientation time
		if (lastteleport_time > FloatTime() - 2) {
			VectorSubtract(entinfo.origin, lastteleport_origin, dir);
			if (VectorLengthSquared(dir) < Square(70))
				continue;
		}
		// hm !! grosse alertnessreichweite, aber kein interesse zeigen wenn weit weg !?
		if (squaredist > Square(900.0 + alertness * 4000.0))
			continue;

		// visible ?
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, i);
		if (vis > 0 || gametype == GT_LPS) {
			players[numplayers++] = i;
			if (numplayers >= maxplayers)
				break;
		}
	}
	// G_Printf("\n %d mates \n", numteammates);
	return numplayers;
}

qboolean IsDuck(int ent) {
	if (ent < 0 || ent > MAX_GENTITIES)
		return qfalse;
	return (g_entities[ent].s.weapon == WP_KILLERDUCKS && g_entities[ent].s.eType == ET_MISSILE);
}

qboolean IsWall(int ent) {
	if (ent < 0 || ent > MAX_GENTITIES)
		return qfalse;
	return (&g_entities[ent] == level.bspraywall || &g_entities[ent] == level.rspraywall ||
			&g_entities[ent] == level.nspraywall);
}

qboolean IsBambam(int ent) {
	if (ent < 0 || ent > MAX_GENTITIES)
		return qfalse;
	return (g_entities[ent].s.eType == ET_BAMBAM);
}

qboolean IsBoomie(int ent) {
	if (ent < 0 || ent > MAX_GENTITIES)
		return qfalse;
	return (g_entities[ent].s.eType == ET_BOOMIES);
}

/*
==================
BotTeam
==================
*/
int BotTeam(bot_state_t *bs) {
	gclient_t *client;

	if (bs->client < 0 || bs->client >= MAX_CLIENTS) {
		// BotAI_Print(PRT_ERROR, "BotCTFTeam: client out of range\n");
		return qfalse;
	}
	client = g_entities[bs->client].client;
	if (!client)
		return qfalse;
	return client->sess.sessionTeam;
}

/*
==================
BotOppositeTeam
==================
*/
int BotOppositeTeam(bot_state_t *bs) {
	switch (BotTeam(bs)) {
	case TEAM_RED:
		return TEAM_BLUE;
	case TEAM_BLUE:
		return TEAM_RED;
	default:
		return TEAM_FREE;
	}
}

/*
==================
EntityIsDead
==================
*/
qboolean EntityIsDead(aas_entityinfo_t *entinfo) {
	playerState_t ps;

	if (entinfo->number >= 0 && entinfo->number < MAX_CLIENTS) {
		// retrieve the current client state
		BotAI_GetClientState(entinfo->number, &ps);
		if (ps.pm_type != PM_NORMAL)
			return qtrue;
		return qfalse;
	}

	if (IsDuck(entinfo->number))
		return qfalse;

	if (IsBambam(entinfo->number))
		return qfalse;

	if (IsBoomie(entinfo->number))
		return qfalse;

	// not a player, not a duck -> dead
	return qtrue;
}

qboolean EntityCarriesCarts(aas_entityinfo_t *entinfo) {
	if (entinfo->type != ET_PLAYER)
		return qfalse;
	return (g_entities[entinfo->number].client->ps.ammo[WP_SPRAYPISTOL]);
}
/*
==================
EntityIsInvisible
==================
*/
qboolean EntityIsInvisible(aas_entityinfo_t *entinfo) {

	// cyr_zIinvis
	if (entinfo->number < MAX_CLIENTS) {
		// spray ammo is visible
		if (EntityCarriesCarts(entinfo))
			return qfalse;
		// lps guys are visible
		if (gametype == GT_LPS)
			return qfalse;
	}
	// cyr_zIinvis

	// no sprayammo and visionless -> invis
	if (entinfo->powerups & (1 << PW_VISIONLESS)) {
		return qtrue;
	}
	return qfalse;
}

/*
==================
EntityIsShooting
==================
*/
qboolean EntityIsShooting(aas_entityinfo_t *entinfo) {
	if (entinfo->flags & EF_FIRING) {
		return qtrue;
	}
	return qfalse;
}

/*
==================
EntityIsChatting
==================
*/
qboolean EntityIsChatting(aas_entityinfo_t *entinfo) {
	if (entinfo->flags & EF_TALK) {
		return qtrue;
	}
	return qfalse;
}

/*
void BotRememberLastOrderedTask(bot_state_t *bs) {
	if (! bs->ordered) {
		return;
	}
	bs->lastgoal_decisionmaker = bs->decisionmaker;
	bs->lastgoal_ltgtype = bs->ltgtype;
	memcpy(&bs->lastgoal_teamgoal, &bs->teamgoal, sizeof(bot_goal_t));
	bs->lastgoal_teammate = bs->teammate;
}

int BotSetLastOrderedTask(bot_state_t *bs) {

	if ( bs->lastgoal_ltgtype ) {
		bs->decisionmaker = bs->lastgoal_decisionmaker;
		// bs->ordered = qtrue;
		bs->ltgtype = bs->lastgoal_ltgtype;
		memcpy(&bs->teamgoal, &bs->lastgoal_teamgoal, sizeof(bot_goal_t));
		bs->teammate = bs->lastgoal_teammate;
		bs->teamgoal_time = FloatTime() + 300;
		return qtrue;
	}
	return qfalse;
}


void BotRefuseOrder(bot_state_t *bs) {
	if (!bs->ordered)
		return;
	// if the bot was ordered to do something
	if ( bs->order_time && bs->order_time > FloatTime() - 10 ) {
		trap_EA_Action(bs->client, ACTION_NEGATIVE);
		bs->order_time = 0;
	}
}
*/

qboolean GetCTLFlagGoal(int team, bot_goal_t *goal) {
	int i;
	gentity_t *ent;
	int itemtag = (team == TEAM_RED) ? PW_REDFLAG : PW_BLUEFLAG;
	vec3_t goalorigin;

	for (i = MAX_CLIENTS; i < level.num_entities; i++) {
		ent = &g_entities[i];
		if (ent->inuse && ent->item && ent->item->giTag == itemtag && ent->flags & FL_DROPPED_ITEM) {
			memset(goal, 0, sizeof(bot_goal_t));
			goal->areanum = trap_AAS_BestReachableArea(ent->r.currentOrigin, tv(-5, -5, -5), tv(5, 5, 5), goalorigin);
			VectorCopy(goalorigin, goal->origin);
			goal->entitynum = i;
			VectorSet(goal->maxs, 8, 8, 8);
			VectorSet(goal->mins, -8, -8, -8);
			return qtrue;
		}
	}
	return qfalse;
}

// cyr{
qboolean IsMyBalloon(int team, bot_goal_t *goal) {
	int state;

	if (!goal || goal->entitynum < 0 || goal->entitynum > MAX_GENTITIES)
		return qfalse;

	state = level.balloonState[g_entities[goal->entitynum].count];
	return ((team == TEAM_RED && state == '1') || (team == TEAM_BLUE && state == '2'));
}

qboolean BotPickUnCappedBalloon(bot_state_t *bs) {
	float rnd, step;
	int i;
	int ballindex;
	int state;
	int uncaptured[MAX_BALLOONS];
	int numuncap;

	// find uncaptured balloons
	numuncap = 0;

	for (i = 0; i < level.numBalloons; i++) {
		ballindex = g_entities[balloongoal[i].entitynum].count;
		state = level.balloonState[ballindex]; // status of goal i

		if ((BotTeam(bs) == TEAM_RED && state != '1') || (BotTeam(bs) == TEAM_BLUE && state != '2')) {
			uncaptured[numuncap++] = i;
		}
	}

	if (!numuncap) {
		// todo: roam
		bs->ltgtype = 0;
		return qfalse;
	}

	// choose one
	rnd = random();
	step = 1.0f / numuncap; // beware of division by zero
	for (i = 1; i <= numuncap; i++)
		if (rnd < step * i) {
			memcpy(&bs->teamgoal, &(balloongoal[uncaptured[i - 1]]), sizeof(bot_goal_t));
			break;
		}

	return qtrue;
}

qboolean BotPickBestBalloonGoal(bot_state_t *bs) {
	int i, j;
	int index;
	int state;
	int capstate[MAX_BALLOONS];
	int numcap, numnmycap, numuncap;
	float weight; // 0 - attack, 100 - defend

	float tt, wtt, multiplier;
	float bestdist = 9999.9f;
	int bestgoal = -1;

	// get status of balloons
	numcap = numnmycap = numuncap = 0;

	for (i = 0; i < level.numBalloons; i++) {
		index = g_entities[balloongoal[i].entitynum].count;
		state = level.balloonState[index]; // status of goal i
		if ((BotTeam(bs) == TEAM_RED && state == '1') || (BotTeam(bs) == TEAM_BLUE && state == '2')) {
			// own balloon
			capstate[i] = 0;
			numcap++;
		} else if ((BotTeam(bs) == TEAM_RED && state == '2') || (BotTeam(bs) == TEAM_BLUE && state == '1')) {
			// nmy balloon
			capstate[i] = 1;
			numnmycap++;
		} else {
			// uncap balloon
			capstate[i] = 2;
			numuncap++;
		}
	}

	// go for uncapped loons as long as there are such
	if (numuncap)
		return BotPickUnCappedBalloon(bs);

	// weight gets 0 if nmy controls and 1 if the bots team controls
	weight = ((numcap - numnmycap) + level.numBalloons) / (2.0 * level.numBalloons);

	if (weight == 0)
		weight = 0.1f;
	if (weight == 1)
		weight = 0.9f;

	scorealert[BotTeam(bs)] = weight; // should also depend on scorediff and caplimit

	// find best goal
	for (j = 0; j < level.numBalloons; j++) {
		tt = BotClientTravelTimeToGoal(bs->client, &balloongoal[j]);
		if (!tt)
			continue;

		// prefer balloons based on current balloon-difference
		if (!capstate[j]) { // our balloon
			multiplier = 1.0f - weight;
		} else {
			if (capstate[j] == 1) // nmy balloon
				multiplier = weight;
			else // uncap balloon
				multiplier = (1.0f - weight) / 2.0f;
		}
		wtt = tt * multiplier * multiplier;

		// G_Printf("%f .. %d -> %f -> %f , %d (%f)\n", weight, i, tt, wtt, capstate[j], multiplier);
		// BotAddInfo(bs, va("ball%d %d -> %.1f", j, capstate[j], wtt), AIDBG_GAMETYPE);

		if (wtt < bestdist) {
			bestdist = wtt;
			bestgoal = j;
		}
	}

	if (bestgoal == -1) // airborne? dont make decision this frame
		return qfalse;

	// store best goal
	memcpy(&bs->teamgoal, &balloongoal[bestgoal], sizeof(bot_goal_t));
	return qtrue;
}

static void BotBalloonSeekGoals(bot_state_t *bs) {
	vec3_t dir;

	// dont go for balloon as long as bad equiped
	if (bs->ltgtype != LTG_BALLCAMP && (bs->inventory[INV_GOTWEAPON] < 40 || bs->inventory[INVENTORY_HEALTH] < 30)) {
		bs->ltgtype = 0;
		return;
	}

	if (bs->ltgtype == 0) {
		// compare balloon goal and choose one
		if (!BotPickBestBalloonGoal(bs))
			return; // invalid traveltymes, probably airborne

		bs->decisionmaker = bs->client;
		bs->teamgoal_time = FloatTime() + TEAM_BALLOONTHINK_TIME;

		if (IsMyBalloon(BotTeam(bs), &bs->teamgoal))
			bs->ltgtype = LTG_DEFENDKEYAREA;
		else
			bs->ltgtype = LTG_ATTACKENEMYBASE;
	}

	// if attacking a balloon and near enough to camp: do it
	if (bs->ltgtype == LTG_ATTACKENEMYBASE) {
		if (IsMyBalloon(BotTeam(bs), &bs->teamgoal)) {
			bs->ltgtype = LTG_DEFENDKEYAREA;
		} else {
			VectorSubtract(bs->teamgoal.origin, bs->origin, dir);
			if (!IsMyBalloon(BotTeam(bs), &bs->teamgoal) && VectorLengthSquared(dir) < Square(75)) {
				// BotRefuseOrder(bs);
				bs->ltgtype = LTG_BALLCAMP; // stay here to capture
				bs->decisionmaker = bs->client;
				// bs->ordered = qfalse;
				bs->teamgoal_time = FloatTime() + TEAM_BALLOONCAMP_TIME;
			}
		}
	}
}

static qboolean PlayerRescuesPlayer(int helper_id, int frozen_id) {
	gentity_t *helper = &g_entities[helper_id];
	bot_state_t *bs;

	if (!(helper->r.svFlags & SVF_BOT))
		return qfalse;

	if (FT_PlayerIsFrozen(helper))
		return qfalse;

	bs = botstates[helper_id];
	if (bs->ltgtype != LTG_UNFREEZE)
		return qfalse;

	return (bs->teammate == frozen_id);
}

static void BotFreezeTagSeekGoals(bot_state_t *bs) {
	int mates[MAX_CLIENTS], frozenmates[MAX_CLIENTS];
	int nummates, numfrozen = 0;
	int i, j, pick;
	int matearea;
	qboolean getting_rescued;
	char buf[128];

	// no freeze -> no team goals
	if (!G_FreezeTag())
		return;

	if (bs->ltgtype == LTG_UNFREEZE)
		return;

	if (bs->freezecheck_time > level.time)
		return;

	bs->freezecheck_time = level.time + 1000 + 2000 * random();

	nummates = BotGetTeammates(bs, mates, sizeof(mates));
	// filter the frozen mates
	for (i = 0; i < nummates; i++) {
		if (mates[i] == bs->client)
			continue;
		if (!FT_PlayerIsFrozen(&g_entities[mates[i]]))
			continue;

		// see if the mate is getting rescued already. if so, ignore him
		getting_rescued = qfalse;
		for (j = 0; j < nummates; j++) {
			if (mates[j] == bs->client)
				continue;
			if (PlayerRescuesPlayer(mates[j], mates[i])) {
				getting_rescued = qtrue;
				break;
			}
		}
		if (getting_rescued)
			continue;

		// can the mate be reached by bots?
		matearea = trap_AAS_PointAreaNum(g_entities[mates[i]].r.currentOrigin);
		if (!matearea)
			continue;

		frozenmates[numfrozen++] = mates[i];
	}

	if (!numfrozen) {
		return;
	}

	pick = random() * numfrozen;
	if (pick == numfrozen)
		pick--;

	bs->ltgtype = LTG_UNFREEZE;
	bs->teammate = frozenmates[pick];
	bs->teamgoal_time = FloatTime() + TEAM_UNFREEZETIME;

	// tell the mate you are on the way
	trap_EA_Command(bs->client, va("tell %d Keep cool %s, i'm on my way!\n", bs->teammate,
								   EasyClientName(bs->teammate, buf, sizeof(buf))));
}

qboolean BotWantsCarts(bot_state_t *bs, int *mate) {
	int i, ccandidates, cand[MAX_CLIENTS];

	// check mates for carts periodically (1sec)
	if (bs->checkcarttime > FloatTime())
		return qfalse;
	bs->checkcarttime = FloatTime() + 1;

	// if self is low, return
	if (bs->inventory[INVENTORY_HEALTH] + bs->inventory[INVENTORY_ARMOR] < 30)
		return qfalse;

	ccandidates = BotGetVisTeamPlayers(bs, cand, sizeof(cand), qtrue);
	for (i = 0; i < ccandidates; i++) {
		int numcarts;
		gentity_t *ent; // mate candidate
		vec3_t vectele;
		vec3_t vecself;
		float teledist, selfdist;

		ent = &g_entities[cand[i]];

		// skip humans
		if (!(ent->r.svFlags & SVF_BOT))
			continue;
		/*		{
					*mate = ent->s.number;
					if(!BotGetNumClientCarts(bs, *mate)) continue;
					return qtrue;
				}	// cyr_ptr
		*/

		// skip players without carts
		numcarts = BotGetNumClientCarts(bs, cand[i]);
		if (!numcarts)
			continue;

		// mate extreme low and self is moderate healthy (regardless of PU, weapon)
		if (ent->health + ent->client->ps.stats[STAT_ARMOR] < 15 &&
			bs->inventory[INVENTORY_HEALTH] + bs->inventory[INVENTORY_ARMOR] > 30) {
			// check distances
			VectorSubtract(ent->r.currentOrigin, spraytele.origin, vectele);
			teledist = VectorLength(vectele);
			VectorSubtract(ent->r.currentOrigin, bs->origin, vecself);
			selfdist = VectorLength(vecself);
			if (teledist < 200)
				continue;
			if (teledist < selfdist)
				continue;
			// ok, take it
			// SpecMe(bs->client);
			*mate = cand[i];
			return qtrue;
		}
		// if mate is not that low, take into account also his powerups
		if (!ent->client->ps.powerups[PW_PADPOWER] && !ent->client->ps.powerups[PW_SPEEDY] &&
			ent->health + ent->client->ps.stats[STAT_ARMOR] < 30 &&
			bs->inventory[INVENTORY_HEALTH] + bs->inventory[INVENTORY_ARMOR] > 60) {
			// check distances
			VectorSubtract(ent->r.currentOrigin, spraytele.origin, vectele);
			teledist = VectorLength(vectele);
			VectorSubtract(ent->r.currentOrigin, bs->origin, vecself);
			selfdist = VectorLength(vecself);
			if (teledist < 200)
				continue;
			if (teledist < selfdist)
				continue;
			// ok, take it
			// SpecMe(bs->client);
			*mate = cand[i];
			return qtrue;
		}
	}
	return qfalse;
}

void BotSyCSeekGoals(bot_state_t *bs) {

	if (ClientInSprayroom(bs->client)) {
		// got ammo and not rushing ?
		if (bs->inventory[INVENTORY_SPRAYPISTOLAMMO] && bs->ltgtype != LTG_RUSHBASE) {
			// BotRefuseOrder(bs);
			bs->ltgtype = LTG_RUSHBASE;
			bs->teamgoal_time = FloatTime() + RUSHBASE_TIME;
			bs->rushbaseaway_time = 0;
			bs->decisionmaker = bs->client;
			// bs->ordered = qfalse;
			bs->which_wall = BotChooseWall(bs);
		}
		// no more ammo and not leaving ?
		else if (!bs->inventory[INVENTORY_SPRAYPISTOLAMMO] && bs->ltgtype != LTG_ATTACKENEMYBASE) {
			// BotRefuseOrder(bs);
			bs->ltgtype = LTG_ATTACKENEMYBASE;
			bs->teamgoal_time = FloatTime() + RUSHBASE_TIME;
			bs->decisionmaker = bs->client;
			// bs->ordered = qfalse;
		}
		return; //
	}

	// outside the sprayroom
	if (bs->ltgtype != LTG_GIVECART && bs->ltgtype != LTG_FETCHCART) {
		int mate = 0;
		// feeling good and want to take others carts ?
		if (BotWantsCarts(bs, &mate)) {
			char buf[MAX_MESSAGE_SIZE];
			// BotRefuseOrder(bs);
			bs->ltgtype = LTG_FETCHCART;
			bs->teammate = mate;
			bs->teamgoal_time = FloatTime() + SYC_CART_EXCHANGE_TIME;
			bs->decisionmaker = bs->client;
			// bs->ordered = qfalse;
			bs->takecart = qfalse;
			// send chat command to mate
			BotAI_BotInitialChat(bs, "fetchcart_start", buf, NULL);
			trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL);
			return;
		} else if (bs->inventory[INVENTORY_SPRAYPISTOLAMMO] && bs->ltgtype != LTG_RUSHBASE) {
			// BotRefuseOrder(bs);
			bs->ltgtype = LTG_RUSHBASE;
			bs->teamgoal_time = FloatTime() + RUSHBASE_TIME;
			bs->rushbaseaway_time = 0;
			bs->decisionmaker = bs->client;
			// bs->ordered = qfalse;
			bs->which_wall = BotChooseWall(bs);
			return;
		}
	}
}

void BotSyCRetreatGoals(bot_state_t *bs) {
	// outside the sprayroom
	if (bs->ltgtype != LTG_GIVECART && bs->ltgtype != LTG_FETCHCART) {
		int mate = 0;
		// feeling good and want to take others carts ?
		if (BotWantsCarts(bs, &mate)) {
			char buf[MAX_MESSAGE_SIZE];
			// BotRefuseOrder(bs);
			bs->ltgtype = LTG_FETCHCART;
			bs->teammate = mate;
			bs->teamgoal_time = FloatTime() + SYC_CART_EXCHANGE_TIME;
			bs->decisionmaker = bs->client;
			// bs->ordered = qfalse;
			bs->takecart = qfalse;
			// send chat command to mate
			BotAI_BotInitialChat(bs, "fetchcart_start", buf, NULL);
			trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL);
			return;
		} else if (bs->inventory[INVENTORY_SPRAYPISTOLAMMO] && bs->ltgtype != LTG_RUSHBASE) {
			// BotRefuseOrder(bs);
			bs->ltgtype = LTG_RUSHBASE;
			bs->teamgoal_time = FloatTime() + RUSHBASE_TIME;
			bs->rushbaseaway_time = 0;
			bs->decisionmaker = bs->client;
			// bs->ordered = qfalse;
			bs->which_wall = BotChooseWall(bs);
			return;
		}
	}
}

// returns which "number" in the team is this bot (will be used for some fake teamplay ;P)
int BotNumberInTeam(bot_state_t *bs) {
	static int maxclients = 0;
	int i;
	int r = 0;

	if (!maxclients)
		maxclients = trap_Cvar_VariableIntegerValue("sv_maxclients");

	for (i = 0; i < maxclients && i < MAX_CLIENTS; ++i) {

		if (i == bs->client)
			break;

		if (BotSameTeam(
				bs,
				i)) // FIXME: that call maybe a bit to heavy ... and probably also return true for none-bot teammates
			++r;
	}

	return r;
}

int GetTeamFlagCarrier(int team) {
	int i;
	int pu = (team == TEAM_RED) ? PW_REDFLAG : PW_BLUEFLAG;

	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		// valid client?
		if (!g_entities[i].inuse || !g_entities[i].client)
			continue;

		// carrying the flag?
		if (g_entities[i].client->ps.powerups[pu])
			return i;
	}
	return -1;
}

qboolean PickBoomieGoal(bot_state_t *bs) {
	int i;
	team_t botTeam = BotTeam(bs);
	static const int TT_MAX = 999999;
	int tt, best_tt = TT_MAX;
	int best_id = 0;

	for (i = 0; i < numboomiespots; i++) {
		if (boomiespots[i].team != botTeam || boomiespots[i].occupied)
			continue;
		tt = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, boomiespots[i].goal.areanum, bs->tfl);
		if (tt && tt < best_tt) {
			best_tt = tt;
			best_id = i;
		}
	}

	if (best_tt == TT_MAX)
		return qfalse;

	memcpy(&bs->teamgoal, &boomiespots[best_id].goal, sizeof(bot_goal_t));
	bs->teammate = best_id;
	return qtrue;
}

qboolean PickBambamGoal(bot_state_t *bs) {
	int i;
	team_t botTeam = BotTeam(bs);
	static const int TT_MAX = 999999;
	int tt, best_tt = TT_MAX;
	int best_id = 0;

	for (i = 0; i < numbambamspots; i++) {
		if (bambamspots[i].team != botTeam || bambamspots[i].occupied)
			continue;

		tt = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, bambamspots[i].goal.areanum, bs->tfl);
		if (tt && tt < best_tt) {
			best_id = i;
			best_tt = tt;
		}
	}

	if (best_tt == TT_MAX)
		return qfalse;

	memcpy(&bs->teamgoal, &bambamspots[best_id].goal, sizeof(bot_goal_t));
	bs->teammate = best_id;
	return qtrue;
}

void BotCtfSeekGoals(bot_state_t *bs) {
	int flagstatus, nmyflagstatus;
	int action = 0;
	int carrier;

	if (bs->ltgtype == LTG_JOINMATE)
		return;

	flagstatus = Team_GetFlagStatus(BotTeam(bs));
	nmyflagstatus = Team_GetFlagStatus(BotOppositeTeam(bs));

	// carrying the flag? rush home!
	if (BotCTFCarryingFlag(bs)) {
		// try to get a health-goal if we are really low
		if (bs->inventory[INVENTORY_HEALTH] < 30) {
			bs->ltgtype = 0;
			return;
		}

		// special case: bot is alone
		if (0 == TeamCount(bs->client, BotTeam(bs))) {
			// if our flag is gone, go get it
			if (flagstatus == FLAG_DROPPED) {
				// pick it up
				if (bs->ltgtype != LTG_PICKUPFLAG) {
					if (GetDroppedLollyGoal(BotTeam(bs), &bs->teamgoal)) {
						bs->decisionmaker = bs->client;
						bs->ltgtype = LTG_PICKUPFLAG;
						bs->teamgoal_time = FloatTime() + CTF_RUSHBASE_TIME;
					} else { // cant get the dropped flag, go home
						if (bs->ltgtype != LTG_CAPTUREFLAG) {
							bs->ltgtype = LTG_CAPTUREFLAG;
							bs->teamgoal_time = FloatTime() + CTF_RUSHBASE_TIME;
							bs->rushbaseaway_time = 0;
							bs->decisionmaker = bs->client;
						}
					}
				}
				return;
			} else if (flagstatus == FLAG_TAKEN) {
				// roam around so you get a chance to encounter the enemy
				bs->ltgtype = 0;
				return;
			}
			// flag is at base, all good
		}

		if (bs->ltgtype != LTG_CAPTUREFLAG) {
			bs->ltgtype = LTG_CAPTUREFLAG;
			bs->teamgoal_time = FloatTime() + CTF_RUSHBASE_TIME;
			bs->rushbaseaway_time = 0;
			bs->decisionmaker = bs->client;
		}
		return;
	}

	/*
	if( bs->cur_ps.stats[STAT_HOLDABLE_ITEM] == MODELINDEX_BAMBAM )
	{
		if( bs->ltgtype == LTG_PLANTBAMBAM )
			return;
		else
		{
			// plant bambam
			if( PickBambamGoal(bs) )
			{
				bs->decisionmaker = bs->client;
				bs->ltgtype = LTG_PLANTBAMBAM;
				bs->teamgoal_time = FloatTime() + 120;
				return;
			}
		}
		return;
	}

	if( bs->cur_ps.stats[STAT_HOLDABLE_ITEM] == MODELINDEX_BOOMIE )
	{
		if(bs->ltgtype == LTG_PLANTBOOMIE )
			return;
		else
		{
			// plant boomie
			if( PickBoomieGoal(bs) )
			{
				bs->decisionmaker = bs->client;
				bs->ltgtype = LTG_PLANTBOOMIE;
				bs->teamgoal_time = FloatTime() + 120;
				return;
			}
			// no goal, choose another ltgtype
		}
	}
	*/

	// dont go for ctl-goals as long as bad equiped (though don't interrupt important tasks)
	if (bs->ltgtype != LTG_PICKUPFLAG && // LTG_CAPTUREFLAG is covered above
		(bs->inventory[INV_GOTWEAPON] < 40 || bs->inventory[INVENTORY_HEALTH] < 30)) {
		bs->ltgtype = 0;
		return;
	}

	if (BotNumberInTeam(bs) & 1) { // the bots 1,3,5,... will be defensive
		switch (flagstatus) {
		case FLAG_TAKEN:
			if (nmyflagstatus == FLAG_TAKEN)
				action = LTG_TEAMACCOMPANY;
			else
				action = LTG_GETFLAG;
			break;
		case FLAG_DROPPED:
			action = LTG_PICKUPFLAG;
			break;
		default: // FLAG_ATBASE
			action = LTG_DEFENDKEYAREA;
		}
	} else {
		// the enemyflag was taken ... and we have our flag ... defend it!
		if (nmyflagstatus == FLAG_TAKEN) {
			if (flagstatus == FLAG_DROPPED)
				action = LTG_PICKUPFLAG;
			else
				action = LTG_TEAMACCOMPANY;
		} else
			action = LTG_GETFLAG;
	}

	// assign LTG, unless the bot is already going for it
	if (action == LTG_DEFENDKEYAREA && bs->ltgtype != LTG_DEFENDKEYAREA) {
		// defend
		if (BotTeam(bs) == TEAM_RED)
			memcpy(&bs->teamgoal, &ctf_redflag, sizeof(bot_goal_t));
		else
			memcpy(&bs->teamgoal, &ctf_blueflag, sizeof(bot_goal_t));

		bs->ltgtype = LTG_DEFENDKEYAREA;
		bs->decisionmaker = bs->client;
		bs->teamgoal_time = FloatTime() + TEAM_DEFENDKEYAREA_TIME;
		bs->defendaway_time = 0;
	} else if (action == LTG_GETFLAG && bs->ltgtype != LTG_GETFLAG) {
		if (nmyflagstatus == FLAG_DROPPED) {
			// get a goal for the flag
			if (!GetCTLFlagGoal(BotOppositeTeam(bs), &bs->teamgoal)) {
				bs->ltgtype = 0;
				return;
			}
		}
		// attack enemy flag/base
		bs->decisionmaker = bs->client;
		bs->ltgtype = LTG_GETFLAG;
		// set the time the bot will stop getting the flag
		bs->teamgoal_time = FloatTime() + CTF_RUSHBASE_TIME;
	} else if (action == LTG_PICKUPFLAG && bs->ltgtype != LTG_PICKUPFLAG) {
		// get the goal
		if (!GetDroppedLollyGoal(BotTeam(bs), &bs->teamgoal))
			return;
		bs->decisionmaker = bs->client;
		bs->ltgtype = LTG_PICKUPFLAG;
		bs->teamgoal_time = FloatTime() + CTF_RUSHBASE_TIME;
	} else if (action == LTG_TEAMACCOMPANY && bs->ltgtype != LTG_TEAMACCOMPANY) {
		carrier = GetTeamFlagCarrier(BotTeam(bs));
		if (carrier == -1) // should never happen
			return;
		bs->decisionmaker = bs->client;
		bs->teammate = carrier;
		bs->teammatevisible_time = FloatTime();
		bs->teammessage_time = 0;
		bs->arrive_time = 0;
		bs->teamgoal_time = FloatTime() + TEAM_ACCOMPANY_TIME;
		bs->ltgtype = LTG_TEAMACCOMPANY;
		bs->formation_dist = 3.5 * 32; // 3.5 meter
	}
}

qboolean GetDroppedLollyGoal(int team, bot_goal_t *goal) {
	char *itemname = (team == TEAM_RED) ? "red Lolly" : "blue Lolly";

	if (trap_BotGetLevelItemGoal(-1, itemname, goal) < 0)
		return qfalse;

	// goal unreachable?
	if (!goal->areanum || !trap_AAS_AreaReachability(goal->areanum))
		return qfalse;

	return qtrue;
}

/*
==================
BotTeamGoals
==================
*/
void BotTeamGoals(bot_state_t *bs, int retreat) {

	if (gametype == GT_CTF) {
		BotCtfSeekGoals(bs);
	} else if (gametype == GT_BALLOON) {
		BotBalloonSeekGoals(bs);
	} else if (gametype == GT_FREEZETAG) {
		BotFreezeTagSeekGoals(bs);
	} else if (gametype == GT_SPRAY || gametype == GT_SPRAYFFA) {
		if (retreat) {
			BotSyCRetreatGoals(bs);
		} else {
			BotSyCSeekGoals(bs);
		}
	}
}

/*
==================
BotPointAreaNum
==================
*/
int BotPointAreaNum(vec3_t origin) {
	int areanum, numareas, areas[10];
	vec3_t end;

	areanum = trap_AAS_PointAreaNum(origin);
	if (areanum)
		return areanum;
	VectorCopy(origin, end);
	end[2] += 10;
	numareas = trap_AAS_TraceAreas(origin, end, areas, NULL, 10);
	if (numareas > 0)
		return areas[0];
	return 0;
}

/*
==================
ClientName
==================
*/
char *ClientName(int client, char *name, int size) {
	char buf[MAX_INFO_STRING];

	if (client < 0 || client >= MAX_CLIENTS) {
		BotAI_Print(PRT_ERROR, "ClientName: client out of range\n");
		return "[client out of range]";
	}
	trap_GetConfigstring(CS_PLAYERS + client, buf, sizeof(buf));
	strncpy(name, Info_ValueForKey(buf, "n"), size - 1);
	name[size - 1] = '\0';
	Q_CleanStr(name);
	return name;
}

/*
==================
ClientSkin
==================
*/
char *ClientSkin(int client, char *skin, int size) {
	char buf[MAX_INFO_STRING];

	if (client < 0 || client >= MAX_CLIENTS) {
		BotAI_Print(PRT_ERROR, "ClientSkin: client out of range\n");
		return "[client out of range]";
	}
	trap_GetConfigstring(CS_PLAYERS + client, buf, sizeof(buf));
	strncpy(skin, Info_ValueForKey(buf, "model"), size - 1);
	skin[size - 1] = '\0';
	return skin;
}

/*
==================
ClientFromName
==================
*/
int ClientFromName(char *name) {
	int i;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if (!maxclients)
		maxclients = trap_Cvar_VariableIntegerValue("sv_maxclients");
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
		Q_CleanStr(buf);
		if (!Q_stricmp(Info_ValueForKey(buf, "n"), name))
			return i;
	}
	return -1;
}

/*
==================
ClientOnSameTeamFromName
==================
*/
int ClientOnSameTeamFromName(bot_state_t *bs, char *name) {
	int i;
	char buf[MAX_INFO_STRING];

	for (i = 0; i < level.maxclients; i++) {
		if (!BotSameTeam(bs, i))
			continue;
		trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
		Q_CleanStr(buf);
		if (!Q_stricmp(Info_ValueForKey(buf, "n"), name))
			return i;
	}
	return -1;
}

/*
==================
stristr
==================
*/
char *stristr(char *str, char *charset) {
	int i;

	while (*str) {
		for (i = 0; charset[i] && str[i]; i++) {
			if (toupper(charset[i]) != toupper(str[i]))
				break;
		}
		if (!charset[i])
			return str;
		str++;
	}
	return NULL;
}

/*
==================
EasyClientName
==================
*/
char *EasyClientName(int client, char *buf, int size) {
	int i;
	char *str1, *str2, *ptr, c;
	char name[128] = {0};

	ClientName(client, name, sizeof(name));

	for (i = 0; name[i]; i++)
		name[i] &= 127;
	// remove all spaces
	for (ptr = strstr(name, " "); ptr; ptr = strstr(name, " ")) {
		memmove(ptr, ptr + 1, strlen(ptr + 1) + 1);
	}
	// check for [x] and ]x[ clan names
	str1 = strstr(name, "[");
	str2 = strstr(name, "]");
	if (str1 && str2) {
		if (str2 > str1)
			memmove(str1, str2 + 1, strlen(str2 + 1) + 1);
		else
			memmove(str2, str1 + 1, strlen(str1 + 1) + 1);
	}
	// remove Mr prefix
	if ((name[0] == 'm' || name[0] == 'M') && (name[1] == 'r' || name[1] == 'R')) {
		memmove(name, name + 2, strlen(name + 2) + 1);
	}
	// only allow lower case alphabet characters
	ptr = name;
	while (*ptr) {
		c = *ptr;
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
			ptr++;
		} else if (c >= 'A' && c <= 'Z') {
			*ptr += 'a' - 'A';
			ptr++;
		} else {
			memmove(ptr, ptr + 1, strlen(ptr + 1) + 1);
		}
	}
	strncpy(buf, name, size - 1);
	buf[size - 1] = '\0';
	return buf;
}

/*
==================
BotChooseWeapon
==================
*/
void BotChooseWeapon(bot_state_t *bs) {
	int offset = (INVENTORY_PUNCHY - WP_PUNCHY);

	// enforce punchy in berserker mode
	if (bs->cur_ps.powerups[PW_BERSERKER]) {
		bs->weaponnum = WP_PUNCHY;
		trap_EA_SelectWeapon(bs->client, bs->weaponnum);
		return;
	}

	if ((bs->cur_ps.weaponstate == WEAPON_RAISING) || (bs->cur_ps.weaponstate == WEAPON_DROPPING)) {
		// check if the weapon is still in inventory
		if (bs->inventory[(bs->weaponnum + offset)]) {
			trap_EA_SelectWeapon(bs->client, bs->weaponnum);
		} else {
			// G_Printf("^4 lost weapon during change \n");
			bs->cur_ps.weaponstate = WEAPON_READY; // hm
			BotChooseWeapon(bs);
			return;
		}
	} else {
		int newweaponnum = trap_BotChooseBestFightWeapon(bs->ws, bs->inventory);

		// Now fix special cases the engine does not know about

		if ((GT_SPRAYFFA == gametype) || (GT_SPRAY == gametype)) {
			if ((bs->cur_ps.stats[STAT_SPRAYROOMSECS] > 0) && (newweaponnum != WP_SPRAYPISTOL)) {
				newweaponnum = WP_SPRAYPISTOL;
			} else if (WP_SPRAYPISTOL == newweaponnum) {
				newweaponnum = WP_PUNCHY; // This should be a save default
			}
		}

		if (g_modInstagib.integer && (newweaponnum != WP_SPRAYPISTOL)) {
			newweaponnum = Instagib_getSpawnWeapon();
			bs->weaponnum = newweaponnum;
			trap_EA_SelectWeapon(bs->client, bs->weaponnum);
			return;
		}

		if (!bs->inventory[newweaponnum + offset]) {
			return;
		}

		if (bs->weaponnum != newweaponnum) {
			bs->weaponchange_time = FloatTime();
		}
		bs->weaponnum = newweaponnum;
		// G_Printf( "bs->weaponnum = %d\n", bs->weaponnum );

		trap_EA_SelectWeapon(bs->client, bs->weaponnum);
	}
}

/*
==================
BotSetupForMovement
==================
*/
void BotSetupForMovement(bot_state_t *bs) {
	bot_initmove_t initmove;

	memset(&initmove, 0, sizeof(bot_initmove_t));
	VectorCopy(bs->cur_ps.origin, initmove.origin);
	VectorCopy(bs->cur_ps.velocity, initmove.velocity);
	VectorClear(initmove.viewoffset);
	initmove.viewoffset[2] += bs->cur_ps.viewheight;
	initmove.entitynum = bs->entitynum;
	initmove.client = bs->client;
	initmove.thinktime = bs->thinktime;
	// set the onground flag
	if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE)
		initmove.or_moveflags |= MFL_ONGROUND;
	// set the teleported flag
	if ((bs->cur_ps.pm_flags & PMF_TIME_KNOCKBACK) && (bs->cur_ps.pm_time > 0)) {
		initmove.or_moveflags |= MFL_TELEPORTED;
	}
	// set the waterjump flag
	if ((bs->cur_ps.pm_flags & PMF_TIME_WATERJUMP) && (bs->cur_ps.pm_time > 0)) {
		initmove.or_moveflags |= MFL_WATERJUMP;
	}
	// set presence type
	if (bs->cur_ps.pm_flags & PMF_DUCKED)
		initmove.presencetype = PRESENCE_CROUCH;
	else
		initmove.presencetype = PRESENCE_NORMAL;
	//
	if (bs->walker > 0.5)
		initmove.or_moveflags |= MFL_WALK;
	//
	VectorCopy(bs->viewangles, initmove.viewangles);
	//
	trap_BotInitMoveState(bs->ms, &initmove);
}

/*
==================
BotUpdateInventory
==================
*/
int BotCalcWeapEquip(bot_state_t *bs, int weapid, int ammoid, int snapammo) {
	int ammo;

	if (bs->inventory[weapid]) {
		// ignore ammo above the pickup amount
		if (bs->inventory[ammoid] > snapammo)
			ammo = snapammo;
		else
			ammo = bs->inventory[ammoid];
		// [0..100]
		return (100 * (float)(ammo / snapammo));
	}
	// no weap, no equip
	return 0;
}

void BotUpdateInventory(bot_state_t *bs) {
	int oldinventory[MAX_ITEMS];
	int sum = 0;

	memcpy(oldinventory, bs->inventory, sizeof(oldinventory));
	// armor
	bs->inventory[INVENTORY_ARMOR] = bs->cur_ps.stats[STAT_ARMOR];
	// weapons
	bs->inventory[INVENTORY_PUNCHY] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_PUNCHY)) != 0;
	bs->inventory[INVENTORY_NIPPER] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_NIPPER)) != 0;
	bs->inventory[INVENTORY_PUMPER] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_PUMPER)) != 0;
	bs->inventory[INVENTORY_BALLOONY] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_BALLOONY)) != 0;
	bs->inventory[INVENTORY_BETTY] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_BETTY)) != 0;
	bs->inventory[INVENTORY_BOASTER] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_BOASTER)) != 0;
	bs->inventory[INVENTORY_SPLASHER] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_SPLASHER)) != 0;
	bs->inventory[INVENTORY_BUBBLEG] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_BUBBLEG)) != 0;
	bs->inventory[INVENTORY_IMPERIUS] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_IMPERIUS)) != 0;
	bs->inventory[INVENTORY_SPRAYPISTOL] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_SPRAYPISTOL)) != 0;
	bs->inventory[INVENTORY_KMA] = (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_KMA97)) != 0;
	// ammo
	bs->inventory[INVENTORY_NIPPERAMMO] = bs->cur_ps.ammo[WP_NIPPER];
	bs->inventory[INVENTORY_PUMPERAMMO] = bs->cur_ps.ammo[WP_PUMPER];
	bs->inventory[INVENTORY_BALLOONYAMMO] = bs->cur_ps.ammo[WP_BALLOONY];
	bs->inventory[INVENTORY_BUBBLEGAMMO] = bs->cur_ps.ammo[WP_BUBBLEG];
	bs->inventory[INVENTORY_BOASTERAMMO] = bs->cur_ps.ammo[WP_BOASTER];
	bs->inventory[INVENTORY_BETTYAMMO] = bs->cur_ps.ammo[WP_BETTY];
	bs->inventory[INVENTORY_SPLASHERAMMO] = bs->cur_ps.ammo[WP_SPLASHER];
	bs->inventory[INVENTORY_IMPERIUSAMMO] = bs->cur_ps.ammo[WP_IMPERIUS];
	bs->inventory[INVENTORY_SPRAYPISTOLAMMO] = bs->cur_ps.ammo[WP_SPRAYPISTOL];
	// holdables
	bs->inventory[INVENTORY_HEALTH] = bs->cur_ps.stats[STAT_HEALTH];
	bs->inventory[INVENTORY_FLOATER] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM] == MODELINDEX_FLOATER;
	bs->inventory[INVENTORY_KILLERDUCKS] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM] == MODELINDEX_KILLERDUCKS;
	bs->inventory[INVENTORY_BAMBAM] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM] == MODELINDEX_BAMBAM;
	bs->inventory[INVENTORY_BOOMIE] = bs->cur_ps.stats[STAT_HOLDABLE_ITEM] == MODELINDEX_BOOMIE;
	// powerups
	bs->inventory[INVENTORY_PADPOWER] = bs->cur_ps.powerups[PW_PADPOWER] != 0;
	bs->inventory[INVENTORY_CLIMBER] = bs->cur_ps.powerups[PW_CLIMBER] != 0;
	bs->inventory[INVENTORY_SPEEDY] = bs->cur_ps.powerups[PW_SPEEDY] != 0;
	bs->inventory[INVENTORY_JUMPER] = bs->cur_ps.powerups[PW_JUMPER] != 0;
	bs->inventory[INVENTORY_VISIONLESS] = bs->cur_ps.powerups[PW_VISIONLESS] != 0;
	bs->inventory[INVENTORY_REVIVAL] = bs->cur_ps.powerups[PW_REVIVAL] != 0;

	bs->inventory[INVENTORY_REDFLAG] = bs->cur_ps.powerups[PW_REDFLAG] != 0;
	bs->inventory[INVENTORY_BLUEFLAG] = bs->cur_ps.powerups[PW_BLUEFLAG] != 0;

	// cyr
	bs->inventory[INVENTORY_CLIENTNUM] = bs->client; // cyr
	sum += BotCalcWeapEquip(bs, INVENTORY_PUMPER, INVENTORY_PUMPERAMMO, 10);
	sum += BotCalcWeapEquip(bs, INVENTORY_BALLOONY, INVENTORY_BALLOONYAMMO, 10);
	sum += BotCalcWeapEquip(bs, INVENTORY_BETTY, INVENTORY_BETTYAMMO, 10);
	sum += BotCalcWeapEquip(bs, INVENTORY_BOASTER, INVENTORY_BOASTERAMMO, 50);
	sum += BotCalcWeapEquip(bs, INVENTORY_SPLASHER, INVENTORY_SPLASHERAMMO, 10);
	sum += BotCalcWeapEquip(bs, INVENTORY_BUBBLEG, INVENTORY_BUBBLEGAMMO, 50);
	sum += BotCalcWeapEquip(bs, INVENTORY_IMPERIUS, INVENTORY_IMPERIUSAMMO, 3);
	if (bs->inventory[INVENTORY_KMA])
		sum += 100;
	bs->inventory[INV_GOTWEAPON] = sum;
}

/*
==================
BotUpdateBattleInventory
==================
*/
void BotUpdateBattleInventory(bot_state_t *bs, int enemy) {
	vec3_t dir;
	aas_entityinfo_t entinfo;

	BotEntityInfo(enemy, &entinfo);
	if (!entinfo.valid)
		return;
	VectorSubtract(entinfo.origin, bs->origin, dir);
	bs->inventory[ENEMY_HEIGHT] = (int)dir[2];
	// dir[2] = 0;
	bs->inventory[ENEMY_DIST] = (int)VectorLength(dir);
	// FIXME: add num visible enemies and num visible team mates to the inventory
}

#define KILLER_DIST 350
// cyr{
void BotUseKillerducks(bot_state_t *bs) {
	int enemies, teammates;

	// abuse: inv_time wird bei jedem check und USE gesetzt
	if (bs->duckuse_time > FloatTime())
		return;
	bs->duckuse_time = FloatTime() + 1;

	if (bs->inventory[INVENTORY_KILLERDUCKS] <= 0)
		return;

	BotVisibleTeamMatesAndEnemies(bs, &teammates, &enemies, KILLER_DIST);
	if (!teammates && enemies) {
		trap_EA_Use(bs->client);
		bs->duckuse_time = FloatTime() + 2;
	}
}
// cyr}

/*
==================
BotBattleUseItems
==================
*/
void BotBattleUseItems(bot_state_t *bs) {
	bsp_trace_t bsptrace;
	vec3_t end, mins = {-15, -15, -2}, maxs = {15, 15, 2};
	float above;
	float rnd;
	aas_entityinfo_t entinfo, enemyinfo;
	vec3_t botvelocity, up, forward;
	qboolean falling;

	BotUseKillerducks(bs); // cyr

	// if enemy has splasher, return	// floaters are easy targets

	// if room(area) ceiling - botheight < 200 return	// keep dist from ceil
	// if botheight - nmy > 300	 return			// dont get to far from nmy

	// if bot has splash weapon: use floater
	// if nmy has splash weapon: use floater

	if (bs->inventory[INVENTORY_FLOATER] <= 0)
		return;

	if (bs->inventory[ENEMY_DIST] > 400)
		return;

	// how many units is the bot above the enemy?
	above = 0 - bs->inventory[ENEMY_HEIGHT];

	VectorCopy(bs->origin, end);
	end[2] += 100;
	VectorSubtract(end, bs->origin, up);
	BotEntityInfo(bs->client, &entinfo);
	if (!entinfo.valid)
		return;
	VectorSubtract(entinfo.origin, entinfo.lastvisorigin, botvelocity);
	falling = (DotProduct(up, botvelocity) < 0);
	// if(falling && bot_developer.integer)
	//	G_Printf("falling! %f \n", DotProduct(up, botvelocity) );

	// trap_EA_MoveForward(bs->client);
	BotEntityInfo(bs->enemy, &enemyinfo);
	if (!enemyinfo.valid)
		return;
	// direction towards the enemy
	VectorSubtract(enemyinfo.origin, bs->origin, forward);

	trap_BotMoveInDirection(bs->ms, forward, 400, MOVE_WALK);
	// trap_EA_MoveLeft(bs->client);

	if (above < 200) {
		// check ceiling distance
		BotAI_Trace(&bsptrace, bs->origin, mins, maxs, end, bs->entitynum,
					CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BOTCLIP);
		if (bsptrace.fraction == 1.0) {

			// ceiling not near, fly !
			if (above > 150 && !falling) {
				rnd = crandom();
				if (rnd > 0.3)
					trap_EA_Use(bs->client);
			}
			//
			else
				trap_EA_Use(bs->client);
		}
		return;
	}
	// den fall etwas dämpfen
	if (above < 250 && falling) {
		rnd = crandom();
		if (rnd > 0.2)
			trap_EA_Use(bs->client);
	}
}

/*
==================
BotSetTeleportTime
==================
*/
void BotSetTeleportTime(bot_state_t *bs) {
	if ((bs->cur_ps.eFlags ^ bs->last_eFlags) & EF_TELEPORT_BIT) {
		bs->teleport_time = FloatTime();
	}
	bs->last_eFlags = bs->cur_ps.eFlags;
}

/*
==================
BotIsDead
==================
*/
qboolean BotIsDead(bot_state_t *bs) {
	return (bs->cur_ps.pm_type == PM_DEAD);
}

/*
==================
BotIsObserver
==================
*/
qboolean BotIsObserver(bot_state_t *bs) {
	char buf[MAX_INFO_STRING];
	if (bs->cur_ps.pm_type == PM_SPECTATOR)
		return qtrue;
	trap_GetConfigstring(CS_PLAYERS + bs->client, buf, sizeof(buf));
	if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR)
		return qtrue;
	return qfalse;
}

/*
==================
BotIntermission
==================
*/
qboolean BotIntermission(bot_state_t *bs) {
	// NOTE: we shouldn't be looking at the game code...
	if (level.intermissiontime)
		return qtrue;
	return (bs->cur_ps.pm_type == PM_FREEZE || bs->cur_ps.pm_type == PM_INTERMISSION);
}

/*
==================
BotInLavaOrSlime
==================
*/
qboolean BotInLavaOrSlime(bot_state_t *bs) {
	vec3_t feet;

	VectorCopy(bs->origin, feet);
	feet[2] -= 23;
	return (trap_AAS_PointContents(feet) & (CONTENTS_LAVA | CONTENTS_SLIME));
}

/*
==================
TeamPlayIsOn
==================
*/
int TeamPlayIsOn(void) {
	return (gametype >= GT_TEAM);
}

/*
==================
BotAggression
==================
*/
float BotAggression(bot_state_t *bs) {
	// if the bot has quad
	if (bs->inventory[INVENTORY_PADPOWER]) {
		// if the bot is not holding the gauntlet or the enemy is really nearby
		if (bs->weaponnum != WP_PUNCHY || bs->inventory[ENEMY_DIST] < 80) {
			return 70;
		}
	}
	// if the enemy is located way higher than the bot
	if (bs->inventory[ENEMY_HEIGHT] > 200)
		return 0;
	// if the bot is very low on health
	if (bs->inventory[INVENTORY_HEALTH] < 60)
		return 0;
	// if the bot is low on health
	if (bs->inventory[INVENTORY_HEALTH] < 80) {
		// if the bot has insufficient armor
		if (bs->inventory[INVENTORY_ARMOR] < 40)
			return 0;
	}

	if (bs->inventory[INVENTORY_KMA]) // FIXME/CHECKME: I'm not really sure if it is good to make them agressive in
									  // instaGib
		return 100;
	// if the bot can use the IMPERiUS
	if (bs->inventory[INVENTORY_IMPERIUS] > 0 && bs->inventory[INVENTORY_IMPERIUSAMMO] > 0)
		return 100;
	// if the bot can use the SPLASHER
	if (bs->inventory[INVENTORY_SPLASHER] > 0 && bs->inventory[INVENTORY_SPLASHERAMMO] > 5)
		return 95;
	// if the bot can use the BOASTER
	if (bs->inventory[INVENTORY_BOASTER] > 0 && bs->inventory[INVENTORY_BOASTERAMMO] > 50)
		return 90;
	// if the bot can use the BETTY
	if (bs->inventory[INVENTORY_BETTY] > 0 && bs->inventory[INVENTORY_BETTYAMMO] > 5)
		return 90;
	// if the bot can use the BUBBLEG
	if (bs->inventory[INVENTORY_BUBBLEG] > 0 && bs->inventory[INVENTORY_BUBBLEGAMMO] > 40)
		return 85;
	// if the bot can use the grenade launcher
	if (bs->inventory[INVENTORY_BALLOONY] > 0 && bs->inventory[INVENTORY_BALLOONYAMMO] > 10)
		return 80;
	// if the bot can use the shotgun
	if (bs->inventory[INVENTORY_PUMPER] > 0 && bs->inventory[INVENTORY_PUMPERAMMO] > 10)
		return 50;
	// otherwise the bot is not feeling too good
	return 0;
}

/*
==================
BotFeelingBad
==================
*/
float BotFeelingBad(bot_state_t *bs) {
	if (bs->weaponnum == WP_PUNCHY) {
		return 100;
	}
	if (bs->inventory[INVENTORY_HEALTH] < 40) {
		return 100;
	}
	if (bs->weaponnum == WP_NIPPER) {
		return 90;
	}
	if (bs->inventory[INVENTORY_HEALTH] < 60) {
		return 80;
	}
	return 0;
}

/*
==================
BotWantsToRetreat
==================
*/
int BotWantsToRetreat(bot_state_t *bs) {
	aas_entityinfo_t entinfo;

	if (gametype == GT_CTF) {
		if (bs->ltgtype == LTG_CAPTUREFLAG || bs->ltgtype == LTG_GETFLAG || bs->ltgtype == LTG_PICKUPFLAG) {
			return qtrue;
		}
	} else if (gametype == GT_FREEZETAG) {
		if (bs->ltgtype == LTG_UNFREEZE) {
			return qtrue;
		}
	} else if (gametype == GT_BALLOON) {
		if (bs->ltgtype == LTG_ATTACKENEMYBASE)
			return qtrue;
		if (bs->ltgtype == LTG_DEFENDKEYAREA && // attack only if inside defendrange
			trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, bs->teamgoal.areanum, TFL_DEFAULT) > 250) {
			return qtrue;
		}
	}

	else if (gametype == GT_SPRAY || gametype == GT_SPRAYFFA) {
		if (IsWall(bs->enemy)) // spraywall, avoid the odd movement of battle_fight node
			return qtrue;

		// bot has cards? -> qtrue
		BotEntityInfo(bs->client, &entinfo);
		if (!entinfo.valid)
			return qfalse;

		if (EntityCarriesCarts(&entinfo))
			return qtrue;

		// nmy has cards? -> qfalse
		BotEntityInfo(bs->enemy, &entinfo);
		if (!entinfo.valid)
			return qfalse;

		if (EntityCarriesCarts(&entinfo))
			return qfalse;
	}

	if (BotAggression(bs) < 50)
		return qtrue;
	return qfalse;
}

/*
==================
BotWantsToChase
==================
*/
int BotWantsToChase(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	aas_entityinfo_t enemyinfo;

	if (gametype == GT_CTF) {
		if (bs->ltgtype == LTG_CAPTUREFLAG || bs->ltgtype == LTG_GETFLAG || bs->ltgtype == LTG_PICKUPFLAG) {
			return qfalse;
		}
	} else if (gametype == GT_FREEZETAG) {
		if (bs->ltgtype == LTG_UNFREEZE) {
			return qfalse;
		}
	} else if (gametype == GT_BALLOON) {
		if (bs->ltgtype == LTG_DEFENDKEYAREA || bs->ltgtype == LTG_ATTACKENEMYBASE)
			return qfalse;
	} else if (gametype == GT_SPRAY || gametype == GT_SPRAYFFA) {

		// bot has cards? -> dont chase
		BotEntityInfo(bs->client, &entinfo);
		if (!entinfo.valid)
			return qfalse;

		if (EntityCarriesCarts(&entinfo))
			return qfalse;

		BotEntityInfo(bs->enemy, &enemyinfo);
		if (!enemyinfo.valid)
			return qfalse;

		// nmy has cards? -> chase
		if (EntityCarriesCarts(&enemyinfo))
			return qtrue;
	}

	if (BotAggression(bs) > 50)
		return qtrue;
	return qfalse;
}

/*
==================
BotWantsToHelp
==================
*/
int BotWantsToHelp(bot_state_t *bs) {
	return qtrue;
}

/*
==================
BotDontAvoid
==================
*/
void BotDontAvoid(bot_state_t *bs, char *itemname) {
	bot_goal_t goal;
	int num;

	num = trap_BotGetLevelItemGoal(-1, itemname, &goal);
	while (num >= 0) {
		trap_BotRemoveFromAvoidGoals(bs->gs, goal.number);
		num = trap_BotGetLevelItemGoal(num, itemname, &goal);
	}
}

/*
==================
BotGoForPowerups
==================
*/
void BotGoForPowerups(bot_state_t *bs) {

	// don't avoid any of the powerups anymore
	BotDontAvoid(bs, "REViVAL");
	BotDontAvoid(bs, "ViSiONLESS");
	BotDontAvoid(bs, "JUMPER");
	BotDontAvoid(bs, "SPEEDY");
	BotDontAvoid(bs, "CLiMBER");
	BotDontAvoid(bs, "PADPOWER");
	BotDontAvoid(bs, "KiLLERDUCKS");
	BotDontAvoid(bs, "FLOATER");
	// reset the long term goal time so the bot will go for the powerup
	// NOTE: the long term goal type doesn't change
	bs->ltg_time = 0;
}

/*
==================
BotRoamGoal
==================
*/
void BotRoamGoal(bot_state_t *bs, vec3_t goal) {
	int pc, i;
	float len, rnd;
	vec3_t dir, bestorg, belowbestorg;
	bsp_trace_t trace;

	for (i = 0; i < 10; i++) {
		// start at the bot origin
		VectorCopy(bs->origin, bestorg);
		rnd = random();
		if (rnd > 0.25) {
			// add a random value to the x-coordinate
			if (random() < 0.5)
				bestorg[0] -= 800 * random() + 100;
			else
				bestorg[0] += 800 * random() + 100;
		}
		if (rnd < 0.75) {
			// add a random value to the y-coordinate
			if (random() < 0.5)
				bestorg[1] -= 800 * random() + 100;
			else
				bestorg[1] += 800 * random() + 100;
		}
		// add a random value to the z-coordinate (NOTE: 48 = maxjump?)
		bestorg[2] += 2 * 48 * crandom();
		// trace a line from the origin to the roam target
		BotAI_Trace(&trace, bs->origin, NULL, NULL, bestorg, bs->entitynum, MASK_SOLID);
		// direction and length towards the roam target
		VectorSubtract(trace.endpos, bs->origin, dir);
		len = VectorNormalize(dir);
		// if the roam target is far away enough
		if (len > 200) {
			// the roam target is in the given direction before walls
			VectorScale(dir, len * trace.fraction - 40, dir);
			VectorAdd(bs->origin, dir, bestorg);
			// get the coordinates of the floor below the roam target
			belowbestorg[0] = bestorg[0];
			belowbestorg[1] = bestorg[1];
			belowbestorg[2] = bestorg[2] - 800;
			BotAI_Trace(&trace, bestorg, NULL, NULL, belowbestorg, bs->entitynum, MASK_SOLID);
			//
			if (!trace.startsolid) {
				trace.endpos[2]++;
				pc = trap_PointContents(trace.endpos, bs->entitynum);
				if (!(pc & (CONTENTS_LAVA | CONTENTS_SLIME))) {
					VectorCopy(bestorg, goal);
					return;
				}
			}
		}
	}
	VectorCopy(bestorg, goal);
}

/*
==================
BotAttackMove
==================
*/
bot_moveresult_t BotAttackMove(bot_state_t *bs, int tfl) {
	int movetype, i;
	float attack_skill, jumper, croucher, dist, strafechange_time;
	float attack_dist, attack_range;
	vec3_t forward, backward, sideward, hordir, up = {0, 0, 1};
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	bot_goal_t goal;

	if (bs->attackchase_time > FloatTime()) {
		// create the chase goal
		goal.entitynum = bs->enemy;
		goal.areanum = bs->lastenemyareanum;
		VectorCopy(bs->lastenemyorigin, goal.origin);
		VectorSet(goal.mins, -8, -8, -8);
		VectorSet(goal.maxs, 8, 8, 8);
		// initialize the movement state
		BotSetupForMovement(bs);
		// move towards the goal
		trap_BotMoveToGoal(&moveresult, bs->ms, &goal, tfl);
		return moveresult;
	}
	//
	memset(&moveresult, 0, sizeof(bot_moveresult_t));
	//
	attack_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_ATTACK_SKILL, 0, 1);
	jumper = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_JUMPER, 0, 1);
	croucher = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CROUCHER, 0, 1);
	// if the bot is really stupid
	if (attack_skill < 0.2)
		return moveresult;
	// initialize the movement state
	BotSetupForMovement(bs);
	// get the enemy entity info
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid)
		return moveresult;
	// direction towards the enemy
	VectorSubtract(entinfo.origin, bs->origin, forward);
	// the distance towards the enemy
	dist = VectorNormalize(forward);
	VectorNegate(forward, backward);
	// walk, crouch or jump
	movetype = MOVE_WALK;
	//
	if (bs->attackcrouch_time < FloatTime() - 1) {
		if (random() < jumper) {
			movetype = MOVE_JUMP;
		}
		// wait at least one second before crouching again
		else if (bs->attackcrouch_time < FloatTime() - 1 && random() < croucher) {
			bs->attackcrouch_time = FloatTime() + croucher * 5;
		}
	}
	if (bs->attackcrouch_time > FloatTime())
		movetype = MOVE_CROUCH;
	// if the bot should jump
	if (movetype == MOVE_JUMP) {
		// if jumped last frame
		if (bs->attackjump_time > FloatTime()) {
			movetype = MOVE_WALK;
		} else {
			bs->attackjump_time = FloatTime() + 1;
		}
	}

	if (IsBoomie(bs->enemy)) {
		attack_range = 30;
		attack_dist = BOOMIES_TRIGGERBOX * 4 - attack_range;
		movetype = MOVE_WALK; // no jump, no crouch
	} else if (bs->cur_ps.weapon == WP_PUNCHY) {
		attack_dist = 0;
		attack_range = 0;
	} else {
		attack_dist = IDEAL_ATTACKDIST;
		attack_range = 40;
	}

	// if the bot is stupid
	if (attack_skill <= 0.4) {
		// just walk to or away from the enemy
		if (dist > attack_dist + attack_range) {
			if (trap_BotMoveInDirection(bs->ms, forward, 400, movetype))
				return moveresult;
		}
		if (dist < attack_dist - attack_range) {
			if (trap_BotMoveInDirection(bs->ms, backward, 400, movetype))
				return moveresult;
		}
		return moveresult;
	}
	// increase the strafe time
	bs->attackstrafe_time += bs->thinktime;
	// get the strafe change time
	strafechange_time = 0.4 + (1 - attack_skill) * 0.2;
	if (attack_skill > 0.7)
		strafechange_time += crandom() * 0.2;
	// if the strafe direction should be changed
	if (bs->attackstrafe_time > strafechange_time) {
		// some magic number :)
		if (random() > 0.935) {
			// flip the strafe direction
			bs->flags ^= BFL_STRAFERIGHT;
			bs->attackstrafe_time = 0;
		}
	}
	//
	for (i = 0; i < 2; i++) {
		hordir[0] = forward[0];
		hordir[1] = forward[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		// get the sideward vector
		CrossProduct(hordir, up, sideward);
		// reverse the vector depending on the strafe direction
		if (bs->flags & BFL_STRAFERIGHT)
			VectorNegate(sideward, sideward);
		// randomly go back a little
		if (random() > 0.9) {
			VectorAdd(sideward, backward, sideward);
		} else {
			// walk forward or backward to get at the ideal attack distance
			if (dist > attack_dist + attack_range) {
				VectorAdd(sideward, forward, sideward);
			} else if (dist < attack_dist - attack_range) {
				VectorAdd(sideward, backward, sideward);
			}
		}
		// perform the movement
		if (trap_BotMoveInDirection(bs->ms, sideward, 400, movetype))
			return moveresult;
		// movement failed, flip the strafe direction
		bs->flags ^= BFL_STRAFERIGHT;
		bs->attackstrafe_time = 0;
	}
	// bot couldn't do any useful movement
	//	bs->attackchase_time = AAS_Time() + 6;
	return moveresult;
}

/*
==================
BotSameTeam
==================
*/
int BotSameTeam(bot_state_t *bs, int entnum) {
	gclient_t *otherclient;
	gclient_t *botclient;

	if (bs->client < 0 || bs->client >= MAX_CLIENTS) {
		// BotAI_Print(PRT_ERROR, "BotSameTeam: client out of range\n");
		return qfalse;
	}
	if (entnum < 0 || entnum >= MAX_CLIENTS) {
		// BotAI_Print(PRT_ERROR, "BotSameTeam: client out of range\n");
		return qfalse;
	}
	if (gametype < GT_TEAM)
		return qfalse;

	otherclient = g_entities[entnum].client;
	botclient = g_entities[bs->client].client;
	if (!otherclient || !botclient)
		return qfalse;

	return (otherclient->sess.sessionTeam == botclient->sess.sessionTeam);
}

/*
==================
InFieldOfVision
==================
*/
qboolean InFieldOfVision(vec3_t viewangles, float fov, vec3_t angles) {
	int i;
	float diff, angle;

	for (i = 0; i < 2; i++) {
		angle = AngleMod(viewangles[i]);
		angles[i] = AngleMod(angles[i]);
		diff = angles[i] - angle;
		if (angles[i] > angle) {
			if (diff > 180.0)
				diff -= 360.0;
		} else {
			if (diff < -180.0)
				diff += 360.0;
		}
		if (diff > 0) {
			if (diff > fov * 0.5)
				return qfalse;
		} else {
			if (diff < -fov * 0.5)
				return qfalse;
		}
	}
	return qtrue;
}

/*
==================
BotEntityVisible

returns visibility in the range [0, 1] taking fog and water surfaces into account
==================
*/
float BotEntityVisible(int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent) {
	int i, contents_mask, passent, hitent, infog, inwater, otherinfog, pc;
	float squaredfogdist, waterfactor, vis, bestvis;
	bsp_trace_t trace;
	aas_entityinfo_t entinfo;
	vec3_t dir, entangles, start, end, middle;

	if (ent < MAX_CLIENTS && gametype == GT_LPS)
		return 1.0f; // all players visible. everywhere

	// calculate middle of bounding box
	BotEntityInfo(ent, &entinfo);
	if (!entinfo.valid)
		return 0;
	VectorAdd(entinfo.mins, entinfo.maxs, middle);
	VectorScale(middle, 0.5, middle);
	VectorAdd(entinfo.origin, middle, middle);
	// check if entity is within field of vision
	VectorSubtract(middle, eye, dir);
	vectoangles(dir, entangles);
	if (!InFieldOfVision(viewangles, fov, entangles))
		return 0;
	//
	pc = trap_AAS_PointContents(eye);
	infog = (pc & CONTENTS_FOG);
	inwater = (pc & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER));
	//
	bestvis = 0;
	for (i = 0; i < 3; i++) {
		// if the point is not in potential visible sight
		// if (!AAS_inPVS(eye, middle)) continue;
		//
		contents_mask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
		passent = viewer;
		hitent = ent;
		VectorCopy(eye, start);
		VectorCopy(middle, end);
		// if the entity is in water, lava or slime
		if (trap_AAS_PointContents(middle) & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER)) {
			contents_mask |= (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER);
		}
		// if eye is in water, lava or slime
		if (inwater) {
			if (!(contents_mask & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))) {
				passent = ent;
				hitent = viewer;
				VectorCopy(middle, start);
				VectorCopy(eye, end);
			}
			contents_mask ^= (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER);
		}
		// trace from start to end
		BotAI_Trace(&trace, start, NULL, NULL, end, passent, contents_mask);
		// if water was hit
		waterfactor = 1.0f;
		// note: trace.contents is always 0, see BotAI_Trace
		if (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER)) {
			// if the water surface is translucent
			if (1) {
				// trace through the water
				contents_mask &= ~(CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER);
				BotAI_Trace(&trace, trace.endpos, NULL, NULL, end, passent, contents_mask);
				waterfactor = 0.5f;
			}
		}
		// if a full trace or the hitent was hit
		if (trace.fraction >= 1 || trace.ent == hitent) {
			// check for fog, assuming there's only one fog brush where
			// either the viewer or the entity is in or both are in
			otherinfog = (trap_AAS_PointContents(middle) & CONTENTS_FOG);
			if (infog && otherinfog) {
				VectorSubtract(trace.endpos, eye, dir);
				squaredfogdist = VectorLengthSquared(dir);
			} else if (infog) {
				VectorCopy(trace.endpos, start);
				BotAI_Trace(&trace, start, NULL, NULL, eye, viewer, CONTENTS_FOG);
				VectorSubtract(eye, trace.endpos, dir);
				squaredfogdist = VectorLengthSquared(dir);
			} else if (otherinfog) {
				VectorCopy(trace.endpos, end);
				BotAI_Trace(&trace, eye, NULL, NULL, end, viewer, CONTENTS_FOG);
				VectorSubtract(end, trace.endpos, dir);
				squaredfogdist = VectorLengthSquared(dir);
			} else {
				// if the entity and the viewer are not in fog assume there's no fog in between
				squaredfogdist = 0;
			}
			// decrease visibility with the view distance through fog
			vis = 1.0f / ((squaredfogdist * 0.001f) < 1 ? 1 : (squaredfogdist * 0.001f));
			// if entering water visibility is reduced
			vis *= waterfactor;
			//
			if (vis > bestvis)
				bestvis = vis;
			// if pretty much no fog
			if (bestvis >= 0.95f)
				return bestvis;
		}
		// check bottom and top of bounding box as well
		if (i == 0)
			middle[2] += entinfo.mins[2];
		else if (i == 1)
			middle[2] += entinfo.maxs[2] - entinfo.mins[2];
	}
	return bestvis;
}

// cyr{
qboolean EnemyFitsWell(bot_state_t *bs, aas_entityinfo_t *entinfo, int curenemy) {
	aas_entityinfo_t curenemyinfo;
	vec3_t dir1, dir2;
	float l1, l2;

	if (curenemy < 0)
		return qtrue;
	if (entinfo->type != ET_PLAYER)
		return qfalse;

	// enemy carries sprayammo?
	if (g_entities[entinfo->number].client->ps.ammo[WP_SPRAYPISTOL] >= 2)
		return qtrue;

	if (bs->weaponnum == WP_IMPERIUS) {
		BotEntityInfo(curenemy, &curenemyinfo);
		if (!curenemyinfo.valid)
			return qfalse;
		VectorSubtract(entinfo->origin, bs->origin, dir1);
		l1 = VectorLength(dir1);
		VectorSubtract(curenemyinfo.origin, bs->origin, dir2);
		l2 = VectorLength(dir2);
		if (l2 < 900 && l1 > l2) { // curenemy is near, candidate is further away
			// G_Printf("^1 imped \n");	// cyr_ptr
			return qtrue;
		}
	}
	return qfalse;
}
// cyr}

// detect enemys that changed teams or went spec
qboolean BotNmyTurnedInvalid(bot_state_t *bs) {
	aas_entityinfo_t entinfo;

	if (bs->enemy < 0)
		return qtrue;

	if (BotSameTeam(bs, bs->enemy))
		return qtrue;

	if (ClientInSprayroom(bs->enemy)) {
		return qtrue;
	}

	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid)
		return qtrue;

	// specs are dead
	if (EntityIsDead(&entinfo))
		return qtrue;

	return qfalse;
}

/*
==================
BotFindEnemy
==================
*/
int BotFindEnemy(bot_state_t *bs, int curenemy) {
	int i, healthdecrease;
	float f, alertness, easyfragger, vis;
	float squaredist, cursquaredist;
	aas_entityinfo_t entinfo, curenemyinfo;
	vec3_t dir, angles;

	gentity_t *ent;
	vec3_t nmydir;
	float lengthsqr;

	alertness = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_ALERTNESS, 0, 1);
	easyfragger = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_EASY_FRAGGER, 0, 1);
	// check if the health decreased
	healthdecrease = bs->lasthealth > bs->inventory[INVENTORY_HEALTH];
	// remember the current health value
	bs->lasthealth = bs->inventory[INVENTORY_HEALTH];
	//
	if (curenemy >= 0) {
		// can we skip checking other candidates?
		BotEntityInfo(curenemy, &curenemyinfo);
		if (curenemyinfo.valid) {
			if (EnemyFitsWell(bs, &curenemyinfo, curenemy))
				return qfalse;
			// no we cant, so lets compare
			VectorSubtract(curenemyinfo.origin, bs->origin, dir);
			cursquaredist = VectorLengthSquared(dir);
		} else {
			cursquaredist = 0;
		}
	} else {
		cursquaredist = 0;
	}

	if (ClientInSprayroom(bs->client) && bs->inventory[INVENTORY_SPRAYPISTOLAMMO]) {
		vec3_t target, dir;
		bot_goal_t *goal;
		bsp_trace_t trace;

		goal = bs->which_wall; // BotChooseWall(bs);

		VectorCopy(goal->origin, target);
		target[2] += 1;

		VectorSubtract(target, bs->origin, dir);
		if (VectorLengthSquared(dir) < Square(256.0f)) { // cyr2

			BotAI_Trace(&trace, bs->eye, NULL, NULL, target, bs->client, CONTENTS_SOLID);
			if (trace.fraction >= 1 || trace.ent == goal->entitynum) {

				if (goal->entitynum == bs->enemy) {
					return qfalse;
				}
				bs->enemy = goal->entitynum;
				bs->enemysight_time = FloatTime();
				bs->enemysuicide = qfalse;
				bs->enemydeath_time = 0;
				bs->enemyvisible_time = FloatTime();
				return qtrue;
			}
		}
		return qfalse;
	} else if (ClientInSprayroom(bs->client)) // after spraying all carts, attack no players nor walls
		return qfalse;

	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (i == bs->client) {
			continue;
		}
		// if it's the current enemy
		if (i == curenemy) {
			continue;
		}
		// if the enemy has targeting disabled
		if (g_entities[i].flags & FL_NOTARGET) {
			continue;
		}
		BotEntityInfo(i, &entinfo);

		if (!entinfo.valid)
			continue;
		// if the enemy isn't dead and the enemy isn't the bot self
		if (EntityIsDead(&entinfo) || entinfo.number == bs->entitynum)
			continue;
		// if the enemy is invisible and not shooting
		if (EntityIsInvisible(&entinfo) && !EntityIsShooting(&entinfo)) {
			continue;
		}
		// no fighting in the sprayroom please
		if (ClientInSprayroom(i))
			continue;
		// if not an easy fragger don't shoot at chatting players
		if (easyfragger < 0.5 && EntityIsChatting(&entinfo))
			continue;

		if (lastteleport_time > FloatTime() - 3) {
			VectorSubtract(entinfo.origin, lastteleport_origin, dir);
			if (VectorLengthSquared(dir) < Square(70))
				continue;
		}
		// calculate the distance towards the enemy
		VectorSubtract(entinfo.origin, bs->origin, dir);
		squaredist = VectorLengthSquared(dir);

		if (!EnemyFitsWell(bs, &entinfo, curenemy)) //(!EntityCarriesFlag(&entinfo))
		{
			// if this enemy is further away than the current one
			if (curenemy >= 0 && squaredist > cursquaredist)
				continue;
		} // end if
		// if the bot has no
		if (squaredist > Square(900.0 + alertness * 4000.0))
			continue;
		// if on the same team
		if (BotSameTeam(bs, i))
			continue;
		// if the bot's health decreased or the enemy is shooting
		if (curenemy < 0 && (healthdecrease || EntityIsShooting(&entinfo)))
			f = 360;
		else
			f = 90 + 90 - (90 - (squaredist > Square(810) ? Square(810) : squaredist) / (810 * 9));
		// check if the enemy is visible
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, f, i);
		if (vis <= 0)
			continue;
		// if the enemy is quite far away, not shooting and the bot is not damaged
		if (curenemy < 0 && squaredist > Square(100) && !healthdecrease && !EntityIsShooting(&entinfo)) {
			// check if we can avoid this enemy
			VectorSubtract(bs->origin, entinfo.origin, dir);
			vectoangles(dir, angles);
			// if the bot isn't in the fov of the enemy
			if (!InFieldOfVision(entinfo.angles, 90, angles)) {
				// update some stuff for this enemy
				BotUpdateBattleInventory(bs, i);
				// if the bot doesn't really want to fight
				if (BotWantsToRetreat(bs))
					continue;
			}
		}
		// found an enemy
		bs->enemy = entinfo.number;
		if (curenemy >= 0)
			bs->enemysight_time = FloatTime() - 2;
		else
			bs->enemysight_time = FloatTime();
		bs->enemysuicide = qfalse;
		bs->enemydeath_time = 0;
		bs->enemyvisible_time = FloatTime();
		return qtrue;
	}
	// no client enemy, search for ducks
	for (i = MAX_CLIENTS; i < level.num_entities; i++) {
		ent = &g_entities[i];
		if (!ent->inuse)
			continue;

		if (i == curenemy)
			continue;

		if (IsBambam(i)) {
			// it has to be an enemy
			if ((ent->team[0] == 'b' && BotTeam(bs) == TEAM_BLUE) || (ent->team[0] == 'r' && BotTeam(bs) == TEAM_RED))
				continue;

			vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, i);
			if (vis <= 0)
				continue;

			bs->enemy = i;
			bs->enemysight_time = FloatTime();
			bs->enemysuicide = qfalse;
			bs->enemydeath_time = 0;
			bs->enemyvisible_time = FloatTime();
			return qtrue;
		}

		if (IsBoomie(i)) {
			// it has to be an enemy
			if ((ent->team[0] == 'b' && BotTeam(bs) == TEAM_BLUE) || (ent->team[0] == 'r' && BotTeam(bs) == TEAM_RED))
				continue;

			vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 80, i);
			if (vis <= 0)
				continue;

			VectorSubtract(ent->r.currentOrigin, bs->origin, nmydir);
			lengthsqr = VectorLengthSquared(nmydir);
			if (lengthsqr > Square(400))
				continue;

			bs->enemy = i;
			bs->enemysight_time = FloatTime();
			bs->enemysuicide = qfalse;
			bs->enemydeath_time = 0;
			bs->enemyvisible_time = FloatTime();
			return qtrue;
		}

		/*if ( !ent->inuse || !IsDuck(i) )	continue;

		// low distance ?
		VectorSubtract( ent->r.currentOrigin, bs->origin, duckdir);
		lengthsqr = VectorLengthSquared(duckdir);
		if(lengthsqr > Square(100) )
				continue;
		// looking at me ? (duck does not store angles !?)
		//AngleVectors (duckangles, duckforward, duckright, duckup);
		if(healthdecrease){
			bs->enemy = i;
			bs->enemysight_time = FloatTime();
			bs->enemysuicide = qfalse;
			bs->enemydeath_time = 0;
			bs->enemyvisible_time = FloatTime();
			return qtrue;
		}*/
	}
	return qfalse;
}

// cyr_ret{
int BotSprayWallCheckAttack(bot_state_t *bs) {
	//    float points, reactiontime, fov, firethrottle;
	float fov;
	int attackentity;
	//    bsp_trace_t bsptrace;
	// float selfpreservation;
	vec3_t forward, right, start, end, dir, angles;
	weaponinfo_t wi;
	bsp_trace_t trace;
	aas_entityinfo_t entinfo;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};

	attackentity = bs->enemy;
	//
	BotEntityInfo(attackentity, &entinfo);
	if (!entinfo.valid)
		return qtrue;

	// if attacking a player
	if (attackentity <= MAX_CLIENTS)
		return qtrue;

	// if changing weapons
	if (bs->weaponchange_time > FloatTime() - 0.45) {
		return qtrue;
	}

	VectorSubtract(bs->aimtarget, bs->eye, dir);
	//
	if (VectorLengthSquared(dir) > Square(bs->spraydist))
		return qtrue;

	if (VectorLengthSquared(dir) < Square(100))
		fov = 120;
	else
		fov = 50;
	//
	vectoangles(dir, angles);
	if (!InFieldOfVision(bs->viewangles, fov, angles)) {
		return qtrue;
	}
	// get the weapon info
	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	// get the start point shooting from
	VectorCopy(bs->origin, start);
	start[2] += bs->cur_ps.viewheight;
	AngleVectors(bs->viewangles, forward, right, NULL);
	start[0] += forward[0] * wi.offset[0] + right[0] * wi.offset[1];
	start[1] += forward[1] * wi.offset[0] + right[1] * wi.offset[1];
	start[2] += forward[2] * wi.offset[0] + right[2] * wi.offset[1] + wi.offset[2];

	// end point aiming at
	// VectorSubtract(start, bs->aimtarget, dir);
	VectorMA(start, 256, forward, end);

	// a little back to make sure not inside a very close enemy
	VectorMA(start, -12, forward, start);
	BotAI_Trace(&trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT);
	// not hitting nmy ?
	if (trace.fraction == 1)
		return qtrue;
	if (trace.ent != attackentity)
		return qtrue;

	// everthing went fine, shoot
	trap_EA_Attack(bs->client);
	return qfalse;
}
// cyr_ret }

/*
==================
BotVisibleTeamMatesAndEnemies
==================
*/
void BotVisibleTeamMatesAndEnemies(bot_state_t *bs, int *teammates, int *enemies, float range) {
	int i;
	float vis;
	aas_entityinfo_t entinfo;
	vec3_t dir;

	if (teammates)
		*teammates = 0;
	if (enemies)
		*enemies = 0;
	for (i = 0; i < level.maxclients; i++) {
		if (i == bs->client)
			continue;
		//
		BotEntityInfo(i, &entinfo);
		// if this player is active
		if (!entinfo.valid)
			continue;
		// if not within range
		VectorSubtract(entinfo.origin, bs->origin, dir);
		if (VectorLengthSquared(dir) > Square(range))
			continue;
		// if the flag carrier is not visible
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, i);
		if (vis <= 0)
			continue;
		// if the flag carrier is on the same team
		if (BotSameTeam(bs, i)) {
			if (teammates)
				(*teammates)++;
		} else {
			if (enemies)
				(*enemies)++;
		}
	}
}

/*
==================
BotAimAtEnemy
==================
*/
void BotAimAtEnemy(bot_state_t *bs) {
	int i, enemyvisible;
	float dist, f, aim_skill, aim_accuracy, speed, reactiontime;
	vec3_t dir, bestorigin, end, start, groundtarget, cmdmove, enemyvelocity;
	vec3_t mins = {-4, -4, -4}, maxs = {4, 4, 4};
	weaponinfo_t wi;
	aas_entityinfo_t entinfo;
	bot_goal_t goal;
	bsp_trace_t trace;
	vec3_t target;
	float wi_spread_y, wi_spread_x; // cyr

	// if the bot has no enemy
	if (bs->enemy < 0) {
		return;
	}
	if (bs->ltgtype == LTG_GIVECART) {
		aas_entityinfo_t mateinfo;
		vec3_t mdir;

		BotEntityInfo(bs->teammate, &mateinfo);
		if (mateinfo.valid) {
			VectorSubtract(mateinfo.origin, bs->origin, mdir);
			if (VectorLengthSquared(mdir) < Square(PUSHCART_DIST))
				return;
		}
	}
	// get the enemy entity information
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid)
		return;

	// if this is not a player (should be an obelisk)
	if (IsWall(bs->enemy)) {
		// if the obelisk is visible
		// eigena cyr{
		if (bs->lastcartcount != bs->inventory[INVENTORY_SPRAYPISTOLAMMO] || bs->sprayTimer < FloatTime()) {

			bs->sprayTimer = FloatTime() + 3;
			// recalc target
			for (i = 0; i < 3; i++) {
				int nmysize;
				nmysize = entinfo.maxs[i] - entinfo.mins[i];
				// random value for each dimension ( 10% space at borders)
				bs->syctarget[i] = entinfo.mins[i] + nmysize * 0.15f + nmysize * 0.7f * random(); // cyr2
				bs->spraydist = 100 + 150.0 * random();
			}
			bs->lastcartcount = bs->inventory[INVENTORY_SPRAYPISTOLAMMO];
		}
		VectorCopy(bs->syctarget, target);
		// cyr}

		// aim at the obelisk
		VectorSubtract(target, bs->eye, dir);
		vectoangles(dir, bs->ideal_viewangles);
		// set the aim target before trying to attack
		VectorCopy(target, bs->aimtarget);
		return;
	}
	//
	// BotAI_Print(PRT_MESSAGE, "client %d: aiming at client %d\n", bs->entitynum, bs->enemy);
	//
	aim_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL, 0, 1);
	aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY, 0, 1);
	//
	if (aim_skill > 0.95) {
		// don't aim too early
		reactiontime = 0.5 * trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1);
		if (bs->enemysight_time > FloatTime() - reactiontime)
			return;
		if (bs->teleport_time > FloatTime() - reactiontime)
			return;
	}

	// get the weapon information
	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	// get the weapon specific aim accuracy and or aim skill
	if (wi.number == WP_NIPPER) {
		aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_MACHINEGUN, 0, 1);
	} else if (wi.number == WP_PUMPER) {
		aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_SHOTGUN, 0, 1);
	} else if (wi.number == WP_BALLOONY) {
		aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_GRENADELAUNCHER, 0, 1);
		aim_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL_GRENADELAUNCHER, 0, 1);
	} else if (wi.number == WP_BETTY) {
		aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_ROCKETLAUNCHER, 0, 1);
		aim_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL_ROCKETLAUNCHER, 0, 1);
	} else if (wi.number == WP_BOASTER) {
		aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_LIGHTNING, 0, 1);
	} else if (wi.number == WP_SPLASHER) {
		aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_RAILGUN, 0, 1);
	} else if (wi.number == WP_BUBBLEG) {
		aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_PLASMAGUN, 0, 1);
		aim_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL_PLASMAGUN, 0, 1);
	} else if (wi.number == WP_IMPERIUS) {
		aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_BFG10K, 0, 1);
		aim_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL_BFG10K, 0, 1);
	}
	//
	if (aim_accuracy <= 0)
		aim_accuracy = 0.0001f;
	// get the enemy entity information
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid)
		return;

	// if the enemy is invisible then shoot crappy most of the time
	if (EntityIsInvisible(&entinfo)) {
		if (random() > 0.1)
			aim_accuracy *= 0.4f;
	}
	//
	VectorSubtract(entinfo.origin, entinfo.lastvisorigin, enemyvelocity);
	VectorScale(enemyvelocity, 1 / entinfo.update_time, enemyvelocity);
	// enemy origin and velocity is remembered every 0.5 seconds
	if (bs->enemyposition_time < FloatTime()) {
		//
		bs->enemyposition_time = FloatTime() + 0.5;
		VectorCopy(enemyvelocity, bs->enemyvelocity);
		VectorCopy(entinfo.origin, bs->enemyorigin);
	}
	// if not extremely skilled
	if (aim_skill < 0.9) {
		VectorSubtract(entinfo.origin, bs->enemyorigin, dir);
		// if the enemy moved a bit
		if (VectorLengthSquared(dir) > Square(48)) {
			// if the enemy changed direction
			if (DotProduct(bs->enemyvelocity, enemyvelocity) < 0) {
				// aim accuracy should be worse now
				aim_accuracy *= 0.7f;
			}
		}
	}
	// check visibility of enemy
	enemyvisible = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy);
	// if the enemy is visible
	if (enemyvisible) {
		//
		VectorCopy(entinfo.origin, bestorigin);
		bestorigin[2] += 8;
		// get the start point shooting from
		// NOTE: the x and y projectile start offsets are ignored
		VectorCopy(bs->origin, start);
		start[2] += bs->cur_ps.viewheight;
		start[2] += wi.offset[2];
		//
		BotAI_Trace(&trace, start, mins, maxs, bestorigin, bs->entitynum, MASK_SHOT);
		// if the enemy is NOT hit
		if (trace.fraction <= 1 && trace.ent != entinfo.number) {
			bestorigin[2] += 16;
		}
		// if it is not an instant hit weapon the bot might want to predict the enemy
		if (wi.speed) {
			//
			VectorSubtract(bestorigin, bs->origin, dir);
			dist = VectorLength(dir);
			VectorSubtract(entinfo.origin, bs->enemyorigin, dir);
			// if the enemy is NOT pretty far away and strafing just small steps left and right
			if (!(dist > 100 && VectorLengthSquared(dir) < Square(32))) {
				// if skilled enough do exact prediction
				if (aim_skill > 0.8 &&
					// if the weapon is ready to fire
					bs->cur_ps.weaponstate == WEAPON_READY) {
					aas_clientmove_t move;
					vec3_t origin;

					VectorSubtract(entinfo.origin, bs->origin, dir);
					// distance towards the enemy
					dist = VectorLength(dir);
					// direction the enemy is moving in
					VectorSubtract(entinfo.origin, entinfo.lastvisorigin, dir);
					//
					VectorScale(dir, 1 / entinfo.update_time, dir);
					//
					VectorCopy(entinfo.origin, origin);
					origin[2] += 1;
					//
					VectorClear(cmdmove);
					// AAS_ClearShownDebugLines();
					trap_AAS_PredictClientMovement(&move, bs->enemy, origin, PRESENCE_CROUCH, qfalse, dir, cmdmove, 0,
												   dist * 10 / wi.speed, 0.1f, 0, 0, qfalse);
					VectorCopy(move.endpos, bestorigin);
					// BotAI_Print(PRT_MESSAGE, "%1.1f predicted speed = %f, frames = %f\n", FloatTime(),
					// VectorLength(dir), dist * 10 / wi.speed);
				}
				// if not that skilled do linear prediction
				else if (aim_skill > 0.4) {
					VectorSubtract(entinfo.origin, bs->origin, dir);
					// distance towards the enemy
					dist = VectorLength(dir);
					// direction the enemy is moving in
					VectorSubtract(entinfo.origin, entinfo.lastvisorigin, dir);
					dir[2] = 0;
					//
					speed = VectorNormalize(dir) / entinfo.update_time;
					// botimport.Print(PRT_MESSAGE, "speed = %f, wi->speed = %f\n", speed, wi->speed);
					// best spot to aim at
					VectorMA(entinfo.origin, (dist / wi.speed) * speed, dir, bestorigin);
				}
			}
		}
		// if the projectile does radial damage
		if (aim_skill > 0.6 && wi.proj.damagetype & DAMAGETYPE_RADIAL) {
			// if the enemy isn't standing significantly higher than the bot
			if (entinfo.origin[2] < bs->origin[2] + 16) {
				// try to aim at the ground in front of the enemy
				VectorCopy(entinfo.origin, end);
				end[2] -= 64;
				BotAI_Trace(&trace, entinfo.origin, NULL, NULL, end, entinfo.number, MASK_SHOT);
				//
				VectorCopy(bestorigin, groundtarget);
				if (trace.startsolid)
					groundtarget[2] = entinfo.origin[2] - 16;
				else
					groundtarget[2] = trace.endpos[2] - 8;
				// trace a line from projectile start to ground target
				BotAI_Trace(&trace, start, NULL, NULL, groundtarget, bs->entitynum, MASK_SHOT);
				// if hitpoint is not vertically too far from the ground target
				if (fabs(trace.endpos[2] - groundtarget[2]) < 50) {
					VectorSubtract(trace.endpos, groundtarget, dir);
					// if the hitpoint is near enough the ground target
					if (VectorLengthSquared(dir) < Square(60)) {
						VectorSubtract(trace.endpos, start, dir);
						// if the hitpoint is far enough from the bot
						if (VectorLengthSquared(dir) > Square(100)) {
							// check if the bot is visible from the ground target
							trace.endpos[2] += 1;
							BotAI_Trace(&trace, trace.endpos, NULL, NULL, entinfo.origin, entinfo.number, MASK_SHOT);
							if (trace.fraction >= 1) {
								// botimport.Print(PRT_MESSAGE, "%1.1f aiming at ground\n", AAS_Time());
								VectorCopy(groundtarget, bestorigin);
							}
						}
					}
				}
			}
		}
		bestorigin[0] += 20 * crandom() * (1 - aim_accuracy) * 0.3f;
		bestorigin[1] += 20 * crandom() * (1 - aim_accuracy) * 0.3f;
		bestorigin[2] += 10 * crandom() * (1 - aim_accuracy) * 0.3f;
	} else { // invisible enemy
		//
		VectorCopy(bs->lastenemyorigin, bestorigin);
		bestorigin[2] += 8;

		// do prediction shots around corners
		if (wi.number == WP_PUMPER || wi.number == WP_BETTY || wi.number == WP_BALLOONY) {
			// create the chase goal
			goal.entitynum = bs->client;
			goal.areanum = bs->areanum;
			VectorCopy(bs->eye, goal.origin);
			VectorSet(goal.mins, -8, -8, -8);
			VectorSet(goal.maxs, 8, 8, 8);
			//
			if (trap_BotPredictVisiblePosition(bs->lastenemyorigin, bs->lastenemyareanum, &goal, TFL_DEFAULT, target)) {
				VectorSubtract(target, bs->eye, dir);
				if (VectorLengthSquared(dir) > Square(80)) {
					VectorCopy(target, bestorigin);
					bestorigin[2] -= 20;
				}
			}
			aim_accuracy = 1;
		}
		// any obstacles/walls in between? this happens when using jumppads or teleporters
		BotAI_Trace(&trace, bs->origin, NULL, NULL, bestorigin, bs->entitynum, MASK_SHOT);
		if (trace.fraction < 0.9) {
			// if(bot_developer.integer)
			//	BotAI_Print(PRT_MESSAGE, "discarding aimtarget.. trace fraction is %.1f\n", trace.fraction);
			return;
		}
	}
	//
	if (enemyvisible) {
		BotAI_Trace(&trace, bs->eye, NULL, NULL, bestorigin, bs->entitynum, MASK_SHOT);
		VectorCopy(trace.endpos, bs->aimtarget);
	} else {
		VectorCopy(bestorigin, bs->aimtarget);
	}
	// get aim direction
	VectorSubtract(bestorigin, bs->eye, dir);
	//
	if (wi.number == WP_NIPPER || wi.number == WP_PUMPER || wi.number == WP_BOASTER || wi.number == WP_SPLASHER) {
		// distance towards the enemy
		dist = VectorLength(dir);
		if (dist > 150)
			dist = 150;
		f = 0.6 + dist / 150 * 0.4;
		aim_accuracy *= f;
	}

	if (IsBambam(bs->enemy))
		aim_accuracy += (1 - aim_accuracy) * 0.66;
	else if (IsBoomie(bs->enemy))
		aim_accuracy += (1 - aim_accuracy) * 0.5;

	// set the ideal view angles
	vectoangles(dir, bs->ideal_viewangles);

	// take the weapon spread into account for lower skilled bots
	wi_spread_x = 9 * crandom() * (1 - aim_accuracy);
	if (crandom() < 0.5f)
		bs->ideal_viewangles[PITCH] += wi_spread_x;
	else
		bs->ideal_viewangles[PITCH] -= wi_spread_x;
	bs->ideal_viewangles[PITCH] = AngleMod(bs->ideal_viewangles[PITCH]);

	wi_spread_y = 6 * crandom() * (1 - aim_accuracy);
	if (crandom() < 0.5f)
		bs->ideal_viewangles[YAW] += wi_spread_y;
	else
		bs->ideal_viewangles[YAW] -= wi_spread_y;
	bs->ideal_viewangles[YAW] = AngleMod(bs->ideal_viewangles[YAW]);
	// G_Printf( "skill %f spread %.2f %.2f \n", aim_skill, wi_spread_x, wi_spread_y );

	// if the bots should be really challenging
	if (bot_challenge.integer) {
		// if the bot is really accurate and has the enemy in view for some time
		if (aim_accuracy > 0.9 && bs->enemysight_time < FloatTime() - 1) {
			// set the view angles directly
			if (bs->ideal_viewangles[PITCH] > 180)
				bs->ideal_viewangles[PITCH] -= 360;
			VectorCopy(bs->ideal_viewangles, bs->viewangles);
			trap_EA_View(bs->client, bs->viewangles);
		}
	}
}

/*
==================
BotCheckAttack
==================
*/
void BotCheckAttack(bot_state_t *bs) {
	float points, reactiontime, fov, firethrottle;
	int attackentity;
	bsp_trace_t bsptrace;
	// float selfpreservation;
	vec3_t forward, right, start, end, dir, angles, miss;
	weaponinfo_t wi;
	bsp_trace_t trace;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};

	attackentity = bs->enemy;

	reactiontime = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1);
	if (bs->enemysight_time > FloatTime() - reactiontime)
		return;
	if (bs->teleport_time > FloatTime() - reactiontime)
		return;
	// if changing weapons
	if (bs->weaponchange_time > FloatTime() - 0.1)
		return;

	if (bs->weaponnum != WP_IMPERIUS) {
		// check fire throttle characteristic
		if (bs->firethrottlewait_time > FloatTime()) {
			return;
		}
		firethrottle = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_FIRETHROTTLE, 0, 1);
		if (bs->firethrottleshoot_time < FloatTime()) {
			if (random() > firethrottle) {
				bs->firethrottlewait_time = FloatTime() + firethrottle;
				bs->firethrottleshoot_time = 0;
			} else {
				bs->firethrottleshoot_time = FloatTime() + 1 - firethrottle;
				bs->firethrottlewait_time = 0;
			}
		}
	}
	//
	//
	VectorSubtract(bs->aimtarget, bs->eye, dir);
	//
	if (bs->weaponnum == WP_PUNCHY) {

		if (VectorLengthSquared(dir) > Square(60)) {
			return;
		}
	}
	if (VectorLengthSquared(dir) < Square(100))
		fov = 120;
	else
		fov = 50;
	//
	vectoangles(dir, angles);
	if (!InFieldOfVision(bs->viewangles, fov, angles))
		return;
	BotAI_Trace(&bsptrace, bs->eye, NULL, NULL, bs->aimtarget, bs->client, CONTENTS_SOLID | CONTENTS_PLAYERCLIP);
	if (bsptrace.fraction < 1 && bsptrace.ent != attackentity) {
		if (IsBambam(bs->enemy) || IsBoomie(bs->enemy)) {
			VectorScale(dir, 1.0f - bsptrace.fraction, miss);
			if (VectorLengthSquared(miss) > Square(30))
				return;
		} else
			return;
	}

	// get the weapon info
	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	// get the start point shooting from
	VectorCopy(bs->origin, start);
	start[2] += bs->cur_ps.viewheight;
	AngleVectors(bs->viewangles, forward, right, NULL);
	start[0] += forward[0] * wi.offset[0] + right[0] * wi.offset[1];
	start[1] += forward[1] * wi.offset[0] + right[1] * wi.offset[1];
	start[2] += forward[2] * wi.offset[0] + right[2] * wi.offset[1] + wi.offset[2];
	// end point aiming at
	VectorMA(start, 1000, forward, end);
	// a little back to make sure not inside a very close enemy
	VectorMA(start, -12, forward, start);
	BotAI_Trace(&trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT);
	// if the entity is a client
	if (trace.ent >= 0 && trace.ent < MAX_CLIENTS) {
		if (trace.ent != attackentity) {
			// if a teammate is hit
			if (BotSameTeam(bs, trace.ent))
				return;
		}
	}
	// if won't hit the enemy or not attacking a player (obelisk)
	if (trace.ent != attackentity || attackentity >= MAX_CLIENTS) {
		// if the projectile does radial damage
		if (wi.proj.damagetype & DAMAGETYPE_RADIAL) {
			if (trace.fraction * 1000 < wi.proj.radius) {
				points = (wi.proj.damage - 0.5 * trace.fraction * 1000) * 0.5;
				if (points > 0) {
					return;
				}
			}
			// FIXME: check if a teammate gets radial damage
		}
	}

	// cyr_zIMP {
	if (wi.number == WP_IMPERIUS) {
		// trace expected fire time...
		if (bs->cur_ps.eFlags & EF_CHARGED) {
			// release fire button...shoot
		} else { // charge
			trap_EA_Attack(bs->client);
			// G_Printf("^1 loading %d\n",bs->cur_ps.weaponTime);
		}
	} else // shoot
		trap_EA_Attack(bs->client);
	// cyr_zIMP }
}

/*
CheckSplash -- trace 800 units in direction of view, return fraction

BotCheckChargedImp
-- test if weapon is IMP and status is CHARGED
-- get optimal target pos with vecMA
-- check splash for 4 angles:
		opti
		opti - 15
		opti + 10
		75 degree up()
  choose the one with the least splash dmg
*/
// cyr_zIMP {
float CheckSplash(bot_state_t *bs, weaponinfo_t *wi, vec3_t view) {
	vec3_t start, end, forward, right;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};
	bsp_trace_t trace;

	VectorCopy(bs->origin, start);
	start[2] += bs->cur_ps.viewheight;
	AngleVectors(view, forward, right, NULL);
	start[0] += forward[0] * wi->offset[0] + right[0] * wi->offset[1];
	start[1] += forward[1] * wi->offset[0] + right[1] * wi->offset[1];
	start[2] += forward[2] * wi->offset[0] + right[2] * wi->offset[1] + wi->offset[2];
	// test for imp splash distance
	VectorMA(start, 800, forward, end);
	// a little back to make sure not inside a very close enemy
	VectorMA(start, -12, forward, start);
	BotAI_Trace(&trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT);
	return trace.fraction;
}

qboolean BotCheckChargedImp(bot_state_t *bs) {
	weaponinfo_t wi;
	// int visplayers[MAX_CLIENTS];
	// int numplayers;
	aas_entityinfo_t entinfo;
	vec3_t dir, bestorigin, angdif, final; //, start, end;
	float speed;
	int bestang, dist;
	// vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};
	float selfhurt[4];

	// test auf entity-dead usw fehlt

	// get the weapon info
	if (!bs->inventory[bs->weaponnum]) // maybe BotChooseWeapon has not yet been called
		return qfalse;
	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	// no imp or not charged
	if (wi.number != WP_IMPERIUS || !(bs->cur_ps.eFlags & EF_CHARGED))
		return qfalse;

	// BotAimAtEnemy(bs);
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid)
		return qfalse;

	VectorSubtract(entinfo.origin, bs->origin, dir);
	// distance towards the enemy
	dist = VectorLength(dir);
	// direction the enemy is moving in
	VectorSubtract(entinfo.origin, entinfo.lastvisorigin, dir);
	dir[2] = 0;
	//
	speed = VectorNormalize(dir) / entinfo.update_time;
	// botimport.Print(PRT_MESSAGE, "speed = %f, wi->speed = %f\n", speed, wi->speed);
	// best spot to aim at
	VectorMA(entinfo.origin, (dist / wi.speed) * speed, dir, bestorigin);

	VectorSubtract(bestorigin, bs->eye, dir);
	vectoangles(dir, bs->viewangles);
	bs->ideal_viewangles[PITCH] = AngleMod(bs->viewangles[PITCH]);
	bs->ideal_viewangles[YAW] = AngleMod(bs->viewangles[YAW]);
	// thats it, shoot

	//  test for splash
	selfhurt[0] = CheckSplash(bs, &wi, bs->viewangles);
	if (selfhurt[0] < 0.9) {
		VectorCopy(bs->viewangles, angdif);
		angdif[0] -= 15;
		selfhurt[1] = CheckSplash(bs, &wi, angdif);
		angdif[0] += 10;
		selfhurt[2] = CheckSplash(bs, &wi, angdif);
		angdif[0] = -75;
		selfhurt[3] = CheckSplash(bs, &wi, angdif);
		// choose highest value (high distance)
		bestang = (selfhurt[0] >= selfhurt[1]) ? 0 : 1;
		bestang = (selfhurt[bestang] >= selfhurt[2]) ? bestang : 2;
		bestang = (selfhurt[bestang] >= selfhurt[3]) ? bestang : 3;
		switch (bestang) {
		case 0:
			VectorCopy(bs->viewangles, final);
			break;
		case 1:
			VectorCopy(bs->viewangles, final);
			final[0] -= 15;
			break;
		case 2:
			VectorCopy(bs->viewangles, final);
			final[0] += 10;
			break;
		case 3:
			VectorCopy(bs->viewangles, final);
			final[0] = -75;
			break;
		default:
			VectorCopy(bs->viewangles, final);
		}
		VectorCopy(final, bs->viewangles);
		trap_EA_View(bs->client, final);
	}

	return qtrue;
}
// cyr_zIMP }

/*
==================
BotMapScripts
==================
*/
void BotMapScripts(bot_state_t *bs) {
	char info[1024];
	char mapname[128];
	vec3_t one, two;

	trap_GetServerinfo(info, sizeof(info));

	strncpy(mapname, Info_ValueForKey(info, "mapname"), sizeof(mapname) - 1);
	mapname[sizeof(mapname) - 1] = '\0';

	// cyr{
	if (!Q_stricmp(mapname, "wop_padattic")) {
		if (bs->inventory[INVENTORY_SPRAYPISTOLAMMO]) {
			trap_BotAddAvoidSpot(bs->ms, outtele.origin, 100, AVOID_ALWAYS);
			VectorCopy(outtele.origin, one);
			one[0] += 100;
			trap_BotAddAvoidSpot(bs->ms, one, 100, AVOID_ALWAYS);
			VectorCopy(outtele.origin, two);
			two[0] -= 100;
			trap_BotAddAvoidSpot(bs->ms, two, 100, AVOID_ALWAYS);
		}
	}
	// cyr}
}

/*
==================
BotSetMovedir
==================
*/
static vec3_t VEC_UP = {0, -1, 0};
static vec3_t MOVEDIR_UP = {0, 0, 1};
static vec3_t VEC_DOWN = {0, -2, 0};
static vec3_t MOVEDIR_DOWN = {0, 0, -1};

void BotSetMovedir(vec3_t angles, vec3_t movedir) {
	if (VectorCompare(angles, VEC_UP)) {
		VectorCopy(MOVEDIR_UP, movedir);
	} else if (VectorCompare(angles, VEC_DOWN)) {
		VectorCopy(MOVEDIR_DOWN, movedir);
	} else {
		AngleVectors(angles, movedir, NULL, NULL);
	}
}

/*
==================
BotModelMinsMaxs

this is ugly
==================
*/
int BotModelMinsMaxs(int modelindex, int eType, int contents, vec3_t mins, vec3_t maxs) {
	gentity_t *ent;
	int i;

	ent = &g_entities[0];
	for (i = 0; i < level.num_entities; i++, ent++) {
		if (!ent->inuse) {
			continue;
		}
		if (eType && ent->s.eType != eType) {
			continue;
		}
		if (contents && ent->r.contents != contents) {
			continue;
		}
		if (ent->s.modelindex == modelindex) {
			if (mins)
				VectorAdd(ent->r.currentOrigin, ent->r.mins, mins);
			if (maxs)
				VectorAdd(ent->r.currentOrigin, ent->r.maxs, maxs);
			return i;
		}
	}
	if (mins)
		VectorClear(mins);
	if (maxs)
		VectorClear(maxs);
	return 0;
}

/*
==================
BotFuncButtonGoal
==================
*/
int BotFuncButtonActivateGoal(bot_state_t *bs, int bspent, bot_activategoal_t *activategoal) {
	int i, areas[10], numareas, modelindex, entitynum;
	char model[128];
	float lip, dist, health, angle;
	vec3_t size, start, end, mins, maxs, angles, points[10];
	vec3_t movedir, origin, goalorigin, bboxmins, bboxmaxs;
	vec3_t extramins = {1, 1, 1}, extramaxs = {-1, -1, -1};
	bsp_trace_t bsptrace;

	activategoal->shoot = qfalse;
	VectorClear(activategoal->target);
	// create a bot goal towards the button
	trap_AAS_ValueForBSPEpairKey(bspent, "model", model, sizeof(model));
	if (!*model)
		return qfalse;
	modelindex = atoi(model + 1);
	if (!modelindex)
		return qfalse;
	VectorClear(angles);
	entitynum = BotModelMinsMaxs(modelindex, ET_MOVER, 0, mins, maxs);
	// get the lip of the button
	trap_AAS_FloatForBSPEpairKey(bspent, "lip", &lip);
	if (!lip)
		lip = 4;
	// get the move direction from the angle
	trap_AAS_FloatForBSPEpairKey(bspent, "angle", &angle);
	VectorSet(angles, 0, angle, 0);
	BotSetMovedir(angles, movedir);
	// button size
	VectorSubtract(maxs, mins, size);
	// button origin
	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5, origin);
	// touch distance of the button
	dist = fabs(movedir[0]) * size[0] + fabs(movedir[1]) * size[1] + fabs(movedir[2]) * size[2];
	dist *= 0.5;
	//
	trap_AAS_FloatForBSPEpairKey(bspent, "health", &health);
	// if the button is shootable
	if (health) {
		// calculate the shoot target
		VectorMA(origin, -dist, movedir, goalorigin);
		//
		VectorCopy(goalorigin, activategoal->target);
		activategoal->shoot = qtrue;
		//
		BotAI_Trace(&bsptrace, bs->eye, NULL, NULL, goalorigin, bs->entitynum, MASK_SHOT);
		// if the button is visible from the current position
		if (bsptrace.fraction >= 1.0 || bsptrace.ent == entitynum) {
			//
			activategoal->goal.entitynum = entitynum; // NOTE: this is the entity number of the shootable button
			activategoal->goal.number = 0;
			activategoal->goal.flags = 0;
			VectorCopy(bs->origin, activategoal->goal.origin);
			activategoal->goal.areanum = bs->areanum;
			VectorSet(activategoal->goal.mins, -8, -8, -8);
			VectorSet(activategoal->goal.maxs, 8, 8, 8);
			//
			return qtrue;
		} else {
			// create a goal from where the button is visible and shoot at the button from there
			// add bounding box size to the dist
			trap_AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, bboxmins, bboxmaxs);
			for (i = 0; i < 3; i++) {
				if (movedir[i] < 0)
					dist += fabs(movedir[i]) * fabs(bboxmaxs[i]);
				else
					dist += fabs(movedir[i]) * fabs(bboxmins[i]);
			}
			// calculate the goal origin
			VectorMA(origin, -dist, movedir, goalorigin);
			//
			VectorCopy(goalorigin, start);
			start[2] += 24;
			VectorCopy(start, end);
			end[2] -= 512;
			numareas = trap_AAS_TraceAreas(start, end, areas, points, 10);
			//
			for (i = numareas - 1; i >= 0; i--) {
				if (trap_AAS_AreaReachability(areas[i])) {
					break;
				}
			}
			if (i < 0) {
				// FIXME: trace forward and maybe in other directions to find a valid area
			}
			if (i >= 0) {
				//
				VectorCopy(points[i], activategoal->goal.origin);
				activategoal->goal.areanum = areas[i];
				VectorSet(activategoal->goal.mins, 8, 8, 8);
				VectorSet(activategoal->goal.maxs, -8, -8, -8);
				//
				for (i = 0; i < 3; i++) {
					if (movedir[i] < 0)
						activategoal->goal.maxs[i] += fabs(movedir[i]) * fabs(extramaxs[i]);
					else
						activategoal->goal.mins[i] += fabs(movedir[i]) * fabs(extramins[i]);
				} // end for
				//
				activategoal->goal.entitynum = entitynum;
				activategoal->goal.number = 0;
				activategoal->goal.flags = 0;
				return qtrue;
			}
		}
		return qfalse;
	} else {
		// add bounding box size to the dist
		trap_AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, bboxmins, bboxmaxs);
		for (i = 0; i < 3; i++) {
			if (movedir[i] < 0)
				dist += fabs(movedir[i]) * fabs(bboxmaxs[i]);
			else
				dist += fabs(movedir[i]) * fabs(bboxmins[i]);
		}
		// calculate the goal origin
		VectorMA(origin, -dist, movedir, goalorigin);
		//
		VectorCopy(goalorigin, start);
		start[2] += 24;
		VectorCopy(start, end);
		end[2] -= 100;
		numareas = trap_AAS_TraceAreas(start, end, areas, NULL, 10);
		//
		for (i = 0; i < numareas; i++) {
			if (trap_AAS_AreaReachability(areas[i])) {
				break;
			}
		}
		if (i < numareas) {
			//
			VectorCopy(origin, activategoal->goal.origin);
			activategoal->goal.areanum = areas[i];
			VectorSubtract(mins, origin, activategoal->goal.mins);
			VectorSubtract(maxs, origin, activategoal->goal.maxs);
			//
			for (i = 0; i < 3; i++) {
				if (movedir[i] < 0)
					activategoal->goal.maxs[i] += fabs(movedir[i]) * fabs(extramaxs[i]);
				else
					activategoal->goal.mins[i] += fabs(movedir[i]) * fabs(extramins[i]);
			} // end for
			//
			activategoal->goal.entitynum = entitynum;
			activategoal->goal.number = 0;
			activategoal->goal.flags = 0;
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
BotFuncDoorGoal
==================
*/
int BotFuncDoorActivateGoal(bot_state_t *bs, int bspent, bot_activategoal_t *activategoal) {
	int modelindex, entitynum;
	char model[MAX_INFO_STRING];
	vec3_t mins, maxs, origin, angles;

	// shoot at the shootable door
	trap_AAS_ValueForBSPEpairKey(bspent, "model", model, sizeof(model));
	if (!*model)
		return qfalse;
	modelindex = atoi(model + 1);
	if (!modelindex)
		return qfalse;
	VectorClear(angles);
	entitynum = BotModelMinsMaxs(modelindex, ET_MOVER, 0, mins, maxs);
	// door origin
	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5, origin);
	VectorCopy(origin, activategoal->target);
	activategoal->shoot = qtrue;
	//
	activategoal->goal.entitynum = entitynum; // NOTE: this is the entity number of the shootable door
	activategoal->goal.number = 0;
	activategoal->goal.flags = 0;
	VectorCopy(bs->origin, activategoal->goal.origin);
	activategoal->goal.areanum = bs->areanum;
	VectorSet(activategoal->goal.mins, -8, -8, -8);
	VectorSet(activategoal->goal.maxs, 8, 8, 8);
	return qtrue;
}

/*
==================
BotTriggerMultipleGoal
==================
*/
int BotTriggerMultipleActivateGoal(bot_state_t *bs, int bspent, bot_activategoal_t *activategoal) {
	int i, areas[10], numareas, modelindex, entitynum;
	char model[128];
	vec3_t start, end, mins, maxs, angles;
	vec3_t origin, goalorigin;

	activategoal->shoot = qfalse;
	VectorClear(activategoal->target);
	// create a bot goal towards the trigger
	trap_AAS_ValueForBSPEpairKey(bspent, "model", model, sizeof(model));
	if (!*model)
		return qfalse;
	modelindex = atoi(model + 1);
	if (!modelindex)
		return qfalse;
	VectorClear(angles);
	entitynum = BotModelMinsMaxs(modelindex, 0, CONTENTS_TRIGGER, mins, maxs);
	// trigger origin
	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5, origin);
	VectorCopy(origin, goalorigin);
	//
	VectorCopy(goalorigin, start);
	start[2] += 24;
	VectorCopy(start, end);
	end[2] -= 100;
	numareas = trap_AAS_TraceAreas(start, end, areas, NULL, 10);
	//
	for (i = 0; i < numareas; i++) {
		if (trap_AAS_AreaReachability(areas[i])) {
			break;
		}
	}
	if (i < numareas) {
		VectorCopy(origin, activategoal->goal.origin);
		activategoal->goal.areanum = areas[i];
		VectorSubtract(mins, origin, activategoal->goal.mins);
		VectorSubtract(maxs, origin, activategoal->goal.maxs);
		//
		activategoal->goal.entitynum = entitynum;
		activategoal->goal.number = 0;
		activategoal->goal.flags = 0;
		return qtrue;
	}
	return qfalse;
}

/*
==================
BotPopFromActivateGoalStack
==================
*/
int BotPopFromActivateGoalStack(bot_state_t *bs) {
	if (!bs->activatestack)
		return qfalse;
	BotEnableActivateGoalAreas(bs->activatestack, qtrue);
	bs->activatestack->inuse = qfalse;
	bs->activatestack->justused_time = FloatTime();
	bs->activatestack = bs->activatestack->next;
	return qtrue;
}

/*
==================
BotPushOntoActivateGoalStack
==================
*/
int BotPushOntoActivateGoalStack(bot_state_t *bs, bot_activategoal_t *activategoal) {
	int i, best;
	float besttime;

	best = -1;
	besttime = FloatTime() + 9999;
	//
	for (i = 0; i < MAX_ACTIVATESTACK; i++) {
		if (!bs->activategoalheap[i].inuse) {
			if (bs->activategoalheap[i].justused_time < besttime) {
				besttime = bs->activategoalheap[i].justused_time;
				best = i;
			}
		}
	}
	if (best != -1) {
		memcpy(&bs->activategoalheap[best], activategoal, sizeof(bot_activategoal_t));
		bs->activategoalheap[best].inuse = qtrue;
		bs->activategoalheap[best].next = bs->activatestack;
		bs->activatestack = &bs->activategoalheap[best];
		return qtrue;
	}
	return qfalse;
}

/*
==================
BotClearActivateGoalStack
==================
*/
void BotClearActivateGoalStack(bot_state_t *bs) {
	while (bs->activatestack)
		BotPopFromActivateGoalStack(bs);
}

/*
==================
BotEnableActivateGoalAreas
==================
*/
void BotEnableActivateGoalAreas(bot_activategoal_t *activategoal, int enable) {
	int i;

	if (activategoal->areasdisabled == !enable)
		return;
	for (i = 0; i < activategoal->numareas; i++)
		trap_AAS_EnableRoutingArea(activategoal->areas[i], enable);
	activategoal->areasdisabled = !enable;
}

/*
==================
BotIsGoingToActivateEntity
==================
*/
int BotIsGoingToActivateEntity(bot_state_t *bs, int entitynum) {
	bot_activategoal_t *a;
	int i;

	for (a = bs->activatestack; a; a = a->next) {
		if (a->time < FloatTime())
			continue;
		if (a->goal.entitynum == entitynum)
			return qtrue;
	}
	for (i = 0; i < MAX_ACTIVATESTACK; i++) {
		if (bs->activategoalheap[i].inuse)
			continue;
		//
		if (bs->activategoalheap[i].goal.entitynum == entitynum) {
			// if the bot went for this goal less than 2 seconds ago
			if (bs->activategoalheap[i].justused_time > FloatTime() - 2)
				return qtrue;
		}
	}
	return qfalse;
}

/*
==================
BotGetActivateGoal

  returns the number of the bsp entity to activate
  goal->entitynum will be set to the game entity to activate
==================
*/
//#define OBSTACLEDEBUG

int BotGetActivateGoal(bot_state_t *bs, int entitynum, bot_activategoal_t *activategoal) {
	int i, ent, cur_entities[10], spawnflags, modelindex, areas[MAX_ACTIVATEAREAS * 2], numareas, t;
	char model[MAX_INFO_STRING], tmpmodel[128];
	char target[128], classname[128];
	float health;
	char targetname[10][128];
	aas_entityinfo_t entinfo;
	aas_areainfo_t areainfo;
	vec3_t origin, angles, absmins, absmaxs;

	memset(activategoal, 0, sizeof(bot_activategoal_t));
	BotEntityInfo(entitynum, &entinfo);
	if (!entinfo.valid)
		return 0;

	Com_sprintf(model, sizeof(model), "*%d", entinfo.modelindex);
	for (ent = trap_AAS_NextBSPEntity(0); ent; ent = trap_AAS_NextBSPEntity(ent)) {
		if (!trap_AAS_ValueForBSPEpairKey(ent, "model", tmpmodel, sizeof(tmpmodel)))
			continue;
		if (!strcmp(model, tmpmodel))
			break;
	}
	if (!ent) {
		BotAI_Print(PRT_ERROR, "BotGetActivateGoal: no entity found with model %s\n", model);
		return 0;
	}
	trap_AAS_ValueForBSPEpairKey(ent, "classname", classname, sizeof(classname));
	if (!*classname) {
		BotAI_Print(PRT_ERROR, "BotGetActivateGoal: entity with model %s has no classname\n", model);
		return 0;
	}
	// if it is a door
	if (!strcmp(classname, "func_door")) {
		if (trap_AAS_FloatForBSPEpairKey(ent, "health", &health)) {
			// if the door has health then the door must be shot to open
			if (health) {
				BotFuncDoorActivateGoal(bs, ent, activategoal);
				return ent;
			}
		}
		//
		trap_AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);
		// if the door starts open then just wait for the door to return
		if (spawnflags & 1)
			return 0;
		// get the door origin
		if (!trap_AAS_VectorForBSPEpairKey(ent, "origin", origin)) {
			VectorClear(origin);
		}
		// if the door is open or opening already
		if (!VectorCompare(origin, entinfo.origin))
			return 0;
		// store all the areas the door is in
		trap_AAS_ValueForBSPEpairKey(ent, "model", model, sizeof(model));
		if (*model) {
			modelindex = atoi(model + 1);
			if (modelindex) {
				VectorClear(angles);
				BotModelMinsMaxs(modelindex, ET_MOVER, 0, absmins, absmaxs);
				//
				numareas = trap_AAS_BBoxAreas(absmins, absmaxs, areas, MAX_ACTIVATEAREAS * 2);
				// store the areas with reachabilities first
				for (i = 0; i < numareas; i++) {
					if (activategoal->numareas >= MAX_ACTIVATEAREAS)
						break;
					if (!trap_AAS_AreaReachability(areas[i])) {
						continue;
					}
					trap_AAS_AreaInfo(areas[i], &areainfo);
					if (areainfo.contents & AREACONTENTS_MOVER) {
						activategoal->areas[activategoal->numareas++] = areas[i];
					}
				}
				// store any remaining areas
				for (i = 0; i < numareas; i++) {
					if (activategoal->numareas >= MAX_ACTIVATEAREAS)
						break;
					if (trap_AAS_AreaReachability(areas[i])) {
						continue;
					}
					trap_AAS_AreaInfo(areas[i], &areainfo);
					if (areainfo.contents & AREACONTENTS_MOVER) {
						activategoal->areas[activategoal->numareas++] = areas[i];
					}
				}
			}
		}
	}
	// if the bot is blocked by or standing on top of a button
	if (!strcmp(classname, "func_button")) {
		return 0;
	}
	// get the targetname so we can find an entity with a matching target
	if (!trap_AAS_ValueForBSPEpairKey(ent, "targetname", targetname[0], sizeof(targetname[0]))) {
		if (bot_developer.integer) {
			BotAI_Print(PRT_ERROR, "BotGetActivateGoal: entity with model \"%s\" has no targetname\n", model);
		}
		return 0;
	}
	// allow tree-like activation
	cur_entities[0] = trap_AAS_NextBSPEntity(0);
	for (i = 0; i >= 0 && i < 10;) {
		for (ent = cur_entities[i]; ent; ent = trap_AAS_NextBSPEntity(ent)) {
			if (!trap_AAS_ValueForBSPEpairKey(ent, "target", target, sizeof(target)))
				continue;
			if (!strcmp(targetname[i], target)) {
				cur_entities[i] = trap_AAS_NextBSPEntity(ent);
				break;
			}
		}
		if (!ent) {
			if (bot_developer.integer) {
				BotAI_Print(PRT_ERROR, "BotGetActivateGoal: no entity with target \"%s\"\n", targetname[i]);
			}
			i--;
			continue;
		}
		if (!trap_AAS_ValueForBSPEpairKey(ent, "classname", classname, sizeof(classname))) {
			if (bot_developer.integer) {
				BotAI_Print(PRT_ERROR, "BotGetActivateGoal: entity with target \"%s\" has no classname\n",
							targetname[i]);
			}
			continue;
		}
		// BSP button model
		if (!strcmp(classname, "func_button")) {
			//
			if (!BotFuncButtonActivateGoal(bs, ent, activategoal))
				continue;
			// if the bot tries to activate this button already
			if (bs->activatestack && bs->activatestack->inuse &&
				bs->activatestack->goal.entitynum == activategoal->goal.entitynum &&
				bs->activatestack->time > FloatTime() && bs->activatestack->start_time < FloatTime() - 2)
				continue;
			// if the bot is in a reachability area
			if (trap_AAS_AreaReachability(bs->areanum)) {
				// disable all areas the blocking entity is in
				BotEnableActivateGoalAreas(activategoal, qfalse);
				//
				t = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, activategoal->goal.areanum, bs->tfl);
				// if the button is not reachable
				if (!t) {
					continue;
				}
				activategoal->time = FloatTime() + t * 0.01 + 5;
			}
			return ent;
		}
		// invisible trigger multiple box
		else if (!strcmp(classname, "trigger_multiple")) {
			//
			if (!BotTriggerMultipleActivateGoal(bs, ent, activategoal))
				continue;
			// if the bot tries to activate this trigger already
			if (bs->activatestack && bs->activatestack->inuse &&
				bs->activatestack->goal.entitynum == activategoal->goal.entitynum &&
				bs->activatestack->time > FloatTime() && bs->activatestack->start_time < FloatTime() - 2)
				continue;
			// if the bot is in a reachability area
			if (trap_AAS_AreaReachability(bs->areanum)) {
				// disable all areas the blocking entity is in
				BotEnableActivateGoalAreas(activategoal, qfalse);
				//
				t = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, activategoal->goal.areanum, bs->tfl);
				// if the trigger is not reachable
				if (!t) {
					continue;
				}
				activategoal->time = FloatTime() + t * 0.01 + 5;
			}
			return ent;
		} else if (!strcmp(classname, "func_timer")) {
			// just skip the func_timer
			continue;
		}
		// the actual button or trigger might be linked through a target_relay or target_delay
		else if (!strcmp(classname, "target_relay") || !strcmp(classname, "target_delay")) {
			if (trap_AAS_ValueForBSPEpairKey(ent, "targetname", targetname[i + 1], sizeof(targetname[0]))) {
				i++;
				cur_entities[i] = trap_AAS_NextBSPEntity(0);
			}
		}
	}
#ifdef OBSTACLEDEBUG
	BotAI_Print(PRT_ERROR, "BotGetActivateGoal: no valid activator for entity with target \"%s\"\n", targetname[0]);
#endif
	return 0;
}

/*
==================
BotGoForActivateGoal
==================
*/
int BotGoForActivateGoal(bot_state_t *bs, bot_activategoal_t *activategoal) {
	aas_entityinfo_t activateinfo;

	activategoal->inuse = qtrue;
	if (!activategoal->time)
		activategoal->time = FloatTime() + 10;
	activategoal->start_time = FloatTime();
	BotEntityInfo(activategoal->goal.entitynum, &activateinfo);
	if (!activateinfo.valid) {
		AIEnter_Seek_LTG(bs, "ActivateGoal: goal ent invalid");
		return qtrue;
	}

	VectorCopy(activateinfo.origin, activategoal->origin);
	//
	if (BotPushOntoActivateGoalStack(bs, activategoal)) {
		// enter the activate entity AI node
		AIEnter_Seek_ActivateEntity(bs, "BotGoForActivateGoal");
		return qtrue;
	} else {
		// enable any routing areas that were disabled
		BotEnableActivateGoalAreas(activategoal, qtrue);
		return qfalse;
	}
}

/*
==================
BotPrintActivateGoalInfo
==================
*/
void BotPrintActivateGoalInfo(bot_state_t *bs, bot_activategoal_t *activategoal, int bspent) {
	char netname[MAX_NETNAME];
	char classname[128];
	char buf[128];

	ClientName(bs->client, netname, sizeof(netname));
	trap_AAS_ValueForBSPEpairKey(bspent, "classname", classname, sizeof(classname));
	if (activategoal->shoot) {
		Com_sprintf(buf, sizeof(buf), "%s: I have to shoot at a %s from %1.1f %1.1f %1.1f in area %d\n", netname,
					classname, activategoal->goal.origin[0], activategoal->goal.origin[1], activategoal->goal.origin[2],
					activategoal->goal.areanum);
	} else {
		Com_sprintf(buf, sizeof(buf), "%s: I have to activate a %s at %1.1f %1.1f %1.1f in area %d\n", netname,
					classname, activategoal->goal.origin[0], activategoal->goal.origin[1], activategoal->goal.origin[2],
					activategoal->goal.areanum);
	}
	trap_EA_Say(bs->client, buf);
}

/*
==================
BotRandomMove
==================
*/
void BotRandomMove(bot_state_t *bs, bot_moveresult_t *moveresult) {
	vec3_t dir, angles;

	angles[0] = 0;
	angles[1] = random() * 360;
	angles[2] = 0;
	AngleVectors(angles, dir, NULL, NULL);

	trap_BotMoveInDirection(bs->ms, dir, 400, MOVE_WALK);

	moveresult->failure = qfalse;
	VectorCopy(dir, moveresult->movedir);
}

/*
==================
BotAIBlocked

Very basic handling of bots being blocked by other entities.
Check what kind of entity is blocking the bot and try to activate
it. If that's not an option then try to walk around or over the entity.
Before the bot ends in this part of the AI it should predict which doors to
open, which buttons to activate etc.
==================
*/
void BotAIBlocked(bot_state_t *bs, bot_moveresult_t *moveresult, int activate) {
#ifdef OBSTACLEDEBUG
	char netname[MAX_NETNAME];
#endif
	int movetype, bspent;
	vec3_t hordir, sideward, angles, up = {0, 0, 1};
	// vec3_t start, end, mins, maxs;
	aas_entityinfo_t entinfo;
	bot_activategoal_t activategoal;

	// if the bot is not blocked by anything
	if (!moveresult->blocked) {
		bs->notblocked_time = FloatTime();
		return;
	}
	// if stuck in a solid area
	if (moveresult->type == RESULTTYPE_INSOLIDAREA) {
		// move in a random direction in the hope to get out
		BotRandomMove(bs, moveresult);
		//
		return;
	}
	// get info for the entity that is blocking the bot
	BotEntityInfo(moveresult->blockentity, &entinfo);
	if (!entinfo.valid)
		return;

#ifdef OBSTACLEDEBUG
	ClientName(bs->client, netname, sizeof(netname));
	BotAI_Print(PRT_MESSAGE, "%s: I'm blocked by model %d\n", netname, entinfo.modelindex);
#endif // OBSTACLEDEBUG
	// if blocked by a bsp model and the bot wants to activate it
	if (activate && entinfo.modelindex > 0 && entinfo.modelindex <= max_bspmodelindex) {
		// find the bsp entity which should be activated in order to get the blocking entity out of the way
		bspent = BotGetActivateGoal(bs, entinfo.number, &activategoal);
		if (bspent) {
			//
			if (bs->activatestack && !bs->activatestack->inuse)
				bs->activatestack = NULL;
			// if not already trying to activate this entity
			if (!BotIsGoingToActivateEntity(bs, activategoal.goal.entitynum)) {
				//
				BotGoForActivateGoal(bs, &activategoal);
			}
			// if ontop of an obstacle or
			// if the bot is not in a reachability area it'll still
			// need some dynamic obstacle avoidance, otherwise return
			if (!(moveresult->flags & MOVERESULT_ONTOPOFOBSTACLE) && trap_AAS_AreaReachability(bs->areanum))
				return;
		} else {
			// enable any routing areas that were disabled
			BotEnableActivateGoalAreas(&activategoal, qtrue);
		}
	}
	// just some basic dynamic obstacle avoidance code
	hordir[0] = moveresult->movedir[0];
	hordir[1] = moveresult->movedir[1];
	hordir[2] = 0;
	// if no direction just take a random direction
	if (VectorNormalize(hordir) < 0.1) {
		VectorSet(angles, 0, 360 * random(), 0);
		AngleVectors(angles, hordir, NULL, NULL);
	}
	//
	// if (moveresult->flags & MOVERESULT_ONTOPOFOBSTACLE) movetype = MOVE_JUMP;
	// else
	movetype = MOVE_WALK;
	// if there's an obstacle at the bot's feet and head then
	// the bot might be able to crouch through
	// VectorCopy(bs->origin, start);
	// start[2] += 18;
	// VectorMA(start, 5, hordir, end);
	// VectorSet(mins, -16, -16, -24);
	// VectorSet(maxs, 16, 16, 4);
	//
	// bsptrace = AAS_Trace(start, mins, maxs, end, bs->entitynum, MASK_PLAYERSOLID);
	// if (bsptrace.fraction >= 1) movetype = MOVE_CROUCH;
	// get the sideward vector
	CrossProduct(hordir, up, sideward);
	//
	if (bs->flags & BFL_AVOIDRIGHT)
		VectorNegate(sideward, sideward);
	// try to crouch straight forward?
	if (movetype != MOVE_CROUCH || !trap_BotMoveInDirection(bs->ms, hordir, 400, movetype)) {
		// perform the movement
		if (!trap_BotMoveInDirection(bs->ms, sideward, 400, movetype)) {
			// flip the avoid direction flag
			bs->flags ^= BFL_AVOIDRIGHT;
			// flip the direction
			// VectorNegate(sideward, sideward);
			VectorMA(sideward, -1, hordir, sideward);
			// move in the other direction
			trap_BotMoveInDirection(bs->ms, sideward, 400, movetype);
		}
	}
	//
	if (bs->notblocked_time < FloatTime() - 0.4) {
		// just reset goals and hope the bot will go into another direction?
		// is this still needed??
		if (bs->ainode == AINode_Seek_NBG)
			bs->nbg_time = 0;
		else if (bs->ainode == AINode_Seek_LTG)
			bs->ltg_time = 0;
	}
}

/*
==================
BotAIPredictObstacles

Predict the route towards the goal and check if the bot
will be blocked by certain obstacles. When the bot has obstacles
on its path the bot should figure out if they can be removed
by activating certain entities.
==================
*/
int BotAIPredictObstacles(bot_state_t *bs, bot_goal_t *goal) {
	int modelnum, entitynum, bspent;
	bot_activategoal_t activategoal;
	aas_predictroute_t route;

	if (!bot_predictobstacles.integer)
		return qfalse;

	// always predict when the goal change or at regular intervals
	if (bs->predictobstacles_goalareanum == goal->areanum && bs->predictobstacles_time > FloatTime() - 6) {
		return qfalse;
	}
	bs->predictobstacles_goalareanum = goal->areanum;
	bs->predictobstacles_time = FloatTime();

	// predict at most 100 areas or 10 seconds ahead
	trap_AAS_PredictRoute(&route, bs->areanum, bs->origin, goal->areanum, bs->tfl, 100, 1000,
						  RSE_USETRAVELTYPE | RSE_ENTERCONTENTS, AREACONTENTS_MOVER, TFL_BRIDGE, 0);
	if (bot_developer.integer & AIDBG_MOVE) {
		DeleteDebugLines();
		DebugLineDouble(bs->origin, route.endpos, 3);
	}
	// if bot has to travel through an area with a mover
	if (route.stopevent & RSE_ENTERCONTENTS) {
		// if the bot will run into a mover
		if (route.endcontents & AREACONTENTS_MOVER) {
			// NOTE: this only works with bspc 2.1 or higher
			modelnum = (route.endcontents & AREACONTENTS_MODELNUM) >> AREACONTENTS_MODELNUMSHIFT;
			if (modelnum) {
				//
				entitynum = BotModelMinsMaxs(modelnum, ET_MOVER, 0, NULL, NULL);
				if (entitynum) {
					// NOTE: BotGetActivateGoal already checks if the door is open or not
					bspent = BotGetActivateGoal(bs, entitynum, &activategoal);
					if (bspent) {
						//
						if (bs->activatestack && !bs->activatestack->inuse)
							bs->activatestack = NULL;
						// if not already trying to activate this entity
						if (!BotIsGoingToActivateEntity(bs, activategoal.goal.entitynum)) {
							//
							// BotAI_Print(PRT_MESSAGE, "blocked by mover model %d, entity %d ?\n", modelnum,
							// entitynum);
							//
							BotGoForActivateGoal(bs, &activategoal);
							return qtrue;
						} else {
							// enable any routing areas that were disabled
							BotEnableActivateGoalAreas(&activategoal, qtrue);
						}
					}
				}
			}
		}
	} else if (route.stopevent & RSE_USETRAVELTYPE) {
		if (route.endtravelflags & TFL_BRIDGE) {
			// FIXME: check if the bridge is available to travel over
		}
	}
	return qfalse;
}

/*
==================
BotCheckConsoleMessages
==================
*/
static void BotCheckConsoleMessages(bot_state_t *bs) {
	int handle;
	bot_consolemessage_t m;

	while ((handle = trap_BotNextConsoleMessage(bs->cs, &m)) != 0) {
		BotMatchMessage(bs, m.message);
		trap_BotRemoveConsoleMessage(bs->cs, handle);
	}
}

/*
==================
BotCheckEvents
==================
*/
void BotCheckForGrenades(bot_state_t *bs, entityState_t *state) {

	// if this is not a grenade
	if (state->eType != ET_MISSILE || state->weapon != WP_BALLOONY)
		return;

	// try to avoid the grenade
	trap_BotAddAvoidSpot(bs->ms, state->pos.trBase, 160, AVOID_ALWAYS);
}

/*
==================
BotCheckEvents
==================
*/
void BotCheckEvents(bot_state_t *bs, entityState_t *state) {
	int event;
	char buf[128];

	// NOTE: this sucks, we're accessing the gentity_t directly
	// but there's no other fast way to do it right now
	if (bs->entityeventTime[state->number] == g_entities[state->number].eventTime) {
		return;
	}
	bs->entityeventTime[state->number] = g_entities[state->number].eventTime;
	// if it's an event only entity
	if (state->eType > ET_EVENTS) {
		event = (state->eType - ET_EVENTS) & ~EV_EVENT_BITS;
	} else {
		event = state->event & ~EV_EVENT_BITS;
	}
	//
	switch (event) {
	case EV_DROP_CARTRIDGE: {
		if (bs->ltgtype == LTG_FETCHCART) {
			// get event location !!!
			bs->takecart = qtrue;

			/*
			vec3_t dir;
			char* items[]={"red Cartridge","blue Cartridge"};
			bot_goal_t goal;
			float dist;

			G_Printf("^2 launched ...");	// cyr_ptr
			VectorSubtract(state->origin, bs->origin, dir);
			if(VectorLength(dir) >= PUSHCART_DIST ){
				G_Printf("too far %d :/ \n", VectorLength(dir));	// cyr_ptr
				//break;	// cyr_ptr
			}
			else G_Printf("in range ...");		// cyr_ptr


			// push droped cart in item db
			trap_BotUpdateEntityItems();
			// identify cart
			dist = BotNearestVisibleItem(bs, items[BotTeam(bs)-1], &goal );
			if(dist < 200){
				bs->takecart = qtrue;
				memcpy(&goal, &bs->teamgoal, sizeof(bot_goal_t));
				G_Printf("taking it ! \n");	// cyr_ptr
			}
			else G_Printf("not found :/ \n");	// cyr_ptr
			*/
		}
		break;
	}
	// client obituary event
	case EV_OBITUARY: {
		int target, attacker, mod;

		target = state->otherEntityNum;
		attacker = state->otherEntityNum2;
		mod = state->eventParm;
		//
		if (target == bs->client) {
			bs->botdeathtype = mod;
			bs->lastkilledby = attacker;
			//
			if (target == attacker || target == ENTITYNUM_NONE || target == ENTITYNUM_WORLD)
				bs->botsuicide = qtrue;
			else
				bs->botsuicide = qfalse;
			//
			bs->num_deaths++;
		}
		// else if this client was killed by the bot
		else if (attacker == bs->client) {
			bs->enemydeathtype = mod;
			bs->lastkilledplayer = target;
			bs->killedenemy_time = FloatTime();
			//
			bs->num_kills++;
		} else if (attacker == bs->enemy && target == attacker) {
			bs->enemysuicide = qtrue;
		}
		break;
	}
	case EV_GLOBAL_SOUND: {
		if (state->eventParm < 0 || state->eventParm > MAX_SOUNDS) {
			BotAI_Print(PRT_ERROR, "EV_GLOBAL_SOUND: eventParm (%d) out of range\n", state->eventParm);
			break;
		}
		trap_GetConfigstring(CS_SOUNDS + state->eventParm, buf, sizeof(buf));
		if (!strcmp(buf, "sounds/items/powerup_respawn")) {
			// powerup respawned... go get it
			BotGoForPowerups(bs);
		}
		break;
	}
	case EV_GLOBAL_TEAM_SOUND: {
		/*if (gametype == GT_CTF) {
			switch(state->eventParm) {
				case GTS_RED_CAPTURE:
					bs->blueflagstatus = 0;
					bs->redflagstatus = 0;
					//bs->flagstatuschanged = qtrue;
					break; //see BotMatch_CTF
				case GTS_BLUE_CAPTURE:
					bs->blueflagstatus = 0;
					bs->redflagstatus = 0;
					//bs->flagstatuschanged = qtrue;
					break; //see BotMatch_CTF
				case GTS_RED_RETURN:
					//blue flag is returned
					bs->blueflagstatus = 0;
					//bs->flagstatuschanged = qtrue;
					break;
				case GTS_BLUE_RETURN:
					//red flag is returned
					bs->redflagstatus = 0;
					//bs->flagstatuschanged = qtrue;
					break;
				case GTS_RED_TAKEN:
					//blue flag is taken
					bs->blueflagstatus = 1;
					//bs->flagstatuschanged = qtrue;
					break; //see BotMatch_CTF
				case GTS_BLUE_TAKEN:
					//red flag is taken
					bs->redflagstatus = 1;
					//bs->flagstatuschanged = qtrue;
					break; //see BotMatch_CTF
			}
		}*/
		break;
	}
	case EV_PLAYER_TELEPORT_IN: {
		VectorCopy(state->origin, lastteleport_origin);
		lastteleport_time = FloatTime();
		break;
	}
	case EV_GENERAL_SOUND: {
		// if this sound is played on the bot
		if (state->number == bs->client) {
			if (state->eventParm < 0 || state->eventParm >= MAX_SOUNDS) {
				BotAI_Print(PRT_ERROR, "EV_GENERAL_SOUND: eventParm (%d) out of range\n", state->eventParm);
				break;
			}
			// check out the sound
			trap_GetConfigstring(CS_SOUNDS + state->eventParm, buf, sizeof(buf));
			// if falling into a death pit
			if (!strcmp(buf, "*falling1")) {
				// if the bot has a personal teleporter
				if (bs->inventory[INVENTORY_TELEPORTER] > 0) {
					// use the holdable item
					trap_EA_Use(bs->client);
				}
			}
		}
		break;
	}
	case EV_FOOTSTEP:
	case EV_FOOTSTEP_METAL:
	case EV_FOOTSPLASH:
	case EV_FOOTWADE:
	case EV_SWIM:
	case EV_FALL_SHORT:
	case EV_FALL_MEDIUM:
	case EV_FALL_FAR:
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:
	case EV_JUMP_PAD:
	case EV_JUMP:
	case EV_TAUNT:

	case EV_HEHE1:
	case EV_HEHE2:

	case EV_FOOTSTEP_CARPET:
	case EV_FOOTSTEP_LATTICE:
	case EV_FOOTSTEP_SAND:
	case EV_FOOTSTEP_SOFT:
	case EV_FOOTSTEP_WOOD:

	case EV_WATER_TOUCH:
	case EV_WATER_LEAVE:
	case EV_WATER_UNDER:
	case EV_WATER_CLEAR:
	case EV_ITEM_PICKUP:
	case EV_GLOBAL_ITEM_PICKUP:
	case EV_NOAMMO:
	case EV_CHANGE_WEAPON:
	case EV_FIRE_WEAPON:
		// FIXME: either add to sound queue or mark player as someone making noise
		break;
	}
}

/*
==================
BotCheckSnapshot
==================
*/
void BotCheckSnapshot(bot_state_t *bs) {
	int ent;
	entityState_t state;

	// remove all avoid spots
	trap_BotAddAvoidSpot(bs->ms, vec3_origin, 0, AVOID_CLEAR);
	//
	ent = 0;
	while ((ent = BotAI_GetSnapshotEntity(bs->client, ent, &state)) != -1) {
		// check the entity state for events
		BotCheckEvents(bs, &state);
		// check for grenades the bot should avoid
		BotCheckForGrenades(bs, &state);
	}
	// check the player state for events
	BotAI_GetEntityState(bs->client, &state);
	// copy the player state events to the entity state
	state.event = bs->cur_ps.externalEvent;
	state.eventParm = bs->cur_ps.externalEventParm;
	//
	BotCheckEvents(bs, &state);
}

/*
==================
BotCheckAir
==================
*/
void BotCheckAir(bot_state_t *bs) {
	if (trap_AAS_PointContents(bs->eye) & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
		return;
	// we have air this frame
	bs->lastair_time = FloatTime();
}

// any gametype specific map contents missing?
qboolean MissingMapConts(void) {

	if (gametype == GT_BALLOON)
		return (level.numBalloons == 0);

	if (gametype == GT_SPRAY || gametype == GT_SPRAYFFA) {
		return (!level.sr_tele || !level.bspraywall || !level.rspraywall || !level.sr_teleout);
	}

	return qfalse;
}

/*
==================
BotDeathmatchAI
==================
*/
void BotDeathmatchAI(bot_state_t *bs, float thinktime) {
	char gender[144], name[144], buf[144];
	char userinfo[MAX_INFO_STRING];
	int i;

	// if the bot has just been setup
	if (bs->setupcount > 0) {
		bs->setupcount--;
		if (bs->setupcount > 0)
			return;
		// get the gender characteristic
		trap_Characteristic_String(bs->character, CHARACTERISTIC_GENDER, gender, sizeof(gender));
		// set the bot gender
		trap_GetUserinfo(bs->client, userinfo, sizeof(userinfo));
		Info_SetValueForKey(userinfo, "sex", gender);
		trap_SetUserinfo(bs->client, userinfo);
		// set the team
		if (!bs->map_restart && g_gametype.integer != GT_TOURNAMENT) {
			Com_sprintf(buf, sizeof(buf), "team %s", bs->settings.team);
			trap_EA_Command(bs->client, buf);
		}
		// set the chat gender
		if (gender[0] == 'm')
			trap_BotSetChatGender(bs->cs, CHAT_GENDERMALE);
		else if (gender[0] == 'f')
			trap_BotSetChatGender(bs->cs, CHAT_GENDERFEMALE);
		else
			trap_BotSetChatGender(bs->cs, CHAT_GENDERLESS);
		// set the chat name
		ClientName(bs->client, name, sizeof(name));
		trap_BotSetChatName(bs->cs, name, bs->client);
		//
		bs->lastframe_health = bs->inventory[INVENTORY_HEALTH];
		bs->lasthitcount = bs->cur_ps.persistant[PERS_HITS];
		bs->which_wall = NULL;

		bs->setupcount = 0;
	}

	// test for map - gametype mismatch
	if (MissingMapConts()) {
		return; // just stand still, dont exec any ainode
	}

	// no ideal view set
	bs->flags &= ~BFL_IDEALVIEWSET;
	//
	if (!BotIntermission(bs)) {
		// set the teleport time
		BotSetTeleportTime(bs);
		// update some inventory values
		BotUpdateInventory(bs);
		// check out the snapshot
		BotCheckSnapshot(bs);
		// check for air
		BotCheckAir(bs);
	}

	// check the console messages
	BotCheckConsoleMessages(bs);

	if (!BotIntermission(bs) && !BotIsObserver(bs)) {
		BotTeamAI(bs);
	}

	// if the bot has no ai node
	if (!bs->ainode) {
		AIEnter_Seek_LTG(bs, "BotDeathmatchAI: no ai node");
	}
	// if the bot entered the game less than 8 seconds ago
	if (!bs->entergamechat && bs->entergame_time > FloatTime() - 8) {
		if (BotChat_EnterGame(bs)) {
			bs->stand_time = FloatTime() + BotChatTime(bs);
			AIEnter_Stand(bs, "BotDeathmatchAI: chat enter game");
		}
		bs->entergamechat = qtrue;
	}
	if (level.cammode)
		bs->ainode = AINode_Cam;
	// reset the node switches from the previous frame
	BotResetNodeSwitches();
	// execute AI nodes
	for (i = 0; i < MAX_NODESWITCHES; i++) {
		if (bs->ainode(bs))
			break;
	}
	// if the bot removed itself :)
	if (!bs->inuse)
		return;
	// if the bot executed too many AI nodes
	if (i >= MAX_NODESWITCHES) {
		trap_BotDumpGoalStack(bs->gs);
		trap_BotDumpAvoidGoals(bs->gs);
		BotDumpNodeSwitches(bs);
		ClientName(bs->client, name, sizeof(name));
		BotAI_Print(PRT_ERROR, "%s at %1.1f switched more than %d AI nodes\n", name, FloatTime(), MAX_NODESWITCHES);
	}
	//
	bs->lastframe_health = bs->inventory[INVENTORY_HEALTH];
	bs->lasthitcount = bs->cur_ps.persistant[PERS_HITS];
}

void BotSelectLogo(bot_state_t *bs) {
	const char *logostring;
	char buf[300];
	char userinfo[MAX_INFO_STRING];

	// trap_Characteristic_String(bs->character, CHARACTERISTIC_SPRAYLOGO, logostring, sizeof(logostring));

	trap_GetUserinfo(bs->client, userinfo, sizeof(userinfo));
	logostring = Info_ValueForKey(userinfo, "spraylogo");

	Com_sprintf(buf, sizeof(buf), "selectlogo %s", logostring);
	trap_EA_Command(bs->client, buf);
}

#define BOXHEIGHT 54
#define BOXDEPTH 45

// alternative zB BotGoalForBSPEntity
void CheckMatrixForGoal(bot_goal_t *goal) {
	int xmin, xmax, ymin, ymax, zmin, zmax;
	int i, j, k;
	int step;
	vec3_t pos, sample, offset, bestsample = {0, 0, 0};
	int sarea, earea;
	int dist, bestdist;
	int reachable;

	VectorCopy(goal->origin, pos);
	sarea = BotPointAreaNum(pos);
	bestdist = 9999;

	zmin = zmax = BOXHEIGHT;
	xmin = xmax = ymin = ymax = BOXDEPTH; // == WIDTH
	step = 2;

	for (i = pos[0] - xmin; i < pos[0] + xmax; i += step)
		for (j = pos[1] - ymin; j < pos[1] + ymax; j += step)
			for (k = pos[2] - zmin; k < pos[2] + zmax; k += step) { // scan 3 dimensions
				VectorSet(sample, i, j, k);
				earea = BotPointAreaNum(sample);
				if (sarea != earea) { // crossed area border
					reachable = trap_AAS_AreaReachability(earea);
					if (reachable) {
						VectorSubtract(pos, sample, offset);
						dist = VectorLength(offset);
						if (dist < bestdist) {
							bestdist = dist;
							VectorCopy(sample, bestsample);
						}
					}
				}
			}

	if (bestdist < 9999) { // success
		VectorCopy(bestsample, goal->origin);
		goal->areanum = BotPointAreaNum(goal->origin);
	}
}

void FixGoalArea(bot_goal_t *goal) {
	vec3_t start, end;
	int numareas, areas[10];

	VectorCopy(goal->origin, start);
	start[2] -= 32;
	VectorCopy(goal->origin, end);
	end[2] += 32;
	numareas = trap_AAS_TraceAreas(start, end, areas, NULL, 10);
	if (numareas)
		goal->areanum = areas[0];
	else
		CheckMatrixForGoal(goal);
}

int CheckAreaForGoal(vec3_t origin) { // copy of BotFuzzyPointReachabilityArea
	int firstareanum, j, x, y, z;
	int areas[10], numareas, areanum, bestareanum;
	float dist, bestdist;
	vec3_t points[10], v, end;

	firstareanum = 0;
	areanum = trap_AAS_PointAreaNum(origin);
	if (areanum) {
		firstareanum = areanum;
		if (trap_AAS_AreaReachability(areanum)) {
			return areanum;
		}
	}
	VectorCopy(origin, end);
	end[2] += 4;
	numareas = trap_AAS_TraceAreas(origin, end, areas, points, 10);
	for (j = 0; j < numareas; j++) {
		if (trap_AAS_AreaReachability(areas[j]))
			return areas[j];
	}
	bestdist = 999999;
	bestareanum = 0;
	for (z = 1; z >= -1; z -= 1) {
		for (x = 1; x >= -1; x -= 1) {
			for (y = 1; y >= -1; y -= 1) {
				VectorCopy(origin, end);
				end[0] += x * 8;
				end[1] += y * 8;
				end[2] += z * 12;
				numareas = trap_AAS_TraceAreas(origin, end, areas, points, 10);
				for (j = 0; j < numareas; j++) {
					if (trap_AAS_AreaReachability(areas[j])) {
						VectorSubtract(points[j], origin, v);
						dist = VectorLength(v);
						if (dist < bestdist) {
							bestareanum = areas[j];
							bestdist = dist;
						}
					}
					if (!firstareanum)
						firstareanum = areas[j];
				}
			}
		}
		if (bestareanum) {
			return bestareanum;
		}
	}
	return firstareanum;
}

/*
==================
BotSetupDeathmatchAI
==================
*/
void BotSetupDeathmatchAI(void) {
	int ent, modelnum, i, j;
	char model[128];
	gentity_t *pent;

	gametype = trap_Cvar_VariableIntegerValue("g_gametype");
	maxclients = trap_Cvar_VariableIntegerValue("sv_maxclients");

	trap_Cvar_Register(&bot_rocketjump, "bot_rocketjump", "1", 0);
	trap_Cvar_Register(&bot_grapple, "bot_grapple", "0", 0);
	trap_Cvar_Register(&bot_fastchat, "bot_fastchat", "0", 0);
	trap_Cvar_Register(&bot_nochat, "bot_nochat", "0", 0);
	trap_Cvar_Register(&bot_testrchat, "bot_testrchat", "0", 0);
	trap_Cvar_Register(&bot_challenge, "bot_challenge", "0", 0);
	trap_Cvar_Register(&bot_predictobstacles, "bot_predictobstacles", "1", 0);
	trap_Cvar_Register(&g_spSkill, "g_spSkill", "2", 0);

	if (gametype == GT_CTF) {
		qboolean redInit = qfalse;
		qboolean blueInit = qfalse;

		// todo: get rid of the trapcalls and to everything in the loop below
		if (trap_BotGetLevelItemGoal(-1, "red Lolly", &ctf_redflag) < 0)
			BotAI_Print(PRT_WARNING, "CTL without Red Lolly\n");
		if (trap_BotGetLevelItemGoal(-1, "blue Lolly", &ctf_blueflag) < 0)
			BotAI_Print(PRT_WARNING, "CTL without Blue Lolly\n");

		// find the entity numbers
		pent = &g_entities[MAX_CLIENTS];
		for (i = MAX_CLIENTS; i < level.num_entities; i++, pent++) {
			if (!pent->inuse)
				continue;
			if (!Q_stricmp(pent->classname, "team_CTL_bluelolly")) {
				ctf_blueflag.entitynum = i;
				blueInit = qtrue;
			} else if (!Q_stricmp(pent->classname, "team_CTL_redlolly")) {
				ctf_redflag.entitynum = i;
				redInit = qtrue;
			}

			if (redInit && blueInit)
				break;
		}

		WaypointInit();
	}

	// find hstations
	pent = &g_entities[0];
	j = 0;

	for (i = 0; i < level.num_entities; i++, pent++) {
		if (!pent->inuse)
			continue;
		if (j == MAX_HSTATIONS)
			break;
		if (pent->s.eType == ET_STATION) {
			hstations[j].entitynum = i;
			VectorCopy(pent->s.origin, hstations[j].origin);
			hstations[j].areanum = BotPointAreaNum(hstations[j].origin);
			VectorCopy(pent->r.mins, hstations[j].mins);
			VectorCopy(pent->r.maxs, hstations[j].maxs);
			hstations[j].flags = 0; // no item, not dropped, no airgoal, not for roaming

			// adjust origin if not reachable
			if (!trap_AAS_AreaReachability(hstations[j].areanum))
				CheckMatrixForGoal(&hstations[j]);

			j++;
		}
	}
	num_hstations = j;

	if (gametype == GT_SPRAY || gametype == GT_SPRAYFFA) {

		if (level.bspraywall) {
			// blue wall
			bwall.origin[0] = (level.bspraywall->r.mins[0] + level.bspraywall->r.maxs[0]) / 2;
			bwall.origin[1] = (level.bspraywall->r.mins[1] + level.bspraywall->r.maxs[1]) / 2;
			if (level.bspraywall->r.mins[2] < level.bspraywall->r.maxs[2])
				bwall.origin[2] = level.bspraywall->r.mins[2] + 5;
			else
				bwall.origin[2] = level.bspraywall->r.maxs[2] + 5;

			bwall.entitynum = level.bspraywall->s.number;
			VectorCopy(level.bspraywall->r.mins, bwall.mins);
			VectorCopy(level.bspraywall->r.maxs, bwall.maxs);
			bwall.areanum = BotPointAreaNum(bwall.origin);
			bwall.flags = 0;
			CheckMatrixForGoal(&bwall);
		}

		if (level.rspraywall) {
			// red wall
			rwall.origin[0] = (level.rspraywall->r.mins[0] + level.rspraywall->r.maxs[0]) / 2;
			rwall.origin[1] = (level.rspraywall->r.mins[1] + level.rspraywall->r.maxs[1]) / 2;
			if (level.rspraywall->r.mins[2] < level.rspraywall->r.maxs[2])
				rwall.origin[2] = level.rspraywall->r.mins[2] + 5;
			else
				rwall.origin[2] = level.rspraywall->r.maxs[2] + 5;

			rwall.entitynum = level.rspraywall->s.number;
			VectorCopy(level.rspraywall->r.mins, rwall.mins);
			VectorCopy(level.rspraywall->r.maxs, rwall.maxs);
			rwall.areanum = BotPointAreaNum(rwall.origin);
			rwall.flags = 0;

			CheckMatrixForGoal(&rwall);
		}

		if (level.sr_tele) {
			// tele in
			spraytele.origin[0] = (level.sr_tele->r.mins[0] + level.sr_tele->r.maxs[0]) / 2;
			spraytele.origin[1] = (level.sr_tele->r.mins[1] + level.sr_tele->r.maxs[1]) / 2;
			spraytele.origin[2] = (level.sr_tele->r.mins[2] + level.sr_tele->r.maxs[2]) / 2;

			spraytele.entitynum = level.sr_tele->s.number;
			VectorCopy(level.sr_tele->r.mins, spraytele.mins);
			VectorCopy(level.sr_tele->r.maxs, spraytele.maxs);
			spraytele.areanum = BotPointAreaNum(spraytele.origin);
			spraytele.flags = 0;

			FixGoalArea(&spraytele);
			// CheckMatrixForGoal(&spraytele);
			// spraytele.areanum = CheckAreaForGoal(spraytele.origin);
		}

		if (level.sr_teleout) {
			// tele out
			outtele.origin[0] = (level.sr_teleout->r.mins[0] + level.sr_teleout->r.maxs[0]) * 0.5f;
			outtele.origin[1] = (level.sr_teleout->r.mins[1] + level.sr_teleout->r.maxs[1]) * 0.5f;
			outtele.origin[2] = level.sr_teleout->r.mins[2] + 10;
			outtele.entitynum = level.sr_teleout->s.number;
			VectorCopy(level.sr_teleout->r.mins, outtele.mins);
			VectorCopy(level.sr_teleout->r.maxs, outtele.maxs);
			outtele.areanum = BotPointAreaNum(outtele.origin);
			outtele.flags = 0;
			CheckMatrixForGoal(&outtele);
		}
	} else if (gametype == GT_BALLOON) {
		pent = &g_entities[0];
		j = 0;

		for (i = 0; i < level.num_entities; i++, pent++) {
			if (!pent->inuse)
				continue;
			if (j == MAX_BALLOONS)
				break;
			if (!Q_stricmp(pent->classname, "trigger_balloonzone")) { //"target_balloon") ) {
				balloongoal[j].entitynum = i;

				VectorCopy(pent->s.origin, balloongoal[j].origin);
				balloongoal[j].origin[0] = (pent->r.mins[0] + pent->r.maxs[0]) / 2;
				balloongoal[j].origin[1] = (pent->r.mins[1] + pent->r.maxs[1]) / 2;
				balloongoal[j].origin[2] = (pent->r.mins[2] < pent->r.maxs[2]) ? pent->r.mins[2] : pent->r.maxs[2];
				balloongoal[j].origin[2] += 32; // testing

				balloongoal[j].areanum = BotPointAreaNum(balloongoal[j].origin);
				VectorCopy(pent->r.mins, balloongoal[j].mins);
				VectorCopy(pent->r.maxs, balloongoal[j].maxs);
				balloongoal[j].flags = 0; // no item, not dropped, no airgoal, not for roaming

				// adjust origin if not reachable
				if (!trap_AAS_AreaReachability(balloongoal[j].areanum))
					CheckMatrixForGoal(&balloongoal[j]);

				j++;
			}
		}
	}

	max_bspmodelindex = 0;
	for (ent = trap_AAS_NextBSPEntity(0); ent; ent = trap_AAS_NextBSPEntity(ent)) {
		if (!trap_AAS_ValueForBSPEpairKey(ent, "model", model, sizeof(model)))
			continue;
		if (model[0] == '*') {
			modelnum = atoi(model + 1);
			if (modelnum > max_bspmodelindex)
				max_bspmodelindex = modelnum;
		}
	}
}
