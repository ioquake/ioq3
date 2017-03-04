/*
===========================================================================
Copyright (C) 2008 Amanieu d'Antras
Copyright (C) 2012-2017 Stephen Larroque <lrq3000@gmail.com>

This file is part of ioquake3.

ioquake3 is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

ioquake3 is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// sv_demo.c -- Server side demo recording

#include "server.h"

#define Q_IsColorStringGameCommand(p)      ((p) && *(p) == Q_COLOR_ESCAPE && *((p)+1)) // ^[anychar]

#define CEIL(VARIABLE) ( (VARIABLE - (int)VARIABLE)==0 ? (int)VARIABLE : (int)VARIABLE+1 ) // UNUSED but can be useful

/***********************************************
 * VARIABLES
 *
 ***********************************************/

// Headers/markers for demo messages (events)
typedef enum {
	demo_endDemo, // end of demo (close the demo)
	demo_EOF, // end of file/flux (end of event, separator, notify the demo parser to iterate to the next event of the _same_ frame)
	demo_endFrame, // player and gentity state marker (recorded each end of frame, hence the name) - at the same time marks the end of the demo frame

	demo_configString, // config string setting event
	demo_clientConfigString, // client config string setting event
	demo_clientCommand, // client command event
	demo_serverCommand, // server command event
	demo_gameCommand, // game command event
	demo_clientUserinfo, // client userinfo event (client_t management)
	demo_entityState, // gentity_t->entityState_t management
	demo_entityShared, // gentity_t->entityShared_t management
	demo_playerState, // players game state event (playerState_t management)

	//demo_clientUsercmd, // players commands/movements packets (usercmd_t management)
} demo_ops_e;

/*** STATIC VARIABLES ***/
// We set them as static so that they are global only for this file, this limit a bit the side-effect

// Big fat buffer to store all our stuff
static byte buf[0x400000];

// Save maxclients and democlients and restore them after the demo
static int savedMaxClients = -1;
static int savedBotMinPlayers = -1;
static int savedFPS = -1;
static int savedGametype = -1;

static int savedDoWarmup = -1;
static int savedAllowVote = -1;
static int savedTeamAutoJoin = -1;

static int savedTimelimit = -1;
static int savedFraglimit = -1;
static int savedCapturelimit = -1;

static char savedFsGameVal[MAX_QPATH] = "";
static char *savedFsGame = savedFsGameVal;

static char savedPlaybackDemonameVal[MAX_QPATH] = "";
static char *savedPlaybackDemoname = savedPlaybackDemonameVal;

static qboolean keepSaved = qfalse; // var that memorizes if we keep the new maxclients and democlients values (in the case that we restart the map/server for these cvars to be affected since they are latched, we need to stop the playback meanwhile we restart, and using this var we can know if the stop is a restart procedure or a real demo end) or if we can restore them (at the end of the demo)

static int demoTeamAutoJoin = -1; // load the g_teamAutoJoin value from the demo recording


/***********************************************
 * AUXILIARY FUNCTIONS: CHECKING, FILTERING AND CLEANING
 *
 * Functions used to either trim unnecessary or privacy concerned data, or to just check if the data should be written in the demo, relayed to a more specialized function or just dropped.
 ***********************************************/

/*
====================
SV_CheckClientCommand

Check that the clientCommand is OK (if not we either continue with a more specialized function, or drop it altogether if there's a privacy concern)
====================
*/
qboolean SV_CheckClientCommand( client_t *client, const char *cmd )
{
	if ( strlen(cmd) > 9 && !Q_strncmp(cmd, "userinfo", 8) ) { // If that's a userinfo command, we directly handle that with a specialized function (and we check that it contains at least 10 characters so that when we copy the string we don't end up copying a random address in memory)
		char	*userinfo = malloc( MAX_INFO_STRING * sizeof *userinfo);
		Q_strncpyz(userinfo, cmd+9, MAX_INFO_STRING); // trimming out the "userinfo " substring (because we only need the userinfo string)
		SV_DemoWriteClientUserinfo(client, (const char*)userinfo); // passing relay to the specialized function for this job
		free( userinfo );

		return qfalse; // we return false if the check wasn't right (meaning that this function does not need to process anything)
	} else if( !Q_strncmp(cmd, "tell", 4) || !Q_strncmp(cmd, "say_team", 8) ) { // Privacy check: if the command is tell or say_team, we just drop it
		return qfalse;
	}
	return qtrue; // else, the check is OK and we continue to process with the original function
}

/*
====================
SV_CheckServerCommand

Check that the serverCommand is OK to save (or if we drop it because already handled somewhere else)
====================
*/
qboolean SV_CheckServerCommand( const char *cmd )
{
	if ( !Q_strncmp(cmd, "print", 5) || !Q_strncmp(cmd, "cp", 2) || !Q_strncmp(cmd, "disconnect", 10) ) { // If that's a print/cp command, it's already handled by gameCommand (here it's a redundancy). FIXME? possibly some print/cp are not handled by gameCommand? From my tests it was ok, but if someday a few prints are missing, try to delete this check... For security, we also drop disconnect (even if it should not be recorded anyway in the first place), because server commands will be sent to every connected clients, and so it will disconnect everyone.
		return qfalse; // we return false if the check wasn't right
	}
	return qtrue; // else, the check is OK and we continue to process with the original function
}

/*
====================
SV_CheckLastCmd

Check and store the last command and compare it with the current one, to avoid duplicates.
If onlyStore is true, it will only store the new cmd, without checking.
====================
*/
qboolean SV_CheckLastCmd( const char *cmd, qboolean onlyStore )
{
	static char prevcmddata[MAX_STRING_CHARS];
	static char *prevcmd = prevcmddata;
    char *cleanedprevcmd = malloc ( MAX_STRING_CHARS * sizeof * cleanedprevcmd );
    char *cleanedcmd = malloc ( MAX_STRING_CHARS * sizeof * cleanedcmd );

	Q_strncpyz(cleanedprevcmd, SV_CleanStrCmd(va("%s", (char *)prevcmd)), MAX_STRING_CHARS);
	Q_strncpyz(cleanedcmd, SV_CleanStrCmd(va("%s", (char *)cmd)), MAX_STRING_CHARS);

	if ( !onlyStore && // if we only want to store, we skip any checking
	    strlen(prevcmd) > 0 && !Q_stricmp(cleanedprevcmd, cleanedcmd) ) { // check that the previous cmd was different from the current cmd.
	    // Clean the vars before exiting the func
	    free(cleanedprevcmd);
	    free(cleanedcmd);
		return qfalse; // drop this command, it's a repetition of the previous one
	} else {
		Q_strncpyz(prevcmd, cmd, MAX_STRING_CHARS); // memorize the current cmd for the next check (clean the cmd before, because sometimes the same string is issued by the engine with some empty colors?)
	    // Clean the vars before exiting the func
	    free(cleanedprevcmd);
	    free(cleanedcmd);
		return qtrue;
	}
}

/*
====================
SV_CheckGameCommand

Check that the gameCommand is OK and that we can save it (or if we drop it for privacy concerns and/or because it's handled somewhere else already)
====================
*/
qboolean SV_CheckGameCommand( const char *cmd )
{
	if ( !SV_CheckLastCmd( cmd, qfalse ) ) { // check that the previous cmd was different from the current cmd. The engine may send the same command to every client by their cid (one command per client) instead of just sending one command to all using NULL or -1.
		return qfalse; // drop this command, it's a repetition of the previous one
	} else {
		if ( !Q_strncmp(cmd, "chat", 4) || !Q_strncmp(cmd, "tchat", 5) ) { // we filter out the chat and tchat commands which are recorded and handled directly by clientCommand (which is easier to manage because it makes a difference between say, say_team and tell, which we don't have here in gamecommands: we either have chat(for say and tell) or tchat (for say_team) and the other difference is that chat/tchat messages directly contain the name of the player, while clientCommand only contains the clientid, so that it itself fetch the name from client->name
			return qfalse; // we return false if the check wasn't right
		} else if ( !Q_strncmp(cmd, "cs", 2) ) { // if it's a configstring command, we handle that with the specialized function
			Cmd_SaveCmdContext();
			Cmd_TokenizeString(cmd);
			SV_DemoWriteConfigString(atoi(Cmd_Argv(1)), Cmd_Argv(2)); // relay to the specialized write configstring function
			Cmd_RestoreCmdContext();
			return qfalse; // drop it with the processing of the game command
		}
		return qtrue; // else, the check is OK and we continue to process with the original function
	}
}

/*
====================
SV_CheckConfigString

Check that the configstring is OK (or if we relay to ClientConfigString)
====================
*/
qboolean SV_CheckConfigString( int cs_index, const char *cs_string )
{
	if ( cs_index >= CS_PLAYERS && cs_index < CS_PLAYERS + sv_maxclients->integer ) { // if this is a player, we save the configstring as a clientconfigstring
		SV_DemoWriteClientConfigString( cs_index - CS_PLAYERS, cs_string );
		return qfalse; // we return false if the check wasn't right (meaning that we stop the normal configstring processing since the clientconfigstring function will handle that)
	} else if ( cs_index < CS_MOTD ) { // if the configstring index is below 4 (the motd), we don't save it (these are systems configstrings and will make an infinite loop for real clients at connection when the demo is played back, their management must be left to the system even at replaying)
		return qfalse; // just drop this configstring
	}
	return qtrue; // else, the check is OK and we continue to process to save it as a normal configstring (for capture scores CS_SCORES1/2, for CS_FLAGSTATUS, etc..)
}

/*
====================
SV_DemoFilterClientUserinfo

Filters privacy keys from a client userinfo to later write in a demo file
====================
*/
void SV_DemoFilterClientUserinfo( const char *userinfo )
{
	char *buf;

	buf = (char *)userinfo;
	Info_RemoveKey(buf, "cl_guid");
	Info_RemoveKey(buf, "ip");
	Info_SetValueForKey(buf, "cl_voip", "0");

	userinfo = (const char*)buf;
}

