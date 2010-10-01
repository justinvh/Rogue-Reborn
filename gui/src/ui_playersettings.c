/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "ui_local.h"

#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"
#define ART_MODEL0			"menu/art/model_0"
#define ART_MODEL1			"menu/art/model_1"
#define ART_FX_BASE			"menu/art/fx_base"
#define ART_FX_BLUE			"menu/art/fx_blue"
#define ART_FX_CYAN			"menu/art/fx_cyan"
#define ART_FX_GREEN		"menu/art/fx_grn"
#define ART_FX_RED			"menu/art/fx_red"
#define ART_FX_TEAL			"menu/art/fx_teal"
#define ART_FX_WHITE		"menu/art/fx_white"
#define ART_FX_YELLOW		"menu/art/fx_yel"

#define ID_NAME			10
#define ID_HANDICAP		11
#define ID_EFFECTS		12
#define ID_BACK			13
#define ID_MODEL		14

#define MAX_NAMELENGTH	20

#define PLAYERSETTINGS_VERTICAL_SPACING 160
#define PLAYERSETTINGS_HORIZONTAL_SPACING 114

#define ID_CROSSHAIR_DOT			141
#define ID_CROSSHAIR_CIRCLE			142
#define ID_CROSSHAIR_CROSS			143
#define ID_CROSSHAIR_SIZE			144

#define ID_CROSSHAIR_TARGET			145
#define ID_CROSSHAIR_TINT			146
#define ID_CROSSHAIR_PULSE			147

#define ID_CROSSHAIR_RED			148
#define ID_CROSSHAIR_GREEN			149
#define ID_CROSSHAIR_BLUE			150
#define ID_CROSSHAIR_ALPHA			151


#define	NUM_CROSSHAIRS			10


int             numDot = 0;
int             numCircle = 0;
int             numCross = 0;


typedef struct
{
	menuframework_s menu;

	menutext_s      banner;
	menutext_s      title_player;
	menutext_s      title_crosshair;
	menutext_s      title_hud;

	//menubitmap_s    framel;
	//menubitmap_s    framer;
	menubitmap_s    player;

	menufield_s     name;
	menufield_s     clan;
	menulist_s      handicap;
	//menulist_s      color1;
	//menulist_s      color2;

	menubitmap_s    back;
	menubitmap_s    model;
	menubitmap_s    item_null;

	qhandle_t       fxBasePic;
	qhandle_t       fxPic[7];
	playerInfo_t    playerinfo;
	int             current_fx;
	char            playerModel[MAX_QPATH];

	menuslider_s    crosshairDot;
	menuslider_s    crosshairCircle;
	menuslider_s    crosshairCross;
	menuslider_s    crosshairSize;


	menuslider_s    hudRed;
	menuslider_s    hudGreen;
	menuslider_s    hudBlue;
	menuslider_s    hudAlpha;


	menuradiobutton_s crosshairTarget;
	menuradiobutton_s crosshairHealth;
	menuradiobutton_s crosshairPulse;

	qhandle_t       crosshairDotShader[NUM_CROSSHAIRS];
	qhandle_t       crosshairCircleShader[NUM_CROSSHAIRS];
	qhandle_t       crosshairCrossShader[NUM_CROSSHAIRS];


} playersettings_t;

static playersettings_t s_playersettings;

//static int      gamecodetoui[] = { 4, 2, 3, 0, 5, 1, 6 };
//static int      uitogamecode[] = { 4, 6, 2, 3, 1, 5, 7 };

/*static const char *handicap_items[] = {
	"None",
	"95",
	"90",
	"85",
	"80",
	"75",
	"70",
	"65",
	"60",
	"55",
	"50",
	"45",
	"40",
	"35",
	"30",
	"25",
	"20",
	"15",
	"10",
	"5",
	0
};*/


