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
// g_local.h -- local definitions for game module

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_public.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
//(original)#define	GAMEVERSION	"baseq3"
#define GAMEVERSION "wop"

//(original)#define BODY_QUEUE_SIZE		8
#define BODY_QUEUE_SIZE                                                                                                \
	64 // noch zu ueberdenken, ob man wirklich so viele entities dafuer frei halten kann ... ohne probleme zu bekommen

#define INFINITE 1000000

#define FRAMETIME 100 // msec
#define CARNAGE_REWARD_TIME 3000
#define REWARD_SPRITE_TIME 4000
#define SPRAYREWARD_SPRITE_TIME 6000

#define INTERMISSION_DELAY_TIME 1000
#define SP_INTERMISSION_DELAY_TIME 5000

// gentity->flags
#define FL_GODMODE 0x00000010
#define FL_NOTARGET 0x00000020
#define FL_TEAMSLAVE 0x00000400 // not the first on the team
#define FL_NO_KNOCKBACK 0x00000800
#define FL_DROPPED_ITEM 0x00001000
#define FL_NO_BOTS 0x00002000		// spawn point not for bot use
#define FL_NO_HUMANS 0x00004000		// spawn point just for bots
#define FL_FORCE_GESTURE 0x00008000 // force gesture on client

// Maybe these should be inline funtions? ///FIXME(#@): yes, they should!
#define IsSyc() ((GT_SPRAYFFA == g_gametype.integer) || (GT_SPRAY == g_gametype.integer))
#define InSprayroom(client) (IsSyc() && ((client)->ps.stats[STAT_SPRAYROOMSECS] > 0))

// NOTE(#@): tiny difference ... "lps death" is <= 0 ... "lps dead spec" is < 0 ... I introduced this year(s) ago to
// have a "nice" death before getting to the spectator-mode
#define LPSDeadSpec(client) ((GT_LPS == g_gametype.integer) && ((client)->sess.livesleft < 0))
#define LPSFinalDead(client) ((GT_LPS == g_gametype.integer) && ((client)->sess.livesleft <= 0))

// Fixed reasons and scores for AddScore and AddTeamScore
// Also see g_team.h for some teamplay (ctf) specific score boni
#define SCORE_SUICIDE -1
#define SCORE_TEAMKILL -1
#define SCORE_KILL 1
#define SCORE_SURVIVE 1
#define SCORE_SPRAY 5
#define SCORE_SPRAY_WRONGWALL -SCORE_SPRAY
#define SCORE_BONUS_SPRAYKILLER 10
#define SCORE_BONUS_SPRAYGOD 25
#define SCORE_CAPTURE 1

#define SCORE_SUICIDE_S "suicide"
#define SCORE_TEAMKILL_S "teamkill"
#define SCORE_KILL_S "kill"
#define SCORE_SURVIVE_S "survive"
#define SCORE_SPRAY_S "spray"
#define SCORE_SPRAY_WRONGWALL_S "spray_wrongwall"
#define SCORE_TARGET_SCORE_S "target_score"
#define SCORE_FROZEN_S "frozen"
#define SCORE_THAWED_S "thawed"

#define SCORE_BONUS_FRAG_CARRIER_S "frag_carrier"
#define SCORE_BONUS_CARRIER_PROTECT_S "carrier_protect"
#define SCORE_BONUS_DEFENSE_S "defense"
#define SCORE_BONUS_RECOVERY_S "recovery"
#define SCORE_BONUS_CAPTURE_S "capture"
#define SCORE_BONUS_CAPTURE_TEAM_S "capture_team"
#define SCORE_BONUS_ASSIST_RETURN_S "assist_return"
#define SCORE_BONUS_ASSIST_FRAG_CARRIER_S "assist_frag_carrier"
#define SCORE_BONUS_FLAG_S "flag"
#define SCORE_BONUS_SPRAYKILLER_S "spraykiller"
#define SCORE_BONUS_SPRAYGOD_S "spraygod"

#define BOOMIES_TRIGGERBOX RANGE_BOOMIE_WIDTH // == RANGE_BOOMIE_HEIGHT

#define PLAYERINFO_TEAM "Players_Team"
#define PLAYERINFO_BOT "Players_Bot"
#define PLAYERINFO_NONE "(None)"

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1,
	ROTATOR_POS1,
	ROTATOR_POS2,
	ROTATOR_1TO2,
	ROTATOR_2TO1
} moverState_t;

