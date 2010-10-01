/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2010 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// cl_cgame.c  -- client system interaction with client game

#include <hat/engine/client.h>

#if defined(USE_JAVA)

#include "../qcommon/vm_java.h"

extern qboolean loadCamera(const char *name);
extern void     startCamera(int time);
extern qboolean getCameraInfo(int time, vec3_t * origin, vec3_t * angles);

/*
====================
CL_GetGameState
====================
*/
static void CL_GetGameState(gameState_t * gs)
{
	*gs = cl.gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
static void CL_GetGlconfig(glConfig_t * glconfig)
{
	*glconfig = cls.glconfig;
}




/*
====================
CL_GetParseEntityState
====================
*/
static qboolean CL_GetParseEntityState(int parseEntityNumber, entityState_t * state)
{
	// can't return anything that hasn't been parsed yet
	if(parseEntityNumber >= cl.parseEntitiesNum)
	{
		Com_Error(ERR_DROP, "CL_GetParseEntityState: %i >= %i", parseEntityNumber, cl.parseEntitiesNum);
	}

	// can't return anything that has been overwritten in the circular buffer
	if(parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES)
	{
		return qfalse;
	}

	*state = cl.parseEntities[parseEntityNumber & (MAX_PARSE_ENTITIES - 1)];
	return qtrue;
}



/*
=====================
CL_SetUserCmdValue
=====================
*/
void CL_SetUserCmdValue(int userCmdValue, float sensitivityScale)
{
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameSensitivity = sensitivityScale;
}

/*
=====================
CL_AddCgameCommand
=====================
*/
void CL_AddCgameCommand(const char *cmdName)
{
	Cmd_AddCommand(cmdName, NULL);
}

/*
=====================
CL_CgameError
=====================
*/
void CL_CgameError(const char *string)
{
	Com_Error(ERR_DROP, "%s", string);
}








/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap(const char *mapname)
{
	int             checksum;

	CM_LoadMap(mapname, qtrue, &checksum);
}



// ====================================================================================


// handle to ClientGame class
static jclass	class_ClientGame = NULL;
static jobject  object_ClientGame = NULL;
static jclass   interface_ClientGameListener;
static jmethodID method_ClientGame_ctor;
static jmethodID method_ClientGame_shutdownClientGame;
static jmethodID method_ClientGame_drawActiveFrame;
static jmethodID method_ClientGame_keyEvent;
static jmethodID method_ClientGame_mouseEvent;
static jmethodID method_ClientGame_eventHandling;
static jmethodID method_ClientGame_lastAttacker;
static jmethodID method_ClientGame_crosshairPlayer;
static jmethodID method_ClientGame_consoleCommand;

static void ClientGame_javaRegister(int serverMessageNum, int serverCommandSequence, int clientNum)
{
	Com_Printf("ClientGame_javaRegister()\n");

	// load the interface ClientGameListener
	interface_ClientGameListener = (*javaEnv)->FindClass(javaEnv, "xreal/client/game/ClientGameListener");
	if(CheckException() || !interface_ClientGameListener)
	{
		Com_Error(ERR_DROP, "Couldn't find class xreal.client.game.ClientGameListener");
	}

	// load the class ClientGame
	class_ClientGame = (*javaEnv)->FindClass(javaEnv, "xreal/client/game/ClientGame");
	if(CheckException() || !class_ClientGame)
	{
		Com_Error(ERR_DROP, "Couldn't find class xreal.client.game.ClientGame");
	}

	// check class ClientGame against interface ClientGameListener
	if(!((*javaEnv)->IsAssignableFrom(javaEnv, class_ClientGame, interface_ClientGameListener)))
	{
		Com_Error(ERR_DROP, "The specified ClientGame class doesn't implement xreal.client.game.ClientGameListener");
	}

	// remove old game if existing
	(*javaEnv)->DeleteLocalRef(javaEnv, interface_ClientGameListener);

	// load game interface methods
	method_ClientGame_shutdownClientGame = (*javaEnv)->GetMethodID(javaEnv, class_ClientGame, "shutdownClientGame", "()V");
	method_ClientGame_drawActiveFrame = (*javaEnv)->GetMethodID(javaEnv, class_ClientGame, "drawActiveFrame", "(IIZ)V");
	method_ClientGame_keyEvent = (*javaEnv)->GetMethodID(javaEnv, class_ClientGame, "keyEvent", "(IIZ)V");
	method_ClientGame_mouseEvent = (*javaEnv)->GetMethodID(javaEnv, class_ClientGame, "mouseEvent", "(III)V");
	method_ClientGame_eventHandling = (*javaEnv)->GetMethodID(javaEnv, class_ClientGame, "eventHandling", "(I)V");
	method_ClientGame_lastAttacker = (*javaEnv)->GetMethodID(javaEnv, class_ClientGame, "lastAttacker", "()I");
	method_ClientGame_crosshairPlayer = (*javaEnv)->GetMethodID(javaEnv, class_ClientGame, "crosshairPlayer", "()I");
	method_ClientGame_consoleCommand = (*javaEnv)->GetMethodID(javaEnv, class_ClientGame, "consoleCommand", "()Z");
	if(CheckException())
	{
		Com_Error(ERR_DROP, "Problem getting handle for one or more of the ClientGame methods\n");
	}

	// load constructor
	method_ClientGame_ctor = (*javaEnv)->GetMethodID(javaEnv, class_ClientGame, "<init>", "(III)V");

	object_ClientGame = (*javaEnv)->NewObject(javaEnv, class_ClientGame, method_ClientGame_ctor, serverMessageNum, serverCommandSequence, clientNum);
	if(CheckException())
	{
		Com_Error(ERR_DROP, "Couldn't create instance of the class ClientGame(serverMessageNum = %i, serverCommandSequence = %i, clientNum = %i)", serverMessageNum, serverCommandSequence, clientNum);
	}
}


static void ClientGame_javaDetach()
{
	Com_Printf("ClientGame_javaDetach()\n");

	if(javaEnv)
	{
		if(class_ClientGame)
		{
			(*javaEnv)->DeleteLocalRef(javaEnv, class_ClientGame);
			class_ClientGame = NULL;
		}

		if(object_ClientGame)
		{
			(*javaEnv)->DeleteLocalRef(javaEnv, object_ClientGame);
			object_ClientGame = NULL;
		}
	}
}

static void Java_CG_Shutdown(void)
{
	if(!object_ClientGame)
		return;

	Com_Printf("Java_CG_Shutdown\n");

	(*javaEnv)->CallVoidMethod(javaEnv, object_ClientGame, method_ClientGame_shutdownClientGame);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_CG_Shutdown()");
	}
}

