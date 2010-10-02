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
// ui_xppm.c -- handle md5 model stuff ( instead of ui_players.c )
#include <hat/gui/ui_local.h>

static int      dp_realtime;

#define SWINGSPEED				0.3f

#ifdef XPPM

/*
======================
UI_XPPMParseCharacterFile

Read a configuration file containing body.md5mesh custom
models/players/visor/character.cfg, etc
======================
*/

static qboolean UI_XPPMParseCharacterFile(const char *filename, playerInfo_t * pi)
{
	char           *text_p, *prev;
	int             len;
	int             i;
	char           *token;
	int             skip;
	char            text[20000];
	fileHandle_t    f;

	Com_Printf("UI_XPPM: Loading configuration %s\n", filename);

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0)
	{
		return qfalse;
	}
	if(len >= sizeof(text) - 1)
	{
		Com_Printf("File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}

	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;
	skip = 0;					// quite the compiler warning


	pi->gender = GENDER_MALE;
	pi->firstTorsoBoneName[0] = '\0';
	pi->lastTorsoBoneName[0] = '\0';
	pi->torsoControlBoneName[0] = '\0';
	pi->neckControlBoneName[0] = '\0';
	pi->modelScale[0] = 1;
	pi->modelScale[1] = 1;
	pi->modelScale[2] = 1;

	// read optional parameters
	while(1)
	{
		prev = text_p;			// so we can unget
		token = Com_Parse(&text_p);
		if(!token[0])
		{
			break;
		}

		if(!Q_stricmp(token, "sex"))
		{
			token = Com_Parse(&text_p);
			if(!token)
			{
				break;
			}
			if(token[0] == 'f' || token[0] == 'F')
			{
				pi->gender = GENDER_FEMALE;
			}
			else if(token[0] == 'n' || token[0] == 'N')
			{
				pi->gender = GENDER_NEUTER;
			}
			else
			{
				pi->gender = GENDER_MALE;
			}
			continue;
		}
		else if(!Q_stricmp(token, "firstTorsoBoneName"))
		{
			token = Com_Parse(&text_p);
			Q_strncpyz(pi->firstTorsoBoneName, token, sizeof(pi->firstTorsoBoneName));
			continue;
		}
		else if(!Q_stricmp(token, "lastTorsoBoneName"))
		{
			token = Com_Parse(&text_p);
			Q_strncpyz(pi->lastTorsoBoneName, token, sizeof(pi->lastTorsoBoneName));
			continue;
		}
		else if(!Q_stricmp(token, "torsoControlBoneName"))
		{
			token = Com_Parse(&text_p);
			Q_strncpyz(pi->torsoControlBoneName, token, sizeof(pi->torsoControlBoneName));
			continue;
		}
		else if(!Q_stricmp(token, "neckControlBoneName"))
		{
			token = Com_Parse(&text_p);
			Q_strncpyz(pi->neckControlBoneName, token, sizeof(pi->neckControlBoneName));
			continue;
		}
		else if(!Q_stricmp(token, "modelScale"))
		{
			for(i = 0; i < 3; i++)
			{
				token = Com_ParseExt(&text_p, qfalse);
				if(!token)
				{
					break;
				}
				pi->modelScale[i] = atof(token);
			}
			continue;
		}

		//Com_Printf("unknown token '%s' is %s\n", token, filename);
	}

	return qtrue;
}

static qboolean UI_XPPM_RegisterPlayerAnimation(playerInfo_t * pi, const char *modelName, int anim, const char *animName,
										   qboolean loop, qboolean reversed, qboolean clearOrigin)
{
	char            filename[MAX_QPATH];
	int             frameRate;


	Com_sprintf(filename, sizeof(filename), "models/players/%s/%s.md5anim", modelName, animName);

	Com_Printf("UI_XPPM: Loading animation %s\n", filename);

	pi->animations[anim].handle = trap_R_RegisterAnimation(filename);


	if(!pi->animations[anim].handle)
	{
		Com_Printf("Failed to load animation file %s\n", filename);
		return qfalse;
	}

	pi->animations[anim].firstFrame = 0;
	pi->animations[anim].numFrames = trap_R_AnimNumFrames(pi->animations[anim].handle);
	frameRate = trap_R_AnimFrameRate(pi->animations[anim].handle);

	if(frameRate == 0)
	{
		frameRate = 1;
	}
	pi->animations[anim].frameTime = 1000 / frameRate;
	pi->animations[anim].initialLerp = 1000 / frameRate;

	if(loop)
	{
		pi->animations[anim].loopFrames = pi->animations[anim].numFrames;
	}
	else
	{
		pi->animations[anim].loopFrames = 0;
	}

	pi->animations[anim].reversed = reversed;
	pi->animations[anim].clearOrigin = clearOrigin;

	return qtrue;
}



/*
==========================
UI_XPPM_RegisterModel
==========================
*/

qboolean UI_XPPM_RegisterModel(playerInfo_t * pi, const char *modelName, const char *skinName)
{
	int             i;
	char            filename[MAX_QPATH * 2];

	Com_sprintf(filename, sizeof(filename), "models/players/%s/body.md5mesh", modelName);

	Com_Printf("UI_XPPM: Loading model %s\n", filename);

	pi->bodyModel = trap_R_RegisterModel(filename, qfalse);

	if(!pi->bodyModel)
	{
		Com_Printf("Failed to load body mesh file  %s\n", filename);
		return qfalse;
	}

	if(pi->bodyModel)
	{
		// load the animations
		Com_sprintf(filename, sizeof(filename), "models/players/%s/character.cfg", modelName);
		if(!UI_XPPMParseCharacterFile(filename, pi))
		{
			Com_Printf("Failed to load character file %s\n", filename);
			return qfalse;
		}

		if(!UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_IDLE, "idle", qtrue, qfalse, qfalse))
		{
			Com_Printf("Failed to load idle animation file %s\n", filename);
			return qfalse;
		}

		// make LEGS_IDLE the default animation
		for(i = 0; i < MAX_PLAYER_ANIMATIONS; i++)
		{
			if(i == LEGS_IDLE)
				continue;

			pi->animations[i] = pi->animations[LEGS_IDLE];
		}

		// FIXME we don't have death animations with player models created for Quake 4
		UI_XPPM_RegisterPlayerAnimation(pi, modelName, BOTH_DEATH1, "die", qfalse, qfalse, qfalse);
		//UI_XPPM_RegisterPlayerAnimation(pi, modelName, BOTH_DEATH2, "die", qfalse, qfalse, qfalse);
		//UI_XPPM_RegisterPlayerAnimation(pi, modelName, BOTH_DEATH3, "die", qfalse, qfalse, qfalse);

		UI_XPPM_RegisterPlayerAnimation(pi, modelName, TORSO_GESTURE, "idle", qfalse, qfalse, qfalse);

		UI_XPPM_RegisterPlayerAnimation(pi, modelName, TORSO_ATTACK, "attack", qfalse, qfalse, qfalse);
		UI_XPPM_RegisterPlayerAnimation(pi, modelName, TORSO_ATTACK2, "idle", qfalse, qfalse, qfalse);

		UI_XPPM_RegisterPlayerAnimation(pi, modelName, TORSO_STAND2, "idle", qfalse, qfalse, qfalse);

		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_IDLECR, "crouch", qtrue, qfalse, qfalse);
		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_WALKCR, "crouch_forward", qtrue, qfalse, qtrue);
		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_BACKCR, "crouch_forward", qtrue, qtrue, qtrue);

		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_WALK, "walk", qtrue, qfalse, qtrue);
		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_BACKWALK, "walk_backwards", qtrue, qfalse, qtrue);

		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_RUN, "run", qtrue, qfalse, qtrue);
		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_BACK, "run_backwards", qtrue, qtrue, qtrue);

		// FIXME UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_SWIM, "swim", qtrue);

		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_JUMP, "jump", qfalse, qfalse, qfalse);
		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_LAND, "land", qfalse, qfalse, qfalse);

		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_JUMPB, "jump", qfalse, qfalse, qfalse);	// FIXME ?
		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_LANDB, "land", qfalse, qfalse, qfalse);


		UI_XPPM_RegisterPlayerAnimation(pi, modelName, LEGS_TURN, "step", qfalse, qfalse, qfalse);


		Com_sprintf(filename, sizeof(filename), "models/players/%s/body_%s.skin", modelName, skinName);

		Com_Printf("UI_XPPM: Loading skin %s\n", filename);

		pi->bodySkin = trap_R_RegisterSkin(filename);

		if(!pi->bodySkin)
		{
			Com_Printf("Body skin load failure: %s\n", filename);
			return qfalse;
		}
	}

	return qtrue;
}

