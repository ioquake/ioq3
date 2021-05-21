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
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

uiStatic_t uis;
qboolean m_entersound; // after a frame, so caching won't disrupt the sound

// these are here so the functions in q_shared.c can link
#ifndef UI_HARD_LINKED

void QDECL Com_Error(int level, const char *error, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL Com_Printf(const char *msg, ...) {
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

#endif

/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar(float min, float max, float value) {
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

/*
=================
UI_StartDemoLoop
=================
*/
void UI_StartDemoLoop(void) {
	trap_Cmd_ExecuteText(EXEC_APPEND, "d1\n");
}

void UI_StartCreditMusic(void) {
	uis.musicbool = qtrue;
	trap_S_StopBackgroundTrack();
	trap_S_StartBackgroundTrack("music/credits", "music/credits");
}

void UI_StartMusic(void) {
	uis.musicbool = qtrue;
	trap_S_StartBackgroundTrack("music/menuloop", "music/menuloop");
}

void UI_StopMusic(void) {
	if (uis.musicbool == qtrue) // nicht stopen wenn 2(wop_music menu -> exit) oder 0(keine music gestartet)
		trap_S_StopBackgroundTrack();
	uis.musicbool = qfalse;
}

/*
=================
UI_PushMenu
=================
*/
void UI_PushMenu(menuframework_s *menu) {
	int i;
	menucommon_s *item;

	// avoid stacking menus invoked by hotkeys
	for (i = 0; i < uis.menusp; i++) {
		if (uis.stack[i] == menu) {
			uis.menusp = i;
			break;
		}
	}

	if (i == uis.menusp) {
		if (uis.menusp >= MAX_MENUDEPTH)
			trap_Error("UI_PushMenu: menu stack overflow");

		uis.stack[uis.menusp++] = menu;
	}

	uis.activemenu = menu;

	// default cursor position
	menu->cursor = 0;
	menu->cursor_prev = 0;

	m_entersound = qtrue;

	trap_Key_SetCatcher(KEYCATCH_UI);

	if (menu->noPushSelect) {
		menu->cursor = -1;
		//		Menu_CursorMoved( &s_main.menu );
	} else
		// force first available item to have focus
		for (i = 0; i < menu->nitems; i++) {
			item = (menucommon_s *)menu->items[i];
			if (!(item->flags & (QMF_GRAYED | QMF_MOUSEONLY | QMF_INACTIVE))) {
				menu->cursor_prev = -1;
				Menu_SetCursor(menu, i);
				break;
			}
		}

	uis.firstdraw = qtrue;
}

/*
=================
UI_PopMenu
=================
*/
void UI_PopMenu(void) {
	uis.menusp--;

	if (uis.menusp < 0)
		trap_Error("UI_PopMenu: menu stack underflow");

	if (uis.menusp) {
		uis.activemenu = uis.stack[uis.menusp - 1];
		uis.firstdraw = qtrue;
	} else {
		UI_ForceMenuOff();
	}
}

void UI_ForceMenuOff(void) {
	uis.menusp = 0;
	uis.activemenu = NULL;

	if (!UI_GetCvarInt("cl_paused"))
		UI_StopMusic();

	trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
	trap_Key_ClearStates();
	trap_Cvar_Set("cl_paused", "0");
}

/*
=================
UI_LerpColor
=================
*/
void UI_LerpColor(const vec4_t a, const vec4_t b, vec4_t c, float t) {
	int i;

	// lerp and clamp each component
	for (i = 0; i < 4; i++) {
		c[i] = a[i] + t * (b[i] - a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0)
			c[i] = 1.0;
	}
}

/*
=================
UI_DrawProportionalString2
=================
*/
static const struct FontData {
	int x; // x offset in font atlas
	int y; // y offset in font atlas
	int width; // width of the character
} propMap[128] = {
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},

	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},
	{0, 0, -1},

	{0, 0, PROP_SPACE_WIDTH}, // SPACE
	{11, 122, 7},			  // !
	{154, 181, 14},			  // "
	{55, 122, 17},			  // #
	{79, 122, 18},			  // $
	{101, 122, 23},			  // %
	{153, 122, 18},			  // &
	{9, 93, 7},				  // '
	{207, 122, 8},			  // (
	{230, 122, 9},			  // )
	{177, 122, 18},			  // *
	{30, 152, 18},			  // +
	{85, 181, 7},			  // ,
	{34, 93, 11},			  // -
	{110, 181, 6},			  // .
	{130, 152, 14},			  // /

	{22, 64, 17},	// 0
	{41, 64, 12},	// 1
	{58, 64, 17},	// 2
	{78, 64, 18},	// 3
	{98, 64, 19},	// 4
	{120, 64, 18},	// 5
	{141, 64, 18},	// 6
	{204, 64, 16},	// 7
	{162, 64, 17},	// 8
	{182, 64, 18},	// 9
	{59, 181, 7},	// :
	{35, 181, 7},	// ;
	{203, 152, 14}, // <
	{56, 93, 14},	// =
	{228, 152, 14}, // >
	{177, 181, 18}, // ?

	{28, 122, 22}, // @
	{5, 4, 18},	   // A
	{27, 4, 18},   // B
	{48, 4, 18},   // C
	{69, 4, 17},   // D
	{90, 4, 13},   // E
	{106, 4, 13},  // F
	{121, 4, 18},  // G
	{143, 4, 17},  // H
	{164, 4, 8},   // I
	{175, 4, 16},  // J
	{195, 4, 18},  // K
	{216, 4, 12},  // L
	{230, 4, 23},  // M
	{6, 34, 18},   // N
	{27, 34, 18},  // O

	{48, 34, 18},	// P
	{68, 34, 18},	// Q
	{90, 34, 17},	// R
	{110, 34, 18},	// S
	{130, 34, 14},	// T
	{146, 34, 18},	// U
	{166, 34, 19},	// V
	{185, 34, 29},	// W
	{215, 34, 18},	// X
	{234, 34, 18},	// Y
	{5, 64, 14},	// Z
	{60, 152, 7},	// [
	{106, 151, 13}, // '\'
	{83, 152, 7},	// ]
	{128, 122, 17}, // ^
	{4, 152, 21},	// _

	{134, 181, 5}, // '
	{5, 4, 18},	   // A
	{27, 4, 18},   // B
	{48, 4, 18},   // C
	{69, 4, 17},   // D
	{90, 4, 13},   // E
	{106, 4, 13},  // F
	{121, 4, 18},  // G
	{143, 4, 17},  // H
	{164, 4, 8},   // I
	{175, 4, 16},  // J
	{195, 4, 18},  // K
	{216, 4, 12},  // L
	{230, 4, 23},  // M
	{6, 34, 18},   // N
	{27, 34, 18},  // O

	{48, 34, 18},	// P
	{68, 34, 18},	// Q
	{90, 34, 17},	// R
	{110, 34, 18},	// S
	{130, 34, 14},	// T
	{146, 34, 18},	// U
	{166, 34, 19},	// V
	{185, 34, 29},	// W
	{215, 34, 18},	// X
	{234, 34, 18},	// Y
	{5, 64, 14},	// Z
	{153, 152, 13}, // {
	{11, 181, 5},	// |
	{180, 152, 13}, // }
	{79, 93, 17},	// ~
	{0, 0, -1}		// DEL
};

#define PROPB_GAP_WIDTH 4
#define PROPB_SPACE_WIDTH 12
#define PROPB_HEIGHT 36

int UI_ProportionalStringWidth(const char *str) {
	const char *s;
	int ch;
	int charWidth;
	int width;

	s = str;
	width = 0;
	while (*s) {
		ch = *s & 127;
		charWidth = propMap[ch].width;
		if (charWidth != -1) {
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
		s++;
	}

	width -= PROP_GAP_WIDTH;
	return width;
}

void UI_DrawProportionalString2(int x, int y, const char *str, const vec4_t color, float sizeScale, qhandle_t charset) {
	const char *s;
	unsigned char ch;
	float ax;
	float ay;
	float aw = 0;
	float ah;
	float frow;
	float fcol;
	float fwidth;
	float fheight;

	// draw the colored text
	trap_R_SetColor(color);

	ax = x * uis.xscale + uis.xbias;
	ay = y * uis.yscale + uis.ybias;

	s = str;
	while (*s) {
		ch = *s & 127;
		if (ch == ' ') {
			aw = (float)PROP_SPACE_WIDTH * uis.xscale * sizeScale;
		} else if (propMap[ch].width != -1) {
			fcol = (float)propMap[ch].x / 256.0f;
			frow = (float)propMap[ch].y / 256.0f;
			fwidth = (float)propMap[ch].width / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch].width * uis.xscale * sizeScale;
			ah = (float)PROP_HEIGHT * uis.yscale * sizeScale;
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, charset);
		}

		ax += (aw + (float)PROP_GAP_WIDTH * uis.xscale * sizeScale);
		s++;
	}

	trap_R_SetColor(NULL);
}

