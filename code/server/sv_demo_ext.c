/*
===========================================================================
Copyright (C) 2012 Stephen Larroque <lrq3000@gmail.com>

This file is part of OpenArena.

OpenArena is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenArena is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// sv_demo_ext.c -- Server side demo recording (supplementary functions)

#include "../game/g_local.h" // get both the definitions of gentity_t (to get gentity_t->health field) AND sharedEntity_t, so that we can convert a sharedEntity_t into a gentity_t (see more details in SV_GentityUpdateHealthField() notes)
#include "../qcommon/qcommon.h" // needed so that the public declarations in server.h can access these functions (because server.h links to qcommon.h, so that it does server.h->qcommon.h->sv_demo_ext.c -- in the end, no includes redundancy conflicts and every server files can access these functions!)

//#include "server.h" // DO NOT DO THAT! if you include server.h directly, you won't be able to include g_local.h, and you're stuck!

/***********************************************
 * AUXILIARY FUNCTIONS: UPDATING OF GENTITY_T->HEALTH
 *
 * Functions used to update some special aspects of the demo and which need to be separated from the main sv_demo.c file because of different includes that are necessary
 ***********************************************/

/*
====================
SV_GentityGetHealthField

Get the value of the gentity_t->health field (only used for testing purposes)
====================
*/
int SV_GentityGetHealthField( sharedEntity_t * gent ) {
    gentity_t *ent;

    ent = (gentity_t*)gent;

    //Com_Printf("DEMODEBUG GENGETFIELD: health: %i\n", ent->health);
    return ent->health;
}

/*
====================
SV_GentitySetHealthField

Set the value of the gentity_t->health field (only used for testing purposes)
====================
*/
void SV_GentitySetHealthField( sharedEntity_t * gent, int value ) {
    gentity_t *ent;

    ent = (gentity_t*)gent;

    ent->health = value;
}

/*
====================
SV_GentityUpdateHealthField

Update the value of the gentity_t->health field with playerState_t->stats[STAT_HEALTH] for a given player
You need to supply the player's sharedEntity and playerState (because since we have special includes here, we don't have access to the functions that can return a player from their int id).
The concrete effect is that when replaying a demo, the players' health will be updated on the HUD (if it weren't for this function to update the health, the health wouldn't change and stay kinda static).

Note: we need to do that because the demo can only records this stats[stat_health], which is concretely the same as gentity_t->health. The latter should have been removed altogether considering the comments in g_active.c (ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...), but it seems to have survived because it allows non-player entities to have health, such as obelisks. And weirdly, gentity_t->health has ascendence over stat_health (meaning stat_health is updated following gentity_t->health, but never the other way around), when for example stat_armor has ascendence over anything else of the same kind, so here we have to update it by ourselves.
Note2: this works pretty simply: sharedEntity_t = gentity_t but with only the first 2 fields declared (entityShared_t and entityState_t), but all the other fields are still in memory! We only need to get a valid declaration for gentity_t (which we do by doing the right includes at the top of this file, in g_local.h), and then we can convert the limited sharedEntity_t into a gentity_t with all the fields!
====================
*/
void SV_GentityUpdateHealthField( sharedEntity_t * gent, playerState_t *player ) {
    gentity_t *ent;

    ent = (gentity_t*)gent; // convert the sharedEntity_t to a gentity_t by using a simple cast (now that we have included g_local.h that contains the definition of gentity_t, and at the same time we have linked to g_public.h via g_local.h with the definition of sharedEntity_t)

    ent->health = player->stats[STAT_HEALTH]; // update player's health from playerState_t->stats[STAT_HEALTH] field

    return;
}