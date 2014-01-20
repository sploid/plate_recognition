CONFIG(debug, debug|release) {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/debug
} else {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/release
}

DEFINES += PLATE_RECOG_LIB
QT -= gui core
TEMPLATE = lib
CONFIG += static

HEADERS = \
		engine.h \
		figure.h \
		plate_recog_lib_config.h \
		sym_recog.h \
		syms.h \
		utils.h
SOURCES = \
		engine.cpp \
		syms.cpp \
		sym_recog.cpp

include(../opencv.pri)