/*
=================
UI_ProportionalSizeScale
=================
*/
float UI_ProportionalSizeScale(int style) {
	if (style & UI_SMALLFONT) {
		return PROP_SMALL_SIZE_SCALE;
	}

	return 1.0f;
}

/*
=================
UI_DrawProportionalString
=================
*/
void UI_DrawProportionalString(int x, int y, const char *str, int style, const vec4_t color) {
	vec4_t drawcolor;
	int width;
	float sizeScale;

	sizeScale = UI_ProportionalSizeScale(style);

	switch (style & UI_FORMATMASK) {
	case UI_CENTER:
		width = UI_ProportionalStringWidth(str) * sizeScale;
		x -= width / 2;
		break;

	case UI_RIGHT:
		width = UI_ProportionalStringWidth(str) * sizeScale;
		x -= width;
		break;

	case UI_LEFT:
	default:
		break;
	}

	if (style & UI_DROPSHADOW) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2(x + 2, y + 2, str, drawcolor, sizeScale, uis.charsetProp);
	}

	if (style & UI_INVERSE) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2(x, y, str, drawcolor, sizeScale, uis.charsetProp);
		return;
	}

	if (style & UI_PULSE) {
		UI_DrawProportionalString2(x, y, str, color, sizeScale, uis.charsetProp);
		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
		drawcolor[3] = 0.75f + 0.25f * sin(uis.realtime / PULSE_DIVISOR);
		UI_DrawProportionalString2(x - 2 + (int)(random() * 5.99f), y - 2 + (int)(random() * 5.99f), str, drawcolor,
								   sizeScale, uis.charsetProp);

		return;
	}

	UI_DrawProportionalString2(x, y, str, color, sizeScale, uis.charsetProp);
}

