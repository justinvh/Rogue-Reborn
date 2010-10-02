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
/*
=======================================================================

GAME OPTIONS MENU

=======================================================================
*/


#include <hat/gui/ui_local.h>


#define ART_FRAMEL				"menu/art/frame2_l"
#define ART_FRAMER				"menu/art/frame1_r"

#define PREFERENCES_X_POS		360


#define ID_SIMPLEITEMS			128
#define ID_HIGHQUALITYSKY		129
#define ID_EJECTINGBRASS		130
#define ID_WALLMARKS			131
//#define ID_SYNCEVERYFRAME		133
#define ID_FORCEMODEL			133
#define ID_FORCEBRIGHTSKINS		134
#define ID_DRAWTEAMOVERLAY		135
#define ID_ALLOWDOWNLOAD		136
#define ID_BACK					137


typedef struct
{
	menuframework_s menu;

	menutext_s      banner;
	menubitmap_s    framel;
	menubitmap_s    framer;


	menuradiobutton_s simpleitems;
	menuradiobutton_s brass;
	menuradiobutton_s wallmarks;
	menuradiobutton_s highqualitysky;
//	menuradiobutton_s synceveryframe;
	menuradiobutton_s forceModel;
	menuradiobutton_s forceBrightSkins;
	menulist_s      drawteamoverlay;
	menuradiobutton_s allowdownload;
	menubitmap_s    back;
} preferences_t;

static preferences_t s_preferences;

static const char *teamoverlay_names[] = {
	"off",
	"upper right",
	"lower right",
	"lower left",
	0
};

static void Preferences_SetMenuItems(void)
{
	s_preferences.simpleitems.curvalue = trap_Cvar_VariableValue("cg_simpleItems") != 0;
	s_preferences.brass.curvalue = trap_Cvar_VariableValue("cg_brassTime") != 0;
	s_preferences.wallmarks.curvalue = trap_Cvar_VariableValue("cg_marks") != 0;

	s_preferences.highqualitysky.curvalue = trap_Cvar_VariableValue("r_fastsky") == 0;
//	s_preferences.synceveryframe.curvalue = trap_Cvar_VariableValue("r_finish") != 0;
	s_preferences.forceModel.curvalue = trap_Cvar_VariableValue("cg_forceModel") != 0;
	s_preferences.forceBrightSkins.curvalue = trap_Cvar_VariableValue("cg_forceBrightSkins") != 0;
	s_preferences.drawteamoverlay.curvalue = Com_Clamp(0, 3, trap_Cvar_VariableValue("cg_drawTeamOverlay"));
	s_preferences.allowdownload.curvalue = trap_Cvar_VariableValue("cl_allowDownload") != 0;
}


static void Preferences_Event(void *ptr, int notification)
{
	if(notification != QM_ACTIVATED)
	{
		return;
	}

	switch (((menucommon_s *) ptr)->id)
	{
		case ID_SIMPLEITEMS:
			trap_Cvar_SetValue("cg_simpleItems", s_preferences.simpleitems.curvalue);
			break;

		case ID_HIGHQUALITYSKY:
			trap_Cvar_SetValue("r_fastsky", !s_preferences.highqualitysky.curvalue);
			break;

		case ID_EJECTINGBRASS:
			if(s_preferences.brass.curvalue)
				trap_Cvar_Reset("cg_brassTime");
			else
				trap_Cvar_SetValue("cg_brassTime", 0);
			break;

		case ID_WALLMARKS:
			trap_Cvar_SetValue("cg_marks", s_preferences.wallmarks.curvalue);
			break;
/*
		case ID_SYNCEVERYFRAME:
			trap_Cvar_SetValue("r_finish", s_preferences.synceveryframe.curvalue);
			break;
			*/

		case ID_FORCEMODEL:
			trap_Cvar_SetValue("cg_forceModel", s_preferences.forceModel.curvalue);
			break;

		case ID_FORCEBRIGHTSKINS:
			trap_Cvar_SetValue("cg_forceBrightSkins", s_preferences.forceBrightSkins.curvalue);
			break;

		case ID_DRAWTEAMOVERLAY:
			trap_Cvar_SetValue("cg_drawTeamOverlay", s_preferences.drawteamoverlay.curvalue);
			break;

		case ID_ALLOWDOWNLOAD:
			trap_Cvar_SetValue("cl_allowDownload", s_preferences.allowdownload.curvalue);
			trap_Cvar_SetValue("sv_allowDownload", s_preferences.allowdownload.curvalue);
			break;

		case ID_BACK:
			UI_PopMenu();
			break;
	}
}