#define SP_PODIUM_MODEL "models/mapobjects/podium/podium4"

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s {
	entityState_t s;  // communicated by server to clients
	entityShared_t r; // shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s *client; // NULL if not a client

	qboolean inuse;

	const char *classname; // set in QuakeEd
	int spawnflags;	 // set in QuakeEd

	qboolean neverFree; // if true, FreeEntity will only unlink
						// bodyque uses this

	int flags; // FL_* variables

	char *model;
	char *model2;
	int freetime; // level.time when the object was freed

	int eventTime; // events will be cleared EVENT_VALID_MSEC after set
	qboolean freeAfterEvent;
	qboolean unlinkAfterEvent;

	qboolean physicsObject; // if true, it can be pushed by movers and fall off edges
							// all game items are physicsObjects,
	float physicsBounce;	// 1.0 = continuous bounce, 0.0 = no bounce
	int clipmask;			// brushes with this content value will be collided against
							// when moving.  items and corpses do not collide against
							// players, for instance

	// movers
	moverState_t moverState;
	int soundPos1;
	int sound1to2;
	int sound2to1;
	int soundPos2;
	int soundLoop;
	gentity_t *parent;
	gentity_t *nextTrain;
	gentity_t *prevTrain;
	vec3_t pos1, pos2;
	float distance;

	char *message;

	int timestamp; // body queue sinking, etc

	int teamMask;
	int teamTime[2];

	char *target;
	char *targetname;
	char *team;
	char *targetShaderName;
	char *targetShaderNewName;
	gentity_t *target_ent;

	float speed;
	vec3_t movedir;

	int nextthink;
	void (*think)(gentity_t *self);
	void (*reached)(gentity_t *self); // movers call this when hitting endpoint
	void (*blocked)(gentity_t *self, gentity_t *other);
	void (*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void (*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void (*pain)(gentity_t *self, gentity_t *attacker, int damage);
	void (*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);

	int pain_debounce_time;
	int fly_sound_debounce_time; // wind tunnel
	int last_move_time;

	int health;

	qboolean takedamage;

	int damage;
	int splashDamage; // quad will increase this without increasing radius
	int splashRadius;
	int methodOfDeath;
	int splashMethodOfDeath;

	int count;

	gentity_t *chain;
	gentity_t *enemy;
	gentity_t *activator;
	gentity_t *teamchain;  // next entity in team
	gentity_t *teammaster; // master of the team

	int watertype;
	int waterlevel;

	int noise_index;

	// timing variables
	float wait;
	float random;

	// some infos for animation of SP_misc_externalmodel
	int animationStart;
	int animationEnd;
	float animationFPS;

	gitem_t *item; // for bonus items
};

typedef enum { CON_DISCONNECTED, CON_CONNECTING, CON_CONNECTED } clientConnected_t;

typedef enum { SPECTATOR_NOT, SPECTATOR_FREE, SPECTATOR_FOLLOW, SPECTATOR_SCOREBOARD } spectatorState_t;

typedef enum {
	TEAM_BEGIN, // Beginning a team game, spawn at base
	TEAM_ACTIVE // Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t state;

	int location;

	int captures;
	int basedefense;
	int carrierdefense;
	int flagrecovery;
	int fragcarrier;
	int assists;

	float lasthurtcarrier;
	float lastreturnedflag;
	float flagsince;
	float lastfraggedcarrier;
} playerTeamState_t;

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t sessionTeam;
	int spectatorNum; // for determining next-in-line to play
	spectatorState_t spectatorState;
	int spectatorClient; // for chasecam and follow mode
	int wins, losses;	 // tournament stats
	qboolean teamLeader; // true when this client is a team leader

	int livesleft; // for LPS
	char selectedlogo[32];
} clientSession_t;

//
#define MAX_NETNAME 36
#define MAX_VOTE_COUNT 3

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t connected;
	usercmd_t cmd;				// we would lose angles if not persistant
	qboolean localClient;		// true if "ip" info key is "localhost"
	qboolean initialSpawn;		// the first spawn should be at a cool location
	qboolean predictItemPickup; // based on cg_predictItems userinfo
	qboolean pmoveFixed;		//
	char netname[MAX_NETNAME];
	int maxHealth;				 // for handicapping
	int enterTime;				 // level.time the client entered the game
	playerTeamState_t teamState; // status in teamplay games
	int voteCount;				 // to prevent people from constantly calling votes
	int teamVoteCount;			 // to prevent people from constantly calling votes
	qboolean teamInfo;			 // send team overlay updates?
	qboolean frozen;
	qboolean ftLateJoin;
} clientPersistant_t;

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t ps; // communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t pers;
	clientSession_t sess;

	qboolean readyToExit; // wishes to leave the intermission

	qboolean noclip;

	qboolean readyToFight;
	int last_slickent_touch; // note last touch time ... to control deactivation
	int lastOwnCartMSGtime;

	int sprayroomleavetime;	 // when the player have to leave the room
	int sprayroomsoundflags; // 1 -> TenSecondsTo..., 2 -> CountDown
	int last_nonspray_weapon;
	int logoasktime;

	int lastBoasterHitTime;
	gentity_t *lastBoasterHitAttacker;
	int nextBoasterDoTTick;

	int balloonTime;
	gentity_t *balloonEnt;

	int lastCmdTime; // level.time of last usercmd_t, for EF_CONNECTION
					 // we can't just use pers.lastCommand.time, because
					 // of the g_sycronousclients case
	int buttons;
	int oldbuttons;
	int latched_buttons;

	vec3_t oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_armor;		   // damage absorbed by armor
	int damage_blood;		   // damage taken out of health
	int damage_knockback;	   // impact damage
	vec3_t damage_from;		   // origin for vector calculation
	qboolean damage_fromWorld; // if true, don't use the damage_from vector

	int accurateCount; // for "impressive" reward sound

	int accuracy_shots; // total number of shots
	int accuracy_hits;	// total number of hits

	int logocounter;   // for spraygod and spraykiller
	int lastDeathTime; // for sorting in lps

	//
	int lastkilled_client; // last client that this client killed
	int lasthurt_client;   // last client that damaged this client
	int lasthurt_mod;	   // type of damage the client did

	// timers
	int respawnTime;			// can respawn when time > this, force after g_forcerespwan
	int inactivityTime;			// kick players when time > this
	qboolean inactivityWarning; // qtrue if the five seoond warning has been given
	int rewardTime;				// clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int airOutTime;

	int lastKillTime; // for multiple kill rewards

	qboolean fireHeld; // used for hook
	gentity_t *hook;   // grapple hook if out

	int switchTeamTime; // time the player switched teams

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int timeResidual;

	char *areabits;

	int lastSentFlying; // last client that sent the player flying
	int lastSentFlyingTime;

	int dropTime;

	int powerupsBackpack[MAX_POWERUPS];
	qboolean frozen;
	vec3_t freezePos;
	int freezeTime;
	int freezeCount;
	int lastProgressTime;
};

