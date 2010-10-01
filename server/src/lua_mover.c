/*
===========================================================================
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2009 Andrew Browne <dersaidin@gmail.com>

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
// lua_mover.c -- mover library for Lua

#include "g_lua.h"

#if(defined(G_LUA))

// mover.Halt(entnum)
static int mover_Halt(lua_State * L)
{
	gentity_t      *ent;
	int             entnum;

	entnum = luaL_checkint(L, 1);
	DEBUG_LUA("mover_Halt: start: ent=%d", entnum);
	ent = &g_entities[entnum];
	if(ent)
	{
		// Update current position
		BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->r.currentOrigin);
		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
		// Halt
		ent->s.pos.trType = TR_STATIONARY;
		ent->s.pos.trTime = level.time;
		ent->nextthink = 0;
		ent->think = NULL;
		ent->nextTrain = NULL;
		trap_LinkEntity(ent);
		DEBUG_LUA("mover_Halt: return: halted ent");
	}
	return 0;
}

// mover.HaltAngles(entnum)
static int mover_HaltAngles(lua_State * L)
{
	gentity_t      *ent;
	int             entnum;

	entnum = luaL_checkint(L, 1);
	DEBUG_LUA("mover_HaltAngles: start: ent=%d", entnum);
	ent = &g_entities[entnum];
	if(ent)
	{
		// Update current position
		BG_EvaluateTrajectory(&ent->s.apos, level.time, ent->s.apos.trBase);
		// Halt
		ent->s.apos.trType = TR_STATIONARY;
		ent->s.apos.trTime = level.time;
		trap_LinkEntity(ent);
		DEBUG_LUA("mover_HaltAngles: return: halted ent");
	}
	return 0;
}

// mover.AsTrain(entnum, targetnum, speed)
static int mover_AsTrain(lua_State * L)
{
	gentity_t      *ent;
	gentity_t      *targ;
	vec3_t          move;
	float           length;

	// Load parameters
	int             entnum = luaL_checkint(L, 1);
	int             targnum = luaL_checkint(L, 2);
	float           speed = (float)luaL_checknumber(L, 3);

	DEBUG_LUA("mover_AsTrain: start: ent=%d target=%d speed=%f", entnum, targnum, speed);
	ent = &g_entities[entnum];
	targ = &g_entities[targnum];

	// Abort if no target
	if(!ent || !targ)
	{
		DEBUG_LUA("mover_AsTrain: return: ent or target missing");
		return 0;
	}
	if(speed < 1)
	{
		DEBUG_LUA("mover_AsTrain: moving: speed less than 1 fixed");
		speed = 1;
	}

	if(ent->nextTrain)
	{
		DEBUG_LUA("mover_AsTrain: pathing: NextTrain=%d ", ent->nextTrain->s.number);
	}

	// Initialize ent for movement
	ent->speed = speed;
	ent->nextTrain = targ;
	ent->reached = Reached_Train;

	// Link path of targets
	SetupTrainPath(ent, qtrue);

	// Update the current starting location
	BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.origin);

	VectorCopy(ent->s.origin, ent->pos1);
	VectorCopy(ent->nextTrain->s.origin, ent->pos2);

	// calculate duration
	VectorSubtract(ent->pos2, ent->pos1, move);
	length = VectorLength(move);

	if(length <= 0.05)
	{
		G_SetOrigin(ent, ent->pos2);
		DEBUG_LUA("mover_AsTrain: return: snapped to target, length too small length=%f", length);
		return 0;
	}

	ent->s.pos.trDuration = length * 1000 / speed;

	// looping sound
	ent->s.loopSound = ent->nextTrain->soundLoop;

	// start it going
	SetMoverState(ent, MOVER_1TO2, level.time);

	DEBUG_LUA("mover_AsTrain: return: moving to target, length=%f duration=%d", length, ent->s.pos.trDuration);
	return 0;
}

// mover.SetAngles(entnum, x, y, z)
// mover.SetAngles(entnum, vector)
static int mover_SetAngles(lua_State * L)
{
	vec3_t          newAngles;
	gentity_t      *ent;
	int             entnum;
	vec_t          *target;

	entnum = luaL_checkint(L, 1);

	if(lua_isvector(L, 2))
	{
		target = lua_getvector(L, 2);
		VectorCopy(target, newAngles);
	}
	else
	{
		newAngles[0] = luaL_checkint(L, 2);
		newAngles[1] = luaL_checkint(L, 3);
		newAngles[2] = luaL_checkint(L, 4);
	}
	DEBUG_LUA("mover_SetAngles: start: ent=%d angles=%s", entnum, vtos(newAngles));
	ent = &g_entities[entnum];
	if(ent)
	{
		VectorCopy(newAngles, ent->s.apos.trBase);
		trap_LinkEntity(ent);
		DEBUG_LUA("mover_SetAngles: return: moved");
	}
	return 0;
}

// mover.SetPosition(entnum, x, y, z)
// mover.SetPosition(entnum, vector)
static int mover_SetPosition(lua_State * L)
{
	vec3_t          newOrigin;
	gentity_t      *ent;
	int             entnum;
	vec_t          *target;

	entnum = luaL_checkint(L, 1);

	if(lua_isvector(L, 2))
	{
		target = lua_getvector(L, 2);
		VectorCopy(target, newOrigin);
	}
	else
	{
		newOrigin[0] = luaL_checkint(L, 2);
		newOrigin[1] = luaL_checkint(L, 3);
		newOrigin[2] = luaL_checkint(L, 4);
	}
	DEBUG_LUA("mover_SetPosition: start: ent=%d pos=%s", entnum, vtos(newOrigin));
	ent = &g_entities[entnum];
	if(ent)
	{
		G_SetOrigin(ent, newOrigin);
		trap_LinkEntity(ent);
		DEBUG_LUA("mover_SetPosition: return: moved");
	}
	return 0;
}

static void SetTrajectoryLinear(trajectory_t * tr, const float speed, const vec3_t endPosition)
{
	vec3_t          delta;
	float           length;

	// Create angular velocity
	VectorSubtract(endPosition, tr->trBase, delta);
	length = VectorLength(delta);
	VectorNormalize(delta);
	tr->trDuration = length * 1000 / speed;
	tr->trTime = level.time;
	VectorScale(delta, speed, tr->trDelta);
	tr->trType = TR_LINEAR_STOP;
}

// mover.ToAngles(entnum, x, y, z, speed)
// mover.ToAngles(entnum, vector, speed)
static int mover_ToAngles(lua_State * L)
{
	vec3_t          newAngles;
	gentity_t      *ent;
	int             entnum;
	float           speed;
	vec_t          *target;

	entnum = luaL_checkint(L, 1);

	if(lua_isvector(L, 2))
	{
		target = lua_getvector(L, 2);
		VectorCopy(target, newAngles);
	}
	else
	{
		newAngles[0] = luaL_checkint(L, 2);
		newAngles[1] = luaL_checkint(L, 3);
		newAngles[2] = luaL_checkint(L, 4);
	}

	speed = (float)luaL_checknumber(L, 5);
	DEBUG_LUA("mover_ToAngles: start: ent=%d angles=%s speed=%f", entnum, vtos(newAngles), speed);
	ent = &g_entities[entnum];
	if(ent)
	{
		// Update to current angles
		BG_EvaluateTrajectory(&ent->s.apos, level.time, ent->s.apos.trBase);
		// Linear Trajectory
		SetTrajectoryLinear(&ent->s.apos, speed, newAngles);
		trap_LinkEntity(ent);
		DEBUG_LUA("mover_ToAngles: return: moving");
	}
	return 0;
}


// mover.ToPosition(entnum, x, y, z, speed)
// mover.ToPosition(entnum, vector, speed)
static int mover_ToPosition(lua_State * L)
{
	vec3_t          newOrigin;
	gentity_t      *ent;
	int             entnum;
	float           speed;
	vec_t          *target;

	entnum = luaL_checkint(L, 1);

	if(lua_isvector(L, 2))
	{
		target = lua_getvector(L, 2);
		VectorCopy(target, newOrigin);
	}
	else
	{
		newOrigin[0] = luaL_checkint(L, 2);
		newOrigin[1] = luaL_checkint(L, 3);
		newOrigin[2] = luaL_checkint(L, 4);
	}
	speed = (float)luaL_checknumber(L, 5);
	DEBUG_LUA("mover_ToPosition: start: ent=%d pos=%s speed=%f", entnum, vtos(newOrigin), speed);
	ent = &g_entities[entnum];
	if(ent)
	{
		// Update to current origin
		BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->s.pos.trBase);
		// Linear Trajectory
		SetTrajectoryLinear(&ent->s.pos, speed, newOrigin);
		//maintain valid mover state
		ent->moverState = MOVER_MISC;
		trap_LinkEntity(ent);
		DEBUG_LUA("mover_ToPosition: return: moving");
	}
	return 0;
}

static const luaL_reg moverlib[] = {
	{"Halt", mover_Halt},
	{"HaltAngles", mover_HaltAngles},
	{"AsTrain", mover_AsTrain},
	{"SetPosition", mover_SetPosition},
	{"ToPosition", mover_ToPosition},
	{"SetAngles", mover_SetAngles},
	{"ToAngles", mover_ToAngles},
	{NULL, NULL}
};

int luaopen_mover(lua_State * L)
{
	luaL_register(L, "mover", moverlib);

	return 1;
}

#endif
