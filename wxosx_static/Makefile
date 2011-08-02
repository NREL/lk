VPATH = ..
CC = gcc
CXX = g++
CFLAGS = -fPIC -Wall -O2 -D__WX__ -D__WXMAC__ `wx-config-29 --cflags`
CXXFLAGS = $(CFLAGS)

LIBNAME = lk
TARGET = $(LIBNAME).a

OBJECTS = \
	lk_absyn.o \
	lk_env.o \
	lk_eval.o \
	lk_lex.o \
	lk_parse.o \
	lk_stdlib.o

$(TARGET) : $(OBJECTS)
	ar rs $@ $^ 

clean:
	rm -rf *.o $(TARGET)

