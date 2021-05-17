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
 * name:		ai_main.h
 *
 * desc:		WoP bot AI
 *
 * $Archive: /source/code/botai/ai_chat.c $
 *
 *****************************************************************************/

//#define DEBUG
#define CTF

#define MAX_ITEMS 256
// bot flags
#define BFL_STRAFERIGHT 1	 // strafe to the right
#define BFL_ATTACKED 2		 // bot has attacked last ai frame
#define BFL_ATTACKJUMPED 4	 // bot jumped during attack last frame
#define BFL_AIMATENEMY 8	 // bot aimed at the enemy this frame
#define BFL_AVOIDRIGHT 16	 // avoid obstacles by going to the right
#define BFL_IDEALVIEWSET 32	 // bot has ideal view angles set
#define BFL_FIGHTSUICIDAL 64 // bot is in a suicidal fight
// long term goal types
#define LTG_TEAMHELP 1		// help a team mate
#define LTG_TEAMACCOMPANY 2 // accompany a team mate
#define LTG_DEFENDKEYAREA 3 // defend a key area
#define LTG_GETFLAG 4		// get the enemy flag
#define LTG_RUSHBASE 5		// rush to the base
//#define LTG_RETURNFLAG				6	//return the flag
#define LTG_BALLCAMP 7 // camp somewhere
//#define LTG_CAMPORDER				8	//ordered to camp somewhere
//#define LTG_PATROL					9	//patrol
#define LTG_GETITEM 10 // get an item
//#define LTG_KILL					11	//kill someone
//#define LTG_HARVEST					12	//harvest skulls
#define LTG_ATTACKENEMYBASE 13 // attack the enemy base
#define LTG_GO_FOR_HEALTH 14
#define LTG_MAKELOVE_ONTOP 15
#define LTG_GIVECART 16
#define LTG_FETCHCART 17
#define LTG_JOINMATE 18
#define LTG_CAPTUREFLAG 19
#define LTG_PICKUPFLAG 20
#define LTG_PLANTBOOMIE 21
#define LTG_PLANTBAMBAM 22
#define LTG_UNFREEZE 23

// some goal dedication times

#define TEAM_BALLOONTHINK_TIME 4
#define TEAM_BALLOONCAMP_TIME 10

#define TEAM_HELP_TIME 60			// 1 minute teamplay help time
#define TEAM_ACCOMPANY_TIME 600		// 10 minutes teamplay accompany time
#define TEAM_DEFENDKEYAREA_TIME 600 // 10 minutes ctf defend base time
#define TEAM_CAMP_TIME 600			// 10 minutes camping time
#define TEAM_GETITEM_TIME 60		// 1 minute
#define SYC_CART_EXCHANGE_TIME 15
#define RUSHBASE_TIME 120		// 2 minutes
#define CTF_RUSHBASE_TIME 120	// 2 minutes ctf rush base time
#define CTF_RETURNFLAG_TIME 180 // 3 minutes to return the flag
#define TEAM_UNFREEZETIME 120

#define PUSHCART_DIST 160

// copied from the aas file header
#define PRESENCE_NONE 1
#define PRESENCE_NORMAL 2
#define PRESENCE_CROUCH 4

#define MAX_ACTIVATESTACK 8
#define MAX_ACTIVATEAREAS 32

extern vmCvar_t bot_developer;

// ai debug modes
#define AIDBG_CHAT 8
#define AIDBG_ROUTES 16
#define AIDBG_GAMETYPE 32
#define AIDBG_MOVE 64
#define AIDBG_GOAL 128
#define AIDBG_COMBAT 256
#define AIDBG_ALL 32767

typedef struct bot_activategoal_s {
	int inuse;
	bot_goal_t goal;				 // goal to activate (buttons etc.)
	float time;						 // time to activate something
	float start_time;				 // time starting to activate something
	float justused_time;			 // time the goal was used
	int shoot;						 // true if bot has to shoot to activate
	int weapon;						 // weapon to be used for activation
	vec3_t target;					 // target to shoot at to activate something
	vec3_t origin;					 // origin of the blocking entity to activate
	int areas[MAX_ACTIVATEAREAS];	 // routing areas disabled by blocking entity
	int numareas;					 // number of disabled routing areas
	int areasdisabled;				 // true if the areas are disabled for the routing
	struct bot_activategoal_s *next; // next activate goal on stack
} bot_activategoal_t;

/*
typedef struct nmyinfo_s{
int client;			// id
int	maxhealth;		// pessimistic guess, depending on painsounds
vec3_t lastpos;		// where last seen/heard
vec3_t lasttime;	// when last seen/heard
int	   flags;		// waffen, items...
}nmy_info_t;
*/

typedef enum { CWM_MOVEMENT, CWM_ENTITY, CWM_TARGET, CWM_ANGLES } cam_viewmode_t;

typedef enum { BCM_IDLE, BCM_MOVETO } cam_movement_t;

typedef enum { WPLINKRED, WPLINKBLUE, WPLINKLATERAL, WPLINKNUMTYPES } cam_link_t;

