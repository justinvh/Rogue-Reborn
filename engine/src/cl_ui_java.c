/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2009-2010 Robert Beckebans <trebor_7@users.sourceforge.net>

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

#include <hat/engine/client.h>

#if defined(USE_JAVA)

#include "../qcommon/vm_java.h"
//#include "java/xreal_client_Client.h"

/*
====================
GetClientState
====================
*/
static void GetClientState(uiClientState_t * state)
{
	state->connectPacketCount = clc.connectPacketCount;
	state->connState = cls.state;
	Q_strncpyz(state->servername, cls.servername, sizeof(state->servername));
	Q_strncpyz(state->updateInfoString, cls.updateInfoString, sizeof(state->updateInfoString));
	Q_strncpyz(state->messageString, clc.serverMessage, sizeof(state->messageString));
	state->clientNum = cl.snap.ps.clientNum;
}

/*
====================
LAN_LoadCachedServers
====================
*/
void LAN_LoadCachedServers(void)
{
	int             size;
	fileHandle_t    fileIn;

	cls.numglobalservers = cls.numfavoriteservers = 0;
	cls.numGlobalServerAddresses = 0;
	if(FS_SV_FOpenFileRead("servercache.dat", &fileIn))
	{
		FS_Read(&cls.numglobalservers, sizeof(int), fileIn);
		FS_Read(&cls.numfavoriteservers, sizeof(int), fileIn);
		FS_Read(&size, sizeof(int), fileIn);
		if(size == sizeof(cls.globalServers) + sizeof(cls.favoriteServers))
		{
			FS_Read(&cls.globalServers, sizeof(cls.globalServers), fileIn);
			FS_Read(&cls.favoriteServers, sizeof(cls.favoriteServers), fileIn);
		}
		else
		{
			cls.numglobalservers = cls.numfavoriteservers = 0;
			cls.numGlobalServerAddresses = 0;
		}
		FS_FCloseFile(fileIn);
	}
}

/*
====================
LAN_SaveServersToCache
====================
*/
void LAN_SaveServersToCache(void)
{
	int             size;
	fileHandle_t    fileOut = FS_SV_FOpenFileWrite("servercache.dat");
	FS_Write(&cls.numglobalservers, sizeof(int), fileOut);
	FS_Write(&cls.numfavoriteservers, sizeof(int), fileOut);
	size = sizeof(cls.globalServers) + sizeof(cls.favoriteServers);
	FS_Write(&size, sizeof(int), fileOut);
	FS_Write(&cls.globalServers, sizeof(cls.globalServers), fileOut);
	FS_Write(&cls.favoriteServers, sizeof(cls.favoriteServers), fileOut);
	FS_FCloseFile(fileOut);
}


/*
====================
LAN_ResetPings
====================
*/
static void LAN_ResetPings(int source)
{
	int             count, i;
	serverInfo_t   *servers = NULL;

	count = 0;

	switch (source)
	{
		case AS_LOCAL:
			servers = &cls.localServers[0];
			count = MAX_OTHER_SERVERS;
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			servers = &cls.globalServers[0];
			count = MAX_GLOBAL_SERVERS;
			break;
		case AS_FAVORITES:
			servers = &cls.favoriteServers[0];
			count = MAX_OTHER_SERVERS;
			break;
	}
	if(servers)
	{
		for(i = 0; i < count; i++)
		{
			servers[i].ping = -1;
		}
	}
}

/*
====================
LAN_AddServer
====================
*/
static int LAN_AddServer(int source, const char *name, const char *address)
{
	int             max, *count, i;
	netadr_t        adr;
	serverInfo_t   *servers = NULL;

	max = MAX_OTHER_SERVERS;
	count = NULL;

	switch (source)
	{
		case AS_LOCAL:
			count = &cls.numlocalservers;
			servers = &cls.localServers[0];
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			max = MAX_GLOBAL_SERVERS;
			count = &cls.numglobalservers;
			servers = &cls.globalServers[0];
			break;
		case AS_FAVORITES:
			count = &cls.numfavoriteservers;
			servers = &cls.favoriteServers[0];
			break;
	}
	if(servers && *count < max)
	{
		NET_StringToAdr(address, &adr, NA_IP);
		for(i = 0; i < *count; i++)
		{
			if(NET_CompareAdr(servers[i].adr, adr))
			{
				break;
			}
		}
		if(i >= *count)
		{
			servers[*count].adr = adr;
			Q_strncpyz(servers[*count].hostName, name, sizeof(servers[*count].hostName));
			servers[*count].visible = qtrue;
			(*count)++;
			return 1;
		}
		return 0;
	}
	return -1;
}

/*
====================
LAN_RemoveServer
====================
*/
static void LAN_RemoveServer(int source, const char *addr)
{
	int            *count, i;
	serverInfo_t   *servers = NULL;

	count = NULL;
	switch (source)
	{
		case AS_LOCAL:
			count = &cls.numlocalservers;
			servers = &cls.localServers[0];
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			count = &cls.numglobalservers;
			servers = &cls.globalServers[0];
			break;
		case AS_FAVORITES:
			count = &cls.numfavoriteservers;
			servers = &cls.favoriteServers[0];
			break;
	}
	if(servers)
	{
		netadr_t        comp;

		NET_StringToAdr(addr, &comp, NA_IP);
		for(i = 0; i < *count; i++)
		{
			if(NET_CompareAdr(comp, servers[i].adr))
			{
				int             j = i;

				while(j < *count - 1)
				{
					Com_Memcpy(&servers[j], &servers[j + 1], sizeof(servers[j]));
					j++;
				}
				(*count)--;
				break;
			}
		}
	}
}


/*
====================
LAN_GetServerCount
====================
*/
static int LAN_GetServerCount(int source)
{
	switch (source)
	{
		case AS_LOCAL:
			return cls.numlocalservers;
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			return cls.numglobalservers;
			break;
		case AS_FAVORITES:
			return cls.numfavoriteservers;
			break;
	}
	return 0;
}

/*
====================
LAN_GetLocalServerAddressString
====================
*/
static void LAN_GetServerAddressString(int source, int n, char *buf, int buflen)
{
	switch (source)
	{
		case AS_LOCAL:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				Q_strncpyz(buf, NET_AdrToStringwPort(cls.localServers[n].adr), buflen);
				return;
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			if(n >= 0 && n < MAX_GLOBAL_SERVERS)
			{
				Q_strncpyz(buf, NET_AdrToStringwPort(cls.globalServers[n].adr), buflen);
				return;
			}
			break;
		case AS_FAVORITES:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				Q_strncpyz(buf, NET_AdrToStringwPort(cls.favoriteServers[n].adr), buflen);
				return;
			}
			break;
	}
	buf[0] = '\0';
}

/*
====================
LAN_GetServerInfo
====================
*/
static void LAN_GetServerInfo(int source, int n, char *buf, int buflen)
{
	char            info[MAX_STRING_CHARS];
	serverInfo_t   *server = NULL;

	info[0] = '\0';
	switch (source)
	{
		case AS_LOCAL:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				server = &cls.localServers[n];
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			if(n >= 0 && n < MAX_GLOBAL_SERVERS)
			{
				server = &cls.globalServers[n];
			}
			break;
		case AS_FAVORITES:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				server = &cls.favoriteServers[n];
			}
			break;
	}
	if(server && buf)
	{
		buf[0] = '\0';
		Info_SetValueForKey(info, "hostname", server->hostName);
		Info_SetValueForKey(info, "mapname", server->mapName);
		Info_SetValueForKey(info, "clients", va("%i", server->clients));
		Info_SetValueForKey(info, "sv_maxclients", va("%i", server->maxClients));
		Info_SetValueForKey(info, "ping", va("%i", server->ping));
		Info_SetValueForKey(info, "minping", va("%i", server->minPing));
		Info_SetValueForKey(info, "maxping", va("%i", server->maxPing));
		Info_SetValueForKey(info, "game", server->game);
		Info_SetValueForKey(info, "gametype", va("%i", server->gameType));
		Info_SetValueForKey(info, "nettype", va("%i", server->netType));
		Info_SetValueForKey(info, "addr", NET_AdrToString(server->adr));
		Q_strncpyz(buf, info, buflen);
	}
	else
	{
		if(buf)
		{
			buf[0] = '\0';
		}
	}
}

/*
====================
LAN_GetServerPing
====================
*/
static int LAN_GetServerPing(int source, int n)
{
	serverInfo_t   *server = NULL;

	switch (source)
	{
		case AS_LOCAL:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				server = &cls.localServers[n];
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			if(n >= 0 && n < MAX_GLOBAL_SERVERS)
			{
				server = &cls.globalServers[n];
			}
			break;
		case AS_FAVORITES:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				server = &cls.favoriteServers[n];
			}
			break;
	}
	if(server)
	{
		return server->ping;
	}
	return -1;
}

/*
====================
LAN_GetServerPtr
====================
*/
static serverInfo_t *LAN_GetServerPtr(int source, int n)
{
	switch (source)
	{
		case AS_LOCAL:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				return &cls.localServers[n];
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			if(n >= 0 && n < MAX_GLOBAL_SERVERS)
			{
				return &cls.globalServers[n];
			}
			break;
		case AS_FAVORITES:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				return &cls.favoriteServers[n];
			}
			break;
	}
	return NULL;
}

