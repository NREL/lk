TEMPLATE = lib
DESTDIR = $${PWD}/..

TARGET = $$qtLibraryTarget(lk)

HEADERS += \
	lk_absyn.h \
	lk_env.h \
	lk_eval.h \
	lk_lex.h \
	lk_parse.h \
	lk_stdlib.h


SOURCES += \
	lk_absyn.cpp \
	lk_env.cpp \
	lk_eval.cpp \
	lk_lex.cpp \
	lk_parse.cpp \
	lk_stdlib.cpp
