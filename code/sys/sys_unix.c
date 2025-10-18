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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <libgen.h>
#include <fcntl.h>
#include <fenv.h>
#include <sys/wait.h>
#include <time.h>

qboolean stdinIsATTY;

static char execBuffer[ 1024 ];
static char *execBufferPointer;
static char *execArgv[ 16 ];
static int execArgc;

/*
==============
Sys_ClearExecBuffer
==============
*/
static void Sys_ClearExecBuffer( void )
{
	execBufferPointer = execBuffer;
	Com_Memset( execArgv, 0, sizeof( execArgv ) );
	execArgc = 0;
}

/*
==============
Sys_AppendToExecBuffer
==============
*/
static void Sys_AppendToExecBuffer( const char *text )
{
	size_t size = sizeof( execBuffer ) - ( execBufferPointer - execBuffer );
	int length = strlen( text ) + 1;

	if( length > size || execArgc >= ARRAY_LEN( execArgv ) )
		return;

	Q_strncpyz( execBufferPointer, text, size );
	execArgv[ execArgc++ ] = execBufferPointer;

	execBufferPointer += length;
}

/*
==============
Sys_Exec
==============
*/
static int Sys_Exec( void )
{
	pid_t pid = fork( );

	if( pid < 0 )
		return -1;

	if( pid )
	{
		// Parent
		int exitCode;

		wait( &exitCode );

		return WEXITSTATUS( exitCode );
	}
	else
	{
		// Child
		execvp( execArgv[ 0 ], execArgv );

		// Failed to execute
		exit( -1 );

		return -1;
	}
}

#ifdef __APPLE__

/*
==================
Sys_DefaultHomePath
==================
*/
static char *Sys_DefaultHomePath(void)
{
	static char homePath[ MAX_OSPATH ] = { 0 };
	char *p;

	if( !*homePath )
	{
		if( ( p = getenv( "HOME" ) ) != NULL )
		{
			Com_sprintf( homePath, sizeof(homePath), "%s%c%s",
				p, PATH_SEP, "Library/Application Support/" );

			if( com_homepath && com_homepath->string[0] )
				Q_strcat(homePath, sizeof(homePath), com_homepath->string);
			else
				Q_strcat(homePath, sizeof(homePath), HOMEPATH_NAME);
		}
	}

	return homePath;
}

char *Sys_DefaultHomeConfigPath(void) { return Sys_DefaultHomePath(); }
char *Sys_DefaultHomeDataPath(void)   { return Sys_DefaultHomePath(); }
char *Sys_DefaultHomeStatePath(void)  { return Sys_DefaultHomePath(); }

#else // __APPLE__

/*
==================
Sys_HomeConfigPath
==================
*/
char *Sys_HomeConfigPath(void)
{
	static char homeConfigPath[ MAX_OSPATH ] = { 0 };
	char *p;

	if( !*homeConfigPath )
	{
		if( ( p = getenv( "XDG_CONFIG_HOME" ) ) != NULL && *p != '\0' )
			Com_sprintf(homeConfigPath, sizeof(homeConfigPath), "%s%c", p, PATH_SEP);
		else if( ( p = getenv( "HOME" ) ) != NULL && *p != '\0' )
			Com_sprintf(homeConfigPath, sizeof(homeConfigPath), "%s%c.config%c", p, PATH_SEP, PATH_SEP);

		if( *homeConfigPath )
		{
			if( com_homepath && com_homepath->string[0] )
				Q_strcat(homeConfigPath, sizeof(homeConfigPath), com_homepath->string);
			else
				Q_strcat(homeConfigPath, sizeof(homeConfigPath), HOMEPATH_NAME);
		}
	}

	return homeConfigPath;
}

