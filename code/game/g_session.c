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
#include "g_local.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*var;

	s = va("%i %i %i %i %i %i %i", 
		client->sess.sessionTeam,
		client->sess.spectatorNum,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader
		);

	var = va( "session%i", (int)(client - level.clients) );

	trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;
	int teamLeader;
	int spectatorState;
	int sessionTeam;

	var = va( "session%i", (int)(client - level.clients) );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i",
		&sessionTeam,
		&client->sess.spectatorNum,
		&spectatorState,
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader
		);

	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo ) {
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;

	// initial team determination
	if ( g_gametype.integer >= GT_TEAM ) {
		if ( g_teamAutoJoin.integer && !(g_entities[ client - level.clients ].r.svFlags & SVF_BOT) ) {
			sess->sessionTeam = PickTeam( -1 );
			BroadcastTeamChange( client, -1 );
		} else {
			// always spawn as spectator in team games
			sess->sessionTeam = TEAM_SPECTATOR;	
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( g_gametype.integer ) {
			default:
			case GT_FFA:
			case GT_SINGLE_PLAYER:
				if ( g_maxGameClients.integer > 0 && 
					level.numNonSpectatorClients >= g_maxGameClients.integer ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_TOURNAMENT:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	AddTournamentQueue(client);

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );
	
	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( g_gametype.integer != gt ) {
		level.newSession = qtrue;
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i", g_gametype.integer) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}

/*
=======================================================================

  SCORE RESTORE ON RECONNECT

  Scores are keyed off the client's cl_guid so a player who drops and
  comes back gets their frags handed back. The table lives only for the
  lifetime of one level -- G_ClearScoreRestore() is called from
  G_InitGame(), so nothing survives a map change or a map_restart.

  Set g_scoreRestoreDebug 1 for a running commentary on the server
  console. See the notes at the bottom of this file for what the
  various failure modes look like.

=======================================================================
*/

#define MAX_SCORE_RESTORE		64
#define SCORE_RESTORE_TIMEOUT	(5 * 60 * 1000)		// give up after 5 minutes

typedef struct {
	char	guid[33];
	int		score;
	int		deaths;
	team_t	team;			// team the score was banked from
	int		expireTime;
} scoreRestore_t;

static scoreRestore_t	g_scoreRestore[MAX_SCORE_RESTORE];

/*
==================
SR_DumpTable

Prints every live entry. Only at g_scoreRestoreDebug 2 or higher, since
ordinary team changes produce misses as a matter of course.
==================
*/
static void SR_DumpTable( const char *why ) {
	int		i, live;

	live = 0;
	for ( i = 0 ; i < MAX_SCORE_RESTORE ; i++ ) {
		if ( !g_scoreRestore[i].guid[0] ) {
			continue;
		}
		live++;
		G_Printf( "ScoreRestore:   [%i] guid=\"%s\" score=%i deaths=%i team=%i expires=%i (%s)\n",
			i, g_scoreRestore[i].guid, g_scoreRestore[i].score,
			g_scoreRestore[i].deaths, g_scoreRestore[i].team,
			g_scoreRestore[i].expireTime,
			g_scoreRestore[i].expireTime <= level.time ? "STALE" : "live" );
	}
	G_Printf( "ScoreRestore: table dump (%s): %i entr%s, level.time=%i\n",
		why, live, live == 1 ? "y" : "ies", level.time );
}

/*
==================
SR_ShouldApply

Vanilla wipes a player's score whenever they change team, so a banked
score is only handed back if they rejoin the team they left. Team is
meaningless in FFA and tournament, so it is ignored there. Set
g_scoreRestoreCrossTeam 1 to hand the score back regardless of team.
==================
*/
static qboolean SR_ShouldApply( gclient_t *client ) {
	if ( g_gametype.integer < GT_TEAM ) {
		return qtrue;
	}
	if ( g_scoreRestoreCrossTeam.integer ) {
		return qtrue;
	}
	return (qboolean)( client->sess.sessionTeam == client->pers.restoreTeam );
}

/*
==================
G_ClearScoreRestore
==================
*/
void G_ClearScoreRestore( void ) {
	memset( g_scoreRestore, 0, sizeof( g_scoreRestore ) );
}

/*
==================
G_SaveClientScore

Called from ClientDisconnect() while the score is still intact.
==================
*/
void G_SaveClientScore( gclient_t *client ) {
	int				i, oldest;
	int				clientNum;
	int				score, deaths;
	team_t			team;
	scoreRestore_t	*slot, *found;

	clientNum = (int)( client - level.clients );

	score = client->ps.persistant[PERS_SCORE];
	deaths = client->ps.persistant[PERS_KILLED];
	team = client->sess.sessionTeam;

	// if they reconnected and spectated without ever picking a team, the
	// claim is still pending in pers -- bank the original values, not zeroes
	if ( client->pers.restoreValid && !score && !deaths ) {
		score = client->pers.restoreScore;
		deaths = client->pers.restoreDeaths;
		team = client->pers.restoreTeam;
		if ( g_scoreRestoreDebug.integer ) {
			G_Printf( "ScoreRestore: SAVE client %i re-banking unclaimed restore\n", clientNum );
		}
	}

	// bots and clients with no qkey have an empty guid -- nothing to key on
	if ( !client->pers.guid[0] ) {
		if ( g_scoreRestoreDebug.integer ) {
			G_Printf( "ScoreRestore: SAVE client %i SKIPPED, empty guid (bot, or client sent no cl_guid)\n",
				clientNum );
		}
		return;
	}

	// don't burn a slot on someone who never scored
	if ( !score && !deaths ) {
		if ( g_scoreRestoreDebug.integer ) {
			G_Printf( "ScoreRestore: SAVE client %i SKIPPED, nothing to save (score 0, deaths 0)\n",
				clientNum );
		}
		return;
	}

	found = NULL;
	oldest = -1;

	for ( i = 0 ; i < MAX_SCORE_RESTORE ; i++ ) {
		slot = &g_scoreRestore[i];

		// reuse our own entry, or any empty / stale one
		if ( !slot->guid[0] || slot->expireTime <= level.time ) {
			if ( !found ) {
				found = slot;
			}
			continue;
		}
		if ( !Q_stricmp( slot->guid, client->pers.guid ) ) {
			found = slot;
			break;
		}
		if ( oldest < 0 || slot->expireTime < g_scoreRestore[oldest].expireTime ) {
			oldest = i;
		}
	}

	// table is full of live entries -- evict the one closest to expiring
	if ( !found ) {
		found = &g_scoreRestore[oldest < 0 ? 0 : oldest];
		if ( g_scoreRestoreDebug.integer ) {
			G_Printf( "ScoreRestore: SAVE table full, evicting slot %i\n",
				(int)( found - g_scoreRestore ) );
		}
	}

	Q_strncpyz( found->guid, client->pers.guid, sizeof( found->guid ) );
	found->score = score;
	found->deaths = deaths;
	found->team = team;
	found->expireTime = level.time + SCORE_RESTORE_TIMEOUT;

	if ( g_scoreRestoreDebug.integer ) {
		G_Printf( "ScoreRestore: SAVE client %i -> slot %i, guid=\"%s\" score=%i deaths=%i team=%i expires=%i\n",
			clientNum, (int)( found - g_scoreRestore ), found->guid,
			found->score, found->deaths, found->team, found->expireTime );
	}
}

/*
==================
G_RestoreClientScore

Called from ClientBegin() after client->ps has been wiped and before
ClientSpawn(), which preserves persistant[].

ClientBegin() runs more than once per connection: once when the client
first enters (usually as a spectator) and again from SetTeam() every
time they change team. Each one wipes ps, so the table entry is claimed
*into* client->pers -- which survives ClientBegin and ClientSpawn --
and held there until the client settles on a real team. Only then is it
applied, and only if the team matches the one it was banked from.
==================
*/
void G_RestoreClientScore( gclient_t *client ) {
	int				i;
	int				clientNum;
	qboolean		claimed;
	scoreRestore_t	*slot;

	clientNum = (int)( client - level.clients );

	if ( !client->pers.restoreValid ) {
		if ( !client->pers.guid[0] ) {
			if ( g_scoreRestoreDebug.integer ) {
				G_Printf( "ScoreRestore: LOOKUP client %i ABORTED, empty guid\n", clientNum );
			}
			return;
		}

		if ( g_scoreRestoreDebug.integer ) {
			G_Printf( "ScoreRestore: LOOKUP client %i guid=\"%s\"\n",
				clientNum, client->pers.guid );
		}

		claimed = qfalse;

		for ( i = 0 ; i < MAX_SCORE_RESTORE ; i++ ) {
			slot = &g_scoreRestore[i];

			if ( !slot->guid[0] ) {
				continue;
			}
			if ( Q_stricmp( slot->guid, client->pers.guid ) ) {
				continue;
			}

			// guid matched -- but is the entry still good?
			if ( slot->expireTime <= level.time ) {
				if ( g_scoreRestoreDebug.integer ) {
					G_Printf( "ScoreRestore: MISS client %i, guid matched slot %i but it EXPIRED (%i <= %i)\n",
						clientNum, i, slot->expireTime, level.time );
				}
				memset( slot, 0, sizeof( *slot ) );
				return;
			}

			// claim it into pers so it survives the next ClientBegin
			client->pers.restoreScore = slot->score;
			client->pers.restoreDeaths = slot->deaths;
			client->pers.restoreTeam = slot->team;
			client->pers.restoreValid = qtrue;

			if ( g_scoreRestoreDebug.integer ) {
				G_Printf( "ScoreRestore: CLAIM client %i <- slot %i, score=%i deaths=%i team=%i\n",
					clientNum, i, slot->score, slot->deaths, slot->team );
			}

			memset( slot, 0, sizeof( *slot ) );
			claimed = qtrue;
			break;
		}

		if ( !claimed ) {
			if ( g_scoreRestoreDebug.integer ) {
				G_Printf( "ScoreRestore: MISS client %i, no entry for guid \"%s\"\n",
					clientNum, client->pers.guid );
			}
			if ( g_scoreRestoreDebug.integer > 1 ) {
				SR_DumpTable( "after miss" );
			}
			return;
		}
	}

	// hold the claim until they stop spectating and commit to a team
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( g_scoreRestoreDebug.integer ) {
			G_Printf( "ScoreRestore: client %i spectating, holding claim (score=%i team=%i)\n",
				clientNum, client->pers.restoreScore, client->pers.restoreTeam );
		}
		return;
	}

	if ( SR_ShouldApply( client ) ) {
		client->ps.persistant[PERS_SCORE] = client->pers.restoreScore;
		client->ps.persistant[PERS_KILLED] = client->pers.restoreDeaths;

		if ( g_scoreRestoreDebug.integer ) {
			G_Printf( "ScoreRestore: APPLY client %i score=%i deaths=%i on team %i\n",
				clientNum, client->pers.restoreScore, client->pers.restoreDeaths,
				client->sess.sessionTeam );
		}
		G_LogPrintf( "ScoreRestore: %i score %i\n", clientNum, client->pers.restoreScore );
	} else {
		if ( g_scoreRestoreDebug.integer ) {
			G_Printf( "ScoreRestore: DROPPED client %i, banked on team %i but rejoined team %i (g_scoreRestoreCrossTeam 0)\n",
				clientNum, client->pers.restoreTeam, client->sess.sessionTeam );
		}
	}

	client->pers.restoreValid = qfalse;
}
