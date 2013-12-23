CONFIG(debug, debug|release) {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/debug
} else {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/release
}

DEFINES += PLATE_RECOG_LIB
QT -= gui core
TEMPLATE = lib

HEADERS = \
		engine.h \
		syms.h \
		plate_recog_lib_config.h
SOURCES = \
		engine.cpp \
		syms.cpp
win32 {
	QMAKE_CXXFLAGS += /MP
	QMAKE_CXXFLAGS_WARN_ON = -W4
}

include(../opencv.pri)