/*
====================
LAN_CompareServers
====================
*/
static int LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2)
{
	int             res;
	serverInfo_t   *server1, *server2;

	server1 = LAN_GetServerPtr(source, s1);
	server2 = LAN_GetServerPtr(source, s2);
	if(!server1 || !server2)
	{
		return 0;
	}

	res = 0;
	switch (sortKey)
	{
		case SORT_HOST:
			res = Q_stricmp(server1->hostName, server2->hostName);
			break;

		case SORT_MAP:
			res = Q_stricmp(server1->mapName, server2->mapName);
			break;
		case SORT_CLIENTS:
			if(server1->clients < server2->clients)
			{
				res = -1;
			}
			else if(server1->clients > server2->clients)
			{
				res = 1;
			}
			else
			{
				res = 0;
			}
			break;
		case SORT_GAME:
			if(server1->gameType < server2->gameType)
			{
				res = -1;
			}
			else if(server1->gameType > server2->gameType)
			{
				res = 1;
			}
			else
			{
				res = 0;
			}
			break;
		case SORT_PING:
			if(server1->ping < server2->ping)
			{
				res = -1;
			}
			else if(server1->ping > server2->ping)
			{
				res = 1;
			}
			else
			{
				res = 0;
			}
			break;
	}

	if(sortDir)
	{
		if(res < 0)
			return 1;
		if(res > 0)
			return -1;
		return 0;
	}
	return res;
}

/*
====================
LAN_GetPingQueueCount
====================
*/
static int LAN_GetPingQueueCount(void)
{
	return (CL_GetPingQueueCount());
}

/*
====================
LAN_ClearPing
====================
*/
static void LAN_ClearPing(int n)
{
	CL_ClearPing(n);
}

/*
====================
LAN_GetPing
====================
*/
static void LAN_GetPing(int n, char *buf, int buflen, int *pingtime)
{
	CL_GetPing(n, buf, buflen, pingtime);
}

/*
====================
LAN_GetPingInfo
====================
*/
static void LAN_GetPingInfo(int n, char *buf, int buflen)
{
	CL_GetPingInfo(n, buf, buflen);
}

/*
====================
LAN_MarkServerVisible
====================
*/
static void LAN_MarkServerVisible(int source, int n, qboolean visible)
{
	if(n == -1)
	{
		int             count = MAX_OTHER_SERVERS;
		serverInfo_t   *server = NULL;

		switch (source)
		{
			case AS_LOCAL:
				server = &cls.localServers[0];
				break;
			case AS_MPLAYER:
			case AS_GLOBAL:
				server = &cls.globalServers[0];
				count = MAX_GLOBAL_SERVERS;
				break;
			case AS_FAVORITES:
				server = &cls.favoriteServers[0];
				break;
		}
		if(server)
		{
			for(n = 0; n < count; n++)
			{
				server[n].visible = visible;
			}
		}

	}
	else
	{
		switch (source)
		{
			case AS_LOCAL:
				if(n >= 0 && n < MAX_OTHER_SERVERS)
				{
					cls.localServers[n].visible = visible;
				}
				break;
			case AS_MPLAYER:
			case AS_GLOBAL:
				if(n >= 0 && n < MAX_GLOBAL_SERVERS)
				{
					cls.globalServers[n].visible = visible;
				}
				break;
			case AS_FAVORITES:
				if(n >= 0 && n < MAX_OTHER_SERVERS)
				{
					cls.favoriteServers[n].visible = visible;
				}
				break;
		}
	}
}


/*
=======================
LAN_ServerIsVisible
=======================
*/
static int LAN_ServerIsVisible(int source, int n)
{
	switch (source)
	{
		case AS_LOCAL:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				return cls.localServers[n].visible;
			}
			break;
		case AS_MPLAYER:
		case AS_GLOBAL:
			if(n >= 0 && n < MAX_GLOBAL_SERVERS)
			{
				return cls.globalServers[n].visible;
			}
			break;
		case AS_FAVORITES:
			if(n >= 0 && n < MAX_OTHER_SERVERS)
			{
				return cls.favoriteServers[n].visible;
			}
			break;
	}
	return qfalse;
}

/*
=======================
LAN_UpdateVisiblePings
=======================
*/
qboolean LAN_UpdateVisiblePings(int source)
{
	return CL_UpdateVisiblePings_f(source);
}

/*
====================
LAN_GetServerStatus
====================
*/
int LAN_GetServerStatus(char *serverAddress, char *serverStatus, int maxLen)
{
	return CL_ServerStatus(serverAddress, serverStatus, maxLen);
}

/*
====================
CL_GetClipboardData
====================
*/
static void CL_GetClipboardData(char *buf, int buflen)
{
	char           *cbd;

	cbd = Sys_GetClipboardData();

	if(!cbd)
	{
		*buf = 0;
		return;
	}

	Q_strncpyz(buf, cbd, buflen);

	Z_Free(cbd);
}




// ====================================================================================


static jclass class_EntityState = NULL;
static jmethodID method_EntityState_ctor = NULL;

static jclass class_PlayerState = NULL;
static jmethodID method_PlayerState_ctor = NULL;

static jclass   class_Snapshot = NULL;
static jmethodID method_Snapshot_ctor = NULL;

void Snapshot_javaRegister()
{
	class_EntityState = (*javaEnv)->FindClass(javaEnv, "xreal/client/EntityState");
	if(CheckException() || !class_EntityState)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.client.EntityState");
	}

	/*
		EntityState(int number, int eType, int eFlags,
			Trajectory pos, Trajectory apos,
			int time, int time2,
			Vector3f origin, Vector3f origin2,
			Angle3f angles, Angle3f angles2,
			int otherEntityNum, int otherEntityNum2, int groundEntityNum,
			int constantLight, int loopSound, int modelindex, int modelindex2,
			int clientNum, int frame, int solid, int event, int eventParm,
			int powerups, int weapon, int legsAnim, int torsoAnim, int generic1) {
	 */


	method_EntityState_ctor = (*javaEnv)->GetMethodID(javaEnv, class_EntityState, "<init>",
			"(III"
			"Lxreal/Trajectory;Lxreal/Trajectory;"
			"II"
			"Ljavax/vecmath/Vector3f;Ljavax/vecmath/Vector3f;"
			"Lxreal/Angle3f;Lxreal/Angle3f;"
			"III"
			"IIII"
			"IIIII"
			"IIIII"
			")V");

	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't find constructor of xreal.client.EntityState");
	}



	class_PlayerState = (*javaEnv)->FindClass(javaEnv, "xreal/client/PlayerState");
	if(CheckException() || !class_PlayerState)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.client.PlayerState");
	}

	/*
	 public PlayerState(int commandTime, int pmType, int pmFlags, int pmTime,
			int bobCycle, Vector3f origin, Vector3f velocity, int weaponTime,
			int gravity, int speed, int deltaPitch, int deltaYaw,
			int deltaRoll, int groundEntityNum, int legsTimer, int legsAnim,
			int torsoTimer, int torsoAnim, int movementDir,
			Vector3f grapplePoint, int eFlags, int eventSequence, int[] events,
			int[] eventParms, int externalEvent, int externalEventParm,
			int externalEventTime, int clientNum, int weapon, int weaponState,
			Angle3f viewAngles, int viewHeight, int damageEvent, int damageYaw,
			int damagePitch, int damageCount, int[] stats, int[] persistant,
			int[] powerups, int[] ammo, int generic1, int loopSound,
			int jumppadEnt, int ping, int pmoveFramecount, int jumppadFrame,
			int entityEventSequence) {
	 */
	method_PlayerState_ctor = (*javaEnv)->GetMethodID(javaEnv, class_PlayerState, "<init>",
			"(IIII"
			"ILjavax/vecmath/Vector3f;Ljavax/vecmath/Vector3f;I"
			"IIII"
			"IIII"
			"III"
			"Ljavax/vecmath/Vector3f;II[I"
			"[III"
			"IIII"
			"Lxreal/Angle3f;III"
			"II[I[I"
			"[I[III"
			"IIII"
			"I"
			")V");

	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't find constructor of xreal.client.PlayerState");
	}



	class_Snapshot = (*javaEnv)->FindClass(javaEnv, "xreal/client/Snapshot");
	if(CheckException() || !class_Snapshot)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.client.Snapshot");
	}

	/*
	public Snapshot(int snapFlags, int ping, int serverTime,
					byte areamask[], PlayerState ps, EntityState[] entities,
					int numServerCommands, int serverCommandSequence)
	 */
	method_Snapshot_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Snapshot, "<init>",
			"(III"
			"[BLxreal/client/PlayerState;[Lxreal/client/EntityState;"
			"I"
			")V");

	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't find constructor of xreal.client.Snapshot");
	}
}

void Snapshot_javaDetach()
{
	if(class_EntityState)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_EntityState);
		class_EntityState = NULL;
	}

	if(class_PlayerState)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_PlayerState);
		class_PlayerState = NULL;
	}

	if(class_Snapshot)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Snapshot);
		class_Snapshot = NULL;
	}
}

static jobject Java_NewEntityState(const entityState_t * ent)
{
	jobject obj = NULL;

	if(class_EntityState)
	{
		/*
		public EntityState(int number, int eType, int eFlags,
			Trajectory pos, Trajectory apos,
			int time, int time2,
			Vector3f origin, Vector3f origin2,
			Angle3f angles, Angle3f angles2,
			int otherEntityNum, int otherEntityNum2, int groundEntityNum,
			int constantLight, int loopSound, int modelindex, int modelindex2,
			int clientNum, int frame, int solid, int event, int eventParm,
			int powerups, int weapon, int legsAnim, int torsoAnim, int generic1) {
		*/
		obj = (*javaEnv)->NewObject(javaEnv, class_EntityState, method_EntityState_ctor,
				ent->number,

				ent->eType,
				ent->eFlags,

				Java_NewTrajectory(&ent->pos),
				Java_NewTrajectory(&ent->apos),

				ent->time,
				ent->time2,

				Java_NewVector3f(ent->origin),
				Java_NewVector3f(ent->origin2),

				Java_NewAngle3f(ent->angles[PITCH], ent->angles[YAW], ent->angles[ROLL]),
				Java_NewAngle3f(ent->angles2[PITCH], ent->angles2[YAW], ent->angles2[ROLL]),

				ent->otherEntityNum,
				ent->otherEntityNum2,
				ent->groundEntityNum,

				ent->constantLight,
				ent->loopSound,

				ent->modelindex,
				ent->modelindex2,

				ent->clientNum,
				ent->frame,
				ent->solid,

				ent->event,
				ent->eventParm,

				ent->powerups,
				ent->weapon,

				ent->legsAnim,
				ent->torsoAnim,

				ent->generic1);
	}

	return obj;
}

