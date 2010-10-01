/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// sv_bot.c

#include <hat/engine/server.h>

/*
==================
SV_BotClientCommand
==================
*/
void BotClientCommand(int client, char *command)
{
	SV_ExecuteClientCommand(&svs.clients[client], command, qtrue);
}

/*
==================
SV_BotAllocateClient
==================
*/
int SV_BotAllocateClient(void)
{
	int             i;
	client_t       *cl;

	// find a client slot
	for(i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
	{
		if(cl->state == CS_FREE)
		{
			break;
		}
	}

	if(i == sv_maxclients->integer)
	{
		return -1;
	}

	cl->gentity = SV_GentityNum(i);
	cl->gentity->s.number = i;
	cl->state = CS_ACTIVE;
	cl->lastPacketTime = svs.time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	return i;
}

/*
==================
SV_BotFreeClient
==================
*/
void SV_BotFreeClient(int clientNum)
{
	client_t       *cl;

	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_BotFreeClient: bad clientNum: %i", clientNum);
	}
	cl = &svs.clients[clientNum];
	cl->state = CS_FREE;
	cl->name[0] = 0;
	if(cl->gentity)
	{
		cl->gentity->r.svFlags &= ~SVF_BOT;
	}
}

/*
==================
SV_BotFrame
==================
*/
void SV_BotFrame(int time)
{
#if defined(USE_JAVA)
	Java_G_RunAIFrame(time);
#else
	//NOTE: maybe the game is already shutdown
	if(!gvm)
		return;

	VM_Call(gvm, BOTAI_START_FRAME, time);
#endif
}



//
//  * * * BOT AI CODE IS BELOW THIS POINT * * *
//

/*
==================
SV_BotGetConsoleMessage
==================
*/
int SV_BotGetConsoleMessage(int client, char *buf, int size)
{
	client_t       *cl;
	int             index;

	cl = &svs.clients[client];
	cl->lastPacketTime = svs.time;

	if(cl->reliableAcknowledge == cl->reliableSequence)
	{
		return qfalse;
	}

	cl->reliableAcknowledge++;
	index = cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1);

	if(!cl->reliableCommands[index][0])
	{
		return qfalse;
	}

	Q_strncpyz(buf, cl->reliableCommands[index], size);
	return qtrue;
}

#if 0
/*
==================
EntityInPVS
==================
*/
int EntityInPVS(int client, int entityNum)
{
	client_t       *cl;
	clientSnapshot_t *frame;
	int             i;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	for(i = 0; i < frame->num_entities; i++)
	{
		if(svs.snapshotEntities[(frame->first_entity + i) % svs.numSnapshotEntities].number == entityNum)
		{
			return qtrue;
		}
	}
	return qfalse;
}
#endif

/*
==================
SV_BotGetSnapshotEntity
==================
*/
int SV_BotGetSnapshotEntity(int client, int sequence)
{
	client_t       *cl;
	clientSnapshot_t *frame;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	if(sequence < 0 || sequence >= frame->num_entities)
	{
		return -1;
	}
	return svs.snapshotEntities[(frame->first_entity + sequence) % svs.numSnapshotEntities].number;
}

/*
==================
SV_BotClientCommand
==================
*/
void SV_BotClientCommand(int client, char *command)
{
	SV_ExecuteClientCommand(&svs.clients[client], command, qtrue);
}
