#include "g_local.h"

#define FREEZE_MESSAGE_ATTACKER "you froze"
#define FREEZE_MESSAGE_VICTIM "you are frozen\n(wait for a teammate to thaw you)"
#define FREEZE_MESSAGE_GLOBAL "was frozen by"
#define FREEZE_MESSAGE_WORLD "was frozen"

#define THAW_MESSAGE_PLAYER "you have been thawed"
#define THAW_MESSAGE_THAWER "you have thawn"
#define THAW_MESSAGE_GLOBAL "was thawn by"
#define THAW_MESSAGE_WORLD "thawed"

#define THAWING_TIME 6000
#define THAWING_TIMESTEP 1000
#define THAWING_STEPS (THAWING_TIME / THAWING_TIMESTEP)
#define THAWING_DISTANCE 200

#define FT_THAWED_HEATRECHARGE 100
#define FT_DEFAULTWEAPONSET "punchy:-1:pumper:15:bubbleg:40"
#define FT_AMMO_PICKUPRATIO 0.33f

static vec_t FT_SquaredDistanceBetweenEntities(const gentity_t *ent, const gentity_t *other) {
	return DistanceSquared(ent->client->ps.origin, other->client->ps.origin);
}

static gentity_t *FT_ClientToEntity(gclient_t *client) {
	return &g_entities[client->ps.clientNum];
}

static void FT_AnnounceFreeze(const gclient_t *client, const gclient_t *attacker) {
	if (!client)
		return;

	if (!FT_MatchInProgress())
		return;

	trap_SendServerCommand(client->ps.clientNum, va("cp \"^7%s\n\"", FREEZE_MESSAGE_VICTIM));

	// tell attacker
	if (attacker) {
		trap_SendServerCommand(attacker->ps.clientNum,
							   va("cp \"^7%s ^7%s^7.\n\"", FREEZE_MESSAGE_ATTACKER, client->pers.netname));
	}

	if (attacker)
		trap_SendServerCommand(
			-1, va("print \"^7%s ^7%s ^7%s\n\"", client->pers.netname, FREEZE_MESSAGE_GLOBAL, attacker->pers.netname));
	else
		trap_SendServerCommand(-1, va("print \"^7%s ^7%s\n\"", client->pers.netname, FREEZE_MESSAGE_WORLD));
}

static void FT_AnnounceThaw(gclient_t *client, gclient_t *other) {
	if (!client)
		return;

	if (!FT_MatchInProgress())
		return;

	trap_SendServerCommand(client->ps.clientNum, va("cp \"^7%s\n\"", THAW_MESSAGE_PLAYER));

	if (other) {
		trap_SendServerCommand(other->ps.clientNum,
							   va("cp \"^7%s ^7%s^7.\n\"", THAW_MESSAGE_THAWER, client->pers.netname));
		trap_SendServerCommand(
			-1, va("print \"^7%s ^7%s ^7%s\n\"", client->pers.netname, THAW_MESSAGE_GLOBAL, other->pers.netname));
	} else {
		trap_SendServerCommand(-1, va("print \"^7%s ^7%s\n\"", client->pers.netname, THAW_MESSAGE_WORLD));
	}
}

static void FT_TossPowerUps(gentity_t *player) {
	int i;
	gclient_t *client;
	gitem_t *item;
	gentity_t *ent;

	client = player->client;

	for (i = PW_NONE + 1; i < PW_NUM_POWERUPS; i++) {
		if (client->ps.powerups[i] > level.time) {
			item = BG_FindItemForPowerup(i);
			if (!item)
				continue;

			ent = Drop_Item(player, item, 45);
			ent->count = (client->ps.powerups[i] - level.time) / 1000;
			client->ps.powerups[i] = 0;
		}
	}
}

static qboolean FT_PlayersTooClose(const gentity_t *player, const gentity_t *other) {
	if (FT_SquaredDistanceBetweenEntities(player, other) < 32 * 32)
		return qtrue;

	return qfalse;
}

