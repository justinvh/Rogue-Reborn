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
// g_lua.c

#include "g_lua.h"

#ifdef G_LUA

// TODO: aiming for compatibility with ETPro lua mods
// http://wolfwiki.anime.net/index.php/Lua_Mod_API

lua_vm_t       *lVM[LUA_NUM_VM];

void QDECL DEBUG_LUA(const char *fmt, ...)
{
	va_list         argptr;
	char            text[1024];

	if(g_debugLua.integer >= 1)
	{
		va_start(argptr, fmt);
		Q_vsnprintf(text, sizeof(text), fmt, argptr);
		va_end(argptr);
		G_Printf("%s\n", text);
	}
}

void QDECL LOG(const char *fmt, ...)
{
	va_list         argptr;
	char            buff[1024], string[1024];
	int             min, tens, sec;

	va_start(argptr, fmt);
	Q_vsnprintf(buff, sizeof(buff), fmt, argptr);
	va_end(argptr);

	if(g_dedicated.integer)
	{
		trap_Printf(buff);
	}

	if(level.logFile)
	{
		/* xreal
		if ( g_logOptions.integer & LOGOPTS_REALTIME ) {
			Com_sprintf(string, sizeof(string), "%s %s", G_GetRealTime(), buff);
		} else {
		*/
			sec = level.time / 1000;
			min = sec / 60;
			sec -= min * 60;
			tens = sec / 10;
			sec -= tens * 10;

			Com_sprintf(string, sizeof(string), "%i:%i%i %s", min, tens, sec, buff);
		/* xreal
		}
		*/

		trap_FS_Write(string, strlen(string), level.logFile);
	}
}

/* xreal
void QDECL LOG(const char *fmt, ...)_attribute((format(printf,1,2)));
*/

/*************/
/* Lua API   */
/*************/


