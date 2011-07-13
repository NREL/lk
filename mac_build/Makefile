VPATH = ..
CC = gcc
CXX = g++
CFLAGS = -arch i386 -fPIC -Wall -O2
CXXFLAGS = $(CFLAGS)
LDFLAGS = -lm -lstdc++ -arch i386

LIBNAME = lk
TARGET = $(LIBNAME).dylib

OBJECTS = \
	lk_absyn.o \
	lk_env.o \
	lk_eval.o \
	lk_lex.o \
	lk_parse.o \
	lk_stdlib.o

$(TARGET) : $(OBJECTS)
	$(CXX) $(LDFLAGS) -dynamiclib -flat_namespace -shared -o $@ $^ 

clean:
	rm -rf *.o $(TARGET)

