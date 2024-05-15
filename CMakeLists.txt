#####################################################################################################################
#
# CMake and System Settings
#
#####################################################################################################################

set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.24)

option(SAM_SKIP_TOOLS "Skips the lk sandbox" OFF)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
endif ()

if (UNIX AND NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
endif ()


#####################################################################################################################
#
# Project Sources
#
#####################################################################################################################

Project(lk)

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

if (MSVC)
    add_compile_options(/W3 /wd4996 /MP)
    add_compile_definitions(WIN32 _CRT_SECURE_NO_DEPRECATE=1 _CRT_NON_CONFORMING_SWPRINTFS=1
            _SCL_SECURE_NO_WARNINGS=1 __WXMSW__ _UNICODE NOPCH LK_USE_WXWIDGETS)
    foreach (flag_var CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG)
        set(${flag_var} "${${flag_var}} /D_DEBUG" CACHE STRING "compile flags" FORCE)
    endforeach ()
else (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    if (APPLE)
        add_compile_options( -fno-common)
        add_definitions(-DWX_PRECOMP)
    endif ()
    add_compile_options(-Wall -O2 )
    add_definitions(-DLK_USE_WXWIDGETS)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(_DEBUG)
    else ()
        add_compile_options(-O3)
    endif ()
endif (MSVC)


#####################################################################################################################
#
# WxWidgets Package
#
#####################################################################################################################

if (UNIX)
    if(EXISTS /usr/local/bin/wx-config-3)
        set(wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config-3)
        find_package(wxWidgets REQUIRED xrc stc richtext ribbon propgrid aui gl html qa adv core xml net base)
    else ()
        set(wxWidgets_CONFIG_EXECUTABLE $ENV{WXMSW3}/bin/wx-config)
        find_package(wxWidgets REQUIRED xrc stc richtext ribbon propgrid aui gl html qa adv core xml net base)
    endif ()
else ()
    if(NOT EXISTS "$ENV{WXMSW3}/wxwidgets.props")
        set(wxWidgets_ROOT_DIR $ENV{WXMSW3})
        find_package(wxWidgets REQUIRED qa webview aui richtext html propgrid adv net stc core base scintilla)
        include(${wxWidgets_USE_FILE})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    endif()
endif ()


#####################################################################################################################
#
# CMake Targets
#
#####################################################################################################################

# lk library
add_library(lk STATIC ${LK_SRC})
set_target_properties(lk
        PROPERTIES
        DEBUG_POSTFIX "d"
        PREFIX ""
        )
if (MSVC)
    set_target_properties(lk
            PROPERTIES
            LINK_FLAGS /SUBSYSTEM:CONSOLE)
endif ()

target_include_directories(lk PRIVATE include)

# sandbox executable
if (NOT SAM_SKIP_TOOLS)
    add_executable(lk_sandbox ${LK_SRC} sandbox/sandbox.cpp)
    set_target_properties(lk_sandbox
            PROPERTIES
            DEBUG_POSTFIX "d"
            )
    if (MSVC)
        set_target_properties(lk_sandbox
                PROPERTIES
                LINK_FLAGS /SUBSYSTEM:WINDOWS)
    endif ()

    target_include_directories(lk_sandbox PUBLIC include)
endif()


#####################################################################################################################
#
# Link Libraries and Options
#
#####################################################################################################################

# lk library
if (${CMAKE_PROJECT_NAME} STREQUAL export_config)
    target_compile_definitions(lk PUBLIC _export_config_)
endif ()

# sandbox executable
if (NOT SAM_SKIP_TOOLS)
    if (UNIX)
        target_link_libraries(lk_sandbox -ldl)
    endif ()

    target_link_libraries(lk_sandbox lk)

    if (MSVC AND EXISTS "$ENV{WXMSW3}/wxwidgets.props")    
        set_property(
          TARGET lk_sandbox
          PROPERTY VS_USER_PROPS "$ENV{WXMSW3}/wxwidgets.props"
        )
    else()
        target_link_libraries(lk_sandbox ${wxWidgets_LIBRARIES})
    endif()
endif()

if (MSVC AND EXISTS "$ENV{WXMSW3}/wxwidgets.props")    
    set_property(
        TARGET lk
        PROPERTY VS_USER_PROPS "$ENV{WXMSW3}/wxwidgets.props"
    )
else()
    target_link_libraries(lk ${wxWidgets_LIBRARIES})
endif()
