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
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"

static qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle);

#define MAXRADIUS 48 // 32
#define PUFFSPEED 1000.0f
/*
==========================
CG_SprayTrail
==========================
*/
void CG_SprayTrail(centity_t *cent, vec3_t start) {
	vec4_t tmpcolor = {1.0f, 1.0f, 1.0f, 1.0f};
	//	vec3_t	smokepos,scalevec;
	vec3_t smokevel; // = { -5.0f, 0.0f, 0.0f };
					 //	int		i;
	localEntity_t *smoke;
	vec3_t end, forward;
	trace_t tr;
	float traillength;

	AngleVectors(cent->lerpAngles, forward, NULL, NULL);
	end[0] = start[0] + forward[0] * 256;
	end[1] = start[1] + forward[1] * 256;
	end[2] = start[2] + forward[2] * 256;

	CG_Trace(&tr, start, 0, 0, end, cent->currentState.clientNum, MASK_SHOT);
	if (tr.fraction != 1.0f) {
		end[0] = tr.endpos[0];
		end[1] = tr.endpos[1];
		end[2] = tr.endpos[2];
	}

	if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_RED) {
		tmpcolor[0] = 1.0f;
		tmpcolor[1] = tmpcolor[2] = 0.0f;
	} else if (cgs.clientinfo[cent->currentState.clientNum].team == TEAM_BLUE) {
		tmpcolor[0] = tmpcolor[1] = 0.0f;
		tmpcolor[2] = 1.0f;
	} else {
		tmpcolor[0] = cgs.clientinfo[cent->currentState.clientNum].color2[0];
		tmpcolor[1] = cgs.clientinfo[cent->currentState.clientNum].color2[1];
		tmpcolor[2] = cgs.clientinfo[cent->currentState.clientNum].color2[2];
	}

	smokevel[0] = end[0] - start[0];
	smokevel[1] = end[1] - start[1];
	smokevel[2] = end[2] - start[2];

	traillength = VectorNormalize(smokevel);

	smokevel[0] *= PUFFSPEED;
	smokevel[1] *= PUFFSPEED;
	smokevel[2] *= PUFFSPEED;

	smoke = CG_SmokePuff(start, smokevel, MAXRADIUS, tmpcolor[0], tmpcolor[1], tmpcolor[2], 0.33f,
						 1000.0f * traillength / PUFFSPEED, // disappear at wall
						 cg.time, 0, 0, cgs.media.spraypuff);
	// use the optimized local entity add
	smoke->leType = LE_SPRAYTRAIL;							 // LE_MOVE_SCALE_FADE
	smoke->angles.trDuration = 1000.0f * 256.0f / PUFFSPEED; // missbrauch !! :P
}

/*
==========================
CG_RailTrail
==========================
*/
void CG_RailTrail(clientInfo_t *ci, vec3_t start, vec3_t end) {
	localEntity_t *le;
	vec3_t delta, right;
	int i;

	// calculate correct positions for double rails
	VectorSubtract(end, start, delta);
	right[0] = delta[1];
	right[1] = -delta[0];
	right[2] = 0;
	VectorNormalize(right);

	VectorMA(start, 6, right, start);
	start[2] -= 14;

	for (i = -1; i < 2; i += 2) {
		// alloc new local entity
		le = CG_AllocLocalEntity();
		le->leType = LE_PUMPERTRAIL;
		le->startTime = cg.time;
		le->endTime = cg.time + 500;
		le->lifeRate = i * M_PI / 4;

		VectorMA(start, 6 * i, right, le->pos.trBase);
		VectorMA(end, 6 * i, right, le->pos.trDelta);
	}
}

/*
==========================
CG_RocketTrail
==========================
*/
static void CG_RocketTrail(centity_t *ent, const weaponInfo_t *wi) {
	entityState_t *es;

	// get entitystate
	es = &ent->currentState;

	// alloc a new LE
	if (!ent->trailLE) {
		ent->trailLE = CG_AllocLocalEntity();
		ent->trailLE->leType = LE_TRAIL;
		ent->trailLE->pos = es->pos;
		ent->trailLE->refEntity.customShader = cgs.media.fireTrailShader;
		ent->trailLE->refEntity.radius = 30;
		VectorCopy(es->origin2, ent->trailLE->refEntity.origin);
	}

	// set the "still alive time"
	ent->trailLE->fadeInTime = cg.time;
	ent->trailLE->endTime = cg.time + TRAIL_FADETIME;

	// the rendering happens in cg_localents.c
}

/*
==========================
CG_SplasherTrail
==========================
*/
static void CG_SplasherTrail(centity_t *ent, const weaponInfo_t *wi) {
	entityState_t *es;

	// get entitystate
	es = &ent->currentState;

	// alloc a new LE
	if (!ent->trailLE) {
		ent->trailLE = CG_AllocLocalEntity();
		ent->trailLE->leType = LE_TRAIL;
		ent->trailLE->pos = es->pos;
		ent->trailLE->refEntity.customShader = cgs.media.waterTrailShader;
		ent->trailLE->refEntity.radius = 10;
		VectorCopy(es->origin2, ent->trailLE->refEntity.origin);
	}

	// set the "still alive time"
	ent->trailLE->fadeInTime = cg.time;
	ent->trailLE->endTime = cg.time + TRAIL_FADETIME;

	// the rendering happens in cg_localents.c
}

/*
==========================
CG_KMATrail
==========================
*/
static void CG_KMATrail(centity_t *ent, const weaponInfo_t *wi) {
	clientInfo_t *cinfo;
	entityState_t *es;
	// byte				colorRGBA;

	es = &ent->currentState;
	cinfo = &cgs.clientinfo[es->clientNum];

	// alloc a new LE
	if (!ent->trailLE) {
		ent->trailLE = CG_AllocLocalEntity();
		ent->trailLE->leType = LE_TRAIL_TRACED_RGB;
		ent->trailLE->pos = es->pos;
		ent->trailLE->refEntity.customShader = cgs.media.kmaTrailShader;

		if (cgs.gametype >= GT_TEAM) // team mode
		{
			// force team colors
			if (cinfo->team == TEAM_BLUE) {
				ent->trailLE->color[0] = 0.0;
				ent->trailLE->color[1] = 0.0;
				ent->trailLE->color[2] = 1.0;
			} else {
				ent->trailLE->color[0] = 1.0;
				ent->trailLE->color[1] = 0.0;
				ent->trailLE->color[2] = 0.0;
			}
		} else // free mode
		{
			// use custom colors
			ent->trailLE->color[0] = cinfo->color2[0];
			ent->trailLE->color[1] = cinfo->color2[1];
			ent->trailLE->color[2] = cinfo->color2[2];

			// force a different color for "black" since black doesn't show up
			if (ent->trailLE->color[0] == 0.0f && ent->trailLE->color[1] == 0.0f && ent->trailLE->color[2] == 0.0f) {
				ent->trailLE->color[0] = 0.8f;
				ent->trailLE->color[1] = 0.0f;
				ent->trailLE->color[2] = 1.0f;
			}
		}
		ent->trailLE->color[3] = 1.0f;
		ent->trailLE->refEntity.radius = wi->trailRadius;
		VectorCopy(es->origin2, ent->trailLE->refEntity.origin);
	}

	// set the "still alive time"
	ent->trailLE->fadeInTime = cg.time;
	ent->trailLE->endTime = cg.time + TRAIL_FADETIME;
}

