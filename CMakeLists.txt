#####################################################################################################################
#
# CMake and System Settings
#
#####################################################################################################################

set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.11)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
elseif(MSVC)
	set(CMAKE_SYSTEM_VERSION 8.1 CACHE TYPE INTERNAL FORCE)
endif()

if(UNIX AND NOT CMAKE_C_COMPILER)
	set(CMAKE_C_COMPILER gcc)
	set(CMAKE_CXX_COMPILER g++)
endif()


#####################################################################################################################
#
# Project Sources 
#
#####################################################################################################################

Project(lk)

include_directories(include)

set(LK_SRC
	src/absyn.cpp
	src/eval.cpp
	src/parse.cpp
	src/vm.cpp
	src/codegen.cpp
	src/invoke.cpp
	src/env.cpp
	src/lex.cpp
	src/sqlite3.c
	src/stdlib.cpp)


#####################################################################################################################
#
# Compile Options per Platform
#
#####################################################################################################################

set(CMAKE_CXX_STANDARD 11)

if(MSVC)
	add_compile_options(/W3 /wd4996)
	if (CMAKE_BUILD_TYPE STREQUAL Debug)
		add_compile_options(/MDd)
		add_compile_definitions(_DEBUG)
	else()
		add_compile_options(/MD /O2)
		add_compile_definitions(NDEBUG )
	endif()
	add_compile_definitions( WIN32 _CRT_SECURE_NO_DEPRECATE=1 _CRT_NON_CONFORMING_SWPRINTFS=1 
		_SCL_SECURE_NO_WARNINGS=1 __WXMSW__ _UNICODE NOPCH)
else(MSVC)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	if (APPLE)
		add_compile_options(-arch x86_64 -fno-common)
		add_compile_definitions(WX_PRECOMP)
	endif()
	add_compile_options(-Wall -O2 -Werror -Wno-deprecated -Wno-unused-function -Wno-deprecated-declarations)
	add_compile_definitions(LK_USE_WXWIDGETS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif(MSVC)


#####################################################################################################################
#
# WxWidgets Package
#
#####################################################################################################################

if (UNIX)
	set(wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config-3)
	find_package(wxWidgets REQUIRED xrc stc richtext ribbon propgrid aui gl html qa adv core xml net base )
else()
	set(wxWidgets_ROOT_DIR $ENV{WXMSW3})
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(wxWidgets_CONFIGURATION mswud)
	else()
		set(wxWidgets_CONFIGURATION mswu)
	endif()
	find_package(wxWidgets REQUIRED qa webview aui richtext html propgrid adv net stc core base scintilla)
endif()

include(${wxWidgets_USE_FILE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}")


#####################################################################################################################
# 
# CMake Targets
#
#####################################################################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_library(lkd STATIC ${LK_SRC})
	set(LK_LIB lkd)
	add_executable(sandboxd ${LK_SRC} sandbox/sandbox.cpp)
	set(SANDBOX_EXE sandboxd)

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
else()
	add_library(lk STATIC ${LK_SRC})
	set(LK_LIB lk)

	add_executable(sandbox ${LK_SRC} sandbox/sandbox.cpp)
	set(SANDBOX_EXE sandbox)
endif()

# Multi-configuration generators (VS, Xcode) append a per-configuration subdirectory 
# to the specified directory unless a generator expression is used
set_target_properties(${LK_LIB} PROPERTIES
	PREFIX ""
	ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>)
set_target_properties(${SANDBOX_EXE} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/sandbox)


#####################################################################################################################
#
# Link Libraries and Options
#
#####################################################################################################################

target_link_libraries(${SANDBOX_EXE} ${LK_LIB} ${wxWidgets_LIBRARIES})
if (MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
	target_compile_definitions(${LK_LIB} PUBLIC _LIB LK_USE_WXWIDGETS)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${SANDBOX_EXE})
endif()

if (${CMAKE_PROJECT_NAME} STREQUAL export_config)
	target_compile_definitions(${LK_LIB} PUBLIC _export_config_ )
endif()

if (UNIX)
	target_link_libraries(${SANDBOX_EXE} -ldl)
endif()