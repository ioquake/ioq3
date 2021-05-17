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
// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#include "bg_game.h"

//(original)#define	GAME_VERSION		BASEGAME "-1"
#define GAME_VERSION "padcode 1.153"

#define DEFAULT_GRAVITY 800

#define DEFAULT_GSPEED_STR "280"

//(original)#define	GIB_HEALTH			-40

#define LOW_GRAVITY 400
#define GIB_HEALTH -200

#define ARMOR_PROTECTION 0.66

#define MAX_ITEMS 256

#define RANK_TIED_FLAG 0x4000

#define BALLOONY_SIZE 8
#define MAX_BALLOONS 3

#define ITEM_RADIUS 15 // item sizes are needed for client side pickup detection

#define LIGHTNING_RANGE 768

#define SCORE_NOT_PRESENT -9999 // for the CS_SCORES[12] when only one player is present

#define VOTE_TIME 30000 // 30 seconds before vote times out

#define MINS_Z -24
#define DEFAULT_VIEWHEIGHT 26
#define CROUCH_VIEWHEIGHT 12
#define DEAD_VIEWHEIGHT -16

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define CS_MUSIC 2
#define CS_MESSAGE 3 // from the map worldspawn's message field
#define CS_MOTD 4	 // g_motd string for server message of the day
#define CS_WARMUP 5	 // server time when the match will be restarted
#define CS_SCORES1 6
#define CS_SCORES2 7
#define CS_VOTE_TIME 8
#define CS_VOTE_STRING 9
#define CS_VOTE_YES 10
#define CS_VOTE_NO 11

#define CS_TEAMVOTE_TIME 12
#define CS_TEAMVOTE_STRING 14
#define CS_TEAMVOTE_YES 16
#define CS_TEAMVOTE_NO 18

#define CS_GAME_VERSION 20
#define CS_LEVEL_START_TIME 21 // so the timer only shows the current level
#define CS_INTERMISSION 22 // when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS 23   // string indicating flag status in CTF
#define CS_SHADERSTATE 24
#define CS_BOTINFO 25

#define CS_BALLOONS 26

#define CS_ITEMS 27 // string of 0's and 1's that tell which items are present

#define CS_MODELS 32
#define CS_SOUNDS (CS_MODELS + MAX_MODELS)
#define CS_PLAYERS (CS_SOUNDS + MAX_SOUNDS)
#define CS_LOCATIONS (CS_PLAYERS + MAX_CLIENTS)
#define CS_PARTICLES (CS_LOCATIONS + MAX_LOCATIONS)

#define CS_MAX (CS_PARTICLES + MAX_LOCATIONS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum {
	GT_FFA,			  // free for all
	GT_TOURNAMENT,	  // one on one tournament
	GT_SINGLE_PLAYER, // single player ffa

	GT_SPRAYFFA, // spray logo ffa GT
	GT_LPS,

	//-- team games go after this --

	GT_TEAM, // team deathmatch
	GT_FREEZETAG,
	GT_CTF,	 // capture the flag

	GT_SPRAY, // spray logo GT

	GT_BALLOON, // Big Balloon

	GT_MAX_GAME_TYPE
} gametype_t;

// NOTE: Make sure those cover all gametypes!
#define GTN__GT_FFA "Free For All"
#define GTN__GT_TOURNAMENT "Tournament"
#define GTN__GT_SINGLE_PLAYER "Singleplayer"
#define GTN__GT_SPRAYFFA "Spray Your Color"
#define GTN__GT_LPS "Last Pad Standing"
#define GTN__GT_TEAM "Free For All Team"
#define GTN__GT_CTF "Capture The Lolly"
#define GTN__GT_SPRAY "Spray Your Color Team"
#define GTN__GT_BALLOON "Big Balloon"
#define GTN__GT_FREEZETAG "Freeze Tag"
#define GTN__GT_MAX_GAME_TYPE "Unknown"

