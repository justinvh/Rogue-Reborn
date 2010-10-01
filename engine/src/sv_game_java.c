/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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

#include <hat/engine/server.h>

#if defined(USE_JAVA)

#include "../qcommon/vm_java.h"
#include "java/xreal_server_Server.h"
#include "java/xreal_server_game_GameEntity.h"


typedef struct gclient_s
{
	playerState_t   ps;

	jobject         object_Player;

	jstring         playerInfo;

	jmethodID		method_Player_clientBegin;
	jmethodID		method_Player_clientUserInfoChanged;
	jmethodID		method_Player_clientDisconnect;
	jmethodID		method_Player_clientCommand;
	jmethodID		method_Player_clientThink;
} gclient_t;

typedef struct gentity_s
{
	entityState_t   s;			// communicated by server to clients
	entityShared_t  r;			// shared by both the server system and game
	gclient_t      *client;		// NULL if not a client

	qboolean        inUse;
	qboolean        neverFree;	// if true, FreeEntity will only unlink

	int				spawnTime;	// level.time when the object was spawned
	int             freeTime;	// level.time when the object was freed
} gentity_t;

static gentity_t		g_entities[MAX_GENTITIES];
static gclient_t		g_clients[MAX_CLIENTS];
static int				g_levelStartTime; // for relaxing entity free policies
static int				g_levelTime;


int SV_NumForGentity(sharedEntity_t * ent)
{
	int             num;

	num = ((byte *) ent - (byte *) g_entities) / sizeof(gentity_t);
//	num = ent - sv.gentities;
//	num = ent - g_entities;

	return num;
}

sharedEntity_t *SV_GentityNum(int num)
{
	sharedEntity_t *ent;

	ent = (sharedEntity_t *) ((byte *) g_entities + sizeof(gentity_t) * (num));
//	ent = &sv.gentities[num];	// &g_entities[num];

	return ent;
}

playerState_t  *SV_GameClientNum(int num)
{
	playerState_t  *ps;

//	ps = (playerState_t *) ((byte *) sv.gameClients + sv.gameClientSize * (num));
	ps = (playerState_t *) &g_clients[num].ps;

	return ps;
}

svEntity_t     *SV_SvEntityForGentity(sharedEntity_t * gEnt)
{
	if(!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
	}

	return &sv.svEntities[gEnt->s.number];
}

sharedEntity_t *SV_GEntityForSvEntity(svEntity_t * svEnt)
{
	int             num;

	num = svEnt - sv.svEntities;
	return SV_GentityNum(num);
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient(int clientNum, const char *reason)
{
	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel(sharedEntity_t * ent, const char *name)
{
	clipHandle_t    h;
	vec3_t          mins, maxs;

	if(!name)
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: NULL");
	}

	if(name[0] != '*')
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name);
	}


	ent->s.modelindex = atoi(name + 1);

	h = CM_InlineModel(ent->s.modelindex);
	CM_ModelBounds(h, mins, maxs);
	VectorCopy(mins, ent->r.mins);
	VectorCopy(maxs, ent->r.maxs);
	ent->r.bmodel = qtrue;

	ent->r.contents = -1;		// we don't know exactly what is in the brushes

	// Tr3B: uncommented
	//SV_LinkEntity(ent);           // FIXME: remove
}



