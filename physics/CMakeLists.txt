# Rogue Reborn Physics
find_package(Bullet REQUIRED)

if(BULLET_FOUND)
	message(STATUS "- bullet include...... ${BULLET_INCLUDE_DIRS}")
	message(STATUS "- bullet libraries.... ${BULLET_LIBRARIES}")
	set(LIBRARIES ${BULLET_LIBRARIES} ${LIBRARIES})
endif(BULLET_FOUND)

# Operating specific libraries and flags
if(WIN32)
	if(NOT RR_DISABLE_IMAGE_BASE)
		set(SPECIFIC_LINKER_FLAGS "/BASE:0x50000000 /DYNAMICBASE:NO")
		message(STATUS "- win32 linker flags.. ${SPECIFIC_LINKER_FLAGS}")
	endif(NOT RR_DISABLE_IMAGE_BASE)
elseif(UNIX AND NOT RR_DISABLE_IMAGE_BASE)
	set(SPECIFIC_LINKER_FLAGS "--image-base=0x40000000")
	message(STATUS "- unix linker flags... ${SPECIFIC_LINKER_FLAGS}")
endif(WIN32)

set(PHYSICS_INCLUDES
	public.h
	traps.hpp
	physics.def
)

set(PHYSICS_SRC
	syscalls.cpp
	main.cpp
)

generate_hat_sources()

# Setup
add_library(physics SHARED ${SRC_ALL})
target_link_libraries(physics ${LIBRARIES})
set_target_properties(physics PROPERTIES PREFIX "")

if(DEFINED SPECIFIC_LINKER_FLAGS)
	set_target_properties(physics PROPERTIES 
		LINK_FLAGS ${SPECIFIC_LINKER_FLAGS})
endif(DEFINED SPECIFIC_LINKER_FLAGS)

if(RR_VISUAL_STUDIO_MP)
	set_target_properties(physics
		PROPERTIES COMPILE_FLAGS "/MP")
endif(RR_VISUAL_STUDIO_MP)

# Installation
install(TARGETS physics DESTINATION ${CMAKE_INSTALL_PREFIX}/base/)
