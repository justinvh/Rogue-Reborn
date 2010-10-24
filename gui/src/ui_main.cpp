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
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/


#include <hat/gui/ui_local.h>
#include <hat/gui/element.hpp>
#include <hat/gui/gui.hpp>
#include <fstream>

namespace hat {
	struct Gui_state { 
		Gui_state() : bad(false) { }
		const char* js_file;
		bool bad; 
		Gui* gui; 
	};
};

namespace {
	typedef std::map<int, hat::Gui_state> Cached_gui;
	Cached_gui available_guis;
	hat::Gui* active_gui;
	hat::Gui_kbm kbm_state;
	const char* broken_menu;
	int last_time = 0;
	int active_index = -1;
};


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
intptr_t vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9,
				int arg10, int arg11)
{
	switch (command)
	{
		case UI_GETAPIVERSION:
			return UI_API_VERSION;

		case UI_INIT: {
			trap_Key_SetCatcher(KEYCATCH_UI);
			// There is a general initialization script that is run
			// for the GUI subsystem. This basically makes sure that we
			// have some correlation between the GUI and the rest of
			// the engine.
			hat::Gui menu_init("base/guis/init.js");
			kbm_state.reset_all();
			return 0;
		}

		case UI_SHUTDOWN:
			active_gui =  NULL;
			broken_menu = NULL;
			kbm_state.reset_all();
            for (auto iter = available_guis.begin();
                iter != available_guis.end();
                ++iter)
            {
                delete iter->second.gui;
            }
			available_guis.clear();
			hat::Gui::engine_menu_clear();
			return 0;

		case UI_KEY_EVENT:
			kbm_state.key = arg0;
			kbm_state.down = (bool)arg1;
			return 0;

		case UI_MOUSE_EVENT:
			kbm_state.mx += arg0;
			kbm_state.my += arg1;
			return 0;

		case UI_REFRESH:
			// Whenever a refresh() is called, then we just call think()
			// on the GUI with the new KBM state. It's important to reset
			// the keys after the think() so we can figure out a new key
			// state after.
			active_gui->think(kbm_state);
			kbm_state.reset_keys();
			last_time = arg0;

			// Make sure that we didn't enter a run-time exception.
			// If this happens, then we need to set the state of the
			// GUI to invalid and alert the developer.
			if (active_gui->in_exception_state()) {
				const hat::Gui_exception& e = active_gui->exception();
				Com_Error(ERR_GUI, "<%s>:%d - %s", e.file.c_str(), e.line, e.message.c_str());
				available_guis[active_index].bad = true;
				broken_menu = active_gui->js_filename;
				active_index = -1;
				active_gui = NULL;
			}
		
			return 0;

		case UI_IS_FULLSCREEN:
			return 0;
			//return UI_IsFullscreen();

		case UI_RECOMPILE:
		case UI_SET_ACTIVE_MENU: {
            if (active_index == arg0) return 0;

			auto ci = available_guis.find(arg0);

			// Reset the active states
			active_index = -1;
			active_gui = NULL;

			// We can be in this state in two conditions. The first is that
			// we are setting a new menu, switching menus, or verifying
			// contexts. The second is that we are recompiling code
			// as a developer.
			if (ci == available_guis.end() || command == UI_RECOMPILE) {
				const char* menu = broken_menu == NULL ? 
					hat::Gui::engine_menu_exists(arg0) : broken_menu;

				// If we get the case that a menu represented in the engine
				// does not have a GUI representation, then we are in a
				// whole lot of bad.
				if (!menu) {
					Com_Error(ERR_FATAL, "The engine menu `%s` did not have a corresponding GUI.", hat::Gui::engine_menu_name(arg0));
					return -1;
				}

				// Create the GUI, check for exceptions, and do a few bits
				// of reporting to ensure everything looks right. The GUI
				// will be put into a state regardless of the success of
				// the constructor.
                hat::Gui* gui = new hat::Gui(menu);
				hat::Gui_state gui_state;
				if (gui->in_exception_state()) {
					const hat::Gui_exception& e = gui->exception();
					gui_state.bad = true;
					gui->shutdown();
					Com_Error(ERR_GUI, "<%s>:%d - %s", e.file.c_str(), e.line, e.message.c_str());
				}

				// This is the actual setting of the state for the GUI
				gui_state.js_file = menu;
				gui_state.gui = gui;
				available_guis[arg0] = gui_state;

                // We're only in an active GUI if the constructor was
                // successful, otherwise there should be an error in the
                // console at this point.
                if (!gui_state.bad) {
    				active_gui = available_guis[arg0].gui;
    				active_index = arg0;
                }

				return 0;
			}

			// The GUI is already in a bad state and the developer
			// doesn't want to lend a hand, so we ignore this pass.
			// Ensure that the active_gui is null so we don't start trying
			// to activate it.
			if (ci->second.bad) {
				active_gui = NULL;
				return -1;
			}

			// Everything looks good, now we can just set the active_gui
			// correctly so the refreshes and thinks work.
			active_gui = ci->second.gui;
			active_index = arg0;
			return 0;
		}

		case UI_CONSOLE_COMMAND:
			//return UI_ConsoleCommand(arg0);

		case UI_DRAW_CONNECT_SCREEN:
			//UI_DrawConnectScreen(arg0);
			return 0;
	}

	return -1;
}


