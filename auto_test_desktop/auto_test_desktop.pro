CONFIG(debug, debug|release) {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/debug
} else {
	DESTDIR = $$_PRO_FILE_PWD_/../bin/release
}

INCLUDEPATH += $$_PRO_FILE_PWD_/../plate_recog_lib
win32 {
	mingw {
		LIBS += -L$${DESTDIR} \
			-lplate_recog_lib
	} else {
		LIBS += $${DESTDIR}/plate_recog_lib.lib
	}
} android {
	LIBS += -L$${DESTDIR} \
		-lplate_recog_lib
}
QT += gui widgets

SOURCES = \
		main.cpp

include(../opencv.pri)

RESOURCES += \
		../other/neural_net.qrc
