/*
######################## ######################## ########################
	reload stations coded by #@(aka Raute)

	this are code for the padmod reload-stations(health/ammo)
	... there was never a ammo version ;P


missbrauchte entety-vars:
pos1[0] => timer for full-health-msg

pos2[0] => reloadtime (==0: kein reload geplant, !=0: reloadzeitpunkt)
pos2[1] => timer for min-touch-frequenz (max. 1touch/0.1sek)
pos2[2] => player touched this station (==0: kein touch seit think, ==1: player touched)

s.angles2[1] => loading-ring animation (0: no rings, 1: full ring animation)
s.angles2[2] => health in the station (0: leer, 1:voll ... wird in der cgame genutzt)


######################## ######################## ########################
*/

#include "g_local.h"

#define HEALTHSTATION_DEFAULT_HEALTH 300
#define HEALTHSTATION_DEFAULT_RATE 2
#define HEALTHSTATION_RELOADTIME 30000
// TODO: #defines for FULLENERGYMESSAGE_DELAY, TOUCH_DELAY, (.3/500)

/*
	Other abused entity fields:
		health:			current amount of health left in station. Also see angles2[2]
		damage:			maximum amount of health in station (for reloading)
		splashDamage:	whether health is considered infinite
		count:			amount of health the player gets. Also see pos2[1]
*/
/*
	TODO/FIXME:
		nextthink is supposed to happen on the next frame, so FRAMETIME should
		be used?

		trDuration seems to be unused.

		Entire code might have some quirks related to multiple players standing
		in the station at the same time.
*/

/*
	{"station_health", SP_station_health},
*/

static void Think_ReloadStation(gentity_t *ent) {
	if (ent->pos2[2] == 0.0f) // only move down if there is no player touch
	{
		if (ent->s.angles2[1] > 0.01f)
			ent->s.angles2[1] -= 0.0006f * (level.time - level.previousTime); // 0.3 / 500 ;);
		else
			ent->s.angles2[1] = 0.0f;
	}

	if (ent->pos2[0] && level.time > ent->pos2[0]) {
		gentity_t *te;

		// restore maximum health, see spawn code
		ent->health = ent->damage;
		ent->s.apos.trDuration = ent->health;
		ent->s.angles2[2] = 1.0f;

		te = G_TempEntity(ent->s.pos.trBase, EV_GENERAL_SOUND);
		te->s.eventParm = G_SoundIndex("sounds/healthstation/station_reloaded");
		te->r.svFlags |= SVF_BROADCAST;

		ent->pos2[0] = 0.0f;
	}

	ent->nextthink = level.time + 5; // think in the next frame
	ent->pos2[2] = 0.0f;
}

static void Touch_ReloadStation(gentity_t *ent, gentity_t *other, trace_t *trace) {
	if (!other->client)
		return;

	if (G_FreezeTag() && FT_PlayerIsFrozen(other))
		return;

	if (ent->s.angles2[1] < 0.99f)
		ent->s.angles2[1] += 0.0006f * (level.time - level.previousTime); // 0.3 / 500 ;)
	else
		ent->s.angles2[1] = 1.0f;

	ent->pos2[2] = 1.0f;

	if (other->health >= other->client->ps.stats[STAT_MAX_HEALTH]) {
		gentity_t *te;

		if (ent->pos1[0] + 2000 < level.time) {
			te = G_TempEntity(ent->s.pos.trBase, EV_GENERAL_SOUND);
			te->s.eventParm = G_SoundIndex("sounds/healthstation/full_energy");
			te->r.svFlags |= SVF_BROADCAST;
			ent->pos1[0] = (float)level.time;
		}

		return;
	}

	// if station ran empty
	if (ent->health <= 0) {
		// and no reload is scheduled
		if (ent->pos2[0] == 0) {
			// schedule a reload after RELOADTIME
			ent->pos2[0] = (float)(level.time + HEALTHSTATION_RELOADTIME);
		}

		return;
	}

	if (((float)level.time - ent->pos2[1]) < 100.0f)
		return; // touch-event only every 0.1 sek

	other->health += ent->count;
	if (other->health > other->client->ps.stats[STAT_MAX_HEALTH]) {
		other->health = other->client->ps.stats[STAT_MAX_HEALTH];
	}
	other->client->ps.stats[STAT_HEALTH] = other->health;

	// only reduce if not "infinite" health
	if (ent->splashDamage) {
		ent->health -= ent->count;
	}

	ent->pos2[1] = (float)level.time;

	ent->s.apos.trDuration = ent->health;
	ent->s.angles2[2] = ((float)ent->health / ent->damage);
}

/*
########################
SP_station_health
########################
*/
void SP_station_health(gentity_t *ent) {
	int health, count;

	// TODO: Use #defines?
	ent->r.mins[0] = -40;
	ent->r.mins[1] = -40;
	ent->r.mins[2] = 0;
	ent->r.maxs[0] = 40;
	ent->r.maxs[1] = 40;
	ent->r.maxs[2] = 64;
	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_ReloadStation;

	G_SetOrigin(ent, ent->s.origin);

	ent->think = Think_ReloadStation;
	ent->nextthink = (level.time + 5);

	ent->s.eType = ET_STATION;
	ent->inuse = qtrue;
	// FIXME: Use G_InitGentity() instead?

	trap_LinkEntity(ent);

	G_SpawnInt("health", XSTRING(HEALTHSTATION_DEFAULT_HEALTH), &health);
	if (health > 0) {
		ent->health = health;
		ent->splashDamage = qtrue; // station can deplete
	} else {
		// Use a positive "random" value, so codechanges are minimal
		ent->health = HEALTHSTATION_DEFAULT_HEALTH;
		ent->splashDamage = qfalse; // station has infinite health
	}
	ent->damage = ent->health; // store maximum health for reloading

	// TODO: Sanity check, at least >0 ?
	G_SpawnInt("count", XSTRING(HEALTHSTATION_DEFAULT_RATE), &count);
	ent->count = count;

	// cache sounds
	G_SoundIndex("sounds/healthstation/full_energy");
	G_SoundIndex("sounds/healthstation/station_reloaded");

	ent->s.apos.trDuration = ent->health;
	ent->s.angles2[2] = ((float)ent->health / ent->damage);
}
