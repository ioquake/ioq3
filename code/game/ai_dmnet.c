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
 * name:		ai_dmnet.c
 *
 * desc:		WoP bot AI
 *
 * $Archive: /MissionPack/code/game/ai_dmnet.c $
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
// data file headers
#include "chars.h" //characteristics
#include "inv.h"   //indexes into the inventory
#include "syn.h"   //synonyms
#include "match.h" //string matching types and vars

// goal flag, see ../botlib/be_ai_goal.h for the other GFL_*
#define GFL_AIR 128

#define LOOKAHEAD_DISTANCE 300
#define DEFENDAWAY_RANGE 1000

extern vmCvar_t bot_thinktime;

static int numnodeswitches;
static char nodeswitch[MAX_NODESWITCHES + 1][144];

extern bot_state_t *botstates[MAX_CLIENTS];

int BotClientTravelTimeToGoal(int client, bot_goal_t *goal) {
	playerState_t ps;
	int areanum;

	BotAI_GetClientState(client, &ps);
	areanum = BotPointAreaNum(ps.origin);
	if (!areanum)
		return 1;
	return trap_AAS_AreaTravelTimeToGoalArea(areanum, ps.origin, goal->areanum, TFL_DEFAULT);
}

int BotGetTeammates(bot_state_t *bs, int *teammates, int maxteammates) {
	int i, numteammates;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if (!maxclients)
		maxclients = trap_Cvar_VariableIntegerValue("sv_maxclients");

	// G_Printf("mates: ");

	numteammates = 0;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
		// if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n")))
			continue;
		// skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR)
			continue;
		//
		if (BotSameTeam(bs, i)) {
			//
			teammates[numteammates++] = i;
			if (numteammates >= maxteammates)
				break;
			// G_Printf("/%s ", Info_ValueForKey(buf, "n"));
		}
	}
	// G_Printf("\n %d mates \n", numteammates);
	return numteammates;
}

/*
==================
BotResetNodeSwitches
==================
*/
void BotResetNodeSwitches(void) {
	numnodeswitches = 0;
}

/*
==================
BotDumpNodeSwitches
==================
*/
void BotDumpNodeSwitches(bot_state_t *bs) {
	int i;
	char netname[MAX_NETNAME];

	ClientName(bs->client, netname, sizeof(netname));
	BotAI_Print(PRT_MESSAGE, "%s at %1.1f switched more than %d AI nodes\n", netname, FloatTime(), MAX_NODESWITCHES);
	for (i = 0; i < numnodeswitches; i++) {
		BotAI_Print(PRT_MESSAGE, "%s", nodeswitch[i]);
	}
	BotAI_Print(PRT_FATAL, "");
}

/*
==================
BotRecordNodeSwitch
==================
*/
static void BotRecordNodeSwitch(bot_state_t *bs, const char *node, const char *str, const char *s) {
	char netname[MAX_NETNAME];

	ClientName(bs->client, netname, sizeof(netname));
	Com_sprintf(nodeswitch[numnodeswitches], 144, "%s at %2.1f entered %s: %s from %s\n", netname, FloatTime(), node,
				str, s);
#ifdef DEBUG
	if (0) {
		BotAI_Print(PRT_MESSAGE, "%s", nodeswitch[numnodeswitches]);
	}
#endif // DEBUG
	numnodeswitches++;
}

// cyr
char *LastNodeSwitch() {
	if (numnodeswitches)
		return nodeswitch[numnodeswitches - 1];
	else
		return "no switch";
}