/*
==========================
CG_ImperiusTrail
==========================
*/
static void CG_ImperiusTrail(centity_t *ent, const weaponInfo_t *wi) {
	localEntity_t *ring;
	refEntity_t re;
	vec3_t origin, altPos;
	int t, startTime, step;

	step = 48;

	startTime = ent->trailTime;
	t = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&ent->currentState.pos, cg.time, origin);
	ent->trailTime = cg.time;

	memset(&re, 0, sizeof(refEntity_t));
	VectorCopy(origin, re.origin);
	re.reType = RT_SPRITE;
	re.customShader = cgs.media.imperiusCoreShader;
	re.radius = 40;
	re.shaderRGBA[0] = re.shaderRGBA[1] = re.shaderRGBA[2] = 255;
	trap_R_AddRefEntityToScene(&re);

	for (; t <= cg.time; t += step) {
		BG_EvaluateTrajectory(&ent->currentState.pos, t, altPos);

		ring = CG_AllocLocalEntity();

		ring->leType = LE_IMPERIUSRINGS;
		ring->refEntity.hModel = cgs.media.dishFlashModel;
		ring->refEntity.customShader = cgs.media.imperiusRingShader;
		ring->refEntity.shaderTime = cg.time / 1000.0f;
		ring->startTime = cg.time - 20;
		ring->endTime = cg.time + wi->wiTrailTime - 20;
		ring->lifeRate = 1.0 / (ring->endTime - ring->startTime);
		ring->color[0] = ring->color[1] = ring->color[2] = ring->color[3] = 1.0;

		VectorCopy(altPos, ring->refEntity.origin);

		// convert direction of travel into axis
		if (VectorNormalize2(ent->currentState.pos.trDelta, ring->refEntity.axis[0]) == 0) {
			ring->refEntity.axis[0][2] = 1;
		}
		RotateAroundDirection(ring->refEntity.axis, cg.time);
	}
}

/*
==========================
CG_GrappleTrail
==========================
*/
void CG_GrappleTrail(centity_t *ent, const weaponInfo_t *wi) {
	vec3_t origin;
	entityState_t *es;
	vec3_t forward, up;
	refEntity_t beam;

	es = &ent->currentState;

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	ent->trailTime = cg.time;

	memset(&beam, 0, sizeof(beam));
	// FIXME adjust for muzzle position
	VectorCopy(cg_entities[ent->currentState.otherEntityNum].lerpOrigin, beam.origin);
	beam.origin[2] += 26;
	AngleVectors(cg_entities[ent->currentState.otherEntityNum].lerpAngles, forward, NULL, up);
	VectorMA(beam.origin, -6, up, beam.origin);
	VectorCopy(origin, beam.oldorigin);

	if (Distance(beam.origin, beam.oldorigin) < 64)
		return; // Don't draw if close

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;

	AxisClear(beam.axis);
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene(&beam);
}