static jobject Java_NewPlayerState(const playerState_t * ps)
{
	jobject obj = NULL;

	if(class_PlayerState)
	{
		jintArray eventsArray;
		jintArray eventParmsArray;

		jintArray statsArray;
		jintArray persistantArray;
		jintArray powerupsArray;
		jintArray ammoArray;

		// build arrays
		eventsArray = (*javaEnv)->NewIntArray(javaEnv, MAX_PS_EVENTS);
		(*javaEnv)->SetIntArrayRegion(javaEnv, eventsArray, 0, MAX_PS_EVENTS, ps->events);

		eventParmsArray = (*javaEnv)->NewIntArray(javaEnv, MAX_PS_EVENTS);
		(*javaEnv)->SetIntArrayRegion(javaEnv, eventParmsArray, 0, MAX_PS_EVENTS, ps->eventParms);


		statsArray = (*javaEnv)->NewIntArray(javaEnv, MAX_STATS);
		(*javaEnv)->SetIntArrayRegion(javaEnv, statsArray, 0, MAX_STATS, ps->stats);

		persistantArray = (*javaEnv)->NewIntArray(javaEnv, MAX_PERSISTANT);
		(*javaEnv)->SetIntArrayRegion(javaEnv, persistantArray, 0, MAX_PERSISTANT, ps->persistant);

		powerupsArray = (*javaEnv)->NewIntArray(javaEnv, MAX_POWERUPS);
		(*javaEnv)->SetIntArrayRegion(javaEnv, powerupsArray, 0, MAX_POWERUPS, ps->powerups);

		ammoArray = (*javaEnv)->NewIntArray(javaEnv, MAX_WEAPONS);
		(*javaEnv)->SetIntArrayRegion(javaEnv, ammoArray, 0, MAX_WEAPONS, ps->ammo);


		/*
		public PlayerState(int commandTime, int pmType, int pmFlags, int pmTime,
			int bobCycle, Vector3f origin, Vector3f velocity, int weaponTime,
			int gravity, int speed, int deltaPitch, int deltaYaw,
			int deltaRoll, int groundEntityNum, int legsTimer, int legsAnim,
			int torsoTimer, int torsoAnim, int movementDir,
			Vector3f grapplePoint, int eFlags, int eventSequence, int[] events,
			int[] eventParms, int externalEvent, int externalEventParm,
			int externalEventTime, int clientNum, int weapon, int weaponState,
			Angle3f viewAngles, int viewHeight, int damageEvent, int damageYaw,
			int damagePitch, int damageCount, int[] stats, int[] persistant,
			int[] powerups, int[] ammo, int generic1, int loopSound,
			int jumppadEnt, int ping, int pmoveFramecount, int jumppadFrame,
			int entityEventSequence)
		*/
		obj = (*javaEnv)->NewObject(javaEnv, class_PlayerState, method_PlayerState_ctor,
				ps->commandTime,

				ps->pm_type,
				ps->pm_flags,
				ps->pm_time,

				ps->bobCycle,

				Java_NewVector3f(ps->origin),
				Java_NewVector3f(ps->velocity),

				ps->weaponTime,
				ps->gravity,
				ps->speed,

				ps->delta_angles[PITCH],
				ps->delta_angles[YAW],
				ps->delta_angles[ROLL],

				ps->groundEntityNum,

				ps->legsTimer,
				ps->legsAnim,
				ps->torsoTimer,
				ps->torsoAnim,

				ps->movementDir,

				Java_NewVector3f(ps->grapplePoint),

				ps->eFlags,

				ps->eventSequence,
				eventsArray,
				eventParmsArray,

				ps->externalEvent,
				ps->externalEventParm,
				ps->externalEventTime,

				ps->clientNum,
				ps->weapon,
				ps->weaponstate,

				Java_NewAngle3f(ps->viewangles[PITCH], ps->viewangles[YAW], ps->viewangles[ROLL]),
				ps->viewheight,

				ps->damageEvent,
				ps->damageYaw,
				ps->damagePitch,
				ps->damageCount,

				statsArray,
				persistantArray,
				powerupsArray,
				ammoArray,

				ps->generic1,
				ps->loopSound,
				ps->jumppad_ent,
				ps->ping,

				ps->pmove_framecount,
				ps->jumppad_frame,

				ps->entityEventSequence);

		(*javaEnv)->DeleteLocalRef(javaEnv, eventsArray);

		CheckException();
	}

	return obj;
}

