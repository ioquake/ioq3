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
// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted
spawning. "random" random number of plus or minus seconds varied from the respawn time "count" override quantity or
duration on most items.
*/

gitem_t bg_itemlist[] = {
	{NULL,
	 NULL,
	 {NULL, NULL, NULL, NULL},
	 /* icon */ NULL,
	 /* pickup */ NULL,
	 0,
	 0,
	 0,
	 /* precache */ "",
	 /* sounds */ ""}, // leave index 0 alone !

	//
	// ARMOR
	//

	/*QUAKED item_armor_shard (1 .5 0) (-16 -16 -16) (16 16 16) suspended
	 */
	{"item_armor_shard",
	 "sounds/items/armor/padshard_pickup",
	 {"models/powerups/armor/pad_shard", 0, 0, 0},
	 /* icon */ "icons/padshard",
	 /* pickup */ "PAD SHARD",
	 5,
	 IT_ARMOR,
	 0,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED item_armor_padshield (1 .5 0) (-16 -16 -16) (16 16 16) suspended
	 */
	{"item_armor_padshield",
	 "sounds/names/powerups/padshield",
	 {"models/powerups/armor/pad_shield", 0, 0, 0},
	 /* icon */ "icons/padshild",
	 /* pickup */ "PAD SHiELD",
	 100,
	 IT_ARMOR,
	 0,
	 /* precache */ "",
	 /* sounds */ ""},

	//
	// WEAPONS
	//

	/*QUAKED weapon_punchy (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_punchy",
	 "sounds/names/weapons/punchy",
	 {"models/weapons2/punchy/punchy", 0, 0, 0},
	 /* icon */ "icons/iconw_punchy",
	 /* pickup */ "PUNCHY",
	 0,
	 IT_WEAPON,
	 WP_PUNCHY,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_nipper (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_nipper",
	 "sounds/names/weapons/nipper",
	 {"models/weapons2/nipper/nipper", 0, 0, 0},
	 /* icon */ "icons/iconw_nipper",
	 /* pickup */ "NiPPER",
	 40,
	 IT_WEAPON,
	 WP_NIPPER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_boaster (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_boaster",
	 "sounds/names/weapons/boaster",
	 {"models/weapons2/boaster/boaster", 0, 0, 0},
	 /* icon */ "icons/iconw_boaster",
	 /* pickup */ "BOASTER",
	 100,
	 IT_WEAPON,
	 WP_BOASTER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_splasher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_splasher",
	 "sounds/names/weapons/splasher",
	 //        { "models/weapons2/railgun/railgun",
	 {"models/weapons2/splasher/splasher", 0, 0, 0},
	 /* icon */ "icons/iconw_splasher",
	 /* pickup */ "SPLASHER",
	 10,
	 IT_WEAPON,
	 WP_SPLASHER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_bubbleg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_bubbleg",
	 "sounds/names/weapons/bubbleg",
	 {"models/weapons2/bubbleg/bubbleg", 0, 0, 0},
	 /* icon */ "icons/iconw_bubbleg",
	 /* pickup */ "BUBBLE.G.",
	 50,
	 IT_WEAPON,
	 WP_BUBBLEG,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_balloony (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_balloony",
	 "sounds/names/weapons/balloony",
	 {"models/weapons2/balloony/balloony", 0, 0, 0},
	 /* icon */ "icons/iconw_balloony",
	 /* pickup */ "BALLOONY",
	 10,
	 IT_WEAPON,
	 WP_BALLOONY,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_betty (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_betty",
	 "sounds/names/weapons/betty",
	 {"models/weapons2/betty/betty", 0, 0, 0},
	 /* icon */ "icons/iconw_betty",
	 /* pickup */ "BETTY",
	 10,
	 IT_WEAPON,
	 WP_BETTY,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_pumper (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_pumper",
	 "sounds/names/weapons/pumper",
	 {"models/weapons2/pumper/pumper", 0, 0, 0},
	 /* icon */ "icons/iconw_pumper",
	 /* pickup */ "PUMPER",
	 10,
	 IT_WEAPON,
	 WP_PUMPER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_imperius (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_imperius",
	 "sounds/names/weapons/imperius",
	 {"models/weapons2/imperius/imperius", 0, 0, 0},
	 /* icon */ "icons/iconw_imperius",
	 /* pickup */ "IMPERiUS",
	 2,
	 IT_WEAPON,
	 WP_IMPERIUS,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_killerducks (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_killerducks",
	 "sounds/names/weapons/killerducks",
	 {"models/weapons2/killerducks/pad_killerduck_pu", 0, 0, 0},
	 /* icon */ "icons/killerducks",
	 /* pickup */ "KiLLERDUCKS WEAPON",
	 20,
	 IT_WEAPON,
	 WP_KILLERDUCKS,
	 /* precache */ "",
	 /* sounds */ ""},

	//
	// SPRAY PISTOLs
	//

	/*QUAKED weapon_spraypistol (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_spraypistol",
	 "sounds/weapons/weapon_pickup",
	 {"models/weapons2/spraypistol/spraypistol", 0, 0, 0},
	 /* icon */ "icons/iconw_spraypistol_red",
	 /* pickup */ "Spray Pistol",
	 0,
	 IT_WEAPON,
	 WP_SPRAYPISTOL,
	 /* precache */ "",
	 /* sounds */ ""},

	//
	// AMMO ITEMS
	//

	/*QUAKED ammo_pumper (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_pumper",
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/powerups/ammo/pumper_am", 0, 0, 0},
	 /* icon */ "icons/icona_pumper",
	 /* pickup */ "PUMPER AMMO",
	 15,
	 IT_AMMO,
	 WP_PUMPER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_nipper (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_nipper",
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/powerups/ammo/nipper_am", 0, 0, 0},
	 /* icon */ "icons/icona_nipper",
	 /* pickup */ "NiPPER AMMO",
	 50,
	 IT_AMMO,
	 WP_NIPPER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_balloony (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_balloony",
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/powerups/ammo/balloony_am", 0, 0, 0},
	 /* icon */ "icons/icona_balloony",
	 /* pickup */ "BALLOONY AMMO",
	 10, // old 15
	 IT_AMMO,
	 WP_BALLOONY,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_bubbleg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_bubbleg",
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/powerups/ammo/bubbleg_am", 0, 0, 0},
	 /* icon */ "icons/icona_bubbleg",
	 /* pickup */ "BUBBLE.G. AMMO",
	 40,
	 IT_AMMO,
	 WP_BUBBLEG,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_boaster (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_boaster",
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/powerups/ammo/boaster_am", 0, 0, 0},
	 /* icon */ "icons/icona_boaster",
	 /* pickup */ "BOASTER AMMO",
	 60, // old 100
	 IT_AMMO,
	 WP_BOASTER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_betty (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_betty",
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/powerups/ammo/betty_am", 0, 0, 0},
	 /* icon */ "icons/icona_betty",
	 /* pickup */ "BETTY AMMO",
	 10,
	 IT_AMMO,
	 WP_BETTY,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_splasher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_splasher",
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/powerups/ammo/splasher_am", 0, 0, 0},
	 /* icon */ "icons/icona_splasher",
	 /* pickup */ "SPLASHER AMMO",
	 10,
	 IT_AMMO,
	 WP_SPLASHER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_imperius (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_imperius",
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/powerups/ammo/imperius_am", 0, 0, 0},
	 /* icon */ "icons/icona_imperius",
	 /* pickup */ "IMPERiUS AMMO",
	 3,
	 IT_AMMO,
	 WP_IMPERIUS,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_killerducks (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_killerducks",
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/weapons2/killerducks/pad_killerduck_pu", 0, 0, 0},
	 /* icon */ "icons/killerducks",
	 /* pickup */ "KiLLERDUCKS AMMO",
	 10,
	 IT_AMMO,
	 WP_KILLERDUCKS,
	 /* precache */ "",
	 /* sounds */ ""},

	//
	// CARTRIDGEs
	//

	/*QUAKED ammo_spray_r (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_spray_r",
	 "sounds/weapons/ammo/spraypistol/cartridges",
	 {"models/weapons2/spraypistol/cart_red", 0, 0, 0},
	 /* icon */ "icons/icona_cart_red",
	 /* pickup */ "red Cartridge", // if changing this string, you will have to change some other code!!!
	 1,
	 IT_AMMO,
	 WP_SPRAYPISTOL,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_spray_b (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_spray_b",
	 "sounds/weapons/ammo/spraypistol/cartridges",
	 {"models/weapons2/spraypistol/cart_blue", 0, 0, 0},
	 /* icon */ "icons/icona_cart_blue",
	 /* pickup */ "blue Cartridge", // if changing this string, you will have to change some other code!!!
	 1,
	 IT_AMMO,
	 WP_SPRAYPISTOL,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_spray_n (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_spray_n",
	 "sounds/weapons/ammo/spraypistol/cartridges",
	 {"models/weapons2/spraypistol/cart_neutral", 0, 0, 0},
	 /* icon */ "icons/icona_cart_neutral",
	 /* pickup */ "neutral Cartridge", // if changing this string, you will have to change some other code!!!
	 1,
	 IT_AMMO,
	 WP_SPRAYPISTOL,
	 /* precache */ "",
	 /* sounds */ ""},

	//
	// HOLDABLE ITEMS
	//

	/*QUAKED holdable_floater (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"holdable_floater",
	 "sounds/names/powerups/floater",
	 {"models/powerups/instant/floater", "models/powerups/instant/floater_ring", 0, 0},
	 /* icon */ "icons/floater",
	 /* pickup */ "FLOATER",
	 60,
	 IT_HOLDABLE,
	 HI_FLOATER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED holdable_killerducks (1 .5 0) (-16 -16 -16) (16 16 16) suspended
	 */
	{"holdable_killerducks",
	 "sounds/names/weapons/killerducks",
	 {"models/weapons2/killerducks/pad_killerduck_pu", "models/powerups/instant/killerducks_ring", 0, 0},
	 /* icon */ "icons/killerducks",
	 /* pickup */ "KiLLERDUCKS",
	 60,
	 IT_HOLDABLE,
	 HI_KILLERDUCKS,
	 /* precache */ "",
	 /* sounds */ ""},

	// vv boomis & bambam vv
	/*QUAKED holdable_floater (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"holdable_bambam",
	 "sounds/names/powerups/bambam",
	 {"models/powerups/instant/bambam", "models/powerups/instant/bambam_ring", 0, 0},
	 /* icon */ "icons/bambam",
	 /* pickup */ "BAMBAM",
	 60,
	 IT_HOLDABLE,
	 HI_BAMBAM,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED holdable_floater (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"holdable_boomies",
	 "sounds/names/powerups/boomies",
	 {"models/powerups/instant/boomies", "models/powerups/instant/boomies_ring", 0, 0},
	 /* icon */ "icons/boomies",
	 /* pickup */ "BOOMiES",
	 60,
	 IT_HOLDABLE,
	 HI_BOOMIES,
	 /* precache */ "",
	 /* sounds */ ""},
	// ^^ boomies & bambam ^^

	//
	// POWERUP ITEMS
	//

	/*QUAKED item_padpower (.5 .5 .5) (-16 -16 -16) (16 16 16) suspended
	 */
	{
		"item_padpower",
		"sounds/names/powerups/padpower",
		{"models/powerups/instant/padpower", "models/powerups/instant/padpower_ring", 0, 0},
		/* icon */ "icons/padpower",
		/* pickup */ "PADPOWER",
		30,
		IT_POWERUP,
		PW_PADPOWER,
		/* precache */ "",
		/* sounds */ "sounds/items/padpower/damage3" // noch ändern
	},

	/*QUAKED item_climber (.8 0 .8) (-16 -16 -16) (16 16 16) suspended
	 */
	{"item_climber",
	 "sounds/names/powerups/climber",
	 {"models/powerups/instant/climber", "models/powerups/instant/climber_ring", 0, 0},
	 /* icon */ "icons/climber",
	 /* pickup */ "CLiMBER",
	 30,
	 IT_POWERUP,
	 PW_CLIMBER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED item_speedy (1 1 0) (-16 -16 -16) (16 16 16) suspended
	 */
	{"item_speedy",
	 "sounds/names/powerups/speedy",
	 {"models/powerups/instant/speedy", "models/powerups/instant/speedy_ring", 0, 0},
	 /* icon */ "icons/speedy",
	 /* pickup */ "SPEEDY",
	 30,
	 IT_POWERUP,
	 PW_SPEEDY,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED item_jumper (0 1 0) (-16 -16 -16) (16 16 16) suspended
	 */
	{"item_jumper",
	 "sounds/names/powerups/jumper",
	 {"models/powerups/instant/jumper", "models/powerups/instant/jumper_ring", 0, 0},
	 /* icon */ "icons/jumper",
	 /* pickup */ "JUMPER",
	 30,
	 IT_POWERUP,
	 PW_JUMPER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED item_visionless (.4 .4 .4) (-16 -16 -16) (16 16 16) suspended
	 */
	{"item_visionless",
	 "sounds/names/powerups/visionless",
	 {"models/powerups/instant/visionless", "models/powerups/instant/visionless_ring", 0, 0},
	 /* icon */ "icons/visionless",
	 /* pickup */ "ViSiONLESS",
	 30,
	 IT_POWERUP,
	 PW_VISIONLESS,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED item_revival (1 0 0) (-16 -16 -16) (16 16 16) suspended
	 */
	{"item_revival",
	 "sounds/names/powerups/revival",
	 {"models/powerups/instant/revival", "models/powerups/instant/revival_ring", 0, 0},
	 /* icon */ "icons/revival",
	 /* pickup */ "REViVAL",
	 30,
	 IT_POWERUP,
	 PW_REVIVAL,
	 /* precache */ "",
	 /* sounds */ "sounds/items/revival/heartbeat"},

	/*QUAKED item_berserker (1 0 0) (-16 -16 -16) (16 16 16) suspended
	 */
	{"item_berserker",
	 "sounds/weapons/punchy/berserker",
	 {0, //"models/powerups/instant/revival",
	  0, //"models/powerups/instant/revival_ring",
	  0, 0},
	 /* icon */ "icons/berserker",
	 /* pickup */ "PUPPET MASTER", //"BERSERKER",
	 30,
	 IT_POWERUP,
	 PW_BERSERKER,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED team_CTL_redlolly (1 0 0) (-16 -16 -16) (16 16 16)
	Only in CTL games
	*/
	{"team_CTL_redlolly",
	 NULL,
	 {"models/ctl/lollipop_red", 0, 0, 0},
	 /* icon */ "icons/iconf_red1",
	 /* pickup */ "red Lolly",
	 0,
	 IT_TEAM,
	 PW_REDFLAG,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED team_CTL_bluelolly (0 0 1) (-16 -16 -16) (16 16 16)
	Only in CTL games
	*/
	{"team_CTL_bluelolly",
	 NULL,
	 {"models/ctl/lollipop_blue", 0, 0, 0},
	 /* icon */ "icons/iconf_blu1",
	 /* pickup */ "blue Lolly",
	 0,
	 IT_TEAM,
	 PW_BLUEFLAG,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED weapon_kma97 ( .3 .3 1 ) (-16 -16 -16) (16 16 16) suspended
	 */
	{"weapon_kma97", // "Kiss My Ass 97"
	 "sounds/names/weapons/kma97",
	 {"models/weapons2/kma97/kma97", 0, 0, 0},
	 /* icon */ "icons/iconw_kma97",
	 /* pickup */ "KMA97",
	 1,
	 IT_WEAPON,
	 WP_KMA97,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED ammo_kma97 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
	 */
	{"ammo_kma97", // "Kiss My Ass 97"
	 "sounds/weapons/ammo/ammo_pickup",
	 {"models/powerups/ammo/kma97_am", 0, 0, 0},
	 /* icon */ "icons/icona_kma97",
	 /* pickup */ "KMA97 AMMO",
	 10,
	 IT_AMMO,
	 WP_KMA97,
	 /* precache */ "",
	 /* sounds */ ""},

	/*QUAKED item_freeze (0 1 0) (-16 -16 -16) (16 16 16) suspended
	 */
	{"item_freeze",
	 "sounds/Items/Jumper.wav",
	 {"models/powerups/instant/jumper.md3", "models/powerups/instant/jumper_ring.md3", 0, 0},
	 /* icon */ "icons/FREEZEicon",
	 /* pickup */ "JUMPER",
	 10,
	 IT_POWERUP,
	 PW_FREEZE,
	 /* precache */ "",
	 /* sounds */ ""},

	// end of list marker
	{NULL}};

int bg_numItems = ARRAY_LEN(bg_itemlist) - 1;

/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t *BG_FindItemForPowerup(powerup_t pw) {
	int i;

	for (i = 0; i < bg_numItems; i++) {
		if ((bg_itemlist[i].giType == IT_POWERUP || bg_itemlist[i].giType == IT_TEAM ||
			 bg_itemlist[i].giType == IT_PERSISTANT_POWERUP) &&
			bg_itemlist[i].giTag == pw) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}

/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t *BG_FindItemForHoldable(holdable_t pw) {
	int i;

	for (i = 0; i < bg_numItems; i++) {
		if (bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw) {
			return &bg_itemlist[i];
		}
	}

	Com_Error(ERR_DROP, "HoldableItem not found");

	return NULL;
}

/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t *BG_FindItemForWeapon(weapon_t weapon) {
	gitem_t *it;

	for (it = bg_itemlist + 1; it->classname; it++) {
		if (it->giType == IT_WEAPON && it->giTag == weapon) {
			return it;
		}
	}

	Com_Error(ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
===============
BG_FindItem

===============
*/
gitem_t *BG_FindItem(const char *pickupName) {
	gitem_t *it;

	for (it = bg_itemlist + 1; it->classname; it++) {
		if (!Q_stricmp(it->pickup_name, pickupName))
			return it;
	}

	return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime) {
	vec3_t origin;

	BG_EvaluateTrajectory(&item->pos, atTime, origin);

	// we are ignoring ducked differences here
	if (ps->origin[0] - origin[0] > 44 || ps->origin[0] - origin[0] < -50 || ps->origin[1] - origin[1] > 36 ||
		ps->origin[1] - origin[1] < -36 || ps->origin[2] - origin[2] > 36 || ps->origin[2] - origin[2] < -36) {
		return qfalse;
	}

	return qtrue;
}

/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed(int gametype, const entityState_t *ent, const playerState_t *ps) {
	gitem_t *item;

	if (ent->modelindex < 1 || ent->modelindex >= bg_numItems) {
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");
	}

	item = &bg_itemlist[ent->modelindex];

	switch (item->giType) {
	case IT_WEAPON:
		return qtrue; // weapons are always picked up

	case IT_AMMO:
		if (ps->ammo[item->giTag] >= MAXAMMO_WEAPON) {
			return qfalse; // can't hold any more
		}
		return qtrue;

	case IT_ARMOR:
		if (ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
		if (item->quantity == 5 || item->quantity == 100) {
			if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2) {
				return qfalse;
			}
			return qtrue;
		}

		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH]) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
		return qtrue; // powerups are always picked up

	case IT_TEAM: // team items, such as flags
		if (gametype == GT_CTF) {
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG || (item->giTag == PW_REDFLAG && ent->modelindex2) ||
					(item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]))
					return qtrue;
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG || (item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
					(item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]))
					return qtrue;
			}
		}

		return qfalse;

	case IT_HOLDABLE:
		// can only hold one item at a time
		if (ps->stats[STAT_HOLDABLE_ITEM]) {

			// pick it up, if you already have one of this type
			if (ps->stats[STAT_HOLDABLE_ITEM] == item - bg_itemlist)
				return qtrue;

			return qfalse;
		}
		return qtrue;

	case IT_BAD:
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD");
	default:
#ifndef Q3_VM
#ifndef NDEBUG // bk0001204
		Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType);
#endif
#endif
		break;
	}

	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec3_t result) {
	float deltaTime;
	float phase;

	switch (tr->trType) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy(tr->trBase, result);
		break;
	case TR_LINEAR:
		deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
		phase = sin(deltaTime * M_PI * 2);
		VectorMA(tr->trBase, phase, tr->trDelta, result);
		break;
	case TR_LINEAR_STOP:
		if (atTime > tr->trTime + tr->trDuration) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
		if (deltaTime < 0) {
			deltaTime = 0;
		}
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
		break;
	case TR_GRAVITY:

	case TR_LOW_GRAVITY:
		if (atTime < tr->trTime)
			atTime = tr->trTime; // HERBY

		deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
		VectorMA(tr->trBase, deltaTime, tr->trDelta, result);

		if (tr->trType == TR_GRAVITY)
			result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...
		else
			result[2] -= 0.5 * LOW_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...

		break;
	default:
		Com_Error(ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime);
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec3_t result) {
	float deltaTime;
	float phase;

	switch (tr->trType) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear(result);
		break;
	case TR_LINEAR:
		VectorCopy(tr->trDelta, result);
		break;
	case TR_SINE:
		deltaTime = (atTime - tr->trTime) / (float)tr->trDuration;
		phase = cos(deltaTime * M_PI * 2); // derivative of sin = cos
		phase *= 0.5;
		VectorScale(tr->trDelta, phase, result);
		break;
	case TR_LINEAR_STOP:
		if (atTime > tr->trTime + tr->trDuration) {
			VectorClear(result);
			return;
		}
		VectorCopy(tr->trDelta, result);
		break;
	case TR_GRAVITY:

	case TR_LOW_GRAVITY:
		if (atTime < tr->trTime)
			atTime = tr->trTime; // HERBY

		deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
		VectorCopy(tr->trDelta, result);

		if (tr->trType == TR_GRAVITY)
			result[2] -= DEFAULT_GRAVITY * deltaTime; // FIXME: local gravity...
		else
			result[2] -= LOW_GRAVITY * deltaTime;

		break;
	default:
		Com_Error(ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime);
		break;
	}
}

char *eventnames[] = {"EV_NONE",

					  "EV_FOOTSTEP",
					  "EV_FOOTSTEP_METAL",
					  "EV_FOOTSPLASH",
					  "EV_FOOTWADE",
					  "EV_SWIM",

					  "EV_STEP_4",
					  "EV_STEP_8",
					  "EV_STEP_12",
					  "EV_STEP_16",

					  "EV_FALL_SHORT",
					  "EV_FALL_MEDIUM",
					  "EV_FALL_FAR",

					  "EV_JUMP_PAD", // boing sound at origin", jump sound on player

					  "EV_JUMP",
					  "EV_WATER_TOUCH", // foot touches
					  "EV_WATER_LEAVE", // foot leaves
					  "EV_WATER_UNDER", // head touches
					  "EV_WATER_CLEAR", // head leaves

					  "EV_ITEM_PICKUP",		   // normal item pickups are predictable
					  "EV_GLOBAL_ITEM_PICKUP", // powerup / team sounds are broadcast to everyone

					  "EV_NOAMMO",
					  "EV_CHANGE_WEAPON",
					  "EV_FIRE_WEAPON",

					  "EV_IMPERIUS_EXPLODE", // HERBY ... nagut eigentlich von mir ;P

					  "EV_USE_ITEM0",
					  "EV_USE_ITEM1",
					  "EV_USE_ITEM2",
					  "EV_USE_ITEM3",
					  "EV_USE_ITEM4",
					  "EV_USE_ITEM5",
					  "EV_USE_ITEM6",
					  "EV_USE_ITEM7",
					  "EV_USE_ITEM8",
					  "EV_USE_ITEM9",
					  "EV_USE_ITEM10",
					  "EV_USE_ITEM11",
					  "EV_USE_ITEM12",
					  "EV_USE_ITEM13",
					  "EV_USE_ITEM14",
					  "EV_USE_ITEM15",

					  "EV_ITEM_RESPAWN",
					  "EV_ITEM_POP",
					  "EV_PLAYER_TELEPORT_IN",
					  "EV_PLAYER_TELEPORT_OUT",

					  "EV_PLAYER_TELEPORT_RED_IN",
					  "EV_PLAYER_TELEPORT_RED_OUT",
					  "EV_PLAYER_TELEPORT_BLUE_IN",
					  "EV_PLAYER_TELEPORT_BLUE_OUT",

					  "EV_DROP_CARTRIDGE",

					  "EV_GRENADE_BOUNCE", // eventParm will be the soundindex

					  "EV_STICKY_BOUNCE", // HERBY

					  "EV_GENERAL_SOUND",
					  "EV_GLOBAL_SOUND", // no attenuation
					  "EV_GLOBAL_TEAM_SOUND",

					  "EV_MISSILE_HIT",
					  "EV_MISSILE_MISS",
					  "EV_MISSILE_MISS_METAL",
					  "EV_RAILTRAIL",
					  "EV_SHOTGUN",

					  "EV_SPRAYLOGO",

					  "EV_PAIN",
					  "EV_DEATH1",
					  "EV_DEATH2",
					  "EV_DEATH3",
					  "EV_OBITUARY",

					  "EV_POWERUP_REVIVAL",
					  "EV_POWERUP_PADPOWER",

					  "EV_GIB_PLAYER", // gib a previously living player
					  "EV_SCOREPLUM",  // score plum

					  "EV_DEBUG_LINE",
					  "EV_STOPLOOPINGSOUND",
					  "EV_TAUNT"

					  ,
					  "EV_HEHE1",
					  "EV_HEHE2"

					  ,
					  "EV_FOOTSTEP_CARPET",
					  "EV_FOOTSTEP_LATTICE",
					  "EV_FOOTSTEP_SAND",
					  "EV_FOOTSTEP_SOFT",
					  "EV_FOOTSTEP_WOOD",
					  "EV_FOOTSTEP_SNOW"

					  ,
					  "EV_BAMBAM_EXPLOSION",
					  "EV_BOOMIES_EXPLOSION"

					  ,
					  "EV_TAUNT_YES",
					  "EV_TAUNT_NO",
					  "EV_TAUNT_FOLLOWME",
					  "EV_TAUNT_GETFLAG",
					  "EV_TAUNT_GUARDBASE",
					  "EV_TAUNT_PATROL"

};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if (atof(buf) != 0) {
#ifdef QAGAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/,
					   ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/,
					   ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS - 1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS - 1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad(playerState_t *ps, entityState_t *jumppad) {
	vec3_t angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if (ps->pm_type != PM_NORMAL) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if (ps->jumppad_ent != jumppad->number) {

		vectoangles(jumppad->origin2, angles);
		p = fabs(AngleNormalize180(angles[PITCH]));
		if (p < 45) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate(EV_JUMP_PAD, effectNum, ps);
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy(jumppad->origin2, ps->velocity);
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, qboolean snap) {
	int i;

	if (ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR) {
		s->eType = ET_INVISIBLE;
	} else if (ps->stats[STAT_HEALTH] <= GIB_HEALTH) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy(ps->origin, s->pos.trBase);
	if (snap) {
		SnapVector(s->pos.trBase);
	}
	// set the trDelta for flag direction
	VectorCopy(ps->velocity, s->pos.trDelta);

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy(ps->viewangles, s->apos.trBase);
	if (snap) {
		SnapVector(s->apos.trBase);
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum; // ET_PLAYER looks here instead of at number
								  // so corpses can also reference the proper config
	ps->eFlags = (ps->eFlags & 0xFFFF) | (ps->stats[STAT_HB_EFLAGS] << 16);
	s->eFlags = ps->eFlags;
	if (ps->stats[STAT_HEALTH] <= 0) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if (ps->externalEvent) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if (ps->entityEventSequence < ps->eventSequence) {
		int seq;

		if (ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS - 1);
		s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[seq];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for (i = 0; i < MAX_POWERUPS; i++) {
		if (ps->powerups[i]) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate(playerState_t *ps, entityState_t *s, int time, qboolean snap) {
	int i;

	if (ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR) {
		s->eType = ET_INVISIBLE;
	} else if (ps->stats[STAT_HEALTH] <= GIB_HEALTH) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy(ps->origin, s->pos.trBase);
	if (snap) {
		SnapVector(s->pos.trBase);
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy(ps->velocity, s->pos.trDelta);
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy(ps->viewangles, s->apos.trBase);
	if (snap) {
		SnapVector(s->apos.trBase);
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum; // ET_PLAYER looks here instead of at number
								  // so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if (ps->stats[STAT_HEALTH] <= 0) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if (ps->externalEvent) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if (ps->entityEventSequence < ps->eventSequence) {
		int seq;

		if (ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS - 1);
		s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
		s->eventParm = ps->eventParms[seq];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for (i = 0; i < MAX_POWERUPS; i++) {
		if (ps->powerups[i]) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

int convertGTStringToGTNumber(const char *argStr) {
	int gt = -1;
	char buf[512];
	Q_strncpyz(buf, argStr, sizeof(buf));
	Q_strupr(buf);

	if (strstr(buf, GAMETYPE_NAME_SHORT(GT_SPRAYFFA)) || (strstr(buf, "SPRAY") && strstr(buf, "COLOR"))) {
		if (strstr(buf, "TP") || strstr(buf, "TEAM")) {
			gt = GT_SPRAY;
		} else {
			gt = GT_SPRAYFFA;
		}
	} else if (strstr(buf, GAMETYPE_NAME_SHORT(GT_BALLOON)) || strstr(buf, "BALLOON")) {
		gt = GT_BALLOON;
	} else if (strstr(buf, GAMETYPE_NAME_SHORT(GT_LPS)) || (strstr(buf, "LAST") && strstr(buf, "STANDING"))) {
		gt = GT_LPS;
	} else if (strstr(buf, GAMETYPE_NAME_SHORT(GT_CTF)) ||
			   (strstr(buf, "CAPTURE") && (strstr(buf, "LOLLY") || strstr(buf, "LOLLI" /*pop"*/)))) {
		gt = GT_CTF;
	} else if (strstr(buf, GAMETYPE_NAME_SHORT(GT_TOURNAMENT)) || strstr(buf, "TOURNAMENT") || strstr(buf, "TOURNEY")) {
		gt = GT_TOURNAMENT;
	} else if (strstr(buf, GAMETYPE_NAME_SHORT(GT_TEAM)) || strstr(buf, "TP") || strstr(buf, "TEAM") ||
			   strstr(buf, "TDM")) {
		gt = GT_TEAM;
	} else if (strstr(buf, GAMETYPE_NAME_SHORT(GT_FFA)) || strstr(buf, "FREE") || strstr(buf, "DM")) {
		gt = GT_FFA;
	} else if (strstr(argStr, GAMETYPE_NAME_SHORT(GT_FREEZETAG)) || strstr(argStr, "FREEZE")) {
		gt = GT_FREEZETAG;
	}

	return gt;
}