qboolean LoadLuaFile(char *path, int num_vm, vmType_t type)
{
	int             flen = 0;
	char           *code;
	//char           *signature;
	fileHandle_t    f;
	lua_vm_t       *vm;

	// try to open lua file
	flen = trap_FS_FOpenFile(path, &f, FS_READ);
	if(flen < 0)
	{
		LOG("Lua API: can not open file %s\n", path);
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	else if(flen > LUA_MAX_FSIZE)
	{
		// quad: Let's not load arbitrarily big files to memory.
		// If your lua file exceeds the limit, let me know.
		LOG("Lua API: ignoring file %s (too big)\n", path);
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	else
	{
		code = malloc(flen + 1);
		trap_FS_Read(code, flen, f);
		*(code + flen) = '\0';
		trap_FS_FCloseFile(f);
		//signature = sha1(code);

		/*
		if ( Q_stricmp(lua_allowedModules.string, "") &&
		!strstr(lua_allowedModules.string, signature) ) {
		// don't load disallowed lua modules into vm
		LOG("Lua API: Lua module [%s] [%s] disallowed by ACL\n", crt, signature);
		} else {
		*/
		// Init lua_vm_t struct
		vm = (lua_vm_t *) malloc(sizeof(lua_vm_t));
		if(vm == NULL)
		{
			LOG("Lua API: failed to allocate memory for lua VM\n", path);
			return qfalse;
		}
		memset(vm, 0, sizeof(lua_vm_t));
		vm->id = -1;
		Q_strncpyz(vm->file_name, path, sizeof(vm->file_name));
		Q_strncpyz(vm->mod_name, "", sizeof(vm->mod_name));
		Q_strncpyz(vm->mod_signature, "", sizeof(vm->mod_signature));
		//Q_strncpyz(vm->mod_signature, signature, sizeof(vm->mod_signature));
		vm->code = code;
		vm->code_size = flen;
		vm->type = type;
		vm->err = 0;

		// Start lua virtual machine
		if(G_LuaStartVM(vm) == qfalse)
		{
			G_LuaStopVM(vm);
			vm = NULL;
			return qfalse;
		}
		else
		{
			vm->id = num_vm;
			lVM[num_vm] = vm;
			return qtrue;
		}
		/*
		}
		*/
	}
	return qfalse;
}

//{{{
/** G_LuaInit()
* Initialises the Lua API interface 
*/
qboolean G_LuaInit()
{
	//load from cvar
	int             i, len, num_vm = 0;
	char            buff[MAX_CVAR_VALUE_STRING], *crt;

	//load from files
	int             numdirs;
	int             numFiles;
	char            filename[128];
	char            dirlist[1024];
	char           *dirptr;
	int             dirlen;

	G_Printf("------- G_LuaInit -------\n");

	if(lua_modules.string[0])
	{
		//Load lua modules from the cvar
		Q_strncpyz(buff, lua_modules.string, sizeof(buff));
		len = strlen(buff);
		crt = buff;

		for(i = 0; i < LUA_NUM_VM; i++)
			lVM[i] = NULL;

		for(i = 0; i <= len; i++)
		{
			if(buff[i] == ' ' || buff[i] == '\0' || buff[i] == ',' || buff[i] == ';')
			{
				buff[i] = '\0';
				if(LoadLuaFile(crt, num_vm, VMT_GAMESCRIPT))
				{
					num_vm++;
					G_Printf("  %s Loaded\n", crt);
				}
				else
				{
					G_Printf("  %s Failed\n", crt);
				}

				// prepare for next iteration
				if(i + 1 < len)
					crt = buff + i + 1;
				else
					crt = NULL;
				if(num_vm >= LUA_NUM_VM)
				{
					LOG("Lua API: too many lua files specified, only the first %d have been loaded\n", LUA_NUM_VM);
					break;
				}
			}
		}
	}

	//Load lua modules in global folder
	numFiles = 0;
	numdirs = trap_FS_GetFileList("scripts/lua/global/", ".lua", dirlist, 1024);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/lua/global/");
		strcat(filename, dirptr);
		numFiles++;

		// load the file
		//G_LoadLuaScript(NULL, filename);
		if(LoadLuaFile(filename, num_vm, VMT_GAMESCRIPT))
		{
			num_vm++;
			G_Printf("  %s Loaded\n", filename);
		}
		else
		{
			G_Printf("  %s Failed\n", filename);
		}

		if(num_vm >= LUA_NUM_VM)
		{
			LOG("Lua API: too many lua files specified, only the first %d have been loaded\n", LUA_NUM_VM);
			break;
		}
	}

	Com_Printf("%i global files parsed\n", numFiles);

	//*
	numFiles = 0;
	trap_Cvar_VariableStringBuffer("mapname", buff, sizeof(buff));
	sprintf(filename, "scripts/lua/%s/", buff);
	numdirs = trap_FS_GetFileList(filename, ".lua", dirlist, 1024);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/lua/");
		strcat(filename, buff);
		strcat(filename, "/");
		strcat(filename, dirptr);
		numFiles++;

		// load the file
		//G_LoadLuaScript(NULL, filename);
		if(LoadLuaFile(filename, num_vm, VMT_MAPSCRIPT))
		{
			num_vm++;
			G_Printf("  %s Loaded\n", filename);
		}
		else
		{
			G_Printf("  %s Failed\n", filename);
		}

		if(num_vm >= LUA_NUM_VM)
		{
			LOG("Lua API: too many lua files specified, only the first %d have been loaded\n", LUA_NUM_VM);
			break;
		}
	}

	Com_Printf("%i map files parsed\n", numFiles);
	//*/

	G_Printf("------- G_LuaInit Finish -------\n");

	return qtrue;
}

/** G_LuaCall( func, vm, nargs, nresults )
 * Calls a function already on the stack.
 */