/*
================
cvars
================
*/

typedef struct
{
	vmCvar_t       *vmCvar;
	char           *cvarName;
	char           *defaultString;
	int             cvarFlags;
} cvarTable_t;

vmCvar_t        ui_ffa_fraglimit;
vmCvar_t        ui_ffa_timelimit;

vmCvar_t        ui_tourney_fraglimit;
vmCvar_t        ui_tourney_timelimit;

vmCvar_t        ui_team_fraglimit;
vmCvar_t        ui_team_timelimit;
vmCvar_t        ui_team_friendly;

vmCvar_t        ui_ctf_capturelimit;
vmCvar_t        ui_ctf_timelimit;
vmCvar_t        ui_ctf_friendly;

vmCvar_t        ui_arenasFile;
vmCvar_t        ui_botsFile;
vmCvar_t        ui_spScores1;
vmCvar_t        ui_spScores2;
vmCvar_t        ui_spScores3;
vmCvar_t        ui_spScores4;
vmCvar_t        ui_spScores5;
vmCvar_t        ui_spAwards;
vmCvar_t        ui_spVideos;
vmCvar_t        ui_spSkill;

vmCvar_t        ui_spSelection;

vmCvar_t        ui_browserMaster;
vmCvar_t        ui_browserGameType;
vmCvar_t        ui_browserSortKey;
vmCvar_t        ui_browserShowFull;
vmCvar_t        ui_browserShowEmpty;

vmCvar_t        ui_brassTime;
vmCvar_t        ui_drawCrosshair;
vmCvar_t        ui_drawCrosshairNames;
vmCvar_t        ui_marks;

vmCvar_t        ui_server1;
vmCvar_t        ui_server2;
vmCvar_t        ui_server3;
vmCvar_t        ui_server4;
vmCvar_t        ui_server5;
vmCvar_t        ui_server6;
vmCvar_t        ui_server7;
vmCvar_t        ui_server8;
vmCvar_t        ui_server9;
vmCvar_t        ui_server10;
vmCvar_t        ui_server11;
vmCvar_t        ui_server12;
vmCvar_t        ui_server13;
vmCvar_t        ui_server14;
vmCvar_t        ui_server15;
vmCvar_t        ui_server16;

// bk001129 - made static to avoid aliasing.
static cvarTable_t cvarTable[] = {
	{&ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE},
	{&ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE},

	{&ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE},
	{&ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE},

	{&ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE},
	{&ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE},
	{&ui_team_friendly, "ui_team_friendly", "1", CVAR_ARCHIVE},

	{&ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE},
	{&ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE},
	{&ui_ctf_friendly, "ui_ctf_friendly", "0", CVAR_ARCHIVE},

	{&ui_arenasFile, "g_arenasFile", "", CVAR_INIT | CVAR_ROM},
	{&ui_botsFile, "g_botsFile", "", CVAR_INIT | CVAR_ROM},
	{&ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE | CVAR_ROM},
	{&ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE | CVAR_ROM},
	{&ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE | CVAR_ROM},
	{&ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE | CVAR_ROM},
	{&ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE | CVAR_ROM},
	{&ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE | CVAR_ROM},
	{&ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE | CVAR_ROM},
	{&ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH},

	{&ui_spSelection, "ui_spSelection", "", CVAR_ROM},

	{&ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE},
	{&ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE},
	{&ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE},
	{&ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE},
	{&ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE},

	{&ui_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE},
	{&ui_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE},
	{&ui_drawCrosshairNames, "cg_crosshairNames", "1", CVAR_ARCHIVE},
	{&ui_marks, "cg_marks", "1", CVAR_ARCHIVE},

	{&ui_server1, "server1", "", CVAR_ARCHIVE},
	{&ui_server2, "server2", "", CVAR_ARCHIVE},
	{&ui_server3, "server3", "", CVAR_ARCHIVE},
	{&ui_server4, "server4", "", CVAR_ARCHIVE},
	{&ui_server5, "server5", "", CVAR_ARCHIVE},
	{&ui_server6, "server6", "", CVAR_ARCHIVE},
	{&ui_server7, "server7", "", CVAR_ARCHIVE},
	{&ui_server8, "server8", "", CVAR_ARCHIVE},
	{&ui_server9, "server9", "", CVAR_ARCHIVE},
	{&ui_server10, "server10", "", CVAR_ARCHIVE},
	{&ui_server11, "server11", "", CVAR_ARCHIVE},
	{&ui_server12, "server12", "", CVAR_ARCHIVE},
	{&ui_server13, "server13", "", CVAR_ARCHIVE},
	{&ui_server14, "server14", "", CVAR_ARCHIVE},
	{&ui_server15, "server15", "", CVAR_ARCHIVE},
	{&ui_server16, "server16", "", CVAR_ARCHIVE}
};

// bk001129 - made static to avoid aliasing
static int      cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);


/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars(void)
{
	int             i;
	cvarTable_t    *cv;

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars(void)
{
	int             i;
	cvarTable_t    *cv;

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
	{
		trap_Cvar_Update(cv->vmCvar);
	}
}