#define GTN_S__GT_FFA "FFA"
#define GTN_S__GT_TOURNAMENT "1VS1"
#define GTN_S__GT_SINGLE_PLAYER "SP"
#define GTN_S__GT_SPRAYFFA "SYC"
#define GTN_S__GT_LPS "LPS"
#define GTN_S__GT_TEAM "TFFA"
#define GTN_S__GT_CTF "CTL"
#define GTN_S__GT_SPRAY "TSYC"
#define GTN_S__GT_BALLOON "BB"
#define GTN_S__GT_FREEZETAG "FT"
#define GTN_S__GT_MAX_GAME_TYPE "?"

#define GAMETYPE_NAME(gametype) GTN__##gametype
#define GAMETYPE_NAME_SHORT(gametype) GTN_S__##gametype

int convertGTStringToGTNumber(const char *argStr);

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		  // can accelerate and turn
	PM_NOCLIP,		  // noclip movement
	PM_SPECTATOR,	  // still run into walls
	PM_DEAD,		  // no acceleration or turning, but free falling
	PM_FREEZE,		  // stuck in place with no control
	PM_FROZEN,		  // freezetag
	PM_INTERMISSION,  // no movement or status bar
	PM_SPINTERMISSION // no movement or status bar
} pmtype_t;

typedef enum {
	WEAPON_READY,
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING

	,
	WEAPON_CHARGING // "loading" an imperius-shoot
} weaponstate_t;

// pmove->pm_flags
#define PMF_DUCKED 1
#define PMF_JUMP_HELD 2
#define PMF_BACKWARDS_JUMP 8   // go into backwards land
#define PMF_BACKWARDS_RUN 16   // coast down to backwards run
#define PMF_TIME_LAND 32	   // pm_time is time before rejump
#define PMF_TIME_KNOCKBACK 64  // pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP 256 // pm_time is waterjump
#define PMF_RESPAWNED 512	   // clear after attack and jump buttons come up
#define PMF_USE_ITEM_HELD 1024
#define PMF_GRAPPLE_PULL 2048 // pull towards grapple location
#define PMF_FOLLOW 4096		  // spectate following another player
#define PMF_SCOREBOARD 8192	  // spectate as a scoreboard
#define PMF_INVULEXPAND 16384 // invulnerability sphere set to full size

#define PMF_TOUCHSLICKENT 0x08000 // touched a slick-entity (spawned by boaster hitting the ground)

#define PMF_ALL_TIMES (PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_KNOCKBACK)

#define MAXTOUCH 32
typedef struct {
	// state (in / out)
	playerState_t *ps;

	// command (in)
	usercmd_t cmd;
	int tracemask;		  // collide against these types of surfaces
	int debugLevel;		  // if set, diagnostic output will be printed
	qboolean noFootsteps; // if the game is setup for no footsteps by the server
	qboolean gauntletHit; // true if a gauntlet attack would actually hit something

	int framecount;

	int gametype;

	// results (out)
	int numtouch;
	int touchents[MAXTOUCH];

	vec3_t mins, maxs; // bounding box size

	int watertype;
	int waterlevel;

	float xyspeed;

	// for fixed msec Pmove
	int pmove_fixed;
	int pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void (*trace)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
				  int passEntityNum, int contentMask);
	int (*pointcontents)(const vec3_t point, int passEntityNum);
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles(playerState_t *ps, const usercmd_t *cmd);
void Pmove(pmove_t *pmove);

//===================================================================================

// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
	STAT_HEALTH,
	STAT_HOLDABLE_ITEM,
	STAT_WEAPONS, // 16 bit fields
	STAT_ARMOR,
	STAT_DEAD_YAW,		// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY, // bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH,	// health / armor limit, changable by handicap

	STAT_HOLDABLEVAR,	// for Floater/Killerduck "ammo"
	STAT_SPRAYROOMSECS, // time for sprayroom->killroom stuff
						// According to latin notation, "cs" can be replaced with "x" :)
	STAT_BALLOONTIME,
	STAT_FORBIDDENITEMS,

	STAT_CHILL,

	STAT_HB_EFLAGS // copy of the higher bytes of eFlags
} statIndex_t;

