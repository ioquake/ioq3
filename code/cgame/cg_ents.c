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
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"

refEntity_t trailEnt; // HERBY

/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName) {
	int i;
	orientation_t lerped;

	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame, 1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	VectorCopy(parent->origin, entity->origin);
	for (i = 0; i < 3; i++) {
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(lerped.axis, ((refEntity_t *)parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}

/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel,
								   char *tagName) {
	int i;
	orientation_t lerped;
	vec3_t tempAxis[3];

	// AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame, 1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	VectorCopy(parent->origin, entity->origin);
	for (i = 0; i < 3; i++) {
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(entity->axis, lerped.axis, tempAxis);
	MatrixMultiply(tempAxis, ((refEntity_t *)parent)->axis, entity->axis);
}

/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition(centity_t *cent) {
	if (cent->currentState.solid == SOLID_BMODEL) {
		vec3_t origin;
		float *v;

		v = cgs.inlineModelMidpoints[cent->currentState.modelindex];
		VectorAdd(cent->lerpOrigin, v, origin);
		trap_S_UpdateEntityPosition(cent->currentState.number, origin);
	} else {
		trap_S_UpdateEntityPosition(cent->currentState.number, cent->lerpOrigin);
	}
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects(centity_t *cent) {

	// update sound origins
	CG_SetEntitySoundPosition(cent);

	// add loop sound
	if (cent->currentState.loopSound) {
		if (cent->currentState.eType != ET_SPEAKER) {
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin,
								   cgs.gameSounds[cent->currentState.loopSound]);
		} else {
			trap_S_AddRealLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin,
									   cgs.gameSounds[cent->currentState.loopSound]);
		}
	}

	// constant light glow
	if (cent->currentState.constantLight) {
		int cl;
		float i, r, g, b;

		cl = cent->currentState.constantLight;
		r = (float)(cl & 0xFF) / 255.0;
		g = (float)((cl >> 8) & 0xFF) / 255.0;
		b = (float)((cl >> 16) & 0xFF) / 255.0;
		i = (float)((cl >> 24) & 0xFF) * 4.0;
		trap_R_AddLightToScene(cent->lerpOrigin, i, r, g, b);
	}
}

/*
==================
CG_General
==================
*/
static void CG_General(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currentState;

	// Boaster-Slik-ents ... don't know why I coded this stuff in such a ugly way (without a new ent-type)
	if (s1->weapon == 23) // extreeeeeeeeeeem-varmissbrauching =)
	{
		if (cent->miscTime != 1) // misbraucht ;)
			CG_ImpactMark(/*cgs.media.energyMarkShader*/ cgs.media.SchaumShader, s1->origin, s1->angles, s1->time2,
						  0.5f, 0.5f, 1, 1, qfalse, s1->generic1, qfalse);
		cent->miscTime = 1;
	}

	// if set to invisible, skip
	if (!s1->modelindex) {
		return;
	}

	memset(&ent, 0, sizeof(ent));

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if (s1->number == cg.snap->ps.clientNum) {
		ent.renderfx |= RF_THIRD_PERSON; // only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis(cent->lerpAngles, ent.axis);

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker(centity_t *cent) {
	if (cent->miscTime == 0 && (cent->currentState.frame || cent->currentState.clientNum))
		cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();

	if (!cent->currentState.clientNum) { // FIXME: use something other than clientNum...
		return;							 // not auto triggering
	}

	if (cg.time < cent->miscTime) {
		return;
	}

	trap_S_StartSound(NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm]);

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

/*
==================
CG_Item
==================
*/
static void CG_Item(centity_t *cent) {
	refEntity_t ent;
	entityState_t *es;
	gitem_t *item;
	int msec;
	float frac;
	float scale;
	weaponInfo_t *wi;

	es = &cent->currentState;
	if (es->modelindex >= bg_numItems) {
		CG_Error("Bad item index %i on entity", es->modelindex);
	}

	// if set to invisible, skip
	if (!es->modelindex || (es->eFlags & EF_NODRAW)) {
		return;
	}

	item = &bg_itemlist[es->modelindex];
	if (cg_simpleItems.integer && item->giType != IT_TEAM) {
		memset(&ent, 0, sizeof(ent));
		ent.reType = RT_SPRITE;
		VectorCopy(cent->lerpOrigin, ent.origin);
		ent.radius = 14;

		if (es->modelindex == cgs.media.neutralCartridgeEntNum &&
			cent->currentState.otherEntityNum == cg.snap->ps.clientNum)
			ent.customShader = cg_items[cgs.media.redCartridgeEntNum].icon;
		else
			ent.customShader = cg_items[es->modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	// items bob up and down continuously
	scale = 0.005 + cent->currentState.number * 0.00001;
	cent->lerpOrigin[2] += 4 + cos((cg.time + 1000) * scale) * 4;

	memset(&ent, 0, sizeof(ent));

	// autorotate at one of two speeds
	if (item->giType == IT_POWERUP || item->giType == IT_HOLDABLE) {
		ent.oldframe = ent.frame;

		cent->lerpAngles[0] = cent->lerpAngles[1] = cent->lerpAngles[2] = 1;
		AnglesToAxis(cent->lerpAngles, ent.axis);

		AxisCopy(cg.autoAxisPadPowerups, ent.axis);

		ent.frame = 0; // so the mainmodel will not be animated
	} else if (item->giType == IT_HEALTH) {
		VectorCopy(cg.autoAnglesFast, cent->lerpAngles);
		AxisCopy(cg.autoAxisFast, ent.axis);
	} else {
		VectorCopy(cg.autoAngles, cent->lerpAngles);
		AxisCopy(cg.autoAxis, ent.axis);
	}

	wi = NULL;
	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
	if (item->giType == IT_WEAPON) {
		wi = &cg_weapons[item->giTag];
		cent->lerpOrigin[0] -= wi->weaponMidpoint[0] * ent.axis[0][0] + wi->weaponMidpoint[1] * ent.axis[1][0] +
							   wi->weaponMidpoint[2] * ent.axis[2][0];
		cent->lerpOrigin[1] -= wi->weaponMidpoint[0] * ent.axis[0][1] + wi->weaponMidpoint[1] * ent.axis[1][1] +
							   wi->weaponMidpoint[2] * ent.axis[2][1];
		cent->lerpOrigin[2] -= wi->weaponMidpoint[0] * ent.axis[0][2] + wi->weaponMidpoint[1] * ent.axis[1][2] +
							   wi->weaponMidpoint[2] * ent.axis[2][2];

		cent->lerpOrigin[2] += 8; // an extra height boost
	}

	if (es->modelindex == cgs.media.neutralCartridgeEntNum &&
		cent->currentState.otherEntityNum == cg.snap->ps.clientNum)
		ent.hModel = cg_items[cgs.media.redCartridgeEntNum].models[0];
	else
		ent.hModel = cg_items[es->modelindex].models[0];

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	msec = cg.time - cent->miscTime;
	if (msec >= 0 && msec < ITEM_SCALEUP_TIME) {
		frac = (float)msec / ITEM_SCALEUP_TIME;
		VectorScale(ent.axis[0], frac, ent.axis[0]);
		VectorScale(ent.axis[1], frac, ent.axis[1]);
		VectorScale(ent.axis[2], frac, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	} else {
		frac = 1.0;
	}

	// items without glow textures need to keep a minimum light value
	// so they are always visible
	if ((item->giType == IT_WEAPON) || (item->giType == IT_ARMOR)) {
		ent.renderfx |= RF_MINLIGHT;
	}

	// increase the size of the weapons when they are presented as items
	if (item->giType == IT_WEAPON) {
		VectorScale(ent.axis[0], 1.5, ent.axis[0]);
		VectorScale(ent.axis[1], 1.5, ent.axis[1]);
		VectorScale(ent.axis[2], 1.5, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// accompanying rings / spheres for powerups
	if (!cg_simpleItems.integer) {
		vec3_t spinAngles;

		VectorClear(spinAngles);

		if (item->giType == IT_HEALTH || item->giType == IT_POWERUP || item->giType == IT_HOLDABLE) {
			if ((ent.hModel = cg_items[es->modelindex].models[1]) != 0) {
				if (item->giType == IT_POWERUP || item->giType == IT_HOLDABLE) {
					ent.frame = (int)(cg.time * 0.022f) % 29; // wieviel frames? doch nicht 44?

					ent.origin[2] += 12; // brauch ich das noch?
					VectorCopy(cent->lerpAngles, spinAngles);
				}
				AnglesToAxis(spinAngles, ent.axis);

				// scale up if respawning
				if (frac != 1.0) {
					VectorScale(ent.axis[0], frac, ent.axis[0]);
					VectorScale(ent.axis[1], frac, ent.axis[1]);
					VectorScale(ent.axis[2], frac, ent.axis[2]);
					ent.nonNormalizedAxes = qtrue;
				}
				trap_R_AddRefEntityToScene(&ent);
			}
		}
	}
}

//============================================================================

/*
===============
CG_Missile
===============
*/
static void CG_Missile(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;
	const weaponInfo_t *weapon;
	//	int	col;

	s1 = &cent->currentState;
	if (s1->weapon >= WP_NUM_WEAPONS) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy(s1->angles, cent->lerpAngles);

	/*
		if ( cent->currentState.modelindex == TEAM_RED ) {
			col = 1;
		}
		else if ( cent->currentState.modelindex == TEAM_BLUE ) {
			col = 2;
		}
		else {
			col = 0;
		}

		// add dynamic light
		if ( weapon->missileDlight ) {
			trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight,
				weapon->missileDlightColor[col][0], weapon->missileDlightColor[col][1],
	   weapon->missileDlightColor[col][2] );
		}
	*/
	// add dynamic light
	if (weapon->missileDlight) {
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight, weapon->missileDlightColor[0],
							   weapon->missileDlightColor[1], weapon->missileDlightColor[2]);
	}

	// add missile sound
	if (weapon->missileSound) {
		vec3_t velocity;

		BG_EvaluateTrajectoryDelta(&cent->currentState.pos, cg.time, velocity);

		trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, velocity, weapon->missileSound);
	}

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	// add trails
	if (weapon->missileTrailFunc) {
		trailEnt = ent;
		weapon->missileTrailFunc(cent, weapon);
	}

	if (cent->currentState.weapon == WP_BETTY) {
		ent.customShader = cgs.media.fireBallShader;
		ent.reType = RT_SPRITE;

		ent.radius = 50;
		if (CG_FreezeTag()) {
			ent.radius = 25;
		}
		ent.rotation = 110;
		ent.shaderTime = 0;
		trap_R_AddRefEntityToScene(&ent);

		ent.radius = 55;
		if (CG_FreezeTag()) {
			ent.radius = 30;
		}
		ent.rotation = 0;
		ent.shaderTime = 1.2f;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	// render the bolt
	if (cent->currentState.weapon == WP_NIPPER || cent->currentState.weapon == WP_SPLASHER) {
		int i, n, time;

		if (cent->currentState.weapon == WP_NIPPER) {
			// nipper
			ent.customShader = cgs.media.nipperBallShader;
			ent.radius = 12;
			n = 5;
		} else {
			// splasher
			ent.customShader = cgs.media.waterBallShader;
			ent.radius = 30;
			n = 14;
		}
		time = cg.time;

		for (i = 0; i < n && time > s1->pos.trTime; i++) {
			// add particle
			BG_EvaluateTrajectory(&s1->pos, time, ent.origin);
			ent.reType = RT_SPRITE;

			ent.rotation = 110;
			ent.shaderTime = 0;
			trap_R_AddRefEntityToScene(&ent);

			// prepare next one
			time -= 6;
			ent.radius -= 2;
		}
		return;
	}

	if (cent->currentState.weapon == WP_BAMBAM_MISSILE) {
		ent.reType = RT_SPRITE;
		ent.radius = 8;
		ent.rotation = 0;
		if (cent->currentState.generic1 == TEAM_BLUE)
			ent.customShader = cgs.media.bambamMissileBlueShader;
		else
			ent.customShader = cgs.media.bambamMissileRedShader;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	if (!weapon->missileModel)
		return;

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// HERBY: Bubble G
	if (s1->weapon == WP_BUBBLEG) {
		if (CG_FreezeTag()) {
			VectorScale(g_color_table[ColorIndex(COLOR_WHITE)], 255, ent.shaderRGBA);
		} else {
			VectorScale(g_color_table[s1->generic1], 255, ent.shaderRGBA);
		}
		ent.shaderRGBA[3] = 255;
	}

	if (s1->weapon == WP_BOASTER) { // only for debugging with g_transmitSVboastermissiles 1
		VectorScale(g_color_table[6], 255, ent.shaderRGBA);
		ent.shaderRGBA[3] = 255;
	}

	// convert direction of travel into axis
	if (VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0) {
		ent.axis[0][2] = 1;
	}

	if (s1->weapon != WP_KILLERDUCKS) {
		// spin as it moves
		if (s1->pos.trType != TR_STATIONARY) {
			if (s1->weapon == WP_BALLOONY) {
				BG_EvaluateTrajectory(&s1->apos, cg.time, cent->lerpAngles);
				AnglesToAxis(cent->lerpAngles, ent.axis);
			} else
				RotateAroundDirection(ent.axis, cg.time / 4);
		} else {
			RotateAroundDirection(ent.axis, s1->time);
		}
	} else {
		vec3_t tmpv3;

		ent.axis[2][0] = ent.axis[2][1] = 0.0f;
		ent.axis[2][2] = 1.0f;
		ent.axis[0][2] = 0.0f;
		if (ent.axis[0][0] == 0.0f && ent.axis[0][1] == 0.0f)
			ent.axis[0][0] = 1.0f;
		else
			VectorNormalize(ent.axis[0]);

		CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);

		// turn the model ;)
		tmpv3[0] = ent.axis[0][0];
		tmpv3[1] = ent.axis[0][1];
		tmpv3[2] = ent.axis[0][2];

		ent.axis[0][0] = -ent.axis[1][0];
		ent.axis[0][1] = -ent.axis[1][1];
		ent.axis[0][2] = -ent.axis[1][2];

		ent.axis[1][0] = -tmpv3[0];
		ent.axis[1][1] = -tmpv3[1];
		ent.axis[1][2] = -tmpv3[2];

		ent.origin[2] = (ent.oldorigin[2] -= 10);

		ent.oldframe = ent.frame;
		ent.frame = (int)(cg.time * 0.022f) % 22;

		if (cg_shadows.integer == 1) {
			vec3_t end, mins = {-10, -10, 0}, maxs = {10, 10, 2};
			trace_t trace;
			float alpha;

			// send a trace down from the player to the ground
			VectorCopy(cent->lerpOrigin, end);
			end[2] -= 128;

			trap_CM_BoxTrace(&trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID);

			// no shadow if too high
			if (trace.fraction != 1.0f || !trace.startsolid || !trace.allsolid) {
				// fade the shadow out with height
				alpha = 1.0f - trace.fraction;

				// add the mark as a temporary, so it goes directly to the renderer
				// without taking a spot in the cg_marks array
				CG_ImpactMark(cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, cent->pe.legs.yawAngle,
							  alpha, alpha, alpha, 1, qfalse, 10, qtrue);
			}
		}
	}

	// add ballony-squish-effect
	if (s1->weapon == WP_BALLOONY && cg.time < s1->pos.trTime) {
		vec3_t normal, delta;
		float fraction, dist;
		int deltaTime, i;

		// generate "parabolic" squish
		VectorCopy(cent->beamEnd, normal);
		dist = fabs(DotProduct(normal, s1->pos.trDelta)) / 500.0f;
		if (dist > 1.0f)
			dist = 1.0f;

		deltaTime = s1->pos.trTime - cent->miscTime;
		fraction = (2.0f * (cg.time - cent->miscTime) - deltaTime) / deltaTime;
		fraction *= fraction * (0.1f + 0.6f * dist);
		fraction += 1.0f - (0.1f + 0.6f * dist);

		for (i = 0; i < 3; i++) {
			dist = DotProduct(normal, ent.axis[i]);
			VectorMA(ent.axis[i], -dist, normal, delta);
			VectorMA(ent.axis[i], -dist * (1.0f - fraction), normal, ent.axis[i]);
			VectorMA(ent.axis[i], 1.0f / fraction - 1, delta, ent.axis[i]);
		}

		VectorMA(ent.origin, -BALLOONY_SIZE * (1.0f - fraction), normal, ent.oldorigin);
		VectorCopy(ent.oldorigin, ent.origin);
	}

	// add to refresh list, possibly with quad glow
	CG_AddRefEntityWithPowerups(&ent, s1, TEAM_FREE);
}

/*
===============
CG_Grapple

This is called when the grapple is sitting up against the wall
===============
*/
static void CG_Grapple(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;
	const weaponInfo_t *weapon;

	s1 = &cent->currentState;
	if (s1->weapon >= WP_NUM_WEAPONS) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy(s1->angles, cent->lerpAngles);

#if 0 // FIXME add grapple pull sound here..?
	// add missile sound
	if ( weapon->missileSound ) {
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->missileSound );
	}
#endif

	// Will draw cable if needed
	CG_GrappleTrail(cent, weapon);

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if (VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0) {
		ent.axis[0][2] = 1;
	}

	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_Mover
===============
*/
static void CG_Mover(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis(cent->lerpAngles, ent.axis);

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = (cg.time >> 6) & 1;

	// get the model, either as a bmodel or a modelindex
	if (s1->solid == SOLID_BMODEL) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if (s1->modelindex2) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		trap_R_AddRefEntityToScene(&ent);
	}
}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(s1->pos.trBase, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);
	AxisClear(ent.axis);
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_Portal
===============
*/
static void CG_Portal(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);
	ByteToDir(s1->eventParm, ent.axis[0]);
	PerpendicularVector(ent.axis[1], ent.axis[0]);

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract(vec3_origin, ent.axis[1], ent.axis[1]);

	CrossProduct(ent.axis[0], ent.axis[1], ent.axis[2]);
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;					   // rotation speed
	ent.skinNum = s1->clientNum / 256.0 * 360; // roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
================
CG_CreateRotationMatrix
================
*/
static void CG_CreateRotationMatrix(const vec3_t angles, vec3_t matrix[3]) {
	AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}

/*
================
CG_TransposeMatrix
================
*/
static void CG_TransposeMatrix(vec3_t matrix[3], vec3_t transpose[3]) {
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

/*
================
CG_RotatePoint
================
*/
static void CG_RotatePoint(vec3_t point, vec3_t matrix[3]) {
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t angles_in, vec3_t angles_out) {
	centity_t *cent;
	vec3_t oldOrigin, origin, deltaOrigin;
	vec3_t oldAngles, angles, deltaAngles;
	vec3_t matrix[3], transpose[3];
	vec3_t org, org2, move2;

	if (moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL) {
		VectorCopy(in, out);
		VectorCopy(angles_in, angles_out);
		return;
	}

	cent = &cg_entities[moverNum];
	if (cent->currentState.eType != ET_MOVER) {
		VectorCopy(in, out);
		VectorCopy(angles_in, angles_out);
		return;
	}

	BG_EvaluateTrajectory(&cent->currentState.pos, fromTime, oldOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles);

	BG_EvaluateTrajectory(&cent->currentState.pos, toTime, origin);
	BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles);

	VectorSubtract(origin, oldOrigin, deltaOrigin);
	VectorSubtract(angles, oldAngles, deltaAngles);

	// origin change when on a rotating object
	CG_CreateRotationMatrix(deltaAngles, transpose);
	CG_TransposeMatrix(transpose, matrix);
	VectorSubtract(in, oldOrigin, org);
	VectorCopy(org, org2);
	CG_RotatePoint(org2, matrix);
	VectorSubtract(org2, org, move2);
	VectorAdd(deltaOrigin, move2, deltaOrigin);

	VectorAdd(in, deltaOrigin, out);
	VectorAdd(angles_in, deltaAngles, angles_out);
}

/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition(centity_t *cent) {
	vec3_t current, next;
	float f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if (cg.nextSnap == NULL) {
		CG_Error("CG_InterpoateEntityPosition: cg.nextSnap == NULL");
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, current);
	BG_EvaluateTrajectory(&cent->nextState.pos, cg.nextSnap->serverTime, next);

	cent->lerpOrigin[0] = current[0] + f * (next[0] - current[0]);
	cent->lerpOrigin[1] = current[1] + f * (next[1] - current[1]);
	cent->lerpOrigin[2] = current[2] + f * (next[2] - current[2]);

	BG_EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, current);
	BG_EvaluateTrajectory(&cent->nextState.apos, cg.nextSnap->serverTime, next);

	cent->lerpAngles[0] = LerpAngle(current[0], next[0], f);
	cent->lerpAngles[1] = LerpAngle(current[1], next[1], f);
	cent->lerpAngles[2] = LerpAngle(current[2], next[2], f);
}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions(centity_t *cent) {

	// if this player does not want to see extrapolated players
	if (!cg_smoothClients.integer) {
		// make sure the clients use TR_INTERPOLATE
		if (cent->currentState.number < MAX_CLIENTS) {
			cent->currentState.pos.trType = TR_INTERPOLATE;
			cent->nextState.pos.trType = TR_INTERPOLATE;
		}
	}

	if (cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE) {
		CG_InterpolateEntityPosition(cent);
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if (cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
		cent->currentState.number < MAX_CLIENTS) {
		CG_InterpolateEntityPosition(cent);
		return;
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if (cent != &cg.predictedPlayerEntity) {
		CG_AdjustPositionForMover(cent->lerpOrigin, cent->currentState.groundEntityNum, cg.snap->serverTime, cg.time,
								  cent->lerpOrigin, cent->lerpAngles, cent->lerpAngles);
	}
}

/*
===============
CG_TeamBase
===============
*/
static void CG_TeamBase(centity_t *cent) {
	refEntity_t model;

	if (cgs.gametype == GT_CTF) {
		// show the flag base
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy(cent->lerpOrigin, model.lightingOrigin);
		VectorCopy(cent->lerpOrigin, model.origin);
		AnglesToAxis(cent->currentState.angles, model.axis);
		if (cent->currentState.modelindex == TEAM_RED) {
			model.hModel = cgs.media.redFlagBaseModel;
		} else if (cent->currentState.modelindex == TEAM_BLUE) {
			model.hModel = cgs.media.blueFlagBaseModel;
		} else {
			model.hModel = cgs.media.neutralFlagBaseModel;
		}
		trap_R_AddRefEntityToScene(&model);
	}
}

/*
==================
CG_Balloon
==================
*/
static void CG_Balloon(centity_t *cent) {
	refEntity_t ent;
	entityState_t *s1;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex) {
		return;
	}

	memset(&ent, 0, sizeof(ent));

	// animate
	if (s1->frame) {
		if (!s1->time)
			s1->time = 4000;
		if (s1->frame != cent->muzzleFlashTime) {
			if (cent->lastweaponframe == 0)
				trap_S_StartSound(s1->origin, s1->number, CHAN_AUTO, cgs.media.ballonAufblasSound);
			cent->lastweaponframe = cent->muzzleFlashTime;
			cent->muzzleFlashTime = s1->frame;
			cent->miscTime = cg.time;
		}

		ent.frame = s1->frame;
		ent.oldframe = cent->lastweaponframe;
		ent.backlerp = 1.0f - 10.0f * (cg.time - cent->miscTime) / s1->time;

		// sanity check
		if (ent.backlerp > 1.0f)
			ent.backlerp = 1.0f;
		if (ent.backlerp < 0.0f)
			ent.backlerp = 0.0f;
	} else {
		// don't interpolate around frame 0
		if (cent->muzzleFlashTime)
			trap_S_StartSound(s1->origin, s1->number, CHAN_AUTO, cgs.media.ballonPlatztSound);
		cent->muzzleFlashTime = 0;
	}

	// colorize
	if (s1->generic1 == 1)
		VectorSet(ent.shaderRGBA, 128, 0, 0);
	else if (s1->generic1 == 2)
		VectorSet(ent.shaderRGBA, 0, 0, 128);
	else
		VectorSet(ent.shaderRGBA, 128, 128, 128);
	ent.shaderRGBA[3] = 255;

	// NOTE: With vertex light the balloons are grey instead of team color
	if (CG_GetCvarInt("r_vertexlight")) {
		VectorSet(ent.shaderRGBA, 255, 255, 255);

		switch (s1->generic1) {
		case TEAM_RED:
			ent.customSkin = trap_R_RegisterSkin("models/special/balloon_vertex_red.skin");
			break;
		case TEAM_BLUE:
			ent.customSkin = trap_R_RegisterSkin("models/special/balloon_vertex_blue.skin");
			break;
		default:
			ent.customSkin = 0;
			break;
		}
	}

	// place model
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// convert angles to axis
	//	AnglesToAxis( cent->lerpAngles, ent.axis );
	AnglesToAxis(cent->currentState.angles, ent.axis);

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

static void CG_Station(centity_t *cent) {
	refEntity_t ent;
	//	vec3_t			temporig;
	static int lastEmptyWarning = 0;

	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);
	//	AxisClear(ent.axis);
	AnglesToAxis(cent->currentState.angles, ent.axis);
	ent.reType = RT_MODEL;
	ent.hModel = cgs.media.HealthStation_Base;

	trap_R_AddRefEntityToScene(&ent);

	if (cent->currentState.angles2[1] == 0.0f) {
		vec3_t tmpangles;

		AnglesToAxis(cg.autoAngles, ent.axis);
		ent.origin[2] += 48.0f + sin(cg.time * 0.005f) * 4.0f; // 24(oldmodel)->48(new)
		ent.oldorigin[2] += 48.0f + sin(cg.time * 0.005f) * 4.0f;
		ent.hModel = cgs.media.HealthStation_Ring;
		trap_R_AddRefEntityToScene(&ent);

		tmpangles[0] = 0;
		tmpangles[1] = 360 - cg.autoAngles[1];
		tmpangles[2] = 0;
		AnglesToAxis(tmpangles, ent.axis);

		ent.hModel = cgs.media.HealthStation_Cross;

		ent.axis[0][0] *= cent->currentState.angles2[2];
		ent.axis[0][1] *= cent->currentState.angles2[2];
		ent.axis[0][2] *= cent->currentState.angles2[2];
		ent.axis[1][0] *= cent->currentState.angles2[2];
		ent.axis[1][1] *= cent->currentState.angles2[2];
		ent.axis[1][2] *= cent->currentState.angles2[2];
		ent.axis[2][0] *= cent->currentState.angles2[2];
		ent.axis[2][1] *= cent->currentState.angles2[2];
		ent.axis[2][2] *= cent->currentState.angles2[2];
		trap_R_AddRefEntityToScene(&ent);

		trap_S_StopLoopingSound(cent->currentState.number);
	} else { // list of re-used variables
		// angles2[1] -> timer for slowing down the loading ring movement (1.0=ein player steht in der station)
		// beamEnd[0] -> angles2[1] of the last frame
		if (cent->currentState.angles2[1] < 1.0f) {
			if (cent->beamEnd[0] < cent->currentState.angles2[1] && !cent->trailTime) {
				trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_ITEM, cgs.media.station_start);
				cent->trailTime = 1;
			} else if (cent->beamEnd[0] > cent->currentState.angles2[1]) {
				/*
								if(!(cg.clientFrame%2)) LaunchStationStar(ent.origin);
				*/

				if (cent->trailTime) {
					trap_S_StopLoopingSound(cent->currentState.number); // loop
					trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_ITEM, cgs.media.station_end);
					cent->trailTime = 0;
				}
			}
		} else if (cent->trailTime)
			trap_S_AddRealLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin,
									   cgs.media.station_loop);

		if (cent->currentState.angles2[2] == 0.0f && lastEmptyWarning + 2000 < cg.time) {
			trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_ITEM, cgs.media.station_empty);
			lastEmptyWarning = cg.time;
		}

		cent->beamEnd[0] = cent->currentState.angles2[1];

		AnglesToAxis(cg.autoAngles, ent.axis);
		ent.axis[0][0] *= 1.3f;
		ent.axis[0][1] *= 1.3f;
		ent.axis[0][2] *= 1.3f;
		ent.axis[1][0] *= 1.3f;
		ent.axis[1][1] *= 1.3f;
		ent.axis[1][2] *= 1.3f;
		ent.axis[2][0] *= 0.1f;
		ent.axis[2][1] *= 0.1f;
		ent.axis[2][2] *= 0.1f;
		ent.customShader = cgs.media.StationRingShader;
		ent.hModel = cgs.media.StationLoadingRings;
		ent.oldorigin[2] = ent.origin[2] += (32.0f + sin(cg.time * 0.002f) * 24.0f) * cent->currentState.angles2[1];

		trap_R_AddRefEntityToScene(&ent);

		ent.oldorigin[2] = ent.origin[2] =
			cent->lerpOrigin[2] + (32.0f + sin(cg.time * 0.002f + 1.57f) * 24.0f) * cent->currentState.angles2[1];
		trap_R_AddRefEntityToScene(&ent);

		ent.oldorigin[2] = ent.origin[2] =
			cent->lerpOrigin[2] + (32.0f + sin(cg.time * 0.002f + 3.14f) * 24.0f) * cent->currentState.angles2[1];
		trap_R_AddRefEntityToScene(&ent);

		ent.oldorigin[2] = ent.origin[2] =
			cent->lerpOrigin[2] + (32.0f + sin(cg.time * 0.002f + 4.71f) * 24.0f) * cent->currentState.angles2[1];
		trap_R_AddRefEntityToScene(&ent);
	}
}

void CG_BamBam_Explosion(vec3_t origin) {
	vec3_t sprVel;
	vec3_t dir;
	localEntity_t *le;

	VectorSet(dir, 0, 0, 1);
	le = CG_MakeExplosion(origin, dir, cgs.media.dishFlashModel, cgs.media.duckExplosionShader, 800, qtrue);
	le->light = 300;
	VectorSet(le->lightColor, 1.0f, 0.7f, 0.0f);

	VectorSet(sprVel, 0, 0, 300);
	CG_GenerateParticles(cgs.media.bambamExplosionLeg, 0, origin, 8, sprVel, 45, 150, 3, 0, cg.time, 3000, 500, 0, 0, 0,
						 0, LEF_GRAVITY | LEF_TUMBLE | LEF_COLLISIONS, 0);
	CG_GenerateParticles(cgs.media.bambamExplosionTorso, 0, origin, 8, sprVel, 45, 150, 1, 0, cg.time, 3000, 500, 0, 0,
						 0, 0, LEF_GRAVITY | LEF_TUMBLE | LEF_COLLISIONS, 0);

	trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.bambamExplosionSound);
}

void CG_Boomies_Explosion(vec3_t origin) {
	localEntity_t *le;
	le = CG_AllocLocalEntity();
	le->leType = LE_BOOMIESEXPLOSION;
	le->startTime = cg.time;
	le->endTime = cg.time + 1000;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
	VectorCopy(origin, le->refEntity.origin);
	VectorSet(le->angles.trBase, 360 * crandom(), 360 * crandom(), 360 * crandom());

	trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.boomiesExplosionSound);
}

static void CG_DrawBamBam(centity_t *cent) {
	refEntity_t ent;

	memset(&ent, 0, sizeof(ent));

	VectorCopy(cent->lerpOrigin, ent.origin);
	AnglesToAxis(cent->currentState.angles, ent.axis);

	ent.reType = RT_MODEL;
	ent.hModel = cgs.gameModels[cent->currentState.modelindex];

	ent.oldframe = cent->lastweaponframe;
	switch (cent->currentState.generic1) {
	case BBS_INACTIVE:
		ent.frame = 0;
		break;
	case BBS_BUILDING: // playing anim 1-50
		ent.frame = ((cg.time - cent->currentState.time) * 25 / 1000) + 1;
		if (ent.frame > 50)
			ent.frame = 50;
		break;
	case BBS_IDLE2SHOOTING:
	case BBS_IDLE: // playing anim 51-80 looped
		ent.frame = (((cg.time - cent->currentState.time) * 25 / 1000) % (80 - 51 + 1)) + 51;
		break;
	case BBS_SHOOTING2IDLE:
	case BBS_SHOOTING: // playing anim 81-105 looped
		ent.frame = (((cg.time - cent->currentState.time) * 25 / 1000) % (105 - 81 + 1)) + 81;
		break;
	case BBS_ZZZ: // playing anim 106-200
		ent.frame = ((cg.time - cent->currentState.time) * 25 / 1000) + 106;
		if (ent.frame > 200)
			ent.frame = 200;
		break;
	}
	cent->lastweaponframe = ent.frame;

	trap_R_AddRefEntityToScene(&ent);
}

static void CG_DrawBoomies(centity_t *cent) {
	refEntity_t ent;

	memset(&ent, 0, sizeof(ent));

	VectorCopy(cent->lerpOrigin, ent.origin);
	//	AxisClear(ent.axis);
	AnglesToAxis(cent->currentState.angles, ent.axis);

	ent.reType = RT_MODEL;
	ent.hModel = cgs.gameModels[cent->currentState.modelindex];

	//	ent.frame = ent.oldframe = (((5*cg.time)/1000)&0x1f);
	ent.frame = ((cg.time - cent->currentState.time) * 25 / 1000);
	if (ent.frame > 15)
		ent.frame = 15;
	ent.oldframe = ent.frame;

	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity(centity_t *cent) {
	// event-only entities will have been dealt with already
	if (cent->currentState.eType >= ET_EVENTS) {
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions(cent);

	// add automatic effects only if ambient is enabled
	if (!((cg_ambient.integer == 0) && (cent->currentState.eType == ET_SPEAKER))) {
		CG_EntityEffects(cent);
	}

	switch (cent->currentState.eType) {
	default:
		CG_Error("Bad entity type: %i", cent->currentState.eType);
		break;
	case ET_EXPLOSION:
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		CG_General(cent);
		break;
	case ET_PLAYER:
		cent->currentState.eFlags |= (cent->currentState.time2 << 16);
		CG_Player(cent);
		break;
	case ET_ITEM:
		CG_Item(cent);
		break;
	case ET_MISSILE:
		CG_Missile(cent);
		break;
	case ET_MOVER:
		CG_Mover(cent);
		break;
	case ET_BEAM:
		CG_Beam(cent);
		break;
	case ET_PORTAL:
		CG_Portal(cent);
		break;
	case ET_SPEAKER:
		CG_Speaker(cent);
		break;
	case ET_GRAPPLE:
		CG_Grapple(cent);
		break;
	case ET_TEAM:
		CG_TeamBase(cent);
		break;
	case ET_BALLOON:
		CG_Balloon(cent);
		break;
	case ET_STATION:
		CG_Station(cent);
		break;
	case ET_BAMBAM:
		CG_DrawBamBam(cent);
		break;
	case ET_BOOMIES:
		CG_DrawBoomies(cent);
	}

	CG_AddBoundingBoxEntity(cent);
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities(void) {
	int num;
	centity_t *cent;
	playerState_t *ps;

	// set cg.frameInterpolation
	if (cg.nextSnap) {
		int delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if (delta == 0) {
			cg.frameInterpolation = 0;
		} else {
			cg.frameInterpolation = (float)(cg.time - cg.snap->serverTime) / delta;
		}
	} else {
		cg.frameInterpolation = 0; // actually, it should never be used, because
								   // no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = (cg.time & 2047) * 360 / 2048.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = (cg.time & 1023) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	cg.autoAnglesPadPowerups[0] = 0;
	cg.autoAnglesPadPowerups[1] =
		(cg.time % 1317) * 360 / 1318.0f; // 1318 ms -> um drehungs dauer bei animation (29f@22fps)
	cg.autoAnglesPadPowerups[2] = 0;

	AnglesToAxis(cg.autoAnglesPadPowerups, cg.autoAxisPadPowerups);

	AnglesToAxis(cg.autoAngles, cg.autoAxis);
	AnglesToAxis(cg.autoAnglesFast, cg.autoAxisFast);

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState(ps, &cg.predictedPlayerEntity.currentState, qfalse);
	CG_AddCEntity(&cg.predictedPlayerEntity);

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions(&cg_entities[cg.snap->ps.clientNum]);

	// add each entity sent over by the server
	for (num = 0; num < cg.snap->numEntities; num++) {
		cent = &cg_entities[cg.snap->entities[num].number];
		CG_AddCEntity(cent);
	}
}
