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
set(CLIENT_INCLUDE_TOPLEVEL_DIR "include")
set(CLIENT_INCLUDE_CLIENT_DIR "${CLIENT_INCLUDE_TOPLEVEL_DIR}/hat/client")
set(CLIENT_SRC_DIR "src")

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
set(CLIENT_SRC_SERVER_SHARED_LIST
	# Server shared (needed for MSVC)
	"${SERVER_INCLUDE_SERVER_DIR}/bg_public.h"
)

set(CLIENT_SRC_HEADER_LIST
	# Client shared (needed for MSVC)
	"${CLIENT_INCLUDE_CLIENT_DIR}/cg_public.h"
	"${CLIENT_INCLUDE_CLIENT_DIR}/cg_local.h"
	"${CLIENT_INCLUDE_CLIENT_DIR}/traps.hpp"
)

set(CLIENT_SRC_ENGINE_SHARED_LIST
	# Engine shared
	"${ENGINE_INCLUDE_ENGINE_DIR}/q_shared.h"
	"${ENGINE_INCLUDE_ENGINE_DIR}/surfaceflags.h"
)

set(CLIENT_SRC_LIST
	# Server specific code (shared)
	"${SERVER_SRC_DIR}/bg_misc.c"
	"${SERVER_SRC_DIR}/bg_pmove.c"
	"${SERVER_SRC_DIR}/bg_slidemove.c"
	# Engine specific code (shared)
	"${ENGINE_SRC_DIR}/q_math.c"
	"${ENGINE_SRC_DIR}/q_shared.c"
	# Client specific code
	"${CLIENT_INCLUDE_CLIENT_DIR}/client.def"
	"${CLIENT_SRC_DIR}/cg_animation.c"
	"${CLIENT_SRC_DIR}/cg_consolecmds.c"
	"${CLIENT_SRC_DIR}/cg_draw.c"
	"${CLIENT_SRC_DIR}/cg_drawtools.c"
	"${CLIENT_SRC_DIR}/cg_effects.c"
	"${CLIENT_SRC_DIR}/cg_ents.c"
	"${CLIENT_SRC_DIR}/cg_event.c"
	"${CLIENT_SRC_DIR}/cg_info.c"
	"${CLIENT_SRC_DIR}/cg_localents.c"
	"${CLIENT_SRC_DIR}/cg_main.c"
	"${CLIENT_SRC_DIR}/cg_marks.c"
	"${CLIENT_SRC_DIR}/cg_osd.c"
	"${CLIENT_SRC_DIR}/cg_particles.c"
	"${CLIENT_SRC_DIR}/cg_players.c"
	"${CLIENT_SRC_DIR}/cg_playerstate.c"
	"${CLIENT_SRC_DIR}/cg_predict.c"
	"${CLIENT_SRC_DIR}/cg_scoreboard.c"
	"${CLIENT_SRC_DIR}/cg_servercmds.c"
	"${CLIENT_SRC_DIR}/cg_snapshot.c"
	"${CLIENT_SRC_DIR}/cg_syscalls.cpp"
	"${CLIENT_SRC_DIR}/cg_view.c"
	"${CLIENT_SRC_DIR}/cg_weapons.cpp"
)

# Define the final source list
set(CLIENT_SRC_ALL_LIST
	${CLIENT_SRC_SERVER_SHARED_LIST}
	${CLIENT_SRC_CLIENT_SHARED_LIST}
	${CLIENT_SRC_HEADER_LIST}
	${CLIENT_SRC_LIST}
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
source_group("Header Files" FILES ${CLIENT_SRC_SERVER_SHARED_LIST} ${CLIENT_SRC_HEADER_LIST} ${CLIENT_SRC_ENGINE_SHARED_LIST})
source_group("Source Files" FILES ${CLIENT_SRC_LIST})

# Setup the library 
add_library(client SHARED ${CLIENT_SRC_ALL_LIST})

set_target_properties(client PROPERTIES PREFIX "")

if(WIN32)
set_target_properties(client PROPERTIES LINK_FLAGS
	"/BASE:0x30000000 /DYNAMICBASE:NO"
)
endif()

