# Operating specific libraries and flags
if(WIN32)
	set(SPECIFIC_LINKER_FLAGS "/BASE:0x30000000 /DYNAMICBASE:NO")
	message(STATUS "- win32 linker flags.. ${SPECIFIC_LINKER_FLAGS}")
elseif(UNIX)
	set(SPECIFIC_LINKER_FLAGS "--image-base=0x30000000")
	message(STATUS "- unix linker flags... ${SPECIFIC_LINKER_FLAGS}")
endif(WIN32)

# Engine includes and sources
set(ENGINE_INCLUDES q_shared.h surfaceflags.h)
set(ENGINE_SRC q_math.c q_shared.c)

# Server includes and sources
set(SERVER_INCLUDES bg_public.h)
set(SERVER_SRC bg_misc.c bg_pmove.cpp bg_slidemove.c)

# Client includes and sources
set(CLIENT_INCLUDES cg_public.h cg_local.h traps.hpp client.def)
set(CLIENT_SRC
	cg_animation.c
	cg_consolecmds.c
	cg_draw.c
	cg_drawtools.c
	cg_effects.c
	cg_ents.c
	cg_event.c
	cg_info.c
	cg_localents.c
	cg_main.c
	cg_marks.c
	cg_osd.c
	cg_particles.c
	cg_players.c
	cg_playerstate.c
	cg_predict.c
	cg_scoreboard.c
	cg_servercmds.c
	cg_snapshot.c
	cg_syscalls.cpp
	cg_view.c
	cg_weapons.cpp
)

generate_hat_sources()

# Setup the library 
add_library(client SHARED ${SRC_ALL})
set_target_properties(client PROPERTIES PREFIX "")
set_target_properties(client PROPERTIES LINK_FLAGS ${SPECIFIC_LINKER_FLAGS})
add_definitions(-DCLIENT)

# Installation
install(TARGETS client DESTINATION ${CMAKE_INSTALL_PREFIX}/base/)
