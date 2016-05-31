VPATH = ../src
CC = gcc
CXX = g++ -mmacosx-version-min=10.9
CFLAGS = -I../include -fPIC -Wall -O2 -DLK_USE_WXWIDGETS `wx-config-3 --cflags`
CXXFLAGS = $(CFLAGS) -DWX_PRECOMP -O2 -arch x86_64 -std=gnu++11 -fno-common
LDFLAGS =  -arch x86_64  -framework IOKit -framework Carbon -framework Cocoa -framework AudioToolbox -framework System -framework OpenGL

LIBNAME = lkosxwx3
TARGET = ../$(LIBNAME).a

OBJECTS = \
	absyn.o \
	codegen.o \
	vm.o \
	env.o \
	eval.o \
	invoke.o \
	lex.o \
	parse.o \
	stdlib.o

$(TARGET) : $(OBJECTS)
	ar rs $@ $^ 

clean:
	rm -rf *.o $(TARGET)

