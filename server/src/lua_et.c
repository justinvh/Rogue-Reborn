/*
===========================================================================
Parts of this file are based on the ETPub source code under GPL.
http://trac2.assembla.com/etpub/browser/trunk/src/game/g_lua.c
rev 170 + rev 192
Code by quad and pheno

Ported to Xreal by Andrew "DerSaidin" Browne.

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
// lua_et.c -- main library for Lua

#include "g_lua.h"

#if(defined(G_LUA))

#include "lua_et.h"

/***************************/
/* Lua et library handlers */
/***************************/

// ET Library Calls {{{
// et.RegisterModname( modname )
static int _et_RegisterModname(lua_State * L)
{
	const char     *modname = luaL_checkstring(L, 1);

	if(modname)
	{
		lua_vm_t       *vm = G_LuaGetVM(L);

		if(vm)
		{
			Q_strncpyz(vm->mod_name, modname, sizeof(vm->mod_name));
		}
	}
	return 0;
}

// et.FindModname( modname )
static int _et_FindModname(lua_State * L)
{
	int             i;
	const char     *modname = luaL_checkstring(L, 1);

	if(modname)
	{
		for(i = 0; i < LUA_NUM_VM; i++)
		{
			if(lVM[i] && !Q_stricmp(lVM[i]->mod_name, modname))
			{
				lua_pushinteger(L, i);
				return 1;
			}
		}
	}
	return 0;
}


// vmnumber = et.FindSelf()
static int _et_FindSelf(lua_State * L)
{
	lua_vm_t       *vm = G_LuaGetVM(L);

	if(vm)
	{
		lua_pushinteger(L, vm->id);
	}
	else
	{
		lua_pushnil(L);
	}
	return 1;
}

// modname, signature = et.FindMod( vmnumber )
static int _et_FindMod(lua_State * L)
{
	int             vmnumber = luaL_checkint(L, 1);
	lua_vm_t       *vm = lVM[vmnumber];

	DEBUG_LUA("et_FindMod: start: vmnum=%d", vmnumber);
	if(vm)
	{
		DEBUG_LUA("et_FindMod: return: name=%s signature=%s", vm->mod_name, vm->mod_signature);
		lua_pushstring(L, vm->mod_name);
		lua_pushstring(L, vm->mod_signature);
	}
	else
	{
		DEBUG_LUA("et_FindMod: return: invalid vm specified");
		return 0;
		//lua_pushnil(L);
		//lua_pushnil(L);
	}
	return 2;
}

// success = et.IPCSend( vmnumber, message )
static int _et_IPCSend(lua_State * L)
{
	int             vmnumber = luaL_checkint(L, 1);
	const char     *message = luaL_checkstring(L, 2);

	lua_vm_t       *sender = G_LuaGetVM(L);
	lua_vm_t       *vm = lVM[vmnumber];

	DEBUG_LUA("_et_IPCSend: start: vmnum=%d message=%s", vmnumber, message);

	if(!vm || vm->err)
	{
		DEBUG_LUA("_et_IPCSend: return: recipient vm not found.");
		lua_pushinteger(L, 0);
		return 1;
	}

	// Find callback
	if(!G_LuaGetNamedFunction(vm, "et_IPCReceive"))
	{
		DEBUG_LUA("_et_IPCSend: return: recipient vm does not have et_IPCRecieve method.");
		lua_pushinteger(L, 0);
		return 1;
	}

	// Arguments
	if(sender)
	{
		lua_pushinteger(vm->L, sender->id);
	}
	else
	{
		lua_pushnil(vm->L);
	}
	lua_pushstring(vm->L, message);

	// Call
	if(!G_LuaCall(vm, "et.IPCSend", 2, 0))
	{
		//G_LuaStopVM(vm);
		lua_pushinteger(L, 0);
		return 1;
	}

	// Success
	lua_pushinteger(L, 1);
	return 1;
}

// }}}

// Printing {{{
// et.G_Print( text )
static int _et_G_Print(lua_State * L)
{
	char            text[1024];

	Q_strncpyz(text, luaL_checkstring(L, 1), sizeof(text));
	trap_Printf(text);
	return 0;
}

// et.G_LogPrint( text ) 
static int _et_G_LogPrint(lua_State * L)
{
	char            text[1024];

	Q_strncpyz(text, luaL_checkstring(L, 1), sizeof(text));
	LOG(text);
	return 0;
}

// }}}

// Argument Handling {{{
// args = et.ConcatArgs( index )
static int _et_ConcatArgs(lua_State * L)
{
	int             index = luaL_checkint(L, 1);

	lua_pushstring(L, ConcatArgs(index));
	return 1;
}

// pheno: removed because it's easier to use the code
//        located in g_cmds.c
/*static int _et_ConcatArgs(lua_State *L)
{
	int i, off = 0, len;
	char buff[MAX_STRING_CHARS];
	int index = luaL_optint(L, 1, 0);
	int count = trap_Argc()-index;
	
	if (count < 0) count = 0;
	
	for (i=index; i<index+count; i++) {
		trap_Argv(i, buff, sizeof(buff));
		len = strlen(buff);
		if (i < index+count-1 && len < sizeof(buff)-2) {
			buff[len] = ' ';
			buff[len+1] = '\0';
		}
		lua_pushstring(L, buff);
	}
	lua_concat(L, count);
	return 1;
}*/

// argcount = et.trap_Argc()
static int _et_trap_Argc(lua_State * L)
{
	lua_pushinteger(L, trap_Argc());
	return 1;
}

