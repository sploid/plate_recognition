CONFIG(debug, debug|release) {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/debug
} else {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/release
}

INCLUDEPATH += $$_PRO_FILE_PWD_/../plate_recog_lib
win32 {
	QT -= gui
	CONFIG += console
	mingw {
		LIBS += -L$${DESTDIR} \
			-lplate_recog_lib
	} else {
		LIBS += $${DESTDIR}/plate_recog_lib.lib
	}
} android {
	QT += gui widgets
	LIBS += -L$${DESTDIR} \
		-lplate_recog_lib
} linux {
	LIBS += -L$${DESTDIR} \
		-lplate_recog_lib
}

SOURCES = \
		main.cpp

include(../opencv.pri)