/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_inPVS(const vec3_t p1, const vec3_t p2)
{
	int             leafnum;
	int             cluster;
	int             area1, area2;
	byte           *mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1 = CM_LeafArea(leafnum);
	mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2 = CM_LeafArea(leafnum);
	if(mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
		return qfalse;
	if(!CM_AreasConnected(area1, area2))
		return qfalse;			// a door blocks sight
	return qtrue;
}


/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
qboolean SV_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	int             leafnum;
	int             cluster;
	int             area1, area2;
	byte           *mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1 = CM_LeafArea(leafnum);
	mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2 = CM_LeafArea(leafnum);

	if(mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
		return qfalse;

	return qtrue;
}


/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState(sharedEntity_t * ent, qboolean open)
{
	svEntity_t     *svEnt;

	svEnt = SV_SvEntityForGentity(ent);
	if(svEnt->areanum2 == -1)
	{
		return;
	}
	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}


/*
==================
SV_GameAreaEntities
==================
*/
qboolean SV_EntityContact(vec3_t mins, vec3_t maxs, const sharedEntity_t * gEnt, traceType_t type)
{
	const float    *origin, *angles;
	clipHandle_t    ch;
	trace_t         trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity(gEnt);
	CM_TransformedBoxTrace(&trace, vec3_origin, vec3_origin, mins, maxs, ch, -1, origin, angles, type);

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo
===============
*/
void SV_GetServerinfo(char *buffer, int bufferSize)
{
	if(bufferSize < 1)
	{
		Com_Error(ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize);
	}
	Q_strncpyz(buffer, Cvar_InfoString(CVAR_SERVERINFO), bufferSize);
}



// ====================================================================================



// handle to Game class
static jclass   class_Game = NULL;
static jobject  object_Game = NULL;
static jclass   interface_GameListener;
static jmethodID method_Game_ctor;
static jmethodID method_Game_initGame;
static jmethodID method_Game_shutdownGame;
static jmethodID method_Game_runFrame;
static jmethodID method_Game_runAIFrame;
static jmethodID method_Game_consoleCommand;

void Game_javaRegister()
{
	Com_DPrintf("Game_javaRegister()\n");

	// load the interface GameListener
	interface_GameListener = (*javaEnv)->FindClass(javaEnv, "xreal/server/game/GameListener");
	if(CheckException() || !interface_GameListener)
	{
		Com_Error(ERR_DROP, "Couldn't find class xreal.server.game.GameListener");
	}

	// load the class Game
	class_Game = (*javaEnv)->FindClass(javaEnv, "xreal/server/game/Game");
	if(CheckException() || !class_Game)
	{
		Com_Error(ERR_DROP, "Couldn't find class xreal.server.game.Game");
	}

	// check class Game against interface GameListener
	if(!((*javaEnv)->IsAssignableFrom(javaEnv, class_Game, interface_GameListener)))
	{
		Com_Error(ERR_DROP, "The specified game class doesn't implement xreal.server.game.GameListener");
	}

	// remove old game if existing
	(*javaEnv)->DeleteLocalRef(javaEnv, interface_GameListener);

	// load game interface methods
	method_Game_initGame = (*javaEnv)->GetMethodID(javaEnv, class_Game, "initGame", "(IIZ)V");
	method_Game_shutdownGame = (*javaEnv)->GetMethodID(javaEnv, class_Game, "shutdownGame", "(Z)V");
	method_Game_runFrame = (*javaEnv)->GetMethodID(javaEnv, class_Game, "runFrame", "(I)V");
	method_Game_runAIFrame = (*javaEnv)->GetMethodID(javaEnv, class_Game, "runAIFrame", "(I)V");
	method_Game_consoleCommand = (*javaEnv)->GetMethodID(javaEnv, class_Game, "consoleCommand", "()Z");
	if(CheckException())
	{
		Com_Error(ERR_DROP, "Problem getting handle for one or more of the Game methods\n");
	}

	// load constructor
	method_Game_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Game, "<init>", "()V");

	object_Game = (*javaEnv)->NewObject(javaEnv, class_Game, method_Game_ctor);
	if(CheckException())
	{
		Com_Error(ERR_DROP, "Couldn't create instance of the class Game");
	}
}


void Game_javaDetach()
{
	Com_DPrintf("Game_javaDetach()\n");

	if(javaEnv)
	{
		if(class_Game)
		{
			(*javaEnv)->DeleteLocalRef(javaEnv, class_Game);
			class_Game = NULL;
		}

		if(object_Game)
		{
			(*javaEnv)->DeleteLocalRef(javaEnv, object_Game);
			object_Game = NULL;
		}
	}
}

// ====================================================================================


/*
 * Class:     xreal_server_Server
 * Method:    getConfigstring
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_xreal_server_Server_getConfigString(JNIEnv *env, jclass cls, jint index)
{
	if(index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_Server_getConfigString: bad index %i\n", index);
	}

	if(!sv.configstrings[index])
	{
		return NULL;
	}

	return (*env)->NewStringUTF(env, sv.configstrings[index]);
}

/*
 * Class:     xreal_server_Server
 * Method:    setConfigstring
 * Signature: (ILjava/lang/String;)V
 */
void JNICALL Java_xreal_server_Server_setConfigString(JNIEnv *env, jclass cls, jint jindex, jstring jvalue)
{
	char           *value;

	value = (char *)((*env)->GetStringUTFChars(env, jvalue, 0));

	SV_SetConfigstring(jindex, value);

	(*env)->ReleaseStringUTFChars(env, jvalue, value);

	//CheckException();
}

/*
 * Class:     xreal_server_Server
 * Method:    broadcastServerCommand
 * Signature: (Ljava/lang/String;)V
 */
void JNICALL Java_xreal_server_Server_broadcastServerCommand(JNIEnv *env, jclass cls, jstring jcommand)
{
	char           *command;

	command = (char *)((*env)->GetStringUTFChars(env, jcommand, 0));

	SV_SendServerCommand(NULL, command);

	(*env)->ReleaseStringUTFChars(env, jcommand, command);

	//CheckException();
}


// handle to Server class
static jclass   class_Server;
static JNINativeMethod Server_methods[] = {
	{"getConfigString", "(I)Ljava/lang/String;", Java_xreal_server_Server_getConfigString},
	{"setConfigString", "(ILjava/lang/String;)V", Java_xreal_server_Server_setConfigString},
	{"broadcastServerCommand", "(Ljava/lang/String;)V", Java_xreal_server_Server_broadcastServerCommand},
};

void Server_javaRegister()
{
	Com_DPrintf("Server_javaRegister()\n");

	class_Server = (*javaEnv)->FindClass(javaEnv, "xreal/server/Server");
	if(CheckException() || !class_Server)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.server.Server");
	}

	(*javaEnv)->RegisterNatives(javaEnv, class_Server, Server_methods, sizeof(Server_methods) / sizeof(Server_methods[0]));
	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't register native methods for xreal.server.Server");
	}
}


void Server_javaDetach()
{
	Com_DPrintf("Server_javaDetach()\n");

	if(javaEnv)
	{
		if(class_Server)
		{
			(*javaEnv)->UnregisterNatives(javaEnv, class_Server);
			(*javaEnv)->DeleteLocalRef(javaEnv, class_Server);
			class_Server = NULL;
		}
	}
}


// ====================================================================================

void G_InitGentity(gentity_t * e)
{
	e->inUse = qtrue;
	e->spawnTime = g_levelTime;
//	e->classname = "noclass";
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;

	// FIXME: this is required as long we don't link entities in the server code
	//e->r.svFlags |= SVF_BROADCAST;
}


/*
=================
G_EntitiesFree
=================
*/
qboolean G_EntitiesFree(void)
{
	int             i;
	gentity_t      *e;

	e = &g_entities[MAX_CLIENTS];
	for(i = MAX_CLIENTS; i < sv.numEntities; i++, e++)
	{
		if(e->inUse)
		{
			continue;
		}

		// slot available
		return qtrue;
	}
	return qfalse;
}




// ====================================================================================

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    allocateEntity
 * Signature: (I)I
 *
 * Either finds a free entity, or allocates a new one.
 *
 * The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
 * never be used by anything else.
 *
 * Try to avoid reusing an entity that was recently freed, because it
 * can cause the client to think the entity morphed into something else
 * instead of being removed and recreated, which can cause interpolated
 * angles and bad trails.
 *
 */