#define STAT_LIVESLEFT STAT_SPRAYROOMSECS // in LPS we don't need the sprayroom seconds

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE,			 // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_TEAM,			 // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,			 // total points damage inflicted so damage beeps can sound on change
	PERS_RANK,			 // player rank or team rank
	PERS_SPAWN_COUNT,	 // incremented every respawn
	PERS_PLAYEREVENTS,	 // 16 bits that can be flipped for events
	PERS_ATTACKER,		 // clientnum of last damage inflicter
	PERS_ATTACKEE_ARMOR, // health/armor of last person we attacked
	PERS_KILLED,		 // count of the number of times you died
	// player awards tracking
	PERS_IMPRESSIVE_COUNT,	  // two railgun hits in a row
	PERS_EXCELLENT_COUNT,	  // two successive kills in a short amount of time
	PERS_DEFEND_COUNT,		  // defend awards
	PERS_ASSIST_COUNT,		  // assist awards
	PERS_GAUNTLET_FRAG_COUNT, // kills with the guantlet
	PERS_CAPTURES			  // captures

	,
	PERS_SPRAYAWARDS_COUNT // 0xFF00 -> god, 0x00FF -> killer ... because there are max 16 PERS and also only 16 bit(per
						   // PERS) O_o
} persEnum_t;

// entityState_t->eFlags
#define EF_DEAD 0x00000001 // don't draw a foe marker over players with EF_DEAD

#define EF_HURT 0x00000002 // used to determine if somebody is hurt (for stars)

#define EF_TELEPORT_BIT 0x00000004	  // toggled every time the origin abruptly changes
#define EF_AWARD_EXCELLENT 0x00000008 // draw an excellent sprite
#define EF_PLAYER_EVENT 0x00000010
#define EF_BOUNCE 0x00000010		 // for missiles
#define EF_BOUNCE_HALF 0x00000020	 // for missiles
#define EF_AWARD_GAUNTLET 0x00000040 // draw a gauntlet sprite
#define EF_NODRAW 0x00000080		 // may have an event, but no model (unspawned items)
#define EF_FIRING 0x00000100		 // for lightning gun

#define EF_CHARGED 0x00000200

#define EF_MOVER_STOP 0x00000400	   // will push otherwise
#define EF_AWARD_CAP 0x00000800		   // draw the capture sprite
#define EF_TALK 0x00001000			   // draw a talk balloon
#define EF_CONNECTION 0x00002000	   // draw a connection trouble sprite
#define EF_VOTED 0x00004000			   // already cast a vote
#define EF_AWARD_IMPRESSIVE 0x00008000 // draw an impressive sprite
#define EF_AWARD_DEFEND 0x00010000	   // draw a defend sprite
#define EF_AWARD_ASSIST 0x00020000	   // draw a assist sprite
#define EF_AWARD_DENIED 0x00040000	   // denied
#define EF_TEAMVOTED 0x00080000		   // already cast a team vote

#define EF_FLOATER 0x00100000 // player uses floater
#define EF_AWARD_SPRAYGOD 0x00200000
#define EF_AWARD_SPRAYKILLER 0x00400000
#define EF_NOLIFESLEFT 0x00800000 //

#define REMOVE_AWARDFLAGS                                                                                              \
	~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND |               \
	  EF_AWARD_CAP | EF_AWARD_SPRAYGOD | EF_AWARD_SPRAYKILLER)

// NOTE: may not have more than 16
typedef enum {
	PW_NONE,

	PW_SPEEDY,
	PW_PADPOWER,
	PW_CLIMBER,
	PW_JUMPER,
	PW_VISIONLESS,
	PW_REVIVAL,
	PW_BERSERKER,

	PW_REDFLAG,
	PW_BLUEFLAG,

	PW_FREEZE,

	PW_NUM_POWERUPS

} powerup_t;