// deprecated
static void FT_PushThawerAway(const gentity_t *player, gentity_t *thawer, int distance) {
	vec3_t dir;
	vec3_t vel;

	VectorSubtract(thawer->client->ps.origin, player->client->ps.origin, dir);
	VectorNormalize(dir);
	dir[2] += 0.5;
	VectorNormalize(dir);
	VectorScale(dir, 40, vel);
	VectorAdd(thawer->client->ps.velocity, vel, thawer->client->ps.velocity);
}

/*
	returns qtrue if we are in FreezeTag
*/
qboolean G_FreezeTag() {
	return g_gametype.integer == GT_FREEZETAG;
}

// deprecated
static void G_checkPlayerThawerDistance(const gentity_t *player, gentity_t *thawer) {
	if (FT_PlayersTooClose(player, thawer)) {
		// G_Printf( "thawer too close to player!\n" );
		FT_PushThawerAway(player, thawer, 32);
	}
}

/*
	returns qtrue if the client is frozen
*/
qboolean FT_ClientIsFrozen(const gclient_t *client) {
	if (!client)
		return qfalse;

	if (client->pers.teamState.state == TEAM_ACTIVE) {
		if (g_ft_debug.integer >= 2) {
			G_Printf("FT_clientIsFrozen: client->pers.frozen: %i\n", client->pers.frozen);
		}
		return client->pers.frozen;
	} else if (client->ps.powerups[PW_FREEZE] > level.time) {
		return qtrue;
	} else {
		if (g_ft_debug.integer >= 2) {
			G_Printf("FT_clientIsFrozen: qfalse\n");
		}
		return qfalse;
	}
}

qboolean FT_PlayerIsFrozen(gentity_t *player) {
	return FT_ClientIsFrozen(player->client);
}

/**
 * Sets the Player to frozen state, meaning he can't deal and recieve further damage
 * drops down to the ground plane if frozen in midair and is unable to move.
 *
 * @param player the player being frozen
 * @param other the attacker (is NULL if attacker is world)
 *
 * Called most likely during combat when a player ran out of heat (health)
 */
void FT_FreezePlayer(gentity_t *player, gentity_t *other) {
	const gclient_t *other_client;

	if (!player->client) {
		return; // not a valid player
	}

	if (other) {
		other_client = other->client;
	} else {
		other_client = NULL;
	}

	FT_TossPowerUps(player);

	player->health = 1;
	player->client->ps.stats[STAT_HEALTH] = player->health;

	player->client->pers.frozen = qtrue;
	player->client->freezeTime = level.time;
	player->client->freezeCount = 6;
	VectorCopy(player->client->ps.origin, player->client->freezePos);

	player->client->ps.pm_type = PM_FREEZE;
	player->client->ps.powerups[PW_FREEZE] = level.time + 10000000;
	player->client->ps.stats[STAT_CHILL] = player->client->freezeCount;

	if (other_client) {
		AddScore(other, player->client->ps.origin, 1, SCORE_FROZEN_S);
	}

	FT_AnnounceFreeze(player->client, other_client);
	CalculateRanks();

	if (!other->client) {
		G_LogPrintf("Freeze: %s by %s\n", player->client->pers.netname, "<world>");
	} else {
		G_LogPrintf("Freeze: %s by %s\n", player->client->pers.netname, other->client->pers.netname);
	}
}

/*
Sets the Player back into an unfrozen state, meaning he can deal and recieve damage,
move about etc normally.
player's health is recharged to FT_THAWED_HEATRECHARGE
player's ammo amount is back to what it was befor getting frozen

Called most likely when a teammate successfully went through the thawing process.
*/
void FT_ThawPlayer(gentity_t *player, gentity_t *other) {
	if (!player || !player->client)
		return;

	player->health = FT_THAWED_HEATRECHARGE;
	player->client->ps.stats[STAT_HEALTH] = player->health;
	player->client->pers.frozen = qfalse;
	player->client->ps.powerups[PW_FREEZE] = 0;
	player->client->ps.pm_type = PM_NORMAL;

	if (other) {
		if (other->client)
			AddScore(other, player->client->ps.origin, 1, SCORE_THAWED_S);

		G_checkPlayerThawerDistance(player, other);
		FT_AnnounceThaw(player->client, other->client);
	} else {
		FT_AnnounceThaw(player->client, NULL);
	}

	CalculateRanks();

	if (!other->client)
		G_LogPrintf("Thaw: %s by %s\n", player->client->pers.netname, "<world>");
	else
		G_LogPrintf("Thaw: %s by %s\n", player->client->pers.netname, other->client->pers.netname);
}

