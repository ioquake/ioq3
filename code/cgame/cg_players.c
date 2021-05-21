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
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

char *cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {"*death1",	 "*death2",	  "*death3",	"*jump1",	 "*pain25_1",
												"*pain50_1", "*pain75_1", "*pain100_1", "*falling1", "*gasp",
												"*drown",	 "*fall1",	  "*taunt",		"*hehe1",	 "*hehe2"};

/*
================
CG_CustomSound

================
*/
sfxHandle_t CG_CustomSound(int clientNum, const char *soundName) {
	clientInfo_t *ci;
	int i;
	char soundBaseName[MAX_QPATH];

	if (soundName[0] != '*') {
		return trap_S_RegisterSound(soundName, qfalse);
	}

	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[clientNum];

	// Backwards compability; "*falling1.wav" etc. should still work
	COM_StripExtension(soundName, soundBaseName, sizeof(soundBaseName));

	for (i = 0; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i]; i++) {
		if (!strcmp(soundBaseName, cg_customSoundNames[i])) {
			return ci->sounds[i];
		}
	}

	CG_Error("Unknown custom sound: %s", soundName);
	return 0;
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation counts and rates
models/players/visor/animation.cfg, etc
======================
*/
static qboolean CG_ParseAnimationFile(const char *filename, clientInfo_t *ci) {
	const char *text_p, *prev;
	int len;
	int i;
	const char *token;
	float fps;
	int skip;
	char text[20000];
	fileHandle_t f;
	animation_t *animations;

	animations = ci->animations;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len <= 0) {
		return qfalse;
	}
	if (len >= sizeof(text) - 1) {
		CG_Printf("File %s too long\n", filename);
		return qfalse;
	}
	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;
	skip = 0; // quite the compiler warning

	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear(ci->headOffset);
	ci->headScale = 1.0f;
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;

	// read optional parameters
	while (1) {
		prev = text_p; // so we can unget
		token = COM_Parse(&text_p);
		if (!token) {
			break;
		}
		if (!Q_stricmp(token, "footsteps")) {
			token = COM_Parse(&text_p);
			if (!token) {
				break;
			}
			if (!Q_stricmp(token, "default") || !Q_stricmp(token, "normal")) {
				ci->footsteps = FOOTSTEP_NORMAL;
			} else if (!Q_stricmp(token, "boot")) {
				ci->footsteps = FOOTSTEP_BOOT;
			} else {
				CG_Printf("Bad footsteps param in %s: %s\n", filename, token);
			}
			continue;
		} else if (!Q_stricmp(token, "headoffset")) {
			for (i = 0; i < 3; i++) {
				token = COM_Parse(&text_p);
				if (!token) {
					break;
				}
				ci->headOffset[i] = atof(token);
			}
			continue;
		} else if (!Q_stricmp(token, "headscale")) {
			token = COM_Parse(&text_p);
			if (!token) {
				break;
			}
			ci->headScale = atof(token);
			continue;
		} else if (!Q_stricmp(token, "menumodeloffset")) {
			for (i = 0; i < 3; i++) {
				token = COM_Parse(&text_p);
				if (!token) {
					break;
				}
			}
			continue;
		} else if (!Q_stricmp(token, "menumodelscale")) {
			token = COM_Parse(&text_p);
			if (!token) {
				break;
			}
			continue;
		} else if (!Q_stricmp(token, "sex")) {
			token = COM_Parse(&text_p);
			if (!token) {
				break;
			}
			if (token[0] == 'f' || token[0] == 'F') {
				ci->gender = GENDER_FEMALE;
			} else if (token[0] == 'n' || token[0] == 'N') {
				ci->gender = GENDER_NEUTER;
			} else {
				ci->gender = GENDER_MALE;
			}
			continue;
		} else if (!Q_stricmp(token, "fixedlegs")) {
			ci->fixedlegs = qtrue;
			continue;
		} else if (!Q_stricmp(token, "fixedtorso")) {
			ci->fixedtorso = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if (token[0] >= '0' && token[0] <= '9') {
			text_p = prev; // unget the token
			break;
		}
		Com_Printf("unknown token '%s' is %s\n", token, filename);
	}

	// read information for each frame
	for (i = 0; i < MAX_ANIMATIONS; i++) {

		token = COM_Parse(&text_p);
		if (!*token) {
			if (i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE) {
				animations[i].firstFrame = animations[TORSO_GESTURE].firstFrame;
				animations[i].frameLerp = animations[TORSO_GESTURE].frameLerp;
				animations[i].initialLerp = animations[TORSO_GESTURE].initialLerp;
				animations[i].loopFrames = animations[TORSO_GESTURE].loopFrames;
				animations[i].numFrames = animations[TORSO_GESTURE].numFrames;
				animations[i].reversed = qfalse;
				animations[i].flipflop = qfalse;
				continue;
			} else if (i == TORSO_SPRAYATTACK) {
				memcpy(&animations[TORSO_SPRAYATTACK], &animations[TORSO_ATTACK],
					   sizeof(animations[TORSO_SPRAYATTACK]));
				continue;
			} else if (i == TORSO_SPRAYSTAND) {
				memcpy(&animations[TORSO_SPRAYSTAND], &animations[TORSO_STAND], sizeof(animations[TORSO_SPRAYSTAND]));
				continue;
			} else if (i == LEGS_FLYING) {
				memcpy(&animations[LEGS_FLYING], &animations[LEGS_IDLE], sizeof(animations[LEGS_FLYING]));
				continue;
			} else if (i == LEGS_HEAVYLAND) {
				memcpy(&animations[LEGS_HEAVYLAND], &animations[LEGS_JUMP], sizeof(animations[LEGS_HEAVYLAND]));
				continue;
			}

			break;
		}
		animations[i].firstFrame = atoi(token);
		// leg only frames are adjusted to not count the upper body only frames
		if (i == LEGS_WALKCR) {
			skip = animations[LEGS_WALKCR].firstFrame - animations[TORSO_GESTURE].firstFrame;
		}
		if (i >= LEGS_WALKCR && i < TORSO_GETFLAG) {
			animations[i].firstFrame -= skip;
		}

		token = COM_Parse(&text_p);
		if (!*token) {
			break;
		}
		animations[i].numFrames = atoi(token);

		animations[i].reversed = qfalse;
		animations[i].flipflop = qfalse;
		// if numFrames is negative the animation is reversed
		if (animations[i].numFrames < 0) {
			animations[i].numFrames = -animations[i].numFrames;
			animations[i].reversed = qtrue;
		}

		token = COM_Parse(&text_p);
		if (!*token) {
			break;
		}
		animations[i].loopFrames = atoi(token);

		token = COM_Parse(&text_p);
		if (!*token) {
			break;
		}
		fps = atof(token);
		if (fps == 0) {
			fps = 1;
		}
		animations[i].frameLerp = 1000 / fps;
		animations[i].initialLerp = 1000 / fps;
	}

	if (i != MAX_ANIMATIONS) {
		CG_Printf("Error parsing animation file: %s", filename);
		return qfalse;
	}

	// crouch backward animation
	memcpy(&animations[LEGS_BACKCR], &animations[LEGS_WALKCR], sizeof(animation_t));
	animations[LEGS_BACKCR].reversed = qtrue;
	// walk backward animation
	memcpy(&animations[LEGS_BACKWALK], &animations[LEGS_WALK], sizeof(animation_t));
	animations[LEGS_BACKWALK].reversed = qtrue;
	// flag moving fast
	animations[FLAG_RUN].firstFrame = 0;
	animations[FLAG_RUN].numFrames = 16;
	animations[FLAG_RUN].loopFrames = 16;
	animations[FLAG_RUN].frameLerp = 1000 / 15;
	animations[FLAG_RUN].initialLerp = 1000 / 15;
	animations[FLAG_RUN].reversed = qfalse;
	// flag not moving or moving slowly
	animations[FLAG_STAND].firstFrame = 16;
	animations[FLAG_STAND].numFrames = 5;
	animations[FLAG_STAND].loopFrames = 0;
	animations[FLAG_STAND].frameLerp = 1000 / 20;
	animations[FLAG_STAND].initialLerp = 1000 / 20;
	animations[FLAG_STAND].reversed = qfalse;
	// flag speeding up
	animations[FLAG_STAND2RUN].firstFrame = 16;
	animations[FLAG_STAND2RUN].numFrames = 5;
	animations[FLAG_STAND2RUN].loopFrames = 1;
	animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].reversed = qtrue;
	//
	// new anims changes
	//
	//	animations[TORSO_GETFLAG].flipflop = qtrue;
	//	animations[TORSO_GUARDBASE].flipflop = qtrue;
	//	animations[TORSO_PATROL].flipflop = qtrue;
	//	animations[TORSO_AFFIRMATIVE].flipflop = qtrue;
	//	animations[TORSO_NEGATIVE].flipflop = qtrue;
	//
	return qtrue;
}

