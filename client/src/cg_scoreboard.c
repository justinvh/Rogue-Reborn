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

// cg_scoreboard -- draw the scoreboard on top of the game screen
#include <hat/client/cg_local.h>

#define	SCOREBOARD_X		(0)

#define SB_HEADER			86
#define SB_TOP				(SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	40
#define SB_INTER_HEIGHT		16	// interleaved height

#define SB_MAXCLIENTS_NORMAL  ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

#define SB_HEAD_X			(SCOREBOARD_X+112)

#define SB_SCORELINE_X		112

#define SB_RATING_WIDTH	    (6 * BIGCHAR_WIDTH)	// width 6
#define SB_SCORE_X			(SB_SCORELINE_X + BIGCHAR_WIDTH)	// width 6
#define SB_RATING_X			(SB_SCORELINE_X + 6 * BIGCHAR_WIDTH)	// width 6
#define SB_PING_X			(SB_SCORELINE_X + 12 * BIGCHAR_WIDTH + 8)	// width 5
#define SB_TIME_X			(SB_SCORELINE_X + 17 * BIGCHAR_WIDTH + 8)	// width 5
#define SB_NAME_X			(SB_SCORELINE_X + 22 * BIGCHAR_WIDTH)	// width 15

char           *monthStr2[12] = {
	"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

// The new and improved score board

// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//  112    144   240   320   400   <-- pixel position
// head  score  ping  time  name

/*
=================
CG_DrawClientScore
=================
*/
static void CG_DrawClientScore(int y, score_t * score, float *color, float fade, qboolean largeFormat)
{
	char            string[1024];
	vec3_t          headAngles;
	clientInfo_t   *ci;
	int             headx;

	if(score->client < 0 || score->client >= cgs.maxclients)
	{
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];

	// highlight your position
	if(score->client == cg.snap->ps.clientNum)
	{
		float           hcolor[4];

		hcolor[0] = 0.7f;
		hcolor[1] = 0.7f;
		hcolor[2] = 0.7f;
		hcolor[3] = fade * 0.7f;
		CG_FillRect(SB_SCORELINE_X + 40, y, SB_SCORELINE_X * 4, BIGCHAR_HEIGHT + 5, hcolor);
	}

	// draw the players head
	headx = SB_HEAD_X + (SB_RATING_WIDTH / 2) - 5;

	VectorClear(headAngles);
	headAngles[YAW] = 90 * (cg.time / 1000.0) + 30;

	if(largeFormat)
		CG_DrawHead(headx, y, 24, 24, score->client, headAngles);
	else
		CG_DrawHead(headx, y, 16, 16, score->client, headAngles);

#ifdef MISSIONPACK
	// draw the team task
	if(ci->teamTask != TEAMTASK_NONE)
	{
		if(ci->teamTask == TEAMTASK_OFFENSE)
		{
			CG_DrawPic(headx + 48, y, 16, 16, cgs.media.assaultShader);
		}
		else if(ci->teamTask == TEAMTASK_DEFENSE)
		{
			CG_DrawPic(headx + 48, y, 16, 16, cgs.media.defendShader);
		}
	}
#endif

	// draw the score line
	if(score->ping == -1)
	{
		Com_sprintf(string, sizeof(string), " connecting    %s", ci->name);
	}
	else if(ci->team == TEAM_SPECTATOR)
	{
		Com_sprintf(string, sizeof(string), " SPECT %3i %4i %s", score->ping, score->time, ci->name);
	}
	else if(cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client))
	{
		Com_sprintf(string, sizeof(string), " READY %3i %4i %s", score->ping, score->time, ci->name);
	}
	else
	{
		Com_sprintf(string, sizeof(string), "%5i %4i %4i %s", score->score, score->ping, score->time, ci->name);
	}

	CG_DrawBigString(SB_SCORELINE_X + (SB_RATING_WIDTH / 2), y, string, fade);
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard(int y, team_t team, float fade, int maxClients, int lineHeight)
{
	int             i;
	score_t        *score;
	float           color[4];
	int             count;
	clientInfo_t   *ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;
	for(i = 0; i < cg.numScores && count < maxClients; i++)
	{
		score = &cg.scores[i];
		ci = &cgs.clientinfo[score->client];

		if(team != ci->team)
			continue;

		CG_DrawClientScore(y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT);

		count++;
	}

	return count;
}

/*
================
CG_CenterGiantLine
================
*/
static void CG_CenterGiantLine(float y, const char *string)
{
	float           x;
	vec4_t          color;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	x = 0.5 * (640 - GIANT_WIDTH * CG_DrawStrlen(string));

	CG_DrawStringExt(x, y, string, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);
}

// ======================================================================================

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/

#define SCOREBOARD_HEIGHT 240
#define SCOREBOARD_DISPLAY 20

#define SCOREBOARD_RED 50
#define SCOREBOARD_BLUE 330
#define SCOREBOARD_FFA 150

void CG_DrawScoreboardTitlebarNew(vec4_t color, qboolean team)
{
	const char     *s;

	trap_R_SetColor(color);
	CG_DrawPic(150, 73, 340, 40, cgs.media.hud_scoreboard_title);
	trap_R_SetColor(NULL);


	if(team)
	{
		if(cg.teamScores[0] == cg.teamScores[1])
		{
			s = va("Teams are tied at %i", cg.teamScores[0]);
		}
		else if(cg.teamScores[0] >= cg.teamScores[1])
		{
			s = va("Red leads %i to %i", cg.teamScores[0], cg.teamScores[1]);
		}
		else
		{
			s = va("Blue leads %i to %i", cg.teamScores[1], cg.teamScores[0]);
		}

		CG_Text_PaintAligned(320, 93, s, 0.4f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	}
	else
	{

		if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		{
			s = "spectating...";
		}
		else if(cg.killerName[0] && cg.predictedPlayerState.pm_type != PM_INTERMISSION &&
				cg.predictedPlayerState.pm_type == PM_DEAD)
		{
			s = va("Fragged by %s", cg.killerName);
		}
		else
		{
			s = va("%s place with %i", CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1), cg.snap->ps.persistant[PERS_SCORE]);
		}

		CG_Text_PaintAligned(320, 93, s, 0.4f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	}

	trap_R_SetColor(color);
	CG_DrawPic(150, 73, 340, 40, cgs.media.hud_scoreboard_title_overlay);
	trap_R_SetColor(NULL);
}


int             headline_ffa[] = { 165, 265, 335, 395, 440 };	//player, clan, score, time, ping
int             headline_red[] = { 12 + SCOREBOARD_RED, 100 + SCOREBOARD_RED, 146 + SCOREBOARD_RED, 184 + SCOREBOARD_RED, 220 + SCOREBOARD_RED };	//player, clan, score, time, ping
int             headline_blue[] = { 12 + SCOREBOARD_BLUE, 100 + SCOREBOARD_BLUE, 146 + SCOREBOARD_BLUE, 184 + SCOREBOARD_BLUE, 220 + SCOREBOARD_BLUE };	//player, clan, score, time, ping


void CG_DrawScoreboardHeadlineNew(int pos[])
{
	const char     *s;

	if(cgs.gametype >= GT_TEAM)
		s = va("Player");
	else
		s = va("Player (%i/%i)", cg.numScores, cgs.maxclients);

	CG_Text_PaintAligned(pos[0], 138, s, 0.2f, UI_LEFT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	s = va("Clan");
	CG_Text_PaintAligned(pos[1], 138, s, 0.2f, UI_LEFT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	s = va("Score");
	CG_Text_PaintAligned(pos[2], 138, s, 0.2f, UI_LEFT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	s = va("Time");
	CG_Text_PaintAligned(pos[3], 138, s, 0.2f, UI_LEFT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	s = va("Ping");
	CG_Text_PaintAligned(pos[4], 138, s, 0.2f, UI_LEFT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

}

		//buttom line:
void CG_DrawScoreboardUnderlineNew(void)
{
	const char     *s;
	const char     *ts = "";
	int             mins, seconds, tens;
	int             msec;

	const char     *info;
	char           *mapname;

	info = CG_ConfigString(CS_SERVERINFO);
	mapname = Info_ValueForKey(info, "mapname");

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

			ts = va("Time left: %i:%i%i", mins, tens, seconds);

		}
	}

	if(cgs.gametype >= GT_TEAM)	// team based scoreboard
	{
		// current players
		s = va("Players: %i/%i", cg.numScores, cgs.maxclients);

		CG_Text_PaintAligned(294, 408, s, 0.2f, UI_RIGHT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

		// time left
		if(cgs.timelimit > 0)
		{

			CG_Text_PaintAligned(344, 408, ts, 0.2f, UI_LEFT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);
		}

		if(cgs.gametype >= GT_CTF)
		{
			if(cgs.gametype == GT_1FCTF)
			{
				s = va("One Flag CTF on %s", mapname);
			}
			else if(cgs.gametype == GT_OBELISK)
			{
				s = va("Overload on %s", mapname);
			}
			else if(cgs.gametype == GT_HARVESTER)
			{
				s = va("Harvester on %s", mapname);
			}
			else
			{
				s = va("CTF on %s", mapname);
			}

			CG_Text_PaintAligned(SCOREBOARD_RED + 15, 408, s, 0.2f, UI_LEFT | UI_DROPSHADOW, colorWhite,
								 &cgs.media.freeSansBoldFont);

			s = va("Capturelimit: %i", cgs.capturelimit);
			CG_Text_PaintAligned(576, 408, s, 0.2f, UI_RIGHT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);
		}
		else
		{
			s = va("TDM on %s", mapname);

			CG_Text_PaintAligned(SCOREBOARD_RED + 15, 408, s, 0.2f, UI_LEFT | UI_DROPSHADOW, colorWhite,
								 &cgs.media.freeSansBoldFont);
			s = va("Fraglimit: %i", cgs.fraglimit);

			CG_Text_PaintAligned(576, 408, s, 0.2f, UI_RIGHT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);
		}

	}
	else						// ffa
	{
		// time left
		if(cgs.timelimit > 0)
		{

			CG_Text_PaintAligned(320, 408, ts, 0.2f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);
		}

		s = va("FFA on %s", mapname);
		CG_Text_PaintAligned(SCOREBOARD_FFA + 15, 408, s, 0.2f, UI_LEFT | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

		s = va("Fraglimit: %i", cgs.fraglimit);
		CG_Text_PaintAligned(640 - SCOREBOARD_FFA - 15, 408, s, 0.2f, UI_RIGHT | UI_DROPSHADOW, colorWhite,
							 &cgs.media.freeSansBoldFont);

	}


}


void CG_DrawScoreboardStatNew(clientInfo_t * ci, score_t * score, int count, int num, int height, float fontsize, vec4_t bgcolor,
							  int pos[])
{
	int             w, w1;
	const char     *s;
	vec4_t          fontcolor;


	if((num & 1) == 0)
		bgcolor[3] = 0.05f;
	else
		bgcolor[3] = 0.0f;

	// set font color
	VectorSet4(fontcolor, 1.0f, 1.0f, 1.0f, 0.80f);

	fontcolor[3] -= (num - cg.scoreboard_offset) * 0.033f;

	if(score->client == cg.snap->ps.clientNum)	// highlight your position
	{
		fontcolor[0] = 0.2f;
		fontcolor[2] = 0.2f;

	}

	trap_R_SetColor(bgcolor);
	CG_DrawPic(pos[0], 150 + num * height, 310, height, cgs.media.whiteShader);
	trap_R_SetColor(NULL);

	if(score->ping == -1)
	{
		// draw "connecting" instead of stats
		s = va("connecting...");

		CG_Text_PaintAligned(pos[0] + 10, 160 + num * height, s, fontsize, UI_LEFT | UI_DROPSHADOW, fontcolor,
							 &cgs.media.freeSansBoldFont);

	}
	else
	{
		// place
		if(ci->team == TEAM_SPECTATOR)
			s = va("S ");
		else
			s = va("%i. ", count + cg.scoreboard_offset);

		w = CG_Text_Width(s, fontsize, 0, &cgs.media.freeSansBoldFont);

		CG_Text_PaintAligned(pos[0], 160 + num * height, s, fontsize, UI_LEFT | UI_DROPSHADOW, fontcolor,
							 &cgs.media.freeSansBoldFont);

		// name
		s = va("%s", ci->name);
		w1 = w + 3;
		w = CG_Text_Width(s, fontsize, 0, &cgs.media.freeSansBoldFont);

		CG_Text_PaintAligned(pos[0] + w1, 160 + num * height, s, fontsize, UI_LEFT | UI_DROPSHADOW, fontcolor,
							 &cgs.media.freeSansBoldFont);


		// ready ?

		if(cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client))
		{
			s = " (ready)";
			w1 += w;
			w = CG_Text_Width(s, fontsize, 0, &cgs.media.freeSansBoldFont);
			CG_Text_PaintAligned(pos[0] + w1, 160 + num * height, s, fontsize, UI_LEFT | UI_DROPSHADOW, colorYellow,
								 &cgs.media.freeSansBoldFont);


		}

		if(ci->team != TEAM_SPECTATOR)
		{
			// TODO clantag
			s = va(" ");
			CG_Text_PaintAligned(pos[1], 160 + num * height, s, fontsize, UI_LEFT | UI_DROPSHADOW, fontcolor,
								 &cgs.media.freeSansBoldFont);

			// Score
			s = va("%i", score->score);
			CG_Text_PaintAligned(pos[2], 160 + num * height, s, fontsize, UI_LEFT | UI_DROPSHADOW, fontcolor,
								 &cgs.media.freeSansBoldFont);


			// Time
			s = va("%i", score->time);
			CG_Text_PaintAligned(pos[3], 160 + num * height, s, fontsize, UI_LEFT | UI_DROPSHADOW, fontcolor,
								 &cgs.media.freeSansBoldFont);


		}

		// Ping
		s = va("%i", score->ping);
		CG_Text_PaintAligned(pos[4], 160 + num * height, s, fontsize, UI_LEFT | UI_DROPSHADOW, fontcolor,
							 &cgs.media.freeSansBoldFont);
	}
}


qboolean CG_DrawScoreboardNew(void)
{
	float           fade;
	float          *fadeColor;
	qtime_t         tm;
	char            st[1024];
	vec4_t          basecolor;
	vec4_t          bgcolor;
	int             i;
	score_t        *score;
	clientInfo_t   *ci;
	int             count = 0;
	int             red_count = 0;
	int             blue_count = 0;
	int             max_height;
	int             max_display;	//how many stats are visible ( note: displaying more then 20 cause non readable stats on low resolutions )

	float           fontsize;
	float           height;



	if(cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if(cg.warmup && !cg.showScores)
		return qfalse;

	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		VectorCopy4(blueTeamColor, basecolor);
	else if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED)
		VectorCopy4(redTeamColor, basecolor);
	else
		VectorCopy4(baseTeamColor, basecolor);


	if(cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		fade = 1.0;
		fadeColor = colorWhite;
	}
	else
	{
		fadeColor = CG_FadeColor(cg.scoreFadeTime, FADE_TIME);

		if(!fadeColor)
		{
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}


	//draw scoreboard

	max_height = SCOREBOARD_HEIGHT;
	max_display = cg.numScores;


	max_display = SCOREBOARD_DISPLAY;

	//FIXME: displaying more then 32 players causes the display to disappear. is this a bug of the scoreboard ?
	if(cgs.gametype >= GT_TEAM)
	{
		max_display *= 2;
		max_height *= 2;

		//FIXME: scrolling does not work for teamscores
		cg.scoreboard_offset = 0;
	}

	if(max_display > cg.numScores)
		max_display = cg.numScores;

	if(cg.numScores > max_display)
		max_display = SCOREBOARD_DISPLAY;


	if(cg.scoreboard_offset > (cg.numScores - SCOREBOARD_DISPLAY))
		cg.scoreboard_offset = (cg.numScores - SCOREBOARD_DISPLAY);

	if(cg.scoreboard_offset < 0)
		cg.scoreboard_offset = 0;

	//calculate size for each entry
	height = 14;

	if((height * cg.numScores) > max_height)
		height = max_height / max_display;


	//calculate fontsize
	fontsize = height / 72.0f;


	if(cgs.gametype >= GT_TEAM)	// team based scoreboard
	{
		// Titlebar

		CG_DrawScoreboardTitlebarNew(basecolor, qtrue);

		//red
		trap_R_SetColor(redTeamColor);
		CG_DrawPic(SCOREBOARD_RED, 120, 260, 300, cgs.media.hud_scoreboard);
		trap_R_SetColor(NULL);

		CG_DrawScoreboardHeadlineNew(headline_red);

		//blue
		trap_R_SetColor(blueTeamColor);
		CG_DrawPic(SCOREBOARD_BLUE, 120, 260, 300, cgs.media.hud_scoreboard);
		trap_R_SetColor(NULL);

		CG_DrawScoreboardHeadlineNew(headline_blue);

		CG_DrawScoreboardUnderlineNew();

		//display scores
		for(i = 0; i < max_display; i++)
		{
			score = &cg.scores[i + cg.scoreboard_offset];
			ci = &cgs.clientinfo[score->client];


			VectorCopy4(basecolor, bgcolor);

			if((ci->team == TEAM_BLUE) || (ci->team == TEAM_SPECTATOR && (i & 1) == 0))
			{

				CG_DrawScoreboardStatNew(ci, score, blue_count + 1, blue_count, height, fontsize, bgcolor, headline_blue);
				blue_count++;
			}
			else
			{

				CG_DrawScoreboardStatNew(ci, score, red_count + 1, red_count, height, fontsize, bgcolor, headline_red);
				red_count++;
			}
		}
	}
	else						// FFA Scoreboard
	{

		// Titlebar
		CG_DrawScoreboardTitlebarNew(basecolor, qfalse);

		// scoreboard
		trap_R_SetColor(basecolor);
		CG_DrawPic(SCOREBOARD_FFA, 120, 340, 300, cgs.media.hud_scoreboard);
		trap_R_SetColor(NULL);

		// top line:
		CG_DrawScoreboardHeadlineNew(headline_ffa);

		// buttom line:
		CG_DrawScoreboardUnderlineNew();


		for(i = 0; i < max_display; i++)
		{
			score = &cg.scores[i + cg.scoreboard_offset];
			ci = &cgs.clientinfo[score->client];

			VectorCopy4(basecolor, bgcolor);

			if(ci->team != TEAM_SPECTATOR)
				count++;

			CG_DrawScoreboardStatNew(ci, score, count, i, height, fontsize, bgcolor, headline_ffa);
		}
	}


	// put the current date and time at the bottom along with version info
	trap_RealTime(&tm);
	Com_sprintf(st, sizeof(st), "%2i:%s%i:%s%i (%i %s %i) " S_COLOR_RED "XreaL v" PRODUCT_VERSION " " S_COLOR_WHITE " http://xreal.sourceforge.net", (1 + (tm.tm_hour + 11) % 12),	// 12 hour format
				(tm.tm_min > 9 ? "" : "0"),	// minute padding
				tm.tm_min, (tm.tm_sec > 9 ? "" : "0"),	// second padding
				tm.tm_sec, tm.tm_mday, monthStr2[tm.tm_mon], 1900 + tm.tm_year);


	CG_Text_PaintAligned(320, 470, st, 0.2f, UI_CENTER | UI_DROPSHADOW, colorWhite, &cgs.media.freeSansBoldFont);

	return qtrue;
}




qboolean CG_DrawOldScoreboard(void)
{
	int             x, y, w, n1, n2;
	float           fade;
	float          *fadeColor;
	const char     *s;
	int             maxClients;
	int             lineHeight;
	int             topBorderSize, bottomBorderSize;
	qtime_t         tm;
	char            st[1024];

	if(cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if(cg.warmup && !cg.showScores)
		return qfalse;

	if(cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		fade = 1.0;
		fadeColor = colorWhite;
	}
	else
	{
		fadeColor = CG_FadeColor(cg.scoreFadeTime, FADE_TIME);

		if(!fadeColor)
		{
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}

	// fragged by ... line
	if(cg.killerName[0] && cg.predictedPlayerState.pm_type != PM_INTERMISSION && cg.predictedPlayerState.pm_type == PM_DEAD)
	{
		s = va("Fragged by %s", cg.killerName);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = 40 + 70;
		CG_DrawStringExt(x, y, s, colorWhite, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 900);
	}
	else
	{
		s = va("%s place with %i", CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1), cg.snap->ps.persistant[PERS_SCORE]);
		w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = 40 + 70;
		CG_DrawStringExt(x, y, s, colorWhite, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 900);
	}

	// scoreboard
	y = SB_HEADER + 70;

	CG_DrawPic(SB_SCORE_X + (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardScore);
	CG_DrawPic(SB_PING_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardPing);
	CG_DrawPic(SB_TIME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardTime);
	CG_DrawPic(SB_NAME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardName);

	y = SB_TOP + 70;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if(cg.numScores > SB_MAXCLIENTS_NORMAL)
	{
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	}
	else
	{
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 16;
		bottomBorderSize = 16;
	}

	if(cgs.gametype >= GT_TEAM)
	{
		// teamplay scoreboard
		y += lineHeight / 2;

		if(cg.teamScores[0] >= cg.teamScores[1])
		{
			n1 = CG_TeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;

			n2 = CG_TeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}
		else
		{
			n1 = CG_TeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;

			n2 = CG_TeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}

		n1 = CG_TeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients, lineHeight);
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
	}
	else
	{
		// free for all scoreboard
		n1 = CG_TeamScoreboard(y, TEAM_FREE, fade, maxClients, lineHeight);
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

		n2 = CG_TeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight);
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	// load any models that have been deferred
	if(++cg.deferredPlayerLoading > 10)
	{
		CG_LoadDeferredPlayers();
	}

	// put the current date and time at the bottom along with version info
	trap_RealTime(&tm);
	Com_sprintf(st, sizeof(st), "%2i:%s%i:%s%i (%i %s %i) " S_COLOR_YELLOW "XreaL v" PRODUCT_VERSION " " S_COLOR_BLUE " http://xreal.sourceforge.net", (1 + (tm.tm_hour + 11) % 12),	// 12 hour format
				(tm.tm_min > 9 ? "" : "0"),	// minute padding
				tm.tm_min, (tm.tm_sec > 9 ? "" : "0"),	// second padding
				tm.tm_sec, tm.tm_mday, monthStr2[tm.tm_mon], 1900 + tm.tm_year);

	w = CG_Text_Width(st, 0.3f, 0, &cgs.media.freeSansBoldFont);
	x = 320 - w / 2;
	y = 475;
	CG_Text_Paint(x, y, 0.3f, colorWhite, st, 0, 0, UI_DROPSHADOW, &cgs.media.freeSansBoldFont);

	return qtrue;
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawOldTourneyScoreboard(void)
{
	const char     *s;
	vec4_t          color;
	int             min, tens, ones;
	clientInfo_t   *ci;
	int             y;
	int             i;

	// request more scores regularly
	if(cg.scoresRequestTime + 2000 < cg.time)
	{
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand("score");
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// draw the dialog background
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color);

	// print the mesage of the day
	s = CG_ConfigString(CS_MOTD);
	if(!s[0])
		s = "Scoreboard";

	// print optional title
	CG_CenterGiantLine(8, s);

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va("%i:%i%i", min, tens, ones);

	CG_CenterGiantLine(64, s);

	// print the two scores
	y = 160;
	if(cgs.gametype >= GT_TEAM)
	{
		// teamplay scoreboard
		CG_DrawStringExt(8, y, "Red Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);

		s = va("%i", cg.teamScores[0]);
		CG_DrawStringExt(632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);

		y += 64;

		CG_DrawStringExt(8, y, "Blue Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);

		s = va("%i", cg.teamScores[1]);
		CG_DrawStringExt(632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);
	}
	else
	{
		// free for all scoreboard
		for(i = 0; i < MAX_CLIENTS; i++)
		{
			ci = &cgs.clientinfo[i];
			if(!ci->infoValid)
				continue;

			if(ci->team != TEAM_FREE)
				continue;

			CG_DrawStringExt(8, y, ci->name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);

			s = va("%i", ci->score);
			CG_DrawStringExt(632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0);

			y += 64;
		}
	}
}
