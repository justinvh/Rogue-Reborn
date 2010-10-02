/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
#include <hat/server/g_local.h>

#define	MISSILE_PRESTEP_TIME	50

/*
================
G_BounceMissile

================
*/
void G_BounceMissile(gentity_t * ent, trace_t * trace)
{
	vec3_t          velocity;
	float           dot;
	int             hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + (level.time - level.previousTime) * trace->fraction;
	BG_EvaluateTrajectoryDelta(&ent->s.pos, hitTime, velocity);
	dot = DotProduct(velocity, trace->plane.normal);
	VectorMA(velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta);

	if(ent->s.eFlags & EF_BOUNCE_HALF)
	{
		VectorScale(ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta);
		// check for stop
		if(trace->plane.normal[2] > 0.2 && VectorLength(ent->s.pos.trDelta) < 40)
		{
			G_SetOrigin(ent, trace->endpos);
			return;
		}
	}

	VectorAdd(ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile(gentity_t * ent)
{
	vec3_t          dir;
	vec3_t          origin;

	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	SnapVector(origin);
	G_SetOrigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	// Tr3B: don't change the entity type because it is required by the EV_PROJECTILE_* events
	// the ent->freeAfterEvent = qtrue; will do the same effect
	//ent->s.eType = ET_GENERAL;

	G_AddEvent(ent, EV_PROJECTILE_MISS, DirToByte(dir));

	ent->s.modelindex = 0;
	ent->freeAfterEvent = qtrue;

	// splash damage
	if(ent->splashDamage)
	{
		if(G_RadiusDamage(ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath))
		{
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}
	}

	trap_LinkEntity(ent);
}

void G_ExplodeIntoNails(gentity_t * ent)
{
	float           angle, angle2;
	vec3_t          forward, forward2, up;
	vec3_t          nailForward, nailRight, nailUp;
	vec3_t          dir;
	vec3_t          origin;

	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);
	SnapVector(origin);
	G_SetOrigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	// Tr3B: don't change the entity type because it is required by the EV_PROJECTILE_* events
	// the ent->freeAfterEvent = qtrue; will do the same effect
	//ent->s.eType = ET_GENERAL;

	G_AddEvent(ent, EV_PROJECTILE_MISS, DirToByte(dir));

	ent->freeAfterEvent = qtrue;

	// splash damage
	if(ent->splashDamage)
	{
		if(G_RadiusDamage(ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath))
		{
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}
	}

	// create orthogonal vector to main direction
	PerpendicularVector(dir, forward);
	//VectorScale(forward, 40, forward);

	origin[2] += 10;

	// spread nails in all directions
	for(angle = 0; angle < 360; angle += 60.0f)
	{
		RotatePointAroundVector(forward2, dir, forward, angle);

		for(angle2 = 0; angle2 < 360; angle2 += 60.0f)
		{
			RotatePointAroundVector(up, forward, dir, angle2);

			VectorAdd(forward2, up, nailForward);
			VectorNormalize(nailForward);

			PerpendicularVector(nailRight, nailForward);
			CrossProduct(nailForward, nailRight, nailUp);
#if 1
			fire_gravnail(ent, origin, nailForward, nailRight, nailUp);
#else
			VectorAdd(forward2, dir, forward2);
			fire_clustergrenade(ent, origin, forward2);
#endif
		}
	}

	trap_LinkEntity(ent);
}



#ifdef MISSIONPACK
/*
================
ProximityMine_Explode
================
*/
static void ProximityMine_Explode(gentity_t * mine)
{
	G_ExplodeMissile(mine);
	// if the prox mine has a trigger free it
	if(mine->activator)
	{
		G_FreeEntity(mine->activator);
		mine->activator = NULL;
	}
}

/*
================
ProximityMine_Die
================
*/
static void ProximityMine_Die(gentity_t * ent, gentity_t * inflictor, gentity_t * attacker, int damage, int mod)
{
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

/*
================
ProximityMine_Trigger
================
*/
void ProximityMine_Trigger(gentity_t * trigger, gentity_t * other, trace_t * trace)
{
	vec3_t          v;
	gentity_t      *mine;

	if(!other->client)
	{
		return;
	}

	// trigger is a cube, do a distance test now to act as if it's a sphere
	VectorSubtract(trigger->s.pos.trBase, other->s.pos.trBase, v);
	if(VectorLength(v) > trigger->parent->splashRadius)
	{
		return;
	}


	if(g_gametype.integer >= GT_TEAM)
	{
		// don't trigger same team mines
		if(trigger->parent->s.generic1 == other->client->sess.sessionTeam)
		{
			return;
		}
	}

	// ok, now check for ability to damage so we don't get triggered thru walls, closed doors, etc...
	if(!CanDamage(other, trigger->s.pos.trBase))
	{
		return;
	}

	// trigger the mine!
	mine = trigger->parent;
	mine->s.loopSound = 0;
	G_AddEvent(mine, EV_PROXIMITY_MINE_TRIGGER, 0);
	mine->nextthink = level.time + 500;

	G_FreeEntity(trigger);
}

/*
================
ProximityMine_Activate
================
*/
static void ProximityMine_Activate(gentity_t * ent)
{
	gentity_t      *trigger;
	float           r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + g_proxMineTimeout.integer;

	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;

	ent->s.loopSound = G_SoundIndex("sound/weapons/proxmine/wstbtick.wav");

	// build the proximity trigger
	trigger = G_Spawn();

	trigger->classname = "proxmine_trigger";

	r = ent->splashRadius;
	VectorSet(trigger->r.mins, -r, -r, -r);
	VectorSet(trigger->r.maxs, r, r, r);

	G_SetOrigin(trigger, ent->s.pos.trBase);

	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;

	trap_LinkEntity(trigger);

	// set pointer to trigger so the entity can be freed when the mine explodes
	ent->activator = trigger;
}

/*
================
ProximityMine_ExplodeOnPlayer
================
*/
static void ProximityMine_ExplodeOnPlayer(gentity_t * mine)
{
	gentity_t      *player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	if(player->client->invulnerabilityTime > level.time)
	{
		G_Damage(player, mine->parent, mine->parent, vec3_origin, mine->s.origin, 1000, DAMAGE_NO_KNOCKBACK, MOD_JUICED);
		player->client->invulnerabilityTime = 0;
		G_TempEntity(player->client->ps.origin, EV_JUICED);
	}
	else
	{
		G_SetOrigin(mine, player->s.pos.trBase);
		// make sure the explosion gets to the client
		mine->r.svFlags &= ~SVF_NOCLIENT;
		mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
		G_ExplodeMissile(mine);
	}
}

/*
================
ProximityMine_Player
================
*/
static void ProximityMine_Player(gentity_t * mine, gentity_t * player)
{
	if(mine->s.eFlags & EF_NODRAW)
	{
		return;
	}

	G_AddEvent(mine, EV_PROXIMITY_MINE_STICK, 0);

	if(player->s.eFlags & EF_TICKING)
	{
		player->activator->splashDamage += mine->splashDamage;
		player->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear(mine->s.pos.trDelta);

	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
	if(player->client->invulnerabilityTime > level.time)
	{
		mine->nextthink = level.time + 2 * 1000;
	}
	else
	{
		mine->nextthink = level.time + 10 * 1000;
	}
}
#endif

/*
================
G_MissileImpact
================
*/
void G_MissileImpact(gentity_t * ent, trace_t * trace)
{
	gentity_t      *other;
	qboolean        hitClient = qfalse;

#ifdef MISSIONPACK
	vec3_t          forward, impactpoint, bouncedir;
	int             eFlags;
#endif
	other = &g_entities[trace->entityNum];

	// check for bounce
	if(!other->takedamage && (ent->s.eFlags & (EF_BOUNCE | EF_BOUNCE_HALF)))
	{
		G_BounceMissile(ent, trace);
		G_AddEvent(ent, EV_GRENADE_BOUNCE, 0);
		return;
	}

	if(ent->s.weapon == WP_FLAK_CANNON && ent->s.eType == ET_PROJECTILE2)
	{
		G_ExplodeIntoNails(ent);
		return;
	}

#ifdef MISSIONPACK
	if(other->takedamage)
	{
		if(ent->s.weapon != WP_PROX_LAUNCHER)
		{
			if(other->client && other->client->invulnerabilityTime > level.time)
			{
				//
				VectorCopy(ent->s.pos.trDelta, forward);
				VectorNormalize(forward);

				if(G_InvulnerabilityEffect(other, forward, ent->s.pos.trBase, impactpoint, bouncedir))
				{
					VectorCopy(bouncedir, trace->plane.normal);
					eFlags = ent->s.eFlags & EF_BOUNCE_HALF;
					ent->s.eFlags &= ~EF_BOUNCE_HALF;
					G_BounceMissile(ent, trace);
					ent->s.eFlags |= eFlags;
				}
				ent->target_ent = other;
				return;
			}
		}
	}
#endif
	// impact damage
	if(other->takedamage)
	{
		// FIXME: wrong damage direction?
		if(ent->damage)
		{
			vec3_t          velocity;

			if(LogAccuracyHit(other, &g_entities[ent->r.ownerNum]))
			{
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
			if(VectorLength(velocity) == 0)
			{
				velocity[2] = 1;	// stepped on a grenade
			}

			G_Damage(other, ent, &g_entities[ent->r.ownerNum], velocity, ent->s.origin, ent->damage, 0, ent->methodOfDeath);
		}
	}

#ifdef MISSIONPACK
	if(ent->s.weapon == WP_PROX_LAUNCHER)
	{
		if(ent->s.pos.trType != TR_GRAVITY)
		{
			return;
		}

		// if it's a player, stick it on to them (flag them and remove this entity)
		if(other->s.eType == ET_PLAYER && other->health > 0)
		{
			ProximityMine_Player(ent, other);
			return;
		}

		SnapVectorTowards(trace->endpos, ent->s.pos.trBase);
		G_SetOrigin(ent, trace->endpos);
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear(ent->s.pos.trDelta);

		G_AddEvent(ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags);

		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;

		VectorToAngles(trace->plane.normal, ent->s.angles);
		ent->s.angles[0] += 90;

		// link the prox mine to the other entity
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		VectorCopy(trace->plane.normal, ent->movedir);
		VectorSet(ent->r.mins, -4, -4, -4);
		VectorSet(ent->r.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}
#endif

	if(!strcmp(ent->classname, "hook"))
	{
		gentity_t      *nent;
		vec3_t          v;

		nent = G_Spawn();
		if(other->takedamage && other->client)
		{

			G_AddEvent(nent, EV_PROJECTILE_HIT, DirToByte(trace->plane.normal));
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			SnapVectorTowards(v, ent->s.pos.trBase);	// save net bandwidth
		}
		else
		{
			VectorCopy(trace->endpos, v);
			G_AddEvent(nent, EV_PROJECTILE_MISS, DirToByte(trace->plane.normal));
			ent->enemy = NULL;
		}

		SnapVectorTowards(v, ent->s.pos.trBase);	// save net bandwidth

		nent->freeAfterEvent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin(ent, v);
		G_SetOrigin(nent, v);

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy(ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity(ent);
		trap_LinkEntity(nent);

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if(other->takedamage && other->client)
	{
		G_AddEvent(ent, EV_PROJECTILE_HIT, DirToByte(trace->plane.normal));
		ent->s.otherEntityNum = other->s.number;
	}
	else if(trace->surfaceFlags & SURF_METALSTEPS)
	{
		G_AddEvent(ent, EV_PROJECTILE_MISS_METAL, DirToByte(trace->plane.normal));
	}
	else
	{
		G_AddEvent(ent, EV_PROJECTILE_MISS, DirToByte(trace->plane.normal));
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	SnapVectorTowards(trace->endpos, ent->s.pos.trBase);	// save net bandwidth

	G_SetOrigin(ent, trace->endpos);

	// splash damage (doesn't apply to person directly hit)
	if(ent->splashDamage)
	{
		if(G_RadiusDamage(trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, other, ent->splashMethodOfDeath))
		{
			if(!hitClient)
			{
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}

	trap_LinkEntity(ent);
}

/*
================
G_RunMissile
================
*/
void G_RunMissile(gentity_t * ent)
{
	vec3_t          origin;
	trace_t         tr;
	int             passent;

	// get current position
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

	// if this missile bounced off an invulnerability sphere
	if(ent->target_ent)
	{
		passent = ent->target_ent->s.number;
	}
#ifdef MISSIONPACK
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if(ent->s.weapon == WP_PROX_LAUNCHER && ent->count)
	{
		passent = ENTITYNUM_NONE;
	}
#endif
	else
	{
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}

	// trace a line from the previous position to the current position
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask);

	if(tr.startsolid || tr.allsolid)
	{
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask);
		tr.fraction = 0;
	}
	else
	{
		VectorCopy(tr.endpos, ent->r.currentOrigin);
	}

	trap_LinkEntity(ent);

	if(tr.fraction != 1)
	{
		// never explode or bounce on sky
		if(tr.surfaceFlags & SURF_NOIMPACT)
		{
			// If grapple, reset owner
			if(ent->parent && ent->parent->client && ent->parent->client->hook == ent)
			{
				ent->parent->client->hook = NULL;
			}
			G_FreeEntity(ent);
			return;
		}

		G_MissileImpact(ent, &tr);

		if(ent->s.eType != ET_PROJECTILE && ent->s.eType != ET_PROJECTILE2)
		{
			return;				// exploded
		}
	}
#ifdef MISSIONPACK
	// if the prox mine wasn't yet outside the player body
	if(ent->s.weapon == WP_PROX_LAUNCHER && !ent->count)
	{
		// check if the prox mine is outside the owner bbox
		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask);
		if(!tr.startsolid || tr.entityNum != ent->r.ownerNum)
		{
			ent->count = 1;
		}
	}
#endif

	// otty: added
	G_TouchTriggers(ent);

	// check think function after bouncing
	G_RunThink(ent);
}

/*
================
G_Missile_Die
================
*/
static void G_Missile_Die(gentity_t * ent, gentity_t * inflictor, gentity_t * attacker, int damage, int mod)
{
	ent->nextthink = level.time + 1;
	ent->think = G_ExplodeMissile;
}


//=============================================================================

/*
=================
fire_plasma

=================
*/
gentity_t      *fire_plasma(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *bolt;

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_PROJECTILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashDamage = 15;
	bolt->splashRadius = 20;
	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);
	VectorScale(dir, 2000, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================


/*
=================
fire_grenade
=================
*/
gentity_t      *fire_grenade(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *bolt;

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_PROJECTILE2;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_FLAK_CANNON;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trAcceleration = g_gravity.value;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);
	VectorScale(dir, 700, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}


/*
=================
fire_flakgrenade
Tr3B: throws a grenade that explodes by the first hit similar to UT99
=================
*/
gentity_t      *fire_flakgrenade(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *bolt;

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = G_ExplodeIntoNails;
	bolt->s.eType = ET_PROJECTILE2;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_FLAK_CANNON;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trAcceleration = g_gravity.value;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);
	VectorScale(dir, 700, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}


gentity_t      *fire_clustergrenade(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *bolt;

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_PROJECTILE2;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_FLAK_CANNON;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trAcceleration = g_gravity.value;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	VectorScale(dir, 800, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}


//=============================================================================


/*
=================
fire_bfg
=================
*/
gentity_t      *fire_bfg(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *bolt;

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_PROJECTILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);
	VectorScale(dir, 2000, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================


/*
=================
fire_rocket
=================
*/
gentity_t      *fire_rocket(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *bolt;
	vec3_t          mins = { -8, -8, -8 };
	vec3_t          maxs = { 8, 8, 8 };

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_PROJECTILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	// make the rocket shootable
	bolt->r.contents = CONTENTS_SHOOTABLE;
	VectorCopy(mins, bolt->r.mins);
	VectorCopy(maxs, bolt->r.maxs);
	bolt->takedamage = qtrue;
	bolt->health = 50;
	bolt->die = G_Missile_Die;

	if(g_rocketAcceleration.integer)
	{
		// use acceleration instead of linear velocity
		bolt->s.pos.trType = TR_ACCELERATION;
		bolt->s.pos.trAcceleration = g_rocketAcceleration.value;
		VectorScale(dir, g_rocketVelocity.value, bolt->s.pos.trDelta);
	}
	else
	{
		bolt->s.pos.trType = TR_LINEAR;
		VectorScale(dir, g_rocketVelocity.value, bolt->s.pos.trDelta);
	}

	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================

void G_HomingMissile(gentity_t * ent)
{
	gentity_t      *target = NULL;
	gentity_t      *blip = NULL;
	vec3_t          dir, blipdir;
	vec_t           angle;
	qboolean        chaff;

	//qboolean        ignorechaff = qfalse;
	const int       HOMING_THINK_TIME = 60;

	// explode after 15 seconds without a hit
	if(ent->spawnTime + 15000 <= level.time)
	{
		G_ExplodeMissile(ent);
		return;
	}

	/*
	if(ent->parent->health <= 0)
	{
		ent->nextthink = level.time + 15000;
		ent->think = G_ExplodeMissile;
		return;
	}
	*/

	/*
	if(ent->parent && ent->parent->client)
	{
		ignorechaff = (ent->parent->client->ps.powerups[PW_ACCURACY] > 0);
	}
	*/

	while((blip = G_FindRadius(blip, ent->r.currentOrigin, 2000)) != NULL)
	{
#if 0
		if(blip->s.weapon == WP_CHAFF)
		{
			if(ignorechaff)
			{
				continue;
			}

			chaff = qtrue;
		}
		else
#endif
		{
			chaff = qfalse;

			if(blip->client == NULL)
				continue;

			if(blip == ent->parent)
				continue;

			if(blip->health <= 0)
				continue;

			if(blip->client->sess.sessionTeam >= TEAM_SPECTATOR)
				continue;

			if((g_gametype.integer == GT_TEAM || g_gametype.integer == GT_CTF) && OnSameTeam(blip, ent->parent))
				continue;
		}

		if(!G_IsVisible(ent, blip->r.currentOrigin))
			continue;

		VectorSubtract(blip->r.currentOrigin, ent->r.currentOrigin, blipdir);

		if(chaff)
		{
			VectorScale(blipdir, 0.5, blipdir);
		}

		if((target == NULL) || (VectorLength(blipdir) < VectorLength(dir)))
		{
			if(chaff)
			{
				VectorScale(blipdir, 2, blipdir);
			}

			angle = AngleBetweenVectors(ent->r.currentAngles, blipdir);

			if(angle < 120.0f)
			{
				// We add it as our target
				target = blip;
				VectorCopy(blipdir, dir);
			}
		}
	}

	if(target == NULL)
	{
		ent->nextthink = level.time + HOMING_THINK_TIME;	// + 10000;
		ent->think = G_HomingMissile;
	}
	else
	{
		// for exact trajectory calculation, set current point to base.
		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);

		VectorNormalize(dir);
		// 0.5 is swing rate.
		VectorScale(dir, 0.5, dir);
		VectorAdd(dir, ent->r.currentAngles, dir);

		// turn nozzle to target angle
		VectorNormalize(dir);
		VectorCopy(dir, ent->r.currentAngles);

		// scale direction, put into trDelta
		if(g_rocketAcceleration.integer)
		{
			// use acceleration instead of linear velocity
			ent->s.pos.trType = TR_ACCELERATION;
			ent->s.pos.trAcceleration = g_rocketAcceleration.value;
			VectorScale(dir, g_rocketVelocity.value, ent->s.pos.trDelta);
		}
		else
		{
			ent->s.pos.trType = TR_LINEAR;
			VectorScale(dir, g_rocketVelocity.value * 0.25, ent->s.pos.trDelta);
		}

		ent->s.pos.trTime = level.time;

		SnapVector(ent->s.pos.trDelta);	// save net bandwidth
		ent->nextthink = level.time + HOMING_THINK_TIME;	// decrease this value also makes fast swing
		ent->think = G_HomingMissile;

		//G_Printf("targeting %s\n", target->classname);
	}
}

/*
=================
fire_homing
=================
*/
gentity_t      *fire_homing(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *bolt;
	vec3_t          mins = { -8, -8, -8 };
	vec3_t          maxs = { 8, 8, 8 };

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "rocket_homing";
	bolt->nextthink = level.time + 600;
	bolt->think = G_HomingMissile;
	bolt->s.eType = ET_PROJECTILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	// make the rocket shootable
	bolt->r.contents = CONTENTS_SHOOTABLE;
	VectorCopy(mins, bolt->r.mins);
	VectorCopy(maxs, bolt->r.maxs);
	bolt->takedamage = qtrue;
	bolt->health = 50;
	bolt->die = G_Missile_Die;

	if(g_rocketAcceleration.integer)
	{
		// use acceleration instead of linear velocity
		bolt->s.pos.trType = TR_ACCELERATION;
		bolt->s.pos.trAcceleration = g_rocketAcceleration.value;
		VectorScale(dir, g_rocketVelocity.value, bolt->s.pos.trDelta);
	}
	else
	{
		bolt->s.pos.trType = TR_LINEAR;
		VectorScale(dir, g_rocketVelocity.value * 0.25, bolt->s.pos.trDelta);
	}

	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================

/*
=================
fire_grapple
=================
*/
gentity_t      *fire_grapple(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *hook;

	VectorNormalize(dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_PROJECTILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GAUNTLET;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number;	// use to match beam in client
	VectorCopy(start, hook->s.pos.trBase);
	VectorScale(dir, 800, hook->s.pos.trDelta);
	SnapVector(hook->s.pos.trDelta);	// save net bandwidth
	VectorCopy(start, hook->r.currentOrigin);

	self->client->hook = hook;

	return hook;
}



/*
=================
fire_nail
=================
*/
#define NAILGUN_SPREAD	500

gentity_t      *fire_nail(gentity_t * self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up)
{
	gentity_t      *bolt;
	vec3_t          dir;
	vec3_t          end;
	float           r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 1500;
	bolt->think = G_FreeEntity;
	bolt->s.eType = ET_PROJECTILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_FLAK_CANNON;
//	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->methodOfDeath = MOD_NAIL;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	VectorCopy(start, bolt->s.pos.trBase);

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * NAILGUN_SPREAD * 16;
	r = cos(r) * crandom() * NAILGUN_SPREAD * 16;
	VectorMA(start, 8192 * 16, forward, end);
	VectorMA(end, r, right, end);
	VectorMA(end, u, up, end);
	VectorSubtract(end, start, dir);
	VectorNormalize(dir);

	scale = 555 + random() * 1800;
	VectorScale(dir, scale, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);

	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

gentity_t      *fire_gravnail(gentity_t * self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up)
{
	gentity_t      *bolt;
	vec3_t          dir;
	vec3_t          end;
	float           r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 500;
	bolt->think = G_FreeEntity;
	bolt->s.eType = ET_PROJECTILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_FLAK_CANNON;
//	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->methodOfDeath = MOD_NAIL;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

//	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trAcceleration = g_gravity.value;
	bolt->s.pos.trTime = level.time;
	VectorCopy(start, bolt->s.pos.trBase);

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * NAILGUN_SPREAD * 16;
	r = cos(r) * crandom() * NAILGUN_SPREAD * 16;
	VectorMA(start, 8192 * 16, forward, end);
	VectorMA(end, r, right, end);
	VectorMA(end, u, up, end);
	VectorSubtract(end, start, dir);
	VectorNormalize(dir);

	scale = 555 + random() * 1800;
	VectorScale(dir, scale, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);

	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}

#ifdef MISSIONPACK
/*
=================
fire_prox
=================
*/
gentity_t      *fire_prox(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *bolt;

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PROX_LAUNCHER;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 0;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_PROXIMITY_MINE;
	bolt->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	// count is used to check if the prox mine left the player bbox
	// if count == 1 then the prox mine left the player bbox and can attack to it
	bolt->count = 0;

	//FIXME: we prolly wanna abuse another field
	bolt->s.generic1 = self->client->sess.sessionTeam;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trAcceleration = g_gravity.value;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);
	VectorScale(dir, 700, bolt->s.pos.trDelta);
	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth

	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}
#endif



//=============================================================================





/*
===============
RailSphereRadiusDamage
===============
*/
static void RailSphereRadiusDamage(vec3_t origin, gentity_t * attacker, float damage, float radius)
{
	float           dist;
	gentity_t      *ent;
	int             entityList[MAX_GENTITIES];
	int             numListedEntities;
	vec3_t          mins, maxs;
	vec3_t          v;
	vec3_t          dir;
	int             i, e;

	if(radius < 1)
	{
		radius = 1;
	}

	for(i = 0; i < 3; i++)
	{
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for(e = 0; e < numListedEntities; e++)
	{
		ent = &g_entities[entityList[e]];

		if(!ent->takedamage)
		{
			continue;
		}

		// dont hit things we have already hit
		if(ent->kamikazeTime > level.time)
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for(i = 0; i < 3; i++)
		{
			if(origin[i] < ent->r.absmin[i])
			{
				v[i] = ent->r.absmin[i] - origin[i];
			}
			else if(origin[i] > ent->r.absmax[i])
			{
				v[i] = origin[i] - ent->r.absmax[i];
			}
			else
			{
				v[i] = 0;
			}
		}

		dist = VectorLength(v);
		if(dist >= radius)
		{
			continue;
		}

//      if( CanDamage (ent, origin) ) {
		VectorSubtract(ent->r.currentOrigin, origin, dir);
		// push the center of mass higher than the origin so players
		// get knocked into the air more
		dir[2] += 24;
		G_Damage(ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS | DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE);
		ent->kamikazeTime = level.time + 3000;
//      }
	}
}

/*
===============
RailSphereShockWave
===============
*/
static void RailSphereShockWave(vec3_t origin, gentity_t * attacker, float damage, float push, float radius)
{
	float           dist;
	gentity_t      *ent;
	int             entityList[MAX_GENTITIES];
	int             numListedEntities;
	vec3_t          mins, maxs;
	vec3_t          v;
	vec3_t          dir;
	int             i, e;

	if(radius < 1)
		radius = 1;

	for(i = 0; i < 3; i++)
	{
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	for(e = 0; e < numListedEntities; e++)
	{
		ent = &g_entities[entityList[e]];

		// dont hit things we have already hit
		if(ent->kamikazeShockTime > level.time)
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for(i = 0; i < 3; i++)
		{
			if(origin[i] < ent->r.absmin[i])
			{
				v[i] = ent->r.absmin[i] - origin[i];
			}
			else if(origin[i] > ent->r.absmax[i])
			{
				v[i] = origin[i] - ent->r.absmax[i];
			}
			else
			{
				v[i] = 0;
			}
		}

		dist = VectorLength(v);
		if(dist >= radius)
		{
			continue;
		}

//      if( CanDamage (ent, origin) ) {
		VectorSubtract(ent->r.currentOrigin, origin, dir);
		dir[2] += 24;
		G_Damage(ent, NULL, attacker, dir, origin, damage, DAMAGE_RADIUS | DAMAGE_NO_TEAM_PROTECTION, MOD_KAMIKAZE);
		//
		dir[2] = 0;
		VectorNormalize(dir);
		if(ent->client)
		{
			ent->client->ps.velocity[0] = dir[0] * push;
			ent->client->ps.velocity[1] = dir[1] * push;
			ent->client->ps.velocity[2] = 100;
		}
		ent->kamikazeShockTime = level.time + 3000;
//      }
	}
}

/*
===============
RailSphereDamage
===============
*/
static void RailSphereDamage(gentity_t * self)
{
	//int             i;
	float           t;

	//gentity_t      *ent;
	//vec3_t          newangles;
	int             damage;

	self->count += 100;
	damage = 200;				// FIXME * s_quadFactor;

	if(self->count >= RAILGUN_SHOCKWAVE_STARTTIME)
	{
		// shockwave push back
		t = self->count - RAILGUN_SHOCKWAVE_STARTTIME;
		RailSphereShockWave(self->s.pos.trBase, self->activator, 25, 200,
						  (int)(float)t * RAILGUN_SHOCKWAVE_MAXRADIUS / (RAILGUN_SHOCKWAVE_ENDTIME - RAILGUN_SHOCKWAVE_STARTTIME));
	}

	//
	if(self->count >= RAILGUN_EXPLODE_STARTTIME)
	{
		// do our damage
		t = self->count - RAILGUN_EXPLODE_STARTTIME;
		RailSphereRadiusDamage(self->s.pos.trBase, self->activator, 200,
							 (int)(float)t * RAILGUN_BOOMSPHERE_MAXRADIUS / (RAILGUN_IMPLODE_STARTTIME - RAILGUN_EXPLODE_STARTTIME));
	}

	// either cycle or kill self
	if(self->count >= RAILGUN_SHOCKWAVE_ENDTIME)
	{
		G_FreeEntity(self);
		return;
	}
	self->nextthink = level.time + 100;

#if 0
	// add earth quake effect
	newangles[0] = crandom() * 2;
	newangles[1] = crandom() * 2;
	newangles[2] = 0;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];
		if(!ent->inuse)
			continue;
		if(!ent->client)
			continue;

		if(ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			ent->client->ps.velocity[0] += crandom() * 120;
			ent->client->ps.velocity[1] += crandom() * 120;
			ent->client->ps.velocity[2] = 30 + random() * 25;
		}

		ent->client->ps.delta_angles[0] += ANGLE2SHORT(newangles[0] - self->movedir[0]);
		ent->client->ps.delta_angles[1] += ANGLE2SHORT(newangles[1] - self->movedir[1]);
		ent->client->ps.delta_angles[2] += ANGLE2SHORT(newangles[2] - self->movedir[2]);
	}
	VectorCopy(newangles, self->movedir);
#endif
}



/*
================
RailSphere_Die
================
*/
static void RailSphere_Die(gentity_t * ent, gentity_t * inflictor, gentity_t * attacker, int damage, int mod)
{
	gentity_t      *explosion;
	vec3_t          snapped;

	// start up the explosion logic
	explosion = G_Spawn();

	explosion->s.eType = ET_EVENTS + EV_RAILEXLOSION;
	explosion->eventTime = level.time;

	//VectorCopy(ent->s.pos.trBase, snapped);
	VectorCopy(ent->r.currentOrigin, snapped);
	SnapVector(snapped);		// save network bandwidth
	G_SetOrigin(explosion, snapped);

	explosion->classname = "kamikaze";
	explosion->s.pos.trType = TR_STATIONARY;

	explosion->kamikazeTime = level.time;

	explosion->think = RailSphereDamage;
	explosion->nextthink = level.time + 10;
	explosion->count = 0;
	VectorClear(explosion->movedir);

	trap_LinkEntity(explosion);

	G_FreeEntity(ent);
}

/*
=================
fire_railsphere
=================
*/
gentity_t      *fire_railsphere(gentity_t * self, vec3_t start, vec3_t dir)
{
	gentity_t      *bolt;
	vec3_t          mins = { -16, -16, -16 };
	vec3_t          maxs = { 16, 16, 16 };

	VectorNormalize(dir);

	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
//	bolt->think = RailSphereDamage;
//	bolt->think = G_ExplodeMissile;
	bolt->think = G_FreeEntity;	// FIXME
	bolt->s.eType = ET_PROJECTILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_RAILGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 75;
	bolt->splashDamage = 50;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_RAILGUN_SPLASH;
	bolt->splashMethodOfDeath = MOD_RAILGUN_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	// make the sphere shootable
	bolt->r.contents = CONTENTS_SHOOTABLE;
	VectorCopy(mins, bolt->r.mins);
	VectorCopy(maxs, bolt->r.maxs);
	bolt->takedamage = qtrue;
	bolt->health = 25;
	bolt->die = RailSphere_Die;

	bolt->s.pos.trType = TR_LINEAR;
	VectorScale(dir, 400, bolt->s.pos.trDelta);

	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	return bolt;
}
