/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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



#define VOICECHAT_GETFLAG			"getflag"	// command someone to get the flag
#define VOICECHAT_OFFENSE			"offense"	// command someone to go on offense
#define VOICECHAT_DEFEND			"defend"	// command someone to go on defense
#define VOICECHAT_DEFENDFLAG		"defendflag"	// command someone to defend the flag
#define VOICECHAT_PATROL			"patrol"	// command someone to go on patrol (roam)
#define VOICECHAT_CAMP				"camp"	// command someone to camp (we don't have sounds for this one)
#define VOICECHAT_FOLLOWME			"followme"	// command someone to follow you
#define VOICECHAT_RETURNFLAG		"returnflag"	// command someone to return our flag
#define VOICECHAT_FOLLOWFLAGCARRIER	"followflagcarrier"	// command someone to follow the flag carrier
#define VOICECHAT_YES				"yes"	// yes, affirmative, etc.
#define VOICECHAT_NO				"no"	// no, negative, etc.
#define VOICECHAT_ONGETFLAG			"ongetflag"	// I'm getting the flag
#define VOICECHAT_ONOFFENSE			"onoffense"	// I'm on offense
#define VOICECHAT_ONDEFENSE			"ondefense"	// I'm on defense
#define VOICECHAT_ONPATROL			"onpatrol"	// I'm on patrol (roaming)
#define VOICECHAT_ONCAMPING			"oncamp"	// I'm camping somewhere
#define VOICECHAT_ONFOLLOW			"onfollow"	// I'm following
#define VOICECHAT_ONFOLLOWCARRIER	"onfollowcarrier"	// I'm following the flag carrier
#define VOICECHAT_ONRETURNFLAG		"onreturnflag"	// I'm returning our flag
#define VOICECHAT_INPOSITION		"inposition"	// I'm in position
#define VOICECHAT_IHAVEFLAG			"ihaveflag"	// I have the flag
#define VOICECHAT_BASEATTACK		"baseattack"	// the base is under attack
#define VOICECHAT_ENEMYHASFLAG		"enemyhasflag"	// the enemy has our flag (CTF)
#define VOICECHAT_STARTLEADER		"startleader"	// I'm the leader
#define VOICECHAT_STOPLEADER		"stopleader"	// I resign leadership
#define VOICECHAT_TRASH				"trash"	// lots of trash talk
#define VOICECHAT_WHOISLEADER		"whoisleader"	// who is the team leader
#define VOICECHAT_WANTONDEFENSE		"wantondefense"	// I want to be on defense
#define VOICECHAT_WANTONOFFENSE		"wantonoffense"	// I want to be on offense
#define VOICECHAT_KILLINSULT		"kill_insult"	// I just killed you
#define VOICECHAT_TAUNT				"taunt"	// I want to taunt you
#define VOICECHAT_DEATHINSULT		"death_insult"	// you just killed me
#define VOICECHAT_KILLGAUNTLET		"kill_gauntlet"	// I just killed you with the gauntlet
#define VOICECHAT_PRAISE			"praise"	// you did something good
