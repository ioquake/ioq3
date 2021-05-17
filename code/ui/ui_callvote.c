// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

CALL VOTE MENU

=======================================================================
*/

#include "ui_local.h"

#define ART_ARROWUP0 "menu/mods/arrowup0"
#define ART_ARROWUP1 "menu/mods/arrowup1"
#define ART_ARROWDOWN0 "menu/mods/arrowdown0"
#define ART_ARROWDOWN1 "menu/mods/arrowdown1"

#define ID_BACK 10
#define ID_GO 11
#define ID_CALLVOTETYPE 12
#define ID_UP 13
#define ID_DOWN 14
#define ID_VOTEYES 15
#define ID_VOTENO 16
#define ID_LISTITEM0 20

#define NUM_LISTITEMS 8

#define LISTITEM_WIDTH 160

#define MAX_LISTSTRARRAY (8 * 1024)
static char ArrayForListStrs[MAX_LISTSTRARRAY];

static void UI_CallVote_Cache(void);
static void UI_CallVote_CVType(void *unused, int event);

vec4_t colorTOrange = {1.f, 0.43f, 0.f, 0.66f};
vec4_t colorMOver = {0.4f, 0.f, 0.4f, 1.f};

static const char *callvote_strs[] = {"kick", "timelimit", "pointlimit",  "gametype",
									  "map",  "next map",  "restart map", 0};

typedef struct {
	menuframework_s menu;

	menutext_s listitems[NUM_LISTITEMS];
	menulist_s votetype;
	menufield_s value;
	menutext_s go;
	menutext_s back;
	menutext_s voteyes;
	menutext_s voteno;
	menubitmap1024s_s arrowup;
	menubitmap1024s_s arrowdown;

	int numListitems;
	int baseItem;
	int selectedItem;
	const char *selItemStr;
	qboolean voteRunning;
	int lastGoTime;
} callVoteMenu_t;

static callVoteMenu_t s_callVoteMenu;

static void UI_CallVote_Vote(void *ptr, int event) {
	if (event != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s *)ptr)->id) {
	case ID_VOTEYES:
		trap_Cmd_ExecuteText(EXEC_APPEND, "vote yes");
		break;

	case ID_VOTENO:
		trap_Cmd_ExecuteText(EXEC_APPEND, "vote no");
		break;

	default:
		break;
	}
}

/*
=================
UI_CallVote_GoEvent
=================
*/
static void UI_CallVote_GoEvent(void *ptr, int event) {
	char *tmpStr = NULL;

	if (event != QM_ACTIVATED) {
		return;
	}

	s_callVoteMenu.lastGoTime = uis.realtime;
	s_callVoteMenu.go.generic.flags |= QMF_GRAYED;

	// FIXME: switch-case on votetype.curvalue instead of strcmp
	//        also in UI_CallVote_CVType()

	if (s_callVoteMenu.numListitems) {
		if (Q_stricmp(s_callVoteMenu.votetype.itemnames[s_callVoteMenu.votetype.curvalue], "gametype") == 0) {
			tmpStr = va("callvote setGametype \"%s\"", s_callVoteMenu.selItemStr);
		} else {
			tmpStr = va("callvote %s \"%s\"", s_callVoteMenu.votetype.itemnames[s_callVoteMenu.votetype.curvalue],
						s_callVoteMenu.selItemStr);
		}
	} else if (!(s_callVoteMenu.value.generic.flags & (QMF_INACTIVE | QMF_HIDDEN))) {
		tmpStr = va("callvote %s \"%s\"", s_callVoteMenu.votetype.itemnames[s_callVoteMenu.votetype.curvalue],
					s_callVoteMenu.value.field.buffer);
	} else {
		if (!Q_stricmp(s_callVoteMenu.votetype.itemnames[s_callVoteMenu.votetype.curvalue], "next map"))
			tmpStr = "callvote nextmap";
		else if (!Q_stricmp(s_callVoteMenu.votetype.itemnames[s_callVoteMenu.votetype.curvalue], "restart map"))
			tmpStr = "callvote map_restart";
		else
			tmpStr = va("callvote %s", s_callVoteMenu.votetype.itemnames[s_callVoteMenu.votetype.curvalue]);
	}

	uis.musicbool = 2;
	trap_Cvar_Set("cl_paused", "0");

	trap_Cmd_ExecuteText(EXEC_APPEND, tmpStr);
	Com_Printf("Menu sent vote: %s\n", tmpStr);
}

