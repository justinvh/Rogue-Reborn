/*
===========================================================================
Copyright (C) 2008 Adrian Fuhrmann

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
// cg_osd.c -- booth classic and dynamic on screen display for commands during play
#include <hat/client/cg_local.h>

#define OSD_RADIUS 100
#define OSD_RADIUS_BASE 75

osd_group_t     osdGroups[MAX_OSD_GROUPS];
int             numOSDGroups;

void CG_OSDUp_f(void)
{
	int             group;

	group = 0;
	cg.osd.curGroup = &osdGroups[group];
	cg.osd.curEntry = &cg.osd.curGroup->entrys[0];
	cg.osd.curGroup->wish_alpha = 1.0f;
	cg.osd.input = qtrue;

}
void CG_OSDDown_f(void)
{

	cg.osd.input = qfalse;


}

void CG_OSDNext_f(void)
{
	int             n;

	if(cg.osd.curEntry->id >= cg.osd.curGroup->numEntrys - 1)
		n = 0;
	else
		n = cg.osd.curEntry->id + 1;

	cg.osd.curEntry = &cg.osd.curGroup->entrys[n];
}
void CG_OSDPrev_f(void)
{

	int             n;

	if(cg.osd.curEntry->id <= 0)
		n = cg.osd.curGroup->numEntrys - 1;
	else
		n = cg.osd.curEntry->id - 1;

	cg.osd.curEntry = &cg.osd.curGroup->entrys[n];
}

osd_group_t    *OSD_FindGroup(const char *name)
{
	int             i;

	for(i = 0; i < numOSDGroups; i++)
	{
		if(Q_stricmp(osdGroups[i].name, name) == 0)
			return &osdGroups[i];
	}

	return NULL;
}

//run a consolecommand
void CG_OSDCommand(const char *parm)
{
	cg.osd.input = qfalse;		// disable input

	trap_SendClientCommand(va("%s \n ", parm));
}

//activate osd group
void CG_OSDActivate(const char *parm)
{

	cg.osd.curGroup->wish_alpha = 0.0f;
	cg.osd.curGroup = OSD_FindGroup(parm);
	cg.osd.curGroup->wish_alpha = 1.0f;
	cg.osd.curEntry = &cg.osd.curGroup->entrys[0];

}



void CG_OSDAddEntry(const char *groupName, const char *caption, void (*func), const char *parm)
{
	osd_group_t    *group;
	osd_entry_t    *entry;
	int             n, num;

	group = OSD_FindGroup(groupName);

	if(!group)
		return;


	if(group->numEntrys >= MAX_OSD_GROUP_ENTRYS)
	{
		Com_Printf("Can't add %s, MAX_OSD_GROUP_ENTRYS reached!\n", caption);
		return;
	}

	entry = &group->entrys[group->numEntrys];

	strcpy(entry->caption, caption);
	strcpy(entry->parm, parm);
	entry->func = func;
	entry->id = group->numEntrys;

	//recalculate group
	group->numEntrys++;

	group->radius = (float)OSD_RADIUS_BASE + (float)OSD_RADIUS *(float)(group->numEntrys / (float)MAX_OSD_GROUP_ENTRYS);

	for(n = 0; n < group->numEntrys; n++)
	{
		entry = &group->entrys[n];
		num = 360 / group->numEntrys;
		entry->angle = num * n + 90;

		VectorSet(entry->dir, 0, entry->angle, 0);
		AngleVectors(entry->dir, entry->dir, NULL, NULL);

		VectorScale(entry->dir, group->radius, entry->dir);
		VectorAdd(entry->dir, group->start, entry->endpos);
	}
}


void CG_OSDAddGroup(const char *name)
{
	osd_group_t    *group;
	int             i;

	if(numOSDGroups > MAX_OSD_GROUPS)
	{
		Com_Printf("CG_OSD: max groups reached\n");
		return;
	}

	group = &osdGroups[numOSDGroups];

	strcpy(group->name, name);

	group->numEntrys = 0;
	group->alpha = 0.0f;
	group->wish_alpha = 0.0f;

	VectorSet(group->start, 320, 240, 0);	// center of screen

	group->radius = 0;

	for(i = 0; i < MAX_OSD_GROUP_ENTRYS; i++)
		memset(&group->entrys[i], 0, sizeof(osd_entry_t));

	numOSDGroups++;
}

void CG_OSD_offset(void)
{
	osd_group_t    *group;
	osd_entry_t    *entry;

	float           step = 0.f;
	int             i, n;

	if(cg.osd.curEntry)
		step = (abs(cg.osd.curEntry->angle - 180)) / 15.0f;

	for(i = 0; i < numOSDGroups; i++)
	{
		group = &osdGroups[i];

		if(group->numEntrys <= 0)
			continue;

		//calculate endpos for each entry

		for(n = 0; n < group->numEntrys; n++)
		{
			entry = &group->entrys[n];

			if(cg.osd.curEntry)
			{
				if(cg.osd.curEntry->angle < 180)
					entry->angle += step;
				else if(cg.osd.curEntry->angle > 180)
					entry->angle -= step;
			}


			if(entry->angle < 0)
				entry->angle += 360;
			if(entry->angle > 360)
				entry->angle -= 360;

			//  entry->scale = tan(  (abs( entry->angle-180 ) ) / 360.0f) * 2.0f;

			VectorSet(entry->dir, 0, entry->angle - 90, 0);
			AngleVectors(entry->dir, entry->dir, NULL, NULL);

			VectorScale(entry->dir, group->radius, entry->dir);
			VectorAdd(entry->dir, group->start, entry->endpos);

			/*      float x;

			   x = entry->endpos[0];
			   x = abs(x - group->start[0]);

			   x = x / 640;

			   entry->endpos[1] *=x;
			   entry->endpos[1] += 240;
			 */
		}
	}
}