/*static void UI_PlayerFloatSprite(playerInfo_t * pi, vec3_t origin, qhandle_t shader)
{
	refEntity_t     ent;

	memset(&ent, 0, sizeof(ent));
	VectorCopy(origin, ent.origin);
	ent.origin[2] += 24;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = 0;
	trap_R_AddRefEntityToScene(&ent);
}*/


void UI_XPPM_TransformSkeleton(refSkeleton_t * skel, const vec3_t scale)
{
	int             i;
	refBone_t      *bone;

	switch (skel->type)
	{
		case SK_INVALID:
		case SK_ABSOLUTE:
			return;

		default:
			break;
	}

	// calculate absolute transforms
	for(i = 0, bone = &skel->bones[0]; i < skel->numBones; i++, bone++)
	{
		if(bone->parentIndex >= 0)
		{
			vec3_t          rotated;
			quat_t          quat;

			refBone_t      *parent;

			parent = &skel->bones[bone->parentIndex];

			QuatTransformVector(parent->rotation, bone->origin, rotated);

			if(scale)
			{
				rotated[0] *= scale[0];
				rotated[1] *= scale[1];
				rotated[2] *= scale[2];
			}

			VectorAdd(parent->origin, rotated, bone->origin);

			QuatMultiply1(parent->rotation, bone->rotation, quat);
			QuatCopy(quat, bone->rotation);
		}
	}

	skel->type = SK_ABSOLUTE;

	if(scale)
	{
		VectorCopy(scale, skel->scale);
	}
	else
	{
		VectorSet(skel->scale, 1, 1, 1);
	}
}


