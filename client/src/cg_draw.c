/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2006 Josef Soentgen <cnuke@users.sourceforge.net>

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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

vec4_t          redTeamColor = { 0.9f, 0.0f, 0.2f, 0.80f };
vec4_t          blueTeamColor = { 0.2f, 0.0f, 0.9f, 0.80f };
vec4_t          baseTeamColor = { 1.0f, 1.0f, 1.0f, 0.80f };




int             drawTeamOverlayModificationCount = -1;

int             sortedTeamPlayers[TEAM_MAXOVERLAY];
int             numSortedTeamPlayers;

char            systemChat[256];
char            teamChat1[256];
char            teamChat2[256];

int CG_Text_Width(const char *text, float scale, int limit, const fontInfo_t * font)
{
	int             count, len;
	float           out;
	const glyphInfo_t *glyph;
	float           useScale;

// FIXME: see ui_main.c, same problem
//  const unsigned char *s = text;
	const char     *s = text;

	useScale = scale * font->glyphScale;
	out = 0;
	if(text)
	{
		len = strlen(text);
		if(limit > 0 && len > limit)
		{
			len = limit;
		}
		count = 0;
		while(s && *s && count < len)
		{
			if(Q_IsColorString(s))
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[(int)*s];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}

	return out * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit, const fontInfo_t * font)
{
	int             len, count;
	float           max;
	const glyphInfo_t *glyph;
	float           useScale;

// TTimo: FIXME
//  const unsigned char *s = text;
	const char     *s = text;

	useScale = scale * font->glyphScale;
	max = 0;
	if(text)
	{
		len = strlen(text);
		if(limit > 0 && len > limit)
		{
			len = limit;
		}
		count = 0;
		while(s && *s && count < len)
		{
			if(Q_IsColorString(s))
			{
				s += 2;
				continue;
			}
			else
			{
				glyph = &font->glyphs[(int)*s];
				if(max < glyph->height)
				{
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}

	return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2,
					   qhandle_t hShader)
{
	float           w, h;

	w = width * scale;
	h = height * scale;
	CG_AdjustFrom640(&x, &y, &w, &h);

	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

void CG_Text_Paint(float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style,
				   const fontInfo_t * font)
{
	int             len, count;
	vec4_t          newColor;
	const glyphInfo_t *glyph;
	float           useScale;

	useScale = scale * font->glyphScale;
	if(text)
	{
// TTimo: FIXME
//      const unsigned char *s = text;
		const char     *s = text;

		trap_R_SetColor(color);
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
		len = strlen(text);
		if(limit > 0 && len > limit)
		{
			len = limit;
		}
		count = 0;
		while(s && *s && count < len)
		{
			glyph = &font->glyphs[(int)*s];

			if(Q_IsColorString(s))
			{
				memcpy(newColor, (float *)g_color_table[ColorIndex(*(s + 1))], sizeof(newColor));
				newColor[3] = color[3];
				trap_R_SetColor(newColor);
				s += 2;
				continue;
			}
			else
			{
				float           yadj = useScale * glyph->top;

				if(style & UI_DROPSHADOW)	// || style == ITEM_TEXTSTYLE_SHADOWEDMORE)
				{
					int             ofs = 1;	//style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;

					colorBlack[3] = newColor[3];
					trap_R_SetColor(colorBlack);
					CG_Text_PaintChar(x + ofs, y - yadj + ofs,
									  glyph->imageWidth,
									  glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor(newColor);
				}
				CG_Text_PaintChar(x, y - yadj,
								  glyph->imageWidth,
								  glyph->imageHeight, useScale, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);

				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
		}
		trap_R_SetColor(NULL);
	}
}

void CG_Text_PaintAligned(int x, int y, const char *s, float scale, int style, const vec4_t color, const fontInfo_t * font)
{
	int             w, h;

	w = CG_Text_Width(s, scale, 0, font);
	h = CG_Text_Height(s, scale, 0, font);

	if(style & UI_CENTER)
	{
		CG_Text_Paint(x - w / 2, y + h / 2, scale, color, s, 0, 0, style, font);
	}
	else if(style & UI_RIGHT)
	{
		CG_Text_Paint(x - w, y + h / 2, scale, color, s, 0, 0, style, font);
	}
	else
	{
		// UI_LEFT
		CG_Text_Paint(x, y + h / 2, scale, color, s, 0, 0, style, font);
	}
}

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
static void CG_DrawField(int x, int y, int width, int value, float size)
{
	char            num[16], *ptr;
	int             l;
	int             frame;

	if(width < 1)
	{
		return;
	}

	// draw number string
	if(width > 5)
	{
		width = 5;
	}

	switch (width)
	{
		case 1:
			value = value > 9 ? 9 : value;
			value = value < 0 ? 0 : value;
			break;
		case 2:
			value = value > 99 ? 99 : value;
			value = value < -9 ? -9 : value;
			break;
		case 3:
			value = value > 999 ? 999 : value;
			value = value < -99 ? -99 : value;
			break;
		case 4:
			value = value > 9999 ? 9999 : value;
			value = value < -999 ? -999 : value;
			break;
	}

	Com_sprintf(num, sizeof(num), "%i", value);
	l = strlen(num);
	if(l > width)
		l = width;
	x += 2 + (CHAR_WIDTH * size * (width - l)) / 2;

	ptr = num;
	while(*ptr && l)
	{
		if(*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		CG_DrawPic(x, y, CHAR_WIDTH * size, CHAR_HEIGHT * size, cgs.media.numberShaders[frame]);
		x += CHAR_SPACE * size;
		ptr++;
		l--;
	}
}

/*
================
CG_Draw3DModel
================
*/
void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles)
{
	refdef_t        refdef;
	refEntity_t     ent;
	refLight_t      light;
	float           fov_x;

	if(!cg_draw3dIcons.integer || !cg_drawIcons.integer)
	{
		return;
	}

	CG_AdjustFrom640(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof(refdef));

	memset(&ent, 0, sizeof(ent));
	AnglesToAxis(angles, ent.axis);
	VectorCopy(origin, ent.origin);
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;	// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL | RDF_NOSHADOWS;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);

	// add light
	memset(&light, 0, sizeof(refLight_t));

	light.rlType = RL_PROJ;

	VectorCopy(cg.refdef.vieworg, light.origin);
	light.origin[1] += 10;

	QuatClear(light.rotation);

	light.color[0] = 0.8f;
	light.color[1] = 0.8f;
	light.color[2] = 0.8f;

	fov_x = tanf(DEG2RAD(cg.refdef.fov_x * 0.5f));
	VectorCopy(cg.refdef.viewaxis[0], light.projTarget);
	VectorScale(cg.refdef.viewaxis[1], -fov_x, light.projRight);
	VectorScale(cg.refdef.viewaxis[2], fov_x, light.projUp);
	VectorScale(cg.refdef.viewaxis[0], -200, light.projStart);
	VectorScale(cg.refdef.viewaxis[0], 1000, light.projEnd);

	trap_R_AddRefLightToScene(&light);

	trap_R_RenderScene(&refdef);
}

/*
================
CG_Draw3DWeaponModel
================
*/
void CG_Draw3DWeaponModel(float x, float y, float w, float h, qhandle_t weaponModel, qhandle_t barrelModel, qhandle_t skin,
						  vec3_t origin, vec3_t angles)
{
	refdef_t        refdef;
	refEntity_t     ent;
	refLight_t      light;
	float           fov_x;

	if(!cg_draw3dIcons.integer || !cg_drawIcons.integer)
	{
		return;
	}

	CG_AdjustFrom640(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof(refdef));

	memset(&ent, 0, sizeof(ent));
	AnglesToAxis(angles, ent.axis);
	VectorCopy(origin, ent.origin);
	ent.hModel = weaponModel;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;	// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL | RDF_NOSHADOWS;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();

	trap_R_AddRefEntityToScene(&ent);
	if(barrelModel)
	{
		refEntity_t     barrel;

		memset(&barrel, 0, sizeof(barrel));

		barrel.hModel = barrelModel;

		VectorCopy(ent.lightingOrigin, barrel.lightingOrigin);
		barrel.shadowPlane = ent.shadowPlane;
		barrel.renderfx = ent.renderfx;

		CG_PositionRotatedEntityOnTag(&barrel, &ent, weaponModel, "tag_barrel");

		AxisCopy(ent.axis, barrel.axis);
		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		trap_R_AddRefEntityToScene(&barrel);
	}

	// add light
	memset(&light, 0, sizeof(refLight_t));

	light.rlType = RL_PROJ;

	VectorCopy(cg.refdef.vieworg, light.origin);
	light.origin[1] += 10;

	QuatClear(light.rotation);

	light.color[0] = 0.8f;
	light.color[1] = 0.8f;
	light.color[2] = 0.8f;

	fov_x = tanf(DEG2RAD(cg.refdef.fov_x * 0.5f));
	VectorCopy(cg.refdef.viewaxis[0], light.projTarget);
	VectorScale(cg.refdef.viewaxis[1], -fov_x, light.projRight);
	VectorScale(cg.refdef.viewaxis[2], fov_x, light.projUp);
	VectorScale(cg.refdef.viewaxis[0], -30, light.projStart);
	VectorScale(cg.refdef.viewaxis[0], 1000, light.projEnd);

	trap_R_AddRefLightToScene(&light);

	trap_R_RenderScene(&refdef);
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead(float x, float y, float w, float h, int clientNum, vec3_t headAngles)
{
	clientInfo_t   *ci;

	ci = &cgs.clientinfo[clientNum];

	if(cg_drawIcons.integer)
	{
		CG_DrawPic(x, y, w, h, ci->modelIcon);
	}

	// if they are deferred, draw a cross out
	if(ci->deferred)
	{
		CG_DrawPic(x, y, w, h, cgs.media.deferShader);
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel(float x, float y, float w, float h, int team, qboolean force2D)
{
	float           len;
	vec3_t          origin, angles;
	vec3_t          mins, maxs;
	qhandle_t       model, skin;

	if(!force2D && cg_draw3dIcons.integer)
	{
		VectorClear(angles);

		model = cgs.media.flagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds(model, mins, maxs);

		origin[2] = -0.5 * (mins[2] + maxs[2]);
		origin[1] = 0.5 * (mins[1] + maxs[1]);

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * (maxs[2] - mins[2]);
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin(cg.time / 2000.0);;

		if(team == TEAM_RED)
		{
			skin = cgs.media.redFlagSkin;
		}
		else if(team == TEAM_BLUE)
		{
			skin = cgs.media.blueFlagSkin;
		}
		else if(team == TEAM_FREE)
		{
			skin = cgs.media.neutralFlagSkin;
		}
		else
		{
			return;
		}

		CG_Draw3DModel(x, y, w, h, model, skin, origin, angles);
	}
	else if(cg_drawIcons.integer)
	{
		gitem_t        *item;

		if(team == TEAM_RED)
		{
			item = BG_FindItemForPowerup(PW_REDFLAG);
		}
		else if(team == TEAM_BLUE)
		{
			item = BG_FindItemForPowerup(PW_BLUEFLAG);
		}
		else if(team == TEAM_FREE)
		{
			item = BG_FindItemForPowerup(PW_NEUTRALFLAG);
		}
		else
		{
			return;
		}
		if(item)
		{
			CG_DrawPic(x, y, w, h, cg_items[ITEM_INDEX(item)].icon);
		}
	}
}

/*
================
CG_DrawStatusBarHead
================
*/
static void CG_DrawStatusBarHead(float x)
{
	vec3_t          angles;
	float           size, stretch;
	float           frac;

	VectorClear(angles);

	if(cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME)
	{
		frac = (float)(cg.time - cg.damageTime) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * (1.5 - frac * 0.5);

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos(crandom() * M_PI);
		cg.headEndPitch = 5 * cos(crandom() * M_PI);

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	}
	else
	{
		if(cg.time >= cg.headEndTime)
		{
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos(crandom() * M_PI);
			cg.headEndPitch = 5 * cos(crandom() * M_PI);
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if(cg.headStartTime > cg.time)
	{
		cg.headStartTime = cg.time;
	}

	frac = (cg.time - cg.headStartTime) / (float)(cg.headEndTime - cg.headStartTime);
	frac = frac * frac * (3 - 2 * frac);
	angles[YAW] = cg.headStartYaw + (cg.headEndYaw - cg.headStartYaw) * frac;
	angles[PITCH] = cg.headStartPitch + (cg.headEndPitch - cg.headStartPitch) * frac;

	CG_DrawHead(x, 480 - size, size, size, cg.snap->ps.clientNum, angles);
}

/*
================
CG_DrawStatusBarFlag
================
*/
static void CG_DrawStatusBarFlag(float x, int team)
{
	CG_DrawFlagModel(x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse);
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team)
{
	vec4_t          hcolor;

	hcolor[3] = alpha;
	if(team == TEAM_RED)
	{
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	}
	else if(team == TEAM_BLUE)
	{
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	}
	else
	{
		return;
	}
	trap_R_SetColor(hcolor);
	CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(NULL);
}

/*
================
CG_DrawStatusBarQ3
================
*/
static void CG_DrawStatusBarQ3(void)
{
	int             color;
	centity_t      *cent;
	playerState_t  *ps;
	int             value;
	vec4_t          hcolor;
	vec3_t          angles;
	vec3_t          origin;

#ifdef MISSIONPACK
	qhandle_t       handle;
#endif
	static float    colors[4][4] = {
		{1.0f, 0.69f, 0.0f, 1.0f},	// normal
		{1.0f, 0.2f, 0.2f, 1.0f},	// low health
		{0.5f, 0.5f, 0.5f, 1.0f},	// weapon firing
		{1.0f, 1.0f, 1.0f, 1.0f}	// health > 100
	};

	// draw the team background
	CG_DrawTeamBackground(0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM]);

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear(angles);

	// draw any 3D icons first, so the changes back to 2D are minimized
	if(cent->currentState.weapon && cg_weapons[cent->currentState.weapon].ammoModel)
	{
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin(cg.time / 1000.0);

		CG_Draw3DModel(CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 445, ICON_SIZE, ICON_SIZE,
					   cg_weapons[cent->currentState.weapon].ammoModel, 0, origin, angles);
	}

	CG_DrawStatusBarHead(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE);

	if(cg.predictedPlayerState.powerups[PW_REDFLAG])
	{
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED);
	}
	else if(cg.predictedPlayerState.powerups[PW_BLUEFLAG])
	{
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE);
	}
	else if(cg.predictedPlayerState.powerups[PW_NEUTRALFLAG])
	{
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE);
	}

	if(ps->stats[STAT_ARMOR])
	{
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = (cg.time & 2047) * 360 / 2048.0;

		CG_Draw3DModel(370 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 445, ICON_SIZE, ICON_SIZE,
					   cgs.media.armorModel, 0, origin, angles);
	}

#ifdef MISSIONPACK
	if(cgs.gametype == GT_HARVESTER)
	{
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = (cg.time & 2047) * 360 / 2048.0;
		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
			handle = cgs.media.blueSkullModel;
		else
			handle = cgs.media.redSkullModel;

		CG_Draw3DModel(640 - (TEXT_ICON_SPACE + ICON_SIZE), 416, ICON_SIZE, ICON_SIZE, handle, 0, origin, angles);
	}
#endif

	// ammo
	if(cent->currentState.weapon)
	{
		value = ps->ammo[cent->currentState.weapon];
		if(value > -1)
		{
			if(cg.predictedPlayerState.weaponstate == WEAPON_FIRING && cg.predictedPlayerState.weaponTime > 100)
			{
				// draw as dark grey when reloading
				color = 2;		// dark grey
			}
			else
			{
				if(value >= 0)
					color = 0;	// green
				else
					color = 1;	// red
			}
			trap_R_SetColor(colors[color]);

			CG_DrawField(0, 445, 3, value, 1.0f);
			trap_R_SetColor(NULL);

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if(!cg_draw3dIcons.integer && cg_drawIcons.integer)
			{
				qhandle_t       icon;

				icon = cg_weapons[cg.predictedPlayerState.weapon].ammoIcon;
				if(icon)
					CG_DrawPic(CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 445, ICON_SIZE, ICON_SIZE, icon);
			}
		}
	}

	// health
	value = ps->stats[STAT_HEALTH];
	if(value > 100)
	{
		trap_R_SetColor(colors[3]);	// white
	}
	else if(value > 25)
	{
		trap_R_SetColor(colors[0]);	// green
	}
	else if(value > 0)
	{
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor(colors[color]);
	}
	else
	{
		trap_R_SetColor(colors[1]);	// red
	}

	// stretch the health up when taking damage
	CG_DrawField(185, 445, 3, value, 1.0f);
	CG_ColorForHealth(hcolor);
	trap_R_SetColor(hcolor);

	// armor
	value = ps->stats[STAT_ARMOR];
	if(value > 0)
	{
		trap_R_SetColor(colors[0]);
		CG_DrawField(370, 445, 3, value, 1.0f);
		trap_R_SetColor(NULL);

		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if(!cg_draw3dIcons.integer && cg_drawIcons.integer)
			CG_DrawPic(370 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 445, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon);
	}

#ifdef MISSIONPACK
	// cubes
	if(cgs.gametype == GT_HARVESTER)
	{
		value = ps->generic1;
		if(value > 99)
			value = 99;

		trap_R_SetColor(colors[0]);
		CG_DrawField(640 - (CHAR_WIDTH * 2 + TEXT_ICON_SPACE + ICON_SIZE), 445, 2, value);
		trap_R_SetColor(NULL);

		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if(!cg_draw3dIcons.integer && cg_drawIcons.integer)
		{
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
				handle = cgs.media.redSkullIcon;
			else
				handle = cgs.media.blueSkullIcon;

			CG_DrawPic(640 - (TEXT_ICON_SPACE + ICON_SIZE), 445, ICON_SIZE, ICON_SIZE, handle);
		}
	}
#endif
}

/*
================
CG_DrawStatusBarXreaL
================
*/


static void CG_DrawStatusBarXreaL(void)
{
/*	int             color;
	centity_t      *cent;
	playerState_t  *ps;
	int             value;
	vec3_t          angles;
	vec3_t          origin;
	static float    colors[4][4] = {
		{1.0f, 0.69f, 0.0f, 1.0f},	// normal
		{1.0f, 0.2f, 0.2f, 1.0f},	// low health
		{0.5f, 0.5f, 0.5f, 1.0f},	// weapon firing
		{1.0f, 1.0f, 1.0f, 1.0f}	// health > 100
	};
	vec4_t         *colorItem;
	vec4_t          colorHudBlack = { 0.0f, 0.0f, 0.0f, 1.0f };	// b/w
	vec4_t          colorHudSemiBlack = { 0.0f, 0.0f, 0.0f, 0.2f };	// b/w
	vec4_t          colorHealth = { 1.0f, 0.25f, 0.25f, 0.7f };
	vec4_t          colorArmor = { 0.25f, 0.25f, 1.0f, 0.7f };
	vec4_t          colorAmmo = { 0.25f, 1.0f, 0.25f, 0.7f };
	vec4_t          colorTeamBlue = { 0.0f, 0.0f, 1.0f, 0.5f };	// blue
	vec4_t          colorTeamRed = { 1.0f, 0.0f, 0.0f, 0.5f };	// red

	colorItem = &colorHudBlack;

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		colorItem = &colorTeamBlue;
	else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		colorItem = &colorTeamRed;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear(angles);

	if(cg.predictedPlayerState.powerups[PW_REDFLAG])
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED);
	else if(cg.predictedPlayerState.powerups[PW_BLUEFLAG])
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE);
	else if(cg.predictedPlayerState.powerups[PW_NEUTRALFLAG])
		CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE);

	// ammo
	trap_R_SetColor(*colorItem);
	CG_DrawPic(640 - 118, 480 - 38, 108, 36, cgs.media.sideBarItemRShader);
	trap_R_SetColor(NULL);

	if(cent->currentState.weapon)
	{
		if(cg_drawStatusLines.integer)
		{
			trap_R_SetColor(colorHudSemiBlack);
			CG_DrawPic(320 + 5, 480 - 38, 300, 36, cgs.media.sideBarItemLShader);
		}

		value = ps->ammo[cent->currentState.weapon];
		if(value > -1)
		{
			if(cg.predictedPlayerState.weaponstate == WEAPON_FIRING && cg.predictedPlayerState.weaponTime > 100)
			{
				// draw as dark grey when reloading
				color = 2;		// dark grey
			}
			else
			{
				if(value >= 0)
					color = 3;	// white
				else
					color = 1;	// red
			}

			if(cg_drawStatusLines.integer)
			{
				colorAmmo[0] = colors[color][0];
				colorAmmo[1] = colors[color][1];
				colorAmmo[2] = colors[color][2];

				trap_R_SetColor(colorAmmo);
				CG_DrawPic(320 + 5, 480 - 38, Q_bound(0, value * 300 / 200, 300), 36, cgs.media.sideBarItemLShader);
			}

			trap_R_SetColor(colors[color]);
			CG_DrawField(520, 480 - CHAR_HEIGHT - 7, 3, value, 1.0f);
			trap_R_SetColor(NULL);

			// if we didn't draw a 2D icon, draw a 3D icon for ammo
			if(!cg_draw3dIcons.integer && cg_drawIcons.integer)
			{
				qhandle_t       icon;

				icon = cg_weapons[cg.predictedPlayerState.weapon].ammoIcon;
				if(icon)
					CG_DrawPic(640 - ICON_SIZE - 12, 480 - ICON_SIZE - 5, ICON_SIZE, ICON_SIZE, icon);
			}
			else
			{
				if(cent->currentState.weapon && cg_weapons[cent->currentState.weapon].ammoModel)
				{
					origin[0] = 70;
					origin[1] = 0;
					origin[2] = 0;
					angles[YAW] = 90 + 20 * sin(cg.time / 1000.0);
					CG_Draw3DModel(640 - ICON_SIZE - 13, 480 - ICON_SIZE - 5, ICON_SIZE, ICON_SIZE,
								   cg_weapons[cent->currentState.weapon].ammoModel, 0, origin, angles);
				}
			}
		}
	}

	// health
	trap_R_SetColor(*colorItem);
	CG_DrawPic(10, 480 - 38, 108, 36, cgs.media.sideBarItemLShader);
	trap_R_SetColor(NULL);

	value = ps->stats[STAT_HEALTH];
	if(value > 100)
		color = 0;				// yellow
	else if(value > 25)
		color = 3;				// white
	else if(value > 0)
		color = (cg.time >> 8) & 1;
	else
		color = 1;				// red

	trap_R_SetColor(colors[color]);
	CG_DrawField(ICON_SIZE + 5, 480 - CHAR_HEIGHT - 7, 3, value, 1.0f);
	trap_R_SetColor(NULL);

	CG_DrawStatusBarHead(10);

	if(cg_drawStatusLines.integer)
	{
		trap_R_SetColor(colorHudSemiBlack);
		CG_DrawPic(320 - 305, 480 - 38, 300, 36, cgs.media.sideBarItemRShader);

		trap_R_SetColor(colorHealth);
		CG_DrawPic(320 - 305 + (300 - Q_bound(0, value * 300 / 200, 300)), 480 - 38, Q_bound(0, value * 300 / 200, 300), 36,
				   cgs.media.sideBarItemRShader);
	}

	// armor
	value = ps->stats[STAT_ARMOR];
	if(value > 0)
	{
		trap_R_SetColor(*colorItem);
		CG_DrawPic(10, 480 - 79, 108, 36, cgs.media.sideBarItemLShader);
		trap_R_SetColor(NULL);

		CG_DrawField(ICON_SIZE + 5, 480 - 72, 3, value, 1.0f);

		// if we didn't draw a 2D icon, draw a 3D icon for armor
		if(!cg_draw3dIcons.integer && cg_drawIcons.integer)
		{
			CG_DrawPic(12, 480 - ICON_SIZE * 2 - 13, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon);
		}
		else
		{
			origin[0] = 90;
			origin[1] = 0;
			origin[2] = -10;
			angles[YAW] = (cg.time & 2047) * 360 / 2048.0;
			CG_Draw3DModel(13, 480 - ICON_SIZE * 2 - 13, ICON_SIZE, ICON_SIZE, cgs.media.armorModel, 0, origin, angles);
		}
	}

	if(cg_drawStatusLines.integer)
	{
		trap_R_SetColor(colorHudSemiBlack);
		CG_DrawPic(320 - 305, 480 - 79, 300, 36, cgs.media.sideBarItemRShader);

		trap_R_SetColor(colorArmor);
		CG_DrawPic(320 - 305 + (300 - Q_bound(0, value * 300 / 200, 300)), 480 - 79, Q_bound(0, value * 300 / 200, 300), 36,
				   cgs.media.sideBarItemRShader);
	}
	*/

}


/* new hud

TODO: divide into sections ( lower, upper etc )

*/

#define HUD_B_Y 425
#define HUD_B_MIDDLE_OFFSET_Y 5
#define HUD_BAR_EXTSIZE 6
#define HUD_B_BORDEROFFSET 5

//fontsizes

#define HUD_TIMERSIZE 0.15f
#define HUD_SCORELIMITSIZETEAM 0.25f
#define HUD_SCORESIZETEAM 0.33f

#define HUD_SCORESIZE 0.4f
#define HUD_STATSIZE 0.5f


void CG_DrawHudString(int x, int y, char *s, float size, int style, const vec4_t color)
{
	CG_Text_PaintAligned(x, y, s, size, style | UI_DROPSHADOW, color, &cgs.media.hudNumberFont);
}


void CG_DrawStatusBarNew(void)
{
	int             value;
	playerState_t  *ps;
	int             offset;
	int             offset2;
	centity_t      *cent;
	int             score;

	char           *s;
	int             mins, seconds, tens;
	int             msec;


	float           hflash = 0;
	float           arflash = 0;
	float           amflash = 0;

	float           fontsize;

	int             pickup = cg.time - cg.itemPickupTime;

	vec4_t          color = { 1.0f, 1.0f, 1.0f, 0.80f };
	vec4_t          colorOverlay = { 1.0f, 1.0f, 1.0f, 1.0f };

	vec4_t          basecolor;
	vec4_t          fadecolor;

	vec4_t          healthcolor = { 1.0f, 1.0f, 1.0f, 0.80f };
	vec4_t          armorcolor = { 1.0f, 1.0f, 1.0f, 0.80f };
	vec4_t          ammocolor = { 1.0f, 1.0f, 1.0f, 0.80f };
	vec4_t          scorecolor = { 1.0f, 1.0f, 1.0f, 0.80f };

	ps = &cg.snap->ps;
	cent = &cg_entities[cg.snap->ps.clientNum];


	if(ps->persistant[PERS_TEAM] == TEAM_BLUE)
		VectorCopy4(blueTeamColor, basecolor);
	else if(ps->persistant[PERS_TEAM] == TEAM_RED)
		VectorCopy4(redTeamColor, basecolor);
	else
		VectorCopy4(baseTeamColor, basecolor);

	colorOverlay[3] = basecolor[3];

	// top stats bar
	if(cgs.gametype >= GT_TEAM)
	{
		// background middle - score limit
		if(cgs.gametype >= GT_CTF)
			score = cgs.capturelimit;
		else
			score = cgs.fraglimit;

		trap_R_SetColor(basecolor);
		CG_DrawPic(320 - 40, 0, 80, 40, cgs.media.hud_top_team_middle);
		trap_R_SetColor(NULL);

		VectorCopy4(color, scorecolor);
		scorecolor[3] = 0.8f;

		// tdm/ctf frag/capturelimit
		s = va("%i", score);
		fontsize = HUD_SCORELIMITSIZETEAM;
		if(score > 99)
			fontsize *= 1.0f;
		else if(score > 9)
			fontsize *= 1.25f;
		else
			fontsize *= 1.5f;

		CG_DrawHudString(320, 24, s, fontsize, UI_CENTER, scorecolor);


		trap_R_SetColor(colorOverlay);
		CG_DrawPic(320 - 40, 0, 80, 40, cgs.media.hud_top_team_middle_overlay);
		trap_R_SetColor(NULL);


		// background left - red team
		VectorCopy4(redTeamColor, color);

		score = cgs.scores1;

		if(cgs.gametype >= GT_CTF)
		{
			trap_R_SetColor(color);
			CG_DrawPic(320 - 10 - 100, 3, 100, 40, cgs.media.hud_top_ctf_left);
			trap_R_SetColor(NULL);
		}
		else
		{
			trap_R_SetColor(color);
			CG_DrawPic(320 - 10 - 100, 3, 100, 40, cgs.media.hud_top_team_left);
			trap_R_SetColor(NULL);
		}

		VectorSet4(scorecolor, 1.0f, 1.0f, 1.0f, 0.80f);

		// digits
		s = va("%i", score);
		fontsize = HUD_SCORESIZETEAM;
		CG_DrawHudString(255, 15, s, fontsize, UI_CENTER, scorecolor);


		trap_R_SetColor(colorOverlay);
		CG_DrawPic(320 - 10 - 100, 3, 100, 40, cgs.media.hud_top_team_left_overlay);
		trap_R_SetColor(NULL);

		// background right - blue team
		VectorCopy4(blueTeamColor, color);
		score = cgs.scores2;

		if(cgs.gametype >= GT_CTF)
		{
			trap_R_SetColor(color);
			CG_DrawPic(320 + 10, 3, 100, 40, cgs.media.hud_top_ctf_right);
			trap_R_SetColor(NULL);
		}
		else
		{
			trap_R_SetColor(color);
			CG_DrawPic(320 + 10, 3, 100, 40, cgs.media.hud_top_team_right);
			trap_R_SetColor(NULL);
		}

		VectorSet4(scorecolor, 1.0f, 1.0f, 1.0f, 0.80f);

		// digits
		s = va("%i", score);
		fontsize = HUD_SCORESIZETEAM;
		CG_DrawHudString(385, 15, s, fontsize, UI_CENTER, scorecolor);

		trap_R_SetColor(colorOverlay);
		CG_DrawPic(320 + 10, 3, 100, 40, cgs.media.hud_top_team_right_overlay);
		trap_R_SetColor(NULL);

	}
	else
	{
		// FFA

		// background middle - your score
		score = cg.snap->ps.persistant[PERS_SCORE];

		trap_R_SetColor(basecolor);
		CG_DrawPic(320 - 25, 0, 50, 40, cgs.media.hud_top_ffa_middle);
		trap_R_SetColor(NULL);

		// blink your score if on first or second place
		VectorCopy4(colorWhite, scorecolor);

		scorecolor[3] = 0.8f;

		if(score == cgs.scores1)
			scorecolor[0] = scorecolor[1] = 0.66f + 0.33f * sin(cg.time / 100.0f);
		else if(score == cgs.scores2)
			scorecolor[1] = scorecolor[2] = 0.66f + 0.33f * sin(cg.time / 100.0f);

		// digits
		s = va("%i", score);
		fontsize = HUD_SCORESIZE;
		if(score > 99)
			fontsize *= 0.5f;
		else if(score > 9)
			fontsize *= 0.75;
		else
			fontsize *= 1.5f;

		CG_DrawHudString(320, 21, s, fontsize, UI_CENTER, scorecolor);

		trap_R_SetColor(colorOverlay);
		CG_DrawPic(320 - 25, 0, 50, 40, cgs.media.hud_top_ffa_middle_overlay);
		trap_R_SetColor(NULL);


		// background left - fraglimit
		score = cgs.fraglimit;

		trap_R_SetColor(basecolor);
		CG_DrawPic(320 - 25 - 66, 0, 66, 40, cgs.media.hud_top_ffa_left);
		trap_R_SetColor(NULL);


		// blink fraglimit if close enough
		VectorCopy4(colorWhite, scorecolor);
		scorecolor[3] = 0.8f;

		if(score > 0 && score - cgs.scores1 < 5)
			scorecolor[2] = scorecolor[1] = sin(cg.time / 200.0f);

		// digits
		s = va("%i", score);
		fontsize = HUD_SCORESIZE * 0.8f;

		CG_DrawHudString(270, 19, s, fontsize, UI_CENTER, scorecolor);


		trap_R_SetColor(colorOverlay);
		CG_DrawPic(320 - 25 - 66, 0, 66, 40, cgs.media.hud_top_ffa_left_overlay);
		trap_R_SetColor(NULL);

		// background right - first place score if self not first, or second place score if on first place
		if(cgs.scores1 == cg.snap->ps.persistant[PERS_SCORE])	//on first place, so draw second
			score = cgs.scores2;
		else					// not on first place, so draw first
			score = cgs.scores1;

		if(score == SCORE_NOT_PRESENT)
			score = 0;


		trap_R_SetColor(basecolor);
		CG_DrawPic(320 + 25, 0, 66, 40, cgs.media.hud_top_ffa_right);
		trap_R_SetColor(NULL);

		VectorCopy4(colorWhite, scorecolor);
		scorecolor[3] = 0.8f;

		// digits
		s = va("%i", score);
		fontsize = HUD_SCORESIZE * 0.8f;

		CG_DrawHudString(370, 19, s, fontsize, UI_CENTER, scorecolor);


		trap_R_SetColor(colorOverlay);
		CG_DrawPic(320 + 25, 0, 66, 40, cgs.media.hud_top_ffa_right_overlay);
		trap_R_SetColor(NULL);

	}

	// draw countdown
	if(cgs.timelimit > 0)
	{
		msec = ((cgs.timelimit * 60 * 1000) - cg.time - cgs.levelStartTime);

		if(msec > 0)
		{
			seconds = msec / 1000;
			mins = seconds / 60;
			seconds -= mins * 60;
			tens = seconds / 10;
			seconds -= tens * 10;

			s = va("%i:%i%i", mins, tens, seconds);

			CG_DrawHudString(320, 5, s, 0.2f, UI_CENTER, scorecolor);
		}
	}

	// left - health
	trap_R_SetColor(basecolor);
	CG_DrawPic(HUD_B_BORDEROFFSET, HUD_B_Y, 130, 50, cgs.media.hud_bar_left);
	trap_R_SetColor(NULL);

	value = ps->stats[STAT_HEALTH];

	if(value <= 5)
	{
		healthcolor[1] = healthcolor[2] = sin(cg.time / 25.0f);
	}
	else if(value <= 25)
	{
		healthcolor[1] = healthcolor[2] = sin(cg.time / 50.0f);
	}
	else if(value <= 50)
	{
		healthcolor[1] = healthcolor[2] = sin(cg.time / 100.0f);
	}


	healthcolor[3] = 1.0f;

	s = va("%i", value);
	fontsize = HUD_STATSIZE;
	CG_DrawHudString(80, 453, s, fontsize, UI_CENTER, healthcolor);

	trap_R_SetColor(colorOverlay);
	CG_DrawPic(HUD_B_BORDEROFFSET, HUD_B_Y, 130, 50, cgs.media.hud_bar_left_overlay);
	trap_R_SetColor(NULL);

	if(pickup > 0 && pickup < 300)
	{
		if(bg_itemlist[cg.itemPickup].giType == IT_HEALTH)
		{
			// flash health
			hflash = pickup / 10;
		}

		if(bg_itemlist[cg.itemPickup].giType == IT_ARMOR)
		{
			// armor health
			arflash = pickup / 10;
		}

		if(bg_itemlist[cg.itemPickup].giType == IT_AMMO)
		{
			// ammo health
			amflash = pickup / 10;
		}
	}
	else
	{
		pickup = 0;
	}

	VectorCopy4(basecolor, color);
	color[3] = 0.75f + 0.25f * sin(cg.time / 400.0f);

	trap_R_SetColor(color);
	CG_DrawPic(17 - hflash / 2, 435 - hflash / 2, 30 + hflash, 30 + hflash, cgs.media.hud_icon_health);
	trap_R_SetColor(NULL);

	//middle, ammo, ammo types, weaponselection
	VectorCopy4(basecolor, fadecolor);
	fadecolor[3] = 0.8f;

	fadecolor[3] *= 1.0f - cg.bar_offset;
	ammocolor[3] = fadecolor[3];

	fadecolor[3] *= basecolor[3];

	trap_R_SetColor(fadecolor);
	CG_DrawPic(295, HUD_B_Y + HUD_B_MIDDLE_OFFSET_Y, 50, 50, cgs.media.hud_bar_middle_middle);
	trap_R_SetColor(NULL);


	offset = cg.bar_count * HUD_BAR_EXTSIZE * cg.bar_offset;
	offset2 = 25 * cg.bar_offset;

	trap_R_SetColor(basecolor);
	CG_DrawPic(201 - offset, HUD_B_Y + HUD_B_MIDDLE_OFFSET_Y, 34, 50, cgs.media.hud_bar_middle_left_end);
	CG_DrawPic(201 - offset + 34, HUD_B_Y + HUD_B_MIDDLE_OFFSET_Y, (276 + offset2) - (201 - offset + 34), 50,
			   cgs.media.hud_bar_middle_left_middle);
	CG_DrawPic(276 + offset2, HUD_B_Y + HUD_B_MIDDLE_OFFSET_Y, 19, 50, cgs.media.hud_bar_middle_left_right);

	CG_DrawPic(345 - offset2, HUD_B_Y + HUD_B_MIDDLE_OFFSET_Y, 19, 50, cgs.media.hud_bar_middle_right_left);
	CG_DrawPic(345 - offset2 + 19, HUD_B_Y + HUD_B_MIDDLE_OFFSET_Y, (405 + offset) - (345 - offset2 + 19), 50,
			   cgs.media.hud_bar_middle_right_middle);
	CG_DrawPic(405 + offset, HUD_B_Y + HUD_B_MIDDLE_OFFSET_Y, 34, 50, cgs.media.hud_bar_middle_right_end);
	trap_R_SetColor(NULL);


	// ammo - TODO
	value = ps->ammo[cent->currentState.weapon];

	s = va("%i", value);
	fontsize = HUD_STATSIZE;
	CG_DrawHudString(254, 453, s, fontsize, UI_CENTER, ammocolor);

	trap_R_SetColor(NULL);

	trap_R_SetColor(colorOverlay);
	CG_DrawPic(201, HUD_B_Y + HUD_B_MIDDLE_OFFSET_Y, 238, 50, cgs.media.hud_bar_middle_overlay);
	trap_R_SetColor(NULL);

	// right, armor
	value = ps->stats[STAT_ARMOR];

	if(value <= 0)
		return;

	trap_R_SetColor(basecolor);
	CG_DrawPic(510 - HUD_B_BORDEROFFSET, HUD_B_Y, 130, 50, cgs.media.hud_bar_right);
	trap_R_SetColor(NULL);

	armorcolor[3] = 1.0f;

	s = va("%i", value);
	fontsize = HUD_STATSIZE;
	CG_DrawHudString(560, 453, s, fontsize, UI_CENTER, armorcolor);


	trap_R_SetColor(colorOverlay);
	CG_DrawPic(510 - HUD_B_BORDEROFFSET, HUD_B_Y, 130, 50, cgs.media.hud_bar_right_overlay);
	trap_R_SetColor(NULL);

	VectorCopy4(basecolor, color);
	color[3] = 0.75f + 0.25f * sin(cg.time / 300.0f);

	trap_R_SetColor(color);
	CG_DrawPic(610 - 17 - arflash / 2, 435 - arflash / 2, 30 + arflash, 30 + arflash, cgs.media.hud_icon_armor);

	// support harvester skulls
	if(cgs.gametype == GT_HARVESTER)
	{
#if 1
		vec3_t          angles;
		vec3_t          origin;
		qhandle_t       model;
		qhandle_t       skin;
		qhandle_t       icon;

		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;

		angles[YAW] = (cg.time & 2047) * 360 / 2048.0;

		model = cgs.media.harvesterSkullModel;
		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
			skin = cgs.media.blueSkullSkin;
		else
			skin = cgs.media.redSkullSkin;

		CG_Draw3DModel(640 - (TEXT_ICON_SPACE + ICON_SIZE), 416, ICON_SIZE, ICON_SIZE, model, skin, origin, angles);

		value = ps->generic1;
		if(value > 99)
			value = 99;

		trap_R_SetColor(basecolor);
		CG_DrawField(640 - (CHAR_WIDTH * 2 + TEXT_ICON_SPACE + ICON_SIZE), 445, 2, value, 1.0f);
		trap_R_SetColor(NULL);

		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if(!cg_draw3dIcons.integer && cg_drawIcons.integer)
		{
			if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
				icon = cgs.media.redSkullIcon;
			else
				icon = cgs.media.blueSkullIcon;

			CG_DrawPic(640 - (TEXT_ICON_SPACE + ICON_SIZE), 445, ICON_SIZE, ICON_SIZE, icon);
		}
#endif
	}

	trap_R_SetColor(NULL);
}

/*
================
CG_DrawStatusBar
================
*/
static void CG_DrawStatusBar(void)
{
	if(cg_drawStatus.integer == 1)
		CG_DrawStatusBarXreaL();
	else if(cg_drawStatus.integer == 2)
		CG_DrawStatusBarQ3();
	else if(cg_drawStatus.integer == 3)
		CG_DrawStatusBarNew();
	else
		return;
}

/*
================
CG_DrawSideBarItem
================
*/
static void CG_DrawSideBarItem(int x, int y, int i)
{
	char           *ammo;
	vec4_t          colorEmpty = { 1.0f, 0.0f, 0.0f, 0.7f };	// red
	vec4_t          colorInActive = { 1.0f, 1.0f, 1.0f, 0.7f };
	vec4_t          colorActive = { 0.25f, 1.0f, 0.25f, 0.8f };
	vec4_t          basecolor;


	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		VectorCopy4(blueTeamColor, basecolor);
	else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		VectorCopy4(redTeamColor, basecolor);
	else
		VectorCopy4(baseTeamColor, basecolor);


	CG_RegisterWeapon(i);

	ammo = va("%i", cg.snap->ps.ammo[i]);

	if(i == cg.weaponSelect)
		basecolor[3] = 0.9f;
	else
		basecolor[3] = 0.5f;

	trap_R_SetColor(basecolor);

	if(i == cg.weaponSelect)
		CG_DrawPic(x, y, 72, 32, cgs.media.sideBarItemSelectShader);

	CG_DrawPic(x, y, 72, 32, cgs.media.sideBarItemShader);

	trap_R_SetColor(NULL);

	CG_DrawPic(x + 4, y + 4, 24, 24, cg_weapons[i].weaponIcon);

	if(i == cg.weaponSelect)
	{
		CG_DrawHudString(x + 72, y + 17, ammo, 0.35f, UI_RIGHT, colorActive);
	}
	else
	{

		if(cg.snap->ps.ammo[i])
			CG_DrawHudString(x + 72, y + 17, ammo, 0.30f, UI_RIGHT, colorInActive);
		else
			CG_DrawHudString(x + 72, y + 17, ammo, 0.30f, UI_RIGHT, colorEmpty);
	}


}


/*
================
CG_DrawSideBarPowerup
================
*/
static void CG_DrawSideBarPowerup(int x, int y, int i)
{
	char           *time;
	int             t;
	vec4_t          colorActive = { 1.0f, 1.0f, 1.0f, 0.7f };	// white
	vec4_t          colorOver = { 1.0f, 0.0f, 0.0f, 0.7f };	// red
	vec4_t          basecolor;
	gitem_t        *item;

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		VectorCopy4(blueTeamColor, basecolor);
	else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		VectorCopy4(redTeamColor, basecolor);
	else
		VectorCopy4(baseTeamColor, basecolor);

	item = BG_FindItemForPowerup(i);

	t = (int)((cg.snap->ps.powerups[i] - cg.time) / 1000);

	time = va("%i", t);

	if(!item)
		return;

	trap_R_SetColor(basecolor);
	CG_DrawPic(x - 72, y, 72, 32, cgs.media.sideBarPowerupShader);
	trap_R_SetColor(NULL);

	if(t < 10)
		CG_DrawHudString(x - 60, y + 16, time, 0.30f + (10 - t) * 0.05f, UI_LEFT, colorOver);
	else
		CG_DrawHudString(x - 60, y + 17, time, 0.30f, UI_LEFT, colorActive);

	CG_DrawPic(x + 4 - 24 - 8, y + 4, 24, 24, trap_R_RegisterShader(item->icon));

}

static void CG_DrawSideBarHoldable(int x, int y, int i)
{

	vec4_t          basecolor;
	int             value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];

	if(!value)
		return;

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		VectorCopy4(blueTeamColor, basecolor);
	else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		VectorCopy4(redTeamColor, basecolor);
	else
		VectorCopy4(baseTeamColor, basecolor);

	trap_R_SetColor(basecolor);
	CG_DrawPic(x - 72, y, 72, 32, cgs.media.sideBarPowerupShader);
	trap_R_SetColor(NULL);

	CG_RegisterItemVisuals(value);
	CG_DrawPic(x + 4 - 24 - 8, y + 4, 24, 24, cg_items[value].icon);


}

/*
================
CG_DrawSideBar
================
*/
static void CG_DrawSideBar(void)
{
	int             x, y;
	int             i;
	int             bits;
	int             count;

	// do not show if the player is dead
	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		return;
	}
	if(cg_drawSideBar.integer != 0)
	{
		// count the number of weapons owned
		bits = cg.snap->ps.stats[STAT_WEAPONS];
		count = 0;
		for(i = 1; i < 16; i++)
		{
			if(bits & (1 << i))
			{
				count++;
			}
		}

		x = 0;
		y = 240 - count * 20 + 32;

		// do not count the gauntlet
		for(i = 2; i < 16; i++)
		{
			if(!(bits & (1 << i)))
			{
				continue;
			}

			CG_DrawSideBarItem(x, y, i);
			y += 32;
		}
	}

	// count the number of powerups owned
	count = 0;
	for(i = 1; i < MAX_POWERUPS; i++)
	{
		if(cg.snap->ps.powerups[i])
		{
			count++;
		}
	}

	x = 640;
	y = 200 - count * 20 + 32;

	//draw powerups
	for(i = 0; i < MAX_POWERUPS; i++)
	{
		if(!cg.snap->ps.powerups[i])
		{
			continue;
		}

		CG_DrawSideBarPowerup(x, y, i);
		y += 32;

	}

	//draw holdable
	CG_DrawSideBarHoldable(x, y, i);

}


/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker(float y)
{
	int             t;
	float           size;
	vec3_t          angles;
	const char     *info;
	const char     *name;
	int             clientNum;

	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		return y;
	}

	if(!cg.attackerTime)
	{
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if(clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum)
	{
		return y;
	}

	t = cg.time - cg.attackerTime;
	if(t > ATTACKER_HEAD_TIME)
	{
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead(640 - size, y, size, size, clientNum, angles);

	info = CG_ConfigString(CS_PLAYERS + clientNum);
	name = Info_ValueForKey(info, "n");
	y += size;
	CG_DrawBigString(640 - (Q_PrintStrlen(name) * BIGCHAR_WIDTH), y, name, 0.5);

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot(float y)
{
	char           *s;

	s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime, cg.latestSnapshotNum, cgs.serverCommandSequence);

	CG_Text_PaintAligned(635, y + 4, s, 0.2f, UI_RIGHT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);


	return y + 16;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES 40
static float CG_DrawFPS(float y)
{
	char           *s;
	static int      previousTimes[FPS_FRAMES];
	static int      index;
	int             i, total;
	int             fps;
	static int      previous;
	int             t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;

	if(index > FPS_FRAMES)
	{
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for(i = 0; i < FPS_FRAMES; i++)
		{
			total += previousTimes[i];
		}
		if(!total)
		{
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va("%ifps", fps);


		CG_Text_PaintAligned(635, y, s, 0.25f, UI_RIGHT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);
	}

	return y + 16;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer(float y)
{
	char           *s;
	int             mins, seconds, tens;
	int             msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va("%i:%i%i", mins, tens, seconds);
	//w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	//CG_DrawBigString(635 - w, y + 2, s, 1.0F);
	CG_Text_PaintAligned(635, y, s, 0.25f, UI_RIGHT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	return y + 16;

}


/*
=================
CG_DrawTeamOverlay
=================
*/
static float CG_DrawTeamOverlay(float y, qboolean right, qboolean upper)
{
	float           x, w, h, xx;
	int             i, j, len;
	const char     *p;
	vec4_t          hcolor;
	int             pwidth, lwidth;
	int             plyrs;
	char            st[16];
	clientInfo_t   *ci;
	gitem_t        *item;
	int             ret_y, count;

	if(!cg_drawTeamOverlay.integer)
	{
		return y;
	}

	if(cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE)
	{
		return y;				// Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for(i = 0; i < count; i++)
	{
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if(ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM])
		{
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if(len > pwidth)
				pwidth = len;
		}
	}

	if(!plyrs)
		return y;

	if(pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for(i = 1; i < MAX_LOCATIONS; i++)
	{
		p = CG_ConfigString(CS_LOCATIONS + i);
		if(p && *p)
		{
			len = CG_DrawStrlen(p);
			if(len > lwidth)
				lwidth = len;
		}
	}

	if(lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if(right)
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if(upper)
	{
		ret_y = y + h;
	}
	else
	{
		y -= h;
		ret_y = y;
	}

	CG_AdjustFrom640(&x, &y, &w, &h);

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
	{
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	}
	else
	{							// if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor(hcolor);
	CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
	trap_R_SetColor(NULL);

	for(i = 0; i < count; i++)
	{
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if(ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM])
		{

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt(xx, y,
							 ci->name, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if(lwidth)
			{
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if(!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);
				if(len > lwidth)
					len = lwidth;

				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt(xx, y,
								 p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth(ci->health, ci->armor, hcolor);

			Com_sprintf(st, sizeof(st), "%3i %3i", ci->health, ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 + TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt(xx, y, st, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if(cg_weapons[ci->curWeapon].weaponIcon)
			{
				CG_DrawPic(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, cg_weapons[ci->curWeapon].weaponIcon);
			}
			else
			{
				CG_DrawPic(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, cgs.media.deferShader);
			}

			// Draw powerup icons
			if(right)
			{
				xx = x;
			}
			else
			{
				xx = x + w - TINYCHAR_WIDTH;
			}
			for(j = 0; j <= PW_NUM_POWERUPS; j++)
			{
				if(ci->powerups & (1 << j))
				{

					item = BG_FindItemForPowerup(j);

					if(item)
					{
						CG_DrawPic(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, trap_R_RegisterShader(item->icon));
						if(right)
						{
							xx -= TINYCHAR_WIDTH;
						}
						else
						{
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
}


/*
=====================
CG_DrawUpperRight

=====================
*/

static void CG_DrawUpperRight(void)
{
	float           y;

	y = 10;

	if(cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1)
	{
		y = CG_DrawTeamOverlay(y, qtrue, qtrue);
	}
	if(cg_drawSnapshot.integer)
	{
		y = CG_DrawSnapshot(y);
	}
	if(cg_drawFPS.integer)
	{
		y = CG_DrawFPS(y);
	}
	if(cg_drawTimer.integer)
	{
		y = CG_DrawTimer(y);
	}
	if(cg_drawAttacker.integer)
	{
		y = CG_DrawAttacker(y);
	}

}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
static float CG_DrawScores(float y)
{
	const char     *s;
	int             s1, s2, score;
	int             x, w;
	int             v;
	vec4_t          color;
	float           y1;
	gitem_t        *item;

	if(cg_drawStatus.integer == 3)
		return y;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -= BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	if(cgs.gametype >= GT_TEAM)
	{
		x = 640;
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va("%2i", s2);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		{
			CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
		}
		CG_DrawBigString(x + 4, y, s, 1.0F);

		if(cgs.gametype == GT_CTF)
		{
			// Display flag status
			item = BG_FindItemForPowerup(PW_BLUEFLAG);

			if(item)
			{
				y1 = y - BIGCHAR_HEIGHT - 8;
				if(cgs.blueflag >= 0 && cgs.blueflag <= 2)
				{
					CG_DrawPic(x, y1 - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.blueFlagShader[cgs.blueflag]);
				}
			}
		}
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va("%2i", s1);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		{
			CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
		}
		CG_DrawBigString(x + 4, y, s, 1.0F);

		if(cgs.gametype == GT_CTF)
		{
			// Display flag status
			item = BG_FindItemForPowerup(PW_REDFLAG);

			if(item)
			{
				y1 = y - BIGCHAR_HEIGHT - 8;
				if(cgs.redflag >= 0 && cgs.redflag <= 2)
				{
					CG_DrawPic(x, y1 - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.redFlagShader[cgs.redflag]);
				}
			}
		}

		if(cgs.gametype == GT_1FCTF)
		{
			// Display flag status
			item = BG_FindItemForPowerup(PW_NEUTRALFLAG);

			if(item)
			{
				y1 = y - BIGCHAR_HEIGHT - 8;
				if(cgs.flagStatus >= 0 && cgs.flagStatus <= 3)
				{
					CG_DrawPic(x, y1 - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.flagShader[cgs.flagStatus]);
				}
			}
		}

		if(cgs.gametype >= GT_CTF)
		{
			v = cgs.capturelimit;
		}
		else
		{
			v = cgs.fraglimit;
		}
		if(v)
		{
			s = va("%2i", v);
			w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

	}
	else
	{
		qboolean        spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR);

		// always show your score in the second box if not in first place
		if(s1 != score)
		{
			s2 = score;
		}
		if(s2 != SCORE_NOT_PRESENT)
		{
			s = va("%2i", s2);
			w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x -= w;
			if(!spectator && score == s2 && score != s1)
			{
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
				CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
			}
			else
			{
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
			}
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

		// first place
		if(s1 != SCORE_NOT_PRESENT)
		{
			s = va("%2i", s1);
			w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x -= w;
			if(!spectator && score == s1)
			{
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
				CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
			}
			else
			{
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color);
			}
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

		if(cgs.fraglimit)
		{
			s = va("%2i", cgs.fraglimit);
			w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString(x + 4, y, s, 1.0F);
		}

	}

	return y1 - 8;
}

/*
================
CG_DrawPowerups
================
*/
#ifndef MISSIONPACK
void CG_DrawPowerups(void)
{
/*	int             sorted[MAX_POWERUPS];
	int             sortedTime[MAX_POWERUPS];
	int             i, j, k;
	int             active;
	playerState_t  *ps;
	int             t;
	gitem_t        *item;
	int             x, y;
	int             color;
	float           size;
	float           f;
	static float    colors[2][4] = {
		{0.2f, 1.0f, 0.2f, 1.0f},
		{1.0f, 0.2f, 0.2f, 1.0f}
	};

	ps = &cg.snap->ps;

	if(ps->stats[STAT_HEALTH] <= 0)
	{
		return ;
	}

	// sort the list by time remaining
	active = 0;
	for(i = 0; i < MAX_POWERUPS; i++)
	{
		if(!ps->powerups[i])
		{
			continue;
		}
		t = ps->powerups[i] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if(t < 0 || t > 999000)
		{
			continue;
		}

		// insert into the list
		for(j = 0; j < active; j++)
		{
			if(sortedTime[j] >= t)
			{
				for(k = active - 1; k >= j; k--)
				{
					sorted[k + 1] = sorted[k];
					sortedTime[k + 1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	y = 240;


	for(i = 0; i < active; i++)
	{
		item = BG_FindItemForPowerup(sorted[i]);
gitem_t        *item;
		if(item)
		{

			color = 1;

			y -= ICON_SIZE;

			trap_R_SetColor(colors[color]);
			CG_DrawField(x, y, 2, sortedTime[i] / 1000, 1.0f);

			t = ps->powerups[sorted[i]];
			if(t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME)
			{
				trap_R_SetColor(NULL);
			}
			else
			{
				vec4_t          modulate;

				f = (float)(t - cg.time) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor(modulate);
			}

			if(cg.powerupActive == sorted[i] && cg.time - cg.powerupTime < PULSE_TIME)
			{
				f = 1.0 - (((float)cg.time - cg.powerupTime) / PULSE_TIME);
				size = ICON_SIZE * (1.0 + (PULSE_SCALE - 1.0) * f);
			}
			else
			{
				size = ICON_SIZE;
			}

			CG_DrawPic(640 - size, y + ICON_SIZE / 2 - size / 2, size, size, trap_R_RegisterShader(item->icon));
		}
	}
	trap_R_SetColor(NULL);

*/
}
#endif							// MISSIONPACK

/*
=====================
CG_DrawLowerRight
=====================
*/
static void CG_DrawLowerRight(void)
{
	float           y;

	y = 480 - 88;				// offset above lagometer

	if(cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2)
	{
		y = CG_DrawTeamOverlay(y, qtrue, qfalse);
	}

	y = CG_DrawScores(y);
//	y = CG_DrawPowerups(y);
}

/*
===================
CG_DrawPickupItem
===================
*/
static int CG_DrawPickupItem(int y)
{
	int             value;
	float          *fadeColor;

	if(!cg_drawPickupItem.integer)
		return y;

	if(cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		return y;
	}

	y -= ICON_SIZE * 2 + 15;

	value = cg.itemPickup;
	if(value)
	{
		fadeColor = CG_FadeColor(cg.itemPickupTime, 3000);
		if(fadeColor)
		{
			CG_RegisterItemVisuals(value);
			trap_R_SetColor(fadeColor);
			CG_DrawPic(8, y, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
			CG_Text_PaintAligned(ICON_SIZE + 16, y + (ICON_SIZE / 2), bg_itemlist[value].pickup_name, 0.4f,
								 UI_LEFT | UI_DROPSHADOW, fadeColor, &cgs.media.freeSansBoldFont);
			trap_R_SetColor(NULL);
		}
	}

	return y;
}

/*
=====================
CG_DrawLowerLeft
=====================
*/
static void CG_DrawLowerLeft(void)
{
	float           y;

	y = 480 - ICON_SIZE;

	if(cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3)
	{
		y = CG_DrawTeamOverlay(y, qfalse, qfalse);
	}

	y = CG_DrawPickupItem(y);
}


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
static void CG_DrawTeamInfo(void)
{
	int             w, h;
	int             i, len;
	vec4_t          hcolor;
	int             chatHeight;

#define CHATLOC_Y 420			// bottom end
#define CHATLOC_X 0

	if(cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if(chatHeight <= 0)
		return;					// disabled

	if(cgs.teamLastChatPos != cgs.teamChatPos)
	{
		if(cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer)
		{
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		w = 0;

		for(i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++)
		{
			len = CG_DrawStrlen(cgs.teamChatMsgs[i % chatHeight]);
			if(len > w)
				w = len;
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		{
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}
		else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		{
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		}
		else
		{
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor(hcolor);
		CG_DrawPic(CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar);
		trap_R_SetColor(NULL);

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for(i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--)
		{
			CG_DrawStringExt(CHATLOC_X + TINYCHAR_WIDTH,
							 CHATLOC_Y - (cgs.teamChatPos - i) * TINYCHAR_HEIGHT,
							 cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
	}
}

/*
===================
CG_DrawHoldableItem
===================
*/
/*static void CG_DrawHoldableItem(void)
{
	int             value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if(value)
	{
		CG_RegisterItemVisuals(value);
		CG_DrawPic(640 - ICON_SIZE, (SCREEN_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
	}

}
*/

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward(void)
{
	float          *color;
	int             i, count;
	float           x, y;
	char            buf[32];

	if(!cg_drawRewards.integer)
	{
		return;
	}

	color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
	if(!color)
	{
		if(cg.rewardStack > 0)
		{
			for(i = 0; i < cg.rewardStack; i++)
			{
				cg.rewardSound[i] = cg.rewardSound[i + 1];
				cg.rewardShader[i] = cg.rewardShader[i + 1];
				cg.rewardCount[i] = cg.rewardCount[i + 1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor(cg.rewardTime, REWARD_TIME);
			trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		}
		else
		{
			return;
		}
	}

	trap_R_SetColor(color);

	if(cg.rewardCount[0] >= 10)
	{
		y = 56;
		x = 320 - ICON_SIZE / 2;
		CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = (SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen(buf)) / 2;
		CG_DrawStringExt(x, y + ICON_SIZE, buf, color, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
	}
	else
	{

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * ICON_SIZE / 2;
		for(i = 0; i < count; i++)
		{
			CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor(NULL);
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct
{
	int             frameSamples[LAG_SAMPLES];
	int             frameCount;
	int             snapshotFlags[LAG_SAMPLES];
	int             snapshotSamples[LAG_SAMPLES];
	int             snapshotCount;
} lagometer_t;

lagometer_t     lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo(void)
{
	int             offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo(snapshot_t * snap)
{
	// dropped packet
	if(!snap)
	{
		lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->ping;
	lagometer.snapshotFlags[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect(void)
{
	float           x, y;
	int             cmdNum;
	usercmd_t       cmd;
	const char     *s;
	int             w;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if(cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time)
	{
		// special check for map_restart
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_Text_Width(s, 0.5f, 0, &cgs.media.freeSansBoldFont);
	CG_Text_Paint(320 - w / 2, 100, 0.5f, colorRed, s, 0, 0, UI_DROPSHADOW, &cgs.media.freeSansBoldFont);

	//otty: readjusted lagometer
	x = 640 - 68;
	y = 480 - 120;

	trap_R_SetColor(baseTeamColor);
	CG_DrawPic(x - 8, y - 8, 48 + 16, 48 + 16, cgs.media.lagometer_lagShader);
	trap_R_SetColor(NULL);

	// blink the icon
	if((cg.time >> 9) & 1)
	{
		return;
	}

	CG_DrawPic(x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga"));
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer(void)
{
	int             a, x, y, i;
	float           v;
	float           ax, ay, aw, ah, mid, range;
	int             color;
	float           vscale;
	qboolean        lag = qfalse;
	vec4_t          basecolor;
	vec4_t          fadecolor;

	playerState_t  *ps;
	centity_t      *cent;

	ps = &cg.snap->ps;
	cent = &cg_entities[cg.snap->ps.clientNum];

	// Tr3B: even draw the lagometer when connected to a local server
	if(!cg_lagometer.integer /*|| cgs.localServer */ )
	{
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
/*#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 90;
#endif
*/
	//otty: readjusted lagometer
	x = 640 - 68;
	y = 480 - 120;

	if(ps->persistant[PERS_TEAM] == TEAM_BLUE)
		VectorCopy4(blueTeamColor, basecolor);
	else if(ps->persistant[PERS_TEAM] == TEAM_RED)
		VectorCopy4(redTeamColor, basecolor);
	else
		VectorCopy4(baseTeamColor, basecolor);


	trap_R_SetColor(basecolor);
	CG_DrawPic(x - 8, y - 8, 48 + 16, 48 + 16, cgs.media.lagometerShader);
	trap_R_SetColor(NULL);


	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for(a = 0; a < aw; a++)
	{
		i = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if(v > 0)
		{
			if(color != 1)
			{
				color = 1;
				VectorCopy4(g_color_table[ColorIndex(COLOR_YELLOW)], fadecolor);
				fadecolor[3] = (float)((aw - a) / aw);
				trap_R_SetColor(fadecolor);
			}
			if(v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if(v < 0)
		{
			if(color != 2)
			{
				color = 2;
				VectorCopy4(g_color_table[ColorIndex(COLOR_BLUE)], fadecolor);
				fadecolor[3] = (float)((aw - a) / aw);
				trap_R_SetColor(fadecolor);

			}
			v = -v;
			if(v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for(a = 0; a < aw; a++)
	{
		i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if(v > 0)
		{
			if(lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED)
			{
				if(color != 5)
				{
					color = 5;	// YELLOW for rate delay
					VectorCopy4(g_color_table[ColorIndex(COLOR_YELLOW)], fadecolor);
					fadecolor[3] = (float)((aw - a) / aw);
					trap_R_SetColor(fadecolor);
				}
			}
			else
			{
				if(color != 3)
				{
					color = 3;
					VectorCopy4(g_color_table[ColorIndex(COLOR_GREEN)], fadecolor);
					fadecolor[3] = (float)((aw - a) / aw) * 0.5f;
					trap_R_SetColor(fadecolor);
				}
			}
			v = v * vscale;
			if(v > range)
			{
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
		else if(v < 0)
		{
			if(color != 4)
			{
				color = 4;		// RED for dropped snapshots
				VectorCopy4(g_color_table[ColorIndex(COLOR_RED)], fadecolor);
				//fadecolor[3] = (float)((aw - a) / aw);
				fadecolor[3] = 1.0f;
				trap_R_SetColor(fadecolor);
			}
			if(ah - range > 10)
				lag = qtrue;

			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}
/*
	if(lag){
		trap_R_SetColor(basecolor);
		CG_DrawPic(x-8, y-8, 48+16, 48+16, cgs.media.lagometer_lagShader);
		trap_R_SetColor(NULL);
	}
*/
	trap_R_SetColor(NULL);

	if(cg_nopredict.integer || cg_synchronousClients.integer)
	{
		CG_Text_Paint(ax, ay, 0.4f, colorRed, "snc", 0, 0, UI_CENTER | UI_DROPSHADOW, &cgs.media.freeSansBoldFont);
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint(const char *str, int y, int charWidth)
{
	char           *s;

	Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while(*s)
	{
		if(*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString(void)
{
	char           *start;
	int             l;
	int             x, y, w, h;
	float          *color;

	if(!cg.centerPrintTime)
	{
		return;
	}

	color = CG_FadeColor(cg.centerPrintTime, 1000 * cg_centertime.value);
	if(!color)
	{
		return;
	}

	trap_R_SetColor(color);

	color[3] *= 0.75f;

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while(1)
	{
		char            linebuffer[1024];

		for(l = 0; l < 50; l++)
		{
			if(!start[l] || start[l] == '\n')
			{
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = CG_Text_Width(linebuffer, 0.4f, 0, &cgs.media.freeSansBoldFont);
		h = CG_Text_Height(linebuffer, 0.4f, 0, &cgs.media.freeSansBoldFont);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, 0.4f, color, linebuffer, 0, 0, UI_CENTER | UI_DROPSHADOW, &cgs.media.freeSansBoldFont);
		y += h + 6;

		while(*start && (*start != '\n'))
		{
			start++;
		}
		if(!*start)
		{
			break;
		}
		start++;
	}

	trap_R_SetColor(NULL);
}



/*
================================================================================

CROSSHAIR

================================================================================
*/

void CG_DrawCrosshairNew(void)
{
	qhandle_t       dot;
	qhandle_t       circle;
	qhandle_t       cross;

	float           w, h;
	float           x, y;
	float           f;


	if(cg_crosshairDot.integer <= 0)	// no dot
	{
		dot = 0;
	}
	else
	{
		dot = cgs.media.crosshairDot[cg_crosshairDot.integer - 1];
	}

	if(cg_crosshairCircle.integer <= 0)	// no circle
	{
		circle = 0;
	}
	else
	{
		circle = cgs.media.crosshairCircle[cg_crosshairCircle.integer - 1];
	}

	if(cg_crosshairCross.integer <= 0)	// no cross
	{
		cross = 0;
	}
	else
	{
		cross = cgs.media.crosshairCross[cg_crosshairCross.integer - 1];
	}

	w = h = cg_crosshairSize.value;

	if(cg_crosshairPulse.integer == 1)	// pulse the size of the crosshair when picking up items
	{
		f = cg.time - cg.itemPickupBlendTime;

		if(f > 0 && f < ITEM_BLOB_TIME)
		{
			f /= ITEM_BLOB_TIME;
			w *= (1 + f);
			h *= (1 + f);
		}

	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;

	CG_AdjustFrom640(&x, &y, &w, &h);

	// set color based on health
	if(cg_crosshairHealth.integer)
	{
		vec4_t          hcolor;

		CG_ColorForHealth(hcolor);
		trap_R_SetColor(hcolor);
	}
	else
	{
		trap_R_SetColor(NULL);
	}


	if(dot)
		trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - w),
							  y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h, 0, 0, 1, 1, dot);
	if(circle)
		trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - w),
							  y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h, 0, 0, 1, 1, circle);
	if(cross)
		trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - w),
							  y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h, 0, 0, 1, 1, cross);



	trap_R_SetColor(NULL);

}


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void)
{
	float           w, h;
	qhandle_t       hShader;
	float           f;
	float           x, y;
	int             ca;

	if(!cg_drawCrosshair.integer)
	{
		return;
	}

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if(cg.renderingThirdPerson)
	{
		return;
	}

	if(cg_drawStatus.integer == 3)
	{
		CG_DrawCrosshairNew();
		return;
	}
	// set color based on health
	if(cg_crosshairHealth.integer)
	{
		vec4_t          hcolor;

		CG_ColorForHealth(hcolor);
		trap_R_SetColor(hcolor);
	}
	else
	{
		trap_R_SetColor(NULL);
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if(f > 0 && f < ITEM_BLOB_TIME)
	{
		f /= ITEM_BLOB_TIME;
		w *= (1 + f);
		h *= (1 + f);
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640(&x, &y, &w, &h);

	ca = cg_drawCrosshair.integer;
	if(ca < 0)
	{
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ca % NUM_CROSSHAIRS];

	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - w),
						  y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h, 0, 0, 1, 1, hShader);
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity(void)
{
	trace_t         trace;
	vec3_t          start, end;
	int             content;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY);
	if(trace.entityNum >= MAX_CLIENTS)
	{
		return;
	}

	// if the player is in fog, don't show it
	content = trap_CM_PointContents(trace.endpos, 0);
	if(content & CONTENTS_FOG)
	{
		return;
	}

	// if the player is invisible, don't show it
	if(cg_entities[trace.entityNum].currentState.powerups & (1 << PW_INVIS))
	{
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames(void)
{
	float          *color;
	char           *name;
	float           w;

	if(!cg_drawCrosshair.integer)
	{
		return;
	}
	if(!cg_drawCrosshairNames.integer)
	{
		return;
	}
	if(cg.renderingThirdPerson)
	{
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor(cg.crosshairClientTime, 1000);
	if(!color)
	{
		trap_R_SetColor(NULL);
		return;
	}

	name = cgs.clientinfo[cg.crosshairClientNum].name;

	if(cg_drawStatus.integer == 3)
	{
		//think this is a better place

		color[3] *= 0.85f;
		w = CG_Text_Width(name, 0.2f, 0, &cgs.media.freeSansBoldFont);
		CG_Text_Paint(320 - w / 2, 265, 0.2f, color, name, 0, 0, 0, &cgs.media.freeSansBoldFont);


		trap_R_SetColor(NULL);
		return;
	}

	color[3] *= 0.5f;
	w = CG_Text_Width(name, 0.3f, 0, &cgs.media.freeSansBoldFont);
	CG_Text_Paint(320 - w / 2, 190, 0.3f, color, name, 0, 0, UI_DROPSHADOW, &cgs.media.freeSansBoldFont);

	trap_R_SetColor(NULL);
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void)
{
	CG_Text_PaintAligned(320, 440, "SPECTATOR", 0.45f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	if(cgs.gametype == GT_TOURNAMENT)
	{
		CG_Text_PaintAligned(320, 460, "waiting to play", 0.25f, UI_CENTER | UI_DROPSHADOW, colorWhite,
							 &cgs.media.freeSansBoldFont);
	}
	else if(cgs.gametype >= GT_TEAM)
	{
		CG_Text_PaintAligned(320, 460, "press ESC and use the JOIN menu to play", 0.25f, UI_CENTER | UI_DROPSHADOW, colorWhite,
							 &cgs.media.freeSansBoldFont);
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void)
{
	char           *s;
	int             sec;

	if(!cgs.voteTime)
	{
		return;
	}

	// play a talk beep whenever it is modified
	if(cgs.voteModified)
	{
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
	if(sec < 0)
	{
		sec = 0;
	}

	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString(0, 58, s, 1.0F);
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void)
{
	char           *s;
	int             sec, cs_offset;

	if(cgs.clientinfo->team == TEAM_RED)
		cs_offset = 0;
	else if(cgs.clientinfo->team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if(!cgs.teamVoteTime[cs_offset])
	{
		return;
	}

	// play a talk beep whenever it is modified
	if(cgs.teamVoteModified[cs_offset])
	{
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	}

	sec = (VOTE_TIME - (cg.time - cgs.teamVoteTime[cs_offset])) / 1000;
	if(sec < 0)
	{
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
		   cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset]);
	CG_DrawSmallString(0, 90, s, 1.0F);
}


static qboolean CG_DrawScoreboard(void)
{
	if(cg_drawStatus.integer == 3)
		return CG_DrawScoreboardNew();

	return CG_DrawOldScoreboard();
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission(void)
{
	if(cgs.gametype == GT_SINGLE_PLAYER)
	{
		CG_DrawCenterString();
		return;
	}

	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow(void)
{
	const char     *name;

	if(!(cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		return qfalse;
	}

	CG_Text_PaintAligned(320, 64, "following", 0.35f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	name = cgs.clientinfo[cg.snap->ps.clientNum].name;
	CG_Text_PaintAligned(320, 90, name, 0.45f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	return qtrue;
}

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning(void)
{

	if(cg_drawAmmoWarning.integer == 0)
	{
		return;
	}

	if(!cg.lowAmmoWarning)
	{
		return;
	}

	if(cg.lowAmmoWarning == 2)
	{
		CG_Text_PaintAligned(320, 400, "out of ammo", 0.4f, UI_CENTER | UI_DROPSHADOW, colorRed, &cgs.media.freeSansBoldFont);
	}
	else
	{

		CG_Text_PaintAligned(320, 400, "ammo low", 0.4f, UI_CENTER | UI_DROPSHADOW, colorYellow, &cgs.media.freeSansBoldFont);
	}
}

/*
=================
CG_DrawProxWarning
=================
*/
static void CG_DrawProxWarning(void)
{
	char            s[32];
	int             w;
	static int      proxTime;
	static int      proxCounter;
	static int      proxTick;

	if(!(cg.snap->ps.eFlags & EF_TICKING))
	{
		proxTime = 0;
		return;
	}

	if(proxTime == 0)
	{
		proxTime = cg.time + 5000;
		proxCounter = 5;
		proxTick = 0;
	}

	if(cg.time > proxTime)
	{
		proxTick = proxCounter--;
		proxTime = cg.time + 1000;
	}

	if(proxTick != 0)
	{
		Com_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
	}
	else
	{
		Com_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");
	}

	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor(320 - w / 2, 64 + BIGCHAR_HEIGHT, s, (float *)g_color_table[ColorIndex(COLOR_RED)]);
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup(void)
{
	int             sec;
	int             i;
	float           scale;
	clientInfo_t   *ci1, *ci2;
	int             cw;
	const char     *s;

	sec = cg.warmup;
	if(!sec)
	{
		return;
	}

	if(sec < 0)
	{
		s = "Waiting for players";
		CG_Text_PaintAligned(320, 64, s, 0.25f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);
		cg.warmupCount = 0;
		return;
	}

	if(cgs.gametype == GT_TOURNAMENT)
	{
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for(i = 0; i < cgs.maxclients; i++)
		{
			if(cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE)
			{
				if(!ci1)
				{
					ci1 = &cgs.clientinfo[i];
				}
				else
				{
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if(ci1 && ci2)
		{
			s = va("%s vs %s", ci1->name, ci2->name);

			CG_Text_PaintAligned(320, 64, s, 0.4f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);
		}
	}
	else
	{
		if(cgs.gametype == GT_FFA)
		{
			s = "Free For All";
		}
		else if(cgs.gametype == GT_TEAM)
		{
			s = "Team Deathmatch";
		}
		else if(cgs.gametype == GT_CTF)
		{
			s = "Capture the Flag";
		}
		else if(cgs.gametype == GT_1FCTF)
		{
			s = "One Flag CTF";
		}
		else if(cgs.gametype == GT_OBELISK)
		{
			s = "Overload";
		}
		else if(cgs.gametype == GT_HARVESTER)
		{
			s = "Harvester";
		}
		else
		{
			s = "";
		}

		CG_Text_PaintAligned(320, 90, s, 0.4f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);
	}

	sec = (sec - cg.time) / 1000;
	if(sec < 0)
	{
		cg.warmup = 0;
		sec = 0;
	}
	s = va("Starts in: %i", sec + 1);
	if(sec != cg.warmupCount)
	{
		cg.warmupCount = sec;
		switch (sec)
		{
			case 0:
				trap_S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
				break;
			case 1:
				trap_S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
				break;
			case 2:
				trap_S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
				break;
			default:
				break;
		}
	}
	scale = 0.45f;
	switch (cg.warmupCount)
	{
		case 0:
			cw = 28;
			scale = 0.34f;
			break;
		case 1:
			cw = 24;
			scale = 0.31f;
			break;
		case 2:
			cw = 20;
			scale = 0.28f;
			break;
		default:
			cw = 16;
			scale = 0.25f;
			break;
	}

	CG_Text_PaintAligned(320, 125, s, scale, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);
}

//==================================================================================



//************** otty debug  ***************//


/*
=================
CG_DrawDebug
=================
*/

static int debugModel(int x, int y)
{
	y += 12;

	CG_Text_Paint(x, y, 0.2f, colorRed, "Model:", 0.0f, 0, 0, &cgs.media.freeSansBoldFont);
	y += 10;

	CG_Text_Paint(x + 10, y, 0.15f, colorYellow, va("current anim: %i", debug_anim_current), 0.0f, 0, 0, &cgs.media.freeSansBoldFont);
	y += 8;
	CG_Text_Paint(x + 10, y, 0.15f, colorYellow, va("old anim: %i", debug_anim_old), 0.0f, 0, 0, &cgs.media.freeSansBoldFont);
	y += 8;
	CG_Text_Paint(x + 10, y, 0.15f, colorYellow, va("anim blend : %f", debug_anim_blend), 0.0f, 0, 0, &cgs.media.freeSansBoldFont);
	y += 8;


	return y;


}

int             lowPeak = 999;
int             highPeak = 0;

static int debugSystem(int x, int y)
{
	char           *s;
	int             mins, seconds, tens;
	int             msec;
	static int      previousTimes[FPS_FRAMES];
	static int      index;
	int             i, total;
	int             fps;
	static int      previous;
	int             t, frameTime;

	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;
	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;


	CG_Text_Paint(x, y, 0.2f, colorRed, "System:", 0.0f, 0, 0, &cgs.media.freeSansBoldFont);
	y += 10;


	if(index > FPS_FRAMES)
	{
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for(i = 0; i < FPS_FRAMES; i++)
		{
			total += previousTimes[i];
		}
		if(!total)
		{
			total = 1;
		}

		fps = 1000 * FPS_FRAMES / total;


		if(fps < lowPeak)
			lowPeak = fps;
		else if(fps > highPeak)
			highPeak = fps;

		s = va("Average FPS: %i ", fps);
		CG_Text_Paint(x + 10, y, 0.15f, colorYellow, s, 0.0f, 0, 0, &cgs.media.freeSansBoldFont);
		y += 8;

		s = va("HighPeak : %i ", highPeak);
		CG_Text_Paint(x + 10, y, 0.15f, colorYellow, s, 0.0f, 0, 0, &cgs.media.freeSansBoldFont);
		y += 8;

		s = va("LowPeak : %i ", lowPeak);
		CG_Text_Paint(x + 10, y, 0.15f, colorYellow, s, 0.0f, 0, 0, &cgs.media.freeSansBoldFont);
		y += 8;

	}


	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;


	s = va("Time : %i:%i%i", mins, tens, seconds);
	CG_Text_Paint(x + 10, y, 0.15f, colorYellow, s, 0.0f, 0, 0, &cgs.media.freeSansBoldFont);

	return y;
}

//==================================================================================

static void CG_DrawDebug(void)
{
	int             x = 5;
	int             y = 140;

	vec4_t          colorDebug = { 0.2f, 0.2f, 0.2f, 0.66f };

	CG_FillRect(x, y, 200, 180, colorDebug);
	x += 10;
	y += 10;


	y = debugSystem(x, y);
	y = debugModel(x, y);

}


/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(void)
{

	if(cg_debugHUD.integer == 1)
	{
		CG_DrawDebug();
		return;
	}

#ifdef MISSIONPACK
	if(cgs.orderPending && cg.time > cgs.orderTime)
	{
		CG_CheckOrderPending();
	}
#endif

	// if we are taking a levelshot for the menu, don't draw anything
	if(cg.levelShot)
	{
		return;
	}

	if(cg_draw2D.integer == 0)
	{
		return;
	}

	VectorSet4(baseTeamColor, cg_hudRed.value, cg_hudGreen.value, cg_hudBlue.value, cg_hudAlpha.value);

	redTeamColor[3] = cg_hudAlpha.value;
	blueTeamColor[3] = cg_hudAlpha.value;


	if(cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		CG_DrawIntermission();
		return;
	}

/*
	if (cg.cameraMode) {
		return;
	}
*/

	CG_DrawOSD();

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		CG_DrawSpectator();
		CG_DrawCrosshair();
		CG_DrawCrosshairNames();
	}
	else
	{
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if(!cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0)
		{
			CG_DrawStatusBar();

			CG_DrawSideBar();
			CG_DrawPowerups();

			CG_DrawAmmoWarning();

			CG_DrawProxWarning();

			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();

			//CG_DrawHoldableItem();

			CG_DrawReward();
		}

		if(cgs.gametype >= GT_TEAM)
		{
			CG_DrawTeamInfo();
		}
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();

	CG_DrawUpperRight();

	CG_DrawLowerRight();
	CG_DrawLowerLeft();

	if(!CG_DrawFollow())
	{
		CG_DrawWarmup();
	}

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if(!cg.scoreBoardShowing)
	{
		CG_DrawCenterString();
	}
}


static void CG_DrawTourneyScoreboard(void)
{
	CG_DrawOldTourneyScoreboard();
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive(stereoFrame_t stereoView)
{
	float           separation;
	vec3_t          baseOrg;

	// optionally draw the info screen instead
	if(!cg.snap)
	{
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && (cg.snap->ps.pm_flags & PMF_SCOREBOARD))
	{
		CG_DrawTourneyScoreboard();
		return;
	}

	switch (stereoView)
	{
		case STEREO_CENTER:
			separation = 0;
			break;
		case STEREO_LEFT:
			separation = -cg_stereoSeparation.value / 2;
			break;
		case STEREO_RIGHT:
			separation = cg_stereoSeparation.value / 2;
			break;
		default:
			separation = 0;
			CG_Error("CG_DrawActive: Undefined stereoView");
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy(cg.refdef.vieworg, baseOrg);
	if(separation != 0)
	{
		VectorMA(cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg);
	}

	// draw 3D view
	trap_R_RenderScene(&cg.refdef);

	// restore original viewpoint if running stereo
	if(separation != 0)
	{
		VectorCopy(baseOrg, cg.refdef.vieworg);
	}

	// draw status bar and other floating elements
	CG_Draw2D();
}