/*
=================
Crosshair_Draw
=================
*/
/*static void Crosshair_Draw(void *self)
{
	menulist_s     *s;
	float          *color;
	int             x, y;
	int             style;
	qboolean        focus;

	s = (menulist_s *) self;
	x = s->generic.x;
	y = s->generic.y;

	style = UI_SMALLFONT;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if(s->generic.flags & QMF_GRAYED)
		color = text_color_disabled;
	else if(focus)
	{
		color = text_color_highlight;
		style |= UI_PULSE;
	}
	else if(s->generic.flags & QMF_BLINK)
	{
		color = text_color_highlight;
		style |= UI_BLINK;
	}
	else
		color = text_color_normal;

	if(focus)
	{
		// draw cursor
		UI_FillRect(s->generic.left, s->generic.top, s->generic.right - s->generic.left + 1,
					s->generic.bottom - s->generic.top + 1, listbar_color);
		UI_DrawChar(x, y, 13, UI_CENTER | UI_BLINK | UI_SMALLFONT, color);
	}

	UI_DrawString(x - SMALLCHAR_WIDTH, y, s->generic.name, style | UI_RIGHT, color);
	if(!s->curvalue)
	{
		return;
	}
	UI_DrawHandlePic(x + SMALLCHAR_WIDTH, y - 4, 24, 24, s_preferences.crosshairDotShader[s->curvalue-1]);

}*/