/*static void UI_XPPM_SwingAngles(float destination, float swingTolerance, float clampTolerance,
						   float speed, float *angle, qboolean * swinging)
{
	float           swing;
	float           move;
	float           scale;

	if(!*swinging)
	{
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if(swing > swingTolerance || swing < -swingTolerance)
		{
			*swinging = qtrue;
		}
	}

	if(!*swinging)
	{
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract(destination, *angle);
	scale = fabs(swing);
	if(scale < swingTolerance * 0.5)
	{
		scale = 0.5;
	}
	else if(scale < swingTolerance)
	{
		scale = 1.0;
	}
	else
	{
		scale = 2.0;
	}

	// swing towards the destination angle
	if(swing >= 0)
	{
		move = uis.frametime * scale * speed;
		if(move >= swing)
		{
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleNormalize360(*angle + move);
	}
	else if(swing < 0)
	{
		move = uis.frametime * scale * -speed;
		if(move <= swing)
		{
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleNormalize360(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if(swing > clampTolerance)
	{
		*angle = AngleNormalize360(destination - (clampTolerance - 1));
	}
	else if(swing < -clampTolerance)
	{
		*angle = AngleNormalize360(destination + (clampTolerance - 1));
	}
}

static float UI_XPPM_MovedirAdjustment(playerInfo_t * pi)
{
	vec3_t          relativeAngles;
	vec3_t          moveVector;

	VectorSubtract(pi->viewAngles, pi->moveAngles, relativeAngles);
	AngleVectors(relativeAngles, moveVector, NULL, NULL);
	if(Q_fabs(moveVector[0]) < 0.01)
	{
		moveVector[0] = 0.0;
	}
	if(Q_fabs(moveVector[1]) < 0.01)
	{
		moveVector[1] = 0.0;
	}

	if(moveVector[1] == 0 && moveVector[0] > 0)
	{
		return 0;
	}
	if(moveVector[1] < 0 && moveVector[0] > 0)
	{
		return 22;
	}
	if(moveVector[1] < 0 && moveVector[0] == 0)
	{
		return 45;
	}
	if(moveVector[1] < 0 && moveVector[0] < 0)
	{
		return -22;
	}
	if(moveVector[1] == 0 && moveVector[0] < 0)
	{
		return 0;
	}
	if(moveVector[1] > 0 && moveVector[0] < 0)
	{
		return 22;
	}
	if(moveVector[1] > 0 && moveVector[0] == 0)
	{
		return -45;
	}

	return -22;
}

static void UI_XPPM_PlayerAngles(playerInfo_t * pi, vec3_t legsAngles, vec3_t torsoAngles, vec3_t headAngles)
{
	float           dest;
	float           adjust;

	VectorCopy(pi->viewAngles, headAngles);
	headAngles[YAW] = AngleNormalize360(headAngles[YAW]);
	VectorClear(legsAngles);
	VectorClear(torsoAngles);

	// --------- yaw -------------

	// allow yaw to drift a bit
	if((pi->legsAnim & ~ANIM_TOGGLEBIT) != LEGS_IDLE || (pi->torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND)
	{
		// if not standing still, always point all in the same direction
		pi->torso.yawing = qtrue;	// always center
		pi->torso.pitching = qtrue;	// always center
		pi->legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	adjust = UI_XPPM_MovedirAdjustment(pi);
	legsAngles[YAW] = headAngles[YAW] + adjust;
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * adjust;


	// torso
	UI_XPPM_SwingAngles(torsoAngles[YAW], 25, 90, SWINGSPEED, &pi->torso.yawAngle, &pi->torso.yawing);
	UI_XPPM_SwingAngles(legsAngles[YAW], 40, 90, SWINGSPEED, &pi->legs.yawAngle, &pi->legs.yawing);

	torsoAngles[YAW] = pi->torso.yawAngle;
	legsAngles[YAW] = pi->legs.yawAngle;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if(headAngles[PITCH] > 180)
	{
		dest = (-360 + headAngles[PITCH]) * 0.75;
	}
	else
	{
		dest = headAngles[PITCH] * 0.75;
	}
	UI_XPPM_SwingAngles(dest, 15, 30, 0.1f, &pi->torso.pitchAngle, &pi->torso.pitching);
	torsoAngles[PITCH] = pi->torso.pitchAngle;

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
}*/