/*
==================
BotGetAirGoal
==================
*/
static int BotGetAirGoal(bot_state_t *bs, bot_goal_t *goal) {
	bsp_trace_t bsptrace;
	vec3_t end, mins = {-15, -15, -2}, maxs = {15, 15, 2};
	int areanum;

	// trace up until we hit solid
	VectorCopy(bs->origin, end);
	end[2] += 1000;
	BotAI_Trace(&bsptrace, bs->origin, mins, maxs, end, bs->entitynum, CONTENTS_SOLID | CONTENTS_PLAYERCLIP);
	// trace down until we hit water
	VectorCopy(bsptrace.endpos, end);
	BotAI_Trace(&bsptrace, end, mins, maxs, bs->origin, bs->entitynum, CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA);
	// if we found the water surface
	if (bsptrace.fraction > 0) {
		areanum = BotPointAreaNum(bsptrace.endpos);
		if (areanum) {
			VectorCopy(bsptrace.endpos, goal->origin);
			goal->origin[2] -= 2;
			goal->areanum = areanum;
			goal->mins[0] = -15;
			goal->mins[1] = -15;
			goal->mins[2] = -1;
			goal->maxs[0] = 15;
			goal->maxs[1] = 15;
			goal->maxs[2] = 1;
			goal->flags = GFL_AIR;
			goal->number = 0;
			goal->iteminfo = 0;
			goal->entitynum = 0;
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
BotGoForAir
==================
*/
static int BotGoForAir(bot_state_t *bs, int tfl, bot_goal_t *ltg, float range) {
	bot_goal_t goal;
	char info[1024];
	char mapname[128];

	// if the bot needs air
	if (bs->lastair_time < FloatTime() - 6) {

		// cyr{
		// kitchen imperius hack, the route includes diving for > 6 sec, dont go for air meanwhile
		// a clean solution could test for watertime when creating the route...
		trap_GetServerinfo(info, sizeof(info));
		Q_strncpyz(mapname, Info_ValueForKey(info, "mapname"), sizeof(mapname));
		if (!Q_stricmp(mapname, "wop_padkitchen"))
			return qfalse;
		// cyr}
		// if we can find an air goal
		if (BotGetAirGoal(bs, &goal)) {
			trap_BotPushGoal(bs->gs, &goal);
			return qtrue;
		} else {
			// get a nearby goal outside the water
			while (trap_BotChooseNBGItem(bs->gs, bs->origin, bs->inventory, tfl, ltg, range)) {
				trap_BotGetTopGoal(bs->gs, &goal);
				// if the goal is not in water
				if (!(trap_AAS_PointContents(goal.origin) & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))) {
					return qtrue;
				}
				trap_BotPopGoal(bs->gs);
			}
			trap_BotResetAvoidGoals(bs->gs);
		}
	}
	return qfalse;
}

static int BotGotIllegalCartGoal(bot_state_t *bs) {
	bot_goal_t goal;

	// cant collect your own carts in syc ffa
	trap_BotGetTopGoal(bs->gs, &goal);
	if (g_gametype.integer == GT_SPRAYFFA && goal.flags & GFL_DROPPED) {
		char buf[128];
		trap_BotGoalName(goal.number, buf, sizeof(buf));

		if (!Q_stricmp(buf, "neutral Cartridge")) {
			// if(goal.entitynum && !strcmp(g_entities[goal.entitynum].classname, "ammo_spray_n")){
			// valid entitynum?
			if (goal.entitynum < 0 || goal.entitynum >= ENTITYNUM_WORLD)
				return 0;
			// was the item dropped by the bot?
			if (g_entities[goal.entitynum].s.otherEntityNum == bs->client) {
				// avoid item and drop goal
				trap_BotSetAvoidGoalTime(bs->gs, goal.number, 30);
				trap_BotPopGoal(bs->gs);

				// if(bot_developer.integer)
				//	G_Printf("bot spotted own cart, now avoiding \n");
				return 1;
			}
		}
	}
	return 0;
}

/*
==================
BotNearbyGoal
==================
*/
static int BotNearbyGoal(bot_state_t *bs, int tfl, bot_goal_t *ltg, float range) {
	int ret;

	// check if the bot should go for air
	if (BotGoForAir(bs, tfl, ltg, range))
		return qtrue;

	ret = trap_BotChooseNBGItem(bs->gs, bs->origin, bs->inventory, tfl, ltg, range);

	if (BotGotIllegalCartGoal(bs)) // goal was dropped, so dont set ltg_time
		return 0;
	/*
	if (ret)
	{
		char buf[128];
		//get the goal at the top of the stack
		trap_BotGetTopGoal(bs->gs, &goal);
		trap_BotGoalName(goal.number, buf, sizeof(buf));
		BotAI_Print(PRT_MESSAGE, "%1.1f: new nearby goal %s\n", FloatTime(), buf);
	}
	*/
	return ret;
}

/*
==================
BotReachedGoal
==================
*/
static int BotReachedGoal(bot_state_t *bs, bot_goal_t *goal) {
	if (goal->flags & GFL_ITEM) {
		// if touching the goal
		if (trap_BotTouchingGoal(bs->origin, goal)) {
			if (!(goal->flags & GFL_DROPPED)) {
				trap_BotSetAvoidGoalTime(bs->gs, goal->number, -1);
			}
			return qtrue;
		}
		// if the goal isn't there
		if (trap_BotItemGoalInVisButNotVisible(bs->entitynum, bs->eye, bs->viewangles, goal)) {
			/*
			float avoidtime;
			int t;

			avoidtime = trap_BotAvoidGoalTime(bs->gs, goal->number);
			if (avoidtime > 0) {
				t = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, goal->areanum, bs->tfl);
				if ((float) t * 0.009 < avoidtime)
					return qtrue;
			}
			*/
			return qtrue;
		}
		// if in the goal area and below or above the goal and not swimming
		if (bs->areanum == goal->areanum) {
			if (bs->origin[0] > goal->origin[0] + goal->mins[0] && bs->origin[0] < goal->origin[0] + goal->maxs[0]) {
				if (bs->origin[1] > goal->origin[1] + goal->mins[1] &&
					bs->origin[1] < goal->origin[1] + goal->maxs[1]) {
					if (!trap_AAS_Swimming(bs->origin)) {
						return qtrue;
					}
				}
			}
		}
	} else if (goal->flags & GFL_AIR) {
		// if touching the goal
		if (trap_BotTouchingGoal(bs->origin, goal))
			return qtrue;
		// if the bot got air
		if (bs->lastair_time > FloatTime() - 1)
			return qtrue;
	} else {
		// if touching the goal
		if (trap_BotTouchingGoal(bs->origin, goal))
			return qtrue;
	}
	return qfalse;
}

static qboolean BotChooseSpawnGoal(bot_state_t *bs) {
	vec3_t spawn;
	bot_goal_t goal;

	memset(&goal, 0, sizeof(goal));

	GetASpawnPosition(bs->origin, spawn);
	VectorCopy(spawn, goal.origin);
	VectorSet(goal.mins, -256, -256, -32);
	VectorSet(goal.maxs, 256, 256, 32);
	goal.areanum = BotPointAreaNum(goal.origin);
	goal.entitynum = 0;
	goal.number = 0;
	goal.flags = GFL_ROAM;
	goal.iteminfo = 0;
	if (goal.areanum) {
		// push the goal on the stack
		trap_BotPushGoal(bs->gs, &goal);
		//		Com_Printf("new spawnpoint-roam-goal\n");
		return qtrue;
	} else {
		return qfalse;
	}
}

/*
==================
BotGetItemLongTermGoal
==================
*/
static int BotGetItemLongTermGoal(bot_state_t *bs, int tfl, bot_goal_t *goal) {
	// if the bot has no goal
	if (!trap_BotGetTopGoal(bs->gs, goal)) {
		// BotAI_Print(PRT_MESSAGE, "no ltg on stack\n");
		bs->ltg_time = 0;
	}
	// if the bot touches the current goal
	else if (BotReachedGoal(bs, goal)) {
		BotChooseWeapon(bs);
		bs->ltg_time = 0;
	}
	// if it is time to find a new long term goal
	if (bs->ltg_time < FloatTime()) {
		// pop the current goal from the stack
		trap_BotPopGoal(bs->gs);
		// BotAI_Print(PRT_MESSAGE, "%s: choosing new ltg\n", ClientName(bs->client, netname, sizeof(netname)));
		// choose a new goal
		// BotAI_Print(PRT_MESSAGE, "%6.1f client %d: BotChooseLTGItem\n", FloatTime(), bs->client);

		if (trap_BotChooseLTGItem(bs->gs, bs->origin, bs->inventory, tfl) || BotChooseSpawnGoal(bs)) {

			char buf[128];
			// get the goal at the top of the stack
			trap_BotGetTopGoal(bs->gs, goal);
			trap_BotGoalName(goal->number, buf, sizeof(buf));
			// BotAI_Print(PRT_MESSAGE, "%1.1f: new ltg %s -> %s \n", FloatTime(), buf,
			// g_entities[goal->entitynum].classname );

			if (BotGotIllegalCartGoal(bs)) // goal was dropped, so dont set ltg_time
				return trap_BotGetTopGoal(bs->gs, goal);

			bs->ltg_time = FloatTime() + 20; // FIXME: I think giving up the goal after 20secs are a bit short ... or?
		} else { // the bot gets sorta stuck with all the avoid timings, shouldn't happen though
			//
#ifdef DEBUG
			char netname[128];

			BotAI_Print(PRT_MESSAGE, "%s: no valid ltg (probably stuck)\n",
						ClientName(bs->client, netname, sizeof(netname)));
#endif
			// trap_BotDumpAvoidGoals(bs->gs);
			// reset the avoid goals and the avoid reach
			trap_BotResetAvoidGoals(bs->gs);
			trap_BotResetAvoidReach(bs->ms);
		}
		// get the goal at the top of the stack
		return trap_BotGetTopGoal(bs->gs, goal);
	}

	return qtrue;
}

#if 0
// cyr{
static void BotEAMoveToGoal(bot_state_t *bs, bot_goal_t *goal) {
	vec3_t dir;

	VectorSubtract(goal->origin, bs->eye, dir);
	VectorNormalize(dir);
	vectoangles(dir, bs->viewangles);
	trap_EA_View(bs->client, bs->viewangles);
	trap_EA_MoveForward(bs->client);
}

static void BotBathScript(bot_state_t *bs, bot_goal_t *goal) {
	char info[1024];
	char mapname[128];

	trap_GetServerinfo(info, sizeof(info));

	Q_strncpy(mapname, Info_ValueForKey(info, "mapname"), sizeof(mapname));

	if (!Q_stricmp(mapname, "wop_cybbath") && bs->cur_ps.stats[STAT_SPRAYROOMSECS]) {
		int tmp;
		tmp = BotClientTravelTimeToGoal(bs->client, goal);

		if (bs->curtt == tmp)
			bs->stuckcounter++;
		else
			bs->stuckcounter = 0;

		bs->curtt = tmp;

		if (bs->stuckcounter > 10 && (bs->curtt > 100 || bs->curtt == 0)) // dont do this in front of the spraywall
			BotEAMoveToGoal(bs, goal);
	}
}
#endif

bot_goal_t *BotChooseWall(bot_state_t *bs) {
	if (gametype == GT_SPRAYFFA) // fifty fifty
		return (random() < 0.5) ? &bwall : &rwall;

	// GT_SPRAY
	if (BotTeam(bs) == TEAM_RED) { // 99:1
		return (random() < 0.99) ? &rwall : &bwall;
	} else // 99:1
		return (random() < 0.99) ? &bwall : &rwall;
}

bot_goal_t *BotChooseCorrectWall(bot_state_t *bs) {
	// shall not be called for sycffa
	if (BotTeam(bs) == TEAM_RED)
		return &rwall;
	return &bwall;
}

static void SelfTeamGoal(bot_state_t *bs) {
	bs->teamgoal.entitynum = bs->client;
	VectorCopy(bs->origin, bs->teamgoal.origin);
	bs->teamgoal.areanum = BotPointAreaNum(bs->origin);
	VectorSet(bs->teamgoal.mins, -8, -8, -8);
	VectorSet(bs->teamgoal.maxs, 8, 8, 8);
}

/*
==================
BotGetLongTermGoal

we could also create a separate AI node for every long term goal type
however this saves us a lot of code
==================
*/
static int BotGetLongTermGoal(bot_state_t *bs, int tfl, int retreat, bot_goal_t *goal) {
	vec3_t target, dir, dir2;
	char netname[MAX_NETNAME];
	char buf[MAX_MESSAGE_SIZE];
	int areanum;
	float croucher;
	aas_entityinfo_t entinfo, botinfo;

	// cyr_drop{
	if (bs->ltgtype == LTG_FETCHCART) {
		// carts already dropped ?
		if (bs->takecart) {
			SelfTeamGoal(bs);
			memcpy(goal, &bs->teamgoal, sizeof(*goal));
			bs->ltgtype = 0;
			return qtrue;
		}
		// mate is out of carts
		if (!BotGetNumClientCarts(bs, bs->teammate)) {
			bs->ltgtype = 0;
			return BotGetItemLongTermGoal(bs, tfl, goal);
		}
		// get entity information of the companion
		BotEntityInfo(bs->teammate, &entinfo);
		// if the entity information is valid (entity in PVS)
		if (entinfo.valid) {
			areanum = BotPointAreaNum(entinfo.origin);
			if (areanum && trap_AAS_AreaReachability(areanum)) {
				// update team goal
				bs->teamgoal.entitynum = bs->teammate;
				bs->teamgoal.areanum = areanum;
				VectorCopy(entinfo.origin, bs->teamgoal.origin);
				VectorSet(bs->teamgoal.mins, -8, -8, -8);
				VectorSet(bs->teamgoal.maxs, 8, 8, 8);
			}
			// the goal the bot should go for
			memcpy(goal, &bs->teamgoal, sizeof(*goal));
		}
		return qtrue;
	}
	if (bs->ltgtype == LTG_GIVECART) {
		// stop if sprayammo is empty
		if (!bs->cur_ps.ammo[WP_SPRAYPISTOL]) {
			bs->ltgtype = 0;
			return BotGetItemLongTermGoal(bs, tfl, goal);
		}
		if (bs->teammessage_time && bs->teammessage_time < FloatTime()) {
			BotAI_BotInitialChat(bs, "givecart_start", EasyClientName(bs->teammate, netname, sizeof(netname)), NULL);
			trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL);
			trap_EA_Action(bs->client, ACTION_AFFIRMATIVE);
			bs->teammessage_time = 0;
		}
		// get entity information of the companion
		BotEntityInfo(bs->teammate, &entinfo);
		if (!entinfo.valid) // should not happen...
			return qfalse;

		// if the companion is visible
		if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->teammate)) {
			// update visible time
			bs->teammatevisible_time = FloatTime();
			VectorSubtract(entinfo.origin, bs->origin, dir);
			if (VectorLengthSquared(dir) < Square(PUSHCART_DIST)) {
				vec3_t forward;

				vectoangles(dir, bs->ideal_viewangles);
				trap_EA_View(bs->client, bs->ideal_viewangles);

				AngleVectors(bs->viewangles, forward, NULL, NULL);
				VectorNormalize(forward);
				VectorNormalize(dir);
				if (DotProduct(dir, forward) > 0.85) {
					Com_sprintf(buf, sizeof(buf), "dropCartridge");
					trap_EA_Command(bs->client, buf);
				}
				//				bs->cartthrown = qtrue;
			}
		}

		// if the entity information is valid (entity in PVS)
		if (entinfo.valid) {
			areanum = BotPointAreaNum(entinfo.origin);
			if (areanum && trap_AAS_AreaReachability(areanum)) {
				// update team goal
				bs->teamgoal.entitynum = bs->teammate;
				bs->teamgoal.areanum = areanum;
				VectorCopy(entinfo.origin, bs->teamgoal.origin);
				VectorSet(bs->teamgoal.mins, -8, -8, -8);
				VectorSet(bs->teamgoal.maxs, 8, 8, 8);
			}
			// the goal the bot should go for
			memcpy(goal, &bs->teamgoal, sizeof(*goal));
		}
		return qtrue;
	}
	// cyr_drop}
	if (bs->ltgtype == LTG_JOINMATE && !retreat) {
		// check for bot typing status message
		if (bs->teammessage_time && bs->teammessage_time < FloatTime()) {
			BotAI_BotInitialChat(bs, "catch_start", EasyClientName(bs->teammate, netname, sizeof(netname)), NULL);
			trap_BotEnterChat(bs->cs, bs->decisionmaker, CHAT_TELL);
			// BotVoiceChatOnly(bs, bs->decisionmaker, VOICECHAT_YES);
			trap_EA_Action(bs->client, ACTION_AFFIRMATIVE);
			bs->teammessage_time = 0;
		}
		// if trying to help the team mate for more than a minute
		if (bs->teamgoal_time < FloatTime()) {
			bs->ltgtype = 0;
		}

		// if the team mate IS visible for quite some time
		if (bs->teammatevisible_time < FloatTime() - 10)
			bs->ltgtype = 0;
		// get entity information of the companion
		BotEntityInfo(bs->teammate, &entinfo);
		if (!entinfo.valid)
			return qfalse;
		// if the team mate is visible
		if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->teammate)) {
			// if close just stand still there
			VectorSubtract(entinfo.origin, bs->origin, dir);
			if (VectorLengthSquared(dir) < Square(30)) {
				trap_BotResetAvoidReach(bs->ms);
				return qfalse;
			}
		} else {
			// last time the bot was NOT visible
			bs->teammatevisible_time = FloatTime();
		}
		// if the entity information is valid (entity in PVS)
		if (entinfo.valid) {
			areanum = BotPointAreaNum(entinfo.origin);
			if (areanum && trap_AAS_AreaReachability(areanum)) {
				// update team goal
				bs->teamgoal.entitynum = bs->teammate;
				bs->teamgoal.areanum = areanum;
				VectorCopy(entinfo.origin, bs->teamgoal.origin);
				VectorSet(bs->teamgoal.mins, -8, -8, -8);
				VectorSet(bs->teamgoal.maxs, 8, 8, 8);
			}
		}
		memcpy(goal, &bs->teamgoal, sizeof(*goal));
		return qtrue;
	}
	if (bs->ltgtype == LTG_UNFREEZE) {
		gentity_t *mate = &g_entities[bs->teammate];
		bot_goal_t mategoal;

		// time up?
		if (bs->teamgoal_time < FloatTime()) {
			bs->ltgtype = 0;
			return qfalse;
		}
		// mate unfrozen already?
		if (!FT_PlayerIsFrozen(mate)) {
			bs->ltgtype = 0;
			return qfalse;
		}

		// create goal from mate position
		VectorCopy(mate->r.currentOrigin, mategoal.origin);
		mategoal.areanum = trap_AAS_PointAreaNum(mategoal.origin);
		VectorSet(mategoal.mins, -10, -10, -10);
		VectorSet(mategoal.maxs, 10, 10, 10);
		if (!mategoal.areanum) {
			bs->ltgtype = 0;
			return qfalse;
		}

		if (FT_InThawingRange(&g_entities[bs->client], mate)) {
			// camp
			// G_Printf("camping \n");
			VectorCopy(bs->origin, goal->origin);
			goal->areanum = BotPointAreaNum(bs->origin);
			VectorSet(goal->mins, -8, -8, -8);
			VectorSet(goal->maxs, 8, 8, 8);
			return qtrue;
		} else {
			memcpy(goal, &mategoal, sizeof(bot_goal_t));
			// G_Printf("moving, tt is %d \n", trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, goal->areanum,
			// TFL_DEFAULT) );
		}
		return qtrue;
	}
	// if the bot accompanies someone
	if (bs->ltgtype == LTG_TEAMACCOMPANY && !retreat) {
		// check for bot typing status message
		if (bs->teammessage_time && bs->teammessage_time < FloatTime()) {
			BotAI_BotInitialChat(bs, "accompany_start", EasyClientName(bs->teammate, netname, sizeof(netname)), NULL);
			trap_BotEnterChat(bs->cs, bs->decisionmaker, CHAT_TELL);
			// BotVoiceChatOnly(bs, bs->decisionmaker, VOICECHAT_YES);
			trap_EA_Action(bs->client, ACTION_AFFIRMATIVE);
			bs->teammessage_time = 0;
		}
		// if accompanying the companion for 3 minutes
		if (bs->teamgoal_time < FloatTime()) {
			BotAI_BotInitialChat(bs, "accompany_stop", EasyClientName(bs->teammate, netname, sizeof(netname)), NULL);
			trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL);
			bs->ltgtype = 0;
		}
		// get entity information of the companion
		BotEntityInfo(bs->teammate, &entinfo);
		if (!entinfo.valid) {
			bs->ltgtype = 0;
			return qfalse;
		}
		// if the companion is visible
		if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->teammate)) {
			// update visible time
			bs->teammatevisible_time = FloatTime();
			VectorSubtract(entinfo.origin, bs->origin, dir);
			if (VectorLengthSquared(dir) < Square(bs->formation_dist)) {
				//
				// if the client being followed bumps into this bot then
				// the bot should back up
				BotEntityInfo(bs->entitynum, &botinfo);
				if (!entinfo.valid)
					return qfalse;
				// if the followed client is not standing ontop of the bot
				if (botinfo.origin[2] + botinfo.maxs[2] > entinfo.origin[2] + entinfo.mins[2]) {
					// if the bounding boxes touch each other
					if (botinfo.origin[0] + botinfo.maxs[0] > entinfo.origin[0] + entinfo.mins[0] - 4 &&
						botinfo.origin[0] + botinfo.mins[0] < entinfo.origin[0] + entinfo.maxs[0] + 4) {
						if (botinfo.origin[1] + botinfo.maxs[1] > entinfo.origin[1] + entinfo.mins[1] - 4 &&
							botinfo.origin[1] + botinfo.mins[1] < entinfo.origin[1] + entinfo.maxs[1] + 4) {
							if (botinfo.origin[2] + botinfo.maxs[2] > entinfo.origin[2] + entinfo.mins[2] - 4 &&
								botinfo.origin[2] + botinfo.mins[2] < entinfo.origin[2] + entinfo.maxs[2] + 4) {
								// if the followed client looks in the direction of this bot
								AngleVectors(entinfo.angles, dir, NULL, NULL);
								dir[2] = 0;
								VectorNormalize(dir);
								// VectorSubtract(entinfo.origin, entinfo.lastvisorigin, dir);
								VectorSubtract(bs->origin, entinfo.origin, dir2);
								VectorNormalize(dir2);
								if (DotProduct(dir, dir2) > 0.7) {
									// back up
									BotSetupForMovement(bs);
									trap_BotMoveInDirection(bs->ms, dir2, 400, MOVE_WALK);
								}
							}
						}
					}
				}
				// check if the bot wants to crouch
				// don't crouch if crouched less than 5 seconds ago
				if (bs->attackcrouch_time < FloatTime() - 5) {
					croucher = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CROUCHER, 0, 1);
					if (random() < bs->thinktime * croucher) {
						bs->attackcrouch_time = FloatTime() + 5 + croucher * 15;
					}
				}
				// don't crouch when swimming
				if (trap_AAS_Swimming(bs->origin))
					bs->attackcrouch_time = FloatTime() - 1;
				/*//if not arrived yet or arived some time ago
				if (bs->arrive_time < FloatTime() - 2) {
					//if not arrived yet
					if (!bs->arrive_time) {
						trap_EA_Gesture(bs->client);
						BotAI_BotInitialChat(bs, "accompany_arrive", EasyClientName(bs->teammate, netname,
				sizeof(netname)), NULL); trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL); bs->arrive_time =
				FloatTime();
					}
					//if the bot wants to crouch
					else if (bs->attackcrouch_time > FloatTime()) {
						trap_EA_Crouch(bs->client);
					}
					//else do some model taunts
					else if (random() < bs->thinktime * 0.05) {
						//do a gesture :)
						trap_EA_Gesture(bs->client);
					}
				}*/
				// if just arrived look at the companion
				if (bs->arrive_time > FloatTime() - 2) {
					VectorSubtract(entinfo.origin, bs->origin, dir);
					vectoangles(dir, bs->ideal_viewangles);
					bs->ideal_viewangles[2] *= 0.5;
				}
				// else look strategically around for enemies
				else if (random() < bs->thinktime * 0.8f) {
					BotRoamGoal(bs, target);
					VectorSubtract(target, bs->origin, dir);
					vectoangles(dir, bs->ideal_viewangles);
					bs->ideal_viewangles[2] *= 0.5;
				}
				// check if the bot wants to go for air
				if (BotGoForAir(bs, bs->tfl, &bs->teamgoal, 400)) {
					trap_BotResetLastAvoidReach(bs->ms);
					// get the goal at the top of the stack
					// trap_BotGetTopGoal(bs->gs, &tmpgoal);
					// trap_BotGoalName(tmpgoal.number, buf, 144);
					// BotAI_Print(PRT_MESSAGE, "new nearby goal %s\n", buf);
					// time the bot gets to pick up the nearby goal item
					bs->nbg_time = FloatTime() + 8;
					AIEnter_Seek_NBG(bs, "BotLongTermGoal: go for air");
					return qfalse;
				}
				//
				trap_BotResetAvoidReach(bs->ms);
				return qfalse;
			}
		}
		// if the entity information is valid (entity in PVS)
		if (entinfo.valid) {
			areanum = BotPointAreaNum(entinfo.origin);
			if (areanum && trap_AAS_AreaReachability(areanum)) {
				// update team goal
				bs->teamgoal.entitynum = bs->teammate;
				bs->teamgoal.areanum = areanum;
				VectorCopy(entinfo.origin, bs->teamgoal.origin);
				VectorSet(bs->teamgoal.mins, -8, -8, -8);
				VectorSet(bs->teamgoal.maxs, 8, 8, 8);
			}
		}
		// the goal the bot should go for
		memcpy(goal, &bs->teamgoal, sizeof(*goal));
		// if the companion is NOT visible for too long
		if (bs->teammatevisible_time < FloatTime() - 60) {
			BotAI_BotInitialChat(bs, "accompany_cannotfind", EasyClientName(bs->teammate, netname, sizeof(netname)),
								 NULL);
			trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL);
			bs->ltgtype = 0;
			// just to make sure the bot won't spam this message
			bs->teammatevisible_time = FloatTime();
		}
		return qtrue;
	}

	if (bs->ltgtype == LTG_DEFENDKEYAREA) {
		int traveltime;

		// timeout? force new ltg decision
		if (bs->teamgoal_time < FloatTime()) {
			bs->ltgtype = 0;
		}

		// passed the def range border? return to teamgoal!
		traveltime = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, bs->teamgoal.areanum, TFL_DEFAULT);
		if (traveltime > DEFENDAWAY_RANGE) {
			bs->defendaway_time = 0;
		}

		// tell the hud
		BotAddInfo(bs, va("defrange %d", traveltime), AIDBG_GAMETYPE);

		if (bs->defendaway_time < FloatTime()) {
			// set the bot goal
			memcpy(goal, &bs->teamgoal, sizeof(*goal));

			// tell the hud
			BotAddInfo(bs, va("defmode: moving back to goal %d", goal->entitynum), AIDBG_GAMETYPE);

			// very close? ... go roam
			if (traveltime && traveltime < DEFENDAWAY_RANGE / 20) // VectorLengthSquared(dir) < Square(120))
			{
				trap_BotResetAvoidReach(bs->ms);
				bs->defendaway_time = FloatTime() + 10 + 3 * random();
			}

			// goal visible and bot is near?
			if (traveltime && traveltime < DEFENDAWAY_RANGE / 3 &&
				BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 200, goal->entitynum)) {
				trap_BotResetAvoidReach(bs->ms);
				bs->defendaway_time = FloatTime() + 10 + 3 * random();
			}

			return qtrue;
		} else {
			// tell the hud that you roam
			BotAddInfo(bs, va("defmode: roam for %.1f sec", bs->defendaway_time - FloatTime()), AIDBG_GAMETYPE);

			// prolong defendaway_time if goal is visible from current pos
			if (random() < (float)bot_thinktime.integer / 1000.0f) { // about once per second
				if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 200, bs->teamgoal.entitynum)) {
					trap_BotResetAvoidReach(bs->ms);
					bs->defendaway_time = FloatTime() + 10 + 3 * random();
				}
			}
		}

	} // end defendkeyarea

	// get an item
	if (bs->ltgtype == LTG_GETITEM && !retreat) {
		// check for bot typing status message
		if (bs->teammessage_time && bs->teammessage_time < FloatTime()) {
			trap_BotGoalName(bs->teamgoal.number, buf, sizeof(buf));
			BotAI_BotInitialChat(bs, "getitem_start", buf, NULL);
			trap_BotEnterChat(bs->cs, bs->decisionmaker, CHAT_TELL);
			// BotVoiceChatOnly(bs, bs->decisionmaker, VOICECHAT_YES);
			trap_EA_Action(bs->client, ACTION_AFFIRMATIVE);
			bs->teammessage_time = 0;
		}
		// set the bot goal
		memcpy(goal, &bs->teamgoal, sizeof(*goal));
		// stop after some time
		if (bs->teamgoal_time < FloatTime()) {
			bs->ltgtype = 0;
		}
		//
		if (trap_BotItemGoalInVisButNotVisible(bs->entitynum, bs->eye, bs->viewangles, goal)) {
			trap_BotGoalName(bs->teamgoal.number, buf, sizeof(buf));
			BotAI_BotInitialChat(bs, "getitem_notthere", buf, NULL);
			trap_BotEnterChat(bs->cs, bs->decisionmaker, CHAT_TELL);
			bs->ltgtype = 0;
		} else if (BotReachedGoal(bs, goal)) {
			trap_BotGoalName(bs->teamgoal.number, buf, sizeof(buf));
			BotAI_BotInitialChat(bs, "getitem_gotit", buf, NULL);
			trap_BotEnterChat(bs->cs, bs->decisionmaker, CHAT_TELL);
			bs->ltgtype = 0;
		}
		return qtrue;
	}
	if (gametype == GT_CTF) {
		if (bs->ltgtype == LTG_CAPTUREFLAG) {
			bot_goal_t *base_flag = NULL;
			int tt;
			int flagstatus;

			// if not carrying the flag anymore, or timer hits
			if (!BotCTFCarryingFlag(bs) || bs->teamgoal_time < FloatTime()) {
				bs->ltgtype = 0;
				return qfalse;
			}

			// stop wandering around if your flag is back home
			flagstatus = Team_GetFlagStatus(BotTeam(bs));
			if (flagstatus == FLAG_ATBASE)
				bs->rushbaseaway_time = 0;

			if (bs->rushbaseaway_time < FloatTime()) {
				// set base flag goal
				if (BotTeam(bs) == TEAM_RED)
					base_flag = &ctf_redflag;
				else
					base_flag = &ctf_blueflag;

				// go for items if your flag is not at base
				if (flagstatus != FLAG_ATBASE) {
					tt = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, base_flag->areanum, bs->tfl);
					if (tt < 400) {
						bs->rushbaseaway_time = FloatTime() + 30;
					}
				}

				// do we have a waypoint system?
				if (!numwaypoints) {

					memcpy(goal, base_flag, sizeof(*goal));

					// if touching the base flag the bot should loose the enemy flag
					if (trap_BotTouchingGoal(bs->origin, goal)) {
						// if the bot is still carrying the enemy flag then the
						// base flag is gone, now just walk near the base a bit
						if (BotCTFCarryingFlag(bs)) {
							trap_BotResetAvoidReach(bs->ms);
							bs->rushbaseaway_time = FloatTime() + 5 + 10 * random();
							// FIXME: add chat to tell the others to get back the flag
						} else {
							bs->ltgtype = 0;
						}
					}
					return qtrue;
				} // oldschool system
				else {
					float wpDist_sqr; // squared distance to the next waypoint
					int tt;

					// do we have a valid waypoint?
					if (!BotWpValid(bs)) {
						FindWp(bs, qtrue);
					}

					// if you are going for the final waypoint, go for the flag instead
					if (!BotWpHasSuccessor(bs, qtrue)) {
						BotAddInfo(bs, "final wp", AIDBG_GAMETYPE);
						memcpy(goal, base_flag, sizeof(*goal));
						bs->wptime = level.time;
						return qtrue;
					}

					// are we close enough to proceed to the next wp?
					tt = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, bs->wp->goal.areanum, bs->tfl);
					if (tt) {
						if (tt < 10)
							GetNextWp(bs, qtrue);
					} else { // tt is invalid, fall back to 3d distance
						wpDist_sqr = DistanceSquared(bs->wp->goal.origin, bs->origin);
						if (wpDist_sqr < 40000) // Square(200)
							GetNextWp(bs, qtrue);
					}

					BotAddInfo(bs, va("wp %s", bs->wp->name), AIDBG_GAMETYPE);

					bs->wptime = level.time;

					memcpy(goal, &bs->wp->goal, sizeof(*goal));
					return qtrue;
				}
			}
		}
		if (bs->ltgtype == LTG_GETFLAG) {
			int status;

			// stop after 3 minutes
			if (bs->teamgoal_time < FloatTime()) {
				bs->ltgtype = 0;
			}

			status = Team_GetFlagStatus(BotOppositeTeam(bs));
			BotAddInfo(bs, va("nmy flag %d", status), AIDBG_GAMETYPE);
			if (status == FLAG_TAKEN) {
				bs->ltgtype = 0;
				return qfalse;
			} else if (status == FLAG_DROPPED) {
				memcpy(goal, &bs->teamgoal, sizeof(*goal));
				// reset ltg if bot touches the goal.. to cover the case where the flag
				// got picked up and dropped again between two AI frames
				VectorSubtract(goal->origin, bs->origin, dir);
				if (VectorLengthSquared(dir) < Square(7)) {
					bs->ltgtype = 0;
					return qfalse;
				}
				return qtrue;
			} else {
				if (!numwaypoints) {
					switch (BotTeam(bs)) {
					case TEAM_RED:
						memcpy(goal, &ctf_blueflag, sizeof(*goal));
						break;
					case TEAM_BLUE:
						memcpy(goal, &ctf_redflag, sizeof(*goal));
						break;
					default:
						bs->ltgtype = 0;
						return qfalse;
					}
				} else {
					float wpDist_sqr; // squared distance to the next waypoint
					int tt;

					// do we have a valid waypoint?
					if (!BotWpValid(bs)) {
						FindWp(bs, qfalse);
					}

					// if you are going for the final waypoint, go for the flag instead
					if (!BotWpHasSuccessor(bs, qfalse)) {
						BotAddInfo(bs, "final wp", AIDBG_GAMETYPE);
						if (BotTeam(bs) == TEAM_RED)
							memcpy(goal, &ctf_blueflag, sizeof(*goal));
						else
							memcpy(goal, &ctf_redflag, sizeof(*goal));
						bs->wptime = level.time;
						return qtrue;
					}

					// are we close enough to proceed to the next wp?
					tt = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, bs->wp->goal.areanum, bs->tfl);
					if (tt) {
						if (tt < 10)
							GetNextWp(bs, qfalse);
					} else { // tt is invalid, fall back to 3d distance
						wpDist_sqr = DistanceSquared(bs->wp->goal.origin, bs->origin);
						if (wpDist_sqr < 40000) // Square(200)
							GetNextWp(bs, qfalse);
					}

					BotAddInfo(bs, va("wp %s", bs->wp->name), AIDBG_GAMETYPE); // va("%d", GetWpID(bs->wp) ) );

					bs->wptime = level.time;

					memcpy(goal, &bs->wp->goal, sizeof(*goal));
				}
				return qtrue;
			}
		}
		if (bs->ltgtype == LTG_PICKUPFLAG) {
			int status;

			// time's up?
			if (bs->teamgoal_time < FloatTime()) {
				bs->ltgtype = 0;
				return qfalse;
			}

			// flag is no longer there?
			status = Team_GetFlagStatus(BotTeam(bs));
			if (status != FLAG_DROPPED) {
				bs->ltgtype = 0;
				return qfalse;
			}

			memcpy(goal, &bs->teamgoal, sizeof(*goal));
			return qtrue;
		}
		if (bs->ltgtype == LTG_PLANTBOOMIE) {
			if (bs->teamgoal_time < FloatTime()) {
				bs->ltgtype = 0;
				return qfalse;
			}

			// reached?
			// VectorSubtract(bs->teamgoal.origin, bs->origin, dir);
			// if(VectorLengthSquared(dir) < Square(10) )
			if (trap_BotTouchingGoal(bs->origin, &bs->teamgoal)) {
				vec3_t bAngles;
				float diffPitch, diffYaw;

				VectorCopy(boomiespots[bs->teammate].angles, bAngles);
				VectorCopy(bAngles, bs->ideal_viewangles);
				bs->flags |= BFL_IDEALVIEWSET;
				diffPitch = AngleDifference(bs->viewangles[0], bs->ideal_viewangles[0]);
				diffYaw = AngleDifference(bs->viewangles[1], bs->ideal_viewangles[1]);

				// BotAddInfo(bs, va("looking at goal %d %d %d", (int)bAngles[0], (int)bAngles[1], (int)bAngles[2]),
				// AIDBG_GAMETYPE );
				// do eeett
				if (diffPitch < 5.f && diffYaw < 5.f) {
					BotAddInfo(bs, "planting", AIDBG_GAMETYPE);
					trap_EA_Use(bs->client);
				}
			} else
				BotAddInfo(bs, "moving to goal", AIDBG_GAMETYPE);

			memcpy(goal, &bs->teamgoal, sizeof(*goal));
			return qtrue;
		}
		if (bs->ltgtype == LTG_PLANTBAMBAM) {
			if (bs->teamgoal_time < FloatTime()) {
				bs->ltgtype = 0;
				return qfalse;
			}

			// reached?
			// VectorSubtract(goal->origin, bs->origin, dir);
			// if(VectorLengthSquared(dir) < Square(10) )
			if (trap_BotTouchingGoal(bs->origin, &bs->teamgoal)) {
				BotAddInfo(bs, "planting", AIDBG_GAMETYPE);
				// do eeett
				trap_EA_Use(bs->client);
			} else
				BotAddInfo(bs, "moving to goal", AIDBG_GAMETYPE);

			memcpy(goal, &bs->teamgoal, sizeof(*goal));
			return qtrue;
		}
	}

	// camp at the balloon untill captured
	if (bs->ltgtype == LTG_BALLCAMP) {
		if (bs->teamgoal_time < FloatTime()) {
			bs->ltgtype = 0;
		}
		// captured?
		if (IsMyBalloon(BotTeam(bs), &bs->teamgoal)) {
			bs->ltgtype = 0;
		}

		// set the bot goal
		memcpy(goal, &bs->teamgoal, sizeof(*goal));

		// if really near the camp spot
		VectorSubtract(goal->origin, bs->origin, dir);
		if (VectorLengthSquared(dir) < Square(60)) {
			// look strategically around for enemies
			if (random() < bs->thinktime * 0.8f) {
				BotRoamGoal(bs, target);
				VectorSubtract(target, bs->origin, dir);
				vectoangles(dir, bs->ideal_viewangles);
				bs->ideal_viewangles[2] *= 0.5f;
			}
			// check if the bot wants to crouch
			// don't crouch if crouched less than 5 seconds ago
			if (bs->attackcrouch_time < FloatTime() - 5) {
				croucher = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CROUCHER, 0, 1);
				if (random() < bs->thinktime * croucher) {
					bs->attackcrouch_time = FloatTime() + 5 + croucher * 15;
				}
			}
			// if the bot wants to crouch
			if (bs->attackcrouch_time > FloatTime()) {
				trap_EA_Crouch(bs->client);
			}
			// don't crouch when swimming
			if (trap_AAS_Swimming(bs->origin))
				bs->attackcrouch_time = FloatTime() - 1;
			// make sure the bot is not gonna drown
			if (trap_PointContents(bs->eye, bs->entitynum) & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
				bs->ltgtype = 0;
			}
			trap_BotResetAvoidReach(bs->ms);
			return qfalse;
		}
		return qtrue;
	}

	if (gametype == GT_SPRAY || gametype == GT_SPRAYFFA) { //&& bs->rushbaseaway_time < FloatTime()) {
		if (bs->ltgtype == LTG_RUSHBASE) {
			// bot_goal_t* wall;

			if (!bs->inventory[INVENTORY_SPRAYPISTOLAMMO]) // false positive, reset ltg
				bs->ltgtype = 0;

			if (!bs->which_wall) { // should not happen
				bs->which_wall = BotChooseCorrectWall(bs);
				if (bot_developer.integer)
					G_Printf("wall wasnt set in rushbase ltg\n");
			}

			memcpy(goal, bs->which_wall, sizeof(*goal));
			// already found it ?
			if (IsWall(bs->enemy)) { // -1 if no enemy
				BotChooseWeapon(bs);
				BotAimAtEnemy(bs);
				if (BotSprayWallCheckAttack(bs)) { // to far or obstacle or weaponswitching
					vec3_t dir;
					// walk on toward wall...
					// if very close, change target
					VectorSubtract(goal->origin, bs->eye, dir);
					if (VectorLengthSquared(dir) < Square(60)) {
						// recalc aimtarget
						bs->sprayTimer = 0;
						return qfalse; // run ainode again
					}
				} else { // ok, bot is spraying so dont walk any further
					SelfTeamGoal(bs);
					memcpy(goal, &bs->teamgoal, sizeof(*goal));
				}
			} else { // go for spraywall

				// go for tele if wall is not reachable
				if (!BotClientTravelTimeToGoal(bs->client, goal)) {
					memcpy(goal, &spraytele, sizeof(*goal));

					// even spraytele not reachable?
					// if (bot_developer.integer) {
					//	if (!BotClientTravelTimeToGoal(bs->client, goal)) {
					//		G_Printf("^1 no way to wall, tele area is %d \n", spraytele.areanum);
					//	}
					// }
				}

				// BotBathScript(bs, goal);    // cyr  buhhh, hack
			}
			return qtrue;
		}
		// get out of the sprayroom
		if (bs->ltgtype == LTG_ATTACKENEMYBASE) {
			memcpy(goal, &outtele, sizeof(*goal));

			// fallback, trashcan needs this
			if (!BotClientTravelTimeToGoal(bs->client, goal)) {
				vec3_t dir;
				// BotEAMoveToGoal(bs, goal);
				VectorSubtract(goal->origin, bs->eye, dir);
				trap_BotMoveInDirection(bs->ms, dir, 320, MOVE_WALK);
			}

			if (bs->cur_ps.eFlags & EF_AWARD_SPRAYKILLER) {
				if (random() < 0.7f) {
					trap_EA_Gesture(bs->client);
				}
			} else if (bs->cur_ps.eFlags & EF_AWARD_SPRAYGOD) {
				trap_EA_Gesture(bs->client);
			}
			// out of sprayroom  ?
			if (trap_BotTouchingGoal(bs->origin, goal) || !ClientInSprayroom(bs->client)) {
				bs->ltgtype = 0;
				bs->enemy = -1;
				return qfalse;
			}
			return qtrue;
		}
	} else if (gametype == GT_BALLOON && bs->ltgtype == LTG_ATTACKENEMYBASE && bs->attackaway_time < FloatTime()) {
		qboolean mateatgoal;
		// vec3_t dir, entangles;

		mateatgoal = qfalse;
		if (BotClientTravelTimeToGoal(bs->client, &bs->teamgoal) < 150) {
			int mates[MAX_CLIENTS];
			int i, nummates;
			// gentity_t *ent;

			nummates = BotGetTeammates(bs, mates, sizeof(mates));

			for (i = 0; i < nummates; i++) {
				aas_entityinfo_t mateinfo;
				vec3_t dist;

				if (bs->client == mates[i])
					continue;
				BotEntityInfo(mates[i], &mateinfo);
				if (!mateinfo.valid)
					continue;
				VectorSubtract(mateinfo.origin, bs->teamgoal.origin, dist);
				if (VectorLengthSquared(dist) < Square(80)) {
					mateatgoal = qtrue;
					bs->attackaway_time = FloatTime() + 2 + 2 * random();
					break;
				}
			}
		}

		BotAddInfo(bs, "attacking loon", AIDBG_GAMETYPE);

		// set the bot goal
		if (!mateatgoal) {
			memcpy(goal, &bs->teamgoal, sizeof(*goal));
			return qtrue;
		}
	}

	if (bs->ltgtype == LTG_GO_FOR_HEALTH) {
		// G_Printf("execing health");
		// reached max health ? // cyr 20055
		if (bs->inventory[INVENTORY_HEALTH] >= g_entities[bs->client].client->pers.maxHealth) {
			bs->ltgtype = 0;
			bs->hstationgoal = 0;
			return qfalse;
		}
		// hstation is empty?
		if (!g_entities[hstations[bs->hstationgoal - 1].entitynum].health) {
			bs->ltgtype = 0;
			bs->hstationgoal = 0;
			return qfalse;
		}

		// set the bot goal
		memcpy(goal, &hstations[bs->hstationgoal - 1], sizeof(*goal));
		// todo: check if mate is using the hstation
		return qtrue;
	}

	// no LTG

	// normal goal stuff
	return BotGetItemLongTermGoal(bs, tfl, goal);
}

