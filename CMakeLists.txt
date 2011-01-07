cmake_minimum_required(VERSION 2.8)
project(ROGUE_REBORN)

set(ROGUE_REBORN_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/engine/modules/)

option(HAT_EXPAND_DEPENDENCIES "If enabled, Visual Studio will show all the dependencies broken apart into their respective libraries." FALSE)
option(HAT_EXPAND_PLATFORM "If enabled, Visual Studio will show all the operating-system specific dependencies in their own folders." FALSE)

# Option sets
if(HAT_EXPAND_DEPENDENCIES)
	message(STATUS "Hat project dependencies will be expanded in build's IDE.")
endif(HAT_EXPAND_DEPENDENCIES)

if (HAT_EXPAND_PLATFORM)
	message(STATUS "Hat platform dependencies will be expanded in build's IDE.")
endif(HAT_EXPAND_PLATFORM)

set(AVAILABLE_PROJECTS gui client renderer engine physics)

# Setup each project
foreach(VAR ${AVAILABLE_PROJECTS})
	string(TOUPPER ${VAR}_INCLUDE_DIR INCLUDE_VAR)
	string(TOUPPER ${VAR}_SRC_DIR SRC_VAR)
	string(TOUPPER ${VAR}_HAT_DIR HAT_VAR)
	set(${INCLUDE_VAR} ${ROGUE_REBORN_DIR}/${VAR}/include)
	set(${HAT_VAR} ${${INCLUDE_VAR}}/hat/${VAR}/)
	set(${SRC_VAR} ${ROGUE_REBORN_DIR}/${VAR}/src)
endforeach(VAR)

# Output
foreach(VAR ${AVAILABLE_PROJECTS})
	string(TOUPPER ${VAR}_INCLUDE_DIR INCLUDE_VAR)
	string(TOUPPER ${VAR}_SRC_DIR SRC_VAR)
	string(TOUPPER ${VAR}_HAT_DIR HAT_VAR)
	message(STATUS "Generating ${VAR}...")
	message(STATUS "- include directory... ${${INCLUDE_VAR}}")
	message(STATUS "- hat directory....... ${${HAT_VAR}}")
	message(STATUS "- src directory....... ${${SRC_VAR}}")
	add_subdirectory(${VAR})
endforeach(VAR)
