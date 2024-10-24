cmake_minimum_required(VERSION 3.19)
project(HelloClapHost)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(CMAKE_VERBOSE_MAKEFILE 1)
#set(RTMIDI_API_WINMM ON)
#add_definitions(-DRTMIDI_API_WINMM=ON)
add_definitions(-D__WINDOWS_MM__ -DRTMIDI_DEBUG)

# Automatically pick up files added to src
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
PRIVATE
"${CMAKE_CURRENT_SOURCE_DIR}/src"
"${CMAKE_CURRENT_SOURCE_DIR}/libs/clap-1.2.2/include/"
"${CMAKE_CURRENT_SOURCE_DIR}/libs/clap-helpers/include/"
"${CMAKE_CURRENT_SOURCE_DIR}/libs/jsoncpp/include/"
)
#"target_compile_options(jsoncpp PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations>)

target_link_libraries(${PROJECT_NAME} 
PRIVATE
    winmm
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/jsoncpp_static.lib"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX src FILES ${SOURCES})