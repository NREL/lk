VPATH = ..
CC = gcc
CXX = g++
CFLAGS = -arch i386 -fPIC -Wall -O2 -D__WX__ -D__WXMAC__ -DLK_USE_WXWIDGETS `wx-config-dsa --cflags`
CXXFLAGS = $(CFLAGS)
LDFLAGS = `wx-config-dsa --libs` -lm -lstdc++ -arch i386

LIBNAME = lk
TARGET = $(LIBNAME).dylib

OBJECTS = \
	lk_absyn.o \
	lk_env.o \
	lk_eval.o \
	lk_lex.o \
	lk_math.o \
	lk_invoke.o \
	lk_parse.o \
	lk_stdlib.o

$(TARGET) : $(OBJECTS)
	$(CXX) $(LDFLAGS) -dynamiclib -flat_namespace -shared -o $@ $^ 

clean:
	rm -rf *.o $(TARGET)