/*
==================
Sys_HomeDataPath
==================
*/
char *Sys_HomeDataPath(void)
{
	static char homeDataPath[ MAX_OSPATH ] = { 0 };
	char *p;

	if( !*homeDataPath )
	{
		if( ( p = getenv( "XDG_DATA_HOME" ) ) != NULL && *p != '\0' )
			Com_sprintf(homeDataPath, sizeof(homeDataPath), "%s%c", p, PATH_SEP);
		else if( ( p = getenv( "HOME" ) ) != NULL && *p != '\0' )
			Com_sprintf(homeDataPath, sizeof(homeDataPath), "%s%c.local%cshare%c", p, PATH_SEP, PATH_SEP, PATH_SEP);

		if( *homeDataPath )
		{
			if( com_homepath && com_homepath->string[0] )
				Q_strcat(homeDataPath, sizeof(homeDataPath), com_homepath->string);
			else
				Q_strcat(homeDataPath, sizeof(homeDataPath), HOMEPATH_NAME);
		}
	}

	return homeDataPath;
}

/*
==================
Sys_HomeStatePath
==================
*/
char *Sys_HomeStatePath(void)
{
	static char homeStatePath[ MAX_OSPATH ] = { 0 };
	char *p;

	if( !*homeStatePath )
	{
		if( ( p = getenv( "XDG_STATE_HOME" ) ) != NULL && *p != '\0' )
			Com_sprintf(homeStatePath, sizeof(homeStatePath), "%s%c", p, PATH_SEP);
		else if( ( p = getenv( "HOME" ) ) != NULL && *p != '\0' )
			Com_sprintf(homeStatePath, sizeof(homeStatePath), "%s%c.local%cstate%c", p, PATH_SEP, PATH_SEP, PATH_SEP);

		if( *homeStatePath )
		{
			if( com_homepath && com_homepath->string[0] )
				Q_strcat(homeStatePath, sizeof(homeStatePath), com_homepath->string);
			else
				Q_strcat(homeStatePath, sizeof(homeStatePath), HOMEPATH_NAME);
		}
	}

	return homeStatePath;
}

/*
==================
Sys_LegacyHomePath
==================
*/
static char *Sys_LegacyHomePath(void)
{
	static char homePath[ MAX_OSPATH ] = { 0 };
	char *p;

	if( ( p = getenv( "FLATPAK_ID" ) ) != NULL && *p != '\0' )
	{
		// Flatpaks always use XDG
		return "";
	}

	if( !*homePath )
	{
		if( ( p = getenv( "HOME" ) ) != NULL && *p != '\0' )
		{
			Com_sprintf(homePath, sizeof(homePath), "%s%c%s",
				p, PATH_SEP, HOMEPATH_NAME_UNIX_LEGACY);
		}
	}

	return homePath;
}