static void Preferences_MenuInit(void)
{
	int             y;

	memset(&s_preferences, 0, sizeof(preferences_t));

	Preferences_Cache();

	s_preferences.menu.wrapAround = qtrue;
	s_preferences.menu.fullscreen = qtrue;

	s_preferences.banner.generic.type = MTYPE_BTEXT;
	s_preferences.banner.generic.x = 320;
	s_preferences.banner.generic.y = 16;
	s_preferences.banner.string = "GAME OPTIONS";
	s_preferences.banner.color = color_white;
	s_preferences.banner.style = UI_CENTER | UI_DROPSHADOW;

/*	s_preferences.framel.generic.type = MTYPE_BITMAP;
	s_preferences.framel.generic.name = ART_FRAMEL;
	s_preferences.framel.generic.flags = QMF_INACTIVE;
	s_preferences.framel.generic.x = 0;
	s_preferences.framel.generic.y = 78;
	s_preferences.framel.width = 256;
	s_preferences.framel.height = 329;

	s_preferences.framer.generic.type = MTYPE_BITMAP;
	s_preferences.framer.generic.name = ART_FRAMER;
	s_preferences.framer.generic.flags = QMF_INACTIVE;
	s_preferences.framer.generic.x = 376;
	s_preferences.framer.generic.y = 76;
	s_preferences.framer.width = 256;
	s_preferences.framer.height = 334;
*/
	y = 144;
/*	s_preferences.crosshairDot.generic.type = MTYPE_TEXT;
	s_preferences.crosshairDot.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT | QMF_NODEFAULTINIT | QMF_OWNERDRAW;
	s_preferences.crosshairDot.generic.x = PREFERENCES_X_POS;
	s_preferences.crosshairDot.generic.y = y;
	s_preferences.crosshairDot.generic.name = "Crosshair Dot:";
	s_preferences.crosshairDot.generic.callback = Preferences_Event;
	s_preferences.crosshairDot.generic.ownerdraw = Crosshair_Draw;
	s_preferences.crosshairDot.generic.id = ID_CROSSHAIR_DOT;
	s_preferences.crosshairDot.generic.top = y - 4;
	s_preferences.crosshairDot.generic.bottom = y + 20;
	s_preferences.crosshairDot.generic.left =
		PREFERENCES_X_POS - ((strlen(s_preferences.crosshairDot.generic.name) + 1) * SMALLCHAR_WIDTH);
	s_preferences.crosshairDot.generic.right = PREFERENCES_X_POS + 48;
*/

	y += BIGCHAR_HEIGHT + 2 + 16;
	s_preferences.allowdownload.generic.type = MTYPE_RADIOBUTTON;
	s_preferences.allowdownload.generic.name = "Automatic Downloading:";
	s_preferences.allowdownload.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_preferences.allowdownload.generic.callback = Preferences_Event;
	s_preferences.allowdownload.generic.id = ID_ALLOWDOWNLOAD;
	s_preferences.allowdownload.generic.x = PREFERENCES_X_POS;
	s_preferences.allowdownload.generic.y = y;

	y += BIGCHAR_HEIGHT + 2;
	s_preferences.forceBrightSkins.generic.type = MTYPE_RADIOBUTTON;
	s_preferences.forceBrightSkins.generic.name = "Force Bright Skins:";
	s_preferences.forceBrightSkins.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_preferences.forceBrightSkins.generic.callback = Preferences_Event;
	s_preferences.forceBrightSkins.generic.id = ID_FORCEBRIGHTSKINS;
	s_preferences.forceBrightSkins.generic.x = PREFERENCES_X_POS;
	s_preferences.forceBrightSkins.generic.y = y;

	y += BIGCHAR_HEIGHT + 2;
	s_preferences.forceModel.generic.type = MTYPE_RADIOBUTTON;
	s_preferences.forceModel.generic.name = "Force Same Player Models:";
	s_preferences.forceModel.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_preferences.forceModel.generic.callback = Preferences_Event;
	s_preferences.forceModel.generic.id = ID_FORCEMODEL;
	s_preferences.forceModel.generic.x = PREFERENCES_X_POS;
	s_preferences.forceModel.generic.y = y;

	y += BIGCHAR_HEIGHT + 2;
	s_preferences.simpleitems.generic.type = MTYPE_RADIOBUTTON;
	s_preferences.simpleitems.generic.name = "Simple Items:";
	s_preferences.simpleitems.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_preferences.simpleitems.generic.callback = Preferences_Event;
	s_preferences.simpleitems.generic.id = ID_SIMPLEITEMS;
	s_preferences.simpleitems.generic.x = PREFERENCES_X_POS;
	s_preferences.simpleitems.generic.y = y;

	y += BIGCHAR_HEIGHT + 2;
	s_preferences.wallmarks.generic.type = MTYPE_RADIOBUTTON;
	s_preferences.wallmarks.generic.name = "Marks on Walls:";
	s_preferences.wallmarks.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_preferences.wallmarks.generic.callback = Preferences_Event;
	s_preferences.wallmarks.generic.id = ID_WALLMARKS;
	s_preferences.wallmarks.generic.x = PREFERENCES_X_POS;
	s_preferences.wallmarks.generic.y = y;

	y += BIGCHAR_HEIGHT + 2;
	s_preferences.brass.generic.type = MTYPE_RADIOBUTTON;
	s_preferences.brass.generic.name = "Ejecting Brass:";
	s_preferences.brass.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_preferences.brass.generic.callback = Preferences_Event;
	s_preferences.brass.generic.id = ID_EJECTINGBRASS;
	s_preferences.brass.generic.x = PREFERENCES_X_POS;
	s_preferences.brass.generic.y = y;

	y += BIGCHAR_HEIGHT + 2;
	s_preferences.highqualitysky.generic.type = MTYPE_RADIOBUTTON;
	s_preferences.highqualitysky.generic.name = "High Quality Sky:";
	s_preferences.highqualitysky.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_preferences.highqualitysky.generic.callback = Preferences_Event;
	s_preferences.highqualitysky.generic.id = ID_HIGHQUALITYSKY;
	s_preferences.highqualitysky.generic.x = PREFERENCES_X_POS;
	s_preferences.highqualitysky.generic.y = y;

#if 0
	y += BIGCHAR_HEIGHT + 2;
	s_preferences.synceveryframe.generic.type = MTYPE_RADIOBUTTON;
	s_preferences.synceveryframe.generic.name = "Sync Every Frame:";
	s_preferences.synceveryframe.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_preferences.synceveryframe.generic.callback = Preferences_Event;
	s_preferences.synceveryframe.generic.id = ID_SYNCEVERYFRAME;
	s_preferences.synceveryframe.generic.x = PREFERENCES_X_POS;
	s_preferences.synceveryframe.generic.y = y;
#endif

	y += BIGCHAR_HEIGHT + 2;
	s_preferences.drawteamoverlay.generic.type = MTYPE_SPINCONTROL;
	s_preferences.drawteamoverlay.generic.name = "Draw Team Overlay:";
	s_preferences.drawteamoverlay.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_preferences.drawteamoverlay.generic.callback = Preferences_Event;
	s_preferences.drawteamoverlay.generic.id = ID_DRAWTEAMOVERLAY;
	s_preferences.drawteamoverlay.generic.x = PREFERENCES_X_POS;
	s_preferences.drawteamoverlay.generic.y = y;
	s_preferences.drawteamoverlay.itemnames = teamoverlay_names;

	y += BIGCHAR_HEIGHT + 2;
	s_preferences.back.generic.type = MTYPE_BITMAP;
	s_preferences.back.generic.name = UI_ART_BUTTON;
	s_preferences.back.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_preferences.back.generic.callback = Preferences_Event;
	s_preferences.back.generic.id = ID_BACK;
	s_preferences.back.generic.x = 0;
	s_preferences.back.generic.y = 480 - 64;
	s_preferences.back.width = 128;
	s_preferences.back.height = 64;
	s_preferences.back.focuspic = UI_ART_BUTTON_FOCUS;
	s_preferences.back.generic.caption.text = "back";
	s_preferences.back.generic.caption.style = UI_CENTER | UI_DROPSHADOW;
	s_preferences.back.generic.caption.fontsize = 0.6f;
	s_preferences.back.generic.caption.font = &uis.buttonFont;
	s_preferences.back.generic.caption.color = text_color_normal;
	s_preferences.back.generic.caption.focuscolor = text_color_highlight;

	Menu_AddItem(&s_preferences.menu, &s_preferences.banner);
//  Menu_AddItem(&s_preferences.menu, &s_preferences.framel);
//  Menu_AddItem(&s_preferences.menu, &s_preferences.framer);

	Menu_AddItem(&s_preferences.menu, &s_preferences.allowdownload);
	Menu_AddItem(&s_preferences.menu, &s_preferences.forceBrightSkins);
	Menu_AddItem(&s_preferences.menu, &s_preferences.forceModel);
	Menu_AddItem(&s_preferences.menu, &s_preferences.simpleitems);
	Menu_AddItem(&s_preferences.menu, &s_preferences.wallmarks);
	Menu_AddItem(&s_preferences.menu, &s_preferences.brass);
//  Menu_AddItem(&s_preferences.menu, &s_preferences.identifytarget);
	Menu_AddItem(&s_preferences.menu, &s_preferences.highqualitysky);
//	Menu_AddItem(&s_preferences.menu, &s_preferences.synceveryframe);
	Menu_AddItem(&s_preferences.menu, &s_preferences.drawteamoverlay);

	Menu_AddItem(&s_preferences.menu, &s_preferences.back);

	Preferences_SetMenuItems();
}


/*
===============
Preferences_Cache
===============
*/
void Preferences_Cache(void)
{
	//trap_R_RegisterShaderNoMip(ART_FRAMEL);
	//trap_R_RegisterShaderNoMip(ART_FRAMER);

/*	for(n = 0; n < NUM_CROSSHAIRS; n++)
	{
		s_preferences.crosshairShader[n] = trap_R_RegisterShaderNoMip(va("gfx/2d/crosshair%c", 'a' + n));
	}
*/



}


/*
===============
UI_PreferencesMenu
===============
*/
void UI_PreferencesMenu(void)
{
	Preferences_MenuInit();
	UI_PushMenu(&s_preferences.menu);
}
