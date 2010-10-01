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
//
#include "g_local.h"



/*
===============================================================================

EXPLOSIVE

===============================================================================
*/

void func_explosive_die(gentity_t * self, gentity_t * inflictor, gentity_t * attacker, int damage, int mod)
{
	//Explosion event
	G_AddEvent(self, EV_EXPLODE, self->materialType);
	self->freeAfterEvent = qtrue;
	self->s.solid = 0;

#ifdef G_LUA
	// Lua API callbacks
	if(self->luaTrigger)
	{
		if(attacker)
		{
			G_LuaHook_EntityTrigger(self->luaTrigger, self->s.number, attacker->s.number);
		}
		else
		{
			G_LuaHook_EntityTrigger(self->luaTrigger, self->s.number, ENTITYNUM_WORLD);
		}
	}
#endif

	//If it takes more damage during the event, it would die again.
	self->takedamage = qfalse;
}


/*QUAKED func_explosive (0 .5 .8) ?
A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"damage"	explosion damage to do to nearby entities when it explodes, also dmg
"health"	damage this entity will take before exploding
"mass"		number & size of chunks
*/
void SP_func_explosive(gentity_t * ent)
{
	char           *type;

	trap_SetBrushModel(ent, ent->model);

	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);

	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	VectorCopy(ent->s.angles, ent->r.currentAngles);

	G_SpawnInt("dmg", "0", &ent->damage);
	G_SpawnInt("damage", "0", &ent->damage);
	G_SpawnInt("health", "100", &ent->health);
	G_SpawnInt("mass1", "10", &ent->s.weapon);
	G_SpawnInt("mass2", "6", &ent->s.legsAnim);
	G_SpawnInt("mass3", "3", &ent->s.torsoAnim);

	ent->s.eType = ET_EXPLOSIVE;

	if(ent->health)
	{
		ent->takedamage = qtrue;
	}

	if(G_SpawnString("type", "none", &type))
	{
		if(!Q_stricmp(type, "wood"))
			ent->materialType = ENTMAT_WOOD;
		else if(!Q_stricmp(type, "glass"))
			ent->materialType = ENTMAT_GLASS;
		else if(!Q_stricmp(type, "metal"))
			ent->materialType = ENTMAT_METAL;
		else if(!Q_stricmp(type, "gibs"))
			ent->materialType = ENTMAT_GIBS;
		else if(!Q_stricmp(type, "brick"))
			ent->materialType = ENTMAT_BRICK;
		else if(!Q_stricmp(type, "rock"))
			ent->materialType = ENTMAT_STONE;
		else if(!Q_stricmp(type, "tiles"))
			ent->materialType = ENTMAT_TILES;
		else if(!Q_stricmp(type, "plaster"))
			ent->materialType = ENTMAT_PLASTER;
		else if(!Q_stricmp(type, "fibers"))
			ent->materialType = ENTMAT_FIBERS;
		else if(!Q_stricmp(type, "sprite"))
			ent->materialType = ENTMAT_SPRITE;
		else if(!Q_stricmp(type, "smoke"))
			ent->materialType = ENTMAT_SMOKE;
		else if(!Q_stricmp(type, "gas"))
			ent->materialType = ENTMAT_GAS;
		else if(!Q_stricmp(type, "fire"))
			ent->materialType = ENTMAT_FIRE;
		else
			ent->materialType = ENTMAT_NONE;
	}
	else
	{
		ent->materialType = ENTMAT_NONE;
	}

	ent->die = func_explosive_die;

	//Put parameters in networked variables
	ent->s.generic1 = ent->materialType;	//Type

	trap_LinkEntity(ent);
}


/*
===============================================================================

FIRE

===============================================================================
*/

gentity_t      *CreateFire(vec3_t org, int duration)
{
	gentity_t      *fire;

	fire = G_Spawn();
	fire->classname = "fire";
	if(duration > 0)
	{
		fire->nextthink = level.time + duration;
	}
	else
	{
		fire->nextthink = -1;
	}
	fire->think = G_FreeEntity;
	fire->s.eType = ET_FIRE;
	fire->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	fire->s.weapon = 20;		//particles
	//fire->r.ownerNum = ;
	//fire->parent = self;
	//fire->damage = 10;
	fire->splashDamage = 5;
	fire->splashRadius = 20;
	fire->methodOfDeath = MOD_BURN;
	fire->splashMethodOfDeath = MOD_BURN;
	fire->clipmask = MASK_SHOT;
	fire->target_ent = NULL;

	//fire->s.pos.trTime = level.time;
	VectorCopy(org, fire->s.pos.trBase);
	VectorCopy(org, fire->r.currentOrigin);

	return fire;
}

/*QUAKED misc_fire (0 .5 .8) ?
Fire entity
"damage"	damage per server frame 
"light"		intensity of light this fire creates
"size"		size of fire
"count"		number of extra particles
*/
void SP_misc_fire(gentity_t * ent)
{
	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);

	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	VectorCopy(ent->s.angles, ent->r.currentAngles);

	G_SpawnInt("size", "50", &ent->s.generic1);
	G_SpawnInt("light", "150", &ent->s.constantLight);
	G_SpawnInt("count", "1", &ent->s.weapon);

	ent->s.eType = ET_FIRE;

	trap_LinkEntity(ent);
}
