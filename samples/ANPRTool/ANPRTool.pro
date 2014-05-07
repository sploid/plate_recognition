TEMPLATE = app

QT += qml quick sensors multimedia

SOURCES += \
	main.cpp \
	cameraitem.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
#QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
	cameraitem.h
	
include(../../opencv.pri)

# Dependencies for camera
contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    ANDROID_EXTRA_LIBS = \
		$${OPENCV_DIR}/libs/armeabi-v7a/libnative_camera_r4.4.0.so
}




