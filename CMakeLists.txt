# .PHONY: all
# all:
# 	clang++ -Wall -std=c++20 -o bin/archiver -Ilib src/app/*.cpp
#	cp src/config/settings.json bin/settings.json
#	cp src/database/make.sql bin/make.sql

cmake_minimum_required(VERSION 3.19)

project(Archiver VERSION 0.1 LANGUAGES CXX)

# Add custom CMake FindModules to the module search path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Prevent in-source builds
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt.")
endif ()

# Enable C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Archiver_MSVC_Disabled_Warnings_Flags /wd4464 /wd4820)

set(Archiver_Compile_Flags -Wall
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
    -Werror> $<$<CXX_COMPILER_ID:MSVC>:${Archiver_MSVC_Disabled_Warnings_Flags}>)

set(Archiver_Util_Path ${CMAKE_CURRENT_SOURCE_DIR}/util)

# Build any libraries we may need

# Build our project


configure_file(src/config/version.h.in include/config/version.h)

# Create an object library targets to allow for a single compilation pass
add_library(Archiver_sources OBJECT "")

target_compile_options(Archiver_sources PRIVATE ${Archiver_Compile_Flags})

target_include_directories(Archiver_sources PUBLIC lib/)

# Settings for the app
add_executable(Archiver $<TARGET_OBJECTS:Archiver_sources>)

target_compile_options(Archiver PRIVATE ${Archiver_Compile_Flags})

target_link_libraries(Archiver PRIVATE Archiver_sources)

file(COPY_FILE ${CMAKE_SOURCE_DIR}/src/config/default_config.json ${CMAKE_CURRENT_BINARY_DIR}/config.json)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(test)