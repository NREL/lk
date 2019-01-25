cmake_minimum_required(VERSION 3.7)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

Project(lk)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config-3)
find_package(wxWidgets COMPONENTS core base stc html REQUIRED)
include(${wxWidgets_USE_FILE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}")


include_directories(include src)

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

# static library
add_library(lkosxwx3 STATIC ${LK_SRC})
set_target_properties(lkosxwx3 PROPERTIES 
  PREFIX ""
  ARCHIVE_OUTPUT_DIRECTORY ..)

# sandbox executable
add_executable(sandbox ${LK_SRC} build_vs2017/sandbox/sandbox.cpp)
target_link_libraries(sandbox ${wxWidgets_LIBRARIES})
set_target_properties(sandbox PROPERTIES 
  PREFIX ""
  RUNTIME_OUTPUT_DIRECTORY ..)

if(MSVC)
# need msvc optimization levels & different configs, windows sdk
  add_compile_options(/W4 /WX)
  add_compile_definitions(LK_USE_WXWIDGETS _CRT_SECURE_NO_WARNINGS _MBCS)
else(MSVC)
  add_compile_options(-Wall -O2 -arch x86_64 -fno-common)
  add_compile_definitions(LK_USE_WXWIDGETS WX_PRECOMP)
endif(MSVC)