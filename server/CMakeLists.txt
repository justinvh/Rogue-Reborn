if(WIN32)
	set(SPECIFIC_LINKER_FLAGS "/BASE:0x20000000 /DYNAMICBASE:NO")
	message(STATUS "- win32 linker flags.. ${SPECIFIC_LINKER_FLAGS}")
elseif(UNIX)
	set(SPECIFIC_LINKER_FLAGS "--image-base=0x20000000")
	message(STATUS "- unix linker flags... ${SPECIFIC_LINKER_FLAGS}")
endif(WIN32)

set(ENGINE_INCLUDES q_shared.h q_platform.h surfaceflags.h)
set(ENGINE_SRC q_math.c q_shared.c)

set(SERVER_INCLUDES
	acebot.h
	bg_local.h
	bg_public.h
	g_local.h
	g_public.h
	g_team.h
	match.h
	syn.h
	server.def
)

set(SERVER_SRC
	acebot_ai.c
	acebot_cmds.c
	acebot_items.c
	acebot_movement.c
	acebot_nodes.c
	acebot_spawn.c
	g_active.c
	g_arenas.c
	g_bot.c
	g_client.c
	g_cmds.cpp
	g_combat.c
	g_explosive.c
	g_items.c
	g_main.c
	g_mem.c
	g_misc.c
	g_missile.c
	g_mover.c
	g_session.c
	g_spawn.c
	g_svcmds.c
	g_syscalls.c
	g_target.c
	g_team.c
	g_trigger.c
	g_utils.c
	g_weapon.c
)

generate_hat_sources()

# Setup
add_library(server SHARED ${SRC_ALL})
add_definitions(-DGLOBALRANK -DSERVER)
set_target_properties(server PROPERTIES PREFIX "")
set_target_properties(server PROPERTIES LINK_FLAGS ${SPECIFIC_LINKER_FLAGS})

# Installation
install(TARGETS server DESTINATION ${CMAKE_INSTALL_PREFIX}/base/)