/**
 * Finds the nearest frozen teammate of player
 *
 * @param player the player's gentity_t*
 * @return gentity_t* of the nearest teammate found
 */
gentity_t *FT_NearestFrozenPlayer(gentity_t *player) {
	gentity_t *nearest;
	gentity_t *ent;
	gclient_t *client;
	int i;
	int team;
	vec_t closestDistance, tempDistance;

	team = player->client->sess.sessionTeam;

	nearest = NULL;
	closestDistance = INFINITE;
	for (i = 0; i < level.maxclients; i++) {
		client = &level.clients[i];
		if (client == player->client)
			continue;
		if (client->sess.sessionTeam == team) {
			ent = FT_ClientToEntity(client);
			if (FT_PlayerIsFrozen(ent)) {
				tempDistance = FT_SquaredDistanceBetweenEntities(player, ent);
				if (tempDistance < closestDistance) {
					closestDistance = tempDistance;
					nearest = ent;
				}
			}
		}
	}
	return nearest;
}

/**
 * Checks if player is in thawing range to other
 *
 * @param player the player trying to thaw
 * @param other the frozen player that's to be thawed
 * @return qtrue or qfalse
 */
qboolean FT_InThawingRange(const gentity_t *player, const gentity_t *other) {
	if (FT_SquaredDistanceBetweenEntities(player, other) <= THAWING_DISTANCE * THAWING_DISTANCE)
		return qtrue;

	return qfalse;
}

/**
 * Advances the thawing process between player and thawer and notifies both about
 * the progress. (FIXME:sends centerprint commands directly to the clients involved)
 *
 * @param player the player that is being thawed
 * @param thawer the player that is doing the thawing
 */
void FT_ProgressThawing(gentity_t *player, gentity_t *thawer) {
	int freezeCount, i;
	static char buffer[10];
	static char buffer2[10];
	memset(buffer, 0, sizeof(buffer));
	memset(buffer2, 0, sizeof(buffer2));

	freezeCount = player->client->freezeCount;

	/*
	 * construct the the "progress bar"
	 * adding one to the green stripes for every freezecount missing
	 * and adding one to the white stripes for every freezecount present
	 */
	for (i = 6; i > 0; i--) {
		if (i > freezeCount)
			Q_strcat(buffer, sizeof(buffer), "|");
		else
			Q_strcat(buffer2, sizeof(buffer2), va("|"));
	}

#if 0
	// send progress to the thawer
	if (level.time - thawer->client->lastProgressTime > 500) {
		// trap_SendServerCommand( thawer - g_entities, va( "cp \"^7thawing\n[^2%s^7%s^7]\n\"", buffer, buffer2 ));
		thawer->client->lastProgressTime = level.time;
	}
	// send progress to the frozen player
	if (level.time - player->client->lastProgressTime > 500) {
		// trap_SendServerCommand( player - g_entities, va( "cp \"^7thawing\n[^2%s^7%s^7]\n\"", buffer, buffer2 ));
		player->client->lastProgressTime = level.time;
	}
#endif

	// advance thawing
	if (level.time - player->client->freezeTime > THAWING_TIMESTEP) {
		player->client->freezeTime = level.time;
		player->client->freezeCount--;
		// player->client->ps.powerups[ PW_FREEZE ] = player->client->freezeCount * 1000000;
		player->client->ps.stats[STAT_CHILL] = player->client->freezeCount;
	}

	if (player->client->freezeCount <= 0) {
		FT_ThawPlayer(player, thawer); // thawing completed
	}

#if 0
	G_checkPlayerThawerDistance(player, thawer); // make sure they don't get stuck in eachother

	if (level.time - thawer->client->lastProgressTime > 500)
		thawer->client->ps.stats[ STAT_CHILL ] = 0;
	else
#endif
	thawer->client->ps.stats[STAT_CHILL] = player->client->ps.stats[STAT_CHILL];

	thawer->client->lastProgressTime = level.time;
	// player->client->lastProgressTime = level.time;
}