/*
=================
PlayerSettings_DrawEffects
=================
*/
/*static void PlayerSettings_DrawEffects(void *self)
{
	menulist_s     *item;
	qboolean        focus;
	int             style;
	float          *color;

	item = (menulist_s *) self;
	focus = (item->generic.parent->cursor == item->generic.menuPosition);

	style = UI_LEFT | UI_SMALLFONT;
	color = text_color_normal;
	if(focus)
	{
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString(item->generic.x, item->generic.y, "Effects", style, color);

	UI_DrawHandlePic(item->generic.x + 64, item->generic.y + PROP_HEIGHT + 8, 128, 8, s_playersettings.fxBasePic);
	UI_DrawHandlePic(item->generic.x + 64 + item->curvalue * 16 + 8, item->generic.y + PROP_HEIGHT + 6, 16, 12,
					 s_playersettings.fxPic[item->curvalue]);
}*/

/*
=================
PlayerSettings_DrawSecondaryEffects
=================
*/
/*static void PlayerSettings_DrawSecondaryEffects(void *self)
{
	menulist_s     *item;

	item = (menulist_s *) self;

	UI_DrawHandlePic(item->generic.x + 64, item->generic.y + PROP_HEIGHT + 8, 128, 8, s_playersettings.fxBasePic);
	UI_DrawHandlePic(item->generic.x + 64 + item->curvalue * 16 + 8, item->generic.y + PROP_HEIGHT + 6, 16, 12,
					 s_playersettings.fxPic[item->curvalue]);
}*/

/*
=================
PlayerSettings_DrawPlayer
=================
*/
static void PlayerSettings_DrawPlayer(void *self)
{
	menubitmap_s   *b;
	char            buf[MAX_QPATH];
	int             x, y, value, size;
	vec4_t          color;

	// draw the model
	trap_Cvar_VariableStringBuffer("model", buf, sizeof(buf));
	if(strcmp(buf, s_playersettings.playerModel) != 0)
	{
		UI_PlayerInfo_SetModel(&s_playersettings.playerinfo, buf);
		strcpy(s_playersettings.playerModel, buf);
	}

	b = (menubitmap_s *) self;

	UI_DrawPlayer(b->generic.x, b->generic.y, b->width, b->height, &s_playersettings.playerinfo, uis.realtime / 2);

	// draw the crosshair
	x = 420;
	y = 230;
	size = (int)s_playersettings.crosshairSize.curvalue;

	value = (int)s_playersettings.crosshairDot.curvalue;
	if(value)
	{
		UI_DrawHandlePic(x - size / 2, y - size / 2, size, size, s_playersettings.crosshairDotShader[value - 1]);

	}
	value = (int)s_playersettings.crosshairCross.curvalue;
	if(value)
	{
		UI_DrawHandlePic(x - size / 2, y - size / 2, size, size, s_playersettings.crosshairCrossShader[value - 1]);

	}
	value = (int)s_playersettings.crosshairCircle.curvalue;
	if(value)
	{
		UI_DrawHandlePic(x - size / 2, y - size / 2, size, size, s_playersettings.crosshairCircleShader[value - 1]);

	}

	// TODO: draw hud color preview
	color[0] = s_playersettings.hudRed.curvalue / 10.0f;
	color[1] = s_playersettings.hudGreen.curvalue / 10.0f;
	color[2] = s_playersettings.hudBlue.curvalue / 10.0f;
	color[3] = s_playersettings.hudAlpha.curvalue / 10.0f;

	x = 420;
	y = 340;

	trap_R_SetColor(color);
	UI_DrawHandlePic(x - 18, y - 18, 36, 36, trap_R_RegisterShaderNoMip("hud/hud_icon_health"));
	trap_R_SetColor(NULL);
}

