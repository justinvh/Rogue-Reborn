# Rogue Reborn Engine (Hat)
cmake_minimum_required(VERSION 2.8)

# Options
option(HAT_EXPAND_DEPENDENCIES "If enabled, Visual Studio will show all the dependencies broken apart into their respective libraries." FALSE)
option(HAT_EXPAND_PLATFORM "If enabled, Visual Studio will show all the operating-system specific dependencies in their own folders." FALSE)

# Setup our paths
# Server
set(SERVER_INCLUDE_TOPLEVEL_DIR "include")
set(SERVER_INCLUDE_SERVER_DIR "${SERVER_INCLUDE_TOPLEVEL_DIR}/hat/server")
set(SERVER_SRC_DIR "src")

# Client
set(CLIENT_INCLUDE_TOPLEVEL_DIR "../client/include")
set(CLIENT_INCLUDE_CLIENT_DIR "${CLIENT_INCLUDE_TOPLEVEL_DIR}/hat/client")
set(CLIENT_SRC_DIR "../client/src")

# Gui
set(GUI_INCLUDE_TOPLEVEL_DIR "../gui/include")
set(GUI_INCLUDE_GUI_DIR "${GUI_INCLUDE_TOPLEVEL_DIR}/hat/gui")

# Renderer
set(RENDERER_INCLUDE_TOPLEVEL_DIR "../renderer/include")

# Various includes
set(ENGINE_INCLUDE_TOPLEVEL_DIR "../engine/include")
set(ENGINE_INCLUDE_ENGINE_DIR "${ENGINE_INCLUDE_TOPLEVEL_DIR}/hat/engine")
set(ENGINE_SRC_DIR "../engine/src")

# Source lists

set(SERVER_SRC_ENGINE_SHARED_LIST
	# Engine shared
	"${ENGINE_INCLUDE_ENGINE_DIR}/q_shared.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/q_platform.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/surfaceflags.h"
)

set(SERVER_SRC_HEADER_LIST
	"${SERVER_INCLUDE_SERVER_DIR}/acebot.h"
	"${SERVER_INCLUDE_SERVER_DIR}/bg_local.h"
	"${SERVER_INCLUDE_SERVER_DIR}/bg_public.h"
	"${SERVER_INCLUDE_SERVER_DIR}/g_local.h"
	"${SERVER_INCLUDE_SERVER_DIR}/g_public.h"
	"${SERVER_INCLUDE_SERVER_DIR}/g_team.h"
	"${SERVER_INCLUDE_SERVER_DIR}/match.h"
	"${SERVER_INCLUDE_SERVER_DIR}/syn.h"
)

set(SERVER_SRC_LIST
	# Server specific code (shared)
	"${SERVER_SRC_DIR}/bg_misc.c"
	"${SERVER_SRC_DIR}/bg_pmove.cpp"
	"${SERVER_SRC_DIR}/bg_slidemove.c"
	# Engine specific code (shared)
	"${ENGINE_SRC_DIR}/q_math.c"
	"${ENGINE_SRC_DIR}/q_shared.c"
	# Server specific code
	"${SERVER_INCLUDE_SERVER_DIR}/server.def"
	"${SERVER_SRC_DIR}/acebot_ai.c"
	"${SERVER_SRC_DIR}/acebot_cmds.c"
	"${SERVER_SRC_DIR}/acebot_items.c"
	"${SERVER_SRC_DIR}/acebot_movement.c"
	"${SERVER_SRC_DIR}/acebot_nodes.c"
	"${SERVER_SRC_DIR}/acebot_spawn.c"
	"${SERVER_SRC_DIR}/g_active.c"
	"${SERVER_SRC_DIR}/g_arenas.c"
	"${SERVER_SRC_DIR}/g_bot.c"
	"${SERVER_SRC_DIR}/g_client.c"
	"${SERVER_SRC_DIR}/g_cmds.cpp"
	"${SERVER_SRC_DIR}/g_combat.c"
	"${SERVER_SRC_DIR}/g_explosive.c"
	"${SERVER_SRC_DIR}/g_items.c"
	"${SERVER_SRC_DIR}/g_main.c"
	"${SERVER_SRC_DIR}/g_mem.c"
	"${SERVER_SRC_DIR}/g_misc.c"
	"${SERVER_SRC_DIR}/g_missile.c"
	"${SERVER_SRC_DIR}/g_mover.c"
	"${SERVER_SRC_DIR}/g_session.c"
	"${SERVER_SRC_DIR}/g_spawn.c"
	"${SERVER_SRC_DIR}/g_svcmds.c"
	"${SERVER_SRC_DIR}/g_syscalls.c"
	"${SERVER_SRC_DIR}/g_target.c"
	"${SERVER_SRC_DIR}/g_team.c"
	"${SERVER_SRC_DIR}/g_trigger.c"
	"${SERVER_SRC_DIR}/g_utils.c"
	"${SERVER_SRC_DIR}/g_weapon.c"
)

# Define the final source list
set(SERVER_SRC_ALL_LIST
	${SERVER_SRC_HEADER_LIST}
	${SERVER_SRC_ENGINE_SHARED_LIST}
	${SERVER_SRC_LIST}
)

# Project include
include_directories(
	"${ENGINE_INCLUDE_TOPLEVEL_DIR}"
	"${SERVER_INCLUDE_TOPLEVEL_DIR}"
	"${CLIENT_INCLUDE_TOPLEVEL_DIR}"
	"${GUI_INCLUDE_TOPLEVEL_DIR}"
	"${RENDERER_INCLUDE_TOPLEVEL_DIR}"
)

# Group the source code
source_group("Header Files" FILES ${SERVER_SRC_HEADER_LIST} ${SERVER_SRC_ENGINE_SHARED_LIST})
source_group("Source Files" FILES ${SERVER_SRC_LIST})

# Setup the library 
add_library(server SHARED ${SERVER_SRC_ALL_LIST})
add_definitions(-DGLOBALRANK -DSERVER)

set_target_properties(server PROPERTIES PREFIX "")

if(WIN32)
set_target_properties(server PROPERTIES LINK_FLAGS
	"/BASE:0x20000000 /DYNAMICBASE:NO"
)
endif()

install(TARGETS server DESTINATION ${CMAKE_INSTALL_PREFIX}/base/)