typedef struct ctf_waypoint_s ctf_waypoint_t;
typedef struct waypointlinks_s waypointlinks_t;

#define MAX_LINKSPERTYPE 5
struct waypointlinks_s {
	ctf_waypoint_t *link[MAX_LINKSPERTYPE];
	int num;
};

struct ctf_waypoint_s {
	waypointlinks_t links[WPLINKNUMTYPES];
	bot_goal_t goal;
	// int travelTime[3];
	char name[128];
};

typedef struct bambamspot_s {
	bot_goal_t goal;
	team_t team;
	gentity_t *occupied;
} bambamspot_t;

typedef struct boomiespot_s {
	bot_goal_t goal;
	vec3_t angles;
	team_t team;
	gentity_t *occupied;
} boomiespot_t;

#define MAX_WAYPOINTS 64
extern ctf_waypoint_t waypoints[MAX_WAYPOINTS];
extern int numwaypoints;

#define MAX_BAMBAMSPOTS 64
extern bambamspot_t bambamspots[MAX_BAMBAMSPOTS];
extern int numbambamspots;

#define MAX_BOOMIESPOTS 64
extern boomiespot_t boomiespots[MAX_BOOMIESPOTS];
extern int numboomiespots;

// bot state
typedef struct bot_state_s {
	int inuse;							// true if this state is used by a bot client
	int botthink_residual;				// residual for the bot thinks
	int client;							// client number of the bot
	int entitynum;						// entity number of the bot
	playerState_t cur_ps;				// current player state
	int last_eFlags;					// last ps flags
	usercmd_t lastucmd;					// usercmd from last frame
	int entityeventTime[MAX_GENTITIES]; // last entity event time
	//
	bot_settings_t settings;			   // several bot settings
	int (*ainode)(struct bot_state_s *bs); // current AI node
	float thinktime;					   // time the bot thinks this frame
	vec3_t origin;						   // origin of the bot
	vec3_t velocity;					   // velocity of the bot
	int presencetype;					   // presence type of the bot
	vec3_t eye;							   // eye coordinates of the bot
	int areanum;						   // the number of the area the bot is in
	int inventory[MAX_ITEMS];			   // string with items amounts the bot has
	int tfl;							   // the travel flags the bot uses
	int flags;							   // several flags
	int respawn_wait;					   // wait until respawned
	int lasthealth;						   // health value previous frame
	int lastkilledplayer;				   // last killed player
	int lastkilledby;					   // player that last killed this bot
	int botdeathtype;					   // the death type of the bot
	int enemydeathtype;					   // the death type of the enemy
	int botsuicide;						   // true when the bot suicides
	int enemysuicide;					   // true when the enemy of the bot suicides
	int setupcount;						   // true when the bot has just been setup
	int map_restart;					   // true when the map is being restarted
	int entergamechat;					   // true when the bot used an enter game chat
	int num_deaths;						   // number of time this bot died
	int num_kills;						   // number of kills of this bot
	int lastframe_health;				   // health value the last frame
	int lasthitcount;					   // number of hits last frame
	int chatto;							   // chat to all or team
	float walker;						   // walker charactertic
	float ltime;						   // local bot time
	float entergame_time;				   // time the bot entered the game
	float ltg_time;						   // long term goal time
	float nbg_time;						   // nearby goal time
	float respawn_time;					   // time the bot takes to respawn
	float respawnchat_time;				   // time the bot started a chat during respawn
	float chase_time;					   // time the bot will chase the enemy
	float enemyvisible_time;			   // time the enemy was last visible
	float check_time;					   // time to check for nearby items
	float stand_time;					   // time the bot is standing still
	float lastchat_time;				   // time the bot last selected a chat
	float standfindenemy_time;			   // time to find enemy while standing
	float attackstrafe_time;			   // time the bot is strafing in one dir
	float attackcrouch_time;			   // time the bot will stop crouching
	float attackchase_time;				   // time the bot chases during actual attack
	float attackjump_time;				   // time the bot jumped during attack
	float enemysight_time;				   // time before reacting to enemy
	float enemydeath_time;				   // time the enemy died
	float enemyposition_time;			   // time the position and velocity of the enemy were stored
	float defendaway_time;				   // time away while defending
	// float defendaway_range;							//max travel time away from defend area
	float rushbaseaway_time;		  // time away from rushing to the base
	float attackaway_time;			  // time away from attacking the enemy base
	float killedenemy_time;			  // time the bot killed the enemy
	float arrive_time;				  // time arrived (at companion)
	float lastair_time;				  // last time the bot had air
	float teleport_time;			  // last time the bot teleported
	float camp_time;				  // last time camped
	float weaponchange_time;		  // time the bot started changing weapons
	float firethrottlewait_time;	  // amount of time to wait
	float firethrottleshoot_time;	  // amount of time to shoot
	float notblocked_time;			  // last time the bot was not blocked
	float predictobstacles_time;	  // last time the bot predicted obstacles
	int predictobstacles_goalareanum; // last goal areanum the bot predicted obstacles for
	vec3_t aimtarget;
	vec3_t enemyvelocity; // enemy velocity 0.5 secs ago during battle
	vec3_t enemyorigin;	  // enemy origin 0.5 secs ago during battle
	//
	int character; // the bot character
	int ms;		   // move state of the bot
	int gs;		   // goal state of the bot
	int cs;		   // chat state of the bot
	int ws;		   // weapon state of the bot
	//
	int enemy;				 // enemy entity number
	int lastenemyareanum;	 // last reachability area the enemy was in
	vec3_t lastenemyorigin;	 // last origin of the enemy in the reachability area
	int weaponnum;			 // current weapon number
	vec3_t viewangles;		 // current view angles
	vec3_t ideal_viewangles; // ideal view angles
	vec3_t viewanglespeed;
	//
	int ltgtype; // long term goal type

	// team goals
	int teammate;	   // team mate involved in this team goal
	int decisionmaker; // player who decided to go for this goal
	// int ordered;									//true if ordered to do something
	// float order_time;								//time ordered to do something
	// int owndecision_time;							//time the bot made it's own decision
	bot_goal_t teamgoal;		// the team goal
	float teammessage_time;		// time to message team mates what the bot is doing
	float teamgoal_time;		// time to stop helping team mate
	float teammatevisible_time; // last time the team mate was NOT visible

	// last ordered team goal
	// int lastgoal_decisionmaker;
	// int lastgoal_ltgtype;
	// int lastgoal_teammate;
	// bot_goal_t lastgoal_teamgoal;

	// for leading team mates
	// int lead_teammate;								//team mate the bot is leading
	// bot_goal_t lead_teamgoal;						//team goal while leading
	// float lead_time;								//time leading someone
	// float leadvisible_time;							//last time the team mate was visible
	// float leadmessage_time;							//last time a messaged was sent to the team mate
	// float leadbackup_time;							//time backing up towards team mate

	//	char teamleader[32];							//netname of the team leader
	//	float askteamleader_time;						//time asked for team leader
	//	float becometeamleader_time;					//time the bot will become the team leader
	//	float teamgiveorders_time;						//time to give team orders
	int freezecheck_time;
	int numteammates; // number of team mates
	int forceorders;  // true if forced to give orders
	int flagcarrier;  // team mate carrying the enemy flag
	int hstationgoal; // 0 = no hstation, > 0 = station index +1
	char subteam[32]; // sub team name
	int formation_dist;

	bot_activategoal_t *activatestack;						// first activate goal on the stack
	bot_activategoal_t activategoalheap[MAX_ACTIVATESTACK]; // activate goal heap

	int lastcartcount;		// number of cartridges the bot carried last frame
	vec3_t syctarget;		// coordinates of current spraytarget
	int spraydist;			// maximum distance for this logo (stupid ?)
	bot_goal_t *which_wall; // which wall to take during this spray rush, see BotChooseWall
	float sprayTimer;		// current spraytarget is best before this time (3 sec)
	int stuckcounter;  // bathscript, increased every frame the bot is stuck (his traveltime is the same as last frame)
	int curtt;		   // bathscript, current traveltime
	qboolean takecart; // stop going for mate, go for NBG
	float checkcarttime; // search for weak cart carrying mates - time interval
	float checkhealth_time;
	qboolean cartthrown;
	int orderclient;	// teamleader AI, if this is == -1 order only this client
	int frametime;		// clock frame duration
	float duckuse_time; // dont use ducks till that time
	// int blueflagstatus;
	// int redflagstatus;
	int observed;
	cam_movement_t cam_movement;
	qboolean cam_taunt;
	qboolean cam_fire;
	cam_viewmode_t cam_viewmode;
	int cam_target;
	vec3_t cam_angles;
	//	nmyinfo_t nmys[4];
	ctf_waypoint_t *wp;
	int wptime;
	char log[10][MAX_SAY_TEXT];
	int logHead;
	char hudinfo[MAX_INFO_STRING];
} bot_state_t;

float AngleDifference(float ang1, float ang2);
// resets the whole bot state
void BotResetState(bot_state_t *bs);
// returns info about the entity
void BotEntityInfo(int entnum, aas_entityinfo_t *info);

qboolean BotWpValid(bot_state_t *bs);
qboolean FindWp(bot_state_t *bs, qboolean direction_home);
qboolean GetNextWp(bot_state_t *bs, qboolean);
qboolean BotWpHasSuccessor(bot_state_t *bs, qboolean direction_home);
int GetWpID(ctf_waypoint_t *wp);
void WaypointInit(void);

extern float floattime;
#define FloatTime() floattime

// from the game source
void BotAddInfo(bot_state_t *bs, const char *value, int dbgFlags);
void QDECL BotAI_Print(int type, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void QDECL QDECL BotAI_BotInitialChat(bot_state_t *bs, const char *type, ...);
void BotAI_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent,
				 int contentmask);
int BotAI_GetClientState(int clientNum, playerState_t *state);
int BotAI_GetEntityState(int entityNum, entityState_t *state);
int BotAI_GetSnapshotEntity(int clientNum, int sequence, entityState_t *state);