int CharWidthForStringStyle(int style) {
	if (style & UI_SMALLFONT)
		return SMALLCHAR_WIDTH;

	else if (style & UI_GIANTFONT)
		return GIANTCHAR_WIDTH;

	return BIGCHAR_WIDTH;
}

int CharHeightForStringStyle(int style) {
	if (style & UI_SMALLFONT)
		return SMALLCHAR_HEIGHT;

	else if (style & UI_GIANTFONT)
		return GIANTCHAR_HEIGHT;

	return BIGCHAR_HEIGHT;
}

int UI_AutoWrappedString_LineCount(int xmax, const char *str, int style, qboolean proportional) {
	char *s1, *s2, *s3;
	char c_bcp;
	char buf[1024];
	float sizeScale;
	int lineCount = 0;
	int width;

	if (!str || str[0] == '\0')
		return 0;

	sizeScale = UI_ProportionalSizeScale(style);

	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while (1) {
		do {
			s3++;
		} while (*s3 != ' ' && *s3 != '\0' && *s3 != '\n');
		c_bcp = *s3;
		*s3 = '\0';

		if (proportional)
			width = UI_ProportionalStringWidth(s1) * sizeScale;
		else
			width = strlen(s1) * CharWidthForStringStyle(style);

		*s3 = c_bcp;
		if (width > xmax || c_bcp == '\n') {
			if (s1 == s2) {
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			}
			*s2 = '\0';
			lineCount++;
			if (c_bcp == '\0') {
				// that was the last word
				// we could start a new loop, but that wouldn't be much use
				// even if the word is too long, we would overflow it (see above)
				// so just print it now if needed
				s2++;
				if (*s2 != '\0') // if we are printing an overflowing line we have s2 == s3
					lineCount++;
				break;
			}
			s2++;
			s1 = s2;
			s3 = s2;
		} else {
			s2 = s3;
			if (c_bcp == '\0') // we reached the end
			{
				lineCount++;
				break;
			}
		}
	}
	return lineCount;
}

/*
=================
UI_DrawProportionalString_Wrapped
=================
*/
void UI_DrawString_AutoWrapped(int x, int y, int xmax, int ystep, const char *str, int style, const vec4_t color,
							   qboolean proportional) {
	int width;
	char *s1, *s2, *s3;
	char c_bcp;
	char buf[1024];
	float sizeScale;

	if (!str || str[0] == '\0')
		return;

	sizeScale = UI_ProportionalSizeScale(style);

	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while (1) {
		do {
			s3++;
		} while (*s3 != ' ' && *s3 != '\0' && *s3 != '\n');

		if (*s3 == '\n') {
			*s3 = '\0';

			if (proportional)
				UI_DrawProportionalString(x, y, s1, style, color);
			else
				UI_DrawString(x, y, s1, style, color);

			y += ystep;
			s3++;
			s1 = s2 = s3;
			continue;
		}

		c_bcp = *s3;
		*s3 = '\0';

		if (proportional)
			width = UI_ProportionalStringWidth(s1) * sizeScale;
		else
			width = strlen(s1) * CharWidthForStringStyle(style);

		*s3 = c_bcp;
		if (width > xmax) {
			if (s1 == s2) {
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			}
			*s2 = '\0';

			if (proportional)
				UI_DrawProportionalString(x, y, s1, style, color);
			else
				UI_DrawString(x, y, s1, style, color);

			y += ystep;
			if (c_bcp == '\0') {
				// that was the last word
				// we could start a new loop, but that wouldn't be much use
				// even if the word is too long, we would overflow it (see above)
				// so just print it now if needed
				s2++;
				if (*s2 != '\0') // if we are printing an overflowing line we have s2 == s3
				{
					if (proportional)
						UI_DrawProportionalString(x, y, s2, style, color);
					else
						UI_DrawString(x, y, s2, style, color);
				}
				break;
			}
			s2++;
			s1 = s2;
			s3 = s2;
		} else {
			s2 = s3;
			if (c_bcp == '\0') // we reached the end
			{
				if (proportional)
					UI_DrawProportionalString(x, y, s1, style, color);
				else
					UI_DrawString(x, y, s1, style, color);
				break;
			}
		}
	}
}