/*
=================
UI_CallVote_ClickListItem
=================
*/
static void UI_CallVote_ClickListItem(void *ptr, int event) {
	if (event != QM_ACTIVATED) {
		return;
	}

	s_callVoteMenu.selectedItem = (((const menucommon_s *)ptr)->id - ID_LISTITEM0 + s_callVoteMenu.baseItem);
	s_callVoteMenu.selItemStr = ((const menutext_s *)ptr)->string;
}

/*
=================
UI_CallVote_BackEvent
=================
*/
static void UI_CallVote_BackEvent(void *ptr, int event) {
	if (event != QM_ACTIVATED) {
		return;
	}

	UI_PopMenu();
}

static void UI_CallVote_UpdateItems(void) {
	int i = 0, j;
	char *ptr = ArrayForListStrs;

	for (j = 0; j < s_callVoteMenu.numListitems; j++, ptr += (strlen(ptr) + 1)) {
		i = (j - s_callVoteMenu.baseItem);
		if ((i >= 0) && (i < NUM_LISTITEMS)) {
			s_callVoteMenu.listitems[i].string = ptr;
			s_callVoteMenu.listitems[i].generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);
		}
	}

	for (j = (i + 1); j < NUM_LISTITEMS; j++) {
		s_callVoteMenu.listitems[j].generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
	}
}

/*
=================
UI_CallVote_UpEvent
=================
*/
static void UI_CallVote_UpEvent(void *ptr, int event) {
	if (event != QM_ACTIVATED) {
		return;
	}

	if (s_callVoteMenu.baseItem > 0) {
		--s_callVoteMenu.baseItem;
		UI_CallVote_UpdateItems();
	}
}

/*
=================
UI_CallVote_DownEvent
=================
*/
static void UI_CallVote_DownEvent(void *ptr, int event) {
	if (event != QM_ACTIVATED) {
		return;
	}

	if ((s_callVoteMenu.baseItem + NUM_LISTITEMS) < s_callVoteMenu.numListitems) {
		++s_callVoteMenu.baseItem;
		UI_CallVote_UpdateItems();
	}
}

static void UI_CallVote_UpdatePassiveButtons(void) {
	int i;

	if (s_callVoteMenu.voteRunning) {
		for (i = 0; i < NUM_LISTITEMS; i++) {
			s_callVoteMenu.listitems[i].generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		}
		s_callVoteMenu.votetype.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		s_callVoteMenu.value.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		s_callVoteMenu.go.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		s_callVoteMenu.arrowup.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		s_callVoteMenu.arrowdown.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);

		s_callVoteMenu.voteyes.generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);
		s_callVoteMenu.voteno.generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);
	} else {
		s_callVoteMenu.votetype.generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);
		UI_CallVote_CVType(&s_callVoteMenu.votetype, QM_ACTIVATED);
		s_callVoteMenu.go.generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);

		s_callVoteMenu.voteyes.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		s_callVoteMenu.voteno.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
	}
}