/*
==========================
CG_FileExists
==========================
*/
static qboolean CG_FileExists(const char *filename) {
	int len;

	len = trap_FS_FOpenFile(filename, 0, FS_READ);
	if (len > 0) {
		return qtrue;
	}
	return qfalse;
}

/*
==========================
CG_FindClientModelFile
==========================
*/
static qboolean CG_FindClientModelFile(char *filename, int length, clientInfo_t *ci, const char *teamName,
									   const char *modelName, const char *skinName, const char *base, const char *ext) {
	const char *team;

	if (cgs.gametype >= GT_TEAM) {
		switch (ci->team) {
		case TEAM_BLUE: {
			team = "blue";
			break;
		}
		default: {
			team = "red";
			break;
		}
		}

		if (strstr(skinName, "_blue") || strstr(skinName, "_red")) {
			char tmpSkin[MAX_QPATH];
			char *ptr;

			Q_strncpyz(tmpSkin, skinName, sizeof(tmpSkin));
			ptr = strrchr(tmpSkin, '_');
			if (ptr != NULL) {
				*ptr = '\0';
				skinName = tmpSkin;
			}
		}
	} else {
		team = DEFAULT_SKIN;
	}

	// "models/wop_players/padman/lower_padsoldier_blue.skin"
	Com_sprintf(filename, length, "models/wop_players/%s/%s_%s_%s.%s", modelName, base, skinName, team, ext);
	if (CG_FileExists(filename)) {
		return qtrue;
	}

	if (cgs.gametype >= GT_TEAM) {
		// "models/wop_players/padman/lower_red.skin"
		Com_sprintf(filename, length, "models/wop_players/%s/%s_%s.%s", modelName, base, team, ext);
	} else {
		// "models/wop_players/padman/lower_padsoldier.skin"
		Com_sprintf(filename, length, "models/wop_players/%s/%s_%s.%s", modelName, base, skinName, ext);
	}
	return CG_FileExists(filename);
}