/*
====================
SV_CleanFilename

Attempts to clean invalid characters from a filename that may prevent the demo to be stored on the filesystem
====================
*/
char *SV_CleanFilename( char *string ) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			s++; // skip if it's a color
		}
		else if ( c == 0x2D || c == 0x2E || c == 0x5F || // - . _
			 (c >= 0x30 && c <= 0x39) || // numbers
			 (c >= 0x41 && c <= 0x5A) || // uppercase letters
			 (c >= 0x61 && c <= 0x7A) // lowercase letters
			 ) {
			*d++ = c; // keep if this character is not forbidden (contained inside the above whitelist)
		}
		s++; // go to next character
	}
	*d = '\0';

	return string;
}

/*
====================
SV_CleanStrCmd

Same as Q_CleanStr but also remove any ^s or special empty color created by the engine in a gamecommand.
====================
*/
char *SV_CleanStrCmd( char *string ) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		if ( Q_IsColorStringGameCommand( s ) ) {
			s++;
		}
		else if ( c >= 0x20 && c <= 0x7E ) {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
====================
SV_GenerateDateTime

Generate a full datetime (local and utc) from now
====================
*/
char *SV_GenerateDateTime(void)
{
	// Current local time
	qtime_t now;
	Com_RealTime( &now );

	// UTC time
	time_t  utcnow = time(NULL);
	struct tm tnow = *gmtime(&utcnow);
	char    buff[1024];

	strftime( buff, sizeof buff, "timezone %Z (UTC timezone: %Y-%m-%d %H:%M:%S W%W)", &tnow );

	// Return local time and utc time
	return va( "%04d-%02d-%02d %02d:%02d:%02d %s",
				1900 + now.tm_year,
				1 + now.tm_mon,
				now.tm_mday,
				now.tm_hour,
				now.tm_min,
				now.tm_sec,
				buff);
}


/***********************************************
 * DEMO WRITING FUNCTIONS
 *
 * Functions used to construct and write demo events
 ***********************************************/

/*
====================
SV_DemoWriteMessage

Write a message/event to the demo file
====================
*/
static void SV_DemoWriteMessage(msg_t *msg)
{
	int len;

	// Write the entire message to the file, prefixed by the length
	MSG_WriteByte(msg, demo_EOF); // append EOF (end-of-file or rather end-of-flux) to the message so that it will tell the demo parser when the demo will be read that the message ends here, and that it can proceed to the next message
	len = LittleLong(msg->cursize);
	FS_Write(&len, 4, sv.demoFile);
	FS_Write(msg->data, msg->cursize, sv.demoFile);
	MSG_Clear(msg);
}

/*
====================
SV_DemoWriteClientCommand

Write a client command to the demo file
====================
*/
void SV_DemoWriteClientCommand( client_t *client, const char *cmd )
{
	msg_t msg;

	if ( !SV_CheckClientCommand(client, cmd) ) { // check that this function really needs to store the command (or if another more specialized function should do it instead, eg: userinfo)
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientCommand);
	MSG_WriteByte(&msg, client - svs.clients);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteServerCommand

Write a server command to the demo file
Note: we record only server commands that are meant to be sent to everyone in the first place, that's why we don't even bother to store the clientnum. For commands that were only intended to specifically one player, we only record them if they were game commands (see below the specialized function), else we do not because it may be unsafe (such as "disconnect" command)
====================
*/
void SV_DemoWriteServerCommand( const char *cmd )
{
	msg_t msg;

	if ( !SV_CheckServerCommand( cmd ) ) { // check that this function really needs to store the command
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_serverCommand);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteGameCommand

Write a game command to the demo file
Note: game commands are sent to server commands, but we record them separately because game commands are safe to be replayed to everyone (even if at recording time it was only intended to one player), while server commands may be unsafe if it was dedicated only to one player (such as "disconnect")
====================
*/
void SV_DemoWriteGameCommand( int clientNum, const char *cmd )
{
	msg_t msg;

	if ( !SV_CheckGameCommand( cmd ) ) { // check that this function really needs to store the command
		return;
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_gameCommand);
	MSG_WriteByte(&msg, clientNum);
	MSG_WriteString(&msg, cmd);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteConfigString

Write a configstring to the demo file
cs_index = index of the configstring (see bg_public.h)
cs_string = content of the configstring to set
====================
*/
void SV_DemoWriteConfigString( int cs_index, const char *cs_string )
{
	msg_t msg;
	char cindex[MAX_CONFIGSTRINGS];

	if ( !SV_CheckConfigString( cs_index, cs_string ) ) { // check that this function really needs to store the configstring (or if it's a client configstring we relay to the specialized function)
		return;
	} // else we save it below as a normal configstring (for capture scores CS_SCORES1/2, for CS_FLAGSTATUS, etc..)

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_configString);
	sprintf(cindex, "%i", cs_index); // convert index to a string since we don't have any other way to store values that are greater than a byte (and max_configstrings is 1024 currently) - FIXME: try to replace by a WriteLong instead of WriteString? Or WriteData (with length! and it uses WriteByte)
	MSG_WriteString(&msg, (const char*)cindex);
	MSG_WriteString(&msg, cs_string);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteClientConfigString

Write a client configstring to the demo file
Note: this is a bit redundant with SV_DemoWriteUserinfo, because clientCommand userinfo sets the new userinfo for a democlient, and clients configstrings are always derived from userinfo string. But this function is left for security purpose (so we make sure the configstring is set right).
====================
*/
void SV_DemoWriteClientConfigString( int clientNum, const char *cs_string )
{
	msg_t msg;

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientConfigString);
	MSG_WriteByte(&msg, clientNum);
	MSG_WriteString(&msg, cs_string);
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteClientUserinfo

Write a client userinfo to the demo file (client_t fields can almost all be filled from the userinfo string)
Note: in practice, clients userinfo should be loaded before their configstrings (because configstrings derive from userinfo in a real game)
====================
*/
void SV_DemoWriteClientUserinfo( client_t *client, const char *userinfo )
{
	msg_t msg;
	static char fuserinfodata[MAX_STRING_CHARS];
	static char *fuserinfo = fuserinfodata;

	Q_strncpyz(fuserinfo, userinfo, sizeof(fuserinfodata)); // copy the client's userinfo to another storage var, so that we don't modify the client's userinfo (else the filtering will clean its infos such as cl_guid and ip, and next map change the client will be disconnected because of a wrong guid/ip!)

	if (strlen(userinfo) > 0) { // do the filtering only if the string is not empty
		SV_DemoFilterClientUserinfo( fuserinfo ); // filters out privacy keys such as ip, cl_guid, cl_voip
	}

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientUserinfo); // write the event marker
	MSG_WriteByte(&msg, client - svs.clients); // write the client number (client_t - svs.clients = client num int)
	MSG_WriteString(&msg, fuserinfo); // write the (filtered) userinfo string
	SV_DemoWriteMessage(&msg); // commit this demo event in the demo file
}

/*
====================
SV_DemoWriteClientUsercmd

Write a client usercmd_t for the current packet (called from sv_client.c SV_UserMove) which contains the movements commands for the player
Note: this is unnecessary to make players move, this is handled by entities management. This is only used to add more data to the dama for data analysis AND avoid inactivity timer activation
Note2: enabling this feature will use a LOT more storage space, so enable it only if you will really use it. Generally, you're probably better off leaving it disabled.
====================
*/
/*
void SV_DemoWriteClientUsercmd( client_t *cl, qboolean delta, int cmdCount, usercmd_t *cmds, int key )
{
	msg_t msg;
	usercmd_t nullcmd;
	usercmd_t *cmd, *oldcmd;
	int i;

	//Com_Printf("DebugGBOWriteusercmd: %i\n", cl - svs.clients);

	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_clientUsercmd);
	MSG_WriteByte(&msg, cl - svs.clients);
	if (delta)
		MSG_WriteByte(&msg, 1);
	else
		MSG_WriteByte(&msg, 0);
	MSG_WriteByte(&msg, cmdCount);

	Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
	oldcmd = &nullcmd;
	for ( i = 0 ; i < cmdCount ; i++ ) {
		cmd = &cmds[i];
		MSG_WriteDeltaUsercmdKey( &msg, key, oldcmd, cmd );
		oldcmd = cmd;
	}
	SV_DemoWriteMessage(&msg);
}
*/

/*
====================
SV_DemoWriteAllEntityShared

Write all active clients playerState (playerState_t)
Note: this is called at every game's endFrame.
Note2: Contrary to the other DemoWrite functions, this one writes all entities at once in one message, instead of one entity/command per message.
====================
*/
void SV_DemoWriteAllPlayerState(void)
{
	msg_t msg;
	playerState_t *player;
	int i;

	// Initiliaze msg
	MSG_Init(&msg, buf, sizeof(buf));

	// Write clients playerState (playerState_t)
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state < CS_ACTIVE)
			continue;
		player = SV_GameClientNum(i);
		MSG_WriteByte(&msg, demo_playerState);
		MSG_WriteByte(&msg, i);
		MSG_WriteDeltaPlayerstate(&msg, &sv.demoPlayerStates[i], player);
		sv.demoPlayerStates[i] = *player;
	}

	// Commit all these datas to the demo file
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteAllEntityState

Write all entities state (gentity_t->entityState_t)
Note: this is called at every game's endFrame.
Note2: Contrary to the other DemoWrite functions, this one writes all entities at once in one message, instead of one entity/command per message. This could be easily changed, but I'm not sure it would be beneficial for the CPU time and demo storage.
====================
*/
void SV_DemoWriteAllEntityState(void)
{
	msg_t msg;
	sharedEntity_t *entity;
	int i;

	// Initiliaze msg
	MSG_Init(&msg, buf, sizeof(buf));

	// Write entities (gentity_t->entityState_t or concretely sv.gentities[num].s, in gamecode level. instead of sv.)
	MSG_WriteByte(&msg, demo_entityState);
	for (i = 0; i < sv.num_entities; i++)
	{
		if (i >= sv_maxclients->integer && i < MAX_CLIENTS)
			continue;
		entity = SV_GentityNum(i);
		entity->s.number = i;
		MSG_WriteDeltaEntity(&msg, &sv.demoEntities[i].s, &entity->s, qfalse);
		sv.demoEntities[i].s = entity->s;
	}
	MSG_WriteBits(&msg, ENTITYNUM_NONE, GENTITYNUM_BITS); // End marker/Condition to break: since we don't know prior how many entities we store, when reading  the demo we will use an empty entity to break from our while loop

	// Commit all these datas to the demo file
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteAllEntityShared

Write all entities (gentity_t->entityShared_t)
Note: this is called at every game's endFrame.
Note2: Contrary to the other DemoWrite functions, this one writes all entities at once in one message, instead of one entity/command per message.
====================
*/
void SV_DemoWriteAllEntityShared(void)
{
	msg_t msg;
	sharedEntity_t *entity;
	int i;

	// Initiliaze msg
	MSG_Init(&msg, buf, sizeof(buf));

	// Write entities (gentity_t->entityShared_t or concretely sv.gentities[num].r, in gamecode level. instead of sv.)
	MSG_WriteByte(&msg, demo_entityShared);
	for (i = 0; i < sv.num_entities; i++)
	{
		if (i >= sv_maxclients->integer && i < MAX_CLIENTS)
			continue;
		entity = SV_GentityNum(i);
		MSG_WriteDeltaSharedEntity(&msg, &sv.demoEntities[i].r, &entity->r, qfalse, i);
		sv.demoEntities[i].r = entity->r;
	}
	MSG_WriteBits(&msg, ENTITYNUM_NONE, GENTITYNUM_BITS); // End marker/Condition to break: since we don't know prior how many entities we store, when reading  the demo we will use an empty entity to break from our while loop

	// Commit all these datas to the demo file
	SV_DemoWriteMessage(&msg);
}

/*
====================
SV_DemoWriteFrame

Record all the entities (gentities fields) and players (player_t fields) at the end of every frame (this is the only write function to be called in every frame for sure)
Will be called once per server's frame
Called in the main server's loop SV_Frame() in sv_main.c
Note that this function could be called DemoWriteEndFrame, because it writes once at the end of every frame (the other events are written whenever they happen using hooks)
====================
*/
void SV_DemoWriteFrame(void)
{
	msg_t msg;

	// STEP1: write all entities states at the end of the frame

	// Write entities (gentity_t->entityState_t or concretely sv.gentities[num].s, in gamecode level. instead of sv.)
	SV_DemoWriteAllEntityState();

	// Write entities (gentity_t->entityShared_t or concretely sv.gentities[num].r, in gamecode level. instead of sv.)
	SV_DemoWriteAllEntityShared();

	// Write clients playerState (playerState_t)
	SV_DemoWriteAllPlayerState();

	//-----------------------------------------------------

	// STEP2: write the endFrame marker and server time

	MSG_Init(&msg, buf, sizeof(buf));

	// Write end of frame marker: this will commit every demo entity change (and it's done at the very end of every server frame to overwrite any change the gamecode/engine may have done)
	MSG_WriteByte(&msg, demo_endFrame);

	// Write server time (will overwrite the server time every end of frame)
	MSG_WriteLong(&msg, sv.time);

	// Commit data to the demo file
	SV_DemoWriteMessage(&msg);
}



/***********************************************
 * DEMO READING FUNCTIONS
 *
 * Functions to read demo events
 ***********************************************/

/*
====================
SV_DemoReadClientCommand

Replay a client command
Client command management (generally automatic, such as tinfo for HUD team overlay status, team selection, etc.) - except userinfo command that is managed by another event
====================
*/
void SV_DemoReadClientCommand( msg_t *msg )
{
	char	*cmd;
	int num;

	Cmd_SaveCmdContext(); // Save the context (tokenized strings) so that the engine can continue its usual processing normally just after this function
	num = MSG_ReadByte(msg);
	cmd = MSG_ReadString(msg);

	SV_ExecuteClientCommand(&svs.clients[num], cmd, qtrue); // 3rd arg = clientOK, and it's necessarily true since we saved the command in the demo (else it wouldn't be saved)

	Cmd_RestoreCmdContext(); // Restore the context (tokenized strings)
}

/*
====================
SV_DemoReadServerCommand

Replay a server command
Server command management - except print/cp (dropped at recording because already handled by gameCommand),
====================
*/
void SV_DemoReadServerCommand( msg_t *msg )
{
	char	*cmd;

	Cmd_SaveCmdContext();
	cmd = MSG_ReadString(msg);
	SV_SendServerCommand(NULL, "%s", cmd);
	Cmd_RestoreCmdContext();
}

/*
====================
SV_DemoReadGameCommand

Replay a game command (only game commands sent to all clients)
Game command management - such as prints/centerprint (cp) scores command - except chat/tchat (handled by clientCommand) - basically the same as demo_serverCommand (because sv_GameSendServerCommand uses SV_SendServerCommand, but game commands are safe to be replayed to everyone, while server commands may be unsafe such as disconnect)
====================
*/
void SV_DemoReadGameCommand( msg_t *msg )
{
	char	*cmd;

	Cmd_SaveCmdContext();

	MSG_ReadByte(msg); // clientNum, useless here so we don't save it, but we need to read it in order to move the msg cursor
	cmd = MSG_ReadString(msg);

	if ( SV_CheckLastCmd( cmd, qfalse ) ) { // check for duplicates: check that the engine did not already send this very same message resulting from an event (this means that engine gamecommands are never filtered, only demo gamecommands)
		SV_GameSendServerCommand( -1, cmd ); // send this game command to all clients (-1)
	}

	Cmd_RestoreCmdContext();
}

/*
====================
SV_DemoReadConfigString

Read a configstring from a message and load it into memory
====================
*/
void SV_DemoReadConfigString( msg_t *msg )
{
	char	*configstring;
	int num;

	//num = MSG_ReadLong(msg, MAX_CONFIGSTRINGS); FIXME: doesn't work, dunno why, but it would be better than a string to store a long int!
	num = atoi(MSG_ReadString(msg));
	configstring = MSG_ReadString(msg);

	if ( num < CS_PLAYERS + sv_democlients->integer || num >= CS_PLAYERS + sv_maxclients->integer ) { // we make sure to not overwrite real client configstrings (else when the demo starts, normal players will have no name, no model and no status!) - this cannot be done at recording time because we can't know how many sv_maxclients will be set at replaying time
		SV_SetConfigstring(num, configstring);
	}
}

/*
====================
SV_DemoReadClientConfigString

Read a demo client configstring from a message, load it into memory and broadcast changes to gamecode and clients
This function also manages demo clientbegin at connections and teamchange (which are normally totally handled by the gamecode, so we can't directly access nor store these events in the demo, we must use clever ways to reproduce them at the right time)
====================
*/
void SV_DemoReadClientConfigString( msg_t *msg )
{
	client_t *client;
	char	*configstring;
	int num;

	num = MSG_ReadByte(msg);
	configstring = MSG_ReadString(msg);

	/**** DEMOCLIENTS CONNECTION MANAGEMENT  ****/
	// This part manages when a client should begin or be dropped based on the configstrings. This is a workaround because begin and disconnect events are managed in the gamecode, so we here use a clever way to know when these events happen (this is based on a careful reading of how work the mechanisms that manage players in a real game, so this should be OK in any case).
	// Note: this part could also be used in userinfo instead of client configstrings (but since DropClient automatically sets userinfo to null, which is not the case the other way around, this way was preferred)
	if ( Q_stricmp(sv.configstrings[CS_PLAYERS + num], configstring) && configstring && strlen(configstring) ) { // client begin or just changed team: previous configstring and new one are different, and the new one is not null

		client = &svs.clients[num];

		int svdoldteam;
		int svdnewteam;
		svdoldteam = strlen(Info_ValueForKey(sv.configstrings[CS_PLAYERS + num], "t")) ? atoi(Info_ValueForKey(sv.configstrings[CS_PLAYERS + num], "t")) : -1; // affect the new team if detected, else if an empty string is returned, just set -1 (will allow us to detect that there's really no team change instead of having 0 which is TEAM_FREE)
		svdnewteam = strlen(Info_ValueForKey(configstring, "t")) ? atoi(Info_ValueForKey(configstring, "t")) : -1;

		// Set the client configstring (using a standard Q3 function)
		SV_SetConfigstring(CS_PLAYERS + num, configstring);

		// Set some infos about this user:
		svs.clients[num].demoClient = qtrue; // to check if a client is a democlient, you can either rely on this variable, either you can check if num (index of client) is >= CS_PLAYERS + sv_democlients->integer && < CS_PLAYERS + sv_maxclients->integer (if it's not a configstring, remove CS_PLAYERS from your if)
		Q_strncpyz( svs.clients[num].name, Info_ValueForKey( configstring, "n" ), MAX_NAME_LENGTH ); // set the name (useful for internal functions such as status_f). Anyway userinfo will automatically set both name (server-side) and netname (gamecode-side).


		// DEMOCLIENT INITIAL TEAM MANAGEMENT
		// Note: needed only to set the initial team of the democlients, subsequent team changes are directly handled by their clientCommands
		// DEPRECATED: moved to userinfo, more interoperable (because here team is an int, while in userinfo the full team name string is stored and can be directly replayed)
		if ( sv_gametype->integer != GT_TOURNAMENT ) {
			if ( !strlen(Info_ValueForKey(svs.clients[num].userinfo, "team")) ) {
				if (configstring && strlen(configstring) &&
				    ( svdoldteam == -1 || (svdoldteam != svdnewteam && svdnewteam != -1) ) // if there was no team for this player before or if the new team is different
				    ) {
					// If the client changed team, we manually issue a team change (workaround by using a clientCommand team)
					char *svdnewteamstr = malloc( 10 * sizeof *svdnewteamstr );

					if (svdnewteam == TEAM_SPECTATOR) {
						strcpy(svdnewteamstr, "spectator");
					} else {
						strcpy(svdnewteamstr, "o"); // random string, we just want the server to considerate the democlient in a team, whatever the team is. It will be automatically adjusted later with a clientCommand or userinfo string.
					}

					SV_ExecuteClientCommand(&svs.clients[num], va("team %s", svdnewteamstr), qtrue); // workaround to force the server's gamecode and clients to update the team for this client - note: in fact, setting any team (except spectator) will make the engine set the client to a random team, but it's only sessionTeam! so the democlients will still be shown in the right team on the scoreboard, but the engine will consider them in a random team (this has no concrete adverse effect to the demo to my knowledge)

					free( svdnewteamstr );
				}
			}
		}

		// Set the remoteAddress of this client to localhost (this will set "ip\localhost" in the userinfo, which will in turn force the gamecode to set this client as a localClient, which avoids inactivity timers and some other stuffs to be triggered)
		if (strlen(configstring)) // we check that the client isn't being dropped
			NET_StringToAdr( "localhost", &client->netchan.remoteAddress, NA_LOOPBACK );

		// Make sure the gamecode consider the democlients (this will allow to show them on the scoreboard and make them spectatable with a standard follow) - does not use argv (directly fetch client infos from userinfo) so no need to tokenize with Cmd_TokenizeString()
		// Note: this also triggers the gamecode refreshing of the client's userinfo
		VM_Call( gvm, GAME_CLIENT_BEGIN, num );
	} else if ( Q_stricmp(sv.configstrings[CS_PLAYERS + num], configstring) && strlen(sv.configstrings[CS_PLAYERS + num]) && (!configstring || !strlen(configstring)) ) { // client disconnect: different configstrings and the new one is empty, so the client is not there anymore, we drop him (also we check that the old config was not empty, else we would be disconnecting a player who is already dropped)
		client = &svs.clients[num];
		SV_DropClient( client, "disconnected" ); // same as SV_Disconnect_f(client);
		SV_SetConfigstring(CS_PLAYERS + num, configstring); // empty the configstring
	} else { // In any other case (should there be?), we simply set the client configstring (which should not produce any error)
		SV_SetConfigstring(CS_PLAYERS + num, configstring);
	}
}

/*
====================
SV_DemoReadClientUserinfo

Read a demo client userinfo string from a message, load it into memory, fills client_t fields by parsing the userinfo and broacast the change to the gamecode and clients
Note: this function also manage the initial team of democlients when demo recording has started. Subsequent team changes will be directly handled by clientCommands "team"
====================
*/
void SV_DemoReadClientUserinfo( msg_t *msg )
{
	client_t *client;
	char	*userinfo; // = malloc( MAX_INFO_STRING * sizeof *userinfo);
	int num;

	// Save context
	Cmd_SaveCmdContext();
	// Get client
	num = MSG_ReadByte(msg);
	client = &svs.clients[num];
	// Get userinfo
	userinfo = MSG_ReadString(msg);

	// Get the old and new team for the client
	char *svdoldteam = malloc( MAX_NAME_LENGTH * sizeof *svdoldteam );
	char *svdnewteam = malloc( MAX_NAME_LENGTH * sizeof *svdnewteam );
	Q_strncpyz(svdoldteam, Info_ValueForKey(client->userinfo, "team"), MAX_NAME_LENGTH);
	Q_strncpyz(svdnewteam, Info_ValueForKey(userinfo, "team"), MAX_NAME_LENGTH);

	// Set the remoteAddress of this client to localhost (this will set "ip\localhost" in the userinfo, which will in turn force the gamecode to set this client as a localClient, which avoids inactivity timers and some other stuffs to be triggered)
	if (strlen(userinfo)) // we check that the client isn't being dropped (in which case we shouldn't set an address)
		NET_StringToAdr( "localhost", &client->netchan.remoteAddress, NA_LOOPBACK );

	// Update the userinfo for both the server and gamecode
	Cmd_TokenizeString( va("userinfo %s", userinfo) ); // we need to prepend the userinfo command (or in fact any word) to tokenize the userinfo string to the second index because SV_UpdateUserinfo_f expects to fetch it with Argv(1)
	SV_UpdateUserinfo_f(client); // will update the server userinfo, automatically fill client_t fields and then transmit to the gamecode and call ClientUserinfoChanged() which will also update the gamecode's client_t from the new userinfo (eg: name [server-side] and netname [gamecode-side] will be both updated)


	// DEMOCLIENT INITIAL TEAM MANAGEMENT
	// Note: it is more interoperable to do team management here than in configstrings because here we have the team name as a string, so we can directly issue it in a "team" clientCommand
	// Note2: this function is only necessary to set the initial team for democlients (the team they were at first when the demo started), for all the latter team changes, the clientCommands are recorded and will be replayed
	if ( sv_gametype->integer != GT_TOURNAMENT ) { // if it's tournament, playing democlients shouldn't send a team command, else they will be set back to spectator waiting queue (and so they won't be spectatable anymore)!) FIXME: all democlients (even spec) will be spectatable, but how to fix that when clients can have an empty team (at connection, until they issue a team clientCommand)?
		if (userinfo && strlen(userinfo) && strlen(svdnewteam) &&
		    ( !strlen(svdoldteam) || (Q_stricmp(svdoldteam, svdnewteam) && strlen(svdnewteam)) ) // if there was no team for this player before OR if the new team is different
		    ) {
			// If the democlient changed team, we manually issue a team change (workaround by using a clientCommand team)
			SV_ExecuteClientCommand(client, va("team %s", svdnewteam), qtrue); // workaround to force the server's gamecode and clients to update the team for this client

		} else if (!strlen(svdoldteam) && !strlen(svdnewteam) && strlen(userinfo) &&
			   !strlen(Info_ValueForKey(sv.configstrings[CS_PLAYERS + num], "t"))
			   ) { // old and new team are not specified in the previous and current userinfo, but a userinfo is present
			// Else if the democlient has no team specified, it's probably because he just has connected and so he is set to the default team by the gamecode depending on the gamecode: for >= GT_TEAM it's spectator, for all the others non-team based gametypes it's direcly in-game
			// FIXME? If you are trying to port this patch and weirdly some democlients are visible in scoreboard but can't be followed, try to uncomment these lines
			if (sv_gametype->integer >= GT_TEAM && demoTeamAutoJoin <= 0) { // if it's a team-based gametype, by default players are spectators (unless g_teamAutoJoin was set)
				SV_ExecuteClientCommand(client, "team spectator", qtrue); // send to spectator the client (prevent the democlient from being followed)
			} else { // else by default they join the game
				SV_ExecuteClientCommand(client, "team o", qtrue);
			}
		}
	}

	// Free memory
	//free( userinfo ); // automatically freed by glibc, if uncommented will produce a crash
	free( svdoldteam );
	free( svdnewteam );

	// Restore context
	Cmd_RestoreCmdContext();
}

/*
====================
SV_DemoReadClientUsercmd

Read the usercmd_t for a democlient and restituate the movements - this is NOT needed to make democlients move, this is handled by entities management, but it should avoid inactivity timer to activate and can be used for demo analysis
FIXME: should set ucmd->serverTime = client->ps.commandTime + 2 to avoid the dropping of the usercmds_t packets, see g_active.c
====================
*/
/*
void SV_DemoReadClientUsercmd( msg_t *msg )
{
	int num, deltaint;
	qboolean delta;

	Cmd_SaveCmdContext(); // Save the context (tokenized strings) so that the engine can continue its usual processing normally just after this function
	num = MSG_ReadByte(msg);
	deltaint = MSG_ReadByte(msg);
	if (deltaint)
		delta = qtrue;
	else
		delta = qfalse;
	//Com_Printf("DebugGBOReadusercmd: %i\n", num);
	SV_UserMove(&svs.clients[num], msg, delta);
	Cmd_RestoreCmdContext(); // Restore the context (tokenized strings)
}
*/

/*
====================
SV_DemoReadAllPlayerState

Read all democlients playerstate (playerState_t) from a message and store them in a demoPlayerStates array (it will be loaded in memory later when SV_DemoReadRefresh() is called)
====================
*/
void SV_DemoReadAllPlayerState( msg_t *msg )
{
	playerState_t *player;
	int num;

	num = MSG_ReadByte(msg);
	player = SV_GameClientNum(num);
	MSG_ReadDeltaPlayerstate(msg, &sv.demoPlayerStates[num], player);
	sv.demoPlayerStates[num] = *player;
}

/*
====================
SV_DemoReadAllEntityState

Read all entities state (gentity_t or sharedEntity_t->entityState_t) from a message and store them in a demoEntities[num].s array (it will be loaded in memory later when SV_DemoReadRefresh() is called)
====================
*/
void SV_DemoReadAllEntityState( msg_t *msg )
{
	sharedEntity_t *entity;
	int num;

    // For each state in this message (because we store a concatenation of all entities states in a single message, we could do otherwise, but that's the design)
	while (1)
	{
        // Get entity num
		num = MSG_ReadBits(msg, GENTITYNUM_BITS);
        // Reached the end of the message, stop here
		if (num == ENTITYNUM_NONE)
			break;
        // Create a blank entity
		entity = SV_GentityNum(num);
        // Interpolate the new entity state from previous state in sv.demoEntities
		MSG_ReadDeltaEntity(msg, &sv.demoEntities[num].s, &entity->s, num);

        // Fix mover movements, (avoid the "bad moverstate" error)
        if (entity->s.eType == ET_MOVER) {
            // 1st way to fix: completely disable movers (but not their displacement effects, so they will be invisible, but players are still moved by movers)
            //entity->s.eType = ET_GENERAL;

            // 2nd way to fix (better): only binarymovers are producing the bug, because the game will be confused about the moverState since we cannot have access nor set it...
            // the solution: avoid changing the moverState, only change the position. To do this, avoid calls to ent->reached, so in other words never allow s.pot.trType to be TR_LINEAR_STOP (other types of movers are not affected since they don't have a stop/reached state, they are just looping over and over)
            if (entity->s.pos.trType == TR_LINEAR_STOP) { // mover reached end of movement? change it...
                entity->s.pos.trType = TR_LINEAR;  // ... to always set movers in a moving linear state.
                //entity->s.apos.trType = TR_LINEAR; // should apos be also set?
            }
        }

        // Save new entity state (in sv.demoEntities, which in other words display the new state)
		sv.demoEntities[num].s = entity->s;
	}
}

/*
====================
SV_DemoReadAllEntityShared

Read all shared entities (gentity_t or sharedEntity_t->entityShared_t - NOTE: sharedEntity_t = gentity_t != entityShared_t which is a subfield of gentity_t) from a message and store them in a demoEntities[num].r array (it will be loaded in memory later when SV_DemoReadRefresh() is called)
====================
*/
void SV_DemoReadAllEntityShared( msg_t *msg )
{
	sharedEntity_t *entity;
	int num;

	while (1) // loop until we read the whole message (which is storing several states)
	{
        // Get entity num
		num = MSG_ReadBits(msg, GENTITYNUM_BITS);
        // Reached end of message, stop here
		if (num == ENTITYNUM_NONE)
			break;
        // Load an empty entity
		entity = SV_GentityNum(num);
        // Interpolate the new entity state from previous state in sv.demoEntities
		MSG_ReadDeltaSharedEntity(msg, &sv.demoEntities[num].r, &entity->r, num);

		entity->r.svFlags &= ~SVF_BOT; // fix bots camera freezing issues - because since now the engine will consider these democlients just as normal players, it won't be using anymore special bots fields and instead just use the standard viewangles field to replay the camera movements

		// Link/unlink the entity
		if (entity->r.linked && (!sv.demoEntities[num].r.linked ||
		    entity->r.linkcount != sv.demoEntities[num].r.linkcount))
			SV_LinkEntity(entity);
		else if (!entity->r.linked && sv.demoEntities[num].r.linked)
			SV_UnlinkEntity(entity);

        // Save the new state in sv.demoEntities (ie, display current entity state)
		sv.demoEntities[num].r = entity->r;
		if (num > sv.num_entities)
			sv.num_entities = num;
	}
}

/*
====================
SV_DemoReadRefreshEntities

Load into memory all stored demo players states and entities (which effectively overwrites the one that were previously written by the game since SV_ReadFrame is called at the very end of every game's frame iteration).
====================
*/
void SV_DemoReadRefreshEntities( void )
{
	int i;

	// Overwrite anything the game may have changed
	for (i = 0; i < sv.num_entities; i++)
	{
		if (i >= sv_democlients->integer && i < MAX_CLIENTS) // FIXME? shouldn't MAX_CLIENTS be sv_maxclients->integer?
			continue;
		*SV_GentityNum(i) = sv.demoEntities[i]; // Overwrite entities
	}
	for (i = 0; i < sv_democlients->integer; i++)
		*SV_GameClientNum(i) = sv.demoPlayerStates[i]; // Overwrite player states
}

/*
====================
SV_DemoReadRefreshPlayersHealth

Update all demoplayers health (will be reflected on the HUD when spectated - for team overlay see tinfo clientcommand)
Note: this function should always be called after SV_DemoReadRefreshEntities() (or you can also change SV_GameClientNum(i) to sv.demoPlayerStates[i], but make sure the entity already has updated its health state...)
====================
*/
void SV_DemoReadRefreshPlayersHealth( void )
{
	int i;

	// Update all players' health in HUD
	for (i = 0; i < sv_democlients->integer; i++)
	{
		SV_GentityUpdateHealthField( SV_GentityNum(i), SV_GameClientNum(i) ); // Update the health with the value of playerState_t->stats[STAT_HEALTH]
	}
}

/*
====================
SV_DemoReadFrame

Play a frame from the demo file
This function will read one frame per call, and will process every events contained (switch to the next event when meeting the demo_EOF marker) until it meets the end of the frame (demo_endDemo marker)
Called in the main server's loop SV_Frame() in sv_main.c (it's called after any other event processing, so that it overwrites anything the game may have loaded into memory, but before the entities are broadcasted to the clients)
====================
*/
void SV_DemoReadFrame(void)
{
	msg_t msg;
	int cmd, r;

	static int memsvtime;
	static int currentframe = -1;

read_next_demo_frame: // used to read another whole demo frame

	// Demo freezed? Just stop reading the demo frames
	if (Cvar_VariableIntegerValue("cl_freezeDemo")) {
		sv.time = memsvtime; // reset server time to the same time as the previous frame, to avoid the time going backward when resuming the demo (which will disconnect every players)
		return;
	}

	// Update timescale
	currentframe++; // update the current frame number

	if (com_timescale->value < 1.0) { // Check timescale: if slowed timescale (below 1.0), then we check that we pass one frame on 1.0/com_timescale (eg: timescale = 0.5, 1.0/0.5=2, so we pass one frame on two)
		if (currentframe % (int)(1.0/com_timescale->value) != 0) { // if it's not yet the right time to read the frame, we just pass and wait for the next server frame to read this demo frame
			return;
		}
	}

	// Initialize / reinitialize the msg buffer
	MSG_Init(&msg, buf, sizeof(buf));

	while (1)
	{
read_next_demo_event: // used to read next demo event

		// Get a message
		r = FS_Read(&msg.cursize, 4, sv.demoFile);
		if (r != 4)
		{
			Com_Error(ERR_DROP, "DEMOERROR: SV_DemoReadFrame: Demo file is corrupted\n");
			SV_DemoStopPlayback();
			return;
		}
		msg.cursize = LittleLong(msg.cursize); // get the size of the next demo message
		if (msg.cursize > msg.maxsize) // if the size is too big, we throw an error
			Com_Error(ERR_DROP, "DEMOERROR: SV_DemoReadFrame: demo message too long\n");
		r = FS_Read(msg.data, msg.cursize, sv.demoFile); // fetch the demo message (using the length we got) from the demo file sv.demoFile, and store it into msg.data (will be accessed automatically by MSG_thing() functions), and store in r the length of the data returned (used to check that it's correct)
		if (r != msg.cursize) // if the returned length of the read demo message is not the same as the length we expected (the one that was stored just prior to the demo message), we return an error because we miss the demo message, and the only reason is that the file is truncated, so there's nothing to read after
		{
			Com_Printf("DEMOERROR: Demo file was truncated.\n");
			SV_DemoStopPlayback();
			return;
		}

		// Parse the message
		while (1)
		{
			cmd = MSG_ReadByte(&msg); // Read the demo message marker
			switch (cmd) // switch to the right processing depending on the type of the marker
			{
				default:
					if ( sv_demoTolerant->integer ) { // Error tolerance mode: if we encounter an unknown demo message, we just skip to the next (this may allow for retrocompatibility)
						MSG_Clear(&msg);
						goto read_next_demo_event;
					} else { // else we just drop the demo and throw a big fat error
						Com_Error(ERR_DROP, "SV_DemoReadFrame: Illegible demo message\n");
						return;
					}
				case demo_EOF: // end of a demo event (the loop will continue to real the next event)
					MSG_Clear(&msg);
					goto read_next_demo_event;
				case demo_configString: // general configstrings setting (such as capture scores CS_SCORES1/2, etc.) - except clients configstrings
					SV_DemoReadConfigString( &msg );
					break;
				case demo_clientConfigString: // client configstrings setting and clients status management
					SV_DemoReadClientConfigString( &msg );
					break;
				case demo_clientUserinfo: // client userinfo setting and client_t fields management
					SV_DemoReadClientUserinfo( &msg );
					break;
				case demo_clientCommand: // client command management (generally automatic, such as tinfo for HUD team overlay status, team selection, etc.) - except userinfo command that is managed by another event
					SV_DemoReadClientCommand( &msg );
					break;
				case demo_serverCommand: // server command management - except print/cp (already handled by gameCommand),
					SV_DemoReadServerCommand( &msg );
					break;
				case demo_gameCommand: // game command management - such as prints/centerprint (cp) scores command - except chat/tchat (handled by clientCommand) - basically the same as demo_serverCommand (because sv_GameSendServerCommand uses SV_SendServerCommand, but game commands are safe to be replayed to everyone, while server commands may be unsafe such as disconnect)
					SV_DemoReadGameCommand( &msg );
					break;
				case demo_playerState: // manage playerState_t (some more players game status management, see demo_endFrame)
					SV_DemoReadAllPlayerState( &msg );
					break;
				case demo_entityState: // manage gentity->entityState_t (some more gentities game status management, see demo_endFrame)
					SV_DemoReadAllEntityState( &msg );
					break;
				case demo_entityShared: // gentity_t->entityShared_t management (see g_local.h for more infos)
					SV_DemoReadAllEntityShared( &msg );
					break;
				/*
				case demo_clientUsercmd:
					SV_DemoReadClientUsercmd( &msg );
					break;
				*/
				case demo_endFrame: // end of the frame - players and entities game status update: we commit every demo entity to the server, update the server time, then release the demo frame reading here to the next server (and demo) frame
					// Update entities
					SV_DemoReadRefreshEntities(); // load into memory the demo entities (overwriting any change the game may have done)
					// Update all players' health in HUD
					SV_DemoReadRefreshPlayersHealth();
					// Set the server time
					sv.time = MSG_ReadLong(&msg); // refresh server in-game time (overwriting any change the game may have done)
					memsvtime = sv.time; // keep memory of the last server time, in case we want to freeze the demo

					if (com_timescale->value > 1.0) { // Check for timescale: if timescale is faster (above 1.0), we read more frames at once (eg: timescale=2, we read 2 frames for one call of this function)
						if (currentframe % (int)(com_timescale->value) != 0) { // Check that we've read all the frames we needed
							goto read_next_demo_frame; // if not true, we read another frame
						}
					}

					return; // else we end the current demo frame
				case demo_endDemo: // end of the demo file - just stop playback and restore saved cvars
					SV_DemoStopPlayback();
					return;
			}
		}
	}
}



/***********************************************
 * DEMO MANAGEMENT FUNCTIONS
 *
 * Functions to start/stop the recording/playback of a demo file
 ***********************************************/

/*
====================
SV_DemoAutoDemoRecord

Generates a meaningful demo filename and automatically starts the demo recording.
This function is used in conjunction with the variable sv_autoDemo 1
Note: be careful, if the hostname contains bad characters, the demo may not be able to be saved at all! There's a small filtering in place but a bad filename may pass through!
Note2: this function is called at MapRestart and SpawnServer (called in Map func), but in no way it's called right at the time it's set.
====================
*/
void SV_DemoAutoDemoRecord(void)
{
	qtime_t now;
	Com_RealTime( &now );
    char *demoname = malloc ( MAX_QPATH * sizeof * demoname );
    

	Q_strncpyz(demoname, va( "%s_%04d-%02d-%02d-%02d-%02d-%02d_%s",
			SV_CleanFilename(va("%s", sv_hostname->string)),
                        1900 + now.tm_year,
                        1 + now.tm_mon,
                        now.tm_mday,
                        now.tm_hour,
                        now.tm_min,
                        now.tm_sec,
                        SV_CleanFilename(Cvar_VariableString( "mapname" )) ),
                        MAX_QPATH);

	Com_Printf("DEMO: recording a server-side demo to: %s/svdemos/%s.%s%d\n",  strlen(Cvar_VariableString("fs_game")) ?  Cvar_VariableString("fs_game") : BASEGAME, demoname, SVDEMOEXT, PROTOCOL_VERSION);

	Cbuf_AddText( va("demo_record %s\n", demoname ) );

	free(demoname);
}

/*
====================
SV_DemoStartRecord

Start the recording of a demo by saving some headers data (such as the number of clients, mapname, gametype, etc..)
sv.demo* have already been set and the demo file opened, start writing gamestate info
====================
*/
void SV_DemoStartRecord(void)
{
	msg_t msg;
	int i;

	// Set democlients to 0 since it's only used for replaying demo
	Cvar_SetValue("sv_democlients", 0);

	MSG_Init(&msg, buf, sizeof(buf));

	// Write number of clients (sv_maxclients < MAX_CLIENTS or else we can't playback)
	MSG_WriteString(&msg, "clients"); // for each demo meta data (infos about the demo), we prepend the name of the var (this allows for fault tolerance and retrocompatibility) - FIXME? We could also use MSG_LookaheadByte() to read a byte, instead of a string, this would save a tiny bit of storage space
	MSG_WriteByte(&msg, sv_maxclients->integer);
	// Write current server in-game time
	MSG_WriteString(&msg, "time");
	MSG_WriteLong(&msg, sv.time);
	// Write sv_fps
	MSG_WriteString(&msg, "sv_fps");
	MSG_WriteLong(&msg, sv_fps->integer);
	// Write g_gametype
	MSG_WriteString(&msg, "g_gametype");
	MSG_WriteLong(&msg, sv_gametype->integer);
	// Write fs_game (mod name)
	MSG_WriteString(&msg, "fs_game");
	MSG_WriteString(&msg, Cvar_VariableString("fs_game"));
	// Write map name
	MSG_WriteString(&msg, "map");
	MSG_WriteString(&msg, sv_mapname->string);
	// Write timelimit
	MSG_WriteString(&msg, "timelimit");
	MSG_WriteLong(&msg, Cvar_VariableIntegerValue("timelimit"));
	// Write fraglimit
	MSG_WriteString(&msg, "fraglimit");
	MSG_WriteLong(&msg, Cvar_VariableIntegerValue("fraglimit"));
	// Write capturelimit
	MSG_WriteString(&msg, "capturelimit");
	MSG_WriteLong(&msg, Cvar_VariableIntegerValue("capturelimit"));
	// Write g_teamAutoJoin (will allow to know what was the default initial team for newly connected players)
	MSG_WriteString(&msg, "g_teamAutoJoin");
	MSG_WriteLong(&msg, Cvar_VariableIntegerValue("g_teamAutoJoin"));
	// Write sv_hostname (only for info)
	MSG_WriteString(&msg, "hostname");
	MSG_WriteString(&msg, sv_hostname->string);
	// Write current datetime (only for info)
	MSG_WriteString(&msg, "datetime");
	MSG_WriteString(&msg, SV_GenerateDateTime());

	// Write end of meta datas (since we will read a string each loop, we need to set a special string to specify the reader that we end the loop, we cannot use a marker because it's a byte)
	MSG_WriteString(&msg, "endMeta");

	// Write all the above into the demo file
	SV_DemoWriteMessage(&msg);

	// Write all configstrings (such as current capture score CS_SCORE1/2, etc...), including clients configstrings
	// Note: system configstrings will be filtered and excluded (there's a check function for that), and clients configstrings  will be automatically redirected to the specialized function (see the check function)
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if ( &sv.configstrings[i] ) { // if the configstring pointer exists in memory (because we will check all the possible indexes, but we don't know if they really exist in memory and are used or not, so here we check for that)
			SV_DemoWriteConfigString(i, sv.configstrings[i]);
		}
	}

	// Write initial clients userinfo
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		client_t *client = &svs.clients[i];

		if (client->state >= CS_CONNECTED) {

			// store client's userinfo (should be before clients configstrings since clients configstrings are derived from userinfo)
			if (client->userinfo) { // if player is connected and the configstring exists, we store it
				SV_DemoWriteClientUserinfo(client, (const char *)client->userinfo);
			}
		}
	}

	// Write entities and players
	Com_Memset(sv.demoEntities, 0, sizeof(sv.demoEntities));
	Com_Memset(sv.demoPlayerStates, 0, sizeof(sv.demoPlayerStates));
	SV_DemoWriteFrame();
	Com_Printf("Recording demo %s.\n", sv.demoName);
	sv.demoState = DS_RECORDING;
	Cvar_SetValue("sv_demoState", DS_RECORDING);
}

/*
====================
SV_DemoStopRecord

Stop the recording of a demo
Write end of demo (demo_endDemo marker) and close the demo file
====================
*/
void SV_DemoStopRecord(void)
{
	msg_t msg;

	// End the demo
	MSG_Init(&msg, buf, sizeof(buf));
	MSG_WriteByte(&msg, demo_endDemo);
	SV_DemoWriteMessage(&msg);

	FS_FCloseFile(sv.demoFile);
	sv.demoState = DS_NONE;
	Cvar_SetValue("sv_demoState", DS_NONE);
	Com_Printf("Stopped recording demo %s.\n", sv.demoName);
}

/*
====================
SV_DemoRestartPlayback

Just issue again the demo_play command along with the demo filename (used when the system will automatically restart the server for special cvars values to be set up, eg: sv_maxclients, fs_game, g_gametype, etc..)
====================
*/
void SV_DemoRestartPlayback(void)
{
	if ( strlen(savedPlaybackDemoname) ) {
		// Set the demoState from DS_WAITINGPLAYBACK to DS_NONE (this avoids the engine to repeatedly restart the playback until it gets done, here we just launch it once) - this fix messages bug and accelerate the loading of the demo when switching mods
		sv.demoState = DS_NONE;
		Cvar_SetValue("sv_demoState", DS_NONE);
		// Restart the playback (reissue the demo_play command and the demo filename)
		Cbuf_AddText( va("%s\n", savedPlaybackDemoname ) );
	}

	return;
}

/*
====================
SV_DemoStartPlayback

Start the playback of a demo
sv.demo* have already been set and the demo file opened, start reading gamestate info
This function will also check that everything is alright (such as gametype, map, sv_fps, sv_democlients, sv_maxclients, etc.), if not it will try to fix it automatically
Note for developers: this is basically a mirror of SV_DemoStartRecord() but the other way around (there it writes, here it reads) - but only for the part about the headers, for the rest (eg: userinfo, configstrings, playerstates) it's directly managed by ReadFrame.
====================
*/
void SV_DemoStartPlayback(void)
{
	msg_t msg;
	int r, time, i, clients, fps, gametype, timelimit, fraglimit, capturelimit;

	char *map = malloc( MAX_QPATH * sizeof *map );
	char *fs = malloc( MAX_QPATH * sizeof *fs );
	char *hostname = malloc( MAX_NAME_LENGTH * sizeof *hostname );
	char *datetime = malloc( 1024 * sizeof *datetime ); // there's no limit in the whole engine specifically designed for dates and time...
	char *metadata; // = malloc( 1024 * sizeof * metadata ); // used to store the current metadata index

	// Init vars with empty values (to avoid compilation warnings)
	r = i = clients = fps = gametype = timelimit = fraglimit = capturelimit = 0;
	time = 400;

	// Initialize the demo message buffer
	MSG_Init(&msg, buf, sizeof(buf));

	// Get the demo header
	r = FS_Read(&msg.cursize, 4, sv.demoFile);
	if (r != 4)
	{
		Com_Error(ERR_DROP, "DEMOERROR: SV_DemoReadFrame: demo is corrupted (not initialized correctly!)\n");
		SV_DemoStopPlayback();
		return;
	}
	msg.cursize = LittleLong(msg.cursize);
	if (msg.cursize == -1)
	{
		Com_Error(ERR_DROP, "DEMOERROR: SV_DemoReadFrame: demo is corrupted (demo file is empty?)\n");
		SV_DemoStopPlayback();
		return;
	}
	if (msg.cursize > msg.maxsize)
		Com_Error(ERR_DROP, "DEMOERROR: SV_DemoReadFrame: demo message too long\n");
	r = FS_Read(msg.data, msg.cursize, sv.demoFile);
	if (r != msg.cursize)
	{
		Com_Printf("DEMOERROR: Demo file was truncated.\n");
		SV_DemoStopPlayback();
		return;
	}


	// Reading meta-data (infos about the demo)
	// Note: we read with an if statement, so that if in the future we add more meta datas, older demos which haven't these meta datas will still be replayable
	metadata = "";
	while( Q_stricmp(metadata, "endMeta")  )
	{
		metadata = MSG_ReadString( &msg ); // We read the meta data marker

		if ( !Q_stricmp(metadata, "endMeta") ) { // if the string is the special endMeta string, we already break
			break;

		} else if ( !Q_stricmp(metadata, "clients") ) { // democlients
			// Check slots, time and map
			clients = MSG_ReadByte(&msg); // number of democlients (sv_maxclients at the time of the recording)
			if (sv_maxclients->integer < clients || // if we have less real slots than democlients slots or
			    savedMaxClients < 0 || // if there's no savedMaxClients (so this means we didin't change sv_maxclients yet, and we always need to do so since we need to add sv_democlients)
			    sv_maxclients->integer <= savedMaxClients) { // or if maxclients is below or equal to the previous value of maxclients (normally it can only be equal, but if we switch the mod with game_restart, it can get the default value of 8, which can be below, so we need to check that as well)
				Com_Printf("DEMO: Not enough demo slots, automatically increasing sv_democlients to %d and sv_maxclients to %d.\n", clients, sv_maxclients->integer + clients);

				// save the old values of sv_maxclients, sv_democlients and bot_minplayers to later restore them
				if (savedMaxClients < 0) // save only if it's the first value, the subsequent ones may be default values of the engine
					savedMaxClients = sv_maxclients->integer;
				if (savedBotMinPlayers < 0)
					savedBotMinPlayers = Cvar_VariableIntegerValue("bot_minplayers");

				// automatically adjusting sv_democlients, sv_maxclients and bot_minplayers
				Cvar_SetValue("sv_democlients", clients);
				Cvar_SetLatched("sv_maxclients", va("%i", sv_maxclients->integer + clients) );
				/* BUGGY makes a dedicated server crash
				Cvar_Get( "sv_maxclients", "8", 0 );
				sv_maxclients->modified = qfalse;
				*/
			}

		} else if ( !Q_stricmp(metadata, "time") ) { // server time
			// reading server time (from the demo)
			time = MSG_ReadLong(&msg);
			if (time < 400)
			{
				Com_Printf("DEMO: Demo time too small: %d.\n", time);
				SV_DemoStopPlayback();
				return;
			}
		} else if ( !Q_stricmp(metadata, "sv_fps") ) {
			// reading sv_fps (from the demo)
			fps = MSG_ReadLong(&msg);
			if ( sv_fps->integer != fps ) {
				savedFPS = sv_fps->integer;
				Cvar_SetValue("sv_fps", fps);
			}
		} else if ( !Q_stricmp(metadata, "g_gametype") ) {
			// reading g_gametype (from the demo)
			gametype = MSG_ReadLong(&msg);
			// memorize the current gametype
			if ( !keepSaved )
				savedGametype = sv_gametype->integer; // save the gametype before switching

		} else if ( !Q_stricmp(metadata, "fs_game") ) {
			// reading fs_game (mod name)
			Q_strncpyz(fs, MSG_ReadString(&msg), MAX_QPATH);
			if (strlen(fs)){
				Com_Printf("DEMO: Warning: this demo was recorded for the following mod: %s\n", fs);
			}

		} else if ( !Q_stricmp(metadata, "map") ) {
			// reading map (from the demo)
			Q_strncpyz(map, MSG_ReadString(&msg), MAX_QPATH);
			if (!FS_FOpenFileRead(va("maps/%s.bsp", map), NULL, qfalse))
			{
				Com_Printf("Map does not exist: %s.\n", map);
				SV_DemoStopPlayback();
				return;
			}

		} else if ( !Q_stricmp(metadata, "timelimit") ) {
			// reading initial timelimit
			timelimit = MSG_ReadLong(&msg);
			// memorize the current timelimit
			if ( !keepSaved )
				savedTimelimit = Cvar_VariableIntegerValue("timelimit");
			// set the demo setting
			Cvar_SetValue("timelimit", timelimit); // Note: setting the timelimit is NOT necessary for the demo to be replayed (in fact even if the timelimit is reached, the demo will still continue to feed new frames and thus force the game to continue, without any bug - also to the opposite, if the timelimit is too high, the game will be finished when the demo will replay the events, even if the timelimit is not reached but was in the demo, it will be when replaying the demo), but setting it allows for timelimit warning and suddenDeath voice announcement to happen. FIXME: if the timelimit is changed during the game, it won't be reflected in the demo (but the demo will still continue to play, or stop if the game is won).

		} else if ( !Q_stricmp(metadata, "fraglimit") ) {
			// reading initial fraglimit
			fraglimit = MSG_ReadLong(&msg);
			// memorize the current fraglimit
			if ( !keepSaved )
				savedFraglimit = Cvar_VariableIntegerValue("fraglimit");
			// set the demo setting
			Cvar_SetValue("fraglimit", fraglimit); // Note: unnecessary for the demo to be replayed, but allows to show the limit in the HUD. FIXME: if the limit is changed during the game, the new value won't be reflected in the demo (but the demo will continue to play to its integrality)

		} else if ( !Q_stricmp(metadata, "capturelimit") ) {
			// reading initial capturelimit
			capturelimit = MSG_ReadLong(&msg);
			// memorize the current capturelimit
			if ( !keepSaved )
				savedCapturelimit = Cvar_VariableIntegerValue("capturelimit");
			// set the demo setting
			Cvar_SetValue("capturelimit", capturelimit); // Note: unnecessary for the demo to be replayed, but allows to show the limit in the HUD. FIXME: if the limit is changed during the game, the new value won't be reflected in the demo (but the demo will continue to play to its integrality)

		} else if ( !Q_stricmp(metadata, "g_teamAutoJoin") ) {
			// reading g_teamAutoJoin and storing it
			demoTeamAutoJoin = MSG_ReadLong(&msg);

		// Additional infos (not necessary to replay a demo)
		} else if ( !Q_stricmp(metadata, "hostname") ) {
			// reading sv_hostname (additional info)
			Q_strncpyz(hostname, MSG_ReadString(&msg), MAX_NAME_LENGTH);

		} else if ( !Q_stricmp(metadata, "datetime") ) {
			// reading datetime
			Q_strncpyz(datetime, MSG_ReadString(&msg), 1024);
		}
	}

	// g_doWarmup
	if (!keepSaved) { // we memorize values only if it's the first time we launch the playback of this demo, else the values may have already been modified by the demo playback
		// Memorize g_doWarmup
		savedDoWarmup = Cvar_VariableIntegerValue("g_doWarmup");
	}
	// Remove g_doWarmup (bugfix: else it will produce a weird bug with all gametypes except CTF and Double Domination because of CheckTournament() in g_main.c which will make the demo stop after the warmup time)
	Cvar_SetValue("g_doWarmup", 0);

	// g_allowVote
	if (!keepSaved) { // same for g_allowVote
		// Memorize g_allowVote
		savedAllowVote = Cvar_VariableIntegerValue("g_allowVote");
	}
	// Remove g_allowVote (prevent players to call a vote while a map is replaying)
	Cvar_SetValue("g_allowVote", 0);

	// g_teamAutoJoin
	if (!keepSaved) {
		// Memorize g_teamAutoJoin
		savedTeamAutoJoin = Cvar_VariableIntegerValue("g_teamAutoJoin");
	}
	// Remove g_teamAutoJoin (prevent players to call a vote while a map is replaying)
	Cvar_SetValue("g_teamAutoJoin", 0);



	// Printing infos about the demo
	if ( !sv_demoTolerant->integer ) // print the meta datas only if we're not in faults tolerance mode (because if there are missing meta datas, the printing will throw an exception! So we'd better avoid it)
		Com_Printf("DEMO: Details of %s recorded %s on server \"%s\": sv_fps: %i initial_servertime: %i clients: %i fs_game: %s g_gametype: %i map: %s timelimit: %i fraglimit: %i capturelimit: %i \n", sv.demoName, datetime, hostname, fps, time, clients, fs, gametype, map, timelimit, fraglimit, capturelimit);

	// Checking if all initial conditions from the demo are met (map, sv_fps, gametype, servertime, etc...)
	// FIXME? why sv_cheats is needed? Just to be able to use cheats commands to pass through walls?
	if ( !com_sv_running->integer || Q_stricmp(sv_mapname->string, map) ||
	    Q_stricmp(Cvar_VariableString("fs_game"), fs) ||
	    !Cvar_VariableIntegerValue("sv_cheats") ||
	    (time < sv.time && !keepSaved) || // if the demo initial time is below server time AND we didn't already restart for demo playback, then we must restart to reinit the server time (because else, it might happen that the server time is still above demo time if the demo was recorded during a warmup time, in this case we won't restart the demo playback but just iterate a few demo frames in the void to catch up the server time, see below the else statement)
	    sv_maxclients->modified ||
	    (sv_gametype->integer != gametype && !(gametype == GT_SINGLE_PLAYER && sv_gametype->integer == GT_FFA) ) // check for gametype change (need a restart to take effect since it's a latched var) AND check that the gametype difference is not between SinglePlayer and DM/FFA, which are in fact the same gametype (and the server will automatically change SinglePlayer to FFA, so we need to detect that and ignore this automatic change)
	   ) {
		/// Change to the right map/maxclients/mod and restart the demo playback at the next SV_Frame() iteration

		//Cvar_SetValue("sv_democlients", 0); // necessary to stop the playback, else it will produce an error since the demo has not yet started!
		keepSaved = qtrue; // Declare that we want to keep the value saved (and we don't want to restore them now, because the demo hasn't started yet!)
		SV_DemoStopPlayback(); // Stop the demo playback (reset back any change)
		sv.demoState = DS_WAITINGPLAYBACK; // Set the status WAITINGPLAYBACK meaning that as soon as the server will be restarted, the next SV_Frame() iteration must reactivate the demo playback
		Cvar_SetValue("sv_demoState", DS_WAITINGPLAYBACK); // set the cvar too because when restarting the server, all sv.* vars will be destroyed
		Q_strncpyz(savedPlaybackDemoname, Cmd_Cmd(), MAX_QPATH); // we need to copy the value because since we may spawn a new server (if the demo is played client-side OR if we change fs_game), we will lose all sv. vars

		Cvar_SetValue("sv_autoDemo", 0); // disable sv_autoDemo else it will start a recording before we can replay a demo (since we restart the map)

		// **** Automatic mod (fs_game) switching management ****
		if ( ( Q_stricmp(Cvar_VariableString("fs_game"), fs) && strlen(fs) ) ||
		    (!strlen(fs) && Q_stricmp(Cvar_VariableString("fs_game"), fs) && Q_stricmp(fs, BASEGAME) ) ) { // change the game mod only if necessary - if it's different from the current gamemod and the new is not empty, OR the new is empty but it's not BASEGAME and it's different (we're careful because it will restart the game engine and so probably every client will get disconnected)

			// Memorize the current mod (only if we are indeed switching mod, otherwise we will save basegame instead of empty strings and force a mod switching when stopping the demo when we haven't changed mod in the first place!)
			if (strlen(Cvar_VariableString("fs_game"))) { // if fs_game is not "", we save it
				Q_strncpyz(savedFsGame, (const char*)Cvar_VariableString("fs_game"), MAX_QPATH);
			} else { // else, it's equal to "", and this means that we were playing in the basegame mod, but we need to have a non-empty string, else we can't use game_restart!
				Q_strncpyz(savedFsGame, BASEGAME, MAX_QPATH);
			}

			// Switch the mod
			Com_Printf("DEMO: Trying to switch automatically to the mod %s to replay the demo\n", strlen(fs) ? fs : BASEGAME);
			Cvar_SetValue("sv_democlients", 0); // set sv_democlients to 0 (because game_restart will reset sv_maxclients, so we have a risk to have a greater sv_democlients than sv_maxclients, and we don't want that)
			Cbuf_AddText(va("game_restart %s\n", fs)); // switch mod!
		}

		Cbuf_AddText(va("g_gametype %i\ndevmap %s\n", gametype, map)); // Change gametype and map (using devmap to enable cheats)

		return; // Quit and wait for the next SV_Frame() iteration (when the server/map will have restarted) to retry playing the demo

	} else if ( time < sv.time && keepSaved ) { // else if the demo time is still below the server time but we already restarted for the demo playback, we just iterate a few demo frames in the void to catch to until we are above the server time. Note: having a server time below the demo time is CRITICAL, else we may send to the clients a server time that is below the previous, making the time going backward, which should NEVER happen!
		int timetoreach = sv.time;
		sv.time = time;
		while (sv.time < timetoreach) {
			SV_DemoReadFrame(); // run a few frames to settle things out
		}
	}



	// Initialize our stuff
	Com_Memset(sv.demoEntities, 0, sizeof(sv.demoEntities));
	Com_Memset(sv.demoPlayerStates, 0, sizeof(sv.demoPlayerStates));
	Cvar_SetValue("sv_democlients", clients); // Note: we need SV_Startup() to NOT use SV_ChangeMaxClients for this to work without crashing when changing fs_game
	Cvar_SetValue("bot_minplayers", 0); // if we have bots that autoconnect, this will make up for a very weird demo!

	// Force all real clients already connected before the demo begins to be set to spectator team
	for (i = sv_democlients->integer; i < sv_maxclients->integer; i++) {
		if (svs.clients[i].state >= CS_CONNECTED) { // Only set as spectator a player that is at least connected (or primed or active)
			SV_ExecuteClientCommand(&svs.clients[i], "team spectator", qtrue); // should be more interoperable than a forceteam
			Cbuf_ExecuteText(EXEC_NOW, va("forceteam %i spectator", i)); // sometimes team spectator does not work when a demo is replayed client-side with some mods (eg: ExcessivePlus), in this case we also issue a forceteam (even if it's less interoperable)
		}
	}

	// Free memory
	free( map );
	free( fs );
	free( hostname );
	free( datetime );
	//free( metadata ); // it seems glibc already frees this pointer automatically since the malloc was removed, if we specify this line we'll get a crash

	// Start reading the first frame
	Com_Printf("Playing demo %s.\n", sv.demoName); // log that the demo is started here
	SV_SendServerCommand( NULL, "chat \"^3Demo replay started!\"" ); // send a message to player
	SV_SendServerCommand( NULL, "cp \"^3Demo replay started!\"" ); // send a centerprint message to player
	sv.demoState = DS_PLAYBACK; // set state to playback
	Cvar_SetValue("sv_demoState", DS_PLAYBACK);
	keepSaved = qfalse; // Don't save values anymore: the next time we stop playback, we will restore previous values (because now we are really launching the playback, so anything that might happen now is either a big bug or the end of demo, in any case we want to restore the values)
	SV_DemoReadFrame(); // reading the first frame, which should contain some initialization events (eg: initial confistrings/userinfo when demo recording started, initial entities states and placement, etc..)
}

/*
====================
SV_DemoStopPlayback

Close the demo file and restart the map (can be used both when recording or when playing or at shutdown of the game)
====================
*/
void SV_DemoStopPlayback(void)
{
	int olddemostate;
	olddemostate = sv.demoState;

	// Clear client configstrings
	int i;
	if (olddemostate == DS_PLAYBACK) { // unload democlients only if we were replaying a demo (if not it will produce an error!)
		for (i = 0; i < sv_democlients->integer; i++)
			SV_SetConfigstring(CS_PLAYERS + i, NULL); //qtrue
	}

	// Close demo file after playback
	FS_FCloseFile(sv.demoFile);
	sv.demoState = DS_NONE;
	Cvar_SetValue("sv_demoState", DS_NONE);
	Com_Printf("DEMO: End of demo. Stopped playing demo %s.\n", sv.demoName);

	// Restore initial cvars of the server that were modified by the demo playback
	// Note: must do it before the map_restart! so that latched values such as sv_maxclients takes effect
	if ( !keepSaved ) {

		Cvar_SetValue("sv_democlients", 0);

		if (savedMaxClients >= 0) {
			Cvar_SetLatched("sv_maxclients", va("%i", savedMaxClients) );
			savedMaxClients = -1;
		}

		if (savedBotMinPlayers >= 0) {
			Cvar_SetValue("bot_minplayers", savedBotMinPlayers);
			savedBotMinPlayers = -1;
		}

		if (savedFPS > 0) {
			Cvar_SetValue("sv_fps", savedFPS);
			savedFPS = -1;
		}

		if (savedDoWarmup >= 0) {
			Cvar_SetValue("g_doWarmup", savedDoWarmup);
			savedDoWarmup = -1;
		}

		if (savedAllowVote >= 0) {
			Cvar_SetValue("g_allowVote", savedAllowVote);
			savedAllowVote = -1;
		}

		if (savedTeamAutoJoin >= 0) {
			Cvar_SetValue("g_teamAutoJoin", savedTeamAutoJoin);
			savedTeamAutoJoin = -1;
		}
		demoTeamAutoJoin = -1;

		if (savedTimelimit >= 0) {
			Cvar_SetValue("timelimit", savedTimelimit);
			savedTimelimit = -1;
		}

		if (savedFraglimit >= 0) {
			Cvar_SetValue("fraglimit", savedFraglimit);
			savedFraglimit = -1;
		}

		if (savedCapturelimit >= 0) {
			Cvar_SetValue("capturelimit", savedCapturelimit);
			savedCapturelimit = -1;
		}

		if (strlen(savedFsGame)) { // After setting all the other vars, we switch back the mod if necessary (it must be done only AFTER all the other cvars are set, else cvars set after won't take effect!)
			Cbuf_AddText(va("game_restart %s\n", savedFsGame));
			Q_strncpyz(savedFsGame, "", MAX_QPATH);
		}

		if (savedGametype >= 0) {
			Cvar_SetLatched("g_gametype", va("%i", savedGametype) );
			Cbuf_AddText(va("g_gametype %i\n", savedGametype)); // force gametype switching (NOTE: must be AFTER game_restart to work!)
			savedGametype = -1;
		}
	}

	// LAST RELOAD (to reinit all vars)
	// demo hasn't actually started yet
	if (olddemostate == DS_NONE && !keepSaved ) { // If keepSaved is true, then this restart procedure is totally normal (and that's why we keep values saved), else it is not normal and probably an error happened
#ifdef DEDICATED
		Com_Printf("DEMOERROR: An error happened while playing/recording the demo, please check the log for more info\n"); // if server, don't crash it if an error happens, just print a message
#else
		Com_Error (ERR_DROP,"An error happened while replaying the demo, please check the log for more info\n");
		Cvar_SetValue("sv_killserver", 1);
#endif
	} else if (olddemostate == DS_PLAYBACK) {
		// set a special state to say that we are waiting to stop (SV_DemoChangeMaxClients() will set to DS_NONE after moving the real clients to their correct slots)
		sv.demoState = DS_WAITINGSTOP;
		Cvar_SetValue("sv_demoState", DS_WAITINGSTOP);
#ifdef DEDICATED
		Cbuf_AddText(va("map %s\n", Cvar_VariableString( "mapname" ))); // better to do a map command rather than map_restart if we do a mod switching with game_restart, map_restart will point to no map (because the config is completely unloaded)
#else
		// Update sv_maxclients latched value (since we will kill the server because it's not a dedicated server, we won't restart the map, so latched values won't be affected unless we force the refresh)
		Cvar_Get( "sv_maxclients", "8", 0 ); // Get sv_maxclients value (force latched values to commit)
		sv_maxclients->modified = qfalse; // Set modified to false
		// Kill the local server
		Cvar_SetValue("sv_killserver", 1); // instead of sending a Cbuf_AddText("killserver") command, we here just set a special cvar which will kill the server at the next SV_Frame() iteration (smoother than force killing)
#endif
	}

	return;

}