static jobject Java_NewSnapshot(const clSnapshot_t * clSnap)
{
	jobject obj = NULL;

	/*
	 snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	Com_Memcpy(snapshot->areamask, clSnap->areamask, sizeof(snapshot->areamask));
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;
	if(count > MAX_ENTITIES_IN_SNAPSHOT)
	{
		Com_DPrintf("CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT);
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
	for(i = 0; i < count; i++)
	{
		snapshot->entities[i] = cl.parseEntities[(clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
	}

	// FIXME: configstring changes and server commands!!!
	 */

	if(class_Snapshot && class_PlayerState && class_EntityState)
	{
		int				i, count;
		jobjectArray 	entitiesArray;
		jbyteArray		areamaskArray;
		jobject			playerState;

		// build EntityState[] entities
		count = clSnap->numEntities;
		if(count > MAX_ENTITIES_IN_SNAPSHOT)
		{
			Com_DPrintf("Java_NewSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT);
			count = MAX_ENTITIES_IN_SNAPSHOT;
		}

		entitiesArray = (*javaEnv)->NewObjectArray(javaEnv, count, class_EntityState, NULL);

		for(i = 0; i < count; i++) {

			jobject			javaEntityState;
			entityState_t*	entityState;

			entityState = &cl.parseEntities[(clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];

			javaEntityState = Java_NewEntityState(entityState);

			(*javaEnv)->SetObjectArrayElement(javaEnv, entitiesArray, i, javaEntityState);
		}

		// build byte[] areamask
		areamaskArray = (*javaEnv)->NewByteArray(javaEnv, sizeof(clSnap->areamask));
		(*javaEnv)->SetByteArrayRegion(javaEnv, areamaskArray, 0, sizeof(clSnap->areamask), clSnap->areamask);

		// build player state
		playerState = Java_NewPlayerState(&clSnap->ps);

		/*
		public Snapshot(int snapFlags, int ping, int serverTime,
					byte areamask[], PlayerState ps, EntityState[] entities,
					int serverCommandSequence)
		*/
		obj = (*javaEnv)->NewObject(javaEnv, class_Snapshot, method_Snapshot_ctor,
				clSnap->snapFlags,
				clSnap->ping,
				clSnap->serverTime,
				areamaskArray,
				playerState,
				entitiesArray,
				clSnap->serverCommandNum);

		(*javaEnv)->DeleteLocalRef(javaEnv, entitiesArray);
		(*javaEnv)->DeleteLocalRef(javaEnv, areamaskArray);
		(*javaEnv)->DeleteLocalRef(javaEnv, playerState);

		CheckException();
	}

	return obj;
}

// ====================================================================================

/*
 * Class:     xreal_client_Client
 * Method:    getConfigString
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_xreal_client_Client_getConfigString(JNIEnv *env, jclass cls, jint index)
{
	int             offset;
	char			buf[MAX_INFO_STRING];

	if(index < 0 || index >= MAX_CONFIGSTRINGS)
		return NULL;

	offset = cl.gameState.stringOffsets[index];
	if(!offset)
	{
		return NULL;
	}

	Q_strncpyz(buf, cl.gameState.stringData + offset, sizeof(buf));

	return (*env)->NewStringUTF(env, buf);
}

/*
 * Class:     xreal_client_Client
 * Method:    getCurrentSnapshotNumber
 * Signature: ()I
 */
jint JNICALL Java_xreal_client_Client_getCurrentSnapshotNumber(JNIEnv *env, jclass cls)
{
	return cl.snap.messageNum;
}

/*
 * Class:     xreal_client_Client
 * Method:    getCurrentSnapshotTime
 * Signature: ()I
 */
jint JNICALL Java_xreal_client_Client_getCurrentSnapshotTime(JNIEnv *env, jclass cls)
{
	return cl.snap.serverTime;
}

/*
 * Class:     xreal_client_Client
 * Method:    getSnapshot
 * Signature: (I)Lxreal/client/game/Snapshot;
 */
jobject JNICALL Java_xreal_client_Client_getSnapshot(JNIEnv *env, jclass cls, jint snapshotNumber)
{
	clSnapshot_t   *clSnap;

	if(snapshotNumber > cl.snap.messageNum)
	{
		Com_Error(ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum");
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if(cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP)
	{
		return NULL;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
	if(!clSnap->valid)
	{
		return NULL;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if(cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES)
	{
		return NULL;
	}

	// write the snapshot
	return Java_NewSnapshot(clSnap);
}



/*
=====================
CL_ConfigstringModified
=====================
*/
static void CL_ConfigstringModified(void)
{
	char           *old, *s;
	int             i, index;
	char           *dup;
	gameState_t     oldGs;
	int             len;

	index = atoi(Cmd_Argv(1));
	if(index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		Com_Error(ERR_DROP, "configstring > MAX_CONFIGSTRINGS");
	}
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[index];
	if(!strcmp(old, s))
	{
		return;					// unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	Com_Memset(&cl.gameState, 0, sizeof(cl.gameState));

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;

	for(i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if(i == index)
		{
			dup = s;
		}
		else
		{
			dup = oldGs.stringData + oldGs.stringOffsets[i];
		}
		if(!dup[0])
		{
			continue;			// leave with the default empty string
		}

		len = strlen(dup);

		if(len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS)
		{
			Com_Error(ERR_DROP, "MAX_GAMESTATE_CHARS exceeded");
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[i] = cl.gameState.dataCount;
		Com_Memcpy(cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1);
		cl.gameState.dataCount += len + 1;
	}

	if(index == CS_SYSTEMINFO)
	{
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}

/*
 * Class:     xreal_client_Client
 * Method:    getServerCommand
 * Signature: (I)[Ljava/lang/String;
 */
jobjectArray JNICALL Java_xreal_client_Client_getServerCommand(JNIEnv *env, jclass cls, jint serverCommandNumber)
{
	char           *s;
	char           *cmd;
	static char     bigConfigString[BIG_INFO_STRING];
	int             argc;

	// if we have irretrievably lost a reliable command, drop the connection
	if(serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS)
	{
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if(clc.demoplaying)
			return NULL;

		Com_Error(ERR_DROP, "Java_xreal_client_Client_getServerCommand: a reliable command was cycled out");
		return NULL;
	}

	if(serverCommandNumber > clc.serverCommandSequence)
	{
		Com_Error(ERR_DROP, "Java_xreal_client_Client_getServerCommand: requested a command not received");
		return NULL;
	}

	s = clc.serverCommands[serverCommandNumber & (MAX_RELIABLE_COMMANDS - 1)];
	clc.lastExecutedServerCommand = serverCommandNumber;

	Com_DPrintf("serverCommand: %i : %s\n", serverCommandNumber, s);

  rescan:
	Cmd_TokenizeString(s);
	cmd = Cmd_Argv(0);
	argc = Cmd_Argc();

	if(!strcmp(cmd, "disconnect"))
	{
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=552
		// allow server to indicate why they were disconnected
		if(argc >= 2)
			Com_Error(ERR_SERVERDISCONNECT, "Server disconnected - %s", Cmd_Argv(1));
		else
			Com_Error(ERR_SERVERDISCONNECT, "Server disconnected\n");
	}

	if(!strcmp(cmd, "bcs0"))
	{
		Com_sprintf(bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2));
		return NULL;
	}

	if(!strcmp(cmd, "bcs1"))
	{
		s = Cmd_Argv(2);
		if(strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		strcat(bigConfigString, s);
		return NULL;
	}

	if(!strcmp(cmd, "bcs2"))
	{
		s = Cmd_Argv(2);
		if(strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		strcat(bigConfigString, s);
		strcat(bigConfigString, "\"");
		s = bigConfigString;
		goto rescan;
	}

	if(!strcmp(cmd, "cs"))
	{
		CL_ConfigstringModified();

		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);

		return Java_NewConsoleArgs();
	}

	if(!strcmp(cmd, "map_restart"))
	{
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();

		// reparse the string, because Con_ClearNotify() may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		Com_Memset(cl.cmds, 0, sizeof(cl.cmds));

		return Java_NewConsoleArgs();
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make apropriate adjustments,
	// but we also clear the console and notify lines here
	if(!strcmp(cmd, "clientLevelShot"))
	{
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if(!com_sv_running->integer)
		{
			return NULL;
		}

		// close the console
		Con_Close();

		// take a special screenshot next frame
		Cbuf_AddText("wait ; wait ; wait ; wait ; screenshot levelshot\n");

		return Java_NewConsoleArgs();
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return Java_NewConsoleArgs();
}

/*
 * Class:     xreal_client_Client
 * Method:    getKeyCatchers
 * Signature: ()I
 */
jint JNICALL Java_xreal_client_Client_getKeyCatchers(JNIEnv *env, jclass cls)
{
	return Key_GetCatcher();
}

/*
 * Class:     xreal_client_Client
 * Method:    setKeyCatchers
 * Signature: (I)V
 */
void JNICALL Java_xreal_client_Client_setKeyCatchers(JNIEnv *env, jclass cls, jint catchers)
{
	// Don't allow the modules to close the console
	Key_SetCatcher(catchers | (Key_GetCatcher() & KEYCATCH_CONSOLE));
}

/*
 * Class:     xreal_client_Client
 * Method:    getKeyBinding
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_xreal_client_Client_getKeyBinding(JNIEnv *env, jclass cls, jint keynum)
{
	char           *value;

	value = Key_GetBinding(keynum);
	if(value && *value)
	{
		return (*env)->NewStringUTF(env, value);
	}

	return NULL;
}

/*
 * Class:     xreal_client_Client
 * Method:    setKeyBinding
 * Signature: (ILjava/lang/String;)V
 */
void JNICALL Java_xreal_client_Client_setKeyBinding(JNIEnv *env, jclass cls, jint keynum, jstring jbinding)
{
	char           *binding;

	binding = (char *)((*env)->GetStringUTFChars(env, jbinding, 0));

	Key_SetBinding(keynum, binding);

	(*env)->ReleaseStringUTFChars(env, jbinding, binding);
}

/*
 * Class:     xreal_client_Client
 * Method:    isKeyDown
 * Signature: (I)Z
 */
jboolean JNICALL Java_xreal_client_Client_isKeyDown(JNIEnv *env, jclass cls, jint keynum)
{
	return (jboolean) Key_IsDown(keynum);
}

/*
 * Class:     xreal_client_Client
 * Method:    clearKeyStates
 * Signature: ()V
 */
void JNICALL Java_xreal_client_Client_clearKeyStates(JNIEnv *env, jclass cls)
{
	Key_ClearStates();
}


/*
 * Class:     xreal_client_Client
 * Method:    getCurrentCommandNumber
 * Signature: ()I
 */
jint JNICALL Java_xreal_client_Client_getCurrentCommandNumber(JNIEnv *env, jclass cls)
{
	return cl.cmdNumber;
}

/*
 * Class:     xreal_client_Client
 * Method:    getOldestCommandNumber
 * Signature: ()I
 */
jint JNICALL Java_xreal_client_Client_getOldestCommandNumber(JNIEnv *env, jclass cls)
{
	return (cl.cmdNumber - CMD_BACKUP + 1);
}

/*
 * Class:     xreal_client_Client
 * Method:    getUserCommand
 * Signature: (I)Lxreal/UserCommand;
 */
jobject JNICALL Java_xreal_client_Client_getUserCommand(JNIEnv *env, jclass cls, jint cmdNumber)
{
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if(cmdNumber > cl.cmdNumber)
	{
		Com_Error(ERR_DROP, "Java_xreal_client_Client_getUserCommand: %i >= %i", cmdNumber, cl.cmdNumber);
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if(cmdNumber <= cl.cmdNumber - CMD_BACKUP)
	{
		return NULL;
	}

	return Java_NewUserCommand(&cl.cmds[cmdNumber & CMD_MASK]);
}



/*
 * Class:     xreal_client_Client
 * Method:    registerSound
 * Signature: (Ljava/lang/String;)I
 */
jint JNICALL Java_xreal_client_Client_registerSound(JNIEnv *env, jclass cls, jstring jname)
{
	char           *name;
	sfxHandle_t		handle;

	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	handle = S_RegisterSound(name);

	(*env)->ReleaseStringUTFChars(env, jname, name);

	return handle;
}

/*
 * Class:     xreal_client_Client
 * Method:    startSound
 * Signature: (FFFIII)V
 */
// void startSound(float posX, float posY, float posZ, int entityNum, int entityChannel, int sfxHandle);
void JNICALL Java_xreal_client_Client_startSound(JNIEnv *env, jclass cls, jfloat posX, jfloat posY, jfloat posZ, jint entityNum, jint entityChannel, jint sfxHandle)
{
	vec3_t position;

	VectorSet(position, posX, posY, posZ);

	S_StartSound(position, entityNum, entityChannel, sfxHandle);
}

/*
 * Class:     xreal_client_Client
 * Method:    startLocalSound
 * Signature: (II)V
 */
void JNICALL Java_xreal_client_Client_startLocalSound(JNIEnv *env, jclass cls, jint sfxHandle, jint channelNum)
{
	S_StartLocalSound(sfxHandle, channelNum);
}

/*
 * Class:     xreal_client_Client
 * Method:    addLoopingSound
 * Signature: (IFFFFFFI)V
 */
// void addLoopingSound(int entityNum, float posX, float posY, float posZ, float velX, float velY, float velZ, int sfxHandle);
void JNICALL Java_xreal_client_Client_addLoopingSound(JNIEnv *env, jclass cls, jint entityNum, jfloat posX, jfloat posY, jfloat posZ, jfloat velX, jfloat velY, jfloat velZ, jint sfxHandle)
{
	vec3_t position;
	vec3_t velocity;

	VectorSet(position, posX, posY, posZ);
	VectorSet(velocity, velX, velY, velZ);

	S_AddLoopingSound(entityNum, position, velocity, sfxHandle);
}

/*
 * Class:     xreal_client_Client
 * Method:    addRealLoopingSound
 * Signature: (IFFFFFFI)V
 */
// void addRealLoopingSound(int entityNum, float posX, float posY, float posZ, float velX, float velY, float velZ, int sfxHandle);
void JNICALL Java_xreal_client_Client_addRealLoopingSound(JNIEnv *env, jclass cls, jint entityNum, jfloat posX, jfloat posY, jfloat posZ, jfloat velX, jfloat velY, jfloat velZ, jint sfxHandle)
{
	vec3_t position;
	vec3_t velocity;

	VectorSet(position, posX, posY, posZ);
	VectorSet(velocity, velX, velY, velZ);

	S_AddRealLoopingSound(entityNum, position, velocity, sfxHandle);
}

/*
 * Class:     xreal_client_Client
 * Method:    updateEntitySoundPosition
 * Signature: (IFFF)V
 */
// void updateEntitySoundPosition(int entityNum, float posX, float posY, float posZ);
void JNICALL Java_xreal_client_Client_updateEntitySoundPosition(JNIEnv *env, jclass cls, jint entityNum, jfloat posX, jfloat posY, jfloat posZ)
{
	vec3_t position;

	VectorSet(position, posX, posY, posZ);

	S_UpdateEntityPosition(entityNum, position);
}

/*
 * Class:     xreal_client_Client
 * Method:    stopLoopingSound
 * Signature: (I)V
 */
void JNICALL Java_xreal_client_Client_stopLoopingSound(JNIEnv *env, jclass cls, jint entityNum)
{
	S_StopLoopingSound(entityNum);
}

/*
 * Class:     xreal_client_Client
 * Method:    clearLoopingSounds
 * Signature: (Z)V
 */
void JNICALL Java_xreal_client_Client_clearLoopingSounds(JNIEnv *env, jclass cls, jboolean killall)
{
	S_ClearLoopingSounds(killall);
}

/*
 * Class:     xreal_client_Client
 * Method:    respatialize
 * Signature: (IFFFFFFFZ)V
 */
// void respatialize(int entityNum, float posX, float posY, float posZ, float quatX, float quatY, float quatZ, float quatW, boolean inWater);
void JNICALL Java_xreal_client_Client_respatialize(JNIEnv *env, jclass cls, jint entityNum, jfloat posX, jfloat posY, jfloat posZ, jfloat quatX, jfloat quatY, jfloat quatZ, jfloat quatW, jboolean inWater)
{
	vec3_t position;
	quat_t quat;
	axis_t axis;

	VectorSet(position, posX, posY, posZ);
	QuatSet(quat, quatX, quatY, quatZ, quatZ);

	QuatToAxis(quat, axis);

	S_Respatialize(entityNum, position, axis, inWater);
}

/*
 * Class:     xreal_client_Client
 * Method:    startBackgroundTrack
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
void JNICALL Java_xreal_client_Client_startBackgroundTrack(JNIEnv *env, jclass cls, jstring jintro, jstring jloop)
{
	char           *intro;
	char           *loop;

	intro = (char *)((*env)->GetStringUTFChars(env, jintro, 0));
	loop = (char *)((*env)->GetStringUTFChars(env, jloop, 0));

	S_StartBackgroundTrack(intro, loop);

	(*env)->ReleaseStringUTFChars(env, jintro, intro);
	(*env)->ReleaseStringUTFChars(env, jloop, loop);
}

/*
 * Class:     xreal_client_Client
 * Method:    stopBackgroundTrack
 * Signature: ()V
 */
void JNICALL Java_xreal_client_Client_stopBackgroundTrack(JNIEnv *env, jclass cls)
{
	S_StopBackgroundTrack();
}

// handle to Client class
static jclass   class_Client = NULL;
static JNINativeMethod Client_methods[] = {
	{"getConfigString", "(I)Ljava/lang/String;", Java_xreal_client_Client_getConfigString},

	{"getCurrentSnapshotNumber", "()I", Java_xreal_client_Client_getCurrentSnapshotNumber},
	{"getCurrentSnapshotTime", "()I", Java_xreal_client_Client_getCurrentSnapshotTime},
	{"getSnapshot", "(I)Lxreal/client/Snapshot;", Java_xreal_client_Client_getSnapshot},
	{"getServerCommand", "(I)[Ljava/lang/String;", Java_xreal_client_Client_getServerCommand},

	{"getKeyCatchers", "()I", Java_xreal_client_Client_getKeyCatchers},
	{"setKeyCatchers", "(I)V", Java_xreal_client_Client_setKeyCatchers},
	{"getKeyBinding", "(I)Ljava/lang/String;", Java_xreal_client_Client_getKeyBinding},
	{"setKeyBinding", "(ILjava/lang/String;)V", Java_xreal_client_Client_setKeyBinding},
	{"isKeyDown", "(I)Z", Java_xreal_client_Client_isKeyDown},
	{"clearKeyStates", "()V", Java_xreal_client_Client_clearKeyStates},

	{"getCurrentCommandNumber", "()I", Java_xreal_client_Client_getCurrentCommandNumber},
	{"getOldestCommandNumber", "()I", Java_xreal_client_Client_getOldestCommandNumber},
	{"getUserCommand", "(I)Lxreal/UserCommand;", Java_xreal_client_Client_getUserCommand},

	{"registerSound", "(Ljava/lang/String;)I", Java_xreal_client_Client_registerSound},
	{"startSound", "(FFFIII)V", Java_xreal_client_Client_startSound},
	{"startLocalSound", "(II)V", Java_xreal_client_Client_startLocalSound},
	{"addLoopingSound", "(IFFFFFFI)V", Java_xreal_client_Client_addLoopingSound},
	{"addRealLoopingSound", "(IFFFFFFI)V", Java_xreal_client_Client_addRealLoopingSound},
	{"updateEntitySoundPosition", "(IFFF)V", Java_xreal_client_Client_updateEntitySoundPosition},
	{"stopLoopingSound", "(I)V", Java_xreal_client_Client_stopLoopingSound},
	{"clearLoopingSounds", "(Z)V", Java_xreal_client_Client_clearLoopingSounds},
	{"respatialize", "(IFFFFFFFZ)V", Java_xreal_client_Client_respatialize},
	{"startBackgroundTrack", "(Ljava/lang/String;Ljava/lang/String;)V", Java_xreal_client_Client_startBackgroundTrack},
	{"stopBackgroundTrack", "()V", Java_xreal_client_Client_stopBackgroundTrack},
};

void Client_javaRegister()
{
	Com_Printf("Client_javaRegister()\n");

	class_Client = (*javaEnv)->FindClass(javaEnv, "xreal/client/Client");
	if(CheckException() || !class_Client)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.client.Client");
	}

	(*javaEnv)->RegisterNatives(javaEnv, class_Client, Client_methods, sizeof(Client_methods) / sizeof(Client_methods[0]));
	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't register native methods for xreal.client.Client");
	}
}


void Client_javaDetach()
{
	Com_Printf("Client_javaDetach()\n");

	if(javaEnv)
	{
		if(class_Client)
		{
			(*javaEnv)->UnregisterNatives(javaEnv, class_Client);
			(*javaEnv)->DeleteLocalRef(javaEnv, class_Client);
			class_Client = NULL;
		}
	}
}


// ====================================================================================


static jclass class_Glyph = NULL;
static jmethodID method_Glyph_ctor = NULL;

static jclass   class_Font = NULL;
static jmethodID method_Font_ctor = NULL;

void Font_javaRegister()
{
	class_Glyph = (*javaEnv)->FindClass(javaEnv, "xreal/client/renderer/Glyph");
	if(CheckException() || !class_Glyph)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.client.renderer.Glyph");
	}

	/*
		public Glyph(int height, int top, int bottom, int pitch, int skip, int imageWidth, int imageHeight, float s, float t, float s2, float t2, int glyph,
				String materialName) {
	 */

	method_Glyph_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Glyph, "<init>",
			"("
			"IIIIIIIFFFFILjava/lang/String;"
			")V");
	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't find constructor of xreal.client.renderer.Glyph");
	}


	class_Font = (*javaEnv)->FindClass(javaEnv, "xreal/client/renderer/Font");
	if(CheckException() || !class_Font)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.client.renderer.Font");
	}

	// public Font(Glyph[] glyphs, float glyphScale, String name)
	method_Font_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Font, "<init>", "([Lxreal/client/renderer/Glyph;FLjava/lang/String;)V");

	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't find constructor of xreal.client.renderer.Font");
	}
}

void Font_javaDetach()
{
	if(class_Glyph)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Glyph);
		class_Glyph = NULL;
	}

	if(class_Font)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_Font);
		class_Font = NULL;
	}
}