/*
==================
BotLongTermGoal
==================
*/
int BotLongTermGoal(bot_state_t *bs, int tfl, int retreat, bot_goal_t *goal) {
	return BotGetLongTermGoal(bs, tfl, retreat, goal);

/* lead teammate... disabled atm ... make this a LTG when putting it back in */
#if 0
	aas_entityinfo_t entinfo;
	char teammate[MAX_MESSAGE_SIZE];
	float squaredist;
	int areanum;
	vec3_t dir;

	// if the bot is leading someone and not retreating
	if (bs->lead_time > 0 && !retreat) {
		if (bs->lead_time < FloatTime()) {
			BotAI_BotInitialChat(bs, "lead_stop", EasyClientName(bs->lead_teammate, teammate, sizeof(teammate)), NULL);
			trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL);
			bs->lead_time = 0;
			return BotGetLongTermGoal(bs, tfl, retreat, goal);
		}
		//
		if (bs->leadmessage_time < 0 && -bs->leadmessage_time < FloatTime()) {
			BotAI_BotInitialChat(bs, "followme", EasyClientName(bs->lead_teammate, teammate, sizeof(teammate)), NULL);
			trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL);
			bs->leadmessage_time = FloatTime();
		}
		// get entity information of the companion
		BotEntityInfo(bs->lead_teammate, &entinfo);
		//
		if (entinfo.valid) {
			areanum = BotPointAreaNum(entinfo.origin);
			if (areanum && trap_AAS_AreaReachability(areanum)) {
				// update team goal
				bs->lead_teamgoal.entitynum = bs->lead_teammate;
				bs->lead_teamgoal.areanum = areanum;
				VectorCopy(entinfo.origin, bs->lead_teamgoal.origin);
				VectorSet(bs->lead_teamgoal.mins, -8, -8, -8);
				VectorSet(bs->lead_teamgoal.maxs, 8, 8, 8);
			}
		}
		// if the team mate is visible
		if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->lead_teammate)) {
			bs->leadvisible_time = FloatTime();
		}
		// if the team mate is not visible for 1 seconds
		if (bs->leadvisible_time < FloatTime() - 1) {
			bs->leadbackup_time = FloatTime() + 2;
		}
		// distance towards the team mate
		VectorSubtract(bs->origin, bs->lead_teamgoal.origin, dir);
		squaredist = VectorLengthSquared(dir);
		// if backing up towards the team mate
		if (bs->leadbackup_time > FloatTime()) {
			if (bs->leadmessage_time < FloatTime() - 20) {
				BotAI_BotInitialChat(bs, "followme", EasyClientName(bs->lead_teammate, teammate, sizeof(teammate)),
									 NULL);
				trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL);
				bs->leadmessage_time = FloatTime();
			}
			// if very close to the team mate
			if (squaredist < Square(100)) {
				bs->leadbackup_time = 0;
			}
			// the bot should go back to the team mate
			memcpy(goal, &bs->lead_teamgoal, sizeof(*goal));
			return qtrue;
		} else {
			// if quite distant from the team mate
			if (squaredist > Square(500)) {
				if (bs->leadmessage_time < FloatTime() - 20) {
					BotAI_BotInitialChat(bs, "followme", EasyClientName(bs->lead_teammate, teammate, sizeof(teammate)),
										 NULL);
					trap_BotEnterChat(bs->cs, bs->teammate, CHAT_TELL);
					bs->leadmessage_time = FloatTime();
				}
				// look at the team mate
				VectorSubtract(entinfo.origin, bs->origin, dir);
				vectoangles(dir, bs->ideal_viewangles);
				bs->ideal_viewangles[2] *= 0.5;
				// just wait for the team mate
				return qfalse;
			}
		}
	}
	return BotGetLongTermGoal(bs, tfl, retreat, goal);