/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon(int weaponNum) {
	weaponInfo_t *weaponInfo;
	gitem_t *item, *ammo;
	char path[MAX_QPATH];
	vec3_t mins, maxs;
	int i;

	weaponInfo = &cg_weapons[weaponNum];

	if (weaponNum == 0) {
		return;
	}

	if (weaponInfo->registered) {
		return;
	}

	memset(weaponInfo, 0, sizeof(*weaponInfo));
	weaponInfo->registered = qtrue;

	for (item = bg_itemlist + 1; item->classname; item++) {
		if (item->giType == IT_WEAPON && item->giTag == weaponNum) {
			weaponInfo->item = item;
			break;
		}
	}
	if (!item->classname) {
		CG_Error("Couldn't find weapon %i", weaponNum);
	}
	CG_RegisterItemVisuals(item - bg_itemlist);

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel(item->world_model[0]);

	// calc midpoint for rotation
	trap_R_ModelBounds(weaponInfo->weaponModel, mins, maxs);
	for (i = 0; i < 3; i++) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * (maxs[i] - mins[i]);
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader(item->icon);
	weaponInfo->ammoIcon = trap_R_RegisterShader(item->icon);

	for (ammo = bg_itemlist + 1; ammo->classname; ammo++) {
		if (ammo->giType == IT_AMMO && ammo->giTag == weaponNum) {
			break;
		}
	}
	if (ammo->classname && ammo->world_model[0]) {
		weaponInfo->ammoModel = trap_R_RegisterModel(ammo->world_model[0]);
	}

	if (weaponNum == WP_KILLERDUCKS) {
		weaponInfo->flashModel = 0;
		weaponInfo->barrelModel = 0;
		weaponInfo->handsModel = 0;

		goto NOADDITIONALMODELS;
	}

	if (weaponNum != WP_PUNCHY) {
		strcpy(path, item->world_model[0]);
		COM_StripExtension(path, path, sizeof(path));
		strcat(path, "_flash");
		weaponInfo->flashModel = trap_R_RegisterModel(path);
	}

	if (weaponNum == WP_NIPPER || weaponNum == WP_IMPERIUS) // || weaponNum == WP_PUNCHY
	{
		strcpy(path, item->world_model[0]);
		COM_StripExtension(path, path, sizeof(path));
		strcat(path, "_barrel");
		weaponInfo->barrelModel = trap_R_RegisterModel(path);
	} else
		weaponInfo->barrelModel = 0;

	strcpy(path, item->world_model[0]);
	COM_StripExtension(path, path, sizeof(path));
	strcat(path, "_hand");
	weaponInfo->handsModel = trap_R_RegisterModel(path);

	if (!weaponInfo->handsModel) {
		weaponInfo->handsModel = trap_R_RegisterModel("models/weapons2/nipper/nipper_hand");
	}
NOADDITIONALMODELS:

	switch (weaponNum) {
	case WP_PUNCHY:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->firingSound = trap_S_RegisterSound("sounds/weapons/punchy/idle", qfalse);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/punchy/attack", qfalse);
		weaponInfo->flashSound[3] = trap_S_RegisterSound("sounds/weapons/punchy/berserker_attack", qfalse);
		cgs.media.berserkerPunchyLoop = trap_S_RegisterSound("sounds/weapons/punchy/berserker_idle", qfalse);
		break;

	case WP_BOASTER:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->missileModel = trap_R_RegisterModel("models/weaponsfx/gum"); // <-- testing server stuff
		weaponInfo->readySound = trap_S_RegisterSound("sounds/weapons/boaster/ready", qfalse);
		weaponInfo->firingSound = trap_S_RegisterSound("sounds/weapons/boaster/firing", qfalse);

		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/boaster/flash", qfalse);

		cgs.media.boasterExplosionShader = trap_R_RegisterShader("boasterExplosion");
		cgs.media.waterBeamShader = trap_R_RegisterShader("waterBeam");

		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/rocket/rocket");
		weaponInfo->missileTrailFunc = CG_GrappleTrail;
		weaponInfo->missileDlight = 200;
		MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0);
		weaponInfo->firingSound = trap_S_RegisterSound("sounds/weapons/punchy/idle", qfalse);
		break;

	case WP_NIPPER:
		// HERBY: Nippers defs
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/nipper/flash", qfalse);
		cgs.media.nipperBallShader = trap_R_RegisterShader("nipperBall");
		cgs.media.nipperWaveShader = trap_R_RegisterShader("nipperWave");
		break;

	case WP_PUMPER:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/pumper/flash", qfalse);
		cgs.media.pumperFlashModel = trap_R_RegisterModel("models/weaponsfx/flash");
		cgs.media.pumperTrailShader = trap_R_RegisterShader("pumperTrail");
		break;

	case WP_BETTY:
		weaponInfo->missileSound = trap_S_RegisterSound("sounds/weapons/betty/fly", qfalse);
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;

		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/betty/flash", qfalse);
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("fireExplosion");
		cgs.media.fireBallShader = trap_R_RegisterShader("fireBall");
		cgs.media.fireTrailShader = trap_R_RegisterShader("fireTrail");
		cgs.media.fireDropModel = trap_R_RegisterModel("models/weaponsfx/firedrop");
		break;

	case WP_BALLOONY:
		// HERBY: Balloony defs
		weaponInfo->missileModel = trap_R_RegisterModel("models/weaponsfx/balloon");
		//		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 12;
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/balloony/flash", qfalse);
		cgs.media.waterExplosionShader = trap_R_RegisterShader("waterSplash");
		cgs.media.bigDropModel = trap_R_RegisterModel("models/weaponsfx/balloony_drop");
		cgs.media.hgrenb1aSound = trap_S_RegisterSound("sounds/weapons/balloony/balloonyb1a", qfalse);
		cgs.media.hgrenb2aSound = trap_S_RegisterSound("sounds/weapons/balloony/balloonyb2a", qfalse);
		cgs.media.BalloonyExplosion = trap_S_RegisterSound("sounds/weapons/balloony/explosion", qfalse);
		break;

	case WP_BAMBAM_MISSILE:
		// NOTE: missile is drawn as sprite directly in CG_Missile (cg_ents.c)
		break;

	case WP_BUBBLEG:
		// HERBY: BUBBLE G
		weaponInfo->missileModel = trap_R_RegisterModel("models/weaponsfx/gum");
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/bubbleg/flash", qfalse);
		cgs.media.gumMarkShader = trap_R_RegisterShader("gumMark");
		break;

	case WP_SPLASHER:
		weaponInfo->missileModel = trap_R_RegisterModel("models/weaponsfx/waterdrop");
		weaponInfo->missileSound = trap_S_RegisterSound("sounds/weapons/betty/fly", qfalse);
		weaponInfo->missileTrailFunc = CG_SplasherTrail;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 16;

		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/splasher/flash", qfalse);
		weaponInfo->readySound = trap_S_RegisterSound("sounds/weapons/splasher/ready", qfalse);
		cgs.media.waterTrailShader = trap_R_RegisterShader("waterTrail");
		cgs.media.waterBallShader = trap_R_RegisterShader("waterBall");
		cgs.media.smallDropModel = trap_R_RegisterModel("models/weaponsfx/smalldrop");
		for (i = 0; i <= 8; i++) {
			cgs.media.zoomsound[i] = trap_S_RegisterSound(va("sounds/weapons/splasher/zoom0%i", i), qtrue);
		}
		cgs.media.zoomhud = trap_R_RegisterShaderNoMip("zoomhud");
		cgs.media.zoomruler = trap_R_RegisterShaderNoMip("zoomruler");
		cgs.media.zoomcompass = trap_R_RegisterShaderNoMip("zoomcompass");
		break;

	case WP_KMA97:
		weaponInfo->missileModel = trap_R_RegisterModel("models/weaponsfx/kmadrop");
		weaponInfo->missileSound = trap_S_RegisterSound("sounds/weapons/betty/fly", qfalse);
		weaponInfo->missileTrailFunc = CG_KMATrail;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 16;

		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/kma97/flash", qfalse);
		cgs.media.kmaTrailShader = trap_R_RegisterShader("kmaTrail");
		cgs.media.kmaBallShader = trap_R_RegisterShader("kmaBall");
		cgs.media.smallKmaDropModel = trap_R_RegisterModel("models/weaponsfx/smallkmadrop");
		cgs.media.zoomhud_kma = trap_R_RegisterShaderNoMip("zoomhud_kma");
		cgs.media.zoomKMAaura = trap_R_RegisterShader("gfx/kmazoomAura");
		cgs.media.zoomKMAbluescreen = trap_R_RegisterShader("gfx/kmaBlueScreen");
		break;

	case WP_IMPERIUS:
		weaponInfo->readySound = trap_S_RegisterSound("sounds/weapons/imperius/ready", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.7f, 1);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/imperius/flash", qfalse);
		weaponInfo->missileTrailFunc = CG_ImperiusTrail;
		weaponInfo->wiTrailTime = 500;
		cgs.media.imperiusSphereModel = trap_R_RegisterModel("models/weaponsfx/impsphere");
		cgs.media.imperiusRingsModel = trap_R_RegisterModel("models/weaponsfx/imprings");
		cgs.media.imperiusBeamModel = trap_R_RegisterModel("models/weaponsfx/impbeam");
		cgs.media.imperiusCoreShader = trap_R_RegisterShader("imperiusCore");
		cgs.media.imperiusRingShader = trap_R_RegisterShader("imperiusRing");
		cgs.media.sfx_imperiusexp = trap_S_RegisterSound("sounds/weapons/imperius/explosion", qfalse);
		cgs.media.imperiuswarning = trap_S_RegisterSound("sounds/weapons/imperius/fully_loaded", qfalse);
		for (i = 0; i < 13; i++) {
			cgs.media.imperiusloading[i] =
				trap_S_RegisterSound(va("sounds/weapons/imperius/loading%i%i", ((i / 10) % 10), (i % 10)), qfalse);
		}
		break;

	case WP_SPRAYPISTOL:
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.7f, 1);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/spraypistol/spray", qfalse);
		// blue spraypistol
		cgs.media.blueSpraypistolskin = trap_R_RegisterSkin("models/weapons2/spraypistol/spraypistol_blue.skin");
		cgs.media.blueSpraypistolicon = trap_R_RegisterShader("icons/iconw_spraypistol_blue");
		cgs.media.neutralSpraypistolskin = trap_R_RegisterSkin("models/weapons2/spraypistol/spraypistol_neutral.skin");
		cgs.media.neutralSpraypistolicon = trap_R_RegisterShader("icons/iconw_spraypistol_neutral");
		cgs.media.blueCartridgeEntNum = (int)(BG_FindItem("blue Cartridge") - bg_itemlist);
		cgs.media.redCartridgeEntNum = (int)(BG_FindItem("red Cartridge") - bg_itemlist);
		cgs.media.neutralCartridgeEntNum = (int)(BG_FindItem("neutral Cartridge") - bg_itemlist);
		CG_RegisterItemVisuals(cgs.media.redCartridgeEntNum);
		CG_RegisterItemVisuals(cgs.media.blueCartridgeEntNum);
		CG_RegisterItemVisuals(cgs.media.neutralCartridgeEntNum);

		break;

	case WP_KILLERDUCKS:
		cgs.media.duckExplosionShader = trap_R_RegisterShader("newDuckExplosion");
		cgs.media.duckWheelModel = trap_R_RegisterModel("models/weapons2/killerducks/duckwheel");
		cgs.media.duckHeadModel = trap_R_RegisterModel("models/weapons2/killerducks/duckhead");
		cgs.media.duckExplosionSound = trap_S_RegisterSound("sounds/weapons/killerducks/explosion", qfalse);
		weaponInfo->missileModel = trap_R_RegisterModel("models/weapons2/killerducks/pad_killerduck");
		weaponInfo->missileSound = trap_S_RegisterSound("sounds/weapons/killerducks/walking", qfalse);
		break;

	default:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 1);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sounds/weapons/betty/flash", qfalse);
		break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals(int itemNum) {
	itemInfo_t *itemInfo;
	gitem_t *item;

	if (itemNum < 0 || itemNum >= bg_numItems) {
		CG_Error("CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems - 1);
	}

	itemInfo = &cg_items[itemNum];
	if (itemInfo->registered) {
		return;
	}

	item = &bg_itemlist[itemNum];

	memset(itemInfo, 0, sizeof(*itemInfo));
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel(item->world_model[0]);

	itemInfo->icon = trap_R_RegisterShader(item->icon);

	if (item->giType == IT_WEAPON) {
		CG_RegisterWeapon(item->giTag);
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if (item->giType == IT_POWERUP || item->giType == IT_HEALTH || item->giType == IT_ARMOR ||
		item->giType == IT_HOLDABLE) {
		if (item->world_model[1]) {
			itemInfo->models[1] = trap_R_RegisterModel(item->world_model[1]);
		}
	}
}

/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame(clientInfo_t *ci, int frame) {

	// change weapon
	if (frame >= ci->animations[TORSO_DROP].firstFrame && frame < ci->animations[TORSO_DROP].firstFrame + 9) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if (frame >= ci->animations[TORSO_ATTACK].firstFrame && frame < ci->animations[TORSO_ATTACK].firstFrame + 6) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if (frame >= ci->animations[TORSO_ATTACK2].firstFrame && frame < ci->animations[TORSO_ATTACK2].firstFrame + 6) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}

	// sprayattack ;)
	if (frame >= ci->animations[TORSO_SPRAYATTACK].firstFrame &&
		frame < ci->animations[TORSO_SPRAYATTACK].firstFrame + ci->animations[TORSO_SPRAYATTACK].numFrames) {
		return 1 + frame - ci->animations[TORSO_SPRAYATTACK].firstFrame;
	}

	return 0;
}

/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition(vec3_t origin, vec3_t angles) {
	float scale;
	int delta;
	float fracsin;

	VectorCopy(cg.refdef.vieworg, origin);
	VectorCopy(cg.refdefViewAngles, angles);

	// on odd legs, invert some angles
	if (cg.bobcycle & 1) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if (delta < LAND_DEFLECT_TIME) {
		origin[2] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
	} else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME) {
		origin[2] += cg.landChange * 0.25 * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin(cg.time * 0.001);
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}

/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void CG_LightningBolt(centity_t *cent, vec3_t origin) {
	localEntity_t *le;
	vec3_t forward, right;
	vec3_t muzzlePoint;
	trace_t tr;

	if (cent->currentState.weapon != WP_BOASTER) {
		return;
	}

	VectorCopy(origin, cgs.clientinfo[cent->currentState.number].realmuzzle);
	// is it time for a new LE?
	if (cg.time < cent->trailTime + 70)
		return;

	// calc muzzle
	CG_CalcMuzzlePoint(cent->currentState.number, muzzlePoint);
	if (cent->currentState.number == cg.snap->ps.clientNum)
		AngleVectors(cg.predictedPlayerState.viewangles, forward, right, NULL);
	else
		AngleVectors(cent->lerpAngles, forward, right, NULL);

	VectorMA(muzzlePoint, 15, right, muzzlePoint);
	muzzlePoint[2] -= 15;

	// check if the realmuzzlepoint(origin) isn't behind a wall
	CG_Trace(&tr, muzzlePoint, NULL, NULL, origin, cent->currentState.number, MASK_SHOT);
	if (tr.fraction != 1.0f || tr.startsolid) {
		if (!(tr.surfaceFlags & SURF_NOIMPACT))
			CG_MissileHitWall(WP_BOASTER, (tr.entityNum == ENTITYNUM_WORLD && tr.plane.normal[2] > 0.8), tr.endpos,
							  tr.plane.normal, 0, IMPACTSOUND_DEFAULT);

		return;
	}

	// add a new local entity
	le = CG_AllocLocalEntity();

	le->leType = LE_WATERBEAM;
	le->startTime = cg.time - 10; // -10 => like le->pos.trTime
	le->endTime = cg.time + 1500;
	le->ownerNum = cent->currentState.number;

	if (cent->trailLE && cg.time < cent->trailTime + 200) {
		if (DotProduct(forward, cent->beamEnd) > 0.98) {
			// merge with an older one
			cent->trailLE->newer = le;
			le->older = cent->trailLE;
		} else {
			// be independent
			cent->trailLE->newer = cent->trailLE;
			le->older = le;
		}
	}

	VectorCopy(forward, cent->beamEnd);

	cent->trailLE = le;
	cent->trailTime = cg.time - 10;
	forward[2] += 0.15f;

	// initialize movement vars
	le->pos.trTime = cg.time - 10;
	le->pos.trType = TR_GRAVITY;
	// backup the sv like movment in the not used angles-struct
	VectorCopy(muzzlePoint, le->angles.trBase);
	VectorScale(forward, 1250, le->angles.trDelta);
	le->angles.trTime = 1;

	// setting the trBase like the missile came out of the weapon-model
	// ... we will change the trBase back after some movement (with the backup in angles.trBase)
	VectorCopy(origin, le->pos.trBase);
	//	VectorScale( forward, 1250, le->pos.trDelta );
	le->pos.trDelta[0] = le->angles.trDelta[0] + (muzzlePoint[0] - origin[0]) * 6.666667f; // 1/0.15=6.666667f
	le->pos.trDelta[1] = le->angles.trDelta[1] + (muzzlePoint[1] - origin[1]) * 6.666667f;
	le->pos.trDelta[2] = le->angles.trDelta[2] + (muzzlePoint[2] - origin[2]) * 6.666667f;
}

void CG_GetWaterMuzzle(localEntity_t *le, centity_t *cent, vec3_t fw) {
	vec3_t right;
	vec3_t muzzle;

	VectorClear(muzzle);
	CG_CalcMuzzlePoint(cent->currentState.number, muzzle);
	if (cent->currentState.number == cg.snap->ps.clientNum) {
		AngleVectors(cg.predictedPlayerState.viewangles, fw, right, NULL);
	} else {
		AngleVectors(cent->lerpAngles, fw, right, NULL);
	}

	VectorMA(muzzle, 15, right, muzzle);
	muzzle[2] -= 15;
	VectorCopy(muzzle, le->color);
}

/*
======================
CG_MachinegunSpinAngle
======================
*/
#define SPIN_SPEED 0.9
#define COAST_TIME 1000
static float CG_MachinegunSpinAngle(centity_t *cent) {
	int delta;
	float angle;
	float speed;

	delta = cg.time - cent->pe.barrelTime;
	if (cent->pe.barrelSpinning) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if (delta > COAST_TIME) {
			delta = COAST_TIME;
		}

		speed = 0.5 * (SPIN_SPEED + (float)(COAST_TIME - delta) / COAST_TIME);
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if (cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING)) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod(angle);
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
	}

	return angle;
}

