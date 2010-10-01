/*
===========================================================================
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
// lua_game.c -- qagame library for Lua

#include "g_lua.h"

#if(defined(G_LUA))

static int game_Print(lua_State * L)
{
	int             i;
	char            buf[MAX_STRING_CHARS];
	int             n = lua_gettop(L);	// number of arguments

	memset(buf, 0, sizeof(buf));

	lua_getglobal(L, "tostring");

	DEBUG_LUA("game_Print: start: ");

	for(i = 1; i <= n; i++)
	{
		const char     *s;

		lua_pushvalue(L, -1);	// function to be called
		lua_pushvalue(L, i);	// value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);	// get result

		if(s == NULL)
		{
			DEBUG_LUA("game_Print: return: no string");
			return luaL_error(L, "`tostring' must return a string to `print'");
		}

		Q_strcat(buf, sizeof(buf), s);

		lua_pop(L, 1);			// pop result
	}

	G_Printf("%s\n", buf);

	DEBUG_LUA("game_Print: return: printed string");
	return 0;
}

static int game_Broadcast(lua_State * L)
{
	int             i;
	char            buf[MAX_STRING_CHARS];
	int             n = lua_gettop(L);	// number of arguments

	memset(buf, 0, sizeof(buf));

	lua_getglobal(L, "tostring");

	DEBUG_LUA("game_Broadcast: start: ");

	for(i = 1; i <= n; i++)
	{
		const char     *s;

		lua_pushvalue(L, -1);	// function to be called
		lua_pushvalue(L, i);	// value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);	// get result

		if(s == NULL)
		{
			DEBUG_LUA("game_Broadcast: return: no string");
			return luaL_error(L, "`tostring' must return a string to `print'");
		}

		Q_strcat(buf, sizeof(buf), s);

		lua_pop(L, 1);			// pop result
	}

	trap_SendServerCommand(-1, va("cp \"" S_COLOR_WHITE "%s\n\"", buf));

	DEBUG_LUA("game_Broadcast: return: broadcasted string");
	return 0;
}

static int game_SpawnGroupRedDisable(lua_State * L)
{
	gentity_t      *next;
	int             spawnGroup;

	spawnGroup = luaL_checkint(L, 1);

	DEBUG_LUA("game_SpawnGroupRedDisable: start: group=%d", spawnGroup);

	next = NULL;
	do
	{
		next = G_Find(next, FOFS(classname), "team_ctf_redspawn");
		if(!next)
		{
			DEBUG_LUA("game_SpawnGroupRedDisable: return: next spawn not found");
			return 0;
		}
		if(next->group == spawnGroup)
		{
			trap_UnlinkEntity(next);
		}
	} while(!Q_stricmp(next->classname, "team_ctf_redspawn"));

	DEBUG_LUA("game_SpawnGroupRedDisable: return: no more spawns");
	return 0;
}

static int game_SpawnGroupBlueDisable(lua_State * L)
{
	gentity_t      *next;
	int             spawnGroup;

	spawnGroup = luaL_checkint(L, 1);

	DEBUG_LUA("game_SpawnGroupBlueDisable: start: group=%d", spawnGroup);

	next = NULL;
	do
	{
		next = G_Find(next, FOFS(classname), "team_ctf_bluespawn");
		if(!next)
		{
			DEBUG_LUA("game_SpawnGroupBlueDisable: return: next spawn not found");
			return 0;
		}
		if(next->group == spawnGroup)
		{
			trap_UnlinkEntity(next);
		}
	} while(!Q_stricmp(next->classname, "team_ctf_bluespawn"));

	DEBUG_LUA("game_SpawnGroupBlueDisable: return: no more spawns");
	return 0;
}

static int game_SpawnGroupRedEnable(lua_State * L)
{
	gentity_t      *next;
	int             spawnGroup;

	spawnGroup = luaL_checkint(L, 1);

	DEBUG_LUA("game_SpawnGroupRedEnable: start: group=%d", spawnGroup);

	next = NULL;
	do
	{
		next = G_Find(next, FOFS(classname), "team_ctf_redspawn");
		if(!next)
		{
			DEBUG_LUA("game_SpawnGroupRedEnable: return: next spawn not found");
			return 0;
		}
		if(next->group == spawnGroup)
		{
			trap_LinkEntity(next);
		}
	} while(!Q_stricmp(next->classname, "team_ctf_redspawn"));

	DEBUG_LUA("game_SpawnGroupRedEnable: return: no more spawns");
	return 0;
}

static int game_SpawnGroupBlueEnable(lua_State * L)
{
	gentity_t      *next;
	int             spawnGroup;

	spawnGroup = luaL_checkint(L, 1);

	DEBUG_LUA("game_SpawnGroupBlueEnable: start: group=%d", spawnGroup);

	next = NULL;
	do
	{
		next = G_Find(next, FOFS(classname), "team_ctf_bluespawn");
		if(!next)
		{
			DEBUG_LUA("game_SpawnGroupBlueEnable: return: next spawn not found");
			return 0;
		}
		if(next->group == spawnGroup)
		{
			trap_LinkEntity(next);
		}
	} while(!Q_stricmp(next->classname, "team_ctf_bluespawn"));

	DEBUG_LUA("game_SpawnGroupBlueEnable: return: no more spawns");
	return 0;
}

// game.EndRound()
static int game_EndRound(lua_State * L)
{
	DEBUG_LUA("game_EndRound: start: round ending");

	trap_SendServerCommand(-1, "print \"Round Ended.\n\"");
	LogExit("Round Ended.");

	DEBUG_LUA("game_EndRound: return: exited");
	return 0;
}

// game.Leveltime()
static int game_Leveltime(lua_State * L)
{
	lua_pushinteger(L, level.time);

	DEBUG_LUA("game_Leveltime: start/return: leveltime=%d", level.time);
	return 1;
}

static const luaL_reg gamelib[] = {
	{"Print", game_Print},
	{"Broadcast", game_Broadcast},
	{"EndRound", game_EndRound},
	{"Leveltime", game_Leveltime},
	{"SpawnGroupRedDisable", game_SpawnGroupRedDisable},
	{"SpawnGroupBlueDisable", game_SpawnGroupBlueDisable},
	{"SpawnGroupRedEnable", game_SpawnGroupRedEnable},
	{"SpawnGroupBlueEnable", game_SpawnGroupBlueEnable},
	{NULL, NULL}
};

int luaopen_game(lua_State * L)
{
	luaL_register(L, "game", gamelib);

	lua_pushliteral(L, "_GAMEVERSION");
	lua_pushliteral(L, GAMEVERSION);

	return 1;
}

#endif