/*
=================
UI_CallVote_Draw
=================
*/
static void UI_CallVote_Draw(void) {
	char info[MAX_INFO_STRING];

	UI_DrawIngameBG();
	UI_DrawProportionalString(320, 110, "CALL VOTE", (UI_CENTER | UI_SMALLFONT), color_black);

	trap_GetConfigString(CS_VOTE_TIME, info, sizeof(info));

	if (info[0] != '\0') {
		int sec;
		int i;
		char buff[32];

		if (!s_callVoteMenu.voteRunning) {
			s_callVoteMenu.voteRunning = qtrue;
			UI_CallVote_UpdatePassiveButtons();
		}

		sec = ((atoi(info) + VOTE_TIME) / 1000);

		UI_DrawStringNS(240, 130, "Vote in progress", UI_LEFT, 16.0f, colorBlack);
		UI_DrawStringNS(240, (130 + 16), va("Lasts: %d:%2.2d", (sec / 60), (sec % 60)), UI_LEFT, 16.0f, colorBlack);

		trap_GetConfigString(CS_VOTE_STRING, info, sizeof(info));
		UI_DrawStringNS(240, (130 + 40), "Vote is about:", UI_LEFT, 16.0f, colorBlack);
		i = 0;
#define MAXLINELEN 20
		do {
			Q_strncpyz(buff, (info + (i * MAXLINELEN)), (MAXLINELEN + 1));
			i++;
			UI_DrawStringNS(240, (130 + 40 + (17 * i)), buff, UI_LEFT, 16.0f, colorBlack);
		} while (strlen(buff) == MAXLINELEN);

		trap_GetConfigString(CS_VOTE_YES, info, sizeof(info));
		UI_DrawStringNS(230, 285, info, UI_LEFT, 23.0f, colorBlack);

		trap_GetConfigString(CS_VOTE_NO, info, sizeof(info));
		UI_DrawStringNS(360, 285, info, UI_RIGHT, 23.0f, colorBlack);
	} else {
		if (s_callVoteMenu.voteRunning) {
			s_callVoteMenu.voteRunning = qfalse;
			UI_CallVote_UpdatePassiveButtons();
		}

		if ((uis.realtime - s_callVoteMenu.lastGoTime) > 5000) {
			s_callVoteMenu.go.generic.flags &= ~QMF_GRAYED;
		}

		UI_DrawStringNS(240, 130, "Vote type:", UI_LEFT, 18.0f, colorBlack);

		if (s_callVoteMenu.numListitems) {
			int i = (s_callVoteMenu.selectedItem - s_callVoteMenu.baseItem);
			if ((i >= 0) && (i < NUM_LISTITEMS)) {
				UI_FillRect((230 - 2), (170 + (i * 18)), (LISTITEM_WIDTH + 4), 18, colorTOrange);
			}
		}
	}

	// standard menu drawing
	Menu_Draw(&s_callVoteMenu.menu);
}

/*
#######################

  CallVoteMenu_Key

#######################
*/
static sfxHandle_t CallVoteMenu_Key(int key) {
	switch (key) {
	case K_MWHEELUP:
	case K_PGUP:
		UI_CallVote_UpEvent(&s_callVoteMenu.arrowup, QM_ACTIVATED);
		break;

	case K_MWHEELDOWN:
	case K_PGDN:
		UI_CallVote_DownEvent(&s_callVoteMenu.arrowdown, QM_ACTIVATED);
		break;

	default:
		break;
	}

	return Menu_DefaultKey(&s_callVoteMenu.menu, key);
}

static void removeEndings(char *buff, int buffsize, const char *ending) {
	char *in = buff;
	char *out = buff;
	int endingLen = strlen(ending);

	for (; (in - buff) < buffsize; in++, out++) {
		if (strcmp(in, ending) == 0) {
			in += endingLen;
			if ((in - buff) >= buffsize) {
				*out = '\0';
				break;
			}
		}

		if (in != out) {
			*out = *in;
		}
	}
}

// TODO: Add a removeRedirect function for suffixes like mapnameBB

/*
=================
UI_CallVote_DownEvent
=================
*/
// NOTE: Those are not all gametypes, this is intentional
static const char *cvGametype_strs[] = {GAMETYPE_NAME(GT_FFA),	 GAMETYPE_NAME(GT_SPRAYFFA),  GAMETYPE_NAME(GT_LPS),
										GAMETYPE_NAME(GT_TEAM),	 GAMETYPE_NAME(GT_FREEZETAG), GAMETYPE_NAME(GT_CTF),
										GAMETYPE_NAME(GT_SPRAY), GAMETYPE_NAME(GT_BALLOON),	  NULL};

