TEMPLATE = app

QT += qml quick sensors

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

OPENCV_DIR = C:\soft\OpenCV-2.4.9-android-sdk\sdk\native

	CONFIG += mobility
	equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
		INCLUDEPATH += $${OPENCV_DIR}/jni/include
		LIBS += -L$${OPENCV_DIR}/libs/armeabi-v7a \
				-L$${OPENCV_DIR}/3rdparty/libs/armeabi-v7a \
#				-lopencv_ml \
#				-lopencv_features2d \
#				-lopencv_objdetect \
#				-lopencv_ts \
#				-lopencv_contrib \
#				-lopencv_calib3d \
				-lopencv_highgui \
				-lopencv_androidcamera \
				-lopencv_imgproc \
				-lopencv_core \
				-ltbb \
				-llibjasper \
				-llibpng \
				-llibjpeg \
				-llibtiff \
				-lIlmImf
	}




