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
// cg_cgsyscalls.c -- this file is only included when building a dll
#include <hat/client/traps.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#include <hat/client/cg_local.h>

static intptr_t(QDECL * cgsyscall) (intptr_t arg, ...) = (intptr_t(QDECL *) (intptr_t,...)) - 1;


void dllEntry(intptr_t(QDECL * cgsyscallptr) (intptr_t arg, ...))
{
  cgsyscall = cgsyscallptr;
}


int PASSFLOAT(float x)
{
  float           floatTemp;

  floatTemp = x;
  return *(int *)&floatTemp;
}

void trap_Print(const char *fmt)
{
  cgsyscall(CG_PRINT, fmt);
}

void trap_Error(const char *fmt)
{
  cgsyscall(CG_ERROR, fmt);
}

int trap_Milliseconds(void)
{
  return cgsyscall(CG_MILLISECONDS);
}

void trap_Cvar_Register(vmCvar_t * vmCvar, const char *varName, const char *defaultValue, int flags)
{
  cgsyscall(CG_CVAR_REGISTER, vmCvar, varName, defaultValue, flags);
}

void trap_Cvar_Update(vmCvar_t * vmCvar)
{
  cgsyscall(CG_CVAR_UPDATE, vmCvar);
}

void trap_Cvar_Set(const char *var_name, const char *value)
{
  cgsyscall(CG_CVAR_SET, var_name, value);
}

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize)
{
  cgsyscall(CG_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize);
}

int trap_Argc(void)
{
  return cgsyscall(CG_ARGC);
}

void trap_Argv(int n, char *buffer, int bufferLength)
{
  cgsyscall(CG_ARGV, n, buffer, bufferLength);
}

void trap_Args(char *buffer, int bufferLength)
{
  cgsyscall(CG_ARGS, buffer, bufferLength);
}

int trap_FS_FOpenFile(const char *qpath, fileHandle_t * f, fsMode_t mode)
{
  return cgsyscall(CG_FS_FOPENFILE, qpath, f, mode);
}

void trap_FS_Read(void *buffer, int len, fileHandle_t f)
{
  cgsyscall(CG_FS_READ, buffer, len, f);
}

void trap_FS_Write(const void *buffer, int len, fileHandle_t f)
{
  cgsyscall(CG_FS_WRITE, buffer, len, f);
}

void trap_FS_FCloseFile(fileHandle_t f)
{
  cgsyscall(CG_FS_FCLOSEFILE, f);
}

int trap_FS_Seek(fileHandle_t f, long offset, int origin)
{
  return cgsyscall(CG_FS_SEEK, f, offset, origin);
}

int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize)
{
  return cgsyscall(CG_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

void trap_SendConsoleCommand(const char *text)
{
  cgsyscall(CG_SENDCONSOLECOMMAND, text);
}

void trap_AddCommand(const char *cmdName)
{
  cgsyscall(CG_ADDCOMMAND, cmdName);
}

void trap_RemoveCommand(const char *cmdName)
{
  cgsyscall(CG_REMOVECOMMAND, cmdName);
}

void trap_SendClientCommand(const char *s)
{
  cgsyscall(CG_SENDCLIENTCOMMAND, s);
}

void trap_UpdateScreen(void)
{
  cgsyscall(CG_UPDATESCREEN);
}

void trap_CM_LoadMap(const char *mapname)
{
  cgsyscall(CG_CM_LOADMAP, mapname);
}

int trap_CM_NumInlineModels(void)
{
  return cgsyscall(CG_CM_NUMINLINEMODELS);
}

clipHandle_t trap_CM_InlineModel(int index)
{
  return cgsyscall(CG_CM_INLINEMODEL, index);
}

clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs)
{
  return cgsyscall(CG_CM_TEMPBOXMODEL, mins, maxs);
}

clipHandle_t trap_CM_TempCapsuleModel(const vec3_t mins, const vec3_t maxs)
{
  return cgsyscall(CG_CM_TEMPCAPSULEMODEL, mins, maxs);
}

int trap_CM_PointContents(const vec3_t p, clipHandle_t model)
{
  return cgsyscall(CG_CM_POINTCONTENTS, p, model);
}

int trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles)
{
  return cgsyscall(CG_CM_TRANSFORMEDPOINTCONTENTS, p, model, origin, angles);
}

