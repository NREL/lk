set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.12)


if(UNIX AND NOT CMAKE_C_COMPILER)
	set(CMAKE_C_COMPILER gcc)
	set(CMAKE_CXX_COMPILER g++)
else()
	set(CMAKE_SYSTEM_VERSION 8.1)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
endif()

Project(lk)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sandbox")
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

# Compiler options need to be set prior to adding targets
if(MSVC)
# need msvc optimization levels & different configs, windows sdk
	set(VS_STARTUP_PROJECT ${SANDBOX_EXE})
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
	add_compile_options(-Wall -O2 -Werror -Wno-deprecated -Wno-deprecated-declarations)
	add_compile_definitions(LK_USE_WXWIDGETS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif(MSVC)



# setup wxWidgets
if (UNIX)
	set(wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config-3)
	find_package(wxWidgets COMPONENTS xrc stc richtext ribbon propgrid aui gl html qa adv core xml net base REQUIRED)
else()
	set(wxWidgets_CONFIGURATION mswu)
	set(wxWidgets_ROOT_DIR "C:\\System Advisor Model Code\\wxWidgets-3.1.1")
	find_package(wxWidgets COMPONENTS qa webview std aui richtext html propgrid adv net core base REQUIRED)
endif()

include(${wxWidgets_USE_FILE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}")

# Create targets
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

set_property(TARGET ${LK_LIB} PROPERTY PREFIX "")
target_link_libraries(${SANDBOX_EXE} ${wxWidgets_LIBRARIES})
if (MSVC)
	target_compile_definitions(${SANDBOX_EXE} PUBLIC _WINDOWS)
	target_compile_definitions(${LK_LIB} PUBLIC _LIB)

endif()
