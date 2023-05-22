cmake_minimum_required(VERSION 3.0)

project(libquickjs)

#set(CMAKE_MAKE_PROGRAM D:/msys64/usr/bin/make).
set(CMAKE_CXX_COMPILE_FEATURES "${CMAKE_CXX_COMPILE_FEATURES} /MT /GR- /utf-8")
#enable debug
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")

file(STRINGS "VERSION" QJS_VERSION_STR)

add_definitions(-DUNICODE -D_UNICODE -DQJS_DLL -DWINVER=0x0501 -D_WIN32_WINNT=0x0501)

include_directories(.)

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
	quickjs-debugger-transport-win.c
)

# build quickjs static library
add_library(quickjs SHARED  ${SOURCES})
target_link_libraries(quickjs -static pthread  ws2_32)
target_compile_definitions(quickjs PRIVATE
    CONFIG_BIGNUM
    CONFIG_VERSION="${QJS_VERSION_STR}"
	QJS_BUILD
)

set_target_properties(quickjs PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

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
OUTPUT  qjscalc.c repl.c 
COMMAND ${CMAKE_BINARY_DIR}/qjsc.exe -fbignum -c -o qjscalc.c qjscalc.js
COMMAND ${CMAKE_BINARY_DIR}/qjsc.exe -c -o repl.c -m repl.js
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
target_link_libraries(qjs quickjs ws2_32)
target_compile_definitions(qjs PRIVATE
	CONFIG_BIGNUM
    CONFIG_VERSION="${QJS_VERSION_STR}"
)
add_dependencies(qjs gen_repl_calc)