jint JNICALL Java_xreal_server_game_GameEntity_allocateEntity0(JNIEnv *env, jclass cls, jint reservedIndex)
{
	int             i, force;
	gentity_t      *e;

	e = NULL;					// shut up warning
	i = 0;						// shut up warning

	if(reservedIndex >= 0)
	{
		if(reservedIndex > (MAX_CLIENTS - 1) && reservedIndex != ENTITYNUM_WORLD)
		{
			Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_allocateEntity0: bad reserved entity index chosen: %i", reservedIndex);
		}

		e = &g_entities[reservedIndex];

		G_InitGentity(e);
		return e->s.number;
	}

	for(force = 0; force < 2; force++)
	{
		// if we go through all entities and can't find one to free,
		// override the normal minimum times before use
		e = &g_entities[MAX_CLIENTS];

		for(i = MAX_CLIENTS; i < sv.numEntities; i++, e++)
		{
			if(e->inUse)
			{
				continue;
			}

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if(!force && e->freeTime > g_levelStartTime + 2000 && g_levelTime - e->freeTime < 1000)
			{
				continue;
			}

			// reuse this slot
			G_InitGentity(e);
			return e->s.number;
		}

		if(i != MAX_GENTITIES)
		{
			break;
		}
	}

	if(i == ENTITYNUM_MAX_NORMAL)
	{
		/*
		for(i = 0; i < MAX_GENTITIES; i++)
		{
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		*/

		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_allocateEntity0: no free entities");
	}

	// open up a new slot
	sv.numEntities++;

	G_InitGentity(e);
	return e->s.number;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    freeEntity0
 * Signature: (I)Z
 */
jboolean JNICALL Java_xreal_server_game_GameEntity_freeEntity0(JNIEnv *env, jclass cls, jint index)
{
	//gentity_t *e = (gentity_t *) entityPtr;
	gentity_t *e = &g_entities[index];

	//trap_UnlinkEntity(e);		// unlink from world

	if(e->neverFree)
	{
		return qfalse;
	}

	memset(e, 0, sizeof(*e));

	//e->classname = "freed";
	e->freeTime = g_levelTime;
	e->inUse = qfalse;
	e->s.number = index;

	return (jboolean) qtrue;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    linkEntity0
 * Signature: (I)V
 */
void JNICALL Java_xreal_server_game_GameEntity_linkEntity0(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_linkEntity0: bad index %i\n", index);
	}
	ent = &g_entities[index];

	SV_LinkEntity((sharedEntity_t *) ent);
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    unlinkEntity0
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_xreal_server_game_GameEntity_unlinkEntity0(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_unlinkEntity0: bad index %i\n", index);
	}
	ent = &g_entities[index];

	SV_UnlinkEntity((sharedEntity_t *) ent);
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_eType
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1eType(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1eType: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.eType;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_eType
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1eType(JNIEnv *env, jclass cls, jint index, jint type)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1eType: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.eType = type;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_eFlags
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1eFlags(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1eFlags: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.eFlags;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_eFlags
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1eFlags(JNIEnv *env, jclass cls, jint index, jint flags)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1eFlags: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.eFlags = flags;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_pos
 * Signature: (I)Lxreal/Trajectory;
 */
jobject JNICALL Java_xreal_server_game_GameEntity_getEntityState_1pos(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1pos: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return Java_NewTrajectory(&ent->s.pos);
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_pos
 * Signature: (IIIIFFFFFFF)V
 */
//int index, int trType, int trTime, int trDuration, float trAcceleration, float trBaseX, float trBaseY, float trBaseZ, float trDeltaX, float trDeltaY, float trDeltaZ);
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1pos(JNIEnv *env, jclass cls, jint index, jint trType, jint trTime, jint trDuration, jfloat trAcceleration, jfloat trBaseX, jfloat trBaseY, jfloat trBaseZ, jfloat trBaseW, jfloat trDeltaX, jfloat trDeltaY, jfloat trDeltaZ, jfloat trDeltaW)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1pos: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.pos.trType = trType;
	ent->s.pos.trTime = trTime;
	ent->s.pos.trDuration = trDuration;
	ent->s.pos.trAcceleration = trAcceleration;

	ent->s.pos.trBase[0] = trBaseX;
	ent->s.pos.trBase[1] = trBaseY;
	ent->s.pos.trBase[2] = trBaseZ;
	ent->s.pos.trBase[3] = trBaseW;

	ent->s.pos.trDelta[0] = trDeltaX;
	ent->s.pos.trDelta[1] = trDeltaY;
	ent->s.pos.trDelta[2] = trDeltaZ;
	ent->s.pos.trDelta[3] = trDeltaW;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_apos
 * Signature: (I)Lxreal/Trajectory;
 */
jobject JNICALL Java_xreal_server_game_GameEntity_getEntityState_1apos(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1apos: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return Java_NewTrajectory(&ent->s.apos);
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_apos
 * Signature: (IIIIFFFFFFF)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1apos(JNIEnv *env, jclass cls, jint index, jint trType, jint trTime, jint trDuration, jfloat trAcceleration, jfloat trBaseX, jfloat trBaseY, jfloat trBaseZ, jfloat trBaseW, jfloat trDeltaX, jfloat trDeltaY, jfloat trDeltaZ, jfloat trDeltaW)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1apos: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.apos.trType = trType;
	ent->s.apos.trTime = trTime;
	ent->s.apos.trDuration = trDuration;
	ent->s.apos.trAcceleration = trAcceleration;

	ent->s.apos.trBase[0] = trBaseX;
	ent->s.apos.trBase[1] = trBaseY;
	ent->s.apos.trBase[2] = trBaseZ;
	ent->s.apos.trBase[3] = trBaseW;

	ent->s.apos.trDelta[0] = trDeltaX;
	ent->s.apos.trDelta[1] = trDeltaY;
	ent->s.apos.trDelta[2] = trDeltaZ;
	ent->s.apos.trDelta[3] = trDeltaW;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_time
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1time(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1time: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.time;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_time
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1time(JNIEnv *env, jclass cls, jint index, jint time)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1time: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.time = time;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_time2
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1time2(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1time2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.time2;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_time2
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1time2(JNIEnv *env, jclass cls, jint index, jint time2)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1time2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.time2 = time2;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_origin
 * Signature: (I)Ljavax/vecmath/Vector3f;
 */
jobject JNICALL Java_xreal_server_game_GameEntity_getEntityState_1origin(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1origin: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return Java_NewVector3f(ent->s.origin);
}


/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_origin
 * Signature: (IFFF)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1origin(JNIEnv *env, jclass cls, jint index, jfloat x, jfloat y, jfloat z)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1origin: bad index %i\n", index);
	}
	ent = &g_entities[index];

	VectorSet(ent->s.origin, x, y, z);
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_origin2
 * Signature: (I)Ljavax/vecmath/Vector3f;
 */
jobject JNICALL Java_xreal_server_game_GameEntity_getEntityState_1origin2(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1origin2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return Java_NewVector3f(ent->s.origin2);
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_origin2
 * Signature: (IFFF)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1origin2(JNIEnv *env, jclass cls, jint index, jfloat x, jfloat y, jfloat z)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1origin2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	VectorSet(ent->s.origin2, x, y, z);
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_angles
 * Signature: (I)Lxreal/Angle3f;
 */
jobject JNICALL Java_xreal_server_game_GameEntity_getEntityState_1angles(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1angles: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return Java_NewAngle3f(ent->s.angles[PITCH], ent->s.angles[YAW], ent->s.angles[ROLL]);
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_angles
 * Signature: (IFFF)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1angles(JNIEnv *env, jclass cls, jint index, jfloat pitch, jfloat yaw, jfloat roll)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1angles: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.angles[PITCH] = pitch;
	ent->s.angles[YAW] = yaw;
	ent->s.angles[ROLL] = roll;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_angles2
 * Signature: (I)Lxreal/Angle3f;
 */
jobject JNICALL Java_xreal_server_game_GameEntity_getEntityState_1angles2(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1angles2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return Java_NewAngle3f(ent->s.angles2[PITCH], ent->s.angles2[YAW], ent->s.angles2[ROLL]);
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_angles2
 * Signature: (IFFF)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1angles2(JNIEnv *env, jclass cls, jint index, jfloat pitch, jfloat yaw, jfloat roll)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1angles2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.angles2[PITCH] = pitch;
	ent->s.angles2[YAW] = yaw;
	ent->s.angles2[ROLL] = roll;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_otherEntityNum
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1otherEntityNum(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1otherEntityNum: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.otherEntityNum;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_otherEntityNum
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1otherEntityNum(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1otherEntityNum: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.otherEntityNum = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_otherEntityNum2
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1otherEntityNum2(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1otherEntityNum2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.otherEntityNum2;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_otherEntityNum2
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1otherEntityNum2(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1otherEntityNum2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.otherEntityNum2 = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_groundEntityNum
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1groundEntityNum(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1groundEntityNum: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.groundEntityNum;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_groundEntityNum
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1groundEntityNum(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1groundEntityNum: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.groundEntityNum = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_constantLight
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1constantLight(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1constantLight: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.constantLight;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_constantLight
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1constantLight(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1constantLight: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.constantLight = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_loopSound
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1loopSound(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1loopSound: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.loopSound;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_loopSound
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1loopSound(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1loopSound: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.loopSound = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_modelindex
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1modelindex(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1modelindex: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.modelindex;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_modelindex
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1modelindex(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1modelindex: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.modelindex = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_modelindex2
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1modelindex2(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1modelindex2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.modelindex2;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_modelindex2
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1modelindex2(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1modelindex2: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.modelindex2 = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_clientNum
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1clientNum(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1clientNum: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.clientNum;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_clientNum
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1clientNum(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1clientNum: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.clientNum = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_frame
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1frame(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1frame: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.frame;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_frame
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1frame(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1frame: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.frame = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_solid
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1solid(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1solid: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.solid;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_solid
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1solid(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1solid: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.solid = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_event
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1event(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1event: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.event;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_event
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1event(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1event: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.event = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_eventParm
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1eventParm(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1eventParm: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.eventParm;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_eventParm
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1eventParm(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1eventParm: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.eventParm = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_powerups
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1powerups(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1powerups: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.powerups;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_powerups
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1powerups(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1powerups: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.powerups = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_weapon
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1weapon(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1weapon: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.weapon;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_weapon
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1weapon(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1weapon: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.weapon = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_legsAnim
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1legsAnim(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1legsAnim: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.legsAnim;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_legsAnim
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1legsAnim(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1legsAnim: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.legsAnim = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_torsoAnim
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1torsoAnim(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1torsoAnim: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.torsoAnim;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_torsoAnim
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1torsoAnim(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1torsoAnim: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.torsoAnim = value;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    getEntityState_generic1
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_GameEntity_getEntityState_1generic1(JNIEnv *env, jclass cls, jint index)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_getEntityState_1generic1: bad index %i\n", index);
	}
	ent = &g_entities[index];

	return ent->s.generic1;
}

/*
 * Class:     xreal_server_game_GameEntity
 * Method:    setEntityState_generic1
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_GameEntity_setEntityState_1generic1(JNIEnv *env, jclass cls, jint index, jint value)
{
	gentity_t	   *ent;

	if(index < 0 || index >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_GameEntity_setEntityState_1generic1: bad index %i\n", index);
	}
	ent = &g_entities[index];

	ent->s.generic1 = value;
}



// handle to GameEntity class
static jclass   class_GameEntity;
static JNINativeMethod GameEntity_methods[] = {
	{"allocateEntity0", "(I)I", Java_xreal_server_game_GameEntity_allocateEntity0},
	{"freeEntity0", "(I)Z", Java_xreal_server_game_GameEntity_freeEntity0},

	{"linkEntity0", "(I)V", Java_xreal_server_game_GameEntity_linkEntity0},
	{"unlinkEntity0", "(I)V", Java_xreal_server_game_GameEntity_unlinkEntity0},

	{"getEntityState_eType", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1eType},
	{"setEntityState_eType", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1eType},

	{"getEntityState_eFlags", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1eFlags},
	{"setEntityState_eFlags", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1eFlags},

	{"getEntityState_pos", "(I)Lxreal/Trajectory;", Java_xreal_server_game_GameEntity_getEntityState_1pos},
	{"setEntityState_pos", "(IIIIFFFFFFFFF)V", Java_xreal_server_game_GameEntity_setEntityState_1pos},

	{"getEntityState_apos", "(I)Lxreal/Trajectory;", Java_xreal_server_game_GameEntity_getEntityState_1apos},
	{"setEntityState_apos", "(IIIIFFFFFFFFF)V", Java_xreal_server_game_GameEntity_setEntityState_1apos},

	{"getEntityState_time", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1time},
	{"setEntityState_time", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1time},

	{"getEntityState_time2", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1time2},
	{"setEntityState_time2", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1time2},

	{"getEntityState_origin", "(I)Ljavax/vecmath/Vector3f;", Java_xreal_server_game_GameEntity_getEntityState_1origin},
	{"setEntityState_origin", "(IFFF)V", Java_xreal_server_game_GameEntity_setEntityState_1origin},

	{"getEntityState_origin2", "(I)Ljavax/vecmath/Vector3f;", Java_xreal_server_game_GameEntity_getEntityState_1origin2},
	{"setEntityState_origin2", "(IFFF)V", Java_xreal_server_game_GameEntity_setEntityState_1origin2},

	{"getEntityState_angles", "(I)Lxreal/Angle3f;", Java_xreal_server_game_GameEntity_getEntityState_1angles},
	{"setEntityState_angles", "(IFFF)V", Java_xreal_server_game_GameEntity_setEntityState_1angles},

	{"getEntityState_angles2", "(I)Lxreal/Angle3f;", Java_xreal_server_game_GameEntity_getEntityState_1angles2},
	{"setEntityState_angles2", "(IFFF)V", Java_xreal_server_game_GameEntity_setEntityState_1angles2},

	{"getEntityState_otherEntityNum", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1otherEntityNum},
	{"setEntityState_otherEntityNum", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1otherEntityNum},

	{"getEntityState_otherEntityNum2", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1otherEntityNum2},
	{"setEntityState_otherEntityNum2", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1otherEntityNum2},

	{"getEntityState_groundEntityNum", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1groundEntityNum},
	{"setEntityState_groundEntityNum", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1groundEntityNum},

	{"getEntityState_constantLight", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1constantLight},
	{"setEntityState_constantLight", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1constantLight},

	{"getEntityState_loopSound", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1loopSound},
	{"setEntityState_loopSound", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1loopSound},

	{"getEntityState_modelindex", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1modelindex},
	{"setEntityState_modelindex", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1modelindex},

	{"getEntityState_modelindex2", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1modelindex2},
	{"setEntityState_modelindex2", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1modelindex2},

	{"getEntityState_clientNum", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1clientNum},
	{"setEntityState_clientNum", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1clientNum},

	{"getEntityState_frame", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1frame},
	{"setEntityState_frame", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1frame},

	{"getEntityState_solid", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1solid},
	{"setEntityState_solid", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1solid},

	{"getEntityState_event", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1event},
	{"setEntityState_event", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1event},

	{"getEntityState_eventParm", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1eventParm},
	{"setEntityState_eventParm", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1eventParm},

	{"getEntityState_powerups", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1powerups},
	{"setEntityState_powerups", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1powerups},

	{"getEntityState_weapon", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1weapon},
	{"setEntityState_weapon", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1weapon},

	{"getEntityState_legsAnim", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1legsAnim},
	{"setEntityState_legsAnim", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1legsAnim},

	{"getEntityState_torsoAnim", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1torsoAnim},
	{"setEntityState_torsoAnim", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1torsoAnim},

	{"getEntityState_generic1", "(I)I", Java_xreal_server_game_GameEntity_getEntityState_1generic1},
	{"setEntityState_generic1", "(II)V", Java_xreal_server_game_GameEntity_setEntityState_1generic1},
};

void GameEntity_javaRegister()
{
	Com_DPrintf("GameEntity_javaRegister()\n");

	class_GameEntity = (*javaEnv)->FindClass(javaEnv, "xreal/server/game/GameEntity");
	if(CheckException() || !class_GameEntity)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.server.game.GameEntity");
	}

	(*javaEnv)->RegisterNatives(javaEnv, class_GameEntity, GameEntity_methods, sizeof(GameEntity_methods) / sizeof(GameEntity_methods[0]));
	if(CheckException())
	{
		Com_Error(ERR_FATAL, "Couldn't register native methods for xreal.server.game.GameEntity");
	}
}


void GameEntity_javaDetach()
{
	Com_DPrintf("GameEntity_javaDetach()\n");

	if(javaEnv)
	{
		if(class_GameEntity)
		{
			(*javaEnv)->UnregisterNatives(javaEnv, class_GameEntity);
			(*javaEnv)->DeleteLocalRef(javaEnv, class_GameEntity);
			class_GameEntity = NULL;
		}
	}
}

// ====================================================================================


static jclass   interface_ClientListener;

void ClientListener_javaRegister()
{
	Com_DPrintf("ClientListener_javaRegister()\n");

	interface_ClientListener = (*javaEnv)->FindClass(javaEnv, "xreal/server/game/ClientListener");
	if(CheckException() || !interface_ClientListener)
	{
		Com_Error(ERR_FATAL, "Couldn't find xreal.server.game.ClientListener");
	}
}


void ClientListener_javaDetach()
{
	Com_DPrintf("ClientListener_javaDetach()\n");

	if(javaEnv)
	{
		if(interface_ClientListener)
		{
			(*javaEnv)->DeleteLocalRef(javaEnv, interface_ClientListener);
			interface_ClientListener = NULL;
		}
	}
}

// ====================================================================================

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs(void)
{
	if(!javaEnv)
	{
		Com_Printf("Can't stop Java game module, javaEnv pointer was null\n");
		return;
	}

	CheckException();

	Java_G_ShutdownGame(qfalse);

	CheckException();

	Server_javaDetach();
	Game_javaDetach();
	GameEntity_javaDetach();
	ClientListener_javaDetach();
}



void Java_G_GameInit(int levelTime, int randomSeed, qboolean restart)
{
	if(!object_Game)
		return;

	g_levelStartTime = levelTime;
	g_levelTime = levelTime;

	(*javaEnv)->CallVoidMethod(javaEnv, object_Game, method_Game_initGame, levelTime, randomSeed, restart);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_GameInit(levelTime = %i, randomSeed = %i, restart = %i)", levelTime, randomSeed, restart);
	}
}

void Java_G_ShutdownGame(qboolean restart)
{
	if(!object_Game)
		return;

	(*javaEnv)->CallVoidMethod(javaEnv, object_Game, method_Game_shutdownGame, restart);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_ShutdownGame(restart = %i)", restart);
	}
}











/*
 * Class:     xreal_server_game_Player
 * Method:    sendServerCommand
 * Signature: (ILjava/lang/String;)V
 */
void JNICALL Java_xreal_server_game_Player_sendServerCommand(JNIEnv *env, jclass cls, jint clientNum, jstring jcommand)
{
	char           *command;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_sendServerCommand: bad index %i\n", clientNum);
	}

	command = (char *)((*env)->GetStringUTFChars(env, jcommand, 0));

	SV_SendServerCommand(svs.clients + clientNum, "%s", command);

	(*env)->ReleaseStringUTFChars(env, jcommand, command);
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getUserInfo
 * Signature: (I)Ljava/lang/String;
 */
jstring JNICALL Java_xreal_server_game_Player_getUserInfo(JNIEnv *env, jclass cls, jint clientNum)
{
	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getUserInfo: bad index %i\n", clientNum);
	}

	return (*env)->NewStringUTF(env, svs.clients[clientNum].userinfo);
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setUserInfo
 * Signature: (ILjava/lang/String;)V
 */
void JNICALL Java_xreal_server_game_Player_setUserInfo(JNIEnv *env, jclass cls, jint clientNum, jstring juserinfo)
{
	char           *userinfo;

	userinfo = (char *)((*env)->GetStringUTFChars(env, juserinfo, 0));

	SV_SetUserinfo(clientNum, userinfo);

	(*env)->ReleaseStringUTFChars(env, juserinfo, userinfo);
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getUserCommand
 * Signature: (I)Lxreal/UserCommand;
 */
jobject JNICALL Java_xreal_server_game_Player_getUserCommand(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getUserCommand: bad index %i\n", clientNum);
	}

	client = &g_clients[clientNum];
	return Java_NewUserCommand(&svs.clients[clientNum].lastUsercmd);
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_commandTime
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1commandTime(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1commandTime: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return client->ps.commandTime;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_commandTime
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1commandTime(JNIEnv *env, jclass cls, jint clientNum, jint time)
{
	gclient_t	   *client;

	//Com_Printf("Java_xreal_server_game_Player_setPlayerState_1commandTime(clientNum = %i, time = %i)\n", clientNum, time);

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1commandTime: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	client->ps.commandTime = time;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_pm_type
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1pm_1type(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1pm_1type: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return client->ps.pm_type;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_pm_type
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1pm_1type(JNIEnv *env, jclass cls, jint clientNum, jint type)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1pm_1type: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	client->ps.pm_type = type;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_pm_flags
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1pm_1flags(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1pm_1flags: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return client->ps.pm_flags;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_pm_flags
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1pm_1flags(JNIEnv *env, jclass cls, jint clientNum, jint flags)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1pm_1flags: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	client->ps.pm_flags = flags;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_pm_time
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1pm_1time(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1pm_1time: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return client->ps.pm_time;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_pm_time
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1pm_1time(JNIEnv *env, jclass cls, jint clientNum, jint time)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1pm_1time: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	client->ps.pm_time = time;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_origin
 * Signature: (I)Ljavax/vecmath/Vector3f;
 */
jobject JNICALL Java_xreal_server_game_Player_getPlayerState_1origin(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1origin: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return Java_NewVector3f(client->ps.origin);
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_origin
 * Signature: (IFFF)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1origin(JNIEnv *env, jclass cls, jint clientNum, jfloat x, jfloat y, jfloat z)
{
	gclient_t	   *client;
	gentity_t      *ent;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1origin: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];
	ent = &g_entities[clientNum];

	client->ps.origin[0] = x;
	client->ps.origin[1] = y;
	client->ps.origin[2] = z;

	VectorCopy(client->ps.origin, ent->s.pos.trBase);
	VectorCopy(client->ps.origin, ent->s.origin);
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_velocity
 * Signature: (I)Ljavax/vecmath/Vector3f;
 */
jobject JNICALL Java_xreal_server_game_Player_getPlayerState_1velocity(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1velocity: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return Java_NewVector3f(client->ps.velocity);
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_velocity
 * Signature: (IFFF)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1velocity(JNIEnv *env, jclass cls, jint clientNum, jfloat x, jfloat y, jfloat z)
{
	gclient_t	   *client;
	gentity_t      *ent;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1velocity: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];
	ent = &g_entities[clientNum];

	client->ps.velocity[0] = x;
	client->ps.velocity[1] = y;
	client->ps.velocity[2] = z;

	ent->s.pos.trDelta[0] = x;
	ent->s.pos.trDelta[1] = y;
	ent->s.pos.trDelta[2] = z;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_speed
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1speed(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1speed: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return client->ps.speed;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_speed
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1speed(JNIEnv *env, jclass cls, jint clientNum, jint speed)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1speed: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	client->ps.speed = speed;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_deltaPitch
 * Signature: (I)S
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1deltaPitch(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1deltaPitch: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return client->ps.delta_angles[PITCH];
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_deltaPitch
 * Signature: (IS)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1deltaPitch(JNIEnv *env, jclass cls, jint clientNum, jshort angle)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1deltaPitch: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	client->ps.delta_angles[PITCH] = angle;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_deltaYaw
 * Signature: (I)S
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1deltaYaw(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1deltaYaw: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return client->ps.delta_angles[YAW];
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_deltaYaw
 * Signature: (IS)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1deltaYaw(JNIEnv *env, jclass cls, jint clientNum, jshort angle)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1deltaYaw: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	client->ps.delta_angles[YAW] = angle;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_deltaRoll
 * Signature: (I)S
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1deltaRoll(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1deltaRoll: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return client->ps.delta_angles[ROLL];
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_deltaRoll
 * Signature: (IS)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1deltaRoll(JNIEnv *env, jclass cls, jint clientNum, jshort angle)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1deltaRoll: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	client->ps.delta_angles[ROLL] = angle;
}


/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_groundEntityNum
 * Signature: (I)I
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1groundEntityNum(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1groundEntityNum: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return client->ps.groundEntityNum;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_groundEntityNum
 * Signature: (II)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1groundEntityNum(JNIEnv *env, jclass cls, jint clientNum, jint groundEntityNum)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1groundEntityNum: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	client->ps.groundEntityNum = groundEntityNum;
}



/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_viewAngles
 * Signature: (I)Lxreal/Angle3f;
 */
jobject JNICALL Java_xreal_server_game_Player_getPlayerState_1viewAngles(JNIEnv *env, jclass cls, jint clientNum)
{
	gclient_t	   *client;

	//Com_Printf("Java_xreal_server_game_Player_getPlayerState_1viewAngles(clientNum = %i)\n", clientNum);

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1viewAngles: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	return Java_NewAngle3f(client->ps.viewangles[PITCH], client->ps.viewangles[YAW], client->ps.viewangles[ROLL]);
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_viewAngles
 * Signature: (IFFF)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1viewAngles(JNIEnv *env, jclass cls, jint clientNum, jfloat pitch, jfloat yaw, jfloat roll)
{
	gclient_t	   *client;
	gentity_t      *ent;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1viewAngles: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];
	ent = &g_entities[clientNum];

	client->ps.viewangles[PITCH] = pitch;
	client->ps.viewangles[YAW] = yaw;
	client->ps.viewangles[ROLL] = roll;

	ent->s.angles[PITCH] = pitch;
	ent->s.angles[YAW] = yaw;
	ent->s.angles[ROLL] = roll;
}

/*
 * Class:     xreal_server_game_Player
 * Method:    getPlayerState_stat
 * Signature: (II)I
 */
jint JNICALL Java_xreal_server_game_Player_getPlayerState_1stat(JNIEnv *env, jclass cls, jint clientNum, jint stat)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1stat: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	if(stat < 0 || stat >= MAX_STATS)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_getPlayerState_1stat: bad stats index %i\n", stat);
	}

	return client->ps.stats[stat];
}

/*
 * Class:     xreal_server_game_Player
 * Method:    setPlayerState_stat
 * Signature: (III)V
 */
void JNICALL Java_xreal_server_game_Player_setPlayerState_1stat(JNIEnv *env, jclass cls, jint clientNum, jint stat, jint value)
{
	gclient_t	   *client;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1stat: bad index %i\n", clientNum);
	}
	client = &g_clients[clientNum];

	if(stat < 0 || stat >= MAX_STATS)
	{
		Com_Error(ERR_DROP, "Java_xreal_server_game_Player_setPlayerState_1stat: bad stats index %i\n", stat);
	}

	client->ps.stats[stat] = value;
}


static JNINativeMethod Player_methods[] = {
	{"sendServerCommand", "(ILjava/lang/String;)V", Java_xreal_server_game_Player_sendServerCommand},

	{"getUserInfo", "(I)Ljava/lang/String;", Java_xreal_server_game_Player_getUserInfo},
	{"setUserInfo", "(ILjava/lang/String;)V", Java_xreal_server_game_Player_setUserInfo},

	{"getUserCommand", "(I)Lxreal/UserCommand;", Java_xreal_server_game_Player_getUserCommand},

	{"getPlayerState_commandTime", "(I)I", Java_xreal_server_game_Player_getPlayerState_1commandTime},
	{"setPlayerState_commandTime", "(II)V", Java_xreal_server_game_Player_setPlayerState_1commandTime},

	{"getPlayerState_pm_type", "(I)I", Java_xreal_server_game_Player_getPlayerState_1pm_1type},
	{"setPlayerState_pm_type", "(II)V", Java_xreal_server_game_Player_setPlayerState_1pm_1type},

	{"getPlayerState_pm_flags", "(I)I", Java_xreal_server_game_Player_getPlayerState_1pm_1flags},
	{"setPlayerState_pm_flags", "(II)V", Java_xreal_server_game_Player_setPlayerState_1pm_1flags},

	{"getPlayerState_pm_time", "(I)I", Java_xreal_server_game_Player_getPlayerState_1pm_1time},
	{"setPlayerState_pm_time", "(II)V", Java_xreal_server_game_Player_setPlayerState_1pm_1time},

	{"getPlayerState_origin", "(I)Ljavax/vecmath/Vector3f;", Java_xreal_server_game_Player_getPlayerState_1origin},
	{"setPlayerState_origin", "(IFFF)V", Java_xreal_server_game_Player_setPlayerState_1origin},

	{"getPlayerState_velocity", "(I)Ljavax/vecmath/Vector3f;", Java_xreal_server_game_Player_getPlayerState_1velocity},
	{"setPlayerState_velocity", "(IFFF)V", Java_xreal_server_game_Player_setPlayerState_1velocity},

	{"getPlayerState_speed", "(I)I", Java_xreal_server_game_Player_getPlayerState_1speed},
	{"setPlayerState_speed", "(II)V", Java_xreal_server_game_Player_setPlayerState_1speed},

	{"getPlayerState_deltaPitch", "(I)S", Java_xreal_server_game_Player_getPlayerState_1deltaPitch},
	{"setPlayerState_deltaPitch", "(IS)V", Java_xreal_server_game_Player_setPlayerState_1deltaPitch},

	{"getPlayerState_deltaYaw", "(I)S", Java_xreal_server_game_Player_getPlayerState_1deltaYaw},
	{"setPlayerState_deltaYaw", "(IS)V", Java_xreal_server_game_Player_setPlayerState_1deltaYaw},

	{"getPlayerState_deltaRoll", "(I)S", Java_xreal_server_game_Player_getPlayerState_1deltaRoll},
	{"setPlayerState_deltaRoll", "(IS)V", Java_xreal_server_game_Player_setPlayerState_1deltaRoll},

	{"getPlayerState_groundEntityNum", "(I)I", Java_xreal_server_game_Player_getPlayerState_1groundEntityNum},
	{"setPlayerState_groundEntityNum", "(II)V", Java_xreal_server_game_Player_setPlayerState_1groundEntityNum},

	{"getPlayerState_viewAngles", "(I)Lxreal/Angle3f;", Java_xreal_server_game_Player_getPlayerState_1viewAngles},
	{"setPlayerState_viewAngles", "(IFFF)V", Java_xreal_server_game_Player_setPlayerState_1viewAngles},

	{"getPlayerState_stat", "(II)I", Java_xreal_server_game_Player_getPlayerState_1stat},
	{"setPlayerState_stat", "(III)V", Java_xreal_server_game_Player_setPlayerState_1stat}
};

char           *Java_G_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	gclient_t	   *client;

	jclass			class_Player;
	jmethodID		method_Player_ctor;
	jobject         object_Player;

	static char		string[MAX_STRING_CHARS];
	jobject 		result;
	jthrowable      exception;

	extern jmethodID method_Throwable_getMessage;

	if(!object_Game)
		return "no Game class loaded yet!!!";

	Com_Printf("Java_G_ClientBegin(%i)\n", clientNum);

	client = &g_clients[clientNum];

	// load the class Game
	class_Player = (*javaEnv)->FindClass(javaEnv, "xreal/server/game/Player");
	if(CheckException() || !class_Player)
	{
		Com_Error(ERR_DROP, "Couldn't find class xreal.server.game.Player");
	}

	// check class Game against interface GameListener
	if(!((*javaEnv)->IsAssignableFrom(javaEnv, class_Player, interface_ClientListener)))
	{
		Com_Error(ERR_DROP, "The current Player class doesn't implement xreal.server.game.ClientListener");
	}

	// register the native methods
	(*javaEnv)->RegisterNatives(javaEnv, class_Player, Player_methods, sizeof(Player_methods) / sizeof(Player_methods[0]));
	if(CheckException())
	{
		Com_Error(ERR_DROP, "Couldn't register native methods for xreal.server.game.Player");
	}

	// load constructor
	method_Player_ctor = (*javaEnv)->GetMethodID(javaEnv, class_Player, "<init>", "(IZZ)V");

	client->method_Player_clientBegin = (*javaEnv)->GetMethodID(javaEnv, class_Player, "clientBegin", "()V");
	client->method_Player_clientUserInfoChanged = (*javaEnv)->GetMethodID(javaEnv, class_Player, "clientUserInfoChanged", "(Ljava/lang/String;)V");
	client->method_Player_clientDisconnect = (*javaEnv)->GetMethodID(javaEnv, class_Player, "clientDisconnect", "()V");
	client->method_Player_clientCommand = (*javaEnv)->GetMethodID(javaEnv, class_Player, "clientCommand", "()V");
	client->method_Player_clientThink = (*javaEnv)->GetMethodID(javaEnv, class_Player, "clientThink", "(Lxreal/UserCommand;)V");

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Problem getting handle for one or more of the Player methods\n");
	}

	// create new player object
	client->object_Player = object_Player = (*javaEnv)->NewObject(javaEnv, class_Player, method_Player_ctor, clientNum, firstTime, isBot);
	exception = (*javaEnv)->ExceptionOccurred(javaEnv);
	if(exception)
	{
		//Com_Error(ERR_DROP, "Couldn't create instance of Player object");

		result = (*javaEnv)->CallObjectMethod(javaEnv, exception, method_Throwable_getMessage);

		ConvertJavaString(string, result, sizeof(string));

		(*javaEnv)->ExceptionClear(javaEnv);

		//Com_Printf("Java_G_ClientConnect '%s'\n", string);

		return string;
	}

	return NULL;
}

void Java_G_ClientBegin(int clientNum)
{
	gclient_t	   *client;

	if(!object_Game)
		return;

	Com_Printf("Java_G_ClientBegin(%i)\n", clientNum);

	client = &g_clients[clientNum];

	(*javaEnv)->CallVoidMethod(javaEnv, client->object_Player, client->method_Player_clientBegin);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_ClientBegin(clientNum = %i)", clientNum);
	}
}

void Java_G_ClientUserInfoChanged(int clientNum)
{
	gclient_t	   *client;
	jstring			userinfo;

	if(!object_Game)
		return;

	//Com_Printf("Java_G_ClientUserInfoChanged(%i)\n", clientNum);

	client = &g_clients[clientNum];
	userinfo = (*javaEnv)->NewStringUTF(javaEnv, svs.clients[clientNum].userinfo);

	(*javaEnv)->CallVoidMethod(javaEnv, client->object_Player, client->method_Player_clientUserInfoChanged, userinfo);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_ClientUserInfoChanged(clientNum = %i)", clientNum);
	}
}

void Java_G_ClientDisconnect(int clientNum)
{
	gclient_t	   *client;

	if(!object_Game)
		return;

	Com_Printf("Java_G_ClientDisconnect(%i)\n", clientNum);

	client = &g_clients[clientNum];

	(*javaEnv)->CallVoidMethod(javaEnv, client->object_Player, client->method_Player_clientDisconnect);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_ClientDisconnect(clientNum = %i)", clientNum);
	}
}

void Java_G_ClientCommand(int clientNum)
{
	gclient_t	   *client;

	if(!object_Game)
		return;

	//Com_Printf("Java_G_ClientCommand(%i)\n", clientNum);

	client = &g_clients[clientNum];

	(*javaEnv)->CallVoidMethod(javaEnv, client->object_Player, client->method_Player_clientCommand);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_ClientCommand(clientNum = %i)", clientNum);
	}
}

void Java_G_ClientThink(int clientNum)
{
	gclient_t	   *client;
	jobject			ucmd;

	if(!object_Game)
		return;

	//Com_Printf("Java_G_ClientThink(%i)\n", clientNum);

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "Java_G_ClientThink: bad clientNum: %i", clientNum);
	}

	client = &g_clients[clientNum];
	ucmd = Java_NewUserCommand(&svs.clients[clientNum].lastUsercmd);

	(*javaEnv)->CallVoidMethod(javaEnv, client->object_Player, client->method_Player_clientThink, ucmd);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_ClientThink(clientNum = %i)", clientNum);
	}
}

void Java_G_RunFrame(int time)
{
	if(!object_Game)
		return;

	g_levelTime = time;

	(*javaEnv)->CallVoidMethod(javaEnv, object_Game, method_Game_runFrame, time);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_RunFrame(time = %i)", time);
	}
}

void Java_G_RunAIFrame(int time)
{
	if(!object_Game)
		return;

	(*javaEnv)->CallVoidMethod(javaEnv, object_Game, method_Game_runAIFrame, time);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_RunAIFrame(time = %i)", time);
	}
}

qboolean Java_G_ConsoleCommand(void)
{
	jboolean result;

	if(!object_Game)
		return qfalse;

	result = (*javaEnv)->CallBooleanMethod(javaEnv, object_Game, method_Game_consoleCommand);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_G_ConsoleCommand()");
	}

	return result;
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM(qboolean restart)
{
	int             i;

	Com_DPrintf("SV_InitGameVM(restart = %i)\n", restart);

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=522
	//   now done before GAME_INIT call
	for(i = 0; i < sv_maxclients->integer; i++)
	{
		svs.clients[i].gentity = NULL;
	}

	// initialize all entities for this game
	memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));

	// initialize all clients for this game
//	level.maxclients = g_maxclients.integer;
	memset(g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]));

	// set client fields on player ents
	for(i = 0; i < sv_maxclients->integer; i++)
	{
		gentity_t *ent;
		gclient_t *client;

		ent = &g_entities[i];
		client = ent->client = &g_clients[i];

		ent->s.pos.trType = TR_INTERPOLATE;
		ent->s.apos.trType = TR_INTERPOLATE;

		ent->s.number = i;
		ent->s.clientNum = i;
		client->ps.clientNum = i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	sv.numEntities = MAX_CLIENTS;

	// use the current msec count for a random seed
	// init for this gamestate
	Java_G_GameInit(sv.time, Com_Milliseconds(), restart);
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs(void)
{
	Com_DPrintf("SV_RestartGameProgs()\n");

	if(!javaEnv)
	{
		return;
	}

	Java_G_ShutdownGame(qtrue);
	SV_InitGameVM(qtrue);
}




/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs(void)
{
	Com_DPrintf("SV_InitGameProgs()\n");

	Server_javaRegister();
	Game_javaRegister();
	GameEntity_javaRegister();
	ClientListener_javaRegister();

	SV_InitGameVM(qfalse);
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand(void)
{
	if(sv.state != SS_GAME)
	{
		return qfalse;
	}

	return Java_G_ConsoleCommand();
}

#endif							//defined(USE_JAVA