void trap_CM_BoxTrace(trace_t * results, const vec3_t start, const vec3_t end,
            const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask)
{
  cgsyscall(CG_CM_BOXTRACE, results, start, end, mins, maxs, model, brushmask);
}

void trap_CM_CapsuleTrace(trace_t * results, const vec3_t start, const vec3_t end,
              const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask)
{
  cgsyscall(CG_CM_CAPSULETRACE, results, start, end, mins, maxs, model, brushmask);
}

void trap_CM_TransformedBoxTrace(trace_t * results, const vec3_t start, const vec3_t end,
                 const vec3_t mins, const vec3_t maxs,
                 clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles)
{
  cgsyscall(CG_CM_TRANSFORMEDBOXTRACE, results, start, end, mins, maxs, model, brushmask, origin, angles);
}

void trap_CM_TransformedCapsuleTrace(trace_t * results, const vec3_t start, const vec3_t end,
                   const vec3_t mins, const vec3_t maxs,
                   clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles)
{
  cgsyscall(CG_CM_TRANSFORMEDCAPSULETRACE, results, start, end, mins, maxs, model, brushmask, origin, angles);
}

void trap_CM_BiSphereTrace(trace_t * results, const vec3_t start,
               const vec3_t end, float startRad, float endRad, clipHandle_t model, int mask)
{
  cgsyscall(CG_CM_BISPHERETRACE, results, start, end, PASSFLOAT(startRad), PASSFLOAT(endRad), model, mask);
}

void trap_CM_TransformedBiSphereTrace(trace_t * results, const vec3_t start,
                    const vec3_t end, float startRad, float endRad,
                    clipHandle_t model, int mask, const vec3_t origin)
{
  cgsyscall(CG_CM_TRANSFORMEDBISPHERETRACE, results, start, end, PASSFLOAT(startRad), PASSFLOAT(endRad), model, mask, origin);
}

int trap_CM_MarkFragments(int numPoints, const vec3_t * points,
              const vec3_t projection,
              int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t * fragmentBuffer)
{
  return cgsyscall(CG_CM_MARKFRAGMENTS, numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer);
}

void trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx)
{
  cgsyscall(CG_S_STARTSOUND, origin, entityNum, entchannel, sfx);
}

void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum)
{
  cgsyscall(CG_S_STARTLOCALSOUND, sfx, channelNum);
}

void trap_S_ClearLoopingSounds(qboolean killall)
{
  cgsyscall(CG_S_CLEARLOOPINGSOUNDS, killall);
}

void trap_S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx)
{
  cgsyscall(CG_S_ADDLOOPINGSOUND, entityNum, origin, velocity, sfx);
}

void trap_S_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx)
{
  cgsyscall(CG_S_ADDREALLOOPINGSOUND, entityNum, origin, velocity, sfx);
}

void trap_S_StopLoopingSound(int entityNum)
{
  cgsyscall(CG_S_STOPLOOPINGSOUND, entityNum);
}

void trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin)
{
  cgsyscall(CG_S_UPDATEENTITYPOSITION, entityNum, origin);
}

void trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater)
{
  cgsyscall(CG_S_RESPATIALIZE, entityNum, origin, axis, inwater);
}

sfxHandle_t trap_S_RegisterSound(const char *sample)
{
  return cgsyscall(CG_S_REGISTERSOUND, sample);
}

void trap_S_StartBackgroundTrack(const char *intro, const char *loop)
{
  cgsyscall(CG_S_STARTBACKGROUNDTRACK, intro, loop);
}

void trap_R_LoadWorldMap(const char *mapname)
{
  cgsyscall(CG_R_LOADWORLDMAP, mapname);
}

qhandle_t trap_R_RegisterModel(const char *name, qboolean forceStatic)
{
  return cgsyscall(CG_R_REGISTERMODEL, name, forceStatic);
}

qhandle_t trap_R_RegisterAnimation(const char *name)
{
  return cgsyscall(CG_R_REGISTERANIMATION, name);
}

qhandle_t trap_R_RegisterSkin(const char *name)
{
  return cgsyscall(CG_R_REGISTERSKIN, name);
}

qhandle_t trap_R_RegisterShader(const char *name)
{
  return cgsyscall(CG_R_REGISTERSHADER, name);
}

qhandle_t trap_R_RegisterShaderNoMip(const char *name)
{
  return cgsyscall(CG_R_REGISTERSHADERNOMIP, name);
}

qhandle_t trap_R_RegisterShaderLightAttenuation(const char *name)
{
  return cgsyscall(CG_R_REGISTERSHADERLIGHTATTENUATION, name);
}

void trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t * font)
{
  cgsyscall(CG_R_REGISTERFONT, fontName, pointSize, font);
}

void trap_R_ClearScene(void)
{
  cgsyscall(CG_R_CLEARSCENE);
}

void trap_R_AddRefEntityToScene(const refEntity_t * ent)
{
  cgsyscall(CG_R_ADDREFENTITYTOSCENE, ent);
}

void trap_R_AddRefLightToScene(const refLight_t * light)
{
  cgsyscall(CG_R_ADDREFLIGHTSTOSCENE, light);
}

void trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t * verts)
{
  cgsyscall(CG_R_ADDPOLYTOSCENE, hShader, numVerts, verts);
}

void trap_R_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t * verts, int num)
{
  cgsyscall(CG_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, num);
}

int trap_R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir)
{
  return cgsyscall(CG_R_LIGHTFORPOINT, point, ambientLight, directedLight, lightDir);
}

void trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b)
{
  cgsyscall(CG_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b));
}

void trap_R_RenderScene(const refdef_t * fd)
{
  cgsyscall(CG_R_RENDERSCENE, fd);
}

void trap_R_SetColor(const float *rgba)
{
  cgsyscall(CG_R_SETCOLOR, rgba);
}

void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader)
{
  cgsyscall(CG_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1),
      PASSFLOAT(s2), PASSFLOAT(t2), hShader);
}

void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs)
{
  cgsyscall(CG_R_MODELBOUNDS, model, mins, maxs);
}

int trap_R_LerpTag(orientation_t * tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName)
{
  return cgsyscall(CG_R_LERPTAG, tag, mod, startFrame, endFrame, PASSFLOAT(frac), tagName);
}

int trap_R_CheckSkeleton(refSkeleton_t * skel, qhandle_t hModel, qhandle_t hAnim)
{
  return cgsyscall(CG_R_CHECKSKELETON, skel, hModel, hAnim);
}

int trap_R_BuildSkeleton(refSkeleton_t * skel, qhandle_t anim, int startFrame, int endFrame, float frac, qboolean clearOrigin)
{
  return cgsyscall(CG_R_BUILDSKELETON, skel, anim, startFrame, endFrame, PASSFLOAT(frac), clearOrigin);
}

int trap_R_BlendSkeleton(refSkeleton_t * skel, const refSkeleton_t * blend, float frac)
{
  return cgsyscall(CG_R_BLENDSKELETON, skel, blend, PASSFLOAT(frac));
}

int trap_R_BoneIndex(qhandle_t hModel, const char *boneName)
{
  return cgsyscall(CG_R_BONEINDEX, hModel, boneName);
}

int trap_R_AnimNumFrames(qhandle_t hAnim)
{
  return cgsyscall(CG_R_ANIMNUMFRAMES, hAnim);
}

int trap_R_AnimFrameRate(qhandle_t hAnim)
{
  return cgsyscall(CG_R_ANIMFRAMERATE, hAnim);
}

void trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset)
{
  cgsyscall(CG_R_REMAP_SHADER, oldShader, newShader, timeOffset);
}

void trap_GetGlconfig(glConfig_t * glconfig)
{
  cgsyscall(CG_GETGLCONFIG, glconfig);
}

void trap_GetGameState(gameState_t * gamestate)
{
  cgsyscall(CG_GETGAMESTATE, gamestate);
}

void trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime)
{
  cgsyscall(CG_GETCURRENTSNAPSHOTNUMBER, snapshotNumber, serverTime);
}

qboolean trap_GetSnapshot(int snapshotNumber, snapshot_t * snapshot)
{
  return (qboolean)cgsyscall(CG_GETSNAPSHOT, snapshotNumber, snapshot);
}

qboolean trap_GetServerCommand(int serverCommandNumber)
{
  return (qboolean)cgsyscall(CG_GETSERVERCOMMAND, serverCommandNumber);
}

int trap_GetCurrentCmdNumber(void)
{
  return cgsyscall(CG_GETCURRENTCMDNUMBER);
}

qboolean trap_GetUserCmd(int cmdNumber, usercmd_t * ucmd)
{
  return (qboolean)cgsyscall(CG_GETUSERCMD, cmdNumber, ucmd);
}

void trap_SetUserCmdValue(int stateValue, float sensitivityScale)
{
  cgsyscall(CG_SETUSERCMDVALUE, stateValue, PASSFLOAT(sensitivityScale));
}

