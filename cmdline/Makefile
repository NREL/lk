VPATH = ../src 

CC = gcc
CXX = g++
CFLAGS = -I../include -Wall -O2 -g
CXXFLAGS = $(CFLAGS)

OBJECTS = \
	lkcmdline.o \
	absyn.o \
	codegen.o \
	env.o \
	eval.o \
	invoke.o \
	lex.o \
	parse.o \
	stdlib.o \
	vm.o



lk.exe: $(OBJECTS)
	g++  -o $@ $^

clean:
	del lk.exe