jobject Java_NewGlyph(const glyphInfo_t * glyph)
{
	jobject obj = NULL;

	if(class_Glyph)
	{
		jstring name = (*javaEnv)->NewStringUTF(javaEnv, glyph->shaderName);

		/*
		public Glyph(int height, int top, int bottom, int pitch, int skip, int imageWidth, int imageHeight, float s, float t, float s2, float t2, int glyph,
					String materialName) {
		*/
		obj = (*javaEnv)->NewObject(javaEnv, class_Glyph, method_Glyph_ctor,
				glyph->height,
				glyph->top,
				glyph->bottom,
				glyph->pitch,
				glyph->xSkip,
				glyph->imageWidth,
				glyph->imageHeight,
				glyph->s,
				glyph->t,
				glyph->s2,
				glyph->t2,
				glyph->glyph,
				name);
	}

	return obj;
}

jobject Java_NewFont(const fontInfo_t * font)
{
	jobject obj = NULL;

	if(class_Glyph && class_Font)
	{
		int i;
		jobjectArray glyphsArray;
		jstring name;

		glyphsArray = (*javaEnv)->NewObjectArray(javaEnv, GLYPHS_PER_FONT, class_Glyph, NULL);

		for(i = 0; i < GLYPHS_PER_FONT; i++) {
			jobject glyph = Java_NewGlyph(&font->glyphs[i]);

			(*javaEnv)->SetObjectArrayElement(javaEnv, glyphsArray, i, glyph);
		}

		name = (*javaEnv)->NewStringUTF(javaEnv, font->name);

		/*
		public Font(Glyph[] glyphs, float glyphScale, String name) {
		*/
		obj = (*javaEnv)->NewObject(javaEnv, class_Font, method_Font_ctor,
				glyphsArray,
				font->glyphScale,
				name);

		(*javaEnv)->DeleteLocalRef(javaEnv, glyphsArray);

		CheckException();
	}

	return obj;
}

// ====================================================================================



/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    setColor
 * Signature: (FFFF)V
 */