void testPrintInt(char *string, int i)
{
  cgsyscall(CG_TESTPRINTINT, string, i);
}

void testPrintFloat(char *string, float f)
{
  cgsyscall(CG_TESTPRINTFLOAT, string, PASSFLOAT(f));
}

int trap_MemoryRemaining(void)
{
  return cgsyscall(CG_MEMORY_REMAINING);
}

qboolean trap_Key_IsDown(int keynum)
{
  return (qboolean)cgsyscall(CG_KEY_ISDOWN, keynum);
}

int trap_Key_GetCatcher(void)
{
  return cgsyscall(CG_KEY_GETCATCHER);
}

void trap_Key_SetCatcher(int catcher)
{
  cgsyscall(CG_KEY_SETCATCHER, catcher);
}

int trap_Key_GetKey(const char *binding)
{
  return cgsyscall(CG_KEY_GETKEY, binding);
}

int trap_PC_AddGlobalDefine(char *define)
{
  return cgsyscall(CG_PC_ADD_GLOBAL_DEFINE, define);
}

int trap_PC_LoadSource(const char *filename)
{
  return cgsyscall(CG_PC_LOAD_SOURCE, filename);
}

int trap_PC_FreeSource(int handle)
{
  return cgsyscall(CG_PC_FREE_SOURCE, handle);
}

int trap_PC_ReadToken(int handle, pc_token_t * pc_token)
{
  return cgsyscall(CG_PC_READ_TOKEN, handle, pc_token);
}

int trap_PC_SourceFileAndLine(int handle, char *filename, int *line)
{
  return cgsyscall(CG_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}

void trap_S_StopBackgroundTrack(void)
{
  cgsyscall(CG_S_STOPBACKGROUNDTRACK);
}

int trap_RealTime(qtime_t * qtime)
{
  return cgsyscall(CG_REAL_TIME, qtime);
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits)
{
  return cgsyscall(CG_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits);
}

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic(int handle)
{
  return (e_status)cgsyscall(CG_CIN_STOPCINEMATIC, handle);
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic(int handle)
{
  return (e_status)cgsyscall(CG_CIN_RUNCINEMATIC, handle);
}


// draws the current frame
void trap_CIN_DrawCinematic(int handle)
{
  cgsyscall(CG_CIN_DRAWCINEMATIC, handle);
}


// allows you to resize the animation dynamically
void trap_CIN_SetExtents(int handle, int x, int y, int w, int h)
{
  cgsyscall(CG_CIN_SETEXTENTS, handle, x, y, w, h);
}

int trap_GetDemoState(void)
{
  return cgsyscall(CG_GETDEMOSTATE);
}

int trap_GetDemoPos(void)
{
  return cgsyscall(CG_GETDEMOPOS);
}

qboolean trap_GetEntityToken(char *buffer, int bufferSize)
{
  return (qboolean)cgsyscall(CG_GET_ENTITY_TOKEN, buffer, bufferSize);
}

qboolean trap_R_inPVS(const vec3_t p1, const vec3_t p2)
{
  return (qboolean)cgsyscall(CG_R_INPVS, p1, p2);
}

void trap_GetDemoName(char *buffer, int size)
{
  cgsyscall(CG_GETDEMONAME, buffer, size);
}

void trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen)
{
  cgsyscall(CG_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen);
}

void trap_Key_GetBindingBuf(int keynum, char *buf, int buflen)
{
  cgsyscall(CG_KEY_GETBINDINGBUF, keynum, buf, buflen);
}

void trap_Key_SetBinding(int keynum, const char *binding)
{
  cgsyscall(CG_KEY_SETBINDING, keynum, binding);
}


qboolean trap_GetWeaponAttrs(int clientNum, int weaponID, const void** attrs)
{
  return (qboolean)cgsyscall(CG_GET_WEAPON_ATTRIBUTES, cg.clientNum, weaponID, attrs);
}

#ifdef __cplusplus
}
#endif



bool trap_GetWeaponAttributes(const int weapon_id,
  hat::Weapon_attrs const ** attrs)
{
  return cgsyscall(CG_GET_WEAPON_ATTRIBUTES, cg.clientNum, weapon_id, attrs);
}

int trap_LoadWeapon(const char* weapon)
{
  return cgsyscall(CG_LOAD_WEAPON, cg.clientNum, weapon);
}
