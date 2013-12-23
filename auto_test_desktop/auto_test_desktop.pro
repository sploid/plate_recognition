CONFIG(debug, debug|release) {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/debug
} else {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/release
}

CONFIG += console
INCLUDEPATH += $$_PRO_FILE_PWD_/../plate_recog_lib
LIBS += $${DESTDIR}/plate_recog_lib.lib
QT -= gui

SOURCES = \
		main.cpp
win32 {
	QMAKE_CXXFLAGS += /MP
	QMAKE_CXXFLAGS_WARN_ON = -W4
}

include(../opencv.pri)
