VPATH = ../src
CC = gcc
CXX = g++
CFLAGS = -I../include -fPIC -Wall -O2 -DLK_USE_WXWIDGETS `wx-config-3 --cflags`
CXXFLAGS = $(CFLAGS)

LIBNAME = lkuxwx3
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