static qboolean Java_CG_ConsoleCommand(void)
{
	jboolean result;

	if(!object_ClientGame)
		return qfalse;

	result = (*javaEnv)->CallBooleanMethod(javaEnv, object_ClientGame, method_ClientGame_consoleCommand);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_CG_ConsoleCommand()");
	}

	return result;
}

static void Java_CG_DrawActiveFrame(int serverTime, int stereoView, qboolean demoPlayback)
{
	if(!object_ClientGame)
		return;

	//Com_Printf("Java_CG_DrawActiveFrame\n");

	(*javaEnv)->CallVoidMethod(javaEnv, object_ClientGame, method_ClientGame_drawActiveFrame, serverTime, stereoView, demoPlayback);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occured during Java_CG_DrawActiveFrame(serverTime = %i, stereoView = %i, demoPlayback = %i)", serverTime, stereoView, demoPlayback);
	}
}

void Java_CG_KeyEvent(int key, qboolean down)
{
	if(!object_ClientGame)
		return;

	Com_Printf("Java_CG_KeyEvent(key = %i, down = %i)\n", key, down);

	(*javaEnv)->CallVoidMethod(javaEnv, object_ClientGame, method_ClientGame_keyEvent, cls.realtime, key, down);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_CG_KeyEvent(key = %i, down = %i)", key, down);
	}
}

void Java_CG_MouseEvent(int dx, int dy)
{
	if(!object_ClientGame)
		return;

	Com_Printf("Java_UI_MouseEvent(dx = %i, dy = %i)\n", dx, dy);

	(*javaEnv)->CallVoidMethod(javaEnv, object_ClientGame, method_ClientGame_mouseEvent, cls.realtime, dx, dy);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_CG_MouseEvent(dx = %i, dy = %i)", dx, dy);
	}
}

void Java_CG_EventHandling(int type)
{
	if(!object_ClientGame)
		return;

	Com_Printf("Java_UI_EventHandling(type = %i)\n", type);

	(*javaEnv)->CallVoidMethod(javaEnv, object_ClientGame, method_ClientGame_mouseEvent, type);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_CG_EventHandling(type = %i)", type);
	}
}

