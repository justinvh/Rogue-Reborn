# Rogue Reborn Engine (Hat)
cmake_minimum_required(VERSION 2.8)

# Options
option(HAT_EXPAND_DEPENDENCIES "If enabled, Visual Studio will show all the dependencies broken apart into their respective libraries." FALSE)
option(HAT_EXPAND_PLATFORM "If enabled, Visual Studio will show all the operating-system specific dependencies in their own folders." FALSE)

# Setup our paths
# Server
set(SERVER_INCLUDE_TOPLEVEL_DIR "../server/include")
set(SERVER_INCLUDE_SERVER_DIR "${SERVER_INCLUDE_TOPLEVEL_DIR}/hat/server")
set(SERVER_SRC_DIR "../server/src")

# Client
set(CLIENT_INCLUDE_TOPLEVEL_DIR "../client/include")
set(CLIENT_INCLUDE_CLIENT_DIR "${CLIENT_INCLUDE_TOPLEVEL_DIR}/hat/client")
set(CLIENT_SRC_DIR "../client/src")

# Gui
set(GUI_INCLUDE_TOPLEVEL_DIR "include")
set(GUI_INCLUDE_GUI_DIR "${GUI_INCLUDE_TOPLEVEL_DIR}/hat/gui")
set(GUI_SRC_DIR "src")

# Renderer
set(RENDERER_INCLUDE_TOPLEVEL_DIR "../renderer/include")

# Various includes
set(ENGINE_INCLUDE_TOPLEVEL_DIR "../engine/include")
set(ENGINE_INCLUDE_ENGINE_DIR "${ENGINE_INCLUDE_TOPLEVEL_DIR}/hat/engine")
set(ENGINE_SRC_DIR "../engine/src")

find_package(V8 REQUIRED)

# Source lists

set(GUI_SRC_ENGINE_SHARED_LIST
	# Engine shared
	"${ENGINE_INCLUDE_ENGINE_DIR}/q_shared.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/keycodes.h"
)

set(GUI_SRC_HEADER_LIST
	"${GUI_INCLUDE_GUI_DIR}/menudef.h"
	"${GUI_INCLUDE_GUI_DIR}/ui_local.h"
	"${GUI_INCLUDE_GUI_DIR}/ui_public.h"
	"${GUI_INCLUDE_GUI_DIR}/element.hpp"
	"${GUI_INCLUDE_GUI_DIR}/gui.hpp"
	"${GUI_INCLUDE_GUI_DIR}/easy.hpp"
	"${GUI_INCLUDE_GUI_DIR}/image.hpp"
)

set(GUI_SRC_LIST
	# Server specific code (shared)
	"${SERVER_SRC_DIR}/bg_misc.c"
	# Engine specific code (shared)
	"${ENGINE_SRC_DIR}/q_math.c"
	"${ENGINE_SRC_DIR}/q_shared.c"
	# Gui definitions
	"${GUI_INCLUDE_GUI_DIR}/gui.def"
	# Gui specific code
	"${GUI_SRC_DIR}/ui_main.cpp"
	"${GUI_SRC_DIR}/ui_syscalls.c"
	"${GUI_SRC_DIR}/ui_atoms.c"
	# New gui
	"${GUI_SRC_DIR}/element.cpp"
	"${GUI_SRC_DIR}/gui.cpp"
	"${GUI_SRC_DIR}/easy.cpp"
	"${GUI_SRC_DIR}/image.cpp"
)

set(GUI_SRC_EXT_LIST
	"${GUI_SRC_DIR}/ui_addbots.c"
	"${GUI_SRC_DIR}/ui_cinematics.c"
	"${GUI_SRC_DIR}/ui_confirm.c"
	"${GUI_SRC_DIR}/ui_connect.c"
	"${GUI_SRC_DIR}/ui_controls2.c"
	"${GUI_SRC_DIR}/ui_credits.c"
	"${GUI_SRC_DIR}/ui_demo2.c"
	"${GUI_SRC_DIR}/ui_display.c"
	"${GUI_SRC_DIR}/ui_gameinfo.c"
	"${GUI_SRC_DIR}/ui_ingame.c"
	"${GUI_SRC_DIR}/ui_loadconfig.c"
	"${GUI_SRC_DIR}/ui_menu.c"
	"${GUI_SRC_DIR}/ui_mfield.c"
	"${GUI_SRC_DIR}/ui_mods.c"
	"${GUI_SRC_DIR}/ui_network.c"
	"${GUI_SRC_DIR}/ui_options.c"
	"${GUI_SRC_DIR}/ui_playermodel.c"
	"${GUI_SRC_DIR}/ui_players.c"
	"${GUI_SRC_DIR}/ui_playersettings.c"
	"${GUI_SRC_DIR}/ui_preferences.c"
	"${GUI_SRC_DIR}/ui_qmenu.c"
	"${GUI_SRC_DIR}/ui_removebots.c"
	"${GUI_SRC_DIR}/ui_saveconfig.c"
	"${GUI_SRC_DIR}/ui_serverinfo.c"
	"${GUI_SRC_DIR}/ui_servers2.c"
	"${GUI_SRC_DIR}/ui_setup.c"
	"${GUI_SRC_DIR}/ui_sound.c"
	"${GUI_SRC_DIR}/ui_sparena.c"
	"${GUI_SRC_DIR}/ui_specifyserver.c"
	"${GUI_SRC_DIR}/ui_splevel.c"
	"${GUI_SRC_DIR}/ui_sppostgame.c"
	"${GUI_SRC_DIR}/ui_spreset.c"
	"${GUI_SRC_DIR}/ui_spskill.c"
	"${GUI_SRC_DIR}/ui_startserver.c"
	"${GUI_SRC_DIR}/ui_team.c"
	"${GUI_SRC_DIR}/ui_teamorders.c"
	"${GUI_SRC_DIR}/ui_video.c"
	"${GUI_SRC_DIR}/ui_xppm.c"
)

# Define the final source list
set(GUI_SRC_ALL_LIST
	${GUI_SRC_HEADER_LIST}
	${GUI_SRC_ENGINE_SHARED_LIST}
	${GUI_SRC_LIST}
)

# Project include
include_directories(
	"${ENGINE_INCLUDE_TOPLEVEL_DIR}"
	"${SERVER_INCLUDE_TOPLEVEL_DIR}"
	"${CLIENT_INCLUDE_TOPLEVEL_DIR}"
	"${GUI_INCLUDE_TOPLEVEL_DIR}"
	"${RENDERER_INCLUDE_TOPLEVEL_DIR}"
	"${V8_INCLUDE_DIR}"
)

if(WIN32)
	set(OS_SPECIFIC_LIBRARIES "winmm.lib" "ws2_32.lib")
endif()

# Group the source code
source_group("Header Files" FILES ${GUI_SRC_HEADER_LIST} ${GUI_SRC_ENGINE_SHARED_LIST})
source_group("Source Files" FILES ${GUI_SRC_LIST})

# Setup the library 
add_library(gui SHARED ${GUI_SRC_ALL_LIST})
target_link_libraries(gui ${V8_LIBRARY} ${OS_SPECIFIC_LIBRARIES})
set_target_properties(gui PROPERTIES PREFIX "")

if(WIN32)
	set_target_properties(gui PROPERTIES LINK_FLAGS
		"/BASE:0x40000000 /DYNAMICBASE:NO"
	)
endif()

