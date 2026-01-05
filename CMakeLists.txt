cmake_minimum_required(VERSION 3.10)
project(LuaDecompiler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(lua_decompiler ${SOURCES})

# Enable strict warnings to catch issues early
if(MSVC)
    target_compile_options(lua_decompiler PRIVATE /W4)
else()
    target_compile_options(lua_decompiler PRIVATE -Wall -Wextra)
endif()