/*
=================
UI_DrawString2
=================
*/
static void UI_DrawString2(int x, int y, const char *str, const vec4_t color, int charw, int charh) {
	const char *s;
	char ch;
	int forceColor = qfalse; // APSFIXME;
	vec4_t tempcolor;
	float ax;
	float ay;
	float aw;
	float ah;
	float frow;
	float fcol;

	if (y < -charh)
		// offscreen
		return;

	// draw the colored text
	trap_R_SetColor(color);

	ax = x * uis.xscale + uis.xbias;
	ay = y * uis.yscale + uis.ybias;

	aw = charw * uis.xscale;
	ah = charh * uis.yscale;

	s = str;
	while (*s) {
		if (Q_IsColorString(s)) {
			if (!forceColor) {
				memcpy(tempcolor, g_color_table[ColorIndex(s[1])], sizeof(tempcolor));
				tempcolor[3] = color[3];
				trap_R_SetColor(tempcolor);
			}
			s += 2;
			continue;
		}

		ch = *s & 255;
		if (ch != ' ') {
			frow = (ch >> 4) * 0.0625;
			fcol = (ch & 15) * 0.0625;
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + 0.0625, frow + 0.0625, uis.charsetShader);
		}

		ax += aw;
		s++;
	}

	trap_R_SetColor(NULL);
}

/*
=================
UI_DrawString
=================
*/
void UI_DrawString(int x, int y, const char *str, int style, const vec4_t color) {
	int len;
	int charw;
	int charh;
	vec4_t newcolor;
	vec4_t lowlight;
	float *drawcolor;
	vec4_t dropcolor;

	if (!str) {
		return;
	}

	if ((style & UI_BLINK) && ((uis.realtime / BLINK_DIVISOR) & 1))
		return;

	charw = CharWidthForStringStyle(style);
	charh = CharHeightForStringStyle(style);

	if (style & UI_PULSE) {
		lowlight[0] = 0.8 * color[0];
		lowlight[1] = 0.8 * color[1];
		lowlight[2] = 0.8 * color[2];
		lowlight[3] = 0.8 * color[3];
		UI_LerpColor(color, lowlight, newcolor, (float)(0.5 + 0.5 * sin(uis.realtime / PULSE_DIVISOR)));
	} else {
		Vector4Copy(color, newcolor);
	}
	drawcolor = newcolor;

	switch (style & UI_FORMATMASK) {
	case UI_CENTER:
		// center justify at x
		len = strlen(str);
		x = x - len * charw / 2;
		break;

	case UI_RIGHT:
		// right justify at x
		len = strlen(str);
		x = x - len * charw;
		break;

	default:
		// left justify at x
		break;
	}

	if (style & UI_DROPSHADOW) {
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3];
		UI_DrawString2(x + 2, y + 2, str, dropcolor, charw, charh);
	}

	UI_DrawString2(x, y, str, drawcolor, charw, charh);
}

/*
=================
UI_DrawStringNS
=================
*/
void UI_DrawStringNS(int x, int y, const char *str, int style, float fontsize, const vec4_t color) {
	int len;
	int charw;
	int charh;
	vec4_t newcolor;
	vec4_t lowlight;
	const float *drawcolor;
	vec4_t dropcolor;

	if (!str) {
		return;
	}

	if ((style & UI_BLINK) && ((uis.realtime / BLINK_DIVISOR) & 1))
		return;

	if (style & UI_NS_STR_HGW) // height = width
	{
		charw = charh = fontsize;
	} else // height/2 = width
	{
		charw = fontsize / 2;
		charh = fontsize;
	}

	if (style & UI_PULSE) {
		lowlight[0] = 0.8 * color[0];
		lowlight[1] = 0.8 * color[1];
		lowlight[2] = 0.8 * color[2];
		lowlight[3] = 0.8 * color[3];
		UI_LerpColor(color, lowlight, newcolor, 0.5 + 0.5 * sin(uis.realtime / PULSE_DIVISOR));
		drawcolor = newcolor;
	} else
		drawcolor = color;

	switch (style & UI_FORMATMASK) {
	case UI_CENTER:
		// center justify at x
		len = strlen(str);
		x = x - len * charw / 2;
		break;

	case UI_RIGHT:
		// right justify at x
		len = strlen(str);
		x = x - len * charw;
		break;

	default:
		// left justify at x
		break;
	}

	if (style & UI_DROPSHADOW) {
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3];
		UI_DrawString2(x + 2, y + 2, str, dropcolor, charw, charh);
	}

	UI_DrawString2(x, y, str, drawcolor, charw, charh);
}

