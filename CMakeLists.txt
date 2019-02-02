set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

if(MSVC)
 	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Debug and Release Builds Configured" FORCE)
endif()

Project(lk)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

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

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# setup wxWidgets
set(wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config-3)
find_package(wxWidgets COMPONENTS core base stc html xrc richtext propgrid aui gl qa adv net REQUIRED)
include(${wxWidgets_USE_FILE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}")

# set build types for lk archive and sandbox
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_library(lkosxwx3d STATIC ${LK_SRC})
	set_property(TARGET lkosxwx3d PROPERTY PREFIX "")
	add_executable(sandboxd ${LK_SRC} build_vs2017/sandbox/sandbox.cpp)
	target_link_libraries(sandboxd ${wxWidgets_LIBRARIES})

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
else()
	add_library(lkosxwx3 STATIC ${LK_SRC})
	set_property(TARGET lkosxwx3 PROPERTY PREFIX "")
	add_executable(sandbox ${LK_SRC} build_vs2017/sandbox/sandbox.cpp)
	target_link_libraries(sandbox ${wxWidgets_LIBRARIES})
endif()



if(MSVC)
# need msvc optimization levels & different configs, windows sdk
	add_compile_options(/W4 /WX)
	add_compile_definitions(LK_USE_WXWIDGETS _CRT_SECURE_NO_WARNINGS _MBCS __WXMSW__)
else(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	add_compile_options(-Wall -O2 -arch x86_64 -fno-common)
	add_compile_definitions(LK_USE_WXWIDGETS WX_PRECOMP)
endif(MSVC)