//
// this structure is cleared as each map is entered
//
#define MAX_SPAWN_VARS 64
#define MAX_SPAWN_VARS_CHARS 4096

typedef struct {
	struct gclient_s *clients; // [maxclients]

	struct gentity_s *gentities;
	int gentitySize;
	int num_entities; // MAX_CLIENTS <= num_entities <= ENTITYNUM_MAX_NORMAL

	int warmupTime; // restart match at this time

	gentity_t *sr_tele; // for bot-code
	gentity_t *rspraywall, *bspraywall, *nspraywall;
	gentity_t *sr_tl_tele; // sprayroom timelimit tele ;)
	gentity_t *sr_teleout; // outtele
	int maxsprayroomtime;
	// HERBY: used for BigBalloon gametype
	int numBalloons;
	char balloonState[MAX_BALLOONS + 1];

	fileHandle_t logFile;

	// store latched cvars here that we want to get at often
	int maxclients;

	int framenum;
	int time;		  // in msec
	int previousTime; // so movers can back up when blocked

	int startTime; // level.time the map was started

	int teamScores[TEAM_NUM_TEAMS];
	int lastTeamLocationTime; // last time of client team location update

	qboolean newSession; // don't use any old session data, because
						 // we changed gametype

	qboolean restarted; // waiting for a map_restart to fire

	int numConnectedClients;
	int numNonSpectatorClients;		// includes connecting clients
	int numPlayingClients;			// connected, non-spectators
	int sortedClients[MAX_CLIENTS]; // sorted by score
	int follow1, follow2;			// clientNums for auto-follow spectators

	int snd_fry; // sound index for standing in lava

	int warmupModificationCount; // for detecting if g_warmup is changed

	// voting state
	char voteString[MAX_STRING_CHARS];
	char voteDisplayString[MAX_STRING_CHARS];
	int voteTime;		 // level.time vote was called
	int voteExecuteTime; // time the vote is executed
	int voteYes;
	int voteNo;
	int numVotingClients; // set by CalculateRanks

	// team voting state
	char teamVoteString[2][MAX_STRING_CHARS];
	int teamVoteTime[2]; // level.time vote was called
	int teamVoteYes[2];
	int teamVoteNo[2];
	int numteamVotingClients[2]; // set by CalculateRanks

	// spawn variables
	qboolean spawning; // the G_Spawn*() functions are valid
	int numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2]; // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int intermissionQueued; // intermission was qualified, but
							// wait INTERMISSION_DELAY_TIME before
							// actually going there so the last
							// frag can be watched.  Disable future
							// kills during this delay
	int intermissiontime;	// time the intermission was started
	char *changemap;
	qboolean readyToExit; // at least one client wants to exit
	int exitTime;
	vec3_t intermission_origin; // also used for spectator spawns
	vec3_t intermission_angle;

	qboolean locationLinked; // target_locations get linked
	gentity_t *locationHead; // head of the location list
	int bodyQueIndex;		 // dead bodies
	gentity_t *bodyQue[BODY_QUEUE_SIZE];
	qboolean cammode;
	vec3_t cam_spawnpos;
	vec3_t cam_spawnangles;
	int numBambams[TEAM_NUM_TEAMS];
	int numBoomies[TEAM_NUM_TEAMS];

	// freezetag
	int ftWeaponSet; // weapon set handed out at client spawn
	int ftWeaponSetAmmo[WP_NUM_WEAPONS];
	float ftWeaponSetAmmoRatio; // ratio of ammo pickup to initial fill

	int ftNumRoundsPlayed;
	int ftNumRoundsWon[TEAM_NUM_TEAMS];

	qboolean allRoundsPlayed;
} level_locals_t;

//
// g_spawn.c
//
qboolean G_SpawnString(const char *key, const char *defaultString, const char **out);
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean G_SpawnFloat(const char *key, const char *defaultString, float *out);
qboolean G_SpawnInt(const char *key, const char *defaultString, int *out);
qboolean G_SpawnVector(const char *key, const char *defaultString, float *out);
void G_SpawnEntitiesFromString(void);
char *G_NewString(const char *string);

