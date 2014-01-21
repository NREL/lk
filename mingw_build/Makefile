VPATH = ..
CC = gcc
CXX = g++
CFLAGS = -Wall -O2
CXXFLAGS = $(CFLAGS)

LIBNAME = lk
TARGET = $(LIBNAME).dll
IMPLIB = $(LIBNAME).a

OBJECTS = \
	lk_absyn.o \
	lk_env.o \
	lk_eval.o \
	lk_lex.o \
	lk_parse.o \
	lk_stdlib.o

$(TARGET) : $(OBJECTS)
	$(CXX) -shared -Wl,--out-implib=$(IMPLIB) -o $(TARGET) $(OBJECTS) 

clean:
	rm -rf *.o $(TARGET) $(IMPLIB)