/*
=================
PlayerSettings_SaveChanges
=================
*/
static void PlayerSettings_SaveChanges(void)
{
	// name
	trap_Cvar_Set("name", s_playersettings.name.field.buffer);
	// clan
	trap_Cvar_Set("clan", s_playersettings.clan.field.buffer);

	// handicap
//  trap_Cvar_SetValue("handicap", 100 - s_playersettings.handicap.curvalue * 5);


	trap_Cvar_SetValue("cg_crosshairDot", s_playersettings.crosshairDot.curvalue);
	trap_Cvar_SetValue("cg_crosshairCross", s_playersettings.crosshairCross.curvalue);
	trap_Cvar_SetValue("cg_crosshairCircle", s_playersettings.crosshairCircle.curvalue);
	trap_Cvar_SetValue("cg_crosshairSize", s_playersettings.crosshairSize.curvalue);


	trap_Cvar_SetValue("cg_crosshairNames", s_playersettings.crosshairTarget.curvalue);
	trap_Cvar_SetValue("cg_crosshairHealth", s_playersettings.crosshairHealth.curvalue);
	trap_Cvar_SetValue("cg_crosshairPulse", s_playersettings.crosshairPulse.curvalue);


	trap_Cvar_SetValue("cg_hudRed", s_playersettings.hudRed.curvalue / 10.0f);
	trap_Cvar_SetValue("cg_hudGreen", s_playersettings.hudGreen.curvalue / 10.0f);
	trap_Cvar_SetValue("cg_hudBlue", s_playersettings.hudBlue.curvalue / 10.0f);
	trap_Cvar_SetValue("cg_hudAlpha", s_playersettings.hudAlpha.curvalue / 10.0f);



	// effects color
//  trap_Cvar_SetValue("color1", uitogamecode[s_playersettings.color1.curvalue]);

	// secondary effects color
//  trap_Cvar_SetValue("color2", uitogamecode[s_playersettings.color2.curvalue]);
}

/*
=================
PlayerSettings_MenuKey
=================
*/
static sfxHandle_t PlayerSettings_MenuKey(int key)
{
	if(key == K_MOUSE2 || key == K_ESCAPE)
	{
		PlayerSettings_SaveChanges();
	}
	return Menu_DefaultKey(&s_playersettings.menu, key);
}

/*
=================
PlayerSettings_SetMenuItems
=================
*/
static void PlayerSettings_SetMenuItems(void)
{
	vec3_t          viewangles;
	int             h;

	// name
	Q_strncpyz(s_playersettings.name.field.buffer, UI_Cvar_VariableString("name"), sizeof(s_playersettings.name.field.buffer));

	// clan
	Q_strncpyz(s_playersettings.clan.field.buffer, UI_Cvar_VariableString("clan"), sizeof(s_playersettings.clan.field.buffer));

/*	// effects color
	c = trap_Cvar_VariableValue("color1") - 1;
	if(c < 0 || c > 6)
		c = 6;
	s_playersettings.color1.curvalue = gamecodetoui[c];

	// secondary effects color
	c = trap_Cvar_VariableValue("color2") - 1;
	if(c < 0 || c > 6)
		c = 6;
	s_playersettings.color2.curvalue = gamecodetoui[c];
*/


	s_playersettings.crosshairDot.curvalue = (int)trap_Cvar_VariableValue("cg_crosshairDot");
	s_playersettings.crosshairCross.curvalue = (int)trap_Cvar_VariableValue("cg_crosshairCross");
	s_playersettings.crosshairCircle.curvalue = (int)trap_Cvar_VariableValue("cg_crosshairCircle");
	s_playersettings.crosshairSize.curvalue = (int)trap_Cvar_VariableValue("cg_crosshairSize");

	s_playersettings.crosshairTarget.curvalue = trap_Cvar_VariableValue("cg_crosshairNames") != 0;
	s_playersettings.crosshairHealth.curvalue = trap_Cvar_VariableValue("cg_crosshairHealth") != 0;
	s_playersettings.crosshairPulse.curvalue = trap_Cvar_VariableValue("cg_crosshairPulse") != 0;

	s_playersettings.hudRed.curvalue = trap_Cvar_VariableValue("cg_hudRed") * 10.0f;
	s_playersettings.hudGreen.curvalue = trap_Cvar_VariableValue("cg_hudGreen") * 10.0f;
	s_playersettings.hudBlue.curvalue = trap_Cvar_VariableValue("cg_hudBlue") * 10.0f;
	s_playersettings.hudAlpha.curvalue = trap_Cvar_VariableValue("cg_hudAlpha") * 10.0f;



	// model/skin
	memset(&s_playersettings.playerinfo, 0, sizeof(playerInfo_t));

	viewangles[YAW] = 180 - 30;
	viewangles[PITCH] = 0;
	viewangles[ROLL] = 0;

	UI_PlayerInfo_SetModel(&s_playersettings.playerinfo, UI_Cvar_VariableString("model"));

	// handicap
	h = Com_Clamp(5, 100, trap_Cvar_VariableValue("handicap"));
	s_playersettings.handicap.curvalue = 20 - h / 5;
}