//
// g_cmds.c
//
void Cmd_Score_f(gentity_t *ent);
void StopFollowing(gentity_t *ent);
void BroadcastTeamChange(gclient_t *client, int oldTeam);
void SetTeam(gentity_t *ent, const char *s);
void Cmd_FollowCycle_f(gentity_t *ent, int dir);
void Cmd_DropCartridge_f(gentity_t *ent);
const char *ConcatArgs(int start);
void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText);

//
// g_items.c
//
void G_CheckTeamItems(void);
void G_RunItem(gentity_t *ent);
void RespawnItem(gentity_t *ent);

void UseHoldableItem(gentity_t *ent);
void PrecacheItem(gitem_t *it);
gentity_t *Drop_Item(gentity_t *ent, gitem_t *item, float angle);
gentity_t *LaunchItem(gitem_t *item, vec3_t origin, vec3_t velocity);
void SetRespawn(gentity_t *ent, float delay);
void G_SpawnItem(gentity_t *ent, gitem_t *item);
void FinishSpawningItem(gentity_t *ent);
void Think_Weapon(gentity_t *ent);
int ArmorIndex(gentity_t *ent);
void Add_Ammo(gentity_t *ent, int weapon, int count);
void Touch_Item(gentity_t *ent, gentity_t *other, trace_t *trace);

void ClearRegisteredItems(void);
void RegisterItem(gitem_t *item);
void SaveRegisteredItems(void);

//
// g_utils.c
//
int G_ModelIndex(const char *name);
int G_SoundIndex(const char *name);
void G_TeamCommand(team_t team, char *cmd);
void G_KillBox(gentity_t *ent);
gentity_t *G_Find(gentity_t *from, int fieldofs, const char *match);
gentity_t *G_PickTarget(char *targetname);
void G_UseTargets(gentity_t *ent, gentity_t *activator);
void G_SetMovedir(vec3_t angles, vec3_t movedir);

void G_InitGentity(gentity_t *e);
gentity_t *G_Spawn(void);
gentity_t *G_TempEntity(vec3_t origin, int event);
void G_Sound(gentity_t *ent, int channel, int soundIndex);
void G_FreeEntity(gentity_t *e);
qboolean G_EntitiesFree(void);

void G_TouchTriggers(gentity_t *ent);

float *tv(float x, float y, float z);
char *vtos(const vec3_t v);

float vectoyaw(const vec3_t vec);

void G_AddPredictableEvent(gentity_t *ent, int event, int eventParm);
void G_AddEvent(gentity_t *ent, int event, int eventParm);
void G_SetOrigin(gentity_t *ent, vec3_t origin);
void AddRemap(const char *oldShader, const char *newShader, float timeOffset);
const char *BuildShaderStateConfig(void);

int DebugLine(vec3_t start, vec3_t end, int color);
void DebugLineDouble(vec3_t start, vec3_t end, int color);
void DeleteDebugLines(void);

void G_BackupPowerups(gclient_t *cl);
void G_RestorePowerups(gclient_t *cl);

// Simply matches EF_AWARD_ flags in bg_public.h
typedef enum {
	AWARD_EXCELLENT = EF_AWARD_EXCELLENT,
	AWARD_GAUNTLET = EF_AWARD_GAUNTLET,
	AWARD_CAP = EF_AWARD_CAP,
	AWARD_IMPRESSIVE = EF_AWARD_IMPRESSIVE,
	AWARD_DEFEND = EF_AWARD_DEFEND,
	AWARD_ASSIST = EF_AWARD_ASSIST,
	AWARD_DENIED = EF_AWARD_DENIED,
	AWARD_SPRAYGOD = EF_AWARD_SPRAYGOD,
	AWARD_SPRAYKILLER = EF_AWARD_SPRAYKILLER
} award_t;

void SetAward(gclient_t *client, award_t award);

void RemoveOwnedItems(gentity_t *client);

const char *GametypeName(gametype_t gametype);
const char *GametypeNameShort(gametype_t gametype);

//
// g_combat.c
//
qboolean CanDamage(gentity_t *targ, vec3_t origin);
void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage,
			  int dflags, int mod);
qboolean G_RadiusDamage(vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod);
int G_InvulnerabilityEffect(gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir);
void body_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath);
void TossClientItems(gentity_t *self);
void TossClientCubes(gentity_t *self);

void BerserkerCheck(gentity_t *ent);

// damage flags
#define DAMAGE_RADIUS 0x00000001		// damage was indirect
#define DAMAGE_NO_ARMOR 0x00000002		// armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK 0x00000004	// do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION 0x00000008 // armor, shields, invulnerability, and godmode have no effect

//
// g_missile.c
//
void G_RunMissile(gentity_t *ent);

void G_RunExplosion(gentity_t *ent);