typedef enum {
	HI_NONE,

	HI_TELEPORTER,
	HI_MEDKIT,

	HI_FLOATER,
	HI_KILLERDUCKS,

	HI_BAMBAM,
	HI_BOOMIES,

	HI_NUM_HOLDABLE
} holdable_t;

typedef enum {
	BBS_INACTIVE = 0,  // throwen to the ground, not yet building
	BBS_BUILDING,	   // can't should, playing anim 1-50
	BBS_IDLE,		   // waiting for enemy, playing anim 51-85 (! set nextthink 20sek, for zzz)
	BBS_IDLE2SHOOTING, // switch to this by first enemy trigger ... switch to shooting at a delayed think
	BBS_SHOOTING, // shoots if it was triggered by a enemy, playing anim 86-105 (! set nextthink, for shooting2idle)
	BBS_SHOOTING2IDLE, // switch to this in a think after last shooting
	BBS_ZZZ,		   // after 20sec idle it will sleep for 3.76sec
} BamBam_State_e;

typedef enum {
	WP_NONE,

	WP_PUNCHY,
	WP_NIPPER,
	WP_PUMPER,
	WP_BALLOONY,
	WP_BETTY,
	WP_BOASTER,
	WP_SPLASHER,
	WP_BUBBLEG,
	WP_IMPERIUS,
	WP_KMA97, // "Kiss My Ass 1997"

	WP_GRAPPLING_HOOK,

	WP_SPRAYPISTOL,

	WP_KILLERDUCKS,	   // isn't a real weapon ... but we use it to build the KDs like missiles
	WP_BAMBAM_MISSILE, // isn't a real weapon ... but the clientside missile code is based on weaponNr

	WP_NUM_WEAPONS
} weapon_t;

// flags for g_LPS_flags for GT_LPS
typedef enum {
	LPSF_PPOINTLIMIT = 1, // Multiple map_restarts until one player won pointlimit times
						  // Why has this two P's in its name?
	LPSF_MULTIPOINTS = 2, // With LPSF_PPOINTLIMIT, everyone get's points depending on his rank
	LPSF_NOARROWS = 4	  // Don't draw Icons clientside
} lpsflag_t;

// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define PLAYEREVENT_DENIEDREWARD 0x0001
#define PLAYEREVENT_GAUNTLETREWARD 0x0002
#define PLAYEREVENT_HOLYSHIT 0x0004

// entityState_t->event values
// entity events are for effects that take place relative
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1 0x00000100
#define EV_EVENT_BIT2 0x00000200
#define EV_EVENT_BITS (EV_EVENT_BIT1 | EV_EVENT_BIT2)

#define EVENT_VALID_MSEC 300

typedef enum {
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,

	EV_JUMP_PAD, // boing sound at origin, jump sound on player

	EV_JUMP,
	EV_WATER_TOUCH, // foot touches
	EV_WATER_LEAVE, // foot leaves
	EV_WATER_UNDER, // head touches
	EV_WATER_CLEAR, // head leaves

	EV_ITEM_PICKUP,		   // normal item pickups are predictable
	EV_GLOBAL_ITEM_PICKUP, // powerup / team sounds are broadcast to everyone

	EV_NOAMMO,
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,

	EV_IMPERIUS_EXPLODE, // HERBY

	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

	EV_PLAYER_TELEPORT_RED_IN,
	EV_PLAYER_TELEPORT_RED_OUT,
	EV_PLAYER_TELEPORT_BLUE_IN,
	EV_PLAYER_TELEPORT_BLUE_OUT,

	EV_DROP_CARTRIDGE,

	EV_GRENADE_BOUNCE, // eventParm will be the soundindex

	EV_STICKY_BOUNCE, // HERBY

	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND, // no attenuation
	EV_GLOBAL_TEAM_SOUND,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_RAILTRAIL,
	EV_SHOTGUN,

	EV_SPRAYLOGO,

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY,

	EV_POWERUP_REVIVAL,
	EV_POWERUP_PADPOWER,

	EV_GIB_PLAYER, // gib a previously living player
	EV_SCOREPLUM,  // score plum

	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_TAUNT,

	EV_HEHE1,
	EV_HEHE2,

	EV_FOOTSTEP_CARPET,
	EV_FOOTSTEP_LATTICE,
	EV_FOOTSTEP_SAND,
	EV_FOOTSTEP_SOFT,
	EV_FOOTSTEP_WOOD,
	EV_FOOTSTEP_SNOW,

	EV_BAMBAM_EXPLOSION,
	EV_BOOMIES_EXPLOSION,

	EV_TAUNT_YES,
	EV_TAUNT_NO,
	EV_TAUNT_FOLLOWME,
	EV_TAUNT_GETFLAG,
	EV_TAUNT_GUARDBASE,
	EV_TAUNT_PATROL

} entity_event_t;