/*
=================
PlayerSettings_MenuEvent
=================
*/
static void PlayerSettings_MenuEvent(void *ptr, int event)
{
	if(event != QM_ACTIVATED)
	{
		return;
	}

	switch (((menucommon_s *) ptr)->id)
	{
			//FIXME: only one dot ?
		case ID_CROSSHAIR_DOT:
			break;
		case ID_CROSSHAIR_CROSS:
			break;
		case ID_CROSSHAIR_CIRCLE:
			break;
		case ID_CROSSHAIR_SIZE:
			break;
		case ID_CROSSHAIR_TARGET:
			break;
		case ID_CROSSHAIR_TINT:
			break;
		case ID_CROSSHAIR_PULSE:
			break;
		case ID_HANDICAP:
			trap_Cvar_Set("handicap", va("%i", 100 - 25 * s_playersettings.handicap.curvalue));
			break;

		case ID_MODEL:
			PlayerSettings_SaveChanges();
			UI_PlayerModelMenu();
			break;

		case ID_BACK:
			PlayerSettings_SaveChanges();
			UI_PopMenu();
			break;
	}
}

/*
=================
PlayerSettings_MenuInit
=================
*/
static void PlayerSettings_MenuInit(void)
{
	int             x, y;

	memset(&s_playersettings, 0, sizeof(playersettings_t));

	PlayerSettings_Cache();

	s_playersettings.menu.key = PlayerSettings_MenuKey;
	s_playersettings.menu.wrapAround = qtrue;
	s_playersettings.menu.fullscreen = qtrue;

	s_playersettings.banner.generic.type = MTYPE_BTEXT;
	s_playersettings.banner.generic.x = 320;
	s_playersettings.banner.generic.y = 16;
	s_playersettings.banner.string = "PLAYER SETTINGS";
	s_playersettings.banner.color = color_white;
	s_playersettings.banner.style = UI_CENTER | UI_DROPSHADOW;




	y = PLAYERSETTINGS_HORIZONTAL_SPACING;
	s_playersettings.title_player.generic.type = MTYPE_TEXT;
	s_playersettings.title_player.generic.x = PLAYERSETTINGS_VERTICAL_SPACING - 120;
	s_playersettings.title_player.generic.y = y;
	s_playersettings.title_player.string = "Player:";
	s_playersettings.title_player.color = color_white;
	s_playersettings.title_player.style = UI_LEFT | UI_BOLD | UI_DROPSHADOW;
	y += BIGCHAR_HEIGHT + 8;

	s_playersettings.name.generic.type = MTYPE_FIELD;
	s_playersettings.name.generic.name = "Name:";
	s_playersettings.name.generic.flags = QMF_PULSEIFFOCUS;
	s_playersettings.name.field.widthInChars = MAX_NAMELENGTH;
	s_playersettings.name.field.maxchars = MAX_NAMELENGTH;
	s_playersettings.name.generic.x = PLAYERSETTINGS_VERTICAL_SPACING;
	s_playersettings.name.generic.y = y;
	y += BIGCHAR_HEIGHT + 2;

	s_playersettings.clan.generic.type = MTYPE_FIELD;
	s_playersettings.clan.generic.name = "Clan:";
	s_playersettings.clan.generic.flags = QMF_PULSEIFFOCUS;
	s_playersettings.clan.field.widthInChars = MAX_NAMELENGTH;
	s_playersettings.clan.field.maxchars = MAX_NAMELENGTH;
	s_playersettings.clan.generic.x = PLAYERSETTINGS_VERTICAL_SPACING;
	s_playersettings.clan.generic.y = y;
	y += BIGCHAR_HEIGHT + 2;

//otty: do we need handycap in xreal ?
/*
	s_playersettings.handicap.generic.type = MTYPE_SPINCONTROL;
	s_playersettings.handicap.generic.name = "Handycap:";
	s_playersettings.handicap.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.handicap.generic.id = ID_HANDICAP;
	s_playersettings.handicap.itemnames = handicap_items;
	s_playersettings.handicap.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.handicap.generic.x = PLAYERSETTINGS_VERTICAL_SPACING;
	s_playersettings.handicap.generic.y = y;
	s_playersettings.handicap.numitems = 20;
	y += BIGCHAR_HEIGHT + 8;
*/


	y = PLAYERSETTINGS_HORIZONTAL_SPACING;
	x = 380;

	s_playersettings.title_crosshair.generic.type = MTYPE_TEXT;
	s_playersettings.title_crosshair.generic.x = x;
	s_playersettings.title_crosshair.generic.y = y;
	s_playersettings.title_crosshair.string = "Crosshair:";
	s_playersettings.title_crosshair.color = color_white;
	s_playersettings.title_crosshair.style = UI_LEFT | UI_BOLD | UI_DROPSHADOW;
	y += BIGCHAR_HEIGHT + 8;


	x += 140;
	s_playersettings.crosshairTarget.generic.type = MTYPE_RADIOBUTTON;
	s_playersettings.crosshairTarget.generic.name = "Show Target:";
	s_playersettings.crosshairTarget.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.crosshairTarget.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.crosshairTarget.generic.id = ID_CROSSHAIR_TARGET;
	s_playersettings.crosshairTarget.generic.x = x;
	s_playersettings.crosshairTarget.generic.y = y;
	y += BIGCHAR_HEIGHT + 2;

	s_playersettings.crosshairHealth.generic.type = MTYPE_RADIOBUTTON;
	s_playersettings.crosshairHealth.generic.name = "Tint on health:";
	s_playersettings.crosshairHealth.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.crosshairHealth.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.crosshairHealth.generic.id = ID_CROSSHAIR_TINT;
	s_playersettings.crosshairHealth.generic.x = x;
	s_playersettings.crosshairHealth.generic.y = y;
	y += BIGCHAR_HEIGHT + 2;

	s_playersettings.crosshairPulse.generic.type = MTYPE_RADIOBUTTON;
	s_playersettings.crosshairPulse.generic.name = "Pulse on pickup:";
	s_playersettings.crosshairPulse.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.crosshairPulse.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.crosshairPulse.generic.id = ID_CROSSHAIR_PULSE;
	s_playersettings.crosshairPulse.generic.x = x;
	s_playersettings.crosshairPulse.generic.y = y;
	y += BIGCHAR_HEIGHT + 6;


	x -= 10;

	s_playersettings.crosshairDot.generic.type = MTYPE_SLIDER;
	s_playersettings.crosshairDot.generic.name = "Dot:";
	s_playersettings.crosshairDot.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.crosshairDot.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.crosshairDot.generic.id = ID_CROSSHAIR_DOT;
	s_playersettings.crosshairDot.generic.x = x;
	s_playersettings.crosshairDot.generic.y = y;
	s_playersettings.crosshairDot.minvalue = 0;
	s_playersettings.crosshairDot.maxvalue = numDot;
	s_playersettings.crosshairDot.integer = qtrue;
	y += BIGCHAR_HEIGHT;
	s_playersettings.crosshairCircle.generic.type = MTYPE_SLIDER;
	s_playersettings.crosshairCircle.generic.name = "Circle:";
	s_playersettings.crosshairCircle.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.crosshairCircle.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.crosshairCircle.generic.id = ID_CROSSHAIR_CIRCLE;
	s_playersettings.crosshairCircle.generic.x = x;
	s_playersettings.crosshairCircle.generic.y = y;
	s_playersettings.crosshairCircle.minvalue = 0;
	s_playersettings.crosshairCircle.maxvalue = numCircle;
	s_playersettings.crosshairCircle.integer = qtrue;
	y += BIGCHAR_HEIGHT;
	s_playersettings.crosshairCross.generic.type = MTYPE_SLIDER;
	s_playersettings.crosshairCross.generic.name = "Cross:";
	s_playersettings.crosshairCross.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.crosshairCross.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.crosshairCross.generic.id = ID_CROSSHAIR_CROSS;
	s_playersettings.crosshairCross.generic.x = x;
	s_playersettings.crosshairCross.generic.y = y;
	s_playersettings.crosshairCross.minvalue = 0;
	s_playersettings.crosshairCross.maxvalue = numCross;
	s_playersettings.crosshairCross.integer = qtrue;
	y += BIGCHAR_HEIGHT;
	s_playersettings.crosshairSize.generic.type = MTYPE_SLIDER;
	s_playersettings.crosshairSize.generic.name = "Size:";
	s_playersettings.crosshairSize.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.crosshairSize.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.crosshairSize.generic.id = ID_CROSSHAIR_SIZE;
	s_playersettings.crosshairSize.generic.x = x;
	s_playersettings.crosshairSize.generic.y = y;
	s_playersettings.crosshairSize.minvalue = 20;
	s_playersettings.crosshairSize.maxvalue = 52;
	s_playersettings.crosshairSize.integer = qtrue;
	s_playersettings.crosshairSize.step = 2;

	y += BIGCHAR_HEIGHT + 16;


	x = 380;
	s_playersettings.title_hud.generic.type = MTYPE_TEXT;
	s_playersettings.title_hud.generic.x = x;
	s_playersettings.title_hud.generic.y = y;
	s_playersettings.title_hud.string = "Hud:";
	s_playersettings.title_hud.color = color_white;
	s_playersettings.title_hud.style = UI_LEFT | UI_BOLD | UI_DROPSHADOW;
	y += BIGCHAR_HEIGHT + 2;

	x += 140;
	x -= 10;

	y += BIGCHAR_HEIGHT;
	s_playersettings.hudRed.generic.type = MTYPE_SLIDER;
	s_playersettings.hudRed.generic.name = "Red:";
	s_playersettings.hudRed.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.hudRed.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.hudRed.generic.id = ID_CROSSHAIR_RED;
	s_playersettings.hudRed.generic.x = x;
	s_playersettings.hudRed.generic.y = y;
	s_playersettings.hudRed.minvalue = 0;
	s_playersettings.hudRed.maxvalue = 10;
	s_playersettings.hudRed.integer = qtrue;
	s_playersettings.hudRed.step = 1;

	y += BIGCHAR_HEIGHT;
	s_playersettings.hudGreen.generic.type = MTYPE_SLIDER;
	s_playersettings.hudGreen.generic.name = "Green:";
	s_playersettings.hudGreen.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.hudGreen.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.hudGreen.generic.id = ID_CROSSHAIR_GREEN;
	s_playersettings.hudGreen.generic.x = x;
	s_playersettings.hudGreen.generic.y = y;
	s_playersettings.hudGreen.minvalue = 0;
	s_playersettings.hudGreen.maxvalue = 10;
	s_playersettings.hudGreen.integer = qtrue;
	s_playersettings.hudGreen.step = 1;

	y += BIGCHAR_HEIGHT;
	s_playersettings.hudBlue.generic.type = MTYPE_SLIDER;
	s_playersettings.hudBlue.generic.name = "Blue:";
	s_playersettings.hudBlue.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.hudBlue.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.hudBlue.generic.id = ID_CROSSHAIR_BLUE;
	s_playersettings.hudBlue.generic.x = x;
	s_playersettings.hudBlue.generic.y = y;
	s_playersettings.hudBlue.minvalue = 0;
	s_playersettings.hudBlue.maxvalue = 10;
	s_playersettings.hudBlue.integer = qtrue;
	s_playersettings.hudBlue.step = 1;


	y += BIGCHAR_HEIGHT;
	s_playersettings.hudAlpha.generic.type = MTYPE_SLIDER;
	s_playersettings.hudAlpha.generic.name = "Alpha:";
	s_playersettings.hudAlpha.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_playersettings.hudAlpha.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.hudAlpha.generic.id = ID_CROSSHAIR_ALPHA;
	s_playersettings.hudAlpha.generic.x = x;
	s_playersettings.hudAlpha.generic.y = y;
	s_playersettings.hudAlpha.minvalue = 0;
	s_playersettings.hudAlpha.maxvalue = 10;
	s_playersettings.hudAlpha.integer = qtrue;
	s_playersettings.hudAlpha.step = 1;






/*
	y += 3 * PROP_HEIGHT;
	s_playersettings.color1.generic.type = MTYPE_SPINCONTROL;
	s_playersettings.color1.generic.flags = QMF_NODEFAULTINIT;
	s_playersettings.color1.generic.id = ID_EFFECTS;
	s_playersettings.color1.generic.ownerdraw = PlayerSettings_DrawEffects;
	s_playersettings.color1.generic.x = 192;
	s_playersettings.color1.generic.y = y;
	s_playersettings.color1.generic.left = 192 - 8;
	s_playersettings.color1.generic.top = y - 8;
	s_playersettings.color1.generic.right = 192 + 200;
	s_playersettings.color1.generic.bottom = y + 2 * PROP_HEIGHT;
	s_playersettings.color1.numitems = 7;

	y += PROP_HEIGHT;
	s_playersettings.color2.generic.type = MTYPE_SPINCONTROL;
	s_playersettings.color2.generic.flags = QMF_NODEFAULTINIT;
	s_playersettings.color2.generic.ownerdraw = PlayerSettings_DrawSecondaryEffects;
	s_playersettings.color2.generic.x = 192;
	s_playersettings.color2.generic.y = y;
	s_playersettings.color2.generic.left = 192 - 8;
	s_playersettings.color2.generic.top = y - 8;
	s_playersettings.color2.generic.right = 192 + 200;
	s_playersettings.color2.generic.bottom = y + 2 * PROP_HEIGHT;
	s_playersettings.color2.numitems = 7;
*/
	s_playersettings.model.generic.type = MTYPE_BITMAP;
	s_playersettings.model.generic.name = UI_ART_BUTTON;
	s_playersettings.model.generic.flags = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_playersettings.model.generic.id = ID_MODEL;
	s_playersettings.model.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.model.generic.x = 640;
	s_playersettings.model.generic.y = 480 - 64;
	s_playersettings.model.width = 128;
	s_playersettings.model.height = 64;
	s_playersettings.model.focuspic = UI_ART_BUTTON_FOCUS;
	s_playersettings.model.generic.caption.text = "model";
	s_playersettings.model.generic.caption.style = UI_CENTER | UI_DROPSHADOW;
	s_playersettings.model.generic.caption.fontsize = 0.6f;
	s_playersettings.model.generic.caption.font = &uis.buttonFont;
	s_playersettings.model.generic.caption.color = text_color_normal;
	s_playersettings.model.generic.caption.focuscolor = text_color_highlight;

	s_playersettings.player.generic.type = MTYPE_BITMAP;
	s_playersettings.player.generic.flags = QMF_INACTIVE;
	s_playersettings.player.generic.ownerdraw = PlayerSettings_DrawPlayer;
	s_playersettings.player.generic.x = 40;
	s_playersettings.player.generic.y = 130;
	s_playersettings.player.width = 30 * 9;
	s_playersettings.player.height = 50 * 7;

	s_playersettings.back.generic.type = MTYPE_BITMAP;
	s_playersettings.back.generic.name = UI_ART_BUTTON;
	s_playersettings.back.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_playersettings.back.generic.id = ID_BACK;
	s_playersettings.back.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.back.generic.x = 0;
	s_playersettings.back.generic.y = 480 - 64;
	s_playersettings.back.width = 128;
	s_playersettings.back.height = 64;
	s_playersettings.back.focuspic = UI_ART_BUTTON_FOCUS;
	s_playersettings.back.generic.caption.text = "back";
	s_playersettings.back.generic.caption.style = UI_CENTER | UI_DROPSHADOW;
	s_playersettings.back.generic.caption.fontsize = 0.6f;
	s_playersettings.back.generic.caption.font = &uis.buttonFont;
	s_playersettings.back.generic.caption.color = text_color_normal;
	s_playersettings.back.generic.caption.focuscolor = text_color_highlight;


	s_playersettings.item_null.generic.type = MTYPE_BITMAP;
	s_playersettings.item_null.generic.flags = QMF_LEFT_JUSTIFY | QMF_MOUSEONLY | QMF_SILENT;
	s_playersettings.item_null.generic.x = 0;
	s_playersettings.item_null.generic.y = 0;
	s_playersettings.item_null.width = 640;
	s_playersettings.item_null.height = 480;

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.player);

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.banner);
	//Menu_AddItem(&s_playersettings.menu, &s_playersettings.framel);
	//Menu_AddItem(&s_playersettings.menu, &s_playersettings.framer);

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.title_player);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.title_crosshair);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.title_hud);


	Menu_AddItem(&s_playersettings.menu, &s_playersettings.name);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.clan);
	//Menu_AddItem(&s_playersettings.menu, &s_playersettings.handicap);
	//Menu_AddItem(&s_playersettings.menu, &s_playersettings.color1);
	//Menu_AddItem(&s_playersettings.menu, &s_playersettings.color2);

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.crosshairDot);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.crosshairCircle);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.crosshairCross);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.crosshairSize);

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.crosshairTarget);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.crosshairHealth);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.crosshairPulse);


	Menu_AddItem(&s_playersettings.menu, &s_playersettings.hudRed);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.hudGreen);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.hudBlue);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.hudAlpha);



	Menu_AddItem(&s_playersettings.menu, &s_playersettings.model);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.back);


	Menu_AddItem(&s_playersettings.menu, &s_playersettings.item_null);

	PlayerSettings_SetMenuItems();
}

