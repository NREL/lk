VPATH = ..
CC = gcc
CXX = g++
CFLAGS = -arch i386 -fPIC -Wall -O2 -DLK_USE_WXWIDGETS `wx-config-dsa --cflags`
CXXFLAGS = $(CFLAGS)

LIBNAME = lkosxwx28
TARGET = ../$(LIBNAME).a

OBJECTS = \
	lk_absyn.o \
	lk_env.o \
	lk_eval.o \
	lk_invoke.o \
	lk_lex.o \
	lk_math.o \
	lk_parse.o \
	lk_stdlib.o

$(TARGET) : $(OBJECTS)
	ar rs $@ $^ 

clean:
	rm -rf *.o $(TARGET)