void UI_XPPM_Player(float x, float y, float w, float h, playerInfo_t * pi, int time)
{
	refEntity_t     body;
	refEntity_t     podium;
	refdef_t        refdef;

	vec3_t          legsAngles;
	//vec3_t          torsoAngles;
	//vec3_t          headAngles;

	vec3_t          podiumAngles;

	vec3_t          origin;
	int             renderfx;

	vec3_t          mins = { -16, -16, -24 };
	vec3_t          maxs = { 16, 16, 32 };
	float           len;
	float           xx;

	if(!pi->bodyModel)
		return;

	if(!pi->bodySkin)
		return;

	dp_realtime = time;

	//UI_DrawRect(x, y, w, h, colorYellow);

	UI_AdjustFrom640(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof(refdef));
	memset(&body, 0, sizeof(body));
	memset(&podium, 0, sizeof(podium));

	refdef.rdflags = RDF_NOWORLDMODEL | RDF_NOSHADOWS;

	AxisClear(refdef.viewaxis);

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = (int)((float)refdef.width / 640.0f * 60.0f);
	xx = refdef.width / tan(refdef.fov_x / 360 * M_PI);
	refdef.fov_y = atan2(refdef.height, xx);
	refdef.fov_y *= (360 / M_PI);

	// calculate distance so the player nearly fills the box
	len = 0.7 * (maxs[2] - mins[2]);
	origin[0] = len / tan(DEG2RAD(refdef.fov_x) * 0.5) + 10;
	origin[1] = 0.5 * (mins[1] + maxs[1]);
	origin[2] = -0.5 * (mins[2] + maxs[2]);
	origin[2] -= len - 20;

	refdef.time = dp_realtime;

	trap_R_ClearScene();

	// get the rotation information
#if 0
	UI_XPPM_PlayerAngles(pi, legsAngles, torsoAngles, headAngles);
#else
	// Quake 2 style
	legsAngles[YAW] = AngleNormalize360((float)(uis.realtime / 30.0)); //180 - 30;
	legsAngles[PITCH] = 0;
	legsAngles[ROLL] = 0;
#endif

	AnglesToAxis(legsAngles, body.axis);

	renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	// add the body
	VectorCopy(origin, body.origin);
	VectorCopy(body.origin, body.oldorigin);

	body.hModel = pi->bodyModel;
	body.customSkin = pi->bodySkin;
	body.shaderTime = 1.0f;

	body.renderfx = renderfx;
	VectorCopy(origin, body.lightingOrigin);
	body.lightingOrigin[0] -= 150;			// + = behind, - = in front
	body.lightingOrigin[1] += 150;			// + = left, - = right
	body.lightingOrigin[2] += 3000;			// + = above, - = below

	body.backlerp = 1.0f;
	body.frame = 1;
	body.oldframe = 0;

	// modify bones and set proper local bounds for culling
	if(!trap_R_BuildSkeleton(&body.skeleton, pi->animations[LEGS_IDLE].handle, body.oldframe, body.frame, 1.0 - body.backlerp, qfalse))
	{
		Com_Printf("Can't build animation\n");
		return;
	}

	if(body.skeleton.type == SK_RELATIVE)
	{
		// transform relative bones to absolute ones required for vertex skinning
		UI_XPPM_TransformSkeleton(&body.skeleton, NULL);
	}


	//UI_PlayerFloatSprite(pi, origin, trap_R_RegisterShaderNoMip("sprites/balloon3"));
	trap_R_AddRefEntityToScene(&body);


	VectorCopy(legsAngles, podiumAngles);
	AnglesToAxis(podiumAngles, podium.axis);

	// add the podium
	VectorCopy(origin, podium.origin);
	podium.origin[2] += 1;

	VectorCopy(podium.origin, podium.oldorigin);

	podium.hModel = uis.podiumModel;

	//podium.customSkin = pi->bodySkin;
	//podium.shaderTime = 1.0f;

	podium.renderfx = renderfx;
	VectorCopy(origin, podium.lightingOrigin);
	podium.lightingOrigin[0] -= 150;			// + = behind, - = in front
	podium.lightingOrigin[1] += 150;			// + = left, - = right
	podium.lightingOrigin[2] += 3000;			// + = above, - = below

	podium.backlerp = 1.0f;
	podium.frame = 1;
	podium.oldframe = 0;

	//UI_PlayerFloatSprite(pi, origin, trap_R_RegisterShaderNoMip("sprites/balloon3"));
	trap_R_AddRefEntityToScene(&podium);


#if 1
	origin[0] -= 150;			// + = behind, - = in front
	origin[1] += 150;			// + = left, - = right
	origin[2] += 150;			// + = above, - = below
	trap_R_AddLightToScene(origin, 300, 1.0, 1.0, 1.0);
#endif

#if 1
	origin[0] -= 150;
	origin[1] -= 150;
	origin[2] -= 150;
	trap_R_AddLightToScene(origin, 400, 1.0, 1.0, 1.0);
#endif

	trap_R_RenderScene(&refdef);
}


#endif
