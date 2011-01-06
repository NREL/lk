CC = gcc
CXX = g++
CFLAGS = -Wall -O2
CXXFLAGS = $(CFLAGS)
TARGET = langkit

OBJECTS = \
	lk_absyn.o \
	lk_lex.o \
	lk_parse.o \
	lk_pretty.o \
	test_parse.o

$(TARGET) : $(OBJECTS)
	$(CXX) -o $@ $^

clean:
	rm -rf $(OBJECTS) $(TARGET)