void JNICALL Java_xreal_client_renderer_Renderer_setColor(JNIEnv *env, jclass cls, jfloat red, jfloat green, jfloat blue, jfloat alpha)
{
	vec4_t color;

	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	re.SetColor(color);
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    drawStretchPic
 * Signature: (FFFFFFFFI)V
 */
//drawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader);
void JNICALL Java_xreal_client_renderer_Renderer_drawStretchPic(JNIEnv *env, jclass cls, jfloat x, jfloat y, jfloat w, jfloat h, jfloat s1, jfloat t1, jfloat s2, jfloat t2, jint hMaterial)
{
	re.DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hMaterial);
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    registerFont
 * Signature: (Ljava/lang/String;I)Lxreal/client/renderer/Font;
 */
jobject JNICALL Java_xreal_client_renderer_Renderer_registerFont(JNIEnv *env, jclass cls, jstring jname, jint pointSize)
{
	fontInfo_t      font;
	char           *name;

	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	Com_Memset(&font, 0, sizeof(font));

	re.RegisterFont(name, pointSize, &font);
	if(font.name[0] == '\0')
	{
		Com_Error(ERR_DROP, "Java_xreal_client_renderer_Renderer_registerFont: Couldn't register font '%s'", name);
	}

	(*env)->ReleaseStringUTFChars(env, jname, name);

	return Java_NewFont(&font);
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    registerMaterial
 * Signature: (Ljava/lang/String;)I
 */
jint JNICALL Java_xreal_client_renderer_Renderer_registerMaterial(JNIEnv *env, jclass cls, jstring jname)
{
	char           *name;
	qhandle_t		handle;

	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	handle = re.RegisterShader(name);

	(*env)->ReleaseStringUTFChars(env, jname, name);

	return handle;
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    registerMaterialNoMip
 * Signature: (Ljava/lang/String;)I
 */
jint JNICALL Java_xreal_client_renderer_Renderer_registerMaterialNoMip(JNIEnv *env, jclass cls, jstring jname)
{
	char           *name;
	qhandle_t		handle;

	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	handle = re.RegisterShaderNoMip(name);

	(*env)->ReleaseStringUTFChars(env, jname, name);

	return handle;
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    registerMaterialLightAttenuation
 * Signature: (Ljava/lang/String;)I
 */
jint JNICALL Java_xreal_client_renderer_Renderer_registerMaterialLightAttenuation(JNIEnv *env, jclass cls, jstring jname)
{
	char           *name;
	qhandle_t		handle;

	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	handle = re.RegisterShaderLightAttenuation(name);

	(*env)->ReleaseStringUTFChars(env, jname, name);

	return handle;
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    registerModel
 * Signature: (Ljava/lang/String;Z)I
 */
jint JNICALL Java_xreal_client_renderer_Renderer_registerModel(JNIEnv *env, jclass cls, jstring jname, jboolean forceStatic)
{
	char           *name;
	qhandle_t		handle;

	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	handle = re.RegisterModel(name, forceStatic);

	(*env)->ReleaseStringUTFChars(env, jname, name);

	return handle;
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    registerAnimation
 * Signature: (Ljava/lang/String;)I
 */
jint JNICALL Java_xreal_client_renderer_Renderer_registerAnimation(JNIEnv *env, jclass cls, jstring jname)
{
	char           *name;
	qhandle_t		handle;

	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	handle = re.RegisterAnimation(name);

	(*env)->ReleaseStringUTFChars(env, jname, name);

	return handle;
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    registerSkin
 * Signature: (Ljava/lang/String;)I
 */
jint JNICALL Java_xreal_client_renderer_Renderer_registerSkin(JNIEnv *env, jclass cls, jstring jname)
{
	char           *name;
	qhandle_t		handle;

	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	handle = re.RegisterSkin(name);

	(*env)->ReleaseStringUTFChars(env, jname, name);

	return handle;
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    loadWorldBsp
 * Signature: (Ljava/lang/String;)V
 */
void JNICALL Java_xreal_client_renderer_Renderer_loadWorldBsp(JNIEnv *env, jclass cls, jstring jname)
{
	char           *name;

	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	re.LoadWorld(name);

	(*env)->ReleaseStringUTFChars(env, jname, name);
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    clearScene
 * Signature: ()V
 */
void JNICALL Java_xreal_client_renderer_Renderer_clearScene(JNIEnv *env, jclass cls)
{
	re.ClearScene();
}

static refEntity_t		refEntity;

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    addRefEntityToScene
 * Signature: (IIIFFFFFFFFFFFFFFIFFFIFIIIFFFFFFFFFIZ)V
 */
// *INDENT-OFF*
void JNICALL Java_xreal_client_renderer_Renderer_addRefEntityToScene(JNIEnv *env, jclass cls, jint reType, jint renderfx, jint hModel,
			jfloat posX, jfloat posY, jfloat posZ,
			jfloat quatX, jfloat quatY, jfloat quatZ, jfloat quatW,
			jfloat scaleX, jfloat scaleY, jfloat scaleZ,
			jfloat lightPosX, jfloat lightPosY, jfloat lightPosZ,
			jfloat shadowPlane,
			jint frame,
			jfloat oldPosX, jfloat oldPosY, jfloat oldPosZ,
			jint oldFrame,
			jfloat lerp,
			jint skinNum, jint customSkin, jint customMaterial,
			jfloat materialRed, jfloat materialGreen, jfloat materialBlue, jfloat materialAlpha,
			jfloat materialTexCoordU, jfloat materialTexCoordV,
			jfloat materialTime,
			jfloat radius, jfloat rotation,
			jint noShadowID,
			jboolean useSkeleton)
{
	quat_t			quat;

	refEntity.reType = reType;
	refEntity.renderfx = renderfx;
	refEntity.hModel = hModel;

	// most recrefEntity data
	VectorSet(refEntity.origin, posX, posY, posZ);

	QuatSet(quat, quatX, quatY, quatZ, quatW);
	QuatToAxis(quat, refEntity.axis);

	if(scaleX != 1 || scaleY != 1 || scaleZ != 1)
	{
		VectorScale(refEntity.axis[0], scaleX, refEntity.axis[0]);
		VectorScale(refEntity.axis[1], scaleY, refEntity.axis[1]);
		VectorScale(refEntity.axis[2], scaleZ, refEntity.axis[2]);

		refEntity.nonNormalizedAxes = qtrue;
	}
	else
	{
		refEntity.nonNormalizedAxes = qfalse;
	}

	VectorSet(refEntity.lightingOrigin, lightPosX, lightPosY, lightPosZ);
	refEntity.shadowPlane = shadowPlane;

	refEntity.frame = frame;

	// previous data for frame interpolation
	VectorSet(refEntity.oldorigin, oldPosX, oldPosY, oldPosZ);
	refEntity.oldframe = oldFrame;
	refEntity.backlerp = 1.0 - lerp;

	// texturing
	refEntity.skinNum = skinNum;
	refEntity.customSkin = customSkin;
	refEntity.customShader = customMaterial;

	// misc
	refEntity.shaderRGBA[0] = (byte) (materialRed * 255);
	refEntity.shaderRGBA[1] = (byte) (materialGreen * 255);
	refEntity.shaderRGBA[2] = (byte) (materialBlue * 255);
	refEntity.shaderRGBA[3] = (byte) (materialAlpha * 255);

	refEntity.shaderTexCoord[0] = materialTexCoordU;
	refEntity.shaderTexCoord[1] = materialTexCoordV;

	refEntity.shaderTime = materialTime;

	// extra sprite information
	refEntity.radius = radius;
	refEntity.rotation = rotation;

	// extra light interaction information
	refEntity.noShadowID = noShadowID;

	if(useSkeleton == qfalse)
	{
		refEntity.skeleton.type = SK_INVALID;
	}

	re.AddRefEntityToScene(&refEntity);

}
// *INDENT-ON*

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    setRefEntityBone
 * Signature: (ILjava/lang/String;IFFFFFFF)V
 */
// *INDENT-OFF*
void JNICALL Java_xreal_client_renderer_Renderer_setRefEntityBone(	JNIEnv *env, jclass cls,
																	jint boneIndex, jstring jname, jint parentIndex,
																	jfloat posX, jfloat posY, jfloat posZ,
																	jfloat quatX, jfloat quatY, jfloat quatZ, jfloat quatW)
{
	refBone_t*		bone;

#if defined(REFBONE_NAMES)
	char           *name;
#endif

	if(boneIndex < 0 || boneIndex >= MAX_BONES)
	{
		Com_Error(ERR_DROP, "Java_xreal_client_renderer_Renderer_setRefEntityBone: bad bone index %i\n", boneIndex);
	}

	bone = &refEntity.skeleton.bones[boneIndex];

#if defined(REFBONE_NAMES)
	name = (char *)((*env)->GetStringUTFChars(env, jname, 0));

	Q_strncpyz(bone->name, name, sizeof(bone->name));

	(*env)->ReleaseStringUTFChars(env, jname, name);
#endif

	bone->parentIndex = parentIndex;

	VectorSet(bone->origin, posX, posY, posZ);
	QuatSet(bone->rotation, quatX, quatY, quatZ, quatW);
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    setRefEntitySkeleton
 * Signature: (IIFFFFFFFFF)V
 */
void JNICALL Java_xreal_client_renderer_Renderer_setRefEntitySkeleton(JNIEnv *env, jclass cls,
																	jint type,
																	jint numBones,
																	jfloat minX, jfloat minY, jfloat minZ,
																	jfloat maxX, jfloat maxY, jfloat maxZ,
																	jfloat scaleX, jfloat scaleY, jfloat scaleZ)
{
	refEntity.skeleton.type = type;
	refEntity.skeleton.numBones = numBones;

	VectorSet(refEntity.skeleton.bounds[0], minX, minY, minZ);
	VectorSet(refEntity.skeleton.bounds[1], maxX, maxY, maxZ);

	VectorSet(refEntity.skeleton.scale, scaleX, scaleY, scaleZ);
}




// ====================================================================================

static jclass class_RefBone = NULL;
static jmethodID method_RefBone_ctor = NULL;

static jclass   class_RefSkeleton = NULL;
static jmethodID method_RefSkeleton_ctor = NULL;


static void RefSkeleton_javaRegister()
{
	class_RefBone = (*javaEnv)->FindClass(javaEnv, "xreal/client/renderer/RefBone");
	if(CheckException() || !class_RefBone)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.client.renderer.RefBone");
	}


	// public RefBone(String name, int parentIndex, Vector3f origin, Quat4f rotation)

	method_RefBone_ctor = (*javaEnv)->GetMethodID(javaEnv, class_RefBone, "<init>", "(Ljava/lang/String;ILjavax/vecmath/Vector3f;Ljavax/vecmath/Quat4f;)V");
	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't find constructor of xreal.client.renderer.RefBone");
	}


	class_RefSkeleton = (*javaEnv)->FindClass(javaEnv, "xreal/client/renderer/RefSkeleton");
	if(CheckException() || !class_RefSkeleton)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.client.renderer.RefSkeleton");
	}

	// public RefSkeleton(int type, RefBone[] bones, Vector3f mins, Vector3f maxs)

	method_RefSkeleton_ctor = (*javaEnv)->GetMethodID(javaEnv, class_RefSkeleton, "<init>", "(I[Lxreal/client/renderer/RefBone;Ljavax/vecmath/Vector3f;Ljavax/vecmath/Vector3f;)V");

	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't find constructor of xreal.client.renderer.RefSkeleton");
	}
}

void RefSkeleton_javaDetach()
{
	if(class_RefBone)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_RefBone);
		class_RefBone = NULL;
	}

	if(class_RefSkeleton)
	{
		(*javaEnv)->DeleteLocalRef(javaEnv, class_RefSkeleton);
		class_RefSkeleton = NULL;
	}
}

jobject Java_NewRefBone(const refBone_t * bone)
{
	jobject obj = NULL;

	if(class_RefBone)
	{
		jstring name = (*javaEnv)->NewStringUTF(javaEnv, bone->name);

		// public RefBone(String name, int parentIndex, Vector3f origin, Quat4f rotation) {
		obj = (*javaEnv)->NewObject(javaEnv, class_RefBone, method_RefBone_ctor,
				name,
				bone->parentIndex,
				Java_NewVector3f(bone->origin),
				Java_NewQuat4f(bone->rotation));
	}

	return obj;
}

jobject Java_NewRefSkeleton(const refSkeleton_t * skel)
{
	jobject obj = NULL;

	if(class_RefSkeleton && class_RefBone)
	{
		int i;
		jobjectArray bonesArray;

		bonesArray = (*javaEnv)->NewObjectArray(javaEnv, skel->numBones, class_RefBone, NULL);

		for(i = 0; i < skel->numBones; i++) {
			jobject bone = Java_NewRefBone(&skel->bones[i]);

			(*javaEnv)->SetObjectArrayElement(javaEnv, bonesArray, i, bone);
		}

		/*
		public RefSkeleton(int type, RefBone[] bones, Vector3f mins, Vector3f maxs)
		*/
		obj = (*javaEnv)->NewObject(javaEnv, class_RefSkeleton, method_RefSkeleton_ctor,
				skel->type,
				bonesArray,
				Java_NewVector3f(skel->bounds[0]),
				Java_NewVector3f(skel->bounds[1]));

		(*javaEnv)->DeleteLocalRef(javaEnv, bonesArray);

		CheckException();
	}

	return obj;
}

// ====================================================================================


/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    buildSkeleton
 * Signature: (IIIFZ)Lxreal/client/renderer/RefSkeleton;
 */
jobject JNICALL Java_xreal_client_renderer_Renderer_buildSkeleton(JNIEnv *env, jclass cls, jint hAnim, jint startFrame, jint endFrame, jfloat frac, jboolean clearOrigin)
{
	refSkeleton_t   skeleton;

	re.BuildSkeleton(&skeleton, hAnim, startFrame, endFrame, frac, clearOrigin);

	return Java_NewRefSkeleton(&skeleton);
}

static refEntity_t		refEntity;

static int				s_maxPolyVerts;
static poly_t			s_poly;

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    addPolygonToSceneBegin
 * Signature: (II)V
 */
void JNICALL Java_xreal_client_renderer_Renderer_addPolygonToSceneBegin(JNIEnv *env, jclass cls, jint hMaterial, jint numVertices)
{
	if(hMaterial <= 0)
	{
		Com_Error(ERR_DROP, "Java_xreal_client_renderer_Renderer_addPolygonToSceneBegin: null hMaterial\n");
	}

	s_poly.numVerts = 0;
	s_poly.hShader = hMaterial;
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    addPolygonVertexToScene
 * Signature: (FFFFFFFFF)V
 */
void JNICALL Java_xreal_client_renderer_Renderer_addPolygonVertexToScene(JNIEnv *env, jclass cls,
																					jfloat posX, jfloat posY, jfloat posZ,
																					jfloat texCoordU, jfloat texCoordV,
																					jfloat red, jfloat green, jfloat blue, jfloat alpha)
{
	polyVert_t*				v;

	if(s_poly.numVerts >= s_maxPolyVerts)
	{
		Com_DPrintf("WARNING: Java_xreal_client_renderer_Renderer_addPolygonVertexToScene: r_maxPolyVerts reached\n");
		return;
	}

	v = &s_poly.verts[s_poly.numVerts++];

	v->xyz[0] = posX;
	v->xyz[1] = posY;
	v->xyz[2] = posZ;

	v->st[0] = texCoordU;
	v->st[1] = texCoordV;

	v->modulate[0] = (byte) (red * 255);
	v->modulate[1] = (byte) (green * 255);
	v->modulate[2] = (byte) (blue * 255);
	v->modulate[3] = (byte) (alpha * 255);
}

/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    addPolygonToSceneEnd
 * Signature: ()V
 */
void JNICALL Java_xreal_client_renderer_Renderer_addPolygonToSceneEnd(JNIEnv *env, jclass cls)
{
	if(s_poly.numVerts == 0) {
		return;
	}

	re.AddPolyToScene(s_poly.hShader, s_poly.numVerts, s_poly.verts, 1);

	s_poly.numVerts = 0;
}



/*
 * Class:     xreal_client_renderer_Renderer
 * Method:    renderScene
 * Signature: (IIIIFFFFFFFFFII)V
 */
void JNICALL Java_xreal_client_renderer_Renderer_renderScene(JNIEnv *env, jclass cls,
																jint viewPortX, jint viewPortY, jint viewPortWidth, jint viewPortHeight,
																jfloat fovX, jfloat fovY,
																jfloat posX, jfloat posY, jfloat posZ,
																jfloat quatX, jfloat quatY, jfloat quatZ, jfloat quatW,
																jint time,
																jint flags)
{
	refdef_t		refdef;
	quat_t			quat;

	Com_Memset(&refdef, 0, sizeof(refdef));

	refdef.x = viewPortX;
	refdef.y = viewPortY;
	refdef.width = viewPortWidth;
	refdef.height = viewPortHeight;

	refdef.fov_x = fovX;
	refdef.fov_y = fovY;

	VectorSet(refdef.vieworg, posX, posY, posZ);

	QuatSet(quat, quatX, quatY, quatZ, quatW);
	QuatToAxis(quat, refdef.viewaxis);

	refdef.time = time;
	refdef.rdflags = flags;

	re.RenderScene(&refdef);
}






// handle to Renderer class
static jclass   class_Renderer = NULL;
static JNINativeMethod Renderer_methods[] = {
	{"setColor", "(FFFF)V", Java_xreal_client_renderer_Renderer_setColor},
	{"drawStretchPic", "(FFFFFFFFI)V", Java_xreal_client_renderer_Renderer_drawStretchPic},
	{"registerFont", "(Ljava/lang/String;I)Lxreal/client/renderer/Font;", Java_xreal_client_renderer_Renderer_registerFont},
	{"registerMaterial", "(Ljava/lang/String;)I", Java_xreal_client_renderer_Renderer_registerMaterial},
	{"registerMaterialNoMip", "(Ljava/lang/String;)I", Java_xreal_client_renderer_Renderer_registerMaterialNoMip},
	{"registerMaterialLightAttenuation", "(Ljava/lang/String;)I", Java_xreal_client_renderer_Renderer_registerMaterialLightAttenuation},
	{"registerModel", "(Ljava/lang/String;Z)I", Java_xreal_client_renderer_Renderer_registerModel},
	{"registerAnimation", "(Ljava/lang/String;)I", Java_xreal_client_renderer_Renderer_registerAnimation},
	{"registerSkin", "(Ljava/lang/String;)I", Java_xreal_client_renderer_Renderer_registerSkin},
	{"loadWorldBsp", "(Ljava/lang/String;)V", Java_xreal_client_renderer_Renderer_loadWorldBsp},
	{"clearScene", "()V", Java_xreal_client_renderer_Renderer_clearScene},
	{"addRefEntityToScene", "(IIIFFFFFFFFFFFFFFIFFFIFIIIFFFFFFFFFIZ)V", Java_xreal_client_renderer_Renderer_addRefEntityToScene},
	{"setRefEntityBone", "(ILjava/lang/String;IFFFFFFF)V", Java_xreal_client_renderer_Renderer_setRefEntityBone},
	{"setRefEntitySkeleton", "(IIFFFFFFFFF)V", Java_xreal_client_renderer_Renderer_setRefEntitySkeleton},
	{"buildSkeleton", "(IIIFZ)Lxreal/client/renderer/RefSkeleton;", Java_xreal_client_renderer_Renderer_buildSkeleton},

	{"addPolygonToSceneBegin", "(II)V", Java_xreal_client_renderer_Renderer_addPolygonToSceneBegin},
	{"addPolygonVertexToScene", "(FFFFFFFFF)V", Java_xreal_client_renderer_Renderer_addPolygonVertexToScene},
	{"addPolygonToSceneEnd", "()V", Java_xreal_client_renderer_Renderer_addPolygonToSceneEnd},

	{"renderScene", "(IIIIFFFFFFFFFII)V", Java_xreal_client_renderer_Renderer_renderScene}
};

void Renderer_javaRegister()
{
	cvar_t* r_maxPolyVerts;

	Com_Printf("Renderer_javaRegister()\n");

	class_Renderer = (*javaEnv)->FindClass(javaEnv, "xreal/client/renderer/Renderer");
	if(CheckException() || !class_Renderer)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.client.renderer.Renderer");
	}

	(*javaEnv)->RegisterNatives(javaEnv, class_Renderer, Renderer_methods, sizeof(Renderer_methods) / sizeof(Renderer_methods[0]));
	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't register native methods for xreal.client.renderer.Renderer");
	}

	// allocate memory for temporary polygon vertices
	r_maxPolyVerts = Cvar_Get("r_maxpolyverts", "100000", 0);	// 3000 in vanilla Q3A

	s_maxPolyVerts = r_maxPolyVerts->integer;
	s_poly.verts = (polyVert_t *) Com_Allocate(sizeof(polyVert_t) * r_maxPolyVerts->integer);
}


void Renderer_javaDetach()
{
	Com_Printf("Renderer_javaDetach()\n");

	if(javaEnv)
	{
		if(class_Renderer)
		{
			(*javaEnv)->UnregisterNatives(javaEnv, class_Renderer);
			(*javaEnv)->DeleteLocalRef(javaEnv, class_Renderer);
			class_Renderer = NULL;
		}
	}

	// destroy polygon vertices pool
	if(s_poly.verts != NULL)
	{
		Com_Dealloc(s_poly.verts);
		s_poly.verts = NULL;

		s_maxPolyVerts = 0;
	}
}

// ====================================================================================


// handle to UserInterface class
static jclass   class_UserInterface = NULL;
static jobject  object_UserInterface = NULL;
static jclass   interface_UserInterfaceListener;
static jmethodID method_UserInterface_ctor;
static jmethodID method_UserInterface_initUserInterface;
static jmethodID method_UserInterface_shutdownUserInterface;
static jmethodID method_UserInterface_keyEvent;
static jmethodID method_UserInterface_mouseEvent;
static jmethodID method_UserInterface_refresh;
static jmethodID method_UserInterface_isFullscreen;
static jmethodID method_UserInterface_setActiveMenu;
static jmethodID method_UserInterface_consoleCommand;
static jmethodID method_UserInterface_drawConnectScreen;

void UserInterface_javaRegister()
{
	Com_Printf("UserInterface_javaRegister()\n");

	// load the interface UserInterfaceListener
	interface_UserInterfaceListener = (*javaEnv)->FindClass(javaEnv, "xreal/client/ui/UserInterfaceListener");
	if(CheckException() || !interface_UserInterfaceListener)
	{
		Com_Error(ERR_DROP, "Couldn't find class xreal.client.ui.UserInterfaceListener");
		cls.uiStarted = qtrue;
	}

	// load the class UserInterface
	class_UserInterface = (*javaEnv)->FindClass(javaEnv, "xreal/client/ui/UserInterface");
	if(CheckException() || !class_UserInterface)
	{
		Com_Error(ERR_DROP, "Couldn't find class xreal.client.ui.UserInterface");
		cls.uiStarted = qtrue;
	}

	// check class UserInterface against interface UserInterfaceListener
	if(!((*javaEnv)->IsAssignableFrom(javaEnv, class_UserInterface, interface_UserInterfaceListener)))
	{
		Com_Error(ERR_DROP, "The specified UserInterface class doesn't implement xreal.client.ui.UserInterfaceListener");
		cls.uiStarted = qtrue;
	}

	// remove old game if existing
	(*javaEnv)->DeleteLocalRef(javaEnv, interface_UserInterfaceListener);

	// load game interface methods
	method_UserInterface_initUserInterface = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "initUserInterface", "()V");
	method_UserInterface_shutdownUserInterface = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "shutdownUserInterface", "()V");
	method_UserInterface_keyEvent = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "keyEvent", "(IIZ)V");
	method_UserInterface_mouseEvent = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "mouseEvent", "(III)V");
	method_UserInterface_refresh = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "refresh", "(I)V");
	method_UserInterface_isFullscreen = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "isFullscreen", "()Z");
	method_UserInterface_setActiveMenu = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "setActiveMenu", "(I)V");
	method_UserInterface_refresh = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "refresh", "(I)V");
	method_UserInterface_drawConnectScreen = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "drawConnectScreen", "(Z)V");
	method_UserInterface_consoleCommand = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "consoleCommand", "(I)Z");
	if(CheckException())
	{
		Com_Error(ERR_DROP, "Problem getting handle for one or more of the UserInterface methods\n");
		cls.uiStarted = qtrue;
	}

	// load constructor
	method_UserInterface_ctor = (*javaEnv)->GetMethodID(javaEnv, class_UserInterface, "<init>", "(IIF)V");

	object_UserInterface = (*javaEnv)->NewObject(javaEnv, class_UserInterface, method_UserInterface_ctor, cls.glconfig.vidWidth, cls.glconfig.vidHeight, cls.glconfig.windowAspect);
	if(CheckException())
	{
		Com_Error(ERR_DROP, "Couldn't create instance of the class UserInterface");
		cls.uiStarted = qtrue;
	}
}