/*
==================
Sys_MigrateToXDG
==================
*/
qboolean Sys_MigrateToXDG(void)
{
	const char *scriptTemplate =
		"#!/bin/sh\n"

		"set -eu\n"

		"legacy_home=\"%s\"\n"
		"xdg_config_home=\"%s\"\n"
		"xdg_data_home=\"%s\"\n"
		"xdg_state_home=\"%s\"\n"

		"xdg_config_pattern=\"*.cfg\"\n"
		"xdg_data_pattern=\"demos/*.dm_* *.log *.pk3 *.txt \\\n"
		"    screenshots/*.jpg screenshots/*.tga videos/*.avi\"\n"
		"xdg_state_pattern=\"*.dat q3history q3key\"\n"

		"glob_copy() {\n"
		"    game_dir=${1:+$1/}\n"
		"    dst=\"$2\"\n"
		"    shift 2\n"
		"    for pattern in \"$@\"; do\n"
		"        subdir=$(dirname \"$pattern\")\n"
		"        [ \"$subdir\" = \".\" ] && subdir=\"\"\n"
		"        find \"$legacy_home/$game_dir\" \\\n"
		"            -path \"$legacy_home/$game_dir$pattern\" -type f \\\n"
		"            -exec mkdir -p \"$dst/$game_dir$subdir\" \\; \\\n"
		"            -exec cp -av {} \"$dst/$game_dir$subdir\" \\;\n"
		"    done\n"
		"}\n"

		"unmatched_copy() {\n"
		"    game_dir=${1:+$1/}\n"
		"    shift\n"
		"    find_args=\"\"\n"
		"    for pattern in \"$@\"; do\n"
		"        find_args=\"$find_args \\\n"
		"            -not -path \\\"$legacy_home/$game_dir$pattern\\\"\"\n"
		"    done\n"
		"    eval \"find '$legacy_home/$game_dir' -type f $find_args\" | \\\n"
		"    while IFS= read -r file; do\n"
		"        dst=\"$xdg_data_home${file#$legacy_home}\"\n"
		"        dst_dir=$(dirname \"$dst\")\n"
		"        mkdir -p \"$dst_dir\"\n"
		"        cp -av \"$file\" \"$dst\"\n"
		"    done\n"
		"}\n"

		"echo \"Starting XDG migration...\"\n"

		"glob_copy \"\" \"$xdg_state_home\" \"qkey\"\n"

		"for game_dir in \"$legacy_home\"/*; do\n"
		"    [ -d \"$game_dir\" ] || continue\n"
		"    game=$(basename \"$game_dir\")\n"
		"    glob_copy \"$game\" \"$xdg_config_home\" $xdg_config_pattern\n"
		"    glob_copy \"$game\" \"$xdg_data_home\" $xdg_data_pattern\n"
		"    glob_copy \"$game\" \"$xdg_state_home\" $xdg_state_pattern\n"
		"    unmatched_copy \"$game\" \\\n"
		"        $xdg_config_pattern \\\n"
		"        $xdg_data_pattern \\\n"
		"        $xdg_state_pattern\n"
		"done\n"

		"echo \"XDG migration complete!\"\n";

	char scriptBuffer[2048];
	int len = Com_sprintf( scriptBuffer, sizeof( scriptBuffer ), scriptTemplate,
		Sys_LegacyHomePath( ), Sys_HomeConfigPath( ),
		Sys_HomeDataPath( ), Sys_HomeStatePath( ) );

	if( len < 0 || len >= (int)sizeof( scriptBuffer ) )
	{
		Com_Printf( "XDG migration error: substitution failed.\n" );
		return qfalse;
	}

	char scriptPath[] = "/tmp/xdgmigrationXXXXXX";
	int fd = mkstemp( scriptPath );
	if( fd == -1 )
	{
		Com_Printf( "XDG migration error: script creation failed.\n" );
		return qfalse;
	}

	if( write( fd, scriptBuffer, len ) != len )
	{
		close( fd );
		unlink( scriptPath );
		Com_Printf( "XDG migration error: script write failed.\n" );
		return qfalse;
	}
	close( fd );

	if( chmod( scriptPath, 0700 ) == -1 )
	{
		unlink( scriptPath );
		Com_Printf( "XDG migration error: script chmod failed.\n" );
		return qfalse;
	}

	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer( scriptPath );
	int result = Sys_Exec( );
	unlink( scriptPath );

	return result == 0;
}

/*
==================
Sys_ShouldUseLegacyHomePath
==================
*/
static qboolean Sys_ShouldUseLegacyHomePath(void)
{
	if( access( Sys_HomeConfigPath( ), F_OK ) == 0 )
	{
		// If the XDG config directory exists, prefer XDG layout, regardless
		return qfalse;
	}

	if( ( com_homepath && com_homepath->string[0] ) ||
		Cvar_VariableString( "fs_homepath" )[0] )
	{
		// If a custom homepath has been explicity set then
		// that strongly implies that migration isn't desired
		return qfalse;
	}

	const char *legacyHomePath = Sys_LegacyHomePath();

	if( !*legacyHomePath || access( legacyHomePath, F_OK ) != 0 )
	{
		// The legacy home path doesn't exist
		return qfalse;
	}

	char migrationRefusedPath[ MAX_OSPATH ];
	Com_sprintf( migrationRefusedPath, sizeof( migrationRefusedPath ),
		"%s/.xdgMigrationRefused", legacyHomePath );

	// If the user hasn't already refused, ask if they want to migrate
	if( access( migrationRefusedPath, F_OK ) != 0 )
	{
		dialogResult_t result = Sys_Dialog( DT_YES_NO, va(
			"Modern games and applications store files in "
			"directories according to the Free Desktop standard. "
			"Here's what that would look like for %s:\n\n"
			"Configuration files:\n  %s\n\n"
			"Data files; pk3s, screenshots, logs, demos, etc.:\n  %s\n\n"
			"Internal runtime files:\n  %s\n\n"
			"At the moment all of these files are found here:\n  %s\n\n"
			"Do you want to copy your files to these new directories?",
			PRODUCT_NAME,
			Sys_HomeConfigPath( ), Sys_HomeDataPath( ), Sys_HomeStatePath( ),
			legacyHomePath ),
			"Home Directory Files Upgrade" );

		if( result == DR_YES )
			return !Sys_MigrateToXDG( );

		// Guard against asking again in future
		fclose( fopen( migrationRefusedPath, "w" ) );
	}

	return qtrue;
}