/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups(refEntity_t *gun, int powerups, int weap) {
	if (powerups & (1 << PW_VISIONLESS)) {
		gun->customShader = cgs.media.invisShader;
	} else {
		if (powerups & (1 << PW_PADPOWER)) {
			if (weap == WP_PUNCHY && cgs.media.PunchyPadPowerSkin)
				gun->customSkin = cgs.media.PunchyPadPowerSkin;
			else
				gun->customShader = cgs.media.PadPowerShader;
			gun->shaderRGBA[0] = gun->shaderRGBA[1] = gun->shaderRGBA[2] = 0xff;
		}
		if (powerups & (1 << PW_BERSERKER) && weap == WP_PUNCHY) {
			if (cgs.media.BerserkerPunchy)
				gun->customSkin = cgs.media.BerserkerPunchy;
			else {
				trap_R_AddRefEntityToScene(gun);

				gun->customShader = cgs.media.BerserkerAura;
				//				gun->shaderRGBA[0]=0xff;ent->shaderRGBA[1]=ent->shaderRGBA[2]=0;
			}
		}
	}
	trap_R_AddRefEntityToScene(gun);
}

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team) {
	refEntity_t gun;
	refEntity_t barrel;
	refEntity_t flash;
	vec3_t angles;
	weapon_t weaponNum;
	weaponInfo_t *weapon;
	centity_t *nonPredictedCent;
	//	int	col;

	weaponNum = cent->currentState.weapon;

	CG_RegisterWeapon(weaponNum);
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset(&gun, 0, sizeof(gun));
	VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	if (cent->currentState.eFlags & (EF_DEAD | EF_NOLIFESLEFT))
		cent->currentState.eFlags &= ~EF_FIRING; // vielleicht hilft das gegen den boaster strahl bei toten =/

	gun.hModel = weapon->weaponModel;
	if (weaponNum == WP_SPRAYPISTOL && team == TEAM_BLUE)
		gun.customSkin = cgs.media.blueSpraypistolskin;
	else if (weaponNum == WP_SPRAYPISTOL && team == TEAM_FREE)
		gun.customSkin = cgs.media.neutralSpraypistolskin;

	if (!gun.hModel) {
		return;
	}

	if (!ps) {
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if ((cent->currentState.eFlags & EF_FIRING) && weapon->firingSound) {
			// lightning gun and guantlet make a different sound when fire is held down
			if (weaponNum == WP_PUNCHY && cent->currentState.powerups & (1 << PW_BERSERKER))
				trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin,
									   cgs.media.berserkerPunchyLoop);
			else
				trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
			cent->pe.lightningFiring = qtrue;
		} else if (weapon->readySound) {
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound);
		}
	}

	gun.oldframe = cent->lastweaponframe;

	switch (cent->currentState.weapon) {
	default:
		gun.frame = 0;
		break;
	case WP_SPLASHER:
		//		gun.frame=(int)(((float)(cg.time - cent->muzzleFlashTime))*0.022f);
		gun.frame = (int)((cg.time - cent->muzzleFlashTime) / 45); // 45.454545 ... woher hatte ich die zahl o_O
		if (gun.frame > 23)
			gun.frame = 0;

		//		if(gun.frame<29)
		//			gun.frame=gun.frame%29;
		//		else
		//			gun.frame=0;

		break;
	case WP_PUNCHY:
		// 0.022 f/ms
		//		gun.frame=((int)(cg.time*0.022f))%29;
		if (cent->currentState.eFlags & EF_FIRING)
			//			gun.frame=(((int)(cg.time*0.022f))%32)+28;//29-60
			//			gun.frame=((cg.time/45)%32)+28;//29-60
			gun.frame = ((int)(cg.time * 0.035f) % 15) + 44; // 45-60
		else
			//			gun.frame=((int)(cg.time*0.022f))%28;//1-28
			//			gun.frame=(cg.time/45)%28;//1-28
			gun.frame = (int)(cg.time * 0.022f) % 44; // 1-44

		break;
	}

	cent->lastweaponframe = gun.frame;

	CG_PositionEntityOnTag(&gun, parent, parent->hModel, "tag_weapon");

	CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups, cent->currentState.weapon);

	// add the spinning barrel
	if (weapon->barrelModel) {
		memset(&barrel, 0, sizeof(barrel));
		VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle(cent);
		AnglesToAxis(angles, barrel.axis);

		CG_PositionRotatedEntityOnTag(&barrel, &gun, weapon->weaponModel, "tag_barrel");

		CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups, cent->currentState.weapon);
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if ((nonPredictedCent - cg_entities) != cent->currentState.clientNum) {
		nonPredictedCent = cent;
	}

	// add the flash
	if ((weaponNum == WP_IMPERIUS || weaponNum == WP_BOASTER || weaponNum == WP_GRAPPLING_HOOK ||
		 weaponNum == WP_SPRAYPISTOL) &&
		(nonPredictedCent->currentState.eFlags & EF_FIRING)) {
		// continuous flash
	} else {
		// impulse flash
		if (!((weaponNum == WP_PUMPER || weaponNum == WP_BUBBLEG || weaponNum == WP_NIPPER || weaponNum == WP_BETTY ||
			   weaponNum == WP_BALLOONY || weaponNum == WP_SPLASHER || weaponNum == WP_KMA97) &&
			  cg.time - cent->muzzleFlashTime <
				  MUZZLE_FLASH_TIME)) // MUZZLE_FLASH_TIME=80 ... increasing it slightly
		{
			return;
		}
	}

	memset(&flash, 0, sizeof(flash));
	VectorCopy(parent->lightingOrigin, flash.lightingOrigin);
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	if (!flash.hModel) {
		return;
	}

	// KMA flash color
	if (weaponNum == WP_KMA97) {
		clientInfo_t *cinfo;
		entityState_t *es;

		es = &cent->currentState;
		cinfo = &cgs.clientinfo[es->clientNum];

		// forced team color
		if (cgs.gametype >= GT_TEAM) {
			if (cinfo->team == TEAM_BLUE) {
				flash.shaderRGBA[0] = 0;
				flash.shaderRGBA[1] = 0;
				flash.shaderRGBA[2] = 255;
				flash.shaderRGBA[3] = 255;
			} else {
				flash.shaderRGBA[0] = 255;
				flash.shaderRGBA[1] = 0;
				flash.shaderRGBA[2] = 0;
				flash.shaderRGBA[3] = 255;
			}
		} else // ffa-type color
		{
			flash.shaderRGBA[0] = cinfo->color2[0] * 255;
			flash.shaderRGBA[1] = cinfo->color2[1] * 255;
			flash.shaderRGBA[2] = cinfo->color2[2] * 255;
			flash.shaderRGBA[3] = 255;

			// force a different color for "black" since black doesn't show up
			if (flash.shaderRGBA[0] == 0 && flash.shaderRGBA[1] == 0 && flash.shaderRGBA[2] == 0) {
				flash.shaderRGBA[0] = (unsigned int)0.8 * 255;
				flash.shaderRGBA[1] = 0;
				flash.shaderRGBA[2] = 255;
			}
		}
	}

	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis(angles, flash.axis);

	// HERBY: Imperius charging, continuous scaling for local client, two step scaling for all the others
	if (!ps) {
		if (weaponNum == WP_IMPERIUS) {
			if (!(nonPredictedCent->currentState.eFlags & EF_CHARGED))
				AxisScale(flash.axis, 0.3f, flash.axis);
			else if (cent->currentState.clientNum == cg.snap->ps.clientNum && cg.imp_lastwarningsound + 500 < cg.time) {
				trap_S_StartLocalSound(cgs.media.imperiuswarning, CHAN_LOCAL_SOUND);
				cg.imp_lastwarningsound = cg.time;
			}
		}
	} else {
		if (ps->weaponstate == WEAPON_CHARGING && ps->weaponTime < CHARGETIME_IMPERIUS) {
			static int ImpLoadingSoundsPlayed = 0;

			AxisScale(flash.axis, (float)ps->weaponTime / CHARGETIME_IMPERIUS, flash.axis); //<-- von Herby ;)

			if (cg.imp_startloading == 0) {
				trap_S_StartLocalSound(cgs.media.imperiusloading[0], CHAN_LOCAL_SOUND);
				cg.imp_startloading = cg.time;
				ImpLoadingSoundsPlayed = 0;
			}

			if (ImpLoadingSoundsPlayed <= 11 && (cg.time - cg.imp_startloading) >= 200 * (1 + ImpLoadingSoundsPlayed)) {
				trap_S_StartLocalSound(cgs.media.imperiusloading[ImpLoadingSoundsPlayed + 1], CHAN_LOCAL_SOUND);
				ImpLoadingSoundsPlayed++;
			}

		} else if (ps->weaponstate == WEAPON_CHARGING && ps->weaponTime > CHARGETIME_IMPERIUS &&
				   cg.imp_lastwarningsound + 500 < cg.time) {
			trap_S_StartLocalSound(cgs.media.imperiuswarning, CHAN_LOCAL_SOUND);
			cg.imp_lastwarningsound = cg.time;
		}
	}
	if (ps && ps->weaponstate != WEAPON_CHARGING)
		cg.imp_startloading = 0;

	CG_PositionRotatedEntityOnTag(&flash, &gun, weapon->weaponModel, "tag_flash");

	// no flash but we need the flash-origin
	if (weaponNum == WP_SPRAYPISTOL) {
		if (ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum)
			CG_SprayTrail(nonPredictedCent, flash.origin);
		return;
	}

	trap_R_AddRefEntityToScene(&flash);

	if (ps || cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum) {
		// add lightning bolt
		CG_LightningBolt(nonPredictedCent, flash.origin);

		if (weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2]) {
			trap_R_AddLightToScene(flash.origin, 300 + (rand() & 31), weapon->flashDlightColor[0],
								   weapon->flashDlightColor[1], weapon->flashDlightColor[2]);
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon(playerState_t *ps) {
	refEntity_t hand;
	centity_t *cent;
	clientInfo_t *ci;
	float fovOffset;
	vec3_t angles;
	weaponInfo_t *weapon;

	float xOffset, yOffset, zOffset;

	if (cgs.gametype == GT_LPS && ps->stats[STAT_LIVESLEFT] <= 0)
		return;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if (ps->pm_type == PM_INTERMISSION) {
		return;
	}

	// no gun if in third person view or a camera is active
	// if ( cg.renderingThirdPerson || cg.cameraMode) {
	if (cg.renderingThirdPerson) {
		return;
	}

	// allow the gun to be completely removed
	if (!cg_drawGun.integer || cg.zoomed) {
		vec3_t origin;

		if (cg.predictedPlayerState.eFlags & EF_FIRING) {
			// special hack for lightning gun...
			VectorCopy(cg.refdef.vieworg, origin);
			VectorMA(origin, -8, cg.refdef.viewaxis[2], origin);
			CG_LightningBolt(&cg_entities[ps->clientNum], origin);
		}
		return;
	}

	// don't draw if testing a gun model
	if (cg.testGun) {
		return;
	}

	// drop gun lower at higher fov
	if (cg_fov.integer > 90) {
		fovOffset = -0.2f * (cg_fov.integer - 90);
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity; // &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon(ps->weapon);
	weapon = &cg_weapons[ps->weapon];

	memset(&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition(hand.origin, angles);

	xOffset = cg_gun_x.value;
	yOffset = cg_gun_y.value;
	zOffset = (cg_gun_z.value + fovOffset);

	if (cg_correctedWeaponPos.integer) {
		switch (ps->weapon) {
		case WP_PUNCHY:
			xOffset += 4;
			yOffset += -11;
			zOffset += -1;
			break;
		case WP_NIPPER:
			xOffset += 8;
			yOffset += -4;
			zOffset += -1;
			break;
		case WP_PUMPER:
			xOffset += -2;
			yOffset += -2;
			//			zOffset+=0;
			break;
		case WP_BALLOONY:
			xOffset += 2;
			yOffset += -3;
			zOffset += -1;
			break;
		case WP_BETTY:
			xOffset += 7;
			yOffset += -7;
			zOffset += -4;
			break;
		case WP_BOASTER:
			xOffset += 2;
			yOffset += -2;
			zOffset += -2;
			break;
		case WP_SPLASHER:
			xOffset += 19;
			yOffset += -7;
			zOffset += -1;
			break;
		case WP_BUBBLEG:
			xOffset += 1;
			yOffset += -3;
			zOffset += 0;
			break;
		case WP_IMPERIUS:
			xOffset += -1;
			yOffset += -2;
			zOffset += 2;
			break;
		case WP_KMA97:
			xOffset += 19;
			yOffset += -7;
			zOffset += -1;
			break;
			//	case WP_GRAPPLING_HOOK:
			//	case WP_KILLERDUCKS://keine echte waffe mehr
		case WP_SPRAYPISTOL:
			xOffset += 10;
			yOffset += -3;
			//		zOffset+=0;
			break;
		}
	}
	zOffset -= (cg.speedyeffect - 1.0f) * 2 * 7;

	VectorMA(hand.origin, xOffset, cg.refdef.viewaxis[0], hand.origin);
	VectorMA(hand.origin, yOffset, cg.refdef.viewaxis[1], hand.origin);
	VectorMA(hand.origin, zOffset, cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis(angles, hand.axis);

	// map torso animations to weapon animations
	if (cg_gun_frame.integer) {
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	} else {
		// get clientinfo for animation map
		ci = &cgs.clientinfo[cent->currentState.clientNum];
		hand.frame = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.frame);
		hand.oldframe = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.oldFrame);
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon(&hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM]);
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect(void) {
	int i;
	int bits;
	int count;
	int x, y, w;
	char *name;
	float *color;

	// don't display if dead
	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) {
		return;
	}

	color = CG_FadeColor(cg.weaponSelectTime, WEAPON_SELECT_TIME);
	if (!color) {
		return;
	}
	trap_R_SetColor(color);

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	bits = cg.snap->ps.stats[STAT_WEAPONS];

	if ((cgs.gametype == GT_SPRAYFFA) || (cgs.gametype == GT_SPRAY)) {
		if (cg.snap->ps.stats[STAT_SPRAYROOMSECS]) {
			bits = (1 << WP_SPRAYPISTOL);
		} else {
			bits &= ~(1 << WP_SPRAYPISTOL);
		}
	}

	count = 0;
	for (i = 1; i < 16; i++) {
		if (bits & (1 << i)) {
			count++;
		}
	}

	x = 320 - count * 20;
	y = 360;

	for (i = 1; i < 16; i++) {
		if (!(bits & (1 << i))) {
			continue;
		}

		CG_RegisterWeapon(i);

		if (i == WP_SPRAYPISTOL && cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
			CG_DrawPic(x, y, 32, 32, cgs.media.blueSpraypistolicon);
		else if (i == WP_SPRAYPISTOL && cg.snap->ps.persistant[PERS_TEAM] == TEAM_FREE)
			CG_DrawPic(x, y, 32, 32, cgs.media.neutralSpraypistolicon);
		else

			// draw weapon icon
			CG_DrawPic(x, y, 32, 32, cg_weapons[i].weaponIcon);

		// draw selection marker
		if (i == cg.weaponSelect) {
			CG_DrawPic(x - 4, y - 4, 40, 40, cgs.media.selectShader);
		}

		// no ammo cross on top
		if (!cg.snap->ps.ammo[i]) {
			CG_DrawPic(x, y, 32, 32, cgs.media.noammoShader);
		}

		x += 40;
	}

	// draw the selected name
	if (cg_weapons[cg.weaponSelect].item) {
		name = cg_weapons[cg.weaponSelect].item->pickup_name;
		if (name) {
			w = CG_DrawStrlen(name) * BIGCHAR_WIDTH;
			x = (SCREEN_WIDTH - w) / 2;
			CG_DrawBigStringColor(x, y - 22, name, color);
		}
	}

	trap_R_SetColor(NULL);
}

/*
===============
CG_WeaponSelectableSprayroom
===============
*/
static qboolean CG_WeaponSelectableSprayroom(int weapon) {
	if ((GT_SPRAYFFA == cgs.gametype) || (GT_SPRAY == cgs.gametype)) {
		// Player is in sprayroom
		if (cg.snap->ps.stats[STAT_SPRAYROOMSECS] > 0) {
			if (WP_SPRAYPISTOL == weapon) {
				return qtrue;
			} else {
				return qfalse;
			}
		}
	}

	// any other gametype and situation
	if (WP_SPRAYPISTOL == weapon) {
		return qfalse;
	}

	// default allow
	return qtrue;
}

/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable(int i) {
	if ((i <= WP_NONE) || (i > /*= *WP_NUM_WEAPONS*/ WP_SPRAYPISTOL)) {
		return qfalse;
	}

	if (!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << i))) {
		return qfalse;
	}

	if (!cg.snap->ps.ammo[i]) {
		return qfalse;
	}

	if (!CG_WeaponSelectableSprayroom(i)) {
		return qfalse;
	}

	if (cg.snap->ps.powerups[PW_BERSERKER]) {
		if ((i != WP_PUNCHY) && (i != WP_SPRAYPISTOL)) {
			return qfalse;
		}
	}

	return qtrue;
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f(void) {
	int i;
	int original;

	if (cg.zoomed)
		CG_ZoomDown_f();

	if (!cg.snap) {
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for (i = 0; i < WP_NUM_WEAPONS; i++) {
		cg.weaponSelect++;
		if (cg.weaponSelect == WP_NUM_WEAPONS) {
			cg.weaponSelect = WP_NONE;
		}

		if (CG_WeaponSelectable(cg.weaponSelect)) {
			break;
		}
	}
	if (i >= WP_NUM_WEAPONS) {
		cg.weaponSelect = original;
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f(void) {
	int i;
	int original;

	if (cg.zoomed)
		CG_ZoomDown_f();

	if (!cg.snap) {
		return;
	}
	if (cg.snap->ps.pm_flags & PMF_FOLLOW) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for (i = 0; i < WP_NUM_WEAPONS; i++) {
		cg.weaponSelect--;
		if (cg.weaponSelect == -1) {
			cg.weaponSelect = WP_NUM_WEAPONS - 1;
		}

		if (CG_WeaponSelectable(cg.weaponSelect)) {
			break;
		}
	}
	if (i >= WP_NUM_WEAPONS) {
		cg.weaponSelect = original;
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f(void) {
	int num;

	if (!cg.snap) {
		return;
	}
	if (cg.snap->ps.pm_flags & PMF_FOLLOW) {
		return;
	}

	num = atoi(CG_Argv(1));

	if (cg.zoomed)
		CG_ZoomDown_f();
	// direkte wahl einer waffe mit 0 amma soll möglich sein -> deshalb CG_WeaponSelectable nicht sinnvoll
	//	if(CG_WeaponSelectable(num))	return;

	if (!CG_WeaponSelectableSprayroom(num)) {
		return;
	}

	if (cg.snap->ps.powerups[PW_BERSERKER]) {
		if ((num != WP_PUNCHY) && (num != WP_SPRAYPISTOL)) {
			return;
		}
	}

	if ((num < WP_PUNCHY) || (num > WP_SPRAYPISTOL)) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	if (!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num))) {
		return; // don't have the weapon
	}

	cg.weaponSelect = num;
}

void CG_WeaponSRWC(int weaponNum) {
	if (cg.zoomed) {
		CG_ZoomDown_f();
	}

	// Using CG_WeaponSelectable() here does not work ( calls flawed CG_WeaponSelectableSprayroom() ), thus copy code.
	// I guess the "srwc" command arrives before STAT_SPRAYROOMSECS is updated.
	if ((weaponNum <= WP_NONE) || (weaponNum > /*= *WP_NUM_WEAPONS*/ WP_SPRAYPISTOL)) {
		return;
	}
	if (!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << weaponNum))) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	cg.weaponSelect = weaponNum;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange(void) {
	int i;

	if (cg.zoomed)
		CG_ZoomDown_f();

	cg.weaponSelectTime = cg.time;

	for (i = WP_NUM_WEAPONS - 1; i > WP_NONE; i--) {
		if (CG_WeaponSelectable(i)) {
			cg.weaponSelect = i;
			break;
		}
	}
}

/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon(centity_t *cent) {
	entityState_t *ent;
	int c;
	weaponInfo_t *weap;

	ent = &cent->currentState;
	if (ent->weapon == WP_NONE) {
		return;
	}
	if (ent->weapon >= WP_NUM_WEAPONS) {
		CG_Error("CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS");
		return;
	}
	weap = &cg_weapons[ent->weapon];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	// lightning gun only does this this on initial press
	if (ent->weapon == WP_BOASTER) {
		if (cent->pe.lightningFiring) {
			return;
		}
	}

	// play quad sound if needed
	if (cent->currentState.powerups & (1 << PW_PADPOWER)) {
		trap_S_StartSound(NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound);
	}

	// play a sound
	for (c = 0; c < 4; c++) {
		if (!weap->flashSound[c]) {
			break;
		}
	}
	if (c > 0) {
		c = rand() % c;
		if (weap->flashSound[c]) {
			if (ent->weapon == WP_PUNCHY && ent->powerups & (1 << PW_BERSERKER))
				trap_S_StartSound(NULL, ent->number, CHAN_WEAPON, weap->flashSound[3]);
			else
				trap_S_StartSound(NULL, ent->number, CHAN_WEAPON, weap->flashSound[c]);
		}
	}

	// do brass ejection
	// No brass in WoP, also removed cg_brassTime
	// if ( weap->ejectBrassFunc && cg_brassTime.integer > 0 ) {
	//	weap->ejectBrassFunc( cent );
	//}
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
// FIXME: why do we use the clientNum-var so strange? ��
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, int colorCode, impactSound_t soundType) {
	qhandle_t mod;
	qhandle_t mark;
	qhandle_t shader;
	sfxHandle_t sfx;
	float radius;
	float light;
	vec3_t lightColor;
	localEntity_t *le;
	qboolean alphaFade;
	qboolean isSprite;
	int duration;
	vec3_t sprVel;

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch (weapon) {
	default:
	case WP_BOASTER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.boasterExplosionShader;
		radius = 32;
		isSprite = qtrue;
		duration = 400;
		sfx = 0;
		mark = cgs.media.bulletMarkShader;
		radius = 16;
		break;
	case WP_BALLOONY:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.waterExplosionShader;
		sfx = cgs.media.BalloonyExplosion;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		duration = 600;
		isSprite = qtrue;
		VectorSet(sprVel, 0, 0, 300);
		CG_GenerateParticles(cgs.media.bigDropModel, 0, origin, 16, sprVel, 120, 200, 30, 0, cg.time, 500, 70, 0, 0, 0,
							 0, LEF_GRAVITY, 0);
		break;
	case WP_BETTY:
		if (!CG_FreezeTag()) {
			mod = cgs.media.dishFlashModel;
		}
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.BettyExplosion;
		if (CG_FreezeTag()) {
			mark = cgs.media.snowMarkShader;
		} else {
			mark = cgs.media.burnMarkShader;
		}
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		if (CG_FreezeTag()) {
			lightColor[0] = 0.0f;
			lightColor[1] = 0.2f;
			lightColor[2] = 1.0f;
		} else {
			lightColor[0] = 1;
			lightColor[1] = 0.75;
			lightColor[2] = 0.0;
		}
		VectorSet(sprVel, 0, 0, 400);
		CG_GenerateParticles(cgs.media.fireDropModel, 0, origin, 16, sprVel, 120, 250, 30, 0, cg.time, 700, 400, 0, 0,
							 0, 0, LEF_GRAVITY | LEF_COLLISIONS, 0);
		break;
	case WP_BAMBAM_MISSILE:
		sfx = cgs.media.bambamMissileImpact;

		if (colorCode == TEAM_BLUE)
			shader = cgs.media.bambamImpactDropsBlueShader;
		else if (colorCode == TEAM_RED)
			shader = cgs.media.bambamImpactDropsRedShader;

		CG_GenerateParticles(cgs.media.bambamImpactDrops, shader, origin, 2, vec3_origin, 120, 300, 10, 0, cg.time, 300,
							 200, 0, 0, 0, 0, 0, 0);

		shader = 0;
		break;
	case WP_SPLASHER:
		mark = cgs.media.waterMarkShader;
		radius = 8;
		CG_GenerateParticles(cgs.media.smallDropModel, 0, origin, 2, vec3_origin, 120, 300, 20, 0, cg.time, 300, 200, 0,
							 0, 0, 0, 0, 0);
		break;
	case WP_BUBBLEG:
		// HERBY: Bubble G
		if (CG_FreezeTag()) {
			mark = cgs.media.snowMarkShader;
		} else {
			mark = cgs.media.gumMarkShader;
		}
		radius = 24;
		mod = 0;
		shader = 0;
		sfx = 0;
		break;
	case WP_IMPERIUS:
		le = CG_AllocLocalEntity();
		le->leType = LE_IMPERIUSBOOM;
		le->startTime = cg.time;
		le->endTime = cg.time + 1000;
		le->lifeRate = 1.0f / (le->endTime - le->startTime);
		le->radius = rand();
		VectorMA(origin, 16, dir, le->refEntity.origin);
		VectorSet(le->angles.trBase, 360 * crandom(), 360 * crandom(), 360 * crandom());

		CG_GenerateParticles(cgs.media.imperiusBeamModel, 0, origin, 16, vec3_origin, 360, 1500, 50, 0, cg.time, 700,
							 400, 0, 0, 0, 0, 0, 0);

		mark = cgs.media.burnMarkShader;
		radius = 32;
		sfx = cgs.media.sfx_imperiusexp;
		break;
	case WP_PUMPER:
		mod = cgs.media.pumperFlashModel;
		//		shader = cgs.media.railExplosionShader;
		sfx = cgs.media.pumperexpSound;
		if (CG_FreezeTag()) {
			mark = cgs.media.snowMarkShader;
		} else {
			mark = cgs.media.burnMarkShader;
		}
		radius = 24;
		break;

	case WP_NIPPER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.nipperWaveShader;
		mark = cgs.media.burnMarkShader;
		radius = 4;
		break;
		//	case WP_SPRAYPISTOL://noch mal drueber nachdenken ... eigentlich brauch ich keine explosion
		//		break;
	case WP_KILLERDUCKS:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.duckExplosionShader;
		sfx = cgs.media.duckExplosionSound;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		light = 300;
		isSprite = qtrue;
		duration = 800;
		lightColor[0] = 1.0f;
		lightColor[1] = 0.7f;
		lightColor[2] = 0.0f;

		VectorSet(sprVel, 0, 0, 300);
		CG_GenerateParticles(cgs.media.duckWheelModel, 0, origin, 8, sprVel, 45, 150, 4, 0, cg.time, 3000, 500, 0, 0, 0,
							 0, LEF_GRAVITY | LEF_TUMBLE | LEF_COLLISIONS, 0);
		CG_GenerateParticles(cgs.media.duckHeadModel, 0, origin, 8, sprVel, 45, 150, 1, 0, cg.time, 3000, 500, 0, 0, 0,
							 0, LEF_GRAVITY | LEF_TUMBLE | LEF_COLLISIONS, 0);
		break;

	case WP_KMA97:
		mark = cgs.media.kmaMarkShader;
		radius = 8;
		CG_GenerateParticles(cgs.media.smallKmaDropModel, 0, origin, 2, vec3_origin, 120, 300, 20, 0, cg.time, 300, 200,
							 0, 0, 0, 0, 0, 0);
		break;
	}

	if (sfx) {
		trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);
	}

	//
	// create the explosion
	//
	if (mod) {
		le = CG_MakeExplosion(origin, dir, mod, shader, duration, isSprite);
		le->light = light;
		VectorCopy(lightColor, le->lightColor);
	}

	//
	// impact mark
	//
	if (!mark)
		return;

	if (weapon == WP_BOASTER && clientNum) // clientNum wird hier missbraucht ;P
		return;
	// HERBY: Bubble G
	alphaFade = (mark == cgs.media.gumMarkShader);
	if (weapon == WP_BUBBLEG) {
		float *color;

		// colorize with client color
		if (CG_FreezeTag()) {
			color = g_color_table[ColorIndex(COLOR_WHITE)];
		} else {
			color = g_color_table[colorCode]; // HERBY
		}

		CG_ImpactMark(mark, origin, dir, random() * 360, color[0], color[1], color[2], 1, alphaFade, radius, qfalse);
	} else {
		CG_ImpactMark(mark, origin, dir, random() * 360, 1, 1, 1, 1, alphaFade, radius, qfalse);
	}
}

/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int ColorCode, int entityNum) { // HERBY
	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch (weapon) {
	// HERBY ... hat da auch bischen dazu getan...
	case WP_BALLOONY:
	case WP_BETTY:
	case WP_IMPERIUS:
	case WP_BUBBLEG:
	case WP_BAMBAM_MISSILE:
	case WP_SPLASHER:
	case WP_KMA97:
		CG_MissileHitWall(weapon, 0, origin, dir, ColorCode, IMPACTSOUND_FLESH); // HERBY
		break;
	default:
		break;
	}
}

/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire(entityState_t *es) {
	// seed und otherEntNum ungenutzt oder?
	// noch so ändern das ich es nicht nochmal berechnen muss
	vec3_t start, end;
	vec3_t forward, right, up;
	trace_t tr;

	start[0] = es->pos.trBase[0];
	start[1] = es->pos.trBase[1];
	start[2] = es->pos.trBase[2];

	VectorNormalize2(es->origin2, forward);
	PerpendicularVector(right, forward);
	CrossProduct(forward, right, up);
	end[0] = start[0] + 1000 * forward[0];
	end[1] = start[1] + 1000 * forward[1];
	end[2] = start[2] + 1000 * forward[2];

	CG_Trace(&tr, start, NULL, NULL, end, 0, MASK_SHOT);
	end[0] = tr.endpos[0];
	end[1] = tr.endpos[1];
	end[2] = tr.endpos[2];

	{
		localEntity_t *le;
		vec3_t delta, right;
		int i;

		// calculate correct positions for double rails
		VectorSubtract(end, start, delta);
		right[0] = delta[1];
		right[1] = -delta[0];
		right[2] = 0;
		VectorNormalize(right);

		VectorMA(start, 6, right, start);
		start[2] -= 14;

		for (i = -1; i < 2; i += 2) {
			// alloc new local entity
			le = CG_AllocLocalEntity();
			le->leType = LE_PUMPERTRAIL;
			le->startTime = cg.time;
			le->endTime = cg.time + 500;
			le->lifeRate = i * M_PI / 4;

			VectorMA(start, 6 * i, right, le->pos.trBase);
			VectorMA(end, 6 * i, right, le->pos.trDelta);
		}
	}
}

/*
============================================================================

BULLETS

============================================================================
*/

/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle) {
	vec3_t forward;
	centity_t *cent;
	int anim;

	if (entityNum == cg.snap->ps.clientNum) {
		VectorCopy(cg.snap->ps.origin, muzzle);
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
		VectorMA(muzzle, 14, forward, muzzle);
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if (!cent->currentValid) {
		return qfalse;
	}

	VectorCopy(cent->currentState.pos.trBase, muzzle);

	AngleVectors(cent->currentState.apos.trBase, forward, NULL, NULL);
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if (anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA(muzzle, 14, forward, muzzle);

	return qtrue;
}
