cmake_minimum_required(VERSION 2.8)
project(ROGUE_REBORN)

set(ROGUE_REBORN_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/engine/modules/)

add_subdirectory(engine)
add_subdirectory(client)
add_subdirectory(gui)
add_subdirectory(server)
add_subdirectory(renderer)