/*
==================
Sys_DefaultHomeConfigPath
==================
*/
char *Sys_DefaultHomeConfigPath(void)
{
	if( Sys_ShouldUseLegacyHomePath( ) )
		return Sys_LegacyHomePath( );

	return Sys_HomeConfigPath( );
}

/*
==================
Sys_DefaultHomeDataPath
==================
*/
char *Sys_DefaultHomeDataPath(void)
{
	if( Sys_ShouldUseLegacyHomePath( ) )
		return Sys_LegacyHomePath( );

	return Sys_HomeDataPath( );
}

/*
==================
Sys_DefaultHomeStatePath
==================
*/
char *Sys_DefaultHomeStatePath(void)
{
	if( Sys_ShouldUseLegacyHomePath( ) )
		return Sys_LegacyHomePath( );

	return Sys_HomeStatePath( );
}

#endif

/*
================
Sys_SteamPath
================
*/
char *Sys_SteamPath( void )
{
	// Steam doesn't let you install Quake 3 on Mac/Linux
	return "";
}

/*
================
Sys_GogPath
================
*/
char *Sys_GogPath( void )
{
	// GOG doesn't let you install Quake 3 on Mac/Linux
	return "";
}

/*
================
Sys_MicrosoftStorePath
================
*/
char* Sys_MicrosoftStorePath(void)
{
	// Microsoft Store doesn't exist on Mac/Linux
	return "";
}


/*
================
Sys_Milliseconds
================
*/
/* base time in seconds, that's our origin
   timeval:tv_sec is an int:
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038 */
unsigned long sys_timeBase = 0;
/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
     0x7fffffff ms - ~24 days
   although timeval:tv_usec is an int, I'm not sure wether it is actually used as an unsigned int
     (which would affect the wrap period) */
int curtime;
int Sys_Milliseconds (void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);

	if (!sys_timeBase)
	{
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	curtime = (tp.tv_sec - sys_timeBase)*1000 + tp.tv_usec/1000;

	return curtime;
}

/*
==================
Sys_RandomBytes
==================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	FILE *fp;

	fp = fopen( "/dev/urandom", "r" );
	if( !fp )
		return qfalse;

	setvbuf( fp, NULL, _IONBF, 0 ); // don't buffer reads from /dev/urandom

	if( fread( string, sizeof( byte ), len, fp ) != len )
	{
		fclose( fp );
		return qfalse;
	}

	fclose( fp );
	return qtrue;
}

/*
==================
Sys_GetCurrentUser
==================
*/
char *Sys_GetCurrentUser( void )
{
	struct passwd *p;

	if ( (p = getpwuid( getuid() )) == NULL ) {
		return "player";
	}
	return p->pw_name;
}

#define MEM_THRESHOLD 96*1024*1024

/*
==================
Sys_LowPhysicalMemory

TODO
==================
*/
qboolean Sys_LowPhysicalMemory( void )
{
	return qfalse;
}

/*
==================
Sys_Basename
==================
*/
const char *Sys_Basename( char *path )
{
	return basename( path );
}

