/* 
 *  ZCam v1.0.4
 *  Spectator Camera for Quake III Arena
 *  Copyright (C), 2001 by Avi "Zung!" Rozen
 *	
 *  http://www.telefragged.com/zungbang/zcam
 *
 *  Credits:
 *  FLIC  camera mode is based on code  taken from q2cam by Paul Jordan
 *  SWING camera mode is based on ideas taken from CreepCam for Quake I 
 *
 * Zcam source code is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Zcam source code is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quake III Arena source code; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* Camera Target Selection */

#include "g_local.h"
#include "zcam.h"

/* IsVisible:
 * Is player #1 visible by player #2 ? 
 */
qboolean IsVisible(gentity_t * player1, gentity_t * player2, float maxrange)
{
	vec3_t length;
	float distance;
	trace_t trace;

	// check for looking through non-transparent water
	if (!trap_InPVS(player1->client->ps.origin, player2->client->ps.origin))
		return qfalse;

	trap_Trace(&trace,
		   player1->client->ps.origin,
		   NULL, NULL, player2->client->ps.origin, player1->s.clientNum, MASK_SOLID);

	VectorSubtract(player1->client->ps.origin, player2->client->ps.origin, length);
	distance = VectorLength(length);

	return ((maxrange == 0 || distance < maxrange) && trace.fraction == 1.0f);
}

/* NumPlayersVisible:
 * Return the number of players visible by the specified viewer.
 */
int NumPlayersVisible(gentity_t * viewer, float maxrange)
{
	int count = 0;
	int i;

	if (viewer == NULL) {
		return 0;
	}

	for (i = 0; i < level.maxclients; i++) {
		if (viewer->s.clientNum != i
		    && level.clients[i].pers.connected == CON_CONNECTED
		    && level.clients[i].sess.sessionTeam != TEAM_SPECTATOR && level.clients[i].ps.pm_type != PM_DEAD) {
			if (IsVisible(viewer, &g_entities[i], maxrange)) {
				count++;
			}
		}
	}

	return count;
}

/* PriorityTarget:
 * Override camera target selection. Returns the new favorite
 * player (or the one already selected).
 * Currently used to select flag carriers in CTF.
 */
gentity_t *PriorityTarget(gentity_t * target, qboolean * override)
{
	int i;
	gentity_t *favorite = NULL;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected == CON_CONNECTED
		    && level.clients[i].sess.sessionTeam != TEAM_SPECTATOR
		    && level.clients[i].ps.pm_type == PM_NORMAL && (level.clients[i].ps.powerups[PW_REDFLAG]
								    || level.clients[i].ps.powerups[PW_BLUEFLAG])) {
			if (favorite == NULL
			    || NumPlayersVisible(favorite, MAX_VISIBLE_RANGE) < NumPlayersVisible(&g_entities[i],
												  MAX_VISIBLE_RANGE)) {
				favorite = &g_entities[i];
			}
		}
	}

	/* set override flag */
	if (favorite)
		*override = qtrue;
	else
		*override = qfalse;
	return (favorite) ? favorite : target;
}

/* PlayerToFollow:
 * Select camera target. Selects the player that sees the maximum
 * number of other players, unless a priority target exists (as 
 * determined by the function PriorityTarget above).
 */
gentity_t *PlayerToFollow(gentity_t * ent, qboolean * override)
{
	gentity_t *viewer;
	gentity_t *best = NULL;
	int i, players, best_count = -1;

	*override = qfalse;

	for (i = 0; i < level.maxclients; i++) {
		/* don't switch to dead people */
		viewer = &g_entities[i];

		if (viewer->client->pers.connected != CON_CONNECTED)
			continue;

		if (viewer->client->sess.sessionTeam != TEAM_SPECTATOR && viewer->client->ps.pm_type == PM_NORMAL) {
			players = NumPlayersVisible(viewer, MAX_VISIBLE_RANGE);
			if (players > best_count) {
				best_count = players;
				best = viewer;
			} else if (players == best_count) {
				if (best->client->ps.persistant[PERS_SCORE] < viewer->client->ps.persistant[PERS_SCORE]) {
					best = viewer;
				}
			}
		}
	}

	if (best == NULL)
		return best;

	return PriorityTarget(best, override);
}

/* PlayerToTrack:
 *  select swing camera target in the following priority order:
 *  - enemy flag carrier
 *  - closest enemy player
 *  - own team flag carrier
 *  - closest own team player
 *  (in DM this amount to picking the closest player)
 */
gentity_t *PlayerToTrack(gentity_t * ent, gentity_t * target1st)
{
	int i;
	gentity_t *target2nd;
	gentity_t *best1 = NULL;
	gentity_t *best2 = NULL;
	vec3_t distance;
	float current, closest1 = -1.0F, closest2 = -1.0F;

	for (i = 0; i < level.maxclients; i++) {
		target2nd = &g_entities[i];
		if (target1st != target2nd
		    && target2nd->client->pers.connected == CON_CONNECTED
		    && target2nd->client->sess.sessionTeam != TEAM_SPECTATOR
		    && target2nd->client->ps.pm_type == PM_NORMAL
		    && trap_InPVS(target1st->client->ps.origin, target2nd->client->ps.origin)) {
			VectorSubtract(target1st->client->ps.origin, target2nd->client->ps.origin, distance);
			current = VectorLength(distance);
			if (target1st->client->sess.sessionTeam == target2nd->client->sess.sessionTeam) {
				if (target2nd->client->ps.powerups[PW_REDFLAG]
				    || target2nd->client->ps.powerups[PW_BLUEFLAG]) {
					best1 = target2nd;
				}

				if (!(best1 && (best1->client->ps.powerups[PW_REDFLAG]
						|| best1->client->ps.powerups[PW_BLUEFLAG]))
				    && (closest1 < 0 || current < closest1)) {
					best1 = target2nd;
					closest1 = current;
				}
			} else {
				if (target2nd->client->ps.powerups[PW_REDFLAG]
				    || target2nd->client->ps.powerups[PW_BLUEFLAG]) {
					best2 = target2nd;
				}

				if (!(best2 && (best2->client->ps.powerups[PW_REDFLAG]
						|| best2->client->ps.powerups[PW_BLUEFLAG]))
				    && (closest2 < 0 || current < closest2)) {
					best2 = target2nd;
					closest2 = current;
				}
			}
		}
	}
	return ((best2) ? best2 : best1);
}