static void UI_CallVote_CVType(void *unused, int event) {
	char info[MAX_INFO_STRING];
	char *ptr;
	int i;
	const char *cvTypeStr;

	if (event != QM_ACTIVATED) {
		return;
	}

	s_callVoteMenu.value.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
	s_callVoteMenu.numListitems = 0;
	s_callVoteMenu.baseItem = 0;
	s_callVoteMenu.selectedItem = 0;
	s_callVoteMenu.selItemStr = ArrayForListStrs;
	memset(ArrayForListStrs, 0, MAX_LISTSTRARRAY);

	cvTypeStr = s_callVoteMenu.votetype.itemnames[s_callVoteMenu.votetype.curvalue];

	if (Q_stricmp(cvTypeStr, "timelimit") == 0) {
		s_callVoteMenu.value.generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);

		trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));
		Q_strncpyz(s_callVoteMenu.value.field.buffer, Info_ValueForKey(info, "timelimit"),
				   sizeof(s_callVoteMenu.value.field.buffer));
	} else if (Q_stricmp(cvTypeStr, "pointlimit") == 0) {
		s_callVoteMenu.value.generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);

		trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));
		Q_strncpyz(s_callVoteMenu.value.field.buffer, Info_ValueForKey(info, "pointlimit"),
				   sizeof(s_callVoteMenu.value.field.buffer));
	} else if (Q_stricmp(cvTypeStr, "gametype") == 0) {
		ptr = ArrayForListStrs;

		for (i = 0; cvGametype_strs[i]; i++) {
			Q_strncpyz(ptr, cvGametype_strs[i], (MAX_LISTSTRARRAY - (ptr - ArrayForListStrs)));
			++s_callVoteMenu.numListitems;

			ptr += (strlen(ptr) + 1);
			if (MAX_LISTSTRARRAY < (ptr - ArrayForListStrs)) {
				break;
			}
		}
	} else if (Q_stricmp(cvTypeStr, "map") == 0) {
		s_callVoteMenu.numListitems = trap_FS_GetFileList("maps", "bsp", ArrayForListStrs, MAX_LISTSTRARRAY);
		removeEndings(ArrayForListStrs, MAX_LISTSTRARRAY, ".bsp");
	} else if (Q_stricmp(cvTypeStr, "kick") == 0) {
		int numPlayers;
		trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));
		numPlayers = atoi(Info_ValueForKey(info, "sv_maxclients"));
		ptr = ArrayForListStrs;

		for (i = 0; i < numPlayers; i++) {
			const char *tmpcp;
			trap_GetConfigString((CS_PLAYERS + i), info, sizeof(info));

			tmpcp = Info_ValueForKey(info, "n");
			if (tmpcp && (tmpcp[0] != '\0')) {
				Q_strncpyz(ptr, Info_ValueForKey(info, "n"), (MAX_LISTSTRARRAY - (ptr - ArrayForListStrs)));
				++s_callVoteMenu.numListitems;

				ptr += (strlen(ptr) + 1);
				if (MAX_LISTSTRARRAY < (ptr - ArrayForListStrs)) {
					break;
				}
			}
		}
	}

	//	"nextmap"
	//	"map_restart"

	UI_CallVote_UpdateItems();

	if (s_callVoteMenu.numListitems > NUM_LISTITEMS) {
		s_callVoteMenu.arrowdown.generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);
		s_callVoteMenu.arrowup.generic.flags &= ~(QMF_INACTIVE | QMF_HIDDEN);
	} else {
		s_callVoteMenu.arrowdown.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
		s_callVoteMenu.arrowup.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);
	}
}

/*
=================
UI_CallVoteMenu_Init
=================
*/

