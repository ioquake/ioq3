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
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t *pm;
pml_t pml;

// movement parameters
float pm_stopspeed = 100.0f;
float pm_duckScale = 0.25f;
float pm_swimScale = 0.50f;

float pm_accelerate = 10.0f;
float pm_airaccelerate = 1.0f;
float pm_wateraccelerate = 4.0f;
float pm_flyaccelerate = 8.0f;

float pm_friction = 6.0f;
float pm_waterfriction = 1.0f;
float pm_flightfriction = 3.0f;
float pm_spectatorfriction = 5.0f;

int c_pmove = 0;

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent(int newEvent) {
	BG_AddPredictableEventToPlayerstate(newEvent, 0, pm->ps);
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt(int entityNum) {
	int i;

	if (entityNum == ENTITYNUM_WORLD) {
		return;
	}
	if (pm->numtouch == MAXTOUCH) {
		return;
	}

	// see if it is already added
	for (i = 0; i < pm->numtouch; i++) {
		if (pm->touchents[i] == entityNum) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim(int anim) {
	if (pm->ps->pm_type >= PM_DEAD) {
		return;
	}
	pm->ps->torsoAnim = ((pm->ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}
static void PM_StartLegsAnim(int anim) {
	if (pm->ps->pm_type >= PM_DEAD) {
		return;
	}
	if (pm->ps->legsTimer > 0) {
		return; // a high priority animation is running
	}
	pm->ps->legsAnim = ((pm->ps->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}

static void PM_ContinueLegsAnim(int anim) {
	if ((pm->ps->legsAnim & ~ANIM_TOGGLEBIT) == anim) {
		return;
	}
	if (pm->ps->legsTimer > 0) {
		return; // a high priority animation is running
	}
	PM_StartLegsAnim(anim);
}

static void PM_ContinueTorsoAnim(int anim) {
	if ((pm->ps->torsoAnim & ~ANIM_TOGGLEBIT) == anim) {
		return;
	}
	if (pm->ps->torsoTimer > 0) {
		return; // a high priority animation is running
	}
	PM_StartTorsoAnim(anim);
}

static void PM_ForceLegsAnim(int anim) {
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim(anim);
}

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce) {
	float backoff;
	float change;
	int i;

	backoff = DotProduct(in, normal);

	if (backoff < 0) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for (i = 0; i < 3; i++) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction(void) {
	vec3_t vec;
	float *vel;
	float speed, newspeed, control;
	float drop;

	vel = pm->ps->velocity;

	VectorCopy(vel, vec);
	if (pml.walking) {
		vec[2] = 0; // ignore slope movement
	}

	speed = VectorLength(vec);
	if (speed < 1) {
		vel[0] = 0;
		vel[1] = 0; // allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if (pm->waterlevel <= 1) {
		if (pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK)) {
			// if getting knocked back, no friction
			if (!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK)) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control * pm_friction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if (pm->waterlevel) {
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
	}

	if (pm->ps->pm_type == PM_SPECTATOR) {
		drop += speed * pm_spectatorfriction * pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate(vec3_t wishdir, float wishspeed, float accel) {
#if 1
	// q2 style
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel * pml.frametime * wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}

	for (i = 0; i < 3; i++) {
		pm->ps->velocity[i] += accelspeed * wishdir[i];
	}
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t wishVelocity;
	vec3_t pushDir;
	float pushLen;
	float canPush;

	VectorScale(wishdir, wishspeed, wishVelocity);
	VectorSubtract(wishVelocity, pm->ps->velocity, pushDir);
	pushLen = VectorNormalize(pushDir);

	canPush = accel * pml.frametime * wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	VectorMA(pm->ps->velocity, canPush, pushDir, pm->ps->velocity);
#endif
}

/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale(usercmd_t *cmd) {
	int max;
	float total;
	float scale;

	max = abs(cmd->forwardmove);
	if (abs(cmd->rightmove) > max) {
		max = abs(cmd->rightmove);
	}
	if (abs(cmd->upmove) > max) {
		max = abs(cmd->upmove);
	}
	if (!max) {
		return 0;
	}

	total = sqrt(cmd->forwardmove * cmd->forwardmove + cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove);
	scale = (float)pm->ps->speed * max / (127.0 * total);

	return scale;
}

/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void PM_SetMovementDir(void) {
	if (pm->cmd.forwardmove || pm->cmd.rightmove) {
		if (pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0) {
			pm->ps->movementDir = 0;
		} else if (pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0) {
			pm->ps->movementDir = 1;
		} else if (pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0) {
			pm->ps->movementDir = 2;
		} else if (pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0) {
			pm->ps->movementDir = 3;
		} else if (pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0) {
			pm->ps->movementDir = 4;
		} else if (pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0) {
			pm->ps->movementDir = 5;
		} else if (pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0) {
			pm->ps->movementDir = 6;
		} else if (pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if (pm->ps->movementDir == 2) {
			pm->ps->movementDir = 1;
		} else if (pm->ps->movementDir == 6) {
			pm->ps->movementDir = 7;
		}
	}
}

/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump(void) {
	if (pm->ps->pm_flags & PMF_RESPAWNED) {
		return qfalse; // don't allow jump until all buttons are up
	}

	if (pm->cmd.upmove < 10) {
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if (pm->ps->pm_flags & PMF_JUMP_HELD) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane = qfalse; // jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	if (pm->ps->powerups[PW_JUMPER] && pm->ps->weapon != WP_SPRAYPISTOL) {
		// FIXME: Magical constant 2.5
		pm->ps->velocity[2] = (JUMP_VELOCITY * 2.5);
	} else {
		pm->ps->velocity[2] = JUMP_VELOCITY;
	}

	PM_AddEvent(EV_JUMP);

	if (pm->cmd.forwardmove >= 0) {
		PM_ForceLegsAnim(LEGS_JUMP);
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
		PM_ForceLegsAnim(LEGS_JUMPB);
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean PM_CheckWaterJump(void) {
	vec3_t spot;
	int cont;
	vec3_t flatforward;

	if (pm->ps->pm_time) {
		return qfalse;
	}

	// check for water jump
	if (pm->waterlevel != 2) {
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	VectorMA(pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if (!(cont & CONTENTS_SOLID)) {
		return qfalse;
	}

	spot[2] += 16;
	cont = pm->pointcontents(spot, pm->ps->clientNum);
	if (cont) {
		return qfalse;
	}

	// jump out of water
	VectorScale(pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================

/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove(void) {
	// waterjump has no control, but falls

	PM_StepSlideMove(qtrue);

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove(void) {
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;
	float vel;

	if (PM_CheckWaterJump()) {
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if (pm->ps->velocity[2] > -300) {
			if ( pm->watertype == CONTENTS_WATER ) {
				pm->ps->velocity[2] = 100;
			} else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity[2] = 80;
			} else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction();

	scale = PM_CmdScale(&pm->cmd);
	//
	// user intentions
	//
	if (!scale) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60; // sink towards bottom
	} else {
		for (i = 0; i < 3; i++)
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pm->ps->speed * pm_swimScale) {
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	if (pml.groundPlane && DotProduct(pm->ps->velocity, pml.groundTrace.plane.normal) < 0) {
		vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove(qfalse);
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove(void) {
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;

	// normal slowdown
	PM_Friction();

	scale = PM_CmdScale(&pm->cmd);
	//
	// user intentions
	//
	if (!scale) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	} else {
		for (i = 0; i < 3; i++) {
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate(wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove(qfalse);
}

/*
===================
PM_AirMove

===================
*/
static void PM_AirMove(void) {
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	usercmd_t cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale(&cmd);

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	for (i = 0; i < 2; i++) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	PM_Accelerate(wishdir, wishspeed, pm_airaccelerate);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if (pml.groundPlane) {
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);
	}

#if 0
	//ZOID:  If we are on the grapple, try stair-stepping
	//this allows a player to use the grapple to pull himself
	//over a ledge
	if (pm->ps->pm_flags & PMF_GRAPPLE_PULL)
		PM_StepSlideMove ( qtrue );
	else
		PM_SlideMove ( qtrue );
#endif

	PM_StepSlideMove(qtrue);
}

/*
===================
PM_GrappleMove

===================
*/
static void PM_GrappleMove(void) {
	vec3_t vel, v;
	float vlen;

	VectorScale(pml.forward, -16, v);
	VectorAdd(pm->ps->grapplePoint, v, v);
	VectorSubtract(v, pm->ps->origin, vel);
	vlen = VectorLength(vel);
	VectorNormalize(vel);

	if (vlen <= 100)
		VectorScale(vel, 10 * vlen, vel);
	else
		VectorScale(vel, 800, vel);

	VectorCopy(vel, pm->ps->velocity);

	pml.groundPlane = qfalse;
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove(void) {
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	usercmd_t cmd;
	float accelerate;
	float vel;

	if (pm->waterlevel > 2 && DotProduct(pml.forward, pml.groundTrace.plane.normal) > 0) {
		// begin swimming
		PM_WaterMove();
		return;
	}

	if (PM_CheckJump()) {
		// jumped away
		if (pm->waterlevel > 1) {
			PM_WaterMove();
		} else {
			PM_AirMove();
		}
		return;
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale(&cmd);

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity(pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP);
	PM_ClipVelocity(pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP);
	//
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	for (i = 0; i < 3; i++) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	// when going up or down slopes the wish velocity should Not be zero
	//	wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if (pm->ps->pm_flags & PMF_DUCKED) {
		if (wishspeed > pm->ps->speed * pm_duckScale) {
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if (pm->waterlevel) {
		float waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - (1.0 - pm_swimScale) * waterScale;
		if (wishspeed > pm->ps->speed * waterScale) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ((pml.groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK) {
		accelerate = pm_airaccelerate;
	} else {
		accelerate = pm_accelerate;
	}

	PM_Accelerate(wishdir, wishspeed, accelerate);

	// Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	// Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ((pml.groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK) {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	} else {
		// don't reset the z velocity for slopes
		//		pm->ps->velocity[2] = 0;
	}

	vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
		return;
	}

	PM_StepSlideMove(qfalse);

	// Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));
}

/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove(void) {
	float forward;

	if (!pml.walking) {
		return;
	}

	// extra friction

	forward = VectorLength(pm->ps->velocity);
	forward -= 20;
	if (forward <= 0) {
		VectorClear(pm->ps->velocity);
	} else {
		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, forward, pm->ps->velocity);
	}
}

/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove(void) {
	float speed, drop, friction, control, newspeed;
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength(pm->ps->velocity);
	if (speed < 1) {
		VectorCopy(vec3_origin, pm->ps->velocity);
	} else {
		drop = 0;

		friction = pm_friction * 1.5; // extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale(pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale(&pm->cmd);

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for (i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate(wishdir, wishspeed, pm_accelerate);

	// move
	VectorMA(pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number appropriate for the groundsurface
================
*/
static int PM_FootstepForSurface(void) {
	if (pml.groundTrace.surfaceFlags & SURF_NOSTEPS) {
		return 0;
	}
	if (pml.groundTrace.surfaceFlags & SURF_METALSTEPS) {
		return EV_FOOTSTEP_METAL;
	}

	if (pml.groundTrace.surfaceFlags & SURF_CARPETSTEPS)
		return EV_FOOTSTEP_CARPET;
	if (pml.groundTrace.surfaceFlags & SURF_LATTICESTEPS)
		return EV_FOOTSTEP_LATTICE;
	if (pml.groundTrace.surfaceFlags & SURF_SANDSTEPS)
		return EV_FOOTSTEP_SAND;
	if (pml.groundTrace.surfaceFlags & SURF_SOFTSTEPS)
		return EV_FOOTSTEP_SOFT;
	if (pml.groundTrace.surfaceFlags & SURF_WOODSTEPS)
		return EV_FOOTSTEP_WOOD;
	if (pml.groundTrace.surfaceFlags & SURF_SNOWSTEPS)
		return EV_FOOTSTEP_SNOW;

	return EV_FOOTSTEP;
}

/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand(void) {
	float delta;
	float dist;
	float vel, acc;
	float t;
	float a, b, c, den;

	// decide which landing animation to use
	if (pm->ps->pm_flags & PMF_BACKWARDS_JUMP) {
		PM_ForceLegsAnim(LEGS_LANDB);
	} else {
		PM_ForceLegsAnim(LEGS_LAND);
	}

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;
	if (den < 0) {
		return;
	}
	t = (-b - sqrt(den)) / (2 * a);

	delta = vel + t * acc;
	delta = delta * delta * 0.0001;

	// ducking while falling doubles damage
	if (pm->ps->pm_flags & PMF_DUCKED) {
		delta *= 2;
	}

	// never take falling damage if completely underwater
	if (pm->waterlevel == 3) {
		return;
	}

	// reduce falling damage if there is standing water
	if (pm->waterlevel == 2) {
		delta *= 0.25;
	}
	if (pm->waterlevel == 1) {
		delta *= 0.5;
	}

	if (delta < 1) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if (!(pml.groundTrace.surfaceFlags & SURF_NODAMAGE)) {

		if (pm->ps->powerups[PW_JUMPER] && delta > 7)
			PM_AddEvent(EV_FALL_SHORT);
		else if (delta > 60) {
			PM_AddEvent(EV_FALL_FAR);

			PM_ForceLegsAnim(LEGS_HEAVYLAND);
		} else if (delta > 40) {
			// this is a pain grunt, so don't play it if dead
			if (pm->ps->stats[STAT_HEALTH] > 0) {
				PM_AddEvent(EV_FALL_MEDIUM);
			}
		} else if (delta > 7) {
			PM_AddEvent(EV_FALL_SHORT);
		} else {
			PM_AddEvent(PM_FootstepForSurface());
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/*
=============
PM_CheckStuck
=============
*/
/*
void PM_CheckStuck(void) {
	trace_t trace;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
	if (trace.allsolid) {
		//int shit = qtrue;
	}
}
*/

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid(trace_t *trace) {
	int i, j, k;
	vec3_t point;

	if (pm->debugLevel) {
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			for (k = -1; k <= 1; k++) {
				VectorCopy(pm->ps->origin, point);
				point[0] += (float)i;
				point[1] += (float)j;
				point[2] += (float)k;
				pm->trace(trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if (!trace->allsolid) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace(trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}

/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed(void) {
	trace_t trace;
	vec3_t point;

	if (pm->ps->groundEntityNum != ENTITYNUM_NONE) {
		// we just transitioned into freefall
		if (pm->debugLevel) {
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy(pm->ps->origin, point);
		point[2] -= 64;

		pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if (trace.fraction == 1.0) {
			if (pm->cmd.forwardmove >= 0) {
				PM_ForceLegsAnim(LEGS_JUMP);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				PM_ForceLegsAnim(LEGS_JUMPB);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}

/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace(void) {
	vec3_t point;
	trace_t trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if (trace.allsolid) {
		if (!PM_CorrectAllSolid(&trace))
			return;
	}

	// if the trace didn't hit anything, we are in free fall
	if (trace.fraction == 1.0) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if (pm->ps->velocity[2] > 0 && DotProduct(pm->ps->velocity, trace.plane.normal) > 10) {
		if (pm->debugLevel) {
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation
		if (pm->cmd.forwardmove >= 0) {
			PM_ForceLegsAnim(LEGS_JUMP);
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else {
			PM_ForceLegsAnim(LEGS_JUMPB);
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if (trace.plane.normal[2] < MIN_WALK_NORMAL) {
		if (pm->debugLevel) {
			Com_Printf("%i:steep\n", c_pmove);
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	if (pm->ps->pm_flags & PMF_TOUCHSLICKENT) {
		pml.groundTrace.surfaceFlags |= SURF_SLICK;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE) {
		// just hit the ground
		if (pm->debugLevel) {
			Com_Printf("%i:Land\n", c_pmove);
		}

		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if (pml.previous_velocity[2] < -200) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
	//	pm->ps->velocity[2] = 0;

	PM_AddTouchEnt(trace.entityNum);
}

/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel(void) {
	vec3_t point;
	int cont;
	int sample1;
	int sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	cont = pm->pointcontents(point, pm->ps->clientNum);

	if (cont & MASK_WATER) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents(point, pm->ps->clientNum);
		if (cont & MASK_WATER) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents(point, pm->ps->clientNum);
			if (cont & MASK_WATER) {
				pm->waterlevel = 3;
			}
		}
	}
}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck(void) {
	trace_t trace;

	if (pm->ps->powerups[PW_CLIMBER]) {
		pm->ps->viewheight = 26; // default

		VectorSet(pm->mins, -30, -30, -30);
		VectorSet(pm->maxs, 30, 30, 30);

		return;
	}

	pm->mins[0] = -15;
	pm->mins[1] = -15;

	pm->maxs[0] = 15;
	pm->maxs[1] = 15;

	pm->mins[2] = MINS_Z;

	if (pm->ps->pm_type == PM_DEAD) {
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if (pm->cmd.upmove < 0) { // duck
		pm->ps->pm_flags |= PMF_DUCKED;
	} else { // stand up if possible
		if (pm->ps->pm_flags & PMF_DUCKED) {
			// try to stand up
			pm->maxs[2] = 32;
			pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
			if (!trace.allsolid)
				pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED) {
		pm->maxs[2] = 16;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	} else {
		pm->maxs[2] = 32;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
}

//===================================================================

/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps(void) {
	float bobmove;
	int old;
	qboolean footstep;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt(pm->ps->velocity[0] * pm->ps->velocity[0] + pm->ps->velocity[1] * pm->ps->velocity[1]);

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE) {

		// airborne leaves position in cycle intact, but doesn't advance
		if (pm->waterlevel > 1) {
			PM_ContinueLegsAnim(LEGS_SWIM);
		}
		return;
	}

	// if not trying to move
	if (!pm->cmd.forwardmove && !pm->cmd.rightmove) {
		if (pm->xyspeed < 5) {
			pm->ps->bobCycle = 0; // start at beginning of cycle again
			if (pm->ps->pm_flags & PMF_DUCKED) {
				PM_ContinueLegsAnim(LEGS_IDLECR);
			} else {
				PM_ContinueLegsAnim(LEGS_IDLE);
			}
		}
		return;
	}

	footstep = qfalse;

	if (pm->ps->pm_flags & PMF_DUCKED) {
		bobmove = 0.5; // ducked characters bob much faster
		if (pm->ps->pm_flags & PMF_BACKWARDS_RUN) {
			PM_ContinueLegsAnim(LEGS_BACKCR);
		} else {
			PM_ContinueLegsAnim(LEGS_WALKCR);
		}
		// ducked characters never play footsteps
		/*
		} else 	if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
				bobmove = 0.4;	// faster speeds bob faster
				footstep = qtrue;
			} else {
				bobmove = 0.3;
			}
			PM_ContinueLegsAnim( LEGS_BACK );
		*/
	} else {
		if (!(pm->cmd.buttons & BUTTON_WALKING)) {
			bobmove = 0.4f; // faster speeds bob faster
			if (pm->ps->pm_flags & PMF_BACKWARDS_RUN) {
				PM_ContinueLegsAnim(LEGS_BACK);
			} else {
				PM_ContinueLegsAnim(LEGS_RUN);
			}
			footstep = qtrue;
		} else {
			bobmove = 0.3f; // walking bobs slow
			if (pm->ps->pm_flags & PMF_BACKWARDS_RUN) {
				PM_ContinueLegsAnim(LEGS_BACKWALK);
			} else {
				PM_ContinueLegsAnim(LEGS_WALK);
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)(old + bobmove * pml.msec) & 255;

	// if we just crossed a cycle boundary, play an appropriate footstep event
	if (((old + 64) ^ (pm->ps->bobCycle + 64)) & 128) {
		if (pm->waterlevel == 0) {
			// on ground will only play sounds if running
			if (footstep && !pm->noFootsteps) {
				PM_AddEvent(PM_FootstepForSurface());
			}
		} else if (pm->waterlevel == 1) {
			// splashing
			PM_AddEvent(EV_FOOTSPLASH);
		} else if (pm->waterlevel == 2) {
			// wading / swimming at surface
			PM_AddEvent(EV_SWIM);
		} else if (pm->waterlevel == 3) {
			// no sound when completely underwater
		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents(void) { // FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent(EV_WATER_TOUCH);
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent(EV_WATER_LEAVE);
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent(EV_WATER_UNDER);
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent(EV_WATER_CLEAR);
	}
}

/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange(int weapon) {
	if (weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS) {
		return;
	}

	if (!(pm->ps->stats[STAT_WEAPONS] & (1 << weapon))) {
		return;
	}

	if (pm->ps->weaponstate == WEAPON_DROPPING) {
		return;
	}

	PM_AddEvent(EV_CHANGE_WEAPON);
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += ADDTIME_WEAPONCHANGE_BEGIN;
	PM_StartTorsoAnim(TORSO_DROP);
}

/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange(void) {
	int weapon;

	weapon = pm->cmd.weapon;
	if (weapon < WP_NONE || weapon >= WP_NUM_WEAPONS) {
		weapon = WP_NONE;
	}

	if (!(pm->ps->stats[STAT_WEAPONS] & (1 << weapon))) {
		weapon = WP_NONE;
	}

	if (pm->ps->powerups[PW_BERSERKER] && !((weapon == WP_PUNCHY) || (weapon == WP_SPRAYPISTOL))) {
		weapon = WP_PUNCHY;
	}

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += ADDTIME_WEAPONCHANGE_FINISH;
	PM_StartTorsoAnim(TORSO_RAISE);
}

/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation(void) {
	if (pm->ps->weaponstate == WEAPON_READY) {
		if (pm->ps->weapon == WP_PUNCHY) {
			PM_ContinueTorsoAnim(TORSO_STAND2);
		} else if (pm->ps->weapon == WP_SPRAYPISTOL) {
			PM_ContinueTorsoAnim(TORSO_SPRAYSTAND);
		} else {
			PM_ContinueTorsoAnim(TORSO_STAND);
		}
		return;
	}
}

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon(void) {
	int addTime;

	// don't allow attack until all buttons are up
	if (pm->ps->pm_flags & PMF_RESPAWNED) {
		return;
	}

	// ignore if spectator
	if (pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	// check for dead player
	if (pm->ps->stats[STAT_HEALTH] <= 0) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	pm->ps->eFlags &= ~EF_FLOATER;

	// check for item using
	if (pm->cmd.buttons & BUTTON_USE_HOLDABLE) {
		// No items in sprayroom
		if ((pm->ps->stats[STAT_WEAPONS] & (1 << WP_SPRAYPISTOL)) && (pm->ps->stats[STAT_SPRAYROOMSECS] > 0)) {
			return;
		}

		if (!(pm->ps->pm_flags & PMF_USE_ITEM_HELD)) {
			if (bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_MEDKIT &&
				pm->ps->stats[STAT_HEALTH] >= (pm->ps->stats[STAT_MAX_HEALTH] + 25)) {
				// don't use medkit if at max health
			} else if (bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_FLOATER) {
				PM_AddEvent(EV_USE_ITEM0 + HI_FLOATER);
				if ((pm->ps->legsAnim & ~ANIM_TOGGLEBIT) != LEGS_FLYING ||
					((pm->ps->legsAnim & ~ANIM_TOGGLEBIT) == LEGS_FLYING && pm->ps->legsTimer <= 0))
					PM_ForceLegsAnim(LEGS_FLYING);

				pm->ps->eFlags |= EF_FLOATER;

				pm->ps->stats[STAT_HOLDABLEVAR] -= FLOATER_USAGE_PER_MS * pml.msec;
				if (pm->ps->stats[STAT_HOLDABLEVAR] <= 0) {
					pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
					pm->ps->stats[STAT_HOLDABLEVAR] = 0;
					pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
				}
				goto shootafterrspecialHI;
			} else if (bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_KILLERDUCKS) {
				if (pm->ps->weaponstate != WEAPON_READY)
					goto shootafterrspecialHI;

				PM_AddEvent(EV_USE_ITEM0 + HI_KILLERDUCKS);
				pm->ps->stats[STAT_HOLDABLEVAR]--;

				if (pm->ps->stats[STAT_HOLDABLEVAR] <= 0) {
					pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
					pm->ps->stats[STAT_HOLDABLEVAR] = 0;
					pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
				}
				pm->ps->weaponTime = 500;
				pm->ps->weaponstate = WEAPON_FIRING;
				goto shootafterrspecialHI;
			} else if (bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_BOOMIES) {
				if (pm->ps->weaponstate != WEAPON_READY)
					goto shootafterrspecialHI;

				PM_AddEvent(EV_USE_ITEM0 + HI_BOOMIES);

				pm->ps->weaponTime = 500;
				pm->ps->weaponstate = WEAPON_FIRING;
				goto shootafterrspecialHI;
			} else if (bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_BAMBAM) {
				if (pm->ps->weaponstate != WEAPON_READY)
					goto shootafterrspecialHI;

				PM_AddEvent(EV_USE_ITEM0 + HI_BAMBAM);
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;

				pm->ps->weaponTime = 2000;
				pm->ps->weaponstate = WEAPON_FIRING;
			} else {
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent(EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag);
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
			return;
		}
	} else {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}

shootafterrspecialHI:

	// HERBY: Imperius charging
	// FIXME: A whole new weapons state for one weapon? Sick
	if (pm->ps->weaponstate == WEAPON_CHARGING) {
		int seed;
		// advance time
		pm->ps->weaponTime += pml.msec;
		if (pm->ps->weaponTime > CHARGETIME_IMPERIUS) {
			pm->ps->eFlags |= EF_CHARGED;
		}

		// shaking
		seed = pm->ps->pmove_framecount;
		pm->ps->delta_angles[PITCH] += pml.msec * pm->ps->weaponTime * 0.003 * Q_crandom(&seed);
		pm->ps->delta_angles[YAW] += pml.msec * pm->ps->weaponTime * 0.003 * Q_crandom(&seed);

		// button release or overcharge
		if (!(pm->cmd.buttons & BUTTON_ATTACK)) {
			pm->ps->weaponstate = WEAPON_FIRING;
			pm->ps->weaponTime = ADDTIME_IMPERIUS;
			if (pm->ps->eFlags & EF_CHARGED)
				goto fire;
		} else if (pm->ps->weaponTime > OVERCHARGETIME_IMPERIUS) {
			PM_AddEvent(EV_IMPERIUS_EXPLODE);
			pm->ps->eFlags &= ~EF_CHARGED;
			pm->ps->weaponstate = WEAPON_FIRING;
			pm->ps->weaponTime = ADDTIME_IMPERIUS;
			--(pm->ps->ammo[pm->ps->weapon]);
		}
		return;
	}

	// make weapon function
	if (pm->ps->weaponTime > 0) {
		pm->ps->weaponTime -= pml.msec;
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if (pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING) {
		if (pm->ps->weapon != pm->cmd.weapon) {
			PM_BeginWeaponChange(pm->cmd.weapon);
		}
	}

	if (pm->ps->weapon == WP_NIPPER && pm->ps->weaponstate == WEAPON_READY && pm->ps->weaponTime > 0 &&
		!(pm->cmd.buttons & BUTTON_ATTACK)) {
		pm->ps->weaponTime = 0;
	}

	if (pm->ps->weaponTime > 0) {
		return;
	}

	// change weapon if time
	if (pm->ps->weaponstate == WEAPON_DROPPING) {
		PM_FinishWeaponChange();
		return;
	}

	if (pm->ps->weaponstate == WEAPON_RAISING) {
		pm->ps->weaponstate = WEAPON_READY;
		if (pm->ps->weapon == WP_PUNCHY) {
			PM_StartTorsoAnim(TORSO_STAND2);
		} else if (pm->ps->weapon == WP_SPRAYPISTOL) {
			PM_StartTorsoAnim(TORSO_SPRAYSTAND);
		} else {
			PM_StartTorsoAnim(TORSO_STAND);
		}
		return;
	}

	// check for fire
	if (!(pm->cmd.buttons & BUTTON_ATTACK)) {
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	if ((pm->ps->stats[STAT_WEAPONS] & (1 << WP_SPRAYPISTOL)) &&
		((pm->ps->stats[STAT_SPRAYROOMSECS] > 0) && (pm->ps->weapon != WP_SPRAYPISTOL))) {
		return;
	}

	if (pm->ps->weapon == WP_SPRAYPISTOL && pm->ps->weaponstate == WEAPON_READY) {
		pm->ps->weaponTime = 400;
		pm->ps->weaponstate = WEAPON_FIRING;
		return;
	}

	if (pm->ps->weapon == WP_NIPPER && pm->ps->weaponstate != WEAPON_READY) {
		// noch was ein binden damit die nipper auch automatisch schießt
		pm->ps->weaponTime += ADDTIME_AUTOFIRE_NIPPER;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// start the animation even if out of ammo
	if (pm->ps->weapon == WP_PUNCHY) {
		// the guantlet only "fires" when it actually hits something
		if (!pm->gauntletHit) {
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
		PM_StartTorsoAnim(TORSO_ATTACK2);
	} else if (pm->ps->weapon == WP_SPRAYPISTOL) {
		PM_StartTorsoAnim(TORSO_SPRAYATTACK);
	} else {
		PM_StartTorsoAnim(TORSO_ATTACK);
	}

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	if (!pm->ps->ammo[pm->ps->weapon]) {
		PM_AddEvent(EV_NOAMMO);
		pm->ps->weaponTime += ADDTIME_WEAPON_NOAMMO;
		return;
	}

	// HERBY: Imperius charging
	if (pm->ps->weapon == WP_IMPERIUS) {
		pm->ps->weaponstate = WEAPON_CHARGING;
		pm->ps->weaponTime = 0;
		return;
	}

fire:
	// HERBY: Reset flag
	pm->ps->eFlags &= ~EF_CHARGED;

	// take an ammo away if not infinite
	if (pm->ps->ammo[pm->ps->weapon] != -1) {
		pm->ps->ammo[pm->ps->weapon]--;
	}

	// fire weapon
	PM_AddEvent(EV_FIRE_WEAPON);

	switch (pm->ps->weapon) {
	default:
	case WP_PUNCHY:
		addTime = ADDTIME_PUNCHY;
		break;
	case WP_BOASTER:
		addTime = ADDTIME_BOASTER;
		break;
	case WP_PUMPER:
		addTime = ADDTIME_PUMPER;
		break;
	case WP_NIPPER:
		addTime = ADDTIME_NIPPER;
		break;
	case WP_BALLOONY:
		addTime = ADDTIME_BALLOONY;
		break;
	case WP_BETTY:
		addTime = ADDTIME_BETTY;
		break;
	case WP_BUBBLEG:
		addTime = ADDTIME_BUBBLEG;
		break;
	case WP_SPLASHER:
		addTime = ADDTIME_SPLASHER;
		break;
	case WP_IMPERIUS:
		addTime = 200;
		break;

	case WP_KMA97: // "Kiss My Ass 97"
		addTime = ADDTIME_KMA97;
		break;

	case WP_GRAPPLING_HOOK:
		addTime = 400;
		break;

	case WP_SPRAYPISTOL:
		addTime = 800;
		break;
	}

	pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/

static void PM_Animate(void) {
	if (pm->cmd.buttons & BUTTON_GESTURE) {
		if (pm->ps->torsoTimer == 0) {
			PM_StartTorsoAnim(TORSO_GESTURE);
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent(EV_TAUNT);
		}
	}
}

/*
================
PM_DropTimers
================
*/
static void PM_DropTimers(void) {
	// drop misc timing counter
	if (pm->ps->pm_time) {
		if (pml.msec >= pm->ps->pm_time) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if (pm->ps->legsTimer > 0) {
		pm->ps->legsTimer -= pml.msec;
		if (pm->ps->legsTimer < 0) {
			pm->ps->legsTimer = 0;
		}
	}

	if (pm->ps->torsoTimer > 0) {
		pm->ps->torsoTimer -= pml.msec;
		if (pm->ps->torsoTimer < 0) {
			pm->ps->torsoTimer = 0;
		}
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void PM_UpdateViewAngles(playerState_t *ps, const usercmd_t *cmd) {
	short temp;
	int i;

	if (ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) {
		return; // no view changes at all
	}

	if (ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0) {
		return; // no view changes at all
	}

	// circularly clamp the angles with deltas
	for (i = 0; i < 3; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if (i == PITCH) {
			// don't let the player look up or down more than 90 degrees
			if (temp > 16000) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if (temp < -16000) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}

/*
================
PmoveSingle

================
*/
void trap_SnapVector(float *v);

void PmoveSingle(pmove_t *pmove) {
	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if (pm->ps->stats[STAT_HEALTH] <= 0) {
		pm->tracemask &= ~CONTENTS_BODY; // corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if (abs(pm->cmd.forwardmove) > 64 || abs(pm->cmd.rightmove) > 64) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if (pm->cmd.buttons & BUTTON_TALK) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// set the firing flag for continuous beam weapons
	// HERBY: Imperius charging
	if (!(pm->ps->pm_flags & PMF_RESPAWNED) && (pm->ps->pm_type != PM_INTERMISSION) && (PM_FREEZE != pm->ps->pm_type) &&
		(pm->cmd.buttons & BUTTON_ATTACK) && pm->ps->ammo[pm->ps->weapon] &&
		(pm->ps->weapon != WP_IMPERIUS || pm->ps->weaponstate == WEAPON_CHARGING) &&
		((pm->gametype != GT_LPS) || (pm->ps->stats[STAT_LIVESLEFT] > 0)) && !(pm->cmd.buttons & BUTTON_TALK)) {
		pm->ps->eFlags |= EF_FIRING;
	} else {
		pm->ps->eFlags &= ~EF_FIRING;
	}

	// clear the respawned flag if attack and use are cleared
	if (pm->ps->stats[STAT_HEALTH] > 0 && !(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE))) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if (pmove->cmd.buttons & BUTTON_TALK) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset(&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if (pml.msec < 1) {
		pml.msec = 1;
	} else if (pml.msec > 200) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy(pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy(pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	PM_UpdateViewAngles(pm->ps, &pm->cmd);

	AngleVectors(pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if (pm->cmd.upmove < 10) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if (pm->cmd.forwardmove < 0) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if (pm->cmd.forwardmove > 0 || (pm->cmd.forwardmove == 0 && pm->cmd.rightmove)) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if (pm->ps->pm_type >= PM_DEAD) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if (pm->ps->pm_type == PM_SPECTATOR) {
		PM_CheckDuck();
		PM_FlyMove();
		PM_DropTimers();
		return;
	}

	if (pm->ps->pm_type == PM_NOCLIP) {
		PM_NoclipMove();
		PM_DropTimers();
		return;
	}

	if (pm->ps->pm_type == PM_FREEZE) {
		return; // no movement at all
	}

	if (pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION) {
		return; // no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck();

	// set groundentity
	PM_GroundTrace();

	if (pm->ps->pm_type == PM_DEAD) {
		PM_DeadMove();
	}

	if (pm->ps->pm_type == PM_FROZEN) {
		PM_DeadMove();
	}

	PM_DropTimers();

	if (pm->ps->pm_flags & PMF_GRAPPLE_PULL) {
		PM_GrappleMove();
		// We can wiggle a bit
		PM_AirMove();
	} else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
		PM_WaterJumpMove();
	} else if (pm->waterlevel > 1) {
		// swimming
		PM_WaterMove();
	} else if (pml.walking) {
		// walking on ground
		PM_WalkMove();
	} else {
		// airborne
		PM_AirMove();
	}

	PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// freezetag
	if (pm->ps->pm_type == PM_FROZEN)
		return;

	// weapons
	PM_Weapon();

	// torso animation
	PM_TorsoAnimation();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap some parts of playerstate to save network bandwidth
	trap_SnapVector(pm->ps->velocity);
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove(pmove_t *pmove) {
	int finalTime;

	finalTime = pmove->cmd.serverTime;

	if (finalTime < pmove->ps->commandTime) {
		return; // should not happen
	}

	if (finalTime > pmove->ps->commandTime + 1000) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount + 1) & ((1 << PS_PMOVEFRAMECOUNTBITS) - 1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while (pmove->ps->commandTime != finalTime) {
		int msec;

		msec = finalTime - pmove->ps->commandTime;

		if (pmove->pmove_fixed) {
			if (msec > pmove->pmove_msec) {
				msec = pmove->pmove_msec;
			}
		} else {
			if (msec > 66) {
				msec = 66;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle(pmove);

		if (pmove->ps->pm_flags & PMF_JUMP_HELD) {
			pmove->cmd.upmove = 20;
		}
	}

	// PM_CheckStuck();
}