/*
=================
UI_DrawChar
=================
*/
void UI_DrawChar(int x, int y, int ch, int style, const vec4_t color) {
	char buff[2];

	buff[0] = ch;
	buff[1] = '\0';

	UI_DrawString(x, y, buff, style, color);
}

qboolean UI_IsFullscreen(void) {
	if (uis.activemenu && (trap_Key_GetCatcher() & KEYCATCH_UI)) {
		return uis.activemenu->fullscreen;
	}

	return qfalse;
}

#if 0
static void NeedCDAction( qboolean result ) {
	if ( !result ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	}
}

static void NeedCDKeyAction( qboolean result ) {
	if ( !result ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	}
}
#endif

void UI_SetActiveMenu(uiMenuCommand_t menu) {
	// this should be the ONLY way the menu system is brought up
	// ensure minimum menu data is cached
	Menu_Cache();

	switch (menu) {
	case UIMENU_NONE:
		UI_ForceMenuOff();
		return;
	case UIMENU_MAIN:
		UI_MainMenu();
		return;
	case UIMENU_INGAME:
		trap_Cvar_Set("cl_paused", "1");
		UI_InGameMenu();
		return;

	// bk001204
	case UIMENU_TEAM:
	case UIMENU_POSTGAME:
	default:
#ifndef NDEBUG
		Com_Printf("UI_SetActiveMenu: bad enum %d\n", menu);
#endif
		break;
	}
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent(int key, int down) {
	sfxHandle_t s;

	if (!uis.activemenu) {
		return;
	}

	if (!down) {
		return;
	}

	if (uis.activemenu->key)
		s = uis.activemenu->key(key);
	else
		s = Menu_DefaultKey(uis.activemenu, key);

	if ((s > 0) && (s != menu_null_sound))
		trap_S_StartLocalSound(s, CHAN_LOCAL_SOUND);
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent(int dx, int dy) {
	int i;
	menucommon_s *m;

	if (!uis.activemenu)
		return;

	// update mouse screen position
	uis.cursorx += dx;
	if (uis.cursorx < -uis.xbias)
		uis.cursorx = -uis.xbias;
	else if (uis.cursorx > SCREEN_WIDTH + uis.xbias)
		uis.cursorx = SCREEN_WIDTH + uis.xbias;

	// todo: use ybias here for odd screens (and symmetry)
	uis.cursory += dy;
	if (uis.cursory < 0)
		uis.cursory = 0;
	else if (uis.cursory > SCREEN_HEIGHT)
		uis.cursory = SCREEN_HEIGHT;

	if (uis.dropdownlist)
		return;

	// dragable slider ;)
	if (uis.activemenu->cursor >= 0 && trap_Key_IsDown(K_MOUSE1) &&
		uis.activemenu->items[uis.activemenu->cursor] != NULL &&
		((menucommon_s *)(uis.activemenu->items[uis.activemenu->cursor]))->type == MTYPE_SLIDER) {
		menuslider_s *s = uis.activemenu->items[uis.activemenu->cursor];
		const int x = uis.cursorx - s->generic.x - 2 * SMALLCHAR_WIDTH;
		const float oldvalue = s->curvalue;
		s->curvalue = (x / (float)(SLIDER_RANGE * SMALLCHAR_WIDTH)) * (s->maxvalue - s->minvalue) + s->minvalue;
		if (s->curvalue < s->minvalue)
			s->curvalue = s->minvalue;
		else if (s->curvalue > s->maxvalue)
			s->curvalue = s->maxvalue;
		if (s->curvalue != oldvalue && s->generic.callback)
			s->generic.callback(s, QM_ACTIVATED);
	}

	// region test the active menu items
	//	for (i=0; i<uis.activemenu->nitems; i++)
	// draw order == selection order ...
	for (i = uis.activemenu->nitems - 1; i >= 0; --i) {
		m = (menucommon_s *)uis.activemenu->items[i];

		if (m->flags & (QMF_GRAYED | QMF_INACTIVE))
			continue;

		if ((uis.cursorx < m->left) || (uis.cursorx > m->right) || (uis.cursory < m->top) ||
			(uis.cursory > m->bottom)) {
			// cursor out of item bounds
			continue;
		}

		// set focus to item at cursor
		if (uis.activemenu->cursor != i) {
			Menu_SetCursor(uis.activemenu, i);
			if (uis.activemenu->cursor_prev >= 0 && uis.activemenu->cursor_prev < uis.activemenu->nitems)
				((menucommon_s *)(uis.activemenu->items[uis.activemenu->cursor_prev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if (!(((menucommon_s *)(uis.activemenu->items[uis.activemenu->cursor]))->flags & QMF_SILENT)) {
				trap_S_StartLocalSound(menu_move_sound, CHAN_LOCAL_SOUND);
			}
		}

		((menucommon_s *)(uis.activemenu->items[uis.activemenu->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		return;
	}

	if (uis.activemenu->cursor >= 0 && uis.activemenu->cursor < uis.activemenu->nitems) {
		// out of any region
		((menucommon_s *)(uis.activemenu->items[uis.activemenu->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;
	}
}

const char *UI_Argv(int arg) {
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

const char *UI_Cvar_VariableString(const char *var_name) {
	static char buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer(var_name, buffer, sizeof(buffer));

	return buffer;
}

/*
=================
UI_Cache
=================
*/
void UI_Cache_f(void) {
	MainMenu_Cache();
	InGame_Cache();
	ConfirmMenu_Cache();
	PlayerSettings_Cache();
	Controls_Cache();
	Demos_Cache();
	Preferences_Cache();
	ServerInfo_Cache();
	SpecifyServer_Cache();
	ArenaServers_Cache();
	StartServer_Cache();

	GraphicsOptions_Cache();
	UI_DisplayOptionsMenu_Cache();
	UI_SoundOptionsMenu_Cache();
	UI_NetworkOptionsMenu_Cache();
	TeamMain_Cache();
	UI_AddBots_Cache();
	UI_RemoveBots_Cache();
	UI_SetupMenu_Cache();

	SetupDefaultMenu_Cache();
	UI_BigCredits_Cache();

	UI_BotSelectMenu_Cache();
	UI_ModsMenu_Cache();
}

/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand(int realTime) {
	const char *cmd;

	uis.frametime = realTime - uis.realtime;
	uis.realtime = realTime;

	cmd = UI_Argv(0);

	// ensure minimum menu data is available
	Menu_Cache();

	// this is still needed, but its ingame menu entry is disabled
	if (Q_stricmp(cmd, "ui_teamOrders") == 0) {
		UI_TeamOrdersMenu_f();
		return qtrue;
	}
	if (Q_stricmp(cmd, "wop_music") == 0) {
		MusicMenu_Open();
		return qtrue;
	}
	if (Q_stricmp(cmd, "wop_checkmusic") == 0) {
		Music_Check();
		return qtrue;
	}
	if (Q_stricmp(cmd, "wop_nextsong") == 0) {
		Music_NextTrack();
		return qtrue;
	}
	if (Q_stricmp(cmd, "wop_restartmusic") == 0) {
		Music_TriggerRestart();
		return qtrue;
	}

	if (Q_stricmp(cmd, "ui_help") == 0) {
		UI_HelpMenu_f();
		return qtrue;
	}
	return qfalse;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown(void) {
	UI_StopMusic();
	MusicMenu_Shutdown();
}

/*
=================
UI_Init
=================
*/
void UI_Init(void) {
	UI_RegisterCvars();

	// not yet
	//	WOP_LoadMenuText(UI_LOCALEFILE, qtrue);

	SetDefaultBinds_onUnusedKeys();

	if (s_wop_restarted.integer != qtrue) {
		trap_Cvar_Set("s_wop_restarted", "1");
		// erstmal rausgenommen fuer WoP-Engine
		//		trap_Cmd_ExecuteText( EXEC_NOW, "snd_restart\n" );//noch testen ob das nicht zu radikal ist, auch da
		//return! ... old: EXEC_APPEND 		return;
	}

	MusicMenu_Init();

	UI_InitGameinfo();

	// cache redundant calulations
	trap_GetGlconfig(&uis.glconfig);

	// for 640x480 virtualized screen
	uis.xscale = uis.glconfig.vidWidth * (1.0 / 640.0);
	uis.yscale = uis.glconfig.vidHeight * (1.0 / 480.0);

	if (uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640) {
		// wide screen
		uis.xbias = 0.5 * (uis.glconfig.vidWidth - (uis.glconfig.vidHeight * (640.0 / 480.0)));
		uis.xscale = uis.yscale;
	} else {
		// no wide screen
		uis.xbias = 0;
	}

	if (uis.glconfig.vidWidth * 480 < uis.glconfig.vidHeight * 640) {
		uis.ybias = 0.5f * (uis.glconfig.vidHeight - (uis.glconfig.vidWidth * (480.0f / 640.0f)));
		uis.yscale = uis.xscale;
	} else
		uis.ybias = 0;

	uis.scale1024 = uis.xscale * (640.0f / 1024.0f); // uis.glconfig.vidHeight * (1.0f/768.0f);

	// initialize the menu system
	Menu_Cache();

	uis.activemenu = NULL;
	uis.menusp = 0;
}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640(float *x, float *y, float *w, float *h) {
	// expect valid pointers
	*x = *x * uis.xscale + uis.xbias;
	*y = *y * uis.yscale + uis.ybias;
	*w *= uis.xscale;
	*h *= uis.yscale;
}

void UI_AdjustFrom1024(float *x, float *y, float *w, float *h) {
	// expect valid pointers
	*x = *x * uis.scale1024 + uis.xbias;
	*y = *y * uis.scale1024 + uis.ybias;
	*w *= uis.scale1024;
	*h *= uis.scale1024;
}

void UI_DrawNamedPic(float x, float y, float width, float height, const char *picname) {
	qhandle_t hShader;

	hShader = trap_R_RegisterShaderNoMip(picname);
	UI_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

void UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader) {
	float s0;
	float s1;
	float t0;
	float t1;

	if (w < 0) { // flip about vertical
		w = -w;
		s0 = 1;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = 1;
	}

	if (h < 0) { // flip about horizontal
		h = -h;
		t0 = 1;
		t1 = 0;
	} else {
		t0 = 0;
		t1 = 1;
	}

	UI_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

void UI_DrawHandlePic1024(float x, float y, float w, float h, qhandle_t hShader) {
	float s0;
	float s1;
	float t0;
	float t1;

	if (w < 0) { // flip about vertical
		w = -w;
		s0 = 1;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = 1;
	}

	if (h < 0) { // flip about horizontal
		h = -h;
		t0 = 1;
		t1 = 0;
	} else {
		t0 = 0;
		t1 = 1;
	}

	UI_AdjustFrom1024(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect(float x, float y, float width, float height, const float *color) {
	trap_R_SetColor(color);

	UI_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, uis.whiteShader);

	trap_R_SetColor(NULL);
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect(float x, float y, float width, float height, const float *color, int thickness) {
	trap_R_SetColor(color);

	UI_AdjustFrom640(&x, &y, &width, &height);

	trap_R_DrawStretchPic(x, y, width, thickness, 0, 0, 0, 0, uis.whiteShader);
	trap_R_DrawStretchPic(x, y, thickness, height, 0, 0, 0, 0, uis.whiteShader);
	trap_R_DrawStretchPic(x, y + height - thickness, width, thickness, 0, 0, 0, 0, uis.whiteShader);
	trap_R_DrawStretchPic(x + width - thickness, y, thickness, height, 0, 0, 0, 0, uis.whiteShader);

	trap_R_SetColor(NULL);
}

void UI_SetColor(const float *rgba) {
	trap_R_SetColor(rgba);
}

void UI_UpdateScreen(void) {
	trap_UpdateScreen();
}

#define WOP_INGAME_X 0
#define WOP_INGAME_Y 0
#define WOP_INGAME_B 640
#define WOP_INGAME_H 480

void UI_DrawIngameBG() {
	float wsCorrection = (uis.glconfig.vidWidth * 480.0f / (float)uis.glconfig.vidHeight) - 640.0f;
	if (wsCorrection < 0)
		wsCorrection = 0.0f;

	UI_DrawHandlePic(WOP_INGAME_X - (wsCorrection * 0.5f), WOP_INGAME_Y, WOP_INGAME_B + wsCorrection, WOP_INGAME_H,
					 uis.pad_ingamebg);
}

void UI_DrawMenu(menuframework_s *menu) {
	if (menu) {
		if (menu->bgparts & BGP_LASTMENU) {
			if (uis.menusp - 2 > 0) {
				UI_DrawMenu(uis.stack[uis.menusp - 2]);
			}
		} else if (menu->fullscreen) {
			// draw the background
			// schwarzer Hintergrund für nicht 4/3-Auflösungen
			trap_R_SetColor(colorBlack);
			trap_R_DrawStretchPic(0, 0, uis.glconfig.vidWidth, uis.glconfig.vidHeight, 0, 0, 0, 0, uis.whiteShader);
			trap_R_SetColor(NULL);

			if (menu->bgparts) {
				if (menu->bgparts & BGP_MAINBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_menushader);
				if (menu->bgparts & BGP_SIMPLEBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_simpleMenuBg);
				if (menu->bgparts & BGP_MAINFRAME)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_mainframe);
				if (menu->bgparts & BGP_SETUPBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_setupbg);
				if (menu->bgparts & BGP_SYSTEMBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_display);
				if (menu->bgparts & BGP_DEFAULTSBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_defaults);
				if (menu->bgparts & BGP_EXITBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_exitbg);
				if (menu->bgparts & BGP_CONTROLBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_controlbg);
				if (menu->bgparts & BGP_PLAYERBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_playerbg);
				if (menu->bgparts & BGP_SERVER2BG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_server2bg);
				if (menu->bgparts & BGP_MODSBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_modsbg);
				if (menu->bgparts & BGP_DEMOBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_demobg);
				if (menu->bgparts & BGP_SPECIFYBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_specifybg);
				if (menu->bgparts & BGP_SPECIFYPASSBG)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_specifypassbg);
				if (menu->bgparts & BGP_GAMEOPTIONS)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_gameoptionsbg);
				if (menu->bgparts & BGP_STARTSERVERMAPS)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_startservermaps);
				if (menu->bgparts & BGP_STARTSERVERBOTS)
					UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.pad_startserverbots);
			} else {
				UI_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, colorBlack);
			}
		}

		if (menu->draw)
			menu->draw();
		else
			Menu_Draw(menu);

		if (uis.firstdraw) {
			UI_MouseEvent(0, 0);
			uis.firstdraw = qfalse;
		}
	}
}

/*
=================
UI_Refresh
=================
*/
void UI_Refresh(int realtime) {
	uis.frametime = realtime - uis.realtime;
	uis.realtime = realtime;

	if (!(trap_Key_GetCatcher() & KEYCATCH_UI)) {
		return;
	}

	UI_UpdateCvars();

	if (!trap_Cvar_VariableValue("cl_paused")) {
		if (!uis.musicbool)
			UI_StartMusic();
	} else
		Music_Check();

	UI_DrawMenu(uis.activemenu);

	if (!uis.activemenu || !uis.activemenu->noMouseCursor) {
		// draw cursor
		UI_SetColor(NULL);
		UI_DrawHandlePic(uis.cursorx - 16, uis.cursory - 16, 32, 32, uis.cursor);
	}

	//	{ vec4_t	twhite = { 1,1,1,0.5f}; UI_DrawString(300,0,GAME_VERSION,UI_SMALLFONT,twhite); }

#ifndef NDEBUG
	if (uis.debug) {
		// cursor coordinates
		int y;
		char tmpstr[16];
		int len;

		y = trap_Key_GetCatcher() & KEYCATCH_CONSOLE ? 240 : 1;
		Com_sprintf(tmpstr, 16, "(%i,%i)", uis.cursorx, uis.cursory);
		len = strlen(tmpstr);
		UI_FillRect(1, y, 8 * len + 2, 18, colorWhite);
		UI_DrawRect(1, y, 8 * len + 2, 18, colorBlack, 1);
		UI_DrawString(2, y + 1, tmpstr, UI_LEFT | UI_SMALLFONT, colorBlack);
	}
#endif

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound) {
		m_entersound = qfalse;
	}
}

void UI_DrawTextBox(int x, int y, int width, int lines) {
	UI_FillRect(x + BIGCHAR_WIDTH / 2, y + BIGCHAR_HEIGHT / 2, (width + 1) * BIGCHAR_WIDTH,
				(lines + 1) * BIGCHAR_HEIGHT, colorBlack);
	UI_DrawRect(x + BIGCHAR_WIDTH / 2, y + BIGCHAR_HEIGHT / 2, (width + 1) * BIGCHAR_WIDTH,
				(lines + 1) * BIGCHAR_HEIGHT, colorWhite, 1);
}

qboolean UI_CursorInRect(int x, int y, int width, int height) {
	if (uis.cursorx < x || uis.cursory < y || uis.cursorx > x + width || uis.cursory > y + height)
		return qfalse;

	return qtrue;
}

/*
#######################
UI_ModelIcon
based on ServerPlayerIcon(ui_startserver.c)
#######################
*/
void UI_ModelIcon(const char *modelAndSkin, char *iconName, int SizeOfIconName) {
	char *skin;
	char model[MAX_QPATH];

	Q_strncpyz(model, modelAndSkin, sizeof(model));
	skin = strrchr(model, '/');
	if (skin) {
		*skin++ = '\0';
	} else {
		skin = "default";
	}

	Com_sprintf(iconName, SizeOfIconName, "models/wop_players/%s/icon_%s", model, skin);

	if (!trap_R_RegisterShaderNoMip(iconName) && Q_stricmp(skin, "default") != 0) {
		Com_sprintf(iconName, SizeOfIconName, "models/wop_players/%s/icon_default", model);
	}
}

/*
#######################
BotListCompare
used for qsort in startserver2- and addbots-menu
#######################
*/
// Ente requested to sort some main characters to the top ...
static int GetBotListSpecialScore(const char *name) {
	if (!Q_stricmp(name, "padman"))
		return 5;
	else if (!Q_stricmp(name, "padgirl"))
		return 4;
	else if (!Q_stricmp(name, "monsterpad"))
		return 3;
	else if (!Q_stricmp(name, "padlilly"))
		return 2;
	else if (!Q_stricmp(name, "fatpad"))
		return 1;
	else
		return 0;
}
int QDECL BotListCompare(const void *arg1, const void *arg2) {
	int num1, num2;
	const char *info1, *info2;
	const char *name1, *name2;

	num1 = *(int *)arg1;
	num2 = *(int *)arg2;

	info1 = UI_GetBotInfoByNumber(num1);
	info2 = UI_GetBotInfoByNumber(num2);

	name1 = Info_ValueForKey(info1, "name");
	name2 = Info_ValueForKey(info2, "name");

	{
		const int score1 = GetBotListSpecialScore(name1);
		const int score2 = GetBotListSpecialScore(name2);
		if (score1 || score2) {
			return score2 - score1;
		}
	}

	return Q_stricmp(name1, name2);
}