void UserInterface_javaDetach()
{
	Com_Printf("UserInterface_javaDetach()\n");

	if(javaEnv)
	{
		if(class_UserInterface)
		{
			(*javaEnv)->DeleteLocalRef(javaEnv, class_UserInterface);
			class_UserInterface = NULL;
		}

		if(object_UserInterface)
		{
			(*javaEnv)->DeleteLocalRef(javaEnv, object_UserInterface);
			object_UserInterface = NULL;
		}
	}
}

void Java_UI_Init(void)
{
	if(!object_UserInterface)
		return;

	//Com_Printf("Java_UI_Init\n");

	(*javaEnv)->CallVoidMethod(javaEnv, object_UserInterface, method_UserInterface_initUserInterface);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_UI_Init()");
	}
}

void Java_UI_Shutdown(void)
{
	if(!object_UserInterface)
		return;

	//Com_Printf("Java_UI_Shutdown\n");

	(*javaEnv)->CallVoidMethod(javaEnv, object_UserInterface, method_UserInterface_shutdownUserInterface);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_UI_Shutdown()");
	}
}

void Java_UI_KeyEvent(int key, qboolean down)
{
	if(!object_UserInterface)
		return;

	//Com_Printf("Java_UI_KeyEvent(key = %i, down = %i)\n", key, down);

	(*javaEnv)->CallVoidMethod(javaEnv, object_UserInterface, method_UserInterface_keyEvent, cls.realtime, key, down);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_UI_KeyEvent(key = %i, down = %i)", key, down);
	}
}