/*
==========================
CG_FindClientHeadFile
==========================
*/
static qboolean CG_FindClientHeadFile(char *filename, int length, clientInfo_t *ci, const char *teamName,
									  const char *headModelName, const char *headSkinName, const char *base,
									  const char *ext) {
	return CG_FindClientModelFile(filename, length, ci, teamName, headModelName, headSkinName, base, ext);
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean CG_RegisterClientSkin(clientInfo_t *ci, const char *teamName, const char *modelName,
									  const char *skinName, const char *headModelName, const char *headSkinName) {
	char filename[MAX_QPATH];

	if (CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, skinName, "lower", "skin")) {
		ci->legsSkin = trap_R_RegisterSkin(filename);
	}
	if (!ci->legsSkin) {
		Com_Printf("Leg skin load failure: %s\n", filename);
	}

	if (CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, skinName, "upper", "skin")) {
		ci->torsoSkin = trap_R_RegisterSkin(filename);
	}
	if (!ci->torsoSkin) {
		Com_Printf("Torso skin load failure: %s\n", filename);
	}

	if (CG_FindClientHeadFile(filename, sizeof(filename), ci, teamName, headModelName, headSkinName, "head", "skin")) {
		ci->headSkin = trap_R_RegisterSkin(filename);
	}
	if (!ci->headSkin) {
		Com_Printf("Head skin load failure: %s\n", filename);
	}

	// if any skins failed to load
	if (!ci->legsSkin || !ci->torsoSkin || !ci->headSkin) {
		return qfalse;
	}

	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean CG_RegisterClientModelname(clientInfo_t *ci, const char *modelName, const char *skinName,
										   const char *headModelName, const char *headSkinName, const char *teamName) {
	char filename[MAX_QPATH];
	qboolean glowSkinRegistered = qfalse;

	if (!ci || !modelName || !skinName || !headModelName || !headSkinName) {
		return qfalse;
	}

	Com_sprintf(filename, sizeof(filename), "models/wop_players/%s/lower", modelName);
	ci->legsModel = trap_R_RegisterModel(filename);
	if (!ci->legsModel) {
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	Com_sprintf(filename, sizeof(filename), "models/wop_players/%s/upper", modelName);
	ci->torsoModel = trap_R_RegisterModel(filename);
	if (!ci->torsoModel) {
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	ci->headModel = 0;
	// Request for special head skin and thus head model
	if ((Q_stricmp(headSkinName, DEFAULT_SKIN) != 0) && (Q_stricmp(headSkinName, "red") != 0) &&
		(Q_stricmp(headSkinName, "blue") != 0)) {
		// first try directly with skinName
		Com_sprintf(filename, sizeof(filename), "models/wop_players/%s/head_%s", headModelName, headSkinName);
		ci->headModel = trap_R_RegisterModel(filename);

		// if this fails, we will try it with a skinName without the teamending
		if (!ci->headModel) {
			char skinWhithoutTeam[MAX_QPATH];
			char *ptr;
			Q_strncpyz(skinWhithoutTeam, headSkinName, sizeof(skinWhithoutTeam));

			if ((ptr = strstr(skinWhithoutTeam, "_blue")) || (ptr = strstr(skinWhithoutTeam, "_red"))) {
				*ptr = '\0';
			}

			Com_sprintf(filename, sizeof(filename), "models/wop_players/%s/head_%s", headModelName, skinWhithoutTeam);
			ci->headModel = trap_R_RegisterModel(filename);
		}
	}

	if (!ci->headModel) {
		Com_sprintf(filename, sizeof(filename), "models/wop_players/%s/head", headModelName);
		ci->headModel = trap_R_RegisterModel(filename);
	}

	if (!ci->headModel) {
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	if (ci->glowModel) {
		char *ptr, subSkinName[MAX_QPATH];
		glowSkinRegistered = qtrue;

		// There is ony one glowskin for each model/skin,
		// e.g. padman/padrock_red uses padman/padrock glowskin
		// padman/red uses padman
		Q_strncpyz(subSkinName, skinName, sizeof(subSkinName));
		ptr = strrchr(subSkinName, '_');
		if (ptr) {
			*ptr++ = '\0';
		}
		// FIXME: Define RED_SKIN and BLUE_SKIN?
		if ((Q_stricmp("red", subSkinName) == 0) || (Q_stricmp("blue", subSkinName) == 0)) {
			Q_strncpyz(subSkinName, DEFAULT_SKIN, sizeof(subSkinName));
		}
		// Glowskins have a different naming convention, in order not to be confused with the regular ones
		Com_sprintf(filename, sizeof(filename), "models/wop_players/%s/" GLOW_SKIN "_lower_%s.skin", modelName,
					subSkinName);
		ci->legsSkin = trap_R_RegisterSkin(filename);
		Com_sprintf(filename, sizeof(filename), "models/wop_players/%s/" GLOW_SKIN "_upper_%s.skin", modelName,
					subSkinName);
		ci->torsoSkin = trap_R_RegisterSkin(filename);

		Q_strncpyz(subSkinName, headSkinName, sizeof(subSkinName));
		ptr = strrchr(subSkinName, '_');
		if (ptr) {
			*ptr++ = '\0';
		}
		if ((Q_stricmp("red", subSkinName) == 0) || (Q_stricmp("blue", subSkinName) == 0)) {
			Q_strncpyz(subSkinName, DEFAULT_SKIN, sizeof(subSkinName));
		}
		Com_sprintf(filename, sizeof(filename), "models/wop_players/%s/" GLOW_SKIN "_head_%s.skin", headModelName,
					subSkinName);
		ci->headSkin = trap_R_RegisterSkin(filename);

		// if any skins failed to load
		if (!ci->legsSkin || !ci->torsoSkin || !ci->headSkin) {
			glowSkinRegistered = qfalse;
			Com_Printf("Failed to load glow skin file: %s : %s, %s : %s\n", modelName, skinName, headModelName,
					   headSkinName);
		}
	}

	// if any skins failed to load, return failure
	if (!glowSkinRegistered && !CG_RegisterClientSkin(ci, NULL, modelName, skinName, headModelName, headSkinName)) {
		Com_Printf("Failed to load skin file: %s : %s, %s : %s\n", modelName, skinName, headModelName, headSkinName);
		return qfalse;
	}

	// load the animations
	Com_sprintf(filename, sizeof(filename), "models/wop_players/%s/animation.cfg", modelName);
	if (!CG_ParseAnimationFile(filename, ci)) {
		Com_Printf("Failed to load animation file %s\n", filename);
		return qfalse;
	}

	if (CG_FindClientHeadFile(filename, sizeof(filename), ci, NULL, headModelName, headSkinName, "icon", "png")) {
		ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
	} else if (CG_FindClientHeadFile(filename, sizeof(filename), ci, NULL, headModelName, headSkinName, "icon",
									 "tga")) {
		ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
	} else if (CG_FindClientHeadFile(filename, sizeof(filename), ci, NULL, headModelName, headSkinName, "icon",
									 "skin")) {
		ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
	}

	if (!ci->modelIcon) {
		return qfalse;
	}

	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString(const char *v, vec3_t color) {
	int val;

	VectorClear(color);

	val = atoi(v);

	if (val < 1 || val > 7) {
		VectorSet(color, 1, 1, 1);
		return;
	}

	if (val & 1) {
		color[2] = 1.0f;
	}
	if (val & 2) {
		color[1] = 1.0f;
	}
	if (val & 4) {
		color[0] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo(clientInfo_t *ci) {
	const char *dir, *fallback;
	int i, modelloaded;
	const char *s;
	int clientNum;
	char teamname[MAX_QPATH];

	teamname[0] = 0;
	modelloaded = qtrue;
	if (!CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname)) {
		if (cg_buildScript.integer) {
			CG_Error("CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName,
					 ci->headModelName, ci->headSkinName, teamname);
		}

		// fall back to default team name
		if (cgs.gametype >= GT_TEAM) {
			if (!CG_RegisterClientModelname(ci, DEFAULT_TEAM_MODEL, DEFAULT_SKIN, DEFAULT_TEAM_HEADMODEL, DEFAULT_SKIN,
											NULL)) {
				CG_Error("DEFAULT_TEAM_MODEL / skin (%s) failed to register", DEFAULT_TEAM_MODEL);
			}
		} else {
			if (!CG_RegisterClientModelname(ci, DEFAULT_MODEL, DEFAULT_SKIN, DEFAULT_HEADMODEL, DEFAULT_SKIN,
											teamname)) {
				CG_Error("DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL);
			}
		}
		modelloaded = qfalse;
	}

	ci->newAnims = qfalse;
	if (ci->torsoModel) {
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if (trap_R_LerpTag(&tag, ci->torsoModel, 0, 0, 1, "tag_flag")) {
			ci->newAnims = qtrue;
		}
	}

	// sounds
	dir = ci->modelName;
	fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

	for (i = 0; i < MAX_CUSTOM_SOUNDS; i++) {
		s = cg_customSoundNames[i];
		if (!s) {
			break;
		}
		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (modelloaded) {
			ci->sounds[i] = trap_S_RegisterSound(va("sounds/wop_player/%s/%s", dir, s + 1), qfalse);
		}
		if (!ci->sounds[i]) {
			ci->sounds[i] = trap_S_RegisterSound(va("sounds/wop_player/%s/%s", fallback, s + 1), qfalse);
		}
	}

	ci->deferred = qfalse;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;
	for (i = 0; i < MAX_GENTITIES; i++) {
		if (cg_entities[i].currentState.clientNum == clientNum && cg_entities[i].currentState.eType == ET_PLAYER) {
			CG_ResetPlayerEntity(&cg_entities[i]);
		}
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel(clientInfo_t *from, clientInfo_t *to) {
	VectorCopy(from->headOffset, to->headOffset);
	to->headScale = from->headScale;
	to->footsteps = from->footsteps;
	to->gender = from->gender;

	to->legsModel = from->legsModel;
	to->legsSkin = from->legsSkin;
	to->torsoModel = from->torsoModel;
	to->torsoSkin = from->torsoSkin;
	to->headModel = from->headModel;
	to->headSkin = from->headSkin;
	to->modelIcon = from->modelIcon;

	to->newAnims = from->newAnims;

	memcpy(to->animations, from->animations, sizeof(to->animations));
	memcpy(to->sounds, from->sounds, sizeof(to->sounds));
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo(clientInfo_t *ci) {
	int i;
	clientInfo_t *match;

	for (i = 0; i < cgs.maxclients; i++) {
		match = &cgs.clientinfo[i];
		if (!match->infoValid) {
			continue;
		}
		if (match->deferred) {
			continue;
		}
		if (!Q_stricmp(ci->modelName, match->modelName) && !Q_stricmp(ci->skinName, match->skinName) &&
			!Q_stricmp(ci->headModelName, match->headModelName) && !Q_stricmp(ci->headSkinName, match->headSkinName) &&
			(cgs.gametype < GT_TEAM || ci->team == match->team) && (ci->glowModel == match->glowModel)) {
			// this clientinfo is identical, so use its handles

			ci->deferred = qfalse;

			CG_CopyClientInfoModel(match, ci);

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo(clientInfo_t *ci) {
	int i;
	clientInfo_t *match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for (i = 0; i < cgs.maxclients; i++) {
		match = &cgs.clientinfo[i];
		if (!match->infoValid || match->deferred) {
			continue;
		}
		if (Q_stricmp(ci->skinName, match->skinName) || Q_stricmp(ci->modelName, match->modelName) ||
			//			 Q_stricmp( ci->headModelName, match->headModelName ) ||
			//			 Q_stricmp( ci->headSkinName, match->headSkinName ) ||
			(cgs.gametype >= GT_TEAM && ci->team != match->team)) {
			continue;
		}
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo(ci);
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if (cgs.gametype >= GT_TEAM) {
		for (i = 0; i < cgs.maxclients; i++) {
			match = &cgs.clientinfo[i];
			if (!match->infoValid || match->deferred) {
				continue;
			}
			if (Q_stricmp(ci->skinName, match->skinName) || (cgs.gametype >= GT_TEAM && ci->team != match->team)) {
				continue;
			}
			if (Q_stricmp(ci->modelName, match->modelName)) {
				continue;
			}

			ci->deferred = qtrue;
			CG_CopyClientInfoModel(match, ci);
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo(ci);
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for (i = 0; i < cgs.maxclients; i++) {
		match = &cgs.clientinfo[i];
		if (!match->infoValid) {
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel(match, ci);
		return;
	}

	// we should never get here...
	CG_Printf("CG_SetDeferredClientInfo: no valid clients!\n");

	CG_LoadClientInfo(ci);
}

/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo(int clientNum) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char *configstring;
	const char *v;
	char *skin;
	char modelStr[MAX_QPATH];
	team_t povTeam; // our point of view has the perspective of what team
	int color;
	int oldTeam;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	if (!configstring[0]) {
		// this may cause bugs elsewhere, I want to keep the TeleOut infos around
		ci->infoValid = qfalse;
		// memset( ci, 0, sizeof( *ci ) );
		return; // player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset(&newInfo, 0, sizeof(newInfo));

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz(newInfo.name, v, sizeof(newInfo.name));

	// colors
	v = Info_ValueForKey(configstring, "c1");
	CG_ColorFromString(v, newInfo.color1);

	v = Info_ValueForKey(configstring, "c2");
	{
		int colorid = atoi(v);

		if (colorid < 0) {
			colorid = 0;
		} else if (colorid >= NUM_SPRAYCOLORS) {
			colorid = (NUM_SPRAYCOLORS - 1);
		}

		VectorCopy(spraycolors[colorid], newInfo.color2);
	}

	// bot skill
	v = Info_ValueForKey(configstring, "skill");
	newInfo.botSkill = atoi(v);

	// handicap
	v = Info_ValueForKey(configstring, "hc");
	newInfo.handicap = atoi(v);

	// wins
	v = Info_ValueForKey(configstring, "w");
	newInfo.wins = atoi(v);

	// losses
	v = Info_ValueForKey(configstring, "l");
	newInfo.losses = atoi(v);

	// team
	v = Info_ValueForKey(configstring, "t");
	newInfo.team = atoi(v);

	// spraylogo
	v = Info_ValueForKey(configstring, "sl");
	Q_strncpyz(newInfo.spraylogo, v, sizeof(newInfo.spraylogo));

	// team leader
	v = Info_ValueForKey(configstring, "tl");
	newInfo.teamLeader = atoi(v);
	oldTeam = ci->team;

	// this may be run before the first snapshot arrives
	povTeam = (cg.snap) ? cgs.clientinfo[cg.snap->ps.clientNum].team : TEAM_SPECTATOR;

	// model
	v = Info_ValueForKey(configstring, "model");
	if (cg_forceModel.integer > 0) {
		if (cgs.gametype >= GT_TEAM) {
			if ((cg_forceModel.integer & FORCEMODEL_TEAM) && (newInfo.team == povTeam)) {
				trap_Cvar_VariableStringBuffer("team_model", modelStr, sizeof(modelStr));
			} else if ((cg_forceModel.integer & FORCEMODEL_ENEMY) && (newInfo.team != povTeam)) {
				trap_Cvar_VariableStringBuffer("model", modelStr, sizeof(modelStr));
			} else {
				Q_strncpyz(modelStr, v, sizeof(modelStr));
			}
		} else if (cg_forceModel.integer & FORCEMODEL_ENEMY) {
			trap_Cvar_VariableStringBuffer("model", modelStr, sizeof(modelStr));
		} else {
			Q_strncpyz(modelStr, v, sizeof(modelStr));
		}
	} else {
		Q_strncpyz(modelStr, v, sizeof(modelStr));
	}

	if ((skin = strchr(modelStr, '/')) == NULL) {
		skin = DEFAULT_SKIN;
	} else {
		*skin++ = 0;
	}

	Q_strncpyz(newInfo.skinName, skin, sizeof(newInfo.skinName));
	Q_strncpyz(newInfo.modelName, modelStr, sizeof(newInfo.modelName));

	// headmodel
	v = Info_ValueForKey(configstring, "hmodel");
	if (cg_forceModel.integer > 0) {
		if (cgs.gametype >= GT_TEAM) {
			if ((cg_forceModel.integer & FORCEMODEL_TEAM) && (newInfo.team == povTeam)) {
				trap_Cvar_VariableStringBuffer("team_headmodel", modelStr, sizeof(modelStr));
			} else if ((cg_forceModel.integer & FORCEMODEL_ENEMY) && (newInfo.team != povTeam)) {
				trap_Cvar_VariableStringBuffer("headmodel", modelStr, sizeof(modelStr));
			} else {
				Q_strncpyz(modelStr, v, sizeof(modelStr));
			}
		} else if (cg_forceModel.integer & FORCEMODEL_ENEMY) {
			trap_Cvar_VariableStringBuffer("headmodel", modelStr, sizeof(modelStr));
		} else {
			Q_strncpyz(modelStr, v, sizeof(modelStr));
		}
	} else {
		Q_strncpyz(modelStr, v, sizeof(modelStr));
	}

	if ((skin = strchr(modelStr, '/')) == NULL) {
		skin = DEFAULT_SKIN;
	} else {
		*skin++ = 0;
	}

	Q_strncpyz(newInfo.headSkinName, skin, sizeof(newInfo.headSkinName));
	Q_strncpyz(newInfo.headModelName, modelStr, sizeof(newInfo.headModelName));

	newInfo.glowModel = qfalse;
	color = ColorIndex(COLOR_BLACK);
	if (cgs.gametype >= GT_TEAM) {
		if (strlen(cg_glowModel.string) && (newInfo.team != povTeam)) {
			color = ColorIndex(cg_glowModel.string[0]);
			newInfo.glowModel = qtrue;
		}
		if (strlen(cg_glowModelTeam.string) && (newInfo.team == povTeam)) {
			color = ColorIndex(cg_glowModelTeam.string[0]);
			newInfo.glowModel = qtrue;
		}
	} else {
		if (strlen(cg_glowModel.string)) {
			color = ColorIndex(cg_glowModel.string[0]);
			newInfo.glowModel = qtrue;
		}
	}
	newInfo.glowColor[0] = (g_color_table[color][0] * 255);
	newInfo.glowColor[1] = (g_color_table[color][1] * 255);
	newInfo.glowColor[2] = (g_color_table[color][2] * 255);
	newInfo.glowColor[3] = 255;

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if (!CG_ScanForExistingClientInfo(&newInfo)) {
		qboolean forceDefer;

		forceDefer = trap_MemoryRemaining() < 4000000;

		// if we are defering loads, just have it pick the first valid
		if (forceDefer || (cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading)) {
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo(&newInfo);
			// if we are low on memory, leave them with this model
			if (forceDefer) {
				CG_Printf("Memory is low.  Using deferred model.\n");
				newInfo.deferred = qfalse;
			}
		} else {
			CG_LoadClientInfo(&newInfo);
		}
	}

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;

	// if the local client changed teams in a team gametype, adjust team- and enemymodels
	if (clientNum == cg.predictedPlayerState.clientNum && cgs.gametype >= GT_TEAM && newInfo.team != oldTeam) {
		CG_ForceModelChange();
	}
}

/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers(void) {
	int i;
	clientInfo_t *ci;

	// scan for a deferred player to load
	for (i = 0, ci = cgs.clientinfo; i < cgs.maxclients; i++, ci++) {
		if (ci->infoValid && ci->deferred) {
			// if we are low on memory, leave it deferred
			if (trap_MemoryRemaining() < 4000000) {
				CG_Printf("Memory is low.  Using deferred model.\n");
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo(ci);
			//			break;
		}
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation) {
	animation_t *anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS) {
		CG_Error("Bad animation number: %i", newAnimation);
	}

	anim = &ci->animations[newAnimation];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if (cg_debugAnim.integer) {
		CG_Printf("Anim: %i\n", newAnimation);
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale) {
	int f, numFrames;
	animation_t *anim;

	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	if (ci->ftIsFrozen) {
		CG_SetLerpFrameAnimation(ci, lf, LEGS_IDLE);
	}

	// see if the animation sequence is switching
	if (newAnimation != lf->animationNumber || !lf->animation) {
		CG_SetLerpFrameAnimation(ci, lf, newAnimation);
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if (!anim->frameLerp) {
			return; // shouldn't happen
		}
		if (cg.time < lf->animationTime) {
			lf->frameTime = lf->animationTime; // initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale; // adjust for haste, etc

		numFrames = anim->numFrames;
		if (anim->flipflop) {
			numFrames *= 2;
		}
		if (f >= numFrames) {
			f -= numFrames;
			if (anim->loopFrames) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if (anim->reversed) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		} else if (anim->flipflop && f >= anim->numFrames) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f % anim->numFrames);
		} else {
			lf->frame = anim->firstFrame + f;
		}
		if (cg.time > lf->frameTime) {
			lf->frameTime = cg.time;
			if (cg_debugAnim.integer) {
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
	}

	if (lf->frameTime > cg.time + 200) {
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int animationNumber) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation(ci, lf, animationNumber);
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}

/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation(centity_t *cent, int *legsOld, int *legs, float *legsBackLerp, int *torsoOld, int *torso,
							   float *torsoBackLerp) {
	clientInfo_t *ci;
	int clientNum;
	float speedScale;

	clientNum = cent->currentState.clientNum;

	if (cg_noPlayerAnims.integer) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	if (cent->currentState.powerups & (1 << PW_SPEEDY)) {
		speedScale = 2;
	} else {
		speedScale = 1;
	}

	ci = &cgs.clientinfo[clientNum];

	// do the shuffle turn frames locally
	if (cent->pe.legs.yawing && (cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == LEGS_IDLE) {
		CG_RunLerpFrame(ci, &cent->pe.legs, LEGS_TURN, speedScale);
	} else {
		CG_RunLerpFrame(ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale);
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrame(ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale);

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles(float destination, float swingTolerance, float clampTolerance, float speed, float *angle,
						   qboolean *swinging) {
	float swing;
	float move;
	float scale;

	if (!*swinging) {
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if (swing > swingTolerance || swing < -swingTolerance) {
			*swinging = qtrue;
		}
	}

	if (!*swinging) {
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract(destination, *angle);
	scale = fabs(swing);
	if (scale < swingTolerance * 0.5) {
		scale = 0.5;
	} else if (scale < swingTolerance) {
		scale = 1.0;
	} else {
		scale = 2.0;
	}

	// swing towards the destination angle
	if (swing >= 0) {
		move = cg.frametime * scale * speed;
		if (move >= swing) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	} else if (swing < 0) {
		move = cg.frametime * scale * -speed;
		if (move <= swing) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if (swing > clampTolerance) {
		*angle = AngleMod(destination - (clampTolerance - 1));
	} else if (swing < -clampTolerance) {
		*angle = AngleMod(destination + (clampTolerance - 1));
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch(centity_t *cent, vec3_t torsoAngles) {
	int t;
	float f;

	t = cg.time - cent->pe.painTime;
	if (t >= PAIN_TWITCH_TIME) {
		return;
	}

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if (cent->pe.painDirection) {
		torsoAngles[ROLL] += 20 * f;
	} else {
		torsoAngles[ROLL] -= 20 * f;
	}
}

/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles(centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3]) {
	vec3_t legsAngles, torsoAngles, headAngles;
	float dest;
	static int movementOffsets[8] = {0, 22, 45, -22, 0, 22, -45, -22};
	vec3_t velocity;
	float speed;
	int dir, clientNum;
	clientInfo_t *ci;

	VectorCopy(cent->lerpAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	VectorClear(legsAngles);
	VectorClear(torsoAngles);

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ((cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) != LEGS_IDLE ||
		(cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND) {
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	 // always center
		cent->pe.torso.pitching = qtrue; // always center
		cent->pe.legs.yawing = qtrue;	 // always center
	}

	// adjust legs for movement dir
	if (cent->currentState.eFlags & EF_DEAD) {
		// don't let dead bodies twitch
		dir = 0;
	} else {
		dir = cent->currentState.angles2[YAW];
		if (dir < 0 || dir > 7) {
			CG_Error("Bad player movement angle");
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[dir];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[dir];

	// torso
	CG_SwingAngles(torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing);
	CG_SwingAngles(legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);

	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if (headAngles[PITCH] > 180) {
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	} else {
		dest = headAngles[PITCH] * 0.75f;
	}
	CG_SwingAngles(dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching);
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

	//
	clientNum = cent->currentState.clientNum;
	if (clientNum >= 0 && clientNum < MAX_CLIENTS) {
		ci = &cgs.clientinfo[clientNum];
		if (ci->fixedtorso) {
			torsoAngles[PITCH] = 0.0f;
		}
	}

	// --------- roll -------------

	// lean towards the direction of travel
	VectorCopy(cent->currentState.pos.trDelta, velocity);
	speed = VectorNormalize(velocity);
	if (speed) {
		vec3_t axis[3];
		float side;

		speed *= 0.05f;

		AnglesToAxis(legsAngles, axis);
		side = speed * DotProduct(velocity, axis[1]);
		legsAngles[ROLL] -= side;

		side = speed * DotProduct(velocity, axis[0]);
		legsAngles[PITCH] += side;
	}

	//
	clientNum = cent->currentState.clientNum;
	if (clientNum >= 0 && clientNum < MAX_CLIENTS) {
		ci = &cgs.clientinfo[clientNum];
		if (ci->fixedlegs) {
			legsAngles[YAW] = torsoAngles[YAW];
			legsAngles[PITCH] = 0.0f;
			legsAngles[ROLL] = 0.0f;
		}
	}

	// pain twitch
	CG_AddPainTwitch(cent, torsoAngles);

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
	AnglesToAxis(legsAngles, legs);
	AnglesToAxis(torsoAngles, torso);
	AnglesToAxis(headAngles, head);
}

/*
===============
CG_SpeedyTrail
===============
*/
static void CG_SpeedyTrail(centity_t *cent) {
	int i;
	vec3_t tmpv3, posBetween;
	int squaredLength;

	if (cent->nextSpeedytime > cg.time) {
		return;
	}

	VectorSubtract(cent->speedyOrigin, cent->lerpOrigin, tmpv3);
	squaredLength = VectorLengthSquared(tmpv3);

	if (squaredLength > Square(128)) {
		VectorCopy(cent->lerpOrigin, cent->speedyOrigin);
		cent->nextSpeedytime += 100;
		return;
	}

	if (squaredLength < Square(32)) {
		cent->nextSpeedytime += 100;
		return;
	}

	if (cent->nextSpeedytime == 0) {
		VectorCopy(cent->lerpOrigin, cent->speedyOrigin);
		cent->nextSpeedytime = (cg.time + 100);
		return;
	}

	for (i = 0; i < 5; i++) {
		VectorMA(cent->lerpOrigin, (i / 5), tmpv3, posBetween);
		posBetween[2] -= 20; // position at about feet

		LaunchSpeedyPuffTrail(posBetween);
	}

	VectorCopy(cent->lerpOrigin, cent->speedyOrigin);
	cent->nextSpeedytime += 100;
}

#define TRAIL_REVIVAL_DISTANCE 20
void CG_RevivalTrail(centity_t *cent) {
	vec3_t origin;
	static int last = 0;

	if ((cent->revivalEffectTime + 150) < cg.time) {
		VectorCopy(cent->lerpOrigin, origin);

		// spreading out the emission points around the player
		// TODO: make it more random
		switch (last) {
		case 0:
			origin[1] -= TRAIL_REVIVAL_DISTANCE;
			break;
		case 1:
			origin[1] += TRAIL_REVIVAL_DISTANCE;
			break;
		case 2:
			origin[0] -= TRAIL_REVIVAL_DISTANCE;
			break;
		case 3:
			origin[0] += TRAIL_REVIVAL_DISTANCE;
			break;
		default:
			last = -1;
		}

		LaunchRevivalParticle(origin, 1000);

		last++;
		cent->revivalEffectTime = cg.time;
	}
}

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem(centity_t *cent, qhandle_t hModel) {
	refEntity_t ent;
	vec3_t angles;
	vec3_t axis[3];

	VectorCopy(cent->lerpAngles, angles);
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis(angles, axis);

	memset(&ent, 0, sizeof(ent));
	VectorMA(cent->lerpOrigin, -16, axis[0], ent.origin);
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis(angles, ent.axis);

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag(centity_t *cent, qhandle_t hSkin, refEntity_t *torso) {
	clientInfo_t *ci;
	refEntity_t pole;
	refEntity_t flag;
	vec3_t angles, dir;
	int legsAnim, flagAnim, updateangles;
	float angle, d;

	// show the flag pole model
	memset(&pole, 0, sizeof(pole));
	pole.hModel = cgs.media.flagPoleModel;
	VectorCopy(torso->lightingOrigin, pole.lightingOrigin);
	pole.shadowPlane = torso->shadowPlane;
	pole.renderfx = torso->renderfx;
	CG_PositionEntityOnTag(&pole, torso, torso->hModel, "tag_flag");
	trap_R_AddRefEntityToScene(&pole);

	// show the flag model
	memset(&flag, 0, sizeof(flag));
	flag.hModel = cgs.media.flagFlapModel;
	flag.customSkin = hSkin;
	VectorCopy(torso->lightingOrigin, flag.lightingOrigin);
	flag.shadowPlane = torso->shadowPlane;
	flag.renderfx = torso->renderfx;

	VectorClear(angles);

	updateangles = qfalse;
	legsAnim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if (legsAnim == LEGS_IDLE || legsAnim == LEGS_IDLECR) {
		flagAnim = FLAG_STAND;
	} else if (legsAnim == LEGS_WALK || legsAnim == LEGS_WALKCR) {
		flagAnim = FLAG_STAND;
		updateangles = qtrue;
	} else {
		flagAnim = FLAG_RUN;
		updateangles = qtrue;
	}

	if (updateangles) {

		VectorCopy(cent->currentState.pos.trDelta, dir);
		// add gravity
		dir[2] += 100;
		VectorNormalize(dir);
		d = DotProduct(pole.axis[2], dir);
		// if there is enough movement orthogonal to the flag pole
		if (fabs(d) < 0.9) {
			//
			d = DotProduct(pole.axis[0], dir);
			if (d > 1.0f) {
				d = 1.0f;
			} else if (d < -1.0f) {
				d = -1.0f;
			}
			angle = acos(d);

			d = DotProduct(pole.axis[1], dir);
			if (d < 0) {
				angles[YAW] = 360 - angle * 180 / M_PI;
			} else {
				angles[YAW] = angle * 180 / M_PI;
			}
			if (angles[YAW] < 0)
				angles[YAW] += 360;
			if (angles[YAW] > 360)
				angles[YAW] -= 360;

			// vectoangles( cent->currentState.pos.trDelta, tmpangles );
			// angles[YAW] = tmpangles[YAW] + 45 - cent->pe.torso.yawAngle;
			// change the yaw angle
			CG_SwingAngles(angles[YAW], 25, 90, 0.15f, &cent->pe.flag.yawAngle, &cent->pe.flag.yawing);
		}

		/*
		d = DotProduct(pole.axis[2], dir);
		angle = Q_acos(d);

		d = DotProduct(pole.axis[1], dir);
		if (d < 0) {
			angle = 360 - angle * 180 / M_PI;
		}
		else {
			angle = angle * 180 / M_PI;
		}
		if (angle > 340 && angle < 20) {
			flagAnim = FLAG_RUNUP;
		}
		if (angle > 160 && angle < 200) {
			flagAnim = FLAG_RUNDOWN;
		}
		*/
	}

	// set the yaw angle
	angles[YAW] = cent->pe.flag.yawAngle;
	// lerp the flag animation frames
	ci = &cgs.clientinfo[cent->currentState.clientNum];
	CG_RunLerpFrame(ci, &cent->pe.flag, flagAnim, 1);
	flag.oldframe = cent->pe.flag.oldFrame;
	flag.frame = cent->pe.flag.frame;
	flag.backlerp = cent->pe.flag.backlerp;

	AnglesToAxis(angles, flag.axis);
	CG_PositionRotatedEntityOnTag(&flag, &pole, pole.hModel, "tag_flag");

	trap_R_AddRefEntityToScene(&flag);
}

/*
===============
CG_PlayerCartridges
===============
*/
// FIXME: Use seperate defines, structs, consts
static void CG_PlayerCartridges(centity_t *cent, int renderfx) {
	int i, j, tokens = cent->currentState.generic1;
	float angle;
	refEntity_t ent;
	vec3_t dir, origin;
	skulltrail_t *trail = &cg.skulltrails[cent->currentState.number];

	if (!tokens) {
		trail->numpositions = 0;
		return;
	}

	if (tokens > MAX_SKULLTRAIL) {
		tokens = MAX_SKULLTRAIL;
	}

	// add cartridges if there are more than last time
	for (i = 0; i < (tokens - trail->numpositions); i++) {
		for (j = trail->numpositions; j > 0; j--) {
			VectorCopy(trail->positions[j - 1], trail->positions[j]);
		}
		VectorCopy(cent->lerpOrigin, trail->positions[0]);
	}
	trail->numpositions = tokens;

	// move all the cartridges along the trail
	VectorCopy(cent->lerpOrigin, origin);
	for (i = 0; i < trail->numpositions; i++) {
		VectorSubtract(trail->positions[i], origin, dir);
		if (VectorNormalize(dir) > 30) {
			VectorMA(origin, 30, dir, trail->positions[i]);
		}
		VectorCopy(trail->positions[i], origin);
	}

	memset(&ent, 0, sizeof(ent));

	switch (cgs.clientinfo[cent->currentState.clientNum].team) {
	case TEAM_RED:
		ent.hModel = cg_items[cgs.media.redCartridgeEntNum].models[0];
		break;
	case TEAM_BLUE:
		ent.hModel = cg_items[cgs.media.blueCartridgeEntNum].models[0];
		break;
	default:
		ent.hModel = cg_items[cgs.media.neutralCartridgeEntNum].models[0];
	}

	ent.renderfx = renderfx;

	VectorCopy(cent->lerpOrigin, origin);
	for (i = 0; i < trail->numpositions; i++) {
		VectorSubtract(origin, trail->positions[i], ent.axis[0]);
		ent.axis[0][2] = 0;
		VectorNormalize(ent.axis[0]);
		VectorSet(ent.axis[2], 0, 0, 1);
		CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);

		VectorInverse(ent.axis[1]);

		VectorCopy(trail->positions[i], ent.origin);
		angle = ((((cg.time + (500 * MAX_SKULLTRAIL) - (500 * i)) / 16) & 255) * (M_PI * 2) / 255);
		ent.origin[2] += (sin(angle) * 10);
		ent.customShader = 0; // to remove the powerup-shader from the last cartridge
		CG_AddRefEntityWithPowerups(&ent, &cent->currentState, cgs.clientinfo[cent->currentState.number].team);
		VectorCopy(trail->positions[i], origin);
	}
}

/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups(centity_t *cent, refEntity_t *torso) {
	int powerups;
	clientInfo_t *ci;

	if (cent->currentState.eFlags & (EF_DEAD | EF_NOLIFESLEFT)) {
		return;
	}

	if (cent->currentState.eFlags & EF_FLOATER) {
		trap_S_AddLoopingSound(cent->currentState.clientNum, cent->lerpOrigin, vec3_origin, cgs.media.flightSound);
		CG_SmokePuff(cent->lerpOrigin, vec3_origin, 20, 0.2, 0.0, 0.4, 0.5, 2000, cg.time, 0, 0,
					 cgs.media.smokePuffShader);
	}

	powerups = cent->currentState.powerups;
	if (!powerups) {
		return;
	}

	// quad gives a dlight

	// ente will das es gar nicht mehr leuchtet
	/*(original)
		if ( powerups & ( 1 << PW_QUAD ) ) {
			trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1 );
		}
	*/

	// flight plays a looped sound

	ci = &cgs.clientinfo[cent->currentState.clientNum];

	// add the lolly only if it's not the local client
	if ((cent->currentState.clientNum != cg.snap->ps.clientNum) || cg.renderingThirdPerson) {
		// red lolly
		if (powerups & (1 << PW_REDFLAG)) {
			if (ci->newAnims) {
				CG_PlayerFlag(cent, cgs.media.redFlagFlapSkin, torso);
			} else {
				CG_TrailItem(cent, cgs.media.redFlagModel);
			}
		}

		// blue lolly
		if (powerups & (1 << PW_BLUEFLAG)) {
			if (ci->newAnims) {
				CG_PlayerFlag(cent, cgs.media.blueFlagFlapSkin, torso);
			} else {
				CG_TrailItem(cent, cgs.media.blueFlagModel);
			}
		}
	}
	// add lolly glow for any client
	if (powerups & (1 << PW_REDFLAG)) {
		trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1.0, 0.2, 0.2);
	} else if (powerups & (1 << PW_BLUEFLAG)) {
		trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 0.2, 0.2, 1.0);
	}

	// speedy leaves smoke trails
	if (powerups & (1 << PW_SPEEDY)) {
		CG_SpeedyTrail(cent);
	}
	// revival leaves floating hearts
	if (powerups & (1 << PW_REVIVAL)) {
		CG_RevivalTrail(cent);
	}
}

/*
=============
Draw an iceblock that encloses part of the player.
=============
*/
static void CG_PlayerFrozen(centity_t *cent, const refEntity_t *legs) {
	refEntity_t iceblock;

	if (!FT_PlayerIsFrozen(cent))
		return;

	memset(&iceblock, 0, sizeof(iceblock));
	VectorCopy(legs->lightingOrigin, iceblock.lightingOrigin);
	iceblock.shadowPlane = legs->shadowPlane;
	iceblock.renderfx = legs->renderfx;

	iceblock.hModel = cgs.media.iceblockModel;

	AxisCopy(axisDefault, iceblock.axis);

	// CG_PositionRotatedEntityOnTag( &iceblock, legs, legs->hModel, "tag_torso");
	VectorCopy(legs->origin, iceblock.origin);

	trap_R_AddRefEntityToScene(&iceblock);
}

/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite(const centity_t *cent, qhandle_t shader, qboolean wallhack) {
	int rf;
	refEntity_t ent;

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		rf = RF_THIRD_PERSON; // only show in mirrors
	} else {
		rf = 0;
	}

	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;

	if (wallhack) {
		ent.renderfx |= RF_DEPTHHACK;
	}

	// NOTE: Currently the gamecode uses EF_AWARD_CAP for both CTL and BB,
	//       which results in medalPadStar. There is no sound however, since
	//       AddBalloonScores() does not increase PERS_CAPTURES, which is associated
	//       with the award in CTL.
	if (cgs.gametype == GT_BALLOON && shader == cgs.media.medalPadStar) {
		int team = cgs.clientinfo[cent->currentState.clientNum].team;
		ent.customShader = cgs.media.hud_balloon;

		ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 255;
		switch (team) {
		case TEAM_BLUE:
			ent.shaderRGBA[2] = 255;
			break;
		default:
			ent.shaderRGBA[0] = 255;
			break;
		}
	}

	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites(const centity_t *cent) {
	int team;

	if (cent->currentState.eFlags & EF_CONNECTION) {
		CG_PlayerFloatSprite(cent, cgs.media.connectionShader, qfalse);
		return;
	}

	if (cent->currentState.eFlags & EF_TALK) {
		CG_PlayerFloatSprite(cent, cgs.media.balloonShader, qfalse);
		return;
	}

	// got voicechat from player within the last second?
	if (cg.lastVoiceTime[cent->currentState.clientNum] > cg.time - 1000) {
		// TODO: find out how bots get through this, do they have sane lastVoiceTime values?
		if (0 == cgs.clientinfo[cent->currentState.clientNum].botSkill) {
			CG_PlayerFloatSprite(cent, cgs.media.voiceIcon, qfalse);
			return;
		}
	}

	if (cent->currentState.eFlags & EF_AWARD_EXCELLENT) {
		CG_PlayerFloatSprite(cent, cgs.media.medalExcellent, qfalse);
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_GAUNTLET) {
		CG_PlayerFloatSprite(cent, cgs.media.medalGauntlet, qfalse);
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_SPRAYGOD) {
		CG_PlayerFloatSprite(cent, cgs.media.medalSpraygod, qfalse);
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_SPRAYKILLER) {
		CG_PlayerFloatSprite(cent, cgs.media.medalSpraykiller, qfalse);
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_CAP) {
		CG_PlayerFloatSprite(cent, cgs.media.medalPadStar, qfalse);
		return;
	}

	team = cgs.clientinfo[cent->currentState.clientNum].team;

	// // freezetag
	// if (CG_FreezeTag() && FT_PlayerIsFrozen(cent)) {
	//  if (cg.snap->ps.persistant[PERS_TEAM] == team)
	//   CG_PlayerFloatSprite(cent, cgs.media.freezeIconShader);
	//  return;
	// }

	if (!(cent->currentState.eFlags & EF_DEAD) && cg.snap->ps.persistant[PERS_TEAM] == team &&
		cgs.gametype >= GT_TEAM) {
		if (cg_drawFriend.integer) {
			qboolean wallhack = (cg_icons.integer & ICON_TEAMMATE) && cgs.gametype == GT_BALLOON;
			CG_PlayerFloatSprite(cent, cgs.media.friendShader, wallhack);
		}
		return;
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define SHADOW_DISTANCE 128
static qboolean CG_PlayerShadow(centity_t *cent, float *shadowPlane) {
	vec3_t end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t trace;
	float alpha;

	*shadowPlane = 0;

	if (cg_shadows.integer == 0) {
		return qfalse;
	}

	// no shadows when invisible
	if (cent->currentState.powerups & (1 << PW_VISIONLESS)) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy(cent->lerpOrigin, end);
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace(&trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID);

	// no shadow if too high
	if (trace.fraction == 1.0 || trace.startsolid || trace.allsolid) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if (cg_shadows.integer != 1) { // no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0f - trace.fraction;

	// bk0101022 - hack / FPE - bogus planes?
	// assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f )

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark(cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, cent->pe.legs.yawAngle, alpha, alpha,
				  alpha, 1, qfalse, 24, qtrue);

	return qtrue;
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash(centity_t *cent) {
	vec3_t start, end;
	trace_t trace;
	int contents;
	polyVert_t verts[4];

	if (!cg_shadows.integer) {
		return;
	}

	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents(end, 0);
	if (!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))) {
		return;
	}

	VectorCopy(cent->lerpOrigin, start);
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents(start, 0);
	if (contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA));

	if (trace.fraction == 1.0) {
		return;
	}

	// create a mark polygon
	VectorCopy(trace.endpos, verts[0].xyz);
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[1].xyz);
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[2].xyz);
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[3].xyz);
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.wakeMarkShader, 4, verts);
}

/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups(refEntity_t *ent, entityState_t *state, int team) {
	if (state->powerups & (1 << PW_VISIONLESS)) {
		ent->customShader = cgs.media.invisShader;
	} else if (state->powerups & (1 << PW_PADPOWER)) {
		ent->customShader = cgs.media.PadPowerShader;
		switch (team) {
		case TEAM_RED:
			ent->shaderRGBA[1] = ent->shaderRGBA[2] = 0;
			ent->shaderRGBA[0] = 255;
			break;
		case TEAM_BLUE:
			ent->shaderRGBA[0] = ent->shaderRGBA[1] = 0;
			ent->shaderRGBA[2] = 255;
			break;
		default:
			ent->shaderRGBA[0] = ent->shaderRGBA[1] = ent->shaderRGBA[2] = 255;
		}
	}

	if (state->powerups & (1 << PW_BERSERKER)) {
		// Add with existing effects
		trap_R_AddRefEntityToScene(ent);

		// Apply shader, ent will be added lateron
		ent->customShader = cgs.media.BerserkerAura;
	}

	if (cg.zoomed && (cg.snap->ps.weapon == WP_KMA97) && cgs.media.zoomKMAaura) {
		int playerTeam;
		playerTeam = cg.snap->ps.persistant[PERS_TEAM];

		if ((playerTeam == TEAM_RED || playerTeam == TEAM_BLUE) && (playerTeam == team)) {
			// not an enemy, draw no aura
		} else {
			// enemy, draw aura
			trap_R_AddRefEntityToScene(ent);
			ent->customShader = cgs.media.zoomKMAaura;
		}
	}

	if ((state->eFlags & EF_DEAD) && state->time) {
		ent->customShader = cgs.media.deadfadeSkin;
		if ((cg.time - state->time) >= 4000) {
			ent->shaderRGBA[3] = 0;
		} else {
			ent->shaderRGBA[3] = (255 - (255 * (cg.time - state->time) / 4000.0));
		}
	}

	trap_R_AddRefEntityToScene(ent);

	// Add teleportation effect
	if (state->eType == ET_PLAYER) {
		int timeIn = (cg.time - cgs.clientinfo[state->clientNum].lastTeleInTime);

		if (!(state->eFlags & EF_DEAD) && cgs.clientinfo[state->clientNum].lastTeleInTime && (timeIn < 1500)) {
			float f = (timeIn / 1500.0);
			vec3_t axisBackup[3];
			int i;
			float scale = (1.01 + (f * 0.3));

			switch (team) {
			case TEAM_RED:
				ent->customShader = cgs.media.teleportEffectRedShader;
				break;
			case TEAM_BLUE:
				ent->customShader = cgs.media.teleportEffectBlueShader;
				break;
			default:
				ent->customShader = cgs.media.teleportEffectGreenShader;
			}

			ent->shaderRGBA[0] = ent->shaderRGBA[1] = ent->shaderRGBA[2] = (255 * (1.0 - f));

			AxisCopy(ent->axis, axisBackup);

			for (i = 0; i < 3; i++) {
				VectorScale(ent->axis[i], scale, ent->axis[i]);
			}
			trap_R_AddRefEntityToScene(ent);

			AxisCopy(axisBackup, ent->axis);
		}
	}
}

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts(vec3_t normal, int numVerts, polyVert_t *verts) {
	int i, j;
	float incoming;
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;

	trap_R_LightForPoint(verts[0].xyz, ambientLight, directedLight, lightDir);

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct(normal, lightDir);
		if (incoming <= 0) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = (ambientLight[0] + incoming * directedLight[0]);
		if (j > 255) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = (ambientLight[1] + incoming * directedLight[1]);
		if (j > 255) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = (ambientLight[2] + incoming * directedLight[2]);
		if (j > 255) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

/*
===============
CG_AddStars
===============
*/
#define STAR_TIME 3000
#define STAR_RADIUS 10
#define STAR_HEIGHT 14
#define STAR_PERIOD 1200
#define STAR_AMPLITUDE 1.5
#define STAR_SHADEROFS 0.05

void CG_AddStars(int clientNum, vec3_t origin, int num, float alpha) {
	refEntity_t star;
	float phi;
	int i, offset;

	// initialize refEntity
	memset(&star, 0, sizeof(star));
	star.hModel = cgs.media.star;

	if (!cg.renderingThirdPerson && (clientNum == cg.snap->ps.clientNum)) {
		star.renderfx = RF_THIRD_PERSON;
	}

	// fix draw order problems
	// hopefully this trick helps in every situation
	// (holds some rather "empiric" values)
	offset = ((cg.time % STAR_TIME) * num / STAR_TIME);

	if (alpha != 1.0) {
		star.shaderRGBA[3] = (255 * alpha);
		star.renderfx |= RF_FORCEENTALPHA;
	}

	for (i = offset; i < (num + offset); i++) {
		// calculate axis
		phi = (2 * M_PI *
			   (((float)(cg.time % STAR_TIME) / STAR_TIME) - ((float)i / num) + (cg.refdefViewAngles[YAW] / 360)));

		VectorSet(star.axis[0], cos(phi), sin(phi), 0);
		VectorSet(star.axis[1], -sin(phi), cos(phi), 0);
		VectorSet(star.axis[2], 0, 0, 1);

		// calculate origin
		phi = (2 * M_PI * (((float)(cg.time % STAR_PERIOD) / STAR_PERIOD) + ((float)i / num)));
		VectorMA(origin, STAR_RADIUS, star.axis[0], star.origin);
		star.origin[2] += (STAR_HEIGHT + (STAR_AMPLITUDE * sin(phi)));

		// add front plane to scene
		star.shaderTime = ((i % num) * STAR_SHADEROFS);
		trap_R_AddRefEntityToScene(&star);
	}
}

static void CG_PlayerGlowmodel(int clientNum, const clientInfo_t *ci, refEntity_t *legs, refEntity_t *torso,
							   refEntity_t *head) {
	if (ci->glowModel) {
		memcpy(legs->shaderRGBA, &ci->glowColor, sizeof(legs->shaderRGBA));
		memcpy(torso->shaderRGBA, &ci->glowColor, sizeof(torso->shaderRGBA));
		memcpy(head->shaderRGBA, &ci->glowColor, sizeof(head->shaderRGBA));
	}
}

/*
===============
CG_Player
===============
*/
void CG_Player(centity_t *cent) {
	clientInfo_t *ci;
	refEntity_t legs;
	refEntity_t torso;
	refEntity_t head;
	int clientNum;
	int renderfx;
	qboolean shadow;
	float shadowPlane;
	float lerpOrigin2Backup = cent->lerpOrigin[2];
	float angleYawBackup = cent->lerpAngles[YAW];
	int stateTime = (cg.time - cent->currentState.time);

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		CG_Error("Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[clientNum];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if (!ci->infoValid) {
		return;
	}

	// get the player model information
	renderfx = 0;
	if (cent->currentState.number == cg.snap->ps.clientNum) {
		if (!cg.renderingThirdPerson) {
			renderfx = RF_THIRD_PERSON; // only draw in mirrors
		} else {
			if (cg_cameraMode.integer) {
				return;
			}
		}
	}

	if ((cent->currentState.eFlags & EF_DEAD) && cent->currentState.time) {
		if (stateTime >= 4000) {
			return;
		} else {
			// make dead corpses fly into the sky, depending on when they died
			cent->lerpOrigin[2] += (stateTime / 20.0);
			cent->lerpAngles[YAW] += (stateTime / 10.0);
		}
	}

	memset(&legs, 0, sizeof(legs));
	memset(&torso, 0, sizeof(torso));
	memset(&head, 0, sizeof(head));

	CG_PlayerGlowmodel(clientNum, ci, &legs, &torso, &head);

	// get the rotation information
	CG_PlayerAngles(cent, legs.axis, torso.axis, head.axis);

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation(cent, &legs.oldframe, &legs.frame, &legs.backlerp, &torso.oldframe, &torso.frame,
					   &torso.backlerp);

	// add the talk baloon or disconnect icon
	CG_PlayerSprites(cent);

	// add the shadow
	shadow = CG_PlayerShadow(cent, &shadowPlane);

	// add a water splash if partially in and out of water
	CG_PlayerSplash(cent);

	if (cg_shadows.integer == 3 && shadow) {
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN; // use the same origin for all

	// add cartridges
	if ((cgs.gametype == GT_SPRAY) || (cgs.gametype == GT_SPRAYFFA)) {
		CG_PlayerCartridges(cent, (renderfx & ~RF_LIGHTING_ORIGIN));
	}

	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	VectorCopy(cent->lerpOrigin, legs.origin);

	VectorCopy(cent->lerpOrigin, legs.lightingOrigin);
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy(legs.origin, legs.oldorigin); // don't positionally lerp at all

	CG_AddRefEntityWithPowerups(&legs, &cent->currentState, ci->team);

	// if the model failed, allow the default nullmodel to be displayed
	if (!legs.hModel) {
		return;
	}

	//
	// add the torso
	//
	torso.hModel = ci->torsoModel;
	if (!torso.hModel) {
		return;
	}

	torso.customSkin = ci->torsoSkin;

	VectorCopy(cent->lerpOrigin, torso.lightingOrigin);

	CG_PositionRotatedEntityOnTag(&torso, &legs, ci->legsModel, "tag_torso");

	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	CG_AddRefEntityWithPowerups(&torso, &cent->currentState, ci->team);

	//
	// add the head
	//
	head.hModel = ci->headModel;
	if (!head.hModel) {
		return;
	}
	head.customSkin = ci->headSkin;

	VectorCopy(cent->lerpOrigin, head.lightingOrigin);

	CG_PositionRotatedEntityOnTag(&head, &torso, ci->torsoModel, "tag_head");

	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	CG_AddRefEntityWithPowerups(&head, &cent->currentState, ci->team);

	// reset values to real values - dead corpses
	cent->lerpOrigin[2] = lerpOrigin2Backup;
	cent->lerpAngles[YAW] = angleYawBackup;

	// add stars if dead
	if (cent->currentState.eFlags & (EF_DEAD | EF_HURT) && !(cent->currentState.powerups & (1 << PW_VISIONLESS))) {
		int stars = 0;
		float alpha = 1.0;

		stars += ((cent->currentState.eFlags & EF_DEAD) ? 3 : 0);
		stars += ((cent->currentState.eFlags & EF_HURT) ? 3 : 0);

		if ((cent->currentState.eFlags & EF_DEAD) && cent->currentState.time) {
			if (stateTime <= 1000) {
				alpha = (1.0 - stateTime / 1000.0);
			} else {
				alpha = 0.0;
			}
		}
		CG_AddStars(cent->currentState.number, head.origin, stars, alpha);
	}

	// save player position for LPS
	if ((cgs.gametype == GT_LPS) && !(cgs.lpsflags & LPSF_NOARROWS) &&
		(cent->currentState.number != cg.snap->ps.clientNum) &&
		!(cent->currentState.eFlags & (EF_DEAD | EF_NOLIFESLEFT))) {
		VectorCopy(cent->lerpOrigin, ci->curPos);
		ci->curPos[2] += 64; // position above head
		ci->lastPosSaveTime = cg.time;
	} else if (ci->lastPosSaveTime != cg.time) {
		ci->lastPosSaveTime = 0; // "reset"
	}

	// save position for freezetag xray icons
	if (CG_FreezeTag() && cent->currentState.number != cg.snap->ps.clientNum &&
		cent->currentState.powerups & (1 << PW_FREEZE) && cg.snap->ps.persistant[PERS_TEAM] == ci->team) {
		ci->curPos[0] = cent->lerpOrigin[0];
		ci->curPos[1] = cent->lerpOrigin[1];
		ci->curPos[2] = cent->lerpOrigin[2] + 64;
		ci->lastPosSaveTime = cg.time;
	} else if (ci->lastPosSaveTime != cg.time) {
		ci->lastPosSaveTime = 0;
	}

	// push frozen state into clientinfo
	// we need that in CG_RunLerpFrame :X
	if (CG_FreezeTag() && cent->currentState.powerups & (1 << PW_FREEZE))
		ci->ftIsFrozen = qtrue;
	else
		ci->ftIsFrozen = qfalse;

	//
	// add the gun / barrel / flash
	//
	CG_AddPlayerWeapon(&torso, NULL, cent, ci->team);

	// add powerups floating behind the player
	CG_PlayerPowerups(cent, &torso);

	// unlagged - client options
	// add the bounding box (if cg_drawBBox is 1)
	CG_AddBoundingBox(cent);
	// unlagged - client options
	CG_PlayerFrozen(cent, &legs);
}

//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity(centity_t *cent) {
	cent->errorTime = -99999; // guarantee no error decay added
	cent->extrapolated = qfalse;

	CG_ClearLerpFrame(&cgs.clientinfo[cent->currentState.clientNum], &cent->pe.legs, cent->currentState.legsAnim);
	CG_ClearLerpFrame(&cgs.clientinfo[cent->currentState.clientNum], &cent->pe.torso, cent->currentState.torsoAnim);

	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);

	VectorCopy(cent->lerpOrigin, cent->rawOrigin);
	VectorCopy(cent->lerpAngles, cent->rawAngles);

	memset(&cent->pe.legs, 0, sizeof(cent->pe.legs));
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset(&cent->pe.torso, 0, sizeof(cent->pe.torso));
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	if (cg_debugPosition.integer) {
		CG_Printf("%i ResetPlayerEntity yaw=%f\n", cent->currentState.number, cent->pe.torso.yawAngle);
	}
}

void CG_UpdateVoipTeamIDs() {
	int i;
	int team;
	char oldTarget[256];
	char newTarget[256];
	qboolean firstEntry = qtrue;
	static int lastUpdateTime = 0;

	// twice per second should be enough
	if (lastUpdateTime + 500 > cg.time)
		return;
	lastUpdateTime = cg.time;

	// team games only
	if (cgs.gametype < GT_TEAM)
		return;

	// only update if the client is talking to his team (check for cl_voip and sv_voip too?)
	trap_Cvar_VariableStringBuffer("cl_voipSendTarget", oldTarget, sizeof(oldTarget));
	if (Q_stricmpn(oldTarget, "team", 4))
		return;

	Q_strncpyz(newTarget, "team", sizeof(newTarget));

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
		team = TEAM_SPECTATOR;
	else
		team = cgs.clientinfo[cg.snap->ps.clientNum].team;

	for (i = 0; i < cgs.maxclients; i++) {
		// don't include local client
		if (i == cg.snap->ps.clientNum)
			continue;

		if (!cgs.clientinfo[i].infoValid)
			continue;

		if (cgs.clientinfo[i].team != team)
			continue;

		// ignore bots
		if (cgs.clientinfo[i].botSkill)
			continue;

		// first entry has no comma
		if (firstEntry) {
			Q_strcat(newTarget, sizeof(newTarget), va("%d", i));
			firstEntry = qfalse;
		} else
			Q_strcat(newTarget, sizeof(newTarget), va(",%d", i));
	}

	if (Q_stricmp(oldTarget, newTarget))
		trap_Cvar_Set("cl_voipSendTarget", newTarget);
}