gentity_t *fire_boaster(gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_nipper(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_splasher(gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *explode_imperius(gentity_t *self);

gentity_t *fire_duck(gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_bambamMissile(gentity_t *self, vec3_t start, vec3_t dir, float velocity);

gentity_t *fire_bubbleg(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_grenade(gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_rocket(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_bfg(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_grapple(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_kma(gentity_t *self, vec3_t start, vec3_t dir);

//
// g_mover.c
//
void G_RunMover(gentity_t *ent);
void Touch_DoorTrigger(gentity_t *ent, gentity_t *other, trace_t *trace);

//
// g_misc.c
//
void TeleportPlayer(gentity_t *player, vec3_t origin, vec3_t angles);

//
// g_weapon.c
//
qboolean LogAccuracyHit(gentity_t *target, gentity_t *attacker);
void CalcMuzzlePoint(gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint);
void SnapVectorTowards(vec3_t v, vec3_t to);
qboolean CheckGauntletAttack(gentity_t *ent);
void Weapon_HookFree(gentity_t *ent);
void Weapon_HookThink(gentity_t *ent);

//
// g_client.c
//
int TeamCount(int ignoreClientNum, int team);
int TeamLeader(int team);
team_t PickTeam(int ignoreClientNum);
void SetClientViewAngle(gentity_t *ent, vec3_t angle);
gentity_t *SelectSpawnPoint(const vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot);
void CopyToBodyQue(gentity_t *ent);
void respawn(gentity_t *ent);
void BeginIntermission(void);
void InitBodyQue(void);
void ClientSpawn(gentity_t *ent);
void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
void AddScore(gentity_t *ent, vec3_t origin, int score, char *reason);
void CalculateRanks(void);
qboolean SpotWouldTelefrag(gentity_t *spot);

gentity_t *SelectSpectatorSpawnPoint(vec3_t origin, vec3_t angles);

qboolean GetASpawnPosition(vec3_t currentPos, vec3_t output);

//
// g_svcmds.c
//
qboolean ConsoleCommand(void);
void G_ProcessIPBans(void);
qboolean G_FilterPacket(const char *from);
int convertGTStringToGTNumber(const char *argStr);

//
// g_weapon.c
//
void FireWeapon(gentity_t *ent);

//
// g_cmds.c
//
void DeathmatchScoreboardMessage(const gentity_t *ent);

//
// g_main.c
//
void MoveClientToIntermission(gentity_t *ent);
int G_GetCvarInt(const char *cvar);
void FindIntermissionPoint(void);
void SetLeader(int team, int client);
void CheckTeamLeader(int team);
void G_RunThink(gentity_t *ent);
void AddTournamentQueue(gclient_t *client);
void QDECL G_LogPrintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void SendScoreboardMessageToAllClients(void);
void QDECL G_Printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void QDECL G_Error(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));

//
// g_client.c
//
char *ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
void ClientUserinfoChanged(int clientNum);
void ClientDisconnect(int clientNum);
void ClientBegin(int clientNum);
void ClientCommand(int clientNum);

//
// g_active.c
//
void ClientThink(int clientNum);
void ClientEndFrame(gentity_t *ent);
void G_RunClient(gentity_t *ent);

//
// g_team.c
//
qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2);
void Team_CheckDroppedItem(gentity_t *dropped);
int Team_GetFlagStatus(int team);
qboolean CheckObeliskAttack(gentity_t *obelisk, gentity_t *attacker);
__attribute__((format(printf, 2, 3))) void QDECL PrintMsg(gentity_t *ent, const char *fmt, ...);

//
// g_mem.c
//
void *G_Alloc(int size);
void G_InitMemory(void);
void Svcmd_GameMem_f(void);

//
// g_session.c
//
void G_ReadSessionData(gclient_t *client);
void G_InitSessionData(gclient_t *client, char *userinfo);

void G_InitWorldSession(void);
void G_WriteSessionData(void);

//
//	g_freezetag
//
qboolean G_FreezeTag(void);
void FT_InitFreezeTag(void);
void FT_CalculateRoundScores(void);
qboolean FT_PlayerIsFrozen(gentity_t *player);
void FT_FreezePlayer(gentity_t *player, gentity_t *other);
void FT_ThawPlayer(gentity_t *player, gentity_t *other);
gentity_t *FT_NearestFrozenPlayer(gentity_t *player);
qboolean FT_InThawingRange(const gentity_t *player, const gentity_t *other);
void FT_ProgressThawing(gentity_t *player, gentity_t *thawer);
qboolean FT_WholeTeamIsFrozen(int team);
void FT_AddStartWeapons(gclient_t *client);
qboolean FT_MatchInProgress(void);
qboolean FT_CanSwitchTeam(gentity_t *player, int team);
qboolean FT_ClientIsFrozen(const gclient_t *client);
void FT_RelocateToNearestSpawnPoint(gentity_t *player);
void FT_AddAmmo(gentity_t *player);

//
// g_arenas.c
//
void UpdateTournamentInfo(void);
void SpawnModelsOnVictoryPads(void);
void Svcmd_AbortPodium_f(void);

//
// g_modifiers.c
//
void Instagib_applyWeaponJumpKnockback(vec3_t origin, gentity_t *ent, int mod);
int Instagib_getSpawnWeapon(void);
int Instagib_calculateDamage(gentity_t *target, gentity_t *inflictor, gentity_t *attacker, int damage, int dflags,
							 int mod);
qboolean Instagib_isRelevantDamageEvent(gentity_t *target, gentity_t *inflictor, gentity_t *attacker, int damage,
										int dflags, int mod);
qboolean Instagib_canSpawnEntity(gentity_t *ent);

//
// g_ctlitems.c
//
qboolean IsItemSameTeam(gentity_t *item, gentity_t *player);
qboolean bambam_createByPlayer(gentity_t *pEnt, char *pickupName);
qboolean boomies_createByPlayer(gentity_t *pEnt, char *pickupName);
qboolean CantDamageTeamitem(gentity_t *target, gentity_t *attacker);

//
// g_bot.c
//
void G_LoadBots(void);
const char *G_GetBotInfoByNumber(int num);
const char *G_GetBotInfoByName(const char *name);
void G_CheckBotSpawn(void);
void G_RemoveQueuedBotBegin(int clientNum);
qboolean G_BotConnect(int clientNum, qboolean restart);
void Svcmd_AddBot_f(void);
void Svcmd_BotList_f(void);
void BotInterbreedEndMatch(void);

// ai_main.c
#define MAX_FILEPATH 144
void BotCamMode(char *botname);
void BotCamMoveTo(int id, vec3_t pos);
void BotCamTaunt(int id);
void BotCamFire(int id);
void BotChooseWeap(int id, int weap);
void BotCamViewEntitiy(int id, int otherId);
void BotCamViewTarget(int id, vec3_t pos);
void BotCamViewangles(int id, vec3_t angles);
void AI_AddBoomie(gentity_t *pEnt1, gentity_t *pEnt2);
void AI_RemoveBoomie(gentity_t *pEnt);
void AI_AddBambam(gentity_t *pEnt1, gentity_t *pEnt2);
void AI_RemoveBambam(gentity_t *pEnt);

// bot settings
typedef struct bot_settings_s {
	char characterfile[MAX_FILEPATH];
	float skill;
	char team[MAX_FILEPATH];
} bot_settings_t;

int BotAISetup(int restart);
int BotAIShutdown(int restart);
int BotAILoadMap(int restart);
int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart);
int BotAIShutdownClient(int client, qboolean restart);
int BotAIStartFrame(int time);
void BotTestAAS(vec3_t origin);

// g_gameinfo.c
void G_InitGameinfo(void);
const char *G_GetArenaInfoByMap(const char *map);
qboolean MapSupportsGametype(const char *mapname, gametype_t gametype);

#include "g_team.h" // teamplay specific stuff

extern level_locals_t level;
extern gentity_t g_entities[MAX_GENTITIES];

#define FOFS(x) ((size_t) & (((gentity_t *)0)->x))

extern vmCvar_t g_gametype;
extern vmCvar_t g_dedicated;
extern vmCvar_t g_cheats;
extern vmCvar_t g_maxclients;	  // allow this many total, including spectators
extern vmCvar_t g_maxGameClients; // allow this many active
extern vmCvar_t g_restarted;

extern vmCvar_t g_dmflags;
extern vmCvar_t g_fraglimit;
extern vmCvar_t g_timelimit;
extern vmCvar_t g_capturelimit;
extern vmCvar_t g_friendlyFire;
extern vmCvar_t g_password;
extern vmCvar_t g_needpass;
extern vmCvar_t g_gravity;
extern vmCvar_t g_speed;
extern vmCvar_t g_knockback;
extern vmCvar_t g_quadfactor;
extern vmCvar_t g_forcerespawn;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_debugAlloc;
extern vmCvar_t g_debugDamage;
extern vmCvar_t g_weaponRespawn;
extern vmCvar_t g_weaponTeamRespawn;
extern vmCvar_t g_synchronousClients;
extern vmCvar_t g_motd;
extern vmCvar_t g_warmup;
extern vmCvar_t g_doWarmup;
extern vmCvar_t g_warmupReady;
extern vmCvar_t g_curWarmupReady;
extern vmCvar_t g_allowVote;
extern vmCvar_t g_teamAutoJoin;
extern vmCvar_t g_teamForceBalance;
extern vmCvar_t g_banIPs;
extern vmCvar_t g_filterBan;
extern vmCvar_t g_obeliskHealth;
extern vmCvar_t g_obeliskRegenPeriod;
extern vmCvar_t g_obeliskRegenAmount;
extern vmCvar_t g_obeliskRespawnDelay;
extern vmCvar_t g_cubeTimeout;
extern vmCvar_t g_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

extern vmCvar_t g_q3Items;
extern vmCvar_t g_sky;
extern vmCvar_t g_skyLensflare;
extern vmCvar_t g_LPS_startlives;
extern vmCvar_t g_LPS_flags;
extern vmCvar_t g_KillerduckHealth;
extern vmCvar_t g_transmitSVboastermissiles;

// freezetag
extern vmCvar_t g_ft_numRounds;
extern vmCvar_t g_ft_playAllRounds;
extern vmCvar_t g_ft_lateJoinTime;
extern vmCvar_t g_ft_useWeaponSet;
extern vmCvar_t g_ft_weaponSetAmmoRatio;
extern vmCvar_t g_ft_debug;

// Modifiers
extern vmCvar_t g_modInstagib;
extern vmCvar_t g_modInstagib_WeaponJump;

extern vmCvar_t g_logDamage;

void trap_Print(const char *fmt);
void trap_Error(const char *fmt) Q_NORETURN;
int trap_Milliseconds(void);
int trap_Argc(void);
void trap_Argv(int n, char *buffer, int bufferLength);
void trap_Args(char *buffer, int bufferLength);
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void trap_FS_Read(void *buffer, int len, fileHandle_t f);
void trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void trap_FS_FCloseFile(fileHandle_t f);
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
int trap_FS_Seek(fileHandle_t f, long offset, int origin); // fsOrigin_t
void trap_SendConsoleCommand(int exec_when, const char *text);
void trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags);
void trap_Cvar_Update(vmCvar_t *cvar);
void trap_Cvar_Set(const char *var_name, const char *value);
int trap_Cvar_VariableIntegerValue(const char *var_name);
float trap_Cvar_VariableValue(const char *var_name);
void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);
void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *gameClients,
						 int sizeofGameClient);
void trap_DropClient(int clientNum, const char *reason);
void trap_SendServerCommand(int clientNum, const char *text);
void trap_SetConfigstring(int num, const char *string);
void trap_GetConfigstring(int num, char *buffer, int bufferSize);
void trap_GetUserinfo(int num, char *buffer, int bufferSize);
void trap_SetUserinfo(int num, const char *buffer);
void trap_GetServerinfo(char *buffer, int bufferSize);
void trap_SetBrushModel(gentity_t *ent, const char *name);
void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
				int passEntityNum, int contentmask);
int trap_PointContents(const vec3_t point, int passEntityNum);
qboolean trap_InPVS(const vec3_t p1, const vec3_t p2);
qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2);
void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open);
qboolean trap_AreasConnected(int area1, int area2);
void trap_LinkEntity(gentity_t *ent);
void trap_UnlinkEntity(gentity_t *ent);
int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount);
qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent);
int trap_BotAllocateClient(void);
void trap_BotFreeClient(int clientNum);
void trap_GetUsercmd(int clientNum, usercmd_t *cmd);
qboolean trap_GetEntityToken(char *buffer, int bufferSize);

int trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void trap_DebugPolygonDelete(int id);
int trap_RealTime(qtime_t *qtime);

int trap_BotLibSetup(void);
int trap_BotLibShutdown(void);
int trap_BotLibVarSet(char *var_name, char *value);
int trap_BotLibVarGet(char *var_name, char *value, int size);
int trap_BotLibDefine(char *string);
int trap_BotLibStartFrame(float time);
int trap_BotLibLoadMap(const char *mapname);
int trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */ *bue);
int trap_BotLibTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);

int trap_BotGetSnapshotEntity(int clientNum, int sequence);
int trap_BotGetServerCommand(int clientNum, char *message, int size);
void trap_BotUserCommand(int client, usercmd_t *ucmd);

int trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
int trap_AAS_AreaInfo(int areanum, void /* struct aas_areainfo_s */ *info);
void trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */ *info);

int trap_AAS_Initialized(void);
void trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
float trap_AAS_Time(void);

int trap_AAS_PointAreaNum(vec3_t point);
int trap_AAS_PointReachabilityAreaIndex(vec3_t point);
int trap_AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);

int trap_AAS_PointContents(vec3_t point);
int trap_AAS_NextBSPEntity(int ent);
int trap_AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size);
int trap_AAS_VectorForBSPEpairKey(int ent, char *key, vec3_t v);
int trap_AAS_FloatForBSPEpairKey(int ent, char *key, float *value);
int trap_AAS_IntForBSPEpairKey(int ent, char *key, int *value);