static void UI_CallVoteMenu_Init(void) {
	int n;
	int y;
	char info[MAX_INFO_STRING];

	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	memset(&s_callVoteMenu, 0, sizeof(s_callVoteMenu));
	s_callVoteMenu.menu.draw = UI_CallVote_Draw;
	s_callVoteMenu.menu.fullscreen = qfalse;
	s_callVoteMenu.menu.wrapAround = qtrue;
	s_callVoteMenu.menu.key = CallVoteMenu_Key;

	UI_CallVote_Cache();

	y = (130 + 18);
	s_callVoteMenu.votetype.generic.type = MTYPE_SPINCONTROL;
	s_callVoteMenu.votetype.generic.flags = (QMF_SMALLFONT | QMF_INGAMESTYLE);
	s_callVoteMenu.votetype.generic.x = 250;
	s_callVoteMenu.votetype.generic.y = (130 + 18);
	s_callVoteMenu.votetype.generic.name = "";
	s_callVoteMenu.votetype.generic.id = ID_CALLVOTETYPE;
	s_callVoteMenu.votetype.itemnames = callvote_strs;
	s_callVoteMenu.votetype.curvalue = 4; // "map"
	s_callVoteMenu.votetype.generic.callback = UI_CallVote_CVType;

	y += 20;
	s_callVoteMenu.value.generic.type = MTYPE_FIELD;
	s_callVoteMenu.value.generic.name = "Value:";
	s_callVoteMenu.value.generic.flags = (QMF_NUMBERSONLY | QMF_SMALLFONT | QMF_INGAMESTYLE);
	s_callVoteMenu.value.generic.x = 300;
	s_callVoteMenu.value.generic.y = y;
	s_callVoteMenu.value.field.widthInChars = 5;
	s_callVoteMenu.value.field.maxchars = 5;
	s_callVoteMenu.value.generic.flags |= (QMF_INACTIVE | QMF_HIDDEN);

	s_callVoteMenu.arrowup.generic.type = MTYPE_BITMAP1024S;
	s_callVoteMenu.arrowup.x = 638;
	s_callVoteMenu.arrowup.y = 300;
	s_callVoteMenu.arrowup.w = 29;
	s_callVoteMenu.arrowup.h = 74;
	s_callVoteMenu.arrowup.shader = trap_R_RegisterShaderNoMip(ART_ARROWUP0);
	s_callVoteMenu.arrowup.mouseovershader = trap_R_RegisterShaderNoMip(ART_ARROWUP1);
	s_callVoteMenu.arrowup.generic.callback = UI_CallVote_UpEvent;
	s_callVoteMenu.arrowup.generic.id = ID_UP;

	s_callVoteMenu.arrowdown.generic.type = MTYPE_BITMAP1024S;
	s_callVoteMenu.arrowdown.x = 638;
	s_callVoteMenu.arrowdown.y = (472 - 74);
	s_callVoteMenu.arrowdown.w = 29;
	s_callVoteMenu.arrowdown.h = 74;
	s_callVoteMenu.arrowdown.shader = trap_R_RegisterShaderNoMip(ART_ARROWDOWN0);
	s_callVoteMenu.arrowdown.mouseovershader = trap_R_RegisterShaderNoMip(ART_ARROWDOWN1);
	s_callVoteMenu.arrowdown.generic.callback = UI_CallVote_DownEvent;
	s_callVoteMenu.arrowdown.generic.id = ID_DOWN;

	for (n = 0, y = 170; n < NUM_LISTITEMS; n++, y += 18) {
		s_callVoteMenu.listitems[n].generic.type = MTYPE_TEXTS;
		s_callVoteMenu.listitems[n].fontHeight = 16.0f;
		s_callVoteMenu.listitems[n].generic.flags = QMF_LEFT_JUSTIFY;
		s_callVoteMenu.listitems[n].generic.id = (ID_LISTITEM0 + n);
		s_callVoteMenu.listitems[n].generic.x = 230;
		s_callVoteMenu.listitems[n].generic.y = y;
		s_callVoteMenu.listitems[n].generic.callback = UI_CallVote_ClickListItem;
		s_callVoteMenu.listitems[n].string = "foobar";
		s_callVoteMenu.listitems[n].color = color_black;
		s_callVoteMenu.listitems[n].focuscolor = colorMOver;
		s_callVoteMenu.listitems[n].style = (UI_LEFT | UI_SMALLFONT);
	}

	y = 270;

	s_callVoteMenu.voteyes.generic.type = MTYPE_TEXTS;
	s_callVoteMenu.voteyes.fontHeight = 32.0;
	s_callVoteMenu.voteyes.generic.flags = QMF_LEFT_JUSTIFY;
	s_callVoteMenu.voteyes.generic.callback = UI_CallVote_Vote;
	s_callVoteMenu.voteyes.generic.id = ID_VOTEYES;
	s_callVoteMenu.voteyes.generic.x = 230;
	s_callVoteMenu.voteyes.generic.y = 250;
	s_callVoteMenu.voteyes.string = "YES";
	s_callVoteMenu.voteyes.style = (UI_SMALLFONT | UI_LEFT);
	s_callVoteMenu.voteyes.color = color_black;
	s_callVoteMenu.voteyes.focuscolor = color_orange;

	s_callVoteMenu.voteno.generic.type = MTYPE_TEXTS;
	s_callVoteMenu.voteno.fontHeight = 32.0f;
	s_callVoteMenu.voteno.generic.flags = QMF_RIGHT_JUSTIFY;
	s_callVoteMenu.voteno.generic.callback = UI_CallVote_Vote;
	s_callVoteMenu.voteno.generic.id = ID_VOTENO;
	s_callVoteMenu.voteno.generic.x = 360;
	s_callVoteMenu.voteno.generic.y = 250;
	s_callVoteMenu.voteno.string = "NO";
	s_callVoteMenu.voteno.style = (UI_SMALLFONT | UI_RIGHT);
	s_callVoteMenu.voteno.color = color_black;
	s_callVoteMenu.voteno.focuscolor = color_orange;

	y = 300;
	s_callVoteMenu.go.generic.type = MTYPE_TEXTS;
	s_callVoteMenu.go.fontHeight = 24.0f;
	s_callVoteMenu.go.generic.flags = QMF_RIGHT_JUSTIFY;
	s_callVoteMenu.go.generic.callback = UI_CallVote_GoEvent;
	s_callVoteMenu.go.generic.id = ID_GO;
	s_callVoteMenu.go.generic.x = 360;
	s_callVoteMenu.go.generic.y = (340 - 4);
	s_callVoteMenu.go.string = "GO!";
	s_callVoteMenu.go.style = (UI_SMALLFONT | UI_RIGHT);
	s_callVoteMenu.go.color = color_black;
	s_callVoteMenu.go.focuscolor = color_orange;

	s_callVoteMenu.back.generic.type = MTYPE_TEXTS;
	s_callVoteMenu.back.fontHeight = 16.0f;
	s_callVoteMenu.back.generic.callback = UI_CallVote_BackEvent;
	s_callVoteMenu.back.generic.id = ID_BACK;
	s_callVoteMenu.back.generic.x = 230;
	s_callVoteMenu.back.generic.y = 340;
	s_callVoteMenu.back.string = "BACK";
	s_callVoteMenu.back.style = UI_SMALLFONT;
	s_callVoteMenu.back.color = color_black;
	s_callVoteMenu.back.focuscolor = color_orange;

	Menu_AddItem(&s_callVoteMenu.menu, &s_callVoteMenu.votetype);
	Menu_AddItem(&s_callVoteMenu.menu, &s_callVoteMenu.arrowup);
	Menu_AddItem(&s_callVoteMenu.menu, &s_callVoteMenu.arrowdown);
	Menu_AddItem(&s_callVoteMenu.menu, &s_callVoteMenu.value);

	for (n = 0; n < NUM_LISTITEMS; n++) {
		Menu_AddItem(&s_callVoteMenu.menu, &s_callVoteMenu.listitems[n]);
		s_callVoteMenu.listitems[n].generic.right = (s_callVoteMenu.listitems[n].generic.left + LISTITEM_WIDTH);
	}

	Menu_AddItem(&s_callVoteMenu.menu, &s_callVoteMenu.voteyes);
	Menu_AddItem(&s_callVoteMenu.menu, &s_callVoteMenu.voteno);
	Menu_AddItem(&s_callVoteMenu.menu, &s_callVoteMenu.go);
	Menu_AddItem(&s_callVoteMenu.menu, &s_callVoteMenu.back);

	UI_CallVote_UpdatePassiveButtons();
}

/*
=================
UI_CallVote_Cache
=================
*/
static void UI_CallVote_Cache(void) {
	trap_R_RegisterShaderNoMip(ART_ARROWUP0);
	trap_R_RegisterShaderNoMip(ART_ARROWUP1);
	trap_R_RegisterShaderNoMip(ART_ARROWDOWN0);
	trap_R_RegisterShaderNoMip(ART_ARROWDOWN1);
}

/*
=================
UI_CallVoteMenu
=================
*/
void UI_CallVoteMenu(void) {
	UI_CallVoteMenu_Init();
	UI_PushMenu(&s_callVoteMenu.menu);
}
