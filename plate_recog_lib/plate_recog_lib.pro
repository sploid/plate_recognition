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
		sym_recog.h \
		utils.h
SOURCES = \
		binarizewolfjolion.cpp \
		engine.cpp \
		sym_recog.cpp

include(../opencv.pri)

INCLUDEPATH += C:/soft/alpr/tesseract/api C:/soft/alpr/tesseract/ccmain C:/soft/alpr/tesseract/ccutil C:/soft/alpr/tesseract/ccstruct