/*
==================
Sys_Dirname
==================
*/
const char *Sys_Dirname( char *path )
{
	return dirname( path );
}

/*
==============
Sys_FOpen
==============
*/
FILE *Sys_FOpen( const char *ospath, const char *mode ) {
	struct stat buf;

	// check if path exists and is a directory
	if ( !stat( ospath, &buf ) && S_ISDIR( buf.st_mode ) )
		return NULL;

	return fopen( ospath, mode );
}

/*
==================
Sys_Mkdir
==================
*/
qboolean Sys_Mkdir( const char *path )
{
	int result = mkdir( path, 0750 );

	if( result != 0 )
		return errno == EEXIST;

	return qtrue;
}

/*
==================
Sys_Mkfifo
==================
*/
FILE *Sys_Mkfifo( const char *ospath )
{
	FILE	*fifo;
	int	result;
	int	fn;
	struct	stat buf;

	// if file already exists AND is a pipefile, remove it
	if( !stat( ospath, &buf ) && S_ISFIFO( buf.st_mode ) )
		FS_Remove( ospath );

	result = mkfifo( ospath, 0600 );
	if( result != 0 )
		return NULL;

	fifo = fopen( ospath, "w+" );
	if( fifo )
	{
		fn = fileno( fifo );
		fcntl( fn, F_SETFL, O_NONBLOCK );
	}

	return fifo;
}