int trap_AAS_AreaReachability(int areanum);

int trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags);
int trap_AAS_EnableRoutingArea(int areanum, int enable);
int trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/ *route, int areanum, vec3_t origin, int goalareanum,
						  int travelflags, int maxareas, int maxtime, int stopevent, int stopcontents, int stoptfl,
						  int stopareanum);

int trap_AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
								   void /*struct aas_altroutegoal_s*/ *altroutegoals, int maxaltroutegoals, int type);
int trap_AAS_Swimming(vec3_t origin);
int trap_AAS_PredictClientMovement(void /* aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype,
								   int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes,
								   float frametime, int stopevent, int stopareanum, int visualize);

void trap_EA_Say(int client, char *str);
void trap_EA_SayTeam(int client, char *str);
void trap_EA_Command(int client, char *command);

void trap_EA_Action(int client, int action);
void trap_EA_Gesture(int client);
void trap_EA_Talk(int client);
void trap_EA_Attack(int client);
void trap_EA_Use(int client);
void trap_EA_Respawn(int client);
void trap_EA_Crouch(int client);
void trap_EA_MoveUp(int client);
void trap_EA_MoveDown(int client);
void trap_EA_MoveForward(int client);
void trap_EA_MoveBack(int client);
void trap_EA_MoveLeft(int client);
void trap_EA_MoveRight(int client);
void trap_EA_SelectWeapon(int client, int weapon);
void trap_EA_Jump(int client);
void trap_EA_DelayedJump(int client);
void trap_EA_Move(int client, vec3_t dir, float speed);
void trap_EA_View(int client, vec3_t viewangles);

void trap_EA_EndRegular(int client, float thinktime);
void trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */ *input);
void trap_EA_ResetInput(int client);

