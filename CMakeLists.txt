cmake_minimum_required(VERSION 3.0.0)

project(womf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_compile_definitions(NOMINMAX _USE_MATH_DEFINES) # Windows is trash
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_subdirectory(subprojects/glwrap)

# Define functions after add_subdirectory, so they are not overwritten
include(cmake/wall.cmake)
include(cmake/CMakeRC.cmake)

if(WOMF_ENABLE_ASAN)
  include(cmake/asan.cmake)
endif()

find_package(sol2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

find_library(LUAJIT_LIBRARY_DIR libluajit-5.1.a lua51 REQUIRED)
find_path(LUAJIT_INCLUDE_DIR luajit.h PATH_SUFFIXES luajit-2.1 luajit REQUIRED)
add_library(luajit INTERFACE)
target_include_directories(luajit INTERFACE ${LUAJIT_INCLUDE_DIR})
target_link_libraries(luajit INTERFACE ${LUAJIT_LIBRARY_DIR})
message(STATUS "LuaJIT Include Dir: ${LUAJIT_INCLUDE_DIR}")
message(STATUS "LuaJIT Library Dir: ${LUAJIT_LIBRARY_DIR}")

set(SRC
  animation.cpp
  buffer.cpp
  graphics.cpp
  keys.cpp
  main.cpp
  sdlw.cpp
)
list(TRANSFORM SRC PREPEND src/)

cmrc_add_resource_library(
  lua-source
  NAMESPACE luaSource
  WHENCE src/lua/
  src/lua/cpml/init.lua
  src/lua/cpml/modules/bound2.lua
  src/lua/cpml/modules/bound3.lua
  src/lua/cpml/modules/bvh.lua
  src/lua/cpml/modules/color.lua
  src/lua/cpml/modules/constants.lua
  src/lua/cpml/modules/intersect.lua
  src/lua/cpml/modules/mat4.lua
  src/lua/cpml/modules/mesh.lua
  src/lua/cpml/modules/octree.lua
  src/lua/cpml/modules/_private_precond.lua
  src/lua/cpml/modules/_private_utils.lua
  src/lua/cpml/modules/quat.lua
  src/lua/cpml/modules/simplex.lua
  src/lua/cpml/modules/utils.lua
  src/lua/cpml/modules/vec2.lua
  src/lua/cpml/modules/vec3.lua
  src/lua/inspect.lua
  src/lua/json.lua

  src/lua/animation.lua
  src/lua/class.lua
  src/lua/gltf.lua
  src/lua/init.lua
)

add_executable(womf ${SRC})
target_include_directories(womf PRIVATE deps/sol_v3.3.0)
target_include_directories(womf PRIVATE include)
target_link_libraries(womf PRIVATE sol2)
target_link_libraries(womf PRIVATE fmt::fmt)
target_link_libraries(womf PRIVATE luajit)
target_link_libraries(womf PRIVATE glw)
target_link_libraries(womf PRIVATE glwx)
target_link_libraries(womf PRIVATE lua-source)
set_wall(womf)