typedef enum {
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDOBELISK_ATTACKED,
	GTS_BLUEOBELISK_ATTACKED,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED,
	GTS_KAMIKAZE
} global_team_sound_t;

// animations
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	TORSO_SPRAYATTACK,
	TORSO_SPRAYSTAND,
	LEGS_FLYING,
	LEGS_HEAVYLAND,

	TORSO_GETFLAG,
	TORSO_GUARDBASE,
	TORSO_PATROL,
	TORSO_FOLLOWME,
	TORSO_AFFIRMATIVE,
	TORSO_NEGATIVE,

	MAX_ANIMATIONS,

	LEGS_BACKCR,
	LEGS_BACKWALK,
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,

	MAX_TOTALANIMATIONS
} animNumber_t;

typedef struct animation_s {
	int firstFrame;
	int numFrames;
	int loopFrames;	 // 0 to numFrames
	int frameLerp;	 // msec between frames
	int initialLerp; // msec to get to first frame
	int reversed;	 // true if animation is reversed
	int flipflop;	 // true if animation should flipflop back to base
} animation_t;

// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT 128

typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME 1000

// How many players on the overlay
#define TEAM_MAXOVERLAY 32

// team task
typedef enum {
	TEAMTASK_NONE,
	TEAMTASK_OFFENSE,
	TEAMTASK_DEFENSE,
	TEAMTASK_PATROL,
	TEAMTASK_FOLLOW,
	TEAMTASK_RETRIEVE,
	TEAMTASK_ESCORT,
	TEAMTASK_CAMP
} teamtask_t;

// means of death
typedef enum {
	MOD_UNKNOWN,
	MOD_PUMPER,
	MOD_PUNCHY,
	MOD_NIPPER,
	MOD_BALLOONY,
	MOD_BALLOONY_SPLASH,
	MOD_BETTY,
	MOD_BETTY_SPLASH,
	MOD_BUBBLEG,
	MOD_BUBBLEG_SPLASH, // should be unused
	MOD_SPLASHER,
	MOD_BOASTER,
	MOD_IMPERIUS,
	MOD_IMPERIUS_SPLASH,
	MOD_INJECTOR,

	MOD_KILLERDUCKS,

	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING, // should be unused
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_GRAPPLE, // should be unused

	MOD_BAMBAM,
	MOD_BOOMIES
} meansOfDeath_t;

//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,	 // EFX: rotate + upscale + minlight
	IT_AMMO,	 // EFX: rotate
	IT_ARMOR,	 // EFX: rotate + minlight
	IT_HEALTH,	 // EFX: static external sphere + rotating internal
	IT_POWERUP,	 // instant on, timer based
				 // EFX: rotate + external ring that rotates
	IT_HOLDABLE, // single use, holdable item
				 // EFX: rotate + bob
	IT_PERSISTANT_POWERUP,
	IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 4