qboolean G_LuaCall(lua_vm_t * vm, char *func, int nargs, int nresults)
{
	int             res = lua_pcall(vm->L, nargs, nresults, 0);

	if(res == LUA_ERRRUN)
	{
		// pheno: made output more ETPro compatible
		LOG("Lua API: %s error running lua script: %s\n", func, lua_tostring(vm->L, -1));
		lua_pop(vm->L, 1);
		vm->err++;
		return qfalse;
	}
	else if(res == LUA_ERRMEM)
	{
		LOG("Lua API: memory allocation error #2 ( %s )\n", vm->file_name);
		vm->err++;
		return qfalse;
	}
	else if(res == LUA_ERRERR)
	{
		LOG("Lua API: traceback error ( %s )\n", vm->file_name);
		vm->err++;
		return qfalse;
	}
	return qtrue;
}

/** G_LuaGetNamedFunction( vm, name )
 * Finds a function by name and puts it onto the stack.
 * If the function does not exist, returns qfalse.
 */
qboolean G_LuaGetNamedFunction(lua_vm_t * vm, char *name)
{
	if(vm->L)
	{
		lua_getglobal(vm->L, name);
		if(lua_isfunction(vm->L, -1))
		{
			return qtrue;
		}
		else
		{
			lua_pop(vm->L, 1);
			return qfalse;
		}
	}
	return qfalse;
}

/** G_LuaStartVM( vm )
 * Starts one individual virtual machine.
 */
qboolean G_LuaStartVM(lua_vm_t * vm)
{
	int             res = 0;
	char            homepath[MAX_QPATH], gamepath[MAX_QPATH];

	// Open a new lua state
	vm->L = luaL_newstate();
	if(!vm->L)
	{
		LOG("Lua API: Lua failed to initialise.\n");
		return qfalse;
	}

	// Initialise the lua state
	luaL_openlibs(vm->L);

	// set LUA_PATH and LUA_CPATH
	// TODO: add "fs_basepath/fs_game/?.lua;" to LUA_PATH
	//       and LUA_CPATH for linux machines
	trap_Cvar_VariableStringBuffer("fs_homepath", homepath, sizeof(homepath));
	trap_Cvar_VariableStringBuffer("fs_game", gamepath, sizeof(gamepath));

	lua_getglobal(vm->L, LUA_LOADLIBNAME);
	if(lua_istable(vm->L, -1))
	{
		lua_pushstring(vm->L, va("%s%s%s%s?.lua;%s%s%s%slualib%slua%s?.lua",
								 homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP,
								 homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, LUA_DIRSEP, LUA_DIRSEP));
		lua_setfield(vm->L, -2, "path");
		lua_pushstring(vm->L, va("%s%s%s%s?.%s;%s%s%s%slualib%sclibs%s?.%s",
								 homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, EXTENSION,
								 homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, LUA_DIRSEP, LUA_DIRSEP, EXTENSION));
		lua_setfield(vm->L, -2, "cpath");
	}
	lua_pop(vm->L, 1);

	// register globals
	lua_registerglobal(vm->L, "LUA_PATH", va("%s%s%s%s?.lua;%s%s%s%slualib%slua%s?.lua",
											 homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP,
											 homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, LUA_DIRSEP, LUA_DIRSEP));
	lua_registerglobal(vm->L, "LUA_CPATH", va("%s%s%s%s?.%s;%s%s%s%slualib%sclibs%s?.%s",
											  homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, EXTENSION,
											  homepath, LUA_DIRSEP, gamepath, LUA_DIRSEP, LUA_DIRSEP, LUA_DIRSEP, EXTENSION));
	lua_registerglobal(vm->L, "LUA_DIRSEP", LUA_DIRSEP);

	// register predefined constants
	lua_newtable(vm->L);
	lua_regconstinteger(vm->L, CS_PLAYERS);
	lua_regconstinteger(vm->L, EXEC_NOW);
	lua_regconstinteger(vm->L, EXEC_INSERT);
	lua_regconstinteger(vm->L, EXEC_APPEND);
	lua_regconstinteger(vm->L, FS_READ);
	lua_regconstinteger(vm->L, FS_WRITE);
	lua_regconstinteger(vm->L, FS_APPEND);
	lua_regconstinteger(vm->L, FS_APPEND_SYNC);
	lua_regconstinteger(vm->L, SAY_ALL);
	lua_regconstinteger(vm->L, SAY_TEAM);
	//xreal
	//lua_regconstinteger(vm->L, SAY_BUDDY);
	//lua_regconstinteger(vm->L, SAY_TEAMNL);
	lua_regconststring(vm->L, HOSTARCH);
	lua_setglobal(vm->L, "et");

	// register functions
	luaopen_et(vm->L);
	luaopen_game(vm->L);
	luaopen_qmath(vm->L);
	luaopen_mover(vm->L);
	luaopen_vector(vm->L);

	// Load the code
	res = luaL_loadbuffer(vm->L, vm->code, vm->code_size, vm->file_name);
	if(res == LUA_ERRSYNTAX)
	{
		LOG("Lua API: syntax error during pre-compilation: %s\n", lua_tostring(vm->L, -1));
		lua_pop(vm->L, 1);
		vm->err++;
		return qfalse;
	}
	else if(res == LUA_ERRMEM)
	{
		LOG("Lua API: memory allocation error #1 ( %s )\n", vm->file_name);
		vm->err++;
		return qfalse;
	}

	// Execute the code
	if(!G_LuaCall(vm, "G_LuaStartVM", 0, 0))
		return qfalse;

	LOG("Lua API: Loading %s\n", vm->file_name);
	return qtrue;
}