/*
===============
returns qtrue if all members of the team are frozen
===============
*/
qboolean FT_WholeTeamIsFrozen(int team) {
	gclient_t *client;
	int i;

	if (g_ft_debug.integer >= 2)
		G_Printf("FT_wholeTeamIsFrozen!\n");

	for (i = 0; i < level.maxclients; i++) {
		client = &level.clients[i];
		if (client->sess.sessionTeam == team && !FT_PlayerIsFrozen(FT_ClientToEntity(client)))
			return qfalse;
	}

	if (g_ft_debug.integer == 1)
		G_Printf("level.teamScores[ %i ]:%i\n", team, level.teamScores[team]);

	return qtrue;
}

/*
===============
FT_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
static const char *FT_ValueForKey(const char *s, const char *key) {
	char pkey[BIG_INFO_KEY];
	static char value[2][BIG_INFO_VALUE]; // use two buffers so compares
										  // work without stomping on each other
	static int valueindex = 0;
	char *o;

	if (!s || !key) {
		return "";
	}

	if (strlen(s) >= BIG_INFO_STRING) {
		Com_Error(ERR_DROP, "FT_ValueForKey: oversize infostring");
	}

	valueindex ^= 1;
	if (*s == ':')
		s++;
	while (1) {
		o = pkey;
		while (*s != ':') {
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != ':' && *s) {
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp(key, pkey))
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}

void FT_InitFreezeTag(void) {
	if (Q_stricmp(g_ft_useWeaponSet.string, "") == 0 || Q_stricmp(g_ft_useWeaponSet.string, "0") == 0) {
		// wop-normal weapon equip
		level.ftWeaponSet = (1 << WP_NIPPER);
		if (g_gametype.integer == GT_TEAM || g_gametype.integer == GT_FREEZETAG) {
			level.ftWeaponSetAmmo[WP_NIPPER] = 50;
		} else {
			level.ftWeaponSetAmmo[WP_NIPPER] = 100;
		}
		level.ftWeaponSet |= (1 << WP_PUNCHY);
		level.ftWeaponSetAmmo[WP_PUNCHY] = -1;
		level.ftWeaponSetAmmo[WP_GRAPPLING_HOOK] = -1;
	} else {
		// set up weapon set
		const char *weaponstring;
		const char *v;
		int ammo;
		int i;

		static char *weapons[] = {"none",	 "punchy",	 "nipper",	"pumper",	"balloony", "betty",
								  "boaster", "splasher", "bubbleg", "imperius", NULL};

		weaponstring = g_ft_useWeaponSet.string;

		level.ftWeaponSet = 0;

		for (i = 1; weapons[i] != NULL; i++) {
			v = FT_ValueForKey(weaponstring, weapons[i]);
			ammo = atoi(v);

			if (ammo) {
				level.ftWeaponSet |= (1 << i);
				level.ftWeaponSetAmmo[i] = ammo;
			}
		}

		level.ftWeaponSetAmmoRatio = g_ft_weaponSetAmmoRatio.value;
	}

	// load rounds status
	{
		char s[MAX_STRING_CHARS];
		const char *v;
		char mapname[256];
		char info[1024];

		trap_Cvar_VariableStringBuffer("ftrounds", s, sizeof(s));

		v = FT_ValueForKey(s, "played");
		level.ftNumRoundsPlayed = atoi(v);

		v = FT_ValueForKey(s, "redwins");
		level.ftNumRoundsWon[TEAM_RED] = atoi(v);

		v = FT_ValueForKey(s, "bluewins");
		level.ftNumRoundsWon[TEAM_BLUE] = atoi(v);

		/*
			check if we changed the map, clear all round info if we did
		*/
		trap_GetServerinfo(info, sizeof(info));
		Q_strncpyz(mapname, Info_ValueForKey(info, "mapname"), sizeof(mapname));
		v = FT_ValueForKey(s, "map");

		// G_Printf("^2This Map: %s\n", mapname);
		// G_Printf("^2Last Map: %s\n", v);

		if (Q_stricmp(mapname, v) != 0) {
			// clear
			level.ftNumRoundsPlayed = 0;
			level.ftNumRoundsWon[TEAM_RED] = 0;
			level.ftNumRoundsWon[TEAM_BLUE] = 0;
		}

		// dbg
		// G_Printf("^2played: %i\n", level.ftNumRoundsPlayed);
		// G_Printf("^2Red Wins: %i\n", level.ftNumRoundsWon[TEAM_RED]);
		// G_Printf("^2Blue Wins: %i\n", level.ftNumRoundsWon[TEAM_BLUE]);
	}
}