typedef struct gitem_s {
	char *classname; // spawning name
	char *pickup_sound;
	char *world_model[MAX_ITEM_MODELS];

	char *icon;
	char *pickup_name; // for printing on pickup

	int quantity;	   // for ammo how much, or duration of powerup
	itemType_t giType; // IT_* flags

	int giTag;

	char *precaches; // string of all models and images this item will use
	char *sounds;	 // string of all sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern gitem_t bg_itemlist[];
extern int bg_numItems;

gitem_t *BG_FindItem(const char *pickupName);
gitem_t *BG_FindItemForWeapon(weapon_t weapon);
gitem_t *BG_FindItemForPowerup(powerup_t pw);
gitem_t *BG_FindItemForHoldable(holdable_t pw);
#define ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean BG_CanItemBeGrabbed(int gametype, const entityState_t *ent, const playerState_t *ps);

// g_dmflags->integer flags
#define DF_NO_FALLING 8
#define DF_FIXED_FOV 16
#define DF_NO_FOOTSTEPS 32

// content masks
#define MASK_ALL (-1)
#define MASK_SOLID (CONTENTS_SOLID)
#define MASK_PLAYERSOLID (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_BODY)
#define MASK_DEADSOLID (CONTENTS_SOLID | CONTENTS_PLAYERCLIP)
#define MASK_WATER (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME)
#define MASK_OPAQUE (CONTENTS_SOLID | CONTENTS_SLIME | CONTENTS_LAVA)
#define MASK_SHOT (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE)

//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE, // grapple hooked on wall
	ET_TEAM,

	ET_EXPLOSION,
	ET_BALLOON,

	ET_STATION, // health-station ...

	ET_BAMBAM,
	ET_BOOMIES,

	ET_EVENTS // any of the EV_* events can be added freestanding
			  // by setting eType to ET_EVENTS + eventNum
			  // this avoids having to set eFlags and eventNum
} entityType_t;

void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec3_t result);
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec3_t result);

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);

void BG_TouchJumpPad(playerState_t *ps, entityState_t *jumppad);

void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, qboolean snap);
void BG_PlayerStateToEntityStateExtraPolate(playerState_t *ps, entityState_t *s, int time, qboolean snap);

qboolean BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime);

#define ARENAS_PER_TIER 4
#define MAX_ARENAS 1024
#define MAX_ARENAS_TEXT 8192

#define MAX_BOTS 1024
#define MAX_BOTS_TEXT 8192

// Kamikaze

// 1st shockwave times
#define KAMI_SHOCKWAVE_STARTTIME 0
#define KAMI_SHOCKWAVEFADE_STARTTIME 1500
#define KAMI_SHOCKWAVE_ENDTIME 2000
// explosion/implosion times
#define KAMI_EXPLODE_STARTTIME 250
#define KAMI_IMPLODE_STARTTIME 2000
#define KAMI_IMPLODE_ENDTIME 2250
// 2nd shockwave times
#define KAMI_SHOCKWAVE2_STARTTIME 2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME 2500
#define KAMI_SHOCKWAVE2_ENDTIME 3000
// radius of the models without scaling
#define KAMI_SHOCKWAVEMODEL_RADIUS 88
#define KAMI_BOOMSPHEREMODEL_RADIUS 72
// maximum radius of the models during the effect
#define KAMI_SHOCKWAVE_MAXRADIUS 1320
#define KAMI_BOOMSPHERE_MAXRADIUS 720
#define KAMI_SHOCKWAVE2_MAXRADIUS 704

// These are used by cgame and q3_ui
#define SPRAYLOGO_PATH "spraylogos"
#define SPRAYLOGO_LIST_CVAR "logolist"
#define SPRAYLOGO_DEFAULT_NAME "01_wop"
#define MAX_SPRAYLOGOS_LOADED 64
#define MAX_SPRAYLOGO_NAME 32

enum { ICON_ARROW = 1, ICON_BALLOON = 2, ICON_TEAMMATE = 4, ICON_HEALTHSTATION = 8, ICON_SPRAYROOM = 16 };

#define ICON_ALL (ICON_ARROW | ICON_BALLOON | ICON_TEAMMATE | ICON_HEALTHSTATION | ICON_SPRAYROOM)
// Don't forget to update cg_icons in cg_main.c as well