/** G_LuaStopVM( vm )
 * Stops one virtual machine, and calls its et_Quit callback. 
 */
void G_LuaStopVM(lua_vm_t * vm)
{
	if(vm == NULL)
		return;
	if(vm->code != NULL)
	{
		free(vm->code);
		vm->code = NULL;
	}
	if(vm->L)
	{
		if(G_LuaGetNamedFunction(vm, "et_Quit"))
			G_LuaCall(vm, "et_Quit", 0, 0);
		lua_close(vm->L);
		vm->L = NULL;
	}
	if(vm->id >= 0)
	{
		if(lVM[vm->id] == vm)
			lVM[vm->id] = NULL;
		if(!vm->err)
		{
			LOG("Lua API: Lua module [%s] [%s] unloaded.\n", vm->file_name, vm->mod_signature);
		}
	}
	free(vm);
}

/** G_LuaShutdown()
 * Shuts down everything related to Lua API.
 */
void G_LuaShutdown()
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			G_LuaStopVM(vm);
		}
	}
}

/** G_LuaStatus(ent)
 * Prints information on the Lua virtual machines.
 */
void G_LuaStatus(gentity_t * ent)
{
	int             i, cnt = 0;

	for(i = 0; i < LUA_NUM_VM; i++)
		if(lVM[i])
			cnt++;

	//G_Printf("Lua API: lua_status for client %d", (ent - g_entities));

	if(ent)
	{
		if(cnt == 0)
		{
			G_PrintfClient(ent, "Lua API: no scripts loaded.");
			return;
		}
		else if(cnt == 1)
		{
			G_PrintfClient(ent, "Lua API: showing lua information ( 1 module loaded )");
		}
		else
		{
			G_PrintfClient(ent, "Lua API: showing lua information ( %d modules loaded )", cnt);
		}
		G_PrintfClient(ent, "%-2s %-24s %-40s %-24s", "VM", "Modname", "Signature", "Filename");
		G_PrintfClient(ent, "-- ------------------------ ---------------------------------------- ------------------------");
		for(i = 0; i < LUA_NUM_VM; i++)
		{
			if(lVM[i])
			{
				G_PrintfClient(ent, "%2d %-24s %-40s %-24s", lVM[i]->id, lVM[i]->mod_name, lVM[i]->mod_signature,
							   lVM[i]->file_name);
			}
		}
		G_PrintfClient(ent, "-- ------------------------ ---------------------------------------- ------------------------");
	}
	else
	{
		if(cnt == 0)
		{
			G_Printf("Lua API: no scripts loaded.\n");
			return;
		}
		else if(cnt == 1)
		{
			G_Printf("Lua API: showing lua information ( 1 module loaded )\n");
		}
		else
		{
			G_Printf("Lua API: showing lua information ( %d modules loaded )\n", cnt);
		}
		G_Printf("%-2s %-24s %-40s %-24s\n", "VM", "Modname", "Signature", "Filename");
		G_Printf("-- ------------------------ ---------------------------------------- ------------------------\n");
		for(i = 0; i < LUA_NUM_VM; i++)
		{
			if(lVM[i])
			{
				G_Printf("%2d %-24s %-40s %-24s\n", lVM[i]->id, lVM[i]->mod_name, lVM[i]->mod_signature, lVM[i]->file_name);
			}
		}
		G_Printf("-- ------------------------ ---------------------------------------- ------------------------\n");
	}

}