int trap_BotLoadCharacter(char *charfile, float skill);
void trap_BotFreeCharacter(int character);
float trap_Characteristic_Float(int character, int index);
float trap_Characteristic_BFloat(int character, int index, float min, float max);
int trap_Characteristic_Integer(int character, int index);
int trap_Characteristic_BInteger(int character, int index, int min, int max);
void trap_Characteristic_String(int character, int index, char *buf, int size);

int trap_BotAllocChatState(void);
void trap_BotFreeChatState(int handle);
void trap_BotQueueConsoleMessage(int chatstate, int type, const char *message);
void trap_BotRemoveConsoleMessage(int chatstate, int handle);
int trap_BotNextConsoleMessage(int chatstate, void /* struct bot_consolemessage_s */ *cm);
int trap_BotNumConsoleMessages(int chatstate);
void trap_BotInitialChat(int chatstate, const char *type, int mcontext, char *var0, char *var1, char *var2, char *var3,
						 char *var4, char *var5, char *var6, char *var7);
int trap_BotNumInitialChats(int chatstate, const char *type);
int trap_BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2,
					  char *var3, char *var4, char *var5, char *var6, char *var7);
int trap_BotChatLength(int chatstate);
void trap_BotEnterChat(int chatstate, int client, int sendto);
void trap_BotGetChatMessage(int chatstate, char *buf, int size);
int trap_StringContains(const char *str1, const char *str2, int casesensitive);
int trap_BotFindMatch(const char *str, void /* struct bot_match_s */ *match, unsigned long int context);
void trap_BotMatchVariable(void /* struct bot_match_s */ *match, int variable, char *buf, int size);
void trap_UnifyWhiteSpaces(char *string);
void trap_BotReplaceSynonyms(char *string, unsigned long int context);
int trap_BotLoadChatFile(int chatstate, char *chatfile, char *chatname);
void trap_BotSetChatGender(int chatstate, int gender);
void trap_BotSetChatName(int chatstate, const char *name, int client);
void trap_BotResetGoalState(int goalstate);
void trap_BotRemoveFromAvoidGoals(int goalstate, int number);
void trap_BotResetAvoidGoals(int goalstate);
void trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */ *goal);
void trap_BotPopGoal(int goalstate);
void trap_BotEmptyGoalStack(int goalstate);
void trap_BotDumpAvoidGoals(int goalstate);
void trap_BotDumpGoalStack(int goalstate);
void trap_BotGoalName(int number, char *name, int size);
int trap_BotGetTopGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int trap_BotGetSecondGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int trap_BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags);
int trap_BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags,
						  void /* struct bot_goal_s */ *ltg, float maxtime);
int trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */ *goal);
int trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal);
int trap_BotGetNextCampSpotGoal(int num, void /* struct bot_goal_s */ *goal);
int trap_BotGetMapLocationGoal(char *name, void /* struct bot_goal_s */ *goal);
int trap_BotGetLevelItemGoal(int index, const char *classname, void /* struct bot_goal_s */ *goal);
float trap_BotAvoidGoalTime(int goalstate, int number);
void trap_BotSetAvoidGoalTime(int goalstate, int number, float avoidtime);
void trap_BotInitLevelItems(void);
void trap_BotUpdateEntityItems(void);
int trap_BotLoadItemWeights(int goalstate, char *filename);
void trap_BotFreeItemWeights(int goalstate);
void trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
void trap_BotSaveGoalFuzzyLogic(int goalstate, char *filename);
void trap_BotMutateGoalFuzzyLogic(int goalstate, float range);
int trap_BotAllocGoalState(int state);
void trap_BotFreeGoalState(int handle);

void trap_BotResetMoveState(int movestate);
void trap_BotMoveToGoal(void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal,
						int travelflags);
int trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type);
void trap_BotResetAvoidReach(int movestate);
void trap_BotResetLastAvoidReach(int movestate);
int trap_BotReachabilityArea(vec3_t origin, int testground);
int trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead,
							   vec3_t target);
int trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags,
								   vec3_t target);
int trap_BotAllocMoveState(void);
void trap_BotFreeMoveState(int handle);
void trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */ *initmove);
void trap_BotAddAvoidSpot(int movestate, const vec3_t origin, float radius, int type);

int trap_BotChooseBestFightWeapon(int weaponstate, int *inventory);
void trap_BotGetWeaponInfo(int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo);
int trap_BotLoadWeaponWeights(int weaponstate, const char *filename);
int trap_BotAllocWeaponState(void);
void trap_BotFreeWeaponState(int weaponstate);
void trap_BotResetWeaponState(int weaponstate);

int trap_GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child);

void trap_SnapVector(float *v);
int trap_AAS_BestReachableArea(vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin);