int Java_CG_CrosshairPlayer(void)
{
	jint result;

	if(!object_ClientGame)
		return qfalse;

	Com_Printf("Java_CG_CrosshairPlayer\n");

	result = (*javaEnv)->CallIntMethod(javaEnv, object_ClientGame, method_ClientGame_crosshairPlayer);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_CG_CrosshairPlayer");
	}

	return result;
}

int Java_CG_LastAttacker(void)
{
	jint result;

	if(!object_ClientGame)
		return qfalse;

	Com_Printf("Java_CG_LastAttacker\n");

	result = (*javaEnv)->CallIntMethod(javaEnv, object_ClientGame, method_ClientGame_lastAttacker);

	if(CheckException())
	{
		Com_Error(ERR_DROP, "Java exception occurred during Java_CG_LastAttacker");
	}

	return result;
}


// ====================================================================================


/*
====================
CL_ShutdonwCGame
====================
*/
void CL_ShutdownCGame(void)
{
	Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_CGAME);
	cls.cgameStarted = qfalse;

	if(!javaEnv)
	{
		Com_Printf("Can't stop Java user interface module, javaEnv pointer was null\n");
		return;
	}

	CheckException();

	Java_CG_Shutdown();

	CheckException();

	ClientGame_javaDetach();
}


/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/
void CL_InitCGame(void)
{
	const char     *info;
	const char     *mapname;
	int             t1, t2;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(cl.mapname, sizeof(cl.mapname), "maps/%s.bsp", mapname);

	// init for this gamestate using the ClientGame constructor

	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	ClientGame_javaRegister(clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum);

	cls.state = CA_LOADING;

	// reset any CVAR_CHEAT cvars registered by cgame
	if(!clc.demoplaying && !cl_connectedToCheatServer)
		Cvar_SetCheatState();

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf("CL_InitCGame: %5.2f seconds\n", (t2 - t1) / 1000.0);

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
	if(!Sys_LowPhysicalMemory())
	{
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand(void)
{
	return Java_CG_ConsoleCommand();
}


/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering(stereoFrame_t stereo)
{
	Java_CG_DrawActiveFrame(cl.serverTime, stereo, clc.demoplaying);
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define	RESET_TIME	500

void CL_AdjustTimeDelta(void)
{
	int             resetTime;
	int             newDelta;
	int             deltaDelta;

	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if(clc.demoplaying)
	{
		return;
	}

	// if the current time is WAY off, just correct to the current value
	if(com_sv_running->integer)
	{
		resetTime = 100;
	}
	else
	{
		resetTime = RESET_TIME;
	}

	newDelta = cl.snap.serverTime - cls.realtime;
	deltaDelta = abs(newDelta - cl.serverTimeDelta);

	if(deltaDelta > RESET_TIME)
	{
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;
		if(cl_showTimeDelta->integer)
		{
			Com_Printf("<RESET> ");
		}
	}
	else if(deltaDelta > 100)
	{
		// fast adjust, cut the difference in half
		if(cl_showTimeDelta->integer)
		{
			Com_Printf("<FAST> ");
		}
		cl.serverTimeDelta = (cl.serverTimeDelta + newDelta) >> 1;
	}
	else
	{
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if(com_timescale->value == 0 || com_timescale->value == 1)
		{
			if(cl.extrapolatedSnapshot)
			{
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			}
			else
			{
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if(cl_showTimeDelta->integer)
	{
		Com_Printf("%i ", cl.serverTimeDelta);
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot(void)
{
	// ignore snapshots that don't have entities
	if(cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE)
	{
		return;
	}
	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if(cl_activeAction->string[0])
	{
		Cbuf_AddText(cl_activeAction->string);
		Cvar_Set("activeAction", "");
	}

#ifdef USE_MUMBLE
	if((cl_useMumble->integer) && !mumble_islinked())
	{
		int             ret = mumble_link(CLIENT_WINDOW_TITLE);

		Com_Printf("Mumble: Linking to Mumble application %s\n", ret == 0 ? "ok" : "failed");
	}
#endif

#ifdef USE_VOIP
	if(!clc.speexInitialized)
	{
		int             i;

		speex_bits_init(&clc.speexEncoderBits);
		speex_bits_reset(&clc.speexEncoderBits);

		clc.speexEncoder = speex_encoder_init(&speex_nb_mode);

		speex_encoder_ctl(clc.speexEncoder, SPEEX_GET_FRAME_SIZE, &clc.speexFrameSize);
		speex_encoder_ctl(clc.speexEncoder, SPEEX_GET_SAMPLING_RATE, &clc.speexSampleRate);

		clc.speexPreprocessor = speex_preprocess_state_init(clc.speexFrameSize, clc.speexSampleRate);

		i = 1;
		speex_preprocess_ctl(clc.speexPreprocessor, SPEEX_PREPROCESS_SET_DENOISE, &i);

		i = 1;
		speex_preprocess_ctl(clc.speexPreprocessor, SPEEX_PREPROCESS_SET_AGC, &i);

		for(i = 0; i < MAX_CLIENTS; i++)
		{
			speex_bits_init(&clc.speexDecoderBits[i]);
			speex_bits_reset(&clc.speexDecoderBits[i]);
			clc.speexDecoder[i] = speex_decoder_init(&speex_nb_mode);
			clc.voipIgnore[i] = qfalse;
			clc.voipGain[i] = 1.0f;
		}
		clc.speexInitialized = qtrue;
		clc.voipMuteAll = qfalse;
		Cmd_AddCommand("voip", CL_Voip_f);
		Cvar_Set("cl_voipSendTarget", "all");
		clc.voipTarget1 = clc.voipTarget2 = clc.voipTarget3 = 0x7FFFFFFF;
	}
#endif
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime(void)
{
	// getting a valid frame message ends the connection process
	if(cls.state != CA_ACTIVE)
	{
		if(cls.state != CA_PRIMED)
		{
			return;
		}
		if(clc.demoplaying)
		{
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if(!clc.firstDemoFrameSkipped)
			{
				clc.firstDemoFrameSkipped = qtrue;
				return;
			}
			CL_ReadDemoMessage();
		}
		if(cl.newSnapshots)
		{
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if(cls.state != CA_ACTIVE)
		{
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if(!cl.snap.valid)
	{
		Com_Error(ERR_DROP, "CL_SetCGameTime: !cl.snap.valid");
	}

	// allow pause in single player
	if(sv_paused->integer && CL_CheckPaused() && com_sv_running->integer)
	{
		// paused
		return;
	}

	if(cl.snap.serverTime < cl.oldFrameServerTime)
	{
		Com_Error(ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime");
	}
	cl.oldFrameServerTime = cl.snap.serverTime;


	// get our current view of time

	if(clc.demoplaying && cl_freezeDemo->integer)
	{
		// cl_freezeDemo is used to lock a demo in place for single frame advances

	}
	else
	{
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int             tn;

		tn = cl_timeNudge->integer;
		if(tn < -30)
		{
			tn = -30;
		}
		else if(tn > 30)
		{
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if(cl.serverTime < cl.oldServerTime)
		{
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if(cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5)
		{
			cl.extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if(cl.newSnapshots)
	{
		CL_AdjustTimeDelta();
	}

	if(!clc.demoplaying)
	{
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if(cl_timedemo->integer)
	{
		int             now = Sys_Milliseconds();
		int             frameDuration;

		if(!clc.timeDemoStart)
		{
			clc.timeDemoStart = clc.timeDemoLastFrame = now;
			clc.timeDemoMinDuration = INT_MAX;
			clc.timeDemoMaxDuration = 0;
		}

		frameDuration = now - clc.timeDemoLastFrame;
		clc.timeDemoLastFrame = now;

		// Ignore the first measurement as it'll always be 0
		if(clc.timeDemoFrames > 0)
		{
			if(frameDuration > clc.timeDemoMaxDuration)
				clc.timeDemoMaxDuration = frameDuration;

			if(frameDuration < clc.timeDemoMinDuration)
				clc.timeDemoMinDuration = frameDuration;

			// 255 ms = about 4fps
			if(frameDuration > UCHAR_MAX)
				frameDuration = UCHAR_MAX;

			clc.timeDemoDurations[(clc.timeDemoFrames - 1) % MAX_TIMEDEMO_DURATIONS] = frameDuration;
		}

		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	while(cl.serverTime >= cl.snap.serverTime)
	{
		// feed another messag, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();
		if(cls.state != CA_ACTIVE)
		{
			return;				// end of demo
		}
	}

}

#endif // !defined(USE_JAVA)