void FT_AddStartWeapons(gclient_t *client) {
	int i;

	client->ps.stats[STAT_WEAPONS] = level.ftWeaponSet;
	for (i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++)
		client->ps.ammo[i] = level.ftWeaponSetAmmo[i];
}

void FT_AddAmmo(gentity_t *player) {
	int i;
	for (i = 1; i < WP_NUM_WEAPONS; i++) {
		if (player->client->ps.stats[STAT_WEAPONS] & (1 << i)) {
			Add_Ammo(player, i, ((float)(level.ftWeaponSetAmmo[i]) * level.ftWeaponSetAmmoRatio) + 0.5f);
		}
	}
}

void FT_CalculateRoundScores(void) {
#if 0
	if (!level.allRoundsPlayed) {
		int blue, red;
		int i;
		blue = red = 0;
		for (i = 0; i < level.maxclients; i++) {
			gclient_t *client = &level.clients[i];
			if (client->sess.sessionTeam == TEAM_BLUE && !FT_PlayerIsFrozen(FT_ClientToEntity(client)))
				blue++;
			else if (client->sess.sessionTeam == TEAM_RED && !FT_PlayerIsFrozen(FT_ClientToEntity(client)))
				red++;
		}
	} else {
		blue = level.ftNumRoundsWon[TEAM_BLUE];
		red = level.ftNumRoundsWon[TEAM_RED];
	}

	level.teamScores[TEAM_BLUE] = blue;
	level.teamScores[TEAM_RED] = red;
#endif
	level.teamScores[TEAM_BLUE] = level.ftNumRoundsWon[TEAM_BLUE];
	level.teamScores[TEAM_RED] = level.ftNumRoundsWon[TEAM_RED];
}

void FT_RelocateToNearestSpawnPoint(gentity_t *player) {
	// FIXME: better idea?
	ClientSpawn(player);
}

static qboolean FT_enoughPlayersToStart(void) {
	if (TeamCount(-1, TEAM_BLUE) >= 1 && TeamCount(-1, TEAM_RED) >= 1)
		return qtrue;

	return qfalse;
}

qboolean FT_MatchInProgress(void) {
	if (g_doWarmup.integer && level.warmupTime == 0) {
		if (FT_enoughPlayersToStart())
			return qtrue;
	} else if (!g_doWarmup.integer && FT_enoughPlayersToStart()) {
		return qtrue;
	}

	return qfalse;
}

qboolean FT_CanSwitchTeam(gentity_t *player, int team) {
#if 0
	G_Printf("frozen: %i\nteam: %i\n", G_playerIsFrozen(player), team);
	if (player->client)
		G_Printf("^2has client\n");

	if (G_playerIsFrozen(player) && team != TEAM_SPECTATOR)
		return qfalse;
#endif
	return qtrue;
}