#endif
}

/*
==================
AINode_Intermission
==================
*/
static int AINode_Intermission(bot_state_t *bs) {
	// if the intermission ended
	if (!BotIntermission(bs)) {
		if (BotChat_StartLevel(bs)) {
			bs->stand_time = FloatTime() + BotChatTime(bs);
		} else {
			bs->stand_time = FloatTime() + 2;
		}
		AIEnter_Stand(bs, "intermission: chat");
	}
	return qtrue;
}

/*
==================
AIEnter_Intermission
==================
*/
static void AIEnter_Intermission(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "intermission", "", s);
	// reset the bot state
	BotResetState(bs);
	// check for end level chat
	if (BotChat_EndLevel(bs)) {
		trap_BotEnterChat(bs->cs, 0, bs->chatto);
	}
	bs->ainode = AINode_Intermission;
}

/*
==================
AINode_Observer
==================
*/
static int AINode_Observer(bot_state_t *bs) {
	// if the bot left observer mode
	if (!BotIsObserver(bs)) {
		AIEnter_Stand(bs, "observer: left observer");
	}
	return qtrue;
}

/*
==================
AIEnter_Observer
==================
*/
static void AIEnter_Observer(bot_state_t *bs, const char *s) {
	BotRecordNodeSwitch(bs, "observer", "", s);
	// reset the bot state
	BotResetState(bs);
	bs->ainode = AINode_Observer;
}

