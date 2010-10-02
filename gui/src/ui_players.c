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
// ui_players.c

#include <hat/gui/ui_local.h>


#define UI_TIMER_GESTURE		2300
#define UI_TIMER_JUMP			1000
#define UI_TIMER_LAND			130
#define UI_TIMER_WEAPON_SWITCH	300
#define UI_TIMER_ATTACK			500
#define	UI_TIMER_MUZZLE_FLASH	20
#define	UI_TIMER_WEAPON_DELAY	250

#define JUMP_HEIGHT				56

#define SWINGSPEED				0.3f

#define SPIN_SPEED				0.9f
#define COAST_TIME				1000


/*
===============
UI_DrawPlayer
===============
*/
void UI_DrawPlayer(float x, float y, float w, float h, playerInfo_t * pi, int time)
{
	UI_XPPM_Player(x, y, w, h, pi, time);
}


/*
==========================
UI_RegisterClientSkin
==========================
*/
/*static qboolean UI_RegisterClientSkin(playerInfo_t * pi, const char *modelName, const char *skinName)
{
	char            filename[MAX_QPATH];

	Com_sprintf(filename, sizeof(filename), "models/players/%s/lower_%s.skin", modelName, skinName);
	pi->legsSkin = trap_R_RegisterSkin(filename);

	Com_sprintf(filename, sizeof(filename), "models/players/%s/upper_%s.skin", modelName, skinName);
	pi->torsoSkin = trap_R_RegisterSkin(filename);

	Com_sprintf(filename, sizeof(filename), "models/players/%s/head_%s.skin", modelName, skinName);
	pi->headSkin = trap_R_RegisterSkin(filename);

	if(!pi->legsSkin || !pi->torsoSkin || !pi->headSkin)
	{
		return qfalse;
	}

	return qtrue;
}*/


/*
==========================
UI_RegisterClientModelname
==========================
*/
qboolean UI_RegisterClientModelname(playerInfo_t * pi, const char *modelSkinName)
{
	char            modelName[MAX_QPATH];
	char            skinName[MAX_QPATH];
	char           *slash;

	pi->torsoModel = 0;
	pi->headModel = 0;

	if(!modelSkinName[0])
	{
		return qfalse;
	}

	Q_strncpyz(modelName, modelSkinName, sizeof(modelName));

	slash = strchr(modelName, '/');
	if(!slash)
	{
		// modelName did not include a skin name
		Q_strncpyz(skinName, "default", sizeof(skinName));
	}
	else
	{
		Q_strncpyz(skinName, slash + 1, sizeof(skinName));
		// truncate modelName
		*slash = 0;
	}

#ifdef XPPM
	return UI_XPPM_RegisterModel(pi, modelName, skinName);
#endif
}


/*
===============
UI_PlayerInfo_SetModel
===============
*/
void UI_PlayerInfo_SetModel(playerInfo_t * pi, const char *model)
{
	memset(pi, 0, sizeof(*pi));
	UI_RegisterClientModelname(pi, model);
}


