# Rogue Reborn Engine (Hat)
cmake_minimum_required(VERSION 2.8)

# Options
option(HAT_EXPAND_DEPENDENCIES "If enabled, Visual Studio will show all the dependencies broken apart into their respective libraries." FALSE)
option(HAT_EXPAND_PLATFORM "If enabled, Visual Studio will show all the operating-system specific dependencies in their own folders." FALSE)

# The only package really needed
find_package(Bullet REQUIRED)

# Setup our paths
set(ENGINE_INCLUDE_TOPLEVEL_DIR "../engine/include")