/*
==================
AINode_Stand
==================
*/
static int AINode_Stand(bot_state_t *bs) {
	// if the bot's health decreased
	if (bs->lastframe_health > bs->inventory[INVENTORY_HEALTH]) {
		if (BotChat_HitTalking(bs)) {
			bs->standfindenemy_time = FloatTime() + BotChatTime(bs) + 0.1;
			bs->stand_time = FloatTime() + BotChatTime(bs) + 0.1;
		}
	}
	if (bs->standfindenemy_time < FloatTime()) {
		if (BotFindEnemy(bs, -1)) {
			AIEnter_Battle_Fight(bs, "stand: found enemy");
			return qfalse;
		}
		bs->standfindenemy_time = FloatTime() + 1;
	}
	// put up chat icon
	trap_EA_Talk(bs->client);
	// when done standing
	if (bs->stand_time < FloatTime()) {
		trap_BotEnterChat(bs->cs, 0, bs->chatto);
		AIEnter_Seek_LTG(bs, "stand: time out");
		return qfalse;
	}
	//
	return qtrue;
}

int AINode_Frozen(bot_state_t *bs) {
	if (BotIsObserver(bs)) {
		AIEnter_Observer(bs, "frozen: observer");
		return qfalse;
	}
	// if in the intermission
	if (BotIntermission(bs)) {
		AIEnter_Intermission(bs, "frozen: intermision");
		return qfalse;
	}
	// respawn if dead
	if (BotIsDead(bs)) {
		AIEnter_Respawn(bs, "frozen: bot dead");
		return qfalse;
	}
	// unfrozen?
	if (!FT_PlayerIsFrozen(&g_entities[bs->client])) {
		AIEnter_Seek_LTG(bs, "frozen: unfrozen");
		return qfalse;
	}

	if (bs->teammessage_time < level.time) {
		// use trap_InitialChat and AIEnterStand instead?
		trap_EA_SayTeam(bs->client, "help me, i'm frozen.\n");
		bs->teammessage_time = level.time + 20000 + 10000 * random();
	}
	return qtrue;
}

void AIEnterFrozen(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "frozen", "", s);
	bs->ainode = AINode_Frozen;
	bs->teammessage_time = 0;
}

/*
==================
AIEnter_Stand
==================
*/
void AIEnter_Stand(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "stand", "", s);
	bs->standfindenemy_time = FloatTime() + 1;
	bs->ainode = AINode_Stand;
}

/*
==================
AIEnter_Respawn
==================
*/
void AIEnter_Respawn(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "respawn", "", s);
	// reset some states
	trap_BotResetMoveState(bs->ms);
	trap_BotResetGoalState(bs->gs);
	//	trap_BotResetAvoidGoals(bs->gs);	// cyr, timings are still valid, so why reset?
	trap_BotResetAvoidReach(bs->ms);
	// if the bot wants to chat
	if (BotChat_Death(bs)) {
		bs->respawn_time = FloatTime() + BotChatTime(bs);
		bs->respawnchat_time = FloatTime();
	} else {
		bs->respawn_time = FloatTime() + 1 + random();
		bs->respawnchat_time = 0;
	}
	// set respawn state
	bs->respawn_wait = qfalse;
	bs->ainode = AINode_Respawn;

	// reset team goal (big balloon)
	bs->teamgoal_time = 0;
}

/*
==================
AINode_Respawn
==================
*/
int AINode_Respawn(bot_state_t *bs) {
	// if waiting for the actual respawn
	if (bs->respawn_wait) {
		if (!BotIsDead(bs)) {
			AIEnter_Seek_LTG(bs, "respawn: respawned");
		} else {
			trap_EA_Respawn(bs->client);
		}
	} else if (bs->respawn_time < FloatTime()) {
		// wait until respawned
		bs->respawn_wait = qtrue;
		// elementary action respawn
		trap_EA_Respawn(bs->client);
		//
		if (bs->respawnchat_time) {
			trap_BotEnterChat(bs->cs, 0, bs->chatto);
			bs->enemy = -1;
		}
	}
	if (bs->respawnchat_time && bs->respawnchat_time < FloatTime() - 0.5) {
		trap_EA_Talk(bs->client);
	}
	//
	return qtrue;
}

/*
==================
BotSelectActivateWeapon
==================
*/
int BotSelectActivateWeapon(bot_state_t *bs) {
	if (bs->inventory[INVENTORY_NIPPER] > 0 && bs->inventory[INVENTORY_NIPPERAMMO] > 0)
		return WEAPONINDEX_NIPPER;
	else if (bs->inventory[INVENTORY_PUMPER] > 0 && bs->inventory[INVENTORY_PUMPERAMMO] > 0)
		return WEAPONINDEX_PUMPER;
	else if (bs->inventory[INVENTORY_BUBBLEG] > 0 && bs->inventory[INVENTORY_BUBBLEGAMMO] > 0)
		return WEAPONINDEX_BUBBLEG;
	else if (bs->inventory[INVENTORY_BOASTER] > 0 && bs->inventory[INVENTORY_BOASTERAMMO] > 0)
		return WEAPONINDEX_BOASTER;
	else if (bs->inventory[INVENTORY_SPLASHER] > 0 && bs->inventory[INVENTORY_SPLASHER] > 0)
		return WEAPONINDEX_SPLASHER;
	else if (bs->inventory[INVENTORY_BETTY] > 0 && bs->inventory[INVENTORY_BETTYAMMO] > 0)
		return WEAPONINDEX_BETTY;
	else if (bs->inventory[INVENTORY_IMPERIUS] > 0 && bs->inventory[INVENTORY_IMPERIUS] > 0)
		return WEAPONINDEX_IMPERIUS;
	else {
		return -1;
	}
}

/*
==================
AIEnter_Seek_ActivateEntity
==================
*/
void AIEnter_Seek_ActivateEntity(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "activate entity", "", s);
	bs->ainode = AINode_Seek_ActivateEntity;
}

