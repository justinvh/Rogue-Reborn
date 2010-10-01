/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2008 Pat Raynor <raynorpat@gmail.com>

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

/*
=======================================================================

CREDITS

=======================================================================
*/

#include "ui_local.h"

#define SCROLLSPEED	3

typedef struct
{
	menuframework_s menu;
} creditsmenu_t;

static creditsmenu_t s_credits;

int             starttime;		// game time at which credits are started
float           mvolume;		// records the original music volume level

//qhandle_t       BackgroundShader;

typedef struct
{
	char           *string;
	int             style;
	vec_t          *color;

} cr_line;

cr_line         credits[] = {
	{"XreaL", UI_CENTER | UI_GIANTFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	{"Project Lead", UI_CENTER | UI_BIGFONT, colorMdGrey},
	{"Robert 'Tr3B' Beckebans", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	{"Programming", UI_CENTER | UI_BIGFONT, colorMdGrey},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	//{"XreaL Team", UI_CENTER | UI_BIGFONT, colorLtGrey},
	{"Robert 'Tr3B' Beckebans", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Pat 'raynorpat' Raynor", UI_CENTER | UI_SMALLFONT, colorWhite},
//  {"Josef 'cnuke' Soentgen", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Adrian 'otty' Fuhrmann", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorBlue},

	{"IOQuake 3 - www.ioquake3.org", UI_CENTER | UI_BIGFONT, colorLtGrey},
	{"Tim 'Timbo' Angus", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Vincent Cojot", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Ryan C. 'icculus' Gordon", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Aaron Gyes", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Ludwig Nussel", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Julian Priestley", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Scirocco Six", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Zachary J. Slater", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Tony J. White", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

//	{"Unlagged - www.ra.is/unlagged/", UI_CENTER | UI_BIGFONT, colorLtGrey},
//	{"Neil 'haste' Toronto", UI_CENTER | UI_SMALLFONT, colorWhite},
//	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	{"Development Assistance", UI_CENTER | UI_BIGFONT, colorLtGrey},
	{"Josef 'cnuke' Soentgen", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Mathias 'skynet' Heyer", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	//{"Art", UI_CENTER | UI_BIGFONT, colorMdGrey},
	//{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	{"Lead Design", UI_CENTER | UI_BIGFONT, colorLtGrey},
	{"Adrian 'otty' Fuhrmann", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	{"Design", UI_CENTER | UI_BIGFONT, colorLtGrey},
	{"Robert 'Tr3B' Beckebans", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Adrian 'otty' Fuhrmann", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Michael 'mic' Denno", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Stefan 'ReFlex' Lautner", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Josef 'cnuke' Soentgen", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Ross 'kit89' Forshaw", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Fabian 'Fabz0r' Rosten", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},


	{"Quake II: Lost Marine Team", UI_CENTER | UI_BIGFONT, colorLtGrey},
	{"Thearrel 'Kiltron' McKinney", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Jim 'Revility' Kern - www.jk.atomicarmadillo.com", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	{"OpenArena Team", UI_CENTER | UI_BIGFONT, colorLtGrey},
	{"jzero", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"leileilol", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Mancubus", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	//{"Sapphire Scar Team", UI_CENTER | UI_BIGFONT, colorWhite},
	//{ "Paul 'JTR' Steffens, Lee David Ash,", UI_CENTER|UI_SMALLFONT, &colorWhite },
	//{ "Michael 'mic' Denno", UI_CENTER|UI_SMALLFONT, &colorWhite },
	//{"", UI_CENTER | UI_SMALLFONT, colorBlue},

	//{ "Level Design", UI_CENTER|UI_SMALLFONT, &colorLtGrey },
	//{ "Michael 'mic' Denno", UI_CENTER|UI_SMALLFONT, &colorWhite },
	//{ "'Dominic 'cha0s' Szablewski", UI_CENTER|UI_SMALLFONT, &colorWhite },
	//{ "", UI_CENTER|UI_SMALLFONT, &colorBlue },

	{"The Freesound Project", UI_CENTER | UI_BIGFONT, colorLtGrey},
	{"AdcBicycle", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Abyssmal", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Andrew Duke", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"aust_paul", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"b0bd0bbs", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"dobroide", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Erdie", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"HcTrancer", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"ignotus", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"JimPurbrick", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Jovica", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"kijjaz", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"loofa", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"man", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"nkuitse", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Oppusum", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"PoisedToGlitch", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"room", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"suonho", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"swelk", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"young_daddy", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	{"Others", UI_CENTER | UI_BIGFONT, colorMdGrey},
	{"Yan 'Method' Ostretsov - www.methodonline.com", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Ken 'kat' Beyer - www.katsbits.com", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Robert 'BJA' Hodri - www.bja-design.de", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Willi 'whammes' Hammes - www.willihammes.com", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Lee David Ash - www.violationentertainment.com", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Philip Klevestav - http://www.philipk.net", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Mikael 'Ruohis' Ruohomaa", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Julian 'Plaque' Morris", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Yves 'evillair' Allaire - Quake4 eX texture set", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Matt 'Lunaran' Breit - Quake4 Powerplant texture set", UI_CENTER | UI_SMALLFONT, colorWhite},
//	{"Tom 'phantazm11' Perryman - Quake4 aztech texture set", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"William 'SPoG' Joseph", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"Robin 'Wakey' Pengelstorfer", UI_CENTER | UI_SMALLFONT, colorWhite},
//  {"Christian 'Lorax' Ballsieper", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"James 'HarlequiN' Taylor", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorWhite},

	{"Special Thanks To:", UI_CENTER | UI_BIGFONT, colorMdGrey},
	{"id Software", UI_CENTER | UI_SMALLFONT, colorWhite},
//  {"IOQuake 3 project - www.ioquake3.org", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorBlue},

	{"Contributors", UI_CENTER | UI_BIGFONT, colorMdGrey},
	{"For a list of contributors,", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"see the accompanying CONTRIBUTORS.txt", UI_CENTER | UI_SMALLFONT, colorWhite},
	{"", UI_CENTER | UI_SMALLFONT, colorBlue},

	{"Websites:", UI_CENTER | UI_BIGFONT, colorMdGrey},
	{"www.sourceforge.net/projects/xreal", UI_CENTER | UI_BIGFONT, colorBlue},
	{"xreal.sourceforge.net", UI_CENTER | UI_BIGFONT, colorBlue},
	{"", UI_CENTER | UI_SMALLFONT, colorBlue},

	{"XreaL(c) 2005-2009, XreaL Team and Contributors", UI_CENTER | UI_SMALLFONT, colorRed},

	{NULL}
};


/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_CreditMenu_Key(int key)
{
	if(key & K_CHAR_FLAG)
		return 0;

	// pressing the escape key or clicking the mouse will exit
	// we also reset the music volume to the user's original
	// choice here,  by setting s_musicvolume to the stored var
	trap_Cmd_ExecuteText(EXEC_APPEND, va("s_musicvolume %f; quit\n", mvolume));
	return 0;
}

/*
=================
ScrollingCredits_Draw

Main drawing function
=================
*/
static void ScrollingCredits_Draw(void)
{
	int             x = 320, y, n;
	float           textScale = 0.25f;
	vec4_t          color;
	float           textZoom;

	// first, fill the background with the specified shader
//  UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BackgroundShader);

	// draw the stuff by setting the initial y location
	y = 480 - SCROLLSPEED * (float)(uis.realtime - starttime) / 100;


	// loop through the entire credits sequence
	for(n = 0; n <= sizeof(credits) - 1; n++)
	{

		// this NULL string marks the end of the credits struct
		if(credits[n].string == NULL)
		{
			/*
			   // credits sequence is completely off screen
			   if(y < -16)
			   {
			   // TODO: bring up XreaL plaque and fade-in and wait for keypress?
			   break;
			   }
			 */
			break;
		}

		if(credits[n].style & UI_GIANTFONT)
			textScale = 0.5f;
		else if(credits[n].style & UI_BIGFONT)
			textScale = 0.35f;
		else
			textScale = 0.2f;

		VectorSet4(color, credits[n].color[0], credits[n].color[1], credits[n].color[2], 0.0f);

		if(y <= 0 || y >= 480)
		{
			color[3] = 0;
		}
		else
		{

			color[3] = sin(M_PI / 480.0f * y);

		}

		textZoom = color[3] * 4 * textScale;

		if(textZoom > textScale)
			textZoom = textScale;

		textScale = textZoom;

		if(credits[n].style & UI_GIANTFONT)
			UI_DrawRect(0, y - color[3] * 20 + (sin(uis.realtime / 100.0f) * 10 * (1.0f - color[3])), 640, 3 + color[3] * 40,
						color_cursorLines);
		else if(credits[n].style & UI_BIGFONT)
			UI_DrawRect(0, y - color[3] * 10 + (sin(uis.realtime / 100.0f) * 10 * (1.0f - color[3])), 640, 1 + color[3] * 20,
						color_cursorLines);



		UI_Text_Paint(x, y, textScale, color, credits[n].string, 0, 0, credits[n].style | UI_DROPSHADOW, &uis.freeSansBoldFont);

		y += SMALLCHAR_HEIGHT + 4;

		/*
		   if(credits[n].style & UI_SMALLFONT)
		   {
		   y += SMALLCHAR_HEIGHT;// * PROP_SMALL_SIZE_SCALE;
		   }
		   else if(credits[n].style & UI_BIGFONT)
		   {
		   y += BIGCHAR_HEIGHT;
		   }
		   else if(credits[n].style & UI_GIANTFONT)
		   {
		   y += GIANTCHAR_HEIGHT;// * (1 / PROP_SMALL_SIZE_SCALE);
		   }
		 */

		// if y is off the screen, break out of loop
		//if(y > 480)
	}

	if(y < 0)
	{
		// repeat the credits
		starttime = uis.realtime;
	}
}

/*
===============
UI_CreditMenu
===============
*/
void UI_CreditMenu(void)
{
	memset(&s_credits, 0, sizeof(s_credits));

	s_credits.menu.draw = ScrollingCredits_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	UI_PushMenu(&s_credits.menu);

	starttime = uis.realtime;	// record start time for credits to scroll properly
	mvolume = trap_Cvar_VariableValue("s_musicvolume");
	if(mvolume < 0.5)
		trap_Cmd_ExecuteText(EXEC_APPEND, "s_musicvolume 0.5\n");
	trap_Cmd_ExecuteText(EXEC_APPEND, "music music/credits.ogg\n");

	// load the background shader
//  BackgroundShader = trap_R_RegisterShaderNoMip("menubackcredits");
}
