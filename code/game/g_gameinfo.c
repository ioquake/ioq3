//
// g_gameinfo.c
//

// TODO: Add some fancy header + GPL

#include "g_local.h"

//
// arena info
//
static int g_numArenas;
static char *g_arenaInfos[MAX_ARENAS];

/*
===============
G_ParseInfos
===============
*/
static int G_ParseInfos(const char *buf, int max, char *infos[]) {
	const char *token;
	int count;
	char key[MAX_TOKEN_CHARS];
	char info[MAX_INFO_STRING];
	int infoStringSize;

	count = 0;

	while (qtrue) {
		token = COM_Parse(&buf);
		if (!token[0]) {
			break;
		} else if (strcmp(token, "{") != 0) {
			G_Printf("Missing { in info file\n");
			break;
		} else if (count >= max) {
			G_Printf("Max infos exceeded\n");
			break;
		}

		info[0] = '\0';
		while (qtrue) {
			token = COM_ParseExt(&buf, qtrue);
			if (!token[0]) {
				G_Printf("Unexpected end of info file\n");
				break;
			} else if (!strcmp(token, "}")) {
				break;
			}
			Q_strncpyz(key, token, sizeof(key));

			token = COM_ParseExt(&buf, qfalse);
			if (!token[0]) {
				Info_SetValueForKey(info, key, "<NULL>");
			} else {
				Info_SetValueForKey(info, key, token);
			}
		}

		// extra space for arena number
		infoStringSize = strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1;
		infos[count] = G_Alloc(infoStringSize);
		if (infos[count]) {
			Q_strncpyz(infos[count], info, infoStringSize);
			count++;
		}
	}

	return count;
}

/*
===============
G_LoadArenaFromFile
===============
*/
static void G_LoadArenaFromFile(const char *filename) {
	int len;
	fileHandle_t f;
	char buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (!f) {
		G_Printf(S_COLOR_RED "file not found: %s\n", filename);
		return;
	} else if (len >= sizeof(buf)) {
		G_Printf(S_COLOR_RED "file too large: %s is %d, max allowed is %ld", filename, len, (sizeof(buf) - 1));
		trap_FS_FCloseFile(f);
		return;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = '\0';
	trap_FS_FCloseFile(f);

	g_numArenas += G_ParseInfos(buf, (ARRAY_LEN(g_arenaInfos) - g_numArenas), &g_arenaInfos[g_numArenas]);
}

/*
===============
G_LoadArenas
===============
*/
static void G_LoadArenas(void) {
	int numdirs;
	char filename[128];
	char dirlist[2048];
	char *dirptr;
	int i;
	int dirlen;

	g_numArenas = 0;

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, sizeof(dirlist));
	dirptr = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += (dirlen + 1)) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		Q_strcat(filename, sizeof(filename), dirptr);
		G_LoadArenaFromFile(filename);
	}
	G_Printf("%d arenas parsed.\n", g_numArenas);

	// set initial numbers
	for (i = 0; i < g_numArenas; i++) {
		Info_SetValueForKey(g_arenaInfos[i], "num", va("%d", i));
	}
}

/*
===============
G_GetArenaInfoByMap
===============
*/
const char *G_GetArenaInfoByMap(const char *map) {
	int n;

	for (n = 0; n < g_numArenas; n++) {
		if (Q_stricmp(Info_ValueForKey(g_arenaInfos[n], "map"), map) == 0) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}

/*
===============
G_GetArenaInfoByNumber
===============
*/
const char *G_GetArenaInfoByNumber(int num) {
	int n;
	const char *value;

	if ((num < 0) || (num >= g_numArenas)) {
		G_Printf(S_COLOR_RED "Invalid arena number: %d\n", num);
		return NULL;
	}

	for (n = 0; n < g_numArenas; n++) {
		value = Info_ValueForKey(g_arenaInfos[n], "num");
		if (*value && (atoi(value) == num)) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}

/*
===============
G_InitGameinfo
===============
*/
void G_InitGameinfo(void) {
	G_LoadArenas();
}

//
// FIXME: Move code below into both-game? Used by UI as well
//

typedef struct {
	const char *name;
	int bit;
} g_gametypeBitmap_s;

static const g_gametypeBitmap_s gametypeMap[GT_MAX_GAME_TYPE] = {
	{"ffa", GT_FFA}, {"tourney", GT_TOURNAMENT}, {"single", GT_SINGLE_PLAYER}, {"team", GT_TEAM}, {"ft", GT_FREEZETAG},
	{"ctl", GT_CTF}, {"syc_ffa", GT_SPRAYFFA},	 {"syc_tp", GT_SPRAY},		   {"lps", GT_LPS},	  {"bb", GT_BALLOON}};

/*
=================
GametypeBits
=================
*/
static int GametypeBits(const char *string) {
	int bits;
	const char *p;
	const char *token;
	int i;

	bits = 0;
	p = (const char *)string;
	while (qtrue) {
		token = COM_ParseExt(&p, qfalse);
		if (!token[0]) {
			break;
		}

		for (i = 0; i < ARRAY_LEN(gametypeMap); i++) {
			if (Q_stricmp(token, gametypeMap[i].name) == 0) {
				bits |= (1 << gametypeMap[i].bit);
				continue;
			}
		}
	}

	return bits;
}

qboolean MapSupportsGametype(const char *mapname, gametype_t gametype) {
	int queryBit, resultBits;
	const char *info;

	queryBit = (1 << gametype);

	info = G_GetArenaInfoByMap(mapname);
	if (!info) {
		// no info for map, e.g. when no .arena entry is present
		return qtrue;
	}

	resultBits = GametypeBits(Info_ValueForKey(info, "type"));

	if (!(resultBits & queryBit)) {
		return qfalse;
	}

	return qtrue;
}