/** G_LuaGetVM
 * Retrieves the VM for a given lua_State
 */
lua_vm_t       *G_LuaGetVM(lua_State * L)
{
	int             i;

	for(i = 0; i < LUA_NUM_VM; i++)
		if(lVM[i] && lVM[i]->L == L)
			return lVM[i];
	return NULL;
}

//}}}

/*****************************/
/* Lua API hooks / callbacks */
/*****************************/

//{{{

/** G_LuaHook_InitGame
 * et_InitGame( levelTime, randomSeed, restart ) callback 
 */
void G_LuaHook_InitGame(int levelTime, int randomSeed, int restart)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_InitGame"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, levelTime);
			lua_pushinteger(vm->L, randomSeed);
			lua_pushinteger(vm->L, restart);
			// Call
			if(!G_LuaCall(vm, "et_InitGame", 3, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/** G_LuaHook_ShutdownGame
 * et_ShutdownGame( restart )  callback 
 */
void G_LuaHook_ShutdownGame(int restart)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_ShutdownGame"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, restart);
			// Call
			if(!G_LuaCall(vm, "et_ShutdownGame", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/** G_LuaHook_RunFrame
 * et_RunFrame( levelTime )  callback
 */
void G_LuaHook_RunFrame(int levelTime)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_RunFrame"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, levelTime);
			// Call
			if(!G_LuaCall(vm, "et_RunFrame", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/** G_LuaHook_ClientConnect
 * rejectreason = et_ClientConnect( clientNum, firstTime, isBot ) callback
 */
qboolean G_LuaHook_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot, char *reason)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_ClientConnect"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			lua_pushinteger(vm->L, (int)firstTime);
			lua_pushinteger(vm->L, (int)isBot);
			// Call
			if(!G_LuaCall(vm, "et_ClientConnect", 3, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if(lua_isstring(vm->L, -1))
			{
				Q_strncpyz(reason, lua_tostring(vm->L, -1), MAX_STRING_CHARS);
				return qtrue;
			}
		}
	}
	return qfalse;
}

/** G_LuaHook_ClientDisconnect
 * et_ClientDisconnect( clientNum ) callback
 */
void G_LuaHook_ClientDisconnect(int clientNum)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_ClientDisconnect"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if(!G_LuaCall(vm, "et_ClientDisconnect", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/** G_LuaHook_ClientBegin
 * et_ClientBegin( clientNum ) callback
 */
void G_LuaHook_ClientBegin(int clientNum)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_ClientBegin"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if(!G_LuaCall(vm, "et_ClientBegin", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/** void G_LuaHook_ClientUserinfoChanged(int clientNum);
 * et_ClientUserinfoChanged( clientNum ) callback
 */
void G_LuaHook_ClientUserinfoChanged(int clientNum)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_ClientUserinfoChanged"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			// Call
			if(!G_LuaCall(vm, "et_ClientUserinfoChanged", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/** G_LuaHook_ClientSpawn
 * et_ClientSpawn( clientNum, revived, teamChange, restoreHealth ) callback
 */
//xreal most of the params gone.
void G_LuaHook_ClientSpawn(int clientNum)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_ClientSpawn"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			/*
			//xreal args gone.
			lua_pushinteger(vm->L, (int)revived);
			lua_pushinteger(vm->L, (int)teamChange);
			lua_pushinteger(vm->L, (int)restoreHealth);
			*/
			// Call
			if(!G_LuaCall(vm, "et_ClientSpawn", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/** G_LuaHook_ClientCommand
 * intercepted = et_ClientCommand( clientNum, command ) callback
 */
qboolean G_LuaHook_ClientCommand(int clientNum, char *command)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_ClientCommand"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, clientNum);
			lua_pushstring(vm->L, command);
			// Call
			if(!G_LuaCall(vm, "et_ClientCommand", 2, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if(lua_isnumber(vm->L, -1))
			{
				if(lua_tointeger(vm->L, -1) == 1)
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/** G_LuaHook_ConsoleCommand
 * intercepted = et_ConsoleCommand( command ) callback
 */
qboolean G_LuaHook_ConsoleCommand(char *command)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_ConsoleCommand"))
				continue;
			// Arguments
			lua_pushstring(vm->L, command);
			// Call
			if(!G_LuaCall(vm, "et_ConsoleCommand", 1, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if(lua_isnumber(vm->L, -1))
			{
				if(lua_tointeger(vm->L, -1) == 1)
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

#if 0
// xreal

/** G_LuaHook_UpgradeSkill
 * result = et_UpgradeSkill( cno, skill ) callback
 */
qboolean G_LuaHook_UpgradeSkill(int cno, skillType_t skill)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_UpgradeSkill"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, cno);
			lua_pushinteger(vm->L, (int)skill);
			// Call
			if(!G_LuaCall(vm, "et_UpgradeSkill", 2, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if(lua_isnumber(vm->L, -1))
			{
				if(lua_tointeger(vm->L, -1) == -1)
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/** G_LuaHook_SetPlayerSkill
 * et_SetPlayerSkill( cno, skill ) callback
 */
qboolean G_LuaHook_SetPlayerSkill(int cno, skillType_t skill)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_SetPlayerSkill"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, cno);
			lua_pushinteger(vm->L, (int)skill);
			// Call
			if(!G_LuaCall(vm, "et_SetPlayerSkill", 2, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if(lua_isnumber(vm->L, -1))
			{
				if(lua_tointeger(vm->L, -1) == -1)
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}
#endif

/** G_LuaHook_Print
 * et_Print( text ) callback
 */
void G_LuaHook_Print(char *text)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_Print"))
				continue;
			// Arguments
			lua_pushstring(vm->L, text);
			// Call
			if(!G_LuaCall(vm, "et_Print", 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
}

/** G_LuaHook_Obituary
 * (customObit) = et_Obituary( victim, killer, meansOfDeath ) callback
 */
qboolean G_LuaHook_Obituary(int victim, int killer, int meansOfDeath, char *customObit)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm)
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, "et_Obituary"))
				continue;
			// Arguments
			lua_pushinteger(vm->L, victim);
			lua_pushinteger(vm->L, killer);
			lua_pushinteger(vm->L, meansOfDeath);
			// Call
			if(!G_LuaCall(vm, "et_Obituary", 3, 1))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
			if(lua_isstring(vm->L, -1))
			{
				Q_strncpyz(customObit, lua_tostring(vm->L, -1), MAX_STRING_CHARS);
				return qtrue;
			}
		}
	}
	return qfalse;
}

//}}}

/*
luaThink(ent)
luaTouch(ent, other)
luaUse(ent, other)
luaHurt(ent, inflictor, attacker)
luaDie(ent, inflictor, attacker, damage, mod)
luaFree(ent)
luaTrigger(ent, other)
luaSpawn(ent)
*/

/** G_LuaHook_EntityThink
 * <function>( entitynum );
 */
qboolean G_LuaHook_EntityThink(char *function, int entity)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm && (vm->type == VMT_MAPSCRIPT))
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, function))
				continue;
			// Arguments
			lua_pushinteger(vm->L, entity);
			// Call
			if(!G_LuaCall(vm, function, 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
	return qfalse;
}

/** G_LuaHook_EntityTouch
 * <function>( entitynum, other );
 */
qboolean G_LuaHook_EntityTouch(char *function, int entity, int other)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm && (vm->type == VMT_MAPSCRIPT))
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, function))
				continue;
			// Arguments
			lua_pushinteger(vm->L, entity);
			lua_pushinteger(vm->L, other);
			// Call
			if(!G_LuaCall(vm, function, 2, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
	return qfalse;
}

/** G_LuaHook_EntityUse
 * <function>( entitynum, other, activator );
 */
qboolean G_LuaHook_EntityUse(char *function, int entity, int other, int activator)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm && (vm->type == VMT_MAPSCRIPT))
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, function))
				continue;
			// Arguments
			lua_pushinteger(vm->L, entity);
			lua_pushinteger(vm->L, other);
			lua_pushinteger(vm->L, activator);
			// Call
			if(!G_LuaCall(vm, function, 3, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
	return qfalse;
}

/** G_LuaHook_EntityHurt
 * <function>( entitynum, inflictor, attacker );
 */
qboolean G_LuaHook_EntityHurt(char *function, int entity, int inflictor, int attacker)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm && (vm->type == VMT_MAPSCRIPT))
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, function))
				continue;
			// Arguments
			lua_pushinteger(vm->L, entity);
			lua_pushinteger(vm->L, inflictor);
			lua_pushinteger(vm->L, attacker);
			// Call
			if(!G_LuaCall(vm, function, 3, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
	return qfalse;
}

/** G_LuaHook_EntityDie
 * <function>( entitynum, inflictor, attacker, damage, mod );
 */
qboolean G_LuaHook_EntityDie(char *function, int entity, int inflictor, int attacker, int dmg, int mod)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm && (vm->type == VMT_MAPSCRIPT))
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, function))
				continue;
			// Arguments
			lua_pushinteger(vm->L, entity);
			lua_pushinteger(vm->L, inflictor);
			lua_pushinteger(vm->L, attacker);
			lua_pushinteger(vm->L, dmg);
			lua_pushinteger(vm->L, mod);
			// Call
			if(!G_LuaCall(vm, function, 5, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
	return qfalse;
}

/** G_LuaHook_EntityFree
 * <function>( entitynum );
 */
qboolean G_LuaHook_EntityFree(char *function, int entity)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm && (vm->type == VMT_MAPSCRIPT))
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, function))
				continue;
			// Arguments
			lua_pushinteger(vm->L, entity);
			// Call
			if(!G_LuaCall(vm, function, 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
	return qfalse;
}

/** G_LuaHook_EntityTrigger
 * <function>( entitynum, other );
 */
qboolean G_LuaHook_EntityTrigger(char *function, int entity, int other)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm && (vm->type == VMT_MAPSCRIPT))
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, function))
				continue;
			// Arguments
			lua_pushinteger(vm->L, entity);
			lua_pushinteger(vm->L, other);
			// Call
			if(!G_LuaCall(vm, function, 2, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
	return qfalse;
}

/** G_LuaHook_EntitySpawn
 * <function>( entitynum );
 */
qboolean G_LuaHook_EntitySpawn(char *function, int entity)
{
	int             i;
	lua_vm_t       *vm;

	for(i = 0; i < LUA_NUM_VM; i++)
	{
		vm = lVM[i];
		if(vm && (vm->type == VMT_MAPSCRIPT))
		{
			if(vm->id < 0 /*|| vm->err */ )
				continue;
			if(!G_LuaGetNamedFunction(vm, function))
				continue;
			// Arguments
			lua_pushinteger(vm->L, entity);
			// Call
			if(!G_LuaCall(vm, function, 1, 0))
			{
				//G_LuaStopVM(vm);
				continue;
			}
			// Return values
		}
	}
	return qfalse;
}

#endif