/*
==================
Sys_Cwd
==================
*/
char *Sys_Cwd( void )
{
	static char cwd[MAX_OSPATH];

	char *result = getcwd( cwd, sizeof( cwd ) - 1 );
	if( result != cwd )
		return NULL;

	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/*
==================
Sys_BinaryPathRelative
==================
*/
char *Sys_BinaryPathRelative(const char *relative)
{
	static char resolved[MAX_OSPATH];
	char combined[MAX_OSPATH];

	snprintf(combined, sizeof(combined), "%s/%s", Sys_BinaryPath(), relative);

	if (!realpath(combined, resolved))
		return NULL;

	return resolved;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define MAX_FOUND_FILES 0x1000

/*
==================
Sys_ListFilteredFiles
==================
*/
void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles )
{
	char          search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char          filename[MAX_OSPATH];
	DIR           *fdir;
	struct dirent *d;
	struct stat   st;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if ( basedir[0] == '\0' ) {
		return;
	}

	if (strlen(subdirs)) {
		Com_sprintf( search, sizeof(search), "%s/%s", basedir, subdirs );
	}
	else {
		Com_sprintf( search, sizeof(search), "%s", basedir );
	}

	if ((fdir = opendir(search)) == NULL) {
		return;
	}

	while ((d = readdir(fdir)) != NULL) {
		Com_sprintf(filename, sizeof(filename), "%s/%s", search, d->d_name);
		if (stat(filename, &st) == -1)
			continue;

		if (st.st_mode & S_IFDIR) {
			if (Q_stricmp(d->d_name, ".") && Q_stricmp(d->d_name, "..")) {
				if (strlen(subdirs)) {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s/%s", subdirs, d->d_name);
				}
				else {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s", d->d_name);
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof(filename), "%s/%s", subdirs, d->d_name );
		if (!Com_FilterPath( filter, filename, qfalse ))
			continue;
		list[ *numfiles ] = CopyString( filename );
		(*numfiles)++;
	}

	closedir(fdir);
}

/*
==================
Sys_ListFiles
==================
*/
char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs )
{
	struct dirent *d;
	DIR           *fdir;
	qboolean      dironly = wantsubs;
	char          search[MAX_OSPATH];
	int           nfiles;
	char          **listCopy;
	char          *list[MAX_FOUND_FILES];
	int           i;
	struct stat   st;

	int           extLen;

	if (filter) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = NULL;
		*numfiles = nfiles;

		if (!nfiles)
			return NULL;

		listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
		for ( i = 0 ; i < nfiles ; i++ ) {
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if ( directory[0] == '\0' ) {
		*numfiles = 0;
		return NULL;
	}

	if ( !extension)
		extension = "";

	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		dironly = qtrue;
	}

	extLen = strlen( extension );

	// search
	nfiles = 0;

	if ((fdir = opendir(directory)) == NULL) {
		*numfiles = 0;
		return NULL;
	}

	while ((d = readdir(fdir)) != NULL) {
		Com_sprintf(search, sizeof(search), "%s/%s", directory, d->d_name);
		if (stat(search, &st) == -1)
			continue;
		if ((dironly && !(st.st_mode & S_IFDIR)) ||
			(!dironly && (st.st_mode & S_IFDIR)))
			continue;

		if (*extension) {
			if ( strlen( d->d_name ) < extLen ||
				Q_stricmp(
					d->d_name + strlen( d->d_name ) - extLen,
					extension ) ) {
				continue; // didn't match
			}
		}

		if ( nfiles == MAX_FOUND_FILES - 1 )
			break;
		list[ nfiles ] = CopyString( d->d_name );
		nfiles++;
	}

	list[ nfiles ] = NULL;

	closedir(fdir);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

/*
==================
Sys_FreeFileList
==================
*/
void Sys_FreeFileList( char **list )
{
	int i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		Z_Free( list[i] );
	}

	Z_Free( list );
}

/*
==================
Sys_Sleep

Block execution for msec or until input is received.
==================
*/
void Sys_Sleep( int msec )
{
	if( msec == 0 )
		return;

	if( stdinIsATTY )
	{
		fd_set fdset;

		FD_ZERO(&fdset);
		FD_SET(STDIN_FILENO, &fdset);
		if( msec < 0 )
		{
			select(STDIN_FILENO + 1, &fdset, NULL, NULL, NULL);
		}
		else
		{
			struct timeval timeout;

			timeout.tv_sec = msec/1000;
			timeout.tv_usec = (msec%1000)*1000;
			select(STDIN_FILENO + 1, &fdset, NULL, NULL, &timeout);
		}
	}
	else
	{
		struct timespec req;

		// With nothing to select() on, we can't wait indefinitely
		if( msec < 0 )
			msec = 10;

		req.tv_sec = msec/1000;
		req.tv_nsec = (msec%1000)*1000000;
		nanosleep(&req, NULL);
	}
}

/*
==============
Sys_ErrorDialog

Display an error message
==============
*/
void Sys_ErrorDialog( const char *error )
{
	char buffer[ 1024 ];
	unsigned int size;
	int f = -1;
	const char *homedatapath = Cvar_VariableString( "fs_homedatapath" );
	const char *gamedir = Cvar_VariableString( "fs_game" );
	const char *fileName = "crashlog.txt";
	char *ospath = FS_BuildOSPath( homedatapath, gamedir, fileName );

	Sys_Print( va( "%s\n", error ) );

#ifndef DEDICATED
	Sys_Dialog( DT_ERROR, va( "%s. See \"%s\" for details.", error, ospath ), "Error" );
#endif

	// Make sure the write path for the crashlog exists...
	if( FS_CreatePath( homedatapath ) )
	{
		Com_Printf("ERROR: couldn't create path '%s' for crash log.\n", ospath);
		return;
	}

	// We might be crashing because we maxed out the Quake MAX_FILE_HANDLES,
	// which will come through here, so we don't want to recurse forever by
	// calling FS_FOpenFileWrite()...use the Unix system APIs instead.
	f = open( ospath, O_CREAT | O_TRUNC | O_WRONLY, 0640 );
	if( f == -1 )
	{
		Com_Printf( "ERROR: couldn't open %s\n", fileName );
		return;
	}

	// We're crashing, so we don't care much if write() or close() fails.
	while( ( size = CON_LogRead( buffer, sizeof( buffer ) ) ) > 0 ) {
		if( write( f, buffer, size ) != size ) {
			Com_Printf( "ERROR: couldn't fully write to %s\n", fileName );
			break;
		}
	}

	close( f );
}

#ifndef __APPLE__
/*
==============
Sys_ZenityCommand
==============
*/
static void Sys_ZenityCommand( dialogType_t type, const char *message, const char *title )
{
	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer( "zenity" );

	switch( type )
	{
		default:
		case DT_INFO:      Sys_AppendToExecBuffer( "--info" ); break;
		case DT_WARNING:   Sys_AppendToExecBuffer( "--warning" ); break;
		case DT_ERROR:     Sys_AppendToExecBuffer( "--error" ); break;
		case DT_YES_NO:
			Sys_AppendToExecBuffer( "--question" );
			Sys_AppendToExecBuffer( "--ok-label=Yes" );
			Sys_AppendToExecBuffer( "--cancel-label=No" );
			break;

		case DT_OK_CANCEL:
			Sys_AppendToExecBuffer( "--question" );
			Sys_AppendToExecBuffer( "--ok-label=OK" );
			Sys_AppendToExecBuffer( "--cancel-label=Cancel" );
			break;
	}

	Sys_AppendToExecBuffer( va( "--text=%s", message ) );
	Sys_AppendToExecBuffer( va( "--title=%s", title ) );
}

/*
==============
Sys_KdialogCommand
==============
*/
static void Sys_KdialogCommand( dialogType_t type, const char *message, const char *title )
{
	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer( "kdialog" );

	switch( type )
	{
		default:
		case DT_INFO:      Sys_AppendToExecBuffer( "--msgbox" ); break;
		case DT_WARNING:   Sys_AppendToExecBuffer( "--sorry" ); break;
		case DT_ERROR:     Sys_AppendToExecBuffer( "--error" ); break;
		case DT_YES_NO:    Sys_AppendToExecBuffer( "--warningyesno" ); break;
		case DT_OK_CANCEL: Sys_AppendToExecBuffer( "--warningcontinuecancel" ); break;
	}

	Sys_AppendToExecBuffer( message );
	Sys_AppendToExecBuffer( va( "--title=%s", title ) );
}

/*
==============
Sys_XmessageCommand
==============
*/
static void Sys_XmessageCommand( dialogType_t type, const char *message, const char *title )
{
	Sys_ClearExecBuffer( );
	Sys_AppendToExecBuffer( "xmessage" );
	Sys_AppendToExecBuffer( "-buttons" );

	switch( type )
	{
		default:           Sys_AppendToExecBuffer( "OK:0" ); break;
		case DT_YES_NO:    Sys_AppendToExecBuffer( "Yes:0,No:1" ); break;
		case DT_OK_CANCEL: Sys_AppendToExecBuffer( "OK:0,Cancel:1" ); break;
	}

	Sys_AppendToExecBuffer( "-center" );
	Sys_AppendToExecBuffer( message );
}

/*
==============
Sys_Dialog

Display a *nix dialog box
==============
*/
dialogResult_t Sys_Dialog( dialogType_t type, const char *message, const char *title )
{
	typedef enum
	{
		NONE = 0,
		ZENITY,
		KDIALOG,
		XMESSAGE,
		NUM_DIALOG_PROGRAMS
	} dialogCommandType_t;
	typedef void (*dialogCommandBuilder_t)( dialogType_t, const char *, const char * );

	const char              *session = getenv( "DESKTOP_SESSION" );
	qboolean                tried[ NUM_DIALOG_PROGRAMS ] = { qfalse };
	dialogCommandBuilder_t  commands[ NUM_DIALOG_PROGRAMS ] = { NULL };
	dialogCommandType_t     preferredCommandType = NONE;
	int                     i;

	commands[ ZENITY ] = &Sys_ZenityCommand;
	commands[ KDIALOG ] = &Sys_KdialogCommand;
	commands[ XMESSAGE ] = &Sys_XmessageCommand;

	// This may not be the best way
	if( !Q_stricmp( session, "gnome" ) )
		preferredCommandType = ZENITY;
	else if( !Q_stricmp( session, "kde" ) )
		preferredCommandType = KDIALOG;

	for( i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++ )
	{
		if( preferredCommandType != NONE && preferredCommandType != i )
			continue;

		if( !tried[ i ] )
		{
			int exitCode;

			commands[ i ]( type, message, title );
			exitCode = Sys_Exec( );

			if( exitCode >= 0 )
			{
				switch( type )
				{
					case DT_YES_NO:    return exitCode ? DR_NO : DR_YES;
					case DT_OK_CANCEL: return exitCode ? DR_CANCEL : DR_OK;
					default:           return DR_OK;
				}
			}

			tried[ i ] = qtrue;

			// The preference failed, so start again in order
			if( preferredCommandType != NONE )
			{
				preferredCommandType = NONE;
				i = NONE + 1;
			}
		}
	}

	Com_DPrintf( S_COLOR_YELLOW "WARNING: failed to show a dialog\n" );
	return DR_OK;
}
#endif

/*
==============
Sys_GLimpSafeInit

Unix specific "safe" GL implementation initialisation
==============
*/
void Sys_GLimpSafeInit( void )
{
	// NOP
}

/*
==============
Sys_GLimpInit

Unix specific GL implementation initialisation
==============
*/
void Sys_GLimpInit( void )
{
	// NOP
}

void Sys_SetFloatEnv(void)
{
	// rounding toward nearest
	fesetround(FE_TONEAREST);
}

/*
==============
Sys_PlatformInit

Unix specific initialisation
==============
*/
void Sys_PlatformInit( void )
{
	const char* term = getenv( "TERM" );

	signal( SIGHUP, Sys_SigHandler );
	signal( SIGQUIT, Sys_SigHandler );
	signal( SIGTRAP, Sys_SigHandler );
	signal( SIGABRT, Sys_SigHandler );
	signal( SIGBUS, Sys_SigHandler );

	Sys_SetFloatEnv();

	stdinIsATTY = isatty( STDIN_FILENO ) &&
		!( term && ( !strcmp( term, "raw" ) || !strcmp( term, "dumb" ) ) );
}

/*
==============
Sys_PlatformExit

Unix specific deinitialisation
==============
*/
void Sys_PlatformExit( void )
{
}

/*
==============
Sys_SetEnv

set/unset environment variables (empty value removes it)
==============
*/

void Sys_SetEnv(const char *name, const char *value)
{
	if(value && *value)
		setenv(name, value, 1);
	else
		unsetenv(name);
}

/*
==============
Sys_PID
==============
*/
int Sys_PID( void )
{
	return getpid( );
}

/*
==============
Sys_PIDIsRunning
==============
*/
qboolean Sys_PIDIsRunning( int pid )
{
	return kill( pid, 0 ) == 0;
}

/*
=================
Sys_DllExtension

Check if filename should be allowed to be loaded as a DLL.
=================
*/
qboolean Sys_DllExtension( const char *name ) {
	const char *p;
	char c = 0;

	if ( COM_CompareExtension( name, DLL_EXT ) ) {
		return qtrue;
	}

#ifdef __APPLE__
	// Allow system frameworks without dylib extensions
	// i.e., /System/Library/Frameworks/OpenAL.framework/OpenAL
	if ( strncmp( name, "/System/Library/Frameworks/", 27 ) == 0 ) {
		return qtrue;
	}
#endif

	// Check for format of filename.so.1.2.3
	p = strstr( name, DLL_EXT "." );

	if ( p ) {
		p += strlen( DLL_EXT );

		// Check if .so is only followed for periods and numbers.
		while ( *p ) {
			c = *p;

			if ( !isdigit( c ) && c != '.' ) {
				return qfalse;
			}

			p++;
		}

		// Don't allow filename to end in a period. file.so., file.so.0., etc
		if ( c != '.' ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
==============
Sys_OpenFolderInPlatformFileManager
==============
*/
qboolean Sys_OpenFolderInPlatformFileManager( const char *path )
{
	Sys_ClearExecBuffer( );

#ifdef __APPLE__
	Sys_AppendToExecBuffer( "open" );
#else
	Sys_AppendToExecBuffer( "xdg-open" );
#endif

	Sys_AppendToExecBuffer( path );

	return Sys_Exec( ) == 0;
}
