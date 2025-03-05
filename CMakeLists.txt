cmake_minimum_required(VERSION 3.10)

project(libquickjs)
set(CMAKE_CONFIGURATION_TYPES Debug Release)
if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")

#set(CMAKE_MAKE_PROGRAM D:/msys64/usr/bin/make)
#set(CMAKE_MAKE_PROGRAM /mingw32/bin/ninja.exe -j4)
set(CMAKE_CXX_COMPILE_FEATURES "${CMAKE_CXX_COMPILE_FEATURES} /MT /GR- /utf-8")
#enable debug
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
#set output small size
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")

if (CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    message(STATUS "Overriding incorrect Linux detection for MSYS")
    set(CMAKE_SYSTEM_NAME Windows CACHE STRING "Target system" FORCE)
endif()
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND MINGW)
    message(STATUS "Overriding incorrect Linux detection for MINGW")
    set(CMAKE_SYSTEM_NAME Windows CACHE STRING "Target system" FORCE)
endif()

if (NOT CMAKE_SYSTEM_NAME MATCHES Windows)
add_subdirectory(swinx)
include_directories(swinx/include)
endif()

file(STRINGS "VERSION" QJS_VERSION_STR)


add_definitions(-DQJS_DLL -DWINVER=0x0501 -D_WIN32_WINNT=0x0501)

set(SOURCES
	quickjs.h
	quickjs-libc.h
	quickjs.c
    libbf.c
	libregexp.c
	libunicode.c
	cutils.c
	quickjs-libc.c
	quickjs-debugger.c
)

if (CMAKE_SYSTEM_NAME MATCHES Windows)
set(SOURCES ${SOURCES} quickjs-debugger-transport-win.c)
else()
set(SOURCES ${SOURCES} quickjs-debugger-transport-unix.c)
endif()

# build quickjs static library
add_library(quickjs SHARED  ${SOURCES})
target_link_libraries(quickjs pthread)
target_compile_definitions(quickjs PRIVATE
    CONFIG_BIGNUM
    CONFIG_VERSION="${QJS_VERSION_STR}"
	QJS_BUILD
)

set_target_properties(quickjs PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if (CMAKE_SYSTEM_NAME MATCHES Windows)
target_link_libraries(quickjs ws2_32)
else()
target_link_libraries(quickjs swinx)
endif()
# build qjsc program
set(
	QJSC_SOURCE
	qjsc.c
)

add_executable(qjsc ${QJSC_SOURCE})
target_link_libraries(qjsc quickjs)
target_compile_definitions(qjsc PRIVATE
	CONFIG_BIGNUM
    CONFIG_VERSION="${QJS_VERSION_STR}"
)

add_custom_target(gen_repl_calc
COMMAND ${CMAKE_BINARY_DIR}/qjsc -fbignum -c -o qjscalc.c qjscalc.js
COMMAND ${CMAKE_BINARY_DIR}/qjsc -c -o repl.c -m repl.js
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
DEPENDS qjsc
)

# build qjs program
set(
	QJS_SOURCE
	qjs.c
	repl.c
	qjscalc.c
)

add_executable(qjs ${QJS_SOURCE})
target_link_libraries(qjs quickjs)
target_compile_definitions(qjs PRIVATE
	CONFIG_BIGNUM
    CONFIG_VERSION="${QJS_VERSION_STR}"
)
add_dependencies(qjs gen_repl_calc)