/*
==================
AINode_Seek_Activate_Entity
==================
*/
int AINode_Seek_ActivateEntity(bot_state_t *bs) {
	bot_goal_t *goal;
	vec3_t target, dir, ideal_viewangles;
	bot_moveresult_t moveresult;
	int targetvisible;
	bsp_trace_t bsptrace;
	aas_entityinfo_t entinfo;

	if (BotIsObserver(bs)) {
		BotClearActivateGoalStack(bs);
		AIEnter_Observer(bs, "active entity: observer");
		return qfalse;
	}
	// if in the intermission
	if (BotIntermission(bs)) {
		BotClearActivateGoalStack(bs);
		AIEnter_Intermission(bs, "activate entity: intermission");
		return qfalse;
	}
	// respawn if dead
	if (BotIsDead(bs)) {
		BotClearActivateGoalStack(bs);
		AIEnter_Respawn(bs, "activate entity: bot dead");
		return qfalse;
	}
	if (FT_PlayerIsFrozen(&g_entities[bs->client])) {
		AIEnterFrozen(bs, "seek ltg: frozen");
		return qfalse;
	}
	//
	bs->tfl = TFL_DEFAULT;
	/*	if( bs->inventory[INVENTORY_JUMPER] ){
			bs->tfl |= TFL_JUMPER;
			bs->tfl &= ~TFL_JUMP;
		}
	*/
	// if in lava or slime the bot should be able to get out
	if (BotInLavaOrSlime(bs))
		bs->tfl |= TFL_LAVA | TFL_SLIME;
	// map specific code
	BotMapScripts(bs);
	// no enemy
	bs->enemy = -1;
	// if the bot has no activate goal
	if (!bs->activatestack) {
		BotClearActivateGoalStack(bs);
		AIEnter_Seek_NBG(bs, "activate entity: no goal");
		return qfalse;
	}
	//
	goal = &bs->activatestack->goal;
	// initialize target being visible to false
	targetvisible = qfalse;
	// if the bot has to shoot at a target to activate something
	if (bs->activatestack->shoot) {
		//
		BotAI_Trace(&bsptrace, bs->eye, NULL, NULL, bs->activatestack->target, bs->entitynum, MASK_SHOT);
		// if the shootable entity is visible from the current position
		if (bsptrace.fraction >= 1.0 || bsptrace.ent == goal->entitynum) {
			targetvisible = qtrue;
			// if holding the right weapon
			if (bs->cur_ps.weapon == bs->activatestack->weapon) {
				VectorSubtract(bs->activatestack->target, bs->eye, dir);
				vectoangles(dir, ideal_viewangles);
				// if the bot is pretty close with its aim
				if (InFieldOfVision(bs->viewangles, 20, ideal_viewangles)) {
					trap_EA_Attack(bs->client);
				}
			}
		}
	}
	// if the shoot target is visible
	if (targetvisible) {
		// get the entity info of the entity the bot is shooting at
		BotEntityInfo(goal->entitynum, &entinfo);
		if (!entinfo.valid) {
			AIEnter_Seek_LTG(bs, "activate entity: invalid goal entity");
			return qfalse;
		}
		// if the entity the bot shoots at moved
		if (!VectorCompare(bs->activatestack->origin, entinfo.origin)) {
#ifdef DEBUG
			BotAI_Print(PRT_MESSAGE, "hit shootable button or trigger\n");
#endif // DEBUG
			bs->activatestack->time = 0;
		}
		// if the activate goal has been activated or the bot takes too long
		if (bs->activatestack->time < FloatTime()) {
			BotPopFromActivateGoalStack(bs);
			// if there are more activate goals on the stack
			if (bs->activatestack) {
				bs->activatestack->time = FloatTime() + 10;
				return qfalse;
			}
			AIEnter_Seek_NBG(bs, "activate entity: time out");
			return qfalse;
		}
		memset(&moveresult, 0, sizeof(bot_moveresult_t));
	} else {
		// if the bot has no goal
		if (!goal) {
			bs->activatestack->time = 0;
		}
		// if the bot does not have a shoot goal
		else if (!bs->activatestack->shoot) {
			// if the bot touches the current goal
			if (trap_BotTouchingGoal(bs->origin, goal)) {
#ifdef DEBUG
				BotAI_Print(PRT_MESSAGE, "touched button or trigger\n");
#endif // DEBUG
				bs->activatestack->time = 0;
			}
		}
		// if the activate goal has been activated or the bot takes too long
		if (bs->activatestack->time < FloatTime()) {
			BotPopFromActivateGoalStack(bs);
			// if there are more activate goals on the stack
			if (bs->activatestack) {
				bs->activatestack->time = FloatTime() + 10;
				return qfalse;
			}
			AIEnter_Seek_NBG(bs, "activate entity: activated");
			return qfalse;
		}
		// predict obstacles
		if (BotAIPredictObstacles(bs, goal))
			return qfalse;
		// initialize the movement state
		BotSetupForMovement(bs);
		// move towards the goal
		trap_BotMoveToGoal(&moveresult, bs->ms, goal, bs->tfl);
		// if the movement failed
		if (moveresult.failure) {
			// reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach(bs->ms);
			//
			bs->activatestack->time = 0;
		}
		// check if the bot is blocked
		BotAIBlocked(bs, &moveresult, qtrue);
	}

	// if the bot has to shoot to activate
	if (bs->activatestack->shoot) {
		// if the view angles aren't yet used for the movement
		if (!(moveresult.flags & MOVERESULT_MOVEMENTVIEW)) {
			VectorSubtract(bs->activatestack->target, bs->eye, dir);
			vectoangles(dir, moveresult.ideal_viewangles);
			moveresult.flags |= MOVERESULT_MOVEMENTVIEW;
		}
		// if there's no weapon yet used for the movement
		if (!(moveresult.flags & MOVERESULT_MOVEMENTWEAPON)) {
			moveresult.flags |= MOVERESULT_MOVEMENTWEAPON;
			//
			bs->activatestack->weapon = BotSelectActivateWeapon(bs);
			if (bs->activatestack->weapon == -1) {
				// FIXME: find a decent weapon first
				bs->activatestack->weapon = 0;
			}
			moveresult.weapon = bs->activatestack->weapon;
		}
	}
	// if the ideal view angles are set for movement
	if (moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	}
	// if waiting for something
	else if (moveresult.flags & MOVERESULT_WAITING) {
		if (random() < bs->thinktime * 0.8) {
			BotRoamGoal(bs, target);
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if (!(bs->flags & BFL_IDEALVIEWSET)) {
		if (trap_BotMovementViewTarget(bs->ms, goal, bs->tfl, 300, target)) {
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
		} else {
			vectoangles(moveresult.movedir, bs->ideal_viewangles);
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	// if the weapon is used for the bot movement
	if (moveresult.flags & MOVERESULT_MOVEMENTWEAPON)
		bs->weaponnum = moveresult.weapon;
	// if there is an enemy
	if (BotFindEnemy(bs, -1)) {
		if (BotWantsToRetreat(bs)) {
			// keep the current long term goal and retreat
			AIEnter_Battle_NBG(bs, "activate entity: found enemy");
		} else {
			trap_BotResetLastAvoidReach(bs->ms);
			// empty the goal stack
			trap_BotEmptyGoalStack(bs->gs);
			// go fight
			AIEnter_Battle_Fight(bs, "activate entity: found enemy");
		}
		BotClearActivateGoalStack(bs);
	}
	return qtrue;
}

/*
==================
AIEnter_Seek_NBG
==================
*/
void AIEnter_Seek_NBG(bot_state_t *bs, char *s) {
	bot_goal_t goal;
	char buf[144];

	if (trap_BotGetTopGoal(bs->gs, &goal)) {
		trap_BotGoalName(goal.number, buf, 144);
		BotRecordNodeSwitch(bs, "seek NBG", buf, s);
	} else {
		BotRecordNodeSwitch(bs, "seek NBG", "no goal", s);
	}
	bs->ainode = AINode_Seek_NBG;
}

/*
==================
AINode_Seek_NBG
==================
*/
int AINode_Seek_NBG(bot_state_t *bs) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;

	if (BotIsObserver(bs)) {
		AIEnter_Observer(bs, "seek nbg: observer");
		return qfalse;
	}
	// if in the intermission
	if (BotIntermission(bs)) {
		AIEnter_Intermission(bs, "seek nbg: intermision");
		return qfalse;
	}
	// respawn if dead
	if (BotIsDead(bs)) {
		AIEnter_Respawn(bs, "seek nbg: bot dead");
		return qfalse;
	}
	if (FT_PlayerIsFrozen(&g_entities[bs->client])) {
		AIEnterFrozen(bs, "seek ltg: frozen");
		return qfalse;
	}
	//
	bs->tfl = TFL_DEFAULT;
	/*	if( bs->inventory[INVENTORY_JUMPER] ){
			bs->tfl |= TFL_JUMPER;
			bs->tfl &= ~TFL_JUMP;
		}
	*/
	// if in lava or slime the bot should be able to get out
	if (BotInLavaOrSlime(bs))
		bs->tfl |= TFL_LAVA | TFL_SLIME;
	//

	// map specific code
	BotMapScripts(bs);
	// no enemy
	bs->enemy = -1;
	// if the bot has no goal
	if (!trap_BotGetTopGoal(bs->gs, &goal))
		bs->nbg_time = 0;
	// if the bot touches the current goal
	else if (BotReachedGoal(bs, &goal)) {
		BotChooseWeapon(bs);
		bs->nbg_time = 0;
	}
	//
	if (bs->nbg_time < FloatTime()) {
		// pop the current goal from the stack
		trap_BotPopGoal(bs->gs);
		// check for new nearby items right away
		// NOTE: we canNOT reset the check_time to zero because it would create an endless loop of node switches
		bs->check_time = FloatTime() + 0.05;
		// go back to seek ltg
		AIEnter_Seek_LTG(bs, "seek nbg: time out");
		return qfalse;
	}
	// predict obstacles
	if (BotAIPredictObstacles(bs, &goal))
		return qfalse;
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the goal
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if (moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		bs->nbg_time = 0;
	}
	// check if the bot is blocked
	BotAIBlocked(bs, &moveresult, qtrue);

	// if the viewangles are used for the movement
	if (moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	}
	// if waiting for something
	else if (moveresult.flags & MOVERESULT_WAITING) {
		if (random() < bs->thinktime * 0.8) {
			BotRoamGoal(bs, target);
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if (!(bs->flags & BFL_IDEALVIEWSET)) {
		if (!trap_BotGetSecondGoal(bs->gs, &goal))
			trap_BotGetTopGoal(bs->gs, &goal);
		if (trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
		}
		// FIXME: look at cluster portals?
		else
			vectoangles(moveresult.movedir, bs->ideal_viewangles);
		bs->ideal_viewangles[2] *= 0.5;
	}
	// if the weapon is used for the bot movement
	if (moveresult.flags & MOVERESULT_MOVEMENTWEAPON)
		bs->weaponnum = moveresult.weapon;
	// if there is an enemy
	if (BotFindEnemy(bs, -1)) {
		if (BotWantsToRetreat(bs)) {
			// keep the current long term goal and retreat
			AIEnter_Battle_NBG(bs, "seek nbg: found enemy");
		} else {
			trap_BotResetLastAvoidReach(bs->ms);
			// empty the goal stack
			trap_BotEmptyGoalStack(bs->gs);
			// go fight
			AIEnter_Battle_Fight(bs, "seek nbg: found enemy");
		}
	}
	return qtrue;
}

// cyr{
qboolean BotDefendsCapturedBalloon(bot_state_t *bs) {
	int index, state;

	if (gametype != GT_BALLOON || bs->ltgtype != LTG_DEFENDKEYAREA)
		return qfalse;

	index = g_entities[bs->teamgoal.entitynum].count;
	state = level.balloonState[index]; // status of goal i
	// G_Printf("#%d -> %d ",index, state);
	if ((BotTeam(bs) == TEAM_RED && state == '1') || (BotTeam(bs) == TEAM_BLUE && state == '2'))
		return qtrue;
	else
		return qfalse;
}

void CheckHealth(bot_state_t *bs) {
	int i, dist, bestdist, bestnum, health, maxH;
	float res;

	bestnum = 0;
	bestdist = 99999;
	health = bs->inventory[INVENTORY_HEALTH];
	maxH = g_entities[bs->client].client->pers.maxHealth;

	if (bs->checkhealth_time > FloatTime()) { // recalc only every 2 sec
		// set ltgtype if station# was set at last calc (ltgtype gets lost every frame)
		if (bs->ltgtype == 0 && bs->hstationgoal != 0) // hstationgoal equals station index +1 or 0(for "no station")
			bs->ltgtype = LTG_GO_FOR_HEALTH;
		return;
	}
	bs->checkhealth_time = FloatTime() + 2;

	// never disturb any other LTG type
	if (bs->ltgtype != 0 && bs->ltgtype != LTG_GO_FOR_HEALTH &&
		!BotDefendsCapturedBalloon(bs)) // bot has a task (which is not "defend captured balloon")
		return;

	if (health >= maxH) {
		bs->ltgtype = 0;
		bs->hstationgoal = 0;
		return;
	}

	for (i = 0; i < num_hstations; i++) {
		// drop empty stations
		if (!g_entities[hstations[i].entitynum].health)
			continue;

		dist = BotClientTravelTimeToGoal(bs->client, &hstations[i]);
		if (dist && dist < bestdist) {
			bestnum = i;
			bestdist = dist;
		}
	}

	res = (maxH - health) * (maxH - health) * 0.3f; // a value < 3000
	res *= 1.0f / bestdist;

	if (res < 1) { // leave it
		bs->ltgtype = 0;
		bs->hstationgoal = 0;
	} else {
		bs->ltgtype = LTG_GO_FOR_HEALTH;
		bs->hstationgoal = bestnum + 1;
	}
}

/*bot_state_t* BotStateForName( char* botname ){
	int i;
	char buf[MAX_INFO_STRING];
	char name[MAX_INFO_STRING];
	bot_state_t* bs;

	for( i=0; i<MAX_CLIENTS; i++ ){
		trap_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		strncpy(name, Info_ValueForKey(buf, "n"), MAX_INFO_STRING-1);
		name[MAX_INFO_STRING-1] = '\0';
		Q_CleanStr( name );
		if( !Q_stricmp(name, botname) )
			break;
	}

	if( i == MAX_CLIENTS)
		return NULL;

	bs = botstates[i];
	if(!bs->inuse)
		return NULL;
	return bs;
}*/

bot_state_t *BotStateForEntNum(int ent) {
	if (!g_entities[ent].inuse || !g_entities[ent].client || !(g_entities[ent].r.svFlags & SVF_BOT))
		return NULL;

	return botstates[ent];
}

void Pos2Goal(vec3_t pos, bot_goal_t *goal) {
	VectorCopy(pos, goal->origin);
	goal->areanum = BotPointAreaNum(pos);
	VectorSet(goal->mins, -8, -8, -8);
	VectorSet(goal->maxs, 8, 8, 8);
}

void BotCamMoveTo(int ent, vec3_t pos) {
	bot_state_t *bs = BotStateForEntNum(ent);

	if (!bs)
		return;

	Pos2Goal(pos, &bs->teamgoal);
	if (!bs->teamgoal.areanum)
		return;
	bs->cam_movement = BCM_MOVETO;
}

void BotCamTaunt(int id) {
	bot_state_t *bs = BotStateForEntNum(id);
	bs->cam_taunt = qtrue;
}

void BotCamFire(int id) {
	bot_state_t *bs = BotStateForEntNum(id);
	bs->cam_fire = qtrue;
}

void BotChooseWeap(int id, int weap) {
	bot_state_t *bs = BotStateForEntNum(id);
	bs->weaponnum = weap;
}

void BotCamViewEntitiy(int id, int otherId) {
	bot_state_t *bs = BotStateForEntNum(id);
	bs->cam_viewmode = CWM_ENTITY;
	bs->cam_target = otherId;
}

void BotCamViewTarget(int id, vec3_t pos) {
	bot_state_t *bs = BotStateForEntNum(id);
	vec3_t dir;
	bs->cam_viewmode = CWM_TARGET;
	VectorSubtract(pos, bs->origin, dir);
	vectoangles(dir, bs->cam_angles);
}

void BotCamViewangles(int id, vec3_t angles) {
	bot_state_t *bs = BotStateForEntNum(id);
	bs->cam_viewmode = CWM_ANGLES;
	VectorCopy(angles, bs->cam_angles);
}

int AINode_Cam(bot_state_t *bs) {

	if (bs->cam_taunt) {
		trap_EA_Gesture(bs->client);
		bs->cam_taunt = qfalse;
	}
	if (bs->cam_fire) {
		trap_EA_Attack(bs->client);
		bs->cam_fire = qfalse;
	}

	switch (bs->cam_movement) {
	case BCM_IDLE:
		break;
	case BCM_MOVETO: {
		bot_moveresult_t moveresult;
		vec3_t target, dir;
		BotSetupForMovement(bs);
		// move towards the goal
		trap_BotMoveToGoal(&moveresult, bs->ms, &bs->teamgoal, TFL_DEFAULT);
		if (trap_BotMovementViewTarget(bs->ms, &bs->teamgoal, TFL_DEFAULT, 300, target)) {
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
		}

		if (trap_BotTouchingGoal(bs->origin, &bs->teamgoal))
			bs->cam_movement = BCM_IDLE;
	} break;
	default: // nothing
		break;
	}

	switch (bs->cam_viewmode) {
	case CWM_MOVEMENT:
		break; // movement sets viewangles allready
	case CWM_ENTITY: {
		bot_state_t *bs2 = BotStateForEntNum(bs->cam_target);
		vec3_t dir;
		VectorSubtract(bs2->origin, bs->origin, dir);
		vectoangles(dir, bs->ideal_viewangles);
		break;
	}
	case CWM_TARGET:
	case CWM_ANGLES:
		VectorCopy(bs->cam_angles, bs->ideal_viewangles);
	}
	return qtrue;
}

/*
==================
AIEnter_Seek_LTG
==================
*/
void AIEnter_Seek_LTG(bot_state_t *bs, char *s) {
	bot_goal_t goal;
	char buf[144];

	if (trap_BotGetTopGoal(bs->gs, &goal)) {
		trap_BotGoalName(goal.number, buf, 144);
		BotRecordNodeSwitch(bs, "seek LTG", buf, s);
	} else {
		BotRecordNodeSwitch(bs, "seek LTG", "no goal", s);
	}
	bs->ainode = AINode_Seek_LTG;
}

/*
==================
AINode_Seek_LTG
==================
*/
int AINode_Seek_LTG(bot_state_t *bs) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	int range;
	// char buf[128];
	// bot_goal_t tmpgoal;

	if (BotIsObserver(bs)) {
		AIEnter_Observer(bs, "seek ltg: observer");
		return qfalse;
	}
	// if in the intermission
	if (BotIntermission(bs)) {
		AIEnter_Intermission(bs, "seek ltg: intermission");
		return qfalse;
	}

	if (FT_PlayerIsFrozen(&g_entities[bs->client])) {
		AIEnterFrozen(bs, "seek ltg: frozen");
		return qfalse;
	}
	// respawn if dead
	if (BotIsDead(bs)) {
		AIEnter_Respawn(bs, "seek ltg: bot dead");
		return qfalse;
	}
	//
	if (BotChat_Random(bs)) {
		bs->stand_time = FloatTime() + BotChatTime(bs);
		AIEnter_Stand(bs, "seek ltg: random chat");
		return qfalse;
	}
	//
	bs->tfl = TFL_DEFAULT;
	/*	if( bs->inventory[INVENTORY_JUMPER] ){
			bs->tfl |= TFL_JUMPER;
			bs->tfl &= ~TFL_JUMP;
		}
	*/
	// if in lava or slime the bot should be able to get out
	if (BotInLavaOrSlime(bs))
		bs->tfl |= TFL_LAVA | TFL_SLIME;
	//

	// map specific code
	BotMapScripts(bs);
	// reset enemy player (not wall)
	if (!IsWall(bs->enemy))
		bs->enemy = -1;
	//
	if (bs->killedenemy_time > FloatTime() - 2) {
		if (random() < bs->thinktime * 1) {
			trap_EA_Gesture(bs->client);
		}
	}
	// if enemy is not a wall, search an enemy
	// if we find a human or duck enemy: check retreat
	if (bs->enemy == -1 && BotFindEnemy(bs, -1) && !IsWall(bs->enemy)) {
		if (BotWantsToRetreat(bs)) {
			// keep the current long term goal and retreat
			AIEnter_Battle_Retreat(bs, "seek ltg: found enemy");
			return qfalse;
		} else {
			trap_BotResetLastAvoidReach(bs->ms);
			// empty the goal stack
			trap_BotEmptyGoalStack(bs->gs);
			// go fight
			AIEnter_Battle_Fight(bs, "seek ltg: found enemy");
			return qfalse;
		}
	}
	//
	BotTeamGoals(bs, qfalse);
	// cyr{
	// check health
	CheckHealth(bs);
	// cyr}
	// get the current long term goal
	bs->cartthrown = qfalse;
	if (!BotLongTermGoal(bs, bs->tfl, qfalse, &goal)) {
		return qtrue;
	}
	if (bs->cartthrown)
		return qtrue;

	// check for nearby goals periodicly
	if (bs->check_time < FloatTime()) {
		int tt_ltg; // traveltime to LTG
		bs->check_time = FloatTime() + 0.5;

		if (bs->ltgtype == LTG_DEFENDKEYAREA)
			range = 400;
		else if (bs->ltgtype == LTG_UNFREEZE)
			range = 50;
		else
			range = 150;

		//
		if (gametype == GT_BALLOON) {
			if (bs->ltgtype == LTG_BALLCAMP)
				range = 0;
			else if (bs->ltgtype == LTG_ATTACKENEMYBASE)
				range = 50;
			else if (bs->ltgtype == LTG_DEFENDKEYAREA)
				range = 100;

			// range *= scorealert[BotTeam(bs)];
		}

		if (bs->ltgtype == LTG_CAPTUREFLAG) //|| bs->ltgtype == LTG_GETFLAG )
			range = 20;

		// make sure to never to for a NBG that is further away from the bot than the LTG
		tt_ltg = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, goal.areanum, bs->tfl);
		if (tt_ltg && tt_ltg < range)
			range = tt_ltg;

		//
		if (BotNearbyGoal(bs, bs->tfl, &goal, range)) {
			bot_goal_t nbg;
			int tt_nbg;
			trap_BotGetTopGoal(bs->gs, &nbg);
			tt_nbg = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, nbg.areanum, bs->tfl);
			if (bot_developer.integer & AIDBG_GOAL)
				G_Printf("^2going for NBG with tt: %d, LTG has %d, range %d \n", tt_nbg, tt_ltg, range);
			trap_BotResetLastAvoidReach(bs->ms);
			// get the goal at the top of the stack
			// trap_BotGetTopGoal(bs->gs, &tmpgoal);
			// trap_BotGoalName(tmpgoal.number, buf, 144);
			// BotAI_Print(PRT_MESSAGE, "new nearby goal %s\n", buf);
			// time the bot gets to pick up the nearby goal item
			bs->nbg_time = FloatTime() + 4 + range * 0.01;
			AIEnter_Seek_NBG(bs, "ltg seek: nbg");
			return qfalse;
		}
	}
	// predict obstacles
	if (BotAIPredictObstacles(bs, &goal))
		return qfalse;
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the goal
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if (moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		// BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, qtrue);

	// if the viewangles are used for the movement
	if (moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
		BotAddInfo(bs, "angles: moveresult viewset", AIDBG_MOVE);
	}
	// if waiting for something
	else if (moveresult.flags & MOVERESULT_WAITING) {
		if (random() < bs->thinktime * 0.8) {
			BotAddInfo(bs, "angles: random roam goal", AIDBG_MOVE);
			BotRoamGoal(bs, target);
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if (!(bs->flags & BFL_IDEALVIEWSET)) {
		BotAddInfo(bs, "angles: look at goal", AIDBG_MOVE);
		if (trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
		}
		// FIXME: look at cluster portals?
		else if (VectorLengthSquared(moveresult.movedir)) {
			vectoangles(moveresult.movedir, bs->ideal_viewangles);
		} else if (random() < bs->thinktime * 0.8) {
			BotRoamGoal(bs, target);
			VectorSubtract(target, bs->origin, dir);
			vectoangles(dir, bs->ideal_viewangles);
			bs->ideal_viewangles[2] *= 0.5;
		}
		bs->ideal_viewangles[2] *= 0.5;
	} else {
		BotAddInfo(bs, "angles: independant", AIDBG_MOVE);
	}
	// if the weapon is used for the bot movement
	if (moveresult.flags & MOVERESULT_MOVEMENTWEAPON)
		bs->weaponnum = moveresult.weapon;
	//
	return qtrue;
}

/*
==================
AIEnter_Battle_Fight
==================
*/
void AIEnter_Battle_Fight(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "battle fight", "", s);
	trap_BotResetLastAvoidReach(bs->ms);
	bs->ainode = AINode_Battle_Fight;
}

/*
==================
AIEnter_Battle_Fight
==================
*/
void AIEnter_Battle_SuicidalFight(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "battle fight", "", s);
	trap_BotResetLastAvoidReach(bs->ms);
	bs->ainode = AINode_Battle_Fight;
	bs->flags |= BFL_FIGHTSUICIDAL;
}

/*
==================
AINode_Battle_Fight
==================
*/
int AINode_Battle_Fight(bot_state_t *bs) {
	int areanum;
	vec3_t target;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;

	if (BotIsObserver(bs)) {
		AIEnter_Observer(bs, "battle fight: observer");
		return qfalse;
	}

	// if in the intermission
	if (BotIntermission(bs)) {
		AIEnter_Intermission(bs, "battle fight: intermission");
		return qfalse;
	}
	// respawn if dead
	if (BotIsDead(bs)) {
		AIEnter_Respawn(bs, "battle fight: bot dead");
		return qfalse;
	}
	if (FT_PlayerIsFrozen(&g_entities[bs->client])) {
		AIEnterFrozen(bs, "seek ltg: frozen");
		return qfalse;
	}
	if (BotCheckChargedImp(bs)) {
		return qtrue;
	}

	if (ClientInSprayroom(bs->client)) {
		AIEnter_Seek_LTG(bs, "battle fight: ran into sprayroom");
		return qfalse;
	}

	// if there is another better enemy
	if (BotFindEnemy(bs, bs->enemy)) {
#ifdef DEBUG
		BotAI_Print(PRT_MESSAGE, "found new better enemy\n");
#endif
	}

	// if no enemy
	if (bs->enemy < 0 || BotNmyTurnedInvalid(bs)) {
		AIEnter_Seek_LTG(bs, "battle fight: no enemy");
		return qfalse;
	}
	// eigena cyr {
	if (trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, bs->teamgoal.areanum, TFL_DEFAULT) > 250) {
		AIEnter_Battle_Retreat(bs, "battle fight: wants to retreat");
	}
	// eigene cyr }

	//
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		AIEnter_Seek_LTG(bs, "battle fight: enemy invalid");
		return qfalse;
	}

	// if the enemy is dead
	if (bs->enemydeath_time) {
		if (bs->enemydeath_time < FloatTime() - 1.0) {
			bs->enemydeath_time = 0;
			if (bs->enemysuicide) {
				BotChat_EnemySuicide(bs);
			}
			if (bs->lastkilledplayer == bs->enemy && BotChat_Kill(bs)) {
				bs->stand_time = FloatTime() + BotChatTime(bs);
				AIEnter_Stand(bs, "battle fight: enemy dead");
			} else {
				bs->ltg_time = 0;
				AIEnter_Seek_LTG(bs, "battle fight: enemy dead");
			}
			return qfalse;
		}
	} else {
		if (EntityIsDead(&entinfo)) {
			bs->enemydeath_time = FloatTime();
		}
	}
	// if the enemy is invisible and not shooting the bot looses track easily
	if (EntityIsInvisible(&entinfo) && !EntityIsShooting(&entinfo)) {
		if (random() < 0.2) {
			AIEnter_Seek_LTG(bs, "battle fight: invisible");
			return qfalse;
		}
	}
	//
	VectorCopy(entinfo.origin, target);
	// update the reachability area and origin if possible
	areanum = BotPointAreaNum(target);
	if (areanum && trap_AAS_AreaReachability(areanum)) {
		VectorCopy(target, bs->lastenemyorigin);
		bs->lastenemyareanum = areanum;
	}
	// update the attack inventory values
	BotUpdateBattleInventory(bs, bs->enemy);
	// if the bot's health decreased
	if (bs->lastframe_health > bs->inventory[INVENTORY_HEALTH]) {
		if (BotChat_HitNoDeath(bs)) {
			bs->stand_time = FloatTime() + BotChatTime(bs);
			AIEnter_Stand(bs, "battle fight: chat health decreased");
			return qfalse;
		}
	}
	// if the bot hit someone
	if (bs->cur_ps.persistant[PERS_HITS] > bs->lasthitcount) {
		if (BotChat_HitNoKill(bs)) {
			bs->stand_time = FloatTime() + BotChatTime(bs);
			AIEnter_Stand(bs, "battle fight: chat hit someone");
			return qfalse;
		}
	}
	// if the enemy is not visible
	if (!BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) {
		if (BotWantsToChase(bs)) {
			AIEnter_Battle_Chase(bs, "battle fight: enemy out of sight");
			return qfalse;
		} else {
			AIEnter_Seek_LTG(bs, "battle fight: enemy out of sight");
			return qfalse;
		}
	}
	// use holdable items
	BotBattleUseItems(bs);
	//
	bs->tfl = TFL_DEFAULT;
	/*	if( bs->inventory[INVENTORY_JUMPER] ){
			bs->tfl |= TFL_JUMPER;
			bs->tfl &= ~TFL_JUMP;
		}
	*/
	// if in lava or slime the bot should be able to get out
	if (BotInLavaOrSlime(bs))
		bs->tfl |= TFL_LAVA | TFL_SLIME;
	//

	// choose the best weapon to fight with
	BotChooseWeapon(bs);
	// do attack movements
	moveresult = BotAttackMove(bs, bs->tfl);
	// if the movement failed
	if (moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		// BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, qfalse);
	// aim at the enemy
	BotAimAtEnemy(bs);
	// attack the enemy if possible
	BotCheckAttack(bs);
	// if the bot wants to retreat
	if (!(bs->flags & BFL_FIGHTSUICIDAL)) {
		if (BotWantsToRetreat(bs)) {
			AIEnter_Battle_Retreat(bs, "battle fight: wants to retreat");
			return qtrue;
		}
	}
	return qtrue;
}

/*
==================
AIEnter_Battle_Chase
==================
*/
void AIEnter_Battle_Chase(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "battle chase", "", s);
	bs->chase_time = FloatTime();
	bs->ainode = AINode_Battle_Chase;
}

/*
==================
AINode_Battle_Chase
==================
*/
int AINode_Battle_Chase(bot_state_t *bs) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	aas_entityinfo_t entinfo;
	float range;

	if (BotIsObserver(bs)) {
		AIEnter_Observer(bs, "battle chase: observer");
		return qfalse;
	}
	// if in the intermission
	if (BotIntermission(bs)) {
		AIEnter_Intermission(bs, "battle chase: intermission");
		return qfalse;
	}
	// respawn if dead
	if (BotIsDead(bs)) {
		AIEnter_Respawn(bs, "battle chase: bot dead");
		return qfalse;
	}
	if (FT_PlayerIsFrozen(&g_entities[bs->client])) {
		AIEnterFrozen(bs, "seek ltg: frozen");
		return qfalse;
	}
	// if no enemy
	if (bs->enemy < 0 || BotNmyTurnedInvalid(bs)) {
		AIEnter_Seek_LTG(bs, "battle chase: no enemy");
		return qfalse;
	}
	// if the enemy is visible
	if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) {
		AIEnter_Battle_Fight(bs, "battle chase");
		return qfalse;
	}
	// if there is another enemy
	if (BotFindEnemy(bs, -1)) {
		AIEnter_Battle_Fight(bs, "battle chase: better enemy");
		return qfalse;
	}
	// there is no last enemy area
	if (!bs->lastenemyareanum) {
		AIEnter_Seek_LTG(bs, "battle chase: no enemy area");
		return qfalse;
	}

	if (ClientInSprayroom(bs->client)) {
		AIEnter_Seek_LTG(bs, "battle chase: ran into sprayroom");
		return qfalse;
	}

	//
	bs->tfl = TFL_DEFAULT;
	/*	if( bs->inventory[INVENTORY_JUMPER] ){
			bs->tfl |= TFL_JUMPER;
			bs->tfl &= ~TFL_JUMP;
		}
	*/

	// if in lava or slime the bot should be able to get out
	if (BotInLavaOrSlime(bs))
		bs->tfl |= TFL_LAVA | TFL_SLIME;

	// map specific code
	BotMapScripts(bs);
	// create the chase goal
	goal.entitynum = bs->enemy;
	goal.areanum = bs->lastenemyareanum;
	VectorCopy(bs->lastenemyorigin, goal.origin);
	VectorSet(goal.mins, -8, -8, -8);
	VectorSet(goal.maxs, 8, 8, 8);
	if (gametype == GT_LPS) { // bot knows enemies current position //cyr 20055
		BotEntityInfo(bs->enemy, &entinfo);
		if (!entinfo.valid) {
			AIEnter_Seek_LTG(bs, "battle fight: enemy invalid");
			return qfalse;
		}
		goal.areanum = BotPointAreaNum(entinfo.origin);
		VectorCopy(entinfo.origin, goal.origin);
	}
	// if the last seen enemy spot is reached the enemy could not be found
	if (trap_BotTouchingGoal(bs->origin, &goal))
		bs->chase_time = 0;
	// if there's no chase time left
	if (!bs->chase_time || bs->chase_time < FloatTime() - 10) {
		AIEnter_Seek_LTG(bs, "battle chase: time out");
		return qfalse;
	}
	// check for nearby goals periodicly
	if (bs->check_time < FloatTime()) {
		bs->check_time = FloatTime() + 1;
		range = 150;
		//
		if (BotNearbyGoal(bs, bs->tfl, &goal, range)) {
			// the bot gets 5 seconds to pick up the nearby goal item
			bs->nbg_time = FloatTime() + 0.1 * range + 1;
			trap_BotResetLastAvoidReach(bs->ms);
			AIEnter_Battle_NBG(bs, "battle chase: nbg");
			return qfalse;
		}
	}
	//
	BotUpdateBattleInventory(bs, bs->enemy);
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the goal
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if (moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		// BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, qfalse);
	//
	if (moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	} else if (!(bs->flags & BFL_IDEALVIEWSET)) {
		if (bs->chase_time > FloatTime() - 2) {
			BotAimAtEnemy(bs);
		} else {
			if (trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
				VectorSubtract(target, bs->origin, dir);
				vectoangles(dir, bs->ideal_viewangles);
			} else {
				vectoangles(moveresult.movedir, bs->ideal_viewangles);
			}
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	// if the weapon is used for the bot movement
	if (moveresult.flags & MOVERESULT_MOVEMENTWEAPON)
		bs->weaponnum = moveresult.weapon;
	// if the bot is in the area the enemy was last seen in
	if (bs->areanum == bs->lastenemyareanum)
		bs->chase_time = 0;
	// if the bot wants to retreat (the bot could have been damage during the chase)
	if (BotWantsToRetreat(bs)) {
		AIEnter_Battle_Retreat(bs, "battle chase: wants to retreat");
		return qtrue;
	}
	return qtrue;
}

/*
==================
AIEnter_Battle_Retreat
==================
*/
void AIEnter_Battle_Retreat(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "battle retreat", "", s);
	bs->ainode = AINode_Battle_Retreat;
}

/*
==================
AINode_Battle_Retreat
==================
*/
int AINode_Battle_Retreat(bot_state_t *bs) {
	bot_goal_t goal;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	vec3_t target, dir;
	float attack_skill, range;
	int areanum;

	if (BotIsObserver(bs)) {
		AIEnter_Observer(bs, "battle retreat: observer");
		return qfalse;
	}
	// if in the intermission
	if (BotIntermission(bs)) {
		AIEnter_Intermission(bs, "battle retreat: intermission");
		return qfalse;
	}
	// respawn if dead
	if (BotIsDead(bs)) {
		AIEnter_Respawn(bs, "battle retreat: bot dead");
		return qfalse;
	}
	if (FT_PlayerIsFrozen(&g_entities[bs->client])) {
		AIEnterFrozen(bs, "seek ltg: frozen");
		return qfalse;
	}
	if (BotCheckChargedImp(bs)) {
		return qtrue;
	}

	if (ClientInSprayroom(bs->client)) {
		AIEnter_Seek_LTG(bs, "battle retreat: ran into sprayroom");
		return qfalse;
	}

	// if no enemy
	if (bs->enemy < 0 || BotNmyTurnedInvalid(bs)) {
		AIEnter_Seek_LTG(bs, "battle retreat: no enemy");
		return qfalse;
	}
	//
	BotEntityInfo(bs->enemy, &entinfo);

	if (!entinfo.valid || EntityIsDead(&entinfo)) {
		AIEnter_Seek_LTG(bs, "battle retreat: enemy dead");
		return qfalse;
	}
	// if there is another better enemy
	if (BotFindEnemy(bs, bs->enemy)) {
#ifdef DEBUG
		BotAI_Print(PRT_MESSAGE, "found new better enemy\n");
#endif
	}
	//
	bs->tfl = TFL_DEFAULT;
	/*	if( bs->inventory[INVENTORY_JUMPER] ){
			bs->tfl |= TFL_JUMPER;
			bs->tfl &= ~TFL_JUMP;
		}
	*/
	// if in lava or slime the bot should be able to get out
	if (BotInLavaOrSlime(bs))
		bs->tfl |= TFL_LAVA | TFL_SLIME;
	// map specific code
	BotMapScripts(bs);
	// update the attack inventory values
	BotUpdateBattleInventory(bs, bs->enemy);
	// if the bot doesn't want to retreat anymore... probably picked up some nice items
	if (BotWantsToChase(bs)) {
		// empty the goal stack, when chasing, only the enemy is the goal
		trap_BotEmptyGoalStack(bs->gs);
		// go chase the enemy
		AIEnter_Battle_Chase(bs, "battle retreat: wants to chase");
		return qfalse;
	}
	// update the last time the enemy was visible
	if (gametype == GT_LPS || // in lps the bot allways known the enemy position	// cyr 20055
		BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) {
		bs->enemyvisible_time = FloatTime();
		VectorCopy(entinfo.origin, target);
		// update the reachability area and origin if possible
		areanum = BotPointAreaNum(target);
		if (areanum && trap_AAS_AreaReachability(areanum)) {
			VectorCopy(target, bs->lastenemyorigin);
			bs->lastenemyareanum = areanum;
		}
	}
	// if the enemy is NOT visible for 4 seconds
	if (bs->enemyvisible_time < FloatTime() - 4) {
		AIEnter_Seek_LTG(bs, "battle retreat: lost enemy");
		return qfalse;
	}
	// else if the enemy is NOT visible
	else if (bs->enemyvisible_time < FloatTime()) {
		// if there is another enemy
		if (BotFindEnemy(bs, -1)) {
			AIEnter_Battle_Fight(bs, "battle retreat: another enemy");
			return qfalse;
		}
	}
	//
	BotTeamGoals(bs, qtrue);
	// use holdable items
	BotBattleUseItems(bs);
	// get the current long term goal while retreating
	bs->cartthrown = qfalse;
	if (!BotLongTermGoal(bs, bs->tfl, qtrue, &goal)) {
		AIEnter_Battle_SuicidalFight(bs, "battle retreat: no way out");
		return qfalse;
	}

	if (bs->cartthrown)
		return qtrue; // cyr_ptr

	// check for nearby goals periodicly
	if (bs->check_time < FloatTime()) {
		bs->check_time = FloatTime() + 1;
		range = 150;

		if (gametype == GT_BALLOON) {
			if (bs->ltgtype == LTG_BALLCAMP)
				range = 0;
			else if (bs->ltgtype == LTG_ATTACKENEMYBASE)
				range = 50;
			else if (bs->ltgtype == LTG_DEFENDKEYAREA)
				range = 150;
			// range *= scorealert[BotTeam(bs)];
		}
		// the more carts you carry the less you care for items
		else if (gametype == GT_SPRAY || gametype == GT_SPRAYFFA) {
			// 0 carts -> 150 ... 8 carts -> 0
			range -= (float)150 / 8 * bs->inventory[INVENTORY_SPRAYPISTOLAMMO];
		}

		//
		if (BotNearbyGoal(bs, bs->tfl, &goal, range)) {
			trap_BotResetLastAvoidReach(bs->ms);
			// time the bot gets to pick up the nearby goal item
			bs->nbg_time = FloatTime() + range / 100 + 1;
			AIEnter_Battle_NBG(bs, "battle retreat: nbg");
			return qfalse;
		}
	}
	// initialize the movement state
	BotSetupForMovement(bs);

	if (gametype == GT_BALLOON && (bs->ltgtype == LTG_ATTACKENEMYBASE || bs->ltgtype == LTG_BALLCAMP)) {
		vec3_t dir;
		VectorSubtract(bs->teamgoal.origin, bs->origin, dir);
		// if dist < radius, attackmove, else movetogoal
		if (VectorLengthSquared(dir) < Square(75))
			moveresult = BotAttackMove(bs, bs->tfl);
		else
			trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	} else if (IsBambam(bs->enemy)) {
		moveresult = BotAttackMove(bs, bs->tfl);
	} else if (IsBoomie(bs->enemy)) {
		// freeze if enemy is boomie and bot is close to the trigger
		aas_entityinfo_t info;
		vec3_t dir;
		BotEntityInfo(bs->enemy, &info);
		if (info.valid) {
			VectorSubtract(info.origin, bs->origin, dir);
			if (VectorLengthSquared(dir) < Square(BOOMIES_TRIGGERBOX * 4)) {
				moveresult = BotAttackMove(bs, bs->tfl);
			} else {
				trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
			}
		} else {
			trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
		}
	} else if (bs->ltgtype == LTG_UNFREEZE) {
		if (FT_InThawingRange(&g_entities[bs->client], &g_entities[bs->teammate]))
			moveresult = BotAttackMove(bs, bs->tfl);
		else
			trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	} else {
		// move towards the goal
		trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	}

	// if the movement failed
	if (moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		// BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, qfalse);
	// choose the best weapon to fight with
	BotChooseWeapon(bs);
	// if the view is fixed for the movement
	if (moveresult.flags & (MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	} else if (!(moveresult.flags & MOVERESULT_MOVEMENTVIEWSET) && !(bs->flags & BFL_IDEALVIEWSET)) {
		attack_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_ATTACK_SKILL, 0, 1);
		// if the bot is skilled enough
		if (attack_skill > 0.3) {
			BotAimAtEnemy(bs);
		} else {
			if (trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
				VectorSubtract(target, bs->origin, dir);
				vectoangles(dir, bs->ideal_viewangles);
			} else {
				vectoangles(moveresult.movedir, bs->ideal_viewangles);
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	// if the weapon is used for the bot movement
	if (moveresult.flags & MOVERESULT_MOVEMENTWEAPON)
		bs->weaponnum = moveresult.weapon;
	// attack the enemy if possible
	BotCheckAttack(bs);
	//
	return qtrue;
}

/*
==================
AIEnter_Battle_NBG
==================
*/
void AIEnter_Battle_NBG(bot_state_t *bs, char *s) {
	BotRecordNodeSwitch(bs, "battle NBG", "", s);
	bs->ainode = AINode_Battle_NBG;
}

/*
==================
AINode_Battle_NBG
==================
*/
int AINode_Battle_NBG(bot_state_t *bs) {
	int areanum;
	bot_goal_t goal;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	float attack_skill;
	vec3_t target, dir;

	if (BotIsObserver(bs)) {
		AIEnter_Observer(bs, "battle nbg: observer");
		return qfalse;
	}
	// if in the intermission
	if (BotIntermission(bs)) {
		AIEnter_Intermission(bs, "battle nbg: intermission");
		return qfalse;
	}
	// respawn if dead
	if (BotIsDead(bs)) {
		AIEnter_Respawn(bs, "battle nbg: bot dead");
		return qfalse;
	}
	if (FT_PlayerIsFrozen(&g_entities[bs->client])) {
		AIEnterFrozen(bs, "seek ltg: frozen");
		return qfalse;
	}

	if (BotCheckChargedImp(bs)) {
		return qtrue;
	}

	// if no enemy
	if (bs->enemy < 0 || BotNmyTurnedInvalid(bs)) {
		AIEnter_Seek_NBG(bs, "battle nbg: no enemy");
		return qfalse;
	}
	//
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid || EntityIsDead(&entinfo)) {
		AIEnter_Seek_NBG(bs, "battle nbg: enemy dead");
		return qfalse;
	}

	if (ClientInSprayroom(bs->client)) {
		AIEnter_Seek_LTG(bs, "battle nbg: ran into sprayroom");
		return qfalse;
	}

	//
	bs->tfl = TFL_DEFAULT;
	/*	if( bs->inventory[INVENTORY_JUMPER] ){
			bs->tfl |= TFL_JUMPER;
			bs->tfl &= ~TFL_JUMP;
		}
	*/
	// if in lava or slime the bot should be able to get out
	if (BotInLavaOrSlime(bs))
		bs->tfl |= TFL_LAVA | TFL_SLIME;
	//

	// map specific code
	BotMapScripts(bs);
	// update the last time the enemy was visible
	if (gametype == GT_LPS || // in lps the bot allways known the enemy position	// cyr 20055
		BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) {
		bs->enemyvisible_time = FloatTime();
		VectorCopy(entinfo.origin, target);
		// update the reachability area and origin if possible
		areanum = BotPointAreaNum(target);
		if (areanum && trap_AAS_AreaReachability(areanum)) {
			VectorCopy(target, bs->lastenemyorigin);
			bs->lastenemyareanum = areanum;
		}
	}
	// if the bot has no goal or touches the current goal
	if (!trap_BotGetTopGoal(bs->gs, &goal)) {
		bs->nbg_time = 0;
	} else if (BotReachedGoal(bs, &goal)) {
		bs->nbg_time = 0;
	}
	//
	if (bs->nbg_time < FloatTime()) {
		// pop the current goal from the stack
		trap_BotPopGoal(bs->gs);
		// if the bot still has a goal
		if (trap_BotGetTopGoal(bs->gs, &goal))
			AIEnter_Battle_Retreat(bs, "battle nbg: time out");
		else
			AIEnter_Battle_Fight(bs, "battle nbg: time out");
		//
		return qfalse;
	}
	// initialize the movement state
	BotSetupForMovement(bs);
	// move towards the goal
	trap_BotMoveToGoal(&moveresult, bs->ms, &goal, bs->tfl);
	// if the movement failed
	if (moveresult.failure) {
		// reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		// BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->nbg_time = 0;
	}
	//
	BotAIBlocked(bs, &moveresult, qfalse);
	// update the attack inventory values
	BotUpdateBattleInventory(bs, bs->enemy);
	// choose the best weapon to fight with
	BotChooseWeapon(bs);
	// if the view is fixed for the movement
	if (moveresult.flags & (MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
		VectorCopy(moveresult.ideal_viewangles, bs->ideal_viewangles);
	} else if (!(moveresult.flags & MOVERESULT_MOVEMENTVIEWSET) && !(bs->flags & BFL_IDEALVIEWSET)) {
		attack_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_ATTACK_SKILL, 0, 1);
		// if the bot is skilled enough and the enemy is visible
		if (attack_skill > 0.3) {
			//&& BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)
			BotAimAtEnemy(bs);
		} else {
			if (trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, 300, target)) {
				VectorSubtract(target, bs->origin, dir);
				vectoangles(dir, bs->ideal_viewangles);
			} else {
				vectoangles(moveresult.movedir, bs->ideal_viewangles);
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	// if the weapon is used for the bot movement
	if (moveresult.flags & MOVERESULT_MOVEMENTWEAPON)
		bs->weaponnum = moveresult.weapon;
	// attack the enemy if possible
	BotCheckAttack(bs);
	//
	return qtrue;
}