/*
=================
PlayerSettings_Cache
=================
*/
void PlayerSettings_Cache(void)
{

	int             i;

	trap_R_RegisterShaderNoMip(ART_MODEL0);
	trap_R_RegisterShaderNoMip(ART_MODEL1);

	s_playersettings.fxBasePic = trap_R_RegisterShaderNoMip(ART_FX_BASE);
	s_playersettings.fxPic[0] = trap_R_RegisterShaderNoMip(ART_FX_RED);
	s_playersettings.fxPic[1] = trap_R_RegisterShaderNoMip(ART_FX_YELLOW);
	s_playersettings.fxPic[2] = trap_R_RegisterShaderNoMip(ART_FX_GREEN);
	s_playersettings.fxPic[3] = trap_R_RegisterShaderNoMip(ART_FX_TEAL);
	s_playersettings.fxPic[4] = trap_R_RegisterShaderNoMip(ART_FX_BLUE);
	s_playersettings.fxPic[5] = trap_R_RegisterShaderNoMip(ART_FX_CYAN);
	s_playersettings.fxPic[6] = trap_R_RegisterShaderNoMip(ART_FX_WHITE);

	numDot = 0;
	numCircle = 0;
	numCross = 0;


	for(i = 0; i < NUM_CROSSHAIRS; i++)
	{

		s_playersettings.crosshairDotShader[i] = trap_R_RegisterShaderNoMip(va("hud/crosshairs/dot%i", i + 1));
		if(s_playersettings.crosshairDotShader[i])
			numDot++;

		s_playersettings.crosshairCircleShader[i] = trap_R_RegisterShaderNoMip(va("hud/crosshairs/circle%i", i + 1));
		if(s_playersettings.crosshairCircleShader[i])
			numCircle++;

		s_playersettings.crosshairCrossShader[i] = trap_R_RegisterShaderNoMip(va("hud/crosshairs/cross%i", i + 1));
		if(s_playersettings.crosshairCrossShader[i])
			numCross++;

	}

}

/*
=================
UI_PlayerSettingsMenu
=================
*/
void UI_PlayerSettingsMenu(void)
{
	PlayerSettings_MenuInit();
	UI_PushMenu(&s_playersettings.menu);
}