void Java_UI_MouseEvent(int dx, int dy)
{
	if(!object_UserInterface)
		return;

	//Com_Printf("Java_UI_MouseEvent(dx = %i, dy = %i)\n", dx, dy);

	(*javaEnv)->CallVoidMethod(javaEnv, object_UserInterface, method_UserInterface_mouseEvent, cls.realtime, dx, dy);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_UI_MouseEvent(dx = %i, dy = %i)", dx, dy);
	}
}

void Java_UI_Refresh(int time)
{
	if(!object_UserInterface)
		return;

	//Com_Printf("Java_UI_Refresh(time = %i)\n", time);

	(*javaEnv)->CallVoidMethod(javaEnv, object_UserInterface, method_UserInterface_refresh, time);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_UI_Refresh(time = %i)", time);
	}
}

qboolean Java_UI_IsFullscreen(void)
{
	jboolean result;

	if(!object_UserInterface)
		return qfalse;

	//Com_Printf("Java_UI_IsFullscreen\n");

	result = (*javaEnv)->CallBooleanMethod(javaEnv, object_UserInterface, method_UserInterface_isFullscreen);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_UI_isFullscreen()");
	}

	return result;
}

void Java_UI_SetActiveMenu(uiMenuCommand_t menu)
{
	if(!object_UserInterface)
		return;

	//Com_Printf("Java_UI_SetActiveMenu(menu = %i)\n", menu);

	(*javaEnv)->CallVoidMethod(javaEnv, object_UserInterface, method_UserInterface_setActiveMenu, menu);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_UI_SetActiveMenu(menu = %i)", menu);
	}
}

void Java_UI_DrawConnectScreen(qboolean overlay)
{
	if(!object_UserInterface)
		return;

	//Com_Printf("Java_UI_DrawConnectScreen(overlay = %i)\n", overlay);

	(*javaEnv)->CallVoidMethod(javaEnv, object_UserInterface, method_UserInterface_drawConnectScreen, overlay);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_UI_DrawConnectScreen(overlay = %i)", overlay);
	}
}

qboolean Java_UI_ConsoleCommand(int realTime)
{
	jboolean result;

	if(!object_UserInterface)
		return qfalse;

	//Com_Printf("Java_UI_ConsoleCommand(realTime = %i)\n", realTime);

	result = (*javaEnv)->CallBooleanMethod(javaEnv, object_UserInterface, method_UserInterface_consoleCommand, realTime);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_UI_ConsoleCommand(realTime = %i)", realTime);
	}

	return result;
}

// ====================================================================================



/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI(void)
{
	Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_UI);
	cls.uiStarted = qfalse;

	if(!javaEnv)
	{
		Com_Printf("Can't stop Java user interface module, javaEnv pointer was null\n");
		return;
	}

	CheckException();

	Java_UI_Shutdown();

	CheckException();

	Client_javaDetach();
	Snapshot_javaDetach();

	Renderer_javaDetach();
	Font_javaDetach();
	RefSkeleton_javaDetach();

	UserInterface_javaDetach();
}

/*
====================
CL_InitUI
====================
*/
void CL_InitUI(void)
{
	Client_javaRegister();
	Snapshot_javaRegister();

	Renderer_javaRegister();
	Font_javaRegister();
	RefSkeleton_javaRegister();

	UserInterface_javaRegister();

	// init for this gamestate
	Java_UI_Init(/*(cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE)*/);

	// reset any CVAR_CHEAT cvars registered by ui
	if(!clc.demoplaying && !cl_connectedToCheatServer)
		Cvar_SetCheatState();
}

/*
====================
UI_GameCommand

See if the current console command is claimed by the ui
====================
*/
qboolean UI_GameCommand(void)
{
	return Java_UI_ConsoleCommand(cls.realtime);
}






#endif
