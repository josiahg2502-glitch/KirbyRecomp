cmake_minimum_required(VERSION 3.20)
project(KirbyRecomp C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GBARECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/gbarecomp"
    CACHE PATH "Path to this game's gbarecomp worktree")
set(RECOMP_UI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/recomp-ui"
    CACHE PATH "Path to the shared recomp-ui checkout")
set(GBARECOMP_RUNTIME_UI_ROOT "${RECOMP_UI_ROOT}"
    CACHE PATH "recomp-ui root used by the live GBA runtime adapter")

if(NOT EXISTS "${GBARECOMP_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "GBARECOMP_ROOT is not a gbarecomp checkout: ${GBARECOMP_ROOT}")
endif()

add_subdirectory("${GBARECOMP_ROOT}" gbarecomp_build EXCLUDE_FROM_ALL)

file(GLOB GAME_GENERATED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/*.cpp")

set(KIRBY_TARGET KirbyRecomp)
add_executable(KirbyRecomp src/main.cpp ${GAME_GENERATED_SOURCES})

set(GAME_RECOMPILED_SOURCES ${GAME_GENERATED_SOURCES})
list(FILTER GAME_RECOMPILED_SOURCES INCLUDE REGEX "/recompiled_[0-9]+\\.cpp$")
if(GAME_RECOMPILED_SOURCES)
    if(MSVC)
        set_source_files_properties(${GAME_RECOMPILED_SOURCES} PROPERTIES
            COMPILE_OPTIONS "/O1;/EHs-c-;/GR-")
    else()
        set_source_files_properties(${GAME_RECOMPILED_SOURCES} PROPERTIES
            COMPILE_OPTIONS "-O1;-g0;-fno-exceptions;-fno-rtti;-fno-strict-aliasing;-fno-math-errno")
    endif()
endif()

target_include_directories(${KIRBY_TARGET} PRIVATE
    "${GBARECOMP_ROOT}/src/armv4t"
    "${GBARECOMP_ROOT}/src/gba"
    "${GBARECOMP_ROOT}/src/runtime"
    "${GBARECOMP_ROOT}/src/debug")

target_link_libraries(${KIRBY_TARGET} PRIVATE
    "-Wl,--start-group"
    gbarecomp_runtime
    gbarecomp_debug
    gbarecomp_gba
    gbarecomp_armv4t
    "-Wl,--end-group")

target_compile_definitions(${KIRBY_TARGET} PRIVATE
    GBARECOMP_DEFAULT_DEBUG_PORT=19894
    GBARECOMP_WINDOW_TITLE="KirbyRecomp"
    GBARECOMP_DEFAULT_GAME_CONFIG="game.toml")

option(GBAGAME_RECOMP_UI "Build the recomp-ui launcher and in-game menu" ON)
if(GBAGAME_RECOMP_UI)
    if(NOT EXISTS "${RECOMP_UI_ROOT}/recomp_ui.cmake")
        message(FATAL_ERROR "recomp-ui is not available at ${RECOMP_UI_ROOT}")
    endif()
    include("${RECOMP_UI_ROOT}/recomp_ui.cmake")
    add_library(game_launcher_ui STATIC src/game_launcher_boot.cpp)
    set_target_properties(game_launcher_ui PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(game_launcher_ui PRIVATE
        "${GBARECOMP_ROOT}/src/runtime"
        "${GBARECOMP_ROOT}/src/gba"
        src)
    recomp_target_launcher_ui(game_launcher_ui CONSOLE gba)
    if(NOT TARGET SDL2::SDL2)
        find_package(SDL2 REQUIRED)
    endif()
    target_link_libraries(game_launcher_ui PUBLIC SDL2::SDL2)
    target_link_libraries(${KIRBY_TARGET} PRIVATE game_launcher_ui)
    set_property(SOURCE src/main.cpp APPEND PROPERTY
        COMPILE_DEFINITIONS GBAGAME_RECOMP_UI)
endif()

if(MSVC)
    target_link_options(KirbyRecomp PRIVATE /STACK:16777216)
elseif(MINGW OR WIN32)
    target_link_options(KirbyRecomp PRIVATE -Wl,--stack,16777216)
endif()