// arg = et.trap_Argv( argnum ) 
static int _et_trap_Argv(lua_State * L)
{
	char            buff[MAX_STRING_CHARS];
	int             argnum = luaL_checkint(L, 1);

	trap_Argv(argnum, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

// }}}

// Cvars {{{
// cvarvalue = et.trap_Cvar_Get( cvarname ) 
static int _et_trap_Cvar_Get(lua_State * L)
{
	char            buff[MAX_CVAR_VALUE_STRING];
	const char     *cvarname = luaL_checkstring(L, 1);

	trap_Cvar_VariableStringBuffer(cvarname, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

// et.trap_Cvar_Set( cvarname, cvarvalue )
static int _et_trap_Cvar_Set(lua_State * L)
{
	const char     *cvarname = luaL_checkstring(L, 1);
	const char     *cvarvalue = luaL_checkstring(L, 2);

	trap_Cvar_Set(cvarname, cvarvalue);
	return 0;
}

// }}}

// Config Strings {{{
// configstringvalue = et.trap_GetConfigstring( index ) 
static int _et_trap_GetConfigstring(lua_State * L)
{
	char            buff[MAX_STRING_CHARS];
	int             index = luaL_checkint(L, 1);

	trap_GetConfigstring(index, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

// et.trap_SetConfigstring( index, configstringvalue ) 
static int _et_trap_SetConfigstring(lua_State * L)
{
	int             index = luaL_checkint(L, 1);
	const char     *csv = luaL_checkstring(L, 2);

	trap_SetConfigstring(index, csv);
	return 0;
}

// }}}

// Server {{{
// et.trap_SendConsoleCommand( when, command )
static int _et_trap_SendConsoleCommand(lua_State * L)
{
	int             when = luaL_checkint(L, 1);
	const char     *cmd = luaL_checkstring(L, 2);

	trap_SendConsoleCommand(when, cmd);
	return 0;
}

// }}}

/*
// Clients {{{
// et.trap_DropClient( clientnum, reason, ban_time )
static int _et_trap_DropClient(lua_State *L)
{
	int clientnum = luaL_checkint(L, 1);
	const char *reason = luaL_checkstring(L, 2);
	int ban = trap_Cvar_VariableIntegerValue("g_defaultBanTime");
	ban = luaL_optint(L, 3, ban);
	// xreal, no ban param
	//trap_DropClient(clientnum, reason, ban);
	trap_DropClient(clientnum, reason);
	return 0;
}
*/

// et.trap_SendServerCommand( clientnum, command )
static int _et_trap_SendServerCommand(lua_State * L)
{
	int             clientnum = luaL_checkint(L, 1);
	const char     *cmd = luaL_checkstring(L, 2);

	trap_SendServerCommand(clientnum, cmd);
	return 0;
}

/*
// et.G_Say( clientNum, mode, text )
static int _et_G_Say(lua_State *L)
{
	int clientnum = luaL_checkint(L, 1);
	int mode = luaL_checkint(L, 2);
	const char *text = luaL_checkstring(L, 3);
	// xreal
	G_Say(g_entities + clientnum, NULL, mode, text);
	//TODO: reimplement
	return 0;
}
*/

// et.ClientUserinfoChanged( clientNum )
static int _et_ClientUserinfoChanged(lua_State * L)
{
	int             clientnum = luaL_checkint(L, 1);

	ClientUserinfoChanged(clientnum);
	return 0;
}

// }}}

// Userinfo {{{
// userinfo = et.trap_GetUserinfo( clientnum )
static int _et_trap_GetUserinfo(lua_State * L)
{
	char            buff[MAX_STRING_CHARS];
	int             clientnum = luaL_checkint(L, 1);

	trap_GetUserinfo(clientnum, buff, sizeof(buff));
	lua_pushstring(L, buff);
	return 1;
}

// et.trap_SetUserinfo( clientnum, userinfo )
static int _et_trap_SetUserinfo(lua_State * L)
{
	int             clientnum = luaL_checkint(L, 1);
	const char     *userinfo = luaL_checkstring(L, 2);

	trap_SetUserinfo(clientnum, userinfo);
	return 0;
}

// }}}

// String Utility Functions {{{
// infostring = et.Info_RemoveKey( infostring, key )
static int _et_Info_RemoveKey(lua_State * L)
{
	char            buff[MAX_INFO_STRING];
	const char     *key = luaL_checkstring(L, 2);

	Q_strncpyz(buff, luaL_checkstring(L, 1), sizeof(buff));
	Info_RemoveKey(buff, key);
	lua_pushstring(L, buff);
	return 1;
}

// infostring = et.Info_SetValueForKey( infostring, key, value )
static int _et_Info_SetValueForKey(lua_State * L)
{
	char            buff[MAX_INFO_STRING];
	const char     *key = luaL_checkstring(L, 2);
	const char     *value = luaL_checkstring(L, 3);

	Q_strncpyz(buff, luaL_checkstring(L, 1), sizeof(buff));
	Info_SetValueForKey(buff, key, value);
	lua_pushstring(L, buff);
	return 1;
}

// keyvalue = et.Info_ValueForKey( infostring, key )
static int _et_Info_ValueForKey(lua_State * L)
{
	const char     *infostring = luaL_checkstring(L, 1);
	const char     *key = luaL_checkstring(L, 2);

	lua_pushstring(L, Info_ValueForKey(infostring, key));
	return 1;
}

// cleanstring = et.Q_CleanStr( string )
static int _et_Q_CleanStr(lua_State * L)
{
	char            buff[MAX_STRING_CHARS];

	Q_strncpyz(buff, luaL_checkstring(L, 1), sizeof(buff));
	Q_CleanStr(buff);
	lua_pushstring(L, buff);
	return 1;
}

// }}}

/*
// ET Filesystem {{{
// fd, len = et.trap_FS_FOpenFile( filename, mode )
static int _et_trap_FS_FOpenFile(lua_State *L)
{
	fileHandle_t fd;
	int len;
	const char *filename = luaL_checkstring(L, 1);
	int mode = luaL_checkint(L, 2);
	len = trap_FS_FOpenFile(filename, &fd, mode);
	lua_pushinteger(L, fd);
	lua_pushinteger(L, len);
	return 2;
}

// filedata = et.trap_FS_Read( fd, count )
static int _et_trap_FS_Read(lua_State *L)
{
	char *filedata = "";
	fileHandle_t fd = luaL_checkint(L, 1);
	int count = luaL_checkint(L, 2);
	filedata = malloc(count + 1);
	trap_FS_Read(filedata, count, fd);
	*(filedata + count) = '\0';
	lua_pushstring(L, filedata);
	return 1;
}

// count = et.trap_FS_Write( filedata, count, fd )
static int _et_trap_FS_Write(lua_State *L)
{
	const char *filedata = luaL_checkstring(L, 1);
	int count = luaL_checkint(L, 2);
	fileHandle_t fd = luaL_checkint(L, 3);
	// xreal 
	trap_FS_Write(filedata, count, fd);
	lua_pushinteger(L, 0);
	return 1;
}

// et.trap_FS_Rename( oldname, newname )
static int _et_trap_FS_Rename(lua_State *L)
{
	const char *oldname = luaL_checkstring(L, 1);
	const char *newname = luaL_checkstring(L, 2);
	trap_FS_Rename(oldname, newname);
	return 0;
}

// et.trap_FS_FCloseFile( fd )
static int _et_trap_FS_FCloseFile(lua_State *L)
{
	fileHandle_t fd = luaL_checkint(L, 1);
	trap_FS_FCloseFile(fd);
	return 0;
}
// }}}
*/

// Indexes {{{
// soundindex = et.G_SoundIndex( filename )
static int _et_G_SoundIndex(lua_State * L)
{
	const char     *filename = luaL_checkstring(L, 1);

	lua_pushinteger(L, G_SoundIndex(filename));
	return 1;
}

// modelindex = et.G_ModelIndex( filename )
static int _et_G_ModelIndex(lua_State * L)
{
	const char     *filename = luaL_checkstring(L, 1);

	lua_pushinteger(L, G_ModelIndex((char *)filename));
	return 1;
}

// }}}

/* xreal
// Sound {{{
// et.G_globalSound( sound )
static int _et_G_globalSound(lua_State *L)
{
	const char *sound = luaL_checkstring(L, 1);
	G_globalSound((char *)sound);
	return 0;
}
*/

// et.G_Sound( entnum, soundindex )
static int _et_G_Sound(lua_State * L)
{
	int             entnum = luaL_checkint(L, 1);
	int             soundindex = luaL_checkint(L, 2);

	/* xreal
	G_Sound(g_entities + entnum, soundindex);
	*/
	G_Sound(g_entities + entnum, 0, soundindex);
	return 0;
}

/* xreal
// et.G_ClientSound( clientnum, soundindex )
static int _et_G_ClientSound( lua_State *L )
{
	int clientnum = luaL_checkint( L, 1 );
	int soundindex = luaL_checkint( L, 2 );
	G_ClientSound( g_entities + clientnum, soundindex );
	return 0;
}
*/
// }}}

// Miscellaneous {{{
// milliseconds = et.trap_Milliseconds()
static int _et_trap_Milliseconds(lua_State * L)
{
	lua_pushinteger(L, trap_Milliseconds());
	return 1;
}

// et.G_Damage( target, inflictor, attacker, damage, dflags, mod )
static int _et_G_Damage(lua_State * L)
{
	int             target = luaL_checkint(L, 1);
	int             inflictor = luaL_checkint(L, 2);
	int             attacker = luaL_checkint(L, 3);
	int             damage = luaL_checkint(L, 4);
	int             dflags = luaL_checkint(L, 5);
	int             mod = luaL_checkint(L, 6);

	G_Damage(g_entities + target, g_entities + inflictor, g_entities + attacker, NULL, NULL, damage, dflags, mod);

	return 0;
}

/* xreal
// flooding = et.ClientIsFlooding( clientnum )
static int _et_ClientIsFlooding(lua_State *L)
{
	int clientnum = luaL_checkint(L, 1);
	lua_pushinteger(L, ClientIsFlooding(g_entities + clientnum, qtrue));
	return 1;
}

// et.G_AddSkillPoints( ent, skill, points )
static int _et_G_AddSkillPoints(lua_State *L)
{
	gentity_t *ent = g_entities + luaL_checkint(L, 1);
	int skill = luaL_checkint(L, 2);
	float points = luaL_checknumber(L, 3);
	G_AddSkillPoints(ent, skill, points);
	return 0;
}

// et.G_LoseSkillPoints( ent, skill, points )
static int _et_G_LoseSkillPoints( lua_State *L )
{
	gentity_t *ent = g_entities + luaL_checkint( L, 1 );
	int skill = luaL_checkint( L, 2 );
	float points = luaL_checknumber( L, 3 );
	G_LoseSkillPoints( ent, skill, points );
	return 0;
}
// }}}
*/

// Entities {{{
// client entity fields
static const gentity_field_t gclient_fields[] = {
//xreal changes
//          NAME       ALIAS         TYPE        FLAGS
	_et_gclient_addfield(inactivityTime, FIELD_INT, 0),
	_et_gclient_addfield(inactivityWarning, FIELD_INT, 0),
	_et_gclient_addfield(pers.connected, FIELD_INT, 0),
	_et_gclient_addfield(pers.netname, FIELD_STRING, FIELD_FLAG_NOPTR),
	_et_gclient_addfield(pers.localClient, FIELD_INT, 0),
	_et_gclient_addfield(pers.initialSpawn, FIELD_INT, 0),
	_et_gclient_addfield(pers.enterTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.connectTime, FIELD_INT, 0),
	_et_gclient_addfield(pers.teamState.state, FIELD_INT, 0),
	_et_gclient_addfield(pers.voteCount, FIELD_INT, 0),
	_et_gclient_addfield(pers.teamVoteCount, FIELD_INT, 0),
	//_et_gclient_addfield(pers.complaints, FIELD_INT, 0),
	//_et_gclient_addfield(pers.complaintClient, FIELD_INT, 0),
	//_et_gclient_addfield(pers.complaintEndTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.lastReinforceTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.applicationClient, FIELD_INT, 0),
	//_et_gclient_addfield(pers.applicationEndTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.invitationClient, FIELD_INT, 0),
	//_et_gclient_addfield(pers.invitationEndTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.propositionClient, FIELD_INT, 0),
	//_et_gclient_addfield(pers.propositionClient2, FIELD_INT, 0),
	//_et_gclient_addfield(pers.propositionEndTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.autofireteamEndTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.autofireteamCreateEndTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.autofireteamJoinEndTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.lastSpawnTime, FIELD_INT, 0),
	//_et_gclient_addfield(pers.ready, FIELD_INT, 0),
	_et_gclient_addfield(pers.teamInfo, FIELD_INT, 0),	//xreal
	_et_gclient_addfield(pers.maxHealth, FIELD_INT, 0),	//xreal
	_et_gclient_addfield(pers.pmoveFixed, FIELD_INT, 0),	//xreal
	_et_gclient_addfield(pers.predictItemPickup, FIELD_INT, 0),	//xreal
	_et_gclient_addfield(ps.stats, FIELD_INT_ARRAY, 0),
	_et_gclient_addfield(ps.persistant, FIELD_INT_ARRAY, 0),
	_et_gclient_addfield(ps.ping, FIELD_INT, 0),
	_et_gclient_addfield(ps.powerups, FIELD_INT_ARRAY, 0),
	_et_gclient_addfield(ps.origin, FIELD_VEC3, 0),
	_et_gclient_addfield(ps.ammo, FIELD_INT_ARRAY, 0),
	//_et_gclient_addfield(ps.ammoclip, FIELD_INT_ARRAY, 0),
	_et_gclient_addfield(sess.sessionTeam, FIELD_INT, 0),
	_et_gclient_addfield(sess.spectatorTime, FIELD_INT, 0),
	_et_gclient_addfield(sess.spectatorState, FIELD_INT, 0),
	_et_gclient_addfield(sess.spectatorClient, FIELD_ENTITY, 0),
	_et_gclient_addfield(sess.wins, FIELD_INT, 0),	//xreal
	_et_gclient_addfield(sess.losses, FIELD_INT, 0),	//xreal
	_et_gclient_addfield(sess.teamLeader, FIELD_INT, 0),	//xreal
	// missing sess.latchSpectatorClient
	//_et_gclient_addfield(sess.playerType, FIELD_INT, 0),
	//_et_gclient_addfield(sess.playerWeapon, FIELD_INT, 0),
	//_et_gclient_addfield(sess.playerWeapon2, FIELD_INT, 0),
	//_et_gclient_addfield(sess.spawnObjectiveIndex, FIELD_INT, 0),
	//_et_gclient_addfield(sess.latchPlayerType, FIELD_INT, 0),
	//_et_gclient_addfield(sess.latchPlayerWeapon, FIELD_INT, 0),
	//_et_gclient_addfield(sess.latchPlayerWeapon2, FIELD_INT, 0),
	//_et_gclient_addfield(sess.damage_given, FIELD_INT, 0),
	//_et_gclient_addfield(sess.damage_received, FIELD_INT, 0),
	//_et_gclient_addfield(sess.deaths, FIELD_INT, 0),
	//_et_gclient_addfield(sess.game_points, FIELD_INT, 0),
	// missing sess.gibs
	//_et_gclient_addfield(sess.kills, FIELD_INT, 0),
	//_et_gclient_addfield(sess.medals, FIELD_INT_ARRAY, 0),
	//_et_gclient_addfieldalias(sess.muted, sess.auto_unmute_time, FIELD_INT, 0),
	//_et_gclient_addfield(sess.rank, FIELD_INT, 0),
	//_et_gclient_addfield(sess.referee, FIELD_INT, 0),
	//_et_gclient_addfield(sess.rounds, FIELD_INT, 0),
	// missing sess.semiadmin
	//_et_gclient_addfield(sess.skill, FIELD_INT_ARRAY, 0),
	//_et_gclient_addfield(sess.spec_invite, FIELD_INT, 0),
	//_et_gclient_addfield(sess.spec_team, FIELD_INT, 0),
	//_et_gclient_addfield(sess.suicides, FIELD_INT, 0),
	//_et_gclient_addfield(sess.team_kills, FIELD_INT, 0),
	//_et_gclient_addfield(sess.team_damage_given, FIELD_INT, 0),
	//_et_gclient_addfield(sess.team_damage_received, FIELD_INT, 0),
	// TODO: sess.aWeaponStats
	//_et_gclient_addfield(sess.aWeaponStats, FIELD_?_ARRAY, 0),

	// To be compatible with ETPro:
	//_et_gclient_addfieldalias(client.inactivityTime, inactivityTime, FIELD_INT, 0),
	//_et_gclient_addfieldalias(client.inactivityWarning, inactivityWarning, FIELD_INT, 0),
	// origin: use ps.origin instead of r.currentOrigin
	//         for client entities
	_et_gclient_addfieldalias(origin, ps.origin, FIELD_VEC3, 0),
	//_et_gclient_addfieldalias(sess.team_damage, sess.team_damage_given, FIELD_INT, 0),
	//_et_gclient_addfieldalias(sess.team_received, sess.team_damage_received, FIELD_INT, 0),

	// New to ETPub:
	//_et_gclient_addfield(sess.guid, FIELD_STRING, FIELD_FLAG_NOPTR + FIELD_FLAG_READONLY),
	//_et_gclient_addfield(sess.ip, FIELD_STRING, FIELD_FLAG_NOPTR + FIELD_FLAG_READONLY),
	//_et_gclient_addfield(sess.ignoreClients, FIELD_INT, 0),
	//_et_gclient_addfield(sess.skillpoints, FIELD_FLOAT_ARRAY, FIELD_FLAG_READONLY),
	//_et_gclient_addfield(pers.lastkilled_client, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gclient_addfield(pers.lastrevive_client, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gclient_addfield(pers.lastkiller_client, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gclient_addfield(pers.lastammo_client, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gclient_addfield(pers.lasthealth_client, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gclient_addfield(pers.lastrevive_client, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gclient_addfield(pers.lastkiller_client, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gclient_addfield(pers.lastammo_client, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gclient_addfield(pers.lasthealth_client, FIELD_INT, FIELD_FLAG_READONLY),

	{NULL},
};

// entity fields
static const gentity_field_t gentity_fields[] = {
// xreal changes
//                     NAME          ALIAS         TYPE         FLAGS
	_et_gentity_addfield(activator, FIELD_ENTITY, FIELD_FLAG_READONLY),
	_et_gentity_addfield(chain, FIELD_ENTITY, 0),
	_et_gentity_addfield(classname, FIELD_STRING, 0),
	_et_gentity_addfield(clipmask, FIELD_INT, 0),
	//_et_gentity_addfield(closespeed, FIELD_FLOAT, 0),
	_et_gentity_addfield(count, FIELD_INT, 0),
	//_et_gentity_addfield(count2, FIELD_INT, 0),
	_et_gentity_addfield(damage, FIELD_INT, 0),
	//_et_gentity_addfield(deathType, FIELD_INT, 0),
	//_et_gentity_addfield(delay, FIELD_FLOAT, 0),
	//_et_gentity_addfield(dl_atten, FIELD_INT, 0),
	//_et_gentity_addfield(dl_color, FIELD_VEC3, 0),
	//_et_gentity_addfield(dl_shader, FIELD_STRING, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(dl_stylestring, FIELD_STRING, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(duration, FIELD_FLOAT, 0),
	//_et_gentity_addfield(end_size, FIELD_INT, 0),
	_et_gentity_addfield(enemy, FIELD_ENTITY, 0),
	_et_gentity_addfield(flags, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(harc, FIELD_FLOAT, 0),
	_et_gentity_addfield(health, FIELD_INT, 0),
	_et_gentity_addfield(inuse, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(isProp, FIELD_INT, FIELD_FLAG_READONLY),
	_et_gentity_addfield(item, FIELD_STRING, 0),
	//_et_gentity_addfield(key, FIELD_INT, 0),
	_et_gentity_addfield(message, FIELD_STRING, 0),
	_et_gentity_addfield(methodOfDeath, FIELD_INT, 0),
	//_et_gentity_addfield(mg42BaseEnt, FIELD_INT, 0),
	//_et_gentity_addfield(missionLevel, FIELD_INT, 0),
	_et_gentity_addfield(model, FIELD_STRING, FIELD_FLAG_READONLY),
	_et_gentity_addfield(model2, FIELD_STRING, FIELD_FLAG_READONLY),
	_et_gentity_addfield(nextTrain, FIELD_ENTITY, 0),
	//_et_gentity_addfield(noise_index, FIELD_INT, 0),
	_et_gentity_addfield(prevTrain, FIELD_ENTITY, 0),
	//_et_gentity_addfield(props_frame_state, FIELD_INT, FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.absmax, FIELD_VEC3, FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.absmin, FIELD_VEC3, FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.bmodel, FIELD_INT, FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.contents, FIELD_INT, 0),
	_et_gentity_addfield(r.currentAngles, FIELD_VEC3, 0),
	_et_gentity_addfield(r.currentOrigin, FIELD_VEC3, 0),
	//_et_gentity_addfield(r.eventTime, FIELD_INT, 0),
	_et_gentity_addfield(r.linkcount, FIELD_INT, FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.linked, FIELD_INT, FIELD_FLAG_READONLY),
	_et_gentity_addfield(r.maxs, FIELD_VEC3, 0),
	_et_gentity_addfield(r.mins, FIELD_VEC3, 0),
	_et_gentity_addfield(r.ownerNum, FIELD_INT, 0),
	_et_gentity_addfield(r.singleClient, FIELD_INT, 0),
	_et_gentity_addfield(r.svFlags, FIELD_INT, 0),
	//_et_gentity_addfield(r.worldflags, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(radius, FIELD_INT, 0),
	_et_gentity_addfield(random, FIELD_FLOAT, 0),
	//_et_gentity_addfield(rotate, FIELD_VEC3, 0),
	_et_gentity_addfield(s.angles, FIELD_VEC3, 0),
	_et_gentity_addfield(s.angles2, FIELD_VEC3, 0),
	//_et_gentity_addfield(s.apos, FIELD_TRAJECTORY, 0),
	_et_gentity_addfield(s.apos.trBase, FIELD_VEC3, 0),
	_et_gentity_addfield(s.apos.trDelta, FIELD_VEC3, 0),
	_et_gentity_addfield(s.apos.trType, FIELD_INT, 0),
	_et_gentity_addfield(s.apos.trDuration, FIELD_INT, 0),
	_et_gentity_addfield(s.apos.trAcceleration, FIELD_FLOAT, 0),
	_et_gentity_addfield(s.clientNum, FIELD_INT, 0),
	_et_gentity_addfield(s.constantLight, FIELD_INT, 0),
	//_et_gentity_addfield(s.density, FIELD_INT, 0),
	//_et_gentity_addfield(s.dl_intensity, FIELD_INT, 0),
	//_et_gentity_addfield(s.dmgFlags, FIELD_INT, 0),
	_et_gentity_addfield(s.eFlags, FIELD_INT, 0),
	_et_gentity_addfield(s.eType, FIELD_INT, 0),
	//_et_gentity_addfield(s.effect1Time, FIELD_INT, 0),
	//_et_gentity_addfield(s.effect2Time, FIELD_INT, 0),
	//_et_gentity_addfield(s.effect3Time, FIELD_INT, 0),
	_et_gentity_addfield(s.frame, FIELD_INT, 0),
	_et_gentity_addfield(s.groundEntityNum, FIELD_INT, FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.loopSound, FIELD_INT, 0),
	_et_gentity_addfield(s.modelindex, FIELD_INT, 0),
	_et_gentity_addfield(s.modelindex2, FIELD_INT, 0),
	_et_gentity_addfield(s.number, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(s.onFireEnd, FIELD_INT, 0),
	//_et_gentity_addfield(s.onFireStart, FIELD_INT, 0),
	_et_gentity_addfield(s.origin, FIELD_VEC3, 0),
	_et_gentity_addfield(s.origin2, FIELD_VEC3, 0),
	//_et_gentity_addfield(s.pos, FIELD_TRAJECTORY, 0),
	_et_gentity_addfield(s.pos.trBase, FIELD_VEC3, 0),
	_et_gentity_addfield(s.pos.trDelta, FIELD_VEC3, 0),
	_et_gentity_addfield(s.pos.trType, FIELD_INT, 0),
	_et_gentity_addfield(s.pos.trDuration, FIELD_INT, 0),
	_et_gentity_addfield(s.pos.trAcceleration, FIELD_FLOAT, 0),
	_et_gentity_addfield(s.powerups, FIELD_INT, FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.solid, FIELD_INT, 0),
	//_et_gentity_addfield(s.teamNum, FIELD_INT, 0),
	_et_gentity_addfield(s.time, FIELD_INT, 0),
	_et_gentity_addfield(s.time2, FIELD_INT, 0),
	_et_gentity_addfield(s.weapon, FIELD_INT, FIELD_FLAG_READONLY),
	_et_gentity_addfield(s.eventParm, FIELD_INT, 0),
	//_et_gentity_addfield(scriptName, FIELD_STRING, FIELD_FLAG_READONLY),
	_et_gentity_addfield(spawnflags, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(spawnitem, FIELD_STRING, FIELD_FLAG_READONLY),
	_et_gentity_addfield(speed, FIELD_INT, 0),
	_et_gentity_addfield(splashDamage, FIELD_INT, 0),
	_et_gentity_addfield(splashMethodOfDeath, FIELD_INT, 0),
	_et_gentity_addfield(splashRadius, FIELD_INT, 0),
	//_et_gentity_addfield(start_size, FIELD_INT, 0),
	//_et_gentity_addfield(tagName, FIELD_STRING, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(tagParent, FIELD_ENTITY, 0),
	_et_gentity_addfield(takedamage, FIELD_INT, 0),
	//_et_gentity_addfield(tankLink, FIELD_ENTITY, 0),
	_et_gentity_addfield(target, FIELD_STRING, 0),
	_et_gentity_addfield(target_ent, FIELD_ENTITY, 0),
	//_et_gentity_addfield(TargetAngles, FIELD_VEC3, 0),
	//_et_gentity_addfield(TargetFlag, FIELD_INT, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(targetname, FIELD_STRING, FIELD_FLAG_READONLY),
	_et_gentity_addfield(teamchain, FIELD_ENTITY, 0),
	_et_gentity_addfield(teammaster, FIELD_ENTITY, 0),
	//_et_gentity_addfield(track, FIELD_STRING, FIELD_FLAG_READONLY),
	//_et_gentity_addfield(varc, FIELD_FLOAT, 0),
	_et_gentity_addfield(wait, FIELD_FLOAT, 0),
	_et_gentity_addfield(waterlevel, FIELD_INT, FIELD_FLAG_READONLY),
	_et_gentity_addfield(watertype, FIELD_INT, FIELD_FLAG_READONLY),

	// To be compatible with ETPro:
	// origin: use r.currentOrigin instead of ps.origin
	//         for non client entities
	_et_gentity_addfieldalias(origin, r.currentOrigin, FIELD_VEC3, 0),

	{NULL},
};

// gentity fields helper functions
gentity_field_t *_et_gentity_getfield(gentity_t * ent, char *fieldname)
{
	int             i;

	// search through client fields first
	if(ent->client)
	{
		for(i = 0; gclient_fields[i].name; i++)
		{
			if(Q_stricmp(fieldname, gclient_fields[i].name) == 0)
			{
				return (gentity_field_t *) & gclient_fields[i];
			}
		}
	}

	for(i = 0; gentity_fields[i].name; i++)
	{
		if(Q_stricmp(fieldname, gentity_fields[i].name) == 0)
		{
			return (gentity_field_t *) & gentity_fields[i];
		}
	}

	return 0;
}

void _et_gentity_getvec3(lua_State * L, vec3_t vec3)
{
	lua_newtable(L);
	lua_pushnumber(L, vec3[0]);
	lua_rawseti(L, -2, 1);
	lua_pushnumber(L, vec3[1]);
	lua_rawseti(L, -2, 2);
	lua_pushnumber(L, vec3[2]);
	lua_rawseti(L, -2, 3);
}

void _et_gentity_setvec3(lua_State * L, vec3_t * vec3)
{
	lua_pushnumber(L, 1);
	lua_gettable(L, -2);
	(*vec3)[0] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, 2);
	lua_gettable(L, -2);
	(*vec3)[1] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, 3);
	lua_gettable(L, -2);
	(*vec3)[2] = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
}

void _et_gentity_gettrajectory(lua_State * L, trajectory_t * traj)
{
	int             index;

	lua_newtable(L);
	index = lua_gettop(L);
	lua_pushstring(L, "trTime");
	lua_pushinteger(L, traj->trTime);
	lua_settable(L, -3);
	lua_pushstring(L, "trType");
	lua_pushinteger(L, traj->trType);
	lua_settable(L, -3);
	lua_pushstring(L, "trDuration");
	lua_pushinteger(L, traj->trDuration);
	lua_settable(L, -3);
	lua_settop(L, index);
	lua_pushstring(L, "trBase");
	_et_gentity_getvec3(L, traj->trBase);
	lua_settable(L, -3);
	lua_settop(L, index);
	lua_pushstring(L, "trDelta");
	_et_gentity_getvec3(L, traj->trDelta);
	lua_settable(L, -3);
}

void _et_gentity_settrajectory(lua_State * L, trajectory_t * traj)
{
	lua_pushstring(L, "trType");
	lua_gettable(L, -2);
	traj->trType = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trTime");
	lua_gettable(L, -2);
	traj->trTime = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trDuration");
	lua_gettable(L, -2);
	traj->trDuration = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushstring(L, "trBase");
	lua_gettable(L, -2);
	_et_gentity_setvec3(L, (vec3_t *) traj->trBase);
	lua_pop(L, 1);
	lua_pushstring(L, "trDelta");
	lua_gettable(L, -2);
	_et_gentity_setvec3(L, (vec3_t *) traj->trDelta);
	lua_pop(L, 1);
}

// entnum = et.G_Spawn()
static int _et_G_Spawn(lua_State * L)
{
	gentity_t      *entnum = G_Spawn();

	lua_pushinteger(L, entnum - g_entities);
	return 1;
}

// entnum = et.G_TempEntity( origin, event )
static int _et_G_TempEntity(lua_State * L)
{
	vec3_t          origin;
	vec_t          *target;
	int             event = luaL_checkint(L, 2);

	target = lua_getvector(L, 1);
	VectorCopy(target, origin);
	//lua_pop(L, 1);
	//_et_gentity_setvec3(L, &origin);
	lua_pushinteger(L, G_TempEntity(origin, event) - g_entities);
	return 1;
}

// et.G_FreeEntity( entnum )
static int _et_G_FreeEntity(lua_State * L)
{
	int             entnum = luaL_checkint(L, 1);

	G_FreeEntity(g_entities + entnum);
	return 0;
}

// TODO:
// spawnval = et.G_GetSpawnVar( entnum, key )
// et.G_SetSpawnVar( entnum, key, value )
// integer entnum = et.G_SpawnGEntityFromSpawnVars( string spawnvar, string spawnvalue, ... )

// et.trap_LinkEntity( entnum )
static int _et_trap_LinkEntity(lua_State * L)
{
	int             entnum = luaL_checkint(L, 1);

	trap_LinkEntity(g_entities + entnum);
	return 0;
}

// et.trap_UnlinkEntity( entnum )
static int _et_trap_UnlinkEntity(lua_State * L)
{
	int             entnum = luaL_checkint(L, 1);

	trap_UnlinkEntity(g_entities + entnum);
	return 0;
}

// (variable) = et.gentity_get( entnum, fieldname, arrayindex )
int _et_gentity_get(lua_State * L)
{
	gentity_t      *ent = g_entities + luaL_checkint(L, 1);
	const char     *fieldname = luaL_checkstring(L, 2);
	gentity_field_t *field = _et_gentity_getfield(ent, (char *)fieldname);
	unsigned long   addr;
	unsigned long   diff;

	// break on invalid gentity field
	if(!field)
	{
		luaL_error(L, "tried to get invalid gentity field \"%s\"", fieldname);
		return 0;
	}

	if(field->flags & FIELD_FLAG_GENTITY)
	{
		addr = (unsigned long)ent;
	}
	else
	{
		addr = (unsigned long)ent->client;
	}

	// for NULL entities, return nil (prevents server crashes!)
	if(!addr)
	{
		lua_pushnil(L);
		return 1;
	}

	addr += (unsigned long)field->mapping;

	switch (field->type)
	{
		case FIELD_INT:
			lua_pushinteger(L, *(int *)addr);
			return 1;
		case FIELD_STRING:
			if(field->flags & FIELD_FLAG_NOPTR)
			{
				lua_pushstring(L, (char *)addr);
			}
			else
			{
				lua_pushstring(L, *(char **)addr);
			}
			return 1;
		case FIELD_FLOAT:
			lua_pushnumber(L, *(float *)addr);
			return 1;
		case FIELD_ENTITY:
			diff = (unsigned long)&(**(gentity_t **) addr);
			if(diff == 0)
			{
				// Pointer to an entity is null
				lua_pushnil(L);
				return 1;
			}
			diff = (diff - (unsigned long)g_entities) / sizeof(gentity_t);
			// Push ent num
			lua_pushinteger(L, diff);
			//lua_pushinteger(L, (*(gentity_t **)addr) - g_entities);
			return 1;
		case FIELD_VEC3:
			lua_pushvector(L, *(vec3_t *) addr);
			//_et_gentity_getvec3(L, *(vec3_t *)addr);
			return 1;
		case FIELD_INT_ARRAY:
			lua_pushinteger(L, (*(int *)(addr + (sizeof(int) * luaL_optint(L, 3, 0)))));
			return 1;
		/*
		case FIELD_TRAJECTORY:
			_et_gentity_gettrajectory(L, (trajectory_t *)addr);
			return 1;
		*/
		case FIELD_FLOAT_ARRAY:
			lua_pushnumber(L, (*(float *)(addr + (sizeof(int) * luaL_optint(L, 3, 0)))));
			return 1;
	}
	return 0;
}

/*
// et.gentity_set( entnum, fieldname, arrayindex, (value) )
static int _et_gentity_set(lua_State *L)
{
	gentity_t *ent = g_entities + luaL_checkint(L, 1);
	const char *fieldname = luaL_checkstring(L, 2);
	gentity_field_t *field = _et_gentity_getfield(ent, (char *)fieldname);
	unsigned long addr;
	const char *buffer;
	
	// break on invalid gentity field
	if ( !field ) {
		luaL_error(L, "tried to set invalid gentity field \"%s\"", fieldname);
		return 0;
	}

	// break on read-only gentity field
	if ( field->flags & FIELD_FLAG_READONLY ) {
		luaL_error(L, "tried to set read-only gentity field \"%s\"", fieldname);
		return 0;
	}
	
	if ( field->flags & FIELD_FLAG_GENTITY ) {
		addr = (unsigned long)ent;
	} else {
		addr = (unsigned long)ent->client;
	}

	// for NULL entities, return nil (prevents server crashes!)
	if ( !addr ) {
		lua_pushnil(L);
		return 1;
	}

	addr += (unsigned long)field->mapping;
	
	switch ( field->type ) {
		case FIELD_INT:
			*(int *)addr = luaL_checkint(L, 3);
			break;
		case FIELD_STRING:
			buffer = luaL_checkstring(L, 3);
			if( field->flags & FIELD_FLAG_NOPTR ) {
				Q_strncpyz((char *)addr, buffer, strlen((char *)addr));
			} else {
				free(*(char **)addr);		
				*(char **)addr = malloc(strlen(buffer));
				Q_strncpyz(*(char **)addr, buffer, strlen(buffer));
			}
			break;
		case FIELD_FLOAT:
			*(float *)addr = (float)luaL_checknumber(L, 3);
			break;
		case FIELD_ENTITY:
			*(gentity_t **)addr = g_entities + luaL_checkint(L, 3);
			break;
		case FIELD_VEC3:
			_et_gentity_setvec3(L, (vec3_t *)addr);
			break;
		case FIELD_INT_ARRAY:
			*(int *)(addr + (sizeof(int) * luaL_checkint(L, 3))) = luaL_checkint(L, 4);
			break;
		case FIELD_TRAJECTORY:
			_et_gentity_settrajectory(L, (trajectory_t *)addr);
			break;
		case FIELD_FLOAT_ARRAY:
			*(float *)(addr + (sizeof(int) * luaL_checkint(L, 3))) = luaL_checknumber(L, 4);
			return 1;
	}
	return 0;
}
*/

// et.G_AddEvent( ent, event, eventparm )
static int _et_G_AddEvent(lua_State * L)
{
	int             ent = luaL_checkint(L, 1);
	int             event = luaL_checkint(L, 2);
	int             eventparm = luaL_checkint(L, 3);

	G_AddEvent(g_entities + ent, event, eventparm);
	return 0;
}

// }}}

/*
// Shrubbot {{{
// permission = et.G_shrubbot_permission( ent, flag )
static int _et_G_shrubbot_permission(lua_State *L)
{
	int entnum = luaL_optint(L, 1, -1);
	char flag = luaL_checkstring(L, 2)[0];
	gentity_t *ent = NULL;
	if ( entnum > -1 ) {
		ent = g_entities + entnum;
	}
	lua_pushinteger(L, G_shrubbot_permission(ent, flag));
	return 1;
}

// level = et.G_shrubbot_level( ent )
static int _et_G_shrubbot_level(lua_State *L)
{
	int entnum = luaL_optint(L, 1, -1);
	gentity_t *ent = NULL;
	if ( entnum > -1 ) {
		ent = g_entities + entnum;
	}
	lua_pushinteger(L, _shrubbot_level(ent));
	return 1;
}
// }}}
*/



// entNum = et.GetEntityByName(entName)
static int _et_GetEntityByName(lua_State * L)
{
	const char     *ename;
	gentity_t      *ent;

	ename = luaL_checkstring(L, 1);
	DEBUG_LUA("et_GetEntityByName: start: name=%s", ename);
	ent = G_Find(NULL, FOFS(name), ename);
	if(ent)
	{
		lua_pushinteger(L, ent->s.number);
		DEBUG_LUA("et_GetEntityByName: return: ent=%d", ent->s.number);
		return 1;
	}
	else
	{
		DEBUG_LUA("et_GetEntityByName: return: ent not found");
		return 0;
	}
}

// funcName = et.GetLuaFunction(entnum, keyname)
static int _et_GetLuaFunction(lua_State * L)
{
	gentity_t      *ent;
	const char     *key;
	char           *val = "";
	int             entnum;

	entnum = luaL_checkint(L, 1);
	key = luaL_checkstring(L, 2);
	DEBUG_LUA("et_GetLuaFunction: start: ent=%d key=%s", entnum, key);
	ent = &g_entities[entnum];
	if(ent)
	{
		if(!Q_stricmp(key, "luaDie"))
			val = ent->luaDie;
		else if(!Q_stricmp(key, "luaFree"))
			val = ent->luaFree;
		else if(!Q_stricmp(key, "luaHurt"))
			val = ent->luaHurt;
		else if(!Q_stricmp(key, "luaSpawn"))
			val = ent->luaSpawn;
		else if(!Q_stricmp(key, "luaThink"))
			val = ent->luaThink;
		else if(!Q_stricmp(key, "luaTouch"))
			val = ent->luaTouch;
		else if(!Q_stricmp(key, "luaTrigger"))
			val = ent->luaTrigger;
		else if(!Q_stricmp(key, "luaUse"))
			val = ent->luaUse;
		else
		{
			DEBUG_LUA("et_GetLuaFunction: return: key not found");
			return 0;
		}
	}
	DEBUG_LUA("et_GetLuaFunction: return: val=%s", val);
	lua_pushstring(L, (const char *)val);
	return 1;
}


// et.SetLuaFunction(entnum, keyname, value)
static int _et_SetLuaFunction(lua_State * L)
{
	gentity_t      *ent;
	const char     *key;
	char           *val;
	int             entnum;

	entnum = luaL_checkint(L, 1);
	key = luaL_checkstring(L, 2);
	val = (char *)luaL_checkstring(L, 3);
	DEBUG_LUA("et_SetLuaFunction: start: ent=%d key=%s val=%s", entnum, key, val);
	ent = &g_entities[entnum];
	if(ent)
	{
		if(!Q_stricmp(key, "luaDie"))
			ent->luaDie = val;
		else if(!Q_stricmp(key, "luaFree"))
			ent->luaFree = val;
		else if(!Q_stricmp(key, "luaHurt"))
			ent->luaHurt = val;
		else if(!Q_stricmp(key, "luaSpawn"))
			ent->luaSpawn = val;
		else if(!Q_stricmp(key, "luaThink"))
			ent->luaThink = val;
		else if(!Q_stricmp(key, "luaTouch"))
			ent->luaTouch = val;
		else if(!Q_stricmp(key, "luaTrigger"))
			ent->luaTrigger = val;
		else if(!Q_stricmp(key, "luaUse"))
			ent->luaUse = val;
		else
		{
			DEBUG_LUA("et_SetLuaFunction: return: key not found");
			return 0;
		}
	}
	DEBUG_LUA("et_SetLuaFunction: return: ent=%d key=%s", entnum, key);
	return 0;
}

// paramVal = et.GetLuaParam(entnum, paramnum)
static int _et_GetLuaParam(lua_State * L)
{
	gentity_t      *ent;
	int             entnum;
	int             paramnum;

	entnum = luaL_checkint(L, 1);
	paramnum = luaL_checkint(L, 2);
	DEBUG_LUA("et_GetLuaParam: start: ent=%d paramnum=%d", entnum, paramnum);
	ent = &g_entities[entnum];
	if(ent)
	{
		switch (paramnum)
		{
			case 1:
				lua_pushstring(L, ent->luaParam1);
				DEBUG_LUA("et_GetLuaParam: return: param1 value=%s", ent->luaParam1);
				break;
			case 2:
				lua_pushstring(L, ent->luaParam2);
				DEBUG_LUA("et_GetLuaParam: return: param2 value=%s", ent->luaParam2);
				break;
			case 3:
				lua_pushstring(L, ent->luaParam3);
				DEBUG_LUA("et_GetLuaParam: return: param3 value=%s", ent->luaParam3);
				break;
			case 4:
				lua_pushstring(L, ent->luaParam4);
				DEBUG_LUA("et_GetLuaParam: return: param4 value=%s", ent->luaParam4);
				break;
			default:
				DEBUG_LUA("et_GetLuaParam: return: invalid paramnum=%d", paramnum);
				return 0;
		}
	}
	return 1;
}

// success = et.RunThink(entnum)
// 1 if called, 0 if it doesn't have a think
static int _et_RunThink(lua_State * L)
{
	gentity_t      *ent;
	int             entnum;

	entnum = luaL_checkint(L, 1);
	DEBUG_LUA("et_RunThink: start: ent=%d", entnum);
	ent = &g_entities[entnum];
	if(ent)
	{
		if(ent->think)
		{
			ent->think(ent);
			//Success
			lua_pushinteger(L, 1);
			return 1;
		}
	}
	//Failure
	lua_pushinteger(L, 0);
	return 1;
}

// success = et.RunUse(entnum, other, activator)
// 1 if called, 0 if it doesn't have a use
static int _et_RunUse(lua_State * L)
{
	gentity_t      *ent;
	gentity_t      *other;
	gentity_t      *activator;
	int             entnum;
	int             othernum;
	int             activatornum;

	entnum = luaL_checkint(L, 1);
	othernum = luaL_checkint(L, 2);
	activatornum = luaL_checkint(L, 3);
	DEBUG_LUA("et_RunUse: start: ent=%d other=%d activator=%d", entnum, othernum, activatornum);
	ent = &g_entities[entnum];
	other = &g_entities[othernum];
	activator = &g_entities[activatornum];
	if(ent && other && activator)
	{
		if(ent->use)
		{
			ent->use(ent, other, activator);
			//Success
			lua_pushinteger(L, 1);
			return 1;
		}
	}
	//Failure
	lua_pushinteger(L, 0);
	return 1;
}

// et library initialisation array
static const luaL_Reg etlib[] = {
	// ET Library Calls
	{"RegisterModname", _et_RegisterModname},
	{"FindModname", _et_FindModname},
	{"FindSelf", _et_FindSelf},
	{"FindMod", _et_FindMod},
	{"IPCSend", _et_IPCSend},
	// Printing
	{"G_Print", _et_G_Print},
	{"G_LogPrint", _et_G_LogPrint},
	// Argument Handling
	{"ConcatArgs", _et_ConcatArgs},
	{"trap_Argc", _et_trap_Argc},
	{"trap_Argv", _et_trap_Argv},
	// Cvars
	{"trap_Cvar_Get", _et_trap_Cvar_Get},
	{"trap_Cvar_Set", _et_trap_Cvar_Set},
	// Config Strings
	{"trap_GetConfigstring", _et_trap_GetConfigstring},
	{"trap_SetConfigstring", _et_trap_SetConfigstring},
	// Server
	{"trap_SendConsoleCommand", _et_trap_SendConsoleCommand},
	// Clients
	//{"trap_DropClient",   _et_trap_DropClient},
	{"trap_SendServerCommand", _et_trap_SendServerCommand},
	//{"G_Say", _et_G_Say},
	{"ClientUserinfoChanged", _et_ClientUserinfoChanged},
	// Userinfo
	{"trap_GetUserinfo", _et_trap_GetUserinfo},
	{"trap_SetUserinfo", _et_trap_SetUserinfo},
	// String Utility Functions
	{"Info_RemoveKey", _et_Info_RemoveKey},
	{"Info_SetValueForKey", _et_Info_SetValueForKey},
	{"Info_ValueForKey", _et_Info_ValueForKey},
	{"Q_CleanStr", _et_Q_CleanStr},
	/*
	// ET Filesystem
	{"trap_FS_FOpenFile", _et_trap_FS_FOpenFile},
	{"trap_FS_Read", _et_trap_FS_Read},
	{"trap_FS_Write", _et_trap_FS_Write},
	{"trap_FS_Rename", _et_trap_FS_Rename},
	{"trap_FS_FCloseFile", _et_trap_FS_FCloseFile},
	*/
	// Indexes
	{"G_SoundIndex", _et_G_SoundIndex},
	{"G_ModelIndex", _et_G_ModelIndex},
	// Sound
	//{"G_globalSound", _et_G_globalSound},
	{"G_Sound", _et_G_Sound},
	// Miscellaneous
	{"trap_Milliseconds", _et_trap_Milliseconds},
	{"G_Damage", _et_G_Damage},
	//{"ClientIsFlooding", _et_ClientIsFlooding},
	//{"G_AddSkillPoints", _et_G_AddSkillPoints},
	//{"G_LoseSkillPoints", _et_G_LoseSkillPoints},
	// Entities
	{"G_Spawn", _et_G_Spawn},
	{"G_TempEntity", _et_G_TempEntity},
	{"G_FreeEntity", _et_G_FreeEntity},
	{"trap_LinkEntity", _et_trap_LinkEntity},
	{"trap_UnlinkEntity", _et_trap_UnlinkEntity},
	{"gentity_get", _et_gentity_get},
	//{"gentity_set", _et_gentity_set},
	{"G_AddEvent", _et_G_AddEvent},

	{"SetLuaFunction", _et_SetLuaFunction},
	{"GetLuaFunction", _et_GetLuaFunction},
	{"GetLuaParam", _et_GetLuaParam},
	{"GetEntityByName", _et_GetEntityByName},

	{"RunThink", _et_RunThink},
	{"RunUse", _et_RunUse},

	// Shrubbot
	//{"G_shrubbot_permission", _et_G_shrubbot_permission},
	//{"G_shrubbot_level", _et_G_shrubbot_level},

	{NULL},
};

int luaopen_et(lua_State * L)
{
	luaL_register(L, "et", etlib);

	return 1;
}

#endif