void CG_RegisterOSD(void)
{
	Com_Printf("----- registering OSD -----\n");

	memset(&osdGroups, 0, sizeof(osd_group_t));
	numOSDGroups = 0;

	cg.osd.offset = 0.0f;

	cg.osd.input = qfalse;
	cg.osd.curGroup = NULL;
	cg.osd.curEntry = NULL;

	CG_OSDAddGroup("main");
	CG_OSDAddEntry("main", "Chat", CG_OSDActivate, "chat");
	CG_OSDAddEntry("main", "Team Chat", CG_OSDActivate, "teamchat");

	CG_OSDAddGroup("chat");
	CG_OSDAddEntry("chat", "hello @ all", CG_OSDCommand, "say hello");
	CG_OSDAddEntry("chat", "gl hf", CG_OSDCommand, "say gl hf");
	CG_OSDAddEntry("chat", "gg", CG_OSDCommand, "say gg");
	CG_OSDAddEntry("chat", "lol", CG_OSDCommand, "say lol");
	CG_OSDAddEntry("chat", "rofl", CG_OSDCommand, "say rofl");

	CG_OSDAddGroup("teamchat");
	CG_OSDAddEntry("teamchat", "follow me!", CG_OSDCommand, "teamsay follow me!");
	CG_OSDAddEntry("teamchat", "im taking the lead!", CG_OSDCommand, "teamsay im taking the lead");
	CG_OSDAddEntry("teamchat", "go go go!", CG_OSDCommand, "teamsay go go go");


	CG_OSD_offset();
}

void CG_OSDInput(void)
{

	if(!cg.osd.input)
		return;

	if(!cg.osd.curEntry)
		return;

	if(!cg.osd.curGroup)
		return;

	if(cg.osd.curEntry->func)
		cg.osd.curEntry->func(cg.osd.curEntry->parm);


}


void CG_DrawOSD(void)
{
	osd_group_t    *group;
	osd_entry_t    *entry;

	int             i, n;
	char           *s;
	float           step;
	int             w = 140;
	int             h = 60;

	vec4_t          color;
	vec4_t          fontcolor;

	step = 8.0f;

	if(!cg.osd.curGroup)
		return;

	CG_OSD_offset();

	// run through groups:
	for(i = 0; i < numOSDGroups; i++)
	{
		group = &osdGroups[i];

		if(!cg.osd.input)
			group->wish_alpha = 0.0f;


		if(group->wish_alpha > group->alpha)
			group->alpha += (group->wish_alpha - group->alpha) / step;
		else if(group->wish_alpha < group->alpha)
			group->alpha -= (group->alpha - group->wish_alpha) / (step * 0.75f);



		for(n = 0; n < group->numEntrys; n++)
		{
			entry = &group->entrys[n];

			VectorSet4(color, 1.0f, 1.0f, 1.0f, group->alpha);
			VectorSet4(fontcolor, 1.0f, 1.0f, 1.0f, group->alpha);


			color[3] *= (1.0f - entry->scale);
			fontcolor[3] *= (1.0f - entry->scale);

			trap_R_SetColor(color);
			CG_DrawPic(entry->endpos[0] - w / 2, entry->endpos[1] - h / 2, w, h, cgs.media.osd_button);
			trap_R_SetColor(NULL);


			if(cg.osd.curEntry == entry)
			{
				color[1] = 0.0f;
				color[2] = 0.0f;

				trap_R_SetColor(color);
				CG_DrawPic(entry->endpos[0] - w / 2, entry->endpos[1] - h / 2, w, h, cgs.media.osd_button_focus);
				trap_R_SetColor(NULL);
			}

			s = va("%s %f", entry->caption, entry->angle);

			CG_Text_PaintAligned(entry->endpos[0], entry->endpos[1], s, 0.3f, UI_CENTER | UI_DROPSHADOW, fontcolor,
								 &cgs.media.freeSansBoldFont);
		}
	}

	return;

	if(cg.osd.offset > 0.0f)
		cg.osd.offset -= cg.osd.offset / 2.0f;
	if(cg.osd.offset < 0.0f)
		cg.osd.offset = 0.0f;

	if(!cg.osd.input)
		return;

}
