cmake_minimum_required(VERSION 3.7)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

Project(lk)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config-3)
find_package(wxWidgets REQUIRED)
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

add_library(lk STATIC ${LK_SRC})

if(MSVC)
# need msvc optimization levels & different settings
  add_compile_options(/W4 /WX)
  add_compile_definitions(K_USE_WXWIDGETS _CRT_SECURE_NO_WARNINGS _MBCS)
else(MSVC)
  add_compile_options(-Wall -O2 -DLK_USE_WXWIDGETS -DWX_PRECOMP -arch x86_64 -fno-common)
endif(MSVC)